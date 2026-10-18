/**
 * Doe 接网球规则
 */

#include "tennis.h"
#include <string.h>

static const uint16_t TIER_SCORE[TENNIS_TIERS] = TENNIS_TIER_SCORE;
static const uint16_t READY_MS[TENNIS_TIERS] = TENNIS_READY_MS;
static const uint16_t ARRIVE_MS[TENNIS_TIERS] = TENNIS_ARRIVE_MS;
static const uint16_t STAR_SCORE[3] = TENNIS_STAR_SCORE;
static const uint16_t SHELLS[4] = TENNIS_SHELLS_BY_STARS;

// xorshift32
static uint32_t next_rand(TennisGame* g) {
    uint32_t x = g->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g->rng = x;
    return x;
}

static uint32_t arrive_at(const TennisGame* g) {
    return (uint32_t)g->ready_ms + g->arrive_ms;
}

static int32_t zone_ms(const TennisGame* g) {
    return (int32_t)TENNIS_ZONE_MS * g->arrive_ms / TENNIS_SLOW_ARRIVE_MS;
}

static void start_serve(TennisGame* g) {
    int tier = 0;
    while (tier + 1 < TENNIS_TIERS && g->score >= TIER_SCORE[tier + 1]) tier++;
    g->serves++;
    g->ready_ms = READY_MS[tier];
    g->arrive_ms = ARRIVE_MS[tier];
    if (tier >= TENNIS_MIXED_TIER && next_rand(g) % 2) g->arrive_ms = TENNIS_SLOW_ARRIVE_MS;
    g->serve_ms = 0;
    g->jump_at = -1;
    g->catch_at = -1;
    g->phase = TENNIS_PHASE_READY;
    g->phase_ms = 0;
}

static void resolve(TennisGame* g, bool caught) {
    int32_t off = g->jump_at - ((int32_t)arrive_at(g) - TENNIS_JUMP_PEAK_MS);
    if (caught) {
        bool perfect = off >= -TENNIS_PERFECT_MS && off <= TENNIS_PERFECT_MS;
        g->last = perfect ? TENNIS_CATCH_PERFECT : TENNIS_CATCH_GOOD;
        g->score++;
        if (perfect) g->perfects++;
    } else {
        g->last = g->jump_at >= 0 && off < 0 ? TENNIS_MISS_EARLY : TENNIS_MISS_LATE;
        g->misses++;
    }
    g->phase = TENNIS_PHASE_RESOLVE;
    g->phase_ms = 0;
}

void tennis_init(TennisGame* g, uint32_t seed) {
    memset(g, 0, sizeof(*g));
    g->rng = seed ? seed : 0x9E3779B9u;
    g->phase = TENNIS_PHASE_COUNTDOWN;
    g->jump_at = -1;
    g->catch_at = -1;
}

uint8_t tennis_countdown(const TennisGame* g) {
    if (g->phase != TENNIS_PHASE_COUNTDOWN || g->phase_ms >= TENNIS_COUNTDOWN_MS) return 0;
    return (uint8_t)(3 - g->phase_ms * 3 / TENNIS_COUNTDOWN_MS);
}

bool tennis_jump(TennisGame* g) {
    if (g->paused || g->jumping) return false;
    if (g->phase != TENNIS_PHASE_READY && g->phase != TENNIS_PHASE_FLIGHT) return false;
    g->jumping = true;
    g->jump_ms = 0;
    g->jump_at = (int32_t)g->serve_ms;

    // 张嘴的时间段和球在嘴边的时间段有重叠就叼得住，叼住的那一刻是两段重叠的开头
    int32_t open_from = g->jump_at + TENNIS_OPEN_START_MS;
    int32_t open_to = g->jump_at + TENNIS_OPEN_END_MS;
    int32_t zone_from = (int32_t)arrive_at(g) - zone_ms(g);
    int32_t zone_to = (int32_t)arrive_at(g) + zone_ms(g);
    g->catch_at = open_from <= zone_to && open_to >= zone_from ? (open_from > zone_from ? open_from : zone_from) : -1;
    return true;
}

bool tennis_toggle_pause(TennisGame* g) {
    if (g->phase == TENNIS_PHASE_OVER) return false;
    g->paused = !g->paused;
    return true;
}

// 离当前阶段结束还有多久
static uint32_t until_phase_end(const TennisGame* g) {
    switch (g->phase) {
        case TENNIS_PHASE_COUNTDOWN: return TENNIS_COUNTDOWN_MS + TENNIS_GO_MS - g->phase_ms;
        case TENNIS_PHASE_READY:     return g->ready_ms - g->serve_ms;
        case TENNIS_PHASE_FLIGHT: {
            uint32_t end = g->catch_at >= 0 ? (uint32_t)g->catch_at : arrive_at(g) + (uint32_t)zone_ms(g);
            return end - g->serve_ms;
        }
        case TENNIS_PHASE_RESOLVE:   return TENNIS_RESOLVE_MS - g->phase_ms;
        default:                     return 0;
    }
}

void tennis_step(TennisGame* g, uint32_t dt_ms) {
    while (dt_ms > 0 && !g->paused && g->phase != TENNIS_PHASE_OVER) {
        uint32_t step = until_phase_end(g);
        if (g->jumping && TENNIS_JUMP_MS - g->jump_ms < step) step = TENNIS_JUMP_MS - g->jump_ms;
        if (step > dt_ms) step = dt_ms;
        dt_ms -= step;
        g->elapsed_ms += step;
        g->phase_ms += step;
        if (g->phase != TENNIS_PHASE_COUNTDOWN) g->serve_ms += step;
        if (g->jumping) {
            g->jump_ms += step;
            if (g->jump_ms >= TENNIS_JUMP_MS) g->jumping = false;
        }
        if (until_phase_end(g) > 0) continue;

        switch (g->phase) {
            case TENNIS_PHASE_COUNTDOWN:
                start_serve(g);
                break;
            case TENNIS_PHASE_READY:
                g->phase = TENNIS_PHASE_FLIGHT;
                g->phase_ms = 0;
                break;
            case TENNIS_PHASE_FLIGHT:
                resolve(g, g->catch_at >= 0);
                break;
            case TENNIS_PHASE_RESOLVE:
                if (g->misses >= TENNIS_MAX_MISSES) {
                    g->phase = TENNIS_PHASE_OVER;
                } else {
                    start_serve(g);
                }
                break;
            default:
                break;
        }
    }
}

int32_t tennis_ball_progress(const TennisGame* g) {
    if (g->phase != TENNIS_PHASE_FLIGHT && g->phase != TENNIS_PHASE_RESOLVE && g->phase != TENNIS_PHASE_OVER) {
        return -1;
    }
    if (g->serve_ms < g->ready_ms) return -1;
    return (int32_t)((g->serve_ms - g->ready_ms) * 1000 / g->arrive_ms);
}

uint16_t tennis_jump_lift(const TennisGame* g) {
    if (!g->jumping || g->jump_ms <= TENNIS_JUMP_WINDUP_MS || g->jump_ms >= TENNIS_JUMP_AIR_END_MS) return 0;
    // 抛物线：起跳和落地时为 0，最高点为 1000
    int32_t half = g->jump_ms < TENNIS_JUMP_PEAK_MS ? TENNIS_JUMP_PEAK_MS - TENNIS_JUMP_WINDUP_MS
                                                    : TENNIS_JUMP_AIR_END_MS - TENNIS_JUMP_PEAK_MS;
    int32_t d = (int32_t)g->jump_ms - TENNIS_JUMP_PEAK_MS;
    int32_t lift = 1000 - d * d * 1000 / (half * half);
    return (uint16_t)(lift < 0 ? 0 : lift);
}

bool tennis_mouth_open(const TennisGame* g) {
    return g->jumping && g->jump_ms >= TENNIS_OPEN_START_MS && g->jump_ms < TENNIS_OPEN_END_MS;
}

TennisReward tennis_reward(uint16_t score) {
    uint8_t stars = 0;
    while (stars < 3 && score >= STAR_SCORE[stars]) stars++;
    return (TennisReward){stars, SHELLS[stars], stars};
}

/**
 * Doe 拔河规则
 */

#include "tug.h"
#include <string.h>

// xorshift32
static uint32_t next_rand(TugGame* g) {
    uint32_t x = g->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g->rng = x;
    return x;
}

static const uint16_t SWEEP_MS[TUG_RAMP_ROUNDS] = TUG_SWEEP_MS_BY_ROUND;
static const uint8_t MATCH[TUG_RAMP_ROUNDS] = TUG_MATCH_BY_ROUND;
static const uint8_t PERFECT[TUG_RAMP_ROUNDS] = TUG_PERFECT_BY_ROUND;
static const uint8_t OWNER_MIN[TUG_RAMP_ROUNDS] = TUG_OWNER_MIN_BY_ROUND;

static uint8_t pick_owner_power(TugGame* g) {
    uint8_t prev = g->owner_power;
    uint8_t power;
    do {
        power = (uint8_t)(g->owner_min + next_rand(g) % (TUG_OWNER_MAX - g->owner_min + 1));
    } while (g->round > 1 && (power > prev ? power - prev : prev - power) < TUG_OWNER_CHANGE);
    return power;
}

static void start_round(TugGame* g) {
    g->round++;
    int tier = g->round < TUG_RAMP_ROUNDS ? g->round - 1 : TUG_RAMP_ROUNDS - 1;
    g->owner_min = OWNER_MIN[tier];
    g->owner_power = pick_owner_power(g);
    g->force = tug_force(g->owner_power);
    g->sweep_ms = SWEEP_MS[tier];
    g->match_range = MATCH[tier];
    g->perfect_range = PERFECT[tier];
    g->phase = TUG_PHASE_OWNER;
    g->phase_ms = 0;
}

static void resolve_round(TugGame* g, uint8_t power) {
    g->doe_power = power;
    g->pull = tug_judge(power, g->owner_power, g->match_range, g->perfect_range);
    if (g->pull == TUG_PULL_PERFECT || g->pull == TUG_PULL_GOOD) {
        g->lost_cells = 0;
        g->cell--;
        if (g->pull == TUG_PULL_PERFECT) g->perfects++;
    } else {
        g->misses++;
        uint8_t gain = g->force == TUG_FORCE_STRONG ? TUG_HEAVY_GAIN : 1;
        if (gain > TUG_CELLS - 1 - g->cell) gain = (uint8_t)(TUG_CELLS - 1 - g->cell);
        g->lost_cells = gain;
        g->cell += gain;
    }
    g->phase = TUG_PHASE_RESOLVE;
    g->phase_ms = 0;
}

static uint32_t phase_limit(const TugGame* g) {
    switch (g->phase) {
        case TUG_PHASE_COUNTDOWN: return TUG_COUNTDOWN_MS + TUG_GO_MS;
        case TUG_PHASE_OWNER:     return TUG_OWNER_ANIM_MS;
        default:                  return TUG_RESOLVE_MS;
    }
}

void tug_init(TugGame* g, uint32_t seed) {
    memset(g, 0, sizeof(*g));
    g->rng = seed ? seed : 0x9E3779B9u;
    g->cell = TUG_CELL_START;
    g->phase = TUG_PHASE_COUNTDOWN;
}

uint8_t tug_countdown(const TugGame* g) {
    if (g->phase != TUG_PHASE_COUNTDOWN || g->phase_ms >= TUG_COUNTDOWN_MS) return 0;
    return (uint8_t)(3 - g->phase_ms * 3 / TUG_COUNTDOWN_MS);
}

uint8_t tug_meter(const TugGame* g) {
    uint32_t sweep = g->sweep_ms ? g->sweep_ms : SWEEP_MS[0];
    uint32_t t = g->phase_ms % (2 * sweep);
    if (t > sweep) t = 2 * sweep - t;
    return (uint8_t)(t * TUG_POWER_MAX / sweep);
}

TugForce tug_force(uint8_t owner_power) {
    if (owner_power >= TUG_FORCE_HEAVY) return TUG_FORCE_STRONG;
    if (owner_power >= TUG_FORCE_MEDIUM) return TUG_FORCE_MID;
    return TUG_FORCE_LIGHT;
}

TugPull tug_judge(uint8_t doe_power, uint8_t owner_power, uint8_t match_range, uint8_t perfect_range) {
    int diff = (int)doe_power - (int)owner_power;
    if (diff < -(int)match_range) return TUG_PULL_WEAK;
    if (diff > (int)match_range) return TUG_PULL_SLIP;
    if (diff >= -(int)perfect_range && diff <= (int)perfect_range) return TUG_PULL_PERFECT;
    return TUG_PULL_GOOD;
}

bool tug_lock(TugGame* g) {
    if (g->phase != TUG_PHASE_SWING) return false;
    resolve_round(g, tug_meter(g));
    return true;
}

void tug_step(TugGame* g, uint32_t dt_ms) {
    while (dt_ms > 0 && g->phase != TUG_PHASE_OVER) {
        if (g->phase == TUG_PHASE_SWING) {
            // 力量条一直摆，等玩家按；只保留一个往返内的进度，长时间不按也不会溢出
            g->phase_ms = (g->phase_ms + dt_ms) % (2u * g->sweep_ms);
            g->elapsed_ms += dt_ms;
            return;
        }
        uint32_t limit = phase_limit(g);
        uint32_t step = limit - g->phase_ms;
        if (step > dt_ms) step = dt_ms;
        g->phase_ms += step;
        g->elapsed_ms += step;
        dt_ms -= step;
        if (g->phase_ms < limit) break;

        switch (g->phase) {
            case TUG_PHASE_COUNTDOWN:
                start_round(g);
                break;
            case TUG_PHASE_OWNER:
                g->phase = TUG_PHASE_SWING;
                g->phase_ms = 0;
                break;
            default:
                if (g->cell == 0) {
                    g->phase = TUG_PHASE_OVER;
                    g->result = TUG_RESULT_WIN;
                } else if (g->cell == TUG_CELLS - 1) {
                    g->phase = TUG_PHASE_OVER;
                    g->result = TUG_RESULT_LOSE;
                } else {
                    start_round(g);
                }
                break;
        }
    }
}

TugReward tug_reward(TugResult result, uint16_t misses) {
    static const uint16_t SHELLS[] = TUG_SHELLS_BY_STARS;
    if (result == TUG_RESULT_NONE) return (TugReward){0, 0, 0};
    uint8_t stars = 0;
    if (result == TUG_RESULT_WIN) stars = misses == 0 ? 3 : misses <= TUG_TWO_STAR_MISSES ? 2 : 1;
    return (TugReward){stars, SHELLS[stars], stars};
}

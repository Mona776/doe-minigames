/**
 * Sausage Snake 规则
 */

#include "sausage.h"
#include <string.h>

static const uint16_t STEP_MS_EARLY[SAUSAGE_EARLY_TIERS] = SAUSAGE_STEP_MS_EARLY;
static const uint8_t ROCK_SEGMENTS_EARLY[SAUSAGE_EARLY_TIERS] = SAUSAGE_ROCK_SEGMENTS_EARLY;
static const uint16_t STAR_BONES[3] = SAUSAGE_STAR_BONES;
static const uint16_t SHELLS[4] = SAUSAGE_SHELLS_BY_STARS;

// xorshift32
static uint32_t next_rand(SausageGame* g) {
    uint32_t x = g->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g->rng = x;
    return x;
}

static int distance(uint8_t a, uint8_t b) {
    int dx = a % SAUSAGE_SIZE - b % SAUSAGE_SIZE;
    int dy = a / SAUSAGE_SIZE - b / SAUSAGE_SIZE;
    return (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
}

// 在空格子里随机放骨头，优先离头够远的格子；一个空格子都没有返回 false
static bool spawn_bone(SausageGame* g) {
    uint8_t head = g->body[g->head];
    uint8_t far[SAUSAGE_CELLS], free_cells[SAUSAGE_CELLS];
    int far_count = 0, free_count = 0;
    for (int c = 0; c < SAUSAGE_CELLS; c++) {
        if (g->occupied[c] || g->blocked[c]) continue;
        free_cells[free_count++] = (uint8_t)c;
        if (distance((uint8_t)c, head) >= SAUSAGE_BONE_MIN_DIST) far[far_count++] = (uint8_t)c;
    }
    if (far_count) {
        g->bone = far[next_rand(g) % (uint32_t)far_count];
    } else if (free_count) {
        g->bone = free_cells[next_rand(g) % (uint32_t)free_count];
    }
    return free_count > 0;
}

static const int DX[4] = {0, 1, 0, -1};
static const int DY[4] = {-1, 0, 1, 0};

static bool open_cell(const bool* blocked, int x, int y) {
    return x >= 0 && x < SAUSAGE_SIZE && y >= 0 && y < SAUSAGE_SIZE && !blocked[y * SAUSAGE_SIZE + x];
}

// 场地公平：每个空格子至少有两个相邻的空格子（没有死胡同），所有空地连成一片；身体占的格子算空地
static bool field_fair(const bool* blocked) {
    int open_count = 0, first = -1;
    for (int c = 0; c < SAUSAGE_CELLS; c++) {
        if (blocked[c]) continue;
        open_count++;
        if (first < 0) first = c;
        int exits = 0;
        for (int d = 0; d < 4; d++) exits += open_cell(blocked, c % SAUSAGE_SIZE + DX[d], c / SAUSAGE_SIZE + DY[d]);
        if (exits < 2) return false;
    }
    if (first < 0) return false;

    bool seen[SAUSAGE_CELLS] = {false};
    uint8_t queue[SAUSAGE_CELLS];
    int head = 0, tail = 0;
    queue[tail++] = (uint8_t)first;
    seen[first] = true;
    while (head < tail) {
        uint8_t c = queue[head++];
        for (int d = 0; d < 4; d++) {
            int x = c % SAUSAGE_SIZE + DX[d], y = c / SAUSAGE_SIZE + DY[d];
            if (!open_cell(blocked, x, y) || seen[y * SAUSAGE_SIZE + x]) continue;
            seen[y * SAUSAGE_SIZE + x] = true;
            queue[tail++] = (uint8_t)(y * SAUSAGE_SIZE + x);
        }
    }
    return tail == open_count;
}

// 头正前方 SAUSAGE_ROCK_SAFE_AHEAD 格以内
static bool ahead_of_head(const SausageGame* g, int x, int y) {
    uint8_t head = g->body[g->head];
    int hx = head % SAUSAGE_SIZE, hy = head / SAUSAGE_SIZE;
    for (int k = 1; k <= SAUSAGE_ROCK_SAFE_AHEAD; k++) {
        if (hx + DX[g->dir] * k == x && hy + DY[g->dir] * k == y) return true;
    }
    return false;
}

// 一段石头能不能放在这些格子上：不压身体和骨头，离头够远、不挡在头正前方，和已有的石头不挨着（斜着挨也不行）
static bool rock_fits(const SausageGame* g, const uint8_t* cells, int len) {
    uint8_t head = g->body[g->head];
    for (int i = 0; i < len; i++) {
        uint8_t c = cells[i];
        int x = c % SAUSAGE_SIZE, y = c / SAUSAGE_SIZE;
        if (g->occupied[c] || c == g->bone) return false;
        if (distance(c, head) <= SAUSAGE_ROCK_SAFE_DIST || ahead_of_head(g, x, y)) return false;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int nx = x + dx, ny = y + dy;
                if (nx >= 0 && nx < SAUSAGE_SIZE && ny >= 0 && ny < SAUSAGE_SIZE &&
                    g->blocked[ny * SAUSAGE_SIZE + nx]) {
                    return false;
                }
            }
        }
    }
    return true;
}

// 随机摆一段石头，横竖随机；摆完场地不公平就换个位置，试够次数还不行返回 false
static bool place_rock(SausageGame* g) {
    const int len = SAUSAGE_ROCK_LEN;
    for (int attempt = 0; attempt < SAUSAGE_ROCK_TRIES; attempt++) {
        bool vertical = next_rand(g) % 2;
        int x = (int)(next_rand(g) % (uint32_t)(vertical ? SAUSAGE_SIZE : SAUSAGE_SIZE - len + 1));
        int y = (int)(next_rand(g) % (uint32_t)(vertical ? SAUSAGE_SIZE - len + 1 : SAUSAGE_SIZE));
        uint8_t cells[SAUSAGE_ROCK_LEN];
        for (int i = 0; i < len; i++) {
            cells[i] = (uint8_t)((y + (vertical ? i : 0)) * SAUSAGE_SIZE + x + (vertical ? 0 : i));
        }
        if (!rock_fits(g, cells, len)) continue;
        for (int i = 0; i < len; i++) g->blocked[cells[i]] = true;
        if (field_fair(g->blocked)) return true;
        for (int i = 0; i < len; i++) g->blocked[cells[i]] = false;
    }
    return false;
}

// 补到本档该有的石头段数，摆不下就先少摆，下次升档再补
static void add_rocks(SausageGame* g) {
    while (g->rock_segments < sausage_rock_segments(g->tier) && place_rock(g)) g->rock_segments++;
}

static void finish(SausageGame* g, SausageEnd end) {
    g->end = end;
    g->phase = SAUSAGE_PHASE_OVER;
}

static void move(SausageGame* g) {
    g->dir = sausage_next_dir(g);

    uint8_t head = g->body[g->head];
    int x = head % SAUSAGE_SIZE + DX[g->dir], y = head / SAUSAGE_SIZE + DY[g->dir];
    if (x < 0 || x >= SAUSAGE_SIZE || y < 0 || y >= SAUSAGE_SIZE) {
        finish(g, SAUSAGE_END_WALL);
        return;
    }

    uint8_t next = (uint8_t)(y * SAUSAGE_SIZE + x);
    if (g->blocked[next]) {
        finish(g, SAUSAGE_END_OBSTACLE);
        return;
    }
    bool eat = next == g->bone;
    bool grow = eat && (g->bones + 1) % SAUSAGE_GROW_EVERY == 0;
    uint8_t tail = sausage_segment(g, g->len - 1);
    // 不长身体时尾巴这一步会离开，头可以跟进去
    if (g->occupied[next] && (grow || next != tail)) {
        finish(g, SAUSAGE_END_SELF);
        return;
    }

    if (!grow) g->occupied[tail] = false;
    g->head = (uint8_t)((g->head + 1) % SAUSAGE_CELLS);
    g->body[g->head] = next;
    g->occupied[next] = true;
    g->moves++;
    if (!eat) return;

    if (grow) g->len++;
    g->bones++;
    uint8_t tier = sausage_tier(g->bones);
    if (tier > g->tier) {
        g->tier = tier;
        add_rocks(g);
    }
    if (!spawn_bone(g)) finish(g, SAUSAGE_END_FULL);
}

void sausage_init(SausageGame* g, uint32_t seed) {
    memset(g, 0, sizeof(*g));
    g->rng = seed ? seed : 0x9E3779B9u;
    g->tier = 1;
    g->dir = SAUSAGE_RIGHT;
    g->next_dir = SAUSAGE_RIGHT;
    g->len = SAUSAGE_START_LEN;
    g->head = SAUSAGE_START_LEN - 1;
    for (int i = 0; i < SAUSAGE_START_LEN; i++) {
        uint8_t c = (uint8_t)(SAUSAGE_START_Y * SAUSAGE_SIZE + SAUSAGE_START_X - (SAUSAGE_START_LEN - 1 - i));
        g->body[i] = c;
        g->occupied[c] = true;
    }
    add_rocks(g);
    spawn_bone(g);
    g->phase = SAUSAGE_PHASE_COUNTDOWN;
}

uint8_t sausage_countdown(const SausageGame* g) {
    if (g->phase != SAUSAGE_PHASE_COUNTDOWN || g->phase_ms >= SAUSAGE_COUNTDOWN_MS) return 0;
    return (uint8_t)(3 - g->phase_ms * 3 / SAUSAGE_COUNTDOWN_MS);
}

uint8_t sausage_segment(const SausageGame* g, uint8_t i) {
    return g->body[(g->head + SAUSAGE_CELLS - i) % SAUSAGE_CELLS];
}

bool sausage_turn(SausageGame* g) {
    // next_dir 不同于 dir 表示本节拍已经锁定右转；移动完成后两者重新一致。
    if (g->phase != SAUSAGE_PHASE_RUN || g->next_dir != g->dir) return false;
    g->next_dir = (SausageDir)((g->dir + 1) % 4);
    return true;
}

SausageDir sausage_next_dir(const SausageGame* g) {
    return g->next_dir;
}

bool sausage_toggle_pause(SausageGame* g) {
    if (g->phase == SAUSAGE_PHASE_RUN) {
        g->phase = SAUSAGE_PHASE_PAUSED;
        return true;
    }
    if (g->phase == SAUSAGE_PHASE_PAUSED) {
        g->phase = SAUSAGE_PHASE_RUN;
        g->phase_ms = 0;
        return true;
    }
    return false;
}

bool sausage_quit(SausageGame* g) {
    if (g->phase == SAUSAGE_PHASE_OVER) return false;
    finish(g, SAUSAGE_END_QUIT);
    return true;
}

static uint32_t phase_limit(const SausageGame* g) {
    if (g->phase == SAUSAGE_PHASE_COUNTDOWN) return SAUSAGE_COUNTDOWN_MS + SAUSAGE_GO_MS;
    return sausage_step_ms(g->tier);
}

void sausage_step(SausageGame* g, uint32_t dt_ms) {
    while (dt_ms > 0) {
        if (g->phase == SAUSAGE_PHASE_PAUSED || g->phase == SAUSAGE_PHASE_OVER) return;
        uint32_t limit = phase_limit(g);
        uint32_t step = limit - g->phase_ms;
        if (step > dt_ms) step = dt_ms;
        g->phase_ms += step;
        g->elapsed_ms += step;
        dt_ms -= step;
        if (g->phase_ms < limit) return;

        g->phase_ms = 0;
        if (g->phase == SAUSAGE_PHASE_COUNTDOWN) {
            g->phase = SAUSAGE_PHASE_RUN;
        } else {
            move(g);
        }
    }
}

uint8_t sausage_tier(uint16_t bones) {
    const uint16_t early = SAUSAGE_EARLY_TIERS * SAUSAGE_EARLY_TIER_BONES;
    uint32_t tier = bones < early ? bones / SAUSAGE_EARLY_TIER_BONES + 1
                                  : SAUSAGE_EARLY_TIERS + 1 + (bones - early) / SAUSAGE_LATE_TIER_BONES;
    return (uint8_t)(tier > SAUSAGE_MAX_TIER ? SAUSAGE_MAX_TIER : tier);
}

uint32_t sausage_step_ms(uint8_t tier) {
    if (tier < 1) tier = 1;
    if (tier <= SAUSAGE_EARLY_TIERS) return STEP_MS_EARLY[tier - 1];
    int32_t ms = STEP_MS_EARLY[SAUSAGE_EARLY_TIERS - 1] - SAUSAGE_STEP_MS_LATE_DROP * (tier - SAUSAGE_EARLY_TIERS);
    return (uint32_t)(ms < SAUSAGE_STEP_MS_MIN ? SAUSAGE_STEP_MS_MIN : ms);
}

uint8_t sausage_rock_segments(uint8_t tier) {
    if (tier < 1) return 0;
    if (tier <= SAUSAGE_EARLY_TIERS) return ROCK_SEGMENTS_EARLY[tier - 1];
    uint32_t n = ROCK_SEGMENTS_EARLY[SAUSAGE_EARLY_TIERS - 1] + (tier - SAUSAGE_EARLY_TIERS);
    return (uint8_t)(n > SAUSAGE_ROCK_SEGMENTS_MAX ? SAUSAGE_ROCK_SEGMENTS_MAX : n);
}

SausageReward sausage_reward(uint16_t bones) {
    uint8_t stars = 0;
    while (stars < 3 && bones >= STAR_BONES[stars]) stars++;
    return (SausageReward){stars, SHELLS[stars], stars};
}

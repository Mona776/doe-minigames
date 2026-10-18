/**
 * 贪吃腊肠规则
 */

#include "snake.h"
#include <string.h>

static const uint16_t STEP_MS[SNAKE_LEVELS] = SNAKE_STEP_MS_BY_LEVEL;
static const uint8_t ROCK_SEGMENTS[SNAKE_LEVELS] = SNAKE_ROCK_SEGMENTS_BY_LEVEL;
static const uint8_t ROCK_LEN[SNAKE_LEVELS] = SNAKE_ROCK_LEN_BY_LEVEL;
static const uint16_t STAR_BONES[3] = SNAKE_STAR_BONES;
static const uint16_t SHELLS[4] = SNAKE_SHELLS_BY_STARS;

// xorshift32
static uint32_t next_rand(SnakeGame* g) {
    uint32_t x = g->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g->rng = x;
    return x;
}

static int distance(uint8_t a, uint8_t b) {
    int dx = a % SNAKE_SIZE - b % SNAKE_SIZE;
    int dy = a / SNAKE_SIZE - b / SNAKE_SIZE;
    return (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
}

// 在空格子里随机放骨头，优先离头够远的格子
static void spawn_bone(SnakeGame* g) {
    uint8_t head = g->body[g->head];
    uint8_t far[SNAKE_CELLS], free_cells[SNAKE_CELLS];
    int far_count = 0, free_count = 0;
    for (int c = 0; c < SNAKE_CELLS; c++) {
        if (g->occupied[c] || g->blocked[c]) continue;
        free_cells[free_count++] = (uint8_t)c;
        if (distance((uint8_t)c, head) >= SNAKE_BONE_MIN_DIST) far[far_count++] = (uint8_t)c;
    }
    if (far_count) {
        g->bone = far[next_rand(g) % (uint32_t)far_count];
    } else if (free_count) {
        g->bone = free_cells[next_rand(g) % (uint32_t)free_count];
    }
}

static const int DX[4] = {0, 1, 0, -1};
static const int DY[4] = {-1, 0, 1, 0};

static bool open_cell(const bool* blocked, int x, int y) {
    return x >= 0 && x < SNAKE_SIZE && y >= 0 && y < SNAKE_SIZE && !blocked[y * SNAKE_SIZE + x];
}

// 场地公平：每个空格子至少有两个相邻的空格子（没有死胡同），所有空地连成一片
static bool field_fair(const bool* blocked) {
    int open_count = 0, first = -1;
    for (int c = 0; c < SNAKE_CELLS; c++) {
        if (blocked[c]) continue;
        open_count++;
        if (first < 0) first = c;
        int exits = 0;
        for (int d = 0; d < 4; d++) exits += open_cell(blocked, c % SNAKE_SIZE + DX[d], c / SNAKE_SIZE + DY[d]);
        if (exits < 2) return false;
    }
    if (first < 0) return false;

    bool seen[SNAKE_CELLS] = {false};
    uint8_t queue[SNAKE_CELLS];
    int head = 0, tail = 0;
    queue[tail++] = (uint8_t)first;
    seen[first] = true;
    while (head < tail) {
        uint8_t c = queue[head++];
        for (int d = 0; d < 4; d++) {
            int x = c % SNAKE_SIZE + DX[d], y = c / SNAKE_SIZE + DY[d];
            if (!open_cell(blocked, x, y) || seen[y * SNAKE_SIZE + x]) continue;
            seen[y * SNAKE_SIZE + x] = true;
            queue[tail++] = (uint8_t)(y * SNAKE_SIZE + x);
        }
    }
    return tail == open_count;
}

// 一段石头能不能放在这些格子上：不在出发那一行，和已有的石头不挨着（斜着挨也不行）
static bool rock_fits(const bool* blocked, const uint8_t* cells, int len) {
    for (int i = 0; i < len; i++) {
        int x = cells[i] % SNAKE_SIZE, y = cells[i] / SNAKE_SIZE;
        if (y == SNAKE_START_Y) return false;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int nx = x + dx, ny = y + dy;
                if (nx >= 0 && nx < SNAKE_SIZE && ny >= 0 && ny < SNAKE_SIZE && blocked[ny * SNAKE_SIZE + nx]) {
                    return false;
                }
            }
        }
    }
    return true;
}

// 按本关配置随机摆石头，每段横竖随机；摆完场地不公平就换个位置，试够次数还不行就少摆这一段
static void place_rocks(SnakeGame* g) {
    int len = snake_rock_len(g->level);
    if (len <= 0 || len > SNAKE_SIZE) return;
    for (int seg = 0; seg < snake_rock_segments(g->level); seg++) {
        for (int attempt = 0; attempt < SNAKE_ROCK_TRIES; attempt++) {
            bool vertical = next_rand(g) % 2;
            int x = (int)(next_rand(g) % (uint32_t)(vertical ? SNAKE_SIZE : SNAKE_SIZE - len + 1));
            int y = (int)(next_rand(g) % (uint32_t)(vertical ? SNAKE_SIZE - len + 1 : SNAKE_SIZE));
            uint8_t cells[SNAKE_SIZE];
            for (int i = 0; i < len; i++) {
                cells[i] = (uint8_t)((y + (vertical ? i : 0)) * SNAKE_SIZE + x + (vertical ? 0 : i));
            }
            if (!rock_fits(g->blocked, cells, len)) continue;
            for (int i = 0; i < len; i++) g->blocked[cells[i]] = true;
            if (field_fair(g->blocked)) break;
            for (int i = 0; i < len; i++) g->blocked[cells[i]] = false;
        }
    }
}

// 摆好本关的场地：身体回到起点、随机摆石头、放第一根骨头，从倒计时开始
static void setup_level(SnakeGame* g) {
    memset(g->occupied, 0, sizeof(g->occupied));
    memset(g->blocked, 0, sizeof(g->blocked));
    place_rocks(g);
    g->dir = SNAKE_RIGHT;
    g->turns = 0;
    g->len = SNAKE_START_LEN;
    g->head = SNAKE_START_LEN - 1;
    for (int i = 0; i < SNAKE_START_LEN; i++) {
        uint8_t c = (uint8_t)(SNAKE_START_Y * SNAKE_SIZE + SNAKE_START_X - (SNAKE_START_LEN - 1 - i));
        g->body[i] = c;
        g->occupied[c] = true;
    }
    g->level_bones = 0;
    g->phase = SNAKE_PHASE_COUNTDOWN;
    g->phase_ms = 0;
    g->moves++;
    spawn_bone(g);
}

static void finish(SnakeGame* g, SnakeEnd end) {
    g->end = end;
    g->phase = SNAKE_PHASE_OVER;
}

static void move(SnakeGame* g) {
    g->dir = snake_next_dir(g);
    g->turns = 0;

    uint8_t head = g->body[g->head];
    int x = head % SNAKE_SIZE, y = head / SNAKE_SIZE;
    switch (g->dir) {
        case SNAKE_UP:    y--; break;
        case SNAKE_RIGHT: x++; break;
        case SNAKE_DOWN:  y++; break;
        case SNAKE_LEFT:  x--; break;
    }
    if (x < 0 || x >= SNAKE_SIZE || y < 0 || y >= SNAKE_SIZE) {
        finish(g, SNAKE_END_WALL);
        return;
    }

    uint8_t next = (uint8_t)(y * SNAKE_SIZE + x);
    if (g->blocked[next]) {
        finish(g, SNAKE_END_OBSTACLE);
        return;
    }
    bool eat = next == g->bone;
    uint8_t tail = snake_segment(g, g->len - 1);
    // 没吃到时尾巴这一步会离开，头可以跟进去
    if (g->occupied[next] && (eat || next != tail)) {
        finish(g, SNAKE_END_SELF);
        return;
    }

    if (!eat) g->occupied[tail] = false;
    g->head = (uint8_t)((g->head + 1) % SNAKE_CELLS);
    g->body[g->head] = next;
    g->occupied[next] = true;
    g->moves++;
    if (!eat) return;

    g->len++;
    g->bones++;
    g->level_bones++;
    if (g->level_bones < SNAKE_LEVEL_BONES) {
        spawn_bone(g);
    } else if (g->level < SNAKE_LEVELS) {
        g->phase = SNAKE_PHASE_LEVEL_UP;
    } else {
        finish(g, SNAKE_END_CLEAR);
    }
}

void snake_init(SnakeGame* g, uint32_t seed) {
    memset(g, 0, sizeof(*g));
    g->rng = seed ? seed : 0x9E3779B9u;
    g->level = 1;
    setup_level(g);
    g->moves = 0;
}

uint8_t snake_countdown(const SnakeGame* g) {
    if (g->phase != SNAKE_PHASE_COUNTDOWN || g->phase_ms >= SNAKE_COUNTDOWN_MS) return 0;
    return (uint8_t)(3 - g->phase_ms * 3 / SNAKE_COUNTDOWN_MS);
}

uint8_t snake_segment(const SnakeGame* g, uint8_t i) {
    return g->body[(g->head + SNAKE_CELLS - i) % SNAKE_CELLS];
}

bool snake_turn(SnakeGame* g) {
    if (g->phase != SNAKE_PHASE_RUN || g->turns >= SNAKE_MAX_TURNS) return false;
    g->turns++;
    return true;
}

SnakeDir snake_next_dir(const SnakeGame* g) {
    // 两次转向是掉头，会直接撞上脖子，忽略
    if (g->turns % 4 == 2) return g->dir;
    return (SnakeDir)((g->dir + g->turns) % 4);
}

bool snake_toggle_pause(SnakeGame* g) {
    if (g->phase == SNAKE_PHASE_RUN) {
        g->phase = SNAKE_PHASE_PAUSED;
        return true;
    }
    if (g->phase == SNAKE_PHASE_PAUSED) {
        g->phase = SNAKE_PHASE_RUN;
        g->phase_ms = 0;
        return true;
    }
    return false;
}

static uint32_t phase_limit(const SnakeGame* g) {
    switch (g->phase) {
        case SNAKE_PHASE_COUNTDOWN: return SNAKE_COUNTDOWN_MS + SNAKE_GO_MS;
        case SNAKE_PHASE_LEVEL_UP:  return SNAKE_LEVEL_UP_MS;
        default:                    return snake_step_ms(g->level);
    }
}

void snake_step(SnakeGame* g, uint32_t dt_ms) {
    while (dt_ms > 0) {
        if (g->phase == SNAKE_PHASE_PAUSED || g->phase == SNAKE_PHASE_OVER) return;
        uint32_t limit = phase_limit(g);
        uint32_t step = limit - g->phase_ms;
        if (step > dt_ms) step = dt_ms;
        g->phase_ms += step;
        g->elapsed_ms += step;
        dt_ms -= step;
        if (g->phase_ms < limit) return;

        g->phase_ms = 0;
        switch (g->phase) {
            case SNAKE_PHASE_COUNTDOWN:
                g->phase = SNAKE_PHASE_RUN;
                break;
            case SNAKE_PHASE_LEVEL_UP:
                g->level++;
                setup_level(g);
                break;
            default:
                move(g);
                break;
        }
    }
}

uint32_t snake_step_ms(uint8_t level) {
    if (level < 1) level = 1;
    if (level > SNAKE_LEVELS) level = SNAKE_LEVELS;
    return STEP_MS[level - 1];
}

uint8_t snake_rock_segments(uint8_t level) {
    return level >= 1 && level <= SNAKE_LEVELS ? ROCK_SEGMENTS[level - 1] : 0;
}

uint8_t snake_rock_len(uint8_t level) {
    return level >= 1 && level <= SNAKE_LEVELS ? ROCK_LEN[level - 1] : 0;
}

SnakeReward snake_reward(uint16_t bones) {
    uint8_t stars = 0;
    while (stars < 3 && bones >= STAR_BONES[stars]) stars++;
    return (SnakeReward){stars, SHELLS[stars], stars};
}

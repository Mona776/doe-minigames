/**
 * Sausage Snake 规则单元测试（在 Mac 上运行：./run_tests.sh）
 * 除了逐条规则，还让一个简单的机器人打 500 局：每次升档都检查新石头摆得公平，并统计得分、档位和单局时长，方便调数值
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/games/sausage.h"

#define GAMES 500

static int s_failures;

#define CHECK(cond, msg)                                              \
    do {                                                              \
        if (!(cond)) {                                                \
            printf("  ✗ %s（第 %d 行）\n", msg, __LINE__);            \
            s_failures++;                                             \
        }                                                             \
    } while (0)

static uint8_t cell(int x, int y) {
    return (uint8_t)(y * SAUSAGE_SIZE + x);
}

static int cell_x(uint8_t c) { return c % SAUSAGE_SIZE; }
static int cell_y(uint8_t c) { return c / SAUSAGE_SIZE; }

static int distance(uint8_t a, uint8_t b) {
    return abs(cell_x(a) - cell_x(b)) + abs(cell_y(a) - cell_y(b));
}

static uint8_t head_of(const SausageGame* g) {
    return sausage_segment(g, 0);
}

// 跳过倒计时，停在刚开始走动的时刻
static void to_run(SausageGame* g) {
    if (g->phase == SAUSAGE_PHASE_COUNTDOWN) sausage_step(g, SAUSAGE_COUNTDOWN_MS + SAUSAGE_GO_MS - g->phase_ms);
}

// 走满一个节拍，正好移动一次
static void tick(SausageGame* g) {
    sausage_step(g, sausage_step_ms(g->tier) - g->phase_ms);
}

static void press(SausageGame* g, int n) {
    for (int i = 0; i < n; i++) sausage_turn(g);
}

static void park_bone(SausageGame* g, int x, int y) {
    g->bone = cell(x, y);
}

// 按给定的格子重新摆身体，cells 从头到尾，每项是 {x, y}
static void set_body(SausageGame* g, const int (*cells)[2], int n, SausageDir dir) {
    memset(g->occupied, 0, sizeof(g->occupied));
    g->len = (uint8_t)n;
    g->head = (uint8_t)(n - 1);
    for (int i = 0; i < n; i++) {
        uint8_t c = cell(cells[i][0], cells[i][1]);
        g->body[n - 1 - i] = c;
        g->occupied[c] = true;
    }
    g->dir = dir;
    g->next_dir = dir;
}

static int count_blocked(const SausageGame* g) {
    int n = 0;
    for (int c = 0; c < SAUSAGE_CELLS; c++) n += g->blocked[c];
    return n;
}

// 从 c 往 dir 走一格，出界返回 -1
static int step_cell(uint8_t c, SausageDir dir) {
    int x = cell_x(c), y = cell_y(c);
    switch (dir) {
        case SAUSAGE_UP:    y--; break;
        case SAUSAGE_RIGHT: x++; break;
        case SAUSAGE_DOWN:  y++; break;
        case SAUSAGE_LEFT:  x--; break;
    }
    if (x < 0 || x >= SAUSAGE_SIZE || y < 0 || y >= SAUSAGE_SIZE) return -1;
    return cell(x, y);
}

// 空地（身体算空地）连成一片、没有死胡同
static bool field_ok(const SausageGame* g) {
    bool seen[SAUSAGE_CELLS];
    int open = 0, first = -1;
    for (int c = 0; c < SAUSAGE_CELLS; c++) {
        seen[c] = g->blocked[c];
        if (g->blocked[c]) continue;
        open++;
        if (first < 0) first = c;
        int exits = 0;
        for (int d = 0; d < 4; d++) {
            int n = step_cell((uint8_t)c, (SausageDir)d);
            exits += n >= 0 && !g->blocked[n];
        }
        if (exits < 2) return false;
    }
    uint8_t queue[SAUSAGE_CELLS];
    int head = 0, tail = 0;
    queue[tail++] = (uint8_t)first;
    seen[first] = true;
    while (head < tail) {
        uint8_t c = queue[head++];
        for (int d = 0; d < 4; d++) {
            int n = step_cell(c, (SausageDir)d);
            if (n < 0 || seen[n]) continue;
            seen[n] = true;
            queue[tail++] = (uint8_t)n;
        }
    }
    return tail == open;
}

// 每块石头都属于一段横的或竖的、长度正好的石头，段与段之间不挨着
static bool segments_ok(const SausageGame* g) {
    const int len = SAUSAGE_ROCK_LEN;
    bool used[SAUSAGE_CELLS] = {false};
    for (int c = 0; c < SAUSAGE_CELLS; c++) {
        if (!g->blocked[c] || used[c]) continue;
        int run_x = 0, run_y = 0;
        while (cell_x((uint8_t)c) + run_x < SAUSAGE_SIZE && g->blocked[c + run_x]) run_x++;
        while (cell_y((uint8_t)c) + run_y < SAUSAGE_SIZE && g->blocked[c + run_y * SAUSAGE_SIZE]) run_y++;
        bool horizontal = run_x == len && run_y == 1;
        bool vertical = run_y == len && run_x == 1;
        if (!horizontal && !vertical) return false;
        for (int i = 0; i < len; i++) used[c + (vertical ? i * SAUSAGE_SIZE : i)] = true;
    }
    for (int c = 0; c < SAUSAGE_CELLS; c++) {
        if (g->blocked[c] != used[c]) return false;
    }
    for (int c = 0; c < SAUSAGE_CELLS; c++) {
        if (!g->blocked[c]) continue;
        for (int dy = -1; dy <= 1; dy += 2) {
            for (int dx = -1; dx <= 1; dx += 2) {
                int x = cell_x((uint8_t)c) + dx, y = cell_y((uint8_t)c) + dy;
                if (x >= 0 && x < SAUSAGE_SIZE && y >= 0 && y < SAUSAGE_SIZE && g->blocked[cell(x, y)]) return false;
            }
        }
    }
    return true;
}

// ---- 机器人 ----

// 头走到 from 之后还能到达多少空格子（尾巴那格算空）
static int reachable(const SausageGame* g, uint8_t from) {
    bool seen[SAUSAGE_CELLS];
    for (int c = 0; c < SAUSAGE_CELLS; c++) seen[c] = g->occupied[c] || g->blocked[c];
    seen[sausage_segment(g, g->len - 1)] = false;
    uint8_t queue[SAUSAGE_CELLS];
    int head = 0, tail = 0, count = 0;
    queue[tail++] = from;
    seen[from] = true;
    while (head < tail) {
        uint8_t c = queue[head++];
        count++;
        for (int d = 0; d < 4; d++) {
            int n = step_cell(c, (SausageDir)d);
            if (n >= 0 && !seen[n]) {
                seen[n] = true;
                queue[tail++] = (uint8_t)n;
            }
        }
    }
    return count;
}

// 头走到 from 之后，绕开石头和身体走到骨头要几步；走不到返回一个大数
static int path_to_bone(const SausageGame* g, uint8_t from) {
    if (from == g->bone) return 0;
    bool seen[SAUSAGE_CELLS];
    for (int c = 0; c < SAUSAGE_CELLS; c++) seen[c] = g->occupied[c] || g->blocked[c];
    seen[sausage_segment(g, g->len - 1)] = false;
    uint8_t queue[SAUSAGE_CELLS];
    int dist[SAUSAGE_CELLS];
    int head = 0, tail = 0;
    queue[tail++] = from;
    seen[from] = true;
    dist[from] = 0;
    while (head < tail) {
        uint8_t c = queue[head++];
        for (int d = 0; d < 4; d++) {
            int n = step_cell(c, (SausageDir)d);
            if (n < 0 || seen[n]) continue;
            if (n == g->bone) return dist[c] + 1;
            seen[n] = true;
            dist[n] = dist[c] + 1;
            queue[tail++] = (uint8_t)n;
        }
    }
    return SAUSAGE_CELLS;
}

// 每拍只在直行和右转中挑选：只能使用实际玩家可执行的操作。
static int choose_presses(const SausageGame* g) {
    static const int PRESSES[] = {0, 1};
    uint8_t head = head_of(g);
    uint8_t tail = sausage_segment(g, g->len - 1);
    int best = 0, best_score = -100000;
    for (int i = 0; i < 2; i++) {
        SausageDir dir = (SausageDir)((g->dir + PRESSES[i]) % 4);
        int n = step_cell(head, dir);
        if (n < 0 || g->blocked[n] || (g->occupied[n] && !(n == tail && n != g->bone))) continue;
        int room = reachable(g, (uint8_t)n);
        int score = -path_to_bone(g, (uint8_t)n) * 4;
        if (room < g->len) score -= 1000;
        score += room / 10;
        if (score > best_score) {
            best_score = score;
            best = PRESSES[i];
        }
    }
    return best;
}

// 机器人走一拍
static void bot_tick(SausageGame* g) {
    to_run(g);
    press(g, choose_presses(g));
    tick(g);
}

// 吃到骨头前一直让机器人走；返回是否吃到（撞了或绕太久返回 false）
static bool bot_eat(SausageGame* g) {
    uint16_t bones = g->bones;
    for (int i = 0; i < 400 && g->phase != SAUSAGE_PHASE_OVER; i++) {
        bot_tick(g);
        if (g->bones > bones) return true;
    }
    return false;
}

// ---- 逐条规则 ----

static void test_start(void) {
    printf("开局\n");
    SausageGame g;
    sausage_init(&g, 1);
    CHECK(g.len == 3 && head_of(&g) == cell(5, 4) && sausage_segment(&g, 1) == cell(4, 4) &&
          sausage_segment(&g, 2) == cell(3, 4), "身体 3 格，头在中间、往左排");
    CHECK(g.dir == SAUSAGE_RIGHT && g.tier == 1 && g.bones == 0, "第 1 档，初始朝右");
    CHECK(count_blocked(&g) == 0 && g.rock_segments == 0, "第 1 档没有石头");

    CHECK(sausage_countdown(&g) == 3, "先显示 3");
    CHECK(!sausage_turn(&g) && !sausage_toggle_pause(&g), "倒计时中转向、暂停都不算");
    sausage_step(&g, 3000);
    CHECK(sausage_countdown(&g) == 0 && g.phase == SAUSAGE_PHASE_COUNTDOWN, "3 秒后显示 GO");
    sausage_step(&g, SAUSAGE_GO_MS);
    CHECK(g.phase == SAUSAGE_PHASE_RUN && g.moves == 0, "GO 之后开始走动，还没移动");
    park_bone(&g, 0, 9);
    sausage_step(&g, sausage_step_ms(1) - 1);
    CHECK(g.moves == 0, "一个节拍没满不动");
    sausage_step(&g, 1);
    CHECK(g.moves == 1 && head_of(&g) == cell(6, 4), "节拍到了往右走一格");

    bool far = true;
    for (uint32_t seed = 1; seed <= 1000; seed++) {
        sausage_init(&g, seed);
        if (g.occupied[g.bone] || distance(g.bone, head_of(&g)) < SAUSAGE_BONE_MIN_DIST) far = false;
    }
    CHECK(far, "开局骨头在空格子上，离头至少 3 格");
}

static void test_turns_and_crash(void) {
    printf("每拍一次右转与正常碰撞\n");
    SausageGame g;
    sausage_init(&g, 2);
    to_run(&g);
    park_bone(&g, 0, 9);
    sausage_step(&g, sausage_step_ms(1) - 1);
    uint32_t elapsed = g.elapsed_ms, phase_ms = g.phase_ms;
    CHECK(sausage_turn(&g), "本节拍首次按下锁定右转");
    bool ignored = true;
    for (int i = 0; i < 260; i++) if (sausage_turn(&g)) ignored = false;
    CHECK(ignored && sausage_next_dir(&g) == SAUSAGE_DOWN, "同节拍高速连按不累计、不取消右转");
    CHECK(g.dir == SAUSAGE_RIGHT && g.moves == 0 && head_of(&g) == cell(5, 4) &&
          g.elapsed_ms == elapsed && g.phase_ms == phase_ms, "锁定方向不提前转身、不移动、不重置计时");
    sausage_step(&g, 1);
    CHECK(g.dir == SAUSAGE_DOWN && g.next_dir == SAUSAGE_DOWN && head_of(&g) == cell(5, 5),
          "原定节拍到达时右转并前进");
    CHECK(sausage_turn(&g) && !sausage_turn(&g), "移动完成后重新接受一次右转");
    tick(&g);
    CHECK(g.dir == SAUSAGE_LEFT && head_of(&g) == cell(4, 5), "连续两拍右转须经过两个格子");
    tick(&g);
    CHECK(g.dir == SAUSAGE_LEFT && head_of(&g) == cell(3, 5), "未按键的下一拍直行，没有多余输入排队");

    sausage_init(&g, 2);
    to_run(&g);
    park_bone(&g, 0, 9);
    press(&g, 2);
    CHECK(g.next_dir == SAUSAGE_DOWN && g.phase == SAUSAGE_PHASE_RUN && g.moves == 0,
          "同一拍按两次仍只锁定一次右转");
    tick(&g);
    CHECK(g.end == SAUSAGE_END_NONE && g.dir == SAUSAGE_DOWN, "连按不能原地掉头撞进脖子");

    sausage_init(&g, 2);
    to_run(&g);
    park_bone(&g, 0, 9);
    press(&g, 1);
    tick(&g);
    CHECK(g.dir == SAUSAGE_DOWN && head_of(&g) == cell(5, 5), "第一次按下后的移动向下");
    press(&g, 1);
    tick(&g);
    CHECK(g.dir == SAUSAGE_LEFT && head_of(&g) == cell(4, 5), "跨移动时间的按下按实际顺序执行，不合并回滚");

    sausage_init(&g, 3);
    to_run(&g);
    park_bone(&g, 0, 9);
    for (int i = 0; i < SAUSAGE_SIZE - 1 - SAUSAGE_START_X; i++) tick(&g);
    CHECK(g.end == SAUSAGE_END_NONE, "走到墙边还活着");
    tick(&g);
    CHECK(g.end == SAUSAGE_END_WALL && g.phase == SAUSAGE_PHASE_OVER, "撞墙立即结束");
    CHECK(head_of(&g) == cell(SAUSAGE_SIZE - 1, SAUSAGE_START_Y), "头停在墙边");

    sausage_init(&g, 4);
    to_run(&g);
    park_bone(&g, 0, 9);
    g.blocked[cell(SAUSAGE_START_X + 1, SAUSAGE_START_Y)] = true;
    tick(&g);
    CHECK(g.end == SAUSAGE_END_OBSTACLE, "撞石头立即结束");

    sausage_init(&g, 5);
    to_run(&g);
    const int line[][2] = {{5, 4}, {4, 4}, {3, 4}, {2, 4}, {1, 4}};
    set_body(&g, line, 5, SAUSAGE_RIGHT);
    park_bone(&g, 9, 9);
    for (int i = 0; i < 3; i++) {
        press(&g, 1);
        tick(&g);
    }
    CHECK(g.end == SAUSAGE_END_SELF, "绕回来撞上身体立即结束");

    sausage_init(&g, 6);
    to_run(&g);
    const int ring[][2] = {{5, 5}, {5, 4}, {4, 4}, {4, 5}};
    set_body(&g, ring, 4, SAUSAGE_DOWN);
    park_bone(&g, 9, 9);
    bool alive = true;
    for (int i = 0; i < 8; i++) {
        press(&g, 1);
        tick(&g);
        if (g.end != SAUSAGE_END_NONE) alive = false;
    }
    CHECK(alive, "追着尾巴走不算撞到自己");
}

static void test_grow(void) {
    printf("每吃 %d 根长一节\n", SAUSAGE_GROW_EVERY);
    SausageGame g;
    sausage_init(&g, 11);
    to_run(&g);
    park_bone(&g, SAUSAGE_START_X + 1, SAUSAGE_START_Y);
    tick(&g);
    CHECK(g.bones == 1 && g.len == SAUSAGE_START_LEN, "第 1 根不长");
    CHECK(sausage_segment(&g, g.len - 1) == cell(SAUSAGE_START_X - 1, SAUSAGE_START_Y) &&
          !g.occupied[cell(SAUSAGE_START_X - 2, SAUSAGE_START_Y)], "不长的这一步尾巴照常跟上");
    park_bone(&g, SAUSAGE_START_X + 2, SAUSAGE_START_Y);
    tick(&g);
    CHECK(g.bones == 2 && g.len == SAUSAGE_START_LEN + 1, "第 2 根长一节");
    CHECK(sausage_segment(&g, g.len - 1) == cell(SAUSAGE_START_X - 1, SAUSAGE_START_Y), "长的这一步尾巴不动");

    bool ok = true;
    for (uint32_t seed = 1; seed <= 100 && ok; seed++) {
        sausage_init(&g, seed);
        for (int i = 0; i < 12 && bot_eat(&g); i++) {
            if (g.len != SAUSAGE_START_LEN + g.bones / SAUSAGE_GROW_EVERY) ok = false;
        }
    }
    CHECK(ok, "身体长度始终是 3 + 骨头数 / 2");

    // 吃到但不长的一步：尾巴这一步离开，头可以追着尾巴走
    sausage_init(&g, 12);
    to_run(&g);
    const int ring[][2] = {{5, 5}, {5, 4}, {4, 4}, {4, 5}};
    set_body(&g, ring, 4, SAUSAGE_DOWN);
    park_bone(&g, 4, 5);  // 正好在尾巴上
    press(&g, 1);
    tick(&g);
    CHECK(g.end == SAUSAGE_END_NONE && g.bones == 1 && head_of(&g) == cell(4, 5),
          "在尾巴格子上吃到但不长，尾巴照常离开，不算撞到自己");
}

static void test_tiers(void) {
    printf("难度档\n");
    static const struct { uint16_t bones; uint8_t tier; } TIERS[] = {
        {0, 1}, {4, 1}, {5, 2}, {9, 2}, {10, 3}, {14, 3}, {15, 4}, {24, 4}, {25, 5}, {34, 5}, {35, 6}, {54, 7}, {55, 8}, {65, 8},
    };
    bool tier_ok = true;
    for (size_t i = 0; i < sizeof(TIERS) / sizeof(TIERS[0]); i++) {
        if (sausage_tier(TIERS[i].bones) != TIERS[i].tier) {
            printf("    %u 根应在第 %u 档，实际 %u\n", TIERS[i].bones, TIERS[i].tier, sausage_tier(TIERS[i].bones));
            tier_ok = false;
        }
    }
    CHECK(tier_ok, "前 15 根每 5 根一档，之后每 10 根一档");
    CHECK(sausage_tier(60000) == SAUSAGE_MAX_TIER, "第 8 档之后不再升档");
    CHECK(sausage_step_ms(SAUSAGE_MAX_TIER) == SAUSAGE_STEP_MS_MIN &&
              sausage_rock_segments(SAUSAGE_MAX_TIER) == SAUSAGE_ROCK_SEGMENTS_MAX,
          "最后一档速度最快、石头最多");

    static const uint16_t STEP[] = {550, 470, 400, 370, 340, 310, 280, 250, 250};
    static const uint8_t ROCKS[] = {0, 2, 3, 4, 5, 6, 6, 6, 6};
    bool step_ok = true, rocks_ok = true;
    for (uint8_t t = 1; t <= 9; t++) {
        if (sausage_step_ms(t) != STEP[t - 1]) step_ok = false;
        if (sausage_rock_segments(t) != ROCKS[t - 1]) rocks_ok = false;
    }
    CHECK(step_ok && sausage_step_ms(255) == SAUSAGE_STEP_MS_MIN, "速度 550/470/400，之后每档快 30ms，最快 250ms");
    CHECK(rocks_ok && sausage_rock_segments(255) == SAUSAGE_ROCK_SEGMENTS_MAX, "石头 0/2/3 段，之后每档加 1 段，最多 6 段");

    // 吃到第 5 根：不停顿、身体不重置，直接加石头、加速
    SausageGame g;
    sausage_init(&g, 7);
    bool ok = true;
    for (int i = 0; i < 4 && ok; i++) ok = bot_eat(&g);
    CHECK(ok && g.tier == 1 && count_blocked(&g) == 0, "前 4 根第 1 档，没石头");
    ok = bot_eat(&g);
    CHECK(ok && g.bones == 5 && g.tier == 2 && g.phase == SAUSAGE_PHASE_RUN, "第 5 根升到第 2 档，接着走，不停顿");
    CHECK(g.len == SAUSAGE_START_LEN + 5 / SAUSAGE_GROW_EVERY, "身体不重置，照常变长");
    CHECK(g.rock_segments == 2 && count_blocked(&g) == 2 * SAUSAGE_ROCK_LEN, "补上 2 段石头");
    uint16_t moves = g.moves;
    sausage_step(&g, sausage_step_ms(2) - 1 - g.phase_ms);
    CHECK(g.moves == moves, "第 2 档差 1ms 还没动");
    sausage_step(&g, 1);
    CHECK(g.moves == moves + 1 || g.phase == SAUSAGE_PHASE_OVER, "按第 2 档的速度走");
}

// 机器人打很多局，每次石头变化都检查：只增不减、不压身体和骨头、离头够远、不挡在正前方、整体公平
static void test_rocks(void) {
    printf("升档追加石头\n");
    bool grow_ok = true, body_ok = true, safe_ok = true, fair_ok = true, shape_ok = true, count_ok = true;
    bool only_on_tier_up = true, bone_ok = true;
    int tier_ups = 0, short_segments = 0;
    for (uint32_t seed = 1; seed <= 500; seed++) {
        SausageGame g;
        sausage_init(&g, seed);
        while (g.phase != SAUSAGE_PHASE_OVER && g.moves < 3000) {
            bool before[SAUSAGE_CELLS];
            memcpy(before, g.blocked, sizeof(before));
            uint8_t tier = g.tier;
            bot_tick(&g);
            if (g.phase == SAUSAGE_PHASE_OVER) break;
            if (g.blocked[g.bone] || g.occupied[g.bone]) bone_ok = false;
            if (memcmp(before, g.blocked, sizeof(before)) == 0) continue;

            if (g.tier == tier) only_on_tier_up = false;
            tier_ups++;
            uint8_t head = head_of(&g);
            for (int c = 0; c < SAUSAGE_CELLS; c++) {
                if (before[c] && !g.blocked[c]) grow_ok = false;
                if (!g.blocked[c] || before[c]) continue;
                if (g.occupied[c]) body_ok = false;
                if (distance((uint8_t)c, head) <= SAUSAGE_ROCK_SAFE_DIST) safe_ok = false;
                int n = head;
                for (int k = 0; k < SAUSAGE_ROCK_SAFE_AHEAD && n >= 0; k++) {
                    n = step_cell((uint8_t)n, g.dir);
                    if (n == c) safe_ok = false;
                }
            }
            if (!field_ok(&g)) fair_ok = false;
            if (!segments_ok(&g)) shape_ok = false;
            if (count_blocked(&g) != g.rock_segments * SAUSAGE_ROCK_LEN ||
                g.rock_segments > sausage_rock_segments(g.tier)) {
                count_ok = false;
            }
            short_segments += sausage_rock_segments(g.tier) - g.rock_segments;
        }
    }
    CHECK(tier_ups > 500, "机器人打出了足够多次升档");
    CHECK(only_on_tier_up, "只在升档时加石头");
    CHECK(grow_ok, "已有的石头不会消失或挪动");
    CHECK(body_ok, "新石头不压身体");
    CHECK(safe_ok, "新石头离头至少 3 格，也不在正前方 3 格以内");
    CHECK(fair_ok, "空地连成一片，没有死胡同");
    CHECK(shape_ok, "每段都是横的或竖的 3 块，段与段不挨着");
    CHECK(count_ok, "石头数和记录的段数一致，不超过本档配置");
    CHECK(bone_ok, "骨头从不出现在石头或身体上");
    printf("    升档 %d 次，一共少摆 %d 段（场地太挤时）\n", tier_ups, short_segments);
}

static void test_pause_and_quit(void) {
    printf("暂停和手动结束\n");
    SausageGame g;
    sausage_init(&g, 9);
    to_run(&g);
    park_bone(&g, 0, 9);
    sausage_step(&g, 300);
    CHECK(sausage_toggle_pause(&g) && g.phase == SAUSAGE_PHASE_PAUSED, "走动中可以暂停");
    uint32_t elapsed = g.elapsed_ms;
    SausageDir selected = g.next_dir;
    CHECK(!sausage_turn(&g) && g.next_dir == selected, "暂停中按下不改变所选方向");
    sausage_step(&g, 60000);
    CHECK(g.moves == 0 && g.elapsed_ms == elapsed, "暂停一分钟不动、不计时");
    CHECK(sausage_toggle_pause(&g), "再按继续");
    sausage_step(&g, sausage_step_ms(1) - 1);
    CHECK(g.moves == 0, "继续后重新走一个完整节拍");

    sausage_toggle_pause(&g);
    CHECK(sausage_quit(&g) && g.phase == SAUSAGE_PHASE_OVER && g.end == SAUSAGE_END_QUIT, "暂停中可以结束本局");
    CHECK(!sausage_quit(&g) && !sausage_toggle_pause(&g) && !sausage_turn(&g), "结束后按键无效");

    sausage_init(&g, 9);
    CHECK(sausage_quit(&g) && g.end == SAUSAGE_END_QUIT, "倒计时中也可以结束本局");
    sausage_init(&g, 9);
    to_run(&g);
    CHECK(sausage_quit(&g) && g.end == SAUSAGE_END_QUIT, "走动中也可以结束本局");
    uint16_t moves = g.moves;
    sausage_step(&g, 10000);
    CHECK(g.moves == moves, "结束后不再移动");
}

static void test_full(void) {
    printf("场地满了\n");
    SausageGame g;
    sausage_init(&g, 10);
    to_run(&g);
    for (int c = 0; c < SAUSAGE_CELLS; c++) g.blocked[c] = true;
    g.blocked[cell(5, 4)] = g.blocked[cell(4, 4)] = g.blocked[cell(3, 4)] = false;
    g.blocked[cell(6, 4)] = false;
    g.bones = SAUSAGE_GROW_EVERY - 1;  // 这一根会长一节，尾巴不腾地方
    park_bone(&g, 6, 4);
    tick(&g);
    CHECK(g.bones == SAUSAGE_GROW_EVERY && g.phase == SAUSAGE_PHASE_OVER && g.end == SAUSAGE_END_FULL,
          "吃完最后一个空格子就结束");
}

static void test_reward(void) {
    printf("结算\n");
    SausageReward r = sausage_reward(0);
    CHECK(r.stars == 0 && r.shells == 5 && r.spirit == 0, "0 根 0 星 5 贝壳");
    r = sausage_reward(5);
    CHECK(r.stars == 1 && r.shells == 10 && r.spirit == 1, "5 根 1 星 10 贝壳");
    r = sausage_reward(10);
    CHECK(r.stars == 2 && r.shells == 20 && r.spirit == 2, "10 根 2 星 20 贝壳");
    r = sausage_reward(15);
    CHECK(r.stars == 3 && r.shells == 30 && r.spirit == 3, "15 根满三星 30 贝壳");
    r = sausage_reward(80);
    CHECK(r.stars == 3 && r.shells == 30 && r.spirit == 3, "满三星后奖励不再增加");
}

static void test_deterministic(void) {
    printf("可复现\n");
    SausageGame a, b;
    sausage_init(&a, 1234);
    sausage_init(&b, 1234);
    while (a.phase != SAUSAGE_PHASE_OVER && a.moves < 3000) {
        bot_tick(&a);
        bot_tick(&b);
    }
    CHECK(memcmp(&a, &b, sizeof(a)) == 0, "同样的种子和按键，结果完全一样");
}

// ---- 机器人模拟 ----

static void simulate(const char* name, int fail_pct) {
    srand(4321);
    uint32_t bones = 0, ms = 0, stars[4] = {0}, best = 0, over25 = 0, stuck = 0;
    uint32_t ends[SAUSAGE_END_FULL + 1] = {0};
    uint32_t tiers[SAUSAGE_MAX_TIER + 1] = {0};
    for (uint32_t seed = 1; seed <= GAMES; seed++) {
        SausageGame g;
        sausage_init(&g, seed);
        while (g.phase != SAUSAGE_PHASE_OVER && g.moves < 5000) {
            to_run(&g);
            int n = choose_presses(&g);
            if (n == 1 && rand() % 100 < fail_pct) n = 0;
            press(&g, n);
            tick(&g);
        }
        if (g.phase != SAUSAGE_PHASE_OVER) stuck++;
        bones += g.bones;
        ms += g.elapsed_ms;
        stars[sausage_reward(g.bones).stars]++;
        ends[g.end]++;
        tiers[g.tier]++;
        if (g.bones > best) best = g.bones;
        if (g.bones >= 25) over25++;
    }
    printf("    %-14s 平均 %.1f 根、%.0f 秒，最多 %u 根；满三星 %u%%  吃到 25 根 %u%%  0 星 %u%%\n", name,
           (double)bones / GAMES, ms / 1000.0 / GAMES, best, stars[3] * 100 / GAMES, over25 * 100 / GAMES,
           stars[0] * 100 / GAMES);
    printf("      结束原因：撞墙 %u%%  撞障碍 %u%%  撞自己 %u%%  场地满 %u 局", ends[SAUSAGE_END_WALL] * 100 / GAMES,
           ends[SAUSAGE_END_OBSTACLE] * 100 / GAMES, ends[SAUSAGE_END_SELF] * 100 / GAMES, ends[SAUSAGE_END_FULL]);
    if (stuck) printf("  机器人绕圈没吃到 %u 局", stuck);
    printf("\n      停在第几档：");
    for (int t = 1; t <= SAUSAGE_MAX_TIER; t++) printf(" %d:%u%%", t, tiers[t] * 100 / GAMES);
    printf("\n");
}

static void test_simulation(void) {
    printf("机器人各打 %d 局（时长含倒计时）\n", GAMES);
    simulate("右转从不按漏", 0);
    simulate("右转 10% 按漏", 10);
    simulate("右转 25% 按漏", 25);
}

int main(void) {
    test_start();
    test_turns_and_crash();
    test_grow();
    test_tiers();
    test_rocks();
    test_pause_and_quit();
    test_full();
    test_reward();
    test_deterministic();
    test_simulation();

    if (s_failures == 0) {
        printf("✅ Sausage Snake 规则测试全部通过\n");
        return 0;
    }
    printf("❌ %d 项失败\n", s_failures);
    return 1;
}

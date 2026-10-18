/**
 * 贪吃腊肠规则单元测试（在 Mac 上运行：./run_tests.sh）
 * 除了逐条规则，还让一个简单的机器人打 500 局，看得分和单局时长，方便调数值
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/games/snake.h"

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
    return (uint8_t)(y * SNAKE_SIZE + x);
}

static int cell_x(uint8_t c) { return c % SNAKE_SIZE; }
static int cell_y(uint8_t c) { return c / SNAKE_SIZE; }

static int distance(uint8_t a, uint8_t b) {
    return abs(cell_x(a) - cell_x(b)) + abs(cell_y(a) - cell_y(b));
}

static uint8_t head_of(const SnakeGame* g) {
    return snake_segment(g, 0);
}

// 跳过过关停顿和倒计时，停在刚开始走动的时刻
static void to_run(SnakeGame* g) {
    if (g->phase == SNAKE_PHASE_LEVEL_UP) snake_step(g, SNAKE_LEVEL_UP_MS - g->phase_ms);
    if (g->phase == SNAKE_PHASE_COUNTDOWN) snake_step(g, SNAKE_COUNTDOWN_MS + SNAKE_GO_MS - g->phase_ms);
}

// 走满一个节拍，正好移动一次
static void tick(SnakeGame* g) {
    snake_step(g, snake_step_ms(g->level) - g->phase_ms);
}

// 把骨头放在头正前方吃掉
static void eat_ahead(SnakeGame* g) {
    uint8_t head = snake_segment(g, 0);
    g->bone = (uint8_t)(head + 1);
    tick(g);
}

// 本关只差一根时吃掉它
static void finish_level(SnakeGame* g) {
    g->level_bones = SNAKE_LEVEL_BONES - 1;
    g->bones = (uint16_t)((g->level - 1) * SNAKE_LEVEL_BONES + SNAKE_LEVEL_BONES - 1);
    eat_ahead(g);
}

static void press(SnakeGame* g, int n) {
    for (int i = 0; i < n; i++) snake_turn(g);
}

// 把骨头挪到不碍事的地方
static void park_bone(SnakeGame* g, int x, int y) {
    g->bone = cell(x, y);
}

// 按给定的格子重新摆身体，cells 从头到尾，每项是 {x, y}
static void set_body(SnakeGame* g, const int (*cells)[2], int n, SnakeDir dir) {
    memset(g->occupied, 0, sizeof(g->occupied));
    g->len = (uint8_t)n;
    g->head = (uint8_t)(n - 1);
    for (int i = 0; i < n; i++) {
        uint8_t c = cell(cells[i][0], cells[i][1]);
        g->body[n - 1 - i] = c;
        g->occupied[c] = true;
    }
    g->dir = dir;
    g->turns = 0;
}

static void test_start(void) {
    printf("开局\n");
    SnakeGame g;
    snake_init(&g, 1);
    CHECK(g.len == 3 && head_of(&g) == cell(5, 4) && snake_segment(&g, 1) == cell(4, 4) &&
          snake_segment(&g, 2) == cell(3, 4), "身体 3 格，头在中间、往左排");
    CHECK(g.dir == SNAKE_RIGHT && g.level == 1, "第 1 关，初始朝右");
    int occupied = 0, blocked = 0;
    for (int c = 0; c < SNAKE_CELLS; c++) {
        occupied += g.occupied[c];
        blocked += g.blocked[c];
    }
    CHECK(occupied == 3, "占用格和身体一致");
    CHECK(blocked == 0, "第 1 关没有障碍");

    CHECK(snake_countdown(&g) == 3, "先显示 3");
    CHECK(!snake_turn(&g) && !snake_toggle_pause(&g), "倒计时中转向、暂停都不算");
    snake_step(&g, 1000);
    CHECK(snake_countdown(&g) == 2, "1 秒后显示 2");
    snake_step(&g, 2000);
    CHECK(snake_countdown(&g) == 0 && g.phase == SNAKE_PHASE_COUNTDOWN, "3 秒后显示「开始!」");
    snake_step(&g, SNAKE_GO_MS);
    CHECK(g.phase == SNAKE_PHASE_RUN && g.moves == 0, "「开始!」之后开始走动，还没移动");
    park_bone(&g, 0, 9);
    snake_step(&g, snake_step_ms(1) - 1);
    CHECK(g.moves == 0, "一个节拍没满不动");
    snake_step(&g, 1);
    CHECK(g.moves == 1 && head_of(&g) == cell(6, 4) && snake_segment(&g, 2) == cell(4, 4),
          "节拍到了往右走一格，尾巴跟上");

    bool far = true, free_cell = true;
    for (uint32_t seed = 1; seed <= 1000; seed++) {
        snake_init(&g, seed);
        if (distance(g.bone, head_of(&g)) < SNAKE_BONE_MIN_DIST) far = false;
        if (g.occupied[g.bone]) free_cell = false;
    }
    CHECK(free_cell && far, "开局骨头在空格子上，离头至少 3 格");
}

static void test_turns(void) {
    printf("顺时针转向\n");
    SnakeGame g;
    snake_init(&g, 2);
    to_run(&g);
    park_bone(&g, 0, 9);

    CHECK(snake_next_dir(&g) == SNAKE_RIGHT, "不按就直走");
    press(&g, 1);
    CHECK(snake_next_dir(&g) == SNAKE_DOWN, "按一次：预计向下（右转）");
    press(&g, 1);
    CHECK(snake_next_dir(&g) == SNAKE_RIGHT, "按两次是掉头，忽略，预计照原方向");
    press(&g, 1);
    CHECK(snake_next_dir(&g) == SNAKE_UP, "按三次：预计向上（左转）");
    CHECK(!snake_turn(&g) && g.turns == SNAKE_MAX_TURNS, "最多攒 3 次，再按不算");

    uint8_t before = head_of(&g);
    tick(&g);
    CHECK(g.moves == 1 && head_of(&g) == cell(cell_x(before), cell_y(before) - 1),
          "三连按直接往上走一格，中间不会先右再下");
    CHECK(g.dir == SNAKE_UP && g.turns == 0, "移动后方向更新，攒的转向清空");

    press(&g, 3);
    before = head_of(&g);
    tick(&g);
    CHECK(head_of(&g) == cell(cell_x(before) - 1, cell_y(before)) && g.dir == SNAKE_LEFT, "连续两个左转");

    press(&g, 2);
    before = head_of(&g);
    tick(&g);
    CHECK(head_of(&g) == cell(cell_x(before) - 1, cell_y(before)) && g.end == SNAKE_END_NONE,
          "同一节拍按两次不会掉头撞上自己");

    press(&g, 1);
    tick(&g);
    CHECK(g.dir == SNAKE_UP, "按一次右转：向左走时右转是向上");
    press(&g, 1);
    tick(&g);
    CHECK(g.dir == SNAKE_RIGHT && g.end == SNAKE_END_NONE, "分两个节拍各按一次，是两次右转，可以绕回来");
}

static void test_wall(void) {
    printf("撞墙\n");
    SnakeGame g;
    snake_init(&g, 3);
    to_run(&g);
    park_bone(&g, 0, 9);
    for (int i = 0; i < SNAKE_SIZE - 1 - SNAKE_START_X; i++) tick(&g);
    CHECK(head_of(&g) == cell(SNAKE_SIZE - 1, SNAKE_START_Y) && g.end == SNAKE_END_NONE, "走到墙边还活着");
    tick(&g);
    CHECK(g.end == SNAKE_END_WALL && g.phase == SNAKE_PHASE_OVER, "再往前撞墙，立即结束");
    CHECK(head_of(&g) == cell(SNAKE_SIZE - 1, SNAKE_START_Y), "头停在墙边，不会穿出去");
    uint16_t moves = g.moves;
    snake_step(&g, 10000);
    CHECK(g.moves == moves && !snake_turn(&g) && !snake_toggle_pause(&g), "结束后不再移动，按键无效");

    snake_init(&g, 3);
    to_run(&g);
    park_bone(&g, 0, 9);
    for (int i = 0; i < SNAKE_SIZE - 1 - SNAKE_START_X; i++) tick(&g);
    press(&g, 3);
    tick(&g);
    CHECK(g.end == SNAKE_END_NONE && g.dir == SNAKE_UP, "贴墙时紧急左转");
}

static void test_self(void) {
    printf("撞到自己\n");
    SnakeGame g;
    snake_init(&g, 4);
    to_run(&g);
    const int line[][2] = {{5, 4}, {4, 4}, {3, 4}, {2, 4}, {1, 4}};
    set_body(&g, line, 5, SNAKE_RIGHT);
    park_bone(&g, 9, 9);
    press(&g, 1);
    tick(&g);
    press(&g, 1);
    tick(&g);
    CHECK(g.end == SNAKE_END_NONE && head_of(&g) == cell(4, 5), "右转两次绕到身体下面");
    press(&g, 1);
    tick(&g);
    CHECK(g.end == SNAKE_END_SELF && g.phase == SNAKE_PHASE_OVER, "再右转撞上身体，立即结束");

    // 4 格首尾相接绕圈：头进入的正是尾巴这一步要离开的格子
    snake_init(&g, 4);
    to_run(&g);
    const int ring[][2] = {{5, 5}, {5, 4}, {4, 4}, {4, 5}};
    set_body(&g, ring, 4, SNAKE_DOWN);
    park_bone(&g, 9, 9);
    bool alive = true;
    for (int i = 0; i < 8; i++) {
        press(&g, 1);
        tick(&g);
        if (g.end != SNAKE_END_NONE) alive = false;
    }
    CHECK(alive && g.len == 4, "追着尾巴走不算撞到自己");
}

static void test_eat(void) {
    printf("吃骨头\n");
    SnakeGame g;
    snake_init(&g, 5);
    to_run(&g);
    park_bone(&g, SNAKE_START_X + 1, SNAKE_START_Y);
    tick(&g);
    CHECK(g.bones == 1 && g.len == 4, "吃到得 1 分，身体长一格");
    CHECK(snake_segment(&g, 3) == cell(SNAKE_START_X - 2, SNAKE_START_Y), "吃到的这一步尾巴不动");
    CHECK(!g.occupied[g.bone] && distance(g.bone, head_of(&g)) >= SNAKE_BONE_MIN_DIST,
          "新骨头在空格子上，离头至少 3 格");

    bool ok = true;
    for (uint32_t seed = 1; seed <= 300; seed++) {
        snake_init(&g, seed);
        to_run(&g);
        park_bone(&g, SNAKE_START_X + 1, SNAKE_START_Y);
        tick(&g);
        if (g.occupied[g.bone] || distance(g.bone, head_of(&g)) < SNAKE_BONE_MIN_DIST) ok = false;
    }
    CHECK(ok, "300 局里新骨头都离头够远");

    // 只剩头旁边一格空着时，放宽距离限制
    snake_init(&g, 6);
    to_run(&g);
    for (int c = 0; c < SNAKE_CELLS; c++) g.occupied[c] = true;
    g.occupied[cell(6, 4)] = false;
    g.occupied[cell(7, 4)] = false;
    park_bone(&g, 6, 4);
    tick(&g);
    CHECK(g.bones == 1 && g.bone == cell(7, 4), "空间不够时骨头可以离头很近");
}

static int count_blocked(const SnakeGame* g) {
    int n = 0;
    for (int c = 0; c < SNAKE_CELLS; c++) n += g->blocked[c];
    return n;
}

static void test_levels(void) {
    printf("三个关卡\n");
    CHECK(snake_step_ms(2) < snake_step_ms(1) && snake_step_ms(3) < snake_step_ms(2), "一关比一关快");
    CHECK(snake_rock_segments(1) == 0 && snake_rock_segments(2) > 0 &&
          snake_rock_segments(3) * snake_rock_len(3) > snake_rock_segments(2) * snake_rock_len(2),
          "第 1 关没石头，第 2 关起有，第 3 关更多");

    SnakeGame g;
    snake_init(&g, 10);
    to_run(&g);
    eat_ahead(&g);
    eat_ahead(&g);
    CHECK(g.level == 1 && g.level_bones == 2 && g.len == 5 && g.phase == SNAKE_PHASE_RUN, "没吃满接着走");
    finish_level(&g);
    CHECK(g.phase == SNAKE_PHASE_LEVEL_UP && g.level == 1 && g.bones == SNAKE_LEVEL_BONES, "吃满 5 根过关，先停一下");
    CHECK(!snake_turn(&g) && !snake_toggle_pause(&g), "过关停顿中按键无效");
    uint16_t moves = g.moves;
    snake_step(&g, SNAKE_LEVEL_UP_MS - 1);
    CHECK(g.phase == SNAKE_PHASE_LEVEL_UP && g.moves == moves, "停顿中不动");
    snake_step(&g, 1);
    CHECK(g.phase == SNAKE_PHASE_COUNTDOWN && g.level == 2 && snake_countdown(&g) == 3, "进入第 2 关，重新倒计时");
    CHECK(g.len == SNAKE_START_LEN && head_of(&g) == cell(SNAKE_START_X, SNAKE_START_Y) && g.dir == SNAKE_RIGHT &&
          g.level_bones == 0 && g.bones == SNAKE_LEVEL_BONES, "身体回到起点 3 格，总分保留");
    CHECK(count_blocked(&g) == snake_rock_segments(2) * snake_rock_len(2), "第 2 关摆出石头");
    bool body_clear = true;
    for (uint8_t i = 0; i < g.len; i++) body_clear = body_clear && !g.blocked[snake_segment(&g, i)];
    CHECK(body_clear && !g.blocked[g.bone] && !g.occupied[g.bone], "石头不压身体，骨头不在石头上");

    // 第 2 关迎面撞上石头
    to_run(&g);
    park_bone(&g, 0, 0);
    g.blocked[cell(SNAKE_START_X + 1, SNAKE_START_Y)] = true;
    tick(&g);
    CHECK(g.end == SNAKE_END_OBSTACLE && g.phase == SNAKE_PHASE_OVER, "撞上石头立即结束");
    CHECK(head_of(&g) == cell(SNAKE_START_X, SNAKE_START_Y), "头停在石头前");

    // 打到第 3 关：石头更多，速度变快
    snake_init(&g, 11);
    to_run(&g);
    finish_level(&g);
    to_run(&g);
    finish_level(&g);
    to_run(&g);
    CHECK(g.level == 3 && g.phase == SNAKE_PHASE_RUN &&
          count_blocked(&g) == snake_rock_segments(3) * snake_rock_len(3), "第 3 关石头更多");
    park_bone(&g, 0, 0);
    moves = g.moves;
    snake_step(&g, snake_step_ms(3) - 1);
    CHECK(g.moves == moves, "第 3 关差 1ms 还没动");
    snake_step(&g, 1);
    CHECK(g.moves == moves + 1, "第 3 关按本关速度走一格");

    finish_level(&g);
    CHECK(g.end == SNAKE_END_CLEAR && g.phase == SNAKE_PHASE_OVER && g.bones == SNAKE_LEVELS * SNAKE_LEVEL_BONES,
          "第 3 关吃满就是通关");

    bool ok = true;
    for (uint32_t seed = 1; seed <= 300; seed++) {
        snake_init(&g, seed);
        to_run(&g);
        finish_level(&g);
        to_run(&g);
        for (int i = 0; i < 4; i++) {
            if (g.blocked[g.bone] || g.occupied[g.bone]) ok = false;
            eat_ahead(&g);
            if (g.end != SNAKE_END_NONE) break;
        }
    }
    CHECK(ok, "有障碍时骨头从不出现在障碍上");
}

static int step_cell(uint8_t c, SnakeDir dir);

// 空地连成一片、没有死胡同
static bool field_ok(const SnakeGame* g) {
    bool seen[SNAKE_CELLS];
    int open = 0, first = -1;
    for (int c = 0; c < SNAKE_CELLS; c++) {
        seen[c] = g->blocked[c];
        if (g->blocked[c]) continue;
        open++;
        if (first < 0) first = c;
        int exits = 0;
        for (int d = 0; d < 4; d++) {
            int n = step_cell((uint8_t)c, (SnakeDir)d);
            exits += n >= 0 && !g->blocked[n];
        }
        if (exits < 2) return false;
    }
    uint8_t queue[SNAKE_CELLS];
    int head = 0, tail = 0;
    queue[tail++] = (uint8_t)first;
    seen[first] = true;
    while (head < tail) {
        uint8_t c = queue[head++];
        for (int d = 0; d < 4; d++) {
            int n = step_cell(c, (SnakeDir)d);
            if (n < 0 || seen[n]) continue;
            seen[n] = true;
            queue[tail++] = (uint8_t)n;
        }
    }
    return tail == open;
}

// 每块石头都属于一段横的或竖的、长度正好的石头，段与段之间不挨着
static bool segments_ok(const SnakeGame* g, int len) {
    bool used[SNAKE_CELLS] = {false};
    for (int c = 0; c < SNAKE_CELLS; c++) {
        if (!g->blocked[c] || used[c]) continue;
        int run_x = 0, run_y = 0;
        while (cell_x((uint8_t)c) + run_x < SNAKE_SIZE && g->blocked[c + run_x]) run_x++;
        while (cell_y((uint8_t)c) + run_y < SNAKE_SIZE && g->blocked[c + run_y * SNAKE_SIZE]) run_y++;
        bool horizontal = run_x == len && run_y == 1;
        bool vertical = run_y == len && run_x == 1;
        if (len == 1) horizontal = run_x == 1 && run_y == 1;
        if (!horizontal && !vertical) return false;
        for (int i = 0; i < len; i++) used[c + (vertical ? i * SNAKE_SIZE : i)] = true;
    }
    // 没被认领的石头说明有段连在一起了
    for (int c = 0; c < SNAKE_CELLS; c++) {
        if (g->blocked[c] != used[c]) return false;
    }
    // 同一段里斜对角不会有石头，有就是两段斜着挨上了
    for (int c = 0; c < SNAKE_CELLS; c++) {
        if (!g->blocked[c]) continue;
        for (int dy = -1; dy <= 1; dy += 2) {
            for (int dx = -1; dx <= 1; dx += 2) {
                int x = cell_x((uint8_t)c) + dx, y = cell_y((uint8_t)c) + dy;
                if (x >= 0 && x < SNAKE_SIZE && y >= 0 && y < SNAKE_SIZE && g->blocked[cell(x, y)]) return false;
            }
        }
    }
    return true;
}

static void test_rocks(void) {
    printf("随机石头\n");
    bool count_ok = true, row_ok = true, fair_ok = true, shape_ok = true, stay_ok = true;
    int distinct = 0, rerolled = 0;
    bool prev[SNAKE_CELLS] = {false};
    for (uint32_t seed = 1; seed <= 500; seed++) {
        SnakeGame g;
        snake_init(&g, seed);
        to_run(&g);
        finish_level(&g);
        to_run(&g);
        bool level2[SNAKE_CELLS];
        for (uint8_t level = 2; level <= SNAKE_LEVELS; level++) {
            int len = snake_rock_len(level);
            if (count_blocked(&g) != snake_rock_segments(level) * len) count_ok = false;
            for (int x = 0; x < SNAKE_SIZE; x++) {
                if (g.blocked[cell(x, SNAKE_START_Y)]) row_ok = false;
            }
            if (!field_ok(&g)) fair_ok = false;
            if (!segments_ok(&g, len)) shape_ok = false;

            if (level == 2) {
                if (memcmp(prev, g.blocked, sizeof(prev)) != 0) distinct++;
                memcpy(prev, g.blocked, sizeof(prev));
                memcpy(level2, g.blocked, sizeof(level2));
            } else if (memcmp(level2, g.blocked, sizeof(level2)) != 0) {
                rerolled++;
            }

            // 一关之内吃骨头、走动，石头不变
            bool before[SNAKE_CELLS];
            memcpy(before, g.blocked, sizeof(before));
            park_bone(&g, 0, 0);
            tick(&g);
            g.level_bones = 0;
            eat_ahead(&g);
            if (memcmp(before, g.blocked, sizeof(before)) != 0) stay_ok = false;

            if (level < SNAKE_LEVELS) {
                finish_level(&g);
                to_run(&g);
            }
        }
    }
    CHECK(count_ok, "每关都摆满配置的段数和长度");
    CHECK(row_ok, "出发那一行从不放石头");
    CHECK(fair_ok, "空地连成一片，没有死胡同");
    CHECK(shape_ok, "每段都是横的或竖的一条，段与段不挨着");
    CHECK(stay_ok, "一关之内石头不变");
    CHECK(distinct > 450, "每局石头位置都不一样");
    CHECK(rerolled > 450, "每关重新随机");
}

static void test_pause(void) {
    printf("暂停\n");
    SnakeGame g;
    snake_init(&g, 9);
    to_run(&g);
    park_bone(&g, 0, 9);
    snake_step(&g, 500);
    press(&g, 1);
    CHECK(snake_toggle_pause(&g) && g.phase == SNAKE_PHASE_PAUSED, "走动中可以暂停");
    uint32_t elapsed = g.elapsed_ms;
    snake_step(&g, 60000);
    CHECK(g.moves == 0 && g.elapsed_ms == elapsed, "暂停一分钟不动、不计时");
    CHECK(!snake_turn(&g) && g.turns == 1, "暂停时按确定不算，之前攒的保留");

    CHECK(snake_toggle_pause(&g) && g.phase == SNAKE_PHASE_RUN, "再按继续");
    snake_step(&g, snake_step_ms(1) - 1);
    CHECK(g.moves == 0, "继续后重新走一个完整节拍，不会因为暂停前走了一半就马上动");
    snake_step(&g, 1);
    CHECK(g.moves == 1 && g.dir == SNAKE_DOWN, "节拍满了才动，暂停前的转向照常生效");
}

static void test_reward(void) {
    printf("结算\n");
    SnakeReward r = snake_reward(0);
    CHECK(r.stars == 0 && r.shells == 5 && r.spirit == 0, "0 根 0 星 5 贝壳");
    r = snake_reward(4);
    CHECK(r.stars == 0, "第 1 关没过 0 星");
    r = snake_reward(5);
    CHECK(r.stars == 1 && r.shells == 10 && r.spirit == 1, "过第 1 关 1 星 10 贝壳 神 +1");
    r = snake_reward(10);
    CHECK(r.stars == 2 && r.shells == 20 && r.spirit == 2, "过第 2 关 2 星 20 贝壳");
    r = snake_reward(15);
    CHECK(r.stars == 3 && r.shells == 30 && r.spirit == 3, "通关 3 星 30 贝壳 神 +3");
}

// ---- 机器人模拟 ----

// 从 c 往 dir 走一格，出界返回 -1
static int step_cell(uint8_t c, SnakeDir dir) {
    int x = cell_x(c), y = cell_y(c);
    switch (dir) {
        case SNAKE_UP:    y--; break;
        case SNAKE_RIGHT: x++; break;
        case SNAKE_DOWN:  y++; break;
        case SNAKE_LEFT:  x--; break;
    }
    if (x < 0 || x >= SNAKE_SIZE || y < 0 || y >= SNAKE_SIZE) return -1;
    return cell(x, y);
}

// 头走到 from 之后还能到达多少空格子（尾巴那格算空）
static int reachable(const SnakeGame* g, uint8_t from) {
    bool seen[SNAKE_CELLS];
    for (int c = 0; c < SNAKE_CELLS; c++) seen[c] = g->occupied[c] || g->blocked[c];
    seen[snake_segment(g, g->len - 1)] = false;
    uint8_t queue[SNAKE_CELLS];
    int head = 0, tail = 0, count = 0;
    queue[tail++] = from;
    seen[from] = true;
    while (head < tail) {
        uint8_t c = queue[head++];
        count++;
        for (int d = 0; d < 4; d++) {
            int n = step_cell(c, (SnakeDir)d);
            if (n >= 0 && !seen[n]) {
                seen[n] = true;
                queue[tail++] = (uint8_t)n;
            }
        }
    }
    return count;
}

// 头走到 from 之后，绕开石头和身体走到骨头要几步；走不到返回一个大数
static int path_to_bone(const SnakeGame* g, uint8_t from) {
    if (from == g->bone) return 0;
    bool seen[SNAKE_CELLS];
    for (int c = 0; c < SNAKE_CELLS; c++) seen[c] = g->occupied[c] || g->blocked[c];
    seen[snake_segment(g, g->len - 1)] = false;
    uint8_t queue[SNAKE_CELLS];
    int dist[SNAKE_CELLS];
    int head = 0, tail = 0;
    queue[tail++] = from;
    seen[from] = true;
    dist[from] = 0;
    while (head < tail) {
        uint8_t c = queue[head++];
        for (int d = 0; d < 4; d++) {
            int n = step_cell(c, (SnakeDir)d);
            if (n < 0 || seen[n]) continue;
            if (n == g->bone) return dist[c] + 1;
            seen[n] = true;
            dist[n] = dist[c] + 1;
            queue[tail++] = (uint8_t)n;
        }
    }
    return SNAKE_CELLS;
}

// 每拍在直走、右转（按 1 次）、左转（按 3 次）里挑：不撞、走过去空间够大、离骨头最近（按绕路后的步数算）
// fail_pct：左转时三连按没按完的概率，只按出两次就会被当成掉头忽略、继续直走
static int choose_presses(const SnakeGame* g) {
    static const int PRESSES[] = {0, 1, 3};
    uint8_t head = head_of(g);
    uint8_t tail = snake_segment(g, g->len - 1);
    int best = 0, best_score = -100000;
    for (int i = 0; i < 3; i++) {
        SnakeDir dir = (SnakeDir)((g->dir + PRESSES[i]) % 4);
        int n = step_cell(head, dir);
        if (n < 0 || g->blocked[n] || (g->occupied[n] && !(n == tail && n != g->bone))) continue;
        int room = reachable(g, (uint8_t)n);
        int score = -path_to_bone(g, (uint8_t)n) * 4;
        if (room < g->len) score -= 1000;
        score += room / 10;
        if (PRESSES[i] == 3) score -= 1;
        if (score > best_score) {
            best_score = score;
            best = PRESSES[i];
        }
    }
    return best;
}

static void simulate(const char* name, int fail_pct) {
    srand(4321);
    uint32_t bones = 0, ms = 0, stars[4] = {0}, lefts = 0, failed = 0, stuck = 0;
    uint32_t ends[SNAKE_END_CLEAR + 1] = {0};
    for (uint32_t seed = 1; seed <= GAMES; seed++) {
        SnakeGame g;
        snake_init(&g, seed);
        while (g.phase != SNAKE_PHASE_OVER && g.moves < 3000) {
            to_run(&g);
            int n = choose_presses(&g);
            if (n == 3) {
                lefts++;
                if (rand() % 100 < fail_pct) {
                    n = 2;
                    failed++;
                }
            }
            press(&g, n);
            tick(&g);
        }
        if (g.phase != SNAKE_PHASE_OVER) stuck++;
        bones += g.bones;
        ms += g.elapsed_ms;
        stars[snake_reward(g.bones).stars]++;
        ends[g.end]++;
    }
    printf("    %-16s 平均 %.1f 根、%.0f 秒；通关(3 星) %u%%  过第 2 关 %u%%  过第 1 关 %u%%  0 星 %u%%\n", name,
           (double)bones / GAMES, ms / 1000.0 / GAMES, stars[3] * 100 / GAMES, stars[2] * 100 / GAMES,
           stars[1] * 100 / GAMES, stars[0] * 100 / GAMES);
    printf("      结束原因：撞墙 %u%%  撞障碍 %u%%  撞自己 %u%%", ends[SNAKE_END_WALL] * 100 / GAMES,
           ends[SNAKE_END_OBSTACLE] * 100 / GAMES, ends[SNAKE_END_SELF] * 100 / GAMES);
    if (stuck) printf("  机器人绕圈没吃到 %u 局", stuck);
    printf("\n");
    if (fail_pct) printf("      左转 %u 次，按漏 %u 次\n", lefts, failed);
}

static void test_simulation(void) {
    printf("机器人各打 %d 局（时长含倒计时）\n", GAMES);
    simulate("左转从不按漏", 0);
    simulate("左转 10% 按漏", 10);
    simulate("左转 25% 按漏", 25);
}

int main(void) {
    test_start();
    test_turns();
    test_wall();
    test_self();
    test_eat();
    test_levels();
    test_rocks();
    test_pause();
    test_reward();
    test_simulation();

    if (s_failures == 0) {
        printf("✅ 贪吃腊肠规则测试全部通过\n");
        return 0;
    }
    printf("❌ %d 项失败\n", s_failures);
    return 1;
}

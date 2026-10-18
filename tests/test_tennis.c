/**
 * 接网球规则单元测试（在 Mac 上运行：./run_tests.sh）
 * 除了逐条规则，还模拟不同手感的玩家各打 500 局，看得分和单局时长，方便调数值
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/games/tennis.h"

#define GAMES 500

static int s_failures;

#define CHECK(cond, msg)                                              \
    do {                                                              \
        if (!(cond)) {                                                \
            printf("  ✗ %s（第 %d 行）\n", msg, __LINE__);            \
            s_failures++;                                             \
        }                                                             \
    } while (0)

// 跳过倒计时和结算动画，停在下一球预备刚开始的时刻
static void to_serve(TennisGame* g) {
    if (g->phase == TENNIS_PHASE_COUNTDOWN) tennis_step(g, TENNIS_COUNTDOWN_MS + TENNIS_GO_MS - g->phase_ms);
    if (g->phase == TENNIS_PHASE_RESOLVE) tennis_step(g, TENNIS_RESOLVE_MS - g->phase_ms);
}

// 球到嘴边的时刻（按 serve_ms 算）和最佳起跳时刻
static int32_t arrive(const TennisGame* g) {
    return g->ready_ms + g->arrive_ms;
}

static int32_t best_jump(const TennisGame* g) {
    return arrive(g) - TENNIS_JUMP_PEAK_MS;
}

// 在本球的 serve_ms == at 时按键，然后一直推进到这一球结算
static void play_at(TennisGame* g, int32_t at) {
    if (at > (int32_t)g->serve_ms) tennis_step(g, (uint32_t)(at - (int32_t)g->serve_ms));
    tennis_jump(g);
    while (g->phase == TENNIS_PHASE_READY || g->phase == TENNIS_PHASE_FLIGHT) tennis_step(g, 1);
}

static void skip_ball(TennisGame* g) {
    while (g->phase == TENNIS_PHASE_READY || g->phase == TENNIS_PHASE_FLIGHT) tennis_step(g, 1);
}

static void test_start(void) {
    printf("开局\n");
    TennisGame g;
    tennis_init(&g, 1);
    CHECK(tennis_countdown(&g) == 3 && !tennis_jump(&g), "先倒计时 3，倒计时中按键无效");
    tennis_step(&g, 1000);
    CHECK(tennis_countdown(&g) == 2, "1 秒后显示 2");
    tennis_step(&g, 2000 + TENNIS_GO_MS);
    CHECK(g.phase == TENNIS_PHASE_READY && g.serves == 1, "倒计时完发第 1 球，先预备");
    CHECK(tennis_ball_progress(&g) == -1, "预备时球还没出来");
    tennis_step(&g, g.ready_ms);
    CHECK(g.phase == TENNIS_PHASE_FLIGHT && tennis_ball_progress(&g) == 0, "预备完球弹出");
    tennis_step(&g, g.arrive_ms / 2);
    CHECK(tennis_ball_progress(&g) == 500, "飞了一半");
}

static void test_jump(void) {
    printf("起跳动作\n");
    TennisGame g;
    tennis_init(&g, 2);
    to_serve(&g);
    CHECK(tennis_jump(&g) && g.jumping, "预备时可以起跳");
    CHECK(!tennis_jump(&g), "跳着的时候再按无效");
    CHECK(tennis_jump_lift(&g) == 0 && !tennis_mouth_open(&g), "刚按下先蹲，还在地上");
    tennis_step(&g, TENNIS_OPEN_START_MS);
    CHECK(tennis_mouth_open(&g) && tennis_jump_lift(&g) > 0, "张嘴时在空中");
    tennis_step(&g, TENNIS_JUMP_PEAK_MS - TENNIS_OPEN_START_MS);
    CHECK(tennis_jump_lift(&g) == 1000, "最高点");
    tennis_step(&g, TENNIS_OPEN_END_MS - TENNIS_JUMP_PEAK_MS);
    CHECK(!tennis_mouth_open(&g), "张嘴时间过了闭上");
    tennis_step(&g, TENNIS_JUMP_AIR_END_MS - TENNIS_OPEN_END_MS);
    CHECK(tennis_jump_lift(&g) == 0 && g.jumping, "落地缓一下");
    tennis_step(&g, TENNIS_JUMP_MS - TENNIS_JUMP_AIR_END_MS);
    CHECK(!g.jumping && g.phase == TENNIS_PHASE_READY, "动作做完，可以再跳");
    CHECK(tennis_jump(&g), "落地后能再跳");
}

static void test_catch(void) {
    printf("叼球判定\n");
    TennisGame g;
    tennis_init(&g, 3);
    to_serve(&g);
    play_at(&g, best_jump(&g));
    CHECK(g.phase == TENNIS_PHASE_RESOLVE && g.last == TENNIS_CATCH_PERFECT && g.score == 1 && g.perfects == 1,
          "最佳时机按：完美叼住，+1 分");
    CHECK(g.serve_ms == (uint32_t)(arrive(&g) - TENNIS_ZONE_MS), "球一进嘴边就被叼住，不再往后飞");

    int32_t window = TENNIS_ZONE_MS + (TENNIS_OPEN_END_MS - TENNIS_OPEN_START_MS) / 2;
    to_serve(&g);
    play_at(&g, best_jump(&g) - window);
    CHECK(g.last == TENNIS_CATCH_GOOD && g.score == 2, "提前 100ms 还能叼住");
    to_serve(&g);
    play_at(&g, best_jump(&g) + window);
    CHECK(g.last == TENNIS_CATCH_GOOD && g.score == 3, "推后 100ms 还能叼住");
    to_serve(&g);
    play_at(&g, best_jump(&g) + TENNIS_PERFECT_MS);
    CHECK(g.last == TENNIS_CATCH_PERFECT, "离最佳 30ms 以内算完美");

    to_serve(&g);
    play_at(&g, best_jump(&g) - window - 1);
    CHECK(g.last == TENNIS_MISS_EARLY && g.misses == 1 && g.score == 4, "再早 1ms：跳早了，失误一次，不扣分");
    CHECK(g.serve_ms == (uint32_t)(arrive(&g) + TENNIS_ZONE_MS), "球飞过嘴边才判漏接");
    to_serve(&g);
    play_at(&g, best_jump(&g) + window + 1);
    CHECK(g.last == TENNIS_MISS_LATE && g.misses == 2, "再晚 1ms：跳晚了");
    to_serve(&g);
    skip_ball(&g);
    CHECK(g.last == TENNIS_MISS_LATE && g.misses == 3, "不按也是漏接");
    CHECK(!tennis_jump(&g), "结算动画中按键无效");
    tennis_step(&g, TENNIS_RESOLVE_MS);
    CHECK(g.phase == TENNIS_PHASE_OVER, "失误 3 次结束");
    CHECK(!tennis_jump(&g) && !tennis_toggle_pause(&g), "结束后按键无效");

    // 在预备阶段就起跳，落地后球才弹出来
    tennis_init(&g, 4);
    to_serve(&g);
    play_at(&g, 0);
    CHECK(g.last == TENNIS_MISS_EARLY, "预备时乱跳：跳早了");
}

static void test_tiers(void) {
    printf("难度递进\n");
    TennisGame g;
    tennis_init(&g, 5);
    to_serve(&g);
    bool ok = true, slow = false, fast = false;
    for (int i = 0; i < 40; i++) {
        uint16_t s = g.score;
        uint16_t ready = s < 4 ? 1200 : s < 12 ? 800 : 700;
        if (s >= 12) ready = 700;
        if (g.ready_ms != ready) ok = false;
        if (s < 8 && g.arrive_ms != 1100) ok = false;
        if (s >= 8 && s < 12 && g.arrive_ms != 850) ok = false;
        if (s >= 12) {
            if (g.arrive_ms == TENNIS_SLOW_ARRIVE_MS) slow = true;
            else if (g.arrive_ms == 750) fast = true;
            else ok = false;
        }
        play_at(&g, best_jump(&g));
        to_serve(&g);
    }
    CHECK(g.score == 40 && g.misses == 0, "每球都对准就一直接得住");
    CHECK(ok, "0-3 分预备 1200ms，4 分起 800ms，8 分起球变快，12 分起预备 700ms");
    CHECK(slow && fast, "12 分起快慢球混合");
}

static void test_pause(void) {
    printf("暂停\n");
    TennisGame g;
    tennis_init(&g, 6);
    to_serve(&g);
    tennis_step(&g, g.ready_ms + 300);
    CHECK(tennis_toggle_pause(&g) && g.paused, "飞行中可以暂停");
    uint32_t serve = g.serve_ms, elapsed = g.elapsed_ms;
    tennis_step(&g, 60000);
    CHECK(g.serve_ms == serve && g.elapsed_ms == elapsed && g.phase == TENNIS_PHASE_FLIGHT, "暂停一分钟球不动");
    CHECK(!tennis_jump(&g), "暂停时按确定无效");
    CHECK(tennis_toggle_pause(&g) && !g.paused, "再按继续");
    play_at(&g, best_jump(&g));
    CHECK(g.last == TENNIS_CATCH_PERFECT, "继续后时机照旧");
}

static void test_reward(void) {
    printf("结算\n");
    TennisReward r = tennis_reward(3);
    CHECK(r.stars == 0 && r.shells == 5 && r.spirit == 0, "0-3 分 0 星 5 贝壳");
    r = tennis_reward(4);
    CHECK(r.stars == 1 && r.shells == 10 && r.spirit == 1, "4 分 1 星 10 贝壳");
    r = tennis_reward(7);
    CHECK(r.stars == 2 && r.shells == 20, "7 分 2 星 20 贝壳");
    r = tennis_reward(10);
    CHECK(r.stars == 3 && r.shells == 30 && r.spirit == 3, "10 分 3 星 30 贝壳 神 +3");
    r = tennis_reward(99);
    CHECK(r.stars == 3, "再多也是 3 星");
}

// 每球在最佳时机附近按，带 ±error_ms 的均匀误差
static void simulate(const char* name, int32_t error_ms) {
    srand(2468);
    uint32_t score = 0, ms = 0, stars[4] = {0}, best = 0;
    for (uint32_t seed = 1; seed <= GAMES; seed++) {
        TennisGame g;
        tennis_init(&g, seed);
        while (g.phase != TENNIS_PHASE_OVER && g.serves < 200) {
            to_serve(&g);
            int32_t off = error_ms ? rand() % (2 * error_ms + 1) - error_ms : 0;
            int32_t at = best_jump(&g) + off;
            play_at(&g, at < 0 ? 0 : at);
            tennis_step(&g, TENNIS_RESOLVE_MS);
        }
        score += g.score;
        ms += g.elapsed_ms;
        stars[tennis_reward(g.score).stars]++;
        if (g.score > best) best = g.score;
    }
    printf("    %-14s 平均 %.1f 分（最高 %u）、%.0f 秒；3 星 %u%%  2 星 %u%%  1 星 %u%%  0 星 %u%%\n", name,
           (double)score / GAMES, best, ms / 1000.0 / GAMES, stars[3] * 100 / GAMES, stars[2] * 100 / GAMES,
           stars[1] * 100 / GAMES, stars[0] * 100 / GAMES);
}

static void test_simulation(void) {
    printf("模拟玩家各 %d 局（时长含倒计时）\n", GAMES);
    simulate("误差 ±80ms", 80);
    simulate("误差 ±100ms", 100);
    simulate("误差 ±120ms", 120);
    simulate("误差 ±150ms", 150);
}

int main(void) {
    test_start();
    test_jump();
    test_catch();
    test_tiers();
    test_pause();
    test_reward();
    test_simulation();

    if (s_failures == 0) {
        printf("✅ 接网球规则测试全部通过\n");
        return 0;
    }
    printf("❌ %d 项失败\n", s_failures);
    return 1;
}

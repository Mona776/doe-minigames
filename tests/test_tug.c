/**
 * 拔河规则单元测试（在 Mac 上运行：./run_tests.sh）
 * 除了逐条规则，还模拟不同玩家各打 500 局，看胜率和单局时长，方便调数值
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/games/tug.h"

#define GAMES 500

static int s_failures;

#define CHECK(cond, msg)                                              \
    do {                                                              \
        if (!(cond)) {                                                \
            printf("  ✗ %s（第 %d 行）\n", msg, __LINE__);            \
            s_failures++;                                             \
        }                                                             \
    } while (0)

// 力量条从 0 往上走到 power 要多久
static uint32_t ms_to_power(const TugGame* g, uint8_t power) {
    return (uint32_t)power * g->sweep_ms / TUG_POWER_MAX;
}

// 跳过倒计时、主人发力和结算动画，停在下一次力量条刚出现的时刻
static void to_swing(TugGame* g) {
    while (g->phase != TUG_PHASE_SWING && g->phase != TUG_PHASE_OVER) {
        uint32_t limit = g->phase == TUG_PHASE_COUNTDOWN ? TUG_COUNTDOWN_MS + TUG_GO_MS
                       : g->phase == TUG_PHASE_OWNER     ? TUG_OWNER_ANIM_MS
                                                         : TUG_RESOLVE_MS;
        tug_step(g, limit - g->phase_ms);
    }
}

static void test_countdown(void) {
    printf("开局倒计时\n");
    TugGame g;
    tug_init(&g, 1);
    CHECK(g.phase == TUG_PHASE_COUNTDOWN && g.round == 0 && tug_countdown(&g) == 3, "开局显示 3");
    CHECK(!tug_lock(&g), "倒计时中按键无效");
    tug_step(&g, TUG_COUNTDOWN_MS / 3);
    CHECK(tug_countdown(&g) == 2, "1 秒后显示 2");
    tug_step(&g, TUG_COUNTDOWN_MS / 3);
    CHECK(tug_countdown(&g) == 1, "2 秒后显示 1");
    tug_step(&g, TUG_COUNTDOWN_MS / 3);
    CHECK(g.phase == TUG_PHASE_COUNTDOWN && tug_countdown(&g) == 0, "3 秒后显示「开始!」");
    tug_step(&g, TUG_GO_MS);
    CHECK(g.phase == TUG_PHASE_OWNER && g.round == 1, "然后第 1 轮主人先发力");
    CHECK(!tug_lock(&g), "主人发力时按键无效");
    tug_step(&g, TUG_OWNER_ANIM_MS);
    CHECK(g.phase == TUG_PHASE_SWING && tug_meter(&g) == 0, "发力动画播完才出力量条");
}

static void test_meter(void) {
    printf("力量条匀速往返\n");
    TugGame g;
    tug_init(&g, 1);
    to_swing(&g);
    uint32_t half = g.sweep_ms / 2;
    tug_step(&g, half);
    CHECK(tug_meter(&g) == TUG_POWER_MAX / 2, "单程一半是 50");
    tug_step(&g, half);
    CHECK(tug_meter(&g) == TUG_POWER_MAX, "单程走完是 100");
    tug_step(&g, half);
    CHECK(tug_meter(&g) == TUG_POWER_MAX / 2, "往回走");
    tug_step(&g, half);
    CHECK(tug_meter(&g) == 0, "回到 0");
}

static void test_judge(void) {
    printf("判定（主人 70，成功 ±8，完美 ±3）\n");
    CHECK(tug_judge(30, 70, 8, 3) == TUG_PULL_WEAK, "30 力量不足");
    CHECK(tug_judge(65, 70, 8, 3) == TUG_PULL_GOOD, "65 反拉成功");
    CHECK(tug_judge(72, 70, 8, 3) == TUG_PULL_PERFECT, "72 完美反拉");
    CHECK(tug_judge(95, 70, 8, 3) == TUG_PULL_SLIP, "95 用力过猛");
    CHECK(tug_judge(62, 70, 8, 3) == TUG_PULL_GOOD, "成功范围下沿算成功");
    CHECK(tug_judge(61, 70, 8, 3) == TUG_PULL_WEAK, "再轻一点就拉不动");
    CHECK(tug_judge(78, 70, 8, 3) == TUG_PULL_GOOD, "成功范围上沿算成功");
    CHECK(tug_judge(79, 70, 8, 3) == TUG_PULL_SLIP, "再重一点就滑倒");
    CHECK(tug_judge(73, 70, 8, 3) == TUG_PULL_PERFECT, "完美范围上沿");
    CHECK(tug_judge(66, 70, 8, 3) == TUG_PULL_GOOD, "完美范围外是普通成功");
}

static void test_force(void) {
    printf("主人力度分档\n");
    CHECK(tug_force(20) == TUG_FORCE_LIGHT, "最小拉力是轻");
    CHECK(tug_force(TUG_FORCE_MEDIUM - 1) == TUG_FORCE_LIGHT, "中档以下是轻");
    CHECK(tug_force(TUG_FORCE_MEDIUM) == TUG_FORCE_MID, "中档");
    CHECK(tug_force(TUG_FORCE_HEAVY - 1) == TUG_FORCE_MID, "重档以下是中");
    CHECK(tug_force(TUG_FORCE_HEAVY) == TUG_FORCE_STRONG, "重档");
    CHECK(tug_force(TUG_OWNER_MAX) == TUG_FORCE_STRONG, "最大拉力是重");
}

static uint8_t gain_of(const TugGame* g) {
    return g->force == TUG_FORCE_STRONG ? TUG_HEAVY_GAIN : 1;
}

// 力量条一出现就按，力量 0，一定拉不动
static void miss(TugGame* g) {
    tug_lock(g);
}

// 玩具在起点或偏主人一侧就完美反拉，偏 Doe 一侧就故意失手，局一直不结束，用来看后面的轮次
static void test_difficulty(void) {
    printf("难度：第 2、3 轮提升，之后不变\n");
    const uint16_t sweep[TUG_RAMP_ROUNDS] = TUG_SWEEP_MS_BY_ROUND;
    const uint8_t match[TUG_RAMP_ROUNDS] = TUG_MATCH_BY_ROUND;
    const uint8_t perfect[TUG_RAMP_ROUNDS] = TUG_PERFECT_BY_ROUND;
    const uint8_t owner_min[TUG_RAMP_ROUNDS] = TUG_OWNER_MIN_BY_ROUND;
    for (int i = 1; i < TUG_RAMP_ROUNDS; i++) {
        CHECK(sweep[i] < sweep[i - 1] && match[i] < match[i - 1] && perfect[i] <= perfect[i - 1] &&
              owner_min[i] > owner_min[i - 1], "数值表每一档都比上一档难");
    }

    TugGame g;
    tug_init(&g, 7);
    bool ok_table = true, ok_owner = true, ok_force = true, ok_gain = true;
    uint8_t prev_owner = 0;
    int heavy_early = 0, heavy_late = 0;
    for (int i = 0; i < 12; i++) {
        to_swing(&g);
        int tier = g.round < TUG_RAMP_ROUNDS ? g.round - 1 : TUG_RAMP_ROUNDS - 1;
        if (g.sweep_ms != sweep[tier] || g.match_range != match[tier] || g.perfect_range != perfect[tier] ||
            g.owner_min != owner_min[tier]) {
            ok_table = false;
        }
        if (g.owner_power < g.owner_min || g.owner_power > TUG_OWNER_MAX) ok_owner = false;
        if (g.round > 1 && abs((int)g.owner_power - (int)prev_owner) < TUG_OWNER_CHANGE) ok_owner = false;
        if (g.force != tug_force(g.owner_power)) ok_force = false;
        prev_owner = g.owner_power;

        uint8_t cell = g.cell;
        if (cell >= TUG_CELL_START) {
            tug_step(&g, ms_to_power(&g, g.owner_power));
            tug_lock(&g);
        } else {
            miss(&g);
            if (g.cell != cell + gain_of(&g) || g.lost_cells != gain_of(&g)) ok_gain = false;
        }
        tug_step(&g, TUG_RESOLVE_MS);
    }
    CHECK(g.result == TUG_RESULT_NONE && g.round == 13, "玩具在中间来回，局不结束");
    CHECK(ok_table, "第 1、2、3 轮按数值表逐档变难，第 3 轮以后一直是第 3 档");
    CHECK(ok_owner, "主人拉力不低于本轮下限，且每轮和上一轮差得够多");
    CHECK(ok_force, "力度档跟着拉力走");
    CHECK(ok_gain, "没接住时重拉退两格，其他退一格");

    // 主人变强：统计第 1 轮和第 3 轮以后的重拉比例
    for (uint32_t seed = 1; seed <= 300; seed++) {
        tug_init(&g, seed);
        for (int i = 0; i < 10; i++) {
            to_swing(&g);
            if (g.round == 1) heavy_early += g.force == TUG_FORCE_STRONG;
            if (g.round >= 3) heavy_late += g.force == TUG_FORCE_STRONG;
            if (g.cell >= TUG_CELL_START) {
                tug_step(&g, ms_to_power(&g, g.owner_power));
                tug_lock(&g);
            } else {
                miss(&g);
            }
            tug_step(&g, TUG_RESOLVE_MS);
        }
    }
    printf("    重拉比例：第 1 轮 %d%%，第 3 轮以后 %d%%\n", heavy_early * 100 / 300, heavy_late * 100 / 2400);
    CHECK(heavy_late * 300 > heavy_early * 2400, "后面的轮次重拉更多");
}

static void test_lock(void) {
    printf("锁定力量\n");
    TugGame g;
    tug_init(&g, 2);
    to_swing(&g);
    uint8_t owner = g.owner_power;
    tug_step(&g, ms_to_power(&g, owner));
    CHECK(tug_lock(&g), "摆动时可以锁定");
    CHECK(g.doe_power == owner, "锁定的是当前读数");
    CHECK(g.pull == TUG_PULL_PERFECT && g.perfects == 1 && g.cell == TUG_CELL_START - 1,
          "对准主人拉力是完美反拉，玩具向 Doe 移一格");
    CHECK(!tug_lock(&g), "每轮只能锁定一次");

    tug_step(&g, TUG_RESOLVE_MS - 1);
    CHECK(g.phase == TUG_PHASE_RESOLVE && !tug_lock(&g), "结算动画中按键无效");
    tug_step(&g, 1);
    CHECK(g.phase == TUG_PHASE_OWNER && g.round == 2, "动画结束进入下一轮，主人先发力");

    tug_init(&g, 2);
    to_swing(&g);
    tug_step(&g, g.sweep_ms);
    tug_lock(&g);
    CHECK(g.pull == TUG_PULL_SLIP && g.cell == TUG_CELL_START + gain_of(&g), "按在 100 用力过猛，玩具向主人移动");
}

static void test_no_timeout(void) {
    printf("不按就一直等\n");
    TugGame g;
    tug_init(&g, 3);
    to_swing(&g);
    uint8_t cell = g.cell;
    tug_step(&g, 3 * 60 * 1000);
    CHECK(g.phase == TUG_PHASE_SWING && g.cell == cell && g.round == 1, "3 分钟不按，力量条还在摆，玩具不动");
    uint32_t before = tug_meter(&g);
    tug_step(&g, g.sweep_ms / 4);
    CHECK(tug_meter(&g) != before, "力量条一直在动");
    CHECK(tug_lock(&g), "等多久都能按");

    tug_init(&g, 3);
    to_swing(&g);
    miss(&g);
    CHECK(g.doe_power == 0 && g.pull == TUG_PULL_WEAK && g.cell == TUG_CELL_START + gain_of(&g),
          "一出来就按是力量 0，拉不动");

    // 找一局第 1 轮是重拉的，确认没接住退两格
    for (uint32_t seed = 1; seed < 1000; seed++) {
        tug_init(&g, seed);
        to_swing(&g);
        if (g.force != TUG_FORCE_STRONG) continue;
        miss(&g);
        CHECK(g.cell == TUG_CELL_START + TUG_HEAVY_GAIN && g.lost_cells == TUG_HEAVY_GAIN, "重拉没接住退两格");
        return;
    }
    CHECK(false, "没找到第 1 轮重拉的局");
}

static void test_end(void) {
    printf("胜负\n");
    TugGame g;
    tug_init(&g, 4);
    for (int i = 0; i < TUG_CELL_START; i++) {
        to_swing(&g);
        tug_step(&g, ms_to_power(&g, g.owner_power));
        tug_lock(&g);
    }
    CHECK(g.cell == 0 && g.result == TUG_RESULT_NONE, "到终点后先播完这一轮的动画");
    tug_step(&g, TUG_RESOLVE_MS);
    CHECK(g.result == TUG_RESULT_WIN && g.phase == TUG_PHASE_OVER, "玩具到 Doe 一侧，Doe 获胜");
    CHECK(!tug_lock(&g), "结束后按键无效");

    bool in_range = true, quick_loss = true;
    for (uint32_t seed = 1; seed < 200; seed++) {
        tug_init(&g, seed);
        while (g.phase != TUG_PHASE_OVER) {
            to_swing(&g);
            miss(&g);
            if (g.cell > TUG_CELLS - 1) in_range = false;
            tug_step(&g, TUG_RESOLVE_MS);
        }
        if (g.result != TUG_RESULT_LOSE || g.round > TUG_CELLS - 1 - TUG_CELL_START) quick_loss = false;
    }
    CHECK(quick_loss, "每轮都失手，主人最多三轮获胜");
    CHECK(in_range, "退两格时玩具不会被拉出终点");

}

static void test_reward(void) {
    printf("结算星数\n");
    TugReward r = tug_reward(TUG_RESULT_WIN, 0);
    CHECK(r.stars == 3 && r.shells == 30 && r.spirit == 3, "零失误赢 3 星 30 贝壳 神 +3");
    r = tug_reward(TUG_RESULT_WIN, 1);
    CHECK(r.stars == 2 && r.shells == 20 && r.spirit == 2, "失误 1 次 2 星 20 贝壳");
    r = tug_reward(TUG_RESULT_WIN, TUG_TWO_STAR_MISSES);
    CHECK(r.stars == 2, "失误 2 次还是 2 星");
    r = tug_reward(TUG_RESULT_WIN, TUG_TWO_STAR_MISSES + 1);
    CHECK(r.stars == 1 && r.shells == 10 && r.spirit == 1, "失误 3 次 1 星 10 贝壳");
    r = tug_reward(TUG_RESULT_LOSE, 0);
    CHECK(r.stars == 0 && r.shells == 5 && r.spirit == 0, "输了 0 星 5 贝壳");

    bool monotonic = true;
    for (uint16_t m = 1; m < 50; m++) {
        if (tug_reward(TUG_RESULT_WIN, m).stars > tug_reward(TUG_RESULT_WIN, m - 1).stars) monotonic = false;
        if (tug_reward(TUG_RESULT_WIN, m).stars == 0) monotonic = false;
    }
    CHECK(monotonic, "失误越多星数不会越高，赢了至少 1 星");

    // 整局跑一遍：3 轮完美拿下是 3 星；第 1 轮故意失手再赢回来，失误记 1 次、不是 3 星
    TugGame g;
    tug_init(&g, 4);
    while (g.phase != TUG_PHASE_OVER) {
        to_swing(&g);
        tug_step(&g, ms_to_power(&g, g.owner_power));
        tug_lock(&g);
        tug_step(&g, TUG_RESOLVE_MS);
    }
    CHECK(g.misses == 0 && tug_reward(g.result, g.misses).stars == 3, "三轮全中 3 星");

    tug_init(&g, 4);
    to_swing(&g);
    miss(&g);
    tug_step(&g, TUG_RESOLVE_MS);
    while (g.phase != TUG_PHASE_OVER) {
        to_swing(&g);
        tug_step(&g, ms_to_power(&g, g.owner_power));
        tug_lock(&g);
        tug_step(&g, TUG_RESOLVE_MS);
    }
    CHECK(g.result == TUG_RESULT_WIN && g.misses == 1 && tug_reward(g.result, g.misses).stars == 2,
          "失手一次再赢回来是 2 星");
}

typedef enum { AIM_OWNER, AIM_MAX, AIM_RANDOM } Strategy;

#define TRACKED_ROUNDS 8

// AIM_OWNER：在指针第一次经过主人拉力时按，带 ±error_ms 的均匀误差
// AIM_MAX：每次都按在 100；AIM_RANDOM：一个往返里随便按
// by_round 为真时额外打印每轮的反拉成功率，第 8 轮及以后合在一起
static int simulate(const char* name, Strategy strategy, uint32_t error_ms, bool by_round) {
    srand(12345);
    int wins = 0;
    uint32_t rounds = 0, ms = 0, perfects = 0, star_count[4] = {0};
    uint32_t tries[TRACKED_ROUNDS] = {0}, hits[TRACKED_ROUNDS] = {0};
    for (uint32_t seed = 1; seed <= GAMES; seed++) {
        TugGame g;
        tug_init(&g, seed);
        while (g.phase != TUG_PHASE_OVER) {
            to_swing(&g);
            int32_t at;
            if (strategy == AIM_MAX) {
                at = g.sweep_ms;
            } else if (strategy == AIM_RANDOM) {
                at = rand() % (2 * g.sweep_ms);
            } else {
                int32_t offset = error_ms ? (int32_t)(rand() % (2 * error_ms + 1)) - (int32_t)error_ms : 0;
                at = (int32_t)ms_to_power(&g, g.owner_power) + offset;
                if (at < 0) at = 0;
            }
            tug_step(&g, (uint32_t)at);
            tug_lock(&g);
            int idx = g.round < TRACKED_ROUNDS ? g.round - 1 : TRACKED_ROUNDS - 1;
            tries[idx]++;
            hits[idx] += g.pull == TUG_PULL_PERFECT || g.pull == TUG_PULL_GOOD;
            tug_step(&g, TUG_RESOLVE_MS);
        }
        wins += g.result == TUG_RESULT_WIN;
        rounds += g.round;
        ms += g.elapsed_ms;
        perfects += g.perfects;
        star_count[tug_reward(g.result, g.misses).stars]++;
    }
    printf("    %-14s 胜率 %3d%%，平均 %.1f 轮、%.1f 秒，完美 %.1f 次\n", name, wins * 100 / GAMES,
           (double)rounds / GAMES, ms / 1000.0 / GAMES, (double)perfects / GAMES);
    printf("      星数分布：3 星 %u%%  2 星 %u%%  1 星 %u%%  0 星 %u%%\n", star_count[3] * 100 / GAMES,
           star_count[2] * 100 / GAMES, star_count[1] * 100 / GAMES, star_count[0] * 100 / GAMES);
    if (by_round) {
        printf("      每轮反拉成功率：");
        for (int i = 0; i < TRACKED_ROUNDS; i++) {
            if (tries[i]) printf("第%d%s轮 %u%%  ", i + 1, i == TRACKED_ROUNDS - 1 ? "+" : "", hits[i] * 100 / tries[i]);
        }
        printf("\n");
    }
    return wins;
}

static void test_simulation(void) {
    printf("模拟玩家各 %d 局（时长含倒计时）\n", GAMES);
    CHECK(simulate("对准 ±0ms", AIM_OWNER, 0, false) == GAMES, "每次对准主人拉力必赢");
    simulate("对准 ±30ms", AIM_OWNER, 30, false);
    simulate("对准 ±60ms", AIM_OWNER, 60, false);
    simulate("对准 ±100ms", AIM_OWNER, 100, true);
    simulate("对准 ±150ms", AIM_OWNER, 150, true);
    CHECK(simulate("固定按 100", AIM_MAX, 0, false) == 0, "固定按 100 赢不了");
    simulate("随便按", AIM_RANDOM, 0, false);
}

int main(void) {
    test_countdown();
    test_meter();
    test_judge();
    test_force();
    test_difficulty();
    test_lock();
    test_no_timeout();
    test_end();
    test_reward();
    test_simulation();

    if (s_failures == 0) {
        printf("✅ 拔河规则测试全部通过\n");
        return 0;
    }
    printf("❌ %d 项失败\n", s_failures);
    return 1;
}

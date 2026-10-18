/**
 * Doe 拔河 - 纯 C 规则，不依赖 LVGL 和硬件，单元测试直接调用
 *
 * 开局先倒计时 3、2、1。绳子上 7 个标记，玩具从中间出发。每轮主人先发力（按拉力分轻、中、重三档播动画），
 * 然后 Doe 的力量条在 0~100 之间匀速往返、一直等到玩家按，按一次锁定当前力量：和主人的拉力越接近越好——
 * 差得不多就反拉成功，玩具向 Doe 移一格；太轻拉不动、太重用力过猛滑倒，玩具向主人移一格，
 * 主人重力度的一拉没接住会被拉走两格。
 * 第 2、3 轮主人变强（拉力下限升高，重拉更多），力量条变快、成功范围变窄，之后保持不变。先拉到自己终点的一方获胜。
 * 时间按毫秒推进，同样的种子和输入一定得到同样的结果。
 */

#ifndef TUG_H
#define TUG_H

#include <stdbool.h>
#include <stdint.h>

// 数值，调手感只改这里
#define TUG_CELLS            7
#define TUG_CELL_START       3      // 0 是 Doe 的终点，TUG_CELLS - 1 是主人的终点
#define TUG_POWER_MAX        100
#define TUG_OWNER_MAX        90
#define TUG_OWNER_CHANGE     15     // 主人拉力和上一轮至少差这么多，逼玩家每轮换时机
#define TUG_FORCE_MEDIUM     44     // 主人拉力到这么大算中等力度
#define TUG_FORCE_HEAVY      67     // 到这么大算重力度
#define TUG_HEAVY_GAIN       2      // 重力度的一拉没接住，玩具被拉走这么多格

#define TUG_COUNTDOWN_MS     3000   // 开局倒计时 3、2、1
#define TUG_GO_MS            500    // 倒计时后「开始!」停留
#define TUG_OWNER_ANIM_MS    900    // 每轮主人发力动画
#define TUG_RESOLVE_MS       700    // 每轮结算动画

// 结算：赢了且一次没失误 3 星，失误不超过这么多次 2 星，再多 1 星，输了 0 星
#define TUG_TWO_STAR_MISSES  2
#define TUG_SHELLS_BY_STARS  {5, 10, 20, 30}  // 0~3 星对应的贝壳，神 +星数

// 难度：第 1、2、3 轮各一档，第 3 轮以后一直用第 3 档
#define TUG_RAMP_ROUNDS      3
#define TUG_SWEEP_MS_BY_ROUND  {1000, 800, 700}  // 力量条单程时间
#define TUG_MATCH_BY_ROUND     {10, 8, 7}        // 和主人拉力相差不超过这么多就反拉成功
#define TUG_PERFECT_BY_ROUND   {4, 3, 3}         // 相差不超过这么多是完美反拉
#define TUG_OWNER_MIN_BY_ROUND {20, 35, 45}      // 主人拉力下限，越往后重拉越多

typedef enum {
    TUG_PHASE_COUNTDOWN,  // 开局倒计时
    TUG_PHASE_OWNER,      // 主人发力动画
    TUG_PHASE_SWING,      // 力量条摆动，等玩家按
    TUG_PHASE_RESOLVE,    // 本轮已结算，播动画
    TUG_PHASE_OVER,
} TugPhase;

typedef enum {
    TUG_FORCE_LIGHT,
    TUG_FORCE_MID,
    TUG_FORCE_STRONG,
} TugForce;

typedef enum {
    TUG_PULL_PERFECT,  // 完美反拉
    TUG_PULL_GOOD,     // 反拉成功
    TUG_PULL_WEAK,     // 力量不足
    TUG_PULL_SLIP,     // 用力过猛，滑倒
} TugPull;

typedef enum {
    TUG_RESULT_NONE,
    TUG_RESULT_WIN,
    TUG_RESULT_LOSE,
} TugResult;

typedef struct {
    uint32_t rng;
    uint32_t elapsed_ms;
    uint8_t cell;             // 玩具所在格
    uint16_t round;           // 第几轮，从 1 开始；倒计时期间是 0
    TugPhase phase;
    uint32_t phase_ms;        // 本阶段已过的时间
    uint16_t perfects;        // 本局完美反拉次数
    uint16_t misses;          // 本局失误（没接住）次数

    // 本轮数值，每轮开始时定好
    uint8_t owner_min;        // 本轮主人拉力下限
    uint8_t owner_power;      // 主人拉力
    TugForce force;           // 主人力度档，决定发力动画
    uint16_t sweep_ms;        // 力量条单程时间
    uint8_t match_range;      // 成功范围（±）
    uint8_t perfect_range;    // 完美范围（±）

    // 下面几项在结算阶段有效
    uint8_t doe_power;
    TugPull pull;
    uint8_t lost_cells;       // 这一轮没接住被拉走几格，接住了是 0

    TugResult result;
} TugGame;

typedef struct {
    uint8_t stars;
    uint16_t shells;
    uint8_t spirit;
} TugReward;

void tug_init(TugGame* g, uint32_t seed);

// 倒计时阶段显示的数字：3、2、1，到 0 表示该显示「开始!」
uint8_t tug_countdown(const TugGame* g);

// 当前力量条读数，摆动阶段有效
uint8_t tug_meter(const TugGame* g);

TugForce tug_force(uint8_t owner_power);

// 按 Doe 和主人的力量判定这一拉，range 用本轮的成功/完美范围
TugPull tug_judge(uint8_t doe_power, uint8_t owner_power, uint8_t match_range, uint8_t perfect_range);

// 锁定当前力量并结算本轮；不在摆动阶段（倒计时、主人发力、结算动画中、已结束、本轮已锁定）返回 false
bool tug_lock(TugGame* g);

void tug_step(TugGame* g, uint32_t dt_ms);

// 星数只看胜负和失误次数，局拖得再长也不会多拿星
TugReward tug_reward(TugResult result, uint16_t misses);

#endif

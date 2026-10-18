/**
 * Doe 接网球 - 纯 C 规则，不依赖 LVGL 和硬件，单元测试直接调用
 *
 * 开局先倒计时 3、2、1。之后发球机一球一球地发：先预备（亮灯、抖动），再把球弹出，球沿固定弧线飞向 Doe。
 * 确定键让 Doe 起跳叼球，是一个固定动作：先蹲一下、跳起、空中张嘴一小段时间、落地，动作中再按无效。
 * 只有张嘴的这段时间和球经过嘴边的时间重叠才叼得住；按早了落地后球才到，按晚了球已经飞过去。
 * 叼住 +1 分；漏接算一次失误，失误 3 次结束。分数越高发球间隔越短、球越快，12 分起快慢球混合。
 * 暂停时不计时。时间按毫秒推进，同样的种子和输入一定得到同样的结果。
 */

#ifndef TENNIS_H
#define TENNIS_H

#include <stdbool.h>
#include <stdint.h>

// 数值，调手感只改这里
#define TENNIS_MAX_MISSES     3

#define TENNIS_COUNTDOWN_MS   3000   // 开局倒计时 3、2、1
#define TENNIS_GO_MS          500    // 倒计时后「开始!」停留
#define TENNIS_RESOLVE_MS     900    // 每球结算动画

// 起跳动作：按下后先蹲 WINDUP，然后在空中到 AIR_END，最高点在 PEAK；张嘴是 OPEN_START~OPEN_END；JUMP_MS 后能再跳
#define TENNIS_JUMP_WINDUP_MS 60
#define TENNIS_JUMP_PEAK_MS   240
#define TENNIS_JUMP_AIR_END_MS 420
#define TENNIS_JUMP_MS        520
#define TENNIS_OPEN_START_MS  180
#define TENNIS_OPEN_END_MS    300
#define TENNIS_ZONE_MS        40     // 慢球（1100ms 到嘴边）在嘴边能被叼到的时间，到达前后各这么久；球越快越短
#define TENNIS_PERFECT_MS     30     // 按键离最佳时机不超过这么多是完美

// 难度：得分到 TIER_SCORE[i] 起用第 i 档；预备时间、球从弹出到嘴边的时间；最后一档快慢球混合
#define TENNIS_TIERS          4
#define TENNIS_TIER_SCORE     {0, 4, 8, 12}
#define TENNIS_READY_MS       {1200, 800, 800, 700}
#define TENNIS_ARRIVE_MS      {1100, 1100, 850, 750}
#define TENNIS_MIXED_TIER     3
#define TENNIS_SLOW_ARRIVE_MS 1100   // 混合档里的慢球

// 结算：得分到这么多分别是 1、2、3 星，0~3 星对应的贝壳，神 +星数
#define TENNIS_STAR_SCORE     {4, 7, 10}
#define TENNIS_SHELLS_BY_STARS {5, 10, 20, 30}

typedef enum {
    TENNIS_PHASE_COUNTDOWN,
    TENNIS_PHASE_READY,    // 发球机预备
    TENNIS_PHASE_FLIGHT,   // 球在飞
    TENNIS_PHASE_RESOLVE,  // 这一球已结算，播动画
    TENNIS_PHASE_OVER,
} TennisPhase;

typedef enum {
    TENNIS_CATCH_PERFECT,
    TENNIS_CATCH_GOOD,
    TENNIS_MISS_EARLY,  // 跳早了
    TENNIS_MISS_LATE,   // 跳晚了或没跳
} TennisCatch;

typedef struct {
    uint32_t rng;
    uint32_t elapsed_ms;      // 不含暂停
    TennisPhase phase;
    uint32_t phase_ms;        // 倒计时、结算动画的进度
    bool paused;

    // 本球：serve_ms 从预备开始一直走到下一球，球弹出时刻是 ready_ms，到嘴边是 ready_ms + arrive_ms
    uint32_t serve_ms;
    uint16_t ready_ms;
    uint16_t arrive_ms;
    int32_t jump_at;          // 本球起跳时的 serve_ms，没跳是 -1
    int32_t catch_at;         // 这一跳能叼住时，叼住那一刻的 serve_ms；叼不住是 -1

    bool jumping;
    uint32_t jump_ms;         // 起跳后过了多久

    uint16_t serves;          // 第几球，从 1 开始；倒计时期间是 0
    uint16_t score;
    uint8_t misses;
    uint16_t perfects;
    TennisCatch last;         // 结算阶段有效
} TennisGame;

typedef struct {
    uint8_t stars;
    uint16_t shells;
    uint8_t spirit;
} TennisReward;

void tennis_init(TennisGame* g, uint32_t seed);

// 倒计时阶段显示的数字：3、2、1，到 0 表示该显示「开始!」
uint8_t tennis_countdown(const TennisGame* g);

// 按一次确定键：起跳；不在预备或飞行阶段、暂停中、上一跳还没落地都返回 false
bool tennis_jump(TennisGame* g);

bool tennis_toggle_pause(TennisGame* g);

void tennis_step(TennisGame* g, uint32_t dt_ms);

// 球飞了多远，千分比：0 刚弹出，1000 正好到嘴边，之后继续往后飞；球还没弹出时是 -1
int32_t tennis_ball_progress(const TennisGame* g);

// 起跳高度，千分比，0 是在地上
uint16_t tennis_jump_lift(const TennisGame* g);

bool tennis_mouth_open(const TennisGame* g);

TennisReward tennis_reward(uint16_t score);

#endif

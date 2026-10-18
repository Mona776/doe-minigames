/**
 * Sausage Snake - 贪吃腊肠的无尽版，纯 C 规则，不依赖 LVGL 和硬件，单元测试直接调用
 *
 * 10×10 网格，四周是墙，只有一条命、没有关卡。开局倒计时 3、2、1，Doe 从中间出发、朝右，身体 3 格，每个节拍自动前进一格。
 * 每吃 SAUSAGE_GROW_EVERY 根骨头身体才长一节，比贪吃腊肠长得慢；吃到但不长的那一步尾巴照常跟上。
 * 每次确定键按下都将所选方向顺时针转 90°，自动移动按当前所选方向执行。
 * 不限制两次移动之间的按下次数，不忽略反向选择；进入身体按正常自撞处理，尾巴本步刚好离开的格子可以进。
 * 难度按累计骨头数分档：0~4、5~9、10~14 根各一档（速度、石头和贪吃腊肠的三关一样），之后每 10 根升一档，
 * 每档更快、多一段石头，速度和石头段数都有上限，都到上限的第 SAUSAGE_MAX_TIER 档之后不再升档。
 * 升档时不停顿、不重置身体，直接在场地上追加石头补到本档的段数：除了贪吃腊肠的公平条件（两段不挨着、
 * 空地连成一片、没有死胡同，身体占的格子算空地），还要离头够远、不挡在头正前方，玩家来得及看到。
 * 摆不下的段就先少摆，下次升档再补。
 * 撞到、玩家手动结束、或者场地满到放不下骨头时结束；奖励按骨头数算，15 根满三星，之后继续玩只为刷新最高分。
 * 暂停时不计时，继续后重新走一个完整节拍。时间按毫秒推进，同样的种子和输入一定得到同样的结果。
 */

#ifndef SAUSAGE_H
#define SAUSAGE_H

#include <stdbool.h>
#include <stdint.h>

// 数值，调手感只改这里
#define SAUSAGE_SIZE           10
#define SAUSAGE_CELLS          (SAUSAGE_SIZE * SAUSAGE_SIZE)
#define SAUSAGE_START_LEN      3
#define SAUSAGE_START_X        5      // 开局头的位置，身体往左排
#define SAUSAGE_START_Y        4
#define SAUSAGE_BONE_MIN_DIST  3      // 新骨头离头至少这么多格（曼哈顿距离），空间不够时放宽
#define SAUSAGE_GROW_EVERY     2      // 每吃这么多根骨头身体长一节

#define SAUSAGE_COUNTDOWN_MS   3000   // 开局倒计时 3、2、1
#define SAUSAGE_GO_MS          500    // 倒计时后「GO」停留

// 难度档：前 3 档每 5 根一档，之后每 10 根一档
#define SAUSAGE_EARLY_TIERS          3
#define SAUSAGE_EARLY_TIER_BONES     5
#define SAUSAGE_LATE_TIER_BONES      10
#define SAUSAGE_MAX_TIER             8      // 到这一档速度已经最快、石头已经最多，之后不再升档
#define SAUSAGE_STEP_MS_EARLY        {550, 470, 400}
#define SAUSAGE_ROCK_SEGMENTS_EARLY  {0, 2, 3}
#define SAUSAGE_STEP_MS_LATE_DROP    30     // 第 4 档起每档快这么多
#define SAUSAGE_STEP_MS_MIN          250
#define SAUSAGE_ROCK_SEGMENTS_MAX    6      // 第 4 档起每档多一段，到这么多段为止
#define SAUSAGE_ROCK_LEN             3
#define SAUSAGE_ROCK_TRIES           100    // 每段最多试这么多个随机位置，都不合适就先少摆这一段
#define SAUSAGE_ROCK_SAFE_DIST       2      // 升档新摆的石头离头的曼哈顿距离至少大于这个数
#define SAUSAGE_ROCK_SAFE_AHEAD      3      // 也不摆在头正前方这么多格以内

// 结算：总共吃到这么多根分别是 1、2、3 星，0~3 星对应的贝壳，神 +星数
#define SAUSAGE_STAR_BONES      {5, 10, 15}
#define SAUSAGE_SHELLS_BY_STARS {5, 10, 20, 30}

// 按顺时针排，转一次就是加一
typedef enum {
    SAUSAGE_UP,
    SAUSAGE_RIGHT,
    SAUSAGE_DOWN,
    SAUSAGE_LEFT,
} SausageDir;

typedef enum {
    SAUSAGE_PHASE_COUNTDOWN,
    SAUSAGE_PHASE_RUN,
    SAUSAGE_PHASE_PAUSED,
    SAUSAGE_PHASE_OVER,
} SausagePhase;

typedef enum {
    SAUSAGE_END_NONE,
    SAUSAGE_END_WALL,      // 撞墙
    SAUSAGE_END_OBSTACLE,  // 撞障碍
    SAUSAGE_END_SELF,      // 撞到自己
    SAUSAGE_END_QUIT,      // 玩家手动结束
    SAUSAGE_END_FULL,      // 场地满了，放不下骨头
} SausageEnd;

typedef struct {
    uint32_t rng;
    uint32_t elapsed_ms;      // 不含暂停
    SausagePhase phase;
    uint32_t phase_ms;        // 倒计时的进度；走动时是离上一次移动过了多久
    uint8_t tier;             // 难度档，从 1 开始
    uint8_t rock_segments;    // 场上实际摆了几段石头
    SausageDir dir;           // 上一次移动的方向（撞到时是撞过去的方向）
    SausageDir next_dir;      // 当前所选方向，每次按下独立更新

    // 身体是环形缓冲：body[head] 是头，往前数 len 个格子是整条身体；格子编号 y * SAUSAGE_SIZE + x
    uint8_t body[SAUSAGE_CELLS];
    uint8_t head;
    uint8_t len;
    bool occupied[SAUSAGE_CELLS];
    bool blocked[SAUSAGE_CELLS];  // 障碍

    uint8_t bone;             // 骨头所在格
    uint16_t bones;           // 一共吃到的骨头数，就是分数
    uint16_t moves;           // 走了多少步，界面据此判断要不要重画
    SausageEnd end;
} SausageGame;

typedef struct {
    uint8_t stars;
    uint16_t shells;
    uint8_t spirit;
} SausageReward;

void sausage_init(SausageGame* g, uint32_t seed);

// 倒计时阶段显示的数字：3、2、1，到 0 表示该显示「GO」
uint8_t sausage_countdown(const SausageGame* g);

// 第 i 节身体所在格，0 是头，len - 1 是尾巴
uint8_t sausage_segment(const SausageGame* g, uint8_t i);

// 每次确定键按下：所选方向顺时针转 90°；仅走动阶段有效
bool sausage_turn(SausageGame* g);

// 当前所选方向，供下一次移动使用
SausageDir sausage_next_dir(const SausageGame* g);

// 暂停/继续；只在走动和暂停时有效，返回是否切换了
bool sausage_toggle_pause(SausageGame* g);

// 玩家手动结束本局，按现在的骨头数结算；已经结束时返回 false
bool sausage_quit(SausageGame* g);

void sausage_step(SausageGame* g, uint32_t dt_ms);

// 吃到这么多根骨头时在第几档
uint8_t sausage_tier(uint16_t bones);

// 第 tier 档每格移动的时间、场上该有几段石头（实际摆出来的在 blocked 里，空间不够时可能少摆）
uint32_t sausage_step_ms(uint8_t tier);
uint8_t sausage_rock_segments(uint8_t tier);

SausageReward sausage_reward(uint16_t bones);

#endif

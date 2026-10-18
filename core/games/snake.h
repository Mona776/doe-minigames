/**
 * 贪吃腊肠 - 纯 C 规则，不依赖 LVGL 和硬件，单元测试直接调用
 *
 * 10×10 网格，四周是墙，一共三关。每关开始先倒计时 3、2、1，Doe 从中间出发、朝右，身体 3 格，每个节拍自动前进一格。
 * 确定键只能顺时针转 90°：同一节拍内的按键先攒着（最多 3 次），到下一次移动时一起算出最终方向——
 * 按一次右转、三次左转；攒出来是掉头就忽略，照原方向走。每次移动后清空。
 * 吃到骨头得 1 分、身体长一格；撞墙、撞障碍或撞到自己立即结束；尾巴这一步刚好离开的格子可以进。
 * 每关吃满 SNAKE_LEVEL_BONES 根过关，停一下进入下一关（身体回到 3 格、重新倒计时）：每关都比上一关快，第 2 关出现石头，第 3 关石头更多。
 * 石头按关卡配置的段数和每段长度，在每关开始时随机摆（横竖随机），整关不变。摆的时候保证公平：
 * 出发那一行不放、两段石头不挨着、所有空地连成一片、每个空格子至少有两条路（没有走进去就出不来的死胡同）。
 * 第 3 关吃满就是通关。暂停时不计时，继续后重新走一个完整节拍。
 * 时间按毫秒推进，同样的种子和输入一定得到同样的结果。
 */

#ifndef SNAKE_H
#define SNAKE_H

#include <stdbool.h>
#include <stdint.h>

// 数值，调手感只改这里
#define SNAKE_SIZE           10
#define SNAKE_CELLS          (SNAKE_SIZE * SNAKE_SIZE)
#define SNAKE_START_LEN      3
#define SNAKE_START_X        5      // 每关开始时头的位置，身体往左排
#define SNAKE_START_Y        4
#define SNAKE_MAX_TURNS      3      // 一个节拍内最多攒这么多次转向，再按不算
#define SNAKE_BONE_MIN_DIST  3      // 新骨头离头至少这么多格（曼哈顿距离），空间不够时放宽

#define SNAKE_COUNTDOWN_MS   3000   // 每关开始倒计时 3、2、1
#define SNAKE_GO_MS          500    // 倒计时后「开始!」停留
#define SNAKE_LEVEL_UP_MS    2000   // 过关后停这么久再进下一关（前一半显示本关完成，后一半显示下一关）

// 关卡：第 i 关每格移动的时间、石头摆几段、每段几块
#define SNAKE_LEVELS         3
#define SNAKE_LEVEL_BONES    5      // 每关吃满这么多根过关
#define SNAKE_STEP_MS_BY_LEVEL       {550, 470, 400}
#define SNAKE_ROCK_SEGMENTS_BY_LEVEL {0, 2, 3}
#define SNAKE_ROCK_LEN_BY_LEVEL      {0, 3, 3}
#define SNAKE_ROCK_TRIES     100    // 每段最多试这么多个随机位置，都不合适就少摆这一段

// 结算：总共吃到这么多根（即通过第 1、2、3 关）分别是 1、2、3 星，0~3 星对应的贝壳，神 +星数
#define SNAKE_STAR_BONES     {5, 10, 15}
#define SNAKE_SHELLS_BY_STARS {5, 10, 20, 30}

// 按顺时针排，转一次就是加一
typedef enum {
    SNAKE_UP,
    SNAKE_RIGHT,
    SNAKE_DOWN,
    SNAKE_LEFT,
} SnakeDir;

typedef enum {
    SNAKE_PHASE_COUNTDOWN,
    SNAKE_PHASE_RUN,
    SNAKE_PHASE_PAUSED,
    SNAKE_PHASE_LEVEL_UP,   // 刚过关，停一下
    SNAKE_PHASE_OVER,
} SnakePhase;

typedef enum {
    SNAKE_END_NONE,
    SNAKE_END_WALL,      // 撞墙
    SNAKE_END_OBSTACLE,  // 撞障碍
    SNAKE_END_SELF,      // 撞到自己
    SNAKE_END_CLEAR,     // 三关全过
} SnakeEnd;

typedef struct {
    uint32_t rng;
    uint32_t elapsed_ms;      // 不含暂停
    SnakePhase phase;
    uint32_t phase_ms;        // 倒计时、过关停顿的进度；走动时是离上一次移动过了多久
    uint8_t level;            // 第几关，从 1 开始
    SnakeDir dir;             // 上一次移动的方向（撞到时是撞过去的方向）
    uint8_t turns;            // 这个节拍攒下的转向次数

    // 身体是环形缓冲：body[head] 是头，往前数 len 个格子是整条身体；格子编号 y * SNAKE_SIZE + x
    uint8_t body[SNAKE_CELLS];
    uint8_t head;
    uint8_t len;
    bool occupied[SNAKE_CELLS];
    bool blocked[SNAKE_CELLS];  // 障碍

    uint8_t bone;             // 骨头所在格
    uint16_t bones;           // 一共吃到的骨头数，就是分数
    uint8_t level_bones;      // 本关吃到几根
    uint16_t moves;           // 走了多少步，界面据此判断要不要重画
    SnakeEnd end;
} SnakeGame;

typedef struct {
    uint8_t stars;
    uint16_t shells;
    uint8_t spirit;
} SnakeReward;

void snake_init(SnakeGame* g, uint32_t seed);

// 倒计时阶段显示的数字：3、2、1，到 0 表示该显示「开始!」
uint8_t snake_countdown(const SnakeGame* g);

// 第 i 节身体所在格，0 是头，len - 1 是尾巴
uint8_t snake_segment(const SnakeGame* g, uint8_t i);

// 按一次确定键：攒一次顺时针转向；不在走动阶段或已经攒满返回 false
bool snake_turn(SnakeGame* g);

// 按现在攒下的转向，下一步会往哪走（攒出掉头时是原方向），给界面画方向箭头
SnakeDir snake_next_dir(const SnakeGame* g);

// 暂停/继续；只在走动和暂停时有效，返回是否切换了
bool snake_toggle_pause(SnakeGame* g);

void snake_step(SnakeGame* g, uint32_t dt_ms);

// 第 level 关每格移动的时间
uint32_t snake_step_ms(uint8_t level);

// 第 level 关配置了几段石头、每段几块（实际摆出来的在 blocked 里，空间不够时可能少摆）
uint8_t snake_rock_segments(uint8_t level);
uint8_t snake_rock_len(uint8_t level);

SnakeReward snake_reward(uint16_t bones);

#endif

/**
 * 界面与逻辑之间的消息
 * 命令：界面 -> 逻辑（玩家操作、小游戏结算）
 * 事件：逻辑 -> 界面（状态变化，由页面订阅）
 * 单线程模拟器是环形缓冲；FreeRTOS 版换成队列。发送都不阻塞
 */

#ifndef APP_MSG_H
#define APP_MSG_H

#include <stdbool.h>
#include <stdint.h>

// 存档按这个编号存最高分，只能往后加
typedef enum {
    GAME_TUG,
    GAME_SNAKE,
    GAME_TENNIS,
    GAME_SAUSAGE,
    GAME_COUNT
} GameId;

// 一局小游戏的结算，奖励由游戏规则算好；中途退出不发
typedef struct {
    uint8_t game;    // GameId
    uint8_t stars;
    uint16_t shells;
    uint8_t spirit;  // 神
    uint16_t score;  // 记最高分用，不比分数的游戏填 0
} GameResult;

typedef enum {
    APP_CMD_GAME_RESULT,  // result 有效
} AppCmdType;

typedef struct {
    AppCmdType type;
    GameResult result;
} AppCmd;

typedef enum {
    APP_EVT_BEST_SCORE,  // game、value 有效：开机读档后逐个发一次，破纪录时再发
} AppEventType;

typedef struct {
    AppEventType type;
    uint8_t game;    // GameId
    int32_t value;
} AppEvent;

#define APP_MSG_WAIT_FOREVER UINT32_MAX

void app_msg_init(void);

bool app_cmd_send(const AppCmd* cmd);
bool app_cmd_receive(AppCmd* cmd);
// 等到一条命令。单线程实现不等待，马上返回有没有
bool app_cmd_wait(AppCmd* cmd, uint32_t timeout_ms);

bool app_event_post(const AppEvent* evt);
bool app_event_receive(AppEvent* evt);

#endif

/**
 * 逻辑服务 - 处理界面发来的命令（小游戏结算记账），保管各小游戏的最高分并存档
 * 以后接 Pico 时放进逻辑任务，和界面之间只通过 app_msg 队列说话
 */

#ifndef GAME_SERVICE_H
#define GAME_SERVICE_H

// 读存档并把最高分发给界面，需在 hal_fs_init 和 app_msg_init 之后调用
void game_service_init(void);

// 周期调用，取出并处理所有命令。单线程模拟器在主循环里调用
void game_service_step(void);

// 逻辑任务：先做 game_service_init，然后一直阻塞等待命令。FreeRTOS 版用这个
void game_service_task(void);

#endif

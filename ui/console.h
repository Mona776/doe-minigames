/**
 * 调试命令台 - 模拟器从终端、Pico 从 USB 串口读入一行命令执行
 * 必须在 LVGL 所在的线程/任务中调用
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>

// 平台专属命令（如模拟器截图），返回 true 表示已处理
typedef bool (*ConsoleExtraHandler)(const char* cmd, const char* args);

void console_set_extra_handler(ConsoleExtraHandler handler, const char* help);

// line 会被修改
void console_exec(char* line);

#endif

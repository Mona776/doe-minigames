/**
 * Mac 主线程和界面任务之间的画面、按键
 * 界面任务画出 240×240，主线程贴到窗口。主线程不调用 FreeRTOS。
 */

#ifndef SIM_HOST_H
#define SIM_HOST_H

#include <stdbool.h>
#include <stdint.h>

#define SIM_SCREEN_SIZE 240

void sim_host_init(void);

void sim_keys_write(uint32_t mask);
uint32_t sim_keys_read(void);

// src 一行 stride_px 个像素。整屏复制到主线程要贴的那块缓冲
void sim_frame_publish(const uint16_t* src, int32_t stride_px);
bool sim_frame_copy(uint16_t* dst);

void sim_request_quit(void);
bool sim_quit_requested(void);

#endif

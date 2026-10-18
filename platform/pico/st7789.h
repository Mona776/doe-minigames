/**
 * ST7789 屏幕驱动（SPI + DMA）
 * 像素按 16 位 SPI 帧发送，LVGL 的 RGB565 缓冲区可以直接交给它，不用交换字节
 */

#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>

#define ST7789_WIDTH  240
#define ST7789_HEIGHT 240

// 在 DMA 中断里调用
typedef void (*St7789DoneCb)(void);

void st7789_init(St7789DoneCb done_cb);

// 把 pixels 写到 (x1,y1)-(x2,y2) 矩形（含边界），立即返回；传输完成后调用 done_cb
// 完成前不能再次调用，也不能改动 pixels
void st7789_write_async(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const uint16_t* pixels, uint32_t count);

#endif

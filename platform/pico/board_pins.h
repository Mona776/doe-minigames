/**
 * Pico 2W 外设引脚分配（接线确定后只改这里）
 */

#ifndef BOARD_PINS_H
#define BOARD_PINS_H

// 物理键：一端接 GPIO、一端接 GND，内部上拉，按下为低电平
#define PIN_KEY_SELECT 10
#define PIN_KEY_OK     11
#define PIN_KEY_BACK   12

// 触摸键：TTP223 之类的触摸模块，输出高电平表示触摸，内部下拉（没接模块时读作未触摸）
#define PIN_TOUCH      13

// ST7789 240x240 屏幕（SPI0），背光 BLK 直接接 3V3 常亮
#define LCD_SPI        spi0
#define LCD_SPI_HZ     (40 * 1000 * 1000)  // 外设时钟 150MHz 只能分出 75 或 37.5MHz，实际为 37.5MHz
#define PIN_LCD_CS     17
#define PIN_LCD_SCK    18
#define PIN_LCD_MOSI   19
#define PIN_LCD_RST    20
#define PIN_LCD_DC     21

#endif

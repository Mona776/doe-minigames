/**
 * ST7789 屏幕驱动
 * 命令和窗口坐标用 8 位阻塞发送（量很小），像素用 16 位帧 + DMA 发送
 */

#include "st7789.h"
#include "board_pins.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

static int s_dma_chan;
static St7789DoneCb s_done_cb;

static void lcd_cmd(uint8_t cmd, const uint8_t* data, size_t len) {
    gpio_put(PIN_LCD_CS, 0);
    gpio_put(PIN_LCD_DC, 0);
    spi_write_blocking(LCD_SPI, &cmd, 1);
    if (len) {
        gpio_put(PIN_LCD_DC, 1);
        spi_write_blocking(LCD_SPI, data, len);
    }
    gpio_put(PIN_LCD_CS, 1);
}

static void lcd_cmd1(uint8_t cmd, uint8_t value) {
    lcd_cmd(cmd, &value, 1);
}

static void set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    uint8_t col[4] = {x1 >> 8, x1 & 0xFF, x2 >> 8, x2 & 0xFF};
    uint8_t row[4] = {y1 >> 8, y1 & 0xFF, y2 >> 8, y2 & 0xFF};
    lcd_cmd(0x2A, col, 4);
    lcd_cmd(0x2B, row, 4);
}

static void dma_irq_handler(void) {
    if (!dma_channel_get_irq1_status(s_dma_chan)) return;
    dma_channel_acknowledge_irq1(s_dma_chan);
    // DMA 完成只代表数据进了 FIFO，要等最后几个像素移出去再拉高片选
    while (spi_is_busy(LCD_SPI)) tight_loop_contents();
    gpio_put(PIN_LCD_CS, 1);
    spi_set_format(LCD_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    if (s_done_cb) s_done_cb();
}

void st7789_init(St7789DoneCb done_cb) {
    s_done_cb = done_cb;

    spi_init(LCD_SPI, LCD_SPI_HZ);
    spi_set_format(LCD_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PIN_LCD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_LCD_MOSI, GPIO_FUNC_SPI);

    const uint pins[] = {PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST};
    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
        gpio_put(pins[i], 1);
    }

    sleep_ms(10);
    gpio_put(PIN_LCD_RST, 0);
    sleep_ms(50);
    gpio_put(PIN_LCD_RST, 1);
    sleep_ms(150);

    lcd_cmd(0x01, NULL, 0);   // 软件复位
    sleep_ms(150);
    lcd_cmd(0x11, NULL, 0);   // 退出睡眠
    sleep_ms(120);
    lcd_cmd1(0x3A, 0x55);     // RGB565
    lcd_cmd1(0x36, 0x00);     // 扫描方向默认，RGB 顺序；240x240 屏此方向无需偏移
    lcd_cmd(0x21, NULL, 0);   // IPS 屏需要反色
    lcd_cmd(0x13, NULL, 0);   // 普通显示模式
    lcd_cmd(0x29, NULL, 0);   // 开显示
    sleep_ms(20);

    s_dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(s_dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_dreq(&cfg, spi_get_dreq(LCD_SPI, true));
    dma_channel_configure(s_dma_chan, &cfg, &spi_get_hw(LCD_SPI)->dr, NULL, 0, false);

    dma_channel_set_irq1_enabled(s_dma_chan, true);
    irq_add_shared_handler(DMA_IRQ_1, dma_irq_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    irq_set_enabled(DMA_IRQ_1, true);
}

void st7789_write_async(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const uint16_t* pixels, uint32_t count) {
    set_window(x1, y1, x2, y2);

    gpio_put(PIN_LCD_CS, 0);
    gpio_put(PIN_LCD_DC, 0);
    uint8_t ramwr = 0x2C;
    spi_write_blocking(LCD_SPI, &ramwr, 1);
    gpio_put(PIN_LCD_DC, 1);

    spi_set_format(LCD_SPI, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    dma_channel_transfer_from_buffer_now(s_dma_chan, pixels, count);
}

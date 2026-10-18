/**
 * Pico 平台主程序（FreeRTOS）
 * 逻辑任务：结算和存档
 * GUI 任务：LVGL、按键、页面、串口命令
 * Console / Heartbeat：串口读入、板载 LED
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lvgl.h"
#include "core/app_msg.h"
#include "core/game_service.h"
#include "core/game_time.h"
#include "hal/hal_audio.h"
#include "hal/hal_clock.h"
#include "hal/hal_fs.h"
#include "hal/hal_input.h"
#include "input/input.h"
#include "ui/console.h"
#include "ui/ui_app.h"
#include "hal_fs_pico.h"
#include "st7789.h"
#include "board_pins.h"

#define SCREEN_WIDTH        ST7789_WIDTH
#define SCREEN_HEIGHT       ST7789_HEIGHT
#define DRAW_BUF_LINES      24
#define GUI_PERIOD_MS       5
#define GUI_STATS_PERIOD_MS 5000
#define FLUSH_TIMEOUT_MS    100
#define CONSOLE_LINE_MAX    96
#define CONSOLE_QUEUE_LEN   4

typedef struct {
    char text[CONSOLE_LINE_MAX];
} ConsoleLine;

static QueueHandle_t s_console_queue;
static uint16_t s_draw_buf[2][SCREEN_WIDTH * DRAW_BUF_LINES] __attribute__((aligned(4)));
static volatile uint32_t s_frame_count;
static TaskHandle_t s_gui_task;

static void heartbeat_task(void* params) {
    (void)params;
    const TickType_t delay = pdMS_TO_TICKS(500);
    while (1) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        vTaskDelay(delay);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        vTaskDelay(delay);
    }
}

static void logic_task(void* params) {
    (void)params;
    printf("[RTOS] 逻辑任务：结算和存档\n");
    game_service_task();
}

static void console_task(void* params) {
    (void)params;
    ConsoleLine line;
    size_t len = 0;
    while (1) {
        int c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        if (c == '\r' || c == '\n') {
            if (len == 0) continue;
            line.text[len] = '\0';
            xQueueSend(s_console_queue, &line, 0);
            len = 0;
        } else if (len < CONSOLE_LINE_MAX - 1) {
            line.text[len++] = (char)c;
        }
    }
}

static uint32_t gui_tick_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

static void gui_flush_done_isr(void) {
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(s_gui_task, &woken);
    portYIELD_FROM_ISR(woken);
}

static void gui_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    if (lv_display_flush_is_last(disp)) s_frame_count++;
    st7789_write_async(area->x1, area->y1, area->x2, area->y2, (const uint16_t*)px_map, lv_area_get_size(area));
}

static void gui_flush_wait_cb(lv_display_t* disp) {
    (void)disp;
    if (ulTaskNotifyTake(pdFALSE, pdMS_TO_TICKS(FLUSH_TIMEOUT_MS)) == 0) printf("[GUI] 屏幕传输超时\n");
}

static void gui_print_stats(uint32_t elapsed_ms, uint32_t busy_us) {
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    float fps = s_frame_count * 1000.0f / elapsed_ms;
    float cpu = busy_us / (elapsed_ms * 10.0f);
    printf("[GUI] 帧率:%.1f fps  CPU:%.1f%%  LVGL堆:%u/%u 已用(峰值%u)  系统堆剩余:%u  GUI栈余量:%u 字\n",
           fps, cpu,
           (unsigned)(mon.total_size - mon.free_size), (unsigned)mon.total_size, (unsigned)mon.max_used,
           (unsigned)xPortGetFreeHeapSize(),
           (unsigned)uxTaskGetStackHighWaterMark(NULL));
    s_frame_count = 0;
}

static bool pico_command(const char* cmd, const char* args) {
    (void)args;
    if (strcmp(cmd, "reboot") == 0) {
        printf("[Sys] 重启\n");
        sleep_ms(50);
        watchdog_reboot(0, 0, 0);
        return true;
    }
    if (strcmp(cmd, "fs") == 0) {
        hal_fs_pico_print_info();
        return true;
    }
    if (strcmp(cmd, "pins") == 0) {
        printf("[Pins] 选择GP%d=%d 确定GP%d=%d 返回GP%d=%d 触摸GP%d=%d（按键 0=按下）\n",
               PIN_KEY_SELECT, gpio_get(PIN_KEY_SELECT), PIN_KEY_OK, gpio_get(PIN_KEY_OK),
               PIN_KEY_BACK, gpio_get(PIN_KEY_BACK), PIN_TOUCH, gpio_get(PIN_TOUCH));
        return true;
    }
    return false;
}

static void gui_task(void* params) {
    (void)params;
    printf("[RTOS] 界面任务：画面和游戏\n");
    lv_init();
    lv_tick_set_cb(gui_tick_ms);

    s_gui_task = xTaskGetCurrentTaskHandle();
    st7789_init(gui_flush_done_isr);

    lv_display_t* disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, s_draw_buf[0], s_draw_buf[1], sizeof(s_draw_buf[0]), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, gui_flush_cb);
    lv_display_set_flush_wait_cb(disp, gui_flush_wait_cb);

    lfs_t* assets = hal_fs_pico_assets();
    if (assets) lv_littlefs_set_handler(assets);

    input_init();
    hal_input_init();
    hal_audio_init();

    int64_t start = 0;
    if (!hal_clock_wall_time(&start)) start = 0;
    game_time_init(start);

    ui_app_init();
    console_set_extra_handler(pico_command,
                              "  fs                         Flash 分区与存档状态\n"
                              "  pins                       按键引脚原始电平\n"
                              "  reboot                     重启\n");

    uint32_t stats_start = gui_tick_ms();
    uint32_t busy_us = 0;
    TickType_t last_wake_time = xTaskGetTickCount();
    ConsoleLine line;

    while (1) {
        uint32_t start_us = time_us_32();

        while (xQueueReceive(s_console_queue, &line, 0) == pdTRUE) console_exec(line.text);
        ui_app_tick(gui_tick_ms());
        lv_timer_handler();

        busy_us += time_us_32() - start_us;

        uint32_t elapsed = gui_tick_ms() - stats_start;
        if (elapsed >= GUI_STATS_PERIOD_MS) {
            gui_print_stats(elapsed, busy_us);
            stats_start += elapsed;
            busy_us = 0;
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(GUI_PERIOD_MS));
    }
}

static void startup_task(void* params) {
    (void)params;
    if (cyw43_arch_init()) {
        printf("无线芯片初始化失败\n");
        vTaskDelete(NULL);
    }

    printf("\n\n");
    printf("╔═══════════════════════════════╗\n");
    printf("║   小游戏模拟器 - Pico 2W     ║\n");
    printf("║   FreeRTOS + LVGL            ║\n");
    printf("║   串口输入 help 查看调试命令 ║\n");
    printf("╚═══════════════════════════════╝\n\n");

    if (!hal_fs_init()) printf("存档分区初始化失败\n");
    app_msg_init();
    s_console_queue = xQueueCreate(CONSOLE_QUEUE_LEN, sizeof(ConsoleLine));

    xTaskCreate(heartbeat_task, "Heartbeat", 512, NULL, 1, NULL);
    xTaskCreate(logic_task, "Logic", 2048, NULL, 2, NULL);
    xTaskCreate(gui_task, "GUI", 8192, NULL, 1, NULL);
    xTaskCreate(console_task, "Console", 512, NULL, 1, NULL);

    printf("✓ FreeRTOS 任务已创建\n\n");
    vTaskDelete(NULL);
}

int main(void) {
    stdio_init_all();
    xTaskCreate(startup_task, "Startup", 2048, NULL, 3, NULL);
    vTaskStartScheduler();
    while (1) tight_loop_contents();
}

/**
 * Mac 模拟器主程序
 * LVGL 自带 SDL 驱动负责窗口，界面代码以后可以和 Pico 固件共用 ui/
 *
 * 启动参数：--speed <倍数>  以指定的时间倍速启动
 * 窗口按键：A 选择  S 确定  D 返回  空格 触摸  K 截图  ESC 退出
 * 终端里可以输入调试命令（每行一条，help 查看）
 */

#include <SDL2/SDL.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "lvgl.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include "core/app_msg.h"
#include "core/game_service.h"
#include "core/game_time.h"
#include "hal/hal_audio.h"
#include "hal/hal_clock.h"
#include "hal/hal_fs.h"
#include "hal/hal_input.h"
#include "input/input.h"
#include "ui/console.h"
#include "ui/crt.h"
#include "ui/ui_app.h"

#define SCREEN_SIZE 240
#define WINDOW_ZOOM 2
#define SCREENSHOT_DIR "screenshots"

static bool s_quit;

static void save_screenshot(const char* path) {
    char default_path[64];
    if (path == NULL || path[0] == '\0') {
        static int shot_index = 0;
        mkdir(SCREENSHOT_DIR, 0755);
        snprintf(default_path, sizeof(default_path), SCREENSHOT_DIR "/shot_%03d.bmp", ++shot_index);
        path = default_path;
    }

    lv_draw_buf_t* buf = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_RGB565);
    if (buf == NULL) {
        lv_image_cache_drop(NULL);
        buf = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_RGB565);
    }
    if (buf == NULL) {
        printf("截图失败\n");
        return;
    }
    lv_area_t area = {0, 0, buf->header.w - 1, buf->header.h - 1};
    crt_apply((uint16_t*)buf->data, (int32_t)(buf->header.stride / 2), &area);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
        buf->data, buf->header.w, buf->header.h, 16, buf->header.stride, SDL_PIXELFORMAT_RGB565);
    if (surface != NULL && SDL_SaveBMP(surface, path) == 0) {
        printf("截图已保存: %s\n", path);
    } else {
        printf("截图失败: %s\n", SDL_GetError());
    }
    SDL_FreeSurface(surface);
    lv_draw_buf_destroy(buf);
}

static bool mac_command(const char* cmd, const char* args) {
    if (strcmp(cmd, "quit") == 0) {
        s_quit = true;
        return true;
    }
    if (strcmp(cmd, "shot") == 0) {
        save_screenshot(args);
        return true;
    }
    if (strcmp(cmd, "mem") == 0) {
        lv_mem_monitor_t mon;
        lv_mem_monitor(&mon);
        printf("[Mem] LVGL 内存 已用 %u / %u 字节（%u%%），最大空闲块 %u，碎片 %u%%\n",
               (unsigned)(mon.total_size - mon.free_size), (unsigned)mon.total_size, (unsigned)mon.used_pct,
               (unsigned)mon.free_biggest_size, (unsigned)mon.frag_pct);
        return true;
    }
    return false;
}

static void poll_console(void) {
    static char buf[256];
    static size_t len = 0;
    static bool closed = false;

    struct pollfd pfd = {.fd = STDIN_FILENO, .events = POLLIN};
    while (!closed && poll(&pfd, 1, 0) > 0) {
        ssize_t n = read(STDIN_FILENO, buf + len, sizeof(buf) - 1 - len);
        if (n <= 0) {
            closed = true;
            break;
        }
        len += (size_t)n;
        buf[len] = '\0';

        char* newline;
        while ((newline = strchr(buf, '\n')) != NULL) {
            *newline = '\0';
            console_exec(buf);
            size_t consumed = (size_t)(newline - buf) + 1;
            memmove(buf, newline + 1, len - consumed + 1);
            len -= consumed;
        }
        if (len == sizeof(buf) - 1) len = 0;
    }
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    long speed = 1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc) {
            speed = strtol(argv[++i], NULL, 10);
        } else {
            printf("未知参数: %s（用法: game_simulator [--speed 倍数]）\n", argv[i]);
            return 1;
        }
    }
    if (speed < 1 || speed > GAME_TIME_MAX_SCALE) {
        printf("倍速应在 1~%d 之间\n", GAME_TIME_MAX_SCALE);
        return 1;
    }

    printf("\n");
    printf("小游戏模拟器\n");
    printf("  [A] 选择  [S] 确定  [D] 返回\n");
    printf("  [空格] 触摸  [K] 截图  [ESC] 退出\n");
    printf("  终端输入 help 查看调试命令\n\n");

    lv_init();
    lv_display_t* disp = lv_sdl_window_create(SCREEN_SIZE, SCREEN_SIZE);
    lv_sdl_window_set_zoom(disp, WINDOW_ZOOM);
    lv_sdl_window_set_title(disp, "小游戏模拟器");

    if (!hal_fs_init()) printf("❌ 存档目录初始化失败\n");
    app_msg_init();
    input_init();
    hal_input_init();
    hal_audio_init();
    game_service_init();

    int64_t start = 0;
    hal_clock_wall_time(&start);
    game_time_init(start);
    if (speed != 1) {
        game_time_set_scale((uint32_t)speed);
        printf("时间倍速 x%ld（游戏里 1 小时 = 现实 %.1f 秒）\n", speed, 3600.0 / (double)speed);
    }

    ui_app_init();
    console_set_extra_handler(mac_command,
                              "  shot [文件.bmp]            截图\n"
                              "  mem                        LVGL 内存占用\n"
                              "  quit                       退出\n");

    bool shot_key_was_down = false;
    while (!s_quit) {
        uint32_t idle_ms = lv_timer_handler();

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_ESCAPE]) break;
        bool shot_down = keys[SDL_SCANCODE_K];
        if (shot_down && !shot_key_was_down) save_screenshot(NULL);
        shot_key_was_down = shot_down;

        ui_app_tick((uint32_t)hal_clock_ms());
        poll_console();
        game_service_step();

        SDL_Delay(idle_ms < 5 ? idle_ms : 5);
    }

    lv_sdl_quit();
    return 0;
}

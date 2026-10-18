/**
 * FreeRTOS 版 Mac 模拟器
 * 界面任务：LVGL、240×240 缓冲、按键、页面里的游戏推进
 * 逻辑任务：读档、结算、最高分，阻塞等界面发来的命令
 * 主线程：只建窗口，把界面任务画出的缓冲贴上去，并采样键盘
 * Mac 上窗口必须建在主线程，所以贴屏留在这里。芯片上把这块缓冲写进屏幕即可。
 * POSIX 移植里任务的 C 栈是 pthread 栈；芯片上要给界面任务单独留够 LVGL 的栈。
 * 用法：--speed <倍数>
 */

#include <SDL2/SDL.h>
#include <pthread.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "lvgl.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sim_host.h"
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

#define WINDOW_ZOOM 2
#define SCREENSHOT_DIR "screenshots"
#define UI_PRIORITY (tskIDLE_PRIORITY + 1)
#define LOGIC_PRIORITY (tskIDLE_PRIORITY + 2)

static long s_speed = 1;
static uint16_t s_draw[SIM_SCREEN_SIZE * SIM_SCREEN_SIZE];

void vAssertCalled(const char* file, int line) {
    fprintf(stderr, "FreeRTOS 断言失败 %s:%d\n", file, line);
    abort();
}

static uint32_t tick_ms(void) {
    return (uint32_t)hal_clock_ms();
}

static void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    (void)area;
    lv_draw_buf_t* buf = lv_display_get_buf_active(disp);
    int32_t stride_px = buf != NULL ? (int32_t)(buf->header.stride / 2) : SIM_SCREEN_SIZE;
    sim_frame_publish((const uint16_t*)px_map, stride_px);
    lv_display_flush_ready(disp);
}

static void save_screenshot(const char* path) {
    char default_path[64];
    if (path == NULL || path[0] == '\0') {
        static int shot_index = 0;
        mkdir(SCREENSHOT_DIR, 0755);
        snprintf(default_path, sizeof(default_path), SCREENSHOT_DIR "/shot_%03d.bmp", ++shot_index);
        path = default_path;
    }

    uint16_t frame[SIM_SCREEN_SIZE * SIM_SCREEN_SIZE];
    if (!sim_frame_copy(frame)) {
        printf("截图失败\n");
        return;
    }
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
        frame, SIM_SCREEN_SIZE, SIM_SCREEN_SIZE, 16, SIM_SCREEN_SIZE * 2, SDL_PIXELFORMAT_RGB565);
    if (surface != NULL && SDL_SaveBMP(surface, path) == 0) {
        printf("截图已保存: %s\n", path);
    } else {
        printf("截图失败: %s\n", SDL_GetError());
    }
    SDL_FreeSurface(surface);
}

static bool mac_command(const char* cmd, const char* args) {
    if (strcmp(cmd, "quit") == 0) {
        sim_request_quit();
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

static void logic_task(void* arg) {
    (void)arg;
    printf("[RTOS] 逻辑任务：结算和存档\n");
    game_service_task();
}

static void ui_task(void* arg) {
    (void)arg;
    printf("[RTOS] 界面任务：画面和游戏\n");

    lv_init();
    lv_tick_set_cb(tick_ms);
    lv_display_t* disp = lv_display_create(SIM_SCREEN_SIZE, SIM_SCREEN_SIZE);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, s_draw, NULL, sizeof(s_draw), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, flush_cb);

    input_init();
    hal_input_init();

    int64_t start = 0;
    hal_clock_wall_time(&start);
    game_time_init(start);
    if (s_speed != 1) {
        game_time_set_scale((uint32_t)s_speed);
        printf("时间倍速 x%ld（游戏里 1 小时 = 现实 %.1f 秒）\n", s_speed, 3600.0 / (double)s_speed);
    }

    ui_app_init();
    console_set_extra_handler(mac_command,
                              "  shot [文件.bmp]            截图\n"
                              "  mem                        LVGL 内存占用\n"
                              "  quit                       退出\n");

    while (!sim_quit_requested()) {
        uint32_t idle_ms = lv_timer_handler();
        ui_app_tick((uint32_t)hal_clock_ms());
        poll_console();

        uint32_t delay_ms = idle_ms < 5 ? idle_ms : 5;
        if (delay_ms == 0) delay_ms = 1;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

static void* scheduler_thread(void* arg) {
    (void)arg;
    xTaskCreate(logic_task, "logic", configMINIMAL_STACK_SIZE, NULL, LOGIC_PRIORITY, NULL);
    xTaskCreate(ui_task, "ui", configMINIMAL_STACK_SIZE, NULL, UI_PRIORITY, NULL);
    vTaskStartScheduler();
    return NULL;
}

static uint32_t sample_keys(const Uint8* keys) {
    uint32_t mask = 0;
    if (keys[SDL_SCANCODE_A])     mask |= INPUT_BIT(INPUT_KEY_SELECT);
    if (keys[SDL_SCANCODE_S])     mask |= INPUT_BIT(INPUT_KEY_OK);
    if (keys[SDL_SCANCODE_D])     mask |= INPUT_BIT(INPUT_KEY_BACK);
    if (keys[SDL_SCANCODE_SPACE]) mask |= INPUT_BIT(INPUT_KEY_TOUCH);
    return mask;
}

static void present_frame(SDL_Texture* tex, SDL_Renderer* renderer) {
    static uint16_t frame[SIM_SCREEN_SIZE * SIM_SCREEN_SIZE];
    if (!sim_frame_copy(frame)) return;
    SDL_UpdateTexture(tex, NULL, frame, SIM_SCREEN_SIZE * 2);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, tex, NULL, NULL);
    SDL_RenderPresent(renderer);
}

static void run_window(void) {
    printf("[RTOS] 主线程：窗口\n");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("窗口初始化失败: %s\n", SDL_GetError());
        return;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_Window* window = SDL_CreateWindow(
        "小游戏模拟器 (FreeRTOS)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SIM_SCREEN_SIZE * WINDOW_ZOOM, SIM_SCREEN_SIZE * WINDOW_ZOOM, 0);
    SDL_Renderer* renderer = window != NULL ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED) : NULL;
    SDL_Texture* tex = renderer != NULL
        ? SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                            SIM_SCREEN_SIZE, SIM_SCREEN_SIZE)
        : NULL;
    if (tex == NULL) {
        printf("窗口创建失败: %s\n", SDL_GetError());
        SDL_Quit();
        return;
    }

    bool shot_key_was_down = false;
    while (!sim_quit_requested()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) sim_request_quit();
        }
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_ESCAPE]) sim_request_quit();
        bool shot_down = keys[SDL_SCANCODE_K];
        if (shot_down && !shot_key_was_down) save_screenshot(NULL);
        shot_key_was_down = shot_down;
        sim_keys_write(sample_keys(keys));
        present_frame(tex, renderer);
        SDL_Delay(5);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc) {
            s_speed = strtol(argv[++i], NULL, 10);
        } else {
            printf("未知参数: %s（用法: game_simulator_freertos [--speed 倍数]）\n", argv[i]);
            return 1;
        }
    }
    if (s_speed < 1 || s_speed > GAME_TIME_MAX_SCALE) {
        printf("倍速应在 1~%d 之间\n", GAME_TIME_MAX_SCALE);
        return 1;
    }

    printf("\n");
    printf("小游戏模拟器（FreeRTOS）\n");
    printf("  [A] 选择  [S] 确定  [D] 返回\n");
    printf("  [空格] 触摸  [K] 截图  [ESC] 退出\n");
    printf("  终端输入 help 查看调试命令\n\n");

    sim_host_init();
    if (!hal_fs_init()) printf("❌ 存档目录初始化失败\n");
    hal_audio_init();
    app_msg_init();

    pthread_t scheduler;
    pthread_create(&scheduler, NULL, scheduler_thread, NULL);
    run_window();
    exit(0);
}

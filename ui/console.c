/**
 * 调试命令台
 * 模拟器从终端读入一行命令执行，必须在 LVGL 所在的线程中调用
 */

#include "console.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/game_time.h"
#include "input/input.h"
#include "input/input_config.h"
#include "ui/crt.h"
#include "ui/page_mgr.h"

#define KEY_TAP_MS  100
#define KEY_LONG_MS (INPUT_LONG_PRESS_MS + 300)

static ConsoleExtraHandler s_extra;
static const char* s_extra_help;

void console_set_extra_handler(ConsoleExtraHandler handler, const char* help) {
    s_extra = handler;
    s_extra_help = help;
}

static void print_time(void) {
    char buf[20];
    game_time_format(game_time_now(), buf, sizeof(buf));
    printf("[Time] %s 倍速 x%u%s\n", buf, (unsigned)game_time_get_scale(),
           game_time_is_frozen() ? " (已冻结)" : "");
}

static void print_mem(void) {
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    printf("[Mem] LVGL 内存池 %u KB，已用 %u KB（%u%%），最大可用块 %u KB，碎片 %u%%\n",
           (unsigned)(mon.total_size / 1024), (unsigned)((mon.total_size - mon.free_size) / 1024),
           (unsigned)mon.used_pct, (unsigned)(mon.free_biggest_size / 1024), (unsigned)mon.frag_pct);
}

static void print_help(void) {
    printf("命令:\n"
           "  key <键>[+<键>] [long|毫秒]  模拟按键，键: select ok back touch\n"
           "  page                       显示页面栈\n"
           "  mem                        显示 LVGL 内存池用量\n"
           "  speed <倍数>               时间倍速，1 为真实速度\n"
           "  skip <时长>                快进游戏时间，如 90s 30m 2h 1d\n"
           "  freeze                     冻结/恢复游戏时间\n"
           "  time                       显示游戏时间\n"
           "  settime YYYY-MM-DD HH:MM[:SS]\n"
           "  crt [on|off]               CRT 扫描线开关（机壳一直在）\n");
    if (s_extra_help) printf("%s", s_extra_help);
}

static bool parse_key(const char* name, uint32_t* mask) {
    for (int k = 0; k < INPUT_KEY_COUNT; k++) {
        if (strcmp(name, input_key_name((InputKey)k)) == 0) {
            *mask |= INPUT_BIT(k);
            return true;
        }
    }
    return false;
}

static void cmd_key(char* args) {
    char* save = NULL;
    char* keys = strtok_r(args, " ", &save);
    char* hold = strtok_r(NULL, " ", &save);
    if (keys == NULL) {
        printf("用法: key select | key ok+back | key select long | key touch 3500\n");
        return;
    }

    uint32_t mask = 0;
    char* ksave = NULL;
    for (char* k = strtok_r(keys, "+", &ksave); k; k = strtok_r(NULL, "+", &ksave)) {
        if (!parse_key(k, &mask)) {
            printf("未知按键: %s\n", k);
            return;
        }
    }

    uint32_t hold_ms = KEY_TAP_MS;
    if (hold) hold_ms = strcmp(hold, "long") == 0 ? KEY_LONG_MS : (uint32_t)strtoul(hold, NULL, 10);
    input_inject(mask, hold_ms);
}

static long long parse_duration(const char* args) {
    char* end = NULL;
    long long n = strtoll(args, &end, 10);
    if (end == args || n <= 0) return 0;
    switch (*end) {
        case 'm': return n * 60;
        case 'h': return n * 3600;
        case 'd': return n * 86400;
        default:  return n;
    }
}

static void cmd_skip(const char* args) {
    long long n = parse_duration(args);
    if (n == 0) {
        printf("用法: skip 90s | skip 30m | skip 2h | skip 1d\n");
        return;
    }
    game_time_skip(n);
    print_time();
}

static void cmd_settime(const char* args) {
    GameDateTime dt = {0};
    int n = sscanf(args, "%d-%d-%d %d:%d:%d", &dt.year, &dt.month, &dt.day, &dt.hour, &dt.minute, &dt.second);
    if (n < 5 || dt.month < 1 || dt.month > 12 || dt.day < 1 || dt.day > 31 ||
        dt.hour > 23 || dt.minute > 59 || dt.second > 59) {
        printf("用法: settime 2026-09-29 21:30[:00]\n");
        return;
    }
    game_time_set(game_time_from_datetime(&dt));
    print_time();
}

void console_exec(char* line) {
    size_t len = strlen(line);
    while (len > 0 && isspace((unsigned char)line[len - 1])) line[--len] = '\0';
    while (*line && isspace((unsigned char)*line)) line++;
    if (*line == '\0') return;

    char* args = line;
    while (*args && !isspace((unsigned char)*args)) args++;
    if (*args) *args++ = '\0';
    while (*args && isspace((unsigned char)*args)) args++;
    const char* cmd = line;

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) print_help();
    else if (strcmp(cmd, "key") == 0) cmd_key(args);
    else if (strcmp(cmd, "page") == 0) page_mgr_print_stack();
    else if (strcmp(cmd, "mem") == 0) print_mem();
    else if (strcmp(cmd, "speed") == 0) {
        long scale = strtol(args, NULL, 10);
        if (scale < 1 || scale > GAME_TIME_MAX_SCALE) {
            printf("用法: speed 1~%d\n", GAME_TIME_MAX_SCALE);
        } else {
            game_time_set_scale((uint32_t)scale);
            print_time();
        }
    }
    else if (strcmp(cmd, "skip") == 0) cmd_skip(args);
    else if (strcmp(cmd, "freeze") == 0) {
        game_time_set_frozen(!game_time_is_frozen());
        print_time();
    }
    else if (strcmp(cmd, "time") == 0) print_time();
    else if (strcmp(cmd, "settime") == 0) cmd_settime(args);
    else if (strcmp(cmd, "crt") == 0) {
        if (strcmp(args, "on") == 0) crt_set_scanlines(true);
        else if (strcmp(args, "off") == 0) crt_set_scanlines(false);
        printf("[CRT] 扫描线 %s\n", crt_scanlines_enabled() ? "开" : "关");
    }
    else if (s_extra && s_extra(cmd, args)) {}
    else printf("未知命令: %s（输入 help 查看）\n", cmd);
}

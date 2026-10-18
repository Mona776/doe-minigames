/**
 * 游戏时间 - 虚拟时钟
 * 所有玩法计时都以它为准：可倍速、可快进、可冻结，模拟器和真机行为一致
 * 时间值为本地时间从 1970-01-01 00:00:00 起的秒数，不做时区换算
 * 线程安全，可在任意任务中调用
 */

#ifndef GAME_TIME_H
#define GAME_TIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GAME_TIME_MAX_SCALE 86400

typedef struct {
    int32_t year;
    int32_t month;    // 1-12
    int32_t day;      // 1-31
    int32_t hour;
    int32_t minute;
    int32_t second;
    int32_t weekday;  // 0=星期日
} GameDateTime;

void game_time_init(int64_t epoch);

int64_t game_time_now(void);
int64_t game_time_now_ms(void);

void game_time_set(int64_t epoch);
void game_time_skip(int64_t seconds);

// 1 表示真实速度，60 表示 1 秒走 1 分钟
void game_time_set_scale(uint32_t scale);
uint32_t game_time_get_scale(void);

// 冻结期间时间不走（真关机、低电锁存等场景使用）
void game_time_set_frozen(bool frozen);
bool game_time_is_frozen(void);

void game_time_to_datetime(int64_t epoch, GameDateTime* out);
int64_t game_time_from_datetime(const GameDateTime* dt);

// 格式化为 "2026-09-29 23:05:00"，buf 至少 20 字节
void game_time_format(int64_t epoch, char* buf, size_t size);

#endif

/**
 * 游戏时间 - 虚拟时钟
 * 记录一个锚点（真实时间, 游戏时间），当前游戏时间 = 锚点游戏时间 + 真实流逝 × 倍速
 */

#include "game_time.h"
#include <stdio.h>
#include "hal/hal_clock.h"
#include "hal/hal_sys.h"

static struct {
    uint64_t anchor_real_ms;
    int64_t anchor_game_ms;
    uint32_t scale;
    bool frozen;
} s_clock = {0, 0, 1, false};

static int64_t now_ms_locked(void) {
    if (s_clock.frozen) return s_clock.anchor_game_ms;
    return s_clock.anchor_game_ms + (int64_t)(hal_clock_ms() - s_clock.anchor_real_ms) * s_clock.scale;
}

static void reanchor_locked(int64_t game_ms) {
    s_clock.anchor_real_ms = hal_clock_ms();
    s_clock.anchor_game_ms = game_ms;
}

void game_time_init(int64_t epoch) {
    hal_critical_enter();
    reanchor_locked(epoch * 1000);
    s_clock.scale = 1;
    s_clock.frozen = false;
    hal_critical_exit();
}

int64_t game_time_now_ms(void) {
    hal_critical_enter();
    int64_t ms = now_ms_locked();
    hal_critical_exit();
    return ms;
}

int64_t game_time_now(void) {
    return game_time_now_ms() / 1000;
}

void game_time_set(int64_t epoch) {
    hal_critical_enter();
    reanchor_locked(epoch * 1000);
    hal_critical_exit();
}

void game_time_skip(int64_t seconds) {
    hal_critical_enter();
    reanchor_locked(now_ms_locked() + seconds * 1000);
    hal_critical_exit();
}

void game_time_set_scale(uint32_t scale) {
    if (scale < 1) scale = 1;
    if (scale > GAME_TIME_MAX_SCALE) scale = GAME_TIME_MAX_SCALE;
    hal_critical_enter();
    reanchor_locked(now_ms_locked());
    s_clock.scale = scale;
    hal_critical_exit();
}

uint32_t game_time_get_scale(void) {
    return s_clock.scale;
}

void game_time_set_frozen(bool frozen) {
    hal_critical_enter();
    reanchor_locked(now_ms_locked());
    s_clock.frozen = frozen;
    hal_critical_exit();
}

bool game_time_is_frozen(void) {
    return s_clock.frozen;
}

// 公历日期与天数互转（Howard Hinnant 的 civil_from_days 算法）
static int64_t days_from_civil(int64_t y, int32_t m, int32_t d) {
    y -= m <= 2;
    int64_t era = (y >= 0 ? y : y - 399) / 400;
    int64_t yoe = y - era * 400;
    int64_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

void game_time_to_datetime(int64_t epoch, GameDateTime* out) {
    int64_t days = epoch >= 0 ? epoch / 86400 : (epoch - 86399) / 86400;
    int64_t secs = epoch - days * 86400;

    int64_t z = days + 719468;
    int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    int64_t doe = z - era * 146097;
    int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    int64_t mp = (5 * doy + 2) / 153;
    int32_t d = (int32_t)(doy - (153 * mp + 2) / 5 + 1);
    int32_t m = (int32_t)(mp < 10 ? mp + 3 : mp - 9);

    out->year = (int32_t)(yoe + era * 400 + (m <= 2));
    out->month = m;
    out->day = d;
    out->hour = (int32_t)(secs / 3600);
    out->minute = (int32_t)(secs / 60 % 60);
    out->second = (int32_t)(secs % 60);
    out->weekday = (int32_t)((days % 7 + 11) % 7);  // 1970-01-01 是星期四
}

int64_t game_time_from_datetime(const GameDateTime* dt) {
    return days_from_civil(dt->year, dt->month, dt->day) * 86400 +
           dt->hour * 3600 + dt->minute * 60 + dt->second;
}

void game_time_format(int64_t epoch, char* buf, size_t size) {
    GameDateTime dt;
    game_time_to_datetime(epoch, &dt);
    snprintf(buf, size, "%04d-%02d-%02d %02d:%02d:%02d",
             (int)dt.year, (int)dt.month, (int)dt.day, (int)dt.hour, (int)dt.minute, (int)dt.second);
}

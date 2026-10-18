/**
 * 蜂鸣器音效和背景乐
 */

#include "ui_sfx.h"
#include "lvgl.h"
#include "hal/hal_audio.h"

#define TICK_MS    2
#define FX_STEP_MS 8  // 滑音、噪声每隔这么久换一次频率
#define MUSIC_GAP_MS 20  // 背景乐每个音末尾留这么久不响，连着的同音才分得开
#define MUSIC_MIN_MS 25  // 音效让出蜂鸣器时，背景乐当前音剩下不到这么久就不补了，免得「哔」一下
#define MUSIC_VOLUME_PERCENT 25

static lv_timer_t* s_timer;
static const UiNote* s_notes;
static uint8_t s_count;
static uint8_t s_next;
static const UiNote* s_cur;
static uint32_t s_cur_at;
static uint32_t s_cur_end;
static uint32_t s_fx_at;
static uint32_t s_rng = 0x2545F491u;

static const UiNote* s_music;
static uint16_t s_music_count;
static uint16_t s_music_i;
static uint32_t s_music_at;   // 当前音开始的时刻
static uint32_t s_music_len;  // 当前音按速度换算后的时长
static uint16_t s_music_tempo = 100;
static bool s_music_paused;
static uint32_t s_music_paused_at;
static bool s_music_on;       // 当前音已经交给蜂鸣器了

static uint16_t fx_hz(const UiNote* n, uint32_t elapsed) {
    if (n->noise) {
        s_rng ^= s_rng << 13;
        s_rng ^= s_rng >> 17;
        s_rng ^= s_rng << 5;
        uint32_t span = n->to_hz > n->hz ? n->to_hz - n->hz : 0;
        return (uint16_t)(n->hz + s_rng % (span + 1));
    }
    return (uint16_t)((int32_t)n->hz + ((int32_t)n->to_hz - (int32_t)n->hz) * (int32_t)elapsed / (int32_t)n->ms);
}

static void start_note(uint32_t now) {
    s_cur = &s_notes[s_next++];
    s_cur_at = now;
    s_cur_end = now + s_cur->ms;
    s_fx_at = now + FX_STEP_MS;
    hal_audio_tone(s_cur->to_hz ? fx_hz(s_cur, 0) : s_cur->hz, s_cur->ms);
}

static bool music_running(void) {
    return s_music && !s_music_paused;
}

static uint32_t music_len(uint16_t i) {
    return (uint32_t)s_music[i].ms * 100 / s_music_tempo;
}

// 按时间走到该放的那个音；蜂鸣器空着、这个音还没放过就放它剩下的部分
static void music_tick(uint32_t now) {
    if (!music_running()) return;
    while ((int32_t)(now - (s_music_at + s_music_len)) >= 0) {
        s_music_at += s_music_len;
        s_music_i = (uint16_t)((s_music_i + 1) % s_music_count);
        s_music_len = music_len(s_music_i);
        s_music_on = false;
    }
    if (s_cur || s_music_on) return;
    const UiNote* n = &s_music[s_music_i];
    int32_t left = (int32_t)(s_music_at + s_music_len - MUSIC_GAP_MS - now);
    if (n->hz == 0 || left < MUSIC_MIN_MS) return;
    hal_audio_tone_volume(n->hz, (uint16_t)left, MUSIC_VOLUME_PERCENT);
    s_music_on = true;
}

static void tick(lv_timer_t* timer) {
    uint32_t now = lv_tick_get();
    if (s_cur && (int32_t)(now - s_cur_end) >= 0) {
        if (s_next < s_count) {
            start_note(now);
        } else {
            s_cur = NULL;
        }
    } else if (s_cur && s_cur->to_hz && (int32_t)(now - s_fx_at) >= 0) {
        hal_audio_tone(fx_hz(s_cur, now - s_cur_at), (uint16_t)(s_cur_end - now));
        s_fx_at = now + FX_STEP_MS;
    }
    music_tick(now);
    if (!s_cur && !music_running()) lv_timer_pause(timer);
}

static void wake(void) {
    if (s_timer == NULL) s_timer = lv_timer_create(tick, TICK_MS, NULL);
    lv_timer_resume(s_timer);
}

void ui_sfx_play(const UiNote* notes, uint8_t count) {
    if (count == 0) return;
    s_notes = notes;
    s_count = count;
    s_next = 0;
    s_music_on = false;
    start_note(lv_tick_get());
    wake();
}

// 背景乐正占着蜂鸣器就让它停下
static void music_silence(void) {
    if (!s_cur && s_music_on) hal_audio_tone(0, 0);
    s_music_on = false;
}

void ui_music_play(const UiNote* notes, uint16_t count) {
    if (count == 0) return;
    music_silence();
    s_music = notes;
    s_music_count = count;
    s_music_i = 0;
    s_music_paused = false;
    s_music_at = lv_tick_get();
    s_music_len = music_len(0);
    wake();
}

void ui_music_stop(void) {
    if (!s_music) return;
    music_silence();
    s_music = NULL;
}

void ui_music_pause(bool paused) {
    if (!s_music || paused == s_music_paused) return;
    uint32_t now = lv_tick_get();
    if (paused) {
        music_silence();
        s_music_paused_at = now;
    } else {
        s_music_at += now - s_music_paused_at;
        wake();
    }
    s_music_paused = paused;
}

void ui_music_tempo(uint16_t percent) {
    if (percent) s_music_tempo = percent;
}

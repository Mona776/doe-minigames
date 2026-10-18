/**
 * 蜂鸣器音效 - Mac 模拟器实现：SDL 音频回调里生成方波，听起来接近无源蜂鸣器
 * 环境变量 SIM_NO_AUDIO=1 时静音（脚本跑截图时用）
 */

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "hal/hal_audio.h"

#define SAMPLE_RATE 44100
#define VOLUME      3000  // 方波幅度，满幅 32767 太刺耳
#define FADE_MS     4     // 结尾淡出，避免咔哒声

static SDL_AudioDeviceID s_dev;

// 以下只在锁住音频设备时访问
static double s_step;        // 每个采样前进多少个周期
static double s_phase;
static uint32_t s_remaining; // 还剩多少个采样
static int32_t s_amplitude = VOLUME;

static void fill(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    int16_t* out = (int16_t*)stream;
    int n = len / (int)sizeof(int16_t);
    const uint32_t fade = SAMPLE_RATE * FADE_MS / 1000;
    for (int i = 0; i < n; i++) {
        if (s_remaining == 0) {
            out[i] = 0;
            continue;
        }
        int32_t amp = s_remaining < fade ? s_amplitude * (int32_t)s_remaining / (int32_t)fade : s_amplitude;
        out[i] = (int16_t)(s_phase < 0.5 ? amp : -amp);
        s_phase += s_step;
        if (s_phase >= 1.0) s_phase -= 1.0;
        s_remaining--;
    }
}

void hal_audio_init(void) {
    const char* mute = getenv("SIM_NO_AUDIO");
    if (mute && mute[0] && mute[0] != '0') return;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        printf("❌ 音频初始化失败: %s\n", SDL_GetError());
        return;
    }
    SDL_AudioSpec want = {0};
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = fill;
    s_dev = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
    if (s_dev == 0) {
        printf("❌ 打开音频设备失败: %s\n", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(s_dev, 0);
}

void hal_audio_tone(uint16_t freq_hz, uint16_t duration_ms) {
    hal_audio_tone_volume(freq_hz, duration_ms, 100);
}

void hal_audio_tone_volume(uint16_t freq_hz, uint16_t duration_ms, uint8_t volume_percent) {
    if (s_dev == 0) return;
    SDL_LockAudioDevice(s_dev);
    s_amplitude = VOLUME * (volume_percent > 100 ? 100 : volume_percent) / 100;
    s_step = (double)freq_hz / SAMPLE_RATE;
    // 正在响时接着原来的相位换频率，滑音才不会一顿一顿地咔哒
    if (s_remaining == 0) s_phase = 0;
    s_remaining = freq_hz ? (uint32_t)SAMPLE_RATE * duration_ms / 1000 : 0;
    SDL_UnlockAudioDevice(s_dev);
}

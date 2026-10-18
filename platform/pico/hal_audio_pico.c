/**
 * 蜂鸣器 - Pico 暂未接线，先空实现，保证固件能跑
 */

#include "hal/hal_audio.h"

void hal_audio_init(void) {}

void hal_audio_tone(uint16_t freq_hz, uint16_t duration_ms) {
    (void)freq_hz;
    (void)duration_ms;
}

void hal_audio_tone_volume(uint16_t freq_hz, uint16_t duration_ms, uint8_t volume_percent) {
    (void)freq_hz;
    (void)duration_ms;
    (void)volume_percent;
}

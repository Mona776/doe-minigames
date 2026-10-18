/**
 * 蜂鸣器音效 - 各平台实现
 * Mac: SDL 音频输出方波；Pico: PWM 驱动无源蜂鸣器
 * 同一时间只响一个音，新的音直接打断正在响的
 * 只能在 LVGL 所在的线程/任务中调用
 */

#ifndef HAL_AUDIO_H
#define HAL_AUDIO_H

#include <stdint.h>

void hal_audio_init(void);

// 响 freq_hz 赫兹的音 duration_ms 毫秒，不阻塞；freq_hz 为 0 表示停止
void hal_audio_tone(uint16_t freq_hz, uint16_t duration_ms);

// 指定本次音符的音量百分比（0–100），不影响之后的普通音效。
void hal_audio_tone_volume(uint16_t freq_hz, uint16_t duration_ms, uint8_t volume_percent);

#endif

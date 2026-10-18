/**
 * 蜂鸣器音效 - 按顺序响一串音（一个音也行），新的音效直接打断正在响的
 * 蜂鸣器同一时间只能响一个音，这里用一个 LVGL 定时器到点换下一个音；
 * 滑音和噪声也是每隔一小段时间换一次频率做出来的，复古街机的音效基本靠这两样
 * 音符数组要一直有效（用 static const），只在 LVGL 所在的线程/任务中使用
 * 背景乐：一串音循环放；音效响的时候让出蜂鸣器，节拍照走，音效完了接着当前那个音放
 */

#ifndef UI_SFX_H
#define UI_SFX_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t hz;     // 0 是休止
    uint16_t ms;
    uint16_t to_hz;  // 非 0 时从 hz 滑到 to_hz；noise 时在 hz~to_hz 之间乱跳，听起来像爆炸、撞击
    bool noise;
} UiNote;

void ui_sfx_play(const UiNote* notes, uint8_t count);

#define UI_SFX_PLAY(notes) ui_sfx_play((notes), (uint8_t)(sizeof(notes) / sizeof((notes)[0])))

// 从头循环播放背景乐，替换正在放的；只用 hz 和 ms
void ui_music_play(const UiNote* notes, uint16_t count);
void ui_music_stop(void);
// 暂停时停在当前位置，继续后从那里接着放；没在放时无效
void ui_music_pause(bool paused);
// 播放速度，100 是原速，越大越快；从下一个音开始生效
void ui_music_tempo(uint16_t percent);

#define UI_MUSIC_PLAY(notes) ui_music_play((notes), (uint16_t)(sizeof(notes) / sizeof((notes)[0])))

#endif

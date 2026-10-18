/**
 * 存档 - 文件头（魔数、版本、长度、CRC32）+ 数据，目前只存各小游戏的最高分
 * 先写临时文件再改名，断电不会写坏旧存档
 * 修改 SaveData 后把 SAVE_VERSION 加一，旧存档会被忽略
 */

#ifndef SAVE_H
#define SAVE_H

#include <stdbool.h>
#include <stdint.h>

#define SAVE_VERSION    1
#define SAVE_GAME_SLOTS 8  // 按 GameId 存，留出空位，加游戏不用改存档版本

typedef struct {
    uint16_t best_score[SAVE_GAME_SLOTS];
} SaveData;

// 文件不存在、校验失败或版本不符都返回 false
bool save_load(SaveData* out);
bool save_write(const SaveData* data);

#endif

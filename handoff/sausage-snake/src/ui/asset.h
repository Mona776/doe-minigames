/**
 * 图标查找。目前没有素材分区，找不到就返回空，按钮和标题照常显示文字。
 */

#ifndef ASSET_H
#define ASSET_H

#include <stdint.h>

#define ASSET_NAME_MAX 32

typedef struct {
    const void** frames;
    uint16_t width;
    uint16_t height;
} AssetAnim;

const AssetAnim* asset_anim_find(const char* name);

#endif

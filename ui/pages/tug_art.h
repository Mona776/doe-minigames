/**
 * 拔河的占位像素画；换成 Doe 正式素材时整份替换
 */

#ifndef TUG_ART_H
#define TUG_ART_H

#include "ui/sprite.h"

#define TUG_DOE_W     20
#define TUG_DOE_H     16
#define TUG_DOE_MOUTH_X 19  // 绳子咬在嘴上的像素位置，前三张 Doe 图都一样
#define TUG_DOE_MOUTH_Y 8
#define TUG_DOE_SLIP_DROP 2 // 滑倒图的嘴比其他图低这么多行
#define TUG_TOY_SIZE  8

extern const Sprite tug_doe_stand;
extern const Sprite tug_doe_pull;   // 后仰、前脚撑地
extern const Sprite tug_doe_guard;  // 压低重心、四脚张开
extern const Sprite tug_doe_slip;   // 用力过猛滑倒，趴在地上
extern const Sprite tug_toy;

#endif

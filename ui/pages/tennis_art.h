/**
 * 接网球的占位像素画；换成正式素材时整份替换
 * Doe 画成朝右，嘴在第 TENNIS_DOE_MOUTH_X 列、第 TENNIS_DOE_MOUTH_Y 行附近；
 * 发球机炮口画成朝左，球从第 TENNIS_MACHINE_NOZZLE_Y 行出来。页面里两者都镜像，面对面
 */

#ifndef TENNIS_ART_H
#define TENNIS_ART_H

#include "ui/sprite.h"

#define TENNIS_DOE_W           20
#define TENNIS_DOE_H           16
#define TENNIS_DOE_MOUTH_X     19
#define TENNIS_DOE_MOUTH_Y     8
#define TENNIS_MACHINE_W       18
#define TENNIS_MACHINE_H       16
#define TENNIS_MACHINE_NOZZLE_Y 6
#define TENNIS_BALL            8
#define TENNIS_HEART_W         7
#define TENNIS_HEART_H         6
#define TENNIS_MOUNTAINS_W     32
#define TENNIS_MOUNTAINS_H     8

extern const Sprite tennis_doe_stand;
extern const Sprite tennis_doe_crouch;    // 起跳前蹲一下、落地缓冲
extern const Sprite tennis_doe_jump;      // 空中，四条腿伸开
extern const Sprite tennis_doe_bite;      // 空中张嘴
extern const Sprite tennis_machine_idle;
extern const Sprite tennis_machine_ready; // 灯亮
extern const Sprite tennis_ball;
extern const Sprite tennis_heart_full;
extern const Sprite tennis_heart_empty;
extern const Sprite tennis_mountains;     // 地平线上的远山

#endif

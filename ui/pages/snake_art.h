/**
 * 贪吃腊肠的占位像素画：侧面看的腊肠狗，按格子拼接；换成正式素材时整份替换
 * 头、尾巴、脚都画成朝右、肚子朝下，其他方向由页面旋转或镜像得到；
 * 身体在格子里占第 2~7 行（列），上下对称，直身体和转角直接旋转就能接上
 * 脚单独一层：前脚挂在头下面、后脚挂在尾巴下面，身体越长两对脚离得越远；两帧交替就是走路
 */

#ifndef SNAKE_ART_H
#define SNAKE_ART_H

#include "ui/sprite.h"

#define SNAKE_TILE      10  // 一格的像素数（放大前）
#define SNAKE_LEGS      12  // 脚那一层比格子四周各多 1 像素，脚能伸出身体下沿
#define SNAKE_ARROW     5
#define SNAKE_STAR      3
#define SNAKE_ICON      8   // 骨头、贝壳图标
#define SNAKE_STAR_ICON 9   // 结算的星位
#define SNAKE_TURN_ICON 9

extern const Sprite snake_head;     // 朝右，脖子接在左边
extern const Sprite snake_body;     // 横向直身体
extern const Sprite snake_corner;   // 转角，接左边和下边
extern const Sprite snake_tail;     // 朝右的狗的屁股和尾巴，身体接在右边
extern const Sprite snake_front_legs[2];  // 两帧：站直、迈开
extern const Sprite snake_back_legs[2];
extern const Sprite snake_bone;
extern const Sprite snake_rock;     // 障碍
extern const Sprite snake_arrow;    // 下一步方向，朝右
extern const Sprite snake_star;     // 撞晕时绕着头转的小星星

// 计分和结算用的小图标：骨头、贝壳 SNAKE_ICON 见方，星位 SNAKE_STAR_ICON 见方
extern const Sprite snake_icon_bone;
extern const Sprite snake_icon_shell;
extern const Sprite snake_icon_star[2];  // 没得到、得到

// 玩法说明的转向示意，SNAKE_TURN_ICON 见方
extern const Sprite snake_turn_right;
extern const Sprite snake_turn_left;
extern const Sprite snake_go_straight;

#endif

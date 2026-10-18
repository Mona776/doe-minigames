/**
 * 接网球的占位像素画
 */

#include "tennis_art.h"

static const char* const DOE_STAND[] = {
    "....................",
    "..........KK...KK...",
    ".........KBBK.KBBK..",
    ".........KBBKKKBBK..",
    "..........KWWWWWWK..",
    ".........KWWWWKWWWK.",
    ".........KWWWWWWWWKK",
    ".K.......KWWWWWWWPPK",
    "KWK......KWWWWWWWKK.",
    ".KWKKKKKKKWWWWWWWK..",
    "..KWWWWWWWWWWWWWK...",
    "..KWWWWWWWWBBWWWK...",
    "..KWWWWWWWWBBWWWK...",
    "..KWWKKKKKKKKWWK....",
    "..KWK.......KWK.....",
    "..KK........KK......",
};

static const char* const DOE_CROUCH[] = {
    "....................",
    "....................",
    "..........KK...KK...",
    ".........KBBK.KBBK..",
    ".........KBBKKKBBK..",
    "..........KWWWWWWK..",
    ".........KWWWWKWWWK.",
    ".........KWWWWWWWWKK",
    ".K.......KWWWWWWWPPK",
    "KWK......KWWWWWWWKK.",
    ".KWKKKKKKKWWWWWWWK..",
    "..KWWWWWWWWWWWWWK...",
    "..KWWWWWWWWBBWWWK...",
    "..KWWWWWWWWBBWWWK...",
    ".KWWKKKKKKKKKWWWK...",
    ".KKK........KKKK....",
};

static const char* const DOE_JUMP[] = {
    "....................",
    "..........KK...KK...",
    ".........KBBK.KBBK..",
    ".........KBBKKKBBK..",
    "..........KWWWWWWK..",
    ".........KWWWWKWWWK.",
    ".........KWWWWWWWWKK",
    "KK.......KWWWWWWWPPK",
    "KWK......KWWWWWWWKK.",
    ".KWKKKKKKKWWWWWWWK..",
    "..KWWWWWWWWWWWWWK...",
    "..KWWWWWWWWBBWWWK...",
    ".KWWWWWWWWWBBWWWWK..",
    "KWWKKKKKKKKKKKKWWK..",
    "KK.............KWK..",
    "................KK..",
};

static const char* const DOE_BITE[] = {
    "....................",
    "..........KK...KK...",
    ".........KBBK.KBBK..",
    ".........KBBKKKBBK..",
    "..........KWWWWWWK..",
    ".........KWWWWKWWWKK",
    ".........KWWWWWWWWPK",
    "KK.......KWWWWWWKKK.",
    "KWK......KWWWWWKRR..",
    ".KWKKKKKKKWWWWWWKKK.",
    "..KWWWWWWWWWWWWWWK..",
    "..KWWWWWWWWBBWWWK...",
    ".KWWWWWWWWWBBWWWWK..",
    "KWWKKKKKKKKKKKKWWK..",
    "KK.............KWK..",
    "................KK..",
};

// 炮口在左边第 6 行，顶上是装球的斗，左上角是指示灯
static const char* const MACHINE_IDLE[] = {
    "..........KKKKK...",
    ".........KLLLLLK..",
    ".........KLLLLLK..",
    ".....KK...KDDDK...",
    "....KDDKKKKKKKKKK.",
    "KKKKKGGGGGGGGGGGK.",
    "DDDDDGGGGGGGGGGGK.",
    "KKKKKGGGGGGGGGGGK.",
    "....KGGGGGGGGGGGK.",
    "....KGGKKKKKKKGGK.",
    "....KGGGGGGGGGGGK.",
    "....KKKKKKKKKKKKK.",
    ".....KK.....KK....",
    "....KDDK...KDDK...",
    "....KDDK...KDDK...",
    ".....KK.....KK....",
};

static const char* const MACHINE_READY[] = {
    "..........KKKKK...",
    ".........KLLLLLK..",
    ".........KLLLLLK..",
    ".....KK...KDDDK...",
    "....KRRKKKKKKKKKK.",
    "KKKKKGGGGGGGGGGGK.",
    "DDDDDGGGGGGGGGGGK.",
    "KKKKKGGGGGGGGGGGK.",
    "....KGGGGGGGGGGGK.",
    "....KGGKKKKKKKGGK.",
    "....KGGGGGGGGGGGK.",
    "....KKKKKKKKKKKKK.",
    ".....KK.....KK....",
    "....KDDK...KDDK...",
    "....KDDK...KDDK...",
    ".....KK.....KK....",
};

static const char* const BALL[] = {
    "..KKKK..",
    ".KWLLLK.",
    "KLLWLLLK",
    "KLLLWLLK",
    "KLLLWLLK",
    "KLLWLLLK",
    ".KWLLLK.",
    "..KKKK..",
};

static const char* const HEART_FULL[] = {
    ".KK.KK.",
    "KRRKRRK",
    "KRRRRRK",
    ".KRRRK.",
    "..KRK..",
    "...K...",
};

static const char* const HEART_EMPTY[] = {
    ".KK.KK.",
    "KDDKDDK",
    "KDDDDDK",
    ".KDDDK.",
    "..KDK..",
    "...K...",
};

static const char* const MOUNTAINS[] = {
    "..........D.....................",
    ".........DGD..............D.....",
    "........DGGGD............DGD....",
    ".......DGGGGGD..........DGGGD...",
    "......DGGGGGGGD.....D..DGGGGGD..",
    "....DDGGGGGGGGGD...DGDDGGGGGGGD.",
    "..DDGGGGGGGGGGGGDDDGGGGGGGGGGGGD",
    "DDGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG",
};

const Sprite tennis_doe_stand = SPRITE(DOE_STAND);
const Sprite tennis_doe_crouch = SPRITE(DOE_CROUCH);
const Sprite tennis_doe_jump = SPRITE(DOE_JUMP);
const Sprite tennis_doe_bite = SPRITE(DOE_BITE);
const Sprite tennis_machine_idle = SPRITE(MACHINE_IDLE);
const Sprite tennis_machine_ready = SPRITE(MACHINE_READY);
const Sprite tennis_ball = SPRITE(BALL);
const Sprite tennis_heart_full = SPRITE(HEART_FULL);
const Sprite tennis_heart_empty = SPRITE(HEART_EMPTY);
const Sprite tennis_mountains = SPRITE(MOUNTAINS);

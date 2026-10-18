/**
 * 各小游戏共用的占位像素画
 */

#include "game_art.h"

static const char* const DIGIT_1[] = {
    "...YY..",
    "..YYY..",
    ".YYYY..",
    "...YY..",
    "...YY..",
    "...YY..",
    "...YY..",
    ".YYYYYY",
    ".YYYYYY",
};

static const char* const DIGIT_2[] = {
    ".YYYYY.",
    "YYYYYYY",
    "YY...YY",
    ".....YY",
    "...YYY.",
    ".YYY...",
    "YY.....",
    "YYYYYYY",
    "YYYYYYY",
};

static const char* const DIGIT_3[] = {
    "YYYYYY.",
    "YYYYYYY",
    ".....YY",
    "..YYYYY",
    "..YYYY.",
    ".....YY",
    ".....YY",
    "YYYYYYY",
    "YYYYYY.",
};

const Sprite game_digits[3] = {SPRITE(DIGIT_1), SPRITE(DIGIT_2), SPRITE(DIGIT_3)};

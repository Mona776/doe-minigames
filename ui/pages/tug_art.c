/**
 * 拔河的占位像素画：Doe 朝右咬着绳子
 */

#include "tug_art.h"

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

static const char* const DOE_PULL[] = {
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
    "..KWWWWWWWWBBWWWK...",
    "..KWWKKKKKKKKWWK....",
    "...KWK.......KWK....",
    "....KK........KWK...",
};

static const char* const DOE_GUARD[] = {
    "....................",
    "....................",
    "....................",
    "..........KK...KK...",
    ".........KBBK.KBBK..",
    ".........KWWWWKWWWK.",
    ".........KWWWWWWWWKK",
    ".K.......KWWWWWWWPPK",
    "KWK......KWWWWWWWKK.",
    ".KWKKKKKKKWWWWWWWWK.",
    "..KWWWWWWWWWWWWWWWK.",
    "..KWWWWWWWWBBWWWWWK.",
    "..KWWKKKKKKKKKKKWWK.",
    ".KWK...........KWK..",
    "KK..............KK..",
    "....................",
};

// 用力过猛滑倒：整个趴在地上，四条腿劈开，嘴比站着低 TUG_DOE_SLIP_DROP 行
static const char* const DOE_SLIP[] = {
    "....................",
    "....................",
    "....................",
    "....................",
    "....................",
    "....................",
    "..........KK...KK...",
    ".........KBBK.KBBK..",
    ".........KWWWWKWWWK.",
    ".........KWWWWWWWWKK",
    ".K.......KWWWWWWWPPK",
    "KWK......KWWWWWWWKK.",
    ".KWKKKKKKKWWWWWWWWK.",
    "..KWWWWWWWWBBWWWWWK.",
    "KKWWKKKKKKKKKKKKWWKK",
    "....................",
};

static const char* const TOY[] = {
    "..KKKK..",
    ".KRRYRK.",
    "KRRYRRRK",
    "KYRRRYRK",
    "KRRYRRYK",
    "KRYRRRRK",
    ".KRRYRK.",
    "..KKKK..",
};

const Sprite tug_doe_stand = SPRITE(DOE_STAND);
const Sprite tug_doe_pull = SPRITE(DOE_PULL);
const Sprite tug_doe_guard = SPRITE(DOE_GUARD);
const Sprite tug_doe_slip = SPRITE(DOE_SLIP);
const Sprite tug_toy = SPRITE(TOY);

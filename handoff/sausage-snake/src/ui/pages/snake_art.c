/**
 * 贪吃腊肠的占位像素画
 */

#include "snake_art.h"

static const char* const HEAD[] = {
    "..KKKK....",
    ".KBBBBK...",
    "KBBBBKBKK.",
    "BBEEBBBBBK",
    "BEEEBBBBBP",
    "BEEEBBBBKK",
    "BKEKBBBBK.",
    "KKKKKKKK..",
    "..........",
    "..........",
};

static const char* const BODY[] = {
    "..........",
    "..........",
    "KKKKKKKKKK",
    "BBBBBBBBBB",
    "BBBBBBBBBB",
    "BBBBBBBBBB",
    "BBBBBBBBBB",
    "KKKKKKKKKK",
    "..........",
    "..........",
};

static const char* const CORNER[] = {
    "..........",
    "..........",
    "KKKKKKK...",
    "BBBBBBBK..",
    "BBBBBBBK..",
    "BBBBBBBK..",
    "BBBBBBBK..",
    "KKKBBBBK..",
    "..KBBBBK..",
    "..KBBBBK..",
};

static const char* const TAIL[] = {
    "KK........",
    "KEK.......",
    ".KEKKKKKKK",
    "..KBBBBBBB",
    "..KBBBBBBB",
    "..KBBBBBBB",
    "...KBBBBBB",
    "....KKKKKK",
    "..........",
    "..........",
};

#define LEGS_TOP "............", "............", "............", "............", "............", \
                 "............", "............", "............", "............"

// 每对脚前面一条是远侧的腿（深棕），后面一条是近侧的
static const char* const FRONT_STAND[] = {
    LEGS_TOP,
    "..EE.BB.....",
    "..EE.BB.....",
    "..KK.KK.....",
};

static const char* const FRONT_STEP[] = {
    LEGS_TOP,
    "..EE.BB.....",
    ".EE...BB....",
    ".KK...KK....",
};

static const char* const BACK_STAND[] = {
    LEGS_TOP,
    ".....EE.BB..",
    ".....EE.BB..",
    ".....KK.KK..",
};

static const char* const BACK_STEP[] = {
    LEGS_TOP,
    ".....EE.BB..",
    "....EE...BB.",
    "....KK...KK.",
};

static const char* const BONE[] = {
    "..........",
    "..........",
    ".KK....KK.",
    "KWWKKKKWWK",
    "KWWWWWWWWK",
    "KWWWWWWWWK",
    "KWWKKKKWWK",
    ".KK....KK.",
    "..........",
    "..........",
};

static const char* const ROCK[] = {
    ".KKKKKKKK.",
    "KGGGGGGGGK",
    "KGWWGGGGGK",
    "KGWGGGGGGK",
    "KGGGGGGGGK",
    "KGGGGGGDGK",
    "KGGGGGDDGK",
    "KGGGGGGGGK",
    "KDDDDDDDDK",
    ".KKKKKKKK.",
};

static const char* const ARROW[] = {
    "..Y..",
    "..YY.",
    "YYYYY",
    "..YY.",
    "..Y..",
};

static const char* const STAR[] = {
    ".Y.",
    "YYY",
    ".Y.",
};

static const char* const ICON_BONE[] = {
    "........",
    "........",
    ".W....W.",
    "WWWWWWWW",
    "WWWWWWWW",
    ".W....W.",
    "........",
    "........",
};

static const char* const ICON_SHELL[] = {
    "..PPPP..",
    ".PKPPKP.",
    "PPKPPKPP",
    "PPKPPKPP",
    ".PKPPKP.",
    "..PKKP..",
    "...PP...",
    "........",
};

#define STAR_ROWS(c) \
    "...." c "....", \
    "...." c "....", \
    "..." c c c "...", \
    c c c c c c c c c, \
    "." c c c c c c c ".", \
    ".." c c c c c "..", \
    ".." c c "." c c "..", \
    "." c c "..." c c ".", \
    "." c "....." c "."

static const char* const ICON_STAR_FULL[] = {STAR_ROWS("Y")};
static const char* const ICON_STAR_EMPTY[] = {STAR_ROWS("D")};

// 玩法说明的转向示意：从下往上走，到拐点转弯
static const char* const TURN_RIGHT[] = {
    ".........",
    "......Y..",
    "......YY.",
    "..YYYYYYY",
    "..Y...YY.",
    "..Y...Y..",
    "..Y......",
    "..Y......",
    "..Y......",
};

static const char* const TURN_LEFT[] = {
    ".........",
    "..Y......",
    ".YY......",
    "YYYYYYY..",
    ".YY...Y..",
    "..Y...Y..",
    "......Y..",
    "......Y..",
    "......Y..",
};

static const char* const GO_STRAIGHT[] = {
    "....Y....",
    "...YYY...",
    "..YYYYY..",
    "....Y....",
    "....Y....",
    "....Y....",
    "....Y....",
    "....Y....",
    "....Y....",
};

const Sprite snake_head = SPRITE(HEAD);
const Sprite snake_body = SPRITE(BODY);
const Sprite snake_corner = SPRITE(CORNER);
const Sprite snake_tail = SPRITE(TAIL);
const Sprite snake_front_legs[2] = {SPRITE(FRONT_STAND), SPRITE(FRONT_STEP)};
const Sprite snake_back_legs[2] = {SPRITE(BACK_STAND), SPRITE(BACK_STEP)};
const Sprite snake_bone = SPRITE(BONE);
const Sprite snake_rock = SPRITE(ROCK);
const Sprite snake_arrow = SPRITE(ARROW);
const Sprite snake_star = SPRITE(STAR);
const Sprite snake_icon_bone = SPRITE(ICON_BONE);
const Sprite snake_icon_shell = SPRITE(ICON_SHELL);
const Sprite snake_icon_star[2] = {SPRITE(ICON_STAR_EMPTY), SPRITE(ICON_STAR_FULL)};
const Sprite snake_turn_right = SPRITE(TURN_RIGHT);
const Sprite snake_turn_left = SPRITE(TURN_LEFT);
const Sprite snake_go_straight = SPRITE(GO_STRAIGHT);

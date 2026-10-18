/**
 * 界面侧记住的各小游戏最高分
 */

#include "ui_records.h"

static uint16_t s_best[GAME_COUNT];

void ui_records_on_event(const AppEvent* e) {
    if (e->type == APP_EVT_BEST_SCORE && e->game < GAME_COUNT) s_best[e->game] = (uint16_t)e->value;
}

uint16_t ui_records_best(GameId game) {
    return game < GAME_COUNT ? s_best[game] : 0;
}

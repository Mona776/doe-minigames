/**
 * 逻辑服务
 */

#include "game_service.h"
#include <stdio.h>
#include "core/app_msg.h"
#include "core/save.h"

_Static_assert(GAME_COUNT <= SAVE_GAME_SLOTS, "存档的最高分槽位不够，调大 SAVE_GAME_SLOTS 并升级存档版本");

static uint32_t s_shells;
static uint32_t s_spirit;
static SaveData s_save;

static const char* game_name(uint8_t game) {
    switch (game) {
        case GAME_TUG:    return "拔河";
        case GAME_SNAKE:  return "贪吃腊肠";
        case GAME_TENNIS: return "接网球";
        case GAME_SAUSAGE: return "Sausage Snake";
        default:          return "?";
    }
}

static void post_best(uint8_t game) {
    AppEvent evt = {.type = APP_EVT_BEST_SCORE, .game = game, .value = s_save.best_score[game]};
    app_event_post(&evt);
}

void game_service_init(void) {
    s_shells = 0;
    s_spirit = 0;
    if (!save_load(&s_save)) s_save = (SaveData){0};
    for (uint8_t game = 0; game < GAME_COUNT; game++) {
        if (s_save.best_score[game]) {
            printf("[Save] %s 最高分 %u\n", game_name(game), (unsigned)s_save.best_score[game]);
            post_best(game);
        }
    }
}

static void on_result(const GameResult* r) {
    s_shells += r->shells;
    s_spirit += r->spirit;
    printf("[Game] %s 结算：%u 星，贝壳 +%u（共 %u），神 +%u（共 %u）\n", game_name(r->game),
           (unsigned)r->stars, (unsigned)r->shells, (unsigned)s_shells, (unsigned)r->spirit, (unsigned)s_spirit);

    if (r->game >= GAME_COUNT || r->score <= s_save.best_score[r->game]) return;
    s_save.best_score[r->game] = r->score;
    printf("[Game] %s 新纪录 %u\n", game_name(r->game), (unsigned)r->score);
    if (!save_write(&s_save)) printf("[Save] 存档写入失败\n");
    post_best(r->game);
}

static void handle_cmd(const AppCmd* cmd) {
    switch (cmd->type) {
        case APP_CMD_GAME_RESULT:
            on_result(&cmd->result);
            break;
    }
}

void game_service_step(void) {
    AppCmd cmd;
    while (app_cmd_receive(&cmd)) handle_cmd(&cmd);
}

void game_service_task(void) {
    AppCmd cmd;
    game_service_init();
    for (;;) {
        if (!app_cmd_wait(&cmd, APP_MSG_WAIT_FOREVER)) continue;
        handle_cmd(&cmd);
    }
}

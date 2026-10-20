/** 页面集成回归：复用实际页面实现，并经真实输入层发送按下、短按、长按事件。 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "input/input.h"
#include "input/input_config.h"
#include "ui/page_mgr.h"

static bool music_pause_requested;
void controls_track_music_pause(bool paused);
#define ui_music_pause controls_track_music_pause
#define page_sausage page_sausage_controls_subject
#include "ui/pages/page_sausage.c"
#undef page_sausage
#undef ui_music_pause
extern void ui_music_pause(bool paused);
void controls_track_music_pause(bool paused) {
    music_pause_requested = paused;
    ui_music_pause(paused);
}

static uint32_t test_now;
static uint32_t test_tick(void) { return test_now; }
static void home_create(lv_obj_t* screen, void* arg) { (void)screen; (void)arg; }
static const Page test_home = {.name = "test_home", .create = home_create};

static void deliver(void) {
    InputEvent event;
    while (input_poll(&event)) page_mgr_handle_input(&event, test_now);
}
static void feed(uint32_t mask) { input_feed(mask, test_now); deliver(); }
static void key(InputKey k, uint32_t held) {
    uint32_t mask = INPUT_BIT(k);
    feed(mask);
    test_now += INPUT_DEBOUNCE_MS + 5;
    feed(mask);
    test_now += held;
    feed(mask); // long event, if due; release must not generate a short event afterward
    feed(0);
    test_now += INPUT_DEBOUNCE_MS + 5;
    feed(0);
}
static void tap(InputKey k) { key(k, 75); }
static void elapse(uint32_t ms) {
    while (ms) {
        uint32_t dt = ms > 10 ? 10 : ms;
        test_now += dt;
        if (page_current() == &page_sausage_controls_subject) on_frame(NULL);
        ms -= dt;
    }
}
static void begin(void) {
    if (page_current() == &test_home) page_push(&page_sausage_controls_subject, NULL);
    else page_replace(&page_sausage_controls_subject, NULL);
    assert(s_game.phase == SAUSAGE_PHASE_COUNTDOWN && !s_pause_open);
    app_msg_init();
}
static void begin_running(void) {
    begin();
    elapse(SAUSAGE_COUNTDOWN_MS + SAUSAGE_GO_MS);
    assert(s_game.phase == SAUSAGE_PHASE_RUN);
}
static void expect_single_reward(void) {
    AppCmd cmd;
    unsigned count = 0;
    while (app_cmd_receive(&cmd)) {
        if (cmd.type != APP_CMD_GAME_RESULT) continue;
        assert(cmd.result.game == GAME_SAUSAGE);
        assert(cmd.result.score == 0 && cmd.result.stars == 0 && cmd.result.shells == 5);
        count++;
    }
    assert(count == 1);
}

int main(void) {
    lv_init();
    lv_sdl_window_create(240, 240);
    lv_tick_set_cb(test_tick);
    app_msg_init();
    input_init();
    page_mgr_init(&test_home);

    begin_running();
    uint8_t start_head = sausage_segment(&s_game, 0);
    tap(INPUT_KEY_OK);
    tap(INPUT_KEY_OK);
    tap(INPUT_KEY_OK);
    assert(s_game.next_dir == SAUSAGE_DOWN && s_game.dir == SAUSAGE_RIGHT);
    assert(s_game.moves == 0 && sausage_segment(&s_game, 0) == start_head);
    assert(segment_tile(&s_game, 0) == s_head_buf[SAUSAGE_RIGHT]);
    elapse(sausage_step_ms(s_game.tier) - s_game.phase_ms);
    assert(s_game.moves == 1 && s_game.dir == SAUSAGE_DOWN);
    assert(segment_tile(&s_game, 0) == s_head_buf[SAUSAGE_DOWN]);
    // A held confirmation key must not turn again when the next beat starts.
    feed(INPUT_BIT(INPUT_KEY_OK));
    test_now += INPUT_DEBOUNCE_MS + 5;
    feed(INPUT_BIT(INPUT_KEY_OK));
    elapse(sausage_step_ms(s_game.tier) - s_game.phase_ms);
    assert(s_game.dir == SAUSAGE_LEFT);
    elapse(sausage_step_ms(s_game.tier));
    assert(s_game.dir == SAUSAGE_LEFT);
    feed(0);
    test_now += INPUT_DEBOUNCE_MS + 5;
    feed(0);
    tap(INPUT_KEY_OK);
    assert(s_game.next_dir == SAUSAGE_UP && s_game.dir == SAUSAGE_LEFT);
    puts("PASS beat input: rapid taps lock once, head turns only with movement, held key does not repeat, release/repress works");

    begin();
    key(INPUT_KEY_BACK, INPUT_LONG_PRESS_MS + 100);
    assert(!s_pause_open && s_game.phase == SAUSAGE_PHASE_COUNTDOWN);
    tap(INPUT_KEY_BACK);
    assert(s_pause_open && s_pause_focus == PAUSE_END);
    uint32_t remaining = s_game.phase_ms;
    elapse(700);
    assert(s_game.phase_ms == remaining);
    tap(INPUT_KEY_SELECT);
    assert(s_pause_focus == PAUSE_CONTINUE);
    tap(INPUT_KEY_OK);
    assert(!s_pause_open && s_game.phase == SAUSAGE_PHASE_COUNTDOWN);
    assert(s_game.phase_ms == remaining);
    tap(INPUT_KEY_BACK);
    tap(INPUT_KEY_OK);
    assert(!s_pause_open && s_result_shown && s_game.end == SAUSAGE_END_QUIT);
    tap(INPUT_KEY_OK);
    assert(s_result_shown && s_game.phase == SAUSAGE_PHASE_OVER);
    expect_single_reward();
    tap(INPUT_KEY_BACK);
    assert(page_current() == &page_sausage_entry);
    puts("PASS countdown: shared menu, default END, frozen time, continue, result guard/back, reward once");

    begin_running();
    lv_obj_t* menu = s_pause;
    tap(INPUT_KEY_SELECT);
    assert(s_pause_open && s_game.phase == SAUSAGE_PHASE_PAUSED && s_pause_focus == PAUSE_CONTINUE);
    assert(music_pause_requested);
    tap(INPUT_KEY_SELECT);
    assert(s_pause_focus == PAUSE_END);
    tap(INPUT_KEY_SELECT);
    assert(s_pause_focus == PAUSE_CONTINUE);
    uint32_t moves = s_game.moves;
    uint32_t elapsed = s_game.elapsed_ms;
    elapse(1200);
    assert(s_game.moves == moves && s_game.elapsed_ms == elapsed);
    key(INPUT_KEY_BACK, INPUT_LONG_PRESS_MS + 100);
    assert(s_pause_open && s_game.phase == SAUSAGE_PHASE_PAUSED);
    tap(INPUT_KEY_BACK); // cancel closes the one menu; it must not open another layer
    assert(!s_pause_open && s_game.phase == SAUSAGE_PHASE_RUN && !music_pause_requested);
    assert(s_game.phase_ms == 0);
    elapse(sausage_step_ms(s_game.tier) - 1);
    assert(s_game.moves == moves);
    elapse(1);
    assert(s_game.moves == moves + 1);
    puts("PASS SELECT entry: default CONTINUE, no double release, freeze, long back ignored, cancel resumes with full interval");

    tap(INPUT_KEY_BACK);
    assert(s_pause == menu && s_pause_open && s_pause_focus == PAUSE_END);
    elapsed = s_game.elapsed_ms;
    elapse(900);
    assert(s_game.elapsed_ms == elapsed && music_pause_requested);
    tap(INPUT_KEY_SELECT);
    assert(s_pause_focus == PAUSE_CONTINUE);
    tap(INPUT_KEY_OK);
    assert(!s_pause_open && s_game.phase == SAUSAGE_PHASE_RUN && !music_pause_requested);
    assert(s_game.phase_ms == 0);
    tap(INPUT_KEY_BACK);
    tap(INPUT_KEY_OK);
    assert(!s_pause_open && s_result_shown && s_game.end == SAUSAGE_END_QUIT);
    expect_single_reward();
    elapse(800);
    tap(INPUT_KEY_SELECT);
    assert(s_result_focus == RESULT_LEAVE);
    tap(INPUT_KEY_OK);
    assert(page_current() == &page_sausage_entry);
    puts("PASS BACK entry: same menu object, default END, continue/END execute directly, result menu retained");

    begin_running();
    tap(INPUT_KEY_SELECT);
    tap(INPUT_KEY_SELECT);
    tap(INPUT_KEY_OK);
    assert(!s_pause_open && s_result_shown && s_game.end == SAUSAGE_END_QUIT);
    expect_single_reward();
    elapse(800);
    tap(INPUT_KEY_OK);
    assert(!s_result_shown && !s_pause_open && s_game.phase == SAUSAGE_PHASE_COUNTDOWN);
    puts("PASS END from SELECT entry: result shown without another confirmation; retry resets menu and countdown");

    begin_running();
    s_game.bones = 8; // non-zero-score fixture: reward rules must remain unchanged
    tap(INPUT_KEY_BACK);
    tap(INPUT_KEY_OK);
    assert(s_result_shown && s_game.end == SAUSAGE_END_QUIT);
    AppCmd cmd;
    unsigned results = 0;
    while (app_cmd_receive(&cmd)) {
        if (cmd.type != APP_CMD_GAME_RESULT) continue;
        assert(cmd.result.score == 8 && cmd.result.stars == 1 && cmd.result.shells == 10 && cmd.result.spirit == 1);
        results++;
    }
    assert(results == 1);
    puts("PASS reward unchanged: 8 bones -> 1 star, 10 shells, +1 spirit, one result");
    destroy();
    lv_sdl_quit();
    puts("Sausage unified menu integration tests passed.");
    return 0;
}

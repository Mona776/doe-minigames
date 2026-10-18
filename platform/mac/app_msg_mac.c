/**
 * 消息队列 - Mac 模拟器实现（单线程环形缓冲）
 */

#include "core/app_msg.h"

#define CMD_QUEUE_LEN 16
#define EVT_QUEUE_LEN 32

static AppCmd s_cmds[CMD_QUEUE_LEN];
static uint32_t s_cmd_head, s_cmd_tail;

static AppEvent s_evts[EVT_QUEUE_LEN];
static uint32_t s_evt_head, s_evt_tail;

void app_msg_init(void) {
    s_cmd_head = s_cmd_tail = 0;
    s_evt_head = s_evt_tail = 0;
}

bool app_cmd_send(const AppCmd* cmd) {
    if (s_cmd_head - s_cmd_tail >= CMD_QUEUE_LEN) return false;
    s_cmds[s_cmd_head++ % CMD_QUEUE_LEN] = *cmd;
    return true;
}

bool app_cmd_receive(AppCmd* cmd) {
    if (s_cmd_head == s_cmd_tail) return false;
    *cmd = s_cmds[s_cmd_tail++ % CMD_QUEUE_LEN];
    return true;
}

bool app_cmd_wait(AppCmd* cmd, uint32_t timeout_ms) {
    (void)timeout_ms;
    return app_cmd_receive(cmd);
}

bool app_event_post(const AppEvent* evt) {
    if (s_evt_head - s_evt_tail >= EVT_QUEUE_LEN) return false;
    s_evts[s_evt_head++ % EVT_QUEUE_LEN] = *evt;
    return true;
}

bool app_event_receive(AppEvent* evt) {
    if (s_evt_head == s_evt_tail) return false;
    *evt = s_evts[s_evt_tail++ % EVT_QUEUE_LEN];
    return true;
}

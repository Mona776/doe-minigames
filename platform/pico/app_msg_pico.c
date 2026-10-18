/**
 * 消息队列 - Pico 实现（FreeRTOS 队列）
 */

#include "core/app_msg.h"
#include "FreeRTOS.h"
#include "queue.h"

#define CMD_QUEUE_LEN 16
#define EVT_QUEUE_LEN 32

static QueueHandle_t s_cmds;
static QueueHandle_t s_evts;

void app_msg_init(void) {
    s_cmds = xQueueCreate(CMD_QUEUE_LEN, sizeof(AppCmd));
    s_evts = xQueueCreate(EVT_QUEUE_LEN, sizeof(AppEvent));
    configASSERT(s_cmds != NULL && s_evts != NULL);
}

bool app_cmd_send(const AppCmd* cmd) {
    return xQueueSend(s_cmds, cmd, 0) == pdPASS;
}

bool app_cmd_receive(AppCmd* cmd) {
    return xQueueReceive(s_cmds, cmd, 0) == pdPASS;
}

bool app_cmd_wait(AppCmd* cmd, uint32_t timeout_ms) {
    TickType_t ticks = timeout_ms == APP_MSG_WAIT_FOREVER ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xQueueReceive(s_cmds, cmd, ticks) == pdPASS;
}

bool app_event_post(const AppEvent* evt) {
    return xQueueSend(s_evts, evt, 0) == pdPASS;
}

bool app_event_receive(AppEvent* evt) {
    return xQueueReceive(s_evts, evt, 0) == pdPASS;
}

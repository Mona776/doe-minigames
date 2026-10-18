/**
 * 主线程和界面任务共享的画面、按键。用互斥锁，不用 FreeRTOS 队列。
 */

#include "sim_host.h"
#include <pthread.h>
#include <string.h>

static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
static uint16_t s_frame[SIM_SCREEN_SIZE * SIM_SCREEN_SIZE];
static bool s_have_frame;
static uint32_t s_keys;
static bool s_quit;

void sim_host_init(void) {}

void sim_keys_write(uint32_t mask) {
    pthread_mutex_lock(&s_lock);
    s_keys = mask;
    pthread_mutex_unlock(&s_lock);
}

uint32_t sim_keys_read(void) {
    pthread_mutex_lock(&s_lock);
    uint32_t mask = s_keys;
    pthread_mutex_unlock(&s_lock);
    return mask;
}

void sim_frame_publish(const uint16_t* src, int32_t stride_px) {
    pthread_mutex_lock(&s_lock);
    if (stride_px == SIM_SCREEN_SIZE) {
        memcpy(s_frame, src, sizeof(s_frame));
    } else {
        for (int y = 0; y < SIM_SCREEN_SIZE; y++) {
            memcpy(s_frame + y * SIM_SCREEN_SIZE, src + y * stride_px, SIM_SCREEN_SIZE * sizeof(uint16_t));
        }
    }
    s_have_frame = true;
    pthread_mutex_unlock(&s_lock);
}

bool sim_frame_copy(uint16_t* dst) {
    pthread_mutex_lock(&s_lock);
    bool have = s_have_frame;
    if (have) memcpy(dst, s_frame, sizeof(s_frame));
    pthread_mutex_unlock(&s_lock);
    return have;
}

void sim_request_quit(void) {
    pthread_mutex_lock(&s_lock);
    s_quit = true;
    pthread_mutex_unlock(&s_lock);
}

bool sim_quit_requested(void) {
    pthread_mutex_lock(&s_lock);
    bool quit = s_quit;
    pthread_mutex_unlock(&s_lock);
    return quit;
}

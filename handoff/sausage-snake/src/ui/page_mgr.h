/**
 * 页面管理器 - 页面栈、切换过渡、全局默认键义、空闲超时回首页
 * 每个页面独占一个 LVGL screen；只在 LVGL 所在的线程/任务中使用
 */

#ifndef PAGE_MGR_H
#define PAGE_MGR_H

#include <stdbool.h>
#include <stdint.h>
#include "lvgl.h"
#include "core/app_msg.h"
#include "input/input.h"

#define PAGE_STACK_MAX            8
#define PAGE_IDLE_TIMEOUT_DEFAULT 10000  // 非首页无操作 10 秒回首页（PRD 6.4）

typedef struct {
    const char* name;
    void (*create)(lv_obj_t* screen, void* arg);
    void (*destroy)(void);                   // 可选：释放定时器、清空静态指针
    void (*resume)(void);                    // 可选：上层页面关闭、重新回到本页时调用
    bool (*on_input)(const InputEvent* e);   // 返回 false 交给全局默认键义
    void (*on_event)(const AppEvent* e);     // 可选：只有栈顶页面会收到
    uint32_t idle_timeout_ms;                // 0 表示不超时
} Page;

void page_mgr_init(const Page* home);

void page_push(const Page* page, void* arg);
void page_push_anim(const Page* page, void* arg, lv_screen_load_anim_t anim);
void page_pop(void);
void page_replace(const Page* page, void* arg);
void page_back_home(void);
void page_replace_above_home(const Page* page, void* arg);  // 栈变成 首页 > page，返回直接回首页

const Page* page_current(void);
void page_mgr_print_stack(void);

void page_mgr_handle_input(const InputEvent* e, uint32_t now_ms);
void page_mgr_on_event(const AppEvent* e);
void page_mgr_tick(uint32_t now_ms);

#endif

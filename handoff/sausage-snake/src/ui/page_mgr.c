/**
 * 页面管理器
 */

#include "page_mgr.h"
#include <stdio.h>

#define TRANSITION_MS 200

typedef struct {
    const Page* page;
    lv_obj_t* screen;
} PageEntry;

static PageEntry s_stack[PAGE_STACK_MAX];
static int s_depth;
static uint32_t s_now_ms;
static uint32_t s_last_input_ms;

static lv_obj_t* create_screen(void) {
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_scrollable(screen, false);
    lv_obj_set_style_text_color(screen, lv_color_hex(0x333333), 0);
    return screen;
}

static void destroy_entry(PageEntry* entry) {
    if (entry->page->destroy) entry->page->destroy();
}

void page_mgr_init(const Page* home) {
    lv_obj_t* old = lv_screen_active();
    lv_obj_t* screen = create_screen();
    s_stack[0].page = home;
    s_stack[0].screen = screen;
    s_depth = 1;
    home->create(screen, NULL);
    lv_screen_load(screen);
    if (old) lv_obj_delete(old);
}

void page_push(const Page* page, void* arg) {
    page_push_anim(page, arg, LV_SCREEN_LOAD_ANIM_MOVE_LEFT);
}

void page_push_anim(const Page* page, void* arg, lv_screen_load_anim_t anim) {
    if (s_depth >= PAGE_STACK_MAX) {
        printf("[Page] 页面栈已满，无法打开 %s\n", page->name);
        return;
    }
    lv_obj_t* screen = create_screen();
    s_stack[s_depth].page = page;
    s_stack[s_depth].screen = screen;
    s_depth++;
    page->create(screen, arg);
    lv_screen_load_anim(screen, anim, TRANSITION_MS, 0, false);
    s_last_input_ms = s_now_ms;
}

void page_pop(void) {
    if (s_depth <= 1) return;
    destroy_entry(&s_stack[--s_depth]);
    PageEntry* prev = &s_stack[s_depth - 1];
    lv_screen_load_anim(prev->screen, LV_SCREEN_LOAD_ANIM_MOVE_RIGHT, TRANSITION_MS, 0, true);
    if (prev->page->resume) prev->page->resume();
    s_last_input_ms = s_now_ms;
}

void page_replace(const Page* page, void* arg) {
    if (s_depth <= 1) {
        page_push(page, arg);
        return;
    }
    PageEntry* top = &s_stack[s_depth - 1];
    destroy_entry(top);
    lv_obj_t* screen = create_screen();
    top->page = page;
    top->screen = screen;
    page->create(screen, arg);
    lv_screen_load_anim(screen, LV_SCREEN_LOAD_ANIM_FADE_IN, TRANSITION_MS, 0, true);
    s_last_input_ms = s_now_ms;
}

// 删掉首页和栈顶之间的页面（不在显示），栈顶页面留给后续的过渡动画删除
static void drop_middle(void) {
    while (s_depth > 2) {
        PageEntry* entry = &s_stack[s_depth - 2];
        destroy_entry(entry);
        lv_obj_delete(entry->screen);
        *entry = s_stack[s_depth - 1];
        s_depth--;
    }
}

void page_back_home(void) {
    if (s_depth <= 1) return;
    drop_middle();
    page_pop();
}
void page_replace_above_home(const Page* page, void* arg) {
    drop_middle();
    page_replace(page, arg);
}

const Page* page_current(void) {
    return s_depth > 0 ? s_stack[s_depth - 1].page : NULL;
}

void page_mgr_print_stack(void) {
    printf("[Page] 页面栈:");
    for (int i = 0; i < s_depth; i++) printf(" %s%s", s_stack[i].page->name, i + 1 < s_depth ? " >" : "");
    printf("\n");
}

void page_mgr_handle_input(const InputEvent* e, uint32_t now_ms) {
    s_now_ms = now_ms;
    s_last_input_ms = now_ms;
    if (s_depth == 0) return;

    const Page* top = s_stack[s_depth - 1].page;
    if (top->on_input && top->on_input(e)) return;

    if (e->type == INPUT_EVT_SHORT_PRESS && e->key == INPUT_KEY_BACK) page_pop();
}

void page_mgr_on_event(const AppEvent* e) {
    if (s_depth == 0) return;
    const Page* top = s_stack[s_depth - 1].page;
    if (top->on_event) top->on_event(e);
}

void page_mgr_tick(uint32_t now_ms) {
    s_now_ms = now_ms;
    if (s_depth <= 1) {
        s_last_input_ms = now_ms;
        return;
    }
    const Page* top = s_stack[s_depth - 1].page;
    if (top->idle_timeout_ms && now_ms - s_last_input_ms >= top->idle_timeout_ms) {
        printf("[Page] %s 空闲 %u 秒，回到首页\n", top->name, (unsigned)(top->idle_timeout_ms / 1000));
        page_back_home();
    }
}

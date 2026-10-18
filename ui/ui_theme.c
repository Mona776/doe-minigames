/**
 * 界面风格
 */

#include "ui_theme.h"
#include "asset.h"

#define BORDER_W  2
#define RADIUS    4
#define STAT_GAP  2
#define BRACKET   10  // 选中按钮四角括号的边长
#define BRACKET_GAP 4  // 括号伸出按钮外的距离

// 按钮的子对象：图标、文字、箭头，后面是四角括号
#define BUTTON_FIRST_BRACKET 3

static lv_style_t s_panel;
static lv_style_t s_button;
static lv_style_t s_button_focused;
static lv_style_t s_bracket;
static lv_style_t s_hint;
static lv_style_t s_hint_alert;
static bool s_styles_ready;

static void init_styles(void) {
    if (s_styles_ready) return;
    s_styles_ready = true;

    lv_style_init(&s_panel);
    lv_style_set_bg_color(&s_panel, lv_color_hex(UI_COLOR_PANEL));
    lv_style_set_bg_opa(&s_panel, LV_OPA_COVER);
    lv_style_set_radius(&s_panel, RADIUS);
    lv_style_set_border_width(&s_panel, BORDER_W);
    lv_style_set_border_color(&s_panel, lv_color_hex(UI_COLOR_LINE));
    lv_style_set_pad_all(&s_panel, 4);
    lv_style_set_text_color(&s_panel, lv_color_hex(UI_COLOR_TEXT));

    lv_style_init(&s_button);
    lv_style_set_bg_color(&s_button, lv_color_hex(UI_COLOR_CREAM));
    lv_style_set_bg_opa(&s_button, LV_OPA_COVER);
    lv_style_set_radius(&s_button, RADIUS);
    lv_style_set_border_width(&s_button, BORDER_W);
    lv_style_set_border_color(&s_button, lv_color_hex(UI_COLOR_INK));
    lv_style_set_pad_hor(&s_button, 8);
    lv_style_set_pad_column(&s_button, 8);
    lv_style_set_text_color(&s_button, lv_color_hex(UI_COLOR_INK));

    lv_style_init(&s_button_focused);
    lv_style_set_bg_color(&s_button_focused, lv_color_hex(UI_COLOR_FOCUS));

    lv_style_init(&s_bracket);
    lv_style_set_border_width(&s_bracket, BORDER_W);
    lv_style_set_border_color(&s_bracket, lv_color_hex(UI_COLOR_GLOW));

    lv_style_init(&s_hint);
    lv_style_set_bg_color(&s_hint, lv_color_hex(UI_COLOR_PANEL));
    lv_style_set_bg_opa(&s_hint, LV_OPA_COVER);
    lv_style_set_radius(&s_hint, RADIUS);
    lv_style_set_border_width(&s_hint, BORDER_W);
    lv_style_set_border_color(&s_hint, lv_color_hex(UI_COLOR_LINE));
    lv_style_set_text_color(&s_hint, lv_color_hex(UI_COLOR_TEXT));
    lv_style_set_text_font(&s_hint, &font_ui_12);

    lv_style_init(&s_hint_alert);
    lv_style_set_bg_color(&s_hint_alert, lv_color_hex(UI_COLOR_FOCUS));
    lv_style_set_border_color(&s_hint_alert, lv_color_hex(UI_COLOR_FOCUS));
    lv_style_set_text_color(&s_hint_alert, lv_color_hex(UI_COLOR_INK));
}

lv_obj_t* ui_box_create(lv_obj_t* parent) {
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_scrollable(obj, false);
    lv_obj_set_clickable(obj, false);
    return obj;
}

static void flex_row(lv_obj_t* obj, int32_t gap) {
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(obj, gap, 0);
}

lv_obj_t* ui_panel_create(lv_obj_t* parent) {
    init_styles();
    lv_obj_t* panel = ui_box_create(parent);
    lv_obj_add_style(panel, &s_panel, 0);
    lv_obj_set_size(panel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    return panel;
}

void ui_screen_dark(lv_obj_t* scr) {
    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_COLOR_SCREEN), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(scr, lv_color_hex(UI_COLOR_TEXT), 0);
}

lv_obj_t* ui_title_create(lv_obj_t* parent, const char* text) {
    lv_obj_t* title = ui_box_create(parent);
    lv_obj_set_size(title, UI_CONTENT_W, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_bottom(title, 4, 0);
    lv_obj_set_style_border_width(title, BORDER_W, 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(title, lv_color_hex(UI_COLOR_LINE_DIM), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(UI_COLOR_TEXT), 0);
    flex_row(title, 8);
    lv_obj_set_flex_align(title, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    ui_icon_create(title, "paw");
    lv_obj_t* label = lv_label_create(title);
    lv_obj_set_style_text_font(label, &font_ui_24, 0);
    lv_label_set_text(label, text);
    ui_icon_create(title, "paw");
    return title;
}

// 按钮外面的一个角：只画两条边的小方框，不参与按钮内的排版
static void create_bracket(lv_obj_t* button, lv_align_t align, lv_border_side_t sides) {
    lv_obj_t* bracket = ui_box_create(button);
    lv_obj_add_style(bracket, &s_bracket, 0);
    lv_obj_set_style_border_side(bracket, sides, 0);
    lv_obj_set_size(bracket, BRACKET, BRACKET);
    lv_obj_set_ignore_layout(bracket, true);
    // 对齐以按钮的内容区为准，要再往外推过内边距和描边
    int32_t out_x = lv_obj_get_style_pad_left(button, 0) + BORDER_W + BRACKET_GAP;
    int32_t out_y = lv_obj_get_style_pad_top(button, 0) + BORDER_W + BRACKET_GAP;
    bool left = align == LV_ALIGN_TOP_LEFT || align == LV_ALIGN_BOTTOM_LEFT;
    bool top = align == LV_ALIGN_TOP_LEFT || align == LV_ALIGN_TOP_RIGHT;
    lv_obj_align(bracket, align, left ? -out_x : out_x, top ? -out_y : out_y);
    lv_obj_set_hidden(bracket, true);
}

// 子对象画到按钮外面时只能超出按钮的扩展绘制区，要把括号伸出去的部分算进来
static void on_button_ext_draw(lv_event_t* e) {
    lv_event_set_ext_draw_size(e, BRACKET_GAP);
}

lv_obj_t* ui_button_create(lv_obj_t* parent, const char* icon, const char* text) {
    init_styles();
    lv_obj_t* button = ui_box_create(parent);
    lv_obj_add_style(button, &s_button, 0);
    lv_obj_add_style(button, &s_button_focused, LV_STATE_CHECKED);
    lv_obj_set_overflow_visible(button, true);
    lv_obj_add_event_cb(button, on_button_ext_draw, LV_EVENT_REFR_EXT_DRAW_SIZE, NULL);
    lv_obj_refresh_ext_draw_size(button);
    flex_row(button, 8);

    ui_icon_create(button, icon);
    lv_obj_t* label = lv_label_create(button);
    lv_obj_set_style_text_font(label, &font_ui_24, 0);
    lv_label_set_text(label, text);
    lv_obj_set_flex_grow(label, 1);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    ui_icon_create(button, "arrow");

    create_bracket(button, LV_ALIGN_TOP_LEFT, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_LEFT);
    create_bracket(button, LV_ALIGN_TOP_RIGHT, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_RIGHT);
    create_bracket(button, LV_ALIGN_BOTTOM_LEFT, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_LEFT);
    create_bracket(button, LV_ALIGN_BOTTOM_RIGHT, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_RIGHT);
    return button;
}

void ui_button_set_focused(lv_obj_t* button, bool focused) {
    lv_obj_set_state(button, LV_STATE_CHECKED, focused);
    uint32_t count = lv_obj_get_child_count(button);
    for (uint32_t i = BUTTON_FIRST_BRACKET; i < count; i++) {
        lv_obj_set_hidden(lv_obj_get_child(button, (int32_t)i), !focused);
    }
}

lv_obj_t* ui_hint_bar_create(lv_obj_t* parent, const char* text) {
    init_styles();
    lv_obj_t* bar = ui_box_create(parent);
    lv_obj_add_style(bar, &s_hint, 0);
    lv_obj_t* label = lv_label_create(bar);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    return bar;
}

void ui_hint_bar_set_alert(lv_obj_t* bar) {
    lv_obj_add_style(bar, &s_hint_alert, 0);
}

lv_obj_t* ui_icon_create(lv_obj_t* parent, const char* name) {
    lv_obj_t* icon = lv_image_create(parent);
    lv_obj_set_clickable(icon, false);
    lv_image_set_antialias(icon, false);
    lv_image_set_inner_align(icon, LV_IMAGE_ALIGN_STRETCH);
    ui_icon_set(icon, name);
    return icon;
}

void ui_icon_set(lv_obj_t* icon, const char* name) {
    if (name == NULL || name[0] == '\0') {
        lv_obj_set_hidden(icon, true);
        return;
    }
    char path[ASSET_NAME_MAX];
    lv_snprintf(path, sizeof(path), "ui/%s", name);
    const AssetAnim* anim = asset_anim_find(path);
    lv_obj_set_hidden(icon, anim == NULL);
    if (anim == NULL) return;
    if (lv_image_get_src(icon) != anim->frames[0]) lv_image_set_src(icon, anim->frames[0]);
    lv_obj_set_size(icon, anim->width * UI_ICON_ZOOM, anim->height * UI_ICON_ZOOM);
}

lv_obj_t* ui_stat_row_create(lv_obj_t* parent, const char* label, const char* icon, int32_t max) {
    lv_obj_t* row = ui_box_create(parent);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    flex_row(row, STAT_GAP);
    lv_obj_set_user_data(row, (void*)icon);

    lv_obj_t* name = lv_label_create(row);
    lv_label_set_text(name, label);
    lv_obj_set_style_pad_right(name, 4, 0);
    for (int32_t i = 0; i < max; i++) ui_icon_create(row, "");
    ui_stat_row_set(row, 0);
    return row;
}

void ui_stat_row_set(lv_obj_t* row, int32_t value) {
    const char* icon = lv_obj_get_user_data(row);
    uint32_t count = lv_obj_get_child_count(row);
    for (uint32_t i = 1; i < count; i++) {
        char name[ASSET_NAME_MAX];
        lv_snprintf(name, sizeof(name), "%s_%s", icon, (int32_t)i <= value ? "full" : "empty");
        ui_icon_set(lv_obj_get_child(row, (int32_t)i), name);
    }
}

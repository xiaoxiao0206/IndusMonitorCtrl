/**
 ******************************************************************************
 * @file    user.c
 * @brief   LVGL 通用控件创建函数
 *
 *  提供:
 *    create_common_label  — 公共标签
 *    create_sensor_btn    — 传感器图标按钮
 *    create_common_slider — 数值滑块(带动态内存管理)
 *    create_slider_switch — 开关滑块(0/1)
 *
 *  内存管理:
 *    每个 slider 创建时 lv_mem_alloc(slider_t) 绑定为 user_data
 *    删除时通过 slider_mem_free_cb 自动 lv_mem_free，无泄漏
 ******************************************************************************
 */

#include "user.h"

/* ============================================================
 *  内部回调: slider 被删除时释放 user_data 内存
 *
 *  触发时机: lv_obj_clean(new_page) 或 lv_obj_del(slider)
 *  保证 slider_t 不会内存泄漏
 * ============================================================ */
static void slider_mem_free_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    slider_t *data = (slider_t *)lv_obj_get_user_data(slider);
    if (data != NULL)
    {
        lv_mem_free(data);
    }
}


/* ============================================================
 *  创建公共标签
 *  对齐到父对象底部居中，黑字，16号字体
 * ============================================================ */
void create_common_label(lv_obj_t **label, lv_obj_t *parent)
{
    *label = lv_label_create(parent);
    lv_obj_set_style_text_font(*label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(*label, lv_color_black(), 0);
    lv_obj_align(*label, LV_ALIGN_BOTTOM_MID, 0, 0);
}


/* ============================================================
 *  创建传感器图标按钮
 *
 *  布局:
 *    ┌────────────┐
 *    │   [图标]   │  ← TOP_MID 对齐
 *    │            │
 *    └────────────┘
 * ============================================================ */
void create_sensor_btn(lv_obj_t *parent, lv_obj_t **btn,
                       int x, int y, int w, int h,
                       lv_style_t *style, const char *symbol,
                       const lv_font_t *font)
{
    /* 创建按钮 */
    *btn = lv_btn_create(parent);
    lv_obj_add_style(*btn, style, LV_PART_MAIN);
    lv_obj_set_pos(*btn, x, y);
    lv_obj_set_size(*btn, w, h);

    /* 创建图标标签(居顶对齐) */
    lv_obj_t *label = lv_label_create(*btn);
    lv_label_set_text(label, symbol);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_align(label, LV_ALIGN_TOP_MID);
}


/* ============================================================
 *  创建数值滑块
 *
 *  内部自动完成:
 *    1. 创建滑块 + 标签
 *    2. lv_mem_alloc(slider_t) 作为 user_data
 *    3. 初始化默认值(link_slider=NULL, limit_type=NONE)
 *    4. 注册 DELETE 回调自动释放内存
 *
 *  使用方创建后需补充:
 *    s->param_id  = PARAM_XXX;   // 关联参数ID
 *    s->range_max = max;         // 记录上限
 *    lv_obj_add_event_cb(...);   // 绑定 slider_key_cb
 *    lv_group_add_obj(...);      // 加入焦点组
 * ============================================================ */
void create_common_slider(lv_obj_t *parent,
                          lv_obj_t **slider,
                          lv_obj_t **label,
                          lv_coord_t w, lv_coord_t h,
                          lv_coord_t x, lv_coord_t y,
                          lv_coord_t m, lv_coord_t n,
                          lv_align_t obj_align,
                          lv_align_t label_align,
                          int min, int max,
                          uint16_t init_val,
                          uint16_t *value_ptr,
                          const char *fmt)
{
    /* ---- 创建滑块 ---- */
    *slider = lv_slider_create(parent);
    lv_obj_set_size(*slider, w, h);
    lv_obj_align(*slider, obj_align, x, y);
    lv_slider_set_range(*slider, min, max);
    lv_slider_set_value(*slider, init_val, LV_ANIM_OFF);

    /* ---- 创建标签(显示当前值) ---- */
    *label = lv_label_create(parent);
    lv_obj_set_style_text_font(*label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align_to(*label, *slider, label_align, m, n);
    lv_label_set_text_fmt(*label, fmt, init_val);

    /* ---- 分配用户数据 ---- */
    slider_t *slider_data = (slider_t *)lv_mem_alloc(sizeof(slider_t));
    slider_data->label        = *label;        /* 标签对象 */
    slider_data->value        = value_ptr;     /* 预览写回地址 */
    slider_data->fmt          = fmt;           /* 格式串 */
    slider_data->link_slider  = NULL;          /* 无联动(由调用方设置) */
    slider_data->limit_type   = SLIDER_NONE;   /* 无限制(由调用方设置) */
    slider_data->param_id     = PARAM_AC_ON;   /* 默认值(由调用方覆盖) */
    slider_data->range_max    = (uint16_t)max; /* 记录创建时上限 */
    slider_data->origin_val   = init_val;      /* 初始原始值 */

    lv_obj_set_user_data(*slider, slider_data);

    /* ---- 注册销毁回调(自动释放 slider_data) ---- */
    lv_obj_add_event_cb(*slider, slider_mem_free_cb, LV_EVENT_DELETE, NULL);
}


/* ============================================================
 *  创建开关滑块(ON/OFF)
 *
 *  与 create_common_slider 的区别:
 *    - 范围固定 0~1
 *    - 标签格式: "xxx: ON" / "xxx: OFF"
 *
 *  其余行为完全相同
 * ============================================================ */
void create_slider_switch(lv_obj_t *parent,
                          lv_obj_t **slider,
                          lv_obj_t **label,
                          lv_coord_t w, lv_coord_t h,
                          lv_coord_t x, lv_coord_t y,
                          lv_coord_t m, lv_coord_t n,
                          lv_align_t obj_align,
                          lv_align_t label_align,
                          uint8_t init_val,
                          uint16_t *value_ptr,
                          const char *fmt)
{
    /* ---- 创建滑块(范围 0~1) ---- */
    *slider = lv_slider_create(parent);
    lv_obj_set_size(*slider, w, h);
    lv_obj_align(*slider, obj_align, x, y);
    lv_slider_set_range(*slider, 0, 1);
    lv_slider_set_value(*slider, init_val, LV_ANIM_OFF);

    /* ---- 创建标签(显示 ON/OFF) ---- */
    *label = lv_label_create(parent);
    lv_obj_set_style_text_font(*label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align_to(*label, *slider, label_align, m, n);
    lv_label_set_text_fmt(*label, "%s: %s", fmt, init_val ? "ON" : "OFF");

    /* ---- 分配用户数据 ---- */
    slider_t *slider_data = (slider_t *)lv_mem_alloc(sizeof(slider_t));
    slider_data->label        = *label;
    slider_data->value        = value_ptr;
    slider_data->fmt          = fmt;
    slider_data->link_slider  = NULL;
    slider_data->limit_type   = SLIDER_NONE;
    slider_data->param_id     = PARAM_AC_AUTO;  /* 默认值(由调用方覆盖) */
    slider_data->range_max    = 1;
    slider_data->origin_val   = init_val;

    lv_obj_set_user_data(*slider, slider_data);

    /* ---- 注册销毁回调 ---- */
    lv_obj_add_event_cb(*slider, slider_mem_free_cb, LV_EVENT_DELETE, NULL);
}

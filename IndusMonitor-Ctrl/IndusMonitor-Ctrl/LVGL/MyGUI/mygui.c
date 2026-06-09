/**
 ******************************************************************************
 * @file    mygui.c
 * @brief   LVGL 界面实现
 *
 *  页面结构:
 *    主页 (main_page)  ── 4个传感器按钮(温/湿/烟/光)
 *       │
 *       └── 点击 → 子页面 (new_page) ── 阈值滑块 + 自动开关 + 返回按钮
 *
 *  数据修改流程:
 *    slider ENTER确认 → DeviceData_SetParam(param_id, val) → 单字段写入g_dev
 *
 *  线程安全:
 *    LVGL 操作在 lv_timer_task 和 updata_task 中通过 lvgl_mutex 互斥
 *    DeviceData_SetParam 内部有 device_param_mutex 保护
 ******************************************************************************
 */

#include "mygui.h"
#include "Public_Data.h"
#include "personal_Data.h"

/* ============================================================
 *  外部变量
 * ============================================================ */
extern lv_indev_t *indev_keypad;    /* 按键输入设备 */

/* ============================================================
 *  样式 & 页面对象
 * ============================================================ */
static lv_style_t my_btn_style;     /* 传感器按钮公共样式 */

static lv_obj_t *main_page;        /* 主页容器 */
static lv_obj_t *new_page;         /* 子页面容器 */

/* 主页4个传感器按钮 */
lv_obj_t *temp_btn_main  = NULL;
lv_obj_t *humd_btn_main  = NULL;
lv_obj_t *smoke_btn_main = NULL;
lv_obj_t *light_btn_main = NULL;

/* 主页数据标签(显示实时数值) */
static lv_obj_t *label_temp_main;
static lv_obj_t *label_humd_main;
static lv_obj_t *label_smoke_main;
static lv_obj_t *label_light_main;

/* 子页面数据标签(进入子页面后显示实时数值) */
static lv_obj_t *label_temp_sub  = NULL;
static lv_obj_t *label_humd_sub  = NULL;
static lv_obj_t *label_smoke_sub = NULL;
static lv_obj_t *label_light_sub = NULL;

/* 记录主页中最后聚焦的按钮(返回时恢复焦点) */
static lv_obj_t *last_focused_btn = NULL;

/* 按键组(管理键盘焦点切换) */
lv_group_t *btn_group = NULL;

/* 外部字体声明(图标字体，由 create_sensor_btn 使用) */
LV_FONT_DECLARE(temp)
LV_FONT_DECLARE(humd)
LV_FONT_DECLARE(smoke)
LV_FONT_DECLARE(light)

/* ============================================================
 *  子页面滑块静态变量(页面生命周期内持久存在)
 * ============================================================ */

/* --- 温度子页面 --- */
static lv_obj_t *temp_max_slider        = NULL;
static lv_obj_t *temp_max_slider_label  = NULL;
static lv_obj_t *temp_min_slider        = NULL;
static lv_obj_t *temp_min_slider_label  = NULL;
static lv_obj_t *temp_control_slider        = NULL;
static lv_obj_t *temp_control_slider_label  = NULL;

/* --- 湿度子页面 --- */
static lv_obj_t *humd_max_slider        = NULL;
static lv_obj_t *humd_max_slider_label  = NULL;
static lv_obj_t *humd_min_slider        = NULL;
static lv_obj_t *humd_min_slider_label  = NULL;
static lv_obj_t *humd_control_slider        = NULL;
static lv_obj_t *humd_control_slider_label  = NULL;

/* --- 烟雾子页面 --- */
static lv_obj_t *smoke_max_slider       = NULL;
static lv_obj_t *smoke_max_slider_label = NULL;
static lv_obj_t *smoke_control_slider       = NULL;
static lv_obj_t *smoke_control_slider_label = NULL;

/* --- 灯光子页面 --- */
static lv_obj_t *light_max_slider       = NULL;
static lv_obj_t *light_max_slider_label = NULL;
static lv_obj_t *light_control_slider       = NULL;
static lv_obj_t *light_control_slider_label = NULL;


/* ============================================================
 *  初始化: LCD + LVGL + 输入设备
 * ============================================================ */
void gui_init(void)
{
    LCD_Init();
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
}


/* ============================================================
 *  返回按钮回调
 *  功能: 从子页面返回主页，恢复焦点，刷新主页数据
 * ============================================================ */
static void back_btn_click_event_cb(lv_event_t *e)
{
    lv_obj_t *back_btn = lv_event_get_target(e);
    lv_group_remove_obj(back_btn);          /* 从焦点组移除返回键 */

    /* 切换页面可见性 */
    lv_obj_clear_flag(main_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(new_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clean(new_page);                 /* 销毁子页面所有对象 */

    /* 清空子页面标签指针(对象已被销毁) */
    label_temp_sub  = NULL;
    label_humd_sub  = NULL;
    label_smoke_sub = NULL;
    label_light_sub = NULL;

    /* 刷新主页标签(读取最新传感器数据) */
    text_updata();

    /* 恢复主页焦点到上次聚焦的按钮 */
    if (last_focused_btn != NULL) {
        lv_group_focus_obj(last_focused_btn);
    } else {
        lv_group_focus_obj(temp_btn_main);
    }
}


/* ============================================================
 *  数值滑块回调(温度/湿度/烟雾/灯光的阈值滑块共用)
 *
 *  操作流程:
 *    聚焦 → 保存原始值
 *    左/右 → 实时预览值变化(仅在编辑模式下生效)
 *    ENTER → 进入/退出编辑模式
 *    退出编辑 → 确认保存 → DeviceData_SetParam 单字段写入
 *
 *  联动机制:
 *    MAX滑块和MIN滑块互相限制范围，防止交叉
 * ============================================================ */
static void slider_key_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    slider_t *sdata = (slider_t *)lv_obj_get_user_data(slider);

    if (sdata == NULL) return;

    /* ---- 聚焦事件: 记录原始值(取消编辑时恢复用) ---- */
    if (code == LV_EVENT_FOCUSED) {
        sdata->origin_val = lv_slider_get_value(slider);
        lv_event_stop_processing(e);
        return;
    }

    /* 过滤无关事件 */
    if (code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_KEY)
        return;

    /* ---- 值变化事件: 实时预览 ---- */
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        /* 非编辑模式下，不允许修改值，强制恢复原始值 */
        if (!lv_obj_has_state(slider, LV_STATE_EDITED)) {
            lv_slider_set_value(slider, sdata->origin_val, LV_ANIM_OFF);
            return;
        }

        uint16_t val = lv_slider_get_value(slider);

        /* 联动限制: 确保 MAX > MIN 或 MIN < MAX */
        if (sdata->link_slider != NULL) {
            slider_t *link = (slider_t *)lv_obj_get_user_data(sdata->link_slider);
            if (link != NULL) {
                uint16_t link_val = *link->value;

                /* MAX滑块不能低于MIN滑块的值+1 */
                if (sdata->limit_type == SLIDER_MAX_LIMIT && val <= link_val) {
                    val = link_val + 1;
                    lv_slider_set_value(slider, val, LV_ANIM_OFF);
                }
                /* MIN滑块不能高于MAX滑块的值-1 */
                if (sdata->limit_type == SLIDER_MIN_LIMIT && val >= link_val) {
                    val = link_val - 1;
                    lv_slider_set_value(slider, val, LV_ANIM_OFF);
                }
            }
        }

        /* 更新预览标签 */
        lv_label_set_text_fmt(sdata->label, sdata->fmt, val);
        *sdata->value = val;
    }
    /* ---- 按键事件: ENTER 切换编辑/确认 ---- */
    else if (code == LV_EVENT_KEY)
    {
        uint32_t key = lv_event_get_key(e);
        if (key != LV_KEY_ENTER) return;

        if (!lv_obj_has_state(slider, LV_STATE_EDITED))
        {
            /* 第一次ENTER: 进入编辑模式，用户可左右调值 */
            lv_obj_add_state(slider, LV_STATE_EDITED);
        }
        else
        {
            /* 第二次ENTER: 退出编辑，确认保存 */
            lv_obj_clear_state(slider, LV_STATE_EDITED);
            uint16_t real_val = *sdata->value;

            /* 确保显示最终确认值 */
            lv_slider_set_value(slider, real_val, LV_ANIM_OFF);
            lv_label_set_text_fmt(sdata->label, sdata->fmt, real_val);

            /* 写入全局参数(单字段，互斥锁保护) */
            DeviceData_SetParam(sdata->param_id, (int32_t)real_val);
        }

        /* 联动范围更新: 改变关联滑块的可调范围 */
        if (sdata->link_slider != NULL && sdata->limit_type != SLIDER_NONE)
        {
            if (sdata->limit_type == SLIDER_MAX_LIMIT) {
                /* MAX值变更 → 更新MIN滑块的上限 */
                lv_slider_set_range(sdata->link_slider, 0, *sdata->value);
            }
            else if (sdata->limit_type == SLIDER_MIN_LIMIT) {
                /* MIN值变更 → 更新MAX滑块的下限 */
                slider_t *link = (slider_t *)lv_obj_get_user_data(sdata->link_slider);
                uint16_t upper = (link != NULL) ? link->range_max : 100;
                lv_slider_set_range(sdata->link_slider, *sdata->value, upper);
            }
        }
    }
}


/* ============================================================
 *  开关滑块回调(自动模式 ON/OFF 共用)
 *
 *  操作流程:
 *    聚焦 → 保存原始值
 *    左/右 → 切换 ON/OFF (仅在编辑模式下生效)
 *    ENTER → 进入/退出编辑模式
 *    退出编辑 → 确认保存 → DeviceData_SetParam 单字段写入
 * ============================================================ */
static void slider_switch_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    slider_t *sdata = (slider_t *)lv_obj_get_user_data(slider);

    if (sdata == NULL) return;

    /* ---- 聚焦事件: 记录原始值 ---- */
    if (code == LV_EVENT_FOCUSED) {
        sdata->origin_val = lv_slider_get_value(slider);
        lv_event_stop_processing(e);
        return;
    }

    if (code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_KEY)
        return;

    /* ---- 值变化: 更新ON/OFF显示 ---- */
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        if (!lv_obj_has_state(slider, LV_STATE_EDITED)) {
            lv_slider_set_value(slider, sdata->origin_val, LV_ANIM_OFF);
            return;
        }
        uint16_t val = lv_slider_get_value(slider);
        lv_label_set_text_fmt(sdata->label, "%s: %s", sdata->fmt, val ? "ON" : "OFF");
        *sdata->value = val;
    }
    /* ---- ENTER键: 切换编辑/确认 ---- */
    else if (code == LV_EVENT_KEY)
    {
        uint32_t key = lv_event_get_key(e);
        if (key != LV_KEY_ENTER) return;

        if (!lv_obj_has_state(slider, LV_STATE_EDITED)) {
            lv_obj_add_state(slider, LV_STATE_EDITED);
        }
        else
        {
            lv_obj_clear_state(slider, LV_STATE_EDITED);
            uint16_t real_val = *sdata->value;

            lv_slider_set_value(slider, real_val, LV_ANIM_OFF);
            lv_label_set_text_fmt(sdata->label, "%s: %s",
                                  sdata->fmt, real_val ? "ON" : "OFF");

            /* 写入全局参数 */
            DeviceData_SetParam(sdata->param_id, (int32_t)real_val);
        }
    }
}


/* ============================================================
 *  子页面 — 温度
 *
 *  布局:
 *    [温度图标+实时值]    ← 左上角
 *    [高温阈值滑块]       ← 右侧，范围 AC_OFF ~ 40
 *    [低温阈值滑块]       ← 右侧，范围 0 ~ AC_ON
 *    [自动模式开关]       ← 居中
 *
 *  联动: 高温滑块(MAX) ↔ 低温滑块(MIN) 互相限制范围
 * ============================================================ */
static void temp_btn_click_event_cb(lv_obj_t *parent_page)
{
		static DeviceStatus dev;       
    dev = DeviceData_Read();

    /* 温度图标按钮 + 子页面实时标签 */
    lv_obj_t *temp_btn_new = NULL;
    create_sensor_btn(parent_page, &temp_btn_new, 10, 10, 100, 80,
                      &my_btn_style, MY_TEMP_SYMBOL, &temp);
    create_common_label(&label_temp_sub, temp_btn_new);

    /* 清空其他子页面标签指针(text_updata只更新当前页面的标签) */
    label_humd_sub  = NULL;
    label_smoke_sub = NULL;
    label_light_sub = NULL;

    /* 立即刷新标签显示最新温度 */
    text_updata();

    /* ---- 高温阈值滑块 (空调开启温度) ---- */
    create_common_slider(parent_page,
                         &temp_max_slider, &temp_max_slider_label,
                         15, 120, -58, -20, 5, -20,
                         LV_ALIGN_RIGHT_MID, LV_ALIGN_OUT_TOP_MID,
                         dev.AC_OFF_VALUE, 40,       /* 范围: 低温阈值 ~ 40°C */
                         dev.AC_ON_VALUE,            /* 当前值 */
                         &dev.AC_ON_VALUE,            /* 写回地址(预览用) */
                         "%d°C");

    slider_t *s_max = lv_obj_get_user_data(temp_max_slider);
    s_max->param_id  = PARAM_AC_ON;     /* 对应 g_dev.AC_ON_VALUE */
    s_max->range_max = 40;              /* 记录上限(供MIN联动使用) */
    lv_obj_add_event_cb(temp_max_slider, slider_key_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(btn_group, temp_max_slider);

    /* ---- 低温阈值滑块 (空调关闭温度) ---- */
    create_common_slider(parent_page,
                         &temp_min_slider, &temp_min_slider_label,
                         15, 120, -10, -20, 5, -20,
                         LV_ALIGN_RIGHT_MID, LV_ALIGN_OUT_TOP_MID,
                         0, dev.AC_ON_VALUE,         /* 范围: 0 ~ 高温阈值 */
                         dev.AC_OFF_VALUE,
                         &dev.AC_OFF_VALUE,
                         "%d°C");

    slider_t *s_min = lv_obj_get_user_data(temp_min_slider);
    s_min->param_id  = PARAM_AC_OFF;    /* 对应 g_dev.AC_OFF_VALUE */
    s_min->range_max = 40;
    lv_obj_add_event_cb(temp_min_slider, slider_key_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(btn_group, temp_min_slider);

    /* ---- 联动关系 ---- */
    slider_t *max_d = lv_obj_get_user_data(temp_max_slider);
    max_d->link_slider = temp_min_slider;
    max_d->limit_type  = SLIDER_MAX_LIMIT;  /* MAX不能低于MIN */

    slider_t *min_d = lv_obj_get_user_data(temp_min_slider);
    min_d->link_slider = temp_max_slider;
    min_d->limit_type  = SLIDER_MIN_LIMIT;  /* MIN不能高于MAX */

    /* ---- 温控自动模式开关 ---- */
    create_slider_switch(parent_page,
                         &temp_control_slider, &temp_control_slider_label,
                         70, 30, -40, 40, -20, 0,
                         LV_ALIGN_CENTER, LV_ALIGN_OUT_BOTTOM_MID,
                         dev.AC_AUTO_MODE,
                         &dev.AC_AUTO_MODE,
                         "temp_auto");

    slider_t *s_sw = lv_obj_get_user_data(temp_control_slider);
    s_sw->param_id = PARAM_AC_AUTO;     /* 对应 g_dev.AC_AUTO_MODE */
    lv_obj_add_event_cb(temp_control_slider, slider_switch_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(btn_group, temp_control_slider);
}


/* ============================================================
 *  子页面 — 湿度
 *  结构与温度子页面相同，联动: 高湿(MAX) ↔ 低湿(MIN)
 * ============================================================ */
static void humd_btn_click_event_cb(lv_obj_t *parent_page)
{
		static DeviceStatus dev;       
    dev = DeviceData_Read();

    /* 湿度图标 + 标签 */
    lv_obj_t *humd_btn_new = NULL;
    create_sensor_btn(parent_page, &humd_btn_new, 10, 10, 100, 80,
                      &my_btn_style, MY_HUMD_SYMBOL, &humd);
    create_common_label(&label_humd_sub, humd_btn_new);

    label_temp_sub  = NULL;
    label_smoke_sub = NULL;
    label_light_sub = NULL;
    text_updata();

    /* ---- 高湿阈值 (加湿器开启湿度) ---- */
    create_common_slider(parent_page,
                         &humd_max_slider, &humd_max_slider_label,
                         15, 120, -58, -20, 5, -20,
                         LV_ALIGN_RIGHT_MID, LV_ALIGN_OUT_TOP_MID,
                         dev.HUMIDIFIER_OFF_VALUE, 100,   /* 范围: 低湿阈值 ~ 100 */
                         dev.HUMIDIFIER_ON_VALUE,
                         &dev.HUMIDIFIER_ON_VALUE,
                         "%dRH");

    slider_t *s_max = lv_obj_get_user_data(humd_max_slider);
    s_max->param_id  = PARAM_HUMI_ON;
    s_max->range_max = 100;               /* 湿度上限100，非40 */
    lv_obj_add_event_cb(humd_max_slider, slider_key_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(btn_group, humd_max_slider);

    /* ---- 低湿阈值 (加湿器关闭湿度) ---- */
    create_common_slider(parent_page,
                         &humd_min_slider, &humd_min_slider_label,
                         15, 120, -10, -20, 5, -20,
                         LV_ALIGN_RIGHT_MID, LV_ALIGN_OUT_TOP_MID,
                         0, dev.HUMIDIFIER_ON_VALUE,
                         dev.HUMIDIFIER_OFF_VALUE,
                         &dev.HUMIDIFIER_OFF_VALUE,
                         "%dRH");

    slider_t *s_min = lv_obj_get_user_data(humd_min_slider);
    s_min->param_id  = PARAM_HUMI_OFF;
    s_min->range_max = 100;
    lv_obj_add_event_cb(humd_min_slider, slider_key_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(btn_group, humd_min_slider);

    /* ---- 联动关系 ---- */
    slider_t *max_d = lv_obj_get_user_data(humd_max_slider);
    max_d->link_slider = humd_min_slider;
    max_d->limit_type  = SLIDER_MAX_LIMIT;

    slider_t *min_d = lv_obj_get_user_data(humd_min_slider);
    min_d->link_slider = humd_max_slider;
    min_d->limit_type  = SLIDER_MIN_LIMIT;

    /* ---- 湿控自动模式开关 ---- */
    create_slider_switch(parent_page,
                         &humd_control_slider, &humd_control_slider_label,
                         70, 30, -40, 40, -20, 0,
                         LV_ALIGN_CENTER, LV_ALIGN_OUT_BOTTOM_MID,
                         dev.HUMI_AUTO_MODE,
                         &dev.HUMI_AUTO_MODE,
                         "humd_auto");

    slider_t *s_sw = lv_obj_get_user_data(humd_control_slider);
    s_sw->param_id = PARAM_HUMI_AUTO;
    lv_obj_add_event_cb(humd_control_slider, slider_switch_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(btn_group, humd_control_slider);
}


/* ============================================================
 *  子页面 — 烟雾
 *  无联动关系，只有一个报警阈值滑块 + 自动开关
 * ============================================================ */
static void smoke_btn_click_event_cb(lv_obj_t *parent_page)
{
		static DeviceStatus dev;       
    dev = DeviceData_Read();

    /* 烟雾图标 + 标签 */
    lv_obj_t *smoke_btn_new = NULL;
    create_sensor_btn(parent_page, &smoke_btn_new, 10, 10, 100, 80,
                      &my_btn_style, MY_SMOKE_SYMBOL, &smoke);
    create_common_label(&label_smoke_sub, smoke_btn_new);

    label_temp_sub  = NULL;
    label_humd_sub  = NULL;
    label_light_sub = NULL;
    text_updata();

    /* ---- 烟雾报警阈值 (ppm) ---- */
    create_common_slider(parent_page,
                         &smoke_max_slider, &smoke_max_slider_label,
                         15, 120, 0, 0, 5, -20,
                         LV_ALIGN_RIGHT_MID, LV_ALIGN_OUT_TOP_MID,
                         500, 2000,                    /* 范围: 500 ~ 2000 ppm */
                         dev.SMOKE_ALARM_VALUE,
                         &dev.SMOKE_ALARM_VALUE,
                         "%dppm");

    slider_t *s_max = lv_obj_get_user_data(smoke_max_slider);
    s_max->param_id    = PARAM_SMOKE_ALARM;
    s_max->range_max   = 2000;
    s_max->link_slider = NULL;               /* 无联动 */
    s_max->limit_type  = SLIDER_NONE;
    lv_obj_add_event_cb(smoke_max_slider, slider_key_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(btn_group, smoke_max_slider);

    /* ---- 烟雾自动模式开关 ---- */
    create_slider_switch(parent_page,
                         &smoke_control_slider, &smoke_control_slider_label,
                         70, 30, -40, 40, -20, 0,
                         LV_ALIGN_CENTER, LV_ALIGN_OUT_BOTTOM_MID,
                         dev.SMOKE_AUTO_MODE,
                         &dev.SMOKE_AUTO_MODE,
                         "smoke_auto");

    slider_t *s_sw = lv_obj_get_user_data(smoke_control_slider);
    s_sw->param_id = PARAM_SMOKE_AUTO;
    lv_obj_add_event_cb(smoke_control_slider, slider_switch_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(btn_group, smoke_control_slider);
}


/* ============================================================
 *  子页面 — 灯光
 *  无联动关系，只有一个亮度滑块 + 自动开关
 * ============================================================ */
static void light_btn_click_event_cb(lv_obj_t *parent_page)
{
		static DeviceStatus dev;       
    dev = DeviceData_Read();

    /* 灯光图标 + 标签 */
    lv_obj_t *light_btn_new = NULL;
    create_sensor_btn(parent_page, &light_btn_new, 10, 10, 100, 80,
                      &my_btn_style, MY_LIGHT_SYMBOL, &light);
    create_common_label(&label_light_sub, light_btn_new);

    label_temp_sub  = NULL;
    label_humd_sub  = NULL;
    label_smoke_sub = NULL;
    text_updata();

    /* ---- 灯光亮度阈值 (%) ---- */
    create_common_slider(parent_page,
                         &light_max_slider, &light_max_slider_label,
                         15, 120, 0, 0, 5, -20,
                         LV_ALIGN_RIGHT_MID, LV_ALIGN_OUT_TOP_MID,
                         0, 100,                       /* 范围: 0 ~ 100% */
                         dev.LIGHT_BRIGHT_VALUE,
                         &dev.LIGHT_BRIGHT_VALUE,
                         "%d%%");

    slider_t *s_max = lv_obj_get_user_data(light_max_slider);
    s_max->param_id    = PARAM_LIGHT_BRIGHT;
    s_max->range_max   = 100;
    s_max->link_slider = NULL;
    s_max->limit_type  = SLIDER_NONE;
    lv_obj_add_event_cb(light_max_slider, slider_key_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(btn_group, light_max_slider);

    /* ---- 灯光自动模式开关 ---- */
    create_slider_switch(parent_page,
                         &light_control_slider, &light_control_slider_label,
                         70, 30, -40, 40, -20, 0,
                         LV_ALIGN_CENTER, LV_ALIGN_OUT_BOTTOM_MID,
                         dev.LIGHT_AUTO_MODE,
                         &dev.LIGHT_AUTO_MODE,
                         "light_auto");

    slider_t *s_sw = lv_obj_get_user_data(light_control_slider);
    s_sw->param_id = PARAM_LIGHT_AUTO;
    lv_obj_add_event_cb(light_control_slider, slider_switch_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(btn_group, light_control_slider);
}


/* ============================================================
 *  主页按钮点击回调 → 进入对应子页面
 *
 *  流程:
 *    1. 记录当前聚焦按钮
 *    2. 隐藏主页，显示子页面
 *    3. 清空子页面旧内容，调用对应子页面创建函数
 *    4. 创建返回按钮
 * ============================================================ */
static void btn_click_event_cb(lv_event_t *e)
{
    lv_obj_t *clicked_btn = lv_event_get_target(e);
    last_focused_btn = clicked_btn;         /* 记住从哪个按钮进来的 */

    /* 页面切换 */
    lv_obj_add_flag(main_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(new_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clean(new_page);                 /* 清除旧子页面内容 */

    /* 根据点击的按钮创建对应子页面 */
    if (clicked_btn == temp_btn_main) {
        temp_btn_click_event_cb(new_page);
    } else if (clicked_btn == humd_btn_main) {
        humd_btn_click_event_cb(new_page);
    } else if (clicked_btn == smoke_btn_main) {
        smoke_btn_click_event_cb(new_page);
    } else if (clicked_btn == light_btn_main) {
        light_btn_click_event_cb(new_page);
    }

    /* ---- 创建返回按钮(右下角，透明风格) ---- */
    lv_obj_t *back_btn = lv_btn_create(new_page);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_size(back_btn, 30, 30);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_opa(back_btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(back_btn, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_set_align(back_label, LV_ALIGN_CENTER);

    lv_obj_add_event_cb(back_btn, back_btn_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(btn_group, back_btn);  /* 加入焦点组，支持键盘导航 */
}


/* ============================================================
 *  主界面构建
 *
 *  布局:
 *    ┌──────────────────────────┐
 *    │  [WiFi状态]              │  ← y=10  (create_top_btn)
 *    │                          │
 *    │  [温度]  [湿度]          │  ← y=60
 *    │  (数值)  (数值)          │
 *    │                          │
 *    │  [烟雾]  [灯光]          │  ← y=150
 *    │  (数值)  (数值)          │
 *    │                          │
 *    └──────────────────────────┘
 * ============================================================ */
void my_GUI(void)
{
    /* ---- 按钮公共样式: 白底、圆角、浅灰边框 ---- */
    lv_style_init(&my_btn_style);
    lv_style_set_bg_color(&my_btn_style, lv_color_hex(0xFFFFFF));
    lv_style_set_radius(&my_btn_style, 8);
    lv_style_set_border_width(&my_btn_style, 1);
    lv_style_set_border_color(&my_btn_style, lv_color_hex(0xCCCCCC));

    /* ---- 主页容器 ---- */
    main_page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_page, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(main_page, lv_color_hex(0xEEEEEE), LV_PART_MAIN);
    lv_obj_set_style_pad_all(main_page, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(main_page, 0, LV_PART_MAIN);

    /* ---- 子页面容器(默认隐藏) ---- */
    new_page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(new_page, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(new_page, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_add_flag(new_page, LV_OBJ_FLAG_HIDDEN);

    /* ---- 创建4个传感器按钮 ---- */
    create_sensor_btn(main_page, &temp_btn_main,  20,  60,  100, 80, &my_btn_style, MY_TEMP_SYMBOL,  &temp);
    create_sensor_btn(main_page, &humd_btn_main,  130, 60,  100, 80, &my_btn_style, MY_HUMD_SYMBOL,  &humd);
    create_sensor_btn(main_page, &smoke_btn_main, 20,  150, 100, 80, &my_btn_style, MY_SMOKE_SYMBOL, &smoke);
    create_sensor_btn(main_page, &light_btn_main, 130, 150, 100, 80, &my_btn_style, MY_LIGHT_SYMBOL, &light);

    /* ---- 绑定点击事件(共用同一个回调，内部判断是哪个按钮) ---- */
    lv_obj_add_event_cb(temp_btn_main,  btn_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(humd_btn_main,  btn_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(smoke_btn_main, btn_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(light_btn_main, btn_click_event_cb, LV_EVENT_CLICKED, NULL);

    /* ---- 创建焦点组(键盘上下键切换焦点) ---- */
    btn_group = lv_group_create();
    lv_group_add_obj(btn_group, temp_btn_main);
    lv_group_add_obj(btn_group, humd_btn_main);
    lv_group_add_obj(btn_group, smoke_btn_main);
    lv_group_add_obj(btn_group, light_btn_main);
    lv_indev_set_group(indev_keypad, btn_group);

    /* 默认聚焦温度按钮 */
    last_focused_btn = temp_btn_main;
    lv_group_focus_obj(last_focused_btn);
}


/* ============================================================
 *  WiFi 状态指示按钮(主页右上角)
 *  wifi_value=0: 仅显示WiFi图标
 *  wifi_value=1: WiFi图标上叠加X号，表示未连接
 * ============================================================ */
void create_top_btn(uint8_t wifi_value)
{
    lv_obj_t *top_btn = lv_btn_create(main_page);
    lv_obj_set_size(top_btn, 90, 30);
    lv_obj_set_pos(top_btn, 140, 10);

    /* WiFi图标 */
    lv_obj_t *wifi_label = lv_label_create(top_btn);
    lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(wifi_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_align(wifi_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    /* 未连接时叠加X号 */
    if (wifi_value == 1) {
        lv_obj_t *wifi_close_label = lv_label_create(top_btn);
        lv_label_set_text(wifi_close_label, LV_SYMBOL_CLOSE);
        lv_obj_set_style_text_font(wifi_close_label, &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(wifi_close_label, lv_color_black(), LV_PART_MAIN);
        lv_obj_align_to(wifi_close_label, wifi_label, LV_ALIGN_BOTTOM_RIGHT, 5, 2);
    }
}


/* ============================================================
 *  创建主页数据标签(4个标签分别对齐到4个按钮下方)
 *  注意: 函数名 create_temp_label 不够准确，实际创建了所有4个标签
 * ============================================================ */
void create_temp_label(void)
{
    label_temp_main  = lv_label_create(main_page);
    label_humd_main  = lv_label_create(main_page);
    label_smoke_main = lv_label_create(main_page);
    label_light_main = lv_label_create(main_page);

    /* 统一设置字体和颜色 */
    lv_obj_set_style_text_font(label_temp_main,  &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_font(label_humd_main,  &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_font(label_smoke_main, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_font(label_light_main, &lv_font_montserrat_16, 0);

    lv_obj_set_style_text_color(label_temp_main,  lv_color_black(), 0);
    lv_obj_set_style_text_color(label_humd_main,  lv_color_black(), 0);
    lv_obj_set_style_text_color(label_smoke_main, lv_color_black(), 0);
    lv_obj_set_style_text_color(label_light_main, lv_color_black(), 0);

    /* 对齐到各自按钮下方 */
    lv_obj_align_to(label_temp_main,  temp_btn_main,  LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_align_to(label_humd_main,  humd_btn_main,  LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_align_to(label_smoke_main, smoke_btn_main, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_align_to(label_light_main, light_btn_main, LV_ALIGN_BOTTOM_MID, 0, 0);
}


/* ============================================================
 *  刷新所有可见标签(主页 + 当前子页面)
 *  由 updata_task 每2秒调用一次，也由返回按钮/进入子页面时调用
 *
 *  线程安全: 调用方需持有 lvgl_mutex (freertos.c 中已处理)
 * ============================================================ */
void text_updata(void)
{
    /* 读取最新传感器数据 */
    float temp_val, humd_val;
    EnvData_Get(&temp_val, &humd_val);
    uint16_t smoke_val;
	  SmokeData_GetValue(&smoke_val);

    /* 主页标签(始终更新) */
    if (label_temp_main != NULL)
        lv_label_set_text_fmt(label_temp_main,  "%d°C",  (int)temp_val);
    if (label_humd_main != NULL)
        lv_label_set_text_fmt(label_humd_main,  "%dRH",  (int)humd_val);
    if (label_smoke_main != NULL)
        lv_label_set_text_fmt(label_smoke_main, "%dppm", smoke_val);
    if (label_light_main != NULL)
        lv_label_set_text_fmt(label_light_main, "%dlx",  0);  /* 光照传感器待接入 */

    /* 子页面标签(仅当前页面对应的标签非NULL) */
    if (label_temp_sub != NULL)
        lv_label_set_text_fmt(label_temp_sub,  "%d°C",  (int)temp_val);
    if (label_humd_sub != NULL)
        lv_label_set_text_fmt(label_humd_sub,  "%dRH",  (int)humd_val);
    if (label_smoke_sub != NULL)
        lv_label_set_text_fmt(label_smoke_sub, "%dppm", smoke_val);
    if (label_light_sub != NULL)
        lv_label_set_text_fmt(label_light_sub, "%dlx",  0);
}

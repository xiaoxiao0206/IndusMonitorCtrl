#ifndef __USER_H_
#define __USER_H_

#include "main.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lcd.h"
#include "lv_port_indev.h"
#include "usart.h"
#include "personal_Data.h"

/* ============================================================
 *  滑块联动限制类型
 *
 *  SLIDER_NONE      : 无联动
 *  SLIDER_MAX_LIMIT : 当前滑块是MAX，不能低于关联MIN滑块的值
 *  SLIDER_MIN_LIMIT : 当前滑块是MIN，不能高于关联MAX滑块的值
 * ============================================================ */
typedef enum {
    SLIDER_NONE,        /* 无联动 */
    SLIDER_MAX_LIMIT,   /* 作为最大值滑块，受限于最小值滑块 */
    SLIDER_MIN_LIMIT    /* 作为最小值滑块，受限于最大值滑块 */
} slider_limit_type_t;

/* ============================================================
 *  滑块用户数据结构
 *
 *  生命周期: 随 lv_obj 动态分配(lv_mem_alloc)
 *            随 lv_obj 删除自动释放(slider_mem_free_cb)
 *
 *  数据流:
 *    创建时 → slider_data->value = value_ptr (预览用)
 *    调值时 → *value = val (实时预览)
 *    ENTER确认 → DeviceData_SetParam(param_id, val) (写入g_dev)
 * ============================================================ */
typedef struct {
    lv_obj_t            *label;         /* 滑块旁的显示标签 */
    uint16_t            *value;         /* 指向 static dev 的字段(预览用) */
    const char          *fmt;           /* 标签格式串, 如 "%d°C" */
    lv_obj_t            *link_slider;   /* 联动的目标滑块(无联动时为NULL) */
    slider_limit_type_t  limit_type;    /* 联动限制类型 */
    ParamId              param_id;      /* 对应 g_dev 的字段ID(ENTER确认时写入) */
    uint16_t             range_max;     /* 创建时的范围上限(MIN联动时使用) */
    uint16_t             origin_val;    /* 聚焦时的原始值(取消编辑时恢复用) */
} slider_t;


/* ============================================================
 *  控件创建函数
 * ============================================================ */

/**
 * @brief  创建公共标签(对齐到父对象底部居中)
 * @param  label  输出: 创建的标签对象指针
 * @param  parent 父对象
 */
void create_common_label(lv_obj_t **label, lv_obj_t *parent);

/**
 * @brief  创建传感器按钮(含图标)
 * @param  parent 父对象(主页/子页面容器)
 * @param  btn    输出: 创建的按钮对象指针
 * @param  x,y    位置
 * @param  w,h    尺寸
 * @param  style  按钮样式
 * @param  symbol 图标符号(MY_TEMP_SYMBOL 等)
 * @param  font   图标字体
 */
void create_sensor_btn(lv_obj_t *parent,
                       lv_obj_t **btn,
                       int x, int y, int w, int h,
                       lv_style_t *style,
                       const char *symbol,
                       const lv_font_t *font);

/**
 * @brief  创建数值滑块(阈值调节)
 *
 *  创建后会自动分配 slider_t 作为 user_data
 *  并注册 DELETE 回调自动释放内存
 *
 *  使用方需在创建后补充赋值:
 *    slider_t *s = lv_obj_get_user_data(slider);
 *    s->param_id  = PARAM_XXX;
 *    s->range_max = max;
 *    lv_obj_add_event_cb(slider, slider_key_cb, ...);
 *    lv_group_add_obj(btn_group, slider);
 *
 * @param  parent    父对象
 * @param  slider    输出: 滑块对象指针
 * @param  label     输出: 滑块标签对象指针
 * @param  w,h       滑块尺寸
 * @param  x,y       滑块对齐偏移
 * @param  m,n       标签相对滑块的对齐偏移
 * @param  obj_align 滑块对齐方式
 * @param  label_align 标签对齐方式
 * @param  min,max   滑块范围
 * @param  init_val  初始值
 * @param  value_ptr 预览写回地址(指向 static dev 的字段)
 * @param  fmt       标签格式串
 */
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
                          const char *fmt);

/**
 * @brief  创建开关滑块(ON/OFF，范围固定 0~1)
 *
 *  用法与 create_common_slider 相同，
 *  区别: 范围固定0/1，标签显示 "xxx: ON/OFF"
 *
 *  使用方需在创建后补充赋值:
 *    slider_t *s = lv_obj_get_user_data(slider);
 *    s->param_id = PARAM_XXX_AUTO;
 *    lv_obj_add_event_cb(slider, slider_switch_cb, ...);
 *    lv_group_add_obj(btn_group, slider);
 *
 * @param  parent    父对象
 * @param  slider    输出: 滑块对象指针
 * @param  label     输出: 滑块标签对象指针
 * @param  w,h       滑块尺寸
 * @param  x,y       滑块对齐偏移
 * @param  m,n       标签相对滑块的对齐偏移
 * @param  obj_align 滑块对齐方式
 * @param  label_align 标签对齐方式
 * @param  init_val  初始值(0=OFF, 1=ON)
 * @param  value_ptr 预览写回地址
 * @param  fmt       标签前缀(如 "temp_auto"，显示为 "temp_auto: ON")
 */
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
                          const char *fmt);

#endif

/**
 ******************************************************************************
 * @file    personal_Data.c
 * @brief   设备参数管理实现 (线程安全)
 *
 *  设计:
 *    - g_dev 完全私有，外部只能通过接口访问
 *    - device_param_mutex 保护所有读写操作
 *    - DeviceData_SetParam: GUI slider 专用，单字段精确写入
 *    - DeviceData_Read/Write: 整包读写，用于初始化和兼容旧接口
 *
 *  调用方:
 *    mygui.c      → DeviceData_SetParam (用户修改参数)
 *                   DeviceData_Read     (进入子页面时初始化滑块)
 *    cooper_task  → DeviceData_Read     (读阈值做自动控制)
 *    freertos.c   → DeviceData_Init     (系统启动时初始化)
 ******************************************************************************
 */

#include "personal_Data.h"
#include "FreeRTOS.h"
#include "semphr.h"

/* ============================================================
 *  私有数据 (外部不可直接访问)
 * ============================================================ */
static DeviceStatus g_dev;

/* 互斥锁 (在 freertos.c 中创建) */
extern SemaphoreHandle_t device_param_mutex;


/* ============================================================
 *  初始化默认参数
 *
 *  协作控制逻辑参考:
 *    高温 >= 25°C 或 高湿 >= 50%RH → 开空调, 关加湿器
 *    低温 <= 20°C 且 低湿 <= 40%RH → 开空调制热, 开加湿器
 *    舒适区 → 全部关闭
 * ============================================================ */
void DeviceData_Init(void)
{
    g_dev = (DeviceStatus){
        .AC_ON_VALUE          = 25,       /* 空调开启温度 25°C */
        .AC_OFF_VALUE         = 20,       /* 空调关闭温度 20°C */
        .HUMIDIFIER_ON_VALUE  = 50,       /* 加湿器开启湿度 50% */
        .HUMIDIFIER_OFF_VALUE = 40,       /* 加湿器关闭湿度 40% */
        .AC_AUTO_MODE         = 1,        /* 温控默认自动 */
        .HUMI_AUTO_MODE       = 1,        /* 湿控默认自动 */
        .SMOKE_ALARM_VALUE    = 1000,     /* 烟雾报警阈值 1000ppm */
        .SMOKE_AUTO_MODE      = 1,        /* 烟雾默认自动 */
        .LIGHT_BRIGHT_VALUE   = 80,       /* 灯光亮度阈值 80% */
        .LIGHT_AUTO_MODE      = 1,        /* 灯光默认自动 */
    };
		
}


/* ============================================================
 *  整包写入 (覆盖所有字段)
 *
 *  注意: 多页面先后调用会互相覆盖，
 *        推荐使用 DeviceData_SetParam 进行单字段写入
 * ============================================================ */
void DeviceData_Write(DeviceStatus data)
{
    xSemaphoreTake(device_param_mutex, portMAX_DELAY);
    g_dev = data;
    xSemaphoreGive(device_param_mutex);
}


/* ============================================================
 *  整包读取 (返回结构体副本)
 *
 *  用途:
 *    1. 进入子页面时，用于初始化滑块范围和初始值
 *    2. cooper_task 读取阈值做自动控制判断
 *
 *  安全性: 返回的是副本，调用方修改副本不影响 g_dev
 * ============================================================ */
DeviceStatus DeviceData_Read(void)
{
    DeviceStatus tmp;
    xSemaphoreTake(device_param_mutex, portMAX_DELAY);
    tmp = g_dev;
    xSemaphoreGive(device_param_mutex);
    return tmp;
}


/* ============================================================
 *  单字段写入 (GUI slider ENTER 确认时调用)
 *
 *  优势:
 *    - 只修改一个字段，其他字段不受影响
 *    - 多页面先后修改不同字段不会互相覆盖
 *    - 持锁时间极短 (只写一个字段)
 *
 *  与 DeviceData_Write 的对比:
 *    SetParam: 精确手术，只动指定的字段
 *    Write:    整包替换，后写的覆盖先写的
 * ============================================================ */
void DeviceData_SetParam(ParamId id, int32_t val)
{
    xSemaphoreTake(device_param_mutex, portMAX_DELAY);
    switch (id)
    {
        case PARAM_AC_ON:        g_dev.AC_ON_VALUE          = (uint16_t)val; break;
        case PARAM_AC_OFF:       g_dev.AC_OFF_VALUE         = (uint16_t)val; break;
        case PARAM_AC_AUTO:      g_dev.AC_AUTO_MODE         = (uint16_t)val; break;
        case PARAM_HUMI_ON:      g_dev.HUMIDIFIER_ON_VALUE  = (uint16_t)val; break;
        case PARAM_HUMI_OFF:     g_dev.HUMIDIFIER_OFF_VALUE = (uint16_t)val; break;
        case PARAM_HUMI_AUTO:    g_dev.HUMI_AUTO_MODE       = (uint16_t)val; break;
        case PARAM_SMOKE_ALARM:  g_dev.SMOKE_ALARM_VALUE    = (uint16_t)val; break;
        case PARAM_SMOKE_AUTO:   g_dev.SMOKE_AUTO_MODE      = (uint16_t)val; break;
        case PARAM_LIGHT_BRIGHT: g_dev.LIGHT_BRIGHT_VALUE   = (uint16_t)val; break;
        case PARAM_LIGHT_AUTO:   g_dev.LIGHT_AUTO_MODE      = (uint16_t)val; break;
        default: break;     /* 无效ID不执行任何操作 */
    }
    xSemaphoreGive(device_param_mutex);
}




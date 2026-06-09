/**
 ******************************************************************************
 * @file    personal_Data.h
 * @brief   设备参数管理接口
 *
 *  管理所有设备的阈值参数和自动模式开关:
 *    - 温控: 空调开启/关闭温度阈值 + 自动模式
 *    - 湿控: 加湿器开启/关闭湿度阈值 + 自动模式
 *    - 烟雾: 报警阈值 + 自动模式
 *    - 灯光: 亮度阈值 + 自动模式
 *
 *  数据修改方式:
 *    GUI slider → DeviceData_SetParam(param_id, val)  (推荐，单字段写入)
 *    整体写入   → DeviceData_Write(data)              (兼容旧接口)
 *    整体读取   → DeviceData_Read()                   (初始化滑块用)
 ******************************************************************************
 */
#ifndef __PERSONAL_DATA_H
#define __PERSONAL_DATA_H

#include "main.h"

/* ============================================================
 *  参数ID枚举 (与 DeviceData_SetParam 配合使用)
 *
 *  每个 slider 创建时绑定一个 ParamId，
 *  ENTER 确认时通过此 ID 精确写入 g_dev 的对应字段
 * ============================================================ */
typedef enum {
    PARAM_AC_ON,            /* 0 → g_dev.AC_ON_VALUE         空调开启温度 */
    PARAM_AC_OFF,           /* 1 → g_dev.AC_OFF_VALUE        空调关闭温度 */
    PARAM_AC_AUTO,          /* 2 → g_dev.AC_AUTO_MODE        温控自动模式 */
    PARAM_HUMI_ON,          /* 3 → g_dev.HUMIDIFIER_ON_VALUE  加湿器开启湿度 */
    PARAM_HUMI_OFF,         /* 4 → g_dev.HUMIDIFIER_OFF_VALUE 加湿器关闭湿度 */
    PARAM_HUMI_AUTO,        /* 5 → g_dev.HUMI_AUTO_MODE       湿控自动模式 */
    PARAM_SMOKE_ALARM,      /* 6 → g_dev.SMOKE_ALARM_VALUE    烟雾报警阈值 */
    PARAM_SMOKE_AUTO,       /* 7 → g_dev.SMOKE_AUTO_MODE      烟雾自动模式 */
    PARAM_LIGHT_BRIGHT,     /* 8 → g_dev.LIGHT_BRIGHT_VALUE   灯光亮度阈值 */
    PARAM_LIGHT_AUTO,       /* 9 → g_dev.LIGHT_AUTO_MODE      灯光自动模式 */
} ParamId;

/* ============================================================
 *  设备参数结构体 (全局唯一实例 g_dev)
 *
 *  自动模式字段: 1=自动, 0=手动
 *  阈值字段:     控制任务读取后与传感器实时值比较
 *
 *  字段类型说明:
 *    所有字段使用 uint16_t 统一类型，
 *    原因: slider_t 的 value 指针为 uint16_t*，
 *          统一类型避免指针类型不匹配的编译警告
 * ============================================================ */
typedef struct {
    uint16_t AC_ON_VALUE;           /* 空调开启温度阈值 (°C) */
    uint16_t AC_OFF_VALUE;          /* 空调关闭温度阈值 (°C) */
    uint16_t HUMIDIFIER_ON_VALUE;   /* 加湿器开启湿度阈值 (%RH) */
    uint16_t HUMIDIFIER_OFF_VALUE;  /* 加湿器关闭湿度阈值 (%RH) */
    uint16_t AC_AUTO_MODE;          /* 温控自动模式: 1=自动, 0=手动 */
    uint16_t HUMI_AUTO_MODE;        /* 湿控自动模式: 1=自动, 0=手动 */
    uint16_t SMOKE_ALARM_VALUE;     /* 烟雾报警阈值 (ppm) */
    uint16_t SMOKE_AUTO_MODE;       /* 烟雾自动模式: 1=自动, 0=手动 */
    uint16_t LIGHT_BRIGHT_VALUE;    /* 灯光亮度阈值 (%) */
    uint16_t LIGHT_AUTO_MODE;       /* 灯光自动模式: 1=自动, 0=手动 */
} DeviceStatus;

/* ============================================================
 *  接口函数
 * ============================================================ */

/**
 * @brief  初始化默认参数 (系统启动时调用一次)
 */
void DeviceData_Init(void);

/**
 * @brief  整包写入设备参数
 * @param  data  要写入的完整参数结构体
 * @note   会覆盖所有字段，多页面场景下建议用 DeviceData_SetParam
 */
void DeviceData_Write(DeviceStatus data);

/**
 * @brief  整包读取设备参数
 * @return 当前参数的副本 (用于初始化滑块范围和初始值)
 */
DeviceStatus DeviceData_Read(void);

/**
 * @brief  单字段写入设备参数 (GUI slider ENTER 确认时调用)
 *
 *  只修改 g_dev 的一个字段，其他字段不受影响。
 *  内部有互斥锁保护，线程安全。
 *
 * @param  id   参数ID (对应 ParamId 枚举)
 * @param  val  要写入的值
 */
void DeviceData_SetParam(ParamId id, int32_t val);

#endif

/**
 ******************************************************************************
 * @file    Public_Data.h
 * @brief   公共传感器数据访问接口
 *
 *  提供线程安全的传感器数据读写:
 *    - 温湿度 (EnvData):   HTU21D 采集，控制任务/显示任务消费
 *    - 烟雾   (SmokeData): ADC 采集，显示/报警/应急任务消费
 *
 *  所有接口内部自带互斥锁，调用方无需关心同步问题
 ******************************************************************************
 */
#ifndef __PUBLIC_DATA_H
#define __PUBLIC_DATA_H

#include "main.h"

/* ============================================================
 *  环境数据 (温度 + 湿度)
 *
 *  生产者: Sensor_Contral 任务 (HTU21D, 周期15秒)
 *  消费者: cooper_task (自动控制), text_updata (显示刷新),
 *          esp8266 (MQTT上报), mygui.c (子页面标签)
 * ============================================================ */
typedef struct {
    float temp;     /* 温度 (°C) */
    float humd;     /* 湿度 (%RH) */
} EnvData_t;

/**
 * @brief  写入温湿度数据
 * @param  temp  温度值
 * @param  humd  湿度值
 */
void EnvData_Set(float temp, float humd);

/**
 * @brief  读取温湿度数据
 * @param  temp  输出: 温度值
 * @param  humd  输出: 湿度值
 */
void EnvData_Get(float *temp, float *humd);


/* ============================================================
 *  烟雾数据 (浓度 + 状态)
 *
 *  生产者: smoking_test 任务 (ADC采集)
 *  消费者: text_updata (显示刷新), emergency_task (报警),
 *          esp8266 (MQTT上报)
 * ============================================================ */
typedef enum {
    SMOKE_STATE_NORMAL = 0,     /* 正常: 电压 < 2.2V */
    SMOKE_STATE_WARN,           /* 警告: 2.2V <= 电压 <= 3.0V */
    SMOKE_STATE_ALARM           /* 报警: 电压 > 3.0V */
} SmokeStateTypeDef;

/**
 * @brief  写入烟雾浓度
 * @param  val  浓度值 (ppm)
 */
void SmokeData_SetValue(uint16_t val);

/**
 * @brief  读取烟雾浓度
 * @return 当前浓度值 (ppm)
 */
void SmokeData_GetValue(uint16_t *val);

/**
 * @brief  写入烟雾状态
 * @param  state  状态枚举值
 */
void SmokeData_SetState(SmokeStateTypeDef state);

/**
 * @brief  读取烟雾状态
 * @return 当前状态枚举值
 */
SmokeStateTypeDef SmokeData_GetState(void);

#endif

/**
 ******************************************************************************
 * @file    sensor.h
 * @brief   传感器抽象层接口
 *
 *  统一传感器读取接口，换传感器只需修改 sensor.c 实现
 *  业务层(cooper_task/mygui.c)无需关心底层是哪个传感器芯片
 ******************************************************************************
 */
#ifndef __SENSOR_H
#define __SENSOR_H

#include "main.h"
#include <stdbool.h>
#include "modbus.h"
#include "rs485.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* ============================================================
 *  Modbus 传感器参数配置（根据你的模块手册改这里）
 * ============================================================ */
#define SENSOR_TH_SLAVE       0x03       /* 温湿度从机地址 */
#define SENSOR_TH_REG_TEMP    0x0000     /* 温度寄存器 */
#define SENSOR_TH_REG_HUM     0x0001     /* 湿度寄存器 */
#define SENSOR_TH_SCALE       10.0f      /* 缩放系数 */

#define SENSOR_SMOKE_SLAVE    0x04       /* 烟雾从机地址 */
#define SENSOR_SMOKE_REG      0x0000     /* 烟雾寄存器 */

#define SENSOR_MODBUS_TIMEOUT 3000        /* 超时 ms */

/**
 * @brief  读取温湿度
 * @param  temp  输出: 温度 (°C)
 * @param  hum   输出: 湿度 (%RH)
 * @return true=读取成功, false=通信失败(输出值无效)
 */
 
bool Read_TempHum(float *temp, float *hum);
bool Read_Smoke(uint16_t *val);



#endif

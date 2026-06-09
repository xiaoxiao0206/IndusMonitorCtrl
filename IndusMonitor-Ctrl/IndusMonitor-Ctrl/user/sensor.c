/**
 ******************************************************************************
 * @file    sensor.c
 * @brief   传感器抽象层实现
 *
 *  换传感器时只需修改本文件:
 *    - HTU21D → SHT30  : 改 Read_TempHum 内部调用
 *    - MQ-2   → MQ-135 : 改 Read_Smoke 内部调用
 *    - 增加新传感器     : 本文件加新函数 + 头文件加声明
 ******************************************************************************
 */

#include "sensor.h"



/* ============================================================
 *  互斥锁 + 任务句柄
 * ============================================================ */

TaskHandle_t modbus_task_handle;



/* ============================================================
 *  底层: 发请求 + 等中断通知 + 解析响应
 *
 *  结果存在 modbus.Host_RxData[] 中
 *  成功返回 true, 失败返回 false
 * ============================================================ */
static bool Modbus_Read(uint8_t slave, uint16_t reg_addr, uint16_t num)
{
    bool result = false;

    modbus_task_handle = xTaskGetCurrentTaskHandle();
    ulTaskNotifyTake(pdTRUE, 0);

    Host_Read03_slave(slave, reg_addr, num);

    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(SENSOR_MODBUS_TIMEOUT)) != 0)
    {
        HOST_ModbusRX();
        if (modbus.Host_End)
        {
            modbus.Host_End = 0;
            result = true;
        }
    }

    modbus_task_handle = NULL;
    return result;
}

/* ============================================================
 *  温湿度读取
 * ============================================================ */
bool Read_TempHum(float *temp, float *hum)
{
    if (Modbus_Read(SENSOR_TH_SLAVE, SENSOR_TH_REG_TEMP, 2))
    {
        *temp = (float)modbus.Host_RxData[0] / SENSOR_TH_SCALE;
        *hum  = (float)modbus.Host_RxData[1] / SENSOR_TH_SCALE;

        if (*temp < -40.0f || *temp > 125.0f) return false;
        if (*hum  < 0.0f   || *hum  > 100.0f) return false;
        return true;
    }
    return false;
}

/* ============================================================
 *  烟雾浓度读取
 * ============================================================ */
bool Read_Smoke(uint16_t *val)
{
    if (Modbus_Read(SENSOR_SMOKE_SLAVE, SENSOR_SMOKE_REG, 1))
    {
        *val = modbus.Host_RxData[0];
        return true;
    }
    return false;
}

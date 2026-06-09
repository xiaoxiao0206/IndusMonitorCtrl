/**
 ******************************************************************************
 * @file    Public_Data.c
 * @brief   公共传感器数据存储 (线程安全)
 *
 *  设计原则:
 *    - 静态私有数据，外部不可直接访问
 *    - 所有读写通过接口函数，内部互斥锁保护
 *    - 温湿度和烟雾分别使用独立的互斥锁，互不阻塞
 *
 *  互斥锁分配:
 *    sensor_data_mutex → 温湿度数据
 *    smoke_data_mutex  → 烟雾数据 (浓度 + 状态共用一把锁)
 ******************************************************************************
 */

#include "Public_Data.h"
#include "FreeRTOS.h"
#include "semphr.h"

/* ============================================================
 *  私有数据 (外部不可直接访问)
 * ============================================================ */

/* 温湿度 */
static EnvData_t env_data = {0.0f, 0.0f};

/* 烟雾 */
static uint16_t         smoke_value = 0;
static SmokeStateTypeDef smoke_state = SMOKE_STATE_NORMAL;

/* 外部互斥锁 (在 freertos.c 中创建) */
extern SemaphoreHandle_t sensor_data_mutex;
extern SemaphoreHandle_t smoke_data_mutex;


/* ============================================================
 *  温湿度接口
 * ============================================================ */

void EnvData_Set(float temp, float humd)
{
    xSemaphoreTake(sensor_data_mutex, portMAX_DELAY);
    env_data.temp = temp;
    env_data.humd = humd;
    xSemaphoreGive(sensor_data_mutex);
}

void EnvData_Get(float *temp, float *humd)
{
    xSemaphoreTake(sensor_data_mutex, portMAX_DELAY);
    *temp = env_data.temp;
    *humd = env_data.humd;
    xSemaphoreGive(sensor_data_mutex);
}


/* ============================================================
 *  烟雾接口 (浓度和状态共用 smoke_data_mutex)
 *
 *  说明: 浓度和状态总是由 smoking_test 任务成对写入，
 *        共用一把锁不会产生死锁或优先级反转问题。
 *        如果未来浓度和状态被不同任务独立写入，
 *        则需要拆分为两把锁。
 * ============================================================ */

void SmokeData_SetValue(uint16_t val)
{
    xSemaphoreTake(smoke_data_mutex, portMAX_DELAY);
    smoke_value = val;
    xSemaphoreGive(smoke_data_mutex);
}

void SmokeData_GetValue(uint16_t *val)
{
 
    xSemaphoreTake(smoke_data_mutex, portMAX_DELAY);
    *val = smoke_value;
    xSemaphoreGive(smoke_data_mutex);
}

void SmokeData_SetState(SmokeStateTypeDef state)
{
    xSemaphoreTake(smoke_data_mutex, portMAX_DELAY);
    smoke_state = state;
    xSemaphoreGive(smoke_data_mutex);
}

SmokeStateTypeDef SmokeData_GetState(void)
{
    SmokeStateTypeDef state;
    xSemaphoreTake(smoke_data_mutex, portMAX_DELAY);
    state = smoke_state;
    xSemaphoreGive(smoke_data_mutex);
    return state;
}

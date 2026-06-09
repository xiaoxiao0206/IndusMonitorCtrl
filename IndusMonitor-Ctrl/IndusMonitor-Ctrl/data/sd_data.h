/**
 ******************************************************************************
 * @file    sd_log.h
 * @brief   SD卡日志：时间戳 + 数据打包 + 文件写入
 ******************************************************************************
 */
#ifndef __SD_DATA_H
#define __SD_DATA_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  写入一条传感器日志
 *         自动拼接 RTC 时间戳，格式化后写入 SD 卡
 *
 *  示例输出: 2025/01/07 14:30:25,25.3,60.2,320\r\n
 *
 * @param  temp   温度
 * @param  hum    湿度
 * @param  smoke  烟雾浓度
 * @return true=写入成功, false=失败
 */
bool SD_LogSensorData(float temp, float hum, uint16_t smoke);

/**
 * @brief  写入一条自定义日志
 *         只加时间戳，内容自定义
 *
 * @param  data  自定义字符串
 * @return true=写入成功
 */
bool SD_LogCustom(const char *data);

/**
 * @brief  补传日志：逐条读取 → 调用回调发送 → 清空文件
 *
 * @param  send_func  发送回调, 传入一行数据
 * @return 补传的条数
 */
typedef void (*SD_SendFunc_t)(const char *line);
uint16_t SD_FlushLog(SD_SendFunc_t send_func);

#endif

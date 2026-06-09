/**
 ******************************************************************************
 * @file    sd_log.c
 * @brief   SD卡日志实现
 ******************************************************************************
 */

#include "sd_data.h"
#include "rtc.h"
#include "ff.h"       /* FatFS */
#include <stdio.h>
#include <string.h>

/* ============================================================
 *  配置宏（改这里）
 * ============================================================ */
#define SD_LOG_FILE       "0:datalog.txt"
#define SD_FLUSH_FILE     "0:datalog.txt"
#define SD_LINE_MAX       128

/* 时间戳格式 */
#define TIME_FMT          "%04d/%02d/%02d %02d:%02d:%02d"

/* 数据行格式: 时间,温度,湿度,烟雾 */
#define SENSOR_LINE_FMT   "%s,%.1f,%.1f,%d\r\n"

/* 自定义行格式: 时间,内容 */
#define CUSTOM_LINE_FMT   "%s,%s\r\n"

/* ============================================================
 *  内部：获取时间戳
 * ============================================================ */
static bool sd_get_timestamp(char *buf, uint16_t size)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    int len = snprintf(buf, size, TIME_FMT,
                       2000 + sDate.Year, sDate.Month, sDate.Date,
                       sTime.Hours, sTime.Minutes, sTime.Seconds);

    return (len > 0 && len < size);
}

/* ============================================================
 *  内部：追加写一行到文件
 * ============================================================ */
static bool sd_append_line(const char *line)
{
    FIL fil;
    UINT bw;
    FRESULT res;

    res = f_open(&fil, SD_LOG_FILE, FA_WRITE | FA_OPEN_ALWAYS);
    if (res != FR_OK)
    {
        printf("[SD] f_open 失败: %d\r\n", res);
        return false;
    }

    f_lseek(&fil, f_size(&fil));

    res = f_write(&fil, line, strlen(line), &bw);
    if (res != FR_OK)
    {
        printf("[SD] f_write 失败: %d\r\n", res);
        f_close(&fil);
        return false;
    }

    if (bw != strlen(line))
    {
        printf("[SD] 写入不完整: 要写%d 实写%d\r\n", strlen(line), bw);
        f_close(&fil);
        return false;
    }

    res = f_sync(&fil);
    if (res != FR_OK)
    {
        printf("[SD] f_sync 失败: %d\r\n", res);
    }

    f_close(&fil);
    return (res == FR_OK);
}

/* ============================================================
 *  传感器日志写入
 * ============================================================ */
bool SD_LogSensorData(float temp, float hum, uint16_t smoke)
{
    char time_buf[24];
    char line_buf[SD_LINE_MAX];

    if (!sd_get_timestamp(time_buf, sizeof(time_buf)))
        return false;

    snprintf(line_buf, sizeof(line_buf), SENSOR_LINE_FMT,
             time_buf, temp, hum, smoke);

    return sd_append_line(line_buf);
}

/* ============================================================
 *  自定义日志写入
 * ============================================================ */
bool SD_LogCustom(const char *data)
{
    char time_buf[24];
    char line_buf[SD_LINE_MAX];

    if (!sd_get_timestamp(time_buf, sizeof(time_buf)))
        return false;

    snprintf(line_buf, sizeof(line_buf), CUSTOM_LINE_FMT,
             time_buf, data);

    return sd_append_line(line_buf);
}

/* ============================================================
 *  补传：读出全部日志 → 逐行回调发送 → 删除文件
 * ============================================================ */
uint16_t SD_FlushLog(SD_SendFunc_t send_func)
{
    FIL fil;
    UINT br;
    uint16_t count = 0;
    char line_buf[SD_LINE_MAX];

    if (f_open(&fil, SD_FLUSH_FILE, FA_READ) != FR_OK)
        return 0;

    while (f_gets(line_buf, sizeof(line_buf), &fil))
    {
        /* 去掉 f_gets 读到的换行，重新加 \r\n 保证格式统一 */
        uint16_t len = strlen(line_buf);
        while (len > 0 && (line_buf[len - 1] == '\r' || line_buf[len - 1] == '\n'))
            line_buf[--len] = '\0';

        if (len > 0 && send_func != NULL)
        {
            send_func(line_buf);
            count++;
        }
    }

    f_close(&fil);

    /* 传完删除 */
    if (count > 0)
        f_unlink(SD_FLUSH_FILE);

    return count;
}

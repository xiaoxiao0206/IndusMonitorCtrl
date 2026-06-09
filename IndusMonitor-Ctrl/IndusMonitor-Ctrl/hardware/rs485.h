#ifndef __RS485_H
#define __RS485_H

#include "main.h"
#include "usart.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
/* ==================== 用户配置区 ==================== */

/* RS485 方向控制引脚 (DE/RE) */
#define RS485_DE_PORT           GPIOD
#define RS485_DE_PIN            GPIO_PIN_3

/* 发送/接收模式切换宏 */
#define RS485_TX_ENABLE()       HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_SET)
#define RS485_RX_ENABLE()       HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_RESET)

/* 缓冲区大小 */
#define RS485_RX_BUF_SIZE       256     /* DMA 循环接收缓冲区（必须是 2 的幂） */
#define RS485_TX_BUF_SIZE       256     /* 发送缓冲区 */
#define RS485_FRAME_MAX_LEN     128     /* 单帧最大长度 */

/* 超时时间 (ms) */
#define RS485_TX_TIMEOUT        1000
#define RS485_RX_IDLE_TIMEOUT   50      /* 空闲超时，用于判断一帧结束 */

/* ==================== 数据结构 ==================== */

typedef struct {
    /* DMA 接收相关 */
    uint8_t  dma_rx_buf[RS485_RX_BUF_SIZE];  /* DMA 循环接收原始缓冲区 */
    uint16_t rx_frame_len;                     /* 当前接收帧长度 */

    /* 帧输出 */
    uint8_t  frame_buf[RS485_FRAME_MAX_LEN];  /* 解析出的完整帧 */
    uint16_t frame_len;                        /* 完整帧长度 */
    volatile uint8_t frame_ready;              /* 帧就绪标志 */

    /* 接收状态 */
    uint16_t last_dma_pos;                     /* 上次 DMA 接收位置 */
} RS485_HandleTypeDef;

/* 全局句柄 */
extern RS485_HandleTypeDef rs485;

/* ==================== API 函数 ==================== */

/**
 * @brief  初始化 RS485（初始化 DE 引脚 + 启动 DMA 接收 + 使能 IDLE 中断）
 * @retval 0: 成功, -1: 失败
 */
int RS485_Init(void);

/**
 * @brief  RS485 发送数据（自动切换方向）
 * @param  data: 发送数据指针
 * @param  len:  数据长度
 * @retval HAL 状态
 */
int RS485_Transmit(const uint8_t *data, uint16_t len);

/**
 * @brief  RS485 发送字符串（自动切换方向）
 * @param  str: 以 '\0' 结尾的字符串
 * @retval HAL 状态
 */
int RS485_TransmitString(const char *str);

/**
 * @brief  检查是否收到完整一帧
 * @retval 1: 有新帧, 0: 无
 */
uint8_t RS485_FrameReady(void);

/**
 * @brief  读取接收到的帧数据
 * @param  buf:   输出缓冲区
 * @param  max_len: 缓冲区最大长度
 * @retval 实际帧长度, 0 表示无数据
 */
uint16_t RS485_ReadFrame(uint8_t *buf, uint16_t max_len);

/**
 * @brief  获取帧数据指针（零拷贝）
 * @param  len: 输出帧长度
 * @retval 帧数据指针, NULL 表示无数据
 */
uint8_t *RS485_GetFramePtr(uint16_t *len);

/**
 * @brief  处理 DMA 接收数据（解析 IDLE 中断后的帧）
 *         在 USART1_IRQHandler 中会被调用，也可周期性调用
 */
void RS485_Process(void);

#endif /* __RS485_H */

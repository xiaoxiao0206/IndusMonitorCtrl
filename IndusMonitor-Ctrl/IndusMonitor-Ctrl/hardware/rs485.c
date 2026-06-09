#include "rs485.h"

/* ==================== 私有变量 ==================== */

RS485_HandleTypeDef rs485;

/* 私有：记录 DMA 缓冲区上次处理位置 */
static uint16_t old_pos = 0;

/* ==================== 初始化 ==================== */

int RS485_Init(void)
{

    RS485_RX_ENABLE();  /* 默认接收模式 */

    /* 2. 清零状态 */
    memset(&rs485, 0, sizeof(rs485));
    old_pos = 0;

    /* 3. 启动 DMA 循环接收（DMA1_Channel5 已在 MspInit 中配置） */
    if (HAL_UART_Receive_DMA(&huart1, rs485.dma_rx_buf, RS485_RX_BUF_SIZE) != HAL_OK) {
        return -1;
    }

    /* 4. 使能空闲中断（IDLE），每帧数据接收完成后触发 */
		__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT | DMA_IT_TC);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
		
    return 0;
}

/* ==================== DMA 接收处理（IDLE 中断中调用） ==================== */

void RS485_Process(void)
{
    uint16_t pos;
    uint16_t cnt;

    /* DMA NDTR 寄存器：剩余待传输数据量 */
    /* 已接收数据量 = 缓冲区大小 - 剩余量 */
    pos = RS485_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart1.hdmarx);

    if (pos != old_pos) {
        if (pos > old_pos) {
            /* 正常情况：数据未绕回 */
            cnt = pos - old_pos;
            if (rs485.rx_frame_len + cnt <= RS485_FRAME_MAX_LEN) {
                memcpy(&rs485.frame_buf[rs485.rx_frame_len],
                       &rs485.dma_rx_buf[old_pos], cnt);
                rs485.rx_frame_len += cnt;
            }
        } else {
            /* 缓冲区绕回：先取尾部，再取头部 */
            uint16_t tail = RS485_RX_BUF_SIZE - old_pos;
            if (rs485.rx_frame_len + tail + pos <= RS485_FRAME_MAX_LEN) {
                memcpy(&rs485.frame_buf[rs485.rx_frame_len],
                       &rs485.dma_rx_buf[old_pos], tail);
                rs485.rx_frame_len += tail;

                memcpy(&rs485.frame_buf[rs485.rx_frame_len],
                       &rs485.dma_rx_buf[0], pos);
                rs485.rx_frame_len += pos;
            }
        }
        old_pos = pos;
    }

    /* IDLE 中断触发意味着一帧结束 */
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);  /* 清除 IDLE 标志 */

        if (rs485.rx_frame_len > 0) {
            rs485.frame_len   = rs485.rx_frame_len;
            rs485.frame_ready = 1;

            /* 重置帧缓冲区计数，准备接收下一帧 */
            rs485.rx_frame_len = 0;
					
					extern TaskHandle_t modbus_task_handle;
            if (modbus_task_handle != NULL)
            {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                vTaskNotifyGiveFromISR(modbus_task_handle,
                                       &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
}

/* ==================== 发送函数 ==================== */

int RS485_Transmit(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) return -1;

    /* 停掉 DMA 接收 */
    HAL_UART_AbortReceive(&huart1);
    rs485.frame_ready  = 0;
    rs485.rx_frame_len = 0;
    old_pos = 0;

    /* 清除所有错误标志 */
    __HAL_UART_CLEAR_OREFLAG(&huart1);
    huart1.Instance->SR;
    huart1.Instance->DR;

    /* 发送模式 */
    RS485_TX_ENABLE();

    /* ★ 用阻塞发送，不用 DMA，不依赖 TC 标志 */
    HAL_StatusTypeDef ret = HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 500);

    /* 接收模式 */
    RS485_RX_ENABLE();

    /* 重启 DMA 接收 */
    HAL_UART_Receive_DMA(&huart1, rs485.dma_rx_buf, RS485_RX_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT | DMA_IT_TC);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);

    return (int)ret;
}


/* ==================== 帧读取接口 ==================== */

uint8_t RS485_FrameReady(void)
{
    return rs485.frame_ready;
}

uint16_t RS485_ReadFrame(uint8_t *buf, uint16_t max_len)
{
    if (!rs485.frame_ready || buf == NULL) return 0;

    uint16_t copy_len = rs485.frame_len;
    if (copy_len > max_len) copy_len = max_len;

    memcpy(buf, rs485.frame_buf, copy_len);
    rs485.frame_ready = 0;  /* 清除标志 */

    return copy_len;
}

uint8_t *RS485_GetFramePtr(uint16_t *len)
{
    if (!rs485.frame_ready) {
        if (len) *len = 0;
        return NULL;
    }

    if (len) *len = rs485.frame_len;
    rs485.frame_ready = 0;

    return rs485.frame_buf;
}

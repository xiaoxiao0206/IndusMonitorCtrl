#ifndef __MODBUS_H
#define __MODBUS_H

#include "main.h"
#include <stdint.h>
#include "rs485.h"

/* ==================== 缓冲区大小 ==================== */
#define MODBUS_RX_BUF_SIZE      256
#define MODBUS_TX_BUF_SIZE      256
#define HOST_TX_BUF_SIZE        256
#define HOST_RX_DATA_MAX        64

/* ==================== 数据结构 ==================== */
typedef struct {
    /* 通用 */
    uint8_t  myadd;                          // 本机（从机）地址
    uint8_t  slave_add;                      // 主机模式：目标从机地址
    uint8_t  timrun;                         // 超时定时器使能
    uint16_t timout;                         // 超时计数
    uint16_t recount;                        // 接收到的帧长度

    /* 缓冲区 */
    uint8_t  rcbuf[MODBUS_RX_BUF_SIZE];     // 接收缓冲区
    uint8_t  sendbuf[MODBUS_TX_BUF_SIZE];   // 从机发送缓冲区
    uint8_t  Host_Txbuf[HOST_TX_BUF_SIZE];  // 主机发送缓冲区

    /* 主机模式 */
    uint16_t Host_RxData[HOST_RX_DATA_MAX]; // 主机接收到的寄存器数据
    uint8_t  Host_RxNum;                     // 接收到的寄存器数量
    volatile uint8_t Host_End;               // 主机通信完成标志
    uint8_t  Host_Error;                     // 主机收到的异常码（0=无错误）
} MODBUS;

/* ==================== 全局变量 ==================== */
extern MODBUS modbus;
extern uint16_t Reg[];

/* ==================== 从机 API ==================== */
void Modbus_Init(void);
void Modbus_Event(void);
void Modbus_Func3(void);
void Modbus_Func6(void);
void Modbus_Func16(void);

/* ==================== 主机 API ==================== */
void Host_Read03_slave(uint8_t slave, uint16_t StartAddr, uint16_t num);
void Host_Func3(void);
void Host_write06_slave(uint8_t slave, uint16_t StartAddr, uint16_t val);
void Host_Func6(void);
void Host_write16_slave(uint8_t slave, uint16_t StartAddr, uint16_t *data, uint16_t num);
void HOST_ModbusRX(void);

/* ==================== CRC 函数 ==================== */
uint16_t Modbus_CRC16(uint8_t *buf, uint16_t len);

#endif /* __MODBUS_H */

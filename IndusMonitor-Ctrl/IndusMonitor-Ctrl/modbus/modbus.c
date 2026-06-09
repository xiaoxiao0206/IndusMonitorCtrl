#include "modbus.h"
#include "rs485.h"
#include <string.h>

/* ==================== 寄存器表 ==================== */
uint16_t Reg[] = {
    0x0001, 0x0002, 0x0003, 0x0004,
    0x0005, 0x0006, 0x0007, 0x0008
};

#define REG_NUM  (sizeof(Reg) / sizeof(Reg[0]))

MODBUS modbus;

/* ==================== 初始化 ==================== */
void Modbus_Init(void)
{
    memset(&modbus, 0, sizeof(MODBUS));  // 先清零
    modbus.myadd     = 0x02;             // 从机地址
    modbus.timrun    = 1;                // 使能超时定时器
    modbus.slave_add = 0x02;             // 主机目标从机地址
}

/* ==================== 功能码 03：读保持寄存器 ==================== */
void Modbus_Func3(void)
{
    uint16_t Regadd, Reglen, crc;
    uint8_t i, j;

    Regadd = (modbus.rcbuf[2] << 8) | modbus.rcbuf[3];
    Reglen = (modbus.rcbuf[4] << 8) | modbus.rcbuf[5];

    /* 越界检查 */
    if (Regadd + Reglen > REG_NUM)
    {
        modbus.sendbuf[0] = modbus.myadd;
        modbus.sendbuf[1] = 0x83;        // 异常功能码
        modbus.sendbuf[2] = 0x02;        // 异常码：非法数据地址
        crc = Modbus_CRC16(modbus.sendbuf, 3);
        modbus.sendbuf[3] = crc & 0xFF;
        modbus.sendbuf[4] = (crc >> 8) & 0xFF;
        RS485_Transmit(modbus.sendbuf, 5);
        return;
    }

    i = 0;
    modbus.sendbuf[i++] = modbus.myadd;
    modbus.sendbuf[i++] = 0x03;
    modbus.sendbuf[i++] = Reglen * 2;    // 字节数

    for (j = 0; j < Reglen; j++)
    {
        modbus.sendbuf[i++] = (Reg[Regadd + j] >> 8) & 0xFF;
        modbus.sendbuf[i++] =  Reg[Regadd + j]       & 0xFF;
    }

    /* CRC：低字节在前，高字节在后（Modbus RTU 规范） */
    crc = Modbus_CRC16(modbus.sendbuf, i);
    modbus.sendbuf[i++] = crc & 0xFF;
    modbus.sendbuf[i++] = (crc >> 8) & 0xFF;

    RS485_Transmit(modbus.sendbuf, i);
}

/* ==================== 功能码 06：写单个寄存器 ==================== */
void Modbus_Func6(void)
{
    uint16_t Regadd, val, crc;
    //uint8_t i;

    Regadd = (modbus.rcbuf[2] << 8) | modbus.rcbuf[3];
    val    = (modbus.rcbuf[4] << 8) | modbus.rcbuf[5];

    /* 越界检查 */
    if (Regadd >= REG_NUM)
    {
        modbus.sendbuf[0] = modbus.myadd;
        modbus.sendbuf[1] = 0x86;
        modbus.sendbuf[2] = 0x02;
        crc = Modbus_CRC16(modbus.sendbuf, 3);
        modbus.sendbuf[3] = crc & 0xFF;
        modbus.sendbuf[4] = (crc >> 8) & 0xFF;
        RS485_Transmit(modbus.sendbuf, 5);
        return;
    }

    Reg[Regadd] = val;

    /* 回显请求帧 */
    memcpy(modbus.sendbuf, modbus.rcbuf, 6);

    crc = Modbus_CRC16(modbus.sendbuf, 6);
    modbus.sendbuf[6] = crc & 0xFF;
    modbus.sendbuf[7] = (crc >> 8) & 0xFF;

    RS485_Transmit(modbus.sendbuf, 8);
}

/* ==================== 功能码 16：写多个寄存器 ==================== */
void Modbus_Func16(void)
{
    uint16_t Regadd, Reglen, crc;
    uint8_t i;

    Regadd = (modbus.rcbuf[2] << 8) | modbus.rcbuf[3];
    Reglen = (modbus.rcbuf[4] << 8) | modbus.rcbuf[5];

    /* 越界检查 */
    if (Regadd + Reglen > REG_NUM)
    {
        modbus.sendbuf[0] = modbus.myadd;
        modbus.sendbuf[1] = 0x90;
        modbus.sendbuf[2] = 0x02;
        crc = Modbus_CRC16(modbus.sendbuf, 3);
        modbus.sendbuf[3] = crc & 0xFF;
        modbus.sendbuf[4] = (crc >> 8) & 0xFF;
        RS485_Transmit(modbus.sendbuf, 5);
        return;
    }

    for (i = 0; i < Reglen; i++)
    {
        Reg[Regadd + i] = (modbus.rcbuf[7 + i * 2] << 8) | modbus.rcbuf[8 + i * 2];
    }

    /* 响应：地址 + 功能码 + 起始地址 + 寄存器数量 + CRC（6字节） */
    modbus.sendbuf[0] = modbus.myadd;
    modbus.sendbuf[1] = 0x10;
    modbus.sendbuf[2] = (Regadd >> 8) & 0xFF;
    modbus.sendbuf[3] = Regadd & 0xFF;
    modbus.sendbuf[4] = (Reglen >> 8) & 0xFF;
    modbus.sendbuf[5] = Reglen & 0xFF;

    crc = Modbus_CRC16(modbus.sendbuf, 6);
    modbus.sendbuf[6] = crc & 0xFF;
    modbus.sendbuf[7] = (crc >> 8) & 0xFF;

    RS485_Transmit(modbus.sendbuf, 8);
}

/* ==================== 从机事件处理 ==================== */
void Modbus_Event(void)
{
    uint16_t crc, rccrc;
    uint16_t len;

    /* 读取 RS485 收到的一帧数据 */
    uint8_t *frame = RS485_GetFramePtr(&len);
    if (frame == NULL || len < 4) return;  // Modbus 最短帧：地址+功能码+CRC=4字节

    /* 复制到 Modbus 接收缓冲区 */
    if (len > sizeof(modbus.rcbuf)) return;
    memcpy(modbus.rcbuf, frame, len);
    modbus.recount = len;

    /* CRC 校验：最后两字节，低字节在前 */
    crc   = Modbus_CRC16(modbus.rcbuf, len - 2);
    rccrc = modbus.rcbuf[len - 2] | (modbus.rcbuf[len - 1] << 8);
    if (crc != rccrc) return;

    /* 地址匹配 */
    if (modbus.rcbuf[0] != modbus.myadd) return;

    /* 分发功能码 */
    switch (modbus.rcbuf[1])
    {
        case 3:  Modbus_Func3();  break;
        case 6:  Modbus_Func6();  break;
        case 16: Modbus_Func16(); break;
        default:
        {
            /* 不支持的功能码，返回异常响应 */
            modbus.sendbuf[0] = modbus.myadd;
            modbus.sendbuf[1] = modbus.rcbuf[1] | 0x80;
            modbus.sendbuf[2] = 0x01;  // 异常码：非法功能码
            crc = Modbus_CRC16(modbus.sendbuf, 3);
            modbus.sendbuf[3] = crc & 0xFF;
            modbus.sendbuf[4] = (crc >> 8) & 0xFF;
            RS485_Transmit(modbus.sendbuf, 5);
            break;
        }
    }
}

/* ==================== 主机功能 ==================== */

/**
 * @brief  主机发送 03 命令：读从机保持寄存器
 */
void Host_Read03_slave(uint8_t slave, uint16_t StartAddr, uint16_t num)
{
    uint16_t crc;
    modbus.slave_add = slave;

    modbus.Host_Txbuf[0] = slave;
    modbus.Host_Txbuf[1] = 0x03;
    modbus.Host_Txbuf[2] = (StartAddr >> 8) & 0xFF;
    modbus.Host_Txbuf[3] = StartAddr & 0xFF;
    modbus.Host_Txbuf[4] = (num >> 8) & 0xFF;
    modbus.Host_Txbuf[5] = num & 0xFF;

    crc = Modbus_CRC16(modbus.Host_Txbuf, 6);
    modbus.Host_Txbuf[6] = crc & 0xFF;          // CRC 低字节在前
    modbus.Host_Txbuf[7] = (crc >> 8) & 0xFF;   // CRC 高字节在后

    RS485_Transmit(modbus.Host_Txbuf, 8);
    modbus.timout  = 0;
    modbus.Host_End = 0;
}

/**
 * @brief  主机处理 03 响应：提取寄存器数据
 */
void Host_Func3(void)
{
	printf("接收成功\r\n");
    uint8_t byte_count = modbus.rcbuf[2];
    uint8_t reg_count  = byte_count / 2;
    uint8_t i;

    /* 校验数据长度 */
    if (byte_count + 5 > modbus.recount) return;  // 地址+功能码+字节数+数据+CRC
      printf("f3 crc sucess\r\n");
    for (i = 0; i < reg_count; i++)
    {
        modbus.Host_RxData[i] =
            (modbus.rcbuf[3 + i * 2] << 8) | modbus.rcbuf[4 + i * 2];
    }

    modbus.Host_RxNum = reg_count;
    modbus.Host_End   = 1;
}

/**
 * @brief  主机发送 06 命令：写从机单个寄存器
 */
void Host_write06_slave(uint8_t slave, uint16_t StartAddr, uint16_t val)
{
    uint16_t crc;
    modbus.slave_add = slave;

    modbus.Host_Txbuf[0] = slave;
    modbus.Host_Txbuf[1] = 0x06;
    modbus.Host_Txbuf[2] = (StartAddr >> 8) & 0xFF;
    modbus.Host_Txbuf[3] = StartAddr & 0xFF;
    modbus.Host_Txbuf[4] = (val >> 8) & 0xFF;
    modbus.Host_Txbuf[5] = val & 0xFF;

    crc = Modbus_CRC16(modbus.Host_Txbuf, 6);
    modbus.Host_Txbuf[6] = crc & 0xFF;
    modbus.Host_Txbuf[7] = (crc >> 8) & 0xFF;

    RS485_Transmit(modbus.Host_Txbuf, 8);
    modbus.timout  = 0;
    modbus.Host_End = 0;
}

/**
 * @brief  主机处理 06 响应：校验回显
 */
void Host_Func6(void)
{
    uint16_t crc, rccrc;

    if (modbus.recount < 8) return;

    crc   = Modbus_CRC16(modbus.rcbuf, 6);
    rccrc = modbus.rcbuf[6] | (modbus.rcbuf[7] << 8);
    if (crc != rccrc) return;

    modbus.Host_End = 1;
}

/**
 * @brief  主机发送 16 命令：写从机多个寄存器
 */
void Host_write16_slave(uint8_t slave, uint16_t StartAddr, uint16_t *data, uint16_t num)
{
    uint16_t crc;
    uint8_t i, len;

    modbus.slave_add = slave;

    modbus.Host_Txbuf[0] = slave;
    modbus.Host_Txbuf[1] = 0x10;
    modbus.Host_Txbuf[2] = (StartAddr >> 8) & 0xFF;
    modbus.Host_Txbuf[3] = StartAddr & 0xFF;
    modbus.Host_Txbuf[4] = (num >> 8) & 0xFF;
    modbus.Host_Txbuf[5] = num & 0xFF;
    modbus.Host_Txbuf[6] = num * 2;  // 字节数

    for (i = 0; i < num; i++)
    {
        modbus.Host_Txbuf[7 + i * 2] = (data[i] >> 8) & 0xFF;
        modbus.Host_Txbuf[8 + i * 2] = data[i] & 0xFF;
    }

    len = 7 + num * 2;
    crc = Modbus_CRC16(modbus.Host_Txbuf, len);
    modbus.Host_Txbuf[len]     = crc & 0xFF;
    modbus.Host_Txbuf[len + 1] = (crc >> 8) & 0xFF;

    RS485_Transmit(modbus.Host_Txbuf, len + 2);
    modbus.timout  = 0;
    modbus.Host_End = 0;
}

/**
 * @brief  主机接收处理：校验地址 + CRC，再分发
 */
void HOST_ModbusRX(void)
{
    uint16_t len, crc, rccrc;
    uint8_t *frame = RS485_GetFramePtr(&len);
    if (frame == NULL || len < 4) return;

    memcpy(modbus.rcbuf, frame, len);
    modbus.recount = len;
	printf("RX[%d]: ", len);
    for (uint16_t i = 0; i < len; i++)
    {
        printf("%02X ", modbus.rcbuf[i]);
    }
    printf("\r\n");
    /* 地址校验 */
    if (modbus.rcbuf[0] != modbus.slave_add) return;

		printf("sucess\r\n");
    /* CRC 校验 */
    crc   = Modbus_CRC16(modbus.rcbuf, len - 2);	
    rccrc = modbus.rcbuf[len - 2] | (modbus.rcbuf[len - 1] << 8);
		printf("CRC: calc=%04X, recv=%04X\r\n", crc, rccrc); 
    if (crc != rccrc) return;
			printf("crc sucess\r\n");
    /* 功能码分发 */
    switch (modbus.rcbuf[1])
    {
        case 0x03: Host_Func3(); break;
        case 0x06: Host_Func6(); break;
        /* 异常响应（功能码最高位为1） */
        case 0x83:
        case 0x86:
        case 0x90:
            modbus.Host_Error = modbus.rcbuf[2];  // 保存异常码
            modbus.Host_End   = 1;
            break;
    }
}

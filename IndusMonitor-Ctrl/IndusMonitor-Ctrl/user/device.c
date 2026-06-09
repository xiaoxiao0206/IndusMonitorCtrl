#include "device.h"



/*=====================================================================
 *  通信协议: x xxx x  (设备类型 + 设备编号 + 设备状态)
 *  示例: 10011 → 类型1(灯), 编号001, 状态1(开)
 *=====================================================================*/

#define STATE_ON   1
#define STATE_OFF  0

/* ---- 设备类型枚举 ---- */
typedef enum {
    DEV_TYPE_LIGHT  = 1,    // 灯
    DEV_TYPE_SOCKET = 2,    // 插座
    // 按需扩展
    DEV_TYPE_MAX
} DeviceType;

/* ---- 单个继电器描述 ---- */
typedef struct {
    DeviceType  type;           // 设备类型
    uint8_t     dev_id;         // 该类型下的编号
    uint8_t     slave_addr;     // Modbus 从机地址
    uint16_t    reg_addr;       // 线圈/寄存器地址
} RelayDev_t;

/*=====================================================================
 *  继电器设备表（只需要维护这一张表）
 *=====================================================================*/
static const RelayDev_t g_device_table[] = {
    /*  类型             编号   从机地址  线圈地址  */
    { DEV_TYPE_LIGHT,     1,    0x01,    0x0000 },    // 灯-1
    { DEV_TYPE_LIGHT,     2,    0x01,    0x0001 },    // 灯-2
    { DEV_TYPE_LIGHT,     3,    0x01,    0x0002 },    // 灯-3
    { DEV_TYPE_SOCKET,    1,    0x01,    0x0003 },    // 插座-1
    { DEV_TYPE_SOCKET,    2,    0x01,    0x0004 },    // 插座-2
    { DEV_TYPE_SOCKET,    3,    0x01,    0x0005 },    // 插座-3
    /* 不同从机的继电器 */
    // { DEV_TYPE_LIGHT,  4,    0x02,    0x0000 },
};

#define DEVICE_TABLE_SIZE  (sizeof(g_device_table) / sizeof(g_device_table[0]))

/*=====================================================================
 *  解码通信帧
 *=====================================================================*/
static inline uint16_t decode_type(uint16_t flag)  { return flag / 10000;       }
static inline uint16_t decode_id(uint16_t flag)    { return (flag / 10) % 1000; }
static inline uint16_t decode_state(uint16_t flag) { return flag % 10;          }

/*=====================================================================
 *  通过 Modbus 05 功能码控制继电器（写单个线圈）
 *  开 → 0xFF00    关 → 0x0000
 *=====================================================================*/
static void Modbus_WriteCoil(uint8_t slave, uint16_t coil_addr, uint8_t state)
{
    uint8_t  buf[8];
    uint16_t crc;
    uint16_t value = (state == STATE_ON) ? 0xFF00 : 0x0000;

    buf[0] = slave;
    buf[1] = 0x05;
    buf[2] = (coil_addr >> 8) & 0xFF;
    buf[3] = coil_addr & 0xFF;
    buf[4] = (value >> 8) & 0xFF;
    buf[5] = value & 0xFF;

    crc = Modbus_CRC16(buf, 6);
    buf[6] = crc & 0xFF;
    buf[7] = (crc >> 8) & 0xFF;

    RS485_Transmit(buf, 8);
}

/*=====================================================================
 *  统一设备控制
 *  输入: 通信帧  例 10011 → 灯1 开
 *  返回: 成功 → flag + type*1000
 *        失败 → (flag ^ 1) + 错误码
 *=====================================================================*/
uint16_t device_control(uint16_t flag)
{
    uint16_t type  = decode_type(flag);
    uint16_t id    = decode_id(flag);
    uint16_t state = decode_state(flag);

    if (type == 0 || type >= DEV_TYPE_MAX)
        return (flag ^ 1) + 9000;

    if (state != STATE_ON && state != STATE_OFF)
        return (flag ^ 1) + 8000;

    for (uint16_t i = 0; i < DEVICE_TABLE_SIZE; i++)
    {
        if (g_device_table[i].type   == type &&
            g_device_table[i].dev_id == id)
        {
            Modbus_WriteCoil(g_device_table[i].slave_addr,
                             g_device_table[i].reg_addr,
                             state);

            return flag + (type * 1000);
        }
    }

    return (flag ^ 1) + (type * 1000);  // 未找到设备
}

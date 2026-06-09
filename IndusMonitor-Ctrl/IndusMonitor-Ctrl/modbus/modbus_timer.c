#include "modbus_timer.h"


static Modbus_TypeDef modbus = {0};

void Modbus_Timer_Task(void)
{
    // 原超时重发逻辑
    if(modbus.timrun != 0)
    {
        modbus.timout++;
        if(modbus.timout >= 100)  // 100ms 超时
        {
            modbus.timout = 0;
            modbus.Host_out_flag = 1;
        }
    }

    // 原发送间隔计时逻辑
    modbus.Host_Sendtime++;
    if(modbus.Host_Sendtime > 1000)  // 1s 间隔
    {
        modbus.Host_time_flag = 1;			
    }
}

#ifndef __MODBUS_TIMER_H
#define __MODBUS_TIMER_H	 
#include "tim.h"



typedef struct {
    uint8_t timrun;
    uint16_t timout;
    uint8_t Host_out_flag;
    uint16_t Host_Sendtime;
    uint8_t Host_time_flag;
    // ... 其他成员
} Modbus_TypeDef;


//extern u8 sec_flag;
//extern int time;
void Modbus_Timer_Task(void);
		 				    
#endif





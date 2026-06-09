#ifndef __HTU21D_H
#define __HTU21D_H

#include "myi2c.h"
#define TRIGGER_TEMP_MEASURE_HOLD 0xE3
#define TRIGGER_HUMD_MEASURE_HOLD 0xE5
#define TRIGGER_TEMP_MEASURE_NOHOLD 0xF3
#define TRIGGER_HUMD_MEASURE_NOHOLD 0xF5
#define WRITE_USER_REG 0xE6
#define READ_USER_REG 0xE7
#define SOFT_RESET 0xFE

uint8_t HTU21D_reset(void);

uint8_t HTU21D_measure(uint8_t reg_val);

void HTU21D_Init(void);

uint16_t HTU21D_ReadMeasureData(void);

#endif

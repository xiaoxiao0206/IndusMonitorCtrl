#ifndef __MYI2C_H
#define __MYI2C_H

#include "main.h"
void I2C_Start(void);
void I2C_Stop(void);
uint8_t I2C_R_SDA(void);
void I2C_Send(uint8_t Byte);
uint8_t I2C_receive(void);
void I2C_SendAck(uint8_t bit);
uint8_t I2C_ReceiveAck(void);
#endif


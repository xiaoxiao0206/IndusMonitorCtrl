#include "HTU21D.h"

#define HTU21D_ADDRESS 0x80

uint8_t HTU21D_reset(void)
{
    uint8_t i;
    I2C_Start();
    I2C_Send(0x80);
    if (I2C_ReceiveAck() != 0) 
    {
        I2C_Stop(); 
        return 1;
    }
    I2C_Send(SOFT_RESET);
    i = I2C_ReceiveAck();    
    I2C_Stop();
    return i;
}

uint8_t HTU21D_measure(uint8_t reg_val)
{
 
        I2C_Start();

        I2C_Send(HTU21D_ADDRESS);
        if (I2C_ReceiveAck() != 0) 
        {
            I2C_Stop();
            return 0;
        }

        I2C_Send(reg_val);  

        if (I2C_ReceiveAck() != 0) 
        {
            I2C_Stop();
            return 0;
        }
        I2C_Stop();

        if (reg_val == TRIGGER_TEMP_MEASURE_HOLD)
            HAL_Delay(50);
        else if (reg_val == TRIGGER_HUMD_MEASURE_HOLD)
            HAL_Delay(20);
        return 1;
}

void HTU21D_Init(void)
{
    HTU21D_reset();
    //HAL_Delay(20);
}

uint16_t HTU21D_ReadMeasureData(void)
{
    static uint16_t raw_data[3];
    uint16_t raw_val;

    I2C_Start();

    I2C_Send(HTU21D_ADDRESS | 0x01);

    if (I2C_ReceiveAck() != 0)
    {
        I2C_Stop();
        return 0xFF;
    }

    raw_data[0] = I2C_receive();
    I2C_SendAck(0);

    raw_data[1] = I2C_receive();
    I2C_SendAck(0);

    raw_data[2] = I2C_receive();
    I2C_SendAck(1);
    I2C_Stop();

    raw_val = (raw_data[0] << 8) | (raw_data[1] & 0xFC);
    return raw_val;
}

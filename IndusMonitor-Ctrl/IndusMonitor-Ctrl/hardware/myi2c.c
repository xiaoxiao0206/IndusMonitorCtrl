#include "main.h"
#include "tim.h"

void My_SCL(uint8_t x)
{
 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, (GPIO_PinState)(x));
 delay_us(10);
}

void My_SDA(uint8_t x)
{
 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, (GPIO_PinState)(x));
 delay_us(10);
}




void I2C_Start(void)
{
    My_SDA(1);
    My_SCL(1);
    My_SDA(0);
    My_SCL(0);
}

void I2C_Stop(void)
{
    My_SDA(0);
    My_SCL(1);
    My_SDA(1);
}

uint8_t I2C_R_SDA(void)
{
    uint8_t Read_Ddata;
    Read_Ddata = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_7);
    delay_us(10);
    return Read_Ddata;
}


void I2C_Send(uint8_t Byte)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        My_SDA(Byte & (0x80 >> i));
        My_SCL(1);
        My_SCL(0);
    }
}

uint8_t I2C_receive(void)
{
    uint8_t receivebyte = 0x00;
    My_SDA(1);
    for (uint8_t i = 0; i < 8; i++)
    {
        My_SCL(1);
        if(I2C_R_SDA()==1)
        {
            receivebyte |= (0x80 >> i);
        }
        My_SCL(0);
    }
    return receivebyte;
}

void I2C_SendAck(uint8_t bit)
{
    My_SDA(bit);
    My_SCL(1);
    My_SCL(0);
}

uint8_t I2C_ReceiveAck(void)
{
    uint8_t AckBit; 
    My_SDA(1);
    My_SCL(1);
    AckBit = I2C_R_SDA();
    My_SCL(0);
    return AckBit;
}

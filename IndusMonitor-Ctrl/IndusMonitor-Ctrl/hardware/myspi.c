#include "gpio.h"

void MySPI_W_SS(uint8_t BitValue)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, (GPIO_PinState)(BitValue)); 
}


void MySPI_W_SCK(uint8_t BitValue)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, (GPIO_PinState)(BitValue));
}

void MySPI_W_MOSI(uint8_t BitValue)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, (GPIO_PinState)(BitValue));
}

uint8_t MySPI_R_MISO(void)
{
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14); 
}


void MySPI_Init(void)
{
    MySPI_W_SS(1);   
    MySPI_W_SCK(0);
}


void MySPI_Start(void)
{
    MySPI_W_SS(0); 
}


void MySPI_Stop(void)
{
    MySPI_W_SS(1); 
}


uint8_t MySPI_SwapByte(uint8_t ByteSend)
{

    for (uint8_t i = 0; i < 8; i++) 
    {
        MySPI_W_MOSI(ByteSend & 0x80);
        ByteSend <<= 1;
        MySPI_W_SCK(1);
        if (MySPI_R_MISO() == 1)
        {
            ByteSend |= 0x01;
        } 
          
        MySPI_W_SCK(0); 
    }

    return ByteSend;
}

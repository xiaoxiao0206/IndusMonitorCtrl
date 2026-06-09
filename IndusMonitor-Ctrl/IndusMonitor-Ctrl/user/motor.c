#include "gpio.h"
#include "tim.h"
uint16_t widows_step_number;

uint16_t Widows_Toggle_Pin;

// 101

void widows_control(uint16_t data)
{
	
	uint8_t cmd = data / 10;
	widows_step_number = (data % 10) * 1600;
	switch (cmd)
	{
	case 10:
			HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2,GPIO_PIN_RESET);
			Widows_Toggle_Pin = GPIO_PIN_3;
		break;
	case 11:
			HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2,GPIO_PIN_SET);
			Widows_Toggle_Pin = GPIO_PIN_3;
		break;
	
	case 20:
			HAL_GPIO_WritePin(GPIOG, GPIO_PIN_4,GPIO_PIN_RESET);
			Widows_Toggle_Pin = GPIO_PIN_5;
		break;
	case 21:
			HAL_GPIO_WritePin(GPIOG, GPIO_PIN_4,GPIO_PIN_SET);
			Widows_Toggle_Pin = GPIO_PIN_5;
		break;
	
	default:
        break;
    }
 HAL_TIM_Base_Start_IT(&htim1);
}	



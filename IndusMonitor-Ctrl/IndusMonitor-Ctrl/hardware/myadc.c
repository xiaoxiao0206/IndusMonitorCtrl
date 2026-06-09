#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "dma.h"

uint16_t data[2] = {0};

void My_ADC_Init(void)
{
    HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)data,3);
}

float collect_smoke(void)
{
    return data[0]*3.3f/4095.0f;
}



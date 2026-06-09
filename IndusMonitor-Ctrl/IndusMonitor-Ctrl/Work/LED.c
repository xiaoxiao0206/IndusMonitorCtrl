/*
控制屋内灯光开关的模块
*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "queue.h"
#include "usart.h"
#include "stdio.h"
#include <stdlib.h>
#include "string.h"

#define LIGHT_ON 1
#define LIGHT_OFF 0
#define LIGHT_Error 2

struct room_light
{
  uint8_t room_id;
  GPIO_TypeDef *gpio;
  uint16_t gpio_Pin;
  uint8_t room_cmd;
};
struct room_light room_light_data[] = {
    {1, GPIOC, GPIO_PIN_1},
    {2, GPIOC, GPIO_PIN_2},
    {3, GPIOC, GPIO_PIN_3}};

    
#define LIGHT_DEVICE_COUNT (sizeof(room_light_data) / sizeof(struct room_light))

uint16_t light_controlall(uint16_t flag)
{
  uint8_t flag_id = flag / 10;
  uint8_t flag_room_cmd = flag % 10;
  uint8_t count;
  for (uint8_t i = 0; i < LIGHT_DEVICE_COUNT; i++)
  {
    if (room_light_data[i].room_id == flag_id)
    {
      if (flag_room_cmd == LIGHT_ON)
      {
        HAL_GPIO_WritePin(room_light_data[i].gpio,
                          room_light_data[i].gpio_Pin,
                          GPIO_PIN_SET);
      }

      if (flag_room_cmd == LIGHT_OFF)
      {
        HAL_GPIO_WritePin(room_light_data[i].gpio,
                          room_light_data[i].gpio_Pin,
                          GPIO_PIN_RESET);
      }
    }
    count = 1;
    }

    if (count)
    {
      count = 0;
      return flag + 1000; /* code */
    }
    else
    {
      return (flag ^ 1) + 1000;
    }
}


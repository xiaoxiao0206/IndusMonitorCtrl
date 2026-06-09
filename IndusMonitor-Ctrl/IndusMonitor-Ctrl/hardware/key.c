#include "key.h"

/*按键数量*/
#define KEY_NUM  5
Key_t key_val[KEY_NUM] = {0};
#define KEY_DEBOUNCE_MS    20      // 消抖阈值（20ms）
#define KEY_LONG_PRESS_MS  400     // 长按阈值（400ms）
#define TIMER_PERIOD_MS    10      // 定时器扫描周期（10ms）
/*定义读取函数*/
#define KEY1_READ() HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_11)//左
#define KEY2_READ() HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_12)//右
#define KEY3_READ() HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_13)//上
#define KEY4_READ() HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_14)//下
#define KEY5_READ() HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_15)//OK


void Key_Scan(Key_t *key, uint8_t pin)
{
    switch (key->state)
    {
    case KEY_IDLE:
        if(pin == 0)
        {
            key->count += TIMER_PERIOD_MS;
            if (key->count >= KEY_DEBOUNCE_MS) 
							{ 
                key->state = KEY_DOWN;
                key->count = 0; 
              }
        } 
				else 
				{
            key->count = 0;
        }       
        break;

    case KEY_DOWN:
        
        if (pin == 1)
        {
             key->count += TIMER_PERIOD_MS;
            if (key->count >= KEY_DEBOUNCE_MS) 
							{ 
                key->key_value = 1; // 1=短按
                key->state = KEY_RELEASE;
                key->count = 0;
              }
        } else if (key->count >= KEY_LONG_PRESS_MS) 
				{ 
            key->key_value = 2; // 2=长按
            key->state = KEY_REPEAT;
            key->count = 0;
        }
        break;

    case KEY_REPEAT:
			 key->count += TIMER_PERIOD_MS;
        if (pin == 1)
        {
					if(key->count >= KEY_DEBOUNCE_MS)
					{
            key->state = KEY_RELEASE;
            key->count = 0;
					}
        }
        break;

    case KEY_RELEASE:        
            key->state = KEY_IDLE;
						 key->count = 0;
        break;
    }
}

void Key_ScanAll(void)
{
    Key_Scan(&key_val[0], KEY1_READ());
    Key_Scan(&key_val[1], KEY2_READ());
    Key_Scan(&key_val[2], KEY3_READ());
    Key_Scan(&key_val[3], KEY4_READ());
		Key_Scan(&key_val[4], KEY5_READ());
}

uint8_t Get_key_val(uint8_t index)
{
    uint8_t val = key_val[index].key_value;
    key_val[index].key_value = 0;
    return val;
}

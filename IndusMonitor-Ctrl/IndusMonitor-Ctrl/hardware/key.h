#ifndef __KEY_H
#define __KEY_H

#include "gpio.h"

/*按键状态*/
typedef enum
{
    KEY_IDLE,
    KEY_DOWN,
    KEY_REPEAT,
    KEY_RELEASE
} KeyState;
/*相关值*/
typedef struct
{
    KeyState state;
    uint16_t count;
    uint8_t key_value;
} Key_t;

void Key_Scan(Key_t *key, uint8_t pin);

void Key_ScanAll(void);

uint8_t Get_key_val(uint8_t index);

#endif

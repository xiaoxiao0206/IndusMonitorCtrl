#ifndef __MYGUI_H
#define __MYGUI_H

#include "user.h"


#define MY_TEMP_SYMBOL "\xEF\x8B\x89"
#define MY_HUMD_SYMBOL "\xEF\x9D\xB3"
#define MY_SMOKE_SYMBOL "\xEF\x81\xAD"
#define MY_LIGHT_SYMBOL "\xEF\x9B\x84"

typedef struct{
	uint16_t temp_value;
	uint16_t humd_value;
	uint16_t smoke_value;
	uint16_t light_value;
} updata;

void gui_init(void);

void my_GUI(void);

 void create_top_btn(uint8_t wifi_value);

void create_temp_label(void);

void text_updata(void);
#endif

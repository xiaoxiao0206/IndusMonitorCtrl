/*
                   LCD模块                STM32单片机				   
	 本程序使用的是模拟SPI接口驱动
     可自由更改接口IO配置，使用任意最少4 IO即可完成本款液晶驱动显示
	 
       VCC         接          3.3V/5V      //电源
       GND         接          GND          //电源地
       SDA/DIN     接          PB15         //液晶屏SPI总线数据写信号
       BLK         接          PB9          //液晶屏背光控制信号，如果不需要控制，接3.3V
       SCL/SCK     接          PB13         //液晶屏SPI总线时钟信号
       DC          接          PB10         //液晶屏数据/命令控制信号
       RES         接          PB12         //液晶屏复位控制信号
       CS          接          PB11         //液晶屏片选控制信号
*/

#ifndef __LCD_H
#define __LCD_H	

#include "main.h"
#include "gpio.h"  
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

//LCD重要参数集
typedef struct  
{										    
	u16 width;			//LCD 宽度
	u16 height;			//LCD 高度
	u16 id;				  //LCD ID
	u8  dir;			  //横屏还是竖屏控制：0，竖屏；1，横屏。	
	u16	 wramcmd;		//开始写gram指令
	u16  setxcmd;		//设置x坐标指令
	u16  setycmd;		//设置y坐标指令	
  u8   xoffset;    
  u8	 yoffset;
}_lcd_dev; 	

//LCD参数
extern _lcd_dev lcddev;	//管理LCD重要参数
/////////////////////////////////////用户配置区///////////////////////////////////	 
#define USE_HORIZONTAL  	 0 //定义液晶屏顺时针旋转方向 	0-0度旋转，1-90度旋转，2-180度旋转，3-270度旋转

//////////////////////////////////////////////////////////////////////////////////	  
//定义LCD的尺寸
#define LCD_W 240
#define LCD_H 240

//TFTLCD部分外要调用的函数		   
extern u16  POINT_COLOR;//默认红色    
extern u16  BACK_COLOR; //背景颜色.默认为白色

////////////////////////////////////////////////////////////////////
//-----------------LCD端口定义---------------- 
#define GPIO_TYPE  GPIOE  //GPIO组类型
#define LED      0        //背光控制引脚        PB9
#define LCD_CS   12       //片选引脚            PB11
#define LCD_RS   11       //寄存器/数据选择引脚 PB10 
#define LCD_RST  10       //复位引脚            PB12



#define LCD_CTRL   	  	GPIOE		
#define SPI_SCLK        GPIO_PIN_8	//SCL
#define SPI_MISO        GPIO_PIN_14	
#define SPI_MOSI        GPIO_PIN_9	//SDA

//液晶控制口置1操作语句宏定义

#define	SPI_MOSI_SET  	LCD_CTRL->BSRR=SPI_MOSI    
#define	SPI_SCLK_SET  	LCD_CTRL->BSRR=SPI_SCLK    


//液晶控制口置0操作语句宏定义

#define	SPI_MOSI_CLR  	LCD_CTRL->BRR=SPI_MOSI    
#define	SPI_SCLK_CLR  	LCD_CTRL->BRR=SPI_SCLK    


//QDtech全系列模块采用了三极管控制背光亮灭，用户也可以接PWM调节背光亮度
#define	LCD_LED PBout(LED) //LCD背光    		 PB9
//如果使用官方库函数定义下列底层，速度将会下降到14帧每秒，建议采用我司推荐方法
//以下IO定义直接操作寄存器，快速IO操作，刷屏速率可以达到28帧每秒！ 

//GPIO置位（拉高）
#define	LCD_CS_SET  GPIO_TYPE->BSRR=1<<LCD_CS    //片选端口  	PB11
#define	LCD_RS_SET	GPIO_TYPE->BSRR=1<<LCD_RS    //数据/命令  PB10	  
#define	LCD_RST_SET	GPIO_TYPE->BSRR=1<<LCD_RST   //复位			  PB12

//GPIO复位（拉低）							    
#define	LCD_CS_CLR  GPIO_TYPE->BRR=1<<LCD_CS     //片选端口  	PB11
#define	LCD_RS_CLR	GPIO_TYPE->BRR=1<<LCD_RS     //数据/命令  PB10	 
#define	LCD_RST_CLR	GPIO_TYPE->BRR=1<<LCD_RST    //复位			  PB12

//画笔颜色
#define WHITE       0xFFFF
#define BLACK      	0x0000	  
#define BLUE       	0x001F  
#define BRED        0XF81F
#define GRED 			 	0XFFE0
#define GBLUE			 	0X07FF
#define RED         0xF800
#define MAGENTA     0xF81F
#define GREEN       0x07E0
#define CYAN        0x7FFF
#define YELLOW      0xFFE0
#define BROWN 			0XBC40 //棕色
#define BRRED 			0XFC07 //棕红色
#define GRAY  			0X8430 //灰色
#define GRAY0       0xEF7D 
#define GRAY1       0x8410      	//灰色1      00000 000000 00000
#define GRAY2       0x4208 
//GUI颜色

#define DARKBLUE      	 0X01CF	//深蓝色
#define LIGHTBLUE      	 0X7D7C	//浅蓝色  
#define GRAYBLUE       	 0X5458 //灰蓝色
//以上三色为PANEL的颜色 
 
#define LIGHTGREEN     	0X841F //浅绿色
#define LIGHTGRAY     0XEF5B //浅灰色(PANNEL)
#define LGRAY 			 		0XC618 //浅灰色(PANNEL),窗体背景色

#define LGRAYBLUE      	0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE          0X2B12 //浅棕蓝色(选择条目的反色)
	    															  
void LCD_Init(void);//初始化	
void LCD_Clear(u16 Color);	 
void LCD_SetCursor(u16 Xpos, u16 Ypos);
void LCD_DrawPoint(u16 x,u16 y);//画点
void LCD_SetWindows(u16 xStar, u16 yStar,u16 xEnd,u16 yEnd);								    
void LCD_WriteReg(u8 LCD_Reg, u16 LCD_RegValue);
void LCD_WR_DATA(u8 data);
void LCD_WriteRAM_Prepare(void);
void Lcd_WriteData_16Bit(u16 Data);
void LCD_direction(u8 direction );
  		 
#endif  
	 

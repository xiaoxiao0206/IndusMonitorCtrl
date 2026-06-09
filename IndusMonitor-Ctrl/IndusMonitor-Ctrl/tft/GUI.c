#include "lcd.h"
#include "string.h"
#include "font.h" 
#include "tim.h"
#include "gui.h"


/*******************************************************************
* @name:void GUI_DrawPoint(u16 x,u16 y,u16颜色)
@日期:2018-08-09
* @function:在LCD屏幕上画一个点
@parameters:x:点的x坐标
Y:点的Y坐标
颜色:点的颜色值
* @retvalue:无
********************************************************************/
void GUI_DrawPoint(u16 x,u16 y,u16 color)
{
	LCD_SetCursor(x,y);//设置光标位置 
	Lcd_WriteData_16Bit(color); 
}

/*******************************************************************
* @name:void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color)
@日期:2018-08-09
* @function:填充指定区域
* @parameters:sx:指定区域的起始x坐标
Sy:指定区域的起始y坐标
例如:指定区域的结束x坐标
Ey:指定区域的结束y坐标
Color:填充颜色值
* @retvalue:无
********************************************************************/
void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color)
{  	
	u16 i,j;			
	u16 width=ex-sx+1; 		//得到填充的宽度
	u16 height=ey-sy+1;		//高度
	LCD_SetWindows(sx,sy,ex,ey);//设置显示窗口
	for(i=0;i<height;i++)
	{
		for(j=0;j<width;j++)
		Lcd_WriteData_16Bit(color);	//写入数据 	 
	}
	LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1);//恢复窗口设置为全屏
}

/*******************************************************************
* @name:void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
@日期:2018-08-09
@function:在两点之间画一条线
* @parameters:x1:直线的起始x坐标
Y1:直线的起始y坐标
X2:直线的结束x坐标
Y2:直线结束的y坐标
* @retvalue:无
********************************************************************/
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 

	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //设置单步方向 
	else if(delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//水平线 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//画线输出 
	{  
		LCD_DrawPoint(uRow,uCol);//画点 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
} 

/*****************************************************************************
* @name:void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
@日期:2018-08-09
* @function:画一个矩形
* @parameters:x1:矩形的起始x坐标
Y1:矩形的起始y坐标
X2:矩形的结束x坐标
Y2:矩形的结束y坐标
* @retvalue:无
******************************************************************************/
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_DrawLine(x1,y1,x2,y1);
	LCD_DrawLine(x1,y1,x1,y2);
	LCD_DrawLine(x1,y2,x2,y2);
	LCD_DrawLine(x2,y1,x2,y2);
}  

/*****************************************************************************
* @name:void LCD_DrawFillRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
@日期:2018-08-09
* @function:填充矩形
* @parameters:x1:填充矩形的起始x坐标
Y1:填充矩形的起始y坐标
X2:填充矩形的结束x坐标
Y2:填充矩形的结束y坐标
* @retvalue:无
******************************************************************************/
void LCD_DrawFillRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_Fill(x1,y1,x2,y2,POINT_COLOR);
}
 
/*****************************************************************************
* @name:void _draw_circle_8(int xc, int yc, int x, int y, u16 c)
@日期:2018-08-09
* @function:8对称圆绘制算法(内部调用)
* @parameters:xc:圆心的x坐标
yc:圆心的y坐标
x:相对于圆心的x坐标
y:相对于圆心的y坐标
C:圆的颜色值
* @retvalue:无
******************************************************************************/  
void _draw_circle_8(int xc, int yc, int x, int y, u16 c)
{
	GUI_DrawPoint(xc + x, yc + y, c);

	GUI_DrawPoint(xc - x, yc + y, c);

	GUI_DrawPoint(xc + x, yc - y, c);

	GUI_DrawPoint(xc - x, yc - y, c);

	GUI_DrawPoint(xc + y, yc + x, c);

	GUI_DrawPoint(xc - y, yc + x, c);

	GUI_DrawPoint(xc + y, yc - x, c);

	GUI_DrawPoint(xc - y, yc - x, c);
}

/*****************************************************************************
* @name:void gui_circle(int xc, int yc, u16c,int r, int fill)
@日期:2018-08-09
* @function:在指定位置绘制指定大小的圆
* @parameters:xc:圆心的x坐标
yc:圆心的y坐标
接待员:圆半径
填充:1-filling,没有填充
* @retvalue:无
******************************************************************************/
void gui_circle(int xc, int yc,u16 c,int r, int fill)
{
	int x = 0, y = r, yi, d;

	d = 3 - 2 * r;


	if (fill) 
	{
		// 如果填充（画实心圆）
		while (x <= y) {
			for (yi = x; yi <= y; yi++)
				_draw_circle_8(xc, yc, x, yi, c);

			if (d < 0) {
				d = d + 4 * x + 6;
			} else {
				d = d + 4 * (x - y) + 10;
				y--;
			}
			x++;
		}
	} else 
	{
		// 如果不填充（画空心圆）
		while (x <= y) {
			_draw_circle_8(xc, yc, x, y, c);
			if (d < 0) {
				d = d + 4 * x + 6;
			} else {
				d = d + 4 * (x - y) + 10;
				y--;
			}
			x++;
		}
	}
}

/*****************************************************************************
* @name:void gui_circle(int xc, int yc, u16c,int r, int fill)
@日期:2018-08-09
* @function:在指定位置绘制指定大小的圆
* @parameters:xc:圆心的x坐标
yc:圆心的y坐标
接待员:圆半径
填充:1-filling,没有填充
* @retvalue:无
******************************************************************************/
void Draw_Triangel(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2)
{
	LCD_DrawLine(x0,y0,x1,y1);
	LCD_DrawLine(x1,y1,x2,y2);
	LCD_DrawLine(x2,y2,x0,y0);
}

static void _swap(u16 *a, u16 *b)
{
	u16 tmp;
  tmp = *a;
	*a = *b;
	*b = tmp;
}

/*****************************************************************************
* @name:void Fill_Triangel(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2)
@日期:2018-08-09
* @function:在指定位置填充三角形
@parameters:x0:三角形边的起始x坐标
Y0:三角形边的起始y坐标
X1:三角形的顶点x坐标
Y1:三角形的顶点y坐标
X2:三角形边的末端x坐标
Y2:结束的y坐标
******************************************************************************/ 
void Fill_Triangel(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2)
{
	u16 a, b, y, last;
	int dx01, dy01, dx02, dy02, dx12, dy12;
	long sa = 0;
	long sb = 0;
 	if (y0 > y1) 
	{
    _swap(&y0,&y1); 
		_swap(&x0,&x1);
 	}
 	if (y1 > y2) 
	{
    _swap(&y2,&y1); 
		_swap(&x2,&x1);
 	}
  if (y0 > y1) 
	{
    _swap(&y0,&y1); 
		_swap(&x0,&x1);
  }
	if(y0 == y2) 
	{ 
		a = b = x0;
		if(x1 < a)
    {
			a = x1;
    }
    else if(x1 > b)
    {
			b = x1;
    }
    if(x2 < a)
    {
			a = x2;
    }
		else if(x2 > b)
    {
			b = x2;
    }
		LCD_Fill(a,y0,b,y0,POINT_COLOR);
    return;
	}
	dx01 = x1 - x0;
	dy01 = y1 - y0;
	dx02 = x2 - x0;
	dy02 = y2 - y0;
	dx12 = x2 - x1;
	dy12 = y2 - y1;
	
	if(y1 == y2)
	{
		last = y1; 
	}
  else
	{
		last = y1-1; 
	}
	for(y=y0; y<=last; y++) 
	{
		a = x0 + sa / dy01;
		b = x0 + sb / dy02;
		sa += dx01;
    sb += dx02;
    if(a > b)
    {
			_swap(&a,&b);
		}
		LCD_Fill(a,y,b,y,POINT_COLOR);
	}
	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);
	for(; y<=y2; y++) 
	{
		a = x1 + sa / dy12;
		b = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;
		if(a > b)
		{
			_swap(&a,&b);
		}
		LCD_Fill(a,y,b,y,POINT_COLOR);
	}
}

/*****************************************************************************
* @name:void LCD_ShowChar(u16 x,u16 y,u16 fc, u16 bc, u8 num,u8 size,u8 mode)
@日期:2018-08-09
* @function:显示单个英文字符
@parameters:x:字符显示位置的起始x坐标
y:字符显示位置的起始y坐标
Fc:显示字符的颜色值
Bc:显示字符的背景色
Num:显示字符的ASCII码(0~94)
尺寸:显示尺寸c
******************************************************************************/ 
void LCD_ShowChar(u16 x,u16 y,u16 fc, u16 bc, u8 num,u8 size,u8 mode)
{  
    u8 temp;
    u8 pos,t;
	u16 colortemp=POINT_COLOR;      
		   
	num=num-' ';//得到偏移后的值
	LCD_SetWindows(x,y,x+size/2-1,y+size-1);//设置单个文字显示窗口
	if(!mode) //非叠加方式
	{		
		for(pos=0;pos<size;pos++)
		{
			if(size==12)temp=asc2_1206[num][pos];//调用1206字体
			else temp=asc2_1608[num][pos];		 //调用1608字体
			for(t=0;t<size/2;t++)
		    {                 
		        if(temp&0x01)Lcd_WriteData_16Bit(fc); 
				else Lcd_WriteData_16Bit(bc); 
				temp>>=1; 
				
		    }
			
		}	
	}else//叠加方式
	{
		for(pos=0;pos<size;pos++)
		{
			if(size==12)temp=asc2_1206[num][pos];//调用1206字体
			else temp=asc2_1608[num][pos];		 //调用1608字体
			for(t=0;t<size/2;t++)
		    {   
				POINT_COLOR=fc;              
		        if(temp&0x01)LCD_DrawPoint(x+t,y+pos);//画一个点    
		        temp>>=1; 
		    }
		}
	}
	POINT_COLOR=colortemp;	
	LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1);//恢复窗口为全屏    	   	 	  
}

/*****************************************************************************
* @name:void LCD_ShowString(u16 x,u16 y,u8大小，u8 *p,u8模式)
@日期:2018-08-09
* @function:显示英文字符串
* @parameters:x:英文字符串的起始x坐标
y:英文字符串的起始y坐标
p:英文字符串的起始地址
大小:显示字符的大小
模式:没有覆盖,1-overlying
* @retvalue:无
******************************************************************************/   	  
void LCD_ShowString(u16 x,u16 y,u8 size,u8 *p,u8 mode)
{         
    while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
    {   
		if(x>(lcddev.width-1)||y>(lcddev.height-1)) 
		return;     
        LCD_ShowChar(x,y,POINT_COLOR,BACK_COLOR,*p,size,mode);
        x+=size/2;
        p++;
    }  
} 

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//在指定位置通过取模可显示汉字（取模后也可显示表情）
//x:0~127
//y:0~63
//num：在gbk_1616[num][t]所对应的位置
//mode:0,反白显示;1,正常显示				 
//size:选择字体 12/16/24
void OLED_ShowGBK(u16 x,u16 y,u8 num,u8 size,u8 mode)
{      			    
	u8 temp,t,t1;
	u16 y0=y;
	u8 csize=(size/8+((size%8)?1:0))*size;		//得到字体一个字符对应点阵集所占的字节数
	
    for(t=0;t<csize;t++)
    {   
			//只定定义了16，12 ,24号字体，没有生名其他字体
		if(size==12)temp=gbk_1212[num][t]; 	 	//调用1206字体
		else if(size==16)temp=gbk_1616[num][t];	//调用1608字体
		else if(size==24)temp=gbk_2424[num][t];	//调用1608字体
		//else if(size==24)temp=asc2_2412[chr][t];	//调用2412字体
		else return;								//没有的字库
        for(t1=0;t1<8;t1++)
		{			    
			if(temp&0x80)GUI_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)GUI_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			y++;
			if(y>=lcddev.height)return;		//超区域了
			if((y-y0)==size)
			{
				y=y0;
				x++;
				if(x>=lcddev.width)return;	//超区域了
				break;
			}	
    }          
}
}

//连续显示汉字（取模后也可显示表情）
//x:0~127
//y:0~63
//num：字数
//size:选择字体 12/16/24
//mode:0,反白显示;1,正常显示				 

void OLED_ShowGBK_String(u16 x,u16 y,u8 num,u8 size,u8 mode)
{      			    
	uint8_t i=0;
	for(i=0;i<num;i++)
	{
		OLED_ShowGBK(x,y,i,size,mode);
		if(size==12)
		{
			x=x+12;
		}
		else if(size==16)
		{
			x=x+16;
		}
		else if(size==24)
		{
			x=x+24;
		}
	}
}


//显示图片
//x:0~127
//y:0~63
//index帧数
//clom宽度，row高度
void show_picture(u16 x, u16 y,u16 column,u16 row,unsigned short *pic)
{
	u16 m,h;
	u16 *data=(u16*)pic;
	for(h=0+y;h<row+y;h++)
	{
		for(m=0+x;m<column+x;m++)
		{
			GUI_DrawPoint(m,h,*data++);
		}
	}
}
	
/////////////////////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
* @name:u32 mypow(u8 m,u8 n)
@日期:2018-08-09
* @function:获取m的n次幂(内部调用)
* @parameters:m:乘数
护士:权力
@retvalue: m的n次幂
******************************************************************************/ 
u32 mypow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}

/*****************************************************************************
* @name:void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size)
@日期:2018-08-09
* @function:显示号码
* @parameters:x:数字的起始x坐标
Y:数字的起始Y坐标
num: (0 ~ 4294967295)
Len:显示号的长度
尺寸:显示数字的大小
* @retvalue:无
******************************************************************************/  			 
void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size)
{         	
	u8 t,temp;
	u8 enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				LCD_ShowChar(x+(size/2)*t,y,POINT_COLOR,BACK_COLOR,' ',size,0);
				continue;
			}else enshow=1; 
		 	 
		}
	 	LCD_ShowChar(x+(size/2)*t,y,POINT_COLOR,BACK_COLOR,temp+'0',size,0); 
	}
} 

/*****************************************************************************
* @name:void GUI_DrawFont16(u16 x, u16 y, u16 fc, u16 bc, u8 *s,u8模式)
@日期:2018-08-09
* @function:显示单个16x16中文字符
* @parameters:x:汉字的起始x坐标
y:汉字的起始坐标
fc:汉字的颜色值
汉字的底色
s:汉字的起始地址
模式:没有覆盖,1-overlying
* @retvalu
******************************************************************************/ 
void GUI_DrawFont16(u16 x, u16 y, u16 fc, u16 bc, u8 *s,u8 mode)
{
	u8 i,j;
	u16 k;
	u16 HZnum;
	u16 x0=x;
	HZnum=sizeof(tfont16)/sizeof(typFNT_GB16);	//自动统计汉字数目
	
			
	for (k=0;k<HZnum;k++) 
	{
	  if ((tfont16[k].Index[0]==*(s))&&(tfont16[k].Index[1]==*(s+1)))
	  { 	LCD_SetWindows(x,y,x+16-1,y+16-1);
		    for(i=0;i<16*2;i++)
		    {
				for(j=0;j<8;j++)
		    	{	
					if(!mode) //非叠加方式
					{
						if(tfont16[k].Msk[i]&(0x80>>j))	Lcd_WriteData_16Bit(fc);
						else Lcd_WriteData_16Bit(bc);
					}
					else
					{
						POINT_COLOR=fc;
						if(tfont16[k].Msk[i]&(0x80>>j))	LCD_DrawPoint(x,y);//画一个点
						x++;
						if((x-x0)==16)
						{
							x=x0;
							y++;
							break;
						}
					}

				}
				
			}
			
			
		}				  	
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}

	LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1);//恢复窗口为全屏  
} 

/*****************************************************************************
* @name:void GUI_DrawFont24(u16 x, u16 y, u16 fc, u16 bc, u8 *s,u8 mode)
@日期:2018-08-09
* @function:显示单个24x24中文字符
* @parameters:x:汉字的起始x坐标
y:汉字的起始坐标
fc:汉字的颜色值
汉字的底色
s:汉字的起始地址
模式:没有覆盖,1-overlying
* @retvalu
******************************************************************************/ 
void GUI_DrawFont24(u16 x, u16 y, u16 fc, u16 bc, u8 *s,u8 mode)
{
	u8 i,j;
	u16 k;
	u16 HZnum;
	u16 x0=x;
	HZnum=sizeof(tfont24)/sizeof(typFNT_GB24);	//自动统计汉字数目
		
			for (k=0;k<HZnum;k++) 
			{
			  if ((tfont24[k].Index[0]==*(s))&&(tfont24[k].Index[1]==*(s+1)))
			  { 	LCD_SetWindows(x,y,x+24-1,y+24-1);
				    for(i=0;i<24*3;i++)
				    {
							for(j=0;j<8;j++)
							{
								if(!mode) //非叠加方式
								{
									if(tfont24[k].Msk[i]&(0x80>>j))	Lcd_WriteData_16Bit(fc);
									else Lcd_WriteData_16Bit(bc);
								}
							else
							{
								POINT_COLOR=fc;
								if(tfont24[k].Msk[i]&(0x80>>j))	LCD_DrawPoint(x,y);//画一个点
								x++;
								if((x-x0)==24)
								{
									x=x0;
									y++;
									break;
								}
							}
						}
					}
					
					
				}				  	
				continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
			}

	LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1);//恢复窗口为全屏  
}

/*****************************************************************************
* @name:void GUI_DrawFont32(u16 x, u16 y, u16 fc, u16 bc, u8 *s,u8 mode)
@日期:2018-08-09
* @function:显示单个32x32中文字符
* @parameters:x:汉字的起始x坐标
y:汉字的起始坐标
fc:汉字的颜色值
汉字的底色
s:汉字的起始地址
模式:没有覆盖,1-overlying
* @retvalu
******************************************************************************/ 
void GUI_DrawFont32(u16 x, u16 y, u16 fc, u16 bc, u8 *s,u8 mode)
{
	u8 i,j;
	u16 k;
	u16 HZnum;
	u16 x0=x;
	HZnum=sizeof(tfont32)/sizeof(typFNT_GB32);	//自动统计汉字数目
	for (k=0;k<HZnum;k++) 
			{
			  if ((tfont32[k].Index[0]==*(s))&&(tfont32[k].Index[1]==*(s+1)))
			  { 	LCD_SetWindows(x,y,x+32-1,y+32-1);
				    for(i=0;i<32*4;i++)
				    {
						for(j=0;j<8;j++)
				    	{
							if(!mode) //非叠加方式
							{
								if(tfont32[k].Msk[i]&(0x80>>j))	Lcd_WriteData_16Bit(fc);
								else Lcd_WriteData_16Bit(bc);
							}
							else
							{
								POINT_COLOR=fc;
								if(tfont32[k].Msk[i]&(0x80>>j))	LCD_DrawPoint(x,y);//画一个点
								x++;
								if((x-x0)==32)
								{
									x=x0;
									y++;
									break;
								}
							}
						}
					}
					
					
				}				  	
				continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
			}
	
	LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1);//恢复窗口为全屏  
} 

/*****************************************************************************
* @name:void Show_Str(u16 x, u16 y, u16 fc, u16 bc, u8 *str,u8 size,u8 mode)
@日期:2018-08-09
* @function:显示中英文字符串
* @parameters:x:中文和英文字符串的起始x坐标
y:中、英文字符串的起始y坐标
fc:中、英文字符串的颜色值
bc:中、英文字符串的背景色
str:中文和英文字符串的起始地址
******************************************************************************/	   		   
void Show_Str(u16 x, u16 y, u16 fc, u16 bc, u8 *str,u8 size,u8 mode)
{					
	u16 x0=x;							  	  
  	u8 bHz=0;     //字符或者中文 
    while(*str!=0)//数据未结束
    { 
        if(!bHz)
        {
			if(x>(lcddev.width-size/2)||y>(lcddev.height-size)) 
			return; 
	        if(*str>0x80)bHz=1;//中文 
	        else              //字符
	        {          
		        if(*str==0x0D)//换行符号
		        {         
		            y+=size;
					x=x0;
		            str++; 
		        }  
		        else
				{
					if(size>16)//字库中没有集成12X24 16X32的英文字体,用8X16代替
					{  
					LCD_ShowChar(x,y,fc,bc,*str,16,mode);
					x+=8; //字符,为全字的一半 
					}
					else
					{
					LCD_ShowChar(x,y,fc,bc,*str,size,mode);
					x+=size/2; //字符,为全字的一半 
					}
				} 
				str++; 
		        
	        }
        }else//中文 
        {   
			if(x>(lcddev.width-size)||y>(lcddev.height-size)) 
			return;  
            bHz=0;//有汉字库    
			if(size==32)
			GUI_DrawFont32(x,y,fc,bc,str,mode);	 	
			else if(size==24)
			GUI_DrawFont24(x,y,fc,bc,str,mode);	
			else
			GUI_DrawFont16(x,y,fc,bc,str,mode);
				
	        str+=2; 
	        x+=size;//下一个汉字偏移	    
        }						 
    }   
}

/*****************************************************************************
* @name:void Gui_StrCenter(u16 x, u16 y, u16 fc, u16 bc, u8 *str,u8 size,u8 mode)
@日期:2018-08-09
* @function:中英文字符串居中显示
* @parameters:x:中文和英文字符串的起始x坐标
y:中、英文字符串的起始y坐标
fc:中、英文字符串的颜色值
bc:中、英文字符串的背景色
str:中文和E的起始地址
******************************************************************************/ 
void Gui_StrCenter(u16 x, u16 y, u16 fc, u16 bc, u8 *str,u8 size,u8 mode)
{
	u16 len=strlen((const char *)str);
	u16 x1=(lcddev.width-len*8)/2;
	Show_Str(x1,y,fc,bc,str,size,mode);
} 
 
/*****************************************************************************
* @name:void Gui_Drawbmp16(u16 x,u16 y,const unsigned char *p)
@日期:2018-08-09
* @function:显示16位BMP图像
* @parameters:x: BMP图像的起始x坐标
y: BMP图像的起始y坐标
P:图像数组的起始地址
* @retvalue:无
******************************************************************************/
void Gui_Drawbmp16(u16 x,u16 y,const unsigned char *p) //显示40*40 QQ图片
{
  	int i; 
	unsigned char picH,picL; 
	LCD_SetWindows(x,y,x+40-1,y+40-1);//窗口设置
    for(i=0;i<40*40;i++)
	{	
	 	picL=*(p+i*2);	//数据低位在前
		picH=*(p+i*2+1);				
		Lcd_WriteData_16Bit(picH<<8|picL);  						
	}	
	LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1);//恢复显示窗口为全屏	
}

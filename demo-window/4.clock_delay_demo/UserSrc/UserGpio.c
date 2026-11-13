
/********************************************************************
* 文件名:  ls1d6_std_UserGpio.c
* 版本：	  V1.0
* 作者:    李鑫
* 日期:    2021.11.05
* 说明:    本文件包含Gpio用户自定义初始化的所有接口函数及宏定义
* 版权:    龙芯中科（太原）技术有限公司
********************************************************************/

#include "UserGpio.h"

void GPIOInit(void)
{
 	GPIO_InitTypeDef GPIO_InitStruct;
 	/* 将GPIO 8 和 GPIO9 复用为串口0，GPIO10进而GPIO11复用为串口1*/
	GPIO_PinRemapConfig((GPIO_Remap_UART0 & GPIO_Remap_UART1), ENABLE);
}

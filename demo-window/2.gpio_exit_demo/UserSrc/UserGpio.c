
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
 	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;								//LED灯所在端口
 	GPIO_InitStruct.GPIO_Mode = GPIO_MAIN;								//端口作为IO口方式使用
 	GPIO_InitStruct.GPIO_Direction = GPIO_Mode_Out;						//端口配置为输出
 	GPIO_BitInit(GPIOA_BIT, &GPIO_InitStruct);							//端口寄存器配置
 	GPIO_WriteBit(GPIOA_BIT,GPIO_InitStruct.GPIO_Pin,Bit_RESET);		//端口初始值配置

 	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;								//外部中断IO口所在端口
 	GPIO_InitStruct.GPIO_Mode = GPIO_MAIN;								//端口作为IO口方式使用
 	GPIO_InitStruct.GPIO_Direction = GPIO_Mode_IN;						//端口配置为输入
 	GPIO_BitInit(GPIOA_BIT, &GPIO_InitStruct);							//端口寄存器配置

 	GPIO_PinRemapConfig((GPIO_Remap_UART0 & GPIO_Remap_UART1), ENABLE);	//串口0 和串口1端口使能
}


/********************************************************************
* 文件名:  ls1d6_std_UserUart.c
* 版本：	  V1.0
* 作者:    李鑫
* 日期:    2021.11.05
* 说明:    本文件包含Uart用户自定义初始化的所有接口函数及宏定义
* 版权:    龙芯中科（太原）技术有限公司
********************************************************************/

#include "UserUart.h"

void UartInit(void)
{
	UART_InitTypeDef UART_InitStruct;
	UART_StructInit(&UART_InitStruct);

	if(CLOCK_GetFlagStatus(CLOCK_HSE_SELECT_STATUS) == RESET)
	{
		UART_InitStruct.Clock_Freq = CLOCK_GetClocksFreq();
	}
	UART_InitStruct.UART_BaudRate = 115200;
	UART_Init(UART0,&UART_InitStruct);
	UART_ITConfig(UART0,UART_IT_IRE,ENABLE);
	UART_ITCmd(UART0,ENABLE);

	UART_StructInit(&UART_InitStruct);
	UART_Init(UART1,&UART_InitStruct);
	UART_ITConfig(UART1,UART_IT_IRE,ENABLE);
	UART_ITCmd(UART1,ENABLE);

	CLOCK_DelayMs(15);

}


void UartTestTask(void)
{

}

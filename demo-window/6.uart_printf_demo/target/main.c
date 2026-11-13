#define GLOBAL

#include"Main.h"

uint32 debugflag;	//工作状态输出
 


int main(void)
{
	EXTI_InitTypeDef  EXTI_InitStruct = {0};
	WDG_SetWatchDog(0x80007fff);//设置看门狗为最长时间
	SystemClock();				//时钟设置
	GPIOInit();		/* GPIO 初始化 */
	UartInit();		/* 串口初始化 */

	uint32 inter_freq;
	while(1)
	{
		printf("uart 0 send a char\r\n");
		CLOCK_DelayMs(50);
		UART_SendChar(UART0,'C');
		CLOCK_DelayMs(50);
		printf("uart 1 send a char\r\n");
		UART_SendChar(UART1,'C');
		CLOCK_DelayMs(500);
		WDG_DogFeed();			//喂狗
	}

	return 0;
}



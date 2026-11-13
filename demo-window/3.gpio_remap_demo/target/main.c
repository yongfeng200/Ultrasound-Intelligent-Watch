#define GLOBAL

#include"Main.h"

uint32 debugflag;	//工作状态输出


extern uint8 g_Uart0Cnt;
extern uint8 g_Rx0Buf[100];
uint8 bit_sta;
int main(void)
{
	int i = 0;

	uint32 reset_source = POWER_GetResetSource();

	EXTI_InitTypeDef  EXTI_InitStruct = {0};
	WDG_SetWatchDog(0x80007fff);//设置看门狗为最长时间
	SystemClock();				//时钟设置

	/* 将uart0、uart1 的gpio进行复用，将IIC复用为普通IO管脚 */
	GPIOInit();					//IO口初始化
	UartInit();					//uart0、uart1初始化 115200 N 8 1

	printf("reset,%x\r\n",reset_source);
	printf("\r\ninte8m: %d,osc32K:%x,timing:%x\r\n",inte8m,inte32K,*(uint32_t *)0x1feb0000);

	if(CLOCK_GetFlagStatus(CLOCK_HSE_FAIL) != RESET)
	{
		printf(" ext8m fail\r\n");
	}
	if(CLOCK_GetFlagStatus(CLOCK_HSE_SELECT_STATUS) == RESET)
	{
		printf(" ext8m not select\r\n");
	}
	if(CLOCK_GetFlagStatus(CLOCK_LSE_FAIL) != RESET)
	{
		printf("ext32K fail\r\n");
	}
	if(CLOCK_GetFlagStatus(CLOCK_LSE_SELECT_STATUS) == RESET)
	{
		printf("ext32K not select,try restart!\r\n");
		CLOCK_DelayS(20);
	}

	g_Uart0Cnt = 0;
	EXTI_EnableInt();

	while(1)
	{
		if(g_Uart0Cnt != 0)
		{
			for(i= 0;i<g_Uart0Cnt;i++)
			{
				printf("%c",g_Rx0Buf[i]);//将收到的串口数据原样返回
			}
			g_Uart0Cnt = 0;
		}
		bit_sta=GPIO_ReadInputDataBit(GPIOA_BIT,GPIO_Pin_13);
		if (bit_sta==0x00)
		{
			printf("GPIO_Pin_13 input\r\n");	//若检测到IO口输入为低，则通过串口打印状态
			CLOCK_DelayMs(100);
		}
		WDG_DogFeed();			//喂狗
	}

	return 0;
}



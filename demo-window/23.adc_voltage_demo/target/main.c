#define GLOBAL

#include"Main.h"

uint32 debugflag;	//工作状态输出


int main(void)
{
	uint32 reset_time;	//看门狗复位等待时间

	debugflag=0;

	reset_time =100;
	WDG_SetWatchDog(reset_time);//设置看门狗为最长时间
	SystemClock();				//时钟设置
	GPIOInit();		/* GPIO 初始化 */
	UartInit(115200,9600);			/* 串口初始化 */

	printf("debug\r\n");
 	ADC_Init();

	while(1)
	{
		ADC_Sample(ADC_CHANNEL_ADC1);
		CLOCK_DelayMs(500);
		WDG_DogFeed();			//喂狗
	}

	return 0;
}




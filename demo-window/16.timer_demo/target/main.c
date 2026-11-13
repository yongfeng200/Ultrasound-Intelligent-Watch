#define GLOBAL

#include"Main.h"

uint32 debugflag;	//工作状态输出

extern BOOL timer_interrupt ;

int main(void)
{
	uint32 reset_time;	//看门狗复位等待时间
	uint32 temp;	//看门狗复位等待时间
	uint8_t tep_Buff[8];

	debugflag=0;

	reset_time =100;
	WDG_SetWatchDog(reset_time);//设置看门狗为最长时间
	SystemClock();				//时钟设置
	GPIOInit();		/* GPIO 初始化 */
	UartInit(115200,9600);		/* 串口初始化 */

	/*使能全局中断*/
	EXTI_EnableInt();
	printf("--EXTI_EnableInt--\r\n");	


	TimerInit();
	printf("--TimerInit--\r\n");	
	timer_interrupt = 0;
	while(1)
	{

		if 	(timer_interrupt == 1)
		{
			timer_interrupt = 0;
			printf("--timer interrupt--\r\n");
		}
		WDG_DogFeed();			//喂狗
	}

	return 0;
}



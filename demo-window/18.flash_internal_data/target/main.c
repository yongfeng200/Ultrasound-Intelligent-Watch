#define GLOBAL

#include"Main.h"

uint32 debugflag;	//工作状态输出


int main(void)
{
	uint32 reset_time;	//看门狗复位等待时间
	uint32 Wdata;	//看门狗复位等待时间

	debugflag=0;

	reset_time =100;
	WDG_SetWatchDog(reset_time);//设置看门狗为最长时间
	SystemClock();				//时钟设置
	GPIOInit();		/* GPIO 初始化 */
	UartInit(115200,9600);		/* 串口初始化 */
	Wdata = 0;
	while(1)
	{
		printf("\r\n---FLASH_WiteTest-----\r\n");
     	FLASH_WiteTest(126976,Wdata);
     	CLOCK_DelayS(2);
		printf("\r\n---FLASH_ReadTest-----\r\n");
		FLASH_ReadTest(126976);
		CLOCK_DelayS(2);
		Wdata++;
		WDG_DogFeed();			//喂狗
	}

	return 0;
}




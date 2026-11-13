#define GLOBAL

#include"Main.h"

uint32 debugflag;	//工作状态输出
RTC_time show_time;


int main(void)
{
	uint32 reset_time;	//看门狗复位等待时间
	debugflag=0;

	reset_time =100;
	WDG_SetWatchDog(reset_time);//设置看门狗为最长时间
	SystemClock();				//时钟设置
	GPIOInit();		/* GPIO 初始化 */
	UartInit(115200,9600);		/* 串口初始化 */

	printf("debug\r\n");

	while(1)
	{
		printf("RamOn\r\n");
		POWER_RamOffConfig(RESET);
		CLOCK_DelayS(5);
		printf("SonarOn\r\n");
		POWER_SonarOffConfig(RESET);
		CLOCK_DelayS(5);
		printf("SpiOn\r\n");
		POWER_SpiLPOffConfig(RESET);
		CLOCK_DelayS(5);
		WDG_DogFeed();			//喂狗

		printf("RamOff\r\n");
		POWER_RamOffConfig(SET);
		CLOCK_DelayS(5);
		printf("SonarOff\r\n");
		POWER_SonarOffConfig(SET);
		CLOCK_DelayS(5);
		printf("SpiOff\r\n");
		POWER_SpiLPOffConfig(SET);
		CLOCK_DelayS(5);
		WDG_DogFeed();			//喂狗
	}

	return 0;
}




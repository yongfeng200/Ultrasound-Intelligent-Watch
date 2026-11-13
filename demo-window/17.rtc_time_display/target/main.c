#define GLOBAL

#include"Main.h"

uint32 debugflag;	//工作状态输出
extern BOOL RTC_int ;



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

	/*RTC测试程序*/
	RTC_time write_test;
	RTCInit();
	RTC_int=0;
	while(1)
	{
		RTC_GetDateTime(&write_test);
		write_test.sec=write_test.sec+1;
		RTC_SetAlarm(&write_test);
/*		tep_Buff[5]=write_test.year;
		tep_Buff[4]=write_test.mon;
		tep_Buff[3]=write_test.day;
		tep_Buff[2]=write_test.hour;
		tep_Buff[1]=write_test.min;
		tep_Buff[0]=write_test.sec;*/
//		printf("20%d-%d-%d %d:%d:%d\r\n",tep_Buff[5],tep_Buff[4],tep_Buff[3],tep_Buff[2],tep_Buff[1],tep_Buff[0]);
		while 	(RTC_int == 0)
		{
			WDG_DogFeed();			//喂狗
			CLOCK_DelayMs(10);
		}
		Screen_Time(&write_test.sec);
		RTC_int = 0;
		printf("--rtc interrupt--\r\n");
		WDG_DogFeed();			//喂狗
	}

	return 0;
}



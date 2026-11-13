#define GLOBAL

#include"Main.h"

uint32 debugflag;	//工作状态输出
RTC_time show_time;



void lcd_test()
{

    LCD_Dis_Init();
    CLOCK_DelayMs(2000);
    LCD_Dis_Cls();
    CLOCK_DelayMs(2000);
}
int main(void)
{
	uint32 reset_time;	//看门狗复位等待时间
	debugflag=0;

	reset_time =100;
	WDG_SetWatchDog(reset_time);//设置看门狗为最长时间
	SystemClock();				//时钟设置
	GPIOInit();		/* GPIO 初始化 */
	UartInit(115200,9600);			/* 串口初始化 */
        GPIO_WriteBit(GPIOA_BIT,GPIO_Pin_2,Bit_SET);
	printf("debug\r\n");

	while(1)
	{
		
		lcd_test();
		CLOCK_DelayMs(5000);
		WDG_DogFeed();			//喂狗
	}

	return 0;
}




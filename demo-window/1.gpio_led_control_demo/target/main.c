#define GLOBAL

#include"Main.h"
#define LED0_GPIO    GPIO_Pin_3

uint32 debugflag;	//工作状态输出

void Led_On(void)
{
	GPIO_WriteBit(GPIOA_BIT,LED0_GPIO,Bit_RESET);	//IO口低电平点亮LED
	printf("\r\nled on\r\n");							//串口0 输出
}

void Led_Off(void)
{
	GPIO_WriteBit(GPIOA_BIT,LED0_GPIO,Bit_SET);		//IO口高电平熄灭LED
	printf("\r\nled off\r\n");							//串口0 输出

}

int main(void)
{
	WDG_SetWatchDog(0x80007fff);//设置看门狗为最长时间
	SystemClock();				//时钟设置
	GPIOInit();					//IO口初始化
	UartInit();					//串口0、串口1初始化 115200 N 8 1
	CLOCK_DelayMs(1);

	while(1)
	{
		Led_On();				//LED灯点亮
		CLOCK_DelayMs(500);		//延时500毫秒
		Led_Off();				//LED灯熄灭
		CLOCK_DelayMs(500);		//延时500毫秒
		WDG_DogFeed();			//喂狗
	}

	return 0;
}



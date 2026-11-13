#define GLOBAL

#include"Main.h"

uint32 debugflag;	//工作状态输出

void Led_On(void)
{
	 GPIO_WriteBit(GPIOA_BIT,GPIO_Pin_3,Bit_SET);
}

void Led_Off(void)
{
	GPIO_WriteBit(GPIOA_BIT,GPIO_Pin_3,Bit_RESET);
}



int main(void)
{
	EXTI_InitTypeDef  EXTI_InitStruct = {0};
	WDG_SetWatchDog(0x80007fff);//设置看门狗为最长时间
	SystemClock();				//时钟设置

	while(1)
	{
		Led_On();
		CLOCK_DelayMs(500);
		Led_Off();
		CLOCK_DelayMs(500);
		WDG_DogFeed();			//喂狗
	}

	return 0;
}



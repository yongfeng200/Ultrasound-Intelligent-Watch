#define GLOBAL

#include"Main.h"
#define LED0_GPIO    GPIO_Pin_3

uint32 debugflag;	//工作状态输出
BOOL Exint_flag;	//外部中断来临标志位翻转

void Led_On(void)
{
	GPIO_WriteBit(GPIOA_BIT,LED0_GPIO,Bit_RESET);	//IO口低电平点亮LED
	printf("led on\r\n");							//串口0 输出
}

void Led_Off(void)
{
	GPIO_WriteBit(GPIOA_BIT,LED0_GPIO,Bit_SET);		//IO口高电平熄灭LED
	printf("led off\r\n");							//串口0 输出

}

int main(void)
{
	EXTI_InitTypeDef  EXTI_InitStruct = {0};
	WDG_SetWatchDog(0x80007fff);//设置看门狗为最长时间
	SystemClock();				//时钟设置
	GPIOInit();					//IO口初始化
	UartInit();					//串口0、串口1初始化 115200 N 8 1

	EXTI_EnableInt();

 	EXTI_StructInit(&EXTI_InitStruct);
 	EXTI_InitStruct.EXTI_GPIO = GPIO_Pin_13;
 	EXTI_InitStruct.EXTI_GpioCmd = ENABLE;
 	EXTI_Init(&EXTI_InitStruct);
	CLOCK_DelayMs(1);
	Exint_flag=0;
	BOOL ledflag=0;
	while(1)
	{
		if (Exint_flag==1)
		{
			Exint_flag=0;
			if  (ledflag==0)
			{
				ledflag = 1;
				Led_On();				//LED灯点亮
			}
			else
			{
				ledflag = 0;
				Led_Off();				//LED灯熄灭
			}
			printf("key\r\n");					//串口0 输出
			CLOCK_DelayMs(500);		//延时500毫秒
		}
		else
		{
			CLOCK_DelayMs(5);		//延时5毫秒
		}
		WDG_DogFeed();			//喂狗
	}

	return 0;
}



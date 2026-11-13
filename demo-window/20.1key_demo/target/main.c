#define GLOBAL

#include"Main.h"
#include"GlobalDef.h"
uint32 debugflag;	
uint8_t key_mode;
uint32_t key_delay_Counter;
uint32_t key_counter;
uint32_t systick;



int main(void)
{
	uint32 reset_time;	
	EXTI_InitTypeDef  EXTI_InitStruct = {0};

	debugflag=0;

	reset_time =100;
	WDG_SetWatchDog(reset_time);
	SystemClock();			
	GPIOInit();		
	UartInit(115200,9600);	
	TimerInit();
	key_mode = 0;
	key_delay_Counter = 0;
	key_counter = 0;
	systick = 0;

 	EXTI_StructInit(&EXTI_InitStruct);
 	EXTI_InitStruct.EXTI_GPIO = GPIO_Pin_13;
 	EXTI_InitStruct.EXTI_GpioCmd = ENABLE;
 	EXTI_Init(&EXTI_InitStruct);
 	printf("\r\n---EXTI->EXINT_EDGE=%d-----\r\n",EXTI->EXINT_EDGE);
 	printf("\r\n---EXTI->EXINT_POL=%d-----\r\n",EXTI->EXINT_POL);


 	EXTI_EnableInt();

	while(1)
	{

		if (key_mode == LongKey)
		{
			key_mode=NoKey;
			printf("\r\n---LongKey-----\r\n");

		}
		else if (key_mode == ShortKey)
		{
			key_mode=NoKey;
			printf("\r\n---ShortKey-----\r\n");
		}
		CLOCK_DelayMs(500);
		WDG_DogFeed();			
	}

	return 0;
}




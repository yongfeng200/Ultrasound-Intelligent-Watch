#define GLOBAL

#include"Main.h"
#include"GlobalDef.h"
uint32 debugflag;	
uint8_t key_mode;
uint32_t key_delay_Counter;
uint32_t key_counter;
uint32_t systick;
BOOL Button_state;
int main(void)
{
	uint32 reset_time;	//看门狗时间

	debugflag=0;

	reset_time =100;
	WDG_SetWatchDog(reset_time);//看门狗时间设置
	SystemClock();				//时钟初始化
	GPIOInit();		/* GPIO 初始化*/
	UartInit(115200,9600);		/*串口初始化*/
	TimerInit();//定时器初始化
	key_mode = 0;
	key_delay_Counter = 0;
	key_counter = 0;
	systick = 0;
	Button_state = 0;
	CLEAR_BIT(CHIPCTRL, 1<< 20);//按下和释放都触发中断
 	MEASURE_ITConfig(MEASURE_INT_KEY,ENABLE);//使能按键中断
	/*使能全局中断*/
 	EXTI_EnableInt();


	while(1)
	{
		if ((key_mode == NoKey)&&(key_counter>0))
		{
			if ((key_counter-key_delay_Counter)>KeyButtonLongThreshold)
			{
				printf("\r\n---LongKey-----\r\n");
			}
			else
			{
				printf("\r\n---ShortKey-----\r\n");
			}
			key_counter = 0;
			key_delay_Counter = 0;
			key_mode = 0;
			systick = 0;
		}
//		printf("\r\n---key_counter=%d-----\r\n",key_counter);
//		printf("\r\n---key_delay_Counter=%d-----\r\n",key_delay_Counter);
//		printf("\r\n---systick=%d-----\r\n",systick);
//		printf("\r\n---Button_state=%d-----\r\n",Button_state);

		CLOCK_DelayMs(500);
		WDG_DogFeed();			//喂狗
	}

	return 0;
}




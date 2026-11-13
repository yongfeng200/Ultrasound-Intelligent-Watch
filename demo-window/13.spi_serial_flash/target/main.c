
#include"Main.h"

uint32 inte8m;
uint32_t key_mode;

int data_test = 3;
extern void FlashTestTask(void);
uint32 debugflag;	//工作状态输出
int main(void)
{
	int i = 0;
 	SPI_InitTypeDef SPI_InitStruct;

	uint32 reset_source = POWER_GetResetSource();

	WDG_SetWatchDog(0x80007fff);//设置看门狗为最长时间

	/*软件全局变量初始化*/
	SystemClock();	//时钟设置
	GPIOInit();		/* GPIO 初始化 */
	UartInit(115200,9600);

	printf("reset,%x\r\n",reset_source);
	printf("\r\ninte8m: %d,osc32K:%x,timing:%x\r\n",inte8m,inte32K,*(uint32_t *)0x1feb0000);

	if(CLOCK_GetFlagStatus(CLOCK_HSE_FAIL) != RESET)
	{
		printf(" ext8m fail\r\n");
	}
	if(CLOCK_GetFlagStatus(CLOCK_HSE_SELECT_STATUS) == RESET)
	{
		printf(" ext8m not select\r\n");
	}
	if(CLOCK_GetFlagStatus(CLOCK_LSE_FAIL) != RESET)
	{
		printf("ext32K fail\r\n");
	}
	if(CLOCK_GetFlagStatus(CLOCK_LSE_SELECT_STATUS) == RESET)
	{
		printf("ext32K not select,try restart!\r\n");
		CLOCK_DelayS(20);
	}

	/*使能全局中断*/
	EXTI_EnableInt();

 	SPI_StructInit(&SPI_InitStruct);
 	SPI_Init(SPI,&SPI_InitStruct);
	printf("debug = %d\r\n",debugflag);

	MainTask = FlashTestTask;
	uint32 inter_freq;
	while(1)
	{
	    MainTask();
	    CLOCK_DelayS(5);
		WDG_DogFeed();			//喂狗
	}

	return 0;
}










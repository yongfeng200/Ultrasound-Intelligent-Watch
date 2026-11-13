
#include"Main.h"

uint32 inte8m;
uint32_t key_mode;

int data_test = 3;
uint32 debugflag;	//工作状态输出
int main(void)
{
	int i = 0;
	uint8 Tdata;
	uint8 Rdata;
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



	I2cInit();
	printf("debug = %d\r\n",debugflag);

	uint32 inter_freq;
	Tdata=0;
	Rdata=0;
	while(1)
	{
		CAT24_Write(0x05,Tdata);
    	printf("---Tdata-----%d--------\r\n",Tdata);
    	CLOCK_DelayMs(500);
		Rdata=CAT24_Read(0x05);
		printf("---Rdata-----%d--------\r\n",Rdata);
		CLOCK_DelayMs(500);
		Tdata++;
		WDG_DogFeed();			//喂狗
	}

	return 0;
}











#include"Main.h"

uint32 inte8m;
uint32_t key_mode;

int data_test = 3;
extern uint8_t resvalid_flag;
extern int gate2_no;
extern int gate2_preno;
int main(void)
{
	int i = 0;
	uint32_t reg;
	uint32 reset_source = POWER_GetResetSource();
	int new_max = 1426;
	int target_y = 1344;
	int old_max = 1426;
	int fhl_clcen = 1;
	int res = 0;

	WDG_SetWatchDog(0x80007fff);//设置看门狗为最长时间

	/*软件全局变量初始化*/
	SystemClock();	//时钟设置
	GPIOInit();		/* GPIO 初始化 */
	UartInit(115200,2400);
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

	resvalid_flag = 0;
	/*使能全局中断*/
	EXTI_EnableInt();

	MEASURE_DefaultInit();

	MEASURE_ClearTdcResReg();
	MEASURE_ClearTdcCalReg();
	MEASURE_ITConfig(MEASURE_INT_RES_READ,ENABLE);
	MEASURE_ClearResITAndReadFlag();
	/* 巡航模式 */
//	MEASURE_SetCruiseMode(MEASURE_CRUISE_MODE_TOF);

	uint32_t tof_start_f = 0, tof_start_b = 0, tof_f = 0,tof_b = 0;
	uint32_t sum_DTOF,avr_DTOF,time_DTOF;
	uint32_t manual_avrDTof,manual_timeDTof;
	int j,peak[15], order[14], fhl;	
	float wakepeakk,wakepeakb;
	int wakepeak;
	MeasureTdcCal_StateTypeDef tdc_cal;

	while(1)
	{
		if (fhl_clcen == 1)
		{
			fhl_clcen = 0;
			for (i=0;i<10;i++)
			{
				//自动设置首波阈值
				res =fhl_init_lite(1,1,1);
				if(res==1)
				{
					GPIO_Uart0Resume();
					fhl =MEASURE_GetPulseFirstWaveThreshold();
 					//读取保存首波阈值
					printf("fhl0=%d\r\n " ,fhl);
					break;
				}
			}
			if(res==0)
			{
				CLOCK_DelayMs(1);
				//读取保存首波阈值
				MEASURE_SetPulseFirstWaveThreshold(852);
				break;
			}
            //开启时差巡航
            MEASURE_SetCruiseMode(MEASURE_CRUISE_MODE_TOF);

		}
		else
		{
			if(resvalid_flag) /*  */
			{
				resvalid_flag=0;
				if(MEASURE_ExpectResPtr() == ERROR)
				{
					printf("resptr is error \n");
					continue;
				}
				if(MEASURE_GetTdcCal(&tdc_cal) != SUCCESS)
				{
					printf("tdc cad get error \n");
					continue;
				}

				MEASURE_WriteTdcCalToReg(&tdc_cal);

				tof_start_f =  MEASURE_GetStartTime(&tdc_cal,ForwardDirction);
				tof_start_b =  MEASURE_GetStartTime(&tdc_cal,BackwardDirction);
				tof_f = MEASURE_GetResData(&tdc_cal,0,ForwardDirction) - tof_start_f;
				tof_b = MEASURE_GetResData(&tdc_cal,0,BackwardDirction) - tof_start_b;
				tof_f = MEASURE_GetDtofConvertPsValue(tof_f,125000);
				tof_b = MEASURE_GetDtofConvertPsValue(tof_b,125000);

				manual_avrDTof = MEASURE_GetManualDtofValue(&tdc_cal, 15, 1);

				printf("\ntof_f = %d, tof_b =%d ",tof_f,tof_b);
				printf("\ndtof=%d ", manual_avrDTof);
				reg = READ_REG(ADC->ADC_DATA);
				/* 首波之后波的峰值，为ADC采样值 */
				printf("wave_peak%d = %d \r\n", gate2_preno,reg);
			}

		}
		CLOCK_DelayMs(5);
		WDG_DogFeed();			//喂狗
	}

	return 0;
}










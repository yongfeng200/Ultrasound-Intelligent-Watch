/*
 * UserMeasure.c
 *
 *  Created on: 2021年10月18日
 *      Author: lixin
 */

#include "UserMeasure.h"
#include "UserLcd.h"
#include "GlobalDef.h"

/* 驱动内部函数，头文件未导出，这里显式声明 */
extern uint32_t tdc2time(uint32_t *addr, uint32_t cal);
extern uint32_t cal_C(uint32_t a, uint32_t b);

volatile uint8_t resvalid_flag = 0;

pMeasureData MeasureRecord;

void MeasureSonarInit(void)
{
	Measure_InitTypeDef Measure;
	MEASURE_StructInit(&Measure);
	MEASURE_ClockInit(&(Measure.Clock));
	MEASURE_PulseInit(&(Measure.Pulse));
	MEASURE_ControlInit(&(Measure.Control));
	MEASURE_CalibrationInit(&(Measure.Calibration));
	MEASURE_AfeInit(&(Measure.Afe));
	MEASURE_GateWindowInit();
	//MEASURE_DefaultInit();

}

void floatcalc(void)
{
	float a = 1.2;
	float b = 2;
	int c = 0;

	b = a*b;
	c = (int)b;
}
/* amc 峰值采集 */
void MeasureAmcTask()
{
	int i,vref = 0;
	int fwd_adc[16],bwd_adc[16];

	MeasureGate_InitTypeDef gate = {1,  7*8,  0,   1, 0, 0};

	MEASURE_DefaultInit();
	vref = GetVrefValue();
	printf("forward vref = %d\n",i,vref);
	MEASURE_SetAfeVtestSel(MEASURE_AFECTRL_VTEST_SEL_DAC_AMC);
	MEASURE_SetAfeVtestDual(ENABLE);
	MEASURE_SetAfeVtest(ENABLE);
	MEASURE_SetAfeVtestOut(MEASURE_AFECTRL_VTEST_OUT_VAMC);
	MEASURE_SetAfeVRefOut(DISABLE);
	MEASURE_SetPulseWavePeakMode(ENABLE);

    for(;;)
    {
		MEASURE_ClearADCPtrReg();
    	for(i=0;i<16;i++)
    	{
    		gate.DelayValCycle = (i + 1)*8;
    		MEASURE_TdcSetWindow(&gate);
    		MEASURE_SetTaskToStart(MEASURE_MOP_ONCE_TOF);
    		MEASURE_SuspendTask();
    		fwd_adc[i] = (MEASURE_GetAmcValue(ForwardDirction) - vref);
    	}
    	MEASURE_SetMannualDirectionReverse();
    	MEASURE_ClearADCPtrReg();

    	for(i=0;i<16;i++)
    	{
    		gate.DelayValCycle = (i + 1)*8;
    		MEASURE_TdcSetWindow(&gate);
    		MEASURE_SetTaskToStart(MEASURE_MOP_ONCE_TOF);
    		MEASURE_SuspendTask();
    		bwd_adc[i] = (MEASURE_GetAmcValue(BackwardDirction) - vref);
    	}

    	for(i = 0;i <16;i++)
    	{
    		printf("fwd-amc[%d] = %4d  ,vol=%4d mV ",i,fwd_adc[i],fwd_adc[i]*3300/4096);
    		printf("bwd-amc[%d] = %4d ,vol=%4d mV\r\n",i,bwd_adc[i],bwd_adc[i]*3300/4096);
    	}
    	MEASURE_SetMannualDirectionReverse();

//    	if(BreakKey())
//    	{
//    		printf("exit AMC-Task\n");
//    		break;
//    	}
    	//CLOCK_DelayMs(1000);
    }
}

void MeasureVrefTask(void)
{
	uint32_t reg = 0,reg_bk = 0,i = 0;
	int fwd_vref = 0,bwd_vref = 0,vref = 0;
	MEASURE_DefaultInit();
	reg = READ_REG(SONAR->AFE_CTRL);
	reg_bk = reg;
	LS_RINS(reg,MEASURE_AFECTRL_VTEST_SEL,MEASURE_AFECTRL_VTEST_SEL_VREF);
	LS_RINS(reg,MEASURE_AFECTRL_VTEST_SEL,MEASURE_AFECTRL_VTEST_DUAL_ENABLE);
	LS_RINS(reg,MEASURE_AFECTRL_VTEST,MEASURE_AFECTRL_VTEST_ENABLE);
	LS_RINS(reg,MEASURE_AFECTRL_VTEST_OUT,MEASURE_AFECTRL_VTEST_OUT_OTHER);
	LS_RINS(reg,MEASURE_AFECTRL_VREF_OUT,MEASURE_AFECTRL_VREF_OUT_ENABLE);

	WRITE_REG(SONAR->AFE_CTRL,reg);

	LS_RINS(reg,MEASURE_FWAVECTRL_WAVEP,MEASURE_FWAVECTRL_WAVEP_ENABLE);

	MEASURE_ClearADCPtrReg();

	for(;;)
	{
		for(;i<2;i++)
		{
			MEASURE_SetTaskToStart(MEASURE_MOP_TWICE_TOF);
			MEASURE_SuspendTask();
			fwd_vref += MEASURE_GetAmcValue(ForwardDirction);
			bwd_vref += MEASURE_GetAmcValue(BackwardDirction);
		}
		//MEASURE_StopTask();
		//MEASURE_ExitCruiseTask();
		vref = (fwd_vref + bwd_vref) >> 2;
		printf("vref = %4d , vol=%4d mV\r\n",vref,vref*3300/4096);
		printf("fwd-vref = %4d , vol=%4d mV ",fwd_vref,fwd_vref*3300/4096);
		printf("bwd-vref = %4d , vol=%4d mV\r\n",bwd_vref,bwd_vref*3300/4096);

		MEASURE_ClearADCPtrReg();

//    	if(BreakKey())
//    	{
//    		printf("exit VREF-Task\n");
//    		break;
//    	}
	}

	WRITE_REG(SONAR->AFE_CTRL,reg_bk);

}

void PrintfSonarRegister(void)
{

	int i;
	if (POWER_GetSonarOffMode() == SET) //SonarOff
	{
		printf("\r\nWarning: sonaroff, unable to readreg !!");
	}
	for(i=0; i<12; i++)
	{
		if((i%4) == 0)
		{
			printf("\n0x%08x:", 0x5a00+i*4);
		}
		printf(" %08x", readw((0x5a00+i*4)));
		//printf(" %08x", *(volatile unsigned int*)(0x5a00+i*4));
	}

	printf("\n timing is %x" ,*(uint32_t *)(0x00005800));
	printf("\n command is %x" ,*(uint32_t *)(0x00005804));
	printf("\n chip is %x" ,*(uint32_t *)(0x00005810));
	printf("\n autocalc is %x" ,*(uint32_t *)(0x00005814));
	printf("\n commandW is %x" ,*(uint32_t *)(0x0000583c));
}

float GetfValue(float  bArray, float MeanData)
{

     float sum = bArray;
//    int j = (int)MeanData;
   sum = sum / MeanData;
    return  sum;
}


void MeasureCuriseTask(void)
{
	uint32 fire_num = 32;
	uint32_t Threshold = 780;
	uint8_t hit_num = 12;
	uint8_t flter_num = 3;

	MeasureGate_InitTypeDef gate1 = {0, 57*8, 0, 12*8, 0, 0};	//considering the fastest speed of sound
	MeasureGate_InitTypeDef gate2 = {1,  7*8-1, 0,   1, 0, 1};

	MeasureTdcCal_StateTypeDef tdc_cal;
	//MedianFilterStructData filter;
	uint32_t sum_DTOF,avr_DTOF,time_DTOF;
	uint32_t manual_avrDTof,manual_timeDTof;
//     float a = 120.4f;
// 	  float c = 2.1f;
//	 int b =   cal_fq(a,c);
	uint32_t tof_start_f = 0, tof_start_b = 0,tof_f = 0,tof_b = 0;
	//printf("in task\n");
	MEASURE_DefaultInit();
	POWER_SonarOffConfig(RESET);
	MEASURE_SetPulseWaveFireNum(fire_num);
	MEASURE_SetPulseWaveHitNum(hit_num);
	MEASURE_SetPulseFirstWaveThreshold(Threshold);
	MEASURE_SetAutoCalculateMode(DISABLE);
	MEASURE_SetAutoCalculateHitNum(hit_num - 1);
	MEASURE_SetAutoCalculateFltRate(flter_num);
	MEASURE_SetPulseWaveTailEnd(fire_num + 1);
	MEASURE_SetPulseWaveWaitEnd(fire_num + 6);
	MEASURE_TdcSetWindow(&gate1);
	MEASURE_TdcSetWindow(&gate2);
	MEASURE_SetTdcAutoCalPre(DISABLE);
	MEASURE_SetTdcAutoCalPst(ENABLE);
	MEASURE_SetPulseWaveWideMode(DISABLE);
	MEASURE_SetFirstPulseWaveMode(ENABLE);
	MEASURE_SetPulseWaveHitIgnNum(0);
	MEASURE_ClearTdcResReg();
	MEASURE_ClearTdcCalReg();
	MEASURE_ITConfig(MEASURE_INT_RES_READ,ENABLE);
	MEASURE_ClearResITAndReadFlag();
	//MEASURE_SetResReadFlag();
	//MEASURE_ClearIT((MEASURE_INT_WAKE|MEASURE_INT_RES_READ|MEASURE_INT_BAT_FAIL|MEASURE_INT_C32K_FAIL|MEASURE_INT_C8M_FAIL|MEASURE_INT_KEY|MEASURE_INT_RING|MEASURE_INT_RTC));
	MEASURE_SetWatchDog(ENABLE);
	//MEASURE_SetTaskToStart(MEASURE_MOP_TWICE_TOF); //非巡航模式下的启动任务，巡航模式下由芯片内部自己设置
	MEASURE_SetCruiseMode(MEASURE_CRUISE_MODE_TOF);
	//InitMedianFilter(&filter);
	for(;;)
	{
		//PrintfSonarRegister();
		//printf("resvalid_flag = %d\r\n",resvalid_flag);
		if(resvalid_flag) /* 收到数据  */
		{
			resvalid_flag=0;
			if(MEASURE_ExpectResPtr() == ERROR)
			{
				printf("resptr is error \n");
			}
			if(MEASURE_GetTdcCal(&tdc_cal) != SUCCESS)
 			{
 				printf("tdc cad get error \n");
 			}

			 MEASURE_WriteTdcCalToReg(&tdc_cal);

			 tof_start_f =  MEASURE_GetStartTime(&tdc_cal,ForwardDirction);
			 tof_start_b =  MEASURE_GetStartTime(&tdc_cal,BackwardDirction);
			 tof_f = MEASURE_GetResData(&tdc_cal,0,ForwardDirction) - tof_start_f;
			 tof_b = MEASURE_GetResData(&tdc_cal,0,BackwardDirction) - tof_start_b;
			 tof_f = MEASURE_GetDtofConvertPsValue(tof_f,125000);
			 tof_b = MEASURE_GetDtofConvertPsValue(tof_b,125000);
			 //avr_DTOF = MEASURE_GetAutoDtofValue(hit_num,flter_num);
			 //time_DTOF = MEASURE_GetDtofConvertPsValue(avr_DTOF,125000);

			 manual_avrDTof = MEASURE_GetManualDtofValue(&tdc_cal,hit_num,flter_num);
 			 manual_timeDTof = MEASURE_GetDtofConvertPsValue(manual_avrDTof,125000);
			// printf("\r\nDTOF = %d ,time = %d\n",avr_DTOF,time_DTOF);
			//printf("\r\nAVG=%d\n---------", time_DTOF);
			 printf("\r\nmDTOF=%d , Mtime = %d \n", manual_avrDTof,manual_timeDTof);
//			 b = GetfValue(a,b);
//			 printf("\r\nb =%d  \n",b );
			 printf("\r\ntof_f =%d , tof_b =%d\n",tof_f,tof_b);
			 //LcdDisplay(DisplayCapacityMode);
			 //LcdDisplay(NormalMode);



		}
		CLOCK_DelayMs(5);
		WDG_DogFeed();			//喂狗
	}
}

void MeasureWaveWideTask(void)
{
	uint32 fire_num = 32;
	uint32_t Threshold = 512;
	uint8_t hit_num = 12;
	uint8_t flter_num = 3;
	MeasureGate_InitTypeDef gate1 = {0, 57*8, 0, 12*8, 0, 0};	//considering the fastest speed of sound
	MeasureGate_InitTypeDef gate2 = {1,  7*8-1, 0,   1, 0, 1};
	MeasureTdcCal_StateTypeDef tdc_cal;
	MeasurePulseWidth_StateTypeDef wave_width;
	uint32_t sum_DTOF,avr_DTOF,time_DTOF;
	//printf("in task\n");
	MEASURE_DefaultInit();
	POWER_SonarOffConfig(RESET);
	MEASURE_SetPulseWaveFireNum(fire_num);
	MEASURE_SetPulseWaveHitNum(hit_num);
	MEASURE_SetPulseFirstWaveThreshold(Threshold);
	MEASURE_SetAutoCalculateFltRate(flter_num);
	MEASURE_SetPulseWaveTailEnd(fire_num + 1);
	MEASURE_SetPulseWaveWaitEnd(fire_num + 6);
	MEASURE_TdcSetWindow(&gate1);
	MEASURE_TdcSetWindow(&gate2);
	MEASURE_SetTdcAutoCalPre(DISABLE);
	MEASURE_SetTdcAutoCalPst(ENABLE);
	MEASURE_SetPulseWaveWideMode(ENABLE);
	MEASURE_SetFirstPulseWaveMode(ENABLE);
	MEASURE_SetPulseWaveHitIgnNum(0);
	MEASURE_ClearTdcResReg();
	MEASURE_ClearTdcCalReg();
	MEASURE_ClearResPtrReg();
	MEASURE_ITConfig(MEASURE_INT_RES_READ,ENABLE);
	MEASURE_ClearResITAndReadFlag();
	//MEASURE_SetResReadFlag();
	//MEASURE_ClearIT((MEASURE_INT_WAKE|MEASURE_INT_RES_READ|MEASURE_INT_BAT_FAIL|MEASURE_INT_C32K_FAIL|\
				MEASURE_INT_C8M_FAIL|MEASURE_INT_KEY|MEASURE_INT_RING|MEASURE_INT_RTC));
	MEASURE_SetWatchDog(ENABLE);
	//MEASURE_SetTaskToStart(MEASURE_MOP_TWICE_TOF); 非巡航模式下的启动任务，巡航模式下由芯片内部自己设置
	MEASURE_SetCruiseMode(MEASURE_CRUISE_MODE_TOF);

	for(;;)
	{

		//printf("wait\n");
		MEASURE_SuspendTask();

		//printf("resvalid_flag = %d\r\n",resvalid_flag);
		if(resvalid_flag) /* 收到数据 */
		{
			if(MEASURE_ExpectResPtr() == ERROR)
			{
				printf("resptr is error \n");
			}
			if(MEASURE_GetTdcCal(&tdc_cal) != SUCCESS)
			{
				printf("tdc cal get error \n");
			}
			MEASURE_ClearResPtrReg();
			MEASURE_WriteTdcCalToReg(&tdc_cal);
			MEASURE_GetPulseWidth(&wave_width);

			printf("\r\nWidth1=%d ", wave_width.PrePulse1Width);
			printf("\r\nWidth2=%d ", wave_width.PrePulse2Width);
			printf("\r\nfwd width_ratio =%d\n", (wave_width.PrePulse1Width*100)/wave_width.PrePulse2Width);
			printf("\r\nWidth1=%d ", wave_width.PstPulse1Width);
			printf("\r\nWidth2=%d ", wave_width.PstPulse2Width);
			printf("\r\nbwd width_ratio =%d\n", (wave_width.PstPulse1Width*100)/wave_width.PstPulse2Width);
			avr_DTOF  = MEASURE_GetManualDtofValue(tdc_cal.ForwardCal_C,hit_num,flter_num);
			time_DTOF = MEASURE_GetDtofConvertPsValue(avr_DTOF,125000);
			//printf("DTOF = %d ,time = %d\n",avr_DTOF,time_DTOF);
			printf("\r\nAVG=%d\n----------------", time_DTOF);
			//LcdDisplay(DisplayCapacityMode);
			//floatcalc();
		}

	}
}


#if 0

void MeasureTemperatureTask(void)
{
	uint32 res_data0 = 0, res_data1 = 0 ,res_data2 = 0,res_data3 = 0;

	MeasureGate_InitTypeDef gate1 = {0, 57*8, 0, 12*8, 0, 0};	//considering the fastest speed of sound
	MeasureGate_InitTypeDef gate2 = {1,  7*8-1, 0,   1, 0, 1};
	MeasureTdcCal_StateTypeDef tdc_cal;

	uint32_t sum_DTOF,avr_DTOF,time_DTOF;
	//printf("in task\n");
	MEASURE_DefaultInit();
	POWER_SonarOffConfig(RESET);


#if 0
	MEASURE_SetPulseWaveFireNum(fire_num);
	MEASURE_SetPulseWaveHitNum(hit_num);
	MEASURE_SetPulseFirstWaveThreshold(Threshold);
	MEASURE_SetAutoCalculateFltRate(flter_num);
	MEASURE_SetPulseWaveTailEnd(fire_num + 1);
	MEASURE_SetPulseWaveWaitEnd(fire_num + 6);
	MEASURE_TdcSetWindow(&gate1);
	MEASURE_TdcSetWindow(&gate2);
	MEASURE_SetTdcAutoCalPre(DISABLE);

	MEASURE_SetPulseWaveWideMode(ENABLE);
	MEASURE_SetFirstPulseWaveMode(ENABLE);
	MEASURE_SetPulseWaveHitIgnNum(0);
	MEASURE_ClearTdcResReg();
	MEASURE_ClearTdcCalReg();
	MEASURE_ClearResPtrReg();
	MEASURE_ITConfig(MEASURE_INT_RES_READ,ENABLE);
	MEASURE_ClearResITAndReadFlag();
	//MEASURE_SetResReadFlag();
	//MEASURE_ClearIT((MEASURE_INT_WAKE|MEASURE_INT_RES_READ|MEASURE_INT_BAT_FAIL|MEASURE_INT_C32K_FAIL|\
				MEASURE_INT_C8M_FAIL|MEASURE_INT_KEY|MEASURE_INT_RING|MEASURE_INT_RTC));
	MEASURE_SetWatchDog(ENABLE);
	//MEASURE_SetTaskToStart(MEASURE_MOP_TWICE_TOF); 非巡航模式下的启动任务，巡航模式下由芯片内部自己设置
	MEASURE_SetCruiseMode(MEASURE_CRUISE_MODE_TOF);
#endif

	MEASURE_SetTdcAutoCalPst(ENABLE);
	MEASURE_SetTemperatureFakeNum(0);
	MEASURE_SetTemperaturePortNum(0);
	MEASURE_SetTemperaturePortDirection(RESET);
	MEASURE_SetTemperatureTimeLoad(0);
	MEASURE_SetTemperatureTimeUnload(0);
	MEASURE_ClearTdcResReg();
	MEASURE_ClearTdcCalReg();
	MEASURE_ClearResPtrReg();
	MEASURE_ITConfig(MEASURE_INT_RES_READ,ENABLE);
	MEASURE_ClearResITAndReadFlag();

	MEASURE_SetTaskToStart(MEASURE_MOP_ONCE_TEMP); 非巡航模式下的启动任务，巡航模式下由芯片内部自己设置
//	MEASURE_SetCruiseMode(MEASURE_CRUISE_MODE_TEMP);
	for(;;)
	{

		//printf("wait\n");
		MEASURE_SuspendTask();

		//printf("resvalid_flag = %d\r\n",resvalid_flag);
		if(resvalid_flag) /* 收到数据 */
		{
			//if(MEASURE_ExpectResPtr() == ERROR)
			//{
			//	printf("resptr is error \n");
			//}
			tdc_cal.BackwardCal_C = cal_C(StsFwd+4, StsFwd+8);
			printf("pstcal1 = %x\n",*(uint32_t *)(StsFwd));
			printf("pstcal2 = %x\n",*(uint32_t *)(StsFwd+4));
			printf("calC = %d\n",tdc_cal.BackwardCal_C);
			//if(MEASURE_GetTdcCal(&tdc_cal) != SUCCESS)
			//{
			//	printf("tdc cal get error \n");
			//}
			//MEASURE_ClearResPtrReg();
			 MEASURE_WriteTdcCalToReg(&tdc_cal);
			//MEASURE_GetPulseWidth(&wave_width);
			MEASURE_GetResData(ResData0,ForwardDirction);
			printf("\r\nWidth1=%d ", wave_width.PrePulse1Width);
			printf("\r\nWidth2=%d ", wave_width.PrePulse2Width);
			printf("\r\nfwd width_ratio =%d\n", (wave_width.PrePulse1Width*100)/wave_width.PrePulse2Width);
			printf("\r\nWidth1=%d ", wave_width.PstPulse1Width);
			printf("\r\nWidth2=%d ", wave_width.PstPulse2Width);
			printf("\r\nbwd width_ratio =%d\n", (wave_width.PstPulse1Width*100)/wave_width.PstPulse2Width);
			avr_DTOF = MEASURE_GetAutoDtofValue(hit_num,flter_num);
			time_DTOF = MEASURE_GetDtofConvertPsValue(avr_DTOF,125000);
			//printf("DTOF = %d ,time = %d\n",avr_DTOF,time_DTOF);
			printf("\r\nAVG=%d\n----------------", time_DTOF);

			//floatcalc();
		}

	}
}


#endif





const uint8_t hit_num = 12;
const uint8_t flter_num = 3;

void StandardMeasureInit()
{
	uint32 fire_num = 32;
	uint32_t Threshold = 780;
	MeasureGate_InitTypeDef gate1 = {0, 54*8, 0, 30*8, 0, 0};	//considering the fastest speed of sound
	MeasureGate_InitTypeDef gate2 = {1,  7*8-1, 0,   1, 0, 1};
	//MedianFilterStructData filter;
	//printf("in task\n");
	MEASURE_DefaultInit();
	POWER_SonarOffConfig(RESET);
	MEASURE_SetPulseWaveFireNum(fire_num);
	MEASURE_SetPulseWaveHitNum(hit_num);
	MEASURE_SetPulseFirstWaveThreshold(Threshold);
	MEASURE_SetAutoCalculateMode(DISABLE);
	MEASURE_SetAutoCalculateHitNum(hit_num - 1);
	MEASURE_SetAutoCalculateFltRate(flter_num);
	MEASURE_SetPulseWaveTailEnd(fire_num + 1);
	MEASURE_SetPulseWaveWaitEnd(fire_num + 6);
	MEASURE_TdcSetWindow(&gate1);
	MEASURE_TdcSetWindow(&gate2);
	MEASURE_SetTdcAutoCalPre(DISABLE);
	MEASURE_SetTdcAutoCalPst(ENABLE);
	MEASURE_SetPulseWaveWideMode(DISABLE);
	MEASURE_SetFirstPulseWaveMode(ENABLE);
	MEASURE_SetPulseWaveHitIgnNum(0);
	MEASURE_ClearTdcResReg();
	MEASURE_ClearTdcCalReg();
	MEASURE_ITConfig(MEASURE_INT_RES_READ,ENABLE);
	MEASURE_ClearResITAndReadFlag();
	//MEASURE_SetResReadFlag();
	//MEASURE_ClearIT((MEASURE_INT_WAKE|MEASURE_INT_RES_READ|MEASURE_INT_BAT_FAIL|MEASURE_INT_C32K_FAIL|MEASURE_INT_C8M_FAIL|MEASURE_INT_KEY|MEASURE_INT_RING|MEASURE_INT_RTC));
	MEASURE_SetWatchDog(ENABLE);
	resvalid_flag=0;
	// MEASURE_SetTaskToStart(MEASURE_MOP_TWICE_TOF); //非巡航模式下的启动任务，巡航模式下由芯片内部自己设置
	MEASURE_SetCruiseMode(MEASURE_CRUISE_MODE_TOF);
	//InitMedianFilter(&filter);
	//	//添加获取静态的时差零位值程序

}

static int32_t sonar_abs32(int32_t v)
{
	return (v < 0) ? -v : v;
}

/* 飞行时间的合理范围：10~500 µs，用来挡掉“无回波 / 结果寄存器全 1”的数据组。
 * 扣除起始时刻后 Tf、Tr 约 40 µs（62 mm 声程 @ 1482 m/s ≈ 41.8 µs），
 * 该范围留有足够余量，不会误伤正常值。 */
#define SONAR_TOF_MIN_PS   10000000u
#define SONAR_TOF_MAX_PS   500000000u
/* 1 = 用 StsFwd/StsBwd 扣除测量起始时刻，tof 即为真正的飞行时间（约 41 µs）
 * 0 = 不扣除，保持绝对值（约 68 µs），仅用于对比验证
 * 若开启后逆流时间异常，说明 ONCE_TOF 模式下起始时刻寄存器无效，改回 0 */
#define SONAR_FIX_START_TIME  1

uint32_t MeasureDataProcess(uint32_t *fwd_fly_time, uint32_t *bwd_fly_time, int32_t *dtof)
{
	uint32_t tof_start_f = 0, tof_start_b = 0, tof_f = 0, tof_b = 0;
	MeasureTdcCal_StateTypeDef tdc_cal;
	int32_t avr_DTOF, time_DTOF, dtof_hit0;

	if(resvalid_flag) /* 收到数据 */
	{
		resvalid_flag = 0;
		if(MEASURE_ExpectResPtr() == ERROR)
		{
			uint32_t p = MEASURE_GetResPtrReg();
			printf("PTR=%08x TO=%u hitF=%u hitB=%u stF=%u stB=%u\n",
				p, p>>31, p&0xFF, (p>>8)&0xFF, (p>>16)&0xFF, (p>>24)&0xFF);
			return 2;
		}
		if(MEASURE_GetTdcCal(&tdc_cal) != SUCCESS)
		{
			printf("tdc cal get error \n");
			return 2;
		}

		/* 驱动在 ONCE_TOF 模式下有两个缺陷：
		 *   a) MEASURE_GetTdcCal() 只计算 ForwardCal_C，BackwardCal_C 未初始化；
		 *   b) MEASURE_GetStartTime() 不处理 ONCE_TOF，直接返回 0。
		 * 因此 tof_b = tdc2time(ResBwd, BackwardCal_C) 用的是栈上残留值，
		 * 表现为逆流飞行时间整周期跳变，而顺流正常。
		 * 处理：按正向相同方式补齐反向校准值，并直接用 tdc2time 取起始时刻。 */
		tdc_cal.BackwardCal_C = cal_C((uint32_t)StsBwd + 4, (uint32_t)StsBwd + 8);

		MEASURE_WriteTdcCalToReg(&tdc_cal);
#if SONAR_FIX_START_TIME
		tof_start_f = tdc2time((uint32_t *)StsFwd, tdc_cal.ForwardCal_C);
		tof_start_b = tdc2time((uint32_t *)StsBwd, tdc_cal.BackwardCal_C);
#else
		tof_start_f = 0;
		tof_start_b = 0;
#endif
		tof_f = MEASURE_GetResData(&tdc_cal,0,ForwardDirction) - tof_start_f;
		tof_b = MEASURE_GetResData(&tdc_cal,0,BackwardDirction) - tof_start_b;
		tof_f = MEASURE_GetDtofConvertPsValue(tof_f,125000);
		tof_b = MEASURE_GetDtofConvertPsValue(tof_b,125000);

		/* 第一层有效性检查：绝对飞行时间不在 10~500 µs 的组直接丢掉，
		 * 不参与后面的 ΔT 计算，也不送进流量。 */
		if((tof_f < SONAR_TOF_MIN_PS) || (tof_f > SONAR_TOF_MAX_PS) ||
		   (tof_b < SONAR_TOF_MIN_PS) || (tof_b > SONAR_TOF_MAX_PS))
		{
			printf("SONAR_REJECT,%u,%u\r\n", (unsigned)tof_f, (unsigned)tof_b);
			return 2;
		}

		/*
		 * 注意：厂商的 MEASURE_GetManualDtofValue() 最后是 sum_soft >>= 3，
		 * 即按 8 个 hit 求平均；但 12 个 hit 去掉两端各 3 个后应该是 6 个。
		 * 实测打印出来的 DTOF 恰好等于 0.75 × (Tr - Tf)，所以这里乘 4/3 补回来。
		 * 这是“数值缩放”问题，不是滤波问题。
		 */
		avr_DTOF = MEASURE_GetManualDtofValue(&tdc_cal, hit_num, flter_num);
		avr_DTOF = (avr_DTOF * 4) / 3;
		time_DTOF = (int32_t)MEASURE_GetDtofConvertPsValue((uint32_t)avr_DTOF, 125000);

		/* 手动平均与 hit0 的差值正常应在几十 ns 内；差得太多说明滤波/周跳异常，
		 * 退回 hit0 的差值，避免把错误值送进流量计算。 */
		dtof_hit0 = (int32_t)tof_b - (int32_t)tof_f;
		if(sonar_abs32(time_DTOF - dtof_hit0) > 300000)
		{
			time_DTOF = dtof_hit0;
		}

		*fwd_fly_time = tof_f;
		*bwd_fly_time = tof_b;
		*dtof = time_DTOF;
		return 1;
	}

	return 0;
}

/*==============================================================================
 *  逆流整周期跳变修正 + ΔT 五点中值滤波
 *  处理顺序：整周期修正 → 量程检查 → 野值剔除 → 变化率限制 → 中值输出
 *  连续丢弃 5 次自动重置窗口，避免真实流量阶跃后被“锁死”
 *============================================================================*/
#define SONAR_PERIOD_REPAIR      1
#define SONAR_PERIOD_PS          964000L    /* 一个声周期，约 0.964 µs */
#define SONAR_ZERO_PS            2000000L   /* 静水时差零点，换管段/换能器后需重测 */
#define SONAR_REPAIR_MIN_PS      700000L    /* 只修 +0.7~+1.3 个周期的跳变 */
#define SONAR_REPAIR_MAX_PS      1300000L

int32_t SonarRepairDtof(int32_t dtof)
{
#if SONAR_PERIOD_REPAIR
	int32_t d;
	uint8_t guard = 0;

	/* 只修“变大”的跳变：漏掉首波只会让 Tr 变晚、ΔT 变大。
	 * 反向水流会让 ΔT 变小，不能加周期去“修”，否则会把真实反向流量改成正向。 */
	d = dtof - SONAR_ZERO_PS;
	while((d > SONAR_REPAIR_MIN_PS) && (d < SONAR_REPAIR_MAX_PS) && (guard < 2))
	{
		dtof -= SONAR_PERIOD_PS;
		d = dtof - SONAR_ZERO_PS;
		guard++;
	}
#endif
	return dtof;
}

/* ---------------- ΔT 五点时差滤波 ---------------- */
#define SONAR_FILTER_N           5
#define SONAR_DTOF_LIMIT_PS      600000L   /* 相对零点 ±600 ns，约 ±9 m/s */
#define SONAR_DTOF_OUTLIER_PS    300000L   /* 与窗口中值差 >300 ns 视为野值 */
#define SONAR_DTOF_RATE_PS       300000L   /* 相邻输出差 >300 ns 视为跳变 */
#define SONAR_FILTER_RECOVER_N   5         /* 连续拒绝 5 次后重置窗口 */

static int32_t s_dtof_win[SONAR_FILTER_N];
static uint8_t s_dtof_n;
static uint8_t s_dtof_i;
static int32_t s_dtof_last;
static uint8_t s_dtof_has_last;
static uint8_t s_dtof_reject;

void SonarFilterReset(void)
{
	uint8_t i;

	for(i = 0; i < SONAR_FILTER_N; i++) s_dtof_win[i] = 0;
	s_dtof_n = 0;
	s_dtof_i = 0;
	s_dtof_last = 0;
	s_dtof_has_last = 0;
	s_dtof_reject = 0;
}

/* 小数组插入排序取中值；n 最大 5，开销可以忽略 */
static int32_t sonar_median(const int32_t *src, uint8_t n)
{
	int32_t buf[SONAR_FILTER_N];
	int32_t key;
	uint8_t i, j;

	if(n == 0) return 0;
	for(i = 0; i < n; i++) buf[i] = src[i];
	for(i = 1; i < n; i++)
	{
		key = buf[i];
		j = i;
		while((j > 0) && (buf[j - 1] > key))
		{
			buf[j] = buf[j - 1];
			j--;
		}
		buf[j] = key;
	}
	return buf[n / 2];
}

/*
 * 返回 1：本次数据有效，*out 为滤波后的 ΔT（ps）
 * 返回 0：本次数据被剔除，*out 保持上一次的有效值（没有有效值时是 0）
 */
uint8_t SonarFilterDtof(int32_t raw, int32_t *out)
{
	int32_t repaired;
	int32_t med;
	int32_t diff;

	repaired = SonarRepairDtof(raw);

	/* 1) 量程检查：相对静水零点 ±600 ns。超出的组不是正常流量。 */
	if((repaired < (SONAR_ZERO_PS - SONAR_DTOF_LIMIT_PS)) ||
	   (repaired > (SONAR_ZERO_PS + SONAR_DTOF_LIMIT_PS)))
	{
		if(out) *out = s_dtof_last;
		if(++s_dtof_reject >= SONAR_FILTER_RECOVER_N) SonarFilterReset();
		return 0;
	}

	/* 2) 与当前窗口中值比较，差太多就是野值，不放进窗口 */
	if(s_dtof_n >= 3)
	{
		med = sonar_median(s_dtof_win, s_dtof_n);
		diff = sonar_abs32(repaired - med);
		if(diff > SONAR_DTOF_OUTLIER_PS)
		{
			if(out) *out = s_dtof_last;
			if(++s_dtof_reject >= SONAR_FILTER_RECOVER_N) SonarFilterReset();
			else return 0;
		}
	}

	/* 3) 变化率限制：相邻两次输出不允许跳 >300 ns（约 4.6 m/s） */
	if(s_dtof_has_last)
	{
		diff = sonar_abs32(repaired - s_dtof_last);
		if(diff > SONAR_DTOF_RATE_PS)
		{
			if(out) *out = s_dtof_last;
			if(++s_dtof_reject >= SONAR_FILTER_RECOVER_N) SonarFilterReset();
			else return 0;
		}
	}

	/* 4) 通过检查：放进 5 点窗口，输出中值 */
	s_dtof_reject = 0;
	s_dtof_win[s_dtof_i] = repaired;
	s_dtof_i = (uint8_t)((s_dtof_i + 1) % SONAR_FILTER_N);
	if(s_dtof_n < SONAR_FILTER_N) s_dtof_n++;

	med = sonar_median(s_dtof_win, s_dtof_n);
	s_dtof_last = med;
	s_dtof_has_last = 1;
	if(out) *out = med;
	return 1;
}

/*====================== 声波时差 → 瞬时流量 ==========================*/
/*
 *   v = L/(2·cosθ) · [ 1/(Tf−d) − 1/(Tr−d) ]
 *   Q = v · A · K，A = π·D²/4
 *   Tf、Tr 已扣除起始时刻，是纯飞行时间，d 通常接近 0
 */
#define FLOW_GEOMETRY_ENABLE  1         /* 0 = 关闭流量计算，屏上显示 "----" */
#define FLOW_PATH_MM          62.0f     /* 有效声程（水中路径），厂家图纸标注 */
#define FLOW_COS_THETA        0.70710678f  /* cos45°；夹角待核实 */
#define FLOW_PIPE_ID_MM       15.0f     /* 流量管内径；待卡尺实测 */
#define FLOW_PROFILE_K        1.0f      /* 流速分布系数；实流标定后改 0.85~0.90 */
#define FLOW_CAL_K            1.0f      /* 实流标定系数，称重法求得 */
#define FLOW_SIGN             1         /* 顺流为正；方向相反时改 -1 */
#define FLOW_MILLI_MAX        999999    /* 显示上限，防止溢出 */

/* 固定延时 d（ps）：换能器 + 管壁 + 耦合层的剩余传播时间
 * 起始时刻被正确扣除后，原来那 27 µs 的系统偏移已经消失，
 * 剩下的只是换能器/管壁的少量额外延时（通常 0~3 µs）。
 * 修正后先填 0，若实测 Tf 仍明显大于 L/c(T)，把差值填这里。 */
#define FLOW_DELAY_PS         0L

static float s_flow_total_ml = 0.0f;

/**
 * @Description: 查询流量是否可用
 * @return 1 = 几何参数已启用，屏上可以显示流量；0 = 显示 "----"
 */
uint8_t SonarFlowCalibrated(void)
{
	return FLOW_GEOMETRY_ENABLE ? 1 : 0;
}

/**
 * @Description: 用滤波后的 ΔT 计算流速和流量
 * @param fwd_ps   顺流飞行时间，ps
 * @param bwd_ps   逆流飞行时间，ps
 * @param dtof_ps  滤波后的 ΔT = Tr - Tf，ps
 * @param v_mms    输出流速，mm/s
 * @param q_milli  输出流量，0.001 m³/h
 */
void SonarFlowCalc(uint32_t fwd_ps, uint32_t bwd_ps, int32_t dtof_ps,
                   int32_t *v_mms, int32_t *q_milli)
{
	float tf, tr, dt, v, area, q_ml_s, q;

	if(v_mms) *v_mms = 0;
	if(q_milli) *q_milli = 0;
	if(!FLOW_GEOMETRY_ENABLE) return;
	if((fwd_ps == 0) || (bwd_ps == 0)) return;

	tf = (float)fwd_ps;
	tr = (float)bwd_ps;
	dt = (float)(dtof_ps - (int32_t)SONAR_ZERO_PS) * (float)FLOW_SIGN;

	/* 扣掉固定延时，只保留水中飞行时间；否则分母被放大 (Tf/(Tf-d))² 倍 */
	tf -= (float)FLOW_DELAY_PS;
	tr -= (float)FLOW_DELAY_PS;
	if((tf <= 0.0f) || (tr <= 0.0f)) return;

	/* v = L/(2cosθ) · [1/(Tf-d) - 1/(Tr-d)] · 1e12，单位 mm/s */
	v = (FLOW_PATH_MM / (2.0f * FLOW_COS_THETA)) * dt * 1.0e12f / (tf * tr);
	v *= FLOW_PROFILE_K * FLOW_CAL_K;

	area = 3.14159265f * FLOW_PIPE_ID_MM * FLOW_PIPE_ID_MM * 0.25f;
	q_ml_s = v * area * 0.001f;         /* mm³/s -> mL/s */
	q = q_ml_s * 3.6f;                  /* mL/s -> 0.001 m³/h */

	if(q < 0.0f) q = 0.0f;              /* 水表不做反向计量；要反向改成保留符号 */
	if(q > (float)FLOW_MILLI_MAX) q = (float)FLOW_MILLI_MAX;

	if(v_mms) *v_mms = (int32_t)(v + (v >= 0.0f ? 0.5f : -0.5f));
	if(q_milli) *q_milli = (int32_t)(q + 0.5f);
}

/**
 * @Description: 累计体积（mL）
 * @param q_milli 瞬时流量，0.001 m³/h
 * @param dt_ms   本次采样间隔，ms
 */
void SonarFlowAccumulate(int32_t q_milli, uint32_t dt_ms)
{
	if(q_milli <= 0) return;
	/* 0.001 m³/h -> mL/s：q_milli / 3.6 */
	s_flow_total_ml += ((float)q_milli / 3.6f) * (float)dt_ms * 0.001f;
}

uint32_t SonarFlowTotalMl(void)
{
	return (uint32_t)(s_flow_total_ml + 0.5f);
}

void SonarFlowResetTotal(void)
{
	s_flow_total_ml = 0.0f;
}

/* 兼容旧接口：没有 Tf/Tr 时无法用几何公式，返回 0 */
int32_t SonarDtofToFlowMilli(uint32_t dtof)
{
	(void)dtof;
	return 0;
}

/*
 * 逐 hit 打印顺/逆流飞行时间差，用于判断逆流不稳是“整体跳周期”还是“个别离群”
 * 每行 12 个值，单位 ps，分别对应 hit 0~11
 */
void MeasureHitDump(void)
{
	MeasureTdcCal_StateTypeDef cal;
	uint32_t sf, sb, tf, tb;
	uint32_t i;

	if(MEASURE_GetTdcCal(&cal) != SUCCESS)
	{
		printf("HITD,cal_error\r\n");
		return;
	}

	sf = MEASURE_GetStartTime(&cal, ForwardDirction);
	sb = MEASURE_GetStartTime(&cal, BackwardDirction);

	printf("HITD");
	for(i = 0; i < hit_num; i++)
	{
		tf = MEASURE_GetResData(&cal, (MeasureResIndex_StateTypedef)i, ForwardDirction);
		tb = MEASURE_GetResData(&cal, (MeasureResIndex_StateTypedef)i, BackwardDirction);
		tf = MEASURE_GetDtofConvertPsValue(tf - sf, 125000);
		tb = MEASURE_GetDtofConvertPsValue(tb - sb, 125000);
		printf(",%d", (int)(tb - tf));
	}
	printf("\r\n");
}

/*==============================================================================
 *  信号质量 SQ = 落在中值 ±SQ_WINDOW_PS 内的 hit 个数 × 100 / 12
 *  信号强度 = 命中个数 (0~12) + 首波脉宽；真正的回波幅值需切 AMC 模式，不能常开
 *============================================================================*/
#define SQ_WINDOW_PS        400000      /* 判定“抓错周期”的窗口，约半个载波周期 */
#define SQ_MAX_HITS         16

static uint8_t sonar_sq_of(const int32_t *v, uint8_t n, int32_t *pp_out)
{
	int32_t tmp[SQ_MAX_HITS];
	int32_t med, t;
	uint8_t i, j, good = 0;

	if(n == 0) { *pp_out = 0; return 0; }
	if(n > SQ_MAX_HITS) n = SQ_MAX_HITS;

	for(i = 0; i < n; i++) tmp[i] = v[i];
	for(i = 1; i < n; i++)                      /* 插入排序 */
	{
		t = tmp[i];
		for(j = i; (j > 0) && (tmp[j - 1] > t); j--) tmp[j] = tmp[j - 1];
		tmp[j] = t;
	}

	med = tmp[n / 2];
	*pp_out = tmp[n - 1] - tmp[0];

	for(i = 0; i < n; i++)
	{
		if(sonar_abs32(v[i] - med) <= SQ_WINDOW_PS) good++;
	}
	return (uint8_t)((uint32_t)good * 100u / n);
}

/*
 * 计算一次测量的信号强度与信号质量
 * 返回：1 = 计算成功；0 = 取校准值失败
 */
uint8_t MeasureQuality(SonarQuality_t *q)
{
	MeasureTdcCal_StateTypeDef cal;
	MeasurePulseWidth_StateTypeDef w;
	uint32_t sf, sb, rf, rb, p;
	int32_t dtf[SQ_MAX_HITS], dtb[SQ_MAX_HITS];
	uint8_t i, n;

	if(MEASURE_GetTdcCal(&cal) != SUCCESS) return 0;

	sf = MEASURE_GetStartTime(&cal, ForwardDirction);
	sb = MEASURE_GetStartTime(&cal, BackwardDirction);
	n = (hit_num > SQ_MAX_HITS) ? SQ_MAX_HITS : hit_num;

	for(i = 0; i < n; i++)
	{
		rf = MEASURE_GetResData(&cal, (MeasureResIndex_StateTypedef)i, ForwardDirction);
		rb = MEASURE_GetResData(&cal, (MeasureResIndex_StateTypedef)i, BackwardDirction);
		dtf[i] = (int32_t)MEASURE_GetDtofConvertPsValue(rf - sf, 125000);
		dtb[i] = (int32_t)MEASURE_GetDtofConvertPsValue(rb - sb, 125000);
	}

	q->sq_f = sonar_sq_of(dtf, n, &q->pp_f);
	q->sq_b = sonar_sq_of(dtb, n, &q->pp_b);

	p = MEASURE_GetResPtrReg();
	q->hit_f = (uint8_t)(p & 0xFF);
	q->hit_b = (uint8_t)((p >> 8) & 0xFF);

	q->w_f = 0;
	q->w_b = 0;
	if(MEASURE_GetPulseWidth(&w) == SUCCESS)
	{
		q->w_f = w.PrePulse1Width;
		q->w_b = w.PstPulse1Width;
	}
	return 1;
}

uint32_t forward_Q;
uint32_t backward_Q;

/*==============================================================================
 *  PT1000 温度测量：对 RTD 充放电，用 TDC 计放电时长，时长与阻值成正比
 *  测温会改动 MOP 字段，结束必须调用 temp_restore_sonar() 交还测量引擎
 *============================================================================*/

/* ---------------- 调试开关 ---------------- */
#define TEMP_DEBUG_RAW      1       /* 1：打印每次采样的原始寄存器值（排查 PT1/PT2 接线） */
#define TEMP_SHOW_ON_LCD    0       /* 1：测温结果同时刷 LCD（会切到温度页） */

/* ---------------- 采样设置 ---------------- */
#define TEMP_SAMPLE_NUM     8       /* 每次测温采样次数，取中值抑制偶发跳变 */
#define TEMP_SETTLE_MS      100     /* PT12 打开后等待模拟前端稳定（原来 20ms 太短） */
#define TEMP_SAMPLE_GAP_MS  5       /* 相邻两次采样的间隔，让充放电回路恢复 */

/* ---------------- 两点标定（raw 正比于阻值，实测后填） ----------------
 * 当前值按「raw=71434261 对应 25℃」用 PT1000 分度表反推，需冰水重新标定
 */
#define TEMP_RAW0           65097260    /* 0℃ 时的计数 */
#define TEMP_T0_X10         0           /* 0.0℃ */
#define TEMP_RAW1           90163285    /* 100℃ 时的计数 */
#define TEMP_T1_X10         1000        /* 100.0℃ */

/* ---------------- 测量参数 ---------------- */
#define TEMP_PORT_NUM       0       /* 端口：0 = PT1/PT2，1 = PT3/PT4 */
#define TEMP_PORT_DIR       0       /* 方向：0/1，一处读不到数据就换另一个 */
#define TEMP_TIME_LOAD      0       /* 充电档：0=90us 1=120us 2=150us 3=500us */
#define TEMP_TIME_UNLOAD    0       /* 放电档：分档同充电档 */

/* readw() 未在头文件中声明，这里直接按物理地址读结果寄存器 */
#define TEMP_RD(addr)       (*(volatile uint32_t *)(addr))

/*
 * 组装 THSENS_CTRL 控制字
 *   bit1   ：端口选择
 *   bit2   ：方向选择
 *   bit4~5 ：充电时间档
 *   bit6~7 ：放电时间档
 */
static uint32_t temp_build_ctrl(uint32_t port, uint32_t dir, uint32_t load, uint32_t unload)
{
	uint32_t ctrl;
	ctrl  = ((port   & 1) << 1);
	ctrl |= ((dir    & 1) << 2);
	ctrl |= ((load   & 3) << 4);
	ctrl |= ((unload & 3) << 6);
	return ctrl;
}
/*
 * PT1/PT2 接入模拟前端后，超声波接收通路就收不到回波（实测 TO=1、hitF/B=0），
 * 因此 PT12 平时保持普通 IO，只在测温任务前后临时切换。
 */
static void temp_pt_enable(void)
{
	GPIO_PinRemapConfig(GPIO_Remap_PT12, ENABLE);
}

static void temp_pt_disable(void)
{
	GPIO_PinRemapConfig(GPIO_Remap_PT12, DISABLE);
}

/*
 * 启动一次温度任务并等待结果
 * 参数：port 端口，dir 方向，load/unload 充放电档
 * 输出：sts_f/sts_b 起始时刻，res_f/res_b 结束时刻
 * 返回：1 = 收到结果，0 = 等待超时
 */
static uint32_t temp_run(uint32_t port, uint32_t dir, uint32_t load, uint32_t unload,
                         uint32_t *sts_f, uint32_t *sts_b, uint32_t *res_f, uint32_t *res_b)
{
	uint32_t i;

	temp_pt_enable();                   /* 只在测温期间把 PT1/PT2 接到模拟前端 */
	THSENS->THSENS_CTRL = temp_build_ctrl(port, dir, load, unload);
	MEASURE_SetTdcAutoCalPst(ENABLE);   /* 与厂商温度任务例程保持一致 */
	MEASURE_ClearTdcResReg();
	MEASURE_ClearTdcCalReg();
	MEASURE_ClearResPtrReg();
	MEASURE_ITConfig(MEASURE_INT_RES_READ, ENABLE);
	MEASURE_ClearResITAndReadFlag();
	resvalid_flag = 0;
	MEASURE_SetTaskToStart(MEASURE_MOP_ONCE_TEMP);
	for(i = 0; i < 500; i++)            /* 最长等 500ms，避免偶发慢一拍就误判超时 */
	{
		if(resvalid_flag) break;
		if((i % 50) == 0) WDG_DogFeed(); /* 等结果时也要喂狗，否则 8 次超时会复位 */
		CLOCK_DelayMs(1);
	}
	if(!resvalid_flag) return 0;
	resvalid_flag = 0;

	*sts_f = TEMP_RD(StsFwd);
	*sts_b = TEMP_RD(StsBwd);
	*res_f = TEMP_RD(ResFwd);
	*res_b = TEMP_RD(ResBwd);
	return 1;
}
/*
 * 用指定参数测一次
 * 返回：温度原始计数（结束时刻 - 起始时刻），*ok 置 1 表示数据有效
 */
static uint32_t temp_measure_raw(uint32_t port, uint32_t dir,
                                 uint32_t load, uint32_t unload, uint32_t *ok)
{
	uint32_t sts_f, sts_b, res_f, res_b;

	*ok = 0;
	if(!temp_run(port, dir, load, unload, &sts_f, &sts_b, &res_f, &res_b))
	{
		return 0;
	}
#if TEMP_DEBUG_RAW
	printf("TEMP,port=%u,dir=%u,ld=%u,ul=%u,stsF=%u,stsB=%u,resF=%u,resB=%u\r\n",
			(unsigned)port, (unsigned)dir, (unsigned)load, (unsigned)unload,
			(unsigned)sts_f, (unsigned)sts_b, (unsigned)res_f, (unsigned)res_b);
#endif
	*ok = 1;
	/* 方向 0 的数据落在 Bwd，方向 1 落在 Fwd（与驱动 GetStartTime 的分支一致） */
	return dir ? (res_f - sts_f) : (res_b - sts_b);
}
/* 测温结束：摘 PT12 → 关温度通道 → MOP 改回 TOF → 重启巡航 */
static void temp_restore_sonar(void)
{
	temp_pt_disable();                  /* 先把 PT1/PT2 从模拟前端摘下来 */
	THSENS->THSENS_CTRL = 0;
	MEASURE_SetTask(MEASURE_MOP_ONCE_TOF);
	StandardMeasureInit();
	SonarFilterReset();                 /* 测温后声波重新起步，清空旧滤波窗口 */
}
/* 本次测温的采样极差（最大值-最小值），用于判断重复性 */
static uint32_t g_temp_spread = 0;

/*
 * 按 TEMP_xxx 默认参数测一次温度
 * 连续采样 TEMP_SAMPLE_NUM 次后取中值，抑制偶发的跳变点
 * 返回：1 = 成功，raw_out 带出原始计数（中值）；0 = 超时
 * 采样极差写入 g_temp_spread，用来判断重复性
 */
uint32_t MeasureTempOnce(uint32_t *raw_out)
{
	uint32_t buf[TEMP_SAMPLE_NUM];
	uint32_t i, j, tmp, ok, raw;

	MEASURE_SetCruiseMode(MEASURE_CRUISE_MODE_CLOSE);
	CLOCK_DelayMs(100);

	temp_pt_enable();                       /* 接入 PT1/PT2 */
	CLOCK_DelayMs(TEMP_SETTLE_MS);          /* 等模拟前端稳定 */

	for(i = 0; i < TEMP_SAMPLE_NUM; i++)
	{
		raw = temp_measure_raw(TEMP_PORT_NUM, TEMP_PORT_DIR,
		                       TEMP_TIME_LOAD, TEMP_TIME_UNLOAD, &ok);
		if(!ok)
		{
			printf("TEMP,timeout\r\n");
			return 0;
		}
		buf[i] = raw;
		WDG_DogFeed();
		CLOCK_DelayMs(TEMP_SAMPLE_GAP_MS);
	}

	/* 插入排序后取中值 */
	for(i = 1; i < TEMP_SAMPLE_NUM; i++)
	{
		tmp = buf[i];
		for(j = i; (j > 0) && (buf[j - 1] > tmp); j--)
		{
			buf[j] = buf[j - 1];
		}
		buf[j] = tmp;
	}

	g_temp_spread = buf[TEMP_SAMPLE_NUM - 1] - buf[0];
	if(raw_out) *raw_out = buf[TEMP_SAMPLE_NUM / 2];
	return 1;
}
/*
 * 参数扫描：2 个端口 × 2 个方向 × 16 种充放电组合各测一次
 * 用途：确认 PT1000 接在哪个端口、哪个方向有响应，开机跑一次即可
 */
void MeasureTempScan(void)
{
	uint32_t port, dir, ld, ul, ok;

	MEASURE_SetCruiseMode(MEASURE_CRUISE_MODE_CLOSE);
	CLOCK_DelayMs(100);

	temp_pt_enable();
	CLOCK_DelayMs(TEMP_SETTLE_MS);

	for(port = 0; port < 2; port++)
	{
		for(dir = 0; dir < 2; dir++)
		{
			for(ld = 0; ld < 4; ld++)
			{
				for(ul = 0; ul < 4; ul++)
				{
					temp_measure_raw(port, dir, ld, ul, &ok);
					WDG_DogFeed();      /* 整轮扫描耗时较长，边扫边喂狗 */
					CLOCK_DelayMs(50);
				}
			}
		}
	}
	temp_restore_sonar();
	/* 回读关键寄存器：确认 MOP 已改回 TOF、PT 通道已关闭 */
	printf("TEMP,cmd=%08x,thsens=%08x\r\n",
			(unsigned)PMU->COMMAND, (unsigned)THSENS->THSENS_CTRL);
	printf("SCAN,done\r\n");
}
/*
 * 原始计数 → 温度（两点线性标定，单位 0.1℃，纯 32 位整数运算）
 * 直接用 (raw-RAW0)*T/dRaw 会溢出到 1e10，这里先算出“每 0.1℃ 对应多少计数”
 * 再相除，既避免溢出，也不需要 libgcc 的 64 位除法函数。
 */
int MeasureTempToC_x10(uint32_t raw)
{
	int32_t span_raw, span_t, per_unit, a;

	if(TEMP_RAW1 == TEMP_RAW0)
	{
		return 0;               /* 还没标定 */
	}

	span_raw = (int32_t)TEMP_RAW1 - (int32_t)TEMP_RAW0;
	span_t   = (int32_t)TEMP_T1_X10 - (int32_t)TEMP_T0_X10;
	if((span_raw == 0) || (span_t == 0))
	{
		return 0;
	}

	per_unit = span_raw / span_t;       /* 每 0.1℃ 对应的计数 */
	if(per_unit == 0)
	{
		return 0;                       /* 标定跨度太小，无法换算 */
	}

	a = (int32_t)raw - (int32_t)TEMP_RAW0;
	if(a >= 0) a += per_unit / 2;       /* 四舍五入 */
	else       a -= per_unit / 2;

	return (int)(TEMP_T0_X10 + a / per_unit);
}
/*
 * 读一次温度：串口输出 TEMPC 行，按需刷新 LCD，最后恢复超声波测量
 * 未标定（TEMP_RAW1 == TEMP_RAW0）时只输出原始计数
 */
void MeasureTempShow(void)
{
	uint32_t raw;
	int t;

	if(!MeasureTempOnce(&raw))
	{
		g_temp_x10 = TEMP_X10_NONE;     /* 测温失败：屏上显示 "----"，不要留旧值 */
		temp_restore_sonar();
		return;
	}

	if(TEMP_RAW1 == TEMP_RAW0)
	{
		printf("TEMPC,raw=%u,spread=%u,uncalibrated\r\n",
				(unsigned)raw, (unsigned)g_temp_spread);
		temp_restore_sonar();
		return;
	}

	t = MeasureTempToC_x10(raw);
	g_temp_x10 = (int32_t)t;            /* 交给 LCD 多页面显示使用 */
	printf("TEMPC,raw=%u,spread=%u,t=%d.%d\r\n",
			(unsigned)raw, (unsigned)g_temp_spread,
			t / 10, (t < 0 ? -t : t) % 10);
#if TEMP_SHOW_ON_LCD
	Screen_Temperature((uint32_t)t);    /* 立即切到温度页；正常轮显时设 0 */
#endif

	temp_restore_sonar();       /* 关 PT 通道 + MOP 改回 TOF + 重启超声波巡航 */
}

/*
 * UserMeasure.h
 *
 *  Created on: 2021年10月27日
 *      Author: lixin
 */

#ifndef USERINC_USERMEASURE_H_
#define USERINC_USERMEASURE_H_

#include "ls1d_std_measure.h"
#include "ls1d_std_printf.h"
#include "ls1d_std_power.h"
#include "Filter.h"
//#include "tasks.h"
#define MeasureResultCaculateNummber	((uint32_t)32)//80
#define MaxPulseHitInterval			((int)0x112A88) 	//1.125us
#define MinPulseHitInterval			((int)0xd59F8)		//0.875us
#define HitIntervalDownRange		((int)(0 - 0xd59F8))		//0.875us
#define HitIntervalUpRange			((int)0xd59F8)		//0.875us
#define MaxDtofRange			  	((int)0x1C9C38)	    //1.875us
#define MinDtofRange			  	((int)(0 - 0x1C9C38))	    //1.875us
#define AMC_Threshold	   			12
int temper_shift;
typedef struct
{
	uint32_t money_current;			//本期购买的金额
	uint32_t money_total;			//累计金额
	uint32_t money_residual;		//本期购买的金额
}WaterMeterMoneyData;


typedef struct{
	uint32 nb_trigger;			/*NB发送数据触发时间间隔*/
	uint32 forward_cubage_interger;//水总累计容积，单位为毫升（1mL）
	uint32 backward_cubage_interger;//水总累计容积，单位为毫升（1mL）
	uint32_t battery_voltage;		/*电池电压*/
	uint32 capacity_purchased;		//购买的容积
	uint32 forward_total_cubage;	//水总累计容积，单位为升（1L）
	uint32 backward_total_cubage;	//水总累计容积，单位为升（1L）
	uint32 residual_cubage;		//剩余容积(单位L)
	uint32_t total_run_time; 	//系统运行累积时间(单位h)
	int32 water_speed;			//水流速(单位L)
	uint32_t upload_total;			//上传次数
	uint32_t upload_success_count;	//上传成功次数
	float forward_cubage_fraction;	//水总累计容积，单位为毫升（0.000001mL）
	float backward_cubage_fraction;//水总累计容积，单位为毫升（0.000001mL）
	uint16_t signal_gain;			/* 信号增益 */
	uint8_t balance_date;			/* 结算日 */
	uint8_t read_meter_date;		/* 抄表日 */
	int negative_cubage_count;
//	WaterMeterMoneyData money;
}pMeasureData;




typedef struct
{
	int fwd_pass_time;
	int bwd_pass_time;
	int dtof_time;
}SonarRecordData;

struct ZeroPositionMeasureStructure
{
	int  standard_value;//17.15数据格式时差值
	short shift_value;	//偏移量ps
	uint16_t step;			//零位测量的步骤
	int count;
}zero_position;


/**
 * @说明 测量初始化定义
 */
typedef struct
{
	int dtof_last;//dtof 上一次值
	int dtof_current;//dtof 当前值
	int delt;		//修正的脉冲的1个周期值
	uint16_t count;	//异常值累计
	uint16_t flag;
}DtofRepairStructure;


/**
 * @说明 测量初始化定义
 */
typedef struct
{
	uint8_t precal_enable;
	uint8_t pstcal_enable;
	uint8_t wave_wide_enable;
	uint8_t Cal_Neg_Edge;
}Sonar_Calibation_Ctrl;

typedef struct
{

	uint16_t count;
	uint16_t adc_value;
	uint16_t temprature;
	uint16_t temp_low_count;
	int temp_offset;
}Ntc_Measure_Ctrl;

extern pMeasureData MeasureRecord;
extern MedianFilterStructData dtof_filter;
extern MedianFilterStructData fwd_filter;
extern MedianFilterStructData bwd_filter;
extern SonarRecordData measure_rawdata[MeasureResultCaculateNummber + 1];
extern DtofRepairStructure Dtof_Repair_Data;
extern uint32_t caculate_index;
extern uint32_t transducer_state;
extern const uint32_t Threshold_default;
extern const uint8_t hit_num  ;
extern const uint8_t flter_num  ;
extern Sonar_Calibation_Ctrl calib_ctrl;
extern Ntc_Measure_Ctrl NTC_measure;
extern uint16_t MeasureDataIRQProcess(SonarRecordData *record);
extern float WaterVolumeMoneyCalculate(uint32_t fwd_fly_time,uint32_t bwd_fly_time,int dtof,uint8 work_mode);
extern ErrorStatus SonarExpectResPtr(void);
extern ErrorStatus SonarGetTdcCal(MeasureTdcCal_StateTypeDef *cal);
ErrorStatus Dtof_Repair(DtofRepairStructure *repair_structure);
//extern void PrintfSonarRegister(void);
extern uint16_t MEASURE_GetManualTempValue(uint8 work_mode);
extern int MEASURE_GetManualTempComValue(int dtof);

#endif






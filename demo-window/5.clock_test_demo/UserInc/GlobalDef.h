
/********************************************************************
* 文件名:	ls1d6_std_GlobalDef.h
* 版本：		V1.0
* 作者:		李鑫
* 日期:		2022.01.04
* 说明:		本文件包含GlobalDef用户自定义的底层驱动库函数声明及宏定义
* 版权:		龙芯中科（太原）技术有限公司
********************************************************************/

#ifndef INCLUDE_GLOBALDEF_H_
#define INCLUDE_GLOBALDEF_H_
#include "ls1d.h"

#define false      1
#define true       0

//typedef enum
//{
//	//InitMode = 0,     				/* 初始化模式  */
//	SleepMode = 1,  				/* 休眠省电模式  */
//	TurboMode = 2,  				/* cpu强力模式  */
//	FaultMode = 2,  				/* 故障报警模式  */
//	StandardMode = 3,  				/* 标定模式  */
//	CruiseMeasureMode = 4,  		/* 巡航测量模式  */
//	LogSaveMode  = 5,  				/* 巡航测量模式  */
//	CommunicationMode = 6,
//}TaskWorkMode;


#define MenuIDfreq 	    100
#define ExecuteRate(freq,tick) ((uint32_t)tick >= (uint32_t)freq)


extern void (*MainTask)(void);
extern void (*LastTask)(void);
/*================================函数返回状态========================================*/

#define RunOk   						  		  	(short)0
#define RunError 								  	(short)1
#define RunNull        						      	(short)2
#define SystemInPutParameterPointerIsNull           (short)3
#define SystemInPutParameterIsOutOfRange		 	(short)4
#define SystemFunctionInitFail		  			  	(short)5
#define SystemUseUninitParameter		  		  	(short)6
#define SystemUseSemaphoreParameterInitFail			(short)7
#define SystemUseSemaphoreNotCatch		  		  	(short)8
#define SystemUseTaskNotCreated		  		      	(short)9

/************************************************************************
* 函数名：  SetSystemErrorLogInfo
* 函数描述： set system error log information to a list ;
*
* 输入参数：
*     none
*
* 输出参数：
*     none；
*
* 返回：
*
* 备注：
**************************************************************************/
extern void SetSystemErrorLogInfo(short info);

/************************************************************************
* 函数名：  SetSystemErrorLogInfo
* 函数描述： get system error log information to from list ;
*
* 输入参数：
*     none
*
* 输出参数：
*     none；
*
* 返回：
*
* 备注：
**************************************************************************/
extern short GetSystemErrorLogInfo(void);

/************************************************************************
* 函数名：  ResetSystemErrorLogInfo
* 函数描述： reset system error log information to a list ;
*
* 输入参数：
*     none
*
* 输出参数：
*     none；
*
* 返回：
*
* 备注：
**************************************************************************/
extern void ResetSystemErrorLogInfo(void);



#endif /* INCLUDE_GLOBALDEF_H_ */












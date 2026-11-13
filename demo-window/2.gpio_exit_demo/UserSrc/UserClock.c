
/********************************************************************
* 文件名:  ls1d6_std_UserClock.c
* 版本：	  V1.0
* 作者:    李鑫
* 日期:    2021.09.26
* 说明:    本文件包含Clock用户自定义初始化的所有接口函数及宏定义
* 版权:    龙芯中科（太原）技术有限公司
********************************************************************/

#include "ls1d_std_clock.h"
extern uint32_t debugflag;




/**
  * @brief System Clock
  * @retval None
  */
void SystemClock(void)
{
	CLOCK_InitTypeDef  CLOCK_InitStruct = {0};

	CLOCK_StructInit(&CLOCK_InitStruct);

 	if(CLOCK_Init(&CLOCK_InitStruct) != SUCCESS)
 	{
 		 debugflag =10;
 	}

}



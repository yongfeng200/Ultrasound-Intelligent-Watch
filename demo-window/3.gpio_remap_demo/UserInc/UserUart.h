
/********************************************************************
* 文件名:	ls1d6_std_UserUart.h
* 版本：		V1.0
* 作者:		李鑫
* 日期:		2021.10.18
* 说明:		本文件包含uart用户自定义的底层驱动库函数声明及宏定义
* 版权:		龙芯中科（太原）技术有限公司
********************************************************************/

#ifndef USERINC_USERUART_H_
#define USERINC_USERUART_H_

#include "ls1d_std_uart.h"
#include "ls1d_std_clock.h"
#include "ls1d_std_printf.h"

void UartInit(void);
void UartTestTask(void);


#endif /* USERINC_USERUART_H_ */

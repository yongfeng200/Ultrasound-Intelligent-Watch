/********************************************************************
* 文件名:   ls1d6_std_printf.h
* 版本：	  V1.0
* 作者:    李鑫
* 日期:    2021.09.30
* 说明:    本文件包含printf打印的底层驱动库函数声明及宏定义
* 版权:    龙芯中科（太原）技术有限公司
********************************************************************/
#ifndef _PRINTF_H_
#define _PRINTF_H_

#include "ls1d.h"
#include "ls1d_std_uart.h"
#define max(a,b) (((a)>(b))?(a):(b))


//#define ReleaseMode

#ifndef  ReleaseMode
#define printf(fmt,args...)	        myprintf(fmt ,##args)
#else
#define printf(fmt,args...)
#endif

#define	BEBUG_IRQ()		printf("\r\n <  line锟斤拷%d func:%s   >\r\n",__LINE__,__FUNCTION__)

#define	Debug_printf(fmt,args...)			myprintf(fmt ,##args)

void 		myputchar(unsigned char chr);
int 		printbase(long v,int w,int base,int sign);
int 		puts(char *s);
int 		myprintf(char *fmt,...);
void 		printBuf(uint8 *buf,uint32 len);

#endif

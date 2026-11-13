
/********************************************************************
* 文件名:	ls1d6_std_interrupt.c
* 版本：		V1.0
* 作者:		李鑫
* 日期:		2021.11.31
* 说明:		本文件包含interrupt的所有接口函数及宏定义
* 版权:		龙芯中科（太原）技术有限公司
********************************************************************/

#include"Main.h"
#include"ls1d_std_cslreg.h"
#include"ls1d_std_timer.h"
#include"ls1d_std_rtc.h"
uint8 g_Uart1Cnt=0;
uint8 g_Rx1Buf[100]={0};
uint8 g_Uart0Cnt=0;
uint8 g_Rx0Buf[100]={0};
#define	BEBUG_IRQ()		printf("\r\n <  line: %d func:%s   >\r\n",__LINE__,__FUNCTION__)
void (* const Ext_IrqHandle[32])(void);

void Soft0_IRQHandler(void)
{
}

void Soft1_IRQHandler(void)
{
}

void Wake_IRQHandler(void)
{
	MEASURE_ClearIT(MEASURE_INT_WAKE);
	MEASURE_ITConfig(MEASURE_INT_WAKE,DISABLE);
}

void ResValid_IRQHandler(void)
{
	MEASURE_ClearResITAndReadFlag();
}

void BatFail_IRQHandler(void)
{

	MEASURE_ClearIT(MEASURE_INT_BAT_FAIL);
}

void C32kFail_IRQHandler(void)
{
	MEASURE_ClearIT(MEASURE_INT_C32K_FAIL);
}

void C8mFail_IRQHandler(void)
{
	MEASURE_ClearIT(MEASURE_INT_C8M_FAIL);
}

void Key_IRQHandler(void)
{
	MEASURE_ClearIT(MEASURE_INT_KEY);
}
void Ring_IRQHandler(void)
{
	BEBUG_IRQ();
	MEASURE_ClearIT(MEASURE_INT_RING);
}
void Rtc_IRQHandler(void)
{
	MEASURE_ClearIT(MEASURE_INT_RTC);
}

void TIMER_IRQHandler(void)
{
	BEBUG_IRQ();
	TIM_ClearIT(TIMER,TIM_FLAG_Trigger);
}

void CpuTimer_IRQHandler(void)
{
	CLOCK_ClearCpuTimerIT();
//    printf("Core Timer clear interrupt..\n");
}

void I2C_IRQHandler(void)
{
	BEBUG_IRQ();
	I2C_ClearIT(I2C);
}

void UART1_IRQHandler(void)
{

	BEBUG_IRQ();
	if(UART_GetITStatus(UART1,UART_IT_RXOT) || UART_GetITStatus(UART1,UART_IT_RXNE))
	{
		while(UART_GetFlagStatus(UART1,UART_LSR_FLAG_RXNE))
		{
			g_Rx0Buf[g_Uart1Cnt++] = UART1->DAT_DL_L;
		}
	}
	UART_ClearIT(UART1);


}

void UART0_IRQHandler(void)
{
	if(UART_GetITStatus(UART0,UART_IT_RXOT) || UART_GetITStatus(UART0,UART_IT_RXNE))
	{
		while(UART_GetFlagStatus(UART0,UART_LSR_FLAG_RXNE))
		{
			g_Rx0Buf[g_Uart0Cnt++] = UART0->DAT_DL_L;
		}
	}
	UART_ClearIT(UART0);


}

void FLASH_IRQHandler(void)
{
	//TODO
	BEBUG_IRQ();
	FLASH_ClearIT();
}

void SPI_IRQHandler(void)
{
	//TODO
	BEBUG_IRQ();	
#ifdef UserSPI
	SPI_ClearIT(SPI);
#else
	SPI->SPCR |=0x80;		//清除控制器中断申请
	SET_BIT(INT->INT_CLR,1<<5);	//清除中断标记位
#endif	
}

void ADC_IRQHandler(void)
{
	//TODO
	BEBUG_IRQ();
	SET_BIT(INT->INT_CLR, IRQ_ADC); //清除中断标记位
 
}

void Exint_IRQHandler(void)
{
	uint32 regsrc;// = EXTI->EXINT_SRC;
	uint32 regen;// = EXTI->EXINT_SRC;
	uint32 irq_no = 0;
	BEBUG_IRQ();
	regsrc = EXTI->EXINT_SRC;
	regen = EXTI->EXINT_EN;
	regsrc = (regsrc & regen);
	asm("nop");

	for(irq_no = 0; irq_no < 32; irq_no++)
    {
    	if((regsrc>>irq_no) & 0x1)
    	{
			Ext_IrqHandle[irq_no]();//进入中断处理程序
    	}
    }
	MEASURE_ClearIT(MEASURE_INT_EXINT);
}

void ExtGpioA0_IRQHandler(void)
{
	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A0);
}
void ExtGpioA1_IRQHandler(void)
{
	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A1);
}
void ExtGpioA2_IRQHandler(void)
{
	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A2);
}
void ExtGpioA3_IRQHandler(void)
{
	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A3);
}
void ExtGpioA4_IRQHandler(void)
{
	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A4);
}
void ExtGpioA5_IRQHandler(void)
{
	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A5);
}
void ExtGpioA6_IRQHandler(void)
{
	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A6);
}
void ExtGpioA7_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A7);
}
void ExtGpioA8_IRQHandler(void)
{
	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A8);
}
void ExtGpioA9_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A9);
}
void ExtGpioA10_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A10);
}
void ExtGpioA11_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A11);
}
void ExtGpioA12_IRQHandler(void)
{
	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A12);
}
void ExtGpioA13_IRQHandler(void)
{
	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A13 );
}
void ExtGpioA14_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A14);
}
void ExtGpioA15_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_A15 );
}

void ExtGpioB0_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B0);
}
void ExtGpioB1_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B1);
}
void ExtGpioB2_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B2);
}
void ExtGpioB3_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B3);
}
void ExtGpioB4_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B4);
}
void ExtGpioB5_IRQHandler(void)
{
	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B5);
}
void ExtGpioB6_IRQHandler(void)
{
	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B6);
}
void ExtGpioB7_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B7);
}
void ExtGpioB8_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B8);
}
void ExtGpioB9_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B9);
}
void ExtGpioB10_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B10);
}
void ExtGpioB11_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B11);
}
void ExtGpioB12_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B12);
}
void ExtGpioB13_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B13);
}
void ExtGpioB14_IRQHandler(void)
{

	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B14);
}
void ExtGpioB15_IRQHandler(void)
{
	BEBUG_IRQ();
	//清除中断标记位
	EXTI_ClearITPendingBit(IRQ_GPIO_B15);
}

void (* const Ext_IrqHandle[32])(void) = {
	ExtGpioA0_IRQHandler,
   ExtGpioA1_IRQHandler,
   ExtGpioA2_IRQHandler,
   ExtGpioA3_IRQHandler,
   ExtGpioA4_IRQHandler,
   ExtGpioA5_IRQHandler,
   ExtGpioA6_IRQHandler,
   ExtGpioA7_IRQHandler,
   ExtGpioA8_IRQHandler,
   ExtGpioA9_IRQHandler,
   ExtGpioA10_IRQHandler,
   ExtGpioA11_IRQHandler,
   ExtGpioA12_IRQHandler,
   ExtGpioA13_IRQHandler,
   ExtGpioA14_IRQHandler,
   ExtGpioA15_IRQHandler,
   ExtGpioB0_IRQHandler,
   ExtGpioB1_IRQHandler,
   ExtGpioB2_IRQHandler,
   ExtGpioB3_IRQHandler,
   ExtGpioB4_IRQHandler,
   ExtGpioB5_IRQHandler,
   ExtGpioB6_IRQHandler,
   ExtGpioB7_IRQHandler,
   ExtGpioB8_IRQHandler,
   ExtGpioB9_IRQHandler,
   ExtGpioB10_IRQHandler,
   ExtGpioB11_IRQHandler,
   ExtGpioB12_IRQHandler,
   ExtGpioB13_IRQHandler,
   ExtGpioB14_IRQHandler,
   ExtGpioB15_IRQHandler,
};


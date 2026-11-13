/*
 * UserLcd.c
 *
 *  Created on: 2021年11月29日
 *      Author: Administrator
 */

#include "UserLcd.h"

void Screen_SonarMeasure(uint32_t num)
{
    LCD_Dis_Cls();
    Num2LcdNum((int)num, NO_DOT_NUM);
    LCD_Disp();
}


//#define LCD_ON()		   {GPIO_WriteBit(GPIOA_BIT,GPIO_Pin_0,Bit_SET);\
//							GPIO_WriteBit(GPIOA_BIT,GPIO_Pin_1,Bit_SET);\
//							GPIO_WriteBit(GPIOA_BIT,GPIO_Pin_2,Bit_SET);\
//							CLOCK_DelayMs(1);\
//							CLEAR_BIT(CHIPCTRL, PMU_CHIPCTRL_LCDGPIO);\
//							WRITE_REG(LCD->RFR, 5);}
//
//#define LCD_OFF()	       {LCD_Dis_Cls();\
//							CLOCK_DelayMs(1);\
//							GPIO_WriteBit(GPIOA_BIT,GPIO_Pin_0,Bit_RESET);\
//							GPIO_WriteBit(GPIOA_BIT,GPIO_Pin_1,Bit_RESET);\
//							GPIO_WriteBit(GPIOA_BIT,GPIO_Pin_2,Bit_RESET);}

void LCD_Init(uint32_t refresh_rate)
{


    CLEAR_BIT(CHIPCTRL, PMU_CHIPCTRL_LCDGPIO);
    WRITE_REG(LCD->RFR, refresh_rate);

    LCD_Dis_Init();
    CLOCK_DelayMs(1000);

    LCD_Dis_Cls();
    CLOCK_DelayMs(100);

}




#ifdef OLD_LCD
/**
 * @Description: 流量界面
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param {uint32_t} num 流量值（实际值的1000倍，最后显示转化为带3位小数的数字）
 */
void Screen_LiuLiang(uint32_t num)
{
    LCD_Dis_Cls();

    Num2LcdNum(num,DOT_NUM_LIULIANG);

#ifdef OLD_LCD
    LCD_SET(LCD_LIFANGMI);
    LCD_SET(LCD_FENDUAN);
#else
    LCD_SET(LCD_Current);
    LCD_SET(LCD_CubicHour);
#endif
    LCD_Disp();
}
#endif


/**
 * @Description: 累积流量界面
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param {uint32_t} num 累积流量值（实际值的1000倍，最后显示转化为带3位小数的数字）
 */
void Screen_TotalCubic(uint32_t num)
{
    LCD_Dis_Cls();
    Num2LcdNum(num,DOT_NUM_LIULIANG);

#ifdef OLD_LCD
    LCD_SET(LCD_LIFANGMI);
    LCD_SET(LCD_LEIJI);
#else
    LCD_SET(LCD_Total);
    LCD_SET(LCD_Cubic);
#endif

    LCD_Disp();
}


#ifdef OLD_LCD
/**
 * @Description: 剩余流量界面
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param {uint32_t} num 剩余流量值（实际值的1000倍，最后显示转化为带3位小数的数字）
 */
void Screen_ShengYu(uint32_t num)
{


	LCD_Dis_Cls();

    Num2LcdNum(num,DOT_NUM_LIULIANG);

    LCD_SET(LCD_LIFANGMI);
    LCD_SET(LCD_SHENGYU);

    LCD_Disp();


}

#endif
/**
 * @Description: 表号界面
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param {uint32_t} num  显示表的号码
 */
void Screen_MeterNummber(uint8_t *buf)
{
//    LCD_Dis_Cls();
//    Num2LcdNum(num,NO_DOT_NUM);
//    LCD_SET(LCD_MeterNumber);
//    LCD_Disp();

    uint8_t tmp=0;
    int i  = 0;
    LCD_Dis_Cls();
    for(i=0;i < 8;i++)
    {
    	 tmp = buf[i]%10;
    	 LCD_Conver_Num(tmp,&g_seg_num[7 - i]);   //将表号显示出来
    }
//    LCD_SET(LCD_MeterNumber);

    LCD_Disp();
}

/**
 * @Description: 信号界面
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param {uint32_t} num  显示信号质量数
 */
void Screen_Signal(uint32_t num)
{
    LCD_Dis_Cls();

//    LCD_SET(LCD_Signal);
    if(1)
    {
    	LCD_Conver_Symbol('O',&g_seg_num[0]);
    	LCD_Conver_Symbol('F',&g_seg_num[1]);
    	LCD_Conver_Symbol('F',&g_seg_num[2]);
    	Num2LcdNum(99,NO_DOT_NUM);
    }else{
    	LCD_Conver_Symbol('O',&g_seg_num[0]);
    	LCD_Conver_Symbol('N',&g_seg_num[1]);
    	Num2LcdNum(num,NO_DOT_NUM);
    }
    LCD_Disp();
}


/**
 * @Description: 电池电量界面
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param {uint32_t} num  电池电量质量数
 */
void Screen_BatteryPower(uint32_t num)
{
    LCD_Dis_Cls();
    Num2LcdNum(num,NO_DOT_NUM);
    LCD_SET(LCD_LowPower);
    LCD_Disp();
}

/**
 * @Description: 电池电量界面
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param {uint32_t} num  电池电量质量数
 */
void Screen_Temperature(uint32_t num)
{
    LCD_Dis_Cls();
    Num2LcdNum(num,DOT_NUM_TEMP);
    LCD_SET(LCD_Degree);
    LCD_SET(LCD_Temperature);
    LCD_Disp();
}
/**
 * @Description: 剩余流速界面
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param {uint32_t} num 流速值（实际值的1000倍，最后显示转化为带3位小数的数字）
 */
void Screen_FlowSpeed(int num)
{
    LCD_Dis_Cls();
    Num2LcdNum(num,DOT_NUM_LIULIANG);
#ifdef OLD_LCD
    LCD_SET(LCD_LIFANGMI);
    LCD_SET(LCD_FENDUAN);
#else
    LCD_SET(LCD_Current);
//    LCD_SET(LCD_CubicHour);
    LCD_SET(LCD_Cubic);
    LCD_SET(LCD_QuaqueHour);
#endif

    LCD_Disp();
}

/**
 * @Description: 实际界面 显示格式 年-月-日
 * @Input: 日期数据缓冲 16进制数据格式
 * @Output: 无
 * @return {*}无
 * @param {uint8_t} *buf 日期数据缓冲 日：buf[0];月：buf[1];年：buf[2]；
 */
void Screen_Time(uint8_t *buf)
{
    uint8_t tmp=0;

    LCD_Dis_Cls();

    tmp = buf[0]%10;
    LCD_Conver_Num(tmp,&g_seg_num[7]);   //日期 日

    tmp = buf[0]/10;
    tmp = tmp>9?(tmp%10):tmp;
    LCD_Conver_Num(tmp,&g_seg_num[6]);
    LCD_Conver_Symbol('-',&g_seg_num[5]);  //"-"

    tmp = buf[1]%10;
    LCD_Conver_Num(tmp,&g_seg_num[4]);   //日期 月
    tmp = buf[1]/10;
    tmp = tmp>9?(tmp%10):tmp;
    LCD_Conver_Num(tmp,&g_seg_num[3]);
    LCD_Conver_Symbol('-',&g_seg_num[2]);  //"-"

    tmp = buf[2]%10;
    LCD_Conver_Num(tmp,&g_seg_num[1]);   //日期 年
    tmp = buf[2]/10;
    tmp = tmp>9?(tmp%10):tmp;
    LCD_Conver_Num(tmp,&g_seg_num[0]);

    LCD_SET(LCD_Time);

    LCD_Disp();
}

#ifdef OLD_LCD
/**
 * @Description: 剩余金额
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param {uint32_t} num 剩余流量值（实际值的1000倍，最后显示转化为带3位小数的数字）
 */
void Screen_Money(uint32_t num)
{
    LCD_Dis_Cls();

    Num2LcdNum(num,DOT_NUM_LIULIANG);

    LCD_SET(LCD_YUAN);
    LCD_SET(LCD_BENQI);
    LCD_SET(LCD_SHENGYU);

    LCD_Disp();
}
#endif
/**
 * @Description: 检定模式下显示累积体积界面
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param {uint32_t} num 累积流量值（实际值的100000倍，最后显示转化为带5位小数的数字）
 */
void Screen_VerificationTotalCubic(uint32_t num)
{
    LCD_Dis_Cls();
    Num2LcdNum(num,DOT_NUM_TURBOLIULIANG);

    LCD_SET(LCD_Verification);
    LCD_SET(LCD_Total);
    LCD_SET(LCD_Cubic);
    LCD_Disp();
}



/**
 * @Description:阀开
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param 无
 */
void Screen_ExValueOpen(void)
{
    LCD_SET(LCD_OpenValve);
    LCD_Disp();
}

/**
 * @Description: 阀关
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param 无
 */
void Screen_ExValueClose()
{
    LCD_SET(LCD_CloseValve);
    LCD_Disp();
}


///**
// * @Description: 非满管
// * @Input: 无
// * @Output: 无
// * @return {*}
// * @param 无
// */
//void Screen_ExNotFullPipe()
//{
//    LCD_SET(LCD_NotFullPipe);
//    LCD_Disp();
//}


/**
 * @Description: 显示无电标志符号
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param 无
 */
void Screen_ExLowPower(void)
{
    LCD_SET(LCD_LowPower);
    LCD_Disp();
}

void Screen_ForwardWater(void)
{
    LCD_SET(LCD_ForwardWater);
    LCD_Disp();
}
void Screen_BackwardWater(void)
{
    LCD_SET(LCD_BackwardWater);
    LCD_Disp();
}

void Screen_ExVerificationMode(void)
{
    LCD_SET(LCD_Verification);
    LCD_SET(LCD_Cubic);
    LCD_Disp();
}

#if OLD_LCD
/**
 * @Description: 请充值充值
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param 无
 */
void Screen_exNeedCharge(void)
{
    LCD_SET(LCD_QINGCHONGZHI);
    LCD_Disp();
}

/**
 * @Description: 请充值充值
 * @Input: 无
 * @Output: 无
 * @return {*}
 * @param 无
 */
void Screen_exFinishCharge(void)
{
    LCD_SET(LCD_YICHONGZHI);
    LCD_Disp();
}

#endif

void Screen_ParameterInfo(void)
{
    LCD_Dis_Cls();
	LCD_Conver_Symbol('[',&g_seg_num[2]);
	LCD_Conver_Symbol('P',&g_seg_num[4]);
	LCD_Conver_Symbol(']',&g_seg_num[6]);
	LCD_Disp();
}

void Screen_HistoryInfo(void)
{
    LCD_Dis_Cls();
	LCD_Conver_Symbol('[',&g_seg_num[2]);
	LCD_Conver_Symbol('H',&g_seg_num[4]);
	LCD_Conver_Symbol(']',&g_seg_num[6]);
	LCD_Disp();
}

void Screen_SystemStateInfo(void)
{
    LCD_Dis_Cls();
	LCD_Conver_Symbol('[',&g_seg_num[2]);
	LCD_Conver_Symbol('S',&g_seg_num[4]);
	LCD_Conver_Symbol(']',&g_seg_num[6]);
	LCD_Disp();
}
void Screen_ErrorInfo(void)
{
    LCD_Dis_Cls();
	LCD_Conver_Symbol('[',&g_seg_num[2]);
	LCD_Conver_Symbol('E',&g_seg_num[4]);
	LCD_Conver_Symbol(']',&g_seg_num[6]);
	LCD_Disp();
}

void Screen_GoBack(void)
{
    LCD_Dis_Cls();
	LCD_Conver_Symbol('[',&g_seg_num[2]);
	LCD_Conver_Symbol('B',&g_seg_num[4]);
	LCD_Conver_Symbol(']',&g_seg_num[6]);
	LCD_Disp();
}

void Screen_SoftReset(void)
{
    LCD_Dis_Cls();
	LCD_Conver_Symbol('R',&g_seg_num[1]);
	LCD_Conver_Symbol('E',&g_seg_num[2]);
	LCD_Conver_Symbol('B',&g_seg_num[3]);
	LCD_Conver_Symbol('O',&g_seg_num[4]);
	LCD_Conver_Symbol('O',&g_seg_num[5]);
	LCD_Conver_Symbol('T',&g_seg_num[6]);
	LCD_Disp();
}

void Screen_ClearCapacity(uint32_t flag)
{
    LCD_Dis_Cls();
	LCD_Conver_Symbol('[',&g_seg_num[0]);
	LCD_Conver_Symbol('L',&g_seg_num[1]);
	LCD_Conver_Symbol('R',&g_seg_num[2]);
	LCD_Conver_Symbol('-',&g_seg_num[3]);
    if(!flag)
    {
    	LCD_Conver_Symbol('O',&g_seg_num[5]);
    	LCD_Conver_Symbol('F',&g_seg_num[6]);
    	LCD_Conver_Symbol('F',&g_seg_num[7]);
    }else{
    	LCD_Conver_Symbol('O',&g_seg_num[6]);
    	LCD_Conver_Symbol('N',&g_seg_num[7]);
    }

	LCD_Disp();
}
void Screen_TotalTime(uint32_t num)
{
    LCD_Dis_Cls();
    Num2LcdNum(num,NO_DOT_NUM);
    LCD_Disp();
}

/*==============================================================================
 *  段码屏自检与多页面显示：0~63 为 8 个数码管，64~95 为图标段
 *============================================================================*/

/* 最近一次测到的温度（0.1℃），TEMP_X10_NONE 表示还没有有效温度 */
int32_t g_temp_x10 = TEMP_X10_NONE;

/**
 * @Description: 全屏点亮，确认段码屏和驱动链路是否正常
 */
void Screen_AllOn(void)
{
    uint8_t i;

    for(i = 0; i < 12; i++)
    {
        g_FBn_Buff[i] = 0xFF;
    }
    LCD_Disp();
}

/**
 * @Description: 只点亮指定段位，其余段位全部熄灭
 * @param {uint16_t} bit 段位编号 0~95
 */
void Screen_OneSeg(uint16_t bit)
{
    uint8_t i;

    for(i = 0; i < 12; i++)
    {
        g_FBn_Buff[i] = 0;
    }
    if(bit < 96)
    {
        LCD_SET(bit);
    }
    LCD_Disp();
}

/**
 * @Description: 图标总览页，把所有图标段一起点亮，用来确认屏上到底有哪些图标
 */
void Screen_AllIcon(void)
{
    LCD_Dis_Cls();

    LCD_SET(LCD_Alarm);         /* 报警 */
    LCD_SET(LCD_Verification);  /* 检定 */
    LCD_SET(LCD_Total);         /* 累积 */
    LCD_SET(LCD_Current);       /* 流量 */
    LCD_SET(LCD_Time);          /* 时间 */
    LCD_SET(LCD_LowPower);      /* 缺电 */
    LCD_SET(LCD_Degree);        /* ℃ */
    LCD_SET(LCD_Cubic);         /* m³ */
    LCD_SET(LCD_QuaqueHour);    /* /h */
    LCD_SET(LCD_Spot);          /* 小数点提示 */
    LCD_SET(LCD_Hour);          /* H */
    LCD_SET(LCD_KW);            /* KW */
    LCD_SET(LCD_GJ);            /* GJ */
    LCD_SET(LCD_K);             /* K */
    LCD_SET(LCD_Power);         /* 功率 */
    LCD_SET(LCD_Temperature);   /* 温度 */
    LCD_SET(LCD_BackwardWater); /* 回水 */
    LCD_SET(LCD_ForwardWater);  /* 进水 */
    LCD_SET(LCD_ColdTotal);     /* 冷水 */
    LCD_SET(LCD_HotTotal);      /* 热水 */
    LCD_SET(LCD_CloseValve);    /* 关阀 */
    LCD_SET(LCD_OpenValve);     /* 开阀 */
    LCD_SET(LCD_Charge);        /* 充值 */
    LCD_SET(LCD_Residual);      /* 剩余 */
    LCD_SET(LCD_Purchase);      /* 买入 */

    Num2LcdNum(88888888, NO_DOT_NUM);   /* 数码管同时全亮，便于和图标对照 */
    LCD_Disp();
}

/**
 * @Description: 在连续 3 个数码管上显示一个带 1 位小数的数
 * @param {uint32_t} v    数值（已放大 10 倍，例如 41.8 传 418）
 * @param {uint8_t} base  最左边一位的编号（0 或 4）
 */
static void lcd_put_num3(uint32_t v, uint8_t base)
{
    LCD_Conver_Num((uint8_t)(v % 10), &g_seg_num[base + 2]);
    LCD_Conver_Num((uint8_t)((v / 10) % 10), &g_seg_num[base + 1]);
    LCD_Conver_Num((uint8_t)((v / 100) % 10), &g_seg_num[base + 0]);
}

/**
 * @Description: 同时显示顺流/逆流飞行时间，单位 µs，各 1 位小数
 * @param {uint32_t} fwd_ps 顺流 Tf，显示在左 3 位（第 4 位留空作分隔）
 * @param {uint32_t} bwd_ps 逆流 Tr，显示在右 3 位（第 8 位留空）
 * @note  41830000 ps -> 左半 41.8；41900000 ps -> 右半 41.9
 *        屏上读作「41.8 _ 41.9 _」，0.1 µs 分辨率足以看出周跳
 */
void Screen_BothTofPage(uint32_t fwd_ps, uint32_t bwd_ps)
{
    LCD_Dis_Cls();

    if(fwd_ps)
    {
        lcd_put_num3((fwd_ps + 50000u) / 100000u, 0);   /* ps -> 0.1 µs */
        LCD_Dot(g_seg_dot[5]);                          /* 第 1、2 位之间 */
    }
    if(bwd_ps)
    {
        lcd_put_num3((bwd_ps + 50000u) / 100000u, 4);
        LCD_Dot(g_seg_dot[1]);                          /* 第 5、6 位之间 */
    }

    LCD_Disp();
}

/**
 * @Description: 主显示页面
 * @param {uint8_t} page        页面号（已不轮显，保留参数）
 * @param {uint32_t} fwd_ps     顺流飞行时间，单位 ps
 * @param {uint32_t} bwd_ps     逆流飞行时间，单位 ps
 * @note  其余参数为兼容保留，当前不使用
 */
void Screen_Page(uint8_t page, uint32_t fwd_ps, uint32_t bwd_ps,
                 int32_t dtof_ps, int32_t temp_x10,
                 int32_t flow_milli, uint8_t flow_valid)
{
    (void)page;
    (void)dtof_ps;
    (void)temp_x10;
    (void)flow_milli;
    (void)flow_valid;

    Screen_BothTofPage(fwd_ps, bwd_ps);
}

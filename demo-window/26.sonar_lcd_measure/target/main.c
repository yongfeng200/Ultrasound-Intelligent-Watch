#include"Main.h"
#include"UserLcd.h"      /* UserInc/Main.h 中没有包含它，这里显式包含 */
#include"UserMeasure.h"  /* 测温与流量换算接口 */

/* ---------------- PT1000 测温配置 ---------------- */
#define TEMP_MEASURE_ENABLE      1    /* 1：启用测温；0：关闭测温，单独验证超声波 */
#define TEMP_SCAN_ON_BOOT        0    /* 1：跑 64 组参数扫描（排查用，耗时较长） */
/* 主循环每 N 拍测一次温度（50 ms/拍）。600 拍 = 30 s。
 * 原来的 20 拍（约 1 s）会让声波测量每秒停一次：既丢 TOF 数据，
 * 又让 PT1000 的模拟前端来不及稳定，温度会在 29~37℃ 之间乱跳。 */
#define TEMP_TICKS_PER_MEASURE   600

/* 连续多少次没有有效 ΔT 就认为“信号丢失/空管”，LCD 显示 "----"。
 * 主循环约 50 ms 一拍，20 拍约 1 秒。 */
#define SONAR_LOST_COUNT         20
/* 流量累计用的标称采样间隔；实际循环还含 printf/LCD，会略大于 50 ms */
#define FLOW_SAMPLE_MS           50

/* ---------------- 段码屏配置 ---------------- */
/* 自检模式：0=正常分页显示  1=全屏点亮  2=逐段扫描  3=图标总览（后三种会停住不测量） */
#define LCD_DEMO_MODE            0
/* 主显示每 N 拍换一页：0=顺流Tf 1=逆流Tr 2=时差ΔT 3=温度 4=流量 */
#define LCD_PAGE_TICKS           30      /* 30 × 50 ms = 1.5 s */
#define LCD_PAGE_NUM             5
/* 1：开机会先“全亮 1.5s → 图标总览 1.5s”，用来确认屏上段位是否正常 */
#define LCD_BOOT_SELFTEST        1

/* ---------------- 超声波诊断 ---------------- */
/* 1：每次测量后打印 12 个 hit 的时差（HITD 行），用来查逆流周跳 */
#define SONAR_HIT_DUMP           0
/* 1：每测一次输出 SIG 行（信号质量/强度），用来判断读数可不可信 */
#define SONAR_SQ_ENABLE          1
uint32 inte8m;
uint32_t key_mode;
int data_test = 3;
extern uint8_t resvalid_flag;

int main(void)
{
	uint32_t reset_source = POWER_GetResetSource();
	uint32_t fwd_fly_time = 0;
	uint32_t bwd_fly_time = 0;
	int32_t  dtof = 0;                     /* 芯片/逐 hit 算出的原始时差（ps） */
	int32_t  dtof_fixed = 0;               /* 滤波后的时差（ps），无效时保持上一次有效值 */
	uint8_t  dtof_valid = 0;               /* 本次 ΔT 是否通过滤波检查 */
	uint32_t measure_seq = 0;
	/* 标定用统计：每 240 组（约 60 秒）输出一次平均值/极值 */
	int32_t  avg_sum = 0;
	int32_t  avg_min = 0;
	int32_t  avg_max = 0;
	uint32_t avg_cnt = 0;
	uint32_t temp_tick = 0;
	uint32_t lcd_tick = 0;
	uint8_t  lcd_page = 0;
	int32_t  flow_milli = 0;
	int32_t  flow_v_mms = 0;
	uint32_t flow_total_ml = 0;
	uint8_t  flow_valid = 0;
	uint16_t invalid_run = 0;              /* 连续无效样本计数 */
	uint8_t  signal_lost = 0;              /* 1=连续无效，认为没信号/空管 */
	FlagStatus lse_selected;
	FlagStatus lse_failed;
	WDG_SetWatchDog(0x80007fff);//设置看门狗为最长时间
	/*软件全局变量初始化*/
	SystemClock();	//时钟设置
	GPIOInit();		/* GPIO 初始化 */
	UartInit(115200,2400);
	GPIO_WriteBit(GPIOA_BIT, GPIO_Pin_2, Bit_SET);  // 控制 LCD 最后一位完整显示
	LCD_Init(5);

#if LCD_BOOT_SELFTEST
	/* 开机显示自检：先全亮、再点亮全部图标，各停 1.5 秒 */
	Screen_AllOn();
	CLOCK_DelayMs(1500);
	Screen_AllIcon();
	CLOCK_DelayMs(1500);
#endif

#if LCD_DEMO_MODE == 1
	/* 全屏点亮：所有段位一起亮，能亮说明屏和驱动都正常 */
	Screen_AllOn();
	printf("LCDDEMO,all_on\r\n");
	while(1)
	{
		WDG_DogFeed();
		CLOCK_DelayMs(200);
	}
#elif LCD_DEMO_MODE == 2
	/* 逐段扫描：先扫图标区 64~95（确认图标段是否存在），再扫数码管区 0~63 */
	{
		uint16_t seg;
		printf("LCDDEMO,seg_scan\r\n");
		for(seg = 64; seg < 160; seg++)     /* 64~95 之后接 0~63 */
		{
			uint16_t bit = (seg < 96) ? seg : (uint16_t)(seg - 96);
			Screen_OneSeg(bit);
			printf("LCDSEG,%u\r\n", (unsigned)bit);
			WDG_DogFeed();
			CLOCK_DelayMs(400);
		}
		Screen_AllOn();
		printf("LCDDEMO,seg_done\r\n");
	}
#elif LCD_DEMO_MODE == 3
	/* 图标总览：所有图标一起点亮 */
	Screen_AllIcon();
	printf("LCDDEMO,all_icon\r\n");
	while(1)
	{
		WDG_DogFeed();
		CLOCK_DelayMs(200);
	}
#endif

	printf("SONAR_READY,baud=115200,format=SONAR,v1\r\n");
	printf("SONAR_RESET,source=%x\r\n",reset_source);
	printf("SONAR_CLOCK,inte8m=%d,osc32k=%x,timing=%x\r\n",inte8m,inte32K,*(uint32_t *)0x1feb0000);
	printf("SONAR_HEADER,seq,dtof_ps,forward_ps,backward_ps\r\n");
	if(CLOCK_GetFlagStatus(CLOCK_HSE_FAIL) != RESET)
	{
		printf("SONAR_CLOCK_STATUS,HSE_FAIL\r\n");
	}
	if(CLOCK_GetFlagStatus(CLOCK_HSE_SELECT_STATUS) == RESET)
	{
		printf("SONAR_CLOCK_STATUS,HSE_NOT_SELECTED\r\n");
	}
	lse_selected = CLOCK_GetFlagStatus(CLOCK_LSE_SELECT_STATUS);
	lse_failed = CLOCK_GetFlagStatus(CLOCK_LSE_FAIL);
	printf("SONAR_32K,status=%s,fail=%u,timing=%x\r\n",
			lse_selected == SET ? "LSE_READY" : "LSE_NOT_SELECTED",
			(unsigned int)lse_failed,
			*(uint32_t *)0x1feb0000);
	if(lse_selected == RESET)
	{
		printf("SONAR_CLOCK_WARNING,32K source not selected; measurement may be unavailable\r\n");
	}
	if(lse_failed != RESET)
	{
		printf("SONAR_CLOCK_WARNING,32K oscillator failure flag is set\r\n");
	}
	resvalid_flag = 0;
	StandardMeasureInit();
	/*使能全局中断*/
	EXTI_EnableInt();

	printf("TEMPCODE,v1\r\n");
#if TEMP_MEASURE_ENABLE && TEMP_SCAN_ON_BOOT
	MeasureTempScan();
#endif

	while(1)
	{
		uint8_t data_updated = 0;
		uint32_t mr = MeasureDataProcess(&fwd_fly_time, &bwd_fly_time, &dtof);

		if(mr == 1)
		{
			measure_seq++;
			data_updated = 1;

			/* 先剔野值/修整周期，再做 5 点中值：
			 *   1) 绝对 Tf/Tr 在 MeasureDataProcess 里挡住 10~500 µs 之外
			 *   2) SonarFilterDtof 里做整周期修正、±600 ns 量程、野值、变化率检查
			 *   3) 只有 valid=1 才更新流量和标定统计；原始值仍打印在 SONAR_DATA */
			dtof_valid = SonarFilterDtof(dtof, &dtof_fixed);

			/* 原始数据：每组一行（保持原格式，方便上位机继续解析） */
			printf("SONAR_DATA,%u,%d,%u,%u\r\n",
					(unsigned)measure_seq,
					(int)dtof,
					(unsigned)fwd_fly_time,
					(unsigned)bwd_fly_time);
			printf("SONAR_FIX,%d,%d,%u\r\n", (int)dtof, (int)dtof_fixed, (unsigned)dtof_valid);
#if SONAR_SQ_ENABLE
			{
				SonarQuality_t q;
				if(MeasureQuality(&q))
				{
					/* 顺/逆流信号质量与强度：sq 0~100，hit 0~12，pp 单位 ps */
					printf("SIG,sqf=%u,ppf=%d,hitf=%u,wf=%u,sqb=%u,ppb=%d,hitb=%u,wb=%u\r\n",
							(unsigned)q.sq_f, (int)q.pp_f, (unsigned)q.hit_f, (unsigned)q.w_f,
							(unsigned)q.sq_b, (int)q.pp_b, (unsigned)q.hit_b, (unsigned)q.w_b);
				}
			}
#endif
#if SONAR_HIT_DUMP
			MeasureHitDump();       /* 诊断：12 个 hit 的时差明细 */
#endif
			if(dtof_valid)
			{
				invalid_run = 0;
				if(signal_lost)
				{
					signal_lost = 0;
					printf("SONAR_OK\r\n");
				}

				/* ---- 标定统计：每 240 组（约 60 秒）一行，只用滤波后的有效值 ---- */
				if(avg_cnt == 0)
				{
					avg_min = dtof_fixed;
					avg_max = dtof_fixed;
				}
				avg_sum += dtof_fixed;
				if(dtof_fixed < avg_min) avg_min = dtof_fixed;
				if(dtof_fixed > avg_max) avg_max = dtof_fixed;
				avg_cnt++;
				if(avg_cnt >= 240)
				{
					printf("AVG60S,n=%u,avg=%d,min=%d,max=%d,pp=%d\r\n",
							(unsigned)avg_cnt,
							(int)(avg_sum / (int32_t)avg_cnt),
							(int)avg_min,
							(int)avg_max,
							(int)(avg_max - avg_min));
					avg_sum = 0;
					avg_cnt = 0;
				}
				/* ---- 统计结束 ---- */

				/* 用滤波后的 ΔT 算流速/流量，并累计体积 */
				SonarFlowCalc(fwd_fly_time, bwd_fly_time, dtof_fixed,
				              &flow_v_mms, &flow_milli);
				SonarFlowAccumulate(flow_milli, FLOW_SAMPLE_MS);
				flow_total_ml = SonarFlowTotalMl();
				flow_valid = SonarFlowCalibrated();
				if((measure_seq % 10) == 0)
				{
					printf("FLOW_DATA,%d,%d,%u\r\n",
							(int)flow_v_mms, (int)flow_milli, (unsigned)flow_total_ml);
				}
			}
			else
			{
				if(invalid_run < 60000) invalid_run++;
				if((invalid_run >= SONAR_LOST_COUNT) && !signal_lost)
				{
					signal_lost = 1;
					flow_valid = 0;
					flow_milli = 0;
					dtof_fixed = 0;
					data_updated = 1;
					printf("SONAR_LOST\r\n");
				}
			}
		}
		else if(mr == 2)
		{
			/* 收到结果但被飞行时间范围/结果指针检查剔除 */
			if(invalid_run < 60000) invalid_run++;
			if((invalid_run >= SONAR_LOST_COUNT) && !signal_lost)
			{
				signal_lost = 1;
				flow_valid = 0;
				flow_milli = 0;
				dtof_fixed = 0;
				data_updated = 1;
				printf("SONAR_LOST\r\n");
			}
		}

		/* 页面轮显与“有没有新数据”解耦：即使暂时收不到回波，也能切页并显示 "----" */
		if(++lcd_tick >= LCD_PAGE_TICKS)
		{
			lcd_tick = 0;
			lcd_page = (uint8_t)((lcd_page + 1) % LCD_PAGE_NUM);
			data_updated = 1;
		}

		if(data_updated)
		{
			/* 0=顺流Tf 1=逆流Tr 2=ΔT 3=温度 4=流量
			 * Tf/Tr 显示原始值，这样逆流跳周期时能在屏上直接看到；
			 * ΔT 和流量使用修正后的值。 */
			Screen_Page(lcd_page, fwd_fly_time, bwd_fly_time,
			            dtof_fixed, g_temp_x10, flow_milli, flow_valid);
		}

#if TEMP_MEASURE_ENABLE
		/* ---- 周期测温，串口输出 TEMPC 行 ---- */
		if(++temp_tick >= TEMP_TICKS_PER_MEASURE)
		{
			temp_tick = 0;
			MeasureTempShow();
		}
#endif
		CLOCK_DelayMs(50);
		WDG_DogFeed();
	}
	return 0;
}
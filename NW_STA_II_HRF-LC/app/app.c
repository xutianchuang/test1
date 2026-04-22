/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                     ST Microelectronics STM32
*                                              on the
*
*                                           STM3240G-EVAL
*                                         Evaluation Board
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : DC
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <stdarg.h>
#include  <stdio.h>
#include  <math.h>
#include  <gsmcu_hal.h>

#include  <cpu.h>
#include  <lib_math.h>
#include  <lib_mem.h>
#include  <os.h>
#include  <os_app_hooks.h>

#include  <app_cfg.h>
#include  <bsp.h>
#include "gsmcu_trng.h"
#include <stdlib.h>
#include <gsmcuxx_hal_def.h>
#include "system_inc.h"
#include "phy_port.h"
#include "hrf_port.h"

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

/* --------------- APPLICATION GLOBALS ---------------- */
static  OS_TCB       AppTaskStartTCB;
static  CPU_STK      AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];


#if (OS_CFG_SEM_EN > 0u)
static  OS_SEM       AppTraceSem;
#endif

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/


void  App_CreateTask(void *p_arg);

ARM_MPU_Region_t MPU_table[] = {
	{ ARM_MPU_RBAR(0, 0x10000000),          ARM_MPU_RASR(1,ARM_MPU_AP_FULL, 0,1,1,0, 0,ARM_MPU_REGION_SIZE_1MB)},
	{ ARM_MPU_RBAR(1, 0x10000000),          ARM_MPU_RASR(0, ARM_MPU_AP_RO,   0, 1, 1, 0, 0, ARM_MPU_REGION_SIZE_256KB) },
	{ ARM_MPU_RBAR(2, 0x10000000+256*1024), ARM_MPU_RASR(0, ARM_MPU_AP_RO,   0, 1, 1, 0, 0, ARM_MPU_REGION_SIZE_64KB) },
	{ ARM_MPU_RBAR(3, FLASH_AHB_ADDR),      ARM_MPU_RASR(1, ARM_MPU_AP_RO, 0, 0, 1, 0, 0, ARM_MPU_REGION_SIZE_2MB) },
	{ ARM_MPU_RBAR(4, 0x40000000),          ARM_MPU_RASR(1, ARM_MPU_AP_FULL, 0, 1, 0, 1, 0, ARM_MPU_REGION_SIZE_16MB) },
	{ ARM_MPU_RBAR(5, 0x50000000),          ARM_MPU_RASR(1,ARM_MPU_AP_FULL, 0,1,0,1, 0,ARM_MPU_REGION_SIZE_8MB)},
};
//#pragma location = ".vectors"
#pragma segment=".vectors"
void mpu_init(void)
{
	//void * sram_start = __section_begin(".vectors");
	ARM_MPU_Load(MPU_table,sizeof(MPU_table)/sizeof(ARM_MPU_Region_t));
	ARM_MPU_Enable(MPU_CTRL_ENABLE_Msk);
}
/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*
* Notes       : 1) STM32F4xx HAL library initialization:
*                      a) Configures the Flash prefetch, intruction and data caches.
*                      b) Configures the Systick to generate an interrupt. However, the function ,
*                         HAL_InitTick(), that initializes the Systick has been overwritten since Micrium's
*                         RTOS has its own Systick initialization and it is recommended to initialize the
*                         Systick after multitasking has started.
*
*********************************************************************************************************
*/

const u32 CCO_Hard_Type=1;//1原来的硬件 0新的硬件

#define FLASH_STOR_DEVEICE_ADDR   (FLASH_AHB_ADDR|0x40)
#define P_DEVEICE                 (*(u32*)(FLASH_STOR_DEVEICE_ADDR))
#define CCO_NEW_HARD             0x12345678
#define CCO_OLD_HARD             0x76543210
u8 g_config_power=0;  //1：用户配置功率
u8 g_config_hrf_power=0;  //1：用户配置功率
//此函数必须在初始化mpu前调用
void GetDeveiceType(void)
{
	//引脚识别设备类型
#if DEV_CCO


#if 0
	if (P_DEVEICE==CCO_NEW_HARD)
	{
		*(u32*)&CCO_Hard_Type=0;//new hard pcb
		return;
	}
	else if (P_DEVEICE==CCO_OLD_HARD)
	{
		*(u32*)&CCO_Hard_Type=1;//old hard pcb
		return;
	}
	CcoModeOpen();
	switch (GetCcoModeValue())
	{
	case 0:
		*(u32*)&CCO_Hard_Type=0;//new hard pcb
		break;
	case 1:
		*(u32*)&CCO_Hard_Type=1;//old hard pcb
	}
#endif	
	*(u32*)&CCO_Hard_Type=1;//old hard pcb
	return;
#endif
}

int main(void)
{
	OS_ERR   err;
#if defined(ZB205_CHIP) || defined(ZB206_CHIP)
    get_efuse_info();
#endif
	QSPI_Enable();
	FLASH_CMD_CONFIG();
#if (CPU_CFG_NAME_EN == DEF_ENABLED)
	CPU_ERR  cpu_err;
#endif
	u8 power[4];
	u8 rfpower;
	if(data_flash_read_straight_with_crc_noadc(STA_POWER_MODE_ADDR,power,4,0xff))
	{
		int i=0;
		for (;i<4;i++)
		{
#if (HPLC_MIN_POWER_VAL == 0)
			if (power[i] > HPLC_MAX_POWER_VAL)
#else
			if ((power[i] < HPLC_MIN_POWER_VAL) || (power[i] > HPLC_MAX_POWER_VAL))
#endif
			{
				break;
			}
		}
		if (i>3)
		{
			memcpy((void *)HPLC_ChlPower, power, sizeof(HPLC_ChlPower));
            g_config_power=1;         
		}
	}
	
	if(data_flash_read_straight_with_crc_noadc(STA_RF_POWER_MODE_ADDR,&rfpower,1,0xff))
	{	
#if (HRF_MIN_POWER_VAL == 0)
		if (rfpower <= HRF_MAX_POWER_VAL)
#else
		if ((rfpower >= HRF_MIN_POWER_VAL) && (rfpower <= HRF_MAX_POWER_VAL))
#endif
		{
			*(u8*)(&HRF_LinePower) = rfpower;
            g_config_hrf_power=1;
		}
	}
	
    GetDeveiceType();
	FLASH_Init();
	FLASH_Reset();
	FLASH_Close();
	GetDevFlashID();
//	if (!BPLC_CHIP_CHECK())
//	{
//		__BKPT(1);
//	}
	Protection1MFlashNoVolatile();
	Protection1MFlash();
	DelayTimerInit();
	HAL_Init();                                                 /* See Note 1.                                          */
	mpu_init();
//	Mem_Init();                                                 /* Initialize Memory Managment Module                   */
	Math_Init();                                                /* Initialize Mathematical Module                       */
#ifdef FPGA_ENV
	srand(10);
#else
	srand(getTrngRand());
#endif
	CloseClockGate();
#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_NameSet((CPU_CHAR *)CPU_NAME,
				(CPU_ERR  *)&cpu_err);
#endif

	BSP_IntDisAll();                                            /* Disable all Interrupts.                              */

	OSInit(&err);                                               /* Init uC/OS-III.                                      */
	App_OS_SetAllHooks();

	OSTaskCreate(&AppTaskStartTCB,                              /* Create the start task                                */
				 "App Task Start",
				 App_CreateTask,
				 0u,
				 APP_CFG_TASK_START_PRIO,
				 &AppTaskStartStk[0u],
				 APP_CFG_TASK_START_STK_SIZE / 10u,
				 APP_CFG_TASK_START_STK_SIZE,
				 0u,
				 0u,
				 0u,
				 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 &err);

	OSStart(&err);                                              /* Start multitasking (i.e. give control to uC/OS-III). */

    while (DEF_ON) {                                            /* Should Never Get Here.                               */
		;
	}
}



/*
*********************************************************************************************************
*                                             AppTrace()
*
* Description : Thread-safe version of printf
*
* Argument(s) : Format string follwing the C format convention..
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  AppTrace(CPU_CHAR *format, ...)
{
	CPU_CHAR  buf_str[80 + 1];
	va_list   v_args;
	OS_ERR    os_err;


	va_start(v_args, format);
	(void)vsnprintf((char       *)&buf_str[0],
					(size_t)sizeof(buf_str),
					(char const *)format,
					v_args);
	va_end(v_args);

	OSSemPend((OS_SEM  *)&AppTraceSem,
			  (OS_TICK)0u,
			  (OS_OPT)OS_OPT_PEND_BLOCKING,
			  (CPU_TS  *)0,
			  (OS_ERR  *)&os_err);

//	printf("%s", buf_str);

	(void)OSSemPost((OS_SEM  *)&AppTraceSem,
					(OS_OPT)OS_OPT_POST_1,
					(OS_ERR  *)&os_err);

}

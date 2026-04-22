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
#include "phy_port.h"
#include "common_includes.h"
#include "bps_port.h"
#include "protocol_includes.h"

#include "crclib.h"

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

ARM_MPU_Region_t MPU_table[]={
  	{ ARM_MPU_RBAR(0,0x10000000),ARM_MPU_RASR(1,ARM_MPU_AP_FULL, 0,1,1,0, 0,ARM_MPU_REGION_SIZE_1MB)},
	{ ARM_MPU_RBAR(1,0x10000000),ARM_MPU_RASR(0,ARM_MPU_AP_RO,   0,1,1,0, 0,ARM_MPU_REGION_SIZE_256KB)},
	{ ARM_MPU_RBAR(2, 0x10000000+256*1024), ARM_MPU_RASR(0, ARM_MPU_AP_RO,   0, 1, 1, 0, 0, ARM_MPU_REGION_SIZE_32KB) },
	{ ARM_MPU_RBAR(3,FLASH_START_ADDR),ARM_MPU_RASR(1,ARM_MPU_AP_FULL, 0,0,1,0, 0,ARM_MPU_REGION_SIZE_2MB)},
	{ ARM_MPU_RBAR(4,0x40000000),ARM_MPU_RASR(1,ARM_MPU_AP_FULL, 0,1,0,1, 0,ARM_MPU_REGION_SIZE_16MB)},
	{ ARM_MPU_RBAR(5,0x50000000),ARM_MPU_RASR(1,ARM_MPU_AP_FULL, 0,1,0,1, 0,ARM_MPU_REGION_SIZE_8MB)},

};
//#pragma location = ".vectors"
#pragma segment=".vectors"
void mpu_init(void)
{
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
const u32 myDevicType=0xffff;
#if DEV_STA
#if defined(ZB205_CHIP)
#define FLASH_STOR_DEVEICE_ADDR   0x13000018
#elif defined(ZB204_CHIP)
#define FLASH_STOR_DEVEICE_ADDR   0x13000040
#endif
#define P_DEVEICE                 (*(u32*)(FLASH_STOR_DEVEICE_ADDR))
#define METER_DEVEICE             0x12345678
#define METER_3PHASE_DEVEICE      0x76543210
#endif
//此函数必须在初始化mpu前调用
void GetDeveiceType(void)
{
	//引脚识别设备类型
#ifdef II_STA
    *(u32*)&myDevicType=3;//meter
#else
	if (P_DEVEICE == METER_DEVEICE)
	{
		*(u32 *)&myDevicType = 3; //meter
		return;
	}
	else if (P_DEVEICE == METER_3PHASE_DEVEICE)
	{
		*(u32 *)&myDevicType = 7; //meter
		return;
	}
	StaModeOpen();
	switch (GetStaModeValue())
	{
	case 0:
		*(u32 *)&myDevicType = 3; //meter
		break;
	case 1:
		*(u32 *)&myDevicType = 7; //3 phase meter
		break;
	}
#endif	

}
#include "hplc_data.h"
int main(void)
{
	OS_ERR   err;
#if (CPU_CFG_NAME_EN == DEF_ENABLED)
	CPU_ERR  cpu_err;
#endif
	QSPI_Enable();
	FLASH_CMD_CONFIG();
    GetDeveiceType();
	FLASH_Init();
	FLASH_Reset();
	FLASH_Close();
	GetDevFlashID();
	DelayTimerInit();
	resetReasonInit();
	Protection1MFlashNoVolatile();
	
	DataFlashInit();
	Protection1MFlash();
//	FLASH_WriteQeFlag();
	DataFlashClose();
	HAL_Init();                                                 /* See Note 1.                                          */
	mpu_init();
//	Mem_Init();                                                 /* Initialize Memory Managment Module                   */
	Math_Init();                                                /* Initialize Mathematical Module                       */
	srand(getTrngRand());
	CloseClockGate();
#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_NameSet((CPU_CHAR *)CPU_NAME,
				(CPU_ERR  *)&cpu_err);
#endif

	BSP_IntDisAll();                                            /* Disable all Interrupts.                              */

	crc_table_init();

	NetStatusLedOpen();

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

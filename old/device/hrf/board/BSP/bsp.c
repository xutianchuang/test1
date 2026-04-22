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
*                                    MICRIUM BOARD SUPPORT PACKAGE
*
*                                     ST Microelectronics STM32
*                                              on the
*
*                                         STM3240G-EVAL
*                                        Evaluation Board
*
* Filename      : bsp.c
* Version       : V1.00
* Programmer(s) : FF
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#define   BSP_MODULE
#include  <bsp.h>


#include  <gsmcu_hal.h>

#include  "common_includes.h"
#include  "bps_zerocross.h"
#include  "bsp_wdg.h"
#include  "gsmcu_cmp_lvd.h"
#include  "hplc_mac_def.h"
#include  "protocol_includes.h"
/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  BSP_BIT_RCC_PLLCFGR_PLLM               25u
#define  BSP_BIT_RCC_PLLCFGR_PLLN              336u
#define  BSP_BIT_RCC_PLLCFGR_PLLP                2u
#define  BSP_BIT_RCC_PLLCFGR_PLLQ                7u


#define  BSP_GPIOG_LED1                        DEF_BIT_06
#define  BSP_GPIOG_LED2                        DEF_BIT_08
#define  BSP_GPIOI_LED3                        DEF_BIT_09
#define  BSP_GPIOC_LED4                        DEF_BIT_07

/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             REGISTERS
*********************************************************************************************************
*/

#define  BSP_REG_DEM_CR                       (*(CPU_REG32 *)0xE000EDFC)
#define  BSP_REG_DWT_CR                       (*(CPU_REG32 *)0xE0001000)
#define  BSP_REG_DWT_CYCCNT                   (*(CPU_REG32 *)0xE0001004)
#define  BSP_REG_DBGMCU_CR                    (*(CPU_REG32 *)0xE0042004)

/*
*********************************************************************************************************
*                                            REGISTER BITS
*********************************************************************************************************
*/

#define  BSP_DBGMCU_CR_TRACE_IOEN_MASK                   0x10
#define  BSP_DBGMCU_CR_TRACE_MODE_ASYNC                  0x00
#define  BSP_DBGMCU_CR_TRACE_MODE_SYNC_01                0x40
#define  BSP_DBGMCU_CR_TRACE_MODE_SYNC_02                0x80
#define  BSP_DBGMCU_CR_TRACE_MODE_SYNC_04                0xC0
#define  BSP_DBGMCU_CR_TRACE_MODE_MASK                   0xC0

#define  BSP_BIT_DEM_CR_TRCENA                    DEF_BIT_24

#define  BSP_BIT_DWT_CR_CYCCNTENA                 DEF_BIT_00

/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/


void GPIO_init()
{
	ZeroCrossInit();
#ifdef I_STA
	LedOpen();
#endif
    HplcLineDriverOpen();
    SetOpen();
    EventOpen();
    StaOpen();
	InsertTopoFuncOpen();
    InsertOpen();
	MultiOpen();
	CapEnCtrOpen();
	U485EnOpen();
}


/*
*********************************************************************************************************
*                                               BSP_Init()
*
* Description : Initialize the Board Support Package (BSP).
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) This function SHOULD be called before any other BSP function is called.
*
*               (2) CPU instruction / data tracing requires the use of the following pins :
*                   (a) (1) Aysynchronous     :  PB[3]
*                       (2) Synchronous 1-bit :  PE[3:2]
*                       (3) Synchronous 2-bit :  PE[4:2]
*                       (4) Synchronous 4-bit :  PE[6:2]
*
*                   (c) The application may wish to adjust the trace bus width depending on I/O
*                       requirements.
*               (3) The voltage scaling allows optimizing the power consumption when the device is
*                   clocked below the maximum system frequency, to update the voltage scaling value
*                   regarding system frequency refer to product datasheet.
*********************************************************************************************************
*/
u8 BootPowerState = 0;
void  BSP_Init (void)
{
#if !defined(II_STA)
	PwmInit(TOPO_PWM_TMR);
	PwmGpioFunc(TOPO_PWM_TMR, 0);

	InsertTopoFuncOpen();
	InsertTopoFuncWrite(0);
#endif

    GPIO_init();

    //UART1Init_Debug();
    
    //watch dog
    InitWdg();
    /* PLLCLK    = HSE * (PLLN / PLLM)      = 336MHz.       */
    /* SYSCLK    = PLLCLK / PLLP            = 168MHz.       */
    /* OTG_FSCLK = PLLCLK / PLLQ            =  48MHz.       */

#ifndef FPGA_ENV
#ifndef INTELLIGENCE_LOADING
#if defined(ZB204_CHIP)
	LVD_Init(ANA_VOLTAGE_2P55,ANA_VOLTAGE_2P7);
	Comparer_Init(ANA_VOLTAGE_1P95,ANA_VOLTAGE_2P4);
#elif defined(ZB205_CHIP)
	LVD_Init(VBATDET_2P5, VBATDET_2P7);
	Comparer_Init(VPINDET_1P9, VPINDET_2P4);
#endif
	//LVD必须是 单独高于高端电压
	while ((CPM->PINVDCFGR&(CPM_PINVDCFGR_PinVolDetHFlag_Msk|CPM_PINVDCFGR_PinVolDetLFlag_Msk))!=CPM_PINVDCFGR_PinVolDetHFlag_Msk)
	{
		realSoftReset();
	}
	resetResonFlag();
	NVIC_EnableIRQ(CMP_IRQn);
#endif
#endif
	apDMA_sCtrlrConfig      sCtrlrConfig;
	/* Initialise DMA ctrlr */

	sCtrlrConfig.AHB1BigEndian                = FALSE;
	sCtrlrConfig.AHB2BigEndian                = FALSE;
	sCtrlrConfig.sDefaultMemAccess.eAhbBus    = apDMA_AHB_BUS_1;
	sCtrlrConfig.sDefaultMemAccess.eWidth     = apDMA_WIDTH_8_BIT;
	sCtrlrConfig.sDefaultMemAccess.eBurstSize = apDMA_BURST_1;
	apDMA_Initialize((uint32_t)DMAC_BASE,
					 DMA_IRQn,
					 &sCtrlrConfig);
	BootPowerState = 1;
	
}

extern OS_Q PowerEventQ;
extern int power;

void DetectADC12VIrq(void)
{
	int current_power=0;
	OS_ERR err;
	//此处由于PINVDCFGR有保护直接写flag不会清除其他的bit位 但是保险起见清除状态位还是需要把其他的bit设好
	uint32_t reg_value=CPM->PINVDCFGR;

	if ((reg_value&(CPM_PINVDCFGR_PinVolDetHFlag_Msk|CPM_PINVDCFGR_PinVolDetLFlag_Msk))==(CPM_PINVDCFGR_PinVolDetHFlag_Msk|CPM_PINVDCFGR_PinVolDetLFlag_Msk))//两个表标志同时置位
	{
		if (power)
		{
			current_power=0;
			EventClose();
			ResetInterrupt(DISABLE);
		}
		else
		{
			current_power = 1;
			EventReopen();
			ResetInterrupt(ENABLE);
		}
	}
	else if (reg_value & CPM_PINVDCFGR_PinVolDetLFlag_Msk) //停电
	{
		current_power=0;
		EventClose();
		ResetInterrupt(DISABLE);

	}
	else if (reg_value & CPM_PINVDCFGR_PinVolDetHFlag_Msk)//复电
	{
		current_power=1;
		EventReopen();
		ResetInterrupt(ENABLE);
	}
	CPM->PINVDCFGR=reg_value;
	if (current_power!=power)
	{
		power=current_power;
		//一上电就复位
//		if (power)
//		{
//			RebootSystem();
//		}
		OSQPost(&PowerEventQ, NULL, power, OS_OPT_POST_1,&err);
	}
}
/*
*********************************************************************************************************
*                                            BSP_CPU_ClkFreq()
*
* Description : Read CPU registers to determine the CPU clock frequency of the chip.
*
* Argument(s) : none.
*
* Return(s)   : The CPU clock frequency, in Hz.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT32U  BSP_CPU_ClkFreq (void)
{
    CPU_INT32U  hclk_freq;


    hclk_freq = SystemCoreClock;
    return (hclk_freq);
}



/*
*********************************************************************************************************
*                                            BSP_Tick_Init()
*
* Description : Initialize all the peripherals that required OS Tick services (OS initialized)
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/
void BSP_Tick_Init (void)
{
    CPU_INT32U  cpu_clk_freq;
    CPU_INT32U  cnts;

    cpu_clk_freq = BSP_CPU_ClkFreq();                           /* Determine SysTick reference freq.                    */

#if (OS_VERSION >= 30000u)
    cnts  = cpu_clk_freq / (CPU_INT32U)OSCfg_TickRate_Hz;       /* Determine nbr SysTick increments.                    */
#else
    cnts  = cpu_clk_freq / (CPU_INT32U)OS_TICKS_PER_SEC;        /* Determine nbr SysTick increments.                    */
#endif

    OS_CPU_SysTickInit(cnts);                                   /* Init uC/OS periodic time src (SysTick).              */
}


void BSP_safe_reset(CPU_INT32U rs)
{
#ifdef INTER_WATCH_DOG
//    CPU_SR_ALLOC();
//    CPU_CRITICAL_ENTER();
	__disable_irq();
	while(1)
	{
#ifdef __DEBUG_MODE
	  FeedWdg();
#endif
	}
//    CPU_CRITICAL_EXIT();
#else
	__disable_irq();
	#ifndef __DEBUG_MODE
	WDG_SoftUnLock();
	#endif
	WDG_Enable();
	while(1)
	{
#ifdef __DEBUG_MODE
	  FeedWdg();
#endif
	}
#endif
}


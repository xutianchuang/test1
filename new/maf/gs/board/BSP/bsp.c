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
#include  <bsp_os.h>

#include  <gsmcu_hal.h>
#include  "system_inc.h"

#define ACD_LOW_POWER_VAILE            600
#ifdef HPLC_GDW
#define ACD_LOW_POWER_VAILE_NEW_DEVICE 600    //国网双模12V供电，分压电阻10:1
#else
#define ACD_LOW_POWER_VAILE_NEW_DEVICE 1799   //南网双模5V供电，分压电阻1:1
#endif
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
	SCU_DisableALLGPIOInput();
    GPIO_GlobalInit();
	ZeroCrossInit();
#if DEV_CCO	
	HplcLineDriverOpen();
	RxSwitchOpen();
	TxSwitchOpen();
	LedOpen();
	PhaseLedOpen();
	LedRunOpen();
    PA_OUT_OFF();
    PB_OUT_OFF();
    PC_OUT_OFF();

    PA_IN_ON();
    PB_IN_ON();
    PC_IN_ON();

	if (CCO_Hard_Type)
	{
		PA_LED_OFF();
		PB_LED_OFF();
		PC_LED_OFF();
	}
#elif DEV_STA
    LedOpen();
    HplcLineDriverOpen();
    SetOpen();
    EventOpen();
    StaOpen();
    InsertOpen();
#endif
}
#if DEV_STA
extern OS_Q PowerEventQ;
extern int power;

void DetectADC12VIrq(void)
{
	int current_power=0;
	OS_ERR err;
	if (ADC_GetIntStatus(ADC) & ADC_THRESHOLD(0)) //停电
	{
		ADC_ResetIntStatus(ADC,ADC_THRESHOLD(0));
		current_power=0;
		ResetInterrupt(DISABLE);
	}
	else if (ADC_GetIntStatus(ADC)&ADC_THRESHOLD(1))//复电
	{
		ADC_ResetIntStatus(ADC,ADC_THRESHOLD(1));
		current_power=1;
		ResetInterrupt(ENABLE);
	}
	ADC_ResetIntStatus(ADC,0xffffffff);
	if (current_power!=power)
	{
		power=current_power;
		OSQPost(&PowerEventQ, NULL, power, OS_OPT_POST_1,&err);
	}
}
#endif
bool ADC_GetCurrentPower(void)
{
	u32 adc_data;
	static u8 geteding=0;
	if (geteding)
	{
		return true;
	}
	geteding=1;
	for (int i=0;i<5;i++)//三次有一次超过阈值就认为有电
	{
		ADC_TriggerConverson(ADC);
		while (!(ADC_GetIntStatus(ADC) & (1 << (POWERDET_ADC_CHL + 8))))
		{
		}
		adc_data = ADC_GetChannelCovnData(ADC, POWERDET_ADC_CHL);
		ADC_ResetIntStatus(ADC, 1 << (POWERDET_ADC_CHL + 8));
		if (CCO_Hard_Type)
		{
			if (adc_data >= ACD_LOW_POWER_VAILE)
			{
				break;
			}
		}
		else
		{
			if (adc_data >= ACD_LOW_POWER_VAILE_NEW_DEVICE)
			{
				break;
			}
		}
	}
	geteding=0;
	if (CCO_Hard_Type)
	{
		if (adc_data < ACD_LOW_POWER_VAILE)
		{
			return false;
		}
	}
	else
	{
		if (adc_data < ACD_LOW_POWER_VAILE_NEW_DEVICE)
		{
			return false;
		}
	}
	return true;
}


u8 BootPowerState = 0;
void ADC_Initialize(void)
{
	OS_ERR err;
	ADC_InitTypeDef adcConf={
		.AUTOPWDN=AUTOPWDN_ENABLE,
		.COVN_MODE=SINGLE_STEP_COVN_MODE,
		.MCLK_DIV=20,
		.WKUPTime=5,
		.PWRENTime=5,
		.TSWKUPTime=1,
		.SETTLTime=2,
		.SAMPLTime=2,
		.HOLDTime=2,
		.CHSELTime=2,
		.CHDLYTime=255,
		.DISCHTime=2,
		.ADCFristChl=POWERDET_ADC_CHL//channel 2
	};
	ADC_Init(ADC,&adcConf);
#if DEV_STA	
	ADC_SetChannelThr(ADC, 0, POWERDET_ADC_CHL, 1000, 800);
	ADC_SetIntrEn(ADC,ADC_THRESHOLD(0)|ADC_THRESHOLD(1));
	BSP_IntVectSet(ADCC_IRQn,DetectADC12VIrq);
	//DMA interrupt
	NVIC_ClearPendingIRQ(ADCC_IRQn);
	NVIC_EnableIRQ(ADCC_IRQn);
#endif
	ADC_Enable(ADC);
	BootPowerState = 0;
	OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_DLY, &err);
	while(!ADC_GetCurrentPower())
	{
	}
	BootPowerState = 1;
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
void  BSP_Init (void)
{

    BSP_IntInit();
    GPIO_init();
	ADC_Initialize();
    //UART1Init_Debug();
    
    //watch dog
    InitWdg();
    /* PLLCLK    = HSE * (PLLN / PLLM)      = 336MHz.       */
    /* SYSCLK    = PLLCLK / PLLP            = 168MHz.       */
    /* OTG_FSCLK = PLLCLK / PLLQ            =  48MHz.       */



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


    hclk_freq = HAL_RCC_GetHCLKFreq();
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
	WDG_SoftUnLock();
	WDG_Enable();
	while(1)
	{
#ifdef __DEBUG_MODE
	  FeedWdg();
#endif
	}
#endif
}


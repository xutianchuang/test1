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
*                                         EXCEPTION VECTORS
*
*                                     ST Microelectronics STM32
*                                              on the
*
*                                           STM3240G-EVAL
*                                         Evaluation Board
*
* Filename      : cstartup.c
* Version       : V1.00
* Programmer(s) : FT
*                 FF
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/


#include <GSMCU_M3.h>
#include "os.h"
#include"gsmcu_flash.h"
#include  <bsp.h>
/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/

typedef  union {
    CPU_FNCT_VOID   Fnct;
    void           *Ptr;
} APP_INTVECT_ELEM;

//------------------------------------------------------------------------------
//         ProtoTypes
//------------------------------------------------------------------------------

extern void __iar_program_start( void );

int __low_level_init( void );

//------------------------------------------------------------------------------
//         Variables
//------------------------------------------------------------------------------
extern unsigned int __ICFEDIT_vector_start__;


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
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/
#define OS_TABLE_FUNC(func)  func##_Wrapper
#define OS_IRQ_FUN_GEN(func)\
extern void func(void);\
void func##_Wrapper(void)\
{\
    CPU_SR_ALLOC();\
	OS_CRITICAL_ENTER();\
	OSIntEnter();\
    func();\
    OSIntExit();\
    OS_CRITICAL_EXIT();\
}
#ifdef __DEBUG_MODE
#define RESET_FLASH() 
#else
#define RESET_FLASH() do{\
__set_SP(0x1007D000);\
FLASH_Init();\
FLASH_Reset();\
FLASH_Close();\
}while(0)
#endif
#pragma language=extended
#pragma segment="CSTACK"

//static  void  App_Reset_ISR       (void);

static  void  App_NMI_ISR         (void);

static  void  App_Fault_ISR       (void);

static  void  App_BusFault_ISR    (void);

static  void  App_UsageFault_ISR  (void);

static  void  App_MemFault_ISR    (void);

static  void  App_Spurious_ISR    (void);


extern  void  __iar_program_start (void);

extern  void  SystemInit          (void);

extern  void  SYSC_Handler        (void);
extern  void  WWDG_Handler        (void);

OS_IRQ_FUN_GEN(apDMA_RawISR);
OS_IRQ_FUN_GEN(SPI0_IRQHandler);

OS_IRQ_FUN_GEN(Timer0IsrFunction);
OS_IRQ_FUN_GEN(Timer1IsrFunction);
OS_IRQ_FUN_GEN(Timer2IsrFunction);
OS_IRQ_FUN_GEN(Timer3IsrFunction);
OS_IRQ_FUN_GEN(Timer4IsrFunction);
OS_IRQ_FUN_GEN(Timer5IsrFunction);
OS_IRQ_FUN_GEN(Timer6IsrFunction);
OS_IRQ_FUN_GEN(Timer7IsrFunction);
OS_IRQ_FUN_GEN(Timer8IsrFunction);
OS_IRQ_FUN_GEN(Timer9IsrFunction);
OS_IRQ_FUN_GEN(Timer10IsrFunction);
OS_IRQ_FUN_GEN(Timer11IsrFunction);
OS_IRQ_FUN_GEN(HRF_interrupt_handler);
OS_IRQ_FUN_GEN(HRF_dfe_interrupt_handler);
OS_IRQ_FUN_GEN(ntb_interrupt_handler);
OS_IRQ_FUN_GEN(bplc_interrupt_handler);
OS_IRQ_FUN_GEN(DetectADC12VIrq);
OS_IRQ_FUN_GEN(USART0_IRQHandler);
OS_IRQ_FUN_GEN(USART1_IRQHandler);
OS_IRQ_FUN_GEN(USART2_IRQHandler);
OS_IRQ_FUN_GEN(USART3_IRQHandler);
OS_IRQ_FUN_GEN(BPS_GPIO0_IRQ);
OS_IRQ_FUN_GEN(BPS_GPIO1_IRQ);
OS_IRQ_FUN_GEN(BPS_GPIO2_IRQ);
OS_IRQ_FUN_GEN(BPS_GPIO3_IRQ);
OS_IRQ_FUN_GEN(BPS_GPIO4_IRQ);
OS_IRQ_FUN_GEN(BPS_GPIO5_IRQ);
OS_IRQ_FUN_GEN(BPS_GPIO6_IRQ);
OS_IRQ_FUN_GEN(BPS_GPIO7_IRQ);
OS_IRQ_FUN_GEN(BPS_GPIO8_IRQ);
OS_IRQ_FUN_GEN(BPS_GPIO9_IRQ);

/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                  EXCEPTION / INTERRUPT VECTOR TABLE
*
* Note(s) : (1) The Cortex-M3 may have up to 256 external interrupts, which are the final entries in the
*               vector table.  The STM32F4xxx has 81 external interrupt vectors.
*
*           (2) Interrupts vector 2-13 are implemented in this file as infinite loop for debuging
*               purposes only. The application might implement a recover procedure if it is needed.
*
*           (3) OS_CPU_PendSVHandler() and OS_CPU_SysTickHandler() are implemented in the generic OS
*               port.
*********************************************************************************************************
*/
void DummyHandlerROM(void)
{
    while(1)
	{
		#ifdef __DEBUG_MODE
		FeedWdg();
		#endif
	}
}


__weak void OS_CPU_PendSVHandler(void)
{
    while(1);
}
__weak void OS_CPU_SysTickHandler(void)
{
    while(1);
}
 
__weak void USART0_IRQHandler(void)
{
	while(1);
}

__weak void USART1_IRQHandler(void)
{
	while(1);
}

__weak void USART2_IRQHandler(void)
{
	while(1);
}

__weak void USART3_IRQHandler(void)
{
	while(1);
}

__weak void HRF_dfe_interrupt_handler(void)
{
	while(1);
}

#pragma section = ".vectors"
#pragma location = ".vectors"
__root  const  APP_INTVECT_ELEM  __vector_table[] = {

    { .Ptr = __sfe("CSTACK") },                             /*  0, SP start value.                                    */
    __iar_program_start,                                              /*  1, PC start value.                                    */
    App_NMI_ISR,                                                /*  2, NMI.                                               */
    App_Fault_ISR,                                              /*  3, Hard Fault.                                        */
    App_MemFault_ISR,                                           /*  4, Memory Management.                                 */
    App_BusFault_ISR,                                           /*  5, Bus Fault.                                         */
    App_UsageFault_ISR,                                         /*  6, Usage Fault.                                       */
    App_Spurious_ISR,                                           /*  7, Reserved.                                          */
    App_Spurious_ISR,                                           /*  8, Reserved.                                          */
    App_Spurious_ISR,                                           /*  9, Reserved.                                          */
    App_Spurious_ISR,                                           /* 10, Reserved.                                          */
    App_Spurious_ISR,                                           /* 11, SVCall.                                            */
    App_Spurious_ISR,                                           /* 12, Debug Monitor.                                     */
    App_Spurious_ISR,                                           /* 13, Reserved.                                          */
    OS_CPU_PendSVHandler,                                       /* 14, PendSV Handler.                                    */
    OS_CPU_SysTickHandler,                                      /* 15, uC/OS-III Tick ISR Handler.                        */

    OS_TABLE_FUNC(apDMA_RawISR), //0 DMA
    OS_TABLE_FUNC(ntb_interrupt_handler), //1
	OS_TABLE_FUNC(bplc_interrupt_handler), //2
    DummyHandlerROM, //3
    DummyHandlerROM, //4
    DummyHandlerROM, //5
    OS_TABLE_FUNC(USART0_IRQHandler), //6
    OS_TABLE_FUNC(USART1_IRQHandler), //7
    OS_TABLE_FUNC(USART2_IRQHandler), //8
    OS_TABLE_FUNC(USART3_IRQHandler), //9
    DummyHandlerROM, //10
    DummyHandlerROM, //11
    DummyHandlerROM, //12
    DummyHandlerROM, //13
    DummyHandlerROM, //14
    OS_TABLE_FUNC(HRF_interrupt_handler), //15
    WWDG_Handler, //16
    DummyHandlerROM, //17
    DummyHandlerROM, //18
    DummyHandlerROM, //19
    OS_TABLE_FUNC(BPS_GPIO0_IRQ), //20
    OS_TABLE_FUNC(BPS_GPIO1_IRQ), //21
    OS_TABLE_FUNC(BPS_GPIO2_IRQ), //22
    OS_TABLE_FUNC(BPS_GPIO3_IRQ), //23
    OS_TABLE_FUNC(BPS_GPIO4_IRQ), //24
    OS_TABLE_FUNC(BPS_GPIO5_IRQ), //25
    OS_TABLE_FUNC(BPS_GPIO6_IRQ), //26
    OS_TABLE_FUNC(BPS_GPIO7_IRQ), //27
    OS_TABLE_FUNC(BPS_GPIO8_IRQ), //28
    OS_TABLE_FUNC(BPS_GPIO9_IRQ), //29
    OS_TABLE_FUNC(Timer0IsrFunction), //30
    OS_TABLE_FUNC(Timer1IsrFunction), //31
    OS_TABLE_FUNC(Timer2IsrFunction), //32
    OS_TABLE_FUNC(Timer3IsrFunction), //33
    OS_TABLE_FUNC(Timer4IsrFunction), //34
    OS_TABLE_FUNC(Timer5IsrFunction), //35
    OS_TABLE_FUNC(Timer6IsrFunction), //36
    OS_TABLE_FUNC(Timer7IsrFunction), //37
    OS_TABLE_FUNC(Timer8IsrFunction), //38
    OS_TABLE_FUNC(Timer9IsrFunction), //39
    OS_TABLE_FUNC(Timer10IsrFunction), //40
    OS_TABLE_FUNC(Timer11IsrFunction), //41
    OS_TABLE_FUNC(DetectADC12VIrq),//42
    DummyHandlerROM, //43
    DummyHandlerROM, //44
    OS_TABLE_FUNC(HRF_dfe_interrupt_handler), //45
};


/*
*********************************************************************************************************
*                                            App_Reset_ISR()
*
* Description : Handle Reset.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : This is an ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

int __low_level_init( void )
{
	//unsigned int * src = __section_begin(".vectors_RAM");
	unsigned int *src = __section_begin(".vectors");

	SCB->VTOR = ((unsigned int)(src)) | (0x0 << 7);



	SystemInit();



	return 1;
}


/*
*********************************************************************************************************
*                                            App_NMI_ISR()
*
* Description : Handle Non-Maskable Interrupt (NMI).
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : This is an ISR.
*
* Note(s)     : (1) Since the NMI is not being used, this serves merely as a catch for a spurious
*                   exception.
*********************************************************************************************************
*/

static  void  App_NMI_ISR (void)
{
    RESET_FLASH();
    while(1)
	{
		#ifdef __DEBUG_MODE
		FeedWdg();
		#endif
	}
}


/*
*********************************************************************************************************
*                                             App_Fault_ISR()
*
* Description : Handle hard fault.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : This is an ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_Fault_ISR (void)
{
    RESET_FLASH();
    while(1)
	{
		#ifdef __DEBUG_MODE
		FeedWdg();
		#endif
	}
}


/*
*********************************************************************************************************
*                                           App_BusFault_ISR()
*
* Description : Handle bus fault.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : This is an ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_BusFault_ISR (void)
{
    RESET_FLASH();
    while(1)
	{
		#ifdef __DEBUG_MODE
		FeedWdg();
		#endif
	}
}


/*
*********************************************************************************************************
*                                          App_UsageFault_ISR()
*
* Description : Handle usage fault.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : This is an ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_UsageFault_ISR (void)
{
    RESET_FLASH();
    while(1)
	{
		#ifdef __DEBUG_MODE
		FeedWdg();
		#endif
	}
}


/*
*********************************************************************************************************
*                                           App_MemFault_ISR()
*
* Description : Handle memory fault.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : This is an ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_MemFault_ISR (void)
{
    RESET_FLASH();
    while(1)
	{
		#ifdef __DEBUG_MODE
		FeedWdg();
		#endif
	}
}


/*
*********************************************************************************************************
*                                           App_Spurious_ISR()
*
* Description : Handle spurious interrupt.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : This is an ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_Spurious_ISR (void)
{
    RESET_FLASH();
    while(1)
	{
		#ifdef __DEBUG_MODE
		FeedWdg();
		#endif
	}
}


static void WWDG_Handler(void)
{
    RESET_FLASH();
	while(1)
	{
		#ifdef __DEBUG_MODE
		FeedWdg();
		#endif
	}
}



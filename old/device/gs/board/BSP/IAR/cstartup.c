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

#include  <cpu.h>
#include  <os.h>
#include <gsmcu_hal.h>
#include  <bsp.h>
#include <gsmcuxx_hal_def.h>

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

#pragma language=extended
#pragma segment="CSTACK"

//static  void  App_Reset_ISR       (void);

static  void  App_NMI_ISR         (void);

static  void  App_Fault_ISR       (void);

static  void  App_BusFault_ISR    (void);

static  void  App_UsageFault_ISR  (void);

static  void  App_MemFault_ISR    (void);

static  void  App_Spurious_ISR    (void);

static  void  IWDG_Handler        (void);
extern  void  __iar_program_start (void);

extern  void  SystemInit          (void);

extern  void  SYSC_Handler        (void);

extern  void  FLASH_Init();
extern  void  FLASH_Reset();
extern  void  FLASH_Close();
#define RESET_FLASH() do{\
__set_SP(0x1007D000);\
FLASH_Init();\
FLASH_Reset();\
FLASH_Close();\
}while(0)
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
	RESET_FLASH();
    while(1);
}

//#define IN_SPI_FLASH    1


#ifdef IN_SPI_FLASH


#pragma section = ".vectors"
#pragma location = ".vectors"
const APP_INTVECT_ELEM __vector_table[] =
{
    { .Ptr = __sfe( "CSTACK" ) },
    __iar_program_start,

    DummyHandlerROM,                                                /*  2, NMI.                                               */
    DummyHandlerROM,                                              /*  3, Hard Fault.                                        */
    DummyHandlerROM,                                           /*  4, Memory Management.                                 */
    DummyHandlerROM,                                           /*  5, Bus Fault.                                         */
    DummyHandlerROM,                                         /*  6, Usage Fault.                                       */
    DummyHandlerROM,                                           /*  7, Reserved.                                          */
    DummyHandlerROM,                                           /*  8, Reserved.                                          */
    DummyHandlerROM,                                           /*  9, Reserved.                                          */
    DummyHandlerROM,                                           /* 10, Reserved.                                          */
    DummyHandlerROM,                                           /* 11, SVCall.                                            */
    DummyHandlerROM,                                           /* 12, Debug Monitor.                                     */
    DummyHandlerROM,                                           /* 13, Reserved.                                          */
    DummyHandlerROM,                                       /* 14, PendSV Handler.                                    */
    DummyHandlerROM,                                      /* 15, uC/OS-III Tick ISR Handler.                        */

    SYSC_Handler,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    0,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    0,
    DummyHandlerROM,
    DummyHandlerROM,
    0,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,//38-70 33ge
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
    DummyHandlerROM,
};


#pragma section = ".vectors_RAM"
#pragma location = ".vectors_RAM"
__root  const  APP_INTVECT_ELEM  __vector_RAM_table[] = {
    
#else
    
#pragma section = ".vectors"
#pragma location = ".vectors"
__root  const  APP_INTVECT_ELEM  __vector_table[] = {
    
#endif

    { .Ptr = __sfe( "CSTACK" ) },                             /*  0, SP start value.                                    */
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

    SYSC_Handler,
    WWDG_Handler,
    IWDG_Handler,
#if ZB202
    TMR1_1_Handler,
    TMR1_2_Handler,
    TMR1_3_Handler,
    TMR1_4_Handler,
    TMR1_5_Handler,
    TMR1_6_Handler,	
    TMR0_1_Handler,
    TMR0_2_Handler,
    TMR0_3_Handler,
    TMR0_4_Handler,
    TMR0_5_Handler,
    TMR0_6_Handler,	
#elif DM750
    TMR0_1_Handler,
    TMR0_2_Handler,
    TMR0_3_Handler,
    TMR1_1_Handler,
    TMR1_2_Handler,
    TMR1_3_Handler,
    TMR2_1_Handler,
    TMR2_2_Handler,
    TMR2_3_Handler,
    TMR2_4_Handler,
    TMR2_5_Handler,
    TMR2_6_Handler,	
#endif
    DMAC_Handler,
    DMAC_TC_Handler,
    UART0_Handler,
    UART1_Handler,
    UART2_Handler,
    GPIO0_Handler,
    GPIO1_Handler,
    IIC0_Handler,
    UART3_Handler,
    IIC1_Handler,
    0,
    SSP0_Handler,
    SSP1_Handler,
    ADCC_Handler,
    0,
    SPI0_Handler,
    BPLC_IRQHandler,
    NTB_IRQHandler,
    CRC_Handler,
    MAC_Handler,
    AES_Handler,
    CRY_Handler,
    SHA_Handler,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,//38-70 33ge
    EXTI0_Handler,
    EXTI1_Handler,
    EXTI2_Handler,
    EXTI3_Handler,
    EXTI4_Handler,
    EXTI5_Handler,
    EXTI6_Handler,
    EXTI7_Handler,
    EXTI8_Handler,
    EXTI9_Handler,
    EXTI10_Handler,
    EXTI11_Handler,
    EXTI12_Handler,
    EXTI13_Handler,
    EXTI14_Handler,
    EXTI15_Handler,
    EXTI16_Handler,
    EXTI17_Handler,
    EXTI18_Handler,
    EXTI19_Handler,
    EXTI20_Handler,
    EXTI21_Handler,
    EXTI22_Handler,
    EXTI23_Handler,
    EXTI24_Handler,
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
    unsigned int * src = __section_begin(".vectors");

    SCB->VTOR = ((unsigned int)(src)) | (0x0 << 7);
#ifndef INTER_WATCH_DOG
        *(int*)0x40105000 = 0;
        *(int*)0x4000700C = 0;
#endif

    
    
    SystemInit();

#if __ARMVFP__                                                  /* Enable access to Floating-point coprocessor.         */
    CPU_REG_NVIC_CPACR = CPU_REG_NVIC_CPACR_CP10_FULL_ACCESS | CPU_REG_NVIC_CPACR_CP11_FULL_ACCESS;

    DEF_BIT_CLR(CPU_REG_SCB_FPCCR, DEF_BIT_31);                 /* Disable automatic FP register content                */
    DEF_BIT_CLR(CPU_REG_SCB_FPCCR, DEF_BIT_30);                 /* Disable Lazy context switch                          */
#endif
    
#ifdef IN_SPI_FLASH
   // __vector_table
        src = __section_begin(".vectors_RAM");
        SCB->VTOR = ((unsigned int)(src)) | (0x0 << 7);
#endif    
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
    while (DEF_TRUE) {
        ;
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
    while (DEF_TRUE) {
        ;
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
    while (DEF_TRUE) {
        ;
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
    while (DEF_TRUE) {
        ;
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
    while (DEF_TRUE) {
        ;
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
    while (DEF_TRUE) {
        ;
    }
}



static  void  IWDG_Handler (void)
{
	RESET_FLASH();
	__BKPT(1);
}

void sw(WORD addr, WORD data, BYTE type);

void SYSC_Handler(void)
{
	sw(SCU_BASE + 0x24,  0x00000048, HSIZE_WORD);  //clear sleep wakeup interrupt
}

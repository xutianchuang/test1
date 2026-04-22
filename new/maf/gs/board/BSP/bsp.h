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
* Filename      : bsp.h
* Version       : V1.00
* Programmer(s) : FF
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                                 MODULE
*
* Note(s) : (1) This header file is protected from multiple pre-processor inclusion through use of the
*               BSP present pre-processor macro definition.
*
*           (2) This file and its dependencies requires IAR v6.20 or later to be compiled.
*
*********************************************************************************************************
*/

#ifndef  BSP_PRESENT
#define  BSP_PRESENT


/*
*********************************************************************************************************
*                                                 EXTERNS
*********************************************************************************************************
*/

#ifdef   BSP_MODULE
#define  BSP_EXT
#else
#define  BSP_EXT  extern
#endif


/*
*********************************************************************************************************
*                                              INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <cpu_core.h>

#include  <lib_def.h>
#include <gsmcuxx_hal_def.h>
//=======================

#include "bps_dmac.h"
#include "bps_uart.h"
#include "bps_timer.h"
#include "bps_bootsrc.h"
#include "bps_zerocross.h"
#include "bps_debug.h"
#include "bps_led.h"
#include "bps_port.h"
#include "bps_flash.h"
#include "bsp_wdg.h"

/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             PERIPH DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                                 MACRO'S
*********************************************************************************************************
*/

#define     BSP_INT_SRC_NBR     96

/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void        BSP_Init       (void);

void        BSP_IntDisAll  (void);

CPU_INT32U  BSP_CPU_ClkFreq(void);

void        BSP_Tick_Init  (void);


/*
*********************************************************************************************************
*                                           INTERRUPT SERVICES
*********************************************************************************************************
*/

void        BSP_IntInit                     (void);

void        BSP_IntEn                       (CPU_DATA       int_id);

void        BSP_IntDis                      (CPU_DATA       int_id);

void        BSP_IntClr                      (CPU_DATA       int_id);

void        BSP_IntVectSet                  (CPU_DATA       int_id,
                                             CPU_FNCT_VOID  isr);

void        BSP_IntPrioSet                  (CPU_DATA       int_id,
                                             CPU_INT08U     prio);

void        SYSC_Handler              (void);
void        WWDG_Handler              (void);
//void        IWDG_Handler               (void);
#if ZB202
void  TMR1_1_Handler                (void);
void  TMR1_2_Handler              (void);
void  TMR1_3_Handler              (void);
void  TMR1_4_Handler           (void);
void  TMR1_5_Handler           (void);
void  TMR1_6_Handler           (void);
void  TMR0_1_Handler         (void);
void  TMR0_2_Handler           (void);
void  TMR0_3_Handler              (void);
void  TMR0_4_Handler           (void);
void  TMR0_5_Handler           (void);
void  TMR0_6_Handler           (void);
#elif DM750
void  TMR0_1_Handler         (void);
void  TMR0_2_Handler           (void);
void  TMR0_3_Handler              (void);
void  TMR1_1_Handler                (void);
void  TMR1_2_Handler              (void);
void  TMR1_3_Handler              (void);
void  TMR2_1_Handler              (void);
void  TMR2_2_Handler              (void);
void  TMR2_3_Handler              (void);
void  TMR2_4_Handler           (void);
void  TMR2_5_Handler           (void);
void  TMR2_6_Handler           (void);
#endif
void  DMAC_Handler           (void);
void  DMAC_TC_Handler           (void);
void  UART0_Handler           (void);
void  UART1_Handler           (void);
void  UART2_Handler                (void);
void  GPIO0_Handler            (void);
void  GPIO1_Handler           (void);
void  IIC0_Handler           (void);
void  UART3_Handler           (void);
void  IIC1_Handler            (void);
void  SSP0_Handler      (void);
void  SSP1_Handler      (void);
void  ADCC_Handler      (void);
void  SPI0_Handler            (void);
void  BPLC_IRQHandler               (void);
void  NTB_IRQHandler               (void);
void  CRC_Handler               (void);
void  MAC_Handler               (void);
void  AES_Handler            (void);
void  CRY_Handler            (void);
void  SHA_Handler            (void);
void  EXTI0_Handler            (void);
void  EXTI1_Handler               (void);
void  EXTI2_Handler               (void);
void  EXTI3_Handler             (void);
void  EXTI4_Handler             (void);
void  EXTI5_Handler             (void);
void  EXTI6_Handler          (void);
void  EXTI7_Handler           (void);
void  EXTI8_Handler        (void);
void  EXTI9_Handler         (void);
void  EXTI10_Handler      (void);
void  EXTI11_Handler        (void);
void  EXTI12_Handler            (void);
void  EXTI13_Handler       (void);
void  EXTI14_Handler               (void);
void  EXTI15_Handler               (void);
void  EXTI16_Handler               (void);
void  EXTI17_Handler               (void);
void  EXTI18_Handler             (void);
void  EXTI19_Handler             (void);
void  EXTI20_Handler           (void);
void  EXTI21_Handler               (void);
void  EXTI22_Handler           (void);
void  EXTI23_Handler           (void);
void  EXTI24_Handler           (void);



/*
*********************************************************************************************************
*                                     PERIPHERAL POWER/CLOCK SERVICES
*********************************************************************************************************
*/

CPU_INT32U   BSP_PeriphClkFreqGet(CPU_DATA  pwr_clk_id);

void         BSP_PeriphEn        (CPU_DATA  pwr_clk_id);

void         BSP_PeriphDis       (CPU_DATA  pwr_clk_id);


/*
*********************************************************************************************************
*                                              LED SERVICES
*********************************************************************************************************
*/

void        BSP_LED_On    (CPU_INT08U  led);

void        BSP_LED_Off   (CPU_INT08U  led);

void        BSP_LED_Toggle(CPU_INT08U  led);

void        BSP_safe_reset(CPU_INT32U rs);

#define BPS_PHASE_A    0
#define BPS_PHASE_B    1
#define BPS_PHASE_C    2

#define BPS_PHASE_A_NEG    3
#define BPS_PHASE_B_NEG    4
#define BPS_PHASE_C_NEG    5

#define METER_UART_DMA_CHL				DMA_Channel0
#define DEBUG_UART_DMA_CHL				DMA_Channel1
#define SPI_DMA_TX_CHL                  DMA_Channel2
#define SPI_DMA_RX_CHL                  DMA_Channel3
extern u8 BootPowerState;

bool ADC_GetCurrentPower(void);
/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/
#endif                                                          /* End of module include.                               */


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
* Filename      : bsp_int.c
* Version       : V1.00
* Programmer(s) : FF
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#define  BSP_INT_MODULE

#include  <cpu.h>
#include  <os.h>

#include  <bsp.h>
#include <gsmcuxx_hal_def.h>

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


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

static  CPU_FNCT_VOID  BSP_IntVectTbl[BSP_INT_SRC_NBR];


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

static  void  BSP_IntHandler     (CPU_DATA  int_id);
static  void  BSP_IntHandlerDummy(void);


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                              BSP_IntClr()
*
* Description : Clear interrupt.
*
* Argument(s) : int_id      Interrupt to clear.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) An interrupt does not need to be cleared within the interrupt controller.
*********************************************************************************************************
*/

void  BSP_IntClr (CPU_DATA  int_id)
{

}


/*
*********************************************************************************************************
*                                              BSP_IntDis()
*
* Description : Disable interrupt.
*
* Argument(s) : int_id      Interrupt to disable.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_IntDis (CPU_DATA  int_id)
{
    if (int_id < BSP_INT_SRC_NBR) {
        CPU_IntSrcDis(int_id + 16u);
    }
}


/*
*********************************************************************************************************
*                                           BSP_IntDisAll()
*
* Description : Disable ALL interrupts.
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

void  BSP_IntDisAll (void)
{
    CPU_IntDis();
}


/*
*********************************************************************************************************
*                                               BSP_IntEn()
*
* Description : Enable interrupt.
*
* Argument(s) : int_id      Interrupt to enable.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_IntEn (CPU_DATA  int_id)
{
    if (int_id < BSP_INT_SRC_NBR) {
        CPU_IntSrcEn(int_id + 16u);
    }
}


/*
*********************************************************************************************************
*                                            BSP_IntVectSet()
*
* Description : Assign ISR handler.
*
* Argument(s) : int_id      Interrupt for which vector will be set.
*
*               isr         Handler to assign
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_IntVectSet (CPU_DATA       int_id,
                      CPU_FNCT_VOID  isr)
{
    CPU_SR_ALLOC();


    if (int_id < BSP_INT_SRC_NBR) {
        CPU_CRITICAL_ENTER();
        BSP_IntVectTbl[int_id] = isr;
        CPU_CRITICAL_EXIT();
    }
}


/*
*********************************************************************************************************
*                                            BSP_IntPrioSet()
*
* Description : Assign ISR priority.
*
* Argument(s) : int_id      Interrupt for which vector will be set.
*
*               prio        Priority to assign
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_IntPrioSet (CPU_DATA    int_id,
                      CPU_INT08U  prio)
{
    CPU_SR_ALLOC();


    if (int_id < BSP_INT_SRC_NBR) {
        CPU_CRITICAL_ENTER();
        CPU_IntSrcPrioSet(int_id + 16u, prio);
        CPU_CRITICAL_EXIT();
    }
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           INTERNAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                              BSP_IntInit()
*
* Description : Initialize interrupts:
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : BSP_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_IntInit (void)
{
    CPU_DATA  int_id;


    for (int_id = 0u; int_id < BSP_INT_SRC_NBR; int_id++) {
        BSP_IntVectSet(int_id, BSP_IntHandlerDummy);
    }
}


/*
*********************************************************************************************************
*                                        BSP_IntHandler####()
*
* Description : Handle an interrupt.
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

//void  SYSC_Handler                     (void)  { BSP_IntHandler(BSP_INT_ID_SYS);                 }
void  WWDG_Handler               (void)  { BSP_IntHandler(WWDG_IRQn);                }
//void  IWDG_Handler               (void)  { BSP_IntHandler(IWDG_IRQn);                 }
#if ZB202
void  TMR0_1_Handler         (void)  { BSP_IntHandler(TMR0_1_IRQn);          }
void  TMR0_2_Handler           (void)  { BSP_IntHandler(TMR0_2_IRQn);            }
void  TMR0_3_Handler              (void)  { BSP_IntHandler(TMR0_3_IRQn);               }
void  TMR0_4_Handler           (void)  { BSP_IntHandler(TMR0_4_IRQn);            }
void  TMR0_5_Handler           (void)  { BSP_IntHandler(TMR0_5_IRQn);            }
void  TMR0_6_Handler           (void)  { BSP_IntHandler(TMR0_6_IRQn);            }
void  TMR1_1_Handler                (void)  { BSP_IntHandler(TMR1_1_IRQn);                 }
void  TMR1_2_Handler              (void)  { BSP_IntHandler(TMR1_2_IRQn);               }
void  TMR1_3_Handler              (void)  { BSP_IntHandler(TMR1_3_IRQn);               }
void  TMR1_4_Handler           (void)  { BSP_IntHandler(TMR1_4_IRQn);            }
void  TMR1_5_Handler           (void)  { BSP_IntHandler(TMR1_5_IRQn);            }
void  TMR1_6_Handler           (void)  { BSP_IntHandler(TMR1_6_IRQn);            }
#elif DM750
void  TMR0_1_Handler         (void)  { BSP_IntHandler(TMR0_1_IRQn);          }
void  TMR0_2_Handler           (void)  { BSP_IntHandler(TMR0_2_IRQn);            }
void  TMR0_3_Handler              (void)  { BSP_IntHandler(TMR0_3_IRQn);               }
void  TMR1_1_Handler                (void)  { BSP_IntHandler(TMR1_1_IRQn);                 }
void  TMR1_2_Handler              (void)  { BSP_IntHandler(TMR1_2_IRQn);               }
void  TMR1_3_Handler              (void)  { BSP_IntHandler(TMR1_3_IRQn);               }
void  TMR2_1_Handler           (void)  { BSP_IntHandler(TMR2_1_IRQn);            }
void  TMR2_2_Handler           (void)  { BSP_IntHandler(TMR2_2_IRQn);            }
void  TMR2_3_Handler           (void)  { BSP_IntHandler(TMR2_3_IRQn);            }
void  TMR2_4_Handler           (void)  { BSP_IntHandler(TMR2_4_IRQn);            }
void  TMR2_5_Handler           (void)  { BSP_IntHandler(TMR2_5_IRQn);            }
void  TMR2_6_Handler           (void)  { BSP_IntHandler(TMR2_6_IRQn);            }
#endif
void  DMAC_Handler           (void)  { BSP_IntHandler(DMAC_IRQn);            }
void  DMAC_TC_Handler           (void)  { BSP_IntHandler(DMAC_TC_IRQn);            }
void  UART0_Handler           (void)  { BSP_IntHandler(UART0_IRQn);            }
void  UART1_Handler           (void)  { BSP_IntHandler(UART1_IRQn);            }
void  UART2_Handler                (void)  { BSP_IntHandler(UART2_IRQn);                 }
void  GPIO0_Handler            (void)  { BSP_IntHandler(GPIO0_IRQn);             }
void  GPIO1_Handler           (void)  { BSP_IntHandler(GPIO1_IRQn);            }
void  IIC0_Handler           (void)  { BSP_IntHandler(IIC0_IRQn);            }
void  UART3_Handler           (void)  { BSP_IntHandler(UART3_IRQn);            }
void  IIC1_Handler            (void)  { BSP_IntHandler(IIC1_IRQn);             }
void  SSP0_Handler      (void)  { BSP_IntHandler(SSP0_IRQn);       }
void  SSP1_Handler      (void)  { BSP_IntHandler(SSP1_IRQn);       }
void  ADCC_Handler      (void)  { BSP_IntHandler(ADCC_IRQn);  }
void  SPI0_Handler            (void)  { BSP_IntHandler(SPI0_IRQn);             }
#if DEV_STA
void  BPLC_IRQHandler               (void)  { BSP_IntHandler(BPLC_IRQn);                }
void  NTB_IRQHandler               (void)  { BSP_IntHandler(NTB_IRQn);                }
#endif
void  CRC_Handler               (void)  { BSP_IntHandler(CRC_IRQn);                }
void  MAC_Handler               (void)  { BSP_IntHandler(MAC_IRQn);                }
void  AES_Handler            (void)  { BSP_IntHandler(AES_IRQn);             }
void  CRY_Handler            (void)  { BSP_IntHandler(CRY_IRQn);             }
void  SHA_Handler            (void)  { BSP_IntHandler(SHA_IRQn);             }
void  EXTI0_Handler            (void)  { BSP_IntHandler(EXTI0_IRQn);             }
void  EXTI1_Handler               (void)  { BSP_IntHandler(EXTI1_IRQn);                }
void  EXTI2_Handler               (void)  { BSP_IntHandler(EXTI2_IRQn);                }
void  EXTI3_Handler             (void)  { BSP_IntHandler(EXTI3_IRQn);              }
void  EXTI4_Handler             (void)  { BSP_IntHandler(EXTI4_IRQn);              }
void  EXTI5_Handler             (void)  { BSP_IntHandler(EXTI5_IRQn);              }
void  EXTI6_Handler          (void)  { BSP_IntHandler(EXTI6_IRQn);           }
void  EXTI7_Handler           (void)  { BSP_IntHandler(EXTI7_IRQn);           }
void  EXTI8_Handler        (void)  { BSP_IntHandler(EXTI8_IRQn);         }
void  EXTI9_Handler         (void)  { BSP_IntHandler(EXTI9_IRQn);      }
void  EXTI10_Handler      (void)  { BSP_IntHandler(EXTI10_IRQn);       }
void  EXTI11_Handler        (void)  { BSP_IntHandler(EXTI11_IRQn);  }
void  EXTI12_Handler            (void)  { BSP_IntHandler(EXTI12_IRQn);             }
void  EXTI13_Handler       (void)  { BSP_IntHandler(EXTI13_IRQn);        }
void  EXTI14_Handler               (void)  { BSP_IntHandler(EXTI14_IRQn);                }
void  EXTI15_Handler               (void)  { BSP_IntHandler(EXTI15_IRQn);                }
void  EXTI16_Handler               (void)  { BSP_IntHandler(EXTI16_IRQn);                }
void  EXTI17_Handler               (void)  { BSP_IntHandler(EXTI17_IRQn);                }
void  EXTI18_Handler             (void)  { BSP_IntHandler(EXTI18_IRQn);              }
void  EXTI19_Handler             (void)  { BSP_IntHandler(EXTI19_IRQn);              }
void  EXTI20_Handler           (void)  { BSP_IntHandler(EXTI20_IRQn);            }
void  EXTI21_Handler               (void)  { BSP_IntHandler(EXTI21_IRQn);                }
void  EXTI22_Handler           (void)  { BSP_IntHandler(EXTI22_IRQn);            }
void  EXTI23_Handler           (void)  { BSP_IntHandler(EXTI23_IRQn);            }
void  EXTI24_Handler           (void)  { BSP_IntHandler(EXTI24_IRQn);            }

/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          BSP_IntHandler()
*
* Description : Central interrupt handler.
*
* Argument(s) : int_id          Interrupt that will be handled.
*
* Return(s)   : none.
*
* Caller(s)   : ISR handlers.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  BSP_IntHandler (CPU_DATA  int_id)
{
    CPU_FNCT_VOID  isr;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();                                       /* Tell the OS that we are starting an ISR            */

    OSIntEnter();

    CPU_CRITICAL_EXIT();

    if (int_id < BSP_INT_SRC_NBR) {
        isr = BSP_IntVectTbl[int_id];
        if (isr != (CPU_FNCT_VOID)0u) {
            isr();
        }
    }

    OSIntExit();                                                /* Tell the OS that we are leaving the ISR            */
}


/*
*********************************************************************************************************
*                                        BSP_IntHandlerDummy()
*
* Description : Dummy interrupt handler.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : BSP_IntHandler().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  BSP_IntHandlerDummy (void)
{
	while (DEF_TRUE) {
        ;
    }
}

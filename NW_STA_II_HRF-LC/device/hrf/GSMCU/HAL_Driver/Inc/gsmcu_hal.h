#ifndef __GSMCU_HAL_H__
#define __GSMCU_HAL_H__
#ifdef __cplusplus
extern "C"
{
#endif
#if defined(ZB204_CHIP)
#include "ZB204.h"
#elif defined(ZB205_CHIP)
#include "ZB205.h"
#endif


#define NVIC_PRIORITYGROUP_0         ((uint32_t)0x00000007) /*!< 0 bits for pre-emption priority
                                                                 4 bits for subpriority */
#define NVIC_PRIORITYGROUP_1         ((uint32_t)0x00000006) /*!< 1 bits for pre-emption priority
                                                                 3 bits for subpriority */
#define NVIC_PRIORITYGROUP_2         ((uint32_t)0x00000005) /*!< 2 bits for pre-emption priority
                                                                 2 bits for subpriority */
#define NVIC_PRIORITYGROUP_3         ((uint32_t)0x00000004) /*!< 3 bits for pre-emption priority
                                                                 1 bits for subpriority */
#define NVIC_PRIORITYGROUP_4         ((uint32_t)0x00000003) /*!< 4 bits for pre-emption priority
                                                                 0 bits for subpriority */






/* ########################### System Configuration ######################### */
/**
  * @brief This is the HAL system configuration section
  */
#define  VDD_VALUE                    ((uint32_t)3300) /*!< Value of VDD in mv */
#define  TICK_INT_PRIORITY            ((uint32_t)0x0F) /*!< tick interrupt priority */
#ifndef  bool
typedef enum {false = 0, true = !false} bool;
#define BOOL        bool
#define TRUE        true
#define FALSE       false
#endif



#include "gsmcu_crc.h"

/* Exported macro ------------------------------------------------------------*/
#ifdef  USE_FULL_ASSERT
/**
  * @brief  The assert_param macro is used for function's parameters check.
  * @param  expr: If expr is false, it calls assert_failed function
  *         which reports the name of the source file and the source
  *         line number of the call that failed.
  *         If expr is true, it returns no value.
  * @retval None
  */
  #define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
/* Exported functions ------------------------------------------------------- */
  void assert_failed(uint8_t* file, uint32_t line);
#else
  #define assert_param(expr) ((void)0)
#endif /* USE_FULL_ASSERT */



#define INTER_WATCH_DOG
 
extern uint32_t SystemCoreClock;          /*!< System Clock Frequency (Core Clock) */  






extern void SystemInit(void);
extern void SystemCoreClockUpdate(void);
void SystemSetCoreDcdc(int isHigh);
extern uint32_t trim_PLL(int32_t freq_diff, uint8_t reset);//*2^24

#include  <bsp.h>

  



#ifdef __cplusplus
}
#endif
#endif /* __GSMCU_HAL_H__ */


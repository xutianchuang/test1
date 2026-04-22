#ifndef __GSMCU_HAL_H__
#define __GSMCU_HAL_H__
#ifdef __cplusplus
extern "C"
{
#endif

#define ISP_UPDATE
#define DEV_PURE_DRV   0
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


#include "iogsmcu.h"
#include "gsmcuxx_hal_def.h"

/* ########################## Module Selection ############################## */
/**
  * @brief This is the list of modules to be used in the HAL driver
  */

#define HAL_DMA_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_IWDG_MODULE_ENABLED
#define HAL_MAC_MODULE_ENABLED
#define HAL_SCU_MODULE_ENABLED
#define HAL_SSP_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED



/* ########################### System Configuration ######################### */
/**
  * @brief This is the HAL system configuration section
  */
#define  VDD_VALUE                    ((uint32_t)3300) /*!< Value of VDD in mv */
#define  TICK_INT_PRIORITY            ((uint32_t)0x0F) /*!< tick interrupt priority */

typedef enum {false = 0, true = !false} bool;
#define BOOL        bool
#define TRUE        true
#define FALSE       false

/* Includes ------------------------------------------------------------------*/
/**
  * @brief Include module's header file
  */

#ifdef HAL_SCU_MODULE_ENABLED
  #include "gsmcu_scu.h"
#endif /* HAL_SCU_MODULE_ENABLED */

#ifdef HAL_GPIO_MODULE_ENABLED
  #include "gsmcu_gpio.h"
#endif /* HAL_GPIO_MODULE_ENABLED */

#ifdef HAL_DMA_MODULE_ENABLED
  #include "gsmcu_dma.h"
#endif /* HAL_DMA_MODULE_ENABLED */

#ifdef HAL_SSP_MODULE_ENABLED
  #include "gsmcu_ssp.h"
#endif /* HAL_SSP_MODULE_ENABLED */

#ifdef HAL_ADC_MODULE_ENABLED
  #include "gsmcu_adc.h"
#endif /* HAL_ADC_MODULE_ENABLED */

#ifdef HAL_MAC_MODULE_ENABLED
  #include "gsmcu_mac.h"
#endif /* HAL_MAC_MODULE_ENABLED */

#ifdef HAL_FLASH_MODULE_ENABLED
  #include "gsmcu_flash.h"
#endif /* HAL_FLASH_MODULE_ENABLED */

#ifdef HAL_TIM_MODULE_ENABLED
  #include "gsmcu_tim.h"
#endif /* HAL_TIM_MODULE_ENABLED */

#ifdef HAL_UART_MODULE_ENABLED
  #include "gsmcu_usart.h"
#endif /* HAL_UART_MODULE_ENABLED */

#ifdef HAL_I2C_MODULE_ENABLED
 #include "gsmcu_iic.h"
#endif /* HAL_I2C_MODULE_ENABLED */

#ifdef HAL_IWDG_MODULE_ENABLED
 #include "gsmcu_iwdg.h"
#endif /* HAL_IWDG_MODULE_ENABLED */

#ifdef HAL_CORTEX_MODULE_ENABLED
#include "gsmcu_cortex.h"
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


/* Initialization and de-initialization functions  ******************************/
HAL_StatusTypeDef HAL_Init(void);
HAL_StatusTypeDef HAL_DeInit(void);
void HAL_MspInit(void);
void HAL_MspDeInit(void);
HAL_StatusTypeDef HAL_InitTick (uint32_t TickPriority);
/**
  * @}
  */

/** @addtogroup HAL_Exported_Functions_Group2
  * @{
  */
/* Peripheral Control functions  ************************************************/
void HAL_IncTick(void);
void HAL_Delay(__IO uint32_t Delay);
uint32_t HAL_GetTick(void);
void HAL_SuspendTick(void);
void HAL_ResumeTick(void);
uint32_t HAL_GetHalVersion(void);
uint32_t HAL_GetREVID(void);
uint32_t HAL_GetDEVID(void);
void HAL_DBGMCU_EnableDBGSleepMode(void);
void HAL_DBGMCU_DisableDBGSleepMode(void);
void HAL_DBGMCU_EnableDBGStopMode(void);
void HAL_DBGMCU_DisableDBGStopMode(void);
void HAL_DBGMCU_EnableDBGStandbyMode(void);
void HAL_DBGMCU_DisableDBGStandbyMode(void);
void HAL_EnableCompensationCell(void);
void HAL_DisableCompensationCell(void);
uint32_t HAL_RCC_GetHCLKFreq(void);


extern void SystemInit(void);
extern void SystemCoreClockUpdate(void);
extern uint32_t trim_PLL(int32_t freq_diff, uint8_t reset);//*2^24

#include  <bsp.h>

  



#ifdef __cplusplus
}
#endif
#endif /* __GSMCU_HAL_H__ */


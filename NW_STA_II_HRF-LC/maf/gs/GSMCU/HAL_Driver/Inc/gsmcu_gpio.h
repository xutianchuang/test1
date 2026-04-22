#ifndef __GSMCU_GPIO_H__
#define __GSMCU_GPIO_H__
#ifdef __cplusplus
extern "C"
{
#endif




/** @defgroup GPIO_Exported_Types
  * @{
  */

#define IS_GPIO_ALL_PERIPH(PERIPH) (((PERIPH) == GPIO0) || \
                                    ((PERIPH) == GPIO1))



/** 
  * @brief  Configuration Mode enumeration  
  */

typedef enum
{
  GPIO_Mode_IN_FLOATING = 0x00,
  GPIO_Mode_IPD = 0x02,
  GPIO_Mode_IPU = 0x03,
  GPIO_Mode_OPP = 0x04,
  GPIO_Mode_OPD = 0x06,
  GPIO_Mode_OPU = 0x07,

}GPIOMode_TypeDef;

#define IS_GPIO_MODE(MODE) (((MODE) == GPIO_Mode_IN_FLOATING) || ((MODE) == GPIO_Mode_IPD) || \
                            ((MODE) == GPIO_Mode_IPU) || ((MODE) == GPIO_Mode_OPP) || \
                            ((MODE) == GPIO_Mode_OPD) || ((MODE) == GPIO_Mode_OPU))

#define GPIO_Trigger_Rising                 ((uint32_t)0x00000000)
#define GPIO_Trigger_Falling                ((uint32_t)0x00000001)
#define GPIO_Trigger_Rising_Falling         ((uint32_t)0x00000100)
#define GPIO_Trigger_High_level             ((uint32_t)0x00010000)
#define GPIO_Trigger_Low_level              ((uint32_t)0x00010001)
/** 
  * @brief  Bit_SET and Bit_RESET enumeration  
  */

typedef enum
{ Bit_RESET = 0,
  Bit_SET
}BitAction;

#define IS_GPIO_BIT_ACTION(ACTION) (((ACTION) == Bit_RESET) || ((ACTION) == Bit_SET))

/** 
  * @brief  GPIO Init structure definition  
  */

typedef struct
{
  uint32_t GPIO_Pin;             /*!< Specifies the GPIO pins to be configured.
                                      This parameter can be any value of @ref GPIO_pins_define */

  GPIOMode_TypeDef GPIO_Mode;    /*!< Specifies the operating mode for the selected pins.
                                      This parameter can be a value of @ref GPIOMode_TypeDef */
}GPIO_InitTypeDef;





/** @defgroup GPIO_pins_define 
  * @{
  */
enum
{
	GPIO_GROUP0 = 0, GPIO_GROUP1 = 1
};


#define GPIO_Pin_0                 ((uint32_t)0x0001)  /*!< Pin 0 selected */
#define GPIO_Pin_1                 ((uint32_t)0x0002)  /*!< Pin 1 selected */
#define GPIO_Pin_2                 ((uint32_t)0x0004)  /*!< Pin 2 selected */
#define GPIO_Pin_3                 ((uint32_t)0x0008)  /*!< Pin 3 selected */
#define GPIO_Pin_4                 ((uint32_t)0x0010)  /*!< Pin 4 selected */
#define GPIO_Pin_5                 ((uint32_t)0x0020)  /*!< Pin 5 selected */
#define GPIO_Pin_6                 ((uint32_t)0x0040)  /*!< Pin 6 selected */
#define GPIO_Pin_7                 ((uint32_t)0x0080)  /*!< Pin 7 selected */
#define GPIO_Pin_8                 ((uint32_t)0x0100)  /*!< Pin 8 selected */
#define GPIO_Pin_9                 ((uint32_t)0x0200)  /*!< Pin 9 selected */
#define GPIO_Pin_10                ((uint32_t)0x0400)  /*!< Pin 10 selected */
#define GPIO_Pin_11                ((uint32_t)0x0800)  /*!< Pin 11 selected */
#define GPIO_Pin_12                ((uint32_t)0x1000)  /*!< Pin 12 selected */
#define GPIO_Pin_13                ((uint32_t)0x2000)  /*!< Pin 13 selected */
#define GPIO_Pin_14                ((uint32_t)0x4000)  /*!< Pin 14 selected */
#define GPIO_Pin_15                ((uint32_t)0x8000)  /*!< Pin 15 selected */
#define GPIO_Pin_16                ((uint32_t)0x10000)  /*!< Pin 16 selected */
#define GPIO_Pin_17                ((uint32_t)0x20000)  /*!< Pin 17 selected */
#define GPIO_Pin_18                ((uint32_t)0x40000)  /*!< Pin 18 selected */
#define GPIO_Pin_19                ((uint32_t)0x80000)  /*!< Pin 19 selected */
#define GPIO_Pin_20                ((uint32_t)0x100000)  /*!< Pin 20 selected */
#define GPIO_Pin_21                ((uint32_t)0x200000)  /*!< Pin 21 selected */
#define GPIO_Pin_22                ((uint32_t)0x400000)  /*!< Pin 22 selected */
#define GPIO_Pin_23                ((uint32_t)0x800000)  /*!< Pin 23 selected */
#define GPIO_Pin_24                ((uint32_t)0x1000000)  /*!< Pin 24 selected */
#define GPIO_Pin_25                ((uint32_t)0x2000000)  /*!< Pin 25 selected */
#define GPIO_Pin_26                ((uint32_t)0x4000000)  /*!< Pin 26 selected */
#define GPIO_Pin_27                ((uint32_t)0x8000000)  /*!< Pin 27 selected */
#define GPIO_Pin_28                ((uint32_t)0x10000000)  /*!< Pin 28 selected */
#define GPIO_Pin_29                ((uint32_t)0x20000000)  /*!< Pin 29 selected */
#define GPIO_Pin_30                ((uint32_t)0x40000000)  /*!< Pin 30 selected */
#define GPIO_Pin_31                ((uint32_t)0x80000000)  /*!< Pin 31 selected */
#define GPIO_Pin_All               ((uint32_t)0xFFFFFFFF)  /*!< All pins selected */

#define IS_GPIO_PIN(PIN) ((((PIN) & (uint32_t)0x00) == 0x00) && ((PIN) != (uint32_t)0x00))

#define IS_GET_GPIO_PIN(PIN) (((PIN) == GPIO_Pin_0) || \
                              ((PIN) == GPIO_Pin_1) || \
                              ((PIN) == GPIO_Pin_2) || \
                              ((PIN) == GPIO_Pin_3) || \
                              ((PIN) == GPIO_Pin_4) || \
                              ((PIN) == GPIO_Pin_5) || \
                              ((PIN) == GPIO_Pin_6) || \
                              ((PIN) == GPIO_Pin_7) || \
                              ((PIN) == GPIO_Pin_8) || \
                              ((PIN) == GPIO_Pin_9) || \
                              ((PIN) == GPIO_Pin_10) || \
                              ((PIN) == GPIO_Pin_11) || \
                              ((PIN) == GPIO_Pin_12) || \
                              ((PIN) == GPIO_Pin_13) || \
                              ((PIN) == GPIO_Pin_14) || \
                              ((PIN) == GPIO_Pin_15) || \
                              ((PIN) == GPIO_Pin_16) || \
                              ((PIN) == GPIO_Pin_17) || \
                              ((PIN) == GPIO_Pin_18) || \
                              ((PIN) == GPIO_Pin_19) || \
                              ((PIN) == GPIO_Pin_20) || \
                              ((PIN) == GPIO_Pin_21) || \
                              ((PIN) == GPIO_Pin_22) || \
                              ((PIN) == GPIO_Pin_23) || \
                              ((PIN) == GPIO_Pin_24) || \
                              ((PIN) == GPIO_Pin_25) || \
                              ((PIN) == GPIO_Pin_26) || \
                              ((PIN) == GPIO_Pin_27) || \
                              ((PIN) == GPIO_Pin_28) || \
                              ((PIN) == GPIO_Pin_29) || \
                              ((PIN) == GPIO_Pin_30) || \
                              ((PIN) == GPIO_Pin_31))


/**
  * @}
  */

/** @defgroup GPIO_Exported_Functions
  * @{
  */
void GPIO_GlobalInit();
void GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct);
void GPIO_StructInit(GPIO_InitTypeDef* GPIO_InitStruct);

uint8_t GPIO_ReadInputDataBit(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin);
uint32_t GPIO_ReadInputData(GPIO_TypeDef* GPIOx);
uint8_t GPIO_ReadOutputDataBit(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin);
uint32_t GPIO_ReadOutputData(GPIO_TypeDef* GPIOx);

void GPIO_SetBits(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin);
void GPIO_ResetBits(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin);
void GPIO_WriteBit(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin, BitAction BitVal);
void GPIO_Write(GPIO_TypeDef* GPIOx, uint32_t PortVal);

void GPIO_SetIRQMode(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin, uint8_t mode);
void GPIO_SetIRQTriggerMode(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin, uint32_t mode);
void GPIO_ClearIRQStatus(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin);

void GPIO_EnableBounce(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin);
void GPIO_DisableBounce(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin);
void GPIO_SetBouncePreScale(GPIO_TypeDef* GPIOx, uint32_t pre_scale);

unsigned int GPIO_GetIntStatus(GPIO_TypeDef *GPIOx);
unsigned int GPIO_ClearPortIntStatus(GPIO_TypeDef *GPIOx, unsigned int pins);
#ifdef __cplusplus
}
#endif
#endif /* __GSMCU_GPIO_H__ */


#ifndef __GSMCU_TIM_H__
#define __GSMCU_TIM_H__
#ifdef __cplusplus
extern "C"
{
#endif



/** @defgroup time_Exported_Types
  * @{
  */

#define IS_TIM_ALL_PERIPH(PERIPH) (((PERIPH) == TMR0) || \
                                    ((PERIPH) == TMR1)|| \
                                    ((PERIPH) == TMR2))


/** @defgroup channel_Exported_Types
  * @{
  */

#define TIME_CHANNEL_1      (0x01)
#define TIME_CHANNEL_2      (0x02)
#define TIME_CHANNEL_3      (0x03)
#define TIME_CHANNEL_4      (0x04)
#define TIME_CHANNEL_5      (0x05)
#define TIME_CHANNEL_6      (0x06)
#define TIME_CHANNEL_7      (0x07)
#define TIME_CHANNEL_8      (0x08)

#define IS_TIME_CHANNEL(CHL) ((CHL == TIME_CHANNEL_1)||(CHL == TIME_CHANNEL_2)||\
                                (CHL == TIME_CHANNEL_3)||(CHL == TIME_CHANNEL_4)||\
                                (CHL == TIME_CHANNEL_5)||(CHL == TIME_CHANNEL_6)||\
                                (CHL == TIME_CHANNEL_7)||(CHL == TIME_CHANNEL_8))
//
#define TIM_INT_LEVEL_MODE    (0x01<<6)
#define TIM_INT_PULSE_MODE    (0x00<<6)

#define TIM_INT_ENABLE      (0x01<<5)
#define TIM_INT_DISABLE     (0x00<<5)

#define TIM_AUTO_RELOAD_ON      (0x01<<4)
#define TIM_AUTO_RELOAD_OFF     (0x00<<4)

#define TIM_OUTPUT_INVERTER_OFF      (0x00<<3)
#define TIM_OUTPUT_INVERTER_ON       (0x01<<3)

#define TIM_UPDATA      (0x01<<2)
#define TIM_START      (0x01<<1)
#define TIM_PWM_ENABLE   (0x1<<8)
//

#define  IS_TIM_CONFIGVALUE(CONFIG)     (((CONFIG)&(~(TIM_INT_LEVEL_MODE|TIM_INT_ENABLE| \
                                        TIM_AUTO_RELOAD_ON|TIM_OUTPUT_INVERTER_OFF| \
                                        TIM_UPDATA|TIM_START))) ==0x00)


/** 
  * @brief  time Init structure definition  
  */

typedef struct
{
  uint32_t DeadZone; 
  uint32_t CfgValue;
  uint32_t TimReload;
  uint32_t CmpValue;
}TIM_InitTypeDef;


//
void TIM_Init(TMR_TypeDef * TMR,uint8_t TimChl,TIM_InitTypeDef* CHL_InitStruct);

void TIM_Start(TMR_TypeDef * TMR,uint8_t TimChl);
void TIM_Stop(TMR_TypeDef * TMR,uint8_t TimChl);

void TIM_ClearFlag(TMR_TypeDef* TMR, uint8_t TimChl);
ITStatus TIM_GetITStatus(TMR_TypeDef* TMR, uint8_t TimChl);
uint32_t TIM_GetChlCounter(TMR_TypeDef* TMR, uint8_t TimChl);





#ifdef __cplusplus
}
#endif
#endif /* __GSMCU_TIM_H__ */


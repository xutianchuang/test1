#include  <gsmcu_hal.h>


/** @defgroup Functions
  * @{
  */
void TIM_Init(TMR_TypeDef * TMR,uint8_t TimChl,TIM_InitTypeDef *CHL_InitStruct)
{
  /* Check the parameters */
  assert_param(IS_TIM_ALL_PERIPH(TMR));
  assert_param(IS_TIME_CHANNEL(TimChl));
  assert_param(IS_TIM_CONFIGVALUE(CHL_InitStruct->CfgValue));
//
  TMR_Chl_TypeDef *TMRCHL = (TMR_Chl_TypeDef *)(((uint32_t)TMR)+(0x10*TimChl));
  
  TMRCHL->CHL_CNTB = (CHL_InitStruct->TimReload);
  TMRCHL->CHL_CMPB = (CHL_InitStruct->CmpValue);
  TMRCHL->CHL_CTRL = ((CHL_InitStruct->DeadZone)<<24)|(CHL_InitStruct->CfgValue);
  
}

void TIM_Start(TMR_TypeDef * TMR,uint8_t TimChl)
{
  /* Check the parameters */
  assert_param(IS_TIM_ALL_PERIPH(TMR));
  assert_param(IS_TIME_CHANNEL(TimChl));
//
  TMR_Chl_TypeDef *TMRCHL = (TMR_Chl_TypeDef *)(((uint32_t)TMR)+(0x10*TimChl));

  TMRCHL->CHL_CTRL |= TIM_START;

}
void TIM_Stop(TMR_TypeDef * TMR,uint8_t TimChl)
{
  /* Check the parameters */
  assert_param(IS_TIM_ALL_PERIPH(TMR));
  assert_param(IS_TIME_CHANNEL(TimChl));
//
  TMR_Chl_TypeDef *TMRCHL = (TMR_Chl_TypeDef *)(((uint32_t)TMR)+(0x10*TimChl));

  TMRCHL->CHL_CTRL  &= ~TIM_START;

}

void TIM_ClearFlag(TMR_TypeDef* TMR, uint8_t TimChl)
{
    /* Check the parameters */
    assert_param(IS_TIM_ALL_PERIPH(TMR));
    assert_param(IS_TIME_CHANNEL(TimChl));
    //
    TMR->INT_CSTAT = (0x01<<(TimChl-1));
}
ITStatus TIM_GetITStatus(TMR_TypeDef* TMR, uint8_t TimChl)
{
    /* Check the parameters */
    assert_param(IS_TIM_ALL_PERIPH(TMR));
    assert_param(IS_TIME_CHANNEL(TimChl));
    //
    uint32_t pos = (0x01<<(TimChl-1));
    if(((TMR->INT_CSTAT) & pos) == pos)
    {
        return SET;
    }
    else
    {
        return RESET;
    }
}

uint32_t TIM_GetChlCounter(TMR_TypeDef* TMR, uint8_t TimChl)
{
    /* Check the parameters */
    assert_param(IS_TIM_ALL_PERIPH(TMR));
    assert_param(IS_TIME_CHANNEL(TimChl));
    //
    TMR_Chl_TypeDef *TMRCHL = (TMR_Chl_TypeDef *)(((uint32_t)TMR)+(0x10*TimChl));

    return TMRCHL->CHL_CNTO;
}







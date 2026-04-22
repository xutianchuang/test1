
#include  <gsmcu_hal.h>

/**************************************************/

#define ADC_SWSTART_SET_BIT         (0x01<<4)
#define ADC_ENABLE_SET_BIT          (0x01<<0)

/**
  * @brief  
  * @param  
  * @param  
  * @retval None 
  */
void ADC_Init(ADC_TypeDef* ADCx, ADC_InitTypeDef* ADC_InitStruct)
{
    assert_param(IS_ADC_PERIPH(ADCx));
    assert_param(IS_ADC_AUTOPWDN(ADC_InitStruct->AUTOPWDN));
    assert_param(IS_ADC_COVN_MODE(ADC_InitStruct->COVN_MODE));
    assert_param(IS_ADC_CHL(ADC_InitStruct->ADCFristChl));
    //
    //ADCx->PRESCAL  = ADC_InitStruct->MCLK_DIV; //dev
    ADCx->CTRL &= ~ADC_ENABLE_SET_BIT;
    SCU->EX_GPIO_DRIVER_OTHER&=~(1<<21);//ref 使用vdd3v3

    ADCx->DISCHTIME = ADC_InitStruct->DISCHTime; //dichang time
    
    ADCx->SMPR = (ADC_InitStruct->CHDLYTime << 24)|
        (ADC_InitStruct->CHSELTime << 20)|
            (ADC_InitStruct->HOLDTime << 16)|
         (ADC_InitStruct->SAMPLTime << 8)|
          (ADC_InitStruct->SETTLTime << 0);
    
    ADCx->TPARM = (ADC_InitStruct->TSWKUPTime<<24)|(ADC_InitStruct->WKUPTime<<16)|(ADC_InitStruct->PWRENTime<<0);

    ADCx->THRHOLD0_1 = 0x00;
    ADCx->THRHOLD2_3 = 0x00;
    ADCx->THRHOLD4_5 = 0x00;
    ADCx->THRHOLD6_7 = 0x00;
    ADCx->THRHOLD8_9 = 0x00;
    ADCx->THRHOLD10_11 = 0x00;
    ADCx->THRHOLD12_13 = 0x00;
    ADCx->THRHOLD14_15 = 0x00;
    
    ADCx->SQR = (ADC_InitStruct->ADCFristChl)&0x07;
 
    ADCx->CTRL = (0<<16)|(0x01UL<<15)|((ADC_InitStruct->AUTOPWDN)<<12)|((ADC_InitStruct->COVN_MODE)<<8);
}
/**
  * @brief 
  * @param  
  * @param  
  * @retval None
  */
void ADC_SetIntrEn(ADC_TypeDef* ADCx, uint32_t intren)
{
    assert_param(IS_ADC_PERIPH(ADCx));
    assert_param(IS_ADC_INTEN(intren));
    ADCx->INTEN |= intren;
}
/**
  * @brief 
  * @param  
  * @param  
  * @retval None
  */
void ADC_ResetIntrEn(ADC_TypeDef* ADCx, uint32_t intren)
{
    assert_param(IS_ADC_PERIPH(ADCx));
    assert_param(IS_ADC_INTEN(intren));
    ADCx->INTEN &= ~intren;
}
/**
  * @brief 
  * @param  
  * @param  
  * @retval None
  */
void ADC_Enable(ADC_TypeDef* ADCx)
{
	volatile int i=50;
    assert_param(IS_ADC_PERIPH(ADCx));
	while (i--)
	{
		__NOP();
	}
	ADCx->CTRL |= ADC_ENABLE_SET_BIT;
	i=100;
	while (i--)
	{
		__NOP();
	}
}
/**
  * @brief 
  * @param  
  * @param  
  * @retval None
  */
void ADC_Disable(ADC_TypeDef* ADCx)
{
    assert_param(IS_ADC_PERIPH(ADCx));
    ADCx->CTRL &= ~ADC_ENABLE_SET_BIT;
}
/**
  * @brief 
  * @param  
  * @param  
  * @retval None
  */
void ADC_TriggerConverson(ADC_TypeDef* ADCx)
{
    assert_param(IS_ADC_PERIPH(ADCx));
    ADCx->CTRL |= ADC_SWSTART_SET_BIT;
}
/**
  * @brief 
  * @param  
  * @param  
  * @retval None
  */
uint32_t ADC_GetChannelCovnData(ADC_TypeDef* ADCx, uint32_t ADC_Chl)
{
     assert_param(IS_ADC_PERIPH(ADCx));
     assert_param(IS_ADC_CHL(ADC_Chl));
     return *((uint32_t*)((uint32_t)(&(ADCx->DATACH0)) + (ADC_Chl<<2)));
}
/**
  * @brief 
  * @param  
  * @param  
  * @retval None
  */
void ADC_AddChannel(ADC_TypeDef* ADCx, uint32_t ADC_Chl)
{
    uint32_t chlsum;
    bool isinchlflg = false;
    assert_param(IS_ADC_PERIPH(ADCx));
    assert_param(IS_ADC_CHL(ADC_Chl));
    
    chlsum = ((ADCx->CTRL>>16)&0x07) + 1;
    if(chlsum < 8)
    {
        uint32_t adcchllist = ADCx->SQR;
        for(int i=0;i<chlsum;i++)
        {
            if(ADC_Chl == ((adcchllist>>(i*4))&0x07))
            {
                isinchlflg = true;
                break;
            }
        }
    }
    else
    {
        isinchlflg = true;
    }
    //不在列表中，添加chl
    if(isinchlflg == false)
    {
        ADCx->SQR |= ((ADC_Chl << (chlsum*4)));
        ADCx->CTRL = ((ADCx->CTRL)&(~(0x07UL<<16)))|(chlsum<<16);
    }
}
/**
  * @brief  
  * @param  
  * @param  
  * @retval None
  */
void ADC_SetChannelThr(ADC_TypeDef* ADCx,uint32_t Thr_chl, uint32_t ADC_Chl,uint32_t htr,uint32_t ltr)
{
     assert_param(IS_ADC_PERIPH(ADCx));
     assert_param(IS_ADC_CHL(ADC_Chl));
     
     *((volatile uint32_t*)((uint32_t)(&(ADCx->THRHOLD0_1)) + (Thr_chl<<2))) = 
         (0x01UL<<31)| (ADC_Chl<<28) |((htr&0xFFF)<<16)|
         (0x00UL<<15)| (ADC_Chl<<12) |(ltr&0xFFF);
}


/**
  * @brief  
  * @param  
  * @param  
  * @retval None
  */
uint32_t ADC_GetIntStatus(ADC_TypeDef* ADCx)
{
    assert_param(IS_ADC_PERIPH(ADCx));     
    return ADCx->INTR;
}

/**
  * @brief  
  * @param  
  * @param  
  * @retval None
  */
void ADC_ResetIntStatus(ADC_TypeDef* ADCx,uint32_t intstatus)
{
    assert_param(IS_ADC_PERIPH(ADCx));
    assert_param(IS_ADC_INTRSTS(intstatus));
    ADCx->INTR = intstatus;
}



















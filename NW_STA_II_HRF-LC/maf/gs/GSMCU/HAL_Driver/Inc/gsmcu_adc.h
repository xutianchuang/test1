#ifndef __GSMCU_ADC_H__
#define __GSMCU_ADC_H__
#ifdef __cplusplus
extern "C"
{
#endif

#define IS_ADC_PERIPH(PERIPH) ((PERIPH) == ADC) 
//ADC Channel
#define ADC_CHL_0   (0)
#define ADC_CHL_1   (1)
#define ADC_CHL_2   (2)
#define ADC_CHL_3   (3)
#define ADC_CHL_4   (4)
#define ADC_CHL_5   (5)
#define ADC_CHL_6   (6)
#define ADC_CHL_7   (7)

#define IS_ADC_CHL(CHL) (((CHL) >= ADC_CHL_0)&&((CHL) <= ADC_CHL_7)) 
//auto power down
#define AUTOPWDN_ENABLE     (1)
#define AUTOPWDN_DISABLE     (0)

#define IS_ADC_AUTOPWDN(FLG)    (((FLG) == AUTOPWDN_ENABLE)||((FLG) == AUTOPWDN_DISABLE)) 

// conversion mode
#define SINGLE_STEP_COVN_MODE       (0)
#define SINGLE_SCAM_COVN_MODE       (1)
#define CONTINUE_SCAM_COVN_MODE     (2)

#define IS_ADC_COVN_MODE(MODE)    (((MODE) == SINGLE_STEP_COVN_MODE)|| \
                                    ((MODE) == SINGLE_SCAM_COVN_MODE)||\
                                    ((MODE) == CONTINUE_SCAM_COVN_MODE))
//ADC DMA
#define ADC_DMA_0   (0)
#define ADC_DMA_1   (1)
#define ADC_DMA_2   (2)
#define ADC_DMA_3   (3)

#define IS_ADC_DMA(ADC_DMA) (((ADC_DMA) >= ADC_DMA_0)&&((ADC_DMA) <= ADC_DMA_3)) 
//ADC interrupt enable flg

#define CH0_EOC_INTREN   ((0x01UL)<<8)
#define CH1_EOC_INTREN   ((0x01UL)<<9)
#define CH2_EOC_INTREN   ((0x01UL)<<10)
#define CH3_EOC_INTREN   ((0x01UL)<<11)
#define CH4_EOC_INTREN   ((0x01UL)<<12)
#define CH5_EOC_INTREN   ((0x01UL)<<13)
#define CH6_EOC_INTREN   ((0x01UL)<<14)
#define CH7_EOC_INTREN   ((0x01UL)<<15)

#define TS_OVR_INTREN    ((0x01UL)<<3)
#define TS_UDR_INTREN    ((0x01UL)<<2)
#define ADC_STOP_INTREN  ((0x01UL)<<1)
#define ADC_DONE_INTREN  ((0x01UL)<<0)
#define ADC_THRESHOLD(chl)    (1<<(chl+16))
#define IS_ADC_INTEN(INTREN) (((INTREN)&(~0xFF0FUL)) == 0x00)
//
#define CH0_EOC_INTRSTS   ((0x01UL)<<8)
#define CH1_EOC_INTRSTS   ((0x01UL)<<9)
#define CH2_EOC_INTRSTS   ((0x01UL)<<10)
#define CH3_EOC_INTRSTS   ((0x01UL)<<11)
#define CH4_EOC_INTRSTS   ((0x01UL)<<12)
#define CH5_EOC_INTRSTS   ((0x01UL)<<13)
#define CH6_EOC_INTRSTS   ((0x01UL)<<14)
#define CH7_EOC_INTRSTS   ((0x01UL)<<15)

#define DMACH0_INTRSTS      ((0x01UL)<<4)
#define DMACH1_INTRSTS      ((0x01UL)<<5)
#define DMACH2_INTRSTS      ((0x01UL)<<6)
#define DMACH3_INTRSTS      ((0x01UL)<<7)

#define TS_OVR_INTRSTS    ((0x01UL)<<3)
#define TS_UDR_INTRSTS    ((0x01UL)<<2)
#define ADC_STOP_INTRSTS  ((0x01UL)<<1)
#define ADC_DONE_INTRSTS  ((0x01UL)<<0)

#define IS_ADC_INTRSTS(INTRSTS) (((INTRSTS)&(~0xFFfFUL)) == 0x00)
//
typedef struct 
{
    uint32_t AUTOPWDN;
    uint32_t COVN_MODE;
    
    //uint32_t SAMPL_MODE;
    uint32_t MCLK_DIV;
    
    uint32_t WKUPTime;
    uint32_t PWRENTime;
	uint32_t TSWKUPTime;
    uint32_t SETTLTime;

    uint32_t SAMPLTime ;
    uint32_t HOLDTime ;
    uint32_t CHSELTime ;   
    uint32_t CHDLYTime;
    uint32_t DISCHTime;
    
    uint32_t ADCFristChl;
}ADC_InitTypeDef;


//
void ADC_Init(ADC_TypeDef* ADCx, ADC_InitTypeDef* ADC_InitStruct);
void ADC_SetIntrEn(ADC_TypeDef* ADCx, uint32_t intren);
void ADC_ResetIntrEn(ADC_TypeDef* ADCx, uint32_t intren);
void ADC_Enable(ADC_TypeDef* ADCx);
void ADC_Disable(ADC_TypeDef* ADCx);
void ADC_TriggerConverson(ADC_TypeDef* ADCx);
uint32_t ADC_GetChannelCovnData(ADC_TypeDef* ADCx, uint32_t ADC_Chl);
void ADC_AddChannel(ADC_TypeDef* ADCx, uint32_t ADC_Chl);
void ADC_SetChannelThr(ADC_TypeDef* ADCx,uint32_t Thr_chl,  uint32_t ADC_Chl,uint32_t htr,uint32_t ltr);
uint32_t ADC_GetIntStatus(ADC_TypeDef* ADCx);
void ADC_ResetIntStatus(ADC_TypeDef* ADCx,uint32_t intstatus);
//
#ifdef __cplusplus
}
#endif
#endif /* __GSMCU_ADC_H__ */



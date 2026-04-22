
#include  <gsmcu_hal.h>


/** @
  */
#define CFG_DST_RS_SHIFT    (0x09)
#define CFG_SRC_RS_SHIFT    (0x03)

/**
  * @brief  
  * @param  
  * @retval 
  */
void DMA_Init(DMA_TypeDef* DMAx, uint32_t DMA_Channelx,DMA_InitTypeDef* DMA_InitStruct)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    assert_param(IS_DMA_CHANNEL(DMA_InitStruct->LLP));
    //assert_param(IS_DMA_PERIPH(DMAx));  
    
    DMA_Channel_TypeDef * DmaChl = (DMA_Channel_TypeDef*)(((uint32_t)(&(DMAx->C0_CSR))) + (DMA_Channelx*0x20));
    //===========
    DMAx->CSR = 0x01;//Little endian, enable DMA 
    //
    if(DMA_InitStruct->SYNC_Flg == SET)
    {
        DMAx->SYNC |= 0x01<<DMA_Channelx;
    }
    //====
    DmaChl->CFG = (DMA_InitStruct->CFG)|
        (DMA_InitStruct->ChlPeriph<<CFG_DST_RS_SHIFT)|
        (DMA_InitStruct->ChlPeriph<<CFG_SRC_RS_SHIFT);
    
    DmaChl->SrcAdd = DMA_InitStruct->SrcAdd;
    DmaChl->DstAddr = DMA_InitStruct->DstAddr;   
    DmaChl->LLP = (DMA_InitStruct->LLP)<<2;
    DmaChl->SIZE = DMA_InitStruct->SIZE;
    DmaChl->CSR = DMA_InitStruct->CSR;
    
}

/**
  * @brief  
  * @param  
  * @retval 
  */
void Set_DMA_Channel(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //assert_param(IS_DMA_PERIPH(DMAx));  
    
    DMA_Channel_TypeDef * DmaChl = (DMA_Channel_TypeDef*)(((uint32_t)(&(DMAx->C0_CSR))) + (DMA_Channelx*0x20));
    DmaChl->CSR |= 0x01UL;
}
/**
  * @brief  
  * @param  
  * @retval 
  */
void ReSet_DMA_Channel(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //assert_param(IS_DMA_PERIPH(DMAx));  
    
    DMA_Channel_TypeDef * DmaChl = (DMA_Channel_TypeDef*)(((uint32_t)(&(DMAx->C0_CSR))) + (DMA_Channelx*0x20));
    DmaChl->CSR &= ~0x01UL;
}

/**
  * @brief  
  * @param  
  * @retval 
  */
ITStatus DMA_GetChannelITStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    if(((DMAx->INTSR) & (0x01<<DMA_Channelx)) == 0x00)
    {
        return RESET;
    }
    else
    {
        return SET;
    }
}

/**
  * @brief  
  * @param  
  * @retval 
  */
uint32_t DMA_GetITStatus(DMA_TypeDef* DMAx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    return ((DMAx->INTSR) & 0xFF);
}
/**
  * @brief  
  * @param  
  * @retval 
  */
ITStatus DMA_GetChannelTCITStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    if(((DMAx->INTTC) & (0x01<<DMA_Channelx)) == 0x00)
    {
        return RESET;
    }
    else
    {
        return SET;
    }
}
/**
  * @brief  
  * @param  
  * @retval 
  */
void DMA_ClearChannelTCITStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    DMAx->INTTC_CLR |= (0x01<<DMA_Channelx);
}

/**
  * @brief  
  * @param  
  * @retval 
  */
ITStatus DMA_GetChannelAbtITStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    if(((DMAx->INT_ERR_ABT) & (0x01<<(DMA_Channelx+16))) == 0x00)
    {
        return RESET;
    }
    else
    {
        return SET;
    }
}
/**
  * @brief  
  * @param  
  * @retval 
  */
void DMA_ClearChannelAbtITStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    DMAx->INT_ERR_ABT_CLR |= (0x01 << (DMA_Channelx+16));
}

/**
  * @brief  
  * @param  
  * @retval 
  */
ITStatus DMA_GetChannelErrITStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    if(((DMAx->INT_ERR_ABT) & (0x01<<DMA_Channelx)) == 0x00)
    {
        return RESET;
    }
    else
    {
        return SET;
    }
}
/**
  * @brief  
  * @param  
  * @retval 
  */
void DMA_ClearChannelErrITStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    DMAx->INT_ERR_ABT_CLR |= (0x01<<DMA_Channelx);
}
/**
  * @brief  
  * @param  
  * @retval 
  */
uint32_t DMA_GetAllChannelAllITStatus(DMA_TypeDef* DMAx)
{
    uint32_t Status;
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    //
    Status = ((DMAx->INTTC)&0xFF)<<8;
    Status |= ((DMAx->INT_ERR_ABT)&(~(0xFFUL<<8)));
    return Status;
}
/**
  * @brief  
  * @param  
  * @retval 
  */
void DMA_ClearAllChannelAllITStatus(DMA_TypeDef* DMAx, uint32_t clvalue)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    DMAx->INT_ERR_ABT_CLR |= (clvalue &(~(0xFFUL<<8)));
    DMAx->INTTC_CLR |= ((clvalue >> 8)&0xFF);
}

/**
  * @brief  
  * @param  
  * @retval 
  */
ITStatus DMA_GetChannelBusyStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    if(((DMAx->CH_BUSY) & (0x01<<DMA_Channelx)) == 0x00)
    {
        return RESET;
    }
    else
    {
        return SET;
    }
}
/**
  * @brief  
  * @param  
  * @retval 
  */
ITStatus DMA_GetChannelEnableStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    if(((DMAx->CHEN) & (0x01<<DMA_Channelx)) == 0x00)
    {
        return RESET;
    }
    else
    {
        return SET;
    }
}
/**
  * @brief  
  * @param  
  * @retval 
  */
ITStatus DMA_GetChannelTCStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    if(((DMAx->TCSR) & (0x01<<DMA_Channelx)) == 0x00)
    {
        return RESET;
    }
    else
    {
        return SET;
    }
}
/**
  * @brief  
  * @param  
  * @retval 
  */
ITStatus DMA_GetChannelAbtStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    if(((DMAx->INT_ERR_ABT_SR) & (0x01<<(DMA_Channelx+16))) == 0x00)
    {
        return RESET;
    }
    else
    {
        return SET;
    }
}
/**
  * @brief  
  * @param  
  * @retval 
  */
ITStatus DMA_GetChannelErrStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    if(((DMAx->INT_ERR_ABT_SR) & (0x01<<(DMA_Channelx))) == 0x00)
    {
        return RESET;
    }
    else
    {
        return SET;
    }
}
/**
  * @brief  
  * @param  
  * @retval 
  */
uint32_t DMA_GetAllChannelAllStatus(DMA_TypeDef* DMAx)
{
    uint32_t Status;
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    //
    Status = ((DMAx->TCSR)&0xFF)<<8;
    Status |= ((DMAx->INT_ERR_ABT_SR)&(~(0xFFUL<<8)));
    return Status;
}
/**
  * @brief  
  * @param  
  * @retval 
  */
void DMA_AbtChannel(DMA_TypeDef* DMAx, uint32_t DMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_DMA_PERIPH(DMAx));
    assert_param(IS_DMA_CHANNEL(DMA_Channelx));
    //
    DMA_Channel_TypeDef * DmaChl = (DMA_Channel_TypeDef*)(((uint32_t)(&(DMAx->C0_CSR))) + (DMA_Channelx*0x20));
    DmaChl->CSR |= (0x01UL<<15);
}








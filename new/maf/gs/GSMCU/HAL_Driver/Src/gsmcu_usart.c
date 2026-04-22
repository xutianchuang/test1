#include  <gsmcu_hal.h>

/**
  * @brief  Initializes the USARTx peripheral according to the specified
  *         parameters in the USART_InitStruct .
  * @param  USARTx: Select the USART or the UART peripheral. 
  *   This parameter can be one of the following values:
  *   USART1, USART2, USART3, UART4 or UART5.
  * @param  USART_InitStruct: pointer to a USART_InitTypeDef structure
  *         that contains the configuration information for the specified USART 
  *         peripheral.
  * @retval None
  */
void USART_Init(USART_TypeDef* USARTx, USART_InitTypeDef* USART_InitStruct)
{
    uint32_t BaudRate,BaudRatePsr;
    uint32_t i;
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_BAUDRATE(USART_InitStruct->USART_BaudRate));  
    assert_param(IS_USART_WORDLEN_STOPBITS(USART_InitStruct->USART_WordLength_StopBits));
    assert_param(IS_USART_PARITY(USART_InitStruct->USART_Parity));
    assert_param(IS_USART_HARDWARE_FLOW_CONTROL(USART_InitStruct->USART_HardwareFlowControl));
    assert_param(IS_FUNCTIONAL_STATE(USART_InitStruct->USART_DMA_Flag));
    //-----------------------------------
    //input enable
    if(USARTx == UART0)
    {
        SCU->EX_PERIPH_INPUT_ENABLE |= (0x01UL<<2);
		SCU->EX_PERIPH_OTHER |= (1<<1);//change clock to 133mhz
        SCU_GPIOInputConfig(UART0_TX_PORT, UART0_TX_PIN);
		SCU_GPIOInputConfig(UART0_RX_PORT, UART0_RX_PIN);
    }
    else if(USARTx == UART1)
    {
        SCU->EX_PERIPH_INPUT_ENABLE |= (0x01UL<<3);
		SCU->EX_PERIPH_OTHER |= (1<<2);//change clock to 133mhz
        SCU_GPIOInputConfig(UART1_TX_PORT, UART1_TX_PIN);
		SCU_GPIOInputConfig(UART1_RX_PORT, UART1_RX_PIN);
    }
    else if(USARTx == UART2)
    {
        SCU->EX_PERIPH_INPUT_ENABLE |= (0x01UL<<4);
		SCU->EX_PERIPH_OTHER |= (1<<3);//change clock to 133mhz
        SCU_GPIOInputConfig(UART2_TX_PORT, UART2_TX_PIN);
		SCU_GPIOInputConfig(UART2_RX_PORT, UART2_RX_PIN);
    }
    else
    {
        SCU->EX_PERIPH_INPUT_ENABLE |= (0x01UL<<11);
		SCU->EX_PERIPH_OTHER |= (1<<6);//change clock to 133mhz
        SCU_GPIOInputConfig(UART3_TX_PORT, UART3_TX_PIN);
		SCU_GPIOInputConfig(UART3_RX_PORT, UART3_RX_PIN);
    }
    //===========
    USARTx->IIR_FCR_PSR = 0x00;
    //8e1
    if(USART_InitStruct->USART_DMA_Flag == ENABLE)
    {
        USARTx->MDR = 0x10;//usart mode enable dma
    }
    else
    {
        USARTx->MDR = 0x00;//usart mode
    }
    USARTx->LCR = (0x01UL<<7)|(USART_InitStruct->USART_WordLength_StopBits)
                             |(USART_InitStruct->USART_Parity);
    //DLL DLM RATE
    BaudRate = USART_SOURCE_CLK/USART_InitStruct->USART_BaudRate;
    BaudRatePsr = 1;
    for(i=0;i<0x1F;i++)
    {
        if((BaudRate&0xFFFF0000) != 0)
        {
            BaudRatePsr++;
            BaudRate = BaudRate>>1;
        }
        else
        {
            break;
        }
    }
    //DLAB = 1
    USARTx->RTBR_DLL = BaudRate&0xFF;
    USARTx->IER_DLM = (BaudRate>>8)&0xFF;
    USARTx->IIR_FCR_PSR = ((BaudRatePsr)&0x1F);
    //DLAB Clear
    USARTx->LCR &= ~(0x01UL<<7);
    //
    USARTx->IER_DLM = (USART_InitStruct->USART_HardwareFlowControl)&0xFF;
    USARTx->IIR_FCR_PSR = (USART_InitStruct->USART_Fifo_Config)&0xFF;
}
/**
  * @brief  
  * @param  
  * @retval 
  */
void USART_ConfigInterruptEnable(USART_TypeDef* USARTx,uint16_t ievalue)
{
    uint32_t currentievalue;
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_INTERRUPT_CONTROL(ievalue));
    //DLAB Clear
    USARTx->LCR &= ~(0x01UL<<7);
    //
    currentievalue = USARTx->IER_DLM;
    USARTx->IER_DLM = (currentievalue&(~0x0FUL))|ievalue;
}

/**
  * @brief  
  * @param  
  * @retval 
  */
uint8_t USART_GetFifoDepth(USART_TypeDef* USARTx)
{
    uint8_t fifodepth = 0;
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    
    switch((USARTx->FEATURE)&0x0F)
    {
        case 0x01:
            fifodepth = 16;
        break;
        case 0x02:
            fifodepth = 32;
        break;
        case 0x04:
            fifodepth = 64;
        break;
        case 0x08:
            fifodepth = 128;
        break;
        default:  
        break;
    }
    return fifodepth;
}
/**
  * @brief  
  * @param  
  * @retval 
  */
void USART_FifoConfig(USART_TypeDef* USARTx,uint16_t fifovalue)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_FIFO_CONFIG(fifovalue));
    //
    USARTx->LCR &= ~(0x01UL<<7);
    //
    USARTx->IIR_FCR_PSR |= fifovalue;
}
/**
  * @brief  Transmits single data through the USARTx peripheral.
  * @param  USARTx: Select the USART or the UART peripheral. 
  *   This parameter can be one of the following values:
  *   USART1, USART2, USART3, UART4 or UART5.
  * @param  Data: the data to transmit.
  * @retval None
  */
void USART_SendData(USART_TypeDef* USARTx, uint8_t Data)
{
  /* Check the parameters */
  assert_param(IS_USART_ALL_PERIPH(USARTx));
  /* Receive Data */
  USARTx->RTBR_DLL = Data;
}


uint32_t GetUartSRAddr(USART_TypeDef* USARTx)
{
  /* Check the parameters */
  assert_param(IS_USART_ALL_PERIPH(USARTx));
  /* Receive Data */
  return (uint32_t)(&(USARTx->RTBR_DLL));
}
int GetUartDmaSrc(USART_TypeDef* USARTx,uint32_t *Tx_chanel,uint32_t*Rx_chanel)
{
    switch ((int)USARTx)
    {
    case UART0_BASE:
        if (Tx_chanel)
        {
            *Tx_chanel = DMA_UART0_TX;
        }
        if (Rx_chanel)
        {
            *Rx_chanel = DMA_UART0_RX;
        }
        break;
    case UART1_BASE:
        if (Tx_chanel)
        {
            *Tx_chanel = DMA_UART1_TX;
        }
        if (Rx_chanel)
        {
            *Rx_chanel = DMA_UART1_RX;
        }
        break;
    case UART2_BASE:
        if (Tx_chanel)
        {
            *Tx_chanel = DMA_UART2_TX;
        }
        if (Rx_chanel)
        {
            *Rx_chanel = DMA_UART2_RX;
        }
        break;
    case UART3_BASE:
        if (Tx_chanel)
        {
            *Tx_chanel = DMA_UART3_TX;
        }
        if (Rx_chanel)
        {
            *Rx_chanel = DMA_UART3_RX;
        }
        break;
    default:
        return -1;
    }
    return 0;
}

/**
  * @brief  Returns the most recent received data by the USARTx peripheral.
  * @param  USARTx: Select the USART or the UART peripheral. 
  *   This parameter can be one of the following values:
  *   USART1, USART2, USART3, UART4 or UART5.
  * @retval The received data.
  */
__inline uint8_t USART_ReceiveData(USART_TypeDef* USARTx)
{
  /* Check the parameters */
  assert_param(IS_USART_ALL_PERIPH(USARTx));
  /* Receive Data */
  return (uint8_t)(USARTx->RTBR_DLL & 0x00FF);
}
/**
  * @brief  
  * @param  
  * @retval 
  */
ITStatus USART_GetITStatus(USART_TypeDef* USARTx, uint8_t USART_IT)
{
  ITStatus bitstatus = RESET;
  /* Check the parameters */
  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_IT_STATUS(USART_IT));
  //Get IT  
  if ((USARTx->IIR_FCR_PSR&0x0F) == USART_IT)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}
/**
  * @brief  
  * @param  
  * @retval 
  */
uint8_t USART_GetITAllStatus(USART_TypeDef* USARTx)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    //
    return (USARTx->IIR_FCR_PSR&0x0F);
}
/**
  * @brief  
  * @param  
  * @retval 
  */
FlagStatus USART_GetStatus(USART_TypeDef* USARTx, uint8_t USART_STATUS)
{
  ITStatus bitstatus = RESET;
  /* Check the parameters */
  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_STATUS(USART_STATUS));
  //Get IT
  if ((USARTx->LSR_TST&USART_STATUS) == USART_STATUS)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}

/**
  * @brief  
  * @param  
  * @retval 
  */
uint8_t USART_GetAllStatus(USART_TypeDef* USARTx)
{
  /* Check the parameters */
  assert_param(IS_USART_ALL_PERIPH(USARTx));
  return (USARTx->LSR_TST&0xFF);
}

/**
  * @brief  
  * @param  
  * @retval 
  */
uint8_t USART_GetRxFifoCount(USART_TypeDef* USARTx)
{
  /* Check the parameters */
  assert_param(IS_USART_ALL_PERIPH(USARTx));
  return (USARTx->RXFF_CNTR&0xFF);
}















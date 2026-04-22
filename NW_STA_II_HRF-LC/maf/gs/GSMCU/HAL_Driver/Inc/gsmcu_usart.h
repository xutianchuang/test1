#ifndef __GSMCU_USART_H__
#define __GSMCU_USART_H__
#ifdef __cplusplus
extern "C"
{
#endif




#ifndef __inline
#define __inline
#endif

/** 
  * @brief  USART Clk
  */ 
#define USART_SOURCE_CLK        (133333333/16)

/** 
  * @brief  USART Init Structure definition  
  */ 
  
typedef struct
{
  uint32_t USART_BaudRate;
  uint16_t USART_WordLength_StopBits;
  uint16_t USART_Parity;
  uint16_t USART_HardwareFlowControl;
  uint16_t USART_Fifo_Config;
  uint16_t USART_DMA_Flag;
} USART_InitTypeDef;

/** @defgroup USART_Exported_Constants
  * @{
  */ 
  
#define IS_USART_ALL_PERIPH(PERIPH) (((PERIPH) == UART0) || \
                                     ((PERIPH) == UART1) || \
                                     ((PERIPH) == UART2) || \
                                     ((PERIPH) == UART3))

/** @defgroup USART_Stop_Bits 
  * @{
  */ 
  
#define USART_5Bit_StopBits_1                     ((uint32_t)0x0000)
#define USART_6Bit_StopBits_1                     ((uint32_t)0x0001)
#define USART_7Bit_StopBits_1                     ((uint32_t)0x0002)
#define USART_8Bit_StopBits_1                     ((uint32_t)0x0003)
#define USART_5Bit_StopBits_1_5                   ((uint32_t)0x0004)
#define USART_6Bit_StopBits_2                     ((uint32_t)0x0005)
#define USART_7Bit_StopBits_2                     ((uint32_t)0x0006)
#define USART_8Bit_StopBits_2                     ((uint32_t)0x0007)

#define IS_USART_WORDLEN_STOPBITS(BITS) (((BITS) == USART_5Bit_StopBits_1) || \
                                     ((BITS) == USART_6Bit_StopBits_1) || \
                                     ((BITS) == USART_7Bit_StopBits_1) || \
                                     ((BITS) == USART_8Bit_StopBits_1) || \
                                     ((BITS) == USART_5Bit_StopBits_1_5) || \
                                     ((BITS) == USART_6Bit_StopBits_2) || \
                                     ((BITS) == USART_7Bit_StopBits_2) || \
                                     ((BITS) == USART_8Bit_StopBits_2))



/** @defgroup USART_Parity 
  * @{
  */ 
  
#define USART_Parity_No                      ((uint32_t)0x0000<<3)
#define USART_Parity_Even                    ((uint32_t)0x0003<<3)
#define USART_Parity_Odd                     ((uint32_t)0x0001<<3)
#define USART_Parity_One                     ((uint32_t)0x0005<<3)
#define USART_Parity_Zero                    ((uint32_t)0x0007<<3)
#define IS_USART_PARITY(PARITY) (((PARITY) == USART_Parity_No) || \
                                 ((PARITY) == USART_Parity_Even) || \
                                 ((PARITY) == USART_Parity_Odd)|| \
                                 ((PARITY) == USART_Parity_One)|| \
                                 ((PARITY) == USART_Parity_Zero))
/**
  * @}
  */ 
/** @defgroup USART_Parity 
  * @{
  */ 
//1
#define USART_RXFIFO_THR_LEVEL_0                     ((uint16_t)0x00<<6)
#define USART_RXFIFO_THR_LEVEL_1                     ((uint16_t)0x01<<6)
#define USART_RXFIFO_THR_LEVEL_2                     ((uint16_t)0x02<<6)
#define USART_RXFIFO_THR_LEVEL_3                     ((uint16_t)0x03<<6)
//2
#define USART_TXFIFO_THR_LEVEL_0                     ((uint16_t)0x00<<4)
#define USART_TXFIFO_THR_LEVEL_1                     ((uint16_t)0x01<<4)
#define USART_TXFIFO_THR_LEVEL_2                     ((uint16_t)0x02<<4)
#define USART_TXFIFO_THR_LEVEL_3                     ((uint16_t)0x03<<4)
//3
#define USART_RXFIFO_CLEAR                           ((uint16_t)0x01<<1)
#define USART_TXFIFO_CLEAR                           ((uint16_t)0x01<<2)
//4
#define USART_FIFO_ENABLE                            ((uint16_t)0x01<<0)
//
#define USART_FIFO_CONFIG_NONE                        ((uint16_t)0x00)
//
#define IS_USART_FIFO_CONFIG(CVALUE)            (((CVALUE)&0x08) == 0x00)
//


/**
  * @}
  */ 



/** @defgroup USART_Hardware_Flow_Control 
  * @{
  */ 
#define USART_HardwareFlowControl_None       ((uint32_t)0x0000)
#define USART_HardwareFlowControl_RTS        ((uint32_t)0x0010)
#define USART_HardwareFlowControl_CTS        ((uint32_t)0x0020)
#define USART_HardwareFlowControl_RTS_CTS    ((uint32_t)0x0030)
#define IS_USART_HARDWARE_FLOW_CONTROL(CONTROL)\
                              (((CONTROL) == USART_HardwareFlowControl_None) || \
                               ((CONTROL) == USART_HardwareFlowControl_RTS) || \
                               ((CONTROL) == USART_HardwareFlowControl_CTS) || \
                               ((CONTROL) == USART_HardwareFlowControl_RTS_CTS))
/**
  * @}
  */ 
/** @defgroup USART_Interrupt_Enable 
  * @{
  */ 
#define USART_MODEM_STATUS_INTER_ENABLE         ((uint32_t)0x0008)
#define USART_RECEIVE_LINE_INTER_ENABLE         ((uint32_t)0x0004)
#define USART_TRANSMITTER_DATA_INTER_ENABLE     ((uint32_t)0x0002)
#define USART_RECEIVE_DATA_INTER_ENABLE         ((uint32_t)0x0001)
#define IS_USART_INTERRUPT_CONTROL(CONTROL)     (((CONTROL)&0x0F) != 0)
/**
  * @}
  */
/** @defgroup USART_Interrupt_status
  * @{
  */ 
#define USART_RECEIVE_LINE_INTER_STATUS             ((uint32_t)0x0006)
#define USART_RECEIVE_DATA_READY_INTER_STATUS       ((uint32_t)0x0004)
#define USART_RECEIVE_DATA_TIMEOUT_INTER_STATUS     ((uint32_t)0x000C)
#define USART_TRANSMITTER_DATA_EMPTY_INTER_STATUS   ((uint32_t)0x0002)
#define USART_MODEM_STATUS_INTER_STATUS             ((uint32_t)0x0000)

#define IS_USART_IT_STATUS(STATUS)      (((STATUS) == USART_RECEIVE_LINE_INTER_STATUS) || \
                                        ((STATUS) == USART_RECEIVE_DATA_READY_INTER_STATUS) || \
                                        ((STATUS) == USART_RECEIVE_DATA_TIMEOUT_INTER_STATUS) || \
                                        ((STATUS) == USART_TRANSMITTER_DATA_EMPTY_INTER_STATUS)|| \
                                        ((STATUS) == USART_MODEM_STATUS_INTER_STATUS))
/**
  * @}
  */
#define IS_USART_BAUDRATE(BAUDRATE) (((BAUDRATE) > 0) && ((BAUDRATE) < (USART_SOURCE_CLK)))
/**
  * @}
  */
#define USART_FIFO_DATA_ERROR_STATUS                ((uint32_t)0x0080)
#define USART_TRANSMITTER_EMPTY_STATUS              ((uint32_t)0x0040)
#define USART_THR_EMPTY_STATUS                      ((uint32_t)0x0020)
#define USART_BREAK_INTERRUPT_STATUS                ((uint32_t)0x0010)
#define USART_FRAMING_ERROR_STATUS                  ((uint32_t)0x0008)
#define USART_PARITY_ERROR_STATUS                   ((uint32_t)0x0004)
#define USART_OVERRUN_ERROR_STATUS                  ((uint32_t)0x0002)
#define USART_DATA_READY_STATUS                     ((uint32_t)0x0001)


#define IS_USART_STATUS(STATUS)      (((STATUS) == USART_FIFO_DATA_ERROR_STATUS) || \
                                        ((STATUS) == USART_TRANSMITTER_EMPTY_STATUS) || \
                                        ((STATUS) == USART_THR_EMPTY_STATUS) || \
                                        ((STATUS) == USART_BREAK_INTERRUPT_STATUS)|| \
                                        ((STATUS) == USART_FRAMING_ERROR_STATUS)|| \
                                        ((STATUS) == USART_PARITY_ERROR_STATUS)|| \
                                        ((STATUS) == USART_OVERRUN_ERROR_STATUS)|| \
                                        ((STATUS) == USART_DATA_READY_STATUS))

/**
  * @}
  */
void USART_Init(USART_TypeDef* USARTx, USART_InitTypeDef* USART_InitStruct);
void USART_ConfigInterruptEnable(USART_TypeDef* USARTx,uint16_t ievalue);
uint8_t USART_GetFifoDepth(USART_TypeDef* USARTx);
void USART_FifoConfig(USART_TypeDef* USARTx,uint16_t fifovalue);
void USART_SendData(USART_TypeDef* USARTx, uint8_t Data);
uint8_t USART_ReceiveData(USART_TypeDef* USARTx);
ITStatus USART_GetITStatus(USART_TypeDef* USARTx, uint8_t USART_IT);
uint8_t USART_GetITAllStatus(USART_TypeDef* USARTx);
FlagStatus USART_GetStatus(USART_TypeDef* USARTx, uint8_t USART_STATUS);
uint8_t USART_GetAllStatus(USART_TypeDef* USARTx);
uint32_t GetUartSRAddr(USART_TypeDef* USARTx);
uint8_t USART_GetRxFifoCount(USART_TypeDef* USARTx);
int GetUartDmaSrc(USART_TypeDef* USARTx,uint32_t *Tx_chanel,uint32_t*Rx_chanel);



#ifdef __cplusplus
}
#endif
#endif /* __GSMCU_USART_H__ */


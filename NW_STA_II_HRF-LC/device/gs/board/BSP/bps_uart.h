/*
*
*/
#ifndef __BSP_UART_H__
#define __BSP_UART_H__
#include <gsmcuxx_hal_def.h>
#include "gsmcu_hal.h"
//=======
#define Stop_Bits_One                (USART_8Bit_StopBits_1)
#define Stop_Bits_Two                (USART_8Bit_StopBits_2)

#define Parity_No                      (USART_Parity_No)
#define Parity_Even                    (USART_Parity_Even)
#define Parity_Odd                     (USART_Parity_Odd)


//=======
typedef void (*UartCallBackFunctionType)();

typedef struct
{
    uint32_t BaudRate;
    uint32_t Parity;
    uint32_t DataBits;
    uint32_t StopBits;
    //
    UartCallBackFunctionType RxHaveDataFunction;
    UartCallBackFunctionType RxParityErrorFunction;
    UartCallBackFunctionType TxDataCompleteFunction;
}UartParameterType;


void UartOpen(UartParameterType *UartParameter);
void UartWrite(uint8_t *pbuff,uint32_t len);
uint32_t UartRead(uint8_t *pbuff,uint32_t len);
uint32_t GetUartRxDataLen();
void UartClose();
bool IsUartTxData();
void UartWriteWithDma(USART_TypeDef *puart,uint8_t *pbuff,uint32_t len,uint32_t dma_channel);
bool IsUartTxSendAllDone(void);
#if defined(METER_UART_USE_UART0)
	#define METER_UART						UART0
	#define METER_UART_TX_PIN_REMAP			UART0_TX_REMAP
	#define METER_UART_RX_PIN_REMAP			UART0_RX_REMAP
	#define METER_UART_INPUT_ENABLE			UART0_INPUT_ENABLE
	#define METER_UART_TX_PORT				UART0_TX_PORT
	#define METER_UART_TX_PIN				UART0_TX_PIN
	#define METER_UART_RX_PORT				UART0_RX_PORT
	#define METER_UART_RX_PIN				UART0_RX_PIN
	#define METER_UART_IRQn					UART0_IRQn
	#define METER_UART_DMA_REQ				DMA_UART0_TX
#elif defined(METER_UART_USE_UART1)
	#define METER_UART						UART1
	#define METER_UART_TX_PIN_REMAP			UART1_TX_REMAP
	#define METER_UART_RX_PIN_REMAP			UART1_RX_REMAP
	#define METER_UART_INPUT_ENABLE			UART1_INPUT_ENABLE
	#define METER_UART_TX_PORT				UART1_TX_PORT
	#define METER_UART_TX_PIN				UART1_TX_PIN
	#define METER_UART_RX_PORT				UART1_RX_PORT
	#define METER_UART_RX_PIN				UART1_RX_PIN
	#define METER_UART_IRQn					UART1_IRQn
	#define METER_UART_DMA_REQ				DMA_UART1_TX
#elif defined(METER_UART_USE_UART2)
	#define METER_UART						UART2
	#define METER_UART_TX_PIN_REMAP			UART2_TX_REMAP
	#define METER_UART_RX_PIN_REMAP			UART2_RX_REMAP
	#define METER_UART_INPUT_ENABLE			UART2_INPUT_ENABLE
	#define METER_UART_TX_PORT				UART2_TX_PORT
	#define METER_UART_TX_PIN				UART2_TX_PIN
	#define METER_UART_RX_PORT				UART2_RX_PORT
	#define METER_UART_RX_PIN				UART2_RX_PIN
	#define METER_UART_IRQn					UART2_IRQn
	#define METER_UART_DMA_REQ				DMA_UART2_TX
#elif defined(METER_UART_USE_UART3)
	#define METER_UART						UART3
	#define METER_UART_TX_PIN_REMAP			UART3_TX_REMAP
	#define METER_UART_RX_PIN_REMAP			UART3_RX_REMAP
	#define METER_UART_INPUT_ENABLE			UART3_INPUT_ENABLE
	#define METER_UART_TX_PORT				UART3_TX_PORT
	#define METER_UART_TX_PIN				UART3_TX_PIN
	#define METER_UART_RX_PORT				UART3_RX_PORT
	#define METER_UART_RX_PIN				UART3_RX_PIN
	#define METER_UART_IRQn					UART3_IRQn
	#define METER_UART_DMA_REQ				DMA_UART3_TX
#endif



#define BPS_UART_OPEN                  (1)
#define BPS_UART_OPEN_MASK             (1)

#define BPS_UART_HAVEDATA               (0x01 <<4)
#define BPS_UART_HAVEDATA_MASK          (0x01 <<4)

#define BPS_UART_TRANSMITING            (0x01 <<8)
#define BPS_UART_TRANSMITMASK           (0x01 <<8)


uint32_t IncreaseIndex(uint32_t idx,uint32_t buffsize);
bool IsFifoEmpty(uint32_t * ri,uint32_t * wi);
bool IsFifoFull(uint32_t * ri,uint32_t * wi,uint32_t buffsize);
void InitFifo(uint32_t * ri,uint32_t * wi);
bool PushFifo(uint32_t * ri,uint32_t * wi,uint8_t *pbuff,uint32_t buffsize,uint8_t dv);
bool PopFifo(uint32_t * ri,uint32_t * wi,uint8_t *pbuff,uint32_t buffsize,uint8_t *pdv);
uint32_t GetFifoDataLen(uint32_t ri,uint32_t wi,uint32_t buffsize);
uint32_t PopSomeOfFifo(uint32_t * ri,uint32_t * wi,uint8_t *pbuff,uint32_t buffsize,uint8_t *pdst,uint32_t poplen);
void UartIrOpen(UartParameterType *UartParameter);
bool UartIrTest(const u8* mac);
#endif





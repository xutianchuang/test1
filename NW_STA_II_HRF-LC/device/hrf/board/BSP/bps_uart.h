/*
*
*/
#ifndef __BSP_UART_H__
#define __BSP_UART_H__
#include <gsmcuxx_hal_def.h>
#include "gsmcu_m3_usart.h"
//=======
#define Parity_No                      (ARM_USART_PARITY_NONE)
#define Parity_Even                    (ARM_USART_PARITY_EVEN)
#define Parity_Odd                     (ARM_USART_PARITY_ODD)


//=======
typedef void (*UartCallBackFunctionType)(int);

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

bool IsUartTxSendAllDone(void);

void UartClose();
bool IsUartTxData();
bool IsUartRxing(void);

#define BPS_UART_OPEN                  (1)
#define BPS_UART_OPEN_MASK             (1)

#define BPS_UART_HAVEDATA               (0x01 <<4)
#define BPS_UART_HAVEDATA_MASK          (0x01 <<4)

#define BPS_UART_TRANSMITING            (0x01 <<8)
#define BPS_UART_TRANSMITMASK           (0x01 <<8)

int getFifoCnt(uint32_t * ri,uint32_t * wi,uint32_t buffsize);
bool pushFifo(uint32_t * ri,uint32_t * wi,uint8_t *pbuff,uint32_t buffsize,uint8_t *data,uint32_t len);
uint32_t popFifo(uint32_t * ri,uint32_t * wi,uint8_t *pbuff,uint32_t buffsize,uint8_t *pdst,uint32_t poplen);

#endif





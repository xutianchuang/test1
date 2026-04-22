/*
*
*/
#ifndef __BSP_UART_H__
#define __BSP_UART_H__
#include <gsmcuxx_hal_def.h>
#include "gsmcu_m3_usart.h"
//=======
#define Stop_Bits_One                (ARM_USART_DATA_BITS_8)
#define Stop_Bits_Two                (ARM_USART_DATA_BITS_9)

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
    //uint32_t StopBits;
    //
    UartCallBackFunctionType RxHaveDataFunction;
    UartCallBackFunctionType RxParityErrorFunction;
    UartCallBackFunctionType TxDataCompleteFunction;
}UartParameterType;


void UartOpen(UartParameterType *UartParameter);
void UartWrite(uint8_t *pbuff,uint32_t len);
uint32_t UartRead(uint8_t *pbuff,uint32_t len);

void UartClose();
bool IsUartTxData();
bool IsUartRxing(void);


#endif





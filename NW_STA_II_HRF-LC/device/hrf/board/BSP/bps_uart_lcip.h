#ifndef __BSP_UART_LCIP_H__
#define __BSP_UART_LCIP_H__
// #include <gsmcuxx_hal_def.h>
// #include "gsmcu_m3_usart.h"

#include "stream_buffer.h"

bool LcipIsUartOpen();
void LcipUartOpen(UartParameterType *UartParameter);
void LcipUartWrite(uint8_t *pbuff,uint32_t len);

bool LcipIsUartTxSendAllDone(void);

void LcipUartClose();
bool LcipIsUartTxData();

extern stream_buffer_t uart_rxstream;


#endif





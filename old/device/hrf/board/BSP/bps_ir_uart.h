#ifndef __BPS_IR_UART_H__
#define __BPS_IR_UART_H__
#ifdef __cplusplus
extern "C"
{
#endif
#include <gsmcuxx_hal_def.h>
#include "bps_uart.h"

void UartIrOpen(UartParameterType *UartParameter);
bool IsIrUartTxData(void);
bool IsIrUartTxSendAllDone(void);
uint32_t UartIrRead(uint8_t *pbuff, uint32_t len);
void UartIrFlushBuffer(void);
bool UartIrTest(const u8* mac);
void UartIrWrite(uint8_t *pbuff,uint32_t len, bool allow_receive);

#ifdef __cplusplus
}
#endif
#endif /* __BPS_IR_UART_H__ */


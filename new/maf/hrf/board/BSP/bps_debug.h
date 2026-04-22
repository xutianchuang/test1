/*
*
*/
#ifndef __BSP_DEBUG_H__
#define __BSP_DEBUG_H__
#include "stdio.h"
#define DEBUG_MASK_BIT                 (DEBUG_LOG_PHY|DEBUG_LOG_MAC|DEBUG_LOG_APS|DEBUG_LOG_APP|DEBUG_LOG_UPDATA|DEBUG_LOG_NET|DEBUG_LOG_INFO|DEBUG_LOG_ERR|DEBUG_LOG_OTHER) 
typedef void (*DebugUartRxDataCallBackFunction)(u8 *,u16 );
void DebugOpen(uint32_t BaudRate,UartCallBackFunctionType txCallback,DebugUartRxDataCallBackFunction rxCallback);
void debug_hex(u32 mask,uint8_t * buf, uint16_t len);
void debug_str(u32 mask,const char *format, ...);
void DebugSetMask(uint32_t mask);
u32 DebugGetMask(void);
u32 DebugGetMaskLater(void);
u32 DebugGetMaskLen(void);
void Debug_AutoLog(void);
#define DEBUG_LOG_PHY                   0x00000001
#define DEBUG_LOG_MAC                   0x00000002
#define DEBUG_LOG_APS                   0x00000004
#define DEBUG_LOG_APP                   0x00000008
#define DEBUG_LOG_UPDATA                0x00000010
#define DEBUG_LOG_NET                   0x00000020
#define DEBUG_LOG_RATE                  0x00000040
#define DEBUG_LOG_CSMA                  0x00000080

#define DEBUG_LOG_INFO                  0x01000000
#define DEBUG_LOG_ERR                   0x02000000
#define DEBUG_LOG_OTHER                 0x04000000

#define DEBUG_LOG_3762                  0x40000000
#define DEBUG_LOG_ALLOC                 0x80000000

#endif

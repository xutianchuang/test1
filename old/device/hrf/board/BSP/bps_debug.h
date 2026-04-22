/*
*
*/
#ifndef __BSP_DEBUG_H__
#define __BSP_DEBUG_H__
#include "stdio.h"

#define DEBUG_WDT_RO         5
#define DEBUG_WDT_WO         6
#define DEBUG_WDT_AC         7
#define DEBUG_WDT_SIZE_BYTE  0
#define DEBUG_WDT_SIZE_HALF  1
#define DEBUG_WDT_SIZE_WORD  2


void DebugOpen(uint32_t BaudRate);
void debug_hex(u32 mask,uint8_t * buf, uint16_t len);
void debug_str(u32 mask,const char *format, ...);
void DebugSetMask(uint32_t mask);
void debug_wdt_init(void);
int debug_at_addr(void *addr, u32 size, u8 mode);
void debug_no_addr(void *addr);
int debug_data_match(void *addr, u32 size, u8 mode,u8 match_size,u32 vaile);
void debug_no_data(void);

#define DEBUG_LOG_PHY                   0x00000001
#define DEBUG_LOG_MAC                   0x00000002
#define DEBUG_LOG_APS                   0x00000004
#define DEBUG_LOG_APP                   0x00000008
#define DEBUG_LOG_UPDATA                0x00000010
#define DEBUG_LOG_NET                   0x00000020
#define DEBUG_LOG_TEMP                  0x00000040
#define DEBUG_LOG_CSMA                  0x00000080
#define DEBUG_LOG_ZONE                  0x00000100
#define DEBUG_LOG_ROUT                  0x00000200

#define DEBUG_LOG_INFO                  0x01000000
#define DEBUG_LOG_ERR                   0x02000000
#define DEBUG_LOG_METER                 0x04000000
#define DEBUG_LOG_OTHER                 0x80000000

#endif

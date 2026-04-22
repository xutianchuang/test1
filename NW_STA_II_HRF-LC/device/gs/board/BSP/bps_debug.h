/*
*
*/
#ifndef __BSP_DEBUG_H__
#define __BSP_DEBUG_H__
#include "stdio.h"

void DebugOpen(uint32_t BaudRate);
void debug_hex(u32 mask,uint8_t * buf, uint16_t len);
void debug_str(u32 mask,const char *format, ...);
void DebugSetMask(uint32_t mask);
#define DEBUG_LOG_PHY                   0x00000001
#define DEBUG_LOG_MAC                   0x00000002
#define DEBUG_LOG_APS                   0x00000004
#define DEBUG_LOG_APP                   0x00000008
#define DEBUG_LOG_UPDATA                0x00000010
#define DEBUG_LOG_NET                   0x00000020
#define DEBUG_LOG_ZONE                  0x00000040
#define DEBUG_LOG_ROUT                  0x00000080

#define DEBUG_LOG_INFO                  0x01000000
#define DEBUG_LOG_ERR                   0x02000000
#define DEBUG_LOG_OTHER                 0x80000000

#if defined(DEBUG_PORT_USE_UART0)
	#define DEBUG_UART						UART0
	#define DEBUG_UART_TX_PIN_REMAP			UART0_TX_REMAP
	#define DEBUG_UART_RX_PIN_REMAP			UART0_RX_REMAP
	#define DEBUG_UART_INPUT_ENABLE			UART0_INPUT_ENABLE
	#define DEBUG_UART_TX_PORT				UART0_TX_PORT
	#define DEBUG_UART_TX_PIN				UART0_TX_PIN
	#define DEBUG_UART_RX_PORT				UART0_RX_PORT
	#define DEBUG_UART_RX_PIN				UART0_RX_PIN
	#define DEBUG_UART_IRQn					UART0_IRQn
	//#define DEBUG_UART_DMA_REQ				DMA_UART0_TX
#elif defined(DEBUG_PORT_USE_UART1)
	#define DEBUG_UART						UART1
	#define DEBUG_UART_TX_PIN_REMAP			UART1_TX_REMAP
	#define DEBUG_UART_RX_PIN_REMAP			UART1_RX_REMAP
	#define DEBUG_UART_INPUT_ENABLE			UART1_INPUT_ENABLE
	#define DEBUG_UART_TX_PORT				UART1_TX_PORT
	#define DEBUG_UART_TX_PIN				UART1_TX_PIN
	#define DEBUG_UART_RX_PORT				UART1_RX_PORT
	#define DEBUG_UART_RX_PIN				UART1_RX_PIN
	#define DEBUG_UART_IRQn					UART1_IRQn
	//#define DEBUG_UART_DMA_REQ				DMA_UART1_TX
#elif defined(DEBUG_PORT_USE_UART2)
	#define DEBUG_UART						UART2
	#define DEBUG_UART_TX_PIN_REMAP			UART2_TX_REMAP
	#define DEBUG_UART_RX_PIN_REMAP			UART2_RX_REMAP
	#define DEBUG_UART_INPUT_ENABLE			UART2_INPUT_ENABLE
	#define DEBUG_UART_TX_PORT				UART2_TX_PORT
	#define DEBUG_UART_TX_PIN				UART2_TX_PIN
	#define DEBUG_UART_RX_PORT				UART2_RX_PORT
	#define DEBUG_UART_RX_PIN				UART2_RX_PIN
	#define DEBUG_UART_IRQn					UART2_IRQn
	//#define DEBUG_UART_DMA_REQ				DMA_UART2_TX
#elif defined(DEBUG_PORT_USE_UART3)
	#define DEBUG_UART						UART3
	#define DEBUG_UART_TX_PIN_REMAP			UART3_TX_REMAP
	#define DEBUG_UART_RX_PIN_REMAP			UART3_RX_REMAP
	#define DEBUG_UART_INPUT_ENABLE			UART3_INPUT_ENABLE
	#define DEBUG_UART_TX_PORT				UART3_TX_PORT
	#define DEBUG_UART_TX_PIN				UART3_TX_PIN
	#define DEBUG_UART_RX_PORT				UART3_RX_PORT
	#define DEBUG_UART_RX_PIN				UART3_RX_PIN
	#define DEBUG_UART_IRQn					UART3_IRQn
	//#define DEBUG_UART_DMA_REQ				DMA_UART3_TX
#endif
#endif

#ifndef _FINSH_DEVICE_H_
#define _FINSH_DEVICE_H_

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include  <gsmcu_hal.h>

//定义rt_kprintf

#if defined(DEBUG_PORT_USE_UART0)
	#define FINSH_USE_UART0
#elif defined(DEBUG_PORT_USE_UART1)
	#define FINSH_USE_UART1
#elif defined(DEBUG_PORT_USE_UART2)
	#define FINSH_USE_UART2
#elif defined(DEBUG_PORT_USE_UART3)
	#define FINSH_USE_UART3
#endif

#ifdef FINSH_USE_UART0
	#define FINSH_UART					UART0
	#define FINSH_IRQn					UART0_IRQn
	#define FINSH_UART_TX_REMAP			UART0_TX_REMAP
	#define FINSH_UART_RX_REMAP			UART0_RX_REMAP
	#define FINSH_UART_INPUT_ENABLE		UART0_INPUT_ENABLE
	#define FINSH_UART_TX_PORT			UART0_TX_PORT
	#define FINSH_UART_TX_PIN			UART0_TX_PIN
	#define FINSH_UART_RX_PORT			UART0_RX_PORT
	#define FINSH_UART_RX_PIN			UART0_RX_PIN
#elif defined FINSH_USE_UART1
	#define FINSH_UART					UART1
	#define FINSH_IRQn					UART1_IRQn
	#define FINSH_UART_TX_REMAP			UART1_TX_REMAP
	#define FINSH_UART_RX_REMAP			UART1_RX_REMAP
	#define FINSH_UART_INPUT_ENABLE		UART1_INPUT_ENABLE
	#define FINSH_UART_TX_PORT			UART1_TX_PORT
	#define FINSH_UART_TX_PIN			UART1_TX_PIN
	#define FINSH_UART_RX_PORT			UART1_RX_PORT
	#define FINSH_UART_RX_PIN			UART1_RX_PIN
#elif defined FINSH_USE_UART2
	#define FINSH_UART					UART2
	#define FINSH_IRQn					UART2_IRQn
	#define FINSH_UART_TX_REMAP			UART2_TX_REMAP
	#define FINSH_UART_RX_REMAP			UART2_RX_REMAP
	#define FINSH_UART_INPUT_ENABLE		UART2_INPUT_ENABLE
	#define FINSH_UART_TX_PORT			UART2_TX_PORT
	#define FINSH_UART_TX_PIN			UART2_TX_PIN
	#define FINSH_UART_RX_PORT			UART2_RX_PORT
	#define FINSH_UART_RX_PIN			UART2_RX_PIN
#elif defined FINSH_USE_UART3
	#define FINSH_UART					UART3
	#define FINSH_IRQn					UART3_IRQn
	#define FINSH_UART_TX_REMAP			UART3_TX_REMAP
	#define FINSH_UART_RX_REMAP			UART3_RX_REMAP
	#define FINSH_UART_INPUT_ENABLE		UART3_INPUT_ENABLE
	#define FINSH_UART_TX_PORT			UART3_TX_PORT
	#define FINSH_UART_TX_PIN			UART3_TX_PIN
	#define FINSH_UART_RX_PORT			UART3_RX_PORT
	#define FINSH_UART_RX_PIN			UART3_RX_PIN
#endif

//int rt_kprintf();

#define rt_kprintf printf

#define rt_strncpy strncpy

#define rt_memmove	memmove

#define RT_ASSERT	assert

typedef void(*pFinshCharInfoFun)(void);

//打印口初始化
void FinshDeviceInit(u32 baud);

//提供一个获取接口
u8  FinshGetChar(void);

//设置一个数据通知回调函数
void FinshCharCallback(pFinshCharInfoFun fun);

#endif

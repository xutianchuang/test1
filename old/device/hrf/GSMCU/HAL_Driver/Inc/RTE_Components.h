
/*
 * Auto generated Run-Time-Environment Component Configuration File
 *      *** Do not modify ! ***
 *
 * Project: 'g3' 
 * Target:  'Target 1' 
 */

#ifndef RTE_COMPONENTS_H
#define RTE_COMPONENTS_H


/*
 * Define the Device Header File: 
 */

#if defined(ZB204_CHIP)
#define CMSIS_device_header  "ZB204.h"
#elif defined(ZB205_CHIP)
#define CMSIS_device_header  "ZB205.h"
#endif

#define RTE_DEVICE_STARTUP_Meter_M0      /* Device Startup for GSMCU_M0 */
//#include "csmis_config.h"
#define USE_DMA 1
#ifndef RTE_SPI0
#define RTE_SPI0    1    /* SPI Switch */
#endif
//#ifndef RTE_SPI1
//#define RTE_SPI1    1    /* SPI Switch */
//#endif
#ifndef RTE_USART0
#define RTE_USART0 1	 /* USART0 used */
#endif

#if defined(ZB204_CHIP)
    #ifndef RTE_USART1
    #define RTE_USART1 1	 /* USART1 used */
    #endif
#elif defined(ZB205_CHIP)
    #ifndef RTE_USART1
    #define RTE_USART1 1	 /* USART1 used */
    #endif
	#ifndef RTE_USART3
    #define RTE_USART3 1	 /* USART3 used */
    #endif
#endif

#ifndef RTE_USART2
#define RTE_USART2 1	 /* USART2 used */
#endif

#ifndef RTE_PWM0
#define RTE_PWM0 1
#endif


typedef enum {
#if RTE_USART0
	UART0_TX_DMA,
	UART0_RX_DMA,
#endif
#if RTE_USART1
	UART1_TX_DMA,
	UART1_RX_DMA,
#endif
#if RTE_USART2
	UART2_TX_DMA,
	UART2_RX_DMA,
#endif
#if RTE_USART3
	UART3_TX_DMA,
	UART3_RX_DMA,
#endif
#if RTE_SPI0
	SPI0_TX_DMA,
	SPI0_RX_DMA,
#endif
#if RTE_SPI1
	SPI1_TX_DMA,
	SPI1_RX_DMA,
#endif
	DMA_CHANNEL_MAX,
}DMA_Channel_e;

typedef enum {
	DMA_REQ_SPI0_TX,
	DMA_REQ_SPI0_RX,
	DMA_REQ_SPI1_TX,
	DMA_REQ_SPI1_RX,
	DMA_REQ_QSPI_TX,
	DMA_REQ_QSPI_RX,
	DMA_REQ_UART0_TX,
	DMA_REQ_UART0_RX,
	DMA_REQ_UART1_TX,
	DMA_REQ_UART1_RX,
	DMA_REQ_UART2_TX,
	DMA_REQ_UART2_RX,
	DMA_REQ_UART3_TX,
	DMA_REQ_UART3_RX,
}DMA_Req_e;

#if DMA_CHANNEL_MAX>6
#error "DMA channel limit"
#endif
#endif /* RTE_COMPONENTS_H */

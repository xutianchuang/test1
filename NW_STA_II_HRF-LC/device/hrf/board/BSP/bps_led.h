/*
*
*/
#ifndef __BSP_LED_H__
#define __BSP_LED_H__
#include <gsmcuxx_hal_def.h>
#define GREEN_LED           (0)
#define RED_LED             (1)

// 网络状态灯
#define NET_STATUS_LED_GROUP2		2
#define NET_STATUS_LED1_RED	5
#define NET_STATUS_LED2_GREEN	4

#define NET_STATUS_LED3_RED	3
#define NET_STATUS_LED4_GREEN	2


#define NET_STATUS_LED_GROUP4		4
#define NET_STATUS_LED5_RED	4
#define NET_STATUS_LED6_GREEN	3

#define NET_STATUS_LED7_GREEN	2
#define NET_STATUS_LED8_RED		1

typedef enum
{
	SOFEWARE_RUN_START,
	UP_CHANNEL_TYPE,
	LEST_SUCCESS_RATE_BETWEEN_CCO,
	COMMUNICATE_RATE_BETWEEN_PCO,
}ledType;

void LedOpen(void);
void LedWrite(uint32_t led,uint32_t mode);

void NetStatusLedOpen(void);
//void NetStatusLedWrite(uint8_t ledType, uint32_t ledColor, uint32_t onOrOff);

#if DEV_STA
void LedOpen(void);
void LedWrite(uint32_t led,uint32_t mode);
void NetStatusLedHandle(void);

#if defined(GREEN_RX_LED_PORT_USE_PORT0)
#define GREEN_RX_LED_PORT			GPIO0
#elif defined(GREEN_RX_LED_PORT_USE_PORT1)
#define GREEN_RX_LED_PORT			GPIO1
#endif

#if defined(RED_TX_LED_PORT_USE_PORT0)
#define RED_TX_LED_PORT				GPIO0
#elif defined(RED_TX_LED_PORT_USE_PORT1)
#define RED_TX_LED_PORT				GPIO1
#endif

#if defined(GREEN_RX_LED_PORT_3PHASE_USE_PORT0)
#define GREEN_RX_LED_PORT_3PHASE	GPIO0
#elif defined(GREEN_RX_LED_PORT_3PHASE_USE_PORT1)
#define GREEN_RX_LED_PORT_3PHASE	GPIO1
#endif

#if defined(RED_TX_LED_PORT_3PHASE_USE_PORT0)
#define RED_TX_LED_PORT_3PHASE		GPIO0
#elif defined(RED_TX_LED_PORT_3PHASE_USE_PORT1)
#define RED_TX_LED_PORT_3PHASE		GPIO1
#endif

extern const u32 myDevicType;
#elif DEV_CCO
#if defined(GREEN_TX_LED_PORT_USE_PORT0)
#define GREEN_TX_LED_PORT			GPIO0
#elif defined(GREEN_TX_LED_PORT_USE_PORT1)
#define GREEN_TX_LED_PORT			GPIO1
#endif

#if defined(RED_RX_LED_PORT_USE_PORT0)
#define RED_RX_LED_PORT				GPIO0
#elif defined(RED_RX_LED_PORT_USE_PORT1)
#define RED_RX_LED_PORT				GPIO1
#endif

void PhaseLedOpen(void);
void PhaseLedWrite(uint32_t phase,uint32_t mode);

#if defined(A_LED_PORT_USE_PORT0)
#define A_LED_PORT					GPIO0
#elif defined(A_LED_PORT_USE_PORT1)
#define A_LED_PORT					GPIO1
#endif

#if defined(B_LED_PORT_USE_PORT0)
#define B_LED_PORT					GPIO0
#elif defined(B_LED_PORT_USE_PORT1)
#define B_LED_PORT					GPIO1
#endif

#if defined(C_LED_PORT_USE_PORT0)
#define C_LED_PORT					GPIO0
#elif defined(C_LED_PORT_USE_PORT1)
#define C_LED_PORT					GPIO1
#endif

#define TX_LED_ON()    GPIO_SetBits(GREEN_TX_LED_PORT,GREEN_TX_LED_PIN)
#define TX_LED_OFF()    GPIO_ResetBits(GREEN_TX_LED_PORT,GREEN_TX_LED_PIN)
#define RX_LED_ON()    GPIO_SetBits(RED_RX_LED_PORT,RED_RX_LED_PIN)
#define RX_LED_OFF()    GPIO_ResetBits(RED_RX_LED_PORT,RED_RX_LED_PIN)

#define PA_LED_ON()    GPIO_SetBits(A_LED_PORT,A_LED_PIN)
#define PA_LED_OFF()    GPIO_ResetBits(A_LED_PORT,A_LED_PIN)

#define PB_LED_ON()    GPIO_SetBits(B_LED_PORT,B_LED_PIN)
#define PB_LED_OFF()    GPIO_ResetBits(B_LED_PORT,B_LED_PIN)

#define PC_LED_ON()    GPIO_SetBits(C_LED_PORT,C_LED_PIN)
#define PC_LED_OFF()    GPIO_ResetBits(C_LED_PORT,C_LED_PIN)

#endif
#endif


/*
*
*/
#ifndef __BSP_LED_H__
#define __BSP_LED_H__
#include <gsmcuxx_hal_def.h>
#define GREEN_LED           (0)
#define RED_LED             (1)

void LedRunOpen(void);
void LedRunWrite(int mode);

void LedOpen(void);
void LedWrite(uint32_t led,uint32_t mode);
uint32_t LedRead(uint32_t led);
void LedClose(void);
#if DEV_STA
void LedOpen(void);
void LedWrite(uint32_t led,uint32_t mode);
uint32_t LedRead(uint32_t led);
void LedClose(void);



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
uint32_t PhaseLedRead(uint32_t phase);

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

#ifdef GREEN_TX_LED_PORT
#define TX_LED_ON()    do{\
	if (CCO_Hard_Type)\
	{\
		GPIO_SetBits(GREEN_TX_LED_PORT, GREEN_TX_LED_PIN);\
	}\
	else\
	{\
		GPIO_ResetBits(GREEN_TX_LED_PORT,GREEN_TX_LED_PIN);\
	}\
}while (0)

#define TX_LED_OFF()   do{ \
	if(CCO_Hard_Type)\
	{\
		GPIO_ResetBits(GREEN_TX_LED_PORT,GREEN_TX_LED_PIN);\
	}\
	else\
	{\
		GPIO_SetBits(GREEN_TX_LED_PORT, GREEN_TX_LED_PIN);\
	}\
}while(0)
#else
#define TX_LED_ON()
#define TX_LED_OFF()
#endif

#ifdef RED_RX_LED_PORT
#define RX_LED_ON()    do{\
	if (CCO_Hard_Type)\
	{\
		GPIO_SetBits(RED_RX_LED_PORT,RED_RX_LED_PIN);\
	}\
	else\
	{\
		GPIO_ResetBits(RED_RX_LED_PORT,RED_RX_LED_PIN);\
	}\
}while (0)

#define RX_LED_OFF()    do{\
	if (CCO_Hard_Type)\
	{\
		GPIO_ResetBits(RED_RX_LED_PORT,RED_RX_LED_PIN);\
	}\
	else\
	{\
		GPIO_SetBits(RED_RX_LED_PORT,RED_RX_LED_PIN);\
	}\
}while (0)
#else
#define RX_LED_ON()
#define RX_LED_OFF()
#endif

#ifdef A_LED_PORT
#define PA_LED_ON()    GPIO_SetBits(A_LED_PORT,A_LED_PIN)
#define PA_LED_OFF()    GPIO_ResetBits(A_LED_PORT,A_LED_PIN)
#else
#define PA_LED_ON()
#define PA_LED_OFF()
#endif

#ifdef B_LED_PORT
#define PB_LED_ON()    GPIO_SetBits(B_LED_PORT,B_LED_PIN)
#define PB_LED_OFF()    GPIO_ResetBits(B_LED_PORT,B_LED_PIN)
#else
#define PB_LED_ON()
#define PB_LED_OFF()
#endif

#ifdef C_LED_PORT
#define PC_LED_ON()    GPIO_SetBits(C_LED_PORT,C_LED_PIN)
#define PC_LED_OFF()    GPIO_ResetBits(C_LED_PORT,C_LED_PIN)
#else
#define PC_LED_ON()
#define PC_LED_OFF()
#endif

#endif
#endif


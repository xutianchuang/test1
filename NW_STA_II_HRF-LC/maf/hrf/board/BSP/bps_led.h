/*
*
*/
#ifndef __BSP_LED_H__
#define __BSP_LED_H__
#include <gsmcuxx_hal_def.h>
#define GREEN_LED           (0)
#define RED_LED             (1)

void LedOpen(void);
void LedWrite(uint32_t led,uint32_t mode);
void LedRunOpen(void);
void LedRunWrite(int mode);



void PhaseLedOpen(void);
void PhaseLedWrite(uint32_t phase,uint32_t mode);



#define TX_LED_ON()    GPIO_PinWrite(GREEN_TX_LED_PORT,GREEN_TX_LED_PIN,1)
#define TX_LED_OFF()   GPIO_PinWrite(GREEN_TX_LED_PORT,GREEN_TX_LED_PIN,0)
#define RX_LED_ON()    GPIO_PinWrite(RED_RX_LED_PORT,RED_RX_LED_PIN,1)
#define RX_LED_OFF()   GPIO_PinWrite(RED_RX_LED_PORT,RED_RX_LED_PIN,0)

#define PA_LED_ON()    GPIO_PinWrite(A_LED_PORT,A_LED_PIN,1)
#define PA_LED_OFF()   GPIO_PinWrite(A_LED_PORT,A_LED_PIN,0)

#define PB_LED_ON()    GPIO_PinWrite(B_LED_PORT,B_LED_PIN,1)
#define PB_LED_OFF()   GPIO_PinWrite(B_LED_PORT,B_LED_PIN,0)

#define PC_LED_ON()    GPIO_PinWrite(C_LED_PORT,C_LED_PIN,1)
#define PC_LED_OFF()   GPIO_PinWrite(C_LED_PORT,C_LED_PIN,0)


#endif


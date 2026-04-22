/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_led.h"
#include "os.h"
#include "bsp.h"
//

void LedOpen(void)
{
#if defined(GREEN_TX_LED_PORT) && defined(RED_RX_LED_PORT)
	GPIO_SetPinDir(RED_RX_LED_PORT, RED_RX_LED_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(RED_RX_LED_PORT, RED_RX_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(RED_RX_LED_PORT, RED_RX_LED_PIN, 0);


	GPIO_SetPinDir(GREEN_TX_LED_PORT, GREEN_TX_LED_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(GREEN_TX_LED_PORT, GREEN_TX_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(GREEN_TX_LED_PORT, GREEN_TX_LED_PIN, 0);
#endif
}
//mode  0 = light-off,other = light,
void LedWrite(uint32_t led,uint32_t mode)
{
#if defined(GREEN_TX_LED_PORT) && defined(RED_RX_LED_PORT)			
	if(led == GREEN_LED)
	{
		GPIO_PinWrite(GREEN_TX_LED_PORT,GREEN_TX_LED_PIN,mode);
	}
	else if(led == RED_LED)
	{
		GPIO_PinWrite(RED_RX_LED_PORT,RED_RX_LED_PIN,mode);
	}
#endif
}
void PhaseLedOpen(void)
{
	if (CCO_Hard_Type)
	{

	#if defined(A_LED_PORT)
		GPIO_SetPinDir(A_LED_PORT, A_LED_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(A_LED_PORT, A_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(A_LED_PORT, A_LED_PIN, 0);
	#endif
	#if defined(B_LED_PORT)
		GPIO_SetPinDir(B_LED_PORT, B_LED_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(B_LED_PORT, B_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(B_LED_PORT, B_LED_PIN, 0);
	#endif
	#if defined(C_LED_PORT)
		GPIO_SetPinDir(C_LED_PORT, C_LED_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(C_LED_PORT, C_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(C_LED_PORT, C_LED_PIN, 0);
	#endif
	}
}


void PhaseLedWrite(uint32_t phase,uint32_t mode)
{
	if (CCO_Hard_Type)
	{
		if (phase == BPS_PHASE_A)
		{
	#if defined(A_LED_PORT)
			GPIO_PinWrite(A_LED_PORT,A_LED_PIN,mode);
	#endif
		}
		else if(phase == BPS_PHASE_B)
		{
	#if defined(B_LED_PORT)
			GPIO_PinWrite(B_LED_PORT,B_LED_PIN,mode); 
	#endif
		}
		else if(phase == BPS_PHASE_C)
		{
	#if defined(C_LED_PORT)
			GPIO_PinWrite(C_LED_PORT,C_LED_PIN,mode);
	#endif
		}
	}
}
void LedRunOpen(void)
{
	if (!CCO_Hard_Type)
	{
	#if defined(RUN_LED_PORT)
		GPIO_SetPinDir(RUN_LED_PORT, RUN_LED_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(RUN_LED_PORT, RUN_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(RUN_LED_PORT, RUN_LED_PIN, 0);

	#endif
	}
}


void LedRunWrite(int mode)
{
	if (!CCO_Hard_Type)
	{
	#if defined(RUN_LED_PORT)
		GPIO_PinWrite(RUN_LED_PORT, RUN_LED_PIN, mode);
	#endif
	}
}



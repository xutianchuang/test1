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

	if (myDevicType == METER)
	{
#if defined(GREEN_RX_LED_PORT) && defined(RED_TX_LED_PORT)		

		GPIO_SetPinDir(GREEN_RX_LED_PORT,GREEN_RX_LED_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(GREEN_RX_LED_PORT,GREEN_RX_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(GREEN_RX_LED_PORT,GREEN_RX_LED_PIN,0);

		GPIO_SetPinDir(RED_TX_LED_PORT,RED_TX_LED_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(RED_TX_LED_PORT,RED_TX_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(RED_TX_LED_PORT,RED_TX_LED_PIN,0);
#endif		
	}
	else if (myDevicType == METER_3PHASE)
	{
#if defined(GREEN_RX_LED_PORT_3PHASE) && defined(RED_TX_LED_PORT_3PHASE)			
		GPIO_SetPinDir(GREEN_RX_LED_PORT_3PHASE,GREEN_RX_LED_PIN_3PHASE, GPIO_OUTPUT); //
		PORT_PinConfigure(GREEN_RX_LED_PORT_3PHASE,GREEN_RX_LED_PIN_3PHASE, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(GREEN_RX_LED_PORT_3PHASE,GREEN_RX_LED_PIN_3PHASE,0);

		GPIO_SetPinDir(RED_TX_LED_PORT_3PHASE,RED_TX_LED_PIN_3PHASE, GPIO_OUTPUT); //
		PORT_PinConfigure(RED_TX_LED_PORT_3PHASE,RED_TX_LED_PIN_3PHASE, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(RED_TX_LED_PORT_3PHASE,RED_TX_LED_PIN_3PHASE,0);
#endif
	}
#if defined(HRF_TX_LED_PORT) && defined(HRF_RX_LED_PORT)		

		GPIO_SetPinDir(HRF_RX_LED_PORT,HRF_RX_LED_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(HRF_RX_LED_PORT,HRF_RX_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(HRF_RX_LED_PORT,HRF_RX_LED_PIN,0);

		GPIO_SetPinDir(HRF_TX_LED_PORT,HRF_TX_LED_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(HRF_TX_LED_PORT,HRF_TX_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(HRF_TX_LED_PORT,HRF_TX_LED_PIN,0);
#endif		        

}
//mode  0 = light-off,other = light,
void LedWrite(uint32_t led,uint32_t mode)
{
	if (myDevicType == METER)
	{
#if defined(GREEN_RX_LED_PORT) && defined(RED_TX_LED_PORT)			
		if(led == GREEN_LED)
		{
			GPIO_PinWrite(GREEN_RX_LED_PORT,GREEN_RX_LED_PIN,mode);
		}
		else if(led == RED_LED)
		{
			GPIO_PinWrite(RED_TX_LED_PORT,RED_TX_LED_PIN,mode);
		}
#endif		
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(GREEN_RX_LED_PORT_3PHASE) && defined(RED_TX_LED_PORT_3PHASE)
		if(led == GREEN_LED)
		{
			GPIO_PinWrite(GREEN_RX_LED_PORT_3PHASE,GREEN_RX_LED_PIN_3PHASE,mode);
		}
		else if(led == RED_LED)
		{
			GPIO_PinWrite(RED_TX_LED_PORT_3PHASE,RED_TX_LED_PIN_3PHASE,mode);
		}
#endif		
	}
}











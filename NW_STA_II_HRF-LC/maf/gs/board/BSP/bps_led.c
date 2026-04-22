/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_led.h"
#include "os.h"
#include "bsp.h"
//
#if DEV_STA
void LedOpen(void)
{
	GPIO_GlobalInit();
	GPIO_InitTypeDef GPIO_InitStruct;
	if (myDevicType == METER)
	{
#if defined(GREEN_RX_LED_PORT) && defined(RED_TX_LED_PORT)		
		SCU_GPIOInputConfig(GREEN_RX_LED_PORT,GREEN_RX_LED_PIN);
		SCU_GPIOInputConfig(RED_TX_LED_PORT,RED_TX_LED_PIN);
		
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = RED_TX_LED_PIN;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPU;
		GPIO_Init(RED_TX_LED_PORT,&GPIO_InitStruct);
		
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = GREEN_RX_LED_PIN;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPU;
		GPIO_Init(GREEN_RX_LED_PORT,&GPIO_InitStruct);
#endif		
	}
	else if (myDevicType == METER_3PHASE)
	{
#if defined(GREEN_RX_LED_PORT_3PHASE) && defined(RED_TX_LED_PORT_3PHASE)			
		SCU_GPIOInputConfig(GREEN_RX_LED_PORT_3PHASE,GREEN_RX_LED_PIN_3PHASE);
		SCU_GPIOInputConfig(RED_TX_LED_PORT_3PHASE,RED_TX_LED_PIN_3PHASE);
		
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = RED_TX_LED_PIN_3PHASE;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPU;
		GPIO_Init(RED_TX_LED_PORT_3PHASE,&GPIO_InitStruct);
		
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = GREEN_RX_LED_PIN_3PHASE;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPU;
		GPIO_Init(GREEN_RX_LED_PORT_3PHASE,&GPIO_InitStruct);
#endif
	}
}
//mode  0 = light-off,other = light,
void LedWrite(uint32_t led,uint32_t mode)
{
	if (myDevicType == METER)
	{
#if defined(GREEN_RX_LED_PORT) && defined(RED_TX_LED_PORT)			
		if(led == GREEN_LED)
		{
			if(mode == 0)
			{
				GPIO_ResetBits(GREEN_RX_LED_PORT,GREEN_RX_LED_PIN);
			}
			else
			{
				GPIO_SetBits(GREEN_RX_LED_PORT,GREEN_RX_LED_PIN);
			}
		}
		else if(led == RED_LED)
		{
			if(mode == 0)
			{
				GPIO_ResetBits(RED_TX_LED_PORT,RED_TX_LED_PIN);
			}
			else
			{
				GPIO_SetBits(RED_TX_LED_PORT,RED_TX_LED_PIN);
			} 
		}
#endif		
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(GREEN_RX_LED_PORT_3PHASE) && defined(RED_TX_LED_PORT_3PHASE)
		if(led == GREEN_LED)
		{
			if(mode == 0)
			{
				GPIO_ResetBits(GREEN_RX_LED_PORT_3PHASE,GREEN_RX_LED_PIN_3PHASE);
			}
			else
			{
				GPIO_SetBits(GREEN_RX_LED_PORT_3PHASE,GREEN_RX_LED_PIN_3PHASE);
			}
		}
		else if(led == RED_LED)
		{
			if(mode == 0)
			{
				GPIO_ResetBits(RED_TX_LED_PORT_3PHASE,RED_TX_LED_PIN_3PHASE);
			}
			else
			{
				GPIO_SetBits(RED_TX_LED_PORT_3PHASE,RED_TX_LED_PIN_3PHASE);
			} 
		}
#endif		
	}
}
uint32_t LedRead(uint32_t led)
{
	if(myDevicType == METER)
	{
#if defined(GREEN_RX_LED_PORT) && defined(RED_TX_LED_PORT)
		if(led == GREEN_LED)
		{
			return GPIO_ReadOutputDataBit(GREEN_RX_LED_PORT,GREEN_RX_LED_PIN);
		}
		else if(led == RED_LED)
		{
			return GPIO_ReadOutputDataBit(RED_TX_LED_PORT,RED_TX_LED_PIN);
		}
#endif		
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(GREEN_RX_LED_PORT_3PHASE) && defined(RED_TX_LED_PORT_3PHASE)		
		if(led == GREEN_LED)
		{
			return GPIO_ReadOutputDataBit(GREEN_RX_LED_PORT_3PHASE,GREEN_RX_LED_PIN_3PHASE);
		}
		else if(led == RED_LED)
		{
			return GPIO_ReadOutputDataBit(RED_TX_LED_PORT_3PHASE,RED_TX_LED_PIN_3PHASE);
		}
#endif		
	}
	return 0xFF;
}
#elif DEV_CCO
void LedOpen(void)
{
	GPIO_GlobalInit();
#if defined(GREEN_TX_LED_PORT) && defined(RED_RX_LED_PORT)	
	GPIO_InitTypeDef GPIO_InitStruct;
	
	SCU_GPIOInputConfig(GREEN_TX_LED_PORT,GREEN_TX_LED_PIN);
	SCU_GPIOInputConfig(RED_RX_LED_PORT,RED_RX_LED_PIN);
	
	GPIO_StructInit(&GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = RED_RX_LED_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPU;
	GPIO_Init(RED_RX_LED_PORT,&GPIO_InitStruct);
	GPIO_ResetBits(RED_RX_LED_PORT,RED_RX_LED_PIN);
	
	GPIO_StructInit(&GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = GREEN_TX_LED_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPU;
	GPIO_Init(GREEN_TX_LED_PORT,&GPIO_InitStruct);
	GPIO_ResetBits(GREEN_TX_LED_PORT,GREEN_TX_LED_PIN);
#endif		
}
//mode  0 = light-off,other = light,
void LedWrite(uint32_t led,uint32_t mode)
{
#if defined(GREEN_TX_LED_PORT) && defined(RED_RX_LED_PORT)			
	if(led == GREEN_LED)
	{
		if(mode == 0)
		{
			GPIO_ResetBits(GREEN_TX_LED_PORT,GREEN_TX_LED_PIN);
		}
		else
		{
			GPIO_SetBits(GREEN_TX_LED_PORT,GREEN_TX_LED_PIN);
		}
	}
	else if(led == RED_LED)
	{
		if(mode == 0)
		{
			GPIO_ResetBits(RED_RX_LED_PORT,RED_RX_LED_PIN);
		}
		else
		{
			GPIO_SetBits(RED_RX_LED_PORT,RED_RX_LED_PIN);
		} 
	}
#endif		
}

uint32_t LedRead(uint32_t led)
{
#if defined(GREEN_TX_LED_PORT) && defined(RED_RX_LED_PORT)
	if(led == GREEN_LED)
	{
		return GPIO_ReadOutputDataBit(GREEN_TX_LED_PORT,GREEN_TX_LED_PIN);
	}
	else if(led == RED_LED)
	{
		return GPIO_ReadOutputDataBit(RED_RX_LED_PORT,RED_RX_LED_PIN);
	}
#endif		
	return 0xFF;
}

void PhaseLedOpen(void)
{
	if (CCO_Hard_Type)
	{
		GPIO_GlobalInit();
		GPIO_InitTypeDef GPIO_InitStruct;
		GPIO_StructInit(&GPIO_InitStruct);	
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPD;
	#if defined(A_LED_PORT)		
		SCU_GPIOInputConfig(A_LED_PORT,A_LED_PIN);
		GPIO_InitStruct.GPIO_Pin = A_LED_PIN;
		GPIO_Init(A_LED_PORT,&GPIO_InitStruct);
		GPIO_ResetBits(A_LED_PORT,A_LED_PIN);
	#endif
	#if defined(B_LED_PORT)		
		SCU_GPIOInputConfig(B_LED_PORT,B_LED_PIN);
		GPIO_InitStruct.GPIO_Pin = B_LED_PIN;
		GPIO_Init(B_LED_PORT,&GPIO_InitStruct);
		GPIO_ResetBits(B_LED_PORT,B_LED_PIN);
	#endif
	#if defined(C_LED_PORT)		
		SCU_GPIOInputConfig(C_LED_PORT,C_LED_PIN);
		GPIO_InitStruct.GPIO_Pin = C_LED_PIN;
		GPIO_Init(C_LED_PORT,&GPIO_InitStruct);
		GPIO_ResetBits(C_LED_PORT,C_LED_PIN);
	#endif		
	}
}
//mode  0 = light-off,other = light,
void PhaseLedWrite(uint32_t phase,uint32_t mode)
{
	if (CCO_Hard_Type)
	{
		if (phase == BPS_PHASE_A)
		{
	#if defined(A_LED_PORT)
			if(mode == 0){GPIO_ResetBits(A_LED_PORT,A_LED_PIN);}
			else{GPIO_SetBits(A_LED_PORT,A_LED_PIN);}
	#endif
		}
		else if(phase == BPS_PHASE_B)
		{
	#if defined(B_LED_PORT)
			if(mode == 0){GPIO_ResetBits(B_LED_PORT,B_LED_PIN);}
			else{GPIO_SetBits(B_LED_PORT,B_LED_PIN);} 
	#endif
		}
		else if(phase == BPS_PHASE_C)
		{
	#if defined(C_LED_PORT)
			if(mode == 0){GPIO_ResetBits(C_LED_PORT,C_LED_PIN);}
			else{GPIO_SetBits(C_LED_PORT,C_LED_PIN);} 
	#endif
		}
	}
}

uint32_t PhaseLedRead(uint32_t phase)
{
	if (CCO_Hard_Type)
	{
		if (phase == BPS_PHASE_A)
		{
	#if defined(A_LED_PORT)
			return GPIO_ReadOutputDataBit(A_LED_PORT,A_LED_PIN);
	#endif
		}
		else if(phase == BPS_PHASE_B)
		{
	#if defined(B_LED_PORT)		
			return GPIO_ReadOutputDataBit(B_LED_PORT,B_LED_PIN);
	#endif
		}
		else if(phase == BPS_PHASE_C)
		{
	#if defined(C_LED_PORT)		
			return GPIO_ReadOutputDataBit(C_LED_PORT,C_LED_PIN);
	#endif
		}
	}
	return 0xFF;
}
#endif

void LedClose(void)
{
	
}

void LedRunOpen(void)
{
	if (!CCO_Hard_Type)
	{
	#if defined(C_LED_PORT)		
          	GPIO_InitTypeDef GPIO_InitStruct;
		GPIO_StructInit(&GPIO_InitStruct);	
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPD;
		SCU_GPIOInputConfig(C_LED_PORT,C_LED_PIN);
		GPIO_InitStruct.GPIO_Pin = C_LED_PIN;
		GPIO_Init(C_LED_PORT,&GPIO_InitStruct);
		GPIO_ResetBits(C_LED_PORT,C_LED_PIN);
	#endif
	}
}
void LedRunWrite(int mode)
{
	if (!CCO_Hard_Type)
	{
	#if defined(C_LED_PORT)		
		if(mode == 0){GPIO_ResetBits(C_LED_PORT,C_LED_PIN);}
		else{GPIO_SetBits(C_LED_PORT,C_LED_PIN);} 
	#endif	
	}
}




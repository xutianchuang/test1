/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_zerocross.h"
#include "os.h"
static ZeroCrossCallBackFunctionType ZeroCrossCallBackFunction = NULL;

static void ZeroXCrossIsr(int phase)
{
	if (phase==BPS_PHASE_A)
	{
#if defined(ZC_PIN_PHASEA_POS)		
		GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEA_POS);
#endif
	}
	else if (phase==BPS_PHASE_B)
	{
#if defined(ZC_PIN_PHASEB_POS)			
		GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEB_POS);
#endif
	}
	else if (phase==BPS_PHASE_C)
	{
#if defined(ZC_PIN_PHASEC_POS)			
		GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEC_POS);
#endif
	}
	else if (phase==BPS_PHASE_A_NEG)
	{
#if defined(ZC_PIN_PHASEA_NEG)		
		GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEA_NEG);
#endif
	}
	else if (phase==BPS_PHASE_B_NEG)
	{
#if defined(ZC_PIN_PHASEB_NEG)			
		GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEB_NEG);
#endif
	}
	else if (phase==BPS_PHASE_C_NEG)
	{
#if defined(ZC_PIN_PHASEC_NEG)			
		GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEC_NEG);
#endif
	}	
	else
	{
		return;
	}
	if(ZeroCrossCallBackFunction != NULL)
	{
		ZeroCrossCallBackFunction(phase);
	}
}

#if defined(ZC_PIN_PHASEA_POS)
static void ZeroAPosCrossIsr(void)
{
    CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	OSIntEnter();	
	ZeroXCrossIsr(BPS_PHASE_A);
	OS_CRITICAL_EXIT();
	OSIntExit();
}
#endif
#if defined(ZC_PIN_PHASEB_POS)
static void ZeroBPosCrossIsr(void)
{
    CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	OSIntEnter();	
	ZeroXCrossIsr(BPS_PHASE_B);
	OS_CRITICAL_EXIT();
	OSIntExit();	
}
#endif
#if defined(ZC_PIN_PHASEC_POS)
static void ZeroCPosCrossIsr(void)
{
    CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	OSIntEnter();	
	ZeroXCrossIsr(BPS_PHASE_C);
	OS_CRITICAL_EXIT();
	OSIntExit();		
}
#endif
#if defined(ZC_PIN_PHASEA_NEG)
static void ZeroANegCrossIsr(void)
{
    CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	OSIntEnter();	
	ZeroXCrossIsr(BPS_PHASE_A_NEG);
	OS_CRITICAL_EXIT();
	OSIntExit();	
}
#endif
#if defined(ZC_PIN_PHASEB_NEG)
static void ZeroBNegCrossIsr(void)
{
    CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	OSIntEnter();	
	ZeroXCrossIsr(BPS_PHASE_B_NEG);
	OS_CRITICAL_EXIT();
	OSIntExit();	
}
#endif
#if defined(ZC_PIN_PHASEC_NEG)
static void ZeroCNegCrossIsr(void)
{
    CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	OSIntEnter();		
	ZeroXCrossIsr(BPS_PHASE_C_NEG);
	OS_CRITICAL_EXIT();
	OSIntExit();	
}
#endif

void ZeroCrossInit()
{
    GPIO_GlobalInit();
    GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_StructInit(&GPIO_InitStruct);
#if defined(ZC_PIN_PHASEA_POS)
	SCU_GPIOInputConfig(GPIO1, ZC_PIN_PHASEA_POS);
	GPIO_InitStruct.GPIO_Pin = ZC_PIN_PHASEA_POS;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIO1,&GPIO_InitStruct);
	GPIO_SetBouncePreScale(GPIO1, 4);
	GPIO_EnableBounce(GPIO1, ZC_PIN_PHASEA_POS);
	BSP_IntVectSet(ZC_PHASEA_POS_IRQn,ZeroAPosCrossIsr);
	NVIC_DisableIRQ(ZC_PHASEA_POS_IRQn);
	GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEA_POS);
	GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEA_POS, DISABLE);
#endif
	if(myDevicType == METER_3PHASE || myDevicType == CONCENTRATOR)
	{
#if defined(ZC_PIN_PHASEB_POS)		
		SCU_GPIOInputConfig(GPIO1, ZC_PIN_PHASEB_POS);	
		GPIO_InitStruct.GPIO_Pin = ZC_PIN_PHASEB_POS;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_Init(GPIO1,&GPIO_InitStruct);
		GPIO_SetBouncePreScale(GPIO1, 4);
		GPIO_EnableBounce(GPIO1, ZC_PIN_PHASEB_POS);
		BSP_IntVectSet(ZC_PHASEB_POS_IRQn,ZeroBPosCrossIsr);
		NVIC_DisableIRQ(ZC_PHASEB_POS_IRQn);
		NVIC_ClearPendingIRQ(ZC_PHASEB_POS_IRQn);		
		GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEB_POS, DISABLE);
#endif
#if defined(ZC_PIN_PHASEC_POS)
		SCU_GPIOInputConfig(GPIO1, ZC_PIN_PHASEC_POS);	
		GPIO_InitStruct.GPIO_Pin = ZC_PIN_PHASEC_POS;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_Init(GPIO1,&GPIO_InitStruct);
		GPIO_SetBouncePreScale(GPIO1, 4);
		GPIO_EnableBounce(GPIO1, ZC_PIN_PHASEC_POS);
		BSP_IntVectSet(ZC_PHASEC_POS_IRQn,ZeroCPosCrossIsr);
		NVIC_DisableIRQ(ZC_PHASEC_POS_IRQn);
		NVIC_ClearPendingIRQ(ZC_PHASEC_POS_IRQn);
		GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEC_POS, DISABLE);
#endif
		if(myDevicType == CONCENTRATOR)
		{
#if defined(ZC_PIN_PHASEA_NEG)
			SCU_GPIOInputConfig(GPIO1, ZC_PIN_PHASEA_NEG);
			GPIO_InitStruct.GPIO_Pin = ZC_PIN_PHASEA_NEG;
			GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
			GPIO_Init(GPIO1,&GPIO_InitStruct);
			GPIO_SetBouncePreScale(GPIO1, 4);
			GPIO_EnableBounce(GPIO1, ZC_PIN_PHASEA_NEG);
			BSP_IntVectSet(ZC_PHASEA_NEG_IRQn,ZeroANegCrossIsr);
			NVIC_DisableIRQ(ZC_PHASEA_NEG_IRQn);
			NVIC_ClearPendingIRQ(ZC_PHASEA_NEG_IRQn);
			GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEA_NEG, DISABLE);
#endif			
#if defined(ZC_PIN_PHASEB_NEG)		
			SCU_GPIOInputConfig(GPIO1, ZC_PIN_PHASEB_NEG);	
			GPIO_InitStruct.GPIO_Pin = ZC_PIN_PHASEB_NEG;
			GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
			GPIO_Init(GPIO1,&GPIO_InitStruct);
			GPIO_SetBouncePreScale(GPIO1, 4);
			GPIO_EnableBounce(GPIO1, ZC_PIN_PHASEB_NEG);
			BSP_IntVectSet(ZC_PHASEB_NEG_IRQn,ZeroBNegCrossIsr);
			NVIC_DisableIRQ(ZC_PHASEB_NEG_IRQn);			
			NVIC_ClearPendingIRQ(ZC_PHASEB_NEG_IRQn);
			GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEB_NEG, DISABLE);
#endif
#if defined(ZC_PIN_PHASEC_NEG)
			SCU_GPIOInputConfig(GPIO1, ZC_PIN_PHASEC_NEG);	
			GPIO_InitStruct.GPIO_Pin = ZC_PIN_PHASEC_NEG;
			GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
			GPIO_Init(GPIO1,&GPIO_InitStruct);
			GPIO_SetBouncePreScale(GPIO1, 4);
			GPIO_EnableBounce(GPIO1, ZC_PIN_PHASEC_NEG);
			BSP_IntVectSet(ZC_PHASEC_NEG_IRQn,ZeroCNegCrossIsr);
			NVIC_DisableIRQ(ZC_PHASEC_NEG_IRQn);			
			NVIC_ClearPendingIRQ(ZC_PHASEC_NEG_IRQn);
			GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEC_NEG, DISABLE);
#endif			
		}
	}
}

void ZeroCrossOpen(BspZeroCrossParamType *param)
{
	ZeroCrossCallBackFunction = param->callfunction;
	
#if defined(ZC_PIN_PHASEA_POS)
	GPIO_SetIRQTriggerMode(GPIO1, ZC_PIN_PHASEA_POS, param->TriggerMode);
	GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEA_POS);
	__NOP();__NOP();__NOP();__NOP();__NOP();
	__NOP();__NOP();__NOP();__NOP();__NOP();
	NVIC_ClearPendingIRQ(ZC_PHASEA_POS_IRQn);
	NVIC_EnableIRQ(ZC_PHASEA_POS_IRQn);	
	GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEA_POS, ENABLE);
#endif
	if(myDevicType == METER_3PHASE || myDevicType == CONCENTRATOR)
	{
#if defined(ZC_PIN_PHASEB_POS)		
		GPIO_SetIRQTriggerMode(GPIO1, ZC_PIN_PHASEB_POS, param->TriggerMode);
		GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEB_POS);
		__NOP();__NOP();__NOP();__NOP();__NOP();
		__NOP();__NOP();__NOP();__NOP();__NOP();
		NVIC_ClearPendingIRQ(ZC_PHASEB_POS_IRQn);
		NVIC_EnableIRQ(ZC_PHASEB_POS_IRQn);
		GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEB_POS, ENABLE);
#endif
#if defined(ZC_PIN_PHASEC_POS)
		GPIO_SetIRQTriggerMode(GPIO1, ZC_PIN_PHASEC_POS, param->TriggerMode);
		GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEC_POS);
		__NOP();__NOP();__NOP();__NOP();__NOP();
		__NOP();__NOP();__NOP();__NOP();__NOP();		
		NVIC_ClearPendingIRQ(ZC_PHASEC_POS_IRQn);
		NVIC_EnableIRQ(ZC_PHASEC_POS_IRQn);
		GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEC_POS, ENABLE);
#endif
		if(myDevicType == CONCENTRATOR)
		{
#if defined(ZC_PIN_PHASEA_NEG)
			GPIO_SetIRQTriggerMode(GPIO1, ZC_PIN_PHASEA_NEG, param->TriggerMode);
			GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEA_NEG);
			__NOP();__NOP();__NOP();__NOP();__NOP();
			__NOP();__NOP();__NOP();__NOP();__NOP();
			NVIC_ClearPendingIRQ(ZC_PHASEA_NEG_IRQn);
			NVIC_EnableIRQ(ZC_PHASEA_NEG_IRQn);	
			GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEA_NEG, ENABLE);
#endif			
#if defined(ZC_PIN_PHASEB_NEG)		
			GPIO_SetIRQTriggerMode(GPIO1, ZC_PIN_PHASEB_NEG, param->TriggerMode);
			GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEB_NEG);
			__NOP();__NOP();__NOP();__NOP();__NOP();
			__NOP();__NOP();__NOP();__NOP();__NOP();
			NVIC_ClearPendingIRQ(ZC_PHASEB_NEG_IRQn);
			NVIC_EnableIRQ(ZC_PHASEB_NEG_IRQn);
			GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEB_NEG, ENABLE);
#endif
#if defined(ZC_PIN_PHASEC_NEG)
			GPIO_SetIRQTriggerMode(GPIO1, ZC_PIN_PHASEC_NEG, param->TriggerMode);
			GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEC_NEG);
			__NOP();__NOP();__NOP();__NOP();__NOP();
			__NOP();__NOP();__NOP();__NOP();__NOP();
			NVIC_ClearPendingIRQ(ZC_PHASEC_NEG_IRQn);
			NVIC_EnableIRQ(ZC_PHASEC_NEG_IRQn);
			GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEC_NEG, ENABLE);
#endif			
		}
	}
}

void ZeroCrossClose(void)
{
#if defined(ZC_PIN_PHASEA_POS)
	NVIC_DisableIRQ(ZC_PHASEA_POS_IRQn);
	GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEA_POS);
	GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEA_POS, DISABLE);
#endif
	if(myDevicType == METER_3PHASE || myDevicType == CONCENTRATOR)
	{
#if defined(ZC_PIN_PHASEB_POS)	
		NVIC_DisableIRQ(ZC_PHASEB_POS_IRQn);
		GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEB_POS);
		GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEB_POS, DISABLE);
#endif
#if defined(ZC_PIN_PHASEC_POS)
		NVIC_DisableIRQ(ZC_PHASEC_POS_IRQn);
		GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEC_POS);
		GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEC_POS, DISABLE);
#endif
		if(myDevicType == CONCENTRATOR)
		{
#if defined(ZC_PIN_PHASEA_NEG)
			NVIC_DisableIRQ(ZC_PHASEA_NEG_IRQn);
			GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEA_NEG);
			GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEA_NEG, DISABLE);
#endif
#if defined(ZC_PIN_PHASEB_NEG)	
			NVIC_DisableIRQ(ZC_PHASEB_NEG_IRQn);
			GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEB_NEG);
			GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEB_NEG, DISABLE);
#endif
#if defined(ZC_PIN_PHASEC_NEG)
			NVIC_DisableIRQ(ZC_PHASEC_NEG_IRQn);
			GPIO_ClearIRQStatus(GPIO1, ZC_PIN_PHASEC_NEG);
			GPIO_SetIRQMode(GPIO1, ZC_PIN_PHASEC_NEG, DISABLE);
#endif
		}
	}
}








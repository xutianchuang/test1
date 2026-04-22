/*
*
*/
#include  <gsmcu_hal.h>
#include "gsmcu_m3_port.h"
#include "bps_zerocross.h"
#include "os.h"
#ifndef NULL
#define NULL	0
#endif
static ZeroCrossCallBackFunctionType ZeroCrossCallBackFunction = NULL;


static void ZeroXCrossIsr(int phase)
{
	if (ZeroCrossCallBackFunction)
	{
		ZeroCrossCallBackFunction(phase);
	}
}

#if defined(ZC_PIN_PHASEA_POS)
static void ZeroAPosCrossIsr(u32 vaile)
{
	ZeroXCrossIsr(BPS_PHASE_A);
}
#endif
#if defined(ZC_PIN_PHASEB_POS)
static void ZeroBPosCrossIsr(u32 vaile)
{
	ZeroXCrossIsr(BPS_PHASE_B);
}
#endif
#if defined(ZC_PIN_PHASEC_POS)
static void ZeroCPosCrossIsr(u32 vaile)
{
	ZeroXCrossIsr(BPS_PHASE_C);
}
#endif
#if defined(ZC_PIN_PHASEA_NEG)
static void ZeroANegCrossIsr(u32 vaile)
{
	ZeroXCrossIsr(BPS_PHASE_A_NEG);
}
#endif
#if defined(ZC_PIN_PHASEB_NEG)
static void ZeroBNegCrossIsr(u32 vaile)
{
	ZeroXCrossIsr(BPS_PHASE_B_NEG);
}
#endif
#if defined(ZC_PIN_PHASEC_NEG)
static void ZeroCNegCrossIsr(u32 vaile)
{
	ZeroXCrossIsr(BPS_PHASE_C_NEG);
}
#endif



void ZeroCrossInit()
{

#if defined(ZC_PIN_PHASEA_POS)
	BPS_AddGpioIrq(4, ZC_PIN_PHASEA_POS, ZeroAPosCrossIsr);
	GPIO_SetPinDir(4, ZC_PIN_PHASEA_POS, GPIO_INPUT); //
	PORT_PinConfigure(4, ZC_PIN_PHASEA_POS, (PORT_FUNC)(PORT_CFG_MODE(2)));
//	GPIO_Port_settrigger(4, ZC_PIN_PHASEA_POS, GPIO_FALLING_TRIGER);
#endif
	if (myDevicType == METER_3PHASE || myDevicType == CONCENTRATOR)
	{
#if defined(ZC_PIN_PHASEB_POS)
		BPS_AddGpioIrq(4, ZC_PIN_PHASEB_POS, ZeroBPosCrossIsr);
		GPIO_SetPinDir(4, ZC_PIN_PHASEB_POS, GPIO_INPUT); //
		PORT_PinConfigure(4, ZC_PIN_PHASEB_POS, (PORT_FUNC)(PORT_CFG_MODE(2)|PORT_PULLUP));
//		GPIO_Port_settrigger(4, ZC_PIN_PHASEB_POS, GPIO_FALLING_TRIGER);
#endif
#if defined(ZC_PIN_PHASEC_POS)
		BPS_AddGpioIrq(4, ZC_PIN_PHASEC_POS, ZeroCPosCrossIsr);
		GPIO_SetPinDir(4, ZC_PIN_PHASEC_POS, GPIO_INPUT); //
		PORT_PinConfigure(4, ZC_PIN_PHASEC_POS, (PORT_FUNC)(PORT_CFG_MODE(2)|PORT_PULLUP));
//		GPIO_Port_settrigger(4, ZC_PIN_PHASEC_POS, GPIO_FALLING_TRIGER);
#endif
		if (myDevicType == CONCENTRATOR)
		{
#if defined(ZC_PIN_PHASEA_NEG)
			BPS_AddGpioIrq(4, ZC_PIN_PHASEA_NEG, ZeroANegCrossIsr);
			GPIO_SetPinDir(4, ZC_PIN_PHASEA_NEG, GPIO_INPUT); //
			PORT_PinConfigure(4, ZC_PIN_PHASEA_NEG, (PORT_FUNC)(PORT_CFG_MODE(2)|PORT_PULLUP));
//			GPIO_Port_settrigger(4, ZC_PIN_PHASEA_NEG, GPIO_FALLING_TRIGER);
#endif
#if defined(ZC_PIN_PHASEB_NEG)
			BPS_AddGpioIrq(4, ZC_PIN_PHASEB_NEG, ZeroBNegCrossIsr);
			GPIO_SetPinDir(4, ZC_PIN_PHASEB_NEG, GPIO_INPUT); //
			PORT_PinConfigure(4, ZC_PIN_PHASEB_NEG, (PORT_FUNC)(PORT_CFG_MODE(2)|PORT_PULLUP));
//			GPIO_Port_settrigger(4, ZC_PIN_PHASEB_NEG, GPIO_FALLING_TRIGER);
#endif
#if defined(ZC_PIN_PHASEC_NEG)
			BPS_AddGpioIrq(4, ZC_PIN_PHASEC_NEG, ZeroCNegCrossIsr);
			GPIO_SetPinDir(4, ZC_PIN_PHASEC_NEG, GPIO_INPUT); //
			PORT_PinConfigure(4, ZC_PIN_PHASEC_NEG, (PORT_FUNC)(PORT_CFG_MODE(2)|PORT_PULLUP));
//			GPIO_Port_settrigger(4, ZC_PIN_PHASEC_NEG, GPIO_FALLING_TRIGER);
#endif
		}
	}
}

void ZeroCrossOpen(BspZeroCrossParamType *param)
{
	ZeroCrossCallBackFunction = param->callfunction;

#if defined(ZC_PIN_PHASEA_POS)

	GPIO_Port_settrigger(4, ZC_PIN_PHASEA_POS, GPIO_FALLING_TRIGER);

#endif
	if (myDevicType == METER_3PHASE || myDevicType == CONCENTRATOR)
	{
#if defined(ZC_PIN_PHASEB_POS)

		GPIO_Port_settrigger(4, ZC_PIN_PHASEB_POS, GPIO_FALLING_TRIGER);


#endif
#if defined(ZC_PIN_PHASEC_POS)

		GPIO_Port_settrigger(4, ZC_PIN_PHASEC_POS, GPIO_FALLING_TRIGER);

#endif
		if (myDevicType == CONCENTRATOR)
		{
#if defined(ZC_PIN_PHASEA_NEG)

			GPIO_Port_settrigger(4, ZC_PIN_PHASEA_NEG, GPIO_FALLING_TRIGER);

#endif
#if defined(ZC_PIN_PHASEB_NEG)

			GPIO_Port_settrigger(4, ZC_PIN_PHASEB_NEG, GPIO_FALLING_TRIGER);

#endif
#if defined(ZC_PIN_PHASEC_NEG)

			GPIO_Port_settrigger(4, ZC_PIN_PHASEC_NEG, GPIO_FALLING_TRIGER);

#endif
		}
	}
}

void ZeroCrossClose(void)
{
#if defined(ZC_PIN_PHASEA_POS)

	GPIO_DisableIrq(4, ZC_PIN_PHASEA_POS);

#endif
	if (myDevicType == METER_3PHASE || myDevicType == CONCENTRATOR)
	{
#if defined(ZC_PIN_PHASEB_POS)

		GPIO_DisableIrq(4, ZC_PIN_PHASEB_POS);
#endif
#if defined(ZC_PIN_PHASEC_POS)

		GPIO_DisableIrq(4, ZC_PIN_PHASEC_POS);
#endif
		if (myDevicType == CONCENTRATOR)
		{
#if defined(ZC_PIN_PHASEA_NEG)

			GPIO_DisableIrq(4, ZC_PIN_PHASEA_NEG);
#endif
#if defined(ZC_PIN_PHASEB_NEG)

			GPIO_DisableIrq(4, ZC_PIN_PHASEB_NEG);
#endif
#if defined(ZC_PIN_PHASEC_NEG)

			GPIO_DisableIrq(4, ZC_PIN_PHASEC_NEG);
#endif
		}
	}
}








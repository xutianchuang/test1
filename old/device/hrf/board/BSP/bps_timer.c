/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_timer.h"
#include "os.h"
#include <gsmcuxx_hal_def.h>
#include "gsmcu_m3_pit32.h"
#include "gsmcu_m3_pwm.h"
#include "RTE_Device.h"
/*
*
*/
static volatile TimerCallBackFunctionType TimerChanlIspList[BSP_TIMER_CHL_LENGTH];



static void  TimerNoTrunRealyTimer(uint32_t Timer,PIT_CHANNEL_Type** TMR)
{
	if (Timer<6)
	{
		*TMR = (PIT_CHANNEL_Type *)&PIT_CHANNEL0[Timer];
	}
	else
	{
		*TMR = (PIT_CHANNEL_Type *)&PIT_CHANNEL6[Timer-6];
	}
}



static void TimerxIsrFunction(uint32_t IntNo)
{
    PIT_CHANNEL_Type *Tmr;
    TimerNoTrunRealyTimer(IntNo, &Tmr);
    PIT32_Clear(Tmr);

    if (TimerChanlIspList[IntNo] != NULL)
    {
        TimerChanlIspList[IntNo]();
    }
}
void Timer0IsrFunction(void)
{
    TimerxIsrFunction(0);
}
void Timer1IsrFunction(void)
{
    TimerxIsrFunction(1);
}
void Timer2IsrFunction(void)
{
    TimerxIsrFunction(2);
}
void Timer3IsrFunction(void)
{
    TimerxIsrFunction(3);
}
void Timer4IsrFunction(void)
{
    TimerxIsrFunction(4);
}
void Timer5IsrFunction(void)
{
    TimerxIsrFunction(5);
}
void Timer6IsrFunction(void)
{
    TimerxIsrFunction(6);
}
void Timer7IsrFunction(void)
{
    TimerxIsrFunction(7);
}
void Timer8IsrFunction(void)
{
    TimerxIsrFunction(8);
}
void Timer9IsrFunction(void)
{
    TimerxIsrFunction(9);
}
void Timer10IsrFunction(void)
{
    TimerxIsrFunction(10);
}
void Timer11IsrFunction(void)
{
    TimerxIsrFunction(11);
}
void (*TimerCallBack[BSP_TIMER_CHL_LENGTH])(void)={
    Timer0IsrFunction,
    Timer1IsrFunction,
    Timer2IsrFunction,
    Timer3IsrFunction,
    Timer4IsrFunction,
    Timer5IsrFunction,
	Timer6IsrFunction,
    Timer7IsrFunction,
    Timer8IsrFunction,
    Timer9IsrFunction,
    Timer10IsrFunction,
    Timer11IsrFunction,
};

void TimerOpen(TimerParameterTypdef *TimerParameter)
{

    PIT_CHANNEL_Type *Tmr;
    TimerNoTrunRealyTimer(TimerParameter->Timer, &Tmr);
    TimerChanlIspList[TimerParameter->Timer] = TimerParameter->TimerCallBackFunction;
    PIT32_Clear(Tmr);
    PIT32_Config(Tmr, TimerParameter->TimerSlot20ns);
    PIT32_EnInt(Tmr);
    NVIC_ClearPendingIRQ((IRQn_Type)(PIT0_IRQn + TimerParameter->Timer));
    NVIC_EnableIRQ((IRQn_Type)(PIT0_IRQn + TimerParameter->Timer));

    
}
void TimerWrite(uint32_t Timer,uint32_t mode)
{
    PIT_CHANNEL_Type *Tmr;
    TimerNoTrunRealyTimer(Timer, &Tmr);
    if (mode)
    {
        PIT32_Enable(Tmr);
    }
    else
    {
        PIT32_Disable(Tmr);
    }
}


uint32_t TimerRead(uint32_t Timer)
{
    PIT_CHANNEL_Type* TMR;
	if (Timer > BSP_TIMER_CHL_LENGTH)
		return 0;
    TimerNoTrunRealyTimer(Timer,&TMR);
    return PIT32_GetCnt(TMR);
}
void TimerClose(uint32_t Timer)
{
    PIT_CHANNEL_Type *Tmr;
    TimerNoTrunRealyTimer(Timer, &Tmr);
    PIT32_Disable(Tmr);
    PIT32_DisInt(Tmr);
    NVIC_DisableIRQ((IRQn_Type)(PIT0_IRQn + Timer));
    NVIC_ClearPendingIRQ((IRQn_Type)(PIT0_IRQn + Timer));

    TimerChanlIspList[Timer] = NULL;
    
}



static void GetPwmPin(uint32_t Timer,u8 *port,u8 *pin)
{
	switch (Timer)
	{
	case 0:
		*port = PWM0_CH0_PORT,*pin = PWM0_CH0_BIT;
		break;
	case 1:
		*port = PWM0_CH1_PORT,*pin = PWM0_CH1_BIT;
		break;
	case 2:
		*port = PWM0_CH2_PORT,*pin = PWM0_CH2_BIT;
		break;
	case 3:
		*port = PWM0_CH3_PORT,*pin = PWM0_CH3_BIT;
		break;
	case 4:
		*port = PWM1_CH0_PORT,*pin = PWM1_CH0_BIT;
		break;
	case 5:
		*port = PWM1_CH1_PORT,*pin = PWM1_CH1_BIT;
		break;
	case 6:
		*port = PWM1_CH2_PORT,*pin = PWM1_CH2_BIT;
		break;
	case 7:
		*port = PWM1_CH3_PORT,*pin = PWM1_CH3_BIT;
		break;
	default:
		__BKPT(1);
		return;
	}
}
void PwmInit(uint32_t Timer)
{
	if (Timer<4)
	{
		#if RTE_PWM0
		Driver_PWM0.Initialize(0, 0);
		Driver_PWM0.Control(PWM_CLOCK_SEL_Msk|PWM_CH_MODE_AUTO|(Timer&0x3)<<PWM_CHANNEL_Pos, 0x7);
		#endif
	}
	else
	{
		#if RTE_PWM1
		Driver_PWM1.Initialize(0, 0);
        Driver_PWM1.Control(PWM_CLOCK_SEL_Msk|PWM_CH_MODE_AUTO|(Timer&0x3)<<PWM_CHANNEL_Pos, 0x7);
		#endif
	}
}
void PwmGpioFunc(uint32_t Timer,u8 isPwmFunc)
{

	u8 port,pin;
	GetPwmPin(Timer, &port, &pin);

	if (isPwmFunc)
	{
		PORT_PinConfigure(port, pin, (PORT_FUNC)(PORT_CFG_MODE(0)));
	}
	else
	{
		PORT_PinConfigure(port, pin, (PORT_FUNC)(PORT_CFG_MODE(2)));
	}
}
static uint32_t TimerCmpVile[8];
void TimerPwmOpen(TimerParameterTypdef *TimerParameter,uint32_t CmpTime20ns)
{

	if (TimerParameter->Timer < 4)
	{
		#if RTE_PWM0
		Driver_PWM0.Control(PWM_PINDDR_OUT|PWM_SET_BASE_VAL_Msk|(TimerParameter->Timer&0x3)<<PWM_CHANNEL_Pos, TimerParameter->TimerSlot20ns);
		#endif
	}
	else
	{
		#if RTE_PWM1
		Driver_PWM1.Control(PWM_PINDDR_OUT|PWM_SET_BASE_VAL_Msk|(TimerParameter->Timer&0x3)<<PWM_CHANNEL_Pos, TimerParameter->TimerSlot20ns);
		#endif
	}
	TimerCmpVile[TimerParameter->Timer]=CmpTime20ns;
}
void TimerPwmReset(TimerParameterTypdef *TimerParameter)
{
	TimerPwmWrite(TimerParameter->Timer,0);
}

void TimerPwmWrite(uint32_t Timer,uint32_t mode)
{
    

	if (Timer < 4)
	{
		#if RTE_PWM0
		if (mode)
		{
			Driver_PWM0.TimerStart(Timer & 0x3, TimerCmpVile[Timer]);
		}
		else
		{
			Driver_PWM0.TimerStop(Timer & 0x3);
		}
		#endif
	}
	else
	{
		#if RTE_PWM1
		if (mode)
		{
			Driver_PWM1.TimerStart(Timer & 0x3, TimerCmpVile[Timer]);
		}
		else
		{
			Driver_PWM1.TimerStop(Timer & 0x3);
		}
		#endif
	}
}
















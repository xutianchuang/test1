#include "gsmcu_m3_pwm.h"
#include "gsmcu_m3_gpio.h"
#include "gsmcu_m3_port.h"

#include "RTE_Device.h" 

#if !(RTE_PWM0||RTE_PWM1)
	#error "PWM not configured in RTE_Components.h!" 
#endif

#define PWM_CLOCK(channel,num)			((num)<<((channel)*4))


#if RTE_PWM0
static PWM_PIN Pwm0_Pin[] = {
	{
		PWM0_CH0_PORT,
		PWM0_CH0_BIT,
		PWM0_CH0_FUNC,
	},
	{
		PWM0_CH1_PORT,
		PWM0_CH1_BIT,
		PWM0_CH1_FUNC,
	},

	{
		PWM0_CH2_PORT,
		PWM0_CH2_BIT,
		PWM0_CH2_FUNC,
	},
	{

		PWM0_CH3_PORT,
		PWM0_CH3_BIT,
		PWM0_CH3_FUNC,
	},
};
static PWM_INFO Pwm0_info={
	0,
	0,
	1,
	{0,0,0,0}
};
static PWM_RESOURCES PWM0_Resources={
	PWM0,
	PWM0_IRQn,
	&IpgClock,
	Pwm0_Pin,
	&Pwm0_info,
};
#endif


#if RTE_PWM1
static PWM_PIN Pwm1_Pin[]={
	{
		PWM1_CH0_PORT,
		PWM1_CH0_BIT,
		PWM1_CH0_FUNC,
	},
	{
		PWM1_CH1_PORT,
		PWM1_CH1_BIT,
		PWM1_CH1_FUNC,
	},

	{
		PWM1_CH2_PORT,
		PWM1_CH2_BIT,
		PWM1_CH2_FUNC,
	},
	{

		PWM1_CH3_PORT,
		PWM1_CH3_BIT,
		PWM1_CH3_FUNC,
	},	 
};
static PWM_INFO Pwm1_info={
	0,
	0,
	1,
	{0,0,0,0}
};
static PWM_RESOURCES PWM1_Resources={
	PWM1,
	PWM1_IRQn,
	&IpgClock,
	Pwm1_Pin,
	&Pwm1_info,
};

#endif

/**
 * Initialize PWM Interface
 * 
 * @param pwm       Pointer to PWM resources
 * @param prescale0 Pre-scale for PWM timer0 timer1
 * @param prescale1 Pre-scale for PWM timer2 timer3
 * 
 * @return execution_status
 */
static int32_t PWM_Initialize(PWM_RESOURCES *pwm, uint32_t prescale0,uint32_t prescale1)
{

	pwm->reg->PPR=PWM_PRESCALE(prescale0,prescale1);
	pwm->info->speed[0] = *pwm->clk/(prescale0+1);
	pwm->info->speed[1] = pwm->info->speed[0];
	pwm->info->speed[2] = *pwm->clk/(prescale1+1);
	pwm->info->speed[3] = pwm->info->speed[2];
	return ARM_DRIVER_OK;
}




/**
 * De-initialize PWM Interface.
 * 
 * @param pwm    Pointer to PWM resources
 * 
 * @return execution_status
 */
static int32_t PWM_UnInitialize(PWM_RESOURCES *pwm)
{
	pwm->reg->PCR=0;
	return ARM_DRIVER_OK;
}

/**
 * Control PWM Interface
 * 
 * @param pwm     pointer to PWM resources
 * @param control operation
 * @param arg     argument of operation
 * 
 * @return execution_status
 */
static int32_t	PWM_Control(PWM_RESOURCES *pwm,uint32_t control, uint32_t arg)
{
	register int channel; 
	uint32_t val;

	volatile PWM_TIMER_CHANNEL_Type	*timer_channel = (volatile PWM_TIMER_CHANNEL_Type*)(&pwm->reg->PCNR0); 
	volatile PWM_PCR_Type *pcr = (volatile PWM_PCR_Type*)(&pwm->reg->PCR);
	
	channel=(control&PWM_CHANNEL_Msk)>> PWM_CHANNEL_Pos;
	switch (control & PWM_DEADZONE_Msk) //set Dead Zone
	{
	case PWM_DZEN0:
		pcr->PCR_b.DZEN0=1;
		if (channel<2)
		{
			pwm->reg->PPR_b.CP0=arg;
		}
		else
		{
			pwm->reg->PPR_b.CP1=arg;
		}
		return ARM_DRIVER_OK;
	case PWM_DZEN1:
		pcr->PCR_b.DZEN1=1;
		if (channel<2)
		{
			pwm->reg->PPR_b.CP0=arg;
		}
		else
		{
			pwm->reg->PPR_b.CP1=arg;
		}
		return ARM_DRIVER_OK;
	case PWM_DZEN_ALL:
		pwm->reg->PCR |= PWM_PCR_DZEN0_Msk | PWM_PCR_DZEN1_Msk;
		pwm->reg->PPR_b.CP1=arg>>8;
		pwm->reg->PPR_b.CP0=arg&0xff;
		return ARM_DRIVER_OK;
	}

	if (control&PWM_HALF_CYCLE_Msk)
	{
		if (pwm->info->speed[channel] == SystemCoreClock  ) 
		{
			return ARM_DRIVER_ERROR;
		}
		pcr[channel].PCR_b.CH_HDU = 1; 
	}
	else
	{
		pcr[channel].PCR_b.CH_HDU = 0; 
	}
	/*  	clock devided   */
	if (control&PWM_CLOCK_SEL_Msk)
	{
		uint32_t num;
		val=control&PWM_CLOCK_SEL_Msk;
		num=pwm->reg->PCSR&(~PWM_CLOCK(channel, 0xF));//(PWM_CLOCK_SEL_Msk<<val);
		pwm->reg->PCSR=num|PWM_CLOCK(channel, val); 
	}


	if (control&PWM_CH_MODE_Msk)
	{
		PORT_PinConfigure(pwm->pin[channel].port ,  pwm->pin[channel].pin ,   (PORT_FUNC)(pwm->pin[channel].func));
		switch (control & PWM_CH_MODE_Msk) //set Dead Zone
		{
			case PWM_CH_MODE_AUTO:
				pcr[channel].PCR_b.CHMOD=1;
				break;
			case PWM_CH_MODE_ONE:	
				pcr[channel].PCR_b.CHMOD=0;
				break;
		}
	}

	switch (control&PWM_INVERTER_Msk)
	{
		case PWM_INVERTER_HIGH:
			pcr[channel].PCR_b.CHINV=0;
			break;
		case PWM_INVERTER_LOW:
			pcr[channel].PCR_b.CHINV=1;
			break;
	}

	
	if (control&PWM_SET_BASE_VAL_Msk)
	{
		timer_channel[channel].PCNR=arg;
	}


	if (control&PWM_PINDDR_Msk)
	{
			//pin set 
		
		switch (control&PWM_PINDDR_Msk)
		{
			case PWM_PINDDR_OUT:
				//config PWM out pin
				pwm->reg->PPCR_b.PDDR |= 0x1<<channel;
				break;
				
			case PWM_PINDDR_IN:
				//config PWM input pin
				pwm->reg->PPCR_b.PDDR &= (~(0x1<<channel))&0x0f;
				break;
		}
	}

	
	return ARM_DRIVER_OK;
}

/**
 * PWM Function start
 * 
 * @param pwm    pointer to PWM resources
 * @param channel PWM which channel
 * @param valve(low 2 Byte/high 2 Byte)  when change latch
 * 
 * @return execution_status
 */
static int32_t	PWM_TimerStart(PWM_RESOURCES *pwm,uint32_t channel, uint32_t valve)
{
	volatile PWM_TIMER_CHANNEL_Type	*timer_channel = (volatile PWM_TIMER_CHANNEL_Type*)(&pwm->reg->PCNR0); 
	volatile PWM_PCR_Type *pcr = (volatile PWM_PCR_Type*)(&pwm->reg->PCR);

	timer_channel[channel].PCMR= valve;
	//timer_channel[channel].PCNR= (valve&0xffff0000)>>16;
	
	
	if (pwm->info->pwm_need_int) 
	{
		pwm->reg->PIER |= 1 << channel;
	}
	pcr[channel].PCR_b.CHEN=1;
	return ARM_DRIVER_OK;
}
/**
 * Stop PWM function
 * 
 * @param pwm    pointer to PWM resources
 * @param channel which channel
 * 
 * @return execution_status
 */
static int32_t PWM_TimerStop(PWM_RESOURCES *pwm,uint32_t channel)
{
	volatile PWM_PCR_Type *pcr = (volatile PWM_PCR_Type*)(&pwm->reg->PCR);
	pcr[channel].PCR_b.CHEN=0;
	return ARM_DRIVER_OK;
}

/**
 * PWM Capture Function start 
 * 
 * @param pwm    pointer to PWM resources
 * @param channel which channel
 * @param latch  Rising Latch or Falling Latch Captrue
 * 
 * @return execution_status
 */
static int32_t	PWM_CaptureStart(PWM_RESOURCES *pwm,uint32_t channel, uint32_t latch)
{
	uint16_t v_PCCR=0;
	volatile PWM_PCR_Type *pcr = (volatile PWM_PCR_Type*)(&pwm->reg->PCR);

// open isr
	NVIC_ClearPendingIRQ(pwm->irqn);
	NVIC_EnableIRQ(pwm->irqn);

// open IER
	if (pwm->info->capture_need_int)
	{
		if (latch & PWM_CAPTURE_FALLING)
		{
			v_PCCR|= PWM_PCCR_FL_IE0_Msk;
		}
		if (latch & PWM_CAPTURE_RISING) 
		{
			v_PCCR|= PWM_PCCR_RL_IE_Msk;
		}
	}
	// clear flag
	pwm->reg->PCCR[channel] |= (PWM_PCCR_CFLRD_Msk|PWM_PCCR_CRLRD_Msk|PWM_PCCR_CAPIF_Msk);
	
	//capture en 
	v_PCCR |= PWM_PCCR_CAPCHEN_Msk;
	pwm->reg->PCCR[channel] = (uint16_t)(~(PWM_PCCR_CFLRD_Msk|PWM_PCCR_CRLRD_Msk|PWM_PCCR_CAPIF_Msk))|v_PCCR;

	//open PWM timer
	pcr[channel].PCR_b.CHEN=1;

	
	return ARM_DRIVER_OK;
}
/**
 * PWM Capture Function stop
 * 
 * @param pwm    pointer to PWM resources
 * @param channel which channel
 * 
 * @return execution_status
 */
static int32_t	PWM_CaptureStop(PWM_RESOURCES *pwm,uint32_t channel)
{

	volatile PWM_PCR_Type *pcr = (volatile PWM_PCR_Type*)(&pwm->reg->PCR);
	pwm->reg->PCCR_b[channel].CAPCHEN=0;
	pcr[channel].PCR_b.CHEN=0;
	NVIC_ClearPendingIRQ(pwm->irqn);
	NVIC_DisableIRQ(pwm->irqn);	
	return ARM_DRIVER_OK;
}

#if RTE_PWM0
static int32_t PWM0_Initialize(uint32_t prescale0, uint32_t prescale1)
{
	return PWM_Initialize(&PWM0_Resources, prescale0, prescale1);
}
static int32_t PWM0_UnInitialize(void)
{
	return PWM_UnInitialize(&PWM0_Resources);
}
static int32_t	PWM0_Control(uint32_t control, uint32_t arg)
{
	return PWM_Control(&PWM0_Resources, control,  arg);
}
static int32_t	PWM0_TimerStart(uint32_t channel, uint32_t valve)
{
	return PWM_TimerStart(&PWM0_Resources, channel,  valve);
}
static int32_t PWM0_TimerStop(uint32_t channel)
{
	return PWM_TimerStop(&PWM0_Resources, channel);
}
static int32_t	PWM0_CaptureStart(uint32_t channel, uint32_t latch)
{
	return PWM_CaptureStart(&PWM0_Resources, channel,  latch);
}
static int32_t	PWM0_CaptureStop(uint32_t channel)
{
	return PWM_CaptureStop(&PWM0_Resources, channel);
}
const PWM_DRIVER Driver_PWM0={
	PWM0_Initialize,
	PWM0_UnInitialize,
	PWM0_Control,
	PWM0_TimerStart,
	PWM0_TimerStop,
	PWM0_CaptureStart,
	PWM0_CaptureStop
};
#endif


#if RTE_PWM1
static int32_t PWM1_Initialize(uint32_t prescale0, uint32_t prescale1)
{
	return PWM_Initialize(&PWM1_Resources, prescale0, prescale1);
}
static int32_t PWM1_UnInitialize(void)
{
	return PWM_UnInitialize(&PWM1_Resources);
}
static int32_t	PWM1_Control(uint32_t control, uint32_t arg)
{
	return PWM_Control(&PWM1_Resources, control,  arg);
}
static int32_t	PWM1_TimerStart(uint32_t channel, uint32_t valve)
{
	return PWM_TimerStart(&PWM1_Resources, channel,  valve);
}
static int32_t PWM1_TimerStop(uint32_t channel)
{
	return PWM_TimerStop(&PWM1_Resources, channel);
}
static int32_t	PWM1_CaptureStart(uint32_t channel, uint32_t latch)
{
	return PWM_CaptureStart(&PWM1_Resources, channel,  latch);
}
static int32_t	PWM1_CaptureStop(uint32_t channel)
{
	return PWM_CaptureStop(&PWM1_Resources, channel);
}
const PWM_DRIVER Driver_PWM1={
	PWM1_Initialize,
	PWM1_UnInitialize,
	PWM1_Control,
	PWM1_TimerStart,
	PWM1_TimerStop,
	PWM1_CaptureStart,
	PWM1_CaptureStop
};
#endif


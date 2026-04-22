/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_port.h"
#include "os.h"
#include "bsp.h"
#include "protocol_includes.h"
void DeviceResetOpen(GPIOx_CALLBACK fun)
{
	GPIO_InitTypeDef GPIO_InitStruct;	
	GPIO_StructInit(&GPIO_InitStruct);
	if (myDevicType == METER)
	{
#if defined(RESET_PORT)
		SCU_GPIOInputConfig(RESET_PORT,RESET_PIN);
		GPIO_InitStruct.GPIO_Pin = RESET_PIN;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_Init(RESET_PORT,&GPIO_InitStruct);
		GPIO_SetBouncePreScale(RESET_PORT, 49);
		GPIO_EnableBounce(RESET_PORT, RESET_PIN);
		//GPIO引脚中断配置
		GPIO_SetIRQTriggerMode(RESET_PORT, RESET_PIN, GPIO_Trigger_Falling);
		
		//注册中断处理函数；开启中断
		if(RESET_PORT == GPIO0)
		{
			BPS_AddGpio0Irq(RESET_PIN,fun);
		}
		else if(RESET_PORT == GPIO1)
		{
			BPS_AddGpio1Irq(RESET_PIN,fun);
		}
		GPIO_SetIRQMode(RESET_PORT, RESET_PIN, ENABLE);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(RESET_PORT_3PHASE)
		SCU_GPIOInputConfig(RESET_PORT_3PHASE,RESET_PIN_3PHASE);
		GPIO_InitStruct.GPIO_Pin = RESET_PIN_3PHASE;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_Init(RESET_PORT_3PHASE,&GPIO_InitStruct);
		GPIO_SetBouncePreScale(RESET_PORT_3PHASE, 49);
		GPIO_EnableBounce(RESET_PORT_3PHASE, RESET_PIN_3PHASE);
		//GPIO引脚中断配置
		GPIO_SetIRQTriggerMode(RESET_PORT_3PHASE, RESET_PIN_3PHASE, GPIO_Trigger_Falling);
		
		//注册中断处理函数；开启中断
		if(RESET_PORT_3PHASE == GPIO0)
		{
			BPS_AddGpio0Irq(RESET_PIN_3PHASE,fun);
		}
		else if(RESET_PORT_3PHASE == GPIO1)
		{
			BPS_AddGpio1Irq(RESET_PIN_3PHASE,fun);
		}
		GPIO_SetIRQMode(RESET_PORT_3PHASE, RESET_PIN_3PHASE, ENABLE);
#endif
	}
}
void ResetInterrupt(FunctionalState open_close)
{
#if defined(RESET_PORT)
	GPIO_SetIRQMode(RESET_PORT, RESET_PIN, open_close);
#endif
#if defined(RESET_PORT_3PHASE)
	GPIO_SetIRQMode(RESET_PORT_3PHASE, RESET_PIN_3PHASE, open_close);
#endif
}
GPIOx_CALLBACK GPIO0_IRQ_TABLE[32];
GPIOx_CALLBACK GPIO1_IRQ_TABLE[32];
void BPS_GPIO0_IRQ(void)
{
	int pins=GPIO_GetIntStatus(GPIO0);
	for (int i=0;i<32;i++)
	{
		if (pins&(1<<i))
		{
			if (GPIO0_IRQ_TABLE[i]!=NULL)
			{
				GPIO0_IRQ_TABLE[i](i);
			}
		}
	}
	GPIO_ClearPortIntStatus(GPIO0,pins);
}

void BPS_GPIO1_IRQ(void)
{
	int pins=GPIO_GetIntStatus(GPIO1);
	for (int i=0;i<32;i++)
	{
		if (pins&(1<<i))
		{
			if (GPIO1_IRQ_TABLE[i]!=NULL)
			{
				GPIO1_IRQ_TABLE[i](i);
			}
		}
	}
	GPIO_ClearPortIntStatus(GPIO1,pins);
}

void BPS_AddGpio0Irq(u32 pin,GPIOx_CALLBACK func)
{
	int idx=0;
	for (idx=0;idx<32;idx++)
	{
		if (pin&(1<<idx))
		{
			break;
		}
	}
	if (idx==32)
	{
		return;
	}
	GPIO0_IRQ_TABLE[idx] = func;
	BSP_IntVectSet(GPIO0_IRQn, BPS_GPIO0_IRQ);
	NVIC_EnableIRQ(GPIO0_IRQn);
}

void BPS_AddGpio1Irq(u32 pin,GPIOx_CALLBACK func)
{
	int idx=0;
	for (idx=0;idx<32;idx++)
	{
		if (pin&(1<<idx))
		{
			break;
		}
	}
	if (idx==32)
	{
		return;
	}
	GPIO1_IRQ_TABLE[idx] = func;
	BSP_IntVectSet(GPIO1_IRQn, BPS_GPIO1_IRQ);
	NVIC_EnableIRQ(GPIO1_IRQn);
}

void StaModeOpen(void)
{
	GPIO_GlobalInit();
#if defined(MODE_PIN1_PORT)
	GPIO_InitTypeDef GPIO_InitStruct;	
	SCU_GPIOInputConfig(MODE_PIN1_PORT,MODE_PIN1);
	GPIO_StructInit(&GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = MODE_PIN1;
#if MODE_PIN1_MODE //外部下拉
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
#else //外部上拉
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD;
#endif
	GPIO_Init(MODE_PIN1_PORT,&GPIO_InitStruct);
#endif
//#if defined(MODE_PIN2_PORT)
//	SCU_GPIOInputConfig(MODE_PIN2_PORT,MODE_PIN2);
//	GPIO_StructInit(&GPIO_InitStruct);
//	GPIO_InitStruct.GPIO_Pin = MODE_PIN2;
//	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
//	GPIO_Init(MODE_PIN2_PORT,&GPIO_InitStruct);
//#endif
}
int GetStaModeValue(void)
{
//#if defined(MODE_PIN1_PORT) && defined(MODE_PIN2_PORT)
#if defined(MODE_PIN1_PORT)
	int mode=GPIO_ReadInputDataBit(MODE_PIN1_PORT,MODE_PIN1);
	//mode|=GPIO_ReadInputDataBit(MODE_PIN2_PORT,MODE_PIN2)<<1;
#if MODE_PIN1_MODE //外部下拉
	return !mode;
#else //外部上拉
	return mode;
#endif
#else
	return 3;
#endif
}
//ouput 0_19
#if DEV_STA
void StaOpen(void)
{
#if  !defined(II_STA)||defined(I_STA)
	GPIO_GlobalInit();
	GPIO_InitTypeDef GPIO_InitStruct;	
	if (myDevicType == METER)
	{
#if defined(STAOUT_PORT)		
		SCU_GPIOInputConfig(STAOUT_PORT,STAOUT_PIN);
		
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = STAOUT_PIN;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPP;
		GPIO_Init(STAOUT_PORT,&GPIO_InitStruct);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(STAOUT_PORT_3PHASE)			
		SCU_GPIOInputConfig(STAOUT_PORT_3PHASE,STAOUT_PIN_3PHASE);
		
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = STAOUT_PIN_3PHASE;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPP;
		GPIO_Init(STAOUT_PORT_3PHASE,&GPIO_InitStruct);		
#endif
	}
#endif
}

void StaWrite(uint32_t mode)
{
#if !defined(II_STA)||defined(I_STA)
	if (myDevicType == METER)
	{
#if defined(STAOUT_PORT)			
		if(mode == 0)
		{
			GPIO_ResetBits(STAOUT_PORT,STAOUT_PIN);
			GPIO_InitTypeDef GPIO_InitStruct;	
			GPIO_InitStruct.GPIO_Pin = STAOUT_PIN;
			GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPP;
			GPIO_Init(STAOUT_PORT,&GPIO_InitStruct);
		}
		else
		{
			GPIO_SetBits(STAOUT_PORT,STAOUT_PIN);
			//od
			GPIO_InitTypeDef GPIO_InitStruct;	
			GPIO_InitStruct.GPIO_Pin = STAOUT_PIN;
			GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
			GPIO_Init(STAOUT_PORT,&GPIO_InitStruct);
		}
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(STAOUT_PORT_3PHASE)			
		if(mode == 0)
		{
			GPIO_ResetBits(STAOUT_PORT_3PHASE,STAOUT_PIN_3PHASE);
			GPIO_InitTypeDef GPIO_InitStruct;	
			GPIO_InitStruct.GPIO_Pin = STAOUT_PIN_3PHASE;
			GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPP;
			GPIO_Init(STAOUT_PORT_3PHASE,&GPIO_InitStruct);
		}
		else
		{
			GPIO_SetBits(STAOUT_PORT_3PHASE,STAOUT_PIN_3PHASE);
			GPIO_InitTypeDef GPIO_InitStruct;	
			GPIO_InitStruct.GPIO_Pin = STAOUT_PIN_3PHASE;
			GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
			GPIO_Init(STAOUT_PORT_3PHASE,&GPIO_InitStruct);
		}
#endif
	}
#endif
}

uint32_t StaRead()
{
	if (myDevicType == METER)
	{
#if defined(STAOUT_PORT)		
		return GPIO_ReadOutputDataBit(STAOUT_PORT,STAOUT_PIN);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(STAOUT_PORT_3PHASE)
		return GPIO_ReadOutputDataBit(STAOUT_PORT_3PHASE,STAOUT_PIN_3PHASE);
#endif
	}

	return -1;
}

void StaClose(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_StructInit(&GPIO_InitStruct);	
	if (myDevicType == METER)
	{
#if defined(STAOUT_PORT)
		GPIO_InitStruct.GPIO_Pin = STAOUT_PIN;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(STAOUT_PORT,&GPIO_InitStruct);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(STAOUT_PORT_3PHASE)		
		GPIO_InitStruct.GPIO_Pin = STAOUT_PIN_3PHASE;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(STAOUT_PORT_3PHASE,&GPIO_InitStruct);
#endif
	}
}

extern OS_SEM EventNotificationSem;
extern int power;
void EventIRQHandle(unsigned int arg)
{
	OS_ERR err;
	if(!power)
	{
#if defined(EVENT_PORT)
		GPIO_ClearIRQStatus(EVENT_PORT, EVENT_PIN);
#endif
#if defined(EVENT_PORT_3PHASE)
		GPIO_ClearIRQStatus(EVENT_PORT_3PHASE, EVENT_PIN_3PHASE);
#endif
		return;
	}
	if (myDevicType == METER)
	{
#if defined(EVENT_PORT)
		if(GPIO_ReadInputDataBit(EVENT_PORT, EVENT_PIN) == Bit_SET)
		{
			//通过信号量通知EventTask任务处理
			OSSemPost(&EventNotificationSem, OS_OPT_POST_ALL, &err);
			GPIO_SetIRQTriggerMode(EVENT_PORT, EVENT_PIN, GPIO_Trigger_Rising);//进入一次之后修改为边沿触发
			//清除中断状态
			GPIO_ClearIRQStatus(EVENT_PORT, EVENT_PIN);
		}
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(EVENT_PORT_3PHASE)		
		if(GPIO_ReadInputDataBit(EVENT_PORT_3PHASE, EVENT_PIN_3PHASE) == Bit_SET)
		{
			//通过信号量通知EventTask任务处理
			OSSemPost(&EventNotificationSem, OS_OPT_POST_ALL, &err);
			GPIO_SetIRQTriggerMode(EVENT_PORT, EVENT_PIN, GPIO_Trigger_Rising);//进入一次之后修改为边沿触发
			//清除中断状态
			GPIO_ClearIRQStatus(EVENT_PORT_3PHASE, EVENT_PIN_3PHASE);
		}	
#endif
	}
}

//input 0_17
void EventOpen(void)
{
#if !defined(II_STA)||defined(I_STA)
	GPIO_InitTypeDef GPIO_InitStruct;
	//GPIO初始化
	GPIO_GlobalInit();	
	if (myDevicType == METER)
	{
#if defined(EVENT_PORT)		
		SCU_GPIOInputConfig(EVENT_PORT,EVENT_PIN);
		//GPIO引脚配置
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = EVENT_PIN;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_Init(EVENT_PORT,&GPIO_InitStruct);
		GPIO_SetBouncePreScale(EVENT_PORT, 49);
        GPIO_EnableBounce(EVENT_PORT, EVENT_PIN);
		//GPIO引脚中断配置
		GPIO_SetIRQTriggerMode(EVENT_PORT, EVENT_PIN, GPIO_Trigger_Rising);
		
		//注册中断处理函数；开启中断
//		BSP_IntVectSet(EVENT_PORT_IRQn, EventIRQHandle);
//		NVIC_ClearPendingIRQ(EVENT_PORT_IRQn);
//		NVIC_EnableIRQ(EVENT_PORT_IRQn);
		if(EVENT_PORT == GPIO0)
		{
			BPS_AddGpio0Irq(EVENT_PIN,EventIRQHandle);
		}
		else if(EVENT_PORT == GPIO1)
		{
			BPS_AddGpio1Irq(EVENT_PIN,EventIRQHandle);
		}		
		GPIO_SetIRQMode(EVENT_PORT, EVENT_PIN, ENABLE);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(EVENT_PORT_3PHASE)			
		SCU_GPIOInputConfig(EVENT_PORT_3PHASE,EVENT_PIN_3PHASE);
		//GPIO引脚配置
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = EVENT_PIN_3PHASE;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_Init(EVENT_PORT_3PHASE,&GPIO_InitStruct);
		GPIO_SetBouncePreScale(EVENT_PORT_3PHASE, 49);
        GPIO_EnableBounce(EVENT_PORT_3PHASE, EVENT_PIN_3PHASE);
		//GPIO引脚中断配置
		GPIO_SetIRQTriggerMode(EVENT_PORT_3PHASE, EVENT_PIN_3PHASE, GPIO_Trigger_Rising);
		
		//注册中断处理函数；开启中断
//		BSP_IntVectSet(EVENT_PORT_3PHASE_IRQn, EventIRQHandle);
//		NVIC_ClearPendingIRQ(EVENT_PORT_3PHASE_IRQn);
//		NVIC_EnableIRQ(EVENT_PORT_3PHASE_IRQn);
		if(EVENT_PORT == GPIO0)
		{
			BPS_AddGpio0Irq(EVENT_PIN_3PHASE,EventIRQHandle);
		}
		else if(EVENT_PORT == GPIO1)
		{
			BPS_AddGpio1Irq(EVENT_PIN_3PHASE,EventIRQHandle);
		}
		GPIO_SetIRQMode(EVENT_PORT_3PHASE, EVENT_PIN_3PHASE, ENABLE);
#endif
	}
#endif
}

void EventWrite(uint32_t mode)
{
	if (myDevicType == METER)
	{
#if defined(EVENT_PORT)
		if(mode == 0)
		{
			GPIO_ResetBits(EVENT_PORT,EVENT_PIN);
		}
		else
		{
			GPIO_SetBits(EVENT_PORT,EVENT_PIN);
		}
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(EVENT_PORT_3PHASE)
		if(mode == 0)
		{
			GPIO_ResetBits(EVENT_PORT_3PHASE,EVENT_PIN_3PHASE);
		}
		else
		{
			GPIO_SetBits(EVENT_PORT_3PHASE,EVENT_PIN_3PHASE);
		}
#endif
	}
}

uint32_t EventRead()
{
#if !defined(II_STA)||defined(I_STA)
	if (myDevicType == METER)
	{
#if defined(EVENT_PORT)
		return GPIO_ReadInputDataBit(EVENT_PORT,EVENT_PIN);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(EVENT_PORT_3PHASE)
		return GPIO_ReadInputDataBit(EVENT_PORT_3PHASE,EVENT_PIN_3PHASE);
#endif
	}

	return -1;
#else
	return 0;
#endif
}

void EventClose(void)
{
#if !defined(II_STA)||defined(I_STA)
	if (myDevicType == METER)
	{
#if defined(EVENT_PORT)		
		GPIO_SetIRQMode(EVENT_PORT, EVENT_PIN, DISABLE);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(EVENT_PORT_3PHASE)			
		GPIO_SetIRQMode(EVENT_PORT_3PHASE, EVENT_PIN_3PHASE, DISABLE);
#endif
	}
#endif
}
void EventReopen(void)
{
#if !defined(II_STA)||defined(I_STA)
	if (myDevicType == METER)
	{
#if defined(EVENT_PORT)		
		GPIO_SetIRQMode(EVENT_PORT, EVENT_PIN, ENABLE);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(EVENT_PORT_3PHASE)			
		GPIO_SetIRQMode(EVENT_PORT_3PHASE, EVENT_PIN_3PHASE, ENABLE);
#endif
	}
#endif
}
void SetOpen(void)
{
#if !defined(II_STA)||defined(I_STA)
	GPIO_GlobalInit();
	if (myDevicType == METER)
	{
#if defined(SET_PORT)
		GPIO_InitTypeDef GPIO_InitStruct;
		SCU_GPIOInputConfig(SET_PORT,SET_PIN);
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = SET_PIN;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_Init(SET_PORT,&GPIO_InitStruct);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(SET_PORT_3PHASE)
		GPIO_InitTypeDef GPIO_InitStruct;
		SCU_GPIOInputConfig(SET_PORT_3PHASE,SET_PIN_3PHASE);
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = SET_PIN_3PHASE;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_Init(SET_PORT_3PHASE,&GPIO_InitStruct);
#endif
	}
#endif
}

void SetWrite(uint32_t mode)
{
#if !defined(II_STA)||defined(I_STA)
	if (myDevicType == METER)
	{
#if defined(SET_PORT)		
		if(mode == 0)
		{
			GPIO_ResetBits(SET_PORT,SET_PIN);
		}
		else
		{
			GPIO_SetBits(SET_PORT,SET_PIN);
		}
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(SET_PORT_3PHASE)
		if(mode == 0)
		{
			GPIO_ResetBits(SET_PORT_3PHASE,SET_PIN_3PHASE);
		}
		else
		{
			GPIO_SetBits(SET_PORT_3PHASE,SET_PIN_3PHASE);
		}
#endif
	}
#endif
}

uint32_t SetRead()
{
#if !defined(II_STA)||defined(I_STA)
	if (myDevicType == METER)
	{
#if defined(SET_PORT)
		return GPIO_ReadOutputDataBit(SET_PORT,SET_PIN);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(SET_PORT_3PHASE)
		return GPIO_ReadOutputDataBit(SET_PORT_3PHASE,SET_PIN_3PHASE);
#endif
	}
	
	return -1;
#else
	return 1;
#endif
}

void SetClose(void)
{
}
void InsertOpen(void)
{
#if !defined(II_STA)||defined(I_STA)
	GPIO_GlobalInit();
	GPIO_InitTypeDef GPIO_InitStruct;
	if (myDevicType == METER)
	{
#if defined(PULLOUT_PORT)
		SCU_GPIOInputConfig(PULLOUT_PORT,PULLOUT_PIN);
		
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = PULLOUT_PIN;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_Init(PULLOUT_PORT,&GPIO_InitStruct);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(PULLOUT_PORT_3PHASE)
		SCU_GPIOInputConfig(PULLOUT_PORT_3PHASE,PULLOUT_PIN_3PHASE);
		
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = PULLOUT_PIN_3PHASE;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_Init(PULLOUT_PORT_3PHASE,&GPIO_InitStruct);	
#endif
	}
#endif
}
void InsertTopoFuncOpen(void)
{
	if (myDevicType == METER)
	{
#if defined(TOPO_PORT)
		GPIO_GlobalInit();
		GPIO_InitTypeDef GPIO_InitStruct;
	
		SCU_ADCGPIODriving(1);
		SCU_GPIOInputConfig(TOPO_PORT, TOPO_PIN);
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = TOPO_PIN;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPD;
		GPIO_Init(TOPO_PORT, &GPIO_InitStruct);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(TOPO_PORT_3PHASE)
		GPIO_GlobalInit();
		GPIO_InitTypeDef GPIO_InitStruct;
	
		SCU_ADCGPIODriving(1);
		SCU_GPIOInputConfig(TOPO_PORT_3PHASE, TOPO_PIN_3PHASE);
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = TOPO_PIN_3PHASE;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPD;
		GPIO_Init(TOPO_PORT_3PHASE, &GPIO_InitStruct);	
#endif
	}
}

uint32_t InsertRead(void)
{
#if !defined(II_STA)||defined(I_STA)
	if (myDevicType == METER)
	{
#if defined(PULLOUT_PORT)
		return GPIO_ReadInputDataBit(PULLOUT_PORT,PULLOUT_PIN);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(PULLOUT_PORT_3PHASE)
		return GPIO_ReadInputDataBit(PULLOUT_PORT_3PHASE,PULLOUT_PIN_3PHASE);
#endif
	}
	
	return -1;
#else
	return 1;
#endif
}
void InsertTopoFuncWrite(BitAction vaile)
{
	if (myDevicType == METER)
	{
#if defined(TOPO_PORT)
		if (vaile==Bit_SET)
		{
			GPIO_SetBits(TOPO_PORT, TOPO_PIN);
		}
		else
		{
			GPIO_ResetBits(TOPO_PORT, TOPO_PIN);
		}
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(TOPO_PORT_3PHASE)
		if (vaile == Bit_SET)
		{
			GPIO_SetBits(TOPO_PORT_3PHASE, TOPO_PIN_3PHASE);
		}
		else
		{
			GPIO_ResetBits(TOPO_PORT_3PHASE, TOPO_PIN_3PHASE);
		}
#endif
	}
	return ;
}
void MultiOpen(void)
{
	GPIO_GlobalInit();
	if (myDevicType == METER)
	{
#if defined(MULTI_PORT)
		GPIO_InitTypeDef GPIO_InitStruct;
		SCU_GPIOInputConfig(MULTI_PORT, MULTI_PIN);
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = MULTI_PIN;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(MULTI_PORT, &GPIO_InitStruct);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(MULTI_PORT_3PHASE)
		GPIO_InitTypeDef GPIO_InitStruct;
		SCU_GPIOInputConfig(MULTI_PORT_3PHASE, MULTI_PIN_3PHASE);
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Pin = MULTI_PIN_3PHASE;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(MULTI_PORT_3PHASE, &GPIO_InitStruct);
#endif
	}
}


#elif DEV_CCO
void TxSwitchOpen(void)
{
	GPIO_GlobalInit();
	GPIO_InitTypeDef GPIO_InitStruct;	
	GPIO_StructInit(&GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPD;	
#if defined(SWITCH_A_TX_PORT)
	SCU_GPIOInputConfig(SWITCH_A_TX_PORT,SWITCH_A_TX_PIN);
	GPIO_InitStruct.GPIO_Pin = SWITCH_A_TX_PIN;
	GPIO_Init(SWITCH_A_TX_PORT,&GPIO_InitStruct);
	GPIO_ResetBits(SWITCH_A_TX_PORT,SWITCH_A_TX_PIN);
#endif
#if defined(SWITCH_B_TX_PORT)
	SCU_GPIOInputConfig(SWITCH_B_TX_PORT,SWITCH_B_TX_PIN);
	GPIO_InitStruct.GPIO_Pin = SWITCH_B_TX_PIN;
	GPIO_Init(SWITCH_B_TX_PORT,&GPIO_InitStruct);
	GPIO_ResetBits(SWITCH_B_TX_PORT,SWITCH_B_TX_PIN);
#endif
#if defined(SWITCH_C_TX_PORT)
	SCU_GPIOInputConfig(SWITCH_C_TX_PORT,SWITCH_C_TX_PIN);
	GPIO_InitStruct.GPIO_Pin = SWITCH_C_TX_PIN;
	GPIO_Init(SWITCH_C_TX_PORT,&GPIO_InitStruct);
	GPIO_ResetBits(SWITCH_C_TX_PORT,SWITCH_C_TX_PIN);
#endif
}

void TxSwitchSet(bool flg, u8 phase)
{
	if(phase == BPS_PHASE_A)
	{
#if defined(SWITCH_A_TX_PORT)
		if(flg){GPIO_SetBits(SWITCH_A_TX_PORT,SWITCH_A_TX_PIN);}
		else{GPIO_ResetBits(SWITCH_A_TX_PORT,SWITCH_A_TX_PIN);}
#endif
	}
	else if(phase == BPS_PHASE_B)
	{
#if defined(SWITCH_B_TX_PORT)
		if(flg){GPIO_SetBits(SWITCH_B_TX_PORT,SWITCH_B_TX_PIN);}
		else{GPIO_ResetBits(SWITCH_B_TX_PORT,SWITCH_B_TX_PIN);}
#endif
	}
	else if(phase == BPS_PHASE_C)
	{
#if defined(SWITCH_C_TX_PORT)
		if(flg){GPIO_SetBits(SWITCH_C_TX_PORT,SWITCH_C_TX_PIN);}
		else{GPIO_ResetBits(SWITCH_C_TX_PORT,SWITCH_C_TX_PIN);}
#endif
	}
}

void RxSwitchOpen(void)
{
	GPIO_GlobalInit();
	GPIO_InitTypeDef GPIO_InitStruct;	
	GPIO_StructInit(&GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPD;	
#if defined(SWITCH_A_RX_PORT)
	SCU_GPIOInputConfig(SWITCH_A_RX_PORT,SWITCH_A_RX_PIN);
	GPIO_InitStruct.GPIO_Pin = SWITCH_A_RX_PIN;
	GPIO_Init(SWITCH_A_RX_PORT,&GPIO_InitStruct);
	GPIO_ResetBits(SWITCH_A_RX_PORT,SWITCH_A_RX_PIN);
#endif
#if defined(SWITCH_B_RX_PORT)
	SCU_GPIOInputConfig(SWITCH_B_RX_PORT,SWITCH_B_RX_PIN);
	GPIO_InitStruct.GPIO_Pin = SWITCH_B_RX_PIN;
	GPIO_Init(SWITCH_B_RX_PORT,&GPIO_InitStruct);
	GPIO_ResetBits(SWITCH_B_RX_PORT,SWITCH_B_RX_PIN);
#endif
#if defined(SWITCH_C_RX_PORT)
	SCU_GPIOInputConfig(SWITCH_C_RX_PORT,SWITCH_C_RX_PIN);
	GPIO_InitStruct.GPIO_Pin = SWITCH_C_RX_PIN;
	GPIO_Init(SWITCH_C_RX_PORT,&GPIO_InitStruct);
	GPIO_ResetBits(SWITCH_C_RX_PORT,SWITCH_C_RX_PIN);
#endif
}

void RxSwitchSet(bool flg, uint32_t phase)
{
	if(phase == BPS_PHASE_A)
	{
#if defined(SWITCH_A_RX_PORT)
		if(flg){GPIO_SetBits(SWITCH_A_RX_PORT,SWITCH_A_RX_PIN);}
		else{GPIO_ResetBits(SWITCH_A_RX_PORT,SWITCH_A_RX_PIN);}
#endif
	}
	else if(phase == BPS_PHASE_B)
	{
#if defined(SWITCH_B_RX_PORT)
		if(flg){GPIO_SetBits(SWITCH_B_RX_PORT,SWITCH_B_RX_PIN);}
		else{GPIO_ResetBits(SWITCH_B_RX_PORT,SWITCH_B_RX_PIN);}
#endif
	}
	else if(phase == BPS_PHASE_C)
	{
#if defined(SWITCH_C_RX_PORT)
		if(flg){GPIO_SetBits(SWITCH_C_RX_PORT,SWITCH_C_RX_PIN);}
		else{GPIO_ResetBits(SWITCH_C_RX_PORT,SWITCH_C_RX_PIN);}
#endif
	}
}

#endif
void HplcLineDriverOpen(void)
{
	GPIO_GlobalInit();
	GPIO_InitTypeDef GPIO_InitStruct;	
	GPIO_StructInit(&GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPU;	
	if (myDevicType == METER || myDevicType == CONCENTRATOR)
	{
#if defined(LD_PORT)
		SCU_GPIOInputConfig(LD_PORT,LD_PIN);
		
		GPIO_InitStruct.GPIO_Pin = LD_PIN;
		GPIO_Init(LD_PORT,&GPIO_InitStruct);
		GPIO_SetBits(LD_PORT,LD_PIN);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(LD_PORT_3PHASE)		
		SCU_GPIOInputConfig(LD_PORT_3PHASE,LD_PIN_3PHASE);
		
		GPIO_InitStruct.GPIO_Pin = LD_PIN_3PHASE;
		GPIO_Init(LD_PORT_3PHASE,&GPIO_InitStruct);
		GPIO_SetBits(LD_PORT_3PHASE,LD_PIN_3PHASE);
#endif
	}
}


void HplcSetLineDriver(bool flg)
{
	if (myDevicType == METER || myDevicType == CONCENTRATOR)
	{
#if defined(LD_PORT)
		if(flg)
		{
			GPIO_SetBits(LD_PORT,LD_PIN);
		}
		else
		{
			GPIO_ResetBits(LD_PORT,LD_PIN);
		}
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(LD_PORT_3PHASE)			
		if(flg)
		{
			GPIO_SetBits(LD_PORT_3PHASE,LD_PIN_3PHASE);
		}
		else
		{
			GPIO_ResetBits(LD_PORT_3PHASE,LD_PIN_3PHASE);
		}
#endif
	}
}






void U485EnOpen(void)
{
#ifdef II_STA
	GPIO_InitTypeDef GPIO_InitStruct;
	//GPIO初始化
	GPIO_GlobalInit();

	SCU_GPIOInputConfig(U485_EN_PORT, U485_EN_PIN);
	//GPIO引脚配置
	GPIO_StructInit(&GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = U485_EN_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPP;
	GPIO_Init(U485_EN_PORT, &GPIO_InitStruct);
	GPIO_SetBouncePreScale(U485_EN_PORT, 49);
	GPIO_EnableBounce(U485_EN_PORT, U485_EN_PIN);
#endif
}

void U485EnWrite(uint32_t mode)
{
#ifdef II_STA
	if (mode == 0)
	{
		GPIO_ResetBits(U485_EN_PORT, U485_EN_PIN);
	}
	else
	{
		GPIO_SetBits(U485_EN_PORT, U485_EN_PIN);
	}
#endif
}
//===================



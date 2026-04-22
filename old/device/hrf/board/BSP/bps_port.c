/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_port.h"
#include "os.h"
#include "bsp.h"
#include "protocol_includes.h"


GPIOx_CALLBACK GPIO0_IRQ_TABLE[sizeof(port_array)/sizeof(EPORT_Type *)][8];

void BPS_GPIOx_IRQ(int port)
{
	int pins=GPIO_Port_Irq_Status(port);
	for (int i=0;i<32;i++)
	{
		if (pins&(1<<i))
		{
			if (GPIO0_IRQ_TABLE[port][i]!=NULL)
			{
				GPIO0_IRQ_TABLE[port][i](i);
			}
			GPIO_Clear_Irq(port,i);
		}
	}
}

void BPS_GPIO0_IRQ(void)
{
	BPS_GPIOx_IRQ(0);
}
void BPS_GPIO1_IRQ(void)
{
	BPS_GPIOx_IRQ(1);
}
void BPS_GPIO2_IRQ(void)
{
	BPS_GPIOx_IRQ(2);
}
void BPS_GPIO3_IRQ(void)
{
	BPS_GPIOx_IRQ(3);
}
void BPS_GPIO4_IRQ(void)
{
	BPS_GPIOx_IRQ(4);
}
void BPS_GPIO5_IRQ(void)
{
	BPS_GPIOx_IRQ(5);
}
void BPS_GPIO6_IRQ(void)
{
	BPS_GPIOx_IRQ(6);
}
void BPS_GPIO7_IRQ(void)
{
	BPS_GPIOx_IRQ(7);
}
void BPS_GPIO8_IRQ(void)
{
	BPS_GPIOx_IRQ(8);
}
void BPS_GPIO9_IRQ(void)
{
	BPS_GPIOx_IRQ(9);
}

void BPS_AddGpioIrq(u32 port,u32 pin,GPIOx_CALLBACK func)
{
	if (port>(sizeof(port_array)/sizeof(EPORT_Type *)))
	{
		__BKPT(1);
	}
	if (pin>8)
	{
		__BKPT(1);
	}
	GPIO0_IRQ_TABLE[port][pin] = func;
	NVIC_EnableIRQ((IRQn_Type)(EPORT0_IRQn+port));
}

void DeviceResetOpen(GPIOx_CALLBACK fun)
{
	if (myDevicType == METER)
	{
#if defined(RESET_PORT)
		GPIO_SetPinDir(RESET_PORT,RESET_PIN, GPIO_INPUT); //
		PORT_PinConfigure(RESET_PORT,RESET_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_Port_settrigger(RESET_PORT,RESET_PIN, GPIO_FALLING_TRIGER);
		BPS_AddGpioIrq(RESET_PORT,RESET_PIN,fun);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(RESET_PORT_3PHASE)
		GPIO_SetPinDir(RESET_PORT_3PHASE,RESET_PIN_3PHASE, GPIO_INPUT); //
		PORT_PinConfigure(RESET_PORT_3PHASE,RESET_PIN_3PHASE, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_Port_settrigger(RESET_PORT_3PHASE,RESET_PIN_3PHASE, GPIO_FALLING_TRIGER);
		BPS_AddGpioIrq(RESET_PORT_3PHASE,RESET_PIN_3PHASE,fun);
#endif
	}
}

void ResetInterrupt(FunctionalState open_close)
{
#if defined(RESET_PORT)
	if (open_close==0)
	{
		GPIO_DisableIrq(RESET_PORT, RESET_PIN);
	}
	else
	{
		GPIO_Port_settrigger(RESET_PORT,RESET_PIN, GPIO_FALLING_TRIGER);
	}
#endif
#if defined(RESET_PORT_3PHASE)
	if (open_close==0)
	{
		GPIO_DisableIrq(RESET_PORT_3PHASE, RESET_PIN_3PHASE);
	}
	else
	{
		GPIO_Port_settrigger(RESET_PORT_3PHASE, RESET_PIN_3PHASE, GPIO_FALLING_TRIGER);
	}
#endif
}

void StaModeOpen(void)
{
#if defined(MODE_PIN1_PORT)
	GPIO_SetPinDir(MODE_PIN1_PORT,MODE_PIN1, GPIO_INPUT); //
#if MODE_PIN1_MODE //外部下拉
	PORT_PinConfigure(MODE_PIN1_PORT,MODE_PIN1, (PORT_FUNC)(PORT_FUNC_GPIO | PORT_PULLUP));
#else  //外部上拉
	PORT_PinConfigure(MODE_PIN1_PORT,MODE_PIN1, (PORT_FUNC)(PORT_FUNC_GPIO | PORT_PULLDOWN));
#endif
#endif

}

int GetStaModeValue(void)
{
#if defined(MODE_PIN1_PORT)
	int mode=GPIO_PinRead(MODE_PIN1_PORT,MODE_PIN1);

#if MODE_PIN1_MODE //外部下拉
	return !mode;
#else //外部上拉
	return mode;
#endif

#elif defined(CCO_DEVICE)
	return 0;
#else
	return 3;
#endif
}

void StaOpen(void)
{
#if  !defined(II_STA)||defined(I_STA)
	if (myDevicType == METER)
	{
#if defined(STAOUT_PORT)

		GPIO_SetPinDir(STAOUT_PORT, STAOUT_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(STAOUT_PORT, STAOUT_PIN, (PORT_FUNC)(PORT_CFG_MODE(2) | PORT_PULLUP | PORT_OPEN_DRAIN));
		GPIO_PinWrite(STAOUT_PORT, STAOUT_PIN, 0);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(STAOUT_PORT_3PHASE)

		GPIO_SetPinDir(STAOUT_PORT_3PHASE, STAOUT_PIN_3PHASE, GPIO_OUTPUT); //
		PORT_PinConfigure(STAOUT_PORT_3PHASE, STAOUT_PIN_3PHASE, (PORT_FUNC)(PORT_CFG_MODE(2) | PORT_PULLUP | PORT_OPEN_DRAIN));
		GPIO_PinWrite(STAOUT_PORT_3PHASE, STAOUT_PIN_3PHASE, 0);
#endif
	}
#endif
}

void StaWrite(uint32_t mode)
{
#if  !defined(II_STA)||defined(I_STA)
	if (myDevicType == METER)
	{
#if defined(STAOUT_PORT)
		GPIO_PinWrite(STAOUT_PORT, STAOUT_PIN, mode);
#endif
	}
	else if (myDevicType == METER_3PHASE)
	{
#if defined(STAOUT_PORT_3PHASE)
		GPIO_PinWrite(STAOUT_PORT_3PHASE, STAOUT_PIN_3PHASE, mode);
#endif
	}
#endif
}




extern OS_SEM EventNotificationSem;
extern int power;
void EventIRQHandle(unsigned int arg)
{
	if(!power)
	{
		return;
	}
	if (myDevicType == METER)
	{
#if defined(EVENT_PORT)
		if(GPIO_PinRead(EVENT_PORT, EVENT_PIN) == 1)
		{
			OS_ERR err;

			//通过信号量通知EventTask任务处理
			OSSemPost(&EventNotificationSem, OS_OPT_POST_ALL, &err);
			GPIO_Port_settrigger(EVENT_PORT, EVENT_PIN, GPIO_RISING_TRIGER);
		}
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(EVENT_PORT_3PHASE)		
		if(GPIO_PinRead(EVENT_PORT_3PHASE, EVENT_PIN_3PHASE) == 1)
		{
			OS_ERR err;
			
			//通过信号量通知EventTask任务处理
			OSSemPost(&EventNotificationSem, OS_OPT_POST_ALL, &err);
			GPIO_Port_settrigger(EVENT_PORT_3PHASE, EVENT_PIN_3PHASE, GPIO_RISING_TRIGER);
		}	
#endif
	}
}

//input 0_17
void EventOpen(void)
{
#if  !defined(II_STA)||defined(I_STA)
	//GPIO初始化

	if (myDevicType == METER)
	{
#if defined(EVENT_PORT)		

		GPIO_SetPinDir(EVENT_PORT, EVENT_PIN, GPIO_INPUT); //
		PORT_PinConfigure(EVENT_PORT, EVENT_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_Port_settrigger(EVENT_PORT, EVENT_PIN, GPIO_RISING_TRIGER);
		BPS_AddGpioIrq(EVENT_PORT, EVENT_PIN,EventIRQHandle);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(EVENT_PORT_3PHASE)		
	
		GPIO_SetPinDir(EVENT_PORT_3PHASE,EVENT_PIN_3PHASE, GPIO_INPUT); //
		PORT_PinConfigure(EVENT_PORT_3PHASE,EVENT_PIN_3PHASE, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_Port_settrigger(EVENT_PORT_3PHASE,EVENT_PIN_3PHASE, GPIO_RISING_TRIGER);
		BPS_AddGpioIrq(EVENT_PORT_3PHASE,EVENT_PIN_3PHASE,EventIRQHandle);	
#endif
	}
#endif
}



uint32_t EventRead()
{
#if !defined(II_STA)
	if (myDevicType == METER)
	{
#if defined(EVENT_PORT)
		return GPIO_PinRead(EVENT_PORT,EVENT_PIN);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(EVENT_PORT_3PHASE)
		return GPIO_PinRead(EVENT_PORT_3PHASE,EVENT_PIN_3PHASE);
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
		GPIO_DisableIrq(EVENT_PORT, EVENT_PIN);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(EVENT_PORT_3PHASE)			
		GPIO_DisableIrq(EVENT_PORT_3PHASE, EVENT_PIN_3PHASE);
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
		GPIO_Port_settrigger(EVENT_PORT, EVENT_PIN, GPIO_RISING_TRIGER);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(EVENT_PORT_3PHASE)			
		GPIO_Port_settrigger(EVENT_PORT_3PHASE,EVENT_PIN_3PHASE, GPIO_RISING_TRIGER);
#endif
	}
#endif
}

void SetOpen(void)
{
#if !defined(II_STA)||defined(I_STA)
	if (myDevicType == METER)
	{
#if defined(SET_PORT)
		GPIO_SetPinDir(SET_PORT,SET_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(SET_PORT,SET_PIN, (PORT_FUNC)(PORT_CFG_MODE(2) | PORT_PULLUP | PORT_OPEN_DRAIN));
		GPIO_PinWrite(SET_PORT,SET_PIN, 0);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(SET_PORT_3PHASE)

		GPIO_SetPinDir(SET_PORT_3PHASE,SET_PIN_3PHASE, GPIO_OUTPUT); //
		PORT_PinConfigure(SET_PORT_3PHASE,SET_PIN_3PHASE, (PORT_FUNC)(PORT_CFG_MODE(2) | PORT_PULLUP | PORT_OPEN_DRAIN));
		GPIO_PinWrite(SET_PORT_3PHASE,SET_PIN_3PHASE, 0);
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
		GPIO_PinWrite(SET_PORT,SET_PIN,mode);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(SET_PORT_3PHASE)
		GPIO_PinWrite(SET_PORT_3PHASE,SET_PIN_3PHASE,mode);
#endif
	}
#endif
}


void InsertOpen(void)
{
#ifndef II_STA

	if (myDevicType == METER)
	{
#if defined(PULLOUT_PORT)
		GPIO_SetPinDir(PULLOUT_PORT,PULLOUT_PIN, GPIO_INPUT); //
		PORT_PinConfigure(PULLOUT_PORT,PULLOUT_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)|PORT_PULLUP));
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(PULLOUT_PORT_3PHASE)
		GPIO_SetPinDir(PULLOUT_PORT_3PHASE,PULLOUT_PIN_3PHASE, GPIO_INPUT); //
		PORT_PinConfigure(PULLOUT_PORT_3PHASE,PULLOUT_PIN_3PHASE, (PORT_FUNC)(PORT_CFG_MODE(2)|PORT_PULLUP));
#endif
	}
#endif
}

void InsertTopoFuncOpen(void)
{
#ifndef II_STA
	if (myDevicType == METER)
	{
#if defined(TOPO_PORT)
		GPIO_SetPinDir(TOPO_PORT, TOPO_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(TOPO_PORT, TOPO_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)|PORT_PULLUP));

		
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(TOPO_PORT_3PHASE)

		GPIO_SetPinDir(TOPO_PORT_3PHASE, TOPO_PIN_3PHASE, GPIO_OUTPUT); //
		PORT_PinConfigure(TOPO_PORT_3PHASE, TOPO_PIN_3PHASE, (PORT_FUNC)(PORT_CFG_MODE(2)|PORT_PULLUP));
#endif
	}
#endif
}

uint32_t InsertRead(void)
{
#ifndef II_STA
	if (myDevicType == METER)
	{
#if defined(PULLOUT_PORT)
		return GPIO_PinRead(PULLOUT_PORT,PULLOUT_PIN);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(PULLOUT_PORT_3PHASE)
		return GPIO_PinRead(PULLOUT_PORT_3PHASE,PULLOUT_PIN_3PHASE);
#endif
	}
	
	return -1;
#else
	return 1;
#endif
}

void InsertTopoFuncWrite(u8 vaile)
{
#ifndef II_STA
	if (myDevicType == METER)
	{
#if defined(TOPO_PORT)
		GPIO_PinWrite(TOPO_PORT, TOPO_PIN,vaile);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(TOPO_PORT_3PHASE)
		GPIO_PinWrite(TOPO_PORT_3PHASE, TOPO_PIN_3PHASE,vaile);
#endif
	}
	return ;
#endif
}

void MultiOpen(void)
{
	if (myDevicType == METER)
	{
#if defined(MULTI_PORT)
		GPIO_SetPinDir(MULTI_PORT, MULTI_PIN, GPIO_INPUT);
		PORT_PinConfigure(MULTI_PORT, MULTI_PIN, (PORT_FUNC)(PORT_FUNC_GPIO));
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(MULTI_PORT_3PHASE)
		GPIO_SetPinDir(MULTI_PORT_3PHASE, MULTI_PIN_3PHASE, GPIO_INPUT);
		PORT_PinConfigure(MULTI_PORT_3PHASE, MULTI_PIN_3PHASE, (PORT_FUNC)(PORT_FUNC_GPIO));
#endif
	}
}

void CapEnCtrOpen(void)
{
	//初始化引脚后，默认使能超容的充电
	if (myDevicType == METER)
	{
#if defined(EN_CTR_PORT)
		GPIO_SetPinDir(EN_CTR_PORT, EN_CTR_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(EN_CTR_PORT, EN_CTR_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(EN_CTR_PORT, EN_CTR_PIN, 1);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(EN_CTR_PORT_3PHASE)		
		GPIO_SetPinDir(EN_CTR_PORT_3PHASE, EN_CTR_PIN_3PHASE, GPIO_OUTPUT); //
		PORT_PinConfigure(EN_CTR_PORT_3PHASE, EN_CTR_PIN_3PHASE, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(EN_CTR_PORT_3PHASE, EN_CTR_PIN_3PHASE, 1);
#endif
	}
}

void CapEnCtrWrite(u32 charge)
{
	if (myDevicType == METER)
	{
#if defined(EN_CTR_PORT)
		GPIO_PinWrite(EN_CTR_PORT, EN_CTR_PIN, charge);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(EN_CTR_PORT_3PHASE)		
		GPIO_PinWrite(EN_CTR_PORT_3PHASE, EN_CTR_PIN_3PHASE, charge);
#endif
	}
}

#ifdef SWITCH_A_TX_PORT
static void HplcSWitchOpen(void)
{
	GPIO_SetPinDir(SWITCH_A_TX_PORT, SWITCH_A_TX_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(SWITCH_A_TX_PORT, SWITCH_A_TX_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(SWITCH_A_TX_PORT, SWITCH_A_TX_PIN, 0);

	GPIO_SetPinDir(SWITCH_A_RX_PORT, SWITCH_A_RX_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(SWITCH_A_RX_PORT, SWITCH_A_RX_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(SWITCH_A_RX_PORT, SWITCH_A_RX_PIN, 0);

	GPIO_SetPinDir(SWITCH_B_TX_PORT, SWITCH_B_TX_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(SWITCH_B_TX_PORT, SWITCH_B_TX_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(SWITCH_B_TX_PORT, SWITCH_B_TX_PIN, 0);

	GPIO_SetPinDir(SWITCH_B_RX_PORT, SWITCH_B_RX_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(SWITCH_B_RX_PORT, SWITCH_B_RX_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(SWITCH_B_RX_PORT, SWITCH_B_RX_PIN, 0);

	GPIO_SetPinDir(SWITCH_C_TX_PORT, SWITCH_C_TX_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(SWITCH_C_TX_PORT, SWITCH_C_TX_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(SWITCH_C_TX_PORT, SWITCH_C_TX_PIN, 0);

	GPIO_SetPinDir(SWITCH_C_RX_PORT, SWITCH_C_RX_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(SWITCH_C_RX_PORT, SWITCH_C_RX_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(SWITCH_C_RX_PORT, SWITCH_C_RX_PIN, 0);
}
void HplcSwitchWrite(int IsTx)
{
	if (IsTx)
	{
		GPIO_PinWrite(SWITCH_A_RX_PORT, SWITCH_A_RX_PIN, 0);
		GPIO_PinWrite(SWITCH_B_RX_PORT, SWITCH_B_RX_PIN, 0);
		GPIO_PinWrite(SWITCH_C_RX_PORT, SWITCH_C_RX_PIN, 0);

		GPIO_PinWrite(SWITCH_A_TX_PORT, SWITCH_A_TX_PIN, 1);
		GPIO_PinWrite(SWITCH_B_TX_PORT, SWITCH_B_TX_PIN, 1);
		GPIO_PinWrite(SWITCH_C_TX_PORT, SWITCH_C_TX_PIN, 1);
	}
	else
	{
		GPIO_PinWrite(SWITCH_A_TX_PORT, SWITCH_A_TX_PIN, 0);
		GPIO_PinWrite(SWITCH_B_TX_PORT, SWITCH_B_TX_PIN, 0);
		GPIO_PinWrite(SWITCH_C_TX_PORT, SWITCH_C_TX_PIN, 0);

		GPIO_PinWrite(SWITCH_A_RX_PORT, SWITCH_A_RX_PIN, 1);
		GPIO_PinWrite(SWITCH_B_RX_PORT, SWITCH_B_RX_PIN, 1);
		GPIO_PinWrite(SWITCH_C_RX_PORT, SWITCH_C_RX_PIN, 1);
	}
}
#endif

void HplcLineDriverOpen(void)
{

	if (myDevicType == METER || myDevicType == CONCENTRATOR || myDevicType == II_COLLECTOR)
	{
#if defined(LD_PORT)
		GPIO_SetPinDir(LD_PORT,LD_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(LD_PORT,LD_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(LD_PORT,LD_PIN,1);
#endif
	}
	else if(myDevicType == METER_3PHASE)
	{
#if defined(LD_PORT_3PHASE)		
		GPIO_SetPinDir(LD_PORT_3PHASE,LD_PIN_3PHASE, GPIO_OUTPUT); //
		PORT_PinConfigure(LD_PORT_3PHASE,LD_PIN_3PHASE, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(LD_PORT_3PHASE,LD_PIN_3PHASE,1);
#endif
	}
	#ifdef SWITCH_A_TX_PORT
	HplcSWitchOpen();
	#endif
}


void HplcSetLineDriver(bool flg)
{
	if (myDevicType == METER || myDevicType == CONCENTRATOR || myDevicType == II_COLLECTOR)
	{
#if defined(LD_PORT)
		GPIO_PinWrite(LD_PORT, LD_PIN, flg);
#endif
	}
	else if (myDevicType == METER_3PHASE)
	{
#if defined(LD_PORT_3PHASE)
		GPIO_PinWrite(LD_PORT_3PHASE, LD_PIN_3PHASE, flg);
#endif
	}
}


void U485EnOpen(void)
{
#ifndef I_STA
#if defined(II_STA) && defined(U485_EN_PORT)
    GPIO_SetPinDir(U485_EN_PORT,U485_EN_PIN, GPIO_OUTPUT); //
    PORT_PinConfigure(U485_EN_PORT,U485_EN_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
    GPIO_PinWrite(U485_EN_PORT,U485_EN_PIN,1);
#endif
#endif
}

void U485EnWrite(uint32_t mode)
{
#ifndef I_STA
#if defined(II_STA) && defined(U485_EN_PORT)
	if (mode == 0)
	{
        GPIO_PinWrite(U485_EN_PORT,U485_EN_PIN,0);
	}
	else
	{
        GPIO_PinWrite(U485_EN_PORT,U485_EN_PIN,1);
	}
#endif
#endif
}

void HrfDirPinInit(void)
{
#if (HRF_DIR_MODE==2)
	GPIO_SetPinDir(HRF_VC1_PORT, HRF_VC1_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(HRF_VC1_PORT, HRF_VC1_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(HRF_VC1_PORT, HRF_VC1_PIN, 1);
	GPIO_SetPinDir(HRF_VC2_PORT, HRF_VC2_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(HRF_VC2_PORT, HRF_VC2_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(HRF_VC2_PORT, HRF_VC2_PIN, 0);
#else
	GPIO_SetPinDir(HRF_DIR_PORT, HRF_DIR_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(HRF_DIR_PORT, HRF_DIR_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(HRF_DIR_PORT, HRF_DIR_PIN, 0);
#endif
}

void HrfDirPinWrite(int v)
{
#if (HRF_DIR_MODE==1)//v=1发送；v=0接收
    GPIO_PinWrite(HRF_DIR_PORT, HRF_DIR_PIN, v);
#elif (HRF_DIR_MODE==2)
	GPIO_PinWrite(HRF_VC1_PORT, HRF_VC1_PIN, !v);
	GPIO_PinWrite(HRF_VC2_PORT, HRF_VC2_PIN, v);
#else
    GPIO_PinWrite(HRF_DIR_PORT, HRF_DIR_PIN, !v);
#endif 
#ifdef HRF_SEND_HIGH_1P5
	SystemSetCoreDcdc(v);
#endif
}
//===================



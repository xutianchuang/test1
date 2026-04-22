/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_port.h"
#include "os.h"
#include "bsp.h"


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
#ifdef HPLC_CSG
	if (!CCO_Hard_Type) //南网双模终端没有reset引脚
		return;
#endif
#if defined(RESET_PORT_CCO)
	GPIO_SetPinDir(RESET_PORT_CCO, RESET_PIN_CCO, GPIO_INPUT); //
	PORT_PinConfigure(RESET_PORT_CCO, RESET_PIN_CCO, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_Port_settrigger(RESET_PORT_CCO, RESET_PIN_CCO, GPIO_FALLING_TRIGER);
	BPS_AddGpioIrq(RESET_PORT_CCO, RESET_PIN_CCO, fun);
#endif
#ifdef NEW_RESET_PORT_CCO
	GPIO_SetPinDir(NEW_RESET_PORT_CCO, NEW_RESET_PIN_CCO, GPIO_INPUT); //
	PORT_PinConfigure(NEW_RESET_PORT_CCO, NEW_RESET_PIN_CCO, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_Port_settrigger(NEW_RESET_PORT_CCO, NEW_RESET_PIN_CCO, GPIO_FALLING_TRIGER);
	BPS_AddGpioIrq(NEW_RESET_PORT_CCO, NEW_RESET_PIN_CCO, fun);

#endif

}

void ResetInterrupt(FunctionalState open_close)
{
	#ifdef HPLC_CSG
	if (!CCO_Hard_Type) //南网双模终端没有reset引脚
		return;
	#endif

	if (CCO_Hard_Type)
	{
#ifdef RESET_PORT_CCO	
		if (open_close)
		{
			GPIO_Port_settrigger(RESET_PORT_CCO, RESET_PIN_CCO, GPIO_FALLING_TRIGER);
		}
		else
		{
			GPIO_DisableIrq(RESET_PORT_CCO, RESET_PIN_CCO);		
		}	
#endif			
	}
	else
	{
#if defined(NEW_RESET_PORT_CCO) && defined(HPLC_GDW)  //国网双模
		if (open_close)
		{
			GPIO_Port_settrigger(NEW_RESET_PORT_CCO, NEW_RESET_PIN_CCO, GPIO_FALLING_TRIGER);
		}
		else
		{
			GPIO_DisableIrq(NEW_RESET_PORT_CCO, NEW_RESET_PIN_CCO);		
		}
#endif
	}
}


void CcoModeOpen(void)
{
#if defined(MODE_PIN1_PORT)
	GPIO_SetPinDir(MODE_PIN1_PORT,MODE_PIN1, GPIO_INPUT); //
	PORT_PinConfigure(MODE_PIN1_PORT,MODE_PIN1, (PORT_FUNC)(PORT_CFG_MODE(2)));
#endif

}
int GetCcoModeValue(void)
{
#if defined(MODE_PIN1_PORT)
	int mode=GPIO_PinRead(MODE_PIN1_PORT,MODE_PIN1);
	return mode;
#else
	return 3;
#endif
}


void TxSwitchOpen(void)
{

#if defined(SWITCH_A_TX_PORT)
	GPIO_SetPinDir(SWITCH_A_TX_PORT, SWITCH_A_TX_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(SWITCH_A_TX_PORT, SWITCH_A_TX_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(SWITCH_A_TX_PORT, SWITCH_A_TX_PIN, 0);
#endif
	if (CCO_Hard_Type)
	{
	#if defined(SWITCH_B_TX_PORT)
		GPIO_SetPinDir(SWITCH_B_TX_PORT, SWITCH_B_TX_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(SWITCH_B_TX_PORT, SWITCH_B_TX_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(SWITCH_B_TX_PORT, SWITCH_B_TX_PIN, 0);
	#endif
	}
	else
	{
	#if defined(SWITCH_B_TX_PORT_NEW)
		GPIO_SetPinDir(SWITCH_B_TX_PORT_NEW, SWITCH_B_TX_PIN_NEW, GPIO_OUTPUT); //
		PORT_PinConfigure(SWITCH_B_TX_PORT_NEW, SWITCH_B_TX_PIN_NEW, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(SWITCH_B_TX_PORT_NEW, SWITCH_B_TX_PIN_NEW, 0);
	#else
		debug_str(DEBUG_LOG_ERR, "No Tx switchB pin\n\r");
	#endif
	}
	if (CCO_Hard_Type)
	{
	#if defined(SWITCH_C_TX_PORT)
		GPIO_SetPinDir(SWITCH_C_TX_PORT, SWITCH_C_TX_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(SWITCH_C_TX_PORT, SWITCH_C_TX_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(SWITCH_C_TX_PORT, SWITCH_C_TX_PIN, 0);
	#endif
	}
	else
	{
	#if defined(SWITCH_C_TX_PORT_NEW)
		GPIO_SetPinDir(SWITCH_C_TX_PORT_NEW, SWITCH_C_TX_PIN_NEW, GPIO_OUTPUT); //
		PORT_PinConfigure(SWITCH_C_TX_PORT_NEW, SWITCH_C_TX_PIN_NEW, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(SWITCH_C_TX_PORT_NEW, SWITCH_C_TX_PIN_NEW, 0);
	#else
		debug_str(DEBUG_LOG_ERR, "No Tx switchC pin\n\r");
	#endif
	}
}

void TxSwitchSet(bool flg, u8 phase)
{
	if(phase == BPS_PHASE_A)
	{
#if defined(SWITCH_A_TX_PORT)
		GPIO_PinWrite(SWITCH_A_TX_PORT,SWITCH_A_TX_PIN,flg);
#endif
	}
	else if(phase == BPS_PHASE_B)
	{
		if (CCO_Hard_Type)
		{
			#if defined(SWITCH_B_TX_PORT)
			GPIO_PinWrite(SWITCH_B_TX_PORT, SWITCH_B_TX_PIN, flg);
			#endif
		}
		else
		{
			#if defined(SWITCH_B_TX_PORT_NEW)
			GPIO_PinWrite(SWITCH_B_TX_PORT_NEW, SWITCH_B_TX_PIN_NEW, flg);
			#endif
		}

	}
	else if(phase == BPS_PHASE_C)
	{
		if (CCO_Hard_Type)
		{
			#if defined(SWITCH_C_TX_PORT)
			GPIO_PinWrite(SWITCH_C_TX_PORT, SWITCH_C_TX_PIN, flg);
			#endif
		}
		else
		{
			#if defined(SWITCH_C_TX_PORT_NEW)
			GPIO_PinWrite(SWITCH_C_TX_PORT_NEW, SWITCH_C_TX_PIN_NEW, flg);
			#endif
		}

	}
}

void RxSwitchOpen(void)
{
#if defined(SWITCH_A_RX_PORT)
	GPIO_SetPinDir(SWITCH_A_RX_PORT, SWITCH_A_RX_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(SWITCH_A_RX_PORT, SWITCH_A_RX_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(SWITCH_A_RX_PORT, SWITCH_A_RX_PIN, 0);
#endif
	if (CCO_Hard_Type)
	{
	#if defined(SWITCH_B_RX_PORT)
		GPIO_SetPinDir(SWITCH_B_RX_PORT, SWITCH_B_RX_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(SWITCH_B_RX_PORT, SWITCH_B_RX_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(SWITCH_B_RX_PORT, SWITCH_B_RX_PIN, 0);
	#endif
	}
	else
	{
	#if defined(SWITCH_B_RX_PORT_NEW)
		GPIO_SetPinDir(SWITCH_B_RX_PORT_NEW, SWITCH_B_RX_PIN_NEW, GPIO_OUTPUT); //
		PORT_PinConfigure(SWITCH_B_RX_PORT_NEW, SWITCH_B_RX_PIN_NEW, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(SWITCH_B_RX_PORT_NEW, SWITCH_B_RX_PIN_NEW, 0);
	#else
		debug_str(DEBUG_LOG_ERR, "No rx switchB pin\n\r");
	#endif
	}
	if (CCO_Hard_Type)
	{
	#if defined(SWITCH_C_RX_PORT)
		GPIO_SetPinDir(SWITCH_C_RX_PORT, SWITCH_C_RX_PIN, GPIO_OUTPUT); //
		PORT_PinConfigure(SWITCH_C_RX_PORT, SWITCH_C_RX_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(SWITCH_C_RX_PORT, SWITCH_C_RX_PIN, 0);
	#endif
	}
	else
	{
	#if defined(SWITCH_C_RX_PORT_NEW)
		GPIO_SetPinDir(SWITCH_C_RX_PORT_NEW, SWITCH_C_RX_PIN_NEW, GPIO_OUTPUT); //
		PORT_PinConfigure(SWITCH_C_RX_PORT_NEW, SWITCH_C_RX_PIN_NEW, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(SWITCH_C_RX_PORT_NEW, SWITCH_C_RX_PIN_NEW, 0);
	#else
		debug_str(DEBUG_LOG_ERR, "No rx switchC pin\n\r");
	#endif
	}
}

void RxSwitchSet(bool flg, uint32_t phase)
{
	if(phase == BPS_PHASE_A)
	{
#if defined(SWITCH_A_RX_PORT)
		GPIO_PinWrite(SWITCH_A_RX_PORT,SWITCH_A_RX_PIN,flg);
#endif
	}
	else if(phase == BPS_PHASE_B)
	{
		if (CCO_Hard_Type)
		{
			#if defined(SWITCH_B_RX_PORT)
			GPIO_PinWrite(SWITCH_B_RX_PORT, SWITCH_B_RX_PIN, flg);
			#endif
		}
		else
		{
			#if defined(SWITCH_B_RX_PORT_NEW)
			GPIO_PinWrite(SWITCH_B_RX_PORT_NEW, SWITCH_B_RX_PIN_NEW, flg);
			#endif
		}
	}
	else if(phase == BPS_PHASE_C)
	{
		if (CCO_Hard_Type)
		{
			#if defined(SWITCH_C_RX_PORT)
			GPIO_PinWrite(SWITCH_C_RX_PORT, SWITCH_C_RX_PIN, flg);
			#endif
		}
		else
		{
			#if defined(SWITCH_C_RX_PORT_NEW)
			GPIO_PinWrite(SWITCH_C_RX_PORT_NEW, SWITCH_C_RX_PIN_NEW, flg);
			#endif
		}
	}
}

int32_t ResetRead()
{
#ifdef HPLC_CSG
	if (!CCO_Hard_Type) //南网双模终端没有reset引脚
		return -1;
#endif

	if (CCO_Hard_Type)
	{
#ifdef RESET_PORT_CCO	
		return GPIO_PinRead(RESET_PORT_CCO, RESET_PIN_CCO);
#else
		return -1;    
#endif
	}
	else
	{
#if defined(NEW_RESET_PORT_CCO) && defined(HPLC_GDW)  //国网双模
		return GPIO_PinRead(NEW_RESET_PORT_CCO, NEW_RESET_PIN_CCO);
#else
		return -1;
#endif
	}
}

void HplcLineDriverOpen(void)
{
#if defined(LD_PORT)
	GPIO_SetPinDir(LD_PORT, LD_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(LD_PORT, LD_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(LD_PORT, LD_PIN, 1);
#endif
}


void HplcSetLineDriver(bool flg)
{
#if defined(LD_PORT)
	GPIO_PinWrite(LD_PORT, LD_PIN, flg);
#endif
}






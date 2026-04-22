/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_led.h"
#include "os.h"
#include "bsp.h"
#include "hplc_data.h"
#include "ble_data.h"
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

void NetStatusLedOpen(void)
{
	// LED灯初始化
	GPIO_SetPinDir(NET_STATUS_LED_GROUP2, NET_STATUS_LED1_RED, GPIO_OUTPUT); 
	PORT_PinConfigure(NET_STATUS_LED_GROUP2, NET_STATUS_LED1_RED, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(NET_STATUS_LED_GROUP2, NET_STATUS_LED1_RED, 0);

	// LED灯初始化
	GPIO_SetPinDir(NET_STATUS_LED_GROUP2, NET_STATUS_LED2_GREEN, GPIO_OUTPUT); 
	PORT_PinConfigure(NET_STATUS_LED_GROUP2, NET_STATUS_LED2_GREEN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(NET_STATUS_LED_GROUP2, NET_STATUS_LED2_GREEN, 0);

	// LED灯初始化
	GPIO_SetPinDir(NET_STATUS_LED_GROUP2, NET_STATUS_LED3_RED, GPIO_OUTPUT); 
	PORT_PinConfigure(NET_STATUS_LED_GROUP2, NET_STATUS_LED3_RED, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(NET_STATUS_LED_GROUP2, NET_STATUS_LED3_RED, 0);

	// LED灯初始化
	GPIO_SetPinDir(NET_STATUS_LED_GROUP2, NET_STATUS_LED4_GREEN, GPIO_OUTPUT); 
	PORT_PinConfigure(NET_STATUS_LED_GROUP2, NET_STATUS_LED4_GREEN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(NET_STATUS_LED_GROUP2, NET_STATUS_LED4_GREEN, 0);

	// LED灯初始化-------
	GPIO_SetPinDir(NET_STATUS_LED_GROUP4, NET_STATUS_LED5_RED, GPIO_OUTPUT); 
	PORT_PinConfigure(NET_STATUS_LED_GROUP4, NET_STATUS_LED5_RED, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(NET_STATUS_LED_GROUP4, NET_STATUS_LED5_RED, 0);

	// LED灯初始化
	GPIO_SetPinDir(NET_STATUS_LED_GROUP4, NET_STATUS_LED6_GREEN, GPIO_OUTPUT); 
	PORT_PinConfigure(NET_STATUS_LED_GROUP4, NET_STATUS_LED6_GREEN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(NET_STATUS_LED_GROUP4, NET_STATUS_LED6_GREEN, 0);

	// LED灯初始化
	GPIO_SetPinDir(NET_STATUS_LED_GROUP4, NET_STATUS_LED7_GREEN, GPIO_OUTPUT); 
	PORT_PinConfigure(NET_STATUS_LED_GROUP4, NET_STATUS_LED7_GREEN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(NET_STATUS_LED_GROUP4, NET_STATUS_LED7_GREEN, 1);

	// LED灯初始化
	GPIO_SetPinDir(NET_STATUS_LED_GROUP4, NET_STATUS_LED8_RED, GPIO_OUTPUT); 
	PORT_PinConfigure(NET_STATUS_LED_GROUP4, NET_STATUS_LED8_RED, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(NET_STATUS_LED_GROUP4, NET_STATUS_LED8_RED, 0);
}

void NetStatusLedClose(void)
{
	GPIO_PinWrite(NET_STATUS_LED_GROUP2, NET_STATUS_LED1_RED, 0);
	GPIO_PinWrite(NET_STATUS_LED_GROUP2, NET_STATUS_LED2_GREEN, 0);

	GPIO_PinWrite(NET_STATUS_LED_GROUP2, NET_STATUS_LED3_RED, 0);
	GPIO_PinWrite(NET_STATUS_LED_GROUP2, NET_STATUS_LED4_GREEN, 0);

	// GPIO_PinWrite(NET_STATUS_LED_GROUP4, NET_STATUS_LED7_GREEN, 0);
	GPIO_PinWrite(NET_STATUS_LED_GROUP4, NET_STATUS_LED8_RED, 0);
}


// 	LED1：显示上行通道是PLC还是RF，PLC绿灯每秒闪一次，RF红灯每秒闪一次。
// 	LED2：获取与CCO间的最小路径成功率，大于80亮绿灯，否则亮红灯。
// 	LED3：诊断与PCO之间的通信性能优劣，可通过与PCO之间的上下行成功率来判断。成功率大于80则亮绿灯，否

void NetStatusLedWrite(uint8_t ledType, uint32_t ledColor, uint32_t onOrOff)
{
	uint32_t ledColorValue = 0;

	switch (ledType)
	{
		case SOFEWARE_RUN_START:
		{
			(ledColor == GREEN_LED)?(ledColorValue = NET_STATUS_LED7_GREEN):(ledColorValue = NET_STATUS_LED8_RED);
			GPIO_PinWrite(NET_STATUS_LED_GROUP4, ledColorValue, onOrOff);
		}
		break;

		case UP_CHANNEL_TYPE:
		{
			(ledColor == GREEN_LED)?(ledColorValue = NET_STATUS_LED6_GREEN):(ledColorValue = NET_STATUS_LED5_RED);
			GPIO_PinWrite(NET_STATUS_LED_GROUP4, ledColorValue, onOrOff);			
		}
		break;

		case LEST_SUCCESS_RATE_BETWEEN_CCO:
		{
			(ledColor == GREEN_LED)?(ledColorValue = NET_STATUS_LED4_GREEN):(ledColorValue = NET_STATUS_LED3_RED);
			GPIO_PinWrite(NET_STATUS_LED_GROUP2, ledColorValue, onOrOff);			
		}
		break;
		
		case COMMUNICATE_RATE_BETWEEN_PCO:
		{
			(ledColor == GREEN_LED)?(ledColorValue = NET_STATUS_LED2_GREEN):(ledColorValue = NET_STATUS_LED1_RED);
			GPIO_PinWrite(NET_STATUS_LED_GROUP2, ledColorValue, onOrOff);			
		}
		break;
	}
}

void NetStatusLed_OutNet()
{
	GPIO_PinWrite(NET_STATUS_LED_GROUP4, NET_STATUS_LED5_RED, 0);
	GPIO_PinWrite(NET_STATUS_LED_GROUP4, NET_STATUS_LED6_GREEN, 0);
}


void NetStatusLed_BleInNet(uint32_t ledColor)
{
	uint32_t ledColorValue = 0;
	(ledColor == GREEN_LED)?(ledColorValue = NET_STATUS_LED6_GREEN):(ledColorValue = NET_STATUS_LED5_RED);

	GPIO_PinWrite(NET_STATUS_LED_GROUP4, ledColorValue, 0);
}

void NetStatusChannelLedBlink(uint32_t ledColor)
{
	static uint8_t onOrOff = 0;
	uint32_t ledColorValue = 0;

        onOrOff++;
	onOrOff = onOrOff%2; 
	(ledColor == GREEN_LED)?(ledColorValue = NET_STATUS_LED6_GREEN):(ledColorValue = NET_STATUS_LED5_RED);

	GPIO_PinWrite(NET_STATUS_LED_GROUP4, ledColorValue, onOrOff);	
}

void NetStatusLed_RateBetweenCCO(uint32_t ledColor)
{
	if(ledColor == GREEN_LED)
	{
		NetStatusLedWrite(LEST_SUCCESS_RATE_BETWEEN_CCO, GREEN_LED, 1);
		NetStatusLedWrite(LEST_SUCCESS_RATE_BETWEEN_CCO, RED_LED, 0);
	}
	else
	{
		NetStatusLedWrite(LEST_SUCCESS_RATE_BETWEEN_CCO, GREEN_LED, 0);
		NetStatusLedWrite(LEST_SUCCESS_RATE_BETWEEN_CCO, RED_LED, 1);
	}	
}

void NetStatusLed_RateBetweenPCO(uint32_t ledColor)
{
	if(ledColor == GREEN_LED)
	{
		NetStatusLedWrite(COMMUNICATE_RATE_BETWEEN_PCO, GREEN_LED, 1);
		NetStatusLedWrite(COMMUNICATE_RATE_BETWEEN_PCO, RED_LED, 0);
	}
	else
	{
		NetStatusLedWrite(COMMUNICATE_RATE_BETWEEN_PCO, GREEN_LED, 0);
		NetStatusLedWrite(COMMUNICATE_RATE_BETWEEN_PCO, RED_LED, 1);
	}	
}

// 网络诊断
extern uint8_t EnterNetReportStatus;
extern void LampEventAddInNetReport(uint8_t * mac_addr);
uint8_t LastCCOMac[6] = {0};
void NetStatusLedHandle(void)
{
	const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();
	const ST_NEIGHBOR_TAB_TYPE *pneighbor = GetNeighborTable();
	// 获取代理站点信息
	const ST_NEIGHBOR_TYPE*  pPcoInfo = GetPCO_Info();
	uint8_t NetReportFlag = 0;
	if(pNetBasePara->NetState == NET_IN_NET)
	{
		if(memcmp(LastCCOMac,pNetBasePara->CcoMac,6) != 0)
		{
			NetReportFlag = 1;
		}
		memcpy(LastCCOMac,pNetBasePara->CcoMac,6);
		if (1 == GetRelaySendLink(1))
		{
			// 判断入网通道
			NetStatusChannelLedBlink(GREEN_LED);
		}
		else
		{
			// 判断入网通道
			NetStatusChannelLedBlink(RED_LED);
		}

		if(EnterNetReportStatus == 0 || NetReportFlag == 1)
		{
			LampEventAddInNetReport((uint8_t *)pNetBasePara->Mac);			
		}


		// 判断最小路径通信成功率
		if(pneighbor->Neighbor[0].RouteMinSucRate > 80)
		{
			NetStatusLed_RateBetweenCCO(GREEN_LED);
		}
		else
		{
			NetStatusLed_RateBetweenCCO(RED_LED);
		}

		// 判断与PCO的通信成功率
		// if(pneighbor->Neighbor[pNetBasePara->TEI].PcoSucRate > 80)
		if(pPcoInfo->PcoSucRate > 80)
		{
			NetStatusLed_RateBetweenPCO(GREEN_LED);
		}
		else
		{
			NetStatusLed_RateBetweenPCO(RED_LED);
		}
	}
	else	// 关闭所有状态灯
	{
		EnterNetReportStatus = 0;
		NetStatusLedClose();
		//蓝牙入网绿灯常亮
		if(ble_sta_net_info.net_statu == 1)
		{
			NetStatusLed_BleInNet(GREEN_LED);
		}
		else
		{
			NetStatusLed_OutNet();
		}
	}
}

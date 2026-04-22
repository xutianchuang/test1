#include <gsmcu_hal.h>
#include "protocol_includes.h"

static void OverFlowHandle(void)
{

}
void PHY_Init(void);
//HPLC物理底层初始化
void HPLC_PHY_Init(void)
{
	SetReceiveCallback(HPLC_PHY_PB_Callback, HPLC_PHY_FC_Callback, OverFlowHandle);
	PHY_Init();
}


extern uint32_t HPLCCurrentFreq;
//HPLC设置频段
void HPLC_PHY_SetFrequence(u8 fre)
{
	CPU_SR_ALLOC();
	CPU_INT_DIS();
//	BPLC_Stop();
	HPLCCurrentFreq = fre;

#ifndef ALL_USE_TESTMODE_PARAM
    if(GetAppCurrentModel() == APP_TEST_NORMAL)
	{
		HPLC_SouthPhyInit(fre,HPLC_ChlPower[fre]);
	}
	else
#endif
	{
		HPLC_SouthPhyInit(fre,HPLC_TestChlPower[fre]);
	}

	CPU_INT_EN();
}

//读取频段
u8 HPLC_PHY_GetFrequence(void)
{
	return FrequenceGet();
}




//计数帧载荷符号数
uint32_t	HPLC_PHY_GetSymbolNum(uint8_t tmi, uint8_t tmiExt, uint8_t pb_num)
{
	return GetSymbolNum(tmi, tmiExt, pb_num);
}

//计数帧长
uint32_t	HPLC_PHY_GetFrameLen(uint8_t FC, uint16_t SymbolNum)
{
	return GetFrameLen(FC, SymbolNum);
}

//HPLC复位
void HPLC_PHY_Reset(void)
{
	HPLC_PhyReset();
}

//HPLC调节
int HPLC_PHY_NTB_Adjust(uint32_t recvNTB0, uint32_t localNTB0, uint32_t recvNTB1, uint32_t localNTB1)
{
	return NTB_Sync_Adjust(recvNTB0, localNTB0, recvNTB1, localNTB1);
}


PLC_STATE BPLC_GetState(void)
{
	if (PLC_STATE_IDLE != BPLC_GetTxState())
	{
		return BPLC_GetTxState();
	}
	else
	{
		return BPLC_GetRxState();
	}
}

//HPLC状态
HPLC_PHY_STATE HPLC_PHY_GetState(void)
{
	if (BPLC_GetState() == PLC_STATE_IDLE)
		return HPLC_PHY_IDLE;
	return HPLC_PHY_BUSY;
}


void HPLCSendOptionFun(uint32_t IsOpen, u32 phase)
{
	if (IsOpen == 1)
	{
#if DEV_STA
		StaWrite(1);
#endif
		HplcSetLineDriver(false);
	}
	else
	{
#if DEV_STA
		StaWrite(0);
#endif
		HplcSetLineDriver(true);
	}
	#ifdef SWITCH_A_TX_PORT
	HplcSwitchWrite(IsOpen);
	#endif
}


//PB回调
void HPLC_PHY_PB_Callback(uint8_t *data, uint16_t len, uint8_t crc_res, BPLC_recv_para *para)
{
	ReceivePhyCallback(data, len, crc_res, para);
}

//FC回调
void HPLC_PHY_FC_Callback(uint8_t *data, BPLC_recv_para *para, bool_t status)
{
	ReceiveCtrlCallback(data, para, status);
}

//inline void HPLC_SendImmediate(uint8_t *data, uint16_t len, SendCallback callback, OptionCallback optionFun)
//{
//	BPLC_SendImmediate(data, len, callback, optionFun,1);
//}

//inline void HPLC_SendAtNtb(uint32_t NTB, uint8_t *data, uint16_t len, SendCallback callback, OptionCallback optionFun)
//{
//	BPLC_SendAtNtb(NTB, data, len, callback, optionFun,1);
//}

//HPLC发送
void HPLC_PHY_SendImmediate(uint8_t *data, uint16_t len, SendCallback callback)
{
//	HPLC_SendImmediate(data,len,callback,HPLCSendOptionFun);
	BPLC_SendImmediate(data, len, callback, HPLCSendOptionFun, 1);
}

//HPLC NTB发送
void HPLC_PHY_SendAtNtb(uint32_t NTB, uint8_t *data, uint16_t len, SendCallback callback)
{
//	HPLC_SendAtNtb(NTB, data, len, callback, HPLCSendOptionFun);
	BPLC_SendAtNtb(NTB, data, len, callback, HPLCSendOptionFun, 1);
}





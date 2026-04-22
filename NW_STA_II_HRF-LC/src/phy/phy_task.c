#include  <gsmcu_hal.h>
#include <string.h>
#include "os.h"
#include <bsp.h>
#include  <os_cfg_app.h>
#include "phy_port.h"

#include "protocol_includes.h"

//#include "HRF_SpiInterface.h"



uint32_t recv_cnt = 0;
uint32_t send_cnt = 0;

uint32_t send_start_err = 0;
uint32_t switch_line = 0;

volatile uint32_t ntb_stamp[16][2];
uint8_t ntb_index = 0;
OS_TCB SyncTaskTCB;
u8 startSyncNtb;
u32 SyncNtbVaile;
u32 HrfGetNtb;

uint32_t user_over_flow_count = 0;
uint32_t user_fc_fail_count = 0;
uint32_t user_fc_pass_count = 0;
uint32_t ZC_Time[3] = { 0, 0, 0 };

phaseRecognizeType ZC_NTB;
u8 zero_use_phase = 0;

u8 reqZero=1;
void Zero_Lost(void)
{
	u32 ntb = BPLC_GetNTB();
	u8 get_zero_phase = 0xf;
	for (int i = 0; i < 3; i++)
	{
		if (ZC_Time[i] != 0)
		{
			if ((ntb - ZC_Time[i]) > (NTB_FRE_PER_MS * 240)) //过零丢失
			{
				if (zero_use_phase == i)
				{
					if (reqZero)
					{
						reqZero=0;
						RecError code = { 0x2, 0x1, 0 };
						saveHplcErrLog((char*)&code,sizeof(code));
					}
					memset(&ZC_NTB, 0, sizeof(phaseRecognizeType));
				}
				ZC_Time[i] = 0;
			}
			else
			{
				if (get_zero_phase == 0xf)
				{
					get_zero_phase = i;
				}
			}
		}

	}
	if (myDevicType == METER_3PHASE)
	{
		if (get_zero_phase != 0xf && get_zero_phase != zero_use_phase)
		{
			if (ZC_Time[zero_use_phase] == 0) //放止频繁更换相线
			{
				if (reqZero)
				{
					reqZero=0;
					RecError code = { 0x2, 0x1, 0 };
					saveHplcErrLog((char *)&code, sizeof(code));
				}
				zero_use_phase = get_zero_phase;
				memset(&ZC_NTB, 0, sizeof(phaseRecognizeType));
			}
		}
	}
}

void SaveZCNtb(uint32_t NTB, phaseRecognizeType *zcNtb)
{
	zcNtb->zc_ntb[zcNtb->Widx++] = NTB;
	if (zcNtb->Widx == MAX_ZC_RECOGNIZE_PHASE)
	{
		zcNtb->Widx = 0;
	}
	if (zcNtb->Widx == zcNtb->Ridx)
	{
		zcNtb->Ridx = zcNtb->Widx + 1;
		if (zcNtb->Ridx == MAX_ZC_RECOGNIZE_PHASE)
		{
			zcNtb->Ridx = 0;
		}
	}
}






//20 ms 中断一次
bool ZC_Handler(u32 ntb, int phase)
{
	if (phase == BPS_PHASE_A)
	{
		ZC_Time[0] = ntb;

		#if (BPS_PHASE_A==ZC_SYNC_CH)
		if (startSyncNtb)
		{
			SyncNtbVaile = ntb;
		}
		else
		#endif
			if (zero_use_phase == phase)
			{
				SaveZCNtb(ZC_Time[0], &ZC_NTB);
			}
	}
	else if (phase == BPS_PHASE_B)
	{
		ZC_Time[1] = ntb;

		#if (BPS_PHASE_B==ZC_SYNC_CH)
		if (startSyncNtb)
		{
			SyncNtbVaile = ntb;
		}
		else
		#endif
			if (zero_use_phase == phase)
			{
				SaveZCNtb(ZC_Time[1], &ZC_NTB);
			}
	}
	else if (phase == BPS_PHASE_C)
	{
		ZC_Time[2] = ntb;

		#if (BPS_PHASE_C==ZC_SYNC_CH)
		if (startSyncNtb)
		{
			SyncNtbVaile = ntb;
		}
		else
		#endif
			if (zero_use_phase == phase)
			{
				SaveZCNtb(ZC_Time[2], &ZC_NTB);
			}
	}
	else if (phase == BPS_PHASE_A_NEG)
	{
		#if (BPS_PHASE_A_NEG==ZC_SYNC_CH)
		if (startSyncNtb)
		{
			SyncNtbVaile = ntb;
		}
		#endif
	}
	else if (phase == BPS_PHASE_B_NEG)
	{
		#if (BPS_PHASE_B_NEG==ZC_SYNC_CH)
		if (startSyncNtb)
		{
			SyncNtbVaile = ntb;
		}
		#endif
	}
	else if (phase == BPS_PHASE_C_NEG)
	{
		#if (BPS_PHASE_C_NEG==ZC_SYNC_CH)
		if (startSyncNtb)
		{
			SyncNtbVaile = ntb;
		}
		#endif
	}
	return false;
}

uint32_t HPLCCurrentFreq = 2;
uint16_t HRFCurrentIndex = 3<<8|112;

void PHY_SendCallbackFun(bool_t flag)
{
	if (flag == SUCCESS)
	{
		send_cnt++;
	}
	else
	{
		send_start_err++;
	}



}



void PHY_OverFlowCallbackFun(void)
{
	user_over_flow_count++;
}

//void Timer0_Callback()
//{
//	uint32_t NTB=BPLC_GetNTB()+25000*2000;//延时后再开启NTB时间
//	BPLC_StartNbi();
//	NTB_TimerStart(3,NTB,Timer0_Callback);
//}

void PHY_Init(void)
{
#ifndef ALL_USE_TESTMODE_PARAM
	if (GetAppCurrentModel() == APP_TEST_NORMAL)
	{
		HPLC_SouthPhyInit(HPLCCurrentFreq, HPLC_ChlPower[HPLCCurrentFreq]);
	}
	else
#endif
	{
		HPLC_SouthPhyInit(HPLCCurrentFreq, HPLC_TestChlPower[HPLCCurrentFreq]);
	}
#ifdef ZC_CH_PHASEA_POS
	ZeroCrossx_Start(ZC_CH_PHASEA_POS);
#endif
	if (myDevicType == METER_3PHASE)
	{
#ifdef ZC_CH_PHASEB_POS
		ZeroCrossx_Start(ZC_CH_PHASEB_POS);
#endif
#ifdef ZC_CH_PHASEC_POS
		ZeroCrossx_Start(ZC_CH_PHASEC_POS);
#endif
	}

	BPLC_EnableIRQ(BPLC_BBP_INT_EN_TX_END_INT_EN_Msk        |         //发送完成
				   BPLC_BBP_INT_EN_TX_START_FAIL_INT_EN_Msk |      //发送起始错误
				   BPLC_BBP_INT_EN_SYNC_BEGIN_INT_EN_Msk    |
				   BPLC_BBP_INT_EN_NBI_DET_END_INT_EN_Msk   |     //NBI
				   BPLC_BBP_INT_EN_FRAMESYNCED_INT_EN_Msk   |         //帧同步完成
				   BPLC_BBP_INT_EN_RX_FC_RCV_INT_EN_Msk     |         //FC中断
				   BPLC_BBP_INT_EN_RX_OVER_FLOW_INT_EN_Msk  |
				   BPLC_BBP_INT_EN_RX_END_INT_EN_Msk);                //接收完成中断
}


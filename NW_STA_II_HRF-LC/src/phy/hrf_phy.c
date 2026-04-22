#include <gsmcu_hal.h>

#include "protocol_includes.h"
#include "board_cco_def.h"
#include "GSMCU_M3.h"
#include "gsmcu_m3_port.h"
#include "gsmcu_m3_gpio.h"
#include "hrf_port.h"
static u8               s_hrf_buffer[600];
//HRF物理底层初始化
void HRF_PHY_SendImmediate(u8 phr_mcs,void *pdata, uint16_t len, SendCallback callback)
{
	memcpy_special(s_hrf_buffer, pdata, len);
	MPDU_CTL *mpduCtl = (MPDU_CTL *)s_hrf_buffer;
	SOF_HRF_ZONE *sofZone = (SOF_HRF_ZONE*)mpduCtl->Zone0;
#ifdef FIX_HRF_LEN
	if (mpduCtl->Type==1)
	{
		if(sofZone->PbSize < 5)
		{
			sofZone->PbSize += 1;
		}
		else
		{
			return;
		}
	}
#endif
	HRF_SendImmediate(phr_mcs, sofZone->MCS,s_hrf_buffer, sofZone->PbSize, callback);
#ifdef DEBUG_FRMAE_MPDU
	debug_hex(true, (u8 *)pdata, len);
#endif
}

void HRF_PHY_SendAtNtb(uint32_t NTB, u8 phr_mcs, void *pdata, uint16_t len, SendCallback callback)
{

	MPDU_CTL* pMpduCtl = (MPDU_CTL*)pdata;
	BEACON_HRF_ZONE *beacZone = (BEACON_HRF_ZONE *)pMpduCtl->Zone0;
	HRF_SendAtNtb(NTB, phr_mcs, beacZone->MCS,pdata, len, callback);
}

void HRF_SendCoor(uint8_t *pdata, uint16_t len, SendCallback callback, u8 phr_mcs)
{

	u32 current_ntb=BPLC_GetNTB();
	MPDU_CTL* pMpduCtl = (MPDU_CTL*)pdata;
    CONCERT_ZONE* pConcertZone = (CONCERT_ZONE*)pMpduCtl->Zone0;
	current_ntb-=*(u32*)&pdata[16];
	current_ntb/=NTB_FRE_PER_MS;
#ifdef HPLC_CSG
	current_ntb/=CSG_COOR_RATE;
	if (pConcertZone->CompleteFlag)
	{
		pConcertZone->TapeEndoffset += current_ntb;
	}
	else
	{
		pConcertZone->TapeEndoffset -= current_ntb;
	}
#endif
	pConcertZone->Tapeoffset-=current_ntb;
	memcpy_special(s_hrf_buffer, pdata, len);

	HRF_SendImmediate(phr_mcs,0,s_hrf_buffer, 0, callback);
#ifdef DEBUG_FRMAE_MPDU
	debug_hex(true, (u8 *)pdata, len);
#endif
}

//HRF复位
void HRF_PHY_Reset(void)
{
	HRF_PhyReset();
}



PLC_STATE HRF_GetState(void)
{
	if (PLC_STATE_IDLE != HRF_GetTxState())
	{
		return HRF_GetTxState();
	}
	else
	{
		return HRF_GetRxState();
	}
}

//HRF状态
bool HRF_PHY_GetState(void)
{
	if (HRF_GetState() == PLC_STATE_IDLE)
		return true;
	return false;
}





//inline void HRF_SendImmediate(uint8_t *data, uint16_t len, SendCallback callback, OptionCallback optionFun)
//{
//	HRF_SendImmediate(data, len, callback, optionFun,1);
//}

//inline void HRF_SendAtNtb(uint32_t NTB, uint8_t *data, uint16_t len, SendCallback callback, OptionCallback optionFun)
//{
//	HRF_SendAtNtb(NTB, data, len, callback, optionFun,1);
//}


void HRFLedOpen(void)
{
	GPIO_SetPinDir(HRF_TX_LED_PORT, HRF_TX_LED_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(HRF_TX_LED_PORT, HRF_TX_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(HRF_TX_LED_PORT, HRF_TX_LED_PIN, 0);


	GPIO_SetPinDir(HRF_TX_LED_PORT, HRF_RX_LED_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(HRF_TX_LED_PORT, HRF_RX_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(HRF_TX_LED_PORT, HRF_RX_LED_PIN, 0);
}


void LedTxSet(int mode)
{
	GPIO_PinWrite(HRF_TX_LED_PORT, HRF_TX_LED_PIN, mode);
}
void LedRxSet(int mode)
{
	GPIO_PinWrite(HRF_RX_LED_PORT, HRF_RX_LED_PIN, mode);
}

static void HrfDirPinWrite(int v)
{
#if EFEM_MODE 
	if (v)   // TX mode
	{
		GPIO_SetPinDir(FEM_CTX_PORT, FEM_CTX_PIN, GPIO_OUTPUT);                       // CTX set to GPIO mode
		PORT_PinConfigure(FEM_CTX_PORT, FEM_CTX_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
		GPIO_PinWrite(FEM_CTX_PORT, FEM_CTX_PIN, 1);
		GPIO_PinWrite(FEM_CPS_PORT, FEM_CPS_PIN, 1);
	}
	else     // RX mode
	{
		GPIO_SetPinDir(FEM_CTX_PORT, FEM_CTX_PIN, GPIO_OUTPUT);                       // CTX set to ANT_SEL mode and controlled from RXGT, eLNA control
		PORT_PinConfigure(FEM_CTX_PORT, FEM_CTX_PIN, (PORT_FUNC)(PORT_CFG_MODE(1)));
		GPIO_PinWrite(FEM_CPS_PORT, FEM_CPS_PIN, 0);
	}
#elif (HRF_DIR_MODE==1)//v=1发送；v=0接收
    GPIO_PinWrite(HRF_DIR_PORT, HRF_DIR_PIN, v);
#elif (HRF_DIR_MODE==2)
	GPIO_PinWrite(HRF_VC1_PORT, HRF_VC1_PIN, !v);
	GPIO_PinWrite(HRF_VC2_PORT, HRF_VC2_PIN, v);
#else//v=0发送；v=1接收
    GPIO_PinWrite(HRF_DIR_PORT, HRF_DIR_PIN, !v);
#endif   
	
	if(!v)
	{
		LedRxSet(0);
	}
}

void HRF_PhyInit(uint8_t channel, uint8_t option, uint8_t power)
{
    HRF_init_para	Para;
    Para.channel = channel;
    Para.option = option;
    Para.gain = power;
    Para.agc = 0x70;
	Para.HrfDirFunc = HrfDirPinWrite;
	#if defined(HPLC_CSG)
	Para.net_type = 0;
	#else
	Para.net_type = 1;
	#endif
    HRF_Init(&Para);
}

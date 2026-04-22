#include "hplc_receive.h"
#include "system_inc.h"
#include "os.h"
#include "hplc_send.h"
#include "Uart_Link.h"
#include "hrf_phy.h"
#include "../../src/pack2lib/pack2lib.h"

extern u32 PLC_TO_RF_TESTMODE_PARAM;

void HPLC_MsgProc();

//--------para--------------

//-------start-------
void HPLC_ReceiveBeaconFCCallback(u8 *data, u32 len, BPLC_recv_para *para)
void HPLC_ReceiveCallback(u8 *data, u32 len, BPLC_recv_para *para,uint8_t crc)
void HRF_ReceiveBeaconFCCallback(u8 *data, u32 len, BPLC_recv_para *para)
void HRF_ReceiveCallback(u8 *data, u32 len, BPLC_recv_para *para,uint8_t crc)
u8 HPLC_PhySnr2ProtocolSnr(int32_t phy_snr)
void HPLC_ChangeFrequence(u8 new_frequence, BOOL test_mode)
void HPLC_ChangeFrequenceNotSave(u8 new_frequence, BOOL test_mode)
void HRF_ChangeChannel(u8 channel, u8 option)
void HPLC_ChangeHrfChannelNotSave(u8 channel, u8 option)
void HPLC_ChangeMCSNotSave(u8 phr_mcs, u8 psdu_mcs, u8 pbsize)
void HPLC_ChangeToneMask(u8 new_value)
void HPLC_ChangePower()
void HPLC_MsgProc()
void HPLC_ReveiveTask(void *arg)
void HPLC_TimerTask(void *arg)
















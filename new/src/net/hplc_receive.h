#ifndef __HPLC_RECEIVE_H__
#define __HPLC_RECEIVE_H__
#ifdef __cplusplus
extern "C"
{
#endif
#include "phy_port.h"

void HPLC_ReveiveTask(void *arg);
void HPLC_ReceiveBeaconFCCallback(u8 *data, u32 len, BPLC_recv_para *para);
void HPLC_ReceiveCallback(u8 *data, u32 len, BPLC_recv_para *para,uint8_t crc);
void HRF_ReceiveBeaconFCCallback(u8 *data, u32 len, BPLC_recv_para *para);
void HRF_ReceiveCallback(u8 *data, u32 len, BPLC_recv_para *para,uint8_t crc);
void HPLC_TimerTask(void *arg);
u8 HPLC_PhySnr2ProtocolSnr(int32_t phy_snr);


#ifdef __cplusplus
}
#endif
#endif /* __HPLC_RECEIVE_H__ */


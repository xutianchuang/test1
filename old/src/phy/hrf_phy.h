#ifndef _HRF_PHY_H
#define _HRF_PHY_H


#include "hrf_port.h"


//HRF物理底层初始化
void HRF_PHY_Init(void);

//HRF设置信道
void HRF_PHY_SetChannel(uint8_t channel, uint8_t option);

//HRF读取信道
u8 HRF_PHY_GetChannel(void);


//HRF复位
void HRF_PHY_Reset(void);



//HRF状态
HPLC_PHY_STATE HRF_PHY_GetState(void);

//PB回调
void HRF_PHY_PB_Callback(uint8_t *data, uint16_t len, uint8_t crc_res, BPLC_recv_para *para);

//FC回调
void HRF_PHY_FC_Callback(uint8_t *data, BPLC_recv_para *para, bool_t status);

//HRF发送
void HRF_PHY_SendImmediate(uint8_t phr_mcs, uint8_t psdu_mcs,void* data, uint16_t len, SendCallback callback);

//HRF NTB发送
void HRF_PHY_SendAtNtb(uint32_t NTB, uint8_t phr_mcs, uint8_t psdu_mcs,void* data, uint16_t len, SendCallback callback);

void LedTxSet(int mode);
void LedRxSet(int mode);

#endif

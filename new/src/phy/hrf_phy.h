#ifndef _HRF_PHY_H
#define _HRF_PHY_H


#include "hrf_port.h"


//HRF物理底层初始化
void HRF_PHY_Init(u8 option,u8 channel);

//HRF设置频段
void HRF_PHY_SetFrequence(u8 fre);

//读取频段
u8 HRF_PHY_GetFrequence(void);


//HRF复位
void HRF_PHY_Reset(void);



//HRF状态
bool HRF_PHY_GetState(void);


void HRF_SendCoor(uint8_t *pdata, uint16_t len, SendCallback callback, u8 phr_mcs);
//HRF发送
void HRF_PHY_SendImmediate(uint8_t phr_mcs, void* data, uint16_t len, SendCallback callback);

//HRF NTB发送
void HRF_PHY_SendAtNtb(uint32_t NTB, uint8_t phr_mcs, void* data, uint16_t len, SendCallback callback);

void LedTxSet(int mode);
void LedRxSet(int mode);
void HRFLedOpen(void);
#endif

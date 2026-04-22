#ifndef _HPLC_ZC_MANAGER_H_
#define _HPLC_ZC_MANAGER_H_

//电压过零NTB管理
typedef bool(*pZeroCrossFun)(u32 ntb,int phase);

void ZeroCrossManagerInit(void);

void AddZeroCrossCallback(pZeroCrossFun fun);
u8 ZeroSelfStatus(void);
bool ZC_Handler(u32 ntb,int phase);
void Zero_Lost(void);
#endif

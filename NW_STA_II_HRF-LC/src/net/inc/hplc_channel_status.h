#ifndef _HPLC_CHANNEL_STATUS_H_
#define _HPLC_CHANNEL_STATUS_H_

typedef void(*pVCS_InfoFun)(void);
typedef void(*pVCS_FcFun)(u8);
//初始化
void VCS_Init(void);

//当前信道是否空闲(如果是可以立即发送数据)
bool IsChannelIdle(u8 no);

//接收一帧以重置VCS定时器(不校验)
void VCS_ReceiveFrame(u8 frome,u8* data, u16 len,bool crc);

//冲突指示
void VCS_IndicateConflict(u8 no,pVCS_InfoFun fun);
void VCS_HPLC_Conflict(void);
void VCS_HRF_Conflict(void);
//如果信道空闲了就调用回调
void VCS_IdleInfo(u8 no,pVCS_InfoFun fun);


void VCS_GetFC(u8 no,pVCS_FcFun fun);
void VCS_FCCallBack(u8 frome,u8 isPayload);
void VCS_StopGetFC(u8 no);
extern HPLC_PHY_STATE  (*const getPhyState[2])(void);

#define HRF_EIFS    70 //unit ms
#define HPLC_EIFS   20// unit ms

#define HRF_CIFS    8//unit 100us
#define HPLC_CIFS   40// unit 10us


#define HRF_RIFS    23//unit 100us
#define HPLC_RIFS   230// unit 10us

#endif

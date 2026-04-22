#ifndef _HPLC_NID_H_
#define _HPLC_NID_H_

#define SAME_HRF_SIZE 20

//管理节点附近的不同网络

//网络管理初始化
void NIDInit(void);

//查看一个网络是否允许加入(如果该网络不存在网络管理列表中，返回true)
bool IsNIDVaild(u8 NID);

//加入一个网络
bool InsertNID(u8 NID);

//把一个网络标记为不可加入
bool SetNIDInvaild(u8 NID,u32 lastTime,u8 isCcoRefuse);

//所有网络列表重置为有效
void SetNIDAllVaild(void);

//清空网络列表
void ClearNID(u8);

void SetNIDBand(u8 NID,u8 freq);
void SetNIDChannel(u8 NID,u16 channelIndex);
//获取cco相对本地的相线
EN_PHASE_TYPE getCcoPhase(u8 NID);

//获取CCO的最好相线的snr
s8 getCcoSnr(u8 NID);

//设置CCO的SNR
void SetCcoSnr(u8 NID,u8 phase,int snr,int rssi, u32 beaconPeriod);

//打印所有网络和PCO情况(请用在未入网情况下)
//void PrintAllNID(void);

//获取可以向cco发送的相线
u32 getCcoSendPhae(void);

//获取一个网络的尝试入网时间
u32 GetNIDTryTime(u8 NID);


void SetBplcRecHrIndex(u16 index);
u16 getNextHrfIdex(void);

int SetOtherNetRf(u8 *data,u8 NID);
void SetSameHrf(u8 *mac);
u8 GetSameHrfMac(u8 *mac);
void ClearSameHrf(void);
u8 AllChannelScan(void);
void RstAllChannelScan(void);
u16 getCurrentSearchIndex(void);
void setCurrentHrfIndex(u16 ch);


#endif


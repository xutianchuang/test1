#ifndef _HPLC_OFFLINE_H_
#define _HPLC_OFFLINE_H_

//离线判断模块初始化
void OfflineInit(void);
//接收到抄控器离网
void ReaderConnectOffline(void);
//调用此函数，直接设置成离线状态
void OfflineSetState(void);

//调用此函数，设置成入网状态
void OnlineSetState(void);

//离线判断
bool OfflineReceiveBeacon(u8* mpdu,u16 len);
//其他网络信标离线判断
void OfflineReceiveOtherBeacon(PDU_BLOCK* pdu);
//接收一个发现列表报文
void OfflineReceiveDiscover(u8* discover,u16 len);
void OfflineReceiveHrfDiscover(u16 Tei,u8 Level,u8 Role);
//CCO指示离线
void CCOIndicateOffline(void);

//调试口指示离线
void DebugOffline(void);

//获取上次离线原因
const char* GetLastOfflineReason(void);
char GetLastOfflineReasonNum(void);
//当前入网/离网状态持续时间
u32 GetStateDuration(void);
//写入错误
void saveHplcErrLog(char *errCode , u8 len);
typedef struct {
	u8 tag;
	u8 info;
	u16 data;
}RecError;
#endif

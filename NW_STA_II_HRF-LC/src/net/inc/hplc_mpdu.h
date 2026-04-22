#ifndef _HPLC_MPDU_H_
#define _HPLC_MPDU_H_

#include "hplc_protocol_init.h"

typedef enum{
    FreamPathNull,
    FreamPathBplc=0x1,
    FreamPathHrf=0x2,
    FreamPathBoth=0x3,
}FramePath;

//需要传递的SOF帧内容
typedef struct
{
    u16 SrcTei;
    u16 DstTei;
    u8 Lid;
    u8 BroadcastFlag;
    u8 NID;
    u8 ResendTimes;
    u8 needChangeTMI;
	u8 Link;//FramePath: 1指定载波信道发送 2 指定HRF信道发送     3 HRF信道和载波信道同时发送
	u8 doubleSend;//单拨失败时是否转向另一通道发送
}SOF_PARAM;

//需要传递的信道参数
typedef struct
{
    int32_t Rssi;
    int32_t  Snr;
    u32 sync_ntb;
}BPLC_PARAM;

//发送MAC帧回调函数
typedef void(*pSendMacCallback)(u8 frameType);//0 代表发送失败 FramePath值代表哪个通道发送成功

//处理MPDU帧

//MPDU层初始化
void MpduInit(void);    

//发送MAC帧
bool MpduSendMAC(SOF_PARAM *sofParam,u8* mac,u16 len,pSendMacCallback callback,u32 opt);

//处理MPDU
void MpduHandleMPDU(PDU_BLOCK* pdu);

//处理信标帧
void MpduHandleBeacon(PDU_BLOCK* pdu);

//处理SOF帧
void MpduHandleSOF(PDU_BLOCK* pdu);

//处理FC
void MpduHandleFC(PDU_BLOCK* pdu,u8 crc);

//设置网络号过滤
void MpduSetNID(bool effect,u8 NID);

//设置TEI过滤
void MpduSetTEI(u16 TEI);

//清理fc个数
void ResetFcCnt(void);
//重置切换频段定时器
void ResetChangeFreTimer(void);
void ResetChangeShortFreTimer(void);
void ResetChangeShorterFreTimer(void);

//获取NTB的差值
int64_t GetNTB_Offset(void);

//获取MPDU层信标FC接收个数
u32 GetBeaconFCNum(void);
extern u32 FreTimeout;
extern int64_t OffsetNTB ;
extern u8 TryBand;
extern u16 TryIndex;
extern bool IsBindAddr;

#define DEFULT_CHANGE_FRE_TIME (35*1000ul)
#define SYNC_REC_NTB   FRAMESYNC_OFFSET
#define HRF_SYNC_REC_NTB   HRF_FRAMESYNC_OFFSET

#define APP_CREATE_MAC_DEF() \
MAC_HEAD *macHead = (MAC_HEAD*)data;\
APP_PACKET *appPacket = 0;\
APP_GENERAL_PACKET *appGeneral = 0;\
if(macHead->HeadType)\
    appPacket = (APP_PACKET*)((u8*)macHead + sizeof(MAC_HEAD_SHORT) + sizeof(MSDU_HEAD_SHORT));\
else \
    appPacket = (APP_PACKET*)((u8*)macHead + sizeof(MAC_HEAD_LONG) + sizeof(MSDU_HEAD_LONG));\
appGeneral = (APP_GENERAL_PACKET*)&appPacket->Data[0];

#endif

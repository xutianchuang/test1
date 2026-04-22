#include "os.h"
#include <string.h>
#include "common_includes.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "System_inc.h"
#include "Uart_Link.h"
#include "plc_autorelay.h"
#include "os_cfg_app.h"
#include "../../src/pack2lib/pack2lib.h"


//不可加入网络列表长度
#define MAX_UNAVAILABLE_NUM     16

//上电后从接受到第一个信标开始计时发送关联请求的时间（ms）
#define SEND_ASSOC_TIMEOUT      7000UL

#pragma pack(4)
typedef struct
{
    TimeValue  TimeStamp;
    u32 NID;
}UNAVAILABLE_NET;

//当前尝试加入网络的信息
#pragma pack(4)
typedef struct
{
    bool VaildNID;          //尝试加入的网络是否有效
    u8 AttemptNID[3];       //尝试加入的网络NID
    u8 AttemptCCO_Mac[6];   //CCO MAC
    u8 NetSeq;              //尝试加入网络的序列号
    bool HasSendAssoc;      //是否已经发送了关联请求
}ATTEMPT_NET;

//过零NTB
typedef struct
{
    bool IsCollect;     //是否在采集中
    u32 BaseNTB;        //基准NTB
    u32 LastNTB;        //上一次采集的NTB
    u16 DiffNTB[255];   //NTB差值数组
    u8  CollectNum;     //目前采集的数量
    u8  CollectTotal;   //需要采集的数量
    u8  ColLectPeriod;  //采集周期
    bool IsFinish;      //采集是否已经完成
}ZERO_CROSS;

#define CREATE_MAC_DEF() \
MAC_HEAD *macHead = (MAC_HEAD*)pdu;\
MME_FRAME *mmeFrame = 0;\
if(macHead->MACFlag)\
{\
    mmeFrame = (MME_FRAME*)((u8*)macHead + sizeof(MAC_HEAD));\
}\
else \
{\
    mmeFrame = (MME_FRAME*)((u8*)macHead + sizeof(MAC_HEAD) - 12);\
}

extern const u8 check_mac_head_ver;

bool SaveMap(u16 SrcSeq,u16 *MsduSeq,u8 isNotReboot,u32 *bitmap,u16 bitmapCnt);
bool IsRepeat(u16 SrcSeq,u16 MsduSeq,u8 isNotReboot,u32 *bitmap,u16 bitmapCnt);
bool HasHandleMac(u16 SrcTei,u16 MsduSeq,u16 RebootTimes);

//--------para--------------

static pfMacCallback s_pfMacCallback = NULL;//PLC_MacFrameCallback
static pfMMFrameCallback s_pfMMFrameCallback = NULL;//HPLC_MMFrameCallback
static u16 s_reader_seq=0;
static u32 s_reader_seq_map;
static u16 s_reader_reboot=0xffff;
static OS_MEM UnMempool;
static UNAVAILABLE_NET UnMem[MAX_UNAVAILABLE_NUM];
static ATTEMPT_NET AttemptNet;
static u16 MSDUSequence = 0;
static u32 P2PSeq = 0x12347778;
static ZERO_CROSS ZeroCross;

//-------start-------
static bool IsRepeat(u16 SrcSeq,u16 MsduSeq,u8 isNotReboot,u32 *bitmap,u16 bitmapCnt)
static bool SaveMap(u16 SrcSeq,u16 *MsduSeq,u8 isNotReboot,u32 *bitmap,u16 bitmapCnt)
void GetZeroCross(void)
u16 MacGetMacSequence(void)
u32 MacGetPortSequence(void)

u32 MacCreatePortSequence(void)

UNAVAILABLE_NET* MallocUnavailableNode(void)
void MacInit(void)

bool HasHandleMac(u16 SrcTei,u16 MsduSeq,u16 RebootTimes)
bool SaveHandleMac(u16 SrcTei,u16 MsduSeq,u8 RebootTimes)
void MacHandleMAC(SOF_PARAM *sofParam, BPLC_PARAM* bplcParam, u8* data, u16 len)
void MacSetMacFrameCallback(pfMacCallback macCallback)
void MacSetMMFrameCallback(pfMMFrameCallback mmCallback)


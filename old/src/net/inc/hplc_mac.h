#ifndef _HPLC_MAC_H_
#define _HPLC_MAC_H_

//网络业务

//MAC层初始化
void MacInit(void);

//处理信标帧
void MacHandleBeacon(u8 NID,const BEACON_ZONE* beaconZone,const BEACON_MPDU *beaconMpdu,const BEACON_STATION *beaconSta,const BEACON_ROUTE *BeaconRoute,const BEACON_UTC *BeaconUTC,PDU_BLOCK*pdu);
void MacHandleRetrenchBeacon(u8 NID,const BEACON_HRF_ZONE* beaconZone,const BEACON_RETRENCH_MPDU *beaconMpdu,const BEACON_RETRENCH_STATION *beaconCap,BPLC_recv_para* para);
//校准NTB
void MacCalibrateNTB(u8 NID,PDU_BLOCK* pdu);
//未入网时处理FC
bool MacHandleFC(u8 NID,PDU_BLOCK* pdu);
//处理MAC帧
void MacHandleMAC(SOF_PARAM *sofParam,BPLC_PARAM* bplcParam,u8* data,u16 len);

//处理APP帧
void MacHandleApp(u8* data,u16 len,u8 pri,u16 dst,u8 broadcastType,u8 broadcastDirection,u32 useHopMac);

//获取MAC序列号
u16 MacGetMacSequence(void);

//获取上次发送的端口序列号
u32 MacGetPortSequence(void);

//生成一个新的端口序列号
u32 MacCreatePortSequence(void);
//递减停止组网时间
void MacDecJoinTime(void);
//重置停止组网时间
void MacReStopJoin(void);
//获取当前是否允许组网
bool MacGetAllowNet(void);
//设置地址类型
void SetMACType(int mode);
//当前尝试加入网络的信息
#pragma pack(4)
typedef struct
{
	bool VaildNID;          //尝试加入的网络是否有效
	u8 AttemptNID;       //尝试加入的网络NID
	u16 AttemptPCO;         //尝试以哪个PCO做代理
	u8 AttemptCCO_Mac[6];   //CCO MAC
	u8 NetSeq;              //尝试加入网络的序列号
	bool HasSendAssoc;      //是否已经发送了关联请求
	u32  RandFlag;          //用于标识此次入网请求
}ATTEMPT_NET;
struct HrfSearch_s
{
	u32 Tick; //hrf 上一次搜索的Tick
	u32 Remain; //单独HRF搜索的剩余时间

	u32 ExtRemain;
	u16 ExtIndex;  //由HPLC或Flash存储上一次的入网Index
	u16 ExtFc;
	u8 ExtUseLoneRemain;

	u8 useLoneRemain;
	u16 getFc;
};
//清空尝试入网参数
void ClearAttemptNet(void);

//发送探测信标
void SendDetectBeaconFrame(void*);
void ClearHandleMacCco(void);
bool IsPoreSequence(u32 seq);


//进行确认搜索HRF的时间
void HrfSearchProc(void);
void HrfGetFc2LongRemain(u16 index);
//由HPLC带来的无线频段设置
void HrfSetExtIndex(u16 index,u8 force);
//强制停止额外的hrf搜网
void HrfStopExtSearch(void);
void AppOneHopHandleData(BPLC_PARAM *bplcParam, u8* data,u16 len);
extern ATTEMPT_NET AttemptNet;
extern struct HrfSearch_s HrfSearch;

void RstLastChannelIndex(void);

void SetFactoryMsg(u8 *msg, u8 len);

#define UNKONW_CHANNEL 0xffff
#define UNKONW_BAND    0xff
#endif

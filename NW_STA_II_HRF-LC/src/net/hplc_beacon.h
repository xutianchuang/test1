#ifndef _HPLC_BEACON_H_
#define _HPLC_BEACON_H_

//每秒NTB走的个数
#define NTB_FRE_PER_SEC     25000000UL

//每毫秒NTB走的个数
#define NTB_FRE_PER_MS      25000UL

//每微秒NTB走的个数
#define NTB_FRE_PER_US      25UL

//信标时隙
typedef struct
{
    u32 BeginNTB;   //开始NTB
    u32 EndNTB;     //结束NTB
}BEACON_SLOT_NTB;

//时隙信息
typedef struct
{
    BEACON_SLOT_NTB ProxySlot;      //发送代理信标的时隙
    bool ProxyVaild;
    BEACON_SLOT_NTB DiscoverSlot;   //发送发现信标的时隙
    bool DiscoverVaild;
    
    BEACON_SLOT_NTB PhaseASlot[32];     //A相线CSMA时隙
    u8 PhaseASlotNum;
    BEACON_SLOT_NTB PhaseBSlot[32];     //B相线CSMA时隙
    u8 PhaseBSlotNum;
    BEACON_SLOT_NTB PhaseCSlot[32];     //C相线CSMA时隙
    u8 PhaseCSlotNum;
    
    u32 CSMA_BeginNTB;
    u32 CSMA_EndNTB;
}SLOT_INFO;


//最近收到的一条信标帧中是否要求本站点发送发现信标
bool IsNeedSendDiscover(void);

//最近收到的一条信标帧中是否要求本站点发送代理信标
bool IsNeedSendProxy(void);

//信标周期
u32 GetBeaconPeriodLen(void);

//获取发现信标时隙
bool GetDiscoverSlot(u32 *beginNTB,u32 *endNTB);

//获取代理信标时隙
bool GetProxySlot(u32 *beginNTB,u32 *endNTB);

//根据NTB获取相线
u8 GetNTB_Phase(u32 NTB);

//根据最新的信标帧，获取一信标帧进行转发
bool GetBeaconFrame(int beaconType,BEACON_STATION* beaconCap,u8 *beacon,u16 *len);

//获取最新信标帧的载荷
void GetBeaconMpduPtr(const BEACON_MPDU **);

//获取最新站点能力
void GetBeaconAbilityPtr(const BEACON_STATION **);

//获取最新信标帧的路由参数
void GetBeaconRoutePtr(const BEACON_ROUTE **);

//获取最新信标帧的频段
void GetBeaconFrequencePtr(const BEACON_FREQUENCE **);

//获取最新信标帧的时隙
void GetBeaconSlotPtr(const BEACON_SLOT **);

//获取最新信标帧可变区域
void GetBeaconZone(const BEACON_ZONE **);
#endif

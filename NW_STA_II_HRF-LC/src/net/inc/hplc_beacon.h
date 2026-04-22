#ifndef _HPLC_BEACON_H_
#define _HPLC_BEACON_H_

#define NTB2TIMER_TICK      2

//每毫秒NTB走的个数
#define NTB_FRE_PER_MS      25000UL

//每微秒NTB走的个数
#define NTB_FRE_PER_US		25UL

//100微秒NTB走的个数
#define NTB_FRE_PER_100US   2500UL

//单个相线最大CSMA时隙总数
#define PHASE_MAX_SLOT_NUM			512


//单位信标时隙长度NTB
#define BEACON_SLOT_LEN_NTB     2500

//单位CSMA时隙分片大小NTB
#define CSMA_CHIP_SLOT_LEN_NTB  2500UL

//单位TDMA时隙长度NTB
#define TDMA_SLOT_LEN_NTB       2500

//单位信标周期的时间长度NTB
#define BEACON_PERIOD_LEN_NTB   2500

//单位CSMA时隙长度NTB
#define CSMA_SLOT_LEN_NTB       2500

//CSMA时隙单位，原协议是100微妙，新协议是10毫秒，做以下换算匹配100微妙为单位
#define CSMA_SLOT_LEN_UNIT      ((10*1000)/100)

typedef enum
{
	LINK_HPLC_ONLY = 0,//仅发HPLC信标
	LINK_HRF_ONLY,//仅发HRF标准信标
	LINK_NORMAL,//发Hplc和hrf标准信标
    LINK_RETRENCH,//发HPLC和HRF精简信标
    LINK_CSMA,//发HPLC和在CSMA时隙发送hrf精简信标
    LINK_SAME_NORMAL,//在同一时刻发送hplc信标和hrf标准信标
    LINK_SAME_RETRENCH,//在同一时刻发送hplc信标和hrf精简信标    
}EN_RF_BEACON_TYPE;
//信标时隙
typedef struct
{
    u32 BeginNTB;   //开始NTB
    u32 EndNTB;     //结束NTB
}BEACON_SLOT_NTB;

//信标时隙(相线)
typedef struct
{
    u32 BeginNTB;   //开始NTB
    u32 EndNTB;     //结束NTB
    u8  Phase;      //相线
}BEACON_SLOT_PHASE;

//CSMA时隙
typedef struct
{
    BEACON_SLOT_NTB PhaseASlot[PHASE_MAX_SLOT_NUM];     //A相线CSMA时隙
    u16 PhaseASlotNum;
    BEACON_SLOT_NTB PhaseBSlot[PHASE_MAX_SLOT_NUM];     //B相线CSMA时隙
    u16 PhaseBSlotNum;
    BEACON_SLOT_NTB PhaseCSlot[PHASE_MAX_SLOT_NUM];     //C相线CSMA时隙
    u16 PhaseCSlotNum;
        
	u8 CsmaNoSlot;// csma 还没有详细的时隙分配信息
    u32 CSMA_BeginNTB;
    u32 CSMA_EndNTB;
}CSMA_SLOT_INFO;

//时隙信息
typedef struct
{
    int64_t OffsetNTB;              //NTB偏移
    u32 BeaconPeriodCnt;               //信标周期计数
    u32 BeaconPeriod;               //信标周期长度(1ms)
    BEACON_SLOT_NTB ProxySlot;      //发送代理信标的时隙
    bool ProxyVaild;
	bool DiscoverVaild;
    u8   HrfBeacon;                 //发送HRF信标 类型  EN_RF_BEACON_TYPE
    BEACON_SLOT_NTB DiscoverSlot;   //发送发现信标的时隙
	BEACON_SLOT_NTB HrfSlot;        //HRF发送信标的时隙

    BEACON_SLOT_NTB TDMA_Slot;      //TDMA时隙
    bool TDMA_Vaild;                
    u8 TDMA_Lid;                    //TDMA lid
    u8 TieCSMA_Lid;                 //绑定CSMAlid
    CSMA_SLOT_INFO  CSMA_Slot;      //CSMA时隙信息
    CSMA_SLOT_INFO  TieCSMA_Slot;   //绑定CSMA时隙信息
    u32 NextBeaconEnd;
}SLOT_INFO;

typedef struct
{
    u32 tick;                //tick
    u8 CCOMacAddr[6];       //CCO mac地址
}OTHER_BEACON;
extern OTHER_BEACON otherNetBeacon[16];

typedef enum
{
	SYNC_NTB_SUCCESS,		//校准成功
	SYNC_NTB_FIRST_DATA,	//接收第一组数据
	SYNC_NTB_LAST_ONE_SEC,	//少于一秒
	SYNC_NTB_OVER_TIME,		//校准时间间隔过长
	SYNC_NTB_RETURN_ERROR,	//校准函数返回值异常
}SYNC_NTB_ERROR;


//处理一条信标帧 返回是否需要处理
bool HandleBeacon(u8* beacon,u16 len,int64_t offset,u32 frameLenMs);
void HandleRetrenchBeacon(u8* beacon,u16 len,int64_t offset);
//最近收到的一条信标帧中是否要求本站点发送发现信标
bool IsNeedSendDiscover(void);

//最近收到的一条信标帧中是否要求本站点发送代理信标
bool IsNeedSendProxy(void);
u8 IsHrfBeacon(void);
//信标周期
u32 GetBeaconPeriodLen(void);

//获取发现信标时隙
bool GetDiscoverSlot(u32 *beginNTB,u32 *endNTB);

//获取代理信标时隙
bool GetProxySlot(u32 *beginNTB,u32 *endNTB);

//获取hrf信标
bool GetHrfSlot(u32 *beginNTB,u32 *endNTB);

//获取时隙信息
bool GetSlotInfo(SLOT_INFO *SlotInfo);

//根据最新的信标帧，获取一信标帧进行转发
u16 GetBeaconFrame(int beaconType,BEACON_STATION* beaconCap,u8 *beacon,u16 *len);

//获取精简信标站点信息条目
void GetBeaconCapPtr(BEACON_RETRENCH_STATION **RetrenchBeaconCap);
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

//获取最新万年历信息
void GetBeaconUTCPtr(const BEACON_UTC **beaconUtc);

//获取最新信标帧可变区域
void GetBeaconZone(const BEACON_ZONE **);

//NTB校准函数
SYNC_NTB_ERROR BeaconCalibrateNTB(u32 BeaconNTB,u32 SynNTB);
SYNC_NTB_ERROR HrfBeaconCalibrateNTB(u16 TargetTei, u32 BeaconNTB,u32 SynNTB,s32 HrfFreq,s32 *PllFreq);
//立即进行下一次校准
void ResetCalibrateNTB(void);
void ResetHrfCalibrateNTB(void);
//是否已经校准
bool HasCalibrateNTB(void);

//获取当前NTB值
u32 GetCurrentNTB(void);
//CCO是否正在改切无线频段
u8 IsChangeChannel(void);

//根据信标帧长返回使用何种分集拷贝模式
DIV_COPY_MODEL GetBeaconCopyModel(u16 len);
int GetBeaconItem(u8 *beacon, BEACON_STATION **beaconCap, BEACON_ROUTE **beaconRoute, BEACON_FREQUENCE **beaconFrq, BEACON_SLOT **beaconSlot, BEACON_DETECT_LIST **beaconDetect,BEACON_COPY **beaconCopy,BEACON_UTC **beaconUTC,BEACON_RF_PARA **beaconRfPara,BEACON_RF_CHANNEL **beaconRfChannel,u16 len);
#define INVAILD_LID  0xffff
extern u16 BcsmaLid;
extern SLOT_INFO SlotInfo;
extern u8  currectFreq;
extern u8  detectFreq;
extern uint32_t startFrqChangeTick;


extern void ResetNtbFlag(void);

#endif

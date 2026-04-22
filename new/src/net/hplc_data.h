#ifndef _HPLC_DATA_H_
#define _HPLC_DATA_H_

#include "os.h"

#define CCO_TEI 1
#define READER_TEI   0xFFD
#define TRIM_TEI     0xFCF
#define	LONG_ADDR_LEN 6
#define NID_BYTE_WIDTH 3
#define CHIP_ID_BYTE_WIDTH 24

#define MAX_PARENT_NUM 5
#define MAX_ROUTE_NUM 1999
#define MAX_NEIGHBOR_NUM MAX_ROUTE_NUM

#define MIN_TEI_VALUE 0
#define MAX_TEI_VALUE 1999

#define MIN_LAYER_VALUE 0
#define MAX_LAYER_VALUE 15

#define READER_CONNECT_TICK  (OSCfg_TickRate_Hz * 60 *3)
typedef enum
{
    NET_OUT_NET,        //未入网
    NET_IN_NET,         //已入网
    NET_OFFLINE_NET,    //离线
	NET_STATE_LIMIT_VALUE,
}EN_NET_STATE_TYPE;

typedef enum
{
	UNKNOWN_ROLE = 0,
	STA_ROLE,
	PCO_ROLE,
	RESERVED_ROLE,
	CCO_ROLE,
	ROLE_LIMIT_VALUE,
}EN_ROLE_TYPE;

typedef enum
{
	LINK_HPLC_ONLY = 0,//仅发HPLC信标
	LINK_HRF_ONLY,//仅发HRF信标
	LINK_NORMAL,//发HRF和标准信标
    LINK_RETRENCH,//HRF和精简信标
    LINK_CSMA,//HRF的信标等到CSMA时隙再发送
    LINK_NORMAL_SAME_TIME,//同时发HRF和标准信标
    LINK_RETRENCH_SAME_TIME//同时发HRF和精简信标
}EN_RF_BEACON_TYPE;

typedef enum
{
	UNKNOWN_LINE = 0,
	A_LINE,
	B_LINE,
	C_LINE,
	LINE_LIMIT_VALUE,
}EN_PHASE_TYPE;

//typedef enum
//{
//	CONTROLLER = 1,
//	CONCENTRATOR,
//	METER,
//	RELAY,
//	II_COLLECTOR,
//	I_COLLECTOR,
//	DEVICE_LIMIT_VALUE,
//}EN_DEVICE_TYPE;

typedef enum
{
	NORMAL_RESET = 0,
	POWER_ON_RESET,
	WATCHDOG_RESET,
	EXCEPTION_RESET,
	RESET_LIMIT_VALUE,
}EN_RESET_TYPE;

//上报成功率信息字段
#pragma pack(1)
typedef struct
{
	u16 TEI;
	u8 DownSucRate;
	u8 UpSucRate;
}ST_SUC_RATE_TYPE;

//单个邻居节点信息如下
#pragma pack(4)
typedef struct
{
	u16 TEI;
	u8 PcoSucRate;
//上1路由周期统计计数	
	u8	LastNghTxCnt;
	u8	LastNghRxCnt;

//本路由周期统计计数
	u8  RebootTimes;
	u16 MsduSeq;
	u32 seqBitMap;
}ST_NEIGHBOR_TYPE;

////邻居索引表，按照TEI从小到大顺序排列
//typedef struct 
//{
//	u16 TEI;
//	u16 Idx;
//}ST_NEIGHBOR_IDX_TYPE;
#pragma pack(4)
typedef struct 
{
	u16 NeighborNum;
	u16 TEI[MAX_NEIGHBOR_NUM];
	ST_NEIGHBOR_TYPE Neighbor[MAX_NEIGHBOR_NUM];
}ST_NEIGHBOR_TAB_TYPE;

#pragma pack(1)
typedef struct
{
	u16 TEI;
	u8 Layer;
}ST_PARENT_TYPE;

#pragma pack(1)
typedef struct
{
	u8 ParentNum;
	ST_PARENT_TYPE Parent[MAX_PARENT_NUM];
}ST_PARENT_TAB_TYPE;

#pragma pack(1)
typedef struct
{
	u16 TEI;
	u16 RelayTEI;
}ST_ROUTE_TYPE;

#pragma pack(1)
typedef struct
{
	u16 RouteNum;
	ST_ROUTE_TYPE Route[MAX_ROUTE_NUM];	
}ST_ROUTE_TAB_TYPE;

#pragma pack(1)
typedef struct
{
	u8 NetState;
	u16 TEI;
	u8 Mac[LONG_ADDR_LEN];
	u8 CcoMac[LONG_ADDR_LEN];
	u8 NID[NID_BYTE_WIDTH];
	EN_DEVICE_TYPE DeviceType;
	u8 FreqBand;
	u8 NetBuildSeq;
	u8 Layer;
	EN_ROLE_TYPE Role;
	EN_PHASE_TYPE Phase;
}ST_STA_ATTR_TYPE;

#pragma pack(1)
typedef struct
{
	u16 Year  :7;
	u16 Month :4;
	u16 Day	  :5; 
}ST_SYS_DATE_TYPE;

#pragma pack(1)
typedef struct 
{
	EN_RESET_TYPE LastRstReason;
	u8 BootVer;
	u8 SofeVer[2];
	ST_SYS_DATE_TYPE VerDate;
	u8 FactoryID[2];
	u8 ChipCode[2];
}ST_STA_VER_TYPE;

#pragma pack(1)
typedef struct 
{
	u32 AssocRandom;
	u8 ChipFactoryId[CHIP_ID_BYTE_WIDTH];
	u16 HardRstCnt;
	u16 SofeRstCnt;
	ST_STA_VER_TYPE StaVerInfo;
}ST_STA_SYS_TYPE;

#pragma pack(1)
typedef struct
{
	u16 RoutingPeriod;//s
	u16 HeartBeatPeriod;//s
	u16 SucRatePeriod;//s
	u16 DiscListPeriod;//s
	u8 LastPeriodDiscListTxCnt;
	u8 DiscListTxCnt;
}ST_STA_PERIOD_TYPE;


//=============================================================================
void RestoreFactorySet(void);

void InitStaData(void);

void StorageStaAttrInfo(void);

void StorageStaParaInfo(void);

void StorageStaRouteInfo(void);
//=============================================================================
//数据加载和初始化
void StaDataInit(void);

//返回网络基础数据可读写指针
const ST_STA_ATTR_TYPE * GetStaBaseAttr(void);

//返回周期参数可读写指针
const ST_STA_PERIOD_TYPE * GetStaPeriodPara(void);

//返回系统参数可读写指针
const ST_STA_SYS_TYPE * GetStaSysPara(void);

//设置网络状态
bool SetNetState(EN_NET_STATE_TYPE netState);
//设置CCO地址
bool SetCCO_Mac(u8 addr[LONG_ADDR_LEN]);
//设置本站点MAC地址
bool SetStaMAC(u8 addr[LONG_ADDR_LEN]);
//设置本站点TEI
bool SetTEI(u16 TEI);
//设置本站点代理TEI
bool SetPCO_TEI(u16 TEI);
//设置本站点层级
bool SetLayer(u8 layer);
//设置本站点角色
bool SetRole(EN_ROLE_TYPE role);
//设置网络NID
bool SetNID(u8 NID[NID_BYTE_WIDTH]);
//设置组网序列号
bool SetNetSequence(u32 net);
//设置本站点相线
bool SetPhase(EN_PHASE_TYPE phase);
//设置频段
bool SetSequence(u8 seq);

//设置路由周期
bool SetRoutePeriod(u16	RoutePeriod);
//设置发现列表周期
bool SetDiscoverPeriod(u16 DiscoverPeriod);
//设置心跳周期
bool SetHeartBeatPeriod(u16 HeartbeatPeriod);
//设置成功率上报周期
bool SetSuccessPeriod(u16 SuccessPeriod);
//设置成功率
bool SetRate(u16 tei,u8 uprate,u8 downrate);



//获取接口



//获取邻居位图(bitmap表示位图,返回值为实际字节数)
u8 GetNeighborBitmap(u8 *bitmap, u16 *ngh_num);




//获取指定邻居节点信息
bool GetNeighborInfo(u16 tei, ST_NEIGHBOR_TYPE** ngh_info);

//获取邻居站点表(table为指向邻居表的指针,返回邻居的个数)
u16 GetNeighborTable(const ST_NEIGHBOR_TYPE** table);





//设置接口
#define SET_NEIGHBOR_TEI				(1UL<<0)





#define SET_NEIGHBOR_ROUTE_MIN_RATE		(1UL<<7)


#define SET_NEIGHBOR_PCO_RATE			(1UL<<10)

#define SET_NEIGHBOR_LAST_TX_CNT		(1UL<<12)
#define SET_NEIGHBOR_LAST_RX_CNT		(1UL<<13)

//#define SET_NEIGHBOR_TX_CNT				(1UL<<15)
//#define SET_NEIGHBOR_RX_CNT				(1UL<<16)
#define SET_NEIGHBOR_MSDU_SEQ			(1UL<<15)
#define SET_NEIGHBOR_BEACON_CNT			(1UL<<16)

#define SET_NEIGHBOR_REBOOTTIMES        (1UL<<18)
//设置一个邻居节点(TEI为邻居节点TEI,neighbor为值,flags为上述宏定义的或值,如果哪一位上置位，则neighbor对应的相应字段有效)
//如果不存在TEI，则在表中新增一个项，返回false，调用方认为TEI不合法，或者参数存在错误。
bool SetNeighbor(u16 TEI,const ST_NEIGHBOR_TYPE* neighbor,u32 flags);

//设置主代理站点(参数同上)
bool SetMainParent(u16 TEI,const ST_NEIGHBOR_TYPE* neighbor,int flags);

/*------------------路由表操作----------------------*/
//获取路由下一跳(参数：要到达的最终目的TEI。返回下一跳TEI，如没有路由，返回0)
u16 GetRelayTEI(u16 TEI);

//设置路由信息(最终目标TEI,中继TEI)
bool SetRoute(u16 DstTEI,u16 RelayTEI);

//清除所有路由表和邻居数据
void CleanAllData(void);


/*------------------其他数据----------------------*/
void GetSelfMSG(u8 msg[18]);	//获取厂家自定义信息

ST_STA_VER_TYPE* GetVersionMSG(void);	//获取厂家版本信息
#pragma pack(4)

extern ST_NEIGHBOR_TAB_TYPE StaNeighborTab;
#endif

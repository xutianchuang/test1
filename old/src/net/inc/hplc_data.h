#ifndef _HPLC_DATA_H_
#define _HPLC_DATA_H_

#include "os.h"
#include "common_includes.h"
#include <time.h>
//#define WUQI_CCO

#define CCO_TEI 1
#define TRIM_TEI     0xFCF
#define READER_TEI 0xffd
#define	LONG_ADDR_LEN 6
#define NID_BYTE_WIDTH 3
#define CHIP_ID_BYTE_WIDTH 24

#define MAX_PARENT_NUM 5
#define MAX_ROUTE_NUM 1015
#define MAX_NEIGHBOR_NUM MAX_ROUTE_NUM

#define MIN_TEI_VALUE 0
#define MAX_TEI_VALUE 1015

#define MIN_LAYER_VALUE 0
#define MAX_LAYER_VALUE 15

#define MAX_CLC_ROUT_PERIOD  1

//#define FIND_MIN_TEI_NODE

#define LEVEL_DEC_RATE			85 //90%

#define LEVEL_DEC_RATE_BITS     8
#define LEVEL_DEC_RATE_0        256

#define SAME_LEVEL_RATE         70*LEVEL_DEC_RATE_0/100

#define LEVEL_DEC_RATE_1        LEVEL_DEC_RATE*LEVEL_DEC_RATE_0/100
#define LEVEL_DEC_RATE_2        ((LEVEL_DEC_RATE_1*LEVEL_DEC_RATE_1)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_3        ((LEVEL_DEC_RATE_1*LEVEL_DEC_RATE_2)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_4        ((LEVEL_DEC_RATE_2*LEVEL_DEC_RATE_2)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_5        ((LEVEL_DEC_RATE_2*LEVEL_DEC_RATE_3)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_6        ((LEVEL_DEC_RATE_3*LEVEL_DEC_RATE_3)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_7        ((LEVEL_DEC_RATE_3*LEVEL_DEC_RATE_4)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_8        ((LEVEL_DEC_RATE_4*LEVEL_DEC_RATE_4)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_9        ((LEVEL_DEC_RATE_4*LEVEL_DEC_RATE_5)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_10       ((LEVEL_DEC_RATE_5*LEVEL_DEC_RATE_5)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_11       ((LEVEL_DEC_RATE_5*LEVEL_DEC_RATE_6)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_12       ((LEVEL_DEC_RATE_6*LEVEL_DEC_RATE_6)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_13       ((LEVEL_DEC_RATE_6*LEVEL_DEC_RATE_7)>>LEVEL_DEC_RATE_BITS)

#define UP_NEED_PRO_FLASH    0x11223344
#define STA_VERSION_FLAG     0x99990002
extern const u16 level_dec_rate[14];

typedef enum
{
    NET_OUT_NET,        //未入网
    NET_WAIT_NET,       //等待入网
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
	UNKNOWN_LINE = 0,
	A_LINE,
	B_LINE,
	C_LINE,
	LINE_LIMIT_VALUE,
}EN_PHASE_TYPE;

typedef struct{
	u32 len;
	u32 flag;
}UPDATA_PARA;
typedef struct {
	u32 flash_addr;
	u32 item_flag;
	u32 erase_size;
	u32 data_size;
	u8 data[0];
}UPDATA_ITEM;

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

//成功率信息
#pragma pack(4)
typedef struct
{
    u8 UpRate;      //上行通信成功率
    u8 DownRate;    //下行通信成功率
    u8 SuccessRate; //通信成功率
    bool Flag;      //成功率有效标志
}ST_SUC_RATE_DATA;
#define HRF_BIT_MAP_SIZE 16
//单个邻居节点信息如下
#pragma pack(4)
typedef struct
{
//	u16 TEI;                //TEI
	u8 Mac[LONG_ADDR_LEN];  //MAC
	EN_ROLE_TYPE Role;      //角色
	EN_PHASE_TYPE Phase;    //相线
	u8 Layer;               //层级
	s8 SNR;                 //信噪比
	s8 RSSI;                //信号强度
	u8 RouteMinSucRate;     //路径最小通信成功率
	//u32 RouteEstSucRate;     //路径评估成功率
	u8 SnrCount;           //信噪比更新次数
	u8 PcoSNR;              //代理站点与此站点通信信噪比
	u8 PcoSucRate;          //代理站点与此站点的通信成功率
	u8 PcoDownSucRate;      //代理站点与此站点通信的下行通信成功率

	s16 snrSum;             //信噪比和
	u16 PcoTEI;             //代理站点TEI
	
//无线相关参数
	u8 Sta2CcoRfHop:4;        //到cco经过hrf转发的跳数
	u8 GetHplc:1;
	u8 GetHrf:1;
	s8 RfSnr;               //hrf SNR值
	s8 RfRssi;              // hrf RSSI
	s8 ReportSnr;           //对方告知我方的信噪比
	s8 ReportRssi;          //对方告知我方的Rssi 
	u8 HrfSeq;              //无线节点发现列表 序号
	//u8 NoListCnt;           //连续未收到发现列表的个数
	u8 HrfUpRate;           //hrf上行成功率 （需老化
	u8 HrfNoCnt;            //hrf 未更新上行成功率的cnt
	u16 HrfBitmap;          //hrf统计成功率的bitmap
	//u32 HrfUpTick;          //hrf 更新bitmap的tick 
	//u32 HrfRouteEstSucRate; 	//Hrf路径评估成功率
//上1路由周期统计计数	
	u8	LastNghTxCnt[MAX_CLC_ROUT_PERIOD];       //上个路由周期邻居节点发送发现列表报文的个数
	u8	LastNghRxCnt[MAX_CLC_ROUT_PERIOD];       //上个路由周期邻居站点接收到本站点的发送报文个数
	u8  headNghIdx;//idx 指向当前周期得到的上一周期的参数
	u8  headNghCnt;//cnt 存储的是上上个路由周期之前存储了几个参数
	u8	LastTheStaRxCnt[MAX_CLC_ROUT_PERIOD];    //上个路由周期本站点接收到的报文个数
    u8  headStaCnt;
	u8  headStaIdx;
	u8  TheStaRxCnt;	    //本站点接收到的报文个数
	u8 PeriodCount;        //与之通信的路由周期个数

	
	//----------------------------//
	//入网相关
	bool CannotConnect:1;		//CCO命令禁止连接PCO
	bool NoAckFlag:1;			//入网无响应标志
	u8   ConnectTryTimes;	//入网尝试次数

	ST_SUC_RATE_DATA  SucRate[2];   //[0]、[1]分别为最近两次的成功率
    TimeValue DiscoverTimer;       //此定时器用于在自己路由周期到期数据转存后，在此定时器时没超时，收到的发现列表帧不统计成功率
    TimeValue DiscoverTimer2;        //此定时器用于在自己路由周期到期数据转存后，在超过此定时器后，收到的发现列表帧不统计成功率


	TimeValue ConnectTimer;	//入网后无响应的定时器
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
	u16 TEI[MAX_ROUTE_NUM];//tei存储表
	u32 TEI_MAP[MAX_BITMAP_SIZE/4];//tei检索表
	ST_NEIGHBOR_TYPE Neighbor[MAX_NEIGHBOR_NUM];
}ST_NEIGHBOR_TAB_TYPE;

#pragma pack(4)
typedef struct
{
	u16 TEI;
	u8 Layer;
}ST_PARENT_TYPE;

#pragma pack(4)
typedef struct
{
	u8 ParentNum;
	ST_PARENT_TYPE Parent[MAX_PARENT_NUM];
}ST_PARENT_TAB_TYPE;

#pragma pack(4)
typedef struct
{
	//	u16 TEI;
	u16 RelayTEI:12;
	u16 RelayType:4;
	u16 CandTEI:12;//备选TEI用于存储代理变更请求的TEI
	u16 CandType:4;
	u32 seqBitMap;
	u16 MsduSeq;            //最近的一个MSDU序号
    u8  RebootTimes;        //重启次数
}ST_ROUTE_TYPE;

#pragma pack(4)
typedef struct
{
//	u16 RouteNum;
//	u16 RouteArray[MAX_ROUTE_NUM];		//用于统计有多少条路由
	ST_ROUTE_TYPE Route[MAX_ROUTE_NUM];	
}ST_ROUTE_TAB_TYPE;

#pragma pack(4)
typedef struct
{
	u8 NetState;                //自身网络状态(未入网、等待入网、已入网、离线)
	u16 TEI;                    //TEI
	u8 Mac[LONG_ADDR_LEN];      //MAC
	u8 CcoMac[LONG_ADDR_LEN];   //CCO的MAC
	u8 NID;                     //网络NID
	u8 connectReader;           //是否链接抄控器
	u8 ReaderLink;              //抄控器链接的信道 0:HPLC  1:HRF
	u16 HrfOrgChannel;          //抄控器连接之前的channel
	EN_DEVICE_TYPE DeviceType;  //设备类型
	u8 NetBuildSeq;             //组网序列号
	u8 Layer;                   //层级
    u8 NetFlag;                 //组网标志(0:未完成,1:已完成)
	u8 RfHop;                   //经过rf的跳数
	EN_ROLE_TYPE Role;          //角色
	EN_PHASE_TYPE Phase;        //所属相线
	u32 startTick;
	u32 lastTick;
	u8 ReaderSeq;
	u8 PcoTmi;             //PCO 发送信标使用的hplc频段
	u8 PcoPhrMCS;               //PCO 发送信标的 phr MCS
	u8 PcoMCS;                  //PCO 发送信标使用的HRF MCS
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
	//模块硬件版本信息
	u8 ModuleHardVer[2];
	ST_SYS_DATE_TYPE ModuleHardVerDate;

	//芯片硬件版本信息
	u8 ChipHardVer[2];
	ST_SYS_DATE_TYPE ChipHardVerDate;

	//芯片软件版本信息
	u8 ChipSoftVer[2];
	ST_SYS_DATE_TYPE ChipSofVerDate;

	//应用程序版本号
	u8 AppVer[2];
}ST_STA_EXT_VER_TYPE;

#pragma pack(1)
typedef struct
{
	u8 mac[6];
	u8 factoryID[2];
	u8 ver[2];
	u8 chipCode[2];
	u8 year;
	u8 month;
	u8 day;
    u8 assetsCode[24]; //资产编码
}ST_STA_ID_TYPE;

extern ST_STA_ID_TYPE StaChipID;
extern ST_STA_VER_TYPE StaVersion;
extern ST_STA_EXT_VER_TYPE StaExtVersion;

#pragma pack(4)
typedef struct 
{
	u16 HardRstCnt;
	u16 SofeRstCnt;
	u32 AssocRandom;
	//u8 ChipFactoryId[CHIP_ID_BYTE_WIDTH];
	//ST_STA_VER_TYPE StaVerInfo;
//	u16 IsPowerOff;
	u8 LastMeter[LONG_ADDR_LEN];   //记录上一次入网的meter
	u8 LastCcoMac[LONG_ADDR_LEN];   //记录上一次入网的CCO的MAC
	u8 LastFreq;                    //记录上一次入网的频段
	u16 LastRFChannel;				//记录上一次入网的无线信道
}ST_STA_SYS_TYPE;
#pragma pack(1)
typedef struct
{
	u8 res[45];
	u8 baud; //波特率，0-1200;1-2400;2-4800;3-9600;4-38400
	u8 ledMode; //LED闪灯模式，0->原方案，1->南网下一代电表方案
	u8 txPowerMode[2]; //0表示发射功率自动切换模式，默认使用HPLC_LinePower作为HPLC_ChlPower
	                //1表示发射功率固定模式，使用flash里存储的txPower作为HPLC_ChlPower
	u8 txPower[4];
	u8 hrfTxPower[2];
}ST_STA_FIX_PARA;
extern ST_STA_SYS_TYPE StaSysPara;

#pragma pack(4)
typedef struct
{
	u16 RoutingPeriod;//s           //路由周期
	u16 HeartBeatPeriod;//s         //心跳周期
	u16 SucRatePeriod;//s           //成功率上报周期
	u16 StaDiscListPeriod;//s          //发现列表周期
	u16 StaProxyChangePeriod;//s		//代理变更周期
    u16 PcoDiscListPeriod;//s          //代理站点发现列表周期
	u16 HrfDiscListPeriod;//s          //hrf 发现列表周期
    u32 BeaconPeriodCnt;               //信标周期计数
    u32 ReceiveBeaconCnt;               //收到信标个数
	u8 LastPeriodDiscListTxCnt[MAX_CLC_ROUT_PERIOD];     //上个路由周期发送发现列表个数
	u8 headCnt;
	u8 headIdx;
	u8 DiscListTxCnt;               //这个路由周期发送发现列表个数
	u8 OldCnt;                      //老化周期个数
	//u8 FirstBeacIsRetrench;         //本周期第一次接到的信标是精简信标
}ST_STA_PERIOD_TYPE;
typedef struct tm  Time_Special;
typedef struct {
	u8 isVaild;//0 无效 1 读到并且无效  2 表时间 3校准表时间 4集中器时间
	u32 sec_form_1900;//自2000年到现在过去的秒数
	Time_Special time;
	u32 sysTick;//更新时刻的系统tick
	s32 diff_tick;
}Time_Module_s;

#define MAC_GRAPH_CONFIG_NUM 50

typedef struct{
	u8 config_flag;
	u8 num[2];//数据项
	u8 mode[2][MAC_GRAPH_CONFIG_NUM]; //使能，1开始，0关闭（间隔为0时）
	u8 period[2][MAC_GRAPH_CONFIG_NUM];//min
	u32 ident[2][MAC_GRAPH_CONFIG_NUM];
}Graph_Config_s;
typedef struct {
	u32 all_used;//该块区域是否全部使用完毕的falg 若是0xffffffff则该区域没有使用完毕 
				 //操作时需要在最后一次才可以写入以防止重复编程的问题
	u32 end_sec; //结束时的时间戳			 
	u32 have_data;//该块区域是否有数据的flag
	u32 used_num;//该块区域使用的序号，每一次使用新块的序号加1
	u32 sum_befor;//前面两个的和
	u8 data[0];
}Flash_4K_Area;

typedef struct{
	u32 sec_from_1900;//自1900年到现在过去的秒数
	u32 ident;
	u8 len;
	u8 cs;
	u8 data[0];
}Graph_Data_s;

typedef enum {
	DATE_YEAR,
	DATE_MON,
	DATE_DATE,
	DATE_HOUR,
	DATE_MIN,
	DATE_SEC,
}DATE_ENUM;
typedef struct
{
	u8 option;
	u8 channel;
}HRF_Reader_Channel;

//=============================================================================
void RestoreFactorySet(void);

void InitStaData(void);

void StorageStaAttrInfo(void);

void StorageStaParaInfo(void);
void ReadStaParaInfo(void);

void StorageStaRouteInfo(void);
void StorageStaResetTime(u16 soft_cnt,u16 hard_cnt);
void StorageStaIdInfo(void);
void ReadStaIdInfo(void);
void FlushAllFlash(void);
//=============================================================================
//数据加载和初始化
void StaDataInit(void);

//返回网络基础数据可读写指针
const ST_STA_ATTR_TYPE * GetStaBaseAttr(void);

//返回周期参数可读写指针
const ST_STA_PERIOD_TYPE * GetStaPeriodPara(void);

//返回系统参数可读写指针
const ST_STA_SYS_TYPE * GetStaSysPara(void);

const ST_STA_VER_TYPE * GetStaVerInfo(void);

const ST_STA_EXT_VER_TYPE* GetStaExtVerInfo(void);

//设置网络状态
bool SetNetState(EN_NET_STATE_TYPE netState);
//设置CCO地址
bool SetCCO_Mac(u8 addr[LONG_ADDR_LEN]);
//设置入网CCO地址，保存在flash中
bool SetLastCCOMac(u8 addr[LONG_ADDR_LEN]);
//设置入网频段，保存在flash中
bool SetLastFreq(u8 freq);
//设置入网无线信道，保存在flash中
bool SetLastRFchannel(u16 ch);
//设置波特率
bool SetBaud(u8 baud);
//设置本站点MAC地址
bool SetStaMAC(u8 addr[LONG_ADDR_LEN]);
//记录上一次入网的电表地址
bool SetStaLastMeter(void);
//设置本站点TEI
bool SetTEI(u16 TEI);
//设置本站点代理TEI
bool SetPCO_TEI(u16 TEI);
//设置本站点层级
bool SetLayer(u8 layer);
//设置本站Hrf跳数
bool SetRfHop(u8 hop);
//设置本站点角色
bool SetRole(EN_ROLE_TYPE role);
//设置网络NID
bool SetNID(u8 NID);
//设置组网序列号
bool SetNetSequence(u32 net);
//设置本站点相线
bool SetPhase(EN_PHASE_TYPE phase);
//设置频段
bool SetSequence(u8 seq);
//设置组网标志
bool SetNetFlag(u8 flag);


//设置无线老化周期个数
bool SetHrfOldPeriodCnt(u16 OldCnt);
//设置路由周期
bool SetRoutePeriod(u16	RoutePeriod);
//设置STA发现列表周期
bool SetStaDiscoverPeriod(u16 DiscoverPeriod);
//设置PCO发现列表周期
bool SetPcoDiscoverPeriod(u16 DiscoverPeriod);
//设置心跳周期
bool SetHeartBeatPeriod(u16 HeartbeatPeriod);
//设置成功率上报周期
bool SetSuccessPeriod(u16 SuccessPeriod);
//设置代理变更周期
bool SetProxyChangePeriod(u16 ProxyPeriod);
//设置HRF发现列表周期
bool SetHrfDiscoverPeriod(u16 ProxyPeriod);
//设置信标周期计数
bool SetBeaconPeriodCnt(u32 BeaconPeriodCnt);
//增加收到信标个数
bool IncreaseBeaconCnt(void);
//设置本周期第一次收到的信标是否是精简信标
//void SetFirstRetrenchFlag(int flag);
//设置PCO的信标使用的TMI
void SetPcoBeaconTmi(u8 tmi);
//设置PCO的信标使用的phr和psdu的mcs
void SetPcoBeaconMcs(u8 phr, u8 psdu);
//获取接口

//获取代理节点表(最多5个,返回值为实际数组个数)
u8 GetParentPCO(u16 *pco_arr);
u8 GetHrfParentPCO(u16 *pco_arr);
int GetBatterParentPCO(u16 *pco_arr);
//设置固定的选取父节点优先级 此函数仅在机台测试时有效
void SetFixLinkParent(u8 link);
//获取代理变更节点
void GetProxyParent(u16 *pco_arr,u8 link);

int GetBatterProxyPCO(u16 pco_arr[2][5]);
//获取邻居位图(bitmap表示位图,返回值为实际字节数)
u8 GetNeighborBitmap(u8 *bitmap, u16 *ngh_num,u16 *max_tei);

//获取发现列表信息(receivePacket为邻居节点接收的发现列表报文数量数组，max指示receivePacket数组的最大值，返回指示实际的数组长度)
u16 GetReceiveNeighbor(u8 *receivePacket, u16 max,u8 *bitmap,u16 bitmapSize);
//获取发现列表个数
u16 GetNeighborNum(void);

//获取主代理站点信息
const ST_NEIGHBOR_TYPE* GetPCO_Info(void);

//获取邻居的TEI
u16 GetPCOTei(void);
//获取指定邻居节点信息
bool GetNeighborInfo(u16 tei, ST_NEIGHBOR_TYPE** ngh_info);

//获取邻居表
const ST_NEIGHBOR_TAB_TYPE* GetNeighborTable(void);

//获取子站点表(table为指向子站点表的指针,返回子站点的个数)
u16 GetChildSucRateTab(ST_SUC_RATE_TYPE* table);

//在上一路由周期结束时转存数据处理
void DataProcessAtEndRoutePeriod(u8 SendDisNum);

//获取邻居站点上下行通信成功率（上个路由周期）,getLast标志说明如果没有获取到上个路由周期的通信成功率，尝试获取上上个路由周期
bool GetNeighborSucRate(u16 TEI,u8 *upRate,u8 *downRate,u8 *sucRate,bool getLast,u8 link);
//过滤发现列表的函数 调用的地方都在Timer的task中因此nei_bitmap无需做临界资源互斥
void* NeighborFilter(int hplc_hrf,int send_num,int*bitmap_size);

//计算通信成功率
bool SetNeighborSucRate(u16 TEI,u16 sendNum,u16 receiveNum);
//获取评估通信成功率
u32 GetEstSucRate(u8 link, u16 tei);
//设置接口
#define SET_NEIGHBOR_TEI				(1UL<<0)
#define SET_NEIGHBOR_MAC				(1UL<<1)
#define SET_NEIGHBOR_ROLE				(1UL<<2)
#define SET_NEIGHBOR_PHASE				(1UL<<3)
#define SET_NEIGHBOR_LAYER				(1UL<<4)
#define SET_NEIGHBOR_SNR				(1UL<<5)
#define SET_NEIGHBOR_RSSI				(1UL<<6)
#define SET_NEIGHBOR_ROUTE_MIN_RATE		(1UL<<7)
#define SET_NEIGHBOR_PCO_TEI			(1UL<<8)
#define SET_NEIGHBOR_PCO_SNR			(1UL<<9)
#define SET_NEIGHBOR_PCO_RATE			(1UL<<10)
#define SET_NEIGHBOR_PCO_DOWNRATE 		(1UL<<11)
#define SET_NEIGHBOR_LAST_TX_CNT		(1UL<<12)
#define SET_NEIGHBOR_LAST_RX_CNT		(1UL<<13)
//#define SET_NEIGHBOR_LAST_STA_RX_CNT	(1UL<<14)
//#define SET_NEIGHBOR_TX_CNT				(1UL<<15)
//#define SET_NEIGHBOR_RX_CNT				(1UL<<16)
//#define SET_NEIGHBOR_MSDU_SEQ			(1UL<<15)
#define SET_NEIGHBOR_BEACON_CNT			(1UL<<16)
#define SET_NEIGHBOR_STA_RX_CNT			(1UL<<17)
//#define SET_NEIGHBOR_REBOOTTIMES        (1UL<<18)
#define SET_NEIGHBOR_HRF_SNR            (1<<19)
#define SET_NEIGHBOR_HRF_RSSI           (1<<20)
#define SET_NEIGHBOR_HRF_HOP            (1<<21)
#define SET_NEIGHBOR_HRF_REPORT_RSSI    (1<<22)
#define SET_NEIGHBOR_HRF_REPORT_SNR     (1<<23)
#define SET_NEIGHBOR_HRF_REPORT_RATE    (1<<24)


#define HRF_INVAILD_CHANNEL_INFO        0x7f
//设置一个邻居节点(TEI为邻居节点TEI,neighbor为值,flags为上述宏定义的或值,如果哪一位上置位，则neighbor对应的相应字段有效)
//如果不存在TEI，则在表中新增一个项，返回false，调用方认为TEI不合法，或者参数存在错误。
bool SetNeighbor(u16 TEI,const ST_NEIGHBOR_TYPE* neighbor,u32 flags);

//删除邻居
bool DeleteNeighbor(u16 TEI);

//清除所有邻居的入网相关参数
void CleanAllNeighborEntryNetPara(void);

//设置主代理站点(参数同上)
bool SetMainParent(u16 TEI,const ST_NEIGHBOR_TYPE* neighbor,int flags);

//设置不可连接PCO
bool SetCannotConnectPCO(u16 TEI);

//设置连接无响应PCO
bool SetNoAckPCO(u16 TEI,u32 timeout);
void clearPCOTryTime(u16 TEI);
//判断一个PCO是否可以连接
bool IsPCOVaild(u16 TEI);


/*------------------路由表操作----------------------*/
//获取路由下一跳(参数：要到达的最终目的TEI。返回下一跳TEI，如没有路由，返回0)
u16 GetRelayTEI(u16 TEI);
u16 GetRelayTeiType(u16 tei);
u8 GetRelaySendLink(u16 tei);
//设置路由信息(最终目标TEI,中继TEI)
bool SetRoute(u16 DstTEI, u16 RelayTEI,u8 RoutType);

//设置备选路由信息(最终目标TEI,中继TEI)
bool SetCandRoute(u16 DstTEI, u16 RelayTEI,u8 RoutType);
//设置优选路由信息 若没有主路由设置主路由 若有主路由设置备选路由(最终目标TEI,中继TEI)
bool SetPreferenceRoute(u16 DstTEI,u16 RelayTEI,u8 RoutType);
//删除路由
bool DeleteRoute(u16 TEI);

//清除所有路由表和邻居数据
void CleanAllData(void);
//清除周期性数据
void CleanPeriodData(void);
//使用默认值初始化网络参数
void InitAttrData(void);

void SetHrfBit(u16 tei, u8 seq);
//获取站点的hrf下行通信成功率
u8 GetHrfDownRate(u16 tei);
/*------------------其他数据----------------------*/
void GetSelfMSG(u8 msg[18]);	//获取厂家自定义信息


extern u32 IsPowerOff;
extern u32 PLL_Vaild;
extern int PLL_Trimming;

void StorageStaFixPara(void);
int ReadStaFixPara(void);
void StorageVerInfo(void);
void ReadVerInfo(void);
void StorageExtVerInfo(void);
void ReadExtVerInfo(void);
void ConfigCSG21DeafultGraphPara(void);
void ReConfigCSG21GraphPara(void);
void StorageGraphPara(void);
void EraseGraph(void);

void StoragePowerOffFlag(void);
void ReadPowerOffFlag(void);
void StoragePllTrim(int trim);
void ReadPllTrim(void);
void             SYS_UP_ReWrite(UPDATA_PARA* para);
void OpenOfflineSendSlot(void);
void DummyBeacon(void);
void MakeBeacon(void);

void StopSendSync(void);

void ReadStaParaInfoOnly(void);
void StaTxPowerProcess(void);
void StaRebootCntProcess(void);
void StaLastFreqProcess(void);
extern Time_Module_s Time_Module[3];
extern Graph_Config_s  Graph_Config;
extern ST_STA_FIX_PARA StaFixPara;
void StorageTime(void);
Graph_Data_s * FindStaGraphData(u32 sec_from_2000,u32 ident);
void StorageGraphData(Graph_Data_s *pdata);
u32 Data2Sec(Time_Special* ptime);
void DateSpecialChangeSec(Time_Special* ptime,s32 sec);
bool DffSecIn24H(Time_Special* ptimea,Time_Special* ptimeb);
int Diff_Specialtime(Time_Special* ptimea,Time_Special* ptimeb);
void EraseGraphAtTime(u32 time);
bool SetGraphConfig(u8 config);

void StaLastRFChannelProcess(void);//读取flash中站点上一次入网的无线信道


void GetNewReader(u8 link);
void StopSendSync(void);
void StartSendSync(u8 broad,u8 seq,u8 hrfindex,u8 pdu_type);

//HRF更新信道信息
u8* UpNeighChannelInfo(u16 scrTei,u8 type, u8 *data, u16 *bitStart,u8 isBit,u16 tei);
//HRF写发送列表信道信息
u16  WiteNeighborInfo(u8 type,u8 bit_type,u8 *src_data,s16 len,u32 *bitmap);
//周期老化上行功率
void HrfOldUpRate(void* arg);
extern ST_STA_FIX_PARA StaFixPara;
extern int defultAddrCnt;
extern u32 HPLCCurrentFreq;
extern ST_NEIGHBOR_TAB_TYPE StaNeighborTab;
extern const HRF_Reader_Channel HrfBroadChannel[14];
extern TimeValue PowerChangeTimer;

#endif

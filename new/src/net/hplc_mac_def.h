#ifndef _HPLC_MAC_DEF_H_
#define _HPLC_MAC_DEF_H_

#define NID_INVALID     0
#define NID_MIN         1

#if defined(HPLC_CSG)
#define NID_MAX         15
#else
#define NID_MAX         0xffffff
#endif

#define TEST_NID        0

#define TEI_INVALID     0
#define TEI_CCO         1
#define TEI_MIN         1
#define TEI_STA_MIN     2
#if 1
#define TEI_MAX         1999
#define TEI_STA_MAX     TEI_MAX
#define TEI_CNT         2000
#else
#define TEI_MAX         499
#define TEI_STA_MAX     TEI_MAX
#define TEI_CNT         500
#endif
#define TEI_BIT_CNT     11

#define TEI_BROADCAST   0xFFF

#define MAX_MAC_FRAME_LEN      2078     // 2076
#define MAX_MSDU_LEN           2046
#define MAC_CHECK_SUM_LEN      4
#define MAC_ADDR_LEN           6


#define MAX_MPDU_FRAME_LEN     2096

#if defined(HPLC_CSG)
#define MAX_BEACON_PAYLOAD_LEN  512
#else
#define MAX_BEACON_PAYLOAD_LEN  513
#endif
#define MAX_SOF_PAYLOAD_LEN     516     //516 x 4 = 2064, MAC帧最大有2078字节，分包传输

//链路标识符，0-3报文优先级，4-254业务分类LID
//优先级数值越大表示该报文越优先
#define SOF_LID_ASSOC_REQ               254     //关联请求
#define SOF_LID_ASSOC_CNF               254     //关联确认
#define SOF_LID_ASSOC_IND               1       //关联指示
#define SOF_LID_ASSOC_GATHER_IND        1       //关联汇总指示
#define SOF_LID_DISCOVER_LIST           1       //发现列表
#define SOF_LID_OFF_LINE_IND            3       //离线指示
#define SOF_LID_PCO_CHANGE_CNF          1       //代理变更确认
#define SOF_LID_ZERO_CROSS_COLLECT      1       //5.1.3.13 过零 NTB 采集指示报文

#define SOF_LID_READ_METER              3       //抄表
#define SOF_LID_TIMING                  3       //校时
#define SOF_LID_EVENT_CFG               3       //事件上报应答
#define SOF_LID_COMM_TEST               3       //通信测试
#define SOF_LID_AUTHENTICATION          3       //鉴权安全 3
#define SOF_LID_SELF_REG                2       //从节点主动注册
#define SOF_LID_UPDATE                  2       //升级报文
#define SOF_LID_AREA_DISCERNMENT        3       //台区识别
#ifdef GDW_ZHEJIANG
#define SOF_LID_ONLINE_LOCK_TIME        3       //在网锁定时间
#endif
#define SOF_LID_DETECT_BCSMA            77

#if defined(HPLC_CSG)

#define SOF_LID_X4_READ_TASK_INIT       3       //采集任务
#define SOF_LID_X4_READ_TASK_ID         3       //采集任务
#define SOF_LID_X4_READ_TASK_DETAIL     3       //采集任务
#define SOF_LID_X4_DELETE_READ_TASK     3       //采集任务
#define SOF_LID_X4_ADD_READ_TASK        3       //采集任务
#define SOF_LID_X4_READ_COLL_TASK_DATA  3       //采集任务


#define SOF_LID_MAINTENANCE_NET         3
#define VLAN_APP_MIN                    0
#define VLAN_APP_MAX                    3
#define VLAN_MM                         0x8100  //网络管理子层
#endif
#define MAX_NW_PARALLEL_NUM  15

extern const u8 BROADCAST_MAC_ADDR[];

//MAC帧头
#pragma pack(1)

#if defined(HPLC_CSG)
#define HRF_ONE_HOP_VERSION   2
typedef struct
{
    u16 HeadType:1;      //帧头类型
    u16 Version:2;       //版本
    u16 Reserve0:13;     //保留
    u16 MsduLen;         //MSDU长度
    u32 DstTei:12;       //目的TEI    
    u32 SrcTei:12;       //源TEI
    u32 NID:4;           //短网络标识
    u32 RebootNum:4;     //重启次数
    u8  RouteLeft:4;     //路由剩余跳数
    u8  Broadcast:4;     //广播方向
    u8  SendType:3;      //发送类型
    u8  SendLimit:5;     //发送次数限值    
    u16 MsduSeq;         //MSDU序号

}MAC_HEAD_SHORT_CSG, *P_MAC_HEAD_SHORT_CSG;

#else
#define HRF_ONE_HOP_VERSION   1
#endif

typedef struct
{
#if defined(HPLC_CSG)

    //长帧头、短帧头相同部分
    u16 HeadType:1;      //帧头类型
    u16 Version:2;       //版本
    u16 Reserve0:13;     //保留
    u16 MsduLen;         //MSDU长度
    u32 DstTei:12;       //目的TEI    
    u32 SrcTei:12;       //源TEI
    u32 NID:4;           //短网络标识
    u32 RebootNum:4;     //重启次数
    u8  RouteLeft:4;     //路由剩余跳数
    u8  Broadcast:4;     //广播方向
    #if 1
    u8  SendType:3;      //发送类型
    u8  SendLimit:5;     //发送次数限值
    #else
    u8  SendType:4;      //发送类型
    u8  SendLimit:4;     //发送次数限值
    #endif
    u16 MsduSeq;         //MSDU序号

    //长帧头部分
    u8  DstMac[MAC_ADDR_LEN];      //目标MAC地址
    u32 Reserve1;        //保留
    u8  Reserve2[10];     //保留

#else

    u16 Version:4;      //版本
    u16 SrcTei:12;      //源TEI     2
    u16 DstTei:12;      //目的TEI
    u16 SendType:4;     //发送类型  4
    u8  SendLimit:4;    //发送次数限值
    u8  Reserve0:4;     //保留      5
    u16 MsduSeq;        //MSDU序号
    u8  MsduType;       //MSDU类型  8
    u16 MsduLen:11;     //MSDU长度
    u16 RebootNum:4;    //重启次数
    u16 PCORouteFlag:1;    //代理主路径标识 10
    u8  RouteSum:4;     //路由总跳数
    u8  RouteLeft:4;    //路由剩余跳数      11
    u16 Broadcast:2;    //广播方向
    u16 RouteRepair:1;  //路径修复标识
    u16 MACFlag:1;      //MAC地址标识
    u16 Reserve1:12;    //保留              13
    u8  NetSeq;        //组网序号
    u8  Reserve2;       //保留
    u8  Reserve3;       //保留              16
    u8  SrcMac[6];      //原始MAC地址
    u8  DstMac[6];      //目标MAC地址

#endif

}MAC_HEAD, *P_MAC_HEAD;

typedef struct
{
#if !defined(HPLC_CSG)
    u8 Version:4;      //版本
    u8 :4;          //保留
    u8 MsduType;       //MSDU类型
    u16 MsduLen:11;
    u16 :5;
#else

    u16 HeadType:1;    //帧头类型
    u16 Version:2;      //版本
    u16 :5;          //保留
	u16 MsduType:8;
    u16 MsduLen;
    //后接短valn标签
    //u8 Vlan;
    //u8 MsduType;       //MSDU类型
#endif
}MAC_HEAD_ONE_HOP;

typedef struct
{
    u8  DstMac[MAC_ADDR_LEN];      //原始目的地址
    u8  SrcMac[MAC_ADDR_LEN];      //原始源地址
    u32 Vlan;
    u16 MsduType;                  //MSDU类型
}MSDU_HEAD_LONG_CSG, *P_MSDU_HEAD_LONG_CSG;

typedef struct
{
    u8 Vlan;
    u8 MsduType;                  //MSDU类型
}MSDU_HEAD_SHORT_CSG, *P_MSDU_HEAD_SHORT_CSG;

typedef struct
{
    MAC_HEAD head;
    unsigned char body[MAX_MSDU_LEN+MAC_CHECK_SUM_LEN];
}MAC_FRAME, *P_MAC_FRAME;

#if defined(HPLC_CSG)
//帧头类型
typedef enum
{
    MAC_HEAD_LONG,
    MAC_HEAD_SHORT,
    MAC_HEAD_HOP,//单跳
}MAC_HEAD_TYPE;

//发送类型
typedef enum
{
    SENDTYPE_SINGLE,            //单播， 需要确认回应
    SENDTYPE_BROADCAST,         //全网广播， 不需要回应
    SENDTYPE_LOCAL,             //本地广播， 不需要回应
    //SENDTYPE_PCOBROADCAST,     //代理广播
    SENDTYPE_BROADCAST_ACK,     //全网广播， 需要确认回应
    SENDTYPE_LOCAL_ACK,         //本地广播， 需要确认回应

    SENDTYPE_PCOBROADCAST = SENDTYPE_BROADCAST,  //代理广播

}MAC_SEND_TYPE;

#else

//发送类型
typedef enum
{
    SENDTYPE_SINGLE,        //单播
    SENDTYPE_BROADCAST,     //全网广播
    SENDTYPE_LOCAL,         //本地广播
    SENDTYPE_PCOBROADCAST,  //代理广播
}MAC_SEND_TYPE;

#endif

//代理主路径标识
enum
{
    PCO_ROUTE_NONE,          //未启用代理主路径
    PCO_ROUTE_USED,          //启用代理主路径
};

//广播方向
typedef enum
{
#if defined(HPLC_CSG)
    BROADCAST_DRT_NONE=0,
    BROADCAST_DRT_DUL = 1,  //0  1     //双向广播
    BROADCAST_DRT_DOWN = 1,     //下行广播  
    BROADCAST_DRT_UP,           //上行广播
#else
    BROADCAST_DRT_DUL,          //双向广播  
    BROADCAST_DRT_DOWN,         //下行广播  
    BROADCAST_DRT_UP,           //上行广播
#endif
}MAC_BROADCAST_DIRECTION;

//路径修复标志
typedef enum
{
    ROUTE_REPAIR_NONE,           //当前报文未触发过路径修复
    ROUTE_REPAIR_TRIGGLE,        //当前报文触发过路径修复
}ROUTE_REPAIR_TYPE;

//MAC地址标识
typedef enum
{
    MACFLAG_NONE,           //未携带MAC地址
    MACFLAG_INCLUDE,        //携带了MAC地址
    MACFLAG_HOP,            //单跳
}MAC_FLAG_e;

//MSDU类型
#pragma pack(4)

typedef enum
{
#if defined(HPLC_CSG)
    MSDU_APP_FRAME = 0x0001,         //应用层报文
	MSDU_NEIGHBOR ,                  //无线发现列表报文
    MSDU_NET_MANAGER = 0x88E1,       //网络管理消息

    MSDU_ONE_HOP_APP_FRAME = 1,    //应用层报文
#else
    MSDU_NET_MANAGER,       //网络管理消息
    MSDU_APP_FRAME = 48,    //应用层报文
    MSDU_IP_FRAME,          //IP报文
    //国网没有下面的内容  为了保证编译通过
    MSDU_NEIGHBOR=0,           //发现列表报文
    MSDU_ONE_HOP_APP_FRAME = 128,    //应用层报文
    MSDU_ONE_HOP_IP_FRAME,        //IP报文
#endif
}MSDU_TYPE;
//typedef enum
//{
//    #ifndef HPLC_CSG
//    MSDU_NEIGHBOR,           //发现列表报文
//    MSDU_ONE_HOP_APP_FRAME = 128,    //应用层报文
//    MSDU_ONE_HOP_IP_FRAME,          //IP报文
//    #else
//    MSDU_ONE_HOP_APP_FRAME = 1,    //应用层报文
//    MSDU_NEIGHBOR=0x88,           //发现列表报文
//    MSDU_ONE_HOP_IP_FRAME,          //IP报文
//    #endif
//}MSDU_ONE_HOP_TYPE;

#pragma pack(1)
//CRC32校验码
typedef struct
{
    u8 byte0;
    u8 byte1;
    u8 byte2;
    u8 byte3;
}_CRC_32_;

#pragma pack(1)
typedef struct
{
    u8 byte0;
    u8 byte1;
    u8 byte2;
}_CRC_24_;

//定界符类型
typedef enum
{
    MPDU_TYPE_BEACON,       //信标帧
    MPDU_TYPE_SOF,          //SOF帧
    MPDU_TYPE_CONFIRM,      //选择确认帧
    MPDU_TYPE_CONCERT,      //网间协调帧
}EN_MPDU_TYPE;

//相线
typedef enum
{
    PHASE_UNKNOW,       //未知
    PHASE_A,            //A相
    PHASE_B,            //B相
    PHASE_C,            //C相
    PHASE_ALL,          //ABC相
}HPLC_PHASE;
//相线
typedef enum
{
    SEND_PHASE_UNKNOW=1<<PHASE_UNKNOW,  //未知
    SEND_PHASE_A=1<<PHASE_A,            //A相
    SEND_PHASE_B=1<<PHASE_B,            //B相
    SEND_PHASE_C=1<<PHASE_C,            //C相
    SEND_PHASE_ALL=1<<PHASE_ALL,        //ABC相
}HPLC_SEND_PHASE;
#define PHASE_DEFAULT PHASE_UNKNOW

//信标类型
enum
{
    BEACON_DISCOVER,        //发现信标
    BEACON_PROXY,           //代理信标
    BEACON_CENTER,          //中央信标
};

//组网标志位
enum
{
    NET_COMPLETE,           //组网完成
    NET_UNCOMPLETE,         //组网未完成
};

//信标使用标志
enum
{
    NO_ALLOW_USE_BEACON,       //不允许使用信标进行信道评估
    ALLOW_USE_BEACON,           //允许使用信标进行信道评估
};

//角色定义
typedef enum
{
    ROLES_UNKNOWN,
    ROLES_STA,
    ROLES_PCO,
    ROLES_CCO = 4,
	ROLES_RELAY = 0xA,
}NODE_ROLES;

typedef enum
{
#if defined(HPLC_CSG)
    OFFLINE_NOT_INNET,          //0x0 CCO 判定站点未入网， 但是接收到来自站点的除关联请求之外的报文。
    OFFLINE_OVER_LEVEL = 2,     //0x2 CCO 判断网络拓扑的层级超过上限。
    OFFLINE_NOT_IN_WHITE_LIST,  //0x3 CCO 判断站点不在最新的白名单中，用延迟离线指示报文
    OFFLINE_NOTIFY,             //0x4 CCO 通知站点立即离线
#else
    OFFLINE_NOTIFY,             //CCO 通知站点立即离线
    OFFLINE_OVER_LEVEL,         //CCO 判断网络拓扑的层级超过上限
    OFFLINE_NOT_IN_WHITE_LIST,  //CCO 判断站点不在最新的白名单中
#endif

    OFFLINE_REASON_NUM
}OFFLINE_REASON;


//信标帧载荷 
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u8  BeaconType:3;        //信标类型
    u8  NetFlag:1;           //组网标志(0:未完成,1:完成)
	u8  Retrench:1;          //精简信标标志
    u8  MultiNetFlag:1;      //多网络优选功能开关
    u8  RelateFlag:1;        //关联标志位(0:不允许站点发送关联请求,1:)
    u8  Reserve2:1;          //保留
    u8  NetSeq;              //组网序列号
    u32 NID:4;               //短网络标识
    u32 Option:2;
	u32 Reserve1:2;          //保留
    u32 Channel:8;             //无线网络编号
    u32 Reserve3:16;        //保留
    u8  Data[MAX_BEACON_PAYLOAD_LEN+8];          //载荷+校验+保留+校验
#else
    u8  BeaconType:3;        //信标类型
    u8  NetFlag:1;           //组网标志(0:未完成,1:完成)
	u8  Retrench:1;          //精简信标标志
	u8  Reserve1:1;           //保留
    u8  RelateFlag:1;        //关联标志位(0:不允许站点发送关联请求,1:)
    u8  BeaconUsedFlag:1;    //信标使用标志位
    u8  NetSeq;              //组网序列号
    u8  CCOMacAddr[6];       //CCO mac地址
    u32 BeaconPeriodCounter; //信标周期计数
	u8  Channel;             //无线信道编号
	u8  Option;              //无线OPtion
	u16 Reserve2;           //保留
    u32 Reserve3;           //保留
    u8  Data[MAX_BEACON_PAYLOAD_LEN+7];          //载荷+校验
#endif
}BEACON_MPDU, *P_BEACON_MPDU;
//精简信标载荷
#ifndef HPLC_CSG
typedef struct
{
    u8 BeaconType:3;        //信标类型
    u8 NetFlag:1;           //组网标志(0:未完成,1:完成)
    u8 Retrench:1;          //精简信标标志
    u8 Reserve1:1;           //保留
    u8 RelateFlag:1;        //关联标志位(0:不允许站点发送关联请求,1:)
    u8 BeaconUsedFlag:1;    //信标使用标志位
    u8 NetSeq;              //组网序列号
    u8 CCOMacAddr[6];       //CCO mac地址
    u32 BeaconPeriodCnt;       //信标周期计数
    u8  Data[0];          //载荷+校验
}BEACON_RETRENCH_MPDU;
#else
typedef struct
{
    u8 BeaconType:3;        //信标类型
    u8 NetFlag:1;           //组网标志(0:未完成,1:完成)
    u8 Retrench:1;          //精简信标标志
    u8 Reserve:1;           //保留
    u8 RelateFlag:1;        //关联标志位(0:不允许站点发送关联请求,1:)
    u8 BeaconUsedFlag:1;    //信标使用标志位
    u8 NetSeq;              //组网序列号
	u8 CCOMacAddr[6];
    u32 BeaconPeriodCnt;       //信标周期计数
    u8  Data[0];          //载荷+校验
}BEACON_RETRENCH_MPDU;
#endif
//信标管理信息格式 
#pragma pack(1)
typedef struct
{
    u8 BeaconNum;           //信标条目数
    u8 Data[512];
}BEACON_MANAGER_MSG, *P_BEACON_MANAGER_MSG;

//信标条目头
enum
{
#if defined(HPLC_CSG)
    BEACON_ITEM_STATION = 0x01,         //站点能力条目
    BEACON_ITEM_TIME,                   //时隙分配条目

    BEACON_ITEM_ROUTE = 0x06,           //路由参数条目
    BEACON_ITEM_FRE,                    //频段变更条目
	BEACON_ITEM_HPLC_READER = 0x09,      //用于抄控器
	BEACON_ITEM_DETECT=   0xa,           //探测频段头
	BEACON_ITEM_UTC=      0xb,           //万年历条目
	BEACON_ITEM_RF_ROUT,        //无线路由参数条目
	BEACON_ITEM_RF_CHANNEL,     //无线信道变更条目
    BEACON_ITEM_RF_RETRENCH,    //精简信标 站点&时隙条目
#else
    BEACON_ITEM_STATION,        //站点能力条目
    BEACON_ITEM_ROUTE,          //路由参数条目
    BEACON_ITEM_FRE,            //频段变更条目
    BEACON_ITEM_RF_ROUT,        //无线路由参数条目
    BEACON_ITEM_RF_CHANNEL,     //无线信道变更条目
    BEACON_ITEM_RF_RETRENCH,    //精简信标 站点&时隙条目
	BEACON_ITEM_UTC=      0xb,           //万年历条目    
    BEACON_ITEM_TIME = 0xC0,    //时隙分配条目
#endif
};

//站点能力条目 
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u8  Level:6;        //层级数
    u8  PhaseLine:2;    //站点所属相线
    u16 TEI:12;         //站点TEI
    u16 Role:4;         //角色
    u8  BeaconUsedFlag; //信标使用标志位
    u8  MacAddr[MAC_ADDR_LEN];     //信标发送站点MAC地址
    u16 PCOTei:12;      //代理站点TEI
    u16 RfHop:4;        //经过的HRF跳数
    u32 MinSuccess;     //站点到CCO整个路径的最低成功率
    u32 Reserve2;      //保留
#else
    u32 TEI:12;         //站点TEI
    u32 PCOTei:12;      //代理站点TEI
    u32 MinSuccess:8;   //站点到CCO整个路径的最低成功率
    u8  MacAddr[6];     //信标发送站点MAC地址
    u8  Role:4;         //角色
    u8  Level:4;        //层级数
    s8  PCO_SNR;        //代理站点信道质量
    u8  PhaseLine:2;    //站点所属相线
    u8  RfHop:4;        //经过的HRF跳数
    u8  Reserve:2;      //保留
#endif
}BEACON_STATION, *P_BEACON_STATION;

typedef struct
{
    u32 TEI:12;         //站点TEI
    u32 PCOTei:12;      //代理站点TEI
    u32  Role:4;         //角色
    u32  Level:4;        //层级数

    u8  MacAddr[6];     //信标发送站点MAC地址
	u8  RfHop:4;        //经过的HRF跳数
#if defined(HPLC_CSG)
	u8  Band:2;         //载波频段
    u8  Reserve:2;      //保留
#else
    u8  Reserve:4;      //保留
#endif
	u32  CsmaStart;     //CSMA开始NTB
    u16  CsmaLen;       //CSMA时隙长度

}BEACON_RETRENCH_STATION;
//路由参数条目 
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u16 RoutePeriod;        //路由周期
    u16 Reserve1;
    u16 LeftTime;           //距离下次路由评估的时间
    u8  Reserve2[20];
    u8  CCOMacAddr[MAC_ADDR_LEN];
#else
    u16 RoutePeriod;        //路由周期
    u16 LeftTime;           //距离下次路由评估的时间
    u16 PCOInterval;        //代理站点发送发现列表报文的间隔
    u16 STAInterval;        //发现站点发送发现列表报文的间隔
#endif
}BEACON_ROUTE, *P_BEACON_ROUTE;



//频段通知条目 
#pragma pack(1)
typedef struct
{
    u8  TargetFre;          //目标频段
    u32 LeftTime;           //距离实施频段切换剩余时间
}BEACON_FREQUENCE, *P_BEACON_FREQUENCE;

//无线路由参数条目
typedef struct
{
    u8 RfListPeriod;          //无线发现列表周期
	u8 OldCnt;           //老化周期个数
}BEACON_RF_PARA;
//无线信道变更条目
typedef struct
{
    u8  Channel;            //无线信道
#if defined(HPLC_CSG)
	u8  option:2;             //无线option
	u8  res:6;      
	u32 LeftTime;           //距离实施频段切换剩余时间
#else
	u32 LeftTime;           //距离实施频段切换剩余时间
    u8  option;             //无线option
#endif
}BEACON_RF_CHANNEL;

typedef struct
{
	u8 Freq;
	u8 Lid;
}BEACON_DETECT,*P_BEACON_DETECT;


typedef struct
{
	u32 frome_2000_sec;
	u32 NTB;
}UTC_TIME,*P_UTC_TIME;
//时隙分配条目 
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u8  NotCenterSlotNum;       //非中央信标时隙总数
    u8  CenterSlotNum;          //中央信标时隙总数
    u8  CSMAPhase;              //CSMA支持的相线个数
    u8  PCOBeaconNum;           //代理信标时隙总数
    u16 SlotLen;                //每个信标时隙占用的时隙长度，100 微秒
    u8  CSMASlotSplitSize;      //CSMA时隙分片大小, 10 毫秒
    u8  TieCSMAPhaseNum;        //绑定CSMA时隙支持的相线个数
    u8  TieCSMALid;             //绑定CSMA时隙支持的业务报文
    u16 TDMASlotLen;            //TDMA时隙长度, 100 微秒
    u8  TDMALid;                //TDMA报文
    u32 NTB;                    //信标周期开始时刻NTB值
    u32 BeaconPeriod;           //信标周期时间长度, 100 微秒
    u32 RfSlot:10;              //hrf信标时隙长度
    u32 Reserve1:22;               //保留
    u8  Data[1];                //包含非中央信标信息、CSMA时隙信息、绑定CSMA时隙信息
#else
    u8 NotCenterSlotNum;        //非中央信标时隙总数
    u16 CenterSlotNum:4;        //中央信标时隙总数
    u16 CSMAPhase:2;            //CSMA支持的相线个数
    u16 Reserve1:10;            //保留
    u8  PCOBeaconNum;           //代理信标时隙总数
    u8  SlotLen;                //每个信标时隙占用的时隙长度，1MS
    u8  CSMASlotSplitSize;      //CSMA时隙分片大小,10MS
    u8  TieCSMAPhaseNum;        //绑定CSMA时隙支持的相线个数
    u8  TieCSMALid;             //绑定CSMA时隙支持的业务报文
    u8  TDMASlotLen;            //TDMA时隙长度，1MS
    u8  TDMALid;                //TDMA报文
    u32 NTB;                    //信标周期开始时刻NTB值
    u32 BeaconPeriod;           //信标周期时间长度,1MS
    u16 RfSlot:10;              //hrf信标时隙长度
    u16 Reserve2:6;               //保留
    u8  Data[1];                //包含非中央信标信息、CSMA时隙信息、绑定CSMA时隙信息
#endif
}BEACON_SLOT, *P_BEACON_SLOT;

//非中央信标信息字段 
#pragma pack(1)
typedef struct
{
    u16 TEI:12;             //指定发送信标的站点TEI
    u16 BeaconType:1;       //信标类型
    u16 RfBeaconType:3;         //保留
}NOT_CENTER_BEACON_FILED, *P_NOT_CENTER_BEACON_FILED;

//CSMA时隙信息字段 
#pragma pack(1)
typedef struct
{
    u32 CSMASlotLen:24;     //CSMA时隙长度,1MS
    u32 CSMAPhase:2;        //CSMA时隙相线
    u32 Reserve1:6;         //保留
}CSMA_SLOT_FILED, *P_CSMA_SLOT_FILED;

//绑定CSMA时隙信息字段 
#pragma pack(1)
typedef struct
{
    u32 TieCSMASlotLen:24;      //绑定CSMA时隙长度,1MS
    u32 TieCSMAPhase:2;         //绑定CSMA时隙相线
    u32 Reserve1:6;             //保留
}TIECSMA_SLOT_FILED, *P_TIECSMA_SLOT_FILED;

//管理消息帧格式
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    MSDU_HEAD_LONG_CSG LMsduHead;
    u8  MMVersion;     //管理消息版本
    u16 MMType;        //管理消息类型
    u8  Reserve1[3];   //保留
#else
    u16 MMType;     //管理消息类型
    u16 Reserve1;   //保留
#endif
}MME_FRAME, MM_HEAD, *P_MM_HEAD;

typedef struct
{
    MM_HEAD head;
    unsigned char body[1];
}MM_FRAME, *P_MM_FRAME;

//管理消息类型
typedef enum
{
#if defined(HPLC_CSG)
    MME_ASSOC_REQ = 0x0030,                     //关联请求
    MME_ASSOC_CNF = 0x0031,                     //关联确认
    MME_CHANGE_PROXY_REQ = 0x0032,              //代理变更请求
    MME_ASSOC_IND = 0x0034,                     //关联指示
    MME_CHANGE_PROXY_CNF = 0x0037,              //代理变更确认
    MME_ASSOC_GATHER_IND = 0x003A,              //关联汇总指示
    MME_CHANGE_PROXYI_BITMAP_CNF = 0x003B,      //代理变更确认位图版
    MME_LEAVE_IND_IMMEDIATE = 0x0049,           //离线指示
    MME_HEARTBEAT_CHECK = 0x0051,               //心跳检测
    MME_DISCOVER_NODE_LIST = 0x0055,            //发现列表
    MME_LEAVE_IND = 0x005D,                     //延迟离线指示
    MME_SUCCESSRATE_REPORT = 0x005E,            //通信成功率上报
	MME_NETWORK_CONFLICT_REPORT = 0x005F,       //网络冲突上报
    MME_ZEROCROSS_NTB_COLLECT_IND = 0x0062,     //过零NTB采集指示
    MME_ZEROCROSS_NTB_REPORT = 0x0063,          //过零NTB上报
	MME_NET_CHECK = 0x0064,                     //网络诊断
    MME_RF_CHANNEL_CONFICT_REPORT = 0x0070,     //无线信道冲突上报
    #if 0
    MME_ROUTE_REQUEST = 0x0050,                 //路由请求
    MME_ROUTE_REPLY = 0x0051,                   //路由回复
    MME_ROUTE_ERROR = 0x0052,                   //路由错误
    MME_ROUTE_ACK = 0x0053,                     //路由应答
    MME_LINK_CONFIRM_REQUEST = 0x0054,          //链路确认请求
    MME_LINK_CONFIRM_RESPONSE = 0x0055          //链路确认回应
    #endif
#else
    MME_ASSOC_REQ = 0x0000,                     //关联请求
    MME_ASSOC_CNF = 0x0001,                     //关联确认
    MME_ASSOC_GATHER_IND = 0x0002,              //关联汇总指示
    MME_CHANGE_PROXY_REQ = 0x0003,              //代理变更请求
    MME_CHANGE_PROXY_CNF = 0x0004,              //代理变更确认
    MME_CHANGE_PROXYI_BITMAP_CNF = 0x0005,      //代理变更确认位图版
    MME_LEAVE_IND = 0x0006,                     //离线指示
    MME_HEARTBEAT_CHECK = 0x0007,               //心跳检测
    MME_DISCOVER_NODE_LIST = 0x0008,            //发现列表
    MME_SUCCESSRATE_REPORT = 0x0009,            //通信成功率上报
    MME_NETWORK_CONFLICT_REPORT = 0x000A,       //网络冲突上报
    MME_ZEROCROSS_NTB_COLLECT_IND = 0x000B,     //过零NTB采集指示
    MME_ZEROCROSS_NTB_REPORT = 0x000C,          //过零NTB上报
    MME_NET_CHECK = 0x004F,                       //网络诊断
    MME_ROUTE_REQUEST = 0x0050,                 //路由请求
    MME_ROUTE_REPLY = 0x0051,                   //路由回复
    MME_ROUTE_ERROR = 0x0052,                   //路由错误
    MME_ROUTE_ACK = 0x0053,                     //路由应答
    MME_LINK_CONFIRM_REQUEST = 0x0054,          //链路确认请求
    MME_LINK_CONFIRM_RESPONSE = 0x0055,          //链路确认回应
	MME_RF_CHANNEL_CONFICT_REPORT = 0x0080,     //无线信道冲突上报
#endif

}EM_MMTYPE;

typedef enum
{
    EM_ASSOC_OK,                            //表示关联请求成功
    EM_ASSOC_NOT_IN_WHITE_LIST,             //表示该站点不在白名单中
    EM_ASSOC_IN_BLACK_LIST,                 //表示该站点在黑名单中
    EM_ASSOC_STA_OVER_UPPER_LIMIT,          //表示加入的站点个数超过上限
    EM_ASSOC_NO_WHITE_LIST,                 //表示没有设置白名单列表
    EM_ASSOC_PCO_OVER_UPPER_LIMIT,          //表示代理站点个数超过上限
    EM_ASSOC_STA_CHILD_OVER_UPPER_LIMIT,    //表示子站点个数超过上限
    EM_ASSOC_RESERVE,                       //保留
    EM_ASSOC_MAC_ADDR_DUPLICATE,            //表示重复的 MAC 地址
    EM_ASSOC_OVER_MAX_LEVEL,                //表示超过拓扑层级
    EM_ASSOC_OK_AGAIN,                      //表示站点再次关联请求入网成功
    EM_ASSOC_PCO_ERR,                       //表示新的站点试图以自己的子站点为代理来入网
    EM_ASSOC_RING_CIRCUIT,                  //表示组网拓扑中存在环路
    EM_ASSOC_CCO_ERR                        //表示 CCO 端未知原因出错

}ASSOC_RESULT;

//站点TEI域
#pragma pack(1)
typedef struct
{
    u16 TEI:12;
    u16 Link:1;
    u16 Reserve:3;         //保留
}TEI_ZONE;

//版本日期
#pragma pack(1)
typedef struct
{
    u16 Year:7;
    u16 Month:4;
    u16 Day:5;
}VERSION_DATE;

//站点版本信息
#pragma pack(1)
typedef struct
{
    u8 BootReason;      //系统启动原因
    u8 BootVersion;     //boot版本号
    u16 SoftVersion;    //软件版本
    VERSION_DATE VersionTime;    //版本时间
    u16 FactoryCode;    //厂商代码
    u16 ChipCode;       //芯片代码
}STATION_VERSION;

//关联请求报文 
#pragma pack(1)

typedef struct
{
    u8 check_sum[8];
	u8 dev_seq[5];
	u8 chip_code[2];
	u8 factory_code[2];
	u8 dev_class;       //设备类别（如 0x01 表示窄带载波通信单元， 0x02 表示宽带载波通信单元
	u8 fix_code4[3];
	u8 fix_code3;
	u8 fix_code2;
	u8 fix_code1;
}STA_CHIP_ID_INFO, *P_STA_CHIP_ID_INFO;


typedef struct
{
#if defined(HPLC_CSG)
    u8  MacAddr[6];         //站点MAC地址
    TEI_ZONE CandidataPCO[5]; //候选代理
    u32 PhaseLine:24;         //相线
    u32 DeviceType:8;         //设备类型
    u8  Reserve0;           //保留
    u8  Reserve1;             //保留1
    u8  MacAddrType;         //MAC地址类型
    u8  MoudleType:2;           //模块类型
    u8  Link:5;            //LINK
    u8  Reserve3:1;            //保留
    u32 STARandom;          //站点关联随机数
    u8  FactoryMSG[18];         //厂家自定义信息
    STATION_VERSION  STAVersion;     //站点版本信息
    u16 HardReboot;         //硬复位次数
    u16 SoftReboot;         //软复位次数
    u8  PCOType;             //代理类型
    u8  NetSeq;              //组网序列号
    u8  Reserve5:1;
    u8  MngMsgVersion:4;    //管理消息版本
    u8  Reserve6:3;
    u8  FrequenceSupport:2; //支持频段标识
    u8  Reserve7:6;
    u32 P2PSeq;             //端到端序列号
#else
    u8  MacAddr[6];         //站点MAC地址
    TEI_ZONE CandidataPCO[5]; //候选代理
    u8  PhaseLine:6;         //相线
    u8  Reserve6:2;         //保留
    u8  DeviceType;         //设备类型
    u8  MacAddrType;        //MAC地址类型
    u8  MoudleType:2;           //模块类型
    u8  Reserve7:6;           //保留
    u32 STARandom;          //站点关联随机数
    u8  FactoryMSG[18];         //厂家自定义信息
    STATION_VERSION  STAVersion;     //站点版本信息
    u16 HardReboot;         //硬复位次数
    u16 SoftReboot;         //软复位次数
    u8  PCOType;             //代理类型
    u8  Reserve8[3];        //保留
    u32 P2PSeq;             //端到端序列号
    STA_CHIP_ID_INFO  ChipID;         //芯片ID信息
#endif
}ASSOC_REQ_PACKET, *P_ASSOC_REQ_PACKET;

//关联确认报文 
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u8  STAMacAddr[6];      //站点MAC地址
    //u8 CCOMacAddr[6];       //CCOMAC地址
    u8  Result;             //结果
    u8  Level;              //站点层级
    u16 STATei:12;             //站点TEI
    u16 :4;
    u16 PCOTei:12;             //代理TEI
    u16 :4;
    u8  TotalPacket;        //总分包数
    u8  PacketSeq;          //分包序号
    u8  LastPacketFlag;     //最后一个分包标识
    u8  Link:1;             //链路类型
    u8  Band:2;             //载波频段
    u8  Reserve1:5;           //保留
    u32 StationRandom;      //站点随机数
    u32 ReAssocTime;        //重新关联时间
    u32 P2PSeq;             //端到端序列号
    u32 RouteSeq;           //路径序号
    u8  NetSeq;             //组网序列号
    u8  MngMsgVersion:4;    //管理消息版本
    u8  detechFlag:1;         //探测频段标识符
    u8  Reserve2:3;         //保留
    u16 Reserve3;           //保留
    u8  RouteData[1];       //路由表信息
#else
    u8  STAMacAddr[6];      //站点MAC地址
    u8  CCOMacAddr[6];      //CCOMAC地址
    u8  Result;             //结果
    u8  Level;              //站点层级
    u16 STATei:12;          //站点TEI
    u16 Link:1;             //链路类型
    u16 Band:2;             //载波频段
    u16 Reserve1:1;         //保留
    u16 PCOTei:12;          //代理TEI
    u16 Reserve2:4;         //保留
    u8  TotalPacket;        //总分包数
    u8  PacketSeq;          //分包序号
    u32 StationRandom;      //站点随机数
    u32 ReAssocTime;        //重新关联时间
    u32 P2PSeq;             //端到端序列号
    u32 RouteSeq;           //路径序号
    u8  Reserve3;           //保留
    u8  AppFlag[2];         //保留
    u8  Reserve4;           //保留
    u8  RouteData[1];       //路由表信息
#endif
}ASSOC_CNF_PACKET;


#if defined(HPLC_CSG)

//关联指示
typedef struct
{
    u8  Result;             //结果
    u8  Level;              //站点层级
    u8  STAMacAddr[6];      //站点MAC地址
    u8  CCOMacAddr[6];      //CCOMAC地址
    u16 STATei:12;             //站点TEI
    u16 :4;
    u16 PCOTei:12;             //代理TEI
    u16 :4;
    u8  LinkType:1;         //链路类型
    u8  Band:2;         //载波频段
    u8 :5;
    u8  Reserve1[2];
    u8  PacketSeq;          //分包序号
    u8  TotalPacket;        //总分包数
    u8  LastPacketFlag;     //最后一个分包标识
    u32 StationRandom;      //站点随机数
    u8  Reserve2[17];
    u8  NetSeq;             //组网序列号
    u8  Reserve3[2];
    u32 ReAssocTime;        //重新关联时间
    u32 P2PSeq;             //端到端序列号
    u8  Reserve4[8];
    u8  RouteData[1];       //路由表信息
}ASSOC_IND_PACKET, *P_ASSOC_IND_PACKET;

#endif

//路由信息字段 
#pragma pack(1)
typedef struct
{
    u16 DirectSTANum;       //直连站点数
    u16 DirectPCONum;       //直连代理数
    u16 RouteTableSize;     //路由信息表大小
    u16 Reserve1;           //保留
    u8 RouteTable[1];      //路由表
}ROUTE_MSG_FILED;

//直连站点 
#pragma pack(1)
typedef struct
{
    u16 DirectSTATei:12;    //直连的STA的TEI
    u16 Reserve1:4;         //保留
}DIRECT_LINK_STA;

//直连PCO 
#pragma pack(1)
typedef struct
{
    u16 DirectPCOTei:12;    //直连代理PCO的TEI
    u16 Reserve1:4;         //保留
    u16 PCOSlaveNum;           //该PCO下的子节点
    DIRECT_LINK_STA  Slave[1];          //子节点表
}DIRECT_LINK_PCO;

//关联汇总指示报文 
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u8  Result;                  //结果
    u8  Level;                   //层级
    u8  CCOMacAddr[6];           //CCO MAC地址
    u16 PCOTei:12;              //代理TEI
    u16 :4;
    u8  NetSeq;                 //组网序列号
    u8  TotalSTA;               //汇总站点数
    u8  Band:2;                 //保留
    u8  :6;
    u8  Reserve[15];               //保留
    u8  StationMSG[1];        //站点信息
#else
    u8  Result;                  //结果
    u8  Level;                   //层级
    u8  CCOMacAddr[6];           //CCO MAC地址
    u16 PCOTei:12;              //代理TEI
    u16 Band:2;                 //载波频段
    u16 Reserve1:2;             //保留
    u8  Reserve2;               //保留
    u8  TotalSTA;               //汇总站点数
    u8  Reserve3;               //保留
    u8  AppFlag[2];              //配电网或者营销网
    u8  Reserve4;               //保留
    u8  StationMSG[1];        //站点信息
#endif

}ASSOC_GATHER_IND_PACKET, *P_ASSOC_GATHER_IND_PACKET;

//站点信息字段 
#pragma pack(1)
typedef struct
{
    u8 STAMacAddr[6];           //站点的MAC地址
    u16 TEI:12;                  //站点TEI
    u16 Reserve1:4;             //保留
}STA_MSG_FILED, *P_STA_MSG_FILED;

//代理变更请求 
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u16 TEI:12;                    //站点TEI
    u16 :4;
    TEI_ZONE NewPCOTei0;             //新代理TEI0
    TEI_ZONE NewPCOTei1;             //新代理TEI1
    TEI_ZONE NewPCOTei2;             //新代理TEI2
    TEI_ZONE NewPCOTei3;             //新代理TEI3
    TEI_ZONE NewPCOTei4;             //新代理TEI4
    u16 OldPCOTei;              //旧代理TEI
    u8  ProxyType;              //代理类型
    u8  Reason;                 //原因
    u32 Phase:24;               //站点相线
    u32 Link0:1;             //保留
    u32 Link1:1;             //保留
    u32 Link2:1;             //保留
    u32 Link3:1;             //保留
    u32 Link4:1;             //保留
    u32 Reserve1:3;             //保留
    u32 P2PSeq;                 //端到端序列号
    u8  NetSeq;                 //组网序列号
    u8  Reserve2[15];
#else
    u16 TEI:12;                 //站点TEI
    u16 :4;
    TEI_ZONE NewPCOTei0;          //新代理TEI0
    TEI_ZONE NewPCOTei1;          //新代理TEI1
    TEI_ZONE NewPCOTei2;          //新代理TEI2
    TEI_ZONE NewPCOTei3;          //新代理TEI3
    TEI_ZONE NewPCOTei4;          //新代理TEI4
    u16 OldPCOTei:12;           //旧代理TEI
    u16 Reserve7:4;             //保留
    u8  ProxyType;              //代理类型
    u8  Reason;                 //原因
    u32 P2PSeq;                 //端到端序列号
    u8  Phase:6;                //站点相线
    u8  Reserve8:2;             //保留
#endif
}CHANGE_PROXY_REQ_PACKET, *P_CHANGE_PROXY_REQ_PACKET;

//代理变更确认 
#pragma pack(1)
typedef struct
{
#ifdef HPLC_CSG
    u32 Result;                  //结果
    u8 TotalPacket;             //总分包数
    u8 PacketSeq;               //分包序号
    u16 TEI:12;                   //站点TEI

    u16 Reserve2:4;              //保留
    u16 PCOTei:12;              //代理站点TEI
    u16 Reserve3:4;              //保留
    u16 SlaveNum;               //子站点数
    u8 res;
    u8 Net_Seq;                 //组网序列号
    u8 Link:1;
    u8 :7;
    u8 res1;
    u32 P2PSeq;                 //端到端序号
    u32 RouteSeq;               //路径序号
    u8  res3[8];               //保留
    u8  Slave[0];             //子站点条目
#else
    u8 Result;                  //结果
    u8 TotalPacket;             //总分包数
    u8 PacketSeq;               //分包序号
    u8 Reserve1;                //保留
    u16 TEI:12;                   //站点TEI
    u16 Link:1;                   //链路类型
    u16 Band:2;                   //载波频段
    u16 PCOTei;              //代理站点TEI
    u32 P2PSeq;                 //端到端序号
    u32 RouteSeq;               //路径序号
    u16 SlaveNum;               //子站点数
    u16 Reserve4;               //保留
    u8  Slave[0];             //子站点条目
#endif
}CHANGE_PROXY_CNF_PACKET;

//子站点条目 
#pragma pack(1)
typedef struct
{
    u16 TEI:12;                 //站点TEI
    u16 Reserve:4;              //保留
}STA_ITEM;

#define SET_STA_BITMAP(bitmap, tei)      ((u8*)bitmap)[(tei)>>3] |= (1<<((tei)&0x07))
#define IS_STA_BITMAP_SET(bitmap, tei)  (((u8*)bitmap)[(tei)>>3] & (1<<((tei)&0x07)))
#define CLEAR_STA_BITMAP(bitmap, tei)    ((u8*)bitmap)[(tei)>>3] &= ~(1<<((tei)&0x07))
#define GET_STA_BITMAP_SIZE(max_tei)    (((max_tei)>>3)+1)

//站点位图
typedef struct
{
    unsigned short count;       //站点个数
    unsigned char  size;        //位图实际占用字节数
    unsigned char  bitmap[TEI_CNT/8+1];     //0字节的1比特值为1，表示的TEI为1；1字节的0比特值为1，表示的TEI为8
}STA_BITMAP, *P_STA_BITMAP;

//代理站点位图及其子站点
typedef struct
{
    unsigned short count;               //代理站点数
//    unsigned char  bitmap[TEI_CNT/8+1]; //代理站点位图
    unsigned short substa_bit_cnt;      //后面字段substa所占用的比特数目
    unsigned char  substa[2*(TEI_CNT-1-1-1)*TEI_BIT_CNT/8+1];     //最大占用空间的情况是只有1个代理的情况。减TEI0,TEI1,TEI代理。代理111bit子站点数N 11bit子站点1 11bit子站点2...11bit子站点N 代理211bit子站点数M 11bit子站点1 11bit子站点2...11bit子站点M
}PCO_BITMAP_SUBSTA, *P_PCO_BITMAP_SUBSTA;

//代理变更请求确认（位图版）
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u32 Result;                  //结果
    u16 TEI:12;                   //站点TEI
    u16 Reserve:4;              //保留
    u16 PCOTei;                  //代理站点TEI
    u8  NetSeq;                  //组网序列号
    u8  Bitmap[130];             //子站点位图
    u8  Link:1;                 //扩展的位图大小
    u8  :7;
    u32 P2PSeq;                  //端到端序号
    u32 RouteSeq;                //路径序号
#else
    u8  Result;                  //结果
    u8  Reserve1;                  //保留
    u16 BitmapSize;              //位图大小
    u16 TEI:12;                   //站点TEI
    u16 Link:1;                  //链路类型
    u16 Reserve2:3;              //保留
    u16 PCOTei:12;              //代理站点TEI
    u16 Reserve3:4;              //保留
    u32 P2PSeq;                 //端到端序号
    u32 RouteSeq;               //路径序号
    u32 Reserve4;               //保留
    u8  Bitmap[1];              //位图
#endif
}CHANGE_PROXY_CNF_BITMAP_PACKET;

#if defined(HPLC_CSG)
#pragma pack(1)
typedef struct
{
    u16 Tei;
    u16 Reason;         //原因
    u8  MacAddr[6];         //MAC地址
    u16 PcoTei;
    u8  Reserve1[8];   //保留
}LEAVE_IND_IMMEDIATE_PACKET, *P_LEAVE_IND_IMMEDIATE_PACKET;
#endif

//离线指示报文
#pragma pack(1)
typedef struct
{
    u16 Reason;         //原因
    u16 STA_Num;        //站点总数
    u16 DelayTime;      //延时时间
    u8  Reserve1[10];   //保留
    u8  Mac[1];         //MAC地址
}LEAVE_IND_PACKET, *P_LEAVE_IND_PACKET;

//心跳检测报文 
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u16 SrcTei;             //原始源TEI
    u16 MaxTei;             //发现站点数最大的TEI
    u32 STANum;             //最大的发现站点数
    u8  Bitmap[130];        //位图
#else
    u16 SrcTei:12;          //原始源TEI
    u16 Reserve1:4;         //保留
    u16 MaxTei:12;          //发现站点数最大的TEI
    u16 Reserve2:4;         //保留
    u16 STANum;             //最大的发现站点数
    u16 BitmapSize;         //位图大小
    u8  Bitmap[1];        //位图
#endif
}HEARTBEAT_CHECK_PACKET, *P_HEARTBEAT_CHECK_PACKET;

//发现列表报文 
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u16 TEI;                //TEI
    u8  Role;               //角色
    u8  Level;              //层级
    u8  MacAddr[6];         //MAC地址
    u16 PCOTei;             //代理TEI
    u32 Reserve1:31;        //保留
    u32 StatComplete:1;     //通信成功率计算完成标志, “ 1” 表示完成计算， “ 0” 表示未完成计算。 
    u32 PCOSuccess;         //代理站点通信成功率
    u32 PCODownSuccess;     //代理站点下行通信成功率
    u16 STANum;             //站点总数
    u16 SendPacketNum;      //发送发现列表报文个数
    u16 UpRouteNum;         //上行路由条目总数
    u8  RouteItemLen;       //路由表项条目长度
    u16 Reserve2;           //保留
    u16 RoutePeriodLeft;    //路由周期到期剩余时间
    u8  Phase;              //相线
    u8  MinSuccess;         //最小通信成功率
	u8 Reserve3[5];         //保留
    u8  Data[1];            //包含上行路由条目、发现站点列表位图、接收发现列表信息
#else
    u32 TEI:12;             //TEI
    u32 PCOTei:12;          //代理TEI
    u32 Role:4;             //角色
    u32 Level:4;            //层级
    u8  MacAddr[6];         //MAC地址
    u8  CCOMacAddr[6];      //CCO MAC地址
    u8  Phase:6;            //相线
    u8  Reserve1:2;         //保留
    s8  PCO_SNR;            //代理站点通信质量
    u8  PCOSuccess;         //代理站点通信成功率
    u8  PCODownSuccess;     //代理站点下行通信成功率
    u16 STANum;             //站点总数
    u8  SendPacketNum;      //发送发现列表报文个数
    u8  UpRouteNum;         //上行路由条目总数
    u16 RoutePeriodLeft;    //路由周期到期剩余时间
    u16 BitmapSize;         //位图大小
    u8  MinSuccess;         //最小通信成功率
    u8  Reserve2[3];        //保留
    u8  Data[1];            //包含上行路由条目、发现站点列表位图、接收发现列表信息
#endif
}DISCOVER_NODE_LIST_PACKET, *P_DISCOVER_NODE_LIST_PACKET;

typedef struct
{
    u8 MacAddr[6];          //MAC地址
    u8 Seq;
	u8 data[0];
}DISCOVER_RF_NODE_LIST_PACKET;
typedef struct{
	u8 ItemType:7;
	u8 LenType:1;
	u8 Len;
	u8 data[0];
}NODE_LIST_ITEM;
typedef struct{
	u8 CcoMac[6];
	u16 PcoTei:12;
	u16 Role:4;
	u8 Level:4;
	u8 RfHop:4;
	u8 PcoUpRate;
	u8 PcoDownRate;
	u8 MinRate;
	u8 SendPeriod;
	u8 OldPeriod;
}LIST_STATION;
typedef struct{
	u16 NextRout:12;
	u16 RoutType:4;
}LIST_ROUT_ITEM;
typedef struct{
	u8 NeighType:4;
	u8 :4;
	u8 data[0];
}LIST_NEIGH;

typedef struct{
	u16 startTei:12;
	u16 :4;
	u8 size;
	u8 data[0];
}LIST_BIT_HEAD;

typedef struct{
	u16 TEI;
	u16 TotalSTA;
	struct{
		u16 TEI;
		u8 UpRate;
		u8 DownRate;
	}Sta[0];
}SUCCESS_RATE_UP_PACKET,P_SUCCESS_RATE_UP_PACKET;

typedef struct{
	u8  CCO_MAC[6];
	u8  Cnt;
	u8  Channel[0];
}HRF_CONFLICT_REPORT,P_HRF_CONFLICT_REPORT;

//上行路由信息字段 
#pragma pack(1)
typedef struct
{
    u16 NextTei:12;         //下一条TEI
    u16 RouteType:4;        //路由类型
}UP_ROUTE_FILED;

//通信成功率报文 
#pragma pack(1)
typedef struct
{
    u16 TEI:12;             //TEI
    u16 Reserve1:4;         //保留
    u16 STANum;             //STA总数
    u8 SuccessRate[1];   //通信成功率信息
}SUCCESS_REPORT_PACKET;

//通信成功率信息 
#pragma pack(1)
typedef struct
{
    u16 TEI:12;             //TEI
    u16 Reserve1:4;          //保留
    u8 DownSuccessRate;     //下行通信成功率
    u8 UpSuccessRate;       //上行通信成功率
}SUCCESS_RATE_FILED;

//网络冲突上报报文格式

//MMeNetworkConflictReport
#pragma pack(1)
typedef struct
{
    u8 NeighborMacAddr[MAC_ADDR_LEN];   //邻居网络的CCO MAC地址
    u8 NeighborNetworkNum;              //邻居网络个数
    u8 NetIDs[2];                       //邻居网络条目
}NETWORK_CONFLICT_REPORT, *P_NETWORK_CONFLICT_REPORT;


//过零NTB采集指示报文
#pragma pack(1)
typedef struct
{
    u16 TEI;             //站点
    u8 CollectSta;          //采集站点(单站,全网)
    u8 CollectPeriod;       //采集周期
    u8 CollectNum;          //采集数量
}ZERO_CROSS_NTB_COLLECT, *P_ZERO_CROSS_NTB_COLLECT;

//采集的站点
enum
{
    INDICATE_STA_COLLECT,   //指定单站点采集
    ALL_STA_COLLECT,        //指定全网站点采集
};

//过零NTB采集周期
enum
{
    HALF_POWER_PERIOD,      //1/2电力线周期
    ONE_POWER_PERIOD,       //一个电力线周期
};

//过零NTB告知报文
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u16 TEI;                //TEI
    u8  TotalNum;           //告知总数量
    u8  Reserve;            //保留
    u32 BaseNTB;            //基准NTB
    u8  Data[1];            //告知报文数量内容
#else
    u16 TEI:12;             //TEI
    u16 Reserve1:4;
    u8  TotalNum;           //告知总数量
    u8  Phase1Num;          //相线1告知数量
    u8  Phase2Num;          //相线2告知数量
    u8  Phase3Num;          //相线3告知数量
    u32 BaseNTB;            //基准NTB
    u8  Data[1];            //告知报文数量内容
#endif
}ZERO_CROSS_NTB_REPORT, *P_ZERO_CROSS_NTB_REPORT;
#ifdef HPLC_CSG
typedef struct
{
    u16 TEI:12;             //TEI
    u16 collectType:2;      //采集方式
    u16 res:2;
    u8  seq;                //采集序号
    u8  TotalNum;           //告知总数量
    u32 BaseNTB;            //基准NTB
    u8  res1;
    u8  Phase1Num;          //相线1告知数量
    u8  Phase2Num;          //相线2告知数量
    u8  Phase3Num;          //相线3告知数量
    u8  Data[1];            //告知报文数量内容

}APP_ZERO_CROSS_NTB_REPORT, *P_APP_ZERO_CROSS_NTB_REPORT;
#endif
#pragma pack(2)
typedef struct
{
    u16  DiffNTB:12;
    u16  NextNTB_L:4;
    u8   NextNTB_H:8;
}ZERO_CROSS_DATA;


#define MAC_BROADCAST_TEI   0xFFF

extern const u8 BroadCastMacAddr[6];


typedef struct
{
    #ifdef  FORWARD_FRAME
    u16             SrcTei;
    #endif
    u16             DstTei;             //目的TEI
    MAC_SEND_TYPE   SendType;           //发送类型
    u8              SendLimit;          //发送次数限值
    u8              HopCount;           //跳数
    MSDU_TYPE       MsduType;           //MSDU类型  8
    MAC_BROADCAST_DIRECTION Broadcast;  //广播方向
    u8              NetSeq;                         //组网序号
    MAC_FLAG_e      mac_flag;           //南网不用
    u8              SrcMac[MAC_ADDR_LEN];                      //原始MAC地址
    u8              DstMac[MAC_ADDR_LEN];                      //目标MAC地址

#if defined(HPLC_CSG)
    MAC_HEAD_TYPE   HeadType;
    u32             Vlan;
    u8              NID;
#endif

}MAC_PARAM, *P_MAC_PARAM;


_CRC_32_ GetCrc32(u8 *data,u16 len);
u32      GetCrcInit32(u32 crc_init, u8 *data, u32 len);

_CRC_24_ GetCrc24(u8 *data,u16 len);

P_MAC_FRAME MakeMacFrame(const P_MAC_PARAM pMacParam, u8* pMsdu, u16 msduLen);
#ifdef HPLC_CSG
u8* MacCreateMsdu(MAC_HEAD_TYPE macHeadType, u8* pAppData, u16 appDataLen, u16* pMsduLen);
#endif
u16 GetMacFrameLen(const P_MAC_FRAME pMacFrame);
u8* MacFrameGetMsduPtr(const P_MAC_FRAME pMacFrame,u16 *Len);
MSDU_TYPE MacFrameGetMsduType(const P_MAC_FRAME pMacFrame);
#define VCS_TIMER           BSP_TIMER_0
#define HRF_VCS_TIMER       BSP_TIMER_1
#define CSMD_SEND_TMR       BSP_TIMER_2
#define BCSMD_SEND_TMR      BSP_TIMER_3
#define HRF_CSMD_SEND_TMR   BSP_TIMER_4
#define HRF_BCSMD_SEND_TMR  BSP_TIMER_5
#define HPLC_SEND_ACK       BSP_TIMER_6
#define HRF_SEND_ACK        BSP_TIMER_7
#define WAIT_BEACON_NTBTIMER             0
#define WAIT_DETECT_START_NTBTIMER       1
#define WAIT_DETECT_STOP_NTBTIMER        2
#define WAIT_3_NTBTIMER                  3
#define WAIT_HRF_BEACON_NTBTIMER         4
#pragma pack(4)
#endif

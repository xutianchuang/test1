#ifndef _HPLC_MAC_DEF_H_
#define _HPLC_MAC_DEF_H_
#include "bps_timer.h"
#pragma pack(1)

// MSDU长帧头
typedef struct
{
    u8 DstMac[6]; // 原始目的地址
    u8 SrcMac[6]; //
    u32 Vlan;     // VLAN标签
    u16 MsduType; // MSDU类型
} MSDU_HEAD_LONG;

// MAC帧公用帧头
typedef struct
{
    u16 HeadType : 1;  // 帧头类型
    u16 Version : 2;   // 版本（为1）
    u16 Reserve1 : 13; // 保留
    u16 MsduLen;       // MSDU长度
    u32 DstTei : 12;   // 原始目的TEI
    u32 SrcTei : 12;   // 原始源TEI
    u32 NID : 4;       // 短网络标识
    u32 RebootNum : 4; // 重启次数
    u8 RouteCnt : 4;   // 路由跳数
    u8 Broadcast : 4;  // 广播方向
    u8 SendType : 3;   // 发送类型
    u8 SendLimit : 5;  // 发送次数限值
    u16 MsduSeq;       // MSDU序列号
} MAC_HEAD;

typedef struct
{
    u8 HeadType : 1; // 帧头类型
    u8 Version : 2;  // 版本
    u8 rsv : 5;      // 保留
    u8 MsduType;     // MSDU类型
    u16 MsduLen;     // MSDU长度
} MAC_HEAD_ONE_HOP;

// MAC长帧头
typedef struct
{
    MAC_HEAD MacHead; // mac帧头
    u8 DstMac[6];     // 目的MAC地址
    u8 Reserve2[4];   // 保留
    u8 Reserve3[10];  // 保留
} MAC_HEAD_LONG;

// MSDU短帧头
typedef struct
{
    u8 Vlan;     // Vlan
    u8 MsduType; // MSDU类型
} MSDU_HEAD_SHORT;

// MAC短帧头
typedef struct
{
    MAC_HEAD MacHead; // mac帧头
} MAC_HEAD_SHORT;

// 单跳帧MSDU 类型
typedef enum
{
    MSDU_TYPE_RSV = 0,    // 保留
    MSDU_TYPE_APP,        // 应用层报文
    MSDU_TYPE_DISCOVER,   // 无线发现列表消息
    MSDU_TYPE_IPV4 = 128, // IPV4 报文
} ONE_HOP_MSDU_TYPE;

// 发送类型
enum
{
    SENDTYPE_SINGLE,        // 单播
    SENDTYPE_BROADCAST,     // 全网广播
    SENDTYPE_LOCAL,         // 本地广播
    SENDTYPE_BROADCAST_ACK, // 全网广播(需回应)
    SENDTYPE_LOCAL_ACK,     // 本地广播(需回应)
};

// 广播方向
enum
{
    BROADCAST_DRT_UNKNOW, // 保留
    BROADCAST_DRT_DOWN,   // 下行广播
    BROADCAST_DRT_UP,     // 上行广播
};

#define MSDU_APP_FRAME    0x01
#define MSDU_MANAGER_VLAN 0x8100
#define MSDU_MANAGER_TYPE 0x88E1

// MSDU类型
enum
{
    MSDU_NET_MANAGER, // 网络管理消息
};

// CRC32校验码
typedef struct
{
    u8 byte0;
    u8 byte1;
    u8 byte2;
    u8 byte3;
} _CRC_32_;

typedef struct
{
    u8 byte0;
    u8 byte1;
    u8 byte2;
} _CRC_24_;

// MPDU帧控制
typedef struct
{
    u8 Type : 3;       // 定界符类型
    u8 ConnectInd : 1; // 接入指示(为1)
    u8 NID : 4;        // 网络ID
    u8 Zone0[12];      // 可变区域(68位)
    _CRC_24_ CRC24;    // 校验帧控制
} MPDU_CTL;

// 定界符类型
typedef enum
{
    MPDU_TYPE_BEACON,  // 信标帧
    MPDU_TYPE_SOF,     // SOF帧
    MPDU_TYPE_CONFIRM, // 选择确认帧
    MPDU_TYPE_CONCERT, // 网间协调帧
} EN_MPDU_TYPE;

// 信标帧可变区域
typedef struct
{
    u32 NTB;             // 信标时间戳
    u32 BeaconPeriodCnt; // 信标周期计数
    u16 SrcTei : 12;     // 源TEI
    u16 CopyModel : 4;   // 分集拷贝模式
    u16 SymbolNum : 9;   // 符号数
    u16 Reserve : 1;     // 保留
    u16 Phase : 2;       // 相线
    u16 Version : 4;     // 恒为1
} BEACON_ZONE;

typedef struct
{
    u32 NTB;             // 信标时间戳
    u32 BeaconPeriodCnt; // 信标周期计数
    u16 SrcTei : 12;     // 源TEI
    u16 MCS : 4;         // MCS
    u16 PBSize : 4;      // 载荷PB大小
    u16 rsv : 12;        // 保留
} BEACON_HRF_ZONE;

// 相线
enum
{
    PHASE_UNKNOW, // 未知
    PHASE_A,      // A相
    PHASE_B,      // B相
    PHASE_C,      // C相
};

// SOF帧可变区域
typedef struct
{
    u32 SrcTei : 12;       // 源TEI
    u32 DstTei : 12;       // 目标TEI
    u32 LinkFlag : 8;      // 链路标识符
    u16 Reserve1;          // 保留
    u8 PhyBlock : 4;       // 物理块个数
    u8 CopyType : 4;       // 分集拷贝模式
    u32 FrameLen : 12;     // 帧长
    u32 Reserve2 : 9;      // 保留
    u32 BroadcastFlag : 1; // 过滤标志位
    u32 ReSendFlag : 1;    // 重传标志
    u32 SymbolNum : 9;     // 符号数
    u8 CopyExType : 4;     // 分集拷贝扩展模式
    u8 Version : 4;        // 恒为1
} SOF_ZONE;

typedef struct
{
    u32 SrcTei : 12;      // 源TEI
    u32 DstTei : 12;      // 目标TEI
    u32 LinkFlag : 8;     // 链路标识符
    u16 FrameLen : 12;    // 帧长
    u16 PbSize : 4;       // 帧载荷大小
    u8 MCS : 4;           // 分集拷贝模式
    u8 BroadcastFlag : 1; // 过滤标志位
    u8 ReSendFlag : 1;    // 重传标志
    u8 reserved : 2;      // 保留
    u8 Reserve1[4];       // 保留
} SOF_HRF_ZONE;

#define SEARCH_IDENT 0x35
// 选择确认的可变区域
typedef struct
{
    union
    {
        u8 array[11];
        struct
        {
            u8 ReceiveResult : 4; // 接收结果
            u8 ReceiveState : 4;  // 接收状态
            u16 DstTei : 12;      // 目标TEI
            u16 BlockNum : 4;     // 接收物理块个数
        };
        struct
        {
            u8 dmac[6];
            u16 stei;
            u8 ident; // 标识flag //国网如果此位为SEARCH_IDENT 代表需要切换频率
            u8 freq;  // 收到搜索帧需要切换的频段
            u8 seq;
        } search;
        struct
        {
            u8 dmac[6];
            u8 channel;
            u8 option;
        } hrf_channel; // hrf切频扩展
        struct
        {
            u32 timestamp;
            u16 stei;
            u16 mac_xor;
            u16 res;
            u8 seq;
        } sync;
    };
    u8 ExtType : 4; // 扩展帧类型
    u8 Version : 4; // 标准版本号，填1
} CONFIRM_ZONE;

typedef struct
{
    u8 ReceiveResult : 4; // 接收结果
    u8 rsv1 : 4;          // 保留
    u16 DstTei : 12;      // 目标TEI
    u16 rsv2 : 4;         // 保留
    u8 rsv3[8];           // 保留
    u8 ExtType : 4;       // 扩展帧类型
    u8 Version : 4;       // 标准版本号，填1
} CONFIRM_HRF_ZONE;

// 物理块头格式
typedef struct
{
    u16 Seq;     // 序列号
    u16 Reserve; // 保留
} PHY_BLOCK_HEAD;

// 信标帧载荷
typedef struct
{
    u8 BeaconType : 3; // 信标类型
    u8 NetFlag : 1;    // 组网标志(0:未完成,1:完成)
    u8 Retrench : 1;   // 精简信标标志
    u8 NetPrior : 1;   // 多网络优选开关
    u8 RelateFlag : 1; // 关联标志位(0:不允许站点发送关联请求,1:)
    u8 Reserve2 : 1;   // 保留
    u8 NetSeq;         // 组网序列号
    u32 NID : 4;       // 短网络标识
    u32 HrfOption : 2; // 本网络无线 option
    u32 rsv : 2;       // 保留
    u32 HrfIndex : 8;  // 无线网络编号
    u32 Reserve3 : 16; // 保留
    u8 Data[1];        // 载荷
} BEACON_MPDU;

// 精简信标载荷
typedef struct
{
    u8 BeaconType : 3;     // 信标类型
    u8 NetFlag : 1;        // 组网标志(0:未完成,1:完成)
    u8 Retrench : 1;       // 精简信标标志
    u8 rsv1 : 1;           // 保留
    u8 RelateFlag : 1;     // 关联标志位(0:不允许站点发送关联请求,1:)
    u8 BeaconUsedFlag : 1; // 信标使用标志位
    u8 NetSeq;             // 组网序列号
    u8 CCOMacAddr[6];      // CCO MAC 地址
    u32 BeaconPeriodCnt;   // 信标周期计数
    u8 Data[0];            // 载荷+校验
} BEACON_RETRENCH_MPDU;

// 信标类型
enum
{
    BEACON_DISCOVER, // 发现信标
    BEACON_PROXY,    // 代理信标
    BEACON_CENTER,   // 中央信标
    BEACON_DETECT,   // 探测信标
};

// 组网标志位
enum
{
    NET_COMPLETE,   // 组网完成
    NET_UNCOMPLETE, // 组网未完成
};

// 信标使用标志
enum
{
    NO_ALLOW_USE_BEACON, // 不允许使用信标进行信道评估
    ALLOW_USE_BEACON,    // 允许使用信标进行信道评估
};

// 信标管理信息格式
typedef struct
{
    u8 BeaconNum; // 信标条目数
    u8 Data[1];
} BEACON_MANAGER_MSG;

// 信标条目头
enum
{
    BEACON_ITEM_STATION = 0x01,     // 站点能力条目
    BEACON_ITEM_TIME = 0x02,        // 时隙分配条目
    BEACON_ITEM_ROUTE = 0x06,       // 路由参数条目
    BEACON_ITEM_FRE = 0x07,         // 频段变更条目
    BEACON_ITEM_MONITOR = 0x09,     // 用于抄控器
    BEACON_ITEM_DETECT = 0x0a,      // 频段探测条目
    BEACON_ITEM_UTC = 0x0b,         // 万年历条目
    BEACON_ITEM_RF_ROUT = 0x0c,     // 无线路由参数条目
    BEACON_ITEM_RF_CHANNEL = 0x0d,  // 无线信道变更条目
    BEACON_ITEM_RF_RETRENCH = 0x0e, // 精简信标 站点&时隙条目
};

// 站点能力条目
typedef struct
{
    u8 Level : 6;      // 层级数
    u8 PhaseLine : 2;  // 站点所属相线
    u16 TEI : 12;      // 站点TEI
    u8 Role : 4;       // 角色
    u8 BeaconUsedFlag; // 信标信道评估标志位
    u8 MacAddr[6];     // 信标发送站点MAC地址
    u16 PCOTei : 12;   // 代理站点TEI
    u16 RfHop : 4;     // 经过的HRF跳数
    u32 MinSuccess;    // 站点到CCO整个路径的最低成功率
    u32 Reserve2;      // 保留
} BEACON_STATION;

// 路由参数条目
typedef struct
{
    u16 RoutePeriod;  // 路由周期
    u16 Reserve1;     // 保留
    u16 LeftTime;     // 距离下次路由评估的时间
    u8 Reserve2[20];  // 保留
    u8 CCOMacAddr[6]; // CCO mac地址
} BEACON_ROUTE;

// 精简信标站点能力
typedef struct
{
    u32 TEI : 12;    // 站点TEI
    u32 PCOTei : 12; // 代理站点TEI
    u32 Role : 4;    // 角色
    u32 Level : 4;   // 层级数
    u8 MacAddr[6];   // 信标发送站点MAC地址
    u8 RfHop : 4;    // 经过的HRF跳数
    u8 HplcBand : 2; // 载波频段  在新的规约上删除了这个字段 此处发送时填写 这个字段接受时不再处理这个字段
    u8 Reserve : 2;  // 保留
    u32 CsmaStart;   // CSMA开始NTB
    u16 CsmaLen;     // CSMA时隙长度
} BEACON_RETRENCH_STATION;

// 频段通知条目
typedef struct
{
    u8 TargetFre; // 目标频段
    u32 LeftTime; // 距离实施频段切换剩余时间
} BEACON_FREQUENCE;

// 无线路由参数条目
typedef struct
{
    u8 RfListPeriod; // 无线发现列表周期
    u8 OldCnt;       // 老化周期个数
} BEACON_RF_PARA;

// 无线信道变更条目
typedef struct
{
    u8 Channel;    // 无线信道
    u8 option : 2; // 目标信道 option
    u8 rsv : 6;    // 保留
    u32 LeftTime;  // 距离实施频段切换剩余时间
} BEACON_RF_CHANNEL;

// 抄控器条目
typedef struct
{
    u8 data[1]; // 目前抄控器条目未知
} BEACON_COPY;

// 时隙分配条目
typedef struct
{
    u8 NotCenterSlotNum; // 非中央信标时隙总数
    u8 CenterSlotNum;    // 中央信标时隙总数
    u8 CSMAPhase;        // CSMA支持的相线个数
    u8 PCOBeaconNum;     // 代理信标时隙总数
    u16 SlotLen;         // 每个信标时隙占用的时隙长度，100us
    u8 CSMASlotLen;      // CSMA时隙分片大小,100us
    u8 TieCSMAPhaseNum;  // 绑定CSMA时隙支持的相线个数
    u8 TieCSMALid;       // 绑定CSMA时隙支持的业务报文
    u16 TDMASlotLen;     // TDMA时隙长度
    u8 TDMALid;          // TDMA报文
    u32 NTB;             // 信标周期开始时刻NTB值
    u32 BeaconPeriod;    // 信标周期时间长度,100us
    u32 RfSlot : 10;     // hrf信标时隙长度
    u32 Reserve2 : 22;   // 保留
    u8 Data[1];          // 包含非中央信标信息、CSMA时隙信息、绑定CSMA时隙信息
} BEACON_SLOT;

// 探测信标条目
typedef struct
{
    u8 Freq; // 探测频段
    u8 Lid;  // 绑定LID
} BEACON_DETECT_LIST;

// 万年历条目
typedef struct
{
    u32 frome_2000_sec;
    u32 NTB;
} BEACON_UTC, *P_BEACON_UTC;

// 非中央信标信息字段
typedef struct
{
    u16 TEI : 12;         // 指定发送信标的站点TEI
    u16 BeaconType : 1;   // 信标类型
    u16 RfBeaconType : 3; // 无线信标标志
} NOT_CENTER_BEACON_FILED;

// CSMA时隙信息字段
typedef struct
{
    u32 CSMASlotLen : 24; // CSMA时隙长度,100us
    u32 CSMAPhase : 8;    // CSMA时隙相线
} CSMA_SLOT_FILED;

// 绑定CSMA时隙信息字段
typedef struct
{
    u32 TieCSMASlotLen : 24; // 绑定CSMA时隙长度,100us
    u32 TieCSMAPhase : 8;    // 绑定CSMA时隙相线
} TIECSMA_SLOT_FILED;

typedef struct
{
    u8 Version;     // 版本
    u16 MMType;     // 类型
    u8 Reserve1[3]; // 保留
} MME_FRAME;

// 管理消息类型
enum
{
    MME_ASSOC_REQ = 0x0030,                 // 关联请求
    MME_ASSOC_CNF = 0x0031,                 // 关联确认
    MME_CHANGE_PROXY_REQ = 0x0032,          // 代理变更请求
    MME_ASSOC_IND = 0x0034,                 // 关联指示
    MME_CHANGE_PROXY_CNF = 0x0037,          // 代理变更确认
    MME_ASSOC_GATHER_IND = 0x003A,          // 关联汇总
    MME_CHANGE_PROXYI_BITMAP_CNF = 0x003B,  // 代理变更确认位图版
    MME_LEAVE_IND = 0x0049,                 // 离线指示
    MME_HEARTBEAT_CHECK = 0x0051,           // 心跳检测
    MME_DISCOVER_NODE_LIST = 0x0055,        // 发现列表
    MME_DELAY_LEAVE_IND = 0x005D,           // 延时离线指示
    MME_SUCCESSRATE_REPORT = 0x005E,        // 通信成功率上报
    MME_ZEROCROSS_NTB_COLLECT_IND = 0x0062, // 过零NTB采集指示
    MME_ZEROCROSS_NTB_REPORT = 0x0063,      // 过零NTB上报
    MME_RF_CHANNEL_CONFICT_REPORT = 0x0070, // 无线信道冲突上报

	MME_CONFIRM_ISP_PROJ_KEY_REQUEST = 0x1000,		//配置运营商及项目key
	MME_CONFIRM_ISP_PROJ_KEY_RESPONSE = 0x1001,		//获取运营商及项目key
};

// 站点TEI域
typedef struct
{
    u16 TEI : 12;
    u16 Link : 1;    // 保留
    u16 Reserve : 3; // 保留
} TEI_ZONE;

// 版本日期
typedef struct
{
    u16 Year : 7;
    u16 Month : 4;
    u16 Day : 5;
} VERSION_DATE;

// 站点版本信息
typedef struct
{
    u8 BootReason;            // 系统启动原因
    u8 BootVersion;           // boot版本号
    u16 SoftVersion;          // 软件版本
    VERSION_DATE VersionTime; // 版本时间
    u16 FactoryCode;          // 厂商代码
    u16 ChipCode;             // 芯片代码
} STATION_VERSION;

#define SUPPORT_FREQUENCE_IDX 1

// 关联请求报文
typedef struct
{
    u8 MacAddr[6];              // 站点MAC地址
    TEI_ZONE CandidataPCO[5];   // 候选代理
    u8 Phase1;                  // 相线1
    u8 Phase2;                  // 相线2
    u8 Phase3;                  // 相线3
    u8 DeviceType;              // 设备类型
    u8 rsv1[2];                 // 保留
    u8 MacAddrType;             // MAC地址类型
    u8 IsHrfOrHplc : 2;         // 模块类型
	u8 Link:5;
	u8 Reserve0 : 1;            // 保留
    u32 STARandom;              // 站点关联随机数
    u8 FactoryMSG[18];          // 厂家自定义信息
    STATION_VERSION STAVersion; // 站点版本信息
    u16 HardReboot;             // 硬复位次数
    u16 SoftReboot;             // 软复位次数
    u8 PCOType;                 // 代理类型
    u8 NetSeq;                  // 组网序列号
    u8 Reserve3 : 1;            // 保留
    u8 Version : 4;             // 版本为1
    u8 Reserve4 : 3;            // 保留
    u8 FreType : 2;             // 支持频段信息
    u8 Reserve5 : 6;            // 保留
    u32 P2PSeq;                 // 端到端序列号
} ASSOC_REQ_PACKET;

// 关联确认报文
typedef struct
{
    u8 STAMacAddr[6];      // 站点MAC地址
    u8 Result;             // 结果
    u8 Level;              // 站点层级
    u16 STATei : 12;       // 站点TEI
    u16 rsv : 4;           // 保留
    u16 PCOTei : 12;       // 站点TEI
    u16 rsv1 : 4;          // 保留
    u8 TotalPacket;        // 总分包数
    u8 PacketSeq;          // 分包序号
    u8 EndPacketFlg;       // 最后一个分包标识
	u8 LinkType : 1;      // 链路类型
    u8 HplcBand : 2;      // 载波频段
	u8 Reserve3 : 5;      // 保留
    u32 StationRandom;     // 站点随机数
    u32 ReAssocTime;       // 重新关联时间
    u32 P2PSeq;            // 端到端序列号
    u32 RouteSeq;          // 路径序号
    u8 NetSeq;             // 组网序号
    u8 Version : 4;        // 版本
    u8 BandDetectFlag : 1; // 探测频段标识符
    u8 Reserve4 : 3;       // 保留
    u8 Reserve5[2];        // 保留
    u8 RouteData[1];       // 路由表信息
} ASSOC_CNF_PACKET;

#define ASSOC_CNF_PACKET_LEN 36

// 关联指示
typedef struct
{
    u8 Result;         // 结果
    u8 Level;          // 站点层级
    u8 STAMacAddr[6];  // 站点MAC地址
    u8 CCOMacAddr[6];  // CCO MAC地址
    u16 STATei : 12;   // 站点TEI
    u16 rsv : 4;       // 保留
    u16 PCOTei : 12;   // 代理TEI
    u16 rsv1 : 4;      // 保留
	u8 LinkType : 1;  // 链路类型
    u8 HplcBand : 2;  // 载波频段
	u8  : 5;
	u8 Reserve3[2];    // 保留
    u8 PacketSeq;      // 分包序号
    u8 TotalPacket;    // 总分包数
    u8 EndPacketFlg;   // 最后一个分包标识
    u32 StationRandom; // 站点随机数
    u8 Reserve4[17];   // 保留
    u8 NetSeq;         // 组网序号
    u8 Reserve5[2];    // 保留
    u32 ReAssocTime;   // 重新关联时间
    u32 P2PSeq;        // 端到端序列号
    u8 Reserve6[8];    // 保留
    u8 RouteData[1];   // 路由表信息
} ASSOC_IND_PACKET;

#define ASSOC_IND_PACKET_LEN 64

// 路由信息字段
typedef struct
{
    u16 DirectSTANum;   // 直连站点数
    u16 DirectPCONum;   // 直连代理数
    u16 RouteTableSize; // 路由信息表大小
    u16 Reserve1;       // 保留
    u8 RouteTable[1];   // 路由表
} ROUTE_MSG_FILED;

// 直连站点
typedef struct
{
    u16 DirectSTATei : 12; // 直连的STA的TEI
    u16 Link : 1;          // 链路类型
    u16 Reserve1 : 3;      // 保留
} DIRECT_LINK_STA;

// 直连PCO
typedef struct
{
    u16 DirectPCOTei : 12;    // 直连代理PCO的TEI
    u16 Link : 1;             // 链路类型
    u16 Reserve1 : 3;         // 保留
    u16 PCOSlaveNum;          // 该PCO下的子节点
    DIRECT_LINK_STA Slave[1]; // 子节点表
} DIRECT_LINK_PCO;

// 关联汇总指示报文
typedef struct
{
    u8 Result;        // 结果
    u8 Level;         // 层级
    u8 CCOMacAddr[6]; // CCO MAC地址
    u16 PCOTei : 12;  // 代理TEI
    u16 rsv : 4;      // 保留
    u8 NetSeq;        // 组网序列号
    u8 TotalSTA;      // 汇总站点数
	u8 HplcBand : 2; // 保留
	u8 :6;
	u8 Reserve2[15];  // 保留
    u8 StationMSG[1]; // 站点信息
} ASSOC_GATHER_IND_PACKET;

// 站点信息字段
typedef struct
{
    u8 STAMacAddr[6]; // 站点的MAC地址
    u16 TEI : 12;     // 站点TEI
    u16 Reserve1 : 4; // 保留
} STA_MSG_FILED;

// 代理变更请求
typedef struct
{
    u16 TEI : 12;             // 站点TEI
    u16 Reserve1 : 4;         // 保留
    TEI_ZONE CandidataPCO[5]; // 候选代理
    u16 OldPCOTei : 12;       // 旧代理TEI
    u16 Reserve2 : 4;         // 保留
    u8 ProxyType;             // 代理类型
    u8 Reason;                // 原因
    u8 Phase1;                // 站点相线1
    u8 Phase2;                // 站点相线2
    u8 Phase3;                // 站点相线3
	u8 LinkType : 5;         // 链路类型
	u8 Reserve3:3;              // 保留
    u32 P2PSeq;               // 端到端序列号
    u8 NetSeq;                // 组网序列号
    u8 Reserve4[15];          // 保留
} CHANGE_PROXY_REQ_PACKET;

// 代理变更确认
typedef struct
{
    u32 Result;       // 结果
    u8 TotalPacket;   // 总分包数
    u8 PacketSeq;     // 分包序号
    u16 TEI : 12;     // 站点TEI
    u16 Reserve2 : 4; // 保留
    u16 PCOTei : 12;  // 代理站点TEI
    u16 Reserve3 : 4; // 保留
    u16 SlaveNum;     // 子站点数
    u8 Reserve4;      // 保留
    u8 NetSeq;        // 组网序列号
	u8 LinkType : 1; // 链路类型
	u8 :7;
	u8 Reserve5;   // 保留
    u32 P2PSeq;       // 端到端序号
    u32 RouteSeq;     // 路径序号
    u8 Reserve6[8];   // 保留
    u8 Slave[1];      // 子站点条目
} CHANGE_PROXY_CNF_PACKET;

// 子站点条目
typedef struct
{
    u16 TEI : 12;    // 站点TEI
    u16 Reserve : 4; // 保留
} STA_ITEM;

#define CHANGE_PROXY_CNF_BITMAP_SIZE 130

// 代理变更请求确认（位图版）
typedef struct
{
    u32 Result;                              // 结果
    u16 TEI : 12;                            // 站点TEI
    
    u16 Reserve2 : 4;                        // 保留
    u16 PCOTei : 12;                         // 代理站点TEI
    u16 Reserve3 : 4;                        // 保留
    u8 NetSeq;                               // 组网序列号
    u8 Bitmap[CHANGE_PROXY_CNF_BITMAP_SIZE]; // 位图
	u8 LinkType : 1;                        // 链路类型
	u8 rsv:7;                                  // 保留
    u32 P2PSeq;                              // 端到端序号
    u32 RouteSeq;                            // 路径序号
} CHANGE_PROXY_CNF_BITMAP_PACKET;

// 离线指示报文
typedef struct
{
    u16 TEI : 12;     // 站点TEI
    u16 Reserve1 : 4; // 保留
    u16 Reason;       // 原因
    u8 MacAddr[6];    // MAC地址
    u16 PCOTei : 12;  // 代理站点TEI
    u16 Reserve2 : 4; // 保留
    u8 Reserve3;      // 保留
} LEAVE_IND_PACKET;

// 延时离线指示报文
typedef struct
{
    u16 Reason;      // 原因
    u16 STA_Num;     // 站点总数
    u16 DelayTime;   // 延时时间
    u8 Reserve1[10]; // 保留
    u8 Mac[1];       // MAC地址
} DELAY_LEAVE_IND_PACKET;

#define HEARTBEAT_BITMAP_SIZE 130

// 心跳检测报文
typedef struct
{
    u16 SrcTei : 12;                  // 原始源TEI
    u16 Reserve1 : 4;                 // 保留
    u16 MaxTei : 12;                  // 发现站点数最大的TEI
    u16 Reserve2 : 4;                 // 保留
    u32 STANum;                       // 最大的发现站点数
    u8 Bitmap[HEARTBEAT_BITMAP_SIZE]; // 位图
} HEARTBEAT_CHECK_PACKET;

typedef struct
{
    u8 CCOMacAddr[6]; // CCO MAC地址
    u8 Num;
    u8 Info[0];
} RF_CHANNEL_CONFLICT_PACKET;

#define DISCOVER_BITMAP_SIZE 128

// 发现列表报文
typedef struct
{
    u16 TEI : 12;        // 站点TEI
    u16 Reserve1 : 4;    // 保留
    u8 Role;             // 角色
    u8 Level;            // 层级
    u8 MacAddr[6];       // MAC地址
    u16 PCOTei : 12;     // 站点TEI
    u16 Reserve2 : 4;    // 保留
    u32 Reserve3 : 31;   // 保留
    u32 FinishRate : 1;  // 成功率完成标志
    u32 PCOSuccess;      // 代理站点通信成功率
    u32 PCODownSuccess;  // 代理站点下行通信成功率
    u16 STANum;          // 站点总数(前面的130字节的站点总数
    u16 SendPacketNum;   // 发送发现列表报文个数
    u16 UpRouteNum;      // 上行路由条目总数
    u8 RouteItemLen;     // 路由表项长度(固定8)
    u16 Reserve4;        // 保留
    u16 RoutePeriodLeft; // 路由周期到期剩余时间
    u8 Phase3 : 2;       // 相线3
    u8 Phase2 : 2;       // 相线2
    u8 Phase1 : 2;       // 相线1
    u8 Reserve5 : 2;     // 保留
    u8 MinSuccess;       // 最小通信成功率
    u8 rsv[5];           // 保留
    u8 Data[1];          // 包含上行路由条目、发现站点列表位图、接收发现列表信息
} DISCOVER_NODE_LIST_PACKET;

typedef struct
{
    u8 MacAddr[6]; // MAC地址
    u8 Seq;
    u8 data[0];
} DISCOVER_RF_NODE_LIST_PACKET;

typedef struct
{
    u8 ItemType : 7;
    u8 LenType : 1;
    u8 Len;
    u8 data[0];
} NODE_LIST_ITEM;

typedef struct
{
    u8 CcoMac[6];
    u16 PcoTei : 12;
    u16 Role : 4;
    u8 Level : 4;
    u8 RfHop : 4;
    u8 PcoUpRate;
    u8 PcoDownRate;
    u8 MinRate;
    u8 SendPeriod;
    u8 OldPeriod;
} LIST_STATION;

typedef struct
{
    u16 NextRout : 12;
    u16 RoutType : 4;
} LIST_ROUT_ITEM;

typedef struct
{
    u8 NeighType : 4;
    u8 : 4;
    u8 data[0];
} LIST_NEIGH;

typedef struct
{
    u16 startTei : 12;
    u16 : 4;
    u8 size;
    u8 data[0];
} LIST_BIT_HEAD;

#define DISCOVER_NODE_LIST_PACKET_LEN 42

// 上行路由信息字段
typedef struct
{
    u16 NextTei : 12; // 下一条TEI
    u16 Reserve1 : 4; // 保留
    u8 RouteType;     // 路由类型
} UP_ROUTE_FILED;

#define UP_ROUT_FILED_LEN 3

// 通信成功率报文
typedef struct
{
    u16 TEI : 12;      // TEI
    u16 Reserve1 : 4;  // 保留
    u16 STANum;        // STA总数
    u8 SuccessRate[1]; // 通信成功率信息
} SUCCESS_REPORT_PACKET;

// 通信成功率信息
typedef struct
{
    u16 TEI : 12;       // TEI
    u16 Reserve1 : 4;   // 保留
    u8 DownSuccessRate; // 下行通信成功率
    u8 UpSuccessRate;   // 上行通信成功率
} SUCCESS_RATE_FILED;

// 过零NTB采集指示报文
typedef struct
{
    u16 TEI : 12;     // 站点
    u16 Reserve1 : 4; // 保留
    u8 CollectSta;    // 采集站点(单站,全网)
    u8 CollectPeriod; // 采集周期
    u8 CollectNum;    // 采集数量
} ZERO_CROSS_NTB_COLLECT;

// 采集的站点
enum
{
    INDICATE_STA_COLLECT, // 指定单站点采集
    ALL_STA_COLLECT,      // 指定全网站点采集
};

// 过零NTB采集周期
enum
{
    HALF_POWER_PERIOD, // 1/2电力线周期
    ONE_POWER_PERIOD,  // 一个电力线周期
};

// 过零NTB告知报文
typedef struct
{
    u16 TEI : 12;     // TEI
    u16 Reserve1 : 4; // 保留
    u8 TotalNum;      // 告知总数量
    u8 Reserve2;      // 保留
    u32 BaseNTB;      // 基准NTB
    u8 Data[1];       // 告知报文数量内容
} ZERO_CROSS_NTB_REPORT;

typedef struct
{
    u16 DiffNTB : 12;
    u16 NextNTB_L : 4;
    u8 NextNTB_H : 8;
} ZERO_CROSS_DATA;

//分集拷贝模式
typedef enum
{
    DIV_COPY_0,//推荐
    DIV_COPY_1,
    DIV_COPY_2,
    DIV_COPY_3,
    DIV_COPY_4,
    DIV_COPY_5,
    DIV_COPY_6,
    DIV_COPY_7,
    DIV_COPY_8,
    DIV_COPY_9,
    DIV_COPY_10,
    DIV_COPY_NUM,
}DIV_COPY_MODEL;

extern const u16 CopyModelBlockSize[DIV_COPY_NUM];

//分集拷贝扩展模式
typedef enum
{
    DIV_COPY_EX_1 = 1,
    DIV_COPY_EX_2,
    DIV_COPY_EX_3,
    DIV_COPY_EX_4,
    DIV_COPY_EX_5,
    DIV_COPY_EX_6,
    DIV_COPY_EX_10 = 10,
    DIV_COPY_EX_11,
    DIV_COPY_EX_12,
    DIV_COPY_EX_13,
    DIV_COPY_EX_14,
    
    DIV_COPY_EX_NUM,
}DIV_COPY_EX_MODEL;
extern const u16 CopyExModelBlockSize[DIV_COPY_EX_NUM];

#pragma pack(4)

#define MAC_BROADCAST_TEI   0xFFF

//全网广播地址
extern const u8 BroadCastMacAddr[6];

//本地广播地址
extern const u8 LocalBroadCastMacAddr[6];

//非法地址
extern const u8 InvaildMacAddr[6];

_CRC_32_ GetCrc32(u8 *data,u16 len);

_CRC_24_ GetCrc24(u8 *data,u16 len);

#define VCS_TIMER           BSP_TIMER_0
#define HRF_VCS_TIMER       BSP_TIMER_1
#define CSMD_SEND_TMR       BSP_TIMER_2
#define HRF_CSMD_SEND_TMR   BSP_TIMER_3
#define BCSMD_SEND_TMR      BSP_TIMER_4
#define HRF_BCSMD_SEND_TMR  BSP_TIMER_5
#define CHANGE_BAND         BSP_TIMER_6
#define HRF_SEND_ACK        BSP_TIMER_7
#define TOPO_TOPO_TMR       BSP_TIMER_8  //拓扑识别锛孖I閲囨病鏈変娇鐢紝鐢ㄤ簬delay


#define IR_TIMER        BSP_TIMER_9
#define LED_TIMER       BSP_TIMER_10
#define USART_TIMER       BSP_TIMER_11


#endif

#ifndef _HPLC_MPDU_H_
#define _HPLC_MPDU_H_

//分集拷贝模式
typedef enum
{
#if defined(HPLC_CSG)
    DIV_COPY_0,
    DIV_COPY_1,
    //DIV_COPY_2,
    DIV_COPY_3 = 3,
    DIV_COPY_4,
    DIV_COPY_5,
    DIV_COPY_6,
    DIV_COPY_7,
    DIV_COPY_8,
    DIV_COPY_9,
    DIV_COPY_10,
    DIV_COPY_NUM = 13,      //使用扩展拷贝模式
#else
    DIV_COPY_0,
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
    DIV_COPY_11,
    DIV_COPY_12,
    DIV_COPY_13,
    DIV_COPY_14,
    DIV_COPY_NUM,       //使用扩展拷贝模式
#endif

}DIV_COPY_MODEL;

//分集拷贝扩展模式
typedef enum
{
    DIV_COPY_EX_INVALID,

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

typedef enum{
    FreamPathNull,
    FreamPathBplc=0x1,
    FreamPathHrf=0x2,
    FreamPathBoth=0x3,
}FramePath;//发送的类型
//物理块头格式
typedef struct
{
    u16 Seq;     // 序列号
    u16 Reserve; // 保留
} PHY_BLOCK_HEAD;
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u16 Seq;            //序列号
    u16 Reserve;        //保留
#else
    u8 Seq:6;           //序列号
    u8 BeginFlag:1;   //起始标志
    u8 EndFlag:1;     //结束标志
#endif
}SOF_PHY_BLOCK_HEAD, *P_SOF_PHY_BLOCK_HEAD;

//MPDU帧控制 
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u8 Type:3;      //定界符类型
    u8 NetType:1;   //接入指示
    u8 NID:4;       //网络标识
    u8 Zone0[12];   //可变区域(92位)
    //u8 MpduVersion:4;   //标准版本号
    _CRC_24_ CRC24;        //校验帧控制
#else
    u32 Type:3;      //定界符类型
    u32 NetType:5;   //网络类型
    u32 NID:24;      //网络标识
    u8 Zone0[9];        //可变区域(68位)
    //u8 MpduVersion:4;   //标准版本号
    _CRC_24_ CRC24;        //校验帧控制
#endif
}MPDU_CTL, *P_MPDU_CTL;


//信标帧可变区域 
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u32 NTB;                //信标时间戳
    u32 BeaconPeriodCouter; //信标周期计数
    u16 SrcTei:12;          //源TEI
    u16 CopyModel:4;        //分集拷贝模式
    u16 SymbolNum:9;        //符号数
    u16 Reserve:1;          //保留
    u16 Phase:2;            //相线
    u16 Version:4;          //标准版本号，填1
#else
    u32 NTB;                //信标时间戳
    u16 SrcTei:12;          //源TEI
    u16 CopyModel:4;        //分集拷贝模式
    u16 SymbolNum:9;        //符号数
    u16 Phase:2;            //相线
    u16 Reserve:1;          //保留
    u16 Version:4;          //标准版本号，填0
#endif
}BEACON_ZONE, *P_BEACON_ZONE;
typedef struct
{
#ifndef  HPLC_CSG
    u32 NTB;                //信标时间戳
    u16 SrcTei:12;          //源TEI
    u16 MCS:4;              //MCS
    u16 PBSize:4;          //载荷PB大小
#else
    u32 NTB;                //信标时间戳
    u32 BeaconPeriodCnt; 
	u16 SrcTei:12;          //源TEI
	u16 MCS:4;              //MCS
	u16 PBSize:4;          //载荷PB大小
    u16 :4;
#endif
}BEACON_HRF_ZONE;
//SOF帧可变区域 
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u32 SrcTei:12;          //源TEI
    u32 DstTei:12;          //目标TEI
    u32 LinkFlag:8;         //链路标识符，0-3报文优先级，4-254业务分类LID
    u16 Reserve1;           //保留
    u8  PhyBlock:4;         //物理块个数
    u8  CopyType:4;         //载波映射表索引
    u16 FrameLen:12;        //帧长, 单位10微秒
    u16 Reserve2:4;         //保留
    u16 Reserve3:5;         //保留
    u16 BroadcastFlag:1;    //TEI 过滤标志位，表示该SOF帧是否为广播报文, 本规约的TEI过滤标志位取值固定为1
    u16 ReSendFlag:1;       //重传标志
    u16 SymbolNum:9;        //符号数
    u8  CopyExType:4;       //扩展载波映射表索引
    u8  Version:4;          //标准版本号，填1
#else
    u32 SrcTei:12;          //源TEI
    u32 DstTei:12;          //目标TEI
    u32 LinkFlag:8;         //链路标识符，0-3报文优先级，4-254业务分类LID
    u16 FrameLen:12;        //帧长, 单位10微秒
    u16 PhyBlock:4;         //物理块个数
    u16 SymbolNum:9;        //符号数
    u16 BroadcastFlag:1;    //广播标志
    u16 ReSendFlag:1;       //重传标志
    u16 EncryptFlag:1;      //加密标志
    u16 CopyType:4;         //分集拷贝模式
    u8  CopyExType:4;       //分集拷贝扩展模式
    u8  Version:4;          //标准版本号，填0
#endif
}SOF_ZONE, *P_SOF_ZONE;
typedef struct
{
#ifndef HPLC_CSG
    u32 SrcTei:12;          //源TEI
    u32 DstTei:12;          //目标TEI
    u32 LinkFlag:8;         //链路标识符
    u16 FrameLen:12;        //帧长
    u16 PbSize:4;           //帧载荷大小
    u16 res:9;              //
    u16 BroadcastFlag:1;    //广播标志
    u16 ReSendFlag:1;       //重传标志
    u16 EncryptFlag:1;      //加密标志
    u16 MCS:4;              //分集拷贝模式
    u8  res2:4;       //分集拷贝扩展模式
#else
    u32 SrcTei:12;          //源TEI
    u32 DstTei:12;          //目标TEI
    u32 LinkFlag:8;         //链路标识符
	u16 FrameLen:12;        //帧长
    u16 PbSize:4;           //帧载荷大小
	u8 MCS:4;              //分集拷贝模式
	u8 BroadcastFlag:1;    //过滤标志位
    u8 ReSendFlag:1;       //重传标志
    u8 res:2;              //
	u32 Reserve2;           //保留
#endif
}SOF_HRF_ZONE;

//选择确认的可变区域 
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
//  u8  ReceiveResult:4;     //接收结果
//  u8  ReceiveState:4;      //接收状态
//  u16 DstTei:12;           //目标TEI
//  u16 BlockNum:4;          //接收物理块个数
    union
    {
        u8 array[11];
        struct {
            u8  ReceiveResult:4;     //接收结果
            u8  ReceiveState:4;      //接收状态
            u16 DstTei:12;           //目标TEI
            u16 BlockNum:4;          //接收物理块个数
        };
        struct
        {
            u8 dmac[6];
            u16 stei;
            u8  ident;//标识flag //国网如果此位为SEARCH_IDENT 代表需要切换频率
            u8  freq;//收到搜索帧需要切换的频段
            u8 seq;
        }search;
		struct
		{
			u8 dmac[6];
			u8 channel;
			u8 option; 
		}hrf_channel;//hrf切频扩展
        struct
        {
            u32 timestamp;
            u16 stei;
            u16 mac_xor;
            u16 res;
            u8 seq;
        }sync;
		struct 
        {
            u32 Timestamp;
            u32 SrcTei : 12;
            u32 DstTei : 12;
            u32 Phase : 2;
            u32 Rsv : 6;
        } ranging_req;  // 测距发起帧
        struct 
        {
            u32 Timestamp;
#if 0
            u32 DstTei : 12;
            u32 Sign : 1;
            u32 Duration : 11;
            u32 Cali : 8;
#else
            u32 SrcTei : 12;
            u32 DstTei : 12;
            u32 Duration : 20;
#endif
        } ranging_rsp; // 测距应答帧        
    };
    u8  ExtType:4;          //扩展帧类型
    u8  Version:4;           //标准版本号，填1
#else

    union
	{
		u8 array[8];
		struct
		{
			u8  ReceiveResult:4;     //接收结果
			u8  ReceiveState:4;      //接收状态
			u32 SrcTei:12;           //源TEI
			u32 DstTei:12;           //目标TEI
			u32 BlockNum:3;          //接收物理块个数
			u32 Reserve1:5;          //保留
			u8  ChannelQuality;      //信道质量
			u8  StationLoad;         //站点负载
			u8  Reserve2;            //保留
		};
		struct
		{
			u8 dmac[6];
			u16 stei:12;
			u16 freq:4; //收到搜索帧需要切换的频段
		}search;
		struct
		{
			u8 dmac[6];
			u8 channel;
			u8 option; 
		}hrf_channel;//hrf切频扩展
		struct
		{
			u32 timestamp;
			u16 stei;
            u16 mac_xor;
		}sync;
	};
    u8  ExtType:4;       //扩展帧类型
    u8  Version:4;          //标准版本号，填0
#endif

}CONFIRM_ZONE, *P_CONFIRM_ZONE;

//网间协调帧的可变区域 
#pragma pack(1)
typedef struct
{
#if defined(HPLC_CSG)
    u16 NID;                 //邻居网络
    u32 Channel:8;          //无线信道标号
	u32 Reserve1:10;         //保留
	u32 ContinueTime:14;     //持续时间,     单位:  40ms
    u8  Reserve2:1;          //保留
    u8  CompleteFlag:1;      //带宽结束标志位
	u8  Option:2;			 //无线信道标option
    u8  Reserve3:4;          //保留
    u16 TapeEndoffset;       //带宽结束偏移, 单位： 4ms
    u16 Tapeoffset;          //带宽开始偏移, 单位： 4ms
    u8  Reserve4:4;          //保留
    u8  Version:4;          //标准版本号，填1
#else
    u16 ContinueTime;       //持续时间,      单位：1毫秒
    u16 Tapeoffset;         //带宽开始偏移,  单位：1毫秒
    u32 NID:24;             //接收到的邻居网络号
    u32 Channel:8;          //无线信道标号
	u8  Option:2;           //无线option标号
	u8  Reserve2:2;         //保留
    u8  Version:4;          //标准版本号，填0
#endif

}CONCERT_ZONE, *P_CONCERT_ZONE;
























#pragma pack(4)
//需要传递的SOF帧内容
typedef struct
{
    u16 SrcTei;
    u16 DstTei;
    u8 Lid;
    u8 BroadcastFlag;           //1 广播报文,  0非广播报文
    //u8 NID[3];
    u32 NID;
    u8 ResendTimes;
    u8 link;
}SOF_PARAM;

typedef struct
{
    u32 NID;
    u16 DstTei;
    u8  Lid;
    u8  BroadcastFlag;           //1 广播报文,  0非广播报文，南网没用到该字段
    u8 doubleSend;           //单拨时是否使用主通道不同时启用另一通道
}SOF_PARAMS, *P_SOF_PARAMS;

//需要传递的信道参数
typedef struct
{
    uint32_t    Rssi;
    s8          Snr;
    u16         SofSrcTei;        //SOF 的源TEI, 或者信标帧的是TEI
    u32         sync_ntb;
    u32         BeaconNid;       //信标的NID 当前的参数仅针对于信标
    u8          Link;          //接收来源于哪个通道 0：HPLC 1：HRF
    u8          option;            //
	u8          channel;            //hplc 为band  hrf为index
    HPLC_PHASE  receive_phase;
}BPLC_PARAM, HPLC_PARAM, *P_HPLC_PARAM;


typedef void (*pfBeaconCallback)(const P_HPLC_PARAM pHplcParam, P_BEACON_ZONE pBeaconZone, P_BEACON_MPDU pBeaconPayload);
typedef void (*pfConcertCallback)(CPU_NTB_64 ntb_receive, u32 nid, P_CONCERT_ZONE pConcertFrame);

//处理MPDU帧

//MPDU层初始化
void MpduInit(void);    

PDU_BLOCK* MpduCreateConcert(u32 time_alloc_ms, u32 tapeoffset_ms, u32 lastoffset_ms,u32 nid_neigbor);
PDU_BLOCK* MpduCreateBeacon(PDU_BLOCK* mpdu,u32 beacon_period_counter, u8* data, u16 len, HPLC_PHASE phase);
PDU_BLOCK* MpduCreateSOFFrame(P_SOF_PARAMS pSOFParam, u8* p_mac_frame, u16 mac_frame_len);
PDU_BLOCK* MpduCreateCommTestFrame(P_SOF_PARAMS pSOFParam, u8* p_mac_frame, u16 mac_frame_len);

BOOL    MpduIsMpduCorrect(u8* pData, u16 nLen,u8 crc);

//从信标帧载荷中获取信标时隙
bool    MpduGetBeaconSlot(const P_BEACON_MPDU pBeaconPayload, P_BEACON_SLOT* ppBeaconSlot);

BOOL    MpduIsSOFBroadcast(u8* pPdu);
u16     MpduGetSOFDstTei(u8* pPdu);

//处理MPDU
void MpduHandleMPDU(PDU_BLOCK* pdu,u8 crc);

//处理测试模式12MPDU
void HPLC_HandleMpduToHRF(PDU_BLOCK* pdu,u8 crc);


//设置网络号过滤
void MpduSetNID(bool effect, u32 NID);



//获取NTB的差值
int64_t GetNTB_Offset(void);

void BeaconSetBeaconFrameCallback(pfBeaconCallback beaconCallback);
void BeaconSetConcertFrameCallback(pfConcertCallback concertCallback);
void HPLCSendHRFImmediate(u8* p0, u8 crc);
BOOL MpduGetPhyBlockSize(u8 copy_mode, u8 copy_mode_ex, u16* p_phyblk_size);
bool GetTMI(u16 len,DIV_COPY_MODEL *tmi,u8 *blockNum);
#pragma pack(4)
#endif

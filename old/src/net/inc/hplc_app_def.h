#ifndef _HPLC_APP_DEF_H_
#define _HPLC_APP_DEF_H_

//应用报文
#pragma pack(1)
typedef struct
{
    u8  Port;           //报文端口号(固定0x11)
    u16 PacketID;       //报文ID(固定0x1010)
    u8  Reserve;        //保留
    u8  Data[1];        //业务报文
}APP_PACKET;

//控制字
typedef struct
{
    u16 FrameType:4;    //帧类型
    u16 Reserve:8;      //保留
    u16 ExZone:1;       //业务扩展域
    u16 AckFlg:1;       //响应标志位
    u16 StartFlg:1;     //启动标志位
    u16 TranDir:1;      //传输方向
}APP_CTL;


//应用业务报文
typedef struct
{
    APP_CTL Ctrl;       //控制域
    u8 BusFlg;          //业务标识
    u8 Version;         //版本
    u16 AppSeq;         //帧序号
    u16 AppFrameLen;    //帧长
    u8  Data[1];        //帧内容
}APP_GENERAL_PACKET;

//帧类型
typedef enum
{
    APP_FRAME_CONFIRM,  //确定/否定帧,       0b'0000
    APP_FRAME_TRAN,     //数据转发帧,        0b'0001
    APP_FRAME_CMD,      //命令帧,          0b'0010
    APP_FRAME_REPORT,   //主动上报帧,        0b'0011
    APP_FRAME_READER,   //抄控器命令帧       0b'0100
    APP_FRAME_DEBUG = 0x0E, //厂家调试, 0b'1110
    APP_FRAME_FACTORY = 0x0F, //内部（威胜生产）
}APP_FRAME_TYPE;

//业务标识
typedef enum
{
    APP_BUS_CONFIRM = 0x00,    //确认
    APP_BUS_DENY = 0x01,       //否认
    APP_BUS_TRAN = 0x00,       //数据转发
    APP_BUS_TRAN_MODULE = 0x01,       //数据转发至模块
    APP_BUS_QUIRY_LIST = 0x00,       //查询搜表结果
    APP_BUS_LIST = 0x01,        //下发搜表列表
    APP_BUS_FILE_TRAN = 0x02,   //文件传输
    APP_BUS_EVENT_FLG = 0x03,   //允许/禁止事件上报
    APP_BUS_REBOOT = 0x04,      //重启
    APP_BUS_QUIRY = 0x05,       //查询节点信息
    APP_BUS_TRAN_LIST = 0x06,   //下发映射表
    APP_BUS_STA_STATUS = 0x7,   //查询从节点运行状态
    APP_BUS_STA_CHANNEL = 0x8,  //查询从节点信道信息
//	APP_BUS_STA_BROAD_TIME=0xa, //广播校时帧
    APP_BUS_TEST_FRAME = 0xF0,  //测试帧
    APP_BUS_EVENT_REPORT = 0x00,    //事件上报
    APP_BUS_POWER_EVENT_REPORT = 0x01,    //停电事件上报
    APP_BUS_ZONE_RELATION = 0x10,    //台区户变关系

    //威胜产测扩展业务标识
    APP_BUS_EXT_FACTORY_SET_MAC          = 0x04,  //设置从节点默认MAC地址
    APP_BUS_EXT_FACTORY_GET_MAC          = 0x05,  //查询从节点默认MAC地址
    APP_BUS_EXT_FACTORY_SET_FACTORY_CODE = 0x2D,  //设置从节点厂商代码和版本信息
    APP_BUS_EXT_FACTORY_GET_ZERO         = 0x3A,  //查询从节点过零信号信息
    APP_BUS_EXT_FACTORY_GET_VER_ALL      = 0x3B,  //查询从节点版本信息（包括内部版本）
    APP_BUS_EXT_FACTORY_GET_TXPOWER      = 0x3E,  //查询从节点发射功率
    APP_BUS_EXT_FACTORY_SET_TXPOWER      = 0x3F,  //查询从节点发射功率
    APP_BUS_EXT_FACTORY_GET_PLL	         = 0x50,  //查询从节点pll值
    APP_BUS_EXT_FACTORY_GET_IO	         = 0x51,  //查询从节点io

    //中慧产测扩展业务标识
    APP_BUS_EXT_FACTORY_GET_WAFER	     = 0xA0,  //查询从节点wafer ID
	APP_BUS_EXT_FACTORY_IR_TEST          = 0xA1,  //二采红外测试
    APP_BUS_EXT_FACTORY_GET_RSSI         = 0xA2,  //查询从节点接收RSSI

    APP_BUS_EXT_FACTORY_GET_ASSETS_CODE	 = 0x42,  //查询南网资产编码
    APP_BUS_EXT_FACTORY_SET_ASSETS_CODE  = 0x43,  //设置南网资产编码
    APP_BUS_EXT_FACTORY_GET_EXT_VER	 = 0x45,  //查询南网扩展版本信息（资产信息使用）
    APP_BUS_EXT_FACTORY_SET_EXT_VER      = 0x46,  //设置南网扩展版本信息（资产信息使用）
    APP_BUS_EXT_FACTORY_GET_HRFTXPOWER   = 0x47,  //设置hrf发送功率
    APP_BUS_EXT_FACTORY_SET_HRFTXPOWER   = 0x48,  //获取hrf发送功率
}APP_BUS_FLG;

//深化应用业务代码
typedef enum
{
	APP_SHENHUA_APP_CODE_TANSPARENT = 0x00, //透传645报文，目前没有使用
	APP_SHENHUA_APP_CODE_TIMING,            //精准校时
	APP_SHENHUA_APP_CODE_GRAPH,             //负荷曲线采集与存储
	APP_SHENHUA_APP_CODE_MAX = APP_SHENHUA_APP_CODE_GRAPH
}APP_SHENHUA_APP_CODE;
	
//负荷曲线采集与存储功能码
typedef enum
{
	APP_GRAPH_FUNC_CODE_CONFIG = 0x01, //配置采集间隔
	APP_GRAPH_FUNC_CODE_QUERY,         //查询采集间隔
	APP_GRAPH_FUNC_CODE_READ,          //抄读数据项
	APP_GRAPH_FUNC_CODE_READ_21PROTOCOL = APP_GRAPH_FUNC_CODE_QUERY //21标准抄读数据项
}APP_GRAPH_FUNC_CODE;

//电表信息
typedef struct
{
    u8 MeterMac[6];     //地址
    u16 Reserve;        //保留
}APP_METER_INFO;

//数据透传下行报文
typedef struct
{
    u8 SrcMac[6];           //源地址
    u8 DstMac[6];           //目的地址
    u8 Timeout;             //设备超时时间
    u8 Reserve;            //保留
    u16 DataLen;            //转发数据长度
    u8 Data[1];             //数据
}APP_READ_METER_DOWN;

//电能表精准校时数据透传下行
typedef struct
{
    u8 SrcMac[6];           //源地址
    u8 DstMac[6];           //目的地址
    u8 Reserve;             //保留    
    u8 App_Code;            //业务代码
    u16 DataLen;            //转发数据长度
    u8 Data[1];             //数据
}APP_READ_MOUDLE_TIMING_DOWN;

//深化应用数据透传下行
typedef struct
{
    u8 SrcMac[6];           //源地址
    u8 DstMac[6];           //目的地址
#ifdef PROTOCOL_NW_2020_OLD //南网电科院20年8月深化应用标准
	u8 Timeout;             //设备超时时间
#endif    
    u8 Reserve;             //保留
    u8 App_Code;            //深化应用新扩充业务代码
    u16 DataLen;            //转发数据长度
    u8 Data[1];             //数据
}APP_READ_MOUDLE_DOWN;

//下一代电表扩展
typedef struct
{
    u16 Port;            //端口号
    u8 Seq;              //序号
    u8 Reserve;          //保留
    u32 NTB;             //发送NTB
    u8 data[0];          //数据报文
}APP_BROADCST_TIME_EXT;

//数据透传上行报文
typedef struct
{
    u8 SrcMac[6];           //源地址
    u8 DstMac[6];           //目的地址
    u16 Reserve;             //保留
    u16 DataLen;            //转发数据长度
    u8 Data[1];             //数据
}APP_READ_METER_UP;

//深化应用数据透传上行
typedef struct
{
    u8 SrcMac[6];           //源地址
    u8 DstMac[6];           //目的地址
    u8 Reserve;             //保留
    u8 AppCode;             //业务代码
    u16 DataLen;            //转发数据长度
    u8 Data[1];             //数据
}APP_READ_MODULE_UP;


//搜表上行报文
typedef struct
{
    u8  MeterNum;           //电表数量
    u8  Reserve[3];         //保留
    u8  Data[1];            //电表信息
}APP_SCAN_METER_UP;
typedef struct {
    u8 type;
    u8 start:1;
    u8 :7;
    u32 rate;
    u32 seq:8;
    u32 res:24;
    u16 len;
    u8 data[0];
}APP_READER_UART_FRAME;
//节点重启报文
typedef struct
{
    u8 DelayTime;           //延时重启时间
    u8  Reserve[3];         //保留
}APP_NODE_REBOOT;

//升级报文
typedef struct
{
    u8 UpdateInfoID;        //升级传输信息ID
    u8 Reserve[3];          //保留
    u8 Data[1];             //文件传输信息
}APP_UPDATE_PACKET;

//文件类型
typedef enum
{
    APP_FILE_CLEAR,         //清除文件下行
    APP_FILE_STA = 0x02,           //模块文件
    APP_FILE_COLLECT,       //采集器文件
    APP_FILE_METER,         //电能表文件
}APP_FILE_PROPERTY;

//升级信息ID
typedef enum
{
    APP_UPDATE_FILE_INFO,     //下发文件信息
    APP_UPDATE_FILE_DATA,       //下发文件数据
    APP_UPDATE_STATE,      //升级接收包状态
    APP_UPDATE_FINISH,     //文件传输完成通知
    APP_UPDATE_BROADCAST,   //本地广播转发
    APP_UPDATE_STATE_READER=0xf6,
}APP_FILE_TRAN_ID;
//事件
typedef enum
{
    UPDATE_EVENT_INVAILD,
    UPDATE_EVENT_START,         //开始升级
    UPDATE_EVENT_FILE_DATA,     //文件传输
    UPDATE_EVENT_FILE_DATA_BROADCAST,  //文件传输(单播转本地广播)
    UPDATE_EVENT_EXECUTE,       //执行升级
}UPDATE_EVENT;
//文件下发信息
typedef struct
{
    u8  FilePro;            //文件性质
    u8  Reserve;            //保留
    u8  DstMac[6];             //目的地址
    u32 FileCRC;            //文件总校验
    u32 FileSize;           //文件大小
    u16 FileSegment;        //文件总段数
    u16 UpdateWin;          //升级时间窗
    u32 UpdateID;           //文件传输ID
}APP_FILE_INFO_DOWN;

//文件下发信息上行
typedef struct
{
    u32 UpdateID;           //文件传输ID
    u16 ResultCode;         //结果码
    u16 ErrorCode;          //错误码
}APP_FILE_INFO_UP;

//文件数据下行帧
typedef struct
{
    u16 FileSegID;        //文件段号
    u16 TotalFileSegment;   //文件总段号
    u32 UpdateID;           //文件传输ID
    u16 FileSegLen;         //文件段长度
    u8  Data[1];            //文件段内容
}APP_FILE_DATA_DOWN;

//文件数据上行帧
typedef struct
{
    u32 UpdateID;           //升级ID
    u32 ResultCode;         //结果码
}APP_FILE_DATA_UP;


//文件接收包状态
typedef struct
{
    u32 UpdateID;           //升级ID
    u16 BeginSegment;       //升级段号
    u16 SegmentNum;         //连续查询的段数
}APP_REC_STATE_DOWN;

//文件接收包状态上行帧
typedef struct
{
    u32 UpdateID;           //升级ID
    u16 BeginSegment;       //升级段号
    u8  State;              //文件传输状态
    u8  Reserve;            //保留
    u8  Data[1];            //数据
}APP_REC_STATE_UP;


//文件接收包状态
typedef struct
{
    u32 UpdateID;           //升级ID
    u16 BeginSegment;       //升级段号
    u16 SegmentNum;         //连续查询的段数
    u8 mac[6];
}APP_REC_STATE_READER_DOWN;

//文件接收包状态上行帧
typedef struct
{
    u32 UpdateID;           //升级ID
    u16 BeginSegment;       //升级段号
    u8  State;              //文件传输状态
    u8  Reserve;            //保留
    u8 mac[6];
    u8  Data[1];            //数据
}APP_REC_STATE_READER_UP;
//文件传输完成通知
typedef struct
{
    u32 UpdateID;           //升级ID
    u16 DelayTime;          //延时启动时间
}APP_FILE_FINISH_DOWN;

//文件传输完成通知上行帧
typedef struct
{
    u32 UpdateID;               //升级ID
    u16 ResultCode;          //结果码
}APP_FILE_FINISH_UP;

//从节点信息查询
typedef struct
{
    u8 Num;                 //元素数量
    u8 Data[1];             //元素列表
}APP_QUIRY_INFO;

//从节点运行状态查询
typedef struct
{
    u8 Num;                 //元素数量
    u8 Data[0];             //元素列表
}APP_QUIRY_STATUS;

//从节点信道信息
typedef struct
{
    u16 start;                 //开始序列号
    u8  num;                   //查询元素个数
}APP_QUIRY_CHANNEL;


//从节点信道信息
typedef struct
{
    u16 total;                 //上报
    u8  num;                   //查询元素个数
    u8  data[0];
}APP_QUIRY_CHANNEL_UP;

//从节点信道信息
typedef struct
{
    u8 mac[6];
    u16 tei;
    u16 pco_tei;
    u8 level;
    u8 up_rate;
    u8 down_rate;
    u8 min_rate;
    u8 snr;
    u8 attenuator;
}APP_CHANNEL_INFO;


//元素信息单元
typedef struct
{
    u8 UnitID;              //元素ID
    u8 Len;                 //元素数据长度
    u8 Data[1];             //元素数据
}APP_INFO_UNIT;

//启动/禁止主动上报
typedef struct
{
    u8 EventFlg;            //事件标志
    u8  Reserve[3];         //保留
}APP_EVENT_ENABLE;


typedef struct{
	struct
	{
		u8 sec;
		u8 min;
		u8 hour;
		u8 day;
		u8 mon;
		u8 year;
	}time;
	u32 ntb;
}APP_BROAD_TIME;

//事件主动上报报文
typedef struct
{
    u8 MeterMac[6];         //电表地址
    u8 Data[1];             //数据
}APP_EVENT_REPORT;

//停电事件上报
typedef struct
{
    u32 HeadLen:6;          //帧头长度
    u32 Function:6;         //功能码
    u32 DataLen:12;         //数据长度
    u32 Reserve1:8;
    u8  Reserve2[2];
    u8  MeterAddr[6];       //电表地址
    u8  Data[1];            //数据
}APP_POWER_EVENT_REPORT;

typedef struct
{
    u8  upType;
    u16 startTei;
    u8  bitMap[0];
}POWER_EVENT_UP_BITMAP;

typedef struct
{
    u8  upType;
    u16 num;
    u8  data[0][7];
}POWER_EVENT_UP_MAC;

//确认/否认帧否认原因
typedef enum
{
    APP_DENY_TIMEOUT,          //通信超时
    APP_DENY_FLAG_NOT_SUPPORT, //业务标识不支持
    APP_DENY_CCO_BUSY,         //CCO忙
    APP_DENY_NO_RESPONE,       //电表层无应答
    APP_DENY_ERR_FORMAT,       //格式错误
    APP_DENY_OTHER = 0xFF      //其他
}APP_DENY_REASON;

//测试帧下行
typedef struct
{
    u8 TestID;              //测试ID
    u8 Reserve;             //保留
    u16 DataLen;            //数据区长度
    u8 Data[1];             //数据
}APP_TEST_DOWN;
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Model:4;            //测试模式
    u16  Protocol:4;        //规约类型
    u16  DataLen:12;         //转发数据长度
    u8  Data[1];            //数据
}COMM_TEST_PACKET;

typedef struct
{
	u8  src_addr[6];
	u8  dst_addr[6];
	u8  unkonwByte1;//大概是长度大于0x14 为3b 否则为fb
	u8  unkonwByte2;//取值是0x1 0x2 0x4 应该是三个比特代表不同的意思
	u8  fix_byte[3];//0x11 01 ff
	u16 dl645_len;
	u8  send_flag;
	u8  rec_flag;
	u16 seq;//查询序列
	u16 res1;
	u8 data[0];
}Reader_Support;

//测试模式
typedef enum
{
    APP_TEST_PHY_BACK,           //回环测试模式
    APP_TEST_THROUGH,           //透明转发模式
    APP_TEST_CHANGE,            //原本的切频帧

    APP_TEST_NEW_MODE=2,        //新的测试模式的帧         
    APP_TEST_PHY_APP_THROUGH,   //应用层透传
    APP_TEST_PHY_APP_BACK,      //应用层回传
    APP_TEST_PHY_THROUGH,       //物理层透传(payload透传)
    APP_TEST_BACK,             //物理层回传(payload按位取反)
    APP_TESE_MAC_THROUGH,       //mac层透传
    APP_TESE_CHANGE_PLC,        //频段切换
    APP_TESE_TONEMASK,          //tone mask 配置动作
    APP_TESE_CHANGE_HRF,        //无线信道切换动作
    APP_TESE_HRF_BACK,          //hrf物理层回传
    APP_TESE_HRF_THROUGH,       //hrf物理层透传
    APP_TESE_HRF_HPLC_BACK,     //hrf&hplc物理层回传
    APP_TESE_PLC2HRF,           //plc转到hrf
    SECUER_TO_UART,             //加密测试模式
	APP_TEST_NORMAL = 0xFF,     //正常模式
}APP_TEST_MODEL;


//台区户变关系报文
#pragma pack(1)
typedef struct
{
    u8  HeadLen:6;          //报文头长度
	u8  CollectPhase:2;		//采集相位
	u8  Reserve2[3];		//保留
	u8  MacAddr[6];			//报文地址
	u8  FeatureType;		//特征类型(工频电压，工频频率，工频周期)
	u8  CollectType;		//采集类型
	u8  Data[1];			//数据
}ZONE_RELATION_PACKET;

//特征类型
typedef enum
{
	POWER_FEATURN_NULL	= 0,		//无效值
	POWER_FEATURN_VOLTAGE = 1,		//工频电压
	POWER_FEATURN_FREQUENCE,		//工频频率
	POWER_FEATURN_PERIOD,			//工频周期
	POWER_FEATURN_NUM = 3,			//数量
}ZONE_FEATURN_TYPE;

//采集类型
typedef enum
{
	COLLECT_FEATURE_BEGIN = 1,		//采集开始
	COLLECT_FEATURE_COLLECT,		//特征收集
	COLLECT_FEATURE_INFORM,			//信息告知
	COLLECT_FEATURE_QUERY,			//结果查询
	COLLECT_FEATURE_INFO,			//结果告知
	COLLECT_PHASE_FEATURE_IND,		//相位特征采集指示
	COLLECT_PHASE_FEATURE_INFORM=0x7,	//相位特征信息告知
}ZONE_COLLECT_TYPE;

//采集启动
#pragma pack(1)
typedef struct
{
	u32 BeginNTB;				//起始NTB
	u8  Period;					//周期
	u8  CollectNum;				//采集数量
	u8  CollectSeq;				//采集序列号
	u8  Reserve;				//保留
}ZONE_COLLECT_START;

//台区特征信息告知
#pragma pack(1)
typedef struct
{
	u16 TEI:12;					//TEI
	u16 CollectType:2;			//采集方式
	u16 Reserve1:2;				//保留
	u8  CollectSeq;				//采集序列号
	u8  InformNum;				//告知数量
	u32 BeginNTB;				//起始NTB
	u8 	Data[1];				//数据
}ZONE_FEATURE_INFORM;

//台区告知内容
#pragma pack(1)
typedef struct
{
	u8 Reserve;					//保留
	u8 PhasexNum[3];			//第一出线
	s16 Data[0];				//实际数据
}ZONE_INFORM_DATA;

//台区结果报文
#pragma pack(1)
typedef struct
{
	u16 TEI;					//TEI
	u8  Flag;					//过程标志
	u8  Result:3;					//结果
	u8  corr:5;
	u8  CCOAddr[6];				//应该隶属的CCO地址
}ZONE_RESULT;

//相位特征采集指示
#pragma pack(1)
typedef struct
{
	u8  CollectNum;				//采集数量
	u8  CollectSeq;				//采集序列号
	u16 Reserve;				//保留
}PHASE_COLLECT_IND;

//相位特征信息告知
#pragma pack(1)
typedef struct
{
	u16 TEI:12;					//TEI
	u16 CollectType:2;			//采集方式
	u16 Reserve1:2;				//保留
	u8  CollectSeq;				//采集序列号
	u8  InformNum;				//告知数量
	u32 BaseNTB;				//基准NTB
	u8  Reserve2;				//保留
	u8  Phase1Num;				//相线1过零NTB差值数量
	u8  Phase2Num;				//相线2过零NTB差值数量
	u8  Phase3Num;				//相线3过零NTB差值数量
	u8 	Data[1];				//数据
}PHASE_FEATURE_INFORM;

typedef struct {
    u8 meter_type;
    u8 num;
    struct {
        u32 ident;
        u8 period;
    }item[0];
}GRAPH_ITEM;

typedef struct {
    u8 meter_type;
    u8 num;
    struct {
        u32 ident;
    }item[0];
}GRAPH_ITEM_QUERY_DOWN;

typedef struct {
    u8 meter_type;
    u8 num;
    struct {
        u32 ident;
		u8 time_data[5];
		u8 count;
    }item[0];
}GRAPH_ITEM_READ_DOWN;

//南网21标准深化应用曲线抄读下行
typedef struct {
    u8 meter_type;
	u8 time_data[5];
    u8 count; //采集点数量
	u8 period;
	u8 DINum; //数据项数量
    struct {
        u32 ident;
    }item[0];
}GRAPH_ITEM_READ_CSG21_DOWN;
#pragma pack(4)
#endif

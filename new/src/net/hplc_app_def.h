#ifndef _HPLC_APP_DEF_H_
#define _HPLC_APP_DEF_H_
//应用报文
#pragma pack(1)
typedef struct
{
    u8  Port;           //报文端口号
    u16 PacketID;       //报文ID
    u8  Ctl;            //报文控制字
    u8  Data[1];        //业务报文
}APP_PACKET, *P_APP_PACKET;

//报文端口号
typedef enum
{
    HPLC_APP_PORT_COMMON = 0x11,
    HPLC_APP_PORT_UPDATE = 0x12,
    HPLC_APP_PORT_RCU    = 0x13,//停电查询从节点状态信道 透传至模块  广播校时
    HPLC_APP_PORT_FACTORY= 0xFF,//生产扩展
}HPLC_APP_PORT_ID;

//报文ID
typedef enum
{
    APP_PACKET_COLLECT = 0x0001,        //集中器主动抄表
    APP_PACKET_ROUTE   = 0x0002,        //路由主动抄表
    APP_PACKET_PARALLEL = 0x0003,       //集中器主动并发抄表
    APP_PACKET_TIMING = 0x0004,         //校时
    APP_PACKET_COMM_TEST = 0x0006,      //通信测试
    APP_PACKET_EVENT_REPORT = 0x0008,   //事件上报
    APP_PACKET_CHECK_REGISTER = 0x0011, //查询从节点主动注册
    APP_PACKET_START_REGISTER = 0x0012, //启动从节点主动注册
    APP_PACKET_STOP_REGISTER = 0x0013,  //停止从节点主动注册
    APP_PACKET_CONFIRM_DENY = 0x0020,   //确认/否认
    APP_PACKET_DATA_COLLECT = 0x0021,   //数据汇集
    APP_PACKET_START_UPDATE = 0x0030,   //开始升级
    APP_PACKET_STOP_UPDATE = 0x0031,    //停止升级
    APP_PACKET_FILE_TRAN   = 0x0032,    //传输文件
    APP_PACKET_FILE_LOCAL  = 0x0033,    //传输文件(单播转本地广播)
    APP_PACKET_CHECK_UPDATE = 0x0034,   //查询站点升级状态
    APP_PACKET_UPDATE       = 0x0035,   //执行升级
    APP_PACKET_CHECK_STA    = 0x0036,   //查询站点信息
    APP_PACKET_READER_CONNECT = 0x0040,  //抄控器相关
	APP_PACKET_SYNC_RTC = 0x0070,  //RTC同步报文
    APP_PACKET_SECURTY      = 0x00A0,   //鉴权安全
    APP_PACKET_AREA_DISCERNMENT = 0x00A1,   //台区识别
    APP_PACKET_GET_ID            = 0x00A2,  //查询ID信息
    APP_PACKET_ACCURATE_TIMING   = 0X00A3,     //精准校时
    APP_PACKET_PHASE_ERR_SWITCH = 0x00B0,  //零线电流异常配置开关
    APP_PACKET_STORE_DATA_TIMING = 0x00B1,  //存储曲线数据广播校时
    APP_PACKET_STORE_DATA_SYNC_CFG = 0x00B2,  //存储曲线数据配置采集方案
    APP_PACKET_STORE_DATA_PARALLEL = 0x00B3,  //存储曲线数据并发抄表
    APP_PACKET_GET_STA_EDGE          = 0x00B4,  //查询电表属性
    APP_PACKET_SET_STA_AUTHEN        = 0x00C1,  //sta配置认证开关
#ifdef GDW_ZHEJIANG
	APP_PACKET_BROADCAST_SET_SUBNODE_PARA	= 0x00F0,//广播设置从节点参数
#endif 
    APP_PACKET_EXT_FACTORY_TEST  = 0x00FF,  //生产测试扩展
#ifdef HPLC_CSG
    APP_PACKET_CSG = 0x0101,
#endif

}HPLC_APP_PACKET_ID;

//抄表下行报文格式
#pragma pack(1)
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Config:4;           //配置字
    u16 Protocol:4;         //转发数据的规约类型
    u16 DataLen:12;         //转发数据长度
    u16 PacketSeq;          //报文序号
    u8  Timeout;            //设备超时时间
    u8  Item;               //选项字
    u8  Data[1];            //抄表数据
}READ_METER_DOWN;

//转发数据的规约类型
typedef enum
{
    APP_PRO_TRANSPARENT,    //透明传输
    APP_PRO_DLT645_1997,    //97
    APP_PRO_DLT645_2007,    //07
    APP_PRO_698_45,          //698协议
}APP_METER_PRO;


//集中器主动抄表方式/路由主动抄表选项字
#pragma pack(1)
typedef struct
{
    u8 Reserve1;
    u8 Direction:1;     //0 下行，1上行
    u8 Reserve2:7;
}COLLECT_ACTIVE_READMETER_ITEM;

//数据汇集选项字
#pragma pack(1)
typedef struct
{
    u8 UpLoadSeq;
    u8 Reserve1;
}DATA_COLLECT_ITEM;

//上行报文选项字
#pragma pack(1)
typedef union
{
    COLLECT_ACTIVE_READMETER_ITEM ActiveItem;   //集中器主动抄表方式/路由主动抄表
    u16 ActiveConcurrentItem;                   //集中器主动并发抄表
    DATA_COLLECT_ITEM DataCollect;              //数据汇集
}READ_METER_UP_ITEM;

//抄表上行报文格式
#pragma pack(1)
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 ReplyState:4;       //应答状态
    u16 Protocol:4;         //转发数据的规约类型
    u16 DataLen:12;         //转发数据长度
    u16 PacketSeq;          //报文序号
    READ_METER_UP_ITEM  Item;               //选项字
    u8  Data[1];            //数据
}READ_METER_UP;

//启动从节点注册下行报文格式
#pragma pack(1)
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 FocusReply:1;       //强制应答标志
    u16 Parameter:3;        //从节点注册参数
    u16 Reserve1;           //保留
    u32 Seq;                //报文序号
}REGIST_START_DOWN, *P_REGIST_START_DOWN;

enum
{
    REGIST_NO_FOCUS = 0,    //非强制应答
    REGIST_FOCUS = 1,       //强制应答
};

enum
{
    REGIST_CHECK = 0,       //查询从节点注册结构命令
    REGIST_START = 1,       //启动从节点主动注册命令
};

//查询从节点注册结果下行报文格式
#pragma pack(1)
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 FocusReply:1;       //强制应答标志
    u16 Parameter:3;        //从节点注册参数
    u16 Reserve1;           //保留
    u32 Seq;                //报文序号
    u8  SrcMac[6];           //源MAC地址
    u8  DstMac[6];          //目的MAC地址
}REGIST_CHECK_DOWN, *P_REGIST_CHECK_DOWN;

//查询从节点注册结果上行报文格式
#pragma pack(1)
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 State:1;            //状态字段
    u16 Parameter:3;        //从节点注册参数
    u8  MeterNum;           //电能表数量
    u8  ProductType;        //产品类型
    u8  DeviceAddr[6];      //设备地址
    u8  DeviceID[6];        //设备ID
    u32 PacketSeq;          //报文序号
    u32 Reserve1;           //保留
    u8  SrcMac[6];          //源MAC地址
    u8  DstMac[6];          //目的MAC地址
    u8  Data[1];            //P_REGIST_METER_DATA
}REGIST_CHECK_UP, *P_REGIST_CHECK_UP;

typedef struct
{
    u8  MeterAddr[6];       //电能表地址
    u8  Protocol;           //规约类型
    u8  ModelType:4;        //模块类型
    u8  Reserve2:4;         //保留
}REGIST_METER, *P_REGIST_METER;

//停止从节点注册下行报文格式
#pragma pack(1)
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;          //保留1
    u16 Reserve2;           //保留2
    u32 Seq;                //序号
}REGIST_STOP_DOWN, *P_REGIST_STOP_DOWN;

//校时下行报文格式
#pragma pack(1)
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;          //保留1
    u16 Reserve2:4;           //保留2
    u16 DataLen:12;         //数据长度
    u8  Data[1];            //数据
}TIMING_DOWN, *P_TIMING_DOWN;


typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Dir:1;             //方向位
    u16 Start:1;           //启动位
    u16 Reserve:2;         //保留2
    u16 Seq;               //序号
    u8 SrcMac[6];
    u8 Cmd;
    u8 DataLen;
    u8 Data[0];
}PHASE_ERR_SWITCH, *P_PHASE_ERR_SWITCH;
//精准校时下行报文格式
#pragma pack(1)
typedef struct
{
    u32 AppProtocol:6;      //协议版本号
    u32 HeadLen:6;          //报文头长度
    u32 DataLen:12;         //转发数据长度
    u32 Seq:8;              //报文序号
    u32 NTB;                //NTB
    u8  Data[1];            //数据
}ACCURATE_TIMING_DOWN, *P_ACCURATE_TIMING_DOWN;

//事件报文格式
#pragma pack(1)
typedef struct
{
#ifdef HPLC_CSG
    u32 HeadLen:6;          //报文头长度
    u32 Function:6;         //功能码
    u32 DataLen:12;         //数据长度
    u32 :8;
    u16 res;
    u8  MeterAddr[6];       //电能表地址
    u8  Data[1];            //数据
#else                       
    u32 AppProtocol:6;      //协议版本号
    u32 HeadLen:6;          //报文头长度
    u32 Direction:1;        //方向
    u32 StartFlag:1;        //启动位
    u32 Function:6;         //功能码
    u32 DataLen:12;         //数据长度
    u16 Seq;                //报文序号
    u8  MeterAddr[6];       //电能表地址
    u8  Data[1];            //数据
#endif
}EVENT_PACKET, *P_EVENT_PACKET;

#pragma pack(1)
typedef struct
{
    u8  EventType;          //1停电，2上电
    u16 EventTEI;
    u8  EventBitmap[1];
}EVENT_DATA_BITMAP, *P_EVENT_DATA_BITMAP;

typedef struct
{
    u8 mac[6];
    u8 phaseL[3];
    u8 phaseN[3];
    u8 diff[3];
    u8 time[6];
}A_PHASE_ITEM;
typedef struct
{
    u8  EventType;          //6  零线电流异常上报
    u16 MeterNum;
    A_PHASE_ITEM  item[0];
}EVENT_A_PHASE;
#pragma pack(1)
typedef struct
{
    u8  EventType;          //3停电，4上电
    u16 MeterCount;
    u8  MacAddr[1][MAC_ADDR_LEN+1];
}EVENT_DATA_MAC, *P_EVENT_DATA_MAC;

#ifdef GDW_ZHEJIANG
#pragma pack(1)
typedef struct
{
    u8  EventType;          //50:代表采集器主动上报结果
    u8  DeviceAddr[MAC_ADDR_LEN];
    u8  MeterCount;
	u8  Data[1];//sub node info
}EVENT_COLLECTOR_DATA_MAC, *P_EVENT_COLLECTOR_DATA_MAC;

#pragma pack(1)
typedef struct
{
    u8 node_addr[6];
    u8 protocol;
}EVENT_COLLECTOR_SUB_NODE_INFO, *P_EVENT_COLLECTOR_SUB_NODE_INFO;

#endif


typedef enum
{
    EVENT_FUNC_UP_REPORT = 1,               //STA 主动上报事件给 CCO
    EVENT_FUNC_DOWN_OK = 1,                 //CCO 应答确认给 STA
    EVENT_FUNC_DOWN_REPORT_ENABLE,          //CCO 下发允许事件主动上报给 STA
    EVENT_FUNC_DOWN_REPORT_DISABLE,         //CCO 下发禁止事件主动上报给 STA
    EVENT_FUNC_DOWN_BUFFER_FULL            //CCO 应答事件缓存区满给 STA
}EVENT_FUNC_CODE;

typedef enum
{
    EVENT_DIR_DOWN,
    EVENT_DIR_UP
}EVENT_DIRECTION;

typedef enum
{
    EVENT_FROM_RSP,
    EVENT_FROM_CMD
}EVENT_START;

//通信测试命令下行报文格式
#pragma pack(1)
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;         //保留
    u16 Protocol:4;           //规约类型
    u16 DataLen:12;             //转发数据长度

    u16 secure_mode:4;
    u16 phr_mcs:4;
    u16 psdu_mcs:4;
    u16 pb_size:4;
    u8  data[0]; //安全测试模式数据区
}COMM_TEST_PACKET, *P_COMM_TEST_PACKET;

typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;         //保留
    u16 Protocol:4;           //规约类型
    u16 DataLen:12;             //转发数据长度
    u8  data[1];
}COMM_TEST_FRAME_PACKET, *P_COMM_TEST_FRAME_PACKET;

typedef enum
{
    TEST_MODE_NONE = 0,                             //非测试模式
    TEST_MODE_TRAN_APP_FRAME_TO_COMM_ONCE,          //转发应用层报文至串口信道一次
    TEST_MODE_TRAN_APP_FRAME_TO_HPLC_ONCE,          //转发应用层报文至载波信道一次
    TEST_MODE_TRAN_MPDU_TO_COMM,                    //物理层透传测试模式
    TEST_MODE_PASS_MPDU_BACK,                       //物理层回传测试模式
    TEST_MODE_TRAN_MAC_TO_COMM,                     //MAC层透传测试模式
    TEST_MODE_FREQUENCE,                            //频率切换模式
    TEST_MODE_TONEMASK,                             //ToneMask配置操作
    TEST_MODE_HRF_CHANNEL_CHANGE,                   //无线信道切换
    TEST_MODE_HRF_PACKET_TO_HRF,                    //无线回传模式
    TEST_MODE_HRF_PACKET_TO_UART,                   //无线透传模式
    TEST_MODE_HRF_PLC_PACKET_TO_HRF_PLC,            //无线&PLC回传模式
    TEST_MODE_PLC_PACKET_TO_HRF,                                    //PLC回传至无线
    TEST_MODE_SECUER_TO_UART,                       //安全测试模式
}COMM_TEST_MODE;
typedef enum
{
    APP_TEST_PHY_APP_THROUGH=1,   //应用层透传
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
}APP_TEST_MODEL;
//确认/否认报文
#pragma pack(1)
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Direction:1;        //方向位
    u16 Confirm:1;          //确认位
    u16 Reserve1:2;         //保留
    u16 Seq;                //报文序号
    u16 Item;               //选项字
}CONFIRM_DENY_PACKET;

//开始升级
#pragma pack(1)
typedef struct
{
    u32 AppProtocol:6;      //协议版本号
    u32 HeadLen:6;          //报文头长度
    u32 Reserve1:20;        //保留
    u32 UpdateID;           //升级ID
    u16 UpdateMinutes;      //升级时间窗
    u16 BlockSize;          //升级块大小
    u32 FileSize;           //升级文件大小
    u32 FileCheckSum;       //文件校验
}UPDATE_START_DOWN, *P_UPDATE_START_DOWN;

typedef struct
{
    u32 AppProtocol:6;      //协议版本号
    u32 HeadLen:6;          //报文头长度
    u32 Reserve1:12;        //保留
    u32 UpdateResult:8;     //开始升级结果码
    u32 UpdateID;           //升级ID
}UPDATE_START_UP, *P_UPDATE_START_UP;

//停止升级
#pragma pack(1)
typedef struct
{
    u32 AppProtocol:6;      //协议版本号
    u32 HeadLen:6;          //报文头长度
    u32 Reserve1:20;        //保留
    u32 UpdateID;           //升级ID
}UPDATE_STOP_DOWN, *P_UPDATE_STOP_DOWN;

//传输文件数据
#pragma pack(1)
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;         //保留
    u16 BlockSize;          //数据块大小
    u32 UpdateID;           //升级ID
    u32 BlockID;            //数据块编号
    u8  BlockData[1];       //数据块
}UPDATE_DATA_DOWN, *P_UPDATE_DATA_DOWN;

//查询升级状态
#pragma pack(1)
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;         //保留
    u16 BlockCount;         //连续查询块数
    u32 StartBlockID;       //起始块号
    u32 UpdateID;           //升级ID
}UPDATE_STATUS_DOWN, *P_UPDATE_STATUS_DOWN;

typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 UpdateStatus:4;     //升级状态
    u16 BlockCountValid;    //有效块数
    u32 StartBlockID;       //起始块号
    u32 UpdateID;           //升级ID
    u8  Bitmap[1];          //位图
}UPDATE_STATUS_UP, *P_UPDATE_STATUS_UP;

//执行升级
#pragma pack(1)
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;         //保留
    u16 ResetAfterSeconds;  //等待复位时间
    u32 UpdateID;           //升级ID
    u32 TestRunSeconds;     //试运行时间
}UPDATE_EXE_DOWN, *P_UPDATE_EXE_DOWN;

typedef enum
{
    EM_STA_FACTORY_CODE,        //厂商编号，2 字节 ASCII
    EM_STA_VERSION,             //版本信息，2 字节 BCD 码（硬件版本号和软件版本号各占一个字节）
    EM_STA_BOOT_LOADER,         //Bootloader，1 字节 BIN
    EM_STA_CRC,                 //CRC-32，4 字节
    EM_STA_FILE_SIZE,           //文件长度，4 字节
    EM_STA_DEV_TYPE             //设备类型
}EM_UPDATE_STA_INFO;

#pragma pack(1)
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;         //保留
    u16 ElementCount;       //信息列表元素个数
    u8  Elements[1];
}UPDATE_STA_INFO_DOWN, *P_UPDATE_STA_INFO_DOWN;

typedef struct
{
    u32 AppProtocol:6;      //协议版本号
    u32 HeadLen:6;          //报文头长度
    u32 Reserve1:12;        //保留
    u32 ElementCount:8;     //信息列表元素个数
    u32 UpdateID;           //升级ID
    u8  Elements[1];
}UPDATE_STA_INFO_UP, *P_UPDATE_STA_INFO_UP;

//读取ID信息
typedef struct {
	u16 AppProtocol:6;
	u16 HeadLen:6;
	u16 Dir:1;
	u16 Id_type:3;
	u16 FrameId;
	u8 Data[0];
}READ_ID;
typedef struct{
	u8 len;
	u8 id_data[11];
	u8 device_type;
}Moudle_ID_Info;
typedef struct{
	u8 len;
	u8 id_data[24];
	u8 device_type;
}Chip_ID_Info;
//存储曲线数据校时下行报文格式
#pragma pack(1)
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;          //保留1
    u16 Reserve2:4;           //保留2
    u16 DataLen:12;         //数据长度
    u8  Data[0];
}STORE_DATA_BCAST_TIMING_DOWN, *P_STORE_DATA_BCAST_TIMING_DOWN;

typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;         //保留1
    u32 Seq;                //报文序号
    u8  SrcMac[6];          //源地址
	u8  DstMac[6];          //目的地址
    u16 Reserve2:4;         //保留2
    u16 DataLen:12;         //数据长度
    u8  Data[0];            //数据
}STORE_DATA_BCAST_SYNC_CFG_DOWN, *P_STORE_DATA_BCAST_SYNC_CFG_DOWN;

typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;         //保留1
    u32 Seq;                //报文序号
    u8  SrcMac[6];          //源地址
	u8  DstMac[6];          //目的地址
	u8  Result;             //结果，0-成功，1-失败
    u16 Reserve2;           //保留2
}STORE_DATA_BCAST_SYNC_CFG_UP, *P_STORE_DATA_BCAST_SYNC_CFG_UP;

typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;          //保留1
    u8  RTC[6];         //rtc时钟
    u32 ntb;
}SYNC_RTC_DOWN, *P_SYNC_RTC_DOWN;

typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;         //保留1
    u32 Seq;                //报文序号
    u8  SrcMac[6];          //源地址
	u8  DstMac[6];          //目的地址
}GET_STA_EDGE_DOWN;
typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;         //保留1
    u32 Seq;                //报文序号
    u8  SrcMac[6];          //源地址
	u8  DstMac[6];          //目的地址
    u8  meter_type:4;       //电能表类型
    u8  phase:4;            //电表采集方式
    u32 res;                //保留4字节
}GET_STA_EDGE_UP;

typedef enum {
    GD_UPINFO_SEND_INFO,
    GD_UPINFO_SEND_DATE,
    GD_UPINFO_QUERY_STATUS,
    GD_UPINFO_SEND_DONE,
    GD_UPINFO_SEND_BROAD,
}GD_UP_INFO_ID;
//开始升级
#pragma pack(1)
typedef struct
{
    u8 InfoId;             //信息ID
    u8 res[3];
    u8 FileType;           //文件性质
    u8 res2;
    u8 DstAddr[6];        //目的地址
    u32 Crc;              //文件校验
    u32 FileSize;         //文件大小
    u16 BlockCount;       //文件总块数
    u16 Timeout;          //文件传输时间窗
    u32 UpdateId;         //升级ID
}GD_UPDATE_START_DOWN, *P_GD_UPDATE_START_DOWN;

typedef struct
{
    u8 InfoId;             //信息ID
    u8 res[3];
    u32 UpdateId;         //升级ID
    u16 Result;           //结果码
    u16 ErrorCode;        //错误代码
}GD_UPDATE_START_UP, *P_GD_UPDATE_START_UP;


//传输文件数据
#pragma pack(1)
typedef struct
{
    u8 InfoId;             //信息ID
    u8 res[3];
    u16 BlockID;            //数据块编号
    u16 TotalBlockCnt;      //文件总段数
    u32 UpdateID;           //升级ID
    u16 BlockSize;          //数据块大小
    u8  BlockData[0];       //数据块
}GD_UPDATE_DATA_DOWN, *P_GD_UPDATE_DATA_DOWN;

//查询升级状态
#pragma pack(1)
typedef struct
{
    u8 InfoId;             //信息ID
    u8 res[3];
    u32 UpdateID;           //升级ID
    u16 StartBlockID;       //起始块号
    u16 BlockCount;         //连续查询块数
}GD_UPDATE_STATUS_DOWN, *P_GD_UPDATE_STATUS_DOWN;

typedef struct
{
    u8 InfoId;             //信息ID
    u8 res[3];
    u32 UpdateID;           //升级ID
    u16 StartBlockID;       //起始块号
    u8  UpdateStatus;     //升级状态
    u8  res1;
    u8  Bitmap[0];          //位图
}GD_UPDATE_STATUS_UP, *P_GD_UPDATE_STATUS_UP;

//执行升级
#pragma pack(1)
typedef struct
{
    u8 InfoId;             //信息ID
    u8 res[3];
    u32 UpdateID;           //升级ID
    u16 ResetAfterSeconds;  //等待复位时间
}GD_UPDATE_EXE_DOWN, *P_GD_UPDATE_EXE_DOWN;

#pragma pack(1)
typedef struct
{
    u8 InfoId;             //信息ID
    u8 res[3];
    u32 UpdateID;           //升级ID
    u32 ResultCode;         //结果码
}GD_UPDATE_EXE_UP, *P_GD_UPDATE_EXE_UP;

typedef enum
{
    GD_STA_IDEL       =0x0,        //空闲状态
    GD_STA_SENDING    =0x1,        //正在传输文件
    GD_STA_REVEIVE    =0x2,        //接收完成
    GD_STA_OK         =0x5,        //文件接收完成并且已达设备
    GD_STA_FAILED     =0x6,        //文件无法传送到目的设备
}GD_UPDATE_STA_STATUS;

#pragma pack(1)
typedef struct
{
    u8 InfoId;             //信息ID
    u8 res[3];
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;         //保留
    u16 ElementCount;       //信息列表元素个数
    u8  Elements[1];
}GD_UPDATE_STA_INFO_DOWN, *P_GD_UPDATE_STA_INFO_DOWN;

typedef struct
{
    u8 InfoId;             //信息ID
    u8 res[3];
    u32 AppProtocol:6;      //协议版本号
    u32 HeadLen:6;          //报文头长度
    u32 Reserve1:12;        //保留
    u32 ElementCount:8;     //信息列表元素个数
    u32 UpdateID;           //升级ID
    u8  Elements[1];
}GD_UPDATE_STA_INFO_UP, *P_GD_UPDATE_STA_INFO_UP;







//台区识别
#pragma pack(1)

typedef struct
{
    u32 NTB;
    u8  CollectionPeriod;
    u8  CollectionNum;
    u8  CollectionSeq;
    u8  Reserve;
}AREA_DISCERNMENT_START_DATA, *P_AREA_DISCERNMENT_START_DATA;


typedef struct
{
    u16 TEI:12;
    u16 CollectionType:2;
    u16 Reserve:2;
    u8  CollectionSeq;
    u8  Total;
    u32 NTB1;
    u8  Other[1];
}AREA_DISCERNMENT_FEATURE_DATA, *P_AREA_DISCERNMENT_FEATURE_DATA;

typedef struct
{
    u16 TEI;
    u8  CompleteFlag;
    u8  Result;
    u8  MainNodeMacAddr[MAC_ADDR_LEN];
}AREA_DISCERNMENT_RESULT_DATA, *P_AREA_DISCERNMENT_RESULT_DATA;

typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Direction:1;        //方向位
    u16 StartFlag:1;        //启动位
    u16 Phase:2;            //相位
    u16 Seq;                    //报文序号
    u8  MacAddr[MAC_ADDR_LEN];  //MAC地址
    u8  FeatureType;            //特征类型
    u8  CollectionType;         //采集类型
    u8  Data[1];            //数据
}AREA_DISCERNMENT_DOWN, AREA_DISCERNMENT_UP, *P_AREA_DISCERNMENT_DOWN, *P_AREA_DISCERNMENT_UP;

#pragma pack(1)
typedef struct
{          
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserved:4;         //保留
    u8  Function;           //功能码
    u16 DataLen;            //数据长度
    u16 Seq;                //报文序号
    u8  Mac[6];             //从节点MAC地址
    u8  Data[0];            //数据  
}READ_INNER_VER, *P_READ_INNER_VER;
typedef struct
{          
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserved:4;         //保留
    u8  Function;           //功能码
    u16 DataLen;            //数据长度
    u16 Seq;                //报文序号
    u8  Mac[6];             //从节点MAC地址
    u8  Data[0];            //数据  
}STA_POWER, *P_STA_POWER;
typedef struct
{          
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserved:4;         //保留
    u8  Function;           //功能码
    u16 DataLen;            //数据长度
    u16 Seq;                //报文序号
    u8  Mac[6];             //从节点MAC地址
    u8  Data[0];            //数据  
}READ_REGISTER, *P_READ_REGISTER;

typedef struct
{
    u16 AppProtocol:6;      //协议版本号
    u16 HeadLen:6;          //报文头长度
    u16 Reserve1:4;         //保留
    u16 Seq;                //报文序列号
    u8  IsEnable;           //是否开启营销和配电
}SET_STA_AUTHEN;

#ifdef GDW_ZHEJIANG
#pragma pack(1)
//在网锁定时间
typedef struct
{
	u32 protocol:6;
	u32 FrameHeadLen:6;
	u32 FrameSN:8;
	u32 DataLen:12;
	u8 data[1];
}APP_ONLINE_LOCK_TIME_PACKET, *P_APP_ONLINE_LOCK_TIME_PACKET;

#pragma pack(1)
typedef struct
{
	u8 FunctionCode;
	u16 OnlineLockTime;
	u16 AbnormalOfflineLockTime;
}APP_ONLINE_LOCK_TIME, *P_APP_ONLINE_LOCK_TIME;
#endif

#ifdef HPLC_CSG

typedef struct
{
    u8  CollectionNum;
    u8  CollectionSeq;
	u16 res;
}APP_PHASE_DISCERNMENT_START_DATA, *P_APP_PHASE_DISCERNMENT_START_DATA;
//应用层业务报文

typedef enum
{
    EM_BUSINESS_NORESPONSE,     //无需响应
    EM_BUSINESS_NEED_RESPONSE,  //需要响应
}EM_BUSINESS_RESPONSE_FLAG;

typedef enum
{
    EM_CMD_BUSINESS_READ_METER_LIST,        //查询搜表结果
    EM_CMD_BUSINESS_WRITE_METER_LIST,       //下发搜表列表
    EM_CMD_BUSINESS_SEND_FILE,              //文件传输
	EM_CMD_BUSINESS_EBENT_SWITCH,           //允许/禁止从节点事件
	EM_CMD_BUSINESS_REBOOT_STA = 0x04,      //从节点重启
    EM_CMD_BUSINESS_READ_STA_INFO,          //从节点信息查询
    EM_CMD_BUSINESS_WRITE_COMM_ADDR,        //下发通信地址映射表列表
    EM_CMD_BUSINESS_STA_RUN_STATUS,         //查询从节点运行状态信息
    EM_CMD_BUSINESS_STA_CHANNEL_INFO,       //查询从节点信道信息
    EM_CMD_BUSINESS_BROAD_TIME,             //深化应用广播校时
    EM_CMD_BUSINESS_AREA_DISCERNMENT=0x10,  //台区识别
    EM_CMD_BUSINESS_READ_TASK_INIT = 0x20,  //初始化采集任务
    EM_CMD_BUSINESS_ADD_READ_TASK,          //添加采集任务
    EM_CMD_BUSINESS_DELETE_READ_TASK,       //删除采集任务
    EM_CMD_BUSINESS_READ_TASK_ID,           //查询采集任务号
    EM_CMD_BUSINESS_READ_TASK_DETAIL,       //查询采集任务详细信息
    EM_CMD_BUSINESS_READ_TASK_DATA,         //采集任务数据
    EM_CMD_BUSINESS_TIMING = 0x30,          //校时命令
    EM_CMD_BUSINESS_COMM_TEST = 0xF0,       //测试帧
    EM_CMD_BUSINESS_INNER_VER = 0x3B,       //从节点内部版本 
    EM_CMD_BUSINESS_REGISTER = 0x70			//读从节点对应寄存器值
}EM_CMD_BUSINESS_ID;

//深化应用业务代码
typedef enum
{
	APP_SHENHUA_APP_CODE_TANSPARENT = 0x00, //透传645报文，目前没有使用
	APP_SHENHUA_APP_CODE_TIMING,            //精准校时
	APP_SHENHUA_APP_CODE_GRAPH,             //负荷曲线采集与存储
	APP_SHENHUA_APP_CODE_MAX = APP_SHENHUA_APP_CODE_GRAPH
}HPLC_APP_SHENHUA_APP_CODE;
	
//负荷曲线采集与存储功能码
typedef enum
{
	APP_GRAPH_FUNC_CODE_CONFIG = 0x01, //配置采集间隔
	APP_GRAPH_FUNC_CODE_QUERY,         //查询采集间隔
	APP_GRAPH_FUNC_CODE_READ,          //抄读数据项
	APP_GRAPH_FUNC_CODE_READ_21PROTOCOL = APP_GRAPH_FUNC_CODE_QUERY //21标准抄读数据项
}HPLC_APP_GRAPH_FUNC_CODE;

#pragma pack(1)
typedef struct
{
    u8 HeadLen:6;          //报文头长度
    u8 Phase:2;            //相位
    u8 res[3];
    u8 MacAddr[MAC_ADDR_LEN];  //MAC地址
    u8 FeatureType;            //特征类型
    u8 CollectionType;         //采集类型
    u8 Data[0];            //数据
}AREA_DISCERNMENT_CSG_DOWN, AREA_DISCERNMENT_CSG_UP, *P_AREA_DISCERNMENT_CSG_DOWN, *P_AREA_DISCERNMENT_CSG_UP;

typedef struct {
    u8 type;
	u8 seq;
    u16 len;
    u8 data[0];
}READER_CCO_FRAME;

typedef struct {
    u8 type;
	u8 seq;
    u8 start:1;
    u8 :7;
    u32 rate;
    u32 res;
    u16 len;
    u8 data[0];
}READER_UART_FRAME;
typedef struct
{
    u16  FrameType:4;        //帧类型域
    u16  Reserve:8;
    u16  ExtFieldFlag:1;     //业务扩展域标识位
    u16  ResponseFlag:1;     //响应标识位
    u16  StartFlag:1;        //启动标志位
    u16  Direction:1;        //传输方向位
}APP_BUSINESS_CTRL, *P_APP_BUSINESS_CTRL;

typedef struct
{
    APP_BUSINESS_CTRL Ctrl;
    u8  BusinessID;         //业务标识
    u8  Version;            //应用版本号
    u16 FrameSeq;           //帧序号
    u16 FrameLen;           //帧长
    u8  Data[1];            //业务数据单元
                            //业务扩展域
}APP_BUSINESS_PACKET, *P_APP_BUSINESS_PACKET;



typedef struct
{
    struct{
		u8 bcd_sec;
		u8 bcd_min;
		u8 bcd_hour;
		u8 bcd_day;
		u8 bcd_mon;
        u8 bcd_year;        
    };
    u32 ntb;
}APP_DATA_BROAD_TIME, *P_APP_DATA_BROAD_TIME;


//下一代电表扩展
typedef struct
{
    u16 Port;            //端口号
    u8 Seq;              //序号
    u8 Reserve;          //保留
    u32 NTB;             //发送NTB
    u8 data[0];          //数据报文
}APP_BROADCST_TIME_EXT;
//数据转发业务
typedef struct
{
    u8  SrcAddr[6];  //源地址
    u8  DstAddr[6];  //目的地址
    u8  Timeout;     //设备超时时间, 单位： 100毫秒。 值为0时， STA取默认超时时间。
    u8  Reserve;
    u16 DataLen;     //转发数据长度
    u8  Data[1];     //转发数据内容   
}APP_DATA_TRANS_DOWN, *P_APP_DATA_TRANS_DOWN;

//数据转发（电能表精准校时）
typedef struct
{
    u8  SrcAddr[6];  //源地址
    u8  DstAddr[6];  //目的地址
	u8  Reserve;
	u8  App_Code;    //业务代码
    u16 DataLen;     //转发数据长度
    u8  Data[1];     //转发数据内容   
}APP_DATA_TRANS_MOUDLE_TIMING_DOWN, *P_APP_DATA_TRANS_MOUDLE_TIMING_DOWN;

//深化应用数据透传下行
typedef struct
{
    u8  SrcAddr[6];  //源地址
    u8  DstAddr[6];  //目的地址
#ifdef PROTOCOL_NW_2020_OLD //南网电科院20年8月深化应用标准
	u8  Timeout;     //设备超时时间, 单位： 100毫秒。 值为0时， STA取默认超时时间。
#endif    
    u8  Reserve;
	u8 App_Code;
    u16 DataLen;     //转发数据长度
    u8  Data[1];     //转发数据内容   
}APP_DATA_TRANS_MOUDLE_DOWN, *P_APP_DATA_TRANS_MOUDLE_DOWN;

typedef struct
{
    u8  SrcAddr[6];  //源地址
    u8  DstAddr[6];  //目的地址
    u16 Reserve;
    u16 DataLen;     //转发数据长度
    u8  Data[1];     //转发数据内容
}APP_DATA_TRANS_UP, *P_APP_DATA_TRANS_UP;

typedef struct
{
    u8  num;          //电表数量
    u8  res[3];       //保留
    u8  data[0];
}APP_SEARCH_UP, *P_APP_SEARCH_UP;

typedef struct
{
    u8  MeterAddr[6];       //电能表地址
    u8  res[2];       //保留
}APP_SEARCH_METER, *P_APP_SEARCH_METER;

//深化应用数据透传上行
typedef struct
{
    u8  SrcAddr[6];  //源地址
    u8  DstAddr[6];  //目的地址
    u8  Reserve;
	u8  App_Code;
    u16 DataLen;     //转发数据长度
    u8  Data[1];     //转发数据内容
}APP_DATA_MODULE_TRANS_UP, *P_APP_DATA_MODULE_TRANS_UP;

//南网深化应用曲线抄读数据上行
typedef struct
{
	u8  FuncCode;        //功能码
    u8  MeterType;       //表类型
    u8  ItemNum;         //数据项数量
    u8  Data[0];
}APP_READ_GRAPH_ITEM_UP, *P_APP_READ_GRAPH_ITEM_UP;

//南网21标准深化应用曲线抄读数据上行
typedef struct
{
	u8  FuncCode;        //功能码
    u8  MeterType;       //表类型
    u8  Time[5];         //起始点时间
    u8  Count;           //采集点数量
    u8  Period;          //采集间隔
    u8  ItemNum;         //数据项数量
    u8  Data[0];
}APP_READ_GRAPH_ITEM_CSG21_UP, *P_APP_READ_GRAPH_ITEM_CSG21_UP;

//南网21标准深化应用曲线抄读数据下行
typedef struct
{
	u8  FuncCode;        //功能码
    u8  MeterType;       //表类型
    u8  Time[5];         //起始点时间
    u8  Count;           //采集点数量
    u8  Period;          //采集间隔
    u8  ItemNum;         //数据项数量
    u8  Data[0];
}APP_READ_GRAPH_ITEM_CSG21_DOWN, *P_APP_READ_GRAPH_ITEM_CSG21_DOWN;

//南网深化应用曲线抄读具体数据项
typedef struct
{
    u32 ItemDI;          //第一个数据项的数据标识
    u8  ItemTime[5];     //第一个数据项的起始点时间
    u8  ItemCount;       //第一个数据项的采集点数量
    u8  Data[0];
}GRAPH_ITEM, *P_GRAPH_ITEM;

//南网21标准深化应用曲线抄读具体数据项
typedef struct
{
    u32 ItemDI;          //数据项的数据标识
    u8  Data[0];
}GRAPH_ITEM_CSG21, *P_GRAPH_ITEM_CSG21;

//南网21标准深化应用曲线抄读具体数据项下行
typedef struct
{
    u32 ItemDI;          //数据项的数据标识
}GRAPH_ITEM_DOWN_CSG21, *P_GRAPH_ITEM_DOWN_CSG21;

typedef struct
{
    u8 testId;
    u8 res;
    u16 len;
    u8 data[0];
}APP_Test_Frame;

//事件主动上报
typedef struct
{
    u8  MeterAddr[6];  //电表地址
    u8  Data[1];       //电表主动上报报文
}APP_EVENT_REPORT_UP, *P_APP_EVENT_REPORT_UP;

typedef struct
{
    u16 ResetAfterSeconds;  //等待复位时间 
}APP_RESET_STA_DOWN, *P_APP_RESET_STA_DOWN;

P_APP_PACKET HplcApp_MakeDataTransFrame(u8 src_addr[6], u8 dst_addr[6],u8 app_code, u8 meter_moudel,u16 packet_seq, EM_BUSINESS_RESPONSE_FLAG response_flag, u8* pdata, u16 len, u16* p_frame_len);
P_APP_PACKET HplcApp_MakeCmdFrame(u16 packet_seq, EM_BUSINESS_RESPONSE_FLAG response_flag, EM_CMD_BUSINESS_ID business_id, u8* pdata, u16 len, u16* p_frame_len);
P_APP_PACKET HplcApp_MakeConfirmFrame( u8 business_id, u16 packet_seq,u8 reason, u16 *p_frame_len);
P_APP_PACKET HplcApp_MakeReadFrame(u16 packet_seq, u8* pdata, u16 len, u16* p_frame_len);//抄控器帧

#else
typedef struct {
    u8 type;
    u16 len;
    u8 data[0];
}READER_CCO_FRAME;
P_APP_PACKET HplcApp_MakeReadFrame(u16 packet_seq, u8* pdata, u16 len, u16* p_frame_len);//抄控器帧
P_APP_PACKET HplcApp_MakeReadMeterFrame(HPLC_APP_PACKET_ID packet_id, u16 packet_seq, u8* pdata, u16 len, APP_METER_PRO protocol, u16* p_frame_len);
P_APP_PACKET HplcApp_MakeSetOnlineTimeLockFrame(u16 OnlineLockTime, u16 AbnormalOfflineLockTime, u16 FrameSN);

#endif

P_APP_PACKET HplcApp_MakeEventFrame(u16 packet_seq, EVENT_FUNC_CODE func, EVENT_DIRECTION dir, EVENT_START start, u8 meter_addr[6], u8* pdata, u16 len);
P_APP_PACKET HplcApp_MakeBroadcastTimingFrame(u8* pdata, u16 len);
P_APP_PACKET HplcApp_MakeAccurateTimingFrame(u8 *pdata, u16 len,u8 seq,u32 NTB);
P_APP_PACKET HplcApp_MakePhaseErrFrame(u8 cmd,u8 vaile,u8* mac,u16 seq);

//主动注册
P_APP_PACKET HplcApp_MakeStartSelfRegFrame(u32 packet_seq);
P_APP_PACKET HplcApp_MakeReadSelfRegNodeFrame(u32 packet_seq, u8 src_mac[MAC_ADDR_LEN], u8 des_mac[MAC_ADDR_LEN],u8 param);
P_APP_PACKET HplcApp_MakeStopSelfRegFrame(u32 packet_seq);

//站点升级
#ifdef HPLC_CSG

P_APP_PACKET HplcApp_MakeStartUpdateFrame(u32 update_id, u16 total_time,u16 fileType, u16 block_cnt, u32 file_size, u32 file_checksum,u16 *len, EM_BUSINESS_RESPONSE_FLAG ack);
P_APP_PACKET HplcApp_MakeStopUpdateFrame(u32 update_id,u16 *len);
P_APP_PACKET HplcApp_MakeUpdateDataFrame(u32 update_id, u32 block_id, u8 *block_data, u32 block_size ,u32 block_tatal,u16 *len);
P_APP_PACKET HplcApp_MakeUpdateDataBroadcastFrame(u32 update_id, u32 block_id, u8 *block_data, u32 block_size ,u32 block_tatal,u16 *len);
P_APP_PACKET HplcApp_MakeUpdateStatusFrame(u32 update_id, u32 start_block_id, u16 block_count,u16 *len);
P_APP_PACKET HplcApp_MakeUpdateExeFrame(u32 update_id, u16 reset_after_seconds,u16 *len);

#else
P_APP_PACKET HplcApp_MakeStartUpdateFrame(u32 update_id, u16 update_minutes, u16 block_size, u32 file_size, u32 file_checksum);
P_APP_PACKET HplcApp_MakeStopUpdateFrame(u32 update_id);
P_APP_PACKET HplcApp_MakeUpdateDataFrame(u32 update_id, u32 block_id, u8* block_data, u32 block_size);
P_APP_PACKET HplcApp_MakeUpdateDataBroadcastFrame(u32 update_id, u32 block_id, u8* block_data, u32 block_size);
P_APP_PACKET HplcApp_MakeUpdateStatusFrame(u32 update_id, u32 start_block_id, u16 block_count);
P_APP_PACKET HplcApp_MakeUpdateExeFrame(u32 update_id, u16 reset_after_seconds, u32 test_run_seconds);
P_APP_PACKET HplcApp_MakeUpdateStaInfoFrame(EM_UPDATE_STA_INFO elements[], u8 element_count);
#endif
//台区识别
P_APP_PACKET HplcApp_MakeAreaDiscernmentFrame(u32 packet_seq, u8 src_mac[MAC_ADDR_LEN], u8 feature_type, u8 collection_type, u8* pdata, u16 *len);

//通信测试
P_APP_PACKET HplcApp_MakeCommTestFrame(u8* pdata, u16 len, APP_METER_PRO protocol);
P_APP_PACKET HplcApp_MakeEnterTestModeFrame(COMM_TEST_MODE test_mode, u32 test_value);

P_APP_PACKET HplcApp_MakeStoreDataBcastTimingFrame(u8 *pdata, u16 len);
P_APP_PACKET HplcApp_MakeStoreDataSyncCfgFrame(u32 packet_seq, u8 src_mac[MAC_ADDR_LEN], u8 dst_mac[MAC_ADDR_LEN], u8 *pdata, u16 len);

P_APP_PACKET HplcApp_MakeSyncRTCFrame(u8 *pdata, u16 len,u32 ntb);

P_APP_PACKET HplcApp_MakeGetInnerVerFrame(u8 node_mac[6], u16* len);
P_APP_PACKET HplcApp_MakeStaPowerFrame(u8 node_mac[6], u16 func,void *data,u16* len);
P_APP_PACKET HplcApp_MakeGetRegisterFrame(u8 node_mac[6], void *data, u16* len);
P_APP_PACKET HplcApp_MakeReadStaStatusFrame(u32 element , u16 *len);
P_APP_PACKET HplcApp_MakeReadStaInfoFrame(u32 element , u16 *len);
P_APP_PACKET HplcApp_MakeReadStaInfoFrameFromArray(u8 element_num, u8* element_array, u16 *len);
P_APP_PACKET HplcApp_MakeReadChannelInfoFrame(u16 start_sn, u8 num, u16 *len);
P_APP_PACKET HplcApp_GetStaEdgeFrame(u16 seq,u8 src_mac[MAC_ADDR_LEN], u8 dst_mac[MAC_ADDR_LEN]);
P_APP_PACKET HplcApp_GetModuleIDFrame(u16 seq);
P_APP_PACKET HplcApp_MakeStaAuthenFrame(int en,u16* len);

//南网从节点重启
P_APP_PACKET HplcApp_MakeResetStaFrame(u16 seconds, u16* len);

#pragma pack(4)
#endif

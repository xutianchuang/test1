#ifndef _APPLOCAL_H_
#define _APPLOCAL_H_
#include "lc_app.h"

#define FIRST_READ_ADDR_DELAY   1000

#define MAX_LOCAL_LEN   256

#define MAX_LOCAL_PDU_SIZE      16

//本地处理完成回调函数
typedef void(*pLocalHandelCallback)(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8* data, u16 len, u8 sendType, u16 srcTei);

//传给本地处理的参数
typedef struct
{
    u8 SrcAddr[6];         //地址
    u8 DstAddr[6];      //目的地址
    u8 Timeout;         //超时时间
    u16 Seq;            //帧序号
    u8 SendType;        //帧是广播的还是单播的
    u16 SrcTei;         //源TEI
    u8 Data[MAX_LOCAL_LEN];       //数据
    u16 Size;           //帧长
	pLocalHandelCallback  fun;		//回调函数
	u32 Baud;                       //二采中作为波特率参数  一采中作为本地接口是否解析参数
}LOCAL_PARAM;


//应用层本地任务
void AppLocalTask(void *arg);

//返回是否已经绑定了地址
bool IsBindLocalMac(void);

//解绑地址
void DispatchLocalMac(void);

//进入/退出测试模式
void LocalEnterTestModel(bool);

//返回串口使用的波特率
u32 GetLoaclUartBaud(void);

//绑定地址是否已经超时(目前设定为1分钟)
bool IsBindLocalTimeout(void);

//返回电能表协议
u8 GetMeterProtocol(void);

//接收APP层的数据
bool AppLocalReceiveData(u8 srcAddr[6], u8 dstAddr[6], u8 timeout, u16 seq, u8* data, u16 len, pLocalHandelCallback fun, u8 sendType, u16 srcTei, u32 Baud);

void sendUartDiffRate(u16 seq,u8 *data,u16 len,u32 rate);
u8* GetMeterUpEvent(void);
bool UartBusy(void);
LOCAL_PARAM* MallocLocal(void);
void FreeLocal(LOCAL_PARAM* pdu);

void II_STA_Task(void *arg);
extern u8 I_STA_SEQ;
extern u8 DectecDoublePortocol;

#ifdef INTELLIGENCE_LOADING
void NetStatusNotify(void);
#endif


#define METER_SUPPORT_GRAPH   (1<<1)//电表支持曲线
#define METER_SUPPORT_RATE    (1<<2)//电表支持电压合格率
#define METER_SUPPORT_COMB    (1<<3)//电表支持组合抄表
									
#define DL645_GRAPH_DI        0x061001FF
#define DL645_RATE_DI         0x03700001
#define DL645_COMB_DI         0xEEEEEE01


//越大，优先级越高
#define SEND_LOCAL_ACK_PRIO_VALUE               	(0x50) //立马应答
#define SEND_METER_ACK_DATA_PRIO_VALUE         		(0x30) //抄表应用数据
#define SEND_METER_ACK_STATE_PRIO_VALUE        		(0x30) //抄表应答抄表状态

#define SEND_LOCAL_READ_VERSION_PRIO_VALUE              (0x4A) //
#define SEND_LOCAL_READ_RESET_INFO_PRIO_VALUE              (0x4B) //
#define SEND_LOCAL_CFG_FRAME_PRIO_VALUE                 (0x45) //

#define SEND_METER_EVENT_PRIO_VALUE                 (0x40) //事件上报

#define SEND_REPORT_METER_LIST_PRIO_VALUE           (0x21) //搜表列表上报
#define SEND_REPORT_REGIST_METER_STATU_PRIO_VALUE   (0x20) //搜表状态上报

#define SEND_REPORT_CHECK_ZONE_METER_LIST_PRIO_VALUE	(0x20) //台区识别上报数据
#define SEND_QUERY_CAPINFO_ACK_DATA_PRIO_VALUE			(0x20) //读资产回复应答

#define SEND_CKQ_TRANS_PRIO_VALUE               		(0x20) //抄控器本地优先级

#define SEND_DEFAULT_PRIO_VALUE                 		(0x10) //默认优先级

#define LOCAL_DEFAUT_FRAME_SIZE      (48)
#define CHECK_METER_TIME_OUT_VALUE      (1000/100)
#define READ_METER_PRIO_VALUE               (0x40) //读表
#define CHECK_METER_LIST_PRIO_VALUE         (0x30) //校验表列表
#define DETECT_METER_LIST_PRIO_VALUE        (0x20) //探测表列表
#define FIND_METER_LIST_PRIO_VALUE          (0x10)  //搜表
#define UART_SUCCESS            (0x01)
#define UART_TIME_OUT           (0x02)
#define UART_CHECK_ERROR        (0x04)
#define UART_NOTHING            (0x00)
#define UART_CLEAR_FIFO         (0x02)
#define DLT_97			0x01
#define DLT_07			0x02		//浣跨敤鏁版嵁鏍囪瘑涓?1鐨勬妱琛ㄥ抚鎼滆〃
#define DLT_698         0x03
#define I_LOCAL 		0x04
#define HDLC_PRO		0x08
#define DLT_07_13		0x05		//浣跨敤鏁版嵁鏍囪瘑涓?3鐨勬悳琛ㄥ抚鎼滆〃
#define HDLC_YJ_DLMS_PRO		0x09
#define HDLC_WZ_LOC_PRO		0x0A    //沃州本地协议
#define NEXT_START_CHECK_AND_FIND_METER_TIME        (24*3600*1000)
#define INIT_SCAN_ADDR              (0x01)
#define INCREASE_SCAN_ADDR          (0x02)
#define CARRY_SCAN_ADDR             (0x03)
#define START_FIND_SCAN     (0x01)
#define IN_FIND_SCAN        (0x02)
#define SCAN_METER_IDLE             0x00
#define SCAN_METER_SCANNING         0x01
#define FIND_METER_LIST_TIME_OUT_VALUE      (1000/100)
#define METER_97TYPE                (0x01)
#define METER_07TYPE                (0x02)
#define INVALID_METER_TYPE          (0)
#define RT_TRUE                         1               
#define RT_FALSE                        0
#define FIND_METER_LIST_PRIO_VALUE          (0x10)  //搜表

#define SYSTEM_METER_LIST_NUM       32

enum Uart_Rate_Type
{
    UART_1200 = 0,
    UART_2400 = 1,
    UART_4800 = 2,
    UART_9600 = 3,
    UART_19200 = 4,
    UART_38400 = 5,
    UART_115200 = 6,
    UART_256000 = 7,
    UART_MAX,
};

struct System_Meter_List_Type
{
    struct{
        u8 Id[LONG_ADDR_SIZE];
        u8 UartRate;
        u8 Type;
        u8 UartDbit;
        u8 UartCbit;
        u8 UartSbit;
        u8 UartTxPin;
    }memter[SYSTEM_METER_LIST_NUM];
    u32 meterlen;
    u32 Crc;
};
//系统参数定义结构体
typedef struct System_Params_Type
{
    u8 CollecterIIAddr[LONG_ADDR_SIZE];
    u8 SystemLongAdd[LONG_ADDR_SIZE];
    u8 SystemMacAddr[LONG_ADDR_SIZE];
    u8 SystemUartRate;
    u8 SystemType;
    
    u8 UartDbit;
    u8 UartCbit;
    u8 UartSbit;
    u8 UartTxPin;
    struct System_Meter_List_Type SystemMeterList;
}SYS_RUN_PARA;

enum
{
    UART_PARITY_NO,
    UART_PARITY_EVEN,
    UART_PARITY_ODD,
};

enum
{
    UART_8_DATA_BITS,
    UART_7_DATA_BITS,
};

enum
{
    UART_1_STOP_BITS,
    UART_2_STOP_BITS,
};

enum
{
    UART_TX_OD,
    UART_TX_TTL,
};

enum
{
    CURTAINS_OPEN,
    CURTAINS_CLOSE,
    CURTAINS_CONTROL,
    CURTAINS_STOP,
    CURTAINS_SET_ITINERARY_POINTS,
    CURTAINS_RUN_TO_ITINERARY_POINTS,
    CURTAINS_DEL_ITINERARY_POINTS,
};

enum
{
    FACTORY_NONE = 0,
    FACTORY_SUNFLOWER,
};

bool CurtainsControlCmdSend(uint8_t factory,uint8_t cmd,uint8_t *data,uint16_t datalen);



bool AddSystemMeterToList(u8 * pid,u8 rate,u8 type,u8 udbit,u8 ucbit,u8 usbit,u8 utxpin);

bool CheckMeterListCallBackFunction(void * exparam,u8* data,u16 len,u8 ust);
u8 GetDetectMeterFrame(u8 * pd,u8 type,u8 *addr);
bool FindMeterCallBackFunction(void * exparam,u8* data,u16 len,u8 ust);
u8 GetReadMeterAddrDlt645(u8 *psd,u8 ptype,u8 *paddr);
void StartFindScanMeter();
bool CheckSystemAddr();
void StartMeterListTaskhandle();
void MeterLocalHandle(void);
bool DeleteMeterUartTask(u8 taskindex);
bool IsSystemMeter(u8 *pid, uint8_t *baudrate, uint8_t *parity);

void AddBleBreakerToSystemMeterList();

#endif

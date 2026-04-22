#ifndef _APPLOCAL_H_
#define _APPLOCAL_H_

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
#endif

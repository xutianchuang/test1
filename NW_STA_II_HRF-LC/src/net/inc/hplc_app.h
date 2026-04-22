#ifndef _HPLC_APP_H_
#define _HPLC_APP_H_

#include "hplc_mpdu.h"
typedef struct
{
	u8  SecureTestAlgorithm;//当前安全测试使用算法
	u8  PHR_MCS;
	u8  PSDU_MSC;
	u8  PbSIZE;
}_AppCommTestParm;

typedef struct
{
	u8  mac[6];
	u8  layer;
	u8  up_succ;
	u8  down_succ;
	u8  snr;
	u8  rssi;
}Obj_NeighborTable;

typedef struct
{
	u8  mac[6];
	u8  layer;
	u8  up_succ;
	u8  down_succ;
	u8  conntype;
}Obj_Pco_Info;

uint32_t lc_get_rf_neighbor_info(uint8_t *p_nodes,const uint32_t offset, const uint32_t count,uint16_t *p_sum);
uint32_t lc_get_plc_neighbor_info(uint8_t *p_nodes,const uint32_t offset, const uint32_t count,uint16_t *p_sum);
bool lc_get_pco_info(uint8_t *data);

#define MAX_BROADCAST_TIME_RESULT   10
typedef struct{
	u8 seq;
	u8 result;
}BroadcasetTimeExtResult;

#define APP_PRO_698_45      1
#define APP_PRO_DLT645_1997 2
#define APP_PRO_DLT645_2007 3

u16 ReadMeterFilter(u8 Protocol, u8* data,u16 len);

//应用层初始化
void AppInit(void);

//事件处理任务（中断上报）
void EventTask(void* arg);

//30秒内处理APP测试帧
void AppTestModeDeal(void);

//解析数据
void AppHandleData(BPLC_PARAM *bplcParam, u8* data,u16 len);

//当前通信测试属于什么模式
u8 GetAppCurrentModel(void);

//获取APP序号(自动+1)
u16 GetAppSequence(void);
extern bool EnterTestModel;
extern bool SecureTestModel;
void CannotEnterTestModel(void* arg);
EN_DEVICE_TYPE GetStaType(void);
bool Find_ID_Meter(u32 DI, u8 *idxa, u8 *idxb);
void AppHandleReadModule(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8* data, u16 len, u8 sendType, u16 srcTei);
void AppHandleEvent(u8 addr[6], u8 dstAddr[6], u16 seq, bool vaild, u8* data, u16 len, u8 sendType, u16 srcTei);

extern _AppCommTestParm Test_Parm;
//处理抄表返回
void AppHandleReadMeter(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8* data, u16 len, u8 sendType, u16 srcTei);

//抄控器测试帧返回
void AppHandleReaderBack(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8* data, u16 len, u8 sendType, u16 srcTei);

#define DL645_SET_RECORD_INTERVAL(val) (0x40000A##val)
#define DL645_SET_RECORD_INTERVAL_33 (0x73333D00)
#define DL645_SET_RECORD_READ_33 (0x35003233)
#define DL645_SET_RECORD_READ (0x0200FF00)

void ClearPowerData(void);
void ReportOtherCCO(void);

bool CheckNetCCOMac(uint8_t *mac, uint32_t nid);
#define WHITE_BLACK_MAC_NUM		8

typedef struct __PACKED
{
    uint8_t white_mac_list[WHITE_BLACK_MAC_NUM][6];
    uint8_t black_mac_list[WHITE_BLACK_MAC_NUM][6];
}mac_list_t;

extern mac_list_t g_mac_list;

//事件状态
typedef enum
{
    APP_EVENT_IDLE,         //空闲状态
    APP_EVENT_WAIT_LOCAL,   //已经发送一帧到本地查询事件状态字
    APP_EVENT_WAIT_ACK,     //已经把事件帧发送给CCO等待CCO回应
    APP_EVENT_OVERFLOW,     //CCO回复满，等待重复
    APP_EVENT_NO_ACK,       //CCO没有回复，等待半小时再发
    APP_EVENT_CLEAR_HIGH,   //等待清除低电平
	APP_WAIT_IN_NET,        //等待入网
}APP_EVENT_STATE;
extern APP_EVENT_STATE AppEventState;
extern SEND_PDU_BLOCK* EventPdu;
#endif

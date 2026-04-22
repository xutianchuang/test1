#ifndef _HPLC_SEND_H_
#define _HPLC_SEND_H_


#define CSMA_ALL_PHASE          (1<<0)      //全相线发送标志

#define CSMA_A_PHASE            (1<<1)      //A相发送
#define CSMA_B_PHASE            (1<<2)      //B相发送
#define CSMA_C_PHASE            (1<<3)      //C相发送

#define CSMA_NOT_WAIT_BEACON    (1<<4)      //发送不出去直接舍弃，不等下一个信标

#define CSMA_SEND_AT_NEXT_NTB   (1<<5)      //最后发送时ntb使用的是下一时刻的ntb 此选项用于在绑定csma发送探测信标
#define CSMA_SEND_SYNC_NEXT_NTB (1<<6)      //最后发送时ntb使用的是下一时刻的ntb 此项用于发送同步帧
#define CSMA_SEND_CONCERT_NTB   (1<<7)
#define CSMA_SEND_NEED_ONE_HOP  (1<<8)      //此帧在HRF发送时需要转化成单跳帧



#define CSMA_QUEUE_SIZE         10          //CSMA发送队列最大数

#define DEFAULT_MAX_RESENDTIMES  3          //默认重发次数

#define MAX_BEACON_NTB          (15*25000*1000ul)
#define HPLC_WAIT_CPU_NTB        350

typedef enum
{
    SEND_SOF_OK,        //发出的SOF帧正确的收到确认帧
    SEND_SOF_FAIL,       //发出的SOF帧经过重发依然不能正确收到确认帧
    SEND_SOF_SLOT_ERR,  //发出的SOF帧因为竞争不到时隙，发送失败
    SEND_SOF_BROADCAST, //发送广播完成
	SEND_SOF_NOT_BUF,	//无法分配到发送BUFF
	SEND_SOF_ABNORMAL,	//异常清空
	SEND_SOF_NOACK
}SEND_SOF_STATE;

//发送完成SOF帧的消息
#pragma pack(1)
typedef struct
{
	u16 SendState;
	u16 TEI;
}SEND_SOF_FINISH_STATE;
#pragma pack(4)

//SOF帧回调
typedef void(*pSendSofCallback)(SEND_SOF_STATE,SEND_PDU_BLOCK*,u16);

//初始化
void HPLC_SendTask_Init(void);

//CSMA发送
bool CSMA_SendData(u8 no,SEND_PDU_BLOCK* mpdu,u16 TEI,u8 lid,u32 opt,u8 resendTimes,pSendSofCallback fun);

//信标发送
bool Beacon_SendData(u8 frome,SEND_PDU_BLOCK* mpdu,u16 realLen, u32 beginNTB,u32 endNTB,pSendMacCallback callback);

//确认帧发送
bool Confirm_SendData(u8 frome,SEND_PDU_BLOCK* mpdu);

//接收确认帧
bool Confirm_ReceiveData(PDU_BLOCK* mpdu);

//更新信标帧
bool UpdataBeacon(void);

//获取CSMA发送队列长度
u8 GetCSMA_ListNum(void);

//任务
void HPLC_SendTask(void* arg);
void HPLC_CSMA_SendTask(void* arg);
void HPLC_BCSMA_SendTask(void* arg);
void HRF_CSMA_SendTask(void* arg);
void HRF_BCSMA_SendTask(void* arg);
bool GetNextSendSlot(CSMA_SLOT_INFO *CSMA_Slot, u32 *begin, u32 *end, u32 opt, u8 lid, u16 FrameLen);
#define READER_CONNECT_TICK  (OS_CFG_TICK_RATE_HZ * 60 *3)

extern SLOT_INFO SendSlotInfo;
extern SLOT_INFO SlotInfo;
void LockSolt(void);
void UnLockSolt(void);
void InitSlotMutex(void);


#define BEACON_NTB_TIME_NUM         (0)
#define BEACON_HRF_NTB_TIME_NUM     (1)

#endif

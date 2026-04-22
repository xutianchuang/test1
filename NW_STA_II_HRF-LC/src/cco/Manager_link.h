#ifndef _Manager_Link_
#define _Manager_Link_

#pragma pack(push,4)
typedef enum
{
	//HPLC_send
	MSG_HPLC_RECEIVE_BEACON_FC,
	MSG_HPLC_RECEIVE_MPDU,
	MSG_HPLC_RECEIVE_ACK,
	MSG_HPLC_SEND_ACK,
	MSG_HPLC_SEND_SOF,
	MSG_HPLC_SEND_CONCERT,
	MSG_HPLC_CHANGE_FREQUENCE_IMMEDIATE,
	MSG_HPLC_CHANGE_FREQUENCE_IMMEDIATE_NOTSAVE,
	MSG_HPLC_CHANGE_TONEMASK_IMMEDIATE,
	MSG_HPLC_CHANGE_POWER_IMMEDIATE,
	MSG_HPLC_CHANGE_HRF_CHANNEL_IMMEDIATE,
	MSG_HPLC_CHANGE_HRF_CHANNEL_IMMEDIATE_NOTSAVE,
	MSG_HPLC_CHANGE_HRF_MCS_PBSIZE_IMMEDIATE_NOTSAVE,

	MSG_PLM_FRAME,
	MSG_PLM_SET_BAUDRATE,
	MSG_ONE_HOP_FRAME,
	MSG_MAC_FRAME,
	MSG_DIR_READ,           //点抄
	MSG_PARALLEL_READ,      //并发抄表
	MSG_ON_PLM_CONFIRM,     //AFN00 F1
	MSG_ON_PLM_READ_ITEM,   //AFN14 F1 response
	MSG_BROADCAST,
	MSG_PHASE_ERR,         //零线电流错误是能开关
	MSG_HPLC_COMM_TEST,

	//格式化
	MSG_PARAM_CLEAR,
	MSG_DATA_CLEAR,
	MSG_TASK_CLEAR,
	//切换频段
//    MSG_PLC_CHANGE_FREQUENCE,

	//主动注册
	MSG_START_SELF_REG,
	MSG_STOP_SELF_REG,

	//STA 升级
	MSG_START_UPDATE_STA,
	MSG_STOP_UPDATE_STA,

	//台区识别
	MSG_START_AREA_DISCERNMENT,
	MSG_STOP_AREA_DISCERNMENT,

	//南网STA采集任务
	MSG_STA_READ_TASK_INIT,

	//存储曲线数据广播校时
	MSG_STORE_DATA_BCAST_TIME,
#ifdef GDW_ZHEJIANG
	MSG_ONLINE_TIME_LOCK,

	MSG_EVENT_COLLECT_REPORT,
#endif
	//曲线存储数据项采集方案同步
	MSG_STORE_DATA_SYNC_CFG,

	//从节点内部版本信息
	MSG_INNER_VER_READ,

	//从节点重启
	MSG_RESET_STA
}TMESSAGE;

typedef uint32_t WPARAM;
typedef uint32_t LPARAM;



typedef struct
{
	TMESSAGE    message;
	WPARAM      wParam;
	LPARAM      lParam;
}TMSG, *P_TMSG;


//系统资源
typedef struct
{
	OS_SEM savedata_mbox;     //保存数据时与主站同步
	OS_SEM sem_wait_send_complete;
	OS_SEM sem_wait_debug_send_done;
	OS_Q   q_up_rx_plc;     //14 F1 命令

	OS_Q   q_ack[2];  //ack 发送队列
#if defined(PROTOCOL_GD_2016)
	OS_Q   read_meter_task_q;     //南网抄表任务队列
#endif

	OS_TCB      App_Bplc_Timer_TCB;
	OS_TCB      App_Bplc_csmaSend_TCB[2];
	OS_TCB      App_Bplc_BcsmaSend_TCB[2];
	OS_TCB      App_Bplc_Send_TCB;
	OS_TCB      App_Bplc_Rec_TCB;
	OS_TCB      APP_PLC_Autorelay_TCB;
	OS_TCB      APP_End_TCB;
	OS_TCB      APP_PLM_2005_TCB;

	//OS_TCB      APP_Sync_Ntb_TCB;

	CPU_STK     App_BplcSendStk[APP_CFG_TASK_BPLC_SEND_STK_SIZE];
	CPU_STK     App_BplcCsmaSendStk[2][APP_CFG_TASK_BPLC_CSMA_SEND_STK_SIZE];
	CPU_STK     App_BplcBcsmaSendStk[2][APP_CFG_TASK_BPLC_CSMA_SEND_STK_SIZE];

	CPU_STK     App_BplcRecStk[APP_CFG_TASK_BPLC_REC_STK_SIZE];
	CPU_STK     App_BplcTimerStk[APP_CFG_TASK_BPLC_TIMER_STK_SIZE];

	CPU_STK     APP_PLC_Autorelay_stk[APP_PLC_AUTORELAY_STK_SIZE];


	CPU_STK     APP_End_stk[APP_END_STK_SIZE];
	//CPU_STK     APP_Sync_Ntb_stk[APP_SYNC_NTB_STK_SIZE];


	CPU_STK     APP_PLM_2005_stk[APP_PLM_2005_STK_SIZE];



	//OS_TMR      millsecond_timer;





}OS_RES, *P_OS_RES;

#pragma pack(pop)


#define SRAM_BASE_ADDR				(0x40000)  	//bytes

//#define SRAM_MAX_SIZE				(unsigned long)K*256*2		//max possible SRAM in bytes
#define SRAM_MIN_SIZE				(unsigned long)K*512		//min possible SRAM in bytes

// 原来K*2 * 32  只够32块表的数据区, 为了保存其它必要变量, 需要扩大.
//#define MIRROR_TO_FRAM_MEMSIZE	(unsigned long)K*32*2	/* 将来需要掉电时保存到FRAM 的内存HEAP  大小*/
//#define MIRROR_TO_FRAM_MEMSIZE	(unsigned long)K*32*4	/* 将来需要掉电时保存到FRAM 的内存HEAP  大小暂时定义为128k */
#define MIRROR_TO_FRAM_MEMSIZE	(unsigned long)K*8	  	/* 为了匹配FRAM 实际大小，配置为8k 即可*/

#define	CACHE_LINE_BYTES			0x20	//32bits - WORD

#define 	CACHE_ROUND(_v)	(((U32)(_v)+CACHE_LINE_BYTES-1)&~((U32)CACHE_LINE_BYTES-1))
//#define 	ROUND_UP(_v, b)	(((U32)(_v)+b-1)&~(b-1))
#define 	ROUND_UP(_v, b)	 (((U32)(_v) + b-1) &~((U32)(b)-1))



#define MAX_MAIL_NUM  30

#define MAX_MSGITEM_LONG  16	/* 接收msg pool 大小*/
//#define MAX_MSGITEM_LONG_TX  8	/* 发送msg pool 大小*/

#define MAX_MSGITEM_SHORT  60	/* 接收msg pool 大小*/
//#define DOWN_MSGITEM_TX  30	/* 发送msg pool 大小*/

#define MAX_QUEUE_ITEM  10



#define END_OBJECT_NUMBER		COM_PORT_MAX

//
//#define COM_PORT_CONNECTHOST 		COM_PORT_MAINTAIN
//#define COM_PORT_CONNECTHOST 		COM_PORT_INTERNET

#ifdef PLC_CASCADE_MODULE
#define COM_PORT_CONNECTHOST 		( CCTT_cascade_isWho(CAS_SLAVE) )? (COM_PORT_LOWBOARD):(COM_PORT_INTERNET)
#else
#define COM_PORT_CONNECTHOST     COM_PORT_INTERNET
#endif
//#else
//#define COM_PORT_CONNECTHOST 	COM_PORT_INTERNET
//#endif

typedef struct tagDEVICE_DESC
{
	unsigned long		SRamSize;                           // SRAM size in bytes

	unsigned long		GlobalHeapSize;                     // Cached global heap size
	unsigned long		GlobalHeap1Size;                        // Cached global heap size
	unsigned long		GlobalHeap2Size;                        // Cached global heap size
	unsigned long		GlobalHeap3Size;                        // Cached global heap size
	//unsigned long		GlobaltoFramHeapSize;						// Cached global heap size

#if 0
	unsigned long		MsgPoolSize;

	void*			pExRAMStart;
	void*			pManagerQueueStart;
	void*			pMailQueueStart;

	void*			pLeftMemTail;   //剩余内存的开始地址.
#endif

	unsigned long 		NumberOfLostDl645OutFrame;
	unsigned long 		NumberOfLostPLCOutFrame;

	unsigned long 		NumberOfLostIICOutFrame;
	unsigned long 		NumberOfLostGprsRespFrame;

	unsigned long 		NumberOfLostCcttRespFrame;
	unsigned long 		NumberOfLostCascadeFrame;

	unsigned long 		NumberOfLostPrintOutFrame;
	unsigned long 		NumberOfLostPLCQueryFrame;
} DEVICE_DESC, *PDEVICE_DESC;




typedef struct _dev_request_
{
	struct _dev_request_ *next_req;
	struct _dev_request_ *pre_req;

	unsigned char state;        /*IDLE, ALLOC*/
	unsigned char end_id;       /* 0~5 */
	unsigned short reserved;

	// 这里保存消息指针地址, 通过它找到*msg 存放地址.
	unsigned long *pdata;

}DEV_REQUEST, *P_DEV_REQUEST;


typedef struct tag_fast_queue_
{
	unsigned long *storage;

	unsigned short get;
	unsigned short put;

	unsigned short size;
	unsigned short sema;
	//spinlock_t lock;
} FAST_QUEUE, *PFAST_QUEUE;

typedef struct tagRINGBUF
{
	UCHAR *Storage;
	ULONG ReadIndex;
	ULONG WriteIndex;
	ULONG Size;
	//spinlock_t Lock;
} RINGBUF, *PRINGBUF;


//#pragma pack(pop)


typedef enum
{

	MSG_SHORT = 0,
	MSG_LONG,
	MSG_BUTT

}MSG_TTYPE;


//extern FAST_QUEUE MailQueue;

// 底层Dev 与资源管理层数据消息buffer.
//extern P_LONG_MSG_INFO pGlobalMSGRxPool;

extern P_LONG_MSG_INFO 	pLongMsgPool[];
extern P_SHORT_MSG_INFO 	pShortMsgPool[];

//extern P_LONG_MSG_INFO pGlobalMSGTxPool;
//extern P_LONG_MSG_INFO MsgTxPool[];

// 底层Dev 与资源管理层数据消息buffer.
// 模块间消息交互队列

void SYS_PROTOCOL_proc();

void defqueue(FAST_QUEUE *fpq, U32 *storage, U16 size, U16 sema);

PBYTE GetComPortBufPtr(UCHAR end_id, UCHAR sub_id);
USHORT* GetComPortLenPtr(UCHAR end_id, UCHAR sub_id);

void ManagerLinkInit();

//////////////////////////////////////////////////
//unsigned char free_send_buffer(pvoid pmsg);
P_MSG_INFO alloc_send_buffer(const char *func_name, MSG_TTYPE type);
P_MSG_INFO alloc_send_buffer_by_len(const char *func_name, USHORT len);
unsigned char free_send_buffer(const char *func_name, pvoid pmsg);

P_TMSG alloc_msg(const char *func_name);
void   free_msg(const char *func_name, P_TMSG pMsg);

PBYTE alloc_buffer(const char *func_name, USHORT len);
void free_buffer(const char *func_name, PVOID pBuffer);

//#define GET_MAX_MSGITEM(type)		(MAX_MSGITEM_COUNT[type])

extern P_OS_RES      g_pOsRes;
extern PDEVICE_DESC pDevicedesc;

#endif


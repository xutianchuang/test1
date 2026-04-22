
#ifndef _Uart_Link_
#define _Uart_Link_

//#pragma pack(push,1)

#define END_TX_QUEUE_SIZE		50

#define END_NON_MODULE_USED 	0xff

#define END_STATUS_IDLE     	0
#define END_STATUS_SENDING 		1
#define END_STATUS_RECEIVING    2

//#define END_STATUS_MUX			2  //双工
//#define END_STATUS_MIRROR       3  //镜像

#define END_STATUS_SPI_R	    4  //当前为SPI口读传输状态
#define END_STATUS_SPI_W	    5  //当前为SPI口写传输状态

#define END_STATUS_INVALID 		6
#define END_STATUS_ERROR 		7

//
#define END_STATUS_SEND_DONE 		8
#define END_STATUS_SEND_TIMEOUT 		9
#define END_STATUS_RECV_DONE 		10
#define END_STATUS_RECV_TIMEOUT 		11

#define END_DEBUG 				1

//alan test
#define UART2_BUFFER_SIZE 	256

#define END_FAST_PROETCT_CYCLE	(unsigned char)1 	/* 1 s */
#define END_SLOW_PROETCT_CYCLE	(unsigned char)5 	/* 4 s */
#define END_SLOW_PROETCT_CYCLE2	(unsigned char)5 	/* 5 s */

#define RX_INTERNET_BUFFER_SIZE         4800

typedef MD_STATUS (* END_RECV_PTR)(UCHAR* txbuf, USHORT txnum);
typedef MD_STATUS (* END_SEND_PTR)(UCHAR* txbuf, USHORT txnum);
//typedef MD_STATUS (* END_WRITE_PTR)(UCHAR* wBuf, USHORT wNum,USHORT wAddr);
typedef MD_STATUS (* END_UPPER_CALL_BACK)();
typedef MD_STATUS (* END_TIMEOUT_CALL_BACK)(pvoid h);
typedef void (* END_START_PTR)(void);
typedef void (* END_STOP_PTR)(void);
typedef void (* END_SWITCH_STATUE_PTR)(unsigned char new_status);


typedef struct _end_object_static_
{
	unsigned long rxPacketCount;
	unsigned long txPacketCount;
}END_STAT, *P_END_STAT;

typedef struct _end_object_
{
	unsigned char end_id;
    unsigned char end_send_status;	
	
	unsigned short receive_len;			//当前收包长度
	unsigned short last_receive_len;    //上次收包长度
	P_MSG_INFO end_recv_buffer;
	
	END_START_PTR Start;                //开始工作
	END_STOP_PTR Stop;                //end 复位
	END_SEND_PTR Send;	

	END_RECV_PTR RecvData;                //end 复位
	//END_UPPER_CALL_BACK UpperCallBack;
	//END_SWITCH_STATUE_PTR SwitchStatus;  //对于半双工端口，切换端口工作状态
	END_STAT endStatistics;
	
	P_MSG_INFO pMsgInfo;   //当前正在处理的消息报文

    unsigned char recv_timeout;
    unsigned char send_interval;
	
}END_OBJ, *P_END_OBJ;

//#pragma pack(pop)

P_END_OBJ End_get_end_obj(UCHAR end_id);
void End_init(void);

void End_SetBaudrate(u32 bps, u32 Parity);

//unsigned char End_Register(unsigned char  device, unsigned char  module_id, CALLBACK  cb_routine);

unsigned char End_OnTick(void* pTimerHandle, void* HANDLE);

unsigned char End_set_expire(UCHAR end_id );
unsigned char End_get_status(UCHAR end_id);
unsigned char End_set_status(UCHAR end_id, unsigned char state );
unsigned short End_send(P_MSG_INFO pMsgInfo);
unsigned short End_post( P_MSG_INFO pMsgInfo);
unsigned char End_check_recv(P_END_OBJ pEndObj);
unsigned char End_check_send(UCHAR end_id);
unsigned char End_IsIdle(P_END_OBJ pEndObj);

unsigned char End_postProcess(unsigned char end_type,  pvoid h);
void END_Set_Upper_Protocol(UPPER_PROTOCOL_PROC up_proc);


extern U32 EndTxQueueMem[COM_PORT_MAX][END_TX_QUEUE_SIZE+2];
extern U32 EndRxQueueMem[COM_PORT_MAX][END_TX_QUEUE_SIZE+2];
//extern unsigned char internet_msg_buf[RX_INTERNET_BUFFER_SIZE+sizeof(MSG_HEADER)];

PBYTE GetComPortBufPtr(UCHAR end_id, UCHAR sub_id);
USHORT* GetComPortLenPtr(UCHAR end_id, UCHAR sub_id);

void DebugGetMsg(u8 *data,u16 len);
void DebugSendComplate(int);

#define END_TASK_MSG_NUM        50
void  End_Task_Proc(void *p_arg);
void ClearWaitAckMsg(u8 seq);
void ReaderGetMsg(u8 *data,u16 len);
#ifdef HPLC_CSG
void End_ChangeRateSend(u8 *data,u16 len,u32 rate);
#endif
#endif


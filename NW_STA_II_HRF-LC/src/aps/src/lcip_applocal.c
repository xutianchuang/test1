// #include  <cpu.h>
#include  <os.h>
#include  <bsp.h>
#include <gsmcuxx_hal_def.h>

#include "timer.h"
// #include  <bsp_os.h>
#include "bps_uart_lcip.h"
#include "crclib.h"
#include  <string.h>
#include <stdlib.h>
#include "lc_whi_pro.h"
#include "ble_local.h"
#include "hplc_mac.h"
#include "ble_api.h"

#define UART_TIME_OUT (0x02)
#define LOCAL_UART_TASK_LENGTH (16)

// 接收缓存
#define LCIP_MAX_LOCAL_LEN 1024
#define DEFAULT_WAIT_FRAME_TIME 500 										// 默认等待帧返回
#define LCIP_UART_RECEIVE_WATCH_DOG_CHECK_TIME_MS (10 * 60 * 1000UL)
#define SEND_LOCAL_ACK_PRIO_VALUE (0x50) 									// 立马应答
#define UART_SUCCESS (0x01)

// 波特率
#define LOCAL_UART_RATE_VALUE (256000)
#define AbeforB(A, B) (((B) - (A)) < ((A) - (B))) 							// a,b 必需是u32 ntb判断函数，是否A在B之前；A,B无符号型，且A,B之间绝对距离不超过类型最大值的一半；

#define LCIP_DATA_LEN (16 * 17)
#define LCIP_STRUCT_LEN (sizeof(lcip_struct_t) + sizeof(uint16_t))
#define LCIP_MAX_LEN (LCIP_DATA_LEN + LCIP_STRUCT_LEN)
#define LOCAL_UART_SEND_MAX_TIME_OUT_VALUE (200) //20秒 //(50) 							// 5秒

#define LCIP_HEAD 0x48

#define LCIP_SUCC 0
#define LCIP_FAIL 1

#define BIT(n) (1 << (n))
typedef enum
{
    LCIP_LOCAL_IDLE,    //空闲
    LCIP_LOCAL_RECEIVE, //正在接收
} LCIP_LOCAL_UART_STATE;

typedef struct
{
    void *exparam;
    uint32_t Urate;
	local_task_cb_fun_t cb_fun;
    uint8_t UChb;
	uint8_t UDBits;
	uint8_t USBits;
	uint8_t UTxPin;
	
    uint16_t Prio;
    uint16_t Tout;
    uint16_t Size;        //帧长
    uint16_t Stype;
    uint16_t SendTimes;
    uint16_t LSn;//must
	uint16_t FrameSeq;	//帧序号，回调根据帧序号进行判断
	uint16_t reserve;
	TimeValue LocalUartReceiveTimer; //定时器

    //以下的数据要4字节对齐，否则在LC830上有问题
	uint8_t Data[];       //数据
}local_task_param_t;

typedef struct 
{
    uint8_t head;
    uint8_t ctrl;
    uint16_t cmd;
    uint16_t seq;
    uint16_t length;
    uint8_t data[0];
}lcip_struct_t;

//接收数据信号量
typedef struct
{
	uint32_t frame_len;
	uint8_t frame_data[];
}whi_local_receive_frame_type;

// 本地串口参数
static UartParameterType LcipUsartPara;
// 本地接收数据队列
static OS_Q LcipLocalDataQueue;

local_task_param_t *LocalUartTaskParamPointList[LOCAL_UART_TASK_LENGTH];

static uint8_t LocalUartTaskParamPointListLen = 0;
static uint8_t CurrentLoaclUartTaskIndex = 0;

// 接收数据信号量
static OS_SEM LcipReceiveComplateSEM;
static u8 LcipReceiveBuf[LCIP_MAX_LOCAL_LEN] = {0};
static volatile u8 UsartSendComplateFlg = false;
static TimeValue LcipUartReceiveWatchDogTimer;
static uint16_t lsn_cnt = 0;

static void LCIP_BSP_OS_TimeDly(CPU_INT32U dly_tick)
{
	OS_ERR err;
	OSTimeDly(dly_tick, OS_OPT_TIME_DLY, &err);

	(void)&err;
}

//初始化本地串口任务
void InitLocalUartTask()
{
	CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();

    for (int i = 0; i < LOCAL_UART_TASK_LENGTH; i++)
    {
        LocalUartTaskParamPointList[i] = NULL;
    }
    LocalUartTaskParamPointListLen = 0;

	OS_CRITICAL_EXIT();
}

//添加一个本地串口任务
bool AddLoackUartTask(local_task_param_t *ppar)
{
    static u32 AppLocalSnCnt = 0;
    bool rflg = false;
    if (ppar == NULL)
	{
        return false;
	}

	// 进入临界区
	CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();

    ppar->LSn = AppLocalSnCnt++;

    if (LocalUartTaskParamPointListLen < LOCAL_UART_TASK_LENGTH)
    {
        for (int i = 0; i < LOCAL_UART_TASK_LENGTH; i++)
        {
            if (LocalUartTaskParamPointList[i] == NULL)
            {
                LocalUartTaskParamPointList[i] = ppar;
                ++LocalUartTaskParamPointListLen;
                rflg = true;
                break;
            }
        }
    }
    
	// 退出临界区
	OS_CRITICAL_EXIT();
    return rflg;
}

bool local_send_fram_add_task(uint8_t *data,uint16_t len,uint16_t pri,uint16_t t_time,uint16_t resend,local_task_cb_fun_t cb_fun,void *cb_fun_arg)
{
    local_task_param_t* local_task = NULL;

    if((data != NULL) && (len <= LCIP_MAX_LOCAL_LEN) && (len > 0))
	{
		local_task = (local_task_param_t*)malloc(len + sizeof(local_task_param_t) + 32);
		if(local_task != NULL)
		{
			local_task->Prio = pri;
			memcpy(local_task->Data,data,len);
			local_task->Size = len;
			local_task->Urate = LcipUsartPara.BaudRate;
			local_task->Stype = SEND_LOCAL_ACK;
			local_task->Tout = t_time > LOCAL_UART_SEND_MAX_TIME_OUT_VALUE ? LOCAL_UART_SEND_MAX_TIME_OUT_VALUE : t_time;
			local_task->SendTimes = resend+1;
			local_task->LSn = lsn_cnt++;
			if(WHI_IsLocalWHIFrame(data,len))
			{
				local_task->FrameSeq = (((uint16_t)data[5])<<8)|data[4];
			}
			//首次立即发送
			StartTimer(&local_task->LocalUartReceiveTimer, 0);
			local_task->cb_fun = cb_fun;
			local_task->exparam = cb_fun_arg;

			if(false == AddLoackUartTask(local_task))
			{
			    free(local_task);
				return false;
			}

			return true;
		}
	}

    return false;
}

//删除本地串口任务
bool DeleteLoackUartTask(u8 taskindex)
{
    bool rflg = false;
    if (taskindex >= LOCAL_UART_TASK_LENGTH)
        return false;
    if (LocalUartTaskParamPointList[taskindex] == NULL)
        return false;

	// 进入临界区
	CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
	if(LocalUartTaskParamPointList[taskindex]->exparam != NULL)
	{
		free(LocalUartTaskParamPointList[taskindex]->exparam);
		LocalUartTaskParamPointList[taskindex]->exparam = NULL;
	}
	free(LocalUartTaskParamPointList[taskindex]);

    LocalUartTaskParamPointList[taskindex] = NULL;
    LocalUartTaskParamPointListLen--;
    rflg = true;

	// 退出临界区
	OS_CRITICAL_EXIT();

    return rflg;
}


// 返回串口使用的波特率
u32 LcipGetLoaclUartBaud(void)
{
	return LcipUsartPara.BaudRate;
}

// 发送完成通知
static void UsartSendComplate(int arg_null)
{
	LCIP_UART.Control(ARM_USART_CONTROL_RX, 1);
}

// 接收完成通知
static void UsartReceiveComplate(int len)
{
	OS_ERR err;
	OSSemPost(&LcipReceiveComplateSEM, OS_OPT_POST_ALL, &err);
}

static void UsartReceiveErr(int arg)
{
	;
}

static void ChangeLocalUartBaud(u32 baud)
{
	LcipUartClose();

    LcipUsartPara.BaudRate = baud;
    //    LcipUsartPara.DataBits = 8;
    LcipUsartPara.Parity = Parity_Even;
    //    LcipUsartPara.StopBits = Stop_Bits_One;
    LcipUsartPara.RxHaveDataFunction = UsartReceiveComplate;
    LcipUsartPara.TxDataCompleteFunction = UsartSendComplate;
    LcipUsartPara.RxParityErrorFunction = UsartReceiveErr;

    LcipUartOpen(&LcipUsartPara);
}

extern bool TestMode;

void LcipLocalInit(void)
{
	OS_ERR err;

	// 串口初始化
	if (false == LcipIsUartOpen())
	{
		LcipUsartPara.BaudRate = LOCAL_UART_RATE_VALUE;
		//    LcipUsartPara.DataBits = 8;
		LcipUsartPara.Parity = Parity_Even;
		//    LcipUsartPara.StopBits = Stop_Bits_One;
		LcipUsartPara.RxHaveDataFunction = UsartReceiveComplate;
		LcipUsartPara.TxDataCompleteFunction = UsartSendComplate;
		LcipUsartPara.RxParityErrorFunction = UsartReceiveErr;

		LcipUartOpen(&LcipUsartPara);
	}

	// 创建信号量
	OSSemCreate(&LcipReceiveComplateSEM, "LcipReceiveSEM", 0, &err);
	if (err != OS_ERR_NONE)
		while (1)
			;

	// 创建消息队列
	OSQCreate(&LcipLocalDataQueue, "LcipLocalDataQueue", 16, &err);
	if (err != OS_ERR_NONE)
		while (1)
			;
}
//#define TEST_PRINTF

void LcipAppLocalSendHandle(void)
{
	OS_ERR err;
	if (LocalUartTaskParamPointListLen > 0)
	{
		u8 maxPrio = 0;
        u32 selectsn = 0;
        bool fflg = false;     
		//寻找需要发送的串口任务
        for (int i = 0; i < LOCAL_UART_TASK_LENGTH; i++)
        {
        	//发送次数大于0，并且定时器超时
            if ((LocalUartTaskParamPointList[i] != NULL) && (LocalUartTaskParamPointList[i]->SendTimes > 0))
            {
            	//定时器超时需要最后判断，避免定时器失效！！！
            	if(true == LcTimeOut(&LocalUartTaskParamPointList[i]->LocalUartReceiveTimer))
            	{
            		//注意要重置定时器，不然下次查找已经失效！！！
            		StartTimer(&LocalUartTaskParamPointList[i]->LocalUartReceiveTimer, 0);
            		if ((fflg == false)||(LocalUartTaskParamPointList[i]->Prio > maxPrio)
	                ||((LocalUartTaskParamPointList[i]->Prio == maxPrio)&&(AbeforB(LocalUartTaskParamPointList[i]->LSn,selectsn))))
	                {
	                    maxPrio = LocalUartTaskParamPointList[i]->Prio;
	                    CurrentLoaclUartTaskIndex = i;
	                    fflg = true;
	                    selectsn = LocalUartTaskParamPointList[i]->LSn;
	                }
            	}
            }
        }

		if (fflg == true)
		{
			local_task_param_t *current_task = LocalUartTaskParamPointList[CurrentLoaclUartTaskIndex];
			LcipUartWrite(current_task->Data, current_task->Size);
		#ifdef TEST_PRINTF
			char buf[300] = {0};
			for(int i=0;i<50&&i<current_task->Size;i++)
			{
				sprintf(&buf[i*3],"%02x ",current_task->Data[i]);
			}
			debug_str(DEBUG_LOG_UPDATA, "sbuf:%s\r\n",buf);
		#endif
			while (LcipIsUartTxData())
			{
				LCIP_BSP_OS_TimeDly(1);
			}
			while (!LcipIsUartTxSendAllDone())
			{
				OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
			}

			UsartSendComplateFlg = false;
			current_task->SendTimes--;
			StartTimer(&current_task->LocalUartReceiveTimer, current_task->Tout * 100);
			//
			u32 rate= LcipUsartPara.BaudRate;
			u32 waitedelaytimems = (current_task->Size * 11 * 1000 + (rate/1000))/rate + 10;
			u16 delaycnt = 0;
			while (UsartSendComplateFlg == false)
			{
				OSTimeDly(1, OS_OPT_TIME_DLY, &err);
				delaycnt++;
				if(delaycnt >= waitedelaytimems){
					//local_debug_cnt.uart_tx_brocast_time_out_cnt++;
					break;
				}
			}

			uint32_t delay_time = ((11*8*1000)/rate);
			if(delay_time < 2)
			{
				delay_time = 2;
			}
			OSTimeDly(delay_time, OS_OPT_TIME_DLY, &err);
			if (current_task->Tout == 0)
			{	
				if(current_task->cb_fun != NULL)
				{
					current_task->cb_fun(current_task->exparam,NULL,0,UART_SUCCESS);
				}
				DeleteLoackUartTask(CurrentLoaclUartTaskIndex);
			}
		}
	}
}

void LcipAppLocalRecvHandle(void)
{
      OS_ERR err;
	whi_local_receive_frame_type *pdu = NULL;
	OS_MSG_SIZE msgSize;
	// 消息队列获取消息
	pdu = OSQPend(&LcipLocalDataQueue, 2, OS_OPT_PEND_BLOCKING, &msgSize, 0, &err);

    if(err == OS_ERR_NONE)
    {
		u32 len = pdu->frame_len >= LCIP_MAX_LOCAL_LEN ? LCIP_MAX_LOCAL_LEN : pdu->frame_len;
		memcpy(&LcipReceiveBuf, pdu->frame_data, len);
		#ifdef TEST_PRINTF
		char buf[300] = {0};
		for(int i=0;i<50&&i<len;i++)
		{
			sprintf(&buf[i*3],"%02x ",LcipReceiveBuf[i]);
		}
		debug_str(DEBUG_LOG_UPDATA, "rbuf:%s\r\n",buf);
		#endif
		free(pdu);
		uint16_t local_send_len = 0;
		uint16_t dlen = (((uint32_t)LcipReceiveBuf[7])<<8)|LcipReceiveBuf[6];
		dlen += 10;

		if (WHI_IsLocalWHIFrame(LcipReceiveBuf, len))
		{
			uint16_t FrameSeq = (((uint16_t)LcipReceiveBuf[5])<<8)|LcipReceiveBuf[4];
			StartTimer(&LcipUartReceiveWatchDogTimer, LCIP_UART_RECEIVE_WATCH_DOG_CHECK_TIME_MS);
			if (((LcipReceiveBuf[1] & 0x80) == 0x00) && ((LcipReceiveBuf[1] & 0x40) == 0x40))
			{ 
				// 主动
				uint16_t framecmd = (((uint16_t)LcipReceiveBuf[3]) << 8) | LcipReceiveBuf[2];
				if (framecmd == WHI_SEND_BUS_FRAME)
				{
					ble_net_ack_send_data_port = BLE_NET_ACK_SEND_BLE_UART_PORT;
					StartTimer(&ble_net_ack_send_data_port_timer, ACK_SEND_PORT_CHECK_TIME); 
				}
			}
			//应答,从动站
			if((LcipReceiveBuf[1]&0x40) == 0x00)
			{
				for (int i = 0; i < LOCAL_UART_TASK_LENGTH; i++)
		        {
		        	if(LocalUartTaskParamPointList[i] != NULL)
		        	{
		        		//发送次数大于0，并且定时器超时
			            if (LocalUartTaskParamPointList[i]->FrameSeq == FrameSeq)
			            {
			                if(LocalUartTaskParamPointList[i]->cb_fun != NULL)
							{
								LocalUartTaskParamPointList[i]->cb_fun(LocalUartTaskParamPointList[i]->exparam,LcipReceiveBuf,len,UART_SUCCESS);
							}
							DeleteLoackUartTask(i);
							return ;
			            }
		        	}
		        }
			}

			if ((local_send_len = WHI_UartLocalWHIProcess(LcipReceiveBuf, dlen, LOCAL_PROCESS_MODE)) != 0)
			{
				local_send_fram_add_task(LcipReceiveBuf, local_send_len, SEND_LOCAL_ACK_PRIO_VALUE, 0, 0, NULL, NULL);
			}
		}
	}
}

void LcipAppLocalTimeoutHandle(void)
{
	if (LocalUartTaskParamPointListLen > 0)
	{ 
		//寻找已经超时的串口任务
		for (int i = 0; i < LOCAL_UART_TASK_LENGTH; i++)
		{
			if ((LocalUartTaskParamPointList[i] != NULL) && (LocalUartTaskParamPointList[i]->SendTimes == 0))
			{
				//定时器超时需要最后判断，避免定时器失效！！！
				if (true == LcTimeOut(&LocalUartTaskParamPointList[i]->LocalUartReceiveTimer))
				{
					//超时传入数据应该为空
					if(LocalUartTaskParamPointList[i]->cb_fun != NULL)
					{
						LocalUartTaskParamPointList[i]->cb_fun(LocalUartTaskParamPointList[i]->exparam,NULL,0,UART_TIME_OUT);
					}
		            DeleteLoackUartTask(i);
				}
			}
		}
	}
}

void LcipAppLocalHandle(void)
{
    //串口任务发送
   LcipAppLocalSendHandle();

	//串口任务接收处理
	LcipAppLocalRecvHandle();

	//串口任务超时处理
    LcipAppLocalTimeoutHandle();
	
	if(TRUE == LcTimeOut(&LcipUartReceiveWatchDogTimer))
	{
		ChangeLocalUartBaud(LOCAL_UART_RATE_VALUE);
		StartTimer(&LcipUartReceiveWatchDogTimer,LCIP_UART_RECEIVE_WATCH_DOG_CHECK_TIME_MS);
	}
}

int lcip_protocol_crc(lcip_struct_t *pl)
{
    uint16_t recv_crc;

    recv_crc = pl->data[pl->length] & 0xFF;
    recv_crc |= (pl->data[pl->length + 1] << 8);

    if (crc16_cal((u8 *)pl, (pl->length + sizeof(lcip_struct_t))) == recv_crc || recv_crc == 0xCCCC )
    {
        return true;
    }
    else
    {
        return false;
    }
}

void lcip_protocol_parsing(void)
{
    u32 recv_len;
    u32 process_len;

    lcip_struct_t head;
    whi_local_receive_frame_type *pl;
	OS_ERR err;

    recv_len = stream_buffer_recv_len(&uart_rxstream);
    if (recv_len < LCIP_STRUCT_LEN)
    {
        return;
    }

    while (recv_len >= LCIP_STRUCT_LEN)
    {
        process_len = 1;
        stream_buffer_copy(&uart_rxstream, (u8 *)&head, uart_rxstream.read_point, sizeof(lcip_struct_t));

        if (head.head == LCIP_HEAD && head.length < LCIP_MAX_LEN)
        {
            process_len = LCIP_STRUCT_LEN + head.length;

			// LCIP帧长度错误，非LCIP帧丢弃
            if (recv_len < process_len)
            {
                return;
            }
			
            pl = (whi_local_receive_frame_type*)malloc( (process_len + 4) );
            if (NULL == pl)
            {
                return;
            }
			pl->frame_len = process_len;

            stream_buffer_copy(&uart_rxstream, (u8 *)pl + 4, uart_rxstream.read_point, process_len);
            
			if (lcip_protocol_crc( (lcip_struct_t*)(pl->frame_data) ))
            {
                OSQPost(&LcipLocalDataQueue, pl, process_len + 4, OS_OPT_POST_FIFO | OS_OPT_POST_ALL, &err);

				if (err != OS_ERR_NONE)
				{
					// 因消息队列原因，未处理该帧，则不丢弃数据
					process_len = 0;
					free(pl);
				}
			}
            else
            {
                process_len = 1;
				free(pl);
            }
        }

        stream_buffer_point_shift(&uart_rxstream, &uart_rxstream.read_point, process_len);
        recv_len = stream_buffer_recv_len(&uart_rxstream);
    }
}

// //读取芯片ID
// void ReadStaIdInfo(void);

// LCIP串口处理任务
void LcipAppLocalTask(void *arg)
{
	OS_ERR err;
	InitLocalUartTask();
	LcipLocalInit();

	while (1)
	{
		// 串口是否接收数据完成
		OSSemPend(&LcipReceiveComplateSEM, 1, OS_OPT_PEND_BLOCKING, 0, &err);

		if(err == OS_ERR_NONE)
		{
			lcip_protocol_parsing();
		}

		LcipAppLocalHandle();
	}
}

extern void node485ReportInNetTask();

void TimerFunTask(void *arg)
{
	while (1)
	{
		OS_ERR  os_err;
		MeterLocalHandle();
		node485ReportInNetTask();
		OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_DLY, &os_err);
	}
}

//===




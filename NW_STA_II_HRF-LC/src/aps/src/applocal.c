#include "protocol_includes.h"
#include "os.h"
#include "common_includes.h"
#include <string.h>
#include "bsp.h"
#include "Revision.h"
#ifdef UART_DEBUG
#include "sta_status_debug.h"
#endif
#include "hplc_data.h"
#include "app_search_meter.h"
#include "app_event_IIcai.h"
#include "app_led_IIcai.h"
#include "timer.h"
#include "bps_timer.h"
#include "bps_uart.h"
#include "bps_ir_uart.h"
#include "plm_2005.h"

#include "app_RF_IIcai.h"
#include "lc_obj_model.h"
#include "local_protocol.h"
#include "applocal.h"
#include "lcip_applocal.h"
#include "ble_api.h"
#include "hplc_data.h"
#include "ble_net.h"
#include "ble_update.h"

#define LOCAL_BUFF_SIZE 1500

#define BIND_ADDR_TOTAL_TIMEOUT 10 * 1000UL

#define DEFAULT_WAIT_FRAME_TIME 500 // 默认等待帧返回

#define LOCAL_DATA_QUEUE MAX_LOCAL_PDU_SIZE

struct
{
	u8 status;
	u32 Tick;
} UART_STATUS;

typedef enum
{
	LOCAL_IDLE,	   // 空闲
	LOCAL_RECEIVE, // 正在接收
} LOCAL_UART_STATE;

// 本地TCB
extern OS_TCB App_II_STA_TCB;
extern OS_TCB App_AppLocalTCB;

extern u32 HPLCCurrentFreq;

// 接收数据信号量
static OS_SEM ReceiveComplateSEM;

// 进入测试模式信号量
OS_SEM ReceiveIrComplateSEM;
static OS_SEM TestModeSEM;

// 本地接收数据队列
static OS_Q LocalDataQueue;

// 是否已经绑定地址
bool IsBindAddr = false;

// 绑定地址是否超时
static bool IsBindAddrTimeout = false;

static LOCAL_PARAM *DiffRatePdu = NULL;
// 本地电能表的协议
u8 MeterProtocol = LOCAL_PRO_DLT645_2007;
u8 MeterSupport = 0;
// 地址模式
u8 BindingAddrModeCnt = 0;
int defultAddrCnt = 3; // 设置默认波特率执行的波特率个数

// 是否已经接收完成
bool LocalReceiveFlg = false;

// 绑定地址超时定时器
static TimeValue bindTimer;

// 绑定地址总超时定时器（超过1分钟，将把超时标志置位）
static TimeValue bindTotalTimer;

// 接收超时定时器
static TimeValue receiveTimer;
#ifndef II_STA
// 连续没有收到帧的次数
static u8 NoAckCount = 0;
#endif
// 接收缓存
static u8 LocalReceiveBuf[LOCAL_BUFF_SIZE] = {0};
static u16 LocalReceiveLen = 0;

// 本地串口参数
static UartParameterType Usart1Para;

// 波特率
static u32 LoacalBaud = 2400;

// 串口状态
static LOCAL_UART_STATE LocalUartState = LOCAL_IDLE;

// 当前处理的数据
static LOCAL_PARAM *CurrentLoaclPdu = NULL;

// P内存池
// static OS_MEM LocalMempool;
// static LOCAL_PARAM LocalMempdu[MAX_LOCAL_PDU_SIZE];
u32 AppUseUartFlag = 0;

static void BSP_OS_TimeDly(CPU_INT32U dly_tick)
{
	OS_ERR err;

	OSTimeDly(dly_tick, OS_OPT_TIME_DLY, &err);

	(void)&err;
}

// P内存池

#ifdef PROTOCOL_NW_2021
#define METER_UP_NUM 4
signed char event_idx = 0;
u8 meter_event[METER_UP_NUM][256];
#endif

// 波特率协商
bool BaudOK = false;
bool BaudGeting = false;
u8 changeBaudVaile = 6;
u8 DectecDoublePortocol = 0;
const u32 baudTable[] = {
	300, 600, 1200, 2400, 4800, 9600, 19200};

// 资产编码告知
bool AssetsCodeSending = false;
u8 AssetsCodeSendTimes = 2; // 发送尝试次数

static OS_MUTEX s_event_mutex;
bool EVENT_AddTolist(P_Dl645_Freme p645)
{
#ifdef SUPPORT_21_METER
	OS_ERR err;
	OSMutexPend(&s_event_mutex, 0, OS_OPT_PEND_BLOCKING, NULL, &err);

	for (int i = 0; i < METER_UP_NUM; i++)
	{
		// 如果有重复的直接返回
		if (meter_event[i][0] == 0x68 && memcmp(p645, meter_event[i], p645->dataLen + 12) == 0)
		{
			LocalReceiveLen = 0;
			OSMutexPost(&s_event_mutex, OS_OPT_POST_1, &err);
			return false;
		}
	}

	// 电表持续上报事件则循环存储最新的事件
	event_idx++;
	if (event_idx > METER_UP_NUM - 1)
	{
		event_idx = 0;
	}
	memcpy(meter_event[event_idx], p645, p645->dataLen + 12);

	OSMutexPost(&s_event_mutex, OS_OPT_POST_1, &err);

#endif
	return true;
}

u8 *GetMeterUpEvent(void)
{
	u8 *ptemp = NULL;
#ifdef SUPPORT_21_METER
	OS_ERR err;

	OSMutexPend(&s_event_mutex, 0, OS_OPT_PEND_BLOCKING, NULL, &err);

	if (meter_event[event_idx][0] == 0x68)
	{
		ptemp = meter_event[event_idx];
		event_idx--;
		if (event_idx < 0)
		{
			event_idx = METER_UP_NUM - 1;
		}
	}

	OSMutexPost(&s_event_mutex, OS_OPT_POST_1, &err);
#endif
	return ptemp;
}

// 分配一个PDU
LOCAL_PARAM *MallocLocal(void)
{
	OS_ERR err;
	LOCAL_PARAM *mem_ptr = 0;
	mem_ptr = (LOCAL_PARAM *)OSMemGet(&Mempool, &err);
	if (err != OS_ERR_NONE)
	{
#ifdef __DEBUG_MODE
		while (1)
		{
			__disable_irq();
			FeedWdg();
		}
#else
		return (void *)0;
#endif
	}
	return mem_ptr;
}

// 释放一个PDU
void FreeLocal(LOCAL_PARAM *pdu)
{
	OS_ERR err;
	OSMemPut(&Mempool, pdu, &err);
}

u8 GetUartStatus(void)
{
	if (UART_STATUS.status)
	{
		OS_ERR err;
		u32 tick = OSTimeGet(&err);
		if ((tick - UART_STATUS.Tick) > OS_CFG_TICK_RATE_HZ * 60 * 5) // 超过5min
		{
			return 1; // 目前不通
		}
		else
		{
			return 2; // 历史出现过不通情况
		}
	}
	else
	{
		return 0; // 正常
	}
}
// 发送完成通知
static void UsartSendComplate(int arg_null)
{
#ifdef II_STA
	METER_UART.Control(ARM_USART_CONTROL_RX, 1);
	
	// rs485_sending_flag_set_fun(false);
#endif
}

// 接收完成通知
static void UsartReceiveComplate(int len)
{
	OS_ERR err;
	OSSemPost(&ReceiveComplateSEM, OS_OPT_POST_ALL, &err);
	debug_str(DEBUG_LOG_METER, "UartReceive ,LEN=%d \r\n",len);
#ifdef II_STA
	// rs485_recving_flag_set_fun(false);
#endif
}

static void UsartIrReceiveComplate(int arg)
{
	OS_ERR err;
	OSSemPost(&ReceiveIrComplateSEM, OS_OPT_POST_ALL, &err);
}

static char getRecFlag = 0;
static void UsartReceiveErr(int arg)
{
#ifdef II_STA
	OS_ERR err;
	if (getRecFlag)
	{
		OSTaskSemPost(&App_II_STA_TCB, OS_OPT_POST_ALL, &err);
	}
#endif
}
void UartNeedProcErr(int flag)
{
	getRecFlag = flag;
}
extern bool TestMode;
// 改变波特率
static void ChangeLocalUartBaud(u32 baud)
{
	if(TestMode == true)
	{
		baud = 9600;
	}
	UartClose();
	Usart1Para.BaudRate = baud;
	//    Usart1Para.DataBits = 8;
	Usart1Para.Parity = Parity_Even;
	//    Usart1Para.StopBits = Stop_Bits_One;
	Usart1Para.RxHaveDataFunction = UsartReceiveComplate;
	Usart1Para.TxDataCompleteFunction = UsartSendComplate;
	Usart1Para.RxParityErrorFunction = UsartReceiveErr;
	UartOpen(&Usart1Para);
}

static void ChangeLocalUartBaud_Parity(u32 baud,u8 parity)
{
	UartClose();
	Usart1Para.BaudRate = baud;
	//    Usart1Para.DataBits = 8;
	if(parity == UART_PARITY_NO)
	{
		Usart1Para.Parity = Parity_No;
	}
	else if(parity == UART_PARITY_EVEN)
	{
		Usart1Para.Parity = Parity_Even;
	}
	else
	{
		Usart1Para.Parity = Parity_Odd;
	}
	
	//    Usart1Para.StopBits = Stop_Bits_One;
	Usart1Para.RxHaveDataFunction = UsartReceiveComplate;
	Usart1Para.TxDataCompleteFunction = UsartSendComplate;
	Usart1Para.RxParityErrorFunction = UsartReceiveErr;
	UartOpen(&Usart1Para);
}


#ifdef II_STA
// 给app_search_meter.c调的
void ChangeLocalUartBaud_t(u32 baud)
{
	LoacalBaud = baud;
	ChangeLocalUartBaud(baud);
}
u32 GetLoacalBaud(void)
{
	return LoacalBaud;
}
#endif

void OpenIr(void)
{
	UartParameterType UsartIrPara;
	UsartIrPara.BaudRate = 1200;
	// UsartIrPara.DataBits = 8;
	UsartIrPara.Parity = Parity_Even;
	// UsartIrPara.StopBits = Stop_Bits_One;
	UsartIrPara.RxHaveDataFunction = UsartIrReceiveComplate;
	UsartIrPara.TxDataCompleteFunction = NULL;
	UsartIrPara.RxParityErrorFunction = NULL;
	UartIrOpen(&UsartIrPara);
}

// 进入/退出测试模式
void LocalEnterTestModel(bool status)
{
	OS_ERR err;
	if (status)
	{
		while (IsUartTxData())
		{
			OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
		}
		OSTaskSuspend(&App_AppLocalTCB, &err);
		UartClose();
		#ifdef NW_NEW_TEST
    	Usart1Para.BaudRate = 115200;
    	Usart1Para.Parity = Parity_No;
		#else
    	Usart1Para.BaudRate = 2400;
    	Usart1Para.Parity = Parity_Even;
		#endif
		UartOpen(&Usart1Para);
	}
	else
	{
		while (IsUartTxData())
		{
			OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
		}
		UartClose();
		Usart1Para.BaudRate = LoacalBaud;
		Usart1Para.Parity = Parity_Even;
		UartOpen(&Usart1Para);
		OSTaskResume(&App_AppLocalTCB, &err);
	}
}

// 返回串口使用的波特率
u32 GetLoaclUartBaud(void)
{
	return LoacalBaud;
}

void LocalInit(void)
{
	OS_ERR err;
	ChangeLocalUartBaud(9600);
	StartTimer(&bindTimer, 0);
	StartTimer(&receiveTimer, 0);
	OSSemCreate(&ReceiveComplateSEM, "Receive SEM", 0, &err);
	if (err != OS_ERR_NONE)
		while (1)
			;

	OSSemCreate(&ReceiveIrComplateSEM, "Ir SEM", 0, &err);
	if (err != OS_ERR_NONE)
		while (1)
			;

	OSSemCreate(&TestModeSEM, "TestMode SEM", 0, &err);
	if (err != OS_ERR_NONE)
		while (1)
			;

	OSQCreate(&LocalDataQueue, "LocalDataQueue", LOCAL_DATA_QUEUE, &err);
	if (err != OS_ERR_NONE)
		while (1)
			;

	//    OSMemCreate(&LocalMempool,"Local Block Memory",LocalMempdu, MAX_LOCAL_PDU_SIZE, sizeof(LOCAL_PARAM), &err);
	//    if(err != OS_ERR_NONE)
	//        while(1);

	StartTimer(&bindTotalTimer, BIND_ADDR_TOTAL_TIMEOUT);

	OSMutexCreate(&s_event_mutex, "event add mutex", &err);
}

void changeBaudBack(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8 *data, u16 len, u8 sendType, u16 srcTei)
{

	if (3 != DectecDoublePortocol) // 未判断是否是双协议表
	{
		if (vaild)
		{

			if (Is645Frame(data, len)) // 返回了645报文
			{
				for (int i = 0; i < 4; i++)
				{
					data[10 + i] -= 0x33;
				}
				if (DectecDoublePortocol == 0) // 判断电压合格率
				{
					if (*(u32 *)&data[10] == DL645_GRAPH_DI)
					{
						MeterSupport |= METER_SUPPORT_GRAPH;
					}
				}
				else if (DectecDoublePortocol == 1)
				{
					if (*(u32 *)&data[10] == DL645_RATE_DI)
					{
						MeterSupport |= METER_SUPPORT_RATE;
					}
				}
				else if (DectecDoublePortocol == 2)
				{
					if (*(u32 *)&data[10] == DL645_COMB_DI)
					{
						MeterSupport |= METER_SUPPORT_COMB;
					}
				}
			}
		}
		DectecDoublePortocol++;
		return;
	}
#ifdef SUPPORT_21_METER
	BaudGeting = false;
#ifndef II_STA
	NoAckCount = 0;
#endif
	if (vaild)
	{
		if (Is645Frame(data, len))
		{
			debug_str(DEBUG_LOG_APP, "changeBaudBack get 645 data\r\n");

			P_Dl645_Freme pFrame = NULL;
			for (int index = 0; index < len; index++)
			{
				if (data[index] == 0x68)
				{
					pFrame = (P_Dl645_Freme)&data[index];
					break;
				}
			}

			if (pFrame->controlCode == 0x9f && pFrame->dataLen == 0)
			{
				BaudOK = true;
				ChangeLocalUartBaud(baudTable[changeBaudVaile]);
				LoacalBaud = baudTable[changeBaudVaile];
				return;
			}
		}
	}

	--changeBaudVaile;
#endif
}

void SetAssetsCodeBack(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8 *data, u16 len, u8 sendType, u16 srcTei)
{
#ifdef SUPPORT_21_METER
	AssetsCodeSending = false;

	if (vaild)
	{
		if (Is645Frame(data, len))
		{
			debug_str(DEBUG_LOG_APP, "SetAssetsCodeBack get 645 data\r\n");

			P_Dl645_Freme pFrame = NULL;
			for (int index = 0; index < len; index++)
			{
				if (data[index] == 0x68)
				{
					pFrame = (P_Dl645_Freme)&data[index];
					break;
				}
			}

			if ((pFrame->controlCode == 0x94 && pFrame->dataLen == 0) || // 从站正常应答
				(pFrame->controlCode == 0xD4 && pFrame->dataLen == 1))	 // 从站异常应答
			{
				AssetsCodeSending = true;
				return;
			}
			else
			{
				debug_str(DEBUG_LOG_APP, "SetAssetsCodeBack, ctrl code:0x%x, data len:%d\r\n", pFrame->controlCode, pFrame->dataLen);
			}
		}
	}
#endif
}

void ChangeBaud(void)
{
	u8 data[58] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x02, 0xee, 0xee, 0xee, 0xff, 0x01, 0x02, 0xbb, 0xff, 0x01, 0x02, 0x02};
	if (3 != DectecDoublePortocol)
	{
		if (MeterProtocol == LOCAL_PRO_DLT645_2007)
		{
			int len = 0;
			memcpy(data, GetStaBaseAttr()->Mac, 6);
			TransposeAddr(data);
			if (0 == DectecDoublePortocol)
			{
				len = Get645_07DataFrame(&LocalReceiveBuf[1000], data, DL645_GRAPH_DI);
			}
			else if (1 == DectecDoublePortocol)
			{
				len = Get645_07DataFrame(&LocalReceiveBuf[1000], data, DL645_RATE_DI);
			}
			else if (2 == DectecDoublePortocol)
			{
				len = Get645_07DataFrameWithData(&LocalReceiveBuf[1000], data, DL645_COMB_DI, &data[6], 13);
			}
			// 探测是否支持曲线读取
			AppLocalReceiveData(&data[0], &data[0], 8, 0, &LocalReceiveBuf[1000], len, changeBaudBack, 0, 0, BAUD_SEARCH);
		}
		return;
	}
#ifdef SUPPORT_21_METER
	// u8 data[58]; //最大支持32字节资产编码告知报文
	u8 len = 0;
#ifdef SUPPORT_21_METER_NO_CHANGE_RATE
	if (baudTable[changeBaudVaile] >= LoacalBaud)
	{
		for (int i = 6; i >= 0; i--)
		{
			if (baudTable[i] == LoacalBaud)
			{
				changeBaudVaile = i;
				break;
			}
		}
	}
#endif
	if ((!BaudGeting) && (!BaudOK))
	{

		if (LoacalBaud <= baudTable[changeBaudVaile]) // 波特率协商需要协商到本身的波特率为止  因为南网的MAC需要使用这条报文写入 放止表默认是19200时没有写入mac
		{
			len = sizeof(data);
			GetBaud(data, &len, GetStaBaseAttr()->Mac, changeBaudVaile);
			AppLocalReceiveData(&data[0], &data[0], 8, 0, data, len, changeBaudBack, 0, 0, 0); // 地址是无用的此处使用错误的地址放止错误计数器增加

			BaudGeting = true;

			return;
		}
	}

	// 波特率协商结束，告知电表资产编码
	if (!AssetsCodeSending && AssetsCodeSendTimes) // 告知电表资产编号
	{
		len = SetAssetsCode(data, GetStaBaseAttr()->Mac);
		AppLocalReceiveData(&data[0], &data[0], 8, 0, data, len, SetAssetsCodeBack, 0, 0, 0); // 地址是无用的此处使用错误的地址放止错误计数器增加

		AssetsCodeSending = true;
		AssetsCodeSendTimes--;
	}
#endif
}
extern u8 meter_uart_isopen;
// 绑定地址任务
void BindingAddr(void)
{
	BindingAddrModeCnt++;
	const u8 *BindAddrFrame = 0;
	u16 BindAddrFrameLen = 0;

	if (defultAddrCnt > 0)
	{
		defultAddrCnt--;
	}
	else if (defultAddrCnt == 0)
	{
		defultAddrCnt--;
		BindingAddrModeCnt = 1;
	}
	switch (BindingAddrModeCnt)
	{
	case 1: // 2400-698
		BindAddrFrame = ReadMeterAddr698;
		BindAddrFrameLen = sizeof(ReadMeterAddr698);
		MeterProtocol = LOCAL_PRO_698_45;
		ChangeLocalUartBaud(2400);
		LoacalBaud = 2400;
		break;
	case 2: // 2400-07
		BindAddrFrame = ReadMeterAddr07;
		BindAddrFrameLen = sizeof(ReadMeterAddr07);
		MeterProtocol = LOCAL_PRO_DLT645_2007;
		ChangeLocalUartBaud(2400);
		LoacalBaud = 2400;
		break;
	case 13: // 2400-97
		BindAddrFrame = ReadMeterAddr97;
		BindAddrFrameLen = sizeof(ReadMeterAddr97);
		MeterProtocol = LOCAL_PRO_DLT645_1997;
		ChangeLocalUartBaud(2400);
		LoacalBaud = 2400;
		break;

	case 3: // 9600-698
		BindAddrFrame = ReadMeterAddr698;
		BindAddrFrameLen = sizeof(ReadMeterAddr698);
		MeterProtocol = LOCAL_PRO_698_45;
		ChangeLocalUartBaud(9600);
		LoacalBaud = 9600;
		break;
	case 4: // 9600-07
		BindAddrFrame = ReadMeterAddr07;
		BindAddrFrameLen = sizeof(ReadMeterAddr07);
		MeterProtocol = LOCAL_PRO_DLT645_2007;
		ChangeLocalUartBaud(9600);
		LoacalBaud = 9600;
		break;
	case 14: // 9600-97
		BindAddrFrame = ReadMeterAddr97;
		BindAddrFrameLen = sizeof(ReadMeterAddr97);
		MeterProtocol = LOCAL_PRO_DLT645_1997;
		ChangeLocalUartBaud(9600);
		LoacalBaud = 9600;
		break;

	case 5: // 19200-698
		BindAddrFrame = ReadMeterAddr698;
		BindAddrFrameLen = sizeof(ReadMeterAddr698);
		MeterProtocol = LOCAL_PRO_698_45;
		ChangeLocalUartBaud(19200);
		LoacalBaud = 19200;
		break;
	case 6: // 19200-07
		BindAddrFrame = ReadMeterAddr07;
		BindAddrFrameLen = sizeof(ReadMeterAddr07);
		MeterProtocol = LOCAL_PRO_DLT645_2007;
		ChangeLocalUartBaud(19200);
		LoacalBaud = 19200;
		break;
	case 15: // 19200-97
		BindAddrFrame = ReadMeterAddr97;
		BindAddrFrameLen = sizeof(ReadMeterAddr97);
		MeterProtocol = LOCAL_PRO_DLT645_1997;
		ChangeLocalUartBaud(19200);
		LoacalBaud = 19200;
		break;

	case 7: // 38400-698
		BindAddrFrame = ReadMeterAddr698;
		BindAddrFrameLen = sizeof(ReadMeterAddr698);
		MeterProtocol = LOCAL_PRO_698_45;
		ChangeLocalUartBaud(38400);
		LoacalBaud = 38400;
		break;
	case 8: // 38400-07
		BindAddrFrame = ReadMeterAddr07;
		BindAddrFrameLen = sizeof(ReadMeterAddr07);
		MeterProtocol = LOCAL_PRO_DLT645_2007;
		ChangeLocalUartBaud(38400);
		LoacalBaud = 38400;
		break;
	case 16: // 38400-97
		BindAddrFrame = ReadMeterAddr97;
		BindAddrFrameLen = sizeof(ReadMeterAddr97);
		MeterProtocol = LOCAL_PRO_DLT645_1997;
		ChangeLocalUartBaud(38400);
		LoacalBaud = 38400;
		break;

	case 9: // 1200-698
		BindAddrFrame = ReadMeterAddr698;
		BindAddrFrameLen = sizeof(ReadMeterAddr698);
		MeterProtocol = LOCAL_PRO_698_45;
		ChangeLocalUartBaud(1200);
		LoacalBaud = 1200;
		break;
	case 10: // 1200-07
		BindAddrFrame = ReadMeterAddr07;
		BindAddrFrameLen = sizeof(ReadMeterAddr07);
		MeterProtocol = LOCAL_PRO_DLT645_2007;
		ChangeLocalUartBaud(1200);
		LoacalBaud = 1200;
		break;
	case 17: // 1200-97
		BindAddrFrame = ReadMeterAddr97;
		BindAddrFrameLen = sizeof(ReadMeterAddr97);
		MeterProtocol = LOCAL_PRO_DLT645_1997;
		ChangeLocalUartBaud(1200);
		LoacalBaud = 1200;
		break;

	case 11: // 4800-698
		BindAddrFrame = ReadMeterAddr698;
		BindAddrFrameLen = sizeof(ReadMeterAddr698);
		MeterProtocol = LOCAL_PRO_698_45;
		ChangeLocalUartBaud(4800);
		LoacalBaud = 4800;
		break;
	case 12: // 4800-07
		BindAddrFrame = ReadMeterAddr07;
		BindAddrFrameLen = sizeof(ReadMeterAddr07);
		MeterProtocol = LOCAL_PRO_DLT645_2007;
		ChangeLocalUartBaud(4800);
		LoacalBaud = 4800;
		break;
	case 18: // 4800-97
		BindAddrFrame = ReadMeterAddr97;
		BindAddrFrameLen = sizeof(ReadMeterAddr97);
		MeterProtocol = LOCAL_PRO_DLT645_1997;
		ChangeLocalUartBaud(4800);
		LoacalBaud = 4800;
		break;

	default: // 2400-07
		BindAddrFrame = ReadMeterAddr07;
		BindAddrFrameLen = sizeof(ReadMeterAddr07);
		MeterProtocol = LOCAL_PRO_DLT645_2007;
		ChangeLocalUartBaud(2400);
		LoacalBaud = 2400;
		BindingAddrModeCnt = 0;
		meter_uart_isopen = !meter_uart_isopen;
		break;
	}
	UartWrite((uint8_t *)BindAddrFrame, BindAddrFrameLen);
	LocalUartState = LOCAL_RECEIVE;
}

// 复位本地串口
void ResetLocalUart(void)
{
	OS_ERR err;
	// 清空发送队列
	while (IsUartTxData())
	{
		OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
	}
	// 清空读取队列
	u32 len = UartRead(&LocalReceiveBuf[0], LOCAL_BUFF_SIZE);

	ChangeLocalUartBaud(LoacalBaud);
}

extern void AppHandleReadMeter(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8 *data, u16 len, u8 sendType, u16 srcTei);

// 接收APP层的数据
// u32 Baud:
//   表示发送时要使用的波特率.
//   0:来自上层的调用,需要根据MAC地址去找对应的波特率.
//   2400 4800 9600 19200: 强制的波特率, 数值来自
//   struct strList_Meter --> BaudType的注释
bool AppLocalReceiveData(u8 srcAddr[6], u8 dstAddr[6], u8 timeout, u16 seq, u8 *data, u16 len, pLocalHandelCallback fun, u8 sendType, u16 srcTei, u32 Baud)
{
	OS_ERR err;
	LOCAL_PARAM *localPdu = MallocLocal();
	if (localPdu == NULL)
		return false;
	memcpy(localPdu->SrcAddr, srcAddr, 6);
	memcpy(localPdu->DstAddr, dstAddr, 6);
	localPdu->Timeout = timeout;
	if (localPdu->Timeout > 100)
		localPdu->Timeout = 100;
	localPdu->Seq = seq;
	localPdu->SendType = sendType;
	localPdu->SrcTei = srcTei;
	if (data && len <= MAX_LOCAL_LEN)
		memcpy(localPdu->Data, data, len);
	localPdu->Size = len;
	localPdu->fun = fun;
	localPdu->Baud = Baud;

	OSQPost(&LocalDataQueue, localPdu, sizeof(LOCAL_PARAM), OS_OPT_POST_FIFO | OS_OPT_POST_ALL, &err);
	if (err != OS_ERR_NONE)
	{
		FreeLocal(localPdu);
		return false;
	}
	else
	{
		if (fun == AppHandleReadMeter)
		{
			AppUseUartFlag++;
		}
	}
	return true;
}

// 返回是否已经绑定了地址
bool IsBindLocalMac(void)
{
	return IsBindAddr;
}

// 解绑地址
void DispatchLocalMac(void)
{
	IsBindAddr = false;
	BindingAddrModeCnt = 0;
	IsBindAddrTimeout = false;
	StartTimer(&bindTotalTimer, BIND_ADDR_TOTAL_TIMEOUT);
	StartTimer(&bindTimer, FIRST_READ_ADDR_DELAY);
}

// 绑定地址是否已经超时(目前设定为1分钟)
bool IsBindLocalTimeout(void)
{
	return IsBindAddrTimeout;
}

// 返回电能表协议
u8 GetMeterProtocol(void)
{
	return MeterProtocol;
}

#ifdef INTELLIGENCE_LOADING

void NetStatusNotify(void)
{
	u8 frame[25] = {0};
	u8 addr[6] = {0};
	u16 len = 0;
	u8 inNet = 0;
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();

	memcpy(addr, pNetBasePara->Mac, 6);
	TransposeAddr(addr);
	inNet = (pNetBasePara->NetState == NET_IN_NET)?1:0;
	len = Get645_NetStatusFrame(frame, addr, inNet);
	AppLocalReceiveData(addr, addr, 8, 0, frame, len, NULL, 0, 0);
}

u16 NetStatusTimerid = INVAILD_TMR_ID;
void NetStatusNotifyTimerCallback(void *arg)
{
    if (IsBindLocalMac()) {
        NetStatusNotify();
    } else {
        if (INVAILD_TMR_ID != NetStatusTimerid) {
            DeleteTmrSelf();
            NetStatusTimerid = INVAILD_TMR_ID;
        }
    }
}

#endif

#if !defined(II_STA)
static void closeStaPin(void *arg)
{
	StaWrite(0);
}
#endif
#pragma pack(1)
typedef struct
{
	u8 len;
	u8 para;
	u32 block;
	u8 data[2];
	u32 crc;
} uart_updata;
#pragma pack(4)
u32 mem_ptr[1024 / 4];
// 本地帧处理

extern u32 zeroNtbDiff[];
extern u8 zero_check_pass[];
extern s32 factoryTestRecvRssi;
#ifndef ZB204
u32 uart_invert = 0;
void UART_ExernExe(void)
{
	OS_ERR err;
	uart_invert = OSTimeGet(&err);
#ifdef II_STA
	// rs485_recving_flag_set_fun(true);
#endif
}
#endif
void AppLocalHandle(void)
{
	OS_ERR err;
	u16 index = 0;
	static int start_uart_update = 0;
	OSSemPend(&ReceiveComplateSEM, 1, OS_OPT_PEND_BLOCKING, 0, &err);
	if (LocalUartState == LOCAL_RECEIVE)
	{
		if (err == OS_ERR_NONE)
		{
			u32 len = UartRead(&LocalReceiveBuf[LocalReceiveLen], LOCAL_BUFF_SIZE - LocalReceiveLen);
			LocalReceiveLen += len;
			debug_str(DEBUG_LOG_METER, "UartRead ,rData=");
			debug_hex(DEBUG_LOG_METER,LocalReceiveBuf,LocalReceiveLen);
			FlashLocalLed();
#ifndef II_STA
			if (!IsBindAddr)
			{
				bool checkCsmState = false;
				if (MeterProtocol == LOCAL_PRO_DLT645_1997 || MeterProtocol == LOCAL_PRO_DLT645_2007)
				{
					// 返回的帧从68头开始
					for (index = 0; index < LocalReceiveLen; index++)
					{
						if (LocalReceiveBuf[index] == 0x68)
						{
							if (Is645Frame(&LocalReceiveBuf[index], LocalReceiveLen - index))
							{
								checkCsmState = true;
								break;
							}
						}
					}

					if (index >= LocalReceiveLen)
					{
						if (LocalReceiveLen >= LOCAL_BUFF_SIZE)
						{
							LocalReceiveLen = 0;
						}
						return;
					}
				}
				else if (MeterProtocol == LOCAL_PRO_698_45)
				{
					for (index = 0; index < LocalReceiveLen; index++)
					{
						if (LocalReceiveBuf[index] == 0x68)
						{
							if (Is698Frame(&LocalReceiveBuf[index], LocalReceiveLen - index))
							{
								checkCsmState = true;
								break;
							}
						}
					}
					if (index >= LocalReceiveLen)
					{
						if (LocalReceiveLen >= LOCAL_BUFF_SIZE)
						{
							LocalReceiveLen = 0;
						}
						return;
					}
				}
				if (checkCsmState)
				{
					for (; index < LocalReceiveLen; index++)
					{
						if (LocalReceiveBuf[index] == 0x68)
							break;
					}
					switch (MeterProtocol)
					{
					case LOCAL_PRO_DLT645_1997:
					{
						u8 global_mac_97[6] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}; // 通配地址
						if (memcmp(global_mac_97, &LocalReceiveBuf[index + 1], LONG_ADDR_LEN) != 0)
						{
							// 地址颠倒
							TransposeAddr(&LocalReceiveBuf[index + 1]);
							SetStaMAC(&LocalReceiveBuf[index + 1]);
							IsBindAddr = true;
						}
						break;
					}
					case LOCAL_PRO_DLT645_2007:
					{
						if (LocalReceiveBuf[index + 8] == 0x93)
						{
							TransposeAddr(&LocalReceiveBuf[index + 1]);
							SetStaMAC(&LocalReceiveBuf[index + 1]);
							IsBindAddr = true;
#ifdef INTELLIGENCE_LOADING
                            if (INVAILD_TMR_ID == NetStatusTimerid) {
                                NetStatusTimerid = HPLC_RegisterTmr(NetStatusNotifyTimerCallback, NULL, 60*1000, TMR_OPT_CALL_PERIOD);
                            }
#endif
						}
						break;
					}
					case LOCAL_PRO_698_45:
					{
						if (LocalReceiveBuf[index + 4] & (1 << 5)) // 有1字节的扩展逻辑地址
						{
							TransposeAddr(&LocalReceiveBuf[index + 6]);
							SetStaMAC(&LocalReceiveBuf[index + 6]);
						}
						else
						{
							TransposeAddr(&LocalReceiveBuf[index + 5]);
							SetStaMAC(&LocalReceiveBuf[index + 5]);
						}
						IsBindAddr = true;
						break;
					}
					default:
						break;
					}
					StaWrite(1);
					HPLC_RegisterTmr(closeStaPin, NULL, 200, TMR_OPT_CALL_ONCE);
					// 使用地址设置随机种子
					//					const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();
					//					srand(CommonGetCrc32((u8*)pNetBasePara->Mac,6));
					LocalUartState = LOCAL_IDLE;
					if (DiffRatePdu != NULL && DiffRatePdu == CurrentLoaclPdu)
					{
						ChangeLocalUartBaud(LoacalBaud);
						DiffRatePdu = NULL;
					}
					LocalReceiveLen = 0;
				}
			}
			else
#endif
			{
#ifdef I_STA
				P_LMP_LINK_HEAD temp = NULL;
				if (temp = PLM_Check_Frame_3762(LocalReceiveBuf, LocalReceiveLen))
				{
					index = (u8 *)temp - LocalReceiveBuf;
					if (CurrentLoaclPdu->Baud == 0) // 需要提取帧中的内容进行上报
					{
						LMP_APP_INFO info;
						info.lmpLinkHead = temp;
						if (SUCCESS == LMP_App_Proc_gd(&info))
						{
							index = info.curProcUnit->data_unit - LocalReceiveBuf; // 解析有效
						}
						else
						{
							index = LocalReceiveLen; // 解析无效
						}
					}
				}
#else
				for (index = 0; index < LocalReceiveLen; index++)
				{
					if (LocalReceiveBuf[index] == 0x68)
					{
						if (Is645Frame(&LocalReceiveBuf[index], LocalReceiveLen - index) || Is698Frame(&LocalReceiveBuf[index], LocalReceiveLen - index))
							break;
					}
				}
#endif
				if (index >= LocalReceiveLen)
				{
					if (LocalReceiveLen >= LOCAL_BUFF_SIZE)
					{
						LocalReceiveLen = 0;
					}
					return;
				}
				if (index < LocalReceiveLen)
				{
#ifndef II_STA
					NoAckCount = 0;
#endif
					if (CurrentLoaclPdu->fun)
					{
						CurrentLoaclPdu->fun(CurrentLoaclPdu->SrcAddr, CurrentLoaclPdu->DstAddr, CurrentLoaclPdu->Seq, true, &LocalReceiveBuf[index], LocalReceiveLen - index, CurrentLoaclPdu->SendType, CurrentLoaclPdu->SrcTei);
					}
				}
				else
				{
					if (CurrentLoaclPdu->fun)
					{
						if (LocalReceiveLen)
						{
							debug_str(DEBUG_LOG_METER, "rcv Meter data ok,seq=%04x\r\n",CurrentLoaclPdu->Seq);
							CurrentLoaclPdu->fun(CurrentLoaclPdu->SrcAddr, CurrentLoaclPdu->DstAddr, CurrentLoaclPdu->Seq, true, &LocalReceiveBuf[0], LocalReceiveLen, CurrentLoaclPdu->SendType, CurrentLoaclPdu->SrcTei);
						}
						else
						{
							CurrentLoaclPdu->fun(CurrentLoaclPdu->SrcAddr, CurrentLoaclPdu->DstAddr, CurrentLoaclPdu->Seq, false, &LocalReceiveBuf[0], LocalReceiveLen, CurrentLoaclPdu->SendType, CurrentLoaclPdu->SrcTei);
							RecError code = {0x1, 0x1, 0};
							saveHplcErrLog((char *)&code, sizeof(code));
						}
					}
				}
				if (DiffRatePdu != NULL && DiffRatePdu == CurrentLoaclPdu)
				{
					ChangeLocalUartBaud(LoacalBaud);
					DiffRatePdu = NULL;
				}
				if ((CurrentLoaclPdu->fun == AppHandleReadMeter) && AppUseUartFlag) // add by lxl, 20211025
				{
					AppUseUartFlag--;
				}
				LocalUartState = LOCAL_IDLE;
				FreeLocal(CurrentLoaclPdu);
#ifndef II_STA
				NoAckCount = 0;
#endif
				LocalReceiveLen = 0;
			}
		}
		else // 串口没读到数据
		{
#ifndef II_STA
			if (!IsBindAddr) // 没绑定地址
			{
				if (TimeOut(&bindTimer))
				{
					LocalReceiveLen = 0;
					if ((CurrentLoaclPdu) && (CurrentLoaclPdu->fun == AppHandleReadMeter) && AppUseUartFlag) // add by lxl, 20211025
					{
						AppUseUartFlag--;
					}
					LocalUartState = LOCAL_IDLE;
					if (DiffRatePdu != NULL && DiffRatePdu == CurrentLoaclPdu)
					{
						ChangeLocalUartBaud(LoacalBaud);
						DiffRatePdu = NULL;
					}
				}
			}
			else
#endif
			{
#ifndef ZB204
				u32 current_tick = OSTimeGet(&err);
				if ((current_tick - uart_invert) < 300) // 帧间隔小于300ms
				{
					StartTimer(&receiveTimer, CurrentLoaclPdu->Timeout * 100);
					return;
				}
#endif
				if (TimeOut(&receiveTimer)) // 超时没收到数据
				{
// 先找出此帧的地址
#ifndef II_STA
					u8 addr[6];
					memcpy(addr, CurrentLoaclPdu->DstAddr, 6);
					const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
					TransposeAddr(addr);
					// 比较与自己的地址是否一致，一致的话就记录失败次数
					if (memcmp(pNetBasePara->Mac, addr, 6) == 0)
					{
						NoAckCount++;
						if (NoAckCount & 0x03) // 每隔4次复位一次
						{
							ResetLocalUart();
						}
						if (NoAckCount >= 10)
						{
							// 如果没有接受到回复的次数超过10，直接进入绑定地址模式
							IsBindAddr = false;
							BindingAddrModeCnt = 0;
							NoAckCount = 0;
							UART_STATUS.status = 1;
							UART_STATUS.Tick = OSTimeGet((OS_ERR *)&UART_STATUS.Tick);
							// 回调通知上层失败
							RecError code = {0x1, 0x1, 0};
							saveHplcErrLog((char *)&code, sizeof(code));
							if (CurrentLoaclPdu->fun)
								CurrentLoaclPdu->fun(CurrentLoaclPdu->SrcAddr, CurrentLoaclPdu->DstAddr, CurrentLoaclPdu->Seq, false, &LocalReceiveBuf[index], LocalReceiveLen - index, CurrentLoaclPdu->SendType, CurrentLoaclPdu->SrcTei);
							LocalReceiveLen = 0;
							if ((CurrentLoaclPdu->fun == AppHandleReadMeter) && AppUseUartFlag) // add by lxl, 20211025
							{
								AppUseUartFlag--;
							}
							LocalUartState = LOCAL_IDLE;
							if (DiffRatePdu != NULL && DiffRatePdu == CurrentLoaclPdu)
							{
								ChangeLocalUartBaud(LoacalBaud);
								DiffRatePdu = NULL;
							}
							FreeLocal(CurrentLoaclPdu);
							return;
						}
					}
#endif
					// 返回空闲状态

					if (CurrentLoaclPdu->fun)
					{
						if (LocalReceiveLen)
						{
#ifndef II_STA
							NoAckCount = 0;
#endif
							CurrentLoaclPdu->fun(CurrentLoaclPdu->SrcAddr, CurrentLoaclPdu->DstAddr, CurrentLoaclPdu->Seq, true, &LocalReceiveBuf[0], LocalReceiveLen, CurrentLoaclPdu->SendType, CurrentLoaclPdu->SrcTei);
						}
						else
						{
							RecError code = {0x1, 0x1, 0};
							saveHplcErrLog((char *)&code, sizeof(code));
							CurrentLoaclPdu->fun(CurrentLoaclPdu->SrcAddr, CurrentLoaclPdu->DstAddr, CurrentLoaclPdu->Seq, false, &LocalReceiveBuf[0], LocalReceiveLen, CurrentLoaclPdu->SendType, CurrentLoaclPdu->SrcTei);
						}
					}
					LocalReceiveLen = 0;
					if ((CurrentLoaclPdu->fun == AppHandleReadMeter) && AppUseUartFlag) // add by lxl, 20211025
					{
						AppUseUartFlag--;
					}
					LocalUartState = LOCAL_IDLE;
					if (DiffRatePdu != NULL && DiffRatePdu == CurrentLoaclPdu)
					{
						ChangeLocalUartBaud(LoacalBaud);
						DiffRatePdu = NULL;
					}
					FreeLocal(CurrentLoaclPdu);
				}
			}
		}
	}
	else
	{
		static TimeValue UartTimer;
		if (err == OS_ERR_NONE)
		{
			if (LocalReceiveLen == 0)
			{
				StartTimer(&UartTimer, 600);
			}
			u32 readLen = UartRead(&LocalReceiveBuf[LocalReceiveLen], LOCAL_BUFF_SIZE - LocalReceiveLen);
			LocalReceiveLen += readLen;
			debug_str(DEBUG_LOG_METER, "UartRead ,nData=");
			debug_hex(DEBUG_LOG_METER,LocalReceiveBuf,LocalReceiveLen);

#ifdef I_STA
			P_LMP_LINK_HEAD temp = NULL;
			if (temp = PLM_Check_Frame_3762(LocalReceiveBuf, LocalReceiveLen))
			{
				LMP_APP_INFO info;
				info.lmpLinkHead = temp;
				if (SUCCESS == LMP_App_Proc_gd(&info))
				{
					char *pAck = NULL;
					u8 err = RT_ERR_CMD_NOT_SUPPORT;
					u16 dg_len = 0;
					if (info.cur_afn == 0x23)
					{
						switch (info.curDT)
						{
						case 0xEA002301:
						{
							index = 0;
							u8 Data[9] = {0};

							Data[index++] = StaVersion.FactoryID[1];
							Data[index++] = StaVersion.FactoryID[0];
							Data[index++] = StaVersion.ChipCode[1];
							Data[index++] = StaVersion.ChipCode[0];

							// 模块版本信息
							Data[index++] = dec_to_bcd(StaVersion.VerDate.Day);
							Data[index++] = dec_to_bcd(StaVersion.VerDate.Month);
							Data[index++] = dec_to_bcd(StaVersion.VerDate.Year);
							Data[index++] = StaVersion.SofeVer[1];
							Data[index++] = StaVersion.SofeVer[0];

							dg_len = index;
							pAck = LMP_make_frame_gd_cmd(info.cur_afn, info.curDT, info.seq, Data, &dg_len, NULL, true);
							break;
						}
						}
					}
					if ((pAck == NULL) && (dg_len == 0))
					{
						dg_len = 1;
						pAck = LMP_make_frame_gd_cmd(0x00, 0xEA010002, info.seq, &err, &dg_len, NULL, true);
					}
					if (pAck)
					{
						UartWrite((u8 *)pAck, dg_len);
						free(pAck);
					}
				}
				LocalReceiveLen = 0;
				return;
			}
#endif
			u8 mac[6], mac_fre[6] = {0xDC, 0xEE, 0xEB, 0xDC, 0xBB, 0xCE};
			u8 *pdata;
			pdata = LocalReceiveBuf;
			for (index = 0; index < LocalReceiveLen; index++)
			{
				if (*pdata == 0x68)
				{
					if (Is645Frame(pdata, LocalReceiveLen - index))
					{
						break;
					}
					else if (start_uart_update)
					{
						if (Is1024Date645Frame(LocalReceiveBuf, LocalReceiveLen))
						{
							break;
						}
					}
				}
				++pdata;
			}
			if (index < LocalReceiveLen)
			{
				readLen = LocalReceiveLen - index;
#if defined(SUPPORT_21_METER) && !defined(II_STA)
				P_Dl645_Freme p645 = (P_Dl645_Freme)pdata;

				if (p645->controlCode == 0x86) // 电能表主动上报状态字
				{
					debug_str(DEBUG_LOG_APS, "EVENT_AddTolist, len:%d\r\n", p645->dataLen);
					EVENT_AddTolist(p645);
				}
				else
#endif
#ifdef UART_DEBUG
					if (OperationMaintenance((P_Dl645_Freme)pdata))
				{
				}
				else if (*((u32 *)(pdata + 10)) == 0xB1324633) // 南网附件5，5.2.2本地频段切换7E FF 13 00
#else
				if (*((u32 *)(pdata + 10)) == 0xB1324633) // 南网附件5，5.2.2本地频段切换7E FF 13 00
#endif
				{
					if (memcmp(&pdata[1], mac_fre, sizeof(mac_fre)) == 0)
					{
						if ((pdata[8] == 0x1E) &&
							(pdata[9] == 5)) // 长度
						{
							u8 fre = pdata[14] - 0x33;
							if (fre <= 2) // 频段范围
							{
								*((u32 *)(pdata + 14)) = 0x33333333; // 结果成功

								if (fre != HPLC_PHY_GetFrequence())
								{
									ResetChangeFreTimer();
									HPLC_PHY_SetFrequence(fre);
									ResetCalibrateNTB();
								}
							}
							else
							{
								*((u32 *)(pdata + 14)) = 0x34333333; // 结果失败
							}

							memcpy(&pdata[1], mac_fre, sizeof(mac));
							pdata[8] = 0x9E;
							pdata[9] = 0x08; // 长度
							*((u32 *)(pdata + 10)) = 0xB1324633;
							pdata[18] = GetCheckSum(pdata, 18);
							pdata[19] = 0x16;
							readLen = 20;
						}
						else // error
						{
							pdata[8] = 0xd1; // C
							pdata[9] = 0x1;
							pdata[10] = 0x1; // error code
							pdata[11] = GetCheckSum(pdata, 11);
							pdata[12] = 0x16;
							readLen = index + 13;
						}

						U485EnWrite(1);
						UartWrite(pdata, readLen);
						while (IsUartTxData())
						{
							BSP_OS_TimeDly(1);
						}
						while (!IsUartTxSendAllDone())
						{
							OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
						}
						U485EnWrite(0);
					}
				}
				else
				{
					memset(mac, 0xaa, sizeof(mac));
					if (memcmp(mac, &pdata[1], sizeof(mac)) != 0)
					{
						memcpy(mac, GetStaBaseAttr()->Mac, sizeof(mac));
						TransposeAddr(mac);
						if (memcmp(mac, &pdata[1], sizeof(mac)) != 0)
						{
							return;
						}
					}
					memcpy(mac, GetStaBaseAttr()->Mac, sizeof(mac));
					TransposeAddr(mac);
					memcpy(&pdata[1], mac, sizeof(mac));
					{
						if (*((u32 *)(pdata + 10)) == 0x37B43334) // 南网附件5，3.2.1扩展04 81 00 01
						{
							u8 verLen = 15; // 6字节（mac地址）+2字节（厂商代码）+2字节（版本号）+2字节（芯片代码）+3字节（版本日期）

							if ((pdata[8] == 0x11) && // 主站读
								(pdata[9] == 0x4))	  // 长度
							{
								ReadStaIdInfo();
								ReadVerInfo();

								memset(&pdata[1], 0xaa, sizeof(mac));
								pdata[8] = 0x91;
								pdata[9] = 0x13;
								*((u32 *)(pdata + 10)) = 0x37B43334;
								readLen = verLen;
								memcpy(&pdata[14], &StaChipID, readLen);

								// 厂商代码
								memcpy_swap(&pdata[20], (u8 *)StaVersion.FactoryID, 2);

								// 版本信息BCD
								memcpy_swap(&pdata[22], (u8 *)StaVersion.SofeVer, 2);

								// 芯片代码
								memcpy_swap(&pdata[24], (u8 *)StaVersion.ChipCode, 2);

								// 日期，中惠要求BCD
								pdata[26] = Hex2BcdChar(StaVersion.VerDate.Day);
								pdata[27] = Hex2BcdChar(StaVersion.VerDate.Month);
								pdata[28] = Hex2BcdChar(StaVersion.VerDate.Year);

								pdata[14 + readLen] = GetCheckSum(pdata, 14 + readLen);
								pdata[15 + readLen] = 0x16;
								readLen = 16 + readLen;
							}
							else if ((pdata[8] == 0x14) && // 主站写
									 (pdata[9] == 0x13))   // 长度
							{
								memcpy(&StaChipID, &pdata[14], verLen);
								StorageStaIdInfo();

								memset(&pdata[1], 0xaa, sizeof(mac));
								pdata[8] = 0x94;
								pdata[9] = 0x13;
								*((u32 *)(pdata + 10)) = 0x37B43334;
								readLen = verLen;
								memcpy(&pdata[14], &StaChipID, readLen);
								pdata[14 + readLen] = GetCheckSum(pdata, 14 + readLen);
								pdata[15 + readLen] = 0x16;
								readLen = 16 + readLen;
							}
							else // error
							{
								pdata[8] = 0xd1; // C
								pdata[9] = 0x1;
								pdata[10] = 0x1; // error code
								pdata[11] = GetCheckSum(pdata, 11);
								pdata[12] = 0x16;
								readLen = index + 13;
							}

							U485EnWrite(1);
							UartWrite(pdata, readLen);
							while (IsUartTxData())
							{
								BSP_OS_TimeDly(1);
							}

							while (!IsUartTxSendAllDone())
							{
								OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
							}
							U485EnWrite(0);
						}
#ifndef II_STA
#ifndef NEW_TOPO
						else if ((*((u32 *)(pdata + 10)) == 0x33370804) || (*((u32 *)(pdata + 10)) == 0x33370604))
						{
							P_Dl645_Freme pFrame = (P_Dl645_Freme)pdata;

							for (int i = 0; i < pFrame->dataLen; i++)
							{
								pFrame->dataBuf[i] -= 0x33;
							}
							extern void StartTopo(u8 func, u8 cnt, u8 interval, u8 * data, u8 datalen);
							StartTopo(pFrame->dataBuf[12], pFrame->dataBuf[13], pFrame->dataBuf[14], &pFrame->dataBuf[16], pFrame->dataBuf[15]);
							pdata[8] = 0x84;
							pdata[9] = 0x00;
							pdata[10] = GetCheckSum(pdata, 10);
							pdata[11] = 0x16;
							readLen = 12;

							U485EnWrite(1);
							UartWrite(pdata, readLen);
							while (IsUartTxData())
							{
								BSP_OS_TimeDly(1);
							}
							while (!IsUartTxSendAllDone())
							{
								OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
							}
							U485EnWrite(0);
						}
#else
						else if (*((u32 *)(pdata + 10)) == 0x3c3b343c)
						{
							extern u32 remain_tick;
							P_Dl645_Freme pFrame = (P_Dl645_Freme)pdata;

							for (int i = 0; i < pFrame->dataLen; i++)
							{
								pFrame->dataBuf[i] -= 0x33;
							}
							u8 device = pFrame->dataBuf[4];
							struct tpc
							{
								u8 sec;
								u8 min;
								u8 hour;
								u8 day;
								u8 mon;
								u8 year;
								u16 bitMs;
								u32 fre : 24;
								u32 len : 8;
								u8 data[0];
							} *topo_config;
							topo_config = (struct tpc *)&pFrame->dataBuf[5];
							u16 pwm_h = *((u16 *)(&(topo_config->data[topo_config->len])));		// u16 pwm_h = topo_config->data[topo_config->len];//
							u16 pwm_l = *((u16 *)(&(topo_config->data[topo_config->len + 2]))); // u16 pwm_l = topo_config->data[topo_config->len+2];//

							u8 err = 0x01;
							u32 sec1900 = 0xffffffff;
							device = device; // 下面如果不用device解决警告
							if (remain_tick == 0)
							{
								// 产测出发topo立即发送
								if (topo_config->bitMs > 1200)
								{
									topo_config->bitMs = 600;
								}

								StartTopo(sec1900, pwm_l, pwm_h, topo_config->bitMs, (u8 *)topo_config->data, topo_config->len);

								pdata[8] = 0x94;
								pdata[9] = 0x00;
								pdata[10] = GetCheckSum(pdata, 10);
								pdata[11] = 0x16;
								readLen = 12;
							}
							else
							{
								err = remain_tick / 1000;

								pdata[8] = 0xD4;
								pdata[9] = 0x01;
								pdata[10] = err + 0x33;
								pdata[11] = GetCheckSum(pdata, 11);
								pdata[12] = 0x16;
								readLen = 13;
							}

							U485EnWrite(1);
							UartWrite(pdata, readLen);
							while (IsUartTxData())
							{
								BSP_OS_TimeDly(1);
							}
							while (!IsUartTxSendAllDone())
							{
								OS_ERR err;
								OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
							}
							U485EnWrite(0);
						}
#endif
#endif
						else if (pdata[8] == 0x1F) // 查询扩展控制码
						{
							if ((pdata[9] == 10) && (pdata[10] == 0x2D)) // 设置厂商代码和芯片代码
							{
								// 厂商代码
								memcpy_swap((u8 *)StaVersion.FactoryID, &pdata[11], 2);

								// 芯片代码
								memcpy_swap((u8 *)StaVersion.ChipCode, &pdata[13], 2);

								// 版本日期
								StaVersion.VerDate.Day = bcd_to_dec(pdata[15]);
								StaVersion.VerDate.Month = bcd_to_dec(pdata[16]);
								StaVersion.VerDate.Year = bcd_to_dec(pdata[17]);

								// 版本号
								memcpy_swap((u8 *)StaVersion.SofeVer, &pdata[18], 2);

								StorageVerInfo();

								// 上行回复
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 10;	  // 长度
								pdata[10] = 0x2D; // 功能码

								memcpy_swap(&pdata[11], (u8 *)StaVersion.FactoryID, 2);
								memcpy_swap(&pdata[13], (u8 *)StaVersion.ChipCode, 2);
								pdata[15] = Hex2BcdChar(StaVersion.VerDate.Day);
								pdata[16] = Hex2BcdChar(StaVersion.VerDate.Month);
								pdata[17] = Hex2BcdChar(StaVersion.VerDate.Year);
								memcpy_swap(&pdata[18], (u8 *)StaVersion.SofeVer, 2);
								pdata[20] = GetCheckSum(pdata, 20);
								pdata[21] = 0x16;

								readLen = 22;

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
							else if ((pdata[9] == 1) && (pdata[10] == 0x3A)) // 查询过零信息长度和功能码
							{
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 18;	  // 长度
								pdata[10] = 0x3A; // 功能码
								pdata[11] = ZERO_EDGE_AREA;
								pdata[12] = (myDevicType == 7 ? 2 : 1); // 设备类型

								readLen = 13;
								for (u8 i = 0; i < 3; i++)
								{
									pdata[readLen++] = zero_check_pass[i];

									memset(&pdata[readLen], 0x00, 4);
									if (zero_check_pass[i])
									{
										memcpy(&pdata[readLen], &zeroNtbDiff[i], 4);
									}
									readLen += 4;
								}

								pdata[readLen] = GetCheckSum(pdata, readLen);
								readLen++;
								pdata[readLen++] = 0x16;

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
#ifndef __DEBUG_MODE
							else if ((pdata[9] == 1) && (pdata[10] == 0x3B)) // 查询内部版本信息
							{
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 6;	  // 长度
								pdata[10] = 0x3B; // 功能码

								pdata[11] = Hex2BcdChar(SLG_COMMIT_DATE);
								pdata[12] = Hex2BcdChar(SLG_COMMIT_MON);
								pdata[13] = Hex2BcdChar(SLG_COMMIT_YEAR % 100);
								pdata[14] = (SLG_COMMIT_HASH >> 12) & 0xff;
								pdata[15] = (SLG_COMMIT_HASH >> 20) & 0xff;
								pdata[16] = GetCheckSum(pdata, 16);
								pdata[17] = 0x16;

								readLen = 18;

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
#endif
							else if (pdata[10] == 0x3C) // 串口波特率
							{
								if (pdata[9] == 1) // 查询
								{
									ReadStaFixPara();

									pdata[8] = 0x9f;  // 应答帧控制码
									pdata[9] = 2;	  // 长度
									pdata[10] = 0x3C; // 功能码
									pdata[11] = StaFixPara.baud;
									pdata[12] = GetCheckSum(pdata, 12);
									pdata[13] = 0x16;
									readLen = 14;
								}
								else if (pdata[9] == 2) // 设置
								{
									u8 baud = pdata[11];

									if (baud <= 5)
									{
										SetBaud(baud);
										StorageStaFixPara();

										pdata[8] = 0x9f;  // 应答帧控制码
										pdata[9] = 2;	  // 长度
										pdata[10] = 0x3C; // 功能码
										pdata[11] = baud;
										pdata[12] = GetCheckSum(pdata, 12);
										pdata[13] = 0x16;
										readLen = 14;
									}
									else
									{
										pdata[8] = 0xd1; // C
										pdata[9] = 0x1;
										pdata[10] = 0x1; // error code
										pdata[11] = GetCheckSum(pdata, 11);
										pdata[12] = 0x16;
										readLen = 13;
									}
								}

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
							else if (pdata[10] == 0x3D) // LED闪灯模式，0->原方案，1->南网下一代电表方案
							{
								if (pdata[9] == 1) // 查询
								{
									ReadStaFixPara();

									pdata[8] = 0x9f;  // 应答帧控制码
									pdata[9] = 2;	  // 长度
									pdata[10] = 0x3D; // 功能码
									pdata[11] = StaFixPara.ledMode;
									pdata[12] = GetCheckSum(pdata, 12);
									pdata[13] = 0x16;
									readLen = 14;
								}
								else if (pdata[9] == 2) // 设置
								{
									u8 led_mode = pdata[11];

									if (led_mode <= 1)
									{
										StaFixPara.ledMode = led_mode;
										StorageStaFixPara();

										pdata[8] = 0x9f;  // 应答帧控制码
										pdata[9] = 2;	  // 长度
										pdata[10] = 0x3D; // 功能码
										pdata[11] = led_mode;
										pdata[12] = GetCheckSum(pdata, 12);
										pdata[13] = 0x16;
										readLen = 14;
									}
									else
									{
										pdata[8] = 0xd1; // C
										pdata[9] = 0x1;
										pdata[10] = 0x1; // error code
										pdata[11] = GetCheckSum(pdata, 11);
										pdata[12] = 0x16;
										readLen = 13;
									}
								}

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
							else if ((pdata[9] == 1) && (pdata[10] == 0x3E)) // 查询发射功率
							{
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 5;	  // 长度
								pdata[10] = 0x3E; // 功能码

								ReadStaFixPara();

								memset(&pdata[11], 0x00, 4);
								if (StaFixPara.txPowerMode[0] != 0)
								{
									memcpy(&pdata[11], StaFixPara.txPower, 4);
								}

								pdata[15] = GetCheckSum(pdata, 15);
								pdata[16] = 0x16;
								readLen = 17;

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
							else if ((pdata[9] == 5) && (pdata[10] == 0x3F)) // 设置发射功率
							{
								u8 valid_count = 0;

								for (u8 i = 0; i < 4; i++)
								{
									if (pdata[11 + i] == 0)
									{
										valid_count += 2;
									}
									else if (pdata[11 + i] <= 13) // 204&205 最大功率13
									{
										valid_count += 3;
									}
								}

								if ((valid_count == 8) || (valid_count == 12)) // 数值全部有效，数值全0或者全部在范围之内
								{
									if (memIsHex(&pdata[11], 0x00, 4)) // 数值全0
									{
										StaFixPara.txPowerMode[0] = 0; // 发射功率自动调整模式
										memset(StaFixPara.txPower, 0x00, 4);
									}
									else
									{
										StaFixPara.txPowerMode[0] = 1; // 发射功率固定模式
										memcpy(StaFixPara.txPower, &pdata[11], 4);
									}

									StorageStaFixPara();
									StaTxPowerProcess();
									u8 appModel = GetAppCurrentModel();
									if (appModel == APP_TEST_NORMAL) // 正常模式
									{
										BPLC_SetTxGain(HPLC_ChlPower[HPLCCurrentFreq]);
									}

									// 上行回复
									pdata[8] = 0x9f;  // 应答帧控制码
									pdata[9] = 5;	  // 长度
									pdata[10] = 0x3F; // 功能码
									memcpy(&pdata[11], StaFixPara.txPower, 4);
									pdata[15] = GetCheckSum(pdata, 15);
									pdata[16] = 0x16;

									readLen = 17;
								}
								else
								{
									pdata[8] = 0xd1; // C
									pdata[9] = 0x1;
									pdata[10] = 0x1; // error code
									pdata[11] = GetCheckSum(pdata, 11);
									pdata[12] = 0x16;
									readLen = 13;
								}

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
							else if ((pdata[9] == 1) && (pdata[10] == 0x30)) // 查询无线发射功率
							{
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 3; //长度
								pdata[10] = 0x30; // 功能码

								ReadStaFixPara();
								memset(&pdata[11], 0, sizeof(StaFixPara.hrfTxPower));
								if (StaFixPara.txPowerMode[1] != 0)
								{
									memcpy(&pdata[11], StaFixPara.hrfTxPower, sizeof(StaFixPara.hrfTxPower));
								}

								pdata[13] = GetCheckSum(pdata, 13);
								pdata[14] = 0x16;
								readLen = 15;
								
								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
							else if ((pdata[9] == 3) && (pdata[10] == 0x31)) //设置无线发射功率
							{
								if (pdata[11] == 0 && pdata[12] == 0) //自动调节
								{
									StaFixPara.txPowerMode[1] = 0;
									memset(StaFixPara.hrfTxPower, 0, sizeof(StaFixPara.hrfTxPower));
									pdata[8] = 0x9f; //应答帧控制码
									pdata[9] = 3; //长度
									pdata[10] = 0x31; //功能码
									memcpy(&pdata[11], StaFixPara.hrfTxPower, sizeof(StaFixPara.hrfTxPower));
									pdata[13] = GetCheckSum(pdata, 13);
									pdata[14] = 0x16;
									readLen = 15;
								}
								#if defined(ZB205_CHIP)
								else if (pdata[11] <= 15) //数值有效
								#else
								else if (pdata[11] <= 31) //数值有效
								#endif
									{
										StaFixPara.txPowerMode[1] = 1;
										memcpy(StaFixPara.hrfTxPower, &pdata[11], sizeof(StaFixPara.hrfTxPower));
										StorageStaFixPara();
										StaTxPowerProcess();
										if (GetAppCurrentModel() == APP_TEST_NORMAL) //正常模式
										{
											HRF_SetTxGain(HRF_ChlPower[(HRFCurrentIndex >> 8) & 0x1]);
										}
										//上行回复
										pdata[8] = 0x9f; //应答帧控制码
										pdata[9] = 3; //长度
										pdata[10] = 0x31; //功能码
										memcpy(&pdata[11], StaFixPara.hrfTxPower, sizeof(StaFixPara.hrfTxPower));
										pdata[13] = GetCheckSum(pdata, 13);
										pdata[14] = 0x16;
										readLen = 15;
									}
									else
									{
										pdata[8] = 0xd1; //C
										pdata[9] = 0x1;
										pdata[10] = 0x1; //error code
										pdata[11] = GetCheckSum(pdata, 11);
										pdata[12] = 0x16;
										readLen = 13;
									}
								
								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
							else if ((pdata[9] == 0x3) && (pdata[10] == 0x33))//设置HRF默认频段
							{
								u8 channel = pdata[12];
								u8 option = pdata[11];
								if ((option < 4) && channel < 0xff)
								{
									HRFCurrentIndex = option << 8 | channel;
									HRF_PHY_SetChannel(channel, option); //暂定等待正确的帧再进行修改
									//收到频段切换的帧令hrf的频段停留在本频段再长时间的timer方便测试机台测试
									HrfSetExtIndex(HRFCurrentIndex, 1);
									SetLastRFchannel(HRFCurrentIndex);
									StorageStaParaInfo();
									pdata[8] = 0x9f; //应答帧控制码
									pdata[9] = 3; //长度
									pdata[10] = 0x33; //功能码
									pdata[11] = option;
									pdata[12] = channel;
									pdata[13] = GetCheckSum(pdata, 13);
									pdata[14] = 0x16;
									readLen = 15;
								}
								else
								{
									pdata[8] = 0xd1; //C
									pdata[9] = 0x1;
									pdata[10] = 0x1; //error code
									pdata[11] = GetCheckSum(pdata, 11);
									pdata[12] = 0x16;
									readLen = 13;
								}
								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);

							}
							else if ((pdata[9] == 1) && (pdata[10] == 0x33)) //查询HRF默认频段
							{
								pdata[8] = 0x9f; //应答帧控制码
								pdata[9] = 3; //长度
								pdata[10] = 0x33; //功能码
								pdata[11] = StaSysPara.LastRFChannel >> 8;
								pdata[12] = StaSysPara.LastRFChannel & 0xff;
								pdata[13] = GetCheckSum(pdata, 13);
								pdata[14] = 0x16;
								readLen = 15;
								
								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);

							}
							else if ((pdata[9] == 1) && (pdata[10] == 0xA0)) // 查询wafer ID
							{
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 9;	  // 长度
								pdata[10] = 0xA0; // 功能码

								memset(&pdata[11], 0x00, 8);
								BPLC_GetWaferID(&pdata[11]);

								pdata[19] = GetCheckSum(pdata, 19);
								pdata[20] = 0x16;
								readLen = 21;

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
#ifndef II_STA
							else if ((pdata[9] == 1) && (pdata[10] == 0x40)) // 打开1Hz输出
							{
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 1;	  // 长度
								pdata[10] = 0x40; // 功能码

								Open1Hz();

								pdata[11] = GetCheckSum(pdata, 11);
								pdata[12] = 0x16;
								readLen = 13;

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
							else if ((pdata[9] == 1) && (pdata[10] == 0x41)) // 关闭1Hz输出
							{
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 1;	  // 长度
								pdata[10] = 0x41; // 功能码

								Close1Hz();
								InsertOpen();

								pdata[11] = GetCheckSum(pdata, 11);
								pdata[12] = 0x16;
								readLen = 13;

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
#endif
							else if ((pdata[9] == 1) && (pdata[10] == 0x42)) // 查询抄控器发送的产测命令接收rssi
							{
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 5;	  // 长度
								pdata[10] = 0x42; // 功能码

								*((s32 *)(pdata + 11)) = factoryTestRecvRssi;

								pdata[15] = GetCheckSum(pdata, 15);
								pdata[16] = 0x16;
								readLen = 17;

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
							else if ((pdata[9] == 1) && (pdata[10] == 0x44)) // 查询南网资产编码
							{
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 25;	  // 长度
								pdata[10] = 0x44; // 功能码

								ReadStaIdInfo();

								memset(&pdata[11], 0x00, 24);
								memcpy(&pdata[11], StaChipID.assetsCode, 24);

								pdata[35] = GetCheckSum(pdata, 35);
								pdata[36] = 0x16;
								readLen = 37;

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
							else if ((pdata[9] == 25) && (pdata[10] == 0x43)) // 设置南网资产编码，大端录入
							{
								memcpy(StaChipID.assetsCode, &pdata[11], 24);
								StorageStaIdInfo();

								// 上行回复
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 25;	  // 长度
								pdata[10] = 0x43; // 功能码
								memcpy(&pdata[11], StaChipID.assetsCode, 24);
								pdata[35] = GetCheckSum(pdata, 35);
								pdata[36] = 0x16;
								readLen = 37;

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
							else if ((pdata[9] == 1) && (pdata[10] == 0x45)) // 查询南网扩展版本信息（资产信息使用）
							{
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 18;	  // 长度
								pdata[10] = 0x45; // 功能码

								ReadExtVerInfo();

								// 模块硬件版本信息
								memcpy_swap(&pdata[11], (u8 *)StaExtVersion.ModuleHardVer, 2);
								pdata[13] = dec_to_bcd(StaExtVersion.ModuleHardVerDate.Day);
								pdata[14] = dec_to_bcd(StaExtVersion.ModuleHardVerDate.Month);
								pdata[15] = dec_to_bcd(StaExtVersion.ModuleHardVerDate.Year);

								// 芯片硬件版本信息
								memcpy_swap(&pdata[16], (u8 *)StaExtVersion.ChipHardVer, 2);
								pdata[18] = dec_to_bcd(StaExtVersion.ChipHardVerDate.Day);
								pdata[19] = dec_to_bcd(StaExtVersion.ChipHardVerDate.Month);
								pdata[20] = dec_to_bcd(StaExtVersion.ChipHardVerDate.Year);

								// 芯片软件版本信息
								memcpy_swap(&pdata[21], (u8 *)StaExtVersion.ChipSoftVer, 2);
								pdata[23] = dec_to_bcd(StaExtVersion.ChipSofVerDate.Day);
								pdata[24] = dec_to_bcd(StaExtVersion.ChipSofVerDate.Month);
								pdata[25] = dec_to_bcd(StaExtVersion.ChipSofVerDate.Year);

								// 应用程序版本号
								memcpy_swap(&pdata[26], (u8 *)StaExtVersion.AppVer, 2);

								pdata[28] = GetCheckSum(pdata, 28);
								pdata[29] = 0x16;
								readLen = 30;

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
							else if ((pdata[9] == 18) && (pdata[10] == 0x46)) // 设置南网扩展版本信息（资产信息使用）
							{
								// 模块硬件版本信息
								memcpy_swap((u8 *)StaExtVersion.ModuleHardVer, &pdata[11], 2);
								StaExtVersion.ModuleHardVerDate.Day = bcd_to_dec(pdata[13]);
								StaExtVersion.ModuleHardVerDate.Month = bcd_to_dec(pdata[14]);
								StaExtVersion.ModuleHardVerDate.Year = bcd_to_dec(pdata[15]);

								// 芯片硬件版本信息
								memcpy_swap((u8 *)StaExtVersion.ChipHardVer, &pdata[16], 2);
								StaExtVersion.ChipHardVerDate.Day = bcd_to_dec(pdata[18]);
								StaExtVersion.ChipHardVerDate.Month = bcd_to_dec(pdata[19]);
								StaExtVersion.ChipHardVerDate.Year = bcd_to_dec(pdata[20]);

								// 芯片软件版本信息
								memcpy_swap((u8 *)StaExtVersion.ChipSoftVer, &pdata[21], 2);
								StaExtVersion.ChipSofVerDate.Day = bcd_to_dec(pdata[23]);
								StaExtVersion.ChipSofVerDate.Month = bcd_to_dec(pdata[24]);
								StaExtVersion.ChipSofVerDate.Year = bcd_to_dec(pdata[25]);

								// 应用程序版本号
								memcpy_swap((u8 *)StaExtVersion.AppVer, &pdata[26], 2);

								StorageExtVerInfo();

								// 上行回复
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 18;	  // 长度
								pdata[10] = 0x46; // 功能码

								memcpy_swap(&pdata[11], (u8 *)StaExtVersion.ModuleHardVer, 2);
								pdata[13] = dec_to_bcd(StaExtVersion.ModuleHardVerDate.Day);
								pdata[14] = dec_to_bcd(StaExtVersion.ModuleHardVerDate.Month);
								pdata[15] = dec_to_bcd(StaExtVersion.ModuleHardVerDate.Year);
								memcpy_swap(&pdata[16], (u8 *)StaExtVersion.ChipHardVer, 2);
								pdata[18] = dec_to_bcd(StaExtVersion.ChipHardVerDate.Day);
								pdata[19] = dec_to_bcd(StaExtVersion.ChipHardVerDate.Month);
								pdata[20] = dec_to_bcd(StaExtVersion.ChipHardVerDate.Year);
								memcpy_swap(&pdata[21], (u8 *)StaExtVersion.ChipSoftVer, 2);
								pdata[23] = dec_to_bcd(StaExtVersion.ChipSofVerDate.Day);
								pdata[24] = dec_to_bcd(StaExtVersion.ChipSofVerDate.Month);
								pdata[25] = dec_to_bcd(StaExtVersion.ChipSofVerDate.Year);
								memcpy_swap(&pdata[26], (u8 *)StaExtVersion.AppVer, 2);
								pdata[28] = GetCheckSum(pdata, 28);
								pdata[29] = 0x16;
								readLen = 30;

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
							else if ((pdata[9] == 1) && (pdata[10] == 0x47)) // 查询抄控器校准的频偏值
							{
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 3;	  // 长度
								pdata[10] = 0x47; // 功能码

								ReadPllTrim();
								s32 temp_PLL_Trimming = PLL_Trimming * 10 / 16; // PLL_Trimming单位1/16ppm，转换为单位0.1ppm
								if (temp_PLL_Trimming == 0 && PLL_Vaild)
								{
									*((s16 *)(pdata + 11)) = 1;
								}
								else
								{
									*((s16 *)(pdata + 11)) = temp_PLL_Trimming;
								}

								pdata[13] = GetCheckSum(pdata, 13);
								pdata[14] = 0x16;
								readLen = 15;

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
							else if ((pdata[9] == 1) && (pdata[10] == 0x48)) // 二采自测
							{
								pdata[8] = 0x9f;  // 应答帧控制码
								pdata[9] = 2;	  // 长度
								pdata[10] = 0x48; // 功能码

								pdata[11] = UartIrTest(mac);

								pdata[12] = GetCheckSum(pdata, 12);
								pdata[13] = 0x16;
								readLen = 14;

								U485EnWrite(1);
								UartWrite(pdata, readLen);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
							}
						}
						else if (pdata[8] == 0x10) // 用于升级 升级程序与hplc升级程序完全独立不使用任何 hplc升级的代码
						{
							uart_updata *pup = (uart_updata *)&pdata[9];
							static u32 file_size = 0;
							static u32 file_crc = 0;
							static u32 start_addr = 0;
							static u32 code_info_addr = 0;
							static u32 erase_size = 0;
							static u32 version;
							if (pup->para == 0x55) // 开始升级信息
							{
								file_size = pup->block;
								file_crc = pup->crc;
								start_uart_update = 1;
								erase_size = 0;

								UserSection_Type *p = CurrentCodeSection;
								code_info_addr = FLASH_USER_FEATURE1_ADDRESS + OldCodeSection * DATA_FLASH_SECTION_SIZE;
								start_addr = FLASH_CODE1_ADDRESS + OldCodeSection * FLASH_CODE1_SIZE;
								if (p->FeatureCode == USER_FEATURE_CODE)
								{
									version = p->Version + 1;
								}
								else
								{
									version = 1;
								}

								debug_str(DEBUG_LOG_UPDATA, "LocalUpdateStart, CodeSection:%d, file_size:%d\r\n", OldCodeSection + 1, file_size);

								DataFlashInit();
								Unprotection1MFlash();
								EraseFlash(code_info_addr, FLASH_USER_FEATURE1_SIZE);
								Protection1MFlash();
								DataFlashClose();
								pup->block = 0;
								pup->crc = 0;
								pup->para = 0x55;
								pup->len = sizeof(uart_updata) - 1;
								memset(pup->data, 0, sizeof(pup->data));
								pdata[21] = GetCheckSum(pdata, 21);
								pdata[22] = 0x16;

								U485EnWrite(1);
								UartWrite(pdata, 23);
								while (IsUartTxData())
								{
									BSP_OS_TimeDly(1);
								}
								while (!IsUartTxSendAllDone())
								{
									OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
								}
								U485EnWrite(0);
								BSP_OS_TimeDly(100);
								ChangeLocalUartBaud(200000);
								LoacalBaud = 200000;
							}
							if (start_uart_update)
							{
								if (pup->para == 0xaa) // 结束升级
								{
									start_uart_update = 0;
									u32 crc = 0xffffffff;
									u32 checksum = 0;

									start_addr = FLASH_CODE1_ADDRESS + OldCodeSection * FLASH_CODE1_SIZE;
									erase_size = start_addr + file_size;
									while (start_addr < erase_size)
									{
										u32 copy_size = erase_size - start_addr;
										if (copy_size > 1024)
										{
											copy_size = 1024;
										}
										memcpy(mem_ptr, (void *)start_addr, copy_size);
										crc = CommonGetCrc32Init(crc, (u8 *)mem_ptr, copy_size);
										start_addr += copy_size;
										for (int i = 0; i < copy_size / 4; i++)
										{
											checksum += mem_ptr[i];
										}
									}
									crc ^= 0xffffffff;

									if (crc == file_crc)
									{
										UserSection_Type codeInfo;
										debug_str(DEBUG_LOG_UPDATA, "LocalUpdateOver, CRC pass\r\n");

										codeInfo.CodeSpiFlashStartAddr = FLASH_CODE1_ADDRESS + OldCodeSection * FLASH_CODE1_SIZE - FLASH_START_ADDR;
										codeInfo.CodeSize = file_size;
#if defined(ZB204_CHIP)
										codeInfo.CopyToRamFlg = 1;
#endif
										codeInfo.FeatureCode = USER_FEATURE_CODE;
										codeInfo.Version = version;
										codeInfo.CodeCheckSum = checksum;
										codeInfo.UserSectionCheckSum = 0;
										for (int i = 0; i < sizeof(codeInfo) / 4 - 1; i++)
										{
											codeInfo.UserSectionCheckSum += ((u32 *)&codeInfo)[i];
										}
										DataFlashInit();
										Unprotection1MFlash();
										WriteFlash(code_info_addr, (u8 *)&codeInfo, sizeof(codeInfo));
										Protection1MFlash();
										DataFlashClose();

										SYS_UP_ReWrite((UPDATA_PARA *)(codeInfo.CodeSpiFlashStartAddr + FLASH_START_ADDR + codeInfo.CodeSize - sizeof(UPDATA_PARA)));
									}
									else
									{
										debug_str(DEBUG_LOG_UPDATA, "LocalUpdateOver, CRC fail\r\n");
									}

									pup->para = 0xaa;
									pup->block = 0;
									pup->crc = crc;
									pup->len = sizeof(uart_updata) - 1;
									memset(pup->data, 0, sizeof(pup->data));
									pdata[21] = GetCheckSum(pdata, 21);
									pdata[22] = 0x16;

									U485EnWrite(1);
									UartWrite(pdata, 23);
									while (IsUartTxData())
									{
										BSP_OS_TimeDly(1);
									}
									while (!IsUartTxSendAllDone())
									{
										OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
									}
									U485EnWrite(0);
									BSP_OS_TimeDly(100);
									REBOOT_SYSTEM();
								}
								else if (pup->para == 0) // 传输文件
								{
									debug_str(DEBUG_LOG_UPDATA, ".");
									if (((pup->block + 1) * 1024) >= erase_size)
									{
										DataFlashInit();
										Unprotection1MFlash();
										EraseFlash(erase_size + start_addr, DATA_FLASH_SECTION_SIZE);
										Protection1MFlash();
										DataFlashClose();
										erase_size += DATA_FLASH_SECTION_SIZE;
									}
									memcpy(mem_ptr, pup->data, 1024);
									u32 write_addr = start_addr + pup->block * 1024;
									pup->block = 0;
									pup->crc = 0;
									pup->para = 0x55;
									pup->len = sizeof(uart_updata) - 1;
									memset(pup->data, 0, sizeof(pup->data));
									pdata[21] = GetCheckSum(pdata, 21);
									pdata[22] = 0x16;

									U485EnWrite(1);
									UartWrite(pdata, 23);
									DataFlashInit();
									Unprotection1MFlash();
									WriteFlash(write_addr, (u8 *)mem_ptr, 1024);
									Protection1MFlash();
									DataFlashClose();
									while (IsUartTxData())
									{
										BSP_OS_TimeDly(1);
									}
									while (!IsUartTxSendAllDone())
									{
										OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
									}
									U485EnWrite(0);
								}
							}
						}
					}
				}
				LocalReceiveLen = 0;
			}
		}
		else if (LocalReceiveLen)
		{
			if (TimeOut(&UartTimer))
			{
				if (start_uart_update)
				{
					LocalReceiveBuf[0] = 0x68;
					memcpy(&LocalReceiveBuf[1], GetStaBaseAttr()->Mac, sizeof(LONG_ADDR_LEN));
					TransposeAddr(&LocalReceiveBuf[1]);
					LocalReceiveBuf[7] = 0x68;
					LocalReceiveBuf[8] = 0x10;
					uart_updata *pup = (uart_updata *)&LocalReceiveBuf[9];
					pup->para = 0xaa;
					pup->block = 0;
					pup->crc = 1;
					pup->len = sizeof(uart_updata) - 1;
					memset(pup->data, 0, sizeof(pup->data));
					LocalReceiveBuf[21] = GetCheckSum(LocalReceiveBuf, 21);
					LocalReceiveBuf[22] = 0x16;

					U485EnWrite(1);
					UartWrite(LocalReceiveBuf, 23);
					while (IsUartTxData())
					{
						BSP_OS_TimeDly(1);
					}
					while (!IsUartTxSendAllDone())
					{
						OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
					}
					U485EnWrite(0);
					BSP_OS_TimeDly(1);
				}
				LocalReceiveLen = 0;
			}
		}
	}
}

void sendUartDiffRate(u16 seq, u8 *data, u16 len, u32 rate)
{

	OS_ERR err;
	if (DiffRatePdu != NULL)
	{
		return;
	}
	DiffRatePdu = MallocLocal();
	if (DiffRatePdu == NULL)
		return;
	memset(DiffRatePdu->SrcAddr, 0xff, 6);
	memset(DiffRatePdu->DstAddr, 0xff, 6);
	DiffRatePdu->Timeout = 18;
	DiffRatePdu->Seq = seq;
	DiffRatePdu->SendType = 0;
	DiffRatePdu->SrcTei = rate;
	if (data && len <= MAX_LOCAL_LEN)
		memcpy(DiffRatePdu->Data, data, len);
	DiffRatePdu->Size = len;
	DiffRatePdu->fun = AppHandleReadModule;

	OSQPost(&LocalDataQueue, DiffRatePdu, sizeof(LOCAL_PARAM), OS_OPT_POST_FIFO | OS_OPT_POST_ALL, &err);
	if (err != OS_ERR_NONE)
	{
		FreeLocal(DiffRatePdu);
		DiffRatePdu = NULL;
		return;
	}
	return;
}

extern TimeValue LastCCOMacTimeoutTimer;
bool UartBusy(void)
{
	if (LocalDataQueue.MsgQ.NbrEntries)
	{
		return true;
	}
	return false;
}
// 应用层本地处理任务
void AppLocalTask(void *arg)
{
	#if 0
	OS_ERR err;
	OS_MSG_SIZE msgSize;
	LocalInit();
	OSTimeDlyHMSM(0, 0, 0, FIRST_READ_ADDR_DELAY, OS_OPT_TIME_DLY, &err);
	while (1)
	{
		if (LocalUartState == LOCAL_IDLE && GetAppCurrentModel() == APP_TEST_NORMAL)
		{
#ifndef II_STA
			if (!IsBindAddr)
			{
				BindingAddr();
				StartTimer(&bindTimer, 2000);
#ifdef CHECK_LAST_CCO
				// 没有获取到电表地址时，更新匹配上一次入网CCO的定时器
				StartTimer(&LastCCOMacTimeoutTimer, CHECK_LAST_CCO_TIMEOUT);
#endif
				OSTimeDlyHMSM(0, 0, 0, 20, OS_OPT_TIME_DLY, &err);
			}
			else
#endif
			{
				const u32 WaitLocalMs = 20 / (1000 / OSCfg_TickRate_Hz);
				CurrentLoaclPdu = OSQPend(&LocalDataQueue, WaitLocalMs, OS_OPT_PEND_BLOCKING, &msgSize, 0, &err);
				if (err == OS_ERR_NONE)
				{
					bool isBroad = false;
					// 清空发送队列
					while (IsUartTxData())
					{
						OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
					}
					// 清空读取队列
					u32 len = UartRead(&LocalReceiveBuf[0], LOCAL_BUFF_SIZE);
					LocalReceiveLen = 0;
					U485EnWrite(1);

					OSSemSet(&ReceiveComplateSEM, 0, &err);
#ifndef STA_I
					if (CurrentLoaclPdu == DiffRatePdu)
					{
						if (DiffRatePdu->SrcTei != 0)
						{
							ChangeLocalUartBaud(DiffRatePdu->SrcTei); // 此时SrcTei用来存储要更改的波特率
						}
						else
						{
							// 如果Baud为零, 需要根据MAC地址去找对应的波特率
							// 在 SEARCH_METER_FLAG.table[] 找
							u8 dstAddr[6] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};
							bool checkCsmState = false;
							// 判断帧是否合法,
							//   如果合法就把地址复制到dstAddr, 并checkCsmState=true
							for (int index = 0; index < CurrentLoaclPdu->Size; index++)
							{
								if (CurrentLoaclPdu->Data[index] == 0x68)
								{
									if (Is645Frame(&(CurrentLoaclPdu->Data[index]), CurrentLoaclPdu->Size - index))
									{
										memcpy(dstAddr, &(CurrentLoaclPdu->Data[index + 1]), 6);
										checkCsmState = true;
										break;
									}
								}
							}
							if (checkCsmState)
							{
								debug_str(DEBUG_LOG_INFO, "DiffRatePdu, dstmac:%02x%02x%02x%02x%02x%02x\r\n",
										  dstAddr[5], dstAddr[4], dstAddr[3],
										  dstAddr[2], dstAddr[1], dstAddr[0]);
								if (memIsHex(dstAddr, 0x99, 6))
								{
									// 如果是合法帧, 并且是广播帧
									isBroad = true;
								}
								else
								{
									// 普通帧
									u32 meterBaud = FindBaudInSEARCH_METER_FLAG_table(dstAddr);
									debug_str(DEBUG_LOG_INFO, "DiffRatePdu, baud:%d\r\n", meterBaud);
									ChangeLocalUartBaud_t(meterBaud);
								}
							}
						}
					}

					else
#endif
					{
#if defined(I_STA)
						if (CurrentLoaclPdu->Baud == 0) // 需要在发送时修改发送内容
						{
							//							u16 len = CurrentLoaclPdu->Size;
							//							memmove(&CurrentLoaclPdu->Data[2],CurrentLoaclPdu->Data,len);
							//							*(u16*)CurrentLoaclPdu->Data=len;
							//							len+=2;
							//							char *p = LMP_make_frame_gd_cmd(0x22, 0xEA042201, I_STA_SEQ++, CurrentLoaclPdu->Data, &len, NULL, false);
							//							memcpy(CurrentLoaclPdu->Data, p, len);
							//							CurrentLoaclPdu->Size = len;
							//							free(p);
						}
#elif defined(II_STA)
						// DiffRatePdu->SrcTei 是 sendUartDiffRate()调用的,
						// 为了兼容原来的程序, DiffRatePdu 作为特列, 应该当作优先判断项
						// 如果不是, 才可执行本语句
						//
						if (CurrentLoaclPdu->Baud & 0x80000000)
						{
							UartNeedProcErr(1);
							CurrentLoaclPdu->Baud &= ~0x80000000;
						}
						else
						{
							UartNeedProcErr(0);
						}
						if (CurrentLoaclPdu->Baud != 0)
						{
							// 如果Baud不为零
							if (BaudValidityCheck(CurrentLoaclPdu->Baud)) // 检查Baud参数合法性
							{
								;
							}
							else
							{
								// 不合法就给默认波特率
								CurrentLoaclPdu->Baud = baudTable_t[1];
							}
						}
						else
						{
							// 如果Baud为零, 需要根据MAC地址去找对应的波特率
							// 在 SEARCH_METER_FLAG.table[] 找
							u8 dstAddr[6] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};
							bool checkCsmState = false;
							// 判断帧是否合法,
							//   如果合法就把地址复制到dstAddr, 并checkCsmState=true
							for (int index = 0; index < CurrentLoaclPdu->Size; index++)
							{
								if (CurrentLoaclPdu->Data[index] == 0x68)
								{
									if (Is645Frame(&(CurrentLoaclPdu->Data[index]), CurrentLoaclPdu->Size - index))
									{
										memcpy(dstAddr, &(CurrentLoaclPdu->Data[index + 1]), 6);
										checkCsmState = true;
										break;
									}
								}
							}
							if (checkCsmState)
							{
								if (memIsHex(dstAddr, 0x99, 6))
								{
									// 如果是合法帧, 并且是广播帧
									isBroad = true;
									CurrentLoaclPdu->Baud = baudTable_t[0];
								}
								else
								{
									// 普通帧
									CurrentLoaclPdu->Baud = FindBaudInSEARCH_METER_FLAG_table(dstAddr);
								}
							}
							else
							{
								// 如果帧不合法, 给默认波特率
								CurrentLoaclPdu->Baud = baudTable_t[1];
							}
						}

						ChangeLocalUartBaud_t(CurrentLoaclPdu->Baud);
#endif
					}

					for (int i = 0; i < BAUDTABLE_T_SIZE; i++)
					{
						FlashLocalLed();
						UartWrite(CurrentLoaclPdu->Data, CurrentLoaclPdu->Size);
						debug_str(DEBUG_LOG_METER, "UartWrite,seq=%04x\r\n",CurrentLoaclPdu->Seq);
						while (IsUartTxData())
						{
							OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
						}
						while (!IsUartTxSendAllDone())
						{
							OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
						}
						if (isBroad)
						{
							if (i < (BAUDTABLE_T_SIZE - 1)) // if (i < BAUDTABLE_T_SIZE)//
							{
								ChangeLocalUartBaud_t(baudTable_t[i + 1]);
							}
						}
						else
						{
							break;
						}
					}
					U485EnWrite(0);

					// 如果不是广播帧, 才等待回应
					//
					if (isBroad == 0)
					{
						LocalUartState = LOCAL_RECEIVE;
						if (CurrentLoaclPdu->Timeout)
						{
							StartTimer(&receiveTimer, CurrentLoaclPdu->Timeout * 100); // 等待帧返回时间
						}
						else
						{
							StartTimer(&receiveTimer, DEFAULT_WAIT_FRAME_TIME); // 等待帧返回时间
						}
					}
					else
					{
						LocalUartState = LOCAL_RECEIVE;
						FreeLocal(CurrentLoaclPdu);
					}
				}
#ifndef II_STA
				else
				{
					ChangeBaud();
				}
#endif
			}
		}
		AppLocalHandle();
	}
	#endif
}
extern int power;
// II采task
void II_STA_Task(void *arg)
{
	#if 0
	OS_ERR err;
	OSTimeDly(100, OS_OPT_TIME_DLY, &err);

	LED_Init();

#ifdef I_STA
	ChangeLocalUartBaud(9600);
	LoacalBaud = 9600;
#else
	LedShowTimeStart();
	ChangeLocalUartBaud(2400);
	LoacalBaud = 2400;
	OpenIr();
#endif

	SEARCH_METER_FLAG_init(); //???????
	TableCopyInit();
	t_no_exist_meter_init_fun();
	t_CHECK_NO_EXIST_METER_init_fun();
	get_collector_addr_fun();
	read_09meter_event_from_flash();
	read_list_flash_power_off();
#ifdef DEBUG_SEARCH_LOG_OUT
	debug_str(DEBUG_LOG_APP, "II_STA_Task() Init ok\r\n");
#endif
	for (;;)
	{
		MeterLocalHandle();
#ifndef PROTOCOL_NW_2021_GUANGDONG
		if (IsPowerOff == 1 && power == 0) // 自身发生了掉电事件
		{
			if (!GetWait485DataFlag())
			{
				if (!SEARCH_METER_FLAG.check_power_off_flag)
				{
					SEARCH_METER_FLAG.check_power_off_flag = 1;
					check_meter_list_power_on_var_init();
				}
				else if (SEARCH_METER_FLAG.check_power_off_flag == 1)
				{
					check_meter_list_power_off();
				}
			}
			OSTimeDly(1, OS_OPT_TIME_DLY, &err);
			continue;
		}
#endif

#ifndef I_STA
		event_RS485_send_data_queue_in();
#endif
#ifndef I_STA
		if (!GetWait485DataFlag())
			AppLocalHandleIFR();
#endif
		if (!GetWait485DataFlag())
			SearchMeter();
#ifndef I_STA
		if (!GetWait485DataFlag())
			check_no_exist_meter_fun();
		if (!GetWait485DataFlag())
			check_meter_type(); //??07????
		if (!GetWait485DataFlag())
			event_port_monitor(); //????
		// hplc_start_report_meterlist_fun();
		AppEventReportIIcai();
		// BindCollectorAddr();
		//     	SearchMeterOnTime();
#endif
		OSTimeDly(1, OS_OPT_TIME_DLY, &err);
	}
	#endif
}


#define SEND_LEN  (64)
#define RECV_LEN  (256)
//uint32_t meter_baudrate = 2400;



#if 1
//判断是否645帧
bool IsHave645FrameEx(uint8_t *data,uint16_t len,uint16_t *start_index, uint16_t *end_index)
{
    uint16_t index = 0;
    bool isFind = false;
    if(len < 12)
		return false;
	
    uint16_t endindex = (len-11);
    for(index = 0; index < endindex; index++)
    {
        if(data[index] == 0x68)
        {
            isFind = true;
            break;
        }
		else if(data[index] != 0xFE)
		{
			break;
		}
    }
    if(!isFind)
        return false;
    if(len - index < 12)
        return false;
    if(data[index] != 0x68 && data[index + 7] != 0x68)
        return false;
    
    uint16_t framelen = data[index + 9];
    framelen += 12;
    if((len - index) < framelen)
		return false;
    if(data[index + framelen - 1] != 0x16)
		return false;
    
    if(GetCheckSum(&data[index], framelen-2) != data[index + framelen - 2])
		return false;
	
    *start_index = index;
	*end_index = index + framelen - 1;
    return true;
}

uint32_t GetUartRate(uint8_t baudrate_type)
{
	switch(baudrate_type)
	{
		case UART_1200:
			return 1200;
		case UART_2400:
			return 2400;
		case UART_4800:
			return 4800;
		case UART_9600:
			return 9600;
		case UART_19200:
			return 19200;
		case UART_38400:
			return 38400;
		case UART_115200:
			return 115200;
		case UART_256000:
			return 256000;
		default:
			return 2400;
	}
}
bool FindMeterComplete = false;			//搜表完成

extern all_ble_breakers_addr_t all_ble_breakers_addr;

void AddBleBreakerToSystemMeterList()
{
	uint8_t zero_mac[LONG_ADDR_SIZE] = {0};
	uint8_t tran_mac[LONG_ADDR_SIZE];
	for(int i = 0; i < MAX_BLE_BREAKERS_NUM; i++)
	{
		if(memcmp(all_ble_breakers_addr.breaker_addr[i].mac, zero_mac,LONG_ADDR_SIZE) == 0)
			break;
		
		memcpy(tran_mac,all_ble_breakers_addr.breaker_addr[i].mac,LONG_ADDR_SIZE);
		TransposeAddr(tran_mac);
		AddSystemMeterToList(tran_mac,3,DLT_97,UART_8_DATA_BITS,UART_PARITY_EVEN,UART_1_STOP_BITS,UART_TX_TTL);//uart info no need, just for the function call integrity
	}
}

bool IsBleBreakerMac(uint8_t *meter_mac)
{	
	uint8_t zero_mac[LONG_ADDR_SIZE] = {0};
	uint8_t tran_mac[LONG_ADDR_SIZE];
	for(int i = 0; i < MAX_BLE_BREAKERS_NUM; i++)
	{
		if(memcmp(all_ble_breakers_addr.breaker_addr[i].mac, zero_mac,LONG_ADDR_SIZE) == 0)
			return false;

		memcpy(tran_mac,all_ble_breakers_addr.breaker_addr[i].mac,LONG_ADDR_SIZE);
		TransposeAddr(tran_mac);

		if((memcmp(all_ble_breakers_addr.breaker_addr[i].mac, meter_mac,LONG_ADDR_SIZE) == 0) || 
			(memcmp(tran_mac, meter_mac,LONG_ADDR_SIZE) == 0))
			return true;
	}
	return false;
}

bool IsForBleBreaker(uint8_t *data,uint16_t len)
{
    uint16_t index = 0;
    bool isFind = false;
	uint8_t meter_mac[6] = {0};
    if(len < 12)
		return false;
	
    uint16_t endindex = (len-11);
    for(index = 0; index < endindex; index++)
    {
        if(data[index] == 0x68)
        {
            isFind = true;
            break;
        }
		else if(data[index] != 0xFE)
		{
			break;
		}
    }
    if(!isFind)
        return false;
    if(len - index < 12)
        return false;
    if(data[index] != 0x68 && data[index + 7] != 0x68)
        return false;

	memcpy(meter_mac, &data[index + 1], 6);
	
    uint16_t framelen = data[index + 9];
    framelen += 12;
    if((len - index) < framelen)
		return false;
    if(data[index + framelen - 1] != 0x16)
		return false;
    
    if(GetCheckSum(&data[index], framelen-2) != data[index + framelen - 2])
		return false;

	
    return IsBleBreakerMac(meter_mac);
}

bool IsBleMeter(uint8_t *src_data, uint16_t src_len)
{
	if(src_len <= 0)
	return false;

	uint8_t data_count = src_data[0];
	if(data_count <= 0)
		return false;

	uint16_t meter_send_len;
	uint16_t src_pos = 1;
	uint16_t index = 0;
	uint8_t meter_send[SEND_LEN] = {0};
	for(index = 0; (index < data_count) && (src_pos < src_len); index++)
	{
		meter_send_len = src_data[src_pos];
		if(src_pos + meter_send_len >= src_len)
		{
			return false;
		}
		
		memset(meter_send, 0, SEND_LEN);
		memcpy(meter_send, &src_data[src_pos + 1], meter_send_len);		
		return IsForBleBreaker(meter_send, meter_send_len);
	}
	return false;
}

uint16_t OperateMeter(uint8_t *send_buf, uint8_t *src_data, uint16_t src_len,uint32_t meter_baudrate,uint8_t meter_parity)
{
	if(src_data == NULL || src_len <= 0)
	{
		debug_str(DEBUG_LOG_NET, "%s %d err %d\r\n",__func__,__LINE__,src_len);
		return 0;
	}

	uint8_t data_count = src_data[0];
	if(data_count <= 0)
	{
		debug_str(DEBUG_LOG_NET, "%s %d err\r\n",__func__,__LINE__);
		return 0;
	}
	ChangeLocalUartBaud_Parity(meter_baudrate,meter_parity);

	OS_ERR err;
	uint8_t send_count = 0;
	uint16_t send_len = 0;
	uint16_t send_pos = 1;
	
	uint16_t src_pos = 1;
	uint16_t index = 0;
	uint8_t meter_send[SEND_LEN] = {0};
	uint8_t meter_recv[RECV_LEN] = {0};
	uint16_t meter_send_len;
	uint16_t meter_recv_len;
	uint8_t retryTime = 100;
	uint16_t startIndex;
	uint16_t endIndex;
	//uint8_t readMeterAddr[20] = {0xFE,0xFE,0xFE,0xFE,0x68, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x11, 0x04,0x33,0x33,0x33,0x33, 0xB3, 0x16};
    // 清空发送队列
	while (IsUartTxData())
	{
		OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
	}
	
	for(index = 0; (index < data_count) && (src_pos < src_len); index++)
	{
		meter_send_len = src_data[src_pos];
		if(src_pos + meter_send_len >= src_len)
		{
			return send_len;
		}
		
		memset(meter_send, 0, SEND_LEN);
		memcpy(meter_send, &src_data[src_pos + 1], meter_send_len);		
		
		bool rflg = false;
		startIndex = 0; 
		endIndex = meter_send_len - 1;
		if(IsHave645FrameEx(meter_send, meter_send_len, &startIndex, &endIndex))
		{
			UartWrite(meter_send, meter_send_len);
		    while (IsUartTxData())
			{
				OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
			}

		    //uint32_t waitedelaytimems = (12 * 11 * 1000 + (meter_baudrate / 1000)) / meter_baudrate + 10;
			//while(waitedelaytimems--)
			OSTimeDlyHMSM(0, 0, 0, 50, OS_OPT_TIME_DLY, &err);
			memset(meter_recv, 0, RECV_LEN);
			uint8_t check_and_retry = 5;
			meter_recv_len = 0;
			uint16_t len_recv;
			while(check_and_retry-- > 0)
			{
				retryTime = 100;
				while(((len_recv = UartRead(&meter_recv[meter_recv_len], RECV_LEN - meter_recv_len)) <= 0) && (retryTime > 0))
				{
					OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_DLY, &err);
					retryTime--;
				}
				
				if(len_recv > 0)
				{
					meter_recv_len += len_recv;
					
					rflg = false;
					startIndex = 0;
					endIndex = meter_recv_len - 1;
					if(IsHave645FrameEx(meter_recv, meter_recv_len, &startIndex, &endIndex))
					{
						rflg = true;
					}
					if(rflg)
					{
						send_count++;
						send_buf[0] = send_count;
						send_buf[send_pos] = endIndex - startIndex + 1;
						memcpy(&send_buf[send_pos + 1], meter_recv + startIndex, endIndex - startIndex + 1);
						send_pos += (send_buf[send_pos] + 1);
						send_len = send_pos;
						break;
					}
				}
			}
		}
		else
		{
			UartWrite(meter_send, meter_send_len);
		    while (IsUartTxData())
			{
				OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
			}

		    //uint32_t waitedelaytimems = (12 * 11 * 1000 + (meter_baudrate / 1000)) / meter_baudrate + 10;
			//while(waitedelaytimems--)
			OSTimeDlyHMSM(0, 0, 0, 50, OS_OPT_TIME_DLY, &err);
			memset(meter_recv, 0, RECV_LEN);
			uint8_t check_and_retry = 5;
			uint16_t len_recv;
			while(check_and_retry-- > 0)
			{
				retryTime = 100;
				while(((len_recv = UartRead(meter_recv, RECV_LEN)) <= 0) && (retryTime > 0))
				{
					OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_DLY, &err);
					retryTime--;
				}
				
				if(len_recv > 0)
				{
					send_count++;
					send_buf[0] = send_count;
					send_buf[send_pos] = len_recv;
					memcpy(&send_buf[send_pos + 1], meter_recv, len_recv);
					send_pos += (send_buf[send_pos] + 1);
					send_len = send_pos;
					break;
				}
			}
		}
		
		src_pos += (meter_send_len + 1);
	}	
	return send_len;
}


//===
typedef bool (*local_task_cb_fun_t)(void * exparam,u8* data,u16 len,u8 state);

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
    uint16_t Rmsend;
    uint16_t LSn;//must

    //以下的数据要4字节对齐，否则在LC830上有问题
	uint8_t Data[];       //数据
}local_task_param_t;

struct System_Params_Type SystemParams = {0};

u8 CheckMeterListIndex = 0;
u8 ReadMeterTimerAddr[LONG_ADDR_SIZE];
bool DirtyMeterList = false;
u32 UartRateEnumList[] = {1200,2400,4800,9600,19200,38400,115200};//与enum Uart_Rate_Type配合使用
extern bool IsBindAddr;
TimeValue LocalCheckFindMeterTimer;
u8 FindScanMeterType = DLT_07;
u8 FindScanMeterAddr[LONG_ADDR_SIZE];
u8 FindScanMeterAddrIndex = 0x00;
u8 FindScanMeterStatu = START_FIND_SCAN;
u8 ScanMeterRunStatus = SCAN_METER_IDLE;
u8 FindScanMeterUartRate = 0;//搜表初始波特率
#define METER_UART_TASK_LENGTH (16)
#define AbeforB(A, B) (((B) - (A)) < ((A) - (B)))

local_task_param_t *MeterUartTaskList[METER_UART_TASK_LENGTH];
static uint8_t MeterUartTaskLen = 0;
static uint8_t CurrentMeterUartTaskIndex = 0;
static TimeValue MeterUartReceiveTimer;
static LOCAL_UART_STATE MeterUartState = LOCAL_IDLE;
//妫€娴嬪伐瑁呭抚鍦板潃鏄惁绗﹀悎
BOOL CheckLocalTestFrame(u8* pbuf)
{
    BOOL flg = FALSE;
	u8 localreadaddr[LONG_ADDR_SIZE] = {0xEE,0xEE,0xEE,0xEE,0xEE,0xEE};
	u8 ToolTestAddr[LONG_ADDR_SIZE] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
	u8 TestCaseAddr[LONG_ADDR_SIZE] = {0xDC, 0xEE, 0xEB, 0xDC, 0xBB, 0xCE};
    if((pbuf[0] == 0x68)&&(pbuf[7] == 0x68)&&
	( (0 == memcmp(&pbuf[1], ToolTestAddr, LONG_ADDR_SIZE))
    ||(0 == memcmp(&pbuf[1], TestCaseAddr, LONG_ADDR_SIZE))
    ||(0 == memcmp(&pbuf[1], StaChipID.mac,LONG_ADDR_SIZE))
	||(0 == memcmp(&pbuf[1], localreadaddr, LONG_ADDR_SIZE))))
    {
        flg = TRUE;
    }
    return flg;
}
extern u8 GetCheckSum(const u8 * src,const u16 len);

//鏋勫缓645宸ヨ鍥炲甯?
#define DL645_REPLY_MASK (0x80)
u8 Get645ToolTestReplayFrame(u8* pbuf, const u8* paddr,u8 con, u32 di, u8 di_len, const u8* pdata, u8 dlen)
{
    u8 flen = 0;
	u8 data_idx = LONG_ADDR_SIZE + 4 + di_len;
	memcpy(&pbuf[data_idx],pdata,dlen);
    pbuf[flen++] = 0x68;
    memcpy(&pbuf[flen], paddr, LONG_ADDR_SIZE);
    flen += LONG_ADDR_SIZE;
    pbuf[flen++] = 0x68;
    pbuf[flen++] = con | DL645_REPLY_MASK;
    pbuf[flen++] = di_len + dlen;
    for(u8 i = 0;i < di_len;i++)
    {
        pbuf[flen++] = (di >> (i * 8)) & 0xFF;
    }
	flen += dlen;
    pbuf[flen] = GetCheckSum(&pbuf[0], flen);
    flen++;
    pbuf[flen++] = 0x16;
    return flen;
}

extern lamp_ble_param_t lamp_ble_param;
extern TimeValue LocalUartTestModeTimer;
//绛夎摑鐗欏洖澶嶅悗锛屽啀缁欐妱鎺у櫒鍥炲
uint8_t set_ble_power_reply = 0xFF;
extern uint16_t ble_get_ble_send_power();
extern bool ble_change_ble_send_power(uint32_t ble_power);

//645甯у鐞?
u16 Local645FrameProcess(u8 *pbuf,u16 dlen,u8 *pdst_data)
{
	//濡傛灉鏀跺埌645甯э紝閲嶆柊鍒锋柊30S
	StartTimer(&LocalUartTestModeTimer, 60*1000);
    u16  pdst_size = 0;
	u8 ToolTestAddr[LONG_ADDR_SIZE] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    if(CheckLocalTestFrame(pbuf))
    {
        u8 ctrl = pbuf[8];
        u32 d_id = 0;
        switch(ctrl)
        {
	        case 0x1F:
			{
	            d_id = pbuf[10];  
	            switch(d_id)
	            {
	            	case 0xA0:
	                {
	                	
	                	u8 ModuleId[8];
                        BPLC_GetWaferID(&ModuleId);
                        pdst_size = Get645ToolTestReplayFrame(pdst_data,ToolTestAddr, ctrl, d_id, 1,ModuleId, sizeof(ModuleId));
                        break;
	                }
	                case 0xF0:
	                {
	                	
	                	u8 Ver[7];
                        Ver[0] = (SELF_VER_RELEASE_DATE >>  8) & 0xff;
                        Ver[1] = (SELF_VER_RELEASE_DATE >> 16) & 0xff;
                        Ver[2] = (SELF_VER_RELEASE_DATE >> 24) & 0xff;
                        Ver[3] = (LC_APP_VER >>  0) & 0xff;
                        Ver[4] = (LC_APP_VER >>  8) & 0xff;
						//娌¤鍒扮増鏈墠锛屽厛涓嶅洖澶?
						if(lamp_ble_param.ble_ver_value.software_version == 0)
						{
							return 0;
						}
						Ver[5] = (lamp_ble_param.ble_ver_value.software_version >>  0) & 0xff;
                        Ver[6] = (lamp_ble_param.ble_ver_value.software_version >>  8) & 0xff;
                        pdst_size = Get645ToolTestReplayFrame(pdst_data,ToolTestAddr, ctrl, d_id, 1,Ver, sizeof(Ver));
                        break;
	                }
					case 0xF1:
	                {
	                	lc_net_my_info_t p_info;
						lc_net_get_net_self_info(&p_info);
	                	u8 mac[6];
                        memcpy(mac,p_info.addr,6);
                        pdst_size = Get645ToolTestReplayFrame(pdst_data,ToolTestAddr, ctrl, d_id, 1,mac, 6);
                        break;
	                }
					case 0xF2:
	                {
	                	lc_net_get_factory_mac(lamp_ble_param.ble_mac_addr);
        				lamp_ble_set_mac_addr(lamp_ble_param.ble_mac_addr);
	                	memcpy(StaChipID.mac,&pbuf[11],LONG_ADDR_SIZE);
						StorageStaIdInfo();
	                	SetStaMAC(&pbuf[11]);
						lc_net_my_info_t p_info;
						lc_net_get_net_self_info(&p_info);
	                	u8 mac[6];
                        memcpy(mac,p_info.addr,6);
                        pdst_size = Get645ToolTestReplayFrame(pdst_data,ToolTestAddr, ctrl, d_id, 1,mac, 6);
                        break;
	                }
					case 0xF3:
	                {
	                	u8 power[3];
                        power[0] = StaFixPara.txPower[0];
						power[1] = StaFixPara.hrfTxPower[0];
						power[2] = (u8)ble_get_ble_send_power();
						//娌¤鍒拌摑鐗欏姛鐜囧墠鍏堜笉鍥炲
						if(power[2] == 0)
						{
							return 0;
						}
                        pdst_size = Get645ToolTestReplayFrame(pdst_data,ToolTestAddr, ctrl, d_id, 1,power, 3);
                        break;
	                }
					case 0xF4:
	                {
	                	uint8_t plc_power[4]= {0};
						plc_power[0] = pbuf[11];
						plc_power[1] = pbuf[11];
						plc_power[2] = pbuf[11];
						plc_power[3] = pbuf[11];
	                	LcPlcPowerCheckAndSet(plc_power);
						uint8_t rf_power[2] = {0};
						rf_power[0] = pbuf[12];
						rf_power[1] = pbuf[12];
						LcRfPowerCheckAndSet(rf_power);
						uint32_t ble_power = pbuf[13];
						ble_change_ble_send_power(ble_power);
						set_ble_power_reply = 0;
						//涓嶅洖澶嶏紝绛夋敹鍒拌缃摑鐗欏姛鐜囨垚鍔熷啀鍥炲
                        break;
	                }
					case 0xF5:
	                {
	                	uint8_t band = FrequenceGet();
	                	u8 FrequencyBand[7];
						FrequencyBand[0] = 0x01;
						FrequencyBand[1] = band;
						FrequencyBand[2] = 0x02;
						FrequencyBand[3] = HRF_Option;
						FrequencyBand[4] = HRF_Channel;
						FrequencyBand[5] = 0x03;
						FrequencyBand[6] = 0;
                        pdst_size = Get645ToolTestReplayFrame(pdst_data,ToolTestAddr, ctrl, d_id, 1,FrequencyBand, 7);
                        break;
	                }
					case 0xF6:
	                {
	                	u8 returnFlag = 0;
						if(pbuf[11] == 0x01)
						{
							uint8_t band = pbuf[12];
							HPLC_PHY_SetFrequence(band);
							HPLC_PhyInit(band, power);
							if(band == FrequenceGet())
							{
								returnFlag = 1;
							}
						}
						else if(pbuf[11] == 0x02)
						{
							uint8_t option = pbuf[12];
							uint8_t channel = pbuf[13];
							HRF_PHY_SetChannel(channel,option);
							
							if(HRF_Option == option && HRF_Channel == channel)
							{
								returnFlag = 1;
							}
						}
						if(1 == returnFlag)
						{
							u8 result[1];
	                        result[0] = 0xF6;
	                        pdst_size = Get645ToolTestReplayFrame(pdst_data,ToolTestAddr, ctrl, d_id, 1,result, 1);
						}
                        break;
	                }
	                default:
	                	break;   
				}
	            break;
	        }
			default:
	                break;
        }
    }
    return pdst_size;
}

extern bool IsHave645Frame(u8*data,u16 len,u16 *hindex);

//宸ヨ甯у鐞?
uint16_t lc_plc_self_info_process(uint8_t* psrc,uint16_t d_len,uint8_t* pdst)
{
	u16 acklen = 0;
	u16 hindex = 0;
	if(IsHave645Frame(psrc,d_len,&hindex))
	{
		acklen = Local645FrameProcess(&(psrc[hindex]),d_len-hindex,pdst);
	}
	return acklen;
}
extern bool AddMeterUartTask(local_task_param_t *ppar);

bool MeterLocal_send_fram_add_task(uint8_t *data,uint16_t len,uint16_t pri,uint16_t t_time,uint16_t resend,local_task_cb_fun_t cb_fun,void *cb_fun_arg)
{
    local_task_param_t* local_task = NULL;

    if((data != NULL) && (len > 0))
	{
		local_task = (local_task_param_t*)malloc(len + sizeof(local_task_param_t) + 32);
		if(local_task != NULL)
		{
			local_task->Prio = pri;
			memcpy(local_task->Data,data,len);
			local_task->Size = len;
			local_task->Urate = 9600;
			local_task->Tout = t_time > 200 ? 200 : t_time;
			local_task->Rmsend = resend;
			
			local_task->cb_fun = cb_fun;
			local_task->exparam = cb_fun_arg;

			if(false == AddMeterUartTask(local_task))
			{
			    free(local_task);
				return false;
			}

			return true;
		}
	}

    return false;
}
extern bool TestMode;

void MeterLocalHandle(void)
{
	if(FindMeterComplete == true)
	return ;
	OS_ERR err;
	// 判断串口是否空闲任务状态，空闲即非接收处理状态可处理发送任务
    if (MeterUartState == LOCAL_IDLE)
    {
        if (MeterUartTaskLen > 0)
        {
            u8 maxPrio = 0;
            u32 selectsn = 0;
            bool fflg = false;
			for (int i = 0; i < METER_UART_TASK_LENGTH; i++)
			{
				if (MeterUartTaskList[i] != NULL)
				{
					if ((fflg == false) || (MeterUartTaskList[i]->Prio > maxPrio) || ((MeterUartTaskList[i]->Prio == maxPrio) && (AbeforB(MeterUartTaskList[i]->LSn, selectsn))))
					{
						maxPrio = MeterUartTaskList[i]->Prio;
						CurrentMeterUartTaskIndex = i;
						fflg = true;
						selectsn = MeterUartTaskList[i]->LSn;
					}
				}
			}

			if (fflg == true)
            {
            	
                local_task_param_t *current_task = MeterUartTaskList[CurrentMeterUartTaskIndex];
                ChangeLocalUartBaud(current_task->Urate);
                UartWrite(current_task->Data, current_task->Size);
				char buf[100] = {0};
				for(int i=0;i<30&&i<current_task->Size;i++)
				{
					sprintf(&buf[i*3],"%02x ",current_task->Data[i]);
				}
				debug_str(DEBUG_LOG_UPDATA, "sbuf%d:%s \r\n",current_task->Urate,buf);

				while (IsUartTxData())
				{
					OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
				}
                u32 rate= Usart1Para.BaudRate;
                u32 waitedelaytimems = (current_task->Size * 11 * 1000 + (rate/1000))/rate + 10;
                if (current_task->Tout > 0)
                {	
					//清空读取队列
                    MeterUartState = LOCAL_RECEIVE;
                    StartTimer(&MeterUartReceiveTimer, current_task->Tout * 100+waitedelaytimems); //等待帧返回时间
                }
				else
				{
					if(current_task->cb_fun == NULL)
					{
						DeleteMeterUartTask(CurrentMeterUartTaskIndex);
					}
				}
				
            }
        }
    }
	uint8_t RecvBuf[256] = {0};
	uint16_t RecvLen = 0;
	uint8_t retryTime = 100;
	while(((RecvLen = UartRead(RecvBuf, 256)) <= 0)&& (retryTime > 0))
	{
		OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_DLY, &err);
		retryTime--;
	}
	if(RecvLen > 0 )
	{
		char buf[100] = {0};
		for(int i=0;i<30&&i<RecvLen;i++)
		{
			sprintf(&buf[i*3],"%02x ",RecvBuf[i]);
		}
		debug_str(DEBUG_LOG_UPDATA, "rbuf:%s\r\n",buf);

		// 接收到数据
		u32 len = RecvLen >= MAX_LOCAL_LEN ? MAX_LOCAL_LEN : RecvLen;
		u16 self_len = 0;
		if(TestMode == true)
		{
			if((self_len = lc_plc_self_info_process(RecvBuf,len,RecvBuf))>0)
			{
				MeterLocal_send_fram_add_task(RecvBuf,self_len,10,0,0,NULL,NULL);
			}
		}
		else
		{
			local_task_param_t *currentpdu = MeterUartTaskList[CurrentMeterUartTaskIndex];
			if(currentpdu->cb_fun != NULL)
			{
				if(currentpdu->cb_fun(currentpdu->exparam,RecvBuf,len,UART_SUCCESS))
				{
					DeleteMeterUartTask(CurrentMeterUartTaskIndex);
					MeterUartState = LOCAL_IDLE;
				}
			}
		}
	}
	ST_STA_ATTR_TYPE const * staattr = GetStaBaseAttr();
	//超时数据
    if (LcTimeOut(&MeterUartReceiveTimer))
    {
        if (MeterUartState != LOCAL_IDLE)
        {
            local_task_param_t *currentpdu = MeterUartTaskList[CurrentMeterUartTaskIndex];
            if (currentpdu->Rmsend > 0)
            {
				currentpdu->Rmsend = currentpdu->Rmsend - 1;
                UartWrite(currentpdu->Data, currentpdu->Size);
				while (IsUartTxData())
				{
					OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
				}
                if (currentpdu->Tout)
                    StartTimer(&MeterUartReceiveTimer, currentpdu->Tout * 100); //等待帧返回时间
                else
                    StartTimer(&MeterUartReceiveTimer, DEFAULT_WAIT_FRAME_TIME); //等待帧返回时间
            }
            else
            {
				if(currentpdu->cb_fun != NULL)
				{
					currentpdu->cb_fun(currentpdu->exparam,NULL,0,UART_TIME_OUT);
				}
                MeterUartState = LOCAL_IDLE;
                TimeStop(&MeterUartReceiveTimer);
                DeleteMeterUartTask(CurrentMeterUartTaskIndex);
            }
        }
    }
	//濡傛灉钃濈墮鍥炲浜嗚缃姛鐜囨垚鍔燂紝鍒欒繘琛屽洖澶?
	if(TestMode == true && set_ble_power_reply == 1)
    {
    	u8 ToolTestAddr[LONG_ADDR_SIZE] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    	set_ble_power_reply = 0xFF;
		uint8_t power[3] = {0};
		power[0] = StaFixPara.txPower[0];
		power[1] = StaFixPara.hrfTxPower[0];
		power[2] = (u8)ble_get_ble_send_power();
        uint16_t pdst_size = Get645ToolTestReplayFrame(RecvBuf,ToolTestAddr, 0x1f, 0xf4, 1,power, 3);
		MeterLocal_send_fram_add_task(RecvBuf,pdst_size,10,0,0,NULL,NULL);
    }

}


//添加一个本地串口任务
bool AddMeterUartTask(local_task_param_t *ppar)
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

    if (MeterUartTaskLen < METER_UART_TASK_LENGTH)
    {
        for (int i = 0; i < METER_UART_TASK_LENGTH; i++)
        {
            if (MeterUartTaskList[i] == NULL)
            {
                MeterUartTaskList[i] = ppar;
                ++MeterUartTaskLen;
                rflg = true;
                break;
            }
        }
    }
    
	// 退出临界区
	OS_CRITICAL_EXIT();
    return rflg;
}

//删除本地串口任务
bool DeleteMeterUartTask(u8 taskindex)
{
    bool rflg = false;
    if (taskindex >= METER_UART_TASK_LENGTH)
        return false;
    if (MeterUartTaskList[taskindex] == NULL)
        return false;

	// 进入临界区
	CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
	if(MeterUartTaskList[taskindex]->exparam != NULL)
	{
		free(MeterUartTaskList[taskindex]->exparam);
		MeterUartTaskList[taskindex]->exparam = NULL;
	}
	free(MeterUartTaskList[taskindex]);

    MeterUartTaskList[taskindex] = NULL;
    MeterUartTaskLen--;
    rflg = true;

	// 退出临界区
	OS_CRITICAL_EXIT();

    return rflg;
}

void TransposeAddrtemp(u8 addr[LONG_ADDR_SIZE])
{
    uint8_t i = 0;
    for(;i < 3;i++)
    {
        u8 temp = addr[i];
		addr[i] = addr[5-i];
		addr[5-i] = temp;
    }
}

//偏历例表
bool GetSystemMeterFromListDepenNo(u8 * pid,u8 * rate,u8 * type,u8 *no,u8 *udbit,u8* ucbit,u8 *usbit,u8 *utxpin)
{
    bool flg = false;
    u8 i;
    if(*no < SYSTEM_METER_LIST_NUM)
    {
        for(i = *no;i < SYSTEM_METER_LIST_NUM;i ++)
        {
            if(SystemParams.SystemMeterList.memter[i].Type != INVALID_METER_TYPE)
            {//查找到空闲区
                memcpy(pid,SystemParams.SystemMeterList.memter[i].Id,LONG_ADDR_SIZE);
                * rate = SystemParams.SystemMeterList.memter[i].UartRate;
                * type = SystemParams.SystemMeterList.memter[i].Type;
				
				*udbit = SystemParams.SystemMeterList.memter[i].UartDbit;
				*ucbit = SystemParams.SystemMeterList.memter[i].UartCbit;
				*usbit = SystemParams.SystemMeterList.memter[i].UartSbit;
				*utxpin = SystemParams.SystemMeterList.memter[i].UartTxPin;
				
                flg = true;
                *no = i+1;
                break;
            }
        }
    }
    return flg;
} 


bool CheckNexMeterAddr(u8 *mno)
{
    bool rflg = false;
    u8 metertype;
    u8 rateindex = 0;
    u8 ucbit,udbit,usbit,utxpin;
    if(GetSystemMeterFromListDepenNo(ReadMeterTimerAddr,&rateindex,
                        &metertype,mno,
                        &udbit,&ucbit,&usbit,&utxpin))
    {
        //偏历到一个表地址
        local_task_param_t* localPdu = malloc(LOCAL_DEFAUT_FRAME_SIZE + sizeof(local_task_param_t) +32); 
        if(localPdu == NULL)return false;
        localPdu->Stype = 0;
        localPdu->Tout = CHECK_METER_TIME_OUT_VALUE;
        localPdu->Rmsend = 1;
        localPdu->cb_fun = CheckMeterListCallBackFunction;
        localPdu->Prio = CHECK_METER_LIST_PRIO_VALUE;
        localPdu->Urate = UartRateEnumList[rateindex];
        localPdu->exparam = NULL;
		
		localPdu->UChb = ucbit;
		localPdu->UDBits = udbit;
		localPdu->USBits = usbit;
		localPdu->UTxPin = utxpin;
		TransposeAddrtemp(ReadMeterTimerAddr);
        localPdu->Size = GetDetectMeterFrame(localPdu->Data,metertype,ReadMeterTimerAddr);
        if(AddMeterUartTask(localPdu))
        {
            rflg = true;
        }
        else
        {
            free(localPdu);
        }
    }
	else
	{
		FindMeterComplete = true;
		u8 addr_temp[] = {0x36, 0x00, 0x00, 0x10, 0xe7, 0x00};
		memcpy(addr_temp,StaChipID.mac,6);
	    SetMACType(1);
		lc_net_set_addr(addr_temp,LONG_ADDR_SIZE);
	    IsBindAddr = true;
	}
    return rflg;
}


void GetMeterUartChkBitFromMeterType(u8 metertype,local_task_param_t* localPdu)
{
    switch (metertype)
    {
        case DLT_97:
        case DLT_07:
        case I_LOCAL:
        default:
            localPdu->UChb = UART_PARITY_EVEN;
            localPdu->UDBits = UART_8_DATA_BITS;
            localPdu->USBits = UART_1_STOP_BITS;
            localPdu->UTxPin = UART_TX_OD;
        break;
    }
}


bool FindNexMeterAddr()
{
    bool rflg = false;
    //偏历到一个表地址
    local_task_param_t* localPdu = malloc(LOCAL_DEFAUT_FRAME_SIZE + sizeof(local_task_param_t) +32);  
    if(localPdu == NULL)return false; 
    //
    localPdu->Stype = 0;
    localPdu->Tout = FIND_METER_LIST_TIME_OUT_VALUE;
    localPdu->Rmsend = 1;
    localPdu->cb_fun = FindMeterCallBackFunction;
    localPdu->Prio = FIND_METER_LIST_PRIO_VALUE;
    localPdu->Urate = UartRateEnumList[FindScanMeterUartRate];
    localPdu->exparam = NULL;
    GetMeterUartChkBitFromMeterType(FindScanMeterType,localPdu);
    localPdu->Size = GetDetectMeterFrame(localPdu->Data,FindScanMeterType,FindScanMeterAddr);
    if(AddMeterUartTask(localPdu))
    {
        rflg = true;
    }
    else
    {
        free(localPdu);
    }
    return rflg;
}

bool FindReadMeterAddr()
{
    bool rflg = false;
    //偏历到一个表地址
    local_task_param_t* localPdu = malloc(LOCAL_DEFAUT_FRAME_SIZE + sizeof(local_task_param_t) +32); 
    if(localPdu == NULL)
        return false; 
    //
    localPdu->Stype = 0;
    localPdu->Tout = FIND_METER_LIST_TIME_OUT_VALUE;
    localPdu->Rmsend = 1;
    localPdu->cb_fun = FindMeterCallBackFunction;
    localPdu->Prio = FIND_METER_LIST_PRIO_VALUE;
    localPdu->Urate = UartRateEnumList[FindScanMeterUartRate];
    localPdu->exparam = NULL;
    GetMeterUartChkBitFromMeterType(FindScanMeterType,localPdu);
    localPdu->Size = GetReadMeterAddrDlt645(localPdu->Data,FindScanMeterType,FindScanMeterAddr);
    if(AddMeterUartTask(localPdu))
    {
        rflg = true;
    }
    else
    {
        free(localPdu);
    }
    return rflg;
}

//生成探测表列表检测帧
u8 GetDetectMeterFrame(u8 * pd,u8 type,u8 *addr)
{
    u8 len = 0;
    pd[len ++] = 0x68;
    memcpy(&pd[len],addr,LONG_ADDR_SIZE);
    len += LONG_ADDR_SIZE;
    pd[len ++] = 0x68;
    if(type == DLT_97)
    {
        pd[len ++] = 0x01;
        pd[len ++] = 0x02;
        pd[len ++] = 0x43;
        pd[len ++] = 0xC3;
    }
    else if(type == DLT_07)
    {
        pd[len ++] = 0x11;
        pd[len ++] = 0x04;
        pd[len ++] = 0x33;
        pd[len ++] = 0x33;
        pd[len ++] = 0x34;
        pd[len ++] = 0x33;
    }
	else
	{
		pd[len ++] = 0x13;
        pd[len ++] = 0x00;
	}
    pd[len] = GetCheckSum(pd,len);
    len ++;
    pd[len ++] = 0x16;
    return len;
}

bool AddSystemMeterToList(u8 * pid,u8 rate,u8 type,u8 udbit,u8 ucbit,u8 usbit,u8 utxpin)
{
    u8 i,solt,asolt;
    solt = SYSTEM_METER_LIST_NUM;
    asolt = SYSTEM_METER_LIST_NUM;
    bool flg = false;
    for(i = 0;i < SYSTEM_METER_LIST_NUM;i++)
    {
        if(SystemParams.SystemMeterList.memter[i].Type == INVALID_METER_TYPE)
        {
            if(solt >= SYSTEM_METER_LIST_NUM)solt = i;
        }
        else if(0 == memcmp(pid,SystemParams.SystemMeterList.memter[i].Id,LONG_ADDR_SIZE))
        {
            asolt = i;
            break;
        }
    }
    if(SYSTEM_METER_LIST_NUM != asolt)
    {
        if((SystemParams.SystemMeterList.memter[asolt].Type != type)
                || (SystemParams.SystemMeterList.memter[asolt].UartRate != rate)
                ||(SystemParams.SystemMeterList.memter[asolt].UartDbit != udbit)
                || (SystemParams.SystemMeterList.memter[asolt].UartCbit != ucbit)
                ||(SystemParams.SystemMeterList.memter[asolt].UartSbit != usbit)
                || (SystemParams.SystemMeterList.memter[asolt].UartTxPin != utxpin)
                )
        {//更新信息
            SystemParams.SystemMeterList.memter[asolt].Type = type;
            SystemParams.SystemMeterList.memter[asolt].UartRate = rate;

            SystemParams.SystemMeterList.memter[asolt].UartDbit = udbit;
            SystemParams.SystemMeterList.memter[asolt].UartCbit = ucbit;
            SystemParams.SystemMeterList.memter[asolt].UartSbit = usbit;
            SystemParams.SystemMeterList.memter[asolt].UartTxPin = utxpin;
            
            //DirtyMeterList = true;
        }
        flg = true;
    }
    else
    {//档案中不存在
        if(SYSTEM_METER_LIST_NUM != solt)
        {//有空闲区，写入档案列表
        	debug_str(DEBUG_LOG_METER, "search meter %02x %02x %02x %02x %02x %02x\r\n",pid[0],pid[1],pid[2],pid[3],pid[4],pid[5]);
            SystemParams.SystemMeterList.meterlen ++;
            memcpy(SystemParams.SystemMeterList.memter[solt].Id,pid,LONG_ADDR_SIZE);
            SystemParams.SystemMeterList.memter[solt].Type = type;
            SystemParams.SystemMeterList.memter[solt].UartRate = rate;
            SystemParams.SystemMeterList.memter[solt].UartDbit = udbit;
            SystemParams.SystemMeterList.memter[solt].UartCbit = ucbit;
            SystemParams.SystemMeterList.memter[solt].UartSbit = usbit;
            SystemParams.SystemMeterList.memter[solt].UartTxPin = utxpin;

            //DirtyMeterList = true;
            flg = true;
        }
    }
    return flg;
}


bool DelSystemMeterFromeList(u8 * pid)
{
    u8 i,asolt;
    asolt = SYSTEM_METER_LIST_NUM;
    bool flg = false;
    for(i = 0;i < SYSTEM_METER_LIST_NUM;i ++)
    {
        if(SystemParams.SystemMeterList.memter[i].Type != INVALID_METER_TYPE)
        {//查找到空闲区
            if(0 == memcmp(pid,SystemParams.SystemMeterList.memter[i].Id,LONG_ADDR_SIZE))
            {//找到相同地址区
                asolt = i;
                break;
            }
        }
    }
    if(SYSTEM_METER_LIST_NUM != asolt)
    {//找到节点
        SystemParams.SystemMeterList.memter[asolt].Type = INVALID_METER_TYPE;
        SystemParams.SystemMeterList.meterlen --;
        flg = true;
        DirtyMeterList = true;
    }
    return flg;
}


//较验表列表；回调函数，包括冲突，有数据，超时;返回：true处理完成，false处理失败;*ust失败时,清空fifo及不清空fifo
bool CheckMeterListCallBackFunction(void * exparam,u8* data,u16 len,u8 ust)
{
    u8 state = ust;
    if(state == UART_SUCCESS)
    {//正常接收到数据
        if(len >= 12)
        {
            u16 hindex = 0;
            if(IsHave645Frame(data,len,&hindex))
            {
                if(0 == memcmp(&data[hindex+1],ReadMeterTimerAddr,LONG_ADDR_SIZE))
                {//处理完成，下一表；
                    CheckNexMeterAddr(&CheckMeterListIndex);
                    ust = UART_CLEAR_FIFO;
                    return true;
                }
                else
                {//跟档案不对应,继续等待
                    ust = UART_CLEAR_FIFO;
                    return false;
                }
            }
            else
            {//未发现645帧，继续等，不清空缓冲
                ust = UART_NOTHING;
                return false;
            }
        }
        else
        {//接收长度不够，继续接收
            ust = UART_NOTHING;
            return false;
        }
    }
    else //if(state)
    {
        if((state&UART_TIME_OUT) == UART_TIME_OUT)
        {//超时
            //删除档案
            DelSystemMeterFromeList(ReadMeterTimerAddr);
            //校验下一个
            CheckNexMeterAddr(&CheckMeterListIndex);
            ust = UART_CLEAR_FIFO;
            return true;
        }
        else
        {//异常,清空缓冲
            ust = UART_CLEAR_FIFO;
            return false;
        }
    }
}

//同步系统mac地址与表地址
bool CheckSystemAddr()
{
    bool r_flg = false;
    bool tflg = false,addrflg=false;
    unsigned char addrtmp[LONG_ADDR_SIZE];
    u8 uartratetmp,metertypetmp;
    u8 udbit,ucbit,usbit,utpin;
    u8 tflgindex=0;
    int i;
    for(i = 0;i < SYSTEM_METER_LIST_NUM;i ++)
    {
        if(SystemParams.SystemMeterList.memter[i].Type != INVALID_METER_TYPE)
        {
            if(!addrflg)
            {//第一次运行，获取一个表地址，备用于系统地址
                memcpy(addrtmp,SystemParams.SystemMeterList.memter[i].Id,LONG_ADDR_SIZE);
                uartratetmp = SystemParams.SystemMeterList.memter[i].UartRate;
                metertypetmp = SystemParams.SystemMeterList.memter[i].Type;
                
                udbit= SystemParams.SystemMeterList.memter[i].UartDbit;
                ucbit = SystemParams.SystemMeterList.memter[i].UartCbit;
                usbit = SystemParams.SystemMeterList.memter[i].UartSbit;
                utpin = SystemParams.SystemMeterList.memter[i].UartTxPin;
        		
                addrflg = true;
            }
            if(0 == memcmp(SystemParams.SystemMeterList.memter[i].Id,SystemParams.SystemLongAdd,LONG_ADDR_SIZE))
            {//列表中找到系统地址
                tflg = true;
                tflgindex = i;
                break;
            }
        }
    }
    
    if(!tflg)
    {//没找到相同表号
        if(addrflg)
        {//有替换表号
            memcpy(SystemParams.SystemLongAdd,addrtmp,LONG_ADDR_SIZE);
            //时钟复位
            SystemParams.SystemUartRate = uartratetmp;
            SystemParams.SystemType = metertypetmp;

            SystemParams.UartDbit = udbit;
            SystemParams.UartCbit = ucbit;
            SystemParams.UartSbit = usbit;
            SystemParams.UartTxPin = utpin;
            r_flg = true;
        }
        else
        {//无表号可使用
            if(0 != memcmp(SystemParams.SystemLongAdd,SystemParams.CollecterIIAddr,LONG_ADDR_SIZE))
            {
				IsBindAddr = RT_FALSE;
                memcpy(SystemParams.SystemLongAdd,SystemParams.CollecterIIAddr,LONG_ADDR_SIZE);//采集器地址作为地址使用
            }
        }
    }
    else
    {//相同表号
        if((SystemParams.SystemUartRate != SystemParams.SystemMeterList.memter[tflgindex].UartRate)
        ||(SystemParams.SystemType != SystemParams.SystemMeterList.memter[tflgindex].Type)
        ||(SystemParams.UartDbit != SystemParams.SystemMeterList.memter[tflgindex].UartDbit)
        ||(SystemParams.UartCbit != SystemParams.SystemMeterList.memter[tflgindex].UartCbit)
        ||(SystemParams.UartSbit != SystemParams.SystemMeterList.memter[tflgindex].UartSbit)
        ||(SystemParams.UartTxPin != SystemParams.SystemMeterList.memter[tflgindex].UartTxPin)
        )
        {
        
            SystemParams.SystemUartRate = SystemParams.SystemMeterList.memter[tflgindex].UartRate;
            SystemParams.SystemType = SystemParams.SystemMeterList.memter[tflgindex].Type;
            SystemParams.UartDbit = SystemParams.SystemMeterList.memter[i].UartDbit;
            SystemParams.UartCbit = SystemParams.SystemMeterList.memter[i].UartCbit;
            SystemParams.UartSbit = SystemParams.SystemMeterList.memter[i].UartSbit;
            SystemParams.UartTxPin = SystemParams.SystemMeterList.memter[i].UartTxPin;
        }
        r_flg = true;
    }
	return r_flg;
}

//记动校验表列表
void StartCheckMeterList()
{
    CheckMeterListIndex = 0;
    CheckNexMeterAddr(&CheckMeterListIndex);
}

u8 BcdIncrease(u8 data)
{
	++ data;
	if((data & 0x0F) == 0x0A)
	{
		data &= 0xF0;
		data += 0x10;
		if(data == 0x0A)
		{
			data = 0x00;
		}
	}
	return data;
}
int retrynum = 0;

bool SetFindScanMeterAddr(u8 mode)
{
	retrynum = 0;
    bool flg = true;
    switch(mode)
    {
        case INIT_SCAN_ADDR :
            memset(FindScanMeterAddr,0xAA,LONG_ADDR_SIZE);
            FindScanMeterAddrIndex = 0x00;
            break;
        case CARRY_SCAN_ADDR :
            if(FindScanMeterAddrIndex < LONG_ADDR_SIZE - 1)
            {
                if(FindScanMeterAddr[FindScanMeterAddrIndex] != 0xAA)
                {
                    FindScanMeterAddrIndex ++;
                }
                FindScanMeterAddr[FindScanMeterAddrIndex] = 0x00;
            }
            else //出错了，有两个地址一样的表存在
        case INCREASE_SCAN_ADDR :
            {
                while(1)
                {//增加下一个表地址
                    if(FindScanMeterAddr[FindScanMeterAddrIndex] == 0x99)
                    {
                        if(FindScanMeterAddrIndex == 0x00)
                        {//已经最后一个了
                            flg = false;
                            break;
                        }
                        else
                        {//
                            FindScanMeterAddr[FindScanMeterAddrIndex] = 0xAA;
                            -- FindScanMeterAddrIndex;
                        }
                    }
                    else
                    {
                        FindScanMeterAddr[FindScanMeterAddrIndex] =
                            BcdIncrease(FindScanMeterAddr[FindScanMeterAddrIndex]);
                        break;
                    }
                }
            }
            break;
        default :
            break;
    }
    return flg;
}

void ClashFindScanMeter()
{
    if(SCAN_METER_IDLE != ScanMeterRunStatus)
    {
        if(SetFindScanMeterAddr(CARRY_SCAN_ADDR))
        {
            FindScanMeterStatu = IN_FIND_SCAN;
            FindNexMeterAddr();
        }
        else
        {
            if(SetFindScanMeterAddr(INIT_SCAN_ADDR))
            {
                FindScanMeterStatu = IN_FIND_SCAN;
                FindNexMeterAddr();
            }
            else
            {
                bool scanflg = true;
                SetFindScanMeterAddr(INIT_SCAN_ADDR);
                FindScanMeterStatu = START_FIND_SCAN;
                FindScanMeterUartRate++;
                if(FindScanMeterUartRate >= (sizeof(UartRateEnumList)/sizeof(UartRateEnumList[0])))
                {
                	
                    FindScanMeterUartRate = 0;
                    ScanMeterRunStatus = SCAN_METER_IDLE;//状态清除
                    scanflg = false;
                    //搜表完成
                    //启动列表校验
                    StartCheckMeterList();
                }      
                if(scanflg)
                {
                    FindNexMeterAddr();
                }
            }
        }
    }
}

void FindScanNextMeterAddr()
{
    if(SCAN_METER_IDLE != ScanMeterRunStatus)
    {
        if((FindScanMeterStatu == IN_FIND_SCAN)
                && (SetFindScanMeterAddr(INCREASE_SCAN_ADDR)))
        {
            FindScanMeterStatu = IN_FIND_SCAN;
            FindNexMeterAddr();
        }
        else
        {
            bool scanflg = true;
            SetFindScanMeterAddr(INIT_SCAN_ADDR);
            FindScanMeterStatu = START_FIND_SCAN;
            FindScanMeterUartRate++;
            if(FindScanMeterUartRate >= (sizeof(UartRateEnumList)/sizeof(UartRateEnumList[0])))
            {
				FindScanMeterUartRate = 0;
                if(FindScanMeterType == DLT_07)
                {// DLT_07鎼滄煡瀹屾瘯
                    FindScanMeterType = DLT_07_13;
                }
                else
                {// DLT_97宸叉悳鏌ュ畬姣?
                    ScanMeterRunStatus = SCAN_METER_IDLE;//鐘舵€佹竻闄?
                    scanflg = false;
                    //鎼滆〃瀹屾垚
                    //BrushSystemMeterList();
                    //鍚姩鍒楄〃鏍￠獙
                    StartCheckMeterList();
                }
            }    
            if(scanflg)
            {
                FindNexMeterAddr();
            }
        }
    }
    else
    {
        ScanMeterRunStatus = SCAN_METER_IDLE;//状态清除
    }
}

u8 CMM_isBCD(u8* addr, u8 len)
{
    if(len == 0)
        return 0;
	
    while(len--)
    {
        if((((*addr)&0x0F) > 9)||(((*addr)&0xF0) > 0x90))
        {
            return 0;
        }
        addr++;
    }

    return 1;
}

bool FindMeterCallBackFunction(void * exparam,u8* data,u16 len,u8 ust)
{
    u8 state = ust;
    static bool readmeterflg = false;
    bool readmeterflgtmp = false;
    bool reflag = true;
    if(state == UART_SUCCESS)
    {//正常接收到数据
        if(len >= 12)
        {
            u16 hindex = 0;
			
            if(IsHave645Frame(data,len,&hindex))
            {
               u8 *psrc = &data[hindex];
               u16 flen = len - hindex;
               if(CMM_isBCD(psrc+1, 6))
                {
                    u8 *paddr = & psrc[1];
                    u8 urate = FindScanMeterUartRate;
                    u8 mtype = (FindScanMeterType == DLT_97) ? DLT_97 : DLT_07;
                    if((psrc[8] & 0x40) == 0x40)
                    {
                        mtype = (FindScanMeterType == DLT_07) ? DLT_97 : DLT_07;
                    }
					u8 TempMac[6] = {0};
					memcpy(TempMac,paddr,6);
					TransposeAddrtemp(TempMac);
	                AddSystemMeterToList(TempMac,urate,mtype,UART_8_DATA_BITS,UART_PARITY_EVEN,UART_1_STOP_BITS,UART_TX_TTL);
                    len = psrc[9] + 12;
                    if(flen <= len)
                    {
                        if(FindScanMeterStatu == START_FIND_SCAN)
                        {//全AA读取,有应答进入进位
                            ClashFindScanMeter();
                        }
                        else
                        {//进位，下一个地址
                            //閲嶈瘯2娆★紝闃叉澶氫釜鐢佃〃鍙樉绀哄叾涓竴涓數琛ㄥ洖澶?
                        	if(retrynum < 2)
                        	{
                        		retrynum++;
								FindNexMeterAddr();
                        	}
                        	else
                        	{
                        		FindScanNextMeterAddr();
                        	}
                        }
                    }
                    else
                    {
                        ClashFindScanMeter();
                    }
                }
                else
                {
                    len = psrc[9] + 12;
                    if(flen <= len)
                    {
                        if(FindScanMeterStatu == START_FIND_SCAN)
                        {//全AA读取,有应答进入进位
                            ClashFindScanMeter();
                        }
                        else
                        {
                            if(((psrc[8] == 0x81)&&(psrc[9] >= 0x08)&&(psrc[10] == 0x65)&&(psrc[11] == 0xF3)))//是否为读地址命令，若读地址命令获取数据域地址为表地址；否则发送读取表地址
                            {//获取表地址
                                u8 i;
                                for(i= 0 ;i<6;i++)
                                {
                                    psrc[12+i] -= 0x33;
                                }
								u8 TempMac[6] = {0};
								memcpy(TempMac,& psrc[12],6);
								TransposeAddrtemp(TempMac);
								
                                AddSystemMeterToList(TempMac,FindScanMeterUartRate,DLT_97,UART_8_DATA_BITS,UART_PARITY_EVEN,UART_1_STOP_BITS,UART_TX_TTL);
                                //搜读下一个表地址
                                //閲嶈瘯2娆★紝闃叉澶氫釜鐢佃〃鍙樉绀哄叾涓竴涓數琛ㄥ洖澶?
	                        	if(retrynum < 2)
	                        	{
	                        		retrynum++;
									FindNexMeterAddr();
	                        	}
	                        	else
	                        	{
	                        		FindScanNextMeterAddr();
	                        	}
                            }
                            else if((psrc[8] == 0x93)&&(psrc[9] >= 0x06))
                            {
                                u8 i;
                                for(i= 0 ;i<6;i++)
                                {
                                    psrc[10+i] -= 0x33;
                                }
								u8 TempMac[6] = {0};
								memcpy(TempMac,& psrc[10],6);
								TransposeAddrtemp(TempMac);
								
                                AddSystemMeterToList(TempMac,FindScanMeterUartRate,DLT_07,UART_8_DATA_BITS,UART_PARITY_EVEN,UART_1_STOP_BITS,UART_TX_TTL);
                                //搜读下一个表地址
                                //閲嶈瘯2娆★紝闃叉澶氫釜鐢佃〃鍙樉绀哄叾涓竴涓數琛ㄥ洖澶?
	                        	if(retrynum < 2)
	                        	{
	                        		retrynum++;
									FindNexMeterAddr();
	                        	}
	                        	else
	                        	{
	                        		FindScanNextMeterAddr();
	                        	}
                            }
							//鏀寔闈濨CD鐮佹悳琛?
							else if((psrc[8] == 0x91)&&(psrc[9] >= 0x08))
                            {
                            	u8 *paddr = & psrc[1];
                                u8 TempMac[6] = {0};
								memcpy(TempMac,paddr,6);
								TransposeAddrtemp(TempMac);
				                AddSystemMeterToList(TempMac,FindScanMeterUartRate,DLT_07,UART_8_DATA_BITS,UART_PARITY_EVEN,UART_1_STOP_BITS,UART_TX_TTL);
                                //鎼滆涓嬩竴涓〃鍦板潃
                                if(retrynum < 2)
	                        	{
	                        		retrynum++;
									FindNexMeterAddr();
	                        	}
	                        	else
	                        	{
	                        		FindScanNextMeterAddr();
	                        	}
                            }
                            else
                            {//发送读表地址命令
                                if(readmeterflg == false)
                                {
                                    FindReadMeterAddr();
                                    readmeterflgtmp = true;
                                    readmeterflg = true;
                                }
                                else
                                {
                                    //閲嶈瘯2娆★紝闃叉澶氫釜鐢佃〃鍙樉绀哄叾涓竴涓數琛ㄥ洖澶?
		                        	if(retrynum < 2)
		                        	{
		                        		retrynum++;
										FindNexMeterAddr();
		                        	}
		                        	else
		                        	{
		                        		FindScanNextMeterAddr();
		                        	}
                                }
                            }
                        }
                    }
                    else
                    {
                        ClashFindScanMeter();
                    }
                }
                ust = UART_CLEAR_FIFO;
            }
			
            else
            {//未发现645帧，认为是冲突
                ClashFindScanMeter();
                ust = UART_CLEAR_FIFO;
            }
        }
        else
        {//接收长度不够，继续接收
            ust = UART_NOTHING;
            reflag = false;
        }
    }
    else if((state&UART_CHECK_ERROR) == UART_CHECK_ERROR)
    {
        ClashFindScanMeter();
        ust = UART_CLEAR_FIFO;
    }
    else
    {
        FindScanNextMeterAddr();
        ust = UART_CLEAR_FIFO;
    }
    if(readmeterflgtmp == false)
    {
        readmeterflg = false;
    }
    return reflag;
}

u8 GetReadMeterAddrDlt645(u8 *psd,u8 ptype,u8 *paddr)
{
    u8 rlen = 0;
    u8 addrtmp[6];
    memcpy(addrtmp,paddr,LONG_ADDR_SIZE);

    psd[rlen++] = 0xFE;
    psd[rlen++] = 0x68;
    memcpy(&psd[rlen],addrtmp,LONG_ADDR_SIZE);
    rlen += 6;
    psd[rlen++] = 0x68;

    if(ptype == DLT_07)
    {
        psd[rlen++] = 0x13;
        psd[rlen++] = 0x00;
    }
    else
    {
        psd[rlen++] = 0x01;
        psd[rlen++] = 0x02;
        psd[rlen++] = 0x65;
        psd[rlen++] = 0xF3;
    }
    psd[rlen] = GetCheckSum(&psd[1],rlen-1);
    rlen ++;
    psd[rlen++] = 0x16;
    return rlen;
}

void StartFindScanMeter()
{
    if(SCAN_METER_IDLE == ScanMeterRunStatus)
    {//搜表未来开始，开始搜表
        if(SetFindScanMeterAddr(INIT_SCAN_ADDR))
        {
            ScanMeterRunStatus = SCAN_METER_SCANNING;
            FindScanMeterType = DLT_07;
            FindScanMeterStatu = START_FIND_SCAN;
            FindScanMeterUartRate = 0;
            FindReadMeterAddr();
        }
    }
}


//二采搜表
void StartMeterListTaskhandle()
{
    StartFindScanMeter();
}

bool IsSystemMeter(u8 *pid, uint8_t *baudrate, uint8_t *parity)
{
    u8 i;
    bool flg = false;
    for(i = 0;i < SYSTEM_METER_LIST_NUM;i ++)
    {
        if(SystemParams.SystemMeterList.memter[i].Type != INVALID_METER_TYPE)
        {
            if(0 == memcmp(pid,SystemParams.SystemMeterList.memter[i].Id,LONG_ADDR_SIZE))
            {
            	*baudrate = SystemParams.SystemMeterList.memter[i].UartRate;
				*parity = SystemParams.SystemMeterList.memter[i].UartCbit;
                flg = true;
                break;
            }
        }
    }
    return flg;
}

bool Obj_GetSearchMeter(uint8_t *buf, uint16_t *len)
{
    u8 i;
    int index = 0;
    for(i = 0;i < 20;i ++)
    {
        if(SystemParams.SystemMeterList.memter[i].Type != INVALID_METER_TYPE)
        {
            memcpy(&buf[index*8],SystemParams.SystemMeterList.memter[i].Id,6);
			buf[index*8 +6] = SystemParams.SystemMeterList.memter[i].UartRate;
			buf[index*8 +7] = SystemParams.SystemMeterList.memter[i].UartSbit;
			index++;
        }
    }
	*len = index*8;
    return true;
}

bool GetSystemMeterByIndex(u8 *id,int index)
{
	if(index >= SYSTEM_METER_LIST_NUM) return false;
    if(SystemParams.SystemMeterList.memter[index].Type != INVALID_METER_TYPE)
    {
    	memcpy(id, SystemParams.SystemMeterList.memter[index].Id,LONG_ADDR_SIZE);
        return true;
    }
    return false;
}

unsigned short CalCrc_RTU(unsigned char *Src,unsigned char Length)
{
	unsigned char i, j, crcbit;
	unsigned short crc, crcpoly;
	crc = 0xFFFF; 
	crcpoly = 0xA001; 
	for(i = 0; i < Length; i ++) 
	{ 
		crc ^= Src[i]; 
		for(j = 0; j < 8; j ++) 
		{ 
			crcbit = (unsigned char)(crc & 0x01); 
			crc >>= 1; 
			if(crcbit) 
			{ 
				crc ^= crcpoly; 
			} 
		} 
	} 
	return crc; 
}


bool CurtainsControlCmdSend(uint8_t factory,uint8_t cmd,uint8_t *data,uint16_t datalen)
{
	if(factory == FACTORY_SUNFLOWER)
	{
		uint16_t mac = 0;
		if(data != NULL && datalen >= 2)
		{
			mac = (data[0]<<8) +data[1];
		}
		
		uint8_t buf[20] = {0};
		uint16_t buflen = 0;
		buf[0] = 0x55;
		buf[1] = mac&0xFF;
		buf[2] = (mac>>8)&0xFF;
		
		if(cmd == CURTAINS_OPEN)
		{
			buf[3] = 0x03;
			buf[4] = 0x01;
			buflen = 7;
		}
		else if(cmd == CURTAINS_CLOSE)
		{
			buf[3] = 0x03;
			buf[4] = 0x02;
			buflen = 7;
		}
		else if(cmd == CURTAINS_CONTROL)
		{
			buf[3] = 0x03;
			buf[4] = 0x04;
			if(datalen == 3)
			{
				buf[5] = data[2];
			}
			else
			{
				return false;
			}
			buflen = 8;
		}
		else if(cmd == CURTAINS_STOP)
		{
			buf[3] = 0x03;
			buf[4] = 0x03;
			buflen = 7;
		}
		else if(cmd == CURTAINS_SET_ITINERARY_POINTS)
		{
			buf[3] = 0x03;
			buf[4] = 0x05;
			if(datalen == 3)
			{
				buf[5] = data[2];
			}
			else
			{
				return false;
			}
			buflen = 8;
		}
		else if(cmd == CURTAINS_RUN_TO_ITINERARY_POINTS)
		{
			buf[3] = 0x03;
			buf[4] = 0x06;
			if(datalen == 3)
			{
				buf[5] = data[2];
			}
			else
			{
				return false;
			}
			buflen = 8;
		}
		else if(cmd == CURTAINS_DEL_ITINERARY_POINTS)
		{
			buf[3] = 0x03;
			buf[4] = 0x07;
			buflen = 7;
		}
		uint16_t crc = CalCrc_RTU(buf,buflen-2);
		buf[buflen-1] = (crc>>8)&0xff;
		buf[buflen-2] = crc&0xff;

		char tmp[60] = {0};
		for(int i=0;i<buflen;i++)
		{
			sprintf(&tmp[i*3],"%02x ",buf[i]);
		}
		debug_str(DEBUG_LOG_UPDATA, "rbuf:%s\r\n",tmp);
		ChangeLocalUartBaud_Parity(9600,UART_PARITY_NO);
		UartWrite(buf, buflen);
	}
    return true;
}
#endif

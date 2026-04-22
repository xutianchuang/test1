#include "protocol_includes.h"
#include "common_includes.h"
#include "app_led_IIcai.h"
#include "os.h"

#pragma pack(4)
typedef union {
	PDU_BLOCK pdu_bolck_u;
	LOCAL_PARAM uart_local_u;
	HPLC_MAC_CACH mac_mem_u;
	SEND_PDU_BLOCK send_mem;
}Mem_Union;

//PDU内存池
OS_MEM Mempool;
static Mem_Union Mempdu[MAX_RECEIVE_PDU_NUM];

//发送PDU
//static OS_MEM SendMempool;
//static SEND_PDU_BLOCK SendMempdu[MAX_SEND_PDU_NUM];

//闪烁定时器
bool FlashReceive = false;
static TimeValue ReceiveLedTimer;
bool FlashSend = false;
static TimeValue SendLedTimer;


#define FLASH_TIME 20

void FlashLedTimer(void* arg)
{
#if !defined(II_STA)||defined(I_STA)
	CPU_SR_ALLOC();
	if (FlashReceive)
	{
		if (TimeOut(&ReceiveLedTimer))
		{
#if defined(PROTOCOL_NW_2021)&&!defined(I_STA)
			//本地红灯
			LedWrite(RED_LED, 1);
#else
			LedWrite(GREEN_LED, 0);
#endif		
			FlashReceive = false;
		}
	}
	CPU_CRITICAL_ENTER();
	if (FlashSend)
	{
		if (TimeOut(&SendLedTimer))
		{
#if defined(PROTOCOL_NW_2021)&&!defined(I_STA)
			//远程绿灯
			LedWrite(GREEN_LED, 0);
#else		
			LedWrite(RED_LED, 0);
#endif
			FlashSend = false;
		}
	}
	CPU_CRITICAL_EXIT();
#else
#ifdef PROTOCOL_NW_2021 //南网21标准，II采状态红绿双色灯
	CPU_SR_ALLOC();
	if (FlashReceive)
	{
		if (TimeOut(&ReceiveLedTimer))
		{
			RS485_LED_OFF;
			FlashReceive = false;
		}
	}
	CPU_CRITICAL_ENTER();
	if (FlashSend)
	{
		if (TimeOut(&SendLedTimer))
		{
			PLC_LED_OFF;
			FlashSend = false;
		}
	}
	CPU_CRITICAL_EXIT();
#endif	
#endif
}

//初始化
void HPLC_ProtocolInit(void)
{
    //内存池初始化
    OS_ERR err;
    OSMemCreate(&Mempool,"Block Memory",Mempdu, MAX_RECEIVE_PDU_NUM, sizeof(Mem_Union), &err);
    if(err != OS_ERR_NONE)
    {
        while(1);
    }
    
    //发送内存池初始化
//    OSMemCreate(&SendMempool,"Send Block Memory",SendMempdu, MAX_SEND_PDU_NUM, sizeof(SEND_PDU_BLOCK), &err);
//    if(err != OS_ERR_NONE)
//    {
//        while(1);
//    }
    
	HPLC_PHY_Init();
	HRF_PHY_Init();
    HPLC_InitTmr();
    HPLC_RegisterTmr(FlashLedTimer,0,50,TMR_OPT_CALL_PERIOD);
    ReadStaParaInfoOnly();
    HRF_PHY_SetChannel(HRFCurrentIndex&0xff, HRFCurrentIndex>>8);//flash中读取后再重新设置一下功率和频段
	HPLC_PHY_SetFrequence(HPLCCurrentFreq);//flash中读取后再重新设置一下功率和频段
    AppInit();
    MacInit();
    MpduInit();
    RouteEventInit();
    HPLC_Task_Init();
    HPLC_SendTask_Init();
	MpduTMIInit();
	NIDInit();
    OfflineInit();
    VCS_Init();
	ZeroCrossManagerInit();

	AddZeroCrossCallback(ZC_Handler);
	CleanAllData();
}

//分配一个PDU
PDU_BLOCK* MallocPDU(void)
{
    OS_ERR err;
    PDU_BLOCK *mem_ptr = 0;
    mem_ptr = (PDU_BLOCK*)OSMemGet(&Mempool,&err);
    if(err != OS_ERR_NONE){
#ifdef __DEBUG_MODE
			while(1)
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

//释放一个PDU
void FreePDU(PDU_BLOCK* pdu)
{
    OS_ERR err;
    OSMemPut(&Mempool,pdu,&err);
}

//分配一个发送PDU
SEND_PDU_BLOCK* MallocSendPDU(void)
{
	OS_ERR err;
	SEND_PDU_BLOCK *mem_ptr = 0;
	mem_ptr = (SEND_PDU_BLOCK *)OSMemGet(&Mempool, &err);
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

//释放一个发送PDU
void FreeSendPDU(SEND_PDU_BLOCK* pdu)
{
    OS_ERR err;
    OSMemPut(&Mempool,pdu,&err);
}

//闪烁接收灯
void FlashReceiveLed(void)
{
#if !defined(II_STA)||defined(I_STA)
	//const ST_STA_SYS_TYPE *psSysType = GetStaSysPara();
	
	if (StaFixPara.ledMode)
	{
#ifdef PROTOCOL_NW_2021
		//远程绿灯
		LedWrite(GREEN_LED, 1);
#else	
		LedWrite(RED_LED, 1);
#endif
		if (!FlashSend)
		{
			StartTimer(&SendLedTimer, FLASH_TIME);
			FlashSend = true;
		}
	}
	else
	{
		LedWrite(GREEN_LED, 1);

		if (!FlashReceive)
		{
			if (IsBindLocalMac()) //在未得到mac地址时不闪烁接受灯
			{
				StartTimer(&ReceiveLedTimer, FLASH_TIME);
				FlashReceive = true;
			}
		}
	}
#else
#ifdef PROTOCOL_NW_2021 //南网21标准，II采状态绿灯（PLC通信）
	PLC_LED_ON;
	if (!FlashSend)
	{
		StartTimer(&SendLedTimer, FLASH_TIME);
		FlashSend = true;
	}
#endif
#endif
}

//闪烁发送灯
void FlashSendLed(void)
{
#if !defined(II_STA)||defined(I_STA)
    CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
#if defined(PROTOCOL_NW_2021)&&!defined(I_STA)
	//远程绿灯
	LedWrite(GREEN_LED, 1);
#else
	LedWrite(RED_LED, 1);
#endif
    if(!FlashSend)
    {
        StartTimer(&SendLedTimer, FLASH_TIME);
        FlashSend = true;
    }
	CPU_CRITICAL_EXIT();
#else	
#if defined(PROTOCOL_NW_2021)&&!defined(I_STA) //南网21标准，II采状态绿灯（PLC通信）
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
	PLC_LED_ON;
    if(!FlashSend)
    {
        StartTimer(&SendLedTimer, FLASH_TIME);
        FlashSend = true;
    }
	CPU_CRITICAL_EXIT();
#endif
#endif
}

//本地LED闪烁
void FlashLocalLed(void)
{
#if !defined(II_STA)||defined(I_STA)
	//const ST_STA_SYS_TYPE *psSysType = GetStaSysPara();
	
	if (StaFixPara.ledMode)
	{
#if defined(PROTOCOL_NW_2021)&&!defined(I_STA)
		//本地红灯
		LedWrite(RED_LED, 0);
#else
		LedWrite(GREEN_LED, 1);
#endif
		if (!FlashReceive)
		{
			if (IsBindLocalMac()) //在未得到mac地址时不闪烁接受灯
			{
				StartTimer(&ReceiveLedTimer, FLASH_TIME);
				FlashReceive = true;
			}
		}
	}
#else
#if defined(PROTOCOL_NW_2021)&&!defined(I_STA) //南网21标准，II采状态红灯（485通信）
	RS485_LED_ON;

	if (!FlashReceive)
	{
		StartTimer(&ReceiveLedTimer, FLASH_TIME);
		FlashReceive = true;
	}
#endif
#endif
}


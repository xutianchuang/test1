#include "System_inc.h"
#include "os.h"
#include "common_includes.h"
#include <string.h>

#define LOCAL_BUFF_SIZE     256

#define BIND_ADDR_TOTAL_TIMEOUT     10*1000UL

#define MAX_LOCAL_PDU_SIZE      13

typedef struct
{
    u8 pdu[256];
    u16 size;
}LOCAL_PDU;

//接收数据信号量
static OS_SEM ReceiveComplateSEM;

//本地接收数据队列
static OS_Q LocalDataQueue;

//是否已经绑定地址
static bool IsBindAddr = false;

//绑定地址是否超时
static bool IsBindAddrTimeout = false;

//本地电能表的协议
static u8 MeterProtocol = LOCAL_PRO_DLT645_2007;

//地址模式
static u8 BindingAddrModeCnt = 0;

//是否已经接收完成
bool LocalReceiveFlg = false;

//绑定地址超时定时器
static TimeValue bindTimer;

//绑定地址总超时定时器（超过1分钟，将把超时标志置位）
static TimeValue bindTotalTimer;

//接收超时定时器
static TimeValue receiveTimer;

//P内存池
static OS_MEM LocalMempool;
static LOCAL_PDU LocalMempdu[MAX_LOCAL_PDU_SIZE];

//接收缓存
static u8 LocalReceiveBuf[LOCAL_BUFF_SIZE] = { 0 };
static u16 LocalReceiveLen = 0;

UartParameterType Usart1Para;

//发送完成通知
static void UsartSendComplate()
{
    
}

//接收完成通知
static void UsartReceiveComplate()
{
    OS_ERR err;
    OSSemPost(&ReceiveComplateSEM,OS_OPT_POST_ALL,&err);
}

//改变波特率
void ChangeLocalUartBaud(u32 baud)
{
    UartClose();
    Usart1Para.BaudRate = baud;
    Usart1Para.DataBits = 8;
    Usart1Para.Parity = Parity_Even;
    Usart1Para.StopBits = Stop_Bits_One;
    Usart1Para.RxHaveDataFunction = UsartReceiveComplate;
    Usart1Para.TxDataCompleteFunction = UsartSendComplate;
    Usart1Para.RxParityErrorFunction = NULL;
    UartOpen(&Usart1Para);
}

void LocalInit(void)
{
    OS_ERR err;
    ChangeLocalUartBaud(2400);
    StartTimer(&bindTimer,0);
    StartTimer(&receiveTimer,0);
    OSSemCreate(&ReceiveComplateSEM,"Receive SEM",0,&err);
    if(err != OS_ERR_NONE)
        while(1);
    StartTimer(&bindTotalTimer,BIND_ADDR_TOTAL_TIMEOUT);
    
    //内存池初始化
    OSMemCreate(&LocalMempool,"Local Block Memory",LocalMempdu, MAX_LOCAL_PDU_SIZE, sizeof(LOCAL_PDU), &err);
    if(err != OS_ERR_NONE)
        while(1);
}


//分配一个PDU
static LOCAL_PDU* MallocLocal(void)
{
    OS_ERR err;
    LOCAL_PDU *mem_ptr = 0;
    mem_ptr = (LOCAL_PDU*)OSMemGet(&LocalMempool,&err);
    if(err != OS_ERR_NONE)
        return (void*)0;
    return mem_ptr;
}

//释放一个PDU
static void FreeLocal(LOCAL_PDU* pdu)
{
    OS_ERR err;
    OSMemPut(&LocalMempool,pdu,&err);
}


//绑定地址任务
void BindingAddrTask(void)
{
      BindingAddrModeCnt++;
      const u8* BindAddrFrame = 0;
      u16 BindAddrFrameLen = 0;
      switch(BindingAddrModeCnt)
      {
          case 1://2400-07
              BindAddrFrame = ReadMeterAddr07;
              BindAddrFrameLen = sizeof(ReadMeterAddr07);
              MeterProtocol = LOCAL_PRO_DLT645_2007;
              ChangeLocalUartBaud(2400);
          break;
          case 2://9600-698
              BindAddrFrame = ReadMeterAddr698;
              BindAddrFrameLen = sizeof(ReadMeterAddr698);
              MeterProtocol = LOCAL_PRO_698_45;
              ChangeLocalUartBaud(9600);
          break;
          case 3://4800-07
              BindAddrFrame = ReadMeterAddr07;
              BindAddrFrameLen = sizeof(ReadMeterAddr07);
              MeterProtocol = LOCAL_PRO_DLT645_2007;
              ChangeLocalUartBaud(4800);
          break;
          case 4://4800-698
              BindAddrFrame = ReadMeterAddr698;
              BindAddrFrameLen = sizeof(ReadMeterAddr698);
              MeterProtocol = LOCAL_PRO_698_45;
              ChangeLocalUartBaud(4800);
          break;
          case 5://2400-698
              BindAddrFrame = ReadMeterAddr698;
              BindAddrFrameLen = sizeof(ReadMeterAddr698);
              MeterProtocol = LOCAL_PRO_698_45;
              ChangeLocalUartBaud(2400);
          break;
          case 6://9600-07
              BindAddrFrame = ReadMeterAddr07;
              BindAddrFrameLen = sizeof(ReadMeterAddr07);
              MeterProtocol = LOCAL_PRO_DLT645_2007;
              ChangeLocalUartBaud(9600);
          break;
          case 7://1200-07
              BindAddrFrame = ReadMeterAddr07;
              BindAddrFrameLen = sizeof(ReadMeterAddr07);
              MeterProtocol = LOCAL_PRO_DLT645_2007;
              ChangeLocalUartBaud(1200);
          break;        
          case 8://1200-698
              BindAddrFrame = ReadMeterAddr698;
              BindAddrFrameLen = sizeof(ReadMeterAddr698);
              MeterProtocol = LOCAL_PRO_698_45;
              ChangeLocalUartBaud(1200);
          break;
          case 9://2400-97
              BindAddrFrame = ReadMeterAddr97;
              BindAddrFrameLen = sizeof(ReadMeterAddr97);
              MeterProtocol = LOCAL_PRO_DLT645_1997;
              ChangeLocalUartBaud(2400);
          break;
          case 10://1200-97
              BindAddrFrame = ReadMeterAddr97;
              BindAddrFrameLen = sizeof(ReadMeterAddr97);
              MeterProtocol = LOCAL_PRO_DLT645_1997;
              ChangeLocalUartBaud(1200);
          break;
          case 11://4800-97
              BindAddrFrame = ReadMeterAddr97;
              BindAddrFrameLen = sizeof(ReadMeterAddr97);
              MeterProtocol = LOCAL_PRO_DLT645_1997;
              ChangeLocalUartBaud(4800);
          break;
          case 12://9600-97
              BindAddrFrame = ReadMeterAddr97;
              BindAddrFrameLen = sizeof(ReadMeterAddr97);
              MeterProtocol = LOCAL_PRO_DLT645_1997;
              ChangeLocalUartBaud(9600);
          break;
          default://2400-07
              BindAddrFrame = ReadMeterAddr07;
              BindAddrFrameLen = sizeof(ReadMeterAddr07);
              MeterProtocol = LOCAL_PRO_DLT645_2007;
              ChangeLocalUartBaud(2400);
              BindingAddrModeCnt = 0;
          break;
      }
      UartWrite((uint8_t*)BindAddrFrame,BindAddrFrameLen);
}

//接收APP层的数据
bool AppLocalReceiveData(u8* data,u16 len)
{
    OS_ERR err;
    LOCAL_PDU* newPdu = MallocLocal();
    if(newPdu == NULL)
        return false;
    memcpy(newPdu->pdu,data,len);
    newPdu->size = len;
    OSQPost(&LocalDataQueue,newPdu,sizeof(LOCAL_PDU),OS_OPT_POST_FIFO | OS_OPT_POST_ALL,&err);
    return true;
}

//返回是否已经绑定了地址
bool IsBindLocalMac(void)
{
    return IsBindAddr;
}

//绑定地址是否已经超时(目前设定为1分钟)
bool IsBindLocalTimeout(void)
{
    return IsBindAddrTimeout;
}

//返回电能表协议
u8 GetMeterProtocol(void)
{
    return MeterProtocol;
}

//本地帧处理
void AppLocalHandle(void)
{
    OS_ERR err;
    u16 index = 0;
    OSSemPend (&ReceiveComplateSEM,1,OS_OPT_PEND_BLOCKING,0,&err);
    if(err == OS_ERR_NONE)
    {
        u32 len = UartRead(&LocalReceiveBuf[LocalReceiveLen],LOCAL_BUFF_SIZE-LocalReceiveLen);
        LocalReceiveLen += len;
        StartTimer(&receiveTimer,20);
        if(MeterProtocol == LOCAL_PRO_698_45)
        {
            if(!CheckCrc16(&LocalReceiveBuf[index],LocalReceiveLen))
            {
                return;
            }
        }
        else
        {
            if(!CheckSum(&LocalReceiveBuf[index],LocalReceiveLen))
            {
                return;
            }
        }
        for(;index < LocalReceiveLen;index++)
        {
            if(LocalReceiveBuf[index] != 0xFE)
                break;
        }
        if(!IsBindAddr)
        {  
            switch(MeterProtocol)
            {
                case LOCAL_PRO_DLT645_1997:
                {
                    SetStaMAC(&LocalReceiveBuf[index + 1]);
                    IsBindAddr = true;
                    break;
                }
                case LOCAL_PRO_DLT645_2007:
                {
                    if(LocalReceiveBuf[index + 8] == 0x93)
                    {
                        SetStaMAC(&LocalReceiveBuf[index + 1]);
                        IsBindAddr = true;
                    }
                    break;
                }
                case LOCAL_PRO_698_45:
                {
                    SetStaMAC(&LocalReceiveBuf[index + 5]);
                    IsBindAddr = true;
                    break;
                }
                default:break;
            }
        }
        else
            AppHandleLocal(&LocalReceiveBuf[index],LocalReceiveLen - index);
        
        LocalReceiveLen = 0;
    }
    else
    {
        if(LocalReceiveLen)
        {
            if(TimeOut(&receiveTimer))
            {
                LocalReceiveLen = 0;
            }
        }
    }
}

//应用层本地处理任务
void AppLocalTask(void *arg)
{
    OS_ERR err;
    OS_MSG_SIZE msgSize;
    LOCAL_PDU* pdu = 0;
    OSQCreate(&LocalDataQueue,"LocalDataQueue",10,&err);
    if(err != OS_ERR_NONE)
        while(1);
    LocalInit();
    OSTimeDlyHMSM(0,0,0,1000,OS_OPT_TIME_DLY,&err);
    while(1)
    {
        if(!IsBindAddr)
        {
            if(TimeOut(&bindTimer))
            {
                BindingAddrTask();
                StartTimer(&bindTimer,2000);
            }
            if((!IsBindAddrTimeout) && TimeOut(&bindTotalTimer))
            {
                IsBindAddrTimeout = true;
            }
            OSTimeDlyHMSM(0,0,0,20,OS_OPT_TIME_DLY,&err);
        }
        else
        {
            pdu = OSQPend (&LocalDataQueue,20,OS_OPT_PEND_BLOCKING,&msgSize,0,&err);
            if(err == OS_ERR_NONE)
            {
                while(IsUartTxData())
                    OSTimeDlyHMSM(0,0,0,1,OS_OPT_TIME_DLY,&err);
                UartWrite(pdu->pdu,pdu->size);
                while(IsUartTxData());
                    OSTimeDlyHMSM(0,0,0,1,OS_OPT_TIME_DLY,&err);
                FreeLocal(pdu);
            }
        }
        AppLocalHandle();
        
    }
}


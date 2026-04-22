#include "app_new_task.h"
#include "protocol_includes.h"
#include "os.h"
#include "common_includes.h"
#include <string.h>
#include "bsp.h"
#include "Revision.h"
#include "applocal.h"
OS_TCB  App_NewTCB;
CPU_STK  App_NewStk[APP_CFG_TASK_NEW_STK_SIZE];



//回调函数
void AppMangeBack(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8* data, u16 len, u8 sendType, u16 srcTei)
{
	OS_ERR err;
	//可以做一些处理
	OSTaskSemPost(&App_NewTCB, OS_OPT_POST_1, &err);
}
//直接组包发送回调
void AppMangeBack2(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8* data, u16 len, u8 sendType, u16 srcTei)
{
	AppHandleReadMeter(srcAddr, dstAddr, seq, vaild, data, len, sendType,srcTei);
}

//数据分发函数
bool AppMange(u8 srcAddr[6], u8 dstAddr[6], u8 timeout, u16 seq, u8* data, u16 len, u8 sendType, u16 srcTei)
{
	int index;
	for (index = 0; index < len; index++)
	{
		if (data[index] == 0x68)
		{
			if (Is645Frame(&data[index], len-index)) 
			{

				break;
			}
		}
	}
	
	if (index >= len)
	{
		if(false)//判断是否是新增应用的帧
		{
			if(false)//直接发到TASK
			{
				OS_ERR err;
				LOCAL_PARAM* localPdu = MallocLocal(); 
				if(localPdu == NULL)
					return false;
				memcpy(localPdu->SrcAddr,srcAddr,6);
				memcpy(localPdu->DstAddr,dstAddr,6);
				localPdu->Timeout = timeout;
				if(localPdu->Timeout > 100)
					localPdu->Timeout = 100;
				localPdu->Seq = seq;
				localPdu->SendType = sendType;
				localPdu->SrcTei = srcTei;
				if(data && len <= MAX_LOCAL_LEN)
					memcpy(localPdu->Data,data,len);
				localPdu->Size = len;
				localPdu->fun = NULL;
				
				OSTaskQPost(&App_NewTCB,localPdu,sizeof(LOCAL_PARAM),OS_OPT_POST_FIFO | OS_OPT_POST_ALL,&err);
				if(err != OS_ERR_NONE)
				{
					FreeLocal(localPdu);
					return false;
				}
				return true;
			}
			else if(false)//直接组包发送
			{
				//根据要求组帧645 然后发送
				AppHandleReadMeter(srcAddr, dstAddr, seq, true, data, len, sendType,srcTei);
			}
			else //直接抄表
			{
				AppLocalReceiveData(srcAddr, dstAddr, timeout, seq, data, len, AppMangeBack2, sendType, srcTei, BAUD_SEARCH);
			}
		}
	}
	return false;
}
//新增应用处理函数
void AppNewTask(void *arg)
{
	OS_ERR err;
	//等待获取完电表类型
	do
	{
		OSTimeDly(100, OS_OPT_TIME_DLY, &err);
	} while (3!=DectecDoublePortocol);
	for(;;)
	{
		OS_MSG_SIZE size;
		LOCAL_PARAM* pData=OSTaskQPend(100, OS_OPT_PEND_ABORT_1, &size,NULL, &err);
		if(err==OS_ERR_NONE)
		{
			//可以进行抄表
			//AppLocalReceiveData(srcAddr, dstAddr, timeout, seq, data, len, AppMangeBack2, sendType, srcTei);
			//可以进行直接组包发送

			//最后需要释放内存
			FreeLocal(pData);
		}
		if(false)//需要上报事件
		{
			EVENT_AddTolist(NULL);
		}
	}
}


void NewTaskInit(void)
{
	OS_ERR err;
	OSTaskCreate(&App_NewTCB,
             "Nw new Task",
              AppNewTask,
              0,
              APP_CFG_TASK_NEW_PRIO,
              App_NewStk,
              APP_CFG_TASK_NEW_STK_SIZE / 10u,
              APP_CFG_TASK_NEW_STK_SIZE,
              5u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &err); 
}

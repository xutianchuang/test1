/*
*******************************************************************************
**
**  This device driver was created by Applilet2 for 78K0R/Kx3
**  16-Bit Single-Chip Microcontrollers
**
**  Copyright(C) NEC Electronics Corporation 2002 - 2007
**  All rights reserved by NEC Electronics Corporation.
**
**  This program should be used on your own responsibility.
**  NEC Electronics Corporation assumes no responsibility for any losses
**  incurred by customers or third parties arising from the use of this file.
**
**  Filename :  main.c
**  Abstract :  This file implements main function.
**  APIlib :    Applilet2 for 78K0R/Kx3 V2.10 [31 Jan. 2007]
**
**  Device :    uPD78F1166_A0
**
**  Compiler :  CC78K0R
**
**  Creation date:  2007-12-27
**
*******************************************************************************
*/

/*
*******************************************************************************
** Include files
*******************************************************************************
*/

#include "system_inc.h"
#include "uart_link.h"
#include "alert.h"
#include "plm_2005.h"
#include "os_cfg_app.h"
u8 uart_main_copy[COM_PORT_BUFFER_SIZE];
PBYTE  g_ComPortBuffer[3]={NULL,NULL,uart_main_copy};
USHORT g_ComPortLen[3];

u32 s_baudRate = 9600;


/***********************************************************
END层初始化< // 需要提供一个reset 接口供上层调用>
************************************************************/

extern USHORT AMR_RS485_postProcess(pvoid h);
void SYS_task_init();
#ifdef HPLC_CSG
static P_MSG_INFO pChangeSend=NULL;
static u32 changeRate=0;
void End_ChangeRateSend(u8 *data,u16 len,u32 rate)
{
	if (pChangeSend!=NULL)
	{
		return;
	}
	pChangeSend = (P_MSG_INFO)alloc_send_buffer_by_len(__func__,(u16)len);
	if (pChangeSend==NULL)
	{
		return ;
	}
	pChangeSend->msg_header.end_id = COM_PORT_MAINTAIN;
	pChangeSend->msg_header.msg_len = len;
	memcpy(pChangeSend->msg_buffer, data, len);
	changeRate=rate;
	End_post(pChangeSend);

}
#endif
//发送完成通知
static void OnUart1SendComplate(int arg_null)
{
    OS_ERR err;
//    if(0==g_pOsRes->sem_wait_send_complete.Ctr)    
        OSSemPost(&g_pOsRes->sem_wait_send_complete,  OS_OPT_POST_NO_SCHED, &err);
}

//接收完成通知
static void OnUart1ReceiveComplate(int len)
{
    OS_ERR err;


    P_TMSG pMsg = NULL;
    P_MSG_INFO pSendMsg = NULL;
    
    pSendMsg = alloc_send_buffer_by_len(__func__,len);
    if(NULL == pSendMsg)
        return;

    pSendMsg->msg_header.msg_len = UartRead(pSendMsg->msg_buffer, len);
    pSendMsg->msg_header.end_id = COM_PORT_MAINTAIN;

    pMsg = alloc_msg(__func__);
    if(NULL == pMsg)
        goto L_Error;

    pMsg->message = MSG_PLM_FRAME;
    pMsg->lParam = (LPARAM)pSendMsg;

    OSTaskQPost(&g_pOsRes->APP_PLM_2005_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
    if(OS_ERR_NONE != err)
        goto L_Error;

    return;

L_Error:
    free_msg(__func__,pMsg);
    free_send_buffer(__func__,pSendMsg);

//    OS_ERR err;
//    OSSemPost(&ReceiveComplateSEM,OS_OPT_POST_ALL,&err);
}

void End_init(void)
{
#if defined(DEBUG_PORT_CONCENTRATOR) || defined(FUNC_HPLC_READER)
    End_SetBaudrate(520000, Parity_Even);

#elif defined(PERFORMAINCE_OUT)
	End_SetBaudrate(115200, Parity_Even);

#elif defined(GDW_CENSORSHIP)//送检的程序115200 波特率是无校验的

    if(g_PlmConfigPara.baud_rate == 115200)
	{
    	End_SetBaudrate(115200, Parity_No);
	}
	else
	{
		End_SetBaudrate(g_PlmConfigPara.baud_rate, Parity_Even);
	}
#else
	End_SetBaudrate(g_PlmConfigPara.baud_rate, Parity_Even);
    //End_SetBaudrate(520000, Parity_Even);
#endif
/*
    u8* pData = alloc_buffer(__func__,100);
    memset_special(pData, 0x55, 100);
    UartWrite(pData, 100);
    //free_buffer(__func__,pData);
*/

#if 0
    P_END_OBJ pEndObj = NULL;

    unsigned char i;

    P_MSG_INFO  pnewmsg = NULL;

    {
        extern unsigned char g_com_port_buffer[COM_PORT_BUFFER_SIZE];
        g_ComPortBuffer[0] = (PBYTE)&g_com_port_buffer[0];
        g_ComPortLen[0] = 0;
    }

    //alan test  需要暂时注释掉, 不知为啥IIC Start 一调用, MCU 就飞啦.
    for( i = COM_PORT_PLC; i < COM_PORT_MAX; i++)
    {
        // 找到当前End Object
        pEndObj = g_EndObjectPool + i;

        /* end queue[x] initialize */ /* each end object define '50 block' queue */
        g_EndTxQueue[i] = define_new_queue((queue *)&EndTxQueueMem[i], END_TX_QUEUE_SIZE);
        g_EndRxQueue[i] = define_new_queue((queue *)&EndRxQueueMem[i], END_TX_QUEUE_SIZE);

        if( (g_EndTxQueue[i] == NULL) || (g_EndRxQueue[i] == NULL) )
        {
            Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);
        }

        if ( pEndObj->end_id == COM_PORT_PLC )
            pnewmsg = alloc_send_buffer(__func__,MSG_SHORT);
        else if ( pEndObj->end_id == COM_PORT_MAINTAIN )
            pnewmsg = alloc_send_buffer(__func__,MSG_LONG);
#ifdef APP_WIRELESS_EN
        else if ( pEndObj->end_id == COM_PORT_WIRELESS )
            pnewmsg = alloc_send_buffer(__func__,MSG_LONG);
#endif
#if defined(APP_WITH_PLC_WB)
        else if ( pEndObj->end_id == COM_PORT_WB )
            pnewmsg = alloc_send_buffer(__func__,MSG_SHORT);
#endif
        pnewmsg->msg_header.end_id = pEndObj->end_id;

        pEndObj->end_recv_buffer = pnewmsg;

        pEndObj->last_receive_len = 0;
        pEndObj->receive_len = 0;

        pEndObj->recv_timeout = 0;

        // 注册接收数据buffer 入口和最大长度
        if(pEndObj->RecvData)
        {
            if ( pEndObj->end_id == COM_PORT_PLC )
                pEndObj->RecvData(pEndObj->end_recv_buffer->msg_buffer, PLC_BUFFER_UART_SIZE );
            else if ( pEndObj->end_id == COM_PORT_MAINTAIN )
                pEndObj->RecvData(pEndObj->end_recv_buffer->msg_buffer, MSA_MESSAGAE_MAX_SIZE );
#ifdef APP_WIRELESS_EN
            else if ( pEndObj->end_id == COM_PORT_WIRELESS)
                pEndObj->RecvData(pEndObj->end_recv_buffer->msg_buffer, MSA_MESSAGAE_MAX_SIZE );
#endif
#if defined(APP_WITH_PLC_WB)
            else if ( pEndObj->end_id == COM_PORT_WB )
                pEndObj->RecvData(pEndObj->end_recv_buffer->msg_buffer, PLC_BUFFER_UART_SIZE );
#endif
        }

        // 所有串口状态转到REVC STATUS
        pEndObj->end_send_status = END_STATUS_IDLE;

        // 注意: 这里暂时在Start() 函数中初始化UART rx buf & max lenght.   暂时从MsgPool 上取得前几帧作为接收缓存区.
        // Fram - CSI01_Start()
        if(pEndObj->Start)
            pEndObj->Start();
    }
#endif
}

void End_SetBaudrate(u32 bps, u32 Parity)
{
	debug_str(DEBUG_LOG_NET, "表串口=%d, Parity=%d\r\n", bps, Parity);
    s_baudRate = bps;
    
    UartParameterType Usart1Para;
    UartClose();
    Usart1Para.BaudRate = bps;
    Usart1Para.DataBits = ARM_USART_DATA_BITS_9;
    Usart1Para.Parity = Parity;
    //Usart1Para.StopBits = Stop_Bits_One;
    Usart1Para.RxHaveDataFunction = OnUart1ReceiveComplate;
    Usart1Para.TxDataCompleteFunction = OnUart1SendComplate;
    Usart1Para.RxParityErrorFunction = NULL;
    UartOpen(&Usart1Para);
}
#ifdef HPLC_CSG
static void End_SetBaudrateOnce(u32 bps)
{
    
    UartParameterType Usart1Para;
    UartClose();
    Usart1Para.BaudRate = bps;
    Usart1Para.DataBits = 8;
    Usart1Para.Parity = Parity_Even;
    //Usart1Para.StopBits = Stop_Bits_One;
    Usart1Para.RxHaveDataFunction = OnUart1ReceiveComplate;
    Usart1Para.TxDataCompleteFunction = OnUart1SendComplate;
    Usart1Para.RxParityErrorFunction = NULL;
    UartOpen(&Usart1Para);
}
#endif

/***********************************************************
pEndObj:  发送接口
pMsgInfo: 发送消息内容通过链路层发送消息
************************************************************/
unsigned short End_send( P_MSG_INFO pMsgInfo)
{
    return TRUE;
}

unsigned short End_post(P_MSG_INFO pMsgInfo)
{
#if 1
    if(NULL == pMsgInfo)
        return (!OK);

    OS_ERR err;

    OSTaskQPost(&g_pOsRes->APP_End_TCB, pMsgInfo, sizeof(pMsgInfo), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);

    if(OS_ERR_NONE != err)
    {
        debug_str(DEBUG_LOG_ERR, "End_post error:%d\r\n", err);
        free_send_buffer(__func__,pMsgInfo);
        return (!OK);
    }

    return OK;
#else
    if(NULL == pMsgInfo)
        return (!OK);

    OS_ERR err;

    UartWrite(pMsgInfo->msg_buffer, pMsgInfo->msg_header.msg_len);

    OSSemPend(&g_pOsRes->sem_wait_send_complete, OSCfg_TickRate_Hz, OS_OPT_PEND_BLOCKING, NULL, &err);
    free_send_buffer(__func__,pMsgInfo);

    return OK;
#endif
}

typedef struct{
	OS_TICK_64 send_tick;
	P_MSG_INFO pMsg;
	u32        time;
}REM_UART_MSG;

REM_UART_MSG End_Task_Msg[END_TASK_MSG_NUM];

void ClearWaitAckMsg(u8 seq)
{
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	for (int i=0;i<END_TASK_MSG_NUM;i++)
	{
		u8 msg_seq=0;
		if (End_Task_Msg[i].pMsg)
		{
			#ifdef HPLC_CSG
			P_LMP_LINK_HEAD_GD pFrame_gd = (P_LMP_LINK_HEAD_GD)End_Task_Msg[i].pMsg->msg_buffer;	
			if (pFrame_gd->ctl_field&0x20)
			{
				msg_seq = pFrame_gd->user_data[13];
			}
			else
			{
				msg_seq = pFrame_gd->user_data[1];
			}
			#else
			P_LMP_LINK_HEAD pFrame = (P_LMP_LINK_HEAD)End_Task_Msg[i].pMsg->msg_buffer;
			msg_seq=pFrame->Inf.up_info.seq_no;
			#endif
#if defined(GDW_2012) || defined(HPLC_CSG)	
			if (seq==msg_seq)
#endif
			{
				//不要清除pMsg   pMsg统一在FindTimeoutMsg里面清除  放止多进程问题
				End_Task_Msg[i].send_tick=0;
				End_Task_Msg[i].time=0;
				break;
			}
		}

	}
	OS_CRITICAL_EXIT();
}
REM_UART_MSG* FindTimeoutMsg(void)
{
	CPU_SR_ALLOC();
	OS_TICK_64 tick_now = GetTickCount64();
	OS_CRITICAL_ENTER();
	for (int i=0;i<END_TASK_MSG_NUM;i++)
	{
		if (End_Task_Msg[i].pMsg)
		{
			if (tick_now>End_Task_Msg[i].send_tick)
			{
				if (End_Task_Msg[i].time)
				{
					End_Task_Msg[i].time--;
					OS_CRITICAL_EXIT();
					return &End_Task_Msg[i];
				}
				else
				{
					free_send_buffer(__func__, End_Task_Msg[i].pMsg);
					End_Task_Msg[i].pMsg=NULL;
					End_Task_Msg[i].send_tick=0;
					End_Task_Msg[i].time=0;
				}
			}
		}
	}
	OS_CRITICAL_EXIT();
	return NULL;
}
bool AddTaskMsg(P_MSG_INFO pMsg)
{
	CPU_SR_ALLOC();
	OS_TICK_64 tick_now = GetTickCount64();
	OS_CRITICAL_ENTER();
	for (int i=0;i<END_TASK_MSG_NUM;i++)
	{
		if (End_Task_Msg[i].pMsg==NULL)
		{
			End_Task_Msg[i].send_tick = tick_now+10*OS_CFG_TICK_RATE_HZ;
			End_Task_Msg[i].time = 2;
			End_Task_Msg[i].pMsg = pMsg;
			OS_CRITICAL_EXIT();
			return true;
		}
	}
	OS_CRITICAL_EXIT();
	return false;
}

void  End_Task_Proc(void *p_arg)
{
    OS_ERR err;
    OS_MSG_SIZE msgSize;

    OS_TICK_64 tick_last_snd = 0, tick_now;
	REM_UART_MSG * pRemMsg=NULL;
    while(1)
    {
		P_MSG_INFO pMsgInfo;
		pRemMsg=FindTimeoutMsg();
		if (pRemMsg)
		{
			pMsgInfo = pRemMsg->pMsg;
		}
		else
		{
			pMsgInfo = OSTaskQPend(OSCfg_TickRate_Hz , OS_OPT_PEND_BLOCKING, &msgSize, NULL, &err);
			if(OS_ERR_NONE != err)
			{
				free_send_buffer(__func__,pMsgInfo);
				continue;
			}
		}

        if(NULL == pMsgInfo)
            continue;

		
		//发送间隔控制 100 ms 根据南科院机台修改
        tick_now = GetTickCount64();
		OS_TICK_64 tick_diff = TicksBetween64(tick_now, tick_last_snd);
		if (tick_diff < SEND_UART_WAIT_MS)
		{
			OSTimeDly((SEND_UART_WAIT_MS - (OS_TICK)tick_diff), OS_OPT_TIME_DLY, &err);
		}

        if(COM_PORT_DEBUG==pMsgInfo->msg_header.end_id)
        {
			while(0 != OSSemPend(&g_pOsRes->sem_wait_debug_send_done, OSCfg_TickRate_Hz, OS_OPT_PEND_NON_BLOCKING, NULL, &err));
			DEBUG_UART.Send(pMsgInfo->msg_buffer, pMsgInfo->msg_header.msg_len);
            OSSemPend(&g_pOsRes->sem_wait_debug_send_done, OSCfg_TickRate_Hz*2, OS_OPT_PEND_BLOCKING, NULL, &err);
        }
		else if (COM_PORT_READER==pMsgInfo->msg_header.end_id)
		{
			PLC_SendReadAck(pMsgInfo->msg_buffer, pMsgInfo->msg_header.msg_len);
			g_ComPortBuffer[1]=NULL;

		}
		else
        {
            while(0 != OSSemPend(&g_pOsRes->sem_wait_send_complete, OSCfg_TickRate_Hz, OS_OPT_PEND_NON_BLOCKING, NULL, &err));
#ifdef HPLC_CSG
			if (pChangeSend!=NULL)
			{
				if (pMsgInfo==pChangeSend&&changeRate!=0)
				{
					End_SetBaudrateOnce(changeRate);
				}
			}
#endif
			for (int i=0;i<3;i++)
			{
				OS_TICK_64 tick_start = GetTickCount64();
				debug_str(DEBUG_LOG_3762, "send3762:");
				debug_hex(DEBUG_LOG_3762, pMsgInfo->msg_buffer, pMsgInfo->msg_header.msg_len);	
#ifdef PROTOCOL_NW_2021	
				FlashSendLed();
#endif
				UartWrite(pMsgInfo->msg_buffer, pMsgInfo->msg_header.msg_len);
				OSSemPend(&g_pOsRes->sem_wait_send_complete, OSCfg_TickRate_Hz*2, OS_OPT_PEND_BLOCKING, NULL, &err);

				OS_TICK_64 tick_dis = TicksBetween64(GetTickCount64(), tick_start);
				u32 bits = (u32)pMsgInfo->msg_header.msg_len*10;
				OS_TICK_64 tick_need = (OS_TICK_64)OSCfg_TickRate_Hz * bits / s_baudRate + 5;
#ifdef UART_WAIT_ACK
				OSSemSet(&g_pOsRes->savedata_mbox, 0, &err);
#endif
				if(tick_need > tick_dis)
				{
					OSTimeDly((OS_TICK)(tick_need-tick_dis), OS_OPT_TIME_DLY, &err);
				}
				tick_last_snd = GetTickCount64();
				if (pMsgInfo->msg_header.send_delay) //如果设置了send_delay先等待一个ack再发送
				{
#ifdef UART_WAIT_ACK
					OSSemPend(&g_pOsRes->savedata_mbox, OS_CFG_TICK_RATE_HZ*2, OS_OPT_PEND_BLOCKING, NULL, &err);
					if (err==OS_ERR_NONE)
					{
						break;
					}
#else
					#ifdef HPLC_CSG
					if (pChangeSend != NULL)
					{
						if (pMsgInfo == pChangeSend && changeRate != 0)
						{
							pChangeSend = NULL;
							changeRate = 0;
							End_SetBaudrateOnce(s_baudRate);
						}
					}
					#endif
					if (pRemMsg)
					{
						pRemMsg->send_tick=tick_last_snd+10*OS_CFG_TICK_RATE_HZ;
						pMsgInfo = NULL;//不释放该buffer
					}
					else
					{
						if (AddTaskMsg(pMsgInfo))
						{
							pMsgInfo = NULL;//不释放该buffer
						}
					}
					break;
#endif
				}
				else
				{
					break;
				}
			}
#ifdef HPLC_CSG
			if (pChangeSend!=NULL)
			{
				if (pMsgInfo==pChangeSend&&changeRate!=0)
				{
					pChangeSend=NULL;
					changeRate=0;
					End_SetBaudrateOnce(s_baudRate);
				}
			}
#endif
        }

		free_send_buffer(__func__,pMsgInfo);


    }

#if 0
    unsigned char i, perr;

    P_END_OBJ   pEndObj = NULL;
    P_LONG_MSG_INFO  pMsg = NULL_PTR;

    for (;;)
    {
        //OSTimeDlyHMSM(0, 0, 1, 0); /* Wait one second */
        OSSemPend(e_sem_end, 50, &perr);

        for( i = COM_PORT_PLC; i < COM_PORT_MAX; i++ )
        {
            pEndObj = g_EndObjectPool + i;

            ///////////////////////////////////////////////
            while( ( pMsg = dequeue( g_EndRxQueue[pEndObj->end_id] ) ) != NULL )
            {
                // 进行msg header end_id 加强校验
                if(pMsg->msg_header.end_id == pEndObj->end_id)
                    // 正常接收数据的解析处理入口
                    // End_postProcess(pEndObj->end_id, pMsg->msg_buffer);
                    End_postProcess(pEndObj->end_id, pMsg );

                /* 释放数据块*/
                free_send_buffer(__func__,pMsg);
            }
        }

    }

    //return;
#endif
}

PBYTE GetComPortBufPtr(UCHAR end_id, UCHAR sub_id)
{
    switch(end_id)
    {
#ifdef APP_WIRELESS_EN
        case COM_PORT_WIRELESS:
            return g_ComPortBuffer[0];
#endif
#ifdef HPLC_CSG
	case COM_PORT_READER:
		return g_ComPortBuffer[1];
#endif
	case COM_PORT_MAINTAIN:
		return g_ComPortBuffer[2];
    default:
            return NULL;
    }
}

USHORT* GetComPortLenPtr(UCHAR end_id, UCHAR sub_id)
{
    switch(end_id)
    {
#ifdef APP_WIRELESS_EN
        case COM_PORT_WIRELESS:
            return &g_ComPortLen[0];
#endif
#ifdef HPLC_CSG
		case COM_PORT_READER:
            return &g_ComPortLen[1];
#endif
		case COM_PORT_MAINTAIN:
			return &g_ComPortLen[2];
        default:
            return NULL;
    }
}

//抄控器请求cco数据
void ReaderGetMsg(u8 *data,u16 len)
{
    OS_ERR err;


    P_TMSG pMsg = NULL;
    P_MSG_INFO pSendMsg = NULL;

	pSendMsg = alloc_send_buffer_by_len(__func__,len);
    if(NULL == pSendMsg)
        return;
	memcpy(pSendMsg->msg_buffer,data,len);

    pSendMsg->msg_header.msg_len = len;
	
    pSendMsg->msg_header.end_id = COM_PORT_READER;

    pMsg = alloc_msg(__func__);
    if(NULL == pMsg)
        goto L_Error;

    pMsg->message = MSG_PLM_FRAME;
    pMsg->lParam = (LPARAM)pSendMsg;

    OSTaskQPost(&g_pOsRes->APP_PLM_2005_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
    if(OS_ERR_NONE != err)
        goto L_Error;

    return;

L_Error:
    free_msg(__func__,pMsg);
    free_send_buffer(__func__,pSendMsg);

//    OS_ERR err;
//    OSSemPost(&ReceiveComplateSEM,OS_OPT_POST_ALL,&err);
}

void DebugSendComplate(int a)
{
	OS_ERR err;
	OSSemPost(&g_pOsRes->sem_wait_debug_send_done,  OS_OPT_POST_NO_SCHED, &err);
}
//debug口请求cco数据
void DebugGetMsg(u8 *data,u16 len)
{
    OS_ERR err;


    P_TMSG pMsg = NULL;
    P_MSG_INFO pSendMsg = NULL;

	pSendMsg = alloc_send_buffer_by_len(__func__,len);
    if(NULL == pSendMsg)
        return;
	memcpy(pSendMsg->msg_buffer,data,len);

    pSendMsg->msg_header.msg_len = len;
	
    pSendMsg->msg_header.end_id = COM_PORT_DEBUG;

    pMsg = alloc_msg(__func__);
    if(NULL == pMsg)
        goto L_Error;

    pMsg->message = MSG_PLM_FRAME;
    pMsg->lParam = (LPARAM)pSendMsg;

    OSTaskQPost(&g_pOsRes->APP_PLM_2005_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
    if(OS_ERR_NONE != err)
        goto L_Error;

    return;

L_Error:
    free_msg(__func__,pMsg);
    free_send_buffer(__func__,pSendMsg);


}

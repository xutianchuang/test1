//#include "../hal/macrodriver.h"
#include "type_define.h"
#include "system_inc.h"
#include "manager_link.h"
//#include "../drv/uart_link.h"
//#include "../drv/timers.h"
#include "rtc.h"
#include "alert.h"
//#include "../sys/system_event.h"
//#include "../dev/dataflash/data_flash.h"
#include "queue.h"
//#include "Storage.h"
#include "Module_def.h"
//#include "data_types.h"
//#include "PLM_AFN_HN.H"
//#include "data_freeze.h"
//#include "Event_Detect.h"
//#include "Data_Report.h"
//#include "Data_Freeze.h"
#include "PLC_AutoRelay.h"
//#include "../par/plc_mfrelay.h"
//#include "PLM_Event.h"
#include "plm_2005.h"
//#include "task.h"
//#include <string.h>
#include "PLM_AFN_GD.h"
#include "plm_afn.h"
#include <string.h>
#include "Revision.h"
#include "os_cfg_app.h"
#include "hrf_port.h"
#include "hrf_phy.h"
extern GLOBAL_CFG_PARA g_PlmConfigPara;
extern GLOBAL_STATUS_PARA g_PlmStatusPara;
//extern RELAY_INFO *g_MeterRelayInfo;
extern unsigned char g_file_trans_map[UPDATE_FILE_MAP_SIZE];
extern unsigned char g_updata_firmware_chk[2];
extern queue *g_PlcBroadcastQueue;
extern BOOL s_meter_sn_from_zero;

typedef union
{
	u16 info;
#ifdef HPLC_CSG
	struct {
		u8 phase_info_a:1;
		u8 phase_info_b:1;
		u8 phase_info_c:1;
		u8 phase_features:2;
		u8 phase_sq:3;
		u8 protocol:5;
		u8 :1;
		u8 meter_type:2;
	};
#else
	struct {
		u8 phase_info_a:1;
		u8 phase_info_b:1;
		u8 phase_info_c:1;
		u8 meter_type:1;
		u8 line_error:1;
		u8 phase_sq:3;
		u8 res;
	};
#endif
}meter_phase;
#ifdef NW_TEST
int read_meter_time=0;
#endif
#ifdef PROTOCOL_GD_2016
u16 areaTimerId=INVAILD_TMR_ID;
BYTE afn_func_01_gd(P_LMP_APP_INFO pLmpAppInfo)
{
	USHORT *pWaitTime = (USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];

	*pWaitTime = 1;

	pLmpAppInfo->responsePoint += 2;

	switch (pLmpAppInfo->curDT)
	{
	case 0xE8020101:    //复位硬件
		{
			*pWaitTime = 20;
			Reboot_system(1, en_reboot_cmd, __func__);
			break;
		}
	case 0xE8020102:   //初始化档案
		{
			PLC_ParamReset();
			//*pWaitTime = 30;
			*pWaitTime = 1;
			g_PlmStatusPara.restart_read_flag = TRUE;
			g_pRelayStatus->init_meter_delay=3;//初始化档案需要延时3s后才可以请求集中器时间
			break;
		}
	case 0xE8020103:    //初始化任务
		{
#if 1
			PLC_TaskReset();
			PLC_stage_pause();
#else
			PLC_delete_all_read_task_gd(EM_TASK_TYPE_READ_TASK);
			PLC_delete_all_read_task_gd(EM_TASK_TYPE_MULTI_READ_TASK);
#endif
			return OK;
		}
	case 0xE8020104:    //初始化采集任务
		{
			//g_X4ReadTaskInfo
			return OK;
		}
	default:
		pLmpAppInfo->responsePoint -= 2;
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DT;
		return ERROR;
	}

	return OK;

}

BYTE afn_func_02_gd(P_LMP_APP_INFO pLmpAppInfo)
{
	switch (pLmpAppInfo->curDT)
	{
	case 0xE8020201:    //添加任务
		{
			USHORT  waitTime = 0;
			u8 meter_moudle = 0, app_code = 0, func_code = 0;
			EM_TASK_TYPE task_type=EM_TASK_TYPE_READ_TASK;
//        OS_ERR err;
			u8 mac_addr[MAC_ADDR_LEN];

			P_READ_TASK_HEADER pTaskDown = (P_READ_TASK_HEADER)&pLmpAppInfo->curProcUnit->data_unit[0];
			P_GD_DL645_BODY pDL645_task = NULL;

			if (pLmpAppInfo->appLayerLen <= (6 + 6))       //AFN + SEQ + DI + 任务头
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}
			#ifdef NW_TEST
			read_meter_time++;
			#endif
			//forward_flag转发标识为1时，深化应用数据透传至模块
			//if(pTaskDown->task_mode.forward_flag == 0) 
			{
				pDL645_task = PLC_check_dl645_frame(&pLmpAppInfo->curProcUnit->data_unit[0] + sizeof(READ_TASK_HEADER), pTaskDown->task_len);

#ifdef DEBUG_TASK_DATA
                if (pDL645_task)
                {
                    if (pDL645_task->dataLen >= 4)
            		{
            			debug_str(DEBUG_LOG_APS, "pDL645_task, id:%d, DI:0x%hhx%hhx%hhx%hhx, meter mac:%02x%02x%02x%02x%02x%02x\r\n", pTaskDown->task_id, 
                            pDL645_task->dataBuf[3]-0x33, pDL645_task->dataBuf[2]-0x33, pDL645_task->dataBuf[1]-0x33, pDL645_task->dataBuf[0]-0x33,
                            pDL645_task->meter_number[5], pDL645_task->meter_number[4], pDL645_task->meter_number[3], 
                            pDL645_task->meter_number[2], pDL645_task->meter_number[1], pDL645_task->meter_number[0]);
            		}
                    else
                    {
                        debug_str(DEBUG_LOG_APS, "pDL645_task, id:%d, ctrl:0x%x, len:%d, value:0x%x\r\n", pTaskDown->task_id, pDL645_task->controlCode, pDL645_task->dataLen, pDL645_task->dataBuf[0]);
                    }
                }
#endif			
			}

			if (pLmpAppInfo->lmpLinkHeadGD->ctl_field & 0x20)
			{
				u8 *pAddr = pLmpAppInfo->lmpLinkHeadGD->user_data;
				pAddr += 6;
				memcpy_special(mac_addr, pAddr, MAC_ADDR_LEN);
			}
			else
			{
				if (!pDL645_task)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
					return ERROR;
				}
				memcpy_special(mac_addr, pDL645_task->meter_number, MAC_ADDR_LEN);
			}

			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &waitTime, 2);

			if ((OK == IsEqualSpecialData(mac_addr, 0x99, 6))||
			    (OK == IsEqualSpecialData(mac_addr, 0xAA, 6))||
			    (OK == IsEqualSpecialData(mac_addr, 0xFF, 6)))
			{
				if (pTaskDown->task_mode.forward_flag==0
					&&pDL645_task
					&&(OK != IsEqualSpecialData(pDL645_task->meter_number, 0x99, 6))
					&&(OK != IsEqualSpecialData(pDL645_task->meter_number, 0xAA, 6))
					&&(OK != IsEqualSpecialData(pDL645_task->meter_number, 0xFF, 6)))
				{
					memcpy(mac_addr, pDL645_task->meter_number, MAC_ADDR_LEN);
				}
				else
				{
					//广播任务
					P_MSG_INFO	p_PlcMsg = alloc_send_buffer(__func__,MSG_SHORT);
					if (NULL == p_PlcMsg)
					{
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_TASK_MEMORY_NOT_ENOUGH;
						return ERROR;
					}
					p_PlcMsg->msg_header.meter_moudle = 0;
					p_PlcMsg->msg_header.up_data = pTaskDown->task_mode.resp_flag;					
#ifdef PROTOCOL_NW_2020

					if (pTaskDown->task_mode.forward_flag&&
						pTaskDown->data[0] == APP_SHENHUA_APP_CODE_GRAPH) //业务代码2表示负荷曲线采集与存储
					{					
						u16 meter_1phase_data_len = 0;
						u16 meter_3phase_data_len = 0;
						u16 total_data_len = 0;
						free_send_buffer(__func__, p_PlcMsg);
					    if(pTaskDown->data[1] == APP_GRAPH_FUNC_CODE_CONFIG) //功能码1表示配置采集间隔，广播配置
					    {
					    	//启停标志为停止，且后续没有数据
					    	if((pTaskDown->data[2] == 0)&&(pTaskDown->task_len == 3))
					    	{
					    		total_data_len = pTaskDown->task_len - 1; //不包含业务代码长度
					    	}
							else if((pTaskDown->data[2])&&(pTaskDown->task_len == 3)) //南网21标准，只有采集间隔
					    	{
					    		total_data_len = pTaskDown->task_len - 1; //不包含业务代码长度
					    	}
							else
							{
					    		meter_1phase_data_len = pTaskDown->data[4]*5; //4(数据标识)+1(采集间隔)
								total_data_len = 4 + meter_1phase_data_len; //不包含业务代码长度
							}
							
							if(total_data_len != pTaskDown->task_len-1) //相等则只有一种表类型，则不进行后续解析判断
							{
								meter_3phase_data_len = (*(pTaskDown->data+5+meter_1phase_data_len+1))*5;
								total_data_len = 6 + meter_1phase_data_len + meter_3phase_data_len;

								if(total_data_len != pTaskDown->task_len-1) //长度判断
								{
									pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
									return ERROR;
								}
							}							

							P_STORE_DATA_SYNC_CFG pStoreDataSyncCfg = (P_STORE_DATA_SYNC_CFG)alloc_buffer(__func__, sizeof(STORE_DATA_SYNC_CFG) + total_data_len);
							if (NULL == pStoreDataSyncCfg)
							{
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_BUSY_NOW;
								return ERROR;
							}

							pStoreDataSyncCfg->data_len = total_data_len;
							memcpy(pStoreDataSyncCfg->data, &pTaskDown->data[1], total_data_len);

							PLC_StoreDateSyncCfg(pStoreDataSyncCfg);
							pLmpAppInfo->responsePoint += 2;
							return OK;
					    }
						else
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
							return ERROR;
						}
					}
					else if (pTaskDown->task_mode.forward_flag&&
						     pDL645_task == NULL &&
						     pTaskDown->data[0] == APP_SHENHUA_APP_CODE_TIMING) //业务代码1表示广播校时报文
					{
						P_PLC_BROADCAST_CB pcb = &g_pRelayStatus->broadcast_cb;

						if (NULL != pcb->pSendMsg)
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_BUSY_NOW;
							return ERROR;
						}
						
						p_PlcMsg->msg_header.protocol = METER_PROTOCOL_3762_BROAD_TIME;
						p_PlcMsg->msg_header.meter_moudle = 1; //透传至模块
						p_PlcMsg->msg_header.msg_len = sizeof(local_tm);
						local_tm *p_tm = (local_tm *)p_PlcMsg->msg_buffer;
						memset(p_tm, 0, sizeof(local_tm));
						p_tm->tm_sec = Bcd2HexChar(pTaskDown->data[1]);
						p_tm->tm_min = Bcd2HexChar(pTaskDown->data[2]);
						p_tm->tm_hour = Bcd2HexChar(pTaskDown->data[3]);
						p_tm->tm_mday = Bcd2HexChar(pTaskDown->data[4]);
						p_tm->tm_mon = Bcd2HexChar(pTaskDown->data[5])-1;
						p_tm->tm_year = Bcd2HexChar(pTaskDown->data[6]);
						p_tm->tm_year = p_tm->tm_year + 2000 - 1900;
						OS_TICK_64 real_time = mktime(p_tm);
						p_PlcMsg->msg_header.time_stamp = real_time*OS_CFG_TICK_RATE_HZ - GetTickCount64();					
					}
					else if (pTaskDown->task_mode.forward_flag&&
						     pDL645_task &&
						     pTaskDown->data[0] == APP_SHENHUA_APP_CODE_TIMING)
					{
						p_PlcMsg->msg_header.protocol = METER_PROTOCOL_3762_NEW_BROAD_TIME;
						p_PlcMsg->msg_header.meter_sn = pTaskDown->task_id;
						p_PlcMsg->msg_header.msg_len = pTaskDown->task_len - 1; //数据第一个字节表示业务代码，不添加进载波报文中
						memcpy_special(p_PlcMsg->msg_buffer, ((PBYTE)pTaskDown) + sizeof(READ_TASK_HEADER) + 1, pTaskDown->task_len - 1);
						p_PlcMsg->msg_header.time_stamp =  GetNtbCount64();	
					}
					else
#endif
					{
						if (pDL645_task && pDL645_task->controlCode==0x08)//广播校时
						{
							p_PlcMsg->msg_header.protocol = METER_PROTOCOL_3762_NORMAL_TIME;
						}
						else
						{
							p_PlcMsg->msg_header.protocol = METER_PROTOCOL_3762_BROAD;
						}
						p_PlcMsg->msg_header.meter_sn = pTaskDown->task_id;
						p_PlcMsg->msg_header.msg_len = pTaskDown->task_len;
						memcpy_special(p_PlcMsg->msg_buffer, ((PBYTE)pTaskDown) + sizeof(READ_TASK_HEADER), pTaskDown->task_len);
					}

                    debug_str(DEBUG_LOG_NET, "AddTaskBcast, id:%d, fwd flag:%d, data0:%d, len:%d, timeout:%d, is645:%d\r\n", pTaskDown->task_id,
                        pTaskDown->task_mode.forward_flag, pTaskDown->data[0],
                        pTaskDown->task_len, pTaskDown->task_timeout, (pDL645_task!=NULL)?1:0);
					if(PLC_Broadcast(p_PlcMsg)!=OK)
					{
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_BUSY_NOW;
						return ERROR;
					}

					pLmpAppInfo->responsePoint += 2;
					return OK;
				}
			}
#ifdef PROTOCOL_NW_2020
	#if 0
			if (pDL645_task&&
				pDL645_task->controlCode == 0x14) //设置  集中器可能进行单拨配置则直接进入任务队列  但是置位发送至模块标志
			{
				if (pDL645_task->dataLen == 16
					&& (*(u32 *)pDL645_task->dataBuf & 0xffffff00) == DL645_SET_RECORD_INTERVAL_33) //写负荷记录时间帧
				{
					u8 record_class = pDL645_task->dataBuf[0] - 0x33 - 1; //负荷记录时间从2到7
					if (record_class > 1 && record_class < 8)
					{
						meter_moudle = 1;
					}
				}
			}
			if (pDL645_task
				&&pDL645_task->controlCode == 0x11) //设置  集中器可能进行单拨配置则直接进入任务队列  但是置位发送至模块标志
			{
				if (pDL645_task->dataLen == 0x0a
					&& (*(u32 *)pDL645_task->dataBuf & 0xff00ffff) == DL645_SET_RECORD_READ_33) //写负荷记录时间帧
				{
					u8 record_class = pDL645_task->dataBuf[2] - 0x33; //负荷记录类别
					if (record_class > 0 && record_class < 7)
					{
						meter_moudle = 1;
					}
				}
			}
	#else
			if (pTaskDown->task_mode.forward_flag)
			{
				app_code = pTaskDown->data[0]; //业务代码
#ifndef GRAPH_COLLECT_EXTEND
				if(app_code > APP_SHENHUA_APP_CODE_MAX) //0表示透传645，1表示精准对时，2表示负荷曲线采集与存储
#else
				if(app_code > APP_SHENHUA_APP_CODE_MAX && (is_Extend_ReadCollectGraphPkt(pTaskDown->data,pTaskDown->task_len) == FALSE)) //0表示透传645，1表示精准对时，2表示负荷曲线采集与存储  //0x68：曲线扩展支持645曲线读数据
#endif
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
					return ERROR;
				}

				if (app_code == APP_SHENHUA_APP_CODE_GRAPH)
				{
					func_code = pTaskDown->data[1]; //功能码
				}
				
				if(app_code == APP_SHENHUA_APP_CODE_TIMING)
				{
					OS_TICK_64 cur_tick=0;

					if (pDL645_task==NULL)//深化应用的精准校时
					{
						task_type=EM_TASK_TYPE_TIME_TASK;
						cur_tick=GetTickCount64();
					}
					else  //深圳的新精准校时
					{
						task_type=EM_TASK_TYPE_NEW_TIME_TASK;
						cur_tick=GetNtbCount64();
					}
					memcpy(&pTaskDown->data[pTaskDown->task_len],&cur_tick,sizeof(cur_tick));//把当前的Tick放到数据末尾
					pTaskDown->task_len+=sizeof(cur_tick);
				}
				
				meter_moudle = 1;
#ifdef GRAPH_COLLECT_EXTEND
				if(is_Extend_ReadCollectGraphPkt(pTaskDown->data,pTaskDown->task_len) == FALSE)
#endif
					pTaskDown->task_len -= 1; //转发标识为1时，数据第一个字节表示业务代码，不添加进载波报文中
			}
	#endif
#endif
//			node_sn = PLC_get_sn_from_addr(mac_addr);
//			if (node_sn >= CCTT_METER_NUMBER)
//			{
//				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_METERNO_INEXISTS;
//				return ERROR;
//			}

//			if (NULL != pDL645_task)
//				PLC_Correct_07_97(node_sn, (PBYTE)pDL645_task, pDL645_task->dataLen + 12);

			u8 rt_err, no_response_flag = 0;

			if((app_code == APP_SHENHUA_APP_CODE_GRAPH)&&(func_code == APP_GRAPH_FUNC_CODE_CONFIG))
			{
				//单播配置采集间隔，cco不需要上报数据给集中器
				no_response_flag = 1;
			}
			else if (app_code == APP_SHENHUA_APP_CODE_TIMING)
			{
				//单播配置精准校时，cco不需要上报数据给集中器
				no_response_flag = 1;
			}

            debug_str(DEBUG_LOG_NET, "AddTask, id:%d, mac:%02x%02x%02x%02x%02x%02x, module:%d, code:%d-%d, len:%d, timeout:%d, is645:%d\r\n", pTaskDown->task_id,
                mac_addr[5], mac_addr[4], mac_addr[3], mac_addr[2], mac_addr[1], mac_addr[0], meter_moudle, app_code, func_code,
                pTaskDown->task_len, pTaskDown->task_timeout, (pDL645_task!=NULL)?1:0);

			if(app_code == APP_SHENHUA_APP_CODE_GRAPH)
			{
				if (func_code == APP_GRAPH_FUNC_CODE_CONFIG)
				{
					debug_str(DEBUG_LOG_NET, "AddTask, APP_GRAPH_FUNC_CODE_CONFIG, period:%d\r\n", pTaskDown->data[2]);
				}
				else if (func_code == APP_GRAPH_FUNC_CODE_READ_21PROTOCOL)
				{
					debug_str(DEBUG_LOG_NET, "AddTask, APP_GRAPH_FUNC_CODE_READ, meter:%d, time:%x-%x-%x %x-%x, num:%d, period:%d, dinum:%d\r\n", 
						pTaskDown->data[2],
						pTaskDown->data[7], pTaskDown->data[6], pTaskDown->data[5], pTaskDown->data[4], pTaskDown->data[3],
						pTaskDown->data[8], pTaskDown->data[9], pTaskDown->data[10]);

#ifdef DEBUG_TASK_DATA
						u8 dinum = pTaskDown->data[10];
						for (int index=0; index<dinum; index++)
						{
							debug_str(DEBUG_LOG_NET, "DI:0x%08x\r\n", *(u32*)&pTaskDown->data[11+index*4]);
						}
#endif
				}
			}
			if (OK != PLC_add_read_meter_task(pLmpAppInfo->end_id, meter_moudle, no_response_flag, mac_addr, task_type, pTaskDown, &rt_err))
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = rt_err;
				return (!OK);
			}

			pLmpAppInfo->responsePoint += 2;
			
			return OK;
		}
	case 0xE8020202:    //删除任务
		{
			USHORT waitTime = 1;
			//       OS_ERR err;

			USHORT task_id = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | pLmpAppInfo->curProcUnit->data_unit[0];

			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &waitTime, 2);
			pLmpAppInfo->responsePoint += 2;

			if(PLC_delete_read_task_gd(EM_TASK_TYPE_READ_TASK, task_id)!=OK
			   &&PLC_delete_read_task_gd(EM_TASK_TYPE_MULTI_READ_TASK, task_id)!=OK)
			{
//				pLmpAppInfo->responsePoint -= 2;
//				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_TASK_NOT_ID;
//				return (!OK);
			}

			return OK;
		}
	case 0xE8000203:    //查询未完成任务数
		{
			u16 taskCount = (u16)g_pOsRes->read_meter_task_q.MsgQ.NbrEntries;
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &taskCount, 2);
			pLmpAppInfo->responsePoint += 2;

			return OK;
		}
	case 0xE8030204:    //查询未完成任务列表
		{
			short  task_counter = 0;
			USHORT start_id = (pLmpAppInfo->curProcUnit->data_unit[1]<<8) | pLmpAppInfo->curProcUnit->data_unit[0];
			u8 task_count = pLmpAppInfo->curProcUnit->data_unit[2];
			OS_ERR err;
			OS_MSG_SIZE msgSize;

			u16 *taskid=(u16 *)alloc_buffer(__func__,READ_TASK_SIZE*2);
			if (taskid==NULL)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_BUSY_NOW;
				return (!OK);
			}
			OSSchedLock(&err);

			OS_MSG_QTY count = g_pOsRes->read_meter_task_q.MsgQ.NbrEntries;

			while (count--)
			{
				P_READ_TASK_INFO pTaskTemp = OSQPend(&g_pOsRes->read_meter_task_q, 0, OS_OPT_PEND_NON_BLOCKING, &msgSize, NULL, &err);
				if (OS_ERR_NONE != err)
					break;
				taskid[task_counter] = pTaskTemp->task_header.task_id;
				task_counter++;


				OSQPost(&g_pOsRes->read_meter_task_q, pTaskTemp, sizeof(READ_TASK_INFO), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);
				if (OS_ERR_NONE != err)
					free_buffer(__func__, pTaskTemp);
			}
			OSSchedUnlock(&err);
			u16Sort(taskid,task_counter);
			task_counter-=start_id;
			if (task_counter>task_count)
			{
				task_counter=task_count;
			}
			if (task_counter <= 0)
			{
				memset(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint],0,2);
				pLmpAppInfo->responsePoint+=2;
			}
			else
			{
				*(u16*)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]=task_counter;
				pLmpAppInfo->responsePoint+=2;
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint],&taskid[start_id],task_counter*2);
				pLmpAppInfo->responsePoint+=task_counter*2;
			}
			free_buffer(__func__,taskid);
			return OK;
		}
	case 0xE8030205:    //查询未完成任务详细信息
		{
//        USHORT i;
			USHORT task_id = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | pLmpAppInfo->curProcUnit->data_unit[0];
			OS_ERR err;
			OS_MSG_SIZE msgSize;
			u8 getedid=0;
			OSSchedLock(&err);

			OS_MSG_QTY count = g_pOsRes->read_meter_task_q.MsgQ.NbrEntries;

			while (count--)
			{
				P_READ_TASK_INFO pTaskTemp = OSQPend(&g_pOsRes->read_meter_task_q, 0, OS_OPT_PEND_NON_BLOCKING, &msgSize, NULL, &err);
				if (OS_ERR_NONE != err)
					break;

				if (pTaskTemp->task_header.task_id == task_id)
				{
					getedid=1;
					*((u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]) = pTaskTemp->task_header.task_id;
					pLmpAppInfo->responsePoint += 2;
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &pTaskTemp->task_header.task_mode, sizeof(READ_TASK_MODE));
					pLmpAppInfo->responsePoint += sizeof(READ_TASK_MODE);
					if (pTaskTemp->task_type==EM_TASK_TYPE_READ_TASK)
					{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;
						memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &pTaskTemp->mac_addr, MAC_ADDR_LEN);   
						pLmpAppInfo->responsePoint += MAC_ADDR_LEN;
					}
					else
					{
						multi_sta_head *p_sta = pTaskTemp->p_sta;
						if (0xffff==p_sta->add_num)
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1;
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;
							memset(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0xff, MAC_ADDR_LEN);
							pLmpAppInfo->responsePoint += MAC_ADDR_LEN;
						}
						else
						{
							u16 * p_num=(u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
							pLmpAppInfo->responsePoint+=2;
							*p_num=0;
							for (int i = 0; i < sizeof(p_sta->bitmap) / 4; i++)
							{
								if (p_sta->bitmap[i])
								{
									for (int j = 0; j < 32; j++)
									{
										if (p_sta->bitmap[i] & (1 << j))
										{
											u16 sn=(i<<5)|j;
											if (g_MeterRelayInfo[sn].status.used == CCO_USERD)
											{
												(*p_num)++;
												memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_MeterRelayInfo[sn].meter_addr, MAC_ADDR_LEN);
												pLmpAppInfo->responsePoint += MAC_ADDR_LEN;
											}
										}
									}
								}
							}
						}
					}
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pTaskTemp->task_header.task_len;
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &pTaskTemp->task_buffer[0], pTaskTemp->task_header.task_len);
					pLmpAppInfo->responsePoint += pTaskTemp->task_header.task_len;
				}

				OSQPost(&g_pOsRes->read_meter_task_q, pTaskTemp, sizeof(READ_TASK_INFO), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);
				if (OS_ERR_NONE != err)
					free_buffer(__func__,pTaskTemp);
			}

			OSSchedUnlock(&err);
			if (!getedid)
			{
				*((u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]) = 0;
				pLmpAppInfo->responsePoint += 2;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]=0;
				*((u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]) = 0;
				pLmpAppInfo->responsePoint += 2;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]=0;
			}
			return OK;
		}
	case 0xE8000206:    //查询剩余可分配任务数
		{
			u16 task_count_left = (u16)(READ_TASK_SIZE - (u16)g_pOsRes->read_meter_task_q.MsgQ.NbrEntries);
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &task_count_left, 2);
			pLmpAppInfo->responsePoint += 2;

			return OK;
		}
	case 0xE8020207:        //E8 02 02 07：添加多播任务（选配）
		{
			USHORT waitTime = 1;

			P_READ_TASK_HEADER pTaskDown = (P_READ_TASK_HEADER)&pLmpAppInfo->curProcUnit->data_unit[0];

			if (pLmpAppInfo->appLayerLen <= (6 + 6))       //AFN + SEQ + DI + 任务头
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &waitTime, 2);
			u8 rt_err;
			if (OK != PLC_add_read_meter_task(pLmpAppInfo->end_id,0, 0, NULL, EM_TASK_TYPE_MULTI_READ_TASK, pTaskDown, &rt_err))
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = rt_err;
				return (!OK);
			}

			pLmpAppInfo->responsePoint += 2;
			
			return OK;
		}
	case 0xE8020208:    //启动任务
		{
			USHORT waitTime = 1;

			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &waitTime, 2);
			pLmpAppInfo->responsePoint += 2;

			PLC_stage_resume();

			return OK;
		}
	case 0xE8020209:    //暂停任务
		{
			USHORT waitTime = 1;

			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &waitTime, 2);
			pLmpAppInfo->responsePoint += 2;

			PLC_stage_pause();

			return OK;
		}
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DT;
		return ERROR;
	}

//    return OK;
}


BYTE afn_get_meter_identify(USHORT node_sn)
{
	BYTE info = 2;

	if (node_sn >= CCTT_METER_NUMBER)
		return info;

	if (MF_RELAY_FREEZED(node_sn))
		info = 0;

	return info;
}

#ifdef PROTOCOL_NW_2021
//南网21标准E8 03 03 13查询模块资产信息信息元素ID返回长度(包含2字节信息元素+信息元素长度)
const u8 element_len_tbl[17] = 
{
	4,  //0x00，厂商编号，2字节
    4,  //0x01，软件版本信息（模块），2字节
    3,  //0x02，bootloader版本号，1字节
    2,  //0x03, 南网21标准不用支持
    2,  //0x04, 南网21标准不用支持
    4,  //0x05，芯片厂商代码，2字节
    5,  //0x06，固件发布日期（模块），3字节
    2,  //0x07, 南网21标准不用支持
	8,  //0x08，模块出厂MAC地址，6字节
	4,  //0x09，硬件版本信息（模块），2字节
	5,  //0x0A，硬件发布日期（模块），3字节
	4,  //0x0B，软件版本信息（芯片），2字节
	5,  //0x0C，软件发布日期（芯片），3字节
	4,  //0x0D，硬件版本信息（芯片），2字节
	5,  //0x0E，硬件发布日期（芯片），3字节
	4,  //0x0F，应用程序版本号，2字节BCD
#ifndef ASSETSCODE_32BYTES_INFORM
	26  //0x10，通信模块资产编码，24字节ASCII
#else
	34  //0x10，通信模块资产编码，32字节ASCII
#endif
};
#endif

BYTE afn_func_03_gd(P_LMP_APP_INFO pLmpAppInfo)
{
	switch (pLmpAppInfo->curDT)
	{
	case 0xE8000301:    //查询厂商代码和版本信息
		{
			TMN_VER_INFO ver_inf;
			memcpy(&ver_inf, &c_Tmn_ver_info, sizeof(TMN_VER_INFO));

			TransposeByteOrder((UCHAR *)ver_inf.factoryCode, sizeof(ver_inf.factoryCode));
			TransposeByteOrder((UCHAR *)ver_inf.chip_code, sizeof(ver_inf.chip_code));
            TransposeByteOrder((UCHAR *)ver_inf.software_v, sizeof(ver_inf.software_v));
            
            ver_inf.software_v[0] = Hex2BcdChar(ver_inf.software_v[0]);
            ver_inf.software_v[1] = Hex2BcdChar(ver_inf.software_v[1]);
            
			ver_inf.softReleaseDate.day = Hex2BcdChar(ver_inf.softReleaseDate.day);
			ver_inf.softReleaseDate.month = Hex2BcdChar(ver_inf.softReleaseDate.month);
			ver_inf.softReleaseDate.year = Hex2BcdChar(ver_inf.softReleaseDate.year);

			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &ver_inf, sizeof(TMN_VER_INFO)-4);
			pLmpAppInfo->responsePoint += sizeof(TMN_VER_INFO)-4;
			g_PlmStatusPara.restart_read_flag = TRUE;
			if (pLmpAppInfo->appLayerLen==7)//afn(1)+seq(1)+FN(4)+Data(1)
			{
				ReaderPhase=pLmpAppInfo->curProcUnit->data_unit[0];
			}
			return OK;
		}
	case 0xE8000302:    //查询本地通信模块运行模式信息
		{
			ROUTER_IFO_GD routerIfo;

			PLC_Get_Router_Ifo_gd(&routerIfo);
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &routerIfo, sizeof(routerIfo));
			pLmpAppInfo->responsePoint += sizeof(routerIfo);
			g_PlmStatusPara.restart_read_flag = TRUE;
			return OK;
		}
	case 0xE8000303:    //查询主节点地址
		{
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_PlmConfigPara.main_node_addr, 6);
			pLmpAppInfo->responsePoint += 6;
			return OK;
		}
	case 0xE8030304:    //查询通信延时时长
		{
			PBYTE pMeterAddr = &pLmpAppInfo->curProcUnit->data_unit[0];
			UCHAR len = pLmpAppInfo->curProcUnit->data_unit[6];
			USHORT time = 0;

			time = (len * 8) / 100;     //按照100bps算

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, pMeterAddr, 6);
			pLmpAppInfo->responsePoint += 6;

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &time, 2);
			pLmpAppInfo->responsePoint += 2;

			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = len;

			return OK;
		}
	case 0xE8000305:    //查询从节点数量
		{
			PLC_get_relay_info();
			if (g_pRelayStatus->selfreg_cb.state<EM_SELFREG_START
				||g_pRelayStatus->selfreg_cb.state> EM_SELFREG_STOP)
			{

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
						   &g_pRelayStatus->cfged_meter_num, 2);
			}
			else
			{
				u16 meter_num = 0;
				//P_RELAY_INFO p_relay = (P_RELAY_INFO)(METER_RELAY_INFO_ADDR + 0x12000000);
				for (int ii = 0; ii <  CCTT_METER_NUMBER; ii++)
				{

					if (g_MeterRelayInfo[ii].status.used != CCO_USERD)
					{
						continue;
					}
					if (g_PlmConfigPara.close_white_list != 1)//开启白名单;或者关闭白名单但档案不写入flash
					{
						if (!g_MeterRelayInfo[ii].status.cfged)
						{
							continue;
						}
					}
					meter_num++;
				}
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
							   &meter_num, 2);
			}
			pLmpAppInfo->responsePoint += 2;
			g_PlmStatusPara.restart_read_flag = TRUE;
			return OK;
		}
	case 0xE8030306:    //查询从节点信息
		{
			PBYTE real_num;
			USHORT ii;
//        USHORT node_counter = 0;
			USHORT start_sn = *((USHORT *)pLmpAppInfo->curProcUnit->data_unit);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			USHORT *total_num;
			if (0 == start_sn)
				s_meter_sn_from_zero = TRUE;
			if (s_meter_sn_from_zero)
			{
				start_sn++;
			}

			PLC_get_relay_info();


			//P_RELAY_INFO p_relay=(P_RELAY_INFO)(METER_RELAY_INFO_ADDR+0x12000000);

			total_num=(USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			*total_num=0;
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)((ULONG)pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);
			*real_num = 0;

			pLmpAppInfo->responsePoint++;


			for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
			{
				if (g_MeterRelayInfo[ii].status.used != CCO_USERD)
					continue;

				if (g_PlmConfigPara.close_white_list != 1)//开启白名单;或者关闭白名单但档案不写入flash
				{
					if (!g_MeterRelayInfo[ii].status.cfged)
					{
						continue ;
					}
				}
				
				(*total_num)++;
				if (start_sn)
				{
					start_sn--;
					if (start_sn!=0)
					{
						continue;
					}
				}
				if (*real_num == meter_num)
					continue;
				(*real_num)++;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
							   g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;

			}

			g_PlmStatusPara.restart_read_flag = TRUE;
			return OK;
		}
	case 0xE8000307:    //查询从节点主动注册进度
		{
			P_PLC_SELFREG_CB pSelfRegCB = &g_pRelayStatus->selfreg_cb;

			if (pSelfRegCB->state >= EM_SELFREG_START && pSelfRegCB->state <= EM_SELFREG_REPORT_STOP)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1;
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;
			}
			
			return OK;
		}
	case 0xE8030308:    //查询从节点的父节点
		{
			PBYTE pMeterAddr = &pLmpAppInfo->curProcUnit->data_unit[0];
			USHORT meter_sn; //, upper_sn;
			#ifdef NW_TEST
			read_meter_time = 0;
			#endif
			meter_sn = PLC_get_sn_from_addr(pMeterAddr);
			if (meter_sn >= CCTT_METER_NUMBER)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_METERNO_INEXISTS;
				return ERROR;
			}

			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &g_MeterRelayInfo[meter_sn].meter_addr[0], 6);
			pLmpAppInfo->responsePoint += 6;

#if 1
			u8 dst_addr[LONG_ADDR_LEN] = {0};
			PLC_GetAppDstMac(pMeterAddr, dst_addr);

			P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
#else
			P_STA_INFO pSta = PLC_GetStaInfoByMac(pMeterAddr);
#endif
			P_STA_INFO pStaParent = NULL;

			if ((NULL != pSta) && (pSta->pco >= (TEI_STA_MIN - 1)) && (pSta->pco <= TEI_STA_MAX) && pSta->net_state==NET_IN_NET)
			{
				pStaParent = PLC_GetValidStaInfoByTei(pSta->pco);
			}

			if (NULL != pStaParent)
			{
				memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &pStaParent->mac[0], 6);
				pLmpAppInfo->responsePoint += 6;

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 27;
			}
			else
			{
				//memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &g_PlmConfigPara.main_node_addr[0], 6);
				memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0xFF, 6);

				pLmpAppInfo->responsePoint += 6;

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;
			}

			return OK;
		}
	case 0xE8000309:        //查询映射表从节点数量
		{
			USHORT count = 0;
			for (USHORT i = 0; i < CCTT_METER_COLL_NUMBER; i++)
			{
				if (!METER_COLL_IS_SET(i))
					continue;
				count++;
			}

			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &count, 2);
			pLmpAppInfo->responsePoint += 2;

			return OK;
		}
		// case 0xE800030A://读表箱终端告警参数
		//  {
		//      memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (U8 *)&g_PlmConfigPara.ALARM_REF, sizeof(ALARM_REF_C));
		//      pLmpAppInfo->responsePoint +=sizeof(ALARM_REF_C);
		//      return OK;
		//  }
	case 0xE803030A:    //返回查询从节点通信地址映射表
		{
			BYTE node_count = 0, real_num = 0;
			USHORT start_sn = 0;
			USHORT counter = 0;

			USHORT presp = pLmpAppInfo->responsePoint;
			pLmpAppInfo->responsePoint += 3;

			memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
			node_count = pLmpAppInfo->curProcUnit->data_unit[2];

			for (USHORT i = 0; i < CCTT_METER_COLL_NUMBER; i++)
			{
				if (!METER_COLL_IS_SET(i))
					continue;
				if ((counter >= start_sn) && (real_num < node_count))
				{
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_pMeterCollMap[i].coll_addr, 18);
					pLmpAppInfo->responsePoint += 18;
					real_num++;
				}
				counter++;
			}

			memcpy_special(&pLmpAppInfo->msg_buffer[presp], &counter, 2);
			memcpy_special(&pLmpAppInfo->msg_buffer[presp + 2], &real_num, 1);

			return OK;
		}
	case 0xE800030B:    //查询任务建议超时时间
		{
			USHORT timeout = 120;
			USHORT task_counter = (USHORT)g_pOsRes->read_meter_task_q.MsgQ.NbrEntries;

			timeout = 120;
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &timeout, 2);
			pLmpAppInfo->responsePoint += 2;

			timeout += task_counter % 20;
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &timeout, 2);
			pLmpAppInfo->responsePoint += 2;

			timeout += task_counter % 20;
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &timeout, 2);
			pLmpAppInfo->responsePoint += 2;

			timeout += task_counter % 20;
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &timeout, 2);
			pLmpAppInfo->responsePoint += 2;

			return OK;
		}
	case 0xE803030C:    //查询从节点相位信息
		{
			BYTE p = 0;
			BYTE node_count = pLmpAppInfo->curProcUnit->data_unit[p++];
			USHORT node_sn = 0;
			USHORT info = 0;
			BYTE node_count_resp = 0;
			USHORT presp = pLmpAppInfo->responsePoint;
			pLmpAppInfo->responsePoint += 1;

			for (BYTE i = 0; i < node_count; i++)
			{
				PBYTE pMeterAddr = &pLmpAppInfo->curProcUnit->data_unit[p];
				p += 6;
				node_sn = PLC_get_sn_from_addr(pMeterAddr);
				if (node_sn >= CCTT_METER_NUMBER)
					continue;

				info = afn_get_meter_phase_info(node_sn);

				memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pMeterAddr, 6);
				pLmpAppInfo->responsePoint += 6;

#ifdef PROTOCOL_NW_2021_GUANGDONG
                u8 dst_addr[LONG_ADDR_LEN] = {0};
                PLC_GetAppDstMac(pMeterAddr, dst_addr);
				
				P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
				if (NULL != pSta)
				{
				    if ((pSta->dev_type == II_COLLECTOR) || (pSta->dev_type == I_COLLECTOR))
                    {
                        if (memcmp(pMeterAddr, dst_addr, 6) == 0) //采集器
                        {

                            ((meter_phase *)&info)->meter_type = 0; //采集器
                                
                        }
                        else //采集器下挂电表
                        {
                            ((meter_phase *)&info)->meter_type = 3; //采集器下挂电表
                        }
                    }
                }
#endif
				memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &info, 2);
				pLmpAppInfo->responsePoint += 2;

				node_count_resp++;
			}

			memcpy_special(&pLmpAppInfo->msg_buffer[presp], &node_count_resp, 1);

			return OK;
		}
	case 0xE803030D:        //批量查询从节点相位信息
		{
			BYTE node_count = 0, real_num = 0;
			USHORT start_sn = 0, info;
			USHORT presp = pLmpAppInfo->responsePoint;
			USHORT ii = 0;

			memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
			node_count = pLmpAppInfo->curProcUnit->data_unit[2];

			PLC_get_relay_info();
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &g_pRelayStatus->cfged_meter_num, 2);
			pLmpAppInfo->responsePoint += 2;

			pLmpAppInfo->responsePoint++;

			//寻找真正的起始位置
			if (PLC_GetRealNodeSnCfg(TRUE, start_sn, &start_sn))
			{
				for (ii = start_sn; ii <  CCTT_METER_NUMBER; ii++)
				{
					if (!METER_IS_CFG(ii))
						continue;

					real_num++;

					info = afn_get_meter_phase_info(ii);

					memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, 6);
					pLmpAppInfo->responsePoint += 6;

#ifdef PROTOCOL_NW_2017_GUANGZHOU       
					/*struct {
						u8 phase_info_a:1;
						u8 phase_info_b:1;
						u8 phase_info_c:1;
						u8 phase_features:2;
						u8 phase_sq:2;
						u8 :1;
						u8 protocol;
					};*/

					/*2bit从节点状态（默认0，线路异常（零火反接，三相逆相序）置1）代替3bit相序类型*/
					if (((meter_phase *)&info)->phase_sq != 0)
					{
						((meter_phase *)&info)->phase_sq = 1;
					}
					
					/*没有模块类型，全部赋值0*/
					((meter_phase *)&info)->meter_type = 0;
#elif defined(PROTOCOL_NW_2021_GUANGDONG)
                    u8 dst_addr[LONG_ADDR_LEN] = {0};
                    PLC_GetAppDstMac(g_MeterRelayInfo[ii].meter_addr, dst_addr);
                    
                    P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
                    if (NULL != pSta)
                    {
                        if ((pSta->dev_type == II_COLLECTOR) || (pSta->dev_type == I_COLLECTOR))
                        {
                            if (memcmp(g_MeterRelayInfo[ii].meter_addr, dst_addr, 6) == 0) //采集器
                            {
                                if (pSta->mac_join) //采集器以自身地址入网
                                {
                                    ((meter_phase *)&info)->meter_type = 0; //采集器
                                    
                                }
                                else //采集器以电表地址入网
                                {
                                    ((meter_phase *)&info)->meter_type = 3; //采集器下挂电表
                                }                              
                            }
                            else //采集器下挂电表
                            {
                                ((meter_phase *)&info)->meter_type = 3; //采集器下挂电表
                            }
                        }
                    }
#endif

					memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &info, 2);
					pLmpAppInfo->responsePoint += 2;

					if (real_num == node_count)
						break;
				}
			}

			memcpy_special(pLmpAppInfo->msg_buffer + presp + 2, &real_num, 1);

			return OK;
		}
	case 0xE803030E:    //查询表档案的台区识别结果
		{
			BYTE node_count = 0, real_num = 0;
			USHORT start_sn = 0;
			BYTE info;
			USHORT presp = pLmpAppInfo->responsePoint;
			USHORT ii = 0;
			USHORT *total;
			memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
			node_count = pLmpAppInfo->curProcUnit->data_unit[2];

			total=(USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			*total=0;
			pLmpAppInfo->responsePoint += 2;

			pLmpAppInfo->responsePoint++;

			//寻找真正的起始位置
			//if (PLC_GetRealNodeSnCfg(TRUE, start_sn, &start_sn))
			{
				for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
				{
					if (!METER_IS_CFG(ii))
						continue;
					
					++(*total);
					if (start_sn)
					{
						--start_sn;
						continue;
					}
					if (real_num != node_count)
					{
						real_num++;
#ifdef FORCE_ZONE_RESULT
						info = 0;
#else
                        /*************支持采集器台区识别结果查询****************/
                        u16 index = ii;

        				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(g_MeterRelayInfo[ii].coll_sn);
        				if (pSta)
        				{
        				    if ((pSta->dev_type == II_COLLECTOR) || (pSta->dev_type == I_COLLECTOR))
                            {
                                u16 sn = PLC_get_sn_from_addr(pSta->mac);
                                if (sn != CCTT_METER_NUMBER)
                                {
                                    index = sn; //找到采集器入网地址对应档案的索引
                                }
                            }
        				}
                        /*******************************************************/
                        
						if (g_MeterRelayInfo[index].cco_num == 0xff) //未知相线
						{
							info = 3;
						}
						else if (g_MeterRelayInfo[index].cco_num == MAX_CCO_NUM) //本台区节点
						{
							info = 0;
						}
						else
						{
							info = 1;
						}
#endif
						memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, 6);
						pLmpAppInfo->responsePoint += 6;

						memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &info, 1);
						pLmpAppInfo->responsePoint += 1;
					}
				}
			}

			memcpy_special(pLmpAppInfo->msg_buffer + presp + 2, &real_num, 1);

			return OK;
		}
	case 0xE803030F:        //返回非本台区节点的台区识别结果
		{
//        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;
//        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;
//        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;
//        return OK;

			BYTE node_count = 0, real_num = 0;
			USHORT start_sn = 0, info;
			USHORT presp = pLmpAppInfo->responsePoint;
			USHORT ii = 0;
			USHORT *total;
			memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
			node_count = pLmpAppInfo->curProcUnit->data_unit[2];

			total=(USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			*total=0;
			pLmpAppInfo->responsePoint += 2;

			pLmpAppInfo->responsePoint++;

			//寻找真正的起始位置
			//if (PLC_GetRealNodeSnCfg(TRUE, start_sn, &start_sn))
			{
				for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
				{
					if (!METER_IS_CFG(ii))
						continue;

                    /*************支持采集器台区识别结果查询****************/
                    u16 index = ii;

    				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(g_MeterRelayInfo[ii].coll_sn);
    				if (pSta)
    				{
    				    if ((pSta->dev_type == II_COLLECTOR) || (pSta->dev_type == I_COLLECTOR))
                        {
                            u16 sn = PLC_get_sn_from_addr(pSta->mac);
                            if (sn != CCTT_METER_NUMBER)
                            {
                                index = sn; //找到采集器入网地址对应档案的索引
                            }
                        }
    				}
                    /*******************************************************/
                    
					if (g_MeterRelayInfo[index].cco_num == MAX_CCO_NUM||g_MeterRelayInfo[index].cco_num == 0xff)
						continue;

					++(*total);
					if (start_sn)
					{
						--start_sn;
						continue;
					}
					if (real_num != node_count)
					{
						real_num++;
#if 0
						if (g_MeterRelayInfo[ii].cco_num==0xff) //未知相线
						{
							info=3;
						}
						else if(g_MeterRelayInfo[ii].cco_num==MAX_CCO_NUM) //本台区节点
						{
							info=0;
						}
						else
						{
							info=1；
						}
#else
						info = 0;
#endif

						memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, 6);
						pLmpAppInfo->responsePoint += 6;

						getAreaCCO(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[index].cco_num);
						pLmpAppInfo->responsePoint += 6;

						memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, (u8 *)&info, 2);
						pLmpAppInfo->responsePoint += 2;

					}
				}
			}

			memcpy_special(pLmpAppInfo->msg_buffer + presp + 2, &real_num, 1);

			return OK;
		}
	case 0xE8030310:  //查询台区识别状态
		{
			if(areaTimerId!=INVAILD_TMR_ID)
			{
				debug_str(DEBUG_LOG_NET, "E8030310, area time left:%d\r\n", HPLC_LeftTmr(areaTimerId)/60000);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]=1;
				*(u16*)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = HPLC_LeftTmr(areaTimerId)/60000; //分钟
				pLmpAppInfo->responsePoint+=2;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]=0;

			}
			else
			{
				debug_str(DEBUG_LOG_NET, "E8030310, area time left, invalid\r\n");
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]=0;
				*(u16*)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0xffff;
				pLmpAppInfo->responsePoint+=2;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]=0;
			}
			return OK;
		}
#ifdef PROTOCOL_NW_2017_GUANGZHOU 
        case 0xE8030311:        //查询网络拓扑信息
        {
			USHORT tei;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT *total_num;

            if (pLmpAppInfo->appLayerLen != 9) //AFN(1) + SEQ(1) + FN(4) + 节点起始序号(2) + 节点数量(1)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}

            if (start_sn == 0) //节点起始序号从1开始
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
            
			total_num = (USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			*total_num = 0;
			pLmpAppInfo->responsePoint += 2;

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &start_sn, 2);
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;
			
			for (USHORT ii=0; ii<CCTT_METER_NUMBER; ii++)
			{
				if ((g_MeterRelayInfo[ii].status.used != CCO_USERD)||
					(g_MeterRelayInfo[ii].status.cfged == 0)||
					(g_MeterRelayInfo[ii].coll_sn >= TEI_CNT || g_MeterRelayInfo[ii].coll_sn < TEI_STA_MIN))
					continue;

				tei = g_MeterRelayInfo[ii].coll_sn;
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (NULL == pSta)
					continue;

				if (NET_OUT_NET == pSta->net_state)
					continue;
				
				if (!PLC_IsInNet(tei))
				{
					continue;
				}

				//华为离线也会返回，离网不返回
#if 1
				if (NET_IN_NET != pSta->net_state)
				{
					continue;
				}
#endif
				
				(*total_num)++;

				if (start_sn > 1) //sta序号从1开始
				{
					start_sn--;
					continue;
				}
				
				if (*real_num == meter_num)
					continue;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

				P_STA_TOPOLOGY_INFO pTop = (P_STA_TOPOLOGY_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
				pTop->tei = tei;
				pTop->pco = pSta->pco;
				pTop->level = PLC_GetStaLevel(tei);
				pTop->roles = (pSta->child == 0) ? ROLES_STA : ROLES_PCO;
				pLmpAppInfo->responsePoint += sizeof(STA_TOPOLOGY_INFO);
				
				(*real_num)++;
			}
			break;
		}
#elif defined(PROTOCOL_NW_2020) //查询多余节点的台区识别结果，已从2022年深化应用文档中删除
	case 0xE8030311:        //查询多余节点的台区识别结果
		{
			extern RemMeter_s OutListMeter;
			BYTE node_count = 0, real_num = 0;
			USHORT start_sn = 0;
			BYTE info;
			USHORT presp = pLmpAppInfo->responsePoint;
			USHORT ii = 0;
			memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
			node_count = pLmpAppInfo->curProcUnit->data_unit[2];

			*(USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = OutListMeter.cnt;
			pLmpAppInfo->responsePoint += 2;

			if (start_sn+1 > OutListMeter.cnt)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;
				return OK;
			}
			else
			{
				pLmpAppInfo->responsePoint++;
			}
			
			for (ii=start_sn; ii<OutListMeter.cnt; ii++)
			{
				if (real_num != node_count)
				{
					real_num++;

					if (OutListMeter.cco_num[ii] == 0xff) //未知
					{
						info = 3;
					}
					else if(OutListMeter.cco_num[ii] == MAX_CCO_NUM) //本台区节点
					{
						info = 0;
					}
					else
					{
						info = 1;
					}

					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], OutListMeter.mac[ii], 6);
					pLmpAppInfo->responsePoint += 6;

					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = info; //台区识别结果

					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0; //保留

					getAreaCCO(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], OutListMeter.cco_num[ii]);
					pLmpAppInfo->responsePoint += 6;
				}
			}

			memcpy_special(pLmpAppInfo->msg_buffer + presp + 2, &real_num, 1);

			return OK;
		}
#endif	

#ifdef PROTOCOL_NW_2017_GUANGZHOU 
    case 0xE8030312: //批量查询网络相位和台区信息
	    {
			BYTE node_count = 0;
			USHORT start_sn = 0;
			BYTE area_info;
            USHORT phase_info;
			USHORT ii = 0;
            UCHAR *real_num;
			USHORT *total_num;

            if (pLmpAppInfo->appLayerLen != 9) //AFN(1) + SEQ(1) + FN(4) + 节点起始序号(2) + 节点数量(1)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}
            
            memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
            node_count = pLmpAppInfo->curProcUnit->data_unit[2];
            
            total_num = (USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			*total_num = 0;
			pLmpAppInfo->responsePoint += 2;

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &start_sn, 2);
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;

            if (start_sn == 1) //CCO
			{
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_PlmConfigPara.main_node_addr, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

                //台区信息
                pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x01; //本台区站点

                //相位信息
                phase_info = (ZC_Time[0] ? 1 : 0) |
				             (ZC_Time[1] ? 1 : 0) << 1 |
				             (ZC_Time[2] ? 1 : 0) << 2;
                memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &phase_info, 2);
				pLmpAppInfo->responsePoint += 2;		
                
				(*real_num)++;	
			}

			(*total_num)++; //CCO
            
			for (ii=0; ii<CCTT_METER_NUMBER; ii++)
			{
				if (!METER_IS_CFG(ii))
					continue;
				
				(*total_num)++;

                if (start_sn > 2) //sta序号从2开始
				{
					start_sn--;
					continue;
				}

                if (*real_num == node_count)
					continue;
                
				(*real_num)++;

                memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;

                //台区信息
                /*************支持采集器台区识别结果查询****************/
                u16 index = ii;

				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(g_MeterRelayInfo[ii].coll_sn);
				if (pSta)
				{
				    if ((pSta->dev_type == II_COLLECTOR) || (pSta->dev_type == I_COLLECTOR))
                    {
                        u16 sn = PLC_get_sn_from_addr(pSta->mac);
                        if (sn != CCTT_METER_NUMBER)
                        {
                            index = sn; //找到采集器入网地址对应档案的索引
                        }
                    }
				}
                /*******************************************************/
                
				if (g_MeterRelayInfo[index].cco_num == 0xff) //未知相线
				{
					area_info = 0; //未知
				}
				else if (g_MeterRelayInfo[index].cco_num == MAX_CCO_NUM) //本台区节点
				{
					area_info = 1; //本台区
				}
				else
				{
					area_info = 2; //非本台区
				}

                memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &area_info, 1);
				pLmpAppInfo->responsePoint += 1;

                //相位信息
                phase_info = afn_get_meter_phase_info(ii);

                /*struct {
                    u8 phase_info_a:1;
                    u8 phase_info_b:1;
                    u8 phase_info_c:1;
                    u8 phase_features:2;
                    u8 :3;
                    u8 res;
                };*/

                /*2bit从节点状态（默认0，线路异常（零火反接，三相逆相序）置1）代替2bit相线特征*/
                if (((meter_phase *)&phase_info)->phase_sq != 0)
                {
                    ((meter_phase *)&phase_info)->phase_features = 1;
                }
                else
                {
                    ((meter_phase *)&phase_info)->phase_features = 0;
                }

                /*没有相序类型，全部赋值0*/
                ((meter_phase *)&phase_info)->phase_sq = 0;
                
                /*没有规约类型，全部赋值0*/
                ((meter_phase *)&phase_info)->protocol = 0;
                
                /*没有模块类型，全部赋值0*/
                ((meter_phase *)&phase_info)->meter_type = 0;

                memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &phase_info, 2);
				pLmpAppInfo->responsePoint += 2;				
			}

			return OK;
		}
#else
	case 0xE8030312: //批量查询从节点厂商代码和版本信息
	{
		BYTE node_count = 0, real_num = 0;
		USHORT start_sn = 0;
		USHORT presp = pLmpAppInfo->responsePoint;
		USHORT ii = 0;
		P_STA_INFO pSta;
	   
		memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
		node_count = pLmpAppInfo->curProcUnit->data_unit[2];

		PLC_get_relay_info();
		u16 total_num = g_pRelayStatus->cfged_meter_num+1; //CCO和从节点总数量
		memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &total_num, 2);
		pLmpAppInfo->responsePoint += 2;

		pLmpAppInfo->responsePoint++;

		if (start_sn == 0) //CCO
		{   
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_PlmConfigPara.main_node_addr, 6);
			pLmpAppInfo->responsePoint += 6;
		 
			//厂商代码
			memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)&c_Tmn_ver_info.factoryCode, 2);
			pLmpAppInfo->responsePoint += 2;
	  
			//芯片代码
			memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)&c_Tmn_ver_info.chip_code, 2);
			pLmpAppInfo->responsePoint += 2;

			//版本时间
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.day);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.month);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.year);

			//版本号
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.software_v[1]);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.software_v[0]);

			real_num++;
		}

	#if 1
		//寻找真正的起始位置
		//if (PLC_GetRealNodeSnCfg(TRUE, start_sn, &start_sn))
		{
			for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
			{
				if (real_num == node_count)
					break;
		 
				if (!METER_IS_CFG(ii))
					continue;

				if (start_sn > 1) //从节点序号从1开始
				{
					start_sn--;
					continue;
				}
		 
				real_num++;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;
		 
				u8 dst_addr[LONG_ADDR_LEN] = {0};
                PLC_GetAppDstMac(g_MeterRelayInfo[ii].meter_addr, dst_addr);

                pSta = PLC_GetStaInfoByMac(dst_addr);

				if (pSta != NULL)
				{     
					//厂商代码
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_factory_code, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2); 
					pLmpAppInfo->responsePoint += 2;

					//芯片代码
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_chip_code, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2); 
					pLmpAppInfo->responsePoint += 2;

					if((pSta-g_pStaInfo) == TEI_CCO)
					{
						//版本时间
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.day);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.month);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.year);

						//版本号
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->software_v[1]);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->software_v[0]);
					}
					else
					{
						//版本时间
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Day);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Month);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Year);

						//版本号
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->software_v[1];
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->software_v[0];
						memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->software_v, 2);
						TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2);
					}
				}
				else
				{
					//厂商代码和芯片代码是ASCII，赋值'0'
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = '0';
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = '0';
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = '0';
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = '0';

					//版本信息BCD码，赋值0
					memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, 5);
					pLmpAppInfo->responsePoint += 5;
				}
			}
		}
#else
			for (ii = start_sn+1; ii <  CCTT_METER_NUMBER; ii++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(ii);
				if (pSta != NULL)
				{
					real_num++;

					memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, pSta->mac, 6);
					pLmpAppInfo->responsePoint += 6;
				
					//厂商代码
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_factory_code, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2);	
					pLmpAppInfo->responsePoint += 2;

					//芯片代码
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_chip_code, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2);	
					pLmpAppInfo->responsePoint += 2;

					if(ii == TEI_CCO)
					{
						//版本时间
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.day);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.month);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.year);

						//版本号
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->software_v[1]);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->software_v[0]);
					}
					else
					{
						//版本时间
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Day);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Month);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Year);

						//版本号
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->software_v[1];
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->software_v[0];
						memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->software_v, 2);
						TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2);
					}
				}
				
				if (real_num == node_count)
					break;
			}
#endif

			memcpy_special(pLmpAppInfo->msg_buffer + presp + 2, &real_num, 1);

			return OK;
		}
#endif    

    case 0xE8030381: //云南电网查询节点版本信息（格式同E8030312)
	{
		BYTE node_count = 0, real_num = 0;
		USHORT start_sn = 0;
		USHORT presp = pLmpAppInfo->responsePoint;
		USHORT ii = 0;
		P_STA_INFO pSta;
	   
		memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
		node_count = pLmpAppInfo->curProcUnit->data_unit[2];

		PLC_get_relay_info();
		u16 total_num = g_pRelayStatus->cfged_meter_num+1; //CCO和从节点总数量
		memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &total_num, 2);
		pLmpAppInfo->responsePoint += 2;

		pLmpAppInfo->responsePoint++;

		if (start_sn == 0) //CCO
		{   
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_PlmConfigPara.main_node_addr, 6);
			pLmpAppInfo->responsePoint += 6;
		 
			//厂商代码
			memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)&c_Tmn_ver_info.factoryCode, 2);
			pLmpAppInfo->responsePoint += 2;
	  
			//芯片代码
			memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)&c_Tmn_ver_info.chip_code, 2);
			pLmpAppInfo->responsePoint += 2;

			//版本时间
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.day);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.month);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.year);

			//版本号
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.software_v[1]);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.software_v[0]);

			real_num++;
		}

		//寻找真正的起始位置
		//if (PLC_GetRealNodeSnCfg(TRUE, start_sn, &start_sn))
		{
			for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
			{
				if (real_num == node_count)
					break;
		 
				if (!METER_IS_CFG(ii))
					continue;

				if (start_sn > 1) //从节点序号从1开始
				{
					start_sn--;
					continue;
				}
		 
				real_num++;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;
		 
				u8 dst_addr[LONG_ADDR_LEN] = {0};
                PLC_GetAppDstMac(g_MeterRelayInfo[ii].meter_addr, dst_addr);

                pSta = PLC_GetStaInfoByMac(dst_addr);

				if (pSta != NULL)
				{     
					//厂商代码
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_factory_code, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2); 
					pLmpAppInfo->responsePoint += 2;

					//芯片代码
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_chip_code, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2); 
					pLmpAppInfo->responsePoint += 2;

					if((pSta-g_pStaInfo) == TEI_CCO)
					{
						//版本时间
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.day);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.month);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.year);

						//版本号
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->software_v[1]);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->software_v[0]);
					}
					else
					{
						//版本时间
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Day);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Month);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Year);

						//版本号
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->software_v[1];
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->software_v[0];
						memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->software_v, 2);
						TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2);
					}
				}
				else
				{
					//厂商代码和芯片代码是ASCII，赋值'0'
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = '0';
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = '0';
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = '0';
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = '0';

					//版本信息BCD码，赋值0
					memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, 5);
					pLmpAppInfo->responsePoint += 5;
				}
			}
		}

		memcpy_special(pLmpAppInfo->msg_buffer + presp + 2, &real_num, 1);

		return OK;
	}
	case 0xE8030313:// 查询模块资产信息
		{
#ifdef PROTOCOL_NW_2021 //南网21标准，资产信息小端传输
			u8 mac_addr[MAC_ADDR_LEN] = {0};
			u8 element_num = 0, element_id = 0, element_total_len = 0, element_len = 0;
			u32 element_mask = 0;
			u8 element_index = 0;
			u8 element_array[0x10+1] = {0};

			element_num = pLmpAppInfo->curProcUnit->data_unit[6];

			if (element_num > (0x10+1)) //信息元素数量，南网21标准信息元素ID目前最大定义到0x10（从0开始）
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}

			if (pLmpAppInfo->appLayerLen != (6 + 6 + 1 + element_num)) //AFN + SEQ + DI + 节点地址(6)+信息元素数量(1)+信息元素
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}
			
			memcpy(mac_addr, pLmpAppInfo->curProcUnit->data_unit, MAC_ADDR_LEN);

			if (memcmp(g_PlmConfigPara.main_node_addr, mac_addr, 6) == 0)			
			{
				//CCO资产信息
				memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], mac_addr, 6);
				pLmpAppInfo->responsePoint += 6;

				for (int i=0; i<element_num; i++)
				{
					element_id = pLmpAppInfo->curProcUnit->data_unit[7+i];
					if (element_id > 0x10) //南网21标准信息元素ID目前最大定义到0x10
					{
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
						return ERROR;
					}
					
					element_len = element_len_tbl[element_id];
					memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, element_len);

					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = element_id;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = element_len - 2;

					if (element_id == 0) //厂商编号，2字节ASCII
					{
						memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_Tmn_ver_info.factoryCode, 2);					
					}
					else if(element_id == 1) //软件版本信息（模块），2字节BCD
					{
                        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = Hex2BcdChar(c_Tmn_ver_info.software_v[1]);
			            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = Hex2BcdChar(c_Tmn_ver_info.software_v[0]);
					}
					else if(element_id == 2) //bootloader版本号，1字节BIN
					{
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 1;
					}
					else if(element_id == 5) //芯片厂商代码，2字节ASCII
					{
						memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_Tmn_ver_info.chip_code, 2);
					}
					else if(element_id == 6) //固件发布日期（模块），3字节BIN
					{
#if 1 //南网21标准规定资产信息中日期BIN格式
                        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = c_Tmn_ver_info.softReleaseDate.day;
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = c_Tmn_ver_info.softReleaseDate.month;
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = c_Tmn_ver_info.softReleaseDate.year;
#else
                        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.day);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.month);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.year);
#endif                        
					}
					else if(element_id == 8) //模块出厂MAC地址，6字节BIN
					{
						memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_cco_id.chip_id, 6); //小端存储
					}
					else if(element_id == 9) //硬件版本信息（模块），2字节BCD
					{
						memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_ext_ver_info.ModuleHardVer, 2);
					}
					else if(element_id == 0xA) //硬件发布日期（模块），3字节BIN
					{
#if 1 //南网21标准规定资产信息中日期BIN格式
                        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = c_ext_ver_info.ModuleHardVerDate.day;
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = c_ext_ver_info.ModuleHardVerDate.month;
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = c_ext_ver_info.ModuleHardVerDate.year;
#else					
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = Hex2BcdChar(c_ext_ver_info_default.ModuleHardVerDate.day);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = Hex2BcdChar(c_ext_ver_info_default.ModuleHardVerDate.month);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = Hex2BcdChar(c_ext_ver_info_default.ModuleHardVerDate.year);
#endif
                    }
					else if(element_id == 0xB) //软件版本信息（芯片），2字节BCD
					{
						memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_ext_ver_info.ChipSoftVer, 2);
					}
					else if(element_id == 0xC) //软件发布日期（芯片），3字节BIN
					{
#if 1 //南网21标准规定资产信息中日期BIN格式
                        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = c_ext_ver_info.ChipSofVerDate.day;
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = c_ext_ver_info.ChipSofVerDate.month;
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = c_ext_ver_info.ChipSofVerDate.year;
#else					
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = Hex2BcdChar(c_ext_ver_info_default.ChipSofVerDate.day);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = Hex2BcdChar(c_ext_ver_info_default.ChipSofVerDate.month);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = Hex2BcdChar(c_ext_ver_info_default.ChipSofVerDate.year);
#endif
                    }
					else if(element_id == 0xD) //硬件版本信息（芯片），2字节BCD
					{
						memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_ext_ver_info.ChipHardVer, 2);
					}
					else if(element_id == 0xE) //硬件发布日期（芯片），3字节BIN
					{
#if 1 //南网21标准规定资产信息中日期BIN格式
                        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = c_ext_ver_info.ChipHardVerDate.day;
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = c_ext_ver_info.ChipHardVerDate.month;
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = c_ext_ver_info.ChipHardVerDate.year;
#else					
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = Hex2BcdChar(c_ext_ver_info_default.ChipHardVerDate.day);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = Hex2BcdChar(c_ext_ver_info_default.ChipHardVerDate.month);
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = Hex2BcdChar(c_ext_ver_info_default.ChipHardVerDate.year);
#endif
                    }
					else if(element_id == 0xF) //应用程序版本号，2字节BCD
					{
						memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_ext_ver_info.AppVer, 2);
					}
#ifndef ASSETSCODE_32BYTES_INFORM
					else if(element_id == 0x10) //通信模块资产编码，24字节ASCII
					{
						memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)g_cco_id.module_id, 24);
					}

					pLmpAppInfo->responsePoint += (element_len - 2);
#else
					else if(element_id == 0x10) //通信模块资产编码，24字节ASCII+填充8字节00H
					{
						u8 zero_complement[8]={0};
						memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], zero_complement, 8);
						pLmpAppInfo->responsePoint +=8;
						memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)g_cco_id.module_id, 24);
					}
					if (element_id == 0x10)
					{
						pLmpAppInfo->responsePoint += (element_len-8 - 2);//资产编码补全时已经加8了，这里只需要加element_len-8
					}
					else
					{
						pLmpAppInfo->responsePoint += (element_len - 2);
					}			
#endif
				}
				
				return OK;
			}

			USHORT meter_sn; 
				
			meter_sn = PLC_get_sn_from_addr(mac_addr);
			if (meter_sn >= CCTT_METER_NUMBER)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_METERNO_INEXISTS;
				return ERROR;
			}
			
			u8 dst_addr[LONG_ADDR_LEN] = {0};
			PLC_GetAppDstMac(mac_addr, dst_addr);

			P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);		
			if (NULL == pSta)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
				return ERROR;
			}

			if (NET_IN_NET != pSta->net_state)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
				return ERROR;
			}

			for (int i=0; i<element_num; i++)
			{
				element_id = pLmpAppInfo->curProcUnit->data_unit[7+i];
				if (element_id > 0x10) //南网21标准信息元素ID目前最大定义到0x10
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
					return ERROR;
				}
				
				element_mask |= 1 << element_id;
				element_total_len += element_len_tbl[element_id];
				element_array[element_index] = element_id;
				element_index++;
			}

			debug_str(DEBUG_LOG_NET, "E8030313, STA: %02x-%02x-%02x-%02x-%02x-%02x, getd:%d, element mask: 0x%x\r\n", 
			          mac_addr[5], mac_addr[4], mac_addr[3], mac_addr[2], mac_addr[1], mac_addr[0], pSta->geted_id, element_mask);

			if (pSta->geted_id && !(element_mask & (~0x1ff67))) //已经周期性获取到资产信息，并且本地接口查询信息的元素在周期性获取的范围之内
			{
				memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], mac_addr, 6);
				pLmpAppInfo->responsePoint += 6;

#if 1 //新算法信息元素ID不重新排序
				for (int i=0; i<element_num; i++)
				{
					if (element_mask & (1 << element_array[i]))
					{
						element_len = element_len_tbl[element_array[i]];
						memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, element_len);

						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = element_array[i];
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = element_len - 2;

						if (element_array[i] == 0) //厂商编号，2字节ASCII
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->FactoryCode, 2);					
						}
						else if(element_array[i] == 1) //软件版本信息（模块），2字节BCD
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ModuleSoftVer, 2);
						}
						else if(element_array[i] == 2) //bootloader版本号，1字节BIN
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pSta->BootVer;
						}
						else if(element_array[i] == 5) //芯片厂商代码，2字节ASCII
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ChipCode, 2);			
						}
						else if(element_array[i] == 6) //固件发布日期（模块），3字节BIN
						{
							//南网21标准规定资产信息中日期BIN格式
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pSta->ModuleSoftVerDate.Day;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = pSta->ModuleSoftVerDate.Month;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = pSta->ModuleSoftVerDate.Year;                           
						}
						else if(element_array[i] == 8) //模块出厂MAC地址，6字节BIN
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->module_id, 6);
						}
						else if(element_array[i] == 9) //硬件版本信息（模块），2字节BCD
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ModuleHardVer, 2);
						}
						else if(element_array[i] == 0xA) //硬件发布日期（模块），3字节BIN
						{
							//南网21标准规定资产信息中日期BIN格式
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pSta->ModuleHardVerDate.Day;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = pSta->ModuleHardVerDate.Month;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = pSta->ModuleHardVerDate.Year;    
	                    }
						else if(element_array[i] == 0xB) //软件版本信息（芯片），2字节BCD
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ChipSoftVer, 2);
						}
						else if(element_array[i] == 0xC) //软件发布日期（芯片），3字节BIN
						{
							//南网21标准规定资产信息中日期BIN格式
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pSta->ChipSoftVerDate.Day;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = pSta->ChipSoftVerDate.Month;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = pSta->ChipSoftVerDate.Year;    
	                    }
						else if(element_array[i] == 0xD) //硬件版本信息（芯片），2字节BCD
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ChipHardVer, 2);
						}
						else if(element_array[i] == 0xE) //硬件发布日期（芯片），3字节BIN
						{
							//南网21标准规定资产信息中日期BIN格式
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pSta->ChipHardVerDate.Day;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = pSta->ChipHardVerDate.Month;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = pSta->ChipHardVerDate.Year;    
	                    }
						else if(element_array[i] == 0xF) //应用程序版本号，2字节BCD
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->AppVer, 2);
						}
						else if(element_array[i] == 0x10) //通信模块资资产编码，24字节ASCII
						{
#ifndef ASSETSCODE_32BYTES_INFORM
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->assetsCode, 24);
#else
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->assetsCode, 32);
#endif
						}

						pLmpAppInfo->responsePoint += (element_len - 2);
					}
				}
#else //老算法按元素ID大小重新排序
				for (int i=0; i<=16; i++)
				{
					if (element_mask & (1 << i))
					{
						element_len = element_len_tbl[i];
						memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, element_len);

						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = i;
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = element_len - 2;

						if (i == 0) //厂商编号
						{
							memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_factory_code, 2);					
						}
						else if(i == 1) //软件版本信息（模块）
						{
							memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->software_v, 2);
						}
						else if(i == 5) //芯片厂商代码
						{
							memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_chip_code, 2);
						}
						else if(i == 6) //固件发布日期（模块）
						{
#if 1 //南网21标准规定资产信息中日期BIN格式
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pSta->year_mon_data.Day;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = pSta->year_mon_data.Month;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = pSta->year_mon_data.Year;
#else

                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = Hex2BcdChar(pSta->year_mon_data.Day);	
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = Hex2BcdChar(pSta->year_mon_data.Month);
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = Hex2BcdChar(pSta->year_mon_data.Year);
#endif                            
						}
						else if(i == 8) //模块出厂MAC地址
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->module_id, 6);
						}

						pLmpAppInfo->responsePoint += (element_len - 2);
					}
				}
#endif				
			}
			else //实时获取
			{
			    if (g_pRelayStatus->sta_info_realtime_cb.state != EM_TASK_READ_INIT)
    			{
    				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_BUSY_NOW;
                    return ERROR;
    			}
                
				memcpy(g_pRelayStatus->sta_info_realtime_cb.mac_addr, mac_addr, MAC_ADDR_LEN);
				g_pRelayStatus->sta_info_realtime_cb.task_type = EM_TASK_TYPE_INFO_REALTIME_TASK;
				g_pRelayStatus->sta_info_realtime_cb.state = EM_TASK_READ_SEND;
				g_pRelayStatus->sta_info_realtime_cb.element = element_mask;
				g_pRelayStatus->sta_info_realtime_cb.start_sn = element_total_len;
				g_pRelayStatus->sta_info_realtime_cb.element_num = element_num;
				memcpy_special(g_pRelayStatus->sta_info_realtime_cb.element_array, element_array, sizeof(element_array));

				debug_str(DEBUG_LOG_NET, "E8030313, realtime get\r\n");
				if(pLmpAppInfo->lmpLinkHeadGD->ctl_field&0x20)
				{
					g_pRelayStatus->sta_info_realtime_cb.report_sn = pLmpAppInfo->lmpLinkHeadGD->user_data[13];
				}
				else
				{
					g_pRelayStatus->sta_info_realtime_cb.report_sn = pLmpAppInfo->lmpLinkHeadGD->user_data[1];
				}
			}
#else		
			BYTE node_count = 0, real_num = 0;
			USHORT start_sn = 0;
			USHORT presp = pLmpAppInfo->responsePoint;
			USHORT ii = 0;
			USHORT *total;
			memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
			node_count = pLmpAppInfo->curProcUnit->data_unit[2];

#if 0
			PLC_get_relay_info();
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &g_pRelayStatus->cfged_meter_num, 2);
			pLmpAppInfo->responsePoint += 2;
#else			
			total = (USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			*total = 0;
			pLmpAppInfo->responsePoint += 2;
#endif

			memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &start_sn, 2);
			pLmpAppInfo->responsePoint += 2;

			pLmpAppInfo->responsePoint += 1; //本次应答的节点数量

#if 1
			//寻找真正的起始位置
			//if (PLC_GetRealNodeSnCfg(TRUE, start_sn, &start_sn))
			{
				for (ii = TEI_STA_MIN; ii <  TEI_CNT; ii++)
				{
					P_STA_INFO pSta = PLC_GetValidStaInfoByTei(ii);
					if (NULL == pSta)
						continue;
					(*total)++;
					if (start_sn == 0)
					{
						--start_sn;
						continue;
					}

					real_num++;

					memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, pSta->mac, 6);
					pLmpAppInfo->responsePoint += 6;

					memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, pSta->module_id, 6);
					pLmpAppInfo->responsePoint += 6;

					memset_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, 0, 24);
					pLmpAppInfo->responsePoint += 24;

					if (real_num == node_count)
						break;
				}
			}
#else			
			for (ii = start_sn; ii <  CCTT_METER_NUMBER; ii++)
			{
				if (!METER_IS_CFG(ii))
					continue;

				if (PLC_GetStaInfoByMac(g_MeterRelayInfo[ii].meter_addr)==NULL)
				{
					continue;
				}
				++(*total);
				if (real_num != node_count)
				{
					real_num++;
					memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, 6);
					pLmpAppInfo->responsePoint += 6;

					memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].module_id, 6);
					pLmpAppInfo->responsePoint += 6;

					memset_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,0,24);
					pLmpAppInfo->responsePoint += 24;
				}
			}
#endif			
			//本次应答的节点数量
			memcpy_special(pLmpAppInfo->msg_buffer + presp + 2 + 2, &real_num, 1);
#endif			
			return OK;
		}
	case 0xE8030314: //批量查询模块资产信息
		{
			BYTE node_count = 0, real_num = 0, element_id = 0, element_len = 0;
			USHORT start_sn = 0;
			USHORT presp = pLmpAppInfo->responsePoint;
			USHORT ii = 0;
			P_STA_INFO pSta;

			element_id = pLmpAppInfo->curProcUnit->data_unit[3];
			if ((element_id > 0x10) || (element_id == 3) || (element_id == 4) || (element_id == 7)) 
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			element_len = element_len_tbl[element_id]-2;

			memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
			node_count = pLmpAppInfo->curProcUnit->data_unit[2];
			
			pLmpAppInfo->responsePoint++; //本次应答的从节点数量

			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = element_id;

			debug_str(DEBUG_LOG_NET, "0xE8030314, element id:%d, start sn:%d, node count:%d\r\n", element_id, start_sn, node_count);

			if (start_sn == 0) //CCO
			{   
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_PlmConfigPara.main_node_addr, 6);
				pLmpAppInfo->responsePoint += 6;

				if (element_id == 0) //厂商编号，2字节ASCII
				{
					memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_Tmn_ver_info.factoryCode, 2);					
				}
				else if(element_id == 1) //软件版本信息（模块），2字节BCD
				{
                    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = Hex2BcdChar(c_Tmn_ver_info.software_v[1]);
			        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = Hex2BcdChar(c_Tmn_ver_info.software_v[0]);
				}
				else if(element_id == 2) //bootloader版本号，1字节BIN
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 1;
				}
				else if(element_id == 5) //芯片厂商代码，2字节ASCII
				{
					memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_Tmn_ver_info.chip_code, 2);
				}
				else if(element_id == 6) //固件发布日期（模块），3字节BIN
				{
					//南网21标准规定资产信息中日期BIN格式
                    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = c_Tmn_ver_info.softReleaseDate.day;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = c_Tmn_ver_info.softReleaseDate.month;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = c_Tmn_ver_info.softReleaseDate.year;                      
				}
				else if(element_id == 8) //模块出厂MAC地址，6字节BIN
				{
					memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_cco_id.chip_id, 6); //小端存储
				}
				else if(element_id == 9) //硬件版本信息（模块），2字节BCD
				{
					memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_ext_ver_info.ModuleHardVer, 2);
				}
				else if(element_id == 0xA) //硬件发布日期（模块），3字节BIN
				{
					//南网21标准规定资产信息中日期BIN格式
                    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = c_ext_ver_info.ModuleHardVerDate.day;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = c_ext_ver_info.ModuleHardVerDate.month;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = c_ext_ver_info.ModuleHardVerDate.year;
                }
				else if(element_id == 0xB) //软件版本信息（芯片），2字节BCD
				{
					memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_ext_ver_info.ChipSoftVer, 2);
				}
				else if(element_id == 0xC) //软件发布日期（芯片），3字节BIN
				{
					//南网21标准规定资产信息中日期BIN格式
                    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = c_ext_ver_info.ChipSofVerDate.day;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = c_ext_ver_info.ChipSofVerDate.month;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = c_ext_ver_info.ChipSofVerDate.year;
                }
				else if(element_id == 0xD) //硬件版本信息（芯片），2字节BCD
				{
					memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_ext_ver_info.ChipHardVer, 2);
				}
				else if(element_id == 0xE) //硬件发布日期（芯片），3字节BIN
				{
					//南网21标准规定资产信息中日期BIN格式
                    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = c_ext_ver_info.ChipHardVerDate.day;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = c_ext_ver_info.ChipHardVerDate.month;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = c_ext_ver_info.ChipHardVerDate.year;
                }
				else if(element_id == 0xF) //应用程序版本号，2字节BCD
				{
					memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_ext_ver_info.AppVer, 2);
				}
#ifndef ASSETSCODE_32BYTES_INFORM
				else if(element_id == 0x10) //通信模块资产编码，24字节ASCII
				{
					memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)g_cco_id.module_id, 24);
				}

				pLmpAppInfo->responsePoint += element_len;
#else
				else if(element_id == 0x10) //通信模块资产编码，24字节ASCII+8位补全
				{
					u8 zero_complement[8]={0};
					memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], zero_complement, 8);
					pLmpAppInfo->responsePoint +=8;
					memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)g_cco_id.module_id, 24);
				}
				if (element_id == 0x10)
				{
					pLmpAppInfo->responsePoint += element_len-8;//模块资产编码补全时已经加8了，这里只需要加element_len-8
				}
				else
				{
					pLmpAppInfo->responsePoint += element_len;
				}
				
#endif		  
				real_num++;
			}

			for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
			{
				if (real_num == node_count)
					break;
		 
				if (!METER_IS_CFG(ii))
					continue;

				if (start_sn > 1) //从节点序号从1开始
				{
					start_sn--;
					continue;
				}
		 
				real_num++;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;

				debug_str(DEBUG_LOG_NET, "0xE8030314, index:%d, STA MAC: %02x-%02x-%02x-%02x-%02x-%02x\r\n", ii,
			          g_MeterRelayInfo[ii].meter_addr[5], g_MeterRelayInfo[ii].meter_addr[4], g_MeterRelayInfo[ii].meter_addr[3], 
			          g_MeterRelayInfo[ii].meter_addr[2], g_MeterRelayInfo[ii].meter_addr[1], g_MeterRelayInfo[ii].meter_addr[0]);
				
				u8 dst_addr[LONG_ADDR_LEN] = {0};
                PLC_GetAppDstMac(g_MeterRelayInfo[ii].meter_addr, dst_addr);              
                pSta = PLC_GetStaInfoByMac(dst_addr);

				memset_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, 0xFF, element_len);
				if (pSta != NULL)
				{     					
					debug_str(DEBUG_LOG_NET, "0xE8030314, STA State:%d, geted ID:%d\r\n", pSta->net_state, pSta->geted_id);
					
					if (pSta->geted_id)
					{
						if (element_id == 0) //厂商编号，2字节ASCII
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->FactoryCode, 2);					
						}
						else if(element_id == 1) //软件版本信息（模块），2字节BCD
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ModuleSoftVer, 2);
						}
						else if(element_id == 2) //bootloader版本号，1字节BIN
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pSta->BootVer;
						}
						else if(element_id == 5) //芯片厂商代码，2字节ASCII
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ChipCode, 2);
						}
						else if(element_id == 6) //固件发布日期（模块），3字节BIN
						{
							//南网21标准规定资产信息中日期BIN格式
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pSta->ModuleSoftVerDate.Day;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = pSta->ModuleSoftVerDate.Month;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = pSta->ModuleSoftVerDate.Year;                           
						}
						else if(element_id == 8) //模块出厂MAC地址，6字节BIN
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->module_id, 6);
						}
						else if(element_id == 9) //硬件版本信息（模块），2字节BCD
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ModuleHardVer, 2);
						}
						else if(element_id == 0xA) //硬件发布日期（模块），3字节BIN
						{
							//南网21标准规定资产信息中日期BIN格式
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pSta->ModuleHardVerDate.Day;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = pSta->ModuleHardVerDate.Month;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = pSta->ModuleHardVerDate.Year;    
	                    }
						else if(element_id == 0xB) //软件版本信息（芯片），2字节BCD
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ChipSoftVer, 2);
						}
						else if(element_id == 0xC) //软件发布日期（芯片），3字节BIN
						{
							//南网21标准规定资产信息中日期BIN格式
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pSta->ChipSoftVerDate.Day;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = pSta->ChipSoftVerDate.Month;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = pSta->ChipSoftVerDate.Year;    
	                    }
						else if(element_id == 0xD) //硬件版本信息（芯片），2字节BCD
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ChipHardVer, 2);
						}
						else if(element_id == 0xE) //硬件发布日期（芯片），3字节BIN
						{
							//南网21标准规定资产信息中日期BIN格式
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pSta->ChipHardVerDate.Day;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = pSta->ChipHardVerDate.Month;
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = pSta->ChipHardVerDate.Year;    
	                    }
						else if(element_id == 0xF) //应用程序版本号，2字节BCD
						{
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->AppVer, 2);
						}
						else if(element_id == 0x10) //通信模块资资产编码，24字节ASCII
						{
#ifndef ASSETSCODE_32BYTES_INFORM
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->assetsCode, 24);
#else
							memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->assetsCode, 32);
#endif
						}
					}
				}

				pLmpAppInfo->responsePoint += element_len;
			}

			memcpy_special(pLmpAppInfo->msg_buffer + presp, &real_num, 1);

			return OK;
		}
    
    case 0xE8030321: //批量查询入网节点版本信息，PROTOCOL_NW_2017_GUANGZHOU
        {
			BYTE node_count = 0;
			USHORT start_sn = 0;
			USHORT ii = 0;
            UCHAR *real_num;
			USHORT *total_num;
            P_STA_INFO pSta;

            if (pLmpAppInfo->appLayerLen != 9) //AFN(1) + SEQ(1) + FN(4) + 节点起始序号(2) + 节点数量(1)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}
            
            memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
            node_count = pLmpAppInfo->curProcUnit->data_unit[2];

            if (start_sn == 0) //节点起始序号从1开始
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
            
            total_num = (USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			*total_num = 0;
			pLmpAppInfo->responsePoint += 2;

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &start_sn, 2);
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;
            
			for (ii=0; ii<CCTT_METER_NUMBER; ii++)
			{
				if (!METER_IS_CFG(ii))
					continue;

                u8 dst_addr[LONG_ADDR_LEN] = {0};
                PLC_GetAppDstMac(g_MeterRelayInfo[ii].meter_addr, dst_addr);
				pSta = PLC_GetStaInfoByMac(dst_addr);

                if (pSta == NULL)
                    continue;
                
				(*total_num)++;

                if (start_sn > 1) //sta序号从1开始
				{
					start_sn--;
					continue;
				}

                if (*real_num == node_count)
					continue;
                
				(*real_num)++;

                memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;

   
				//厂商代码
				memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_factory_code, 2);
				TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2); 
				pLmpAppInfo->responsePoint += 2;

				//芯片代码
				memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_chip_code, 2);
				TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2); 
				pLmpAppInfo->responsePoint += 2;

				//版本时间
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Day);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Month);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Year);

				//版本号
				memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->software_v, 2);
				TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2);	
                pLmpAppInfo->responsePoint += 2;

                //从节点特征；Bit0-1是否具备超级电容；Bit2-3是否具备地理定位
                pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x05;
			}

			return OK;
		}
    case 0xE8030322: //查询单个节点版本信息，PROTOCOL_NW_2017_GUANGZHOU
        {
            u8 node_addr[MAC_ADDR_LEN] = {0};
            USHORT meter_sn; 
                
            if (pLmpAppInfo->appLayerLen != 12) //AFN(1) + SEQ(1) + FN(4) + 节点地址(6)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}
			
			memcpy(node_addr, pLmpAppInfo->curProcUnit->data_unit, MAC_ADDR_LEN);
        
			meter_sn = PLC_get_sn_from_addr(node_addr);
			if (meter_sn >= CCTT_METER_NUMBER)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_METERNO_INEXISTS;
				return ERROR;
			}
			
			u8 dst_addr[LONG_ADDR_LEN] = {0};
			PLC_GetAppDstMac(node_addr, dst_addr);

			P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);		
			if (NULL == pSta)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
				return ERROR;
			}

			if (NET_IN_NET != pSta->net_state)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
				return ERROR;
			}

            memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], node_addr, 6);
			pLmpAppInfo->responsePoint += 6;
                
            //厂商代码
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_factory_code, 2);
			TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2); 
			pLmpAppInfo->responsePoint += 2;

			//芯片代码
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_chip_code, 2);
			TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2); 
			pLmpAppInfo->responsePoint += 2;

			//版本时间
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Day);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Month);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Year);

			//版本号
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->software_v, 2);
			TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2);
            pLmpAppInfo->responsePoint += 2;

            //从节点特征；Bit0-1是否具备超级电容；Bit2-3是否具备地理定位
            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x05;
                    
			return OK;
		}
	case 0xE8030365://查询网络拓扑
		{
			USHORT tei;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT *total_num;

			total_num = (USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			*total_num=0;
			pLmpAppInfo->responsePoint += 2;

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &start_sn, 2);
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;
			
#ifndef GUANGDONG_CSG //节点信息从白表获取，兼容二采
			if (start_sn == 0) //CCO
			{
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_pStaInfo[TEI_CCO].mac, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

				P_STA_TOPOLOGY_INFO_GD pTop = (P_STA_TOPOLOGY_INFO_GD)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
				pTop->tei = TEI_CCO;
				pTop->pco = g_pStaInfo[TEI_CCO].pco;
				pTop->level = 0;
				pTop->role = ROLES_CCO;
				pTop->InNetTime = TicksBetween64(GetTickCount64(), g_pStaStatusInfo[TEI_CCO].tick_innet) / OSCfg_TickRate_Hz;
				pTop->changePcoTime = g_pStaStatusInfo[TEI_CCO].changePcoTime;
				pTop->leaveTime = g_pStaStatusInfo[TEI_CCO].leaveTime;
				pLmpAppInfo->responsePoint += sizeof(STA_TOPOLOGY_INFO_GD);

				(*real_num)++;	
			}

			(*total_num)++; //CCO

			for (USHORT ii=0; ii<CCTT_METER_NUMBER; ii++)
			{
				if ((g_MeterRelayInfo[ii].status.used != CCO_USERD)||
					(g_MeterRelayInfo[ii].status.cfged == 0)||
					(g_MeterRelayInfo[ii].coll_sn >= TEI_CNT || g_MeterRelayInfo[ii].coll_sn < TEI_STA_MIN))
					continue;

				tei = g_MeterRelayInfo[ii].coll_sn;
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (NULL == pSta)
					continue;

				if (NET_OUT_NET == pSta->net_state)
					continue;
				
				if (!PLC_IsInNet(tei))
				{
					continue;
				}

				//华为离线也会返回，离网不返回
#if 1
				if(NET_IN_NET != pSta->net_state)
				{
					continue;
				}
#endif
				
				(*total_num)++;

				if (start_sn > 1) //sta序号从1开始
				{
					start_sn--;
					continue;
				}
				
				if (*real_num == meter_num)
					continue;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

				P_STA_TOPOLOGY_INFO_GD pTop = (P_STA_TOPOLOGY_INFO_GD)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
				P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
				pTop->tei = tei;
				pTop->pco = pSta->pco;
				pTop->level = PLC_GetStaLevel(tei);			
				if(pSta->dev_type == RELAY)
				{
					pTop->role = ROLES_RELAY;
				}
				else			
				{
					pTop->role = pSta->child == 0 ? ROLES_STA : ROLES_PCO;
				}
				pTop->InNetTime = TicksBetween64(GetTickCount64(), pStaStatus->tick_innet) / OSCfg_TickRate_Hz;
				pTop->changePcoTime = pStaStatus->changePcoTime;
				pTop->leaveTime = pStaStatus->leaveTime;
				pLmpAppInfo->responsePoint += sizeof(STA_TOPOLOGY_INFO_GD);
				
				(*real_num)++;
			}
#else			
			for (tei = TEI_CCO; tei < TEI_CNT; tei++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (NULL == pSta)
					continue;

				if (NET_OUT_NET == pSta->net_state)
					continue;
				if (!PLC_IsInNet(tei))
				{
					continue;
				}
				//华为离线也会返回，离网不返回
#if 1
				if(NET_IN_NET != pSta->net_state)
				{
					continue;
				}
#endif
				++(*total_num);
				if (start_sn!=0)
                {
                    --start_sn;
                    continue ;
                }
				if (*real_num == meter_num)
					continue;
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, pSta->mac, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

				P_STA_TOPOLOGY_INFO_GD pTop = (P_STA_TOPOLOGY_INFO_GD)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
				P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
				pTop->tei = tei;
				pTop->pco = pSta->pco;
				pTop->level = PLC_GetStaLevel(tei);
				if(pSta->dev_type == RELAY)
				{
					pTop->role = ROLES_RELAY;
				}
				else			
				{
					pTop->role = tei == CCO_TEI ? ROLES_CCO : pSta->child == 0 ? ROLES_STA : ROLES_PCO;
				}
				pTop->InNetTime =  TicksBetween64(GetTickCount64(), pStaStatus->tick_innet) / OSCfg_TickRate_Hz;
				pTop->changePcoTime = pStaStatus->changePcoTime;
				pTop->leaveTime = pStaStatus->leaveTime;
				pLmpAppInfo->responsePoint += sizeof(STA_TOPOLOGY_INFO_GD);

				++(*real_num);
			
			}
#endif
			break;
		}

		case 0xE8030375://查询双模网络拓扑
		{
			USHORT tei;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT *total_num;

			total_num = (USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			*total_num=0;
			pLmpAppInfo->responsePoint += 2;

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &start_sn, 2);
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;
			

			if (start_sn == 0) //CCO
			{
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_pStaInfo[TEI_CCO].mac, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

				P_STA_TOPOLOGY_INFO_HRF_GD pTop = (P_STA_TOPOLOGY_INFO_HRF_GD)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
				pTop->tei = TEI_CCO;
				pTop->module = 1;
				pTop->pco = g_pStaInfo[TEI_CCO].pco;
				pTop->level = 0;
				pTop->role = ROLES_CCO;
				pTop->Link=0;
				pTop->InNetTime = TicksBetween64(GetTickCount64(), g_pStaStatusInfo[TEI_CCO].tick_innet) / OSCfg_TickRate_Hz;
				pTop->changePcoTime = g_pStaStatusInfo[TEI_CCO].changePcoTime;
				pTop->leaveTime = g_pStaStatusInfo[TEI_CCO].leaveTime;
				pLmpAppInfo->responsePoint += sizeof(STA_TOPOLOGY_INFO_GD);

				(*real_num)++;	
			}

			(*total_num)++; //CCO

			for (USHORT ii=0; ii<CCTT_METER_NUMBER; ii++)
			{
				if ((g_MeterRelayInfo[ii].status.used != CCO_USERD)||
					(g_MeterRelayInfo[ii].status.cfged == 0)||
					(g_MeterRelayInfo[ii].coll_sn >= TEI_CNT || g_MeterRelayInfo[ii].coll_sn < TEI_STA_MIN))
					continue;

				tei = g_MeterRelayInfo[ii].coll_sn;
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (NULL == pSta)
					continue;

				if (NET_OUT_NET == pSta->net_state)
					continue;
				
				if (!PLC_IsInNet(tei))
				{
					continue;
				}

				//华为离线也会返回，离网不返回

				if (NET_IN_NET != pSta->net_state)
				{
					continue;
				}

				
				(*total_num)++;

				if (start_sn > 1) //sta序号从1开始
				{
					start_sn--;
					continue;
				}
				
				if (*real_num == meter_num)
					continue;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

				P_STA_TOPOLOGY_INFO_HRF_GD pTop = (P_STA_TOPOLOGY_INFO_HRF_GD)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
				P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
				pTop->tei = tei;
				pTop->module = pSta->moduleType;
				pTop->pco = pSta->pco;
				pTop->level = PLC_GetStaLevel(tei);
				pTop->Link = pSta->Link;
				pTop->role = pSta->child == 0 ? ROLES_STA : ROLES_PCO;
				pTop->InNetTime = TicksBetween64(GetTickCount64(), pStaStatus->tick_innet) / OSCfg_TickRate_Hz;
				pTop->changePcoTime = pStaStatus->changePcoTime;
				pTop->leaveTime = pStaStatus->leaveTime;
				pLmpAppInfo->responsePoint += sizeof(STA_TOPOLOGY_INFO_GD);
				
				(*real_num)++;
			}

			break;
		}
	case 0xE8000391://查询多网络信息
		{
			UCHAR *real_num = &pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_PlmConfigPara.net_id;
			PLC_CopyCCOMacAddr(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], false);
			pLmpAppInfo->responsePoint += MAC_ADDR_LEN;
			for (u8 i = 0; i < MAX_NID_NEIGHBOR_COUNT; i++)
			{
				P_NET_NEIGHBOR pNet = &s_net_neigbor[i];

				u32 nid = pNet->nid;

				if (nid < NID_MIN)
					continue;
				if (nid > NID_MAX)
					continue;
				if(NtbsBetween64(GetNtbCount64(), s_net_neigbor[i].ntb_receive) >= 120 * NTB_FRE_PER_SEC)
				{
					continue ;
				}
				(*real_num)++;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = nid;
			}
		}
		break;
	case 0xE8000390://查询宽带载波频段
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]=g_PlmConfigPara.hplc_comm_param;
		break;
	case 0xE8000399://读取无线频段
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_pPlmConfigPara->hrf_option;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_pPlmConfigPara->hrf_channel;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_pPlmConfigPara->hrf_allowChangeChannel;
			break;
		}
	case 0xE8000393://查询白名单生效信息
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]=1;//白名单打开
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]=0;//表档案
		break;
	case 0xE8030366://查询节点运行时长
		{
			P_STA_INFO pSta = PLC_GetStaInfoByMac(pLmpAppInfo->curProcUnit->data_unit);
			if (pSta != 0 && pSta->run_status.runtime != 0)
			{
				*(u32 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = GetTickCount64() / OSCfg_TickRate_Hz -pSta->run_status.runtime;
			}
			else
			{
				*(u32 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0;
			}
			pLmpAppInfo->responsePoint+=4;
		}
		break;
	case 0xE8030367: //查询从节点入网被拒信息
		{
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			USHORT allNode = (NotWhiteSta.widx < MAX_NOT_WHITE_SIZE && NotWhiteSta.ridx >= MAX_NOT_WHITE_SIZE) ? 
				NotWhiteSta.widx + MAX_NOT_WHITE_SIZE*2 - NotWhiteSta.ridx : NotWhiteSta.widx - NotWhiteSta.ridx;
			*(u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = allNode; //总结点数
			USHORT rptNode = allNode > 32 ? 32 : allNode;//每次最大上传32个
			rptNode = rptNode > meter_num ? meter_num : rptNode;
			rptNode = (start_sn + rptNode) > allNode ? allNode - start_sn : rptNode;
			rptNode = start_sn >= allNode ? 0 : rptNode;
			pLmpAppInfo->responsePoint += 2;
			UCHAR *real_num = &pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num=rptNode;
			for (int i=0;i <rptNode;i++ )
			{
				int idx = (NotWhiteSta.ridx+start_sn+i)%MAX_NOT_WHITE_SIZE;
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &NotWhiteSta.mac[idx][0], MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint+=MAC_ADDR_LEN;
			}
		}
		break;
	case 0xE8030368://查询节点信道信息
		{
			u16 start_sn=*(u16*)&pLmpAppInfo->curProcUnit->data_unit[6];
			u8 num=pLmpAppInfo->curProcUnit->data_unit[8];
			u8 addr[6];
			PLC_CopyCCOMacAddr(addr,FALSE);
			if (macCmp(addr,pLmpAppInfo->curProcUnit->data_unit))//查询主节点
			{
				if (num>20)
				{
					num=20;
				}
				P_STA_BITMAP pStaBitmap = PLC_GetCCONeighbor();
				if(NULL == pStaBitmap)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_TASK_MEMORY_NOT_ENOUGH;
					return ERROR;
				}
				
				*(u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pStaBitmap->count;
				pLmpAppInfo->responsePoint+=2;
				UCHAR *real_num = &pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
				*real_num = 0;
				
				for (int i = 0; i < pStaBitmap->size; i++)
				{
					if (pStaBitmap->bitmap[i])
					{
						for (int j=0;j<8;j++)
						{
							if (pStaBitmap->bitmap[i]&(1<<j))
							{
								if (start_sn)
								{
									--start_sn;
									continue ;
								}
								u16 tei=i<<3|j;
								ST_NEIGHBOR_TYPE* neighborInfo = 0;
								GetNeighborInfo(tei,&neighborInfo);
								//地址
								memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], PLC_GetStaInfoByTei(tei)->mac, MAC_ADDR_LEN);
								pLmpAppInfo->responsePoint+=6;
								//节点标识
								*(u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]=tei;
								pLmpAppInfo->responsePoint+=2;
								//代理节点标识
								*(u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]=g_pStaInfo[tei].pco;
								pLmpAppInfo->responsePoint+=2;
								//层级
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]=PLC_GetStaLevel(tei);
								P_STA_STATUS_INFO pStaStaus = PLC_GetStaStatusInfoByTei(tei);
								u8 up_rate = StaNeighborTab.Neighbor[tei].LastNghTxCnt*100 / pStaStaus->found_list_count_last_route;
								u8 down_rate = StaNeighborTab.Neighbor[tei].LastNghRxCnt*100 / g_pRelayStatus->found_list_count_last_route;
								if (up_rate>100)
								{
									up_rate=100;
								}
								if (down_rate>100)
								{
									down_rate=100;
								}
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = down_rate;
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = up_rate;
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = down_rate>up_rate?up_rate:down_rate;
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 10+rand()%10;//暂时只报报符合协议
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 50-(rand()%10);
								(*real_num)++;
								if(--num==0)
								{
									free_buffer(__func__,pStaBitmap);
									return OK;
								}
							}
						}
					}
				}
				free_buffer(__func__,pStaBitmap);
			}
			else
			{
				if (num>6)
				{
					num=6;
				}
				if (g_pRelayStatus->sta_channel_cb.state!=EM_TASK_READ_INIT)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_BUSY_NOW;
					return !OK;
				}
				if (PLC_GetStaInfoByMac(pLmpAppInfo->curProcUnit->data_unit)==NULL)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_METER_NOT_RESPONSE;
					return !OK;
				}
				memcpy(g_pRelayStatus->sta_channel_cb.mac_addr, pLmpAppInfo->curProcUnit->data_unit, MAC_ADDR_LEN);
				g_pRelayStatus->sta_channel_cb.num=num;
				g_pRelayStatus->sta_channel_cb.task_type = EM_TASK_TYPE_CHANNEL_TASK;
				g_pRelayStatus->sta_channel_cb.start_sn=start_sn;
				g_pRelayStatus->sta_channel_cb.state=EM_TASK_READ_SEND;



				if(pLmpAppInfo->lmpLinkHeadGD->ctl_field&0x20)
				{
					g_pRelayStatus->sta_channel_cb.report_sn = pLmpAppInfo->lmpLinkHeadGD->user_data[13];
				}
				else
				{
					g_pRelayStatus->sta_channel_cb.report_sn = pLmpAppInfo->lmpLinkHeadGD->user_data[1];
				}
			}
		}
		break;
#ifdef NW_GUANGDONG_NEW_LOCAL_INTERFACE    
    case 0xE8030398://查询节点信道信息（新一代计量自动化主站通信网络管理技术要求）
		{
			u16 start_sn = *(u16*)&pLmpAppInfo->curProcUnit->data_unit[6];
			u8 num = pLmpAppInfo->curProcUnit->data_unit[8];
			u8 addr[6];
            
			PLC_CopyCCOMacAddr(addr, FALSE);
			if (macCmp(addr, pLmpAppInfo->curProcUnit->data_unit))//查询主节点
			{
				if (num > 20)
				{
					num = 20;
				}
                
				P_STA_BITMAP pStaBitmap = PLC_GetCCONeighbor();
				if(NULL == pStaBitmap)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_TASK_MEMORY_NOT_ENOUGH;
					return ERROR;
				}

                memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_PlmConfigPara.main_node_addr, 6);
			    pLmpAppInfo->responsePoint += 6;
            
                *(u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = CCO_TEI;
				pLmpAppInfo->responsePoint+=2;

                *(u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0;
				pLmpAppInfo->responsePoint+=2;             
				
				*(u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pStaBitmap->count;
				pLmpAppInfo->responsePoint+=2;
				UCHAR *real_num = &pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
				*real_num = 0;
				
				for (int i=0; i<pStaBitmap->size; i++)
				{
					if (pStaBitmap->bitmap[i])
					{
						for (int j=0; j<8; j++)
						{
							if (pStaBitmap->bitmap[i] & (1<<j))
							{
								if (start_sn)
								{
									--start_sn;
									continue ;
								}
                                
								u16 tei = i<<3|j;
								ST_NEIGHBOR_TYPE* neighborInfo = 0;
								GetNeighborInfo(tei,&neighborInfo);
								//地址
								memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], PLC_GetStaInfoByTei(tei)->mac, MAC_ADDR_LEN);
								pLmpAppInfo->responsePoint+=6;
								//节点标识
								*(u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = tei;
								pLmpAppInfo->responsePoint+=2;
								//代理节点标识
								*(u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = g_pStaInfo[tei].pco;
								pLmpAppInfo->responsePoint+=2;
								//层级
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = PLC_GetStaLevel(tei);
								P_STA_STATUS_INFO pStaStaus = PLC_GetStaStatusInfoByTei(tei);
								u8 up_rate = StaNeighborTab.Neighbor[tei].LastNghTxCnt*100 / pStaStaus->found_list_count_last_route;
								u8 down_rate = StaNeighborTab.Neighbor[tei].LastNghRxCnt*100 / g_pRelayStatus->found_list_count_last_route;
								if (up_rate>100)
								{
									up_rate=100;
								}
								if (down_rate>100)
								{
									down_rate=100;
								}
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = down_rate;
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = up_rate;
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = down_rate>up_rate?up_rate:down_rate;
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 10+rand()%10;//暂时只报报符合协议
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 50-(rand()%10);
								(*real_num)++;
								if(--num==0)
								{
									free_buffer(__func__,pStaBitmap);
									return OK;
								}
							}
						}
					}
				}
				free_buffer(__func__,pStaBitmap);
			}
			else
			{
				if (num > 6)
				{
					num = 6;
				}
                
				if (g_pRelayStatus->sta_new_channel_cb.state != EM_TASK_READ_INIT)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_BUSY_NOW;
					return !OK;
				}
                
				if (PLC_GetStaInfoByMac(pLmpAppInfo->curProcUnit->data_unit)==NULL)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_METER_NOT_RESPONSE;
					return !OK;
				}
                
				memcpy(g_pRelayStatus->sta_new_channel_cb.mac_addr, pLmpAppInfo->curProcUnit->data_unit, MAC_ADDR_LEN);
				g_pRelayStatus->sta_new_channel_cb.num = num;
				g_pRelayStatus->sta_new_channel_cb.task_type = EM_TASK_TYPE_NEW_CHANNEL_TASK;
				g_pRelayStatus->sta_new_channel_cb.start_sn = start_sn;
				g_pRelayStatus->sta_new_channel_cb.state = EM_TASK_READ_SEND;

				if(pLmpAppInfo->lmpLinkHeadGD->ctl_field & 0x20)
				{
					g_pRelayStatus->sta_new_channel_cb.report_sn = pLmpAppInfo->lmpLinkHeadGD->user_data[13];
				}
				else
				{
					g_pRelayStatus->sta_new_channel_cb.report_sn = pLmpAppInfo->lmpLinkHeadGD->user_data[1];
				}
			}
		}
		break;    
#endif    
	case 0xE8030369://返回应用层报文信息
		{
			//暂不支持 返回0
			*(u16*)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]=0;//
			pLmpAppInfo->responsePoint+=2;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]=0;
		}
		break;
	case 0xE8030370://查询应用层报文信息
		{
			u8 addr[6];
			memcpy(addr,pLmpAppInfo->curProcUnit->data_unit, MAC_ADDR_LEN);
			P_STA_INFO pSta=PLC_GetStaInfoByMac(addr);
			if (pSta!=NULL)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->run_status.zc;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->run_status.u485;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->run_status.leave;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->run_status.reset;
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;//未知
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0xff;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0xff;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0xff;
			}
		}
		break;
#ifdef NW_GUANGDONG_NEW_LOCAL_INTERFACE    
    case 0xE8030364:    //查询设备在线状态
		{
		    USHORT tei;
            USHORT start_sn = *((USHORT *)pLmpAppInfo->curProcUnit->data_unit);
			UCHAR meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
            
			PBYTE real_num;			
			USHORT *total_num;

			PLC_get_relay_info();

            //入网节点总数量
			total_num = (USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			*total_num = 0;
			pLmpAppInfo->responsePoint += 2;

            //本次应答的节点数量
			real_num = (UCHAR *)((ULONG)pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);
			*real_num = 0;
			pLmpAppInfo->responsePoint++;

			for (USHORT ii=0; ii<CCTT_METER_NUMBER; ii++)
			{
			    if ((g_MeterRelayInfo[ii].status.used != CCO_USERD)||
					(g_MeterRelayInfo[ii].status.cfged == 0)||
					(g_MeterRelayInfo[ii].coll_sn >= TEI_CNT || g_MeterRelayInfo[ii].coll_sn < TEI_STA_MIN))
                {         
					continue;
                }
                    
				tei = g_MeterRelayInfo[ii].coll_sn;
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (NULL == pSta)
                {            
                    continue;
                }
                
				if(NET_IN_NET != pSta->net_state)
				{
					continue;
				}

                (*total_num)++;

                if (start_sn) //sta序号从0开始
				{
					start_sn--;
					continue;
				}
				
				if (*real_num == meter_num)
                {            
					continue;
                }
                
				if (g_MeterRelayInfo[ii].status.used != CCO_USERD)
				{            
					continue;
                }

                memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

                (*real_num)++;
			}
            
			return OK;
		}
    case 0xE8030396:    //查询设备类型
		{
		    USHORT tei;
            USHORT start_sn = *((USHORT *)pLmpAppInfo->curProcUnit->data_unit);
			UCHAR meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
            
			PBYTE real_num;			
			USHORT *total_num;

			PLC_get_relay_info();

            //节点总数量
			total_num = (USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			*total_num = 0;
			pLmpAppInfo->responsePoint += 2;

            //节点起始序号
            *(USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = start_sn;
            pLmpAppInfo->responsePoint += 2;

            //本次应答的节点数量
			real_num = (UCHAR *)((ULONG)pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);
			*real_num = 0;
			pLmpAppInfo->responsePoint++;

            if (start_sn == 0) //CCO
			{
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_pStaInfo[TEI_CCO].mac, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 2;

                *(UINT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0;
                pLmpAppInfo->responsePoint += 4;                

				(*real_num)++;	
			}

			(*total_num)++; //CCO

			for (USHORT ii=0; ii<CCTT_METER_NUMBER; ii++)
			{
			    if ((g_MeterRelayInfo[ii].status.used != CCO_USERD)||
					(g_MeterRelayInfo[ii].status.cfged == 0)||
					(g_MeterRelayInfo[ii].coll_sn >= TEI_CNT || g_MeterRelayInfo[ii].coll_sn < TEI_STA_MIN))
                {         
					continue;
                }
                    
				tei = g_MeterRelayInfo[ii].coll_sn;
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (NULL == pSta)
                {            
                    continue;
                }
                
				if(NET_IN_NET != pSta->net_state)
				{
					continue;
				}

                (*total_num)++;

                if (start_sn > 1) //sta序号从1开始
				{
					start_sn--;
					continue;
				}
				
				if (*real_num == meter_num)
					continue;
                
                memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

                pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->dev_type;

                *(UINT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0;
                if (pSta->offline_tick)
                {
                	P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
                    *(UINT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pStaStatus->tick_innet - pSta->offline_tick;
                }
                
                pLmpAppInfo->responsePoint += 4;

                (*real_num)++;
			}
            
			return OK;
		}
    case 0xE8000395://查询并发数
        {
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = MAX_NW_PARALLEL_NUM;
		}
		break;
    case 0xE8000397://查询组网成功率
        {
            PLC_get_relay_info();
			*(u16*)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = (g_pRelayStatus->online_num*10000)/g_pRelayStatus->cfged_meter_num;
			pLmpAppInfo->responsePoint += 2;
		}
		break;
#endif    
#if 0
	case 0xE80303FE:        //贵阳鼎信集中器扩展
		{
			//[2019-02-21 09:20:14 846][发送 0018][68 12 00 40 03 04 FE 03 03 E8 87 40 05 00 18 00 17 16]
			//[2019-02-21 09:20:14 874][接收 0017][68 11 00 80 03 04 FE 03 04 E8 04 06 F0 00 02 70 16]
			//04 05 F0 00 02        A相，07规约
			//04 06 F0 00 02        B相，07规约
			//04 07 F0 00 02        C相，07规约

			PBYTE pMeterDown = &pLmpAppInfo->curProcUnit->data_unit[0];
			BYTE protocol = 0x02;       //07规约
			BYTE phase = 0x05;          //A相

			for(u8 i=0; i<countof(g_PlmStatusPara.nodes_report); i++)
			{
				if(OK == IsEqual(&g_PlmStatusPara.nodes_report[i][0], pMeterDown, 6))
				{
					protocol = (g_PlmStatusPara.nodes_report[i][6] & 0x07);
					phase = (g_PlmStatusPara.nodes_report[i][6] >> 3) & 0x03;
					if(AC_PHASE_A==phase)
					phase = 0x05;
					else if(AC_PHASE_A==phase)
					phase = 0x06;
					else if(AC_PHASE_A==phase)
					phase = 0x07;
					break;
				}
			}

			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x04;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = phase;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0xF0;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x00;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = protocol;

			return OK;

		}
#endif
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DT;
		return ERROR;
	}

	return OK;
}

BYTE afn_func_04_gd(P_LMP_APP_INFO pLmpAppInfo)
{
	USHORT *pWaitTime = (USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];

	pLmpAppInfo->responsePoint += 2;

	*pWaitTime = 1;

	switch (pLmpAppInfo->curDT)
	{
	case 0xE8020401:    //设置主节点地址
		{
#ifdef NW_TEST
extern const u8 MAX_HPLC_POWER[];
extern u8 g_nw_test_high_power;
extern u8 g_nw_test_atten;
			//南网送检深化应用台体增大发射功率
			const u8 mac_list[2][6] = {
				{0x66, 0x55, 0x44, 0x33, 0x22, 0x11},
				{0x66, 0x55, 0x44, 0x44, 0x33, 0x22},
			};
            u8 i;
			for (i = 0; i < 2; ++i)
			{
				if (0 == memcmp(pLmpAppInfo->curProcUnit->data_unit, mac_list[i], 6))
				{
					BPLC_SetTxGain(MAX_HPLC_POWER[BPLC_TxFrequenceGet()]);
					break;
				}
			}
            if(i >= 2)
                g_nw_test_high_power=0;
            else
                g_nw_test_high_power=1;
            
            			//南网送检过衰减标志位
			const u8 mac_list_atten[6] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};

            if (0 == memcmp(pLmpAppInfo->curProcUnit->data_unit, &mac_list_atten[0], 6))
            {
                g_nw_test_atten=1;
                debug_str(DEBUG_LOG_INFO, "MAC ALL 99, NW TEST ATTEN\r\n");
            }
            else
            {
              g_nw_test_atten=0;
              debug_str(DEBUG_LOG_INFO, "MAC not 99, NW TEST other test\r\n");
            }
                  
              
#endif
			if (memcmp(g_PlmConfigPara.main_node_addr, pLmpAppInfo->curProcUnit->data_unit, 6) != 0)
			{
				memcpy_special(g_PlmConfigPara.main_node_addr, pLmpAppInfo->curProcUnit->data_unit, 6);
				SYS_STOR_MAIN_NODE(g_PlmConfigPara.main_node_addr);
				PLC_set_rf_channel_flag(FALSE);
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(TEI_CCO);
				if (NULL != pSta)
				{
					PLC_CopyCCOMacAddr(pSta->mac, FALSE);
				}
			}
			g_PlmStatusPara.restart_read_flag = TRUE;
			return OK;
		}
		//  case 0xE8000408://表箱报警参数
		//   {
		//           memcpy((U8 *)&g_PlmConfigPara.ALARM_REF,pLmpAppInfo->curProcUnit->data_unit, sizeof(ALARM_REF_C));
//            dc_flush_fram((U8 *)&g_PlmConfigPara.ALARM_REF, sizeof(ALARM_REF_C), FLASH_FLUSH );
//            return OK;
		//   }
	case 0xE8020402:    //添加从节点
		{
			#ifdef NW_TEST
			read_meter_time=0;
			#endif
			UCHAR ii, meter_num = pLmpAppInfo->curProcUnit->data_unit[0];
			PBYTE pMeterAddr = &pLmpAppInfo->curProcUnit->data_unit[1];
#ifdef PROTOCOL_NW_2020			
			//ClearOutListMeter(1);//变动从节点前清除之前档案同步的节点信息
#endif			
			g_pRelayStatus->init_meter_delay=3;//初始化档案需要延时3s后才可以请求集中器时间
			PLC_get_relay_info();
			if (g_pRelayStatus->all_meter_num + meter_num > (TEI_STA_MAX-TEI_STA_MIN+1)) 
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NONE;
				return ERROR;
			}
			
			for (ii = 0; ii < meter_num; ii++)
			{
				PLC_add_new_node(0, METER_TYPE_PROTOCOL_07, pMeterAddr);
				pMeterAddr += 6;
			}

			return OK;
		}
	case 0xE8020403:    //删除从节点
		{
			UCHAR ii, meter_num = pLmpAppInfo->curProcUnit->data_unit[0];
			PBYTE pMeterAddr = &pLmpAppInfo->curProcUnit->data_unit[1];
#ifdef PROTOCOL_NW_2020				
			//ClearOutListMeter(1);//变动从节点前清除之前档案同步的节点信息
#endif
			for (ii = 0; ii < meter_num; ii++)
			{
				PLC_del_new_node(pMeterAddr,LIST_TYPE_WHITE);
				pMeterAddr += 6;
			}

			return OK;
		}
	case 0xE8020404:    //允许/禁止上报从节点事件
		{
			g_PlmStatusPara.report_meter_event = pLmpAppInfo->curProcUnit->data_unit[0];
			g_pRelayStatus->event_switch_cb.bitmap[0]|=1;//广播
			if (g_PlmStatusPara.report_meter_event)//允许事件上报需要单拨
			{
				memset(g_pRelayStatus->event_switch_cb.bitmap,0xff,sizeof(g_pRelayStatus->event_switch_cb.bitmap));
			}
			return OK;
		}
	case 0xE8020405:    //激活从节点主动注册
		{
			USHORT selfreg_minutes_last; //*(USHORT*)(&pLmpAppInfo->curProcUnit->data_unit[0]);
			USHORT meterestimated;


			PLC_get_relay_info();
			meterestimated = g_pRelayStatus->cfged_meter_num + 5;
			meterestimated = MAX(meterestimated, 40);

			//每只表给多15秒读采集器下面的485表
			selfreg_minutes_last = (2 * meterestimated + meterestimated * 15) / 60 + 1;

			if (selfreg_minutes_last < 30)
				selfreg_minutes_last = 30;

			if (selfreg_minutes_last > 120)
				selfreg_minutes_last = 120;

			PLC_StartSelfReg(selfreg_minutes_last);

			debug_str(DEBUG_LOG_NET, "0xE8020405, len:%d, min:%d\r\n", pLmpAppInfo->appLayerLen, selfreg_minutes_last);
			//*pWaitTime = (selfreg_minutes_last + 1) * 60;
			*pWaitTime = 1;

			return OK;
		}
	case 0xE8020406:   //终止从节点主动注册
		{
			PLC_StopSelfReg();
			return OK;
		}
	case 0xE8020407:    //添加从节点通信地址映射表
		{
			UCHAR meter_num = pLmpAppInfo->curProcUnit->data_unit[0];
			P_METER_COLL_MAP pMap = (P_METER_COLL_MAP)(&pLmpAppInfo->curProcUnit->data_unit[1]);

			USHORT existed_sn, empty_sn = 0;

			for (BYTE i = 0; i < meter_num; i++)
			{
				existed_sn = Relay_Trans_MeterColl2Sn(pMap, &empty_sn);
				if ((existed_sn >= CCTT_METER_COLL_NUMBER) && (empty_sn < CCTT_METER_COLL_NUMBER))
				{
					data_flash_write_straight(((u32)&g_pMeterCollMap[empty_sn])-FLASH_AHB_ADDR,(u8*)pMap,sizeof(METER_COLL_MAP));
				}
				pMap++;
			}

			PLC_save_relay_timer_start();

			return OK;
		}
	case 0xE8020480:
		{
			debug_str(DEBUG_LOG_NET, "E8020480, PLC_StartAreaDiscernment, time:%d\r\n", *(u16*)pLmpAppInfo->curProcUnit->data_unit);
			PLC_StartAreaDiscernment();
			u32 time=*(u16*)pLmpAppInfo->curProcUnit->data_unit*60*1000;
			if (time==0)
			{
				time=1440*60*1000;
			}
			#ifdef NW_TEST
			time= 35*60*1000;
			#endif
			if (areaTimerId != INVAILD_TMR_ID)
			{
				HPLC_DelTmr(areaTimerId);
			}
			areaTimerId=HPLC_RegisterTmr(PLC_StopAreaDiscernment, NULL, time, TMR_OPT_CALL_ONCE);
		}
		return OK;
	case 0xE8020481:
		{
			debug_str(DEBUG_LOG_NET, "E8020481, PLC_StopAreaDiscernment\r\n");
			PLC_StopAreaDiscernment((void*)1);
			return OK;
		}
	case 0xE8020490:
		if (pLmpAppInfo->curProcUnit->data_unit[0]<3)
		{
		    g_pPlmConfigPara->hplc_comm_param = pLmpAppInfo->curProcUnit->data_unit[0];
			SYS_STOR_FREQ(g_pPlmConfigPara->hplc_comm_param);
            
			PLC_ChangeFrequence(pLmpAppInfo->curProcUnit->data_unit[0]);
		}
		return OK;
	case 0xE802F099: //设置无线频段
		{
			if (pLmpAppInfo->appLayerLen != 9) //AFN(1) + SEQ(1) + FN(4) + option(1)+channel(1)+ llowChangeChannel(1)
			{
				pLmpAppInfo->responsePoint -= 2;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_LEN;
				return ERROR;

			}
			u8 newchannel, newoption;
			newoption = pLmpAppInfo->curProcUnit->data_unit[0];
			newchannel = pLmpAppInfo->curProcUnit->data_unit[1];
			g_pPlmConfigPara->hrf_allowChangeChannel = pLmpAppInfo->curProcUnit->data_unit[2];
			if (g_pPlmConfigPara->hrf_option == newoption && g_pPlmConfigPara->hrf_channel == newchannel)
			{
				pLmpAppInfo->responsePoint -= 2;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			if(((newoption == 2) && (newchannel < 1 || newchannel > 80)) || ((newoption == 3) && (newchannel < 1 || newchannel > 200)))
			{
				pLmpAppInfo->responsePoint -= 2;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			uint64_t tick_now = GetTickCount64();
			if (tick_now > 11 * 1000) //30 s后cco开始发送信标若要修改信道需要信道切换流程
			{
				HRF_ChangeFrequence(newchannel, newoption);
			}
			else //11s内直接修改
			{
				HRF_ChangeChannel(newchannel, newoption);
			}

			return OK;
		}
	case 0xE8020450: //s私有协议修改串口波特率
		{
			P_TMSG pMsg = alloc_msg(__func__);
			if (NULL != pMsg)
			{
				OS_ERR err;
				pMsg->message = MSG_PLM_SET_BAUDRATE;
				pMsg->wParam = pLmpAppInfo->curProcUnit->data_unit[0];
				OSTaskQPost(&g_pOsRes->APP_PLM_2005_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);
				if (OS_ERR_NONE != err)
					free_msg(__func__,pMsg);
			}
		}
		return OK;
    case 0xE80204F0: //重启节点
        {
			u8 mac[6] = NULL;
            u8 seconds = 0;

			if (pLmpAppInfo->appLayerLen != 13) //afn(1)+seq(1)+FN(4)+Data(7)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
            seconds = pLmpAppInfo->curProcUnit->data_unit[6];

            //CCO重启
            if (memcmp(g_PlmConfigPara.main_node_addr, mac, 6) == 0)
            {
                Reboot_system(seconds, en_reboot_cmd, __func__);
                return OK;
            }

            //STA重启
			P_STA_INFO pSta = PLC_GetStaInfoByMac(mac);

			if (NULL == pSta)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
				return ERROR;
			}

			if (NET_IN_NET != pSta->net_state)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
				return ERROR;
			}

            P_RESET_STA_INFO pResetSta = (P_RESET_STA_INFO)alloc_buffer(__func__, sizeof(RESET_STA_INFO));
			if (NULL == pResetSta)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_TASK_MEMORY_NOT_ENOUGH;
				return ERROR;
			}

			memcpy_special(pResetSta->mac, mac, MAC_ADDR_LEN);
            pResetSta->seconds = seconds;
			
			PLC_ResetSta(pResetSta);

			return OK;
		}
		break;
#if 0
	case 0xE8FFFF01:   //测试
		{
			UCHAR ii, meter_num = 800;
			ULONG ii_bcd;
			BYTE meter_addr[6];
			PBYTE pMeterAddr = &meter_addr[0];

			memset_special(meter_addr, 0x00, 6);

			for(ii = 0; ii < meter_num; ii++)
			{
				ii_bcd = Hex2BcdLong(ii+1);

				memcpy_special(meter_addr, &ii_bcd, 4);
				PLC_add_new_node(0, METER_TYPE_PROTOCOL_07, pMeterAddr);
			}

			return OK;
		}
#endif
	default:
		pLmpAppInfo->responsePoint -= 2;
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DT;
		return ERROR;
	}

//    return OK;
}

BYTE afn_func_06_gd(P_LMP_APP_INFO pLmpAppInfo)
{
	switch (pLmpAppInfo->curDT)
	{
	case 0xE8060601:
		{
			struct tm t;
			memset(&t,0,sizeof(t));
			PBYTE p = &pLmpAppInfo->curProcUnit->data_unit[0];

			t.tm_sec  = Bcd2HexChar(*p++);
			t.tm_min  = Bcd2HexChar(*p++);
			t.tm_hour = Bcd2HexChar(*p++);
			t.tm_mday = Bcd2HexChar(*p++);
			t.tm_mon  = Bcd2HexChar(*p++)-1;
			t.tm_year = Bcd2HexChar(*p++) + 2000 -1900;

			g_PlmStatusPara.t_gettime_sec = mktime(&t);
			g_PlmStatusPara.tick_gettime = GetTickCount64();
			g_PlmStatusPara.t_now_sec = g_PlmStatusPara.t_gettime_sec;
			g_PlmStatusPara.no_response_times = 0;
			debug_str(DEBUG_LOG_APP, "get local time\r\n");
			//OSMboxPostOpt(s_plc_ccb.savedata_mbox, pLmpAppInfo, OS_POST_OPT_NO_SCHED);

			return OK;
		}
	default:
		return ERROR;
	}
}
extern u32 s_file_addr_start;
extern unsigned char g_updata_firmware_chk[2];
extern u32 g_update_crc32[2];
u16 g_update_crc16;

extern u32 s_code_checksum;
extern u8  s_last_left_bytes;
extern u32 s_dword;
extern u32 s_user_addr;
extern u32 s_new_version;
extern P_FLASH_USER_CHARACTER_FIELD s_pUser;

//#define CHECK_SEGLEN (0x400)
#pragma section = ".vectors"

extern char _evectors;
extern char _svectors;
BYTE afn_func_07_gd(P_LMP_APP_INFO pLmpAppInfo)
{
	USHORT *pWaitTime;

	switch (pLmpAppInfo->curDT)
	{
	case 0xE8020701:        //启动文件传输
		{
			P_FILE_TRANS_BEGIN_GD pFileBegin = (P_FILE_TRANS_BEGIN_GD)pLmpAppInfo->curProcUnit->data_unit;

#if 0
			if ((0x55 == g_PlmStatusPara.file_update_flag) || (0x56 == g_PlmStatusPara.file_update_flag))
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_BUSY_NOW;
				return ERROR;
			}
#endif
			
			switch (pFileBegin->file_property)
			{
			case FILE_PROPERTY_CLEAR_GD:
			case FILE_PROPERTY_MASTER_GD:
			case FILE_PROPERTY_SLAVE_GD:
			case FILE_PROPERTY_COLL_GD:
				{
					//增加对文件大小的校验判断，目前只支持UPDATE_FIRMWARE_DB_SIZE（512K），后续可能有最大1M的STA升级文件
					if (((pFileBegin->file_property) != FILE_PROPERTY_CLEAR_GD) && ((pFileBegin->file_size) != 0) && ((pFileBegin->file_seg_count) != 0) && ((pFileBegin->file_size) > UPDATE_FIRMWARE_DB_SIZE))
					{
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
						debug_str(DEBUG_LOG_UPDATA, "0xE8020701, file size over:%d\r\n", pFileBegin->file_size);
						return ERROR;
					}
					g_pRelayStatus->sta_update_cb.update_flag = 0;
					memset_special(g_file_trans_map, 0, (UPDATE_FILE_MAP_SIZE));
					memset_special(g_updata_firmware_chk, 0, sizeof(g_updata_firmware_chk));
					g_PlmStatusPara.file_segid_gd = 0;

					s_code_checksum = 0;
					if (s_pUser!=NULL)
					{
						free_buffer(__func__,s_pUser);
					}
					SYS_STOR_GetUpdateUserInfo(NULL, &s_pUser, (unsigned long *)&s_user_addr, &s_new_version);
					g_PlmStatusPara.file_addr = s_pUser->code_addr;
					s_file_addr_start = g_PlmStatusPara.file_addr;
					s_pUser->code_size = 0xffffffff;
					s_pUser->character = 0xffffffff;
					Unprotection1MFlash();
					data_flash_write_straight(s_user_addr, (u8 *)s_pUser, sizeof(FLASH_USER_CHARACTER_FIELD));
					Protection1MFlash();
					s_pUser->character = 0x0123abcd;

					g_update_crc32[0] = 0xFFFFFFFF;
					g_update_crc32[1] = 0xFFFFFFFF;
					g_update_crc16 = 0x0000;
					memcpy_special(&g_PlmStatusPara.file_param_gd, pFileBegin, sizeof(FILE_TRANS_BEGIN_GD));

//					if ((FILE_PROPERTY_SLAVE_GD == pFileBegin->file_property)
//						|| (FILE_PROPERTY_COLL_GD == pFileBegin->file_property))
//					{
//						data_flash_write_straight(g_PlmStatusPara.file_addr, (u8 *)&g_PlmStatusPara.file_param_gd, sizeof(FILE_TRANS_BEGIN_GD));
//						g_PlmStatusPara.file_addr += sizeof(FILE_TRANS_BEGIN_GD);
//					}

					break;
				}
			default:
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			pWaitTime = (USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			pLmpAppInfo->responsePoint += 2;
			*pWaitTime = 1;

			return OK;
		}
	case 0xE8020702:    //传输文件内容
		{
			P_FILE_TRANS_CONTENT_GD pFileContent = (P_FILE_TRANS_CONTENT_GD)pLmpAppInfo->curProcUnit->data_unit;

			if (g_PlmStatusPara.file_segid_gd != pFileContent->file_seg_id)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			else if (s_pUser==NULL)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			if (FILE_PROPERTY_MASTER_GD == g_PlmStatusPara.file_param_gd.file_property)
			{
				if (!(g_file_trans_map[pFileContent->file_seg_id >> 3] & (0x01 << (pFileContent->file_seg_id & 0x07))))
				{
					unsigned short ii;

					g_pRelayStatus->sta_update_cb.update_flag = 1;
					
					if (pFileContent->file_seg_id == (g_PlmStatusPara.file_param_gd.file_seg_count - 1))
						pFileContent->file_seg_len -= 2;
					Unprotection1MFlash();
					data_flash_write_straight(g_PlmStatusPara.file_addr, (u8 *)&pFileContent->file_content[0], pFileContent->file_seg_len);
					Protection1MFlash();
					data_flash_read_straight(g_PlmStatusPara.file_addr, (u8 *)&pFileContent->file_content[0], pFileContent->file_seg_len);
					g_PlmStatusPara.file_addr += pFileContent->file_seg_len;

					for (ii = 0; ii < pFileContent->file_seg_len; ii++)
					{
						g_updata_firmware_chk[0] += pFileContent->file_content[ii];
						g_updata_firmware_chk[1] ^= pFileContent->file_content[ii];
					}

					for (ii = 0; ii < pFileContent->file_seg_len;)
					{
						u8 cpy_size = (4 - s_last_left_bytes);
						memcpy_special(((u8 *)&s_dword) + s_last_left_bytes, &pFileContent->file_content[ii], cpy_size);
						s_code_checksum += s_dword;
						s_dword = 0;
						s_last_left_bytes = 0;
						ii += cpy_size;

						if ((pFileContent->file_seg_len - ii) < 4)
						{
							s_last_left_bytes = (pFileContent->file_seg_len - ii);
							memcpy_special(((u8 *)&s_dword), &pFileContent->file_content[ii], s_last_left_bytes);
							break;
						}
					}

					g_file_trans_map[pFileContent->file_seg_id >> 3] |= (UCHAR)(0x01 << (pFileContent->file_seg_id & 0x07));

					//最后一段，检查自带校验，正确则升级
					if (pFileContent->file_seg_id == (g_PlmStatusPara.file_param_gd.file_seg_count - 1))
					{
						u32 ownCode;
						
						g_pRelayStatus->sta_update_cb.update_flag = 0;
						
						#if defined(__ICCARM__)
						data_flash_read_straight((s_pUser->code_addr+__section_size(".vectors")-4), (u8*)&ownCode, sizeof(ownCode));
						#elif defined(__GNUC__)
						data_flash_read_straight(s_pUser->code_addr+(u32)(&_evectors-&_svectors))-4), (u8*)&ownCode, sizeof(ownCode));
						#endif
						if (ownCode==FACTORY_CODE_FLAG
							&&(g_updata_firmware_chk[0] == pFileContent->file_content[pFileContent->file_seg_len])
							&& (g_updata_firmware_chk[1] == pFileContent->file_content[pFileContent->file_seg_len + 1]))
						{
							//校验
							s_pUser->character = 0x0123abcd;
							s_pUser->version = s_new_version;
							s_pUser->code_size = g_PlmStatusPara.file_addr - s_file_addr_start;
							if (0 == s_pUser->code_size % 4)
							{
								#if defined(ZB204_CHIP)
								s_pUser->need_copy = 1;
								#endif
								s_pUser->code_checksum = s_code_checksum;
								s_pUser->field_checksum = Get_checksum_32((unsigned long *)s_pUser, (sizeof(FLASH_USER_CHARACTER_FIELD) - 4) / 4);

								//flash完整读回来再算一遍，以防中途被改变；此处没有考虑和文件下发时的校验和做对比，以后可能看情况考虑下
								u32 check_code_checksum = Get_checksum_32((unsigned long *)(FLASH_AHB_ADDR + s_file_addr_start), s_pUser->code_size / 4);

								if (check_code_checksum != s_code_checksum)
								{  
									pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_CRC;
									free_buffer(__func__,s_pUser);
									s_pUser=NULL;
									debug_str(DEBUG_LOG_UPDATA, "0xE8020702, flash error , code_checksum :%x != check_checksum :%x\r\n", s_code_checksum, check_code_checksum);
									return ERROR;
								}

								Unprotection1MFlash();
								data_flash_write_straight(s_user_addr, (u8 *)s_pUser, sizeof(FLASH_USER_CHARACTER_FIELD));								
								Protection1MFlash();
								SYS_UP_ReWrite((UPDATA_PARA*)(FLASH_AHB_ADDR+s_pUser->code_addr+s_pUser->code_size-sizeof(UPDATA_PARA)));
								Reboot_system(3, 0, __func__);
							}
							free_buffer(__func__,s_pUser);
							s_pUser=NULL;

						}
						else
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_CRC;
							free_buffer(__func__,s_pUser);
							s_pUser=NULL;

                            //兼容机台厂家文件传输测试（测试文件不是CCO升级文件），文件传输完成后会通过AFN07 0xE8000703查询文件信息中“已成功接收文件段数”，
                            g_PlmStatusPara.file_segid_gd = pFileContent->file_seg_id + 1;
                            
							return ERROR;
						}
						
					}
				}
			}
			else
			{
#if 0
				//校验检查
				USHORT trans_sum = *(USHORT*)&pFileContent->file_content[pFileContent->file_seg_len];
				USHORT cacla_sum = get_crc_16(0, &pFileContent->file_content[0], pFileContent->file_seg_len);
				if(trans_sum != cacla_sum)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_CRC;
					return ERROR;
				}
#endif
				if (!(g_file_trans_map[pFileContent->file_seg_id >> 3] & (0x01 << (pFileContent->file_seg_id & 0x07))))
				{
					g_pRelayStatus->sta_update_cb.update_flag = 1;
					
					Unprotection1MFlash();
					data_flash_write_straight(g_PlmStatusPara.file_addr, (u8 *)&pFileContent->file_content[0], pFileContent->file_seg_len);
					Protection1MFlash();
					data_flash_read_straight(g_PlmStatusPara.file_addr, pFileContent->file_content, pFileContent->file_seg_len);
					g_update_crc32[0] = GetCrcInit32(g_update_crc32[0], pFileContent->file_content, pFileContent->file_seg_len);
					g_update_crc16 = GetInitCrc16(g_update_crc16, pFileContent->file_content, pFileContent->file_seg_len);

					g_PlmStatusPara.file_addr += pFileContent->file_seg_len;

					g_file_trans_map[pFileContent->file_seg_id >> 3] |= (UCHAR)(0x01 << (pFileContent->file_seg_id & 0x07));

					//最后一段，启动从节点或采集器升级
					if (pFileContent->file_seg_id == (g_PlmStatusPara.file_param_gd.file_seg_count - 1))
					{
						g_PlmStatusPara.file_update_flag = 0x55;

						if (g_PlmStatusPara.file_param_gd.file_check_sum == g_update_crc16)
						{
//							unsigned char chk_res[1];
//							chk_res[0] = LOAD_APP_REQ;
//							data_flash_write_straight(ADDR_LOAD_BOOT_FLAG, (u8 *)chk_res, 1);
							//启动升级子节点
							u32 file_size = g_PlmStatusPara.file_addr - s_file_addr_start;
							u32 crcmask = ((((u32)1 << (32 - 1)) - 1) << 1) | 1;
							g_update_crc32[0] ^= 0xFFFFFFFF;
							g_update_crc32[0] &= crcmask;

							PLC_StartUpdateSta(s_file_addr_start, file_size, g_update_crc32[0]);
							free_buffer(__func__,s_pUser);
							s_pUser=NULL;
						}
						else
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_CRC;
							free_buffer(__func__,s_pUser);
							s_pUser=NULL;
							return ERROR;
						}
						
					}
				}
			}

			g_PlmStatusPara.file_segid_gd = pFileContent->file_seg_id + 1;

			pWaitTime = (USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			pLmpAppInfo->responsePoint += 2;
			*pWaitTime = 1;

			return OK;
		}
	case 0xE8000703:    //查询文件信息
		{
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &g_PlmStatusPara.file_param_gd, sizeof(g_PlmStatusPara.file_param_gd));
			pLmpAppInfo->responsePoint += sizeof(g_PlmStatusPara.file_param_gd);

			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &g_PlmStatusPara.file_segid_gd, 2);
			pLmpAppInfo->responsePoint += 2;

			return OK;
		}
	case 0xE8000704:    //查询文件处理进度
		{
			P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;

			if(pcb->update_flag != 1)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0; //全部成功
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;
				memset(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, 2);
				pLmpAppInfo->responsePoint += 2;
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1; //正在处理
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_PlmStatusPara.file_param_gd.file_id;
				memset(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, 2);
				pLmpAppInfo->responsePoint += 2;
			}
			return OK;
		}
	case 0xE8030705:    //查询文件传输失败节点
		{
			USHORT start_sn;
//        u8 count;

			memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
//        count = pLmpAppInfo->curProcUnit->data_unit[2];

			PLC_get_relay_info();
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
						   &g_pRelayStatus->cfged_meter_num, 2);
			pLmpAppInfo->responsePoint += 2;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;

			return OK;
		}
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DT;
		return ERROR;
	}

//    return OK;
}

BYTE afn_func_08_gd(P_LMP_APP_INFO pLmpAppInfo)
{
#ifdef PLC_X4_FUNC
	X4_READ_TASK_INFO *Task_ptr;
	u8 leng, ans, i;
	u16 crc, Crc_Val;
//    u8 *Addr;
	ans = ERROR;
	switch (pLmpAppInfo->curDT)
	{
	case 0xE8020801:
		Task_ptr = (X4_READ_TASK_INFO *)pLmpAppInfo->curProcUnit->data_unit;
		if (Task_ptr->protocol == 2)
		{
			leng = 9 + (Task_ptr->di_count * 6);

		}
		else
		{
			leng = 9 + (Task_ptr->di_count * 4);
		}
		crc = get_crc16(pLmpAppInfo->curProcUnit->data_unit, leng);
		Crc_Val = pLmpAppInfo->curProcUnit->data_unit[leng - 8];
		Crc_Val <<= 8;
		Crc_Val += pLmpAppInfo->curProcUnit->data_unit[leng - 9];


		if (crc == Crc_Val)
		{
			memcpy(&g_X4ReadTaskInfo[Task_ptr->task_id - 1].task, pLmpAppInfo->curProcUnit->data_unit, leng + 2);
			g_X4ReadTaskInfo[Task_ptr->task_id - 1].status = CCO_USERD;
//                Addr=pLmpAppInfo->lmpLinkHeadGD->user_data;
			X4_Task_AddtoSTA(pLmpAppInfo->lmpLinkHeadGD->user_data + 6, Task_ptr->task_id); //把任务添加到STA
			ans = OK;
		}
		ans = ERROR;
		break;
	case   0xE8020802:
		if (X4_Task_DeltoSTA(pLmpAppInfo->lmpLinkHeadGD->user_data + 6, pLmpAppInfo->curProcUnit->data_unit[0]))
		{
			ans = OK;
			g_X4ReadTaskInfo[pLmpAppInfo->curProcUnit->data_unit[0] - 1].status = CCO_IDLE;
		}
		else
			ans = ERROR;
		break;
	case   0xE8000803: //查询采集任务号
		if (OK == IsEqualSpecialData(pLmpAppInfo->lmpLinkHeadGD->user_data + 6, 0x99, 6))
		{
			leng = 0;
			for (i = 0; i <= CCTT_X4_READ_TASK_INFO_NUMBER; i++)
			{
				if ((g_X4ReadTaskInfo[i].status == CCO_USERD) && g_X4ReadTaskInfo[i].task.task_id)
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_X4ReadTaskInfo[i].task.task_id;
				g_PlmStatusPara.restart_read_flag = TRUE;
			}
			ans = OK;
		}
		else
		{ //查询STA中的任务号
			PLC_ReadCollTaskID(pLmpAppInfo->lmpLinkHeadGD->user_data + 6, pLmpAppInfo->lmpLinkHeadGD->user_data[pLmpAppInfo->cur_afn_pos + 1]);
			pLmpAppInfo->confirmFlag = 2;
			ans = OK;
		}

		break;
	case   0xE8030804: //查询采集任务信息
		if (OK == IsEqualSpecialData(pLmpAppInfo->lmpLinkHeadGD->user_data + 6, 0x99, 6))
		{
			if (pLmpAppInfo->curProcUnit->data_unit[0] <= CCTT_X4_READ_TASK_INFO_NUMBER)
			{
				if ((g_X4ReadTaskInfo[pLmpAppInfo->curProcUnit->data_unit[0] - 1].status == CCO_USERD) && g_X4ReadTaskInfo[pLmpAppInfo->curProcUnit->data_unit[0]].task.task_id)
				{

					if (g_X4ReadTaskInfo[pLmpAppInfo->curProcUnit->data_unit[0]].task.protocol == 2)
					{
						leng = 9 + (g_X4ReadTaskInfo[pLmpAppInfo->curProcUnit->data_unit[0]].task.di_count * 6);

					}
					else
					{
						leng = 9 + (g_X4ReadTaskInfo[pLmpAppInfo->curProcUnit->data_unit[0]].task.di_count * 4);
					}

				}
				memcpy(pLmpAppInfo->msg_buffer, (u8 *)&g_X4ReadTaskInfo[pLmpAppInfo->curProcUnit->data_unit[0]].task, leng + 2);
				pLmpAppInfo->responsePoint = leng + 2;
				ans = OK;

			}
		}
		else
		{ //去读STA中的采集任务
			PLM_ReadCollTaskDetailResponse_gd(pLmpAppInfo->lmpLinkHeadGD->user_data[pLmpAppInfo->cur_afn_pos + 1], pLmpAppInfo->msg_buffer, 1);
		}
		break;
	case   0xE8020805: //添加采集数据读取任务
		if (pLmpAppInfo->curProcUnit->data_unit[0] < CCTT_X4_CLL_TASK_NUM_MAX)
		{
			g_X4_CLL_Task[pLmpAppInfo->curProcUnit->data_unit[0] - 1].status = CCO_USERD;
			memcpy(&g_X4_CLL_Task[pLmpAppInfo->curProcUnit->data_unit[0] - 1].task, pLmpAppInfo->curProcUnit->data_unit, 4);
			ans = OK;
		}
		else
			ans = ERROR;

		break;
	case   0xE8020806: //删除采集数据读取任务
		if (pLmpAppInfo->curProcUnit->data_unit[0] < CCTT_X4_CLL_TASK_NUM_MAX)
		{
			g_X4_CLL_Task[pLmpAppInfo->curProcUnit->data_unit[0] - 1].status = CCO_IDLE;
			//memcpy(&g_X4_CLL_Task[pLmpAppInfo->curProcUnit->data_unit[0]-1].task,pLmpAppInfo->curProcUnit->data_unit,4);
			ans = OK;
		}
		else
			ans = ERROR;

		break;
	case   0xE8030807:
		if ((pLmpAppInfo->curProcUnit->data_unit[0] < CCTT_X4_CLL_TASK_NUM_MAX) && (g_X4_CLL_Task[pLmpAppInfo->curProcUnit->data_unit[0] - 1].status == CCO_USERD))
		{

			memcpy(pLmpAppInfo->msg_buffer, &g_X4_CLL_Task[pLmpAppInfo->curProcUnit->data_unit[0] - 1].task, 4);
			pLmpAppInfo->responsePoint = 4;
			ans = OK;
		}
		else
			ans = ERROR;

		break;
	}
	return (ans);
#else
	return ERROR;
#endif
}

extern u8 zero_check_pass[];
extern u32 zero_check_ntb[];

BYTE afn_func_F0_gd(P_LMP_APP_INFO pLmpAppInfo)
{
	switch (pLmpAppInfo->curDT)
	{
	case 0xE802f001: //中慧产测，CCO Wafer号
		{
			if (pLmpAppInfo->appLayerLen != 6) //afn(1)+seq(1)+FN(4)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			BPLC_GetWaferID(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			pLmpAppInfo->responsePoint += 8;

			return OK;
		}
	case 0xE802f003: //断开抄控器链接
		{
			g_pRelayStatus->connectReader=0;
			return OK;
		}
	case 0xE802f007://强制设置cco相线
		{
			if (pLmpAppInfo->curProcUnit->data_unit[0]>=PHASE_A&&pLmpAppInfo->curProcUnit->data_unit[0]<=PHASE_C)
			{
				g_PlmStatusPara.force_phase=pLmpAppInfo->curProcUnit->data_unit[0];
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}

            return OK;
		}
		
    case 0xE802F010: //激活台区识别，PROTOCOL_NW_2017_GUANGZHOU
		{
		    if (pLmpAppInfo->appLayerLen != 16) //AFN(1) + SEQ(1) + FN(4) + 开始时间(6) + 持续时间(2) + 从节点重发次数(1) + 随机时间片个数(1)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}
            
		    P_AFN11_F5 pDataUnit = (P_AFN11_F5)pLmpAppInfo->curProcUnit->data_unit;
            
			PLC_StartAreaDiscernment();

            debug_str(DEBUG_LOG_NET, "AFNF0 E802F010, PLC_StartAreaDiscernment, time:%d\r\n", pDataUnit->time_last_min);

            u32 time = pDataUnit->time_last_min*60*1000;
			if (time == 0)
			{
				time = 1440*60*1000;
			}
            
			if (areaTimerId != INVAILD_TMR_ID)
			{
				HPLC_DelTmr(areaTimerId);
			}
			areaTimerId = HPLC_RegisterTmr(PLC_StopAreaDiscernment, NULL, time, TMR_OPT_CALL_ONCE);

            USHORT *pWaitTime = (USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			pLmpAppInfo->responsePoint += 2;
			*pWaitTime = 1;

			return OK;
		}
	case 0xE802F011: 
		{
			if (pLmpAppInfo->appLayerLen == 7) //AFN(1) + SEQ(1) + FN(4) + 频段(1)
			{
				//设置CCO 通信频段
				HPLC_ChangeFrequence(pLmpAppInfo->curProcUnit->data_unit[0], false);
				return OK;
			}
			else if (pLmpAppInfo->appLayerLen == 6) //AFN(1) + SEQ(1) + FN(4)
			{	
				//终止台区识别，PROTOCOL_NW_2017_GUANGZHOU
				debug_str(DEBUG_LOG_NET, "AFNF0 E802F011, PLC_StopAreaDiscernment\r\n");
				PLC_StopAreaDiscernment((void*)1);

                USHORT *pWaitTime = (USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			    pLmpAppInfo->responsePoint += 2;
			    *pWaitTime = 1;
            
				return OK;
			}
			
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
			return ERROR;
		}
    case 0xE800F012:    //查询CCO通信频段下行帧
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]=FrequenceGet();
			return OK;
		}
	case 0xE803F012: //批量查询从节点内部版本信息(暂未考虑批量查询临时节点（非cfged）内部版本的逻辑) 版本号小端序
		{
			BYTE node_count = 0, real_num = 0;
			USHORT start_sn = 0;
			USHORT presp = pLmpAppInfo->responsePoint;
			USHORT ii = 0;
			P_STA_INFO pSta;
		   
			memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
			node_count = pLmpAppInfo->curProcUnit->data_unit[2];

			PLC_get_relay_info();
			u16 total_num = g_pRelayStatus->cfged_meter_num+1; //CCO和从节点总数量
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &total_num, 2);
			pLmpAppInfo->responsePoint += 2;

			pLmpAppInfo->responsePoint++;

			if (start_sn == 0) //CCO
			{   
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_PlmConfigPara.main_node_addr, 6);
				pLmpAppInfo->responsePoint += 6;
			 
				//内部版本号
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = (SLG_COMMIT_HASH >> 12) & 0xff; //内部版本号
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = (SLG_COMMIT_HASH >> 20) & 0xff;
				
				real_num++;
			}

			//寻找真正的起始位置
			//if (PLC_GetRealNodeSnCfg(TRUE, start_sn, &start_sn))
			{
				for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
				{
					if (real_num == node_count)
						break;
			 
					if (!METER_IS_CFG(ii))
						continue;

					if (start_sn > 1) //从节点序号从1开始
					{
						start_sn--;
						continue;
					}
			 
					real_num++;

					memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, 6);
					pLmpAppInfo->responsePoint += 6;
			 
					u8 dst_addr[LONG_ADDR_LEN] = {0};
					PLC_GetAppDstMac(g_MeterRelayInfo[ii].meter_addr, dst_addr);

					pSta = PLC_GetStaInfoByMac(dst_addr);

					if (pSta != NULL)
					{
						if((pSta-g_pStaInfo) == TEI_CCO)
						{
							//内部版本号
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = (SLG_COMMIT_HASH >> 12) & 0xff; //内部版本号
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = (SLG_COMMIT_HASH >> 20) & 0xff;
						}
						else
						{
							//内部版本号
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->inner_v[0];
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->inner_v[1];
						}
					}
					else
					{
						//内部版本号赋值0
						memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, 2);
						pLmpAppInfo->responsePoint += 2;
					}
				}
			}

			memcpy_special(pLmpAppInfo->msg_buffer + presp + 2, &real_num, 1);

			return OK;
		}
	case 0xE802F014: //设置HPLC/HRF临时发送功率（产测）
		{
			if(pLmpAppInfo->curProcUnit->data_unit[0] == 0) //设置HPLC临时发送功率，AFN+Fn+数据
			{
				BPLC_SetTxGain(pLmpAppInfo->curProcUnit->data_unit[BPLC_TxFrequenceGet()+1]);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;				
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &pLmpAppInfo->curProcUnit->data_unit[1], 4);
				pLmpAppInfo->responsePoint += 4;					
			}
			else if(pLmpAppInfo->curProcUnit->data_unit[0] == 1)//设置HRF临时发送功率-无补偿
			{
				HRF_SetTxGain_abs(pLmpAppInfo->curProcUnit->data_unit[(HRF_Option&0x1)+1]);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1;				
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &pLmpAppInfo->curProcUnit->data_unit[1], 2);
				pLmpAppInfo->responsePoint += 2;	
			}
			else
			{
                HRF_SetTxGain(pLmpAppInfo->curProcUnit->data_unit[(HRF_Option&0x1)+1]);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 2;				
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &pLmpAppInfo->curProcUnit->data_unit[1], 2);
				pLmpAppInfo->responsePoint += 2;					
			}
			
			//memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			
			return OK;
		}
	case 0xE802F017: //打开/关闭singletones输出（产测）
		{
			HRF_phy_single_tone(pLmpAppInfo->curProcUnit->data_unit[0]);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pLmpAppInfo->curProcUnit->data_unit[0];
			return OK;
		}
	case 0xE802F018: //设置/查询HRF发送功率补偿（产测）
		{
			if(pLmpAppInfo->curProcUnit->data_unit[0] != 1) 
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}
			
			if(pLmpAppInfo->appLayerLen != 7) //设置
			{
				//pLmpAppInfo->curProcUnit->data_unit[0]==8
				memcpy(g_PlmConfigPara.HRF_TXChlPowerComp, &pLmpAppInfo->curProcUnit->data_unit[1], 2);
				SYS_STOR_HRF_POWER_COMP();
                SetHrfPowerComp(g_PlmConfigPara.HRF_TXChlPowerComp[0],g_PlmConfigPara.HRF_TXChlPowerComp[1]);
				//u8 gain = HRF_GetTxGain();
				HRF_SetTxGain(g_PlmConfigPara.HRF_TXChlPowerComp[HRF_Option&0x1]);		
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1;				
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_PlmConfigPara.HRF_TXChlPowerComp, sizeof(g_PlmConfigPara.HRF_TXChlPowerComp));
				pLmpAppInfo->responsePoint += 2;					
			}
			else//查询
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1;
                SYS_READ_HRF_POWER_COMP();				
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_PlmConfigPara.HRF_TXChlPowerComp, sizeof(g_PlmConfigPara.HRF_TXChlPowerComp));
				pLmpAppInfo->responsePoint += 2;			
			}
			
			return OK;
		}
#if defined(ZB205_CHIP)
	case 0xE804F251://固定增益（产测）
		{
            hrf_agc_factory_set(pLmpAppInfo->curProcUnit->data_unit[0]);
			return OK;
		} 
#endif		
	case 0xE802f025:
		{
			pLmpAppInfo->curProcUnit->data_unit[0] = pLmpAppInfo->curProcUnit->data_unit[0]?0:1; //上位机1为开启白名单，0为关闭白名单
			if (pLmpAppInfo->curProcUnit->data_unit[0] != g_PlmConfigPara.close_white_list)
			{
				g_PlmConfigPara.close_white_list = pLmpAppInfo->curProcUnit->data_unit[0];
				SYS_STOR_WHITE_LIST_SWITCH(g_PlmConfigPara.close_white_list);

                //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
                SYS_READ_WHITE_LIST_SWITCH();
			}
			return OK;
		}
	case 0xE803f026:
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = !g_PlmConfigPara.close_white_list;
			return OK;
		}
	case 0xE804F001:    ////南网附件5，5.2.1本地频段切换
		{
			if(pLmpAppInfo->curProcUnit->data_unit[0] <= 2)
			{
				HPLC_ChangeFrequence(pLmpAppInfo->curProcUnit->data_unit[0],false);
				memset(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, 4);
				pLmpAppInfo->responsePoint += 4;
			}
			else
			{
				memset(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, 4);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 1;
				pLmpAppInfo->responsePoint += 4;
			}
			
			return OK;
		}
	case 0xE80FF001:       //私有，读唯一标识MAC
		{
			u8 dev_type = pLmpAppInfo->curProcUnit->data_unit[0];
			u8 id_type = pLmpAppInfo->curProcUnit->data_unit[1];
			
			if (2 == dev_type)     //1 抄控器	 2 集中器本地通信单元  3 电表通信单元	 4 中继器  5 II型采集器  6 I型采集器单元  7 三相电表通信单元
			{
				if (id_type == 1)  //MAC信息
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    //设备类型
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = id_type;     //ID类型
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_cco_id.chip_id, 6);
					pLmpAppInfo->responsePoint += 6;
					return OK;
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    //设备类型
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = id_type;     //ID类型
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1;
					return ERROR;
				}
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    //设备类型
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = id_type;     //ID类型
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1;
				return ERROR;
			}

			break;
		}
	case 0xE80FF002:       //私有，写唯一标识MAC
		{
			u8 dev_type = pLmpAppInfo->curProcUnit->data_unit[0];
			u8 id_type = pLmpAppInfo->curProcUnit->data_unit[1];
			if (2 == dev_type)     //1 抄控器	 2 集中器本地通信单元  3 电表通信单元	 4 中继器  5 II型采集器  6 I型采集器单元  7 三相电表通信单元
			{
				if (id_type == 1)  //MAC信息
				{
					memcpy(g_cco_id.chip_id, &pLmpAppInfo->curProcUnit->data_unit[2], 6);
					SYS_STOR_CCO_ID();

                    //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
                    SYS_READ_CCO_ID();
                    
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    //设备类型
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = id_type;     //ID类型
					//MAC信息
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_cco_id.chip_id, 6);
					pLmpAppInfo->responsePoint += 6;
					return OK;
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    //设备类型
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = id_type;     //ID类型
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1;
					return ERROR;
				}
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    //设备类型
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = id_type;     //ID类型
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1;
				return ERROR;
			}

			break;
		}
	case 0xE802F228://查询CCO的IO状态
		{
			if (pLmpAppInfo->appLayerLen != 6) //afn(1)+seq(1)+FN(4)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			//IO字段，和sta复用。第1字节STA Event，第2字节STA Insert，第3字节CCO Reset，其余保留
			memset(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0, 6);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = ResetRead();
			pLmpAppInfo->responsePoint += 6;
						
			return OK;
		}
	case 0xE802F019: //设置主节点厂商代码和版本信息（生产）
		{
			if(pLmpAppInfo->appLayerLen != 15) //用户数据长度，AFN+Fn+数据
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}

			c_Tmn_ver_info.factoryCode[0] = pLmpAppInfo->curProcUnit->data_unit[1];
			c_Tmn_ver_info.factoryCode[1] = pLmpAppInfo->curProcUnit->data_unit[0];
			c_Tmn_ver_info.chip_code[0] = pLmpAppInfo->curProcUnit->data_unit[3];
			c_Tmn_ver_info.chip_code[1] = pLmpAppInfo->curProcUnit->data_unit[2];
			c_Tmn_ver_info.softReleaseDate.day = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[4]);
			c_Tmn_ver_info.softReleaseDate.month = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[5]);
			c_Tmn_ver_info.softReleaseDate.year = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[6]);
			c_Tmn_ver_info.software_v[0] = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[8]);
			c_Tmn_ver_info.software_v[1] = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[7]);
			
			SYS_STOR_VER_INFO();

            //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
            SYS_READ_VER_INFO();
			
			return OK;
		}
	case 0xE803F0E3: //查询从节点内部版本信息（生产）
		{
			u8 sta_mac[6] = NULL;

			if (pLmpAppInfo->appLayerLen != 12) //afn(1)+seq(1)+FN(4)+Data(6)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

			if (NULL == pSta)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
				return ERROR;
			}

			if (NET_IN_NET != pSta->net_state)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
				return ERROR;
			}
			
			u8* node_mac = (u8 *)alloc_buffer(__func__, MAC_ADDR_LEN);
			if(NULL == node_mac)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_TASK_MEMORY_NOT_ENOUGH;
				return ERROR;
			}
			memcpy_special(node_mac, sta_mac, MAC_ADDR_LEN);
			
			PLC_InnerVer(node_mac);

			return OK;
		}
	case 0xE800F0E4: //查询主节点内部版本信息（生产）
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = c_Tmn_ver_info.factoryCode[1];
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = c_Tmn_ver_info.factoryCode[0];
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = c_Tmn_ver_info.chip_code[1];
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = c_Tmn_ver_info.chip_code[0];
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.software_v[1]); //模块版本号
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.software_v[0]);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.day); //模块版本日期
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.month);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.year);

			memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0, 4); //软件版本号
			pLmpAppInfo->responsePoint += 4;
			memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0, 3); //软件发布日期
			pLmpAppInfo->responsePoint += 3;
			memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0, 4); //硬件版本号
			pLmpAppInfo->responsePoint += 4;
			memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0, 3); //硬件发布日期
			pLmpAppInfo->responsePoint += 3;
			
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(SLG_COMMIT_DATE); //内部版本日期
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(SLG_COMMIT_MON);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(SLG_COMMIT_YEAR % 100);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = (SLG_COMMIT_HASH >> 12) & 0xff; //内部版本号
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = (SLG_COMMIT_HASH >> 20) & 0xff;
			return OK;
		}
	case 0xE800E0F7: //查询主节点过零信号信息（生产）
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 3; //采集方式，双沿
			for(u8 i=0; i<6; i++)
			{
				memset(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0, 5);
				if(zero_check_pass[i])
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1; //采集到过零信号
					memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &zero_check_ntb[i], 4); //过零信号周期
					pLmpAppInfo->responsePoint += 4;
				}
				else
				{
					pLmpAppInfo->responsePoint += 5;
				}
			}
			return OK;
		}
	case 0xE802F020:
		{
			if (pLmpAppInfo->curProcUnit->data_unit[0]!=4
				&&pLmpAppInfo->curProcUnit->data_unit[0]!=8)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_CMD_NOT_SUPPORT;
				pLmpAppInfo->responsePoint++;
				return ERROR;
			}
			P_TMSG pMsg = alloc_msg(__func__);
			if (NULL != pMsg)
			{
				OS_ERR err;
				pMsg->message = MSG_PLM_SET_BAUDRATE;
				if (pLmpAppInfo->curProcUnit->data_unit[0]==8)
				{
					pMsg->wParam = 1;
				}
				else 
				{
					pMsg->wParam = 0;
				}
				OSTaskQPost(&g_pOsRes->APP_PLM_2005_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);
				if (OS_ERR_NONE != err)
					free_msg(__func__, pMsg);
			}
			return OK;
		}
	case 0xE802F216:  //查询主节点内部CODE信息（升级flag、省份宏等）
		{
			#if defined(GUIZHOU_SJ)
				DefaultCodeInfo.AreaProtocol |= 1<<0;
			#endif	
			#if defined(GUANGXI_CSG)
				DefaultCodeInfo.AreaProtocol |= 1<<1;
			#endif	
			#if defined(SHENZHEN_CSG)
				DefaultCodeInfo.AreaProtocol |= 1<<2;
			#endif	
			#if defined(YUNNAN_CSG)
				DefaultCodeInfo.AreaProtocol |= 1<<3;
			#endif	
			#if defined(GUANGDONG_CSG)
				DefaultCodeInfo.AreaProtocol |= 1<<4;
			#endif	
			#if defined(NW_TEST)
				DefaultCodeInfo.AreaProtocol |= (u32)1<<31;
			#endif  

			memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &DefaultCodeInfo.Chip[0], sizeof(DefaultCodeInfo.Chip));
			pLmpAppInfo->responsePoint += sizeof(DefaultCodeInfo.Chip);
			memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &DefaultCodeInfo.Factory[0], sizeof(DefaultCodeInfo.Factory));
			pLmpAppInfo->responsePoint += sizeof(DefaultCodeInfo.Factory);
			memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8*)&DefaultCodeInfo.Factory_code, sizeof(DefaultCodeInfo.Factory_code));
			pLmpAppInfo->responsePoint += sizeof(DefaultCodeInfo.Factory_code);
			memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8*)&DefaultCodeInfo.AreaProtocol, sizeof(DefaultCodeInfo.AreaProtocol));
			pLmpAppInfo->responsePoint += sizeof(DefaultCodeInfo.AreaProtocol);
			
			u8 branch_len = strlen(SLG_BRANCH_NAME);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = branch_len;
			strcpy((char *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], SLG_BRANCH_NAME);
			pLmpAppInfo->responsePoint += branch_len;
			
			return OK;
		}
	case 0xE802F129://设置主节点rf发送功率
		{
			if (pLmpAppInfo->curProcUnit->data_unit[0] != 0xff)
			{
#if (HRF_MIN_POWER_VAL == 0)
				if (pLmpAppInfo->curProcUnit->data_unit[0] > HRF_MAX_POWER_VAL)
#else
				if ((pLmpAppInfo->curProcUnit->data_unit[0] < HRF_MIN_POWER_VAL) || (pLmpAppInfo->curProcUnit->data_unit[0] > HRF_MAX_POWER_VAL))
#endif
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DT;
					return ERROR;
				}
			}
			data_flash_write_straight_with_crc(STA_RF_POWER_MODE_ADDR, pLmpAppInfo->curProcUnit->data_unit, 1);
			return OK;
		}
	case 0xE802F130://查询主节点rf发送功率
		{
			u8 rfpower;
			data_flash_read_straight_with_crc_noadc(STA_RF_POWER_MODE_ADDR,&rfpower,1,0xff);
			if(rfpower == 0xff)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0xff;
				pLmpAppInfo->responsePoint += 1;
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = HRF_LinePower;
				pLmpAppInfo->responsePoint += 1;
			}
			return OK;
		}		
	case 0xE802F219://设置主节点功率
		{
			for (int i = 0; i < 4; i++)
			{
				if (pLmpAppInfo->curProcUnit->data_unit[i] == 0xff)
				{
					memset(pLmpAppInfo->curProcUnit->data_unit,0xff,4);
					break;
				}
				
#if (HPLC_MIN_POWER_VAL == 0)
				if (pLmpAppInfo->curProcUnit->data_unit[i] > HPLC_MAX_POWER_VAL)
#else
				if ((pLmpAppInfo->curProcUnit->data_unit[i] < HPLC_MIN_POWER_VAL) || (pLmpAppInfo->curProcUnit->data_unit[i] > HPLC_MAX_POWER_VAL))
#endif
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DT;
					return ERROR;
				}
			}
			data_flash_write_straight_with_crc(STA_POWER_MODE_ADDR, pLmpAppInfo->curProcUnit->data_unit, 4);
			return OK;
		}
	case 0xE802F220://查询主节点功率
		{
			u8 power[4];
			data_flash_read_straight_with_crc_noadc(STA_POWER_MODE_ADDR,power,4,0xff);
			if(memIsHex(power, 0xff, 4))
			{
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], power, 4);
				pLmpAppInfo->responsePoint += 4;
			}
			else
			{
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], HPLC_ChlPower, sizeof(HPLC_ChlPower));
				pLmpAppInfo->responsePoint += 4;
			}
			return OK;
		}
	case 0xE802F171://设置从节点功率 新接口
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 17) //afn(1)+seq(1)+FN(4)+Data(11)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, pLmpAppInfo->curProcUnit->data_unit, 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x61,&pLmpAppInfo->curProcUnit->data_unit[6]);
			return OK;
		}
	case 0xE802F172://查询从节点功率 新接口
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 12) //afn(1)+seq(1)+FN(4)+Data(6)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x60,NULL);
			return OK;
		}
	case 0xE802F173://设置从节点rf功率 新接口
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 15) //afn(1)+seq(1)+FN(4)+Data(9)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, pLmpAppInfo->curProcUnit->data_unit, 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x63,&pLmpAppInfo->curProcUnit->data_unit[6]);
			return OK;
		}
	case 0xE802F174://查询从节点rf功率 新接口
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 12) //afn(1)+seq(1)+FN(4)+Data(6)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x62,NULL);
			return OK;
		}
	case 0xE802F221://设置从节点功率
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 16) //afn(1)+seq(1)+FN(4)+Data(10)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, pLmpAppInfo->curProcUnit->data_unit, 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x3F,&pLmpAppInfo->curProcUnit->data_unit[6]);
			return OK;
		}
	case 0xE802F222://查询从节点功率
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 12) //afn(1)+seq(1)+FN(4)+Data(6)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x3E,NULL);
			return OK;
		}
	case 0xE802F229://设置从节点rf功率
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 14) //afn(1)+seq(1)+FN(4)+Data(8)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, pLmpAppInfo->curProcUnit->data_unit, 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x48,&pLmpAppInfo->curProcUnit->data_unit[6]);
			return OK;
		}
	case 0xE802F230://查询从节点rf功率
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 12) //afn(1)+seq(1)+FN(4)+Data(6)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x47,NULL);
			return OK;
		}
#ifdef HPLC_CSG	 //只有南网版本支持
	case 0xE802F233://重新生成网络ID
		{
			if (pLmpAppInfo->appLayerLen != 6) //afn(1)+seq(1)+FN(4)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			
			HPLC_ChangeNID();

			return OK;
		}
	case 0xE802F234://查询网络ID
		{
			if (pLmpAppInfo->appLayerLen != 6) //afn(1)+seq(1)+FN(4)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_pPlmConfigPara->net_id;

			return OK;
		}	
#endif	
    case 0xE802F241://查询接收抄控器RSSI
		{
		    if (!g_pRelayStatus->connectReader)
    		{
    			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_CMD_NOT_SUPPORT;
				return ERROR;
    		}
            
			if (pLmpAppInfo->appLayerLen != 6&&pLmpAppInfo->appLayerLen != 7) //afn(1)+seq(1)+FN(4)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			if (pLmpAppInfo->appLayerLen==7)
			{
				ReaderPhase=pLmpAppInfo->curProcUnit->data_unit[0];
			}
            *(s32 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = g_pRelayStatus->recv_rssi;
            pLmpAppInfo->responsePoint += 4;

			return OK;
		}	
	case 0xE802F244://CCO本地接口直接接收抄控器RSSI
		{            
			if (pLmpAppInfo->appLayerLen != 6) //afn(1)+seq(1)+FN(4)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

            *(s32 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = g_pRelayStatus->factory_recv_rssi;
            pLmpAppInfo->responsePoint += 4;

			return OK;
		}	
    case 0xE802F247: //查询主节点扩展版本信息（资产信息使用）
		{
		    if (pLmpAppInfo->appLayerLen != 6) //afn(1)+seq(1)+FN(4)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
            
		    //模块硬件版本信息
            memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_ext_ver_info.ModuleHardVer, 2);
            pLmpAppInfo->responsePoint += 2;
            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_ext_ver_info.ModuleHardVerDate.day);
            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_ext_ver_info.ModuleHardVerDate.month);
            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_ext_ver_info.ModuleHardVerDate.year);

            //芯片硬件版本信息
            memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_ext_ver_info.ChipHardVer, 2);
            pLmpAppInfo->responsePoint += 2;
            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_ext_ver_info.ChipHardVerDate.day);
            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_ext_ver_info.ChipHardVerDate.month);
            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_ext_ver_info.ChipHardVerDate.year);

            //芯片软件版本信息            
            memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_ext_ver_info.ChipSoftVer, 2);
            pLmpAppInfo->responsePoint += 2;
            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_ext_ver_info.ChipSofVerDate.day);
            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_ext_ver_info.ChipSofVerDate.month);
            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_ext_ver_info.ChipSofVerDate.year);

            //应用程序版本号                  
            memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)c_ext_ver_info.AppVer, 2);
            pLmpAppInfo->responsePoint += 2;
            
			return OK;
		}
    case 0xE802F248: //设置主节点扩展版本信息（资产信息使用）
		{
		    if (pLmpAppInfo->appLayerLen != 23) //afn(1)+seq(1)+FN(4)+Data(17)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
            
            //模块硬件版本信息
            memcpy_swap((u8 *)c_ext_ver_info.ModuleHardVer, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
            c_ext_ver_info.ModuleHardVerDate.day = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[2]);
            c_ext_ver_info.ModuleHardVerDate.month = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[3]);
            c_ext_ver_info.ModuleHardVerDate.year = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[4]);

            //芯片硬件版本信息
            memcpy_swap((u8 *)c_ext_ver_info.ChipHardVer, &pLmpAppInfo->curProcUnit->data_unit[5], 2);
            c_ext_ver_info.ChipHardVerDate.day = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[7]);
            c_ext_ver_info.ChipHardVerDate.month = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[8]);
            c_ext_ver_info.ChipHardVerDate.year = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[9]);

            //芯片软件版本信息            
            memcpy_swap((u8 *)c_ext_ver_info.ChipSoftVer, &pLmpAppInfo->curProcUnit->data_unit[10], 2);
            c_ext_ver_info.ChipSofVerDate.day = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[12]);
            c_ext_ver_info.ChipSofVerDate.month = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[13]);
            c_ext_ver_info.ChipSofVerDate.year = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[14]);

            //应用程序版本号                  
            memcpy_swap((u8 *)c_ext_ver_info.AppVer, &pLmpAppInfo->curProcUnit->data_unit[15], 2);
			
			SYS_STOR_EXT_VER_INFO();

            //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
            SYS_READ_EXT_VER_INFO();
			
			return OK;
		}
	case 0xE802F249://查询是否允许采集器模块自身地址入网（不在档案中）的开关
        {
            if (pLmpAppInfo->appLayerLen != 6) //afn(1)+seq(1)+FN(4)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_pPlmConfigPara->allow_collect_innet;
            return OK;
        }
    case 0xE802F250://设置是否允许采集器模块自身地址入网（不在档案中）的开关
        {
            if (pLmpAppInfo->appLayerLen != 7) //afn(1)+seq(1)+FN(4)+Data(1)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

            u8 mode = (pLmpAppInfo->curProcUnit->data_unit[0]==1)?1:0;

            if (mode != g_pPlmConfigPara->allow_collect_innet)
            {
                g_PlmConfigPara.allow_collect_innet = mode;
				SYS_STOR_COLLECT_INNET_MODE(mode);

                //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
                SYS_READ_COLLECT_INNET_MODE();
            }

            return OK;
        }
	case 0xE802F133://设置debug_mask
		{
#ifndef DEBUG_UART
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_CMD_NOT_SUPPORT;
			return ERROR;
#endif
			if (pLmpAppInfo->appLayerLen != (6+DebugGetMaskLen())) //afn(1)+seq(1)+FN(4)+Data(4)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			DebugSetMask(*(u32 *)(&pLmpAppInfo->curProcUnit->data_unit[0]));
			//存FLASH
			SYS_STOR_DEBUGMASK(*(u32 *)(&pLmpAppInfo->curProcUnit->data_unit[0]));

			return OK;
		}
	case 0xE802F134://查询debug_mask
		{
#ifndef DEBUG_UART
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_CMD_NOT_SUPPORT;
			return ERROR;
#endif
			if (pLmpAppInfo->appLayerLen != 6) //afn(1)+seq(1)+FN(4)+Data(0)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			*(u32 *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]) = DebugGetMask();
			pLmpAppInfo->responsePoint += DebugGetMaskLen();
			*(u32 *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]) = DebugGetMaskLater();
			pLmpAppInfo->responsePoint += DebugGetMaskLen();
			
			return OK;
		}
	case 0xE802F328: //设置南网主节点资产编码
		{
			u8 dev_type = pLmpAppInfo->curProcUnit->data_unit[0];
			u8 *pMacAddr = (u8 *)&pLmpAppInfo->curProcUnit->data_unit[1];
			u8 id_type = pLmpAppInfo->curProcUnit->data_unit[7];
			u8 id_len = pLmpAppInfo->curProcUnit->data_unit[8];
            
			if (2 == dev_type)     //1 抄控器	 2 集中器本地通信单元  3 电表通信单元	 4 中继器  5 II型采集器  6 I型采集器单元  7 三相电表通信单元
			{
				if (0x02 == id_type && id_len == sizeof(g_cco_id.module_id))  //适配产测上位机（和国网模块ID的id_type相同），但是资产编码长度为24字节
				{
					memcpy(g_cco_id.module_id, &pLmpAppInfo->curProcUnit->data_unit[9], id_len);
					SYS_STOR_CCO_ID();

                    //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
                    SYS_READ_CCO_ID();

                    //设备类型
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    
					//设备地址
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pMacAddr, 6);
					pLmpAppInfo->responsePoint += 6;
                    //ID 类型
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = id_type;    
                    //资产编码 长度
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = sizeof(g_cco_id.module_id);      
					//资产编码 信息
					P_STA_CHIP_ID_INFO pChipID = (P_STA_CHIP_ID_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
					memcpy_special(pChipID, g_cco_id.module_id, sizeof(g_cco_id.module_id));
					pLmpAppInfo->responsePoint += sizeof(g_cco_id.module_id);

                    return OK;
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
					return ERROR;
				}
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
		}
    case 0xE802F329: //查询南网主节点资产编码
		{
			u8 dev_type = pLmpAppInfo->curProcUnit->data_unit[0];
			u8 *pMacAddr = (u8 *)&pLmpAppInfo->curProcUnit->data_unit[1];
			u8 id_type = pLmpAppInfo->curProcUnit->data_unit[7];

			if (2 == dev_type)     //1 抄控器	 2 集中器本地通信单元  3 电表通信单元	 4 中继器  5 II型采集器  6 I型采集器单元  7 三相电表通信单元
			{
				if (0x02 == id_type)  //适配产测上位机（和国网模块ID的id_type相同）
				{
				    //设备类型
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    
					//设备地址
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pMacAddr, 6);
					pLmpAppInfo->responsePoint += 6;
                    //ID 类型
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = id_type;    
                    //资产编码 长度
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = sizeof(g_cco_id.module_id);      
                    //资产编码 信息
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_cco_id.module_id, sizeof(g_cco_id.module_id));
					pLmpAppInfo->responsePoint += sizeof(g_cco_id.module_id);

                    return OK;
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
					return ERROR;
				}
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
		}
	case 0xE802F425://查询cco频偏
		{
			if (pLmpAppInfo->appLayerLen != 6) //afn(1)+seq(1)+FN(4)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
            
            s32 temp_PLL_Trimming = g_PlmConfigPara.PLL_Trimming*10/16; //PLL_Trimming单位1/16ppm，转换为单位0.1ppm
            
			if ((g_PlmConfigPara.PLL_Vaild == 1) && (temp_PLL_Trimming == 0))
			{
				*(s16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 1;
			}
			else
			{
				*(s16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = temp_PLL_Trimming;
			}
			pLmpAppInfo->responsePoint+=2;
			return OK;
		}
    case 0xE802F100://设置和查询需要比对的从节点内部版本信息
        {
            if (pLmpAppInfo->appLayerLen == 6) //afn(1)+seq(1)+FN(4)
            {
                memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &g_PlmConfigPara.sta_inner_ver, 6);
                pLmpAppInfo->responsePoint += 2;

                return OK;
            }
            else if (pLmpAppInfo->appLayerLen == 8) //afn(1)+seq(1)+FN(4)+Data(2)
            {
                memcpy_special((u8 *)&g_PlmConfigPara.sta_inner_ver, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
                SYS_STOR_COMPARE_STA_INVER();

                //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
                SYS_READ_COMPARE_STA_INVER();
                
                return OK;
            }

            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
            return ERROR;
		}
    case 0xE802F101://批量查询从节点比对内部版本信息结果
        {
            if (pLmpAppInfo->appLayerLen != 9) //afn(1)+seq(1)+FN(4)+Data(从节点起始序号(2)+从节点数量(1))
            {
                pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
                return ERROR;
            }
            
    		BYTE node_count = 0, real_num = 0;
    		USHORT start_sn = 0;
    		USHORT presp = pLmpAppInfo->responsePoint;
    		USHORT ii = 0;
    		P_STA_INFO pSta = NULL;
            P_STA_STATUS_INFO pStaStatus = NULL;
    	   
    		memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2); //从节点起始序号
    		if (start_sn == 0) //从节点序号从1开始
    		{   
    			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
                return ERROR;
    		}
            
    		node_count = pLmpAppInfo->curProcUnit->data_unit[2]; //本次查询的从节点数量

    		PLC_get_relay_info();
    		u16 total_num = g_pRelayStatus->cfged_meter_num; //从节点总数量
    		
    		memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &total_num, 2);
    		pLmpAppInfo->responsePoint += 2;

    		pLmpAppInfo->responsePoint++;

			for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
			{
				if (real_num == node_count)
					break;
		 
				if (!METER_IS_CFG(ii))
					continue;

				if (start_sn > 1) //从节点序号从1开始
				{
					start_sn--;
					continue;
				}
		 
				real_num++;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;
		 
				u8 dst_addr[LONG_ADDR_LEN] = {0};
                PLC_GetAppDstMac(g_MeterRelayInfo[ii].meter_addr, dst_addr);

                u16 tei = PLC_GetStaSnByMac(g_MeterRelayInfo[ii].meter_addr);
                pSta = PLC_GetValidStaInfoByTei(tei);
                if (pSta != NULL)
				{     
				    
					//厂商代码
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_factory_code, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2); 
					pLmpAppInfo->responsePoint += 2;

					//芯片代码
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_chip_code, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2); 
					pLmpAppInfo->responsePoint += 2;

                    //版本时间
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Day);
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Month);
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Year);

					//版本号
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->software_v, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2); 
					pLmpAppInfo->responsePoint += 2;

                    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0;
                    pStaStatus = PLC_GetStaStatusInfoByTei(tei);
                    if (pStaStatus != NULL)
    				{   
                        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pStaStatus->res; //内部版本比对结果
                    }
                    pLmpAppInfo->responsePoint += 1;
				}
                else
                {
                    memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, 10);
                    pLmpAppInfo->responsePoint += 10;
                }
			}

			memcpy_special(pLmpAppInfo->msg_buffer + presp + 2, &real_num, 1);

			return OK;
		}
    case 0xE802F102://查询实时查询从节点资产信息（AFN03 E8030313）没有响应回复全0的开关
        {
            if (pLmpAppInfo->appLayerLen != 6) //afn(1)+seq(1)+FN(4)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_pPlmConfigPara->element_report0_mode;
            return OK;
        }
    case 0xE802F103://设置实时查询从节点资产信息（AFN03 E8030313）没有响应回复全0的开关
        {
            if (pLmpAppInfo->appLayerLen != 7) //afn(1)+seq(1)+FN(4)+Data(1)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

            u8 mode = (pLmpAppInfo->curProcUnit->data_unit[0]==1)?1:0;

            if (mode != g_pPlmConfigPara->element_report0_mode)
            {
                g_PlmConfigPara.element_report0_mode = mode;
				SYS_STOR_ELEMENT_REPORT0_MODE(mode);

                //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
                SYS_READ_ELEMENT_REPORT0_MODE();
            }

            return OK;
        }
#ifdef ZT_CCO    
    case 0xE802F400://设置主节点功率
		{
			for (int i = 0; i < 4; i++)
			{
				if (pLmpAppInfo->curProcUnit->data_unit[i] < 15 || pLmpAppInfo->curProcUnit->data_unit[i] > 31)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DT;
					return ERROR;
				}
			}
			data_flash_write_straight_with_crc(STA_POWER_MODE_ADDR, pLmpAppInfo->curProcUnit->data_unit, 4);
			return OK;
		}
	case 0xE802F401://查询主节点功率
		{
			memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], HPLC_ChlPower, sizeof(HPLC_ChlPower));
			pLmpAppInfo->responsePoint += 4;
			return OK;
		}
#endif   
	case 0xE800F0E7://查询主节点寄存器值
		{
			u32 addr = *(u32 *)&pLmpAppInfo->curProcUnit->data_unit[0];
			u16 len = *(u32 *)&pLmpAppInfo->curProcUnit->data_unit[4];
			
			if(len > MSA_MESSAGAE_MAX_SIZE || pLmpAppInfo->appLayerLen != 12) //afn(1)+seq(1)+FN(4)+Data(6)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)&len, 2);
			pLmpAppInfo->responsePoint += 2;
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)addr, len);
			pLmpAppInfo->responsePoint += len;
			return OK;
		}
	case 0xE800F0E8://查询从节点寄存器值
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 18) //afn(1)+seq(1)+FN(4)+Data(12)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendRegisterRead(sta_mac,&pLmpAppInfo->curProcUnit->data_unit[6], 6);
			return OK;
		}
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DT;
		return ERROR;
	}

//	return OK;
}

#endif
extern uint32_t ZC_Time[3];       //如果不接线，则对应的值是0
const u8 phaseTable[4][6]={
	{
	0x39, //ABC
	0x2d, //ACB
	0x36, //BAC
	0x1e, //BCA
	0x27, //CAB
	0x1b, //CBA
	},
	{
		0x39&0xfc, //ABo
		0x2d&0xfc, //ACo
		0x36&0xfc, //BAo
		0x1e&0xfc, //BCo
		0x27&0xfc, //CAo
		0x1b&0xfc, //CBo
	},
	{
		0x39&0xf3, //AoC
		0x2d&0xf3, //AoB
		0x36&0xf3, //BoC
		0x1e&0xf3, //BoA
		0x27&0xf3, //CoB
		0x1b&0xf3, //CoA
	},
	{
		0x39&0xcf, //oBC
		0x2d&0xcf, //oCB
		0x36&0xcf, //oAC
		0x1e&0xcf, //oCA
		0x27&0xcf, //oAB
		0x1b&0xcf, //oBA
	},
};

USHORT afn_get_meter_phase_info(USHORT node_sn)
{
	meter_phase info;
	memset(&info, 0, sizeof(meter_phase));
	if (node_sn >= CCTT_METER_NUMBER)
		return info.info;

	P_STA_INFO pSta;
	u8 phy_phase;
#ifdef HPLC_CSG
	u8 dst_addr[LONG_ADDR_LEN] = {0};
    PLC_GetAppDstMac(g_MeterRelayInfo[node_sn].meter_addr, dst_addr);
    
    pSta = PLC_GetStaInfoByMac(dst_addr);                
#else
	pSta = PLC_GetStaInfoByTei(node_sn);
#endif
	if (pSta != NULL)
	{
	    if((pSta-g_pStaInfo) == TEI_CCO)
		{
			pSta->phy_phase = (ZC_Time[0] ? PHASE_A : 0) |
				(ZC_Time[1] ? PHASE_B : 0) << 2 |
				(ZC_Time[2] ? PHASE_C : 0) << 4 |
				(1 << 7);
		}
		else
		{
#if 0
			if (pSta->phy_phase & 0x3c)
			{
				pSta->phy_phase |= 1 << 7;
			}
#endif
		}
		phy_phase = pSta->phy_phase;
		debug_str(DEBUG_LOG_APP, "3762 phase %x, edge:%d-%d-%d-%d, getd:%d, mac is %02x%02x%02x%02x%02x%02x\r\n", pSta->phy_phase,
			      pSta->phase_edge_ask, pSta->V2_7_get_edge, pSta->phase_edge, pSta->phase_edge_vaile, pSta->geted_phase,
			      pSta->mac[5], pSta->mac[4], pSta->mac[3], pSta->mac[2], pSta->mac[1], pSta->mac[0]);
		if (pSta->phase_edge_ask || pSta->V2_7_get_edge)
		{
			if (pSta->phase_edge != pSta->phase_edge_vaile
				&& (phy_phase & 0x3f))
			{
				debug_str(DEBUG_LOG_APP, "line N<->A\r\n");
				phy_phase |= (1 << 6); //陕西零火反接
			}
		}
#if (!defined(HPLC_CSG) && !defined(PHASE_REMAIN)) || defined(NW_TEST)
		else 
		{
			phy_phase&=~(0x3f);
		}
#endif
		debug_str(DEBUG_LOG_APP, "up phase is %x\r\n", phy_phase);
#ifdef HPLC_CSG
		if ((phy_phase & 0x3f) == 0)
		{
			info.phase_features = 2; //相线不确定
		}
		else
		{
			info.phase_features = 0; //支持相线识别
		}
#else
//  	if ((phy_phase&0x3f) == 0) {
//			return 0;
//		}
#endif
		u8 get_phase=0;
		for (int i = 0; i < 3; i++)
		{
			volatile u8 phase = (phy_phase >> (i * 2)) & 0x3;
#ifndef UP_STA_BINDING_POST           
			if (phase == PHASE_A)
			{
				info.phase_info_a = 1;
				++get_phase;
			}
			else if (phase == PHASE_B)
			{
				info.phase_info_b = 1;
				++get_phase;
			}
			else if (phase == PHASE_C)
			{
				info.phase_info_c = 1;
				++get_phase;
			}
#else//上报sta接线柱
			if (phy_phase&(1<<7))
			{
				if (phase != 0)
				{
					switch (i)
					{
					case 0:
						info.phase_info_a = 1;
						break;
					case 1:
						info.phase_info_b = 1;
						break;
					case 2:
						info.phase_info_c = 1;
						break;
					}
					++get_phase;
				}
			}
			else
			{
				if (phase == PHASE_A)
				{
					info.phase_info_a = 1;
					++get_phase;
				}
				else if (phase == PHASE_B)
				{
					info.phase_info_b = 1;
					++get_phase;
				}
				else if (phase == PHASE_C)
				{
					info.phase_info_c = 1;
					++get_phase;
				}
			}
#endif
		}

		if(!USE_OLD_PHASE)
		{

			if (!(phy_phase & (0x1 << 7)))
			{
				if (phy_phase & (1 << 6))
				{

#ifndef HPLC_CSG
					info.line_error = 1; ///零火反接
#endif
					info.phase_sq = 6; //零火反接

				}
#ifndef HPLC_CSG
				info.meter_type = 0;
#else
				info.meter_type = 1;
#endif
			}
			else
			{
#ifndef HPLC_CSG
				info.meter_type = 1;
				if ((info.info & 0x7) != 0x7  //不是三项全都有
					&& (phy_phase & 0x3f)) //并且得到了相位信息
				{
					info.line_error = 1;
				}
#else
				info.meter_type = 2;
#endif
				int i, j;
				info.phase_sq = 0x7;
				for (j=0;
#if !defined(HPLC_CSG)&&defined(UP_STA_BINDING_POST)
					 j < 4;
#else
					 j<1;
#endif
					 j++)
				{
					for (i = 0; i < 6; i++)
					{
#ifndef HPLC_CSG
						if (i != 0)
						{
							info.line_error = 1; //断项
						}
#endif
						if ((phy_phase & 0x3f) == phaseTable[j][i])
						{
							info.phase_sq = i;
							break;
						}
					}
					if (i != 6)
					{
						break;
					}
				}
				if (info.phase_sq == 0x7
					&& (phy_phase & 0x3f))
				{
#ifndef HPLC_CSG
					info.line_error = 1; //断项
#else
					if (phy_phase & (1 << 6))
					{
						info.phase_sq = 6; //零火反接
					}
#endif
				}
				else if ((phy_phase & 0x3f) == 0)
				{
#ifndef HPLC_CSG
					info.line_error = 0;
#endif
				}
				if ((phy_phase & 0x3f) && (phy_phase & (1 << 6))) //零火反接
				{
#ifndef HPLC_CSG
					info.line_error = 1; //断项
#endif
					info.phase_sq = 6; //零火反接
				}
			}
			if (get_phase >= 3)
			{
				info.phase_info_a = 1;
				info.phase_info_b = 1;
				info.phase_info_c = 1;
			}
		}

#ifdef HPLC_CSG
		//规约类型
		info.protocol = PLC_meterprotocol_to_3762protocol(g_MeterRelayInfo[node_sn].meter_type.protocol);
#endif
	}
	else
	{
#ifdef HPLC_CSG
		info.phase_features = 2;
		info.meter_type = 3;
#endif
	}

	if (!USE_OLD_PHASE)
	{
		if ((pSta - g_pStaInfo) == TEI_CCO)
		{
			info.meter_type = 0;
		}
	}
	debug_str(DEBUG_LOG_APP, "info phase is %x\r\n", info.info);
	return info.info;
}

#ifdef GDW_2019_GRAPH_STOREDATA
//陕西读取相位
typedef union
{
	u16 info;
	struct {
		u8 meter_forme:1;
		u8 phase_sq:3;
		u8 line_error:1;
		u8 :2;
		u8 phase_info_c:2;
		u8 phase_info_b:2;
		u8 phase_info_a:2;
		u8 meter_type:2;
	};

}meter_phase_xian;//西安
USHORT afn_get_meter_shanxi_phase_info(USHORT node_sn)
{
	meter_phase_xian info;
	memset(&info,0,sizeof(info));
	if (node_sn >= CCTT_METER_NUMBER)
		return 0;

	P_STA_INFO pSta;
	u8 phy_phase;
	pSta=PLC_GetStaInfoByTei(node_sn);
	if (pSta!=NULL)
	{
		if (node_sn == TEI_CCO)
		{
			pSta->phy_phase = (ZC_Time[0] ? PHASE_A : 0) |
				(ZC_Time[1] ? PHASE_B : 0) << 2 |
				(ZC_Time[2] ? PHASE_C : 0) << 4 |
				(1 << 7);
		}
		//P_RELAY_INFO p_relay = (P_RELAY_INFO)(METER_RELAY_INFO_ADDR + 0x12000000);
		u16 sn=PLC_GetWhiteNodeSnByStaInfo(pSta);
		if (sn < CCTT_METER_NUMBER && g_MeterRelayInfo[sn].status.cfged)
		{
			info.meter_forme = 1;
		}
		else
		{
			if (node_sn == TEI_CCO)
			{
				info.meter_forme = 1;
			}
			else
			{
				info.meter_forme = 0;
			}
		}
#if 0		
		if (pSta->phy_phase&0x3c)
		{
			pSta->phy_phase|=1<<7;
		}
#endif		
		if (pSta->phy_phase&(0x1<<7))
		{
			info.phase_sq=0x7;
			info.meter_type=1;
		}
		phy_phase = pSta->phy_phase;
		if (pSta->phase_edge_ask || pSta->V2_7_get_edge)
		{
			if (pSta->phase_edge != pSta->phase_edge_vaile
				&& (phy_phase&0x3f))
			{
				phy_phase|=(1<<6);//陕西零火反接
			}
		}
#if !defined(PHASE_REMAIN)		
		else if (!(phy_phase&(1<<7)))
		{
			phy_phase&=~(0x3f);
		}
#endif

		if ((phy_phase & 0x3f) == 0)
		{
			return info.info;
		}
		if (phy_phase&(1<<7))
		{
			if (phy_phase&0x3f)
			{
				info.phase_sq=0x7;
				for (int i = 0; i < 3; i++)
				{
					u8 phase = (phy_phase >> (i * 2)) & 0x3;
					switch (i)
					{
					case 0:
						info.phase_info_a = phase;
						break;
					case 1:
						info.phase_info_b = phase;
						break;
					case 2:
						info.phase_info_c = phase;
						break;
					}
				}
			}
		}
		else
		{
			info.phase_sq=1<<((phy_phase&0x3)-1);
		}
		
		
		if (phy_phase&(1<<6))
		{
			info.line_error = 1;
		}
		if (phy_phase & (1 << 7))
		{
			info.line_error = 0;
		}
		
		
	}
	if (phy_phase&(1<<7))
	{
		info.line_error=0;
	}
	return info.info;
}
#endif

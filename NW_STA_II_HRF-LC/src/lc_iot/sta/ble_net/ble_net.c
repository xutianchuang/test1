#include "ble_net.h"
#include "ble_local.h"
#include "ble_data.h"
#include "ble_mac.h"
#include "lc_all.h"
#include "ble_apl.h"
#include "crclib.h"
#include "lamp_date.h"



//===
ble_net_debug_cnt_list_type ble_net_debug_cnt_list;
//===
TimeValue ble_net_perio_timer;
TimeValue get_ble_net_info_timer;

#define BLE_NET_PERIO_TIMER_MS		((60*1000UL)*2)	//1分钟
#define GET_BLE_NET_INFO_TIMER_MS		((60*1000UL)*5)	//5分钟

//==================
//时间同步
TimeValue ble_net_sync_timer;
#define BLE_NET_SYNC_TIMER_SOLT_MS	((60*1000UL)*6)
//
static void send_sync_time_stamp_net_frame()
{
	const lamp_date_param_t* p_date = get_lamp_date_base_param();
	if(p_date->date_check == LAMP_DATE_HAVE_SYNC_FLG)
	{
		uint16_t mac_len = BLE_MAC_FRAME_HEAD_LEN + BLE_MAC_CMD_HEAD_LEN +  sizeof(ble_mac_sync_time_stamp_cmd_type);
		uint8_t *ptr = ble_local_malloc(BLE_LOCAL_SEND_FRAME_HEAD_LEN + mac_len + 16);
		if(ptr != NULL)
		{
			uint16_t frame_len = ble_mac_create_sync_time_stamp_cmd_frame(ptr,p_date->date_second_stamp_cnt);
			if(frame_len > 0)
			{
				if(true == local_send_fram_add_task(ptr,frame_len,SEND_LOCAL_ACK_PRIO_VALUE,NULL,0,NULL,NULL))
				{
				    ble_net_debug_cnt_list.ble_sync_time_cnt++;
                    //BLE_DEBUG_PRINT("sync time send!");
				}
			}
			ble_local_free(ptr);
		}
	}
}
//
void ble_net_sync_timer_init()
{
	StartTimer(&ble_net_sync_timer,(ble_net_debug_cnt_list.ble_sync_time_cnt < 5) ? (10 * 1000) : BLE_NET_SYNC_TIMER_SOLT_MS);
}

void ble_net_sync_timer_task()
{
	if(TRUE == LcTimeOut(&ble_net_sync_timer))
	{
		if((ble_net_info.NetEnableFlag == 1)
			&&(ble_net_info.NetModeFlg  == 0))
		{
			send_sync_time_stamp_net_frame();
		}
		StartTimer(&ble_net_sync_timer,(ble_net_debug_cnt_list.ble_sync_time_cnt < 5) ? (10 * 1000) : BLE_NET_SYNC_TIMER_SOLT_MS);
	}
}

//===============
//离网指示任务
static uint16_t ble_check_leave_net_read_idx = 0;

static void send_leave_net_frame()
{
	uint16_t mac_len = BLE_MAC_FRAME_HEAD_LEN + BLE_MAC_CMD_HEAD_LEN +  sizeof(ble_mac_leave_inc_cmd_type) + BLE_LONG_ADDR_SIZE*32;
	uint8_t *ptr = ble_local_malloc(BLE_LOCAL_SEND_FRAME_HEAD_LEN + mac_len + 16);
	if(ptr != NULL)
	{
		uint16_t frame_len = ble_mac_create_leave_inc_cmd_frame(ptr,&ble_check_leave_net_read_idx);
		if(frame_len > 0)
		{
			if(true == ble_local_send_data(ptr,frame_len))
			{
                ble_net_debug_cnt_list.ble_leave_net_cnt++;
			}
		}
		ble_local_free(ptr);
	}
}



static void ble_check_leave_net_process()
{
	if(ble_get_leave_net_tei_process(NULL,1,&ble_check_leave_net_read_idx) != 0)
	{//需要发送
		
		send_leave_net_frame();
	}
}
//========================
TimeValue NetBindTimer;

uint8_t StartNetBindTimer(uint32_t ms)
{
	if(ble_system_params.NetEnableFlag != 0)
	{
		if(ms < 2000)ms=2000;
		if(ms > (36*3600UL*1000)) ms = (36*3600UL*1000);
		
		StartTimer(&NetBindTimer,ms);
		ble_net_info.NetBindFlg = 1;
		//
		ble_net_debug_cnt_list.ble_bind_net_cnt++;
		
		return 1;
	}
	else
	{
		return 0;
	}
}

uint8_t StopNetBindTimer()
{
	if(ble_system_params.NetEnableFlag != 0)
	{
		TimeStop(&NetBindTimer);
		ble_net_info.NetBindFlg = 0;
		return 1;
	}
	else
	{
		return 0;
	}
}

void QueryNetBindInfo(bind_code_query_info_type *p_info)
{
	p_info->leave_time_ms = TimeLeft(&NetBindTimer);
	p_info->bind_statue = ble_net_info.NetBindFlg;
}

void lc_net_set_NetBindFlg(uint8_t bind_flg)
{
	bind_flg = (bind_flg != 0) ? 1 : 0;
	
	if(ble_net_info.NetBindFlg != bind_flg)
	{
		if(bind_flg == 1)
		{
			StartNetBindTimer(600 * 1000);
		}
		else
		{
			StopNetBindTimer();
		}
		
	}
}

uint8_t lc_net_get_NetBindFlg()
{
	return ble_net_info.NetBindFlg;
}


//====
#define SYNC_ISP_PROJ_TIMER_MS		(11000)
TimeValue SyncIspProjTimer;
uint32_t MacAddrIdxCnt = 0;
//uint32_t MacCfgIspProjKeyRunCnt = 0;

//MAC层初始化
void ble_net_bind_task_init(void)
{
	StartTimer(&SyncIspProjTimer,SYNC_ISP_PROJ_TIMER_MS);
}
//
void ble_net_bind_task(void)
{
	if(LcTimeOut(&SyncIspProjTimer))
	{
		uint16_t level = 1;
		const ble_system_params_type *psparams =  &ble_system_params;
		//u16 macaddrnormalsum = psparams->MacAddrNormalSum;
		u16 macaddrsum = psparams->MacAddrSum;
		//MacCfgIspProjKeyRunCnt++;
		//if(MacCfgIspProjKeyRunCnt > 20)
		{
			//MacCfgIspProjKeyRunCnt = 0;
			
			for(uint32_t add_idx=0;add_idx<macaddrsum;add_idx++)
			{
				if(MacAddrIdxCnt>=macaddrsum)
				{
					MacAddrIdxCnt = 0;
				}
				if(psparams->MacAddrList[MacAddrIdxCnt].NeightIndex != BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE)
				{
					uint16_t dst_tei = psparams->MacAddrList[MacAddrIdxCnt].NeightIndex;
					ble_neighbor_info_st_type *pnbinfo = NULL;
					if((dst_tei>=BLE_MIN_TEI_VALUE)&&(dst_tei<=BLE_MAX_TEI_VALUE))
					{
						if(ble_neighbor_list[dst_tei].NeightState == NEIGHT_HAVE_FLG_VALUE)
						{
							pnbinfo =  (&ble_neighbor_list[dst_tei]);
						}
					}
					if(pnbinfo != NULL)
					{
						if((pnbinfo->pro_check_flg == 0)
							&&(pnbinfo->BindProjKeyCnt) < 10)
						{//需要配置
							(pnbinfo->BindProjKeyCnt) += 1;
							level = pnbinfo->Layer;
							ble_mac_send_cfg_isp_proj_key_cmd_frame((pnbinfo->MacAddr),ble_net_info.IspKey, ble_net_info.ProjKey);
							//ble_net_send_net_cfg_isp_proj_key_frame(dst_tei);
							break;
						}
					}
				}
				MacAddrIdxCnt++;
			}
		}
		level = level<1?1:level;
		level = level>BLE_MAX_LAYER_VALUE?BLE_MAX_LAYER_VALUE:level;
		StartTimer(&SyncIspProjTimer, (SYNC_ISP_PROJ_TIMER_MS*level));
	}
	//
	if(ble_net_info.NetBindFlg == 1)
	{
		if(LcTimeOut(&NetBindTimer))
		{
			ble_net_info.NetBindFlg = 0;
		}
	}
}

void LcSetFactoryMsg(void)
{
	uint32_t IspKey = lc_net_get_factory_id_code();
	uint32_t ProjKey = lc_net_get_project_id_code();
	// uint32_t ProjKey = 178;
	uint8_t frame[18] = {0};
	
	frame[2] = 'L';
	frame[3] = 'C';

    frame[4] = (SELF_VER_RELEASE_DATE >>  8) & 0xff;
    frame[5] = (SELF_VER_RELEASE_DATE >>  16) & 0xff;
    frame[6] = (SELF_VER_RELEASE_DATE >>  24) & 0xff;
    
    frame[7] = (SELF_VER_VALUE >>  0) & 0xff;
    frame[8] = (SELF_VER_VALUE >>  8) & 0xff;

	memcpy(&(frame[9]), (uint8_t *)&IspKey, 4);
	memcpy(&(frame[13]), (uint8_t *)&ProjKey, 4);

	uint16_t crc16 = crc16_cal(&(frame[2]), 18-2);
    frame[0] = crc16&0xFF;
    frame[1] = (crc16>>8)&0xFF;

	// uint8_t frame[18] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,};
	
	SetFactoryMsg(frame, 18);
}

void ble_net_init()
{
	memset(&ble_net_debug_cnt_list,0,sizeof(ble_net_debug_cnt_list));
	ble_net_bind_task_init();
	#ifdef REGIST_NET_REPORT_USING
	ble_mac_init_addr_report();
	#endif
	#ifdef BLE_NODE_IN_LEAVE_NET_REPORT_USING
	ble_mac_addr_in_leave_net_report_init_task();
	#endif
	ble_system_info_reset_init();
	ble_net_sync_timer_init();
	StartTimer(&ble_net_perio_timer,BLE_NET_PERIO_TIMER_MS);
	StartTimer(&get_ble_net_info_timer,GET_BLE_NET_INFO_TIMER_MS);

	LcSetFactoryMsg();
}	

void ble_net_sync_key_task()
{
	OS_ERR err;
	static uint32_t last_tick = 0;
	if ((OSTimeGet(&err) - last_tick) > 1000)
	{
		last_tick = OSTimeGet(&err);
		if(ble_net_info.IspKey != lc_net_get_factory_id_code())
		{
			ble_net_info.IspKey = lc_net_get_factory_id_code();
		}
		if(ble_net_info.ProjKey != lc_net_get_project_id_code() && ble_system_params.key_switch == 0)
		{
			ble_net_info.ProjKey = lc_net_get_project_id_code();
			LcSetFactoryMsg();
		}
	}
}
extern void ble_sta_net_check_param_send();

void ble_net_process()
{
	ble_system_info_syn_process();
	ble_local_run_process();
	//每个5分钟读取蓝牙信息
	if(LcTimeOut(&get_ble_net_info_timer))
	{
		ble_sta_net_check_param_send();
		StartTimer(&get_ble_net_info_timer,GET_BLE_NET_INFO_TIMER_MS);
	}
	
	#ifdef REGIST_NET_REPORT_USING
	ble_mac_addr_report_process_task();
	#endif
	#ifdef BLE_NODE_IN_LEAVE_NET_REPORT_USING
	ble_mac_addr_in_leave_net_report_task();
	#endif
	ble_net_bind_task();
	ble_net_sync_timer_task();
	ble_net_sync_key_task();
	if(LcTimeOut(&ble_net_perio_timer))
	{
		ble_neighbor_list_period_process();
		ble_check_leave_net_process();
		StartTimer(&ble_net_perio_timer,BLE_NET_PERIO_TIMER_MS);
	}
}



















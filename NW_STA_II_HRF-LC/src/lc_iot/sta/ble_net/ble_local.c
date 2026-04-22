#include <string.h>
#include "common_includes.h"
#include "ble_local.h"
#include "ble_mac.h"
#include "ble_net.h"
#include "os.h"
#include "hplc_data.h"
#include "lc_all.h"
#include "lamp_date.h"

//================
//#define BLE_LAMP_MODE
enum
{
	PLC_BLE_LAMP_POWER_INIT_START,
	PLC_LAMP_INIT_START,
	PLC_LAMP_BLE_LOCAL_INIT_START,
	PLC_LAMP_BLE_LOCAL_INIT_WAITE,
	PLC_LAMP_BLE_LOCAL_INIT_WAITE_ACK,
	PLC_LAMP_BLE_LOCAL_INIT_ERROR,
	PLC_LAMP_BLE_LOCAL_CHECK_STATE,
	PLC_LAMP_BLE_LOCAL_RUN_STATE,
#ifdef BLE_LAMP_MODE
	BLE_LAMP_INIT_START,
	BLE_LAMP_BLE_LOCAL_INIT_START,
	BLE_LAMP_BLE_LOCAL_INIT_WAITE,
	BLE_LAMP_BLE_LOCAL_INIT_WAITE_ACK,
	BLE_LAMP_BLE_LOCAL_INIT_ERROR,
	BLE_LAMP_BLE_LOCAL_CHECK_STATE,
	BLE_LAMP_BLE_LOCAL_RUN_STATE,
#endif
	PLC_BLE_LAMP_INIT_START,
	PLC_BLE_LAMP_BLE_LOCAL_INIT_START,
	PLC_BLE_LAMP_BLE_LOCAL_INIT_WAITE,
	PLC_BLE_LAMP_BLE_LOCAL_INIT_WAITE_ACK,
	PLC_BLE_LAMP_BLE_LOCAL_INIT_ERROR,
	PLC_BLE_LAMP_BLE_LOCAL_CHECK_STATE,
	PLC_BLE_LAMP_BLE_LOCAL_RUN_STATE,
}ble_local_run_state = PLC_BLE_LAMP_POWER_INIT_START;

#define BLE_LOCAL_WAIT_TIME_MS				(20*1000UL)
#define BLE_LOCAL_ERROR_RESET_TIME_MS		(60*1000UL)
#define BLE_LOCAL_CHECK_NET_TIME_MS			(30*60*1000UL)

//0:idle,1:waite cfg ack;2:run
TimeValue ble_local_run_timer;
uint16_t ble_local_sn = 0;
volatile uint32_t ble_local_run_state_error_cnt = 0;
//================
uint16_t ble_local_process_receive_data_frame(uint8_t *p_frame,uint16_t frame_len)
{
	uint16_t r_len = 0;
	//if()
	{
		ble_local_frame_type *p_local_fram = (ble_local_frame_type *)p_frame;
		ble_local_receive_data_frame_type *p_local_receive_frame = (ble_local_receive_data_frame_type*)p_local_fram->frame_data;
		//
		ble_net_set_neighbor(p_local_receive_frame->src_mac,p_local_receive_frame->rssi);
		ble_local_set_neighbor_info(p_local_receive_frame->src_mac,p_local_receive_frame->rssi);
		//
		uint16_t tei = ble_get_mac_to_tei(p_local_receive_frame->src_mac);
		if(tei != 0)
		{
		//	ble_brush_neighbor_heart(tei,0xFF);
		}
		//
		r_len = ble_mac_process_frame(p_local_receive_frame->frame_data,p_local_receive_frame->data_len);
		//uint16_t ble_mac_process_frame(uint8_t *mac_frame,uint16_t frame_len);
	}
	return r_len;
}

uint16_t ble_local_process_receive_neighbor_frame(uint8_t *p_frame,uint16_t frame_len)
{
	uint16_t r_len = 0;
	//
	ble_local_frame_type *p_local_fram = (ble_local_frame_type *)p_frame;
	ble_local_receive_neighbor_frame_type *p_neighbor_receive_frame = (ble_local_receive_neighbor_frame_type*)p_local_fram->frame_data;
	//
	ble_net_set_neighbor(p_neighbor_receive_frame->neighbor_mac,p_neighbor_receive_frame->rssi);
	ble_local_set_neighbor_info(p_neighbor_receive_frame->neighbor_mac,p_neighbor_receive_frame->rssi);
	//
	if((ble_net_info.NetEnableFlag == 1)
		&&(ble_net_info.NetModeFlg == 0)
	 &&(p_neighbor_receive_frame->net_id == ble_net_info.NetId))
	{//cco 模式，网络ID相同；
		uint16_t tei = ble_get_mac_to_tei(p_neighbor_receive_frame->neighbor_mac);
		if(tei != 0)
		{
			ble_brush_neighbor_heart(tei,1);
		}
	}
	//
	return r_len;
}

uint16_t ble_local_process_send_data_ack_frame(uint8_t *p_frame,uint16_t frame_len)
{
	uint16_t r_len = 0;
	return r_len;
}

// uint16_t ble_local_process_send_data_frame(uint8_t *p_frame,uint16_t frame_len)
// {
// 	uint16_t r_len = 0;
// 	return r_len;
// }

// uint16_t ble_local_process_link_send_data_frame(uint8_t *p_frame,uint16_t frame_len)
// {
// 	uint16_t r_len = 0;
// 	return r_len;
// }

// uint16_t ble_local_process_config_net_info_frame(uint8_t *p_frame,uint16_t frame_len)
// {
// 	uint16_t r_len = 0;
// 	return r_len;
// }

// uint16_t ble_local_process_file_updata_frame(uint8_t *p_frame,uint16_t frame_len)
// {
// 	uint16_t r_len = 0;
// 	return r_len;
// }
//=====================
uint8_t * ble_local_malloc(uint32_t size)
{
	uint8_t* a_buff = malloc(size);
	if(a_buff != NULL)
	{
		ble_net_debug_cnt_list.ble_local_malloc_cnt++;
	}
	return a_buff;
}
void ble_local_free(void *ptr)
{
	ble_net_debug_cnt_list.ble_local_malloc_cnt--;
	free(ptr);
}


//=================
uint8_t ble_net_ack_send_data_port = BLE_NET_ACK_SEND_PLC_PORT;

TimeValue ble_net_ack_send_data_port_timer;

bool ble_local_send_data(uint8_t *ptr,uint16_t data_len)
{
	bool r_flg = local_send_fram_add_task(ptr,data_len,SEND_LOCAL_CFG_FRAME_PRIO_VALUE,NULL,0,NULL,NULL);
	if(r_flg == true)
	{
		ble_net_debug_cnt_list.ble_local_send_add_task_success_cnt++;
	}
	else
	{
		ble_net_debug_cnt_list.ble_local_send_add_task_fail_cnt++;
	}
	return r_flg;
}

//==
bool ble_net_ack_send_data(uint8_t *ptr,uint16_t data_len)
{	
	if(ble_net_ack_send_data_port == BLE_NET_ACK_SEND_PLC_PORT)
	{
		lc_net_my_info_t p_info;
		lc_net_get_net_self_info(&p_info);
		if(p_info.net_state == LC_NET_IN_STATE)
		{
			ble_net_debug_cnt_list.ble_to_plc_tx_cnt++;
			lc_net_hplc_tx_sta_frame(p_info.cco_addr,ptr,data_len);
			return true;
		}
		return false;
	}
	else if(ble_net_ack_send_data_port == BLE_NET_ACK_SEND_BLE_UART_PORT)
	{
		ble_net_debug_cnt_list.ble_to_ble_tx_cnt++;
		return local_send_fram_add_task(ptr,data_len,SEND_LOCAL_CFG_FRAME_PRIO_VALUE,NULL,0,NULL,NULL);
	}
	else
	{
		ble_net_debug_cnt_list.ble_to_debug_tx_cnt++;
		//local_debug_uart_tx_data(ptr,data_len);  //$$$先关闭
		return true;
	}
}

void ble_net_ack_send_data_port_check()
{	
	if (LcTimeOut(&ble_net_ack_send_data_port_timer))
	{
		lc_net_my_info_t p_info;
		lc_net_get_net_self_info(&p_info);
		if(p_info.net_state == LC_NET_IN_STATE)
		{
			if(ble_net_ack_send_data_port != BLE_NET_ACK_SEND_PLC_PORT)
			{
				ble_net_ack_send_data_port = BLE_NET_ACK_SEND_PLC_PORT;
				ble_net_debug_cnt_list.ble_net_ack_send_data_port_ble_2_plc++;
			}
		}
		else
		{
			if(ble_net_ack_send_data_port != BLE_NET_ACK_SEND_BLE_UART_PORT)
			{
				ble_net_ack_send_data_port = BLE_NET_ACK_SEND_BLE_UART_PORT;
				ble_net_debug_cnt_list.ble_net_ack_send_data_port_plc_2_ble++;
			}
		}
		StartTimer(&ble_net_ack_send_data_port_timer, ACK_SEND_PORT_CHECK_TIME); 
	}
}
//=========
uint16_t ble_create_local_frame(uint8_t *ptr,uint8_t cmd,uint8_t data_len,uint8_t *frame_data,uint16_t f_seq)
{
	ble_local_frame_type *l_ptr = (ble_local_frame_type *)(&ptr[WHI_PRO_STAT_HEAD_SIZE]);
	l_ptr->cmd = cmd;
	l_ptr->data_len = data_len;
	if((frame_data != NULL)&&(l_ptr->frame_data != frame_data))
	{
		memmove(l_ptr->frame_data,frame_data,data_len);
	}
	//
	uint16_t frame_len = WHI_BuildFrameProcess(ptr,false,WHI_LAMP_BLE_NET_CTRL_FRAME,f_seq,&ptr[WHI_PRO_STAT_HEAD_SIZE],data_len + (sizeof(ble_local_frame_type)));
	//WHI_CreateLocalSeq()
	//
	return frame_len;
}

//===
uint16_t ble_create_local_send_frame(uint8_t *ptr,uint8_t send_type,uint8_t *dst_addr,uint8_t data_len,uint8_t *frame_data)
{
	ble_local_send_data_frame_type * p_send_data = (ble_local_send_data_frame_type *)(&ptr[BLE_LOCAL_DATA_OFF]);
	p_send_data->send_type = send_type;
	if(dst_addr == NULL)
	{
		// memset(p_send_data->addr.mac,0xFF,BLE_LONG_ADDR_SIZE);
		p_send_data->addr.mac[0] = 0xFF;
		p_send_data->addr.mac[1] = 0xFF;
		p_send_data->addr.mac[2] = 0xFF;
		p_send_data->addr.mac[3] = 0xFF;
		p_send_data->addr.mac[4] = 0xFF;
		p_send_data->addr.mac[5] = 0xFF;
	}
	else
	{
		// memcpy(p_send_data->addr.mac,dst_addr,BLE_LONG_ADDR_SIZE);
		p_send_data->addr.mac[0] = dst_addr[0];
		p_send_data->addr.mac[1] = dst_addr[1];
		p_send_data->addr.mac[2] = dst_addr[2];
		p_send_data->addr.mac[3] = dst_addr[3];
		p_send_data->addr.mac[4] = dst_addr[4];
		p_send_data->addr.mac[5] = dst_addr[5];
	}
	p_send_data->len = data_len;
	if(p_send_data->frame_data != frame_data)
	{
		memmove(p_send_data->frame_data,frame_data,data_len);
	}
	return ble_create_local_frame(ptr,BLE_LOCAL_SEND_DATA_CMD,data_len+sizeof(ble_local_send_data_frame_type),&ptr[BLE_LOCAL_DATA_OFF],WHI_CreateLocalSeq());
}


#define LOCAL_LINK_SEND_LAYER_TIME_SECOND	(10)
uint16_t ble_create_local_link_send_frame(uint8_t *ptr,uint8_t *dst_addr,uint8_t layer,uint8_t data_len,uint8_t *frame_data)
{
	ble_local_link_send_data_frame_type * p_send_link_data = (ble_local_link_send_data_frame_type *)(&ptr[BLE_LOCAL_DATA_OFF]);
	p_send_link_data->link_time = LOCAL_LINK_SEND_LAYER_TIME_SECOND*layer;
	p_send_link_data->link_flg = 1;//保持
	if(dst_addr == NULL)
	{
		memset(p_send_link_data->dst_mac,0xFF,BLE_LONG_ADDR_SIZE);
	}
	else
	{
		memcpy(p_send_link_data->dst_mac,dst_addr,BLE_LONG_ADDR_SIZE);
	}
	p_send_link_data->len = data_len;
	if(p_send_link_data->frame_data != frame_data)
	{
		memmove(p_send_link_data->frame_data,frame_data,data_len);
	}
	return ble_create_local_frame(ptr,BLE_LOCAL_LINK_SEND_DATA_CMD,data_len+sizeof(ble_local_link_send_data_frame_type),&ptr[BLE_LOCAL_DATA_OFF],WHI_CreateLocalSeq());
}
//============
uint16_t ble_create_local_config_net_info_send_frame(uint16_t frame_seq,uint8_t *ptr)
{
	ble_local_config_net_info_frame_type * p_send_config_data = (ble_local_config_net_info_frame_type *)(&ptr[BLE_LOCAL_DATA_OFF]);
	p_send_config_data->net_id = ble_net_info.NetId;
	p_send_config_data->net_enable_flg = ble_net_info.NetEnableFlag;
	p_send_config_data->net_seq = ble_net_info.NetSeq;
	p_send_config_data->net_mode = ble_net_info.NetModeFlg;
	p_send_config_data->isp_key = ble_net_info.IspKey;
	p_send_config_data->proj_key = ble_net_info.ProjKey;
	p_send_config_data->date_tamp = get_lamp_date_base_param()->date_second_stamp_cnt;
	memcpy(p_send_config_data->mac_addr,ble_net_info.CCO_MAC,BLE_LONG_ADDR_SIZE);
	
	return ble_create_local_frame(ptr,BLE_LOCAL_CONFIG_NET_INFO_CMD,sizeof(ble_local_config_net_info_frame_type),&ptr[BLE_LOCAL_DATA_OFF],frame_seq);
}
//==================================
#define BLE_PLC_RUN_CHECK_SOLT_MS	(30*1000UL)
typedef struct
{
	uint16_t ble_sta_net_check_seq;
	uint16_t ble_sta_net_check_statue;
}ble_sta_net_check_param_st_type;
ble_sta_net_check_param_st_type ble_sta_net_check_param_st;
TimeValue ble_plc_lamp_run_check_net_timer;
uint16_t ble_plc_lamp_run_check_net_fail_cnt = 0;

void ble_sta_net_check_param_send()
{
	memset(&ble_sta_net_check_param_st,0,sizeof(ble_sta_net_check_param_st));
	ble_sta_net_check_param_st.ble_sta_net_check_seq = WHI_CreateLocalSeq();
	ble_sta_net_check_param_st.ble_sta_net_check_statue = 0;
	uint8_t ptr[BLE_LOCAL_FRAME_HEAD_LEN + 16];
	uint16_t frame_len = ble_create_local_frame(ptr,BLE_LOCAL_READ_NET_INFO_CMD,0,NULL,ble_sta_net_check_param_st.ble_sta_net_check_seq);
	whi_pro_local_add_uart_task(ptr,frame_len,SEND_LOCAL_CFG_FRAME_PRIO_VALUE,1);
}


uint16_t ble_local_process_ack_frame(uint8_t *p_frame,uint16_t frame_len,uint16_t ack_seq)
{
	uint16_t r_flg = 0;
	if(frame_len <= 4)
	{//确认帧
		if(ble_local_sn == ack_seq)
		{
			switch(ble_local_run_state)
			{
				case PLC_LAMP_BLE_LOCAL_INIT_WAITE:
					ble_local_run_state = PLC_LAMP_BLE_LOCAL_INIT_WAITE_ACK;
					r_flg = 1;
				break;
				#ifdef BLE_LAMP_MODE
				case BLE_LAMP_BLE_LOCAL_INIT_WAITE:
					ble_local_run_state = BLE_LAMP_BLE_LOCAL_INIT_WAITE_ACK;
					r_flg = 1;
				break;
				#endif
				case PLC_BLE_LAMP_BLE_LOCAL_INIT_WAITE:
					ble_local_run_state = PLC_BLE_LAMP_BLE_LOCAL_INIT_WAITE_ACK;
					r_flg = 1;
				break;
				default:
				break;
			}
		}
		return r_flg;
	}
	
	uint16_t cmd = ((ble_local_frame_type*)p_frame)->cmd;
	switch(cmd)
	{
		case BLE_LOCAL_READ_NET_INFO_CMD:
		{
			if((frame_len >= (sizeof(ble_local_frame_type)+sizeof(ble_local_config_net_info_frame_type)))
				&&((((ble_local_frame_type*)p_frame)->data_len) >= sizeof(ble_local_config_net_info_frame_type)))
			{
				if(ble_sta_net_check_param_st.ble_sta_net_check_seq == ack_seq)
				{
					ble_local_config_net_info_frame_type * p_send_config_data = (ble_local_config_net_info_frame_type*)(((ble_local_frame_type*)p_frame)->frame_data);
					if(((p_send_config_data->net_enable_flg) != 0)
						&&((p_send_config_data->net_mode) == 1)
					&& (p_send_config_data->isp_key == ble_net_info.IspKey)
					)
					{
						if(((p_send_config_data->net_id) != 0x0000)&&((p_send_config_data->net_id) != 0xFFFF))
						{//入网
							ble_sta_net_check_param_st.ble_sta_net_check_statue = 1;
							ble_sta_net_info.net_statu = 1;
							if(p_send_config_data->proj_key != lc_net_get_project_id_code())
							{//同步项目key
								lc_net_write_project_id_code(p_send_config_data->proj_key);
								ble_net_info.ProjKey = p_send_config_data->proj_key;
								LcSetFactoryMsg();
							}
						}
						else
						{
							ble_sta_net_info.net_statu = 0;
						}
						ble_sta_net_info.net_id = p_send_config_data->net_id;
						ble_sta_net_info.net_seq = p_send_config_data->net_seq;
						ble_sta_net_info.net_enable_flg = p_send_config_data->net_enable_flg;
						ble_sta_net_info.net_mode = p_send_config_data->net_mode;
						memcpy(ble_sta_net_info.cco_addr,p_send_config_data->mac_addr,BLE_LONG_ADDR_SIZE);
						memcpy(ble_sta_net_info.pco_addr,p_send_config_data->mac_addr,BLE_LONG_ADDR_SIZE);
						//
						ble_sta_net_info.isp_key = p_send_config_data->isp_key;
						ble_sta_net_info.proj_key = p_send_config_data->proj_key;
						//=============
						//sync date
						lc_net_my_info_t plc_net_info;
						lc_net_get_net_self_info(&plc_net_info);
						if(plc_net_info.net_state != LC_NET_IN_STATE)
						{//入网
							if(ble_sta_net_info.net_statu == 1)
							{
								lc_iot_calibrate_date_cb_fun(p_send_config_data->date_tamp);  //电表校时 
							}
						}
					}
				//ble_sta_net_check_param_st
				}
				r_flg = 1;
			}
			break;
		}
		default:
			break;
	}
	return r_flg;
}

uint16_t ble_local_process_frame(uint8_t *p_frame,uint16_t frame_len)
{
	if(frame_len < BLE_LOCAL_FRAME_START_HEAD_LEN)return 0;
	
	ble_net_debug_cnt_list.ble_local_receive_success_cnt++;
	
	uint16_t r_len = 0;
	uint16_t cmd = ((ble_local_frame_type*)p_frame)->cmd;
	switch(cmd)
	{
		case BLE_LOCAL_CONFIG_NET_INFO_CMD:
		case BLE_LOCAL_READ_NET_INFO_CMD:
			{
				ble_local_config_net_info_frame_type * p_send_config_data = (ble_local_config_net_info_frame_type*)(((ble_local_frame_type*)p_frame)->frame_data);
				
				p_send_config_data->net_enable_flg = ble_net_info.NetEnableFlag;
				p_send_config_data->net_id = ble_net_info.NetId;
				p_send_config_data->net_seq = ble_net_info.NetSeq;
				p_send_config_data->net_mode = ble_net_info.NetModeFlg;
				p_send_config_data->isp_key = ble_net_info.IspKey;
				p_send_config_data->proj_key = ble_net_info.ProjKey;
				p_send_config_data->date_tamp = get_lamp_date_base_param()->date_second_stamp_cnt;
				memcpy(p_send_config_data->mac_addr,ble_net_info.CCO_MAC,BLE_LONG_ADDR_SIZE);
				
				((ble_local_frame_type*)p_frame)->data_len = sizeof(ble_local_config_net_info_frame_type);
				r_len = sizeof(ble_local_config_net_info_frame_type) + sizeof(ble_local_frame_type);
			}
			break;
		case BLE_LOCAL_RECEIVE_DATA_CMD:
			if(ble_local_run_state == PLC_BLE_LAMP_BLE_LOCAL_RUN_STATE)
			{
				r_len = ble_local_process_receive_data_frame(p_frame,frame_len);
			}
			break;
		case BLE_LOCAL_NEIGHBOR_DATA_CMD:
			r_len = ble_local_process_receive_neighbor_frame(p_frame,frame_len);
			break;
		case BLE_LOCAL_SEND_SUCCESS_ACK_CMD:
			r_len = ble_local_process_send_data_ack_frame(p_frame,frame_len);
			break;
		default:
			break;
	}
	return r_len;
}

uint16_t ble_local_process_frame_ex(uint8_t *p_frame,uint16_t frame_len, bool *p_need_ack)
{
	*p_need_ack = true;
	if(frame_len < BLE_LOCAL_FRAME_START_HEAD_LEN)return 0;
	
	ble_net_debug_cnt_list.ble_local_receive_success_cnt++;
	
	uint16_t r_len = 0;
	uint16_t cmd = ((ble_local_frame_type*)p_frame)->cmd;
	switch(cmd)
	{
		case BLE_LOCAL_CONFIG_NET_INFO_CMD:
		case BLE_LOCAL_READ_NET_INFO_CMD:
			{
				ble_local_config_net_info_frame_type * p_send_config_data = (ble_local_config_net_info_frame_type*)(((ble_local_frame_type*)p_frame)->frame_data);
				
				p_send_config_data->net_enable_flg = ble_net_info.NetEnableFlag;
				p_send_config_data->net_id = ble_net_info.NetId;
				p_send_config_data->net_seq = ble_net_info.NetSeq;
				p_send_config_data->net_mode = ble_net_info.NetModeFlg;
				p_send_config_data->isp_key = ble_net_info.IspKey;
				p_send_config_data->proj_key = ble_net_info.ProjKey;
				p_send_config_data->date_tamp = get_lamp_date_base_param()->date_second_stamp_cnt;
				memcpy(p_send_config_data->mac_addr,ble_net_info.CCO_MAC,BLE_LONG_ADDR_SIZE);
				
				((ble_local_frame_type*)p_frame)->data_len = sizeof(ble_local_config_net_info_frame_type);
				r_len = sizeof(ble_local_config_net_info_frame_type) + sizeof(ble_local_frame_type);
			}
			break;
		case BLE_LOCAL_RECEIVE_DATA_CMD:
			if(ble_local_run_state == PLC_BLE_LAMP_BLE_LOCAL_RUN_STATE)
			{
				r_len = ble_local_process_receive_data_frame(p_frame,frame_len);
			}
			break;
		case BLE_LOCAL_NEIGHBOR_DATA_CMD:
			*p_need_ack = false;
			r_len = ble_local_process_receive_neighbor_frame(p_frame,frame_len);
			break;
		case BLE_LOCAL_SEND_SUCCESS_ACK_CMD:
			r_len = ble_local_process_send_data_ack_frame(p_frame,frame_len);
			break;
		default:
			break;
	}
	return r_len;
}

//==================================
void ble_local_restart_net()
{
	ble_local_run_state = PLC_BLE_LAMP_POWER_INIT_START;
}

void BleRestartNet(void)
{
    ble_data_clear_addrs();
	ble_local_restart_net();//重新组网

}
void ble_local_run_init_process()
{
	ble_local_sn = WHI_CreateLocalSeq();
	uint8_t ptr[BLE_LOCAL_FRAME_HEAD_LEN + sizeof(ble_local_config_net_info_frame_type) +16];
	uint16_t frame_len = ble_create_local_config_net_info_send_frame(ble_local_sn,ptr);
	//
	whi_pro_local_add_uart_task(ptr,frame_len,SEND_LOCAL_CFG_FRAME_PRIO_VALUE,1);
	//ble_local_send_data(ptr,frame_len);
	
}

void ble_local_run_process()
{	
	ble_local_neighbor_list_task();
	ble_net_ack_send_data_port_check();
	switch(ble_local_run_state)
	{
		case PLC_BLE_LAMP_POWER_INIT_START:
		{
			#ifdef BLE_LAMP_MODE
			if(ble_system_params.PlcBleMode != 0)
			{
				ble_local_run_state = BLE_LAMP_INIT_START;
			}
			else
			#endif
			{
				ble_local_run_state = PLC_LAMP_INIT_START;
			}
			ble_local_neighbor_list_init();
			break;
		}
		case PLC_LAMP_INIT_START:
		{
			memset(&ble_sta_net_info,0,sizeof(ble_sta_net_info));
			ble_local_run_state = PLC_LAMP_BLE_LOCAL_INIT_START;
			lc_net_start_net();//启动plc入网
			break;
		}	
		case PLC_LAMP_BLE_LOCAL_INIT_START:
		{
			//初始化网络参数
			ble_data_reset_net_data();
			//
			ble_net_info.NetId = 0;//网络ID无
			ble_net_info.NetSeq = 0;//系号无
			#ifdef BLE_LAMP_MODE
			ble_net_info.NetEnableFlag  = 0;//网络关闭
			ble_net_info.NetModeFlg  = 0;//模式tamp
			#else
			if(ble_system_params.ble_sta_mode == 0)
			{
				ble_net_info.NetEnableFlag  = 0;//网络关闭
				ble_net_info.NetModeFlg  = 1;//模式STA
			}
			else
			{
				ble_net_info.NetEnableFlag  = 1;//网络开启
				ble_net_info.NetModeFlg  = 1;//模式STA
			}
			#endif
			ble_local_run_init_process();
			ble_local_run_state = PLC_LAMP_BLE_LOCAL_INIT_WAITE;
			StartTimer(&ble_local_run_timer,BLE_LOCAL_WAIT_TIME_MS);
			break;
		}
		case PLC_LAMP_BLE_LOCAL_INIT_WAITE:
		{
			if(LcTimeOut(&ble_local_run_timer))
			{
				ble_local_run_state_error_cnt++;
				ble_local_run_state = PLC_LAMP_BLE_LOCAL_INIT_ERROR;
				StartTimer(&ble_local_run_timer,BLE_LOCAL_ERROR_RESET_TIME_MS);
			}
			break;
		}
		case PLC_LAMP_BLE_LOCAL_INIT_WAITE_ACK:
		{
			StartTimer(&ble_local_run_timer,BLE_LOCAL_CHECK_NET_TIME_MS);
			ble_local_run_state = PLC_LAMP_BLE_LOCAL_CHECK_STATE;
			break;
		}
		case PLC_LAMP_BLE_LOCAL_INIT_ERROR:
		{
			if(LcTimeOut(&ble_local_run_timer))
			{
				ble_local_run_state = PLC_LAMP_BLE_LOCAL_INIT_START;
			}
			break;
		}
		case PLC_LAMP_BLE_LOCAL_CHECK_STATE:
		{
			//等待plc入网30分钟
			lc_net_my_info_t plc_net_info;
			lc_net_get_net_self_info(&plc_net_info);
			if(plc_net_info.net_state == LC_NET_IN_STATE)
			{//入网				
				#ifdef BLE_LAMP_MODE
				if(ble_system_params.PlcBleMode != 0)
				{
					ble_system_params.PlcBleMode = 0;
					ble_system_params.DirtySystemParamFlg = 1;
					ble_system_info_stored();
				}
				#endif
				//网关开启
				ble_local_run_state = PLC_BLE_LAMP_BLE_LOCAL_INIT_START;

			}
			else
			{
				if(LcTimeOut(&ble_local_run_timer))
				{//超时未入网,停止plc,启动ble入网
					#ifdef BLE_LAMP_MODE
					ble_local_run_state = BLE_LAMP_INIT_START;
					#else
					ble_local_run_state = PLC_LAMP_BLE_LOCAL_RUN_STATE;
					ble_plc_lamp_run_check_net_fail_cnt = 0;
					StartTimer(&ble_plc_lamp_run_check_net_timer,BLE_PLC_RUN_CHECK_SOLT_MS);
					ble_sta_net_check_param_send();
					#endif
				}
			}
			break;
		}
		case PLC_LAMP_BLE_LOCAL_RUN_STATE:
		{//检测长时间不在线，转向BLE
			if(LcTimeOut(&ble_plc_lamp_run_check_net_timer))
			{
				#ifndef BLE_LAMP_MODE
				ble_sta_net_check_param_send();
				#endif
				lc_net_my_info_t plc_net_info;
				lc_net_get_net_self_info(&plc_net_info);
				if(plc_net_info.net_state == LC_NET_IN_STATE)
				{//入网
					ble_plc_lamp_run_check_net_fail_cnt = 0;
					// if(ble_system_params.NetEnableFlag != 0)
					{//网关开启
						ble_local_run_state = PLC_BLE_LAMP_BLE_LOCAL_INIT_START;
					}
				}
				else
				{
					++ble_plc_lamp_run_check_net_fail_cnt;
					if(ble_plc_lamp_run_check_net_fail_cnt > 100)
					{//50分钟不入网
						#ifdef BLE_LAMP_MODE
						ble_local_run_state = PLC_LAMP_INIT_START;
						#else
						ble_plc_lamp_run_check_net_fail_cnt = 0;
						#endif
					}
				}
				StartTimer(&ble_plc_lamp_run_check_net_timer,BLE_PLC_RUN_CHECK_SOLT_MS);
			}
			break;
		}
		//===
		#ifdef BLE_LAMP_MODE
		case BLE_LAMP_INIT_START:
		{
			ble_local_run_state = BLE_LAMP_BLE_LOCAL_INIT_START;
			memset(&ble_sta_net_info,0,sizeof(ble_sta_net_info));
			break;
		}
		case BLE_LAMP_BLE_LOCAL_INIT_START:
		{
			lc_net_stop_net();
			//初始化网络参数
			ble_data_reset_net_data();
			//
			ble_net_info.NetId = 0;//网络ID无
			ble_net_info.NetSeq = 0;//系号无
			ble_net_info.NetEnableFlag  = 1;//网络开启
			ble_net_info.NetModeFlg  = 1;//模式1=STA
			StartTimer(&ble_local_run_timer,BLE_LOCAL_WAIT_TIME_MS);
			ble_local_run_init_process();
			ble_local_run_state = BLE_LAMP_BLE_LOCAL_INIT_WAITE;
			break;
		}
		case BLE_LAMP_BLE_LOCAL_INIT_WAITE:
		{
			if(LcTimeOut(&ble_local_run_timer))
			{
				ble_local_run_state_error_cnt++;
				ble_local_run_state = BLE_LAMP_BLE_LOCAL_INIT_ERROR;
				//
				StartTimer(&ble_local_run_timer,BLE_LOCAL_ERROR_RESET_TIME_MS);
			}
			break;
		}
		case BLE_LAMP_BLE_LOCAL_INIT_WAITE_ACK:
		{
			StartTimer(&ble_local_run_timer,BLE_LOCAL_CHECK_NET_TIME_MS);
			ble_local_run_state = BLE_LAMP_BLE_LOCAL_CHECK_STATE;
			//
			ble_sta_net_check_param_send();
			StartTimer(&ble_plc_lamp_run_check_net_timer,BLE_PLC_RUN_CHECK_SOLT_MS);
			break;
		}
		case BLE_LAMP_BLE_LOCAL_INIT_ERROR:
		{
			if(LcTimeOut(&ble_local_run_timer))
			{
				ble_local_run_state = PLC_LAMP_BLE_LOCAL_INIT_START;
			}
			break;
		}
		case BLE_LAMP_BLE_LOCAL_CHECK_STATE:
		{//检测BLE入网
			if(ble_sta_net_check_param_st.ble_sta_net_check_statue == 1)
			{//入网
				if(ble_system_params.PlcBleMode == 0)
				{
					ble_system_params.PlcBleMode = 1;
					ble_system_params.DirtySystemParamFlg = 1;
					ble_system_info_stored();
				}
				ble_local_run_state = BLE_LAMP_BLE_LOCAL_RUN_STATE;
				ble_plc_lamp_run_check_net_fail_cnt = 0;
				StartTimer(&ble_plc_lamp_run_check_net_timer,BLE_PLC_RUN_CHECK_SOLT_MS);
			}
			else
			{
				if(LcTimeOut(&ble_local_run_timer))
				{//超时未入网,启动PLC入网
					ble_local_run_state = PLC_LAMP_INIT_START;
				}
				if(LcTimeOut(&ble_plc_lamp_run_check_net_timer))
				{
					ble_sta_net_check_param_send();
					StartTimer(&ble_plc_lamp_run_check_net_timer,BLE_PLC_RUN_CHECK_SOLT_MS);
				}
			}
			break;
		}
		case BLE_LAMP_BLE_LOCAL_RUN_STATE:
		{
			if(LcTimeOut(&ble_plc_lamp_run_check_net_timer))
			{
				if(ble_sta_net_check_param_st.ble_sta_net_check_statue == 0)
				{
					++ble_plc_lamp_run_check_net_fail_cnt;
					if(ble_plc_lamp_run_check_net_fail_cnt > 100)
					{//50分钟不入网
						ble_local_run_state = PLC_LAMP_INIT_START;
					}
				}
				else
				{
					ble_plc_lamp_run_check_net_fail_cnt = 0;
				}
				ble_sta_net_check_param_send();
				StartTimer(&ble_plc_lamp_run_check_net_timer,BLE_PLC_RUN_CHECK_SOLT_MS);
			}
			break;
		}
		#endif
		//==============
		case PLC_BLE_LAMP_INIT_START:
		{
			ble_local_run_state = PLC_BLE_LAMP_BLE_LOCAL_INIT_START;
			break;
		}	
		case PLC_BLE_LAMP_BLE_LOCAL_INIT_START:
		{
			ble_system_params.NetSeq++;
			ble_system_params.DirtySystemParamFlg = 1;
			ble_system_info_stored();
			//初始化网络参数
			ble_data_reset_net_data();
			//
			ble_net_info.NetEnableFlag = ble_system_params.NetEnableFlag;
			ble_net_info.NetModeFlg  = 0;//模式1=STA,0=cco
			StartTimer(&ble_local_run_timer,BLE_LOCAL_WAIT_TIME_MS);
			ble_local_run_init_process();
			ble_local_run_state = PLC_BLE_LAMP_BLE_LOCAL_INIT_WAITE;
			break;
		}
		case PLC_BLE_LAMP_BLE_LOCAL_INIT_WAITE:
		{
			if(LcTimeOut(&ble_local_run_timer))
			{
				ble_local_run_state_error_cnt++;
				ble_local_run_state = PLC_BLE_LAMP_BLE_LOCAL_INIT_ERROR;
				//
				StartTimer(&ble_local_run_timer,BLE_LOCAL_ERROR_RESET_TIME_MS);
			}
			break;
		}
		case PLC_BLE_LAMP_BLE_LOCAL_INIT_WAITE_ACK:
		{
			StartTimer(&ble_local_run_timer,BLE_LOCAL_CHECK_NET_TIME_MS);
			ble_local_run_state = PLC_BLE_LAMP_BLE_LOCAL_CHECK_STATE;
			break;
		}
		case PLC_BLE_LAMP_BLE_LOCAL_INIT_ERROR:
		{
			if(LcTimeOut(&ble_local_run_timer))
			{
				ble_local_run_state = PLC_LAMP_BLE_LOCAL_INIT_START;
			}
			break;
		}
		case PLC_BLE_LAMP_BLE_LOCAL_CHECK_STATE:
		{
			ble_local_run_state = PLC_BLE_LAMP_BLE_LOCAL_RUN_STATE;
			ble_plc_lamp_run_check_net_fail_cnt = 0;
			StartTimer(&ble_plc_lamp_run_check_net_timer,BLE_PLC_RUN_CHECK_SOLT_MS);
			break;
		}
		case PLC_BLE_LAMP_BLE_LOCAL_RUN_STATE:
		{
			//检测plc入网离网情况；若长时间离网，离开网关模式，重启PLC组网；或转换ble
			if(LcTimeOut(&ble_plc_lamp_run_check_net_timer))
			{
				lc_net_my_info_t plc_net_info;
				lc_net_get_net_self_info(&plc_net_info);
				if(plc_net_info.net_state == LC_NET_IN_STATE)
				{//入网
					ble_plc_lamp_run_check_net_fail_cnt = 0;					
				}
				else
				{
					++ble_plc_lamp_run_check_net_fail_cnt;
					if(ble_plc_lamp_run_check_net_fail_cnt > 100)
					{//50分钟不入网
						ble_local_run_state = PLC_LAMP_INIT_START;
					}
				}
				StartTimer(&ble_plc_lamp_run_check_net_timer,BLE_PLC_RUN_CHECK_SOLT_MS);
			}
			break;
		}
		default:
			break;
	}	
}
//0:plc,1:ble
uint8_t get_sta_in_net_src()
{
	lc_net_my_info_t plc_net_info;
	lc_net_get_net_self_info(&plc_net_info);
	if(plc_net_info.net_state != LC_NET_IN_STATE)
	{//入网
		if(ble_sta_net_info.net_statu == 1)
		{
			return 1;
		}
	}
	return 0;
}















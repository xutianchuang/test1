#include "ble_apl.h"
#include "ble_mac.h"
#include "ble_local.h"
//#include "malloc.h"
#include "lc_all.h"
#include "ble_net.h"
//===================================
#include "lc_whi_sysctrl.h"
#include "lc_obj_model.h"
//===========================
uint8_t ble_apl_send_system_ctrl_trun_cmd_frame(uint16_t dev_addr,uint8_t cmd,ble_apl_tag_str_type *p_tag_str,uint8_t *p_data,uint8_t d_len, uint16_t app_seq_num)
{
	uint8_t r_flg = 0;
	if(d_len > 16)
	{
		return 0;
	}
	uint16_t apl_len = BLE_MAC_SEND_FRAME_HEAD_LEN +  sizeof(ble_apl_frame_type) + sizeof(ble_apl_system_trun_cmd_frame_type) + d_len;
	uint8_t *ptr = ble_local_malloc(BLE_LOCAL_SEND_FRAME_HEAD_LEN + apl_len + 16);
	if(ptr != NULL)
	{
		ble_apl_frame_type *p_send_apl = (ble_apl_frame_type *)(&ptr[BLE_MAC_SEND_APL_LINK_HEAD_OFF]);
		p_send_apl->cmd = BEL_APL_SYSTEM_CTRL_TRUN_CMD;
		p_send_apl->ver = BEL_APL_VERSION;
		//p_send_apl->data_len = sizeof(ble_apl_system_trun_cmd_frame_type) + d_len;
		//
		ble_apl_system_trun_cmd_frame_type *p_trun_cmd_fram = (ble_apl_system_trun_cmd_frame_type*)p_send_apl->frame_data;
		p_trun_cmd_fram->dev_addr = dev_addr;
		p_trun_cmd_fram->cmd = cmd;
		//
		uint16_t data_indx = 0;
		//
		if((p_tag_str != NULL)
			&&(p_tag_str->tag_sum != 0))
		{
			p_trun_cmd_fram->tag_flg = 1;
			uint16_t tag_len = sizeof(ble_apl_tag_str_type) + p_tag_str->tag_sum;
			memcpy(&(p_trun_cmd_fram->frame_data[data_indx]),p_tag_str,tag_len);
			data_indx += tag_len;
		}
		else
		{
			p_trun_cmd_fram->tag_flg = 0;
			data_indx = 0;
		}
		memcpy(&(p_trun_cmd_fram->frame_data[data_indx]),p_data,d_len);
		d_len += data_indx;		
		p_send_apl->data_len = sizeof(ble_apl_system_trun_cmd_frame_type) + d_len;
		
		//
		uint8_t app_frame_len = sizeof(ble_apl_frame_type) + sizeof(ble_apl_system_trun_cmd_frame_type) + d_len;
		//
		uint16_t frame_len = ble_mac_create_apl_cmd_no_link_frame(ptr,&ptr[BLE_MAC_SEND_APL_LINK_HEAD_OFF],app_frame_len, app_seq_num);
		//
		
		if(true == ble_local_send_data(ptr,frame_len))
		{
			ble_net_debug_cnt_list.plc_to_ble_apl_send_ctrl_cnt++;
			ble_net_debug_cnt_list.bus_fram_send_success_cnt++;
			r_flg = true;
		}
		else
		{
			ble_net_debug_cnt_list.bus_fram_send_err_cnt++;
		}
		//
		ble_local_free(ptr);
	}
	return r_flg;
}



//=================
uint8_t ble_apl_send_local_cmd_frame(uint8_t* to_mac,const void *msg, const uint32_t len,const uint32_t flg)
{
	uint8_t brocast_addr[LC_NET_ADDR_SIZE] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	uint8_t local_brocast_addr[LC_NET_ADDR_SIZE] = {0xFE,0xFF,0xFF,0xFF,0xFF,0xFE};
	
	if((0 != memcmp(brocast_addr,to_mac,BLE_LONG_ADDR_SIZE))
		&&(0 != memcmp(local_brocast_addr,to_mac,BLE_LONG_ADDR_SIZE)))
	{
		if(ble_get_mac_addr_to_net_tei(to_mac)<= 0)
		{
			return 0;
		}
	}
	
	
	uint16_t apl_len = BLE_MAC_SEND_FRAME_HEAD_LEN +  sizeof(ble_apl_frame_type) + len;
	uint8_t *ptr = ble_local_malloc(BLE_LOCAL_SEND_FRAME_HEAD_LEN + apl_len + 16);
	if(ptr != NULL)
	{
		ble_apl_frame_type *p_send_apl = (ble_apl_frame_type *)(&ptr[BLE_MAC_SEND_APL_LINK_HEAD_OFF]);
		p_send_apl->cmd = BEL_APL_LOCAL_CMD;
		p_send_apl->ver = BEL_APL_VERSION;
		p_send_apl->data_len = len;
		memcpy(p_send_apl->frame_data,msg,len);
		//
		uint16_t frame_len = ble_mac_create_apl_cmd_frame(ptr,to_mac,&ptr[BLE_MAC_SEND_APL_LINK_HEAD_OFF],sizeof(ble_apl_frame_type) + len);
		//
		if(true == ble_local_send_data(ptr,frame_len))
		{
			ble_net_debug_cnt_list.plc_to_ble_apl_send_debug_cnt++;//发送的蓝牙调试
			uint16_t dst_tei = ble_get_mac_addr_to_net_tei(to_mac);
			if(dst_tei <= BLE_MAX_TEI_VALUE)
			{
				ble_neighbor_list[dst_tei].FromCcoAplCnt ++;
			}
		}
		//
		ble_local_free(ptr);
	}
	return 0;
}
//==
uint8_t ble_apl_send_system_ctrl_cmd_frame(uint8_t* to_mac,void *msg, uint32_t len,uint32_t flg)
{
    whi_sysctrl_type *p_send_ctrl;
    uint16_t dst_tei;

	uint8_t brocast_addr[BLE_LONG_ADDR_SIZE] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	uint8_t local_brocast_addr[BLE_LONG_ADDR_SIZE] = {0xFE,0xFF,0xFF,0xFF,0xFF,0xFF};

    ble_net_debug_cnt_list.bus_fram_cnt++;
	
	if((0 != memcmp(brocast_addr,to_mac,BLE_LONG_ADDR_SIZE))
		&&(0 != memcmp(local_brocast_addr,to_mac,BLE_LONG_ADDR_SIZE)))
	{
		if(ble_get_mac_addr_to_net_tei(to_mac)<= 0)
		{
            ble_net_debug_cnt_list.bus_fram_err_cnt++;
			return 0;
		}
	}
	//===============
	bool tran_data_flg = true;
	//处理，若为广播帧，且灯头亮度命令，转换成ble应用命令，广播发送；
	if((0 == memcmp(brocast_addr,to_mac,BLE_LONG_ADDR_SIZE))
		||(0 == memcmp(local_brocast_addr,to_mac,BLE_LONG_ADDR_SIZE)))
	{
		p_send_ctrl = (whi_sysctrl_type *)msg;
		//msg//len
		if((0 == memcmp(brocast_addr,p_send_ctrl->mac_add,BLE_LONG_ADDR_SIZE))
				||(0 == memcmp(local_brocast_addr,p_send_ctrl->mac_add,BLE_LONG_ADDR_SIZE)))
		{
			if(p_send_ctrl->user_len >=(8))
			{
				if(p_send_ctrl->fun_code == WHI_SYSCTRL_WRITE_DEV_ATTR)
				{
					uint32_t obj_mode_len = p_send_ctrl->user_len;
					if(obj_mode_len > (WHI_SYSCTRL_HEAD_USER_START_SIZE+OBJECT_MODE_HEAD_START_SIZE))
					{
						obj_mode_len -= WHI_SYSCTRL_HEAD_USER_START_SIZE;
						uint32_t saved_obj_mode_len = obj_mode_len;
						object_model_interface_t *p_object_mode = (object_model_interface_t *)(&p_send_ctrl->data_body[0]);
						//
						uint8_t tag_buff[sizeof(ble_apl_tag_str_type) + TAG_LIST_MAX_LEN];
						ble_apl_tag_str_type *p_tag_str = (ble_apl_tag_str_type *)tag_buff;
						p_tag_str->tag_sum = 0;

						bool trans_fast_ctrl = true;
						//
						uint8_t tag_len = 0;
						uint32_t data_body_index = 0;
						while(obj_mode_len >= 4)
						{
							if(p_object_mode->SIID == SIID_TAG_OPERATE_OBJ)
							{
								if(p_object_mode->data_buff[0] == TAG_LIST_MAX_LEN)
								{
									p_tag_str->tag_sum = TAG_LIST_MAX_LEN;
									for(int k = 0; k < TAG_LIST_MAX_LEN; k++)
									{
										p_tag_str->tag_list[k] = p_object_mode->data_buff[k + 1];
									}
								}
								//break;
							}
							else if((((p_object_mode->SIID == SIID_LAMP_LED) && (p_object_mode->CIID == CIID_LAMP_DEBUG_LED_BRIGHTNESS)) == false) && 
								(((p_object_mode->SIID == SIID_LAMP) && (p_object_mode->CIID == CIID_LAMP_MODE)) == false))
							{
								trans_fast_ctrl = false;
							}
							tag_len = (sizeof(object_model_interface_t) + p_object_mode->data_len);
							obj_mode_len -= tag_len;
							data_body_index += tag_len;
							p_object_mode = (object_model_interface_t *)(&p_send_ctrl->data_body[data_body_index]);
						}

						obj_mode_len = saved_obj_mode_len;
						data_body_index = 0;
						p_object_mode = (object_model_interface_t *)(&p_send_ctrl->data_body[0]);
						while(trans_fast_ctrl && (obj_mode_len >= 4))
						{
							if(obj_mode_len > OBJECT_MODE_HEAD_START_SIZE)
							{
								if(p_object_mode->data_len + OBJECT_MODE_HEAD_START_SIZE <= obj_mode_len)
								{
									if((p_object_mode->SIID == SIID_LAMP_LED)&&(p_object_mode->CIID == CIID_LAMP_DEBUG_LED_BRIGHTNESS)
										&&((p_object_mode->data_len >= UINT_SIZE)&&(p_object_mode->data_type == UINT_TYPE)))
									{
										uint16_t brigh_value = *((uint32_t*)p_object_mode->data_buff);
										uint16_t dev_addr = p_send_ctrl->dev_addr;
										//
										tran_data_flg = false;
										//
										ble_apl_send_system_ctrl_trun_cmd_frame(dev_addr,BEL_APL_SYSTEM_CTRL_TRUN_LED_BRIGHTNESS_CMD,
											p_tag_str,(uint8_t*)&brigh_value,sizeof(brigh_value), p_send_ctrl->seq_num);
									}
									else if((p_object_mode->SIID == SIID_LAMP)&&(p_object_mode->CIID == CIID_LAMP_MODE)
										&&((p_object_mode->data_len >= ENUM_SIZE)&&(p_object_mode->data_type == ENUM_TYPE)))
									{
										uint8_t mode_value = p_object_mode->data_buff[0];
										uint16_t dev_addr = p_send_ctrl->dev_addr;
										//
										tran_data_flg = false;
										//
										ble_apl_send_system_ctrl_trun_cmd_frame(dev_addr,BEL_APL_SYSTEM_CTRL_TRUN_MODE_CMD,
											p_tag_str,(uint8_t*)&mode_value,sizeof(mode_value), p_send_ctrl->seq_num);
									}
								}
							}
							tag_len = (sizeof(object_model_interface_t) + p_object_mode->data_len);
							obj_mode_len -= tag_len;
							data_body_index += tag_len;
							p_object_mode = (object_model_interface_t *)(&p_send_ctrl->data_body[data_body_index]);
						}
					}
				}
			}
		}
			
		
	}
    else
    {   
        // 若帧为上报响应数据，则转ble快速广播
        dst_tei = ble_get_mac_addr_to_net_tei(to_mac);
        if(dst_tei <= BLE_MAX_TEI_VALUE)
        {
            p_send_ctrl = (whi_sysctrl_type *)msg;
            
            if(p_send_ctrl->user_len >=(8))
            {
                if(p_send_ctrl->fun_code == (WHI_SYSCTRL_REPORT_DEV_EVENT | WHI_SYSCTRL_ACK_MASK))
                {
                    tran_data_flg = false;
                    ble_mac_create_report_ack_cmd_frame(dst_tei, p_send_ctrl->data_body, p_send_ctrl->user_len - USER_DATA_HEAD_LEN);
                }
            }
        }
    }
	//
	if(tran_data_flg == true)
	{
		uint16_t apl_len = BLE_MAC_SEND_FRAME_HEAD_LEN +  sizeof(ble_apl_frame_type) + len;
		uint8_t *ptr = ble_local_malloc(BLE_LOCAL_SEND_FRAME_HEAD_LEN + apl_len + 16);
		if(ptr != NULL)
		{
			ble_apl_frame_type *p_send_apl = (ble_apl_frame_type *)(&ptr[BLE_MAC_SEND_APL_LINK_HEAD_OFF]);
			p_send_apl->cmd = BEL_APL_SYSTEM_CTRL_CMD;
			p_send_apl->ver = BEL_APL_VERSION;
			p_send_apl->data_len = len;
			memcpy(p_send_apl->frame_data,msg,len);
			//
			uint16_t frame_len = ble_mac_create_apl_cmd_frame(ptr,to_mac,&ptr[BLE_MAC_SEND_APL_LINK_HEAD_OFF],sizeof(ble_apl_frame_type) + len);
			//
			if(true == ble_local_send_data(ptr,frame_len))
			{
				ble_net_debug_cnt_list.plc_to_ble_apl_send_ctrl_cnt++;
				ble_net_debug_cnt_list.bus_fram_send_success_cnt++;
				uint16_t dst_tei = ble_get_mac_addr_to_net_tei(to_mac);
				if(dst_tei <= BLE_MAX_TEI_VALUE)
				{
					ble_neighbor_list[dst_tei].FromCcoAplCnt ++;
				}
				//
				if(0 != memcmp(brocast_addr,to_mac,BLE_LONG_ADDR_SIZE))
				{
					ble_net_debug_cnt_list.bus_fram_brocast_cnt++;
				}
				else if(0 != memcmp(local_brocast_addr,to_mac,BLE_LONG_ADDR_SIZE))
				{
					ble_net_debug_cnt_list.bus_fram_local_brocast_cnt++;
				}
				else
				{
					ble_net_debug_cnt_list.bus_fram_send_success_cnt++;
				}
			}
			else
			{
				ble_net_debug_cnt_list.bus_fram_send_err_cnt++;
			}
			//
			ble_local_free(ptr);
		}
	}
	
	return 0;
}

//===================================
uint16_t ble_apl_local_cmd_process(uint8_t *from_mac,ble_mac_cmd_type *p_mac_cmd)
{
	ble_apl_frame_type *p_apl_frame = (ble_apl_frame_type *)p_mac_cmd->cmd_data;
	//
	if(p_apl_frame->data_len >= (BLE_LONG_ADDR_SIZE + 2))
	{
		uint8_t* send_buff = malloc(WHI_PRO_HEAD_SIZE+32 + p_apl_frame->data_len);
		if(send_buff != NULL)
		{
			//
			uint16_t frame_idx = WHI_PRO_STAT_HEAD_SIZE;
			memcpy(&send_buff[frame_idx],p_apl_frame->frame_data,p_apl_frame->data_len);
			memcpy(&send_buff[frame_idx],from_mac,BLE_LONG_ADDR_SIZE);
			//frame_idx += BLE_LONG_ADDR_SIZE;
			//send_buff[frame_idx++] = (p_apl_frame->data_len)&0xFF;
			//send_buff[frame_idx++] = ((p_apl_frame->data_len)>>8)&0xFF;

			frame_idx += p_apl_frame->data_len;
			uint16_t frame_len = WHI_BuildFrameProcess(send_buff,true,WHI_RECEIVE_STA_FRAME,WHI_CreateLocalSeq(),&send_buff[WHI_PRO_STAT_HEAD_SIZE],frame_idx - WHI_PRO_STAT_HEAD_SIZE);
			//
			ble_net_ack_send_data(send_buff,frame_len);
			//
			free(send_buff);
		}
	}
	//uint8_t *from_mac = NULL;
	//p_apl_frame->frame_data[];
	

	//
	return 0;
}
//BEL_APL_SYSTEM_CTRL_CMD
uint16_t ble_apl_system_ctrl_cmd_process(uint8_t *from_mac,ble_mac_cmd_type *p_mac_cmd)
{
	ble_apl_frame_type *p_apl_frame = (ble_apl_frame_type *)p_mac_cmd->cmd_data;
	//
	if(p_apl_frame->data_len >= (BLE_LONG_ADDR_SIZE + 2))
	{
		uint8_t* send_buff = malloc(WHI_PRO_HEAD_SIZE+32 + p_apl_frame->data_len);
		if(send_buff != NULL)
		{
			//
			uint16_t frame_idx = WHI_PRO_STAT_HEAD_SIZE;
			memcpy(&send_buff[frame_idx],p_apl_frame->frame_data,p_apl_frame->data_len);
			memcpy(&send_buff[frame_idx],from_mac,BLE_LONG_ADDR_SIZE);
			//frame_idx += BLE_LONG_ADDR_SIZE;
			//send_buff[frame_idx++] = (p_apl_frame->data_len)&0xFF;
			//send_buff[frame_idx++] = ((p_apl_frame->data_len)>>8)&0xFF;

			frame_idx += p_apl_frame->data_len;
			uint16_t frame_len = WHI_BuildFrameProcess(send_buff,true,WHI_SEND_BUS_FRAME,WHI_CreateLocalSeq(),&send_buff[WHI_PRO_STAT_HEAD_SIZE],frame_idx - WHI_PRO_STAT_HEAD_SIZE);
			//
			ble_net_ack_send_data(send_buff,frame_len);
			//
			free(send_buff);
		}
	}
	//uint8_t *from_mac = NULL;
	//p_apl_frame->frame_data[];
	

	//
	return 0;
}


uint16_t ble_apl_process_frame(ble_mac_frame_type *p_mac,ble_mac_path_type *p_mac_path,ble_mac_cmd_type *p_mac_cmd,uint16_t frame_len)
{
//	if((p_mac != NULL)&&(p_mac_cmd != NULL))
	{
		uint16_t apl_frame_len = (((uint32_t)p_mac_cmd) - ((uint32_t)p_mac));
//		if(apl_frame_len >= frame_len)
//		{
//			apl_frame_len = 0;
//		}
//		else
		{
			apl_frame_len = frame_len - apl_frame_len;
		}
		//
		ble_net_debug_cnt_list.ble_net_apl_frame_cnt++;
		//
		if(apl_frame_len >= sizeof(ble_apl_frame_type))
		{
			ble_apl_frame_type *p_apl_frame = (ble_apl_frame_type *)p_mac_cmd->cmd_data;
			if(p_apl_frame->ver == BEL_APL_VERSION)
			{
				if(p_apl_frame->data_len <= (apl_frame_len - sizeof(ble_apl_frame_type)))
				{
					switch(p_apl_frame->cmd)
					{
						case BEL_APL_LOCAL_CMD:
						case BEL_APL_SYSTEM_CTRL_CMD:
						{
							uint8_t from_mac[BLE_LONG_ADDR_SIZE];
							if(p_mac_path != NULL)
							{
								uint16_t from_tei_idx = (p_mac_path->path_len - 1)*2;
								uint16_t from_tei = p_mac_path->path_list[from_tei_idx+1];
								from_tei = (from_tei<<8)|p_mac_path->path_list[from_tei_idx];
								//
								if((from_tei>BLE_MIN_TEI_VALUE)&&(from_tei <= BLE_MAX_TEI_VALUE))
								{
									ble_neighbor_list[from_tei].ToCcoAplCnt ++ ;
									memcpy(from_mac,ble_neighbor_list[from_tei].MacAddr,BLE_LONG_ADDR_SIZE);
								}
								else
								{
									memset(from_mac,0xFF,BLE_LONG_ADDR_SIZE);
								}
							}
							else
							{
								memset(from_mac,0xFF,BLE_LONG_ADDR_SIZE);
							}
							if(p_apl_frame->cmd == BEL_APL_SYSTEM_CTRL_CMD)
							{
								ble_apl_system_ctrl_cmd_process(from_mac,p_mac_cmd);
							}
							else
							{
								ble_apl_local_cmd_process(from_mac,p_mac_cmd);
							}
						}
							break;
						case BEL_APL_SYSTEM_CTRL_TRUN_CMD:
						{
						}
							break;
						default:
							break;
					}
				}
			}
		}
	}
	return 0;
}



















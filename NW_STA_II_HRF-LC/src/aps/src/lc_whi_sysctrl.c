#include <stdlib.h>
#include <string.h>
#include "lc_all.h"
#include "tmr_call.h"
#include "ble_event.h"


uint16_t whi_sysctrl_seq_value;
//========================
//系统控制处理接口

/**
 * [whi_sysctrl_process_frame    whi应用处理]
 * @author    ginvc
 * @dateTime  2022-10-16
 * @attention none
 * @param     src_data     []
 * @param     src_len      []
 * @return    d_len        [回复报文长度]    
 */
uint16_t whi_sysctrl_process_frame(uint8_t *src_data,uint16_t src_len,uint8_t is_brocast_addr)
{
	uint16_t d_len = 0;
	whi_sysctrl_type *sysctrl_head = (whi_sysctrl_type*)src_data;

	if(sysctrl_head->plc_pro == 0x0001)
	{//地址判断
		switch(sysctrl_head->fun_code)
		{
			// 设置物模型属性
			case WHI_SYSCTRL_WRITE_DEV_ATTR:
			{
				bool process_flg = false;
				bool check_ack_flg = false; // 是否处理过
				int process_offset;
				int obj_model_idx;

				int obj_model_len = sysctrl_head->user_len - WHI_SYSCTRL_HEAD_USER_START_SIZE;
				if (obj_model_len > OBJECT_MODE_HEAD_START_SIZE)
				{
					process_flg = true;
					if (whi_group_tag_obj_model_check(sysctrl_head->data_body, obj_model_len))
					{
						obj_model_idx = 0;
						while (obj_model_len >= 4)
						{
							object_model_interface_t *p_obj = (object_model_interface_t *)&sysctrl_head->data_body[obj_model_idx];
							process_offset = OBJECT_MODE_HEAD_START_SIZE + p_obj->data_len;
							if (obj_model_len < process_offset)
							{
								obj_model_len = 0;
								break;
							}
							if (p_obj->SIID != SIID_TAG_OPERATE_OBJ)
							{
								if (p_obj->data_len > 0)
								{
									check_ack_flg = true;
									if (false == obj_mode_put_attr_value(p_obj))
									{
										process_flg = false;
									}
								}
							}
							obj_model_len -= process_offset;
							obj_model_idx += process_offset;
						}
					}
				}
				else
				{
					check_ack_flg = true;
				}

				sysctrl_head->stat_code = (process_flg == false) ? WHI_SYSCTRL_ATTR_NOT_WRITE_CODE : WHI_SYSCTRL_SUCCESS_CODE;
				//sysctrl_head->dev_addr = light_sysctrl.dev_addr;
				sysctrl_head->fun_code |= WHI_SYSCTRL_ACK_MASK;
				if(is_brocast_addr == BROCAST_ADDR_VALID && process_flg == false)
				{
					d_len = 0;
				}
				else
				{
					d_len = (check_ack_flg == true) ? (MAC_LEN_HEAD_LEN + USER_DATA_HEAD_LEN) : 0;
				}
				sysctrl_head->user_len = USER_DATA_HEAD_LEN;
				break;
			}

			// 读取物模型属性
			case WHI_SYSCTRL_READ_DEV_ATTR:
			{
				//bool process_flg = false;
				uint16_t stat_code = WHI_SYSCTRL_SUCCESS_CODE;
				uint16_t obj_model_idx = 0;
				bool check_ack_flg = false; // 是否处理过
				uint8_t* process_buff = malloc(1024 + 32);

				uint32_t obj_model_len = sysctrl_head->user_len;
				if ((process_buff != NULL) && (obj_model_len >= (WHI_SYSCTRL_HEAD_USER_START_SIZE + (sizeof(uint16_t) * 2))))
				{
					//process_flg = true;

					obj_model_len = sysctrl_head->user_len;
					obj_model_len -= WHI_SYSCTRL_HEAD_USER_START_SIZE;
					uint32_t read_idx = 0;
					while (obj_model_len >= 4)
					{
						object_model_interface_t *p_object_mode = (object_model_interface_t *)&process_buff[obj_model_idx];
						object_model_interface_t *p_src_object_mode = (object_model_interface_t *)&sysctrl_head->data_body[read_idx];
						if (p_src_object_mode->SIID != SIID_TAG_OPERATE_OBJ)
						{
							check_ack_flg = true;
							p_object_mode->SIID = p_src_object_mode->SIID;
							p_object_mode->CIID = p_src_object_mode->CIID;
							if (true == obj_mode_get_attr_value(p_object_mode, 1024 - obj_model_idx))
							{
								obj_model_idx += p_object_mode->data_len + OBJECT_MODE_HEAD_START_SIZE;
								if (obj_model_idx >= 256)
									break;
							}
							else
							{
								//成品工装测试时，如果这个没读到，就不回复
								if(p_src_object_mode->SIID == SIID_FACTORY_TEST && p_src_object_mode->CIID == CIID_FACTORY_BLE_RSSI)
								{
									check_ack_flg = false;
								}
							}
						}
						read_idx += 4;
						obj_model_len -= 4;
					}
					if (obj_model_idx == 0)
					{
						stat_code = WHI_SYSCTRL_ATTR_NOT_READ_CODE;
					}
					else
					{
						memcpy(sysctrl_head->data_body, process_buff, obj_model_idx);
					}
				}
				else
				{
					stat_code = WHI_SYSCTRL_UNDEFINE_CODE;
					check_ack_flg = true;
				}
				sysctrl_head->stat_code = stat_code;
				//sysctrl_head->dev_addr = light_sysctrl.dev_addr;
				sysctrl_head->fun_code |= WHI_SYSCTRL_ACK_MASK;
				d_len = (check_ack_flg == true) ? (MAC_LEN_HEAD_LEN + USER_DATA_HEAD_LEN + obj_model_idx) : 0;
				sysctrl_head->user_len = USER_DATA_HEAD_LEN + obj_model_idx;
				if (process_buff != NULL)
				{
					free(process_buff);
				}
				break;
			}

			//复位
			case WHI_SYSCTRL_RESET_DEV:
			{
				sysctrl_head->stat_code = WHI_SYSCTRL_SUCCESS_CODE;
				sysctrl_head->dev_addr = 0;
				sysctrl_head->fun_code |= WHI_SYSCTRL_ACK_MASK;
				d_len = MAC_LEN_HEAD_LEN + USER_DATA_HEAD_LEN;
				sysctrl_head->user_len = USER_DATA_HEAD_LEN;
				HPLC_RegisterTmr(LcAppRebootTimerCallback, 0, 1000ul, TMR_OPT_CALL_ONCE);
				break;
			}
				

			case WHI_SYSCTRL_READ_MEMORY_DATA:
			{
				if(sysctrl_head->user_len >= (5 + USER_DATA_HEAD_LEN))
				{
					uint32_t read_addr = sysctrl_head->data_body[3];
					read_addr = (read_addr<<8)|sysctrl_head->data_body[2];
					read_addr = (read_addr<<8)|sysctrl_head->data_body[1];
					read_addr = (read_addr<<8)|sysctrl_head->data_body[0];
					//
					uint16_t read_bytes = sysctrl_head->data_body[4];
					if(read_bytes>192)read_bytes = 192;
					if(read_bytes<4)read_bytes = 4;
					sysctrl_head->data_body[4] = read_bytes;
					
					
					#if defined(II_STA)
					// 读flash
					if (read_addr > FLASH_START_ADDR && read_addr < (FLASH_START_ADDR + MAX_DATA_FLASH_SIZE))
					{
						ReadDataFlash(read_addr, &(sysctrl_head->data_body[5]), read_bytes);
					}
					// 读内存
					else if ((read_addr > 0 && read_addr < 0xC0000) || (read_addr > 0x10000000 && read_addr < 0x100C0000))
					{
						memcpy(&(sysctrl_head->data_body[5]),(void*)read_addr,read_bytes);
					}
					// 读外设
					else if (read_addr > 0x40000000 && read_addr < 0x41000000)
					{
						memcpy(&(sysctrl_head->data_body[5]), (void*)read_addr, read_bytes);
					}
					// 读arm
					else if (read_addr > 0xE0000000 && read_addr < 0xE0040FFF)
					{
						memcpy(&(sysctrl_head->data_body[5]), (void*)read_addr, read_bytes);
					}
					else
					#endif
					{
						memset(&(sysctrl_head->data_body[5]), 0xFF, read_bytes);
					}
					//
					sysctrl_head->stat_code = WHI_SYSCTRL_SUCCESS_CODE;
					sysctrl_head->dev_addr = 0;
					sysctrl_head->fun_code |= WHI_SYSCTRL_ACK_MASK;
					sysctrl_head->user_len = USER_DATA_HEAD_LEN + 5 + read_bytes;
					d_len = MAC_LEN_HEAD_LEN + sysctrl_head->user_len;
				}
				break;
			}
				
			case WHI_SYSCTRL_WRITE_MEMORY_DATA:
			{
				if(sysctrl_head->user_len > (5 + USER_DATA_HEAD_LEN))
				{
					uint32_t write_addr = sysctrl_head->data_body[3];
					write_addr = (write_addr<<8)|sysctrl_head->data_body[2];
					write_addr = (write_addr<<8)|sysctrl_head->data_body[1];
					write_addr = (write_addr<<8)|sysctrl_head->data_body[0];
					//
					uint16_t write_bytes = sysctrl_head->data_body[4];
					if(write_bytes>192)write_bytes = 192;
					sysctrl_head->data_body[4] = write_bytes;

					
					memcpy((void*)write_addr,&(sysctrl_head->data_body[5]),write_bytes);
					memcpy(&(sysctrl_head->data_body[5]),(void*)write_addr,write_bytes);
					
					//
					sysctrl_head->stat_code = WHI_SYSCTRL_SUCCESS_CODE;
					sysctrl_head->dev_addr = 0;
					sysctrl_head->fun_code |= WHI_SYSCTRL_ACK_MASK;
					sysctrl_head->user_len = USER_DATA_HEAD_LEN + 5 + write_bytes;
					d_len = MAC_LEN_HEAD_LEN + sysctrl_head->user_len;
				}
				break;
			}
				
			case (WHI_SYSCTRL_REPORT_DEV_EVENT | WHI_SYSCTRL_ACK_MASK):
			{
				lamp_event_recv_callback(sysctrl_head->data_body, sysctrl_head->user_len - USER_DATA_HEAD_LEN);
				break;
			}

			default:
				break;
		}
	}
	return d_len;
}

uint32_t whi_sysctrl_report_process(uint8_t report_fun_code, uint8_t *report_process_buffer, uint16_t report_data_len)
{
    lc_net_my_info_t p_info;
    lc_net_get_net_self_info(&p_info);
    if (p_info.net_state == LC_NET_IN_STATE)
    {
        whi_sysctrl_type *sysctrl_head = (whi_sysctrl_type *)(&(report_process_buffer[WHI_HPLC_ADAPT_HEAD_LEN]));

        memcpy(sysctrl_head->mac_add, p_info.cco_addr, LC_NET_ADDR_SIZE);
        sysctrl_head->user_len = USER_DATA_HEAD_LEN + report_data_len;
        sysctrl_head->plc_pro = 0x0001;
        sysctrl_head->seq_num = ++whi_sysctrl_seq_value;
        sysctrl_head->fun_code = report_fun_code;
        sysctrl_head->dev_addr = 5;//light_sysctrl.dev_addr;  $$$
        sysctrl_head->stat_code = WHI_SYSCTRL_SUCCESS_CODE;

        uint16_t send_len = whi_hplc_adapt_create(WHI_SEND_BUS_FRAME, report_process_buffer, (uint8_t *)sysctrl_head, MAC_LEN_HEAD_LEN + sysctrl_head->user_len);
        lc_net_tx_data(sysctrl_head->mac_add, report_process_buffer, send_len, LC_NET_NOT_BLOCK);
    }
    return whi_sysctrl_seq_value;
}

void whi_sysctrl_report_event_data(uint8_t *event_data, uint16_t event_len)
{
    uint8_t *process_buff = malloc(1024 + 32);
    if (process_buff != NULL)
    {
        memcpy(&process_buff[WHI_HPLC_ADAPT_HEAD_LEN + USER_DATA_HEAD_LEN + MAC_LEN_HEAD_LEN], event_data, event_len);

        whi_sysctrl_report_process(WHI_SYSCTRL_REPORT_DEV_EVENT, process_buff, event_len);
        free(process_buff);
    }
}
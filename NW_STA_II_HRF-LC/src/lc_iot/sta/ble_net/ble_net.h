
#ifndef __BLE_NET_H__
#define __BLE_NET_H__
#include "stdint.h"

#define BUILD_YEAR    (((__DATE__[9] - '0')<<4)|((__DATE__[10] - '0')<<0))
#define BUILD_MONTH    ((__DATE__[2] == 'n')?(__DATE__[1] == 'a'?1:6)\
:(__DATE__[2] == 'b')?(2)\
:(__DATE__[2] == 'r')?(__DATE__[0] == 'M')?(3):(4)\
:(__DATE__[2] == 'y')?(5)\
:(__DATE__[2] == 'n')?(6)\
:(__DATE__[2] == 'l')?(7)\
:(__DATE__[2] == 'g')?(8)\
:(__DATE__[2] == 'p')?(9)\
:(__DATE__[2] == 't')?(0x10)\
:(__DATE__[2] == 'v')?(0x11):(0x12))
#define BUILD_DAY    ((((__DATE__[4] == ' ')?0:(__DATE__[4] - '0'))<<4) | ((__DATE__[5] - '0')<<0))
#define BUILD_HOUR      (((__TIME__[0]-'0')<<4)|((__TIME__[1]-'0')<<0))
#define BUILD_MIN          (((__TIME__[3]-'0')<<4)|((__TIME__[4]-'0')<<0))
#define BUILD_SECOND      (((__TIME__[6]-'0')<<4)|((__TIME__[7]-'0')<<0))
#define SELF_VER_VALUE              (0xAA33)//
#define SELF_VER_RELEASE_DATE       ((BUILD_YEAR<<24)|(BUILD_MONTH<<16)|(BUILD_DAY<<8)|(BUILD_HOUR<<0))//

typedef struct
{
	//
	uint32_t ble_local_malloc_cnt;
	//
	uint32_t ble_local_send_add_task_success_cnt;//本地任务添加成功
	uint32_t ble_local_send_add_task_fail_cnt;//本地任务添加失败
	
	
	uint32_t ble_to_plc_tx_cnt;//ble接收处理回复到plc 发送帧；系统控制及事件
	uint32_t ble_to_ble_tx_cnt;//ble接收处理回复到ble端口；系统控制及事件
	uint32_t ble_to_debug_tx_cnt;//ble接收处理回复到debug端口计数；系统控制及事件
	
	uint32_t ble_local_receive_success_cnt;//ble处理帧；
	
	uint32_t create_path_enable_frame_cnt;//创建的带路径帧
	
	uint32_t receive_path_enable_frame_cnt;//接收到带路径帧
	
	uint32_t plc_to_ble_apl_send_ctrl_cnt;//发送的蓝牙抄控帧；
	uint32_t plc_to_ble_apl_send_debug_cnt;//发送的蓝牙调试
	//
	uint32_t ble_net_assoc_req_cnt;//关联请求
	uint32_t ble_net_into_net_success_cnt;
	uint32_t ble_net_into_check_key_fail_cnt;
	uint32_t ble_net_into_fail_cnt;
	//
	uint32_t ble_net_heart_cnt;
	uint32_t ble_net_key_send_cnt;
	uint32_t ble_net_key_ack_cnt;
	//
	uint32_t ble_net_apl_frame_cnt;
	//
	uint32_t ble_sync_time_cnt;
	uint32_t ble_leave_net_cnt;
    //
    uint32_t ble_bind_net_cnt;
	//
	uint32_t bus_fram_cnt;
	uint32_t bus_fram_brocast_cnt;
	uint32_t bus_fram_local_brocast_cnt;
	uint32_t bus_fram_err_cnt;
	uint32_t bus_fram_send_err_cnt;
	uint32_t bus_fram_send_success_cnt;
	
	
	uint32_t ble_net_ack_send_data_port_plc_2_ble;
	uint32_t ble_net_ack_send_data_port_ble_2_plc;
}ble_net_debug_cnt_list_type;
extern ble_net_debug_cnt_list_type ble_net_debug_cnt_list;


void ble_net_init();
void ble_net_process();
//==
typedef struct
{
	uint8_t bind_statue;//1 bind,0 not
	uint32_t leave_time_ms;//
}bind_code_query_info_type;

uint8_t StartNetBindTimer(uint32_t ms);
uint8_t StopNetBindTimer();
void QueryNetBindInfo(bind_code_query_info_type *p_info);
void lc_net_set_NetBindFlg(uint8_t bind_flg);
void LcSetFactoryMsg(void);
#endif


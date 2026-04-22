#ifndef __BLE_MAC_H__
#define __BLE_MAC_H__

#include "stdint.h"
#include "ble_data.h"
#include "cmsis_compiler.h"


//========================
#define MAC_VERSION					(0x01)

#define MAC_PATH_DISABLE_FLG		(0x00)
#define MAC_PATH_ENABLE_FLG			(0x01)

#define MAC_PATH_ADDR_2SIZE			(0x00)
#define MAC_PATH_ADDR_6SIZE			(0x01)

#define MAC_DST_PATH				(0x00)
#define MAC_SRC_PATH				(0x01)

#define MAC_ASSOC_REQ_CMD			(0x10)
#define MAC_ASSOC_REQ_ACK_CMD		(0x11)
#define MAC_LEAVE_IND_CMD			(0x12)
#define MAC_LEAVE_IND_ACK_CMD		(0x13)
#define MAC_HEART_REPORT_CMD		(0x14)
#define MAC_REPORT_CMD		(0x15)
#define MAC_REPORT_ACK_CMD		(0x16)
#define MAC_HEARTBEAT_UP_CMD		(0x17)
#define MAC_HEARTBEAT_DOWN_CMD		(0x18)

#define MAC_SYNC_TIME_STAMP_CMD		(0x50)
#define MAC_CFG_ISP_PROJ_KEY_CMD	(0x70)

#define MAC_APL_FRAME_CMD			(0x80)


typedef struct __PACKED
{
	uint8_t ver:4;
	uint8_t path_flg:1;
	uint8_t addr_flg:1;
	uint8_t path_mode:1;
	uint8_t res_flg:1;
	uint8_t frame_data[];
}ble_mac_frame_type;


typedef struct __PACKED
{
	uint8_t path_idx:4;
	uint8_t path_len:4;
	uint8_t path_list[];
}ble_mac_path_type;


typedef struct __PACKED
{
	uint8_t cmd;
	uint8_t cmd_data[];
}ble_mac_cmd_type;

//================
//命令

typedef struct __PACKED
{
	uint8_t mac_addr[BLE_LONG_ADDR_SIZE];
	uint16_t pco_tei;
	uint8_t port;
	int8_t rssi;
	uint8_t isp_key[3];
	uint8_t proj_key[3];
}ble_mac_assoc_cmd_type;

typedef struct __PACKED
{
	uint8_t status;
	uint8_t mac_addr[BLE_LONG_ADDR_SIZE];
	uint16_t pco_tei;
	uint16_t dev_tei;
	uint8_t port;
    uint8_t isp_key[3];
	uint8_t proj_key[3];
}ble_mac_assoc_ack_cmd_type;

typedef struct __PACKED
{
	uint8_t dealy_time;
	uint8_t tei_sum;
	uint8_t tei_list[];
}ble_mac_leave_inc_cmd_type;

typedef struct __PACKED
{
	uint8_t status;
	uint8_t mac_addr[BLE_LONG_ADDR_SIZE];
	uint16_t dev_tei;
}ble_mac_leave_inc_ack_cmd_type;

typedef struct __PACKED
{
    uint16_t src_tei;
    uint8_t mac[6];
}ble_mac_heart_report_cmd_type;

typedef struct __PACKED
{
	uint16_t src_tei;
    uint8_t data[0];
}ble_mac_event_report_cmd_type;

typedef struct __PACKED
{
	uint32_t time_stamp;
}ble_mac_sync_time_stamp_cmd_type;

typedef struct __PACKED
{
	uint8_t dir;//1下行，0上行
	uint32_t isp_key;
	uint32_t proj_key;
}ble_mac_cfg_isp_proj_key_cmd_type;


#define BLE_MAC_FRAME_HEAD_LEN	(sizeof(ble_mac_frame_type))
#define BLE_MAC_PATH_HEAD_LEN	(sizeof(ble_mac_path_type))
#define BLE_MAC_CMD_HEAD_LEN	(sizeof(ble_mac_cmd_type))
	
#define BLE_MAC_SEND_FRAME_HEAD_LEN			(BLE_MAC_FRAME_HEAD_LEN + BLE_MAC_PATH_HEAD_LEN + BLE_MAC_CMD_HEAD_LEN + (BLE_MAX_LAYER_VALUE*BLE_LONG_ADDR_SIZE))
#define BLE_MAC_SEND_APL_LINK_HEAD_OFF		(BLE_LOCAL_LINK_SEND_FRAME_HEAD_LEN + BLE_MAC_FRAME_HEAD_LEN + BLE_MAC_PATH_HEAD_LEN + BLE_MAC_CMD_HEAD_LEN + (BLE_MAX_LAYER_VALUE*BLE_LONG_ADDR_SIZE))

//====
//网络接收数据处理
uint16_t ble_mac_process_frame(uint8_t *mac_frame,uint16_t frame_len);
//创建应答层帧
uint16_t ble_mac_create_apl_cmd_frame(uint8_t *ptr,uint8_t *to_mac,uint8_t *apl_data,uint16_t apl_len);
uint16_t ble_mac_create_apl_cmd_no_link_frame(uint8_t *ptr,uint8_t *apl_data,uint16_t apl_len, uint16_t app_seq_num);
//创建离网指示，连接发送广播
uint16_t ble_mac_create_leave_inc_cmd_frame(uint8_t *ptr,uint16_t *leave_addr_idx);
//创建时间同步，无连接发送PCO广播
uint16_t ble_mac_create_sync_time_stamp_cmd_frame(uint8_t *ptr,uint32_t time_stamp);
//上报响应数据发送
uint16_t ble_mac_create_report_ack_cmd_frame(uint16_t dst_tei, uint8_t *report_ack, uint32_t report_ack_len);
//入网节点上报
void ble_mac_addr_report_process_task();
void ble_mac_init_addr_report();

//in_or_leave 0:leave 1:in net
bool ble_mac_addr_in_leave_net_ack_cb_fun(uint16_t fr_seq,uint16_t in_or_leave);
void ble_mac_addr_in_leave_net_report_init_task();
//入网离网上报任务
void ble_mac_addr_in_leave_net_report_task();
//===
uint8_t ble_mac_send_cfg_isp_proj_key_cmd_frame(uint8_t* to_mac,uint32_t isp_key, uint32_t proj_key);
#endif

#ifndef __BLE_LOCAL_H__
#define __BLE_LOCAL_H__
#include "stdint.h"
#include "ble_data.h"
#include "cmsis_compiler.h"
//#include "whi_pro.h"

//================
#define BLE_LOCAL_CONFIG_NET_INFO_CMD		(0x00)
#define BLE_LOCAL_READ_NET_INFO_CMD			(0x01)

#define BLE_LOCAL_RECEIVE_DATA_CMD			(0x10)
#define BLE_LOCAL_NEIGHBOR_DATA_CMD			(0x11)


#define BLE_LOCAL_SEND_DATA_CMD				(0x24)
#define BLE_LOCAL_LINK_SEND_DATA_CMD		(0x25)
#define BLE_LOCAL_SEND_SUCCESS_ACK_CMD		(0x26)

typedef struct __PACKED
{
	uint8_t cmd; //固定
	uint8_t data_len;//固定 0x00
	uint8_t frame_data[];
}ble_local_frame_type;

typedef struct __PACKED
{
	uint8_t data_type; //固定
	uint8_t src_mac[BLE_LONG_ADDR_SIZE]; 
	uint8_t rssi;
	uint8_t data_len;//固定 0x00
	uint8_t frame_data[];
}ble_local_receive_data_frame_type;

typedef struct __PACKED
{
	uint8_t neighbor_mac[BLE_LONG_ADDR_SIZE]; 
	uint8_t rssi;
	uint16_t net_id;
}ble_local_receive_neighbor_frame_type;

#define SEND_SHORT_ADDR_TYPE	(0)
#define SEND_LONG_ADDR_TYPE		(1)
#define SEND_APP_SEQ_NUM_TYPE	(2)

#define SEND_BROCAST_ADDR_VALUE			(0xFFFF)
#define SEND_LOCAL_BROCAST_ADDR_VALUE	(0xFFFE)

typedef struct __PACKED
{
	uint8_t send_type;
	union {
        uint16_t tei;
        uint8_t mac[BLE_LONG_ADDR_SIZE];
		uint16_t seq_num;
    } addr;
	uint8_t len;
	uint8_t frame_data[];
}ble_local_send_data_frame_type;


typedef struct __PACKED
{
	uint8_t link_time;
	uint8_t link_flg;
	uint8_t dst_mac[BLE_LONG_ADDR_SIZE]; 
	uint8_t len;
	uint8_t frame_data[];
}ble_local_link_send_data_frame_type;

typedef struct __PACKED
{
	uint16_t net_id;
	uint8_t mac_addr[BLE_LONG_ADDR_SIZE]; 
	uint8_t net_seq;
	uint8_t net_enable_flg;
	uint8_t net_mode;
	uint32_t isp_key;
	uint32_t proj_key;
	uint32_t date_tamp;
}ble_local_config_net_info_frame_type;

//==================

#define BLE_LOCAL_FRAME_START_HEAD_LEN					(sizeof(ble_local_frame_type))
#define BLE_LOCAL_SEND_FRAME_START_HEAD_LEN				(sizeof(ble_local_send_data_frame_type))
#define BLE_LOCAL_LINK_SEND_FRAME_START_HEAD_LEN		(sizeof(ble_local_link_send_data_frame_type))

#define BLE_LOCAL_DATA_OFF	 			(WHI_PRO_STAT_HEAD_SIZE + BLE_LOCAL_FRAME_START_HEAD_LEN)
#define BLE_LOCAL_SEND_DATA_OFF	 		(BLE_LOCAL_DATA_OFF + BLE_LOCAL_SEND_FRAME_START_HEAD_LEN)
#define BLE_LOCAL_LINK_SEND_DATA_OFF	(BLE_LOCAL_DATA_OFF + BLE_LOCAL_LINK_SEND_FRAME_START_HEAD_LEN)

#define BLE_LOCAL_FRAME_HEAD_LEN			(WHI_PRO_HEAD_SIZE + WHI_PRO_HEAD_SIZE)
#define BLE_LOCAL_SEND_FRAME_HEAD_LEN		(BLE_LOCAL_FRAME_HEAD_LEN + BLE_LOCAL_SEND_FRAME_START_HEAD_LEN)
#define BLE_LOCAL_LINK_SEND_FRAME_HEAD_LEN	(BLE_LOCAL_FRAME_HEAD_LEN + BLE_LOCAL_LINK_SEND_FRAME_START_HEAD_LEN)



void ble_local_run_process();
void ble_local_restart_net();



uint16_t ble_local_process_frame(uint8_t *p_frame,uint16_t frame_len);
uint16_t ble_local_process_frame_ex(uint8_t *p_frame,uint16_t frame_len, bool *p_need_ack);
uint16_t ble_local_process_ack_frame(uint8_t *p_frame,uint16_t frame_len,uint16_t frame_seq);



uint8_t * ble_local_malloc(uint32_t size);
void ble_local_free(void *ptr);
uint8_t *get_local_malloc_head_start(uint8_t *ptr);
//=====
bool ble_net_ack_send_data(uint8_t *ptr,uint16_t data_len);
bool ble_local_send_data(uint8_t *ptr,uint16_t data_len);

uint16_t ble_create_local_send_frame(uint8_t *ptr,uint8_t send_type,uint8_t *dst_addr,uint8_t data_len,uint8_t *frame_data);
uint16_t ble_create_local_link_send_frame(uint8_t *ptr,uint8_t *dst_addr,uint8_t layer,uint8_t data_len,uint8_t *frame_data);

//=======================================
#define BLE_NET_ACK_SEND_PLC_PORT			(0)
#define BLE_NET_ACK_SEND_DEUBG_UART_PORT	(1)
#define BLE_NET_ACK_SEND_BLE_UART_PORT		(2)

//
#define ACK_SEND_PORT_CHECK_TIME	5000 //5 seconds
extern TimeValue ble_net_ack_send_data_port_timer;

extern uint8_t ble_net_ack_send_data_port;
//0:plc,1:ble
uint8_t get_sta_in_net_src();
#endif



#ifndef __BEL_APL_H__
#define __BEL_APL_H__
#include "stdint.h"
#include "cmsis_compiler.h"
#include "ble_mac.h"

#define BEL_APL_LOCAL_CMD				(0x50)
#define BEL_APL_SYSTEM_CTRL_CMD			(0x51)
#define BEL_APL_SYSTEM_CTRL_TRUN_CMD			(0x81)
//==
#define BEL_APL_SYSTEM_CTRL_TRUN_LED_BRIGHTNESS_CMD		(0x01)
#define BEL_APL_SYSTEM_CTRL_TRUN_MODE_CMD				(0x02)


#define BEL_APL_VERSION	(0x01)

typedef struct __PACKED
{
	uint8_t ver;
	uint8_t cmd;
	uint8_t data_len;
	uint8_t frame_data[];
}ble_apl_frame_type;

#define TAG_LIST_MAX_LEN		(4)
typedef struct __PACKED
{
	uint8_t tag_oper:4;//WHI_TAG_AND_OPER,1 WHI_TAG_OR_OPER,2 WHI_TAG_INVERT_OPER
	uint8_t tag_sum:4;//max=4
	uint8_t tag_list[];
}ble_apl_tag_str_type;

typedef struct __PACKED
{
	uint16_t dev_addr;
	uint8_t cmd:7;
	uint8_t tag_flg:1;
	uint8_t frame_data[];
}ble_apl_system_trun_cmd_frame_type;

//typedef struct __PACKED
//{
//	uint8_t dst_mac[BLE_LONG_ADDR_SIZE];
//	uint16_t user_len;
//	uint8_t user_data[];
//}ble_apl_local_cmd_frame_type;


typedef struct __PACKED
{
	uint8_t dst_mac[BLE_LONG_ADDR_SIZE];
	uint16_t user_len;
	uint8_t user_data[];
}ble_apl_system_ctrl_cmd_frame_type;

	

uint16_t ble_apl_process_frame(ble_mac_frame_type *p_mac,ble_mac_path_type *p_mac_path,ble_mac_cmd_type *p_mac_cmd,uint16_t frame_len);


uint8_t ble_apl_send_system_ctrl_cmd_frame(uint8_t* to_mac,void *msg, uint32_t len,uint32_t flg);
uint8_t ble_apl_send_local_cmd_frame(uint8_t* to_mac,const void *msg, const uint32_t len,const uint32_t flg);
#endif


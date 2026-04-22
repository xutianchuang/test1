#ifndef __WHI_SYSCTRL_H__
#define __WHI_SYSCTRL_H__

#include <stdint.h>
#include  <gsmcu_hal.h>

#ifndef   __PACKED
  #define __PACKED                               __attribute__((packed))
#endif

//=======================================
#define WHI_SYSCTRL_GROUP_ADDRS_LEN		(59)
#define WHI_GROUP_TAG_LEN				(4)
typedef struct
{
	uint16_t short_dev_addr;
	
	uint16_t group_addr_sum;
	uint32_t group_addr[WHI_SYSCTRL_GROUP_ADDRS_LEN];
	
	uint8_t  group_tag_list[WHI_GROUP_TAG_LEN];
}whi_sysctrl_base_param_type;
//=======================================

#define USER_DATA_HEAD_LEN	(8)
#define MAC_LEN_HEAD_LEN	(8)

//
//错误代码
//0x00 成功
//0x01 无法解析此请求
//0x02 设备控制受限中
//0x03 属性不可读
//0x04 属性不可写
//0x05 参数值错误
//0x06 未定义
#define WHI_SYSCTRL_SUCCESS_CODE			(0x00)
#define WHI_SYSCTRL_NOREQ_CODE				(0x01)
#define WHI_SYSCTRL_DEV_LIMITED_CODE		(0x02)
#define WHI_SYSCTRL_ATTR_NOT_READ_CODE		(0x03)
#define WHI_SYSCTRL_ATTR_NOT_WRITE_CODE		(0x04)
#define WHI_SYSCTRL_PARAM_VALUE_ERROR_CODE	(0x05)
#define WHI_SYSCTRL_UNDEFINE_CODE			(0x06)
#define BROCAST_ADDR_VALID 1
#define BROCAST_ADDR_INVALID 0

//基本结构体
typedef struct __PACKED
{
	uint8_t mac_add[LC_NET_ADDR_SIZE];
	uint16_t user_len;
	uint16_t plc_pro;
	uint16_t seq_num;
	uint8_t fun_code;
	uint8_t stat_code;
	uint16_t dev_addr;
	uint8_t data_body[0];
}whi_sysctrl_type;

//===========================
typedef struct __PACKED
{
	uint8_t group_mode;
	uint8_t group_action;
	uint16_t group_addr;
	uint16_t group_dev_num;
	uint16_t dev_addrs[];
}whi_sysctrl_check_add_group_addrs_data_type;

typedef struct __PACKED
{
	uint16_t SIID;
	uint16_t CIID;
	uint8_t tag_sum;
	uint8_t tag_list[];
}whi_tag_operate_str_type;


#define OBJECT_MODE_HEAD_START_SIZE (sizeof(object_model_interface_t))

//
#define WHI_SYSCTRL_HEAD_START_SIZE sizeof(whi_sysctrl_type)
#define WHI_SYSCTRL_HEAD_USER_START_SIZE (sizeof(whi_sysctrl_type) - LC_NET_ADDR_SIZE - sizeof(uint16_t))


//fun code
#define WHI_SYSCTRL_ACK_MASK						(0x80)

#define WHI_SYSCTRL_READ_DEV_INFO		(0x01)

#define WHI_SYSCTRL_WRITE_DEV_ADDR		(0x02)
#define WHI_SYSCTRL_READ_DEV_ADDR		(0x03)

#define WHI_SYSCTRL_ADD_GROUP_ADDRS		(0x04)
#define WHI_SYSCTRL_READ_GROUP_ADDRS	(0x05)
#define WHI_SYSCTRL_DEL_GROUP_ADDRS		(0x06)

#define WHI_SYSCTRL_WRITE_DEV_ATTR		(0x07)
#define WHI_SYSCTRL_READ_DEV_ATTR		(0x08)

#define WHI_SYSCTRL_REPORT_DEV_ATTR		(0x09)
#define WHI_SYSCTRL_REPORT_DEV_EVENT				(0x0A)

#define WHI_SYSCTRL_CHECK_ADD_GROUP_ADDRS			(0x0B)

#define WHI_SYSCTRL_SET_DEV_SCENE					(0x0C)
#define WHI_SYSCTRL_READ_DEV_SCENE_CHECK_CODE		(0x0D)
#define WHI_SYSCTRL_EXECUTE_DEV_SCENE				(0x0E)
#define WHI_SYSCTRL_DEL_DEV_SCENE					(0x0F)

#define WHI_SYSCTRL_GET_HEART_HOP					(0x10)

#define WHI_SYSCTRL_RESET_DEV						(0x11)

#define WHI_SYSCTRL_TRUN_NEXT_DEV_DATA				(0x12)

#define WHI_SYSCTRL_READ_MEMORY_DATA				(0x70)
#define WHI_SYSCTRL_WRITE_MEMORY_DATA				(0x71)

#define WHI_SYSCTRL_READ_NEIGHBOR_INFO				(0x78)


uint16_t whi_sysctrl_process_frame(uint8_t *src_data,uint16_t src_len,uint8_t is_brocast_addr);
void whi_sysctrl_report_event_data(uint8_t *event_data, uint16_t event_len);
#endif
#ifndef __BLE_API_H__
#define __BLE_API_H__

#include <string.h>
#include "common_includes.h"
#include "hplc_data.h"
#include "applocal.h"
#include "bps_flash.h"
#include "lc_app.h"

#ifndef   __PACKED
  #define __PACKED                               __attribute__((packed))
#endif
/**
* @brief 地址长度
*/
#define LC_NET_ADDR_SIZE   6

/**
* @brief 常用数据类型
*/
// typedef   signed          char int8_t;
// typedef   signed short     int int16_t;
// typedef   signed           int int32_t;
// typedef   signed  long long  int64_t;
// typedef unsigned          char uint8_t;
// typedef unsigned short     int uint16_t;
// typedef unsigned           int uint32_t;
// typedef unsigned    long long uint64_t;

/**
* @brief 网络错误类型
*/
// typedef uint32_t lc_net_err_t;

#define LC_NET_EOK                          0               /**< 无错误 */
#define LC_NET_ERROR                        1               /**< 一般错误 */
#define LC_NET_ETIMEOUT                     2               /**< 超时错误 */
#define LC_NET_EFULL                        3               /**< 资源已满 */
#define LC_NET_EEMPTY                       4               /**< 资源已空  */
#define LC_NET_ENOMEM                       5               /**< 内存不足 */
#define LC_NET_ENOSYS                       6               /**< 无系统  */
#define LC_NET_EBUSY                        7               /**< 忙线中  */
#define LC_NET_EIO                          8               /**< IO 错误  */
#define LC_NET_EINTR                        9               /**<  中断系统调用  */
#define LC_NET_EINVAL                       10              /**< 无效参数  */

/**
* @brief 事件类型
*/
typedef uint32_t lc_net_event_t;

#define LC_NET_RX_DATA_EVENT           (1)               /**< 网络接收事件 参数 lc_net_comm_param_t */
#define LC_NET_STATE_CHGED_EVENT           (2)               /**< 网络状态改变事件 参数 lc_net_state_t */

/**
* @brief 网络版本信息类型
*/
typedef struct
{
    uint8_t boot_ver;
    uint8_t ver_time_year;  //0-99 (BCD 2000-2099)
    uint8_t ver_time_mon;  //1-12
    uint8_t ver_time_day;   //1-31
    uint16_t soft_ver;    //BCD
    uint16_t factory_code; //BCD
    uint16_t chip_code;  //BCD
	
	uint32_t net_ver;
    uint8_t net_ver_year;
    uint8_t net_ver_mon;
    uint8_t net_ver_day;
    uint8_t net_ver_hour;
    uint8_t net_ver_min;
    uint8_t net_ver_second;

}lc_net_ver_info_t;

typedef struct __PACKED
{
    uint32_t isp_key;
    uint32_t proj_key;
	uint32_t ex_param;
}ST_MODE_ID_INFO_TYPE;


#define MAX_BLE_BREAKERS_NUM	20
typedef struct
{
	uint8_t mac[LC_NET_ADDR_SIZE];
	uint8_t rsv[2];
}ble_breaker_addr_t;

typedef struct
{
	ble_breaker_addr_t breaker_addr[MAX_BLE_BREAKERS_NUM];
}all_ble_breakers_addr_t;


//获取出厂配置MAC地址
void lc_net_get_factory_mac(uint8_t *p_mac);
//设置出厂配置MAC地址
lc_net_err_t lc_net_set_factory_mac(uint8_t *p_mac,uint8_t len);
//获取版本配置
void lc_net_get_ver_info(lc_net_ver_info_t *p_ver);
uint32_t lc_net_get_factory_id_code(void);
uint32_t lc_net_get_project_id_code(void);
bool lc_net_write_factory_id_code(uint32_t code);
bool lc_net_write_project_id_code(uint32_t code);
//
uint32_t lc_net_get_ex_param_code(void);
bool lc_net_write_ex_param_code(uint32_t param);
bool lc_net_get_neighbor_some_info(uint16_t index, uint16_t *succ_rate);


void lc_net_get_ble_breaker_addr(uint8_t *buf,uint32_t *len);
bool lc_net_write_ble_breaker_addr(uint8_t *buf,uint32_t len);

//================================
//网络管理
//网络版本信息类型
typedef enum
{
    LC_NET_OUT_STATE = 1,            //未入网
    LC_NET_IN_STATE = 2,                //已入网
}lc_net_state_t;
//节点网络角色类型
typedef enum
{
    LC_NET_STA_ROLE = 1,
    LC_NET_PCO_ROLE = 2,
    LC_NET_CCO_ROLE = 4,
}lc_net_role_t;
//本节点基本网络信息结构类型
typedef struct
{
    lc_net_state_t net_state;
    uint8_t addr[LC_NET_ADDR_SIZE];
    uint8_t cco_addr[LC_NET_ADDR_SIZE];
    uint16_t tei;
    uint16_t pco_tei;
    lc_net_role_t role;
    uint32_t nid;
    uint32_t  level;
	uint32_t freq_band;
	uint32_t reset_time;
	uint8_t rf_option;
	uint8_t rf_seq;
}lc_net_my_info_t;

//拓扑节点网络信息结构类型
typedef struct __PACKED
{
	uint8_t mac[LC_NET_ADDR_SIZE]; //节点mac地址
	uint16_t tei;//本节点TEI  1-1000；0：未入网无TEI
	uint16_t pco_tei;//父节点TEI：CCO父节点为0;
	uint8_t level:4;//0-15 层级
	uint8_t  role:4;//sta pco cco 
	uint8_t  add_check_flg:1;//sta pco cco 
	uint8_t  red:7;//sta pco cco 
}lc_net_topo_node_info_t;
//获取网络状态
// lc_net_state_t lc_net_get_net_state();
//设置自己的网络通信地址
lc_net_err_t lc_net_set_addr(const uint8_t *p_addr,const uint8_t addr_len);
//获取当前节点基本信息
void lc_net_get_net_self_info(lc_net_my_info_t *p_info);
//获取网络拓扑节点总数
uint32_t lc_net_get_topo_node_count();
// //获取网络拓扑节点
uint32_t lc_net_get_topo_nodes(lc_net_topo_node_info_t *p_nodes,const uint32_t offset, const uint32_t count);
//启动网络
void lc_net_start_net();
//停止网络
void lc_net_stop_net();

//获取设备类型
uint16_t lc_net_get_device_type();
//=====================================
//数据收发
#define LC_NET_NOT_BLOCK		(0x00)
#define LC_NET_BLOCK			(0x01)

#define LEN_APP_PACKET 12
/*
//发送网络数据
lc_net_err_t lc_net_tx_data(const uint8_t* to_mac,const void *msg, const uint32_t len,const uint32_t flg);
//应用接收网络数据
uint32_t lc_net_rx_data(uint8_t* from_mac,uint8_t *buff, uint32_t len,uint32_t flg);
//设置接收回调
lc_net_err_t lc_net_set_rx_data_evnet_cb_fun(void (*lc_net_rx_data_cb)(const uint8_t* from_mac,const uint8_t *buff, const uint32_t len));
*/
bool lc_net_is_in_net();
//=====================================
//文件升级
typedef enum
{
	LC_FILE_INIT = 0,
	LC_FILE_UNKNOW = 1,
	LC_FILE_WRITING = 2,
	LC_FILE_FINISH = 3,
}lc_file_state_type;
//网络文件类型
typedef enum
{
    LC_NET_LOCAL_FILE = 1,  //本地升级文件
    LC_NET_ROMOTE_FILE = 2,//远程升级文件
}lc_net_file_type_t;

typedef struct
{
	uint32_t id; //文件标识ID
	uint32_t size;//文件大小
	uint32_t crc;//文件CRC16
	lc_net_file_type_t type; //文件类型 1:本地文件，2:远程文件
	uint16_t seg_cnt;//传输文件总段数
}lc_net_file_info_t;
// //清空文件存储区
void lc_net_clear_file();
// //配置升级文件信息
void lc_net_config_file(lc_net_file_info_t *p_file);
// //写入升级文件段
lc_net_err_t lc_net_write_file_seg(uint32_t seg_offset,uint32_t seg_crc,uint8_t *seg_data,uint32_t seg_len);
void lc_file_set_states_cbfun(void(* state_cbfun)(lc_file_state_type),uint32_t(* check_cbfun)());
uint32_t lc_net_read_file_addr(void);
//=================
//数据存储区
#define DATA_APP_PARA_STORE_MAX_SIZE	(4096*1)
extern bool LocalStartUpdataFileProcess(uint32_t id,uint32_t size,uint32_t crc,uint32_t type,uint32_t seg_cnt);
extern bool LocalUpdataFileSegmetProcess(uint32_t seg_offset,uint32_t seg_crc,uint8_t *seg_data,uint32_t seg_len);
lc_net_err_t lc_store_clear(uint32_t start_addr,uint32_t size);
//写入存储区
lc_net_err_t lc_store_write(uint32_t start_addr,uint8_t *p_buff,uint32_t len);
//读取存储区
lc_net_err_t lc_store_read(uint32_t start_addr,uint8_t *p_buff,uint32_t len);
//key
bool ble_key_info_stored();
bool ble_read_key_info(void);
void ble_init_key_info_from_store();

bool lc_ble_breaker_addr_stored();
bool lc_read_ble_breaker_addr(void);
void lc_init_ble_breaker_addr_from_store();


// //=================
bool lc_plc_mac_addr_in_net_report_send(uint16_t node_sum,uint8_t *addr_list,uint16_t fr_seq);
bool lc_plc_mac_addr_leave_net_report_send(uint16_t node_sum,uint8_t *addr_list,uint16_t fr_seq);
//in_or_leave 0:leave 1:in net
// bool lc_plc_mac_addr_in_leave_net_ack_cb_fun(uint16_t fr_seq,uint16_t in_or_leave);

//==================
//节点同步
bool lc_set_net_node_sync_flg(bool flg);
uint32_t lc_get_net_node_sync_ntb();
//===
typedef uint16_t (*ctrl_debug_process_cb_type)(uint8_t* psrc,uint16_t d_len,uint8_t* pdst);
//===
lc_net_err_t lc_net_reboot_system(const uint32_t delay_ms);
//
uint16_t lc_plc_self_info_process(uint8_t* psrc,uint16_t d_len,uint8_t* pdst);
//====================================
//内存管理
#define MAX_BLE_DATA_SIZE     (4)
#define MAX_RECEIVE_DATA_NUM  (1)
#pragma pack(4)

typedef struct {
	uint8_t data[MAX_BLE_DATA_SIZE];
}Ble_Mem_Union;

void ble_molloc_init();
//分配一个ble black
Ble_Mem_Union* BleMalloc(uint32_t size);
//释放一个ble black
void BleFree(Ble_Mem_Union* pdu);

#endif

#ifndef __BLE_UPDATE_H__
#define __BLE_UPDATE_H__

#include <string.h>
#include "common_includes.h"
#include "lc_whi_adapt.h"
#include "crclib.h"

#define LAMP_BLE_SEND_DEFAULT_POWER (6)
#define LAMP_BLE_POWER_MAXDBM (10)

#define LAMP_BLE_CONNECT_ERROR_STATUS (0x01)
#define LAMP_BLE_CONNECT_SUCCESS_STATUS (0x00)

#define LAMP_BLE_VER_VALUE_LEN (8)
#define LAMP_BLE_RESET_INFO_LEN (64)

// senser protocol 
#define	LC_ADV_ILLUM_SENSOR_TYPE    0xC1
#define	LC_ADV_ILLUM_CTRL_TYPE      0xC2

#define	LC_ADV_SWITCH_TYPE   0xC3

typedef struct __PACKED
{
    // HEAD:14Bytes
    uint8_t len;
    uint8_t type;
    uint8_t mac[6];
    uint16_t seq;
    uint16_t ver:4;
    uint16_t ttl:2;
    uint16_t batt:5;
    uint16_t rsv:5;
    uint16_t version;

    // SENSOR:21Bytes
    uint8_t data[0];
} sensor_msg_t;

typedef struct  {
    uint16_t manufacturer_code;   // 厂商代码，2字节
    uint16_t chip_type;           // 芯片类型，2字节
    uint16_t software_version;    // 软件版本号，2字节
    uint16_t reserved;            // 保留，2字节
}lamp_module_version_t;

typedef struct
{
    uint8_t ble_status;
    uint8_t updata_status;
    uint8_t ble_connect_status;
    uint16_t set_mac_seq;
	uint16_t device_type;

    uint8_t ble_mac_addr[LC_NET_ADDR_SIZE];
    uint8_t ble_sys_reset_info[LAMP_BLE_RESET_INFO_LEN];

    lamp_module_version_t ble_ver_value;
    
    TimeValue ble_check_connect_timer;
    TimeValue ble_heart_connect_timer;
    TimeValue ble_read_version_connect_timer;

} lamp_ble_param_t;



// 参数配置
typedef struct __PACKED
{
    uint8_t radar_type;
    uint8_t radar_count;
    uint32_t avg_energy;
    uint8_t rsv[26];
}lcip_radar_report_t;


typedef struct __PACKED
{
    uint8_t mac[6];
    uint8_t data_len;
    uint8_t data[25];
}lcip_sensor_report_t;

typedef struct __PACKED
{
    uint8_t linkage_num;
    uint8_t rsv[31];
}local_linkage_param_t;

typedef struct __PACKED
{
    uint8_t state;
    uint8_t freq;
    uint8_t check;
    uint16_t thr;
    uint16_t energy_scale;
    uint8_t rsv1[25];
}local_radar_param_t;

typedef struct __PACKED 
{
    uint8_t power;
    uint8_t rsv[31];
}local_ble_param_t;

typedef struct __PACKED
{
    u8 type;
    u32 func;
    u8 power;
    s8 recv_rssi;
    s8 send_rssi;
    u8 uid[16];
}factory_ble_test_t;

typedef struct  __PACKED
{
    factory_ble_test_t ble;
    uint8_t rsv[8];
}local_ble_factory_test_param_t;


void lamp_ble_init();
void lamp_ble_task();

void lamp_ble_power_set(uint8_t ble_power);
// void lamp_ble_radar_param_set(uint8_t freq, uint16_t thr, uint16_t scale);
// void lamp_ble_radar_param_read();

bool lamp_ble_set_mac_addr(uint8_t *mac_addr);
bool lamp_ble_set_mac_addr_cb_fun(uint16_t ack_seq, uint8_t result);

bool lamp_ble_updata_send_ack_chekc_frame(uint32_t seq, uint32_t status);

void lamp_ble_start_read_version();
bool lamp_ble_start_read_version_cb_fun(uint16_t ack_seq, uint8_t result, uint8_t *version, uint16_t ver_len);
bool lamp_ble_start_read_reset_info_cb_fun(uint16_t ack_seq, uint8_t result, uint8_t *info, uint16_t info_len);
// bool lamp_ble_radar_param_version_cb_fun(uint16_t ack_seq, uint8_t result, uint8_t *data, uint16_t len);
bool lamp_ble_device_type_info_cb_fun(uint16_t ack_seq, uint8_t result, uint8_t *info, uint16_t info_len);

extern lamp_ble_param_t lamp_ble_param;

#endif

#ifndef _BLE_EVENT_H_
#define _BLE_EVENT_H_

#include "stdint.h"
#include <stdbool.h>
#define EMASK(e) e##_BIT_MASK
#define BIT(n)                  		( 1<<(n) )
enum
{
    EVENT_RADAR_CONN_FAIL = 0x01,
    EVENT_RADAR_COUNT = 0x02,
    EVENT_RADAR_WARN = 0x03,
    EVENT_FREEZE_POWER_REPORT = 0x04,
    EVENT_SENSOR_BATTERY_REPORT = 0x05,
    EVENT_REJECT_IN_NET_REPORT = 0x06,
    EVENT_IN_NET_REPORT = 0x07,
    EVENT_BLE_VER_ERR = 0x08,
    EVENT_NODE_NETSTAT = 0x09,
    EVENT_NODE_NETSTAT_D7 = 0x0A,
    EVENT_BLE_CONN_FAIL = 0x0B
};

enum
{
    EMASK(EVENT_RADAR_CONN_FAIL) = 0,
    EMASK(EVENT_RADAR_COUNT),
    EMASK(EVENT_RADAR_WARN),
    EMASK(EVENT_FREEZE_POWER_REPORT),
    EMASK(EVENT_SENSOR_BATTERY_REPORT),
    EMASK(EVENT_REJECT_IN_NET_REPORT),
    EMASK(EVENT_IN_NET_REPORT),
    EMASK(EVENT_BLE_VER_ERR),
    EMASK(EVENT_NODE_NETSTAT),
    EMASK(EVENT_NODE_NETSTAT_D7),
    EMASK(EVENT_BLE_CONN_FAIL)
};

// 默认开启上报
#define ALWAYS_ENABLE_MASK  \
    BIT(EMASK(EVENT_IN_NET_REPORT))         |   \
    BIT(EMASK(EVENT_REJECT_IN_NET_REPORT)) 

#define MAX_EVENT_VALUE_NUM 2 

#define MAX_EVENT_REPORT_BYTES  (sizeof(lamp_event_report_t) + MAX_EVENT_VALUE_NUM * sizeof(uint32_t))
#define GET_EVENT_REPORT_BYTES(p)  (sizeof(lamp_event_report_t) + ((p)->value_num) * sizeof(uint32_t))

typedef struct __PACKED
{
    uint8_t mac[6];
    uint16_t code;
    uint32_t value[0];
}lamp_event_report_t;


typedef struct 
{
    uint32_t report_time;
    uint32_t create_time;
    uint16_t value_num;
    uint16_t event_code;
    uint8_t mac[6];
    uint32_t event_value[0];  
} event_item_t;

void lamp_event_report_add(uint16_t code, uint8_t * mac, uint32_t code_mask, ...); 
void lamp_event_task(void);
void lamp_event_recv_callback(uint8_t *data, uint32_t len);
void lamp_event_list_reset();

#define lamp_event_add(code, mac,...) lamp_event_report_add(code, mac, BIT(EMASK(code)), ##__VA_ARGS__);

void LampEventAddInNetReport(uint8_t * mac_addr);

extern uint8_t event_all_enable;

bool ble_put_report_event_all_enable(uint8_t enable);
#endif

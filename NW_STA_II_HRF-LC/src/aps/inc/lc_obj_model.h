#ifndef __LC_OBJ_MODEL_H__
#define __LC_OBJ_MODEL_H__

#include "stdint.h"
#include  <gsmcu_hal.h>


#define READ_U32(p)     (((((char *)(p))[0] & 0xFF) << 0)  | \
                         ((((char *)(p))[1] & 0xFF) << 8)  | \
                         ((((char *)(p))[2] & 0xFF) << 16) | \
                         ((((char *)(p))[3] & 0xFF) << 24))

#define WRITE_U32(p, v) (((char *)(p))[0] = (((v) >> 0) & 0xFF),  \
                         ((char *)(p))[1] = (((v) >> 8) & 0xFF),  \
                         ((char *)(p))[2] = (((v) >> 16) & 0xFF), \
                         ((char *)(p))[3] = (((v) >> 24) & 0xFF))


//==================================

/**
 * @author    ginvc
 * @dateTime  2022-10-21
 * @attention 双模中继器物模型信息    
 */
#define SIID_RELAY_POWER (0x8401)
#define SIID_RELAY_SYS_INFO  (0x8402)
#define SIID_RELAY_NET_INFO  (0x8403)
//===============
#define CIID_BAND_POWER (0x6401)
#define CIID_PLC_POWER_MODE (0x6402)
#define CIID_OPT_POWER  (0x6403)
#define CIID_RF_POWER_MODE (0x6404)
#define CIID_RUNNING_TIME_S (0x6405)
#define CIID_HARD_RESET_TIMES (0x6406)
#define CIID_SOFT_RESET_TIMES (0x6407)
#define CIID_SYS_INFO_ALL (0x6408)
#define CIID_LAST_RESET_REASON (0x6409)
#define CIID_LAST_OFFLINE_REASON (0x6410)
#define CIID_NET_PERIOD_INFO (0x6411)
#define CIID_NET_BASE_INFO (0x6412)
#define CIID_NET_BASE_INFO_STR (0x6413)


#define SIID_FACTORY_TEST (0x8389)
#define CIID_FACTORY_BLE_RSSI (0x65A2)
#define CIID_FACTORY_RADAR_TEST_ENABLE (0x65A1)
#define CIID_FACTORY_PHOTODIODE_TEST_STATUS (0x65A7)
#define CIID_FACTORY_PHOTODIODE_DATA (0x65A6)
#define CIID_FACTORY_RADAR_ENERGY (0x65A3)
#define CIID_FACTORY_CURRENT_PHOTODIODE_VOLTAGE (0x65A8)
#define CIID_FACTORY_TEST_HISTORY_PHOTODIODE_VOLTAGE (0x65A9)
#define CIID_FACTORY_TEST_PHOTODIODE_SAMPLE (0x65AA)





//===============
#define SIID_LAMP (0x8280)
#define SIID_NODE_VERSION (0x8009)
#define SIID_LAMP_TEMP (0x8010)
#define SIID_LAMP_VOLTAGE (0x8000)
#define SIID_LAMP_AMPERAGE (0x8001)
#define SIID_LAMP_POWER (0x8002)
#define SIID_LAMP_POWER_COS (0x8003)
#define SIID_LAMP_RUN_TIME (0x8004)
#define SIID_LAMP_FAULT (0x8005)
#define SIID_LAMP_CONFIG_TIME (0x8006)
#define SIID_LAMP_E_QUANTITY (0x8008)

#define SIID_LAMP_LED (0x8201)
#define SIID_LAMP_RADAR_SENSOR (0x8202)
#define SIID_LAMP_BLE_MODE (0x8203)
#define SIID_LAMP_DATE (0x8204)
#define SIID_LAMP_RADAR_ADJUST (0x8205)
#define SIID_LAMP_DEBUG (0x82D0)
#define SIID_LAMP_OPEN_LED (0x8281)
#define SIID_LAMP_BLINK_LED (0x8282)
#define SIID_LAMP_SENSE_LED (0x8283)
#define SIID_LAMP_ILLUM_LED (0x8284)
#define SIID_LAMP_SENSE_LINK_LED (0x8284)
#define SIID_LAMP_SENSE_LINK_GROUP_LED (0x8285)
#define SIID_LAMP_SENSE_LINK_LEARN_LED (0x8286)
#define SIID_LAMP_SWITCH_LED (0x8287)
#define SIID_LAMP_SENSOR_LIST (0x8288)

#define SIID_ISP_PROJ_KEY (0x8F01)
#define SIID_GATEWAY_PARAM (0x8F02)
#define SIID_NODE_REPORT_ONLINE (0x8F03)

#define SIID_NET_BLE_NEIGHBOR_INFO (0x8F08)
#define SIID_NET_BLE_LINK_NEIGHBOR_INFO (0x8F09)
#define SIID_NET_PLC_NEIGHBOR_INFO (0x8F0A)

#define SIID_TAG_LIST_OBJ (0x8F10)
#define SIID_TAG_OPERATE_OBJ (0x8F11)
#define SIID_KEY_SWITCH (0x8F12)

#define SIID_CRASH_LOG (0x8F16)

#define SIID_MAC_LIST (0x8F21)

#define SIID_CURTAINS_CONTROL (0x8E01)
#define CIID_KEY_SWITCH						(0x6F12)

#ifdef PLAT_HPLC_SWITCH
#define SIID_TAG_POWER_SWITCH_OBJ (0x03E9)
#endif

#ifdef PLAT_II_485_READ_METER
#define SIID_TAG_II_READ_METER_OBJ (0x03EA)
#endif
#define SIID_NET_NEIGHBOR_LIST (0x8404)
#define SIID_NET_NEIGHBOR_NUM  (0x8405)
#define SIID_NET_NEIGHBOR_TEI  (0x8406)
#define SIID_BROADCAST_MSG_TEST  (0x8407)

#define SIID_II_COLLECTOR_READ_METER_OBJ 	(0x03EA)

#define SIID_GET_PLC_NEIGHBOR_INFO_OBJ 	(0x03ED)
#define SIID_GET_RF_NEIGHBOR_INFO_OBJ 	(0x03EE)
#define SIID_GET_PCO_INFO_OBJ 	(0x03EF)

#define CIID_II_SEARCH_METER_OBJ 			(0x3EA2)

//===============
#define CIID_II_COLLECTOR_METER_MAC_OBJ 	(0x3EA0)
#define CIID_TAG_LIST_OBJ (0x7F01)
#define CIID_GROUP_FUNC_ADDR_OBJ (0x7F05)
#define CIID_TAG_OPERATE_OBJ (0x8F01)

#define CIID_LAMP_CURRENT_MODE (0x6202)
#define CIID_LAMP_MODE (0x6203)
#define CIID_LAMP_CURRENT_BRIGHTNESS (0x6267)

#define CIID_SOFT_VERSION (0x6001)
#define CIID_HARD_VERSION (0x6002)
#define CIID_DEVICE_TYPE (0x6103)

#define CIID_LAMP_CURRENT_UINT_VALUE (0x7001)

#define CIID_LAMP_RUN_TIME (0x6050)
#define CIID_LAMP_FAULT_TIME (0x6051)
#define CIID_LAMP_CURRENT_RUN_TIME (0x60F0)
#define CIID_LAMP_RESET_CNT (0x60F1)

#define CIID_LAMP_CONFIG_REPORT_WAITE_TIME (0x6020)
#define CIID_LAMP_CONFIG_REPORT_ALL_ENABLE (0x6030)
#define CIID_LAMP_CONFIG_REPORT_BIT_MASK (0x6031)

#define CIID_LAMP_E_QUANTITY_WH_UINT_VALUE (0x7030)
#define CIID_LAMP_E_FREEZE_VALUE (0x7031)
#define CIID_LAMP_E_QUANTITY_WS_UINT_VALUE (0x7040)

#define CIID_LAMP_DEBUG_LED_BRIGHTNESS (0x6207)
#define CIID_LAMP_DEBUG_MODE_DURATION (0x6268)

#define CIID_LAMP_RADAR_STATUS (0x6210)
#define CIID_LAMP_RADAR_DET_FREQ (0x6211)
#define CIID_LAMP_RADAR_DET_THR (0x6212)
#define CIID_LAMP_RADAR_DET_DEBUG (0x6213)
#define CIID_LAMP_RADAR_DET_SCALE (0x6214)
#define CIID_LAMP_RADAR_TRI_BLINK_LED_FLG (0x6050)

#define CIID_LAMP_RADAR_SELF_TRI_TIME (0x6254)
#define CIID_LAMP_RADAR_LINKAGE_TRI_TIME (0x6255)
#define CIID_LAMP_RADAR_RAW_TRI_COUNT (0x6256)
#define CIID_LAMP_RADAR_AVG_ENERGY (0x6257)

#define CIID_LAMP_BLE_MODE_SEND_POWER (0x6240)
#define CIID_LAMP_BLE_MODE_SYS_RESET_INFO (0x6241)
#define CIID_LAMP_BLE_MODULE_DEBUG_INFO (0x6003)

#define CIID_LAMP_DATE_TIMESTAMP (0x623B)
#define CIID_LAMP_DATE_DATESTAMP (0x623C)

#define CIID_LAMP_OPEN_LED_BRIGHTNESS (0x6207)

#define CIID_LAMP_BLINK_LED_BRIGHTNESS (0x6207)
#define CIID_LAMP_BLINK_LED_OPEN_TIME (0x620A)
#define CIID_LAMP_BLINK_LED_CLOSE_TIME (0x620B)


#define CIID_LAMP_SENSE_LED_CURRENT_STRATEGY (0x6250)
#define CIID_LAMP_SENSE_LED_HAVE_PEOPLE_RUN_TIME (0x6251)
#define CIID_LAMP_SENSE_LED_NOT_PEOPLE_RUN_TIME (0x6252)
#define CIID_LAMP_SENSE_LED_TRI_TIME (0x6253)
#define CIID_LAMP_SENSE_OPEN_RATE_TIME (0x6301)
#define CIID_LAMP_SENSE_CLOSE_RATE_TIME (0x6302)
#define CIID_LAMP_SENSE_ILLUM_LUX_UPPER (0x6303)
#define CIID_LAMP_SENSE_ILLUM_LUX_LOWER (0x6304)

#define CIID_LAMP_SENSE_STRATEGY_DEFAULT    (0x6260)
#define CIID_LAMP_SENSE_STRATEGY_TIME       (0x6261)
#define CIID_LAMP_SENSE_TEMPORARY_STRATEGY  (0x6300)

#define CIID_LAMP_ILLUM_BRIGHTNESS (0x6207)
#define CIID_LAMP_ILLUM_LUX (0x6208)
#define CIID_LAMP_ILLUM_RATIO (0x6209)
#define CIID_LAMP_ILLUM_BRIGHTNESS_MAXIMUM (0x620C)
#define CIID_LAMP_ILLUM_BRIGHTNESS_MINIMUM (0x620D)
#define CIID_LAMP_ILLUM_SENSOR_LUX (0x620E)
#define CIID_LAMP_ILLUM_LIGHT_DURATION (0x620F)
#define CIID_LAMP_ILLUM_MODE_DURATION (0x6220)

#define CIID_LAMP_SWITCH_BATT (0x6402)
#define CIID_LAMP_SWITCH_OPEN_BRIGHTNESS (0x6403)
#define CIID_LAMP_SWITCH_STEP (0x6404)

#define CIID_LAMP_ILLUM_SENSOR_MAC (0x6501)
#define CIID_LAMP_SWITCH_MAC_1 (0x6502)
#define CIID_LAMP_SWITCH_MAC_2 (0x6503)
#define CIID_LAMP_SWITCH_MAC_3 (0x6504)

#define CIID_ISP_KEY (0x6F01)
#define CIID_PROJ_KEY (0x6F02)
#define CIID_NODE_REPORT_ONLINE (0x6F03)

#define CIID_GATEWAY_NET_ENABLE_FLG				(0x6F08)
#define CIID_UP_NET_BASE_INFO					(0x6F10)
#define CIID_DOWN_NET_BASE_INFO					(0x6F11)
#define CIID_SET_DOWN_NET_BASE_INFO					(0x6F81)

#ifdef PLAT_HPLC_SWITCH
#define CIID_SWITCH_ALL_CHANNEL				(0x0000)
#define CIID_SWITCH_CHANNEL_1				(0x0001)
#define CIID_SWITCH_CHANNEL_16				(0x0010)
#define CIID_SWITCH_CHANNEL_COUNT			(0x1000)
#define CIID_SWITCH_ALL_CHANNEL_TIME_SLOT	(0xF000)
#define CIID_SWITCH_CHANNEL_1_TIME_SLOT		(0xF001)
#define CIID_SWITCH_CHANNEL_16_TIME_SLOT	(0xF010)
#endif

#ifdef PLAT_II_485_READ_METER
#define CIID_TAG_II_METER_MAC_OBJ 			(0x3EA0)
#define CIID_TAG_II_METER_RESET_OBJ 		(0x3EA1)
#endif

#define SIID_TAG_BLE_BREAKER_CONFIG_OBJ 	(0x03EB)
#define CIID_TAG_CONFIG_MAC_OBJ 			(0x3EB0)
#define CIID_TAG_REPEATER_RESET_OBJ			(0x3EB1)

#define SIID_BLE_STA_MODE 0x03EC

//==================
#define INT_TYPE (0x0001)
#define BOOL_TYPE (0x0002)
#define STRING_TYPE (0x0003)
#define ENUM_TYPE (0x0004)
#define ARRY_TYPE (0x0005)
#define FLOAT_TYPE (0xC006)
#define UINT_TYPE (0xC007)
#define STRUCT_TYPE (0xC008)
#define UINT16_TYPE (0xC009)

#define INT_SIZE (0x0004)
#define BOOL_SIZE (0x0001)
#define STRING_SIZE (0xFFFF)
#define ENUM_SIZE (0x0001)
#define ARRY_SIZE (0xFFFF)
#define FLOAT_SIZE (0x0004)
#define UINT_SIZE (0x0004)
#define STRUCT_SIZE (0xFFFF)
#define UINT16_SIZE (0x0002)

// 物模型
typedef struct __PACKED
{
    uint16_t SIID;
    uint16_t CIID;
    uint16_t data_type;
    uint16_t data_len;
    uint8_t data_buff[];
} object_model_interface_t;

#define OBJECT_MODE_HEAD_START_SIZE (sizeof(object_model_interface_t))

//====================
void obj_mode_init();
bool obj_mode_put_attr_value(object_model_interface_t *p_obj_mode);
bool obj_mode_get_attr_value(object_model_interface_t *p_obj_mode, uint16_t max_len);


void set_obj_model_bool_value(object_model_interface_t *obj, u8 value);
void set_obj_model_enum_value(object_model_interface_t *obj, u8 value);
void set_obj_model_uint_value(object_model_interface_t *obj, uint32_t value);
void set_obj_model_array_value(object_model_interface_t *obj, u8 *value, u32 len);
void set_obj_model_string_value(object_model_interface_t *obj, char *str);
bool get_obj_model_bool_value(object_model_interface_t *obj, u8 *value);
bool get_obj_model_enum_value(object_model_interface_t *obj, u8 *value);
bool get_obj_model_uint_value(object_model_interface_t *obj, uint32_t *value);
bool get_obj_model_array_value(object_model_interface_t *obj, u8 *value, u32 len);

#endif

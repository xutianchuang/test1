#include <stdlib.h>
#include <string.h>
#include "lc_all.h"
#include "ble_data.h"
#include "ble_event.h"
#include "ble_update.h"
#include "ble_api.h"
#include "ble_net.h"
#include "ble_local.h"
#include "bps_flash.h"
#include "hplc_app.h"
void set_obj_model_bool_value(object_model_interface_t *obj, u8 value)
{
    obj->data_type = BOOL_TYPE;
    obj->data_len = BOOL_SIZE;
    obj->data_buff[0] = value;
}

void set_obj_model_enum_value(object_model_interface_t *obj, u8 value)
{
    obj->data_type = ENUM_TYPE;
    obj->data_len = ENUM_SIZE;
    obj->data_buff[0] = value;
}

void set_obj_model_uint_value(object_model_interface_t *obj, uint32_t value)
{
    obj->data_type = UINT_TYPE;
    obj->data_len = UINT_SIZE;

    WRITE_U32(obj->data_buff, value);
}

void set_obj_model_array_value(object_model_interface_t *obj, u8 *value, u32 len)
{
    obj->data_type = ARRY_TYPE;
    obj->data_len = len;
    memcpy(obj->data_buff, value, len);
}

void set_obj_model_string_value(object_model_interface_t *obj, char *str)
{
    u32 idx = 0;
    while (*str != NULL)
    {
        obj->data_buff[idx] = *str;
        str++;
        idx++;
    }
    obj->data_type = STRING_TYPE;
    obj->data_len = idx;
}

bool get_obj_model_bool_value(object_model_interface_t *obj, u8 *value)
{
    if ((obj->data_len >= BOOL_SIZE) && (obj->data_type == BOOL_TYPE))
    {
        *value = obj->data_buff[0];
        return true;
    }
    return false;
}

bool get_obj_model_enum_value(object_model_interface_t *obj, u8 *value)
{
    if ((obj->data_len >= ENUM_SIZE) && (obj->data_type == ENUM_TYPE))
    {
        *value = obj->data_buff[0];
        return true;
    }
    return false;
}

bool get_obj_model_uint_value(object_model_interface_t *obj, uint32_t *value)
{
    if ((obj->data_len >= UINT_SIZE) && (obj->data_type == UINT_TYPE))
    {
        *value = READ_U32(obj->data_buff);
        return true;
    }
    return false;
}

bool get_obj_model_array_value(object_model_interface_t *obj, u8 *value, u32 len)
{
    if ((obj->data_len >= len) && (obj->data_type == ARRY_TYPE))
    {
        memcpy(value, obj->data_buff, obj->data_len);
        return true;
    }
    return false;
}

uint16_t lamp_get_soft_version(uint8_t *version)
{
    uint16_t bufflen = 0;
    bufflen += sprintf((char *)version + bufflen, "%04x-%04x-%04x-%04x-%s %s", 0xE620, 0, LC_APP_VER, 0, __DATE__, __TIME__);
    return bufflen;
}


/**
 * @brief 模块软件版本读取
 *
 * @param p_obj_mode
 * @return true
 * @return false
 */
bool obj_model_lamp_node_soft_version_get(object_model_interface_t *p_obj_mode)
{
    p_obj_mode->data_len = lamp_get_soft_version(p_obj_mode->data_buff);
    p_obj_mode->data_type = STRING_TYPE;
    return true;
}



/**
 * @brief 双模中继器PLC功率读写
 *
 * @param p_obj_mode
 * @return true
 * @return false
 */


bool obj_model_plc_power_get(object_model_interface_t *p_obj_mode)
{
    ReadStaFixPara();
    p_obj_mode->data_len = 4;
    memcpy(p_obj_mode->data_buff,StaFixPara.txPower,p_obj_mode->data_len);
    p_obj_mode->data_type = STRUCT_TYPE;
    return true;
}


bool obj_model_plc_power_put(object_model_interface_t *p_obj_mode)
{
    bool result = LcPlcPowerCheckAndSet(p_obj_mode->data_buff);
    return result;    
}



/**
 * @brief 双模中继器RF功率读写
 *
 * @param p_obj_mode
 * @return true
 * @return false
 */


bool obj_model_rf_power_get(object_model_interface_t *p_obj_mode)
{
    ReadStaFixPara();
    p_obj_mode->data_len = 2;
    memcpy(p_obj_mode->data_buff,StaFixPara.hrfTxPower,p_obj_mode->data_len);
    p_obj_mode->data_type = STRUCT_TYPE;
    return true;
}


bool obj_model_rf_power_put(object_model_interface_t *p_obj_mode)
{
    bool result = LcRfPowerCheckAndSet(p_obj_mode->data_buff);
    return result;    
    
}



/**
 * @brief 双模中继器PLC功率模式读取
 *
 * @param p_obj_mode
 * @return true
 * @return false
 */

bool obj_model_plc_mode_get(object_model_interface_t *p_obj_mode)
{
    ReadStaFixPara();
    p_obj_mode->data_len = 1;
    p_obj_mode->data_buff[0] = StaFixPara.txPowerMode[0];
    p_obj_mode->data_type = ENUM_TYPE;
    return true;
}


/**
 * @brief 双模中继器RF功率读写
 *
 * @param p_obj_mode
 * @return true
 * @return false
 */

bool obj_model_rf_mode_get(object_model_interface_t *p_obj_mode)
{
    ReadStaFixPara();
    p_obj_mode->data_len = 1;
    p_obj_mode->data_buff[0] = StaFixPara.txPowerMode[1];
    p_obj_mode->data_type = ENUM_TYPE;
    return true;
}






/**
 * @brief 读取系统信息
 *
 * @param p_obj_mode
 * @return true
 * @return false
 */

u32 getRunTimeS(void);
u8 GetResetReason(void);


bool obj_model_current_run_time_get(object_model_interface_t *p_obj_mode)
{
    set_obj_model_uint_value(p_obj_mode, getRunTimeS());
    return true;
}


bool obj_model_hard_reset_time_get(object_model_interface_t *p_obj_mode)
{
    set_obj_model_uint_value(p_obj_mode, StaSysPara.HardRstCnt);
    return true;
}

bool obj_model_soft_reset_time_get(object_model_interface_t *p_obj_mode)
{
    set_obj_model_uint_value(p_obj_mode, StaSysPara.SofeRstCnt);
    return true;
}

bool obj_model_system_info_get(object_model_interface_t *p_obj_mode)
{
    p_obj_mode->data_len = sizeof(ST_STA_SYS_TYPE);
    memcpy(p_obj_mode->data_buff,&StaSysPara,sizeof(ST_STA_SYS_TYPE));
    p_obj_mode->data_type = STRUCT_TYPE;
    return true;
    
}

bool obj_model_reset_reason_get(object_model_interface_t *p_obj_mode)
{
    set_obj_model_enum_value(p_obj_mode, GetResetReason());
    return true;
}

// 2- 两个路由周期收不到信标帧 ； 3 - 4个路由周期成功率为0 ； 4- 最大层级超过15; 5- 命令指示离线
bool obj_model_offline_reason_get(object_model_interface_t *p_obj_mode)
{
    set_obj_model_enum_value(p_obj_mode, GetLastOfflineReasonNum());
    return true;
}



/**
 * @brief 读取网络周期信息
 *
 * @param p_obj_mode
 * @return true
 * @return false
 */
bool obj_model_period_info_get(object_model_interface_t *p_obj_mode)
{
    p_obj_mode->data_len = sizeof(ST_STA_PERIOD_TYPE);
    memcpy(p_obj_mode->data_buff,(u8 *)GetStaPeriodPara(),sizeof(ST_STA_PERIOD_TYPE));
    p_obj_mode->data_type = STRUCT_TYPE;
    return true;
    
}


/**
 * @brief 读取网络基础信息
 *
 * @param p_obj_mode
 * @return true
 * @return false
 */
bool obj_model_base_info_get(object_model_interface_t *p_obj_mode)
{
    p_obj_mode->data_len = sizeof(ST_STA_ATTR_TYPE);
    memcpy(p_obj_mode->data_buff,(u8 *)GetStaBaseAttr(),sizeof(ST_STA_ATTR_TYPE));
    p_obj_mode->data_type = STRUCT_TYPE;
    return true;
    
}

uint16_t lamp_get_base_info_str(uint8_t *info_str)
{
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();

    uint16_t bufflen = 0;
    bufflen += sprintf((char *)info_str + bufflen, "tei = %d, layer = %d, RfHop = %d, ReaderLink = %s, NetFlag = %d, PcoTmi = %d, Phase = %d", 
						pNetBasePara->TEI, pNetBasePara->Layer, pNetBasePara->RfHop, (pNetBasePara->ReaderLink == 0) ? "HPLC" : "HRF", 
						pNetBasePara->NetFlag,pNetBasePara->PcoTmi, pNetBasePara->Phase);
    return bufflen;
}

bool obj_model_base_info_str_get(object_model_interface_t *p_obj_mode)
{
    p_obj_mode->data_len = lamp_get_base_info_str(p_obj_mode->data_buff);
    p_obj_mode->data_type = STRING_TYPE;
    return true;
    
}

/**
 * @brief 上报配置
 *
 * @param p_obj_mode
 * @return true
 * @return false
 */
bool obj_model_report_waite_time_get(object_model_interface_t *p_obj_mode)
{
    uint32_t wait_time = ble_get_report_wait_time();
    set_obj_model_uint_value(p_obj_mode, wait_time);
    return true;
}

bool obj_model_event_all_enable_get(object_model_interface_t *p_obj_mode)
{
    extern uint8_t event_all_enable;
    set_obj_model_enum_value(p_obj_mode, event_all_enable);
    return true;
}

bool obj_model_event_en_mask_get(object_model_interface_t *p_obj_mode)
{
    uint32_t en_mask = ble_get_report_event_en_mask();
    set_obj_model_uint_value(p_obj_mode, en_mask);
    return true;
}

bool obj_model_report_waite_time_put(object_model_interface_t *p_obj_mode)
{
    uint32_t value;
    bool r_flg;
    if ((r_flg = get_obj_model_uint_value(p_obj_mode, &value)))
    {
        r_flg = ble_put_report_wait_time(value);
    }
    return r_flg;
}

bool obj_model_event_all_enable_put(object_model_interface_t *p_obj_mode)
{
    uint8_t value;
    bool r_flg;
    if ((r_flg = get_obj_model_enum_value(p_obj_mode, &value)))
    {
        r_flg = ble_put_report_event_all_enable(value);
    }
    return r_flg;
}

bool obj_model_event_en_mask_put(object_model_interface_t *p_obj_mode)
{
    uint32_t value;
    bool r_flg;
    if ((r_flg = get_obj_model_uint_value(p_obj_mode, &value)))
    {
        r_flg = ble_put_report_event_en_mask(value);
    }
    return r_flg;
}

/**
 * @brief 蓝牙模块
 *
 * @param p_obj_mode
 * @return true
 * @return false
 */
bool obj_model_ble_send_power_get(object_model_interface_t *p_obj_mode)
{
    uint16_t power = ble_get_ble_send_power();
    // p_obj_mode->data_len = sizeof(uint16_t);
    // p_obj_mode->data_buff[0] = power;
    // p_obj_mode->data_buff[1] = power >> 8;
    // p_obj_mode->data_type = STRUCT_TYPE;
    set_obj_model_uint_value(p_obj_mode, power);
    return true;
}

bool obj_model_ble_send_power_put(object_model_interface_t *p_obj_mode)
{
    bool r_flg;
    uint32_t value;

    r_flg = get_obj_model_uint_value(p_obj_mode, &value);
    if (r_flg)
    {
        r_flg = ble_change_ble_send_power(value);
    }
    return r_flg;
    // uint32_t power = *((uint32_t *)(p_obj_mode->data_buff));
    // return ble_change_ble_send_power(power);
}

bool obj_model_ble_reset_info_get(object_model_interface_t *p_obj_mode)
{
    // p_obj_mode->data_len = sizeof(lamp_ble_param.ble_sys_reset_info);
    // memcpy(p_obj_mode->data_buff,lamp_ble_param.ble_sys_reset_info,sizeof(lamp_ble_param.ble_sys_reset_info));
    // p_obj_mode->data_type = STRUCT_TYPE;

    set_obj_model_array_value(p_obj_mode, (uint8_t *)lamp_ble_param.ble_sys_reset_info, sizeof(lamp_ble_param.ble_sys_reset_info));
    return true;
}
bool obj_model_ble_soft_version_get(object_model_interface_t *p_obj_mode)
{
    p_obj_mode->data_type = STRING_TYPE;
    p_obj_mode->data_len = sprintf((char *)p_obj_mode->data_buff, "%04x-%04x-%04x-%04x", lamp_ble_param.ble_ver_value.chip_type, 0, lamp_ble_param.ble_ver_value.software_version, 0);
    return true;
}

bool obj_model_ble_hard_version_get(object_model_interface_t *p_obj_mode) 
{
    p_obj_mode->data_type = STRING_TYPE;
    p_obj_mode->data_len = sprintf((char *)p_obj_mode->data_buff, "%04x-%04x-%04x-%04x", lamp_ble_param.ble_ver_value.chip_type, 0, lamp_ble_param.ble_ver_value.software_version, 0);
    return true;
}

bool obj_model_ble_device_type_get(object_model_interface_t *p_obj_mode)
{
    p_obj_mode->data_type = UINT16_TYPE;
	memcpy(p_obj_mode->data_buff,(uint8_t *)&lamp_ble_param.device_type,UINT16_SIZE);
    p_obj_mode->data_len = UINT16_SIZE;
    return true;
}

/**
 * @brief 读取网络KEY
 *
 * @param p_obj_mode
 * @return true
 * @return false
 */
bool obj_model_check_isp_key_get(object_model_interface_t *p_obj_mode)
{
    set_obj_model_uint_value(p_obj_mode, lc_net_get_factory_id_code());
    return true;
}

bool obj_model_check_project_key_get(object_model_interface_t *p_obj_mode)
{
    set_obj_model_uint_value(p_obj_mode, lc_net_get_project_id_code());
    return true;
}


bool obj_model_check_isp_key_set(object_model_interface_t *p_obj_mode)
{
    bool r_flg = false;
    uint32_t value;
    if ((r_flg = get_obj_model_uint_value(p_obj_mode, &value)))
    {
        ble_net_info.IspKey = value;

		if (value != lc_net_get_factory_id_code())
		{
			lc_net_write_factory_id_code(value);
		}
    }
    return r_flg;
}

bool obj_model_check_project_key_set(object_model_interface_t *p_obj_mode)
{
    bool r_flg = false;
    uint32_t value;
    if ((r_flg = get_obj_model_uint_value(p_obj_mode, &value)))
    { 
		if(value != lc_net_get_project_id_code())
		{
			lc_net_write_project_id_code(value);

			ble_local_restart_net();

			LcSetFactoryMsg();
		}
    }
    return r_flg;
}

bool obj_model_net_neighber_list_get(object_model_interface_t *p_obj_mode)
{
	if((p_obj_mode->CIID & 0x6000) != 0x6000)
	{
		return false;
	}
    uint16_t getListIndex = (p_obj_mode->CIID - 0x6000);
	getListIndex = ((getListIndex>>8)&0x000F)*100 + ((getListIndex>>4)&0x000F)*10 + (getListIndex&0x000F);
	if(getListIndex <= 0 || getListIndex > MAX_NEIGHBOR_NUM)
	{
		return false;
	}
	const ST_NEIGHBOR_TAB_TYPE* tabType = GetNeighborTable();
	p_obj_mode->data_len = sizeof(ST_NEIGHBOR_TYPE);
	memcpy(p_obj_mode->data_buff,&tabType->Neighbor[getListIndex -1],sizeof(ST_NEIGHBOR_TYPE));
	p_obj_mode->data_type = STRUCT_TYPE;
    return true;
}

bool obj_model_net_neighber_num_get(object_model_interface_t *p_obj_mode)
{
	p_obj_mode->data_len = 4;
	p_obj_mode->data_type = INT_TYPE;
	const ST_NEIGHBOR_TAB_TYPE* tabType = GetNeighborTable();
	memcpy(p_obj_mode->data_buff,&tabType->NeighborNum,4);
    return true;
}

bool obj_model_net_neighber_tei_get(object_model_interface_t *p_obj_mode)
{
	if((p_obj_mode->CIID & 0x6000) != 0x6000)
	{
		return false;
	}
    uint16_t getTeiIndex = (p_obj_mode->CIID - 0x6000);
	getTeiIndex = ((getTeiIndex>>8)&0x000F)*100 + ((getTeiIndex>>4)&0x000F)*10 + (getTeiIndex&0x000F);
	if(getTeiIndex <= 0 || getTeiIndex > (MAX_ROUTE_NUM/64)+1)
	{
		return false;
	}
	const ST_NEIGHBOR_TAB_TYPE* tabType = GetNeighborTable();
	
	if(getTeiIndex*64 >= MAX_ROUTE_NUM)
	{
		p_obj_mode->data_len = (MAX_ROUTE_NUM - (getTeiIndex-1)*64)*2;
		memcpy(p_obj_mode->data_buff,&tabType->TEI[(getTeiIndex-1)*64],(MAX_ROUTE_NUM - (getTeiIndex-1)*64)*2);
	}
	else
	{
		p_obj_mode->data_len = 64*2;
		memcpy(p_obj_mode->data_buff,&tabType->TEI[(getTeiIndex-1)*64],64*2);
	}
	p_obj_mode->data_type = STRUCT_TYPE;
    return true;
}

extern bool GetSystemMeterByIndex(u8 *id,int index);
extern void node485ReportInNetInit();
extern bool IsNode485ReportInNetFlag;

bool obj_model_node_report_online_put(object_model_interface_t *p_obj_mode)
{
	u8 addrs[6 * 10];
	u8 mac[6];
	u8 meterMac[6];
	u32 len = p_obj_mode->data_len;
	if(len % 6 != 0)
		return false;
	if(len > 6 * 10)
		len = 6 * 10;
	bool r_flg = get_obj_model_array_value(p_obj_mode, addrs, len);
	if (r_flg)
    {
    	lc_net_my_info_t net_info;
		lc_net_get_net_self_info(&net_info);
		for(int i = 0; i < (len / 6); i++)
		{
			for(int j=0;j<6;j++)
			{
				mac[j] = addrs[5-j + i*6];
			
}
			if((0 == memcmp(mac, net_info.addr, 6)) || (0 == memcmp(addrs + i * 6, net_info.addr, 6)))
			{
				LampEventAddInNetReport(net_info.addr);
				return true;
			}
			for(int k=0;k<32;k++)
			{
				bool ret = GetSystemMeterByIndex((u8 *)meterMac,k);
				if(ret == true)
				{
					if((0 == memcmp(mac, meterMac, 6)) || (0 == memcmp(addrs + i * 6, meterMac, 6)))
					{
						LampEventAddInNetReport(net_info.addr);
						return true;
					}
				}
			}
		}
    }
	return false;
}

bool obj_model_ble_breaker_config_put(object_model_interface_t *p_obj_mode)
{	
	if (p_obj_mode->CIID == CIID_TAG_CONFIG_MAC_OBJ)
	{
		if ((p_obj_mode->data_type != ARRY_TYPE) && (p_obj_mode->data_len < 20 * 8))
	    {
	        return false;
	    }		
		lc_net_write_ble_breaker_addr(p_obj_mode->data_buff, 20 * 8);
		return true;
	}
	else if (p_obj_mode->CIID == CIID_TAG_REPEATER_RESET_OBJ)
	{
		//reset
		uint8_t value;
	    if (get_obj_model_enum_value(p_obj_mode, &value))
	    {
	    	if(value == 1)
				REBOOT_SYSTEM();
	    }
	}
	return false;
}

bool obj_model_ble_breaker_config_mac_get(object_model_interface_t *p_obj_mode)
{	
	if(p_obj_mode->CIID == CIID_TAG_CONFIG_MAC_OBJ)
	{
		uint32_t len = 0;
		lc_net_get_ble_breaker_addr(p_obj_mode->data_buff,&len);
		p_obj_mode->data_len = len;
        p_obj_mode->data_type = ARRY_TYPE;
		return true;
	}
	return false;
}
extern ble_system_params_type ble_system_params;
extern bool ble_system_info_stored();

bool obj_model_ble_sta_mode_put(object_model_interface_t *p_obj_mode)
{	
	uint32_t mode = 0;
	if(p_obj_mode->data_len == UINT_SIZE)
	{
		memcpy(&mode,p_obj_mode->data_buff,UINT_SIZE);
		if(mode != ble_system_params.ble_sta_mode)
		{
			ble_system_params.ble_sta_mode = mode;
			//存储到flash中
			ble_system_params.DirtySystemParamFlg = 1;
			ble_system_info_stored();
		}
		return true;
	}
	return false;
}

bool obj_model_ble_sta_mode_get(object_model_interface_t *p_obj_mode)
{	
	memcpy(p_obj_mode->data_buff,&ble_system_params.ble_sta_mode,UINT_SIZE);
	p_obj_mode->data_len = UINT_SIZE;
    p_obj_mode->data_type = UINT_TYPE;
	return true;
}

uint8_t GetUartRateType(uint32_t baudrate)
{
	if(baudrate < UART_MAX)
		return (uint8_t)baudrate;
	
	switch(baudrate)
	{
		case 1200:
			return UART_1200;
		case 2400:
			return UART_2400;
		case 4800:
			return UART_4800;
		case 9600:
			return UART_9600;
		case 19200:
			return UART_19200;
		case 38400:
			return UART_38400;
		case 115200:
			return UART_115200;
		case 256000:
			return UART_256000;
		default:
			return UART_2400;
	}
}

bool obj_mode_ii_read_meter_put(object_model_interface_t *p_obj_mode)
{	
	if (p_obj_mode->CIID == CIID_II_COLLECTOR_METER_MAC_OBJ)
	{
		if ((p_obj_mode->data_type != ARRY_TYPE) && (p_obj_mode->data_len < 20 * 8))
	    {
	        return false;
	    }

		for(int i = 0; i < 20; i++)
		{
			memcpy(ble_system_params.meters_addr[i].mac, &p_obj_mode->data_buff[i * 8], LC_NET_ADDR_SIZE);
			ble_system_params.meters_addr[i].baudrate = GetUartRateType(p_obj_mode->data_buff[i * 8 + 6]);
			ble_system_params.meters_addr[i].parity = p_obj_mode->data_buff[i * 8 + 7];
		}
		//存储到flash中
		ble_system_params.DirtySystemParamFlg = 1;
		ble_system_info_stored();
		return true;
	}
	return false;
}
extern bool Obj_GetSearchMeter(uint8_t *buf, uint16_t *len);

bool obj_mode_ii_read_meter_get(object_model_interface_t *p_obj_mode)
{	
	if(p_obj_mode->CIID == CIID_II_COLLECTOR_METER_MAC_OBJ)
	{
		memcpy(p_obj_mode->data_buff, &ble_system_params.meters_addr[0].mac[0], 20 * 8);
		p_obj_mode->data_len = 20 * 8;
        p_obj_mode->data_type = ARRY_TYPE;
		return true;
	}
	else if(p_obj_mode->CIID == CIID_II_SEARCH_METER_OBJ)
	{
		uint16_t len = 0;
		Obj_GetSearchMeter(p_obj_mode->data_buff,&len);
		p_obj_mode->data_len = len;
        p_obj_mode->data_type = ARRY_TYPE;
		return true;
	}
	return false;
}
extern void lamp_ble_factory_test_param_read();
extern void lamp_ble_factory_test_param_set(uint8_t *p);

// 成品产测配置
bool obj_model_factory_test_param_put(object_model_interface_t *p_obj_mode)
{
    bool r_flg = false;
	debug_str(DEBUG_LOG_UPDATA, "obj_model_factory_test_param_put \r\n");

	if (CIID_FACTORY_BLE_RSSI == p_obj_mode->CIID)
    {
        if ((p_obj_mode->data_type == ARRY_TYPE) && (p_obj_mode->data_len == sizeof(factory_ble_test_t)))
        {
            lamp_ble_factory_test_param_set(p_obj_mode->data_buff);
            r_flg = true;
        }
    }

    return r_flg;
}
extern factory_ble_test_t factory_ble;
extern uint8_t test_param_version_cb_flag;
bool obj_model_factory_test_param_get(object_model_interface_t *p_obj_mode)
{
    bool r_flg = true;

	debug_str(DEBUG_LOG_UPDATA, "obj_model_factory_test_param_get \r\n");

    if (CIID_FACTORY_BLE_RSSI == p_obj_mode->CIID )
    {   
        lamp_ble_factory_test_param_read();
		//没收到蓝牙回复时，不回复
		if(test_param_version_cb_flag == 0)
		{
			return false;
		}
		debug_str(DEBUG_LOG_UPDATA, "obj_model_factory_test_param_get ok \r\n");
        set_obj_model_array_value(p_obj_mode, (u8 *)&factory_ble, sizeof(factory_ble_test_t));
    }


    return r_flg;
}

bool obj_model_mac_list_get(object_model_interface_t *p_obj_mode)
{
    bool r_flg = false;

	debug_str(DEBUG_LOG_UPDATA, "obj_model_mac_list_get \r\n");

	//白名单
    if (0x6F01 == p_obj_mode->CIID )
    {   
         p_obj_mode->data_type = ARRY_TYPE;
	     p_obj_mode->data_len = WHITE_BLACK_MAC_NUM*6;
	     memcpy(p_obj_mode->data_buff, g_mac_list.white_mac_list, WHITE_BLACK_MAC_NUM*6);
		 r_flg = true;
    }
	//黑名单
	else if(0x6F02 == p_obj_mode->CIID)
	{
		 p_obj_mode->data_type = ARRY_TYPE;
	     p_obj_mode->data_len = WHITE_BLACK_MAC_NUM*6;
	     memcpy(p_obj_mode->data_buff, g_mac_list.black_mac_list, WHITE_BLACK_MAC_NUM*6);
		 r_flg = true;
	}
    return r_flg;
}
extern void mac_list_flash_save();

bool obj_model_mac_list_put(object_model_interface_t *p_obj_mode)
{
    bool r_flg = false;

	//白名单
    if (0x6F01 == p_obj_mode->CIID && ARRY_TYPE == p_obj_mode->data_type && WHITE_BLACK_MAC_NUM*6 >= p_obj_mode->data_len)
    {   
    	 uint16_t temp_num = p_obj_mode->data_len/6;
		 if(temp_num > 0)
		 {
		 	memcpy(g_mac_list.white_mac_list,p_obj_mode->data_buff, temp_num*6);
			mac_list_flash_save();
			r_flg = true;
		 }
    }
	//黑名单
	else if(0x6F02 == p_obj_mode->CIID && ARRY_TYPE == p_obj_mode->data_type && WHITE_BLACK_MAC_NUM*6 >= p_obj_mode->data_len)
	{
		 uint16_t temp_num = p_obj_mode->data_len/6;
		 if(temp_num > 0)
		 {
		 	memcpy(g_mac_list.black_mac_list,p_obj_mode->data_buff, temp_num*6);
			mac_list_flash_save();
			r_flg = true;
		 }
	}
    return r_flg;
}

bool obj_model_gateway_net_enable_flg_get(object_model_interface_t *p_obj_mode)
{
    set_obj_model_enum_value(p_obj_mode, ble_system_params.NetEnableFlag);
	return true;
}

bool obj_model_gateway_net_enable_flg_set(object_model_interface_t *p_obj_mode)
{
	if (p_obj_mode->data_len == 0)
    {
        return false;
    }
	if(ble_system_params.NetEnableFlag != p_obj_mode->data_buff[0])
	{
		ble_system_params.NetEnableFlag = p_obj_mode->data_buff[0];
		uint32_t param = lc_net_get_ex_param_code();
		if(ble_system_params.NetEnableFlag == 0)
		{
			param &= ~(0x01UL);
		}
		else
		{
			param |= 0x01;
		}
		lc_net_write_ex_param_code(param);
		ble_local_restart_net();
	}
	return true;
}

bool obj_mode_plc_neighbor_info_get(object_model_interface_t *p_obj_mode)
{
	uint16_t node_sum = 0;
	uint16_t read_start = 0;
	uint16_t read_sum = 6;
	if(p_obj_mode->CIID >= 0x6001)
	{
		read_start = p_obj_mode->CIID - 0x6001;
	}
	else
	{
		return false;
	}
	read_sum = lc_get_plc_neighbor_info(&(p_obj_mode->data_buff[6]),read_start*read_sum+1, read_sum,&node_sum);
	
	p_obj_mode->data_buff[0] = node_sum&0xFF;
	p_obj_mode->data_buff[1] = (node_sum>>8)&0xFF;
	p_obj_mode->data_buff[2] = (read_start+1)&0xFF;
	p_obj_mode->data_buff[3] = ((read_start+1)>>8)&0xFF;
	p_obj_mode->data_buff[4] = read_sum&0xFF;
	p_obj_mode->data_buff[5] = (read_sum>>8)&0xFF;
	//
	p_obj_mode->data_type = STRUCT_TYPE;
	p_obj_mode->data_len = 6 + read_sum*sizeof(Obj_NeighborTable);
	//
	return true;
}

bool obj_mode_rf_neighbor_info_get(object_model_interface_t *p_obj_mode)
{
	uint16_t node_sum = 0;
	uint16_t read_start = 0;
	uint16_t read_sum = 6;
	if(p_obj_mode->CIID >= 0x6001)
	{
		read_start = p_obj_mode->CIID - 0x6001;
	}
	else
	{
		return false;
	}
	read_sum = lc_get_rf_neighbor_info(&(p_obj_mode->data_buff[6]),read_start*read_sum+1, read_sum,&node_sum);
	
	p_obj_mode->data_buff[0] = node_sum&0xFF;
	p_obj_mode->data_buff[1] = (node_sum>>8)&0xFF;
	p_obj_mode->data_buff[2] = (read_start+1)&0xFF;
	p_obj_mode->data_buff[3] = ((read_start+1)>>8)&0xFF;
	p_obj_mode->data_buff[4] = read_sum&0xFF;
	p_obj_mode->data_buff[5] = (read_sum>>8)&0xFF;
	//
	p_obj_mode->data_type = STRUCT_TYPE;
	p_obj_mode->data_len = 6 + read_sum*sizeof(Obj_NeighborTable);
	//
	return true;
}

bool obj_mode_pco_info_get(object_model_interface_t *p_obj_mode)
{
	bool ret = lc_get_pco_info(p_obj_mode->data_buff);
	if(ret == false)
	{
		return false;
	}
	p_obj_mode->data_type = STRUCT_TYPE;
	p_obj_mode->data_len = sizeof(Obj_Pco_Info);
	return true;
}

bool obj_mode_curtains_control_put(object_model_interface_t *p_obj_mode)
{
	uint8_t factory = p_obj_mode->data_buff[0];
	uint8_t datalen = p_obj_mode->data_buff[1];
	if(p_obj_mode->CIID == 0x6001)
	{
		CurtainsControlCmdSend(factory,CURTAINS_OPEN,&p_obj_mode->data_buff[2],datalen);
	}
	else if(p_obj_mode->CIID == 0x6002)
	{
		CurtainsControlCmdSend(factory,CURTAINS_CLOSE,&p_obj_mode->data_buff[2],datalen);
	}
	else if(p_obj_mode->CIID == 0x6003)
	{
		CurtainsControlCmdSend(factory,CURTAINS_CONTROL,&p_obj_mode->data_buff[2],datalen);
	}
	else if(p_obj_mode->CIID == 0x6004)
	{
		CurtainsControlCmdSend(factory,CURTAINS_STOP,&p_obj_mode->data_buff[2],datalen);
	}
	else if(p_obj_mode->CIID == 0x6005)
	{
		CurtainsControlCmdSend(factory,CURTAINS_SET_ITINERARY_POINTS,&p_obj_mode->data_buff[2],datalen);
	}
	else if(p_obj_mode->CIID == 0x6006)
	{
		CurtainsControlCmdSend(factory,CURTAINS_RUN_TO_ITINERARY_POINTS,&p_obj_mode->data_buff[2],datalen);
	}
	else if(p_obj_mode->CIID == 0x6007)
	{
		CurtainsControlCmdSend(factory,CURTAINS_DEL_ITINERARY_POINTS,&p_obj_mode->data_buff[2],datalen);
	}
    return true;
}
extern void broadcast_msg_test_send(uint8_t sendtype,uint8_t *msg,uint8_t msg_len);
extern u32 g_broadcast_msg_test_recv_plc_num;
extern u32 g_broadcast_msg_test_recv_rf_num;
bool obj_mode_broadcast_msg_test_put(object_model_interface_t *p_obj_mode)
{
	if(p_obj_mode->CIID == 0x6001 && p_obj_mode->data_type == ARRY_TYPE)
	{
		uint8_t sendtype = p_obj_mode->data_buff[0];
		if(sendtype == 1 && p_obj_mode->data_len == 8)
		{
			uint8_t band = p_obj_mode->data_buff[1];
			if(band <= 2)
			{
				HPLC_PHY_SetFrequence(band);
			}
			const ST_STA_ATTR_TYPE* pNetBasePara = GetStaBaseAttr();
			memcpy(&p_obj_mode->data_buff[8],pNetBasePara->Mac,6);
			broadcast_msg_test_send(sendtype,&p_obj_mode->data_buff[0],14);
		}
		else if(sendtype == 2 && p_obj_mode->data_len == 9)
		{
			uint8_t channel = p_obj_mode->data_buff[1];
			uint8_t option = p_obj_mode->data_buff[2];
			if(option <= 3 && option >= 1)
			{
				HRF_PHY_SetChannel(channel,option);
			}
			const ST_STA_ATTR_TYPE* pNetBasePara = GetStaBaseAttr();
			memcpy(&p_obj_mode->data_buff[9],pNetBasePara->Mac,6);
			broadcast_msg_test_send(sendtype,&p_obj_mode->data_buff[0],15);
		}
		else
		{
			return false;
		}
		
	}
	else if(p_obj_mode->CIID == 0x6002 && p_obj_mode->data_type == UINT_TYPE)
	{
		u32 value = 0;
		memcpy(&value,p_obj_mode->data_buff,UINT_SIZE);
		if(value == 0)
		{
			g_broadcast_msg_test_recv_plc_num = 0;
		}
		else
		{
			return false;
		}
	}
	else if(p_obj_mode->CIID == 0x6003 && p_obj_mode->data_type == UINT_TYPE)
	{
		u32 value = 0;
		memcpy(&value,p_obj_mode->data_buff,UINT_SIZE);
		if(value == 0)
		{
			g_broadcast_msg_test_recv_rf_num = 0;
		}
		else
		{
			return false;
		}
	}
	else if(p_obj_mode->CIID == 0x6004 && p_obj_mode->data_type == ARRY_TYPE)
	{
		uint8_t band = p_obj_mode->data_buff[0];
		if(band <= 2)
		{
			HPLC_PHY_SetFrequence(band);
		}
		else
		{
			return false;
		}
	}
	else if(p_obj_mode->CIID == 0x6005 && p_obj_mode->data_type == ARRY_TYPE)
	{
		uint8_t option = p_obj_mode->data_buff[1];
		uint8_t channel = p_obj_mode->data_buff[0];
		if(option <= 3 && option >= 1)
		{
			HRF_PHY_SetChannel(channel,option);
		}
		else
		{
			return false;
		}
	}
    return true;
}

bool obj_mode_broadcast_msg_test_get(object_model_interface_t *p_obj_mode)
{
	if(p_obj_mode->CIID == 0x6002)
	{
		memcpy(p_obj_mode->data_buff,&g_broadcast_msg_test_recv_plc_num,UINT_SIZE);
		p_obj_mode->data_type = UINT_TYPE;
		p_obj_mode->data_len = UINT_SIZE;
	}
	else if(p_obj_mode->CIID == 0x6003)
	{
		memcpy(p_obj_mode->data_buff,&g_broadcast_msg_test_recv_rf_num,UINT_SIZE);
		p_obj_mode->data_type = UINT_TYPE;
		p_obj_mode->data_len = UINT_SIZE;
	}
	else if(p_obj_mode->CIID == 0x6004)
	{
		uint8_t band = HPLC_PHY_GetFrequence();
		memcpy(p_obj_mode->data_buff,&band,ENUM_SIZE);
		p_obj_mode->data_type = ARRY_TYPE;
		p_obj_mode->data_len = 1;
	}
	else if(p_obj_mode->CIID == 0x6005)
	{
		p_obj_mode->data_buff[0] = HRF_Channel;
		p_obj_mode->data_buff[1] = HRF_Option;	
		p_obj_mode->data_type = ARRY_TYPE;
		p_obj_mode->data_len = 2;
	}
    return true;
}
extern bool lc_net_write_key_switch_code(uint8_t key_switch);
extern uint8_t lc_net_get_key_switch_code();

bool obj_mode_key_switch_put(object_model_interface_t *p_obj_mode)
{
	if (p_obj_mode->data_type != ENUM_TYPE)
    {
        return false;
    }
	lc_net_write_key_switch_code(p_obj_mode->data_buff[0]);
	
	return true;
}

bool obj_mode_key_switch_get(object_model_interface_t *p_obj_mode)
{
	set_obj_model_enum_value(p_obj_mode,lc_net_get_key_switch_code());
	return true;
}

bool obj_model_crash_log_put(object_model_interface_t *p_obj_mode)
{
	crash_log_flash_clear();
	return true;
}

bool obj_model_crash_log_get(object_model_interface_t *p_obj_mode)
{
	u8 block_idx = (u8)p_obj_mode->CIID;
	if (block_idx > 3)
		return false;

	u32 len = crash_log_flash_read_block(block_idx, p_obj_mode->data_buff, CRASH_LOG_BLOCK_SIZE);
	if (len == 0)
		return false;

	p_obj_mode->data_type = ARRY_TYPE;
	p_obj_mode->data_len = len;
	return true;
}

//===========================================================
// 物模型属性读取与写入接口
typedef struct
{
    uint32_t siid;
    uint32_t ciid;
    bool (*process_put_cb_fun)(object_model_interface_t *p_obj_mode);
    bool (*process_get_cb_fun)(object_model_interface_t *p_obj_mode);
} obj_model_process_t;

const obj_model_process_t obj_mode_set_process_list[] = {
    // 节点版本
    {SIID_NODE_VERSION, CIID_SOFT_VERSION, NULL, obj_model_lamp_node_soft_version_get},

   
    // 系统参数
    {SIID_RELAY_SYS_INFO, CIID_RUNNING_TIME_S, NULL, obj_model_current_run_time_get},               // 本次复位运行时长
    {SIID_RELAY_SYS_INFO, CIID_HARD_RESET_TIMES, NULL, obj_model_hard_reset_time_get},             // 硬复位次数
    {SIID_RELAY_SYS_INFO, CIID_SOFT_RESET_TIMES, NULL, obj_model_soft_reset_time_get},             // 软复位次数
    {SIID_RELAY_SYS_INFO, CIID_SYS_INFO_ALL, NULL, obj_model_system_info_get},                      // 系统参数结构体
    {SIID_RELAY_SYS_INFO, CIID_LAST_RESET_REASON, NULL, obj_model_reset_reason_get},                // 上一次复位的原因
    {SIID_RELAY_SYS_INFO, CIID_LAST_OFFLINE_REASON, NULL, obj_model_offline_reason_get},            // 上一次离网的原因
    


    // 功率参数
    {SIID_RELAY_POWER,CIID_BAND_POWER,obj_model_plc_power_put,obj_model_plc_power_get},     // plc 功率
    {SIID_RELAY_POWER,CIID_PLC_POWER_MODE,NULL,obj_model_plc_mode_get},                     // plc 功率自适应模式
    {SIID_RELAY_POWER,CIID_OPT_POWER,obj_model_rf_power_put,obj_model_rf_power_get},        // rf  功率
    {SIID_RELAY_POWER,CIID_RF_POWER_MODE,NULL,obj_model_rf_mode_get},                       // rf  功率自适应模式

    // 网络参数
    {SIID_GATEWAY_PARAM, CIID_GATEWAY_NET_ENABLE_FLG, obj_model_gateway_net_enable_flg_set, obj_model_gateway_net_enable_flg_get},
    {SIID_RELAY_NET_INFO,CIID_NET_PERIOD_INFO,NULL,obj_model_period_info_get},               // 获取网络周期信息
    {SIID_RELAY_NET_INFO,CIID_NET_BASE_INFO,NULL,obj_model_base_info_get},                   // 获取网络基本信息
    {SIID_RELAY_NET_INFO,CIID_NET_BASE_INFO_STR,NULL,obj_model_base_info_str_get},               // 获取网络基本信息, 字符串描述
	{SIID_NODE_REPORT_ONLINE, CIID_NODE_REPORT_ONLINE, obj_model_node_report_online_put, NULL},

    //上报配置
    {SIID_LAMP_CONFIG_TIME,CIID_LAMP_CONFIG_REPORT_WAITE_TIME,obj_model_report_waite_time_put,obj_model_report_waite_time_get},         // 上报等待确认时间
    {SIID_LAMP_CONFIG_TIME,CIID_LAMP_CONFIG_REPORT_ALL_ENABLE,obj_model_event_all_enable_put,obj_model_event_all_enable_get},           // 上报总开关
    {SIID_LAMP_CONFIG_TIME,CIID_LAMP_CONFIG_REPORT_BIT_MASK,obj_model_event_en_mask_put,obj_model_event_en_mask_get},                   // 上报使能位

    //蓝牙模块
    {SIID_LAMP_BLE_MODE,CIID_LAMP_BLE_MODE_SEND_POWER,obj_model_ble_send_power_put,obj_model_ble_send_power_get},   // 功率
    {SIID_LAMP_BLE_MODE,CIID_LAMP_BLE_MODE_SYS_RESET_INFO,NULL,obj_model_ble_reset_info_get},                       // 复位信息记录
    {SIID_LAMP_BLE_MODE,CIID_SOFT_VERSION,NULL,obj_model_ble_soft_version_get},                                     // 软件版本
    {SIID_LAMP_BLE_MODE,CIID_HARD_VERSION,NULL,obj_model_ble_hard_version_get},                                     // 硬件版本
    {SIID_LAMP_BLE_MODE,CIID_DEVICE_TYPE,NULL,obj_model_ble_device_type_get},                                       // 设备类型

    //认证参数
    {SIID_ISP_PROJ_KEY, CIID_ISP_KEY, obj_model_check_isp_key_set, obj_model_check_isp_key_get},
    {SIID_ISP_PROJ_KEY, CIID_PROJ_KEY, obj_model_check_project_key_set, obj_model_check_project_key_get},
    {SIID_NET_NEIGHBOR_LIST, NULL, NULL, obj_model_net_neighber_list_get},
    {SIID_NET_NEIGHBOR_NUM, NULL, NULL, obj_model_net_neighber_num_get},
    {SIID_NET_NEIGHBOR_TEI, NULL, NULL, obj_model_net_neighber_tei_get},
	{SIID_II_COLLECTOR_READ_METER_OBJ,NULL,obj_mode_ii_read_meter_put, obj_mode_ii_read_meter_get},
	{SIID_FACTORY_TEST, CIID_FACTORY_BLE_RSSI, obj_model_factory_test_param_put, obj_model_factory_test_param_get},

	//蓝牙断路器
    {SIID_TAG_BLE_BREAKER_CONFIG_OBJ, CIID_TAG_CONFIG_MAC_OBJ, obj_model_ble_breaker_config_put, obj_model_ble_breaker_config_mac_get},
    {SIID_TAG_BLE_BREAKER_CONFIG_OBJ, CIID_TAG_REPEATER_RESET_OBJ, obj_model_ble_breaker_config_put, NULL},

	//蓝牙STA模式
	{SIID_BLE_STA_MODE, NULL, obj_model_ble_sta_mode_put, obj_model_ble_sta_mode_get},
	{SIID_MAC_LIST, NULL, obj_model_mac_list_put, obj_model_mac_list_get},

	//邻居表
	{SIID_GET_PLC_NEIGHBOR_INFO_OBJ, NULL, NULL, obj_mode_plc_neighbor_info_get},
	{SIID_GET_RF_NEIGHBOR_INFO_OBJ, NULL, NULL, obj_mode_rf_neighbor_info_get},
	{SIID_GET_PCO_INFO_OBJ, NULL, NULL, obj_mode_pco_info_get},
	{SIID_CURTAINS_CONTROL, NULL, obj_mode_curtains_control_put, NULL},

	{SIID_BROADCAST_MSG_TEST, NULL, obj_mode_broadcast_msg_test_put, obj_mode_broadcast_msg_test_get},
	{SIID_KEY_SWITCH,CIID_KEY_SWITCH,obj_mode_key_switch_put, obj_mode_key_switch_get},
	{SIID_CRASH_LOG, NULL, obj_model_crash_log_put, obj_model_crash_log_get},
};

/**
 * @brief 查找物模型表
 *
 * @param p_obj_mode
 * @return const obj_model_process_t*
 */
static const obj_model_process_t *find_obj_mode_process_from_list(object_model_interface_t *p_obj_mode)
{
    uint32_t list_len = sizeof(obj_mode_set_process_list) / sizeof(obj_mode_set_process_list[0]);
    for (uint32_t i = 0; i < list_len; i++)
    {
        if (obj_mode_set_process_list[i].siid == p_obj_mode->SIID)
        {
            if ((obj_mode_set_process_list[i].ciid == p_obj_mode->CIID) || (obj_mode_set_process_list[i].ciid == NULL))
            {
                return &obj_mode_set_process_list[i];
            }
        }
    }
    return NULL;
}

/**
 * @brief 写物模型属性
 *
 * @param p_obj_mode
 * @return true
 * @return false
 */
bool obj_mode_put_attr_value(object_model_interface_t *p_obj_mode)
{
    bool r_flg = false;
    const obj_model_process_t *p_obj_mode_process = find_obj_mode_process_from_list(p_obj_mode);
    if (p_obj_mode_process != NULL)
    {
        if (p_obj_mode_process->process_put_cb_fun != NULL)
        {
            r_flg = p_obj_mode_process->process_put_cb_fun(p_obj_mode);
        }
    }
    return r_flg;
}

/**
 * @brief 读物模型属性
 *
 * @param p_obj_mode
 * @param max_len
 * @return true
 * @return false
 */
bool obj_mode_get_attr_value(object_model_interface_t *p_obj_mode, uint16_t max_len)
{
    bool r_flg = false;
    const obj_model_process_t *p_obj_mode_process = find_obj_mode_process_from_list(p_obj_mode);
    if (p_obj_mode_process != NULL)
    {
        if (p_obj_mode_process->process_get_cb_fun != NULL)
        {
            r_flg = p_obj_mode_process->process_get_cb_fun(p_obj_mode);
        }
    }
    return r_flg;
}


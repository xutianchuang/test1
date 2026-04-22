#include  <stdarg.h>
#include  <stdio.h>
#include "ble_event.h"
#include "ble_api.h"
#include "lc_whi_sysctrl.h"
#include "lamp_date.h"
#include "ble_data.h"
#include "hplc_data.h"

#define EVENT_LIST_NUM 8


uint32_t wait_report_tick;

uint8_t event_all_enable = true;
event_item_t *event_list[EVENT_LIST_NUM];
// 入网上报事件
u8 enter_net_report_buffer[sizeof(event_item_t) + sizeof(uint32_t)];
event_item_t *enter_net_report;
u8 EnterNetReportStatus = 0;

bool ble_put_report_event_all_enable(uint8_t enable)
{
    if((enable != 1) && (enable != 0)) return FALSE;
    event_all_enable = enable;
    return TRUE;
}

static void event_list_report_send(event_item_t *item)
{
    uint8_t buf[MAX_EVENT_REPORT_BYTES];
    lamp_event_report_t *e = (void *)buf;
    const lamp_date_param_t* lamp_date = get_lamp_date_base_param();

    if (item)
    {
        if (item->value_num > MAX_EVENT_VALUE_NUM)
        {
            item->value_num = MAX_EVENT_VALUE_NUM;
        }

        memcpy(e->mac, item->mac, 6);
        e->code = item->event_code;

        for (int i = 0; i < item->value_num; i++)
        {
            e->value[i] = item->event_value[i];
        }
        whi_sysctrl_report_event_data((u8 *)e, GET_EVENT_REPORT_BYTES(item));
        wait_report_tick = lamp_date->runtime;
    }
}

extern bool GetSystemMeterByIndex(u8 *id,int index);

bool IsNode485ReportInNetFlag = false;
bool IsCcoSearchMeter = false;
u8 Node485ReportStatus[32] = {0};
event_item_t *node_485_enter_net_report = NULL;
int Node485ReportInNetIndex = 0;
extern bool FindMeterComplete ;
extern void reportSerachMeterResult();
u8 Node485ReportMac[6] = {0};

void node485ReportInNetInit()
{
	IsNode485ReportInNetFlag = true;
	memset(Node485ReportStatus,0,32);
	Node485ReportInNetIndex = 0;
	if(node_485_enter_net_report != NULL)
	{
		free(node_485_enter_net_report);
		node_485_enter_net_report = NULL;
	}
}

void node485ReportInNetTask()
{
	u8 mac[6] = {0};
	if((FindMeterComplete == true) && (IsNode485ReportInNetFlag == true) && (node_485_enter_net_report == NULL) && enter_net_report == NULL)
	{
		if(IsCcoSearchMeter == false)
		{
			reportSerachMeterResult();
			IsCcoSearchMeter = true;
		}
		for(;Node485ReportInNetIndex<32;Node485ReportInNetIndex++)
		{
			if(GetSystemMeterByIndex(mac,Node485ReportInNetIndex) 
				&& Node485ReportStatus[Node485ReportInNetIndex] == 0)
			{
				u8 macTemp[6] = {0};
				const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();
				for(int i=0;i<6;i++)
				{
					macTemp[i] = mac[5-i];
				}
				if(memcmp(pNetBasePara->CcoMac,mac,6)==0 || memcmp(pNetBasePara->CcoMac,macTemp,6)==0)
				{
					continue;
				}
				memcpy(Node485ReportMac,macTemp,6);
				static uint32_t in_net_seq = 50;
			    node_485_enter_net_report = (void *)malloc(sizeof(event_item_t) + sizeof(uint32_t));;
			    node_485_enter_net_report->value_num = 1;
			    node_485_enter_net_report->event_code = EVENT_IN_NET_REPORT;
			    memcpy(node_485_enter_net_report->mac, macTemp, 6);
			    node_485_enter_net_report->event_value[0] = in_net_seq++;
			    event_list_report_send(node_485_enter_net_report);
				return ;
			}
		}
		if(Node485ReportInNetIndex == 32)
		{
			IsNode485ReportInNetFlag = false;
		}
	}
}

void lamp_event_report_add(uint16_t code, uint8_t * mac, uint32_t code_mask, ...) 
{
    uint8_t args_num;
    uint8_t min_idx;
    uint32_t min_time;
    va_list args;
    va_start(args, code_mask); 
    const lamp_date_param_t* lamp_date = get_lamp_date_base_param();

    // if (!(event_all_enable && (code_mask & (light_param.event_en_mask | ALWAYS_ENABLE_MASK))))
     uint32_t event_en_mask = ble_get_report_event_en_mask();
    if (!(event_all_enable && (code_mask & (event_en_mask | ALWAYS_ENABLE_MASK))))
    {
        va_end(args); 
        return;
    }

    event_item_t *item_add = malloc(sizeof(event_item_t) + MAX_EVENT_VALUE_NUM * sizeof(uint32_t));
    if (NULL == item_add)
    {
        va_end(args); 
        return;
    }
    args_num = 0;
    while (args_num < MAX_EVENT_VALUE_NUM) 
    {
        uint32_t value = va_arg(args, uint32_t); 
        if (value == -1) 
        { 
            break;
        }
        item_add->event_value[args_num] = value;
        args_num++;
    }
    va_end(args); 

    item_add->value_num = args_num;
    item_add->create_time = lamp_date->runtime;
    item_add->report_time = 0;
    item_add->event_code = code;
    // memcpy(item_add->mac, (mac ? mac : lamp_mac_address), 6);  $$$
    uint8_t mac_addr[6];
    lc_net_get_factory_mac(mac_addr);
    memcpy(item_add->mac, (mac ? mac : mac_addr), 6);

    // 放入发送列表里
    min_idx = 0;
    min_time = lamp_date->runtime;

    for (int i = 0; i < EVENT_LIST_NUM; i++)
    {
        if (event_list[i])
        {
            if (event_list[i]->create_time < min_time)
            {
                min_time = event_list[i]->create_time;
                min_idx = i;
            }
        }
        else
        {
            min_idx = i;
            break;
        }
    }

    if (event_list[min_idx])
    {
        free(event_list[min_idx]);
    }
    event_list[min_idx] = item_add;
}

void lamp_event_recv_callback(uint8_t *data, uint32_t len)
{
    if (len < sizeof(lamp_event_report_t))
    {
        return;
    }
    lamp_event_report_t *e = (void *)data;
	const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();
	u8 mactemp[6] = {0};
	for(int i=0;i<6;i++)
	{
		mactemp[i] = e->mac[5-i];
	}
    // 入网事件上报处理
    if (enter_net_report && (memcmp(pNetBasePara->Mac,e->mac,6) == 0 || memcmp(pNetBasePara->Mac,mactemp,6) == 0))
    {
        if (e->code == enter_net_report->event_code)
        {
            enter_net_report = NULL;
            wait_report_tick = 0;
			EnterNetReportStatus = 2;
        }
    }
	else if (node_485_enter_net_report && (memcmp(Node485ReportMac,e->mac,6) == 0 || memcmp(Node485ReportMac,mactemp,6) == 0))
	{
		if (e->code == node_485_enter_net_report->event_code)
		{
			free(node_485_enter_net_report);
			node_485_enter_net_report = NULL;
			Node485ReportStatus[Node485ReportInNetIndex] = 1;
			wait_report_tick = 0;
		}
	}

    for (int i = 0; i < EVENT_LIST_NUM; i++)
    {
        if (event_list[i])
        {
            if (e->code == event_list[i]->event_code)
            {
                free(event_list[i]);
                event_list[i] = NULL;
                wait_report_tick = 0;
                break;
            }
        }
    }
}

void lamp_event_list_reset()
{
    for (int i = 0; i < EVENT_LIST_NUM; i++)
    {
        free(event_list[i]);
        event_list[i] = NULL;
    }
}

void LampEventAddInNetReport(uint8_t * mac_addr)
{
    static uint32_t in_net_seq = 0;
	
	EnterNetReportStatus = 1;
	
    enter_net_report = (void *)enter_net_report_buffer;
    enter_net_report->value_num = 1;
    enter_net_report->event_code = EVENT_IN_NET_REPORT;
    memcpy(enter_net_report->mac, mac_addr, 6);
    enter_net_report->event_value[0] = in_net_seq++;

    event_list_report_send(enter_net_report);
	node485ReportInNetInit();
}


void lamp_event_task(void)
{
    const lamp_date_param_t* lamp_date = get_lamp_date_base_param();

    if (event_all_enable)
    {
        if(!lc_net_is_in_net())
        {
            return;
        }
        if (wait_report_tick)  
        { 
            uint32_t lamp_report_waite_time = ble_get_report_wait_time();//60 * 1000;
            if ((uint32_t)(lamp_date->runtime - wait_report_tick) < (lamp_report_waite_time))
            {
                return;
            }
        }

        if (enter_net_report)
        {
            event_list_report_send(enter_net_report);
            return;
        }
		if (node_485_enter_net_report)
		{
			event_list_report_send(node_485_enter_net_report);
			return;
		}

        uint32_t min_time = lamp_date->runtime;
        int min_idx = -1;
        for (int i = 0; i < EVENT_LIST_NUM; i++)
        {
            if (event_list[i])
            {
                if (event_list[i]->report_time < min_time)
                {
                    min_time = event_list[i]->report_time;
                    min_idx = i;
                }
            }
        }
        if (min_idx > -1)
        {
            event_list[min_idx]->report_time = lamp_date->runtime;
            event_list_report_send(event_list[min_idx]);
        }
    }
}

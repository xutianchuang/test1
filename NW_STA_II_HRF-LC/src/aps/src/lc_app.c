#include <stdlib.h>
#include <string.h>
#include "lc_all.h"
#include "ble_apl.h"
#include "ble_local.h"

//================
//hplc 接收事件，回调处理；
volatile uint32_t plc_receive_success_bus_frame_cnt = 0;
volatile uint32_t plc_receive_success_sta_frame_cnt = 0;
volatile uint32_t plc_receive_success_sta_frame_process_cnt = 0;

bool LcRfPowerCheckAndSet(u8 *data)
{
	u8 valid_count = 0;

    for (u8 i = 0; i < 2; i++)
    {
        if (data[i] == 0)
        {
            valid_count += 1;
        }
        else if (data[i] <= STAII_MAX_RF_POWER) // 204 & 205 rf最大功率 15
        {
            valid_count += 2;
        }
    }

    if ((valid_count == 2) || (valid_count == 4)) 
    {
        if (memIsHex(data, 0x00, 2)) // 数值全0
        {
            StaFixPara.txPowerMode[1] = 0; // 发射功率自动调整模式
            memset(StaFixPara.hrfTxPower, 0x00, 2);
        }
        else
        {
            StaFixPara.txPowerMode[1] = 1; // 发射功率固定模式
            memcpy(StaFixPara.hrfTxPower, data, 2);
        }
        StorageStaFixPara();
        StaTxPowerProcess();
        HRF_SetTxGain(HRF_ChlPower[(HRFCurrentIndex >> 8) & 0x1]);
        return true;  
    } 
    else
    {
        return false;
    }
}

bool LcPlcPowerCheckAndSet(u8 *data)
{
	u8 valid_count = 0;

    for (u8 i = 0; i < 4; i++)
    {
        if (data[i] == 0)
        {
            valid_count += 1;
        }
        else if (data[i] <= STAII_MAX_PLC_POWER) // 204 & 205 plc最大功率13
        {
            valid_count += 2;
        }
    }

    if ((valid_count == 4) || (valid_count == 8)) 
    {
        if (memIsHex(data, 0x00, 4)) // 数值全0
        {
            StaFixPara.txPowerMode[0] = 0; // 发射功率自动调整模式
            memset(StaFixPara.txPower, 0x00, 4);
        }
        else
        {
            StaFixPara.txPowerMode[0] = 1; // 发射功率固定模式
            memcpy(StaFixPara.txPower, data, 4);
        }
        StorageStaFixPara();
        StaTxPowerProcess();
        BPLC_SetTxGain(HPLC_ChlPower[HPLCCurrentFreq]);
        return true;  
    } 
    else
    {
        return false;
    }
}



void LcAppRebootTimerCallback(void *arg)
{
	REBOOT_SYSTEM();
}


u16 lc_645CreateFrame(u8 *data,u16 len)
{    
	lcFrame645 *pframe = (lcFrame645 *)data;
    pframe->datalen = len;
    pframe->control_code.control_bits.direction_flag = 1;
    *(pframe->data + pframe->datalen) = GetCheckSum(data, len + LC_645_CS_POSITION);
    *(pframe->data + pframe->datalen + 1) = LC_645_FT;
	return len + LC_645_BASE_LEN;
}


bool lc_IsHave645Frame(u8*data,u16 len,u16 *hindex)
{
    u16 index = 0;
    bool isFind = false;
    if(len<12)return false;
    u16 endindex = (len-11);
    for(index = 0;index < endindex;index++)
    {
        if(data[index] == 0x68)
        {
            isFind = true;
            break;
        }
		else if(data[index] != 0xFE)
		{
			break;
		}
    }
    if(!isFind)
        return false;
    if(len - index < 12)
        return false;
    if(data[index] != 0x68 && data[index + 7] != 0x68)
        return false;
    
    u16 framelen = data[index+9];
    framelen += 12;
    if((len - index) < framelen)return false;
    if(data[index + framelen - 1] != 0x16)return false;
    
    if(GetCheckSum(&data[index],framelen-2) != data[index + framelen - 2])return false;
    *hindex = index;
    return true;
}

u16 lc_protocol645_dispose_IFR(u8 *data)
{
	lcFrame645 *pframe = (lcFrame645 *)data;
	u16 len = 0;
	u16 result = 0;
	u16 id = data[LC_645_ID];
    switch(pframe->control_code.control_byte)
    {    
        case LC_645_CMD://
        {        	
			if(id == 0x01)//设置Mac地址  ask: 68 AA AA AA AA AA AA 68 00 07 01 xx xx xx xx xx xx cs 16  
			{
				memcpy(&StaChipID.mac[0],&pframe->data[1],6);		
				StorageStaIdInfo();		
				ReadStaIdInfo();				
				memcpy(&pframe->addr[0],&StaChipID.mac[0],6);
		        len = 1;
			}
			else if (id == 0x02)//读取Mac地址  ask: 68 AA AA AA AA AA AA 68 00 01 02 cs 16  
			{
				ReadStaIdInfo();				
				memcpy(&pframe->addr[0],&StaChipID.mac[0],6);
		        len = 1;
		        
			}
			
			else if (id == 0x03)//读取内部版本  ask: 68 AA AA AA AA AA AA 68 00 01 03 cs 16   
			{
				ReadStaIdInfo();
				memcpy(&pframe->addr[0],&StaChipID.mac[0],6);
				pframe->data[1] = (LC_APP_VER >> 8) & 0xff;
				pframe->data[2] =  LC_APP_VER & 0xff;
		        len = 3;		       
			}

			if(id == 0x04)//设置RF功率  
			{
				memcpy(&pframe->addr[0],&StaChipID.mac[0],6);
				LcRfPowerCheckAndSet(&pframe->data[1]);
				len = 1;	
			}
			else if (id == 0x05)//读取RF功率  
			{
				ReadStaFixPara();
				memcpy(&pframe->addr[0],&StaChipID.mac[0],6);
				len = 4;
				memcpy(&pframe->data[1],StaFixPara.hrfTxPower,2);
				pframe->data[3] = StaFixPara.txPowerMode[1];
   
			}
			if(id == 0x06)//设置PLC功率  
			{
				memcpy(&pframe->addr[0],&StaChipID.mac[0],6);
				LcPlcPowerCheckAndSet(&pframe->data[1]);
				len = 1;

			}
			else if (id == 0x07)//读取PLC功率  
			{
				ReadStaFixPara();
				memcpy(&pframe->addr[0],&StaChipID.mac[0],6);
				len = 6;
				memcpy(&pframe->data[1],StaFixPara.txPower,4);
				pframe->data[5] = StaFixPara.txPowerMode[0];
			}
			break;
		} 
		default:
			break;
            
    }
	result = lc_645CreateFrame(data,len);	
	return result;	
}

//发送网络数据
lc_net_err_t lc_net_tx_data(const uint8_t* to_mac,const void *msg, const uint32_t len,const uint32_t flg)
{
	uint16_t to_tei;
	lc_net_err_t result = LC_NET_ERROR;
	bool broadcastmode = false;
	//
	const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();
	if(pNetBasePara->NetState != NET_IN_NET)return LC_NET_ERROR;
	//
	uint8_t tran_to_mac[LONG_ADDR_SIZE];
	memcpy(tran_to_mac,to_mac,LONG_ADDR_SIZE);
	TransposeAddr(tran_to_mac);
	//
	if(0 == memcmp(tran_to_mac,pNetBasePara->CcoMac,LONG_ADDR_SIZE))
	{
		to_tei = CCO_TEI;
		broadcastmode = false;
	}
	else if(0 == memcmp(tran_to_mac,BroadCastMacAddr,LONG_ADDR_SIZE))
	{//广播
		to_tei = MAC_BROADCAST_TEI;
		broadcastmode = true;
	}
	else
	{
		return LC_NET_EINVAL;
	}

	uint16_t mac_seq = MacGetMacSequence();
	uint8_t macbroadcastDirection = BROADCAST_DRT_UP;
	

	SEND_PDU_BLOCK* pdu = MallocSendPDU();
	if(pdu == NULL)
	{
		debug_str(DEBUG_LOG_NET, "%s %d malloc err\r\n",__func__,__LINE__);
		result = LC_NET_ENOMEM;
		return result;
	}

	APP_PACKET* sendAppPacket = (APP_PACKET*)pdu->pdu;      
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET*)&sendAppPacket->Data[0];
	
	CreateAppPacket(sendAppPacket);        
	CreateAppBusinessPacket(sendAppGeneralPacket,APP_FRAME_DEBUG,0,1,0,1,APP_BUS_TRAN_MODULE,GetAppSequence(),len,msg); 	
	pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
	MacHandleApp(pdu->pdu, len + LEN_APP_PACKET, 2, to_tei, (broadcastmode == true)?SENDTYPE_BROADCAST:SENDTYPE_SINGLE, macbroadcastDirection, mac_seq);
	FreeSendPDU(pdu);
	
	return result;
}

lc_net_err_t lc_net_tx_data_to_tei_broadcast(const uint16_t to_tei,const void *msg, const uint32_t len,const uint32_t flg)
{
	lc_net_err_t result = LC_NET_ERROR;
	uint16_t mac_seq = MacGetMacSequence();
	uint8_t macbroadcastDirection = BROADCAST_DRT_DOWN;
	
	
	SEND_PDU_BLOCK* pdu = MallocSendPDU();
    bool rflg = false;
    if(pdu == NULL)
        return rflg;
	APP_PACKET* sendAppPacket = (APP_PACKET*)pdu->pdu;      
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET*)&sendAppPacket->Data[0];
	
	CreateAppPacket(sendAppPacket);        
	CreateAppBusinessPacket(sendAppGeneralPacket,APP_FRAME_DEBUG,0,1,0,1,APP_BUS_TRAN_MODULE,GetAppSequence(),len,msg); 	
	pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
	MacHandleApp(pdu->pdu, len + LEN_APP_PACKET, 2, to_tei, SENDTYPE_LOCAL_ACK, macbroadcastDirection, mac_seq);
	FreeSendPDU(pdu);

	return result;
}
/*
uint32_t time_records[10] = {0};
uint32_t time_record_index = 0;
void save_time_record(uint32_t time_now)
{
	if(time_record_index >= 10)
		time_record_index = 0;

	time_records[time_record_index] = time_now;
	time_record_index++;
}
*/
extern uint16_t OperateMeter(uint8_t *send_buf, uint8_t *src_data, uint16_t src_len,uint32_t meter_baudrate,uint8_t meter_parity);
extern uint32_t meter_baudrate;
extern uint32_t GetUartRate(uint8_t baudrate_type);
extern bool IsBleMeter(uint8_t *src_data, uint16_t src_len);

#define WHI_SYSCTRL_READ_METER (0x13)

//whi_sysctrl_type g_userdata_head;

bool whi_read_ble_breaker_add_uart_task_cb_fun(void * exparam,u8* psrc,u16 len,u8 state)
{
	bool r_flg = false;
	if(exparam == NULL)
	{
		debug_str(DEBUG_LOG_NET, "%s %d err\r\n",__func__,__LINE__);
		return true;
	}
	if(psrc == NULL || len == 0)
	{
		debug_str(DEBUG_LOG_NET, "%s %d err\r\n",__func__,__LINE__);
		return true;
	}
	
	if(WHI_IsLocalWHIFrame(psrc,len))
	{
		uint16_t framecmd = (((uint16_t)psrc[3])<<8)|psrc[2];
	    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
	    uint8_t *pdst = &psrc[8];
						
		uint8_t *send_buff = malloc(WHI_HPLC_ADAPT_HEAD_LEN + 16 + 1024);
		uint16_t send_len;
		whi_sysctrl_type *userdata_head;
		operate_meter_task_param_t *param = exparam;
		//Dir： Dir=0 表示此帧报文是由主控设备发出的下行报文；
		//Dir=1 表示此帧报文是由通信模组发出的上行报文。
		//Prm： Prm=1 表示此帧报文来自启动站；
		//Prm=0 表示此帧报文来自从动站。
		if(send_buff != NULL)
		{
			if((psrc[1]&0x40) == 0x00)
			{//从动站报文
				switch(framecmd)
				{
					case WHI_READ_BLE_BREAKER_FRAME:
						//save_time_record(Time_Module[0].sysTick);
						memset(send_buff,0,WHI_HPLC_ADAPT_HEAD_LEN + 16 + 1024);
						userdata_head = (whi_sysctrl_type*)&send_buff[WHI_HPLC_ADAPT_HEAD_LEN];
						memcpy(userdata_head->mac_add, param->mac, LC_NET_ADDR_SIZE);
						userdata_head->user_len = framelen + 8;
						userdata_head->plc_pro = param->plc_pro;
						userdata_head->seq_num = param->seq_num;
						userdata_head->fun_code = param->fun_code;
						userdata_head->stat_code = 0;
						memcpy(&send_buff[WHI_HPLC_ADAPT_HEAD_LEN + 16], pdst, framelen);
						send_len = whi_hplc_adapt_create(WHI_SEND_BUS_FRAME,send_buff,
															&send_buff[WHI_HPLC_ADAPT_HEAD_LEN], 
															framelen + 16);
						lc_net_tx_data(param->mac,send_buff, send_len, LC_NET_NOT_BLOCK);
						break;
					default:
						break;
				}
			}
			free(send_buff);
			r_flg = true;
		}
	}
	return r_flg;
}

extern bool FindMeterComplete;
uint8_t  AppHandDataTempBuf[WHI_HPLC_ADAPT_HEAD_LEN + 16 + 1024];
/**
 * [LcAppHandleData    LCIP处理]
 * @author    ginvc
 * @dateTime  2022-10-16
 * @attention none
 * @param     data      [数据 u8*data]
 * @param     len       [数据长度 u16len ]
 * @return    none        []    
 */
void LcAppHandleData(u8 *data, u16 len)
{
	MAC_HEAD *macHead = (MAC_HEAD*)(data);
	uint8_t* appPacket = 0;
	uint8_t src_mac[LONG_ADDR_SIZE];


	if(macHead->HeadType)
	{
		appPacket = (uint8_t*)((u8*)macHead + sizeof(MAC_HEAD_SHORT) + sizeof(MSDU_HEAD_SHORT) +  LEN_APP_PACKET);
		if(macHead->SrcTei == CCO_TEI)
		{
			const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();
			memcpy(src_mac,pNetBasePara->CcoMac,sizeof(src_mac));
		}
		else
		{
			memset(src_mac,0xFF,sizeof(src_mac));
		}
	}
	else
	{
		//MAC_HEAD_LONG *macheadlong = (MAC_HEAD_LONG*)macHead;
		MSDU_HEAD_LONG * pmsdu_head_long= (MSDU_HEAD_LONG*)((u8*)macHead + sizeof(MAC_HEAD_LONG));
		memcpy(src_mac,pmsdu_head_long->SrcMac,sizeof(src_mac));
		appPacket = (uint8_t*)((u8*)macHead + sizeof(MAC_HEAD_LONG) + sizeof(MSDU_HEAD_LONG) + LEN_APP_PACKET);
	}
	TransposeAddr(src_mac);
	len -= (uint32_t)(appPacket - (data));
    len = len >= macHead->MsduLen?macHead->MsduLen:len;

	if(len > 0)
	{
		uint32_t whi_cmd = 0;
		uint32_t p_dst_len = len;
		uint8_t * p_dst = whi_hplc_adapt_process(&whi_cmd,(uint8_t*)appPacket, &p_dst_len);

		
		lc_net_my_info_t p_info;
		lc_net_get_net_self_info(&p_info);
		switch (whi_cmd) // 目前支持 0x0120 0x101 0x111 0x103
		{
			case WHI_RECEIVE_STA_FRAME:
			{
				// #ifdef PLAT_LAMP_NET
				if(0 == memcmp(p_dst,BroadCastMacAddr,LC_NET_ADDR_SIZE))
				{//广播转发
					ble_apl_send_local_cmd_frame(p_dst,p_dst,p_dst_len,LC_NET_NOT_BLOCK);
				}				
				// #endif
				plc_receive_success_sta_frame_cnt++;
				if(WHI_IsLocalWHIFrame(&p_dst[8],p_dst_len-8))
				{
					// #ifdef PLAT_LAMP_NET  
					if(((p_dst[9]&0x80) == 0x00)&&((p_dst[9]&0x40) == 0x40))
					{//主动
						uint16_t framecmd = (((uint16_t)p_dst[11])<<8)|p_dst[10];
						if(framecmd == WHI_SEND_BUS_FRAME)
						{
							ble_net_ack_send_data_port = BLE_NET_ACK_SEND_PLC_PORT;
							StartTimer(&ble_net_ack_send_data_port_timer, ACK_SEND_PORT_CHECK_TIME); 
						}
					}
					// #endif
					plc_receive_success_sta_frame_process_cnt++;
					uint16_t ack_len = WHI_UartLocalWHIProcess(&p_dst[8],p_dst_len-8,LOCAL_BLE_NET_PROCESS_MODE);
					if(ack_len != 0)
					{
						memcpy(p_dst,src_mac,LC_NET_ADDR_SIZE);
						p_dst[6] = ack_len&0xFF;
						p_dst[7] = (ack_len>>8)&0xFF;
						ack_len += 8;
						uint8_t* send_buff = malloc(WHI_HPLC_ADAPT_HEAD_LEN+ack_len+32);
						if(send_buff == NULL)
						{
							return;
						}
						uint16_t send_len = whi_hplc_adapt_create(WHI_SEND_STA_FRAME,send_buff,p_dst,ack_len);
						lc_net_tx_data(src_mac,send_buff,send_len,LC_NET_NOT_BLOCK);
						free(send_buff);
					}
				}

			}
			break;
			case WHI_SEND_BUS_FRAME:
			{
#if 1			
				whi_sysctrl_type *sysctrl_head = (whi_sysctrl_type*)p_dst;
				uint8_t m_addr[6] = {0};
				memcpy(m_addr,sysctrl_head->mac_add,6);
				uint8_t bd;
				uint8_t parity;
				TransposeAddr(m_addr);
				bool is_meter = IsSystemMeter(m_addr, &bd, &parity);

				if((p_dst_len >= (8)) && ((0 == memcmp(p_dst,p_info.addr,LC_NET_ADDR_SIZE))) || is_meter)
				{
					//meter_baudrate = GetUartRate(bd);
					if((sysctrl_head->plc_pro == 0x0001)//plc 版本号判断
						&& (whi_group_sysctrl_addr_check(sysctrl_head->dev_addr)))
					{
						switch(sysctrl_head->fun_code)
						{
							case WHI_SYSCTRL_READ_METER:
							{
								uint8_t *send_buff = (uint8_t *)&AppHandDataTempBuf;
								memset(send_buff,0,WHI_HPLC_ADAPT_HEAD_LEN + 16 + 1024);
								if(false == IsBleMeter(sysctrl_head->data_body, sysctrl_head->user_len - 8))
								{
									if(FindMeterComplete == false) 
									{
										return ;
									}
									
									operate_meter_task_param_t param = {0};
									memcpy(param.mac, src_mac,LC_NET_ADDR_SIZE);
									param.meter_baudrate = GetUartRate(bd);
									param.meter_parity = parity;
									param.plc_pro = sysctrl_head->plc_pro;
									param.fun_code = sysctrl_head->fun_code;
									param.seq_num = sysctrl_head->seq_num;
									param.data_len = sysctrl_head->user_len - 8;
									param.src_data = malloc(param.data_len);
									debug_str(DEBUG_LOG_NET, "WHI_SEND_BUS_FRAME %d %d \r\n",param.meter_baudrate,param.meter_parity);
									if(param.src_data != NULL)
									{
										memcpy(param.src_data,sysctrl_head->data_body,param.data_len);
										bool ret = AddOperateMeterTask(&param);
										if(ret == false) free(param.src_data);
									}	
									else
									{
										debug_str(DEBUG_LOG_NET, "%s %d malloc err\r\n",__func__,__LINE__);
									}
								}
								else
								{
									
									//save_time_record(Time_Module[0].sysTick);
									uint16_t seq = WHI_CreateLocalSeq();
									uint16_t send_len = WHI_BuildFrameProcess(send_buff,false,WHI_READ_BLE_BREAKER_FRAME,seq,
																				sysctrl_head->data_body, 
																				sysctrl_head->user_len - 8);
									if(send_len >= WHI_PRO_HEAD_SIZE)
									{
										operate_meter_task_param_t *param = (operate_meter_task_param_t *)malloc(sizeof(operate_meter_task_param_t));
										if(param != NULL)
										{
											memcpy(param->mac, src_mac,LC_NET_ADDR_SIZE);
											param->plc_pro = sysctrl_head->plc_pro;
											param->fun_code = sysctrl_head->fun_code;
											param->seq_num = sysctrl_head->seq_num;
											bool ret = local_send_fram_add_task(send_buff,send_len,SEND_LOCAL_ACK_PRIO_VALUE + 1,150,0,whi_read_ble_breaker_add_uart_task_cb_fun,param);
											if(ret == false) free(param);
										}
										else
										{
											debug_str(DEBUG_LOG_NET, "%s %d malloc err\r\n",__func__,__LINE__);
										}
									}
								}
								return;
							}
							default:
								break;
						}
					}
				}
#endif

				if(0 == memcmp(p_dst,BroadCastMacAddr,LC_NET_ADDR_SIZE))
				{
					//广播转发
					ble_apl_send_system_ctrl_cmd_frame(p_dst,p_dst,p_dst_len,LC_NET_NOT_BLOCK);
				}
				
				uint8_t *send_buff = malloc (WHI_HPLC_ADAPT_HEAD_LEN+1024+32);
				if(send_buff != NULL)
				{		
					memcpy(&send_buff[WHI_HPLC_ADAPT_HEAD_LEN],p_dst,p_dst_len);
					memcpy(&send_buff[WHI_HPLC_ADAPT_HEAD_LEN],src_mac,LC_NET_ADDR_SIZE);	
					
					uint16_t rdlen = 0;
					if(0 == memcmp(p_dst,p_info.addr,LC_NET_ADDR_SIZE))
					{
						rdlen = whi_sysctrl_process_frame(&send_buff[WHI_HPLC_ADAPT_HEAD_LEN],p_dst_len,BROCAST_ADDR_INVALID);
					}
					else
					{
						rdlen = whi_sysctrl_process_frame(&send_buff[WHI_HPLC_ADAPT_HEAD_LEN],p_dst_len,BROCAST_ADDR_VALID);
					}
					if(rdlen > 0)
					{
						uint16_t send_len = whi_hplc_adapt_create(WHI_SEND_BUS_FRAME,send_buff,&send_buff[WHI_HPLC_ADAPT_HEAD_LEN],rdlen);
						lc_net_tx_data(src_mac,send_buff,send_len,LC_NET_NOT_BLOCK);
					}
					free(send_buff);
				}
			}break;

			case WHI_RECEIVE_HPLC_FRAME:
			case WHI_RECEIVE_STA_NO_ACK_FRAME:
			{
				uint8_t* send_buff = malloc(WHI_PRO_HEAD_SIZE+len+32);
				if(send_buff == NULL)
				{
					#ifdef PLAT_II_485_READ_METER
					status_hplc_led_flash(false);
					#endif
					return;
				}
				memcpy(p_dst,src_mac,LC_NET_ADDR_SIZE);
				//
				//void TransposeAddr(uint8_t*);
				//TransposeAddr(p_dst);
				//
				uint16_t send_len = WHI_BuildFrameProcess(send_buff,false,whi_cmd&0xFFFF,WHI_CreateLocalSeq(),p_dst,p_dst_len);
				if(send_len >= WHI_PRO_HEAD_SIZE)
				{
					local_send_fram_add_task(send_buff,send_len,SEND_METER_ACK_DATA_PRIO_VALUE,0,0,NULL,NULL);
				}
				free(send_buff);
			}
			break;		
			default:
				break;
		}
	}
	return;
}

//抄表最大数
#define OPERATEMETERTASKNUMMAX 5
//抄表信号量
static OS_SEM OperateMeterSEM;
//抄表参数，用于回复帧
operate_meter_task_param_t OperateMeterParam[OPERATEMETERTASKNUMMAX];
//当前抄表数
int OperateMeterTaskRunNum = 0;

//初始化抄表任务
void InitOperateMeterTask()
{
	//debug_str(DEBUG_LOG_NET, "%s %d\r\n",__func__,__LINE__);
	OS_ERR err;
	CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();

    for (int i = 0; i < OPERATEMETERTASKNUMMAX; i++)
    {
        memset(&OperateMeterParam[i],0,sizeof(operate_meter_task_param_t));
    }
    OperateMeterTaskRunNum = 0;

	OS_CRITICAL_EXIT();

	// 创建信号量
	OSSemCreate(&OperateMeterSEM, "OperateMeterSEM", 0, &err);
	if (err != OS_ERR_NONE)
	{
		debug_str(DEBUG_LOG_NET, "%s %d err\r\n",__func__,__LINE__);
		while (1);
	}
}

//添加抄表任务
bool AddOperateMeterTask(operate_meter_task_param_t *param)
{
	//debug_str(DEBUG_LOG_NET, "%s %d\r\n",__func__,__LINE__);
    bool rflg = false;
	OS_ERR err;
	
    if (param == NULL)
	{
		debug_str(DEBUG_LOG_NET, "%s %d err\r\n",__func__,__LINE__);
        return false;
	}

	// 进入临界区
	CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();

    if (OperateMeterTaskRunNum < OPERATEMETERTASKNUMMAX)
    {
        for (int i = 0; i < OPERATEMETERTASKNUMMAX; i++)
        {
            if (OperateMeterParam[i].task_status == TASK_IDLE)
            {
            	param->task_status = TASK_RUN;
                memcpy(&OperateMeterParam[i],param,sizeof(operate_meter_task_param_t));
                ++OperateMeterTaskRunNum;
                rflg = true;
                break;
            }
        }
    }
    
	// 退出临界区
	OS_CRITICAL_EXIT();
	OSSemPost(&OperateMeterSEM, OS_OPT_POST_1, &err);
    return rflg;
}

//删除抄表任务
bool DeleteOperateMeterTask(u8 taskindex)
{
	//debug_str(DEBUG_LOG_NET, "%s %d\r\n",__func__,__LINE__);
    bool rflg = false;
    if (taskindex >= OPERATEMETERTASKNUMMAX)
        return false;
    if (OperateMeterParam[taskindex].task_status == TASK_IDLE)
        return false;
	
	if(OperateMeterParam[taskindex].src_data != NULL)
	{
		free(OperateMeterParam[taskindex].src_data);
		OperateMeterParam[taskindex].src_data = NULL;
	}

	// 进入临界区
	CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
	
	memset(&OperateMeterParam[taskindex],0,sizeof(operate_meter_task_param_t));
    OperateMeterTaskRunNum--;
    rflg = true;

	// 退出临界区
	OS_CRITICAL_EXIT();

    return rflg;
}

uint8_t OperateMeterSendBuf[WHI_HPLC_ADAPT_HEAD_LEN + 16 + 1024];

//抄表任务执行
void OperateMeterTask(void *arg)
{
	OS_ERR err;
	while (1)
	{
		OSSemPend(&OperateMeterSEM, 0, OS_OPT_PEND_BLOCKING, 0, &err);

		if(err == OS_ERR_NONE)
		{
			//debug_str(DEBUG_LOG_NET, "%s %d\r\n",__func__,__LINE__);
			for (int i = 0; i < OPERATEMETERTASKNUMMAX; i++)
	        {
	            if (OperateMeterParam[i].task_status == TASK_RUN)
	            {
	            	uint8_t *send_buff = (uint8_t *)&OperateMeterSendBuf;
					memset(send_buff,0,WHI_HPLC_ADAPT_HEAD_LEN + 16 + 1024);
					uint16_t rdlen = OperateMeter(&send_buff[WHI_HPLC_ADAPT_HEAD_LEN + 16],OperateMeterParam[i].src_data,OperateMeterParam[i].data_len,OperateMeterParam[i].meter_baudrate,OperateMeterParam[i].meter_parity);
					if(rdlen > 0)
					{
						whi_sysctrl_type *userdata_head = (whi_sysctrl_type*)&send_buff[WHI_HPLC_ADAPT_HEAD_LEN];
						memcpy(userdata_head->mac_add, OperateMeterParam[i].mac, LC_NET_ADDR_SIZE);
						userdata_head->user_len = rdlen + 8;
						userdata_head->plc_pro = OperateMeterParam[i].plc_pro;
						userdata_head->seq_num = OperateMeterParam[i].seq_num;
						userdata_head->fun_code = OperateMeterParam[i].fun_code;
						userdata_head->stat_code = 0;
						uint16_t send_len = whi_hplc_adapt_create(WHI_SEND_BUS_FRAME,send_buff,
															&send_buff[WHI_HPLC_ADAPT_HEAD_LEN], 
															rdlen + 16);
						lc_net_tx_data(OperateMeterParam[i].mac,send_buff, send_len, LC_NET_NOT_BLOCK);
					}
					else
					{
						debug_str(DEBUG_LOG_NET, "%s %d err %d \r\n",__func__,__LINE__,rdlen);
					}
					DeleteOperateMeterTask(i);
	                break;
	            }
	        }
		}
	}
}

#define  TEI_ZERO 0
#define  TEI_BC 0xFFF
#define  NID_ZERO 0
#define  REBOOT_TIMES_ZERO 0
#define  ROUTE_LEFT_ZERO 0
#define  SEND_LIMIT_THREE 3
#define  LID_THREE 3
#define  BC_FLG_ON 1
#define  RESEND_FLG_OFF 0
extern uint32_t crc24_cal(uint8_t *data, uint32_t len);

u16 LC_MakeMpdu(u8* macFrame,u16 len,u8* blockLoad,u16 *blockLoadLen,u32 tmi,u8 blockNum)
{
	u8 i = 0;
	u16 blockSize = CopyModelBlockSize[tmi];		
	u16 blockPhySize = blockSize - 8;				
	u8* headMpdu = 0;
	u16 leftLen = len;
	*blockLoadLen = 0;
	for(i = 0;i < blockNum;i++)
    {
        u32 crc24;
        headMpdu = i * blockSize + blockLoad;
        PHY_BLOCK_HEAD *head = 0;
        u8* phyLoad = 0;
        head = (PHY_BLOCK_HEAD *)headMpdu;                  
        phyLoad = (u8*)((u8*)head + sizeof(PHY_BLOCK_HEAD));    
        head->Seq = i;
        head->Reserve = 0;
        *blockLoadLen += CopyModelBlockSize[tmi];
        if(leftLen > blockPhySize)//
        {
            memcpy(phyLoad,macFrame,blockPhySize);
            phyLoad[blockPhySize] = 0;      
            crc24 = crc24_cal((u8*)head,blockPhySize + 4 +1);      
            memcpy(phyLoad + blockPhySize + 1,&crc24,3);     
            macFrame += blockPhySize;
            leftLen = leftLen - blockPhySize;
        }
        else
        {
            memcpy(phyLoad,macFrame,leftLen);
            memset(phyLoad + leftLen,0,blockPhySize - leftLen);   
            phyLoad[blockPhySize] = 0;      
            crc24 = crc24_cal((u8*)head,blockPhySize + 4 + 1);
            memcpy(phyLoad + blockPhySize + 1,&crc24,3); 
            macFrame += blockPhySize;
            leftLen = 0;
        }
    }
	return leftLen;
}
extern void tool_test_callback(int status);
extern void send_callback(bool_t status);

extern void tool_test_OptionCallback(uint32_t IsOpen,uint32_t phase);

static u8 lc_plc_diagnosis_data[1024]  = {0};
#define TEST_PB_SIZE 3
#define TEST_MCS 2
char test_buf[200] = {0};

// 该接口不考虑模块网络状态  以本地广播形式发送报文
void LC_MpduHandleApp(u8* data,u16 len,u8 pri,u16 dst,u8 broadcastType,u8 broadcastDirection,u32 useHopMac,u8 sendtype)
{
	debug_str(DEBUG_LOG_NET, "LC_MpduHandleApp %d\r\n",sendtype);
	SEND_PDU_BLOCK* MacPdu = MallocSendPDU();
	if(MacPdu == NULL)
	{
		return;
	}
	MAC_HEAD_SHORT  *macShortHead = (MAC_HEAD_SHORT*)MacPdu->pdu;
    MSDU_HEAD_SHORT *msduShortHead = (MSDU_HEAD_SHORT*)((u8*)macShortHead + sizeof(MAC_HEAD_SHORT));
    u8* macLoad = (u8*)msduShortHead + sizeof(MSDU_HEAD_SHORT);
	

	CreateShortMACHead(macShortHead,sizeof(MSDU_HEAD_SHORT) + len,dst,TEI_ZERO, NID_ZERO,REBOOT_TIMES_ZERO,
                       ROUTE_LEFT_ZERO,BROADCAST_DRT_UNKNOW,SENDTYPE_LOCAL,SEND_LIMIT_THREE,MacGetMacSequence());
	
	uint8_t valan= 3;
	
	
    CreateShortMsduHead(msduShortHead,valan,MSDU_APP_FRAME);
	memcpy(macLoad,data,len);
	CreateMACCrc32(msduShortHead,macShortHead->MacHead.MsduLen);
    MacPdu->size = sizeof(MAC_HEAD_SHORT) + macShortHead->MacHead.MsduLen + 4;
	
	SEND_PDU_BLOCK* sendMpdu = NULL;
	sendMpdu = MallocSendPDU();
	if(sendMpdu == NULL)
	{
		return ;
	}

	u16 mpduLen = 0;
	//当前使用的分级拷贝模式
	u32 tmi = DIV_COPY_4;
	if (len <= 136)
	{
		tmi = DIV_COPY_4;
	}
	else
	{
		tmi = DIV_COPY_9;
	}
	
	//当前一帧中的物理块数
	u8 blockNum = 1;

	MPDU_CTL *mpduCtl = (MPDU_CTL *)&sendMpdu->pdu[0];
	

	uint32_t symbol_num = HPLC_PHY_GetSymbolNum(tmi,0,blockNum);
	uint32_t frame_len = HPLC_PHY_GetFrameLen(MPDU_TYPE_SOF, symbol_num);

	//组MPDU
	CreateMPDUCtl(mpduCtl,MPDU_TYPE_SOF,NID_ZERO);
	if(sendtype == 1)
	{
		SOF_ZONE *sofZone = (SOF_ZONE *)(mpduCtl->Zone0);
		CreateSOFZone(sofZone,TEI_ZERO,TEI_BC,LID_THREE,frame_len,blockNum,symbol_num,BC_FLG_ON,RESEND_FLG_OFF,tmi,0);
	}
	else
	{
		SOF_HRF_ZONE *sofZone = (SOF_HRF_ZONE *)(mpduCtl->Zone0);
		memset((sofZone),0,sizeof(SOF_HRF_ZONE));\
	    sofZone->SrcTei = (TEI_ZERO);\
	    sofZone->DstTei = (TEI_BC);\
	    sofZone->LinkFlag = (LID_THREE);\
	    sofZone->FrameLen = (frame_len); \
	    sofZone->PbSize = (TEST_PB_SIZE);  \
	    sofZone->MCS = (TEST_MCS);\
	    sofZone->BroadcastFlag = (BC_FLG_ON); \
	    sofZone->ReSendFlag = (RESEND_FLG_OFF);
	}
	
	
	LC_MakeMpdu(MacPdu->pdu,MacPdu->size ,&sendMpdu->pdu[sizeof(MPDU_CTL)],&mpduLen,tmi,blockNum);
	mpduLen += sizeof(MPDU_CTL);
	sendMpdu->size = mpduLen;
	memcpy(lc_plc_diagnosis_data,sendMpdu->pdu,sendMpdu->size);
	memset(test_buf,0,200);
	for(int i=0;i<80&&i<sendMpdu->size;i++)
	{
		sprintf(&test_buf[i*2],"%02x",lc_plc_diagnosis_data[i]);
	}
	debug_str(DEBUG_LOG_UPDATA, "sbuf:%s\r\n",test_buf);
	if(sendtype == 1)
	{
		debug_str(DEBUG_LOG_NET, "LC_MpduHandleApp plc %d\r\n",sendMpdu->size);
		BPLC_SendImmediate(lc_plc_diagnosis_data,sendMpdu->size,tool_test_callback,tool_test_OptionCallback,1);
	}
	else
	{
		debug_str(DEBUG_LOG_NET, "LC_MpduHandleApp rf \r\n");
		HRF_SendImmediate(2, 2,lc_plc_diagnosis_data,3,send_callback);
	}
	FreeSendPDU(MacPdu);
	FreeSendPDU(sendMpdu);
}


//发送数据(不判断网络状态，广播发送)
lc_net_err_t lc_broadcast_tx_data(const void *msg, const uint32_t len,uint8_t sendtype)
{
	uint16_t to_tei;
	lc_net_err_t result = LC_NET_ERROR;
	bool broadcastmode = true;
	const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();
	uint8_t tran_to_mac[LONG_ADDR_SIZE];
	memset(tran_to_mac,0,LONG_ADDR_SIZE);
	TransposeAddr(tran_to_mac);
	to_tei = MAC_BROADCAST_TEI;
	uint16_t mac_seq = MacGetMacSequence();
	uint8_t macbroadcastDirection = BROADCAST_DRT_UNKNOW;
	
	SEND_PDU_BLOCK* pdu = MallocSendPDU();
	if(pdu == NULL)
	{
		result = LC_NET_ENOMEM;
		return result;
	}

	APP_PACKET* sendAppPacket = (APP_PACKET*)pdu->pdu;      
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET*)&sendAppPacket->Data[0];
	CreateAppPacket(sendAppPacket);        
	CreateAppBusinessPacket(sendAppGeneralPacket,APP_FRAME_DEBUG,0,1,0,1,APP_BUS_TRAN_MODULE,GetAppSequence(),len,msg); 	
	pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
	LC_MpduHandleApp(pdu->pdu, len + LEN_APP_PACKET, 2, to_tei, (broadcastmode == true)?SENDTYPE_BROADCAST:SENDTYPE_SINGLE, macbroadcastDirection, mac_seq ,sendtype);
	debug_str(DEBUG_LOG_NET, "lc_broadcast_tx_data %d\r\n",sendtype);
	FreeSendPDU(pdu);
	
	return result;
}

void broadcast_msg_test_send(uint8_t sendtype,uint8_t *msg,uint8_t msg_len)
{
	uint8_t *send_buff = malloc (WHI_HPLC_ADAPT_HEAD_LEN+1024+32);
	if(send_buff != NULL)
	{		
		uint16_t send_len = whi_hplc_adapt_create(WHI_PLC_DIAGNOSIS,send_buff,msg,msg_len);
		lc_broadcast_tx_data(send_buff,send_len,sendtype);
		free(send_buff);
	}
}

u32 g_broadcast_msg_test_recv_plc_num = 0;
u32 g_broadcast_msg_test_recv_rf_num = 0;

bool MpduHandleMPDU_Diagnosis(PDU_BLOCK* pdu,u8 recvtype)
{  
    bool result = false;
    MPDU_CTL *mpduCtl =  (MPDU_CTL *)pdu->pdu;

    if(mpduCtl->Type == MPDU_TYPE_SOF)
    {
        u8* data = (u8*)mpduCtl + sizeof(MPDU_CTL) + sizeof(PHY_BLOCK_HEAD);
        APP_CREATE_MAC_DEF();
        if(appGeneral->Ctrl.FrameType == APP_FRAME_DEBUG)
        {
        	uint32_t whi_cmd = 0;
            uint32_t p_dst_len = appGeneral->AppFrameLen; 
            uint8_t * p_dst = whi_hplc_adapt_process(&whi_cmd,(uint8_t *)appGeneral->Data, &p_dst_len); 
            if(whi_cmd == WHI_PLC_DIAGNOSIS)
            {
            	char buf[60] = {0};
				for(int i=0;i<30&&i<p_dst_len;i++)
				{
					sprintf(&buf[i*2],"%02x",p_dst[i]);
				}
				debug_str(DEBUG_LOG_UPDATA, "rbuf:%s\r\n",buf);
            	debug_str(DEBUG_LOG_NET, "MpduHandleMPDU_Diagnosis %d\r\n",recvtype);
				const ST_STA_ATTR_TYPE* pNetBasePara = GetStaBaseAttr();
				uint8_t broadcast_mac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
				uint8_t zero_mac[6] = {0};
            	if(recvtype == RECEIVE_PL_PDU && p_dst[0] == 1 && p_dst_len == sizeof(msg_test_plc_t))
	            {
	            	msg_test_plc_t *msg = (msg_test_plc_t *)p_dst;
	            	if(memcmp(pNetBasePara->Mac,msg->dst_mac,6) == 0)
	            	{
	            		debug_str(DEBUG_LOG_NET, "RECEIVE_PL_PDU mac right\r\n");
						if(memcmp(zero_mac,msg->src_mac,6) != 0)
						{
							if(msg->fre <= 2)
							{
								HPLC_PHY_SetFrequence(msg->fre);
							}
							memcpy(msg->dst_mac,msg->src_mac,6);
							memset(msg->src_mac,0,6);
							broadcast_msg_test_send(msg->sendtype,(uint8_t *)msg,sizeof(msg_test_plc_t));
							debug_str(DEBUG_LOG_NET, "RECEIVE_PL_PDU reply\r\n");
						}
						g_broadcast_msg_test_recv_plc_num++;
	            	}
					else if(memcmp(broadcast_mac,msg->dst_mac,6) == 0)
					{
						debug_str(DEBUG_LOG_NET, "RECEIVE_PL_PDU broadcast\r\n");
						g_broadcast_msg_test_recv_plc_num++;
					}
	            }
				else if(recvtype == RECEIVE_HRF_PL_PDU && p_dst[0] == 2 && p_dst_len == sizeof(msg_test_rf_t))
				{
					msg_test_rf_t *msg = (msg_test_rf_t *)p_dst;
	            	if(memcmp(pNetBasePara->Mac,msg->dst_mac,6) == 0)
	            	{
	            		debug_str(DEBUG_LOG_NET, "RECEIVE_HRF_PL_PDU mac right\r\n");
						if(memcmp(zero_mac,msg->src_mac,6) != 0)
						{
							if(msg->option <= 3 && msg->option >= 1)
							{
								HRF_PHY_SetChannel(msg->channel,msg->option);
							}
							memcpy(msg->dst_mac,msg->src_mac,6);
							memset(msg->src_mac,0,6);
							broadcast_msg_test_send(msg->sendtype,(uint8_t *)msg,sizeof(msg_test_rf_t));
							debug_str(DEBUG_LOG_NET, "RECEIVE_HRF_PL_PDU reply\r\n");
						}
						g_broadcast_msg_test_recv_rf_num++;
	            	}
					else if(memcmp(broadcast_mac,msg->dst_mac,6) == 0)
					{
						debug_str(DEBUG_LOG_NET, "RECEIVE_HRF_PL_PDU broadcast\r\n");
						g_broadcast_msg_test_recv_rf_num++;
					}
				}
            }   
        }
    }
 
    return result;
}


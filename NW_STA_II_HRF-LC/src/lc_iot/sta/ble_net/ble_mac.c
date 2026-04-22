#include "ble_mac.h"
#include "ble_data.h"
#include "ble_local.h"
#include "ble_apl.h"
#include "ble_net.h"
#include "ble_event.h"


#include "lc_all.h"

//====================

#ifdef REGIST_NET_REPORT_USING
//主动注册节点上报
//主动注册后的节点上报；
#define REPORT_METER_REGISTER_STATU         (0x02)//等上报状态
#define CHECK_METER_REGISTER_STATU          (0x10)//检测是否完成状态
//=
#define HPLC_METER_REGIST_REPORT_TIME        (20000)  //30秒
#define HPLC_METER_REGIST_CHECK_TIME         (60000*2)  //60秒
#define HPLC_METER_REGIST_CHECK_END_TIME         (60000*8)  //60秒

#define HPLC_METER_REGIST_START_CHECK_TIME         (60000*8)  //60秒

#define HPLC_METER_REGIST_REPORT_ADDR_SIZE        (16)
#define REPORT_MAC_ADDR_BIT_MAP_U32_LEN        ((BLE_MAX_NEIGHBOR_NUM+31)/8)
//==
static uint32_t ReportMacAddrBitMap[REPORT_MAC_ADDR_BIT_MAP_U32_LEN];
static uint8_t RegisterMeerStatu = REPORT_METER_REGISTER_STATU;
static TimeValue NodeAddrReportTimerTmr;

//获取需要主动上报的节点表地址,注册上报地址
static bool GetHplcNeedReportAndSetMeterList(uint8_t *pmlist,uint16_t *metsum,uint8_t needreadsum)
{
    uint16_t readcnt = 0;

    bool reendflg = false;
    for(int i=0;i<REPORT_MAC_ADDR_BIT_MAP_U32_LEN;i++)
    {
        if(reendflg)break;
        
        u32 wordbitvalue = ReportMacAddrBitMap[i];
        for(int j=0;j<32;j++)
        {
            if(readcnt >= needreadsum)
            {
                reendflg = true;
                break;
            }
            if((wordbitvalue&(0x01<<j)) != 0)
            {//有效置位
                int tei = i*32 + j;
                wordbitvalue &= ~(0x01<<j);//去除
                //
                if( true == ble_get_net_tei_mac_addr(tei,pmlist))
                {
                    //TransposeAddr(pmlist);
                    pmlist += BLE_LONG_ADDR_SIZE;
                    readcnt++;
                }
            }
        }
        ReportMacAddrBitMap[i] = wordbitvalue;
    }
    *metsum = readcnt;
    return readcnt==0?false:true;
}

bool ble_net_check_regist_report_addr()
{
	bool check_flg = false;
    uint16_t readcnt = 0;
    for(int i=0;i<REPORT_MAC_ADDR_BIT_MAP_U32_LEN;i++)
    {
        if(ReportMacAddrBitMap[i] != 0)
		{
			check_flg = true;
			break;
		}
	}
	return check_flg;
}

//
void ble_mac_addr_report_process_task()
{//获取上报节点，并上报数据；
   
	if(LcTimeOut(&NodeAddrReportTimerTmr))
	{
		if(ble_net_check_regist_report_addr())
		{
			uint16_t node_sum = 0;
			uint8_t *send_buff = ble_local_malloc(WHI_PRO_HEAD_SIZE+HPLC_METER_REGIST_REPORT_ADDR_SIZE*BLE_LONG_ADDR_SIZE + 16);
			if(send_buff != NULL)
			{
				if(true == GetHplcNeedReportAndSetMeterList(&send_buff[WHI_PRO_STAT_HEAD_SIZE + 2],&node_sum,HPLC_METER_REGIST_REPORT_ADDR_SIZE))
				{
					uint16_t frame_len = WHI_BuildReportInNetNodeListProcess(send_buff,&send_buff[WHI_PRO_STAT_HEAD_SIZE + 2],node_sum,WHI_CreateLocalSeq());
					//
					if(frame_len >0)
					{
						//ble_net_ack_send_data(send_buff,frame_len);
						lc_net_my_info_t p_info;
						lc_net_get_net_self_info(&p_info);
						if(p_info.net_state == LC_NET_IN_STATE)
						{
							ble_net_debug_cnt_list.ble_to_plc_tx_cnt++;
							lc_net_hplc_tx_sta_frame(p_info.cco_addr,send_buff,frame_len);
						}
					}
					//重置一个定时器
					RegisterMeerStatu = REPORT_METER_REGISTER_STATU;
					if(node_sum >= HPLC_METER_REGIST_REPORT_ADDR_SIZE)
					{
						StartTimer(&NodeAddrReportTimerTmr,HPLC_METER_REGIST_REPORT_TIME);
					}
					else
					{
						StartTimer(&NodeAddrReportTimerTmr,HPLC_METER_REGIST_CHECK_TIME);
					}
				}
				ble_local_free(send_buff);
			}
		}
		else
		{
			//if(RegisterMeerStatu == REPORT_METER_REGISTER_STATU)
			//{//若等待无上报，将完成
				RegisterMeerStatu = CHECK_METER_REGISTER_STATU;
			//}
			StartTimer(&NodeAddrReportTimerTmr,HPLC_METER_REGIST_CHECK_END_TIME);
		}
	}
}

//
void ble_mac_init_addr_report()
{
    memset(ReportMacAddrBitMap,0,sizeof(ReportMacAddrBitMap));
    RegisterMeerStatu = CHECK_METER_REGISTER_STATU;
	StartTimer(&NodeAddrReportTimerTmr,HPLC_METER_REGIST_START_CHECK_TIME);
}

//添加个报主动注册档案
bool ble_mac_report_add_addr_to_report(int tei)
{
    if(tei != -1)
    {
        uint16_t wordidx = tei>>5;
        uint16_t bitidx = tei&0x1F;
        ReportMacAddrBitMap[wordidx] |= 0x01UL<<bitidx;
        if(RegisterMeerStatu == CHECK_METER_REGISTER_STATU)
        {
            RegisterMeerStatu = REPORT_METER_REGISTER_STATU;
			StartTimer(&NodeAddrReportTimerTmr,HPLC_METER_REGIST_REPORT_TIME);
        }
        return true;
    }
    return false;
}
#endif



//===============================
//帧创建
void ble_mac_create_mac_frame_head(ble_mac_frame_type *p_send_mac,uint8_t addr_flg,uint8_t path_flg,uint8_t path_mode_flg)
{
	p_send_mac->ver = MAC_VERSION;
	p_send_mac->addr_flg = addr_flg;
	p_send_mac->path_flg = path_flg;
	p_send_mac->path_mode = path_mode_flg;
	p_send_mac->res_flg = 0;
}

void ble_mac_create_mac_cmd_frame_head(ble_mac_cmd_type *p_send_mac_cmd,uint32_t mac_cmd)
{
	p_send_mac_cmd->cmd = mac_cmd;
}


uint16_t ble_mac_create_cmd_frame(uint8_t *ptr,uint8_t *to_mac,uint8_t *apl_data,uint16_t apl_len,uint8_t mac_cmd)
{
	uint16_t frame_len=0;
	uint16_t dst_tei = 0;
	uint8_t brocast_addr[BLE_LONG_ADDR_SIZE] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	uint8_t local_brocast_addr[BLE_LONG_ADDR_SIZE] = {0xFE,0xFF,0xFF,0xFF,0xFF,0xFF};
	if((0 == memcmp(brocast_addr,to_mac,BLE_LONG_ADDR_SIZE))
		||(0 == memcmp(local_brocast_addr,to_mac,BLE_LONG_ADDR_SIZE)))
	{
		dst_tei = SEND_BROCAST_ADDR_VALUE;
	}
	else
	{
		dst_tei = ble_get_mac_addr_to_net_tei(to_mac);
	}
	
	if(dst_tei > 0)
	{
		uint8_t path_mode = MAC_DST_PATH;
		uint16_t mac_len = apl_len;
		ble_mac_frame_type *p_send_mac = (ble_mac_frame_type *)(&ptr[BLE_LOCAL_LINK_SEND_DATA_OFF]);
		ble_mac_create_mac_frame_head(p_send_mac,MAC_PATH_ADDR_2SIZE,MAC_PATH_ENABLE_FLG,path_mode);
		//
		ble_net_debug_cnt_list.create_path_enable_frame_cnt++;
		//
		ble_mac_path_type *p_send_mac_path = (ble_mac_path_type *)p_send_mac->frame_data;
		uint16_t path_off = sizeof(ble_mac_path_type);
		//
		uint16_t next_tei =0;
		if(dst_tei == SEND_BROCAST_ADDR_VALUE)
		{
			next_tei = SEND_BROCAST_ADDR_VALUE;
			path_mode = MAC_DST_PATH;
		}
		else
		{
			next_tei = ble_get_to_tei_route(dst_tei);//((uint16_t*)(p_send_mac_path->path_list))[p_send_mac_path->path_idx];
		}
		
		if(next_tei != 0)
		{
			uint16_t path_len;
			if(path_mode == MAC_DST_PATH)
			{
				path_len = 2;
				p_send_mac_path->path_list[0] = dst_tei&0xFF;
				p_send_mac_path->path_list[1] = (dst_tei>>8)&0xFF;
				p_send_mac_path->path_list[2] = (BLE_CCO_TEI)&0xFF;
				p_send_mac_path->path_list[3] = (BLE_CCO_TEI>>8)&0xFF;
			}
			else
			{
				path_len = ble_get_to_tei_route_path(dst_tei,(uint16_t*)(p_send_mac_path->path_list));
			}
			if(path_len >= 2)
			{
				p_send_mac_path->path_len = path_len;
				p_send_mac_path->path_idx = path_len-2;
				//
				path_off += path_len*BLE_SHORT_ADDR_SIZE;
				//
				ble_mac_cmd_type *p_send_mac_cmd = (ble_mac_cmd_type *)&(p_send_mac->frame_data[path_off]);
				ble_mac_create_mac_cmd_frame_head(p_send_mac_cmd,mac_cmd);
				//
				if(p_send_mac_cmd->cmd_data != apl_data)
				{
					memmove(p_send_mac_cmd->cmd_data,apl_data,apl_len);
				}
				//
				mac_len += sizeof(ble_mac_frame_type) + path_off + sizeof(ble_mac_cmd_type);
				//
				if(next_tei == SEND_BROCAST_ADDR_VALUE)
				{
					
					frame_len = ble_create_local_link_send_frame(ptr,brocast_addr,BLE_MAX_LAYER_VALUE,mac_len,&ptr[BLE_LOCAL_LINK_SEND_DATA_OFF]);
				}
				else
				{
					uint8_t dst_layer = ble_neighbor_list[next_tei].Layer;//BLE_MAX_LAYER_VALUE;
					if((dst_layer <1)||(dst_layer > BLE_MAX_LAYER_VALUE))
					{
						dst_layer = (BLE_MAX_LAYER_VALUE>>1)+1;
					}
					frame_len = ble_create_local_link_send_frame(ptr,ble_neighbor_list[next_tei].MacAddr,dst_layer,mac_len,&ptr[BLE_LOCAL_LINK_SEND_DATA_OFF]);
				}
			}
		}
	}
	return frame_len;
}

uint16_t ble_mac_create_apl_cmd_frame(uint8_t *ptr,uint8_t *to_mac,uint8_t *apl_data,uint16_t apl_len)
{	
	return ble_mac_create_cmd_frame(ptr,to_mac,apl_data,apl_len,MAC_APL_FRAME_CMD);
}

uint16_t ble_mac_create_apl_cmd_no_link_frame(uint8_t *ptr,uint8_t *apl_data,uint16_t apl_len, uint16_t app_seq_num)
{
	uint16_t mac_len = BLE_MAC_FRAME_HEAD_LEN + BLE_MAC_CMD_HEAD_LEN +  apl_len;

	ble_mac_frame_type *p_send_mac = (ble_mac_frame_type *)(&ptr[BLE_LOCAL_SEND_DATA_OFF]);
	ble_mac_cmd_type *p_send_mac_cmd = (ble_mac_cmd_type *)p_send_mac->frame_data;
	//
	ble_mac_create_mac_frame_head(p_send_mac,MAC_PATH_ADDR_2SIZE,MAC_PATH_DISABLE_FLG,MAC_DST_PATH);
	ble_mac_create_mac_cmd_frame_head(p_send_mac_cmd,MAC_APL_FRAME_CMD);
	//
	if(apl_data != (p_send_mac_cmd->cmd_data))
	{
		memmove(p_send_mac_cmd->cmd_data,apl_data,apl_len);
	}
	//
	//uint16_t frame_len = ble_create_local_send_frame(ptr,SEND_BROCAST_ADDR_TYPE,NULL,mac_len,&ptr[BLE_LOCAL_SEND_DATA_OFF]);
	uint16_t next_tei = app_seq_num;
	
	uint8_t dst_addr[BLE_LONG_ADDR_SIZE];
	memset(dst_addr,0,sizeof(dst_addr));
	
	//memcpy(dst_addr,ble_neighbor_list[next_tei].MacAddr,BLE_LONG_ADDR_SIZE)
	dst_addr[0] = next_tei&0xFF;
	dst_addr[1] = (next_tei>>8)&0xFF;
	//
	uint16_t frame_len = ble_create_local_send_frame(ptr,SEND_APP_SEQ_NUM_TYPE,dst_addr,mac_len,&ptr[BLE_LOCAL_SEND_DATA_OFF]);
	//
	return frame_len;
}


//创建离网指示，连接发送广播
uint16_t ble_mac_create_leave_inc_cmd_frame(uint8_t *ptr,uint16_t *leave_addr_idx)
{
	uint16_t frame_len=0;
	uint16_t mac_len = 0;
	ble_mac_frame_type *p_send_mac = (ble_mac_frame_type *)(&ptr[BLE_LOCAL_LINK_SEND_DATA_OFF]);
	ble_mac_create_mac_frame_head(p_send_mac,MAC_PATH_ADDR_2SIZE,MAC_PATH_DISABLE_FLG,MAC_DST_PATH);
	//
	uint16_t path_off = 0;
	ble_mac_cmd_type *p_send_mac_cmd = (ble_mac_cmd_type *)&(p_send_mac->frame_data[path_off]);
	ble_mac_create_mac_cmd_frame_head(p_send_mac_cmd,MAC_LEAVE_IND_CMD);
	//
	ble_mac_leave_inc_cmd_type *p_send_mac_leave_inc_cmd = (ble_mac_leave_inc_cmd_type *)(p_send_mac_cmd->cmd_data);
	p_send_mac_leave_inc_cmd->dealy_time = 10;
	p_send_mac_leave_inc_cmd->tei_sum = ble_get_leave_net_tei_process(p_send_mac_leave_inc_cmd->tei_list,1,leave_addr_idx);
	//
	if(p_send_mac_leave_inc_cmd->tei_sum > 0)
	{
		//uint16_t dst_tei = ble_get_mac_addr_to_net_tei(p_send_mac_leave_inc_cmd->tei_list);
		uint16_t dst_tei = p_send_mac_leave_inc_cmd->tei_list[1];
		dst_tei = (dst_tei<<8)|p_send_mac_leave_inc_cmd->tei_list[0];
		
		if(dst_tei > 0)
		{
			uint16_t next_tei = ble_get_to_tei_route(dst_tei);
			if(next_tei > 0)
			{
				mac_len = sizeof(ble_mac_frame_type) + sizeof(ble_mac_cmd_type) + sizeof(ble_mac_leave_inc_cmd_type) + p_send_mac_leave_inc_cmd->tei_sum*BLE_SHORT_ADDR_SIZE;
				//
				uint8_t dst_addr[BLE_LONG_ADDR_SIZE];
				//memcpy(dst_addr,ble_neighbor_list[next_tei].MacAddr,BLE_LONG_ADDR_SIZE)
				memset(dst_addr,0,sizeof(dst_addr));
				dst_addr[0] = next_tei&0xFF;
				dst_addr[1] = (next_tei>>8)&0xFF;
				//
				frame_len = ble_create_local_send_frame(ptr,SEND_SHORT_ADDR_TYPE,dst_addr,mac_len,&ptr[BLE_LOCAL_LINK_SEND_DATA_OFF]);
			}
		}
	}
	return p_send_mac_leave_inc_cmd->tei_sum>0?frame_len:0;
}

uint32_t ble_mac_report_ack_count;

uint16_t ble_mac_create_report_ack_cmd_frame(uint16_t dst_tei, uint8_t *report_ack, uint32_t report_ack_len)
{
    uint16_t frame_len = 0;
    uint16_t mac_len = BLE_MAC_FRAME_HEAD_LEN + BLE_MAC_CMD_HEAD_LEN + sizeof(ble_mac_event_report_cmd_type) + report_ack_len;
    uint8_t *ptr = ble_local_malloc(BLE_LOCAL_SEND_FRAME_HEAD_LEN + mac_len + 16);
    if(ptr != NULL)
    {
        ble_mac_frame_type *p_send_mac = (ble_mac_frame_type *)(&ptr[BLE_LOCAL_SEND_DATA_OFF]);
        ble_mac_cmd_type *p_send_mac_cmd = (ble_mac_cmd_type *)p_send_mac->frame_data;
        ble_mac_event_report_cmd_type *p_send_mac_report_ack_cmd = (ble_mac_event_report_cmd_type *)p_send_mac_cmd->cmd_data;

        ble_mac_create_mac_frame_head(p_send_mac,MAC_PATH_ADDR_2SIZE,MAC_PATH_DISABLE_FLG,MAC_DST_PATH);
        ble_mac_create_mac_cmd_frame_head(p_send_mac_cmd,MAC_REPORT_ACK_CMD);
        
        p_send_mac_report_ack_cmd->src_tei = dst_tei;
        memcpy(p_send_mac_report_ack_cmd->data, report_ack, report_ack_len);

        uint16_t next_tei = ble_get_to_tei_route(dst_tei);
        
        uint8_t dst_addr[BLE_LONG_ADDR_SIZE];
        memset(dst_addr,0,sizeof(dst_addr));

        dst_addr[0] = next_tei&0xFF;
        dst_addr[1] = (next_tei>>8)&0xFF;
        
        frame_len = ble_create_local_send_frame(ptr,SEND_SHORT_ADDR_TYPE,dst_addr,mac_len,&ptr[BLE_LOCAL_SEND_DATA_OFF]);
        ble_local_send_data(ptr,frame_len);

        ble_mac_report_ack_count++;
        ble_local_free(ptr);
    }

	return frame_len;
}


//创建时间同步，无连接发送PCO广播
uint16_t ble_mac_create_sync_time_stamp_cmd_frame(uint8_t *ptr,uint32_t time_stamp)
{
	uint16_t frame_len=0;
	uint16_t mac_len = 0;
	ble_mac_frame_type *p_send_mac = (ble_mac_frame_type *)(&ptr[BLE_LOCAL_LINK_SEND_DATA_OFF]);
	ble_mac_create_mac_frame_head(p_send_mac,MAC_PATH_ADDR_2SIZE,MAC_PATH_DISABLE_FLG,MAC_DST_PATH);
	//
	uint16_t path_off = 0;
	ble_mac_cmd_type *p_send_mac_cmd = (ble_mac_cmd_type *)&(p_send_mac->frame_data[path_off]);
	ble_mac_create_mac_cmd_frame_head(p_send_mac_cmd,MAC_SYNC_TIME_STAMP_CMD);
	//
	ble_mac_sync_time_stamp_cmd_type *p_send_mac_sync_time_stamp_cmd = (ble_mac_sync_time_stamp_cmd_type *)(p_send_mac_cmd->cmd_data);
	p_send_mac_sync_time_stamp_cmd->time_stamp = time_stamp;

	uint16_t next_tei = SEND_BROCAST_ADDR_VALUE;
	mac_len = sizeof(ble_mac_frame_type) + sizeof(ble_mac_cmd_type) + sizeof(ble_mac_sync_time_stamp_cmd_type);
	//
	uint8_t dst_addr[BLE_LONG_ADDR_SIZE];
	memset(dst_addr,0,sizeof(dst_addr));
	dst_addr[0] = next_tei&0xFF;
	dst_addr[1] = (next_tei>>8)&0xFF;
	//
	frame_len = ble_create_local_send_frame(ptr,SEND_SHORT_ADDR_TYPE,dst_addr,mac_len,&ptr[BLE_LOCAL_LINK_SEND_DATA_OFF]);

	return frame_len;
}

//ble_mac_create_sync_time_stamp_cmd_frame
uint16_t ble_mac_create_cfg_isp_proj_key_cmd_frame(uint8_t *ptr,uint8_t* to_mac,uint32_t isp_key, uint32_t proj_key)
{
	ble_mac_cfg_isp_proj_key_cmd_type *p_cfg_frame = (ble_mac_cfg_isp_proj_key_cmd_type *)(&ptr[BLE_MAC_SEND_APL_LINK_HEAD_OFF]);
	p_cfg_frame->dir = 1;//1下行，0上行
	p_cfg_frame->isp_key = isp_key;
	p_cfg_frame->proj_key = proj_key;
	return ble_mac_create_cmd_frame(ptr,to_mac,&ptr[BLE_MAC_SEND_APL_LINK_HEAD_OFF],sizeof(ble_mac_cfg_isp_proj_key_cmd_type),MAC_CFG_ISP_PROJ_KEY_CMD);
}

uint8_t ble_mac_send_cfg_isp_proj_key_cmd_frame(uint8_t* to_mac,uint32_t isp_key, uint32_t proj_key)
{
	uint16_t apl_len = BLE_MAC_SEND_FRAME_HEAD_LEN + sizeof(ble_mac_cfg_isp_proj_key_cmd_type);
	uint8_t *ptr = ble_local_malloc(BLE_LOCAL_SEND_FRAME_HEAD_LEN + apl_len + 16);
	if(ptr != NULL)
	{
		uint16_t frame_len = ble_mac_create_cfg_isp_proj_key_cmd_frame(ptr,to_mac,isp_key,proj_key);
		if(true == ble_local_send_data(ptr,frame_len))
		{
			ble_net_debug_cnt_list.ble_net_key_send_cnt++;
		}
		ble_local_free(ptr);
	}
	return 0;
}
extern uint8_t lc_net_get_key_switch_code();

//============
//帧处理
uint16_t ble_mac_process_assoc_cmd_frame(ble_mac_frame_type *p_mac,ble_mac_path_type *p_mac_path,ble_mac_cmd_type *p_mac_cmd,uint16_t frame_len)
{
//	if((p_mac != NULL)&&(p_mac_cmd != NULL))
	{
		uint16_t cmd_frame_len = (((uint32_t)p_mac_cmd) - ((uint32_t)p_mac));
//		if(cmd_frame_len >= frame_len)
//		{
//			cmd_frame_len = 0;
//		}
//		else
		{
			cmd_frame_len = frame_len - cmd_frame_len;
		}
		//
		
		ble_net_debug_cnt_list.ble_net_assoc_req_cnt++;//关联请求
		
		if(cmd_frame_len >= sizeof(ble_mac_assoc_cmd_type))
		{
			ble_mac_assoc_cmd_type *p_assoc_frame = (ble_mac_assoc_cmd_type *)p_mac_cmd->cmd_data;
			uint8_t revalue;
			uint16_t dev_tei = 0;
			bool check_isp_proj_key_flg = false;
			uint8_t check_proj_key_flg = 0;
			
			ble_brush_neighbor_heart(p_assoc_frame->pco_tei,0xFF);
			//
			uint32_t isp_key = 0;
			uint32_t proj_key = 0;
            memcpy(&isp_key, p_assoc_frame->isp_key, 3);
            memcpy(&proj_key, p_assoc_frame->proj_key, 3);
			//
			if(lc_net_get_key_switch_code() == 1)
			{
				check_isp_proj_key_flg = true;
				check_proj_key_flg = 1;
			}
			else
			{
				if(ble_net_info.ProjKey != 0)
				{
					if(ble_net_info.NetBindFlg == 1)
					{//绑定组网
						if(isp_key == ble_net_info.IspKey)
						{
							if(proj_key == ble_net_info.ProjKey)
							{
								check_isp_proj_key_flg = true;
								check_proj_key_flg = 1;
							}
							else if(proj_key == 0)
							{
								check_isp_proj_key_flg = true;
								check_proj_key_flg = 0;
							}
							else
							{	
								//入网上报事件					
								lamp_event_add(EVENT_REJECT_IN_NET_REPORT, p_assoc_frame->mac_addr, proj_key);
							}
						}
					}
					else
					{//正常组网
						if((proj_key == ble_net_info.ProjKey)&&(isp_key == ble_net_info.IspKey))
						{
							check_isp_proj_key_flg = true;
							check_proj_key_flg = 1;
						}
						else
						{	
							//入网上报事件					
							lamp_event_add(EVENT_REJECT_IN_NET_REPORT, p_assoc_frame->mac_addr, proj_key);
						}
					}
					//ALL IN
					//check_isp_proj_key_flg = true;
					//check_proj_key_flg = 1;
				}
			}
			
			if(check_isp_proj_key_flg == true)
			{
				ble_net_debug_cnt_list.ble_net_into_check_key_fail_cnt++;
				
				dev_tei = ble_malloc_tei_from_mac_addr(p_assoc_frame->mac_addr);
				if(dev_tei == 0)
				{
					if(-1 == ble_check_mac_addr_from_list(p_assoc_frame->mac_addr))
					{
						int macaddridx = ble_insert_mac_addr_to_list(p_assoc_frame->mac_addr,0,BLE_MAC_ADDR_FINDED_ATTR);
						if(-1 != macaddridx)
						{
							dev_tei = ble_malloc_tei_from_mac_addr(p_assoc_frame->mac_addr);
							if(dev_tei != 0)
							{
								#ifdef REGIST_NET_REPORT_USING
								ble_mac_report_add_addr_to_report(dev_tei);
								#endif
								ble_neighbor_list[dev_tei].AssocReqCnt = 0;
								ble_neighbor_list[dev_tei].FromCcoAplCnt = 0;
								ble_neighbor_list[dev_tei].ToCcoAplCnt = 0;
								ble_neighbor_list[dev_tei].StayInNetPrioCnt = 0;
								
							}
						}
					}
				}
				
				
				if(dev_tei != 0)
				{//分配成功
					ble_neighbor_list[dev_tei].AssocReqCnt += 1;
					//
					#ifdef BLE_NODE_IN_LEAVE_NET_REPORT_USING
					//uint16_t old_pco_tei = ble_neighbor_list[dev_tei].NeighborPcoTEI;
					#endif
					//
					revalue = ble_into_pco_tei(0,dev_tei,p_assoc_frame->pco_tei,SINGLE_METER,0x01);//加入pco,tei为sta或是pco;新加入或变更代理
					if(INTO_NET_REQ_SUCCESS == revalue)
					{//入网成功
						OS_ERR err;
						ble_neighbor_list[dev_tei].brush_neighbor_tick = OSTimeGet(&err);
						ble_neighbor_list[dev_tei].NetStatePeriodCount = 0;
						ble_neighbor_list[dev_tei].pro_check_flg = check_proj_key_flg;
						ble_neighbor_list[dev_tei].BindProjKeyCnt = 0;
						ble_neighbor_list[dev_tei].CurrentRxBeaconCnt = 0;
						ble_neighbor_list[dev_tei].HeartFrameHeartCnt = 0;	
						ble_neighbor_list[dev_tei].NeighborHeartCnt = 0;		
						ble_neighbor_list[dev_tei].OtherHeartCnt = 0;
						ble_brush_neighbor_heart(dev_tei,0xFF);
						//
						#ifdef BLE_NODE_IN_LEAVE_NET_REPORT_USING

						#ifdef BLE_NODE_IN_NET_REPORT_ENABLE
						// if(old_pco_tei == 0)
						{//需要上报
							if(ble_neighbor_list[dev_tei].NeighborPcoTEI != 0)
							{//入网
								ble_neighbor_list[dev_tei].NeedNetReport = 1;
								ble_neighbor_list[dev_tei].NetReportFlg = 0;//初始离网已上报
							}
						}
						#endif
						
						#endif
					}
				}
				else
				{
					revalue = INTO_NET_NOT_IN_LIST_FAIL;
				}
			}
			else
			{
				revalue = INTO_NET_NOT_IN_LIST_FAIL;
			}
			//
			if(INTO_NET_REQ_SUCCESS == revalue)
			{
				ble_net_debug_cnt_list.ble_net_into_net_success_cnt++;
			}
			else
			{
				ble_net_debug_cnt_list.ble_net_into_fail_cnt++;
			}
			//
			if(p_mac_path == NULL)
			{//不带路径
				uint16_t mac_len = BLE_MAC_FRAME_HEAD_LEN + BLE_MAC_CMD_HEAD_LEN +  sizeof(ble_mac_assoc_ack_cmd_type);
				uint8_t *ptr = ble_local_malloc(BLE_LOCAL_SEND_FRAME_HEAD_LEN + mac_len + 16);
				if(ptr != NULL)
				{
					ble_mac_frame_type *p_send_mac = (ble_mac_frame_type *)(&ptr[BLE_LOCAL_SEND_DATA_OFF]);
					ble_mac_cmd_type *p_send_mac_cmd = (ble_mac_cmd_type *)p_send_mac->frame_data;
					ble_mac_assoc_ack_cmd_type *p_send_mac_assoc_ack_cmd = (ble_mac_assoc_ack_cmd_type *)p_send_mac_cmd->cmd_data;
					//
					ble_mac_create_mac_frame_head(p_send_mac,MAC_PATH_ADDR_2SIZE,MAC_PATH_DISABLE_FLG,MAC_DST_PATH);
					ble_mac_create_mac_cmd_frame_head(p_send_mac_cmd,MAC_ASSOC_REQ_ACK_CMD);
					//
					//p_send_mac_cmd->cmd = MAC_ASSOC_REQ_ACK_CMD;
					//
					p_send_mac_assoc_ack_cmd->status = revalue;
					memcpy(p_send_mac_assoc_ack_cmd->mac_addr,p_assoc_frame->mac_addr,BLE_LONG_ADDR_SIZE);
					p_send_mac_assoc_ack_cmd->pco_tei = p_assoc_frame->pco_tei;
					p_send_mac_assoc_ack_cmd->dev_tei = dev_tei;
					p_send_mac_assoc_ack_cmd->port = p_assoc_frame->port;

                    if(ble_net_info.NetBindFlg == 1)
                    {
                        isp_key = ble_net_info.IspKey;
                        proj_key = ble_net_info.ProjKey;
                    }
                    else
                    {
                        isp_key = 0;
                        proj_key = 0;
                    }
                    memcpy(p_send_mac_assoc_ack_cmd->isp_key, &isp_key, 3);
                    memcpy(p_send_mac_assoc_ack_cmd->proj_key, &proj_key, 3);
					//
					//uint16_t frame_len = ble_create_local_send_frame(ptr,SEND_BROCAST_ADDR_TYPE,NULL,mac_len,&ptr[BLE_LOCAL_SEND_DATA_OFF]);
					uint16_t next_tei = SEND_LOCAL_BROCAST_ADDR_VALUE;
					
					uint8_t dst_addr[BLE_LONG_ADDR_SIZE];
					memset(dst_addr,0,sizeof(dst_addr));
					
					if(p_assoc_frame->pco_tei != BLE_CCO_TEI)
					{
						next_tei = ble_get_to_tei_route(p_assoc_frame->pco_tei);
					}

					//memcpy(dst_addr,ble_neighbor_list[next_tei].MacAddr,BLE_LONG_ADDR_SIZE)
					dst_addr[0] = next_tei&0xFF;
					dst_addr[1] = (next_tei>>8)&0xFF;
					
					uint16_t frame_len = ble_create_local_send_frame(ptr,SEND_SHORT_ADDR_TYPE,dst_addr,mac_len,&ptr[BLE_LOCAL_SEND_DATA_OFF]);
					//
					ble_local_send_data(ptr,frame_len);
					//
					ble_local_free(ptr);
				}
			}
			else
			{//带路径
				
			}
		}
	}
	return 0;
}

uint32_t ble_mac_heart_ack_count;

uint16_t ble_mac_process_heart_report_down_cmd_frame(ble_mac_heart_report_cmd_type *down)
{
    uint16_t frame_len = 0;
    uint16_t mac_len = BLE_MAC_FRAME_HEAD_LEN + BLE_MAC_CMD_HEAD_LEN + sizeof(ble_mac_heart_report_cmd_type);
    uint8_t *ptr = ble_local_malloc(BLE_LOCAL_SEND_FRAME_HEAD_LEN + mac_len + 16);
    if(ptr != NULL)
    {
        ble_mac_frame_type *p_send_mac = (ble_mac_frame_type *)(&ptr[BLE_LOCAL_SEND_DATA_OFF]);
        ble_mac_cmd_type *p_send_mac_cmd = (ble_mac_cmd_type *)p_send_mac->frame_data;
        ble_mac_create_mac_frame_head(p_send_mac,MAC_PATH_ADDR_2SIZE,MAC_PATH_DISABLE_FLG,MAC_DST_PATH);
        ble_mac_create_mac_cmd_frame_head(p_send_mac_cmd,MAC_HEARTBEAT_DOWN_CMD);
        
        memcpy(p_send_mac_cmd->cmd_data, down, sizeof(ble_mac_heart_report_cmd_type));

        uint16_t next_tei = ble_get_to_tei_route(down->src_tei);
        
        uint8_t dst_addr[BLE_LONG_ADDR_SIZE];
        memset(dst_addr,0,sizeof(dst_addr));

        dst_addr[0] = next_tei&0xFF;
        dst_addr[1] = (next_tei>>8)&0xFF;
        
        frame_len = ble_create_local_send_frame(ptr,SEND_SHORT_ADDR_TYPE,dst_addr,mac_len,&ptr[BLE_LOCAL_SEND_DATA_OFF]);
        ble_local_send_data(ptr,frame_len);

        ble_mac_heart_ack_count++;
        ble_local_free(ptr);
    }

	return frame_len;
}

uint32_t ble_mac_heart_recv_count;

uint16_t ble_mac_process_heart_report_cmd_frame(ble_mac_frame_type *p_mac,ble_mac_path_type *p_mac_path,ble_mac_cmd_type *p_mac_cmd,uint16_t frame_len)
{
    uint16_t cmd_frame_len = frame_len - (((uint32_t)p_mac_cmd) - ((uint32_t)p_mac));

    cmd_frame_len -= sizeof(ble_mac_cmd_type);

    if(cmd_frame_len >= sizeof(ble_mac_heart_report_cmd_type))
    {
        ble_mac_heart_report_cmd_type *up = (void *)p_mac_cmd->cmd_data;
        if (up->src_tei > BLE_CCO_TEI && up->src_tei < BLE_MAX_NEIGHBOR_NUM)
        {
            ble_mac_heart_recv_count++;
            if((ble_neighbor_list[up->src_tei].NeightState == NEIGHT_HAVE_FLG_VALUE) && (ble_neighbor_list[up->src_tei].NeighborPcoTEI != 0x00))//有效的,有父节点
            {
            	if(0 == memcmp(up->mac, ble_neighbor_list[up->src_tei].MacAddr, LC_NET_ADDR_SIZE))
            	{
                    ble_brush_neighbor_heart(up->src_tei, 0);
                    ble_mac_process_heart_report_down_cmd_frame(up);
                }
            }
        }
    }
	return 0;
}

uint32_t ble_mac_report_count;
extern uint8_t EnterNetReportStatus;
/**
 * @brief D7灯上报处理接口
 * 
 * @param p_mac 
 * @param p_mac_path 
 * @param p_mac_cmd 
 * @param frame_len 
 * @return uint16_t 
 */
uint16_t ble_mac_process_event_report_cmd_frame(ble_mac_frame_type *p_mac,ble_mac_path_type *p_mac_path,ble_mac_cmd_type *p_mac_cmd,uint16_t frame_len)
{
    uint16_t cmd_frame_len = frame_len - (((uint32_t)p_mac_cmd) - ((uint32_t)p_mac));

    cmd_frame_len -= sizeof(ble_mac_cmd_type);

    if(cmd_frame_len > sizeof(ble_mac_event_report_cmd_type))
    {
		
        ble_mac_event_report_cmd_type *report = (void *)p_mac_cmd->cmd_data;
        if (report->src_tei > BLE_CCO_TEI && report->src_tei < BLE_MAX_NEIGHBOR_NUM)
        {		
			if(cmd_frame_len - sizeof(ble_mac_event_report_cmd_type) >=8)
			{
				uint16_t Code = report->data[6] + (report->data[7]<<8);
				if(Code == EVENT_IN_NET_REPORT && EnterNetReportStatus != 2)
				{
					return 0;
				}
			}
			
            uint8_t* send_buff = malloc(WHI_PRO_HEAD_SIZE + WHI_SYSCTRL_HEAD_START_SIZE + cmd_frame_len);
            if(send_buff != NULL)
            {
                whi_sysctrl_type *sysctrl_head = (whi_sysctrl_type *)(&(send_buff[WHI_PRO_STAT_HEAD_SIZE]));

                memcpy(sysctrl_head->mac_add, ble_neighbor_list[report->src_tei].MacAddr, LC_NET_ADDR_SIZE);
                sysctrl_head->user_len = USER_DATA_HEAD_LEN + cmd_frame_len - sizeof(ble_mac_event_report_cmd_type);
                sysctrl_head->plc_pro = 0x0001;
                sysctrl_head->seq_num = 0;
                sysctrl_head->fun_code = WHI_SYSCTRL_REPORT_DEV_EVENT;
                sysctrl_head->dev_addr = 0;
                sysctrl_head->stat_code = WHI_SYSCTRL_SUCCESS_CODE;
                memcpy(sysctrl_head->data_body, report->data, cmd_frame_len - sizeof(ble_mac_event_report_cmd_type));

                uint16_t frame_len = WHI_BuildFrameProcess(send_buff, false, WHI_SEND_BUS_FRAME, WHI_CreateLocalSeq(), (uint8_t *)sysctrl_head, WHI_SYSCTRL_HEAD_START_SIZE + cmd_frame_len - sizeof(ble_mac_event_report_cmd_type));
                ble_mac_report_count++;
                // 蓝牙网络的上报数据默认通过PLC网络发送出去
				lc_net_my_info_t p_info;
				lc_net_get_net_self_info(&p_info);
				if(p_info.net_state == LC_NET_IN_STATE)
				{
					ble_net_debug_cnt_list.ble_to_plc_tx_cnt++;
					lc_net_hplc_tx_sta_frame(p_info.cco_addr,send_buff,frame_len);
				}
                free(send_buff);
            }
        }
    }
	return 0;
}

uint16_t ble_mac_process_cfg_isp_proj_key_frame(ble_mac_frame_type *p_mac,ble_mac_path_type *p_mac_path,ble_mac_cmd_type *p_mac_cmd,uint16_t frame_len)
{	
	if(p_mac_path != NULL)
	{
		uint16_t from_tei_idx = (p_mac_path->path_len - 1)*2;
		uint16_t from_tei = p_mac_path->path_list[from_tei_idx+1];
		from_tei = (from_tei<<8)|p_mac_path->path_list[from_tei_idx];
		//
		if((from_tei>BLE_MIN_TEI_VALUE)&&(from_tei <= BLE_MAX_TEI_VALUE))
		{
			if(frame_len >= (sizeof(ble_mac_cfg_isp_proj_key_cmd_type) + 1))
			{
				ble_mac_cfg_isp_proj_key_cmd_type *p_cfg_frame = (ble_mac_cfg_isp_proj_key_cmd_type *)(p_mac_cmd->cmd_data);
				//
				if(p_cfg_frame->dir == 0)//上行帧
				{
					if((ble_net_info.IspKey == p_cfg_frame->isp_key)
						&&(ble_net_info.ProjKey == p_cfg_frame->proj_key))
					{
						ble_net_debug_cnt_list.ble_net_key_ack_cnt++;
						ble_neighbor_list[from_tei].pro_check_flg = 1;
					}
				}
			}
		}
	}
	
	
	return 0;
}
//============================================
uint16_t ble_mac_process_frame(uint8_t *mac_frame,uint16_t frame_len)
{
	ble_mac_frame_type *p_mac = (ble_mac_frame_type *)mac_frame;
	ble_mac_path_type *p_mac_path = NULL;
	ble_mac_cmd_type *p_mac_cmd = NULL;
	uint16_t frame_min_len = BLE_MAC_FRAME_HEAD_LEN + BLE_MAC_CMD_HEAD_LEN;
	
	if((p_mac->ver == MAC_VERSION)&&(frame_len >= frame_min_len))
	{
		frame_min_len += BLE_MAC_PATH_HEAD_LEN;
		if((p_mac->path_flg == MAC_PATH_ENABLE_FLG)&&(frame_len >= (frame_min_len)))
		{//带路径
			ble_net_debug_cnt_list.receive_path_enable_frame_cnt++;
			//
			uint16_t addr_size = p_mac->addr_flg == MAC_PATH_ADDR_2SIZE?2:6;
			p_mac_path = (ble_mac_path_type *)(p_mac->frame_data);
			if(p_mac_path->path_len >= 2)
			{
				frame_min_len += addr_size*p_mac_path->path_len;
				if((frame_len >= frame_min_len)&&(p_mac_path->path_idx == 0)&&(p_mac_path->path_idx < (p_mac_path->path_len - 1)))
				{
					if(p_mac->addr_flg == MAC_PATH_ADDR_2SIZE)
					{
						uint16_t dst_tei = p_mac_path->path_list[1];
						dst_tei = (dst_tei<<8)|p_mac_path->path_list[0];
						if(dst_tei == BLE_CCO_TEI)
						{
							p_mac_cmd = (ble_mac_cmd_type*)(&(p_mac_path->path_list[p_mac_path->path_len*addr_size]));
						}
					}
					else
					{
						if(0 == memcmp(p_mac_path->path_list,ble_system_params.SystemLongAdd,BLE_LONG_ADDR_SIZE))
						{
							p_mac_cmd = (ble_mac_cmd_type*)(&(p_mac_path->path_list[p_mac_path->path_len*addr_size]));
						}
					}
				}
			}
		}
		else
		{//无路径
			p_mac_cmd = (ble_mac_cmd_type*)(p_mac->frame_data);
		}
	}
	
	if(p_mac_cmd != NULL)
	{
		switch(p_mac_cmd->cmd)
		{
			case MAC_ASSOC_REQ_CMD:
				ble_mac_process_assoc_cmd_frame(p_mac,p_mac_path,p_mac_cmd,frame_len);
				break;
			case MAC_ASSOC_REQ_ACK_CMD:
				break;
			case MAC_LEAVE_IND_CMD:
				break;
			case MAC_LEAVE_IND_ACK_CMD:
				break;
			case MAC_HEART_REPORT_CMD:
                break;
            case MAC_HEARTBEAT_DOWN_CMD:
                break;    
            case MAC_HEARTBEAT_UP_CMD:
				ble_mac_process_heart_report_cmd_frame(p_mac,p_mac_path,p_mac_cmd,frame_len);
				break;
			case MAC_REPORT_CMD:
				ble_mac_process_event_report_cmd_frame(p_mac,p_mac_path,p_mac_cmd,frame_len);
				break;
			case MAC_SYNC_TIME_STAMP_CMD:
				break;
			case MAC_CFG_ISP_PROJ_KEY_CMD:
				ble_mac_process_cfg_isp_proj_key_frame(p_mac,p_mac_path,p_mac_cmd,frame_len);
				break;
			case MAC_APL_FRAME_CMD:
				ble_apl_process_frame(p_mac,p_mac_path,p_mac_cmd,frame_len);
				break;
			default:
				break;
		}
	}
	return 0;
}


//===============================
#ifdef BLE_NODE_IN_LEAVE_NET_REPORT_USING

#define BLE_MAC_IN_LEAVE_NET_REPORT_SOLT_MS	(66*1000UL)
#define BLE_MAC_IN_LEAVE_NET_REPORT_STEP_MS	(4*1000UL)

#define REPORT_ADDR_MAX_COUNT		(14)

#define REPORT_CHECK_ADDR_IN_NET_STATUS			(0)		//检测上报
#define REPORT_CHECK_ADDR_LEAVE_NET_STATUS		(1)		//检测上报
#define REPORT_CHECK_ADDR_IN_NET_WAIT_ACK		(2)		//等待应答
#define REPORT_CHECK_ADDR_LEAVE_NET_WAIT_ACK	(3)		//等待应答

typedef struct
{
	TimeValue report_timer;
	//
	uint8_t report_status;//上报状态
	uint8_t report_net_status;//入网或是离网上报
	uint16_t report_seq;//上报系号
	
	uint8_t addr_count;
	uint8_t report_addr_list[REPORT_ADDR_MAX_COUNT*BLE_LONG_ADDR_SIZE];
}ble_mac_addr_in_leave_net_report_param_type;
ble_mac_addr_in_leave_net_report_param_type ble_mac_addr_in_leave_net_report_param;
//=================================
uint16_t get_report_check_in_net_addr(uint8_t *addr_list)
{
	uint16_t addr_count = 0;
    for(uint16_t a_idx=0;a_idx<ble_system_params.MacAddrSum;a_idx++)
    {
        uint16_t n_idx = ble_system_params.MacAddrList[a_idx].NeightIndex;
		if(n_idx != BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE)
		{
			if((ble_neighbor_list[n_idx].NeighborPcoTEI != 0)
				&&(ble_neighbor_list[n_idx].NeedNetReport == 1)
				&&(ble_neighbor_list[n_idx].NetReportFlg == 0))
			{//需要上报
				memcpy(&addr_list[addr_count*BLE_LONG_ADDR_SIZE],ble_system_params.MacAddrList[a_idx].MacAddr,BLE_LONG_ADDR_SIZE);
				addr_count++;
				if(addr_count>=REPORT_ADDR_MAX_COUNT)break;
			}
		}
    }
	return addr_count;
}

uint16_t get_report_check_leave_net_addr(uint8_t *addr_list)
{
	uint16_t addr_count = 0;
    for(uint16_t a_idx=0;a_idx<ble_system_params.MacAddrSum;a_idx++)
    {
        uint16_t n_idx = ble_system_params.MacAddrList[a_idx].NeightIndex;
		if(n_idx != BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE)
		{
			if((ble_neighbor_list[n_idx].NeighborPcoTEI == 0)
				&&(ble_neighbor_list[n_idx].NeedNetReport == 1)
				&&(ble_neighbor_list[n_idx].NetReportFlg == 0))
			{//不在网，需要上报
				memcpy(&addr_list[addr_count*BLE_LONG_ADDR_SIZE],ble_system_params.MacAddrList[a_idx].MacAddr,BLE_LONG_ADDR_SIZE);
				addr_count++;
				if(addr_count>=REPORT_ADDR_MAX_COUNT)break;
			}
		}
    }
	return addr_count;
}
//in_or_leave 0:leave 1:in net
bool ble_mac_addr_in_leave_net_ack_cb_fun(uint16_t fr_seq,uint16_t in_or_leave)
{
	if(fr_seq == ble_mac_addr_in_leave_net_report_param.report_seq)
	{
		if((in_or_leave == 0)
			&&(ble_mac_addr_in_leave_net_report_param.report_net_status == REPORT_CHECK_ADDR_LEAVE_NET_STATUS)
		&&(ble_mac_addr_in_leave_net_report_param.report_status == REPORT_CHECK_ADDR_LEAVE_NET_WAIT_ACK))
		{
			for(int i=0;i<ble_mac_addr_in_leave_net_report_param.addr_count;i++)
			{
				uint16_t ngh_idx = ble_get_mac_addr_to_net_tei(&(ble_mac_addr_in_leave_net_report_param.report_addr_list[i*BLE_LONG_ADDR_SIZE]));
				if(ngh_idx != 0)
				{
					if(ble_neighbor_list[ngh_idx].NeighborPcoTEI == 0)
					{//离网
						ble_neighbor_list[ngh_idx].NeedNetReport = 0;
						ble_neighbor_list[ngh_idx].NetReportFlg = 1;
						//
						ble_free_tei_and_mac_addr(NULL,ngh_idx);
						ble_delete_mac_addr_from_list(&(ble_mac_addr_in_leave_net_report_param.report_addr_list[i*BLE_LONG_ADDR_SIZE]),true);
					}
				}
				else
				{
					ble_delete_mac_addr_from_list(&(ble_mac_addr_in_leave_net_report_param.report_addr_list[i*BLE_LONG_ADDR_SIZE]),true);
				}
			}
		}else if((in_or_leave == 1)
			&&(ble_mac_addr_in_leave_net_report_param.report_net_status == REPORT_CHECK_ADDR_IN_NET_STATUS)
		&&(ble_mac_addr_in_leave_net_report_param.report_status == REPORT_CHECK_ADDR_IN_NET_WAIT_ACK))
		{
			for(int i=0;i<ble_mac_addr_in_leave_net_report_param.addr_count;i++)
			{
				uint16_t ngh_idx = ble_get_mac_addr_to_net_tei(&(ble_mac_addr_in_leave_net_report_param.report_addr_list[i*BLE_LONG_ADDR_SIZE]));
				if(ngh_idx != 0)
				{
					if(ble_neighbor_list[ngh_idx].NeighborPcoTEI != 0)
					{//在网
						ble_neighbor_list[ngh_idx].NeedNetReport = 0;
						ble_neighbor_list[ngh_idx].NetReportFlg = 1;
					}
				}
			}
		}
	}
	return true;
}

bool ble_mac_addr_in_net_report_send(uint16_t node_sum,uint8_t *addr_list,uint16_t fr_seq)
{
	bool r_flg = false;
	uint8_t *send_buff = ble_local_malloc(WHI_PRO_HEAD_SIZE+node_sum*BLE_LONG_ADDR_SIZE + 16);
	if(send_buff != NULL)
	{
		uint16_t frame_len = WHI_BuildReportInNetNodeListProcess(send_buff,addr_list,node_sum,fr_seq);
		//
		if(frame_len >0)
		{
			//ble_net_ack_send_data(send_buff,frame_len);
			lc_net_my_info_t p_info;
			lc_net_get_net_self_info(&p_info);
			if(p_info.net_state == LC_NET_IN_STATE)
			{
				ble_net_debug_cnt_list.ble_to_plc_tx_cnt++;
				lc_net_hplc_tx_sta_frame(p_info.cco_addr,send_buff,frame_len);
			}
		
			r_flg = true;
		}
		ble_local_free(send_buff);
	}
	return r_flg;
}
bool ble_mac_addr_leave_net_report_send(uint16_t node_sum,uint8_t *addr_list,uint16_t fr_seq)
{
	bool r_flg = false;
	uint8_t *send_buff = ble_local_malloc(WHI_PRO_HEAD_SIZE+node_sum*BLE_LONG_ADDR_SIZE + 16);
	if(send_buff != NULL)
	{
		uint16_t frame_len = WHI_BuildReportLeaveNetNodeListProcess(send_buff,addr_list,node_sum,fr_seq);
		//
		if(frame_len >0)
		{
			//ble_net_ack_send_data(send_buff,frame_len);
			lc_net_my_info_t p_info;
			lc_net_get_net_self_info(&p_info);
			if(p_info.net_state == LC_NET_IN_STATE)
			{
				ble_net_debug_cnt_list.ble_to_plc_tx_cnt++;
				lc_net_hplc_tx_sta_frame(p_info.cco_addr,send_buff,frame_len);
			}
			r_flg = true;
		}
		ble_local_free(send_buff);
	}
	return r_flg;
}

//入网离网上报任务
void ble_mac_addr_in_leave_net_report_task()
{
	if(LcTimeOut(&(ble_mac_addr_in_leave_net_report_param.report_timer)))
	{
		uint32_t timer_next_ms = BLE_MAC_IN_LEAVE_NET_REPORT_SOLT_MS;
		ble_mac_addr_in_leave_net_report_param.report_seq = WHI_CreateLocalSeq();
		if(ble_mac_addr_in_leave_net_report_param.report_seq == 0)
		{
			ble_mac_addr_in_leave_net_report_param.report_seq = WHI_CreateLocalSeq();
		}
		ble_mac_addr_in_leave_net_report_param.addr_count = 0;
		
		switch(ble_mac_addr_in_leave_net_report_param.report_status)
		{
			case REPORT_CHECK_ADDR_IN_NET_STATUS:
			case REPORT_CHECK_ADDR_IN_NET_WAIT_ACK:
			{
				//检测离网节点上报；
				ble_mac_addr_in_leave_net_report_param.report_status = REPORT_CHECK_ADDR_LEAVE_NET_STATUS;
				uint16_t addr_count = get_report_check_leave_net_addr(ble_mac_addr_in_leave_net_report_param.report_addr_list);
				if(addr_count > 0)
				{
					ble_mac_addr_in_leave_net_report_param.addr_count = addr_count;
					if(false == ble_mac_addr_leave_net_report_send(addr_count
						,ble_mac_addr_in_leave_net_report_param.report_addr_list
						,ble_mac_addr_in_leave_net_report_param.report_seq))
					{
						timer_next_ms = BLE_MAC_IN_LEAVE_NET_REPORT_STEP_MS;
					}
				}
				else
				{
					timer_next_ms = BLE_MAC_IN_LEAVE_NET_REPORT_STEP_MS;
				}
				ble_mac_addr_in_leave_net_report_param.report_net_status = REPORT_CHECK_ADDR_LEAVE_NET_STATUS;
				ble_mac_addr_in_leave_net_report_param.report_status = REPORT_CHECK_ADDR_LEAVE_NET_WAIT_ACK;
				break;
			}
			case REPORT_CHECK_ADDR_LEAVE_NET_STATUS:
			case REPORT_CHECK_ADDR_LEAVE_NET_WAIT_ACK:
			{
				/*入网时会把上报标志置1，现在由于不需要放大器代报底下蓝牙节点，故屏蔽
				//检测在网节点上报；
				ble_mac_addr_in_leave_net_report_param.report_status = REPORT_CHECK_ADDR_IN_NET_STATUS;
				uint16_t addr_count = get_report_check_in_net_addr(ble_mac_addr_in_leave_net_report_param.report_addr_list);
				if(addr_count > 0)
				{
					ble_mac_addr_in_leave_net_report_param.addr_count = addr_count;
					if(false == ble_mac_addr_in_net_report_send(addr_count
						,ble_mac_addr_in_leave_net_report_param.report_addr_list
						,ble_mac_addr_in_leave_net_report_param.report_seq))
					{
						timer_next_ms = BLE_MAC_IN_LEAVE_NET_REPORT_STEP_MS;
					}
				}
				else
				{
					timer_next_ms = BLE_MAC_IN_LEAVE_NET_REPORT_STEP_MS;
				}
				*/
				ble_mac_addr_in_leave_net_report_param.report_net_status = REPORT_CHECK_ADDR_IN_NET_STATUS;
				ble_mac_addr_in_leave_net_report_param.report_status = REPORT_CHECK_ADDR_IN_NET_WAIT_ACK;
				break;
			}
			default:
				ble_mac_addr_in_leave_net_report_param.report_status = REPORT_CHECK_ADDR_IN_NET_STATUS;
				break;
		}
		StartTimer(&(ble_mac_addr_in_leave_net_report_param.report_timer),timer_next_ms);
	}
}

void ble_mac_addr_in_leave_net_report_init_task()
{
	memset(&ble_mac_addr_in_leave_net_report_param,0,sizeof(ble_mac_addr_in_leave_net_report_param));
	ble_mac_addr_in_leave_net_report_param.report_status = REPORT_CHECK_ADDR_LEAVE_NET_STATUS;
	StartTimer(&(ble_mac_addr_in_leave_net_report_param.report_timer),BLE_MAC_IN_LEAVE_NET_REPORT_SOLT_MS);
}

#endif

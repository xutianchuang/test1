#include "protocol_includes.h"
#include "os.h"
#include "common_includes.h"
#include <string.h>
#include "bsp.h"
#include "Revision.h"
#include "hplc_data.h"
#include "app_search_meter.h"
#include "app_event_IIcai.h"
#include "bps_flash.h"
#include "app_RF_IIcai.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//add by LiHuaQiang 2020.10.9  -START-
//----------------------------------------------------------------
//??
extern bool IsBindAddr;
extern void AppHandleLocal(u8 seq, u16 packetID, u8 protocol, u16 frameSeq, bool vaild, u8* data, u16 len, u8 sendType, u16 srcTei);
//----------------------------------------------------------------
//698.45读取新增事件列表和跟随上报状态字
uint8_t read_NEL_and_FRS_698[]={
0xFE ,0xFE ,0xFE ,0xFE ,0x68 ,0x31 ,0x00 ,0x43 ,0x05 ,0x26 ,0x04 ,0x60 ,0x10 ,0x19 ,0x16 ,0x00 ,0xB5 ,0xFF ,0x10 ,0x00 ,0x0D ,0x05 ,
0x02 ,0x03 ,0x02 ,0x33 ,0x20 ,0x02 ,0x00 ,0x20 ,0x15 ,0x02 ,0x00 ,0x00 ,0x01 ,0x10 ,0xB3 ,0x75 ,0x09 ,0x19 ,0x0F ,0xCF ,0x66 ,0x92 ,
0x4B ,0xBC ,0xB2 ,0xBA ,0xBA ,0xBE ,0x10 ,0xFD ,0x0B ,0xC2 ,0x16
    };
uint8_t answer_reset_statue_09_meter[]={0x68 ,0x22 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x68 ,0x94 ,0x00 ,0x86 ,0x16};
//645-07读事件通用，地址可变，数据标识可变
uint8_t read_event_07[]={0xFE ,0xFE ,0xFE ,0xFE ,0x68 ,0xAA ,0xAA ,0xAA ,0xAA ,0xAA ,0xAA ,0x68 ,0x11 ,0x04 ,0x34 ,0x48 ,0x33 ,0x37 ,0xC7 ,0x16};
//09表有事件上报报文
uint8_t event_645_07[]=    {0x68 ,0xAA ,0xAA ,0xAA ,0xAA ,0xAA ,0xAA ,0x68 ,0x91 ,0x13 ,0x34 ,0x48 ,0x33 ,0x37 ,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0xDD,0x32,0xDD,0xC7 ,0x16};
//09表无事件上报报文
uint8_t no_event_645_07[]= {0x68 ,0xAA ,0xAA ,0xAA ,0xAA ,0xAA ,0xAA ,0x68 ,0x91 ,0x12 ,0x34 ,0x48 ,0x33 ,0x37 ,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0xDD,0xDD,0xC7 ,0x16};
//645-07事件相关数据标识
uint8_t DI_event_statue[4]={0x34,0x48,0x33,0x37};//事件主动上报状态字
uint8_t DI_reset_event_statue[4]={0x36,0x48,0x33,0x37};//复位事件主动上报状态字
uint8_t DI_phaseB_volte[4]={0x33,0x35,0x34,0x35};//B相电压

uint8_t DI_open_meter_cover[4]={0x33,0x40,0x63,0x36};//开表盖总次数
uint8_t DI_power_cut[4]={0x33,0x33,0x44,0x36};//掉电总次数
uint8_t DI_calibration_time[4]={0x33,0x37,0x63,0x36};//校时总次数
uint8_t DI_meter_run_statue1[4]={0x34,0x38,0x33,0x37};//电表运行状态字1

uint8_t DI_open_button_box[4]={0x33,0x41,0x63,0x36};//开端钮盒总次数
uint8_t DI_phaseA_volte_lose[4]={0x34,0x33,0x34,0x43};//A相失压总次数
uint8_t DI_phaseB_volte_lose[4]={0x34,0x33,0x35,0x43};//B相失压总次数
uint8_t DI_phaseC_volte_lose[4]={0x34,0x33,0x36,0x43};//C相失压总次数
uint8_t DI_phaseA_current_lose[4]={0x34,0x33,0x34,0x4B};//A相失流总次数
uint8_t DI_phaseB_current_lose[4]={0x34,0x33,0x35,0x4B};//B相失流总次数
uint8_t DI_phaseC_current_lose[4]={0x34,0x33,0x36,0x4B};//C相失流总次数
uint8_t DI_phaseA_break[4]={0x34,0x33,0x34,0x46};//A相断相总次数
uint8_t DI_phaseB_break[4]={0x34,0x33,0x35,0x46};//B相断相总次数
uint8_t DI_phaseC_break[4]={0x34,0x33,0x36,0x46};//C相断相总次数
uint8_t DI_volte_reverse[4]={0x34,0x33,0x33,0x47};//电压逆相序总次数

//事件采集RS485口发送队列
typedef struct
{
	uint16_t head;
	uint16_t tail;
	uint16_t RecCount;
        uint8_t DataLen[RS485_MULTI_SEND];
	uint8_t buff[RS485_MULTI_SEND][RS485_BUFFER_SIZE];
}EVENT_RS485_SEND;

EVENT_RS485_SEND event_RS485_send_data;

//国网09类型表的主动上报状态字,最多可以存32个表地址和09类型表的主动上报状态字
typedef struct
{
    uint8_t meter_num;// 09类型电表数量
    uint8_t meter_address[STATUE_MULTI][6];//确认完表类型后，从table[]中选取09类型表的地址填入
    uint32_t event_count[STATUE_MULTI][15];//每个电表存15个事件发生次数
    uint8_t event_statue[STATUE_MULTI][STATUE_BUFFER_SIZE];//主动上报状态字
    uint8_t cco_comfirm_flag[STATUE_MULTI];//收到CCO确认标志
    uint8_t event_count_dif[STATUE_MULTI];//两次读取的事件发生次数之差
    uint8_t bat_volte_low_flag[STATUE_MULTI];//电池欠压只报1次标志
}EVENT_STATUE_09METER;

EVENT_STATUE_09METER event_statue_645_09ver;

typedef struct{
    uint8_t meter_num;// 13??????
    uint8_t meter_address[EVENT_HPLC_MULTI_SEND][6];
    uint8_t len[EVENT_HPLC_MULTI_SEND];
    uint8_t buff[EVENT_HPLC_MULTI_SEND][EVENT_HPLC_BUFFER_SIZE];
}EVENT_HPLC_SEND_DATA;

EVENT_HPLC_SEND_DATA event_HPLC_send_data;

uint8_t event_table_cnt=0;//遍历table计数 0:开始遍历table,采集事件数据入队列 
uint8_t report_09meter_cnt=0;//遍历09电表事件上报
uint8_t report_13meter_cnt=0;//遍历13电表事件上报
uint32_t read_meter_event_statue_start_time;//采集事件的起始时间
uint8_t event_statue_null[12]={0};
uint8_t report_meterlist_flag=OFF;//ON:开启上报搜表信息 OFF:关闭上报搜表信息
uint32_t report_meterlist_start_time;//上报时候的时间
uint8_t report_meterlist_buf[232]={0};//上报搜表信息报文中 数据域最大值 8+224
uint16_t report_meterlist_wait_answer_timer=OFF;//已经上报了搜表信息,等待CCO回复确认
uint8_t write_flash_buf[2114];
uint8_t event09_report_flag_one_time=0;//上电第一次读取09表事件次数先存储 0: 存储不上报1:存储判断可上报
u8 read_event_start_flag=false;//false--不可以读事件  true--可以读事件
u8 check_send_count=0;
u8 traversal_list_count=0;
u8 collector_addr[6]={0x99,0x99,0x99,0x99,0x99,0x89};//采集器地址

u8 get_read_event_start_flag(void)
{
	return 	read_event_start_flag;
}
void set_read_event_start_flag(u8 data)
{
	read_event_start_flag = data;
}
void set_event_statue_645_09ver_addr(void)
{
    uint8_t i,j;
    struct strList_Meter *p;
    for(i=0;i<SEARCH_METER_FLAG.table_index;i++)
    {
        p = (struct strList_Meter *)&SEARCH_METER_FLAG.table[i*7];
        if(p->Type.meter_type1 == 0 && p->Type.Statute == 2)
        {
            for(j=0;j<STATUE_MULTI;j++)
            {
				if (memIsHex(event_statue_645_09ver.meter_address[j], 0, 6)  || memcmp(event_statue_645_09ver.meter_address[j], &SEARCH_METER_FLAG.table[i * 7], 6) == 0)
                {
                    memcpy(event_statue_645_09ver.meter_address[j],&SEARCH_METER_FLAG.table[i*7],6);
                    event_statue_645_09ver.meter_num = j+1;
                    break;
                }                
            }
        }
    }    
}
//----------------------------------------------------------------
//检查07表的类型:13表 09单相表 09三相表
//----------------------------------------------------------------
void check_meter_type(void)
{
	u32 list;        
    u8 buf_07[20];   	

	if(AppUseUartFlag != 0)
		return;
	
    if(SEARCH_METER_FLAG.TableList==0 || GetCheckMeterEndFlag()==true)
    {
        return;
    }
    if(check_send_count >= SEARCH_METER_FLAG.table_index)
    {   
    	traversal_list_count++;
        SEARCH_METER_FLAG.list_count=0;
        check_send_count =0;        
        if(traversal_list_count>=2)
        {
            traversal_list_count=0;
			SetCheckMeterEndFlag(true);            
            set_event_statue_645_09ver_addr();
			set_read_event_start_flag(true);
            return;
        }
    }
    list=SEARCH_METER_FLAG.TableList;
    list=list>>SEARCH_METER_FLAG.list_count;     
    if(((list&0x00000001) == 0x00000001) && (check_send_count < SEARCH_METER_FLAG.table_index))//
    {
        memcpy(meter_list.address,(u8 *)&SEARCH_METER_FLAG.table[SEARCH_METER_FLAG.list_count*7],6);
		        
        memcpy(buf_07,read_event_07,sizeof(read_event_07));
        if(traversal_list_count==0)
        {
            memcpy((u8 *)&buf_07[14],DI_event_statue,4);
            meter_list.Type.meter_type1=0;
        }
        else if(traversal_list_count==1)
        {
            memcpy((u8 *)&buf_07[14],DI_phaseB_volte,4);
            meter_list.Type.meter_type2=0;
        }
        
        memcpy((u8 *)&buf_07[5],meter_list.address,6);//
        buf_07[18]=GetCheckSum(buf_07+4,14);
        
		meter_list.Type.Statute = (SEARCH_METER_FLAG.table[SEARCH_METER_FLAG.list_count*7+6]&0x03);//规约类型
		
        if(meter_list.Type.Statute == 0x02)//645-07
        {
			u8 addr[6] = {0};
			AppLocalReceiveData( addr, addr, IFR_READ_METER_5000MS, 0, buf_07, sizeof(read_event_07), RS485DataDeal, 0, 0, BAUD_SEARCH );
//			AppLocalReceiveData(SEARCH_METER_1500MS,
//				0,
//				0,
//				0,
//				0,
//				0,
//				0,
//				0, 
//				buf_07, 
//				sizeof(read_event_07), 
//				RS485DataDeal,
//				0,
//				0);          
			SetWait485DataFlag(UART_SCAN_485);          
            
            check_send_count++;
        }
		else//其他规约电表不用确认                	
            check_send_count++;        
        SEARCH_METER_FLAG.list_count++;
        
    }
    else
    {
        check_send_count++;
        SEARCH_METER_FLAG.list_count++;
    }
}
//----------------------------------------------------------------
//----------------------------------------------------------------
void buf_crc_sum_cal(uint8_t* buff645,uint8_t len)
{
    buff645[len-2]=GetCheckSum(buff645, len-2);
}
void event_report_09meter(void)
{

    for(;;)
    {        
        if(report_09meter_cnt >= event_statue_645_09ver.meter_num)
        {
            report_09meter_cnt = 0;
            if(event_statue_645_09ver.cco_comfirm_flag[report_09meter_cnt] == OFF)
                return;
        }
        if(event_statue_645_09ver.cco_comfirm_flag[report_09meter_cnt] == ON)
            break;
        report_09meter_cnt++;
    }
    if(event_statue_645_09ver.cco_comfirm_flag[report_09meter_cnt] == ON)
    {        
        if(memcmp(event_statue_645_09ver.event_statue[report_09meter_cnt],event_statue_null,12)==0)
        {}
        else
        {
            memcpy(&event_645_07[1],event_statue_645_09ver.meter_address[report_09meter_cnt],6);
            memcpy(&event_645_07[14],event_statue_645_09ver.event_statue[report_09meter_cnt],12);
            memadd(&event_645_07[14],0x33, 12);
            event_645_07[27]=event_statue_645_09ver.event_count_dif[report_09meter_cnt] + 0x33;
            buf_crc_sum_cal(event_645_07,31);            
            //nomal_event_report(&event_645_07[1],event_645_07,sizeof(event_645_07));
            
			AppEventState = APP_EVENT_WAIT_LOCAL;
			//临时用0x08代替APP_PACKET_EVENT_REPORT, lxl changed flag 20211024
			//AppHandleLocal(0, 0x08, 0, 0, true, event_645_07, sizeof(event_645_07), 0, 0);
			//事件交付HPLC后清除事件标志
			reset_cco_comfirm_flag_fun(NULL,event_statue_645_09ver.meter_address[report_09meter_cnt]);
		} 
		
    }    
}
void event_report_13meter(void)
{
    for(;;)
    {        
        if(report_13meter_cnt >= event_HPLC_send_data.meter_num)
        {
            report_13meter_cnt = 0;
            if(memIsHex(event_HPLC_send_data.meter_address[report_13meter_cnt],0,6) )
                return;
        }
        if(!memIsHex(event_HPLC_send_data.meter_address[report_13meter_cnt],0,6)) 
            break;
        report_13meter_cnt++;
    }    
    //nomal_event_report(event_HPLC_send_data.meter_address[report_13meter_cnt],event_HPLC_send_data.buff[report_13meter_cnt],event_HPLC_send_data.len[report_13meter_cnt]);
	
	AppEventState = APP_EVENT_WAIT_LOCAL;

	if(EventPdu)
	{
		FreeSendPDU(EventPdu);
		EventPdu = NULL;
	}
	AppHandleEvent(NULL, NULL, 0, true, event_HPLC_send_data.buff[report_13meter_cnt], event_HPLC_send_data.len[report_13meter_cnt], SENDTYPE_SINGLE, 0);
	//事件交付HPLC后清除事件标志
	reset_cco_comfirm_flag_fun(NULL, event_HPLC_send_data.meter_address[report_13meter_cnt]);
}
void AppEventReportIIcai(void)
{
  static uint8_t event_in_turn_flag=0;

  if( GetStaBaseAttr()->NetState != NET_IN_NET || (AppEventState!=APP_EVENT_IDLE) )//上报事件需要等待入网之后 没有其他事件后再进行
  	return;

  if(event_in_turn_flag)
  	event_report_09meter();
  else
  	event_report_13meter();

  event_in_turn_flag = ~event_in_turn_flag;
}
void reset_cco_comfirm_flag_fun(uint16_t *serial,uint8_t *addr)
{
    uint8_t i;    

    for(i=0;i<event_statue_645_09ver.meter_num;i++)
    {
        if(memcmp(event_statue_645_09ver.meter_address[i],addr,6)==0)
        {
            event_statue_645_09ver.cco_comfirm_flag[i] = OFF;            
            //print_debug("event_statue_645_09ver.cco_comfirm_flag[i] = OFF, i=",&i,1);
            return;
        }
    }
    for(i=0;i<event_HPLC_send_data.meter_num;i++)
    {
        if(memcmp(event_HPLC_send_data.meter_address[i],addr,6)==0)
        {
            memset(event_HPLC_send_data.meter_address[i],0,6);            
            //print_debug("event_HPLC_send_data.meter_address[i] clear, i=",&i,1);
            break;
        }
    }
    
}
void hplc_answer_reset_statue_09_meter_fun(uint8_t *addr)
{
	memcpy(&answer_reset_statue_09_meter[1],addr,6);
	answer_reset_statue_09_meter[10]=GetCheckSum(answer_reset_statue_09_meter,10);
}
uint8_t is_right_reset_event_statue_645_09ver_fun(uint8_t *data,u16 len)
{
    uint8_t i;	
    if(memcmp(&data[10],DI_reset_event_statue,4)==0 && data[8]==0x14)
    {
        for(i=0;i<event_statue_645_09ver.meter_num;i++)
        {
            if(memcmp(event_statue_645_09ver.meter_address[i],&data[1],6)==0)
            {
                memset(event_statue_645_09ver.event_statue[i],0,12);                               				
				hplc_answer_reset_statue_09_meter_fun(event_statue_645_09ver.meter_address[i]);				
                return TRUE;
            }
        }
    }
    return FALSE;
}

//
void delete_wrong_meter(void)
{
    uint8_t i,j;
    if(IsBindAddr)
    {
        for(i=0;i<SEARCH_METER_FLAG.table_index;i++)
        {
            if(memcmp(SEARCH_METER_FLAG.online_addr,&SEARCH_METER_FLAG.table[i*7],6)==0)
            {
                //print_debug("online_addr meter in table[]: ",online_addr,6);
                break;
            }
        }
        if(i == SEARCH_METER_FLAG.table_index )
        {
            if(memcmp(SEARCH_METER_FLAG.online_addr,collector_addr,6)!=0)
            {
                //print_debug("online_addr meter error,reset...",0,0);
                
            }
        }
    }
    for(j=0;j<event_statue_645_09ver.meter_num;j++)
    {
        for(i=0;i<SEARCH_METER_FLAG.table_index;i++)
        {
            if(memcmp(event_statue_645_09ver.meter_address[j],&SEARCH_METER_FLAG.table[i*7],6)==0)
            {
                //print_debug("find right meter in table[]: ",event_statue_645_09ver.meter_address[j],6);
                break;
            }
        }
        if(i == SEARCH_METER_FLAG.table_index)
        {
            //print_debug("no find right meter in table[],delete: ",event_statue_645_09ver.meter_address[j],6);
            memset(event_statue_645_09ver.meter_address[j],0,6);    
			memset(event_statue_645_09ver.event_count[j],0,15); 
        }
    }
}

void write_09meter_event_to_flash(void)
{	
    memcpy(write_flash_buf,&event_statue_645_09ver.meter_num,2113);
    write_flash_buf[2113] = GetCheckSum(write_flash_buf,2113);
	
}
void read_09meter_event_from_flash(void)
{
    uint16_t i;
    memset(&event_statue_645_09ver.meter_num,0,2529);
    for(i=0;i<2114;i++)
        write_flash_buf[i] = *( ((uint8_t *)II_STA_EVENT_DATA_ADDR)+i );
    if(write_flash_buf[2113] != GetCheckSum(write_flash_buf,2113))
    {        
        return;
    }
    memcpy(&event_statue_645_09ver.meter_num,write_flash_buf,2113);
}
uint8_t is_right_09_sanxiang(uint8_t *addr)
{
    uint8_t i;
    struct strList_Meter *p;
    for(i=0;i<SEARCH_METER_FLAG.table_index;i++)
    {
        if(memcmp(addr,&SEARCH_METER_FLAG.table[i*7],6)==0)
        {
            p = (struct strList_Meter *)&SEARCH_METER_FLAG.table[i*7];
            if(p->Type.meter_type2 == 1)
                return TRUE;
            else
                return FALSE;
        }
    }    
    return FALSE;
}
void event_13meter_data_queue_in(uint8_t *addr,uint8_t *data,uint8_t len)
{
    uint8_t i;
    if(len >= EVENT_HPLC_BUFFER_SIZE)return;
    for(i=0;i<EVENT_HPLC_MULTI_SEND;i++)
    {
        if(memIsHex(event_HPLC_send_data.meter_address[i],0,6))
        {
            memcpy(event_HPLC_send_data.meter_address[i],addr,6);
            event_HPLC_send_data.len[i]=len;
            memcpy(event_HPLC_send_data.buff[i],data,len);
            event_HPLC_send_data.meter_num = i+1;
            break;
        }
    }
}
void event_count_dif_record_fun(uint8_t index,uint32_t last_cnt,uint32_t cur_cnt)
{
    uint32_t cnt_dif;
    if(last_cnt > cur_cnt)
        cnt_dif = 0xFFFFFF - last_cnt + cur_cnt + 1;
    else
        cnt_dif = cur_cnt - last_cnt;
    if(cnt_dif > 0xFF)
        cnt_dif = 0xFF;
    event_statue_645_09ver.event_count_dif[index]=cnt_dif;   
}
void rs485_recv_event_data_deal_fun(struct frame645 *pframe,UINT16 receive_len)
{
    uint32_t *ptr,temp;
    uint8_t i,buf[4]={0};
    uint16_t run_statue1;
    event_count_645 *p ;
    event_statue_bit_645 *p2;
    
    ptr=(uint32_t*)&(pframe->data[0]);
    
    if(pframe->control_code.control_byte == 0x91)
    {
        if(*ptr == DI_event_statue_H)
        {
			//事件上报前12个字节是事件帧  最后在两个DD中间的是发生次数 (有些或单是两个DD)
			if (!memIsHex(&(pframe->data[4]),0x33, 12))
            {
                //print_debug("645-07 13meter event",0,0);                
                event_13meter_data_queue_in(pframe->addr,(uint8_t*)pframe,receive_len);
            }
            //else
                //print_debug("NO 645-07 13meter event",0,0);
            return;
        }
        for(i=0;i<STATUE_MULTI;i++)
        {
            if(memcmp(event_statue_645_09ver.meter_address[i],pframe->addr,6) == 0)
            {
                break;
            }
        }
        if(i == STATUE_MULTI)
        {
            return;
        }
        p= (event_count_645 *)event_statue_645_09ver.event_count[i];        
        p2 = (event_statue_bit_645 *)event_statue_645_09ver.event_statue[i];
        if(*ptr == DI_meter_run_statue1_H)
        {
            memcpy(buf,&(pframe->data[4]),2);
            temp=(*((uint32_t*)buf)) - 0x3333;
        }
        else
        {
            memcpy(buf,&(pframe->data[4]),3);
            temp=(*((uint32_t*)buf)) - 0x333333;
        }
        switch(*ptr) 
        {
            case DI_open_meter_cover_H:                
                if(p->open_meter_cover_cnt !=temp && p->open_meter_cover_cnt !=0)
                {
                    p2->bit10=1;
                    event_count_dif_record_fun(i,p->open_meter_cover_cnt,temp);
                }
                else
                    p2->bit10=0;
                p->open_meter_cover_cnt = temp;
                //print_debug("p->open_meter_cover_cnt = ",(uint8_t*)&temp,4);
                break;
            case DI_power_cut_H:
                if(p->power_cut_cnt !=temp && p->power_cut_cnt !=0)
                {
                    p2->bit69=1;
                    event_count_dif_record_fun(i,p->power_cut_cnt,temp);
                }
                else
                    p2->bit69=0;
                p->power_cut_cnt = temp;
                //print_debug("p->power_cut_cnt = ",(uint8_t*)&temp,4);
                break; 
            case DI_calibration_time_H:
                if(p->calibration_time_cnt !=temp && p->calibration_time_cnt !=0)
                {
                    p2->bit84=1;
                    event_count_dif_record_fun(i,p->calibration_time_cnt,temp);
                }
                else
                    p2->bit84=0;
                p->calibration_time_cnt = temp;
                //print_debug("p->calibration_time_cnt = ",(uint8_t*)&temp,4);
                break;
            case DI_meter_run_statue1_H:
                run_statue1 =(uint16_t)(temp & 0x0000FFFF);
                if(TRUE == is_right_09_sanxiang(pframe->addr))
                {
                    if((run_statue1 & 0x0008) == 0x0008)
                    {
                        p2->bit8=1;
                        event_statue_645_09ver.event_count_dif[i]=0xFF;                        
                    }
                    else
                    {
                        p2->bit8=0;
                    }
                    if((run_statue1 & 0x0004) == 0x0004)
                    {
                        if(event_statue_645_09ver.bat_volte_low_flag[i] == 0)
                        {
                            event_statue_645_09ver.bat_volte_low_flag[i] = 1;
                            p2->bit3=1;
                            event_statue_645_09ver.event_count_dif[i]=0xFF;
                        }
                        else
                            p2->bit3=0;
                    }
                    else
                    {
                        p2->bit3=0;
                    }                    
                    //print_debug("09 sanxiang DI_meter_run_statue1_H",0,0);
                }
                else
                {
                    if((run_statue1 & 0x0004) == 0x0004)
                    {
                        if(event_statue_645_09ver.bat_volte_low_flag[i] == 0)
                        {
                            event_statue_645_09ver.bat_volte_low_flag[i] = 1;
                            p2->bit3=1;
                            event_statue_645_09ver.event_count_dif[i]=0xFF;
                            
                        }
                        else
                            p2->bit3=0;
                    }
                    else
                    {
                        p2->bit3=0;
                    }
                    if(memcmp(event_statue_645_09ver.event_statue[i],event_statue_null,12) != 0)
                    {
                        event_statue_645_09ver.cco_comfirm_flag[i]=ON;
                        //print_debug("event_statue_645_09ver.cco_comfirm_flag[i] = ON",0,0);
                    }
                    //print_debug("09 danxiang DI_meter_run_statue1_H",0,0);
                }
                break;
            case DI_open_button_box_H:
                if(p->open_button_box_cnt_cnt !=temp && p->open_button_box_cnt_cnt !=0)
                {
                    p2->bit11=1;
                    event_count_dif_record_fun(i,p->open_button_box_cnt_cnt,temp);
                }
                else
                    p2->bit11=0;
                p->open_button_box_cnt_cnt = temp;
                //print_debug("p->open_button_box_cnt_cnt = ",(uint8_t*)&temp,4);
                break; 
            case DI_phaseA_volte_lose_H:
                if(p->phaseA_volte_lose_cnt !=temp && p->phaseA_volte_lose_cnt !=0)
                {
                    p2->bit16=1;
                    event_count_dif_record_fun(i,p->phaseA_volte_lose_cnt,temp);
                }
                else
                    p2->bit16=0;
                p->phaseA_volte_lose_cnt = temp;
                //print_debug("p->phaseA_volte_lose_cnt = ",(uint8_t*)&temp,4);
                break;
            case DI_phaseB_volte_lose_H:
                if(p->phaseB_volte_lose_cnt !=temp && p->phaseB_volte_lose_cnt !=0)
                {
                    p2->bit32=1;
                    event_count_dif_record_fun(i,p->phaseB_volte_lose_cnt,temp);
                }
                else
                    p2->bit32=0;
                p->phaseB_volte_lose_cnt = temp;
                //print_debug("p->phaseB_volte_lose_cnt = ",(uint8_t*)&temp,4);
                break;
            case DI_phaseC_volte_lose_H:
                if(p->phaseC_volte_lose_cnt !=temp && p->phaseC_volte_lose_cnt !=0)
                {
                    p2->bit48=1;
                    event_count_dif_record_fun(i,p->phaseC_volte_lose_cnt,temp);
                }
                else
                    p2->bit48=0;
                p->phaseC_volte_lose_cnt = temp;
                //print_debug("p->phaseC_volte_lose_cnt = ",(uint8_t*)&temp,4);
                break;
            case DI_phaseA_current_lose_H:
                if(p->phaseA_current_lose_cnt !=temp && p->phaseA_current_lose_cnt !=0)
                {
                    p2->bit19=1;
                    event_count_dif_record_fun(i,p->phaseA_current_lose_cnt,temp);
                }
                else
                    p2->bit19=0;
                p->phaseA_current_lose_cnt = temp;
                //print_debug("p->phaseA_current_lose_cnt = ",(uint8_t*)&temp,4);
                break;
            case DI_phaseB_current_lose_H:
                if(p->phaseB_current_lose_cnt !=temp && p->phaseB_current_lose_cnt !=0)
                {
                    p2->bit35=1;
                    event_count_dif_record_fun(i,p->phaseB_current_lose_cnt,temp);
                }
                else
                    p2->bit35=0;
                p->phaseB_current_lose_cnt = temp;
                //print_debug("p->phaseB_current_lose_cnt = ",(uint8_t*)&temp,4);
                break;
            case DI_phaseC_current_lose_H:
                if(p->phaseC_current_lose_cnt !=temp && p->phaseC_current_lose_cnt !=0)
                {
                    p2->bit51=1;
                    event_count_dif_record_fun(i,p->phaseC_current_lose_cnt,temp);
                }
                else
                    p2->bit51=0;
                p->phaseC_current_lose_cnt = temp;
                //print_debug("p->phaseC_current_lose_cnt = ",(uint8_t*)&temp,4);
                break;
            case DI_phaseA_break_H:
                if(p->phaseA_break_cnt !=temp && p->phaseA_break_cnt !=0)
                {
                    p2->bit23=1;
                    event_count_dif_record_fun(i,p->phaseA_break_cnt,temp);
                }
                else
                    p2->bit23=0;
                p->phaseA_break_cnt = temp;
                //print_debug("p->phaseA_break_cnt = ",(uint8_t*)&temp,4);
                break;
            case DI_phaseB_break_H:
                if(p->phaseB_break_cnt !=temp && p->phaseB_break_cnt !=0)
                {
                    p2->bit39=1;
                    event_count_dif_record_fun(i,p->phaseB_break_cnt,temp);
                }
                else
                    p2->bit39=0;
                p->phaseB_break_cnt = temp;
                //print_debug("p->phaseB_break_cnt = ",(uint8_t*)&temp,4);
                break;
            case DI_phaseC_break_H:
                if(p->phaseC_break_cnt !=temp && p->phaseC_break_cnt !=0)
                {
                    p2->bit55=1;
                    event_count_dif_record_fun(i,p->phaseC_break_cnt,temp);
                }
                else
                    p2->bit55=0;
                p->phaseC_break_cnt = temp;
                //print_debug("p->phaseC_break_cnt = ",(uint8_t*)&temp,4);
                break;
            case DI_volte_reverse_H:
                if(p->volte_reverse_cnt !=temp && p->volte_reverse_cnt !=0)
                {
                    p2->bit64=1;
                    event_count_dif_record_fun(i,p->volte_reverse_cnt,temp);
                }
                else
                    p2->bit64=0;
                p->volte_reverse_cnt = temp;
                if(memcmp(event_statue_645_09ver.event_statue[i],event_statue_null,12) != 0)
                {
                    event_statue_645_09ver.cco_comfirm_flag[i]=ON;
                    //print_debug("event_statue_645_09ver.cco_comfirm_flag[i] = ON",0,0);
                }
                //print_debug("p->volte_reverse_cnt = ",(uint8_t*)&temp,4);
                break;
            default:break;
        }
    }
}

void event_port_monitor(void)
{
	if(AppUseUartFlag != 0)
		return;
	
	if(event_RS485_send_data.RecCount>0)
    {
    	if(event_RS485_send_data.tail==event_RS485_send_data.head)
    	{
    		return;
    	}
    	else
    	{
    		if(event_RS485_send_data.head < RS485_MULTI_SEND)
    		{
				u8 addr[6] = {0};
				AppLocalReceiveData( addr, addr, SEARCH_METER_1500MS, 0, event_RS485_send_data.buff[event_RS485_send_data.head], event_RS485_send_data.DataLen[event_RS485_send_data.head], RS485DataDeal, 0, 0, BAUD_SEARCH );
       
				SetWait485DataFlag(UART_EVENT_DATA); 
    			event_RS485_send_data.head++ ;
    			event_RS485_send_data.RecCount--;
    		}
    	}
    }
}
void queue_in(uint8_t*buf,uint8_t len)
{
    if(event_RS485_send_data.tail < RS485_MULTI_SEND)
    {
        event_RS485_send_data.DataLen[event_RS485_send_data.tail]=len;
    	memcpy(event_RS485_send_data.buff[event_RS485_send_data.tail],buf,len);    	
        event_RS485_send_data.tail++;
    	event_RS485_send_data.RecCount++;
    }
}
void queue_in_645(uint8_t *addr,uint8_t *di)
{
    memcpy(&read_event_07[14],di,4);
    memcpy(&read_event_07[5],addr,6);    
    read_event_07[18]=GetCheckSum(read_event_07+4, 14);
    queue_in(read_event_07,20); 
}

void read_meter_event_statue_on_time(void*arg)
{
    {
        event_table_cnt=0;
        event09_report_flag_one_time=1;
        //print_debug("60min over",0,0);
    }
}
void buf_crc16_cal(uint8_t* buff698)
{
    UINT16 crc_value=0;
    crc_value = GetCrc16(buff698+5,11);
	
	buff698[16] = crc_value&0x00ff;
	buff698[17] = (crc_value>>8)&0x00ff;

	crc_value = GetCrc16(buff698+5,47);
	
	buff698[52] = crc_value&0x00ff;
	buff698[53] = (crc_value>>8)&0x00ff;	
}

void event_RS485_send_data_queue_in(void)
{
    struct strList_Meter *p;
    if(event_table_cnt>=SEARCH_METER_FLAG.table_index)
        return;
    if(get_read_event_start_flag() == true && GetCheckTableFlag() == false)
    {
        if(event_table_cnt == 0)
        {
            event_RS485_send_data.head=0;
            event_RS485_send_data.tail=0;
            event_RS485_send_data.RecCount=0;
            //print_debug("event_RS485_send_data_queue_in start",0,0);
        }
        p=(struct strList_Meter *)&SEARCH_METER_FLAG.table[event_table_cnt*7];
        event_table_cnt++;
                
        if(p->Type.meter_type1 == 1)
        { 
            queue_in_645(p->address,DI_event_statue);
            //print_debug("645 13:",p->address,6);
        }
        else if(p->Type.meter_type2 == 1)
        {
            queue_in_645(p->address,DI_calibration_time); 
            queue_in_645(p->address,DI_power_cut);
            
            queue_in_645(p->address,DI_phaseC_break);
            queue_in_645(p->address,DI_phaseC_current_lose); 
            queue_in_645(p->address,DI_phaseC_volte_lose); 
            queue_in_645(p->address,DI_phaseB_break); 
            queue_in_645(p->address,DI_phaseB_current_lose);
            queue_in_645(p->address,DI_phaseB_volte_lose); 
            queue_in_645(p->address,DI_phaseA_break); 
            queue_in_645(p->address,DI_phaseA_current_lose); 
            queue_in_645(p->address,DI_phaseA_volte_lose); 
            queue_in_645(p->address,DI_open_button_box);             
            queue_in_645(p->address,DI_open_meter_cover);             
            queue_in_645(p->address,DI_meter_run_statue1); 
			queue_in_645(p->address,DI_volte_reverse);
            //print_debug("645 09 sanxiang:",p->address,6);
        }
        else if(p->Type.meter_type2 == 0)
        {
            queue_in_645(p->address,DI_calibration_time); 
            queue_in_645(p->address,DI_power_cut); 
            queue_in_645(p->address,DI_open_meter_cover); 
            queue_in_645(p->address,DI_meter_run_statue1); 
            //print_debug("645 09 danxiang:",p->address,6);
        }				  
    }
	if (event_table_cnt>=SEARCH_METER_FLAG.table_index)//搜索到了最后
	{
		HPLC_RegisterTmr(read_meter_event_statue_on_time,NULL,60*60*1000UL,TMR_OPT_CALL_ONCE);//一个小时后重新搜索
	}
}
//----------------------------------------------------------------
//-END-
//----------------------------------------------------------------
//-START-
//----------------------------------------------------------------
int left_shift_1bit(uint8_t *str, uint8_t len)
{
	 int i;
	 for(i = 0; i < len; i++)
	 {
		 str[i] = str[i] >> 1;
		 
		 if(str[i+1] & 0x01)
		 {
			 str[i] = str[i] | 0x80;	
		 }	  
	 }	  
	 return 0;
}
void left_shift_4bit(uint8_t *str, uint8_t len)
{
	uint8_t i;
	for(i=0;i<4;i++)
		left_shift_1bit(str,len);
}
void get_collector_addr_fun(void)
{
//	uint8_t temp[11];
//	memcpy(temp, StaChipID.module_id,11);
//	left_shift_4bit(temp,11);
//	memcpy(collector_addr,temp,6);	
	
	//结构体信息不一致, 临时这样改, lxl changed flag 20211024
	memcpy(collector_addr,StaChipID.mac,6);	
}

//载波发送搜表信息
//void hplc_send_meterlist_to_cco(void)
//{
//    uint16_t len = 8 + SEARCH_METER_FLAG.meter_total * 7;    
//    AppEventState = APP_EVENT_WAIT_LOCAL;
	//为了编译, 临时这样改, lxl changed flag 20211024
	//AppHandleLocal(0, 0x08, 0xff, 0, true, report_meterlist_buf, len, 0, 0);//0xff:功能码--采集器触发//AppHandleLocal(0, APP_PACKET_EVENT_REPORT, 0xff, 0, true, report_meterlist_buf, len, 0, 0);//0xff:功能码--采集器触发//
//}
void copy_meter_imfor_fun(void)
{
    uint8_t i=0,j=0;
    uint32_t table_list_tmp=SEARCH_METER_FLAG.TableList;
    report_meterlist_buf[0] = 50;//代表搜表结果
    get_collector_addr_fun();
    memcpy(report_meterlist_buf+1,collector_addr,6);//采集器地址
    report_meterlist_buf[7] = SEARCH_METER_FLAG.meter_total;//下接从节点数量
    for(i=0;i<32;i++)
    {
        if((table_list_tmp&0x00000001)==0x00000001)
        {
	        memcpy(&report_meterlist_buf[8+7*j],&SEARCH_METER_FLAG.table[i*7], 6);//表地址
			report_meterlist_buf[8+7*j+6] = (SEARCH_METER_FLAG.table[i*7+6] & 0x03);//规约类型			
			j++;
	    }
        table_list_tmp = table_list_tmp >> 1;
	}
}
void report_meterlist_flag_set_fun(u8 flag)
{
	report_meterlist_flag = flag;   
}
u8 report_meterlist_flag_get_fun(void)
{
	return report_meterlist_flag;
}
//启动将搜表信息上报,在主循环中调用
void hplc_start_report_meterlist_fun(void)
{  
    if(GetStaBaseAttr()->NetState!=NET_IN_NET )//不在线 	
    {
    	report_meterlist_flag_set_fun(true);
        return;
    }
    else if(GetSearchOneMeterFlag())//全A搜到1块电表
    {}
    else if(GetCheckTableFlag())//全A未搜到电表,正在搜表中...
		return;
    if(report_meterlist_flag_get_fun() && (AppEventState==APP_EVENT_IDLE))
    {
        copy_meter_imfor_fun();
       	//hplc_send_meterlist_to_cco(); 
		report_meterlist_flag_set_fun(false);
    }
     
}
//----------------------------------------------------------------
//-END-
//----------------------------------------------------------------
//-START-
//----------------------------------------------------------------
u8 poweroff_report_buff[42];
u8 list_flash_power_off[42];
bool check_poweroff_meter_end_flag=false;//true

void SetCheckPowerOffMeterEndFlag(bool data)
{
	check_poweroff_meter_end_flag = data;
}
bool GetCheckPowerOffMeterEndFlag(void)
{
	return check_poweroff_meter_end_flag;
}
void write_list_flash_power_off(void)
{
    uint8_t flash_bytes[43]={0};
    
   	memcpy(flash_bytes,SEARCH_METER_FLAG.table,42);
	flash_bytes[42]=GetCheckSum(flash_bytes,42);//flash
	DataFlashInit();
	EraseFlash(II_STA_POWEROFF_ADDR,DATA_FLASH_SECTION_SIZE);
	WriteFlash(II_STA_POWEROFF_ADDR,flash_bytes,43);
	DataFlashClose();
}
void read_list_flash_power_off(void)
{
    uint8_t i,flash_temp[43];
	
	//修复复电时, 二采有电电表都没电, 但电表复电后, 上报电表都不在线的问题. add, by lxl, 20211208@1600
	ReadPowerOffFlag();
	if( !IsPowerOff )
	{
		return;
	}
	
	for(i=0;i<43;i++)
		flash_temp[i] = *(((uint8_t *)II_STA_POWEROFF_ADDR)+i);
	if(flash_temp[42] == GetCheckSum(flash_temp,42))
	{
		memcpy(list_flash_power_off,flash_temp,42);
		for(i=0;i<6;i++)
		{
			if(!memIsHex(&list_flash_power_off[i*7],0,6))
			{
				SEARCH_METER_FLAG.TableList = ((SEARCH_METER_FLAG.TableList<<1) | 0x00000001);	
				SEARCH_METER_FLAG.table_index ++;
				SEARCH_METER_FLAG.meter_total ++;                
			}
		}
		memcpy(SEARCH_METER_FLAG.table,list_flash_power_off,42);
		SEARCH_METER_FLAG.back_List=0x00;//复电前初始化, add, by lxl, 20211208@0900

#ifdef DEBUG_SEARCH_LOG_OUT
        debug_str(DEBUG_LOG_APP, "read_list_flash_power_off: %d\r\n", SEARCH_METER_FLAG.meter_total);
		for(i=0;i<6;i++)
		{
			if(!memIsHex(&list_flash_power_off[i*7],0,6))
			{
			    debug_str( DEBUG_LOG_APP, "  %02x%02x%02x%02x%02x%02x, status:%d\r\n", 
                    list_flash_power_off[i*7+5], list_flash_power_off[i*7+4], list_flash_power_off[i*7+3], 
                    list_flash_power_off[i*7+2], list_flash_power_off[i*7+1], list_flash_power_off[i*7], list_flash_power_off[i*7+6]);
			}
		}
#endif
	}
    else
    {
        //print_debug("no find powercut meter record.",0,0);
        //set_search_meter_delay5s_flag_fun(1);
    }
	//修复复电时, 二采有电电表都没电, 但电表复电后, 上报电表都不在线的问题. add, by lxl, 20211208@1600
	//只写不擦除
    //DataFlashInit();
	//EraseFlash(II_STA_POWEROFF_ADDR,DATA_FLASH_SECTION_SIZE);
	//DataFlashClose();
}

void check_meter_list_power_off(void)
{
    UINT32 list,list_temp;
    UINT8 i,j=0,addr_null[6]={0};    
    UINT8 buf_07[sizeof(ReadPower645_07)];

    list=SEARCH_METER_FLAG.TableList;
    list=list>>SEARCH_METER_FLAG.list_count; 
    if((((UINT8)list)&0x01) == 0x01 && check_send_count<6)
    {        
		struct strList_Meter* meter_list = (struct strList_Meter*)(&SEARCH_METER_FLAG.table[SEARCH_METER_FLAG.list_count*7]);//add by lxl, 20211028@1403
        if(meter_list->Type.Statute == 0x02)//07
        {
			memcpy(buf_07,ReadPower645_07,sizeof(ReadPower645_07));
			memcpy(&buf_07[5],&SEARCH_METER_FLAG.table[SEARCH_METER_FLAG.list_count*7],6);
			buf_07[sizeof(ReadPower645_07)-2]=GetCheckSum(buf_07+4,sizeof(ReadPower645_07)-4-2);
			AppLocalReceiveData( &buf_07[5], &buf_07[5], SEARCH_METER_1500MS, 0, buf_07, sizeof(ReadPower645_07), RS485DataDeal, 0, 0 ,0);
      
			SetWait485DataFlag(UART_CHECK_POWERCUT_FRAME); 
            check_send_count++;
        }
        SEARCH_METER_FLAG.list_count++;
        return;
    }
    else
    {
        
    }    
    if(check_send_count >= SEARCH_METER_FLAG.table_index||check_send_count>=6)//6个后停止 //改为check_send_count>=6
    {        
        traversal_list_count=0;
        SetCheckTableFlag(false);
        list_temp = SEARCH_METER_FLAG.TableList ^ SEARCH_METER_FLAG.back_List;

		//不在出厂测试模式时才进行掉电的检测
		if(!GetProduceModeFlag())		
			write_list_flash_power_off();
		
        for(i=0;i<SEARCH_METER_FLAG.table_index;i++)
        {                    
            if((((UINT8)list_temp)&0x01) == 0x00)   
                SEARCH_METER_FLAG.table[i*7+6] = 1;                               
            else
				SEARCH_METER_FLAG.table[i*7+6] = 0;
            list_temp=list_temp>>1; 
        }


		j = 0;
	    for(i=0;i<6;i++)
	    {
	        if(memcmp(&SEARCH_METER_FLAG.table[i*7],addr_null,6)!=0)
	        {
	            memcpy((uint8_t *)&poweroff_report_buff[j*7],&SEARCH_METER_FLAG.table[i*7],6);
	            poweroff_report_buff[6+j*7]=SEARCH_METER_FLAG.table[i*7+6];        
	            j++;
	        } 
	    }

#if !defined(PROTOCOL_NW_2021_GUANGDONG) && !defined(PROTOCOL_NW_2023_GUANGDONG_V2DOT1) //南网深化应用V1.1，广东深化应用V2.1都采用TEI位图上报   
		II_STA_PowerOffEventReport(poweroff_report_buff,j);
#endif			

        SetCheckPowerOffMeterEndFlag(true);    
		SEARCH_METER_FLAG.check_power_off_flag = 2;
        
#ifdef DEBUG_SEARCH_LOG_OUT
        debug_str( DEBUG_LOG_APP, "check_meter_list_power_off: %d\r\n", j);
		for(i=0;i<j;i++)
		{
			debug_str( DEBUG_LOG_APP, "  %02x%02x%02x%02x%02x%02x, status:%d\r\n", 
                poweroff_report_buff[i*7+5], poweroff_report_buff[i*7+4], poweroff_report_buff[i*7+3], 
                poweroff_report_buff[i*7+2], poweroff_report_buff[i*7+1], poweroff_report_buff[i*7], poweroff_report_buff[i*7+6]);
		}
#endif
    } 
}
void check_meter_list_power_on_var_init(void)
{	
	check_send_count=0;
	SEARCH_METER_FLAG.list_count=0;
	//SetCheckTableFlag(true);//在回调里增加新的条目, 不在使用check_table_flga的, changed by lxl,20211029@1155
	SEARCH_METER_FLAG.back_List=0x00;//停电前初始化, add, by lxl, 20211208@0900
#ifdef DEBUG_SEARCH_LOG_OUT
	debug_str(DEBUG_LOG_APP, "check_meter_list_power_on_var_init() Init ok\r\n");
#endif
}
void check_meter_list_power_on(void)
{
    UINT32 list,list_temp;
    UINT8 i;
    UINT8 buf_07[sizeof(ReadPower645_07)];

	
	list=SEARCH_METER_FLAG.TableList;
    list=list>>SEARCH_METER_FLAG.list_count; 
    if((((UINT8)list)&0x01) == 0x01 && SEARCH_METER_FLAG.list_count<6)
    {        
		struct strList_Meter* meter_list = (struct strList_Meter*)(&SEARCH_METER_FLAG.table[SEARCH_METER_FLAG.list_count*7]);//add by lxl, 20211028@1403
        if(meter_list->Type.Statute == 0x02)//07
		{
			memcpy(buf_07,ReadPower645_07,sizeof(ReadPower645_07));
			memcpy(&buf_07[5],&SEARCH_METER_FLAG.table[SEARCH_METER_FLAG.list_count*7],6);
			buf_07[sizeof(ReadPower645_07)-2]=GetCheckSum(buf_07+4,sizeof(ReadPower645_07)-4-2);
			AppLocalReceiveData( &buf_07[5], &buf_07[5], SEARCH_METER_1500MS, 0, buf_07, sizeof(ReadPower645_07), RS485DataDeal, 0, 0,0);

			SetWait485DataFlag(UART_CHECK_POWERCUT_FRAME);
			check_send_count++;
		}

        SEARCH_METER_FLAG.list_count++;
        return;
    }
    else
    {
       
    }
   
    if(check_send_count >= SEARCH_METER_FLAG.table_index)
    {        
        traversal_list_count=0;      
		u16 getPowerOnNum=0;  
        list_temp = SEARCH_METER_FLAG.TableList ^ SEARCH_METER_FLAG.back_List;                    
        for(i=0;i<SEARCH_METER_FLAG.table_index;i++)
        {                    
            if((((UINT8)list_temp)&0x01) == 0x00)        
                list_flash_power_off[i*7+6] = 1;                        
            else
				list_flash_power_off[i*7+6] = 0;
            list_temp=list_temp>>1; 
			getPowerOnNum++;
        }
		if (!getPowerOnNum)//没有电表数据 不上报复电事件
		{
			IsPowerOff=0;
			StoragePowerOffFlag();
		}
		SetCMLPowerOnFlag(false);
		SearchMeterStart();
		
#ifdef DEBUG_SEARCH_LOG_OUT
        list_temp = SEARCH_METER_FLAG.TableList ^ SEARCH_METER_FLAG.back_List;
        debug_str( DEBUG_LOG_APP, "check_meter_list_power_on: %d\r\n", getPowerOnNum);
		for(i=0;i<6;i++)
		{
			if(!memIsHex(&list_flash_power_off[i*7],0,6))
			{
				debug_str( DEBUG_LOG_APP, "  %02x%02x%02x%02x%02x%02x, status:%d\r\n", 
                    list_flash_power_off[i*7+5], list_flash_power_off[i*7+4], list_flash_power_off[i*7+3], 
                    list_flash_power_off[i*7+2], list_flash_power_off[i*7+1], list_flash_power_off[i*7], list_flash_power_off[i*7+6] );
			}
		}
#endif
    } 
}
//----------------------------------------------------------------
//-END-
//----------------------------------------------------------------
//add by LiHuaQiang 2020.10.9  -END-
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------


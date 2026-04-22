#include "protocol_includes.h"
#include "os.h"
#include "common_includes.h"
#include <string.h>
#include "bsp.h"
#include "Revision.h"
#include "app_search_meter.h"
#include "app_event_IIcai.h"
#include "timer.h"
#include "hplc_mac.h"
#include "app_RF_IIcai.h"
#include "plm_2005.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//add by LiHuaQiang 2020.10.8  -START-
//----------------------------------------------------------------
//声明
#if 0
extern bool IsBindAddr;
extern OS_TCB  App_II_STA_TCB;
extern int power;
u8 I_STA_SEQ=0;
//变量定义
SEARCH_METER_FLAG_STRUCT SEARCH_METER_FLAG;
//                 0   1    2   3    4   5   6    7  8    9   0   1   2    3   4    5    6   7    8    9
u8 ReadPower645_07[]={0xFE,0xFE,0xFE,0xFE,0x68,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0x68,0x11,0x04,0x33,0x33,0x34,0x33,0xAE,0x16};//u8 ReadPower645_07[]={0xFE,0xFE,0xFE,0xFE,0x68,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0x68,0x13,0x00,0xDF,0x16};//
u8 ReadPower_698[]={0xFE,0xFE,0xFE,0xFE,0x68,0x17,0x00,0x43,0x45,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0x00,0x5B,0x4F,0x05,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x10,0x31,0x16};
u8 ReadPower645_97[]={0xFE,0xFE,0xFE,0xFE,0x68,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0x68,0x01,0x02,0x43,0xC3,0xD5,0x16};

//#define DEBUG_LOG_SEARCH

STACK S;

struct CONFLICT_TABLE conflict_table;
struct CONFLICT_LEVEL1 conflict_level0;
struct CONFLICT_LEVEL1 conflict_level1;
struct CONFLICT_LEVEL2 conflict_level2;
struct CONFLICT_LEVEL3 conflict_level3;
struct CONFLICT_LEVEL4 conflict_level4;
struct CONFLICT_LEVEL5 conflict_level5;
struct CONFLICT_LEVEL6 conflict_level6;
struct CONFLICT_LEVEL7 conflict_level7;
struct CONFLICT_LEVEL8 conflict_level8;
struct CONFLICT_LEVEL9 conflict_level9;
struct CONFLICT_LEVEL10 conflict_level10;
struct CONFLICT_LEVEL11 conflict_level11;

TABLE_COPY t_table_copy;
TABLE_COPY t_no_exist_meter;

struct strList_Meter meter_list;

//----------------------------------------------------------------
//变量设置与查询操作
void SEARCH_METER_FLAG_init(void)
{
	SEARCH_METER_FLAG.check_meter_list_power_on_flag = true;
	SEARCH_METER_FLAG.check_power_off_flag           = 0;
	SEARCH_METER_FLAG.firt_check_table_flag          = true;
	SEARCH_METER_FLAG.check_meter_type_end_flag      = true;
	SEARCH_METER_FLAG.search_one_meter_flag          = false;
	SEARCH_METER_FLAG.wait_485data_flag              = false;

}
void SetCMLPowerOnFlag(bool data)
{
	SEARCH_METER_FLAG.check_meter_list_power_on_flag = data;
}
bool GetCMLPowerOnFlag(void)
{
	return SEARCH_METER_FLAG.check_meter_list_power_on_flag;
}
void SetCheckTableFlag(bool data)
{
	SEARCH_METER_FLAG.check_table_flag = data;
	if (data==false)
	{
		OS_ERR err;
		SEARCH_METER_FLAG.end_search_tick=OSTimeGet(&err);
	}
}
bool GetCheckTableFlag(void)
{
	return SEARCH_METER_FLAG.check_table_flag;
}
void SetSendAAFlag(bool data)
{
	SEARCH_METER_FLAG.send_AA_flag = data;
}
bool GetSendAAFlag(void)
{
	return SEARCH_METER_FLAG.send_AA_flag;
}
void SetSerach645OverFlag(bool data)
{
	SEARCH_METER_FLAG.search645_over_flag = data;
}
bool GetSerach645OverFlag(void)
{
	return SEARCH_METER_FLAG.search645_over_flag;
}
void SetSerach698OverFlag(bool data)
{
	SEARCH_METER_FLAG.search698_over_flag = data;
}
bool GetSerach698OverFlag(void)
{
	return SEARCH_METER_FLAG.search698_over_flag;
}
void SetSerach64597OverFlag(bool data)
{
	SEARCH_METER_FLAG.search645_97_over_flag = data;
}
bool GetSerach64597OverFlag(void)
{
	return SEARCH_METER_FLAG.search645_97_over_flag;
}
void SetWait485DataFlag(u8 data)
{
	SEARCH_METER_FLAG.wait_485data_flag = data;
}
u8 GetWait485DataFlag(void)
{
	return SEARCH_METER_FLAG.wait_485data_flag;
}
void SetFirstCheckTableFlag(bool data)
{
	SEARCH_METER_FLAG.firt_check_table_flag = data;
}
bool GetFirstCheckTableFlag(void)
{
	return SEARCH_METER_FLAG.firt_check_table_flag;
}
void SetCheckMeterEndFlag(bool data)
{
	SEARCH_METER_FLAG.check_meter_type_end_flag = data;
}
bool GetCheckMeterEndFlag(void)
{
	return SEARCH_METER_FLAG.check_meter_type_end_flag;
}
void SetSearchOneMeterFlag(bool data)
{
	SEARCH_METER_FLAG.search_one_meter_flag= data;
}
bool GetSearchOneMeterFlag(void)
{
	return SEARCH_METER_FLAG.search_one_meter_flag;
}
void SetRS485ConflictFlag(bool data)
{
	SEARCH_METER_FLAG.rs485_conflict_flag= data;
}
bool GetRS485ConflictFlag(void)
{
	return SEARCH_METER_FLAG.rs485_conflict_flag;
}

//----------------------------------------------------------------
//存储搜表时冲突的地址值相关操作
bool StackEmpty(STACK S)
{
    if(S.top == -1)
        return true;
    else
        return false;
}
void InitStack(STACK *s)
{
    s->top=-1;
}

void StackPush(STACK *s,StackEntry item)
{
    if(s->top == MAX_STACK -1)
        return;
    else
        s->item[++s->top]=item;
}

void StackPop(STACK *s,StackEntry *item)
{
    if(StackEmpty(*s))
        return;
    else
        *item=s->item[s->top--];
}

void GetTop(STACK s,StackEntry *item)
{
    if(StackEmpty(s))
        return;
    else
        *item=s.item[s.top];
}
//从data(每存一个字节数据，自增1)指向的数据取出nbytes字节数据压入堆栈S,后清除原始数据
void push_nbytes_to_S_and_clear(uint8_t *data,uint8_t nbytes)
{
    uint8_t i;   
    
    InitStack(&S);
    
	for(i=0;i<nbytes;i++)
	{
        	StackPush(&S,data[i]);//压栈
        	data[i]=0;//清0
	}
}
//从data(每存一个字节数据，自增1)指向的数据取出nbytes字节数据压入堆栈S
void push_nbytes_to_S(uint8_t *data,uint8_t nbytes)
{
    uint8_t i;    									
	for(i=0;i<nbytes;i++)
	{
        StackPush(&S,data[i]);//压栈        
	}
}
//从 &data[nbytes-1] (每存一个字节数据，自减1)指向的数据取出nbytes字节数据压入堆栈S
void push_nbytes_to_S_dec(uint8_t *data,uint8_t nbytes)
{
    int8_t i;
    if(nbytes<1)return;    								
	for(i=(nbytes-1);i>=0;i--)
	{
        StackPush(&S,data[i]);//压栈
        data[i]=0;//清0
	}
}
//从堆栈S取nbytes字节数据存入 data (每取一个字节数据，自增1)指向的内存地址
void pop_nbytes_from_S(uint8_t *data,uint8_t nbytes)
{
    uint8_t i;
    for(i=0;i<nbytes;i++)
        StackPop(&S,&data[i]);
}
//从堆栈S取nbytes字节数据存入 &data[nbytes-1] (每取一个字节数据，自减1)指向的内存地址
void pop_nbytes_from_S_dec(uint8_t *data,uint8_t nbytes)
{
    int8_t i;
    if(nbytes<1)return; 
    for(i=(nbytes-1);i>=0;i--)
    {
        StackPop(&S,&data[i]);        
    }
}
//----------------------------------------------------------------
// 645&698帧处理:采集器转发载波数据后，接收表端数据处理
void protocol645_698_dispose(struct frame645 *pframe,u16 receive_len)
{
    if(GetWait485DataFlag() == UART_IFR_DATA)//采集器转发红外数据后，接收表端数据处理
    {          
        //红外发送数据
        AnswerReadMeter2IFR((UINT8 *)&pframe->frame_start_flag1,receive_len);   
        return;
    }
    else if(GetWait485DataFlag() == UART_EVENT_DATA)
    {
        rs485_recv_event_data_deal_fun(pframe,receive_len);
        return;
    }   
    else if(GetWait485DataFlag() == UART_CHECK_NO_EXIST_METER_FRAME)
    {
	    t_CHECK_NO_EXIST_METER_set_fun();
	    return;
    }
	//停电复电
    else if(GetWait485DataFlag() == UART_CHECK_POWERCUT_FRAME)
    {
		if(SEARCH_METER_FLAG.list_count >0)
		{
			if(memcmp((UINT8 *)&SEARCH_METER_FLAG.table[(SEARCH_METER_FLAG.list_count-1)*7],(UINT8 *)&pframe->addr[0],6) == 0)
			{
				SEARCH_METER_FLAG.back_List |= (1<<(SEARCH_METER_FLAG.list_count-1));
			} 
		}
		//用不到, 所以注释掉. changed, by lxl, 20211208@0900
		//if((SEARCH_METER_FLAG.list_count ==0)&&(SEARCH_METER_FLAG.meter_total==1))
		//{
		//	SEARCH_METER_FLAG.back_List=0x01;
		//}
		return;
    }
    if(!Is645Frame((u8*)pframe,receive_len))
		return;
    //采集器主动寻址，返回指令处理 //对645报文的判断    
    switch(pframe->control_code.control_byte)
    {    
		case READ_METER_ACK:
        case READ_ACK_07:              
			if(GetCheckTableFlag())// 用于检测485列表是否变更
			{
				//当前程序back_List只用于停复电, 这段程序没用, 所以注释掉. changed, by lxl, 20211208@0900
				//if(SEARCH_METER_FLAG.list_count >0)
				//{
				//	if(memcmp((UINT8 *)&SEARCH_METER_FLAG.table[(SEARCH_METER_FLAG.list_count-1)*7],(UINT8 *)&pframe->addr[0],6) == 0)
				//	{
				//		SEARCH_METER_FLAG.back_List |= (1<<(SEARCH_METER_FLAG.list_count-1));                        
				//	} 
				//
				//}
				//if((SEARCH_METER_FLAG.list_count ==0)&&(SEARCH_METER_FLAG.meter_total==1))
				//{
				//	SEARCH_METER_FLAG.back_List=0x01;
				//}
				if(!GetCMLPowerOnFlag())
				{	                           
					write_meter_list(&pframe->addr[0],PRO_645_07);  
				}
			}
            else if(!GetCheckMeterEndFlag())
            {
                if(memcmp(pframe->data,DI_event_statue,4)==0) 
                {
                    meter_list.Type.meter_type1=1;
                    meter_list.Type.meter_type2=0;
                    //print_debug("13meter addr:",pframe->addr,6);
                }
                else if(memcmp(pframe->data,DI_phaseB_volte,4)==0)
                {
                    meter_list.Type.meter_type2=1;
                    //print_debug("09 sanxiang meter addr:",pframe->addr,6);
                }
				if(!GetCMLPowerOnFlag())
				{	                
	    		    write_meter_list2(&pframe->addr[0],PRO_645_07);
				}
            }
            break;
		case READ_ACK_07_B1:
            if(GetCheckTableFlag())// 用于检测白名单是否变更
            {
				//当前程序back_List只用于停复电, 这段程序没用, 所以注释掉. changed, by lxl, 20211208@0900
				//if(SEARCH_METER_FLAG.list_count >0)
				//{
				//	if(memcmp((UINT8 *)&SEARCH_METER_FLAG.table[(SEARCH_METER_FLAG.list_count-1)*7],(UINT8 *)&pframe->addr[0],6) == 0)
				//	{
				//		SEARCH_METER_FLAG.back_List |= (1<<(SEARCH_METER_FLAG.list_count-1));
				//	} 
				//}
				//if((SEARCH_METER_FLAG.list_count ==0)&&(SEARCH_METER_FLAG.meter_total==1))
				//{
				//	SEARCH_METER_FLAG.back_List=0x01;
				//}
				if(!GetCMLPowerOnFlag())
				{             
	                write_meter_list(&pframe->addr[0],PRO_645_07);
				}
                
            }      
            break;      
#ifdef II_STA_SUPPORT_DLT645_97
		case READ_ACK_97:              
			if(GetCheckTableFlag())
			{
				if(!GetCMLPowerOnFlag())
				{	                           
					write_meter_list(&pframe->addr[0], PRO_645_97);  
				}
			}
            break;
#endif
        default:
            break;
    }
}
#ifdef II_STA

//sizeof(baudTable_t)/sizeof(u32)
//>>2
//数组的值与要与struct strList_Meter对应 
//u8 BaudType:4;  // 波特率类型
//  0x01:1200bps
//  0x02:2400bps
//  0x03:4800bps
//  0x04:9600bps
//  0x05:19200

#ifdef II_STA_NO_9600_BAUD
const u32 baudTable_t[BAUDTABLE_T_SIZE]={1200,2400,4800};
#else
const u32 baudTable_t[BAUDTABLE_T_SIZE]={1200,2400,4800,9600};
#endif

u32 CurrentBaud = 0;//当前波特率
extern void ChangeLocalUartBaud_t(u32 baud);
extern u32 GetLoacalBaud( void );

//能在 SEARCH_METER_FLAG.table[] 找到
u32 FindBaudInSEARCH_METER_FLAG_table( u8 dstAddr[6] )
{
	u8 i = 0;
	u8 find = 0;
	u32 baud_t = 0;
	u32 table_list_tmp = SEARCH_METER_FLAG.TableList;

	for ( i=0; i < 32; i++ )
	{
        if((table_list_tmp&0x00000001)==0x00000001)
        {
	        if( 0 == memcmp( (u8*)(dstAddr), &SEARCH_METER_FLAG.table[i*7], 6) )
			{
				struct strList_Meter* meter_list = (struct strList_Meter*)(&SEARCH_METER_FLAG.table[i*7]);
				//找到
				find = 1;
				//取类型
				//meter_list->Type.BaudType
				//从类型获得波特率
				baud_t = GetBaudFromType( meter_list->Type.BaudType );
				break;
			}
	    }
        table_list_tmp = table_list_tmp >> 1;
	}

	//在SEARCH_METER_FLAG找不到, 再到t_table_copy里找
	if ( find == 0 )
	{
		table_list_tmp = t_table_copy.TableList;

		for ( i=0; i < 32; i++ )
		{
			if((table_list_tmp&0x00000001)==0x00000001)
			{
				if( 0 == memcmp( (u8*)(dstAddr), &t_table_copy.table[i*7], 6) )
				{
					struct strList_Meter* meter_list = (struct strList_Meter*)(&t_table_copy.table[i*7]);
					//找到
					find = 1;
					//取类型
					//meter_list->Type.BaudType
					//从类型获得波特率
					baud_t = GetBaudFromType( meter_list->Type.BaudType );
					break;
				}
			}
			table_list_tmp = table_list_tmp >> 1;
		}
	}

	if ( find != 0 )
	{
		return baud_t;
	}
	else
	{
		//找不到就返回默认波特率
		return baudTable_t[1];
	}
}

bool ChangeBaudInSEARCH_METER_FLAG_table(u8 dstAddr[6], u8 baudType)
{
	u8 i = 0;
	u8 find = 0;
	u32 table_list_tmp = SEARCH_METER_FLAG.TableList;

	for (i=0; i<32; i++)
	{
        if((table_list_tmp&0x00000001)==0x00000001)
        {
	        if( 0 == memcmp( (u8*)(dstAddr), &SEARCH_METER_FLAG.table[i*7], 6) )
			{
				struct strList_Meter* meter_list = (struct strList_Meter*)(&SEARCH_METER_FLAG.table[i*7]);
				//找到
				find = 1;

				meter_list->Type.BaudType = baudType;

				return true;
			}
	    }
        table_list_tmp = table_list_tmp >> 1;
	}

	//在SEARCH_METER_FLAG找不到, 再到t_table_copy里找
	if ( find == 0 )
	{
		table_list_tmp = t_table_copy.TableList;

		for ( i=0; i < 32; i++ )
		{
			if((table_list_tmp&0x00000001)==0x00000001)
			{
				if( 0 == memcmp( (u8*)(dstAddr), &t_table_copy.table[i*7], 6) )
				{
					struct strList_Meter* meter_list = (struct strList_Meter*)(&t_table_copy.table[i*7]);
					//找到
					find = 1;

					meter_list->Type.BaudType = baudType;
					
					return true;
				}
			}
			table_list_tmp = table_list_tmp >> 1;
		}
	}

	return false;
}

//检查波特率是否在baudTable_t里
//  1: 在
//  0: 不在
u8 BaudValidityCheck( u32 CurrentBaud_t )
{
	u8 i = 0;
	u8 Validity = 0;
	for ( i=0; i<BAUDTABLE_T_SIZE; i++ )
	{
		if ( CurrentBaud_t == baudTable_t[i] )
		{
			Validity = 1;
			break;
		}
	}

	return Validity;
}

u32 GetBaudFromType( u8 BaudType_t )
{
	u32 CurrentBaud_t = 0;

	if( (BaudType_t!=0) && ( BaudType_t <= BAUDTABLE_T_SIZE ) )
	{
		CurrentBaud_t = baudTable_t[BaudType_t-1];
	}
	else
	{
		CurrentBaud_t = baudTable_t[1];
	}

	return CurrentBaud_t;
}
u8 GetBaudType( u32 CurrentBaud_t )
{
	u8 BaudType_t = 0;
	switch( CurrentBaud_t )
	{
	case 1200:
		BaudType_t = 1;
		break;
	case 2400:
		BaudType_t = 2;
		break;
		break;
	case 4800:
		BaudType_t = 3;
		break;
	case 9600:
		BaudType_t = 4;
		break;

	default:
		BaudType_t = 0;
		break;
	}
		
	return BaudType_t;
}

bool GetNextBaudType(u32 CurrentBaud_t, u8* NextBaudType)
{	
	switch(CurrentBaud_t)
	{
		case 1200:
			*NextBaudType = 2;
			break;
		case 2400:
			*NextBaudType = 3;
			break;
			break;
		case 4800:
			*NextBaudType = 4;
			break;
		case 9600:
			*NextBaudType = 1;
			break;

		default:
			return false;
	}
		
	return true;
}

#endif
//----------------------------------------------------------------
/*写索引表*/
void write_meter_list(u8 *addr,u8 protocol_type)
{
    u8 i=0,j;
    u8 exist_flag=0;
    u32 list;
    u8 flag=0;
    u8 total=0;//20171120

    memcpy(meter_list.address,addr,6);
    meter_list.Type.BaudType=GetBaudType( GetLoacalBaud() );//meter_list.Type.BaudType=0x02;//
    meter_list.Type.Statute=protocol_type;

    //如果存在缩位寻址0xAA，则不记录
   	for(j=0;j<6;j++)
	{
		if(addr[j]==0xaa)
		{
			flag++;
		}
	}	
	if(flag!=0)
	{
		return;
	}
    //-----------------------------  
    total = SEARCH_METER_FLAG.meter_total;
    for(i=0;i<total;i++)
    {    	
        if(!memIsHex(&SEARCH_METER_FLAG.table[i*7],0,6))//非0x00 说明被写入参数
        {                
            if(memcmp(meter_list.address,&SEARCH_METER_FLAG.table[i*7],6)==0)
            {
                exist_flag=1;//该地址已经存在，直接跳出                
                break;
            }                
            else
                exist_flag=0;
        }
        else
        {}
    }
    
    list=SEARCH_METER_FLAG.TableList;
    if(exist_flag == 0)
    {
        for(i=0;i<32;i++)
        {                
            if((((u8)list)&0x01) == 0x00)//查找任意一个没有写入的空间，把数据写入
            {
                memcpy((u8 *)&SEARCH_METER_FLAG.table[i*7],meter_list.address,6);
                memcpy((u8 *)&SEARCH_METER_FLAG.table[i*7+6],(u8 *)&meter_list.Type,1);
                
                SEARCH_METER_FLAG.TableList |= (1<<i);//已经有数据，置1
                SEARCH_METER_FLAG.meter_total++;// 电表总数累计
                SEARCH_METER_FLAG.table_index++;//               
                break;
            }
            list=list>>1;                    
        }
    }    
}

/*写索引表*/
//写485列表时，如果存在同地址的表，需要同时更新波特率和规约类型
void write_meter_list2(u8 *addr,u8 protocol_type)
{
    u8 i=0,j;
    u8 exist_flag=0;    
    u32 list;
	u8 flag=0;
	u8 total=0;

    memcpy(meter_list.address,addr,6);
    meter_list.Type.BaudType=GetBaudType( GetLoacalBaud() );//meter_list.Type.BaudType=0x02;
    meter_list.Type.Statute=protocol_type;       
    
    //如果存在缩位寻址0xAA，则不记录
   	for(j=0;j<6;j++)
	{
		if(addr[j]==0xaa)
		{
			flag++;
		}
	}	
	if(flag!=0)
	{
		return;
	}
    //------------------------------
    total = SEARCH_METER_FLAG.meter_total;
    for(i=0;i<total;i++)
    {    		   
        if(!memIsHex(&SEARCH_METER_FLAG.table[i*7],0x0,6))//非0x00 说明被写入参数
        {            
            if(memcmp(meter_list.address,&SEARCH_METER_FLAG.table[i*7],6)==0)
            {
                exist_flag=1;//该地址已经存在，直接跳出                    
                break;
            }                
            else
                exist_flag=0;
        }
        else
        {}
    } 
    
    list=SEARCH_METER_FLAG.TableList;
    if(exist_flag == 0)
    {
        for(i=0;i<32;i++)
        {                
            if((((u8)list)&0x01) == 0x00)//查找任意一个没有写入的空间，把数据写入
            {
                memcpy((u8 *)&SEARCH_METER_FLAG.table[i*7],meter_list.address,6);
                memcpy((u8 *)&SEARCH_METER_FLAG.table[i*7+6],(u8 *)&meter_list.Type,1);
                
                SEARCH_METER_FLAG.TableList |= (1<<i);//已经有数据，置1
                SEARCH_METER_FLAG.meter_total++;// 电表总数累计
                SEARCH_METER_FLAG.table_index++;                
                break;
            }
            list=list>>1;                    
        }
    }   
    else
    {
        memcpy((UINT8 *)&SEARCH_METER_FLAG.table[i*7+6],(u8 *)&meter_list.Type,1);        
    }
}
u8 ResetFlag = 0;//修复复电时, 二采有电电表都没电, 但电表复电后, 上报电表都不在线的问题. add, by lxl, 20211208@1600
//----------------------------------------------------------------
//RS485口接收回调
void RS485DataDeal(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8* data, u16 len, u8 sendType, u16 srcTei)
{
	u8 addr_temp[6];
	u16 index = 0;
	if(Is645Frame(data,len))
	{
		for (; index < len; index++)
		{
			if (data[index] == 0x68) break;
		}
		if(!IsBindAddr)
		{
			//修复复电时, 二采有电电表都没电, 但电表复电后, 上报电表都不在线的问题. add, by lxl, 20211208@1600
			if( (ResetFlag==2) && (SEARCH_METER_FLAG.meter_total==0) )
			{
				OS_ERR err;
				u32 Tick = OSTimeGet(&err);
				//30秒内复搜到, 重启. 30秒外的, 就不是电表启动慢的问题了
				if( Tick < 30*1000 )
					RebootSystem();
				else
					ResetFlag = 0;
			}
			
			//地址颠倒
			memcpy(addr_temp,&data[index + 1],6);
			TransposeAddr(addr_temp);
			SetStaMAC(addr_temp);
			memcpy(SEARCH_METER_FLAG.online_addr,addr_temp,6);
			IsBindAddr = true;
		}
		protocol645_698_dispose((struct frame645 *)&data[index],len-index);

#ifdef DEBUG_LOG_SEARCH
		debug_str(DEBUG_LOG_APP, "RS485DataDeal\r\n");
		debug_hex(DEBUG_LOG_NET, data, len);
#endif

		if (conflict_table.conflict_level == SEARCH_LEVEL0)//二采搜表收到正确帧也记录冲突
		{
			SetRS485ConflictFlag(false);
			conflict_val_record();
		}
	}
	else
	{
		
		if(len != 0)//确定485口接收数据有冲突
		{
			SetRS485ConflictFlag(false);
			if((SEARCH_METER_FLAG.search645_over_flag)&&(SEARCH_METER_FLAG.send_AA_flag))//缩位寻址时
			{
				conflict_val_record();
				debug_str(DEBUG_LOG_APP, "find conflict: conflict_level=%d, sub_count=%d\r\n", conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count);
#ifdef DEBUG_LOG_SEARCH		
				debug_hex(DEBUG_LOG_NET, data, len);
#endif
			}
		}
		else//不确定485口接收数据是否有冲突
			SetRS485ConflictFlag(true);//开启底层检查校验
	}
	
	SetWait485DataFlag(false);
}
u8 getUartRate=0;
#ifdef I_STA
static void closeStaPin(void *arg)
{
	StaWrite(0);
}
#endif
void RS485SearchBack(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8* data, u16 len, u8 sendType, u16 srcTei)
{
	if (vaild)
	{
		getUartRate = 1;
		LMP_APP_INFO frame = PLM_process_frame((P_LMP_LINK_HEAD)data);
		if (frame.cur_afn = 0x21 && frame.curDT == 0xEA032102) //表地址
		{
			u8 total = frame.curProcUnit->data_unit[0]; //个数
			u8 *pmac = &frame.curProcUnit->data_unit[1];
			for (int i = 0; i < total; i++)
			{
				write_meter_list(pmac, PRO_645_07);
				if (!IsBindAddr)
				{
					TransposeAddr(pmac);
					SetMACType(1);
					SetStaMAC(pmac);
					IsBindAddr = true;
#ifdef I_STA
					StaWrite(1);
					HPLC_RegisterTmr(closeStaPin, NULL, 200, TMR_OPT_CALL_ONCE);
#endif					
				}
				pmac += 6;
			}
			SetCheckTableFlag(false); //关闭搜表
		}

	}
	else
	{
		if(!IsBindAddr)
		{
			ResetChangeFreTimer();
		}
	}
	SetWait485DataFlag(false);
}
//----------------------------------------------------------------
//搜表相关操作
void ConflictVariableInit(void)
{
	InitStack(&S);
	conflict_table.conflict_level = SEARCH_LEVEL0;//memset((u8 *)&conflict_table.conflict_level,0x00,sizeof(conflict_table));//
	memset((u8 *)&conflict_level0.conflict_count,0x00,sizeof(conflict_level0)); memset((u8 *)&conflict_level1.conflict_count,0x00,sizeof(conflict_level1));
	memset((u8 *)&conflict_level2.conflict_count,0x00,sizeof(conflict_level2));
	memset((u8 *)&conflict_level3.conflict_count,0x00,sizeof(conflict_level3));
	memset((u8 *)&conflict_level4.conflict_count,0x00,sizeof(conflict_level4));
	memset((u8 *)&conflict_level5.conflict_count,0x00,sizeof(conflict_level5));
	memset((u8 *)&conflict_level6.conflict_count,0x00,sizeof(conflict_level6));
    memset((u8 *)&conflict_level7.conflict_count,0x00,sizeof(conflict_level7));
    memset((u8 *)&conflict_level8.conflict_count,0x00,sizeof(conflict_level8));
	memset((u8 *)&conflict_level9.conflict_count,0x00,sizeof(conflict_level9));
	memset((u8 *)&conflict_level10.conflict_count,0x00,sizeof(conflict_level10));
	memset((u8 *)&conflict_level11.conflict_count,0x00,sizeof(conflict_level11));	
}
void SearchMeterStart(void)//启动搜表
{
	SEARCH_METER_FLAG.check_table_flag               = true;
#ifdef II_STA
	//跳过默认波特率
	SEARCH_METER_FLAG.send_AA_flag                   = true;
	SEARCH_METER_FLAG.search645_over_flag            = true;
#else
	SEARCH_METER_FLAG.send_AA_flag                   = false;
	SEARCH_METER_FLAG.search645_over_flag            = false;
#endif
	SEARCH_METER_FLAG.search698_over_flag            = false;	
	SEARCH_METER_FLAG.search645_97_over_flag          = false;
	SEARCH_METER_FLAG.sub_count                      = false;
	SEARCH_METER_FLAG.list_count                     = false;
	memset(SEARCH_METER_FLAG.table,0,TABLE_MAX_SIZE);
	SEARCH_METER_FLAG.table_index                    = false; 	
	SEARCH_METER_FLAG.meter_total                    = false;
	SEARCH_METER_FLAG.TableList                      = false; 	
	SEARCH_METER_FLAG.back_List                      = false;
	SEARCH_METER_FLAG.search_times                   = false;	
	SEARCH_METER_FLAG.sub_count_99_twice             = false;	
	SEARCH_METER_FLAG.rs485_conflict_flag            = false;
	ConflictVariableInit();
    
#ifdef DEBUG_SEARCH_LOG_OUT
    debug_str(DEBUG_LOG_APP, "SearchMeterStart() ok\r\n");
#endif
}
void TableCopyInit(void)
{
    memset(t_table_copy.table,0,224);
    t_table_copy.TableList=0;
    t_table_copy.table_index=0;
    t_table_copy.meter_total=0;
}
void t_no_exist_meter_init_fun(void)
{
	memset(t_no_exist_meter.table,0,224);
    t_no_exist_meter.TableList=0;
    t_no_exist_meter.table_index=0;
    t_no_exist_meter.meter_total=0;
}
/****************************************************************
*功能     :  找出定时搜表未搜到已经存在485列表内的电表
*返回值: TRUE-找到  FALSE-未找到
*****************************************************************/
u8 find_no_exist_meter_fun(void)
{
	u8 i,j,k=0;
	for(i=0;i<t_table_copy.meter_total;i++)
	{		
		for(j=0;j<SEARCH_METER_FLAG.meter_total;j++)
		{
			if(memcmp(&t_table_copy.table[i*7],&SEARCH_METER_FLAG.table[j*7],6) != 0)
			{
				//print_debug("diffrent addr: ",&table[j*7],6);							
			}
			else
			{
				//print_debug("same addr: ",&table[j*7],6);
				break;
			}			
		}
		if(j == SEARCH_METER_FLAG.meter_total)
		{				
			memcpy(&t_no_exist_meter.table[k*7],&t_table_copy.table[i*7],7);
						
			//print_debug("find no_exist_meter: ",&t_no_exist_meter.table[k*7],7);
						
			k++;
		}
	}
	if(k == 0)//上一次搜的表这次都能搜到
	{
		//print_debug("not find no_exist_meter",0,0);
		if(SEARCH_METER_FLAG.meter_total > t_table_copy.meter_total)//如果有多余的表
			report_meterlist_flag_set_fun(true);//启动上报CCO 
		return FALSE;
	}
	else
	{
		t_no_exist_meter.meter_total = k;
		t_CHECK_NO_EXIST_METER_init_fun();
		return TRUE;
	}
}
void table_copy_fun(void)
{
    if(GetFirstCheckTableFlag())
	{
        //print_debug("first search meter end.",0,0);
        SetFirstCheckTableFlag(false);
		//g_timer_2h_counter=ON;
	}
    
    if(t_table_copy.TableList != 0)//上次搜到表个数 > 0
    {
    	//print_debug("t_table_copy.table: ",t_table_copy.table,224);
		//print_debug("table: ",table,224);
		//print_debug("t_table_copy.meter_total: ",&t_table_copy.meter_total,1);
		//print_debug("meter_total: ",&meter_total,1);
		if(find_no_exist_meter_fun() == TRUE)
				return;	
		delete_wrong_meter();
		SetCheckMeterEndFlag(false);//可以开启确认表类型了        
    }
    else//上次搜到表个数为0 或者 这次是第一次搜表结束
    {
        if(GetSearchOneMeterFlag() == TRUE && SEARCH_METER_FLAG.meter_total == 1)//搜表结束，仍然只是搜到1块电表，则不再上报
        {
            //print_debug("SEARCH_METER_FLAG.search_one_meter_flag == TRUE",0,0);
        }
        else
            report_meterlist_flag_set_fun(true); 
		delete_wrong_meter();
		SetCheckMeterEndFlag(false);//可以开启确认表类型了
    }
	
    t_table_copy_fun();
       
}
void t_table_copy_fun(void)
{
	memcpy(t_table_copy.table,SEARCH_METER_FLAG.table,224);
    t_table_copy.TableList=SEARCH_METER_FLAG.TableList;
    t_table_copy.table_index=SEARCH_METER_FLAG.table_index;
    t_table_copy.meter_total=SEARCH_METER_FLAG.meter_total;
    //print_debug("t_table copy",0,0); 
}
void table_copy_fun_reverse(void)
{
    memcpy(SEARCH_METER_FLAG.table,t_table_copy.table,224);
    SEARCH_METER_FLAG.TableList=t_table_copy.TableList;
    SEARCH_METER_FLAG.table_index=t_table_copy.table_index;
    SEARCH_METER_FLAG.meter_total=t_table_copy.meter_total;
    //print_debug("table copy",0,0);    
}
void CycleSendAA645(void)
{
	static u8 send_AA_count = 0;
	send_AA_count++;
	switch(send_AA_count)
    {
        case 1:    
			{
			u8 addr[6] = {0};
			AppLocalReceiveData( addr, addr, SEARCH_METER_1500MS, 0, ReadPower645_07, sizeof(ReadPower645_07), RS485DataDeal, 0, 0, BAUD_SEARCH );
//			AppLocalReceiveData(SEARCH_METER_1500MS,
//				0,
//				0,
//				0,
//				0,
//				0,
//				0,
//				0, 
//				ReadPower645_07, 
//				sizeof(ReadPower645_07), 
//				RS485DataDeal,
//				0,
//				0);
			SetWait485DataFlag(UART_SCAN_485);
            break;               
			}
		case 2:			
            SetSendAAFlag(true);
			send_AA_count = 0;
#ifdef II_STA_ONLY645
			SetSerach645OverFlag(true);
#endif
			break;            
        default:
            send_AA_count = 0;
            break;
    }  
}
void CycleSendAA698(void)
{
	static u8 send_AA_count = 0;
	send_AA_count++;
	switch(send_AA_count)
    {
        case 1:
			{
			u8 addr[6] = {0};
			AppLocalReceiveData( addr, addr, SEARCH_METER_1500MS, 0, ReadPower_698, sizeof(ReadPower_698), RS485DataDeal, 0, 0, BAUD_SEARCH );
//			AppLocalReceiveData(SEARCH_METER_1500MS,
//				0,
//				0,
//				0,
//				0,
//				0,
//				0,
//				0, 
//				ReadPower_698, 
//				sizeof(ReadPower_698), 
//				RS485DataDeal,
//				0,
//				0);
			SetWait485DataFlag(UART_SCAN_485);     
            break;                      
			}
        case 2: 
			SetSerach645OverFlag(true); 
			send_AA_count = 0;
            if(SEARCH_METER_FLAG.meter_total == 1 && GetFirstCheckTableFlag() == true)//上电全A搜到表，表示只接了1个电表
            {                
                report_meterlist_flag_set_fun(true);//立即启动上报
                SetSearchOneMeterFlag(true);
            } 
            else
                SetSearchOneMeterFlag(false);
            //run_led_light();             
            break;
		default:
            send_AA_count = 0;
			break;
    } 
}
void MakeFrame645Send2RS485( u32 baud_t )
{       
    u8 count[6];
	static u8 buf[sizeof(ReadPower645_07)];
    
    if(SEARCH_METER_FLAG.sub_count !=0)
    {
		//if(SEARCH_METER_FLAG.search_times == 0)//因为第一个和最后一个的search_times不准, 所以用search_times判断
		{
		    memcpy(buf,ReadPower645_07,sizeof(ReadPower645_07));
	    	switch(conflict_table.conflict_level)
		    {
		        case SEARCH_LEVEL0:
#ifdef DEBUG_LOG_SEARCH
					debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645 level(0)\r\n");
#endif
		            break;
		        case SEARCH_LEVEL1:	            
		            buf[5]=Hex2BcdChar(SEARCH_METER_FLAG.sub_count-1);
		            buf[sizeof(ReadPower645_07)-2]=GetCheckSum(buf+4,sizeof(ReadPower645_07)-4-2);
#ifdef DEBUG_LOG_SEARCH				
					debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645 level(1)\r\n");
					debug_hex(DEBUG_LOG_NET, buf, sizeof(ReadPower645_07));
#endif
					break;
		        case SEARCH_LEVEL2:
	                pop_nbytes_from_S(&count[0],1);//获取第一级别搜表时的冲突地址   
		            buf[5]=Hex2BcdChar(count[0]);
		            buf[6]=Hex2BcdChar(SEARCH_METER_FLAG.sub_count-1);
		            buf[sizeof(ReadPower645_07)-2]=GetCheckSum(buf+4,sizeof(ReadPower645_07)-4-2);
		            push_nbytes_to_S_dec(&count[0],1);//将冲突地址压回堆栈	  
#ifdef DEBUG_LOG_SEARCH		            
		            debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645 level(2)\r\n");
					debug_hex(DEBUG_LOG_NET, buf, sizeof(ReadPower645_07));
#endif					
		            break;
		        case SEARCH_LEVEL3:
		            pop_nbytes_from_S(&count[0],2);
		            buf[5]=Hex2BcdChar(count[1]);  
		            buf[6]=Hex2BcdChar(count[0]);
		            buf[7]=Hex2BcdChar(SEARCH_METER_FLAG.sub_count-1);
		            buf[sizeof(ReadPower645_07)-2]=GetCheckSum(buf+4,sizeof(ReadPower645_07)-4-2);     
		            push_nbytes_to_S_dec(&count[0],2);
#ifdef DEBUG_LOG_SEARCH					
					debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645 level(3)\r\n");
					debug_hex(DEBUG_LOG_NET, buf, sizeof(ReadPower645_07));
#endif					
		            break;
		        case SEARCH_LEVEL4:
		            pop_nbytes_from_S(&count[0],3);
		            buf[5]=Hex2BcdChar(count[2]);
		            buf[6]=Hex2BcdChar(count[1]);  
		            buf[7]=Hex2BcdChar(count[0]);
		            buf[8]=Hex2BcdChar(SEARCH_METER_FLAG.sub_count-1);
		            buf[sizeof(ReadPower645_07)-2]=GetCheckSum(buf+4,sizeof(ReadPower645_07)-4-2);      
		            push_nbytes_to_S_dec(&count[0],3);
#ifdef DEBUG_LOG_SEARCH					
					debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645 level(4)\r\n");
					debug_hex(DEBUG_LOG_NET, buf, sizeof(ReadPower645_07));
#endif					
		            break;
		        case SEARCH_LEVEL5:
		            pop_nbytes_from_S(&count[0],4);
		            buf[5]=Hex2BcdChar(count[3]);
		            buf[6]=Hex2BcdChar(count[2]);
		            buf[7]=Hex2BcdChar(count[1]);   
		            buf[8]=Hex2BcdChar(count[0]);
		            buf[9]=Hex2BcdChar(SEARCH_METER_FLAG.sub_count-1);
		            buf[sizeof(ReadPower645_07)-2]=GetCheckSum(buf+4,sizeof(ReadPower645_07)-4-2);            
		            push_nbytes_to_S_dec(&count[0],4);
#ifdef DEBUG_LOG_SEARCH					
					debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645 level(5)\r\n");
					debug_hex(DEBUG_LOG_NET, buf, sizeof(ReadPower645_07));
#endif					
		            break;
		        case SEARCH_LEVEL6:
		            pop_nbytes_from_S(&count[0],5);
		            buf[5]=Hex2BcdChar(count[4]);
		            buf[6]=Hex2BcdChar(count[3]);
		            buf[7]=Hex2BcdChar(count[2]);
		            buf[8]=Hex2BcdChar(count[1]);   
		            buf[9]=Hex2BcdChar(count[0]);
		            buf[10]=Hex2BcdChar(SEARCH_METER_FLAG.sub_count-1);
		            buf[sizeof(ReadPower645_07)-2]=GetCheckSum(buf+4,sizeof(ReadPower645_07)-4-2);           
		            push_nbytes_to_S_dec(&count[0],5);
#ifdef DEBUG_LOG_SEARCH					
					debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645 level(6)\r\n");
					debug_hex(DEBUG_LOG_NET, buf, sizeof(ReadPower645_07));
#endif					
		            break;
		        default:
		        	{
#ifdef DEBUG_LOG_SEARCH		        	
		        		debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645 level<%d>\r\n", conflict_table.conflict_level);
#endif
		        	}
		            break;
		    }     
		}
		
		{
		u8 addr[6] = {0};
		AppLocalReceiveData( addr, addr, SEARCH_METER_1500MS, 0, buf, sizeof(ReadPower645_07), RS485DataDeal, 0, 0, baud_t );
//		AppLocalReceiveData(SEARCH_METER_1500MS,
//				0,
//				0,
//				0,
//				0,
//				0,
//				0,
//				0, 
//				buf, 
//				sizeof(ReadPower645_07), 
//				RS485DataDeal,
//				0,
//				0);          
		}
		SetWait485DataFlag(UART_SCAN_485);
    }
}


void SubCycleSend645(void)
{
	if(SEARCH_METER_FLAG.sub_count == 100 && GetFirstCheckTableFlag() == false)//保证99也抄读两次
	{
	    SEARCH_METER_FLAG.sub_count_99_twice++;
		if(SEARCH_METER_FLAG.sub_count_99_twice < 2)
		{
			SEARCH_METER_FLAG.search_times=2;
			SEARCH_METER_FLAG.sub_count=99;
		}				
		else
			SEARCH_METER_FLAG.sub_count_99_twice=0;
	}
	else if(  (conflict_table.conflict_level == SEARCH_LEVEL0) && SEARCH_METER_FLAG.sub_count == BAUDTABLE_T_SIZE && GetFirstCheckTableFlag() == false)//保证最后一个波段也抄读两次
	{
	    SEARCH_METER_FLAG.sub_count_99_twice++;
		if(SEARCH_METER_FLAG.sub_count_99_twice < 2)
		{
			SEARCH_METER_FLAG.search_times=2;
			SEARCH_METER_FLAG.sub_count=BAUDTABLE_T_SIZE-1;
		}				
		else
			SEARCH_METER_FLAG.sub_count_99_twice=0;
	}
	if( (SEARCH_METER_FLAG.sub_count >99) ||
	     ( (conflict_table.conflict_level == SEARCH_LEVEL0) && (SEARCH_METER_FLAG.sub_count>= BAUDTABLE_T_SIZE ) )
	   )
	{
		//初始化计数器, 为下一次扫描做准备
		SEARCH_METER_FLAG.sub_count =0;
		SEARCH_METER_FLAG.search_times = 0;
		
		if((conflict_level1.conflict_count ==0)&&(conflict_level2.conflict_count ==0)&&(conflict_level3.conflict_count ==0)
		 &&(conflict_level4.conflict_count ==0)&&(conflict_level5.conflict_count ==0)&&(conflict_level0.conflict_count ==0))
		{
#ifdef II_STA_SUPPORT_DLT645_97
			SetSerach64597OverFlag(true);
			ConflictVariableInit();
#else
#ifndef II_STA_ONLY645
			SetSerach698OverFlag(true);
			ConflictVariableInit();
#else
			//不启动698搜表
			SetCheckTableFlag(false);//关闭搜表
			table_copy_fun();
			set_read_event_start_flag(false);
#endif
#endif

#ifdef DEBUG_SEARCH_LOG_OUT
            debug_str(DEBUG_LOG_APP, "SubCycleSend645() Search Mater OK: SEARCH_METER_FLAG.meter_total=%d, SEARCH_METER_FLAG.TableList=0X%x \r\n", SEARCH_METER_FLAG.meter_total, SEARCH_METER_FLAG.TableList );
			if( SEARCH_METER_FLAG.meter_total > 0 )
            {
                u8 i;
                u32 table_list_tmp = SEARCH_METER_FLAG.TableList;
                for ( i=0; i < 32; i++ )
                {
                    if((table_list_tmp&0x00000001)==0x00000001)
                    {
						struct strList_Meter* meter_list = (struct strList_Meter*)(&SEARCH_METER_FLAG.table[i*7]);//add by lxl, 20211106@1419
                        debug_str( DEBUG_LOG_APP, "meter addr:%02x%02x%02x%02x%02x%02x, Statute=%d, BaudType=%d\r\n", 
							SEARCH_METER_FLAG.table[i*7+5], SEARCH_METER_FLAG.table[i*7+4], SEARCH_METER_FLAG.table[i*7+3], 
							SEARCH_METER_FLAG.table[i*7+2], SEARCH_METER_FLAG.table[i*7+1], SEARCH_METER_FLAG.table[i*7], 
							meter_list->Type.Statute, meter_list->Type.BaudType );
                    }
                    table_list_tmp = table_list_tmp >> 1;
					if( table_list_tmp == 0 )
					{
						break;
					}
                }                
            }
#endif
            return;
		}
		else
		{
			switch(conflict_table.conflict_level)
			{
            	case SEARCH_LEVEL0:
					if(conflict_level0.conflict_count >0)
					{
						//准备好相应的波特率
						CurrentBaud = baudTable_t[ conflict_level0.conflict_list[conflict_level0.conflict_count-1] ];
						
						//转到下一级
						conflict_level0.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL1;
					}
					break;
            	case SEARCH_LEVEL1:
					if(conflict_level1.conflict_count >0)
					{
		                push_nbytes_to_S_and_clear(&conflict_level1.conflict_list[conflict_level1.conflict_count -1],1);
						conflict_level1.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL2;
					}
					else
					{
						if(conflict_level0.conflict_count >0)
						{
							//准备好相应的波特率
							CurrentBaud = baudTable_t[ conflict_level0.conflict_list[conflict_level0.conflict_count-1] ];
							
							conflict_level0.conflict_count --;
							conflict_table.conflict_level =SEARCH_LEVEL1;
						}
					}
					break;
				case SEARCH_LEVEL2:
					if(conflict_level2.conflict_count >0)
					{
						push_nbytes_to_S_and_clear(&conflict_level2.conflict_list[conflict_level2.conflict_count -1][0],2);
						conflict_level2.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL3;
						
					}
					else
					{
//						if(conflict_level1.conflict_count >0)
//						{
//			                push_nbytes_to_S_and_clear(&conflict_level1.conflict_list[conflict_level1.conflict_count -1],1);
//							conflict_level1.conflict_count --;
//							conflict_table.conflict_level =SEARCH_LEVEL2;
//						}
						conflict_table.conflict_level=SEARCH_LEVEL1;
                        SEARCH_METER_FLAG.sub_count=GOTO_LOW_LEVEL_VALUE_645;//以便可以进入 case SEARCH_LEVEL1,进而进行冲突值压栈push操作，而不是直接进入下面出栈pop                        
                        return;
					}
					break;
				case SEARCH_LEVEL3:
					if(conflict_level3.conflict_count >0)
					{
						push_nbytes_to_S_and_clear(&conflict_level3.conflict_list[conflict_level3.conflict_count -1][0],3);
						conflict_level3.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL4;
					}	
					else
					{
						conflict_table.conflict_level=SEARCH_LEVEL2;
                        SEARCH_METER_FLAG.sub_count=GOTO_LOW_LEVEL_VALUE_645;//以便可以进入 case SEARCH_LEVEL2,进而进行冲突值压栈push操作，而不是直接进入下面出栈pop                        
                        return;
					}
					break;
				case SEARCH_LEVEL4:
					if(conflict_level4.conflict_count >0)
					{
						push_nbytes_to_S_and_clear(&conflict_level4.conflict_list[conflict_level4.conflict_count -1][0],4);	
						conflict_level4.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL5;
					}
					else
					{
					    	conflict_table.conflict_level=SEARCH_LEVEL3;
                        			SEARCH_METER_FLAG.sub_count=GOTO_LOW_LEVEL_VALUE_645;                    
                        			return;
					}
					
					break;
				case SEARCH_LEVEL5:
					if(conflict_level5.conflict_count >0)
					{
						push_nbytes_to_S_and_clear(&conflict_level5.conflict_list[conflict_level5.conflict_count -1][0],5);	
						conflict_level5.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL6;
					}
					else
					{
						conflict_table.conflict_level=SEARCH_LEVEL4;
                        			SEARCH_METER_FLAG.sub_count=GOTO_LOW_LEVEL_VALUE_645;                    
                       	 			return;
					}
					break;
				case SEARCH_LEVEL6:
					if(conflict_level5.conflict_count >0)
					{
						push_nbytes_to_S_and_clear(&conflict_level5.conflict_list[conflict_level5.conflict_count -1][0],5);	
						conflict_level5.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL6;
						break;
					}
					if(conflict_level4.conflict_count >0)
					{
						push_nbytes_to_S_and_clear(&conflict_level4.conflict_list[conflict_level4.conflict_count -1][0],4);	
						conflict_level4.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL5;
						break;
					}
					
					if(conflict_level3.conflict_count >0)
					{
						push_nbytes_to_S_and_clear(&conflict_level3.conflict_list[conflict_level3.conflict_count -1][0],3);	
						conflict_level3.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL4;
						break;
					}
					if(conflict_level2.conflict_count >0)
					{
						push_nbytes_to_S_and_clear(&conflict_level2.conflict_list[conflict_level2.conflict_count -1][0],2);	
						conflict_level2.conflict_count --;						
						conflict_table.conflict_level =SEARCH_LEVEL3;
						break;
					}
					if(conflict_level1.conflict_count >0)
					{
						push_nbytes_to_S_and_clear(&conflict_level1.conflict_list[conflict_level1.conflict_count -1],1);	
						conflict_level1.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL2;
						break;
					}												
					if(conflict_level0.conflict_count >0)//增加对多波特率支持, add by lxl, 20211106
					{
						//准备好相应的波特率
						CurrentBaud = baudTable_t[ conflict_level0.conflict_list[conflict_level0.conflict_count-1] ];	
						conflict_level0.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL1;
						break;
					}
					break;						
				default:
					//conflict_table.conflict_level =0;
					break;
            }			
			
		}
    }

	if(GetFirstCheckTableFlag() == false)
	{
		//复搜时, sub_count从0开始, 表号从sub_count开始, sub_count=0-->表号=255, 导致数组越界
		//为0时, 要变成1
		if( SEARCH_METER_FLAG.sub_count == 0 )
			SEARCH_METER_FLAG.sub_count++;
		
		if(SEARCH_METER_FLAG.search_times >= 2)
		{
			SEARCH_METER_FLAG.search_times = 0;
			SEARCH_METER_FLAG.sub_count++;
			
		}
		SEARCH_METER_FLAG.search_times++;
	}
	else
	{
		SEARCH_METER_FLAG.search_times=0;
		SEARCH_METER_FLAG.sub_count++;
	}
	
	//如果是LEVEL0, 准备先切换好波特率
	/**/
	{
		if(conflict_table.conflict_level == SEARCH_LEVEL0)
		{
			CurrentBaud = baudTable_t[ SEARCH_METER_FLAG.sub_count-1 ];
		}
//		else if(conflict_table.conflict_level == SEARCH_LEVEL1)
//		{
//			//if( CurrentBaud != GetLoacalBaud() )
//			//	ChangeLocalUartBaud_t( CurrentBaud );
//		}
	}
	
#ifdef DEBUG_SEARCH_LOG_OUT
    debug_str( DEBUG_LOG_APP, "  SubCycleSend645(): conflict_level=%d, sub_count=%d, CurrentBaud=%d\r\n", conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count, CurrentBaud );
#endif

    MakeFrame645Send2RS485( 0x80000000 | CurrentBaud );
}

void MakeFrame64597Send2RS485(u32 baud_t)
{		
	u8 count[6];
	static u8 buf[sizeof(ReadPower645_97)];
	
	if (SEARCH_METER_FLAG.sub_count != 0)
	{
		memcpy(buf, ReadPower645_97, sizeof(ReadPower645_97));
		switch (conflict_table.conflict_level)
		{
			case SEARCH_LEVEL0:
#ifdef DEBUG_LOG_SEARCH
				debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645_97 level(0)\r\n");
#endif
				break;
			case SEARCH_LEVEL1: 			
				buf[5] = Hex2BcdChar(SEARCH_METER_FLAG.sub_count-1);
				buf[sizeof(ReadPower645_97)-2] = GetCheckSum(buf+4, sizeof(ReadPower645_97)-4-2);
#ifdef DEBUG_LOG_SEARCH				
				debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645_97 level(1)\r\n");
				debug_hex(DEBUG_LOG_NET, buf, sizeof(ReadPower645_97));
#endif
				break;
			case SEARCH_LEVEL2:
				pop_nbytes_from_S(&count[0], 1); //获取第一级别搜表时的冲突地址   
				buf[5] = Hex2BcdChar(count[0]);
				buf[6] = Hex2BcdChar(SEARCH_METER_FLAG.sub_count-1);
				buf[sizeof(ReadPower645_97)-2] = GetCheckSum(buf+4, sizeof(ReadPower645_97)-4-2);
				push_nbytes_to_S_dec(&count[0], 1);//将冲突地址压回堆栈   
#ifdef DEBUG_LOG_SEARCH		            
				debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645_97 level(2)\r\n");
				debug_hex(DEBUG_LOG_NET, buf, sizeof(ReadPower645_97));
#endif					
				break;
			case SEARCH_LEVEL3:
				pop_nbytes_from_S(&count[0], 2);
				buf[5] = Hex2BcdChar(count[1]);  
				buf[6] = Hex2BcdChar(count[0]);
				buf[7] = Hex2BcdChar(SEARCH_METER_FLAG.sub_count-1);
				buf[sizeof(ReadPower645_97)-2] = GetCheckSum(buf+4, sizeof(ReadPower645_97)-4-2);	   
				push_nbytes_to_S_dec(&count[0], 2);
#ifdef DEBUG_LOG_SEARCH					
				debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645_97 level(3)\r\n");
				debug_hex(DEBUG_LOG_NET, buf, sizeof(ReadPower645_97));
#endif					
				break;
			case SEARCH_LEVEL4:
				pop_nbytes_from_S(&count[0], 3);
				buf[5] = Hex2BcdChar(count[2]);
				buf[6] = Hex2BcdChar(count[1]);  
				buf[7] = Hex2BcdChar(count[0]);
				buf[8] = Hex2BcdChar(SEARCH_METER_FLAG.sub_count-1);
				buf[sizeof(ReadPower645_97)-2] = GetCheckSum(buf+4, sizeof(ReadPower645_97)-4-2);		
				push_nbytes_to_S_dec(&count[0], 3);
#ifdef DEBUG_LOG_SEARCH					
				debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645_97 level(4)\r\n");
				debug_hex(DEBUG_LOG_NET, buf, sizeof(ReadPower645_97));
#endif					
				break;
			case SEARCH_LEVEL5:
				pop_nbytes_from_S(&count[0], 4);
				buf[5] = Hex2BcdChar(count[3]);
				buf[6] = Hex2BcdChar(count[2]);
				buf[7] = Hex2BcdChar(count[1]);	
				buf[8] = Hex2BcdChar(count[0]);
				buf[9] = Hex2BcdChar(SEARCH_METER_FLAG.sub_count-1);
				buf[sizeof(ReadPower645_97)-2] = GetCheckSum(buf+4, sizeof(ReadPower645_97)-4-2);			  
				push_nbytes_to_S_dec(&count[0], 4);
#ifdef DEBUG_LOG_SEARCH					
				debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645_97 level(5)\r\n");
				debug_hex(DEBUG_LOG_NET, buf, sizeof(ReadPower645_97));
#endif					
				break;
			case SEARCH_LEVEL6:
				pop_nbytes_from_S(&count[0], 5);
				buf[5] = Hex2BcdChar(count[4]);
				buf[6] = Hex2BcdChar(count[3]);
				buf[7] = Hex2BcdChar(count[2]);
				buf[8] = Hex2BcdChar(count[1]);	
				buf[9] = Hex2BcdChar(count[0]);
				buf[10] = Hex2BcdChar(SEARCH_METER_FLAG.sub_count-1);
				buf[sizeof(ReadPower645_97)-2] = GetCheckSum(buf+4, sizeof(ReadPower645_97)-4-2);			 
				push_nbytes_to_S_dec(&count[0], 5);
#ifdef DEBUG_LOG_SEARCH					
				debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645_97 level(6)\r\n");
				debug_hex(DEBUG_LOG_NET, buf, sizeof(ReadPower645_97));
#endif					
				break;
			default:
				{
#ifdef DEBUG_LOG_SEARCH		        	
					debug_str(DEBUG_LOG_NET, "\r\nMakeFrame645_97 level<%d>\r\n", conflict_table.conflict_level);
#endif
				}
				break;
		}	  
		
		u8 addr[6] = {0};
		AppLocalReceiveData(addr, addr, SEARCH_METER_1500MS, 0, buf, sizeof(ReadPower645_97), RS485DataDeal, 0, 0, baud_t);

		SetWait485DataFlag(UART_SCAN_485);
	}
}

void SubCycleSend64597(void)
{
	if (SEARCH_METER_FLAG.sub_count == 100 && GetFirstCheckTableFlag() == false) //保证99也抄读两次
	{
	    SEARCH_METER_FLAG.sub_count_99_twice++;
		if(SEARCH_METER_FLAG.sub_count_99_twice < 2)
		{
			SEARCH_METER_FLAG.search_times = 2;
			SEARCH_METER_FLAG.sub_count = 99;
		}				
		else
			SEARCH_METER_FLAG.sub_count_99_twice = 0;
	}
	else if ((conflict_table.conflict_level == SEARCH_LEVEL0) && SEARCH_METER_FLAG.sub_count == BAUDTABLE_T_SIZE && GetFirstCheckTableFlag() == false) //保证最后一个波段也抄读两次
	{
	    SEARCH_METER_FLAG.sub_count_99_twice++;
		if(SEARCH_METER_FLAG.sub_count_99_twice < 2)
		{
			SEARCH_METER_FLAG.search_times = 2;
			SEARCH_METER_FLAG.sub_count = BAUDTABLE_T_SIZE-1;
		}				
		else
			SEARCH_METER_FLAG.sub_count_99_twice = 0;
	}
	
	if ((SEARCH_METER_FLAG.sub_count >99) ||
	    ((conflict_table.conflict_level == SEARCH_LEVEL0) && (SEARCH_METER_FLAG.sub_count>= BAUDTABLE_T_SIZE)))
	{
		//初始化计数器, 为下一次扫描做准备
		SEARCH_METER_FLAG.sub_count = 0;
		SEARCH_METER_FLAG.search_times = 0;
		
		if ((conflict_level1.conflict_count ==0)&&(conflict_level2.conflict_count ==0)&&(conflict_level3.conflict_count ==0)
		  &&(conflict_level4.conflict_count ==0)&&(conflict_level5.conflict_count ==0)&&(conflict_level0.conflict_count ==0))
		{
			SetCheckTableFlag(false); //关闭搜表
			table_copy_fun();
			set_read_event_start_flag(false);

#ifdef DEBUG_SEARCH_LOG_OUT
            debug_str(DEBUG_LOG_APP, "SubCycleSend645_97() Search Mater OK: SEARCH_METER_FLAG.meter_total=%d, SEARCH_METER_FLAG.TableList=0X%x \r\n", SEARCH_METER_FLAG.meter_total, SEARCH_METER_FLAG.TableList );
			if (SEARCH_METER_FLAG.meter_total > 0)
            {
                u8 i;
                u32 table_list_tmp = SEARCH_METER_FLAG.TableList;
                for (i=0; i<32; i++)
                {
                    if ((table_list_tmp&0x00000001) == 0x00000001)
                    {
						struct strList_Meter* meter_list = (struct strList_Meter*)(&SEARCH_METER_FLAG.table[i*7]);
                        debug_str( DEBUG_LOG_APP, "meter addr:%02x%02x%02x%02x%02x%02x, Statute=%d, BaudType=%d\r\n", 
							SEARCH_METER_FLAG.table[i*7+5], SEARCH_METER_FLAG.table[i*7+4], SEARCH_METER_FLAG.table[i*7+3], 
							SEARCH_METER_FLAG.table[i*7+2], SEARCH_METER_FLAG.table[i*7+1], SEARCH_METER_FLAG.table[i*7], 
							meter_list->Type.Statute, meter_list->Type.BaudType);
                    }
                    table_list_tmp = table_list_tmp >> 1;
					if (table_list_tmp == 0)
					{
						break;
					}
                }                
            }
#endif
            return;
		}
		else
		{
			switch (conflict_table.conflict_level)
			{
            	case SEARCH_LEVEL0:
					if (conflict_level0.conflict_count > 0)
					{
						//准备好相应的波特率
						CurrentBaud = baudTable_t[conflict_level0.conflict_list[conflict_level0.conflict_count-1]];
						
						//转到下一级
						conflict_level0.conflict_count--;
						conflict_table.conflict_level = SEARCH_LEVEL1;
					}
					break;
            	case SEARCH_LEVEL1:
					if (conflict_level1.conflict_count > 0)
					{
		                push_nbytes_to_S_and_clear(&conflict_level1.conflict_list[conflict_level1.conflict_count -1], 1);
						conflict_level1.conflict_count--;
						conflict_table.conflict_level = SEARCH_LEVEL2;
					}
					else
					{
						if (conflict_level0.conflict_count > 0)
						{
							//准备好相应的波特率
							CurrentBaud = baudTable_t[conflict_level0.conflict_list[conflict_level0.conflict_count-1]];
							
							conflict_level0.conflict_count--;
							conflict_table.conflict_level = SEARCH_LEVEL1;
						}
					}
					break;
				case SEARCH_LEVEL2:
					if (conflict_level2.conflict_count > 0)
					{
						push_nbytes_to_S_and_clear(&conflict_level2.conflict_list[conflict_level2.conflict_count -1][0], 2);
						conflict_level2.conflict_count--;
						conflict_table.conflict_level = SEARCH_LEVEL3;
						
					}
					else
					{
						conflict_table.conflict_level = SEARCH_LEVEL1;
                        SEARCH_METER_FLAG.sub_count = GOTO_LOW_LEVEL_VALUE_645; //以便可以进入 case SEARCH_LEVEL1,进而进行冲突值压栈push操作，而不是直接进入下面出栈pop                        
                        return;
					}
					break;
				case SEARCH_LEVEL3:
					if (conflict_level3.conflict_count > 0)
					{
						push_nbytes_to_S_and_clear(&conflict_level3.conflict_list[conflict_level3.conflict_count -1][0], 3);
						conflict_level3.conflict_count--;
						conflict_table.conflict_level = SEARCH_LEVEL4;
					}	
					else
					{
						conflict_table.conflict_level = SEARCH_LEVEL2;
                        SEARCH_METER_FLAG.sub_count = GOTO_LOW_LEVEL_VALUE_645; //以便可以进入 case SEARCH_LEVEL2,进而进行冲突值压栈push操作，而不是直接进入下面出栈pop                        
                        return;
					}
					break;
				case SEARCH_LEVEL4:
					if (conflict_level4.conflict_count > 0)
					{
						push_nbytes_to_S_and_clear(&conflict_level4.conflict_list[conflict_level4.conflict_count -1][0], 4);	
						conflict_level4.conflict_count--;
						conflict_table.conflict_level = SEARCH_LEVEL5;
					}
					else
					{
				    	conflict_table.conflict_level = SEARCH_LEVEL3;
            			SEARCH_METER_FLAG.sub_count = GOTO_LOW_LEVEL_VALUE_645;                    
            			return;
					}
					break;
				case SEARCH_LEVEL5:
					if (conflict_level5.conflict_count > 0)
					{
						push_nbytes_to_S_and_clear(&conflict_level5.conflict_list[conflict_level5.conflict_count -1][0], 5);	
						conflict_level5.conflict_count--;
						conflict_table.conflict_level = SEARCH_LEVEL6;
					}
					else
					{
						conflict_table.conflict_level=SEARCH_LEVEL4;
            			SEARCH_METER_FLAG.sub_count=GOTO_LOW_LEVEL_VALUE_645;                    
           	 			return;
					}
					break;
				case SEARCH_LEVEL6:
					if (conflict_level5.conflict_count > 0)
					{
						push_nbytes_to_S_and_clear(&conflict_level5.conflict_list[conflict_level5.conflict_count -1][0], 5);	
						conflict_level5.conflict_count--;
						conflict_table.conflict_level = SEARCH_LEVEL6;
						break;
					}
					
					if (conflict_level4.conflict_count > 0)
					{
						push_nbytes_to_S_and_clear(&conflict_level4.conflict_list[conflict_level4.conflict_count -1][0], 4);	
						conflict_level4.conflict_count--;
						conflict_table.conflict_level = SEARCH_LEVEL5;
						break;
					}
					
					if (conflict_level3.conflict_count > 0)
					{
						push_nbytes_to_S_and_clear(&conflict_level3.conflict_list[conflict_level3.conflict_count -1][0], 3);	
						conflict_level3.conflict_count--;
						conflict_table.conflict_level = SEARCH_LEVEL4;
						break;
					}
					
					if (conflict_level2.conflict_count > 0)
					{
						push_nbytes_to_S_and_clear(&conflict_level2.conflict_list[conflict_level2.conflict_count -1][0], 2);	
						conflict_level2.conflict_count--;						
						conflict_table.conflict_level = SEARCH_LEVEL3;
						break;
					}
					
					if (conflict_level1.conflict_count > 0)
					{
						push_nbytes_to_S_and_clear(&conflict_level1.conflict_list[conflict_level1.conflict_count -1], 1);	
						conflict_level1.conflict_count--;
						conflict_table.conflict_level = SEARCH_LEVEL2;
						break;
					}		
					
					if (conflict_level0.conflict_count > 0)
					{
						//准备好相应的波特率
						CurrentBaud = baudTable_t[conflict_level0.conflict_list[conflict_level0.conflict_count-1]];	
						conflict_level0.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL1;
						break;
					}
					break;						
				default:
					//conflict_table.conflict_level =0;
					break;
            }			
		}
    }

	if (GetFirstCheckTableFlag() == false)
	{
		//复搜时, sub_count从0开始, 表号从sub_count开始, sub_count=0-->表号=255, 导致数组越界
		//为0时, 要变成1
		if(SEARCH_METER_FLAG.sub_count == 0)
			SEARCH_METER_FLAG.sub_count++;
		
		if(SEARCH_METER_FLAG.search_times >= 2)
		{
			SEARCH_METER_FLAG.search_times = 0;
			SEARCH_METER_FLAG.sub_count++;
			
		}
		SEARCH_METER_FLAG.search_times++;
	}
	else
	{
		SEARCH_METER_FLAG.search_times = 0;
		SEARCH_METER_FLAG.sub_count++;
	}
	
	//如果是LEVEL0, 准备先切换好波特率
	{
		if(conflict_table.conflict_level == SEARCH_LEVEL0)
		{
			CurrentBaud = baudTable_t[SEARCH_METER_FLAG.sub_count-1];
		}
	}
	
#ifdef DEBUG_SEARCH_LOG_OUT
    debug_str( DEBUG_LOG_APP, "  SubCycleSend645_97(): conflict_level=%d, sub_count=%d, CurrentBaud=%d\r\n", conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count, CurrentBaud );
#endif

    MakeFrame64597Send2RS485(0x80000000 | CurrentBaud);
}

void MakeFrame698Send2RS485(void)
{
	static u8 buff698[29];
    u8 count[12];
	u16 crc_value=0;	 
	
    if(SEARCH_METER_FLAG.sub_count !=0)
    {
    	if(SEARCH_METER_FLAG.search_times == 0)
    	{
    		memcpy(buff698,ReadPower_698,29);
	    	switch(conflict_table.conflict_level)
		    {
		        case SEARCH_LEVEL1:                          
		            buff698[9]=0xA0 | (SEARCH_METER_FLAG.sub_count-1);	            
		            break;
		        case SEARCH_LEVEL2: 
	                pop_nbytes_from_S(&count[0],1);//获取第一级别搜表时的冲突地址                
		            buff698[9]=((SEARCH_METER_FLAG.sub_count-1)<<4) | count[0];	            
	                push_nbytes_to_S_dec(&count[0],1);//将冲突地址压回堆栈
		            break;
		        case SEARCH_LEVEL3:
		            pop_nbytes_from_S(&count[0],2);
		            buff698[9]=(count[0]<<4) | count[1];   
		            buff698[10]=0xA0 | (SEARCH_METER_FLAG.sub_count-1);	                       	            
		            push_nbytes_to_S_dec(&count[0],2);
		            break;
		        case SEARCH_LEVEL4:
		            pop_nbytes_from_S(&count[0],3);
		            buff698[9]=(count[1]<<4) | (count[2]);
		            buff698[10]=((SEARCH_METER_FLAG.sub_count-1)<<4) | (count[0]);            
		            push_nbytes_to_S_dec(&count[0],3);
		            break;
		        case SEARCH_LEVEL5:
		            pop_nbytes_from_S(&count[0],4);                
		            buff698[9]=(count[2]<<4) | (count[3]);
		            buff698[10]=(count[0]<<4) | (count[1]);
		            buff698[11]=0xA0 | (SEARCH_METER_FLAG.sub_count-1);   
		            push_nbytes_to_S_dec(&count[0],4);
		            break;
		        case SEARCH_LEVEL6:
		            pop_nbytes_from_S(&count[0],5);                
		            buff698[9]=(count[3]<<4) | (count[4]);
		            buff698[10]=(count[1]<<4) | (count[2]);
		            buff698[11]=((SEARCH_METER_FLAG.sub_count-1)<<4) | (count[0]);	                                    
		            push_nbytes_to_S_dec(&count[0],5);
		            break;
	            case SEARCH_LEVEL7:
		            pop_nbytes_from_S(&count[0],6);
		            buff698[9]=(count[4]<<4) | (count[5]);
		            buff698[10]=(count[2]<<4) | (count[3]);
		            buff698[11]=(count[0]<<4) | (count[1]);	
	                buff698[12]=0xA0 | (SEARCH_METER_FLAG.sub_count-1);
	                push_nbytes_to_S_dec(&count[0],6);
		            break;
	            case SEARCH_LEVEL8:
		            pop_nbytes_from_S(&count[0],7);
		            buff698[9]=(count[5]<<4) | (count[6]);
		            buff698[10]=(count[3]<<4) | (count[4]);
		            buff698[11]=(count[1]<<4) | (count[2]);	
	                buff698[12]=((SEARCH_METER_FLAG.sub_count-1)<<4) | (count[0]);
	                push_nbytes_to_S_dec(&count[0],7);
		            break;
	            case SEARCH_LEVEL9:
		            pop_nbytes_from_S(&count[0],8);
		            buff698[9]=(count[6]<<4) | (count[7]);
		            buff698[10]=(count[4]<<4) | (count[5]);
		            buff698[11]=(count[2]<<4) | (count[3]);
	                buff698[12]=(count[0]<<4) | (count[1]);
	                buff698[13]=0xA0 | (SEARCH_METER_FLAG.sub_count-1);
	                push_nbytes_to_S_dec(&count[0],8);
		            break;
	            case SEARCH_LEVEL10:
		            pop_nbytes_from_S(&count[0],9);
		            buff698[9]=(count[7]<<4) | (count[8]);
		            buff698[10]=(count[5]<<4) | (count[6]);
		            buff698[11]=(count[3]<<4) | (count[4]);
	                buff698[12]=(count[1]<<4) | (count[2]);
	                buff698[13]=((SEARCH_METER_FLAG.sub_count-1)<<4) | (count[0]);
	                push_nbytes_to_S_dec(&count[0],9);
		            break;
	            case SEARCH_LEVEL11:
		            pop_nbytes_from_S(&count[0],10);
		            buff698[9]=(count[8]<<4) | (count[9]);
		            buff698[10]=(count[6]<<4) | (count[7]);
		            buff698[11]=(count[4]<<4) | (count[5]);                
	                buff698[12]=(count[2]<<4) | (count[3]);
	                buff698[13]=(count[0]<<4) | (count[1]);	
	                buff698[14]=0xA0 | (SEARCH_METER_FLAG.sub_count-1);
	                push_nbytes_to_S_dec(&count[0],10);
		            break;
	            case SEARCH_LEVEL12:
		            pop_nbytes_from_S(&count[0],11);
		            buff698[9]=(count[9]<<4) | (count[10]);
		            buff698[10]=(count[7]<<4) | (count[8]);
		            buff698[11]=(count[5]<<4) | (count[6]);                
	                buff698[12]=(count[3]<<4) | (count[4]);
	                buff698[13]=(count[1]<<4) | (count[2]);	
	                buff698[14]=((SEARCH_METER_FLAG.sub_count-1)<<4) | (count[0]);
	                push_nbytes_to_S_dec(&count[0],11);
		            break;
		        default: break;
		    }
	        crc_value = GetCrc16(&buff698[5],11);
	    	buff698[16] = crc_value&0x00ff;
	    	buff698[17] = (crc_value>>8)&0x00ff;

	    	crc_value = GetCrc16(&buff698[5],21);
	    	buff698[26] = crc_value&0x00ff;
	    	buff698[27] = (crc_value>>8)&0x00ff;
    	}
		{
			
		u8 addr[6] = {0};
		AppLocalReceiveData( addr, addr, SEARCH_METER_1500MS, 0, buff698, sizeof(buff698), RS485DataDeal, 0, 0, BAUD_SEARCH );
//		AppLocalReceiveData(SEARCH_METER_1500MS,
//				0,
//				0,
//				0,
//				0,
//				0,
//				0,
//				0, 
//				buff698, 
//				sizeof(ReadPower_698), 
//				RS485DataDeal,
//				0,
//				0);          
		}
		SetWait485DataFlag(UART_SCAN_485); 
    }
}
void SubCycleSend698(void)
{
	if(SEARCH_METER_FLAG.sub_count ==10 && GetFirstCheckTableFlag() == false)//保证9也抄读两次
	{
	    SEARCH_METER_FLAG.sub_count_99_twice++;
		if(SEARCH_METER_FLAG.sub_count_99_twice <2)
		{
			SEARCH_METER_FLAG.search_times=2;
			SEARCH_METER_FLAG.sub_count=9;
		}				
		else			
			SEARCH_METER_FLAG.sub_count_99_twice=0;
	}
	if(SEARCH_METER_FLAG.sub_count>=10)//DLT698.45规约搜表采用半字节方式，所以sub_count变化范围是: 0 ~ 9
    {
		SEARCH_METER_FLAG.sub_count =0;
		SEARCH_METER_FLAG.search_times=0;
		if((conflict_level1.conflict_count ==0)&&(conflict_level2.conflict_count ==0)&&(conflict_level3.conflict_count ==0)
	     &&(conflict_level4.conflict_count ==0)&&(conflict_level5.conflict_count ==0)&&(conflict_level6.conflict_count ==0)
         &&(conflict_level7.conflict_count ==0)&&(conflict_level8.conflict_count ==0)&&(conflict_level9.conflict_count ==0)
         &&(conflict_level10.conflict_count ==0)&&(conflict_level11.conflict_count ==0))//缩位寻址结束		
		{	
			SetCheckTableFlag(false);//关闭搜表
            table_copy_fun();
            set_read_event_start_flag(false);
            return;				
		}
		else
		{		
			switch(conflict_table.conflict_level)
			{
            	case SEARCH_LEVEL1:
					if(conflict_level1.conflict_count >0)
					{
		                push_nbytes_to_S_and_clear(&conflict_level1.conflict_list[conflict_level1.conflict_count -1],1);	
						conflict_level1.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL2;
					}
					break;
				case SEARCH_LEVEL2:
					if(conflict_level2.conflict_count >0)
					{
					    push_nbytes_to_S_and_clear(&conflict_level2.conflict_list[conflict_level2.conflict_count -1][0],2);	
						conflict_level2.conflict_count --;						
						conflict_table.conflict_level =SEARCH_LEVEL3;							
					}
					else
					{							
						if(conflict_level1.conflict_count >0)
						{
			                push_nbytes_to_S_and_clear(&conflict_level1.conflict_list[conflict_level1.conflict_count -1],1);	
							conflict_level1.conflict_count --;
							conflict_table.conflict_level =SEARCH_LEVEL2; 
						}
					}
					break;
				case SEARCH_LEVEL3:
					if(conflict_level3.conflict_count >0)
					{
						push_nbytes_to_S_and_clear(&conflict_level3.conflict_list[conflict_level3.conflict_count -1][0],3);	
						conflict_level3.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL4;
					}	
					else
					{
						conflict_table.conflict_level=SEARCH_LEVEL2;
                        SEARCH_METER_FLAG.sub_count=GOTO_LOW_LEVEL_VALUE_698;//以便可以进入 case 1,进而进行冲突值压栈push操作，而不是直接进入下面出栈pop
                        return;
					}
					break;
				case SEARCH_LEVEL4:
					if(conflict_level4.conflict_count >0)
					{
						push_nbytes_to_S_and_clear(&conflict_level4.conflict_list[conflict_level4.conflict_count -1][0],4);	
						conflict_level4.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL5;
					}
					else
					{
					    conflict_table.conflict_level=SEARCH_LEVEL3;
                        SEARCH_METER_FLAG.sub_count=GOTO_LOW_LEVEL_VALUE_698;//以便可以进入 case 2,进而进行冲突值压栈push操作，而不是直接进入下面出栈pop
                        return;
					}
					
					break;
				case SEARCH_LEVEL5:
					if(conflict_level5.conflict_count >0)
					{
						push_nbytes_to_S_and_clear(&conflict_level5.conflict_list[conflict_level5.conflict_count -1][0],5);	
						conflict_level5.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL6;
					}
					else
					{
						conflict_table.conflict_level=SEARCH_LEVEL4;
                        SEARCH_METER_FLAG.sub_count=GOTO_LOW_LEVEL_VALUE_698;//以便可以进入 case 3,进而进行冲突值压栈push操作，而不是直接进入下面出栈pop
                        return;
					}
					break;
                case SEARCH_LEVEL6:
					if(conflict_level6.conflict_count >0)
					{
						push_nbytes_to_S_and_clear(&conflict_level6.conflict_list[conflict_level6.conflict_count -1][0],6);	
						conflict_level6.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL7;
						
					}
					else
					{
						conflict_table.conflict_level=SEARCH_LEVEL5;
                        SEARCH_METER_FLAG.sub_count=GOTO_LOW_LEVEL_VALUE_698;//以便可以进入 case 4,进而进行冲突值压栈push操作，而不是直接进入下面出栈pop
                        return;
					}
					break;  
                case SEARCH_LEVEL7:                        
					if(conflict_level7.conflict_count >0)
					{                        
                        push_nbytes_to_S_and_clear(&conflict_level7.conflict_list[conflict_level7.conflict_count -1][0],7);							
						conflict_level7.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL8;							
					}
					else
					{
						conflict_table.conflict_level=SEARCH_LEVEL6;
                        SEARCH_METER_FLAG.sub_count=GOTO_LOW_LEVEL_VALUE_698;
                        return;
					}
					break; 
                case SEARCH_LEVEL8:                        
					if(conflict_level8.conflict_count >0)
					{	
                        push_nbytes_to_S_and_clear(&conflict_level8.conflict_list[conflict_level8.conflict_count -1][0],8);							
						conflict_level8.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL9;							
					}
					else
					{
						conflict_table.conflict_level=SEARCH_LEVEL7;
                        SEARCH_METER_FLAG.sub_count=GOTO_LOW_LEVEL_VALUE_698;
                        return;
					}
					break;
                case SEARCH_LEVEL9:                        
					if(conflict_level9.conflict_count >0)
					{	
                        push_nbytes_to_S_and_clear(&conflict_level9.conflict_list[conflict_level9.conflict_count -1][0],9);							
						conflict_level9.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL10;							
					}
					else
					{
						conflict_table.conflict_level=SEARCH_LEVEL8;
                        SEARCH_METER_FLAG.sub_count=GOTO_LOW_LEVEL_VALUE_698;
                        return;
					}
					break; 
                case SEARCH_LEVEL10:                        
					if(conflict_level10.conflict_count >0)
					{	
                        push_nbytes_to_S_and_clear(&conflict_level10.conflict_list[conflict_level10.conflict_count -1][0],10);							
						conflict_level10.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL11;							
					}
					else
					{
						conflict_table.conflict_level=SEARCH_LEVEL9;
                        SEARCH_METER_FLAG.sub_count=GOTO_LOW_LEVEL_VALUE_698;
                        return;
					}
					break; 
                case SEARCH_LEVEL11:                        
					if(conflict_level11.conflict_count >0)
					{	
                        push_nbytes_to_S_and_clear(&conflict_level11.conflict_list[conflict_level11.conflict_count -1][0],11);							
						conflict_level11.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL12;							
					}
					else
					{
						conflict_table.conflict_level=SEARCH_LEVEL10;
                        SEARCH_METER_FLAG.sub_count=GOTO_LOW_LEVEL_VALUE_698;
                        return;
					}
					break;                     
				case SEARCH_LEVEL12:
                    if(conflict_level11.conflict_count >0)
					{	
                        push_nbytes_to_S_and_clear(&conflict_level11.conflict_list[conflict_level11.conflict_count -1][0],11);							
						conflict_level11.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL12;							
					}
                    else if(conflict_level10.conflict_count >0)
					{	
                        push_nbytes_to_S_and_clear(&conflict_level10.conflict_list[conflict_level10.conflict_count -1][0],10);							
						conflict_level10.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL11;							
					}
                    else if(conflict_level9.conflict_count >0)
					{	
                        push_nbytes_to_S_and_clear(&conflict_level9.conflict_list[conflict_level9.conflict_count -1][0],9);							
						conflict_level9.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL10;							
					}
                    else if(conflict_level8.conflict_count >0)
					{	
                        push_nbytes_to_S_and_clear(&conflict_level8.conflict_list[conflict_level8.conflict_count -1][0],8);							
						conflict_level8.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL9;							
					}
                    else if(conflict_level7.conflict_count >0)
					{	
                        push_nbytes_to_S_and_clear(&conflict_level7.conflict_list[conflict_level7.conflict_count -1][0],7);							
						conflict_level7.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL8;							
					}                      
					else if(conflict_level6.conflict_count >0)
					{		
                        push_nbytes_to_S_and_clear(&conflict_level6.conflict_list[conflict_level6.conflict_count -1][0],6);							
						conflict_level6.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL7;							
					}	
					else if(conflict_level5.conflict_count >0)
					{		
                        push_nbytes_to_S_and_clear(&conflict_level5.conflict_list[conflict_level5.conflict_count -1][0],5);							
						conflict_level5.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL6;							
					}
                    else if(conflict_level4.conflict_count >0)
					{		
                        push_nbytes_to_S_and_clear(&conflict_level4.conflict_list[conflict_level4.conflict_count -1][0],4);							
						conflict_level4.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL5;							
					}
                    else if(conflict_level3.conflict_count >0)
					{			
                        push_nbytes_to_S_and_clear(&conflict_level3.conflict_list[conflict_level3.conflict_count -1][0],3);							
						conflict_level3.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL4;							
					}
                    else if(conflict_level2.conflict_count >0)
					{			
                        push_nbytes_to_S_and_clear(&conflict_level2.conflict_list[conflict_level2.conflict_count -1][0],2);							
						conflict_level2.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL3;							
					}
                    else if(conflict_level1.conflict_count >0)
					{		
                        push_nbytes_to_S_and_clear(&conflict_level1.conflict_list[conflict_level1.conflict_count -1],1);							
						conflict_level1.conflict_count --;
						conflict_table.conflict_level =SEARCH_LEVEL2;							
					}
					break;						
				default:break;
            }		
		}
    }
    if(GetFirstCheckTableFlag() == false)
	{
		SEARCH_METER_FLAG.search_times++;
		if(SEARCH_METER_FLAG.search_times >= 2)
		{
			SEARCH_METER_FLAG.search_times = 0;
			SEARCH_METER_FLAG.sub_count++;
		}
    }
	else
	{
		SEARCH_METER_FLAG.search_times = 0;
		SEARCH_METER_FLAG.sub_count++;
	}

	MakeFrame698Send2RS485();
}
//搜表时电表地址冲突值记录 
void conflict_val_record(void)
{
    u8 i,tmp,diff_count=0;
    
    tmp =SEARCH_METER_FLAG.sub_count-1;//当前的冲突值	
    //print_debug("not 645/698,conflict val:",&tmp,1);
    
    switch(conflict_table.conflict_level)//当前冲突级别
    {
      case SEARCH_LEVEL0:		
#ifdef DEBUG_LOG_SEARCH	  	
	  		debug_str(DEBUG_LOG_APP, "conflict record, level=%d, sub_count=%d, cnt:%d\r\n", 
				conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count, conflict_level0.conflict_count);
#endif
            		//print_debug("cur conflict level=1,count=",&conflict_level1.conflict_count,1);
			diff_count =0;                            
			for(i=0;i<conflict_level0.conflict_count;i++)
			{
				if(conflict_level0.conflict_list[i] != tmp)
					diff_count++;
			}
			if(diff_count == conflict_level0.conflict_count)//冲突已记录则不存储
			{
				//if(conflict_level0.conflict_count >= CONFLICT_CNT_MAX_PRO645)
				//		break;
				conflict_level0.conflict_list[conflict_level0.conflict_count]=tmp;
				conflict_level0.conflict_count++;
                
                		//print_debug("record conflict val",0,0);
			}
			break;
      case SEARCH_LEVEL1:	
#ifdef DEBUG_LOG_SEARCH	  	
	  	debug_str(DEBUG_LOG_APP, "conflict record, level=%d, sub_count=%d, cnt:%d\r\n", 
				conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count, conflict_level1.conflict_count);
#endif
            		//print_debug("cur conflict level=1,count=",&conflict_level1.conflict_count,1);
			diff_count =0;                            
			for(i=0;i<conflict_level1.conflict_count;i++)
			{
				if(conflict_level1.conflict_list[i] != tmp)
					diff_count++;
			}
			if(diff_count == conflict_level1.conflict_count)//冲突已记录则不存储
			{
				if(conflict_level1.conflict_count >= CONFLICT_CNT_MAX_PRO645)
						break;
				conflict_level1.conflict_list[conflict_level1.conflict_count]=tmp;
				conflict_level1.conflict_count++;
                
                		//print_debug("record conflict val",0,0);
			}
			break;
	case SEARCH_LEVEL2:	
#ifdef DEBUG_LOG_SEARCH		
		debug_str(DEBUG_LOG_APP, "conflict record, level=%d, sub_count=%d, cnt:%d\r\n", 
				conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count, conflict_level2.conflict_count);
#endif
            		//print_debug("cur conflict level=2,count=",&conflict_level2.conflict_count,1);
			diff_count =0;
			for(i=0;i<conflict_level2.conflict_count;i++)
			{
				if(conflict_level2.conflict_list[i][1] != tmp)
					diff_count++;
			}
			if(diff_count == conflict_level2.conflict_count)//冲突已记录则不存储
			{
                		if(conflict_level2.conflict_count >= CONFLICT_CNT_MAX_PRO645)
                    			break;
				StackPop(&S,&conflict_level2.conflict_list[conflict_level2.conflict_count][0]);
				conflict_level2.conflict_list[conflict_level2.conflict_count][1]=tmp;
				StackPush(&S,conflict_level2.conflict_list[conflict_level2.conflict_count][0]);
				conflict_level2.conflict_count++;
                		//print_debug("record conflict val",0,0);
			}							
			break;
	case SEARCH_LEVEL3:
#ifdef DEBUG_LOG_SEARCH		
		debug_str(DEBUG_LOG_APP, "conflict record, level=%d, sub_count=%d, cnt:%d\r\n", 
				conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count, conflict_level3.conflict_count);
#endif
            		//print_debug("cur conflict level=3,count=",&conflict_level3.conflict_count,1);
			diff_count =0;
			for(i=0;i<conflict_level3.conflict_count;i++)
			{
				if(conflict_level3.conflict_list[i][2] != tmp)
					diff_count++;
			}
			if(diff_count == conflict_level3.conflict_count)//冲突已记录则不存储
			{
                		if(conflict_level3.conflict_count >= CONFLICT_CNT_MAX_PRO645)
                    			break;
				pop_nbytes_from_S_dec(&conflict_level3.conflict_list[conflict_level3.conflict_count][0],2);
				conflict_level3.conflict_list[conflict_level3.conflict_count][2]=tmp;
				push_nbytes_to_S(&conflict_level3.conflict_list[conflict_level3.conflict_count][0],2);	
				conflict_level3.conflict_count++;
                		//print_debug("record conflict val",0,0);
			}
			break;
	case SEARCH_LEVEL4:
#ifdef DEBUG_LOG_SEARCH		
		debug_str(DEBUG_LOG_APP, "conflict record, level=%d, sub_count=%d, cnt:%d\r\n", 
				conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count, conflict_level4.conflict_count);
#endif
            		//print_debug("cur conflict level=4,count=",&conflict_level4.conflict_count,1);
			diff_count =0;
			for(i=0;i<conflict_level4.conflict_count;i++)
			{
				if(conflict_level4.conflict_list[i][3] != tmp)
					diff_count++;
			}
			if(diff_count == conflict_level4.conflict_count)//冲突已记录则不存储
			{
                		if(conflict_level4.conflict_count >= CONFLICT_CNT_MAX_PRO645)
                    			break;
				pop_nbytes_from_S_dec(&conflict_level4.conflict_list[conflict_level4.conflict_count][0],3);
				conflict_level4.conflict_list[conflict_level4.conflict_count][3]=tmp;
				push_nbytes_to_S(&conflict_level4.conflict_list[conflict_level4.conflict_count][0],3);	
				conflict_level4.conflict_count++;
                		//print_debug("record conflict val",0,0);
			}
			break;
	case SEARCH_LEVEL5:
#ifdef DEBUG_LOG_SEARCH		
		debug_str(DEBUG_LOG_APP, "conflict record, level=%d, sub_count=%d, cnt:%d\r\n", 
				conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count, conflict_level5.conflict_count);
#endif
            		//print_debug("cur conflict level=5,count=",&conflict_level5.conflict_count,1);
			diff_count =0;
			for(i=0;i<conflict_level5.conflict_count;i++)
			{
				if(conflict_level5.conflict_list[i][4] != tmp)
					diff_count++;
			}
			if(diff_count == conflict_level5.conflict_count)//冲突已记录则不存储
			{
                		if(conflict_level5.conflict_count >= CONFLICT_CNT_MAX_PRO645)
                    			break;
				pop_nbytes_from_S_dec(&conflict_level5.conflict_list[conflict_level5.conflict_count][0],4);
				conflict_level5.conflict_list[conflict_level5.conflict_count][4]=tmp;
				push_nbytes_to_S(&conflict_level5.conflict_list[conflict_level5.conflict_count][0],4);	
				conflict_level5.conflict_count++;
                		//print_debug("record conflict val",0,0);
			}
			break;
	case SEARCH_LEVEL6:
#ifdef DEBUG_LOG_SEARCH		
		debug_str(DEBUG_LOG_APP, "conflict record, level=%d, sub_count=%d, cnt:%d\r\n", 
				conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count, conflict_level6.conflict_count);
#endif
            		//print_debug("cur conflict level=6,count=",&conflict_level6.conflict_count,1);
			diff_count =0;
			for(i=0;i<conflict_level6.conflict_count;i++)
			{
				if(conflict_level6.conflict_list[i][5] != tmp)
					diff_count++;
			}
			if(diff_count == conflict_level6.conflict_count)//冲突已记录则不存储
			{
                		if(conflict_level6.conflict_count >= CONFLICT_CNT_MAX_PRO645)
                    			break;
				pop_nbytes_from_S_dec(&conflict_level6.conflict_list[conflict_level6.conflict_count][0],5);
				conflict_level6.conflict_list[conflict_level6.conflict_count][5]=tmp;
				push_nbytes_to_S(&conflict_level6.conflict_list[conflict_level6.conflict_count][0],5);	
				conflict_level6.conflict_count++;
                		//print_debug("record conflict val",0,0);
			}
			break;
        case SEARCH_LEVEL7:
#ifdef DEBUG_LOG_SEARCH			
			debug_str(DEBUG_LOG_APP, "conflict record, level=%d, sub_count=%d, cnt:%d\r\n", 
				conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count, conflict_level7.conflict_count);
#endif
            		//print_debug("cur conflict level=7,count=",&conflict_level7.conflict_count,1);
			diff_count =0;
			for(i=0;i<conflict_level7.conflict_count;i++)
			{
				if(conflict_level7.conflict_list[i][6] != tmp)
					diff_count++;
			}
			if(diff_count == conflict_level7.conflict_count)//冲突已记录则不存储
			{          
                		if(conflict_level7.conflict_count >= CONFLICT_CNT_MAX_PRO698)
                    			break;
                		pop_nbytes_from_S_dec(&conflict_level7.conflict_list[conflict_level7.conflict_count][0],6);
				conflict_level7.conflict_list[conflict_level7.conflict_count][6]=tmp;
               	 		push_nbytes_to_S(&conflict_level7.conflict_list[conflict_level7.conflict_count][0],6);								
				conflict_level7.conflict_count++;
                		//print_debug("record conflict val",0,0);
			}
			break;
        case SEARCH_LEVEL8:
#ifdef DEBUG_LOG_SEARCH			
			debug_str(DEBUG_LOG_APP, "conflict record, level=%d, sub_count=%d, cnt:%d\r\n", 
				conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count, conflict_level8.conflict_count);
#endif
            		//print_debug("cur conflict level=8,count=",&conflict_level8.conflict_count,1);
			diff_count =0;
			for(i=0;i<conflict_level8.conflict_count;i++)
			{
				if(conflict_level8.conflict_list[i][7] != tmp)
					diff_count++;
			}
			if(diff_count == conflict_level8.conflict_count)//冲突已记录则不存储
			{             
                		if(conflict_level8.conflict_count >= CONFLICT_CNT_MAX_PRO698)
                    			break;
                		pop_nbytes_from_S_dec(&conflict_level8.conflict_list[conflict_level8.conflict_count][0],7);
				conflict_level8.conflict_list[conflict_level8.conflict_count][7]=tmp;
                		push_nbytes_to_S(&conflict_level8.conflict_list[conflict_level8.conflict_count][0],7);								
				conflict_level8.conflict_count++;
                		//print_debug("record conflict val",0,0);
			}
			break;
        case SEARCH_LEVEL9:
#ifdef DEBUG_LOG_SEARCH			
			debug_str(DEBUG_LOG_APP, "conflict record, level=%d, sub_count=%d, cnt:%d\r\n", 
				conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count, conflict_level9.conflict_count);
#endif
            		//print_debug("cur conflict level=9,count=",&conflict_level9.conflict_count,1);
			diff_count =0;
			for(i=0;i<conflict_level9.conflict_count;i++)
			{
				if(conflict_level9.conflict_list[i][8] != tmp)
					diff_count++;
			}
			if(diff_count == conflict_level9.conflict_count)//冲突已记录则不存储
			{         
                		if(conflict_level9.conflict_count >= CONFLICT_CNT_MAX_PRO698)
                    			break;
                		pop_nbytes_from_S_dec(&conflict_level9.conflict_list[conflict_level9.conflict_count][0],8);
				conflict_level9.conflict_list[conflict_level9.conflict_count][8]=tmp;
                		push_nbytes_to_S(&conflict_level9.conflict_list[conflict_level9.conflict_count][0],8);								
				conflict_level9.conflict_count++;
                		//print_debug("record conflict val",0,0);
			}
			break;
        case SEARCH_LEVEL10:
#ifdef DEBUG_LOG_SEARCH			
			debug_str(DEBUG_LOG_APP, "conflict record, level=%d, sub_count=%d, cnt:%d\r\n", 
				conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count, conflict_level10.conflict_count);
#endif
            		//print_debug("cur conflict level=10,count=",&conflict_level10.conflict_count,1);
			diff_count =0;
			for(i=0;i<conflict_level10.conflict_count;i++)
			{
				if(conflict_level10.conflict_list[i][9] != tmp)
					diff_count++;
			}
			if(diff_count == conflict_level10.conflict_count)//冲突已记录则不存储
			{           
                		if(conflict_level10.conflict_count >= CONFLICT_CNT_MAX_PRO698)
                    			break;
                		pop_nbytes_from_S_dec(&conflict_level10.conflict_list[conflict_level10.conflict_count][0],9);
				conflict_level10.conflict_list[conflict_level10.conflict_count][9]=tmp;
                		push_nbytes_to_S(&conflict_level10.conflict_list[conflict_level10.conflict_count][0],9);								
				conflict_level10.conflict_count++;
                		//print_debug("record conflict val",0,0);
			}
			break;
        case SEARCH_LEVEL11:
#ifdef DEBUG_LOG_SEARCH			
			debug_str(DEBUG_LOG_APP, "conflict record, level=%d, sub_count=%d, cnt:%d\r\n", 
				conflict_table.conflict_level, SEARCH_METER_FLAG.sub_count, conflict_level11.conflict_count);
#endif
            		//print_debug("cur conflict level=11,count=",&conflict_level11.conflict_count,1);
			diff_count =0;
			for(i=0;i<conflict_level11.conflict_count;i++)
			{
				if(conflict_level11.conflict_list[i][10] != tmp)
					diff_count++;
			}
			if(diff_count == conflict_level11.conflict_count)//冲突已记录则不存储
			{           
                		if(conflict_level11.conflict_count >= CONFLICT_CNT_MAX_PRO698)
                    			break;
                		pop_nbytes_from_S_dec(&conflict_level11.conflict_list[conflict_level11.conflict_count][0],10);
				conflict_level11.conflict_list[conflict_level11.conflict_count][10]=tmp;
                		push_nbytes_to_S(&conflict_level11.conflict_list[conflict_level11.conflict_count][0],10);								
				conflict_level11.conflict_count++;
                		//print_debug("record conflict val",0,0);
			}
			break;
	default:break;
    }
}
#ifndef I_STA
void SearchMeter(void)
{
	OS_ERR err;
	static bool excute_once_flag = true;

	if (GetRS485ConflictFlag()) //等待电表响应超时或者485接收数据有冲突
	{
		SetRS485ConflictFlag(false);
		OSTaskSemPend(0, OS_OPT_PEND_NON_BLOCKING, NULL, &err); //相当于查询串口接收是否有冲突
		if (err == OS_ERR_NONE) //相当于串口接收数据有冲突
		{
			if ((SEARCH_METER_FLAG.search645_over_flag) && (SEARCH_METER_FLAG.send_AA_flag)) //缩位寻址时
			{
				conflict_val_record();
			}
		}
	}

	if (IsPowerOff == 1 && power == 0) //自身掉电不进行搜表
	{
		if (SEARCH_METER_FLAG.meter_total == 0)
			return;
		if (excute_once_flag)
		{
			excute_once_flag = false;
		}
	}
	else //??????
	{
		excute_once_flag = true;
		SetCheckPowerOffMeterEndFlag(false);

		if (AppUseUartFlag != 0)
			return;

		if (GetCMLPowerOnFlag())
		{
			if (SEARCH_METER_FLAG.meter_total != 0)
			{
#if defined(CHECKPOWEROFF_DELAY5S)
				debug_str(DEBUG_LOG_APP, "Wait 5S Start\r\n" );
				//OSTimeDly(5*1000, OS_OPT_TIME_DLY, &err);
				debug_str(DEBUG_LOG_APP, "Wait 5S End, Start check_meter_list_power_on()\r\n" );
#endif
				check_meter_list_power_on();
			}
			else
			{
				SetCMLPowerOnFlag(false);
				SearchMeterStart();
			}
		}
		else if (GetCheckTableFlag())
		{
//			if(!GetSendAAFlag())
//			{
//				OSTaskSemSet(&App_II_STA_TCB,0,&err);//相当于清空串口接收冲突数据
//				CycleSendAA645();
//			}
//			if((!GetSerach645OverFlag())&&(GetSendAAFlag()))
//			{
//				OSTaskSemSet(&App_II_STA_TCB,0,&err);//相当于清空串口接收冲突数据
//				CycleSendAA698();
//			}
			if ((GetSerach645OverFlag()) && (GetSendAAFlag()))
			{

#ifdef II_STA_SUPPORT_DLT645_97
				if(!GetSerach64597OverFlag())
	            {       
	            	OSTaskSemSet(&App_II_STA_TCB, 0, &err);//相当于清空串口接收冲突数据
					SubCycleSend645();               
	            }        
				
	            if(GetSerach64597OverFlag())
				{	
					OSTaskSemSet(&App_II_STA_TCB, 0, &err);//相当于清空串口接收冲突数据
					SubCycleSend64597();					
				}	
#else
				OSTaskSemSet(&App_II_STA_TCB, 0, &err); //相当于清空串口接收冲突数据
				SubCycleSend645();
#endif				
				if (SEARCH_METER_FLAG.meter_total == 0) //搜表过程中没有表地址的话不惊行切换频段
				{
					ResetChangeFreTimer();
				}
			}
		}
		else
		{
			OS_ERR err;
			u32 Tick = OSTimeGet(&err);
			u32 overTick = 24 * 60 * 60 * 1000;
			if (SEARCH_METER_FLAG.meter_total == 0)
			{
				//未搜到表5秒钟后再次搜索//未搜到表5分钟后再次搜索
				overTick = 5 * 1000;//overTick = 5 * 60 * 1000;//
				//修复复电时, 二采有电电表都没电, 但电表复电后, 上报电表都不在线的问题. add, by lxl, 20211208@1600
				if( IsPowerOff && (ResetFlag!=1) )//if( IsPowerOff )//
					ResetFlag = 1;
			}
			if ((Tick - SEARCH_METER_FLAG.end_search_tick) > overTick) //超时重新搜表: 未搜到表5秒钟后再次搜索, 搜到表24小时后再次搜索
			{
				SearchMeterStart();
				//修复复电时, 二采有电电表都没电, 但电表复电后, 上报电表都不在线的问题. add, by lxl, 20211208@1600
				if( ResetFlag == 1 )
				{
					ResetFlag = 2;
				}
			}
		}
	}
}
#else
//I 采搜表只需要定时向 1采本体询问
void SearchMeter(void)
{
	static bool excute_once_flag = true;

	if (IsPowerOff == 1 && power == 0) //自身掉电不进行搜表
	{
		if (SEARCH_METER_FLAG.meter_total == 0)
			return;
		if (excute_once_flag)
		{
			excute_once_flag = false;
		}
	}
	else //??????
	{

		SetCheckPowerOffMeterEndFlag(false);

		if (AppUseUartFlag != 0)
			return;
#if 0
		if (GetCMLPowerOnFlag())
		{
			if (SEARCH_METER_FLAG.meter_total != 0)
				check_meter_list_power_on();
			else
			{
				SetCMLPowerOnFlag(false);
				SearchMeterStart();
			}
		}
		 
#else
		if(excute_once_flag)
		{
			SearchMeterStart();
			excute_once_flag = false;
		}
#endif
		else  if (GetCheckTableFlag())
		{

			if (GetSerach645OverFlag())
			{
				#if 0 //暂时不切换波特率
				if (!getUartRate) //获取到正确的地址
				{
					ChangeLocalUartBaud_t(baudTable_t[CurrentBaud++]);
				}
				#endif
				u8 data[2] = { 0, 32 };
				u16 len = 2;
				char *p = LMP_make_frame_gd_cmd(0x21, 0xEA042102, I_STA_SEQ++, data, &len, NULL, false);
				AppLocalReceiveData(data, data, 20, 0, (u8 *)p, len, RS485SearchBack, 0, 0, 0xEA042102);
				free(p);
				SetWait485DataFlag(UART_SCAN_485);
			}
		}
		else
		{
			OS_ERR err;
			u32 Tick = OSTimeGet(&err);
			u32 overTick = 5 * 60 * 1000;
			if (SEARCH_METER_FLAG.meter_total == 0)
			{
				//未搜到表3s后再次搜索
				overTick = 5 * 1000;
			}
			if ((Tick - SEARCH_METER_FLAG.end_search_tick) > overTick) //超过一天重新搜表
			{
				SearchMeterStart();
			}
		}
	}
}
#endif
//----------------------------------------------------------------
//探测定时搜表未搜到但已经存在485列表内的电表相关操作
CHECK_NO_EXIST_METER t_CHECK_NO_EXIST_METER;

void t_CHECK_NO_EXIST_METER_init_fun(void)
{
	memset(t_CHECK_NO_EXIST_METER.is_exist_flag,false,32);
	t_CHECK_NO_EXIST_METER.meter_count = 0;
	t_CHECK_NO_EXIST_METER.check_count = 0;	
}
void t_CHECK_NO_EXIST_METER_set_fun(void)
{
	//print_debug("2.t_CHECK_NO_EXIST_METER.meter_count= ",&t_CHECK_NO_EXIST_METER.meter_count,1);
	t_CHECK_NO_EXIST_METER.is_exist_flag[t_CHECK_NO_EXIST_METER.meter_count-1]=true;
	//print_debug("t_CHECK_NO_EXIST_METER.is_exist_flag[t_CHECK_NO_EXIST_METER.meter_count-1]= ",&t_CHECK_NO_EXIST_METER.is_exist_flag[t_CHECK_NO_EXIST_METER.meter_count-1],1);
}
void table_compare(void)
{
	u8 i,j;
	if(SEARCH_METER_FLAG.meter_total != t_table_copy.meter_total)
	{
		report_meterlist_flag_set_fun(true); 
	}
	else
	{	
		for(j=0;j<SEARCH_METER_FLAG.meter_total;j++)
		{		
			for(i=0;i<t_table_copy.meter_total;i++)
			{
				if(memcmp(&t_table_copy.table[i*7],&SEARCH_METER_FLAG.table[j*7],6) != 0)
				{
					//print_debug("diffrent addr: ",&t_table_copy.table[j*7],6);							
				}
				else
				{
					//print_debug("same addr: ",&t_table_copy.table[j*7],6);
					break;
				}			
			}
			if(i == t_table_copy.meter_total)//这次搜到的表和之前搜到的所有表都不同
			{				
				report_meterlist_flag_set_fun(true); 
				break;
			}
		}
	}
}
void add_meter_to_table_fun(void)
{
	u8 i,j,k=0;
	u32 list;
	//print_debug("add meter before,table: ",table,224);
	//print_debug("add meter before,meter_total: ",&meter_total,1);
	//print_debug("add meter before,table_index: ",&table_index,1);
	//print_debug("add meter before,TableList: ",(uint8_t*)&TableList,4);
	for(i=0;i<t_no_exist_meter.meter_total;i++)
	{
		//print_debug("i= ",&i,1);
		//print_debug("t_CHECK_NO_EXIST_METER.is_exist_flag[i]= ",&t_CHECK_NO_EXIST_METER.is_exist_flag[i],1);
		if(t_CHECK_NO_EXIST_METER.is_exist_flag[i]==true)
		{
			memcpy(&SEARCH_METER_FLAG.table[SEARCH_METER_FLAG.meter_total*7+k*7],&t_no_exist_meter.table[i*7],7);
			//print_debug("table[meter_total*7+k*7]: ",&table[meter_total*7+k*7],7);  
			k++;
			list = SEARCH_METER_FLAG.TableList;
			for(j=0;j<32;j++)
			{
				if((((u8)list)&0x01) == 0x00)
				{
					SEARCH_METER_FLAG.TableList |= (1<<j);
					break;
				}
				list=list>>1;
			}			
		}
	}
	SEARCH_METER_FLAG.meter_total +=k;
	SEARCH_METER_FLAG.table_index +=k;
	//print_debug("add meter after,table: ",table,224);
	//print_debug("add meter after,meter_total: ",&meter_total,1);
	//print_debug("add meter after,table_index: ",&table_index,1);
	//print_debug("add meter after,TableList: ",(uint8_t*)&TableList,4);

	delete_wrong_meter();
	table_compare();
		
}
void check_no_exist_meter_fun(void)
{    
    u8 buf_07[sizeof(ReadPower645_07)];


	if(AppUseUartFlag != 0)
		return;

	if(t_no_exist_meter.meter_total == 0)
		return;
	if(t_CHECK_NO_EXIST_METER.check_count >= 2)
	{
		add_meter_to_table_fun();
		t_table_copy_fun();
		t_no_exist_meter_init_fun();
		SetCheckMeterEndFlag(false);//可以开启确认表类型了
		return;
	}
	if(t_CHECK_NO_EXIST_METER.meter_count >= t_no_exist_meter.meter_total)
	{
		t_CHECK_NO_EXIST_METER.meter_count = 0;
		t_CHECK_NO_EXIST_METER.check_count++;
		if(t_CHECK_NO_EXIST_METER.check_count >= 2)
		{			
			return;
		}
	}
	//print_debug("t_CHECK_NO_EXIST_METER.meter_count= ",&t_CHECK_NO_EXIST_METER.meter_count,1);
    if(t_CHECK_NO_EXIST_METER.is_exist_flag[t_CHECK_NO_EXIST_METER.meter_count] == false)//
    {
        memcpy(meter_list.address,(u8 *)&t_no_exist_meter.table[t_CHECK_NO_EXIST_METER.meter_count*7],6);        		          
        memcpy(buf_07,ReadPower645_07,sizeof(ReadPower645_07));        
        memcpy((u8 *)&buf_07[5],meter_list.address,6);//
        buf_07[sizeof(ReadPower645_07)-2]=GetCheckSum(buf_07+4,sizeof(ReadPower645_07)-4-2);      
		meter_list.Type.Statute = (t_no_exist_meter.table[t_CHECK_NO_EXIST_METER.meter_count*7+6]&0x03);//20170926
        if(meter_list.Type.Statute == 0x02)//07
        {
			{
			u8 addr[6] = {0};
			AppLocalReceiveData( addr, addr, SEARCH_METER_1500MS, 0, buf_07, sizeof(ReadPower645_07), RS485DataDeal, 0, 0, BAUD_SEARCH );
     
			}
			SetWait485DataFlag(UART_CHECK_NO_EXIST_METER_FRAME); 
        }
		
    } 
	
	t_CHECK_NO_EXIST_METER.meter_count++;
}

//----------------------------------------------------------------
//--------------------------------------------------------------------
//??IsMeterInTable
//??:???CCO?????,???645????????????????
//?????,?????,?????
//--------------------------------------------------------------------
bool IsMeterInTable(u8 *addr)
{
#ifndef NO_FILTER
	u8 i;

    for(i=0;i<METER_NUM_MAX;i++)
    {
        if(memcmp(addr,&SEARCH_METER_FLAG.table[i*7],LONG_ADDR_LEN) == 0 
			|| memcmp(addr,&t_table_copy.table[i*7],LONG_ADDR_LEN)  == 0 )
        {            
            return true;
        }
    } 
    return false;
#else
    return true;
#endif
}
//----------------------------------------------------------------
void BindCollectorAddr(void)
{
	u8 addr_temp[6];
	if(!IsBindAddr && SEARCH_METER_FLAG.meter_total==0 && !GetCheckTableFlag())
	{
		memcpy(addr_temp,collector_addr,6);
		TransposeAddr(addr_temp);
		SetMACType(1);
		SetStaMAC(addr_temp);
		IsBindAddr = true;
		report_meterlist_flag_set_fun(true);
	}
    
}
#endif
//----------------------------------------------------------------
//add by LiHuaQiang 2020.10.8  -END-
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------

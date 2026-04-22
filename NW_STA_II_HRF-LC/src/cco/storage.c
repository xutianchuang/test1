
#include "system_inc.h"
#include "manager_link.h"
#include "alert.h"
#include "storage.h"
#include "RTC.h"
#include "plc_autorelay.h"
#include "PLM_2005.H"
#include "App_cfg.h"
#include "Hplc_data.h"
#include "hrf_port.h"
ST_CCO_ID_TYPE g_cco_id;
//#include <string.h>

//#define FRAM_STATUS_OK                    0x86235628
#ifdef HPLC_CSG
#define DATAFLASH_STATUS_OK             0x128588ab
#else
    #ifdef GDW_2019_GRAPH_STOREDATA
    #define DATAFLASH_STATUS_OK             0x16566dac
    #else
    #define DATAFLASH_STATUS_OK             0x13968dcb
    #endif
#endif
#define CHIP_ID_WRITE_FLAG              0x55669635
#define VER_INFO_WRITE_FLAG             0x12345678
#define CHIP_ID_WRITE_NEW_FLAG          0x45465899
#define VER_INFO_WRITE_NEW_FLAG         0x85645055
#define MIRROR_TO_FRAM_BASE_ADDRESS     SRAM_BASE_ADDR

extern GLOBAL_CFG_PARA g_PlmConfigPara;
u8 g_MeterAddrSuccess=0;
//extern __far CCTT_MNG_METER  MeterDataPoolMem[];

const DB_ADDR_ITEM gc_dbAddrArray[] =
{
    /***************************************************************************
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    格式: 保存项目， {首地址，长度}，如果修改此数组，要保证不同数据之间不能越界
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ****************************************************************************/

    { DFLASH_SYSTEM_CFG_ADDR, sizeof(GLOBAL_CFG_PARA), DFLASH_SYSTEM_CFG_SIZE },
    { METER_RELAY_INFO_ADDR, CCTT_METER_NUMBER*sizeof(RELAY_INFO)+4, METER_RELAY_INFO_SIZE },
	#if STA_INFO_SIZE
    { STA_INFO_ADDR, CCTT_METER_NUMBER*sizeof(STA_INFO)+4, STA_INFO_SIZE },
	#endif
#ifdef PROTOCOL_GD_2016
    { METER_COLL_DB_ADDR, CCTT_METER_COLL_NUMBER*sizeof(METER_COLL_MAP)+4, METER_COLL_DB_SIZE },
    { X4_READ_TASK_INFO_ADDR, CCTT_X4_READ_TASK_INFO_NUMBER*sizeof(X4_READ_TASK_RECORD)+4, X4_READ_TASK_INFO_SIZE },
#endif
};

unsigned char dc_flush_fram(void *ptr, U32 len, U8 action )
{
    //UCHAR iRet = FALSE;

    unsigned long base_addr = (unsigned long)(&g_PlmConfigPara);

    unsigned long start_addr    = (unsigned long)ptr;
    unsigned long end_addr  = (unsigned long)ptr + len;

    unsigned long start_offset = 0x0, end_offset = 0x0;

    if( start_addr < base_addr )
    {
        //Alert(ALERT_UNEX_VALUE, ALERT_NO_ACTION, __FILE__, __LINE__);
        return FALSE;
    }

    start_offset = start_addr - base_addr;
    if( start_offset > sizeof(GLOBAL_CFG_PARA))
    {
        //Alert(ALERT_UNEX_VALUE, ALERT_NO_ACTION, __FILE__, __LINE__);
        return FALSE;
    }

    end_offset = end_addr - base_addr;
    if( end_offset > sizeof(GLOBAL_CFG_PARA) )
    {
        //Alert(ALERT_UNEX_VALUE, ALERT_NO_ACTION, __FILE__, __LINE__);
        return FALSE;
    }

    if(action == FRAM_FLUSH )
        return data_flash_write_straight(DFLASH_SYSTEM_CFG_ADDR + (ULONG)start_offset, ptr, (ULONG)len );
    else if(action == FRAM_FETCH )
        return data_flash_read_straight(DFLASH_SYSTEM_CFG_ADDR + (ULONG)start_offset, ptr, (ULONG)len );
    else
        return FALSE;
}

/* 输入:  Hex ULONG /输出: 返回BCD LONG */
unsigned long Hex2BcdLong(unsigned long hex)
{
    unsigned long temp;
    unsigned long ret;
    unsigned char ii;

    ret=0;

    for(ii=0; ii<8; ii++)
    {
        ret>>=4;

        // 取得余数
        temp=hex%10;

        //向高位移位28 bit (32 bits - 4 bits).
        temp<<=28;

        // 累加.
        ret|=temp;

        // 取整
        hex=hex/10;
    }
    return(ret);
}

#define g_cst_serialnumber  (*((ULONG *)(0x00C4UL)))  // OPTION_BYTE DEBUG SECURITY

unsigned long SYS_get_serial_no()
{
    unsigned long serialno;

    serialno = Hex2BcdLong(g_cst_serialnumber);
    return serialno;
}

ST_CODE_INFO_TYPE DefaultCodeInfo=
{
 	#ifdef ZB204_CHIP
	.Chip = { "ZB204" },
	#elif defined(ZB205_CHIP)
	.Chip = { "ZB205" },
	#elif defined(ZB206_CHIP)
	.Chip = { "ZB206" },
	#else
	.Chip = { "" },
	#endif


	.Factory = { FACTORY_CODE[0]&0xFF, FACTORY_CODE[1]&0xFF },
	.Factory_code = FACTORY_CODE_FLAG,
	.AreaProtocol = 0,
};

//FRAM上电后第一次初始化, 需要设置一些初始参数
//全部初始化：清配置信息，节点信息，STA信息，采集器表计映射表
//参数格式化：清配置信息，节点信息，STA信息，采集器表计映射表
//数据格式化：无动作
void STOR_Dataflash_SYS_init(USHORT clear_type)
{
	memset_special((UCHAR *)&g_PlmConfigPara, 0, sizeof(GLOBAL_CFG_PARA));

	g_PlmConfigPara.dataflash_status = DATAFLASH_STATUS_OK;
	memset_special(g_PlmConfigPara.main_node_addr, 0x11, sizeof(g_PlmConfigPara.main_node_addr));
	g_PlmConfigPara.net_id = rand_hard_range(1, NID_MAX);
	//g_PlmConfigPara->net_id = 0x776821;
	g_PlmConfigPara.isNeedChangeNid = 0;
	
	g_PlmConfigPara.sysEraseStackCount++;
    

    if( (MSG_STOR_PARA_CLEAR == clear_type)
        ||(MSG_STOR_ALL_CLEAR == clear_type))
    {
        if(MSG_STOR_ALL_CLEAR == clear_type)
        {
			data_flash_erase_straight_with_poweron(DFLASH_SYSTEM_CFG_ADDR, DFLASH_SYSTEM_CFG_SIZE);
			data_flash_write_straight(DFLASH_SYSTEM_CFG_ADDR, (UCHAR *)&g_PlmConfigPara, sizeof(GLOBAL_CFG_PARA));
        }
        else if(MSG_STOR_PARA_CLEAR == clear_type)
        {
            g_PlmConfigPara.sysEraseStackCount ++;
            dc_flush_fram(&g_PlmConfigPara.sysEraseStackCount, sizeof(g_PlmConfigPara.sysEraseStackCount), FRAM_FLUSH);
        }

        memset_special(g_pMeterRelayInfo, 0xff, CCTT_METER_NUMBER*sizeof(RELAY_INFO));
        memset_special(g_pStaInfo, 0xff, TEI_CNT*sizeof(STA_INFO));
    #ifdef PROTOCOL_GD_2016
        data_flash_erase_straight_with_poweron(METER_COLL_DB_ADDR,CCTT_METER_COLL_NUMBER * sizeof(METER_COLL_MAP));
    #endif
		FeedWdg();
		data_flash_erase_straight_with_poweron(METER_RELAY_INFO_ADDR, METER_RELAY_INFO_SIZE*2);
		FeedWdg();
		#if STA_INFO_SIZE
        data_flash_erase_straight_with_poweron( STA_INFO_ADDR, STA_INFO_SIZE ); 
		FeedWdg();       
		#endif
    #ifdef PROTOCOL_GD_2016
        data_flash_erase_straight_with_poweron( METER_COLL_DB_ADDR, METER_COLL_DB_SIZE );
		FeedWdg();
    #endif

    }
    else if(MSG_STOR_DATA_CLEAR == clear_type)
    {
        data_flash_erase_straight_with_poweron(DFLASH_SYSTEM_CFG_ADDR, DFLASH_SYSTEM_CFG_SIZE);
		data_flash_write_straight(DFLASH_SYSTEM_CFG_ADDR, (UCHAR *)&g_PlmConfigPara, sizeof(GLOBAL_CFG_PARA));
    }
}

void STOR_clear_alert()
{
#if 0
    unsigned short ii;

    alert_sn = 0;
    alert_num = 0;

    memset_special((UCHAR *)&g_alert_buf[0], 0, (USHORT)(ALERT_BUFFER_SIZE)*sizeof(ALERT_INFO));

    for(ii = 0; ii < ALERT_BUFFER_SIZE; ii++)
    {
        g_alert_buf[ii].alert_type = ALERT_NO_ALERT;
        g_alert_buf[ii].alert_flag = ALERT_NO_ALERT;
    }
#endif
}

void SYS_STOR_DEBUGMASK(u32 mask)
{
	for (int i=0;i<2;i++)
	{
		for (int j=0;j<3;j++)
		{
			u32 temp;
			data_flash_write_straight_with_crc(DEBUGMASK_ADDR + DATA_FLASH_SECTION_SIZE * i, (u8 *)&mask, 4);
			data_flash_read_straight_with_crc_noadc(DEBUGMASK_ADDR + DATA_FLASH_SECTION_SIZE * i, (u8 *)&temp, 4, 0xff);
			if (temp == mask)
			{
				break;
			}
		}
	}
}

void SYS_READ_DEBUGMASK(void)
{
	u32 tempmask = 0;
	char read_success;
	for (int i=0; i<3; i++)
	{
		if (read_success = data_flash_read_straight_with_crc(DEBUGMASK_ADDR, (u8 *)&tempmask, 4, 0xff))
		{
			DebugSetMask(tempmask);
			break;
		}
	}
	if (read_success==false)
	{
		for (int i=0; i<3; i++)
		{
			read_success = data_flash_read_straight_with_crc(DEBUGMASK_ADDR + DATA_FLASH_SECTION_SIZE, (u8 *)&tempmask, 4, 0xff);
			if (read_success)
			{
				DebugSetMask(tempmask);
				break;
			}
		}
		if (read_success) 
		{
			//回写原始区域
			for (int j = 0; j < 3; j++)
			{
				data_flash_write_straight_with_crc(DEBUGMASK_ADDR , (u8 *)&tempmask, 4);
				u32 temp;
				data_flash_read_straight_with_crc_noadc(DEBUGMASK_ADDR, (u8 *)&temp, 4, 0xff);
				if (temp == tempmask)
				{
					break;
				}
			}
		}
	}
}

void SYS_STOR_FREQ(u8 freq)
{
	if (freq>3)
	{
		Reboot_system_now(__func__);
	}
	for (int i=0;i<2;i++)
	{
		for (int j=0;j<3;j++)
		{
			u8 temp;
			data_flash_write_straight_with_crc(FREQUENCE_FLASH_ADDE+DATA_FLASH_SECTION_SIZE*i, &freq, 1);
			data_flash_read_straight_with_crc_noadc(FREQUENCE_FLASH_ADDE+DATA_FLASH_SECTION_SIZE*i, &temp, 1,0xff);
			if (temp==freq)
			{
				break;
			}
		}
	}
}
void SYS_READ_FREQ(void)
{
	u8 freq;
	for (int i=0;i<3;i++)
	{
		if(data_flash_read_straight_with_crc(FREQUENCE_FLASH_ADDE, &freq, 1, 0xff))
		{
			break;
		}
	}
	if (freq > 3)
	{
		char read_success;
		for (int i=0;i<3;i++)
		{
			read_success=data_flash_read_straight_with_crc(FREQUENCE_FLASH_ADDE + DATA_FLASH_SECTION_SIZE, &freq, 1, 0xff);
			if (read_success)
			{
				break;
			}
		}
		if (read_success)
		{
			//回写原始区域
			for (int j = 0; j < 3; j++)
			{
				u8 temp;
				data_flash_write_straight_with_crc(FREQUENCE_FLASH_ADDE, &freq, 1);
				data_flash_read_straight_with_crc_noadc(FREQUENCE_FLASH_ADDE, &temp, 1, 0xff);
				if (temp == freq)
				{
					break;
				}
			}
		}
	}
	if (freq > 3)
	{
#ifdef HPLC_CSG
		g_PlmConfigPara.hplc_comm_param = 2;
#else
	#if defined(GDW_CHONGQING)|| defined(GDW_BEIJING)
		g_PlmConfigPara.hplc_comm_param = 1;
	#else
		g_PlmConfigPara.hplc_comm_param = 2;
	#endif
#endif
	}
	else
	{
		g_PlmConfigPara.hplc_comm_param = freq;
	}
}

void SYS_STOR_MAIN_NODE(u8 *mac)
{
	for (int i=0;i<2;i++)
	{
		for (int j=0;j<3;j++)
		{
			data_flash_write_straight_with_crc(MAIN_NODE_FLASH_ADDE+i*DATA_FLASH_SECTION_SIZE, mac, LONG_ADDR_LEN);
			u8 temp[LONG_ADDR_LEN];
			data_flash_read_straight_with_crc_noadc(MAIN_NODE_FLASH_ADDE+DATA_FLASH_SECTION_SIZE*i, temp, LONG_ADDR_LEN,0xff);
			if (macCmp(temp, mac))
			{
				break;
			}
		}
	}
}
void SYS_READ_MAIN_NODE(void)
{
	data_flash_read_straight_with_crc(MAIN_NODE_FLASH_ADDE, g_PlmConfigPara.main_node_addr, LONG_ADDR_LEN,0xce);
	if (memIsHex(g_PlmConfigPara.main_node_addr,0xce, LONG_ADDR_LEN))
	{
		data_flash_read_straight_with_crc(MAIN_NODE_FLASH_ADDE+DATA_FLASH_SECTION_SIZE, g_PlmConfigPara.main_node_addr, LONG_ADDR_LEN,0xce);
	}
}
void SYS_STOR_WHITE_LIST_SWITCH(u8 en)
{
	for (int i=0;i<2;i++)
	{
		for (int j=0;j<3;j++)
		{
			data_flash_write_straight_with_crc(WHITE_LIST_FLASH_ADDE+i*DATA_FLASH_SECTION_SIZE, &en, 1);
			u8 temp;
			data_flash_read_straight_with_crc_noadc(WHITE_LIST_FLASH_ADDE+i*DATA_FLASH_SECTION_SIZE, &temp, 1,0xff);
			if (temp==en)
			{
				break;
			}
		}
	}
}
void SYS_READ_WHITE_LIST_SWITCH(void)
{
	for (int i=0;i<3;i++)
	{
		if(data_flash_read_straight_with_crc(WHITE_LIST_FLASH_ADDE, &g_PlmConfigPara.close_white_list, 1, 0xff))
		{
			break;
		}
	}
	if (g_PlmConfigPara.close_white_list==0xff)
	{
		char read_success;
		for (int i=0;i<3;i++)
		{
			read_success=data_flash_read_straight_with_crc(WHITE_LIST_FLASH_ADDE + DATA_FLASH_SECTION_SIZE, &g_PlmConfigPara.close_white_list, 1, 0xff);
			if (read_success)
			{
				break;
			}
		}
		if (read_success) 
		{
			//回写原始区域
			for (int j = 0; j < 3; j++)
			{
				data_flash_write_straight_with_crc(WHITE_LIST_FLASH_ADDE , &g_PlmConfigPara.close_white_list, 1);
				u8 temp;
				data_flash_read_straight_with_crc_noadc(WHITE_LIST_FLASH_ADDE, &temp, 1,0xff);
				if (temp == g_PlmConfigPara.close_white_list)
				{
					break;
				}
			}
		}
	}
	if (g_PlmConfigPara.close_white_list==0xff)
	{
#if defined(OVERSEA) || defined(GDW_ZHEJIANG)
		g_PlmConfigPara.close_white_list=2; //关闭白名单且档案不写入flash
#else
		g_PlmConfigPara.close_white_list=0;
#endif
	}
}

#ifdef GDW_ZHEJIANG

void SYS_STOR_BLACK_METER(void)
{
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
	for (int i=0;i<2;i++)
	{
		for (int j=0;j<3;j++)
		{
			FeedWdg();
			data_flash_write_straight_with_crc(METER_REFUSE_INFO_ADDR+i*METER_REFUSE_INFO_SIZE, (u8 *)g_BlackMeterInfo, CCTT_BLACK_METER_NUMBER * sizeof(BLACK_RELAY_INFO));
			u32 crc=CommonGetCrc32((u8*)(FLASH_AHB_ADDR+METER_REFUSE_INFO_ADDR+i*METER_REFUSE_INFO_SIZE), CCTT_BLACK_METER_NUMBER * sizeof(BLACK_RELAY_INFO));
			if (crc==*(u32*)(FLASH_AHB_ADDR+METER_REFUSE_INFO_ADDR+i*METER_REFUSE_INFO_SIZE+CCTT_BLACK_METER_NUMBER * sizeof(BLACK_RELAY_INFO)))
			{
				break;
			}
		}
	}
	CPU_CRITICAL_EXIT();
}

bool SYS_READ_BLACK_METER(void)
{
	char read_success;
	for (int i=0;i<3;i++)
	{
		FeedWdg();
		read_success=data_flash_read_straight_with_crc(METER_REFUSE_INFO_ADDR, (u8 *)g_BlackMeterInfo, CCTT_BLACK_METER_NUMBER * sizeof(BLACK_RELAY_INFO), 0xff);
		if (read_success)
		{
			break;
		}
	}
	if (!read_success)
	{
		for (int i = 0; i < 3; i++)
		{
			FeedWdg();
			read_success = data_flash_read_straight_with_crc(METER_REFUSE_INFO_ADDR + METER_REFUSE_INFO_SIZE, (u8 *)g_BlackMeterInfo, CCTT_BLACK_METER_NUMBER * sizeof(BLACK_RELAY_INFO), 0xff);
			if (read_success)
			{
				break;
			}
		}
		if (read_success)
		{
			int i;
			for (i = 0; i < 3; i++)
			{
				FeedWdg();
				//把备份区内容写回原始区域
				data_flash_write_straight_with_crc(METER_REFUSE_INFO_ADDR, (u8 *)g_BlackMeterInfo, CCTT_BLACK_METER_NUMBER * sizeof(BLACK_RELAY_INFO));
				if (data_flash_read_straight_with_crc(METER_REFUSE_INFO_ADDR, (u8 *)g_BlackMeterInfo, CCTT_BLACK_METER_NUMBER * sizeof(BLACK_RELAY_INFO), 0xff))
				{
					break;
				}
				else
				{
					if (!data_flash_read_straight_with_crc(METER_REFUSE_INFO_ADDR + METER_REFUSE_INFO_SIZE, (u8 *)g_BlackMeterInfo, CCTT_BLACK_METER_NUMBER * sizeof(BLACK_RELAY_INFO), 0xff))
					{ //再次读取备份区错误
						Reboot_system_now(__func__);
					}
				}

			}
			if (i >= 3) //原始区域未写入成功 复位
			{
				Reboot_system_now(__func__);
			}
		}
		else //备份区域也出错 返回错误
		{
			return false;
		}
	}
	return true;
}
#endif //GDW_ZHEJIANG

void SYS_STOR_METER(void)
{
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
	for (int i=0;i<2;i++)
	{
		for (int j=0;j<3;j++)
		{
			FeedWdg();
			data_flash_write_straight_with_crc(METER_RELAY_INFO_ADDR+i*METER_RELAY_INFO_SIZE, (u8 *)g_pMeterRelayInfo, CCTT_METER_NUMBER * sizeof(RELAY_INFO));
			u32 crc=CommonGetCrc32((u8*)(FLASH_AHB_ADDR+METER_RELAY_INFO_ADDR+i*METER_RELAY_INFO_SIZE), CCTT_METER_NUMBER * sizeof(RELAY_INFO));
			if (crc==*(u32*)(FLASH_AHB_ADDR+METER_RELAY_INFO_ADDR+i*METER_RELAY_INFO_SIZE+CCTT_METER_NUMBER * sizeof(RELAY_INFO)))
			{
				break;
			}
		}
	}
	CPU_CRITICAL_EXIT();
}

bool SYS_READ_METER(void)
{
	char read_success;
	for (int i=0;i<3;i++)
	{
		FeedWdg();
		read_success=data_flash_read_straight_with_crc(METER_RELAY_INFO_ADDR, (u8 *)g_pMeterRelayInfo, CCTT_METER_NUMBER * sizeof(RELAY_INFO), 0xff);
		if (read_success)
		{
			break;
		}
	}
	if (!read_success)
	{
		for (int i = 0; i < 3; i++)
		{
			FeedWdg();
			read_success = data_flash_read_straight_with_crc(METER_RELAY_INFO_ADDR + METER_RELAY_INFO_SIZE, (u8 *)g_pMeterRelayInfo, CCTT_METER_NUMBER * sizeof(RELAY_INFO), 0xff);
			if (read_success)
			{
				break;
			}
		}
		if (read_success)
		{
			int i;
			for (i = 0; i < 3; i++)
			{
				FeedWdg();
				//把备份区内容写回原始区域
				data_flash_write_straight_with_crc(METER_RELAY_INFO_ADDR, (u8 *)g_pMeterRelayInfo, CCTT_METER_NUMBER * sizeof(RELAY_INFO));
				if (data_flash_read_straight_with_crc(METER_RELAY_INFO_ADDR, (u8 *)g_pMeterRelayInfo, CCTT_METER_NUMBER * sizeof(RELAY_INFO), 0xff))
				{
					break;
				}
				else
				{
					if (!data_flash_read_straight_with_crc(METER_RELAY_INFO_ADDR + METER_RELAY_INFO_SIZE, (u8 *)g_pMeterRelayInfo, CCTT_METER_NUMBER * sizeof(RELAY_INFO), 0xff))
					{ //再次读取备份区错误
						Reboot_system_now(__func__);
					}
				}

			}
			if (i >= 3) //原始区域未写入成功 复位
			{
				Reboot_system_now(__func__);
			}
		}
		else //备份区域也出错 返回错误
		{
			return false;
		}
	}
	return true;
}
//存储参数一般参数
void sys_normal_para_stor(u32 flash_addr,void *data, u8 len)
{
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
	for (int i=0;i<2;i++)
	{
		for (int j=0;j<3;j++)
		{
			FeedWdg();
			data_flash_write_straight_with_crc(flash_addr+i*DATA_FLASH_SECTION_SIZE, data,len);
			u32 crc=CommonGetCrc32((u8*)(FLASH_AHB_ADDR+flash_addr+i*DATA_FLASH_SECTION_SIZE), len);
			if (crc==*(u32*)(FLASH_AHB_ADDR+flash_addr+i*DATA_FLASH_SECTION_SIZE+len))
			{
				break;
			}
		}
	}
	CPU_CRITICAL_EXIT();
}
bool sys_read_normal_para(u32 flash_addr,void *data, u8 len,void *defult_data)
{
	char read_success;
	for (int i=0;i<3;i++)
	{
		FeedWdg();
		read_success=data_flash_read_straight_with_crc(flash_addr, data, len, 0xff);
		if (read_success)
		{
			break;
		}
	}
	if (!read_success)
	{
		for (int i = 0; i < 3; i++)
		{
			FeedWdg();
			read_success = data_flash_read_straight_with_crc(flash_addr + DATA_FLASH_SECTION_SIZE, data, len, 0xff);
			if (read_success)
			{
				break;
			}
		}
		if (read_success)
		{
			int i;
			for (i = 0; i < 3; i++)
			{
				FeedWdg();
				//把备份区内容写回原始区域
				data_flash_write_straight_with_crc(flash_addr, data, len);
				if (data_flash_read_straight_with_crc(flash_addr, data, len, 0xff))
				{
					break;
				}
				else
				{
					if (!data_flash_read_straight_with_crc(flash_addr + DATA_FLASH_SECTION_SIZE, data, len, 0xff))
					{ //再次读取备份区错误
						Reboot_system_now(__func__);
					}
				}

			}
			if (i >= 3) //原始区域未写入成功 复位
			{
				Reboot_system_now(__func__);
			}
		}
		else //备份区域也出错 返回错误
		{
			memcpy(data,defult_data,len);
			return false;
		}
	}
	return true;
}

void SYS_STOR_BAUDRATE(u32 baudrate)
{
	sys_normal_para_stor(BAUD_RATE_FLASH_ADDR,&baudrate,sizeof(baudrate));
}
void SYS_READ_BAUDRATE(void)
{
	u32 defult_baudrate=0xFFFF;
	sys_read_normal_para(BAUD_RATE_FLASH_ADDR,&g_PlmConfigPara.baud_rate,sizeof(g_PlmConfigPara.baud_rate),&defult_baudrate);
    if ((g_PlmConfigPara.baud_rate != 115200)&&(g_PlmConfigPara.baud_rate != 9600))//没有默认的波特率
    {
#ifdef GDW_JIANGSU
			g_PlmConfigPara.baud_rate=115200;
#else	
	  		g_PlmConfigPara.baud_rate=9600;
#endif		
    }
}

void SYS_STOR_HRF_POWER_COMP(void)
{
	sys_normal_para_stor(HRF_POEWE_COMP_ADDR,&g_PlmConfigPara.HRF_TXChlPowerComp[0],sizeof(g_PlmConfigPara.HRF_TXChlPowerComp));
}

void SYS_READ_HRF_POWER_COMP(void)
{
    unsigned char defult_power[2]={0,0};
	sys_read_normal_para(HRF_POEWE_COMP_ADDR,&g_PlmConfigPara.HRF_TXChlPowerComp[0],sizeof(g_PlmConfigPara.HRF_TXChlPowerComp),&defult_power[0]);
}

void SYS_STRO_PLL(signed int trim)
{
#ifdef YD_CCO
    if ((trim > 960) || (trim < -960)) //trim单位1/16ppm，正负60ppm
#else
     if ((trim > 400) || (trim < -400)) //trim单位1/16ppm，正负25ppm
#endif
	{
	    return;
    }
    g_PlmConfigPara.PLL_Vaild=1;
    g_PlmConfigPara.PLL_Trimming=trim;
    sys_normal_para_stor(PLL_FLASH_ADDE,&g_PlmConfigPara.PLL_Trimming,sizeof(g_PlmConfigPara.PLL_Trimming));
}
void SYS_READ_PLL(void)
{

    s32  defule_pll=-10000;

	sys_read_normal_para(PLL_FLASH_ADDE,&g_PlmConfigPara.PLL_Trimming,sizeof(g_PlmConfigPara.PLL_Trimming),&defule_pll);
#ifdef YD_CCO    
    if (g_PlmConfigPara.PLL_Trimming>960||g_PlmConfigPara.PLL_Trimming<-960) //PLL_Trimming单位1/16ppm，正负60ppm
#else
    if (g_PlmConfigPara.PLL_Trimming>400||g_PlmConfigPara.PLL_Trimming<-400) //PLL_Trimming单位1/16ppm，正负25ppm
#endif
	{
		g_PlmConfigPara.PLL_Trimming=0;
		g_PlmConfigPara.PLL_Vaild=0;
		return;
	}
    g_PlmConfigPara.PLL_Vaild=1;
}
void SYS_STOR_AUTHEN(void)
{
	sys_normal_para_stor(AUTHEN_FLASH_ADDE,&g_PlmConfigPara.YX_mode,sizeof(g_PlmConfigPara.YX_mode));
}
void SYS_READ_AUTHEN(void)
{
	char defule_mode='Y';
	sys_read_normal_para(AUTHEN_FLASH_ADDE,&g_PlmConfigPara.YX_mode,sizeof(g_PlmConfigPara.YX_mode),&defule_mode);
}


void SYS_STOR_HRF_CHANNEL(void)
{
	sys_normal_para_stor(HRF_CHANNEL_FLASH_ADDE,&g_PlmConfigPara.hrf_option,3);
}

void SYS_READ_HRF_CHANNEL(void)
{
	u32 defule_channel=0xFFFF;
	sys_read_normal_para(HRF_CHANNEL_FLASH_ADDE,&g_PlmConfigPara.hrf_option,3,&defule_channel);
    if (((g_PlmConfigPara.hrf_option | g_PlmConfigPara.hrf_channel<<8)==0xffff)||(g_PlmConfigPara.hrf_option<=0)||(g_PlmConfigPara.hrf_option>3))//没有默认的频段
    {
        u16 hrfindex = rand_hard_range(0, 2*20-1);
        g_PlmConfigPara.hrf_option=hrfindex/20+2;
        hrfindex=hrfindex%20;
		g_PlmConfigPara.hrf_channel=BatterHrfChannel[g_PlmConfigPara.hrf_option-1][hrfindex];
		g_PlmConfigPara.hrf_allowChangeChannel=1;
        SYS_STOR_HRF_CHANNEL();
    }
}

void SYS_STOR_A_PHASE_ERR(void)
{
	#ifdef STA_A_PHASE_ERR_ADDR
	sys_normal_para_stor(STA_A_PHASE_ERR_ADDR,&g_PlmConfigPara.a_phase_error,sizeof(g_PlmConfigPara.a_phase_error));
	#endif
}
void SYS_READ_A_PHASE_ERR(void)
{
	#ifdef STA_A_PHASE_ERR_ADDR
	u32 defult_flag=0;
	sys_read_normal_para(STA_A_PHASE_ERR_ADDR,&g_PlmConfigPara.a_phase_error,sizeof(g_PlmConfigPara.a_phase_error),&defult_flag);
	#else
	g_PlmConfigPara.a_phase_error=0;
	#endif
}

#ifdef GDW_JIANGXI
void SYS_STOR_NETWORK_MODE(void)
{
	#ifdef NETWORK_MODE_ADDR
	sys_normal_para_stor(NETWORK_MODE_ADDR,&g_PlmConfigPara.network_mode,sizeof(g_PlmConfigPara.network_mode));
	#endif
}
void SYS_READ_NETWORK_MODE(void)
{
	#ifdef NETWORK_MODE_ADDR
	u8 defult_flag = NETWORK_MODE_MIX;//默认就先暂时0-双模混合
	sys_read_normal_para(NETWORK_MODE_ADDR,&g_PlmConfigPara.network_mode,sizeof(g_PlmConfigPara.network_mode),&defult_flag);
	#else
	g_PlmConfigPara.network_mode = NETWORK_MODE_MIX;
	#endif
}
#endif



//中继地址范围
void SYS_STOR_RELAY_RANGE(void)
{
	sys_normal_para_stor(RELAY_RANGE_ADDR, (u8 *)&g_PlmConfigPara.relay_addr_start, 4);
}

void SYS_READ_RELAY_RANGE(void)
{
	u32 defule_value = 0xFFFFFFFF;
	sys_read_normal_para(RELAY_RANGE_ADDR, (u8 *)&g_PlmConfigPara.relay_addr_start, 4, &defule_value);

	if (g_PlmConfigPara.relay_addr_start == 0xFFFF)
	{
		//默认范围
		g_PlmConfigPara.relay_addr_start = 0x0001;
		g_PlmConfigPara.relay_addr_end = 0xFFFC;
	}
}

void SYS_STOR_COLLECT_INNET_MODE(u8 mode)
{
	for (int i=0; i<2; i++)
	{
		for (int j=0; j<3; j++)
		{
			data_flash_write_straight_with_crc(COLLECT_INNET_MODE_ADDR+i*DATA_FLASH_SECTION_SIZE, &mode, 1);
			u8 temp;
			data_flash_read_straight_with_crc_noadc(COLLECT_INNET_MODE_ADDR+i*DATA_FLASH_SECTION_SIZE, &temp, 1, 0xff);
			if (temp==mode)
			{
				break;
			}
		}
	}
}

void SYS_READ_COLLECT_INNET_MODE(void)
{
	for (int i=0;i<3;i++)
	{
		if(data_flash_read_straight_with_crc(COLLECT_INNET_MODE_ADDR, &g_PlmConfigPara.allow_collect_innet, 1, 0xff))
		{
			break;
		}
	}
    
	if (g_PlmConfigPara.allow_collect_innet == 0xff)
	{
		char read_success;
		for (int i=0;i<3;i++)
		{
			read_success=data_flash_read_straight_with_crc(COLLECT_INNET_MODE_ADDR + DATA_FLASH_SECTION_SIZE, &g_PlmConfigPara.allow_collect_innet, 1, 0xff);
			if (read_success)
			{
				break;
			}
		}
		if (read_success) 
		{
			//回写原始区域
			for (int j = 0; j < 3; j++)
			{
				data_flash_write_straight_with_crc(COLLECT_INNET_MODE_ADDR , &g_PlmConfigPara.allow_collect_innet, 1);
				u8 temp;
				data_flash_read_straight_with_crc_noadc(COLLECT_INNET_MODE_ADDR, &temp, 1, 0xff);
				if (temp == g_PlmConfigPara.allow_collect_innet)
				{
					break;
				}
			}
		}
	}
    
	if (g_PlmConfigPara.allow_collect_innet == 0xff)
	{
		g_PlmConfigPara.allow_collect_innet = 0; //默认不允许采集器自身地址入网（不在档案中）
	}
}

void SYS_STOR_COMPARE_STA_INVER(void)
{
	data_flash_write_straight_with_crc(COMPARE_STA_INVER_ADDR, (u8 *)&g_PlmConfigPara.sta_inner_ver, sizeof(g_PlmConfigPara.sta_inner_ver));
}
void SYS_READ_COMPARE_STA_INVER(void)
{
	data_flash_read_straight_with_crc(COMPARE_STA_INVER_ADDR, (u8*)&g_PlmConfigPara.sta_inner_ver, sizeof(g_PlmConfigPara.sta_inner_ver), 0);
}

/*MCU 1166 16 bits problem:
operation_type = g_MeterDataPool[meterno].operationType;

{
AX = 0x9F82
ES:!0D9EAH = 0x8A260
}

ADDW  AX,ES:!0D9EAH

{
AX = 0x41E2

; ES 没有进位到：0x9(41E2)，导致数据回写到：0x8(41E2)
}
*/
unsigned char STOR_Dataflash_init()
{   
    unsigned char ii = 0;

    unsigned short db_number = sizeof(gc_dbAddrArray)/sizeof(DB_ADDR_ITEM);

    for(ii = 0; ii < db_number; ii++)
    {
        if(gc_dbAddrArray[ii].usedSize > gc_dbAddrArray[ii].allocSize)
        {
            //Alert(ALERT_DB_INIT_FAIL, ALERT_NO_ACTION, __FILE__, __LINE__);
            return ERROR;
        }

        if((ii > 0 ))
        {
            if((gc_dbAddrArray[ii-1].addr + gc_dbAddrArray[ii-1].allocSize) > gc_dbAddrArray[ii].addr )
            {
                //Alert(ALERT_DB_INIT_FAIL, ALERT_NO_ACTION, __FILE__, __LINE__);
                return ERROR;
            }
        }
    }

    if(DATAFLASH_END_ADDR < DATAFLASH_USED_END_ADDR)
    {
        //Alert(ALERT_DB_INIT_FAIL, ALERT_NO_ACTION, __FILE__, __LINE__);
        return ERROR;
    }

    for(ii=0; ii<8; ii++)
    {
		OS_ERR err;
        g_PlmConfigPara.dataflash_status = 0;
        dc_flush_fram(&g_PlmConfigPara.dataflash_status, sizeof(g_PlmConfigPara.dataflash_status), FRAM_FETCH);

        if(g_PlmConfigPara.dataflash_status == DATAFLASH_STATUS_OK)
            break;
		if (!BootPowerState)
		{
			Reboot_system_now(__func__);
    }
		OSTimeDly(500, OS_OPT_TIME_DLY, &err);
		FeedWdg();
    }

    if(g_PlmConfigPara.dataflash_status != DATAFLASH_STATUS_OK)
    {
		if (!BootPowerState)
		{
			Reboot_system_now(__func__);
		}
		STOR_clear_alert();
		memset_special(&g_PlmConfigPara, 0, sizeof(g_PlmConfigPara));
        STOR_Dataflash_SYS_init(MSG_STOR_DATA_CLEAR);


#if 0
		data_flash_read_straight( STA_INFO_ADDR, (u8*)g_pStaInfo, TEI_CNT*sizeof(STA_INFO));
#else
		memset(g_pStaInfo, 0xff, TEI_CNT * sizeof(STA_INFO));
#endif

#ifdef PROTOCOL_GD_2016
		data_flash_erase_straight_with_poweron(METER_COLL_DB_ADDR,  METER_COLL_DB_SIZE);
#endif
#ifdef PLC_X4_FUNC
		data_flash_read_straight_with_crc(X4_READ_TASK_INFO_ADDR,  (u8 *)g_X4ReadTaskInfo, CCTT_X4_READ_TASK_INFO_NUMBER * sizeof(X4_READ_TASK_RECORD),0xff);
#endif
    }
	else
	{
		// 上电时，读出' 表配置数据 '
		dc_flush_fram(&g_PlmConfigPara, sizeof(GLOBAL_CFG_PARA), FRAM_FETCH);
		g_PlmConfigPara.isNeedChangeNid = 0;
		if (g_PlmConfigPara.dataflash_status != DATAFLASH_STATUS_OK)
		{
			//debug_str("000\r\n");
			//Alert(ALERT_FRAM_INIT_FAIL, ALERT_NO_ACTION, __FILE__, __LINE__);
			while (1)
			{
				Reboot_system_now(__func__);
			}
		}
		g_PlmConfigPara.sysRebootCount++;
		dc_flush_fram(&g_PlmConfigPara.sysRebootCount, sizeof(g_PlmConfigPara.sysRebootCount), FRAM_FLUSH);
//		if(data_flash_read_straight_with_crc(METER_RELAY_INFO_ADDR, (u8 *)g_pMeterRelayInfo, CCTT_METER_NUMBER * sizeof(RELAY_INFO),0xff))
//		{
//			g_MeterAddrSuccess=true;
//		}
#if 0
		data_flash_read_straight( STA_INFO_ADDR, (u8*)g_pStaInfo, TEI_CNT*sizeof(STA_INFO));
#else
		memset(g_pStaInfo, 0xff, TEI_CNT * sizeof(STA_INFO));
#endif

#ifdef PLC_X4_FUNC
//		data_flash_read_straight_with_crc(METER_COLL_DB_ADDR,  (u8 *)g_pMeterCollMap, CCTT_METER_COLL_NUMBER * sizeof(METER_COLL_MAP),0xff);
		data_flash_read_straight_with_crc(X4_READ_TASK_INFO_ADDR,  (u8 *)g_X4ReadTaskInfo, CCTT_X4_READ_TASK_INFO_NUMBER * sizeof(X4_READ_TASK_RECORD),0xff);
#endif
	}
#if defined(GDW_2019_PERCEPTION) || defined(NW_GUANGDONG_NEW_LOCAL_INTERFACE)
	for (int i=0;i<TEI_CNT;i++)
	{
		g_pStaInfo[i].offline_tick=0;
	}
#endif
	SYS_READ_VER_INFO();
	SYS_READ_CCO_ID();
#ifdef GDW_ZHEJIANG
	SYS_READ_BLACK_METER();
#endif //GDW_ZHEJIANG	
	if(SYS_READ_METER())
	{
		g_MeterAddrSuccess=true;
	}
	for (int i=0;i<CCTT_METER_NUMBER;i++)
	{
		g_MeterRelayInfo[i].freeze_done=0;
		g_MeterRelayInfo[i].coll_sn=0xffff;
	}
	SYS_READ_DEBUGMASK();
	SYS_READ_FREQ();
	SYS_READ_WHITE_LIST_SWITCH();
	SYS_READ_MAIN_NODE();
	SYS_READ_BAUDRATE();
	SYS_READ_AUTHEN();
	SYS_READ_RELAY_RANGE();
	SYS_READ_COLLECT_INNET_MODE();
    SYS_READ_COMPARE_STA_INVER();
    SYS_READ_PLL();
    trim_PLL(-(g_PlmConfigPara.PLL_Trimming),0);
	HRF_SetSofPm(-g_PlmConfigPara.PLL_Trimming/16);
    SYS_READ_HRF_CHANNEL();
    SYS_READ_HRF_POWER_COMP();
    SetHrfPowerComp(g_PlmConfigPara.HRF_TXChlPowerComp[0],g_PlmConfigPara.HRF_TXChlPowerComp[1]);
	SYS_READ_A_PHASE_ERR();
#ifdef GDW_JIANGXI
	SYS_READ_NETWORK_MODE();
	if (g_PlmConfigPara.network_mode == NETWORK_MODE_PLC)
	{
		max_beacon_period_ms = 10;
	}
#endif
#if defined(PERFORMAINCE_OUT) || defined(GDW_JILIN_SJ)
	g_PlmConfigPara.hplc_comm_param=2;
	g_PlmConfigPara.hrf_option=3;
	g_PlmConfigPara.hrf_channel=41;
	g_PlmConfigPara.hrf_allowChangeChannel=1;
#endif

#ifdef HPLC_CSG
    SYS_READ_EXT_VER_INFO();
    SYS_READ_ELEMENT_REPORT0_MODE();

#ifdef PROTOCOL_NW_2023_GUANGDONG_V2DOT1
    if (!SYS_READ_OUTLIST_METER())
    {
        OutListMeter.cnt = 0;
    }
#endif

    for (int i=0; i<TEI_CNT; i++)
	{
	    //出厂MAC地址初始化默认值
		memset(g_pStaInfo[i].module_id, 0xFF, 6);
	}
#endif
	g_PlmConfigPara.rm_run_switch = 1 << 1;
	g_PlmConfigPara.forceLongTime=0;
    g_PlmConfigPara.test_flag = 0;
	g_PlmConfigPara.plc_max_timeout = DIR_READ_DEFAULT_TIMEOUT_SEC;
	g_PlmConfigPara.RF_CH=0;
	g_PlmConfigPara.RF_PO=0;
    return OK;
}
void SYS_STOR_CCO_ID(void)
{
	g_cco_id.write_flag=CHIP_ID_WRITE_NEW_FLAG;
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
	for (int i=0;i<2;i++)
	{
		for (int j=0;j<3;j++)
		{
			FeedWdg();
			data_flash_write_straight_with_crc(CHIP_ID_ADDR+i*DATA_FLASH_SECTION_SIZE, (u8 *)&g_cco_id, sizeof(g_cco_id));
			u32 crc=CommonGetCrc32((u8*)(FLASH_AHB_ADDR+CHIP_ID_ADDR+i*DATA_FLASH_SECTION_SIZE),  sizeof(g_cco_id));
			if (crc==*(u32*)(FLASH_AHB_ADDR+CHIP_ID_ADDR+i*DATA_FLASH_SECTION_SIZE+sizeof(g_cco_id)))
			{
				break;
}
		}
	}
	CPU_CRITICAL_EXIT();
}

void SYS_READ_CCO_ID(void)
{
	char read_success;
	for (int i=0;i<3;i++)
	{
		FeedWdg();
		read_success=data_flash_read_straight_with_crc(CHIP_ID_ADDR, (u8 *)&g_cco_id, sizeof(g_cco_id), 0x0);
		if (g_cco_id.write_flag!=CHIP_ID_WRITE_NEW_FLAG)
		{
			read_success=0;
			continue ;
		}
		if (read_success)
		{
			break;
		}
	}
	if (!read_success)
	{
		for (int i = 0; i < 3; i++)
		{
			FeedWdg();
			read_success = data_flash_read_straight_with_crc(CHIP_ID_ADDR + DATA_FLASH_SECTION_SIZE, (u8 *)&g_cco_id,  sizeof(g_cco_id), 0x0);
			if (g_cco_id.write_flag!=CHIP_ID_WRITE_NEW_FLAG)
			{
				read_success = 0;
				continue;
			}
			if (read_success)
			{
				break;
			}
		}
		if (read_success)
		{
			int i;
			for (i = 0; i < 3; i++)
			{
				FeedWdg();
				//把备份区内容写回原始区域
				data_flash_write_straight_with_crc(CHIP_ID_ADDR, (u8 *)&g_cco_id,  sizeof(g_cco_id));
				if (data_flash_read_straight_with_crc(CHIP_ID_ADDR, (u8 *)&g_cco_id, sizeof(g_cco_id), 0))
				{
					break;
				}
				else
				{
					if (!data_flash_read_straight_with_crc(CHIP_ID_ADDR + DATA_FLASH_SECTION_SIZE, (u8 *)&g_cco_id,  sizeof(g_cco_id), 0))
					{ //再次读取备份区错误
						Reboot_system_now(__func__);
					}
				}

			}
			if (i >= 3) //原始区域未写入成功 复位
			{
				Reboot_system_now(__func__);
			}
		}
		else //备份区域也出错 返回错误
		{
#ifdef HPLC_CSG		
#ifdef NW_TEST
			memcpy(&g_cco_id, &g_cco_id_default, sizeof(g_cco_id));
#endif
#endif
			return ;
		}
	}
	return ;
}
/*
void FlushAllFlash(void)
{
	if (dev_flash_id == 0x00144020 || dev_flash_id == 0x00154020 || dev_flash_id == 0x0014605e)
	{
		FLASH_Init();
		for (int i = 0; i < 16; i++)
		{
			FLASH_EarseBlock(FLASH_FLUSH_ADDR + i * DATA_FLASH_SECTION_SIZE);
			Feed_WDG();
		}
		FLASH_Close();
	}
}
*/
void SYS_STOR_VER_INFO(void)
{


	c_Tmn_ver_info.write_flag=VER_INFO_WRITE_NEW_FLAG;
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
	for (int i=0;i<2;i++)
	{
		for (int j=0;j<3;j++)
		{
			FeedWdg();
			data_flash_write_straight_with_crc(VER_INFO_ADDR+i*DATA_FLASH_SECTION_SIZE, (u8 *)&c_Tmn_ver_info, sizeof(c_Tmn_ver_info));
			u32 crc=CommonGetCrc32((u8*)(FLASH_AHB_ADDR+VER_INFO_ADDR+i*DATA_FLASH_SECTION_SIZE),  sizeof(c_Tmn_ver_info));
			if (crc==*(u32*)(FLASH_AHB_ADDR+VER_INFO_ADDR+i*DATA_FLASH_SECTION_SIZE+sizeof(c_Tmn_ver_info)))
			{
				break;
}
		}
	}
	CPU_CRITICAL_EXIT();
}
void SYS_READ_VER_INFO(void)
{
	char read_success;
	for (int i=0;i<3;i++)
	{
		FeedWdg();
		read_success=data_flash_read_straight_with_crc(VER_INFO_ADDR, (u8 *)&c_Tmn_ver_info, sizeof(c_Tmn_ver_info), 0x0);
		if (c_Tmn_ver_info.write_flag!=VER_INFO_WRITE_NEW_FLAG)
		{
			read_success=0;
			continue;
		}
		if (read_success)
		{
			break;
		}
	}
	if (!read_success)
	{
		for (int i = 0; i < 3; i++)
		{
			FeedWdg();
			read_success = data_flash_read_straight_with_crc(VER_INFO_ADDR + DATA_FLASH_SECTION_SIZE, (u8 *)&c_Tmn_ver_info,  sizeof(c_Tmn_ver_info), 0x0);
			if (c_Tmn_ver_info.write_flag!=VER_INFO_WRITE_NEW_FLAG)
			{
				continue ;
			}
			if (read_success)
			{
				break;
			}
		}
		if (read_success)
		{
			int i;
			for (i = 0; i < 3; i++)
			{
				FeedWdg();
				//把备份区内容写回原始区域
				data_flash_write_straight_with_crc(VER_INFO_ADDR, (u8 *)&c_Tmn_ver_info,  sizeof(c_Tmn_ver_info));
				if (data_flash_read_straight_with_crc(VER_INFO_ADDR, (u8 *)&c_Tmn_ver_info, sizeof(c_Tmn_ver_info), 0))
				{
					break;
				}
				else
				{
					if (!data_flash_read_straight_with_crc(VER_INFO_ADDR + DATA_FLASH_SECTION_SIZE, (u8 *)&c_Tmn_ver_info,  sizeof(c_Tmn_ver_info), 0))
					{ //再次读取备份区错误
						Reboot_system_now(__func__);
					}
				}

			}
			if (i >= 3) //原始区域未写入成功 复位
			{
				Reboot_system_now(__func__);
			}
		}
		else //备份区域也出错 返回错误
		{
			memcpy(&c_Tmn_ver_info, &c_Tmn_ver_info_default, sizeof(c_Tmn_ver_info));
			return ;
		}
	}
    
#ifdef FIXED_DEFAULT_VER_INFO
    memcpy(c_Tmn_ver_info.software_v, c_Tmn_ver_info_default.software_v, sizeof(c_Tmn_ver_info.software_v));
    memcpy(&c_Tmn_ver_info.softReleaseDate, &c_Tmn_ver_info_default.softReleaseDate, sizeof(c_Tmn_ver_info.softReleaseDate));
#endif

	return ;
}

#ifdef HPLC_CSG
void SYS_STOR_EXT_VER_INFO(void)
{
    c_ext_ver_info.write_flag = VER_INFO_WRITE_NEW_FLAG;
    CPU_SR_ALLOC();
    CPU_CRITICAL_ENTER();
    for (int i=0; i<2; i++)
    {
        for (int j=0; j<3; j++)
        {
            FeedWdg();
            data_flash_write_straight_with_crc(EXT_VER_INFO_ADDR+i*DATA_FLASH_SECTION_SIZE, (u8 *)&c_ext_ver_info, sizeof(c_ext_ver_info));
            u32 crc = CommonGetCrc32((u8*)(FLASH_AHB_ADDR+EXT_VER_INFO_ADDR+i*DATA_FLASH_SECTION_SIZE), sizeof(c_ext_ver_info));
            if (crc == *(u32*)(FLASH_AHB_ADDR+EXT_VER_INFO_ADDR+i*DATA_FLASH_SECTION_SIZE+sizeof(c_ext_ver_info)))
            {
                break;
            }
        }
    }
    CPU_CRITICAL_EXIT();
}

void SYS_READ_EXT_VER_INFO(void)
{
	char read_success;
	for (int i=0; i<3; i++)
	{
		FeedWdg();
		read_success = data_flash_read_straight_with_crc(EXT_VER_INFO_ADDR, (u8 *)&c_ext_ver_info, sizeof(c_ext_ver_info), 0x0);
		if (c_ext_ver_info.write_flag != VER_INFO_WRITE_NEW_FLAG)
		{
			read_success=0;
			continue;
		}
		if (read_success)
		{
			break;
		}
	}
	if (!read_success)
	{
		for (int i = 0; i < 3; i++)
		{
			FeedWdg();
			read_success = data_flash_read_straight_with_crc(EXT_VER_INFO_ADDR + DATA_FLASH_SECTION_SIZE, (u8 *)&c_ext_ver_info,  sizeof(c_ext_ver_info), 0x0);
			if (c_ext_ver_info.write_flag != VER_INFO_WRITE_NEW_FLAG)
			{
				continue ;
			}
			if (read_success)
			{
				break;
			}
		}
        
		if (read_success)
		{
			int i;
			for (i = 0; i < 3; i++)
			{
				FeedWdg();
				//把备份区内容写回原始区域
				data_flash_write_straight_with_crc(EXT_VER_INFO_ADDR, (u8 *)&c_ext_ver_info,  sizeof(c_ext_ver_info));
				if (data_flash_read_straight_with_crc(EXT_VER_INFO_ADDR, (u8 *)&c_ext_ver_info, sizeof(c_ext_ver_info), 0))
				{
					break;
				}
				else
				{
					if (!data_flash_read_straight_with_crc(EXT_VER_INFO_ADDR + DATA_FLASH_SECTION_SIZE, (u8 *)&c_ext_ver_info, sizeof(c_ext_ver_info), 0))
					{ //再次读取备份区错误
						Reboot_system_now(__func__);
					}
				}

			}
			if (i >= 3) //原始区域未写入成功 复位
			{
				Reboot_system_now(__func__);
			}
		}
		else //备份区域也出错 返回错误
		{
			memcpy(&c_ext_ver_info, &c_ext_ver_info_default, sizeof(c_ext_ver_info));
			return;
		}
	}
	return;
}

void SYS_STOR_ELEMENT_REPORT0_MODE(u8 mode)
{
	for (int i=0; i<2; i++)
	{
		for (int j=0; j<3; j++)
		{
			data_flash_write_straight_with_crc(ELEMENT_REPORT0_MODE_ADDR+i*DATA_FLASH_SECTION_SIZE, &mode, 1);
			u8 temp;
			data_flash_read_straight_with_crc_noadc(ELEMENT_REPORT0_MODE_ADDR+i*DATA_FLASH_SECTION_SIZE, &temp, 1, 0xff);
			if (temp==mode)
			{
				break;
			}
		}
	}
}

void SYS_READ_ELEMENT_REPORT0_MODE(void)
{
	for (int i=0;i<3;i++)
	{
		if(data_flash_read_straight_with_crc(ELEMENT_REPORT0_MODE_ADDR, &g_PlmConfigPara.element_report0_mode, 1, 0xff))
		{
			break;
		}
	}
    
	if (g_PlmConfigPara.element_report0_mode == 0xff)
	{
		char read_success;
		for (int i=0;i<3;i++)
		{
			read_success=data_flash_read_straight_with_crc(ELEMENT_REPORT0_MODE_ADDR + DATA_FLASH_SECTION_SIZE, &g_PlmConfigPara.element_report0_mode, 1, 0xff);
			if (read_success)
			{
				break;
			}
		}
		if (read_success) 
		{
			//回写原始区域
			for (int j = 0; j < 3; j++)
			{
				data_flash_write_straight_with_crc(ELEMENT_REPORT0_MODE_ADDR , &g_PlmConfigPara.element_report0_mode, 1);
				u8 temp;
				data_flash_read_straight_with_crc_noadc(ELEMENT_REPORT0_MODE_ADDR, &temp, 1, 0xff);
				if (temp == g_PlmConfigPara.element_report0_mode)
				{
					break;
				}
			}
		}
	}
    
	if (g_PlmConfigPara.element_report0_mode == 0xff)
	{
		g_PlmConfigPara.element_report0_mode = 0; //默认关闭实时查询从节点资产信息（AFN03 E8030313）没有响应回复全0的开关
	}
}

#ifdef PROTOCOL_NW_2023_GUANGDONG_V2DOT1
void SYS_STOR_OUTLIST_METER(void)
{
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
	for (int i=0; i<2; i++)
	{
		for (int j=0; j<3; j++)
		{
			FeedWdg();
			data_flash_write_straight_with_crc(OUTLIST_METER_INFO_ADDR + i*OUTLIST_METER_INFO_SIZE, (u8 *)&OutListMeter, sizeof(RemMeter_s));
			u32 crc = CommonGetCrc32((u8*)(FLASH_AHB_ADDR + OUTLIST_METER_INFO_ADDR + i*OUTLIST_METER_INFO_SIZE), sizeof(RemMeter_s));
			if (crc == *(u32*)(FLASH_AHB_ADDR + OUTLIST_METER_INFO_ADDR + i*OUTLIST_METER_INFO_SIZE + sizeof(RemMeter_s)))
			{
				break;
			}
		}
	}
	CPU_CRITICAL_EXIT();
}

bool SYS_READ_OUTLIST_METER(void)
{
	char read_success;
	for (int i=0; i<3; i++)
	{
		FeedWdg();
		read_success = data_flash_read_straight_with_crc(OUTLIST_METER_INFO_ADDR, (u8 *)&OutListMeter, sizeof(RemMeter_s), 0xff);
		if (read_success)
		{
			break;
		}
	}
	if (!read_success)
	{
		for (int i = 0; i < 3; i++)
		{
			FeedWdg();
			read_success = data_flash_read_straight_with_crc(OUTLIST_METER_INFO_ADDR + OUTLIST_METER_INFO_SIZE, (u8 *)&OutListMeter, sizeof(RemMeter_s), 0xff);
			if (read_success)
			{
				break;
			}
		}
		if (read_success)
		{
			int i;
			for (i = 0; i < 3; i++)
			{
				FeedWdg();
				//把备份区内容写回原始区域
				data_flash_write_straight_with_crc(OUTLIST_METER_INFO_ADDR, (u8 *)&OutListMeter, sizeof(RemMeter_s));
				if (data_flash_read_straight_with_crc(OUTLIST_METER_INFO_ADDR, (u8 *)&OutListMeter, sizeof(RemMeter_s), 0xff))
				{
					break;
				}
				else
				{
					if (!data_flash_read_straight_with_crc(OUTLIST_METER_INFO_ADDR + OUTLIST_METER_INFO_SIZE, (u8 *)&OutListMeter, sizeof(RemMeter_s), 0xff))
					{ //再次读取备份区错误
						Reboot_system_now(__func__);
					}
				}

			}
            
			if (i >= 3) //原始区域未写入成功 复位
			{
				Reboot_system_now(__func__);
			}
		}
		else //备份区域也出错 返回错误
		{
			return false;
		}
	}
	return true;
}
#endif

#endif

unsigned char	 SYS_STOR_ParamClear()
{
    STOR_Dataflash_SYS_init(MSG_STOR_PARA_CLEAR);
    return OK;
}

unsigned char	 SYS_STOR_DataClear()
{
    return OK;
}

unsigned char    SYS_STOR_GetUpdateUserInfo(P_FLASH_INFO_FIELD* ppFlash, P_FLASH_USER_CHARACTER_FIELD* ppUser, unsigned long* p_user_addr, u32* p_new_version)
{

    P_FLASH_USER_CHARACTER_FIELD pUser = NULL;
    unsigned long now_version = 0;

    //P_FLASH_INFO_FIELD pFlash = (P_FLASH_INFO_FIELD)alloc_buffer(__func__,sizeof(FLASH_INFO_FIELD));
    P_FLASH_USER_CHARACTER_FIELD pUser1 = (P_FLASH_USER_CHARACTER_FIELD)alloc_buffer(__func__,sizeof(FLASH_USER_CHARACTER_FIELD));
    P_FLASH_USER_CHARACTER_FIELD pUser2 = (P_FLASH_USER_CHARACTER_FIELD)alloc_buffer(__func__,sizeof(FLASH_USER_CHARACTER_FIELD));



    unsigned long user_addr_1 = DATA_FLASH_SECTION_SIZE*1;
    unsigned long user_addr_2 = DATA_FLASH_SECTION_SIZE*2;

    data_flash_read_straight(user_addr_1, (u8*)pUser1, sizeof(FLASH_USER_CHARACTER_FIELD));
    data_flash_read_straight(user_addr_2, (u8*)pUser2, sizeof(FLASH_USER_CHARACTER_FIELD));

    if(0x0123abcd == pUser1->character)
    {
        now_version = MAX(now_version, pUser1->version);
    }

    if(0x0123abcd == pUser2->character)
    {
        now_version = MAX(now_version, pUser2->version);
    }

	if (0x0123abcd != pUser1->character 
		&&0x0123abcd != pUser2->character)//两个区域都未读到有效值说明flash读取错误 cco需要复位
	{
		OS_ERR err;
		debug_str(DEBUG_LOG_ERR,"read flsh info failed!!\r\n");
		OSTimeDly(10, OS_OPT_TIME_DLY, &err);
		Reboot_system_now(__func__);
	}

    if(0x0123abcd != pUser1->character)
    {
        pUser = pUser1;
		memset(pUser,0,sizeof(*pUser));
		pUser->code_addr = UPDATE_FIRMWARE_DB_S_ADDR; //规避user1区域初始为全0的情况.
        goto L_Free;
    }

    if(0x0123abcd != pUser2->character)
    {
        pUser = pUser2;
		memset(pUser,0,sizeof(*pUser));
		pUser->code_addr = UPDATE_FIRMWARE_DB_ADDR; //规避user2区域初始为全0的情况.
        goto L_Free;
    }
	

    if(pUser1->version < pUser2->version)
        pUser = pUser1;
    else
        pUser = pUser2;

L_Free:

    if(pUser1 == pUser)
    {
        if(NULL != p_user_addr)
            *p_user_addr = user_addr_1;
        free_buffer(__func__,pUser2);
    }
    else if(pUser2 == pUser)
    {
        if(NULL != p_user_addr)
            *p_user_addr = user_addr_2;
        free_buffer(__func__,pUser1);
    }

//    if(NULL != ppFlash)
//        *ppFlash  = pFlash;
//    else
//        free_buffer(__func__,pFlash);

    if(NULL != ppUser)
        *ppUser = pUser;
    else
        free_buffer(__func__,pUser);

    if(NULL != p_new_version)
        *p_new_version = (now_version+1);

    return OK;


}

unsigned long SYS_STOR_GetUpdateAddr()
{
    P_FLASH_USER_CHARACTER_FIELD pUser;

    SYS_STOR_GetUpdateUserInfo(NULL, &pUser, NULL, NULL);
    
    unsigned long  addr = 0;

    if(NULL != pUser)
    {
        addr = pUser->code_addr;
        free_buffer(__func__,pUser);
    }

    return addr;
}

//存储模块进行初始化
unsigned char SYS_STOR_Init(HANDLE h)
{
//    u32 flash_id = data_flash_read_flash_id();
//    debug_str("%x\r\n", flash_id);
    
#if 1
    if(OK == STOR_Dataflash_init())
        return OK;
    else
        return (!OK);
#endif
}

unsigned char SYS_STOR_proc(HANDLE h)
{
    return TRUE;
}

void SYS_UP_ReWrite(UPDATA_PARA* para)
{
	if (para->flag==UP_NEED_PRO_FLASH)//本次升级文件中携带了需要对falsh进行操作的信息
	{
		UPDATA_ITEM *p_item = (UPDATA_ITEM*)(((u32)para) - para->len);
		while ((u32)p_item<(u32)para)
		{
			switch (p_item->item_flag)
			{
			case TMN_VER_INFO_FLAG:
				{
					//厂商代码和芯片代码不升级，先读出再存回
					char temp_factoryCode[2];
					char temp_chip_code[2];
					memcpy(temp_factoryCode, c_Tmn_ver_info.factoryCode, sizeof(c_Tmn_ver_info.factoryCode));
					memcpy(temp_chip_code, c_Tmn_ver_info.chip_code, sizeof(c_Tmn_ver_info.chip_code));
					
					memcpy(&c_Tmn_ver_info, p_item->data, sizeof(c_Tmn_ver_info));
					
					memcpy(c_Tmn_ver_info.factoryCode, temp_factoryCode, sizeof(c_Tmn_ver_info.factoryCode));
					memcpy(c_Tmn_ver_info.chip_code, temp_chip_code, sizeof(c_Tmn_ver_info.chip_code));
					SYS_STOR_VER_INFO();
				}
				break;
			default:
				{
					uint32_t addr=p_item->flash_addr;
					uint32_t size=p_item->erase_size;
					DataFlashInit();
					EraseDataFlash(addr, size);
					DataFlashClose();
				}
				break;
			}
			p_item=(UPDATA_ITEM*)((u8 *)p_item + p_item->data_size);
			FeedWdg();
		}
	}
}


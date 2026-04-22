#include "protocol_includes.h"
#include "os.h"
#include <string.h>
#include <stdlib.h>
#include "bsp.h"
#include <math.h>
#include "Revision.h"
#include "ble_data.h"
#include "common_includes.h"

#define STORE_HEAD_FLG          0x12735896
#define STORE_HEAD_ID_INFO_FLG  0x25689981
#define STORE_GRAPG_FLG         0x25688366
#define STORE_FIX_PARA          0x12745896
#define STORE_FIX_PLL           0x13745896

#pragma pack(4)
typedef struct
{
	u32 Head;
	u32 DataIdx;
	u32 DataLen;
	u32 Crc32;
}ST_FLASH_STORE_HEAD;


//ȱʡ�������
const ST_STA_ATTR_TYPE DefaultStaAttr =
{
	.NetState = NET_OUT_NET,
	.TEI = 0,
	.Mac = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
	.CcoMac = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.NID = { 0xFF },
	.DeviceType = METER,
	.NetBuildSeq = 0,
	.Layer = 0,
	.NetFlag = 0,
	.Role = UNKNOWN_ROLE,
	.Phase = UNKNOWN_LINE,
};

//Ĭ��Ӳ������
const ST_STA_SYS_TYPE DefaultStaSysPara =
{
	.AssocRandom = 0x31759717,
	//.ChipFactoryId = { 0x00 },
	.HardRstCnt = 0,
	.SofeRstCnt = 0,
	.LastCcoMac = { 0 },
	.LastFreq = 2, //Ĭ��Ƶ��2
	.LastRFChannel = 0x208,
};

//Ĭ�ϰ汾
__weak const ST_STA_VER_TYPE DefaultStaVersion =
{
	.LastRstReason = POWER_ON_RESET,
	.BootVer = 0x01,
	
#if defined(NW_TEST) || defined(USE_NW_FACTORY_CODE)
	.SofeVer = { 0x00, 0x01 },
	.VerDate.Year = 21,
	.VerDate.Month = 4,
	.VerDate.Day = 25,	

#elif defined(ZH_STA)
#ifdef GUIZHOU_SJ
	.SofeVer = { 0, 0 },
	.VerDate.Year = 20,
	.VerDate.Month = 7,
	.VerDate.Day = 2,
#else
	.SofeVer = { 1, 1 },
	.VerDate.Year = 20,
	.VerDate.Month = 11,
	.VerDate.Day = 05,
#endif	
	
#elif defined(GS_STA)
#ifdef GUANGDONG_CSG
#ifdef GUANGDONG_CSG_23_1 //�㶫23��1��
    .SofeVer = { 0x23, 0x13 }, 
#else 
    //�㶫24��1��
    .SofeVer = { 0x24, 0x14 }, 
    
    //�㶫23��2��
    //.SofeVer = { 0x23, 0x24 }, 
#endif
#ifdef DONGGUAN_CSG
	.VerDate.Year = 23,
	.VerDate.Month = 10,
	.VerDate.Day = 30,	
#elif defined(GUANGDONG_CSG_23_1)	
    .VerDate.Year = 23,
	.VerDate.Month = 9,
	.VerDate.Day = 1,	
#else
	//�㶫24��1��
	.VerDate.Year = 24,
	.VerDate.Month = 10,
	.VerDate.Day = 10,
	
    //�㶫23��2��
	/*
	.VerDate.Year = 24,
	.VerDate.Month = 1,
	.VerDate.Day = 10,
	*/
#endif

#elif defined(YUNNAN_CSG)
    //230526, 0001��23�����±�����Ϣ�汾
    .SofeVer = { 0, 1 },
	.VerDate.Year = 23,
	.VerDate.Month = 5,
	.VerDate.Day = 26,	
#else
	.SofeVer = { (MODULE_SOFT_VER >> 8) & 0xff, MODULE_SOFT_VER & 0xff },
	.VerDate.Year = MODULE_SOFT_VER_DATE_YEAR,
	.VerDate.Month = MODULE_SOFT_VER_DATE_MON,
	.VerDate.Day = MODULE_SOFT_VER_DATE_DAY,
#endif

#elif defined(ZT_STA)

#ifdef GUANGXI_CSG
    .SofeVer = { 0, 2 },
	.VerDate.Year = 23,
	.VerDate.Month = 12,
	.VerDate.Day = 29,		
#else 
    //�㶫24��1��	
    .SofeVer = { 0x24, 0x14 },
	.VerDate.Year = 24,
	.VerDate.Month = 10,
	.VerDate.Day = 10,
	
    //�㶫23��2��	
    /*
    .SofeVer = { 0x23, 0x24 },
	.VerDate.Year = 24,
	.VerDate.Month = 1,
	.VerDate.Day = 10,
	*/
#endif

#else
	.SofeVer = { (SLG_COMMIT_HASH >> 20) & 0xff, (SLG_COMMIT_HASH >> 12) & 0xff },
	.VerDate.Year = SLG_COMMIT_YEAR % 100,
	.VerDate.Month = SLG_COMMIT_MON,
	.VerDate.Day = SLG_COMMIT_DATE,
#endif

#ifdef GUIZHOU_SJ
	.ChipCode = { "dl" },
	.FactoryID = { "gz" },	
#elif defined(NW_TEST) || defined(USE_NW_FACTORY_CODE)
	.ChipCode = { "NW" },
	.FactoryID = { "NW" },		
#else
	.ChipCode = { CHIP_CODE },
	.FactoryID = { FACTORY_CODE },	
#endif
};

//Ĭ����չ�汾��Ϣ
const ST_STA_EXT_VER_TYPE DefaultStaExtVersion = 
{
#if defined(NW_TEST) || defined(USE_NW_FACTORY_CODE)
	//ģ��Ӳ���汾��Ϣ
	.ModuleHardVer = { 0x00, 0x01},
	.ModuleHardVerDate.Year = 21,
	.ModuleHardVerDate.Month = 4,
	.ModuleHardVerDate.Day = 25,

	//оƬӲ���汾��Ϣ
	.ChipHardVer = { 0x00, 0x01},
	.ChipHardVerDate.Year = 21,
	.ChipHardVerDate.Month = 4,
	.ChipHardVerDate.Day = 25,

	//оƬ�����汾��Ϣ
	.ChipSoftVer = { 0x00, 0x01},
	.ChipSofVerDate.Year = 21,
	.ChipSofVerDate.Month = 4,
	.ChipSofVerDate.Day = 25,

	//Ӧ�ó���汾��
	.AppVer = { 0x00, 0x11},
#else
	//ģ��Ӳ���汾��Ϣ
	.ModuleHardVer = { (MODULE_HARD_VER >> 8) & 0xff, MODULE_HARD_VER & 0xff },
	.ModuleHardVerDate.Year = MODULE_HARD_VER_DATE_YEAR,
	.ModuleHardVerDate.Month = MODULE_HARD_VER_DATE_MON,
	.ModuleHardVerDate.Day = MODULE_HARD_VER_DATE_DAY,

	//оƬӲ���汾��Ϣ
	.ChipHardVer = { (CHIP_HARD_VER >> 8) & 0xff, CHIP_HARD_VER & 0xff },
	.ChipHardVerDate.Year = CHIP_HARD_VER_DATE_YEAR,
	.ChipHardVerDate.Month = CHIP_HARD_VER_DATE_MON,
	.ChipHardVerDate.Day = CHIP_HARD_VER_DATE_DAY,

	//оƬ�����汾��Ϣ
	.ChipSoftVer = { (CHIP_SOFT_VER >> 8) & 0xff, CHIP_SOFT_VER & 0xff },
	.ChipSofVerDate.Year = CHIP_SOFT_VER_DATE_YEAR,
	.ChipSofVerDate.Month = CHIP_SOFT_VER_DATE_MON,
	.ChipSofVerDate.Day = CHIP_SOFT_VER_DATE_DAY,

	//Ӧ�ó���汾��
	.AppVer = { (APP_VER >> 8) & 0xff, APP_VER & 0xff },
#endif
};

const ST_STA_FIX_PARA StaFixParaDefult=
{
	.baud = 1,
#if defined(PROTOCOL_NW_2021)&&!defined(I_STA)
	.ledMode = 1, 
#endif
};
const ST_STA_ID_TYPE StaChipIDDefult = 
{
    .mac        = { 0x01,0x00,0x00,0x10,0x10,0xFF }, //С�ˣ�ĿǰLC����С������
    .assetsCode = { '0','0','0','0','0','0','0','0',
                    '0','0','0','0','0','0','0','0',
                    '0','0','0','0','0','0','0','0'}
};
const Graph_Config_s DefaultGraphConfig = 
{
#ifndef PROTOCOL_NW_EXT_GRAPH
	.config_flag = 0,
	.num =    { 33 },
	.mode =   { 1,1,1,1,1,1,1,1,1,1,
	            1,1,1,1,1,1,1,1,1,1,
	            1,1,1,1,1,1,1,1,1,1,
	            1,1,1 },
	.period = { 15,15,15,15,15,15,15,15,15,15,
	            15,15,15,15,15,15,15,15,15,15,
	            15,15,15,15,15,15,15,15,15,15,
	            15,15,15 },
	.ident =  { 0x02010100,  //A���ѹ
  		 	    0x02010200,  //B���ѹ
  		 	    0x02010300,  //C���ѹ
  		 	    0x0201FF00,  //��ѹ���ݿ�	
  		 	    0x02020100,  //A�����
  		 	    0x02020200,  //B�����
  		 	    0x02020300,  //C�����
  		 	    0x0202FF00,  //�������ݿ�
  		 	    0x02030000,  //���й�����
  		 	    0x02030100,  //A���й�����
  		 	    0x02030200,  //B���й�����
  		 	    0x02030300,  //C���й�����
  		 	    0x0203FF00,  //�й��������ݿ�
  		 	    0x02040000,  //���޹�����
  		 	    0x02040100,  //A���޹�����
  		 	    0x02040200,  //B���޹�����
  		 	    0x02040300,  //C���޹�����
  		 	    0x0204FF00,  //�޹��������ݿ�
  		 	    0x02060000,  //�ܹ�������
  		 	    0x02060100,  //A�๦������
  		 	    0x02060200,  //B�๦������
  		 	    0x02060300,  //C�๦������
  		 	    0x0206FF00,  //�����������ݿ�
  		 	    0x00010000,  //�����й��ܵ���
  		 	    0x00020000,  //�����й��ܵ���
  		 	    0x00030000,  //����޹�1�ܵ���
  		 	    0x00040000,  //����޹�2�ܵ���	
  		 	    0x00050000,  //��һ�����޹��ܵ���
  		 	    0x00060000,  //�ڶ������޹��ܵ���
  		 	    0x00070000,  //���������޹��ܵ���
  		 	    0x00080000,  //���������޹��ܵ���	
  		 	    0x02800004,  //��ǰ�й�����
  		 	    0x02800005 } //��ǰ�޹�����
#else
    .config_flag = 0,
	.num =    { 34 },
	.mode =   { 1,1,1,1,1,1,1,1,1,1, 
	            1,1,1,1,1,1,1,1,1,1,
	            1,1,1,1,1,1,1,1,1,1,
	            1,1,1,1 },
	.period = { 15,15,15,15,15,15,15,15,15,15,
	            15,15,15,15,15,15,15,15,15,15,
	            15,15,15,15,15,15,15,15,15,15,
	            15,15,15,15 },
	.ident =  { 0x02010100,  //A���ѹ
  		 	    0x02010200,  //B���ѹ
  		 	    0x02010300,  //C���ѹ
  		 	    0x0201FF00,  //��ѹ���ݿ�	
  		 	    0x02020100,  //A�����
  		 	    0x02020200,  //B�����
  		 	    0x02020300,  //C�����
  		 	    0x0202FF00,  //�������ݿ�
  		 	    0x02030000,  //���й�����
  		 	    0x02030100,  //A���й�����
  		 	    0x02030200,  //B���й�����
  		 	    0x02030300,  //C���й�����
  		 	    0x0203FF00,  //�й��������ݿ�
  		 	    0x02040000,  //���޹�����
  		 	    0x02040100,  //A���޹�����
  		 	    0x02040200,  //B���޹�����
  		 	    0x02040300,  //C���޹�����
  		 	    0x0204FF00,  //�޹��������ݿ�
  		 	    0x02060000,  //�ܹ�������
  		 	    0x02060100,  //A�๦������
  		 	    0x02060200,  //B�๦������
  		 	    0x02060300,  //C�๦������
  		 	    0x0206FF00,  //�����������ݿ�
  		 	    0x00010000,  //�����й��ܵ���
  		 	    0x00020000,  //�����й��ܵ���
  		 	    0x00030000,  //����޹�1�ܵ���
  		 	    0x00040000,  //����޹�2�ܵ���	
  		 	    0x00050000,  //��һ�����޹��ܵ���
  		 	    0x00060000,  //�ڶ������޹��ܵ���
  		 	    0x00070000,  //���������޹��ܵ���
  		 	    0x00080000,  //���������޹��ܵ���	
  		 	    0x02800004,  //��ǰ�й�����
  		 	    0x02800005,  //��ǰ�޹�����
                0x02800001}  //���ߵ���
#endif
};

extern ST_STA_ATTR_TYPE StaAttr;
extern ST_ROUTE_TAB_TYPE StaRouteTab;

#ifdef ENABLE_PROTOCOL_FLASH
//˫���ݴ洢
void StorageDoubleFlash(u32 flag,u32 addr,void *data,u16 dlen)
{
	ST_FLASH_STORE_HEAD store_head;

	if(BootPowerState == 1)
	{
		CPU_SR_ALLOC();
		OS_CRITICAL_ENTER();
		for (int i=0;i<2;i++)
		{
			for (int j=0;j<3;j++)
			{
				FLASH_Init();
				EraseDataFlash(addr+i*DATA_FLASH_SECTION_SIZE, dlen + sizeof(ST_FLASH_STORE_HEAD));
				store_head.Head = flag;
				store_head.DataIdx = 0;
				store_head.DataLen = dlen;
				store_head.Crc32 = CommonGetCrc32((u8 *)data, store_head.DataLen);
				WriteDataFlash(addr+i*DATA_FLASH_SECTION_SIZE, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
				WriteDataFlash(addr+i*DATA_FLASH_SECTION_SIZE + sizeof(ST_FLASH_STORE_HEAD), (u8 *)data, store_head.DataLen);
				FLASH_ResetAtFault();
				FLASH_Close();
				if (memcmp(&store_head,(void*)(addr + i * DATA_FLASH_SECTION_SIZE),sizeof(ST_FLASH_STORE_HEAD))!=0)
				{
					continue ;
				}
				store_head.Crc32 = CommonGetCrc32((u8 *)(addr + i * DATA_FLASH_SECTION_SIZE + sizeof(ST_FLASH_STORE_HEAD)), store_head.DataLen);
				if (store_head.Crc32 == ((ST_FLASH_STORE_HEAD *)(addr + i * DATA_FLASH_SECTION_SIZE))->Crc32)
				{
					break;
				}
			}
		}
		OS_CRITICAL_EXIT();
	}
	else
	{
		RebootSystem();
	}
}

//˫���ݶ�ȡ
int ReadDoubleFlash(u32 flag,u32 addr,void *data,u16 dlen,const void *defult)
{
	ST_FLASH_STORE_HEAD store_head;
	u32 crc32;
	int i;
	if (BootPowerState != 1)
	{
		RebootSystem();
	}
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	for (i = 0; i < 3; i++)
	{
		ReadDataFlash(addr, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
		if (flag != store_head.Head)
		{
			continue;
		}
		if (store_head.DataLen>dlen)//�̶����򳤶�ֻ����������
		{
			continue;
		}
		ReadDataFlash(addr + sizeof(ST_FLASH_STORE_HEAD), (u8 *)data, store_head.DataLen);
		crc32 = CommonGetCrc32((u8 *)data, store_head.DataLen);
		if (crc32 != store_head.Crc32)
		{
			continue;
		}
		for (int j = 0; j < 3; j++)
		{
			store_head.Head = flag;
			store_head.DataIdx = 0;
			store_head.DataLen = dlen;
			store_head.Crc32 = CommonGetCrc32((u8 *)data, store_head.DataLen);
			if (memcmp(data, (void *)((addr + DATA_FLASH_SECTION_SIZE) + sizeof(ST_FLASH_STORE_HEAD)), dlen) != 0
				|| memcmp(&store_head, (void *)(addr + DATA_FLASH_SECTION_SIZE), sizeof(ST_FLASH_STORE_HEAD)) != 0)
			{
				FLASH_Init();
				EraseDataFlash((addr + DATA_FLASH_SECTION_SIZE), dlen + sizeof(ST_FLASH_STORE_HEAD));

				WriteDataFlash((addr + DATA_FLASH_SECTION_SIZE), (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
				WriteDataFlash((addr + DATA_FLASH_SECTION_SIZE) + sizeof(ST_FLASH_STORE_HEAD), (u8 *)data, store_head.DataLen);
				FLASH_ResetAtFault();
				FLASH_Close();
				if (memcmp(&store_head, (void *)(addr + DATA_FLASH_SECTION_SIZE), sizeof(ST_FLASH_STORE_HEAD)) != 0)
				{
					continue;
				}
				store_head.Crc32 = CommonGetCrc32((u8 *)(addr + DATA_FLASH_SECTION_SIZE + sizeof(ST_FLASH_STORE_HEAD)), store_head.DataLen);
				if (store_head.Crc32 == ((ST_FLASH_STORE_HEAD *)(addr + DATA_FLASH_SECTION_SIZE))->Crc32)
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
		OS_CRITICAL_EXIT();
		return 1;
	}
	for (i = 0; i < 3; i++)
	{
		ReadDataFlash(addr + DATA_FLASH_SECTION_SIZE, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
		if (flag != store_head.Head)
		{
			continue;
		}
		if (store_head.DataLen>dlen)//�̶����򳤶�ֻ����������
		{
			continue;
		}
		ReadDataFlash(addr + DATA_FLASH_SECTION_SIZE + sizeof(ST_FLASH_STORE_HEAD), (u8 *)data, store_head.DataLen);
		crc32 = CommonGetCrc32((u8 *)data, store_head.DataLen);
		if (crc32 == store_head.Crc32)
		{
			break;
		}
	}
	if (i >= 3) //������Ҳû������
	{
		if(data==defult)
		{
			memset(data, 0, dlen);
		}
		else
		{
			memcpy(data, defult, dlen);
		}
		OS_CRITICAL_EXIT();
		return 0;
	}
	else
	{
		for (i = 0; i < 3; i++)
		{
			FLASH_Init();
			EraseDataFlash(addr, dlen + sizeof(ST_FLASH_STORE_HEAD));
			store_head.Head = flag;
			store_head.DataIdx = 0;
			store_head.DataLen = dlen;
			store_head.Crc32 = CommonGetCrc32((u8 *)data, store_head.DataLen);
			WriteDataFlash(addr, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
			WriteDataFlash(addr + sizeof(ST_FLASH_STORE_HEAD), (u8 *)data, store_head.DataLen);
			FLASH_ResetAtFault();
			FLASH_Close();
			if (memcmp(&store_head,(void*)addr,sizeof(ST_FLASH_STORE_HEAD))!=0)
			{
				continue ;
			}
			store_head.Crc32 = CommonGetCrc32( (u8 *)(addr+sizeof(ST_FLASH_STORE_HEAD)), store_head.DataLen);
			if (store_head.Crc32 == ((ST_FLASH_STORE_HEAD *)addr)->Crc32)
			{
				break;
			}
		}
		if (i >= 3)//ԭ��ַ�޷�д��  ��λ
		{
			RebootSystem();
		}
	}
	
	OS_CRITICAL_EXIT();
	return 1;
}
/*
void FlushAllFlash(void)
{
	if (dev_flash_id == 0x00144020 || dev_flash_id == 0x00154020 || dev_flash_id == 0x0014605e)
	{
		static u8 firstFlushFlash=1;
		if (firstFlushFlash)
		{
			firstFlushFlash=0;
			if ((StaSysPara.SofeRstCnt & 0xf)==0)
			{
				DataFlashInit();
				for (int i = 0; i < 16; i++)
				{
					FLASH_EarseBlock(FLASH_FLUSH_ADDR + i * DATA_FLASH_SECTION_SIZE);
					Feed_WDG();
				}
				//����reset��ÿ����Ҫˢ��17��  ����һ��������16������ѭ��
				FLASH_EarseBlock(FLASH_FLUSH_ADDR + ((StaSysPara.SofeRstCnt>>4)&0xf) * DATA_FLASH_SECTION_SIZE);
				DataFlashClose();
			}
		}
	}
}
*/

//�洢�������
void StorageStaAttrInfo(void)
{
	ST_FLASH_STORE_HEAD store_head;
	if(BootPowerState == 1)
	{
		DataFlashInit();
		EraseDataFlash(DATA_STA_ATTR_STORE_ADDR, sizeof(ST_STA_ATTR_TYPE) + sizeof(ST_FLASH_STORE_HEAD));
		store_head.Head = STORE_HEAD_FLG;
		store_head.DataIdx = 0;
		store_head.DataLen = sizeof(ST_STA_ATTR_TYPE);
		store_head.Crc32 = CommonGetCrc32((u8 *)&StaAttr, store_head.DataLen);
		WriteDataFlash(DATA_STA_ATTR_STORE_ADDR, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
		WriteDataFlash(DATA_STA_ATTR_STORE_ADDR + sizeof(ST_FLASH_STORE_HEAD), (u8 *)&StaAttr, sizeof(ST_STA_ATTR_TYPE));
		DataFlashClose();
	}
}

//��ȡ�������
void ReadStaAttrInfo(void)
{
	ST_FLASH_STORE_HEAD store_head;
	u32 crc32;
	ReadDataFlash(DATA_STA_ATTR_STORE_ADDR, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
	if (store_head.DataLen>sizeof(StaAttr))
	{
		store_head.Head=0;
	}
	if (STORE_HEAD_FLG != store_head.Head)
	{
		memcpy(&StaAttr, &DefaultStaAttr, sizeof(ST_STA_ATTR_TYPE));
		StaAttr.DeviceType=GetStaType();
		return;
	}
	ReadDataFlash(DATA_STA_ATTR_STORE_ADDR + sizeof(ST_FLASH_STORE_HEAD), (u8 *)&StaAttr, store_head.DataLen);
	crc32 = CommonGetCrc32((u8 *)&StaAttr, store_head.DataLen);
	if (crc32 != store_head.Crc32)
	{
		memcpy(&StaAttr, &DefaultStaAttr, sizeof(ST_STA_ATTR_TYPE));
	}
	StaAttr.DeviceType=GetStaType();
}

//�洢ͣ�����
void StoragePowerOffFlag(void)
{
	ST_FLASH_STORE_HEAD store_head;
	if(BootPowerState == 1)
	{
		DataFlashInit();
		EraseDataFlash(DATA_POWER_OFF_STORE_ADDR, sizeof(ST_STA_ATTR_TYPE) + sizeof(ST_FLASH_STORE_HEAD));
		store_head.Head = STORE_HEAD_FLG;
		store_head.DataIdx = 0;
		store_head.DataLen = sizeof(IsPowerOff);
		store_head.Crc32 = CommonGetCrc32((u8 *)&IsPowerOff, store_head.DataLen);
		WriteDataFlash(DATA_POWER_OFF_STORE_ADDR, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
		WriteDataFlash(DATA_POWER_OFF_STORE_ADDR + sizeof(ST_FLASH_STORE_HEAD), (u8 *)&IsPowerOff, sizeof(IsPowerOff));
		DataFlashClose();
	}
}

//��ȡͣ�����
void ReadPowerOffFlag(void)
{
	#ifndef I_STA
	ST_FLASH_STORE_HEAD store_head;
	u32 crc32;
	ReadDataFlash(DATA_POWER_OFF_STORE_ADDR, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
	if (store_head.DataLen>sizeof(IsPowerOff))
	{
		store_head.Head=0;
	}
	if (STORE_HEAD_FLG != store_head.Head)
	{
		IsPowerOff=0;
		return;
	}
	ReadDataFlash(DATA_POWER_OFF_STORE_ADDR + sizeof(ST_FLASH_STORE_HEAD), (u8 *)&IsPowerOff, sizeof(IsPowerOff));
	crc32 = CommonGetCrc32((u8 *)&IsPowerOff, store_head.DataLen);
	if (crc32 != store_head.Crc32)
	{
		IsPowerOff=0;
	}
	#endif
}

//�洢PLL Trimming ֵ
void StoragePllTrim(s32 trim)
{

#ifdef II_STA
    if ((trim > 1280) || (trim < -1280)) //trim��λ1/16ppm������80ppm
#else
    if ((trim > 400) || (trim < -400)) //trim��λ1/16ppm������25ppm
#endif
	{
	    return;
    }
    
	if(BootPowerState == 1&&(!PLL_Vaild))
	{
		PLL_Trimming=trim;
		StorageDoubleFlash(STORE_FIX_PLL,DATA_PLL_TRIM_STORE_ADDR,&PLL_Trimming,sizeof(PLL_Trimming));
		PLL_Vaild=1;
	}
}

//��ȡPLL Trimming ֵ
void ReadPllTrim(void)
{
	int default_pll=-10000;
	ReadDoubleFlash(STORE_FIX_PLL,DATA_PLL_TRIM_STORE_ADDR, &PLL_Trimming, sizeof(PLL_Trimming), &default_pll);
#ifdef II_STA
	if ((PLL_Trimming > 1280) || (PLL_Trimming < -1280)) //trim��λ1/16ppm������80ppm
#else	    
	if ((PLL_Trimming > 400) || (PLL_Trimming < -400)) //PLL_Trimming��λ1/16ppm������25ppm
#endif	
	{
		PLL_Trimming=0;
		return;
	}
	PLL_Vaild=1;
}

void StorageStaResetTime(u16 soft_cnt,u16 hard_cnt)
{
	u16 cnt[2];
	cnt[0]=soft_cnt;
	cnt[1]=hard_cnt;
	if(BootPowerState == 1)
	{
		int addr=DATA_RESET_CNT_STORE_ADDR;
		int i=0;
		for (;i<DATA_FLASH_SECTION_SIZE;i+=sizeof(cnt))
		{
			if (*(u32*)addr==0xffffffff)
			{
				break;
			}
			addr+=sizeof(cnt);
		}

        DataFlashInit();
		//�Ѿ������������
		if (addr==DATA_RESET_CNT_STORE_ADDR+DATA_FLASH_SECTION_SIZE)
		{
			EraseDataFlash(DATA_RESET_CNT_STORE_ADDR, DATA_FLASH_SECTION_SIZE);
			addr=DATA_RESET_CNT_STORE_ADDR;
		}
		WriteDataFlash(addr, (u8 *)cnt, sizeof(cnt));
		DataFlashClose();
	}
}
void ReadResetCnt(u16 *soft_cnt,u16 *hard_cnt)
{
	u16 cnt[2];
	int addr=0;
	for (int i=0;i<DATA_FLASH_SECTION_SIZE;i+=sizeof(cnt))
	{
		if (*(u32*)(i+DATA_RESET_CNT_STORE_ADDR)!=0xffffffff)
		{
			addr=DATA_RESET_CNT_STORE_ADDR+i;
		}
	}
	if (addr==0)
	{
		*soft_cnt = 0;
		*hard_cnt= 0;
	}
	else
	{
		memcpy(cnt,(void*)addr,sizeof(cnt));
		*soft_cnt = cnt[0];
		*hard_cnt = cnt[1];
	}
	
}
static ST_STA_SYS_TYPE sta_temp;
//�洢Ӳ������
void StorageStaParaInfo(void)
{
	ST_FLASH_STORE_HEAD store_head;
	if(BootPowerState == 1)
	{	
		memcpy(&sta_temp,&StaSysPara,sizeof(StaSysPara));
		ReadStaParaInfo();
		if (memcmp(&sta_temp.AssocRandom, &StaSysPara.AssocRandom,sizeof(StaSysPara)-4)==0)//ֻ�Ƚϳ���λ�������������
		{
			memcpy(&StaSysPara,&sta_temp,sizeof(StaSysPara));//��ԭ������
			return;
		}
		memcpy(&StaSysPara,&sta_temp,sizeof(StaSysPara));//��ԭ���洢
		DataFlashInit();
		EraseDataFlash(DATA_STA_PARA_STORE_ADDR, sizeof(ST_STA_SYS_TYPE) + sizeof(ST_FLASH_STORE_HEAD));
		store_head.Head = STORE_HEAD_FLG;
		store_head.DataIdx = 0;
		store_head.DataLen = sizeof(ST_STA_SYS_TYPE);
		store_head.Crc32 = CommonGetCrc32((u8 *)&StaSysPara, store_head.DataLen);
		WriteDataFlash(DATA_STA_PARA_STORE_ADDR, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
		WriteDataFlash(DATA_STA_PARA_STORE_ADDR + sizeof(ST_FLASH_STORE_HEAD), (u8 *)&StaSysPara, sizeof(ST_STA_SYS_TYPE));
		DataFlashClose();
	}
}

//��ȡӲ������
void ReadStaParaInfo(void)
{
	ST_FLASH_STORE_HEAD store_head;
	u32 crc32;
	ReadDataFlash(DATA_STA_PARA_STORE_ADDR, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
	if (store_head.DataLen>sizeof(ST_STA_SYS_TYPE))
	{
		store_head.Head=0;
	}
	if (STORE_HEAD_FLG != store_head.Head)
	{
		memcpy(&StaSysPara, &DefaultStaSysPara, sizeof(ST_STA_SYS_TYPE));
		StaSysPara.AssocRandom = rand();
		ReadResetCnt(&StaSysPara.SofeRstCnt, &StaSysPara.HardRstCnt);
		return;
	}
	ReadDataFlash(DATA_STA_PARA_STORE_ADDR + sizeof(ST_FLASH_STORE_HEAD), (u8 *)&StaSysPara, store_head.DataLen);
	crc32 = CommonGetCrc32((u8 *)&StaSysPara, store_head.DataLen);
	if (crc32 != store_head.Crc32)
	{
		memcpy(&StaSysPara, &DefaultStaSysPara, sizeof(ST_STA_SYS_TYPE));
	}
	ReadResetCnt(&StaSysPara.SofeRstCnt, &StaSysPara.HardRstCnt);
	if (StaSysPara.LastFreq >= 3) //Ƶ�β���������֧�ַ�Χ�ڣ�ʹ��Ĭ��Ƶ��
	{
		StaSysPara.LastFreq = DefaultStaSysPara.LastFreq;
		debug_str(DEBUG_LOG_INFO, "ReadStaParaInfo, last freq:%d, change to freq:%d\r\n", StaSysPara.LastFreq, DefaultStaSysPara.LastFreq);
	}
	//���ڴ洢�˲���ʱ��Ҫ���¶�ȡ��Ƚ� ��˵��ڳ��ȵĲ�����Ҫ�������ֹ�Ƚ�ʱ û��������flash�ж�ȡ
	if(sizeof(StaSysPara)>store_head.DataLen)
	{
		memset(&((u8*)(&StaSysPara))[store_head.DataLen],0,sizeof(StaSysPara)-store_head.DataLen);
	}
}

//�洢·�ɲ���
void StorageStaRouteInfo(void)
{
	ST_FLASH_STORE_HEAD store_head;
	u32 dlen;
	if(BootPowerState == 1)
	{
		dlen = sizeof(StaRouteTab);
		DataFlashInit();
		EraseDataFlash(DATA_STA_ROUTE_STORE_ADDR, dlen + sizeof(ST_FLASH_STORE_HEAD));
		store_head.Head = STORE_HEAD_FLG;
		store_head.DataIdx = 0;
		store_head.DataLen = dlen;
		store_head.Crc32 = CommonGetCrc32((u8 *)&StaRouteTab, store_head.DataLen);
		WriteDataFlash(DATA_STA_ROUTE_STORE_ADDR, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
		WriteDataFlash(DATA_STA_ROUTE_STORE_ADDR + sizeof(ST_FLASH_STORE_HEAD), (u8 *)&StaRouteTab, store_head.DataLen);
		DataFlashClose();
	}
}

//��ȡ·�ɲ���
void ReadStaRouteInfo(void)
{
	ST_FLASH_STORE_HEAD store_head;
	u32 crc32;
	ReadDataFlash(DATA_STA_ROUTE_STORE_ADDR, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
	if (store_head.DataLen>sizeof(ST_ROUTE_TAB_TYPE))
	{
		store_head.Head=0;
	}
	if (STORE_HEAD_FLG != store_head.Head)
	{
		memset((u8 *)&StaRouteTab, 0, sizeof(ST_ROUTE_TAB_TYPE));
		return;
	}
	ReadDataFlash(DATA_STA_ROUTE_STORE_ADDR + sizeof(ST_FLASH_STORE_HEAD), (u8 *)&StaRouteTab, store_head.DataLen);
	crc32 = CommonGetCrc32((u8 *)&StaRouteTab, store_head.DataLen);
	if (crc32 != store_head.Crc32)
	{
		memset((u8 *)&StaRouteTab, 0, sizeof(ST_ROUTE_TAB_TYPE));
	}
}


//**********************************************************
//˫��������

//�洢�̶�����
void StorageStaFixPara(void)
{
	StorageDoubleFlash(STORE_FIX_PARA,DATA_STA_FIX_PARA_STORE_ADDR,&StaFixPara,sizeof(StaFixPara));
}
//��ȡ�̶�����
int ReadStaFixPara(void)
{
	int rst= ReadDoubleFlash(STORE_FIX_PARA,DATA_STA_FIX_PARA_STORE_ADDR,&StaFixPara,sizeof(StaFixPara),&StaFixParaDefult);

#ifdef FORCE_TX_POWER_HIGH
    StaFixPara.txPowerMode = 1;
    StaFixPara.txPower[0] = 28;
    StaFixPara.txPower[1] = 28;
    StaFixPara.txPower[2] = 28;
    StaFixPara.txPower[3] = 28;
#endif
	return  rst;	
}
//�洢оƬID
void StorageStaIdInfo(void)
{
	StorageDoubleFlash(STORE_HEAD_ID_INFO_FLG,DATA_STA_ID_INFO_STORE_ADDR,&StaChipID,sizeof(StaChipID));
}

//��ȡоƬID
void ReadStaIdInfo(void)
{
	ReadDoubleFlash(STORE_HEAD_ID_INFO_FLG,DATA_STA_ID_INFO_STORE_ADDR,&StaChipID,sizeof(StaChipID),&StaChipIDDefult);
}

//�洢�汾��Ϣ
void StorageVerInfo(void)
{
	StorageDoubleFlash(STORE_HEAD_ID_INFO_FLG,DATA_VER_INFO_STORE_ADDR,&StaVersion,sizeof(StaVersion));
}

//��ȡ�汾��Ϣ
void ReadVerInfo(void)
{
	ReadDoubleFlash(STORE_HEAD_ID_INFO_FLG,DATA_VER_INFO_STORE_ADDR,&StaVersion,sizeof(StaVersion),&DefaultStaVersion);

#ifdef FIXED_DEFAULT_VER_INFO
    memcpy(StaVersion.SofeVer, DefaultStaVersion.SofeVer, sizeof(StaVersion.SofeVer));
    memcpy(&StaVersion.VerDate, &DefaultStaVersion.VerDate, sizeof(StaVersion.VerDate));
#endif
}

//�洢�汾��Ϣ
void StorageExtVerInfo(void)
{
	StorageDoubleFlash(STORE_HEAD_ID_INFO_FLG,DATA_EXT_VER_INFO_STORE_ADDR, &StaExtVersion, sizeof(StaExtVersion));
}

//��ȡ�汾��Ϣ
void ReadExtVerInfo(void)
{
	ReadDoubleFlash(STORE_HEAD_ID_INFO_FLG,DATA_EXT_VER_INFO_STORE_ADDR, &StaExtVersion, sizeof(StaExtVersion), &DefaultStaExtVersion);
}

//************************************************************


Time_Module_s Time_Module[3];//0������ 1��ƫ�� 3 �洢��ʱ��
Graph_Config_s  Graph_Config;

u32 Data2Sec(Time_Special* ptime)
{
	u32 temp;
	ptime->tm_isdst=0;
	temp=mktime(ptime);
	return temp;
}
//�������� 
void DateSpecialChangeSec(Time_Special* ptime,s32 sec)
{
	ptime->tm_isdst=0;
	time_t temp=mktime(ptime);
	temp+=sec;
	*ptime=*gmtime(&temp);
	ptime->tm_isdst=0;
}
int Diff_Specialtime(Time_Special* ptimea,Time_Special* ptimeb) //a-b
{
	ptimea->tm_isdst=0;
	ptimeb->tm_isdst=0;
	u32 tempa=mktime(ptimea);
	u32 tempb=mktime(ptimeb);
	return tempa-tempb;
}

bool DffSecIn24H(Time_Special* ptimea,Time_Special* ptimeb)
{
	ptimea->tm_isdst=0;
	ptimeb->tm_isdst=0;
	u32 tempa=mktime(ptimea);
	u32 tempb=mktime(ptimeb);
	if (tempa>tempb)
	{
		if (tempa<(tempb+24*60*60))
		{
			return true;
		}
	}
	else
	{
		if ((tempa+24*60*60)>tempb)
		{
			return true;
		}
	}
	return false;
}



//�洢����
void StorageParaxInfo(u32 flash_addr,void * para,u16 dlen)
{
	ST_FLASH_STORE_HEAD store_head;
	if(BootPowerState == 1)
	{
		DataFlashInit();
		EraseDataFlash(flash_addr, dlen + sizeof(ST_FLASH_STORE_HEAD));
		store_head.Head = STORE_HEAD_FLG;
		store_head.DataIdx = 0;
		store_head.DataLen = dlen;
		store_head.Crc32 = CommonGetCrc32((u8 *)para, store_head.DataLen);
		WriteFlash(flash_addr, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
		WriteFlash(flash_addr + sizeof(ST_FLASH_STORE_HEAD), (u8 *)para, store_head.DataLen);
		DataFlashClose();
	}
}

//��ȡ����
void ReadStaParaxInfo(u32 flash_addr,void * para,u16 dlen)
{
	ST_FLASH_STORE_HEAD store_head;
	u32 crc32;
	ReadDataFlash(flash_addr, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
	if (store_head.DataLen>dlen)
	{
		store_head.Head=0;
	}
	if (STORE_HEAD_FLG != store_head.Head)
	{
		memset((u8 *)para, 0, dlen);
		return;
	}
	ReadDataFlash(flash_addr + sizeof(ST_FLASH_STORE_HEAD), (u8 *)para, store_head.DataLen);
	crc32 = CommonGetCrc32((u8 *)para, store_head.DataLen);
	if (crc32 != store_head.Crc32)
	{
		memset((u8 *)para, 0, dlen);
	}
}
void StorageTime(void)
{
	StorageParaxInfo(STA_REAL_TIME_ADDRESS, &Time_Module, sizeof(Time_Module));
}
void ReadStaTime(void)
{
	ReadStaParaxInfo(STA_REAL_TIME_ADDRESS, &Time_Module, sizeof(Time_Module));
	if (Time_Module[0].sec_form_1900==0)
	{
		memset(&Time_Module,0,sizeof(Time_Module));
	}
	Time_Module[0].isVaild=0;
}
void StorageGraphPara(void)
{
	StorageParaxInfo(STA_READ_GRAPH_PARA, &Graph_Config, sizeof(Graph_Config));
}
void ReadStaGraphPara(void)
{
	ReadStaParaxInfo(STA_READ_GRAPH_PARA, &Graph_Config, sizeof(Graph_Config));

    if ((Graph_Config.num[0] == 0) && (Graph_Config.num[1] == 0)) //STA_READ_GRAPH_PARA�ж�ȡGraph_Config����
    {
        ConfigCSG21DeafultGraphPara();
    }
    else //ֻ����21��׼Ҫ��ĸ������߲ɼ�����
    {
        ReConfigCSG21GraphPara();
    }
}

bool SetGraphConfig(u8 config)
{
	if (config != Graph_Config.config_flag)
	{
		Graph_Config.config_flag = config;
		return true;
	}
	
	return false;
}

//��������21��׼Ĭ�����߳�������
void ConfigCSG21DeafultGraphPara(void)
{
	u8 index = (myDevicType == METER)?0:1;
	
	memset(&Graph_Config, 0x00, sizeof(Graph_Config));

	Graph_Config.config_flag = DefaultGraphConfig.config_flag;
	Graph_Config.num[index] = DefaultGraphConfig.num[0];
	memcpy(&Graph_Config.mode[index], &DefaultGraphConfig.mode[0], MAC_GRAPH_CONFIG_NUM);
	memcpy(&Graph_Config.period[index], &DefaultGraphConfig.period[0], MAC_GRAPH_CONFIG_NUM);
	memcpy(&Graph_Config.ident[index], &DefaultGraphConfig.ident[0], 4*MAC_GRAPH_CONFIG_NUM);

    debug_str(DEBUG_LOG_INFO, "ConfigCSG21DeafultGraphPara, num:%d\r\n", Graph_Config.num[index]);
}

//������������21��׼Ĭ�����߳������ã�ֻ��������
void ReConfigCSG21GraphPara(void)
{
	u8 index = (myDevicType == METER)?0:1;

    Graph_Config.config_flag = DefaultGraphConfig.config_flag;
	Graph_Config.num[index] = DefaultGraphConfig.num[0];
	memcpy(&Graph_Config.mode[index], &DefaultGraphConfig.mode[0], MAC_GRAPH_CONFIG_NUM);
	memcpy(&Graph_Config.ident[index], &DefaultGraphConfig.ident[0], 4*MAC_GRAPH_CONFIG_NUM);

    if (memIsHex((u8*)&Graph_Config.period[index], 0x00, MAC_GRAPH_CONFIG_NUM)) //����Ϊ0��Ĭ��ֵ
    {
        memcpy(&Graph_Config.period[index], &DefaultGraphConfig.period[0], MAC_GRAPH_CONFIG_NUM);
    }
    else
    {
        //���ڸ������ݱ�ʶ�������¸�ֵ��������ϰ汾�����������ݱ�ʶ�����仯
        u8 period = Graph_Config.period[index][0];
        memset(&Graph_Config.period[index], 0x00, MAC_GRAPH_CONFIG_NUM);
        memset(&Graph_Config.period[index], period, Graph_Config.num[index]);
    }

    debug_str(DEBUG_LOG_INFO, "ReConfigCSG21GraphPara, num:%d\r\n", Graph_Config.num[index]);
}

void EraseGraph(void)
{
	for (int i=0;i<STA_READ_GRAPH_DATA_SIZE;i+=DATA_FLASH_SECTION_SIZE)
	{
		DataFlashInit();
		EraseDataFlash(i+STA_READ_GRAPH_DATA, DATA_FLASH_SECTION_SIZE);
		DataFlashClose();
		Feed_WDG();
	}
}
//����ĳ��ʱ����Ժ������
void EraseGraphAtTime(u32 time)
{
	Graph_Data_s *pdata_max=NULL;
	for (int i=0;i<STA_READ_GRAPH_DATA_SIZE;i+=DATA_FLASH_SECTION_SIZE)
	{
		Flash_4K_Area *parea=(Flash_4K_Area*)(STA_READ_GRAPH_DATA+i);
		if (parea->have_data==STORE_GRAPG_FLG)
		{
			Graph_Data_s *pdata = (Graph_Data_s *)parea->data;
			if (pdata->sec_from_1900>=time)
			{
				DataFlashInit();
				EraseDataFlash(i+STA_READ_GRAPH_DATA, DATA_FLASH_SECTION_SIZE);
				DataFlashClose();
			}
			else
			{
				if (pdata_max==NULL)
				{
					pdata_max=pdata;
				}
				else
				{
					if (pdata_max->sec_from_1900 > pdata->sec_from_1900)
					{
						pdata_max=pdata;
					}
				}
			}
		}
		Feed_WDG();
	}
	u32 max_addr=(u32)pdata_max&0xfffff000+DATA_FLASH_SECTION_SIZE;
	if(pdata_max)
	{
		while (pdata_max->sec_from_1900 != 0xffffffff)
		{
			if (pdata_max->cs != GetGraphCs(pdata_max)) //У���벻�� ����4k�ĺ���ȫ��������
			{
				break;
			}
			if ((u32)pdata_max  > max_addr) //���Դ洢��λ�ó�����4K��Χ
			{
				break;
			}
			if (pdata_max->sec_from_1900>=time)//��������
			{
				DataFlashInit();
				EraseDataFlash((u32)pdata_max&0xfffff000, DATA_FLASH_SECTION_SIZE);
				DataFlashClose();
				break;
			}
			pdata_max = (Graph_Data_s *)((u32)pdata_max + pdata_max->len + sizeof(Graph_Data_s));
		}
	}
}


void SYS_UP_ReWrite(UPDATA_PARA* para)
{
	if (para->flag==UP_NEED_PRO_FLASH)//���������ļ���Я������Ҫ��falsh���в�������Ϣ
	{
		UPDATA_ITEM *p_item = (UPDATA_ITEM*)(((u32)para) - para->len);
		while ((u32)p_item<(u32)para)
		{
			switch (p_item->item_flag)
			{
			case STA_VERSION_FLAG:
				{
					ST_STA_VER_TYPE StaVersionTemp;
					memcpy(&StaVersionTemp,&StaVersion,sizeof(ST_STA_VER_TYPE));
					memcpy(&StaVersion, p_item->data, sizeof(ST_STA_VER_TYPE));
					StorageVerInfo();
					memcpy(&StaVersion,&StaVersionTemp,sizeof(ST_STA_VER_TYPE));
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
void initNew4kArea(u32 addr,u32 num)
{
	Flash_4K_Area area;
	area.have_data=STORE_GRAPG_FLG;
	area.used_num=num;
	area.sum_befor=STORE_GRAPG_FLG+num;
	DataFlashInit();
	EraseDataFlash(addr, DATA_FLASH_SECTION_SIZE);
	WriteFlash(addr+sizeof(area.all_used)+sizeof(area.end_sec), (u8 *)&area.have_data, sizeof(area.have_data) + sizeof(area.used_num) + sizeof(area.sum_befor));
	DataFlashClose();
}
static bool save_block_ok(Flash_4K_Area *parea)
{
	if(parea->have_data==STORE_GRAPG_FLG)
	{
		if (parea->have_data + parea->used_num == parea->sum_befor)
		{
			return true;
		}
	}
	return false;
}
static Graph_Data_s * alloc_flash_area(u32 len,u32 alloc_sec)
{
	Flash_4K_Area *parea=(Flash_4K_Area*)STA_READ_GRAPH_DATA;
	Flash_4K_Area *parea_min;
	u32 num=0xffffffff;
	u32 num_max=0;
	if (Time_Module[0].isVaild)
	{
		for (int i = 0; i < (STA_READ_GRAPH_DATA_SIZE / DATA_FLASH_SECTION_SIZE); i++)
		{
			if(!save_block_ok(parea))
			{
				if (num)
				{
					num = 0;
					parea_min = parea;
				}
			}
			else
			{
				if (num > parea->used_num)
				{
					num = parea->used_num;
					parea_min = parea;
				}
				if (num_max < parea->used_num)
				{
					num_max = parea->used_num;
				}
				if (parea->all_used == 0xffffffff) //δʹ����
				{
					Graph_Data_s *pdata = (Graph_Data_s *)parea->data;
					Graph_Data_s *firstGraph=pdata;
					u32 end_sec=0;
					u8 lockThisBolck=0;
					while (pdata->sec_from_1900 != 0xffffffff) //�˴�δ��ʹ��
					{
						if (pdata->cs!=GetGraphCs(pdata))//У���벻�� ����4k�ĺ���ȫ��������
						{
							lockThisBolck = 1;
							pdata = (Graph_Data_s *)((u32)parea+DATA_FLASH_SECTION_SIZE);//������ָ��ָ���β
							break;
						}
						end_sec=pdata->sec_from_1900;
						if (alloc_sec < pdata->sec_from_1900)//�����ʱ��С�ڿ��ڵ�ʱ��
						{
							lockThisBolck=1;//��Ҫ��������
						}
						if (firstGraph->sec_from_1900 > pdata->sec_from_1900) //��һ֡�����ݵ�ʱ�����ͷ���ֵ�ʱ���С
						{
							lockThisBolck=1;//��Ҫ��������
						}
						if (((u32)pdata - (u32)parea + len) >= DATA_FLASH_SECTION_SIZE) //���Դ洢��λ�ó�����4K��Χ
						{
							lockThisBolck=1;//��Ҫ��������
						}
						if (((u32)pdata - (u32)parea + sizeof(Graph_Data_s)) >= DATA_FLASH_SECTION_SIZE)//��һ���Ѿ������Դ洢һ����
						{
							lockThisBolck=1;
							break;
						}
						pdata = (Graph_Data_s *)((u32)pdata + pdata->len + sizeof(Graph_Data_s));
					}
					if (lockThisBolck||(((u32)pdata - (u32)parea + len) >= DATA_FLASH_SECTION_SIZE)) //���Դ洢��λ�ó�����4K��Χ
					{
						//if (((u32)pdata - (u32)parea)<DATA_FLASH_SECTION_SIZE)//ʣ��Ŀռ䲻���洢һ֡����
						{
							u32 flag[2] = {STORE_GRAPG_FLG, end_sec};
							DataFlashInit();
							WriteFlash((u32)&parea->all_used, (u8 *)flag, sizeof(flag));
							DataFlashClose();
						}

					}
					else
					{
						return pdata;
					}
				}
			}
			parea=(Flash_4K_Area *)((u32)parea+DATA_FLASH_SECTION_SIZE);
		}
		initNew4kArea((u32)parea_min, num_max+1);
		return (Graph_Data_s *)parea_min->data;
	}
	return NULL;
}
void StorageGraphData(Graph_Data_s *pdata)
{
	u32 flash_addr = (u32)alloc_flash_area(pdata->len + sizeof(Graph_Data_s), pdata->sec_from_1900);
	DataFlashInit();
	WriteFlash(flash_addr, (u8 *)pdata, sizeof(Graph_Data_s) + pdata->len);
	DataFlashClose();
}
#define MAX_GH_CNT  50
typedef struct {
	u32 num;
	Graph_Data_s* point;
}graph_find_stack_s;
static graph_find_stack_s find_stack[MAX_GH_CNT];
Graph_Data_s * FindStaGraphData(u32 sec_from_2000,u32 ident)
{
	Flash_4K_Area *parea=(Flash_4K_Area*)STA_READ_GRAPH_DATA;
	Graph_Data_s *pdata;
	memset(find_stack,0,sizeof(find_stack));
	graph_find_stack_s temp;
	//�ҵ�����С�ڵ���Ҫ��ʱ���4k����ʱ������ ����ӽ���ǰʱ��Ŀ�
	for (int i = 0; i < (STA_READ_GRAPH_DATA_SIZE / DATA_FLASH_SECTION_SIZE); i++)
	{
		if(save_block_ok(parea))
		{
			pdata = (Graph_Data_s *)parea->data;
			if (GetGraphCs(pdata) != pdata->cs)//��һ���������cs���ǲ��Ե�
			{
				parea =(Flash_4K_Area*)((u32)parea+DATA_FLASH_SECTION_SIZE);
				continue ;
			}
			if(parea->all_used==STORE_GRAPG_FLG)//�Ѿ�����
			{
				if(parea->end_sec < sec_from_2000)//������ʱ�����ĩβ��ʱ��
				{
					parea = (Flash_4K_Area*)((u32)parea+DATA_FLASH_SECTION_SIZE);
					continue;
				}
			}
			if (pdata->sec_from_1900 <= sec_from_2000)
			{			
				temp.num = ((Flash_4K_Area *)((u32)pdata - sizeof(Flash_4K_Area)))->used_num;
				temp.point=pdata;
				for (int j=0;j<MAX_GH_CNT;j++)//С������ʱ������µĿ�
				{
					if (temp.num > find_stack[j].num)
					{
						memmove(&find_stack[j+1],&find_stack[j],sizeof(graph_find_stack_s)*(MAX_GH_CNT-1-j));
						find_stack[j]=temp;
						break;
					}
				}
			}
		}
		parea = (Flash_4K_Area*)((u32)parea+DATA_FLASH_SECTION_SIZE);
	}

	for (int i=0;i<MAX_GH_CNT;i++)
	{
		Graph_Data_s *pdata_real = find_stack[i].point;
		Graph_Data_s *pdata_last = NULL;
		if (pdata_real != NULL)
		{
			parea = (Flash_4K_Area*)((u32)pdata_real&0xfffff000);//��4k���伴Ϊ������ʼ��ַ
			while ((u32)pdata_real<(u32)parea+DATA_FLASH_SECTION_SIZE)
			{
				if (pdata_real->sec_from_1900>sec_from_2000)
				{
					break;
				}
				if (pdata_real->sec_from_1900 == sec_from_2000 && pdata_real->ident == ident)
				{
					if (GetGraphCs(pdata_real) == pdata_real->cs)//У��ͨ��
					{
						pdata_last= pdata_real;
					}
				}
				pdata_real = (Graph_Data_s*)((u32)pdata_real + pdata_real->len +sizeof(Graph_Data_s));
			}
			if (pdata_last)
			{
				return pdata_last;
			}
		}
	}

	return NULL;
}



void StaRebootCntProcess(void)
{
	/*
	switch(GetRebootSource())
	{
	case REBOOT_BY_WATCHDOG:
		StaVersion.LastRstReason = WATCHDOG_RESET;
		StaSysPara.SofeRstCnt += 1;
		break;
	case REBOOT_BY_HARDWARE:
		StaVersion.LastRstReason = POWER_ON_RESET;
		StaSysPara.HardRstCnt += 1;
		break;
	case REBOOT_BY_SOFTWARE:
		StaVersion.LastRstReason = NORMAL_RESET;
		StaSysPara.SofeRstCnt += 1;
		break;
	default:
		StaVersion.LastRstReason = EXCEPTION_RESET;
		StaSysPara.SofeRstCnt += 1;
		break;
	}
	*/
	StaVersion.LastRstReason = POWER_ON_RESET;
	StaSysPara.SofeRstCnt += 1;
	StorageStaResetTime(StaSysPara.SofeRstCnt, StaSysPara.HardRstCnt);
}
u8 GetResetReason(void)
{
	return StaVersion.LastRstReason;
}
extern u32 HPLCCurrentFreq;
extern u8 BindingAddrModeCnt;

//��ȡflash��վ����һ��������Ƶ��
void StaLastFreqProcess(void)
{
	if (StaSysPara.LastFreq <= 3)
	{
		ResetFcCnt();
#if defined(GUANGXI_CSG)
		ResetChangeShortFreTimer();
#elif defined(GUANGDONG_CSG)
		ResetChangeShorterFreTimer();//��Զ��Ҫ90�������� ����Ƶ��1�������ܶ�
#else
		ResetChangeFreTimer();
#endif
		if (GetAppCurrentModel() == APP_TEST_NORMAL)
		{
#ifndef CHECK_LAST_FREQ	
			HPLCCurrentFreq = StaSysPara.LastFreq;
			HPLC_PHY_SetFrequence(HPLCCurrentFreq);
#else
			const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();
			//δ��������£�����ѡ����һ������Ƶ��
			if (pNetBasePara->NetState != NET_IN_NET)
			{         
				HPLCCurrentFreq = StaSysPara.LastFreq;
				HPLC_PHY_SetFrequence(HPLCCurrentFreq);

				ResetCalibrateNTB();
				SetNetState(NET_OUT_NET);
				ClearAttemptNet();
				CleanAllData();
			}
#endif                
		}       
	//�õ���ʵƵ��
		
		debug_str(DEBUG_LOG_NET,"Current Band plan is %d\r\n", StaSysPara.LastFreq);
	}
}
//��ȡflash��վ����һ�������������ŵ�
void StaLastRFChannelProcess(void)
{
	if (((u8)(StaSysPara.LastRFChannel>>8) == 2 && (u8)(StaSysPara.LastRFChannel&0xff) <= 80) || ((u8)(StaSysPara.LastRFChannel>>8) == 3 && (u8)(StaSysPara.LastRFChannel&0xff) <= 200))
	{
		if (GetAppCurrentModel()==APP_TEST_NORMAL)
		{
			HRFCurrentIndex = StaSysPara.LastRFChannel;
			setCurrentHrfIndex(HRFCurrentIndex);
			HrfSetExtIndex(HRFCurrentIndex,0);
			HRF_PHY_SetChannel(HRFCurrentIndex&0xff,HRFCurrentIndex>>8);
		}
		debug_str(DEBUG_LOG_NET,"Current RFChannel is op=%d, ch=%d\r\n",HRFCurrentIndex>>8,HRFCurrentIndex&0xff);
	}
}

//���ڷ��书��ģʽ����ֵHPLC_ChlPower
void StaTxPowerProcess(void)
{
	if(StaFixPara.txPowerMode[0] == 0) //���书���Զ��л�ģʽ
	{
		memcpy(HPLC_ChlPower,HPLC_HighPower, 4);
	}
	else
	{
		memcpy(HPLC_ChlPower, StaFixPara.txPower, 4);
	}
	
	if(StaFixPara.txPowerMode[1] == 0) //���书���Զ��л�ģʽ
	{
		memcpy(HRF_ChlPower, HRF_HighPower, 2);
	}
	else
	{
		memcpy(HRF_ChlPower, StaFixPara.hrfTxPower, 2);
	}
}


//��ȡflash�д��ڲ�����
void StaBaudProcess(void)
{
	switch(StaFixPara.baud)
	{
		case 0: //1200
			BindingAddrModeCnt = 8;
			break;
		case 1: //2400
			BindingAddrModeCnt = 0;
			defultAddrCnt=-1;
			break;
		/****************˳������͹���һ��********************/	
		case 2: //9600
			BindingAddrModeCnt = 2;
			break;
		case 3: //4800
			BindingAddrModeCnt = 10;
			break;
		/******************************************************/
		case 4: //19200
			BindingAddrModeCnt = 4;
			break;
		case 5: //38400
			BindingAddrModeCnt = 6;
			break;
		default: //2400
			BindingAddrModeCnt = 0;
			break;
	}
}

u8 diffMacFlag=0;
__weak void EraseData(void)
{

}
//��¼��һ�������ĵ����ַ
bool SetStaLastMeter(void)
{
	if (0 != memcmp(StaAttr.Mac, StaSysPara.LastMeter, LONG_ADDR_LEN))
	{
#ifdef PROTOCOL_NW_2020		
		memset(Time_Module,0,sizeof(Time_Module));
		StorageTime();
		memset(&Graph_Config, 0, sizeof(Graph_Config));
		ConfigCSG21DeafultGraphPara();
		StorageGraphPara();
		EraseGraph();
#endif			
		diffMacFlag=1;
		EraseData();
		memcpy(StaSysPara.LastMeter,StaAttr.Mac,LONG_ADDR_LEN);
	}

	return true;
}

void ReadStaParaInfoOnly(void)
{
	ST_FLASH_STORE_HEAD store_head;
	u32 crc32;
	
	if(!ReadStaFixPara())
	{
		defultAddrCnt=-1;
	}
#if defined(PROTOCOL_NW_2021)&&!defined(I_STA)
	StaFixPara.ledMode = 1; 
#endif

	ReadDataFlash(DATA_STA_PARA_STORE_ADDR, (u8 *)&store_head, sizeof(ST_FLASH_STORE_HEAD));
	if (store_head.DataLen>sizeof(ST_STA_SYS_TYPE))
	{
		store_head.Head=0;
	}
	if (STORE_HEAD_FLG != store_head.Head)
	{
		memcpy(&StaSysPara, &DefaultStaSysPara, sizeof(ST_STA_SYS_TYPE));
		StaSysPara.AssocRandom = rand();

#ifdef II_STA //��ܲɼ���û�ѵ���ʱû�а汾��Ϣ������
		ReadStaIdInfo();
		ReadVerInfo();
#endif                
	}
	else
	{
		ReadDataFlash(DATA_STA_PARA_STORE_ADDR + sizeof(ST_FLASH_STORE_HEAD), (u8 *)&StaSysPara, store_head.DataLen);
		crc32 = CommonGetCrc32((u8 *)&StaSysPara, store_head.DataLen);
		if (crc32 != store_head.Crc32)
		{
			memcpy(&StaSysPara, &DefaultStaSysPara, sizeof(ST_STA_SYS_TYPE));
		}
	}
	if (StaSysPara.LastFreq >= 3) //Ƶ�β���������֧�ַ�Χ�ڣ�ʹ��Ĭ��Ƶ��
	{
		StaSysPara.LastFreq = DefaultStaSysPara.LastFreq;
		debug_str(DEBUG_LOG_INFO, "ReadStaParaInfoOnly, last freq:%d, change to freq:%d\r\n", StaSysPara.LastFreq, DefaultStaSysPara.LastFreq);
	}
	StaLastFreqProcess();
	if(defultAddrCnt != -1)
	{
		StaBaudProcess();
	}
	StaTxPowerProcess();
	ReadStaAttrInfo();
	StaLastRFChannelProcess();	
	setCurrentHrfIndex(HRFCurrentIndex);
#ifdef PROTOCOL_NW_2020		
	ReadStaTime();
	ReadStaGraphPara();
#endif

#ifdef PROTOCOL_NW_2021
	Graph_Config.config_flag = 0; //����21��׼Ĭ�Ϲر����߲ɼ������ű���������Ŀ����
#endif
#ifdef II_STA //��ܲɼ���û�ѵ���ʱû�а汾��Ϣ������
	ReadStaIdInfo();
	ReadVerInfo();
#endif
}
extern uint32_t crc32_cal(uint8_t *data, uint32_t len);

mac_list_t g_mac_list = {0};

void mac_list_flash_save()
{
	debug_str(DEBUG_LOG_UPDATA, "mac_list_flash_save\r\n");
	flash_store_head_st store_head;
	DataFlashInit();
    EraseDataFlash(LC_MAC_LIST, DATA_FLASH_PAGE_SIZE + sizeof(mac_list_t));
    
    store_head.Head = BLE_STORE_HEAD_FLG;
    store_head.DataIdx = 0;
    store_head.DataLen = sizeof(mac_list_t);
    store_head.Crc32 = crc32_cal((uint8_t *)&g_mac_list, store_head.DataLen);

    WriteDataFlash(LC_MAC_LIST + DATA_FLASH_PAGE_SIZE, (uint8_t *)&g_mac_list,sizeof(mac_list_t));
    WriteDataFlash(LC_MAC_LIST, (uint8_t*)&store_head,sizeof(flash_store_head_st));
	DataFlashClose();
}

bool mac_list_flash_read(void)
{
    flash_store_head_st store_head;
    uint32_t crc32 = 0;
    bool rflg = true;
    ReadDataFlash(LC_MAC_LIST, (uint8_t*)&store_head, sizeof(flash_store_head_st));
    if((BLE_STORE_HEAD_FLG != store_head.Head)||(store_head.DataLen != sizeof(mac_list_t)))
    {
        rflg = false;
    }
    else
    {
        ReadDataFlash(LC_MAC_LIST+ DATA_FLASH_PAGE_SIZE, (uint8_t *)&g_mac_list,store_head.DataLen);

        crc32 = crc32_cal((uint8_t *)&g_mac_list, store_head.DataLen);
    }
    if(crc32 != store_head.Crc32)
    {
        rflg = false;
    }
    return rflg;
}

void mac_list_flash_init()
{
	if(false == mac_list_flash_read())
    {
		memset((uint8_t *)&g_mac_list,0,sizeof(mac_list_t));
		uint8_t mac[6] = {0x00,0xAF,0xFF,0xFF,0xFF,0xFF};
		memcpy(g_mac_list.white_mac_list[0],mac,6);
        mac_list_flash_save();
    }
}



#endif

/*
*********************************************************************************************************
*                                   CRASH LOG FLASH STORAGE
*
*  Layout:
*    Header (16 bytes): magic(4) + seq(4) + len(4) + crc32(4)
*    Records (960 bytes): 20 x crash_record_t (48 bytes each)
*    Actual struct size = 976 bytes; erase unit = CRASH_LOG_TOTAL_SIZE (1024)
*
*  Ring buffer: new records overwrite oldest via (seq % 20).
*  CRC32 covers only the 960 bytes of records, not the header.
*********************************************************************************************************
*/

#define CRASH_LOG_MAGIC         0x11223344
#define CRASH_LOG_RECORD_MAX    20
#define CRASH_LOG_BT_MAX        9
#define CRASH_LOG_TOTAL_SIZE    1024
#define CRASH_LOG_BLOCK_RECORDS 5

#pragma pack(push, 1)
typedef struct {
    u32 sofeRstCnt;                     /* Software reset count */
    u32 pc;                             /* PC at crash */
    u32 lr;                             /* LR at crash */
    u32 backtrace[CRASH_LOG_BT_MAX];    /* Stack backtrace (up to 9, 0 if unused) */
} crash_record_t;  /* 4+4+4+36 = 48 bytes */

typedef struct {
    u32 magic;                          /* 0x11223344 */
    u32 seq;                            /* Current storage sequence number */
    u32 len;                            /* Data length (960) */
    u32 crc32;                          /* CRC32 of records[] */
} crash_log_header_t;  /* 16 bytes */

typedef struct {
    crash_log_header_t header;
    crash_record_t     records[CRASH_LOG_RECORD_MAX]; /* 48*20 = 960 bytes */
} crash_log_flash_t;  /* 16+960 = 976 bytes */
#pragma pack(pop)

void crash_log_flash_save(void)
{
    volatile crash_info_t *sram_info_p = CRASH_INFO_ADDR;
    crash_log_flash_t *crash_log_p;
    u32 idx;
    u32 i;
    u32 bt_depth;

    if (resetReason() != SYS_FALUT_RESET)
        return;

    crash_log_p = (crash_log_flash_t *)malloc(sizeof(crash_log_flash_t));
    if (crash_log_p == NULL)
    {
        debug_str(0xFFFFFFFF, "crash_log_flash_save: malloc failed\r\n");
        return;
    }

    /* Read existing crash log from flash */
    ReadDataFlash(LC_CRASH_LOG_ADDR, (u8 *)crash_log_p, sizeof(crash_log_flash_t));

    /* Validate header */
    if (crash_log_p->header.magic != CRASH_LOG_MAGIC ||
        crash_log_p->header.len != sizeof(crash_record_t) * CRASH_LOG_RECORD_MAX)
    {
        /* First use or corrupted, initialize */
        memset(crash_log_p, 0, sizeof(crash_log_flash_t));
        crash_log_p->header.magic = CRASH_LOG_MAGIC;
        crash_log_p->header.seq = 0;
        crash_log_p->header.len = sizeof(crash_record_t) * CRASH_LOG_RECORD_MAX;
    }
    else
    {
        /* Verify CRC */
        u32 crc = crc32_cal((u8 *)crash_log_p->records, crash_log_p->header.len);
        if (crc != crash_log_p->header.crc32)
        {
            /* CRC mismatch, re-initialize */
            memset(crash_log_p->records, 0, sizeof(crash_log_p->records));
            crash_log_p->header.seq = 0;
        }
    }

    /* Calculate write index (ring buffer) */
    idx = crash_log_p->header.seq % CRASH_LOG_RECORD_MAX;

    /* Fill in the new record from SRAM crash info */
    crash_log_p->records[idx].sofeRstCnt = (u32)StaSysPara.SofeRstCnt;
    crash_log_p->records[idx].pc = sram_info_p->pc;
    crash_log_p->records[idx].lr = sram_info_p->lr;

    /* Clamp bt_depth from SRAM (value may be corrupted after abnormal reset) */
    bt_depth = sram_info_p->bt_depth;
    if (bt_depth > CRASH_BT_DEPTH_MAX)
        bt_depth = CRASH_BT_DEPTH_MAX;

    /* Copy backtrace, up to 9 entries, fill rest with 0 */
    for (i = 0; i < CRASH_LOG_BT_MAX; i++)
    {
        if (i < bt_depth)
            crash_log_p->records[idx].backtrace[i] = sram_info_p->backtrace[i];
        else
            crash_log_p->records[idx].backtrace[i] = 0;
    }

    /* Update sequence number */
    crash_log_p->header.seq++;

    /* Recalculate CRC over all records */
    crash_log_p->header.crc32 = crc32_cal((u8 *)crash_log_p->records, crash_log_p->header.len);

    /* Write back to flash */
    DataFlashInit();
    EraseDataFlash(LC_CRASH_LOG_ADDR, CRASH_LOG_TOTAL_SIZE);
    WriteDataFlash(LC_CRASH_LOG_ADDR, (u8 *)crash_log_p, sizeof(crash_log_flash_t));
    DataFlashClose();

    free(crash_log_p);
}

void crash_log_flash_clear(void)
{
    DataFlashInit();
    EraseDataFlash(LC_CRASH_LOG_ADDR, CRASH_LOG_TOTAL_SIZE);
    DataFlashClose();
}

u32 crash_log_flash_read_block(u8 block_idx, u8 *buf, u32 buf_size)
{
    crash_log_flash_t *crash_log_p;
    u32 copy_len;

    if (block_idx > 3 || buf == NULL)
        return 0;

    copy_len = CRASH_LOG_BLOCK_RECORDS * sizeof(crash_record_t);
    if (buf_size < copy_len)
        return 0;

    crash_log_p = (crash_log_flash_t *)malloc(sizeof(crash_log_flash_t));
    if (crash_log_p == NULL)
    {
        debug_str(0xFFFFFFFF, "crash_log_flash_read_block: malloc failed\r\n");
        return 0;
    }

    ReadDataFlash(LC_CRASH_LOG_ADDR, (u8 *)crash_log_p, sizeof(crash_log_flash_t));

    /* Validate header + CRC */
    if (crash_log_p->header.magic != CRASH_LOG_MAGIC ||
        crash_log_p->header.len != sizeof(crash_record_t) * CRASH_LOG_RECORD_MAX)
    {
        memset(buf, 0, copy_len);
        free(crash_log_p);
        return copy_len;
    }

    {
        u32 crc = crc32_cal((u8 *)crash_log_p->records, crash_log_p->header.len);
        if (crc != crash_log_p->header.crc32)
        {
            memset(buf, 0, copy_len);
            free(crash_log_p);
            return copy_len;
        }
    }

    {
        u32 offset = (u32)block_idx * CRASH_LOG_BLOCK_RECORDS;
        memcpy(buf, &crash_log_p->records[offset], copy_len);
    }

    free(crash_log_p);
    return copy_len;
}

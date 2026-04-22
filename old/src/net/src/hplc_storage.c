#include "protocol_includes.h"
#include "os.h"
#include <string.h>
#include "bsp.h"
#include <math.h>
#include "Revision.h"

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


//缺省网络参数
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

//默认硬件参数
const ST_STA_SYS_TYPE DefaultStaSysPara =
{
	.AssocRandom = 0x31759717,
	//.ChipFactoryId = { 0x00 },
	.HardRstCnt = 0,
	.SofeRstCnt = 0,
	.LastCcoMac = { 0 },
	.LastFreq = 2, //默认频段2
	.LastRFChannel = 0x208,
};

//默认版本
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
#ifdef GUANGDONG_CSG_23_1 //广东23年1批
    .SofeVer = { 0x23, 0x13 }, 
#else 
    //广东24年1批
    .SofeVer = { 0x24, 0x14 }, 
    
    //广东23年2批
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
	//广东24年1批
	.VerDate.Year = 24,
	.VerDate.Month = 10,
	.VerDate.Day = 10,
	
    //广东23年2批
	/*
	.VerDate.Year = 24,
	.VerDate.Month = 1,
	.VerDate.Day = 10,
	*/
#endif

#elif defined(YUNNAN_CSG)
    //230526, 0001是23年最新备案信息版本
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
    //广东24年1批	
    .SofeVer = { 0x24, 0x14 },
	.VerDate.Year = 24,
	.VerDate.Month = 10,
	.VerDate.Day = 10,
	
    //广东23年2批	
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

//默认扩展版本信息
const ST_STA_EXT_VER_TYPE DefaultStaExtVersion = 
{
#if defined(NW_TEST) || defined(USE_NW_FACTORY_CODE)
	//模块硬件版本信息
	.ModuleHardVer = { 0x00, 0x01},
	.ModuleHardVerDate.Year = 21,
	.ModuleHardVerDate.Month = 4,
	.ModuleHardVerDate.Day = 25,

	//芯片硬件版本信息
	.ChipHardVer = { 0x00, 0x01},
	.ChipHardVerDate.Year = 21,
	.ChipHardVerDate.Month = 4,
	.ChipHardVerDate.Day = 25,

	//芯片软件版本信息
	.ChipSoftVer = { 0x00, 0x01},
	.ChipSofVerDate.Year = 21,
	.ChipSofVerDate.Month = 4,
	.ChipSofVerDate.Day = 25,

	//应用程序版本号
	.AppVer = { 0x00, 0x11},
#else
	//模块硬件版本信息
	.ModuleHardVer = { (MODULE_HARD_VER >> 8) & 0xff, MODULE_HARD_VER & 0xff },
	.ModuleHardVerDate.Year = MODULE_HARD_VER_DATE_YEAR,
	.ModuleHardVerDate.Month = MODULE_HARD_VER_DATE_MON,
	.ModuleHardVerDate.Day = MODULE_HARD_VER_DATE_DAY,

	//芯片硬件版本信息
	.ChipHardVer = { (CHIP_HARD_VER >> 8) & 0xff, CHIP_HARD_VER & 0xff },
	.ChipHardVerDate.Year = CHIP_HARD_VER_DATE_YEAR,
	.ChipHardVerDate.Month = CHIP_HARD_VER_DATE_MON,
	.ChipHardVerDate.Day = CHIP_HARD_VER_DATE_DAY,

	//芯片软件版本信息
	.ChipSoftVer = { (CHIP_SOFT_VER >> 8) & 0xff, CHIP_SOFT_VER & 0xff },
	.ChipSofVerDate.Year = CHIP_SOFT_VER_DATE_YEAR,
	.ChipSofVerDate.Month = CHIP_SOFT_VER_DATE_MON,
	.ChipSofVerDate.Day = CHIP_SOFT_VER_DATE_DAY,

	//应用程序版本号
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
    .mac        = { 0x01,0x00,0x00,0x10,0x10,0xFF }, //小端，目前LC产测小端输入
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
	.ident =  { 0x02010100,  //A相电压
  		 	    0x02010200,  //B相电压
  		 	    0x02010300,  //C相电压
  		 	    0x0201FF00,  //电压数据块	
  		 	    0x02020100,  //A相电流
  		 	    0x02020200,  //B相电流
  		 	    0x02020300,  //C相电流
  		 	    0x0202FF00,  //电流数据块
  		 	    0x02030000,  //总有功功率
  		 	    0x02030100,  //A相有功功率
  		 	    0x02030200,  //B相有功功率
  		 	    0x02030300,  //C相有功功率
  		 	    0x0203FF00,  //有功功率数据块
  		 	    0x02040000,  //总无功功率
  		 	    0x02040100,  //A相无功功率
  		 	    0x02040200,  //B相无功功率
  		 	    0x02040300,  //C相无功功率
  		 	    0x0204FF00,  //无功功率数据块
  		 	    0x02060000,  //总功率因素
  		 	    0x02060100,  //A相功率因素
  		 	    0x02060200,  //B相功率因素
  		 	    0x02060300,  //C相功率因素
  		 	    0x0206FF00,  //功率因素数据块
  		 	    0x00010000,  //正向有功总电能
  		 	    0x00020000,  //反向有功总电能
  		 	    0x00030000,  //组合无功1总电能
  		 	    0x00040000,  //组合无功2总电能	
  		 	    0x00050000,  //第一像限无功总电能
  		 	    0x00060000,  //第二像限无功总电能
  		 	    0x00070000,  //第三像限无功总电能
  		 	    0x00080000,  //第四像限无功总电能	
  		 	    0x02800004,  //当前有功需量
  		 	    0x02800005 } //当前无功需量
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
	.ident =  { 0x02010100,  //A相电压
  		 	    0x02010200,  //B相电压
  		 	    0x02010300,  //C相电压
  		 	    0x0201FF00,  //电压数据块	
  		 	    0x02020100,  //A相电流
  		 	    0x02020200,  //B相电流
  		 	    0x02020300,  //C相电流
  		 	    0x0202FF00,  //电流数据块
  		 	    0x02030000,  //总有功功率
  		 	    0x02030100,  //A相有功功率
  		 	    0x02030200,  //B相有功功率
  		 	    0x02030300,  //C相有功功率
  		 	    0x0203FF00,  //有功功率数据块
  		 	    0x02040000,  //总无功功率
  		 	    0x02040100,  //A相无功功率
  		 	    0x02040200,  //B相无功功率
  		 	    0x02040300,  //C相无功功率
  		 	    0x0204FF00,  //无功功率数据块
  		 	    0x02060000,  //总功率因素
  		 	    0x02060100,  //A相功率因素
  		 	    0x02060200,  //B相功率因素
  		 	    0x02060300,  //C相功率因素
  		 	    0x0206FF00,  //功率因素数据块
  		 	    0x00010000,  //正向有功总电能
  		 	    0x00020000,  //反向有功总电能
  		 	    0x00030000,  //组合无功1总电能
  		 	    0x00040000,  //组合无功2总电能	
  		 	    0x00050000,  //第一像限无功总电能
  		 	    0x00060000,  //第二像限无功总电能
  		 	    0x00070000,  //第三像限无功总电能
  		 	    0x00080000,  //第四像限无功总电能	
  		 	    0x02800004,  //当前有功需量
  		 	    0x02800005,  //当前无功需量
                0x02800001}  //零线电流
#endif
};

extern ST_STA_ATTR_TYPE StaAttr;
extern ST_ROUTE_TAB_TYPE StaRouteTab;

#ifdef ENABLE_PROTOCOL_FLASH
//双备份存储
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

//双备份读取
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
		if (store_head.DataLen>dlen)//固定区域长度只许增不许减
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
		if (store_head.DataLen>dlen)//固定区域长度只许增不许减
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
	if (i >= 3) //备份区也没有内容
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
		if (i >= 3)//原地址无法写入  复位
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
				//不擦reset后每次需要刷新17次  最后的一次在整个16个块中循环
				FLASH_EarseBlock(FLASH_FLUSH_ADDR + ((StaSysPara.SofeRstCnt>>4)&0xf) * DATA_FLASH_SECTION_SIZE);
				DataFlashClose();
			}
		}
	}
}
*/

//存储网络参数
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

//读取网络参数
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

//存储停电参数
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

//读取停电参数
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

//存储PLL Trimming 值
void StoragePllTrim(s32 trim)
{

#ifdef II_STA
    if ((trim > 1280) || (trim < -1280)) //trim单位1/16ppm，正负80ppm
#else
    if ((trim > 400) || (trim < -400)) //trim单位1/16ppm，正负25ppm
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

//读取PLL Trimming 值
void ReadPllTrim(void)
{
	int default_pll=-10000;
	ReadDoubleFlash(STORE_FIX_PLL,DATA_PLL_TRIM_STORE_ADDR, &PLL_Trimming, sizeof(PLL_Trimming), &default_pll);
#ifdef II_STA
	if ((PLL_Trimming > 1280) || (PLL_Trimming < -1280)) //trim单位1/16ppm，正负80ppm
#else	    
	if ((PLL_Trimming > 400) || (PLL_Trimming < -400)) //PLL_Trimming单位1/16ppm，正负25ppm
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
		//已经搜索到了最大
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
//存储硬件参数
void StorageStaParaInfo(void)
{
	ST_FLASH_STORE_HEAD store_head;
	if(BootPowerState == 1)
	{	
		memcpy(&sta_temp,&StaSysPara,sizeof(StaSysPara));
		ReadStaParaInfo();
		if (memcmp(&sta_temp.AssocRandom, &StaSysPara.AssocRandom,sizeof(StaSysPara)-4)==0)//只比较除复位次数以外的区域
		{
			memcpy(&StaSysPara,&sta_temp,sizeof(StaSysPara));//还原并返回
			return;
		}
		memcpy(&StaSysPara,&sta_temp,sizeof(StaSysPara));//还原并存储
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

//读取硬件参数
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
	if (StaSysPara.LastFreq >= 3) //频段不在在南网支持范围内，使用默认频段
	{
		StaSysPara.LastFreq = DefaultStaSysPara.LastFreq;
		debug_str(DEBUG_LOG_INFO, "ReadStaParaInfo, last freq:%d, change to freq:%d\r\n", StaSysPara.LastFreq, DefaultStaSysPara.LastFreq);
	}
	//由于存储此参数时需要重新读取后比较 因此低于长度的部分需要被清零防止比较时 没有真正从flash中读取
	if(sizeof(StaSysPara)>store_head.DataLen)
	{
		memset(&((u8*)(&StaSysPara))[store_head.DataLen],0,sizeof(StaSysPara)-store_head.DataLen);
	}
}

//存储路由参数
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

//读取路由参数
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
//双备份区域

//存储固定参数
void StorageStaFixPara(void)
{
	StorageDoubleFlash(STORE_FIX_PARA,DATA_STA_FIX_PARA_STORE_ADDR,&StaFixPara,sizeof(StaFixPara));
}
//读取固定参数
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
//存储芯片ID
void StorageStaIdInfo(void)
{
	StorageDoubleFlash(STORE_HEAD_ID_INFO_FLG,DATA_STA_ID_INFO_STORE_ADDR,&StaChipID,sizeof(StaChipID));
}

//读取芯片ID
void ReadStaIdInfo(void)
{
	ReadDoubleFlash(STORE_HEAD_ID_INFO_FLG,DATA_STA_ID_INFO_STORE_ADDR,&StaChipID,sizeof(StaChipID),&StaChipIDDefult);
}

//存储版本信息
void StorageVerInfo(void)
{
	StorageDoubleFlash(STORE_HEAD_ID_INFO_FLG,DATA_VER_INFO_STORE_ADDR,&StaVersion,sizeof(StaVersion));
}

//读取版本信息
void ReadVerInfo(void)
{
	ReadDoubleFlash(STORE_HEAD_ID_INFO_FLG,DATA_VER_INFO_STORE_ADDR,&StaVersion,sizeof(StaVersion),&DefaultStaVersion);

#ifdef FIXED_DEFAULT_VER_INFO
    memcpy(StaVersion.SofeVer, DefaultStaVersion.SofeVer, sizeof(StaVersion.SofeVer));
    memcpy(&StaVersion.VerDate, &DefaultStaVersion.VerDate, sizeof(StaVersion.VerDate));
#endif
}

//存储版本信息
void StorageExtVerInfo(void)
{
	StorageDoubleFlash(STORE_HEAD_ID_INFO_FLG,DATA_EXT_VER_INFO_STORE_ADDR, &StaExtVersion, sizeof(StaExtVersion));
}

//读取版本信息
void ReadExtVerInfo(void)
{
	ReadDoubleFlash(STORE_HEAD_ID_INFO_FLG,DATA_EXT_VER_INFO_STORE_ADDR, &StaExtVersion, sizeof(StaExtVersion), &DefaultStaExtVersion);
}

//************************************************************


Time_Module_s Time_Module[3];//0存数据 1存偏差 3 存储表时间
Graph_Config_s  Graph_Config;

u32 Data2Sec(Time_Special* ptime)
{
	u32 temp;
	ptime->tm_isdst=0;
	temp=mktime(ptime);
	return temp;
}
//增加秒数 
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



//存储参数
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

//读取参数
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

    if ((Graph_Config.num[0] == 0) && (Graph_Config.num[1] == 0)) //STA_READ_GRAPH_PARA中读取Graph_Config有误
    {
        ConfigCSG21DeafultGraphPara();
    }
    else //只保留21标准要求的负荷曲线采集周期
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

//配置南网21标准默认曲线抄读配置
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

//重新配置南网21标准默认曲线抄读配置，只保留周期
void ReConfigCSG21GraphPara(void)
{
	u8 index = (myDevicType == METER)?0:1;

    Graph_Config.config_flag = DefaultGraphConfig.config_flag;
	Graph_Config.num[index] = DefaultGraphConfig.num[0];
	memcpy(&Graph_Config.mode[index], &DefaultGraphConfig.mode[0], MAC_GRAPH_CONFIG_NUM);
	memcpy(&Graph_Config.ident[index], &DefaultGraphConfig.ident[0], 4*MAC_GRAPH_CONFIG_NUM);

    if (memIsHex((u8*)&Graph_Config.period[index], 0x00, MAC_GRAPH_CONFIG_NUM)) //周期为0赋默认值
    {
        memcpy(&Graph_Config.period[index], &DefaultGraphConfig.period[0], MAC_GRAPH_CONFIG_NUM);
    }
    else
    {
        //周期根据数据标识数量重新赋值，规避新老版本负荷曲线数据标识发生变化
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
//擦除某个时间点以后的数据
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
			if (pdata_max->cs != GetGraphCs(pdata_max)) //校验码不对 整个4k的后面全部不可用
			{
				break;
			}
			if ((u32)pdata_max  > max_addr) //可以存储的位置超出了4K范围
			{
				break;
			}
			if (pdata_max->sec_from_1900>=time)//擦除本块
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
	if (para->flag==UP_NEED_PRO_FLASH)//本次升级文件中携带了需要对falsh进行操作的信息
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
				if (parea->all_used == 0xffffffff) //未使用完
				{
					Graph_Data_s *pdata = (Graph_Data_s *)parea->data;
					Graph_Data_s *firstGraph=pdata;
					u32 end_sec=0;
					u8 lockThisBolck=0;
					while (pdata->sec_from_1900 != 0xffffffff) //此处未被使用
					{
						if (pdata->cs!=GetGraphCs(pdata))//校验码不对 整个4k的后面全部不可用
						{
							lockThisBolck = 1;
							pdata = (Graph_Data_s *)((u32)parea+DATA_FLASH_SECTION_SIZE);//把曲线指针指向结尾
							break;
						}
						end_sec=pdata->sec_from_1900;
						if (alloc_sec < pdata->sec_from_1900)//申请的时间小于块内的时间
						{
							lockThisBolck=1;//需要封堵这个块
						}
						if (firstGraph->sec_from_1900 > pdata->sec_from_1900) //这一帧的数据的时间戳比头部分的时间戳小
						{
							lockThisBolck=1;//需要封堵这个块
						}
						if (((u32)pdata - (u32)parea + len) >= DATA_FLASH_SECTION_SIZE) //可以存储的位置超出了4K范围
						{
							lockThisBolck=1;//需要封堵这个块
						}
						if (((u32)pdata - (u32)parea + sizeof(Graph_Data_s)) >= DATA_FLASH_SECTION_SIZE)//下一个已经不足以存储一个块
						{
							lockThisBolck=1;
							break;
						}
						pdata = (Graph_Data_s *)((u32)pdata + pdata->len + sizeof(Graph_Data_s));
					}
					if (lockThisBolck||(((u32)pdata - (u32)parea + len) >= DATA_FLASH_SECTION_SIZE)) //可以存储的位置超出了4K范围
					{
						//if (((u32)pdata - (u32)parea)<DATA_FLASH_SECTION_SIZE)//剩余的空间不够存储一帧数据
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
	//找到所有小于等于要求时间的4k块中时间最大的 即最接近当前时间的块
	for (int i = 0; i < (STA_READ_GRAPH_DATA_SIZE / DATA_FLASH_SECTION_SIZE); i++)
	{
		if(save_block_ok(parea))
		{
			pdata = (Graph_Data_s *)parea->data;
			if (GetGraphCs(pdata) != pdata->cs)//第一个数据项的cs就是不对的
			{
				parea =(Flash_4K_Area*)((u32)parea+DATA_FLASH_SECTION_SIZE);
				continue ;
			}
			if(parea->all_used==STORE_GRAPG_FLG)//已经封锁
			{
				if(parea->end_sec < sec_from_2000)//抄读的时间大于末尾的时间
				{
					parea = (Flash_4K_Area*)((u32)parea+DATA_FLASH_SECTION_SIZE);
					continue;
				}
			}
			if (pdata->sec_from_1900 <= sec_from_2000)
			{			
				temp.num = ((Flash_4K_Area *)((u32)pdata - sizeof(Flash_4K_Area)))->used_num;
				temp.point=pdata;
				for (int j=0;j<MAX_GH_CNT;j++)//小于设置时间的最新的块
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
			parea = (Flash_4K_Area*)((u32)pdata_real&0xfffff000);//向4k对其即为此区域开始地址
			while ((u32)pdata_real<(u32)parea+DATA_FLASH_SECTION_SIZE)
			{
				if (pdata_real->sec_from_1900>sec_from_2000)
				{
					break;
				}
				if (pdata_real->sec_from_1900 == sec_from_2000 && pdata_real->ident == ident)
				{
					if (GetGraphCs(pdata_real) == pdata_real->cs)//校验通过
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

//读取flash中站点上一次入网的频段
void StaLastFreqProcess(void)
{
	if (StaSysPara.LastFreq <= 3)
	{
		ResetFcCnt();
#if defined(GUANGXI_CSG)
		ResetChangeShortFreTimer();
#elif defined(GUANGDONG_CSG)
		ResetChangeShorterFreTimer();//清远需要90秒内组网 测试频段1功率谱密度
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
			//未入网情况下，优先选择上一次入网频段
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
	//得到真实频段
		
		debug_str(DEBUG_LOG_NET,"Current Band plan is %d\r\n", StaSysPara.LastFreq);
	}
}
//读取flash中站点上一次入网的无线信道
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

//基于发射功率模式，赋值HPLC_ChlPower
void StaTxPowerProcess(void)
{
	if(StaFixPara.txPowerMode[0] == 0) //发射功率自动切换模式
	{
		memcpy(HPLC_ChlPower,HPLC_HighPower, 4);
	}
	else
	{
		memcpy(HPLC_ChlPower, StaFixPara.txPower, 4);
	}
	
	if(StaFixPara.txPowerMode[1] == 0) //发射功率自动切换模式
	{
		memcpy(HRF_ChlPower, HRF_HighPower, 2);
	}
	else
	{
		memcpy(HRF_ChlPower, StaFixPara.hrfTxPower, 2);
	}
}


//读取flash中串口波特率
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
		/****************顺序调整和国网一致********************/	
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
//记录上一次入网的电表地址
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

#ifdef II_STA //规避采集器没搜到表时没有版本信息的问题
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
	if (StaSysPara.LastFreq >= 3) //频段不在在南网支持范围内，使用默认频段
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
	Graph_Config.config_flag = 0; //南网21标准默认关闭曲线采集，由信标万年历条目触发
#endif
#ifdef II_STA //规避采集器没搜到表时没有版本信息的问题
	ReadStaIdInfo();
	ReadVerInfo();
#endif
}
#endif



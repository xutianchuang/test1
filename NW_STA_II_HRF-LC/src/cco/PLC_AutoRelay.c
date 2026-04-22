#include "system_inc.h"
//#include "../hal/macrodriver.h"
#include "manager_link.h"
#include "RTC.h"
//#include "../drv/uart_link.h"
#include "alert.h"
#include "queue.h"
#include "module_def.h"
//#include "data_types.h"
#include "plm_2005.h"
//#include "../app/PLM_AFN.h"
//#include "../app/storage.h"
//#include "../dev/dataflash/data_flash.h"
#include "plc_autorelay.h"
//#include "../dev/plcboard.h"
#include "os_cfg_app.h"
#include "hplc_send.h"
#include <string.h>
//#include <stdlib.h>
//#include "plc_mfrelay.h"
#include <stdio.h>
#include "Revision.h"
#include "Uart_Link.h"

#include "CCO_app.h"
#include "hplc_app_def.h"
#include "hrf_port.h"
//#include "test.h"

#ifdef CHECK_MAC_HEAD_VER
const unsigned char check_mac_head_ver = 1; //判断MAC帧头中版本号
#else
const unsigned char check_mac_head_ver = 0; //不判断MAC帧头中版本号
#endif

#ifdef BEACON_ADAPT_TC_STA //信标中保留字段适配鼎信STA(TopsComm)，修改鼎信STA性能
const unsigned char beacon_adapt_TC_sta = 1;
#else
const unsigned char beacon_adapt_TC_sta = 0;
#endif

#ifdef UPDATE_REQ_STATUS_UNICAST //使用单播查询站点升级状态
const unsigned char update_reg_status_unicast = 1;
#else
const unsigned char update_reg_status_unicast = 0;
#endif

#ifdef GDW_HENAN
const unsigned int update_timeout_minutes = 300; //升级流程超时时间（单位分钟），栾川现场测试局方要求90分钟完成升级
#else
const unsigned int update_timeout_minutes = 300;
#endif

const pPLC_AssocReqCheckAuthFunc PLC_AssocReqCheckAuthFunc = PLC_AssocReqCheckAuth;
const pPLC_AssocCnfAddAuthFunc PLC_AssocCnfAddAuthFunc = PLC_AssocCnfAddAuth;

u32 nodeinfo_bitmap[MAX_BITMAP_SIZE/4];
u32 nodeinfo_report_bitmap[MAX_BITMAP_SIZE/4];

#ifdef TEST_COLLECTION
const unsigned int collect_search_interval = 60*1000;     //采集器搜表间隔60秒，单位微秒
#else
const unsigned int collect_search_interval = 10*60*1000;  //采集器搜表间隔15分钟，单位微秒
#endif

#ifdef GW_BEACON_ITEM_UTC
u16 getCentreTimerId = INVAILD_TMR_ID;
#endif
extern const unsigned char update_reg_status_unicast;
extern const unsigned int update_timeout_minutes;
extern const unsigned int collect_search_interval;

struct
{
	u8 mac[MAX_CCO_NUM][6];
	u8 idx;
}OtherCcoAddr;



OS_MUTEX mutex_sta_info;
//u8 PlcRxWaitQueueMem[PLC_RX_WAIT_QUEUE_SIZE*(sizeof(queue)-sizeof(HANDLE))];
u8 PlcRxWaitQueueMem[sizeof(queue)-sizeof(HANDLE)+ PLC_RX_WAIT_QUEUE_SIZE*sizeof(HANDLE)];
extern U32 PlcBroadcastQueueMem[];

MSG_SHORT_INFO g_mf_Dl645Msg;

MSG_SHORT_INFO g_mf_DirectReadMsg;

queue *g_PlcWaitRxQueue = NULL;         //并发抄表等待队列
queue *g_PlcBroadcastQueue = NULL;

unsigned char g_mf_report_rmsg = 4;


RELAY_STATUS        g_RelayStatus;
#ifdef NET_FLAG_FIX
unsigned char net_flag_fix = 0;
#endif
RELAY_INFO          g_MeterRelayInfo[CCTT_METER_NUMBER];
STA_INFO            g_StaInfo[TEI_CNT];
STA_STATUS_INFO     g_StaStatusInfo[TEI_CNT];
#ifdef GDW_ZHEJIANG
BLACK_RELAY_INFO    g_BlackMeterInfo[CCTT_BLACK_METER_NUMBER];
#endif //GDW_ZHEJIANG

#ifdef PLC_X4_FUNC
X4_READ_TASK_RECORD g_X4ReadTaskInfo[CCTT_X4_READ_TASK_INFO_NUMBER];
X4_CLL_Task_C       g_X4_CLL_Task[CCTT_X4_CLL_TASK_NUM_MAX];
#endif

P_GLOBAL_CFG_PARA   g_pPlmConfigPara = &g_PlmConfigPara;
P_RELAY_STATUS      g_pRelayStatus   = &g_RelayStatus;
P_RELAY_INFO        g_pMeterRelayInfo = g_MeterRelayInfo;
P_STA_INFO          g_pStaInfo = g_StaInfo;
P_STA_STATUS_INFO   g_pStaStatusInfo = g_StaStatusInfo;       //STA_STATUS_INFO_BY_TEI
#ifdef PROTOCOL_GD_2016
P_METER_COLL_MAP    g_pMeterCollMap = (P_METER_COLL_MAP)(METER_COLL_DB_ADDR+FLASH_AHB_ADDR);
#endif

#ifdef GDW_2019_GRAPH_STOREDATA
//陕西全网感知要求
u32 goto_online_bitmap[MAX_BITMAP_SIZE/4];
u32 goto_offline_bitmap[MAX_BITMAP_SIZE/4];
u32 offline_reason[MAX_BITMAP_SIZE/4];
#endif

#define CON_PLC_RESEND_COUNT 3
#define CON_REASSOC_TIME_MS  (60000)

#if defined(PERFORMAINCE_OUT) || defined(GDW_JILIN_SJ)
#define CON_REASSOC_MUTI_TIME_MS  (60000)
#else
#define CON_REASSOC_MUTI_TIME_MS  (60000*7)
#endif

#define TMM_VER_FUN_CHAR    'E'



//内部版本
#define TMM_VERSION_INNER         {0x01, 0x98}       /*不能被更改，用于升级确定是否1606E存储芯片*/


#ifndef HPLC_CSG
#define ASSOC_CNF_MAC_FRAME (520-8)
#define ASSOC_CNF_MSDU_LEN (ASSOC_CNF_MAC_FRAME-32-4)
#else
#define ASSOC_CNF_MAC_FRAME (520-8)
#define ASSOC_CNF_MSDU_LEN (ASSOC_CNF_MAC_FRAME-32-4)
u32 selfreg_bitmap[MAX_BITMAP_SIZE/4];
#endif
u8  ReaderPhase=0;
TMN_VER_INFO c_Tmn_ver_info;

__weak const TMN_VER_INFO c_Tmn_ver_info_default =
{
#ifdef ZHENGTAI
	{ FACTORY_CODE },    //厂商代号
	{ CHIP_CODE },       //芯片代码
	{ SLG_COMMIT_DATE, SLG_COMMIT_MON, SLG_COMMIT_YEAR % 100,  },
	{ SLG_COMMIT_HASH >> 20, (SLG_COMMIT_HASH >> 12) & 0xff },
	
#elif defined (NW_TEST)
    { 'N', 'W' },  //厂商代码
    { 'N', 'W' },  //芯片代码
    { 25, 4, 21,  },
    { 0, 1 },

#elif ZH_CCO
#ifdef HPLC_CSG //南网
#ifdef GUIZHOU_SJ //贵州送检
	{ 'g', 'z' },    //厂商代码
	{ 'd', 'l' },  //芯片代码
#elif defined GUANGXI_CSG //广西出货
	{ 'S', 'W' },    //厂商代码
	{ '1', '2' },  //芯片代码
#else
	{ FACTORY_CODE },   //厂商代码
	{ CHIP_CODE },  //芯片代码
#endif

#else //国网
#if defined(GDW_JILIN)
	{ 'S', 'K' },    //厂商代码
#elif defined(GDW_SHANDONG)
	{ 'W', 'S' },    //厂商代码
#else
	{ FACTORY_CODE },    //厂商代码
#endif

#if 1
	{ CHIP_CODE },  //芯片代码
#else
#if defined (GDW_JILIN)||defined(GDW_HENAN)||defined(GDW_SHANDONG)
	{ 'W', '1' },  //芯片代码
#elif defined( GDW_HUNAN)
	{ 'S', '5' },  //芯片代码
#else
	{ '5', 'C' },  //芯片代码
#endif
#endif
#endif

#ifdef HPLC_CSG //南网
#ifdef GUIZHOU_SJ
	{ 2, 7, 20,  },
	{ 0, 0 },
#else
#ifdef MODULE_SOFT_VER
	{ MODULE_SOFT_VER_DATE_DAY, MODULE_SOFT_VER_DATE_MON, MODULE_SOFT_VER_DATE_YEAR,  },
	{ (MODULE_SOFT_VER >> 8) & 0xff, MODULE_SOFT_VER & 0xff },
#else
	{ 26, 10, 20,  },
	{ 1, 1 },
#endif
#endif
#else //国网
#ifdef 	GDW_SHAN3XI
	{ 11, 6, 20,  },
	{ 0, 3 },
#elif defined GDW_HUNAN
	{ 7, 1, 21,  },
	{ 43, 1 },
#elif defined GDW_HENAN
	{  5, 11, 20,  },
	{ 43, 1 },
#elif defined (GDW_JILIN)
	{ 1, 6, 20,  },
	{ 7, 2 },
#elif defined (GDW_CHONGQING)
	{ 18, 10, 20,  },
	{ 27, 1 },
#elif defined (GDW_SHANDONG)
	{ 2, 11, 20,  },
	{ 2, 7 },
#elif defined (GDW_BEIJING)
	{ 27, 6, 20,  },
	{ 0, 1 },
#elif defined (OVERSEA)
	{ 1, 3, 21,  },
	{ 0, 1 },
#else
	{ 26, 10, 23,  },
	{ 0, 1 },
#endif
#endif

#elif defined(GS_CCO)
#ifdef HPLC_GDW //国网
#ifdef FACTORY_CODE
        { FACTORY_CODE },  //厂商代码
#else
	{ 'G', 'S' },  //厂商代码
#endif
#ifdef CHIP_CODE
        { CHIP_CODE },  //芯片代码
#else
    { 'Z', '2' },  //芯片代码
#endif
#else //南网（南网科技）
	{ FACTORY_CODE },    //厂商代号
	{ CHIP_CODE },    //芯片代码
#endif

#ifdef GUANGDONG_CSG //广东供货
  #ifdef GUANGDONG_CSG_24_1
    //24年1批
    { 10, 10, 24  },
    { 24, 13 },
  #elif defined(GUANGDONG_CSG_23_1)
    //23年1批
    { 1, 9, 23  },
    { 23, 10 },
  #elif defined(GUANGDONG_CSG_23_2)
    //23年2批
    { 10, 1, 24  },
    { 23, 23 },
  #elif defined(GUANGDONG_CSG_24_2)
    //24年2批
    { 20, 3, 25  },	
    { 24, 27 },
  #else
    //默认用最新的，24年2批
    { 20, 3, 25  },
    { 24, 27 },
  #endif
#elif defined(YUNAN_CSG)
    { 26, 5, 23  },
	{ 0, 1 },
#else
	{ MODULE_SOFT_VER_DATE_DAY, MODULE_SOFT_VER_DATE_MON, MODULE_SOFT_VER_DATE_YEAR,  },
	{ (MODULE_SOFT_VER >> 8) & 0xff, MODULE_SOFT_VER & 0xff },
#endif

#elif ZT_CCO
    { FACTORY_CODE },    //厂商代号
	{ CHIP_CODE },    //芯片代码
#ifdef HPLC_GDW
	{ 29, 3, 21  },
	{ 0, 1 },
#else

#ifdef GUANGDONG_CSG //广东供货
  #ifdef GUANGDONG_CSG_24_1
    //24年1批
    { 10, 10, 24  },
    { 24, 13 },
  #elif defined(GUANGDONG_CSG_23_1)
    //23年1批
    { 1, 9, 23  },
    { 23, 10 },
  #elif defined(GUANGDONG_CSG_23_2)
    //23年2批
    { 10, 1, 24  },
    { 23, 23 },
  #elif defined(GUANGDONG_CSG_24_2)
    //24年2批
    { 20, 3, 25  },
    { 24, 27 },
  #else
    //默认用最新的，24年2批
    { 20, 3, 25  },
    { 24, 27 },
  #endif
#else

    { 1, 11, 21  },
	{ 0, 1 },
#endif

#endif
#elif MD_CCO
	{ FACTORY_CODE },    //厂商代号
	{ CHIP_CODE },    //芯片代码
	{ 1, 11, 21  },
	{ 0, 1 },
#elif YD_CCO
	{ FACTORY_CODE },    //厂商代号
	{ CHIP_CODE },    //芯片代码
	{ 23, 2, 23  },
	{ 0, 1 },
#elif LQ_CCO
	{ FACTORY_CODE },    //厂商代号
	{ CHIP_CODE },    //芯片代码
	{ 4, 5, 23  },
	{ 0, 1 },
#else
	{ FACTORY_CODE },    //厂商代号
	{ CHIP_CODE },  //设备编号
	{ MODULE_SOFT_VER_DATE_DAY, MODULE_SOFT_VER_DATE_MON, MODULE_SOFT_VER_DATE_YEAR,  },
	{ (MODULE_SOFT_VER >> 8) & 0xff, MODULE_SOFT_VER & 0xff },
#endif
//   TMM_VER_INFO_SF,
//   TMM_VERSION,
	0,
};

const TMN_VER_INFO c_Tmn_ver_info_inner =
{
	{ FACTORY_CODE },    //厂商代号
	{ CHIP_CODE },  //设备编号
	{ SLG_COMMIT_YEAR % 100, SLG_COMMIT_MON, SLG_COMMIT_DATE },
	TMM_VERSION_INNER,
	0,
};
#ifdef HPLC_CSG
EXT_VER_INFO c_ext_ver_info;

//默认扩展版本信息
__weak const EXT_VER_INFO c_ext_ver_info_default = 
{
    .write_flag = 0,
        
#ifdef NW_TEST
	//模块硬件版本信息
	.ModuleHardVer = { 0x00, 0x01},
	.ModuleHardVerDate.year = 21,
	.ModuleHardVerDate.month = 4,
	.ModuleHardVerDate.day = 25,

	//芯片硬件版本信息
	.ChipHardVer = { 0x00, 0x01},
	.ChipHardVerDate.year = 21,
	.ChipHardVerDate.month = 4,
	.ChipHardVerDate.day = 25,

	//芯片软件版本信息
	.ChipSoftVer = { 0x00, 0x01},
	.ChipSofVerDate.year = 21,
	.ChipSofVerDate.month = 4,
	.ChipSofVerDate.day = 25,

	//应用程序版本号
	.AppVer = { 0x00, 0x11},	
#else
	//模块硬件版本信息
	.ModuleHardVer = { (MODULE_HARD_VER >> 8) & 0xff, MODULE_HARD_VER & 0xff },
	.ModuleHardVerDate.year = MODULE_HARD_VER_DATE_YEAR,
	.ModuleHardVerDate.month = MODULE_HARD_VER_DATE_MON,
	.ModuleHardVerDate.day = MODULE_HARD_VER_DATE_DAY,

	//芯片硬件版本信息
	.ChipHardVer = { (CHIP_HARD_VER >> 8) & 0xff, CHIP_HARD_VER & 0xff },
	.ChipHardVerDate.year = CHIP_HARD_VER_DATE_YEAR,
	.ChipHardVerDate.month = CHIP_HARD_VER_DATE_MON,
	.ChipHardVerDate.day = CHIP_HARD_VER_DATE_DAY,

	//芯片软件版本信息
	.ChipSoftVer = { (CHIP_SOFT_VER >> 8) & 0xff, CHIP_SOFT_VER & 0xff },
	.ChipSofVerDate.year = CHIP_SOFT_VER_DATE_YEAR,
	.ChipSofVerDate.month = CHIP_SOFT_VER_DATE_MON,
	.ChipSofVerDate.day = CHIP_SOFT_VER_DATE_DAY,

	//应用程序版本号
	.AppVer = { (APP_VER >> 8) & 0xff, APP_VER & 0xff },
#endif
};
#endif
const u8 MaxHrfChannel[3]=
{
	0,79,199
};
#ifdef HPLC_CSG
#define CNANNEL_OFFSET  0
#else
#define CNANNEL_OFFSET  0
#endif
const u8 BatterHrfChannel[3][20]={
	{0},
	{8-CNANNEL_OFFSET,40-CNANNEL_OFFSET,16-CNANNEL_OFFSET,44-CNANNEL_OFFSET,24-CNANNEL_OFFSET,48-CNANNEL_OFFSET,32-CNANNEL_OFFSET,56-CNANNEL_OFFSET,36-CNANNEL_OFFSET,64-CNANNEL_OFFSET,  4-CNANNEL_OFFSET,46-CNANNEL_OFFSET,20-CNANNEL_OFFSET,50-CNANNEL_OFFSET,28-CNANNEL_OFFSET,52-CNANNEL_OFFSET,38-CNANNEL_OFFSET,54-CNANNEL_OFFSET,42-CNANNEL_OFFSET,69-CNANNEL_OFFSET},
	{41-CNANNEL_OFFSET,101-CNANNEL_OFFSET,61-CNANNEL_OFFSET,107-CNANNEL_OFFSET,81-CNANNEL_OFFSET,121-CNANNEL_OFFSET,91-CNANNEL_OFFSET,141-CNANNEL_OFFSET,97-CNANNEL_OFFSET,161-CNANNEL_OFFSET,  30-CNANNEL_OFFSET,117-CNANNEL_OFFSET,50-CNANNEL_OFFSET,128-CNANNEL_OFFSET,70-CNANNEL_OFFSET,135-CNANNEL_OFFSET,86-CNANNEL_OFFSET,147-CNANNEL_OFFSET,112-CNANNEL_OFFSET,180-CNANNEL_OFFSET},
	//{7,16,8,32,23,48,24,64,39,79,           1,40,17,55,13,56,49,71,65,72,},
	//{1,40,41,80,81,120,121,160,161,199,    21,22,61,62,100,101,140,141,180,181}
};
const ST_CCO_ID_TYPE g_cco_id_default = 
{
	.chip_id        = { 0x01,0x00,0x00,0x10,0x10,0xFF }, //小端，目前LC产测小端输入
#ifdef HPLC_CSG
	.module_id      = {'0','0','0','0','0','0','0','0',
					   '0','0','0','0','0','0','0','0',
					   '0','0','0','0','0','0','0','0'}
#endif
};

TMN_VER_INFO plc_ver_info;      //通道版

const unsigned long DI_PACK[] =
{
	0x05060001, //日冻结时间 5
	0x05060101, //日冻结数据 20

	0x0001ff00, //当前正向有功  20
	0x04000101, //当前日期
	0x040005ff, //状态字
	0x070002ff, //剩余金额，购电次数
	0x070102ff, //0xFF,0x02,0x01,0x07,//下发购电，控制码0x03   32 35 34 3A

	0x05060201, //（上 1 次）日冻结反向有功电能数据
	0x0001ff01, //上一结算日当前正向有功  20
	0x0002ff01, //上一结算日反向有功总电能

	0x0002ff00, //反向有功总电能
	0x0203ff00,
	0x0204ff00,
	0x0206ff00,
	0x0201ff00,
	0x0202ff00,
	0x02800001,

	0x03330201, //0x01,0x02,0x33,0x03,//购电次数
	0x00900200, //0x00,0x02,0x90,0x00,//剩余金额
	0x03330601  //0x01,0x06,0x33,0x03,//累计购电金额
};

#ifdef APP_READ_SPECIAL_DI
//列表中的DI允许抄读，否则不抄读，回复异常应答或任务超时
const unsigned long DI_READABLE[] =
{
	0x9010,     //当前正向有功总电量示值
	0x901f,     //当前正向有功总电量示值，数据块
	0x9020,     //当前反向有功总电量示值
	0x902f,     //当前反向有功电能示值（总、费率1～M）
	0x9e1f,     // （上1次）日冻结正向有功电能数据，广西
	0x9e2f,     // （上1次）日冻结反向有功电能数据，广西
	0xde1f,     // （上1次）日冻结正向有功电能数据
	0xde2f,     // （上1次）日冻结反向有功电能数据

	0x05060001,     //日冻结时间
//    0x05060002,     //日冻结时间
//    0x05060003,     //日冻结时间
//    0x05060004,     //日冻结时间
//    0x05040000,     // （上1次）日冻结正向有功电能数据，甘肃
	0x05060101,     // （上1次）日冻结正向有功电能数据
#if 1
	0x05060201,     // （上1次）日冻结反向有功电能数据

#if 1
	0x00010000,     //当前正向有功总电量示值
	0x0001ff00,     //当前正向有功总电量示值
	0x00020000,     //当前反向有功总电量示值
	0x0002ff00,     //当前反向有功总电量示值
	0x0000ff00,     //(当前)组合有功电能数据块
//    0x0203ff00,     //瞬时有功功率数据块
#endif

	0x0001ff01,     // （上1次）月冻结正向有功电能数据
	0x0002ff01,     // （上1次）月冻结反向有功电能数据
	0x0000ff01,     //(上 1 结算日)组合有功电能数据块
#endif
};
#endif


int macVaileCmp(u8 *macA, u8 *macB)
{
#if 0
	u32 vaileA[2],vaileB[2];
	u32 temp=*(u32*)macA;
	temp=*(u32*)&macA[4];
	vaileA[1]=__REV(temp);
	vaileA[1]>>=16;

	temp=*(u32*)macB;
	vaileB[0]=__REV(temp);
	temp=*(u32*)&macB[4];
	vaileB[1]=__REV(temp);
	vaileB[1]>>=16;
	int64_t diff=*(int64_t*)vaileA-*(int64_t*)vaileB;
#else
	uint64_t vaileA=(*(int64_t*)macA)&0xffffffffffff;
	uint64_t vaileB=(*(int64_t*)macB)&0xffffffffffff;
	int64_t diff=vaileA-vaileB;
#endif	
	
	if (diff>0)
	{
		return 1;
	}
	else if (diff<0)
	{
		return -1;
	}
	else
	{
		return 0;
	}

	
}




/*
extern METER_DATA_STORAGE_BODY g_Meter_Data_1[626];
extern METER_DATA_STORAGE_BODY g_Meter_Data_2[106];
extern METER_DATA_STORAGE_BODY g_Meter_Data_3[366];
extern METER_DATA_STORAGE_BODY g_Meter_Data_4[403];
*/

METER_DATA_STORAGE g_MeterDataStorages[] = {
	{ 0, NULL },
//    {countof(g_Meter_Data_2), g_Meter_Data_2},
};

u8 PLC_Get_Phase_By_Relay(USHORT metersn, PBYTE pRelayAddr, u8 level);

void PLC_HandleBeaconFrame(const P_HPLC_PARAM pHplcParam, P_BEACON_ZONE pBeaconZone, P_BEACON_MPDU pBeaconPayload);
unsigned char PLC_GetMacAddrByMacFrame(P_MAC_HEAD pMacHead, U8 mac_addr[MAC_ADDR_LEN]);
void PLC_RemoveRxQueueOldItem();
P_STA_BITMAP PLC_GetAllSubStas(USHORT tei);
ASSOC_RESULT PLC_SmartSelectPco(u16 tei, u16 old_pco, TEI_ZONE candidata_pco[5], TEI_ZONE *p_pco_selected);
//BOOL PLC_RefixedRoles(u16 tei);
//void PLC_TimerRefixedRoles();
HPLC_PHASE PLC_GetHplcPhaseByReport(u32 phase_report);

#if defined(PROTOCOL_GD_2016)
P_READ_TASK_INFO PLC_find_read_task_gd(EM_TASK_TYPE task_type, u16 taskid);
BOOL PLC_fetch_task_read_item_gd(P_READ_TASK_INFO *ppReadTask,bool isMultiTask ,u16 *taskid,u8 taskmac[][MAC_ADDR_LEN],u8 max_num);
void PLC_read_task_timeout_check_gd();
#endif
#ifdef PLC_X4_FUNC
u8 PLC_X4GetReadTaskInfoLen(P_X4_READ_TASK_INFO p_read_task);
P_X4_READ_TASK_INFO PLC_X4FindReadTaskInfo(u8 task_id);
P_X4_READ_TASK_INFO PLC_X4FindNodeReadTaskInfo(u8 node_addr[MAC_ADDR_LEN], u8 task_id);
u8 PLC_X4ReadTaskIsEqual(P_X4_READ_TASK_INFO p_read_task1, P_X4_READ_TASK_INFO p_read_task2);
u8 PLC_X4IsReadTaskCfg(u8 node_addr[MAC_ADDR_LEN], P_X4_READ_TASK_INFO p_read_task);
#endif

static u8 currentsecuremode = 0;

int insertAreaCCO(u8 mac[6])
{
	int i = 0;
	if (macCmp(g_PlmConfigPara.main_node_addr, mac) == OK)
	{
		return MAX_CCO_NUM;
	}
	for (; i < OtherCcoAddr.idx; i++)
	{
		if (macCmp(mac, &OtherCcoAddr.mac[i][0]) == OK)
		{
			return i;
		}
	}
	if (OtherCcoAddr.idx < MAX_CCO_NUM)
	{
		memcpy(&OtherCcoAddr.mac[OtherCcoAddr.idx][0], mac, 6);
		return OtherCcoAddr.idx++;
	}
	return MAX_CCO_NUM + 1;
}
void getAreaCCO(u8 *mac, u8 sn)
{
	if (sn == MAX_CCO_NUM)
	{
		memcpy(mac, g_PlmConfigPara.main_node_addr, 6);
	}
	else if (sn<MAX_CCO_NUM/*&&sn>0*/)
	{
		memcpy(mac, &OtherCcoAddr.mac[sn][0], 6);
	}
	else
	{
		memset(mac, 0xff, 6);
	}
}



void lockStaInfo(void)
{
	OS_ERR err;
	OSMutexPend(&mutex_sta_info, 0, OS_OPT_PEND_BLOCKING, 0, &err);
	if (err != OS_ERR_NONE)
	{
		__BKPT(1);
	}
}
void unLockStaInfo(void)
{
	OS_ERR err;
	OSMutexPost(&mutex_sta_info, OS_OPT_POST_1, &err);
	if (err != OS_ERR_NONE)
	{
		__BKPT(1);
	}
}


u32 PLC_GetReadMeterTimeoutMS()
{
	return 10000;
}

OS_TICK_64 PLC_GetReadMeterTimeoutTick(u16 datalen)
{
#ifdef GDW_JIANGSU  //并发超时时间20S  9*2
  	return (OSCfg_TickRate_Hz * 2);
#else
	u16 sec=datalen*2/200+1;//以2400进行舍入计算
	if (sec<3)
	{
		sec=3;
	}
	return (OSCfg_TickRate_Hz * sec);
#endif
}

P_RELAY_INFO PLC_GetValidNodeInfoBySn(USHORT node_sn)
{
	if (node_sn>=CCTT_METER_NUMBER)
	{
		return NULL;
	}
	P_RELAY_INFO pNode = NODE_INFO_PTR_BY_IDX(node_sn);

	if (CCO_USERD == pNode->status.used)
		return pNode;

	return NULL;
}


/*
void PLC_FreeSubstaInfo(u16 tei_parent)
{
	P_STA_BITMAP pBitmap = PLC_GetAllSubStas(tei_parent);

	if (NULL == pBitmap)
		return;

	if (0 == pBitmap->count)
	{
		free_buffer(__func__,pBitmap);
		return;
	}

	for (u16 tei = TEI_STA_MIN; tei <= TEI_STA_MAX; tei++)
	{
		if (IS_STA_BITMAP_SET(pBitmap->bitmap, tei))
			PLC_FreeStaInfo(tei);
	}

	free_buffer(__func__,pBitmap);
}
*/



u8 PLC_IsTeiUsed(USHORT tei)
{
	OS_ERR err;

	OSSchedLock(&err);

	if (CCO_USERD == g_pStaInfo[tei].used)
	{
		OSSchedUnlock(&err);
		return OK;
	}

	OSSchedUnlock(&err);
	return ERROR;
}

P_STA_INFO PLC_GetStaInfoByTei(USHORT tei)
{
//    OS_ERR err;
//    OSSchedLock(&err);
	return &g_pStaInfo[tei];
}

P_STA_INFO PLC_GetValidStaInfoByTei(USHORT tei)
{
	if (tei < TEI_MIN || tei > TEI_MAX)
		return NULL;

	P_STA_INFO pSta = &g_pStaInfo[tei];

	if (CCO_USERD == pSta->used)
		return pSta;

	return NULL;
}

P_STA_INFO PLC_GetStaInfoByMac(PBYTE pMac)
{
	USHORT sta_sn = PLC_GetStaSnByMac(pMac);

	if (sta_sn >= TEI_CNT)
		return NULL;

	return &g_pStaInfo[sta_sn];
}

USHORT PLC_GetStaSnByMac(PBYTE pMac)
{
	for (USHORT i = TEI_STA_MIN; i < TEI_CNT; i++)
	{
		P_STA_INFO pSta = &g_pStaInfo[i];

		if (CCO_USERD != pSta->used)
			continue;

		if (OK == macCmp(pSta->mac, pMac))
			return i;
	}

	return TEI_CNT;
}

P_STA_STATUS_INFO PLC_GetStaStatusInfoByTei(USHORT tei)
{
	if (tei < TEI_CNT)
		return &g_pStaStatusInfo[tei];
	else
		return NULL;
}

P_RELAY_INFO PLC_GetWhiteNodeInfoByMac(PBYTE pMac)
{
	USHORT node_sn = PLC_GetWhiteNodeSnByMac(pMac);

	if (node_sn >= CCTT_METER_NUMBER)
		return NULL;

	return NODE_INFO_PTR_BY_IDX(node_sn);
}
void PLC_IdentInNet(u16 tei)
{
	if (tei < TEI_CNT)
	{
		g_pStaStatusInfo[tei].check_in_net = 1;
	}
}
BOOL PLC_IsInNet(u16 tei)
{
	if (tei < TEI_CNT)
	{
		return (BOOL)g_pStaStatusInfo[tei].check_in_net;
	}
	return FALSE;
}

USHORT PLC_GetWhiteNodeSnByMac(PBYTE pMac)
{
	u16 sn = PLC_get_sn_from_addr(pMac);

	if (sn < CCTT_METER_NUMBER)
	{
//		if (g_pMeterRelayInfo[sn].status.cfged)
			return sn;
	}

	return CCTT_METER_NUMBER;
}

USHORT PLC_GetWhiteNodeSnByStaInfo(P_STA_INFO pSta)
{
	if (NULL == pSta)
		return CCTT_METER_NUMBER;

	u16 sn = PLC_get_sn_from_addr(pSta->mac);

	if (sn < CCTT_METER_NUMBER)
	{
//		if (g_pMeterRelayInfo[sn].status.cfged)
			return sn;
	}

	return CCTT_METER_NUMBER;
}

unsigned char PLC_Get_Meter_Phase(unsigned short metersn)
{
	if (metersn < CCTT_METER_NUMBER)
	{
		return g_MeterRelayInfo[metersn].status.phase;
	}
	return AC_PHASE_U;
}

void PLC_CopyCCOMacAddr(u8 mac_addr[MAC_ADDR_LEN], BOOL swap)
{
	if (swap)
	{
		memcpy_swap(mac_addr, g_PlmConfigPara.main_node_addr, MAC_ADDR_LEN);
	}
	else
	{
		memcpy_special(mac_addr, g_PlmConfigPara.main_node_addr, MAC_ADDR_LEN);
	}
}



void send_debug_ifo(char *msg)
{
}

void send_debug_frame(PBYTE pFrame, int len)
{
}

USHORT PLC_get_sn_from_addr(UCHAR *addr)
{
	USHORT ii;
	RELAY_INFO *p_relay_info = g_MeterRelayInfo;

	for (ii = 0; ii <  CCTT_METER_NUMBER; ii++, p_relay_info++)
	{
		if (p_relay_info->status.used != CCO_USERD)
			continue;

		if (OK == macCmp(p_relay_info->meter_addr, addr))
			break;
	}
	return ii;
}

#ifdef GDW_ZHEJIANG
bool PLC_Is_Black_Meter(UCHAR *addr)
{
	USHORT ii;
	BLACK_RELAY_INFO *p_relay_info = g_BlackMeterInfo;

	for (ii = 0; ii <  CCTT_BLACK_METER_NUMBER; ii++, p_relay_info++)
	{
		if (p_relay_info->used != CCO_USERD)
			continue;

		if (OK == macCmp(p_relay_info->meter_addr, addr))
			return true;
	}
	return false;
}
#endif //GDW_ZHEJIANG

unsigned short PLC_Trans_MeterAddr2Sn(unsigned char *pAddr6, unsigned short *pEmptySn)
{
	unsigned short metersn;

	*pEmptySn = CCTT_METER_NUMBER;

	for (metersn = 0; metersn < (TEI_STA_MAX-TEI_STA_MIN+1); metersn++)
	{
		if (CCO_USERD != g_MeterRelayInfo[metersn].status.used)
		{
			if (*pEmptySn == CCTT_METER_NUMBER)
				*pEmptySn = metersn;
			continue;
		}

		if (macCmp(g_MeterRelayInfo[metersn].meter_addr, pAddr6))
			return metersn;
	}

	return CCTT_METER_NUMBER;
}

#ifdef PROTOCOL_GD_2016
unsigned short Relay_Trans_MeterColl2Sn(const P_METER_COLL_MAP pMap, unsigned short *pEmptySn)
{
	unsigned short metersn;

	*pEmptySn = CCTT_METER_COLL_NUMBER;
	for (metersn = 0; metersn < CCTT_METER_COLL_NUMBER; metersn++)
	{
		if (!METER_COLL_IS_SET(metersn))
		{
			if (*pEmptySn == CCTT_METER_COLL_NUMBER)
			{
				*pEmptySn = metersn;
			}
			continue;
		}

		if (memcmp(&g_pMeterCollMap[metersn], pMap, sizeof(METER_COLL_MAP)) == 0)
		{
			return metersn;
		}
	}

	return CCTT_METER_NUMBER;
}
#endif


void PLC_NewJoinGetColl(u16 tei)
{
	P_STA_INFO p_Sta=PLC_GetValidStaInfoByTei(tei);
	if (p_Sta
		&& p_Sta->dev_type == II_COLLECTOR || p_Sta->dev_type == I_COLLECTOR) 
	{
		debug_str(DEBUG_LOG_NET, "add collect, %02x%02x%02x%02x%02x%02x, tei=%d, devtype=%d\r\n", p_Sta->mac[5], p_Sta->mac[4], p_Sta->mac[3], p_Sta->mac[2], p_Sta->mac[1], p_Sta->mac[0], tei, p_Sta->dev_type);
		OS_TICK_64 tick_now = GetTickCount64();
		
		P_PLC_GET_SN_CB pcb = &g_pRelayStatus->get_sn_cb;
		pcb->new_tei_tick = tick_now + collect_search_interval;
		pcb->bitmap[tei>>5] |= 1<<(tei&0x1f);
	}
}


BOOL PLC_GetRealNodeSnCfg(BOOL usersn_from_zero, u16 usersn, u16 *p_real_sn)
{
	u16 node_counter = 0;
	P_RELAY_INFO pNode = NULL;

	for (u16 ii = 0; ii <  CCTT_METER_NUMBER; ii++)
	{
		pNode = &g_MeterRelayInfo[ii];
		if (pNode->status.used != CCO_USERD)
			continue;

		if (pNode->status.cfged == 0)
			continue;

		if (FALSE == usersn_from_zero)
			node_counter++;

		if (node_counter == usersn)
		{
			*p_real_sn = ii;
			return TRUE;
		}

		if (TRUE == usersn_from_zero)
			node_counter++;
	}

	return FALSE;
}

BOOL PLC_GetRealNodeSnSelfreg(BOOL usersn_from_zero, u16 usersn, u16 *p_real_sn)
{
	u16 node_counter = 0;
	P_RELAY_INFO pNode = NULL;

	for (u16 ii = 0; ii < CCTT_METER_NUMBER; ii++)
	{
		pNode = &g_MeterRelayInfo[ii];
		if (pNode->status.used != CCO_USERD)
			continue;

		if (pNode->status.selfreg == 0)
			continue;

		if (FALSE == usersn_from_zero)
			node_counter++;

		if (node_counter == usersn)
		{
			*p_real_sn = ii;
			return TRUE;
		}

		if (TRUE == usersn_from_zero)
			node_counter++;
	}

	return FALSE;
}

BOOL PLC_GetRealStaSn(BOOL usersn_from_zero, u16 usersn, u16 *p_real_sn)
{
	u16 node_counter = 0;

	for (u16 tei = TEI_CCO; tei <= TEI_STA_MAX; tei++)
	{
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
		if (NULL == pSta)
			continue;

		if (NET_OUT_NET == pSta->net_state)
			continue;

	#if 0
		if(NET_IN_NET != pSta->net_state)
		continue;
	#endif

		if (FALSE == usersn_from_zero)
			node_counter++;

		if (node_counter == usersn)
		{
			*p_real_sn = tei;
			return TRUE;
		}

		if (TRUE == usersn_from_zero)
			node_counter++;
	}

	return FALSE;
}

//清除主动注册的表计信息
void PLC_ClearSelfRegList()
{
	for (u16 i = 0; i < CCTT_METER_NUMBER; i++)
	{
		P_RELAY_INFO pNode = &g_pMeterRelayInfo[i];

		if (CCO_USERD != pNode->status.used)
			continue;

		if (!pNode->status.cfged)
		{
			memset_special(pNode, 0xff, sizeof(RELAY_INFO));
			PLC_TriggerSaveNodeInfo();
		}
		else
		{
			pNode->status.selfreg = 0;
		}
	}
}

void PLC_AddToSelfRegList(u8 pNodeAddr[CCTT_METER_ADDR_LEN])
{
	u16 empty_sn;
	u16 node_sn = PLC_Trans_MeterAddr2Sn(pNodeAddr, &empty_sn);
	P_RELAY_INFO pNode = NULL;

	if (node_sn >= CCTT_METER_NUMBER)
	{
		if (empty_sn >= CCTT_METER_NUMBER)
			return;
		pNode = &g_pMeterRelayInfo[empty_sn];
		pNode->status.used = CCO_USERD;
#ifdef GDW_ZHEJIANG
		pNode->status.cfged = 1;
#else
		pNode->status.cfged = 0;
#endif
		pNode->user_sn = 0;
		memcpy_special(pNode->meter_addr, pNodeAddr, CCTT_METER_ADDR_LEN);
	}
	else
	{
		pNode = &g_pMeterRelayInfo[node_sn];
	}

	pNode->status.selfreg = 1;
	#ifdef GDW_ZHEJIANG
	pNode->status.IsMeter = 0;//该站点为电能表
	#endif
}

AC_PHASE PLC_PlmPhase_to_AcPhase(PLM_PHASE phase)
{
	if (PLM_PHASE_A == phase)
		return AC_PHASE_A;
	else if (PLM_PHASE_B == phase)
		return AC_PHASE_B;
	else if (PLM_PHASE_C == phase)
		return AC_PHASE_C;
	else
		return AC_PHASE_U;
}

PLM_PHASE PLC_AcPhase_to_PlmPhase(AC_PHASE phase)
{
	if (AC_PHASE_A == phase)
		return PLM_PHASE_A;
	else if (AC_PHASE_B == phase)
		return PLM_PHASE_B;
	else if (AC_PHASE_C == phase)
		return PLM_PHASE_C;
	else
		return PLM_PHASE_U;
}

PLM_PHASE PLC_HplcPhase_to_PlmPhase(HPLC_PHASE phase)
{
	if (PHASE_A == phase)
		return PLM_PHASE_A;
	else if (PHASE_B == phase)
		return PLM_PHASE_B;
	else if (PHASE_C == phase)
		return PLM_PHASE_C;
	else
		return PLM_PHASE_U;
}

PLM_PHASE PLC_PhyPhase_to_PlmPhase(u8 PhyPhase)
{
	PLM_PHASE phase = PLM_PHASE_U;
	if ((PhyPhase & 0x03) == PHASE_A)
		phase = PLM_PHASE_A;
	else if ((PhyPhase & 0x03) == PHASE_B)
		phase = PLM_PHASE_B;
	else if ((PhyPhase & 0x03) == PHASE_C)
		phase = PLM_PHASE_C;
	if (PhyPhase & (1 << 6))
	{
		phase |= PLM_PHASE_ERR;
	}
	if (PhyPhase & (1 << 7))
	{
		if ((PhyPhase & 0x3f) != 0x57 && (PhyPhase & 0x3f) != 0)
		{
			phase |= PLM_PHASE_ERR;
		}
	}
	return phase;
}

HPLC_PHASE PLC_PhyPhase_to_HplcPhase(u8 PhyPhase)
{
#if 0
	if(PhyPhase&0x01)
	return PHASE_A;
	else if(PhyPhase&0x02)
	return PHASE_B;
	else if(PhyPhase&0x04)
	return PHASE_C;
	else
	return PHASE_DEFAULT;
#else
	for (int i=0;i<3;i++)
	{
		if (PhyPhase&0x3)
		{
			return (HPLC_PHASE)(PhyPhase & 0x03);
		}
		PhyPhase>>=2;
	}
	return PHASE_UNKNOW;
#endif
}


//3762协议类型转换为表库协议类型

unsigned char PLC_3762protocol_to_meterprotocol(unsigned char protocol_3762)
{
	if (METER_PROTOCOL_3762_97 == protocol_3762)
		return METER_TYPE_PROTOCOL_97;
	else if (METER_PROTOCOL_3762_07 == protocol_3762)
		return METER_TYPE_PROTOCOL_07;
	else if (METER_PROTOCOL_3762_69845 == protocol_3762)
		return METER_TYPE_PROTOCOL_69845;

	return METER_TYPE_PROTOCOL_TRANSPARENT;
}

//3762协议类型转换为发给通道板的协议类型

unsigned char PLC_3762protocol_to_plcprotocol(unsigned char protocol_3762)
{
	switch (protocol_3762)
	{
	case METER_PROTOCOL_3762_97:
		return APP_PRO_DLT645_1997;
	case METER_PROTOCOL_3762_07:
		return APP_PRO_DLT645_2007;
	case METER_PROTOCOL_3762_69845:
		return APP_PRO_698_45;
	}

	return APP_PRO_TRANSPARENT;
}

//通道板表协议类型转换为发给集中器的协议类型

unsigned char PLC_plcprotocol_to_3762protocol(unsigned char protocol_plc)
{
	switch (protocol_plc)
	{
	case APP_PRO_DLT645_1997:
		return METER_PROTOCOL_3762_97;
	case APP_PRO_DLT645_2007:
		return METER_PROTOCOL_3762_07;
	case APP_PRO_698_45:
		return METER_PROTOCOL_3762_69845;
	case APP_PRO_TRANSPARENT:
		return METER_PROTOCOL_3762_TRANSPARENT;
	}

	return METER_PROTOCOL_3762_TRANSPARENT;
}

//表库协议类型转换为发给通道板的协议类型

unsigned char PLC_meterprotocol_to_plcprotocol(unsigned char protocol_meter)
{
	switch (protocol_meter)
	{
	case METER_TYPE_PROTOCOL_97:
		return APP_PRO_DLT645_1997;
	case METER_TYPE_PROTOCOL_07:
		return APP_PRO_DLT645_2007;
	case METER_TYPE_PROTOCOL_69845:
		return APP_PRO_698_45;
	case METER_TYPE_PROTOCOL_TRANSPARENT:
		return APP_PRO_TRANSPARENT;
	}

	return APP_PRO_TRANSPARENT;
}

//表库协议类型转换为集中器表协议类型

unsigned char PLC_meterprotocol_to_3762protocol(unsigned char protocol_meter)
{
	switch (protocol_meter)
	{
	case METER_TYPE_PROTOCOL_97:
		return METER_PROTOCOL_3762_97;
	case METER_TYPE_PROTOCOL_07:
		return METER_PROTOCOL_3762_07;
	case METER_TYPE_PROTOCOL_69845:
		return METER_PROTOCOL_3762_69845;
	case METER_TYPE_PROTOCOL_TRANSPARENT:
		return METER_PROTOCOL_3762_TRANSPARENT;
	}

	return METER_PROTOCOL_3762_07;
}

PLM_PHASE PLC_GetPlmPhaseByMacAddr(u8 mac[MAC_ADDR_LEN])
{
	PLM_PHASE plm_phase = PLM_PHASE_U;
	P_STA_INFO pSta = PLC_GetStaInfoByMac(mac);
	if (NULL != pSta)
	{
#if defined(GET_PHY_PHASE)
		plm_phase = PLC_PhyPhase_to_PlmPhase(pSta->phy_phase);
#else
		plm_phase = PLC_HplcPhase_to_PlmPhase(pSta->receive_phase);
#endif
	}

	return plm_phase;
}

//返回值:  1正在工作，0停止工作
UCHAR PLC_get_run_state()
{
	UCHAR state = 0;
#if 0
	if (g_pRelayStatus->all_meter_num == g_pRelayStatus->check_online)
	{
		state = 1;
	}
#else
	if (g_pRelayStatus->all_meter_num!=0)
	{
		
		if (g_pRelayStatus->all_meter_num==g_pRelayStatus->check_online)
		{
			state = 1;
		}
		else if (g_pRelayStatus->check_online
				 && GetTickCount64()>30*60*1000)//启动超过半个小时时间
		{
			state = 1;
		}

	}
#endif
	#ifdef GDW_2019_PERCEPTION
	P_PLC_SELFREG_CB pSelfRegCB = &g_pRelayStatus->selfreg_cb;
	P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;
	if (pSelfRegCB->state >= EM_SELFREG_START && pSelfRegCB->state <= EM_SELFREG_REPORT_STOP)
	{
		state |= 1 << 1;
	}

	if (g_pRelayStatus->pause_sec > 0 &&g_pRelayStatus->pause_sec!=0xff)
	{
		state = 0;
	}
	if (pcb->update_flag == 1
		|| g_pRelayStatus->route_read_cb.state != EM_ROUTE_READ_INIT
		|| g_pRelayStatus->dir_read_cb.state != EM_DIR_READ_INIT
		|| 0 != get_queue_cnt(g_PlcWaitRxQueue))
	{
		state |= 1 << 1;
	}
	//
	#endif
	return state;
}

void PLC_save_relay_info(void *ptmr, void *parg)
{
#if 0
	unsigned long len = CCTT_METER_NUMBER*sizeof(RELAY_INFO);

	data_flash_write_straight(METER_RELAY_INFO_ADDR, (ULONG)g_MeterRelayInfo, len);
	//clr_wdt();
#ifdef APP_CUS_YILANG
	//clr_wdt();
	data_flash_write_straight(DEVICE_DB_ADDR, (ULONG)(&g_device_addr_yl), DEVICE_DB_SIZE);
#endif
#ifdef PROTOCOL_GD_2016
	//clr_wdt();
	data_flash_write_straight(METER_COLL_DB_ADDR, (ULONG)(&g_MeterCollMap), METER_COLL_DB_SIZE);
#endif

#endif
}

void PLC_relay_sorage_init()
{
#if 0
	unsigned long len = CCTT_METER_NUMBER*sizeof(RELAY_INFO);

	data_flash_read_straight( METER_RELAY_INFO_ADDR, (ULONG)g_MeterRelayInfo, len );

#ifdef APP_CUS_YILANG
	data_flash_read_straight( DEVICE_DB_ADDR, (ULONG)&g_device_addr_yl, DEVICE_DB_SIZE);
#endif

#ifdef PROTOCOL_GD_2016
	data_flash_read_straight( METER_COLL_DB_ADDR, (ULONG)&g_MeterCollMap, METER_COLL_DB_SIZE);
#endif
#endif
}

BYTE PLC_Is_Meter_270(USHORT metersn)
{
	switch (PLC_Get_Meter_Baud(metersn))
	{
	case PLC_BAUDRATE_329BPS:
	case PLC_BAUDRATE_330BPS:
	case PLC_BAUDRATE_331BPS:
	case PLC_BAUDRATE_1BPS:
	case PLC_BAUDRATE_5BPS:
	case PLC_BAUDRATE_270_330BPS_SL:
	case PLC_BAUDRATE_270_330BPS_3D5_SL:
	case PLC_BAUDRATE_270_330BPS_RF_SL:
	case PLC_BAUDRATE_270_1200BPS_SL:
	case PLC_BAUDRATE_270_1200BPS_3D5_SL:
		return TRUE;
	default:
		return FALSE;
	}
}

//是否检测版本
BYTE PLC_IsTestMode()
{
	return FALSE;
}

BYTE PLC_Get_Meter_Baud(USHORT metersn)
{
	return  0;
}

USHORT PLC_Get_Baud_Number(BYTE plc_baudrate_storage)
{
#if defined(PLC_BAUD_99BPS_ALWAYS)
	return 99;
#elif defined(PLC_BAUD_100BPS_ALWAYS)
	return 100;
#elif defined(PLC_BAUD_330BPS_ALWAYS)
	return 330;
#elif defined(PLC_BAUD_331BPS_ALWAYS)
	return 331;
#elif defined(PLC_BAUD_416_1000BPS_ALWAYS)
	return (0x4005);
#elif defined(PLC_BAUD_416_1000BPS_RF_ALWAYS)
	return (0x4005 | (1 << 6));
#else
	switch (plc_baudrate_storage)
	{
	case PLC_BAUDRATE_0BPS:
		return 0;
	case PLC_BAUDRATE_1BPS:
		return 1;
	case PLC_BAUDRATE_2BPS:
		return 2;
	case PLC_BAUDRATE_4BPS:
		return 4;
	case PLC_BAUDRATE_5BPS:
		return 5;
	case PLC_BAUDRATE_329BPS:
		return 329;
	case PLC_BAUDRATE_330BPS:
		return 330;
	case PLC_BAUDRATE_331BPS:
		return 331;
	case PLC_BAUDRATE_50BPS:
		return 50;
	case PLC_BAUDRATE_99BPS:
		return 99;
	case PLC_BAUDRATE_100BPS:
		return 100;
	case PLC_BAUDRATE_101BPS:
		return 101;
	case PLC_BAUDRATE_500BPS:
		return 500;
	case PLC_BAUDRATE_600BPS:
		return 600;
	case PLC_BAUDRATE_1200BPS:
		return 1200;
	case PLC_BAUDRATE_1201BPS:
		return 1201;
	case PLC_BAUDRATE_2400BPS:
		return 2400;
	case PLC_BAUDRATE_2500BPS:
		return 2500;
		/*
			case PLC_BAUDRATE_421_100BPS_SL:
				return 33;
			case PLC_BAUDRATE_421_1200BPS_SL:
				return 34;
			case PLC_BAUDRATE_270_330BPS_SL:
				return 35;
			case PLC_BAUDRATE_RF_SL:
				return 36;
			case PLC_BAUDRATE_421_100BPS_RF_SL:
				return 37;
			case PLC_BAUDRATE_421_1200BPS_RF_SL:
				return 38;
			case PLC_BAUDRATE_270_330BPS_RF_SL:
				return 39;
		*/
	case PLC_BAUDRATE_RF_SL:
		return 36;
	case PLC_BAUDRATE_421_100BPS_SL:
		return ((1 << 12) | 1);
	case PLC_BAUDRATE_421_100BPS_3D5_SL:
		return ((1 << 12) | (1 << 7) | 1);
	case PLC_BAUDRATE_421_600BPS_SL:
		return ((1 << 12) | 4);
	case PLC_BAUDRATE_421_1200BPS_SL:
		return ((1 << 12) | 2);
	case PLC_BAUDRATE_421_1200BPS_3D5_SL:
		return ((1 << 12) | (1 << 7) | 2);
	case PLC_BAUDRATE_421_100BPS_RF_SL:
		return ((1 << 12) | (1 << 6) | 1);
	case PLC_BAUDRATE_421_1200BPS_RF_SL:
		return ((1 << 12) | (1 << 6) | 2);

	case PLC_BAUDRATE_270_330BPS_SL:
		return ((2 << 12) | 3);
	case PLC_BAUDRATE_270_330BPS_3D5_SL:
		return ((2 << 12) | (1 << 7) | 3);
	case PLC_BAUDRATE_270_330BPS_RF_SL:
		return ((2 << 12) | (1 << 6) | 3);
	case PLC_BAUDRATE_270_1200BPS_SL:
		return ((2 << 12) | 2);
	case PLC_BAUDRATE_270_1200BPS_3D5_SL:
		return ((2 << 12) | (1 << 7) | 2);
	case PLC_BAUDRATE_178_1000BPS_SL:
		return 0x3005;
	case PLC_BAUDRATE_178_5000BPS_SL:
		return 0x3003;
		//0x3004 178 3000
	case PLC_BAUDRATE_416_599BPS_SL:
		return 0xffff;      //没定
	case PLC_BAUDRATE_416_600BPS_SL:
		return 0x5005;
	case PLC_BAUDRATE_416_600BPS_RF_SL:
		return (0x5005 | (1 << 6));
	case PLC_BAUDRATE_416_1000BPS_SL:
		return 0x4005;
	case PLC_BAUDRATE_416_1000BPS_RF_SL:
		return (0x4005 | (1 << 6));
	case PLC_BAUDRATE_416_3000BPS_SL:
		return 0x4004;
	case PLC_BAUDRATE_416_5000BPS_SL:
		return 0x4003;
	case PLC_BAUDRATE_416_7500BPS_SL:
		return 0x4002;
	case PLC_BAUDRATE_416_15000BPS_SL:
		return 0x4001;
	default:
#if defined (PLC_BAUD_DR3_DEFAULT)
		return 330;
#elif defined (PLC_BAUD_DX3_DEFAULT)
		return 99;
#else
		return 100;
#endif
	}
#endif
}

USHORT PLC_Is_Baud_SL()
{
	USHORT baud;
	baud = PLC_Get_Baud_Default();

	return (baud >= PLC_BAUDRATE_RF_SL);
}

USHORT PLC_Get_Baud_Default()
{
	return PLC_BAUDRATE_RF_SL;
}

void PLC_set_fix_relay(P_RT_RELAY relay_info)
{
}
#ifdef PROTOCOL_GD_2016
u8 X4_Task_AddtoSTA(u8 *addr, u8 Task_ID)
{
	u16 meter_sn, emptysn;

	if (OK == IsEqualSpecialData(addr, 0x99, 6))
	{
		for (emptysn = 0; emptysn < CCTT_METER_NUMBER; emptysn++)
		{
			if (CCO_USERD == g_MeterRelayInfo[emptysn].status.used)
			{
				if (g_MeterRelayInfo[emptysn].task_id_num < CCTT_X4_READ_TASK_NUM_PER_METER)
					g_MeterRelayInfo[emptysn].task_id_array[g_MeterRelayInfo[emptysn].task_id_num++] = Task_ID;

			}
		}
	}
	else
	{
		meter_sn = PLC_Trans_MeterAddr2Sn(addr, &emptysn);
		if (meter_sn < CCTT_METER_NUMBER)
		{
			if (g_MeterRelayInfo[meter_sn].task_id_num < CCTT_X4_READ_TASK_NUM_PER_METER)
			{
				g_MeterRelayInfo[meter_sn].task_id_array[g_MeterRelayInfo[meter_sn].task_id_num++] = Task_ID;
			}

		}
		else
			return (0); //节点不存在配置失败

	}
	return (1);
}
#endif
u8 Arry_Rank(u8 n, u8 ID, u8 *buf)
{
	u8 i, sum;
	sum = n;
	if (n)
	{
		for (i = 0; i < n; i++)
		{
			if (ID == buf[i])
			{
				memmove(buf + i, buf + i + 1, n - i - 1);
			}
		}
		sum--;
	}
	return (sum);
}
#ifdef PROTOCOL_GD_2016
u8 X4_Task_DeltoSTA(u8 *addr, u8 Task_ID)
{
	u16 meter_sn, emptysn;

	if (OK == IsEqualSpecialData(addr, 0x99, 6))
	{
		for (emptysn = 0; emptysn < CCTT_METER_NUMBER; emptysn++)
		{
			if (CCO_USERD == g_MeterRelayInfo[emptysn].status.used)
			{
				if (g_MeterRelayInfo[emptysn].task_id_num)
				{
					g_MeterRelayInfo[emptysn].task_id_num = Arry_Rank(g_MeterRelayInfo[emptysn].task_id_num, Task_ID, g_MeterRelayInfo[emptysn].task_id_array);
				}

			}
		}
	}
	else
	{
		meter_sn = PLC_Trans_MeterAddr2Sn(addr, &emptysn);
		if (meter_sn < CCTT_METER_NUMBER)
		{
			g_MeterRelayInfo[meter_sn].task_id_num = Arry_Rank(g_MeterRelayInfo[meter_sn].task_id_num, Task_ID, g_MeterRelayInfo[meter_sn].task_id_array);
		}
		else
			return (0); //节点不存在配置失败

	}
	return (1);
}
#endif
u8 allow_save=1;
void PLC_SaveNodeOnce(void)
{
	if (allow_save)
	{
		static u16 sec=0;
		if (++sec<(60*60))//1小时一次统计
		{
			return;
		}
		sec=0;

		int i;
		for (i=TEI_STA_MIN;i<TEI_CNT;i++)
		{
			if (g_StaInfo[i].used!=CCO_USERD)
			{
				continue ;
			}
			if (!g_StaInfo[i].geted_id)
				return;
		}
		PLC_TriggerSaveNodeInfo();
	}
}

bool PLC_find_new_node(UCHAR *addr)
{
	USHORT empty_sn, meter_sn;

	meter_sn = PLC_Trans_MeterAddr2Sn(addr, &empty_sn);

	if (meter_sn >= CCTT_METER_NUMBER)
	{
		if ((empty_sn < CCTT_METER_NUMBER) && (CCO_USERD != g_MeterRelayInfo[empty_sn].status.used))
			return TRUE;
	}

	return FALSE;
}

#ifdef GDW_ZHEJIANG
void PLC_add_black_node(UCHAR *addr ) //添加黑名单
{
	unsigned short metersn,emptysn = CCTT_BLACK_METER_NUMBER;

	for (metersn = 0; metersn < CCTT_BLACK_METER_NUMBER; metersn++)
	{
		if (macCmp(g_BlackMeterInfo[metersn].meter_addr, addr))
			return;
		if (CCO_USERD != g_BlackMeterInfo[metersn].used)
		{
			if(CCTT_BLACK_METER_NUMBER == emptysn)
				emptysn = metersn;
			continue;
		}
	}
	
	if(metersn >= CCTT_BLACK_METER_NUMBER)
	{
		memcpy_special(g_BlackMeterInfo[emptysn].meter_addr, addr, CCTT_METER_ADDR_LEN);
		g_BlackMeterInfo[emptysn].used = CCO_USERD;
		PLC_TriggerSaveBlackNodeInfo();
	}
}

#endif //GDW_ZHEJIANG

void PLC_add_new_node(USHORT user_sn, UCHAR meter_type, UCHAR *addr ) //meter_type 高位用来判断是否黑名单
{
#ifdef GDW_ZHEJIANG
	BYTE protocol_meter = meter_type & 0x0F;
#else
	BYTE protocol_meter = meter_type ;        
#endif
	RELAY_INFO *p_RelayInfo = NULL;

	USHORT emptysn, meter_sn;
	BOOL bNew = FALSE;

	meter_sn = PLC_Trans_MeterAddr2Sn(addr, &emptysn);
	allow_save=1;
	g_RelayStatus.area_dis_cb.getedZoneStatusBitMap[meter_sn>>5] &= ~(1<<(meter_sn&0x1f));
	g_RelayStatus.area_dis_cb.zoneErrorMap[meter_sn>>5] &= ~(1<<(meter_sn&0x1f));
	if (meter_sn >= CCTT_METER_NUMBER)
	{
		if ((emptysn < CCTT_METER_NUMBER) && (CCO_USERD != g_MeterRelayInfo[emptysn].status.used))
			meter_sn = emptysn;
		else
			return;
		bNew = TRUE;
	}
	else
	{
		if (g_MeterRelayInfo[meter_sn].status.cfged
			&& g_MeterRelayInfo[meter_sn].meter_type.protocol == protocol_meter
			&& g_MeterRelayInfo[meter_sn].user_sn == user_sn)
			return;
	}


	if (meter_sn >= CCTT_METER_NUMBER)
		return;

	p_RelayInfo = &g_MeterRelayInfo[meter_sn];

	if (bNew)
	{
		memset_special((UCHAR *)p_RelayInfo, 0xff, sizeof(RELAY_INFO));
		memset_special((UCHAR *)&p_RelayInfo->meter_type, 0x00, sizeof(p_RelayInfo->meter_type));
	}

	p_RelayInfo->status.used = CCO_USERD;
	if (user_sn != CCTT_METER_NUMBER)
	{
		p_RelayInfo->user_sn = user_sn;
	}
	else
	{
		p_RelayInfo->user_sn = meter_sn;
	}
	memcpy_special(p_RelayInfo->meter_addr, addr, sizeof(p_RelayInfo->meter_addr));
#ifdef GDW_ZHEJIANG        
        if(meter_type > METER_TYPE_PROTOCOL_TRANSPARENT)
        {
          p_RelayInfo->black_list = 0;//0:使能
          PLC_add_black_node(addr);
        }
        else
        {         
          p_RelayInfo->white_list = 0;//0:使能
#endif           
          p_RelayInfo->meter_type.protocol = protocol_meter;
#ifdef GDW_ZHEJIANG            
        }
#endif 
        
	p_RelayInfo->status.cfged = 1;
	p_RelayInfo->meter_type.m0c1 = METER_TYPE_METER;  // default meter node
#ifndef GDW_ZHEJIANG
	p_RelayInfo->status.plc = 1;  // default plc node   
#endif

	PLC_SET_FREEZE_NO(meter_sn);

	//data_flash_write_straight( METER_RELAY_INFO_ADDR +(ULONG)meter_sn * sizeof(RELAY_INFO),
	//                          (ULONG)p_RelayInfo,
	//                          sizeof(RELAY_INFO));
	if (user_sn != CCTT_METER_NUMBER) //不存储入flash
	{
		// start timer for later save to flash
		PLC_TriggerSaveNodeInfo();
		//PLC_TriggerChangeNetid(5);

		// pause
	}
	else
	{
#ifdef GDW_ZHEJIANG
		if (g_PlmConfigPara.close_white_list == 2)
		{
			p_RelayInfo->status.cfged = 1;
		}
		else
		{
		p_RelayInfo->status.cfged = 0;
	}
#else
		p_RelayInfo->status.cfged = 0;
#endif	
	}
	Relay_Task_PauseAWhile();
	PLC_TriggerOfflineList();
}

void PLC_del_new_node(UCHAR *addr,u8 list_type)
{
	USHORT ii;

	RELAY_INFO *p_RelayInfo = g_MeterRelayInfo;

	for (ii = 0; ii <  CCTT_METER_NUMBER; ii++, p_RelayInfo++)
	{
		if (p_RelayInfo->status.used != CCO_USERD)
			continue;

		if (OK != macCmp(addr, p_RelayInfo->meter_addr))
			continue;
                if(list_type == LIST_TYPE_WHITE )
                {       
                    //清除异常表
                    g_RelayStatus.area_dis_cb.getedZoneStatusBitMap[ii>>5] &= ~(1<<(ii&0x1f));
                    g_RelayStatus.area_dis_cb.zoneErrorMap[ii>>5] &= ~(1<<(ii&0x1f));
                    // start timer for later save to flash
                    PLC_TriggerSaveNodeInfo();

            #if 1
                    PLC_TriggerOfflineList();
            #else
                    u16 tei = PLC_GetStaSnByMac(p_RelayInfo->meter_addr);
                    if(tei>=TEI_STA_MIN && tei<=TEI_STA_MAX)
                    {
                            //通知离线
                            PLC_AddToOfflineList(p_RelayInfo->meter_addr, OFFLINE_NOT_IN_WHITE_LIST);
                            //PLC_FreeStaInfoRelationship(tei);     //等待自动离线?????
                    }
            #endif
            #ifdef GDW_ZHEJIANG
                    u8 last_black_flag;
                    last_black_flag = p_RelayInfo->black_list;
            #endif
                    memset_special((UCHAR *)p_RelayInfo, 0xff, sizeof(RELAY_INFO));
            #ifdef GDW_ZHEJIANG
                    p_RelayInfo->black_list =last_black_flag;
            #endif
                    Relay_Task_PauseAWhile();
                }
                else
                {
                  #ifdef GDW_ZHEJIANG
                    PLC_TriggerSaveNodeInfo();
                    p_RelayInfo->black_list = 1;//0:使能
                    Relay_Task_PauseAWhile();
                    #endif
                }
                

		break;
	}


}

USHORT PLC_calccrc(UCHAR crcbuf, USHORT crc)
{
	UCHAR i;
	crc = crc ^ crcbuf;

	for (i = 0; i < 8; i++)
	{
		UCHAR chk;
		chk = (UCHAR)crc & 1;

		crc = crc >> 1;
		crc = crc & 0x7fff;

		if (chk == 1)
			crc = crc ^ 0xa001;
	}

	return crc;
}


unsigned short  PLC_Checksum(UCHAR *PLC_data, USHORT  Len)
{
	unsigned short  crc = 0xffff;

	while (Len--)
	{
		// temp+=*PLC_data ++;
		crc = PLC_calccrc(*PLC_data++, crc);
	}

	return crc;
}

BYTE PLC_MeterFilePhase_To_3762Phase(BYTE filePhase)
{
	if (AC_PHASE_A == filePhase)
		return 1;
	if (AC_PHASE_B == filePhase)
		return 2;
	if (AC_PHASE_C == filePhase)
		return 3;
	return 0;
}

u8 PLC_Get_Phase_By_Relay(USHORT metersn, PBYTE pRelayAddr, u8 level)
{
	int i = 0;
	USHORT nMeterSn = 0;
	USHORT relay[MAX_RELAY_DEEP];

	nMeterSn = metersn;

	if (nMeterSn >= CCTT_METER_NUMBER)
		return PLC_MeterFilePhase_To_3762Phase(AC_PHASE_U);

	if (level > MAX_RELAY_DEEP)
	{
		return PLC_MeterFilePhase_To_3762Phase(AC_PHASE_U);
	}
	//0级
	if (0 == level)
	{
		return PLC_MeterFilePhase_To_3762Phase(g_MeterRelayInfo[nMeterSn].status.phase);
	}

#if  APP_USE_JL421_DIVIDE_ABC
	//本身有相位
	if (AC_PHASE_U != g_MeterRelayInfo[nMeterSn].status.phase)
	{
		return PLC_MeterFilePhase_To_3762Phase(g_MeterRelayInfo[nMeterSn].status.phase);
	}
#else
	if (level == 0)
	{
		return PLC_MeterFilePhase_To_3762Phase(g_MeterRelayInfo[nMeterSn].status.phase);
	}
#endif
	//从中继中找相位
	for (i = 0; i < level; i++)
	{
		relay[i] = PLC_get_sn_from_addr(pRelayAddr + i * 6);
	}

#if  APP_USE_JL421_DIVIDE_ABC
	for (i = level - 1; i >= 0; i--)
	{
		nMeterSn = relay[i];
		if (nMeterSn >= CCTT_METER_NUMBER)
			return PLC_MeterFilePhase_To_3762Phase(AC_PHASE_U);

		if (AC_PHASE_U != g_MeterRelayInfo[nMeterSn].status.phase)
		{
			//2012年12月7日17:21:23，通道板总是应答三相表，屏蔽对三相表的处理
#if 0
			if(METER_TYPE_THREE_PHASE==g_MeterRelayInfo[nMeterSn].meter_type.phaseType)
			{
				if(0==i)
				{
					//0级是三相表，随机一相
					return PLC_MeterFilePhase_To_3762Phase((BYTE)(rand_special()%3));
				}
			}
			else
#endif
			{
				return PLC_MeterFilePhase_To_3762Phase(g_MeterRelayInfo[nMeterSn].status.phase);
			}
		}
		else
		{
			continue;
		}
	}
	return PLC_MeterFilePhase_To_3762Phase(AC_PHASE_U);
#else  // APP_USE_JL421_DIVIDE_ABC
	return PLC_MeterFilePhase_To_3762Phase(g_MeterRelayInfo[relay[0]].status.phase);
#endif //APP_USE_JL421_DIVIDE_ABC
}

unsigned char PLC_get_wait_rx_count()
{
	return get_queue_cnt(g_PlcWaitRxQueue);
}

/*******************************************************

**  函数说明:  模块初始化

*******************************************************/
unsigned short PLC_init(HANDLE h)
{
	return TRUE;
}


/*******************************************************

**函数说明:  载波自动路由消息处理

*******************************************************/
unsigned short PLC_proc(HANDLE h)
{
	return OK;
}
/*
UCHAR PLC_Directread_Time_Related_Request(USHORT metersn, unsigned short relay_meter[MAX_RELAY_DEEP+1])
{
	P_MSG_INFO p_CccttSendMsg, pRelatedMsg;
	P_PLC_READ_ITEM_RSP_GDW2009 pReadIfo, pRelatedReadIfo;
	P_GD_DL645_BODY pDL645, pRelatedDL645;
	USHORT time_related = 0;
	BYTE dataUnit[80];
	USHORT p = 0;
	OS_ERR err_code;
	BYTE i, level;
	unsigned char addr_field[1+MAX_RELAY_DEEP+1][6];
	OS_MSG_SIZE  msg_size;

	if(FALSE == g_mf_DirectReadMsg.msg_header.time_related)
		return READ_ITEM_READABLE;

	pReadIfo = (P_PLC_READ_ITEM_RSP_GDW2009)g_mf_DirectReadMsg.msg_buffer;

	if(pReadIfo->len > sizeof(dataUnit)-9)
		return READ_ITEM_READABLE;

	pDL645 = (P_GD_DL645_BODY)pReadIfo->data;

	memcpy_special(&addr_field[0][0], g_PlmConfigPara.main_node_addr, FRM_PLC_ADDR_LEN_HARBIN);
	if((NULL != relay_meter) && (OK == Relay_Exist(relay_meter)))
	{
		level = Relay_GetLevel(relay_meter);
		for(i=0; i<level; i++)
		{
			memcpy_special(&addr_field[1+i][0], g_MeterRelayInfo[relay_meter[i]].meter_addr, FRM_PLC_ADDR_LEN_HARBIN);
		}
	}
	else
	{
		level = 0;
	}
	memcpy_special(&addr_field[1+level][0], pDL645->meter_number, FRM_PLC_ADDR_LEN_HARBIN);

	time_related = GDW3762_CopyMeter_Time(&addr_field[0][0], level, 0x13, 0x02, pReadIfo->len, (PBYTE)pDL645, PLC_Get_Baud_Number(PLC_Get_Meter_Baud(metersn)));

	memcpy_special(&dataUnit[p], pDL645->meter_number, FRM_PLC_ADDR_LEN_HARBIN);
	p += FRM_PLC_ADDR_LEN_HARBIN;
	memcpy_special(&dataUnit[p], &time_related, 2);
	p += 2;
	dataUnit[p++] = pReadIfo->len;
	memcpy_special(&dataUnit[p], pDL645, pReadIfo->len);
	p += pReadIfo->len;

	if(!(p_CccttSendMsg = LMP_make_frame_3762_cmd(AFN_REAME_READ, F3, PLM_PHASE_A, dataUnit, p, FALSE, NULL)))
		return READ_ITEM_READABLE;

	End_send(p_CccttSendMsg);

	if(!(pRelatedMsg = OSQPend(&g_pOsRes->q_up_rx_plc, OSCfg_TickRate_Hz, OS_OPT_PEND_BLOCKING, &msg_size, NULL, &err_code)))
		return READ_ITEM_READABLE;

	pRelatedReadIfo = (P_PLC_READ_ITEM_RSP_GDW2009)pRelatedMsg->msg_buffer;
	pRelatedDL645 = (P_GD_DL645_BODY)pRelatedReadIfo->data;
	if(pRelatedReadIfo->len > 0)
	{
		pReadIfo->len = pRelatedReadIfo->len;
		memcpy_special(pDL645, pRelatedDL645, pRelatedReadIfo->len);
	}
	free_send_buffer(__func__,pRelatedMsg);

	return READ_ITEM_READABLE;
}
*/
unsigned short PLC_Correct_07_97(unsigned short read_sn, PBYTE pCmd, USHORT cmd_len)
{
#ifdef APP_CORRECT_07_97

	P_GD_DL645_BODY pDL645Cmd = NULL;
	P_DL69845_BODY p69845Cmd = NULL;
	P_RELAY_INFO pMeter = NULL;
	BYTE meter_protocol = METER_TYPE_PROTOCOL_TRANSPARENT;

	if (read_sn >= CCTT_METER_NUMBER)
		return ERROR;

	pMeter = &g_MeterRelayInfo[read_sn];

	if (CCO_USERD != pMeter->status.used)
		return ERROR;

	pDL645Cmd = PLC_check_dl645_frame(pCmd, cmd_len);
	if (NULL == pDL645Cmd)
		p69845Cmd = PLC_check_69845_frame(pCmd, cmd_len);

	meter_protocol = pMeter->meter_type.protocol;

	if (NULL != pDL645Cmd)
	{
		if ((0x01 == pDL645Cmd->controlCode) && (pDL645Cmd->dataLen == 0x02))
		{
			meter_protocol = METER_TYPE_PROTOCOL_97;
		}
		else if ((0x11 == pDL645Cmd->controlCode) && (pDL645Cmd->dataLen == 0x04))
		{
			meter_protocol = METER_TYPE_PROTOCOL_07;
		}
	}
	else if (NULL != p69845Cmd)
	{
		meter_protocol = METER_TYPE_PROTOCOL_69845;
	}

	if (meter_protocol != pMeter->meter_type.protocol)
	{
		pMeter->meter_type.protocol = meter_protocol;
		PLC_save_relay_timer_start();
	}

#endif

	return OK;
}

BOOL PLC_get_system_time(local_tm *p_sys_time)
{

	if (g_PlmStatusPara.t_gettime_sec)
	{
		__time64_t tt=g_PlmStatusPara.t_now_sec;
		*p_sys_time=*gmtime(&tt);
		return TRUE;
	}

	return FALSE;


}

UCHAR PLC_get_meter_item(USHORT sn, unsigned short relay_meter[MAX_RELAY_DEEP + 1], BYTE send_phase, P_MSG_INFO *recv_msg)
{
	return READ_ITEM_READABLE;
}


//返回路由信息的长度
USHORT PLC_Get_Router_Ifo(P_ROUTER_IFO pRouterIfo)
{
	BYTE i;
	//USHORT bps[] = {50, 100, 330, 331, 600, 1200};
	USHORT bps[] = { 0 };

	PLC_get_relay_info();

	//本地通信模式字
	#if defined(COMM_MODE_OLD)
		pRouterIfo->comm_mode.comm_mode = 2; //宽带电力线载波通信
	#elif defined(HRF_SUPPORT)
		#ifdef GDW_SHAN3XI
		pRouterIfo->comm_mode.comm_mode = 14; //宽带电力线载波通信+HRF
		#else
		pRouterIfo->comm_mode.comm_mode = 4; //宽带电力线载波通信+HRF
		#endif
	#else
	pRouterIfo->comm_mode.comm_mode = 4; //宽带电力线载波通信+HRF
	#endif
	pRouterIfo->comm_mode.is_router = 1;
	pRouterIfo->comm_mode.tp_mode = 1;
#if defined(APP_CONCENTRATOR_MODE)
	pRouterIfo->comm_mode.read_period_mode = 0x01;      //集中器主导抄表
#else
	pRouterIfo->comm_mode.read_period_mode = 0x03;    //集中器和路由主导抄表模式都支持
#endif
	pRouterIfo->comm_mode.read_delay_enabled = 0x07;
	pRouterIfo->comm_mode.meter_tab_mode = 0x01;
	pRouterIfo->comm_mode.broadcast_confirm_mode = 0x01;    //广播命令在本地信道执行广播通信之前就返回确认报文
	pRouterIfo->comm_mode.broadcast_exe_mode = 0x00;

	pRouterIfo->comm_mode.chanel_count = 1;
	pRouterIfo->comm_mode.powercut_ifo = g_pRelayStatus->powercut_ifo;

	pRouterIfo->comm_mode.baudrate_count = countof(bps);
#ifdef GDW_JIANGSU	
	pRouterIfo->comm_mode.minute_coll = 1;
#endif
	pRouterIfo->comm_mode.reverse1 = 0;

	pRouterIfo->comm_mode.reverse2 = 0;

	pRouterIfo->comm_mode.reverse3 = 0;

	pRouterIfo->direct_read_timeout = g_PlmConfigPara.plc_max_timeout;
//	if (pRouterIfo->direct_read_timeout <= DIR_READ_MIN_TIMEOUT_SEC)
//		pRouterIfo->direct_read_timeout = DIR_READ_MIN_TIMEOUT_SEC;
	pRouterIfo->broadcast_timeout = 900;
	pRouterIfo->max_frame_len = 1024;
	pRouterIfo->max_file_package_len = 1000;
	pRouterIfo->update_waittime = 0xff;
	memcpy_special(pRouterIfo->main_node_addr, g_PlmConfigPara.main_node_addr, 6);
	pRouterIfo->max_node_count = TEI_STA_MAX-TEI_STA_MIN+1;
	pRouterIfo->now_node_count = g_pRelayStatus->cfged_meter_num;
	pRouterIfo->protocol_date_release.day   = 0x10;
	pRouterIfo->protocol_date_release.month = 0x09;
	pRouterIfo->protocol_date_release.year  = 0x20;

	pRouterIfo->protocol_date_bak.day   = 0x10;
	pRouterIfo->protocol_date_bak.month = 0x09;
	pRouterIfo->protocol_date_bak.year  = 0x20;

	memcpy_special(&pRouterIfo->router_version, &c_Tmn_ver_info, sizeof(c_Tmn_ver_info)-4);
	TransposeByteOrder((UCHAR *)pRouterIfo->router_version.version, sizeof(pRouterIfo->router_version.version));
	pRouterIfo->router_version.version[0] = Hex2BcdChar(pRouterIfo->router_version.version[0]);
	pRouterIfo->router_version.version[1] = Hex2BcdChar(pRouterIfo->router_version.version[1]);
	pRouterIfo->router_version.day = Hex2BcdChar(pRouterIfo->router_version.day);
	pRouterIfo->router_version.month = Hex2BcdChar(pRouterIfo->router_version.month);
	pRouterIfo->router_version.year = Hex2BcdChar(pRouterIfo->router_version.year);

#if 1
	//中惠要求厂商代码和芯片代码写死，不从芯片ID中获取
	TransposeByteOrder((UCHAR *)pRouterIfo->router_version.factoryCode, sizeof(pRouterIfo->router_version.factoryCode));
	TransposeByteOrder((UCHAR *)pRouterIfo->router_version.chipCode, sizeof(pRouterIfo->router_version.chipCode));
#else
	memcpy(pRouterIfo->router_version.factoryCode, ((P_STA_CHIP_ID_INFO)(g_cco_id.chip_id))->factory_code, 2);
	memcpy(pRouterIfo->router_version.chipCode, ((P_STA_CHIP_ID_INFO)(g_cco_id.chip_id))->chip_code, 2);
#endif

	//通信速率
	for (i = 0; i < countof(bps); i++)
	{
		pRouterIfo->baudrate_ifo[i] = bps[i];
	}

	return (sizeof(ROUTER_IFO) - 2 + 2 * countof(bps));
}

#if defined(PROTOCOL_GD_2016)
//返回路由信息的长度
USHORT PLC_Get_Router_Ifo_gd(P_ROUTER_IFO_GD pRouterIfo)
{
	PLC_get_relay_info();

	memset_special(pRouterIfo, 0, sizeof(ROUTER_IFO_GD));

	pRouterIfo->comm_mode.comm_mode = 2; //2-宽带电力线载波通信
	pRouterIfo->max_frame_len = MSA_MESSAGAE_MAX_SIZE;
	pRouterIfo->max_file_package_len = 1024;
	pRouterIfo->update_waittime = 5;
	memcpy_special(&pRouterIfo->main_node_addr[0], g_PlmConfigPara.main_node_addr, 6);
	pRouterIfo->max_node_count = TEI_STA_MAX-TEI_STA_MIN+1;
	pRouterIfo->now_node_count = g_pRelayStatus->cfged_meter_num;
	pRouterIfo->max_node_per_frame = 20;
#if 0
	//南网2016
	pRouterIfo->protocol_date_release.day = 0x06;
	pRouterIfo->protocol_date_release.month = 0x01;
	pRouterIfo->protocol_date_release.year = 0x16;
#else
	//南网2017
	pRouterIfo->protocol_date_release.day = 0x01;
	pRouterIfo->protocol_date_release.month = 0x08;
	pRouterIfo->protocol_date_release.year = 0x17;
#endif
	memcpy_special(&pRouterIfo->router_version.factoryCode[0], &c_Tmn_ver_info.factoryCode[0], 2);
	TransposeByteOrder((UCHAR *)pRouterIfo->router_version.factoryCode, 2);
	memcpy_special(&pRouterIfo->router_version.chipCode[0], c_Tmn_ver_info.chip_code, 2);
	TransposeByteOrder((UCHAR *)pRouterIfo->router_version.chipCode, 2);

    pRouterIfo->router_version.version[0] = Hex2BcdChar(c_Tmn_ver_info.software_v[1]);
    pRouterIfo->router_version.version[1] = Hex2BcdChar(c_Tmn_ver_info.software_v[0]);

	pRouterIfo->router_version.softReleaseDate.day = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.day);
	pRouterIfo->router_version.softReleaseDate.month = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.month);
	pRouterIfo->router_version.softReleaseDate.year = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.year);

	return sizeof(ROUTER_IFO_GD);
}
#endif

void PLC_RequestCentreTime(void *arg)
{
    P_MSG_INFO pMsgIfo;

	if (pMsgIfo = LMP_make_frame_3762_cmd(AFN_REAME_READ, F2, PLM_PHASE_U, NULL, 0, false, NULL))
    {
        End_post(pMsgIfo);
    }
}

USHORT PLC_report_msg_to_main()
{
#if defined(PROTOCOL_GD_2016)

	BYTE dataUnit[sizeof(ROUTER_IFO_GD)];

	PLC_Get_Router_Ifo_gd((P_ROUTER_IFO_GD)dataUnit);

	P_MSG_INFO pMsgIfo = LMP_make_frame_gd_cmd(0x03, 0xE8000302, &dataUnit[0], sizeof(ROUTER_IFO_GD), NULL);
	pMsgIfo->msg_header.send_delay=false;
	if (pMsgIfo)
		End_post(pMsgIfo);
	return OK;

#elif defined(GDW_2012)
	P_MSG_INFO pMsgIfo;
	u8 dataUnit[sizeof(ROUTER_IFO) + 40];
	u8 nDataLen = 0;


#if 0
	for(i=0; i<10; i++)
	{
		BSP_OS_TimeDlyMs(1000);
		if(0xff != g_pRelayStatus->powercut_ifo)
		break;
	}
#endif

	nDataLen = (u8)PLC_Get_Router_Ifo((P_ROUTER_IFO)dataUnit);
	pMsgIfo = LMP_make_frame_3762_cmd(AFN_READ_PARAM, F10, PLM_PHASE_U, dataUnit, nDataLen, FALSE, NULL);
	if (pMsgIfo)
		End_post(pMsgIfo);

	return OK;
#else
	P_MSG_INFO p_CccttSendMsg = NULL;

	P_LMP_LINK_HEAD sendLinkHead;

	//UCHAR err_code;

	if (EM_FACTORY_DR != g_PlmStatusPara.factoryFlag)
	{
		return ERROR;
	}

	if (!(p_CccttSendMsg = alloc_send_buffer(__func__,MSG_SHORT)))
	{
		//Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);
		return ERROR;
	}

	sendLinkHead = (P_LMP_LINK_HEAD)(p_CccttSendMsg->msg_buffer);

	sendLinkHead->firstStartByte = FRAME_PLM2005_START_FLAG;

	sendLinkHead->ctl_field = 0xC7;     //PRM = 0; DIR = 1;

	memset_special((UCHAR *)&sendLinkHead->Inf, 0, sizeof(INFO_FD));

	sendLinkHead->user_data[0] = AFN_EXT_DATA_QUERY;

	sendLinkHead->user_data[1] = (0x01 << (F5 - 1));

	sendLinkHead->user_data[2] = 0x0;

	sendLinkHead->frm_len = OffsetOf(LMP_LINK_HEAD, user_data) + 3 + 1;

	sendLinkHead->user_data[3] = 1;

	p_CccttSendMsg->msg_buffer[sendLinkHead->frm_len] = Get_checksum(&sendLinkHead->ctl_field, sendLinkHead->frm_len - 3);
	sendLinkHead->frm_len++;

	p_CccttSendMsg->msg_buffer[sendLinkHead->frm_len] = 0x16;
	sendLinkHead->frm_len++;

	p_CccttSendMsg->msg_header.msg_len = sendLinkHead->frm_len;

	p_CccttSendMsg->msg_header.need_buffer_free = TRUE; /* 标识end 负责buffer 释放*/
	p_CccttSendMsg->msg_header.send_delay = false;
	p_CccttSendMsg->msg_header.end_id = COM_PORT_MAINTAIN;

#if 0
	{
		BYTE tmp[] =
		{0x68, 0x13, 0x00, 0x81, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xFF, 0xFF, 0x02, 0x00, 0xC2, 0x16};
		memcpy_special(p_CccttSendMsg->msg_buffer, tmp, sizeof(tmp));
		p_CccttSendMsg->msg_header.msg_len = sizeof(tmp);
	}
#endif

	End_post(p_CccttSendMsg);

	return OK;
#endif
}


//点抄返回
USHORT PLC_dirread_response(BYTE meter_protocol, P_GD_DL645_BODY pDL645Resp)
{
	P_MSG_INFO pSndMsg = NULL, pDataUnitMsg = NULL;
	PBYTE pMeterAddr;
	PBYTE p0, p;

	if (!(pDataUnitMsg = alloc_send_buffer(__func__,MSG_SHORT)))
	{
		return ERROR;
	}

	p0 = p = pDataUnitMsg->msg_buffer;

#ifdef GDW_2012
	//通讯时长
	*(USHORT *)p = 20;
	p += 2;
#endif

	*p++ = PLC_meterprotocol_to_3762protocol(meter_protocol);

	pMeterAddr = pDL645Resp->meter_number;

	*p++ = pDL645Resp->dataLen + 12;
	memcpy_special(p, pDL645Resp, pDL645Resp->dataLen + 12);
	p += (pDL645Resp->dataLen + 12);

	pSndMsg = LMP_make_frame_3762_resp(AFN_FRAME_FORWARD, F1, PLM_PHASE_A, p0, p - p0, TRUE, pMeterAddr, g_PlmStatusPara.plm_seq_no_down);

	free_send_buffer(__func__,pDataUnitMsg);

	if (!pSndMsg)
		return ERROR;

	End_post(pSndMsg);

	return OK;
}

//上报从节点信息
void PLC_report_node_ifo(P_MF_RELAY_REG_METER_INFO pRptMeterIfo)
{
	P_MSG_INFO pMsgIfo;
	P_MSG_INFO pMsgData;
	PBYTE pDataUnit;
	int i, p = 0;
	BYTE nodeCounter = 0;

	if (pRptMeterIfo->meterCount <= 0)
		return;

	if (!(pMsgData = (P_MSG_INFO)alloc_send_buffer(__func__,MSG_SHORT)))
	{
		//Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);
		return;
	}

	pDataUnit = &pMsgData->msg_buffer[0];

	p++;

	for (i = 0; i < pRptMeterIfo->meterCount; i++)
	{
		P_MF_RELAY_METER_ITEM pItem = &pRptMeterIfo->Items[i];

		if (IsMemSet(pItem->meterAddr, 0xAA, FRM_DL645_ADDRESS_LEN))
			continue;

		if (IsMemSet(pItem->meterAddr, 0x99, FRM_DL645_ADDRESS_LEN))
			continue;

		memcpy_special(&pDataUnit[p], pItem->meterAddr, FRM_DL645_ADDRESS_LEN);
		p += 6;
		pDataUnit[p++] = (1 == pItem->meterProto) ? 1 : 2;
		//从节点序号
		#if 0
		memcpy_special(&pDataUnit[p], (void*)&pItem->meterSn, 2);
		#else
		*(u16 *)&pDataUnit[p] = pItem->meterSn;
		#endif
		p += 2;
		nodeCounter++;
	}

	if (0 == nodeCounter)
	{
		free_send_buffer(__func__,pMsgData);
		return;
	}

	pDataUnit[0] = nodeCounter;

	pMsgIfo = LMP_make_frame_3762_cmd(AFN_REPORT_INFO, F1, PLM_PHASE_A, pDataUnit, p, FALSE, NULL);
	free_send_buffer(__func__,pMsgData);

	if (!pMsgIfo)
	{
		return;
	}

	pMsgIfo->msg_header.end_id = COM_PORT_MAINTAIN;
	pMsgIfo->msg_header.need_buffer_free = TRUE;
	pMsgIfo->msg_header.send_delay = false;
	End_post(pMsgIfo);
}

//上报从节点信息及设备类型
void PLC_report_dev_ifo(P_MF_RELAY_REG_METER_INFO pRptMeterIfo)
{
	P_MSG_INFO pMsgIfo;
	P_MSG_INFO pMsgData;
	PBYTE pDataUnit;
	int i, j, p = 0;

	BYTE nodeCounter = 0;
	PBYTE pSubNodeCounter = NULL;
	PBYTE pSubSendCounter = NULL;
	BYTE subNodeCounter = 0;
	BYTE subSendCounter = 0;
	BYTE bReportColl = FALSE;

	if (pRptMeterIfo->meterCount <= 0)
		return;

	if (!(pMsgData = (P_MSG_INFO)alloc_send_buffer(__func__,MSG_SHORT)))
	{
		//Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);
		return;
	}

	pDataUnit = &pMsgData->msg_buffer[0];
	//上报从节点的数量n
	p++;

	for (i = 0; i < pRptMeterIfo->meterCount; i++)
	{
		P_MF_RELAY_METER_ITEM pItem = &pRptMeterIfo->Items[0];

		if (4 == pItem->meterProto)
			bReportColl = TRUE;
		else
			bReportColl = FALSE;

		memcpy_special(&pDataUnit[p], pItem->meterAddr, FRM_DL645_ADDRESS_LEN);
		p += 6;
		//通讯协议
		if (pItem->meterProto <= 2)
			pDataUnit[p++] = pItem->meterProto;
		else
			pDataUnit[p++] = 2;

		//从节点序号
		#if 0
		memcpy_special(&pDataUnit[p], &pItem->meterSn, 2);
		#else
		*(u16 *)&pDataUnit[p] = pItem->meterSn;
		#endif
		p += 2;
		//从节点设备类型
		pDataUnit[p++] = (bReportColl) ? 0 : 1; //0采集器，1电能表
		pSubNodeCounter = &pDataUnit[p++];
		subNodeCounter = 0;
		pSubSendCounter = &pDataUnit[p++];
		subSendCounter = 0;
		//载波节点下挂从节点
		if (bReportColl)
		{
			for (j = i + 1; j < pRptMeterIfo->meterCount; j++)
			{
				P_MF_RELAY_METER_ITEM pSubItem = &pRptMeterIfo->Items[j];
				if (4 == pSubItem->meterProto)
				{
					i = j;
					break;
				}
				memcpy_special(&pDataUnit[p], pSubItem->meterAddr, FRM_DL645_ADDRESS_LEN);
				p += 6;
				pDataUnit[p++] = pSubItem->meterProto;
				(subNodeCounter)++;
				(subSendCounter)++;
				i = j + 1;
			}
		}

		memcpy_special(pSubNodeCounter, &subNodeCounter, 1);
		memcpy_special(pSubSendCounter, &subSendCounter, 1);
		nodeCounter++;
	}

	if (0 == nodeCounter)
	{
		free_send_buffer(__func__,pMsgData);
		return;
	}

	pDataUnit[0] = nodeCounter;

	pMsgIfo = LMP_make_frame_3762_cmd(AFN_REPORT_INFO, F4, PLM_PHASE_U, pDataUnit, p, FALSE, NULL);
	free_send_buffer(__func__,pMsgData);

	if (!pMsgIfo)
	{
		return;
	}

	pMsgIfo->msg_header.end_id = COM_PORT_MAINTAIN;
	pMsgIfo->msg_header.need_buffer_free = TRUE;
	pMsgIfo->msg_header.send_delay = false;
	End_post(pMsgIfo);
}

unsigned short PLC_stage_start()
{
	for (u16 sn = 0; sn < CCTT_METER_NUMBER; sn++)
	{
		g_MeterRelayInfo[sn].freeze_done = 0;
	}

	PLC_ClearCacheData();
	PLC_stage_resume();
	P_PLC_ROUTE_READ_CB pcb = &g_pRelayStatus->route_read_cb;
	pcb->sn=0;
	g_pRelayStatus->plc_start_time = GetTickCount64();

	return OK;
}

unsigned short PLC_stage_pause()
{
	g_pRelayStatus->pause_sec = 0xff;
	return OK;
}

unsigned short PLC_stage_resume()
{
	if (g_pRelayStatus->pause_sec > 0)
		g_pRelayStatus->pause_sec = 1;
	return OK;
}

unsigned short Relay_GetRouteReadMin()
{
	OS_TICK_64 tick_pass = GetTickCount64() - g_pRelayStatus->plc_start_time;

	return (unsigned short)(tick_pass / OSCfg_TickRate_Hz / 60);
}

void PLC_trigger_rmsg_report()
{
	g_mf_report_rmsg = 1;
}

#ifdef APP_READ_SPECIAL_DI
BYTE PLC_IsDIReadable(ULONG DI)
{
	BYTE i;
	BYTE cnt = countof(DI_READABLE);

	for (i = 0; i < cnt; i++)
	{
		if (DI == DI_READABLE[i])
			return TRUE;
	}

	return FALSE;
}
#endif
//由递归改为全遍历 放止孤儿节点没有被遍历到
//离线节点
u8 offlineBitMap[MAX_BITMAP_SIZE];
void PLC_StaStatAndOffline()
{


	//debug_str("sec\r\n");
	memset(offlineBitMap, 0, sizeof(offlineBitMap));
	lockStaInfo();


	//if(g_pRelayStatus->cfged_meter_num < MAX_NODE_COUNT_PROTOCOL_TEST)
	//max_search_beacon_cnt = 1;

	g_pRelayStatus->innet_num = 0;
	g_pRelayStatus->online_num = 0;
	g_pRelayStatus->check_online = 0;
	g_pRelayStatus->max_tei=0;
	g_pRelayStatus->pco_num = 0;
	g_pRelayStatus->sta_num = 0;
	g_pRelayStatus->sta_num_not_level_1 = 0;
	memset_special(g_pRelayStatus->innet_num_phase, 0, sizeof(g_pRelayStatus->innet_num_phase));
	memset_special(g_pRelayStatus->innet_num_level, 0, sizeof(g_pRelayStatus->innet_num_level));
	memset_special(g_pRelayStatus->sta_num_level, 0, sizeof(g_pRelayStatus->sta_num_level));

	g_pRelayStatus->assoc_gather_ind_num = 0;



//    if(g_pRelayStatus->cfged_meter_num > 0)       //主动注册需要关闭
	{
		for (USHORT tei = TEI_STA_MIN; tei < TEI_CNT; tei++)
		{
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
			if (NULL == pSta)
				continue;

			if (NET_OUT_NET == pSta->net_state)
				continue;
			
			P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);

			g_pRelayStatus->innet_num++;

			if (NET_IN_NET == pSta->net_state)
			{
				g_pRelayStatus->online_num++;
				if (pStaStatus->check_in_net)
				{
					g_pRelayStatus->check_online++;
				}
				extern u8  max_beacon_period_ms;				
				if (pSta->moduleType == 0
#ifdef GDW_JIANGXI
					&& (g_PlmConfigPara.network_mode != NETWORK_MODE_HRF)
#endif
				)//plc单模
				{
					max_beacon_period_ms = 10;
				}				
			}
			g_pRelayStatus->max_tei=tei;
			u16 level = PLC_GetStaLevel(tei);
			if (0 != pSta->child)
			{
				g_pRelayStatus->pco_num++;
				if (level <= MAX_RELAY_DEEP)
				{
					g_pRelayStatus->innet_num_level[level]++;
				}
				else
				{
					if (level == (MAX_RELAY_DEEP + 1))
					{
//						if (pSta->net_state == NET_IN_NET)
//						{
//							pSta->net_state = NET_OFFLINE_NET;
//							level=pSta-g_pStaInfo;
						SET_STA_BITMAP(offlineBitMap, tei);
//						}
					}
				}
			}
			else
			{
				g_pRelayStatus->sta_num++;
				if (level <= MAX_RELAY_DEEP)
				{
					g_pRelayStatus->innet_num_level[level]++;
					g_pRelayStatus->sta_num_level[level]++;
				}
				else
				{
					if (level == (MAX_RELAY_DEEP + 1))
					{
//						if (pSta->net_state == NET_IN_NET)
//						{
//							pSta->net_state = NET_OFFLINE_NET;
//							level=pSta-g_pStaInfo;
						SET_STA_BITMAP(offlineBitMap, tei);
//						}
					}
				}

			}


			if (TEI_CCO != pSta->pco)
			{
				g_pRelayStatus->sta_num_not_level_1++;
			}
			
			if (PHASE_A == pSta->hplc_phase)
				g_pRelayStatus->innet_num_phase[0]++;
			else if (PHASE_B == pSta->hplc_phase)
				g_pRelayStatus->innet_num_phase[1]++;
			else if (PHASE_C == pSta->hplc_phase)
				g_pRelayStatus->innet_num_phase[2]++;
			else if (PHASE_ALL == pSta->hplc_phase)
				g_pRelayStatus->innet_num_phase[3]++;
			
			if (pStaStatus->assoc_gather_ind)
				g_pRelayStatus->assoc_gather_ind_num++;

			if (pStaStatus->silence_sec > 3366)      //420 x 2 x 4 = 3360
				continue;

			if (g_pRelayStatus->heartbeat_period_sec <= 0)
				continue;

			pStaStatus->silence_sec++;

			//debug_str("tei=%d, silence_sec=%d\r\n", tei, pStaStatus->silence_sec);

			if (pStaStatus->silence_sec > 4 * g_pRelayStatus->heartbeat_period_sec)
			{
				if (pSta->net_state != NET_OUT_NET)
				{
					debug_str(DEBUG_LOG_NET, "tei %d %02x%02x%02x%02x%02x%02x 未入网 %d %d %d\r\n", tei,
							  pSta->mac[5], pSta->mac[4], pSta->mac[3], pSta->mac[2], pSta->mac[1], pSta->mac[0],
							  pSta->net_state, pStaStatus->silence_sec, g_pRelayStatus->heartbeat_period_sec);
					pSta->net_state = NET_OUT_NET;
					SET_STA_BITMAP(offlineBitMap, tei);
				}
				else if (PLC_GetWhiteNodeSnByStaInfo(pSta) == CCTT_METER_NUMBER) //已不在白名单中删除此节点
				{
					PLC_FreeStaInfoRelationship(tei);
				}
			}
			else if (pStaStatus->silence_sec > g_pRelayStatus->heartbeat_period_sec)
			{
				if (pSta->net_state != NET_OFFLINE_NET)
				{
#ifdef GDW_2019_PERCEPTION
					OS_ERR err;
					OSSchedLock(&err);
					//u16 node_sn=PLC_GetWhiteNodeSnByStaInfo(pSta);
					//if (node_sn < CCTT_METER_NUMBER)
					{
						goto_offline_bitmap[tei >> 5] |= 1 << (tei & 0x1f);
						pSta->offline_tick=GetTickCount64()/(OS_CFG_TICK_RATE_HZ);
					}

					OSSchedUnlock(&err);
#elif defined(NW_GUANGDONG_NEW_LOCAL_INTERFACE) 
                    OS_ERR err;
					OSSchedLock(&err);

					pSta->offline_tick = GetTickCount64()/(OS_CFG_TICK_RATE_HZ);

					OSSchedUnlock(&err);
#endif
					debug_str(DEBUG_LOG_NET, "tei %d %02x%02x%02x%02x%02x%02x 离线 %d %d %d\r\n", tei,
							  pSta->mac[5], pSta->mac[4], pSta->mac[3], pSta->mac[2], pSta->mac[1], pSta->mac[0],
							  pSta->net_state, pStaStatus->silence_sec, g_pRelayStatus->heartbeat_period_sec);
					g_pRelayStatus->offline_time++;
					pSta->net_state = NET_OFFLINE_NET;
//					level = pSta - g_pStaInfo;
					#ifdef HPLC_CSG
					pStaStatus->leaveTime++;
					#endif
//					SET_STA_BITMAP(offlineBitMap, level);
					PLC_TriggerSaveStaInfo();
				}
				
			}

		}
		debug_str(DEBUG_LOG_NET,"online num is %d check %d innet %d pco %d\r\n", g_pRelayStatus->online_num, g_pRelayStatus->check_online, g_pRelayStatus->innet_num, g_pRelayStatus->pco_num);
		for (int i = 0; i < sizeof(offlineBitMap); i++)
		{
			if (offlineBitMap[i])
			{
				for (int x = 0; x < 8; x++)
				{
					if (offlineBitMap[i] & (1 << x))
					{
						SplitFormePco((i << 3) | x);
					}
				}

			}
		}
	}

	
	unLockStaInfo();
}

//重新组网
void PLC_RestartNetworking()
{
	OS_ERR err;
	OSSchedLock(&err);
	#ifdef ERROR_APHASE
	g_pRelayStatus->phase_err_cb.sta_percent=0;
	#endif
	//清TEI
	for (u16 tei = TEI_STA_MIN; tei < TEI_CNT; tei++) 
	{
		PLC_FreeStaInfo(tei);
	}
	g_pStaInfo[CCO_TEI].brother=0;
	g_pStaInfo[CCO_TEI].child = 0;
	g_pStaInfo[CCO_TEI].pco = 0;
	PLC_TriggerSaveStaInfo();

	//重新组网
	g_pPlmConfigPara->net_seq++;
	dc_flush_fram(&g_pPlmConfigPara->net_seq, sizeof(g_pPlmConfigPara->net_seq), FRAM_FLUSH);
	MpduSetNID(true, g_pPlmConfigPara->net_id);

	g_pRelayStatus->search_level = 0;

	OSSchedUnlock(&err);
}
void PLC_DetectNid()
{
	OS_ERR err;
	if (g_pPlmConfigPara->isNeedChangeNid)
	{
		OSSchedLock(&err);
		g_pPlmConfigPara->isNeedChangeNid=0;
		//清TEI
		for (u16 tei = TEI_STA_MIN; tei < TEI_CNT; tei++)
		{
			PLC_FreeStaInfo(tei);
		}
		g_pStaInfo[CCO_TEI].brother = 0;
		g_pStaInfo[CCO_TEI].child = 0;
		g_pStaInfo[CCO_TEI].pco = 0;
		PLC_TriggerSaveStaInfo();

		//重新组网
		g_pPlmConfigPara->net_seq++;
		g_pPlmConfigPara->net_id= g_pPlmConfigPara->new_nid;
		dc_flush_fram(&g_pPlmConfigPara->net_seq, sizeof(g_pPlmConfigPara->net_seq), FRAM_FLUSH);
		dc_flush_fram(&g_pPlmConfigPara->net_id, sizeof(g_pPlmConfigPara->net_id), FRAM_FLUSH);
		MpduSetNID(true, g_pPlmConfigPara->net_id);
		g_pRelayStatus->search_level = 0;

		OSSchedUnlock(&err);
	}
}


u32 s_plc_idle_cnt = 0;
u32 s_plc_receiving_cnt = 0;
u32 s_plc_rxbusy_cnt = 0;
u32 s_plc_unknow_cnt = 0;

void PLC_RxStateSta()
{
	PLC_STATE plc_state = BPLC_GetRxState();
	if (PLC_STATE_IDLE == plc_state)
		s_plc_idle_cnt++;
	else if (PLC_STATE_RECEIVING == plc_state)
		s_plc_receiving_cnt++;
	else if (PLC_STATE_RXBUSY == plc_state)
		s_plc_rxbusy_cnt++;
	else
		s_plc_unknow_cnt++;
}



//检查中继地址
bool Check_Relay_Addr(u8* mac)
{
	u8 relay_addr[6] = {0};
	u16 realy_addr_low = 0; //低位2字节

	memcpy_swap(relay_addr, mac, MAC_ADDR_LEN);
	
	//FFFFFFFF0001-FFFFFFFFFFFC
	if (memIsHex(relay_addr, 0xFF, 4))
	{
		realy_addr_low = htons(*(u16 *)&relay_addr[4]);
		
		if (g_PlmConfigPara.close_white_list == 0) //开启白名单
		{
			//按设置的中继范围判断
			if ((realy_addr_low>=g_pPlmConfigPara->relay_addr_start) && (realy_addr_low<=g_pPlmConfigPara->relay_addr_end))
			{
				return true;
			}
		}
		else
		{
			//按默认范围判断，FFFFFFFF0001-FFFFFFFFFFFC
			if ((realy_addr_low>=0x0001) && (realy_addr_low<=0xFFFC))
			{
				return true;
			}
		}
	}

	return false;
}


void PLC_ReportNodeInfoProc(void)
{
		int i, j;
	    u8 need_report = 0;
		u8 max_report_num = 1; //1次只上报一个节点信息
	    u16 len = 1 + 9 * max_report_num;
	    u8 *pDataUnit;
		u8 mac_addr[MAC_ADDR_LEN] = {0};
	u8 *pNodeInfo = NULL;
	u8 *total_num = NULL;

	if (g_PlmConfigPara.close_white_list == 2) //关闭白名单档案不写入flash，目前只针对海外版本，需要上报节点信息
	{
	    static u8 second_node_info = 0;
        
	    second_node_info++;
        if (second_node_info%2 == 0) //2秒上报1次
        {
            second_node_info = 0;
        }
        else
        {
            return;
        }

	    for (i = 0; i < (MAX_BITMAP_SIZE / 4); i++)
	    {
	        if (nodeinfo_bitmap[i] != 0)
	        {
	        	if (nodeinfo_bitmap[i] & (~nodeinfo_report_bitmap[i]))
	        	{
	            need_report = 1;
	            break;
	        }
	    }
	    }

	    if (!need_report)
	    {
	        return;
	    }

	    pDataUnit = alloc_buffer(__func__, len);
	    if (!pDataUnit)
	    {
	        return;
	    }

	    pNodeInfo = pDataUnit;
	    total_num = pNodeInfo; //总数量
	    *total_num = 0;
	    pNodeInfo++;

	    for (i = i; i < (MAX_BITMAP_SIZE / 4); i++)
		{
			for (j = 0; j < 32; j++)
			{
				if (nodeinfo_bitmap[i] & (1 << j))
				{	
					if (!(nodeinfo_report_bitmap[i]&(1<<j))) //没有上报过
					{
						nodeinfo_bitmap[i] &= ~((1 << j));
						nodeinfo_report_bitmap[i] |= (1 << j);
						
						u16 tei = (i << 5) | (j & 0x1f);
						P_STA_INFO pSta = PLC_GetStaInfoByTei(tei);

						//中继地址不上报
						if (true == Check_Relay_Addr(pSta->mac))
						{
							continue;
						}

						//从节点地址
						memcpy_special(mac_addr, pSta->mac, MAC_ADDR_LEN);
						memcpy_special(pNodeInfo, pSta->mac, LONG_ADDR_LEN);
						pNodeInfo += 6;

						//通信协议类型
						*pNodeInfo = 0; //默认透明传输
						pNodeInfo += 1;
						
						//序号
					    u16 meter_sn = PLC_get_sn_from_addr(pSta->mac);
					    if (meter_sn < CCTT_METER_NUMBER)
					    {
					        *(u16 *)pNodeInfo = g_pMeterRelayInfo[meter_sn].user_sn;
					    }
						else
						{
							*(u16 *)pNodeInfo = 0;
						}
						pNodeInfo += 2;

						(*total_num)++;
						if ((*total_num) == max_report_num)
						{
							break;
						}
					}
				}
			}
			if ((*total_num) == max_report_num)
			{
				break;
			}
		}
	}
	else if((g_PlmConfigPara.close_white_list == 0) && (g_PlmStatusPara.report_not_white_sta == 1))
	{
	    static u8 second_no_white = 0;
        
	    second_no_white++;
        if (second_no_white%2 == 0) //2秒上报1次
        {
            second_no_white = 0;
        }
        else
        {
            return;
        }
        
	    if (NotWhiteSta.widx == 0)
	    {
	        return;
	    }

	    pDataUnit = alloc_buffer(__func__, len);
	    if (!pDataUnit)
	    {
	        return;
	    }

	    pNodeInfo = pDataUnit;
	    total_num = pNodeInfo; //总数量
	    *total_num = 0;
	    pNodeInfo++;

	    for (i=0; i<MAX_NOT_WHITE_SIZE; i++)
		{
			if (!memIsHex(&NotWhiteSta.mac[i][0], 0x00, 6))
			{	
				//从节点地址
				memcpy_special(mac_addr, &NotWhiteSta.mac[i][0], MAC_ADDR_LEN);
				memcpy_special(pNodeInfo, &NotWhiteSta.mac[i][0], LONG_ADDR_LEN);
				pNodeInfo += 6;

				//通信协议类型
				*pNodeInfo = 0; //默认透明传输
				pNodeInfo += 1;
				
				//序号
				*(u16 *)pNodeInfo = 0;
				pNodeInfo += 2;

				memset_special(&NotWhiteSta.mac[i][0], 0x00, MAC_ADDR_LEN);
				NotWhiteSta.widx--;
				(*total_num)++;
				if ((*total_num) == max_report_num)
				{
					break;
				}
			}

			if ((*total_num) == max_report_num)
			{
				break;
			}
		}
	}

    if ((*total_num) == 0)
    {
        free_buffer(__func__, pDataUnit);
        return;
    }
    else
    {
        len = 1 + 9 * (*total_num);
		P_MSG_INFO pSendMsg = LMP_make_frame_3762_cmd(AFN_REPORT_INFO, F1, PLM_PHASE_U, pDataUnit, len, TRUE, mac_addr);

	    free_buffer(__func__, pDataUnit);

	    if (NULL != pSendMsg)
	    {
	        pSendMsg->msg_header.send_delay = true;
			End_post(pSendMsg);
	    }
	}	
}


bool ReportIDMsg(u8 *data,u16 len)
{
	if (data)
	{
		P_MSG_INFO pSendMsg = LMP_make_frame_3762_cmd(AFN_RELAY_QUERY, F40, PLM_PHASE_U, data, len, false, NULL);
		if (NULL == pSendMsg)
		{
			return false;
		}
		End_post(pSendMsg);
		return true;
	}
	else
	{
		u8 err_code=0;
		P_MSG_INFO pSendMsg = LMP_make_frame_3762_cmd(AFN_RESPONSE, F2, PLM_PHASE_U, &err_code, 1, false, NULL);
		if (NULL == pSendMsg)
		{
			return false;
		}
		End_post(pSendMsg);
		return true;
	}

	
}

#ifdef GDW_2019_PERCEPTION
//陕西全网感知
bool ReportMsgSend(u32 bitmap[MAX_BITMAP_SIZE/4],u8 line,BOOL isPlcMeterCommModule, u8 meter_addr[FRM_DL645_ADDRESS_LEN])
{
	int i;
	int j;
//	OS_ERR err;
	u8 cur_total = 10;
	u16 len = 5 + sizeof(RT_REPORT_METER_LINE_STATUS) * 10; //申请最大
	PBYTE p = alloc_buffer(__func__,len);
	if (p == NULL)
	{
		return false;
	}
	u16 *total_num = (u16 *)p; //总数量
	u8 *cur_cnt = p + 2; //本次数量

	*total_num = 0;
	*cur_cnt = 0;
	*(u16 *)&p[3] = 0; //开始序号
	RT_REPORT_METER_LINE_STATUS *pStatus = (RT_REPORT_METER_LINE_STATUS *)&p[5];


	for (i = 0; i < (MAX_BITMAP_SIZE / 4); i++)
	{
		if (bitmap[i])
		{
			for (j = 0; j < 32; j++)
			{
				if (bitmap[i] & (1 << j))
				{
					bitmap[i] &= ~((1 << j));
					//u16 sn = (i << 5) | (j & 0x1f);
					//P_RELAY_INFO prelay = &g_pMeterRelayInfo[sn];
					P_STA_INFO p_info = PLC_GetValidStaInfoByTei((i<<5)|j);
					if (p_info==NULL)
					{
						continue;
					}
					--cur_total;
					++(*total_num);
					++(*cur_cnt);
					pStatus->result = 0;
					pStatus->status = line;
					if (line) //在线变为掉线
					{
						pStatus->time = 0xeeeeeeee;
					}
					else
					{
						
						if (offline_reason[i]&(1<<j))
						{
							pStatus->result = 1;//掉电
							offline_reason[i]&=~(1<<j);
						}
						else if (p_info!=NULL
								 && p_info->re_join)
						{
							pStatus->result = 2;//网络变化
						}
						if ((p_info->offline_tick)&&(p_info->offline_tick != 0xFFFFFFFF))
						{
							pStatus->time = GetTickCount64() / OS_CFG_TICK_RATE_HZ - p_info->offline_tick;
						}
						else
						{
							pStatus->time = 0;
						}
						p_info->offline_tick = 0;
					}
					memcpy_special(pStatus->mac, p_info->mac, LONG_ADDR_LEN);

					//P_STA_INFO pSta = PLC_GetStaInfoByMac(g_pMeterRelayInfo[sn].meter_addr);
					P_STA_CHIP_ID_INFO pChipID = (P_STA_CHIP_ID_INFO)&pStatus->chip_id;
					pChipID->fix_code1 = 0x01;
					pChipID->fix_code2 = 0x02;
					pChipID->fix_code3 = 0x9c;
					pChipID->fix_code4[0] = 0xfb;
					pChipID->fix_code4[1] = 0xc1;
					pChipID->fix_code4[2] = 0x01;
					//pChipID->dev_class = 2;//p_info->moduleType;
					memcpy_special(&pChipID->dev_class, &p_info->dev_class, sizeof(p_info->dev_class));
					memcpy_special(pChipID->factory_code, p_info->factory_code, sizeof(p_info->factory_code));
					memcpy_special(pChipID->chip_code, p_info->chip_code, sizeof(p_info->chip_code));
					memcpy_special(pChipID->dev_seq, p_info->dev_seq, sizeof(p_info->dev_seq));
					memcpy_special(pChipID->check_sum, p_info->check_sum, sizeof(p_info->check_sum));


					pStatus++;
				}
				if (!cur_total)
				{
					break;
				}
			}
		}
		if (!cur_total)
		{
			break;
		}
	}
	if (cur_total == 10)
	{
		free_buffer(__func__,p);
		return false;
	}
	else
	{
		P_MSG_INFO pSendMsg = LMP_make_frame_3762_cmd(AFN_REPORT_INFO, F10, PLM_PHASE_U, p, sizeof(RT_REPORT_METER_LINE_STATUS) * (10 - cur_total) + 5, isPlcMeterCommModule, meter_addr);
		free_buffer(__func__,p);
		if (NULL == pSendMsg)
		{
			return false;
		}
		End_post(pSendMsg);
		return true;
	}
}
void PLC_ReportMeterStatusProc(void)
{
//	OS_ERR err;
	static u16 sec_t=0;
	if (sec_t++ < 604)
	{
		return;
	}
	sec_t = 600;
	if (!ReportMsgSend(goto_offline_bitmap, 1, false, NULL))
	{
		if(ReportMsgSend(goto_online_bitmap,0, false, NULL))
		{
			//OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz / 2, OS_OPT_PEND_BLOCKING, NULL, &err);
		}
	}
	else
	{
		//OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz / 2, OS_OPT_PEND_BLOCKING, NULL, &err);
	}

}
#endif


#ifdef HPLC_CSG
void PLC_RequestCentreTimePeriod(void)
{
    static int get_centre_time = 1;
    static u8 RequestCentreTime_Flag = 0;
    if (g_pRelayStatus->init_meter_delay)
    {
        g_pRelayStatus->init_meter_delay--;
    }

#ifdef REQ_CENTRE_TIME_FIXED_VALUE 
    if ((g_pRelayStatus->init_meter_delay == 0) || RequestCentreTime_Flag)
#else
    if ((g_pRelayStatus->online_num&&g_pRelayStatus->init_meter_delay==0) || RequestCentreTime_Flag)
#endif        
    {
        get_centre_time--;
        if (get_centre_time == 0)
        {
            get_centre_time = 15*60;

			g_PlmStatusPara.no_response_times++;
			if (g_PlmStatusPara.no_response_times > 4)
			{			
	            g_PlmStatusPara.t_now_sec = 0;
	            g_PlmStatusPara.t_gettime_sec = 0;
				g_PlmStatusPara.no_response_times = 0;
			}

			debug_str(DEBUG_LOG_APP, "req local time, no response times:%d\r\n", g_PlmStatusPara.no_response_times);
            PLM_RequestCentreTime_gd();
            RequestCentreTime_Flag = 1;
        }
    }
}
#endif


#ifdef GDW_ZHEJIANG
void HPLC_OnlineTimeLockSet()
{
	P_PLC_ONLINE_LOCK_TIME_CB pcb = &g_pRelayStatus->online_lock_time_cb;
	pcb->send_counter=10;
	
	PLC_OnlineLockTimeTime();
}

//发送在网锁定时间
void HPLC_SendOnlineTimeLock()
{
	P_APP_PACKET pAppFrame = NULL;
	USHORT msdu_len = 0;
	USHORT tei = TEI_BROADCAST;//广播
	u16 FrameSN;
	P_PLC_ONLINE_LOCK_TIME_CB pcb = &g_pRelayStatus->online_lock_time_cb;
	pcb->FrameSN++;
	FrameSN = pcb->FrameSN;

	pAppFrame = HplcApp_MakeSetOnlineTimeLockFrame(pcb->OnlineLockTime, pcb->abnormalOfflineLockTime,FrameSN);

	if(pAppFrame == NULL)
	{
		goto L_Error;
	}
	msdu_len = sizeof(APP_PACKET) - 1 + sizeof(APP_ONLINE_LOCK_TIME_PACKET) - 1 + sizeof(APP_ONLINE_LOCK_TIME);

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 1;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_ONLINE_LOCK_TIME;

	mac_param.DstTei = TEI_BROADCAST;
	mac_param.SendType = SENDTYPE_BROADCAST;
	mac_param.SendLimit = 4;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return ;

L_Error:
	free_buffer(__func__,pAppFrame);

}
#endif


void PLC_second_task()
{
	OS_ERR err;
	
	if ((g_pRelayStatus->pause_sec > 0) && (0xff != g_pRelayStatus->pause_sec))
		g_pRelayStatus->pause_sec--;

	if (g_pRelayStatus->reboot_sec_down && (--g_pRelayStatus->reboot_sec_down == 0))
	{
		if (g_pRelayStatus->save_nodes_sec_down > 0)
		{
			g_MeterAddrSuccess=true;
			SYS_STOR_METER();
		}
//		if (g_pRelayStatus->save_sta_sec_down > 0)
//			data_flash_write_straight_with_crc(STA_INFO_ADDR, (u8 *)g_pStaInfo, TEI_CNT * sizeof(STA_INFO));

#if defined(EXTER_WATCH_DOG) || defined(INTER_WATCH_DOG)
		OSSchedLock(&err);
//        SAFE_SYSTEM_RESET(SYS_RESET_POS_0);
		Reboot_system_now(__func__);
#else
		*(int *)0x40100020 |= (1 << 10);      //软复位命令
#endif
	}

	if (g_pRelayStatus->route_period_sec_down)
		g_pRelayStatus->route_period_sec_down--;

	if (g_pRelayStatus->save_nodes_sec_down && (0 == --g_pRelayStatus->save_nodes_sec_down))
	{
		g_MeterAddrSuccess=true;
		SYS_STOR_METER();
	}
#ifdef GDW_ZHEJIANG	
	if (g_pRelayStatus->save_black_nodes_sec_down && (0 == --g_pRelayStatus->save_black_nodes_sec_down))
	{
		SYS_STOR_BLACK_METER();
	}
#endif //GDW_ZHEJIANG	

#ifdef PROTOCOL_NW_2023_GUANGDONG_V2DOT1
	if (g_pRelayStatus->save_OutListMeter_sec_down && (0 == --g_pRelayStatus->save_OutListMeter_sec_down))
	{
		SYS_STOR_OUTLIST_METER();
	}
#endif

//		data_flash_write_straight_with_crc(STA_INFO_ADDR, (u8 *)g_pStaInfo, TEI_CNT * sizeof(STA_INFO));

//		data_flash_write_straight_with_crc(STA_INFO_ADDR, (u8 *)g_pStaInfo, TEI_CNT * sizeof(STA_INFO));

	if (g_pRelayStatus->sta_offline_sec_down && (0 == --g_pRelayStatus->sta_offline_sec_down))
		PLC_CheckOfflineList();

	if (g_pRelayStatus->sta_change_netid_sec_down && (0 == --g_pRelayStatus->sta_change_netid_sec_down))
		PLC_RestartNetworking();

#if 0
	//PLC_SaveNodeOnce();
#endif
	//档案统计
	PLC_get_relay_info();
	//STA统计
	PLC_StaStatAndOffline();
	//离线指示报文的发送
	PLC_SendOfflineIndAuto();

	
#ifdef GDW_2019_PERCEPTION
//陕西全网感知上报
	PLC_ReportMeterStatusProc();
#endif

	//定时纠正STA角色
//    PLC_TimerRefixedRoles();

#ifdef GDW_ZHEJIANG
	if (g_pRelayStatus->online_lock_time_cb.second_down > 0)
	{
		OSSchedLock(&err);
		g_pRelayStatus->online_lock_time_cb.second_down--;

		if(!g_pRelayStatus->online_lock_time_cb.second_down)
		{
			//启动广播锁网时间
			HPLC_OnlineTimeLockSet();
		}

		OSSchedUnlock(&err);
	}
#endif
	if (g_pRelayStatus->change_frequence_cb.second_down > 0)
	{
		OSSchedLock(&err);
		g_pRelayStatus->change_frequence_cb.second_down--;      //河北电科院台体设置频段后，等待360秒
		if (0 == g_pRelayStatus->change_frequence_cb.second_down)
		{
			HPLC_ChangeFrequence(g_pRelayStatus->change_frequence_cb.new_frequence, FALSE);
			debug_str(DEBUG_LOG_NET, "设置新频率时间到%d\r\n", g_pRelayStatus->change_frequence_cb.new_frequence);
		}
		OSSchedUnlock(&err);
	}
	//hrf channel 切换
	if (g_pRelayStatus->change_hrf_channel_cb.second_down > 0)
	{
		OSSchedLock(&err);
		g_pRelayStatus->change_hrf_channel_cb.second_down--;      //河北电科院台体设置频段后，等待360秒
		if (2 == g_pRelayStatus->change_hrf_channel_cb.second_down)//机台会在信标周期结束的瞬间等待hrf变更过来 此处提前1s变更 应对机台  不影响实际使用
		{
			HRF_ChangeChannel(g_pRelayStatus->change_hrf_channel_cb.new_frequence&0xff,g_pRelayStatus->change_hrf_channel_cb.new_frequence>>8);
		}
		OSSchedUnlock(&err);
	}

#ifdef PROTOCOL_GD_2016
	PLC_read_task_timeout_check_gd();
#endif

#ifdef OVERSEA
	static u8 second_num = 0;
	second_num++;
	if(second_num > 60) //启动后60秒
	{
		second_num = 60;
		PLC_ReportNodeInfoProc();
	}
#endif

	if (g_mf_report_rmsg)
	{
		g_mf_report_rmsg--;
		if (g_mf_report_rmsg == 0)
		{
#ifndef ZHENGTAI
			PLC_report_msg_to_main();
#endif
#ifdef GDW_JIANGSU
			if(routerReqeustCenterRTCTimerId == INVAILD_TMR_ID)
			{
				PLC_RequestCentreTime(0);
				routerReqeustCenterRTCTimerId = HPLC_RegisterTmr(PLC_RequestCentreTime,NULL, 12 * 60 * 60 * 1000, TMR_OPT_CALL_PERIOD);
			}
#endif
#ifdef GW_BEACON_ITEM_UTC
			if(getCentreTimerId == INVAILD_TMR_ID)
			{
				PLC_RequestCentreTime(0);
				getCentreTimerId = HPLC_RegisterTmr(PLC_RequestCentreTime,NULL, 15 * 60 * 1000, TMR_OPT_CALL_PERIOD);
			}
#endif
		}
	}

#ifdef HPLC_CSG
	PLC_RequestCentreTimePeriod();
#endif

	if (g_PlmStatusPara.t_now_sec)
	{
		OS_TICK_64 tick_now = GetTickCount64();
#ifdef WNL_UART_TRANS_DELAY
		//稳妥起见向上取整，以免延时有变
		unsigned long seconds = ((unsigned long)TicksBetween64(tick_now, g_PlmStatusPara.tick_gettime) + OSCfg_TickRate_Hz - 1) / OSCfg_TickRate_Hz;
#else
		unsigned long seconds = (unsigned long)TicksBetween64(tick_now, g_PlmStatusPara.tick_gettime) / OSCfg_TickRate_Hz;
#endif
		g_PlmStatusPara.t_now_sec  = g_PlmStatusPara.t_gettime_sec + seconds;
		if (seconds%60==0)
		{
			debug_str(DEBUG_LOG_APP, "time is %d\r\n", g_PlmStatusPara.t_now_sec);
		}
	}
}

void PLC_minute_task()
{
}

void PLC_save_relay_timer_start()
{
}

//flag==TRUE 工作信道，FALSE 组网信道
void PLC_set_rf_channel_flag(BYTE flag)
{
}

void PLC_get_relay_info()
{
	USHORT ii;
	P_RELAY_INFO p_relay_info;
	OS_ERR err;

	OSSchedLock(&err);

	g_pRelayStatus->all_meter_num = 0;
	g_pRelayStatus->cfged_meter_num = 0;
	g_pRelayStatus->selfreg_meter_num = 0;
	g_pRelayStatus->freeze_num = 0;

	for (ii = 0; ii < CCTT_METER_NUMBER; ii++)
	{
		p_relay_info = PLC_GetValidNodeInfoBySn(ii);

		if (NULL == p_relay_info)
			continue;

		if (p_relay_info->status.cfged)
			g_pRelayStatus->cfged_meter_num++;

		if (p_relay_info->status.selfreg)
			g_pRelayStatus->selfreg_meter_num++;

		g_pRelayStatus->all_meter_num++;

		if (PLC_IS_FREEZE(ii))
			g_pRelayStatus->freeze_num++;
	}
	debug_str(DEBUG_LOG_NET, "white list num is %d \r\n",g_pRelayStatus->cfged_meter_num);
	OSSchedUnlock(&err);
}

P_MSG_INFO PLC_make_frame_3762_resp(BYTE Afn, BYTE Fn, PBYTE pDataUnit, USHORT nDataLen, BOOL isPlcMeterCommModule, PBYTE pSrcAddr)
{
	P_LMP_LINK_HEAD pFrame;
	PBYTE p;
	P_MSG_INFO pMsg = NULL;

	if (!(pMsg = (P_MSG_INFO)alloc_send_buffer(__func__,MSG_SHORT)))
	{
		//Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);
		return NULL;
	}

	pFrame = (P_LMP_LINK_HEAD)pMsg->msg_buffer;
	p = pFrame->user_data;

	memset_special((PBYTE)pFrame, 0, sizeof(LMP_LINK_HEAD));

	pFrame->firstStartByte = 0x68;
	#if defined(HRF_SUPPORT) && !defined(COMM_MODE_OLD)
	pFrame->ctl_field = 0x84;
	#else
	pFrame->ctl_field = 0x83;
	#endif
	if (((AFN_DATA_TRANS == Afn) && (F5 == Fn))
		|| ((AFN_RESET == Afn) && (F7 == Fn)))
	{

		#ifdef HRF_SUPPORT
		pFrame->ctl_field = 0xc7;
		#else
		pFrame->ctl_field = 0xc7;
		#endif
	}

	if (isPlcMeterCommModule && pSrcAddr)  //对载波表的通信模块操作
	{
		//USHORT metersn;
		pFrame->Inf.up_info.comm_module = 1;
		//源地址A1
		memcpy_special(p, pSrcAddr, 6);
		p += 6;
		//中继地址A2
		pFrame->Inf.up_info.relay_level = 0;
		//目的地址A3
		memcpy_special(p, g_PlmConfigPara.main_node_addr, 6);
		p += 6;
	}

	pFrame->Inf.up_info.channel_phase = 1;

#ifdef GDW_2012
	pFrame->Inf.up_info.seq_no = g_PlmStatusPara.plm_seq_no_down;
#endif

	*p++ = Afn;
	*p++ = (BYTE)(1 << ((Fn - 1) & 0x07));    //DT1
	*p++ = (BYTE)((Fn - 1) >> 3);        //DT2
	pFrame->frm_len = 6
		+ sizeof(pFrame->Inf)
		+ (isPlcMeterCommModule ? 12 : 0)
		+ 3
		+ nDataLen;

	if (nDataLen)
	{
		memcpy_special(p, pDataUnit, nDataLen);
		p += nDataLen; //数据单元
	}

	*((PBYTE)pFrame + pFrame->frm_len - 2) = Get_checksum((PBYTE)&pFrame->ctl_field, pFrame->frm_len - 5);
	*((PBYTE)pFrame + pFrame->frm_len - 1) = 0x16;

	pMsg->msg_header.msg_len = pFrame->frm_len;
//    pMsg->msg_header.end_id = COM_PORT_MAINTAIN;
	pMsg->msg_header.need_buffer_free = TRUE;
	pMsg->msg_header.send_delay = false;
	return pMsg;
}

//获取458表地址
UCHAR PLC_GetRs485MeterAddr(P_GD_DL645_BODY pDl645, UCHAR *pRs485MeterAddr)
{
	if (0x01 == pDl645->controlCode)
	{
		if (0x08 == pDl645->dataLen)
		{
			memcpy_special((void *)pRs485MeterAddr, &pDl645->dataBuf[2], 6);
			AddValue(pRs485MeterAddr, 6, PLC_645_MINUS_0X33);
			return OK;
		}
	}
	else if (0x11 == pDl645->controlCode)
	{
		if (0x0A == pDl645->dataLen)
		{
			memcpy_special((void *)pRs485MeterAddr, &pDl645->dataBuf[4], 6);
			AddValue(pRs485MeterAddr, 6, PLC_645_MINUS_0X33);
			return OK;
		}
	}
	return ERROR;
}

unsigned long PLC_GetDIByDL645(P_GD_DL645_BODY pDl645)
{
	return PLC_GetDIByReducedDL645((P_GD_REDUCED_DL645_BODY)&pDl645->meter_number[0]);
}

unsigned long PLC_GetDIByReducedDL645(P_GD_REDUCED_DL645_BODY pDl645)
{
	unsigned long DI;

	DI = CCTT_INVALID_DL645_DI;

	switch (pDl645->controlCode & 0x1f)
	{
	case 0x01:
	case 0x02:
#if defined(APP_CJQ_ADD_BEFORE_CMD)
		if (0x08 == pDl645->dataLen)
		{
			memcpy_special(&DI, &pDl645->dataBuf[6], 2);
			AddValue(&DI, 2, PLC_645_MINUS_0X33);
		}
		else
#endif
		{
			memcpy_special(&DI, &pDl645->dataBuf[0], 2);
			AddValue(&DI, 2, PLC_645_MINUS_0X33);
			DI &= 0xffff;
		}
		break;
	case 0x03:
		memcpy_special(&DI, &pDl645->dataBuf[0], 4);
		AddValue(&DI, 4, PLC_645_MINUS_0X33);
		break;
	case 0x11:
	case 0x12:
		if ((4 == pDl645->dataLen) || (6 == pDl645->dataLen))
		{
			memcpy_special(&DI, &pDl645->dataBuf[0], 4);
			AddValue(&DI, 4, PLC_645_MINUS_0X33);
		}
#if defined(APP_CJQ_ADD_BEFORE_CMD)
		else if (0x0A == pDl645->dataLen)
		{
			memcpy_special(&DI, &pDl645->dataBuf[6], 4);
			AddValue(&DI, 4, PLC_645_MINUS_0X33);
		}
#endif
		else if ((pDl645->controlCode & 0x80) && (pDl645->dataLen >= 4))       //响应帧
		{
			memcpy_special(&DI, &pDl645->dataBuf[0], 4);
#if defined(APP_CJQ_ADD_BEFORE_CMD)
			USHORT meter_sn;
			meter_sn = PLC_get_sn_from_addr(pDl645->meter_number);
			if (meter_sn < CCTT_METER_NUMBER)
				if (1 == g_MeterRelayInfo[meter_sn].meter_type.cjq_with_add)
					memcpy_special(&DI, &pDl645->dataBuf[6], 4);
#endif

			AddValue(&DI, 4, PLC_645_MINUS_0X33);
		}
		break;
	default:
		break;
	}

	return DI;
}


void PLC_Check_cjq_with_add(USHORT read_sn, P_GD_DL645_BODY pDL645)
{
#if defined(APP_CJQ_ADD_BEFORE_CMD)

	if ((0x11 == pDL645->controlCode) || (0x12 == pDL645->controlCode))      //07
	{
		if (0x0A == pDL645->dataLen)
			g_MeterRelayInfo[read_sn].meter_type.cjq_with_add = 1;
		else if (0x04 == pDL645->dataLen)
			g_MeterRelayInfo[read_sn].meter_type.cjq_with_add = 0;
	}
	else if ((0x01 == pDL645->controlCode) || (0x02 == pDL645->controlCode))     //97
	{
		if (0x08 == pDL645->dataLen)
			g_MeterRelayInfo[read_sn].meter_type.cjq_with_add = 1;
		else if (0x02 == pDL645->dataLen)
			g_MeterRelayInfo[read_sn].meter_type.cjq_with_add = 0;
	}

#endif
}

u8 PLC_PostMessage(TMESSAGE message,  WPARAM wParam, LPARAM lParam)
{
	P_TMSG pMsg = NULL;
	OS_ERR err;

	pMsg = alloc_msg(__func__);
	if (NULL == pMsg)
		goto L_Error;

	pMsg->message = message;
	pMsg->wParam = wParam;
	pMsg->lParam = lParam;

	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return OK;

L_Error:
	free_msg(__func__,pMsg);

	return (!OK);
}

u8 PLC_PostDirRead(P_READ_MSG_INFO pReadMsg)
{
	if (OK != PLC_PostMessage(MSG_DIR_READ, 0, (LPARAM)pReadMsg))
		goto L_Error;

	return OK;

L_Error:
	free_buffer(__func__,pReadMsg);
	return ERROR;
}

u8 PLC_PostParallelRead(P_READ_MSG_INFO pReadMsg, u8 is_src_3762)
{
	P_TMSG pMsg = NULL;
	u8 res = RT_ERR_NONE;

	//错误检查
	OS_ERR err;

	OSSchedLock(&err);
	u8 qcount = get_queue_cnt(g_PlcWaitRxQueue);
	u8 counter = 0;

	while (qcount--)
	{
		P_READ_MSG_INFO pRead = (P_READ_MSG_INFO)dequeue(g_PlcWaitRxQueue);

		if (NULL == pRead)
			break;

#ifdef GDW_2019_GRAPH_STOREDATA
		if ((APP_PACKET_PARALLEL != pRead->header.packet_id) && (APP_PACKET_STORE_DATA_PARALLEL != pRead->header.packet_id))
#else
		if (APP_PACKET_PARALLEL != pRead->header.packet_id)
#endif			
			goto L_Next;

		if (OK == macCmp(pRead->header.mac, pReadMsg->header.mac))
		{
			res = RT_ERR_PARALLEL_STA_BUSY;
			enqueue(g_PlcWaitRxQueue, pRead);
			OSSchedUnlock(&err);
			goto L_Error;
		}
		counter++;
	L_Next:
		enqueue(g_PlcWaitRxQueue, pRead);
	}

	OSSchedUnlock(&err);

	if(is_src_3762) //1376.2本地接口调用PLC_PostParallelRead，如果队列不为空，则放入队列但不进行载波发送，由PLC_RemoveRxQueueOldItem处理
	{
#if defined(GDW_JILIN_SJ)	
		if (counter >= 21)
#else
		if (counter >= 25)
#endif
		{
			res = RT_ERR_PARALLEL_OVER_MAX_COO_PACKET;
			goto L_Error;
		}
		if(get_queue_cnt(g_PlcWaitRxQueue))
		{
			enqueue(g_PlcWaitRxQueue, pReadMsg);
			return RT_ERR_NONE;
		}
	}
	
	//并发抄表
	pMsg = alloc_msg(__func__);
	if (NULL == pMsg)
	{
		res = RT_ERR_BUSY_NOW;
		goto L_Error;
	}

	pMsg->message = MSG_PARALLEL_READ;
	pMsg->lParam = (LPARAM)pReadMsg;

	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
	{
		res = RT_ERR_BUSY_NOW;
		goto L_Error;
	}

	return RT_ERR_NONE;

L_Error:
	free_buffer(__func__,pReadMsg);
	free_msg(__func__,pMsg);
	return res;
}

u8 PLC_Broadcast(P_MSG_INFO pMsgIfo)
{

	P_TMSG pMsg = alloc_msg(__func__);

	if (NULL == pMsg)
		goto L_Error;

	if (pMsgIfo->msg_header.protocol==METER_PROTOCOL_3762_BROAD)
	{
		if(g_pRelayStatus->broadcast_cb.pSendMsgLower!=NULL)
		{
			goto L_Error;
		}
	}
	else
	{
		if(g_pRelayStatus->broadcast_cb.pSendMsgLower!=NULL
			&& g_pRelayStatus->broadcast_cb.pSendMsgHight!=NULL)
		{
			goto L_Error;
		}
	}

	pMsg->message = MSG_BROADCAST;
	pMsg->lParam = (LPARAM)pMsgIfo;

	OS_ERR err;
	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return OK;

L_Error:
	free_send_buffer(__func__,pMsgIfo);
	free_msg(__func__,pMsg);
	return ERROR;
}


u8 PLC_PhaseErrStart(u8 isForce)//是否强制刷新报文序列号
{

	P_TMSG pMsg = alloc_msg(__func__);

	if (NULL == pMsg)
		goto L_Error;


	pMsg->message = MSG_PHASE_ERR;
	pMsg->lParam = isForce;

	OS_ERR err;
	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return OK;

L_Error:
	free_msg(__func__,pMsg);
	return ERROR;
}

u8 PLC_CommTest(P_COMM_TEST_INFO pTestMsg)
{
	P_TMSG pMsg = alloc_msg(__func__);

	if (NULL == pMsg)
		goto L_Error;

	pMsg->message = MSG_HPLC_COMM_TEST;
	pMsg->lParam = (LPARAM)pTestMsg;

	OS_ERR err;
	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return OK;

L_Error:
	free_send_buffer(__func__,pTestMsg);
	free_msg(__func__,pMsg);
	return ERROR;
}

u8 PLC_InnerVer(u8* node_mac)
{
	P_TMSG pMsg = alloc_msg(__func__);

	if (NULL == pMsg)
		goto L_Error;

	pMsg->message = MSG_INNER_VER_READ;
	pMsg->lParam = (LPARAM)node_mac;

	OS_ERR err;
	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return OK;

L_Error:
	free_send_buffer(__func__,node_mac);
	free_msg(__func__,pMsg);
	return ERROR;
}

#ifdef HPLC_CSG    
u8 PLC_ResetSta(P_RESET_STA_INFO pResetSta)
{
	P_TMSG pMsg = alloc_msg(__func__);

	if (NULL == pMsg)
		goto L_Error;

	pMsg->message = MSG_RESET_STA;
	pMsg->lParam = (LPARAM)pResetSta;

	OS_ERR err;
	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return OK;

L_Error:
	free_send_buffer(__func__, pResetSta);
	free_msg(__func__,pMsg);
	return ERROR;
}
#endif

void PLC_broadcast_frame_proc(P_MSG_INFO  p_Msg)
{
}

void PLC_Reset_freezeflag()
{
	memset_special((UCHAR *)g_pRelayStatus->s_freeze_flag, 0x00, sizeof(g_pRelayStatus->s_freeze_flag));
}

//#ifdef PROTOCOL_GD_2016
#if 0
BYTE PLC_update()
{
//#define PLC_FILE_SEG_LEN 220
//#define PLC_FILE_SEG_LEN 198
#define PLC_FILE_SEG_LEN 180

#define PLC_FILE_RESEND_COUNT   1       //重发次数

#define PLC_FILE_SEND_INTERVEL_SEC   8       //广播发送间隔

	P_MSG_INFO pMsg;
	P_GD_DL645_BODY pDL645;

	if (0x55 == g_PlmStatusPara.file_update_flag)
	{
		P_FILE_TRANS_BEGIN_GD pFile = &g_PlmStatusPara.file_param_gd;

		memset_special(pFile, 0xff, sizeof(FILE_TRANS_BEGIN_GD));
		data_flash_read_straight(UPDATE_FIRMWARE_DB_ADDR, (u8 *)pFile, sizeof(FILE_TRANS_BEGIN_GD));
#if 0
		pFile->file_size = 500;
		pFile->file_property = FILE_PROPERTY_COLL_GD;
		pFile->file_id = 0x01;
		pFile->file_check_sum = 0x0000;
#endif

		if ((pFile->file_size >= 248 * 1024UL))
		{
			g_PlmStatusPara.file_update_flag = 0x00;
			return ERROR;
		}

		if ((FILE_PROPERTY_SLAVE_GD != pFile->file_property)
			&& (FILE_PROPERTY_COLL_GD != pFile->file_property))
		{
			g_PlmStatusPara.file_update_flag = 0x00;
			return ERROR;
		}

		memset_special(&pFile->des_addr[0], 0x99, sizeof(pFile->des_addr));
		pFile->file_seg_count = (unsigned short)(pFile->file_size / PLC_FILE_SEG_LEN);
		if (pFile->file_size % PLC_FILE_SEG_LEN) pFile->file_seg_count++;

		pMsg = alloc_send_buffer(__func__,MSG_SHORT);

		pDL645 = (P_GD_DL645_BODY)&pMsg->msg_buffer[0];

		pDL645->protocolHead = pDL645->protocolHead2 = FRAME_DL645_START_FLAG;
		memcpy_special(pDL645->meter_number, &pFile->des_addr[0], FRM_DL645_ADDRESS_LEN);
		pDL645->controlCode = pFile->file_property; //0x08;
		pDL645->dataLen = 0;

		*(ULONG *)&pDL645->dataBuf[pDL645->dataLen] = DI_TRANS_FILE_BEGIN_GD;
		pDL645->dataLen += 4;
		memcpy_special(&pDL645->dataBuf[pDL645->dataLen], &(pFile->file_id), sizeof(FILE_TRANS_BEGIN_GD) - 1);
		pDL645->dataLen += (sizeof(FILE_TRANS_BEGIN_GD) - 1);

		AddValue(pDL645->dataBuf, pDL645->dataLen, 0x33);

		pDL645->dataBuf[pDL645->dataLen] = Get_checksum(&pDL645->protocolHead, pDL645->dataLen + 10);
		pDL645->dataBuf[pDL645->dataLen + 1] = FRAME_DL645_END_FLAG;

		pMsg->msg_header.msg_len = pDL645->dataLen + 12;
		pMsg->msg_header.protocol = 0x02;

		g_PlmStatusPara.file_update_time_stamp = gettick();

		PLC_broadcast_frame_proc(pMsg);
		free_send_buffer(__func__,pMsg);

		g_PlmStatusPara.file_segid_gd = 0;
		g_PlmStatusPara.file_trans_counter = 0;
		g_PlmStatusPara.file_update_flag = 0x56;

		return OK;
	}
	else if (0x56 == g_PlmStatusPara.file_update_flag)
	{
		P_FILE_TRANS_BEGIN_GD pFile = &g_PlmStatusPara.file_param_gd;

		if ((g_PlmStatusPara.file_segid_gd >= pFile->file_seg_count)
			|| (pFile->file_size >= 248 * 1024UL))
		{
			g_PlmStatusPara.file_update_flag = 0x00;
			return ERROR;
		}

		if ((gettick() - g_PlmStatusPara.file_update_time_stamp) / OSCfg_TickRate_Hz >= PLC_FILE_SEND_INTERVEL_SEC)
		{
			//文件传输
			USHORT seg_len = 0, check_sum;
			ULONG file_addr;
			g_PlmStatusPara.file_update_time_stamp = gettick();

			pMsg = alloc_send_buffer(__func__,MSG_SHORT);
			pDL645 = (P_GD_DL645_BODY)&pMsg->msg_buffer[0];

			pDL645->protocolHead = pDL645->protocolHead2 = FRAME_DL645_START_FLAG;
			memcpy_special(pDL645->meter_number, &pFile->des_addr[0], FRM_DL645_ADDRESS_LEN);
			pDL645->controlCode = pFile->file_property; //0x08;
			pDL645->dataLen = 0;

			*(ULONG *)&pDL645->dataBuf[pDL645->dataLen] = DI_TRANS_FILE_CONTENT_GD;
			pDL645->dataLen += 4;
			*(USHORT *)&pDL645->dataBuf[pDL645->dataLen] = g_PlmStatusPara.file_segid_gd;
			pDL645->dataLen += 2;

			if (g_PlmStatusPara.file_segid_gd < (pFile->file_seg_count - 1)) seg_len = PLC_FILE_SEG_LEN;
			else seg_len = (unsigned short)(pFile->file_size - PLC_FILE_SEG_LEN * (pFile->file_seg_count - 1));
			*(USHORT *)&pDL645->dataBuf[pDL645->dataLen] = seg_len;
			pDL645->dataLen += 2;

			file_addr = UPDATE_FIRMWARE_DB_ADDR + sizeof(FILE_TRANS_BEGIN_GD) + PLC_FILE_SEG_LEN * g_PlmStatusPara.file_segid_gd;
			data_flash_read_straight(file_addr, (u8 *)&pDL645->dataBuf[pDL645->dataLen], seg_len);
			check_sum = get_crc_16(0, &pDL645->dataBuf[pDL645->dataLen], seg_len);
			//check_sum = PLC_Checksum(&pDL645->dataBuf[pDL645->dataLen], seg_len);

			pDL645->dataLen += (BYTE)seg_len;

			//校验   get_crc_16   PLC_Checksum
			*(USHORT *)&pDL645->dataBuf[pDL645->dataLen] = check_sum;
			pDL645->dataLen += 2;

			AddValue(pDL645->dataBuf, pDL645->dataLen, 0x33);

			pDL645->dataBuf[pDL645->dataLen] = Get_checksum(&pDL645->protocolHead, pDL645->dataLen + 10);
			pDL645->dataBuf[pDL645->dataLen + 1] = FRAME_DL645_END_FLAG;

			pMsg->msg_header.msg_len = pDL645->dataLen + 12;
			pMsg->msg_header.protocol = 0x02;

			PLC_broadcast_frame_proc(pMsg);
			free_send_buffer(__func__,pMsg);

			g_PlmStatusPara.file_segid_gd++;

			if (g_PlmStatusPara.file_segid_gd >= pFile->file_seg_count)
			{
				g_PlmStatusPara.file_update_flag = 0x57;
			}
		}

		return OK;
	}
	else if (0x57 == g_PlmStatusPara.file_update_flag)
	{
		//读取传输成功段数，失败的重传3次
		if ((gettick() - g_PlmStatusPara.file_update_time_stamp) / OSCfg_TickRate_Hz >= PLC_FILE_SEND_INTERVEL_SEC)
		{
			P_MSG_INFO pMsgResp;
			USHORT meter_sn, meter_counter, min_seg_count;
			P_FILE_TRANS_BEGIN_GD pFile = &g_PlmStatusPara.file_param_gd;
			unsigned short relay_meter[MAX_RELAY_DEEP + 1];
			BYTE level, rc, FrmHead1;
			BYTE dataUnit[5];

			g_PlmStatusPara.file_update_time_stamp = gettick();

			g_PlmStatusPara.file_trans_counter++;

			pMsg = alloc_send_buffer(__func__,MSG_SHORT);
			pDL645 = (P_GD_DL645_BODY)&pMsg->msg_buffer[0];

			meter_counter = 0;
			min_seg_count = 0;

			for (meter_sn = 0; meter_sn < CCTT_METER_NUMBER; meter_sn++)
			{
				if (CCO_USERD != g_MeterRelayInfo[meter_sn].status.used) continue;

				if (1 != g_MeterRelayInfo[meter_sn].status.plc) continue;

				*(ULONG *)&dataUnit[0] = DI_REQUEST_FILE_INFO_GD;
				PLC_make_frame_645(pDL645, g_MeterRelayInfo[meter_sn].meter_addr, 0x11, &dataUnit[0], 4);

				memset_special(&relay_meter[0], 0xff, sizeof(relay_meter));
				level = Relay_MF_GetDefRelayOfMeter(meter_sn, relay_meter);
				if (level >= MAX_RELAY_DEEP)
				{
					memset_special(&relay_meter[0], 0xff, sizeof(relay_meter));
					relay_meter[0] = meter_sn;
					level = 0;
				}

				g_mf_Dl645Msg.msg_header.time_related = FALSE;

				g_mf_Dl645Msg.msg_buffer[0] = 0x02;
				g_mf_Dl645Msg.msg_buffer[1] = pDL645->dataLen + 12;
				memcpy_special(&g_mf_Dl645Msg.msg_buffer[2], pDL645, pDL645->dataLen + 12);

				PLC_Post_Aframe(&g_mf_Dl645Msg, relay_meter, level, 0);

				pMsgResp = NULL;
				rc = (BYTE)PLC_Wait_frame(meter_sn, READ_ITEM_FINISH, &FrmHead1, &pMsgResp);

				if (OK == rc)
				{
					P_GD_REDUCED_DL645_BODY pDL645Resp;
					P_FILE_TRANS_INFO_COLL_GD pFileIfo;

					pDL645Resp = (P_GD_REDUCED_DL645_BODY)(pMsgResp->msg_buffer + pMsgResp->msg_header.pos_dl645_head);

					AddValue(pDL645Resp->dataBuf, pDL645Resp->dataLen, PLC_645_MINUS_0X33);

					if (4 + sizeof(FILE_TRANS_INFO_COLL_GD) == pDL645Resp->dataLen)
					{
						pFileIfo = (P_FILE_TRANS_INFO_COLL_GD)&pDL645Resp->dataBuf[4];

						if ((0 == min_seg_count) || (pFileIfo->seg_ok_count < min_seg_count)) min_seg_count = pFileIfo->seg_ok_count;
					}
					++meter_counter;
				}

				if (NULL != pMsgResp) free_send_buffer(__func__,pMsgResp);

				if (meter_counter >= 10) break;
			}

			if ((0 == min_seg_count)
				|| (pFile->file_seg_count == min_seg_count)
				|| (g_PlmStatusPara.file_trans_counter >= PLC_FILE_RESEND_COUNT))       //可以设定重发次数
			{
				//结束
				g_PlmStatusPara.file_update_flag = 0x00;
			}
			else
			{
				g_PlmStatusPara.file_update_flag = 0x56;
				g_PlmStatusPara.file_segid_gd = min_seg_count;
			}

			free_send_buffer(__func__,pMsg);
		}

		return OK;
	}

	return ERROR;
}

#endif

BYTE PLC_IsFreezeDI97(ULONG DI)
{
	if (CCTT_INVALID_DL645_DI == DI)
		return FALSE;

	return ((DI & 0xffff0000) == 0xffff0000);
}

ULONG PLC_GetRealFreezeDI(ULONG DI)
{
	if (PLC_IsFreezeDI97(DI))
		return (DI & 0xffff);
	else
		return DI;
}

BYTE PLC_IsFreezeDICfg()
{
#if 1
	BYTE i, check_one = FALSE;;
	ULONG sum = 0;

	for (i = 0; i < CCTT_FREEZE_DI_NUMBER; i++)
	{
		if ((g_PlmConfigPara.freeze_di[i] > 0) && (g_PlmConfigPara.freeze_di[i] < CCTT_INVALID_DL645_DI))
		{
			sum += g_PlmConfigPara.freeze_di[i];
			if (!check_one)
			{
				check_one = TRUE;
			}
		}
	}

	return (check_one && (sum == g_PlmConfigPara.freeze_di[CCTT_FREEZE_DI_NUMBER]));
#else
	return FALSE;
#endif
}

BYTE PLC_IsTheFreezeDICfg(ULONG DI)
{
#if 1
	BYTE i;

	if (0xffff == DI)
		return FALSE;

	if (CCTT_INVALID_DL645_DI == DI)
		return FALSE;

	if (!PLC_IsFreezeDICfg())
		return FALSE;

	for (i = 0; i < CCTT_FREEZE_DI_NUMBER; i++)
	{
		ULONG DI_cfg = PLC_GetRealFreezeDI(g_PlmConfigPara.freeze_di[i]);

		if (DI_cfg == DI)
			return TRUE;
	}

	return FALSE;

#else
	return FALSE;
#endif
}

//是否支持打包抄
BYTE PLC_IsPackReadValidable()
{
	return PLC_Is_Baud_SL();
}

BYTE PLC_GetReadPackCfgCount()
{
	BYTE c = 0;
	for (BYTE i = 0; i < countof(g_PlmConfigPara.pack_info); i++)
	{
		P_PACK_INFO p_pack_info = &g_PlmConfigPara.pack_info[i];

		if (TRUE == PLC_IsPackInfoValid(p_pack_info))
			c++;
	}

	return c;
}

BYTE PLC_IsPackInfoValid(P_PACK_INFO p_pack_info)
{
	BYTE i, read_pack_counter = 0;
	BYTE max_di_idx;
	USHORT dl645_len_total;

	dl645_len_total = 0;

	if (!PLC_IsPackReadValidable())
		return FALSE;

	/*
		switch(PLC_Get_Baud_Default())
		{
		//case PLC_BAUDRATE_4BPS:
		//case PLC_BAUDRATE_5BPS:
		case PLC_BAUDRATE_421_100BPS_SL:
		case PLC_BAUDRATE_421_1200BPS_SL:
			break;
		default:
			return (!OK);
		}
	*/
	max_di_idx = countof(DI_PACK) - 1;

	for (i = 0; i < countof(p_pack_info->DiGrp) - 1; i++)
	{
		if ((p_pack_info->DiGrp[i] > max_di_idx) || p_pack_info->DiGrpLen[i] > CCTT_READ_PACK_DATA_LEN)
		{
#if 1
			if (i < 2)     //配置项至少2个
				return FALSE;
			else
#endif
				break;
		}

		read_pack_counter++;
		dl645_len_total += p_pack_info->DiGrpLen[i];
	}

	if ((dl645_len_total + 1 + read_pack_counter) > CCTT_READ_PACK_DATA_LEN)
		return FALSE;

	if (dl645_len_total != p_pack_info->DiGrpLen[countof(p_pack_info->DiGrp) - 1])
		return FALSE;

	return TRUE;
}

void PLC_AutoCfgFreezeDI()
{
	BYTE i = 0;
	ULONG DI;
	ULONG sum_cal = 0;

	if (PLC_IsFreezeDICfg())
		return;

	memset_special(g_PlmConfigPara.freeze_di, 0xff, sizeof(g_PlmConfigPara.freeze_di));

#if defined(APP_CONCENTRATOR_MODE)

	DI = DI_ACT_POSI_ENER_LAST_DAY_BLK_07;
	sum_cal += DI;
	g_PlmConfigPara.freeze_di[i++] = DI;

	DI = DI_ACT_NEGA_ENER_LAST_DAY_BLK_07;
	sum_cal += DI;
	g_PlmConfigPara.freeze_di[i++] = DI;
	/*
	DI = DI_REA_POSI_ENER_LAST_DAY_BLK_07;
	sum_cal += DI;
	g_PlmConfigPara.freeze_di[i++] = DI;

	DI = DI_REA_NEGA_ENER_LAST_DAY_BLK_07;
	sum_cal += DI;
	g_PlmConfigPara.freeze_di[i++] = DI;
	*/
//#elif defined(PROTOCOL_GD_2016)
#else

	DI = DI_ACT_POSI_ENER_LAST_DAY_BLK_07;
	sum_cal += DI;
	g_PlmConfigPara.freeze_di[i++] = DI;

#endif

	//checksum
	g_PlmConfigPara.freeze_di[CCTT_FREEZE_DI_NUMBER] = sum_cal;
}

void PLC_AutoCfgPackInfo()
{
	BYTE i, len, max_pack_idx;
	P_PACK_INFO p_pack_info;

	for (i = 0; i < countof(g_PlmConfigPara.pack_info); i++)
	{
		p_pack_info = &g_PlmConfigPara.pack_info[i];

		if (TRUE == PLC_IsPackInfoValid(p_pack_info))
			return;
	}

	p_pack_info = &g_PlmConfigPara.pack_info[0];
	memset_special((PBYTE)p_pack_info, 0xff, sizeof(PACK_INFO));

	i = 0;
	len = 0;

#if defined(PROTOCOL_GD_2016)

	//日冻结时间
	p_pack_info->DiGrp[i] = 0;
	p_pack_info->DiGrpLen[i] = 5;
	len += p_pack_info->DiGrpLen[i++];
#endif

#if 1
	//上1次日冻结正向有功
	p_pack_info->DiGrp[i] = 1;
	p_pack_info->DiGrpLen[i] = 20;
	len += p_pack_info->DiGrpLen[i++];

	//上1次日冻结反向有功
	p_pack_info->DiGrp[i] = 7;
	p_pack_info->DiGrpLen[i] = 20;
	len += p_pack_info->DiGrpLen[i++];
#endif

	max_pack_idx = countof(p_pack_info->DiGrpLen) - 1;
	p_pack_info->DiGrpLen[max_pack_idx] = len;


}

BYTE PLC_freeze_update_meterdata(USHORT meter_sn, BYTE pack_flag, P_GD_REDUCED_DL645_BODY pDl645Resp)
{
	//只保存打包抄和配置的冻结项

	BYTE *pdata = pDl645Resp->dataBuf;
	BYTE len = pDl645Resp->dataLen;

	P_METER_DATA_STORAGE_BODY pMeterData, pMeterData_idx = NULL, pMeterData_empty = NULL;
	static USHORT idx = 0;
	USHORT counter = 0, bufferTotal = 0;

	if (meter_sn >= CCTT_METER_NUMBER)
		return (!OK);

	if (len > CCTT_READ_PACK_DATA_LEN)
		return (!OK);

	//如果是打包数据存储, 则校验打包个数
	BYTE cmp_len;
	if (pack_flag)
	{
		cmp_len = pdata[0] - 0x33 + 1;
		if (cmp_len > countof(g_PlmConfigPara.pack_info[0].DiGrp))
			return (!OK);
	}
	else
	{
#if 0
		return (!OK);
#else
		ULONG DI = PLC_GetDIByReducedDL645(pDl645Resp);
		if (!PLC_IsTheFreezeDICfg(DI))
			return (!OK);

		if (g_MeterRelayInfo[meter_sn].meter_type.cjq_with_add)
			return (!OK);

		if (METER_TYPE_PROTOCOL_97 == g_MeterRelayInfo[meter_sn].meter_type.protocol)
			cmp_len = 2;
		else if (METER_TYPE_PROTOCOL_07 == g_MeterRelayInfo[meter_sn].meter_type.protocol)
			cmp_len = 4;
		else
			return (!OK);
#endif
	}

	for (BYTE i = 0; i < countof(g_MeterDataStorages); i++)
	{
		P_METER_DATA_STORAGE pStorageInfo = &g_MeterDataStorages[i];

		bufferTotal += pStorageInfo->nBufferCount;

		for (USHORT c = 0; c < pStorageInfo->nBufferCount; c++)
		{
			pMeterData = &pStorageInfo->pBufferAddr[c];

			if (idx == counter++)
				pMeterData_idx = pMeterData;

			if ((NULL == pMeterData_empty)
				&& (pMeterData->meter_sn >= CCTT_METER_NUMBER || pMeterData->data_len > CCTT_READ_PACK_DATA_LEN))
				pMeterData_empty = pMeterData;

			if (meter_sn == pMeterData->meter_sn)
			{
				if (0 == memcmp(pdata, &pMeterData->data[0], cmp_len))
				{
					goto L_SAVE;
				}
			}
		}
	}

	if (pMeterData_empty)
	{
		pMeterData = pMeterData_empty;
		goto L_SAVE;
	}
	else
	{
		//在起始位置开始找块空间替换
		pMeterData = pMeterData_idx;
		if (++idx >= bufferTotal)
			idx = 0;
		goto L_SAVE;

		//return (!OK);
	}

L_SAVE:

	pMeterData->meter_sn = meter_sn;
	if (pack_flag)
		pMeterData->data_flag = 1;
	else
		pMeterData->data_flag = 0;
	pMeterData->data_len = len;
	memset_special(&pMeterData->data[0], 0xff, CCTT_READ_PACK_DATA_LEN);
	memcpy(&pMeterData->data[0], pdata, len);

	return OK;
}

BYTE PLC_IsAllFreezeDIDone(unsigned short meter_sn)
{
	BYTE freeze_flag[CCTT_FREEZE_DI_NUMBER];

	if (meter_sn >= CCTT_METER_NUMBER)
		return TRUE;

	if ((METER_TYPE_PROTOCOL_97 != g_MeterRelayInfo[meter_sn].meter_type.protocol)
		&& (METER_TYPE_PROTOCOL_07 != g_MeterRelayInfo[meter_sn].meter_type.protocol))
		return TRUE;

	for (BYTE i = 0; i < CCTT_FREEZE_DI_NUMBER; i++)
	{
		if (METER_TYPE_PROTOCOL_97 == g_MeterRelayInfo[meter_sn].meter_type.protocol)
		{
			if (PLC_IsFreezeDI97(g_PlmConfigPara.freeze_di[i]))
				freeze_flag[i] = FALSE;
			else
				freeze_flag[i] = TRUE;
		}
		else
		{
			if (PLC_IsFreezeDI97(g_PlmConfigPara.freeze_di[i]))
				freeze_flag[i] = TRUE;
			else
				freeze_flag[i] = FALSE;
		}
	}

	for (BYTE i = 0; i < countof(g_MeterDataStorages); i++)
	{
		P_METER_DATA_STORAGE pStorageInfo = &g_MeterDataStorages[i];

		for (USHORT c = 0; c < pStorageInfo->nBufferCount; c++)
		{
			ULONG DI_resp;
			P_METER_DATA_STORAGE_BODY pMeterData = &pStorageInfo->pBufferAddr[c];

			if (meter_sn != pMeterData->meter_sn)
				continue;

			if (0 != pMeterData->data_flag)
				continue;

			memcpy_special(&DI_resp, pMeterData->data, 4);
			AddValue(&DI_resp, 4, PLC_645_MINUS_0X33);

			if (METER_TYPE_PROTOCOL_97 == g_MeterRelayInfo[meter_sn].meter_type.protocol)
				DI_resp &= 0xffff;

			for (BYTE j = 0; j < CCTT_FREEZE_DI_NUMBER; j++)
			{
				if (DI_resp == PLC_GetRealFreezeDI(g_PlmConfigPara.freeze_di[j]))
				{
					freeze_flag[j] = TRUE;
					break;
				}
			}
		}
	}

	for (BYTE i = 0; i < CCTT_FREEZE_DI_NUMBER; i++)
	{
		if (!freeze_flag[i])
			return FALSE;
	}

	return TRUE;
}

//根据表号获取存储 DL645 数据
P_METER_DATA PLC_GetMeterDataByMetersn(unsigned short meter_sn)
{
	P_METER_DATA_STORAGE_BODY pMeterData;
	BYTE pack_idx = 0, pack_count, data_c = 0;
	BYTE count_storages = countof(g_MeterDataStorages);
	BYTE count_di_max = countof(g_PlmConfigPara.pack_info[0].DiGrp);
	BYTE count_di_pack = countof(DI_PACK);

	memset_special(&g_Meter_Data, 0xff, sizeof(g_Meter_Data));

	g_Meter_Data.meter_sn = meter_sn;

	pack_count = PLC_GetReadPackCfgCount();

	for (BYTE i = 0; i < count_storages; i++)
	{
		P_METER_DATA_STORAGE pStorageInfo = &g_MeterDataStorages[i];

		for (USHORT c = 0; c < pStorageInfo->nBufferCount; c++)
		{
			pMeterData = &pStorageInfo->pBufferAddr[c];

			if (meter_sn == pMeterData->meter_sn && pMeterData->data_len <= CCTT_READ_PACK_DATA_LEN)
			{
				if (1 == pMeterData->data_flag)
				{
					BYTE di_count_645 = pMeterData->data[0] - 0x33;
					BYTE p;

					if (di_count_645 > (count_di_max - 1))
						continue;

					p = (di_count_645 + 1);

					for (BYTE j = 0; j < di_count_645; j++)
					{
						BYTE di_idx_645 = pMeterData->data[j + 1] - 0x33;
						BYTE pack_info_idx;
						P_PACK_INFO p_pack_info;
						BYTE len;

						if (di_idx_645 >= count_di_pack)
							break;

						p_pack_info = PLC_GetPackInfoByDIIdx(di_idx_645);
						if (NULL == p_pack_info)
							break;

						pack_info_idx = PLC_GetPackInfoIdx(p_pack_info, di_idx_645);
						if (pack_info_idx >= 0xff)
							break;

						len = p_pack_info->DiGrpLen[pack_info_idx];
						if ((p + len) > pMeterData->data_len)
							break;

						g_Meter_Data.meter_data[data_c].data_len = len + 4;
						memcpy_special(&g_Meter_Data.meter_data[data_c].data[0], &DI_PACK[di_idx_645], 4);
						AddValue(&g_Meter_Data.meter_data[data_c].data[0], 4, 0x33);
						memcpy_special(&g_Meter_Data.meter_data[data_c].data[4], &pMeterData->data[p], len);
						data_c++;

						p += p_pack_info->DiGrpLen[pack_info_idx];
					}

					pack_idx++;
					if (pack_idx >= pack_count)
						return &g_Meter_Data;
				}
				else
				{
					g_Meter_Data.meter_data[data_c].data_len = pMeterData->data_len;
					memcpy_special(g_Meter_Data.meter_data[data_c].data, pMeterData->data, pMeterData->data_len);
					data_c++;
				}

				if (data_c >= CCTT_METER_DATA_NUMBER)
					return &g_Meter_Data;
			}
		}
	}

	if (data_c > 0)
		return &g_Meter_Data;
	else
		return NULL;
}

//输入: 根据DL645标识符
//输出: 应答的DL645的数据域部分，以及长度
BYTE PLC_GetMeterDataByDI(USHORT meter_sn, ULONG DI, BYTE **ppDataUnit, BYTE *pLen)
{
	BYTE i;
	P_METER_DATA p_meter_data;

	if (meter_sn >= CCTT_METER_NUMBER)
		return (!OK);

	p_meter_data = PLC_GetMeterDataByMetersn(meter_sn);

	if (NULL == p_meter_data)
		return (!OK);

	for (i = 0; i < CCTT_METER_DATA_NUMBER; i++)
	{
		ULONG DI_buffer;
		if (p_meter_data->meter_data[i].data_len > CCTT_READ_PACK_DATA_LEN)
			continue;

		memcpy_special(&DI_buffer, p_meter_data->meter_data[i].data, 4);
		AddValue(&DI_buffer, 4, PLC_645_MINUS_0X33);
		if (METER_TYPE_PROTOCOL_97 == g_MeterRelayInfo[meter_sn].meter_type.protocol)
			DI_buffer &= 0xffff;

		if (DI == DI_buffer)
		{
			if (NULL != pLen)
				*pLen = p_meter_data->meter_data[i].data_len;
			if (NULL != ppDataUnit)
				*ppDataUnit = p_meter_data->meter_data[i].data;
			return OK;
		}
	}

	return (!OK);
}

//输入: 根据DL645读命令帧
//输出: 应答的DL645的数据域部分，以及长度
BYTE PLC_GetMeterDataByDL645(const P_GD_DL645_BODY pDL645Cmd, BYTE **ppDataUnit, BYTE *pLen)
{
	unsigned short meter_sn;
	ULONG DI;

	DI = PLC_GetDIByDL645(pDL645Cmd);
	if (CCTT_INVALID_DL645_DI == DI)
		return (!OK);

	meter_sn = PLC_get_sn_from_addr(pDL645Cmd->meter_number);
	if (meter_sn >= CCTT_METER_NUMBER)
		return (!OK);

	return PLC_GetMeterDataByDI(meter_sn, DI, ppDataUnit, pLen);
}

BYTE PLC_GetMeterDataByDIIdx(unsigned short meter_sn, BYTE di_idx, BYTE **ppDataUnit, BYTE *pLen)
{
	ULONG DI;

	if (di_idx >= countof(DI_PACK))
		return (!OK);

	DI = DI_PACK[di_idx];

	return PLC_GetMeterDataByDI(meter_sn, DI, ppDataUnit, pLen);
}

BYTE PLC_DelMeterData()
{
	for (BYTE i = 0; i < countof(g_MeterDataStorages); i++)
	{
		P_METER_DATA_STORAGE pStorageInfo = &g_MeterDataStorages[i];
		if (pStorageInfo->nBufferCount > 0)
			memset_special(pStorageInfo->pBufferAddr, 0xff, pStorageInfo->nBufferCount * sizeof(METER_DATA_STORAGE_BODY));
	}

	return OK;
}

BYTE PLC_ClearCacheData()
{
	memset_special(&g_PlmStatusPara.meter_freeze_time_lst1[0], 0xff, sizeof(g_PlmStatusPara.meter_freeze_time_lst1));
	memset_special(&g_PlmStatusPara.meter_freeze_time_lst2[0], 0xff, sizeof(g_PlmStatusPara.meter_freeze_time_lst2));
	g_PlmStatusPara.meter_freeze_time_lst1_flag = FALSE;
	g_PlmStatusPara.meter_freeze_time_lst2_flag = FALSE;
	PLC_DelMeterData();

	return OK;
}

BYTE PLC_DelMeterDataByMetersn(unsigned short meter_sn)
{
	P_METER_DATA_STORAGE_BODY pMeterData;

	for (BYTE i = 0; i < countof(g_MeterDataStorages); i++)
	{
		P_METER_DATA_STORAGE pStorageInfo = &g_MeterDataStorages[i];

		for (USHORT c = 0; c < pStorageInfo->nBufferCount; c++)
		{
			pMeterData = &pStorageInfo->pBufferAddr[c];

			if (meter_sn == pMeterData->meter_sn)
			{
				memset_special(pMeterData, 0xff, sizeof(METER_DATA_STORAGE_BODY));
			}
		}
	}

	return OK;
}

P_PACK_INFO PLC_GetPackInfoByDIIdx(BYTE di_idx)
{
	BYTE i;
	BYTE pack_info_idx;
	P_PACK_INFO p_pack_info;

	for (i = 0; i < CCTT_READ_PACK_NUMBER; i++)
	{
		p_pack_info = &g_PlmConfigPara.pack_info[i];

		if (TRUE == PLC_IsPackInfoValid(p_pack_info))
		{
			pack_info_idx = PLC_GetPackInfoIdx(p_pack_info, di_idx);
			if (0xff != pack_info_idx)
			{
				return p_pack_info;
			}
		}
	}

	return NULL;
}

BYTE PLC_GetDIIdxByDI(ULONG DI, BYTE *di_idx)
{
	BYTE i;
	BYTE di_count = countof(DI_PACK);

	for (i = 0; i < di_count; i++)
	{
		if (DI == DI_PACK[i])
		{
			*di_idx = i;
			return OK;
		}
	}

	return (!OK);
}

BYTE PLC_GetDIIdxByDL645(P_GD_DL645_BODY pDL645, BYTE *di_idx)
{
	unsigned long DI;

	if (0x11 != pDL645->controlCode)
		return (!OK);

	if (0x04 != pDL645->dataLen)
		return (!OK);

	DI = PLC_GetDIByDL645(pDL645);

	return PLC_GetDIIdxByDI(DI, di_idx);
}

BYTE PLC_GetPackInfoIdx(const P_PACK_INFO p_pack_info, BYTE di_idx)
{
	BYTE i;
	BYTE di_count = countof(p_pack_info->DiGrp);

	for (i = 0; i < di_count - 1; i++)
	{
		if (di_idx == p_pack_info->DiGrp[i])
			return i;
	}

	return 0xff;
}

#if 0

void flash_test()
{
	//保存档案路由测试
	PLC_save_relay_info(NULL_PTR, NULL_PTR);
	OSTimeDly(OS_TICKS_PER_SEC);

	//整芯片测试
	unsigned long cnt = 800;
	unsigned long my_addr;
	unsigned long ii;

	for(my_addr=0; my_addr<0x200000-cnt; )
	{
		for(ii=0; ii<cnt; ii++)
		{
			g_relay_stack0s[ii] = (BYTE)(my_addr+ii);
		}
		data_flash_write_straight(my_addr, (ULONG)g_relay_stack0s, cnt);
		memset_special(g_relay_stack0s, 0xff, cnt);
		data_flash_read_straight(my_addr, (ULONG)g_relay_stack0s, cnt);
		for(ii=0; ii<cnt; ii++)
		{
			if(g_relay_stack0s[ii] != (BYTE)(my_addr+ii))
			{
				OSTimeDly(1);
			}
		}
		my_addr += cnt;
		cnt = rand_special() % 4096;
		OSTimeDly(1);
	}

}

#endif

void PLC_ChangeNetIDCallback(u32 new_net_id)
{
	if (g_pPlmConfigPara->net_id != new_net_id)
	{
		g_pPlmConfigPara->new_nid = new_net_id;
		g_pPlmConfigPara->isNeedChangeNid=1;
	}
}

void PLC_BeaconFrameCallback(const P_HPLC_PARAM pHplcParam, P_BEACON_ZONE pBeaconZone, P_BEACON_MPDU pBeaconPayload)
{
#if 0
	if(IsCommTestMode(TEST_MODE_TRAN_MPDU_TO_COMM, NULL) || IsCommTestMode(TEST_MODE_PASS_MPDU_BACK, NULL) || IsCommTestMode(TEST_MODE_TRAN_MAC_TO_COMM, NULL))
	//if(IsCommTestMode(TEST_MODE_PASS_MPDU_BACK, NULL))
	HPLC_TestModeUpdateSlot(sync_ntb, pBeaconZone->NTB, pBeaconPayload);
#endif
	PLC_HandleBeaconFrame(pHplcParam, pBeaconZone, pBeaconPayload);
}

void PLC_MacFrameCallback(const P_HPLC_PARAM pHplcParam, P_MAC_FRAME pMacFrame,SOF_PARAM *sofParam)
{
	P_TMSG pMsg = NULL;
	P_RECEIVE_MAC pReceiveMac = NULL;
	P_MAC_FRAME pMac = NULL;

//    PLC_HandleMacFrame(pMacFrame);

	USHORT mac_len = GetMacFrameLen(pMacFrame);

	pReceiveMac = (P_RECEIVE_MAC)alloc_buffer(__func__,sizeof(RECEIVE_MAC) + mac_len);
	if (NULL == pReceiveMac)
		return;

	memcpy_special(&pReceiveMac->hplcParam, pHplcParam, sizeof(HPLC_PARAM));

	pMac = (P_MAC_FRAME)&pReceiveMac->macFrame[0];

	memcpy_special(pMac, pMacFrame, mac_len);

	pMsg = alloc_msg(__func__);
	if (NULL == pMsg)
		goto L_Error;

	if (pMacFrame->head.Version == HRF_ONE_HOP_VERSION)
	{
		pMsg->message = MSG_ONE_HOP_FRAME;
	}
	else
	{
		pMsg->message = MSG_MAC_FRAME;
	}
	pMsg->lParam = (LPARAM)pReceiveMac;
	pMsg->wParam = sofParam->SrcTei << 16 | sofParam->DstTei;
	OS_ERR err;
	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return;

L_Error:
	free_buffer(__func__,pReceiveMac);
	free_msg(__func__,pMsg);
}

void PLC_ParamReset()
{
	P_TMSG pMsg = NULL;

	pMsg = alloc_msg(__func__);
	if (NULL == pMsg)
		goto L_Error;

	pMsg->message = MSG_PARAM_CLEAR;

	OS_ERR err;
	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return;

L_Error:
	free_msg(__func__,pMsg);
}

void PLC_TaskReset(void)
{
	P_TMSG pMsg = NULL;

	pMsg = alloc_msg(__func__);
	if (NULL == pMsg)
		goto L_Error;

	pMsg->message = MSG_TASK_CLEAR;

	OS_ERR err;
	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return;

L_Error:
	free_msg(__func__,pMsg);
}


void PLC_DataReset()
{
	P_TMSG pMsg = NULL;

	pMsg = alloc_msg(__func__);
	if (NULL == pMsg)
		goto L_Error;

	pMsg->message = MSG_DATA_CLEAR;

	OS_ERR err;
	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return;

L_Error:
	free_msg(__func__,pMsg);
}

void PLC_TriggerSaveNodeInfo()
{
	g_pRelayStatus->save_nodes_sec_down = 5;
}

#ifdef GDW_ZHEJIANG
void PLC_TriggerSaveBlackNodeInfo()
{
	g_pRelayStatus->save_black_nodes_sec_down = 5;
}
#endif //GDW_ZHEJIANG

void PLC_TriggerSaveOutListMeterInfo()
{
	g_pRelayStatus->save_OutListMeter_sec_down = 5;
}

void PLC_TriggerSaveStaInfo()
{
//    g_pRelayStatus->save_sta_sec_down = 5;
}


//bit_start from 0
//0   11
//11  11    11 / 8 = 1 ,  11 % 8 = 3
//22  11    22 / 8 = 2 ,  22 % 8 = 6
ULONG PLC_PutTeiBit(USHORT tei, PBYTE pData, ULONG bit_start, u8 bit_len)
{
	USHORT byte_pos = (bit_start >> 3);
	u8 bit_pos = (bit_start & 0x07);

	for (u8 i = 0; i < bit_len; i++)
	{
		pData[byte_pos] |= ((tei >> i & 0x01) << bit_pos);
		bit_pos++;

		if (bit_pos == 8)
		{
			byte_pos++;
			bit_pos = 0;
		}
	}

	return (bit_start + bit_len);
}

ULONG PLC_GetTeiBit(USHORT *p_tei, PBYTE pData, ULONG bit_start, u8 bit_len)
{
	USHORT byte_pos = (bit_start >> 3);
	u8 bit_pos = (bit_start & 0x07);

	*p_tei = 0;

	for (u8 i = 0; i < bit_len; i++)
	{
		(*p_tei) |= (((u16)(pData[byte_pos] >> bit_pos & 0x01)) << i);
		bit_pos++;

		if (bit_pos == 8)
		{
			byte_pos++;
			bit_pos = 0;
		}
	}

	return (bit_start + bit_len);
}








HPLC_PHASE PLC_GetHplcPhaseByReport(u32 phase_report)
{
	HPLC_PHASE phase;

#if defined(HPLC_CSG)
	phase = (HPLC_PHASE)(phase_report & 0x03);
#else
	phase = (HPLC_PHASE)(phase_report & 0x03);
#endif

	if ((phase < PHASE_A) || (phase > PHASE_C))
		phase = PHASE_DEFAULT;

	return phase;
}

HPLC_PHASE PLC_GetSendPhase(P_STA_INFO pSta)
{
	HPLC_PHASE send_phase;

#if defined(COMM_USE_PHY_PHASE)

	if (0 != (pSta->phy_phase&0x3f))
		send_phase = PLC_PhyPhase_to_HplcPhase(pSta->phy_phase);
	else
	{
	#ifdef  UNKONW_PHASE_USE_RECV_PHASE
		send_phase = (HPLC_PHASE)pSta->hplc_phase;
	#else
		send_phase = PHASE_ALL;
	#endif
	}

#elif defined(COMM_USE_RECEIVE_PHASE)

	if (pSta->receive_phase >= PHASE_A && pSta->receive_phase <= PHASE_C)
		send_phase = pSta->receive_phase;
	else
		send_phase = pSta->hplc_phase;

#else

	send_phase = pSta->hplc_phase;

#endif

	if ((send_phase < PHASE_A) || (send_phase > PHASE_ALL))
		send_phase = PHASE_ALL;

	return send_phase;
}
int PLC_GetAppDstMac(u8 *meter,u8 *mac)//返回
{
	if (memIsHex(meter, 0xff, LONG_ADDR_LEN)
		||memIsHex(meter, 0x99, LONG_ADDR_LEN)
		||memIsHex(meter, 0xaa, LONG_ADDR_LEN))
	{
		memcpy(mac,meter,LONG_ADDR_LEN);
		return 0;
	}
	//if(PLC_GetStaInfoByMac(meter)==NULL)//表未入网 或表挂在二采模块下
	{
		u16 sn=PLC_get_sn_from_addr(meter);
		if (sn<CCTT_METER_NUMBER)
		{
			if (g_MeterRelayInfo[sn].coll_sn < TEI_CNT && g_MeterRelayInfo[sn].coll_sn >= TEI_STA_MIN)
			{
				P_STA_INFO p_staInfo=PLC_GetValidStaInfoByTei(g_MeterRelayInfo[sn].coll_sn);
				if (p_staInfo)
				{
					memcpy(mac, p_staInfo->mac, LONG_ADDR_LEN);
					return p_staInfo->ReaderSec;
				}
				
			}
		}

	}
	memcpy(mac,meter,LONG_ADDR_LEN);
	return MAX_READ_TIME;

}
void PLC_GetSendParam(u16 tei, u16 *p_des_tei, HPLC_SEND_PHASE *p_send_phase,u8* link)
{
	*p_des_tei = tei;
	*p_send_phase = SEND_PHASE_ALL;
	if (tei==READER_TEI)
	{
		*p_des_tei=READER_TEI;
		*link=g_pRelayStatus->ReaderLink==0?FreamPathBplc:FreamPathHrf;
		return;
	}
	u16 top_pco = PLC_GetTopPco(tei);

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(top_pco);
	if (pSta == NULL || pSta->pco != CCO_TEI)
	{
		*p_des_tei = TEI_BROADCAST;
		*p_send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
		*link=FreamPathBoth;
	}
	else
	{
		*p_des_tei = top_pco;
		*p_send_phase = (HPLC_SEND_PHASE)(1 << PLC_GetSendPhase(pSta));
		if (*p_send_phase == SEND_PHASE_ALL)
		{
			*p_send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
		}
		*link = pSta->Link?FreamPathHrf:FreamPathBplc;
	}

//    *p_send_phase = PHASE_A;        //for test

#if 0
	pSta = PLC_GetValidStaInfoByTei(tei);
	if(NULL != pSta)
	*p_send_phase = pSta->hplc_phase;
#endif
}
bool IsOrphanNode(u16 tei)
{
	u16 top_pco = PLC_GetTopPco(tei);
	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(top_pco);
	if (pSta == NULL || pSta->pco != CCO_TEI)
	{
		return true;
	}
	return false;
}



unsigned char PLC_SendReadAck(u8 *pdata, u16 len)
{

	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu = NULL;
	u16 app_len, msdu_len = 0;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_READ_METER;

	mac_param.DstTei = READER_TEI;
	mac_param.SendLimit = 3;
	mac_param.HopCount = 0;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

#if defined(HPLC_CSG)
	mac_param.Broadcast = BROADCAST_DRT_DOWN;
	mac_param.HeadType = MAC_HEAD_SHORT;
	mac_param.Vlan = VLAN_MM;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
#else
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.mac_flag = MACFLAG_NONE;
#endif

	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);

	pAppFrame = HplcApp_MakeReadFrame(++g_pRelayStatus->packet_seq_read_meter, (u8 *)pdata, len, &app_len);
	if (NULL == pAppFrame)
		goto L_Error;
#ifdef HPLC_CSG
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, app_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	if (NULL == pMsdu)
		goto L_Error;
#else
	pMsdu=(u8*)pAppFrame;
	msdu_len=app_len;
#endif
	SOF_param.DstTei = READER_TEI;
	SOF_param.BroadcastFlag = 0;
	mac_param.SendType = SENDTYPE_SINGLE;

	send_param.send_phase = SEND_PHASE_ALL;
	send_param.link = ((g_pRelayStatus->ReaderLink == 0) ? FreamPathBplc : FreamPathHrf);
	if (ReaderPhase>=PHASE_A&&ReaderPhase<=PHASE_C)
	{
		send_param.send_phase = (HPLC_SEND_PHASE)(1<<ReaderPhase);
		ReaderPhase=0;
	}
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);


	free_buffer(__func__,pMsdu);

	return OK;

L_Error:
	free_buffer(__func__,pMsdu);
	return (!OK);
}
#if defined(HPLC_CSG)

//关联指示
void PLC_AssocIndOK(ASSOC_REQ_PACKET *pAssocReq, USHORT tei, u8 assoc_res)
{
	u8 *pMsdu = NULL;
	P_STA_BITMAP pStaBimap = NULL;
	ROUTE_MSG_FILED *pRouteHead = NULL;
	P_STA_INFO pStaInfo = PLC_GetValidStaInfoByTei(tei);
	if (NULL == pStaInfo)
		return;

	pMsdu = (u8 *)alloc_buffer(__func__,ASSOC_CNF_MAC_FRAME);
	if (NULL == pMsdu)
		return;

	memset_special(pMsdu, 0, ASSOC_CNF_MAC_FRAME);

	P_MM_FRAME pMmFrame = (P_MM_FRAME)pMsdu;

	pMmFrame->head.MMVersion = 1;   //pAssocReq->MngMsgVersion;
	pMmFrame->head.MMType = MME_ASSOC_IND;

	P_ASSOC_IND_PACKET pAssocInd = (P_ASSOC_IND_PACKET)pMmFrame->body;

	//直连站点
	pStaBimap = PLC_GetDirectSubStas(tei, false);
	if (NULL == pStaBimap)
		goto PLC_AssocIndOK_Exit;

	pAssocInd->Result = assoc_res;
	pAssocInd->Level = PLC_GetStaLevel(tei);
	memcpy_special(pAssocInd->STAMacAddr, pAssocReq->MacAddr, sizeof(pAssocInd->STAMacAddr));
	PLC_CopyCCOMacAddr(pAssocInd->CCOMacAddr, TRUE);

	pAssocInd->STATei = tei;
	pAssocInd->PCOTei = pStaInfo->pco;
	pAssocInd->LinkType = pStaInfo->Link;
	pAssocInd->StationRandom = pAssocReq->STARandom;
	pAssocInd->NetSeq = g_pPlmConfigPara->net_seq;
	pAssocInd->ReAssocTime = 0;
	pAssocInd->P2PSeq = pAssocReq->P2PSeq;
	pAssocInd->Band =  g_pPlmConfigPara->hplc_comm_param;
	pRouteHead = (ROUTE_MSG_FILED *)&pAssocInd->RouteData;
	pRouteHead->Reserve1 = 0;

	writeSendRoutTable(pMmFrame, false, pRouteHead, pStaBimap, tei, (u16 *)&pMsdu[ASSOC_CNF_MSDU_LEN - 4 - 1]);


	g_pRelayStatus->selfreg_cb.selfreg_new_sta_tick = GetTickCount64();

PLC_AssocIndOK_Exit:
	free_buffer(__func__,pMsdu);
	free_buffer(__func__,pStaBimap);
}

void PLC_AssocIndError(MAC_HEAD *pMacHead, ASSOC_REQ_PACKET *pAssocReq, ASSOC_RESULT assoc_res)
{
	u8 *pMsdu = (u8 *)alloc_buffer(__func__,MAX_MAC_FRAME_LEN);
	if (NULL == pMsdu)
		return;

	memset_special(pMsdu, 0, MAX_MAC_FRAME_LEN);

	u16 tei_pco = pAssocReq->CandidataPCO[0].TEI;

	P_MM_FRAME pMmFrame = (P_MM_FRAME)pMsdu;

	pMmFrame->head.MMVersion = pAssocReq->MngMsgVersion;
	pMmFrame->head.MMType = MME_ASSOC_IND;

	P_ASSOC_IND_PACKET pAssocInd = (P_ASSOC_IND_PACKET)pMmFrame->body;

	pAssocInd->Result = (u8)assoc_res;
	pAssocInd->Level = PLC_GetStaLevel(tei_pco) + 1;
	memcpy_special(pAssocInd->STAMacAddr, pAssocReq->MacAddr, sizeof(pAssocInd->STAMacAddr));
	PLC_CopyCCOMacAddr(pAssocInd->CCOMacAddr, TRUE);
	pAssocInd->STATei = TEI_INVALID;
	pAssocInd->PCOTei = tei_pco;
	pAssocInd->PacketSeq = 1;          //分包序号
	pAssocInd->TotalPacket = 1;        //总分包数
	pAssocInd->LastPacketFlag = 1;
	pAssocInd->StationRandom = pAssocReq->STARandom;
	pAssocInd->NetSeq = g_pPlmConfigPara->net_seq;
	if (HPLC_FindOneNeighborNidNum())
	{
		pAssocInd->ReAssocTime = CON_REASSOC_MUTI_TIME_MS;        //重新关联时间, ms
	}
	else
	{
		pAssocInd->ReAssocTime = CON_REASSOC_TIME_MS;        //重新关联时间, ms
	}
	pAssocInd->P2PSeq = pAssocReq->P2PSeq;
	ROUTE_MSG_FILED *pRouteHead = (ROUTE_MSG_FILED *)&pAssocInd->RouteData;
	pRouteHead->DirectSTANum = 0;       //直连站点数
	pRouteHead->DirectPCONum = 0;       //直连代理数
	pRouteHead->RouteTableSize = 0;     //路由信息表大小
	pRouteHead->Reserve1 = 0;

	//发送
	u16 msdu_len = sizeof(MM_HEAD) + (sizeof(ASSOC_IND_PACKET) - 1) + (sizeof(ROUTE_MSG_FILED) - 1);

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 1;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_ASSOC_IND;

	mac_param.HeadType = MAC_HEAD_LONG;
	mac_param.Vlan = VLAN_MM;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;

//    if(TEI_CCO==tei_pco)
	{
		//广播
		send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);


		SOF_param.DstTei = TEI_BROADCAST;
		SOF_param.BroadcastFlag = 1;

		mac_param.DstTei = TEI_BROADCAST;
		mac_param.SendType = SENDTYPE_LOCAL;
		mac_param.SendLimit = 3;
		mac_param.HopCount = MAX_RELAY_DEEP;
		mac_param.MsduType = MSDU_NET_MANAGER;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		mac_param.NetSeq = g_pPlmConfigPara->net_seq;
		send_param.link = FreamPathBoth;
		//mac_param.mac_flag = MACFLAG_INCLUDE;
		PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
		memcpy_swap(mac_param.DstMac, pAssocReq->MacAddr, MAC_ADDR_LEN);
	}
	/*
	else
	{
		//单播
		P_STA_INFO pStaPco = PLC_GetValidStaInfoByTei(tei_pco);
		if(NULL == pStaPco)
			goto L_Exit;

		PLC_GetSendParam(tei_pco, &SOF_param.DstTei, &send_param.send_phase);
		SOF_param.BroadcastFlag = 1;

		mac_param.DstTei = TEI_BROADCAST;     //关联指示报文的MAC帧的目的TEI必须填写目的站点的TEI
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.SendLimit = 5;
		mac_param.HopCount = 0;
		mac_param.MsduType = MSDU_NET_MANAGER;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		mac_param.NetSeq = g_pPlmConfigPara->net_seq;
		//mac_param.mac_flag = MACFLAG_INCLUDE;
		PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);

		pStaPco = PLC_GetValidStaInfoByTei(SOF_param.DstTei);
		if(NULL != pStaPco)
			memcpy_special(mac_param.DstMac, pStaPco->mac, MAC_ADDR_LEN);
		else
			goto L_Exit;
	}
*/
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, pMsdu, msdu_len);

	free_buffer(__func__,pMsdu);
}

#if defined(HPLC_CSG)
unsigned char PLC_SendDataTrans(BOOL broadcast,u8 meter_moudle, HPLC_SEND_PHASE phase, u16 seq,u8 mac_addr[MAC_ADDR_LEN], u8 *pdata, u16 len)
{

	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu = NULL;
	u16 app_len, msdu_len = 0;
	u8 datmac[LONG_ADDR_LEN];
	PLC_GetAppDstMac(mac_addr,datmac);
	USHORT tei = PLC_GetStaSnByMac(datmac);
	USHORT sendtei;
#ifdef GRAPH_COLLECT_REWRITE_DEVTYPE
	u8 devtype = 0xFF;
#endif

#if 1
    debug_str(DEBUG_LOG_NET, "PLC_SendDataTrans, tei:%d, bcast:%d, module:%d\r\n",
	    tei, broadcast, meter_moudle);
	if (tei >= TEI_CNT)
	{
		tei = TEI_BROADCAST;
		sendtei = tei;
	}
	else
	{
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
		if (NULL == pSta)
			goto L_Error;

#ifdef GRAPH_COLLECT_REWRITE_DEVTYPE
		if (pSta->dev_type == METER)
		{
			devtype = 0;
		}
		else if (pSta->dev_type == METER_3PHASE)
		{
			devtype = 1;
		}
#endif

		if ((NET_IN_NET != pSta->net_state))
		{
              debug_str(DEBUG_LOG_NET, "PLC_SendDataTrans not NET_IN_NET, tei:%d, bcast:%d, module:%d\r\n",
                tei, broadcast, meter_moudle);
			//sendtei = TEI_BROADCAST;
          	goto L_Error;
		}
		else
		{
			sendtei = tei;
		}
		if (
			//#ifndef NW_TEST
			//(NET_IN_NET != pSta->net_state) || 
			//#endif
			broadcast)
			tei = TEI_BROADCAST;
	}
#else
	if(tei >= TEI_CNT)
	goto L_Error;

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
	if(NULL == pSta)
	goto L_Error;

	if((NET_IN_NET != pSta->net_state) || pReadMsg->header.broadcast)
	tei = TEI_BROADCAST;
#endif

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 3;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_READ_METER;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;

	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, datmac, MAC_ADDR_LEN);

#ifdef GRAPH_COLLECT_REWRITE_DEVTYPE
	if ((meter_moudle == 1) && (devtype != 0xFF))
	{
		if ((pdata[0] == APP_GRAPH_FUNC_CODE_READ_21PROTOCOL))
		{
			pdata[1] = devtype;
		}
	}
#endif

#ifdef COLLECT_READ_USE_METER_MAC
	if (meter_moudle == 0) //数据转发到电表
	{
		memcpy_special(datmac, mac_addr, MAC_ADDR_LEN); //采集器使用下挂电表地址；模块datmac和mac_addr值相同
	}
#endif

	pAppFrame = HplcApp_MakeDataTransFrame(g_pPlmConfigPara->main_node_addr, datmac, APP_SHENHUA_APP_CODE_GRAPH, meter_moudle, seq, EM_BUSINESS_NEED_RESPONSE, (u8 *)pdata, len, &app_len);
	if (NULL == pAppFrame)
		goto L_Error;

	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, app_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	if (NULL == pMsdu)
		goto L_Error;
	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);//预处理是否是广播
	send_param.doubleSend=1;
	if (TEI_BROADCAST == SOF_param.DstTei)
	{
		send_param.resend_count = 3;
		send_param.send_type = CSMA_SEND_TYPE_CSMA;
		send_param.cb = NULL;

//        SOF_param.DstTei = tei;
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		PLC_GetSendParam(sendtei, &sendtei, &send_param.send_phase,&send_param.link);

		if (phase != SEND_PHASE_UNKNOW)
		{
			send_param.send_phase = phase;
		}

		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}
	else
	{
		PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		mac_param.HopCount = 0;

		if (phase != SEND_PHASE_UNKNOW)
		{
			send_param.send_phase = phase;
		}
		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}

	free_buffer(__func__,pMsdu);

	return OK;

L_Error:
	free_buffer(__func__,pMsdu);
	return (!OK);
}
#endif

#ifdef PLC_X4_FUNC

//发送采集任务初始化
unsigned char PLC_X4SendReadTaskInit()
{
	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu = NULL;
	u16 app_len, msdu_len = 0;

	USHORT tei = TEI_BROADCAST;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_X4_READ_TASK_INIT;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 5;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_X4_READ_TASK_INIT;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;

	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, BroadCastMacAddr, MAC_ADDR_LEN);

	pAppFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NORESPONSE, EM_CMD_BUSINESS_READ_TASK_INIT, NULL, 0, &app_len);
	if (NULL == pAppFrame)
		goto L_Error;

	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, app_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	if (NULL == pMsdu)
		goto L_Error;

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.DstTei = tei;
	SOF_param.BroadcastFlag = 1;
	mac_param.SendType = SENDTYPE_PCOBROADCAST;
	mac_param.Broadcast = BROADCAST_DRT_DOWN;

	send_param.link = FreamPathBoth;
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);

	free_buffer(__func__,pMsdu);

	return OK;

L_Error:
	free_buffer(__func__,pMsdu);
	return (!OK);
}

unsigned char PLC_X4SendReadTaskID(u16 tei)
{
	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu = NULL;
	u16 app_len, msdu_len = 0;

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
	if (NULL == pSta)
		goto L_Error;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_X4_READ_TASK_ID;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_X4_READ_TASK_ID;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;

	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, pSta->mac, MAC_ADDR_LEN);

	pAppFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NEED_RESPONSE, EM_CMD_BUSINESS_READ_TASK_ID, NULL, 0, &app_len);
	if (NULL == pAppFrame)
		goto L_Error;

	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, app_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	if (NULL == pMsdu)
		goto L_Error;
	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
#if 1
	if (TEI_BROADCAST == tei)
	{
		send_param.resend_count = 2;
		send_param.send_type = CSMA_SEND_TYPE_CSMA;
		send_param.cb = NULL;
		SOF_param.DstTei = tei;
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_LOCAL;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;

		send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}
	else
#endif
	{
		PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		mac_param.HopCount = 0;

		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}

	free_buffer(__func__,pMsdu);

	return OK;

L_Error:
	free_buffer(__func__,pMsdu);
	return (!OK);
}

unsigned char PLC_X4SendReadTaskDetail(u16 tei, u8 task_id)
{
	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu = NULL;
	u16 app_len, msdu_len = 0;

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
	if (NULL == pSta)
		goto L_Error;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_X4_READ_TASK_DETAIL;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_X4_READ_TASK_DETAIL;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;

	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, pSta->mac, MAC_ADDR_LEN);

	pAppFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NEED_RESPONSE, EM_CMD_BUSINESS_READ_TASK_DETAIL, (u8 *)&task_id, 1, &app_len);
	if (NULL == pAppFrame)
		goto L_Error;

	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, app_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	if (NULL == pMsdu)
		goto L_Error;
	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
#if 1
	if (TEI_BROADCAST == SOF_param.DstTei)
	{
		send_param.resend_count = 2;
		send_param.send_type = CSMA_SEND_TYPE_CSMA;
		send_param.cb = NULL;
//        SOF_param.DstTei = tei;
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;

		send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}
	else
#endif
	{
		PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		mac_param.HopCount = 0;

		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}

	free_buffer(__func__,pMsdu);

	return OK;

L_Error:
	free_buffer(__func__,pMsdu);
	return (!OK);
}


//发送添加采集任务
unsigned char PLC_X4SendAddReadTask(u16 tei, P_X4_READ_TASK_INFO p_read_task)
{
	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu = NULL;
	u8 *pAppData = NULL;

	u16 app_len, msdu_len = 0, app_data_len = 0;

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
	if (NULL == pSta)
		goto L_Error;

	app_data_len = PLC_X4GetReadTaskInfoLen(p_read_task) + 12;
	pAppData = alloc_buffer(__func__,app_data_len);
	if (NULL == pAppData)
		goto L_Error;

	memset_special(pAppData, 0, 6);             //任务下发时间，待填充
	local_tm now;
	if (PLC_get_system_time(&now))
	{
		P_DATA_FORMAT_01 p_now = (P_DATA_FORMAT_01)pAppData;
		p_now->second = Hex2BcdChar(now.tm_sec);
		p_now->minute = Hex2BcdChar(now.tm_min);
		p_now->hour = Hex2BcdChar(now.tm_hour);
		p_now->day = Hex2BcdChar(now.tm_mday);
		p_now->month = Hex2BcdChar(now.tm_mon+1);
		p_now->year = Hex2BcdChar(now.tm_year % 100);
		AddValue(p_now, 6, PLC_645_MINUS_0X33);
	}
	PLC_CopyCCOMacAddr(pAppData + 6, FALSE);
	memcpy_special(pAppData + 12, p_read_task, app_data_len - 12);

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_X4_ADD_READ_TASK;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_X4_ADD_READ_TASK;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;

	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, pSta->mac, MAC_ADDR_LEN);

	pAppFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NEED_RESPONSE, EM_CMD_BUSINESS_ADD_READ_TASK, (u8 *)pAppData, app_data_len, &app_len);
	if (NULL == pAppFrame)
		goto L_Error;

	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, app_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	if (NULL == pMsdu)
		goto L_Error;
	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
#if 1
	if (TEI_BROADCAST == SOF_param.DstTei)
	{
		send_param.resend_count = 2;
//        send_param.slot_type = SLOT_TYPE_CSMA_ANY;
		SOF_param.DstTei = tei;
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;

//        send_param.send_phase = PHASE_ALL;
		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}
	else
#endif
	{
//        PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase);
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		mac_param.HopCount = 0;

		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}

	free_buffer(__func__,pAppData);
	free_buffer(__func__,pMsdu);

	return OK;

L_Error:
	free_buffer(__func__,pAppData);
	free_buffer(__func__,pMsdu);
	return (!OK);
}

unsigned char PLC_X4SendDeleteReadTask(u16 tei, u8 task_id)
{
	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu = NULL;
	u16 app_len, msdu_len = 0;

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
	if (NULL == pSta)
		goto L_Error;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_X4_DELETE_READ_TASK;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_X4_DELETE_READ_TASK;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;

	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, pSta->mac, MAC_ADDR_LEN);

	pAppFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NEED_RESPONSE, EM_CMD_BUSINESS_DELETE_READ_TASK, (u8 *)&task_id, 1, &app_len);
	if (NULL == pAppFrame)
		goto L_Error;

	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, app_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	if (NULL == pMsdu)
		goto L_Error;
	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
#if 1
	if (TEI_BROADCAST == SOF_param.DstTei)
	{
		send_param.resend_count = 2;
		send_param.send_type = CSMA_SEND_TYPE_CSMA;
		send_param.cb = NULL;
//        SOF_param.DstTei = tei;
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;

		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}
	else
#endif
	{
		PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		mac_param.HopCount = 0;

		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}

	free_buffer(__func__,pMsdu);

	return OK;

L_Error:
	free_buffer(__func__,pMsdu);
	return (!OK);
}

unsigned char PLC_X4SendReadCollTaskData(BOOL broadcast, HPLC_SEND_PHASE phase, u8 mac_addr[MAC_ADDR_LEN], u8 *pdata, u16 len)
{
	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu = NULL;
	u8 *pAppData = NULL;
	u16 app_len, msdu_len = 0, app_data_len = 0;

	USHORT tei = PLC_GetStaSnByMac(mac_addr);
	USHORT sendtei;
#if 1
	if (tei >= TEI_CNT)
	{
		tei = TEI_BROADCAST;
		sendtei = tei;
	}
	else
	{
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
		if (NULL == pSta)
		{
			goto L_Error;
		}
		if ((NET_IN_NET != pSta->net_state))
		{
			sendtei = TEI_BROADCAST;
		}
		else
		{
			sendtei = tei;
		}
		if ((NET_IN_NET != pSta->net_state) || broadcast)
		{
			tei = TEI_BROADCAST;
		}
	}
#else
	if(tei >= TEI_CNT)
	goto L_Error;

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
	if(NULL == pSta)
	goto L_Error;

	if((NET_IN_NET != pSta->net_state) || pReadMsg->header.broadcast)
	tei = TEI_BROADCAST;
#endif

	app_data_len = 6 + len;
	pAppData = alloc_buffer(__func__,app_data_len);
	if (NULL == pAppData)
		goto L_Error;

	PLC_CopyCCOMacAddr(pAppData, FALSE);
	memcpy_special(pAppData + 6, pdata, len);

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_X4_READ_COLL_TASK_DATA;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_X4_READ_COLL_TASK_DATA;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;

	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, mac_addr, MAC_ADDR_LEN);

	pAppFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NEED_RESPONSE, EM_CMD_BUSINESS_READ_TASK_DATA, (u8 *)pAppData, app_data_len, &app_len);
	if (NULL == pAppFrame)
		goto L_Error;

	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, app_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	if (NULL == pMsdu)
		goto L_Error;
	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	if (TEI_BROADCAST == SOF_param.DstTei)
	{
		send_param.resend_count = 2;
		send_param.send_type = CSMA_SEND_TYPE_CSMA;
		send_param.cb = NULL;
//        SOF_param.DstTei = tei;
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		PLC_GetSendParam(sendtei, &sendtei, &send_param.send_phase,&send_param.link);
		if (phase != SEND_PHASE_UNKNOW)
		{
			send_param.send_phase = phase;
		}
		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}
	else
	{
		PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		mac_param.HopCount = 0;
		if (phase != SEND_PHASE_UNKNOW)
		{
			send_param.send_phase = phase;
		}
		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}

	free_buffer(__func__,pAppData);
	free_buffer(__func__,pMsdu);

	return OK;

L_Error:
	free_buffer(__func__,pAppData);
	free_buffer(__func__,pMsdu);
	return (!OK);
}

u8 PLC_X4CollTaskInitProc(OS_TICK_64 tick_now)
{
	P_STA_PLC_READ_TASK_INIT_CB pcb = &g_pRelayStatus->sta_read_task_init_cb;

	if (EM_STA_READ_TASK_INIT == pcb->state)
	{
		if (pcb->tick_start > 0)
		{
			pcb->state = EM_STA_READ_TASK_SEND;
			return (!OK);
		}
		return OK;
	}
	else if (EM_STA_READ_TASK_SEND == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->tick_last_snd) < 3 * OSCfg_TickRate_Hz)
			return OK;

		if (TicksBetween64(tick_now, pcb->tick_start) >= 10 * OSCfg_TickRate_Hz)
		{
			pcb->tick_start = 0;
			pcb->state = EM_STA_READ_TASK_INIT;
			return OK;
		}

		PLC_X4SendReadTaskInit();
		pcb->tick_last_snd = tick_now;

		return (OK);
	}

	return OK;
}
//采集任务同步
u8 PLC_X4ReadTaskSyncProc(OS_TICK_64 tick_now)
{
	P_PLC_X4_READ_TASK_SYNC_CB pcb = &g_pRelayStatus->x4_read_task_sync_cb;

	if (EM_X4_READ_TASK_SYNC_INIT == pcb->state)
	{
		//还需要考虑STA不支持X4，如何结束同步过程，特别是STA都不应答的时候

		if (TicksBetween64(tick_now, pcb->tick_last_find) < 5 * OSCfg_TickRate_Hz)
			return OK;

		u16 tei = pcb->tei_sync;

		for (u16 i = TEI_STA_MIN; i <= TEI_STA_MAX; i++)
		{
			tei++;
			if (tei < TEI_STA_MIN || tei > TEI_STA_MAX)
				tei = TEI_STA_MIN;

			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
			if (NULL == pSta)
				continue;

			P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
			if (pStaStatus->x4_sync_complete)
				continue;

			if (TicksBetween64(tick_now, pStaStatus->tick_innet) <= 13 * OSCfg_TickRate_Hz)
				continue;

			pcb->tei_sync = tei;
			pcb->task_idx = 0;
			pcb->task_read_num = 0;
			pcb->task_delete_num = 0;
			pcb->task_add_num = 0;
			pcb->state = EM_X4_READ_TASK_SYNC_READ_TASK_ID_SEND;

			break;
		}

		pcb->tick_last_find = tick_now;

		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_READ_TASK_ID_SEND == pcb->state)
	{
		if (OK == PLC_X4SendReadTaskID(pcb->tei_sync))
		{
			pcb->app_frame_seq_task_id_read = g_pRelayStatus->packet_seq_read_meter;
			pcb->tick_send = tick_now;
			pcb->tick_timeout = PLC_GetReadMeterTimeoutTick(0);
			pcb->state = EM_X4_READ_TASK_SYNC_READ_TASK_ID_WAIT_PLC;
			return (!OK);
		}
		else
		{
			pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
		}
		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_READ_TASK_ID_WAIT_PLC == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->tick_send) >= pcb->tick_timeout)
		{
			pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
			return OK;
		}
		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_READ_TASK_ID_RECEIVE_PLC == pcb->state)
	{
		if (pcb->task_read_num == 0)
		{
			pcb->state = EM_X4_READ_TASK_SYNC_COMMPARE;
		}
		else
		{
			pcb->task_idx = 0;
		}
		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_READ_TASK_DETAIL_SEND == pcb->state)
	{
		if (OK == PLC_X4SendReadTaskDetail(pcb->tei_sync, pcb->task_id_read_array[pcb->task_idx]))
		{
			pcb->app_frame_seq_task_info_read = g_pRelayStatus->packet_seq_read_meter;
			pcb->tick_send = tick_now;
			pcb->tick_timeout = PLC_GetReadMeterTimeoutTick(0);
			pcb->state = EM_X4_READ_TASK_SYNC_READ_TASK_DETAIL_WAIT_PLC;
			return (!OK);
		}
		else
		{
			pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
		}
		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_READ_TASK_DETAIL_WAIT_PLC == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->tick_send) >= pcb->tick_timeout)
		{
			pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
			return OK;
		}
		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_READ_TASK_DETAIL_RECEIVE_PLC == pcb->state)
	{
		pcb->task_idx++;
		if (pcb->task_idx >= pcb->task_read_num)
		{
			pcb->task_idx = 0;
			pcb->state = EM_X4_READ_TASK_SYNC_COMMPARE;
		}
		else
		{
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pcb->tei_sync);
			if (NULL == pSta)
				pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
			else
				pcb->state = EM_X4_READ_TASK_SYNC_READ_TASK_DETAIL_SEND;
		}
		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_COMMPARE == pcb->state)
	{
		//比较任务
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pcb->tei_sync);
		if (NULL == pSta)
		{
			pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
			return OK;
		}
		u16 node_sn = PLC_get_sn_from_addr(pSta->mac);
		if (node_sn >= CCTT_METER_NUMBER)
		{
			pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
			return OK;
		}
		P_RELAY_INFO pNodeInfo = &g_MeterRelayInfo[node_sn];
#ifdef PROTOCOL_GD_2016
		//删除多余的或不一样的
		for (u8 i = 0; i < pcb->task_read_num; i++)
		{
			P_X4_READ_TASK_INFO pTaskSta = pcb->task_info_read_array[i];
			if (NULL == pTaskSta)
				continue;

			BOOL found = FALSE;
			if (pNodeInfo->task_id_num <= CCTT_X4_READ_TASK_NUM_PER_METER)
			{
				for (u8 j = 0; j < pNodeInfo->task_id_num; j++)
				{
					P_X4_READ_TASK_INFO pReadTask = PLC_X4FindReadTaskInfo(pNodeInfo->task_id_array[j]);
					if (NULL == pReadTask)
						continue;

					if (OK == PLC_X4ReadTaskIsEqual(pReadTask, pTaskSta))
					{
						found = TRUE;
						break;
					}
				}
			}
			if (found)
				continue;
			pcb->task_id_delete_array[pcb->task_delete_num++] = pTaskSta->task_id;
		}

		//下发没有的
		if (pNodeInfo->task_id_num <= CCTT_X4_READ_TASK_NUM_PER_METER)
		{
			for (u8 i = 0; i < pNodeInfo->task_id_num; i++)
			{
				P_X4_READ_TASK_INFO pReadTask = PLC_X4FindReadTaskInfo(pNodeInfo->task_id_array[i]);
				if (NULL == pReadTask)
					continue;

				BOOL found = FALSE;
				for (u8 j = 0; j < pcb->task_read_num; j++)
				{
					P_X4_READ_TASK_INFO pTaskSta = pcb->task_info_read_array[i];
					if (NULL == pTaskSta)
						continue;
					if (OK == PLC_X4ReadTaskIsEqual(pReadTask, pTaskSta))
					{
						found = TRUE;
						break;
					}
				}
				if (found)
					continue;
				pcb->task_id_add_array[pcb->task_add_num++] = pReadTask->task_id;
			}
		}
#endif
		pcb->task_idx = 0;

		if (pcb->task_delete_num)
			pcb->state = EM_X4_READ_TASK_SYNC_DELETE_TASK_SEND;
		else if (pcb->task_add_num)
			pcb->state = EM_X4_READ_TASK_SYNC_ADD_TASK_SEND;
		else
			pcb->state = EM_X4_READ_TASK_SYNC_COMPLETE;

		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_DELETE_TASK_SEND == pcb->state)
	{
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pcb->tei_sync);
		if (NULL == pSta)
		{
			pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
			return OK;
		}
		if (OK == PLC_X4SendDeleteReadTask(pcb->tei_sync, pcb->task_id_delete_array[pcb->task_idx]))
		{
			pcb->app_frame_seq_task_delete = g_pRelayStatus->packet_seq_read_meter;
			pcb->tick_send = tick_now;
			pcb->tick_timeout = PLC_GetReadMeterTimeoutTick(0);
			pcb->state = EM_X4_READ_TASK_SYNC_DELETE_TASK_WAIT_PLC;
			return (!OK);
		}
		else
		{
			pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
		}
		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_DELETE_TASK_WAIT_PLC == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->tick_send) >= pcb->tick_timeout)
		{
			pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
			return OK;
		}
		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_DELETE_TASK_RECEIVE_PLC == pcb->state)
	{
		pcb->task_idx++;
		if (pcb->task_idx >= pcb->task_delete_num)
		{
			pcb->task_idx = 0;
			if (pcb->task_add_num)
				pcb->state = EM_X4_READ_TASK_SYNC_ADD_TASK_SEND;
			else
				pcb->state = EM_X4_READ_TASK_SYNC_COMPLETE;
		}
		else
		{
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pcb->tei_sync);
			if (NULL == pSta)
				pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
			else
				pcb->state = EM_X4_READ_TASK_SYNC_DELETE_TASK_SEND;
		}
		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_ADD_TASK_SEND == pcb->state)
	{
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pcb->tei_sync);
		if (NULL == pSta)
		{
			pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
			return OK;
		}
		P_X4_READ_TASK_INFO pReadTask = PLC_X4FindReadTaskInfo(pcb->task_id_add_array[pcb->task_idx]);
		if ((NULL != pReadTask) && (OK == PLC_X4SendAddReadTask(pcb->tei_sync, pReadTask)))
		{
			pcb->app_frame_seq_task_add = g_pRelayStatus->packet_seq_read_meter;
			pcb->tick_send = tick_now;
			pcb->tick_timeout = PLC_GetReadMeterTimeoutTick(0);
			pcb->state = EM_X4_READ_TASK_SYNC_ADD_TASK_WAIT_PLC;
			return (!OK);
		}
		else
		{
			pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
		}
		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_ADD_TASK_WAIT_PLC == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->tick_send) >= pcb->tick_timeout)
		{
			pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
			return OK;
		}
		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_ADD_TASK_RECEIVE_PLC == pcb->state)
	{
		pcb->task_idx++;
		if (pcb->task_idx >= pcb->task_add_num)
		{
			pcb->task_idx = 0;
			pcb->state = EM_X4_READ_TASK_SYNC_COMPLETE;
		}
		else
		{
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pcb->tei_sync);
			if (NULL == pSta)
				pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
			else
				pcb->state = EM_X4_READ_TASK_SYNC_ADD_TASK_SEND;
		}
		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_COMPLETE == pcb->state)
	{
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pcb->tei_sync);
		if (NULL == pSta)
		{
			pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
			return OK;
		}
		P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(pcb->tei_sync);
		pStaStatus->x4_sync_complete = 1;

		//上报同步完成

		pcb->state = EM_X4_READ_TASK_SYNC_EXIT;

		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_NOT_SUPPORT == pcb->state)
	{
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pcb->tei_sync);
		if (NULL == pSta)
		{
			pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
			return OK;
		}
		P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(pcb->tei_sync);
		pStaStatus->x4_sync_complete = 1;

		//上报不支持

		pcb->state = EM_X4_READ_TASK_SYNC_EXIT;
		return OK;
	}
	else if (EM_X4_READ_TASK_SYNC_EXIT == pcb->state)
	{
		u8 c = countof(pcb->task_info_read_array);
		for (u8 i = 0; i < c; i++)
		{
			if (NULL != pcb->task_info_read_array[i])
			{
				free_buffer(__func__,pcb->task_info_read_array[i]);
				pcb->task_info_read_array[i] = NULL;
			}
		}
		pcb->state = EM_X4_READ_TASK_SYNC_INIT;
		return OK;
	}

	return OK;
}
#endif
#endif


#if defined(HPLC_CSG)
unsigned char PLC_SendPreciseTime(BOOL broadcast,u8 meter_moudle, HPLC_SEND_PHASE phase, u16 seq,u8 mac_addr[MAC_ADDR_LEN], u8 *pdata, u16 len)
{

	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu = NULL;
	u16 app_len, msdu_len = 0;

	USHORT tei = PLC_GetStaSnByMac(mac_addr);
	USHORT sendtei;
#if 1
	if (tei >= TEI_CNT)
	{
		return !OK;
	}
	else
	{
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
		if (NULL == pSta)
			return !OK;
		if ((NET_IN_NET != pSta->net_state))
		{
			sendtei = TEI_BROADCAST;
			return !OK;
		}
		else
		{
			sendtei = tei;
		}
		if ((NET_IN_NET != pSta->net_state) || broadcast)
		{
			tei = TEI_BROADCAST;
			return !OK;
		}
	}
#else
	if(tei >= TEI_CNT)
	goto L_Error;

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
	if(NULL == pSta)
	goto L_Error;

	if((NET_IN_NET != pSta->net_state) || pReadMsg->header.broadcast)
	tei = TEI_BROADCAST;
#endif

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 3;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_READ_METER;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;

	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, mac_addr, MAC_ADDR_LEN);

	APP_DATA_BROAD_TIME broad_time;
	local_tm broad_tm;
	memset(&broad_tm,0,sizeof(broad_tm));
	broad_tm.tm_sec = Bcd2HexChar(pdata[0]);
	broad_tm.tm_min = Bcd2HexChar(pdata[1]);
	broad_tm.tm_hour = Bcd2HexChar(pdata[2]);
	broad_tm.tm_mday = Bcd2HexChar(pdata[3]);
	broad_tm.tm_mon = Bcd2HexChar(pdata[4])-1;
	broad_tm.tm_year = Bcd2HexChar(pdata[5]);
	broad_tm.tm_year =broad_tm.tm_year + 2000 - 1900;
	__time64_t real_time = mktime(&broad_tm);
	OS_TICK_64 thoughTime=*(u32*)&pdata[len-sizeof(u32)];
	thoughTime<<=32;
	thoughTime|=*(u32*)&pdata[len-sizeof(OS_TICK_64)];
	thoughTime=GetTickCount64()-thoughTime;
	thoughTime/=OS_CFG_TICK_RATE_HZ;
	real_time+=thoughTime;
	broad_tm = *gmtime(&real_time);
	broad_time.bcd_year = Hex2BcdChar((broad_tm.tm_year + 1900) % 100);
	broad_time.bcd_mon = Hex2BcdChar(broad_tm.tm_mon + 1);
	broad_time.bcd_day = Hex2BcdChar(broad_tm.tm_mday);
	broad_time.bcd_hour = Hex2BcdChar(broad_tm.tm_hour);
	broad_time.bcd_min = Hex2BcdChar(broad_tm.tm_min);
	broad_time.bcd_sec = Hex2BcdChar(broad_tm.tm_sec);
	broad_time.ntb = BPLC_GetNTB();

	pAppFrame = HplcApp_MakeDataTransFrame(g_pPlmConfigPara->main_node_addr, mac_addr, APP_SHENHUA_APP_CODE_TIMING, meter_moudle, ++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NORESPONSE, (u8 *)&broad_time, sizeof(broad_time), &app_len);


	if (NULL == pAppFrame)
		goto L_Error;

	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, app_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	if (NULL == pMsdu)
		goto L_Error;
	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);//预处理是否是广播
	send_param.doubleSend=1;
	if (TEI_BROADCAST == SOF_param.DstTei)
	{
		send_param.resend_count = 3;
		send_param.send_type = CSMA_SEND_TYPE_CSMA;
		send_param.cb = NULL;

//        SOF_param.DstTei = tei;
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		PLC_GetSendParam(sendtei, &sendtei, &send_param.send_phase,&send_param.link);
		if (phase != SEND_PHASE_UNKNOW)
		{
			send_param.send_phase = phase;
		}
		if (phase != SEND_PHASE_UNKNOW)
		{
			send_param.send_phase = phase;
		}
		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}
	else
	{
		PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		mac_param.HopCount = 0;

		if (phase != SEND_PHASE_UNKNOW)
		{
			send_param.send_phase = phase;
		}
		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}

	free_buffer(__func__,pMsdu);

	return OK;

L_Error:
	free_buffer(__func__,pMsdu);
	return (!OK);
}
unsigned char PLC_SendNewPreciseTime(BOOL broadcast,u8 meter_moudle, HPLC_SEND_PHASE phase, u16 seq,u8 mac_addr[MAC_ADDR_LEN], u8 *pdata, u16 len)
{

	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu = NULL;
	u16 app_len, msdu_len = 0;

	USHORT tei = PLC_GetStaSnByMac(mac_addr);
	USHORT sendtei;
#if 1
	if (tei >= TEI_CNT)
	{
		return (!OK);
	}
	else
	{
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
		if (NULL == pSta)
			return !OK;
		if ((NET_IN_NET != pSta->net_state))
		{
			sendtei = TEI_BROADCAST;
			return !OK;
		}
		else
		{
			sendtei = tei;
		}
		if ((NET_IN_NET != pSta->net_state) || broadcast)
		{
			tei = TEI_BROADCAST;
			return !OK;
		}
	}
#else
	if(tei >= TEI_CNT)
	goto L_Error;

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
	if(NULL == pSta)
	goto L_Error;

	if((NET_IN_NET != pSta->net_state) || pReadMsg->header.broadcast)
	tei = TEI_BROADCAST;
#endif

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 3;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_READ_METER;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;

	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, mac_addr, MAC_ADDR_LEN);

	APP_BROADCST_TIME_EXT *data = (APP_BROADCST_TIME_EXT *)alloc_buffer(__func__, sizeof(APP_BROADCST_TIME_EXT) + len);
	OS_TICK_64 thoughTime=0;
	if (data == NULL)
	{
		goto L_Error;
	}
	data->Port = 0x1;
	data->Seq = seq;
	data->Reserve = 0;
	thoughTime=*(u32*)&pdata[len-sizeof(u32)];
	thoughTime<<=32;
	thoughTime|=*(u32*)&pdata[len-sizeof(OS_TICK_64)];

	data->NTB = thoughTime;
	memcpy(data->data,pdata, len-sizeof(OS_TICK_64));
	pAppFrame = HplcApp_MakeDataTransFrame(g_pPlmConfigPara->main_node_addr,mac_addr,  APP_SHENHUA_APP_CODE_TIMING, 2, ++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NORESPONSE,(u8*)data, sizeof(APP_BROADCST_TIME_EXT)+len-sizeof(OS_TICK_64), &app_len);
	free_buffer(__func__,data);

	if (NULL == pAppFrame)
		goto L_Error;

	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, app_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	if (NULL == pMsdu)
		goto L_Error;
	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link); //预处理是否是广播
	send_param.doubleSend=1;
	if (TEI_BROADCAST == SOF_param.DstTei)
	{
		send_param.resend_count = 3;
		send_param.send_type = CSMA_SEND_TYPE_CSMA;
		send_param.cb = NULL;

//        SOF_param.DstTei = tei;
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		PLC_GetSendParam(sendtei, &sendtei, &send_param.send_phase,&send_param.link);
		if (phase != SEND_PHASE_UNKNOW)
		{
			send_param.send_phase = phase;
		}
		if (phase != SEND_PHASE_UNKNOW)
		{
			send_param.send_phase = phase;
		}
		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}
	else
	{
		PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		mac_param.HopCount = 0;

		if (phase != SEND_PHASE_UNKNOW)
		{
			send_param.send_phase = phase;
		}
		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}

	free_buffer(__func__,pMsdu);

	return OK;

L_Error:
	free_buffer(__func__,pMsdu);
	return (!OK);
}
#endif



bool PLC_AssocReqCheckAuth(u8* data)
{
#ifdef HPLC_CSG
	return true;
#else
	if (!data)
	{
		return true;
	}

#ifdef ASSOC_AUTH_CRC16
	ASSOC_REQ_PACKET *assocReq = (ASSOC_REQ_PACKET *)data;
	u16 crc = 0;

	//CRC大端传输
	crc = (assocReq->FactoryMSG[16]<<8) + assocReq->FactoryMSG[17];

	if(GetCrc16(data, (u8 *)&(assocReq->FactoryMSG[0]) - data) != crc) //站点MAC地址-->站点关联随机数
	{
        return false;
	}
#endif

	return true;
#endif
}

void PLC_AssocCnfAddAuth(u8* data, u16 data_len)
{
#ifdef HPLC_CSG
	return;
#else
	if (!data)
	{
		return;
	}

#if defined(ASSOC_CNF_YINGXIAO) || defined(ASSOC_AUTH_CRC16)
	ASSOC_CNF_PACKET *pAssocCnf = (ASSOC_CNF_PACKET *)data;
	ASSOC_GATHER_IND_PACKET *pAssocGather = (ASSOC_GATHER_IND_PACKET *)data;
#endif

#ifdef ASSOC_CNF_YINGXIAO
	if (g_PlmConfigPara.YX_mode=='Y')
	{
		if (data_len == sizeof(ASSOC_CNF_PACKET))
		{
			memcpy(pAssocCnf->AppFlag, ASSOC_CNF_YINGXIAO, sizeof(pAssocCnf->AppFlag));
		}
		else if (data_len == sizeof(ASSOC_GATHER_IND_PACKET))
		{
			memcpy(pAssocGather->AppFlag, ASSOC_CNF_YINGXIAO, sizeof(pAssocGather->AppFlag));
		}
	}
#elif defined(ASSOC_AUTH_CRC16)
	u16 crc = 0;

	if (data_len == sizeof(ASSOC_CNF_PACKET))
	{
		crc = GetCrc16(data, (u8 *)&(pAssocCnf->Reserve3) - data); //站点MAC地址-->路径序号
		//CRC大端传输
		pAssocCnf->AppFlag[1] = (crc>>8) & 0xFF; 
		pAssocCnf->Reserve4 = crc & 0xFF;
	}
	else if (data_len == sizeof(ASSOC_GATHER_IND_PACKET))
	{
		crc = GetCrc16(data, (u8 *)&(pAssocGather->Reserve3) - data); //结果-->汇总站点数
		//CRC大端传输
		pAssocGather->AppFlag[1] = (crc>>8) & 0xFF; 
		pAssocGather->Reserve4 = crc & 0xFF;
	}
#endif

	return;
#endif
}



//关联确认
void PLC_AssocCnfOK(ASSOC_REQ_PACKET *pAssocReq, USHORT tei, u8 assoc_res)
{
	u8 *pMsdu = NULL;
	P_STA_BITMAP pStaBimap = NULL;
	ROUTE_MSG_FILED *pRouteHead = NULL;


	P_STA_INFO pStaInfo = PLC_GetValidStaInfoByTei(tei);
	if (NULL == pStaInfo)
		return;

	pMsdu = (u8 *)alloc_buffer(__func__,ASSOC_CNF_MAC_FRAME);
	if (NULL == pMsdu)
		return;

	memset_special(pMsdu, 0, ASSOC_CNF_MAC_FRAME);

	P_MM_FRAME pMmFrame = (P_MM_FRAME)pMsdu;

	pMmFrame->head.MMType = MME_ASSOC_CNF;
	ASSOC_CNF_PACKET *pAssocCnf = (ASSOC_CNF_PACKET *)pMmFrame->body;


	//直连站点
	pStaBimap = PLC_GetDirectSubStas(tei, false);
	if (NULL == pStaBimap)
		goto PLC_AssocCnfOK_Exit;

	memset_special(pAssocCnf, 0, sizeof(ASSOC_CNF_PACKET));
	memcpy_special(pAssocCnf->STAMacAddr, pAssocReq->MacAddr, sizeof(pAssocCnf->STAMacAddr));
#if defined(HPLC_CSG)
	pMmFrame->head.MMVersion = 1;
	pAssocCnf->NetSeq = g_pPlmConfigPara->net_seq;
	pAssocCnf->MngMsgVersion = pAssocReq->MngMsgVersion;
	if (FrequenceGet()!=g_PlmConfigPara.hplc_comm_param) //此时在探测频段内
	  	pAssocCnf->detechFlag =1;
	else
		pAssocCnf->detechFlag =0;
#else
	PLC_CopyCCOMacAddr(pAssocCnf->CCOMacAddr, TRUE);
#endif
	pAssocCnf->Result = assoc_res;
	pAssocCnf->Level = PLC_GetStaLevel(tei);
	pAssocCnf->STATei = tei;
	pAssocCnf->PCOTei = pStaInfo->pco;
	pAssocCnf->StationRandom = pAssocReq->STARandom;
	pAssocCnf->P2PSeq = pAssocReq->P2PSeq;
	pAssocCnf->RouteSeq = g_pPlmConfigPara->route_seq++;

#ifdef HRF_SUPPORT
	if(pAssocReq->MoudleType==0)
	{
		pAssocCnf->Band = 0;
		pAssocCnf->Link = 0;
	}
    else
    {
      pAssocCnf->Band = g_pPlmConfigPara->hplc_comm_param;
      pAssocCnf->Link = pStaInfo->Link;
    }
#else
      pAssocCnf->Band = 0;
      pAssocCnf->Link = 0;    
#endif

	pRouteHead = (ROUTE_MSG_FILED *)&pAssocCnf->RouteData;
	pRouteHead->Reserve1 = 0;

	writeSendRoutTable(pMmFrame, true, pRouteHead, pStaBimap, tei, (u16 *)&pMsdu[ASSOC_CNF_MSDU_LEN - 4 - 1]);

	g_pRelayStatus->selfreg_cb.selfreg_new_sta_tick = GetTickCount64();

PLC_AssocCnfOK_Exit:
	free_buffer(__func__,pMsdu);
	free_buffer(__func__,pStaBimap);

}

//关联否认
void PLC_AssocCnfError(MAC_HEAD *pMacHead, ASSOC_REQ_PACKET *pAssocReq, ASSOC_RESULT assoc_res)
{
	u8 *pMsdu = (u8 *)alloc_buffer(__func__,MAX_MAC_FRAME_LEN);
	if (NULL == pMsdu)
		return;

	memset_special(pMsdu, 0, MAX_MAC_FRAME_LEN);

	u16 tei_pco = pAssocReq->CandidataPCO[0].TEI;
	u8 link = pAssocReq->CandidataPCO[0].Link;

	P_MM_FRAME pMmFrame = (P_MM_FRAME)pMsdu;

	pMmFrame->head.MMType = MME_ASSOC_CNF;
	ASSOC_CNF_PACKET *pAssocCnf = (ASSOC_CNF_PACKET *)pMmFrame->body;

	memset_special(pAssocCnf, 0, sizeof(ASSOC_CNF_PACKET));
	memcpy_special(pAssocCnf->STAMacAddr, pAssocReq->MacAddr, sizeof(pAssocCnf->STAMacAddr));

#if defined(HPLC_CSG)
	pMmFrame->head.MMVersion = 1;
	pAssocCnf->NetSeq = g_pPlmConfigPara->net_seq;
	pAssocCnf->MngMsgVersion = pAssocReq->MngMsgVersion;
	pAssocCnf->LastPacketFlag = 1;
	if (FrequenceGet()!=g_PlmConfigPara.hplc_comm_param) //此时在探测频段内
	  	pAssocCnf->detechFlag =1;
	else
		pAssocCnf->detechFlag =0;
#else
	PLC_CopyCCOMacAddr(pAssocCnf->CCOMacAddr, TRUE);
#endif

	pAssocCnf->Result = (u8)assoc_res;
	pAssocCnf->Level = 0;
	pAssocCnf->STATei = TEI_INVALID;
	pAssocCnf->PCOTei = tei_pco;
	pAssocCnf->TotalPacket = 1;        //总分包数
	pAssocCnf->PacketSeq = 1;          //分包序号
	pAssocCnf->StationRandom = pAssocReq->STARandom;
	pAssocCnf->P2PSeq = pAssocReq->P2PSeq;
	pAssocCnf->RouteSeq = g_pPlmConfigPara->route_seq++;
#ifdef HRF_SUPPORT
	if(pAssocReq->MoudleType==0)
	{
		pAssocCnf->Band = 0;
		pAssocCnf->Link = 0;
	}
    else
    {
      pAssocCnf->Band = g_pPlmConfigPara->hplc_comm_param;
      pAssocCnf->Link = 0;
    }
#else
      pAssocCnf->Band = 0;
      pAssocCnf->Link = 0;    
#endif
	if (HPLC_FindOneNeighborNidNum())
	{
		pAssocCnf->ReAssocTime = CON_REASSOC_MUTI_TIME_MS;        //重新关联时间, ms        华为取值 = 420000
	}
	else
	{
		pAssocCnf->ReAssocTime = CON_REASSOC_TIME_MS;        //重新关联时间, ms        华为取值 = 420000
	}
	//pAssocCnf->ReAssocTime = 10000;        //for test   重新关联时间, ms
	if (PLC_AssocCnfAddAuthFunc)
	{
		PLC_AssocCnfAddAuthFunc((u8 *)pAssocCnf, sizeof(ASSOC_CNF_PACKET));
	}

	ROUTE_MSG_FILED *pRouteHead = (ROUTE_MSG_FILED *)&pAssocCnf->RouteData;
	pRouteHead->DirectSTANum = 0;       //直连站点数
	pRouteHead->DirectPCONum = 0;       //直连代理数
	pRouteHead->RouteTableSize = 0;     //路由信息表大小
	pRouteHead->Reserve1 = 0;

	//发送
	u16 msdu_len = sizeof(MM_HEAD) + (sizeof(ASSOC_CNF_PACKET) - 1) + (sizeof(ROUTE_MSG_FILED) - 1);

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 1;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_ASSOC_CNF;

#if defined(HPLC_CSG)
	mac_param.HeadType = MAC_HEAD_LONG;
	mac_param.Vlan = VLAN_MM;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
#endif

	if (TEI_CCO == tei_pco)
	{
		//广播
		send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
		send_param.link=link?FreamPathHrf:FreamPathBplc;
		send_param.doubleSend=0;
		SOF_param.DstTei = TEI_BROADCAST;
		SOF_param.BroadcastFlag = 1;

		mac_param.DstTei = TEI_BROADCAST;
		mac_param.SendType = SENDTYPE_LOCAL;
		mac_param.SendLimit = 3;

		mac_param.MsduType = MSDU_NET_MANAGER;
		mac_param.NetSeq = g_pPlmConfigPara->net_seq;

#if defined(HPLC_CSG)
		mac_param.HopCount = 1;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
#else
		mac_param.HopCount = MAX_RELAY_DEEP;
		mac_param.Broadcast = BROADCAST_DRT_DUL;
		mac_param.mac_flag = MACFLAG_NONE;
#endif
		PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
		//memcpy_swap(mac_param.DstMac, pAssocReq->MacAddr, MAC_ADDR_LEN);
		memset(mac_param.DstMac, 0xff, MAC_ADDR_LEN);
	}
	else
	{
		//单播
		P_STA_INFO pStaPco = PLC_GetValidStaInfoByTei(tei_pco);
		if (NULL == pStaPco)
			goto PLC_AssocCnfError_Exit;

		PLC_GetSendParam(tei_pco, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
		send_param.doubleSend=0;
		SOF_param.BroadcastFlag = 0;

		mac_param.DstTei = SOF_param.DstTei;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.SendLimit = 3;
		mac_param.MsduType = MSDU_NET_MANAGER;
		mac_param.NetSeq = g_pPlmConfigPara->net_seq;

#if defined(HPLC_CSG)
		mac_param.HopCount = 0;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
#else
		mac_param.HopCount = MAX_RELAY_DEEP;
		mac_param.Broadcast = BROADCAST_DRT_DUL;
		mac_param.mac_flag = MACFLAG_NONE;
#endif

		PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
		pStaPco = PLC_GetValidStaInfoByTei(SOF_param.DstTei);
		if (NULL != pStaPco)
			memcpy_special(mac_param.DstMac, pStaPco->mac, MAC_ADDR_LEN);
		else
			goto PLC_AssocCnfError_Exit;
	}

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, pMsdu, msdu_len);

PLC_AssocCnfError_Exit:
	free_buffer(__func__,pMsdu);
}



//重新定位角色
BOOL PLC_RefixedRoles(u16 tei)
{
#if 0
	if(TEI_CCO == tei)
	return TRUE;

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
	if(NULL == pSta)
	return FALSE;

	P_STA_BITMAP pBitmap = PLC_GetAllSubStas(tei);
	if(NULL == pBitmap)
	return FALSE;

	u8 old_roles = pSta->roles;
	if(0==pBitmap->count)
	pSta->roles = ROLES_STA;
	else
	pSta->roles = ROLES_PCO;
	free_buffer(__func__,pBitmap);

	if(old_roles != pSta->roles)
	{
		PLC_TriggerSaveStaInfo();
		return TRUE;
	}

	return FALSE;
#endif
	return TRUE;
}

void PLC_TimerRefixedRoles()
{
	static u16 s_tei = TEI_STA_MIN;

//    PLC_RefixedRoles(s_tei++);
	if (s_tei > TEI_STA_MAX)
		s_tei = TEI_STA_MIN;
}

u8 PLC_ResetSilenceTime(u16 tei, BOOL check_whitelist)
{
	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
	PLC_IdentInNet(tei);
	if (NULL == pSta)
		return (!OK);
	u16 node_sn=PLC_get_sn_from_addr(pSta->mac);
	if (pSta->mac_join)//次节点使用自身节点入网
	{
		if (pSta->dev_type != METER && pSta->dev_type != METER_3PHASE)
		{
			check_whitelist=false;
		}
	}
	if(pSta->dev_type == RELAY)
	{
		check_whitelist=false;
	}
	if (check_whitelist)
	{
		if (node_sn >= CCTT_METER_NUMBER||pSta->net_state == NET_OUT_NET)
		{
			PLC_AddToOfflineList(pSta->mac, OFFLINE_NOTIFY);
			//PLC_TriggerOfflineList();
			return (!OK);
		}
	}

	if (NET_OFFLINE_NET == pSta->net_state)
	{
		pSta->net_state = NET_IN_NET;
		debug_str(DEBUG_LOG_NET, "tei %d 重新入网\r\n", tei);
#ifdef GDW_2019_PERCEPTION
		if (node_sn < CCTT_METER_NUMBER)
		{
			goto_online_bitmap[tei >> 5] |= 1 << (tei & 0x1f);
		}

#endif
	}

	P_STA_STATUS_INFO pStaStatus = STA_STATUS_INFO_BY_TEI(tei);
	pStaStatus->silence_sec = 0;

	return OK;
}

#ifdef DEBUG_METER_RELAY

typedef struct
{
	u8 level;
	u8 mac[MAC_ADDR_LEN];
}DEBUG_MAC, *P_DEBUG_MAC;

DEBUG_MAC debug_mac[] =
{
#if 0
	{.level = 1, .mac = {0x12, 0x49, 0x07, 0x04, 0x19, 0x02}
	},
	{.level = 2, .mac = {0x13, 0x49, 0x07, 0x04, 0x19, 0x02}
	},
	{.level = 3, .mac = {0x10, 0x49, 0x07, 0x04, 0x19, 0x02}
	},
	{.level = 3, .mac = {0x11, 0x49, 0x07, 0x04, 0x19, 0x02}
	},
#elif 1
	{ .level = 1, .mac = { 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 } },
	{ .level = 2, .mac = { 0x12, 0x49, 0x07, 0x04, 0x19, 0x02 } },
	{ .level = 3, .mac = { 0x13, 0x49, 0x07, 0x04, 0x19, 0x02 } },
	{ .level = 4, .mac = { 0x55, 0x88, 0x29, 0x00, 0x90, 0x12 } },
#elif 1
	{.level = 1, .mac = {0x01, 0x01, 0x00, 0x00, 0x55, 0x07}
	},
	{.level = 2, .mac = {0x02, 0x01, 0x00, 0x00, 0x55, 0x07}
	},
	{.level = 3, .mac = {0x03, 0x01, 0x00, 0x00, 0x55, 0x07}
	},
	{.level = 4, .mac = {0x04, 0x01, 0x00, 0x00, 0x55, 0x07}
	},
	{.level = 4, .mac = {0x05, 0x01, 0x00, 0x00, 0x55, 0x07}
	},
	{.level = 5, .mac = {0x06, 0x01, 0x00, 0x00, 0x55, 0x07}
	},
	{.level = 6, .mac = {0x07, 0x01, 0x00, 0x00, 0x55, 0x07}
	},
	{.level = 7, .mac = {0x08, 0x01, 0x00, 0x00, 0x55, 0x07}
	},
#else
	{ .level = 1, .mac = { 0x01, 0x01, 0x00, 0x00, 0x55, 0x07 } },
	{ .level = 2, .mac = { 0x02, 0x01, 0x00, 0x00, 0x55, 0x07 } },
	{ .level = 3, .mac = { 0x03, 0x01, 0x00, 0x00, 0x55, 0x07 } },
	{ .level = 4, .mac = { 0x04, 0x01, 0x00, 0x00, 0x55, 0x07 } },
	{ .level = 5, .mac = { 0x05, 0x01, 0x00, 0x00, 0x55, 0x07 } },
	{ .level = 6, .mac = { 0x06, 0x01, 0x00, 0x00, 0x55, 0x07 } },
	{ .level = 7, .mac = { 0x07, 0x01, 0x00, 0x00, 0x55, 0x07 } },
	{ .level = 8, .mac = { 0x08, 0x01, 0x00, 0x00, 0x55, 0x07 } },
#endif
};

P_DEBUG_MAC PLC_IsDebugMac(unsigned char mac_addr[MAC_ADDR_LEN])
{
	for (u16 i = 0; i < countof(debug_mac); i++)
	{
		if (OK != macCmp(mac_addr, debug_mac[i].mac))
			continue;
		return &debug_mac[i];
	}

	return NULL;
}

#endif

#ifdef PROTOCOL_NW_2020
RemMeter_s OutListMeter;
void AddOutListMeter(u8* mac)
{
	if (OutListMeter.cnt >= MAX_OUT_LIST_METER)
	{
		return;
	}

	for (int i = 0; i < OutListMeter.cnt; i++)
	{
		if (macCmp(mac, OutListMeter.mac[i]) == OK)
		{
			return;
		}
	}
	
	u16 meter_sn, emptysn;
	meter_sn = PLC_Trans_MeterAddr2Sn(mac, &emptysn);
	if (meter_sn < CCTT_METER_NUMBER)
	{
		memcpy(&OutListMeter.mac[OutListMeter.cnt], mac, 6);
		OutListMeter.cco_num[OutListMeter.cnt] = 0xFF; //赋初值，未知台区
		OutListMeter.cnt++;
	}

    PLC_TriggerSaveOutListMeterInfo();
}
void ClearOutListMeter(int clear_with)//bit 0删除白名单  bit 1删除多余列表
{
	if (clear_with&1)
	{
		for (int i = 0; i < OutListMeter.cnt; i++)
		{
			P_RELAY_INFO pNode = PLC_GetWhiteNodeInfoByMac(OutListMeter.mac[i]);
			if (pNode != NULL)
			{
				if (!pNode->status.cfged)
				{
					memset(pNode, 0xFF, sizeof(RELAY_INFO)); //删除白名单
				}
			}
		}
        PLC_TriggerSaveOutListMeterInfo();
	}
	
	if (clear_with&0x2)
	{
		OutListMeter.cnt = 0;
        PLC_TriggerSaveOutListMeterInfo();
	}
}
int FindOutListMeter(u8 *mac)
{
	for (int i = 0; i < OutListMeter.cnt; i++)
	{
		if (macCmp(mac, OutListMeter.mac[i]) == OK)
		{
			return i;
		}
	}
	
	return MAX_OUT_LIST_METER;
}
#endif



//处理发现列表报文
void PLC_HandleNodeList(MAC_HEAD *pMacHead, u8 *pdata, u16 len)
{
	DISCOVER_NODE_LIST_PACKET *pDiscoverList = (DISCOVER_NODE_LIST_PACKET *)pdata;
	USHORT p = 0;
	u8 mac_addr[MAC_ADDR_LEN];

	memcpy_swap(mac_addr, pDiscoverList->MacAddr, MAC_ADDR_LEN);

	P_STA_INFO pSta = PLC_GetStaInfoByMac(mac_addr);
	if (NULL == pSta)
	{
		//发送离线指示报文，送检必测项目
		//debug_str("发现列表触发离线指示\r\n");
		PLC_AddToOfflineList(mac_addr, OFFLINE_NOTIFY);
		return;
	}
	if (pSta->net_state!=NET_IN_NET)//已经离线并且不再白名单中
	{
		if(PLC_GetWhiteNodeSnByStaInfo(pSta)>=CCTT_METER_NUMBER||pSta->net_state == NET_OUT_NET)
		{
			PLC_AddToOfflineList(mac_addr, OFFLINE_NOTIFY);
			return;
		}
	}
	//更新相位
	u8 new_phase;

#if defined(HPLC_CSG)
	new_phase = (pDiscoverList->Phase >> 4) & 0x03;
#else
	new_phase = (PLC_GetHplcPhaseByReport(pDiscoverList->Phase));
#endif

	if (PHASE_UNKNOW != new_phase)
	{
		if (PLC_GetStaLevel(pSta - g_pStaInfo) == 1)
		{
			lockStaInfo();
			if (pSta->Link)
			{
				//changeHplcPhase(pSta, PHASE_ALL);
			}
			else
			{
				changeHplcPhase(pSta, (HPLC_PHASE)new_phase);
			}
			unLockStaInfo();
		}
//        u8 old_pahse = pSta->hplc_phase;
//        pSta->hplc_phase = new_phase;
//        if(old_pahse != pSta->hplc_phase)
//            PLC_TriggerSaveStaInfo();
	}
	if (!pSta->Link)//入网采用HPLC方式
	{
		if (OK != PLC_ResetSilenceTime(pDiscoverList->TEI, TRUE))
			return;
	}

	P_STA_STATUS_INFO pStaStaus = PLC_GetStaStatusInfoByTei(pDiscoverList->TEI);
	if (NULL == pStaStaus)
		return;
	pStaStaus->found_list_counter_this_route++;
	
	//debug_str("HandleNodeList, TEI = %d, PCOTei = %d\r\n", pDiscoverList->TEI, pDiscoverList->PCOTei);

//    UP_ROUTE_FILED* pUpRoute = (UP_ROUTE_FILED*)(&pDiscoverList->Data[p]);
	p += (sizeof(UP_ROUTE_FILED) * pDiscoverList->UpRouteNum);

	u16 BitmapSize;
	u8 upRouteSize;

#if defined(HPLC_CSG)
	BitmapSize = CSG_DISCOVER_BITMAP_SIZE;
	upRouteSize = 3;
#else
	BitmapSize = pDiscoverList->BitmapSize;
	upRouteSize = 2;
#endif

	u16 bitmap_size =  BitmapSize;
	if (pDiscoverList->Data[pDiscoverList->UpRouteNum*upRouteSize] & (1 << CCO_TEI))
	{
		StaNeighborTab.Neighbor[pSta-g_pStaInfo].LastNghRxCnt= pDiscoverList->Data[pDiscoverList->UpRouteNum*upRouteSize+bitmap_size];//若发现cco其后bitmap第一个必是cco的个数
	}
	StaNeighborTab.Neighbor[pSta-g_pStaInfo].LastNghTxCnt=pDiscoverList->SendPacketNum;


	p += BitmapSize;
}
#ifdef HRF_SUPPORT
//获取站点的hrf下行通信成功率
u8 GetHrfDownRate(u16 TEI)
{
	//如果bitmap更改大小此处代码需要重新改写！！！！
	u8 rate=0;
	if (TEI > MIN_TEI_VALUE && TEI <= MAX_TEI_VALUE)
	{
		//最低bit由于延时的问题暂时不算在成功率统计之中  即最近的一个无线发现列表不统计入成功率之中
		// 进行三段加权统计成功率每段5bit 权值为5 3 2
		u32 getBit=0;
		
		getBit=(g_pStaInfo[TEI].HrfBitmap>>1)&0x1f;
		getBit=ByteCount1Bit(getBit);
             
		rate=getBit*100/5 *5/10;//getBit*100/5 为百分比  *5/10为权重

		getBit=(g_pStaInfo[TEI].HrfBitmap>>6)&0x1f;
		getBit=ByteCount1Bit(getBit);
		rate+=getBit*100/5 *3/10;

		getBit=(g_pStaInfo[TEI].HrfBitmap>>11)&0x1f;
		getBit=ByteCount1Bit(getBit);
		rate+=getBit*100/5 *2/10;
	}
	return rate;
}
void SetHrfBit(u16 TEI, u8 seq)
{
	if (TEI > MIN_TEI_VALUE && TEI <= MAX_TEI_VALUE)
	{
		s8 diff=seq-g_pStaInfo[TEI].HrfSeq;
		if (diff>=0)//获取到的新的
		{
			g_pStaInfo[TEI].HrfBitmap<<=diff;
			g_pStaInfo[TEI].HrfBitmap|=1;
			g_pStaInfo[TEI].HrfSeq=seq;
		}
		else if (diff >(-HRF_BIT_MAP_SIZE))//在bitmap之内
		{
			if (g_pStaInfo[TEI].HrfBitmap)
			{
				diff = -diff;
				g_pStaInfo[TEI].HrfBitmap |= 1<<diff;
				g_pStaInfo[TEI].HrfBitmap >>= diff;
			}
			else
			{
				g_pStaInfo[TEI].HrfBitmap = 1;
			}
			g_pStaInfo[TEI].HrfSeq = seq;
		}
		else//超出bitmap范围之外认为bitmap已经无效
		{
			g_pStaInfo[TEI].HrfBitmap=1;
			g_pStaInfo[TEI].HrfSeq=seq;
		}
		g_pStaInfo[TEI].HrfNoCnt=0;
	}
}

#define BIT_LEN0 0
#define BIT_LEN4 1
#define BIT_LEN6 2
#define BIT_LEN8 3
const u8 HRF_BIT2LEN[4]={0,4,6,8
};

#if defined(HPLC_CSG)
const u8 HRF_TYPE_LEN[]=
{
	//接受率      SNR         RSSI
 	BIT_LEN8|(BIT_LEN0<<2)|(BIT_LEN0<<4),
	BIT_LEN8|(BIT_LEN4<<2)|(BIT_LEN0<<4),
	BIT_LEN8|(BIT_LEN6<<2)|(BIT_LEN6<<4),
	BIT_LEN8|(BIT_LEN0<<2)|(BIT_LEN4<<4),
};
const u8 HRF_BIT_LEN[]=
{
	//接受率      SNR         RSSI
 	BIT_LEN8|(BIT_LEN0<<2)|(BIT_LEN0<<4),
	BIT_LEN8|(BIT_LEN4<<2)|(BIT_LEN0<<4),
	BIT_LEN8|(BIT_LEN0<<2)|(BIT_LEN8<<4),
	BIT_LEN8|(BIT_LEN6<<2)|(BIT_LEN6<<4),
	BIT_LEN8|(BIT_LEN4<<2)|(BIT_LEN4<<4),
	BIT_LEN8|(BIT_LEN0<<2)|(BIT_LEN4<<4),
};

#else
const u8 HRF_TYPE_LEN[]=
{
	//接受率      SNR         RSSI
 	BIT_LEN8|(BIT_LEN0<<2)|(BIT_LEN0<<4),
	BIT_LEN0|(BIT_LEN4<<2)|(BIT_LEN0<<4),
	BIT_LEN0|(BIT_LEN0<<2)|(BIT_LEN8<<4),
	BIT_LEN0|(BIT_LEN0<<2)|(BIT_LEN4<<4),
	BIT_LEN8|(BIT_LEN4<<2)|(BIT_LEN0<<4),
	BIT_LEN6|(BIT_LEN0<<2)|(BIT_LEN6<<4),
	BIT_LEN0|(BIT_LEN6<<2)|(BIT_LEN6<<4),
	BIT_LEN8|(BIT_LEN6<<2)|(BIT_LEN6<<4),
	BIT_LEN0|(BIT_LEN4<<2)|(BIT_LEN4<<4),
	BIT_LEN8|(BIT_LEN0<<2)|(BIT_LEN4<<4),
};
const u8 HRF_BIT_LEN[]=
{
	//接受率      SNR         RSSI
 	BIT_LEN8|(BIT_LEN0<<2)|(BIT_LEN0<<4),
	BIT_LEN0|(BIT_LEN4<<2)|(BIT_LEN0<<4),
	BIT_LEN0|(BIT_LEN0<<2)|(BIT_LEN8<<4),
	BIT_LEN0|(BIT_LEN0<<2)|(BIT_LEN4<<4),
	BIT_LEN8|(BIT_LEN4<<2)|(BIT_LEN0<<4),
	BIT_LEN8|(BIT_LEN0<<2)|(BIT_LEN8<<4),
	BIT_LEN0|(BIT_LEN6<<2)|(BIT_LEN6<<4),
	BIT_LEN8|(BIT_LEN6<<2)|(BIT_LEN6<<4),
	BIT_LEN8|(BIT_LEN4<<2)|(BIT_LEN4<<4),
	BIT_LEN0|(BIT_LEN4<<2)|(BIT_LEN4<<4),
	BIT_LEN8|(BIT_LEN0<<2)|(BIT_LEN4<<4),
};

#endif
u8* UpNeighChannelInfo(u16 scrTei,u8 type, u8 *data, u16 *bitStart,u8 isBit,u16 tei)//其中的TEI的参数在bitmap模式下才需要传入
{

	u32 * p=(u32*)data;
	u32 thisBit=(*p)>>(*bitStart);
	++p;
	thisBit|=(*p)<<(32-(*bitStart));
//	u16 tei;
	if (isBit)
	{
		type=HRF_BIT_LEN[type];
	}
	else
	{
		tei=thisBit&0xfff;
		type=HRF_TYPE_LEN[type];
		thisBit >>= 12;
		*bitStart=+12;
	}

	if (CCO_TEI==tei)
	{
		u8 getLen = HRF_BIT2LEN[type & 0x3]; //获取接收率长度
		type >>= 2;
		if (getLen)
		{
			*bitStart=+getLen;
			g_pStaInfo[scrTei].HrfUpRate = thisBit & GetMask(getLen);
			thisBit >>= getLen;
		}

		getLen = HRF_BIT2LEN[type & 0x3]; //获取hrf SNR值
		type >>= 2;
		if (getLen)
		{
			*bitStart=+getLen;
			g_pStaInfo[scrTei].RfSnr = thisBit & GetMask(getLen);
			thisBit >>= getLen;
		}

		getLen = HRF_BIT2LEN[type & 0x3]; //获取hrf RSSI
		type >>= 2;
		if (getLen)
		{
			*bitStart=+getLen;
			g_pStaInfo[scrTei].RfRssi = thisBit & GetMask(getLen);
			thisBit >>= getLen;
		}
		return NULL;//已找到自身无需在向下寻找

	}
	else
	{
		for (int i = 0; i < 3; i++)
		{
			*bitStart += HRF_BIT2LEN[type & 0x3]; //获取接收率长度
			type >>= 2;
		}
	}
	data += (*bitStart) / 8;
	*bitStart %= 8;
	return data;
}

//写入到对应的bit之中
static u16 WriteBit2Buffer(u8 *buffer, u16 startBit, u32 data, u8 len)
{
	u16 byteIndex=startBit>>3;
	u16 bitIndex=startBit&0x7;
	//由于非对齐访问此处不可使用u64直接赋值
	u32 *p=(u32 *)&buffer[byteIndex];
	u32 mask=1<<bitIndex;
	mask-=1;
	(*p)&=mask;
	(*p)|=data<<bitIndex;
	p++;
	*p = 0;
	*p|=data<<(32-bitIndex);
	return startBit+len;
	
}

//获取到邻居节点的信道信息
static void GetNigInfo(u8 type, u16 tei,u8 *data, u16 *bitStart,u8 isBit)
{
	u32 thisBit=0;
	
	u8 useLen;
	if (isBit)
	{
		useLen=0;
		type=HRF_BIT_LEN[type];
	}
	else
	{
		useLen=12;
		type=HRF_TYPE_LEN[type];
		thisBit = tei;
	}
	
	if ( tei < CCO_TEI || tei > MAX_TEI_VALUE)
	{
		return;
	}

	u8 getLen = HRF_BIT2LEN[type & 0x3];
	type>>=2;
	if (getLen)//成功率
	{
		u8 downRate;
		downRate=GetHrfDownRate(tei);
		if (getLen==6)
		{
			downRate/=2;
		}
		thisBit |= downRate << useLen;
		useLen+=getLen;
	}

	getLen = HRF_BIT2LEN[type & 0x3];
	type>>=2;
	if (getLen)//信噪比
	{
		if (getLen==4)//4bit 形式
		{
			s8 snr=g_pStaInfo[tei].RfSnr;
			if(snr<=-6)
			{
				snr=0;
			}
			else if (snr<=16)
			{
				snr=(snr-(-5))/2+1;
			}
			else if (snr<=25)
			{
				snr=(snr-17)/3+0xc;
			}
			else 
			{
				snr=0xf;
			}
			thisBit|=(u32)snr<<useLen;
		}
		else//6bit形式
		{
			u8 snr=g_pStaInfo[tei].RfSnr<=-6?0:g_pStaInfo[tei].RfSnr+6;
			thisBit|=(u32)snr<<useLen;
		}
		useLen+=getLen;
	}

	getLen = HRF_BIT2LEN[type & 0x3];
	type>>=2;
	if (getLen)//RSSI
	{
		if (getLen==4)
		{
			s8 rssi=g_pStaInfo[tei].RfRssi;
			if (rssi<=-105)
			{
				rssi=0;//0
			}
			else if (rssi<=-95)
			{
				rssi=1;
			}
			else if (rssi<=-86)
			{
				rssi=2;
			}
			else if (rssi<=-56)
			{
				rssi=(rssi-(-85))/6+3;
			}
            else if (rssi<=-48)
			{
				rssi=8;
			}            
			else if (rssi<=-30)
			{
				rssi=(rssi-(-47))/6+9;
			}
			else if (rssi<0)
			{
				rssi=(rssi-(-31))/10+0xb;
			}
			thisBit|=(u32)rssi<<useLen;
		}
		else if(getLen==6)
		{
			s8 rssi=g_pStaInfo[tei].RfRssi;
			if (rssi<=-110)
			{
				rssi=0;//0
			}
			else if (rssi<=14)
			{
				rssi=(rssi-(-109))/2+1;
			}
			else
			{
				rssi=0x3f;
			}
			thisBit|=(u32)rssi<<useLen;
		}
		else
		{
			thisBit|=(u32)g_pStaInfo[tei].RfRssi<<useLen;
		}
		useLen+=getLen;
	}
	*bitStart=WriteBit2Buffer(data, *bitStart,thisBit,useLen);
	return;
}

/*
内存被分成4块
			-------------------------------------------------------------------------------------------------------------
			|发送的帧    |非bitmap的临时空间   | bitmap临时空间   |bitmap 的数据暂存地址 在结算时会把这部分空间搬运到前面  |
			------------------------------------------------------------------------------------------------------------
			每次进行结算时会比较 非bitmap和bitmap哪种方案更节省空间 则确定哪种 另一种的指针回退作废本次无用的空间
*/
u16 WiteNeighborInfo(u8 type,u8 bit_type,u8 *src_data,s16 len,u8 * bitmap)
{
	if (len>500)//长度错误
	{
		return 0;
	}
	u8 *temp=(u8*)MallocPDU();
	if (temp==NULL)
	{
		return 0;
	}

	u16 item_bit_len=HRF_BIT_LEN[bit_type];
	u16 item_len=HRF_TYPE_LEN[type];
	//每个位图版的bit长度
	item_bit_len=HRF_BIT2LEN[item_bit_len&0x3]+HRF_BIT2LEN[(item_bit_len>>2)&0x3]+HRF_BIT2LEN[(item_bit_len>>4)&0x3];
	//每个非位图的bit长度
	item_len=HRF_BIT2LEN[item_len&0x3]+HRF_BIT2LEN[(item_len>>2)&0x3]+HRF_BIT2LEN[(item_len>>4)&0x3];
	item_len+=12;//+tei

	u8 *buffer=temp;//非位图的缓冲指针 
	buffer=(u8 *)((u32)(buffer+3)/4*4);//buffer 指针对4取整
	buffer[0]=2|1<<7;//信息单元类型2 非位图版本 长度字段为2byte
	buffer[3]=type;//组合信道类型
	buffer+=4;
	u16 buffer_current=0;//已经写入的bit位置
	u16 buffer_last=0;//最后一个确定的位图位置


	u8 *buffer_bit=&temp[508];//位图的缓冲指针 WriteBit2Buffer会多写2个32位放止内存被破坏
	buffer_bit=(u8 *)((u32)(buffer_bit+3)/4*4);//buffer 指针对4取整
	buffer_bit[0]=3|1<<7;//信息单元类型3 位图版本 长度字段为2byte
	buffer_bit[3]=bit_type;//组合信道类型
	buffer_bit+=4;
	u16 buffer_bit_current=0;
	//u16 buffer_bit_last=0;
	u8 *buffer_bitdata=&temp[1016];//bitmap 内容 的指针位置 WriteBit2Buffer会多写2个32位放止内存被破坏
	buffer_bitdata=(u8 *)((u32)(buffer_bitdata+3)/4*4);//buffer 指针对4取整
	s16 bitRemain=len*8-4*8-1*8-(3*8+item_bit_len)*2;//空余一个节点的空位放止写溢出 4=1类型+2长度+1组合类型  1组合信息类型  3=2起始tei+1bitmap len+1bitmap

	u8 null_bit_num=0;//连续bit为0的个数
	u8 map_inv_index=0;//字节内
	u8 map_index=0;//bitmap的第几个字节
	u8 *pmap=buffer_bit;
	s16 Remain=len*8-4*8-item_len*2;//空余一个节点的空位放止写溢出 4=1类型+2长度+1组合类型 

	for (int tei=TEI_STA_MIN;tei<TEI_CNT;tei++)
	{
		if (bitmap[tei>>3]&(1<<(tei&0x7)))//下行成功率为0则不添加到邻居列表中
		{
			if (map_inv_index==0)//下一个byte上  需要把该byte清零
			{
				pmap[map_index]=0;
				if (map_index==0)//本次的bitmap重新开始了
				{
					*(u16*)pmap=tei;//起始TEI
					pmap+=2;
					*pmap++=0;//位图大小为0
					bitRemain-=3*8;
					pmap[map_index]=0;
				}
				pmap[map_index]|=1<<map_inv_index;
				map_inv_index=0xff;//设置一个值用于启动map_inv_index++
			}
			else
			{
				pmap[map_index]|=1<<map_inv_index;
			}
			
			null_bit_num=0;
			GetNigInfo(bit_type,tei, buffer_bitdata,&buffer_bit_current,1);//写入位图
			bitRemain-=item_bit_len;
			if (Remain>=0)//有
			{
				GetNigInfo(type, tei, buffer, &buffer_current, 0); //写入非位图
				Remain-=item_len;
			}
			else
			{
				//buffer_current=buffer_last;
			}
			
		}
		else
		{
			if (map_inv_index==0)
			{
				pmap[map_index]=0;
			}
			if (map_inv_index != 0 || map_index != 0)
			{
				null_bit_num++;
			}
		}
		if (map_inv_index!=0||map_index!=0)
		{
			if (map_inv_index==0xff)
			{
				map_inv_index=1;//	map_inv_index 代表新的一次 下一次的值是1
			}
			else//
			{
				map_inv_index++;
			}
		}
		if (map_inv_index==8)//写到了最后的byte
		{
			map_index++;
			if (null_bit_num>=3*8||bitRemain<=0)//出现了连续3个字节是空的字节需要进行结算长度
			{
				u8 nullIndex;
				for (nullIndex=0;nullIndex<3;nullIndex++)
				{
					if (pmap[map_index-1-nullIndex]!=0)//发现为0的bitmap需要移除
					{
					  	bitRemain+=8;
						break;
					}
				}
				map_index=map_index-nullIndex;
				u16 bitmap_total=buffer_bit_current+(map_index)*8+3*8;//可变长度+bitmap长度+固定长度
				u16 total=buffer_current-buffer_last;
				if (bitmap_total<total||Remain<0)//到当前为止使用bitmap更具优势
				{					
					memcpy(&pmap[map_index], buffer_bitdata, (buffer_bit_current + 7) / 8);
					pmap[-1]=map_index;//设置bitmap大小
					pmap+=map_index+(buffer_bit_current+7)/8;//移动指针位置
					Remain+=total;
					buffer_current=buffer_last;
					buffer_bit_current&=0x7;//最后的一个字节占用了几个bit
					if(buffer_bit_current)
					{
						buffer_bit_current=8-buffer_bit_current;
						bitRemain-=buffer_bit_current;//额外占据的bit数需要被减掉
					}
				}
				else//使用非位图更具备优势
				{
					if (bitRemain>0)
					{
						bitRemain += bitmap_total;
					}
					pmap-=3;//bitmap 位置回退
					buffer_last=buffer_current;
				}
				map_index = 0;
				if (bitRemain<=0)//bit map 的长度已经耗尽结束搜索
				{
					break;
				}
				buffer_bit_current = 0;
				null_bit_num = 0;
			}
			else
			{
				if (map_inv_index!=0||map_index!=0)
				{
					bitRemain -= 8; //减掉下一个bitmap的长度
				}
			}
			map_inv_index=0;
			
			

			
		}
		else
		{

			if (bitRemain<=0)//bit map 的长度已经耗尽结束搜索
			{
				u16 bitmap_total=buffer_bit_current+(map_index)*8+3*8;//可变长度+bitmap长度+固定长度
				u16 total=buffer_current-buffer_last;
				map_index+=1;//到了结算时刻下面map_index代表bitmap的大小进行计算  因此map_index+1
				u8 nullIndex;
				for (nullIndex=0;nullIndex<3;nullIndex++)
				{
					if (pmap[map_index-1-nullIndex]!=0)//发现为0的bitmap需要移除
					{
					  	bitRemain+=8;
						break;
					}
				}
				if (bitmap_total<total||Remain<0)//到当前为止使用bitmap更具优势
				{
					memcpy(&pmap[map_index],buffer_bitdata,(buffer_bit_current+7)/8);
					pmap[-1]=map_index;//设置bitmap大小
					pmap+=map_index+(buffer_bit_current+7)/8;//移动指针位置
					Remain+=total;
					buffer_current=buffer_last;
				}
				else//使用非位图更具备优势
				{
					if (bitRemain>0)
					{
						bitRemain += bitmap_total;
					}
					pmap-=3;//bitmap 位置回退
					buffer_last=buffer_current;
				}
				break;
			}
		}

	}
	buffer_bit-=4;//buffer指针转到原来的位置
	buffer-=4;
	buffer_bit_current=pmap-buffer_bit;//此处用作存储bitmap帧的全长
	pmap=src_data;

	//优先发送bitmap的帧  此处求出剩余的字节数  用来存储非bitmap
	Remain=len-buffer_bit_current;
	buffer_current=(buffer_last+7)/8;//非bitmao的数据的长度字节
	
	if (buffer_current)//非位图具有一定长度
	{
		buffer_current+=4;//非map的总长度
		if (Remain > buffer_current)
		{
			*(u16*)&buffer[1]=buffer_current-3;//3个是头
			memcpy(pmap,buffer,buffer_current);
			pmap+=buffer_current;
		}
		else if (Remain>4+(item_len+7)/8)//可以放下至少一个 4= 1head + 2len +1type 
		{
			Remain=(Remain-4)*8/item_len;//能装多少个
			Remain=(Remain*item_len+7)/8+4;//非map帧的总长度
			*(u16*)&buffer[1]=Remain-3;
			memcpy(pmap,buffer,Remain);
			pmap+=Remain;
		}
	}
	if (buffer_bit_current>4)
	{
		*(u16 *)&buffer_bit[1] = buffer_bit_current - 3;
		memcpy(pmap,buffer_bit,buffer_bit_current);
		pmap+=buffer_bit_current;
	}
	FreePDU((PDU_BLOCK*) temp);
	return pmap-src_data;

}




void HrfOldUpRate(void)
{

	for (int tei = 0; tei <= MAX_TEI_VALUE; tei++)
	{
		if (g_pStaInfo[tei].used == CCO_USERD)
		{
			g_pStaInfo[tei].HrfNoCnt++;
			g_pStaInfo[tei].HrfBitmap<<=1;
			g_pStaInfo[tei].HrfSeq++;
			if (g_pStaInfo[tei].HrfNoCnt > HRF_OLD_PERIOD)
			{
				g_pStaInfo[tei].HrfUpRate = 0;
				g_pStaInfo[tei].HrfNoCnt = 0;
			}
		}
	}

}


void* NeighborFilter(int hplc_hrf,int send_num,int*bitmap_size)
{
	static u16 neighbor_cnt[50];//存储自己接收到对方的不同SNR的邻居个数neighbor_cnt[0]代表-10db neighbor_cnt[1]为-9db 依次类推
	static u8 nei_bitmap[MAX_BITMAP_SIZE];
	memset(neighbor_cnt,0,sizeof(neighbor_cnt));
	memset(nei_bitmap,0,sizeof(nei_bitmap));
	int total_dirsta=1;//直连节点个数  pco+直连sta
	int snr=0;
	*bitmap_size=0;
	for (int tei=TEI_STA_MIN;tei<TEI_CNT;tei++)
	{
		if (g_pStaInfo[tei].used != CCO_USERD)
		{
			continue ;
		}
		if (g_pStaInfo[tei].pco==CCO_TEI)
		{
			if (hplc_hrf)//hrf
			{
				if (g_pStaInfo[tei].HrfBitmap || g_StaStatusInfo[tei].firstJoinIn)
				{
				}
				else
				{
					continue ;
				}
			}
			else//hplc
			{			
				if(g_StaStatusInfo[tei].found_list_counter_this_route||g_StaStatusInfo[tei].found_list_count_last_route|| g_StaStatusInfo[tei].firstJoinIn)
				{
				}
				else
				{
					continue ;
				}
			}
			++total_dirsta;
			nei_bitmap[tei>>3]|=1<<(tei&0x7);
			*bitmap_size=tei;
			continue ;
		}
		
		//刚入网2个周期之内 或者最近接到了 该节点发出的帧 才可以判断是snr是否需要过滤
		if (hplc_hrf)//hrf
		{
			if (g_pStaInfo[tei].HrfBitmap || g_StaStatusInfo[tei].firstJoinIn)
			{
				snr=g_pStaInfo[tei].RfSnr;
			}
			else
			{
				continue ;
			}
		}
		else//hplc
		{			
			if(g_StaStatusInfo[tei].found_list_counter_this_route||g_StaStatusInfo[tei].found_list_count_last_route|| g_StaStatusInfo[tei].firstJoinIn)
			{
				snr = g_pStaInfo[tei].snr;
			}
			else
			{
				continue ;
			}
		}
		if (snr<=-10)
		{
			++neighbor_cnt[0];
		}
		else if (snr>=39)
		{
			++neighbor_cnt[49];
		}
		else
		{
			++neighbor_cnt[snr+10];
		}

	}

	if (send_num>total_dirsta)
	{
		send_num-= total_dirsta;
	}
	else
	{
		*bitmap_size=(*bitmap_size+7)/8;
		return nei_bitmap;//超过最大snr除了直连节点其他的都不可发送
	}
	snr=-100;
	for (int i = 49; i >= 0; i--)
	{
		send_num-=neighbor_cnt[i];
		if (send_num<=0)
		{
			snr=i-10;//返回snr的分位数
		}
	}
	int find_size=0;
	for (int tei=TEI_STA_MIN;tei<TEI_CNT;tei++)
	{
		if (g_pStaInfo[tei].used != CCO_USERD)
		{
			continue ;
		}
		if (nei_bitmap[tei>>3]&(1<<(tei&0x7)))
		{
			continue ;
		}
		if (hplc_hrf)//hrf
		{
			if(g_pStaInfo[tei].HrfBitmap|| g_StaStatusInfo[tei].firstJoinIn)
			{
				if (snr<g_pStaInfo[tei].RfSnr)
				{
					nei_bitmap[tei>>3]|=1<<(tei&0x7);
					find_size=tei;
				}
			}

		}
		else//hplc
		{			
			if(g_StaStatusInfo[tei].found_list_counter_this_route||g_StaStatusInfo[tei].found_list_count_last_route|| g_StaStatusInfo[tei].firstJoinIn)
			{
				if (snr<g_pStaInfo[tei].snr)
				{
					nei_bitmap[tei >> 3] |= 1 << (tei & 0x7);
					find_size = tei;
				}
			}

		}
	}
	if (find_size>*bitmap_size)
	{
		*bitmap_size = find_size;
	}
	*bitmap_size=(*bitmap_size+7)/8;
	return nei_bitmap;
}
//无线发送发现列表
void PLC_SendHrfFoundListAuto(void)
{
	u8 *pMsdu = NULL;
//    HPLC_PHASE send_phase = PHASE_DEFAULT;
	NODE_LIST_ITEM *item = NULL;
	LIST_STATION *station = NULL;

	u16 max_len = 520 - 4 - 4 //520-物理块体 crc24 crc32;
		#ifdef HPLC_CSG
		-4 -2                   //南网物理块头 短msdu
		#endif
		- sizeof(MAC_HEAD_ONE_HOP); 

	pMsdu = (u8 *)alloc_buffer(__func__, max_len);
	if (NULL == pMsdu)
		return;
	memset_special(pMsdu, 0, max_len);

	DISCOVER_RF_NODE_LIST_PACKET *pListFrame = (DISCOVER_RF_NODE_LIST_PACKET *)pMsdu;
	max_len-=sizeof(DISCOVER_RF_NODE_LIST_PACKET);
	lockStaInfo();

	USHORT msdu_len;




	static u8 seq = 0;

	pListFrame->Seq = seq;
	seq++;

	PLC_CopyCCOMacAddr(pListFrame->MacAddr, TRUE);

	item = (NODE_LIST_ITEM *)pListFrame->data;

	item->ItemType = 0; //站点属性
	item->LenType = 0; //长度为1字节
	item->Len = sizeof(LIST_STATION);
	station = (LIST_STATION *)item->data;
	max_len -= sizeof(LIST_STATION) + sizeof(NODE_LIST_ITEM);
	PLC_CopyCCOMacAddr(station->CcoMac, TRUE);
	station->PcoTei = 0;
	station->Role = ROLES_CCO;
	station->Level = 0;
	station->RfHop = 0;


	station->PcoUpRate = 100;
	station->PcoDownRate = 100;
	station->MinRate = 100;
	station->SendPeriod = g_pRelayStatus->cco_hrf_foundlist_interval_sec;
	station->OldPeriod = HRF_OLD_PERIOD;

/*
	CCO 无需上行路由条目
	item=(NODE_LIST_ITEM *)(station+1);
	item->ItemType=1;//站点路由信息
	item->LenType=0;
	item->Len=sizeof(LIST_ROUT_ITEM);
	LIST_ROUT_ITEM* rout = (LIST_ROUT_ITEM*)item->data;
	//上行路由条目
	rout->NextRout=GetRelayTEI(CCO_TEI);
	rout->RoutType = 3;
	max_len-=sizeof(LIST_ROUT_ITEM)+sizeof(NODE_LIST_ITEM);
*/
	int bitSize=0;
	u8* bitMap=NeighborFilter(1, 300,&bitSize);
#if defined(HPLC_CSG)
	max_len -= WiteNeighborInfo(1,1, (u8*)(station + 1), max_len,bitMap);
#else
	max_len -= WiteNeighborInfo(4,4, (u8*)(station + 1), max_len,bitMap);
#endif


	msdu_len = 520 - 4 - 4  
		#ifdef HPLC_CSG
		-4 -2                //南网物理块头  短msdu
		#endif
		- sizeof(MAC_HEAD_ONE_HOP) - max_len;

	//发送
	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 0;
	//send_param.slot_type = SLOT_TYPE_CSMA;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	send_param.send_phase = SEND_PHASE_ALL;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_DISCOVER_LIST;
	SOF_param.DstTei = TEI_BROADCAST;

	mac_param.DstTei = TEI_BROADCAST;
	mac_param.SendType = SENDTYPE_LOCAL;
	mac_param.SendLimit = 0;
	mac_param.HopCount = 1;
	mac_param.MsduType = MSDU_NET_MANAGER;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

#if defined(HPLC_CSG)
	mac_param.MsduType = MSDU_NEIGHBOR;  //南网的发现列表类型
	mac_param.Broadcast = BROADCAST_DRT_DOWN;
	mac_param.HeadType = MAC_HEAD_HOP;
	mac_param.Vlan = 0;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
#else
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.mac_flag = MACFLAG_HOP;
#endif

	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, BroadCastMacAddr, MAC_ADDR_LEN);


	send_param.link = FreamPathHrf;
	send_param.doubleSend = 0;
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	//g_pRelayStatus->found_list_counter_this_route++;

	HrfOldUpRate();

	unLockStaInfo();
	free_buffer(__func__, pMsdu);

}

//HRF 处理发现列表报文
void PLC_HandleHrfNodeList(u8 *pdata, s16 len,P_HPLC_PARAM para)
{
	DISCOVER_RF_NODE_LIST_PACKET *pDiscoverList = (DISCOVER_RF_NODE_LIST_PACKET *)pdata;
	len-=sizeof(DISCOVER_RF_NODE_LIST_PACKET);
	u8 mac_addr[MAC_ADDR_LEN];

	memcpy_swap(mac_addr, pDiscoverList->MacAddr, MAC_ADDR_LEN);

	P_STA_INFO pSta = PLC_GetStaInfoByMac(mac_addr);
	if (NULL == pSta)//入网采用HRF方式
	{
		//发送离线指示报文，送检必测项目
		//debug_str("发现列表触发离线指示\r\n");
		PLC_AddToOfflineList(mac_addr, OFFLINE_NOTIFY);
		return;
	}
	if (pSta->net_state!=NET_IN_NET)//已经离线并且不再白名单中
	{
		if(PLC_GetWhiteNodeSnByStaInfo(pSta)>=CCTT_METER_NUMBER||pSta->net_state == NET_OUT_NET)
		{
			PLC_AddToOfflineList(mac_addr, OFFLINE_NOTIFY);
			return;
		}
	}

	if (pSta->Link)
	{
		if (OK != PLC_ResetSilenceTime(para->SofSrcTei, TRUE))
			return;
	}

	P_STA_STATUS_INFO pStaStaus = PLC_GetStaStatusInfoByTei(para->SofSrcTei);
	if (NULL == pStaStaus)
		return;

	//u8 *endAddr = &pDiscoverList->data[len - sizeof(DISCOVER_RF_NODE_LIST_PACKET)];
	NODE_LIST_ITEM *item = (NODE_LIST_ITEM*)pDiscoverList->data;
	SetHrfBit(para->SofSrcTei, pDiscoverList->Seq);
//	u8 getAllInfo=0;
	while (len>(s16)sizeof(NODE_LIST_ITEM))
	{
		
		switch (item->ItemType)
		{
		case 0://站点属性信息
//			getAllInfo|=1<<0;
//			break;
		case 1://站点路由信息
//			getAllInfo|=1<<1;
			break;
		case 2://邻居节点信道
			{
				LIST_NEIGH *neighbor=NULL;
				u8 *endAddr=(u8 *)item + sizeof(NODE_LIST_ITEM);
				if (item->LenType)//2字节长度
				{
					endAddr += *(u16*)&item->Len+1;//多一个字节的长度
					neighbor=(LIST_NEIGH*)&item->data[1];//长度是2个字节
				}
				else
				{
					endAddr +=  item->Len;
					neighbor=(LIST_NEIGH*)item->data;
				}
				endAddr-=1;//非位图一个信息最少占据16bit 因此搜索到最后一个字节就意味着所搜结束了
				u8 type = neighbor->NeighType;
				u8 *data = neighbor->data;
				u16 startBit=0;
				
				while (data < endAddr)//邻居节点信道信息最低2个字节
				{
					data=UpNeighChannelInfo(para->SofSrcTei,type, data, &startBit,0, 0);
					if (data==NULL)
					{
						return;
					}
				}
				break;
			}
		case 3://邻居节点信道（位图版）
			{
				LIST_NEIGH *neighbor=NULL;
				u8 *endAddr=(u8 *)item + sizeof(NODE_LIST_ITEM);
				if (item->LenType)//2字节长度
				{
					endAddr += *(u16*)&item->Len+1;//多一个字节的长度
					neighbor=(LIST_NEIGH*)&item->data[1];//长度是2个字节
				}
				else
				{
					endAddr +=  item->Len;
					neighbor=(LIST_NEIGH*)item->data;
				}
				LIST_BIT_HEAD *head=(LIST_BIT_HEAD*)neighbor->data;
				u8 type = neighbor->NeighType;
				
				u16 startBit;
				u8 *data = head->data;
				while (data<endAddr)//邻居节点信道信息最低2个字节
				{
					data = &head->data[head->size];
					startBit=0;
					for (int i=0;i < head->size&&data<endAddr;i++)
					{
						if (head->data[i])
						{
							for (int j=0;j<8;j++)
							{
								if (head->data[i]&(1<<j))
								{
									u16 tei = head->startTei+(i<<3|j);
									data = UpNeighChannelInfo(para->SofSrcTei,type, data, &startBit, true, tei);
									if (data == NULL)//出现错误
									{
										return;
									}
								}
							}
							
						}
					}
					if (startBit)//有不完整的byte
					{
						data++;
					}
					head=(LIST_BIT_HEAD*)data;
				}
				break;
			}
		default:
			{
				return;
			}
		}
		len-=sizeof(NODE_LIST_ITEM);
		if (item->LenType)//2字节长度
		{
			if(*(u16*)&item->Len>520)//一个pb块的最大值
			{
				return;
			}
			len -= *(u16*)&item->Len+1;
			item = (NODE_LIST_ITEM *)((u8 *)item + sizeof(NODE_LIST_ITEM) + *(u16*)&item->Len+1);//多一个字节的长度
		}
		else
		{
			len -= item->Len;
			item = (NODE_LIST_ITEM *)((u8 *)item + sizeof(NODE_LIST_ITEM) + item->Len);
		}
	}
}
#endif



#ifndef DIS_FUNC
static int findBaseNtbIdx(int32_t ntb, phaseRecognizeType *pntbInfo)
{
	int diff;
	int idx = pntbInfo->Widx;
	uint32_t exitFlag = 0xffffffff;
	if (pntbInfo->Ridx == pntbInfo->Widx)
	{
		return MAX_ZC_RECOGNIZE_PHASE;
	}
	idx--;
	if (idx < 0)
	{
		idx = MAX_ZC_RECOGNIZE_PHASE - 1;
	}
	do
	{
		diff = pntbInfo->zc_ntb[idx] - ntb;
		diff = diff > 0 ? diff : -diff;
		if (diff < (NTB_FRE_PER_MS*3/2))
		{
			return idx;
		}

		if (idx == 0)
		{
			idx = MAX_ZC_RECOGNIZE_PHASE;
		}
		idx--;
		if (exitFlag > diff)
		{
			exitFlag = diff;
		}
		else
		{
			return MAX_ZC_RECOGNIZE_PHASE;
		}
	}while (idx != pntbInfo->Ridx);
	return MAX_ZC_RECOGNIZE_PHASE;
}
#endif//DIS_FUNC
//获取当前时间之前一段时间的 idx
static int findPerZcIdx(phaseRecognizeType *pntbInfo, u32 ntb)
{
	u32 i = 0;
	int ligth;
	for (i = 0; i < MAX_ZC_RECOGNIZE_PHASE; i++)
	{
		if (i == 0)
		{	
			ligth = MAX_ZC_RECOGNIZE_PHASE-1;
		}
		else
		{
			ligth = (u32)(i - 1) % MAX_ZC_RECOGNIZE_PHASE;
		}
		if (ntb-pntbInfo->zc_ntb[ligth]<(50*NTB_FRE_PER_MS)	
			&&pntbInfo->zc_ntb[i]-ntb <(50*NTB_FRE_PER_MS))
		{
			return i;
		}
	}
	return MAX_ZC_RECOGNIZE_PHASE;



//	int idx=pntbInfo->Widx-1;
//	if (idx<0)
//	{
//		idx=MAX_ZC_RECOGNIZE_PHASE-1;
//	}
//	int size=(u32)(pntbInfo->Widx-pntbInfo->Ridx)%MAX_ZC_RECOGNIZE_PHASE;
//	if (size<cnt)
//	{
//		*cnt=size;
//	}
//	idx-=*cnt;
//	if (idx<0)
//	{
//		idx+=MAX_ZC_RECOGNIZE_PHASE;
//	}
}
//static u8 zcType = 1; //0二分之一周期  1为一个电力线周期
extern phaseRecognizeType ZC_NTB[6]; //ZC_NTB 此处声明成一维数组方便运算
#ifndef DIS_FUNC
static u32 get_12BitDate(u8 *data, u16 *bit_pos)
{
	u32 vaile;
	if (*bit_pos % 8) //非整字节开始
	{
		vaile = data[*bit_pos / 8] >> 4;
		vaile |= data[*bit_pos / 8 + 1] << 4;
	}
	else
	{
		vaile = data[*bit_pos / 8];
		vaile |= data[*bit_pos / 8 + 1] << 8;
		vaile &= 0xfff;
	}
	*bit_pos += 12;
	vaile <<= 8;
	return vaile;
}
#endif//DIS_FUNC
u8 falut_phase[TEI_MAX];
void PLC_ResetPhaseFault(u16 tei)
{
	if (tei>TEI_MAX)
	{
		return;
	}
	falut_phase[tei] = 0;
}
bool PLC_PhaseFault(u16 tei,u8 phase)
{
	u8 cnt[3]={0,0,0};
	int i;
	if (tei>TEI_MAX)
	{
		return false;
	}
	for (i=0;i<3;i++)
	{
		u8 idx = (phase >> (i * 2)) & 0x3;
		if (idx)
		{
			--idx;
			++cnt[idx];
		}
	}
	for (i=0;i<3;i++)
	{
		if (cnt[i]>1)//有相同的相线
		{
			if (falut_phase[tei]!=(phase&0x3f))//该相线与上一次错误的结果不同
			{
				falut_phase[tei] = phase&0x3f;
				return false;
			}
		}
	}
	return true;
}
bool PLC_PhaseLost(P_STA_INFO pSta,u8 *sta_phase,u8 real_num)
{
	int i;
	if (pSta->phase_edge_ask || pSta->V2_7_get_edge)
	{
		if (pSta->phase_edge == pSta->phase_edge_vaile) //并非零火反接
		{
			return false;
		}
		if ((pSta->phy_phase&0x3f)==0)
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	u8 seg[3]={0,0,0};
	for (i = 0; i < 3; i++)
	{
		if (sta_phase[i])
		{
			seg[i]=1;
		}
	}
	for (i = 0; i < 3; i++)
	{
		u8 phase = (pSta->phy_phase >> (i * 2)) & 0x3;
		if (phase != 0)
		{
			sta_phase[phase-1] = 0;
			seg[i]=0;
		}
	}

	for (i = 0; i < 3; i++)
	{
		if (seg[i]!=0)
		{
			for (int j=0;j<3;j++)
			{
				if (sta_phase[j]!=0)
				{
					pSta->phy_phase |= sta_phase[j]<<(i*2);
					sta_phase[j]=0;
					break;
				}
			}
		}
	}
	return true;
}
//#define
//处理过零上报
//u32 diff_back[170];
int diff_idx=0;
//#define FIX_BUG_PHASE
void PLC_HandleZeroCrossReport(MAC_HEAD *pMacHead, u8 *pdata, u16 len)
{
#ifndef DIS_FUNC
	P_ZERO_CROSS_NTB_REPORT pZC = (P_ZERO_CROSS_NTB_REPORT)pdata;

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pZC->TEI);
	if (NULL == pSta)
		return;
	u8 up_phase_cnt=0;
	u8 sta_phase[3]={ 0, 0, 0 };
	u32 sta_zc_time;
	u32 sta_zc_num[3] = { 0, 0, 0 };
	#ifndef PHASE_ONLY_USE_FIRST
	uint32_t ntb_diff[6];
	#endif
	diff_idx=0;

	//已经识别相线不再使用过零告知识别
	if (pSta->geted_phase)
	{
		return;
	}
#if defined(HPLC_CSG)
	sta_zc_num[0] = pZC->TotalNum>1?pZC->TotalNum-1:0;
	sta_zc_num[1] = 0;
	sta_zc_num[2] = 0;
	up_phase_cnt = 1;
#else
	sta_zc_num[0] = pZC->Phase1Num;
	sta_zc_num[1] = pZC->Phase2Num;
	sta_zc_num[2] = pZC->Phase3Num;
	if (pZC->TotalNum==sta_zc_num[0])
	{
		sta_zc_num[0]--;
	}
	for (int i=0;i<3;i++)
	{
		if (sta_zc_num[i]!=0)
		{
			up_phase_cnt++;
		}
	}
#endif

	sta_zc_time = (pZC->BaseNTB << 8);
	u32 ntb_now = BPLC_GetNTB();

	u32 diff_time = NtbsBetween(ntb_now, sta_zc_time) / NTB_FRE_PER_MS;

	if (diff_time >= 16000)
	{
		debug_str(DEBUG_LOG_APP, "get phase net over lim, mac is %02x%02x%02x%02x%02x%02x\r\n",
			      pSta->mac[5], pSta->mac[4], pSta->mac[3], pSta->mac[2], pSta->mac[1], pSta->mac[0]);
		return;
	}
	else
	{
	}

#if 0
	if (sta_zc_num[1]!=0||sta_zc_num[2]!=0)
	{
		pSta->phy_phase=1<<7;
	}
#endif
	u16 idx[6];
	u16 bit_pos = 0;
	u32 diff;
#ifdef FIX_BUG_PHASE
	u32 first_diff;
	u8 zhixin_fix=0;
#endif
#ifndef PHASE_ONLY_USE_FIRST
	u8 upDummyData=0;
#endif	
	u8 all_phase=0;
#ifdef FIX_BUG_PHASE
	if (pSta->ver_factory_code[0]=='T'||pSta->ver_factory_code[0]=='X')//智芯bug 第二第三出线相位会在第一个差值还有第二个差值之间跳动 
	{
		zhixin_fix=1;
	}
#endif

	u8 last_phy_phase = pSta->phy_phase;
	pSta->phy_phase &= ~(0x7f);
	
	debug_str(DEBUG_LOG_APP, "phase a is %d, b is %d, c is %d\n\r", sta_zc_num[0], sta_zc_num[1], sta_zc_num[2]);
	for (u8 i = 0; i < 3; i++) //三个相线分别具有上升和下降沿
	{
		u8 j;
		u32 counter = 0;
		
		if (sta_zc_num[i] == 0)
			continue;
		sta_phase[i]=i+1;
		diff = get_12BitDate(pZC->Data, &bit_pos);
//		diff_back[diff_idx++]=diff;
#ifdef FIX_BUG_PHASE
		first_diff=diff;
#endif
		#ifndef PHASE_ONLY_USE_FIRST
		upDummyData=0;
		#endif
		sta_zc_time = (pZC->BaseNTB << 8) + diff;
		for (j = 0; j < 6; j++)
		{
			idx[j] = findBaseNtbIdx(sta_zc_time, &ZC_NTB[j]);
			if (idx[j] != MAX_ZC_RECOGNIZE_PHASE)
			{
//				if (((ZC_NTB[j].Widx - idx[j]) % MAX_ZC_RECOGNIZE_PHASE) < (sta_zc_num[i] - 1)) //不足够计算相线
//				{
//					idx[j] = MAX_ZC_RECOGNIZE_PHASE;
//				}
			}
			if (idx[j] == MAX_ZC_RECOGNIZE_PHASE)
			{
				counter++;
			}
			else
			{
				idx[j]++;
				debug_str(DEBUG_LOG_APP, "currect NTB is %x, phase NTB is %x, phase is %d, mac is %02x%02x%02x%02x%02x%02x\r\n",
					      ntb_now, sta_zc_time, j, pSta->mac[5], pSta->mac[4], pSta->mac[3], pSta->mac[2], pSta->mac[1], pSta->mac[0]);
				if (idx[j] == MAX_ZC_RECOGNIZE_PHASE)
				{
					idx[j] = 0;
				}
			}
		}
		if (counter == 6) //未找到任何的符合项
		{
			debug_str(DEBUG_LOG_APP, "currect NTB is %x, phase NTB is %x, No find phase is %d, mac is %02x%02x%02x%02x%02x%02x\r\n",
				      ntb_now, sta_zc_time, i, pSta->mac[5], pSta->mac[4], pSta->mac[3], pSta->mac[2], pSta->mac[1], pSta->mac[0]);
			bit_pos+=(sta_zc_num[i]-1)*12;
			continue;
		}


		counter = sta_zc_num[i] > MAX_ZC_NUM_PER_PHASE ? MAX_ZC_NUM_PER_PHASE : sta_zc_num[i];
		--counter;
		if (counter == 0)
		{
			continue;
		}
		#ifndef PHASE_ONLY_USE_FIRST
		for (u8 x = 0; x < 6; x++)
		{
			if (idx[x] == MAX_ZC_RECOGNIZE_PHASE)
			{
				ntb_diff[x] = 0x7fffffff;
			}
			else
			{
				ntb_diff[x] = 0;
			}
		}

#define COEFFICIENT_MS    100
		u8 half_per_cnt=0;
		u16 real_count=0;
		u8 period=0;//应对力合的请求整周期返回半周期的bug 
				// period为0表示返回整周期 1表时返回半周期
		/***兼容过零NTB告知中所有过零NTB差值完全一样的情况（力合STA）***/
        u8 same_diff_count = 0; //相同过零NTB差值的数量
        u8 compare_report_count = counter; //待比较的上报过零NTB差值告知数量
		u32 last_diff = 0;
		#endif
        /***************************************************************/
        
		for (j = 0; j < counter; j++)
		{

			diff = get_12BitDate(pZC->Data, &bit_pos);
			#ifndef PHASE_ONLY_USE_FIRST
            /*******前后过零NTB差值比较是否相同，并记录相同的个数********/
        	if (j == 0)
			    last_diff = diff;

			if (last_diff == diff) 
			{
				same_diff_count++;
			}

			last_diff = diff;
            /************************************************************/
            
//			diff_back[diff_idx++]=diff;
#ifdef FIX_BUG_PHASE
/*********************应对智芯bug*************************/
			if (diff == 0)
			{
				if (j<6)
				{
					pSta->phy_phase&=0xc0;
					pSta->phy_phase &= ~(0x3f); //某一项未识别
					return;
				}
				else
				{
					int remain=counter-1;//当前的j还未++
					counter = j;//已获取的数量是j
					for (;j<remain;j++)
					{
						diff = get_12BitDate(pZC->Data, &bit_pos);
//						diff_back[diff_idx++]=diff;
					}
					
				}
				break;
			}
/***********************bug B***************************/
			if(zhixin_fix
			   &&(j&1))
			{
				if (first_diff==diff)
				{
					diff = 500000;
					zhixin_fix=2;//
				}
				else
				{
					zhixin_fix=1;
				}
			}
/*******************end*****************************/

/********************应对力合bug************************/
			if (j == 0)
			{
				if (diff > 200000 && diff < 300000)//虽然本机询问的是整周期  但是sta回复的却是半周期
				{
					period=1;
				}
			}
			if (period)
			{
				if (j!=(counter-1))//不是最后一个数据
				{
					diff+= get_12BitDate(pZC->Data, &bit_pos);//两个半周期合并为1个整周期
//					diff_back[diff_idx++]=diff;
					j++;
					half_per_cnt++;
				}
				else//sta 返回了奇数个数据最后一个数据放弃计算
				{
					break;
				}
			}
			//规避深国电智芯方案中过零BUG 
			//出现一会首个过零点位置不对一会第二第三个过零点出现偏差的问题
			//此处会造成若首个过零位置不对 会导致我们识别出两个相同的相位（因为一旦偏差我们得到的首个过零点就会是500000） ?
			//若第二第三个点不对 我们无法识别这个相线偏差过大
			//解决方法 是若出现第一个bug进行 则重新询问一次 若出现第二个bug屏蔽头两相的数据
			else if (j<2)//深国电头两相数据错误
			{
				diff=500000;
			}
#endif
/******************end*******************************/
			sta_zc_time += diff;
			for (u8 x = 0; x < 6; x++)
			{
				if (idx[x] == MAX_ZC_RECOGNIZE_PHASE)
				{
					continue;
				}
				int32_t squ = sta_zc_time - ZC_NTB[x].zc_ntb[idx[x]];
				squ = squ > 0 ? squ : -squ;
				squ /= NTB_FRE_PER_US * COEFFICIENT_MS;
				squ *= squ;
//				if (zhixin_fix==2)//已经确定是智芯的bug
				{
					if (ZC_NTB[x].Widx == idx[x])
					{
						upDummyData=3;
					}
					
				}
				if (upDummyData!=3)
				{
					ntb_diff[x] += squ;
					real_count++;
				}
				idx[x]++;
				if (idx[x] == MAX_ZC_RECOGNIZE_PHASE)
				{
					idx[x] = 0;
				}
			}
			#endif
		}
		#ifndef PHASE_ONLY_USE_FIRST
		if (period)
		{
			counter=half_per_cnt;
		}
		else if (counter!=real_count)
		{
			counter=real_count;
		}

        debug_str(DEBUG_LOG_APP, "phase:%d, same diff cnt:%d, report cnt:%d\n\r", i, same_diff_count, compare_report_count);

        /***兼容过零NTB告知中所有过零NTB差值完全一样的情况（力合STA）***/
        if (same_diff_count == compare_report_count) 
        {
            for (u8 x=0; x<6; x++)
    		{
    			if (idx[x] == MAX_ZC_RECOGNIZE_PHASE)
    			{
    				continue;
    			}

                ntb_diff[x] = 0; //对应ntb_diff直接赋值0，下面计算x对应的ntb_diff必然最小

                break;
    		}
        }
        /***************************************************************/

		for (u8 x = 0; x < 6; x++)
		{
			ntb_diff[x] /= counter;
		}
        
		u8 sta_phy_phase = 0;
		for (u8 x = 1; x < 6; x++) //找出最小的方差
		{
			if (idx[x] == MAX_ZC_RECOGNIZE_PHASE)
			{
				continue;
			}
            
			if (ntb_diff[0] > ntb_diff[x])
			{
			sta_phy_phase = x;
				ntb_diff[0] = ntb_diff[x];
		}
		}
		#else
		u8 sta_phy_phase = 0;
		for (u8 x = 0; x < 6; x++)
		{
			if (idx[x] == MAX_ZC_RECOGNIZE_PHASE)
			{
				continue;
			}
			sta_phy_phase = x;
		}
        #endif
		#ifndef PHASE_ONLY_USE_FIRST
		if (ntb_diff[0] < (1800 / COEFFICIENT_MS) * (1800 / COEFFICIENT_MS)) //方差在期望值里(1.2ms)
		#endif
		{
			pSta->phy_phase &= ~(3 << (i * 2));
			pSta->phy_phase |= (sta_phy_phase / 2 + 1) << (i * 2); //HPLC_PHASE
			debug_str(DEBUG_LOG_APP, "phase:%d, phy phase:0x%x\n\r", i, pSta->phy_phase);
			u8 edge=sta_phy_phase & 0x1; //记录是再上升沿得到的相位还是下降沿得到的相位
			
			if (all_phase)//获得了一相的相位
			{
				if (pSta->phase_edge_vaile != edge)
				{
					pSta->phy_phase |= 1 << 6; //只要有一相认为是边沿不同的就认为是零火反接
					debug_str(DEBUG_LOG_APP, "phase event diff edge\r\n");
				}
			}
			debug_str(DEBUG_LOG_APP, "edge %d-%d\n\r", edge, pSta->phase_edge_vaile);
			pSta->phase_edge_vaile = edge; //记录是再上升沿得到的相位还是下降沿得到的相位
			all_phase++;
		}


	}
	debug_str(DEBUG_LOG_APP, "phy phase is %x, cnt:%d-%d, edge:%d-%d-%d-%d\n\r", pSta->phy_phase, 
	          all_phase, up_phase_cnt, pSta->phase_edge_ask, pSta->V2_7_get_edge, pSta->phase_edge, pSta->phase_edge_vaile);
	if (all_phase != up_phase_cnt)
	{
		if(PLC_PhaseLost(pSta, sta_phase, all_phase))
		{
			debug_str(DEBUG_LOG_APP, "phy phase lost, phase is %x\n\r", pSta->phy_phase);
			if(pSta->phy_phase & 0x3f)
			{
				pSta->geted_phase = 1;
			}
			
			return;
		}
		pSta->phy_phase &= ~(0x7f); //某一项未识别
		return;
	}
	if(!PLC_PhaseFault(pZC->TEI,pSta->phy_phase))
	{
		pSta->phy_phase &= ~(0x7f); //相线识别发现重复的相线
	}

	if(pSta->phy_phase & 0x3f)
	{
		pSta->geted_phase = 1;
	}
	else
	{
		//本次识别结果出错，用之前的结果
		pSta->phy_phase = last_phy_phase;
	}

	debug_str(DEBUG_LOG_APP, "phy phase final is %x, last %x, geted phase:%d\n\r", pSta->phy_phase, last_phy_phase, pSta->geted_phase);
#endif //DIS_FUNC
}

void PLC_AppZeroCrossReport(u8 *pdata, u16 len, u8 collectionType)
{
#ifndef DIS_FUNC

#ifdef HPLC_CSG
	P_APP_ZERO_CROSS_NTB_REPORT pZC = (P_APP_ZERO_CROSS_NTB_REPORT)pdata;
#else
	P_AREA_DISCERNMENT_FEATURE_DATA pZC = (P_AREA_DISCERNMENT_FEATURE_DATA)pdata;
#endif
	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pZC->TEI);
	if (NULL == pSta)
		return;

	if (pSta->phase_edge_ask || pSta->V2_7_get_edge)
	{
		if (pSta->geted_phase)
		{
			return;
		}
	}
	
#ifdef HPLC_CSG
	u8 sta_phase[3]={ 0, 0, 0 };
	u32 sta_zc_num[3] = { 0, 0, 0 };
	u32 sta_zc_time;
	u8 *p = &pZC->Data[0];
	sta_zc_time = pZC->BaseNTB;
	sta_zc_num[0] = pZC->Phase1Num;
	sta_zc_num[1] = pZC->Phase2Num;
	sta_zc_num[2] = pZC->Phase3Num;
	if (sta_zc_num[0] == pZC->TotalNum)
	{
		sta_zc_num[0]--;
	}
	
#else
	
	u8 *p = &pZC->Other[0];
	
	p++; //保留


	p++;
	p++;
	p++;
#endif
	P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(pZC->TEI);

	pStaStatus->get_area_phase=1;//已经获得了2.7台区报文
#ifdef HPLC_CSG
	if (pZC->collectType == 1) //下降沿
	{
		pSta->phase_edge = 1;
		pSta->V2_7_get_edge = 1;
		debug_str(DEBUG_LOG_APP, "get edge neg, mac is %02x%02x%02x%02x%02x%02x\r\n",
			      pSta->mac[5], pSta->mac[4], pSta->mac[3], pSta->mac[2], pSta->mac[1], pSta->mac[0]);
	}
	else if (pZC->collectType == 2) //上升沿
	{
		pSta->phase_edge =  0;
		pSta->V2_7_get_edge = 1;
		debug_str(DEBUG_LOG_APP, "get edge reg, mac is %02x%02x%02x%02x%02x%02x\r\n",
			      pSta->mac[5], pSta->mac[4], pSta->mac[3], pSta->mac[2], pSta->mac[1], pSta->mac[0]);
	}
	else
	{
		pStaStatus->get_area_phase=0;
		return;
	}
#else
	if (pZC->CollectionType == 1) //下降沿
	{
		pSta->phase_edge = 1;
		pSta->V2_7_get_edge = 1;
		debug_str(DEBUG_LOG_APP, "get edge neg, mac is %02x%02x%02x%02x%02x%02x\r\n",
			      pSta->mac[5], pSta->mac[4], pSta->mac[3], pSta->mac[2], pSta->mac[1], pSta->mac[0]);
	}
	else if (pZC->CollectionType == 2) //上升沿
	{
		pSta->phase_edge =  0;
		pSta->V2_7_get_edge = 1;
		debug_str(DEBUG_LOG_APP, "get edge reg, mac is %02x%02x%02x%02x%02x%02x\r\n",
			      pSta->mac[5], pSta->mac[4], pSta->mac[3], pSta->mac[2], pSta->mac[1], pSta->mac[0]);
	}
	else
	{
		pStaStatus->get_area_phase=0;
		return;
	}
	return ;//国网目前修改为只从台区报文中获取边沿信息
#endif

#ifdef HPLC_CSG

	u8 all_phase=0,get_phase=0;
	uint32_t ntb_diff[6];
	if (sta_zc_num[0]==0
		&&sta_zc_num[1]==0
		&&sta_zc_num[2]==0)//此表不支持台区识别 无法使用台区识别进行 相线识别
	{
		if (pStaStatus == NULL)
		{
			return;
		}
//		pStaStatus->zc_read_counter = 0;
//		pStaStatus->zc_read_node = 0;
#ifdef HPLC_CSG
//		pStaStatus->first_unkonw_phase = 1;
#else
		pStaStatus->first_unkonw_phase = 1;
#endif
		return;
	}
	for (int i=0;i<3;i++)
	{
		if (sta_zc_num[i]!=0)
		{
			++all_phase;
		}
	}
	sta_zc_time = pZC->BaseNTB;

	u32 ntb_now = BPLC_GetNTB();

	u32 diff_time = NtbsBetween(ntb_now, sta_zc_time) / NTB_FRE_PER_MS;
	debug_str(DEBUG_LOG_APS, "zero@get node %d aps zero frame\r\n", pZC->TEI);
	if (diff_time >= 16000)
	{
		debug_str(DEBUG_LOG_APS, "zero@node %d base NTB over 10s\r\n", pZC->TEI);

		if (sta_zc_time == 0
			&& (!pSta->phy_phase & (1 << 7))) //南网非三相必须使用台区识别才能获得相线信息 过零只能识别单相
		{
			pStaStatus->first_unkonw_phase = 1;
		}
		else
		{
#ifdef HPLC_CSG
			pStaStatus->get_area_phase=0;
			pStaStatus->first_unkonw_phase = 0;
#endif
		}
		return;
	}
	else
	{
	}

	u16 idx[6];
	for (u8 i = 0; i < 3; i++) //三个相线分别具有上升和下降沿
	{
		u8 j;
		u32 counter = 0;
		u8 upDummyData=0;
		u16 real_count=0;
		if (sta_zc_num[i] == 0)
			continue;
		sta_phase[i]=i+1;
		//使用差值的第一个值来计算
#ifdef HPLC_CSG
		u32 diff = *(u16 *)p;
		if(collectionType == 7) //20200317标准
		{
			diff *= 16;
		}
		else //20191202标准
		{
			diff *= 8;
		}
		sta_zc_time = pZC->BaseNTB + diff;
#else
		s32 diff = *(s16 *)p;
		diff *= 8;
		diff += NTB_FRE_PER_MS * 20;
		sta_zc_time = pZC->NTB1 + diff;
#endif

		for (j = 0; j < 6; j++)
		{
			idx[j] = findBaseNtbIdx(sta_zc_time, &ZC_NTB[j]);
			if (idx[j] != MAX_ZC_RECOGNIZE_PHASE)
			{
				if (((u32)(ZC_NTB[j].Widx - idx[j]) % MAX_ZC_RECOGNIZE_PHASE) < (sta_zc_num[i] - 1)) //不足够计算相线
				{
					idx[j] = MAX_ZC_RECOGNIZE_PHASE;
				}
			}
			if (idx[j] == MAX_ZC_RECOGNIZE_PHASE)
			{
				counter++;
			}
			else
			{
				debug_str(DEBUG_LOG_APP, "currect NTB is %x, phase NTB is %x, phase is %d, mac is %02x%02x%02x%02x%02x%02x\r\n",
					      ntb_now, sta_zc_time, j, pSta->mac[5], pSta->mac[4], pSta->mac[3], pSta->mac[2], pSta->mac[1], pSta->mac[0]);
				idx[j]++;
				if (idx[j] == MAX_ZC_RECOGNIZE_PHASE)
				{
					idx[j] = 0;
				}
			}
		}
		if (counter == 6) //未找到任何的符合项
		{
			debug_str(DEBUG_LOG_APP, "currect NTB is %x, phase NTB is %x, No find phase is %d, mac is %02x%02x%02x%02x%02x%02x\r\n", 
				      ntb_now, sta_zc_time, i, pSta->mac[5], pSta->mac[4], pSta->mac[3], pSta->mac[2], pSta->mac[1], pSta->mac[0]);
			p += sta_zc_num[i] * 2;
			continue ;
		}


		counter = sta_zc_num[i] > MAX_ZC_NUM_PER_PHASE ? MAX_ZC_NUM_PER_PHASE : sta_zc_num[i];
		for (u8 x = 0; x < 6; x++)
		{
			if (idx[x] == MAX_ZC_RECOGNIZE_PHASE)
			{
				ntb_diff[x] = 0x7fffffff;
			}
			else
			{
				ntb_diff[x] = 0;
			}
		}
		counter--; //移除最后一个
		counter--; //移除第一个
		if (counter == 0 || counter > 100)
		{
			p += sta_zc_num[i] * 2;
			continue;
		}
#define COEFFICIENT_MS    100

#ifdef HPLC_CSG
		u16 *p_data = (u16 *)(p + 2);
#else
		s16 *p_data = (s16 *)(p + 2);
#endif

        /***兼容相位特征信息告知中所有过零NTB差值完全一样的情况（力合STA）***/
        u8 same_diff_count = 0; //相同过零NTB差值的数量
        u8 compare_report_count = counter; //待比较的上报过零NTB差值告知数量
		u32 last_diff = 0;
        /********************************************************************/
        
		for (j = 0; j < counter; j++)
		{

#ifdef HPLC_CSG
			u32 diff = *p_data++;
			if(collectionType == 7) //20200317标准
			{
				diff *= 16;
			}
			else //20191202标准
			{
				diff *= 8;
			}
#else			
			s32 diff = *p_data++;
			diff *= 8;
			diff += NTB_FRE_PER_MS * 20;
#endif

            /*******前后过零NTB差值比较是否相同，并记录相同的个数********/
			if (j == 0)
				last_diff = diff;

			if (last_diff == diff) //和前一个过零NTB差值比较是否相同
			{
				same_diff_count++;
			}

			last_diff = diff;
            /************************************************************/
            
			sta_zc_time += diff;
			for (u8 x = 0; x < 6; x++)
			{
				if (idx[x] == MAX_ZC_RECOGNIZE_PHASE)
				{
					continue;
				}
				int32_t squ = sta_zc_time - ZC_NTB[x].zc_ntb[idx[x]];
				squ = squ > 0 ? squ : -squ;
				squ /= NTB_FRE_PER_US * COEFFICIENT_MS;
				squ *= squ;


				if (ZC_NTB[x].Widx == idx[x])
				{
					upDummyData = 3;
				}
				if (upDummyData != 3)
				{
					ntb_diff[x] += squ;
					real_count++;
				}

				idx[x]++;
				if (idx[x] == MAX_ZC_RECOGNIZE_PHASE)
				{
					idx[x] = 0;
				}
			}
		}

        debug_str(DEBUG_LOG_APP, "PLC_AppZeroCrossReport, phase:%d, same diff cnt:%d, report cnt:%d\n\r", i, same_diff_count, compare_report_count);

        /***兼容相位特征信息告知中所有过零NTB差值完全一样的情况（力合STA）**
         (主要针对南网曾城超高压实验室深化应用台体，50Hz信号故意抖动的场景)*/
        if (same_diff_count == compare_report_count) 
        {
            for (u8 x=0; x<6; x++)
    		{
    			if (idx[x] == MAX_ZC_RECOGNIZE_PHASE)
    			{
    				continue;
    			}

                ntb_diff[x] = 0; //对应ntb_diff直接赋值0，下面计算x对应的ntb_diff必然最小

                break;
    		}
        }
        /******************************************************************/
        
		counter=real_count;
		for (u8 x = 0; x < 6; x++)
		{
			ntb_diff[x] /= counter;
			//debug_str(DEBUG_LOG_APS, "zero@phase %d variance is %d\r\n", x, ntb_diff[x]);
		}
		u8 sta_phy_phase = 0;
		for (u8 x = 1; x < 6; x++) //找出最小的方差
		{
			if (ntb_diff[0] > ntb_diff[x])
			{
				sta_phy_phase = x;
				ntb_diff[0] = ntb_diff[x];
			}

		}
		if (ntb_diff[0] < (1800 / COEFFICIENT_MS) * (1800 / COEFFICIENT_MS)) //方差在期望值里(1.2ms)
		{

			++get_phase;
			pSta->phy_phase &= ~(3 << (i * 2));
			pSta->phy_phase |= (sta_phy_phase / 2 + 1) << (i * 2); //HPLC_PHASE
			pSta->phase_edge_vaile=sta_phy_phase&0x1;//记录是再上升沿得到的相位还是下降沿得到的相位
#ifdef HPLC_CSG
			if (((sta_phy_phase & 1) + 1) == pZC->collectType) // sta_phy_phase奇是整周期 偶是半周期
#else
			if (((sta_phy_phase & 1) + 1) == pZC->CollectionType) // sta_phy_phase奇是整周期 偶是半周期
#endif
			{
				pSta->phy_phase |= 1 << 6; //零火反接
			}
		}
		else
		{
			debug_str(DEBUG_LOG_APS, "zero@can't find any phase, ntb diff:%d\r\n", ntb_diff[0]);
#ifdef HPLC_CSG			
			pStaStatus->get_area_phase=0;
			pStaStatus->first_unkonw_phase = 0;
#endif
		}

		p += sta_zc_num[i] * 2;
	}
	pStaStatus->get_area_phase=1;
	pStaStatus->first_unkonw_phase = 0;
	if (all_phase != get_phase)
	{
		if (PLC_PhaseLost(pSta, sta_phase, all_phase))
		{
			return;
		}
		pSta->phy_phase&=~(0x3f);//某一项未识别
#ifdef HPLC_CSG
			pStaStatus->get_area_phase=0;
			pStaStatus->first_unkonw_phase = 0;
#endif
		return;
	}
	if(!PLC_PhaseFault(pZC->TEI,pSta->phy_phase))
	{
		debug_str(DEBUG_LOG_APS, "zero@ PLC_PhaseFault\r\n");
		pSta->phy_phase &= ~(0x3f); //相线识别发现重复的相线
#ifdef HPLC_CSG
			pStaStatus->get_area_phase=0;
			pStaStatus->first_unkonw_phase = 0;
#endif
        return;
	}

	debug_str(DEBUG_LOG_APS, "zero@ geted phy phase:0x%x\r\n", pSta->phy_phase);
	pSta->geted_phase = 1;
	
#endif

#endif //end DIS_FUNC
}



PowerEventBitMap_s PowerEventBitMap[4];
#ifdef GDW_ZHEJIANG
u8 EventCollectMeterUpBitMap[128];
u8 EventCollectBitMap[128];
#endif
u16 OnUpTimerId = INVAILD_TMR_ID;
u16 OnDelUpTimerId = INVAILD_TMR_ID;
u16 OffUpTimerId = INVAILD_TMR_ID;
u16 OffDelUpTimerId = INVAILD_TMR_ID;
CPU_NTB_64 PowerOffTick=0;

//#define DEBUG_POWER_EVENT
#ifdef PROTOCOL_NW_2021_GUANGDONG
static u32 EventUpDate(PowerEventBitMap_s* pMap, u8* pbuf, u8 from, u8 eventype, u8 isIncludeStatus, u16 length, u8* meter_addr, u8* collect_event_flag)
{
    u32 *pHappen = (u32 *)pMap->happend;
    u32 *pUp = (u32 *)pMap->uped;
    int len = 0;
    u8 first_dev_type = 0;
    u16 coll_sn = 0;
	u8 coll_addr[LONG_ADDR_LEN] = {0};

#ifdef DEBUG_POWER_EVENT
    u8 index = 0;
#endif

    if (length > (MAC_ADDR_LEN + 1))
    {
        length -= MAC_ADDR_LEN + 1;
    }
    else
    {
        return 0;
    }

    debug_str(DEBUG_LOG_NET, "EventUpDate, from:%d, collect_event_flag:%d\r\n", from, *collect_event_flag);
    
    for (int i = 0; i < sizeof(pMap->happend) / 4; i++)
    {
        if (*pHappen ^ *pUp)
        {
            for (int j = 0; j < 32; j++)
            {
                u32 bit = 1 << j;
                if ((*pHappen & bit) && (!(*pUp & bit)))
                {
                    u16 sn = i << 5 | j;
                    if (from) //从白名单列表获取mac地址
                    {
                        if (g_pMeterRelayInfo[sn].status.used == CCO_USERD)
                        {               

                            if (!first_dev_type) //pMap中第一个未上报设备的类型
                            {
                                u8 dst_addr[LONG_ADDR_LEN] = {0};
                                PLC_GetAppDstMac(g_pMeterRelayInfo[sn].meter_addr, dst_addr);

                                debug_str(DEBUG_LOG_NET, "EventUpDate, first meter info, dev mac:%02x-%02x-%02x-%02x-%02x-%02x\r\n", 
                                          g_pMeterRelayInfo[sn].meter_addr[5], g_pMeterRelayInfo[sn].meter_addr[4], 
                                          g_pMeterRelayInfo[sn].meter_addr[3], g_pMeterRelayInfo[sn].meter_addr[2], 
                                          g_pMeterRelayInfo[sn].meter_addr[1], g_pMeterRelayInfo[sn].meter_addr[0]);
                                
                                P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
                                if (NULL != pSta)
                                {
                                    if ((pSta->dev_type == II_COLLECTOR) || (pSta->dev_type == I_COLLECTOR))
                                    {
                                        debug_str(DEBUG_LOG_NET, "EventUpDate, first dev collect, coll_sn:%d\r\n", g_pMeterRelayInfo[sn].coll_sn);
											
                                        //pMap中第一个未上报设备的类型是采集器，并且可以上报采集器事件
                                        if (*collect_event_flag)
                                        {
                                            coll_sn = g_pMeterRelayInfo[sn].coll_sn;
                                        }
                                    }
                                    else
                                    {
                                        *collect_event_flag = 0;
                                    }
                                }
                                else
                                {
                                    *collect_event_flag = 0;
                                }

                                first_dev_type = 1;
                            }
                            
                            if ((*collect_event_flag) && coll_sn) //采集器停复电上报
                            {
                                *pbuf++ = 1; //类型：采集器
                                ++len;
                            }
                            else
                            {
                                //过滤采集器下电表
                                u8 dst_addr[LONG_ADDR_LEN] = {0};
                                PLC_GetAppDstMac(g_pMeterRelayInfo[sn].meter_addr, dst_addr);
                                P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
                                if (NULL != pSta)
                                {
                                    if (pSta->dev_type==II_COLLECTOR || pSta->dev_type==I_COLLECTOR)
                                    {
                                        continue;
                                    }
                                }

                                *pbuf++ = 0; //类型：电能表
                                ++len;
                            }

                            *pUp |= bit;
                            
#ifdef DEBUG_POWER_EVENT
                            index++;
                            debug_str(DEBUG_LOG_NET, "EventUpDate, index:%d, meter dev mac:%02x-%02x-%02x-%02x-%02x-%02x\r\n", index, 
                                      g_pMeterRelayInfo[sn].meter_addr[5], g_pMeterRelayInfo[sn].meter_addr[4], 
                                      g_pMeterRelayInfo[sn].meter_addr[3], g_pMeterRelayInfo[sn].meter_addr[2], 
                                      g_pMeterRelayInfo[sn].meter_addr[1], g_pMeterRelayInfo[sn].meter_addr[0]);
#endif  
							PLC_GetAppDstMac(g_pMeterRelayInfo[sn].meter_addr, coll_addr);
                            memcpy(pbuf, coll_addr, MAC_ADDR_LEN);
                            pbuf += MAC_ADDR_LEN;
                            len += MAC_ADDR_LEN;

                            if (len >= length)
                            {
                                return len;
                            }
                        }
                        else
                        {
                            *pUp |= bit;
                        }
                    
                        if ((*collect_event_flag) && coll_sn) //20220308条文解释，采集器停复电载波上报只上报采集器信息，不会携带下挂电表信息，需要CCO汇总下挂电表信息    
                        {
                            for (int ii = 0; ii <  CCTT_METER_NUMBER; ii++)
                            {
                                if (g_MeterRelayInfo[ii].status.used != CCO_USERD)
                                    continue;

#ifdef PROTOCOL_NW_2023_GUANGDONG_V2DOT1 //广东23年深化应用V2.1，采集器复电事件携带下挂电表地址
                                if ((coll_sn == g_pMeterRelayInfo[ii].coll_sn) && g_pMeterRelayInfo[ii].power_status) //复电事件以MAC地址上报，根据带电状态过滤电表信息
#else  //20220308条文解释，采集器停复电载波上报只上报采集器信息，不会携带下挂电表信息，需要CCO汇总下挂电表信息 
                                if (coll_sn == g_pMeterRelayInfo[ii].coll_sn) //如果采集器以电表地址入网，该电表地址也需要填充到采集器下挂电表中上报
#endif                                
                                {
                                    //格式按先类型后电表地址

                                    *pbuf++ = 2; //类型：采集器下接电表
                                    len += 1;

                                    memcpy(pbuf, g_pMeterRelayInfo[ii].meter_addr, MAC_ADDR_LEN);
                                    pbuf += MAC_ADDR_LEN;
                                    len += MAC_ADDR_LEN;

                                    debug_str(DEBUG_LOG_NET, "EventUpDate, collect sn:%d, dev mac:%02x-%02x-%02x-%02x-%02x-%02x\r\n", coll_sn,  
                                              g_pMeterRelayInfo[ii].meter_addr[5], g_pMeterRelayInfo[ii].meter_addr[4], 
                                              g_pMeterRelayInfo[ii].meter_addr[3], g_pMeterRelayInfo[ii].meter_addr[2], 
                                              g_pMeterRelayInfo[ii].meter_addr[1], g_pMeterRelayInfo[ii].meter_addr[0]);
                                    
                                    g_pMeterRelayInfo[ii].power_status = 0; //上报后清除电表带电状态

									SET_STA_BITMAP(pMap->uped, ii);
                                    if (len >= length)
                                    {
                                        return len;
                                    }
                                }
                            }

                            //1个采集器的停复电信息独立上报
                            return len;
                        }
                    }
                    else //从STA_INFO 获取mac地址
                    {
                        P_STA_INFO pStaEvent = PLC_GetValidStaInfoByTei(sn);
                        if (pStaEvent != NULL)
                        {
                            if (!first_dev_type) //pMap中第一个未上报设备的类型
                            {
                                debug_str(DEBUG_LOG_NET, "EventUpDate, first sta info, dev mac:%02x-%02x-%02x-%02x-%02x-%02x, dev type:%d\r\n",
                                          pStaEvent->mac[5], pStaEvent->mac[4], pStaEvent->mac[3], pStaEvent->mac[2], pStaEvent->mac[1], pStaEvent->mac[0], pStaEvent->dev_type);
                                
                                if ((pStaEvent->dev_type == II_COLLECTOR) || (pStaEvent->dev_type == I_COLLECTOR))
                                {
                                    P_RELAY_INFO pNode = PLC_GetWhiteNodeInfoByMac(pStaEvent->mac);

                                    if (NULL != pNode)
                                    {
                                        *collect_event_flag = 1;
                                        
                                        coll_sn = pNode->coll_sn;
                                        debug_str(DEBUG_LOG_NET, "EventUpDate, sta info, first dev collect, coll_sn:%d\r\n", g_pMeterRelayInfo[sn].coll_sn);
                                    }
                                }

                                first_dev_type = 1;
                            }

                            if (coll_sn) //pMap中第一个还未上报的设备是采集器
                            {
                                *pUp |= bit;
                                
                                *pbuf++ = 1; //类型：采集器
                                ++len;                          

                                memcpy(pbuf, pStaEvent->mac, MAC_ADDR_LEN);
                                pbuf += MAC_ADDR_LEN;
                                len += MAC_ADDR_LEN;
                                
                                //20220308条文解释，采集器停复电载波上报只上报采集器信息，不会携带下挂电表信息，需要CCO汇总下挂电表信息       
                                for (int ii = 0; ii <  CCTT_METER_NUMBER; ii++)
                                {
                                    if (g_MeterRelayInfo[ii].status.used != CCO_USERD)
                                        continue;

                                    if (coll_sn == g_pMeterRelayInfo[ii].coll_sn) //如果采集器以电表地址入网，该电表地址也需要填充到采集器下挂电表中上报
                                    {
                                        //格式按先类型后电表地址
                                        *pbuf++ = 2; //类型：采集器下接电表
                                        len += 1;

                                        memcpy(pbuf, g_pMeterRelayInfo[ii].meter_addr, MAC_ADDR_LEN);
                                        pbuf += MAC_ADDR_LEN;
                                        len += MAC_ADDR_LEN;

                                        debug_str(DEBUG_LOG_NET, "EventUpDate, collect sn:%d, dev mac:%02x-%02x-%02x-%02x-%02x-%02x\r\n", coll_sn,  
                                                  g_pMeterRelayInfo[ii].meter_addr[5], g_pMeterRelayInfo[ii].meter_addr[4], 
                                                  g_pMeterRelayInfo[ii].meter_addr[3], g_pMeterRelayInfo[ii].meter_addr[2], 
                                                  g_pMeterRelayInfo[ii].meter_addr[1], g_pMeterRelayInfo[ii].meter_addr[0]);
                                        
                                        g_pMeterRelayInfo[ii].power_status = 0; //上报后清除电表带电状态
                                        
                                        if (len >= length)
                                        {
                                            return len;
                                        }
                                    }
                                }

                                //1个采集器的停复电信息独立上报
                                return len;
                            }
                            else
                            {
                                //过滤采集器下电表
                                if ((pStaEvent->dev_type == II_COLLECTOR) || (pStaEvent->dev_type == I_COLLECTOR))
                                {
                                    continue;
                                }

                                *pUp |= bit;
                                
                                *pbuf++ = 0; //类型：电能表
                                ++len;							

#ifdef DEBUG_POWER_EVENT
                                index++;
                                debug_str(DEBUG_LOG_NET, "EventUpDate, index:%d, sta dev mac:%02x-%02x-%02x-%02x-%02x-%02x\r\n", index, 
                                          pStaEvent->mac[5], pStaEvent->mac[4], pStaEvent->mac[3], 
                                          pStaEvent->mac[2], pStaEvent->mac[1], pStaEvent->mac[0]);
#endif

                                memcpy(pbuf, pStaEvent->mac, MAC_ADDR_LEN);
                                pbuf += MAC_ADDR_LEN;
                                len += MAC_ADDR_LEN;

                                if (len >= length)
                                {
                                    return len;
                                }
                            }
                        }
                        else
                        {
                            *pUp |= bit;
                        }
                    }
                }
            }
        }
        ++pHappen;
        ++pUp;
    }
    return len;
}
#else

#ifdef GDW_ZHEJIANG

static u32 EventCollectUpData(u8 *pbuf, u16 maxlength)
{
	u16 i,ii,j,jj,CollectTei,len;

	u8 *pMeterUpMap = &EventCollectMeterUpBitMap[0];
	u8 *pCollectorMap = &EventCollectBitMap[0];
        
    P_AFN06_F4_DEV_INFO pDevInfo = (P_AFN06_F4_DEV_INFO)(pbuf);
	P_AFN06_F4_NODE_INFO pNodeInfo = (P_AFN06_F4_NODE_INFO)pDevInfo->data;
	len = 1;//初始值长度加上从节点数量
	u8* p = NULL;

	for(i = 0; i < 128; i++)//寻找可上报的采集器并记录其SN
	{
		if((pCollectorMap[i]&0xff) != 0)
		{
			u16 UpCollectSN[8] = {0};
			u16 UpCollectCnt = 0;
			for(ii = 0; ii < 8; ii++)
			{
				u8 bit = 1 << ii;
				u8 tei = bit & pCollectorMap[i];
				if(tei != 0)
				{
					UpCollectSN[UpCollectCnt] = (i<<3) + ii;
					UpCollectCnt++;
				}
			}
#if 0
            for(j = 0; j < UpCollectCnt; j++)
#else
            j = 0;//考虑兼容性,为防一次上报多个从节点导致解析不了,每次只上报一个从节点
#endif
            {
                if(len > maxlength)//若长度超了则等待下一次上报该采集器消息
                {
                        return len;
                }
                pDevInfo->node_count++;
                
                CollectTei = UpCollectSN[j];
                P_STA_INFO psta=PLC_GetStaInfoByTei(CollectTei);

                memcpy_special(pNodeInfo->node_addr, psta->mac, 6);
                pNodeInfo->protocol = METER_PROTOCOL_3762_TRANSPARENT;
                pNodeInfo->node_sn = CollectTei;
                pNodeInfo->dev_type = 0;//采集器
                pNodeInfo->sub_node_count = psta->MeterNumConnect;
                pNodeInfo->trans_sub_node_count = 0;
                len += 12;
                p = (u8*)pNodeInfo->data;
                for(jj = 0; jj < CCTT_METER_NUMBER; jj++)
                {
                    u8 isUpedFlag = pMeterUpMap[jj>>3]&(1<<(jj&0x7));
                    if((g_MeterRelayInfo[jj].coll_sn == CollectTei)&&(isUpedFlag == 0)&&(g_MeterRelayInfo[jj].status.IsMeter == 0))//找到采集器下接的未上报的从节点
                    {
                        if(len >= maxlength)//找到下接从节点，但是长度超了则返回，等待下一次     上报剩余下接从节点
                        {
							return len;
                        }
                        if(pNodeInfo->trans_sub_node_count == 8)//一次报文最多上报下挂8个节点
                        {
                            break;
                        }
                        P_AFN06_F4_SUB_NODE_INFO pSubNode = (P_AFN06_F4_SUB_NODE_INFO)p;
                        memcpy(pSubNode->node_addr, g_pMeterRelayInfo[jj].meter_addr, MAC_ADDR_LEN); 
                        pSubNode->protocol = PLC_meterprotocol_to_3762protocol(g_MeterRelayInfo[jj].meter_type.protocol);
                        p+=7;
                        pMeterUpMap[jj>>3] |= 1<<(jj&0x7);//该集中器下挂从节点已上报,bitmap标志位置1
                        pNodeInfo->trans_sub_node_count++;
                        len+=7;
                    }
                    else if((g_MeterRelayInfo[jj].coll_sn == CollectTei)&&(g_MeterRelayInfo[jj].status.IsCollector == 0))
                    {
                        pNodeInfo->protocol = PLC_meterprotocol_to_3762protocol(g_MeterRelayInfo[jj].meter_type.protocol);
                    }
                }
                if(jj == CCTT_METER_NUMBER)//这样才是上报完一个采集器，否则是上报满8个下挂节点
                {
                    pCollectorMap[CollectTei>>3] &= ~(1<<(CollectTei&0x7));//该采集器上报完后将bitmap有表更新的标志位置0
                }
#if 0
                pNodeInfo = (P_AFN06_F4_NODE_INFO)p;
#else
                break;//考虑兼容性,为防一次上报多个从节点导致解析不了,每次只上报一个从节点
#endif
            }
			
		}
	}
	if(len == 1)//没有需要更新上报的内容了
	{
		return 0;
	}
        return len;

}

#endif

static u32 EventUpDate(PowerEventBitMap_s *pMap, u8 *pbuf, u8 from, u8 eventype, u8 isIncludeStatus, u16 length, u8* meter_addr, u8* collect_event_flag)
{
    u32 *pHappen = (u32 *)pMap->happend;
    u32 *pUp = (u32 *)pMap->uped;
    int len = 0;
    u8 meter_get_flag = 0;
    u8 first_dev_type = 0;
#ifndef HPLC_CSG
    u16 coll_sn = 0;
#endif

    if (length > (MAC_ADDR_LEN + 1))
    {
        length -= MAC_ADDR_LEN + 1;
    }
    else
    {
        return 0;
    }

    debug_str(DEBUG_LOG_NET, "EventUpDate, from:%d, collect_event_flag:%d\r\n", from, *collect_event_flag);
    
    for (int i = 0; i < sizeof(pMap->happend) / 4; i++)
    {
        if (*pHappen ^ *pUp)
        {
            for (int j = 0; j < 32; j++)
            {
                u32 bit = 1 << j;
                if ((*pHappen & bit) && (!(*pUp & bit)))
                {
                    u16 sn = i << 5 | j;
                    if (from) //从白名单列表获取mac地址，采集器停复电事件只有MAC地址形式上报
                    {
                        if (g_pMeterRelayInfo[sn].status.used == CCO_USERD)
                        {               

                            if (!first_dev_type) //pMap中第一个未上报设备的类型
                            {
                                u8 dst_addr[LONG_ADDR_LEN] = {0};
                                PLC_GetAppDstMac(g_pMeterRelayInfo[sn].meter_addr, dst_addr);

                                debug_str(DEBUG_LOG_NET, "EventUpDate, first dev collect, mac:%02x-%02x-%02x-%02x-%02x-%02x\r\n", 
                                          g_pMeterRelayInfo[sn].meter_addr[5], g_pMeterRelayInfo[sn].meter_addr[4], 
                                          g_pMeterRelayInfo[sn].meter_addr[3], g_pMeterRelayInfo[sn].meter_addr[2], 
                                          g_pMeterRelayInfo[sn].meter_addr[1], g_pMeterRelayInfo[sn].meter_addr[0]);
                                
                                P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
                                if (NULL != pSta)
                                {
                                    if (pSta->dev_type==II_COLLECTOR || pSta->dev_type==I_COLLECTOR)
                                    {
                                        debug_str(DEBUG_LOG_NET, "EventUpDate, first dev collect, coll_sn:%d\r\n", g_pMeterRelayInfo[sn].coll_sn);

#ifndef HPLC_CSG										
                                        //pMap中第一个未上报设备的类型是采集器，并且可以上报采集器事件
                                        if (*collect_event_flag)
                                        {
                                            coll_sn = g_pMeterRelayInfo[sn].coll_sn;
                                        }
#endif
                                    }
                                    else
                                    {
                                        *collect_event_flag = 0;
                                    }
                                }
                                else
                                {
                                    *collect_event_flag = 0;
                                }

                                first_dev_type = 1;
                            }
                            
                            if (!meter_get_flag)
                            {
                                //获取mac地址，填1376.2地址域用
                                memcpy(meter_addr, g_pMeterRelayInfo[sn].meter_addr, MAC_ADDR_LEN);
                                meter_get_flag = 1;
                            }

                            if (*collect_event_flag) //采集器停复电上报
                            {
#ifndef HPLC_CSG
                                if (coll_sn != g_pMeterRelayInfo[sn].coll_sn)
                                {
                                    continue;
                                }
#else
                                //南网PowerEventBitMap[2]掉电的mac 一定是采集器上报的
                                if (pMap != &PowerEventBitMap[3])
                                {
                                    //其他的mac需要判断是否是采集器
                                    u8 dst_addr[LONG_ADDR_LEN] = { 0 };
                                    PLC_GetAppDstMac(g_pMeterRelayInfo[sn].meter_addr, dst_addr);
                                    P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
                                    if (NULL != pSta)
                                    {
                                        if (pSta->dev_type!=II_COLLECTOR && pSta->dev_type!=I_COLLECTOR)
                                        {
                                            continue;
                                        }
                                    }
                                }
#endif
#ifdef ENENT_LOCAL_FRAME_TYPE
								*pbuf++ = 2; //类型：采集器
                                ++len;
#endif
                            }
                            else
                            {
                                //过滤采集器下电表
                                u8 dst_addr[LONG_ADDR_LEN] = {0};
                                PLC_GetAppDstMac(g_pMeterRelayInfo[sn].meter_addr, dst_addr);
                                P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
                                if (NULL != pSta)
                                {
                                    if (pSta->dev_type==II_COLLECTOR || pSta->dev_type==I_COLLECTOR)
                                    {
                                        continue;
                                    }
                                }
#ifdef ENENT_LOCAL_FRAME_TYPE
                                *pbuf++ = 0; //类型：电能表
                                ++len;
#endif
                            }

                            *pUp |= bit;
                            
                            memcpy(pbuf, g_pMeterRelayInfo[sn].meter_addr, MAC_ADDR_LEN);
                            pbuf += MAC_ADDR_LEN;
                            len += MAC_ADDR_LEN;
#ifdef HPLC_CSG
#ifndef ENENT_LOCAL_FRAME_TYPE
                            if (*collect_event_flag)
                            {
								*pbuf++ = 2; //类型：采集器
                                ++len;
                            }
                            else
                            {
                                *pbuf++ = 0; //类型：电能表
                                ++len;
                            }
#endif							
#else
                            if (*collect_event_flag)
                            {
                                *pbuf++ = g_pMeterRelayInfo[sn].power_status; //采集器停复电上报电表带电状态
                                ++len;

                                g_pMeterRelayInfo[sn].power_status = 0; //上报后清除电表带电状态
                            }
                            else if (isIncludeStatus)
                            {
                                *pbuf++ = eventype;
                                ++len;
                            }
#endif
                            if (len >= length)
                            {
                                return len;
                            }
                        }
                        else
                        {
                            *pUp |= bit;
                        }
                    }
                    else //从STA_INFO 获取mac地址
                    {
                        *pUp |= bit;
                        
                        P_STA_INFO pStaEvent = PLC_GetValidStaInfoByTei(sn);
                        if (pStaEvent != NULL)
                        {
#ifdef ENENT_LOCAL_FRAME_TYPE
                            if (*collect_event_flag)
                            {
                                *pbuf++ = 2; //类型：采集器
                                ++len;
                            }
                            else
                            {
                                *pbuf++ = 0; //类型：电能表
                                ++len;
                            }
#endif							

                            if (!meter_get_flag)
                            {
                                //获取mac地址，填1376.2地址域用
                                memcpy(meter_addr, pStaEvent->mac, MAC_ADDR_LEN);
                                meter_get_flag = 1;
                            }

                            memcpy(pbuf, pStaEvent->mac, MAC_ADDR_LEN);
                            pbuf += MAC_ADDR_LEN;
                            len += MAC_ADDR_LEN;
#ifdef HPLC_CSG
#ifndef ENENT_LOCAL_FRAME_TYPE

                            if (*collect_event_flag)
                            {
                                *pbuf++ = 2; //类型：采集器
                                ++len;
                            }
                            else
                            {
                                *pbuf++ = 0; //类型：电能表
                                ++len;
                            }
#endif							
#else
                            if (isIncludeStatus)
                            {
                                *pbuf++ = eventype;
                                ++len;
                            }
#endif
                            if (len >= length)
                            {
                                return len;
                            }
                        }
                    }
                }
            }
        }
        ++pHappen;
        ++pUp;
    }
    return len;
}
#endif

void PowerOnUp3762(void *arg)
{
//	OS_ERR err;
#define MAX_ALLOC_UP_LEN 200
	if (!arg) //NULL需要上报
	{
		u8 meter_addr[6] = {0};
		OS_TICK_64 tick_now=GetTickCount64();
		if (tick_now<(OS_CFG_TICK_RATE_HZ*60))
		{
			OnUpTimerId = HPLC_RegisterTmr(PowerOnUp3762, NULL, 10 * 1000, TMR_OPT_CALL_ONCE);
			return;
		}
		u8 *pbuf = alloc_buffer(__func__, MAX_ALLOC_UP_LEN);
		OnUpTimerId = INVAILD_TMR_ID;
		if (pbuf == NULL)
		{
			OnUpTimerId = HPLC_RegisterTmr(PowerOnUp3762, NULL, 10 * 1000, TMR_OPT_CALL_ONCE);
			free_buffer(__func__,pbuf);
			return;
		}
//		while (0 != OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz, OS_OPT_PEND_NON_BLOCKING, NULL, &err));
//		if (err == OS_ERR_NONE)
		{
			int len = 1;
			u8 collect_event_flag = 0;
			
			len += EventUpDate(&PowerEventBitMap[1], &pbuf[len], 0, 0, 0, MAX_ALLOC_UP_LEN - 1, meter_addr, &collect_event_flag); //sta info

#ifdef PROTOCOL_NW_2021_GUANGDONG
            if (collect_event_flag == 0) //没有获取到采集器的tei复电上报，理论上不会出现（复电都是mac地址）
#endif
            {         
    			if (len == 1) //没有获取到普通电表模块上电事件
    			{
    				collect_event_flag = 1;
    			}
    			
    			len += EventUpDate(&PowerEventBitMap[3], &pbuf[len], 1, 0, 0, MAX_ALLOC_UP_LEN - 1 - len, meter_addr, &collect_event_flag); //白名单
            }

            if (len != 1)
			{
#ifdef HPLC_CSG
				*pbuf = 0x82; //上电
#else
				*pbuf = 2; //上电
#endif

//				OSSemSet(&g_pOsRes->savedata_mbox, 0, &err);
#ifdef OVERSEA
				PLM_ReportMeterEvent(EVENT_DEVICE_TYPE_HPLC, METER_PROTOCOL_3762_POWER_EVENT, true, meter_addr, pbuf, len);
#else
				EM_EVENT_DEVICE_TYPE dev_type = EVENT_DEVICE_TYPE_HPLC;
				if (collect_event_flag == 1)
				{
					dev_type = EVENT_DEVICE_TYPE_COLLECTION;
				}
				PLM_ReportMeterEvent(dev_type, METER_PROTOCOL_3762_POWER_EVENT, false, NULL, pbuf, len);
#endif
//				OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz / 2, OS_OPT_PEND_BLOCKING, NULL, &err);
				if (len < MAX_ALLOC_UP_LEN - 8)
				{
					OnUpTimerId = HPLC_RegisterTmr(PowerOnUp3762, NULL, 10 * 1000, TMR_OPT_CALL_ONCE);
				}
				else //有后续帧
				{
					OnUpTimerId = HPLC_RegisterTmr(PowerOnUp3762, NULL, 2 * 1000, TMR_OPT_CALL_ONCE);
				}
			}
			free_buffer(__func__, pbuf);
			if (OnDelUpTimerId == INVAILD_TMR_ID)
			{
				OnDelUpTimerId = HPLC_RegisterTmr(PowerOnUp3762, (void *)1, POWER_EVENT_REMOVE_TIME*60*1000, TMR_OPT_CALL_ONCE); //POWER_EVENT_REMOVE_TIME时间后删除已经上报的节点
			}
			else
			{
				HPLC_DelTmr(OnDelUpTimerId);
				OnDelUpTimerId = HPLC_RegisterTmr(PowerOnUp3762, (void *)1, POWER_EVENT_REMOVE_TIME*60*1000, TMR_OPT_CALL_ONCE); //POWER_EVENT_REMOVE_TIME时间后删除已经上报的节点
			}
		}
//		else
//		{
//			upTimerId=HPLC_RegisterTmr(PowerUp3762, NULL, 2 * 1000, TMR_OPT_CALL_ONCE);
//		}
	}
	else
	{
		OnDelUpTimerId = INVAILD_TMR_ID;
		CPU_SR_ALLOC();
		CPU_CRITICAL_ENTER();

		for (int j = 0; j < sizeof(PowerEventBitMap[1].happend); j++)
		{
			PowerEventBitMap[1].happend[j] &= ~PowerEventBitMap[1].uped[j];
			PowerEventBitMap[1].uped[j] = 0;
		}
		for (int j = 0; j < sizeof(PowerEventBitMap[3].happend); j++)
		{
			PowerEventBitMap[3].happend[j] &= ~PowerEventBitMap[3].uped[j];
			PowerEventBitMap[3].uped[j] = 0;
		}
		CPU_CRITICAL_EXIT();
	}
}

#ifdef GDW_ZHEJIANG
void PLC_EventCollectReport()
{
	P_PLC_EVENT_COLLECT_REPORT_CB pcb = &g_pRelayStatus->event_collect_report_cb;
	if(pcb->updatecnt == 0)
	{
		pcb->updatecnt++;//每当有节点更新则++
		pcb->tick_send = GetTickCount64()+(EVENT_COLLECT_REPORT_INTERVAL * OS_CFG_TICK_RATE_HZ);
	}

	PLC_PostMessage(MSG_EVENT_COLLECT_REPORT, 0, 0);
	
	return; 
}

void PLC_OnlineLockTimeTime()
{
	PLC_PostMessage(MSG_ONLINE_TIME_LOCK, 0, 0);
	
	return; 
}

void PLC_NewStaJoinOnlineLockTimeSetTimeDelay()
{
	P_PLC_ONLINE_LOCK_TIME_CB pcb = &g_pRelayStatus->online_lock_time_cb;
	if(pcb->lockTimeFlag)
	{
		pcb->send_counter = 3;
		pcb->second_down = 20;//新站点上来，等待20s发送广播在网锁定时间报文
		debug_str(DEBUG_LOG_INFO, "新站点入网开始发送在网锁定时间报文，剩余次数=%d\r\n", pcb->send_counter);
	}
	return;
}

u8 PLC_OnlineLockTimeTimeProc(OS_TICK_64 tick_now)
{
	P_PLC_ONLINE_LOCK_TIME_CB pcb = &g_pRelayStatus->online_lock_time_cb;

	if (EM_ONLINE_LOCK_TIME_TIME_INIT == pcb->state)
	{

	}
	else if (EM_ONLINE_LOCK_TIME_TIME_START == pcb->state)
	{
		debug_str(DEBUG_LOG_INFO, "开始发送在网锁定时间报文，剩余次数=%d\r\n", pcb->send_counter);
		HPLC_SendOnlineTimeLock();
		pcb->tick_send = tick_now;
		if (pcb->send_counter)
			pcb->send_counter--;
		pcb->state = EM_ONLINE_LOCK_TIME_TIME_WAIT;
		
		return OK;
	}
	else if (EM_ONLINE_LOCK_TIME_TIME_WAIT == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->tick_send) >= 1*OSCfg_TickRate_Hz) //1秒重传
		{
			if (pcb->send_counter)
			{
				pcb->state = EM_ONLINE_LOCK_TIME_TIME_START;
			}
			else
				pcb->state = EM_ONLINE_LOCK_TIME_TIME_INIT;

		}
		
		return OK;
	}

	return OK;
}

void EventCollectUp3762()
{
#define MAX_ALLOC_EVENT_UP_LEN 200

	u8 *pbuf = alloc_buffer(__func__, MAX_ALLOC_EVENT_UP_LEN);

	if (pbuf == NULL)
	{
		free_buffer(__func__, pbuf);
		return;
	}
	int len = 0;
	do 
	{
		len = 0;
		memset(pbuf, 0x00, MAX_ALLOC_EVENT_UP_LEN);
		len += EventCollectUpData(&pbuf[0], MAX_ALLOC_EVENT_UP_LEN - 1);
		//3762协议规定最大长度65535，空间充足，一次可以上报完
		if(len != 0)//上报集中器
		{
			PLM_ReportCollectSubNodeInfoAndDev(&pbuf[0], len);
		}
	} while (len != 0);
	free_buffer(__func__, pbuf);

	for (int j = 0; j < sizeof(EventCollectMeterUpBitMap); j++)
	{
		EventCollectMeterUpBitMap[j] = 0;
	}
	for (int i = 0; i < sizeof(EventCollectBitMap);i++)
	{
		EventCollectBitMap[i] = 0;
	}
}

u8 PLC_EventCollectReportTimeProc(OS_TICK_64 tick_now)
{

	P_PLC_EVENT_COLLECT_REPORT_CB pcb = &g_pRelayStatus->event_collect_report_cb;

	if (EM_EVENT_COLLECT_REPORT_TIME_INIT == pcb->state)
	{
          

	}
	else if (EM_EVENT_COLLECT_REPORT_TIME_START == pcb->state)
	{
		if ((pcb->updatecnt)&&(pcb->tick_send != 0))//有节点更新
		{
			if(tick_now > pcb->tick_send)//5s时间到
			{
				EventCollectUp3762();//上报集中器
				pcb->updatecnt = 0;//上报结束后
				pcb->tick_send = 0;
				pcb->state = EM_EVENT_COLLECT_REPORT_TIME_INIT;
			}
		}
		
		return OK;
	}
	return OK;
}

#endif

void PowerOffUp3762(void *arg)
{
//	OS_ERR err;
#define MAX_ALLOC_UP_LEN 200
	if (!arg) //NULL需要上报
	{
		u8 meter_addr[6] = {0};
		OS_TICK_64 now = GetTickCount64();
		if (now<(OS_CFG_TICK_RATE_HZ*60))
		{
			OnUpTimerId = HPLC_RegisterTmr(PowerOffUp3762, NULL, 4 * 1000, TMR_OPT_CALL_ONCE);
			return;
		}
		if (now<PowerOffTick)
		{
			OffUpTimerId = HPLC_RegisterTmr(PowerOffUp3762, NULL, 4 * 1000, TMR_OPT_CALL_ONCE);
			return;
		}
		u8 *pbuf = alloc_buffer(__func__, MAX_ALLOC_UP_LEN);
		OffUpTimerId = INVAILD_TMR_ID;
		if (pbuf == NULL)
		{
			OffUpTimerId = HPLC_RegisterTmr(PowerOffUp3762, NULL, 10 * 1000, TMR_OPT_CALL_ONCE);
			free_buffer(__func__, pbuf);
			return;
		}
//		while (0 != OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz, OS_OPT_PEND_NON_BLOCKING, NULL, &err));
//		if (err == OS_ERR_NONE)
		{
			int len = 1;
			u8 collect_event_flag = 0;
			
			len += EventUpDate(&PowerEventBitMap[0], &pbuf[len], 0, 1, 0, MAX_ALLOC_UP_LEN - 1, meter_addr, &collect_event_flag); //sta info

#ifdef PROTOCOL_NW_2021_GUANGDONG
            if (collect_event_flag == 0) //没有获取到采集器的tei停电上报
#endif
            {         
    			if (len == 1) //没有获取到普通电表模块停电事件
    			{
    				collect_event_flag = 1;
    			}
    			
    			len += EventUpDate(&PowerEventBitMap[2], &pbuf[len], 1, 1, 0, MAX_ALLOC_UP_LEN - 1 - len, meter_addr, &collect_event_flag); //白名单
            }

			if (len != 1)
			{
#ifdef HPLC_CSG
				*pbuf = 0x81; //停电
#else
				*pbuf = 1; //停电
#endif

//				OSSemSet(&g_pOsRes->savedata_mbox, 0, &err);
#ifdef OVERSEA
				PLM_ReportMeterEvent(EVENT_DEVICE_TYPE_HPLC, METER_PROTOCOL_3762_POWER_EVENT, true, meter_addr, pbuf, len);
#else
				EM_EVENT_DEVICE_TYPE dev_type = EVENT_DEVICE_TYPE_HPLC;
				if (collect_event_flag == 1)
				{
					dev_type = EVENT_DEVICE_TYPE_COLLECTION;
				}
				PLM_ReportMeterEvent(dev_type, METER_PROTOCOL_3762_POWER_EVENT, false, NULL, pbuf, len);
#endif
//				OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz / 2, OS_OPT_PEND_BLOCKING, NULL, &err);
				if (len < MAX_ALLOC_UP_LEN - 8)
				{
					OffUpTimerId = HPLC_RegisterTmr(PowerOffUp3762, NULL, 10 * 1000, TMR_OPT_CALL_ONCE);
				}
				else
				{
					OffUpTimerId = HPLC_RegisterTmr(PowerOffUp3762, NULL, 2 * 1000, TMR_OPT_CALL_ONCE);
				}
			}
			free_buffer(__func__, pbuf);
			if (OffDelUpTimerId == INVAILD_TMR_ID)
			{
				OffDelUpTimerId = HPLC_RegisterTmr(PowerOffUp3762, (void *)1, POWER_EVENT_REMOVE_TIME*60*1000, TMR_OPT_CALL_ONCE); //POWER_EVENT_REMOVE_TIME时间后删除已经上报的节点
			}
			else
			{
				HPLC_DelTmr(OffDelUpTimerId);
				OffDelUpTimerId = HPLC_RegisterTmr(PowerOffUp3762, (void *)1, POWER_EVENT_REMOVE_TIME*60*1000, TMR_OPT_CALL_ONCE); //POWER_EVENT_REMOVE_TIME时间后删除已经上报的节点
			}
		}
//		else
//		{
//			upTimerId=HPLC_RegisterTmr(PowerUp3762, NULL, 2 * 1000, TMR_OPT_CALL_ONCE);
//		}
	}
	else
	{
		OffDelUpTimerId = INVAILD_TMR_ID;
		CPU_SR_ALLOC();
		CPU_CRITICAL_ENTER();

		for (int j = 0; j < sizeof(PowerEventBitMap[0].happend); j++)
		{
			PowerEventBitMap[0].happend[j] &= ~PowerEventBitMap[0].uped[j];
			PowerEventBitMap[0].uped[j] = 0;
		}
		for (int j = 0; j < sizeof(PowerEventBitMap[2].happend); j++)
		{
			PowerEventBitMap[2].happend[j] &= ~PowerEventBitMap[2].uped[j];
			PowerEventBitMap[2].uped[j] = 0;
		}
		CPU_CRITICAL_EXIT();
	}
}

void PLC_HandlesAppEvent(u16 SrcTei,u16 DstTei, P_EVENT_PACKET pEvent
#ifdef HPLC_CSG
						 , P_APP_BUSINESS_PACKET pBusinessPacket
#endif
						)
{
#ifndef HPLC_CSG
	if (0 == pEvent->Direction)
		return;

	if (0 == pEvent->StartFlag)
		return;

//    if(1!=pEvent->Function && 2!=pEvent->Function)
//        return;
#endif
	USHORT tei = TEI_CNT;
	P_APP_PACKET pAppFrame = NULL;

	u8 mac_addr[MAC_ADDR_LEN]={0x02,0x09,0x00,0x00,0x01,0x42};
	USHORT msdu_len = 0;
	EVENT_FUNC_CODE func_code = (EVENT_FUNC_CODE)0;
#if 0
	if(MACFLAG_INCLUDE==pMacHead->MACFlag)
	memcpy_swap(mac_addr, pMacHead->SrcMac, MAC_ADDR_LEN);
	else
	memcpy_swap(mac_addr, pEvent->MeterAddr, MAC_ADDR_LEN);
	tei = PLC_GetStaSnByMac(mac_addr);
#endif

	#ifdef ERROR_APHASE
	u8 isPhaseErr=0;
	#endif

	if (tei >= TEI_CNT)
	{
		tei = SrcTei;
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
		if (NULL == pSta)
		{
			#ifndef GDW_HEILONGJIANG
			goto PLC_HandlesAppEvent_Exit;
			#else
			tei=0;
			memcpy(mac_addr,pEvent->MeterAddr,MAC_ADDR_LEN);
			#endif
		}
		else
		{
			memcpy_special(mac_addr, pSta->mac, MAC_ADDR_LEN);
		}
	}

	if (SrcTei == CCO_TEI)
	{
		return;
	}




	//添加停电 上电事件
	//华为模块断电再上电会上报功能码是2的事件，表计地址全为0，转发数据长度4
#ifdef HPLC_CSG
	if (0 == pBusinessPacket->BusinessID)
	{
		P_APP_EVENT_REPORT_UP pEvent = (P_APP_EVENT_REPORT_UP)pBusinessPacket->Data;
		u16 event_len = pBusinessPacket->FrameLen - 6;
		u8 protocol_3762 = METER_PROTOCOL_3762_TRANSPARENT;

		P_RELAY_INFO pNode = PLC_GetWhiteNodeInfoByMac(pEvent->MeterAddr);
		if (NULL == pNode)
			goto PLC_HandlesAppEvent_Exit;

		protocol_3762 = PLC_meterprotocol_to_3762protocol(pNode->meter_type.protocol);

//		OS_ERR err;
		#if 0
		while (0 != OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz, OS_OPT_PEND_NON_BLOCKING, NULL, &err));
		#else
//		OSSemSet(&g_pOsRes->savedata_mbox, 0, &err);
		#endif
		if(g_PlmStatusPara.report_meter_event)//允许事件上报
		{
			if (!PLM_ReportMeterEvent(EVENT_DEVICE_TYPE_HPLC, protocol_3762, true, pEvent->MeterAddr, pEvent->Data, event_len))
			{
				func_code = EVENT_FUNC_DOWN_BUFFER_FULL;
			}
		}
		else
		{
			g_pRelayStatus->event_switch_cb.bitmap[tei >> 5] |= 1 << (tei & 0x1f);
		}
//		OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz / 5, OS_OPT_PEND_BLOCKING, NULL, &err);
	}
#else
	if (1 == pEvent->Function)       //电表事件
	{
		u8 protocol_3762 = METER_PROTOCOL_3762_TRANSPARENT;
		debug_str(DEBUG_LOG_NET, "event tei %x up\r\n", DstTei);
		P_RELAY_INFO pNode = NULL;
		if (memIsHex(pEvent->MeterAddr, 0x00, 6)) //电表地址为全0
		{
			pNode = PLC_GetWhiteNodeInfoByMac(mac_addr);
		}
		else
		{
			pNode = PLC_GetWhiteNodeInfoByMac(pEvent->MeterAddr);
		}
		if (NULL == pNode)
			goto PLC_HandlesAppEvent_Exit;

		protocol_3762 = PLC_meterprotocol_to_3762protocol(pNode->meter_type.protocol);

//		OS_ERR err;
		#if 0
		while (0 != OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz, OS_OPT_PEND_NON_BLOCKING, NULL, &err));
		#else
//		OSSemSet(&g_pOsRes->savedata_mbox, 0, &err);
		#endif
		//698电表，事件数据长度为0
		if(g_PlmStatusPara.report_meter_event)//允许事件上报
		{
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(SrcTei);
			EM_EVENT_DEVICE_TYPE dev_type=EVENT_DEVICE_TYPE_METER;//默认电能表
			if (pSta)
			{
				if (pSta->dev_type != METER
					&& pSta->dev_type != METER_3PHASE)
				{
					dev_type=EVENT_DEVICE_TYPE_COLLECTION;
				}
			}
			if ((protocol_3762 != METER_PROTOCOL_3762_07) && (protocol_3762 != METER_PROTOCOL_3762_97) && (pEvent->DataLen == 0))
			{
				u8 event698_data[30] = { 0 };
				u16 event698_data_len = 0;

				event698_data_len = PLC_make_frame_698_event_frame(event698_data, mac_addr);
				if (!PLM_ReportMeterEvent(dev_type, protocol_3762, true, mac_addr, event698_data, event698_data_len))
				{
					func_code = EVENT_FUNC_DOWN_BUFFER_FULL;
				}
			}
			else
			{
				if (!PLM_ReportMeterEvent(dev_type, protocol_3762, true, mac_addr, pEvent->Data, pEvent->DataLen))
				{
					func_code = EVENT_FUNC_DOWN_BUFFER_FULL;
				}
			}
		}
		else
		{
			g_pRelayStatus->event_switch_cb.bitmap[tei>>5]|=1<<(tei&0x1f);
		}
//		OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz / 2, OS_OPT_PEND_BLOCKING, NULL, &err);
	}
	#ifdef ERROR_APHASE  //陕西零线电流
	else if (2 == pEvent->Function)
	{
		EVENT_A_PHASE* pPhaseErr = (EVENT_A_PHASE*)pEvent->Data;
		if (6==pPhaseErr->EventType)
		{
			isPhaseErr=1;
			if (g_PlmConfigPara.a_phase_error) //允许零线异常事件上报
			{
				EM_EVENT_DEVICE_TYPE dev_type = EVENT_DEVICE_TYPE_HPLC;
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(SrcTei);
				if (pSta)
				{
					dev_type = pSta->moduleType == 0 ? EVENT_DEVICE_TYPE_HPLC : EVENT_DEVICE_TYPE_PLC_RF;
				}
			
				P_AFN06_F5_PHASE_HEAD upHead = (P_AFN06_F5_PHASE_HEAD)pPhaseErr->item;
				upHead--;
				u16 total = pPhaseErr->MeterNum;
				if (total>32)
				{
					total=32;
				}
				upHead->num = total;
				upHead->start_sn=1;
				upHead->total = total;
				if (!PLM_ReportMeterEvent(dev_type, METER_PROTOCOL_3762_PHASE_ERR, true, mac_addr, (u8*)upHead, sizeof(AFN06_F5_PHASE_HEAD) +total*sizeof(A_PHASE_ITEM)))
				{
					func_code = EVENT_FUNC_DOWN_BUFFER_FULL;
				}
				else
				{
					func_code = EVENT_FUNC_DOWN_OK;
				}
			}
			else
			{
				func_code=EVENT_FUNC_DOWN_REPORT_DISABLE;
			}
		}
	}
	#endif
#endif
	while (DstTei != TEI_BROADCAST) //广播不回复ack
	{
#ifdef HPLC_CSG
		//南网事件上报回复确认帧
		if (0 == pBusinessPacket->BusinessID)
		{
			if (func_code != EVENT_FUNC_DOWN_BUFFER_FULL)
			{
				pAppFrame = HplcApp_MakeConfirmFrame(0, pBusinessPacket->FrameSeq, 0, &msdu_len);
			}
			else
			{
				break;
			}
		}
		else
#endif
		{
#ifndef HPLC_CSG

			if (
				#ifdef ERROR_APHASE
				!isPhaseErr&&
				#endif
				func_code != EVENT_FUNC_DOWN_BUFFER_FULL)
			{
				if (0 == g_PlmStatusPara.report_meter_event)
					func_code = EVENT_FUNC_DOWN_REPORT_DISABLE;
				else
					func_code = EVENT_FUNC_DOWN_OK;
			}
			pAppFrame = HplcApp_MakeEventFrame(pEvent->Seq, func_code, EVENT_DIR_DOWN, EVENT_FROM_RSP, mac_addr, NULL, 0);
#else
			if (func_code == EVENT_FUNC_DOWN_BUFFER_FULL)
			{
				break;
			}
			pAppFrame = HplcApp_MakeEventFrame(pBusinessPacket->FrameSeq, (EVENT_FUNC_CODE)1, EVENT_DIR_DOWN, EVENT_FROM_RSP, mac_addr, NULL, 0);
			msdu_len = sizeof(APP_PACKET) - 1 + sizeof(APP_BUSINESS_PACKET) - 1 + sizeof(EVENT_PACKET) - 1;
#endif
		}
		if (NULL == pAppFrame)
			goto PLC_HandlesAppEvent_Exit;
#ifdef HPLC_CSG
		u8 *pMsdu = MacCreateMsdu(MAC_HEAD_SHORT, (u8 *)pAppFrame, msdu_len, &msdu_len);
		free_buffer(__func__, pAppFrame);
		pAppFrame = (P_APP_PACKET)pMsdu;
		if (NULL == pAppFrame)
			goto PLC_HandlesAppEvent_Exit;
#else
		msdu_len = sizeof(APP_PACKET) - 1 + sizeof(EVENT_PACKET) - 1 + 0;
#endif
		//发送

		PLC_SEND_PARAM send_param;
		SOF_PARAMS SOF_param;
		MAC_PARAM mac_param;
		#ifdef FORWARD_FRAME
		mac_param.SrcTei = CCO_TEI;
		#endif

		send_param.resend_count = 2;
		send_param.send_type = CSMA_SEND_TYPE_CSMA;
		send_param.cb = NULL;
		PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
		send_param.doubleSend=1;

		SOF_param.NID = g_pPlmConfigPara->net_id;
		SOF_param.BroadcastFlag = 0;
		SOF_param.Lid = SOF_LID_EVENT_CFG;

		mac_param.DstTei = tei;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.SendLimit = 3;
		mac_param.HopCount = MAX_RELAY_DEEP;
		mac_param.MsduType = MSDU_APP_FRAME;
		mac_param.Broadcast = BROADCAST_DRT_DUL;
		mac_param.NetSeq = g_pPlmConfigPara->net_seq;
		mac_param.mac_flag = MACFLAG_NONE;
#ifdef HPLC_CSG
		mac_param.HeadType = MAC_HEAD_SHORT;
		mac_param.Vlan = SOF_LID_EVENT_CFG;
		mac_param.NID = (u8)g_pPlmConfigPara->net_id;
#endif

		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

		debug_str(DEBUG_LOG_NET, "PLC_HandlesAppEvent, tei = %d\r\n", tei);

		if (0 == g_PlmStatusPara.report_meter_event)
		{
			debug_str(DEBUG_LOG_NET, "集中器禁止CCO上报事件\r\n");
			goto PLC_HandlesAppEvent_Exit;
		}
		break;
	}

	if (0 == g_PlmStatusPara.report_meter_event)
	{
		debug_str(DEBUG_LOG_NET, "集中器禁止CCO上报事件\r\n");
		goto PLC_HandlesAppEvent_Exit;
	}

#ifdef ERROR_APHASE
if(!isPhaseErr)
#endif
#ifdef HPLC_CSG
	if (1 == pBusinessPacket->BusinessID)  //停上电事件
#else
	if (2 == pEvent->Function || 3 == pEvent->Function)  //模块事件或者采集器事件
#endif
		{
			//待实现，采集器事件
			u8 update_flag = 0;
			P_EVENT_DATA_BITMAP pEventBitmap = (P_EVENT_DATA_BITMAP)pEvent->Data;
			u8 tpyeIdx = pEventBitmap->EventType - 1;
			if ((1 == pEventBitmap->EventType) || (2 == pEventBitmap->EventType))
			{
				u16 bitmap_size = pEvent->DataLen - 3;

				for (u16 i = 0; i < bitmap_size; i++)
				{
					if (pEventBitmap->EventBitmap[i])
					{
						for (u16 j = 0; j < 8; j++)
						{
							if (!(pEventBitmap->EventBitmap[i] & (1 << j)))
								continue;
							u16 tei_event = pEventBitmap->EventTEI + (i << 3 | j);
							if (PowerEventBitMap[tpyeIdx].happend[tei_event / 8] & (1 << (tei_event & 0x7)))
							{
								continue;
							}
							update_flag = 1;
							SET_STA_BITMAP(PowerEventBitMap[tpyeIdx].happend, tei_event);
#ifdef GDW_2019_PERCEPTION
							if (pEventBitmap->EventType == 1) //停电事件
							{
								P_STA_INFO p_sta = PLC_GetValidStaInfoByTei(tei_event);
								if (p_sta != NULL)
								{
									//u16 sn = PLC_GetWhiteNodeSnByMac(p_sta->mac);
									//if (sn < CCTT_METER_NUMBER)
									{
										SET_STA_BITMAP(offline_reason, p_sta-g_pStaInfo);
									}
								}
							}
#endif
#if 0
							P_STA_INFO pStaEvent = PLC_GetValidStaInfoByTei(tei_event);
							if (NULL == pStaEvent) continue;
							event_data[0] = pEventBitmap->EventType;
							memcpy_special(&event_data[1], pStaEvent->mac, MAC_ADDR_LEN);

							OS_ERR err;
							while (0 != OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz, OS_OPT_PEND_NON_BLOCKING, NULL, &err));
							PLM_ReportMeterEvent(EVENT_DEVICE_TYPE_HPLC, METER_PROTOCOL_3762_POWER_EVENT, pStaEvent->mac, event_data, 7);
							OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz / 2, OS_OPT_PEND_BLOCKING, NULL, &err);
#endif
						}
					}
				}
				if (update_flag == 1)
				{
					CPU_SR_ALLOC();
					CPU_CRITICAL_ENTER();
					if (tpyeIdx == 0)
					{
						OS_TICK_64 now = GetTickCount64();
						if (!BootPowerState)
						{
							if (PowerOffTick == 0 ||
								now > (PowerOffTick + 6 * 60 * 1000)) //产生此次的掉电事件的时间已经超过上一次的6min
							{
								PowerOffTick = now + 30 * 1000;
							}
						}
						if (OffUpTimerId == INVAILD_TMR_ID)
						{
							OffUpTimerId = HPLC_RegisterTmr(PowerOffUp3762, NULL, 4 * 1000, TMR_OPT_CALL_ONCE);
						}
						else
						{
							HPLC_DelTmr(OffUpTimerId);
							OffUpTimerId = HPLC_RegisterTmr(PowerOffUp3762, NULL, 4 * 1000, TMR_OPT_CALL_ONCE);
						}
					}
					else
					{
						if (OnUpTimerId == INVAILD_TMR_ID)
						{
							OnUpTimerId = HPLC_RegisterTmr(PowerOnUp3762, NULL, 30 * 1000, TMR_OPT_CALL_ONCE);
						}
						else
						{
							HPLC_DelTmr(OnUpTimerId);
							OnUpTimerId = HPLC_RegisterTmr(PowerOnUp3762, NULL, 30 * 1000, TMR_OPT_CALL_ONCE);
						}
					}
					CPU_CRITICAL_EXIT();
				}
			}
			else if ((3 == pEventBitmap->EventType) || (4 == pEventBitmap->EventType))
			{
				P_EVENT_DATA_MAC pEventMac = (P_EVENT_DATA_MAC)pEvent->Data;

				u16 coll_tei = 0; //采集器TEI
				if (
#ifdef HPLC_CSG
					(2 == pEvent->Function)
#else					
					(3 == pEvent->Function) 
#endif						
					&& (4 == pEventBitmap->EventType)) //采集器复电事件(MAC地址形式)
				{
					debug_str(DEBUG_LOG_NET, "PLC_HandlesAppEvent upPower, coll tei:%d\r\n", tei);
					
					if ((tei > TEI_MIN) && (tei <= TEI_MAX))
					{
						coll_tei = tei;
					}
				}
				
				for (u16 i = 0; i < pEventMac->MeterCount; i++)
				{
					memcpy(mac_addr, &pEventMac->MacAddr[i][0], MAC_ADDR_LEN);

					u16 sn = PLC_GetWhiteNodeSnByMac(mac_addr);
					if (sn != CCTT_METER_NUMBER)
					{
#ifdef HPLC_CSG
						//过滤停复电事件和停复电状态不匹配，因为本地接口上报没有停复电状态
						u8 power_status = pEventMac->MacAddr[i][MAC_ADDR_LEN];
                        debug_str(DEBUG_LOG_NET, "PLC_HandlesAppEvent upPower, mac:%02x%02x%02x%02x%02x%02x, type:%d, status:%d\r\n", 
                                                   mac_addr[5],mac_addr[4],mac_addr[3],mac_addr[2],mac_addr[1],mac_addr[0], pEventBitmap->EventType, power_status);

						if (((3 == pEventBitmap->EventType)&&(power_status != 0)) ||   //停电
						    ((4 == pEventBitmap->EventType)&&(power_status != 1)))     //复电
						{
							continue;
						}
#endif

						g_pMeterRelayInfo[sn].power_status = pEventMac->MacAddr[i][MAC_ADDR_LEN]; //电表带电状态

						if (coll_tei)
						{
							g_pMeterRelayInfo[sn].coll_sn = coll_tei;
						}

#ifdef PROTOCOL_NW_2023_GUANGDONG_V2DOT1 /*广东23年深化应用V2.1，采集器复电事件携带下挂电表地址，
                                           PowerEventBitMap只置采集器入网地址，统一由EventUpDate处理下挂电表信息*/
                        u8 dst_addr[LONG_ADDR_LEN] = {0};
                        PLC_GetAppDstMac(g_pMeterRelayInfo[sn].meter_addr, dst_addr);
                        u16 coll_sn = PLC_GetWhiteNodeSnByMac(dst_addr); //找到采集器入网地址对应的sn
                        if (CCTT_METER_NUMBER != coll_sn)
                        {
                            if (PowerEventBitMap[tpyeIdx].happend[coll_sn / 8] & (1 << (coll_sn & 0x7)))
    						{
    							continue;
    						}
    						update_flag = 1;
    						SET_STA_BITMAP(PowerEventBitMap[tpyeIdx].happend, coll_sn);
                        }
#else
						if (PowerEventBitMap[tpyeIdx].happend[sn / 8] & (1 << (sn & 0x7)))
						{
							continue;
						}
						update_flag = 1;
						SET_STA_BITMAP(PowerEventBitMap[tpyeIdx].happend, sn);
#endif
                        
#ifdef GDW_2019_PERCEPTION
						if (pEventBitmap->EventType == 3) //停电事件
						{
							P_STA_INFO ptei= PLC_GetStaInfoByMac(mac_addr);
							if (ptei)
							{
								SET_STA_BITMAP(offline_reason, ptei-g_pStaInfo);
							}
						}
#endif
					}

#if 0
					event_data[0] = (3 == pEventMac->EventType) ? 1 : 2;
					memcpy_special(&event_data[1], mac_addr, MAC_ADDR_LEN);

					OS_ERR err;
					while (0 != OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz, OS_OPT_PEND_NON_BLOCKING, NULL, &err));
					PLM_ReportMeterEvent(EVENT_DEVICE_TYPE_HPLC, METER_PROTOCOL_3762_POWER_EVENT, mac_addr, event_data, 7);
					OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz / 2, OS_OPT_PEND_BLOCKING, NULL, &err);
#endif
				}
				if (update_flag == 1)
				{
					CPU_SR_ALLOC();
					CPU_CRITICAL_ENTER();
					if (tpyeIdx == 2)
					{
						if (OffUpTimerId == INVAILD_TMR_ID)
						{
							OffUpTimerId = HPLC_RegisterTmr(PowerOffUp3762, NULL, 4 * 1000, TMR_OPT_CALL_ONCE);
						}
						else
						{
							HPLC_DelTmr(OffUpTimerId);
							OffUpTimerId = HPLC_RegisterTmr(PowerOffUp3762, NULL, 4 * 1000, TMR_OPT_CALL_ONCE);
						}
					}
					else
					{
						if (OnUpTimerId == INVAILD_TMR_ID)
						{
							OnUpTimerId = HPLC_RegisterTmr(PowerOnUp3762, NULL, 10 * 1000, TMR_OPT_CALL_ONCE);
						}
						else
						{
							HPLC_DelTmr(OnUpTimerId);
							OnUpTimerId = HPLC_RegisterTmr(PowerOnUp3762, NULL, 10 * 1000, TMR_OPT_CALL_ONCE);
						}
					}
					CPU_CRITICAL_EXIT();
				}
			}
#ifdef GDW_ZHEJIANG
			else if ((3 == pEvent->Function || 2 == pEvent->Function)&&(50 == pEventBitmap->EventType)) //采集器搜表结果上报
			{
				P_EVENT_COLLECTOR_DATA_MAC pEventCollect = (P_EVENT_COLLECTOR_DATA_MAC)pEvent->Data;

				P_EVENT_COLLECTOR_SUB_NODE_INFO pEventSubNodeInfo = (P_EVENT_COLLECTOR_SUB_NODE_INFO)pEventCollect->Data;

				PLC_add_new_node(CCTT_METER_NUMBER, METER_TYPE_PROTOCOL_07, pEventCollect->DeviceAddr);
//				u16 collect_sn=PLC_GetWhiteNodeSnByMac(pEventCollect->DeviceAddr);
//				P_RELAY_INFO p_RelayInfo = &g_MeterRelayInfo[collect_sn];
//				p_RelayInfo->coll_sn = SrcTei;
//				p_RelayInfo->status.IsCollector = 0;

				u8 update_flag = 0;
				u16 collect_TEI = SrcTei;

				for (u16 i = 0; i < pEventCollect->MeterCount; i++)
				{
				    int j=0;
					for (; j<CCTT_METER_NUMBER; j++)
					{
						if (g_pMeterRelayInfo[j].status.used == CCO_USERD)
						{
							if (macCmp(g_pMeterRelayInfo[j].meter_addr, pEventSubNodeInfo->node_addr)) //重复的不再更新上报
							{		
								if(g_pMeterRelayInfo[j].coll_sn != collect_TEI || g_pMeterRelayInfo[j].isReport != 0)
								{
									j = CCTT_METER_NUMBER;
									g_pMeterRelayInfo[j].isReport = true;
								}
								break;
							}	
						}
					}
					if(j == CCTT_METER_NUMBER)//新增站点需要上报
					{
						update_flag = 1;//需要上报集中器
						
						PLC_AddToSelfRegList(pEventSubNodeInfo->node_addr);     //添加即装即采表信息
						u16 meter_sn = PLC_get_sn_from_addr(pEventSubNodeInfo->node_addr);
						
						P_STA_INFO pSta = PLC_GetStaInfoByTei(collect_TEI);
						pSta->MeterNumConnect++;
						if (!(EventCollectBitMap[collect_TEI / 8] & (1 << (collect_TEI & 0x7))))
						{
							SET_STA_BITMAP(EventCollectBitMap, collect_TEI);
						}
						debug_str(DEBUG_LOG_INFO, "即装即采新站点, index %d, mac:%02x%02x%02x%02x%02x%02x, sn:%d\r\n", i,
								  pEventSubNodeInfo->node_addr[5], pEventSubNodeInfo->node_addr[4], pEventSubNodeInfo->node_addr[3],
								  pEventSubNodeInfo->node_addr[2], pEventSubNodeInfo->node_addr[1], pEventSubNodeInfo->node_addr[0], meter_sn);
						
						if (meter_sn < CCTT_METER_NUMBER)
						{
							g_MeterRelayInfo[meter_sn].coll_sn = SrcTei;;
							switch (pEventSubNodeInfo->protocol)
							{
							case 1:
								g_MeterRelayInfo[meter_sn].meter_type.protocol = METER_TYPE_PROTOCOL_97;
								break;
							case 2:
								g_MeterRelayInfo[meter_sn].meter_type.protocol = METER_TYPE_PROTOCOL_07;
								break;
							case 3:
								g_MeterRelayInfo[meter_sn].meter_type.protocol = METER_TYPE_PROTOCOL_69845;
								break;
							default:
								g_MeterRelayInfo[meter_sn].meter_type.protocol = METER_TYPE_PROTOCOL_TRANSPARENT;
								break;
							}
						}
					}
					pEventSubNodeInfo++;
				}

				if(update_flag == 1)//有节点更新则进入轮询队列中
				{
					PLC_EventCollectReport();
				}		
			}
#endif			
		}

PLC_HandlesAppEvent_Exit:
	free_buffer(__func__, pAppFrame);
}

bool PLC_GetMacAddrAppFrame(u8*data,u16 len, U8 mac_addr[MAC_ADDR_LEN])
{
	P_GD_DL645_BODY pDL645 = PLC_check_dl645_frame(data, len);
	P_DL69845_BODY pDL698 =  PLC_check_69845_frame(data, len);
	if (pDL645)
	{
		memcpy(mac_addr, pDL645->meter_number, MAC_ADDR_LEN);
		return  true;
	}
	else if (pDL698)
	{
		PLC_get_69845_SAMac(pDL698, mac_addr);
		return  true;
	}
	return false;
}
unsigned char PLC_GetMacAddrByMacFrame(P_MAC_HEAD pMacHead, U8 mac_addr[MAC_ADDR_LEN])
{
#if defined(HPLC_CSG)

	if (MAC_HEAD_LONG == pMacHead->HeadType)
	{
		P_MSDU_HEAD_LONG_CSG pMsduHead = (P_MSDU_HEAD_LONG_CSG)((u8 *)pMacHead) + sizeof(MAC_HEAD);
		memcpy_swap(mac_addr, pMsduHead->SrcMac, MAC_ADDR_LEN);
		return OK;
	}

#else

	if (MACFLAG_INCLUDE == pMacHead->MACFlag)
	{
		memcpy_swap(mac_addr, pMacHead->SrcMac, MAC_ADDR_LEN);
		return OK;
	}

#endif

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pMacHead->SrcTei);
	if (NULL == pSta)
		return (!OK);

	memcpy_special(mac_addr, pSta->mac, MAC_ADDR_LEN);
	return OK;
}


#if defined(HPLC_CSG)

typedef struct
{
	u32 DI;
	u8  Len;
	u8  BlockNum; //数据块数量
}DI_Len_s;

//数据标识对应数据回复长度
const DI_Len_s DI_Len_Tbl[]=
{
	{0x0201FF00, 2, 3},  //电压
	{0x0202FF00, 3, 3},  //电流
	{0x0203FF00, 3, 4},  //瞬时有功功率
	{0x0204FF00, 3, 4},  //瞬时无功功率
	{0x0205FF00, 3, 4},  //瞬时视在功率
	{0x0206FF00, 2, 4},  //功率因素
	{0x0207FF00, 2, 3},  //相角
	{0x0208FF00, 2, 3},  //电压波形失真度
	{0x0209FF00, 2, 3},  //电流波形失真度
	{0x020A01FF, 2, 21}, //A相电压谐波含量
	{0x020A02FF, 2, 21}, //B相电压谐波含量
	{0x020A03FF, 2, 21}, //C相电压谐波含量
	{0x020B01FF, 2, 21}, //A相电流谐波含量
	{0x020B02FF, 2, 21}, //B相电流谐波含量
	{0x020B03FF, 2, 21}, //C相电流谐波含量
	{0x02800101, 3, 1},  //零线电流
	{0x02800102, 2, 1},  //电网频率
	{0x02800103, 3, 1},  //一分钟有功总平均功率
	{0x02800104, 3, 1},  //当前有功需量
	{0x02800105, 3, 1},  //当前无功需量
	{0x02800106, 3, 1},  //当前视在需量
	{0x02800107, 2, 1},  //表内温度
	{0x02800108, 2, 1},  //时钟电池电压
	{0x02800109, 2, 1},  //停电超标电池电压
	{0x0280010A, 4, 1},  //内部电池工作时间
};

//南网21标准数据标识对应数据回复长度
const DI_Len_s CSG21_DI_Len_Tbl[]=
{
	{0x061201FF, 2, 3},  //电压
	{0x061202FF, 3, 3},  //电流
	{0x061203FF, 3, 4},  //有功功率
	{0x061204FF, 3, 4},  //无功功率
	{0x061205FF, 2, 4},  //功率因素
	{0x061206FF, 4, 4},  //总电能
	{0x061207FF, 4, 4},  //象限无功总电能
	{0x061208FF, 3, 2},  //当前需量
	{0x06120900, 3, 1},  //当前需量
};

static bool Find_DI_Len(u8 is_CSG21, u32 DI, u16* Len)
{
	if (is_CSG21)
	{
		for(int i=0; i<sizeof(CSG21_DI_Len_Tbl)/sizeof(CSG21_DI_Len_Tbl[0]); i++)
		{
			if((DI&0xFFFFFF00) == (CSG21_DI_Len_Tbl[i].DI&0xFFFFFF00))
			{
				if (Len != NULL)
				{
					if((DI&0xFFFFFFFF) == CSG21_DI_Len_Tbl[i].DI)
					{
						*Len = CSG21_DI_Len_Tbl[i].Len * CSG21_DI_Len_Tbl[i].BlockNum;
					}
					else
					{
						*Len = CSG21_DI_Len_Tbl[i].Len;
					}
				}

				//Len为NULL，单纯匹配判断是否南网21标准
				return true;
			}
		}
	}
	else
	{
		for(int i=0; i<sizeof(DI_Len_Tbl)/sizeof(DI_Len_Tbl[0]); i++)
		{
			if((DI&0xFFFF00FF) == (DI_Len_Tbl[i].DI&0xFFFF00FF))
			{
				if (Len != NULL)
				{
					if((DI&0xFFFFFFFF) == DI_Len_Tbl[i].DI)
					{
						*Len = DI_Len_Tbl[i].Len * DI_Len_Tbl[i].BlockNum;
					}
					else
					{
						*Len = DI_Len_Tbl[i].Len;
					}
				}

				//Len为NULL，单纯匹配判断是否南网老规范
				return true;
			}
			else if((DI&0xFFFFFF00) == (DI_Len_Tbl[i].DI&0xFFFFFF00))
			{
				if (Len != NULL)
				{
					if((DI&0xFFFFFFFF) == DI_Len_Tbl[i].DI)
					{
						*Len = DI_Len_Tbl[i].Len * DI_Len_Tbl[i].BlockNum;
					}
					else
					{
						*Len = DI_Len_Tbl[i].Len;
					}
				}

				//Len为NULL，单纯匹配判断是否南网老规范
				return true;
			}
		}
	}
	
	return false;
}

//匹配数据中DI，获得对应DI总长度
static bool Find_CSG21_DI_Len(u8* Data, u8 DINum, u8 ItemCount, u8* Len, u16* ToatlLen)
{
	u8  DICount = 0;
	u16 DILen = 0;
	P_GRAPH_ITEM_CSG21 pGraphItemCSG21 = (P_GRAPH_ITEM_CSG21)Data;

	debug_str(DEBUG_LOG_NET, "Find_CSG21_DI_Len, DINum:%d, ItemCount:%d\r\n", DINum, ItemCount);
	for(int i=0; i<DINum; i++)
	{
		if (!Find_DI_Len(1, pGraphItemCSG21->ItemDI, &DILen))
		{
			return false;
		}

		DICount++;
		Len[i] = DILen;
		*ToatlLen += DILen;

		debug_str(DEBUG_LOG_NET, "Find_CSG21_DI_Len, DI:0x%08x, Len:%d\r\n", pGraphItemCSG21->ItemDI, DILen);
		
		pGraphItemCSG21 = (P_GRAPH_ITEM_CSG21)((u8*)pGraphItemCSG21+ItemCount*DILen+4);
	}

	debug_str(DEBUG_LOG_NET, "Find_CSG21_DI_Len, Real DINum:%d, ToatlLen:%d\r\n", DICount, *ToatlLen);
	if (DINum != DICount)
	{
		return false;
	}

	return true;
}

//匹配下行数据中DI，获得对应DI总长度
static bool Find_Down_CSG21_DI_Len(u8* Data, u8 DINum, u8 ItemCount, u8* Len, u16* ToatlLen)
{
	u8  DICount = 0;
	u16 DILen = 0;
	P_GRAPH_ITEM_DOWN_CSG21 pGraphItemCSG21 = (P_GRAPH_ITEM_DOWN_CSG21)Data;

	debug_str(DEBUG_LOG_NET, "Find_Down_CSG21_DI_Len, DINum:%d, ItemCount:%d\r\n", DINum, ItemCount);
	for(int i=0; i<DINum; i++)
	{
		if (!Find_DI_Len(1, pGraphItemCSG21->ItemDI, &DILen))
		{
			return false;
		}

		DICount++;
		Len[i] = DILen;
		*ToatlLen += DILen;

		debug_str(DEBUG_LOG_NET, "Find_Down_CSG21_DI_Len, DI:0x%08x, Len:%d\r\n", pGraphItemCSG21->ItemDI, DILen);
		
		pGraphItemCSG21 = (P_GRAPH_ITEM_DOWN_CSG21)((u8*)pGraphItemCSG21+4);
	}

	debug_str(DEBUG_LOG_NET, "Find_Down_CSG21_DI_Len, Real DINum:%d, ToatlLen:%d\r\n", DICount, *ToatlLen);
	if (DINum != DICount)
	{
		return false;
	}

	return true;
}

u8 PLC_IsMyResponseByTei_gd(P_MAC_HEAD pMacHead, P_APP_BUSINESS_PACKET pBusinessPacket, u16 my_tei, u16 my_packet_seq_plc, u16 my_first_packet_seq_plc)
{
	if ((pBusinessPacket->FrameSeq - my_first_packet_seq_plc) > (my_packet_seq_plc - my_first_packet_seq_plc))
		return FALSE;

	if (pMacHead->SrcTei != my_tei)
		return FALSE;

	return TRUE;
}

u8 PLC_IsMyResponseByMacAddr_gd(P_MAC_HEAD pMacHead, P_APP_BUSINESS_PACKET pBusinessPacket, u8 my_mac_addr[MAC_ADDR_LEN], u16 my_packet_seq_plc)
{
	if ((pBusinessPacket->FrameSeq != my_packet_seq_plc) )
		return FALSE;

	BOOL get_mac_addr = TRUE;
	U8 mac_addr[MAC_ADDR_LEN];
	if (OK != PLC_GetMacAddrByMacFrame(pMacHead, mac_addr))
		get_mac_addr = FALSE;

	if (get_mac_addr)
	{
		U8 dst_addr[MAC_ADDR_LEN];
		PLC_GetAppDstMac(my_mac_addr,dst_addr);

		if (OK == macCmp(mac_addr, dst_addr))
		{
			return TRUE;
		}
		else
		{
			//回复的报文，源TEI是采集器TEI
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pMacHead->SrcTei);
			if (NULL != pSta)
			{
				if (pSta->dev_type == II_COLLECTOR || pSta->dev_type == I_COLLECTOR) 
				{
					debug_str(DEBUG_LOG_NET, "Rsp check coll stei:%d\r\n", pMacHead->SrcTei);
					return TRUE;
				}
			}
			
			return FALSE;
		}
	}

	return TRUE;
}
void PLC_GotoNormal(void *arg)
{
	Reboot_system(1, en_reboot_cmd, __func__);
}

#if defined(NW_TEST)

u16 PlcChceckTimerId = INVAILD_TMR_ID;
OS_TICK_64 g_read_mete_time=0;
u8 g_nw_test_high_power =0;
u8 g_nw_test_atten =0;
extern const u8 MAX_HPLC_POWER[];
void PLC_Chceck_time_from5to20(void *arg)
{
    OS_TICK_64 tick_diff =TicksBetween64(GetTickCount64(), g_read_mete_time);
    if (tick_diff >= 30*OSCfg_TickRate_Hz )
    {
       //恢复
      if(HPLC_ChlPower[BPLC_TxFrequenceGet()] != BPLC_GetTxGain() && g_nw_test_high_power==0)
      {
        BPLC_SetTxGain(HPLC_ChlPower[BPLC_TxFrequenceGet()]);
        debug_str(DEBUG_LOG_INFO,"timer resume_HPLC_POWER %d\r\n",HPLC_ChlPower[BPLC_TxFrequenceGet()]);
      }
    }
}
#endif
#ifdef ASSETSCODE_32BYTES_INFORM
void Complement_AssetCode(u8 *complement_data,u8 *data)
{
	/*对24位的资产编码扩充到32位，其余直接复制*/
	u8 element_num = 0, element_id = 0, element_len = 0;
	u16 copy_point=0,ergodic_point=0;
	element_num=data[0];
	ergodic_point++;
	for (int i = 0; i < element_num; i++)
	{
		element_id=data[ergodic_point++];
		element_len=(data[ergodic_point++]);
		if (0x10==element_id)
		{
			complement_data[copy_point++]=element_id;
			if (24==element_len)
			{
				complement_data[copy_point++]=element_len+8;
				memcpy(&complement_data[copy_point+8],&data[ergodic_point],element_len);
				copy_point+=(element_len+8);
			}
			else if (32==element_len)
			{
				complement_data[copy_point++]=element_len;
				memcpy(&complement_data[copy_point],&data[ergodic_point],element_len);
				copy_point+=(element_len);
			}
			ergodic_point+=element_len;
		}
		else
		{
			complement_data[copy_point++]=element_id;
			complement_data[copy_point++]=element_len;
			memcpy(&complement_data[copy_point],&data[ergodic_point],element_len);
			copy_point+=element_len;
			ergodic_point+=element_len;
		}
		
		
	}
	
}
#endif

u8 get_change_freq_frame=0;
void PLC_HandleAppFrame(P_HPLC_PARAM pHplcParam, P_MAC_HEAD pMacHead, P_APP_PACKET pAppFrame)
{
	if (APP_PACKET_CSG != pAppFrame->PacketID)
		return;

	P_APP_BUSINESS_PACKET pBusinessPacket = (P_APP_BUSINESS_PACKET)pAppFrame->Data;

	if (pMacHead->NID!=0)
	{
		if (1 != pBusinessPacket->Ctrl.Direction)
			return;
	}

	if (0 == pBusinessPacket->Ctrl.FrameType)   //确认/否认
	{
		if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
			return;
		
		if (0 != pBusinessPacket->Ctrl.StartFlag)
			return;

		if (0 == pBusinessPacket->BusinessID)        //确认
		{
			BOOL bProc = FALSE;
			if (!bProc)
			{
				if (g_pRelayStatus->event_switch_cb.state != 0
					&& g_pRelayStatus->event_switch_cb.seq == pBusinessPacket->FrameSeq)
				{
					g_pRelayStatus->event_switch_cb.bitmap[pMacHead->SrcTei >> 5] &= ~(1 << (pMacHead->SrcTei & 0x1f));
				}
				//单播配置负荷曲线采集间隔,回复确认帧
				P_PLC_TASK_READ_CB pcb = &g_pRelayStatus->task_read_cb;
				int i;
				for (i = 0; i < MAX_NW_PARALLEL_NUM; i++)
				{
					if (pcb->array[i].pReadTask
						&& PLC_IsMyResponseByMacAddr_gd(pMacHead, pBusinessPacket, pcb->array[i].pReadTask->mac_addr, pcb->array[i].packet_seq_plc))
					{
						break;
					}
				}

				if (i < MAX_NW_PARALLEL_NUM)
				{
					if ((pcb->array[i].pReadTask->meter_moudle) && (pcb->array[i].pReadTask->no_response_flag)) //配置负荷曲线采集间隔, 数据转发标识为1，不需要回复集中器
					{
						P_PLC_RSP_MSG_INFO pPlc = (P_PLC_RSP_MSG_INFO)alloc_buffer(__func__, sizeof(PLC_RSP_MSG_INFO));
						if (NULL != pPlc)
						{
							free_buffer(__func__, pcb->array[i].pPlcRsp);
							pcb->array[i].pPlcRsp = pPlc;
							bProc = TRUE;
						}
					}
				}
			}
		}
		else if (1 == pBusinessPacket->BusinessID)    //否认
		{
			u8 reason = pBusinessPacket->Data[0];
			//0为通信超时
			//1表示业务标识不支持
			//2表示CCO忙，
			//3表示电表层无应答，
			//4表示格式错误，
			//FFH其他；
			if (0 == reason)
			{
			}
			else if (1 == reason)
			{
				//X4功能
				P_PLC_X4_READ_TASK_SYNC_CB pcb = &g_pRelayStatus->x4_read_task_sync_cb;
				if (EM_X4_READ_TASK_SYNC_READ_TASK_ID_WAIT_PLC == pcb->state)
				{
					if (pcb->tei_sync != pMacHead->SrcTei)
						return;
					if (pBusinessPacket->FrameSeq != pcb->app_frame_seq_task_id_read)
						return;
					//上报集中器STA不支持
					pcb->state = EM_X4_READ_TASK_SYNC_NOT_SUPPORT;
				}
			}
		}
	}
	else if (1 == pBusinessPacket->Ctrl.FrameType)    //数据转发
	{
		if (0 != pBusinessPacket->Ctrl.StartFlag)
			return;

		if (0 == pBusinessPacket->BusinessID) //转发至电表
		{
			if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
			return;
		}
		else if (1 == pBusinessPacket->BusinessID) //转发至模块
		{
			if (pAppFrame->Port != HPLC_APP_PORT_RCU)
				return;
		}
		else
		{
			return;
		}

		P_APP_DATA_TRANS_UP pDataTrans = (P_APP_DATA_TRANS_UP)pBusinessPacket->Data;

		BOOL bProc = FALSE;
		if (!bProc)
		{
			//南网任务
			P_PLC_TASK_MultiREAD_CB p_multi_pcb = &g_pRelayStatus->task_multi_read_cb;

#if 1
			//多播任务时，每块表的帧序号固定，packet_seq_start+sn
			u16 tei = pMacHead->SrcTei;
			u16 sn = 0, packet_seq = 0;

			if (p_multi_pcb->state==EM_TASK_READ_FIND_STA||p_multi_pcb->state==EM_TASK_READ_SEND)
			{
				if (p_multi_pcb->pReadTask != NULL)
				{
					sn = get_multi_sta_sn(p_multi_pcb->pReadTask->p_sta, tei);
						
					if (sn != 0xFFFF)
					{
						packet_seq = p_multi_pcb->packet_seq_plc + sn;
					}
				}
			}

			if (packet_seq == pBusinessPacket->FrameSeq
				&&(p_multi_pcb->state==EM_TASK_READ_FIND_STA||p_multi_pcb->state==EM_TASK_READ_SEND))
			{
#else
			if (p_multi_pcb->packet_seq_plc == pBusinessPacket->FrameSeq
				&&(p_multi_pcb->state==EM_TASK_READ_FIND_STA||p_multi_pcb->state==EM_TASK_READ_SEND)
				)
			{

				u16 tei = pMacHead->SrcTei;
				u16 sn=PLC_GetWhiteNodeSnByStaInfo(PLC_GetValidStaInfoByTei(tei));
#endif
				if (p_multi_pcb->pReadTask->p_sta->bitmap[sn >> 5] & (1 << (sn & 0x1f)))
				{
					++p_multi_pcb->get_cnt;
					PLC_DeleteMultiSta(p_multi_pcb->pReadTask->p_sta, tei, sn);
					if (p_multi_pcb->pReadTask->task_header.task_mode.resp_flag)
					{
#if 1
						P_RELAY_INFO p_relay_info = PLC_GetValidNodeInfoBySn(sn);

						if (p_relay_info != NULL)
						{
							PLM_ReportTaskRead_gd(COM_PORT_MAINTAIN, p_multi_pcb->pReadTask->task_header.task_id, p_relay_info->meter_addr, pDataTrans->Data, pDataTrans->DataLen);
						}
#else					
						PLM_ReportTaskRead_gd(COM_PORT_MAINTAIN,p_multi_pcb->pReadTask->task_header.task_id, pDataTrans->SrcAddr, pDataTrans->Data, pDataTrans->DataLen);
#endif						
					}
				}
				bProc = TRUE;
			}
			else
			{
				P_PLC_TASK_READ_CB pcb = &g_pRelayStatus->task_read_cb;
				int i;
				for (i=0;i<MAX_NW_PARALLEL_NUM;i++)
				{
					if (pcb->array[i].pReadTask
						&&PLC_IsMyResponseByMacAddr_gd(pMacHead, pBusinessPacket, pcb->array[i].pReadTask->mac_addr, pcb->array[i].packet_seq_plc))
					{
						break;
					}
				}
				if (i<MAX_NW_PARALLEL_NUM)
				{
#if 1
					u8 dst_addr[LONG_ADDR_LEN] = {0};
					PLC_GetAppDstMac(pcb->array[i].pReadTask->mac_addr, dst_addr);

					if ((OK == macCmp(pDataTrans->SrcAddr, dst_addr))||
                        (OK == macCmp(pDataTrans->SrcAddr, pcb->array[i].pReadTask->mac_addr))) //规避采集器返回的源地址为下挂电表地址
#else
					if (OK == macCmp(pDataTrans->SrcAddr, pcb->array[i].pReadTask->mac_addr))
#endif						
					{
						P_PLC_RSP_MSG_INFO pPlc = (P_PLC_RSP_MSG_INFO)alloc_buffer(__func__,sizeof(PLC_RSP_MSG_HEAD) + pDataTrans->DataLen + 1);
						if (NULL != pPlc)
						{
							if(pcb->array[i].pReadTask->meter_moudle) //数据转发标识为1，需要添加业务代码
							{
								P_APP_DATA_MODULE_TRANS_UP pDataModuleTrans = (P_APP_DATA_MODULE_TRANS_UP)pBusinessPacket->Data;
								pPlc->body[0] = pDataModuleTrans->App_Code;

								/*12字节为地址域，8字节为AFN(1)+SEQ(1)+DI(4)+CS(1)+END(1)，3字节为任务ID(2)+任务报文长度(1)，1字节业务代码*/
								u16 max_len = PLC_BUFFER_UART_SIZE - (sizeof(LMP_LINK_HEAD_GD)-1) - 12 - 8 - 3 - 1;
								u16 report_len = pDataTrans->DataLen; //上报任务数据长度

								if ((pDataModuleTrans->App_Code == APP_SHENHUA_APP_CODE_GRAPH) && (pDataTrans->DataLen > max_len))
								{	
									u8 itemCount = 0;

									if (pDataModuleTrans->Data[0] == APP_GRAPH_FUNC_CODE_READ_21PROTOCOL) //21标准曲线抄读
									{
										u8 reportItemCount = 0; //载波上报采集点数量
										u16 DIToatlLen = 0;
										u16 copyLen = 0, moveLen = 0; //copyLen为截取数据长度，moveLen为指针移动长度
										u8 DILen[36] = {0}; //南网21标准目前最多支持36个数据标识
										P_APP_READ_GRAPH_ITEM_CSG21_UP pReadGraphItemCSG21Up = (P_APP_READ_GRAPH_ITEM_CSG21_UP)pDataModuleTrans->Data;
										P_GRAPH_ITEM_CSG21 pGraphItemCSG21 = (P_GRAPH_ITEM_CSG21)pReadGraphItemCSG21Up->Data;

										debug_str(DEBUG_LOG_NET, "APP_GRAPH_FUNC_CODE_READ_21PROTOCOL oversize, mac:%02x-%02x-%02x-%02x-%02x-%02x, period:%d, DI num:%d, count:%d\r\n", 
			                                      pDataTrans->SrcAddr[5], pDataTrans->SrcAddr[4], pDataTrans->SrcAddr[3], pDataTrans->SrcAddr[2], pDataTrans->SrcAddr[1], pDataTrans->SrcAddr[0], 
			                                      pReadGraphItemCSG21Up->Period, pReadGraphItemCSG21Up->ItemNum, pReadGraphItemCSG21Up->Count);
										
										if (!Find_DI_Len(1, pGraphItemCSG21->ItemDI, NULL)) //南网21标准匹配第一个数据标识
										{
											free_buffer(__func__, pPlc);
											return;
										}

										if (!Find_CSG21_DI_Len(pReadGraphItemCSG21Up->Data, pReadGraphItemCSG21Up->ItemNum, pReadGraphItemCSG21Up->Count, DILen, &DIToatlLen))
										{
											free_buffer(__func__, pPlc);
											return;
										}

										//实际可以上报的数据标识对应的采集点数量
										itemCount = (max_len - sizeof(APP_READ_GRAPH_ITEM_CSG21_UP) - 4*pReadGraphItemCSG21Up->ItemNum)/DIToatlLen;
										reportItemCount = pReadGraphItemCSG21Up->Count;
										pReadGraphItemCSG21Up->Count = itemCount;

										pPlc->header.msg_len = 1; //1字节业务代码
										memcpy_special(&pPlc->body[pPlc->header.msg_len], pDataTrans->Data, sizeof(APP_READ_GRAPH_ITEM_CSG21_UP));

#ifdef GRAPH_COLLECT_REWRITE_DEVTYPE
										if (pcb->array[i].pkt_devtype != 0xFF)
										{
											P_APP_READ_GRAPH_ITEM_CSG21_UP data = (P_APP_READ_GRAPH_ITEM_CSG21_UP)&pPlc->body[pPlc->header.msg_len];
											data->MeterType = pcb->array[i].pkt_devtype;
										}
#endif										
										pPlc->header.msg_len += sizeof(APP_READ_GRAPH_ITEM_CSG21_UP);
										
										for(int i=0; i<pReadGraphItemCSG21Up->ItemNum; i++)
										{												
											moveLen = reportItemCount*DILen[i] + 4; //4字节数据标识
											copyLen = itemCount*DILen[i] + 4;

											//截取每个数据标识对应的实际采集点数量
											memcpy_special(&pPlc->body[pPlc->header.msg_len], (u8*)pGraphItemCSG21, copyLen);
											pPlc->header.msg_len += copyLen;
											pGraphItemCSG21 = (P_GRAPH_ITEM_CSG21)((u8 *)pGraphItemCSG21 + moveLen);
										}				
									}
									else if (pDataModuleTrans->Data[0] == APP_GRAPH_FUNC_CODE_READ) //老规范曲线抄读
									{
										u16 dataIndex = 0;
										u32 itemDI = 0;
										u16 itemDILen = 0;
										u8 *itemCountReal = NULL; //南网深化应用老规范，最终上报的每一个数据标识对应的采集点数量，根据实际情况重新赋值
										P_APP_READ_GRAPH_ITEM_UP pReadGraphItemUp = (P_APP_READ_GRAPH_ITEM_UP)pDataModuleTrans->Data;
										P_GRAPH_ITEM pGraphItem = (P_GRAPH_ITEM)pReadGraphItemUp->Data;

										debug_str(DEBUG_LOG_NET, "APP_GRAPH_FUNC_CODE_READ oversize, mac:%02x-%02x-%02x-%02x-%02x-%02x, DI num:%d\r\n", 
			                                      pDataTrans->SrcAddr[5], pDataTrans->SrcAddr[4], pDataTrans->SrcAddr[3], pDataTrans->SrcAddr[2], pDataTrans->SrcAddr[1], pDataTrans->SrcAddr[0], 
			                                      pReadGraphItemUp->ItemNum);
										
										if (!Find_DI_Len(0, pGraphItem->ItemDI, NULL)) //匹配第一个数据标识
										{
											free_buffer(__func__, pPlc);
											return;
										}

										pReadGraphItemUp->ItemNum = 0; //最终上报数据项数量清0，根据实际情况重新赋值
										dataIndex += sizeof(APP_READ_GRAPH_ITEM_UP);
										
										while(dataIndex < max_len) //南网本地接口任务上报数据，报文长度字段大小为1字节，需要截取数据上报
										{
											report_len = dataIndex;
											if (NULL != itemCountReal)
											{
												*itemCountReal += 1; //实际上报采集点数量计算
											}
											
											if (itemCount == 0) //获取具体数据项
											{
												pGraphItem = (P_GRAPH_ITEM)&(pDataModuleTrans->Data[dataIndex]);
												
												itemDI = pGraphItem->ItemDI;
												itemCount = pGraphItem->ItemCount;
												pGraphItem->ItemCount = 0;
												itemCountReal = &pGraphItem->ItemCount;
												if (!Find_DI_Len(0, itemDI, &itemDILen)) //数据标识得不到对应长度直接退出循环
												{
													break;
												}

												dataIndex += sizeof(GRAPH_ITEM);

												//数据项头+1个数据项长度，满足长度条件，则最终上报数据项数量+1
												if (dataIndex+itemDILen < max_len)
												{
													pReadGraphItemUp->ItemNum += 1;
												}
											}

											itemCount--;
											dataIndex += itemDILen;
										}

										pPlc->header.msg_len = report_len + 1;
										memcpy_special(&pPlc->body[1], pDataTrans->Data, report_len);
									}
#ifdef GRAPH_COLLECT_EXTEND
									else if(*(u32 *)(pDataModuleTrans->Data) == 0x0702FF00 || *(u32 *)(pDataModuleTrans->Data) == 0x0702FF01 )
									{
										pPlc->header.msg_len = report_len + 1;
										memcpy_special(&pPlc->body[1], pDataTrans->Data, report_len);
									}
#endif
								}
								else
								{
									pPlc->header.msg_len = report_len + 1;								
									memcpy_special(&pPlc->body[1], pDataTrans->Data, report_len);

#ifdef GRAPH_COLLECT_REWRITE_DEVTYPE
									P_APP_DATA_MODULE_TRANS_UP pDataModuleTrans = (P_APP_DATA_MODULE_TRANS_UP)pBusinessPacket->Data;
									if ((pDataModuleTrans->App_Code == APP_SHENHUA_APP_CODE_GRAPH))
									{
										if (pDataModuleTrans->Data[0] == APP_GRAPH_FUNC_CODE_READ_21PROTOCOL)
										{
											P_APP_READ_GRAPH_ITEM_CSG21_UP data = (P_APP_READ_GRAPH_ITEM_CSG21_UP)&pPlc->body[1];
											
											debug_str(DEBUG_LOG_NET, "APP_GRAPH_FUNC_CODE_READ_21PROTOCOL, mac:%02x-%02x-%02x-%02x-%02x-%02x, devtype:%d-%d, period:%d, DI num:%d, count:%d\r\n", 
			                                      pDataTrans->SrcAddr[5], pDataTrans->SrcAddr[4], pDataTrans->SrcAddr[3], pDataTrans->SrcAddr[2], pDataTrans->SrcAddr[1], pDataTrans->SrcAddr[0], 
			                                      data->MeterType, pcb->array[i].pkt_devtype,
			                                      data->Period, data->ItemNum, data->Count);
											
											if (pcb->array[i].pkt_devtype != 0xFF)
											{
												data->MeterType = pcb->array[i].pkt_devtype;
											}
										}
									}
#endif
								}
							}
							else
							{
								pPlc->header.msg_len = pDataTrans->DataLen;
								memcpy_special(pPlc->body, pDataTrans->Data, pDataTrans->DataLen);

                                debug_str(DEBUG_LOG_NET, "ReadMeter Rsp, tei:%d, mac:%02x%02x%02x%02x%02x%02x, seq:%d, len:%d\r\n", 
                                                  pMacHead->SrcTei,
			                                      pDataTrans->SrcAddr[5], pDataTrans->SrcAddr[4], pDataTrans->SrcAddr[3], 
			                                      pDataTrans->SrcAddr[2], pDataTrans->SrcAddr[1], pDataTrans->SrcAddr[0], 
			                                      pBusinessPacket->FrameSeq, pDataTrans->DataLen);
							}
							
							free_buffer(__func__, pcb->array[i].pPlcRsp);
							pcb->array[i].pPlcRsp = pPlc;
							bProc = TRUE;

							P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pMacHead->SrcTei);
							if (NULL != pSta)
							{
								if (pSta->dev_type == II_COLLECTOR || pSta->dev_type == I_COLLECTOR) 
								{
									u16 m_sn = PLC_get_sn_from_addr(pDataTrans->SrcAddr);
									if (m_sn < CCTT_METER_NUMBER)
									{
										if (g_MeterRelayInfo[m_sn].coll_sn == 0xffff)
										{
											debug_str(DEBUG_LOG_NET, "ADD_MLIST, Coll:%d, mac:%02x%02x%02x%02x%02x%02x, sn:%d\r\n", pMacHead->SrcTei,
											      pDataTrans->SrcAddr[5], pDataTrans->SrcAddr[4], pDataTrans->SrcAddr[3],
											      pDataTrans->SrcAddr[2], pDataTrans->SrcAddr[1], pDataTrans->SrcAddr[0], m_sn);
											g_MeterRelayInfo[m_sn].coll_sn = pMacHead->SrcTei;
											g_MeterRelayInfo[m_sn].meter_type.protocol = METER_TYPE_PROTOCOL_07;
										}
									}									
								}
							}
						}
					}
				}
			}
		}


	}
	else if (2 == pBusinessPacket->Ctrl.FrameType)  //命令帧
	{
		if (pMacHead->NID!=0)
		{
			if (0 != pBusinessPacket->Ctrl.StartFlag)
				return;
		}

		EM_CMD_BUSINESS_ID business_id = (EM_CMD_BUSINESS_ID)pBusinessPacket->BusinessID;

		if (EM_CMD_BUSINESS_READ_METER_LIST == business_id)              //查询搜表结果
		{
			if (pMacHead->SrcTei == CCO_TEI)
			{
				return;
			}
			
			if (0 != pBusinessPacket->Ctrl.StartFlag)
			{
				return;
			}

			P_APP_SEARCH_UP pRegResult = (P_APP_SEARCH_UP)pBusinessPacket->Data;

			if (pRegResult->num == 0)
			{
				debug_str(DEBUG_LOG_NET, "搜表结果个数为0\r\n");
				return;
			}
#ifdef NW_NEW_TEST 
            if(g_pRelayStatus->packet_seq_read_meter != pBusinessPacket->FrameSeq)
            {
				debug_str(DEBUG_LOG_NET, "seq不匹配 %d,%d\r\n",g_pRelayStatus->packet_seq_read_meter,pBusinessPacket->FrameSeq);
				return;
			}
#endif
			debug_str(DEBUG_LOG_NET, "READ_MLIST, srcTei:%d, meterNum:%d\r\n", pMacHead->SrcTei, pRegResult->num);
			P_APP_SEARCH_METER pRegMeter = (P_APP_SEARCH_METER)pRegResult->data;
			for (u16 i = 0; i < pRegResult->num; i++)
			{
				PLC_AddToSelfRegList(pRegMeter->MeterAddr);     //第1块表与mac_addr一样
				u16 m_sn = PLC_get_sn_from_addr(pRegMeter->MeterAddr);
				debug_str(DEBUG_LOG_NET, "READ_MLIST, index %d, mac:%02x%02x%02x%02x%02x%02x, sn:%d\r\n", i,
					      pRegMeter->MeterAddr[5], pRegMeter->MeterAddr[4], pRegMeter->MeterAddr[3],
					      pRegMeter->MeterAddr[2], pRegMeter->MeterAddr[1], pRegMeter->MeterAddr[0], m_sn);
				if (m_sn < CCTT_METER_NUMBER)
				{
					g_MeterRelayInfo[m_sn].coll_sn = pMacHead->SrcTei;
					g_MeterRelayInfo[m_sn].meter_type.protocol = METER_TYPE_PROTOCOL_07;
				}

				pRegMeter++;
			}

			u16 tei = pMacHead->SrcTei;
			if (tei <= TEI_STA_MAX)
			{
				P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
				pStaStatus->selfreg_search_node = 1;
			}

			P_PLC_SELFREG_CB pcb  = &g_pRelayStatus->selfreg_cb;
			if ((EM_SELFREG_INIT == pcb->state) || (pcb->state >= EM_SELFREG_REPORT_STOP))
			{
				g_pRelayStatus->get_sn_cb.bitmap[tei>>5]&=~(1<<(tei&0x1f));
				debug_str(DEBUG_LOG_NET, "搜表已结束\r\n");
				return;
			}
			
			if (tei <= TEI_STA_MAX && (!(pcb->upBitmap[tei >> 3] & (1 << (tei & 0x7)))))
			{
				g_pRelayStatus->get_sn_cb.bitmap[tei>>5]&=~(1<<(tei&0x1f));
				if (PLM_ReportSelfReg(pRegResult))
				{
					pcb->upBitmap[tei >> 3] |= 1 << (tei & 0x7);
				}
				else
				{
					P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
					pStaStatus->selfreg_search_node = 1;
				}
			}
			
		}
		else if (EM_CMD_BUSINESS_WRITE_METER_LIST == business_id)        //下发搜表列表
		{
		}
		else if (EM_CMD_BUSINESS_REBOOT_STA == business_id)              //从节点重启
		{
		}
		else if (EM_CMD_BUSINESS_READ_STA_INFO == business_id)           //从节点信息查询
		{
			if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
				return;
			
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pMacHead->SrcTei);

			if (pSta != NULL)
			{
				debug_str(DEBUG_LOG_NET, "EM_CMD_BUSINESS_READ_STA_INFO, mac:%02x-%02x-%02x-%02x-%02x-%02x, element num:%d\r\n", 
			              pSta->mac[5], pSta->mac[4], pSta->mac[3], pSta->mac[2], pSta->mac[1], pSta->mac[0], pBusinessPacket->Data[0]);
				
				int idx = 1;
				int len = 0;
				for (int i = 0; i < pBusinessPacket->Data[0]; i++)
				{
					switch (pBusinessPacket->Data[idx])
					{
					    case 0: //厂商代码，2字节ASCII
							{
								++idx;
								len = pBusinessPacket->Data[idx];
								++idx;

                                if (len) //有厂家STA回复len为0表示从节点不支持
                                {
								    memcpy(pSta->FactoryCode, &pBusinessPacket->Data[idx], 2);
                                }
							}
							break;
					    case 1: //软件版本信息（模块），2字节BCD
							{
								++idx;
								len = pBusinessPacket->Data[idx];
								++idx;

                                if (len) //有厂家STA回复len为0表示从节点不支持
                                {
								    memcpy(pSta->ModuleSoftVer, &pBusinessPacket->Data[idx], 2);
                                }
							}
							break;
						case 2: //bootloader版本号，1字节BIN
							{
								++idx;
								len=pBusinessPacket->Data[idx];
								++idx;
		
                                if (len) //有厂家STA回复len为0表示从节点不支持
                                {
								pSta->BootVer = pBusinessPacket->Data[idx];
							}
							}
							break;	
                        case 5: //芯片代码，2字节ASCII
							{
								++idx;
								len = pBusinessPacket->Data[idx];
								++idx;

                                if (len) //有厂家STA回复len为0表示从节点不支持
                                {
								    memcpy(pSta->ChipCode, &pBusinessPacket->Data[idx], 2);
                                }
							}
							break;
                        case 6: //软件发布日期（模块），3字节BIN
							{
								++idx;
								len = pBusinessPacket->Data[idx];
								++idx;

                                if (len) //有厂家STA回复len为0表示从节点不支持
                                {
    		                        pSta->ModuleSoftVerDate.Year = pBusinessPacket->Data[idx+2];
    								pSta->ModuleSoftVerDate.Month = pBusinessPacket->Data[idx+1];
    								pSta->ModuleSoftVerDate.Day = pBusinessPacket->Data[idx];
                                }
		                    }
							break;
						case 8: //模块出厂MAC地址，6字节BIN
								{
								++idx;
								len = pBusinessPacket->Data[idx];
								++idx;

                                if (len) //有厂家STA回复len为0表示从节点不支持
                                {
								    memcpy(pSta->module_id, &pBusinessPacket->Data[idx], 6);
                                }
								pSta->geted_id = 1;
							}
						    break;
						case 9: //硬件版本信息（模块），2字节BCD
							{
								++idx;
								len = pBusinessPacket->Data[idx];
								++idx;
								
                                if (len) //有厂家STA回复len为0表示从节点不支持
                                {
								memcpy(pSta->ModuleHardVer, &pBusinessPacket->Data[idx], 2);
							}
							}
							break;
						case 0xA: //硬件发布日期（模块），3字节BIN
							{
								++idx;
								len = pBusinessPacket->Data[idx];
								++idx;
								
                                if (len) //有厂家STA回复len为0表示从节点不支持
                                {
		                        pSta->ModuleHardVerDate.Year = pBusinessPacket->Data[idx+2];
								pSta->ModuleHardVerDate.Month = pBusinessPacket->Data[idx+1];
								pSta->ModuleHardVerDate.Day = pBusinessPacket->Data[idx];
		                    }
		                    }
							break;
						case 0xB: //软件版本信息（芯片），2字节BCD
							{
								++idx;
								len = pBusinessPacket->Data[idx];
								++idx;

                                if (len) //有厂家STA回复len为0表示从节点不支持
                                {
								memcpy(pSta->ChipSoftVer, &pBusinessPacket->Data[idx], 2);
							}
							}
							break;
						case 0xC: //软件发布日期（芯片），3字节BIN
							{
								++idx;
								len = pBusinessPacket->Data[idx];
								++idx;
								
                                if (len) //有厂家STA回复len为0表示从节点不支持
                                {
								pSta->ChipSoftVerDate.Year = pBusinessPacket->Data[idx+2];
								pSta->ChipSoftVerDate.Month = pBusinessPacket->Data[idx+1];
								pSta->ChipSoftVerDate.Day = pBusinessPacket->Data[idx];
		                    }
		                    }
							break;
						case 0xD: //硬件版本信息（芯片），2字节BCD
							{
								++idx;
								len = pBusinessPacket->Data[idx];
								++idx;

                                if (len) //有厂家STA回复len为0表示从节点不支持
                                {
								memcpy(pSta->ChipHardVer, &pBusinessPacket->Data[idx], 2);
							}
							}
							break;
						case 0xE: //硬件发布日期（芯片），3字节BIN
							{
								++idx;
								len = pBusinessPacket->Data[idx];
								++idx;
								
                                if (len) //有厂家STA回复len为0表示从节点不支持
                                {
	                            pSta->ChipHardVerDate.Year = pBusinessPacket->Data[idx+2];
								pSta->ChipHardVerDate.Month = pBusinessPacket->Data[idx+1];
								pSta->ChipHardVerDate.Day = pBusinessPacket->Data[idx];
		                    }
		                    }
							break;
						case 0xF: //应用程序版本号，2字节BCD
							{
								++idx;
								len = pBusinessPacket->Data[idx];
								++idx;
	
                                if (len) //有厂家STA回复len为0表示从节点不支持
                                {
								memcpy(pSta->AppVer, &pBusinessPacket->Data[idx], 2);
							}
							}
							break;
						case 0x10: //通信模块资资产编码，24字节ASCII
							{
								++idx;
								len = pBusinessPacket->Data[idx];
								++idx;

                                if (len) //有厂家STA回复len为0表示从节点不支持
                                {
									#ifndef ASSETSCODE_32BYTES_INFORM
								memcpy(pSta->assetsCode, &pBusinessPacket->Data[idx], 24);
									#else
									memset(pSta->assetsCode,0,32);
									if (24==len)
									{
										memcpy(pSta->assetsCode+8, &pBusinessPacket->Data[idx], 24);
							}
									else if (32==len)
									{
										memcpy(pSta->assetsCode, &pBusinessPacket->Data[idx], 32);
							}
									#endif
                                }
							}
							break;
						default:
							++idx;
							len=pBusinessPacket->Data[idx];
							++idx;
							break;
					}
					idx+=len;
				}

#ifdef PROTOCOL_NW_2021	
				P_PLC_MAINTENANCE_NET_CB pcb = &g_pRelayStatus->sta_info_realtime_cb;
				
				if (EM_TASK_READ_WAIT_PLC == pcb->state)
				{
					u8 dst_addr[LONG_ADDR_LEN] = {0};
					PLC_GetAppDstMac(pcb->mac_addr, dst_addr);
					if (macCmp(dst_addr, pSta->mac))
					{
						if (pcb->report_sn)  //去重
						{
							P_PLC_RSP_MSG_INFO pPlc = (P_PLC_RSP_MSG_INFO)alloc_buffer(__func__, sizeof(PLC_RSP_MSG_HEAD) + pBusinessPacket->FrameLen + 6 - 1);
							if (NULL != pPlc)
							{
								memcpy_special(pPlc->body, pcb->mac_addr, 6); //sta mac或者采集器下挂电表mac
#ifdef ASSETSCODE_32BYTES_INFORM
								Complement_AssetCode(&pPlc->body[6],&pBusinessPacket->Data[0]);//24位资产编码扩充32位
								pPlc->header.msg_len = pBusinessPacket->FrameLen+6-1+8; //6字节mac地址;1字节信息元素数量;24位资产编码扩充32位，长度加8字节
#else
								memcpy_special(&pPlc->body[6], &pBusinessPacket->Data[1], pBusinessPacket->FrameLen-1); //信息元素数量不需要上报
								pPlc->header.msg_len = pBusinessPacket->FrameLen+6-1; //6字节mac地址;1字节信息元素数量
#endif
								free_buffer(__func__, pcb->pPlcRsp);
								pcb->pPlcRsp = pPlc;
								return;
							}
						}
					}
				}
#endif				
			}
		}
		else if (EM_CMD_BUSINESS_WRITE_COMM_ADDR == business_id)         //下发通信地址映射表列表
		{
		}
		#ifdef PLC_X4_FUNC
		else if (EM_CMD_BUSINESS_READ_TASK_INIT == business_id)          //初始化采集任务
		{
		}
		else if (EM_CMD_BUSINESS_ADD_READ_TASK == business_id)           //添加采集任务
		{
			u8 status = pBusinessPacket->Data[0];       //非广播应答 1 字节， 0x00-确认，其他-否认；

			P_PLC_X4_READ_TASK_SYNC_CB pcb = &g_pRelayStatus->x4_read_task_sync_cb;
			if (EM_X4_READ_TASK_SYNC_ADD_TASK_WAIT_PLC == pcb->state
				&& PLC_IsMyResponseByTei_gd(pMacHead, pBusinessPacket, pcb->tei_sync, pcb->app_frame_seq_task_add, pcb->app_frame_seq_task_add))
			{
				if (0 == status)
					pcb->state = EM_X4_READ_TASK_SYNC_ADD_TASK_RECEIVE_PLC;
				else
					pcb->state = EM_X4_READ_TASK_SYNC_NOT_SUPPORT;
			}
		}
		else if (EM_CMD_BUSINESS_DELETE_READ_TASK == business_id)        //删除采集任务
		{
			u8 status = pBusinessPacket->Data[0];       //非广播应答 1 字节， 0x00-确认，其他-否认；

			BOOL bProc = FALSE;

			if (!bProc)
			{
				P_PLC_DIR_READ_CB pcb = &g_pRelayStatus->dir_read_cb;
				if ((EM_DIR_READ_WAIT_PLC == pcb->state)
					&& PLC_IsMyResponseByMacAddr_gd(pMacHead, pBusinessPacket, pcb->pPlmCmd->header.mac, pcb->pPlmCmd->header.packet_seq_plc))
				{
					P_PLC_RSP_MSG_INFO pPlc = (P_PLC_RSP_MSG_INFO)alloc_buffer(__func__,sizeof(PLC_RSP_MSG_HEAD) + 1);
					if (NULL != pPlc)
					{
						pPlc->header.msg_len = 1;
						pPlc->body[0] = status;
						free_buffer(__func__,pcb->pPlcRsp);
						pcb->pPlcRsp = pPlc;
						bProc = TRUE;
					}
				}
			}

			if (!bProc)
			{
				P_PLC_X4_READ_TASK_SYNC_CB pcb = &g_pRelayStatus->x4_read_task_sync_cb;

				if ((EM_X4_READ_TASK_SYNC_DELETE_TASK_WAIT_PLC == pcb->state)
					&& PLC_IsMyResponseByTei_gd(pMacHead, pBusinessPacket, pcb->tei_sync, pcb->app_frame_seq_task_delete, pcb->app_frame_seq_task_delete))
				{
					if (0 == status)
						pcb->state = EM_X4_READ_TASK_SYNC_DELETE_TASK_RECEIVE_PLC;
					else
						pcb->state = EM_X4_READ_TASK_SYNC_NOT_SUPPORT;
				}
			}
		}
		else if (EM_CMD_BUSINESS_READ_TASK_ID == business_id)            //查询采集任务号
		{
			BOOL bProc = FALSE;

			if (!bProc)
			{
				P_PLC_DIR_READ_CB pcb = &g_pRelayStatus->dir_read_cb;
				if ((EM_DIR_READ_WAIT_PLC == pcb->state)
					&& PLC_IsMyResponseByMacAddr_gd(pMacHead, pBusinessPacket, pcb->pPlmCmd->header.mac, pcb->pPlmCmd->header.packet_seq_plc)
					&& pBusinessPacket->FrameLen >= 7)
				{
					P_PLC_RSP_MSG_INFO pPlc = (P_PLC_RSP_MSG_INFO)alloc_buffer(__func__,sizeof(PLC_RSP_MSG_HEAD) + pBusinessPacket->FrameLen - 6);
					if (NULL != pPlc)
					{
						pPlc->header.msg_len = pBusinessPacket->FrameLen - 6;
						memcpy_special(pPlc->body, &pBusinessPacket->Data[6], pBusinessPacket->FrameLen - 6);
						free_buffer(__func__,pcb->pPlcRsp);
						pcb->pPlcRsp = pPlc;
						bProc = TRUE;
					}
				}
			}

			if (!bProc)
			{
				P_PLC_X4_READ_TASK_SYNC_CB pcb = &g_pRelayStatus->x4_read_task_sync_cb;
				if (EM_X4_READ_TASK_SYNC_READ_TASK_ID_WAIT_PLC == pcb->state
					&& PLC_IsMyResponseByTei_gd(pMacHead, pBusinessPacket, pcb->tei_sync, pcb->app_frame_seq_task_id_read, pcb->app_frame_seq_task_id_read))
				{
					u8 *p = pBusinessPacket->Data;
					p += 6;
					u8 task_num_1 = *p++;
					u8 task_num_2 = countof(pcb->task_id_read_array);
					pcb->task_read_num = MIN(task_num_1, task_num_2);
					for (u8 i = 0; i < pcb->task_read_num; i++) pcb->task_id_read_array[i] = *p++;
					pcb->state = EM_X4_READ_TASK_SYNC_READ_TASK_ID_RECEIVE_PLC;
				}
			}
		}
		else if (EM_CMD_BUSINESS_READ_TASK_DETAIL == business_id)        //查询采集任务详细信息
		{
			BOOL bProc = FALSE;

			if (!bProc)
			{
				P_PLC_DIR_READ_CB pcb = &g_pRelayStatus->dir_read_cb;
				if ((EM_DIR_READ_WAIT_PLC == pcb->state)
					&& PLC_IsMyResponseByMacAddr_gd(pMacHead, pBusinessPacket, pcb->pPlmCmd->header.mac, pcb->pPlmCmd->header.packet_seq_plc)
					&& pBusinessPacket->FrameLen >= 1)
				{
					P_PLC_RSP_MSG_INFO pPlc = (P_PLC_RSP_MSG_INFO)alloc_buffer(__func__,sizeof(PLC_RSP_MSG_HEAD) + pBusinessPacket->FrameLen);
					if (NULL != pPlc)
					{
						pPlc->header.msg_len = pBusinessPacket->FrameLen;
						memcpy_special(pPlc->body, pBusinessPacket->Data, pBusinessPacket->FrameLen);
						free_buffer(__func__,pcb->pPlcRsp);
						pcb->pPlcRsp = pPlc;
						bProc = TRUE;
					}
				}
			}

			if (!bProc)
			{
				P_PLC_X4_READ_TASK_SYNC_CB pcb = &g_pRelayStatus->x4_read_task_sync_cb;

				if (EM_X4_READ_TASK_SYNC_READ_TASK_DETAIL_WAIT_PLC == pcb->state
					&& PLC_IsMyResponseByTei_gd(pMacHead, pBusinessPacket, pcb->tei_sync, pcb->app_frame_seq_task_info_read, pcb->app_frame_seq_task_info_read))
				{
					P_X4_READ_TASK_INFO pReadTaskSta = (P_X4_READ_TASK_INFO)pBusinessPacket->Data;
					u8 i_null = 0xff;
					BOOL found = FALSE;
					u8 c = countof(pcb->task_info_read_array);
					for (u8 i = 0; i < c; i++)
					{
						P_X4_READ_TASK_INFO pReadTask = pcb->task_info_read_array[i];
						if (NULL == pReadTask)
						{
							if (0xff == i_null)
								i_null = i;
							continue;
						}
						if (OK == PLC_X4ReadTaskIsEqual(pReadTask, pReadTaskSta))
						{
							found = TRUE;
							break;
						}
					}
					if (!found && i_null < 0xff)
					{
						P_X4_READ_TASK_INFO pReadTask = (P_X4_READ_TASK_INFO)alloc_buffer(__func__,pBusinessPacket->FrameLen);

						if (NULL != pReadTask)
						{
							memcpy_special(pReadTask, pReadTaskSta, pBusinessPacket->FrameLen);
							pcb->task_info_read_array[i_null] = pReadTask;
						}
					}
					pcb->state = EM_X4_READ_TASK_SYNC_READ_TASK_DETAIL_RECEIVE_PLC;
				}
			}
		}
		else if (EM_CMD_BUSINESS_READ_TASK_DATA == business_id)          //采集任务数据
		{
			P_PLC_TASK_READ_CB pcb = &g_pRelayStatus->task_read_cb;
			int i;
			for (i = 0; i < MAX_NW_PARALLEL_NUM; i++)
			{
				if (pcb->array[i].pReadTask
					&&PLC_IsMyResponseByMacAddr_gd(pMacHead, pBusinessPacket, pcb->array[i].pReadTask->mac_addr, pcb->array[i].packet_seq_plc))
				{
					break;
				}
			}
			if (i<MAX_NW_PARALLEL_NUM)
			{
				P_PLC_RSP_MSG_INFO pPlc = (P_PLC_RSP_MSG_INFO)alloc_buffer(__func__,sizeof(PLC_RSP_MSG_HEAD) + pBusinessPacket->FrameLen);
				if (NULL != pPlc)
				{
					pPlc->header.msg_len = pBusinessPacket->FrameLen;
					memcpy_special(pPlc->body, pBusinessPacket->Data, pBusinessPacket->FrameLen);
					free_buffer(__func__,pcb->array[i].pPlcRsp);
					pcb->array[i].pPlcRsp = pPlc;
					return;
				}
			}
		}
		#endif
		else if (EM_CMD_BUSINESS_AREA_DISCERNMENT == business_id)
		{
#ifndef DIS_FUNC
			P_PLC_AREA_DISCERNMENT pcb = &g_pRelayStatus->area_dis_cb;

			if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
				return;
			
			if (0 != pBusinessPacket->Ctrl.StartFlag)
				return;

			P_AREA_DISCERNMENT_CSG_DOWN pInfo = (P_AREA_DISCERNMENT_CSG_DOWN)pBusinessPacket->Data;

			if (0x05 == pInfo->CollectionType)  //台区判别结果信息
			{
				if (EM_AREA_DISCERNMENT_INIT == pcb->state)
					return;
				P_AREA_DISCERNMENT_RESULT_DATA pResult = (P_AREA_DISCERNMENT_RESULT_DATA)pInfo->Data;

				u8 meter_addr[MAC_ADDR_LEN];
				u8 main_node_mac[MAC_ADDR_LEN];
				u8 my_mac[MAC_ADDR_LEN];

				PLC_CopyCCOMacAddr(my_mac, FALSE);
				memcpy_swap(meter_addr, pInfo->MacAddr, MAC_ADDR_LEN);
				memcpy_swap(main_node_mac, pResult->MainNodeMacAddr, MAC_ADDR_LEN);

				debug_str(DEBUG_LOG_NET, "EM_CMD_BUSINESS_AREA_DISCERNMENT, sta:%02x-%02x-%02x-%02x-%02x-%02x, result:%d, CompleteFlag:%d, tei:%d\r\n", 
					      meter_addr[5], meter_addr[4], meter_addr[3], meter_addr[2], meter_addr[1], meter_addr[0], pResult->Result, pResult->CompleteFlag, pResult->TEI);
				debug_str(DEBUG_LOG_NET, "EM_CMD_BUSINESS_AREA_DISCERNMENT, cco mac:%02x-%02x-%02x-%02x-%02x-%02x\r\n", 
					      my_mac[5], my_mac[4], my_mac[3], my_mac[2], my_mac[1], my_mac[0]);
				debug_str(DEBUG_LOG_NET, "EM_CMD_BUSINESS_AREA_DISCERNMENT, main mac:%02x-%02x-%02x-%02x-%02x-%02x\r\n", 
					      main_node_mac[5], main_node_mac[4], main_node_mac[3], main_node_mac[2], main_node_mac[1], main_node_mac[0]);
				
				if (pResult->Result==0||pResult->Result>2)//cco隶属未知
				{
					return;
				}
				int me_sn = PLC_get_sn_from_addr(meter_addr);
				if (me_sn != CCTT_METER_NUMBER)
				{
					u8 un_report=1;
					//考虑兼容性，部分厂家不置完成标志，上报结果各异，无法准确判断是否识别完成，故仅在完成时/ 结果属于本台区且地址正确时 置位bitmap，不再查询
					//其他情况无法判断，先记录当前结果（如有未知识别为非本台区，只能通过地址去判断，CCO以STA上报的结果来记录），并再次查询，后续结果更新
					if (pResult->CompleteFlag==1 || (pResult->Result == 1 && (OK == macCmp(main_node_mac, my_mac))) )//结果识别完
					{
						if (g_pRelayStatus->area_dis_cb.getedZoneStatusBitMap[me_sn >> 5] & (1 << (me_sn & 0x1f)))
						{
							un_report=0;
						}
						g_pRelayStatus->area_dis_cb.getedZoneStatusBitMap[me_sn >> 5] |= 1 << (me_sn & 0x1f);
					}
					int cco_num = insertAreaCCO(main_node_mac);
					g_MeterRelayInfo[me_sn].cco_num = cco_num;
#ifdef PROTOCOL_NW_2020
					//多余节点记录台区识别结果
					int outlist_index = FindOutListMeter(meter_addr);
					if (outlist_index < MAX_OUT_LIST_METER)
					{
						OutListMeter.cco_num[outlist_index] = cco_num;
					}
#endif
					if (pResult->Result == 2) //非本台区
					{
						if (OK != macCmp(main_node_mac, my_mac))
						{
//							OS_ERR err;
							
//							while (0 != OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz, OS_OPT_PEND_NON_BLOCKING, NULL, &err));
							if (un_report)
							{
								if (PLM_ReportAreaDiscernment(meter_addr, main_node_mac))
								{
									g_pRelayStatus->area_dis_cb.zoneErrorMap[me_sn >> 5] |= 1 << (me_sn & 0x1f);
								}
								else
								{
									g_pRelayStatus->area_dis_cb.getedZoneStatusBitMap[me_sn >> 5] &=~( 1 << (me_sn & 0x1f));
								}
							}
//							OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz / 2, OS_OPT_PEND_BLOCKING, NULL, &err);
						}
					}
				}

			}
			else if ((0x08 == pInfo->CollectionType)||(0x07 == pInfo->CollectionType))
			{
				u8 mac_addr[6];
				memcpy_swap(mac_addr, pInfo->MacAddr, MAC_ADDR_LEN);
				if (PLC_GetStaInfoByMac(mac_addr) != NULL)
				{
					PLC_AppZeroCrossReport(pInfo->Data, pInfo->HeadLen, pInfo->CollectionType);
				}
			}
#endif//DIS_FUNC
		}
		else if (EM_CMD_BUSINESS_SEND_FILE == business_id)                //升级任务
		{
			if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
				return;
			
			if (BROADCAST_DRT_DOWN == pMacHead->Broadcast)
				return;

			if (pMacHead->SrcTei == CCO_TEI)
			{
				return;
			}
			P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;
			if (pBusinessPacket->Data[0] == GD_UPINFO_SEND_INFO)
			{
				P_GD_UPDATE_START_UP pStartUpdateUp = (P_GD_UPDATE_START_UP)pBusinessPacket->Data;

				debug_str(DEBUG_LOG_UPDATA, "GD_UPINFO_SEND_INFO, src:%d, up id:0x%x-0x%x, need:%d, upGetState:%d, result:%d, pcb state:%d, plan status:%d\r\n",
						  pMacHead->SrcTei, pStartUpdateUp->UpdateId, pcb->update_id, g_pStaStatusInfo[pMacHead->SrcTei].need_up, g_pStaStatusInfo[pMacHead->SrcTei].updateGetStart,
						  pStartUpdateUp->Result, pcb->state, pcb->planStatus);

#if 0
				if (0 != pStartUpdateUp->Result)
				{
					debug_str(DEBUG_LOG_UPDATA, "pStartUpdateUp->UpdateResult = %d\r\n", pStartUpdateUp->Result);
					return;
				}
#endif

				if (pcb->update_id != pStartUpdateUp->UpdateId)
					return;

				if (g_pStaStatusInfo[pMacHead->SrcTei].need_up)
				{
					g_pStaStatusInfo[pMacHead->SrcTei].updateGetStart = 1;
					if (pcb->state == EM_STA_UPDATE_START) //清除等待start节点
					{
						pcb->staBitmap[pMacHead->SrcTei >> 5] &= ~(1 << (pMacHead->SrcTei & 0x1f));
					}
					if (pMacHead->SrcTei == pcb->tei) //得到了当前tei
					{
						pcb->get_current_status = 1;
					}
				}

				return;
			}
			else if (pBusinessPacket->Data[0] == GD_UPINFO_QUERY_STATUS)
			{
				P_GD_UPDATE_STATUS_UP pStatus = (P_GD_UPDATE_STATUS_UP)pBusinessPacket->Data;

				debug_str(DEBUG_LOG_UPDATA, "GD_UPINFO_QUERY_STATUS, src:%d, up id:0x%x-0x%x, up status:%d, need:%d, upGetState:%d, pcb state:%d, plan status:%d, total sta:%d\r\n",
						  pMacHead->SrcTei, pStatus->UpdateID, pcb->update_id, pStatus->UpdateStatus, g_pStaStatusInfo[pMacHead->SrcTei].need_up,
						  g_pStaStatusInfo[pMacHead->SrcTei].updateGetStart, pcb->state, pcb->planStatus, pcb->total_sta);

				if (pcb->update_id != pStatus->UpdateID)
					return;

				u16 bytes = (pcb->block_count + 7) / 8;
				if (pStatus->StartBlockID != 0) //每次询问都是从0开始
				{
					debug_str(DEBUG_LOG_UPDATA, "GD_UPINFO_QUERY_STATUS, error StartBlockID: %d\r\n", pStatus->StartBlockID);
					return;
				}

				debug_str(DEBUG_LOG_UPDATA, "GD_UPINFO_QUERY_STATUS, frame len:%d-%d, bytes:%d\r\n", 
						  pBusinessPacket->FrameLen, sizeof(GD_UPDATE_STATUS_UP), bytes);
				
				if (g_pStaStatusInfo[pMacHead->SrcTei].need_up && g_pStaStatusInfo[pMacHead->SrcTei].updateGetStart)
				{
					if (pStatus->UpdateStatus == 2 || pStatus->UpdateStatus == 5 || pStatus->UpdateStatus == 6 || 
						pStatus->UpdateStatus == 1 || pStatus->UpdateStatus == 0) 
					{
						if(pBusinessPacket->FrameLen == sizeof(GD_UPDATE_STATUS_UP)) //20190317规约，正确全部接收不带数据块bitmap
						{
						    if ((pcb->staBitmap[pMacHead->SrcTei >> 5]) & (1 << (pMacHead->SrcTei & 0x1f)))
                            {
								if (pcb->total_sta)
                                {               
            					    pcb->total_sta--;
                                }
                            }
							pcb->staBitmap[pMacHead->SrcTei >> 5] &= ~(1 << (pMacHead->SrcTei & 0x1f));
							g_pStaStatusInfo[pMacHead->SrcTei].need_up = 0; //已经收全所有的块
							debug_str(DEBUG_LOG_UPDATA, "GD_UPINFO_QUERY_STATUS, src tei:%d recv all\r\n", pMacHead->SrcTei);
						}
                        else if(pBusinessPacket->FrameLen != (sizeof(GD_UPDATE_STATUS_UP)+bytes)) //文件包接收状态报文中文件段长度和升级报文不一致，不做漏包判断
						{
						    if ((pcb->staBitmap[pMacHead->SrcTei >> 5]) & (1 << (pMacHead->SrcTei & 0x1f)))
                            {
                				pcb->get_sta_status_frame++;
                				pcb->staBitmap[pMacHead->SrcTei >> 5] &= ~(1 << (pMacHead->SrcTei & 0x1f));
                            }

							debug_str(DEBUG_LOG_UPDATA, "GD_UPINFO_QUERY_STATUS, src tei:%d len err\r\n", pMacHead->SrcTei);
						}
						else
						{
							u8 done = 1;
							for (int i = 0; i < bytes; i++)
							{
								if ((pcb->state == EM_STA_UPDATE_REQ_STATUS ) || (pcb->state == EM_STA_UPDATE_WAIT_REQ_STATUS))
								{
									pcb->bitmap[i] |= ~pStatus->Bitmap[i];
								}
								if ((i == (bytes - 1)) && done)
								{
									u16 bitnum = pcb->block_count % 8;
                                    if (bitnum == 0) //可以被8整除情况下，最后一个字节8个bit都需要check
                                    {
                                        bitnum = 8;
                                    }
                                    
									u8 bitmap = 0;
									for (int j = 0; j < bitnum; j++)
									{
										bitmap |= 1 << (7 - j);
									}
                                    
									if ((bitmap & pStatus->Bitmap[i]) == bitmap)
									{
                                        if ((pcb->staBitmap[pMacHead->SrcTei >> 5]) & (1 << (pMacHead->SrcTei & 0x1f)))
                                        {
            								if (pcb->total_sta)
                                            {               
                        					    pcb->total_sta--;
                                            }
                                        }
                                        
										g_pStaStatusInfo[pMacHead->SrcTei].need_up = 0; //已经收全所有的块
										debug_str(DEBUG_LOG_UPDATA, "GD_UPINFO_QUERY_STATUS, src tei:%d recv all\r\n", pMacHead->SrcTei);
									}
                                    else
                                    {
                                        done = 0;
                                    }
								}
								else if (done)
								{
									if (pStatus->Bitmap[i] != 0xff)
									{
										done = 0;
									}
								}
							}

                            if ((pcb->state == EM_STA_UPDATE_REQ_STATUS ) || (pcb->state == EM_STA_UPDATE_WAIT_REQ_STATUS))
            				{
                                if (!done) //有漏包
                                {
                                    if ((pcb->staBitmap[pMacHead->SrcTei >> 5]) & (1 << (pMacHead->SrcTei & 0x1f)))
                                    {
                                        pcb->get_sta_status_frame++;
                    				    pcb->staBitmap[pMacHead->SrcTei >> 5] &= ~(1 << (pMacHead->SrcTei & 0x1f));
                                    }
                                }
                            }
						}
					}
					else if ((pcb->state == EM_STA_UPDATE_REQ_STATUS ) || (pcb->state == EM_STA_UPDATE_WAIT_REQ_STATUS))
					{
						if(pBusinessPacket->FrameLen >= sizeof(GD_UPDATE_STATUS_UP))
						{
                            if ((pcb->staBitmap[pMacHead->SrcTei >> 5]) & (1 << (pMacHead->SrcTei & 0x1f)))
                            {
                				pcb->get_sta_status_frame++;
                				pcb->staBitmap[pMacHead->SrcTei >> 5] &= ~(1 << (pMacHead->SrcTei & 0x1f));
                            }

                            if (pBusinessPacket->FrameLen == (sizeof(GD_UPDATE_STATUS_UP)+bytes)) //文件包接收状态报文中文件段长度和升级报文一致时，才判断漏包
                            {
    							for (int i = 0; i < bytes; i++)
    							{
    								pcb->bitmap[i] |= ~pStatus->Bitmap[i];
    							}
                            }
						}
					}
                    
					if ((pcb->state == EM_STA_UPDATE_REQ_STATUS) || (pcb->state == EM_STA_UPDATE_WAIT_REQ_STATUS))
					{
						if (((pcb->get_sta_status_frame * 3) > pcb->total_sta) || (pcb->total_sta == 0) || (pcb->total_sta < 10 && pcb->get_sta_status_frame)) //得到1/3的帧就停止
						{
							memset(pcb->staBitmap, 0, sizeof(pcb->staBitmap));
						}
					}
                    
					if (pcb->state == EM_STA_UPDATE_WAIT_REQ_STATUS)
					{
						if (pMacHead->SrcTei == pcb->tei) //得到了当前tei
						{
							pcb->get_current_status = 1;
						}
					}
				}
				if (g_pStaStatusInfo[pMacHead->SrcTei].updateGetStart != 1 && pStatus->UpdateStatus == 1)
				{
					g_pStaStatusInfo[pMacHead->SrcTei].updateGetStart = 1;
				}
			}

		}
		else if (EM_CMD_BUSINESS_TIMING == business_id)                  //校时命令
		{
		}
		else if (EM_CMD_BUSINESS_COMM_TEST == business_id)               //测试帧
		{

			APP_Test_Frame *pCommTest = (APP_Test_Frame *)pBusinessPacket->Data;
			P_COMM_TEST_PARAM pTestMode = &g_PlmStatusPara.comm_test_param;

			u8 test_mode = pCommTest->testId;

			if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
				return;
			
			if(IsCommTestMode(TEST_MODE_SECUER_TO_UART, NULL))
			{
				return;
			}

			if(GetTickCount64()>(OS_TICK_64)(40*OS_CFG_TICK_RATE_HZ))
			{
				return;
			}

			else if (0x2 == test_mode) //切换频段
			{
				get_change_freq_frame=1;
				HPLC_ChangeFrequence(pCommTest->data[0], TRUE);      //TRUE
			}

			else if (!IsCommTestMode((COMM_TEST_MODE)test_mode, NULL))     //重复收到，以第1次为准
			{

				OS_TICK_64 tick_now = GetTickCount64();

				CPU_SR_ALLOC();
				CPU_CRITICAL_ENTER();

				pTestMode->comm_test_start = tick_now;

#if defined(NW_NEW_TEST)
				pTestMode->comm_test_value = 120; 
#else
				pTestMode->comm_test_value = 10; //10 分钟
#endif                 

				if (test_mode == 0) //回环模式
				{
					pTestMode->comm_test_mode = TEST_MODE_PASS_MPDU_BACK;
					HPLC_ChangeFrequence(pCommTest->data[0]&0xf, TRUE);
#if defined(NW_NEW_TEST)
					HPLC_RegisterTmr(PLC_GotoNormal, NULL, 120*60 * 1000, TMR_OPT_CALL_ONCE);    
#else
                    HPLC_RegisterTmr(PLC_GotoNormal, NULL, 10*60 * 1000, TMR_OPT_CALL_ONCE);    
#endif 
					HPLC_SwitchReceivePin();
				}
				else if (test_mode == 1) //透传模式
				{
#if TEST_COM_MODE_USE_MAC
					pTestMode->comm_test_mode = TEST_MODE_TRAN_MAC_TO_COMM;
#else
					pTestMode->comm_test_mode = TEST_MODE_TRAN_MPDU_TO_COMM;
#endif
					HPLC_ChangeFrequence(pCommTest->data[0]&0xf, TRUE);
#if defined(NW_NEW_TEST)
                    End_SetBaudrate(115200, Parity_No);
					HPLC_RegisterTmr(PLC_GotoNormal, NULL, 120*60 * 1000, TMR_OPT_CALL_ONCE);    
#else
                    HPLC_RegisterTmr(PLC_GotoNormal, NULL, 10*60 * 1000, TMR_OPT_CALL_ONCE);    
#endif                    

                    
					HPLC_SwitchReceivePin();
				}
				else//前期暂时使用其他值代表新的南网测试帧 具体情况要看几台的实际测试方式
				{
					P_COMM_TEST_PACKET pTest = (P_COMM_TEST_PACKET)pCommTest->data;
					u8 test_mode = pTest->Reserve1;
					u16 test_val = pTest->DataLen;
					switch (test_mode)
					{
						
						case APP_TEST_BACK:
						{
							OS_TICK_64 tick_now = GetTickCount64();
							pTestMode->comm_test_start = tick_now;
							pTestMode->comm_test_value = test_val;
							pTestMode->comm_test_mode = TEST_MODE_PASS_MPDU_BACK;

							pTestMode->receive_phase = PHASE_A;     //台体设定在A相
							BPLC_SetTestModeParam();
							HRF_SetTestModeParam();

							HRF_TestModeFix(1);
							//End_SetBaudrate(115200, Parity_No);
							HPLC_SwitchReceivePin();
							debug_str(DEBUG_LOG_NET, "进入测试模式：test_mode=%d, test_val=%d\r\n", test_mode, test_val);
							break;
						}
						case APP_TEST_PHY_THROUGH:
						case APP_TESE_MAC_THROUGH:
						case APP_TESE_HRF_THROUGH://物理层透传
						{
							OS_TICK_64 tick_now = GetTickCount64();



							pTestMode->comm_test_start = tick_now;
							pTestMode->comm_test_value = test_val;
							if (test_mode == APP_TEST_PHY_THROUGH)
							{
								pTestMode->comm_test_mode = TEST_MODE_TRAN_MPDU_TO_COMM;
							}
							else if (test_mode == APP_TESE_HRF_THROUGH)
							{
								pTestMode->comm_test_mode = TEST_MODE_HRF_PACKET_TO_UART;
							}
							else
							{
								pTestMode->comm_test_mode = TEST_MODE_TRAN_MAC_TO_COMM;

							}

							pTestMode->receive_phase = PHASE_A;     //台体设定在A相
							BPLC_SetTestModeParam();
							HRF_SetTestModeParam();


							HRF_TestModeFix(1);
							End_SetBaudrate(115200, Parity_No);
							HPLC_SwitchReceivePin();
							debug_str(DEBUG_LOG_NET, "进入测试模式：test_mode=%d, test_val=%d\r\n", test_mode, test_val);
							break;
						}
						case APP_TESE_CHANGE_PLC: //调频
						{
							g_PlmConfigPara.test_flag |= 1;
							debug_str(DEBUG_LOG_INFO, "hplc set frequence = %d\r\n", test_val);
							HPLC_ChangeFrequence(test_val, FALSE);      //TRUE
							break;
						}
						case APP_TESE_TONEMASK : //设置ToneMask
						{
							debug_str(DEBUG_LOG_INFO, "hplc set tonemask = %d\r\n", test_val);
							//if(0==test_val)
							//BPLC_SetToneMask(const uint32_t * table);
							HPLC_ChangeFrequence(test_val, TRUE);
							HPLC_ChangeToneMask(test_val);
							break;
						}
						case APP_TESE_CHANGE_HRF: //信道切换
						{
							u8 channel = test_val >> 4;
							u8 option = test_val & 0xf;
							debug_str(DEBUG_LOG_INFO, "hrf set channel  = %d\r hrf set option  = %d\r\n", channel, option);
							HPLC_ChangeHrfChannelNotSave(channel, option);      //TRUE
							break;
						}
						case  APP_TESE_PLC2HRF: //PLC回传至RF
						{
							u8 phr_mcs;
							u8 psdu_mcs;
							u8 pbsize;
							OS_TICK_64 tick_now = GetTickCount64();



							phr_mcs = pTest->phr_mcs;
							psdu_mcs = pTest->psdu_mcs;
							pbsize = pTest->pb_size;
							debug_str(DEBUG_LOG_INFO, "hrf set phrmcs  = %d\r hrf set psdumcs  = %d\r hrf set pbsize = %d\r\n", phr_mcs, psdu_mcs, pbsize);
							HPLC_ChangeMCSNotSave(phr_mcs, psdu_mcs, pbsize);
							pTestMode->comm_test_start = tick_now;
							pTestMode->comm_test_value = test_val;
							pTestMode->comm_test_mode = TEST_MODE_PLC_PACKET_TO_HRF;

							pTestMode->receive_phase = PHASE_A;     //台体设定在A相
							BPLC_SetTestModeParam();
							HRF_SetTestModeParam();
							HRF_TestModeFix(1);

							HPLC_SwitchReceivePin();
							debug_str(DEBUG_LOG_NET, "进入测试模式：test_mode=%d, test_val=%d\r\n", test_mode, test_val);
							break;
						}
						case APP_TESE_HRF_BACK:
						{

							OS_TICK_64 tick_now = GetTickCount64();


							pTestMode->comm_test_start = tick_now;
							pTestMode->comm_test_value = test_val;
							pTestMode->comm_test_mode = TEST_MODE_HRF_PACKET_TO_HRF;
							

							pTestMode->receive_phase = PHASE_A;     //台体设定在A相
							BPLC_SetTestModeParam();
							HRF_SetTestModeParam();



							HRF_TestModeFix(1);
							debug_str(DEBUG_LOG_NET, "进入测试模式：test_mode=%d, test_val=%d\r\n", test_mode, test_val);
							break;
						}
						case APP_TESE_HRF_HPLC_BACK: //RF/PLC To RF/PLC
						{

							OS_TICK_64 tick_now = GetTickCount64();


							pTestMode->comm_test_start = tick_now;
							pTestMode->comm_test_value = test_val;
							pTestMode->comm_test_mode = TEST_MODE_HRF_PLC_PACKET_TO_HRF_PLC;

							pTestMode->receive_phase = PHASE_A;     //台体设定在A相
							BPLC_SetTestModeParam();
							HRF_SetTestModeParam();


							HRF_TestModeFix(1);
							HPLC_SwitchReceivePin();
							debug_str(DEBUG_LOG_NET, "进入测试模式：test_mode=%d, test_val=%d\r\n", test_mode, test_val);
							break;
						}
					case SECUER_TO_UART: //加密测试模式需要把加解密的数据发到串口
						{
							OS_TICK_64 tick_now = GetTickCount64();

							OpenClockGate(); //打开加密模块的clock
							pTestMode->comm_test_start = tick_now;
							pTestMode->comm_test_value = test_val;
							pTestMode->comm_test_mode = TEST_MODE_SECUER_TO_UART;
							currentsecuremode = pTest->secure_mode;
							currentsecuremode = currentsecuremode;//解决警告  后续如果增加南网测测试模式这个值记录当前的安全加密模式

							pTestMode->receive_phase = PHASE_A;     //台体设定在A相
							BPLC_SetTestModeParam();
							HRF_SetTestModeParam();



							debug_str(DEBUG_LOG_NET, "进入测试模式：test_mode=%d, test_val=%d\r\n", test_mode, test_val);
							break;
						}
					}
				}

				pTestMode->receive_phase = PHASE_A;     //台体设定在A相
				BPLC_SetTestModeParam();
				HRF_TestModeFix(1);
				CPU_CRITICAL_EXIT();

				if (TEST_MODE_TRAN_MPDU_TO_COMM == test_mode 
					|| TEST_MODE_TRAN_MAC_TO_COMM == test_mode
					|| TEST_MODE_HRF_PACKET_TO_UART == test_mode)
				{
					//End_SetBaudrate(9600, Parity_Even);
				}

			}
		}
		else if (EM_CMD_BUSINESS_STA_RUN_STATUS == business_id) //查询从节点运行状态信息
		{
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pMacHead->SrcTei);

			if (pAppFrame->Port != HPLC_APP_PORT_RCU)
				return;
			
			////////
			//兼容抄控器添加
			P_PLC_MAINTENANCE_NET_CB pcb = &g_pRelayStatus->sta_status_cb;
			if (EM_TASK_READ_WAIT_PLC == pcb->state)
			{
				P_PLC_RSP_MSG_INFO pPlc = (P_PLC_RSP_MSG_INFO)alloc_buffer(__func__,sizeof(PLC_RSP_MSG_HEAD) + pBusinessPacket->FrameLen);

				if (NULL != pPlc)
				{
					pPlc->header.msg_len = pBusinessPacket->FrameLen;
					memcpy_special(pPlc->body, pBusinessPacket->Data, pBusinessPacket->FrameLen);
					free_buffer(__func__,pcb->pPlcRsp);
					pcb->pPlcRsp = pPlc;

				}
			}
			/////////
			if (pSta != NULL)
			{
				int idx = 1;
				int len=0;
				for (int i = 0; i < pBusinessPacket->Data[0]; i++)
				{
					switch (pBusinessPacket->Data[idx])
					{
					case 0:
						++idx;
						len=pBusinessPacket->Data[idx];
						++idx;
						pSta->run_status.runtime = GetTickCount64() / OSCfg_TickRate_Hz - (*(u32 *)&pBusinessPacket->Data[idx]);
						if (!pSta->run_status.runtime)
						{
							pSta->run_status.runtime = 1;
						}
						break;
					case 1:
						++idx;
						len=pBusinessPacket->Data[idx];
						++idx;
						pSta->run_status.zc = pBusinessPacket->Data[idx];
						break;
					case 2:
						++idx;
						len=pBusinessPacket->Data[idx];
						++idx;
						pSta->run_status.u485 = pBusinessPacket->Data[idx];
						break;
					case 3:
						++idx;
						len=pBusinessPacket->Data[idx];
						++idx;
						pSta->run_status.leave = pBusinessPacket->Data[idx];
						break;
					case 4:
						++idx;
						len=pBusinessPacket->Data[idx];
						++idx;
						pSta->run_status.reset = pBusinessPacket->Data[idx];
						break;
					default:
						++idx;
						len=pBusinessPacket->Data[idx];
						++idx;
						break;
					}
					idx+=len;
				}
			}
		}
		else if (EM_CMD_BUSINESS_STA_CHANNEL_INFO == business_id) //查询从节点信道信息
		{
			P_PLC_MAINTENANCE_NET_CB pcb = &g_pRelayStatus->sta_channel_cb;
#ifdef NW_GUANGDONG_NEW_LOCAL_INTERFACE              
            P_PLC_MAINTENANCE_NET_CB new_pcb = &g_pRelayStatus->sta_new_channel_cb;
#endif

			if (pAppFrame->Port != HPLC_APP_PORT_RCU)
				return;
			
			if (EM_TASK_READ_WAIT_PLC == pcb->state)
			{
				P_PLC_RSP_MSG_INFO pPlc = (P_PLC_RSP_MSG_INFO)alloc_buffer(__func__,sizeof(PLC_RSP_MSG_HEAD) + pBusinessPacket->FrameLen);
				if (NULL != pPlc)
				{
                    pPlc->header.msg_len = pBusinessPacket->FrameLen;
					memcpy_special(pPlc->body, pBusinessPacket->Data, pBusinessPacket->FrameLen);
					free_buffer(__func__,pcb->pPlcRsp);
					pcb->pPlcRsp = pPlc;
					return;
				}
			}
#ifdef NW_GUANGDONG_NEW_LOCAL_INTERFACE            
            else if (EM_TASK_READ_WAIT_PLC == new_pcb->state)
			{
				P_PLC_RSP_MSG_INFO pPlc = (P_PLC_RSP_MSG_INFO)alloc_buffer(__func__,sizeof(PLC_RSP_MSG_HEAD) + pBusinessPacket->FrameLen);
				if (NULL != pPlc)
				{
                    P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pMacHead->SrcTei);

                    debug_str(DEBUG_LOG_UPDATA, "EM_CMD_BUSINESS_STA_CHANNEL_INFO, src tei:%d\r\n", pMacHead->SrcTei);
                    
                    if (NULL != pSta)
                    {
                        debug_str(DEBUG_LOG_UPDATA, "EM_CMD_BUSINESS_STA_CHANNEL_INFO, pco tei:%d\r\n", pSta->pco);
                        
                        pPlc->header.msg_len = pBusinessPacket->FrameLen + 10;

                        memcpy_special(&pPlc->body[0], pSta->mac, MAC_ADDR_LEN);
                        *(u16 *)&pPlc->body[6] = pMacHead->SrcTei;
                        *(u16 *)&pPlc->body[8] = pSta->pco;                        
                        memcpy_special(&pPlc->body[10], pBusinessPacket->Data, pBusinessPacket->FrameLen);
                    }
                  
					free_buffer(__func__, new_pcb->pPlcRsp);
					new_pcb->pPlcRsp = pPlc;
					return;
				}
			}
#endif                 
		}
	}
	else if (3 == pBusinessPacket->Ctrl.FrameType) //主动上报帧
	{
		if (1 != pBusinessPacket->Ctrl.StartFlag)
			return;

		if (pBusinessPacket->BusinessID > 4)
			return;

		if (0 == pBusinessPacket->BusinessID) //电表事件
		{
			if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
				return;
		}
		else if (1 == pBusinessPacket->BusinessID || 2 == pBusinessPacket->BusinessID|| 3 == pBusinessPacket->BusinessID) //模块事件，停复电，3:协议转换器上报 
		{
			if (pAppFrame->Port != HPLC_APP_PORT_RCU)
				return;
		}
		else
		{
			return;
		}

		P_EVENT_PACKET pEvent = (P_EVENT_PACKET)pBusinessPacket->Data;
		PLC_HandlesAppEvent(pMacHead->SrcTei,pMacHead->DstTei, pEvent, pBusinessPacket);
	}
	else if (4 == pBusinessPacket->Ctrl.FrameType) //抄控器协议帧
	{
		if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
			return;
		
		if (!g_pRelayStatus->connectReader)
		{
			return;
		}
		g_pRelayStatus->ReaderLink = pHplcParam->Link;
		switch (pBusinessPacket->BusinessID)
		{
		case 0:  //本地接口协议 CCO
			{
				READER_CCO_FRAME *pFrame = (READER_CCO_FRAME *)pBusinessPacket->Data;
				if (pFrame->type != 0)
				{
					return;
				}
				if (pFrame->seq != g_pRelayStatus->seqReader)
				{
					return;
				}
				ReaderGetMsg(pFrame->data, pFrame->len);

				g_pRelayStatus->factory_recv_rssi = pHplcParam->Rssi;

                //如果是抄控器发送的查询主节点接收RSSI命令（AFNF0 E802F241），记录RSSI
                u8 *pLocalFrame = NULL;
                u8 offset = sizeof(LMP_LINK_HEAD_GD) - 1;
                //检查帧格式
                pLocalFrame = (u8 *)PLM_Check_Frame_3762(pFrame->data, pFrame->len);
                if (pLocalFrame)
                {
                    if (((P_LMP_LINK_HEAD)pLocalFrame)->ctl_field & 0x20)
                    {
                        offset += 12;
                    }

                    //匹配数据标识
                    u8 DI[4] = {0x41,0xF2,0x02,0xE8};
                    if ((pLocalFrame[offset] == 0xF0) &&
                        (memcmp(&pLocalFrame[offset+2], DI, 4) == 0))
                    {
                        g_pRelayStatus->recv_rssi = pHplcParam->Rssi;
                    }
                }
			}
			break;
		case 1: //串口转发协议 CCO&STA
			{
				READER_UART_FRAME *pFrame = (READER_UART_FRAME *)pBusinessPacket->Data;
				if (pFrame->type != 0)
				{
					return;
				}
				if (pFrame->seq != g_pRelayStatus->seqReader)
				{
					return;
				}

				g_pRelayStatus->factory_recv_rssi = pHplcParam->Rssi;
				End_ChangeRateSend(pFrame->data, pFrame->len, pFrame->rate);
			}
			break;
		default:
			break;
		}
	}
	else if (14 == pBusinessPacket->Ctrl.FrameType)    //厂家调试
	{
		//
	}
	else if (0xF == pBusinessPacket->Ctrl.FrameType) //生产扩展
	{
		if ((pAppFrame->Port != HPLC_APP_PORT_FACTORY) && (pAppFrame->Port != HPLC_APP_PORT_COMMON)) //HPLC_APP_PORT_COMMON兼容老版本sta,20210406
			return;
		
		if (TEI_CCO != pMacHead->DstTei)
			return;

		if (BROADCAST_DRT_DOWN == pMacHead->Broadcast)
			return;

		if (pBusinessPacket->BusinessID == 0x3B) //查询内部版本
		{
			if(pBusinessPacket->FrameLen == 34) //从节点地址(6)+厂商代码(2)+芯片代码(2)+模板版本(5)+软件版本(7)+硬件版本(7)+内部版本(5)
			{
				u8 node_addr[MAC_ADDR_LEN];

				memcpy(node_addr, pBusinessPacket->Data, MAC_ADDR_LEN);
				PLM_ResponseStaInnerVerInfo(node_addr, pBusinessPacket->Data, pBusinessPacket->FrameLen);
			}
		}
		else if (pBusinessPacket->BusinessID == 0x3E)//查询节点发射功率
		{
			if(pBusinessPacket->FrameLen == 10) //从节点地址(6)+power(4)
			{
				u8 node_addr[MAC_ADDR_LEN];

				memcpy(node_addr, pBusinessPacket->Data, MAC_ADDR_LEN);
				PLM_ResponseStaPower(0, node_addr, pBusinessPacket->Data, pBusinessPacket->FrameLen);
			}
		}
		else if (pBusinessPacket->BusinessID == 0x47)//查询节点rf发射功率
		{
			if(pBusinessPacket->FrameLen == 8) //从节点地址(6)+power(2)
			{
				u8 node_addr[MAC_ADDR_LEN];

				memcpy(node_addr, pBusinessPacket->Data, MAC_ADDR_LEN);
				PLM_ResponseStaRFPower(0, node_addr, pBusinessPacket->Data, pBusinessPacket->FrameLen);
			}
		}
		else if (pBusinessPacket->BusinessID == 0x60)//查询节点发射功率 新接口
		{
			if(pBusinessPacket->FrameLen == 11) //从节点地址(6)+芯片型号代码(1)+power(4)
			{
				u8 node_addr[MAC_ADDR_LEN];

				memcpy(node_addr, pBusinessPacket->Data, MAC_ADDR_LEN);
				PLM_ResponseStaPower(1, node_addr, pBusinessPacket->Data, pBusinessPacket->FrameLen);
			}
		}
		else if (pBusinessPacket->BusinessID == 0x62)//查询节点rf发射功率 新接口
		{
			if(pBusinessPacket->FrameLen == 9) //从节点地址(6)+芯片型号代码(1)+power(2)
			{
				u8 node_addr[MAC_ADDR_LEN];

				memcpy(node_addr, pBusinessPacket->Data, MAC_ADDR_LEN);
				PLM_ResponseStaRFPower(1, node_addr, pBusinessPacket->Data, pBusinessPacket->FrameLen);
			}
		}
		else if (pBusinessPacket->BusinessID == EM_CMD_BUSINESS_REGISTER)//查询节点对应寄存器值
		{
			u8 node_addr[MAC_ADDR_LEN];
			u16 len = *(u16 *)&pBusinessPacket->Data[6] + 8;//加从节点地址(6)+len(2)
			if(pBusinessPacket->FrameLen == len)
			{
				memcpy(node_addr, pBusinessPacket->Data, MAC_ADDR_LEN);
				PLM_ResponseStaRegisterInfo(node_addr,pBusinessPacket->Data, pBusinessPacket->FrameLen);
			}
		}
	}
}

#else


void PLC_HandleAppFrame(P_HPLC_PARAM pHplcParam, P_MAC_HEAD pMacHead, P_APP_PACKET pAppFrame)
{
	if (APP_PACKET_EVENT_REPORT == pAppFrame->PacketID)
	{
		if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
			return;
		
		P_EVENT_PACKET pEvent = (P_EVENT_PACKET)pAppFrame->Data;
		PLC_HandlesAppEvent(pMacHead->SrcTei,pMacHead->DstTei, pEvent);
	}
	else if (APP_PACKET_COLLECT == pAppFrame->PacketID)
	{
		if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
			return;
		
		if (BROADCAST_DRT_DOWN == pMacHead->Broadcast)
		{
			//debug_str("if(BROADCAST_DRT_DOWN == pMacHead->Broadcast)\r\n");
			return;
		}

		READ_METER_UP *pReadUp = (READ_METER_UP *)pAppFrame->Data;

		if (1 != pReadUp->Item.ActiveItem.Direction)
		{
			//debug_str("if(1 != pReadUp->Item.ActiveItem.Direction)\r\n");
			return;
		}

		if (0 != pReadUp->ReplyState)
			return;

		debug_str(DEBUG_LOG_APP, "APP_PACKET_COLLECT\r\n");

//		if (EM_ROUTE_DIR_READ != g_pRelayStatus->read_state)
//			return;

		P_PLC_DIR_READ_CB pcb = &g_pRelayStatus->dir_read_cb;

		if (EM_DIR_READ_WAIT_PLC != pcb->state)
		{
			return;
		}

		//国网测试，原始源TEI不正确，获取不到MAC地址
		BOOL get_mac_addr = TRUE;
		U8 mac_addr[MAC_ADDR_LEN];
#if 1
		get_mac_addr=PLC_GetMacAddrAppFrame(pReadUp->Data, pReadUp->DataLen,mac_addr);
#else
		if (OK != PLC_GetMacAddrByMacFrame(pMacHead, mac_addr))
			get_mac_addr = FALSE;
#endif
		if (get_mac_addr)
		{
#if 1
			if (OK != macCmp(mac_addr, pcb->pPlmCmd->header.mac))
#else
			u8 dst_addr[MAC_ADDR_LEN];
			PLC_GetAppDstMac(pcb->pPlmCmd->header.mac,dst_addr);
			if (OK != macCmp(mac_addr, dst_addr))
#endif				
			{
				debug_str(DEBUG_LOG_INFO, "APP_PACKET_COLLECT MAC ERROR\r\n");    
				return;
			}
		}

		if (pReadUp->PacketSeq !=pcb->pPlmCmd->header.packet_seq_plc )
			return;

		if (EM_DIR_READ_WAIT_PLC == pcb->state)
		{
//            debug_str("R6\r\n");
			P_PLC_RSP_MSG_INFO pPlc = (P_PLC_RSP_MSG_INFO)alloc_buffer(__func__,sizeof(PLC_RSP_MSG_HEAD) + pReadUp->DataLen);
			if (NULL != pPlc)
			{
				pPlc->header.msg_len = pReadUp->DataLen;
				memcpy_special(pPlc->body, pReadUp->Data, pReadUp->DataLen);
				free_buffer(__func__,pcb->pPlcRsp);
				pcb->pPlcRsp = pPlc;
				return;
			}
		}
	}
#ifdef GDW_2019_GRAPH_STOREDATA
	else if ((APP_PACKET_PARALLEL == pAppFrame->PacketID) || (APP_PACKET_STORE_DATA_PARALLEL == pAppFrame->PacketID))
#else
	else if (APP_PACKET_PARALLEL == pAppFrame->PacketID)
#endif		
	{
		if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
			return;
		
		if ((TEI_CCO != pMacHead->DstTei) && (TEI_BROADCAST != pMacHead->DstTei))
			return;

		if (BROADCAST_DRT_DOWN == pMacHead->Broadcast)
			return;

		READ_METER_UP *pReadUp = (READ_METER_UP *)pAppFrame->Data;

		if (0 != pReadUp->ReplyState)
			return;

//        if(0==pReadUp->Item.ActiveConcurrentItem)
//            return;
		//现修改为从应用层报文中获取mac地址进行上报
//		BOOL get_mac_addr = TRUE;
		U8 mac_addr[MAC_ADDR_LEN];
//		if (OK != PLC_GetMacAddrByMacFrame(pMacHead, mac_addr))
//			get_mac_addr = FALSE;

		P_READ_MSG_INFO pReadMsg;
		u16 cnt = get_queue_cnt(g_PlcWaitRxQueue);
#ifdef GDW_JIANGSU
		if(cnt == 0)
		{
		  	if (pReadUp->Protocol != 0)
			{
				READ_MSG_INFO tempReadMsg;
				u8 temp_dst_addr[LONG_ADDR_LEN] = {0};
				P_DL69845_BODY pDL698_temp =  PLC_check_69845_frame(pReadUp->Data, pReadUp->DataLen);
				if (pDL698_temp)
				{
					PLC_get_69845_SAMac(pDL698_temp, mac_addr);
					PLC_GetAppDstMac(mac_addr, temp_dst_addr);
					PLM_PHASE temp_plm_phase = PLC_GetPlmPhaseByMacAddr(temp_dst_addr);
					memcpy(tempReadMsg.header.mac, mac_addr, MAC_ADDR_LEN);
					tempReadMsg.header.meter_protocol_plc = APP_PRO_698_45;
					PLM_ReportParallelRead(&tempReadMsg, temp_plm_phase, pAppFrame->PacketID, pReadUp->Data, pReadUp->DataLen);
				}
			}
		}
#endif	
		while (cnt--)
		{
			pReadMsg = dequeue(g_PlcWaitRxQueue);
			if (NULL == pReadMsg)
				break;

			if (pAppFrame->PacketID != pReadMsg->header.packet_id)
			{
				enqueue(g_PlcWaitRxQueue, pReadMsg);
				continue;
			}
#ifndef GDW_JIANGSU 		
			if (pReadUp->PacketSeq !=pReadMsg->header.packet_seq_plc )
			{
				enqueue(g_PlcWaitRxQueue, pReadMsg);
				continue;
			}
#endif
			if (pReadUp->Protocol != 0) //不是透明传输，需要判断MAC地址
			{
				P_GD_DL645_BODY pDL645 = PLC_check_dl645_frame(pReadUp->Data, pReadUp->DataLen);
				P_DL69845_BODY pDL698 =  PLC_check_69845_frame(pReadUp->Data, pReadUp->DataLen);
				if (pDL645)
				{
					memcpy(mac_addr, pDL645->meter_number, MAC_ADDR_LEN);
				}
				else if (pDL698)
				{
					PLC_get_69845_SAMac(pDL698, mac_addr);
				}
				else
				{
					debug_str(DEBUG_LOG_ERR, "APP_PACKET_PARALLEL MAC ERROR\r\n");
					enqueue(g_PlcWaitRxQueue, pReadMsg);
					continue;
				}

				if (OK != macCmp(mac_addr, pReadMsg->header.mac))
				{
					enqueue(g_PlcWaitRxQueue, pReadMsg);
					continue;
				}
			}

			//上报抄读数据
			u8 dst_addr[LONG_ADDR_LEN] = {0};
			PLC_GetAppDstMac(pReadMsg->header.mac, dst_addr);
			PLM_PHASE plm_phase = PLC_GetPlmPhaseByMacAddr(dst_addr);
			PLM_ReportParallelRead(pReadMsg, plm_phase, pAppFrame->PacketID, pReadUp->Data, pReadUp->DataLen);
			free_buffer(__func__,pReadMsg);

			break;
		}	
	}
	else if (APP_PACKET_ROUTE == pAppFrame->PacketID)
	{
		if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
			return;
		
		if (BROADCAST_DRT_DOWN == pMacHead->Broadcast)
			return;

		READ_METER_UP *pReadUp = (READ_METER_UP *)pAppFrame->Data;

		if (1 != pReadUp->Item.ActiveItem.Direction)
			return;

		if (0 != pReadUp->ReplyState)
			return;

//		if (EM_ROUTE_ROUTE_READ != g_pRelayStatus->read_state)
//			return;

		P_PLC_ROUTE_READ_CB pcb = &g_pRelayStatus->route_read_cb;


		if (EM_ROUTE_READ_WAIT_PLM == pcb->state)
		{
			return;
		}

		BOOL get_mac_addr = TRUE;
		U8 mac_addr[MAC_ADDR_LEN];
#if 1
		get_mac_addr=PLC_GetMacAddrAppFrame(pReadUp->Data, pReadUp->DataLen,mac_addr);
#else
		if (OK != PLC_GetMacAddrByMacFrame(pMacHead, mac_addr))
			get_mac_addr = FALSE;
#endif
		

		if (pReadUp->PacketSeq !=pcb->packet_seq_plc)
			return;

		if (get_mac_addr)
		{
#if 1
			if (OK != macCmp(mac_addr, pcb->plmCmd.node_addr))
#else		
			u8 dst_addr[MAC_ADDR_LEN];
			PLC_GetAppDstMac(pcb->plmCmd.node_addr,dst_addr);
			if (OK != macCmp(mac_addr, dst_addr))
#endif				
			{
				debug_str(DEBUG_LOG_NET, "APP_PACKET_ROUTE MAC ERROR\r\n"); 
				return;
			}
		}

		if (EM_ROUTE_READ_WAIT_PLC == pcb->state)
		{
			P_PLC_RSP_MSG_INFO pPlc = (P_PLC_RSP_MSG_INFO)alloc_buffer(__func__,sizeof(PLC_RSP_MSG_HEAD) + pReadUp->DataLen);
			if (NULL != pPlc)
			{
				pPlc->header.msg_len = pReadUp->DataLen;
				memcpy_special(pPlc->body, pReadUp->Data, pReadUp->DataLen);
				free_buffer(__func__,pcb->pPlcRsp);
				pcb->pPlcRsp = pPlc;
				return;
			}
		}
	}
	else if (APP_PACKET_COMM_TEST == pAppFrame->PacketID)
	{
		if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
			return;
		
		P_COMM_TEST_PACKET pCommTest = (P_COMM_TEST_PACKET)pAppFrame->Data;
		P_COMM_TEST_PARAM pTestMode = &g_PlmStatusPara.comm_test_param;

		u8 test_mode = pCommTest->Reserve1;
		u16 test_val = pCommTest->DataLen;
		if(pTestMode->comm_test_mode == TEST_MODE_SECUER_TO_UART)
		{
			if (test_mode!=TEST_MODE_SECUER_TO_UART)
			{
				P_COMM_TEST_FRAME_PACKET pCommTest = (P_COMM_TEST_FRAME_PACKET)pAppFrame->Data;
				Secure_TestMode(currentsecuremode, &pCommTest->data[0]);
			}
			return;
		}
		if (TEST_MODE_TRAN_APP_FRAME_TO_COMM_ONCE == test_mode)
		{
		}
		else if (TEST_MODE_TRAN_APP_FRAME_TO_HPLC_ONCE == test_mode)
		{
		}
		else if (TEST_MODE_PASS_MPDU_BACK == test_mode)
		{
			OS_TICK_64 tick_now = GetTickCount64();
			pTestMode->comm_test_start = tick_now;
			pTestMode->comm_test_value = test_val;
			pTestMode->comm_test_mode = (COMM_TEST_MODE)test_mode;

			pTestMode->receive_phase = PHASE_A;     //台体设定在A相
			BPLC_SetTestModeParam();
			HRF_SetTestModeParam();

			HRF_TestModeFix(1);
			//End_SetBaudrate(115200, Parity_No);
			HPLC_SwitchReceivePin();
			debug_str(DEBUG_LOG_NET, "进入测试模式：test_mode=%d, test_val=%d\r\n", test_mode, test_val);
		}
		else if ((TEST_MODE_TRAN_MPDU_TO_COMM == test_mode) || (TEST_MODE_TRAN_MAC_TO_COMM == test_mode)) //物理层透传
		{
			OS_TICK_64 tick_now = GetTickCount64();
			
			CPU_SR_ALLOC();
			CPU_CRITICAL_ENTER();

			pTestMode->comm_test_start = tick_now;
			pTestMode->comm_test_value = test_val;
			pTestMode->comm_test_mode = (COMM_TEST_MODE)test_mode;

			pTestMode->receive_phase = PHASE_A;     //台体设定在A相
			BPLC_SetTestModeParam();
			HRF_SetTestModeParam();

			CPU_CRITICAL_EXIT();
			HRF_TestModeFix(1);
			End_SetBaudrate(115200, Parity_No);
			HPLC_SwitchReceivePin();
			debug_str(DEBUG_LOG_NET, "进入测试模式：test_mode=%d, test_val=%d\r\n", test_mode, test_val);
		}
		else if (TEST_MODE_FREQUENCE == test_mode)//调频
		{
			g_PlmConfigPara.test_flag |= 1;
			debug_str(DEBUG_LOG_INFO, "hplc set frequence = %d\r\n", test_val);
			HPLC_ChangeFrequence(test_val, FALSE);      //TRUE
		}
		else if (TEST_MODE_TONEMASK == test_mode)//设置ToneMask
		{
			debug_str(DEBUG_LOG_INFO, "hplc set tonemask = %d\r\n", test_val);
			//if(0==test_val)
			//BPLC_SetToneMask(const uint32_t * table);
			HPLC_ChangeFrequence(test_val, TRUE);
			HPLC_ChangeToneMask(test_val);
		}
		else if(test_mode==TEST_MODE_HRF_CHANNEL_CHANGE)//信道切换
		{
			u8 channel = test_val >> 4;
			u8 option = test_val & 0xf; 
			debug_str(DEBUG_LOG_INFO, "hrf set channel  = %d\r hrf set option  = %d\r\n", channel,option);
			HPLC_ChangeHrfChannelNotSave(channel,option);      //TRUE
		}
		else if(test_mode == TEST_MODE_PLC_PACKET_TO_HRF)//PLC回传至RF
		{
			u8 phr_mcs;
			u8 psdu_mcs;
			u8 pbsize;
			OS_TICK_64 tick_now = GetTickCount64();
			
			CPU_SR_ALLOC();
			CPU_CRITICAL_ENTER();
			
			phr_mcs = pCommTest->phr_mcs;
			psdu_mcs = pCommTest->psdu_mcs;
			pbsize = pCommTest->pb_size;
			debug_str(DEBUG_LOG_INFO, "hrf set phrmcs  = %d\r hrf set psdumcs  = %d\r hrf set pbsize = %d\r\n", phr_mcs,psdu_mcs,pbsize);
			HPLC_ChangeMCSNotSave(phr_mcs,psdu_mcs,pbsize);
			pTestMode->comm_test_start = tick_now;
			pTestMode->comm_test_value = test_val;
			pTestMode->comm_test_mode = (COMM_TEST_MODE)test_mode;

			pTestMode->receive_phase = PHASE_A;     //台体设定在A相
			BPLC_SetTestModeParam();
			HRF_SetTestModeParam();
			HRF_TestModeFix(1);
			CPU_CRITICAL_EXIT();
			HPLC_SwitchReceivePin();
			debug_str(DEBUG_LOG_NET, "进入测试模式：test_mode=%d, test_val=%d\r\n", test_mode, test_val);
		}
		else if((test_mode == TEST_MODE_HRF_PACKET_TO_HRF)||(test_mode == TEST_MODE_HRF_PACKET_TO_UART))//无线回传&无线透传
		{

			OS_TICK_64 tick_now = GetTickCount64();
			
			CPU_SR_ALLOC();
			CPU_CRITICAL_ENTER();

			pTestMode->comm_test_start = tick_now;
			pTestMode->comm_test_value = test_val;
			pTestMode->comm_test_mode = (COMM_TEST_MODE)test_mode;

			pTestMode->receive_phase = PHASE_A;     //台体设定在A相
			BPLC_SetTestModeParam();
			HRF_SetTestModeParam();

			CPU_CRITICAL_EXIT();
			
			if(test_mode == TEST_MODE_HRF_PACKET_TO_UART)
			{
				End_SetBaudrate(115200, Parity_No);
			}
			HRF_TestModeFix(1);
			debug_str(DEBUG_LOG_NET, "进入测试模式：test_mode=%d, test_val=%d\r\n", test_mode, test_val);
			
		}
		else if(test_mode == TEST_MODE_HRF_PLC_PACKET_TO_HRF_PLC)//RF/PLC To RF/PLC
		{

			OS_TICK_64 tick_now = GetTickCount64();
			
			CPU_SR_ALLOC();
			CPU_CRITICAL_ENTER();

			pTestMode->comm_test_start = tick_now;
			pTestMode->comm_test_value = test_val;
			pTestMode->comm_test_mode = (COMM_TEST_MODE)test_mode;

			pTestMode->receive_phase = PHASE_A;     //台体设定在A相
			BPLC_SetTestModeParam();
			HRF_SetTestModeParam();

			CPU_CRITICAL_EXIT();
			HRF_TestModeFix(1);
			HPLC_SwitchReceivePin();
			debug_str(DEBUG_LOG_NET, "进入测试模式：test_mode=%d, test_val=%d\r\n", test_mode, test_val);
		}
		else if (test_mode==TEST_MODE_SECUER_TO_UART)//加密测试模式需要把加解密的数据发到串口 
		{
			OS_TICK_64 tick_now = GetTickCount64();
			
			CPU_SR_ALLOC();
			CPU_CRITICAL_ENTER();
			OpenClockGate();//打开加密模块的clock
			pTestMode->comm_test_start = tick_now;
			pTestMode->comm_test_value = test_val;
			pTestMode->comm_test_mode = (COMM_TEST_MODE)test_mode;
			currentsecuremode = pCommTest->secure_mode;

			pTestMode->receive_phase = PHASE_A;     //台体设定在A相
			BPLC_SetTestModeParam();
			HRF_SetTestModeParam();

			CPU_CRITICAL_EXIT();
			
			debug_str(DEBUG_LOG_NET, "进入测试模式：test_mode=%d, test_val=%d\r\n", test_mode, test_val);
		}
		
	}
	else if (APP_PACKET_CHECK_REGISTER == pAppFrame->PacketID)
	{
		if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
			return;
		
		if (BROADCAST_DRT_DOWN == pMacHead->Broadcast)
		{
			debug_str(DEBUG_LOG_NET, "读到下行搜表报文\r\n");
			return;
		}

		P_REGIST_CHECK_UP pRegResult = (P_REGIST_CHECK_UP)pAppFrame->Data;
		u8 mac_addr[MAC_ADDR_LEN];
#if 0 //def GDW_2019_GRAPH_STOREDATA			
		u8 syncCfgFlag = 0; //第一个电表地址的规约类型为698时，需要重新下发曲线存储数据采集方案
#endif
		memcpy_swap(mac_addr, pRegResult->SrcMac, MAC_ADDR_LEN);

		//PLC_AddToSelfRegList(mac_addr);
		P_REGIST_METER pRegMeter = (P_REGIST_METER)pRegResult->Data;
		debug_str(DEBUG_LOG_NET, "CHECK_REG, srcTei:%d, meterNum:%d\r\n", pMacHead->SrcTei, pRegResult->MeterNum);
#ifdef GDW_JIANGSU 		
		if(pRegResult->Parameter == 5)
		{
			u16 coll_store_tei = PLC_GetStaSnByMac(mac_addr);
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(coll_store_tei);
			pSta->minute_coll=pRegResult->Data[0] & 0x01;
		}
		else
#endif		  
		{

			for (u16 i = 0; i < pRegResult->MeterNum; i++)
			{
				PLC_AddToSelfRegList(pRegMeter->MeterAddr);     //第1块表与mac_addr一样
				u16 m_sn = PLC_get_sn_from_addr(pRegMeter->MeterAddr);

				debug_str(DEBUG_LOG_NET, "CHECK_REG, index %d, mac:%02x%02x%02x%02x%02x%02x, sn:%d\r\n", i,
						  pRegMeter->MeterAddr[5], pRegMeter->MeterAddr[4], pRegMeter->MeterAddr[3],
						  pRegMeter->MeterAddr[2], pRegMeter->MeterAddr[1], pRegMeter->MeterAddr[0], m_sn);
				
				if (m_sn < CCTT_METER_NUMBER)
				{
					g_MeterRelayInfo[m_sn].coll_sn = pMacHead->SrcTei;
					switch (pRegMeter->Protocol)
					{
					case 1:
						g_MeterRelayInfo[m_sn].meter_type.protocol = METER_TYPE_PROTOCOL_97;
						break;
					case 2:
						g_MeterRelayInfo[m_sn].meter_type.protocol = METER_TYPE_PROTOCOL_07;
						break;
					case 3:
#if 0 //def GDW_2019_GRAPH_STOREDATA
						if ((i == 0) && (g_MeterRelayInfo[m_sn].meter_type.protocol != METER_TYPE_PROTOCOL_69845))
						{
							syncCfgFlag = 1;
						}
#endif
						g_MeterRelayInfo[m_sn].meter_type.protocol = METER_TYPE_PROTOCOL_69845;
						break;
					default:
						g_MeterRelayInfo[m_sn].meter_type.protocol = METER_TYPE_PROTOCOL_TRANSPARENT;
						break;
					}
				}

				pRegMeter++;
			}

			u16 tei = PLC_GetStaSnByMac(mac_addr);
			if (tei <= TEI_STA_MAX)
			{
				P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
				pStaStatus->selfreg_search_node = 1;

#if 0 //def GDW_2019_GRAPH_STOREDATA
				P_PLC_STORE_DATA_SYNC_CFG_CB syncCfgPcb = &g_pRelayStatus->store_data_sync_cfg_cb;
				u8 syncCfgExist = 0; //采集方案是否存在

				if (syncCfgFlag)
				{
					g_pStaStatusInfo[tei].need_sync_cfg = 2;
					g_pStaStatusInfo[tei].sync_cfg_result = 0;

					if (syncCfgPcb->meter_698_data_len != 0)
					{
						syncCfgExist = 1;
					}

					if (syncCfgExist)
					{
						//没有采集方案时，不置bitmap标志
							syncCfgPcb->sta_bitmap[0][tei >> 5] |= 1 << (tei & 0x1f);
							syncCfgPcb->sta_bitmap[1][tei >> 5] |= 1 << (tei & 0x1f);

						if ((syncCfgPcb->state == EM_STORE_DATA_SYNC_CFG_INIT) ||
							(syncCfgPcb->state == EM_STORE_DATA_SYNC_CFG_RESTART)) //直接切换成单播方式下发配置
						{
							if (syncCfgPcb->round < 3)
							{
								syncCfgPcb->round++;
							}
							syncCfgPcb->tei = tei;
							syncCfgPcb->tick_send = 0;
							syncCfgPcb->send_counter = 0;
							syncCfgPcb->state = EM_STORE_DATA_SYNC_CFG_SEND_UNICAST;
						}
						else if ((syncCfgPcb->state == EM_STORE_DATA_SYNC_CFG_SEND_UNICAST) ||
								 (syncCfgPcb->state == EM_STORE_DATA_SYNC_CFG_WAIT_UNICAST))
						{
							//控制块正在执行单播流程时，如果tei小于当前控制块执行的tei，则控制块执行完操作后从最小tei开始再执行一遍round
							//防止关联请求入网的tei没有马上执行到，要等到EM_STORE_DATA_SYNC_CFG_RESTART过后才能执行
							if (tei < syncCfgPcb->tei)
							{
								syncCfgPcb->restart_immediate = 1;
							}
						}
					}
				}
#endif
			}
			if (0 == pRegResult->State)        //STA已搜表完成
			{
				if (pRegResult->PacketSeq != g_pRelayStatus->selfreg_cb.packet_seq_self_reg)
				{
					debug_str(DEBUG_LOG_NET, "搜表报文序号错误\r\n");
					return;
				}
				if ((EM_SELFREG_INIT == g_pRelayStatus->selfreg_cb.state) || (g_pRelayStatus->selfreg_cb.state >= EM_SELFREG_REPORT_STOP))
				{
					g_pRelayStatus->get_sn_cb.bitmap[tei>>5]&=~(1<<(tei&0x1f));
					debug_str(DEBUG_LOG_NET, "搜表已结束\r\n");
					return;
				}
				P_PLC_SELFREG_CB pcb  = &g_pRelayStatus->selfreg_cb;
				if (tei <= TEI_STA_MAX && (!(pcb->upBitmap[tei >> 3] & (1 << (tei & 0x7)))))
				{
					g_pRelayStatus->get_sn_cb.bitmap[tei>>5]&=~(1<<(tei&0x1f));
					if (PLM_ReportSelfReg(pRegResult))
					{
						pcb->upBitmap[tei >> 3] |= 1 << (tei & 0x7);
					}
					else
					{
						P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
						pStaStatus->selfreg_search_node = 1;
					}
				}
			}
		}
	}
	else if (APP_PACKET_START_UPDATE == pAppFrame->PacketID)
	{
		if (pAppFrame->Port != HPLC_APP_PORT_UPDATE)
			return;
		
		if (BROADCAST_DRT_DOWN == pMacHead->Broadcast)
			return;

		P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;

		P_UPDATE_START_UP pStartUpdateUp = (P_UPDATE_START_UP)pAppFrame->Data;

		debug_str(DEBUG_LOG_UPDATA, "APP_PACKET_START_UPDATE, src tei:%d, need up:%d, result:%d, pcb state:%d, plan status:%d\r\n",
		          pMacHead->SrcTei, g_pStaStatusInfo[pMacHead->SrcTei].need_up, pStartUpdateUp->UpdateResult, pcb->state, pcb->planStatus);

		if (0 != pStartUpdateUp->UpdateResult)
		{
			return;
		}

		if (pcb->update_id != pStartUpdateUp->UpdateID)
			return;

		if (g_pStaStatusInfo[pMacHead->SrcTei].need_up)
		{
			g_pStaStatusInfo[pMacHead->SrcTei].updateGetStart = 1;
			if (pcb->state == EM_STA_UPDATE_START ) //清除等待start节点
			{
				pcb->staBitmap[pMacHead->SrcTei >> 5] &= ~(1 << (pMacHead->SrcTei & 0x1f));
			}
			if (pMacHead->SrcTei == pcb->tei) //得到了当前tei
			{
				pcb->get_current_status = 1;
			}
		}

		return;

	}
	else if (APP_PACKET_CHECK_UPDATE == pAppFrame->PacketID)
	{
		if (pAppFrame->Port != HPLC_APP_PORT_UPDATE)
			return;
		
		if (BROADCAST_DRT_DOWN == pMacHead->Broadcast)
			return;

		P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;

		P_UPDATE_STATUS_UP pStatus = (P_UPDATE_STATUS_UP)pAppFrame->Data;

		debug_str(DEBUG_LOG_NET, "APP_PACKET_CHECK_UPDATE, src tei:%d, need up:%d, updateGetState:%d, pcb state:%d\r\n",
		          pMacHead->SrcTei, g_pStaStatusInfo[pMacHead->SrcTei].need_up, g_pStaStatusInfo[pMacHead->SrcTei].updateGetStart, pcb->state);

//		if (EM_STA_UPDATE_WAIT_REQ_STATUS != pcb->state)
//		{
//			debug_str("APP_PACKET_CHECK_UPDATE != pcb->state\r\n");
//			return;
//		}

		//debug_str("block_count=%d, BlockCountValid = %d\r\n", pcb->block_count, pStatus->BlockCountValid);

//		if (pcb->block_count != pStatus->BlockCountValid)
//			return;

		if (pcb->update_id != pStatus->UpdateID)
			return;

//		U8 mac_addr[MAC_ADDR_LEN];
//		if (OK != PLC_GetMacAddrByMacFrame(pMacHead, mac_addr))
//			return;
//
//		if (OK != macCmp(mac_addr, g_pStaInfo[pcb->tei].mac))
//			return;

		u16 bytes = (pStatus->BlockCountValid + 7) / 8;

		if (pStatus->BlockCountValid > pcb->block_count)
		{
			debug_str(DEBUG_LOG_ERR, "APP_PACKET_CHECK_UPDATE, error block count: %d-%d, bytes=%d\r\n", pStatus->BlockCountValid, pcb->block_count, bytes);
			return;
		}

        if (pStatus->StartBlockID != 0) //每次询问都是从0开始
		{
		    debug_str(DEBUG_LOG_ERR, "APP_PACKET_CHECK_UPDATE, error StartBlockID: %d\r\n", pStatus->StartBlockID);
			return;
		}

        if (g_pStaStatusInfo[pMacHead->SrcTei].need_up && g_pStaStatusInfo[pMacHead->SrcTei].updateGetStart)
		{
			debug_str(DEBUG_LOG_NET, "APP_PACKET_CHECK_UPDATE, bolck count valid:%d, block_count:%d, bytes:%d\r\n",
			          pStatus->BlockCountValid, pcb->block_count, bytes);

			if (pStatus->UpdateStatus!=1)//不是接收态
			{
				if ((pcb->staBitmap[pMacHead->SrcTei >> 5]) & (1 << (pMacHead->SrcTei & 0x1f)));
				{
					if (pcb->total_sta)
                    {               
					    pcb->total_sta--;
                    }
                    
                    pcb->staBitmap[pMacHead->SrcTei >> 5] &= ~(1 << (pMacHead->SrcTei & 0x1f));
				}
				g_pStaStatusInfo[pMacHead->SrcTei].need_up = 0; //已经收全所有的块
			}
			else if (pStatus->BlockCountValid == pcb->block_count) //有效的块数与总块数一致
			{
				u8 done = 1;
				for (int i = 0; i < bytes; i++)
				{
					if ((pcb->state == EM_STA_UPDATE_REQ_STATUS ) || (pcb->state == EM_STA_UPDATE_WAIT_REQ_STATUS))
					{
						pcb->bitmap[i] |= ~pStatus->Bitmap[i];
					}
					if ((i == (bytes - 1)) && done)
					{
						u16 bitnum = pStatus->BlockCountValid % 8;
                        if (bitnum == 0) //可以被8整除情况下，最后一个字节8个bit都需要check
                        {
                            bitnum = 8;
                        }
                        
						u8 bitmap = 0;
						for (int j = 0; j < bitnum; j++)
						{
							bitmap |= 1 << j;
						}
                        
						if ((bitmap & pStatus->Bitmap[i]) == bitmap)
						{
							if ((pcb->staBitmap[pMacHead->SrcTei >> 5]) & (1 << (pMacHead->SrcTei & 0x1f)))
                            {
								if (pcb->total_sta)
                                {               
            					    pcb->total_sta--;
                                }
                            }
							
							g_pStaStatusInfo[pMacHead->SrcTei].need_up = 0; //已经收全所有的块
							debug_str(DEBUG_LOG_UPDATA, "APP_PACKET_CHECK_UPDATE, src tei:%d recv all\r\n", pMacHead->SrcTei);
						}
                        else
                        {
                            done = 0;
                        }
					}
					else if (done)
					{
						if (pStatus->Bitmap[i] != 0xff)
						{
							done = 0;
						}
					}
				}

                if ((pcb->state == EM_STA_UPDATE_REQ_STATUS ) || (pcb->state == EM_STA_UPDATE_WAIT_REQ_STATUS))
				{
                    if (!done) //有漏包
                    {
                        if ((pcb->staBitmap[pMacHead->SrcTei >> 5]) & (1 << (pMacHead->SrcTei & 0x1f)))
                        {
                            pcb->get_sta_status_frame++;
        				    pcb->staBitmap[pMacHead->SrcTei >> 5] &= ~(1 << (pMacHead->SrcTei & 0x1f));
                        }
                    }
                }
			}
			else if ((pcb->state == EM_STA_UPDATE_REQ_STATUS ) || (pcb->state == EM_STA_UPDATE_WAIT_REQ_STATUS))
			{
			    if ((pcb->staBitmap[pMacHead->SrcTei >> 5]) & (1 << (pMacHead->SrcTei & 0x1f)))
                {
    				pcb->get_sta_status_frame++;
    				pcb->staBitmap[pMacHead->SrcTei >> 5] &= ~(1 << (pMacHead->SrcTei & 0x1f));
                }
                
				for (int i = 0; i < bytes; i++)
				{
					pcb->bitmap[i] |= ~pStatus->Bitmap[i];
				}
			}
            
			if ((pcb->state == EM_STA_UPDATE_REQ_STATUS ) || (pcb->state == EM_STA_UPDATE_WAIT_REQ_STATUS))
			{
				if (((pcb->get_sta_status_frame * 3) > pcb->total_sta) || (pcb->total_sta == 0) || (pcb->total_sta < 10 && pcb->get_sta_status_frame)) //得到1/3的帧就停止
				{
					debug_str(DEBUG_LOG_NET, "APP_PACKET_CHECK_UPDATE clear staBitmap, total sta:%d, get sta status frame:%d\r\n", pcb->total_sta, pcb->get_sta_status_frame);
					memset(pcb->staBitmap, 0, sizeof(pcb->staBitmap));
				}
			}
            
			if (pcb->state == EM_STA_UPDATE_WAIT_REQ_STATUS)
			{
				if (pMacHead->SrcTei == pcb->tei) //得到了当前tei
				{
					pcb->get_current_status = 1;
				}
			}

		}

//		memset_special(pcb->bitmap, 0, pcb->block_count + 1);
//		memcpy_special(pcb->bitmap, pStatus->Bitmap, bytes);
//
//		BOOL send_all = TRUE;
//
//		for (u16 i = 0; i < pcb->block_count; i++)
//		{
//			if (0 == (pcb->bitmap[i / 8] & (1 << (i % 8))))
//			{
//				send_all = FALSE;
//				break;
//			}
//		}
//
//		if (EM_STA_UPDATE_WAIT_REQ_STATUS == pcb->state)
//		{
//			if (send_all)
//			{
//				debug_str("pcb->state = EM_STA_UPDATE_EXE\r\n");
//				pcb->state = EM_STA_UPDATE_EXE;
//				pcb->plc_send_counter = CON_PLC_RESEND_COUNT;
//			}
//			else
//			{
//				debug_str("pcb->state = EM_STA_UPDATE_DATA\r\n");
//				pcb->state = EM_STA_UPDATE_DATA;
//			}
//		}
//		else
//		{
//			debug_str("APP_PACKET_CHECK_UPDATE\r\n");
//		}
	}
	else if (APP_PACKET_CHECK_STA == pAppFrame->PacketID)
	{
		if (pAppFrame->Port != HPLC_APP_PORT_UPDATE)
			return;
		
		if (BROADCAST_DRT_DOWN == pMacHead->Broadcast)
			return;

		debug_str(DEBUG_LOG_NET, "rsp = APP_PACKET_CHECK_STA\r\n");

		P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;

		P_UPDATE_STA_INFO_UP pInfo = (P_UPDATE_STA_INFO_UP)pAppFrame->Data;

		U8 mac_addr[MAC_ADDR_LEN];
		if (OK != PLC_GetMacAddrByMacFrame(pMacHead, mac_addr))
			return;

		if (OK != macCmp(mac_addr, g_pStaInfo[pcb->tei].mac))
			return;

//		pcb->read_sta_info = TRUE;

		if (pcb->update_id != pInfo->UpdateID)
			return;

		u16 p = 0;
		for (u8 i = 0; i < pInfo->ElementCount; i++)
		{
			u8 id = pInfo->Elements[p++];
			u8 len = pInfo->Elements[p++];
			if (EM_STA_CRC == id)
			{
				if (EM_STA_UPDATE_WAIT_REQ_INFO == pcb->state)
					memcpy_special(&pcb->file_checksum_sta, &pInfo->Elements[p], 4);
			}
			else if (EM_STA_FILE_SIZE == id)
			{
				if (EM_STA_UPDATE_WAIT_REQ_INFO == pcb->state)
					memcpy_special(&pcb->file_size_sta, &pInfo->Elements[p], 4);
			}
			p += len;
		}
	}
	else if (APP_PACKET_AREA_DISCERNMENT == pAppFrame->PacketID)
	{
#ifndef DIS_FUNC
		if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
			return;
		
		P_PLC_AREA_DISCERNMENT pcb = &g_pRelayStatus->area_dis_cb;

		P_AREA_DISCERNMENT_UP pInfo = (P_AREA_DISCERNMENT_UP)pAppFrame->Data;

		if (0 == pInfo->Direction)
			return;

		if (0x05 == pInfo->CollectionType)//结果告知
		{
			P_AREA_DISCERNMENT_RESULT_DATA pResult = (P_AREA_DISCERNMENT_RESULT_DATA)pInfo->Data;

			u8 meter_addr[MAC_ADDR_LEN];
			u8 main_node_mac[MAC_ADDR_LEN];
			u8 my_mac[MAC_ADDR_LEN];
			if (EM_AREA_DISCERNMENT_INIT == pcb->state)
				return;
			PLC_CopyCCOMacAddr(my_mac, FALSE);
			memcpy_swap(meter_addr, pInfo->MacAddr, MAC_ADDR_LEN);
			memcpy_swap(main_node_mac, pResult->MainNodeMacAddr, MAC_ADDR_LEN);
			int me_sn = PLC_get_sn_from_addr(meter_addr);

			debug_str(DEBUG_LOG_NET, "APP_PACKET_AREA_DISCERNMENT, sta:%02x-%02x-%02x-%02x-%02x-%02x, result:%d, CompleteFlag:%d, TEI:%d\r\n", 
				      meter_addr[5], meter_addr[4], meter_addr[3], meter_addr[2], meter_addr[1], meter_addr[0], pResult->Result, pResult->CompleteFlag, pResult->TEI);
			debug_str(DEBUG_LOG_NET, "APP_PACKET_AREA_DISCERNMENT, cco mac:%02x-%02x-%02x-%02x-%02x-%02x\r\n", 
				      my_mac[5], my_mac[4], my_mac[3], my_mac[2], my_mac[1], my_mac[0]);
			debug_str(DEBUG_LOG_NET, "APP_PACKET_AREA_DISCERNMENT, main mac:%02x-%02x-%02x-%02x-%02x-%02x\r\n", 
				      main_node_mac[5], main_node_mac[4], main_node_mac[3], main_node_mac[2], main_node_mac[1], main_node_mac[0]);
			
			if (pResult->Result==0||pResult->Result>2)//cco隶属未知
			{
				return;
			}
			if (me_sn != CCTT_METER_NUMBER)
			{
				g_MeterRelayInfo[me_sn].cco_num = insertAreaCCO(main_node_mac);
				u8 un_report=1;
				//考虑兼容性，部分厂家不置完成标志，上报结果各异，无法准确判断是否识别完成，故仅在完成时/ 结果属于本台区且地址正确时 置位bitmap，不再查询
				//其他情况无法判断，先记录当前结果（如有未知识别为非本台区，只能通过地址去判断，CCO以STA上报的结果来记录），并再次查询，后续结果更新
				if (pResult->CompleteFlag==1 || (pResult->Result == 1 && (OK == macCmp(main_node_mac, my_mac))) )//结果识别完
				{
					if(g_pRelayStatus->area_dis_cb.getedZoneStatusBitMap[me_sn >> 5] & (1 << (me_sn & 0x1f)))
					{
						un_report=0;
					}
					g_pRelayStatus->area_dis_cb.getedZoneStatusBitMap[me_sn >> 5] |= 1 << (me_sn & 0x1f);
				}
			
				if (pResult->Result==2)//非本台区
				{
					if (OK != macCmp(main_node_mac, my_mac))
					{
//						OS_ERR err;
//						while (0 != OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz, OS_OPT_PEND_NON_BLOCKING, NULL, &err));
						if(un_report)
						{
							if(PLM_ReportAreaDiscernment(meter_addr, main_node_mac))
							{
								g_pRelayStatus->area_dis_cb.zoneErrorMap[me_sn >> 5] |= 1 << (me_sn & 0x1f);
							}
							else
							{
								g_pRelayStatus->area_dis_cb.getedZoneStatusBitMap[me_sn >> 5] &= ~(1 << (me_sn & 0x1f));
							}
						}
//						OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz / 2, OS_OPT_PEND_BLOCKING, NULL, &err);
					}
				}
			}
		}
		else if (0x03 == pInfo->CollectionType) //cco用于相线识别
		{
			PLC_AppZeroCrossReport(pInfo->Data, 0, 0);
		}
#endif //DIS_FUNC
	}
	else if (APP_PACKET_READER_CONNECT == pAppFrame->PacketID) //抄控器协议帧
	{
		if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
			return;
		
		if (!g_pRelayStatus->connectReader)
		{
			return;
		}
		g_pRelayStatus->ReaderLink = pHplcParam->Link;
		READER_CCO_FRAME *pFrame = (READER_CCO_FRAME *)pAppFrame->Data;
		if (pFrame->type != 0)
		{
			return;
		}
		ReaderGetMsg(pFrame->data, pFrame->len);
	
        //如果是抄控器发送的查询主节点接收RSSI命令（AFNF0 F241），记录RSSI
        u8 *pLocalFrame = NULL;
        u8 offset = sizeof(LMP_LINK_HEAD) - 1;
        //检查帧格式
        pLocalFrame = (u8 *)PLM_Check_Frame_3762(pFrame->data, pFrame->len);
        if (pLocalFrame)
        { 
            if (((P_LMP_LINK_HEAD)pLocalFrame)->Inf.down_info.comm_module)
            {
                offset += 12 + 6*(((P_LMP_LINK_HEAD)pLocalFrame)->Inf.down_info.relay_level);
            }

            //匹配数据标识
            u8 Fn[2] = {0x01,0x1E};
            if ((pLocalFrame[offset] == 0xF0) &&
                (memcmp(&pLocalFrame[offset+1], Fn, 2) == 0))
            {
                g_pRelayStatus->recv_rssi = pHplcParam->Rssi;
            }
        }
	}
#ifndef HPLC_CSG
	else if (APP_PACKET_GET_ID == pAppFrame->PacketID)
	{
		if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
			return;
		
		READ_ID * pStaID=(READ_ID*)pAppFrame->Data;
		P_STA_INFO pSta;
		if (pStaID->Dir == 0 || pMacHead->DstTei != CCO_TEI)
		{
			return;
		}
		if (pStaID->Id_type!=2&&pStaID->Id_type!=0)
		{
			return;
		}
		Moudle_ID_Info *pInfo = (Moudle_ID_Info*)pStaID->Data;
		if (pInfo->len > sizeof(pInfo->id_data))
		{
			pInfo->len = sizeof(pInfo->id_data);
		}
		pSta = PLC_GetValidStaInfoByTei(pMacHead->SrcTei);
		if (pSta!=NULL)
		{
#if 0
			u8 *p = &pInfo->len;
			p-=8;
			p[0] = pInfo->device_type;
			memcpy_special(&p[1], pSta->mac, LONG_ADDR_LEN);
			p[7]=2;
			g_pRelayStatus->get_moudle_id_cb.tei=0;
			ReportIDMsg(p,20);
#else
			pSta->geted_id = 1;
			//P_RELAY_INFO p_info= PLC_GetWhiteNodeInfoByMac(pSta->mac);
			if (pSta)
			{
				memset(pSta->module_id, 0x00, sizeof(pSta->module_id));
				memcpy(pSta->module_id, pInfo->id_data, pInfo->len);
			}
#endif
            PLC_GET_MOUDLE_ID_CB* pcb = &g_pRelayStatus->get_moudle_id_realtime_cb;
				
			if (EM_TASK_READ_WAIT_PLC == pcb->state)
			{
				u8 dst_addr[LONG_ADDR_LEN] = {0};
				PLC_GetAppDstMac(pcb->mac_addr, dst_addr); //兼容采集器下接电表
				if (macCmp(dst_addr, pSta->mac))
				{
					if (0xFFFF != pcb->seq)  //去重
					{
						P_PLC_RSP_MSG_INFO pPlc = (P_PLC_RSP_MSG_INFO)alloc_buffer(__func__, sizeof(PLC_RSP_MSG_HEAD)+sizeof(Moudle_ID_Info));
						if (NULL != pPlc)
						{
						    pPlc->header.msg_len = sizeof(Moudle_ID_Info);
				            memcpy_special(pPlc->body, pInfo, sizeof(Moudle_ID_Info));
							free_buffer(__func__, pcb->pPlcRsp);
							pcb->pPlcRsp = pPlc;
							return;
						}
					}
				}
			}
		}
	}
#endif
#ifdef GDW_2019_GRAPH_STOREDATA
	else if (APP_PACKET_STORE_DATA_SYNC_CFG == pAppFrame->PacketID)
	{
		if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
			return;
		
		u8 mac_addr[MAC_ADDR_LEN] = {0}, bcast_mac[MAC_ADDR_LEN] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
		u16 tei = 0;
		P_PLC_STORE_DATA_SYNC_CFG_CB pcb = &g_pRelayStatus->store_data_sync_cfg_cb;
		P_STORE_DATA_BCAST_SYNC_CFG_UP pSyncCfgUp = (P_STORE_DATA_BCAST_SYNC_CFG_UP)pAppFrame->Data;

		//判断目的MAC地址
		memcpy_swap(mac_addr, pSyncCfgUp->DstMac, MAC_ADDR_LEN);
		if ((memcmp(mac_addr, g_PlmConfigPara.main_node_addr, 6) != 0)&&
			(memcmp(mac_addr, bcast_mac, 6) != 0))
		{
			return;
		}

		//判断源MAC地址
		memcpy_swap(mac_addr, pSyncCfgUp->SrcMac, MAC_ADDR_LEN);
		tei = PLC_GetStaSnByMac(mac_addr);
		if (tei >= TEI_CNT)
		{
			return;
		}
		if (g_pStaStatusInfo[tei].sync_cfg_result==0)
		{
		g_pStaStatusInfo[tei].sync_cfg_result = (pSyncCfgUp->Result==0)?1:0;
		}
		if(g_pStaStatusInfo[tei].sync_cfg_result)//只有sta配置成功后才会真正清除STA,可能出现同一协议配置两次的情况（为预防sta的协议和cco的不一致）
		{
			if (pSyncCfgUp->Seq == pcb->packet_seq[0])
			{
				pcb->sta_bitmap[0][tei >> 5] &= ~(1 << (tei & 0x1f));
		}
			else if (pSyncCfgUp->Seq == pcb->packet_seq[1])
			{
				pcb->sta_bitmap[1][tei >> 5] &= ~(1 << (tei & 0x1f));
	}
		}
	}
	else if (APP_PACKET_GET_STA_EDGE == pAppFrame->PacketID)
	{
		GET_STA_EDGE_UP * pStaEdge=(GET_STA_EDGE_UP*)pAppFrame->Data;
		P_STA_INFO pSta;
		u8 mac_addr[MAC_ADDR_LEN] = {0};

		if (pAppFrame->Port != HPLC_APP_PORT_COMMON)
			return;
		
		memcpy_swap(mac_addr, pStaEdge->DstMac, MAC_ADDR_LEN);
		if ((memcmp(mac_addr, g_PlmConfigPara.main_node_addr, 6) != 0)&&
			(memIsHex(mac_addr, 0xff, 6) != 0))
		{
			return;
		}
		
		memcpy_swap(mac_addr, pStaEdge->SrcMac, MAC_ADDR_LEN);
		pSta = PLC_GetStaInfoByMac(mac_addr);
		if (pSta!=NULL)
		{
			pSta->phase_edge = !pStaEdge->phase;//cco的0是上升沿1是下降沿
			pSta->phase_edge_ask =1;//已经拿到sta 的边沿情况
			if (pStaEdge->meter_type==1)//1为三相表
			{
				pSta->phy_phase|=1<<7;//设置三相表标志
			}
		}
	}
#endif	
	else if (APP_PACKET_EXT_FACTORY_TEST == pAppFrame->PacketID) //生产扩展
	{
		if (pAppFrame->Port != HPLC_APP_PORT_FACTORY)
			return;
		
		if (TEI_CCO != pMacHead->DstTei)
			return;

		if (BROADCAST_DRT_DOWN == pMacHead->Broadcast)
			return;

		READ_INNER_VER *pReadVerUp = (READ_INNER_VER *)pAppFrame->Data;

		if(pReadVerUp->Function == 0x3B) //查询内部版本
		{
			if(pReadVerUp->DataLen == 11) //从节点地址(6)+版本日期(3)+版本号(2)
			{
				u8 node_addr[MAC_ADDR_LEN];

				memcpy(node_addr, pReadVerUp->Mac, MAC_ADDR_LEN);
				PLM_ResponseStaInnerVerInfo(node_addr, pReadVerUp->Mac, pReadVerUp->DataLen);
			}
		}
		else if (pReadVerUp->Function == 0x3E)//查询节点plc发射功率
		{
			P_STA_POWER pStaPower=(P_STA_POWER)pAppFrame->Data;
			if(pStaPower->DataLen == 10) //从节点地址(6)+power(4)
			{
				PLM_ResponseStaPower(0, pStaPower->Mac, pStaPower->Mac, 10);
			}
		}
		else if (pReadVerUp->Function == 0x30)//查询节点rf发射功率
		{
			P_STA_POWER pStaPower=(P_STA_POWER)pAppFrame->Data;
			if(pStaPower->DataLen == 8) //从节点地址(6)+power(2)
			{
				PLM_ResponseStaRFPower(0, pStaPower->Mac, pStaPower->Mac, 8);
			}
		}
		else if (pReadVerUp->Function == 0x60)//查询节点plc发射功率 新接口
		{
			P_STA_POWER pStaPower=(P_STA_POWER)pAppFrame->Data;
			if(pStaPower->DataLen == 11) //从节点地址(6)+芯片型号代码(1)+power(4)
			{
				PLM_ResponseStaPower(1, pStaPower->Mac, pStaPower->Mac, 11);
			}
		}
		else if (pReadVerUp->Function == 0x62)//查询节点rf发射功率 新接口
		{
			P_STA_POWER pStaPower=(P_STA_POWER)pAppFrame->Data;
			if(pStaPower->DataLen == 9) //从节点地址(6)+芯片型号代码(1)+power(2)
			{
				PLM_ResponseStaRFPower(1, pStaPower->Mac, pStaPower->Mac, 9);
			}
		}
		else if (pReadVerUp->Function == 0x70)//查询节点对应寄存器值
		{
			u8 node_addr[MAC_ADDR_LEN];
			u16 len = *(u16 *)pReadVerUp->Data + 8;//加从节点地址(6)+len(2)

			if(pReadVerUp->DataLen == len)
			{
				memcpy(node_addr, pReadVerUp->Mac, MAC_ADDR_LEN);
				PLM_ResponseStaRegisterInfo(node_addr,pReadVerUp->Mac, pReadVerUp->DataLen);
			}
		}
	}
}

#endif

void PLC_SetReceivePhase(const P_HPLC_PARAM pReceiveParam)
{
	if (pReceiveParam->SofSrcTei >= TEI_STA_MIN && pReceiveParam->SofSrcTei <= TEI_STA_MAX)
	{
		P_STA_INFO pStaIfo = PLC_GetValidStaInfoByTei(pReceiveParam->SofSrcTei);
		if (NULL != pStaIfo)
		{
			//还可以比较信道质量
			if (pReceiveParam->Snr > pStaIfo->snr)
			{
				pStaIfo->snr = pReceiveParam->Snr;
				pStaIfo->receive_phase = (u8)pReceiveParam->receive_phase;
			}
		}
	}
}

void PLC_HandleMacFrame(P_RECEIVE_MAC pReceiveMac)
{
	P_MAC_FRAME pMacFrame = (P_MAC_FRAME)&pReceiveMac->macFrame[0];
	u8 *pMsdu = MacFrameGetMsduPtr(pMacFrame,NULL);

	PLC_SetReceivePhase(&pReceiveMac->hplcParam);

	switch (MacFrameGetMsduType(pMacFrame))
	{
	case MSDU_NET_MANAGER:
		{
			//P_MSDU_HEAD_LONG_CSG
			PLC_HandleMMFrame(&pMacFrame->head, (P_MM_FRAME)pMsdu, &pReceiveMac->hplcParam);
			break;
		}
	case MSDU_APP_FRAME:
		{
			P_APP_PACKET pApp;
		#if defined(HPLC_CSG)
			if (MAC_HEAD_LONG == pMacFrame->head.HeadType)
				pApp = (P_APP_PACKET)(pMsdu + sizeof(MSDU_HEAD_LONG_CSG));
			else
				pApp = (P_APP_PACKET)(pMsdu + sizeof(MSDU_HEAD_SHORT_CSG));
		#else
			pApp = (P_APP_PACKET)pMsdu;
		#endif
			if (GetTickCount64()>(OS_TICK_64)(40*OS_CFG_TICK_RATE_HZ)//越过开始的30S允许进入测试模式的时间
				&&!IsCommTestMode(TEST_MODE_SECUER_TO_UART, NULL))//不是安全模式
			{
				if (pMacFrame->head.SrcTei==0)
				{
					break;
				}
			}
			PLC_HandleAppFrame(&pReceiveMac->hplcParam, &pMacFrame->head, pApp);
			break;
		}
	#if !defined(HPLC_CSG)
	case MSDU_IP_FRAME:
		{
			break;
		}
	#endif
	default:
		{
			break;
		}
	}
	//处理 

}
void PLC_HandleOneHopMacFrame(P_RECEIVE_MAC pReceiveMac,u16 stei,u16 dtei)
{

	MAC_HEAD_ONE_HOP* pFrame = (MAC_HEAD_ONE_HOP*)&pReceiveMac->macFrame[0];

	switch (pFrame->MsduType)
	{
	case MSDU_NEIGHBOR:
		{
			PLC_HandleHrfNodeList( (u8 *)(pFrame + 1), pFrame->MsduLen, &pReceiveMac->hplcParam);
			break;
		}
	case MSDU_ONE_HOP_APP_FRAME:
		{
			P_APP_PACKET pApp = (P_APP_PACKET)(pFrame + 1);

			if (APP_PACKET_EVENT_REPORT == pApp->PacketID)
			{
				if (pApp->Port != HPLC_APP_PORT_COMMON)
					return;
				#ifdef HPLC_CSG
				P_APP_BUSINESS_PACKET pBusinessPacket = (P_APP_BUSINESS_PACKET)pApp->Data;
				P_EVENT_PACKET pEvent = (P_EVENT_PACKET)pBusinessPacket->Data;
				PLC_HandlesAppEvent(stei, dtei, pEvent,pBusinessPacket);
				#else
				P_EVENT_PACKET pEvent = (P_EVENT_PACKET)pApp->Data;
				PLC_HandlesAppEvent(stei, dtei, pEvent);
				#endif
				
			}
			break;
		}
	default:
		{
			break;
		}
	}
}


void HrfConfilctProc(u8 *ccoMac,u32  nid,u8 option,u8 channel)
{
	int idx = insertNeighbor(ccoMac, 0, nid);
	if (idx==HRF_CCO_MAX)//未找到mac地址
	{
		return;
	}
	uint64_t tick_now = GetTickCount64();
	
	HrfCcoNeighbor[idx].option = option;
	HrfCcoNeighbor[idx].channel = channel;
	
	

	if (!g_pRelayStatus->change_hrf_channel_cb.second_down)
	{
		if(tick_now-HrfCcoNeighbor[idx].lastTick>50*60*1000)
		{
			HrfCcoNeighbor[idx].firstTick=0;//过长时间没有接到这个CCO的信标帧
		}
		if (g_PlmConfigPara.hrf_allowChangeChannel
			&&option == g_PlmConfigPara.hrf_option 
			&& channel == g_PlmConfigPara.hrf_channel)
		{
			int cmpVaile = macVaileCmp(g_PlmConfigPara.main_node_addr, HrfCcoNeighbor[idx].cco_mac);
			if (HrfCcoNeighbor[idx].firstTick)
			{
				if (HrfCcoNeighbor[idx].firstTick < tick_now)//协商时间到
				{
					//比较mac地址

					if (cmpVaile<0||//本cco的mac地址较小
						tick_now-HrfCcoNeighbor[idx].firstTick  > 120 * 1000//120s以内再次发现冲突则改变信道
						) 
					{
						u8 bitmap[8] = { 0x0, 0x0, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x0f }; //代表所有的可选
						u8 bitindex = (g_PlmConfigPara.hrf_option - 1) * 20 + g_PlmConfigPara.hrf_channel;
						bitmap[bitindex >> 3] &= ~(1 << (bitindex & 0x7));
						Hrf2IdelChannel(tick_now,bitmap);
						HrfCcoNeighbor[idx].firstTick=0;
					}

				}


			}
			else
			{
				HrfCcoNeighbor[idx].firstTick = tick_now + rand_range(5, 55) * 1000;//防止机台意外超市 随机时间设置为5-55s
			}

		}
		else
		{
			HrfCcoNeighbor[idx].firstTick = 0;
		}
	}
	else
	{
		HrfCcoNeighbor[idx].firstTick = 0;
	}
	HrfCcoNeighbor[idx].lastTick=tick_now;
}






unsigned char PLC_GetDirReadCount()
{
	u8 counter = 0;
	u16 cnt = get_queue_cnt(g_PlcWaitRxQueue);

	while (cnt--)
	{
		P_READ_MSG_INFO pReadMsg = (P_READ_MSG_INFO)dequeue(g_PlcWaitRxQueue);

		if (APP_PACKET_COLLECT != pReadMsg->header.packet_id)
		{
			enqueue(g_PlcWaitRxQueue, pReadMsg);
			continue;
		}
		enqueue(g_PlcWaitRxQueue, pReadMsg);
		counter++;
	}

	return counter;
}


typedef struct {
//	u8 meter_addr[LONG_ADDR_LEN];
	u8 mac_addr[LONG_ADDR_LEN];
}ParallelMac_Rem_s;
ParallelMac_Rem_s ParallelMac[PLC_MAX_PARALLEL_NUM];

u8 PLC_FindParallelMac(ParallelMac_Rem_s* temp_mac)//查找正在发送的任务mac
{
	u8 dst_addr[LONG_ADDR_LEN];
	memset(temp_mac,0,sizeof(ParallelMac_Rem_s)*PLC_MAX_PARALLEL_NUM);
	u16 cnt = get_queue_cnt(g_PlcWaitRxQueue);
	int idx=0;
	while (cnt--)
	{
		P_READ_MSG_INFO pReadMsg = (P_READ_MSG_INFO)dequeue(g_PlcWaitRxQueue);
		if (NULL == pReadMsg)
			break;
		enqueue(g_PlcWaitRxQueue, pReadMsg);
		if (pReadMsg->header.read_time==0)
		{
			continue ;
		}
		PLC_GetAppDstMac(pReadMsg->header.mac, dst_addr);
		memcpy(&temp_mac[idx++],dst_addr,LONG_ADDR_LEN);
		if (idx==PLC_MAX_PARALLEL_NUM)
		{
			return idx;
		}
	}
	return idx;
}

bool PLC_AddParallelMac(u8 *meter, ParallelMac_Rem_s *temp_mac)
{
	u8 i = 0, dst_addr[LONG_ADDR_LEN] = {0};
	
	PLC_GetAppDstMac(meter, dst_addr);
	
	for (i=0; i<PLC_MAX_PARALLEL_NUM; i++)
	{
		if (memcmp(&temp_mac[i], dst_addr, LONG_ADDR_LEN) == 0)
		{
			return true;
		}
	}

	if(i == PLC_MAX_PARALLEL_NUM)
	{
		for (i=0; i<PLC_MAX_PARALLEL_NUM; i++)
		{
			if (memIsHex((u8 *)&temp_mac[i], 0x00, LONG_ADDR_LEN))
			{
				memcpy(&temp_mac[i], dst_addr, LONG_ADDR_LEN);
				return true;
			}
		}
	}

	return false;
}

bool PLC_IsRepeatMac(u8 *meter,ParallelMac_Rem_s* temp_mac)
{
	u8 dst_addr[LONG_ADDR_LEN];
	PLC_GetAppDstMac(meter, dst_addr);
	for (int i=0;i<PLC_MAX_PARALLEL_NUM;i++)
	{
		if (memcmp(&temp_mac[i],dst_addr,LONG_ADDR_LEN)==0)
		{
			return true;
		}
	}
	return false;
}

void PLC_RemoveRxQueueOldItem()
{
	OS_TICK_64 tick_now = GetTickCount64();
	u8 idx=PLC_FindParallelMac(ParallelMac);
	u16 cnt = get_queue_cnt(g_PlcWaitRxQueue);
	while (cnt--)
	{
		P_READ_MSG_INFO pReadMsg = (P_READ_MSG_INFO)dequeue(g_PlcWaitRxQueue);

		if (NULL == pReadMsg)
			break;
		if (pReadMsg->header.read_time==0  //并未开始发送
			&&PLC_IsRepeatMac(pReadMsg->header.mac,ParallelMac))//与正在发送的重复
		{
			enqueue(g_PlcWaitRxQueue, pReadMsg);
			continue ;
		}
		if (idx<PLC_MAX_PARALLEL_NUM)
		{
			PLC_AddParallelMac(pReadMsg->header.mac, ParallelMac);
		}
		if (TicksBetween64(tick_now, pReadMsg->header.ts_send) >= pReadMsg->header.ts_timeout)
		{
#ifdef GDW_2019_GRAPH_STOREDATA
			if ((APP_PACKET_PARALLEL == pReadMsg->header.packet_id) || (APP_PACKET_STORE_DATA_PARALLEL == pReadMsg->header.packet_id))
#else

			if (APP_PACKET_PARALLEL == pReadMsg->header.packet_id)
#endif				
			{
				if ((pReadMsg->header.read_count * MAX_MINOR_LOOP_READ_METER) > pReadMsg->header.read_time)
				{
					PLC_PostParallelRead(pReadMsg, 0);
				}
				else
				{
					//上报抄读超时
					u8 dst_addr[LONG_ADDR_LEN] = {0};
					PLC_GetAppDstMac(pReadMsg->header.mac, dst_addr);
					PLM_PHASE plm_phase = PLC_GetPlmPhaseByMacAddr(dst_addr);
					PLM_ReportParallelRead(pReadMsg, plm_phase, pReadMsg->header.packet_id, NULL, 0);
					free_buffer(__func__,pReadMsg);
				}
				continue;
			}
			else
			{
				free_buffer(__func__,pReadMsg);
				continue;
			}
		}

		enqueue(g_PlcWaitRxQueue, pReadMsg);
	}
}

unsigned char PLC_SendDirRead(P_READ_MSG_INFO pReadMsg)
{
#if defined(HPLC_CSG)

	__BKPT(1);
	return (!OK);

#else

	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu = NULL;
	USHORT msdu_len = 0;
	u8 datmac[LONG_ADDR_LEN];
	PLC_GetAppDstMac(pReadMsg->header.mac,datmac);
	USHORT tei = PLC_GetStaSnByMac(datmac);
	u32 tei_phase = tei;

	if (tei >= TEI_CNT)
	{
#if defined(SUPPORT_READ_METER_NOT_IN_WHITELIST) && !defined(OVERSEA)	
		if (pReadMsg->header.packet_id == APP_PACKET_COLLECT
			#ifdef GDW_HEILONGJIANG
			||pReadMsg->header.packet_id == APP_PACKET_PARALLEL
			#endif
			) //集中器主动抄表
		{
			tei = TEI_BROADCAST;
			tei_phase = TEI_BROADCAST;
		}
		else
#endif			
		{
			goto L_Error;
		}
	}
	else
	{
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
		if (NULL == pSta)
			goto L_Error;

#ifdef OVERSEA
		if (pSta->net_state != NET_IN_NET)
		{
			goto L_Error;
		}
#endif

		if ((NET_IN_NET != pSta->net_state) || pReadMsg->header.broadcast)
		{
			tei = TEI_BROADCAST;
		}
		if ((NET_IN_NET != pSta->net_state))
		{
			tei_phase = TEI_BROADCAST;
		}

#ifndef OVERSEA		
		if (IsOrphanNode(tei))
		{
			tei = TEI_BROADCAST;
			tei_phase = TEI_BROADCAST;
		}
#endif		
	}

	if (pReadMsg->header.read_time == 0)
	{
		pReadMsg->header.packet_seq_plc = ++g_pRelayStatus->packet_seq_read_meter;
	}
	pAppFrame = HplcApp_MakeReadMeterFrame((HPLC_APP_PACKET_ID)(pReadMsg->header.packet_id), pReadMsg->header.packet_seq_plc, (u8 *)pReadMsg->body, pReadMsg->header.msg_len, (APP_METER_PRO)pReadMsg->header.meter_protocol_plc, &msdu_len);
	pMsdu = (u8 *)pAppFrame;

	if (NULL == pMsdu)
		goto L_Error;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 3;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_READ_METER;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	send_param.doubleSend=1;
	if ((pReadMsg->header.read_time + 1) % MAX_MINOR_LOOP_READ_METER)
	{
		PLC_GetSendParam(tei_phase, (u16 *)&tei_phase, &send_param.send_phase,&send_param.link);
				
	}


	if (TEI_BROADCAST == tei)
	{
		send_param.resend_count = 3;
		send_param.send_type = CSMA_SEND_TYPE_CSMA;
		send_param.cb = NULL;
		SOF_param.DstTei = TEI_BROADCAST;
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		//mac_param.SendType = SENDTYPE_BROADCAST;
#ifdef DLMS_BROADCAST
		mac_param.mac_flag = MACFLAG_INCLUDE;
#else
		mac_param.mac_flag = MACFLAG_NONE;
#endif
		mac_param.Broadcast = BROADCAST_DRT_DOWN;

		PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
		memcpy_special(mac_param.DstMac, datmac, MAC_ADDR_LEN);
		send_param.link=FreamPathBoth;
		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
		
	}
	else
	{
		PLC_GetSendParam(tei, &SOF_param.DstTei, (HPLC_SEND_PHASE *)&tei_phase,&send_param.link);
		SOF_param.BroadcastFlag = 0;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.mac_flag = MACFLAG_NONE;
		mac_param.Broadcast = BROADCAST_DRT_DUL;
		PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
		memcpy_special(mac_param.DstMac, datmac, MAC_ADDR_LEN);

		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	}

	free_buffer(__func__,pMsdu);

	return OK;

L_Error:
	free_buffer(__func__,pMsdu);
	return (!OK);

#endif
}

unsigned char PLC_SendSelfRegStart()
{
#ifndef HPLC_CSG
	USHORT msdu_len;
	P_APP_PACKET pAppFrame = NULL;

	pAppFrame = HplcApp_MakeStartSelfRegFrame(g_pRelayStatus->selfreg_cb.packet_seq_self_reg);
	if (NULL == pAppFrame)
		goto L_Error;

	msdu_len = sizeof(APP_PACKET) - 1 + sizeof(REGIST_START_DOWN);

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 9;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	send_param.link = FreamPathBoth;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_SELF_REG;
	SOF_param.DstTei = TEI_BROADCAST;

	mac_param.DstTei = TEI_BROADCAST;
	mac_param.SendType = SENDTYPE_PCOBROADCAST;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
#if 1
	mac_param.mac_flag = MACFLAG_NONE;      //华为
#else
	mac_param.mac_flag = MACFLAG_INCLUDE;
	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, BROADCAST_MAC_ADDR, MAC_ADDR_LEN);
#endif
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

//    send_param.send_phase = PHASE_B;
//    HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8*)pAppFrame, msdu_len);
//
//    send_param.send_phase = PHASE_C;
//    HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8*)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
#else
	return OK;
#endif
}

u8 PLC_SendAreaDiscernmentStart(u16 tei, u32 NTB, u8 seq)
{
	USHORT msdu_len;
	P_APP_PACKET pAppFrame = NULL;
#ifdef HPLC_CSG
	u8 *pMsdu;
#endif
	u8 mac_addr[MAC_ADDR_LEN];
	AREA_DISCERNMENT_START_DATA data;
	P_PLC_AREA_DISCERNMENT pcb = &g_pRelayStatus->area_dis_cb;

#if 1
	PLC_CopyCCOMacAddr(mac_addr, FALSE);
#else	
	if (tei == TEI_BROADCAST) //正常的台区识别流程
	{
		memcpy_special(mac_addr, BroadCastMacAddr, MAC_ADDR_LEN);       //测试用例要求填广播地址
	}
	else //台区识别或相线识别
	{
//		PLC_CopyCCOMacAddr(mac_addr, FALSE);
		memcpy_special(mac_addr, PLC_GetValidStaInfoByTei(tei)->mac, MAC_ADDR_LEN);
	}
#endif
	data.NTB = NTB;
	data.CollectionPeriod = 0;
	data.CollectionNum = MAX_ZC_NUM_PER_PHASE;
	data.CollectionSeq = seq;
	data.Reserve = 0;

	msdu_len = sizeof(data);
	pAppFrame = HplcApp_MakeAreaDiscernmentFrame(pcb->packet_seq++, mac_addr, 3, 1, (u8 *)&data, &msdu_len);
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;

	if (SOF_param.DstTei == TEI_BROADCAST)
	{
		if (tei!=TEI_BROADCAST)
		{
			goto L_Error;
		}
		SOF_param.BroadcastFlag = 1;
		mac_param.DstTei = TEI_BROADCAST;
		mac_param.SendType = SENDTYPE_BROADCAST;
		mac_param.SendLimit = 4;
		send_param.link=FreamPathBoth;

	}
	else
	{
		SOF_param.BroadcastFlag = 0;
		mac_param.DstTei = tei;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.SendLimit = 3;
		send_param.send_phase |= SEND_PHASE_ALL;
	}
	SOF_param.NID = g_pPlmConfigPara->net_id;

	SOF_param.Lid = SOF_LID_AREA_DISCERNMENT;



	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.mac_flag = MACFLAG_NONE;
	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	if (SOF_param.DstTei == TEI_BROADCAST) 
	{
		memcpy_special(mac_param.DstMac, BroadCastMacAddr, MAC_ADDR_LEN);
	}
	else 
	{
		memcpy_special(mac_param.DstMac, PLC_GetValidStaInfoByTei(tei)->mac, MAC_ADDR_LEN);
	}

#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}
u8 PLC_SendAreaDiscernmentCollect(u16 tei)
{
	USHORT msdu_len;
	P_APP_PACKET pAppFrame = NULL;
#ifdef HPLC_CSG
	u8 *pMsdu;
#endif
	u8 mac_addr[MAC_ADDR_LEN];
	P_PLC_AREA_DISCERNMENT pcb = &g_pRelayStatus->area_dis_cb;
	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
	//PLC_CopyCCOMacAddr(mac_addr, FALSE);
	if (tei == TEI_BROADCAST) //正常的台区识别流程
	{
		memcpy_special(mac_addr, BroadCastMacAddr, MAC_ADDR_LEN);       //测试用例要求填广播地址
	}
	else if (pSta != NULL) //台区识别或相线识别
	{
		memcpy_special(mac_addr, pSta->mac, MAC_ADDR_LEN);
	}
	else
	{
		return OK;
	}

	msdu_len = 0;
	pAppFrame = HplcApp_MakeAreaDiscernmentFrame(pcb->packet_seq++, mac_addr, 3, 2, NULL, &msdu_len);
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;

	if (SOF_param.DstTei == TEI_BROADCAST)
	{
		if (tei!=TEI_BROADCAST)
		{
			goto L_Error;
		}
		send_param.link=FreamPathBoth;
		SOF_param.BroadcastFlag = 1;
		mac_param.DstTei = TEI_BROADCAST;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.SendLimit = 4;

	}
	else
	{
		SOF_param.BroadcastFlag = 0;
		mac_param.DstTei = tei;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.SendLimit = 3;
		send_param.send_phase |= SEND_PHASE_ALL;
	}
	SOF_param.NID = g_pPlmConfigPara->net_id;

	SOF_param.Lid = SOF_LID_AREA_DISCERNMENT;



	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.mac_flag = MACFLAG_NONE;
	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, mac_addr, MAC_ADDR_LEN);

#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}
//问询台区信息
u8 PLC_SendAreaDiscernmentQuery(u16 tei)
{
	USHORT msdu_len;
	P_APP_PACKET pAppFrame = NULL;
#ifdef HPLC_CSG
	u8 *pMsdu;
#endif
	u8 mac_addr[MAC_ADDR_LEN];
	P_PLC_AREA_DISCERNMENT pcb = &g_pRelayStatus->area_dis_cb;


	if (tei == TEI_BROADCAST) 
	{
		memcpy_special(mac_addr, BroadCastMacAddr, MAC_ADDR_LEN);       
	}
	else 
	{
//		PLC_CopyCCOMacAddr(mac_addr, FALSE);
		memcpy_special(mac_addr, PLC_GetValidStaInfoByTei(tei)->mac, MAC_ADDR_LEN);
	}



	msdu_len = 0;
	pAppFrame = HplcApp_MakeAreaDiscernmentFrame(pcb->packet_seq++, mac_addr, 3, 4, NULL, &msdu_len);
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;

	if (SOF_param.DstTei == TEI_BROADCAST)
	{
		if (tei!=TEI_BROADCAST)
		{
			goto L_Error;
		}
		SOF_param.BroadcastFlag = 1;
		mac_param.DstTei = TEI_BROADCAST;
		mac_param.SendType = SENDTYPE_BROADCAST;
		mac_param.SendLimit = 4;
		send_param.link=FreamPathBoth;

	}
	else
	{
		SOF_param.BroadcastFlag = 0;
		mac_param.DstTei = tei;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.SendLimit = 3;
		send_param.send_phase |= SEND_PHASE_ALL;
	}
	SOF_param.NID = g_pPlmConfigPara->net_id;

	SOF_param.Lid = SOF_LID_AREA_DISCERNMENT;



	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.mac_flag = MACFLAG_NONE;
	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	if (SOF_param.DstTei == TEI_BROADCAST) 
	{
		memcpy_special(mac_param.DstMac, BroadCastMacAddr, MAC_ADDR_LEN);
	}
	else 
	{
		memcpy_special(mac_param.DstMac, PLC_GetValidStaInfoByTei(tei)->mac, MAC_ADDR_LEN);
	}

#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}

#ifdef HPLC_CSG
//新要求使用应用层的台区识别扩展字段进行相线识别
u8 PLC_SendAppPhaseDiscernment(u16 tei)
{
	USHORT msdu_len;
	P_APP_PACKET pAppFrame = NULL;
#ifdef HPLC_CSG
	u8 *pMsdu;
#endif
	u8 mac_addr[MAC_ADDR_LEN];
	APP_PHASE_DISCERNMENT_START_DATA data;
	P_PLC_AREA_DISCERNMENT pcb = &g_pRelayStatus->area_dis_cb;

	PLC_CopyCCOMacAddr(mac_addr, FALSE);


	data.CollectionNum = 36;
	data.CollectionSeq = rand_special();
	data.res = 0;

	msdu_len = sizeof(data);
	pAppFrame = HplcApp_MakeAreaDiscernmentFrame(pcb->packet_seq++, mac_addr, 3, 6, (u8 *)&data, &msdu_len);
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif
	u16 sof_tei;
	PLC_GetSendParam(tei, &sof_tei, &send_param.send_phase,&send_param.link);//获取sta是否可达
	send_param.doubleSend=1; 
	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_AREA_DISCERNMENT;


	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;
	mac_param.DstTei = tei;
	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);

	if (TEI_BROADCAST == sof_tei)
	{
		if (tei!=TEI_BROADCAST)
		{
			goto L_Error;
		}
		SOF_param.DstTei = TEI_BROADCAST;
		SOF_param.BroadcastFlag = 1;
		mac_param.DstTei = TEI_BROADCAST;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.mac_flag = MACFLAG_NONE;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;
		send_param.link = FreamPathBoth;
		memcpy_special(mac_param.DstMac, BroadCastMacAddr, MAC_ADDR_LEN);
	}
	else
	{
		PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
		SOF_param.BroadcastFlag = 0;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.mac_flag = MACFLAG_NONE;
		mac_param.Broadcast = BROADCAST_DRT_DUL;
		memcpy_special(mac_param.DstMac, PLC_GetValidStaInfoByTei(tei)->mac, MAC_ADDR_LEN);
	}


#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}
#endif


//使用第一相线的50个值来做平均
static int get_arg_diff(void)
{
	phaseRecognizeType* pzc=ZC_NTB;
	int start=pzc->Widx-60;
	int sum=0;
	if (start<0)
	{
		start=MAX_ZC_RECOGNIZE_PHASE-60;
	}

	for (int i = 0; i < 50; i++)
	{
		int ntb_diff = pzc->zc_ntb[i +start+1] - pzc->zc_ntb[i +start];
		ntb_diff -= NTB_FRE_PER_MS * 20;
		sum+=ntb_diff;
	}
	sum/=50;
	return sum>>3;
}


u8 PLC_SendAreaDiscernmentFeature()
{
	static OS_TICK_64 first_tick=0;
	if (first_tick==0)
	{
		first_tick=GetTickCount64()+1000*60*2;//前2分钟进行调整应对机台
	}
	int fix_diff=0;
	if (GetTickCount64()<=first_tick)
	{
		fix_diff=get_arg_diff();
	}
	g_PlmConfigPara.test_flag|=0x4;
	if (g_PlmConfigPara.test_flag==0x7)
	{
		//设置默认频段为2以期待下一次可以通过
		SYS_STOR_FREQ(2);
	}
	USHORT msdu_len;
#ifdef HPLC_CSG
	u8 *pMsdu;
#endif
	CPU_SR_ALLOC();
	int i;
	P_APP_PACKET pAppFrame = NULL;
	P_AREA_DISCERNMENT_FEATURE_DATA pdata = NULL;
	P_PLC_AREA_DISCERNMENT pcb = &g_pRelayStatus->area_dis_cb;
	u16 len = sizeof(AREA_DISCERNMENT_FEATURE_DATA) - 1 + 4 + 2 * 4;
	pdata = (P_AREA_DISCERNMENT_FEATURE_DATA)alloc_buffer(__func__,len + 2 * (CON_FEATURE_COUNT_PER_PHASE * 3 * 2));
    if (NULL == pdata)
        return (!OK);   
	u8 *p = &pdata->Other[0];
	u8 *pNum[3];
	int idx[6];
	u32 ntbP, ntbN = pcb->start_ntb;
//	ntbN -= NTB_FRE_PER_MS * 20 * (CON_FEATURE_COUNT_PER_PHASE + 6);
	ntbP = ntbN;
	//以A B C 的顺序为第一出线相位
	for (i = 0; i < 3;)
	{
		idx[i * 2] = findPerZcIdx(&ZC_NTB[i * 2], ntbP);

		if (idx[i * 2] != MAX_ZC_RECOGNIZE_PHASE)
		{
			++i;
			break;
		}
		++i;
	}
	if (idx[(i - 1) * 2] != MAX_ZC_RECOGNIZE_PHASE)
	{
		ntbP = ZC_NTB[(i - 1) * 2].zc_ntb[idx[(i - 1) * 2]];
		for (; i < 3; i++)
		{
			idx[i * 2] = findPerZcIdx(&ZC_NTB[i * 2], ntbP);
		}
	}

	for (i = 0; i < 3;)
	{
		idx[i * 2 + 1] = findPerZcIdx(&ZC_NTB[i * 2 + 1], ntbN);

		if (idx[i * 2 + 1] != MAX_ZC_RECOGNIZE_PHASE)
		{
			++i;
			break;
		}
		++i;
	}
	if (idx[(i - 1) * 2 + 1] != MAX_ZC_RECOGNIZE_PHASE)
	{
		ntbN = ZC_NTB[(i - 1) * 2 + 1].zc_ntb[idx[(i - 1) * 2 + 1]];
		for (; i < 3; i++)
		{
			idx[i * 2 + 1] = findPerZcIdx(&ZC_NTB[i * 2 + 1], ntbN);
		}
	}

	if (NULL == pdata)
		goto L_Error;


	u8 mac_addr[MAC_ADDR_LEN];
	PLC_CopyCCOMacAddr(mac_addr, FALSE);

	pdata->TEI = TEI_CCO;
	pdata->CollectionType = 3;      //双沿采集
	pdata->Reserve = 0;
	pdata->CollectionSeq = pcb->collection_seq;
	pdata->Total = 0; //(u8)(CON_FEATURE_COUNT_PER_PHASE * 3);



	OS_CRITICAL_ENTER();

	//下升沿
	pdata->NTB1 = ntbN;
	*p++ = 0;     //保留
#if 0
	*p++ = (u8)CON_FEATURE_COUNT_PER_PHASE;     //第一出线告知数量
	*p++ = (u8)CON_FEATURE_COUNT_PER_PHASE;     //第二出线告知数量
	*p++ = (u8)CON_FEATURE_COUNT_PER_PHASE;     //第三出线告知数量
#else
	pNum[0] = p++;    //第一出线告知数量
	pNum[1] = p++;    //第二出线告知数量
	pNum[2] = p++;    //第三出线告知数量
	*pNum[0] = 0;
	*pNum[1] = 0;
	*pNum[2] = 0;
#endif												//

	for (u8 i = 0; i < 3; i++)
	{
		if (idx[i * 2 + 1] != MAX_ZC_RECOGNIZE_PHASE)
		{
			for (u16 j = 0; j < CON_FEATURE_COUNT_PER_PHASE; j++)
			{
				u32 nextIdx = (idx[i * 2 + 1] + 1) % MAX_ZC_RECOGNIZE_PHASE;
				u32 curIdx = idx[i * 2 + 1] % MAX_ZC_RECOGNIZE_PHASE;
				if (nextIdx == ZC_NTB[i * 2 + 1].Widx)
				{
					break;
				}
				s32 ntb_diff = ZC_NTB[i * 2 + 1].zc_ntb[nextIdx] - ZC_NTB[i * 2 + 1].zc_ntb[curIdx];
				ntb_diff -= NTB_FRE_PER_MS * 20;
				if(ntb_diff >= 0)
				{
					ntb_diff = (ntb_diff+4)/8;
				}
				else
				{
					ntb_diff = (ntb_diff-4)/8;
				}
				if(ntb_diff > 3000 || ntb_diff < -3000)
				{
					debug_str(DEBUG_LOG_APP, "ntb_diff Err! neg ntb_diff:%d\r\n", ntb_diff);
				}
				if (GetTickCount64()>first_tick)
				{
					memcpy_special(p, &ntb_diff, 2);
				}
				else
				{
					memcpy_special(p, &fix_diff, 2);
				}

				p += 2;
				idx[i * 2 + 1]++;
				(*pNum[i])++;
				pdata->Total++;
				len += 2;
			}
		}
	}

	//上升沿
	memcpy_special(p, &ntbP, 4);
	p += 4;
	*p++ = 0;     //保留
#if 0
	*p++ = (u8)CON_FEATURE_COUNT_PER_PHASE;     //第一出线告知数量
	*p++ = (u8)CON_FEATURE_COUNT_PER_PHASE;     //第二出线告知数量
	*p++ = (u8)CON_FEATURE_COUNT_PER_PHASE;     //第三出线告知数量
#else
	pNum[0] = p++;    //第一出线告知数量
	pNum[1] = p++;    //第二出线告知数量
	pNum[2] = p++;    //第三出线告知数量
	*pNum[0] = 0;
	*pNum[1] = 0;
	*pNum[2] = 0;
#endif
	for (u8 i = 0; i < 3; i++)
	{
		if (idx[i * 2] != MAX_ZC_RECOGNIZE_PHASE)
		{
			for (u16 j = 0; j < CON_FEATURE_COUNT_PER_PHASE; j++)
			{
				u32 nextIdx = (idx[i * 2] + 1) % MAX_ZC_RECOGNIZE_PHASE;
				u32 curIdx = idx[i * 2] % MAX_ZC_RECOGNIZE_PHASE;
				if (nextIdx == ZC_NTB[i * 2].Widx)
				{
					break;
				}
				s32 ntb_diff = ZC_NTB[i * 2].zc_ntb[nextIdx] - ZC_NTB[i * 2].zc_ntb[curIdx];
				ntb_diff -= NTB_FRE_PER_MS * 20;
				if(ntb_diff >= 0)
				{
					ntb_diff = (ntb_diff+4)/8;
				}
				else
				{
					ntb_diff = (ntb_diff-4)/8;
				}
				if(ntb_diff > 3000 || ntb_diff < -3000)
				{
					debug_str(DEBUG_LOG_APP, "ntb_diff Err! reg ntb_diff:%d\r\n", ntb_diff);
				}

				if (GetTickCount64()>first_tick)
				{
					memcpy_special(p, &ntb_diff, 2);
				}
				else
				{
					memcpy_special(p, &fix_diff, 2);
				}

				p += 2;
				idx[i * 2]++;
				(*pNum[i])++;
				pdata->Total++;
				len += 2;
			}
		}
	}

	OS_CRITICAL_EXIT();
	msdu_len = len;

	debug_str(DEBUG_LOG_NET, "PLC_SendAreaDiscernmentFeature, total:%d\r\n", pdata->Total);	
	pAppFrame = HplcApp_MakeAreaDiscernmentFrame(pcb->packet_seq++, mac_addr, 3, 3, (u8 *)pdata, &msdu_len);
	if (NULL == pAppFrame)
		goto L_Error;



	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	send_param.link = FreamPathBoth;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_AREA_DISCERNMENT;
	SOF_param.DstTei = TEI_BROADCAST;

	mac_param.DstTei = TEI_BROADCAST;
	mac_param.SendType = SENDTYPE_BROADCAST;
	mac_param.SendLimit = 4;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.mac_flag = MACFLAG_NONE;
	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, BroadCastMacAddr, MAC_ADDR_LEN);
#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);
	free_buffer(__func__,pdata);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);
	free_buffer(__func__,pdata);

	return (!OK);
}


unsigned char PLC_SendSelfRegStop()
{
#ifndef HPLC_CSG
	P_APP_PACKET pAppFrame = NULL;
	USHORT msdu_len;
	pAppFrame = HplcApp_MakeStopSelfRegFrame(g_pRelayStatus->selfreg_cb.packet_seq_self_reg);
	if (NULL == pAppFrame)
		goto L_Error;

	msdu_len = sizeof(APP_PACKET) - 1 + sizeof(REGIST_STOP_DOWN);

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 4;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	send_param.link = FreamPathBoth;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_SELF_REG;
	SOF_param.DstTei = TEI_BROADCAST;

	mac_param.DstTei = TEI_BROADCAST;
	mac_param.SendType = SENDTYPE_PCOBROADCAST;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

//
//    send_param.send_phase = PHASE_B;
//    HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8*)pAppFrame, msdu_len);
//
//    send_param.send_phase = PHASE_C;
//    HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8*)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
#else
	return OK;
#endif
}

unsigned char PLC_SendGetReadMeterCollStore(u16 tei)
{
	P_STA_STATUS_INFO pStaStatus = NULL;

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
	if (NULL == pSta)
	{
		for (tei = TEI_STA_MIN; tei < TEI_CNT; tei++)
		{
			pStaStatus = PLC_GetStaStatusInfoByTei(tei);
			pStaStatus->selfreg_read_node = 0;
		}

		return !OK;
	}
	
	if (NET_IN_NET != pSta->net_state)
		return !OK;

	P_APP_PACKET pAppFrame = NULL;
	USHORT msdu_len;
	u8 cco_mac[MAC_ADDR_LEN];
	PLC_CopyCCOMacAddr(cco_mac, FALSE);

	pAppFrame = HplcApp_MakeReadSelfRegNodeFrame(g_pRelayStatus->selfreg_cb.packet_seq_self_reg, cco_mac, pSta->mac,5);
	msdu_len = sizeof(APP_PACKET) - 1 + sizeof(REGIST_CHECK_DOWN);

	if (NULL == pAppFrame)
		goto L_Error;

	

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;
	if (SOF_param.DstTei == TEI_BROADCAST)
	{
		goto L_Error;
	}

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 0;
	SOF_param.Lid = SOF_LID_SELF_REG;

	mac_param.DstTei = tei;
	mac_param.SendType = SENDTYPE_SINGLE;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}


unsigned char PLC_SendGetReadMeterColl(u16 tei)
{
	P_STA_STATUS_INFO pStaStatus = NULL;

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
	if (NULL == pSta)
	{
		for (tei = TEI_STA_MIN; tei < TEI_CNT; tei++)
		{
			pStaStatus = PLC_GetStaStatusInfoByTei(tei);
			pStaStatus->selfreg_read_node = 0;
		}

		return !OK;
	}
	
	if (NET_IN_NET != pSta->net_state)
		return !OK;

	P_APP_PACKET pAppFrame = NULL;
	USHORT msdu_len;
	u8 cco_mac[MAC_ADDR_LEN];
	PLC_CopyCCOMacAddr(cco_mac, FALSE);
#ifdef HPLC_CSG
	u8 *pMsdu=NULL;
	pAppFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NEED_RESPONSE, EM_CMD_BUSINESS_READ_METER_LIST,  NULL, 0, &msdu_len);
#else
	pAppFrame = HplcApp_MakeReadSelfRegNodeFrame(g_pRelayStatus->selfreg_cb.packet_seq_self_reg, cco_mac, pSta->mac,0);
	msdu_len = sizeof(APP_PACKET) - 1 + sizeof(REGIST_CHECK_DOWN);
#endif
	if (NULL == pAppFrame)
		goto L_Error;

	

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;
	if (SOF_param.DstTei == TEI_BROADCAST)
	{
		goto L_Error;
	}

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 0;
	SOF_param.Lid = SOF_LID_SELF_REG;

	mac_param.DstTei = tei;
	mac_param.SendType = SENDTYPE_SINGLE;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;
#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}

unsigned char PLC_SendSelfRegReadMeter()
{
	u16 tei = 0;
	P_STA_STATUS_INFO pStaStatus = NULL;

	for (tei = TEI_STA_MIN; tei < TEI_CNT; tei++)
	{
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
		if (NULL == pSta)
			continue;

		pStaStatus = PLC_GetStaStatusInfoByTei(tei);

		if (1 == pStaStatus->selfreg_read_node)
			continue;

		if (1 == pStaStatus->selfreg_search_node)
			continue;

		if (g_pRelayStatus->innet_num>10)
		{
			if (0 == pStaStatus->check_in_net)
				continue;
		}



		pStaStatus->selfreg_read_node = 1;

		break;
	}

	return PLC_SendGetReadMeterColl(tei);
}


#define MAX_UPATE_WAIT_CNT   4
static u8 sendflag = 0;
static u8 startflag = 0;
static u8 statusflag = 0;

static u8 sendIndex = 0;
static u8 startIndex = 0;
static u8 statusIndex = 0;
//最好的方法是增加序列号来识别回复的是否是相同的序号但是需要增加的地方比较多考虑到仅此三处需要callback
//因此使用多个回调函数的方式用来识别
static void sendUpdateDone(void)
{
	sendflag = 0;
}
static void sendUpStartDone(void)
{
	startflag = 0;
}
static void sendUpStatusDone(void)
{
	statusflag = 0;
}
#define GEN_UP_CALL_BACK(func, num,arr)\
static void func##num(CSMA_SEND_STATUS resion,u8 link)\
{\
	if (arr[num])\
	{\
		func();\
		arr[num]=0;\
	}\
}
#define UP_CALL_BACK_NUM 5
static u8 sendUpdataArr[UP_CALL_BACK_NUM] ;
static u8 sendStartArr[UP_CALL_BACK_NUM] ;
static u8 sendStatusArr[UP_CALL_BACK_NUM] ;
GEN_UP_CALL_BACK(sendUpdateDone, 0, sendUpdataArr);
GEN_UP_CALL_BACK(sendUpdateDone, 1, sendUpdataArr);
GEN_UP_CALL_BACK(sendUpdateDone, 2, sendUpdataArr);
GEN_UP_CALL_BACK(sendUpdateDone, 3, sendUpdataArr);
GEN_UP_CALL_BACK(sendUpdateDone, 4, sendUpdataArr);

GEN_UP_CALL_BACK(sendUpStartDone, 0, sendStartArr);
GEN_UP_CALL_BACK(sendUpStartDone, 1, sendStartArr);
GEN_UP_CALL_BACK(sendUpStartDone, 2, sendStartArr);
GEN_UP_CALL_BACK(sendUpStartDone, 3, sendStartArr);
GEN_UP_CALL_BACK(sendUpStartDone, 4, sendStartArr);

GEN_UP_CALL_BACK(sendUpStatusDone, 0, sendStatusArr);
GEN_UP_CALL_BACK(sendUpStatusDone, 1, sendStatusArr);
GEN_UP_CALL_BACK(sendUpStatusDone, 2, sendStatusArr);
GEN_UP_CALL_BACK(sendUpStatusDone, 3, sendStatusArr);
GEN_UP_CALL_BACK(sendUpStatusDone, 4, sendStatusArr);

void (*sendUpdataCallback[UP_CALL_BACK_NUM])(CSMA_SEND_STATUS resion,u8 link)={
	sendUpdateDone0,sendUpdateDone1,sendUpdateDone2,sendUpdateDone3,sendUpdateDone4,
};
void (*sendStartCallback[UP_CALL_BACK_NUM])(CSMA_SEND_STATUS resion,u8 link)={
	sendUpStartDone0,sendUpStartDone1,sendUpStartDone2,sendUpStartDone3,sendUpStartDone4,
};
void (*sendStatusCallback[UP_CALL_BACK_NUM])(CSMA_SEND_STATUS resion,u8 link)={
	sendUpStatusDone0,sendUpStatusDone1,sendUpStatusDone2,sendUpStatusDone3,sendUpStatusDone4,
};

unsigned char PLC_SendUpdateStart(void)
{
	P_APP_PACKET pAppFrame = NULL;
#ifdef HPLC_CSG
	u8 *pMsdu;
#endif
	P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;
	USHORT msdu_len = sizeof(APP_PACKET) - 1 + sizeof(UPDATE_START_DOWN);;
	if (NULL == PLC_GetValidStaInfoByTei(pcb->tei))
		goto L_Error;
#ifdef HPLC_CSG
	pAppFrame = HplcApp_MakeStartUpdateFrame(pcb->update_id, pcb->update_minutes, pcb->fileType, pcb->block_count, pcb->file_size, pcb->file_checksum, &msdu_len, EM_BUSINESS_NEED_RESPONSE);
#else
	pAppFrame = HplcApp_MakeStartUpdateFrame(pcb->update_id, pcb->update_minutes, pcb->block_size, pcb->file_size, pcb->file_checksum);
#endif
	if (NULL == pAppFrame)
		goto L_Error;


	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif



	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_BCSMA;
	sendStartArr[startIndex]=1;
	send_param.cb = sendStartCallback[startIndex++];
	startIndex=startIndex%UP_CALL_BACK_NUM;

	startflag = MAX_UPATE_WAIT_CNT;
	PLC_GetSendParam(pcb->tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 0;
	SOF_param.Lid = SOF_LID_UPDATE;

	mac_param.DstTei = pcb->tei;
	mac_param.SendType = SENDTYPE_SINGLE;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;

	if ((pcb->plan) && (pcb->broad))
	{
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.DstTei = TEI_BROADCAST;
		SOF_param.DstTei = TEI_BROADCAST;
		send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
		send_param.link=FreamPathBoth;
	}
	else if (SOF_param.DstTei == TEI_BROADCAST) //未找到路由放弃发送
	{
		goto L_Error;
	}
#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}
u8 needSendStop = 0;
unsigned char PLC_SendUpdateStop()
{
	P_APP_PACKET pAppFrame = NULL;
	USHORT msdu_len = sizeof(APP_PACKET) - 1 + sizeof(UPDATE_STOP_DOWN);;
#ifdef HPLC_CSG
	u8 *pMsdu;
	P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;
#endif
	
	//if (NULL == PLC_GetValidStaInfoByTei(pcb->tei))
   	//	goto L_Error;

	//update_id减1处理，停止上一次升级
#ifdef HPLC_CSG
	pAppFrame = HplcApp_MakeStopUpdateFrame(pcb->update_id-1, &msdu_len);
#else
	pAppFrame = HplcApp_MakeStopUpdateFrame(0);
#endif

	if (NULL == pAppFrame)
		goto L_Error;


	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 3;
	send_param.send_type = CSMA_SEND_TYPE_BCSMA;
	send_param.cb = NULL;
//	PLC_GetSendParam(pcb->tei, &SOF_param.DstTei, &send_param.send_phase);
	SOF_param.DstTei = TEI_BROADCAST;
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	send_param.link = FreamPathBoth;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_UPDATE;

//	mac_param.DstTei = pcb->tei;
//  mac_param.SendType = SENDTYPE_SINGLE;
	mac_param.DstTei = TEI_BROADCAST;
	mac_param.SendType = SENDTYPE_PCOBROADCAST;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;
#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}

unsigned char PLC_SendUpdateData(BOOL isLocalBroadcast, u32 *pBlockid, BOOL *pbAllSent)
{
	P_APP_PACKET pAppFrame = NULL;
	*pbAllSent = FALSE;
#ifdef HPLC_CSG
	u8 *pMsdu;
#endif
	P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;

	u32 block_id = 0xffffffff;      //当前数据块号，暂时从0开始
	u8 *block_data = NULL;
	u32 block_size = pcb->block_size;
	USHORT msdu_len;
	if (NULL == PLC_GetValidStaInfoByTei(pcb->tei) && pcb->broad == 0)
		goto L_Error;



	if (NULL == pcb->bitmap)
		goto L_Error;

	for (u16 i = 0; i < pcb->block_count; i++)
	{
#ifdef HPLC_CSG
		if (pcb->bitmap[i >> 3] & (1 << (7 - (i & 0x7))))
#else
		if (pcb->bitmap[i >> 3] & (1 << (i & 0x7)))
#endif
			{
				block_id = i;
#ifdef HPLC_CSG
				pcb->bitmap[i >> 3] &= ~(1 << (7 - (i & 0x7)));
#else
				pcb->bitmap[i >> 3] &= ~(1 << (i & 0x7));
#endif
				break;
			}
	}

	if (0xffffffff == block_id)
	{
		*pbAllSent = TRUE;
		return OK;
	}

	if ((pcb->block_count - 1) == block_id)
		block_size = pcb->file_size - block_id * pcb->block_size;

	block_data = (u8 *)alloc_buffer(__func__,block_size);
	if (NULL == block_data)
		goto L_Error;

	data_flash_read_straight(pcb->flash_addr + block_id * pcb->block_size, block_data, block_size);
	msdu_len = sizeof(APP_PACKET) - 1 + sizeof(UPDATE_DATA_DOWN) - 1 + block_size;

	if (isLocalBroadcast)
	{
#ifdef HPLC_CSG
		pAppFrame = HplcApp_MakeUpdateDataBroadcastFrame(pcb->update_id, block_id, block_data, block_size, pcb->block_count, &msdu_len);
#else
		pAppFrame = HplcApp_MakeUpdateDataBroadcastFrame(pcb->update_id, block_id, block_data, block_size);
#endif
	}
	else
	{
#ifdef HPLC_CSG
		pAppFrame = HplcApp_MakeUpdateDataFrame(pcb->update_id, block_id, block_data, block_size, pcb->block_count, &msdu_len);
#else
		pAppFrame = HplcApp_MakeUpdateDataFrame(pcb->update_id, block_id, block_data, block_size);
#endif
	}

	if (NULL == pAppFrame)
		goto L_Error;

	

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_BCSMA;
	sendUpdataArr[sendIndex]=1;
	send_param.cb = sendUpdataCallback[sendIndex++];
	sendIndex=sendIndex%UP_CALL_BACK_NUM;
	PLC_GetSendParam(pcb->tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 0;
	SOF_param.Lid = SOF_LID_UPDATE;

	mac_param.DstTei = pcb->tei;
	mac_param.SendType = SENDTYPE_SINGLE;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;
	if ((pcb->plan) && (pcb->broad))
	{
		send_param.link=FreamPathBoth;
		SOF_param.BroadcastFlag = 1;
		send_param.resend_count = 1;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.DstTei = TEI_BROADCAST;
		SOF_param.DstTei = TEI_BROADCAST;
		send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	}
	else if (SOF_param.DstTei == TEI_BROADCAST) //未找到路由放弃发送
	{
		goto L_Error;
	}
#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif
	if (HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len))
	{
		sendflag = MAX_UPATE_WAIT_CNT;
	}

	free_buffer(__func__,pAppFrame);
	free_buffer(__func__,block_data);
	*pBlockid = block_id;

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);
	free_buffer(__func__,block_data);

	return (!OK);
}

unsigned char PLC_SendUpdateReqStatus(void)
{
#ifdef HPLC_CSG
	u8 *pMsdu;
#endif
	P_APP_PACKET pAppFrame = NULL;
	USHORT msdu_len = sizeof(APP_PACKET) - 1 + sizeof(UPDATE_STATUS_DOWN);;
	P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;

	if (NULL == PLC_GetValidStaInfoByTei(pcb->tei))
		goto L_Error;
#ifdef HPLC_CSG
	pAppFrame = HplcApp_MakeUpdateStatusFrame(pcb->update_id, 0, 0xffff, &msdu_len);
#else
	pAppFrame = HplcApp_MakeUpdateStatusFrame(pcb->update_id, 0, 0xffff);
#endif
	if (NULL == pAppFrame)
		goto L_Error;



	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_BCSMA;

	sendStatusArr[statusIndex]=1;
	send_param.cb = sendStatusCallback[statusIndex++];
	statusIndex=statusIndex%UP_CALL_BACK_NUM;

	statusflag = MAX_UPATE_WAIT_CNT;
	PLC_GetSendParam(pcb->tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 0;
	SOF_param.Lid = SOF_LID_UPDATE;

	mac_param.DstTei = pcb->tei;
	mac_param.SendType = SENDTYPE_SINGLE;
	mac_param.SendLimit = 2;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;

	if ((pcb->plan) && (pcb->broad))
	{
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.DstTei = TEI_BROADCAST;
		SOF_param.DstTei = TEI_BROADCAST;
		send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
		send_param.link=FreamPathBoth;
	}
	else if (SOF_param.DstTei == TEI_BROADCAST) //未找到路由放弃发送
	{
		goto L_Error;
	}
#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}

unsigned char PLC_SendUpdateExe()
{
	P_APP_PACKET pAppFrame = NULL;
#ifdef HPLC_CSG
	u8 *pMsdu;
#endif
	P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;
	USHORT msdu_len = sizeof(APP_PACKET) - 1 + sizeof(UPDATE_EXE_DOWN);;
	if (NULL == PLC_GetValidStaInfoByTei(pcb->tei))
		goto L_Error;
#ifdef HPLC_CSG
	pAppFrame = HplcApp_MakeUpdateExeFrame(pcb->update_id, 120, &msdu_len);
#else
	pAppFrame = HplcApp_MakeUpdateExeFrame(pcb->update_id, 120, 0);
#endif
	if (NULL == pAppFrame)
		goto L_Error;


	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 3;
	send_param.send_type = CSMA_SEND_TYPE_BCSMA;
	send_param.cb = NULL;
//	PLC_GetSendParam(pcb->tei, &SOF_param.DstTei, &send_param.send_phase);
	SOF_param.DstTei = TEI_BROADCAST;
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	send_param.link = FreamPathBoth;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_UPDATE;

//	mac_param.DstTei = pcb->tei;
//  mac_param.SendType = SENDTYPE_SINGLE;
	mac_param.DstTei = TEI_BROADCAST;
	mac_param.SendType = SENDTYPE_PCOBROADCAST;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;
#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}
#ifndef HPLC_CSG
unsigned char PLC_SendUpdateReqStaInfo(void)
{
	P_APP_PACKET pAppFrame = NULL;

	P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;
	USHORT msdu_len;
	EM_UPDATE_STA_INFO elements[] = { EM_STA_FACTORY_CODE };
	if (NULL == PLC_GetValidStaInfoByTei(pcb->tei))
		goto L_Error;

	//EM_UPDATE_STA_INFO elements[] = {EM_STA_FACTORY_CODE, EM_STA_CRC, EM_STA_FILE_SIZE};
	//EM_UPDATE_STA_INFO elements[] = {EM_STA_CRC, EM_STA_FILE_SIZE};
	//EM_UPDATE_STA_INFO elements[] = {EM_STA_FACTORY_CODE, EM_STA_VERSION};


	pAppFrame = HplcApp_MakeUpdateStaInfoFrame(elements, countof(elements));
	if (NULL == pAppFrame)
		goto L_Error;

	msdu_len = sizeof(APP_PACKET) - 1 + sizeof(UPDATE_STA_INFO_DOWN) - 1 + countof(elements);

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	PLC_GetSendParam(pcb->tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 0;
	SOF_param.Lid = SOF_LID_UPDATE;

	mac_param.DstTei = pcb->tei;
	mac_param.SendType = SENDTYPE_SINGLE;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;
	if ((pcb->plan) && (pcb->broad))
	{
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.DstTei = TEI_BROADCAST;
		SOF_param.DstTei = TEI_BROADCAST;
		send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
		send_param.link=FreamPathBoth;
	}
	else if (SOF_param.DstTei == TEI_BROADCAST) //未找到路由放弃发送
	{
		goto L_Error;
	}

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);


	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}
#endif

unsigned char PLC_SendBroadcast(P_MSG_INFO pSendMsg, u16 seq, u8 timing_seq)
{
#if defined(HPLC_CSG)

	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu = NULL;
	u16 app_len, msdu_len;

	u8 src_addr[MAC_ADDR_LEN];
	u8 des_addr[MAC_ADDR_LEN];

	PLC_CopyCCOMacAddr(src_addr, FALSE);
	memset_special(des_addr, 0x99, sizeof(des_addr));

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 4;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	send_param.link = FreamPathBoth;
	//send_param.send_phase = PHASE_A;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_TIMING;
	SOF_param.DstTei = TEI_BROADCAST;

	mac_param.DstTei = TEI_BROADCAST;
	mac_param.SendType = SENDTYPE_PCOBROADCAST;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DOWN;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;

	P_GD_DL645_BODY pDL645 = PLC_check_dl645_frame(pSendMsg->msg_buffer, pSendMsg->msg_header.msg_len);
	if (pSendMsg->msg_header.protocol != METER_PROTOCOL_3762_BROAD_TIME&&pSendMsg->msg_header.protocol != METER_PROTOCOL_3762_NEW_BROAD_TIME)

	{
		pAppFrame = HplcApp_MakeDataTransFrame(src_addr, des_addr, 0, pSendMsg->msg_header.meter_moudle, seq, EM_BUSINESS_NORESPONSE, pSendMsg->msg_buffer, pSendMsg->msg_header.msg_len, &app_len);
	}
	else if (pSendMsg->msg_header.protocol == METER_PROTOCOL_3762_NEW_BROAD_TIME)
	{
		if (pDL645==NULL||pDL645->controlCode!=0x08)
		{
			pAppFrame = HplcApp_MakeDataTransFrame(src_addr, des_addr, 0, pSendMsg->msg_header.meter_moudle, seq, EM_BUSINESS_NORESPONSE, pSendMsg->msg_buffer, pSendMsg->msg_header.msg_len, &app_len);
		}
		else
		{
			APP_BROADCST_TIME_EXT* data = (APP_BROADCST_TIME_EXT*)alloc_buffer(__func__,sizeof(APP_BROADCST_TIME_EXT)+pSendMsg->msg_header.msg_len);
			if (data==NULL)
			{
				return (!OK);
			}
			data->Port = 0x1;
			data->Seq = timing_seq;
			data->Reserve=0;
			data->NTB=pSendMsg->msg_header.time_stamp;
			memcpy(data->data,pSendMsg->msg_buffer, pSendMsg->msg_header.msg_len);
			pAppFrame = HplcApp_MakeDataTransFrame(src_addr, des_addr, APP_SHENHUA_APP_CODE_TIMING, 2, seq, EM_BUSINESS_NORESPONSE, (u8*)data, sizeof(APP_BROADCST_TIME_EXT)+pSendMsg->msg_header.msg_len, &app_len);
			free_buffer(__func__,data);
		}
	}
	else
	{
		APP_DATA_BROAD_TIME broad_time;
		local_tm broad_tm;
		__time64_t real_time = pSendMsg->msg_header.time_stamp+GetTickCount64();
		real_time/=OS_CFG_TICK_RATE_HZ;
		broad_tm=*gmtime(&real_time);
		broad_time.bcd_year = Hex2BcdChar((broad_tm.tm_year + 1900) % 100);
		broad_time.bcd_mon = Hex2BcdChar(broad_tm.tm_mon+1);
		broad_time.bcd_day = Hex2BcdChar(broad_tm.tm_mday);
		broad_time.bcd_hour = Hex2BcdChar(broad_tm.tm_hour);
		broad_time.bcd_min = Hex2BcdChar(broad_tm.tm_min);
		broad_time.bcd_sec = Hex2BcdChar(broad_tm.tm_sec);		
		broad_time.ntb=BPLC_GetNTB();
#if 1
		pAppFrame = HplcApp_MakeDataTransFrame(src_addr, des_addr, APP_SHENHUA_APP_CODE_TIMING, pSendMsg->msg_header.meter_moudle, seq, EM_BUSINESS_NORESPONSE, (u8*)&broad_time, sizeof(broad_time), &app_len);
#else
		pAppFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NORESPONSE, EM_CMD_BUSINESS_BROAD_TIME, (u8*)&broad_time, sizeof(broad_time), &app_len);
#endif
	}

	if (NULL == pAppFrame)
		goto L_Error;

	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, app_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	if (NULL == pMsdu)
		goto L_Error;

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);

	free_buffer(__func__,pMsdu);

	return OK;

L_Error:
	free_buffer(__func__,pMsdu);
	return (!OK);

#else

	P_APP_PACKET pAppFrame = NULL;
	USHORT msdu_len;
	P_TIMING_DOWN pTimingFrame;
#ifdef GDW_NEIMENG
	P_GD_DL645_BODY pDL645 = PLC_check_dl645_frame(pSendMsg->msg_buffer, pSendMsg->msg_header.msg_len);
    P_DL69845_BODY pDL698 = PLC_check_69845_frame(pSendMsg->msg_buffer, pSendMsg->msg_header.msg_len);
	if (pDL645)
	{
		if (pDL645->controlCode == 0x08 && pDL645->dataLen==0x6) //广播校时
		{
			u8 len=pDL645->dataLen+12;
			P_GD_DL645_BODY new_645= (P_GD_DL645_BODY)alloc_buffer(__func__,len);
			if (new_645==NULL)
			{
				goto L_Error;
			}
			local_tm broad_tm;
			memset(&broad_tm,0,sizeof(broad_tm));
			memcpy(new_645,pDL645,len);
			broad_tm.tm_sec = Bcd2HexChar(pDL645->dataBuf[0]-0x33);
			broad_tm.tm_min = Bcd2HexChar(pDL645->dataBuf[1]-0x33);
			broad_tm.tm_hour = Bcd2HexChar(pDL645->dataBuf[2]-0x33);
			broad_tm.tm_mday = Bcd2HexChar(pDL645->dataBuf[3]-0x33);
			broad_tm.tm_mon = Bcd2HexChar(pDL645->dataBuf[4]-0x33)-1;
			broad_tm.tm_year = Bcd2HexChar(pDL645->dataBuf[5]-0x33)+ 2000 - 1900;
			__time64_t real_time = mktime(&broad_tm);
			real_time+=(GetTickCount64()-pSendMsg->msg_header.time_stamp)/OS_CFG_TICK_RATE_HZ;
			broad_tm = *gmtime(&real_time);

			new_645->dataBuf[0]= Hex2BcdChar(broad_tm.tm_sec)+0x33;
			new_645->dataBuf[1]= Hex2BcdChar(broad_tm.tm_min)+0x33;
			new_645->dataBuf[2]= Hex2BcdChar(broad_tm.tm_hour)+0x33;
			new_645->dataBuf[3] = Hex2BcdChar(broad_tm.tm_mday)+0x33;
			new_645->dataBuf[4]= Hex2BcdChar(broad_tm.tm_mon+1)+0x33;
			new_645->dataBuf[5]= Hex2BcdChar((broad_tm.tm_year+1900)%100)+0x33;

			new_645->dataBuf[new_645->dataLen] = Get_checksum(&new_645->protocolHead, new_645->dataLen+10);

			pAppFrame = HplcApp_MakeBroadcastTimingFrame((u8*)new_645, len);
			free_buffer(__func__,new_645);
		}
		else
		{
			pAppFrame = HplcApp_MakeBroadcastTimingFrame(pSendMsg->msg_buffer, pSendMsg->msg_header.msg_len);
		}
	}
	else if (pDL698)
	{
		if (pDL698->AF.SA_type == 3 && pDL698->AF.SA_len == 0 && pDL698->other.other_s.SA[0]==0xaa //广播
			&& *(u32*)&pDL698->other.other_c[7]==0x007f0040 && pDL698->other.other_c[11]==28)//广播校时
		{
			u16 len=pDL698->len+2;
			P_DL69845_BODY new_698 = (P_DL69845_BODY)alloc_buffer(__func__, len);
			if (new_698==NULL)
			{
				goto L_Error;
			}
			local_tm broad_tm;
			memset(&broad_tm,0,sizeof(broad_tm));
			memcpy(new_698,pDL698,len);

			broad_tm.tm_sec = pDL698->other.other_c[18];
			broad_tm.tm_min =  pDL698->other.other_c[17];
			broad_tm.tm_hour = pDL698->other.other_c[16];
			broad_tm.tm_mday = pDL698->other.other_c[15];
			broad_tm.tm_mon = pDL698->other.other_c[14]-1;
			broad_tm.tm_year = ((pDL698->other.other_c[12]<<8)|pDL698->other.other_c[13]) - 1900;
			__time64_t real_time = mktime(&broad_tm);
			real_time+=(GetTickCount64()-pSendMsg->msg_header.time_stamp)/OS_CFG_TICK_RATE_HZ;
			broad_tm = *gmtime(&real_time);

			new_698->other.other_c[18]= broad_tm.tm_sec;
			new_698->other.other_c[17]= broad_tm.tm_min;
			new_698->other.other_c[16]= broad_tm.tm_hour;
			new_698->other.other_c[15]= broad_tm.tm_mday;
			new_698->other.other_c[14]= broad_tm.tm_mon+1;
			broad_tm.tm_year+=1900;
			new_698->other.other_c[12]= broad_tm.tm_year>>8;
			new_698->other.other_c[13]= broad_tm.tm_year&0xff;
			u16 fcs=GetCrc16(&((u8*)new_698)[1], new_698->len-2);
			new_698->other.other_c[len-8]=fcs&0xff;
			new_698->other.other_c[len-7]=fcs>>8;
			pAppFrame = HplcApp_MakeBroadcastTimingFrame((u8*)new_698, len);
			free_buffer(__func__,new_698);
		}
		else
		{
			pAppFrame = HplcApp_MakeBroadcastTimingFrame(pSendMsg->msg_buffer, pSendMsg->msg_header.msg_len);
		}
	}
	else
#endif
	{
		pAppFrame = HplcApp_MakeBroadcastTimingFrame(pSendMsg->msg_buffer, pSendMsg->msg_header.msg_len);
	}
	if (NULL == pAppFrame)
		goto L_Error;

	pTimingFrame = (P_TIMING_DOWN)pAppFrame->Data;

	msdu_len = sizeof(APP_PACKET) - 1 + sizeof(TIMING_DOWN) - 1 + pTimingFrame->DataLen;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 4;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	send_param.link = FreamPathBoth;
	//send_param.send_phase = PHASE_A;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_TIMING;
	SOF_param.DstTei = TEI_BROADCAST;

	mac_param.DstTei = TEI_BROADCAST;
	mac_param.SendType = SENDTYPE_PCOBROADCAST;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);
	return (!OK);

#endif
}
#ifdef ERROR_APHASE
unsigned char PLC_SendPhaseErr(u16 seq)
{


	P_APP_PACKET pAppFrame = NULL;
	USHORT msdu_len;
	P_PHASE_ERR_SWITCH pPhaseFrame;
	pAppFrame = HplcApp_MakePhaseErrFrame(2, g_pPlmConfigPara->a_phase_error, g_pPlmConfigPara->main_node_addr,seq);
	if (NULL == pAppFrame)
		goto L_Error;

	pPhaseFrame = (P_PHASE_ERR_SWITCH)pAppFrame->Data;

	msdu_len = sizeof(APP_PACKET) - 1 + sizeof(PHASE_ERR_SWITCH)  + pPhaseFrame->DataLen;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 4;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	send_param.link = FreamPathBoth;
	//send_param.send_phase = PHASE_A;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_TIMING;
	SOF_param.DstTei = TEI_BROADCAST;

	mac_param.DstTei = TEI_BROADCAST;
	mac_param.SendType = SENDTYPE_PCOBROADCAST;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);
	return (!OK);

}
#endif

unsigned char PLC_SendAccurateTiming(P_MSG_INFO pSendMsg, u16 seq, u8 timing_seq)
{
	P_APP_PACKET pAppFrame = NULL;
	USHORT msdu_len;
	P_ACCURATE_TIMING_DOWN pTimingFrame;

	pAppFrame = HplcApp_MakeAccurateTimingFrame(pSendMsg->msg_buffer, pSendMsg->msg_header.msg_len,timing_seq,pSendMsg->msg_header.time_stamp);

	if (NULL == pAppFrame)
		goto L_Error;

	pTimingFrame = (P_ACCURATE_TIMING_DOWN)pAppFrame->Data;

	msdu_len = sizeof(APP_PACKET) - 1 + sizeof(ACCURATE_TIMING_DOWN) - 1 + pTimingFrame->DataLen;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 4;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	send_param.link = FreamPathBoth;
	//send_param.send_phase = PHASE_A;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_TIMING;
	SOF_param.DstTei = TEI_BROADCAST;

	mac_param.DstTei = TEI_BROADCAST;
	mac_param.SendType = SENDTYPE_PCOBROADCAST;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);
	return (!OK);
}

unsigned char PLC_SendCommTest(P_COMM_TEST_INFO pTestMsg)
{
	P_APP_PACKET pAppFrame = NULL;

	USHORT tei = PLC_GetStaSnByMac(pTestMsg->mac);
	USHORT msdu_len;
	P_STA_INFO pSta;
	if (tei >= TEI_CNT)
		goto L_Error;

	pSta = PLC_GetValidStaInfoByTei(tei);
	if (NULL == pSta)
		goto L_Error;

	if (NET_IN_NET != pSta->net_state)
		tei = TEI_BROADCAST;

	pAppFrame = HplcApp_MakeCommTestFrame(pTestMsg->data, pTestMsg->len, pTestMsg->meter_protocol_plc);
	if (NULL == pAppFrame)
		goto L_Error;

	msdu_len = sizeof(APP_PACKET) - 1 + sizeof(COMM_TEST_FRAME_PACKET)-1 + pTestMsg->len;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 4;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_READ_METER;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	send_param.doubleSend=1;
	if (TEI_BROADCAST == tei)
	{
		send_param.link=FreamPathBoth;
		send_param.resend_count = 2;
		send_param.send_type = CSMA_SEND_TYPE_CSMA;
		send_param.cb = NULL;
		SOF_param.DstTei = tei;
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		//mac_param.SendType = SENDTYPE_BROADCAST;
		mac_param.mac_flag = MACFLAG_NONE;
		mac_param.Broadcast = BROADCAST_DRT_DOWN;

		PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
		memcpy_special(mac_param.DstMac, pTestMsg->mac, MAC_ADDR_LEN);

		send_param.send_phase = SEND_PHASE_ALL;
		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);
	}
	else
	{
		PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
		
		SOF_param.BroadcastFlag = 0;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.mac_flag = MACFLAG_NONE;
		mac_param.Broadcast = BROADCAST_DRT_DUL;

		//memcpy_special(mac_param.SrcMac, g_pPlmConfigPara->main_node_addr, MAC_ADDR_LEN);
		//memcpy_special(mac_param.DstMac, pReadMsg->header.mac, MAC_ADDR_LEN);
		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);
	}

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}

unsigned char PLC_SendInnerVerRead(u8 node_mac[6])
{
	P_APP_PACKET pAppFrame = NULL;
#ifdef HPLC_CSG
	u8 *pMsdu;
#endif
	USHORT tei = PLC_GetStaSnByMac(node_mac);
	USHORT msdu_len;
	P_STA_INFO pSta;
	if (tei >= TEI_CNT)
		goto L_Error;

	pSta = PLC_GetValidStaInfoByTei(tei);
	if (NULL == pSta)
		goto L_Error;

	if (NET_IN_NET != pSta->net_state)
		goto L_Error;

	pAppFrame = HplcApp_MakeGetInnerVerFrame(node_mac, &msdu_len);
	if (NULL == pAppFrame)
		goto L_Error;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 4;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_READ_METER;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;
	SOF_param.BroadcastFlag = 0;
	mac_param.SendType = SENDTYPE_SINGLE;
	mac_param.mac_flag = MACFLAG_NONE;
	mac_param.Broadcast = BROADCAST_DRT_DUL;

#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;	 //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}

#ifdef HPLC_CSG    
unsigned char PLC_SendStaReset(P_RESET_STA_INFO pResetSta)
{
    debug_str(DEBUG_LOG_NET, "PLC_SendStaReset, %02x%02x%02x%02x%02x%02x, seconds:%d\r\n", 
        pResetSta->mac[5], pResetSta->mac[4], pResetSta->mac[3], pResetSta->mac[2], pResetSta->mac[1], pResetSta->mac[0], 
		pResetSta->seconds);
	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu;
	USHORT tei = PLC_GetStaSnByMac(pResetSta->mac);
	USHORT msdu_len;
	P_STA_INFO pSta;
	if (tei >= TEI_CNT)
		goto L_Error;

	pSta = PLC_GetValidStaInfoByTei(tei);
	if (NULL == pSta)
		goto L_Error;

	if (NET_IN_NET != pSta->net_state)
		goto L_Error;

	pAppFrame = HplcApp_MakeResetStaFrame(pResetSta->seconds, &msdu_len);
	if (NULL == pAppFrame)
		goto L_Error;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_READ_METER;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase, &send_param.link);
	SOF_param.BroadcastFlag = 0;
	mac_param.SendType = SENDTYPE_SINGLE;
	mac_param.mac_flag = MACFLAG_NONE;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.HeadType = MAC_HEAD_SHORT;	 //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}
#endif

unsigned char PLC_SendStaPower(u8 node_mac[6],u16 func,u8 *data)
{
	P_APP_PACKET pAppFrame = NULL;
#ifdef HPLC_CSG
	u8 *pMsdu;
#endif
	USHORT tei = PLC_GetStaSnByMac(node_mac);
	USHORT msdu_len;
	P_STA_INFO pSta;
	if (!memIsHex(node_mac, 0xff, 6))
	{
		if (tei >= TEI_CNT)
			goto L_Error;

		pSta = PLC_GetValidStaInfoByTei(tei);
		if (NULL == pSta)
			goto L_Error;

		if (NET_IN_NET != pSta->net_state)
			goto L_Error;
	}
	else
	{
		tei = TEI_BROADCAST;
	}
	if (data)
	{
		switch(func)
		{
			case 0x31:  //设置从节点hrf功率 国网
				msdu_len = 2;
				break;
			case 0x3F:  //设置从节点plc功率
				msdu_len = 4;
				break;
			case 0x48:  //设置从节点hrf功率 南网
				msdu_len = 2;
				break;
			case 0x61:  //设置从节点plc功率 新接口
				msdu_len = 5;
				break;
			case 0x63:  //设置从节点hrf功率 新接口
				msdu_len = 3;
				break;
			default:
				msdu_len = 0;
				break;
		}
	}
	else
	{
		msdu_len=0;
	}
	pAppFrame = HplcApp_MakeStaPowerFrame(node_mac, func, data, &msdu_len);
	if (NULL == pAppFrame)
		goto L_Error;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 4;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_READ_METER;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;
	if (tei!=TEI_BROADCAST)
	{
		SOF_param.BroadcastFlag = 0;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.mac_flag = MACFLAG_NONE;
		mac_param.Broadcast = BROADCAST_DRT_DUL;
		//send_param.link=FreamPathBoth;
	}
	else
	{
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.mac_flag = MACFLAG_NONE;
		mac_param.Broadcast = BROADCAST_DRT_DUL;
	}
#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;	 //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}

unsigned char PLC_SendRegisterRead(u8 node_mac[6], u8 *data, u16 len)
{
	P_APP_PACKET pAppFrame = NULL;
#ifdef HPLC_CSG
	u8 *pMsdu;
#endif
	USHORT tei = PLC_GetStaSnByMac(node_mac);
	USHORT msdu_len;
	P_STA_INFO pSta;
	if (tei >= TEI_CNT)
		goto L_Error;

	pSta = PLC_GetValidStaInfoByTei(tei);
	if (NULL == pSta)
		goto L_Error;

	if (NET_IN_NET != pSta->net_state)
		goto L_Error;

	msdu_len = len;
	pAppFrame = HplcApp_MakeGetRegisterFrame(node_mac, data, &msdu_len);
	if (NULL == pAppFrame)
		goto L_Error;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 4;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_READ_METER;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;
	SOF_param.BroadcastFlag = 0;
	mac_param.SendType = SENDTYPE_SINGLE;
	mac_param.mac_flag = MACFLAG_NONE;
	mac_param.Broadcast = BROADCAST_DRT_DUL;

#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;	 //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}


unsigned char PLC_SendStaAuthen(int en)
{
	P_APP_PACKET pAppFrame = NULL;
#ifdef HPLC_CSG
	u8 *pMsdu;
#endif
	USHORT tei;
	USHORT msdu_len;

	tei = TEI_BROADCAST;

	pAppFrame = HplcApp_MakeStaAuthenFrame(en, &msdu_len);
	if (NULL == pAppFrame)
		goto L_Error;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 4;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_READ_METER;

	mac_param.DstTei = tei;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;
	if (tei!=TEI_BROADCAST)
	{
		SOF_param.BroadcastFlag = 0;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.mac_flag = MACFLAG_NONE;
		mac_param.Broadcast = BROADCAST_DRT_DUL;
		send_param.link=FreamPathBoth;
	}
	else
	{
		SOF_param.BroadcastFlag = 1;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.mac_flag = MACFLAG_NONE;
		mac_param.Broadcast = BROADCAST_DRT_DUL;
	}
#ifdef HPLC_CSG
	mac_param.HeadType = MAC_HEAD_SHORT;	 //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
#endif

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}


u8 PLC_SendStaAuthenProc(OS_TICK_64 tick_now)
{
	PLC_SET_STA_AUTHEN_CB* pcb = &g_pRelayStatus->set_sta_authen_cb;
	switch (pcb->status)
	{
	case 0:
		if (pcb->time)
		{
			PLC_SendStaAuthen(pcb->isEnable);
			pcb->tick_next=tick_now+5*1000;
			pcb->status=1;
			pcb->time--;
		}
		else
		{
			pcb->tick_next=tick_now+24*60*60*1000;
			pcb->status=2;
		}
		break;
	case 1:
		if (tick_now>pcb->tick_next)
		{
			pcb->status=0;
		}
		break;
	case 2:
		{
			if (tick_now > pcb->tick_next)
			{
				pcb->status = 0;
				pcb->time = 3;
				break;
			}
			if (pcb->time)
			{
				pcb->status = 0;
			}
		}
	}
	return OK;
}

//是否组网完成
unsigned char PLC_IsNetComplited()
{
#if defined(NET_FLAG_FIX)
	return net_flag_fix; //给客户开放 可以自己调整的
#elif defined(NET_FLAG_ALWAYS_TRUE)
	if(g_pRelayStatus->online_num >= 1)
	{
		return OK;
	}
	else
	{
		return !OK;
	}
#elif defined(HPLC_CSG)&& defined(HRF_SUPPORT)
	if (g_pRelayStatus->online_num >= 1)//有一个入网
	{
		return OK;
	}
	return (!OK);
#elif defined(HPLC_CSG)
	return (!OK); //现场适配林洋采集器（深国电方案17年），Ture影响和它载波交互
#else
#if 1
	if (g_pRelayStatus->online_num >= 1)//有一个入网
	{
		return OK;
	}
	return (!OK);
	#else
	if(g_pRelayStatus->innet_num >= g_pRelayStatus->cfged_meter_num)
	return OK;
	return (!OK);
#endif
#endif
}



u16 PLC_GetCCOFoundListIntervalSecond(u16 route_period_sec)
{
	u16 interval = 5;
//    return interval;
//#ifdef GDW_TEST
	interval = MIN(route_period_sec / 7 - 1, 30);        //代理站点发送发现列表报文的间隔
	interval = MAX(13, interval);
	return interval;
//#else
//    interval = MIN(MIN_ROUTE_PERIOD_SEC, 20);        //代理站点发送发现列表报文的间隔
//    interval = MAX(10, interval);
//    return interval;
//#endif
}

u16 PLC_GetPcoFoundListIntervalSecond(u16 route_period_sec)
{
	u16 interval = 5;
//    return interval;
//#ifdef GDW_TEST
	interval = MIN(route_period_sec / 6 - 1, 40);        //代理站点发送发现列表报文的间隔
	interval = MAX(15, interval);
	return interval;
//#else
//    interval = MIN(MIN_ROUTE_PERIOD_SEC, 20);        //代理站点发送发现列表报文的间隔
//    interval = MAX(3, interval);
//    return interval;
//#endif
}

u16 PLC_GetStaFoundListIntervalSecond(u16 route_period_sec)
{
	u16 interval = 6;
//    return interval;
//#ifdef GDW_TEST
	interval = MIN(route_period_sec / 5 - 1, 50);        //发现站点发送发现列表报文的间隔
	interval = MAX(17, interval);
	return interval;
//#else
//    interval = MIN(MIN_ROUTE_PERIOD_SEC, 40);        //发现站点发送发现列表报文的间隔
//    interval = MAX(3, interval);
//    return interval;
//#endif
}

u8 PLC_GetFoundListIntervalSecond(u16 route_period_sec, u8 *pCcoIntervalSec,  u8 *pPcoIntervalSec, u8 *pStaIntervalSec)
{
#if 0
	*pCcoIntervalSec = 12;
	*pPcoIntervalSec = 10;
	*pStaIntervalSec = 10;
#elif 0
	u8 cco_interval, pco_interval, sta_interval;

	cco_interval = MIN(route_period_sec/12-1, 70);

	pco_interval = MIN(route_period_sec/10-1, 60);
	//pco_interval = MAX(8, pco_interval);

	sta_interval = pco_interval; // + 5;

	if(NULL != pCcoIntervalSec)
	*pCcoIntervalSec = cco_interval;

	if(NULL != pPcoIntervalSec)
	*pPcoIntervalSec = pco_interval;

	if(NULL != pStaIntervalSec)
	*pStaIntervalSec = sta_interval;

	return OK;
#else
	u8 cco_interval, pco_interval, sta_interval;

	//pco_interval = (u8)MIN(route_period_sec/6, 40);

	pco_interval = (u8)MIN(route_period_sec / 10, 40);
	pco_interval = (u8)MAX(5, pco_interval);

	cco_interval = pco_interval;
	sta_interval = pco_interval;

	if (NULL != pCcoIntervalSec)
		*pCcoIntervalSec = cco_interval;

	if (NULL != pPcoIntervalSec)
		*pPcoIntervalSec = pco_interval;

	if (NULL != pStaIntervalSec)
		*pStaIntervalSec = sta_interval;

	return OK;
#endif
}


u8 HRF_GetFoundListPeriodSecond(u16 route_period_sec)
{
	u8 period;

	//每个路由周期暂定发4个
	period=route_period_sec/HRF_HRF_PERIOD_SEND_CNT;
	//最小为10
	if (period<10)
	{
		return 10;
	}
	return period;
}
//获得中继路径，不包括  TEI_CCO 和自身
//返回值：中继路径的个数
u8 PLC_GetRelayPcos(u16 tei, u16 pcos[MAX_RELAY_DEEP])
{
	u8 p = 0;

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);

	if (NULL == pSta)
		return 0;

	u16 tmp_pcos[MAX_RELAY_DEEP];

	//tmp_pco[p++] = tei;

	while ((NULL != pSta) && (pSta->pco >= TEI_STA_MIN) && (pSta->pco <= TEI_STA_MAX))
	{
		tmp_pcos[p++] = pSta->pco;
		pSta = PLC_GetValidStaInfoByTei(pSta->pco);
	}

	if (p > 0)
	{
		u8 j = p - 1;
		for (u8 i = 0; i < p; i++) pcos[i] = tmp_pcos[j--];
	}

	return p;
}




BOOL GetNotCenterInfo(const P_RELAY_PATH p_relay_from, P_GET_BEACON p_not_center_info)
{

	for (u8 i = 0; i < p_relay_from->num; i++)
	{
		int tei = p_relay_from->sta[i];
		SET_STA_BITMAP(p_not_center_info->sta, tei);
	}

	return TRUE;
}


#if 1


EN_RF_BEACON_TYPE getStaRfBeaconType(P_STA_INFO psta)
{
	if (!psta->child)
	{
		return LINK_HPLC_ONLY;
	}
	psta = PLC_GetValidStaInfoByTei(psta->child);
	while (psta)
	{
		if (psta->Link)
		{
			return LINK_NORMAL;
		}
		psta = PLC_GetValidStaInfoByTei(psta->brother);
	}
	return LINK_HPLC_ONLY;
}




#else

BOOL PLC_GetNotCenterBeaconSta(const P_WAIT_FOR_SEND_INFO p_wait_for_send, P_NOT_CENTER_BEACON_INFO p_not_center_info)
{
	//不论层次，全部发信标
	//送检用，协议一致性能过
	p_not_center_info->num = 0;

	u8 relay_count = 0;
	u8 max_relay_num = MAX_NOT_CENTER_BEACON_STA;       //MAX(p_wait_for_send->num, 60);
	P_RELAY_PATH relay_array = (P_RELAY_PATH)alloc_buffer(__func__,sizeof(RELAY_PATH)*max_relay_num);   //[50] = {NULL};

	if(NULL == relay_array)
	goto PLC_GetNotCenterBeaconSta_Exit;

	for(u8 i=0; i<max_relay_num; i++)
	relay_array[i].num = 0;

	//发送队列TEI非中央信标
#ifndef GDW_TEST
	if(p_wait_for_send->num)
	{
		for(u16 i=0; i<p_wait_for_send->num; i++)
		{
			P_RELAY_PATH p_relay_path = &relay_array[i];
			p_relay_path->num = PLC_GetRelayPcos(p_wait_for_send->sta[i], p_relay_path->sta);
			p_relay_path->sta[p_relay_path->num++] = p_wait_for_send->sta[i];
		}
		relay_count = p_wait_for_send->num;

		goto L_FillRelay;
	}
#endif

	if(g_pRelayStatus->innet_num < max_relay_num)
	{
		for(u16 tei=TEI_STA_MIN; tei<TEI_CNT; tei++)
		{
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
			if(NULL == pSta)
			continue;

			P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
			pStaStatus->beacon_slot = 0;
		}
	}

	//组网TEI非中央信标
	//待优化:  选取1级PCO或STA，找到尚未发送信标的所有子节点，打包一起发，避免同一个PCO多次发送信标，浪费时间
	for(u8 c=0; c<2; c++)
	{
		relay_count = 0;
		p_not_center_info->num = 0;

		for(u16 tei=TEI_STA_MIN; tei<TEI_CNT; tei++)
		{
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
			if(NULL == pSta)
			continue;

			if(ROLES_STA != pSta->roles)   //待斟酌，代理变更等操作后，有可能出现PCO下没有STA的情形
			continue;

			P_STA_STATUS_INFO pStaStatus = STA_STATUS_INFO_BY_TEI(tei);
			if(pStaStatus->beacon_slot)
			continue;

			P_RELAY_PATH p_relay_path = &relay_array[relay_count];

			p_relay_path->num = PLC_GetRelayPcos(tei, p_relay_path->sta);
			p_relay_path->sta[p_relay_path->num++] = tei;

			if(!PLC_PutToNotCenterInfo(p_relay_path, p_not_center_info))
			break;

			pStaStatus->beacon_slot = 1;
			relay_count++;

			if(relay_count >= max_relay_num)
			break;
		}

		if(relay_count > 0)
		break;

		//全部发完，清掉重发
		for(u16 tei=TEI_STA_MIN; tei<TEI_CNT; tei++)
		{
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
			if(NULL == pSta)
			continue;

			P_STA_STATUS_INFO pStaStatus = STA_STATUS_INFO_BY_TEI(tei);

			if(0==pStaStatus->beacon_slot)
			continue;

			pStaStatus->beacon_slot = 0;
		}
	}

	L_FillRelay:

	p_not_center_info->num = 0;

	if(relay_count > 0)
	{
		//先加入低层级，再加入高层级
		for(u8 i=0; i<MAX_RELAY_DEEP; i++)
		{
			for(u8 j=0; j<relay_count; j++)
			{
				P_RELAY_PATH p_relay_path = &relay_array[j];

				if(i >= p_relay_path->num)
				continue;

				PLC_PutOneToNotCenterInfo(p_relay_path->sta[i], p_not_center_info, TRUE);
			}
		}
	}

	PLC_GetNotCenterBeaconSta_Exit:
	free_buffer(__func__,relay_array);

	return (p_not_center_info->num > 0);
}

#endif

BOOL PLC_OnGetBeaconParam(P_BEACON_PARAM pBeaconParam)
{
	PLC_CopyCCOMacAddr(pBeaconParam->cco_mac, FALSE);
	pBeaconParam->net_seq = g_pPlmConfigPara->net_seq;
//    pBeaconParam->cfged_meter_num = g_pRelayStatus->cfged_meter_num;

	pBeaconParam->is_net_complited = (OK == PLC_IsNetComplited());


	OS_ERR err;
	OSSchedLock(&err);

	if (g_pRelayStatus->route_period_sec_down == 0)
	{
		g_pRelayStatus->route_period_sec = PLC_GetRoutePeriodSecond(g_pRelayStatus->cfged_meter_num);
		g_pRelayStatus->route_period_sec_down = g_pRelayStatus->route_period_sec;
		g_pRelayStatus->route_period_count++;
		PLC_GetFoundListIntervalSecond(g_pRelayStatus->route_period_sec, &g_pRelayStatus->cco_foundlist_interval_sec, NULL, NULL);
		g_pRelayStatus->cco_hrf_foundlist_interval_sec=HRF_GetFoundListPeriodSecond(g_pRelayStatus->route_period_sec);
		g_pRelayStatus->found_list_count_last_route = g_pRelayStatus->found_list_counter_this_route;
		g_pRelayStatus->found_list_counter_this_route = 0;

		for (USHORT tei = 0; tei < TEI_CNT; tei++)
		{
			P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
			pStaStatus->found_list_count_last_route = pStaStatus->found_list_counter_this_route;
			pStaStatus->found_list_counter_this_route = 0;
			if (pStaStatus->firstJoinIn)
			{
				--pStaStatus->firstJoinIn;
			}
		}

		g_pRelayStatus->heartbeat_period_sec = 2 * g_pRelayStatus->route_period_sec;

		MpduSetNID(true, g_pPlmConfigPara->net_id);
	}

	g_pRelayStatus->found_list_counter_this_route++;        //使用信标帧进行信道评估

	pBeaconParam->route_period_sec = g_pRelayStatus->route_period_sec;
	if ((g_pRelayStatus->route_period_count >= 2) && ((g_pRelayStatus->route_period_sec - g_pRelayStatus->route_period_sec_down) <= 10))
		pBeaconParam->route_estimate_left_sec = 0;
	else
		pBeaconParam->route_estimate_left_sec = g_pRelayStatus->route_period_sec_down;

	PLC_GetFoundListIntervalSecond(pBeaconParam->route_period_sec, NULL, &pBeaconParam->PcoFoundListIntervalSecond, &pBeaconParam->StaFoundListIntervalSecond);

	pBeaconParam->new_frequence = g_pRelayStatus->change_frequence_cb.new_frequence;
	pBeaconParam->new_frequence_second_down = g_pRelayStatus->change_frequence_cb.second_down;
	pBeaconParam->new_hrf_channel = g_pRelayStatus->change_hrf_channel_cb.new_frequence & 0xff;
	pBeaconParam->new_hrf_second_down = g_pRelayStatus->change_hrf_channel_cb.second_down;
	pBeaconParam->new_hrf_option = (g_pRelayStatus->change_hrf_channel_cb.new_frequence>>8) & 0xf;

	OSSchedUnlock(&err);

	if (!pBeaconParam->need_not_center_beacon_sta)
		return TRUE;

	pBeaconParam->not_center_beacon_info.num = 0;
	memset_special(pBeaconParam->not_center_beacon_info.sta, 0x0, sizeof(pBeaconParam->not_center_beacon_info.sta));

//	if (g_pRelayStatus->innet_num <= 0)
//		goto L_GET_SLOT;


	PLC_GetNotCenterBeaconSta(&pBeaconParam->wait_for_send_info, &pBeaconParam->not_center_beacon_info,&beacon_param_preget.Hrf_Hplc_Times);

//L_GET_SLOT:

	PLC_GenerateBeaconSlot(pBeaconParam->not_center_beacon_info.num, &pBeaconParam->beacon_period_ms, pBeaconParam->csma_slot_phase_ms, pBeaconParam->tie_csma_slot_phase_ms);

	return TRUE;

#if 0
	P_RELAY_PATH pRelayPath = PLC_GetNotCenterBeaconSta(pBeaconParam->sending_tei);
	if(NULL == pRelayPath)
	return TRUE;

	u8 p = 0;
	for(u8 i=0; i<pRelayPath->num; i++)
	{
		u16 tei_pco = pRelayPath->sta[i];
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei_pco);

		if(NULL == pSta)
		continue;

		P_NOT_CENTER_BEACON_STA pNotCenterSta = &pBeaconParam->not_center_beacon_info.sta[p++];
		pNotCenterSta->TEI = tei_pco;
		pNotCenterSta->Roles = pSta->roles;
		pNotCenterSta->Phase = pSta->hplc_phase;
		pBeaconParam->not_center_beacon_info.num = p;
	}

	free_buffer(__func__,pRelayPath);

	return TRUE;
#endif
}

BOOL PLC_OnGetStaParam(u16 tei, P_HPLC_STA_PARAM pStaParam)
{
	pStaParam->hplc_phase = PHASE_DEFAULT;
	pStaParam->receive_phase = PHASE_DEFAULT;

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);

	if (NULL == pSta)
		return FALSE;

	pStaParam->hplc_phase = (HPLC_PHASE)pSta->hplc_phase;

	if (pSta->receive_phase >= PHASE_A && pSta->receive_phase <= PHASE_C)
		pStaParam->receive_phase = (HPLC_PHASE)pSta->receive_phase;
	else if (pSta->hplc_phase >= PHASE_A && pSta->hplc_phase <= PHASE_C)
		pStaParam->receive_phase = (HPLC_PHASE)pSta->hplc_phase;
/*
	memset_special(pStaParam->relay_info, 0xff, sizeof(pStaParam->relay_info));
	u8 p = 0;
	while ((NULL != pSta) && (pSta->pco >= TEI_STA_MIN) && (pSta->pco <= TEI_STA_MAX))
	{
		if (p >= countof(pStaParam->relay_info))
			return FALSE;
		pStaParam->relay_info[p].tei = pSta->pco;
		pSta = PLC_GetValidStaInfoByTei(pSta->pco);
		if (NULL != pSta)
		{
			pStaParam->relay_info[p].Roles = tei == CCO_TEI ? ROLES_CCO : pSta->child == 0 ? ROLES_STA : ROLES_PCO;
			pStaParam->relay_info[p].Phase = PLC_GetSendPhase(pSta);
		}
		p++;
	}
*/
	return TRUE;
}

P_STA_BITMAP PLC_GetCCONeighbor()
{
	P_STA_BITMAP pStaBitmap = (P_STA_BITMAP)alloc_buffer(__func__,sizeof(STA_BITMAP));

	if (NULL == pStaBitmap)
		return NULL;

	memset_special(pStaBitmap, 0, sizeof(STA_BITMAP));

	for (u16 tei = TEI_STA_MIN; tei < TEI_CNT; tei++)
	{
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);

		if (NULL == pSta)
			continue;

		P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);

		if (0 == pStaStatus->found_list_counter_this_route)
			continue;

		if (0 == pStaStatus->found_list_count_last_route)
			continue;

		SET_STA_BITMAP(pStaBitmap->bitmap, tei);
		pStaBitmap->count++;
		pStaBitmap->size = (u8)GET_STA_BITMAP_SIZE(tei);
	}

	return pStaBitmap;
}
P_STA_BITMAP PLC_GetHrfCCONeighbor()
{
	P_STA_BITMAP pStaBitmap = (P_STA_BITMAP)alloc_buffer(__func__,sizeof(STA_BITMAP));

	if (NULL == pStaBitmap)
		return NULL;

	memset_special(pStaBitmap, 0, sizeof(STA_BITMAP));

	for (u16 tei = TEI_STA_MIN; tei < TEI_CNT; tei++)
	{
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);

		if (NULL == pSta)
			continue;

		P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);

		if (0 == pStaStatus->found_list_counter_this_route)
			continue;

		if (0 == pStaStatus->found_list_count_last_route)
			continue;

		SET_STA_BITMAP(pStaBitmap->bitmap, tei);
		pStaBitmap->count++;
		pStaBitmap->size = (u8)GET_STA_BITMAP_SIZE(tei);
	}

	return pStaBitmap;
}
#ifdef HPLC_CSG
u8 PLC_SendGetStaStatus(u8 broadcast, u8 mac_addr[MAC_ADDR_LEN], u32 element, HPLC_SEND_PHASE phase)
{
	USHORT msdu_len;
	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu;


	u16 tei = PLC_GetStaSnByMac(mac_addr);
	if (tei == TEI_CNT)
	{
		tei = TEI_BROADCAST;
	}


	pAppFrame = HplcApp_MakeReadStaStatusFrame(element, &msdu_len);

	if (NULL == pAppFrame)
	{
		goto L_Error;
	}

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;
	if (broadcast)
	{
		SOF_param.DstTei = TEI_BROADCAST;
		send_param.link=FreamPathBoth;
	}
	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;

	if (SOF_param.DstTei == TEI_BROADCAST)
	{
		if (!broadcast)
		{
			goto L_Error;
		}
		SOF_param.BroadcastFlag = 1;
		mac_param.DstTei = TEI_BROADCAST;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.SendLimit = 4;

	}
	else
	{
		SOF_param.BroadcastFlag = 0;
		mac_param.DstTei = tei;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.SendLimit = 3;
		send_param.send_phase |= SEND_PHASE_ALL;
		//send_param.link = FreamPathBoth;
	}
	SOF_param.NID = g_pPlmConfigPara->net_id;

	SOF_param.Lid = SOF_LID_MAINTENANCE_NET;



	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.mac_flag = MACFLAG_NONE;
	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, mac_addr, MAC_ADDR_LEN);


	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
	if (phase != SEND_PHASE_UNKNOW)
	{
		send_param.send_phase = phase;
	}
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}



//事件上报开关
unsigned char PLC_SendEventSwitch(u16 tei,u16 seq, u8 en)
{
	P_APP_PACKET pAppFrame = NULL;
	USHORT msdu_len = sizeof(APP_PACKET) - 1 + 4;
	u8 *pMsdu;
	u8 updata[4];
	memset(updata,0,sizeof(updata));
	updata[0]=en;
	pAppFrame = HplcApp_MakeCmdFrame(seq, tei == TEI_BROADCAST?EM_BUSINESS_NORESPONSE:EM_BUSINESS_NEED_RESPONSE, EM_CMD_BUSINESS_EBENT_SWITCH, updata, sizeof(updata), &msdu_len);

	if (NULL == pAppFrame)
	{
		goto L_Error;
	}

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;

	if (SOF_param.DstTei == TEI_BROADCAST)
	{
		send_param.link=FreamPathBoth;
		SOF_param.BroadcastFlag = 1;
		mac_param.DstTei = TEI_BROADCAST;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.SendLimit = 4;

	}
	else
	{
		SOF_param.BroadcastFlag = 0;
		mac_param.DstTei = tei;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.SendLimit = 3;
		send_param.send_phase |= SEND_PHASE_ALL;
	}
	SOF_param.NID = g_pPlmConfigPara->net_id;

	SOF_param.Lid = SOF_LID_MAINTENANCE_NET;



	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.mac_flag = MACFLAG_NONE;
//	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
//	memcpy_special(mac_param.DstMac, mac_addr, MAC_ADDR_LEN);


	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}

u8 PLC_SendGetStaInfo(u8 broadcast, u8 mac_addr[MAC_ADDR_LEN], u8 element_num, u8* element_array, HPLC_SEND_PHASE phase)
{
	USHORT msdu_len;
	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu;

	u8 dst_addr[LONG_ADDR_LEN] = {0};
	PLC_GetAppDstMac(mac_addr, dst_addr);
				
	u16 tei = PLC_GetStaSnByMac(dst_addr);
	if (tei == TEI_CNT)
	{
		tei = TEI_BROADCAST;
	}

	pAppFrame = HplcApp_MakeReadStaInfoFrameFromArray(element_num, element_array, &msdu_len);

	if (NULL == pAppFrame)
	{
		goto L_Error;
	}

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;
	if (broadcast)
	{
		SOF_param.DstTei = TEI_BROADCAST;
		send_param.link=FreamPathBoth;
	}
	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;

	if (SOF_param.DstTei == TEI_BROADCAST)
	{
		if (!broadcast)
		{
			goto L_Error;
		}
		SOF_param.BroadcastFlag = 1;
		mac_param.DstTei = TEI_BROADCAST;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.SendLimit = 4;

	}
	else
	{
		SOF_param.BroadcastFlag = 0;
		mac_param.DstTei = tei;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.SendLimit = 3;
		send_param.send_phase |= SEND_PHASE_ALL;
	}
	SOF_param.NID = g_pPlmConfigPara->net_id;

	SOF_param.Lid = SOF_LID_MAINTENANCE_NET;



	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.mac_flag = MACFLAG_NONE;
	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, dst_addr, MAC_ADDR_LEN);


	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
	if (phase != SEND_PHASE_UNKNOW)
	{
		send_param.send_phase = phase;
	}
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}

u8 PLC_SendGetChannelInfo(u8 broadcast, u8 mac_addr[MAC_ADDR_LEN], u16 start_sn, u8 num, HPLC_SEND_PHASE phase)
{
	USHORT msdu_len;
	P_APP_PACKET pAppFrame = NULL;
	u8 *pMsdu;

	u16 tei = PLC_GetStaSnByMac(mac_addr);

	if (tei == TEI_CNT)
	{
		tei = TEI_BROADCAST;
	}

	pAppFrame = HplcApp_MakeReadChannelInfoFrame(start_sn, num, &msdu_len);

	if (NULL == pAppFrame)
	{
		goto L_Error;
	}

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;
	if (broadcast)
	{
		SOF_param.DstTei = TEI_BROADCAST;
		send_param.link=FreamPathBoth;
	}

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;

	if (SOF_param.DstTei == TEI_BROADCAST)
	{
		goto L_Error;

	}
	else
	{
		SOF_param.BroadcastFlag = 0;
		mac_param.DstTei = tei;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.SendLimit = 3;
		send_param.send_phase |= SEND_PHASE_ALL;
	}
	SOF_param.NID = g_pPlmConfigPara->net_id;

	SOF_param.Lid = SOF_LID_MAINTENANCE_NET;



	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.mac_flag = MACFLAG_NONE;
	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, mac_addr, MAC_ADDR_LEN);


	mac_param.HeadType = MAC_HEAD_SHORT;     //MAC_HEAD_LONG
	mac_param.Vlan = SOF_LID_READ_METER;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
	pMsdu = MacCreateMsdu(mac_param.HeadType, (u8 *)pAppFrame, msdu_len, &msdu_len);
	free_buffer(__func__,pAppFrame);
	pAppFrame = (P_APP_PACKET)pMsdu;
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
	if (phase != SEND_PHASE_UNKNOW)
	{
		send_param.send_phase = phase;
	}
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}
#endif
//是否分别获取3个相位的邻居，然后分ABC发送？
void PLC_SendFoundListAuto()
{
	u8 *pMsdu = NULL;

//    HPLC_PHASE send_phase = PHASE_DEFAULT;

	u16 max_mac_len = MAX_MAC_FRAME_LEN;   //sizeof(MAC_HEAD) + sizeof(MM_HEAD) + sizeof(DISCOVER_NODE_LIST_PACKET) + TEI_CNT/8+1 + TEI_CNT + 50;

	pMsdu = (u8 *)alloc_buffer(__func__,max_mac_len);
	if (NULL == pMsdu)
		return;
	memset_special(pMsdu, 0, max_mac_len);
	P_MM_FRAME pMmFrame = (P_MM_FRAME)pMsdu;
	DISCOVER_NODE_LIST_PACKET *pListFrame = (DISCOVER_NODE_LIST_PACKET *)pMmFrame->body;
//    pStaBitmap = PLC_GetDirectSubStas(TEI_CCO, true);
//    pStaBitmap = PLC_GetAllSubStas(TEI_CCO);
	lockStaInfo();

	USHORT msdu_len;

	int map_size;
	u8 *bitmap=NeighborFilter(0, 500,&map_size);
	pMmFrame->head.MMType = MME_DISCOVER_NODE_LIST;



	pListFrame->TEI = TEI_CCO;
	pListFrame->PCOTei = TEI_INVALID;
	pListFrame->Role = ROLES_CCO;
	pListFrame->Level = 0;
	PLC_CopyCCOMacAddr(pListFrame->MacAddr, TRUE);

	pListFrame->Phase = PHASE_UNKNOW | (PHASE_UNKNOW << 2) | (PHASE_UNKNOW << 4);

	pListFrame->PCOSuccess = 100;             //代理站点通信成功率
	pListFrame->PCODownSuccess = 100;      //代理站点下行通信成功率
	pListFrame->STANum = 0;             //站点总数
	pListFrame->SendPacketNum = g_pRelayStatus->found_list_count_last_route;      //发送发现列表报文个数
	pListFrame->UpRouteNum = 0;          //上行路由条目总数
	pListFrame->RoutePeriodLeft = g_pRelayStatus->route_period_sec_down;        //路由周期到期剩余时间

#if defined(HPLC_CSG)
	pMmFrame->head.MMVersion = 1;
	pListFrame->StatComplete = 1;
	pListFrame->RouteItemLen =  8;      //固定8，表示8bit
#else
	PLC_CopyCCOMacAddr(pListFrame->CCOMacAddr, TRUE);
	pListFrame->PCO_SNR = 0;             //代理站点通信质量
	pListFrame->BitmapSize = map_size;         //位图大小
#endif
	pListFrame->MinSuccess = 100;       //最小通信成功率

	//上行路由条目
	//发现站点列表位图

	memcpy_special(pListFrame->Data, bitmap, map_size);
	//接收发现列表信息
	u16 p;

#if defined(HPLC_CSG)
	p = CSG_DISCOVER_BITMAP_SIZE;
#else
	p = pListFrame->BitmapSize;
#endif

	for (USHORT tei = TEI_STA_MIN; 
		 #ifdef HPLC_CSG
		 tei < CSG_DISCOVER_BITMAP_SIZE*8; 
		 #else
		 tei < TEI_CNT;
		 #endif
		 tei++)
	{
		if (IS_STA_BITMAP_SET(bitmap, tei))
		{
			P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
			pListFrame->Data[p++] = pStaStatus->found_list_count_last_route;

			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
			++pListFrame->STANum;
			if (NULL != pSta)
			{
//                send_phase = PLC_GetSendPhase(pSta);
			}
		}
	}

	msdu_len = sizeof(MM_HEAD) + (sizeof(DISCOVER_NODE_LIST_PACKET) - 1) + p;

	//发送
	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 0;
	//send_param.slot_type = SLOT_TYPE_CSMA;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	send_param.send_phase = SEND_PHASE_ALL;
	send_param.link=FreamPathBplc;
	send_param.doubleSend=0;

	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_DISCOVER_LIST;
	SOF_param.DstTei = TEI_BROADCAST;

	mac_param.DstTei = TEI_BROADCAST;
	mac_param.SendType = SENDTYPE_LOCAL;
	mac_param.SendLimit = 0;
	mac_param.HopCount = 1;
	mac_param.MsduType = MSDU_NET_MANAGER;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

#if defined(HPLC_CSG)
	#if defined(NW_NEW_TEST)
	mac_param.Broadcast = BROADCAST_DRT_NONE;
	#else
	mac_param.Broadcast = BROADCAST_DRT_DOWN;
	#endif   
	mac_param.HeadType = MAC_HEAD_LONG;
	mac_param.Vlan = VLAN_MM;
	mac_param.NID = (u8)g_pPlmConfigPara->net_id;
#else
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.mac_flag = MACFLAG_INCLUDE;
#endif

	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, BroadCastMacAddr, MAC_ADDR_LEN);

	//send_param.send_phase = send_phase;
	//send_param.send_phase = PHASE_A;
//    send_param.send_phase = PHASE_ALL;
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pMsdu, msdu_len);
	g_pRelayStatus->found_list_counter_this_route++;

#if 0
	send_param.send_phase = PHASE_B;
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8*)pMsdu, msdu_len);
	//g_pRelayStatus->found_list_counter_this_route++;

	send_param.send_phase = PHASE_C;
	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8*)pMsdu, msdu_len);
	//g_pRelayStatus->found_list_counter_this_route++;
#endif


	unLockStaInfo();
	free_buffer(__func__,pMsdu);

}

#ifdef GDW_ZHEJIANG
void PLC_SetOnlineLockTime(u32 OnlineLockTime, u32 abnormalOfflineLockTime)
{
	OS_ERR err;

	OSSchedLock(&err);

	g_pRelayStatus->online_lock_time_cb.OnlineLockTime = OnlineLockTime;
	g_pRelayStatus->online_lock_time_cb.abnormalOfflineLockTime = abnormalOfflineLockTime;
	g_pRelayStatus->online_lock_time_cb.second_down = 1;      //广播发送10s
	g_pRelayStatus->online_lock_time_cb.lockTimeFlag = 1;		//CCO需要发送锁网报文

	debug_str(DEBUG_LOG_INFO, "集中器开始设置在网锁定时间=%d, 异常离网锁定时间=%d， 广播时长=%d\r\n", 
							g_pRelayStatus->online_lock_time_cb.OnlineLockTime, 
							g_pRelayStatus->online_lock_time_cb.abnormalOfflineLockTime,
							g_pRelayStatus->online_lock_time_cb.second_down);

	OSSchedUnlock(&err);
}
#endif

void PLC_ChangeFrequence(u8 new_frequence)
{
	OS_ERR err;

	OSSchedLock(&err);

	g_pRelayStatus->change_frequence_cb.new_frequence = new_frequence;
	g_pRelayStatus->change_frequence_cb.second_down = 100;      //河北电科院台体设置频段后，等待360秒

	debug_str(DEBUG_LOG_INFO, "集中器开始设置新频率%d, %d\r\n", g_pRelayStatus->change_frequence_cb.new_frequence, g_pRelayStatus->change_frequence_cb.second_down);

	OSSchedUnlock(&err);
}
void HRF_ChangeFrequence(u8 channel,u8 option)
{
	OS_ERR err;

	OSSchedLock(&err);

	g_pRelayStatus->change_hrf_channel_cb.new_frequence = channel|(option<<8);
	g_pRelayStatus->change_hrf_channel_cb.second_down = 30;      //河北电科院台体设置频段后，等待360秒

	debug_str(DEBUG_LOG_INFO, "集中器开始设置新频率%d, %d\r\n", g_pRelayStatus->change_frequence_cb.new_frequence, g_pRelayStatus->change_frequence_cb.second_down);

	OSSchedUnlock(&err);
}

u8 PLC_StartSelfReg(u16 time_last_min)
{
	P_TMSG pMsg = NULL;
	OS_ERR err;

//	P_PLC_SELFREG_CB pcb  = &g_pRelayStatus->selfreg_cb;
//	if (EM_SELFREG_INIT != pcb->state)
//		return (!OK);

	pMsg = alloc_msg(__func__);
	if (NULL == pMsg)
		goto L_Error;

	pMsg->message = MSG_START_SELF_REG;
	pMsg->wParam = time_last_min;

	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return OK;

L_Error:
	free_msg(__func__,pMsg);
	return (!OK);
}

void PLC_StopSelfReg()
{
	P_TMSG pMsg = NULL;
	OS_ERR err;

	pMsg = alloc_msg(__func__);
	if (NULL == pMsg)
		goto L_Error;

	pMsg->message = MSG_STOP_SELF_REG;

	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return;

L_Error:
	free_msg(__func__,pMsg);
}

void PLC_SelfRegClearSendOffline()
{
	for (u16 tei = TEI_STA_MIN; tei < TEI_CNT; tei++)
	{
		P_STA_STATUS_INFO pStaStatus = &g_pStaStatusInfo[tei];
		pStaStatus->selfreg_send_offline = 0;
	}
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	//P_RELAY_INFO p_relay=(P_RELAY_INFO)(METER_RELAY_INFO_ADDR+0x12000000);
	if (g_MeterAddrSuccess)
	{
		for (int i = 0; i < CCTT_METER_NUMBER; i++)
		{
			if (g_pMeterRelayInfo[i].status.used != CCO_USERD)
			{
				//p_relay++;
				continue ;
			}
			if (!g_pMeterRelayInfo[i].status.cfged)
			{
				memset(&g_pMeterRelayInfo[i], 0xff, sizeof(RELAY_INFO));
				g_RelayStatus.area_dis_cb.getedZoneStatusBitMap[i >> 5] &= ~(1 << (i & 0x1f));
				g_RelayStatus.area_dis_cb.zoneErrorMap[i >> 5] &= ~(1 << (i & 0x1f));
			}
			//p_relay++;
		}
	}
	else
	{
		memset(g_pMeterRelayInfo, 0xff, CCTT_METER_NUMBER * sizeof(RELAY_INFO));
	}
	OS_CRITICAL_EXIT();
}

#ifndef COPY_OUT
#ifdef HPLC_GDW	
unsigned short selfRegCollReportNum = 0; //采集下需要上报从节点信息的数量
SELF_REG_COLL_REPORT_MAP g_SelfRegCollReportMap[CCTT_METER_NUMBER];

void AddSelfRegCollReportMap(u8* meter_mac, u8 protocol, u8 meter_num, u16 coll_sn)
{
	u16 total_num = 0;
	
	for (int i=0; i<CCTT_METER_NUMBER; i++)
	{
		if (!memIsHex(g_SelfRegCollReportMap[i].meter_addr, 0x00, 6))
		{
			if (macCmp(g_SelfRegCollReportMap[i].meter_addr, meter_mac)) //去重
			{
				debug_str(DEBUG_LOG_NET, "AddSelfRegCollReportMap, meter mac:%02X%02X%02X%02X%02X%02X already exist\r\n", 
					      meter_mac[5], meter_mac[4], meter_mac[3], meter_mac[2], meter_mac[1], meter_mac[0]);
				
				return;
			}
			
			total_num++;
		}

		if (total_num >= selfRegCollReportNum)
		{
			break;
		}
	}
	
	for (int i=0; i<CCTT_METER_NUMBER; i++)
	{
		if (memIsHex(g_SelfRegCollReportMap[i].meter_addr, 0x00, 6))
		{
			memcpy_special(g_SelfRegCollReportMap[i].meter_addr, meter_mac, 6);
			g_SelfRegCollReportMap[i].protocol = protocol;
			g_SelfRegCollReportMap[i].meter_num = meter_num;
			g_SelfRegCollReportMap[i].coll_sn = coll_sn;
			g_SelfRegCollReportMap[i].report_flag = 0;

			selfRegCollReportNum++;
			
			return;
		}
	}
}

void DelSelfRegCollReportMap(u8 meter_num, u16 coll_sn)
{
	u8 real_num = 0;
	
	for (int i=0; i<CCTT_METER_NUMBER; i++)
	{
		if (!memIsHex(g_SelfRegCollReportMap[i].meter_addr, 0x00, 6))
		{
			if ((g_SelfRegCollReportMap[i].coll_sn == coll_sn) &&
				(!g_SelfRegCollReportMap[i].report_flag))
			{
				real_num++;
				g_SelfRegCollReportMap[i].report_flag = 1;
			}
		}

		if (real_num == meter_num)
		{
			break;
		}
	}

	debug_str(DEBUG_LOG_NET, "DelSelfRegCollReportMap, collect tei:%d, report meterNum:%d-%d, total:%d\r\n", coll_sn, meter_num, real_num, selfRegCollReportNum);
}

void FindSelfRegCollReportMapFirstDev(u16* coll_sn)
{
	for (int i=0; i<CCTT_METER_NUMBER; i++)
	{
		if ((!memIsHex(g_SelfRegCollReportMap[i].meter_addr, 0x00, 6)) &&
			(!g_SelfRegCollReportMap[i].report_flag))
		{
			*coll_sn = g_SelfRegCollReportMap[i].coll_sn;

			return;
		}
	}
}
#endif

//从节点主动注册
u8 PLC_SelfRegProc(OS_TICK_64 tick_now)
{
#define CON_READ_SELF_REG_INTERVAL  4

	P_PLC_SELFREG_CB pcb = &g_pRelayStatus->selfreg_cb;

	if (pcb->start_stop_req==1)//start req
	{
		pcb->state=EM_SELFREG_START;
		pcb->start_stop_req=0;
#ifdef PROTOCOL_NW_2020		
		ClearOutListMeter(0x3);//删除多余节点列表和白名单
#endif		
	}
	else if (pcb->start_stop_req==2)//stop req
	{
		pcb->state=EM_SELFREG_REPORT_STOP;
		pcb->start_stop_req=3;
	}

	if (EM_SELFREG_INIT == pcb->state)
	{
		allow_save=1;
		return OK;
	}
	else if (EM_SELFREG_START == pcb->state)
	{
		allow_save=0;
		pcb->selfreg_start_tick = tick_now;
		pcb->selfreg_new_sta_tick = 0;
		pcb->selfreg_send_start_tick = 0;
		pcb->selfreg_read_meter_tick = tick_now;
#ifdef HPLC_CSG
		memset(selfreg_bitmap,0,sizeof(selfreg_bitmap));
#endif
		pcb->packet_seq_self_reg++;
		memset(pcb->upBitmap,0,sizeof(pcb->upBitmap));
		for (u16 tei = TEI_STA_MIN; tei < TEI_CNT; tei++)
		{
			P_STA_STATUS_INFO pStaStatus = &g_pStaStatusInfo[tei];
			pStaStatus->selfreg_read_node = 0;
			pStaStatus->selfreg_search_node = 0;
			pStaStatus->selfreg_send_offline = 0;
#ifdef HPLC_CSG
			P_STA_INFO p_sta=PLC_GetValidStaInfoByTei(tei);
			if (p_sta&&p_sta->net_state==NET_IN_NET)
			{
				selfreg_bitmap[tei>>5]|=1<<(tei&0x1f);
			}
#endif
		}

		PLC_ClearSelfRegList();

		pcb->selfreg_send_start_counter = 0;
		pcb->state = EM_SELFREG_SEND_START;

		debug_str(DEBUG_LOG_NET, "开始搜表\r\n");

		return OK;
	}
	else if (EM_SELFREG_SEND_START == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->selfreg_send_start_tick) <= 4 * OSCfg_TickRate_Hz)
			return OK;

		PLC_SendSelfRegStart();

		pcb->selfreg_send_start_tick = tick_now;
		if (++pcb->selfreg_send_start_counter >= 5)
		{
			pcb->selfreg_new_sta_tick = 0;
			pcb->state = EM_SELFREG_WAIT_SEND_START;
		}

		return OK;
	}
	else if (EM_SELFREG_WAIT_SEND_START == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->selfreg_start_tick) / OSCfg_TickRate_Hz >= pcb->selfreg_time_last_min * 60)
		{
			pcb->state = EM_SELFREG_REPORT_STOP;
			return OK;
		}

		if (TicksBetween64(tick_now, pcb->selfreg_send_start_tick) <= 10 * OSCfg_TickRate_Hz)
			return OK;

	#if 0
		if((pcb->selfreg_new_sta_tick>0) && (TicksBetween64(tick_now, pcb->selfreg_new_sta_tick)>=5*OSCfg_TickRate_Hz))
		{
			pcb->state = EM_SELFREG_SEND_START;
		}
		e1se
	#endif
		{
			pcb->selfreg_send_read_all = 0;
			pcb->state = EM_SELFREG_SEND_READ;
		}

		return OK;
	}
	else if (EM_SELFREG_SEND_READ == pcb->state)
	{
		OS_TICK_64 sec_time_last = pcb->selfreg_time_last_min * 60;
		OS_TICK_64 sec_pass = TicksBetween64(tick_now, pcb->selfreg_start_tick) / OSCfg_TickRate_Hz;
		OS_TICK_64 sec_left = 0;

		if (sec_time_last >= sec_pass)
			sec_left = sec_time_last - sec_pass;

		if (sec_pass < 10)       //30
			return OK;

		if (sec_pass >= sec_time_last)
		{
			pcb->state = EM_SELFREG_REPORT_STOP;
			return OK;
		}

		if (sec_left < 3)
			return OK;

	#if 1
		if (TicksBetween64(tick_now, pcb->selfreg_read_meter_tick) < (OS_TICK_64)(1.5 * OSCfg_TickRate_Hz))
			return OK;

		if (OK != PLC_SendSelfRegReadMeter())
		{
			debug_str(DEBUG_LOG_NET, " 搜表发完一轮\r\n");
			pcb->selfreg_send_read_all = 1;
		}

	#else
		if(TicksBetween64(tick_now, pcb->selfreg_read_meter_tick) < CON_READ_SELF_REG_INTERVAL*OSCfg_TickRate_Hz)
		return OK;
		for(u8 i=0; i<4; i++)
		{
			if(OK != PLC_SendSelfRegReadMeter())
			{
				pcb->selfreg_send_read_all = 1;
				break;
			}
		}
	#endif
		pcb->selfreg_read_meter_tick = tick_now;

		if (pcb->selfreg_send_read_all)
			pcb->state = EM_SELFREG_WAIT_SEND_READ;

	#if 0
		if((pcb->selfreg_new_sta_tick>0) && (TicksBetween64(tick_now, pcb->selfreg_new_sta_tick)>=5*OSCfg_TickRate_Hz))
		pcb->state = EM_SELFREG_WAIT_SEND_READ;
	#endif

		return OK;
	}
	else if (EM_SELFREG_WAIT_SEND_READ == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->selfreg_start_tick) / OSCfg_TickRate_Hz >= pcb->selfreg_time_last_min * 60)
		{
			pcb->state = EM_SELFREG_REPORT_STOP;
			return OK;
		}

		if (TicksBetween64(tick_now, pcb->selfreg_read_meter_tick) <= 5 * OSCfg_TickRate_Hz)
			return OK;

#ifdef HPLC_GDW
		PLM_ReportCollectSelfReg();
#endif

#if 0
		//pcb->state = EM_SELFREG_SEND_START;
#else
		pcb->state = EM_SELFREG_SEND_READ;
		pcb->selfreg_send_read_all = 0;
#endif

		return OK;
	}
	else if (EM_SELFREG_REPORT_STOP == pcb->state)
	{
#ifndef HPLC_CSG	
		if (pcb->start_stop_req!=3)
		{
			PLM_ReportRouterWorkStatus(2);
		}
#endif
		PLC_SelfRegClearSendOffline();
		pcb->selfreg_try_stop_counter = 0;
		pcb->state = EM_SELFREG_SEND_OFFLINE;
		debug_str(DEBUG_LOG_NET, "搜表结束\r\n");
		return OK;
	}
	else if (EM_SELFREG_SEND_OFFLINE == pcb->state)
	{

		u16 send_offline_counter = 0;
		for (u16 tei = TEI_STA_MIN; tei < TEI_CNT; tei++)
		{
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
			if (NULL == pSta)
				continue;

			if (NULL != PLC_GetWhiteNodeInfoByMac(pSta->mac))
				continue;

			P_STA_STATUS_INFO pStaStatus = &g_pStaStatusInfo[tei];
			if (pStaStatus->selfreg_send_offline)
				continue;

			if (OK == PLC_AddToOfflineList(pSta->mac, OFFLINE_NOT_IN_WHITE_LIST))
			{
				pStaStatus->selfreg_send_offline = 1;
				send_offline_counter++;
			}
			else
				break;
		}

		pcb->selfreg_send_offline_tick = tick_now;

		if (0 == send_offline_counter)
		{
			pcb->state = EM_SELFREG_WAIT_SEND_OFFLINE;
		}

		return OK;
	}
	else if (EM_SELFREG_WAIT_SEND_OFFLINE == pcb->state)
	{
		if (PLC_GetOfflineWaitCount() > 0)
			return OK;

		if (TicksBetween64(tick_now, pcb->selfreg_send_offline_tick) <= 10 * OSCfg_TickRate_Hz)
			return OK;

		pcb->selfreg_send_stop_tick = tick_now;
		pcb->selfreg_send_stop_counter = 0;
		pcb->state = EM_SELFREG_SEND_STOP;
		return OK;
	}
	else if (EM_SELFREG_SEND_STOP == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->selfreg_send_stop_tick) <= 4 * OSCfg_TickRate_Hz)
			return OK;

		PLC_SendSelfRegStop();
		pcb->selfreg_send_stop_tick = tick_now;
		if (++pcb->selfreg_send_stop_counter >= 5)
			pcb->state = EM_SELFREG_WAIT_SEND_STOP;

		return OK;
	}
	else if (EM_SELFREG_WAIT_SEND_STOP == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->selfreg_send_stop_tick) <= 10 * OSCfg_TickRate_Hz)
			return OK;

		if (++pcb->selfreg_try_stop_counter >= 3)
		{
			pcb->state = EM_SELFREG_STOP;
		}
		else
		{
			PLC_SelfRegClearSendOffline();
			pcb->state = EM_SELFREG_SEND_OFFLINE;
		}

		return OK;
	}
	else if (EM_SELFREG_STOP == pcb->state)
	{
#ifdef HPLC_CSG
		if (pcb->start_stop_req!=3)
		{
			PLM_ReportSelfRegStop();
		}
#endif		
		//清除STA信息????  还是等待自动离线 ????
		pcb->state = EM_SELFREG_INIT;
		pcb->start_stop_req=0;
		return OK;
	}
	else
	{
		return OK;
	}
}

//获取电表二采序列号
u8 PLC_GetCollSnProc(OS_TICK_64 tick_now)
{
	P_PLC_GET_SN_CB pcb = &g_pRelayStatus->get_sn_cb;
	switch (pcb->state)
	{
	case EM_GETS_SN_INIT:
		if (g_pRelayStatus->selfreg_cb.state!=EM_SELFREG_INIT)
		{
			pcb->next_hour_tick = tick_now+5*60*60*1000;//五小时后再次查询
			pcb->new_tei_tick=0;
			pcb->next_tick=0;
			return OK;
		}
		if (tick_now > pcb->next_hour_tick)
		{
			pcb->state = EM_GETS_SN_FIND;
			pcb->next_hour_tick = tick_now+5*60*60*1000;//五小时后再次查询
			return OK;
		}
		if (tick_now > pcb->new_tei_tick
			&&pcb->new_tei_tick!=0)
		{
			pcb->state = EM_GETS_SN_SEND;
			pcb->new_tei_tick=0;
			pcb->next_tick=0;
			return OK;
		}
		if (tick_now > pcb->next_tick
			&&pcb->next_tick!=0)
		{
			pcb->state = EM_GETS_SN_SEND;
			pcb->next_tick=0;
			return OK;
		}
		return OK;
	case EM_GETS_SN_FIND://全部查找进行查询
		for (int i= TEI_STA_MIN;i<TEI_CNT;i++)
		{
			P_STA_INFO p_Sta=PLC_GetValidStaInfoByTei(i);
			if (p_Sta
				&& p_Sta->net_state==NET_IN_NET
				&& (p_Sta->dev_type==II_COLLECTOR||p_Sta->dev_type==I_COLLECTOR))
				{
					pcb->bitmap[i>>5]|=1<<(i&0x1f);
				}
		}
		pcb->state = EM_GETS_SN_SEND;
		return OK;
	case EM_GETS_SN_SEND:
		{
			if (pcb->tei < TEI_STA_MIN || pcb->tei>TEI_STA_MAX)
			{
				pcb->tei=1;
			}
			u16 get_tei=0;
			for (int i=(pcb->tei>>5);i<MAX_BITMAP_SIZE/4;i++)
			{
				if (pcb->bitmap[i])
				{
					for (int j = 0; j < 32; j++)
					{
						if (pcb->bitmap[i]&(1<<j))
						{
							get_tei=(i<<5)|j;
							if (get_tei>pcb->tei)
							{
								break;
							}
						}
					}
					if (get_tei>pcb->tei)
					{
						break;
					}
				}
			}
			if (get_tei>pcb->tei)
			{
				pcb->tei=get_tei;
				pcb->next_tick=tick_now+5*1000;//5s后再查询下一个
			}
			else
			{
				pcb->tei=0;
				pcb->next_tick=tick_now+collect_search_interval;
			}
			if (pcb->tei>=TEI_STA_MIN&&pcb->tei<=TEI_STA_MAX)
			{
				debug_str(DEBUG_LOG_NET, "PLC_SendGetReadMeterColl, tei=%d\r\n", pcb->tei);
				PLC_SendGetReadMeterColl(pcb->tei);
			}
			pcb->state = EM_GETS_SN_END;
		}
		return OK;
	case EM_GETS_SN_END:
		{
			int i;
			for (i=0;i<MAX_BITMAP_SIZE/4;i++)
			{
				if (pcb->bitmap[i]!=0)
				{
					break;
				}
			}
			if (i==(MAX_BITMAP_SIZE/4))
			{
				pcb->next_tick=0;
			}

			pcb->state = EM_GETS_SN_INIT;
		}
		return OK;
	}
	return OK;
}


void PLC_OnPlmConfirm()
{
	P_TMSG pMsg = alloc_msg(__func__);

	if (NULL == pMsg)
		goto L_Error;

	pMsg->message = MSG_ON_PLM_CONFIRM;

	OS_ERR err;
	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return;

L_Error:
	free_msg(__func__,pMsg);
}

void PLC_OnPlmRequestReadItem(P_PLM_READ_ITEM_RSP pPlmReadItem)
{
	P_TMSG pMsg = alloc_msg(__func__);

	if (NULL == pMsg)
		goto L_Error;

	pMsg->message = MSG_ON_PLM_READ_ITEM;
	pMsg->lParam = (LPARAM)pPlmReadItem;

	OS_ERR err;
	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return;

L_Error:
	free_msg(__func__,pMsg);
	free_buffer(__func__,pPlmReadItem);
}

u8 PLC_SendRouteRead(P_PLC_ROUTE_READ_CB pReadItemInfo,u16 first_read)
{
	P_PLM_READ_ITEM_RSP pPlmRsp = pReadItemInfo->pPlmRsp;

	P_READ_MSG_INFO pReadMsg = (P_READ_MSG_INFO)alloc_buffer(__func__,sizeof(READ_MSG_INFO) - 1 + pPlmRsp->len);
	if (NULL == pReadMsg)
	{
		free_buffer(__func__,pPlmRsp);
		pReadItemInfo->pPlmRsp = NULL;
		return (!OK);
	}

	pReadMsg->header.broadcast = 0;
	memcpy_special(pReadMsg->header.mac, pReadItemInfo->plmCmd.node_addr, MAC_ADDR_LEN);
	pReadMsg->header.meter_protocol_plc = pReadItemInfo->protocol_plc;
	pReadMsg->header.packet_id = APP_PACKET_ROUTE;
	pReadMsg->header.msg_len = pPlmRsp->len;
	pReadMsg->header.read_time = first_read?0:1;
	memcpy_special(pReadMsg->body, pPlmRsp->data, pPlmRsp->len);

	PLC_SendDirRead(pReadMsg);
	free_buffer(__func__,pReadMsg);

	return OK;
}

u8 PLC_DirReadMeterProc(OS_TICK_64 tick_now)
{
	P_PLC_DIR_READ_CB pcb = &g_pRelayStatus->dir_read_cb;
	P_READ_MSG_INFO pReadMsg = pcb->pPlmCmd;
//	OS_TICK_64 alltimeout=0;
	if (EM_DIR_READ_INIT == pcb->state)
	{
		pcb->pPlmCmd = pcb->pPlmCmdBak;
		if (NULL == pcb->pPlmCmd)
			return OK;
		pcb->pPlmCmdBak=NULL;
		pcb->pPlcRsp = NULL;
		pReadMsg = pcb->pPlmCmd;
		pReadMsg->header.broadcast = 0;
//		if (g_PlmConfigPara.plc_max_timeout>=1)
//		{
//			alltimeout = tick_now + (g_PlmConfigPara.plc_max_timeout - 1) * OS_CFG_TICK_RATE_HZ+OS_CFG_TICK_RATE_HZ/2;//提前500ms
//		}
//		else
//		{
//			alltimeout = tick_now +OS_CFG_TICK_RATE_HZ/2;
//		}
		if (pReadMsg->header.time_related==1)
		{
			pcb->state = EM_DIR_READ_TIME_FIX_REQ ;
		}
		else
		{
			pcb->state = EM_DIR_READ_SEND;
		}
		return (!OK);
	}
	else if (EM_DIR_READ_TIME_FIX_REQ == pcb->state)
	{
		P_PLM_READ_ITEM_FIX_TIME_RSP p_rep = (P_PLM_READ_ITEM_FIX_TIME_RSP)alloc_buffer(__func__,sizeof(PLM_READ_ITEM_FIX_TIME_RSP) + pReadMsg->header.msg_len);
		if (p_rep == NULL)
		{
			pcb->state = EM_DIR_READ_POST;
			return (!OK);
		}
		memcpy(p_rep->node_addr, pReadMsg->header.mac, MAC_ADDR_LEN);
		memcpy(p_rep->data, pReadMsg->body, pReadMsg->header.msg_len);
		p_rep->len = pReadMsg->header.msg_len;
		p_rep->delay_seconds = 1; //暂时固定为1
		PLM_RequestReadItem2FixTime(PLM_PHASE_U, (u8 *)p_rep, sizeof(PLM_READ_ITEM_FIX_TIME_RSP) + p_rep->len);
		free_buffer(__func__,p_rep);
		pReadMsg->header.ts_send = tick_now;
		pcb->state = EM_DIR_READ_TIME_FIX_WAIT;
		return (!OK);
	}
	else if (EM_DIR_READ_TIME_FIX_WAIT == pcb->state)
	{
		OS_MSG_SIZE msgSize;
		OS_ERR err;
		P_MSG_INFO p_CccttSendMsg = (P_MSG_INFO)OSQPend(&g_pOsRes->q_up_rx_plc, 0, OS_OPT_PEND_NON_BLOCKING, &msgSize, NULL, &err);
		if (err == OS_ERR_NONE)
		{
			P_PLC_READ_ITEM_RSP_GDW2009 pReadIfo = (P_PLC_READ_ITEM_RSP_GDW2009)p_CccttSendMsg->msg_buffer;
			if (pReadIfo->len != 0)
			{
				pcb->state = EM_DIR_READ_SEND;
				if (pReadMsg->header.msg_len != pReadIfo->len) //得到的并非上次申请修改时间的帧
				{
					free_send_buffer(__func__,p_CccttSendMsg);
					pcb->state = EM_DIR_READ_POST;
					return (!OK);
				}
				pReadMsg->header.time_related = 0;
				memcpy(pReadMsg->body, pReadIfo->data, pReadMsg->header.msg_len);
				free_send_buffer(__func__,p_CccttSendMsg);
			}
			else //len =0放弃本次发送
			{
				free_send_buffer(__func__,p_CccttSendMsg);
				pcb->state = EM_DIR_READ_POST;
				return (!OK);
			}
		}
		else if (TicksBetween64(tick_now, pReadMsg->header.ts_send) >= 3 * OSCfg_TickRate_Hz)
		{
			pcb->state = EM_DIR_READ_POST;
			return (!OK);
		}
		return (!OK);
	}
	else if (EM_DIR_READ_SEND == pcb->state)
	{
		u8 send_flag = (!OK);

#if defined(PLC_X4_FUNC)
		u16 tei = PLC_GetStaSnByMac(pReadMsg->header.mac);
		if (EM_SENDER_AFN_DELETE_COLL_TASK_CSG == pReadMsg->header.sender)
			send_flag = PLC_X4SendDeleteReadTask(tei, pReadMsg->body[0]);
		else if (EM_SENDER_AFN_READ_COLL_TASK_ID_CSG == pReadMsg->header.sender)
			send_flag = PLC_X4SendReadTaskID(tei);
		else if (EM_SENDER_AFN_READ_COLL_TASK_DETAIL_CSG == pReadMsg->header.sender)
			send_flag = PLC_X4SendReadTaskDetail(tei, pReadMsg->body[0]);
		else
#endif
			send_flag = PLC_SendDirRead(pReadMsg);

		if (OK != send_flag)
		{
			pcb->state = EM_DIR_READ_POST;
			return (!OK);
		}
		pReadMsg->header.ts_send = tick_now;
		pReadMsg->header.ts_timeout = PLC_GetReadMeterTimeoutTick(pReadMsg->header.msg_len);


#if defined(SUPPORT_READ_METER_NOT_IN_WHITELIST) && !defined(OVERSEA)	
		if (PLC_get_sn_from_addr(pReadMsg->header.mac) >= CCTT_METER_NUMBER&&pReadMsg->header.read_time==0)
		{
			pReadMsg->header.read_time = MAX_MINOR_LOOP_READ_METER*2-1; //广播抄读
		}
		//else
#endif			
		{
			if (pReadMsg->header.read_count * MAX_MINOR_LOOP_READ_METER > pReadMsg->header.read_time++)
			{
				if (((pReadMsg->header.read_count - 1) * MAX_MINOR_LOOP_READ_METER) <= pReadMsg->header.read_time)
				{
#ifdef DLMS_BROADCAST
					pReadMsg->header.broadcast = 1;	//24年8月决定开启广播增加抄表成功率，在广播时带上表mac地址，STA去过滤
#elif defined(OVERSEA)
					pReadMsg->header.broadcast = 0; //海外版本不进行广播重传尝试
#else			
					pReadMsg->header.broadcast = 1;
#endif
				}

			}
		}
		pcb->state = EM_DIR_READ_WAIT_PLC;
		return (!OK);
	}
	else if (EM_DIR_READ_WAIT_PLC == pcb->state)
	{
		if (NULL != pcb->pPlcRsp)
		{
			//pReadMsg->header.packet_seq_plc = pReadMsg->header.packet_first_seq_plc;
			pcb->state = EM_DIR_READ_POST;
			return (!OK);
		}
		if (NULL != pcb->pPlmCmdBak)//下来了新的数据
		{
			//pReadMsg->header.packet_seq_plc = pReadMsg->header.packet_first_seq_plc;
			pcb->state = EM_DIR_READ_POSTED;
			return (!OK);
		}
		if (TicksBetween64(tick_now, pReadMsg->header.ts_send) >= pReadMsg->header.ts_timeout)
		{
//			if (tick_now>alltimeout)
//			{
//				pcb->state = EM_DIR_READ_POST;
//			}
			if ((pReadMsg->header.read_count * MAX_MINOR_LOOP_READ_METER) > pReadMsg->header.read_time)
			{
				pcb->state = EM_DIR_READ_SEND;
			}
			else
			{
				pcb->state = EM_DIR_READ_POST;
			}
			return (!OK);
		}
		return (OK);
	}
	else if (EM_DIR_READ_POST == pcb->state)
	{
		P_PLC_RSP_MSG_INFO  pPlcRsp = pcb->pPlcRsp;

		u8 dst_addr[LONG_ADDR_LEN] = {0};
		PLC_GetAppDstMac(pReadMsg->header.mac, dst_addr);
		PLM_PHASE plm_phase = PLC_GetPlmPhaseByMacAddr(dst_addr);

#if defined(PROTOCOL_GD_2016)
		if (EM_SENDER_AFN_DELETE_COLL_TASK_CSG == pReadMsg->header.sender)
		{
			if (NULL != pPlcRsp)
				PLM_DeleteCollTaskResponse_gd(pReadMsg->header.packet_seq_3762, pPlcRsp->body[0]);
			else
				PLM_DeleteCollTaskResponse_gd(pReadMsg->header.packet_seq_3762, 0xff);
		}
		else if (EM_SENDER_AFN_READ_COLL_TASK_ID_CSG == pReadMsg->header.sender)
		{
			if (NULL != pPlcRsp)
				PLM_ReadCollTaskIDResponse_gd(pReadMsg->header.packet_seq_3762, pPlcRsp->body, pPlcRsp->header.msg_len);
			else
				PLM_ReadCollTaskIDResponse_gd(pReadMsg->header.packet_seq_3762, NULL, 0);
		}
		else if (EM_SENDER_AFN_READ_COLL_TASK_DETAIL_CSG == pReadMsg->header.sender)
		{
			if (NULL != pPlcRsp)
				PLM_ReadCollTaskDetailResponse_gd(pReadMsg->header.packet_seq_3762, pPlcRsp->body, pPlcRsp->header.msg_len);
			else
				PLM_ReadCollTaskDetailResponse_gd(pReadMsg->header.packet_seq_3762, NULL, 0);
		}
		else
#endif
		{
			if (NULL != pPlcRsp)
				PLM_ReportDirRead(pReadMsg, plm_phase, pPlcRsp->body, pPlcRsp->header.msg_len);
			else
				PLM_ReportDirRead(pReadMsg, plm_phase, NULL, 0);
		}

		pcb->tick_post = tick_now;
		pcb->state = EM_DIR_READ_WAIT_POST;
		return (!OK);
	}
	else if (EM_DIR_READ_WAIT_POST == pcb->state)
	{
		//在等待的过程中可能会下来一个点抄帧，所以不等了
		//if(TicksBetween64(tick_now, pcb->tick_post) >= OSCfg_TickRate_Hz/200)
		pcb->state = EM_DIR_READ_POSTED;
		return (!OK);
	}
	else if (EM_DIR_READ_POSTED == pcb->state)
	{
		free_buffer(__func__,pcb->pPlmCmd);
		pcb->pPlmCmd = NULL;
		free_buffer(__func__,pcb->pPlcRsp);
		pcb->pPlcRsp = NULL;
		pcb->state = EM_DIR_READ_INIT;
		return OK;
	}
	else
	{
		return OK;
	}
}

#if defined(PROTOCOL_GD_2016)
//抄读采集器负荷曲线自动填充数据
bool PLC_ReadCollectGraphAutoload(u8* reqData, u8** rspData)
{
	//南网本地接口E8050501上报任务数据，报文长度有限制（255字节）
	/*12字节为地址域，8字节为AFN(1)+SEQ(1)+DI(4)+CS(1)+END(1)，3字节为任务ID(2)+任务报文长度(1)，1字节业务代码*/
	u16 max_len = PLC_BUFFER_UART_SIZE - (sizeof(LMP_LINK_HEAD_GD)-1) - 12 - 8 - 3 - 1;
	u16 msg_len = 0;
	
	P_PLC_RSP_MSG_INFO pPlcRsp = (P_PLC_RSP_MSG_INFO)alloc_buffer(__func__, sizeof(PLC_RSP_MSG_HEAD) + max_len);
	if (NULL != pPlcRsp)
	{
		u8 itemCount = 0;
		u16 DIToatlLen = 0, copyLen = 0;
		u8 DILen[36] = {0}; //南网21标准目前最多支持36个数据标识
					
		P_APP_READ_GRAPH_ITEM_CSG21_DOWN pReadGraphItemCSG21Down = (P_APP_READ_GRAPH_ITEM_CSG21_DOWN)reqData;
		P_GRAPH_ITEM_DOWN_CSG21 pGraphItemCSG21Down = (P_GRAPH_ITEM_DOWN_CSG21)pReadGraphItemCSG21Down->Data;

		//计算一个时间点，所有数据标识回复数据长度之和DIToatlLen
		if (!Find_Down_CSG21_DI_Len(pReadGraphItemCSG21Down->Data, pReadGraphItemCSG21Down->ItemNum, pReadGraphItemCSG21Down->Count, DILen, &DIToatlLen))
		{
			free_buffer(__func__, pPlcRsp);
			return false;
		}

		//业务码
		pPlcRsp->body[msg_len++] = APP_SHENHUA_APP_CODE_GRAPH; 

		//P_APP_READ_GRAPH_ITEM_CSG21_DOWN定长部分
		memcpy_special(&pPlcRsp->body[msg_len], reqData, sizeof(APP_READ_GRAPH_ITEM_CSG21_DOWN));
		msg_len += sizeof(APP_READ_GRAPH_ITEM_CSG21_DOWN);

		//理论支持最大采集点数量
		itemCount = (max_len - sizeof(APP_READ_GRAPH_ITEM_CSG21_UP) - 4*pReadGraphItemCSG21Down->ItemNum)/DIToatlLen;
		//实际采集点数量，和理论最大数量取最小值
		itemCount = MIN(itemCount, pReadGraphItemCSG21Down->Count);

		for(int i=0; i<pReadGraphItemCSG21Down->ItemNum; i++)
		{						
			//填充每个数据标识对应的实际采集点数量
			copyLen = itemCount*DILen[i];

			//数据标识
			memcpy_special(&pPlcRsp->body[msg_len], (u8*)pGraphItemCSG21Down, 4);
			msg_len += 4;
			
			//数据
			memset_special(&pPlcRsp->body[msg_len], 0xFF, copyLen);
			msg_len += copyLen;

			pGraphItemCSG21Down = (P_GRAPH_ITEM_DOWN_CSG21)((u8 *)pGraphItemCSG21Down + 4);	
		}	

		pPlcRsp->header.msg_len = msg_len;
		*rspData = (u8 *)pPlcRsp;
		return true;
	}

	return false;
}

u16  taskid[MAX_NW_PARALLEL_NUM];
u8  taskmac[MAX_NW_PARALLEL_NUM][MAC_ADDR_LEN];
u8 PLC_RouteReadMeterProc(OS_TICK_64 tick_now)
{
	P_PLC_TASK_READ_CB pcb = &g_pRelayStatus->task_read_cb;

	if (EM_TASK_READ_INIT == pcb->state)
	{
		if (g_pRelayStatus->pause_sec)
        {
            //debug_str(DEBUG_LOG_NET, "RouteReadMeter pause_sec not 0, pause_sec:%d\r\n", g_pRelayStatus->pause_sec);
			return OK;
        }
		u8 find_num=MAX_NW_PARALLEL_NUM;
		memset(taskid,0xff,sizeof(taskid));
		memset(taskmac,0xff,sizeof(taskmac));
		for (int i=0;i<MAX_NW_PARALLEL_NUM;i++)
		{
			if (pcb->array[i].pReadTask!=NULL)
			{
				taskid[i] = pcb->array[i].pReadTask->task_header.task_id;
#if 0
				memcpy(taskmac[i], pcb->array[i].pReadTask->mac_addr,MAC_ADDR_LEN);
#else
				PLC_GetAppDstMac(pcb->array[i].pReadTask->mac_addr,taskmac[i]);
#endif
			}
			else
			{
				find_num=i;
			}
		}
		if (find_num<MAX_NW_PARALLEL_NUM)
		{
			PLC_fetch_task_read_item_gd(&pcb->array[find_num].pReadTask, false, taskid,taskmac, MAX_NW_PARALLEL_NUM);
			if(pcb->array[find_num].pReadTask)
			{
				pcb->array[find_num].bcast_sta = 0;
				pcb->array[find_num].pkt_devtype = 0xFF;
				pcb->array[find_num].first_read_tick=tick_now;
				
				if (PLC_get_sn_from_addr(pcb->array[find_num].pReadTask->mac_addr) >= CCTT_METER_NUMBER)
				{
					pcb->idx = find_num;
					pcb->state = EM_TASK_READ_POST;
					return OK;
				}

#if 1
				u8 dst_addr[LONG_ADDR_LEN] = {0};
				PLC_GetAppDstMac(pcb->array[find_num].pReadTask->mac_addr, dst_addr);			
							
				P_STA_INFO p_Sta=PLC_GetStaInfoByMac(dst_addr);
#else
				P_STA_INFO p_Sta=PLC_GetStaInfoByMac(pcb->array[find_num].pReadTask->mac_addr);
#endif				

#ifdef GRAPH_COLLECT_AUTOLOAD
				if (p_Sta && (p_Sta->dev_type==II_COLLECTOR||p_Sta->dev_type==I_COLLECTOR))		
				{
					P_READ_TASK_INFO pReadTask = pcb->array[find_num].pReadTask;
					
					if (pReadTask->meter_moudle &&
						pReadTask->task_buffer[0] == APP_GRAPH_FUNC_CODE_READ_21PROTOCOL)
					{
						debug_str(DEBUG_LOG_NET, "ReadGraph collect, mac:%02x%02x%02x%02x%02x%02x\r\n",
							  pcb->array[find_num].pReadTask->mac_addr[5], pcb->array[find_num].pReadTask->mac_addr[4], 
							  pcb->array[find_num].pReadTask->mac_addr[3], pcb->array[find_num].pReadTask->mac_addr[2], 
							  pcb->array[find_num].pReadTask->mac_addr[1], pcb->array[find_num].pReadTask->mac_addr[0]);
						u8* rspData = NULL;
						if (PLC_ReadCollectGraphAutoload(pReadTask->task_buffer, &rspData))
						{
							debug_str(DEBUG_LOG_NET, "ReadGraph collect, autoload, idx:%d\r\n", find_num);
							pcb->idx = find_num;
							pcb->array[pcb->idx].pPlcRsp = (P_PLC_RSP_MSG_INFO)rspData;
							pcb->state = EM_TASK_READ_POST;
							return OK;
						}
					}
				}
#endif

				if (p_Sta==NULL 
					#ifndef NW_TEST
					|| p_Sta->net_state!=NET_IN_NET
					#endif
					)
				{
					if (g_pRelayStatus->II_geted==0||p_Sta)//没有二采  或者设备有指向
					{
						pcb->idx = find_num;
						pcb->state = EM_TASK_READ_POST;
						return OK;
					}
					else
					{
						pcb->array[find_num].bcast_sta = 1;

						debug_str(DEBUG_LOG_NET, "ReadMeter bcast, mac:%02x%02x%02x%02x%02x%02x\r\n",
							  pcb->array[find_num].pReadTask->mac_addr[5], pcb->array[find_num].pReadTask->mac_addr[4], 
							  pcb->array[find_num].pReadTask->mac_addr[3], pcb->array[find_num].pReadTask->mac_addr[2], 
							  pcb->array[find_num].pReadTask->mac_addr[1], pcb->array[find_num].pReadTask->mac_addr[0]);
					}
				}

#ifdef GRAPH_COLLECT_REWRITE_DEVTYPE
				if (pcb->array[find_num].pReadTask->meter_moudle &&
					pcb->array[find_num].pReadTask->task_buffer[0] == APP_GRAPH_FUNC_CODE_READ_21PROTOCOL)
				{
					pcb->array[find_num].pkt_devtype = pcb->array[find_num].pReadTask->task_buffer[1];
				}
#endif

				pcb->array[find_num].packet_seq_plc = ++g_pRelayStatus->packet_seq_read_meter;
				pcb->array[find_num].plc_send_counter = 3;
				pcb->array[find_num].plc_send_num = 0;
				pcb->array[find_num].tick_next=0;
				if (p_Sta && p_Sta->net_state == NET_OUT_NET)
				{
					pcb->array[find_num].plc_send_num =(pcb->array[find_num].plc_send_counter - 1) * MAX_MINOR_LOOP_READ_METER;
				}                
			}
		}
		
		//寻找一个当前需要发送的sta  需要考虑优先不同sta？
		for (;pcb->idx<MAX_NW_PARALLEL_NUM;pcb->idx++)
		{
			if (pcb->array[pcb->idx].pReadTask)
			{
				if (NULL==PLC_find_read_task_gd(pcb->array[pcb->idx].pReadTask->task_type, pcb->array[pcb->idx].pReadTask->task_header.task_id))
				{
					pcb->state = EM_TASK_READ_EXIT;
					return (!OK);
				}
				if (NULL != pcb->array[pcb->idx].pPlcRsp)
				{
					pcb->state = EM_TASK_READ_POST;
					return (!OK);
				}
				if (tick_now > pcb->array[pcb->idx].tick_next)
				{
					if (pcb->array[pcb->idx].plc_send_counter * MAX_MINOR_LOOP_READ_METER > pcb->array[pcb->idx].plc_send_num)
					{
						pcb->state = EM_TASK_READ_SEND;
					}
					else
					{
						pcb->state = EM_TASK_READ_POST;
					}
					return (!OK);
				}

			}
		}
		if (pcb->idx>=MAX_NW_PARALLEL_NUM)
		{
			pcb->idx=0;
		}
		pcb->state = EM_TASK_READ_INIT;

		return (OK);
	}
	else if (EM_TASK_READ_SEND == pcb->state)
	{
		P_READ_TASK_INFO pReadTask = pcb->array[pcb->idx].pReadTask;

		u8 send_flag = (!OK);
		BOOL broadcast;
		HPLC_SEND_PHASE phase;
		if ((pcb->array[pcb->idx].plc_send_counter - 1) * MAX_MINOR_LOOP_READ_METER < pcb->array[pcb->idx].plc_send_num)
		{
			broadcast = true;
		}
		else
		{
			broadcast = false;
		}
		if ((pcb->array[pcb->idx].plc_send_num % MAX_MINOR_LOOP_READ_METER) == (MAX_MINOR_LOOP_READ_METER - 1))
		{
			phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
		}
		else
		{
			phase = SEND_PHASE_UNKNOW;
		}

		if (pcb->array[pcb->idx].bcast_sta == 1)
		{
			broadcast = true;
			phase = SEND_PHASE_UNKNOW;
            debug_str(DEBUG_LOG_NET, "ReadMeter bcast_sta, mac:%02x%02x%02x%02x%02x%02x, seq:%d, send_flag:%d\r\n",
							  pReadTask->mac_addr[5], pReadTask->mac_addr[4], 
							  pReadTask->mac_addr[3], pReadTask->mac_addr[2], 
							  pReadTask->mac_addr[1], pReadTask->mac_addr[0],
							  pcb->array[pcb->idx].packet_seq_plc, send_flag);            
		}
		
		if (EM_TASK_TYPE_READ_TASK == pReadTask->task_type)
		{            
			send_flag = PLC_SendDataTrans(broadcast,pReadTask->meter_moudle, phase,pcb->array[pcb->idx].packet_seq_plc, pReadTask->mac_addr, pReadTask->task_buffer, pReadTask->task_header.task_len);

            debug_str(DEBUG_LOG_NET, "ReadMeter Req, mac:%02x%02x%02x%02x%02x%02x, seq:%d, send_flag:%d\r\n",
							  pReadTask->mac_addr[5], pReadTask->mac_addr[4], 
							  pReadTask->mac_addr[3], pReadTask->mac_addr[2], 
							  pReadTask->mac_addr[1], pReadTask->mac_addr[0],
							  pcb->array[pcb->idx].packet_seq_plc, send_flag);
            
			if (pcb->array[pcb->idx].bcast_sta == 1)
			{
				pcb->array[pcb->idx].plc_send_num=pcb->array[pcb->idx].plc_send_counter * MAX_MINOR_LOOP_READ_METER;
			}
		}
		else if (EM_TASK_TYPE_TIME_TASK == pReadTask->task_type)
		{
			send_flag = PLC_SendPreciseTime(broadcast,pReadTask->meter_moudle, phase,pcb->array[pcb->idx].packet_seq_plc, pReadTask->mac_addr, pReadTask->task_buffer, pReadTask->task_header.task_len);
			if(pcb->array[pcb->idx].plc_send_num>=MAX_MINOR_LOOP_READ_METER-1)//单拨1轮就结束
			{
				pcb->array[pcb->idx].plc_send_num=pcb->array[pcb->idx].plc_send_counter * MAX_MINOR_LOOP_READ_METER;
			}
		}
		else if (EM_TASK_TYPE_NEW_TIME_TASK == pReadTask->task_type)
		{
			send_flag = PLC_SendNewPreciseTime(broadcast,pReadTask->meter_moudle, phase,pcb->array[pcb->idx].packet_seq_plc, pReadTask->mac_addr, pReadTask->task_buffer, pReadTask->task_header.task_len);
			if(pcb->array[pcb->idx].plc_send_num>=MAX_MINOR_LOOP_READ_METER-1)//单拨1轮就结束
			{
				pcb->array[pcb->idx].plc_send_num=pcb->array[pcb->idx].plc_send_counter * MAX_MINOR_LOOP_READ_METER;
			}
		}
#ifdef PLC_X4_FUNC
		else
		{
			send_flag = PLC_X4SendReadCollTaskData(broadcast, phase, pReadTask->mac_addr, pReadTask->task_buffer, pReadTask->task_header.task_len);
		}
#endif
		if (OK != send_flag)
		{
			if (NULL != PLC_find_read_task_gd(pReadTask->task_type, pReadTask->task_header.task_id))
			{
				if (EM_TASK_TYPE_READ_TASK == pReadTask->task_type)
				{
					if(pReadTask->no_response_flag == 0) //需要上报给集中器
					{
					    debug_str(DEBUG_LOG_NET, "ReportTaskStatus(0x01), id:%d, mac:%02x%02x%02x%02x%02x%02x\r\n", pReadTask->task_header.task_id,
			                                      pReadTask->mac_addr[5], pReadTask->mac_addr[4], pReadTask->mac_addr[3], 
			                                      pReadTask->mac_addr[2], pReadTask->mac_addr[1], pReadTask->mac_addr[0]);
						PLM_ReportTaskStatus_gd(pReadTask->sour_frome, pReadTask->task_header.task_id, pReadTask->mac_addr, 0x01);
					}
				}
				else
				{
					PLM_ReportCollDataTaskStatus_gd(pReadTask->task_header.task_id, pReadTask->mac_addr, 0x01);
				}
			}
			pcb->state = EM_TASK_READ_EXIT;
			return OK;
		}
		pcb->array[pcb->idx].tick_next = tick_now+PLC_GetReadMeterTimeoutTick(pReadTask->task_header.task_len);
		pcb->array[pcb->idx].plc_send_num++;
		pcb->state = EM_TASK_READ_INIT;

		return (!OK);
	}
	else if (EM_TASK_READ_POST == pcb->state)
	{
		P_READ_TASK_INFO pReadTask = pcb->array[pcb->idx].pReadTask;

		if (NULL != PLC_find_read_task_gd(pReadTask->task_type, pReadTask->task_header.task_id))
		{
			P_PLC_RSP_MSG_INFO  pPlcRsp = pcb->array[pcb->idx].pPlcRsp;
			u8 dst_addr[LONG_ADDR_LEN] = {0};
			PLC_GetAppDstMac(pReadTask->mac_addr, dst_addr);			
						
			P_STA_INFO p_Sta=PLC_GetStaInfoByMac(dst_addr);
			u8 over_time=(tick_now-pcb->array[pcb->idx].first_read_tick)/1000;
			if (EM_TASK_TYPE_READ_TASK == pReadTask->task_type)
			{
				if(pReadTask->no_response_flag == 0) //需要上报给集中器
				{
					if (NULL != pPlcRsp)
					{
						if(p_Sta)
						{
							p_Sta->ReaderSec=over_time;
						}
						if (pReadTask->task_header.task_mode.resp_flag)
						{
#ifdef DEBUG_TASK_DATA						
						    P_GD_DL645_BODY pDL645_task = NULL;

            				pDL645_task = PLC_check_dl645_frame(pPlcRsp->body, pPlcRsp->header.msg_len);

                            if (pDL645_task)
                            {
                                if (pDL645_task->dataLen >= 4)
                        		{
                        		    debug_str(DEBUG_LOG_NET, "ReportTaskRead645, id:%d, meter mac:%02x%02x%02x%02x%02x%02x, DI:0x%08x\r\n", pReadTask->task_header.task_id,
    			                                      pReadTask->mac_addr[5], pReadTask->mac_addr[4], pReadTask->mac_addr[3], 
    			                                      pReadTask->mac_addr[2], pReadTask->mac_addr[1], pReadTask->mac_addr[0],
    			                                      *(u32*)&pDL645_task->dataBuf[0]-0x33333333);
                        		}
                                else
                                {
                                    debug_str(DEBUG_LOG_NET, "ReportTaskRead645, id:%d, meter mac:%02x%02x%02x%02x%02x%02x, ctrl:0x%x, len:%d\r\n", pReadTask->task_header.task_id,
    			                                      pReadTask->mac_addr[5], pReadTask->mac_addr[4], pReadTask->mac_addr[3], 
    			                                      pReadTask->mac_addr[2], pReadTask->mac_addr[1], pReadTask->mac_addr[0],
    			                                      pDL645_task->controlCode, pDL645_task->dataLen);
                                }
                            }
                            else
                            {
    						    debug_str(DEBUG_LOG_NET, "ReportTaskRead, id:%d, meter mac:%02x%02x%02x%02x%02x%02x, len:%d\r\n", pReadTask->task_header.task_id,
    			                                      pReadTask->mac_addr[5], pReadTask->mac_addr[4], pReadTask->mac_addr[3], 
    			                                      pReadTask->mac_addr[2], pReadTask->mac_addr[1], pReadTask->mac_addr[0],
    			                                      pPlcRsp->header.msg_len);
                            }
							PLM_ReportTaskRead_gd(pReadTask->sour_frome, pReadTask->task_header.task_id, pReadTask->mac_addr, pPlcRsp->body, pPlcRsp->header.msg_len);
#else
                            debug_str(DEBUG_LOG_NET, "ReportTaskRead, id:%d, meter mac:%02x%02x%02x%02x%02x%02x, len:%d use %d s\r\n", pReadTask->task_header.task_id,
			                                      pReadTask->mac_addr[5], pReadTask->mac_addr[4], pReadTask->mac_addr[3], 
			                                      pReadTask->mac_addr[2], pReadTask->mac_addr[1], pReadTask->mac_addr[0],
			                                      pPlcRsp->header.msg_len,over_time);
							PLM_ReportTaskRead_gd(pReadTask->sour_frome, pReadTask->task_header.task_id, pReadTask->mac_addr, pPlcRsp->body, pPlcRsp->header.msg_len);
#endif
						}
						else
						{
						    debug_str(DEBUG_LOG_NET, "ReportTaskStatus(0x00), id:%d, meter mac:%02x%02x%02x%02x%02x%02x use %d s\r\n", pReadTask->task_header.task_id,
			                                      pReadTask->mac_addr[5], pReadTask->mac_addr[4], pReadTask->mac_addr[3], 
			                                      pReadTask->mac_addr[2], pReadTask->mac_addr[1], pReadTask->mac_addr[0],over_time);
							PLM_ReportTaskStatus_gd(pReadTask->sour_frome, pReadTask->task_header.task_id, pReadTask->mac_addr, 0x00);
						}
					}
					else
					{
						if(p_Sta)
						{
							p_Sta->ReaderSec=MAX_READ_TIME;
						}
					    debug_str(DEBUG_LOG_NET, "ReportTaskStatus(0x01), id:%d, meter mac:%02x%02x%02x%02x%02x%02x use %d s\r\n", pReadTask->task_header.task_id,
			                                      pReadTask->mac_addr[5], pReadTask->mac_addr[4], pReadTask->mac_addr[3], 
			                                      pReadTask->mac_addr[2], pReadTask->mac_addr[1], pReadTask->mac_addr[0],over_time);
                        
						PLM_ReportTaskStatus_gd(pReadTask->sour_frome, pReadTask->task_header.task_id, pReadTask->mac_addr, 0x01);
					}
				}
			}
			else
			{
				if (NULL != pPlcRsp)
					PLM_ReportCollDataTaskData_gd(pPlcRsp->body, pPlcRsp->header.msg_len);
				else
					PLM_ReportCollDataTaskStatus_gd(pReadTask->task_header.task_id, pReadTask->mac_addr, 0x01);
			}
			pcb->array[pcb->idx].tick_next = tick_now+OSCfg_TickRate_Hz / 50;
			pcb->state = EM_TASK_READ_WAIT_POST;
		}
		else
		{
			pcb->state = EM_TASK_READ_EXIT;
		}

		return (!OK);
	}
	else if (EM_TASK_READ_WAIT_POST == pcb->state)
	{
		if (tick_now> pcb->array[pcb->idx].tick_next)
			pcb->state = EM_TASK_READ_POSTED;
		return (!OK);
	}
	else if (EM_TASK_READ_POSTED == pcb->state)
	{
		pcb->state = EM_TASK_READ_EXIT;
		return (!OK);
	}
	else if (EM_TASK_READ_EXIT == pcb->state)
	{
		P_READ_TASK_INFO pReadTask = pcb->array[pcb->idx].pReadTask;
		PLC_delete_read_task_gd(pReadTask->task_type, pReadTask->task_header.task_id);
		free_buffer(__func__,pcb->array[pcb->idx].pPlcRsp);
		pcb->array[pcb->idx].pPlcRsp = NULL;
		free_buffer(__func__,pcb->array[pcb->idx].pReadTask);
		pcb->array[pcb->idx].pReadTask = NULL;
		pcb->state = EM_TASK_READ_INIT;
		return OK;
	}

	return OK;
}

static u16 find_new_multi_sta(multi_sta_head *p_sta, u16 *sn)
{
	for (int i=0;i < sizeof(p_sta->bitmap)/4;i++)
	{
		if (p_sta->bitmap[i]!=0)
		{
			for (int j=0;j<32;j++)
			{
				if (p_sta->bitmap[i]&(1<<j))
				{
					*sn=(i<<5)|j;
					u16 tei=0;
					if(g_MeterRelayInfo[*sn].status.used==CCO_USERD)
					{
						u8 datmac[LONG_ADDR_LEN];
						PLC_GetAppDstMac(g_MeterRelayInfo[*sn].meter_addr,datmac);
						tei=PLC_GetStaSnByMac(datmac);
					}
					if (tei < TEI_STA_MIN||tei>TEI_STA_MAX)
					{
						p_sta->bitmap[i]&=~(1<<j);
						continue ;
					}
					int x;
					for (x=0;x<MAX_MULTI_STA;x++)
					{
						if (p_sta->sta[x].tei==tei)
						{
							break;
						}
					}
					if (x>=MAX_MULTI_STA)
					{
						return tei;
					}
				}
				
			}
		}
	}
	return 0;
}

void PLC_DeleteMultiSta(multi_sta_head *p_sta, u16 tei, u16 sn)
{
#if 1
	if (sn<CCTT_METER_NUMBER)
	{
		p_sta->bitmap[sn>>5] &= ~(1<<(sn&0x1f));
	}
#else
	u16 sn=PLC_GetWhiteNodeSnByStaInfo(PLC_GetValidStaInfoByTei(tei));
	if (sn<CCTT_METER_NUMBER)
	{
		p_sta->bitmap[sn>>5]&=~(1<<(sn&0x1f));
	}
#endif

	for (int x = 0; x < MAX_MULTI_STA; x++)
	{
		if (p_sta->sta[x].tei == tei)
		{
			p_sta->sta[x].tei=0;
			p_sta->sta[x].send_time=0;
			p_sta->sta[x].timeout=0;
			break;
		}
	}
}

static void init_multi_sta(multi_sta_head *p_sta, u16 tei, u16 sn)
{
	for (int x = 0; x < MAX_MULTI_STA; x++)
	{
		if (p_sta->sta[x].tei == 0)
		{
			p_sta->sta[x].tei=tei;
			p_sta->sta[x].send_time = 0;
			p_sta->sta[x].timeout = 0;
			p_sta->sta[x].sn = sn;
			break;
		}
	}
}

static u16 get_multi_num(multi_sta_head *p_sta)
{
	u16 num=0;
	for (int x = 0; x < MAX_MULTI_STA; x++)
	{
		if (p_sta->sta[x].tei != 0)
		{
			++num;
		}
	}
	return num;
}

u16 get_multi_sta_sn(multi_sta_head *p_sta, u16 tei)
{
	for (int x = 0; x < MAX_MULTI_STA; x++)
	{
		if (p_sta->sta[x].tei == tei)
		{
			return p_sta->sta[x].sn;
		}
	}

	return 0xFFFF;
}

//南网多播抄表
u8 PLC_RouteReadMultiMeterProc(OS_TICK_64 tick_now)
{
	P_PLC_TASK_MultiREAD_CB pcb = &g_pRelayStatus->task_multi_read_cb;
	multi_sta_head *p_sta;
	if (EM_TASK_READ_INIT == pcb->state)
	{
		if (g_pRelayStatus->pause_sec)
			return OK;

		PLC_fetch_task_read_item_gd(&pcb->pReadTask,true,NULL,NULL, MAX_NW_PARALLEL_NUM);
		if (NULL == pcb->pReadTask)
			return OK;

		pcb->plc_send_counter = 3;
		pcb->state = EM_TASK_READ_FIND_STA;
		pcb->get_cnt=0;
		pcb->total_cnt=0;
		p_sta=pcb->pReadTask->p_sta;

		//多播任务时，每块表的帧序号固定，packet_seq_start+sn
		pcb->packet_seq_plc = ++g_pRelayStatus->packet_seq_read_meter;
		g_pRelayStatus->packet_seq_read_meter += CCTT_METER_NUMBER; 

		for (int i=0;i < sizeof(p_sta->bitmap)/4;i++)
		{
			if (p_sta->bitmap[i])
			{
				for (int j=0;j<32;j++)
				{
					if (p_sta->bitmap[i]&(1<<j))
					{
						++pcb->total_cnt;
					}
				}
			}
		}

		return (OK);
	}
	else if (EM_TASK_READ_FIND_STA == pcb->state)
	{
		p_sta=pcb->pReadTask->p_sta;
		P_READ_TASK_INFO pReadTask = pcb->pReadTask;

		if (NULL == PLC_find_read_task_gd(pReadTask->task_type, pReadTask->task_header.task_id))
		{
			pcb->state = EM_TASK_READ_EXIT;
			return !OK;
		}
		if(get_multi_num(p_sta)<MAX_MULTI_STA)
		{
			u16 sn = 0;
			u16 tei = find_new_multi_sta(p_sta, &sn);
			if (tei)
			{
				init_multi_sta(p_sta, tei, sn);
			}
		}
		for (int i=0;i<MAX_MULTI_STA;i++)
		{
			if (p_sta->sta[i].tei)
			{
				if (tick_now >p_sta->sta[i].timeout)
				{
					if (pcb->plc_send_counter * MAX_MINOR_LOOP_READ_METER > p_sta->sta[i].send_time)
					{
						pcb->state = EM_TASK_READ_SEND;
						pcb->send_idx=i;
						return (OK);
					}
					PLC_DeleteMultiSta(p_sta, p_sta->sta[i].tei, p_sta->sta[i].sn);
				}
			}
		}
		if (mem32IsHex((u32 *)p_sta->bitmap,0,sizeof(p_sta->bitmap)))//全部发送完成
		{
			if (!pReadTask->task_header.task_mode.resp_flag)//只上报任务状态
			{
				pcb->state = EM_TASK_READ_POST;//尚有为发送完成的sta上报超时
				return OK;
			}
			if ((pcb->get_cnt >= pcb->total_cnt)
				&&pcb->total_cnt)
			{
				pcb->state = EM_TASK_READ_EXIT;//全部的节点都成功直接退出
			}
			else
			{
				pcb->state = EM_TASK_READ_POST;//尚有为发送完成的sta上报超时
			}
			return OK;
		}
		return OK;
	}
	else if (EM_TASK_READ_SEND == pcb->state)
	{
		P_READ_TASK_INFO pReadTask = pcb->pReadTask;
		p_sta=pcb->pReadTask->p_sta;
		u8 send_flag = (!OK);
		BOOL broadcast;
		HPLC_SEND_PHASE phase;
		if ((pcb->plc_send_counter - 1) * MAX_MINOR_LOOP_READ_METER <  p_sta->sta[pcb->send_idx].send_time)
		{
			broadcast = true;
		}
		else
		{
			broadcast = false;
		}
		if ((p_sta->sta[pcb->send_idx].send_time % MAX_MINOR_LOOP_READ_METER) == (MAX_MINOR_LOOP_READ_METER - 1))
		{
			phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
		}
		else
		{
			phase = SEND_PHASE_UNKNOW;
		}
		
		P_STA_INFO p_sta_info=PLC_GetValidStaInfoByTei(p_sta->sta[pcb->send_idx].tei);
		u8 mac_addr[MAC_ADDR_LEN];
		if (p_sta_info!=NULL)
		{
			memcpy(mac_addr, p_sta_info->mac, MAC_ADDR_LEN);

			P_GD_DL645_BODY pFrame = NULL;
			pFrame = PLC_check_dl645_frame(pReadTask->task_buffer, pReadTask->task_header.task_len);
			if(pFrame)
			{
				P_RELAY_INFO p_relay_info = PLC_GetValidNodeInfoBySn(p_sta->sta[pcb->send_idx].sn);
				if (p_relay_info == NULL)
				{
					return OK;
				}
				
				memcpy(pFrame->meter_number, p_relay_info->meter_addr, MAC_ADDR_LEN);
				pFrame->dataBuf[pFrame->dataLen] = Get_checksum(&pFrame->protocolHead, pFrame->dataLen+10);				
			}
		}
		else
		{
			memset(mac_addr,0xff,MAC_ADDR_LEN);

			P_GD_DL645_BODY pFrame = NULL;
			pFrame = PLC_check_dl645_frame(pReadTask->task_buffer, pReadTask->task_header.task_len);
			if(pFrame)
			{
				memset(pFrame->meter_number, 0x99, MAC_ADDR_LEN);
				pFrame->dataBuf[pFrame->dataLen] = Get_checksum(&pFrame->protocolHead, pFrame->dataLen+10);
			}
		}

		//多播任务时，每块表的帧序号固定，packet_seq_start+sn
		u16 packet_seq = pcb->packet_seq_plc + p_sta->sta[pcb->send_idx].sn;
		send_flag = PLC_SendDataTrans(broadcast, pReadTask->meter_moudle,phase, packet_seq, mac_addr, pReadTask->task_buffer, pReadTask->task_header.task_len);
		if (OK != send_flag)
		{
			pcb->state = EM_TASK_READ_FIND_STA;
			return OK;
		}

		p_sta->sta[pcb->send_idx].timeout = PLC_GetReadMeterTimeoutTick(pReadTask->task_header.task_len)+tick_now;
		p_sta->sta[pcb->send_idx].send_time++;
		pcb->state = EM_TASK_READ_FIND_STA;
		return (OK);
	}

	else if (EM_TASK_READ_POST == pcb->state)
	{
		P_READ_TASK_INFO pReadTask = pcb->pReadTask;

		if (NULL != PLC_find_read_task_gd(pReadTask->task_type, pReadTask->task_header.task_id))
		{
			if ((pcb->get_cnt >= pcb->total_cnt)
				&&pcb->total_cnt)
			{
				PLM_ReportTaskStatus_gd(COM_PORT_MAINTAIN,pReadTask->task_header.task_id, NULL, 0x01);
			}
			else
			{
				PLM_ReportTaskStatus_gd(COM_PORT_MAINTAIN,pReadTask->task_header.task_id, NULL, 0x00);
			}
			pcb->tick_post = tick_now;
			pcb->state = EM_TASK_READ_WAIT_POST;
		}
		else
		{
			pcb->state = EM_TASK_READ_EXIT;
		}
		return (OK);
	}
	else if (EM_TASK_READ_WAIT_POST == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->tick_post) >= OSCfg_TickRate_Hz / 5)
			pcb->state = EM_TASK_READ_POSTED;
		return (OK);
	}
	else if (EM_TASK_READ_POSTED == pcb->state)
	{
		pcb->state = EM_TASK_READ_EXIT;
		return (!OK);
	}
	else if (EM_TASK_READ_EXIT == pcb->state)
	{
		p_sta=pcb->pReadTask->p_sta;
		P_READ_TASK_INFO pReadTask = pcb->pReadTask;
		PLC_delete_read_task_gd(pReadTask->task_type, pReadTask->task_header.task_id);
		free_buffer(__func__,pcb->pReadTask);
		pcb->state = EM_TASK_READ_INIT;
		return OK;
	}

	return OK;
}

extern const u8 element_len_tbl[];
//网络维护处理
static u8 PLC_MaintenanceNetProc(P_PLC_MAINTENANCE_NET_CB pcb, OS_TICK_64 tick_now)
{
//	P_PLC_MAINTENANCE_NET_CB pcb = &g_pRelayStatus->maintenance_net_cb;

	if (EM_TASK_READ_INIT == pcb->state)
	{
		if (EM_TASK_TYPE_INFO_REALTIME_TASK == pcb->task_type)
		{
			pcb->plc_send_counter = 1;
		}
		else
		{
			pcb->plc_send_counter = 3;
		}
		pcb->plc_send_num = 0;
		return (OK);
	}
	else if (EM_TASK_READ_SEND == pcb->state)
	{
		u8 send_flag = (!OK);
		BOOL broadcast = false;
		HPLC_SEND_PHASE phase;

		if ((EM_TASK_TYPE_STATUS_TASK == pcb->task_type)||(EM_TASK_TYPE_INFO_TASK == pcb->task_type))
		{
			if ((pcb->plc_send_counter - 1) * MAX_MINOR_LOOP_READ_METER < pcb->plc_send_num)
			{
				//broadcast = true;
				broadcast = false;
			}
			else
			{
				broadcast = false;
			}
		}

		if ((pcb->plc_send_num % MAX_MINOR_LOOP_READ_METER) == (MAX_MINOR_LOOP_READ_METER - 1))
		{
			phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
		}
		else
		{
			phase = SEND_PHASE_UNKNOW;
		}
		if (EM_TASK_TYPE_STATUS_TASK == pcb->task_type)
		{
			send_flag = PLC_SendGetStaStatus(broadcast, pcb->mac_addr, pcb->element, phase);
		}
		else if ((EM_TASK_TYPE_INFO_TASK == pcb->task_type) || (EM_TASK_TYPE_INFO_REALTIME_TASK == pcb->task_type))
		{
			send_flag = PLC_SendGetStaInfo(broadcast, pcb->mac_addr, pcb->element_num, pcb->element_array, phase);
		}
		else if ((EM_TASK_TYPE_CHANNEL_TASK == pcb->task_type) || (EM_TASK_TYPE_NEW_CHANNEL_TASK == pcb->task_type))
		{
			send_flag = PLC_SendGetChannelInfo(broadcast, pcb->mac_addr, pcb->start_sn, pcb->num, phase);
		}
		else
		{
			pcb->state = EM_TASK_READ_INIT;
			return OK;
		}
		if (send_flag != OK)
		{

		}
		pcb->tick_send = tick_now;
		pcb->tick_timeout = PLC_GetReadMeterTimeoutTick(0);
		pcb->packet_seq_plc = g_pRelayStatus->packet_seq_read_meter;
		pcb->plc_send_num++;
		pcb->state = EM_TASK_READ_WAIT_PLC;

		return (!OK);
	}
	else if (EM_TASK_READ_WAIT_PLC == pcb->state)
	{
	    if (EM_TASK_TYPE_INFO_TASK == pcb->task_type)
        {
            u8 dst_addr[LONG_ADDR_LEN] = {0};
			PLC_GetAppDstMac(pcb->mac_addr, dst_addr);		
            P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
            if (pSta && pSta->geted_id)
            {
                pcb->state = EM_TASK_READ_POST;
    			return (OK);
            }
        }
        
		if (NULL != pcb->pPlcRsp)
		{
			pcb->state = EM_TASK_READ_POST;
			return (!OK);
		}
		if (TicksBetween64(tick_now, pcb->tick_send) >= pcb->tick_timeout)
		{
			if (pcb->plc_send_counter * MAX_MINOR_LOOP_READ_METER > pcb->plc_send_num)
			{
				pcb->state = EM_TASK_READ_SEND;
			}
			else
			{
				pcb->state = EM_TASK_READ_POST;
			}
		}

		return (OK);
	}
	else if (EM_TASK_READ_POST == pcb->state)
	{
		P_PLC_RSP_MSG_INFO  pPlcRsp = pcb->pPlcRsp;
		if (EM_TASK_TYPE_STATUS_TASK == pcb->task_type)
		{
			#ifdef FUNC_HPLC_READER
			if (NULL != pPlcRsp)
			{
				PLM_ReportStaStatus_gd(pPlcRsp->body, pPlcRsp->header.msg_len);
			}
			else
			{

				PLM_ReportStaStatus_gd(NULL, 0);
			}
			#endif
		}
		else if (EM_TASK_TYPE_CHANNEL_TASK == pcb->task_type)
		{
			if (NULL != pPlcRsp)
			{
				PLM_ReportChannelInfo_gd(pPlcRsp->body, pPlcRsp->header.msg_len);
			}
			else
			{
				PLM_ReportChannelInfo_gd(NULL, 0);
			}
		}
        else if (EM_TASK_TYPE_NEW_CHANNEL_TASK == pcb->task_type)
		{
			if (NULL != pPlcRsp)
			{
				PLM_ReportChannelNewInfo_gd(pPlcRsp->body, pPlcRsp->header.msg_len);
			}
			else
			{
				PLM_ReportChannelNewInfo_gd(NULL, 0);
			}
		}
		else if (EM_TASK_TYPE_INFO_REALTIME_TASK == pcb->task_type)
		{
#ifdef PROTOCOL_NW_2021		
			if (NULL != pPlcRsp)
			{	
				PLM_ReadStaModuleInfoResponse_gd(pPlcRsp->body, pPlcRsp->header.msg_len);
			}
#if 1
            //实时查询资产信息超时回复，规避华立集中器查询不到资产信息不执行日冻结的问题（现场有老模块不支持载波查询从节点信息）	
            else
            {
				u8 dst_addr[LONG_ADDR_LEN] = {0};
				PLC_GetAppDstMac(pcb->mac_addr, dst_addr);
				P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
				if (pSta && pSta->geted_id==0)
				{
				    if (g_PlmConfigPara.element_report0_mode)
                    {
                        debug_str(DEBUG_LOG_NET, "PLM_ReadStaModuleInfoResponse autoload all zero, mac:%02x-%02x-%02x-%02x-%02x-%02x\r\n", 
			                  pcb->mac_addr[5], pcb->mac_addr[4], pcb->mac_addr[3], pcb->mac_addr[2], pcb->mac_addr[1], pcb->mac_addr[0]);
					
    					u16 alloc_len = pcb->start_sn + 6; //6字节节点地址
    					u16 index = 0;
    					u8 element_id = 0;
    					u8* report_data = alloc_buffer(__func__, alloc_len);
    					if (report_data)
    					{
    						memcpy_special(&report_data[index], pcb->mac_addr, 6);
    						index += 6;

    						for (int i=0; i<pcb->element_num; i++)
    						{
    							element_id = pcb->element_array[i];
    							if (pcb->element & (1 << element_id))
    							{
    								report_data[index++] = element_id;
    								report_data[index++] = 0; //长度
    							}
    						}
    						
    						PLM_ReadStaModuleInfoResponse_gd(report_data, index);
    						free_buffer(__func__, report_data);
                       }
					}
				}
			}
#else
			else //载波没获取到资产信息
			{
				u8 dst_addr[LONG_ADDR_LEN] = {0};
				PLC_GetAppDstMac(pcb->mac_addr, dst_addr);
				P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
				if (pSta && pSta->geted_id && (pcb->element & 0x163)) //周期性获取到过资产信息信息，并且和本地接口查询的资产信息有交集
				{
					debug_str(DEBUG_LOG_NET, "PLM_ReadStaModuleInfoResponse autoload, mac:%02x-%02x-%02x-%02x-%02x-%02x\r\n", 
			                  pcb->mac_addr[5], pcb->mac_addr[4], pcb->mac_addr[3], pcb->mac_addr[2], pcb->mac_addr[1], pcb->mac_addr[0]);
					
					u16 alloc_len = pcb->start_sn + 6; //6字节节点地址
					u16 index = 0;
					u8 element_len = 0;
					u8 element_id = 0;
					u8* report_data = alloc_buffer(__func__, alloc_len);
					if (report_data)
					{
						memcpy_special(&report_data[index], pcb->mac_addr, 6);
						index += 6;

#if 1 //新算法信息元素ID不重新排序，按本地接口查询顺序
						for (int i=0; i<pcb->element_num; i++)
						{
							element_id = pcb->element_array[i];
							if (pcb->element & (1 << element_id))
							{
								element_len = element_len_tbl[element_id];
								memset_special(&report_data[index], 0x00, element_len);

								report_data[index++] = element_id;
								report_data[index++] = element_len - 2;

								if (element_id == 0) //厂商编号
								{
									memcpy_swap(&report_data[index], pSta->ver_factory_code, 2);					
								}
								else if(element_id == 1) //软件版本信息（模块）
								{
									memcpy_swap(&report_data[index], pSta->software_v, 2);
								}
								else if(element_id == 5) //芯片厂商代码
								{
									memcpy_swap(&report_data[index], pSta->ver_chip_code, 2);
								}
								else if(element_id == 6) //固件发布日期（模块）
								{
									report_data[index] = Hex2BcdChar(pSta->year_mon_data.Day);
									report_data[index+1] = Hex2BcdChar(pSta->year_mon_data.Month);
									report_data[index+2] = Hex2BcdChar(pSta->year_mon_data.Year);
								}
								else if(element_id == 8) //模块出厂MAC地址
								{
									memcpy_special(&report_data[index], pSta->module_id, 6);
								}

								index += (element_len - 2);
							}
						}
#else //老算法按元素ID按值大小重新排序
						for (int i=0; i<=16; i++)
						{
							if (pcb->element & (1 << i))
							{
								element_len = element_len_tbl[i];
								memset_special(&report_data[index], 0x00, element_len);

								report_data[index++] = i;
								report_data[index++] = element_len - 2;

								if (i == 0) //厂商编号
								{
									memcpy_swap(&report_data[index], pSta->ver_factory_code, 2);					
								}
								else if(i == 1) //软件版本信息（模块）
								{
									memcpy_swap(&report_data[index], pSta->software_v, 2);
								}
								else if(i == 5) //芯片厂商代码
								{
									memcpy_swap(&report_data[index], pSta->ver_chip_code, 2);
								}
								else if(i == 6) //固件发布日期（模块）
								{
									report_data[index] = Hex2BcdChar(pSta->year_mon_data.Day);
									report_data[index+1] = Hex2BcdChar(pSta->year_mon_data.Month);
									report_data[index+2] = Hex2BcdChar(pSta->year_mon_data.Year);
								}
								else if(i == 8) //模块出厂MAC地址
								{
									memcpy_special(&report_data[index], pSta->module_id, 6);
								}

								index += (element_len - 2);
							}
						}
#endif
						
						PLM_ReadStaModuleInfoResponse_gd(report_data, index);
						free_buffer(__func__, report_data);
					}
				}
			}
#endif
			pcb->report_sn = 0;

            pcb->state = EM_TASK_READ_POSTED;
            return (!OK);
#endif	
		}
        pcb->tick_post = tick_now;
	    pcb->state = EM_TASK_READ_WAIT_POST;
        
		return (!OK);
	}
	else if (EM_TASK_READ_WAIT_POST == pcb->state)
	{        
		if (TicksBetween64(tick_now, pcb->tick_post) >= OSCfg_TickRate_Hz / 5)
			pcb->state = EM_TASK_READ_POSTED;
		return (OK);
	}
	else if (EM_TASK_READ_POSTED == pcb->state)
	{
		pcb->state = EM_TASK_READ_EXIT;
		return (!OK);
	}
	else if (EM_TASK_READ_EXIT == pcb->state)
	{
		free_buffer(__func__,pcb->pPlcRsp);
		pcb->pPlcRsp = NULL;
		pcb->state = EM_TASK_READ_INIT;
		return OK;
	}

	return OK;
}
u8 PLC_StaStatusProc(OS_TICK_64 tick_now)
{
	static u16 tei = 0;
	static OS_TICK_64 procTick = 0;
	P_PLC_MAINTENANCE_NET_CB pcb = &g_pRelayStatus->sta_status_cb;
	if ((g_pRelayStatus->check_online * 10 / g_pRelayStatus->cfged_meter_num) < 9) //超过90%才开始统计
	{
		if (GetTickCount64()<30*60*1000)
		{
			return OK;
		}
	}
	if (procTick == 0 || (procTick != 0 && procTick < tick_now))
	{
		if (pcb->state == EM_TASK_READ_INIT)
		{
			pcb->plc_send_counter = 3;
			pcb->plc_send_num = 0;

			for (; tei < TEI_MAX; tei++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (pSta != NULL && pSta->net_state==NET_IN_NET)
				{
					if (pSta->run_status.runtime == 0)
					{
						pcb->task_type = EM_TASK_TYPE_STATUS_TASK;
						pcb->state = EM_TASK_READ_SEND;
						pcb->element = 0x1f;
						memcpy(pcb->mac_addr, pSta->mac, MAC_ADDR_LEN);
						break;
					}
				}
			}
			if (tei == TEI_MAX)
			{
				tei = TEI_STA_MIN;
				procTick = tick_now + OS_CFG_TICK_RATE_HZ * 60 * 10; //达到最大后等待10分钟再进入
				return OK;
			}
			tei++; /*发生break后才会走到这一步，下次进入PLC_StaStatusProc时从tei+1开始查询;
					 直接在break前tei++，会导致最后一个有效节点(1014)进入tei==TEI_MAX逻辑*/
			procTick = 0; //
		}
		return PLC_MaintenanceNetProc(pcb, tick_now);
	}
	else
	{
		return OK;
	}
}
u8 PLC_StaInfoProc(OS_TICK_64 tick_now)
{
	static u16 tei = 0;
	static OS_TICK_64 procTick = 0;
	P_PLC_MAINTENANCE_NET_CB pcb = &g_pRelayStatus->sta_info_cb;
	if ((g_pRelayStatus->check_online * 10 / g_pRelayStatus->cfged_meter_num) < 9) //超过90%才开始统计
	{
		if (GetTickCount64()<30*60*1000)
		{
			return OK;
		}
	}
	if (procTick == 0 || (procTick != 0 && procTick < tick_now))
	{
		if (pcb->state == EM_TASK_READ_INIT)
		{
			pcb->plc_send_counter = 3;
			pcb->plc_send_num = 0;
			u8 element_array[0x10+1] = {0};
			u8 element_num = 0;

			for (; tei < TEI_MAX; tei++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (pSta != NULL && pSta->net_state==NET_IN_NET)
				{
					if (pSta->geted_id == 0)
					{
						pcb->task_type = EM_TASK_TYPE_INFO_TASK;
						pcb->state = EM_TASK_READ_SEND;
						pcb->element = 0x1ff67;

						for (int i=0; i<=0x10; i++)
						{
							if (pcb->element & (1 << i))
							{
								element_array[element_num] = i;
								element_num++;
							}
						}
						pcb->element_num = element_num;
						memcpy(pcb->element_array, element_array, sizeof(pcb->element_array));
						memcpy(pcb->mac_addr, pSta->mac, MAC_ADDR_LEN);
						break;
					}
				}
			}
			if (tei == TEI_MAX)
			{
				tei = TEI_STA_MIN;
				procTick = tick_now + OS_CFG_TICK_RATE_HZ * 60 * 10; //达到最大后等待10分钟再进入
				return OK;
			}
			tei++; /*发生break后才会走到这一步，下次进入PLC_StaStatusProc时从tei+1开始查询;
					 直接在break前tei++，会导致最后一个有效节点(1014)进入tei==TEI_MAX逻辑*/
			procTick = 0; //
		}
		return PLC_MaintenanceNetProc(pcb, tick_now);
	}
	else
	{
		return OK;
	}
}

u8 PLC_StaInfoRealTimeProc(OS_TICK_64 tick_now)
{
	return PLC_MaintenanceNetProc(&g_pRelayStatus->sta_info_realtime_cb, tick_now);
}

u8 PLC_StaChannelInfoProc(OS_TICK_64 tick_now)
{
	return PLC_MaintenanceNetProc(&g_pRelayStatus->sta_channel_cb, tick_now);
}

u8 PLC_StaChannelNewInfoProc(OS_TICK_64 tick_now)
{
	return PLC_MaintenanceNetProc(&g_pRelayStatus->sta_new_channel_cb, tick_now);
}

#else

u8 PLC_RouteReadMeterProc(OS_TICK_64 tick_now)
{
	//考虑切表机制、超时机制
	//切表条件：路由抄表失败 || 集中器应答抄表完成 || 集中器答应抄表失败

#define CON_TRY_REQUEST_COUNT               3
#define CON_READ_COUNT_TOTAL                128                     //一只表连续抄读成功次数
#define CON_READ_COUNT_FAILED               CON_PLC_RESEND_COUNT    //连续抄失败次数，切下一块表


	P_PLC_ROUTE_READ_CB pcb = &g_pRelayStatus->route_read_cb;
    static u8 first_rep=1;
	if (EM_ROUTE_READ_INIT == pcb->state)
	{
		if (0 != g_pRelayStatus->pause_sec)
			return OK;

		if (FALSE == is_queue_empty(g_PlcWaitRxQueue))
			return OK;

		u16 sn = pcb->sn;
		
		first_rep=0;
		P_STA_INFO pSta = NULL;
		for (u16 i = 0; i < CCTT_METER_NUMBER; i++, sn++)
		{
			if (sn >= CCTT_METER_NUMBER)
				sn = 0;
			if (g_MeterRelayInfo[sn].status.used != CCO_USERD)
			{
				continue;
			}
			if (g_MeterRelayInfo[sn].freeze_done == 1)
				continue;
			u8 dst_addr[LONG_ADDR_LEN];
			PLC_GetAppDstMac(g_MeterRelayInfo[sn].meter_addr,dst_addr);
			pSta = PLC_GetStaInfoByMac(dst_addr);
			if (NULL == pSta)
				continue;
			break;
		}

		if (NULL == pSta)
			return OK;

		pcb->state = EM_ROUTE_READ_REQUEST;
		pcb->sn = sn;

		pcb->plmCmd.plm_phase = (u8)PLC_HplcPhase_to_PlmPhase((HPLC_PHASE)pSta->hplc_phase);
		memcpy_special(pcb->plmCmd.node_addr, g_MeterRelayInfo[sn].meter_addr, FRM_DL645_ADDRESS_LEN);

		P_RELAY_INFO pNode = &g_MeterRelayInfo[sn];

		if (NULL != pNode)
		{
			pcb->plmCmd.user_sn = pNode->user_sn;
			pcb->protocol_plc = PLC_meterprotocol_to_plcprotocol(pNode->meter_type.protocol);
		}
		else
		{
			pcb->plmCmd.user_sn = (u16)0;
			pcb->protocol_plc = APP_PRO_TRANSPARENT;
		}

		pcb->req_counter = CON_TRY_REQUEST_COUNT;
		pcb->tick_post = tick_now;
		
		return (OK);
	}
	else if (EM_ROUTE_READ_REQUEST == pcb->state)
	{
		free_buffer(__func__,pcb->pPlmRsp);
		pcb->pPlmRsp = NULL;

		PLM_RequestReadItem(&pcb->plmCmd);
		pcb->tick_last_req = tick_now;
		if (pcb->req_counter)
			pcb->req_counter--;
		pcb->state = EM_ROUTE_READ_WAIT_PLM;
		return (OK);
	}
	else if (EM_ROUTE_READ_WAIT_PLM == pcb->state)
	{
		if (pcb->pPlmRsp)
		{
			//连续3次请求到的数据一样，切表
//			static u8 same_counter = 0;
//			P_PLM_READ_ITEM_RSP pThis = pcb->pPlmRsp;
//			P_PLM_READ_ITEM_RSP pLast = pcb->pPlmRspLast;
//			if ((NULL != pLast)
//				&& (pThis->len > 0)
//				&& (pThis->len == pLast->len)
//				&& (0 == memcmp(pThis->data, pLast->data, pThis->len))
//			   )
//			{
//				same_counter++;
//				if (same_counter >= 3)
//					goto L_NEXT_METER;
//			}
//			else
//			{
//				same_counter = 0;
//				u16 len = sizeof(PLM_READ_ITEM_RSP) - 1 + pThis->len;
//				free_buffer(__func__,pcb->pPlmRspLast);
//				pcb->pPlmRspLast = (P_PLM_READ_ITEM_RSP)alloc_buffer(__func__,len);
//				if (NULL != pcb->pPlmRspLast)
//				{
//					memcpy_special(pcb->pPlmRspLast, pThis, len);
//				}
//			}
			if (first_rep&&(READ_ITEM_FINISH == pcb->pPlmRsp->read_flag))
			{
//				free_buffer(__func__,pcb->pPlmRspLast);
				free_buffer(__func__,pcb->pPlmRsp);
//				pcb->pPlmRspLast=NULL;
				pcb->pPlmRsp=NULL;
				pcb->state = EM_ROUTE_READ_WAIT_PLM;
				pcb->tick_last_req = tick_now;
				first_rep=0;
				return (OK);
			}
			else
			{
				//抄读
				pcb->state = EM_ROUTE_READ_SEND;
				pcb->plc_send_counter = CON_PLC_RESEND_COUNT;
			}
		}
		else if (TicksBetween64(tick_now, pcb->tick_last_req) >= 10 * OSCfg_TickRate_Hz)
		{
			if (g_pRelayStatus->pause_sec)
			{
				return OK;
			}
			if (pcb->req_counter)
				pcb->state = EM_ROUTE_READ_REQUEST;
			else
				goto L_NEXT_METER;
		}
		return (OK);
	}
	else if (EM_ROUTE_READ_FIXTIME_REQUEST == pcb->state)
	{
		P_PLM_READ_ITEM_FIX_TIME_RSP p_rep = (P_PLM_READ_ITEM_FIX_TIME_RSP)alloc_buffer(__func__,sizeof(PLM_READ_ITEM_FIX_TIME_RSP) + pcb->pPlmRsp->len);
		if (p_rep == NULL)
		{
			goto L_NEXT_METER;
		}
		memcpy(p_rep->node_addr, pcb->pPlmRsp->node_addr, MAC_ADDR_LEN);
		memcpy(p_rep->data, pcb->pPlmRsp->data, pcb->pPlmRsp->len);
		p_rep->len = pcb->pPlmRsp->len;
		p_rep->delay_seconds = 1; //暂时固定为1
		PLM_RequestReadItem2FixTime((PLM_PHASE)pcb->plmCmd.plm_phase, (u8 *)p_rep, sizeof(PLM_READ_ITEM_FIX_TIME_RSP) + p_rep->len);
		free_buffer(__func__,p_rep);
		pcb->tick_last_req = tick_now;
		pcb->state = EM_ROUTE_READ_FIXTIME_WAIT_PLM;
		return (OK);
	}
	else if (EM_ROUTE_READ_FIXTIME_WAIT_PLM == pcb->state)
	{
		OS_MSG_SIZE msgSize;
		OS_ERR err;
		P_MSG_INFO p_CccttSendMsg = (P_MSG_INFO)OSQPend(&g_pOsRes->q_up_rx_plc, 0, OS_OPT_PEND_NON_BLOCKING, &msgSize, NULL, &err);
		if (err == OS_ERR_NONE)
		{
			P_PLC_READ_ITEM_RSP_GDW2009 pReadIfo = (P_PLC_READ_ITEM_RSP_GDW2009)p_CccttSendMsg->msg_buffer;
			if (pReadIfo->len != 0)
			{
				pcb->state = EM_ROUTE_READ_SEND;
				pcb->plc_send_counter = CON_PLC_RESEND_COUNT;
				if (pcb->pPlmRsp->len != pReadIfo->len) //得到的并非上次申请修改时间的帧
				{
					free_send_buffer(__func__,p_CccttSendMsg);
					goto L_NEXT_METER;
				}
				pcb->pPlmRsp->read_flag = READ_ITEM_READABLE;
				pcb->pPlmRsp->time_related = 0;
				memcpy(pcb->pPlmRsp->data, pReadIfo->data, pcb->pPlmRsp->len);
				free_send_buffer(__func__,p_CccttSendMsg);
			}
			else //len =0放弃本次发送
			{
				free_send_buffer(__func__,p_CccttSendMsg);
				goto L_NEXT_METER;
			}
		}
		else if (TicksBetween64(tick_now, pcb->tick_last_req) >= 3 * OSCfg_TickRate_Hz)
		{
			if (g_pRelayStatus->pause_sec)
			{
				return OK;
			}
			if (pcb->req_counter)
				pcb->state = EM_ROUTE_READ_REQUEST;
			else
				goto L_NEXT_METER;
		}
		return (OK);
	}
	else if (EM_ROUTE_READ_SEND == pcb->state)
	{
		P_PLM_READ_ITEM_RSP pPlmRsp = pcb->pPlmRsp;
		if (pPlmRsp)
		{
			if (READ_ITEM_FINISH == pPlmRsp->read_flag || READ_ITEM_FAILURE == pPlmRsp->read_flag)
			{
				if (READ_ITEM_FAILURE == pPlmRsp->read_flag)
				{
					pcb->state=EM_ROUTE_WAIT;
					pcb->tick_last_req = tick_now;
					return OK;
				}
				if (READ_ITEM_FINISH == pPlmRsp->read_flag)
				{
					if (g_MeterRelayInfo[pcb->sn].freeze_done ==0)
					{
						g_MeterRelayInfo[pcb->sn].freeze_done=1;
					}
				}
				goto L_NEXT_METER;
			}
			if (pPlmRsp->time_related == 1) //需要请求修改时间
			{
				void *p_qmsg;
				OS_MSG_SIZE msgSize;
				OS_ERR err;
				while (p_qmsg = OSQPend(&g_pOsRes->q_up_rx_plc, 0, OS_OPT_PEND_NON_BLOCKING, &msgSize, NULL, &err))
				{
					free_send_buffer(__func__,p_qmsg);
				}
				pcb->state = EM_ROUTE_READ_FIXTIME_REQUEST;
				return (OK);
			}

			//plc send
			if (OK != PLC_SendRouteRead(pcb,pcb->plc_send_counter == CON_PLC_RESEND_COUNT))
				goto L_NEXT_METER;

			pcb->tick_timeout = tick_now+PLC_GetReadMeterTimeoutTick(0);
			pcb->packet_seq_plc = g_pRelayStatus->packet_seq_read_meter;

			pcb->state = EM_ROUTE_READ_WAIT_PLC;
			if (pcb->plc_send_counter)
				pcb->plc_send_counter--;
		}
		else
		{
			goto L_NEXT_METER;
		}
		return (OK);
	}
	else if (EM_ROUTE_WAIT == pcb->state)
	{
		if(TicksBetween64(tick_now, pcb->tick_last_req) >= 200)
		{
			goto L_NEXT_METER;
		}
		return (OK);
	}
	else if (EM_ROUTE_READ_WAIT_PLC == pcb->state)
	{
		if (NULL != pcb->pPlcRsp)
		{
			//pcb->packet_seq_plc = pcb->packet_first_seq_plc;
			pcb->plc_read_success_counter++;
			pcb->state = EM_ROUTE_READ_POST;
			if(g_pRelayStatus->pause_sec)
			{
				pcb->state = EM_ROUTE_READ_POSTED;
			}
			return (OK);
		}
		if (tick_now > pcb->tick_timeout)
		{
			if (pcb->plc_send_counter)
				pcb->state = EM_ROUTE_READ_SEND;
			else
				goto L_NEXT_METER;
		}
		return (OK);
	}
	else if (EM_ROUTE_READ_POST == pcb->state)
	{
		OS_ERR err;
		u8 plm_protocol = PLC_plcprotocol_to_3762protocol(pcb->protocol_plc);
		u8 took_time_sec = (u8)(TicksBetween64(tick_now, pcb->tick_timeout-PLC_GetReadMeterTimeoutTick(0)) / OSCfg_TickRate_Hz);
		PLM_ReportRouteRead(pcb->pPlmRsp->time_related==0xff,pcb->plmCmd.user_sn, pcb->plmCmd.node_addr, plm_protocol, (PLM_PHASE)pcb->plmCmd.plm_phase, took_time_sec, pcb->pPlcRsp->body, pcb->pPlcRsp->header.msg_len);
		pcb->tick_post = tick_now+(OS_CFG_TICK_RATE_HZ*8);
		pcb->state = EM_ROUTE_READ_WAIT_POST;
		OSSemSet(&g_pOsRes->savedata_mbox, 0, &err);

		return OK;
//		g_pStaStatusInfo[pcb->tei].freeze_done=1;
	}
	else if (EM_ROUTE_READ_WAIT_POST == pcb->state)
	{
		//try post again ?
		OS_ERR err;
		OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz/2, OS_OPT_PEND_NON_BLOCKING, NULL, &err);
		if (err==OS_ERR_NONE)
		{
			pcb->state = EM_ROUTE_READ_POSTED;
		}
		if (tick_now > pcb->tick_post)
			pcb->state = EM_ROUTE_READ_POSTED;
		return (OK);
	}
	else if (EM_ROUTE_READ_POSTED == pcb->state)
	{
		if (pcb->plc_read_success_counter >= CON_READ_COUNT_TOTAL)
			goto L_NEXT_METER;
		else
			goto L_NEXT_ITEM;
	}
	else
	{
		return OK;
	}

L_NEXT_ITEM:
	free_buffer(__func__,pcb->pPlmRsp);
	pcb->pPlmRsp = NULL;
//    free_buffer(__func__,pcb->pPlmRspLast);
//    pcb->pPlmRspLast = NULL;
	free_buffer(__func__,pcb->pPlcRsp);
	pcb->pPlcRsp = NULL;
//    pcb->plc_read_success_counter = 0;
//    pcb->tei++;
	pcb->state = EM_ROUTE_READ_INIT;
	return OK;

L_NEXT_METER:
	free_buffer(__func__,pcb->pPlmRsp);
	pcb->pPlmRsp = NULL;
//	free_buffer(__func__,pcb->pPlmRspLast);
//	pcb->pPlmRspLast = NULL;
	free_buffer(__func__,pcb->pPlcRsp);
	pcb->pPlcRsp = NULL;
	pcb->plc_read_success_counter = 0;
	pcb->sn++;
	pcb->state = EM_ROUTE_READ_INIT;
	return OK;
}

#endif



#ifdef ERROR_APHASE
u8 PLC_PhaseErrProc(OS_TICK_64 tick_now)
{
	P_PLC_PHASE_ERR_CB pcb = &g_pRelayStatus->phase_err_cb;
	if (pcb->status==0)
	{
		if (g_pRelayStatus->online_num&&g_pRelayStatus->all_meter_num)
		{
			u8 sta_percent = g_pRelayStatus->online_num * 100 / g_pRelayStatus->all_meter_num;
			if (sta_percent>=100)
			{
				sta_percent=100;
				if (sta_percent > pcb->sta_percent)
				{
					pcb->sta_percent=sta_percent;
					PLC_PhaseErrStart(0);
				}
			}
			else if (sta_percent>=80)
			{
				sta_percent=80;
				if (sta_percent > pcb->sta_percent)
				{
					pcb->sta_percent=sta_percent;
					PLC_PhaseErrStart(0);
				}
			}
			else if (sta_percent>=50)
			{
				sta_percent=50;
				if (sta_percent > pcb->sta_percent)
				{
					pcb->sta_percent=sta_percent;
					PLC_PhaseErrStart(0);
				}
			}
			else if (sta_percent>=30)
			{
				sta_percent=30;
				if (sta_percent > pcb->sta_percent)
				{
					pcb->sta_percent=sta_percent;
					PLC_PhaseErrStart(0);
				}
			}
		}
		return OK;
	}
	else if (pcb->status==1)
	{
		pcb->seq=++g_pRelayStatus->packet_seq_read_meter;
		pcb->status=2;
	}
	
	if (TicksBetween64(tick_now, pcb->tick_last_snd) < 5 * OSCfg_TickRate_Hz)
		return OK;
	if (pcb->cnt)
	{
		--pcb->cnt;
		PLC_SendPhaseErr(pcb->seq);
		pcb->tick_last_snd = tick_now;
		return OK;
	}
	else
	{
		pcb->status=0;
	}
	return OK;
}
#endif

//广播校时
u8 PLC_BroadcastProc(OS_TICK_64 tick_now)
{
	P_PLC_BROADCAST_CB pcb = &g_pRelayStatus->broadcast_cb;
	static u32 total_timeout=0;
	static u16 seq=1; 
	static u8 timing_seq = 0;
	if (NULL == pcb->pSendMsg)
	{
		if (NULL!=pcb->pSendMsgHight)
		{
			pcb->pSendMsg=pcb->pSendMsgHight;
			seq=++g_pRelayStatus->packet_seq_read_meter;
			pcb->pSendMsgHight=NULL;
		}
		else if (NULL != pcb->pSendMsgLower)
		{
			pcb->pSendMsg=pcb->pSendMsgLower;
			seq=++g_pRelayStatus->packet_seq_read_meter;
			pcb->pSendMsgLower=NULL;
		}
		else if (NULL != pcb->pSendMsgBak)
		{
			pcb->pSendMsg=pcb->pSendMsgBak;
			seq=++g_pRelayStatus->packet_seq_read_meter;
			pcb->pSendMsgBak=NULL;
		}
		else
		{
			return OK;
		}
        
		pcb->tick_start = GetTickCount64();
		pcb->tick_last_snd = 0;
		if (pcb->pSendMsg->msg_header.protocol==METER_PROTOCOL_3762_BROAD||pcb->pSendMsg->msg_header.protocol==METER_PROTOCOL_3762_NEW_BROAD_TIME)
		{
			total_timeout=9 * OS_CFG_TICK_RATE_HZ +OS_CFG_TICK_RATE_HZ/3;
		}
		else
		{
			total_timeout = 20 * OS_CFG_TICK_RATE_HZ;
		}

#ifdef HPLC_CSG //南网21标准深化应用精准对时
        if (pcb->pSendMsg->msg_header.protocol == METER_PROTOCOL_3762_NEW_BROAD_TIME)
        {
            timing_seq = (++timing_seq==0)?1:timing_seq; //序号范围1-255
        }
#endif        
	}
		

	if (TicksBetween64(tick_now, pcb->tick_last_snd) < 3 * OSCfg_TickRate_Hz)
		return OK;

	if (TicksBetween64(tick_now, pcb->tick_start) >= total_timeout)       //30
	{
		pcb->tick_start = 0;
#ifdef HPLC_CSG
		if(pcb->pSendMsg->msg_header.protocol != METER_PROTOCOL_3762_BROAD_TIME) //正常广播校时上报，精确校时不上报
		{
			u8 addr[MAC_ADDR_LEN];
			memset(addr, 0x99, MAC_ADDR_LEN);
			PLM_ReportTaskStatus_gd(COM_PORT_MAINTAIN,pcb->pSendMsg->msg_header.meter_sn, addr, 0);
		}
#endif
		free_buffer(__func__, pcb->pSendMsg);
		pcb->pSendMsg = NULL;
		return OK;
	}
	#ifndef HPLC_CSG
	//暂时国网每发1帧序列号增加
	seq=++g_pRelayStatus->packet_seq_read_meter;
	#endif
#ifdef HPLC_GDW
	if (pcb->pSendMsg->msg_header.accutate_timing==1)
	{
		timing_seq = (++timing_seq==0)?1:timing_seq; //序号范围1-255
		PLC_SendAccurateTiming(pcb->pSendMsg, seq, timing_seq);//国网精准校时
		pcb->tick_last_snd = tick_now;
		return OK;
	}
	else
#endif	  
	{
		PLC_SendBroadcast(pcb->pSendMsg, seq, timing_seq);
		pcb->tick_last_snd = tick_now;
		return OK;
	}

}
#endif //COPY_OUT


//集中器传输文件完毕，启动升级
u8 PLC_StartUpdateSta(u32 flash_addr, u32 file_size, u32 file_checksum)
{
	P_TMSG pMsg = NULL;
	P_STA_UPDATE_PARAM pStaUpdateParam = NULL;
	OS_ERR err;

	pMsg = alloc_msg(__func__);
	if (NULL == pMsg)
		goto L_Error;

	pStaUpdateParam = (P_STA_UPDATE_PARAM)alloc_buffer(__func__,sizeof(STA_UPDATE_PARAM));
	if (NULL == pStaUpdateParam)
		goto L_Error;

	pStaUpdateParam->flash_addr = flash_addr;
	pStaUpdateParam->file_size = file_size;
	pStaUpdateParam->file_checksum = file_checksum;

	pMsg->message = MSG_START_UPDATE_STA;
	pMsg->lParam = (LPARAM)pStaUpdateParam;

	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return OK;

L_Error:
	free_buffer(__func__,pStaUpdateParam);
	free_msg(__func__,pMsg);

	return (!OK);
}

//集中器传输文件的时候需要停止升级
u8 PLC_StopUpdateSta()
{
	P_TMSG pMsg = alloc_msg(__func__);
	if (NULL == pMsg)
		goto L_Error;

	pMsg->message = MSG_STOP_UPDATE_STA;

	OS_ERR err;
	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return OK;

L_Error:
	free_msg(__func__,pMsg);

	return (!OK);
}
BOOL IsAllStaDone(u32 bitmap[MAX_BITMAP_SIZE / 4])
{
	u32 *p = (u32 *)bitmap;
	for (int i = 0; i < MAX_BITMAP_SIZE / 4; i++)
	{
		if (*p != 0)
		{
			for (int j = 0; j < 32; j++)
			{
				if (*p & (1 << j))
				{
					if (PLC_GetValidStaInfoByTei((i << 5) | j))
					{
						debug_str(DEBUG_LOG_UPDATA, "IsAllStaDone is not done, tei:%d, %d-%d\r\n", ((i << 5) | j), i, j);
						return FALSE;
					}
				}
			}
		}
		++p;
	}
	debug_str(DEBUG_LOG_UPDATA, "IsAllStaDone is done\r\n");
	return TRUE;
}
static BOOL IsAllMapDone(u8 *bitmap, u16 len)
{
	u32 *p = (u32 *)bitmap;
	u16 len32 = len / 4;
	for (int i = 0; i < len32; i++)
	{
		if (*p != 0)
		{
			debug_str(DEBUG_LOG_UPDATA, "IsAllMapDone is not done, i:%d, p:0x%x\r\n", i, *p);
			return false;
		}
		++p;
	}
	for (int i = len32 * 4; i < len; i++)
	{
		if (bitmap[i] != 0)
		{
			debug_str(DEBUG_LOG_UPDATA, "IsAllMapDone is not done, i:%d, 0x%x\r\n", i, bitmap[i]);
			return false;
		}
	}
	debug_str(DEBUG_LOG_UPDATA, "IsAllMapDone is done\r\n");
	return TRUE;
}
static u16 getUpdateStaMap(u32 bitmap[MAX_BITMAP_SIZE / 4], bool checkStartFlag)
{
	memset(bitmap, 0, MAX_BITMAP_SIZE);
	P_STA_INFO pSta;
	u16 num = 0;
	for (int i = TEI_STA_MIN; i < TEI_CNT; i++)
	{
		pSta = PLC_GetValidStaInfoByTei(i);
		if (pSta != NULL)
		{
			if (checkStartFlag) //开始标志
			{
				if (g_pStaStatusInfo[i].need_up)
				{
					bitmap[i >> 5] |= 1 << (i & 0x1f);
					num++;
				}
			}
			else
			{
				if (g_pStaStatusInfo[i].need_up && g_pStaStatusInfo[i].updateGetStart)
				{
					bitmap[i >> 5] |= 1 << (i & 0x1f);
					num++;
				}
			}
		}
	}
	return num;

}
//广播发送A流程
#define UPDATA_TRY_TIME  4
typedef unsigned char (*FUNC_UPDATE)(void);
PLAN_STATUS PLC_UpDatePlanA(u8 *wait_flag,OS_TICK_64 tick_now, P_PLC_STA_UPDATE_CB pcb, FUNC_UPDATE func, BOOL start,const char *log)
{
//	P_STA_INFO pSta;
	int i;
	static OS_TICK_64 wait_tick=0; 
	switch (pcb->planStatus)
	{
	case PLAN_INIT:
		if (pcb->broad) //以广播开始   第一次设置需要发送的stabitmap
		{
			pcb->total_sta = getUpdateStaMap(pcb->staBitmap, start);
			pcb->tei = TEI_STA_MIN - 1;
		}
		else
		{
			u16 tei = 0;
			for (i = pcb->tei >> 5; i < sizeof(pcb->staBitmap) / 4; i++)
			{
				if (pcb->staBitmap[i] != 0)
				{
					for (int j = 0; j < 32; j++)
					{
						if (pcb->staBitmap[i] & (1 << j))
						{
							tei = i << 5 | j;
							if (tei > pcb->tei)
							{
								break;
							}
						}
					}
					if (tei > pcb->tei)
					{
						break;
					}
				}
			}
//			pcb->plc_send_counter = UPDATA_TRY_TIME;
			
			if (tei <= pcb->tei || tei > TEI_STA_MAX)
			{
				pcb->plc_send_counter--;
				if (pcb->plc_send_counter==0)
				{
					pcb->planStatus = PLAN_INIT;
					return PLAN_DONE;
				}
				else
				{
					pcb->tei = TEI_STA_MIN - 1;
					pcb->planStatus = PLAN_INIT;
					return PLAN_RUNNING;
				}
			}
			else
			{
				debug_str(DEBUG_LOG_UPDATA, "PLAN_INIT,send counter:%d, tei:%d\r\n", pcb->plc_send_counter,tei);
				pcb->tei = tei;
			}
		}
		pcb->planStatus = PLAN_SEND;
		break;
	case PLAN_SEND:
		if (pcb->broad)
		{
			wait_tick = beacon_param_current.beacon_period_ms * 2;
			wait_tick = wait_tick>15*1000?15*1000:wait_tick;
			pcb->tick_send = tick_now + wait_tick;
			func();
		}
		else
		{
			wait_tick = 1000/2;
			pcb->tick_send = tick_now + wait_tick;
			func();
		}

		if ((pcb->plan) && (pcb->broad))
		{
			debug_str(DEBUG_LOG_UPDATA, "%s PLAN_SEND broadcast\r\n", log);
		}
		else
		{
			debug_str(DEBUG_LOG_UPDATA, "%s PLAN_SEND unicast, tei:%d\r\n", log, pcb->tei);
		}

		pcb->planStatus = PLAN_WAIT;
		break;
	case PLAN_WAIT:
		if (*wait_flag)
		{
			if (tick_now > pcb->tick_send)
			{
				(*wait_flag)--;
				pcb->tick_send = tick_now+wait_tick;
				break;
			}
		}
		if (tick_now > pcb->tick_send)
		{
			if (pcb->broad)
			{
				if (IsAllStaDone(pcb->staBitmap))
				{
					pcb->planStatus = PLAN_INIT;
					return PLAN_DONE;
				}
				else
				{
					debug_str(DEBUG_LOG_UPDATA, "PLAN_WAIT, send counter:%d\r\n", pcb->plc_send_counter);
					if (pcb->plc_send_counter)
					{
						--pcb->plc_send_counter;
						pcb->planStatus = PLAN_SEND;
					}
					else
					{
						pcb->plc_send_counter = UPDATA_TRY_TIME;
						pcb->planStatus = PLAN_INIT;
						pcb->broad = 0; //进入单拨路程
						pcb->tei = TEI_STA_MIN - 1;
					}
				}
			}
			else
			{
				if (IsAllStaDone(pcb->staBitmap))
				{
					pcb->planStatus = PLAN_INIT;
					return PLAN_DONE;
				}
				else
				{
					pcb->planStatus = PLAN_INIT;
				}
			}
		}
		break;
	}
	return PLAN_RUNNING;
}
void  InitProcPara(P_PLC_STA_UPDATE_CB pcb, EM_STA_UPDATE_STATE status, u32 timeout, u8  broad, u8 send_cnt, BOOL bitmap, BOOL startFlag)
{
	pcb->state = status;
	pcb->plc_send_counter = send_cnt;
	pcb->broad = broad;
	pcb->get_current_status = 0;
	pcb->tick_timeout = timeout;
	if (bitmap)
	{
		if (EM_STA_UPDATE_REQ_STATUS == status)
		{
			memset(pcb->bitmap, 0, (pcb->block_count + 7) / 8);
		}
		else
		{
			memset(pcb->bitmap, 0xff, (pcb->block_count + 7) / 8);
			u8 remain = pcb->block_count % 8;
			u8 byte_data = 0;
			if (remain)
			{
				for (int bit = 0; bit < remain; bit++)
				{
#ifdef HPLC_CSG
					byte_data |= 1 << (7 - bit);
#else
					byte_data |= 1 << bit;
#endif
				}
				pcb->bitmap[(pcb->block_count + 7) / 8 - 1] = byte_data;
			}
		}
	}

	pcb->total_sta = getUpdateStaMap(pcb->staBitmap, startFlag);

}
#define BROAD_SEND_DATE_TIMEOUT (OS_CFG_TICK_RATE_HZ)
#define SINGL_SEND_DATE_TIMEOUT (OS_CFG_TICK_RATE_HZ/5)

u8 PLC_RouteUpdateProc(OS_TICK_64 tick_now)
{
	P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;
	if (!pcb->need_update)
	{
		if (Bcsma_Lid == SOF_LID_UPDATE)
		{
			if ((getBeaconPeriod() - pcb->period) > 2)
			{
				UpdataLidReq = 2;
			}
		}
		return OK;
	}
	else
	{
		if ((getBeaconPeriod() - pcb->period) < 1) //等待进入绑定csma
		{
			return OK;
		}
	}

	if (!pcb->plan) //单拨流程
	{
		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pcb->tei);
		if ((NULL == pSta) || (NET_IN_NET != pSta->net_state))
			goto L_NEXT_METER;

		if (pcb->update_round != 0) //pcb->update_round==0轮次，执行EM_STA_UPDATE_EXE
		{
			//如果已升级，跳转到下一只表
			if (!g_pStaStatusInfo[pcb->tei].need_up)
				goto L_NEXT_METER;
		}
		//一只表设置3次升级机会
//		if (g_pStaStatusInfo[pcb->tei].try_update_counter >= 3)
//			goto L_NEXT_METER;

	}
	if (EM_STA_UPDATE_INIT == pcb->state)
	{
		debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_INIT, plan:%d\r\n", pcb->plan);
		needSendStop = 0;
		int tei;
		for (tei = TEI_STA_MIN; tei < TEI_STA_MAX; tei++)
		{
			if (PLC_GetValidStaInfoByTei(tei))
			{
				if (g_pStaStatusInfo[tei].need_up)
				{
					needSendStop = 1;
				}
				else
				{
					g_pStaStatusInfo[tei].need_up = 1;
				}
				g_pStaStatusInfo[tei].updateGetStart = 0;
			}
		}

		if (
#ifdef HPLC_CSG
			1
#else
			needSendStop
#endif

			)
		{
			InitProcPara(pcb, EM_STA_UPDATE_STOP, OS_CFG_TICK_RATE_HZ * 3, 1, 10, false, true);
		}
		else
		{
			InitProcPara(pcb, EM_STA_UPDATE_START, OS_CFG_TICK_RATE_HZ * 3, 1, CON_PLC_RESEND_COUNT, false, true);
		}
		if (pcb->plan)
		{
			pcb->update_round = 20;
		}
		else
		{
			pcb->update_round = 5;
		}
		pcb->retry = 20;
		return OK;
	}
#if 0
	else if (EM_STA_UPDATE_REQ_INFO == pcb->state)
	{
		if (pcb->plan) //plan A
		{
			PLAN_STATUS status;
			status=PLC_UpDatePlanA(tick_now, pcb, PLC_SendUpdateReqStaInfo, false);
			if (status==PLAN_ERROR)
			{
				return !OK;
			}
			else if (status==PLAN_DONE)
			{
				InitProcPara(pcb, EM_STA_UPDATE_REQ_INFO,OS_CFG_TICK_RATE_HZ*3, 1, CON_PLC_RESEND_COUNT, false, false);
				return OK;
			}
		}
		else //plan B
		{
			if (OK == PLC_SendUpdateReqStaInfo())
			{
//				pcb->read_sta_info = FALSE;
				pcb->tick_send = tick_now;
				InitProcPara(pcb, EM_STA_UPDATE_REQ_INFO,OS_CFG_TICK_RATE_HZ*3, 1, pcb->plc_send_counter, false, false);
				if (pcb->plc_send_counter)
				pcb->plc_send_counter--;
				debug_str("PLC_SendUpdateReqStaInfo\r\n");
				return (!OK);
			}
			else
			{
				goto L_NEXT_METER;
			}
		}
	}
	else if (EM_STA_UPDATE_WAIT_REQ_INFO == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->tick_send) > pcb->tick_timeout)
		{
			if (pcb->plc_send_counter)
			{
				InitProcPara(pcb, EM_STA_UPDATE_REQ_INFO,OS_CFG_TICK_RATE_HZ*3, 1, pcb->plc_send_counter, false, false);
				return OK;
			}
			else
			{
				g_pStaStatusInfo[pcb->tei].try_update_counter++;
				goto L_NEXT_METER;
			}
		}
		else if (pcb->get_current_status)
		{
			g_pStaStatusInfo[pcb->tei].try_update_counter = 0;
			if ((pcb->file_size_sta == pcb->file_size) && (pcb->file_checksum_sta == pcb->file_checksum))
			{
				g_pStaStatusInfo[pcb->tei].need_up = 0;
				goto L_NEXT_METER;
			}
			else
			{
				goto L_NEXT_METER;
//				pcb->state = EM_STA_UPDATE_STOP;      //EM_STA_UPDATE_START;
//				pcb->plc_send_counter = CON_PLC_RESEND_COUNT;
//				return OK;
			}
		}
		return (!OK);

	}
#endif
	else if (EM_STA_UPDATE_STOP == pcb->state)
	{
		debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_STOP, tei:%d, plan:%d, round:%d\r\n", pcb->tei, pcb->plan, pcb->update_round);
		if (OK == PLC_SendUpdateStop())
		{
			pcb->tick_send = tick_now;
			InitProcPara(pcb, EM_STA_UPDATE_WAIT_STOP, OS_CFG_TICK_RATE_HZ * 3, 1, pcb->plc_send_counter, false, false);
			if (pcb->plc_send_counter)
				pcb->plc_send_counter--;
			return (!OK);
		}
		else
		{
			InitProcPara(pcb, EM_STA_UPDATE_WAIT_STOP, OS_CFG_TICK_RATE_HZ * 3, 1, pcb->plc_send_counter, false, false);
		}
	}
	else if (EM_STA_UPDATE_WAIT_STOP == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->tick_send) > pcb->tick_timeout && tick_now > pcb->tick_send)
		{
			debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_WAIT_STOP timeout, tei:%d, send counter:%d, plan:%d, round:%d\r\n", pcb->tei, pcb->plc_send_counter, pcb->plan, pcb->update_round);

			if (pcb->plc_send_counter)
			{
				pcb->state = EM_STA_UPDATE_STOP;
				return OK;
			}
			else
			{
				InitProcPara(pcb, EM_STA_UPDATE_START, OS_CFG_TICK_RATE_HZ * 3, 1, CON_PLC_RESEND_COUNT, false, true);
				return OK;
			}
		}
		return (OK);
	}
	else if (EM_STA_UPDATE_START == pcb->state)
	{
		if (pcb->plan) //plan A
		{

			PLAN_STATUS status;
			status = PLC_UpDatePlanA(&startflag, tick_now, pcb, PLC_SendUpdateStart, true,"Send Start");
			if (status == PLAN_ERROR)
			{
				return !OK;
			}
			else if (status == PLAN_DONE)
			{
				debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_START, PLAN_DONE, round:%d\r\n", pcb->update_round);

				InitProcPara(pcb, EM_STA_UPDATE_DATA, BROAD_SEND_DATE_TIMEOUT, 1, CON_PLC_RESEND_COUNT, true, false);
				return OK;
			}
			return OK;
		}
		else //plan B
		{
			if (OK == PLC_SendUpdateStart())
			{
				debug_str(DEBUG_LOG_UPDATA, "PLC_SendUpdateStart ok, tei:%d, round:%d\r\n", pcb->tei, pcb->update_round);
					pcb->tick_send = tick_now + OS_CFG_TICK_RATE_HZ * 1;
				InitProcPara(pcb, EM_STA_UPDATE_WAIT_START, OS_CFG_TICK_RATE_HZ * 1, 1, pcb->plc_send_counter, true, true);
				if (pcb->plc_send_counter)
					pcb->plc_send_counter--;
				return (!OK);
			}
			else
			{
				debug_str(DEBUG_LOG_UPDATA, "PLC_SendUpdateStart not ok, tei:%d, round:%d\r\n", pcb->tei, pcb->update_round);
				goto L_NEXT_METER;
			}
		}
	}
	else if (EM_STA_UPDATE_WAIT_START == pcb->state)
	{
		if (startflag)
		{
			if (TicksBetween64(tick_now, pcb->tick_send) > pcb->tick_timeout && tick_now > pcb->tick_send)
			{
				startflag--;
				pcb->tick_send = tick_now;
			}
			return OK;
		}
		if (TicksBetween64(tick_now, pcb->tick_send) > pcb->tick_timeout && tick_now > pcb->tick_send)
		{
			debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_WAIT_START timeout, plan:%d, round:%d, send counter:%d, current status:%d\r\n",
					  pcb->plan, pcb->update_round, pcb->plc_send_counter, pcb->get_current_status);

			if (pcb->plc_send_counter && (!pcb->get_current_status))
			{
				pcb->state = EM_STA_UPDATE_START;
				InitProcPara(pcb, EM_STA_UPDATE_START, OS_CFG_TICK_RATE_HZ * 3, 1, pcb->plc_send_counter, false, true);

				return OK;
			}
			else if (pcb->get_current_status)
			{
				InitProcPara(pcb, EM_STA_UPDATE_DATA, SINGL_SEND_DATE_TIMEOUT, 1, CON_PLC_RESEND_COUNT, false, true);
				pcb->retry = 20;
				return OK;
			}
			else
			{
				goto L_NEXT_METER;
			}
		}
		return (OK);
	}
	else if (EM_STA_UPDATE_DATA == pcb->state)
	{
		debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_DATA, plan:%d, round:%d, UpdataLidReq:%d, Bcsma_Lid:0x%x\r\n",
				  pcb->plan, pcb->update_round, UpdataLidReq, Bcsma_Lid);

		//u32 block_id;
		BOOL bAllSent;
		if (OK == PLC_SendUpdateData(FALSE, &pcb->block_id, &bAllSent))
		{
			if (bAllSent)
			{
				debug_str(DEBUG_LOG_UPDATA, "PLC_SendUpdateData, all send\n");
				InitProcPara(pcb, EM_STA_UPDATE_REQ_STATUS, OS_CFG_TICK_RATE_HZ, 1, 1, true, false);
				pcb->get_sta_status_frame = 0;
				return OK;
			}
			else
			{
				if ((pcb->plan) && (pcb->broad))
				{
					
					u32 * p_level=(u32 * )alloc_buffer(__func__, (MAX_RELAY_DEEP+1)*4);
					if (p_level)
					{
						u32 max_listen_pco=0;
						for (int i=0;i<MAX_RELAY_DEEP;i++)
						{
							p_level[i] = g_pRelayStatus->innet_num_level[i]-g_pRelayStatus->sta_num_level[i];
						}
						for (int i=0;i<MAX_RELAY_DEEP-3;i++)//找到三个相邻层次pco和中最大的
						{
							u32 temp=p_level[i]+p_level[i+1]+p_level[i];
							if (temp>max_listen_pco)
							{
								max_listen_pco=temp;
							}
						}
						pcb->tick_timeout=max_listen_pco*20;
						if (pcb->tick_timeout < 200)
						{
							pcb->tick_timeout =200;
						}
						if (pcb->update_round>18)//前两轮发送时间较短
						{
							if (pcb->tick_timeout > 400)
							{
								pcb->tick_timeout = 400;
							}
						}
						else 
						{
							if (pcb->tick_timeout > 1500)
							{
								pcb->tick_timeout = 1500;
							}
						}
						free_buffer(__func__, p_level);
					}
					else
					{
						pcb->tick_timeout=1000;
					}
					debug_str(DEBUG_LOG_UPDATA, "PLC_SendUpdateData broadcast, block id:%d-%d\r\n", pcb->block_id, pcb->block_count);
				}
				else
				{
					pcb->tick_timeout = OS_CFG_TICK_RATE_HZ/4;
					debug_str(DEBUG_LOG_UPDATA, "PLC_SendUpdateData unicast, tei:%d, block id:%d-%d\r\n", pcb->tei, pcb->block_id, pcb->block_count);
				}
				pcb->tick_send = tick_now;
				InitProcPara(pcb, EM_STA_UPDATE_WAIT_DATA, pcb->tick_timeout, 1, CON_PLC_RESEND_COUNT, false, false);
				return OK;
			}
		}
		else
		{
			debug_str(DEBUG_LOG_UPDATA, "PLC_SendUpdateData, not ok\n");
			pcb->tick_send = tick_now;
			InitProcPara(pcb, EM_STA_UPDATE_WAIT_DATA, OS_CFG_TICK_RATE_HZ, 1, CON_PLC_RESEND_COUNT, false, false);
			return (!OK);
		}
	}
	else if (EM_STA_UPDATE_WAIT_DATA == pcb->state)
	{
		if (sendflag)
		{
			if (TicksBetween64(tick_now, pcb->tick_send) > pcb->tick_timeout && tick_now > pcb->tick_send)
			{
				sendflag--;
				pcb->tick_send = tick_now;
			}
			return OK;
		}
		if (TicksBetween64(tick_now, pcb->tick_send) > pcb->tick_timeout && tick_now > pcb->tick_send)
		{
			pcb->state = EM_STA_UPDATE_DATA;
			return OK;
		}
		return (OK);
	}

	else if (EM_STA_UPDATE_REQ_STATUS == pcb->state)
	{
		if (pcb->plan) //plan A
		{
			PLAN_STATUS status;

			if (update_reg_status_unicast == 1)
			{
				if (pcb->broad) //设置需要发送的stabitmap
				{
					pcb->total_sta = getUpdateStaMap(pcb->staBitmap, false);
					pcb->tei = TEI_STA_MIN - 1;
					pcb->plc_send_counter = UPDATA_TRY_TIME;
					pcb->broad = 0;
					debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_REQ_STATUS, UNICAST, total sta:%d\r\n", pcb->total_sta);
				}
			}

			status = PLC_UpDatePlanA(&statusflag, tick_now, pcb, PLC_SendUpdateReqStatus, false, "Ask Status");
			if (status == PLAN_ERROR)
			{
				return !OK;
			}
			else if (status == PLAN_DONE)
			{
				debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_REQ_STATUS, PLAN_DONE\r\n");
				pcb->tick_send = tick_now;
				InitProcPara(pcb, EM_STA_UPDATE_WAIT_REQ_STATUS, OS_CFG_TICK_RATE_HZ / 10, 1, pcb->plc_send_counter, false, false);
				return OK;
			}
			return OK;
		}
		else //plan B
		{
			if (OK == PLC_SendUpdateReqStatus())
			{
				debug_str(DEBUG_LOG_UPDATA, "PLC_SendUpdateReqStatus ok, tei:%d, send counter:%d, total sta:%d\r\n", pcb->tei, pcb->plc_send_counter, pcb->total_sta);

					pcb->tick_send = tick_now + OS_CFG_TICK_RATE_HZ * 1;

				InitProcPara(pcb, EM_STA_UPDATE_WAIT_REQ_STATUS, OS_CFG_TICK_RATE_HZ * 1, 1, pcb->plc_send_counter, false, false);
				if (pcb->plc_send_counter)
					pcb->plc_send_counter--;
				return (!OK);
			}
			else
			{
				debug_str(DEBUG_LOG_UPDATA, "PLC_SendUpdateReqStatus not ok, tei:%d\r\n", pcb->tei);
				goto L_NEXT_METER;
			}
		}
	}
	else if (EM_STA_UPDATE_WAIT_REQ_STATUS == pcb->state)
	{
		if (statusflag)
		{
			if (TicksBetween64(tick_now, pcb->tick_send) > pcb->tick_timeout && tick_now > pcb->tick_send)
			{
				statusflag--;
				pcb->tick_send = tick_now;
			}
			return OK;
		}
		if (TicksBetween64(tick_now, pcb->tick_send) > pcb->tick_timeout && tick_now > pcb->tick_send)
		{
			debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_WAIT_REQ_STATUS timeout, tei:%d, plan:%d, round:%d, total sta:%d\r\n", pcb->tei, pcb->plan, pcb->update_round, pcb->total_sta);

            if (pcb->block_count & 0x7) //block count不是8的倍数
            {
    			u8 clearbyte = 0; //APP_PACKET_CHECK_UPDATE对上报的bitmap取反|操作，最后一个bitmap需要清除不在block count范围的bit 
    			
        		for (int i = 0; i < (pcb->block_count & 0x7); i++)
        		{
#ifdef HPLC_CSG
        			clearbyte |= 1 << (7 - i);
#else 
        			clearbyte |= 1 << i;
#endif
        		}

                pcb->bitmap[(pcb->block_count + 7) / 8 - 1] &= clearbyte;
            }
            
			if (!pcb->plan)
			{
				if (!pcb->get_current_status)
				{
					if (pcb->plc_send_counter)
					{
						InitProcPara(pcb, EM_STA_UPDATE_REQ_STATUS, OS_CFG_TICK_RATE_HZ * 5, 1, pcb->plc_send_counter, false, false);
						pcb->get_sta_status_frame = 0;
						debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_WAIT_REQ_STATUS -> EM_STA_UPDATE_REQ_STATUS\r\n");
						return OK;
					}
					else
					{
						debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_WAIT_REQ_STATUS -> L_NEXT_METER\r\n");
						goto L_NEXT_METER;
					}
				}
			}
			//判断是否需要继续发送
			//预进入升级模式
			//更新需要升级的sta个数
			InitProcPara(pcb, EM_STA_UPDATE_DATA, pcb->plan ? BROAD_SEND_DATE_TIMEOUT : SINGL_SEND_DATE_TIMEOUT, 1, pcb->plc_send_counter, false, false);
			if (!IsAllMapDone(pcb->bitmap, (pcb->block_count + 7) / 8))
			{
				//update_minutes超时后还未执行完，直接升级
				if (TicksBetween64(tick_now, pcb->tick_start_update)/OSCfg_TickRate_Hz > (update_timeout_minutes-2)*60)
				{
					InitProcPara(pcb, EM_STA_UPDATE_EXE, OS_CFG_TICK_RATE_HZ / 10, 1, 20, false, false);
					return OK;
				}
				
				if (pcb->plan)
				{
					if (pcb->update_round != 0)
					{
						pcb->update_round--;
						//继续发送升级文件
						debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_WAIT_REQ_STATUS -> next round, tei:%d\r\n", pcb->tei);
						return !OK;
					}
				}
				else
				{
					if (pcb->retry != 0)
					{
						pcb->state = EM_STA_UPDATE_DATA;
						pcb->tick_timeout = SINGL_SEND_DATE_TIMEOUT;
						pcb->retry--;
						debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_WAIT_REQ_STATUS -> EM_STA_UPDATE_DATA\r\n");
						return !OK;
					}
				}
			}

			//判断是否所有节点都升级完毕
			//尝试进入执行升级流程

//			pcb->total_sta = getUpdateStaMap(pcb->staBitmap, false);
			if (pcb->total_sta == 0) //全部升级成功
			{
				debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_WAIT_REQ_STATUS -> EM_STA_UPDATE_EXE\r\n");
				InitProcPara(pcb, EM_STA_UPDATE_EXE, OS_CFG_TICK_RATE_HZ / 10, 1, 20, false, false);
			}
			else
			{
				//进入planB
				if (pcb->plan)
				{
					pcb->tei = TEI_STA_MIN - 1;
					pcb->plan = 0; //进入plan b
					pcb->update_round = 5;
					debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_WAIT_REQ_STATUS -> plan b, total sta:%d\r\n", pcb->total_sta);
				}
				else
				{
					debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_WAIT_REQ_STATUS -> another L_NEXT_METER, total sta:%d\r\n", pcb->total_sta);
					goto L_NEXT_METER;
				}
			}
		}

		return OK;
	}
	else if (EM_STA_UPDATE_EXE == pcb->state)
	{
		debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_EXE, plan:%d, round:%d\r\n", pcb->plan, pcb->update_round);

		if (OK == PLC_SendUpdateExe())
		{
			pcb->state = EM_STA_UPDATE_WAIT_EXE;
			pcb->tick_send = tick_now;
			pcb->tick_timeout = OSCfg_TickRate_Hz * 10;
			if (pcb->plc_send_counter)
				pcb->plc_send_counter--;
			return (!OK);
		}
	}
	else if (EM_STA_UPDATE_WAIT_EXE == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->tick_send) > pcb->tick_timeout && tick_now > pcb->tick_send)
		{
			debug_str(DEBUG_LOG_UPDATA, "EM_STA_UPDATE_WAIT_EXE, plan:%d, round:%d, send counter:%d\r\n", pcb->plan, pcb->update_round, pcb->plc_send_counter);
			if (pcb->plc_send_counter)
			{
				pcb->state = EM_STA_UPDATE_EXE;
				return OK;
			}
			else
			{
				debug_str(DEBUG_LOG_UPDATA, "end update\r\n");
				if (NULL != pcb->bitmap)
				{
					free_buffer(__func__,pcb->bitmap);
					pcb->bitmap = NULL;
				}
#ifdef HPLC_CSG
				u32 update_id = pcb->update_id;
				u32 old_file_size = pcb->old_file_size;
				u32 old_file_checksum = pcb->old_file_checksum;
				u16 old_block_count = pcb->old_block_count;		
#endif

				memset_special(pcb, 0, sizeof(PLC_STA_UPDATE_CB));
				pcb->period = getBeaconPeriod();
#ifdef HPLC_CSG				
				pcb->update_id = update_id;
				pcb->old_file_size = old_file_size;
				pcb->old_file_checksum = old_file_checksum;
				pcb->old_block_count = old_block_count;		
#endif
			}
		}
		return (OK);
	}
	else
	{
		return OK;
	}

L_NEXT_METER:
	if (!pcb->plan)
	{
		pcb->tei++;
		if (pcb->tei < TEI_STA_MIN || pcb->tei > TEI_STA_MAX)
		{
			debug_str(DEBUG_LOG_UPDATA, "L_NEXT_METER -> next round, tei:%d, plan:%d, round:%d\r\n", pcb->tei, pcb->plan, pcb->update_round);

			if (pcb->update_round == 0) //停止升级流程
			{
				if (NULL != pcb->bitmap)
				{
					free_buffer(__func__,pcb->bitmap);
					pcb->bitmap = NULL;
				}
				debug_str(DEBUG_LOG_UPDATA, "end update\r\n");
#ifdef HPLC_CSG				
				u32 update_id = pcb->update_id;
				u32 old_file_size = pcb->old_file_size;
				u32 old_file_checksum = pcb->old_file_checksum;
				u16 old_block_count = pcb->old_block_count;		
#endif

				memset_special(pcb, 0, sizeof(PLC_STA_UPDATE_CB));
				pcb->period = getBeaconPeriod();
#ifdef HPLC_CSG				
				pcb->update_id = update_id;
				pcb->old_file_size = old_file_size;
				pcb->old_file_checksum = old_file_checksum;
				pcb->old_block_count = old_block_count;		
#endif				
				return OK;
			}

			pcb->tei = TEI_STA_MIN;
			pcb->update_round--;
			if (pcb->update_round == 0) //升级完成
			{
				InitProcPara(pcb, EM_STA_UPDATE_EXE, OS_CFG_TICK_RATE_HZ * 5, 1, 10, false, false);
				for (int tei = TEI_STA_MIN; tei < TEI_STA_MAX; tei++) //清除所有的升级请求
				{
					g_pStaStatusInfo[tei].need_up = 0;
				}

				return OK;
			}
		}
		if (g_pStaStatusInfo[pcb->tei].updateGetStart) //已经回应开始升级状态 直接询问节点收到多少数据块
		{
			if (g_pStaStatusInfo[pcb->tei].need_up)
			{
				debug_str(DEBUG_LOG_UPDATA, "L_NEXT_METER from state:%d -> EM_STA_UPDATE_REQ_STATUS, tei:%d, plan:%d, round:%d\r\n", pcb->state, pcb->tei, pcb->plan, pcb->update_round);
			}

			pcb->retry = 20;
			InitProcPara(pcb, EM_STA_UPDATE_REQ_STATUS, OS_CFG_TICK_RATE_HZ * 3, 1, 3, true, false);
		}
		else //没有得到开始升级的ack 重新开始升级
		{
			if (g_pStaStatusInfo[pcb->tei].need_up)
			{
				debug_str(DEBUG_LOG_UPDATA, "L_NEXT_METER from state:%d -> EM_STA_UPDATE_START, tei:%d, plan:%d, round:%d\r\n", pcb->state, pcb->tei, pcb->plan, pcb->update_round);
			}

			pcb->state = EM_STA_UPDATE_START;
			pcb->plc_send_counter = CON_PLC_RESEND_COUNT;
		}
	}

	return OK;
}

//台区识别
u8 PLC_StartAreaDiscernment()
{
	return PLC_PostMessage(MSG_START_AREA_DISCERNMENT, 0, 0);
}

void PLC_StopAreaDiscernment(void *agc)
{
	#ifdef HPLC_CSG
	if (agc)
	{
		if (areaTimerId != INVAILD_TMR_ID)
		{
			HPLC_DelTmr(areaTimerId);
		}
	}
	areaTimerId = INVAILD_TMR_ID;
	#endif
	PLC_PostMessage(MSG_STOP_AREA_DISCERNMENT, 0, 0);
}

#if defined(PROTOCOL_GD_2016)

u8 PLC_PostCollTaskInit()
{
	return PLC_PostMessage(MSG_STA_READ_TASK_INIT, 0, 0);
}

u8 PLC_ReadCollTaskID(u8 mac_addr[MAC_ADDR_LEN], u8 packet_seq_3762)                       //查询采集任务号
{
	P_READ_MSG_INFO pReadMsg = (P_READ_MSG_INFO)alloc_buffer(__func__,sizeof(READ_MSG_INFO));

	if (NULL == pReadMsg)
		return (!OK);

	pReadMsg->header.broadcast = 0;           //0单播， 1 PCO广播

	//PLM 填充
	pReadMsg->header.read_count = 3;           //抄读次数
	pReadMsg->header.read_time = 0;
	pReadMsg->header.packet_seq_3762 = packet_seq_3762;      //3762 数据包序列号
	//unsigned short task_id;              //南网taskid
	memcpy_special(pReadMsg->header.mac, mac_addr, MAC_ADDR_LEN);
	//unsigned char  meter_protocol_plc:3;     //APP_METER_PRO: APP_PRO_DLT645_1997
	//unsigned char  packet_id:3;       //HPLC_APP_PACKET_ID
	//unsigned char  time_related:1;
	//unsigned char  reserve1:1;
	pReadMsg->header.sender = EM_SENDER_AFN_READ_COLL_TASK_ID_CSG;            //发送模块
	pReadMsg->header.msg_len = 0;

	return PLC_PostDirRead(pReadMsg);
}

u8 PLC_ReadCollTaskDetail(u8 mac_addr[MAC_ADDR_LEN], u8 task_id, u8 packet_seq_3762)       //查询采集任务详细信息
{
	P_READ_MSG_INFO pReadMsg = (P_READ_MSG_INFO)alloc_buffer(__func__,sizeof(READ_MSG_INFO));

	if (NULL == pReadMsg)
		return (!OK);

	pReadMsg->header.broadcast = 0;           //0单播， 1 PCO广播

	//PLM 填充
	pReadMsg->header.read_count = 3;           //抄读次数
	pReadMsg->header.read_time = 0;
	pReadMsg->header.packet_seq_3762 = packet_seq_3762;      //3762 数据包序列号
	//unsigned short task_id;              //南网taskid
	memcpy_special(pReadMsg->header.mac, mac_addr, MAC_ADDR_LEN);
	//unsigned char  meter_protocol_plc:3;     //APP_METER_PRO: APP_PRO_DLT645_1997
	//unsigned char  packet_id:3;       //HPLC_APP_PACKET_ID
	//unsigned char  time_related:1;
	//unsigned char  reserve1:1;
	pReadMsg->header.sender = EM_SENDER_AFN_READ_COLL_TASK_DETAIL_CSG;            //发送模块
	pReadMsg->header.msg_len = 1;

	pReadMsg->body[0] = task_id;

	return PLC_PostDirRead(pReadMsg);
}

u8 PLC_DeleteCollTask(u8 mac_addr[MAC_ADDR_LEN], u8 task_id, u8 packet_seq_3762)           //删除采集任务
{
	P_READ_MSG_INFO pReadMsg = (P_READ_MSG_INFO)alloc_buffer(__func__,sizeof(READ_MSG_INFO));

	if (NULL == pReadMsg)
		return (!OK);

	pReadMsg->header.broadcast = 0;           //0单播， 1 PCO广播

	//PLM 填充
	pReadMsg->header.read_count = 3;           //抄读次数
	pReadMsg->header.read_time = 0;
	pReadMsg->header.packet_seq_3762 = packet_seq_3762;      //3762 数据包序列号
	//unsigned short task_id;              //南网taskid
	memcpy_special(pReadMsg->header.mac, mac_addr, MAC_ADDR_LEN);
	//unsigned char  meter_protocol_plc:3;     //APP_METER_PRO: APP_PRO_DLT645_1997
	//unsigned char  packet_id:3;       //HPLC_APP_PACKET_ID
	//unsigned char  time_related:1;
	//unsigned char  reserve1:1;
	pReadMsg->header.sender = EM_SENDER_AFN_DELETE_COLL_TASK_CSG;            //发送模块
	pReadMsg->header.msg_len = 1;

	pReadMsg->body[0] = task_id;

	return PLC_PostDirRead(pReadMsg);
}

#endif

OS_TICK_64 start_zc_tick = 0;
extern u32 zeroBroadTime;
#define AREA_DISCERNMENT_START_INTERVAL 8 //采集启动的时间间隔，单位秒
#define AREA_DISCERNMENT_FEATURE_INTERVAL 9 //信息告知的时间间隔，单位秒

u8 PLC_AreaDiscernmentProc(OS_TICK_64 tick_now)
{
	P_PLC_AREA_DISCERNMENT pcb = &g_pRelayStatus->area_dis_cb;
//	static u32 online_num=0;
	static OS_TICK_64 next_ask_tick;
//	if (pcb->phase_or_area == 0)
	{
		static u16 ask_sta=TEI_STA_MIN;
#ifndef	HPLC_CSG
		u16 start_sta=ask_sta;
#endif

		if (pcb->allow && 
			#ifdef NW_TEST
			pcb->round > 25
			#else
			pcb->round > 80
			#endif
			) //进行问询sta的台区状态
		{
			if (tick_now > next_ask_tick)
			{
				P_STA_INFO pSta = NULL;
				next_ask_tick = tick_now + 
					#ifdef NW_TEST
					60 * OS_CFG_TICK_RATE_HZ;//南网送检1min查一次
					#else
					10*60 * OS_CFG_TICK_RATE_HZ;//默认10min后查询一次  当有新的节点需要被询问时时间修改为10s
					#endif
				for (;ask_sta <= TEI_STA_MAX; ask_sta++)
				{
					pSta = PLC_GetValidStaInfoByTei(ask_sta);
					if (NULL == pSta)
						continue;
					if (pSta->net_state != NET_IN_NET)
					{
						continue;
					}
					u16 sn= PLC_GetWhiteNodeSnByStaInfo(pSta);
					if (pcb->getedZoneStatusBitMap[sn>>5]&(1<<(sn&0x1f)))//已经得到该表的台区信息
					{
						continue ;
					}
					break;
				}
				
				if (ask_sta<=TEI_STA_MAX)//得到了一个需要询问的sta
				{
					debug_str(DEBUG_LOG_NET, "PLC_SendAreaDiscernmentQuery, tei:%d, mac:%02x%02x%02x%02x%02x%02x\r\n", ask_sta,
			                  pSta->mac[5], pSta->mac[4], pSta->mac[3], pSta->mac[2], pSta->mac[1], pSta->mac[0]);
					PLC_SendAreaDiscernmentQuery(ask_sta);
					next_ask_tick = tick_now + 10 * OS_CFG_TICK_RATE_HZ;//10s 后询问下一个sta
					ask_sta++;
					
				}
#if 1				
				else
				{
#ifndef HPLC_CSG //南网深化应用规定未达到最大时间，即使本台区所有节点都有明确归属（特征信息继续发送，为邻台区判别所用）
					if (start_sta == TEI_STA_MIN)//从开始到最大都没有需要获得的sta台区信息结束台区识别
					{
						PLM_ReportRouterWorkStatus(3);//上报台区信息
						pcb->allow = 0;//结束台区识别
						pcb->state = EM_AREA_DISCERNMENT_INIT;
					}
#endif					
					ask_sta = TEI_STA_MIN;
				}
#else
				else
				{
					if (start_sta==TEI_STA_MIN)//从开始到最大都没有需要获得的sta台区信息结束台区识别
					{
#ifndef HPLC_CSG
						PLM_ReportRouterWorkStatus(3);//上报台区信息
#endif
						pcb->allow=0;//结束台区识别
#ifdef PROTOCOL_NW_2020							
						ClearOutListMeter(1);
#endif
						pcb->state=EM_AREA_DISCERNMENT_INIT;
					}
					ask_sta=TEI_STA_MIN;
				}
#endif				
			}
#if 0			
			for (;ask_sta <= TEI_STA_MAX; ask_sta++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(ask_sta);
				if (NULL == pSta)
					continue;
				u16 sn= PLC_GetWhiteNodeSnByStaInfo(pSta);
				if (pcb->getedZoneStatusBitMap[sn>>5]&(1<<(sn&0x1f)))//已经得到该表的台区信息
				{
					continue ;
				}
				break;
			}
			if (ask_sta<=TEI_STA_MAX)//得到了一个需要询问的sta
			{
				PLC_SendAreaDiscernmentQuery(ask_sta);
				next_ask_tick = tick_now + 10 * OS_CFG_TICK_RATE_HZ;//10s 后询问下一个sta
				ask_sta++;
				
			}
			else
			{
				if (start_sta==TEI_STA_MIN)//从开始到最大都没有需要获得的sta台区信息结束台区识别
				{
					PLM_ReportRouterWorkStatus(3);//上报台区信息
					pcb->allow=0;//结束台区识别
					pcb->state=EM_AREA_DISCERNMENT_INIT;
				}
				ask_sta=TEI_STA_MIN;
			}
#endif			
		}

		if (EM_AREA_DISCERNMENT_INIT == pcb->state)
		{
#if 0
			u32 ntb = BPLC_GetNTB();
			for (int i = 0; i < 3; i++)
			{
				if (ZC_Time[i] != 0)
				{
					if ((ntb - ZC_Time[i]) > (NTB_FRE_PER_MS * 240)
              		  &&(ZC_Time[i] - ntb) > (NTB_FRE_PER_MS * 240)) //过零丢失
					{
						memset(&ZC_NTB[i * 2], 0, sizeof(phaseRecognizeType));
						memset(&ZC_NTB[i * 2 + 1], 0, sizeof(phaseRecognizeType));
						ZC_Time[i] = 0;
					}
				}
			}
			if ((0 == ZC_Time[0]) && (0 == ZC_Time[1]) && (0 == ZC_Time[2]))
			{
				return OK;
			}
			//if ((g_pRelayStatus->online_num * 10 / g_pRelayStatus->cfged_meter_num) < 6) //超过60%才开始统计相线
			if (!g_pRelayStatus->online_num)
			{
				return OK;
			}
			if (start_zc_tick == 0)
			{
#if 0
				start_zc_tick = tick_now+OS_CFG_TICK_RATE_HZ*12;
#else
				start_zc_tick = tick_now + (60 * 2 * OS_CFG_TICK_RATE_HZ); //第一次达到60%后等待时间
#endif
				return OK;
			}
			else
			{
				if (tick_now < start_zc_tick)
				{
					if (online_num!=0//判断online_num是否为0  第一次的2分钟是必须等待的
						&&online_num<g_pRelayStatus->online_num)//新增一个sta等待12s后开始进行识别
					{
						start_zc_tick=tick_now+12*OS_CFG_TICK_RATE_HZ;
						online_num=g_pRelayStatus->online_num;
					}
					return OK;
				}
			}

			online_num = g_pRelayStatus->online_num;

			pcb->phase_or_area = 1; //下一次开始相线识别
#endif
			return OK;
		}
		else if (EM_AREA_DISCERNMENT_START == pcb->state)
		{
			//采集启动报文中的起始NTB时间（告知STA在该NTB时间进行采集），尽量和特征信息告知中的起始采集NTB（CCO的采集时间）相近
			//采集启动报文的发送时间尽量和报文中起始NTB时间，间隔15秒以上，保证各种环境下，STA收到报文时，CCO采集还没有开始
			pcb->collection_seq++;
			pcb->tick_last_send = 0;
			pcb->send_counter = 0;
			pcb->state = EM_AREA_DISCERNMENT_SEND_START;
			return OK;
		}
		else if (EM_AREA_DISCERNMENT_SEND_START == pcb->state)
		{
#if 1	//发送信息告知前先发送采集开始，兼容海思方案
			if (TicksBetween64(tick_now, pcb->tick_last_send) >= 3 * OSCfg_TickRate_Hz)
			{
				if (!pcb->allow)
				{
					pcb->state = EM_AREA_DISCERNMENT_INIT;
				}
				else if (pcb->send_counter < 1)
				{
					debug_str(DEBUG_LOG_NET, "PLC_SendAreaDiscernmentStart, round:%d\r\n", pcb->round);
					pcb->start_ntb = BPLC_GetNTB() + ((AREA_DISCERNMENT_START_INTERVAL)*NTB_FRE_PER_SEC) - (NTB_FRE_PER_MS * 20 * (CON_FEATURE_COUNT_PER_PHASE+CON_FEATURE_COUNT_PER_PHASE/2 + 6));
					PLC_SendAreaDiscernmentStart(TEI_BROADCAST, pcb->start_ntb, pcb->collection_seq);
					pcb->tick_last_send = tick_now;
					pcb->send_counter++;
				}
				else
				{
					pcb->tick_last_send = tick_now;
					pcb->send_counter = 0;
					pcb->state = EM_AREA_DISCERNMENT_SEND_FEATURE;
				}
			}
#else		
			if (TicksBetween64(tick_now, pcb->tick_last_send) >= 5 * OSCfg_TickRate_Hz)
			{
				if (pcb->send_counter < 1)
				{
					PLC_SendAreaDiscernmentStart(TEI_BROADCAST);
					pcb->tick_last_send = tick_now;
					pcb->send_counter++;
				}
				else
				{
					pcb->tick_last_send = 0;
					pcb->send_counter = 0;
					pcb->state = EM_AREA_DISCERNMENT_SEND_FEATURE;
				}
			}
#endif			
			return OK;
		}
		else if (EM_AREA_DISCERNMENT_SEND_FEATURE == pcb->state)
		{
			if (TicksBetween64(tick_now, pcb->tick_last_send) >= AREA_DISCERNMENT_FEATURE_INTERVAL * OSCfg_TickRate_Hz)
			{
				if (pcb->send_counter < 1)
				{
					debug_str(DEBUG_LOG_NET, "PLC_SendAreaDiscernmentFeature, round:%d\r\n", pcb->round);
					PLC_SendAreaDiscernmentFeature();
					pcb->tick_last_send = tick_now;
					pcb->send_counter++;
				}
				else
				{
					pcb->tick_last_send = tick_now;
					pcb->send_counter = 0;
					pcb->state = EM_AREA_DISCERNMENT_WAIT_REPORT;
				}
			}
			return OK;
		}
		else if (EM_AREA_DISCERNMENT_WAIT_REPORT == pcb->state)
		{
			if (TicksBetween64(tick_now, pcb->tick_last_send) >= 40 * OSCfg_TickRate_Hz)
			{
				pcb->tick_last_send = 0;
				pcb->send_counter = 0;
				if (!pcb->allow)
				{
					pcb->state = EM_AREA_DISCERNMENT_INIT;
				}
				else
				{
					pcb->round++;
#if 1
					pcb->state = EM_AREA_DISCERNMENT_START;
#else
					pcb->state = EM_AREA_DISCERNMENT_SEND_FEATURE;
#endif					
				}
			}
			return OK;
		}
	}
#if 0
	else
	{
		static u8 phase_status = 0; //0 send 1 wait report 2 wait next
		static u8 waitTime = 20;
		static u16 tei;
		static u16 broadTime = 0;
		u16 find_node_num = 0;
		if (phase_status == 0)
		{
			tei = TEI_BROADCAST;

			pcb->tick_last_send = tick_now;
			for (int i = TEI_STA_MIN; i <= TEI_STA_MAX; i++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(i);
				if (NULL == pSta)
					continue;
				P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(i);
				if (pStaStatus->get_area_phase)
				{
					continue;
				}
#ifdef HPLC_CSG
//国网单三相  南网三项台区识别必须得到一次,不管如何得到的相位信息  
				if (0 != (pSta->phy_phase&0x3f)
					&&(pSta->phy_phase&(1<<7)))
				{
						continue;
				}
#endif	
				if (TicksBetween64(tick_now, pStaStatus->tick_innet) <= 10 * OSCfg_TickRate_Hz)
				{
					continue ;
				}
				if (1 == pStaStatus->zc_read_node_area)
					continue;

//				if (pStaStatus->zc_read_counter >= 4)
//					continue;

				

				if (TicksBetween64(tick_now, pStaStatus->tick_innet) <= 10 * OSCfg_TickRate_Hz)
					continue;

				if (!find_node_num) //第一次找到
				{
					pStaStatus->zc_read_node_area = 1;
//					pStaStatus->zc_read_counter++;
					tei = i;
				}
				find_node_num++;
			}

			if (tei > TEI_STA_MAX)
			{
				for (tei = TEI_STA_MIN; tei <= TEI_STA_MAX; tei++)
				{
//					P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
					P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
//					if (pSta != NULL
//						/*&& (!pSta->phy_phase)
//						&& (!pStaStatus->get_area_phase)*/)
					{
//						pStaStatus->first_unkonw_phase = 1;
//						pStaStatus->zc_read_counter = 0;
						pStaStatus->zc_read_node_area = 0;
					}
				}
				phase_status = 0;
				start_zc_tick = tick_now + (60 * 5 * OS_CFG_TICK_RATE_HZ); //一轮查询后等待5分钟再次查询
//				pcb->phase_or_area = 0; //退出相线识别
				return OK;
			}
			if (find_node_num == 0)
			{
				phase_status = 0;
				return OK;
			}
			if ((find_node_num * 10 / g_pRelayStatus->online_num) > 4)
			{
				if (broadTime<3)//前三次广播
				{
					P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
					if (pStaStatus)
					{
						pStaStatus->zc_read_node_area=0;
					}
					zeroBroadTime++;
					tei = TEI_BROADCAST;
				}
				broadTime++;
				if (broadTime > 200) //每200次允许3次广播获取
				{
					broadTime = 0;
				}
			}
			if (tei == TEI_BROADCAST)
			{
				#ifdef HPLC_CSG
				waitTime = 10;
				#else
				waitTime = 8;
				#endif
			}
			else
			{
				#ifdef HPLC_CSG
				waitTime = 5;
				#else
				waitTime = 8; //询问周期信息时间加长否则有些模块不回复  上行报文
				#endif
			}
			pcb->start_ntb = BPLC_GetNTB();
			#ifdef HPLC_CSG
			PLC_SendAppPhaseDiscernment(tei);
			#else
			PLC_SendAreaDiscernmentStart(tei);
			#endif
			phase_status = 1;

		}
		else if (phase_status == 1) //等待启动命令全部发送完成
		{
			if (TicksBetween64(tick_now, pcb->tick_last_send) >= waitTime * OSCfg_TickRate_Hz)
			{
				pcb->tick_last_send = tick_now;
				phase_status = 2;
				waitTime = waitTime*3/2;
			}
		}
#ifndef HPLC_CSG
		else if (phase_status == 2) //发送特征收集 获取sta特征信息
		{
			PLC_SendAreaDiscernmentCollect(tei);

			pcb->tick_last_send = tick_now;
			phase_status = 3;
		}
#endif
		else
		{
			if (TicksBetween64(tick_now, pcb->tick_last_send) >= waitTime * OSCfg_TickRate_Hz)
			{
				phase_status = 0; //等待下一次发送
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				
				if (pSta != NULL && pSta->phy_phase==0)
				{
					PLC_GetStaStatusInfoByTei(tei)->first_unkonw_phase=1;
				}
			}

//			pcb->phase_or_area = 0; //一次发送完成  回到台区识别流程

		}
		return OK;
	}
#endif
	return OK;
}

#define NOT_BROAD_GET_EDGE
u8 PLC_AreaGetEdgeProc(OS_TICK_64 tick_now)
{
	P_PLC_AREA_PHASE pcb = &g_pRelayStatus->area_phase_cb;
	static u32 online_num=0;
#ifdef APP_PHASE_SEND_START	
	P_PLC_AREA_DISCERNMENT areaPcb = &g_pRelayStatus->area_dis_cb;
#endif

	u32 ntb = BPLC_GetNTB();
	for (int i = 0; i < 3; i++)
	{
		if (ZC_Time[i] != 0)
		{
			if ((ntb - ZC_Time[i]) > (NTB_FRE_PER_MS * 240)
              &&(ZC_Time[i] - ntb) > (NTB_FRE_PER_MS * 240)) //过零丢失
			{
				memset(&ZC_NTB[i * 2], 0, sizeof(phaseRecognizeType));
				memset(&ZC_NTB[i * 2 + 1], 0, sizeof(phaseRecognizeType));
				ZC_Time[i] = 0;
			}
		}
	}
	if ((0 == ZC_Time[0]) && (0 == ZC_Time[1]) && (0 == ZC_Time[2]))
	{
		return OK;
	}
//#ifndef HPLC_CSG	
	//u8 frequence = FrequenceGet();
	//BPLC_SetTxGain(HPLC_LinePower[frequence]);
//#endif
	if (!g_pRelayStatus->online_num)
	{
		return OK;
	}
	if (start_zc_tick == 0)
	{
		if (g_pRelayStatus->cfged_meter_num==3)//测试台区识别时用三个sta
		{
			start_zc_tick = tick_now + (60 * 3 * OS_CFG_TICK_RATE_HZ); //第一次达到60%后等待时间
		}
		else
		{
			start_zc_tick = tick_now + OS_CFG_TICK_RATE_HZ * 10;
		}

		return OK;
	}
	else
	{
		if (tick_now < start_zc_tick)
		{
			if (online_num != 0 //判断online_num是否为0  第一次的2分钟是必须等待的
				&& online_num < g_pRelayStatus->online_num) //新增一个sta等待12s后开始进行识别
			{
				start_zc_tick = tick_now + 12 * OS_CFG_TICK_RATE_HZ;
				online_num = g_pRelayStatus->online_num;
			}
			return OK;
		}
	}

	online_num = g_pRelayStatus->online_num;
	static u8 phase_status = 0; //0 send 1 wait report 2 wait next
	static u8 waitTime = 20;
	static u16 tei;
	#ifdef NOT_BROAD_GET_EDGE
	static u16 broadTime = 3;
	#else
	static u16 broadTime = 0;
	#endif
	u16 find_node_num = 0;
	if (phase_status == 0)
	{
		tei = TEI_BROADCAST;

		pcb->tick_last_send = tick_now;
		for (int i = TEI_STA_MIN; i <= TEI_STA_MAX; i++)
		{
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(i);
			if (NULL == pSta)
				continue;
			if (pSta->net_state!=NET_IN_NET)
			{
				continue;
			}
			P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(i);
			if (pStaStatus->get_area_phase)
			{
#ifdef HPLC_CSG
				if ((pSta->phy_phase&0x3f)!=0)//南网支持新的识别命令的固定使用新的识别命令来识别
#endif
				{
				continue;
				}
			}
#if 0
//国网单三相  南网三项台区识别必须得到一次,不管如何得到的相位信息
			if (0 != (pSta->phy_phase & 0x3f)
				&& (!(pSta->phy_phase & (1 << 7))))
			{
				continue;
			}
#endif
			if (TicksBetween64(tick_now, pStaStatus->tick_innet) <= 10 * OSCfg_TickRate_Hz)
			{
				continue;
			}
			if (1 == pStaStatus->zc_read_node_area)
				continue;

//				if (pStaStatus->zc_read_counter >= 4)
//					continue;



			if (TicksBetween64(tick_now, pStaStatus->tick_innet) <= 10 * OSCfg_TickRate_Hz)
				continue;

			if (!find_node_num) //第一次找到
			{
				pStaStatus->zc_read_node_area = 1;
//					pStaStatus->zc_read_counter++;
				tei = i;
			}
			find_node_num++;
		}

		if (tei > TEI_STA_MAX)
		{
			for (tei = TEI_STA_MIN; tei <= TEI_STA_MAX; tei++)
			{
//					P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
				pStaStatus->zc_read_node_area = 0;

			}
			phase_status = 0;
			start_zc_tick = tick_now + (60 * 2 * OS_CFG_TICK_RATE_HZ); //一轮查询后等待2分钟再次查询
			return OK;
		}
		if (find_node_num == 0)
		{
			phase_status = 0;
			return OK;
		}
		if ((find_node_num * 10 / g_pRelayStatus->online_num) > 4)
		{
			if (broadTime < 3) //前三次广播
			{
				P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
				if (pStaStatus)
				{
					pStaStatus->zc_read_node_area = 0;
				}
				zeroBroadTime++;
				tei = TEI_BROADCAST;
			}
			#ifndef NOT_BROAD_GET_EDGE
			broadTime++;
			if (broadTime > 500) //每500次允许3次广播获取
			{
				broadTime = 0;
			}
			#endif
		}
		if (tei == TEI_BROADCAST)
		{
				#ifdef HPLC_CSG
			waitTime = 10;
				#else
			waitTime = 8;
				#endif
		}
		else
		{
			#ifdef APP_PHASE_SEND_START
			waitTime = 3;
			#else
			waitTime = 1;
			#endif
		}
		pcb->start_ntb = BPLC_GetNTB();
#ifdef HPLC_CSG
		PLC_SendAppPhaseDiscernment(tei);
#else
	#ifdef APP_PHASE_SEND_START
		//台区识别开启时，不发送台区采集启动
		if (areaPcb->allow == 0)
		{
			PLC_SendAreaDiscernmentStart(tei, BPLC_GetNTB()+NTB_FRE_PER_SEC*2, ++areaPcb->collection_seq);
		}
	#else
		PLC_SendAreaDiscernmentCollect(tei);//国网直接询问边沿
	#endif
#endif
		phase_status = 1;

	}
#ifdef APP_PHASE_SEND_START
	else if (phase_status == 1) //等待启动命令全部发送完成
	{
		if (TicksBetween64(tick_now, pcb->tick_last_send) >= waitTime * OSCfg_TickRate_Hz)
		{
			pcb->tick_last_send = tick_now;
			phase_status = 2;
		}
	}

	else if (phase_status == 2) //发送特征收集 获取sta特征信息
	{
		PLC_SendAreaDiscernmentCollect(tei);

		pcb->tick_last_send = tick_now;
		phase_status = 3;
	}
#endif
	else
	{
		if (TicksBetween64(tick_now, pcb->tick_last_send) >= waitTime * OSCfg_TickRate_Hz)
		{
			phase_status = 0; //等待下一次发送
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
			P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
			if (pSta != NULL && pSta->phy_phase == 0)
			{
#ifdef HPLC_CSG
				if (!pStaStatus->get_area_phase) //南网只有不支持新台区识别的才使用过零识别
#endif
				{
					PLC_GetStaStatusInfoByTei(tei)->first_unkonw_phase = 1;
				}
			}
		}
	}
	return OK;
}

u8 PLC_IsFoundBeaconExisted(u16 tei, u16 *pFoundBeaconSta, u16 num)
{
	for (u16 i = 0; i < num; i++)
	{
		if (tei == pFoundBeaconSta[i])
			return TRUE;
	}

	return FALSE;
}
#ifdef HPLC_CSG
u8 PLC_SetEventSwitchProc(OS_TICK_64 tick_now)
{
	PLC_EVENT_SWITCH_CB *pcb = &g_pRelayStatus->event_switch_cb;
	switch (pcb->state)
	{
	case 0://find tei
		for (int i=0;i<MAX_BITMAP_SIZE/4;i++)
		{
			if (pcb->bitmap[i])
			{
				for (int j=0;j<32;j++)
				{
					if (pcb->bitmap[i]&(1<<j))
					{
						pcb->tei = (i << 5) | j;
						if (pcb->tei < TEI_MAX)
						{
							if (pcb->tei < TEI_STA_MIN)
							{
								pcb->tei = TEI_BROADCAST;
							}
							else
							{
								P_STA_INFO psta = PLC_GetValidStaInfoByTei(pcb->tei);
								if (psta == NULL)
								{
									pcb->bitmap[i] &= ~(1 << j);
									continue;
								}
								if (psta->net_state != NET_IN_NET)
								{
									continue;
								}
							}
							pcb->state = 1; //send to plc
							pcb->remainTime = 3;
							++pcb->seq;
							return OK;
						}
						else
						{
							pcb->bitmap[i] &= ~(1 << j);
						}

					}
					
				}
			}
		}
		break;
	case 1://send
		PLC_SendEventSwitch(pcb->tei, pcb->seq, g_PlmStatusPara.report_meter_event);
		if (pcb->tei==TEI_BROADCAST)
		{
			pcb->tick_next = tick_now + OS_CFG_TICK_RATE_HZ * 10;
		}
		else
		{
			pcb->tick_next = tick_now + OS_CFG_TICK_RATE_HZ * 5;
		}
		pcb->state=2;//wait
		--pcb->remainTime;
		break;
	case 2://wait
		if (tick_now>pcb->tick_next)
		{
			if (pcb->tei != TEI_BROADCAST)
			{
				if (pcb->bitmap[pcb->tei >> 5] & (1 << pcb->tei & 0x1f))
				{
					pcb->state = 0;
					break;
				}
			}
			if (pcb->remainTime)
			{
				pcb->state = 1; //send
			}
			else
			{
				pcb->state = 3; //clear
			}

		}
		break;
	case 3: //clear bitmap
		if (pcb->tei == TEI_BROADCAST)
		{
			pcb->bitmap[0]&=~0x3;
		}
		else
		{
			pcb->bitmap[pcb->tei>>5]&=~(1<<pcb->tei);
		}
		pcb->state=0;
		break;
	}
	return OK;
}
#endif
//国网读取ID信息
#ifndef HPLC_CSG
unsigned char PLC_SendAskModuleID(u16 tei)
{
	P_APP_PACKET pAppFrame = NULL;

	PLC_GET_STAEDGE_CB *pcb = &g_pRelayStatus->get_phase_edge_cb;
	USHORT msdu_len = sizeof(APP_PACKET) - 1 + sizeof(READ_ID);
	pAppFrame = HplcApp_GetModuleIDFrame(pcb->seq++);

	if (NULL == pAppFrame)
		goto L_Error;


	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;

	if (SOF_param.DstTei == TEI_BROADCAST)
	{
		goto L_Error;
	}
	else
	{
		SOF_param.BroadcastFlag = 0;
		mac_param.DstTei = tei;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.SendLimit = 3;
		memcpy(mac_param.DstMac, PLC_GetValidStaInfoByTei(tei)->mac, MAC_ADDR_LEN);
	}
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_AREA_DISCERNMENT;

	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.mac_flag = MACFLAG_NONE;
	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}
bool PLC_AskNewStaID(u16 tei)
{
	PLC_GET_MOUDLE_ID_CB* pcb = &g_pRelayStatus->get_moudle_id_cb;
	if (pcb->state == 2)
	{
		if ( pcb->tei > TEI_STA_MAX) //等待状态
		{
			pcb->tei=tei;
			pcb->retry=0;
			pcb->state=2;
			pcb->tick_next=GetTickCount64()+10LL*OS_CFG_TICK_RATE_HZ+(rand()%(10*OS_CFG_TICK_RATE_HZ));
			return true;
		}
		else if(pcb->tei<TEI_STA_MAX)
		{
			if(pcb->tei>tei)
			{
				pcb->tei = tei;
				pcb->retry = 0;
				pcb->state = 2;
				pcb->tick_next = GetTickCount64()+10LL*OS_CFG_TICK_RATE_HZ+(rand()%(10*OS_CFG_TICK_RATE_HZ));
			}
			
		}
	}
	return false;
}
u8 PLC_GetStaGetModuleIdProc(OS_TICK_64 tick_now)
{
	static u16 last_tei = 0;
	PLC_GET_MOUDLE_ID_CB* pcb = &g_pRelayStatus->get_moudle_id_cb;
	
	if (0 == pcb->state)
	{
		if (pcb->tei<TEI_STA_MIN||pcb->tei>TEI_STA_MAX)
		{
			pcb->tei=TEI_STA_MIN;
			pcb->retry=0;
		}
		for (; pcb->tei <= TEI_STA_MAX; pcb->tei++)
		{
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pcb->tei);
			if (NULL == pSta)
				continue;
			if (pSta->net_state!=NET_IN_NET)
			{
				continue;
			}
			if (!pSta->geted_id)
			{
				//新tei，重传次数重新置0
				if(last_tei != pcb->tei)
				{
					last_tei = pcb->tei;
					pcb->retry = 0;
				}
				
				//send ask
				PLC_SendAskModuleID(pcb->tei);
				pcb->tick_next=tick_now+OS_CFG_TICK_RATE_HZ*5;
				pcb->retry++;
				pcb->state = 1;
				if (pcb->retry>3)
				{
					pcb->retry=0;
					pcb->tei++;
				}
				return OK;
			}
		}
		pcb->state = 2;
		pcb->tick_next = tick_now + OS_CFG_TICK_RATE_HZ * 60*10;
		return OK;
	}
	else if (pcb->state==1)
	{
		if (tick_now>pcb->tick_next) //10秒重传
		{
			pcb->state=0;
		}
		return OK;
	}
	else if (pcb->state==2)
	{
		if (tick_now>pcb->tick_next) 
		{
			pcb->state=0;
		}
		return OK;
	}
	return OK;

#if 0
	PLC_GET_MOUDLE_ID_CB* pcb = &g_pRelayStatus->get_moudle_id_cb;
	static OS_TICK_64 tick_next;
	if (0 == pcb->state)
	{
		if (pcb->tei<TEI_STA_MIN||pcb->tei>TEI_STA_MAX)
		{
			return OK;
		}

		P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pcb->tei);
		if (NULL == pSta)
		{
			ReportIDMsg(NULL, 0);
			pcb->retry=0;
			pcb->tei=0;
		}
		if (!pSta->geted_id)
		{
			//send ask
			PLC_SendAskModuleID(pcb->tei);
			pcb->state=1;
			tick_next=tick_now+OS_CFG_TICK_RATE_HZ*5;
			pcb->retry++;
			pcb->state = 1;
			return OK;
		}
		return OK;
	}
	else if (pcb->state==1)
	{
		if (tick_now>tick_next) //10秒重传
		{
			pcb->state=0;
			if (pcb->retry>3)
			{
				ReportIDMsg(NULL, 0);
				pcb->retry=0;
				pcb->tei=0;
			}
		}
		return OK;
	}
	return OK;
#endif
}

u8 PLC_GetStaGetModuleIdRealTimeProc(OS_TICK_64 tick_now)
{
    PLC_GET_MOUDLE_ID_CB* pcb = &g_pRelayStatus->get_moudle_id_realtime_cb;

	if (EM_TASK_READ_INIT == pcb->state)
	{
		pcb->plc_send_counter = 1;
		pcb->plc_send_num = 0;
        pcb->seq = 0xFFFF;
		return (OK);
	}
	else if (EM_TASK_READ_SEND == pcb->state)
	{
        PLC_SendAskModuleID(pcb->tei);

		pcb->tick_send = tick_now;
		pcb->tick_timeout = PLC_GetReadMeterTimeoutTick(0);
		pcb->plc_send_num++;
		pcb->state = EM_TASK_READ_WAIT_PLC;

		return (!OK);
	}
	else if (EM_TASK_READ_WAIT_PLC == pcb->state)
	{
		if (NULL != pcb->pPlcRsp)
		{
			pcb->state = EM_TASK_READ_POST;
			return (!OK);
		}
        
		if (TicksBetween64(tick_now, pcb->tick_send) >= pcb->tick_timeout)
		{
			if (pcb->plc_send_counter * MAX_MINOR_LOOP_READ_METER > pcb->plc_send_num)
			{
				pcb->state = EM_TASK_READ_SEND;
			}
			else
			{
				pcb->state = EM_TASK_READ_POST;
			}
		}

		return (OK);
	}
	else if (EM_TASK_READ_POST == pcb->state)
	{
		P_PLC_RSP_MSG_INFO pPlcRsp = pcb->pPlcRsp;

		if (NULL != pPlcRsp)
		{	
		    PLM_ResponseStaModuleID(pcb->mac_addr, pcb->retry, pcb->dev_type, pcb->seq, pPlcRsp->body);
		}
		else
        {   
            PLM_ResponseStaModuleID(pcb->mac_addr, pcb->retry, pcb->dev_type, pcb->seq, NULL);
        }	

		pcb->tick_post = tick_now;
		pcb->state = EM_TASK_READ_WAIT_POST;

		return (!OK);
	}
	else if (EM_TASK_READ_WAIT_POST == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->tick_post) >= OSCfg_TickRate_Hz/5)
        {      
			pcb->state = EM_TASK_READ_EXIT;
        }
		return (OK);
	}
	else if (EM_TASK_READ_EXIT == pcb->state)
	{
		free_buffer(__func__, pcb->pPlcRsp);
		pcb->pPlcRsp = NULL;
		pcb->state = EM_TASK_READ_INIT;
		return OK;
	}

	return OK;
}

#endif
u8 PLC_NettingProc(OS_TICK_64 tick_now)
{

#if 0//defined(NETTING_LEVEL_BY_LEVEL)

	if(g_pRelayStatus->search_level<1 || g_pRelayStatus->search_level>MAX_RELAY_DEEP)
	{
		g_pRelayStatus->search_level = 1;
		goto L_SearchInitParam;
	}

	if((g_pRelayStatus->search_level>=2) && (g_pRelayStatus->innet_num_level[g_pRelayStatus->search_level-1] == 0))
	{
		g_pRelayStatus->search_level = 1;
		goto L_SearchInitParam;
	}

	OS_TICK_64 tick_level_timeout, tick_assoc_req_timeout;

	if(g_pRelayStatus->cfged_meter_num < MAX_NODE_COUNT_PROTOCOL_TEST)
	{
		//协议一致性测试
		tick_level_timeout = 1*OSCfg_TickRate_Hz;
		tick_assoc_req_timeout = 1*OSCfg_TickRate_Hz;
	}
	else
	{
		//互操作性测试
		//tick_level_timeout = 90*OSCfg_TickRate_Hz;
		tick_level_timeout = 2*OSCfg_TickRate_Hz*g_pRelayStatus->sta_num_level[g_pRelayStatus->search_level];
		tick_assoc_req_timeout = 20*OSCfg_TickRate_Hz;

		//u16 outnet_num = 10;
		//if(g_pRelayStatus->all_meter_num > g_pRelayStatus->innet_num)
		//outnet_num = g_pRelayStatus->all_meter_num - g_pRelayStatus->innet_num;

		if(tick_level_timeout < 30*OSCfg_TickRate_Hz)
		tick_level_timeout = 30*OSCfg_TickRate_Hz;
		if(tick_level_timeout > 60*OSCfg_TickRate_Hz)
		tick_level_timeout = 60*OSCfg_TickRate_Hz;
	}

	if(TicksBetween64(tick_now, g_pRelayStatus->search_start) < tick_level_timeout)       //28  90        //太晚安排发现信标和代理信标，协议一致性测试通不过
	return OK;

	if(TicksBetween64(tick_now, g_pRelayStatus->search_level_last_assoc_req) < tick_assoc_req_timeout)        //20  40
	return OK;

	if(!g_pRelayStatus->search_found_beacon_complete)
	return OK;

//L_NextLevel:

	//下一级组网
	if(0==g_pRelayStatus->innet_num_level[g_pRelayStatus->search_level])
	{
		g_pRelayStatus->search_level = 1;
		goto L_SearchInitParam;
	}

	g_pRelayStatus->search_level++;
	if(g_pRelayStatus->search_level>MAX_RELAY_DEEP)
	{
		g_pRelayStatus->search_level = 1;
		goto L_SearchInitParam;
	}

	L_SearchInitParam:
	{
		u16 max_found_beacon_per_time = MAX_RELAY_PER_LEVEL;

		u16* pFoundBeaconSta = (u16*)alloc_buffer(__func__,max_found_beacon_per_time*2);
		if(NULL == pFoundBeaconSta)
		return OK;

		OS_ERR err;
		OSSchedLock(&err);

		u16 found_beacon_counter = 0;

		for(u8 c=0; c<2; c++)
		{

			for(u16 tei=TEI_STA_MIN; tei<=TEI_STA_MAX; tei++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if(NULL == pSta)
				continue;
				P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
				pStaStatus->search_beacon_cnt = 0;
				pStaStatus->search_need_found_beacon = 0;

				if(0 != pStaStatus->search_beacon_flag)
				continue;

//				if(ROLES_STA != pSta->roles)
				if (0 != pSta->child)
				continue;

				u8 level = PLC_GetStaLevel(tei);
				if(g_pRelayStatus->search_level != (level+1))
				continue;

				if(found_beacon_counter < max_found_beacon_per_time)
				{
					if(!PLC_IsFoundBeaconExisted(tei, pFoundBeaconSta, found_beacon_counter))
					pFoundBeaconSta[found_beacon_counter++] = tei;
				}
				else if(2 == g_pRelayStatus->search_level)
				{
					//优先用信道质量好的一级STA学习二级STA
					//待优化: 学习二级以上的STA，可以优先使用通讯成功率高的学习
					u8 min_snr = 0xff;
					u16 min_idx = 0xff;
					for(u16 i=0; i<found_beacon_counter; i++)
					{
						P_STA_INFO pBeaconSta = PLC_GetValidStaInfoByTei(pFoundBeaconSta[i]);
						if((NULL != pBeaconSta) && (pBeaconSta->snr < min_snr))
						{
							min_snr = pBeaconSta->snr;
							min_idx = i;
						}
					}
					if(min_snr < pSta->snr)
					{
						pFoundBeaconSta[min_idx] = tei;
					}
				}
			}

			if(found_beacon_counter >= max_found_beacon_per_time)
			break;
			if(1==c)
			break;

			//清掉, 重新找 或 继续添加
			for(u16 tei=TEI_STA_MIN; tei<=TEI_STA_MAX; tei++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if(NULL == pSta)
				continue;
				if (0 != pSta->child)
//				if(ROLES_STA != pSta->roles)
				continue;

				u8 level = PLC_GetStaLevel(tei);
				if(g_pRelayStatus->search_level != (level+1))
				continue;

				if(PLC_IsFoundBeaconExisted(tei, pFoundBeaconSta, found_beacon_counter))
				continue;

				P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei(tei);
				pStaStatus->search_beacon_flag = 0;
			}
		}

		char* print_buffer = (char*)alloc_buffer(__func__,5*max_found_beacon_per_time + 1);

		if(NULL != print_buffer)
		memset_special(print_buffer, 0, 5*max_found_beacon_per_time + 1);

		u32 ii = 0;
		for(u16 i=0; i<found_beacon_counter; i++)
		{
			P_STA_STATUS_INFO pBeaconStaStatus = PLC_GetStaStatusInfoByTei(pFoundBeaconSta[i]);
			pBeaconStaStatus->search_need_found_beacon = 1;
			pBeaconStaStatus->search_beacon_flag = 1;
			if(NULL != print_buffer)
			{
				ii = strlen(print_buffer);
				sprintf(&print_buffer[ii], "%d,", pFoundBeaconSta[i]);
			}
		}

		if(NULL != print_buffer)
		{
			//debug_str("开始 %d 级组网 : %s \r\n", g_pRelayStatus->search_level, print_buffer);
		}
		else
		{
			//debug_str("开始 %d 级组网\r\n", g_pRelayStatus->search_level);
		}

		free_buffer(__func__,print_buffer);

		OSSchedUnlock(&err);

		g_pRelayStatus->search_found_beacon_complete = FALSE;
		g_pRelayStatus->search_start = tick_now;
		g_pRelayStatus->search_level_last_assoc_req = tick_now;

		free_buffer(__func__,pFoundBeaconSta);

		return OK;

	}
#else

	return OK;

#endif
}
#ifdef GDW_2019_GRAPH_STOREDATA
unsigned char PLC_SendAskPhaseEdge(u16 tei)
{
	P_APP_PACKET pAppFrame = NULL;

	PLC_GET_STAEDGE_CB *pcb = &g_pRelayStatus->get_phase_edge_cb;
	USHORT msdu_len = sizeof(APP_PACKET) - 1 + sizeof(GET_STA_EDGE_DOWN);
	P_STA_INFO p_sta=PLC_GetValidStaInfoByTei(tei);
	if (NULL == p_sta)
		goto L_Error;

	pAppFrame = HplcApp_GetStaEdgeFrame(pcb->seq++, g_PlmConfigPara.main_node_addr, p_sta->mac);

	if (NULL == pAppFrame)
		goto L_Error;


	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;

	if (SOF_param.DstTei == TEI_BROADCAST)
	{
		goto L_Error;
	}
	else
	{
		SOF_param.BroadcastFlag = 0;
		mac_param.DstTei = tei;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.SendLimit = 3;
	}
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_AREA_DISCERNMENT;

	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.mac_flag = MACFLAG_NONE;
	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, p_sta->mac, MAC_ADDR_LEN);

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}
void PLC_AskNewSta(u16 tei)
{
	PLC_GET_STAEDGE_CB* pcb = &g_pRelayStatus->get_phase_edge_cb;
	if (pcb->state == 2)
	{
		if ( pcb->tei > TEI_STA_MAX) //等待状态
		{
			pcb->tei=tei;
			pcb->retry=0;
			pcb->state=2;
			pcb->tick_next=GetTickCount64()+10LL*OS_CFG_TICK_RATE_HZ+(rand()%(10*OS_CFG_TICK_RATE_HZ));
		}
		else if(pcb->tei<TEI_STA_MAX)
		{
			if(pcb->tei>tei)
			{
				pcb->tei=tei;
				pcb->retry = 0;
				pcb->state = 2;
				pcb->tick_next = GetTickCount64()+10LL*OS_CFG_TICK_RATE_HZ+(rand()%(10*OS_CFG_TICK_RATE_HZ));
			}
			
		}
	}
}
//陕西获取hplc边沿信息
u8 PLC_GetStaPhaeEdgeProc(OS_TICK_64 tick_now)
{
	PLC_GET_STAEDGE_CB* pcb = &g_pRelayStatus->get_phase_edge_cb;
	if (0 == pcb->state)
	{
		if (pcb->tei<TEI_STA_MIN||pcb->tei>TEI_STA_MAX)
		{
			pcb->tei=TEI_STA_MIN;
		}
		for (; pcb->tei <= TEI_STA_MAX; pcb->tei++)
		{
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pcb->tei);
			if (NULL == pSta)
				continue;
			if(pSta->net_state!=NET_IN_NET)
			{
				continue;
			}
			if (!pSta->phase_edge_ask)
			{
				//send ask
				PLC_SendAskPhaseEdge(pcb->tei);
				pcb->state=1;
				pcb->tick_next=tick_now+OS_CFG_TICK_RATE_HZ*5;
				pcb->retry++;
				pcb->state = 1;
				if (pcb->retry>3)
				{
					pcb->retry=0;
					pcb->tei++;
				}
				return OK;
			}
		}
		pcb->state = 2;
		pcb->tick_next = tick_now + OS_CFG_TICK_RATE_HZ * 60*10;
		return OK;
	}
	else if (pcb->state==1)
	{
		if (tick_now>pcb->tick_next) //10秒重传
		{
			pcb->state=0;
		}
		return OK;
	}
	else if (pcb->state==2)
	{
		if (tick_now>pcb->tick_next) 
		{
			pcb->state=0;
		}
		return OK;
	}
	return OK;
}
#endif

#ifdef GDW_JIANGSU
u16 routerReqeustCenterRTCTimerId = INVAILD_TMR_ID;
u16 routerSendRTCTimerId = INVAILD_TMR_ID;
u16 routerResendRTCTimerId = INVAILD_TMR_ID;
u16 routerNewStaSendRTCTimerId = INVAILD_TMR_ID;
u8 routerSendRTCCount = 3;


void PLC_NewStaSendRTCTime(void *arg)
{
	P_APP_PACKET pAppFrame = NULL;
	u16 msdu_len;
	//P_SYNC_RTC_DOWN pTimingFrame;
	local_tm now;
	OS_TICK_64 innet_tick;
	OS_TICK_64 tick_now=GetTickCount64();
    CPU_NTB ntb_now = BPLC_GetNTB();
#if 1
	u8 data[6];
	if (!PLC_get_system_time(&now))
		goto L_Error;
	memset(&data, 0, sizeof(data));
	data[0] = Hex2BcdChar(now.tm_sec); //sec
	data[1] = Hex2BcdChar(now.tm_min); //minute
	data[2] = Hex2BcdChar(now.tm_hour); //hour
	data[3] = Hex2BcdChar(now.tm_mday); //day
	data[4] = Hex2BcdChar(now.tm_mon+1); //month
	data[5] = Hex2BcdChar(now.tm_year % 100); //year
#else
	P_GD_DL645_BODY p645=NULL;
	u8 data[18] = { 0x68, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x68 };
	if (!PLC_get_system_time(&now))
		goto L_Error;
	p645 = (P_GD_DL645_BODY)data;
	p645->dataBuf[0] = Hex2BcdChar(now.tm_sec);
	p645->dataBuf[1] = Hex2BcdChar(now.tm_min);
	p645->dataBuf[2] = Hex2BcdChar(now.tm_hour);
	p645->dataBuf[3] = Hex2BcdChar(now.tm_mday);
	p645->dataBuf[4] = Hex2BcdChar(now.tm_mon);
	p645->dataBuf[5] = Hex2BcdChar(now.tm_year % 100);
	PLC_make_frame_645(p645, &data[1], 0x08, p645->dataBuf, 6);

#endif	
	pAppFrame = HplcApp_MakeSyncRTCFrame((u8 *)&data, sizeof(data), ntb_now);
	if (NULL == pAppFrame)
		goto L_Error;

	//pTimingFrame = (P_SYNC_RTC_DOWN)pAppFrame->Data;
	msdu_len = sizeof(APP_PACKET) - 1 + sizeof(SYNC_RTC_DOWN) ;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	for (u16 tei = TEI_STA_MIN; tei <= TEI_STA_MAX; tei++)
	{
		 innet_tick = PLC_GetStaInNetTick(tei);

		if (innet_tick == 0 || tick_now > innet_tick + 30*60*1000)
			continue;
		
		send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);

		PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
		send_param.doubleSend=1;

		send_param.resend_count = 2;
		send_param.send_type = CSMA_SEND_TYPE_CSMA;
		send_param.cb = NULL;

		if (SOF_param.DstTei == TEI_BROADCAST)
		{
			send_param.link=FreamPathBoth;
			SOF_param.BroadcastFlag = 1;
			mac_param.DstTei = TEI_BROADCAST;
			mac_param.SendType = SENDTYPE_PCOBROADCAST;
			mac_param.SendLimit = 4;
			send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
		}
		else
		{
			SOF_param.BroadcastFlag = 0;
			mac_param.DstTei = tei;
			mac_param.SendType = SENDTYPE_SINGLE;
			mac_param.SendLimit = 3;
		}
		SOF_param.NID = g_pPlmConfigPara->net_id;
		SOF_param.Lid = SOF_LID_TIMING;

		mac_param.HopCount = MAX_RELAY_DEEP;
		mac_param.MsduType = MSDU_APP_FRAME;
		mac_param.Broadcast = BROADCAST_DRT_DUL;
		mac_param.NetSeq = g_pPlmConfigPara->net_seq;

		mac_param.mac_flag = MACFLAG_NONE;
		PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);

		memcpy_special(mac_param.DstMac, PLC_GetValidStaInfoByTei(tei)->mac, MAC_ADDR_LEN);

		HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);
		}

	free_buffer(__func__,pAppFrame);

	return ;

L_Error:
	free_buffer(__func__,pAppFrame);
	return;
}

u8 PLC_RouterSendRTCBcastTime(void)
{
	P_APP_PACKET pAppFrame = NULL;
	u16 msdu_len;
	local_tm now;
	
	CPU_NTB ntb_now = BPLC_GetNTB();
	
#if 1
	u8 data[6];
	if (!PLC_get_system_time(&now))
		goto L_Error;
	memset(&data, 0, sizeof(data));
	data[0] = Hex2BcdChar(now.tm_sec); //sec
	data[1] = Hex2BcdChar(now.tm_min); //minute
	data[2] = Hex2BcdChar(now.tm_hour); //hour
	data[3] = Hex2BcdChar(now.tm_mday); //day
	data[4] = Hex2BcdChar(now.tm_mon+1); //month
	data[5] = Hex2BcdChar(now.tm_year % 100); //year
#else
	P_GD_DL645_BODY p645=NULL;
	u8 data[18] = { 0x68, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x68 };
	if (!PLC_get_system_time(&now))
		goto L_Error;
	p645 = (P_GD_DL645_BODY)data;
	p645->dataBuf[0] = Hex2BcdChar(now.tm_sec);
	p645->dataBuf[1] = Hex2BcdChar(now.tm_min);
	p645->dataBuf[2] = Hex2BcdChar(now.tm_hour);
	p645->dataBuf[3] = Hex2BcdChar(now.tm_mday);
	p645->dataBuf[4] = Hex2BcdChar(now.tm_mon);
	p645->dataBuf[5] = Hex2BcdChar(now.tm_year % 100);
	PLC_make_frame_645(p645, &data[1], 0x08, p645->dataBuf, 6);

#endif	
	pAppFrame = HplcApp_MakeSyncRTCFrame((u8 *)&data, sizeof(data), ntb_now);
	if (NULL == pAppFrame)
		goto L_Error;

	msdu_len = sizeof(APP_PACKET) - 1 + sizeof(SYNC_RTC_DOWN) ;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif


	send_param.resend_count = 4;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	send_param.link = FreamPathBoth;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_TIMING;
	SOF_param.DstTei = TEI_BROADCAST;

	mac_param.DstTei = TEI_BROADCAST;
	mac_param.SendType = SENDTYPE_PCOBROADCAST;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);
	return (!OK);
}

void routerSendRTCTime(void *arg)
{

	PLC_RouterSendRTCBcastTime();

	if(routerSendRTCCount)
	{
		//1min广播一次，共3次
		routerSendRTCCount--;
		routerResendRTCTimerId = HPLC_RegisterTmr(routerSendRTCTime, NULL, 60*OSCfg_TickRate_Hz, TMR_OPT_CALL_ONCE);
	}
	else 
	{
		routerSendRTCCount = 3;
	}
}
#endif


#ifdef GDW_2019_GRAPH_STOREDATA
u16 storeDataReqeustTimerId = INVAILD_TMR_ID;
u8 storeDateRequestCount = 3;

#ifdef GDW_ZHEJIANG
u16 OnlineLockTimeTimerId = INVAILD_TMR_ID;
u8 OnlineLockTimeCount = 3;
#endif

void StoreDateRequestTime(void *arg)
{
//	OS_ERR err;
	u32 time_period = g_PlmStatusPara.store_data_param.bcast_time_period;
	
//	OSSemSet(&g_pOsRes->savedata_mbox, 0, &err);
	PLM_RequestCentreTime();
//	OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz/2, OS_OPT_PEND_BLOCKING, NULL, &err);

	if(storeDateRequestCount)
	{
		//10秒重新向集中器请求，共请求3次
		storeDateRequestCount--;
		storeDataReqeustTimerId = HPLC_RegisterTmr(StoreDateRequestTime, NULL, 10*OSCfg_TickRate_Hz, TMR_OPT_CALL_ONCE);
	}
	else //重发次数归0后没有收到路由请求集中器回复，重置周期性请求定时器
	{
		storeDateRequestCount = 3;
		if (time_period==0)
		{
			time_period=10;
		}
		storeDataReqeustTimerId = HPLC_RegisterTmr(StoreDateRequestTime, NULL, time_period * OSCfg_TickRate_Hz, TMR_OPT_CALL_ONCE);
	}
}

u8 PLC_SendStoreDateBcastTime(void)
{
	P_APP_PACKET pAppFrame = NULL;
	u16 msdu_len;
	P_STORE_DATA_BCAST_TIMING_DOWN pTimingFrame;
	local_tm now;
	
	
	
#if 1
	u8 data[6];
	if (!PLC_get_system_time(&now))
		goto L_Error;
	memset(&data, 0, sizeof(data));
	data[0] = Hex2BcdChar(now.tm_sec); //sec
	data[1] = Hex2BcdChar(now.tm_min); //minute
	data[2] = Hex2BcdChar(now.tm_hour); //hour
	data[3] = Hex2BcdChar(now.tm_mday); //day
	data[4] = Hex2BcdChar(now.tm_mon+1); //month
	data[5] = Hex2BcdChar(now.tm_year % 100); //year
#else
	P_GD_DL645_BODY p645=NULL;
	u8 data[18] = { 0x68, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x68 };
	if (!PLC_get_system_time(&now))
		goto L_Error;
	p645 = (P_GD_DL645_BODY)data;
	p645->dataBuf[0] = Hex2BcdChar(now.tm_sec);
	p645->dataBuf[1] = Hex2BcdChar(now.tm_min);
	p645->dataBuf[2] = Hex2BcdChar(now.tm_hour);
	p645->dataBuf[3] = Hex2BcdChar(now.tm_mday);
	p645->dataBuf[4] = Hex2BcdChar(now.tm_mon);
	p645->dataBuf[5] = Hex2BcdChar(now.tm_year % 100);
	PLC_make_frame_645(p645, &data[1], 0x08, p645->dataBuf, 6);

#endif	
	pAppFrame = HplcApp_MakeStoreDataBcastTimingFrame((u8 *)&data, sizeof(data));
	if (NULL == pAppFrame)
		goto L_Error;

	pTimingFrame = (P_STORE_DATA_BCAST_TIMING_DOWN)pAppFrame->Data;
	msdu_len = sizeof(APP_PACKET) - 1 + sizeof(STORE_DATA_BCAST_TIMING_DOWN) + pTimingFrame->DataLen;

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif

	send_param.resend_count = 4;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	send_param.link = FreamPathBoth;
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.BroadcastFlag = 1;
	SOF_param.Lid = SOF_LID_TIMING;
	SOF_param.DstTei = TEI_BROADCAST;

	mac_param.DstTei = TEI_BROADCAST;
	mac_param.SendType = SENDTYPE_PCOBROADCAST;
	mac_param.SendLimit = 3;
	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;
	mac_param.mac_flag = MACFLAG_NONE;

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);
	return (!OK);
}

u8 PLC_StoreDataBcastTime(void)
{
	return PLC_PostMessage(MSG_STORE_DATA_BCAST_TIME, 0, 0);
}
void PLC_NewStaJoinTimeDelay()
{
	P_PLC_STORE_DATA_BCAST_CB pcb = &g_pRelayStatus->store_data_bcast_cb;
	pcb->next_tick_send=GetTickCount64()+(60*OS_CFG_TICK_RATE_HZ*5);
}
u8 PLC_StoreDataBcastTimeProc(OS_TICK_64 tick_now)
{
	P_PLC_STORE_DATA_BCAST_CB pcb = &g_pRelayStatus->store_data_bcast_cb;

	if (EM_STORE_DATA_BCAST_TIME_INIT == pcb->state)
	{
		if (pcb->next_tick_send != 0
			&& tick_now > pcb->next_tick_send)
		{
			if (storeDataReqeustTimerId != INVAILD_TMR_ID) //重新发送广播时间
			{
				HPLC_DelTmr(storeDataReqeustTimerId);
			}
			storeDateRequestCount = 2;
			pcb->next_tick_send=0;
			storeDataReqeustTimerId = HPLC_RegisterTmr(StoreDateRequestTime, NULL, OS_CFG_TICK_RATE_HZ, TMR_OPT_CALL_ONCE);
		}
	}
	else if (EM_STORE_DATA_BCAST_TIME_START == pcb->state)
	{
		PLC_SendStoreDateBcastTime();
		pcb->next_tick_send=0;
		pcb->tick_send = tick_now;
		if (pcb->send_counter)
			pcb->send_counter--;
		pcb->state = EM_STORE_DATA_BCAST_TIME_WAIT;
		
		return OK;
	}
	else if (EM_STORE_DATA_BCAST_TIME_WAIT == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->tick_send) >= 5*OSCfg_TickRate_Hz) //5秒重传
		{
			if (pcb->send_counter)
			{
				pcb->next_tick_send=0;
				pcb->state = EM_STORE_DATA_BCAST_TIME_START;
			}
			else
				pcb->state = EM_STORE_DATA_BCAST_TIME_INIT;

		}
		
		return OK;
	}

	return OK;
}



u8 PLC_SendStoreDataSyncCfg(u16 tei, u8 protocol,u32 seq)
{
	u16 msdu_len;
	u8 *pData = NULL;
	u8 mac_addr[MAC_ADDR_LEN];
	P_APP_PACKET pAppFrame = NULL;
	P_PLC_STORE_DATA_SYNC_CFG_CB pcb = &g_pRelayStatus->store_data_sync_cfg_cb;

	if (tei == TEI_BROADCAST)
	{
		memcpy_special(mac_addr, BroadCastMacAddr, MAC_ADDR_LEN);
	}
	else
	{
		memcpy_special(mac_addr, PLC_GetValidStaInfoByTei(tei)->mac, MAC_ADDR_LEN);
	}

	if(protocol == METER_PROTOCOL_3762_69845)
	{
		msdu_len = pcb->meter_698_data_len;
		pData = pcb->meter_698_data;
	}
	else
	{
		msdu_len = pcb->meter_645_data_len;
		pData = pcb->meter_645_data;
	}
	
	pAppFrame = HplcApp_MakeStoreDataSyncCfgFrame(seq, g_PlmConfigPara.main_node_addr, mac_addr, pData, msdu_len);
	if (NULL == pAppFrame)
	{
		goto L_Error;
	}
	msdu_len += sizeof(APP_PACKET) - 1 + sizeof(STORE_DATA_BCAST_SYNC_CFG_DOWN);

	PLC_SEND_PARAM send_param;
	SOF_PARAMS SOF_param;
	MAC_PARAM mac_param;
	#ifdef FORWARD_FRAME
	mac_param.SrcTei = CCO_TEI;
	#endif
	send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);

	PLC_GetSendParam(tei, &SOF_param.DstTei, &send_param.send_phase,&send_param.link);
	send_param.doubleSend=1;

	send_param.resend_count = 2;
	send_param.send_type = CSMA_SEND_TYPE_CSMA;
	send_param.cb = NULL;

	if (SOF_param.DstTei == TEI_BROADCAST)
	{
		if (tei!=TEI_BROADCAST)
		{
			free_buffer(__func__,pAppFrame);
			return OK;
		}
		send_param.link=FreamPathBoth;
		SOF_param.BroadcastFlag = 1;
		mac_param.DstTei = TEI_BROADCAST;
		mac_param.SendType = SENDTYPE_PCOBROADCAST;
		mac_param.SendLimit = 4;
		send_param.send_phase = (HPLC_SEND_PHASE)(SEND_PHASE_A | SEND_PHASE_B | SEND_PHASE_C);
	}
	else
	{
		SOF_param.BroadcastFlag = 0;
		mac_param.DstTei = tei;
		mac_param.SendType = SENDTYPE_SINGLE;
		mac_param.SendLimit = 3;
	}
	SOF_param.NID = g_pPlmConfigPara->net_id;
	SOF_param.Lid = SOF_LID_AREA_DISCERNMENT;

	mac_param.HopCount = MAX_RELAY_DEEP;
	mac_param.MsduType = MSDU_APP_FRAME;
	mac_param.Broadcast = BROADCAST_DRT_DUL;
	mac_param.NetSeq = g_pPlmConfigPara->net_seq;

	mac_param.mac_flag = MACFLAG_NONE;
	PLC_CopyCCOMacAddr(mac_param.SrcMac, FALSE);
	memcpy_special(mac_param.DstMac, mac_addr, MAC_ADDR_LEN);

	HPLC_MakeAndSendMacFrame(&send_param, &SOF_param, &mac_param, (u8 *)pAppFrame, msdu_len);

	free_buffer(__func__,pAppFrame);

	return OK;

L_Error:
	free_buffer(__func__,pAppFrame);

	return (!OK);
}

u8 PLC_StoreDataSyncCfgProc(OS_TICK_64 tick_now)
{
	P_PLC_STORE_DATA_SYNC_CFG_CB pcb = &g_pRelayStatus->store_data_sync_cfg_cb;
	

	if (EM_STORE_DATA_SYNC_CFG_START == pcb->state) //各协议广播3次
	{
		if (TicksBetween64(tick_now, pcb->tick_send) >= 10*OSCfg_TickRate_Hz)
		{

			if (
#if 0     //移除广播配置曲线
				pcb->send_counter < 6
#else
				0
#endif
				)
			{
				if (IsAllStaDone(pcb->sta_bitmap[0])
#ifndef HPLC_CSG
					&& IsAllStaDone(pcb->sta_bitmap[1])
#endif
				   ) //广播全部设置完成或者没有需要设置的sta
				{
					pcb->tick_send = 0;
					pcb->send_counter = 0;
					pcb->tei = TEI_STA_MIN;
					pcb->portocol = 0;
					pcb->state = EM_STORE_DATA_SYNC_CFG_INIT;
				}
				else
				{
					if (pcb->send_counter % 2) //奇数次发698.45
					{
						if (pcb->meter_698_data_len)
						{
							PLC_SendStoreDataSyncCfg(TEI_BROADCAST, METER_PROTOCOL_3762_69845, ++pcb->packet_seq_total);
#ifndef HPLC_CSG
							pcb->packet_seq[1] = pcb->packet_seq_total;
#endif
						}
					}
					else //偶数次发645-2007
					{
						if (pcb->meter_645_data_len)
						{
							PLC_SendStoreDataSyncCfg(TEI_BROADCAST, METER_PROTOCOL_3762_07, ++pcb->packet_seq_total);
#ifndef HPLC_CSG
							pcb->packet_seq[0] = pcb->packet_seq_total;
#endif
						}
					}
					
					pcb->tick_send = tick_now;
					pcb->send_counter++;
				}
			}
			else
			{
				pcb->tick_send = 0;
				pcb->send_counter = 0;
				pcb->tei = TEI_STA_MIN;
				pcb->state = EM_STORE_DATA_SYNC_CFG_SEND_UNICAST; //切换到单播设置流程
			}
		}
	}
	else if (EM_STORE_DATA_SYNC_CFG_SEND_UNICAST == pcb->state)
	{
		int tei;
		
		tei = pcb->tei;

			P_STA_INFO pSta=PLC_GetValidStaInfoByTei(tei);
			if (pSta&&pSta->net_state==NET_IN_NET)
			{
			if (pcb->sta_bitmap[pcb->portocol][tei >> 5] & (1 << (tei & 0x1f)))
				{
				if (pcb->portocol == 1)
					{
						if (pcb->meter_698_data_len)
						{
						PLC_SendStoreDataSyncCfg(tei, METER_PROTOCOL_3762_69845, ++pcb->packet_seq_total);
#ifndef HPLC_CSG
						pcb->packet_seq[1] = pcb->packet_seq_total;
#endif
						}
					}
					else
					{
						if(pcb->meter_645_data_len)
						{
						PLC_SendStoreDataSyncCfg(tei, METER_PROTOCOL_3762_07, ++pcb->packet_seq_total);
#ifndef HPLC_CSG
						pcb->packet_seq[0] = pcb->packet_seq_total;
#endif
						}
					}
					pcb->tei = tei;
					pcb->tick_send = tick_now;
					//pcb->send_counter = 0;
					pcb->state = EM_STORE_DATA_SYNC_CFG_WAIT_UNICAST;

					return OK;
				}
				else //切到下一个STA
				{
					if(pcb->restart_immediate) //从最小tei开始重新执行一轮
					{
						pcb->tei = TEI_STA_MIN-1;
						pcb->restart_immediate = 0;
					}
					
					goto L_NEXT_METER;
				}
			}
		else
		{
			if (pcb->restart_immediate) //从最小tei开始重新执行一轮
			{
				pcb->tei = TEI_STA_MIN - 1;
				pcb->restart_immediate = 0;
		}

			goto L_NEXT_METER;
	}

	}
	else if (EM_STORE_DATA_SYNC_CFG_WAIT_UNICAST == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->tick_send) >= 3*OSCfg_TickRate_Hz)
		{
			if (pcb->send_counter < 2
				&&(pcb->sta_bitmap[pcb->portocol][pcb->tei >> 5] & (1 << (pcb->tei & 0x1f)))) //每个STA单播发3次
			{
				if (pcb->portocol == 1)
				{
					if (pcb->meter_698_data_len)
					{
						PLC_SendStoreDataSyncCfg(pcb->tei, METER_PROTOCOL_3762_69845, pcb->packet_seq[1]);
					}
				}
				else
				{
					if (pcb->meter_645_data_len)
					{
						PLC_SendStoreDataSyncCfg(pcb->tei, METER_PROTOCOL_3762_07, pcb->packet_seq[0]);
					}
				}


				pcb->tick_send = tick_now;
				pcb->send_counter++;
			}
			else //切到下一个STA
			{
				if(pcb->restart_immediate) //从最小tei开始重新执行一轮
				{
					pcb->tei = TEI_STA_MIN-1;
					pcb->restart_immediate = 0;
				}

				pcb->state = EM_STORE_DATA_SYNC_CFG_SEND_UNICAST;
				goto L_NEXT_METER;
			}
		}
	}
	else if (EM_STORE_DATA_SYNC_CFG_RESTART == pcb->state)
	{
		if (TicksBetween64(tick_now, pcb->tick_send) >= 600 * OSCfg_TickRate_Hz)
		{
			pcb->tick_send = 0;
			pcb->send_counter = 0;
			pcb->state = EM_STORE_DATA_SYNC_CFG_SEND_UNICAST;
		}
	}
	
	return OK;

L_NEXT_METER:
	pcb->tei++;
	pcb->tick_send = 0;
	pcb->send_counter = 0;
	
	if (pcb->tei < TEI_STA_MIN || pcb->tei > TEI_STA_MAX)
	{
		pcb->tei = TEI_STA_MIN;
#ifndef HPLC_CSG
//南网只有1种协议pcb->portocol不增加
		pcb->portocol++;
		if (pcb->portocol > 1)
		{
			pcb->portocol = 0;
		}
#endif
		pcb->round--;
#ifdef HPLC_CSG
		//南网无需回复因此有轮次限制
		if (pcb->round == 0)
		{
			pcb->tick_send = 0;
			pcb->send_counter = 0;
			pcb->tei = TEI_STA_MIN;
			memset(pcb->sta_bitmap, 0, sizeof(pcb->sta_bitmap));
			pcb->state = EM_STORE_DATA_SYNC_CFG_INIT;
		}
		else
#endif
		{
			if (IsAllStaDone(pcb->sta_bitmap[0])
#ifndef HPLC_CSG
				&& IsAllStaDone(pcb->sta_bitmap[1])
#endif
			   ) //已经全部配置完
			{
				pcb->tick_send = 0;
				pcb->send_counter = 0;
				pcb->portocol = 0;
				pcb->state = EM_STORE_DATA_SYNC_CFG_INIT;
			}
			else if (pcb->portocol == 1) //仍有协议没发完
			{
				pcb->tick_send = 0;
				pcb->send_counter = 0;
				pcb->tei = TEI_STA_MIN;
				pcb->state = EM_STORE_DATA_SYNC_CFG_SEND_UNICAST;
			}
			else //单轮次结束，没有全部配置完，切换到新轮次需要等待10分钟
			{
				pcb->state = EM_STORE_DATA_SYNC_CFG_RESTART;
				pcb->tick_send = tick_now;
			}
			
			return OK;
		}
	}
	
	return OK;	
}
#endif

#ifdef GDW_PEROID_ID_MESSAGE_SET_CAPTURE
u16 IDMsgReqeustTimerId;
void ReadModuleIDMsg(void *arg)
{
	//具体实现批量读取ID的逻辑
	OS_TICK_64 tick_now= GetTickCount64()/1000;
	static u16 last_tei = 0;
	u32 period = g_PlmStatusPara.id_set_period;
	PLC_GET_MOUDLE_ID_CB* pcb = &g_pRelayStatus->get_moudle_id_cb;
	
	if (pcb->tei<TEI_STA_MIN||pcb->tei>TEI_STA_MAX)
		{
			pcb->tei=TEI_STA_MIN;
			pcb->retry=0;
		}
		for (; pcb->tei <= TEI_STA_MAX; pcb->tei++)
		{
			P_STA_INFO pSta = PLC_GetValidStaInfoByTei(pcb->tei);
			if (NULL == pSta)
				continue;
			if (pSta->net_state!=NET_IN_NET)
			{
				continue;
			}
			if (!pSta->received_id)
			{
				//新tei，重传次数重新置0
				if(last_tei != pcb->tei)
				{
					last_tei = pcb->tei;
					pcb->retry = 0;
				}

				//send ask
				PLC_SendAskModuleID(pcb->tei);
				pcb->tick_next=tick_now+OS_CFG_TICK_RATE_HZ*5;
				pcb->retry++;
				if (pcb->retry>3)
				{
					pcb->retry=0;
					pcb->tei++;
				}
			}
		}
		pcb->tick_next = tick_now + OS_CFG_TICK_RATE_HZ * period;
}

#endif

#if defined(GDW_2019_GRAPH_STOREDATA) || defined(PROTOCOL_NW_2020)
u8 PLC_StoreDateSyncCfg(P_STORE_DATA_SYNC_CFG pSyncCfg)
{
	P_TMSG pMsg = alloc_msg(__func__);

	if (NULL == pMsg)
		goto L_Error;

	pMsg->message = MSG_STORE_DATA_SYNC_CFG;
	pMsg->lParam = (LPARAM)pSyncCfg;

	OS_ERR err;
	OSTaskQPost(&g_pOsRes->APP_PLC_Autorelay_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO, &err);
	if (OS_ERR_NONE != err)
		goto L_Error;

	return OK;

L_Error:
	free_buffer(__func__,pSyncCfg);
	free_msg(__func__,pMsg);
	return ERROR;
}
#endif
#ifdef PROTOCOL_NW_2020
u8 PLC_SendRecordCfg(u8 *data,u16 len)
{
	u8 mac_addr[MAC_ADDR_LEN];
	memset(mac_addr, 0x99, MAC_ADDR_LEN);
	PLC_SendDataTrans(true, 1, (HPLC_SEND_PHASE)(SEND_PHASE_A|SEND_PHASE_B|SEND_PHASE_C), ++g_pRelayStatus->packet_seq_read_meter, mac_addr, data, len);
	return OK;
}

u8 PLC_ConfigRecordProc(OS_TICK_64 tick_now)//南网配置曲线采集间隔时间
{
	P_PLC_STORE_DATA_SYNC_CFG_CB pcb = &g_pRelayStatus->store_data_sync_cfg_cb;

#if 1
	if (EM_STORE_DATA_SYNC_CFG_START == pcb->state) //各协议广播3次
	{
		if(pcb->send_counter == 0)
		{
			pcb->state = EM_STORE_DATA_SYNC_CFG_INIT;
			return OK;
		}
		
		if(pcb->meter_645_data_len != 0)
		{
			PLC_SendRecordCfg(pcb->meter_645_data, pcb->meter_645_data_len);
			--pcb->send_counter;
			pcb->tick_send = tick_now + 10 * OSCfg_TickRate_Hz;
			pcb->state = EM_STORE_DATA_SYNC_CFG_WAIT;
		}
		else
		{
			pcb->state = EM_STORE_DATA_SYNC_CFG_INIT;
			return OK;
		}
	}
#else
	if (EM_STORE_DATA_SYNC_CFG_START == pcb->state) //各协议广播3次
	{
		int i;
		for (i = 0; i < 6; i++)
		{
			++pcb->idx;
			if (pcb->idx >= 6)
			{
				pcb->idx = 0;
			}
			if (pcb->collect_cnt[pcb->idx] != 0)
			{
				break;
			}
		}
		if (i >= 6) //未找到任何需要发送的帧
		{
			pcb->state = EM_STORE_DATA_SYNC_CFG_INIT;
			return OK;
		}
		PLC_SendRecordCfg(pcb->meter_645_data[pcb->idx], 28);
		--pcb->collect_cnt[pcb->idx];
		pcb->tick_send = tick_now + 10 * OSCfg_TickRate_Hz;
		pcb->state = EM_STORE_DATA_SYNC_CFG_WAIT;
	}
#endif	
	else if (EM_STORE_DATA_SYNC_CFG_WAIT == pcb->state)
	{
		if (tick_now > pcb->tick_send)
		{
			pcb->state = EM_STORE_DATA_SYNC_CFG_START;
		}
	}

	return OK;
}

#endif

void PLC_RouterProc(OS_TICK_64 tick_now)
{
	if (EM_ROUTE_INIT == g_pRelayStatus->read_state)
	{
		g_pRelayStatus->read_state = EM_ROUTE_DIR_READ;
	}
//	if (EM_ROUTE_NETTING == g_pRelayStatus->read_state)
//	{
//		if (OK != PLC_NettingProc(tick_now)) return;
//		g_pRelayStatus->read_state = EM_ROUTE_DIR_READ;
//	}
	if (EM_ROUTE_DIR_READ == g_pRelayStatus->read_state)
	{
		if (OK != PLC_DirReadMeterProc(tick_now))
			return;
		g_pRelayStatus->read_state = EM_MULTI_READ_TASK;
	}
	if (EM_MULTI_READ_TASK == g_pRelayStatus->read_state)
	{
#ifdef HPLC_CSG
		if (OK != PLC_RouteReadMultiMeterProc(tick_now))
			return;
#endif
		g_pRelayStatus->read_state = EM_ROUTE_ROUTE_READ;
		
	}
	if (EM_ROUTE_ROUTE_READ == g_pRelayStatus->read_state)
	{
#ifndef OVERSEA
		if (OK != PLC_RouteReadMeterProc(tick_now))
			return;
#endif		
		g_pRelayStatus->read_state = EM_ROUTE_EVENT_SWITCH;
	}
	if (EM_ROUTE_EVENT_SWITCH == g_pRelayStatus->read_state)
	{
#ifdef HPLC_CSG
		if (OK != PLC_SetEventSwitchProc(tick_now))
			return;
#endif		
		g_pRelayStatus->read_state = EM_ROUTE_BROAD_CAST;
	}
	if (EM_ROUTE_BROAD_CAST == g_pRelayStatus->read_state)
	{
		if (OK != PLC_BroadcastProc(tick_now))
			return;
		g_pRelayStatus->read_state = EM_ROUTE_ZC;
	}
	if (EM_ROUTE_ZC == g_pRelayStatus->read_state)
	{
#ifndef DIS_FUNC
		if (OK != PLC_ZCProc(tick_now))
			return;
#endif
		g_pRelayStatus->read_state = EM_ROUTE_SELFREG;
	}
	if (EM_ROUTE_SELFREG == g_pRelayStatus->read_state)
	{
		if (OK != PLC_SelfRegProc(tick_now))
			return;
		g_pRelayStatus->read_state = EM_ROUTE_GET_COLL_SN;
	}
	if (EM_ROUTE_GET_COLL_SN == g_pRelayStatus->read_state)
	{
		if (OK != PLC_GetCollSnProc(tick_now))
			return;
		g_pRelayStatus->read_state = EM_ROUTE_STA_AUTHEN;
	}
	if (EM_ROUTE_STA_AUTHEN == g_pRelayStatus->read_state)
	{
#ifdef ASSOC_CNF_YINGXIAO
		if (OK != PLC_SendStaAuthenProc(tick_now))
			return;
#endif
		g_pRelayStatus->read_state = EM_ROUTE_UPDATE;
	}
	if (EM_ROUTE_UPDATE == g_pRelayStatus->read_state)
	{
		if (OK != PLC_RouteUpdateProc(tick_now))
			return;
		g_pRelayStatus->read_state = EM_ROUTE_PHASE_ERR;
	}
	if (EM_ROUTE_PHASE_ERR == g_pRelayStatus->read_state)
	{
		#ifdef ERROR_APHASE
		if (OK != PLC_PhaseErrProc(tick_now))
			return;
		#endif
		g_pRelayStatus->read_state = EM_ROUTE_GET_MODULE_ID;
	}
	if (EM_ROUTE_GET_MODULE_ID == g_pRelayStatus->read_state)
	{
#ifndef HPLC_CSG
		if (OK != PLC_GetStaGetModuleIdProc(tick_now))
			return;
#endif
		g_pRelayStatus->read_state = EM_ROUTE_GET_MODULE_ID_REALTIME;
	}
    if (EM_ROUTE_GET_MODULE_ID_REALTIME == g_pRelayStatus->read_state)
	{
#ifndef HPLC_CSG
		if (OK != PLC_GetStaGetModuleIdRealTimeProc(tick_now))
			return;
#endif
		g_pRelayStatus->read_state = EM_ROUTE_AREA_PHASE;
	}
	if (EM_ROUTE_AREA_PHASE==g_pRelayStatus->read_state)
	{
		if(!USE_OLD_PHASE)
		{
		if (OK != PLC_AreaGetEdgeProc(tick_now))
		{
			return;
		}
		}
#ifdef GDW_ZHEJIANG
		g_pRelayStatus->read_state = EM_ROUTE_ONLINE_TIME_LOCK;
#else
		g_pRelayStatus->read_state = EM_ROUTE_AREA_DISCERNMENT;
#endif
	}
#ifdef GDW_ZHEJIANG
	if (EM_ROUTE_ONLINE_TIME_LOCK == g_pRelayStatus->read_state)
	{
		if (OK != PLC_OnlineLockTimeTimeProc(tick_now))
			return;
		g_pRelayStatus->read_state = EM_ROUTE_COLLECT_EVENT_REPORT;
	}
	if(EM_ROUTE_COLLECT_EVENT_REPORT== g_pRelayStatus->read_state)
	{
		if (OK != PLC_EventCollectReportTimeProc(tick_now))
			return;
		g_pRelayStatus->read_state = EM_ROUTE_AREA_DISCERNMENT;
	}	
#endif

	if (EM_ROUTE_AREA_DISCERNMENT == g_pRelayStatus->read_state)
	{
#ifndef DIS_FUNC
		if (OK != PLC_AreaDiscernmentProc(tick_now))
			return;
#endif
#if defined(HPLC_CSG)
		g_pRelayStatus->read_state = EM_ROUTE_READ_STA_STATUS;   //EM_ROUTE_INIT;
#else
#ifdef GDW_2019_GRAPH_STOREDATA	
		g_pRelayStatus->read_state = EM_ROUTE_STORE_DATA_BCAST_TIMING;
#else
		g_pRelayStatus->read_state = EM_ROUTE_INIT;
#endif
#endif
	}
#ifdef GDW_2019_GRAPH_STOREDATA	
	if (EM_ROUTE_STORE_DATA_BCAST_TIMING == g_pRelayStatus->read_state)
	{
		if (OK != PLC_StoreDataBcastTimeProc(tick_now))
			return;

		g_pRelayStatus->read_state = EM_ROUTE_STORE_DATA_SYNC_CFG;
	}
	if (EM_ROUTE_STORE_DATA_SYNC_CFG == g_pRelayStatus->read_state)
	{
		if (OK != PLC_StoreDataSyncCfgProc(tick_now))
			return;

		g_pRelayStatus->read_state = EM_ROUTE_GET_STA_EDGE;
	}
	if (EM_ROUTE_GET_STA_EDGE == g_pRelayStatus->read_state)
	{
#ifndef DIS_FUNC
		if (OK != PLC_GetStaPhaeEdgeProc(tick_now))
			return;
#endif
#if defined(HPLC_CSG)
		g_pRelayStatus->read_state = EM_ROUTE_READ_STA_STATUS;   //EM_ROUTE_INIT;
#else
		g_pRelayStatus->read_state = EM_ROUTE_INIT;
#endif
	}
#endif	
#if defined(HPLC_CSG)
	if (EM_ROUTE_READ_STA_STATUS == g_pRelayStatus->read_state)
	{
		if (OK != PLC_StaStatusProc(tick_now))
			return;
		g_pRelayStatus->read_state = EM_ROUTE_READ_STA_INFO;
	}
	if (EM_ROUTE_READ_STA_INFO == g_pRelayStatus->read_state)
	{
		if (OK != PLC_StaInfoProc(tick_now))
			return;
		g_pRelayStatus->read_state = EM_ROUTE_READ_STA_INFO_REALTIME;
	}
	if (EM_ROUTE_READ_STA_INFO_REALTIME == g_pRelayStatus->read_state)
	{
#ifdef PROTOCOL_NW_2021		
		if (OK != PLC_StaInfoRealTimeProc(tick_now))
			return;
#endif		
		g_pRelayStatus->read_state = EM_ROUTE_READ_STA_CHANNEL;
	}
	if (EM_ROUTE_READ_STA_CHANNEL == g_pRelayStatus->read_state)
	{
		if (OK != PLC_StaChannelInfoProc(tick_now))
			return;

		g_pRelayStatus->read_state = EM_ROUTE_READ_STA_NEW_CHANNEL;
	}
    if (EM_ROUTE_READ_STA_NEW_CHANNEL == g_pRelayStatus->read_state)
	{
		if (OK != PLC_StaChannelNewInfoProc(tick_now))
			return;
#ifdef PROTOCOL_NW_2020	
		g_pRelayStatus->read_state = EM_ROUTE_STORE_DATA_SYNC_CFG;
#else
		g_pRelayStatus->read_state = EM_ROUTE_INIT;
#endif
	}
	if (EM_ROUTE_STORE_DATA_SYNC_CFG == g_pRelayStatus->read_state)
	{
		if (OK != PLC_ConfigRecordProc(tick_now))
			return;
		g_pRelayStatus->read_state = EM_ROUTE_INIT;
	}
	#ifdef PLC_X4_FUNC
	if (EM_ROUTE_READ_TASK_INIT == g_pRelayStatus->read_state)
	{
		if (OK != PLC_X4CollTaskInitProc(tick_now))
			return;
		g_pRelayStatus->read_state = EM_ROUTE_X4_READ_TASK_SYNC;
	}
	if (EM_ROUTE_X4_READ_TASK_SYNC == g_pRelayStatus->read_state)
	{
		if (OK != PLC_X4ReadTaskSyncProc(tick_now))
			return;
		g_pRelayStatus->read_state = EM_ROUTE_INIT;
	}
	#endif
#endif
}

void PLC_Task_Init(void)
{
	OS_ERR err;
	OS_TICK_64 tick_now = GetTickCount64();
	OSMutexCreate(&mutex_sta_info, "sta  info mutex", &err);
	g_PlcWaitRxQueue = define_new_queue((queue *)&PlcRxWaitQueueMem[0], PLC_RX_WAIT_QUEUE_SIZE);

	PLC_ClearSelfRegList();

//	if (g_PlmConfigPara.plc_max_timeout <= DIR_READ_MIN_TIMEOUT_SEC)
//	{
//		g_PlmConfigPara.plc_max_timeout = DIR_READ_DEFAULT_TIMEOUT_SEC;
//		dc_flush_fram(&g_PlmConfigPara.plc_max_timeout, 1, FRAM_FLUSH);
//	}
#if defined(PROTOCOL_NW_2020) && !defined(PROTOCOL_NW_2023_GUANGDONG_V2DOT1)
	memset_special(&OutListMeter, 0, sizeof(RemMeter_s));
#endif
#ifdef HPLC_GDW
	memset_special(g_SelfRegCollReportMap, 0, CCTT_METER_NUMBER*sizeof(SELF_REG_COLL_REPORT_MAP));
#endif
	memset_special(g_pRelayStatus, 0, sizeof(RELAY_STATUS));
	g_pRelayStatus->pause_sec = 0xff;
	g_pRelayStatus->selfreg_cb.packet_seq_self_reg = rand_special();
	g_pRelayStatus->area_dis_cb.collection_seq = (u8)rand_special();
	g_pRelayStatus->area_phase_cb.ntb_collection_seq = (u8)rand_special();
#ifdef REQ_CENTRE_TIME_FIXED_VALUE   
    g_pRelayStatus->init_meter_delay = 55;//默认在初始化档案之前请求集中器时间的延时
#else
	g_pRelayStatus->init_meter_delay = 3*60;//默认在初始化档案之前请求集中器时间的延时
#endif	
	//g_pRelayStatus->search_level = 1;
	g_pRelayStatus->cfged_meter_num = 0;
#ifdef DEBUG_METER_RELAY
	g_pRelayStatus->plc_listen_sec_down = 3;
#else
	g_pRelayStatus->plc_listen_sec_down = 10;
#endif

	memset_special(g_pStaStatusInfo, 0, sizeof(STA_STATUS_INFO) * TEI_CNT);
	PLC_IdentInNet(CCO_TEI);
	g_PlmStatusPara.t_now_sec=0;
//	memset_special(&g_PlmStatusPara.t_now_sec, 0xff, sizeof(g_PlmStatusPara.t_now_sec));
	memset_special(&g_PlmStatusPara.meter_freeze_time_lst1[0], 0xff, sizeof(g_PlmStatusPara.meter_freeze_time_lst1));
	memset_special(&g_PlmStatusPara.meter_freeze_time_lst2[0], 0xff, sizeof(g_PlmStatusPara.meter_freeze_time_lst2));
	g_PlmStatusPara.meter_freeze_time_lst1_flag = FALSE;
	g_PlmStatusPara.meter_freeze_time_lst2_flag = FALSE;

	if ('C' == c_Tmn_ver_info.factoryCode[0] && 'T' == c_Tmn_ver_info.factoryCode[1])
		g_PlmStatusPara.factoryFlag = EM_FACTORY_DX;
	else if ('S' == c_Tmn_ver_info.factoryCode[0] && 'E' == c_Tmn_ver_info.factoryCode[1])
		if ('8' == c_Tmn_ver_info.chip_code[0] && '3' == c_Tmn_ver_info.chip_code[1])
			g_PlmStatusPara.factoryFlag = EM_FACTORY_DR_ESY;
		else
			g_PlmStatusPara.factoryFlag = EM_FACTORY_DR;
	else
		g_PlmStatusPara.factoryFlag = EM_FACTORY_SL;

	g_PlmStatusPara.report_meter_event = 1;

#ifdef OVERSEA
	g_PlmStatusPara.report_not_white_sta = 1;
#endif

#ifdef GDW_2019_GRAPH_STOREDATA
	g_PlmStatusPara.store_data_param.bcast_time_period = 12*60*60; //单位秒，默认12小时
#endif

#if defined(PROTOCOL_GD_2016)
	g_PlmStatusPara.restart_read_flag = TRUE;
//    PLC_AutoCfgPackInfo();
	PLC_AutoCfgFreezeDI();
#elif defined(APP_CONCENTRATOR_MODE) || defined(APP_KENENG_MXDX)
//    PLC_AutoCfgPackInfo();
	PLC_AutoCfgFreezeDI();
#endif

	if ((0 == g_pPlmConfigPara->net_id) || (g_pPlmConfigPara->net_id > NID_MAX))
	{
		g_pPlmConfigPara->net_id = rand_hard_range(1, NID_MAX);
		dc_flush_fram(&g_pPlmConfigPara->net_id, sizeof(g_pPlmConfigPara->net_id), FRAM_FLUSH);
	}

//#if defined(HPLC_CSG)
#if 0
	//g_pPlmConfigPara->net_id = 7;
	g_pPlmConfigPara->net_id = 8;
	g_pPlmConfigPara->net_seq = 102;
#endif

	BeaconSetBeaconFrameCallback(PLC_BeaconFrameCallback);
	MacSetMacFrameCallback(PLC_MacFrameCallback);

	P_STA_INFO pCco = &g_pStaInfo[TEI_CCO];
	PLC_CopyCCOMacAddr(pCco->mac, FALSE);
	pCco->used = CCO_USERD;
	pCco->pco = TEI_INVALID;
	pCco->brother = TEI_INVALID;
	pCco->child = TEI_INVALID;
//    pCco->roles = ROLES_CCO;
	pCco->net_state = NET_IN_NET;       //已入网、离线、未入网  NET_OUT_NET NET_IN_NET NET_OFFLINE_NET
	pCco->dev_type = 2;                 //1 抄控器	 2 集中器本地通信单元  3 电表通信单元	 4 中继器  5 II型采集器  6 I型采集器单元  7 三相电表通信单元
	pCco->hplc_phase = PHASE_UNKNOW;         //相线
	pCco->bootloader=1;
	pCco->phy_phase = 0x39|(1<<7);
	StaNeighborTab.Neighbor[TEI_CCO].PcoSucRate=100;

//	P_STA_CHIP_ID_INFO pChipID = (P_STA_CHIP_ID_INFO)g_cco_id.chip_id;
//	memcpy_special(pCco->factory_code, pChipID->factory_code, sizeof(pChipID->factory_code));
//	memcpy_special(pCco->chip_code, pChipID->chip_code, sizeof(pChipID->chip_code));
//	memcpy_special(pCco->dev_seq, pChipID->dev_seq, sizeof(pChipID->dev_seq));
//	memcpy_special(pCco->check_sum, pChipID->check_sum, sizeof(pChipID->check_sum));
	memcpy_special(pCco->software_v, c_Tmn_ver_info.software_v, sizeof(c_Tmn_ver_info.software_v));
	memcpy_special(pCco->ver_factory_code, c_Tmn_ver_info.factoryCode, sizeof(c_Tmn_ver_info.factoryCode));
	memcpy_special(pCco->ver_chip_code, c_Tmn_ver_info.chip_code, sizeof(c_Tmn_ver_info.chip_code));

	PLC_stage_pause();
	PLC_get_relay_info();

	TX_LED_OFF();
	RX_LED_OFF();
	if (CCO_Hard_Type)
	{
		PA_LED_OFF();
		PB_LED_OFF();
		PC_LED_OFF();
	}

#if !defined(FUNC_HPLC_READER)
//	BSP_OS_TimeDly(200);
//	PLC_report_msg_to_main();
#endif
#ifndef HPLC_CSG
	HPLC_RegisterTmr(clearRepeatNotWhiteSta, NULL, 6 * 60 * 60 * 1000, TMR_OPT_CALL_PERIOD);
#endif
}




void PLC_MsgProc()
{
	OS_ERR err;

	P_TMSG pMsg = NULL;
	OS_MSG_SIZE msgSize;

	pMsg = OSTaskQPend(OSCfg_TickRate_Hz / 200, OS_OPT_PEND_BLOCKING, &msgSize, NULL, &err);

	if ((err != OS_ERR_NONE) || (NULL == pMsg))
	{
		free_msg(__func__,pMsg);
		return;
	}

	if (MSG_PARAM_CLEAR == pMsg->message)
	{	
		memset(g_RelayStatus.area_dis_cb.getedZoneStatusBitMap,0,sizeof(g_RelayStatus.area_dis_cb.getedZoneStatusBitMap));
		memset(g_RelayStatus.area_dis_cb.zoneErrorMap,0,sizeof(g_RelayStatus.area_dis_cb.zoneErrorMap));
		memset_special(g_pMeterRelayInfo, 0xff, CCTT_METER_NUMBER * sizeof(RELAY_INFO));

#ifdef PROTOCOL_GD_2016
		data_flash_erase_straight(METER_COLL_DB_ADDR,CCTT_METER_COLL_NUMBER * sizeof(METER_COLL_MAP));
#endif

#if 1
		u32 meter_size=CCTT_METER_NUMBER * sizeof(RELAY_INFO);
		meter_size/=DATA_FLASH_SECTION_SIZE;
		meter_size*=DATA_FLASH_SECTION_SIZE;
		if ((meter_size+DATA_FLASH_SECTION_SIZE)>=METER_RELAY_INFO_SIZE)
		{
			meter_size=METER_RELAY_INFO_SIZE-DATA_FLASH_SECTION_SIZE*2;
		}
		data_flash_erase_straight_with_poweron(METER_RELAY_INFO_ADDR + meter_size, DATA_FLASH_SECTION_SIZE * 2);
		data_flash_erase_straight_with_poweron(METER_RELAY_INFO_ADDR+METER_RELAY_INFO_SIZE+meter_size, DATA_FLASH_SECTION_SIZE*2);
		OSTimeDly(2, OS_OPT_TIME_DLY, &err);
//		data_flash_write_straight_with_crc(STA_INFO_ADDR, (u8 *)g_pStaInfo, TEI_CNT * sizeof(STA_INFO));
#if 0
		OSTimeDly(2, OS_OPT_TIME_DLY, &err);
		data_flash_write_straight_with_crc(METER_COLL_DB_ADDR, (u8 *)g_pMeterCollMap, CCTT_METER_COLL_NUMBER * sizeof(METER_COLL_MAP));
#endif
#else
		data_flash_erase_straight( METER_RELAY_INFO_ADDR, METER_RELAY_INFO_SIZE );
		data_flash_erase_straight( STA_INFO_ADDR, STA_INFO_SIZE );
	#ifdef PROTOCOL_GD_2016
		data_flash_erase_straight( METER_COLL_DB_ADDR, METER_COLL_DB_SIZE );
	#endif
#endif
		//g_PlmConfigPara.sysEraseStackCount ++;
		//dc_flush_fram(&g_PlmConfigPara.sysEraseStackCount, sizeof(g_PlmConfigPara.sysEraseStackCount), FRAM_FLUSH);

		PLC_TriggerChangeNetid(4);       //华为参数格式化并未更改组网序列号
	}
	else if (MSG_TASK_CLEAR== pMsg->message)
	{
#ifdef HPLC_CSG
		PLC_delete_all_read_task_gd(EM_TASK_TYPE_READ_TASK);
		PLC_delete_all_read_task_gd(EM_TASK_TYPE_MULTI_READ_TASK);
#endif
	}
	else if (MSG_DATA_CLEAR == pMsg->message)
	{
		//可以清STA信息，重新组网
	}
	else if (MSG_MAC_FRAME == pMsg->message)
	{
		P_RECEIVE_MAC pReceiveMac = (P_RECEIVE_MAC)pMsg->lParam;
		PLC_HandleMacFrame(pReceiveMac);
		free_buffer(__func__,pReceiveMac);
	}
	else if (MSG_ONE_HOP_FRAME == pMsg->message)
	{
		P_RECEIVE_MAC pReceiveMac = (P_RECEIVE_MAC)pMsg->lParam;
		PLC_HandleOneHopMacFrame(pReceiveMac, pMsg->wParam>>16,pMsg->wParam&0xfff);
		free_buffer(__func__,pReceiveMac);
	}
	else if (MSG_DIR_READ == pMsg->message)
	{
		P_READ_MSG_INFO pReadMsg = (P_READ_MSG_INFO)pMsg->lParam;
		P_PLC_DIR_READ_CB pcb = &g_pRelayStatus->dir_read_cb;
//		if (EM_ROUTE_DIR_READ == g_pRelayStatus->read_state)
		{
			if (EM_DIR_READ_INIT != pcb->state && pcb->pPlmCmdBak!=NULL)
			{
				u8 dst_addr[LONG_ADDR_LEN] = {0};
				PLC_GetAppDstMac(pReadMsg->header.mac, dst_addr);
				PLM_PHASE plm_phase = PLC_GetPlmPhaseByMacAddr(dst_addr);
				PLM_ReportDirRead(pReadMsg, plm_phase, NULL, 0);
				free_buffer(__func__,pReadMsg);
				free_msg(__func__,pMsg);
				return;
			}
		}
		free_buffer(__func__,pcb->pPlmCmdBak);
		pcb->pPlmCmdBak = pReadMsg;
	}
	else if (MSG_PARALLEL_READ == pMsg->message)
	{
		P_READ_MSG_INFO pReadMsg = (P_READ_MSG_INFO)pMsg->lParam;
		//if (OK == PLC_SendDirRead(pReadMsg))
		PLC_SendDirRead(pReadMsg);
		{
			pReadMsg->header.ts_send = GetTickCount64();
			pReadMsg->header.ts_timeout = PLC_GetReadMeterTimeoutTick(pReadMsg->header.msg_len);
	
			pReadMsg->header.broadcast = 0;
			if (pReadMsg->header.read_count * MAX_MINOR_LOOP_READ_METER > pReadMsg->header.read_time++)
			{
				if (((pReadMsg->header.read_count - 1) * MAX_MINOR_LOOP_READ_METER) <= pReadMsg->header.read_time)
				{
#ifdef DLMS_BROADCAST
					pReadMsg->header.broadcast = 1;	//24年8月决定开启广播增加抄表成功率，在广播时带上表mac地址，STA去过滤
#elif defined(OVERSEA)
					pReadMsg->header.broadcast = 0; //海外版本不进行广播重传尝试
#else				
					pReadMsg->header.broadcast = 1;
#endif
				}

			}
			enqueue(g_PlcWaitRxQueue, pReadMsg);
		}
//		else
//		{
//			enqueue(g_PlcWaitRxQueue, pReadMsg);
////			PLM_ReportParallelRead(pReadMsg, NULL, 0);
////			free_buffer(__func__,pReadMsg);
//		}
	}
	else if (MSG_ON_PLM_CONFIRM == pMsg->message)
	{
		if (EM_ROUTE_DIR_READ == g_pRelayStatus->read_state)
		{
			P_PLC_DIR_READ_CB pcb = &g_pRelayStatus->dir_read_cb;
			if (EM_DIR_READ_WAIT_POST == pcb->state)
				pcb->state = EM_DIR_READ_POSTED;
		}
		else if (EM_ROUTE_ROUTE_READ == g_pRelayStatus->read_state)
		{
			P_PLC_ROUTE_READ_CB pReadItemInfo = &g_pRelayStatus->route_read_cb;
			if (EM_ROUTE_READ_WAIT_POST == pReadItemInfo->state)
				pReadItemInfo->state = EM_ROUTE_READ_POSTED;
		}
	}
	else if (MSG_ON_PLM_READ_ITEM == pMsg->message)
	{
		P_PLM_READ_ITEM_RSP pReadMsg = (P_PLM_READ_ITEM_RSP)pMsg->lParam;
		P_PLC_ROUTE_READ_CB pReadItemInfo = &g_pRelayStatus->route_read_cb;

		bool addr_correct = true;

		if (!IsMemSet(pReadMsg->node_addr, 0, FRM_DL645_ADDRESS_LEN))
		{
			if (OK != macCmp(pReadMsg->node_addr, pReadItemInfo->plmCmd.node_addr))
				addr_correct = false;
		}

		if (addr_correct && (EM_ROUTE_READ_WAIT_PLM == pReadItemInfo->state /*||EM_ROUTE_READ_FIXTIME_WAIT_PLM ==pReadItemInfo->state*/))
		{
			if (pReadItemInfo->pPlmRsp)
				free_buffer(__func__,pReadItemInfo->pPlmRsp);
			pReadItemInfo->pPlmRsp = pReadMsg;
		}
		else
		{
			free_buffer(__func__,pReadMsg);
		}
	}
	#ifdef ERROR_APHASE
	else if (MSG_PHASE_ERR== pMsg->message)
	{
		P_PLC_PHASE_ERR_CB pcb = &g_pRelayStatus->phase_err_cb;
		if(!pcb->status)//当前没有发送
		{
			pcb->status=1;
		}
		else if (pMsg->lParam)//强制刷新报文序列号
		{
			pcb->status=1;
		}
		pcb->cnt = 5;
	}
	#endif
	else if (MSG_BROADCAST == pMsg->message)
	{
		P_PLC_BROADCAST_CB pcb = &g_pRelayStatus->broadcast_cb;
		P_MSG_INFO pSendMsg = (P_MSG_INFO)pMsg->lParam;
		if (pSendMsg->msg_header.protocol == METER_PROTOCOL_3762_BROAD)
		{
			if (pcb->pSendMsgLower==NULL)
			{
				pcb->pSendMsgLower=pSendMsg;
			}
			else if (pcb->pSendMsgBak==NULL)
			{
				pcb->pSendMsgBak=pSendMsg;
			}
			else
			{
				free_buffer(__func__, pSendMsg);
			}
		}
		else
		{
			if (pcb->pSendMsgHight==NULL)
			{
				pcb->pSendMsgHight=pSendMsg;
			}
			else if (pcb->pSendMsgLower == NULL)
			{
				pcb->pSendMsgLower=pSendMsg;
			}
			else if (pcb->pSendMsgBak==NULL)
			{
				pcb->pSendMsgBak=pSendMsg;
			}
			else
			{
				free_buffer(__func__, pSendMsg);
			}
		}
	}
	else if (MSG_HPLC_COMM_TEST == pMsg->message)
	{
		P_COMM_TEST_INFO pTestMsg = (P_COMM_TEST_INFO)pMsg->lParam;
		PLC_SendCommTest(pTestMsg);
		free_buffer(__func__,pTestMsg);
	}
	else if (MSG_START_SELF_REG == pMsg->message)
	{
		P_PLC_SELFREG_CB pcb  = &g_pRelayStatus->selfreg_cb;
#if 0
		if (EM_SELFREG_INIT == pcb->state)
		{
			pcb->selfreg_time_last_min = (u16)pMsg->wParam;
			pcb->state = EM_SELFREG_START;
		}
#else
		pcb->start_stop_req=1;//start req
		pcb->selfreg_time_last_min = (u16)pMsg->wParam;

#ifdef HPLC_GDW
		memset_special(g_SelfRegCollReportMap, 0, CCTT_METER_NUMBER*sizeof(SELF_REG_COLL_REPORT_MAP));
#endif
#endif
	}
	else if (MSG_STOP_SELF_REG == pMsg->message)
	{
		P_PLC_SELFREG_CB pcb  = &g_pRelayStatus->selfreg_cb;
#if 0
		if (EM_SELFREG_INIT != pcb->state)
		{
	#if 1
			if (pcb->state < EM_SELFREG_REPORT_STOP)
				pcb->state = EM_SELFREG_REPORT_STOP;
	#else
			pcb->state = EM_SELFREG_INIT;
	#endif
		}
#else
		pcb->start_stop_req=2;//start req
#endif
	}
	else if (MSG_START_UPDATE_STA == pMsg->message)
	{
//		for (u16 tei = TEI_STA_MIN; tei < TEI_CNT; tei++)
//		{
//			if (PLC_GetValidStaInfoByTei(tei))
//			{
//				g_pStaStatusInfo[tei].need_up = 1;
//				g_pStaStatusInfo[tei].updateGetStart = 0;
//				g_pStaStatusInfo[tei].try_update_counter = 0;
//			}
//			else
//			{
//				g_pStaStatusInfo[tei].need_up = 0;
//				g_pStaStatusInfo[tei].updateGetStart = 0;
//				g_pStaStatusInfo[tei].try_update_counter = 0;
//			}
//		}

		P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;
		P_STA_UPDATE_PARAM pStaUpdateParam = (P_STA_UPDATE_PARAM)pMsg->lParam;

		pcb->flash_addr = pStaUpdateParam->flash_addr;

#ifdef HPLC_CSG
		pcb->old_file_size = pcb->file_size;
		pcb->old_file_checksum = pcb->file_checksum;
		pcb->old_block_count = pcb->block_count;
#endif
		pcb->file_size = pStaUpdateParam->file_size;
		pcb->file_checksum = pStaUpdateParam->file_checksum;

		pcb->state = EM_STA_UPDATE_INIT;
		pcb->block_size = 400;      //100、200、300、400
		pcb->block_count = pcb->file_size / pcb->block_size;
		if (pcb->file_size % pcb->block_size)
			pcb->block_count++;
		pcb->update_minutes = update_timeout_minutes;

		if (pcb->update_id==0)
		{
			pcb->update_id=rand();
#ifdef HPLC_CSG
			pcb->old_file_size = pcb->file_size;
			pcb->old_file_checksum = pcb->file_checksum;
			pcb->old_block_count = pcb->block_count;
#endif			
		}
		pcb->update_id++;

        debug_str(DEBUG_LOG_UPDATA, "start update, update id:0x%x\r\n", pcb->update_id);
        
		pcb->tick_start_update = GetTickCount64();
		if (NULL != pcb->bitmap)
		{
			free_buffer(__func__,pcb->bitmap);
			pcb->bitmap = NULL;
		}
		pcb->bitmap = alloc_buffer(__func__,(pcb->block_count + 7) / 8);
		pcb->period = getBeaconPeriod();
		pcb->need_update = TRUE;
		UpdataLidReq = 1;
		pcb->plan = 1; //先使用 广播 升级计划
		pcb->tei = TEI_STA_MIN;
#ifdef HPLC_CSG
		pcb->fileType = 2; //从节点模块文件
#endif
		if (pcb->plan == 0) //仅供测试单播升级计划
		{
			U16 tei = 0;
			P_STA_INFO pSta = NULL;
			for (tei = TEI_STA_MIN; tei < TEI_STA_MAX; tei++)
			{
				pSta = PLC_GetValidStaInfoByTei(tei);
				if ((NULL != pSta) && (NET_IN_NET == pSta->net_state))
				{
					g_pStaStatusInfo[tei].need_up = 1;
					g_pStaStatusInfo[tei].updateGetStart = 0;
				}
			}

			pcb->update_round = 5;
			pcb->retry = 20;
			InitProcPara(pcb, EM_STA_UPDATE_START, OS_CFG_TICK_RATE_HZ * 3, 1, CON_PLC_RESEND_COUNT, false, true);
		}

		free_buffer(__func__,pStaUpdateParam);

		debug_str(DEBUG_LOG_UPDATA, "flash_addr = %d, file_size = %d, file_checksum = 0x%08X\r\n", pcb->flash_addr, pcb->file_size, pcb->file_checksum);
	}
	else if (MSG_STOP_UPDATE_STA == pMsg->message)
	{
		P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;
		pcb->need_update = FALSE;
	}
	else if (MSG_START_AREA_DISCERNMENT == pMsg->message)
	{
		P_PLC_AREA_DISCERNMENT pcb = &g_pRelayStatus->area_dis_cb;
		if (EM_AREA_DISCERNMENT_INIT == pcb->state)
		{
			pcb->state = EM_AREA_DISCERNMENT_START;
			pcb->allow=1;
			pcb->round=0;
			memset(pcb->getedZoneStatusBitMap,0,sizeof(pcb->getedZoneStatusBitMap));
			memset(pcb->zoneErrorMap,0,sizeof(pcb->zoneErrorMap));
		}
	}
	else if (MSG_STOP_AREA_DISCERNMENT == pMsg->message)
	{
		P_PLC_AREA_DISCERNMENT pcb = &g_pRelayStatus->area_dis_cb;
		pcb->allow=0;
#ifdef PROTOCOL_NW_2020			
		ClearOutListMeter(1);
#endif
		pcb->round=0;
		//pcb->state = EM_AREA_DISCERNMENT_INIT;
	}
#if defined(HPLC_CSG)
	else if (MSG_STA_READ_TASK_INIT == pMsg->message)
	{
		P_STA_PLC_READ_TASK_INIT_CB pcb = &g_pRelayStatus->sta_read_task_init_cb;
		pcb->tick_start = GetTickCount64();
	}
#endif


#ifdef PROTOCOL_NW_2020
	else if(MSG_STORE_DATA_SYNC_CFG == pMsg->message)
	{
		P_PLC_STORE_DATA_SYNC_CFG_CB pcb = &g_pRelayStatus->store_data_sync_cfg_cb;
		P_STORE_DATA_SYNC_CFG pStoreDataSyncCfg = (P_STORE_DATA_SYNC_CFG)pMsg->lParam;
#if 1
		pcb->meter_645_data_len = pStoreDataSyncCfg->data_len;
		memcpy(pcb->meter_645_data, pStoreDataSyncCfg->data, pStoreDataSyncCfg->data_len);
		pcb->send_counter = 5;
		pcb->state = EM_STORE_DATA_SYNC_CFG_START;
#else
		u8 record_class = pStoreDataSyncCfg->data[0] - 0x33 - 1; //负荷记录时间从2到7
		if (record_class > 1 && record_class < 8)
		{
			record_class -= 2; //得到数组开始标号
			memcpy(pcb->meter_645_data[record_class],pStoreDataSyncCfg->data,28);
			pcb->collect_cnt[record_class]=5;//广播5次
		}
#endif		
		free_buffer(__func__, pStoreDataSyncCfg);
	}
#endif

#ifdef GDW_2019_GRAPH_STOREDATA
	else if(MSG_STORE_DATA_SYNC_CFG == pMsg->message)
	{
		P_PLC_STORE_DATA_SYNC_CFG_CB pcb = &g_pRelayStatus->store_data_sync_cfg_cb;
		P_STORE_DATA_SYNC_CFG pStoreDataSyncCfg = (P_STORE_DATA_SYNC_CFG)pMsg->lParam;

		if(pStoreDataSyncCfg->protocol == METER_PROTOCOL_3762_07) //645-2007通信协议
		{
			if(memcmp(pStoreDataSyncCfg->data, pcb->meter_645_data, pStoreDataSyncCfg->data_len) != 0) //判断采集方案是否有变化
			{
				pcb->meter_645_data_len = pStoreDataSyncCfg->data_len;
				memset(pcb->meter_645_data, 0, sizeof(pcb->meter_645_data));
				memcpy(pcb->meter_645_data, pStoreDataSyncCfg->data, pStoreDataSyncCfg->data_len);

				u16 tei = 0;
				P_STA_INFO pSta = NULL;
				for (tei = TEI_STA_MIN; tei < TEI_STA_MAX; tei++)
				{
					pSta = PLC_GetValidStaInfoByTei(tei);
					if ((NULL != pSta) && (NET_IN_NET == pSta->net_state))
					{
						P_RELAY_INFO pNode = PLC_GetWhiteNodeInfoByMac(pSta->mac);
						if (NULL == pNode)
							continue;

						if(pNode->meter_type.protocol != METER_TYPE_PROTOCOL_69845)
						{
							g_pStaStatusInfo[tei].need_sync_cfg = 1;
							g_pStaStatusInfo[tei].sync_cfg_result = 0;
							pcb->sta_bitmap[0][tei >> 5] |= 1 << (tei & 0x1f);
						}
					}
				}

				//if(pcb->state == EM_STORE_DATA_SYNC_CFG_INIT)
				{
					pcb->state = EM_STORE_DATA_SYNC_CFG_START;
					pcb->round = 3;
				}
			}
		}
		else if(pStoreDataSyncCfg->protocol == METER_PROTOCOL_3762_69845) //698.45通信协议
		{
			if(memcmp(pStoreDataSyncCfg->data, pcb->meter_698_data, pStoreDataSyncCfg->data_len) != 0) //判断采集方案是否有变化
			{
				pcb->meter_698_data_len = pStoreDataSyncCfg->data_len;
				memset(pcb->meter_698_data, 0, sizeof(pcb->meter_698_data));
				memcpy(pcb->meter_698_data, pStoreDataSyncCfg->data, pStoreDataSyncCfg->data_len);

				u16 tei = 0;
				P_STA_INFO pSta = NULL;
				for (tei = TEI_STA_MIN; tei < TEI_STA_MAX; tei++)
				{
					pSta = PLC_GetValidStaInfoByTei(tei);
					if ((NULL != pSta) && (NET_IN_NET == pSta->net_state))
					{
						P_RELAY_INFO pNode = PLC_GetWhiteNodeInfoByMac(pSta->mac);
						if (NULL == pNode)
							continue;

						if(pNode->meter_type.protocol == METER_TYPE_PROTOCOL_69845)
						{
							g_pStaStatusInfo[tei].need_sync_cfg = 2;
							g_pStaStatusInfo[tei].sync_cfg_result = 0;
							pcb->sta_bitmap[1][tei >> 5] |= 1 << (tei & 0x1f);
						}
					}
				}
				
				//if(pcb->state == EM_STORE_DATA_SYNC_CFG_INIT)
				{
					pcb->state = EM_STORE_DATA_SYNC_CFG_START;
					pcb->round = 3;
				}
			}
		}
		
		free_buffer(__func__,pStoreDataSyncCfg);
	}
	else if(MSG_STORE_DATA_BCAST_TIME == pMsg->message)
	{
		P_PLC_STORE_DATA_BCAST_CB pcb = &g_pRelayStatus->store_data_bcast_cb;

		if (EM_STORE_DATA_BCAST_TIME_INIT == pcb->state)
		{
			pcb->state = EM_STORE_DATA_BCAST_TIME_START;
			pcb->send_counter = 3;
		}
	}
	
#endif
#ifdef GDW_ZHEJIANG
	else if(MSG_ONLINE_TIME_LOCK == pMsg->message)
	{
		P_PLC_ONLINE_LOCK_TIME_CB pcb = &g_pRelayStatus->online_lock_time_cb;

		if (EM_ONLINE_LOCK_TIME_TIME_INIT == pcb->state)
		{
			pcb->state = EM_ONLINE_LOCK_TIME_TIME_START;
			pcb->send_counter = 10;
		}
	}
	else if(MSG_EVENT_COLLECT_REPORT == pMsg->message)
	{
		P_PLC_EVENT_COLLECT_REPORT_CB pcb = &g_pRelayStatus->event_collect_report_cb;

		if (EM_EVENT_COLLECT_REPORT_TIME_INIT == pcb->state)
		{
			pcb->state = EM_EVENT_COLLECT_REPORT_TIME_START;
		}
	}
#endif
	else if((MSG_INNER_VER_READ == pMsg->message))
	{
		u8* node_mac = (u8 *)pMsg->lParam;
		PLC_SendInnerVerRead(node_mac);
		free_buffer(__func__,node_mac);
	}
#ifdef HPLC_CSG    
    else if((MSG_RESET_STA == pMsg->message))
	{
	    P_RESET_STA_INFO pResetSta = (P_RESET_STA_INFO)pMsg->lParam;

		PLC_SendStaReset(pResetSta);

		free_buffer(__func__, pResetSta);
	}
#endif

	free_msg(__func__,pMsg);
}

#ifdef HPLC_CSG
void gotoDetect(OS_TICK_64 tick_now)
{
#if defined(GUIZHOU_SJ) || defined(GUANGDONG_CSG) //广东清远台体（数据链路层测试），CCO频段探测测试；台体测试时间较短，第一次频段探测时间启动后10分钟
	static OS_TICK_64 tick_next = OS_CFG_TICK_RATE_HZ * 60 * 15; //改15分钟，规避CCO功率谱密度测试（在频段2测试，STA会离线触发频段探测，功率谱测试180s*4次，最后的带外会被频段1拉高）
#else
	static OS_TICK_64 tick_next = OS_CFG_TICK_RATE_HZ * 60 * 60; 
#endif
	if (tick_now > tick_next)
	{       
		if (Bcsma_Lid == SOF_LID_DETECT_BCSMA) //上次正在探测
		{
			HPLC_DetectStop();
			tick_next = tick_now + OS_CFG_TICK_RATE_HZ * 60 * 60 * 3; //下一次过3个小时再开始探测
		}
		else if (Bcsma_Lid == 0xff) //没有使用bcsma
		{
			if (g_pRelayStatus->innet_num != g_pRelayStatus->cfged_meter_num
				&& g_pRelayStatus->cfged_meter_num != 0
				&& g_PlmConfigPara.hplc_comm_param == 2
				&& (!g_pRelayStatus->change_frequence_cb.second_down))
			{
				HPLC_DetectStart();
				tick_next = tick_now + OS_CFG_TICK_RATE_HZ * 60 * 10; //探测10分钟
			}
		}

	}
}
#endif

void Task_Plc_proc(void *p_arg)
{
	OS_ERR err;

	OS_TICK_64 tick_now = GetTickCount64();
	OS_TICK_64 tick_last = tick_now;
	OS_TICK_64 tick_last_RemoveRxQueue = tick_now;
	beaconDescoverInfo.rountHrf=3;
	PLC_Task_Init();

	PLC_RestartNetworking();    //g_pPlmConfigPara->net_seq = 70;     //71;

	HPLC_SetStaParamFunction(PLC_OnGetStaParam);
	HPLC_SetBeaconParamFunction(PLC_OnGetBeaconParam);
	HPLC_SetChangeNetIDFunction(PLC_ChangeNetIDCallback);
	DeviceResetOpen(Reset_CallBack);
	//for test param
#if 0
	{
		{
			BYTE mac[] =
			{0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
			//BYTE mac[] = {0x15, 0x64, 0x57, 0x01, 0x00, 0x00};      //00 00 01 57 64 15
			PLC_add_new_node(0, METER_TYPE_PROTOCOL_07, mac);
		}
		{
			BYTE main_mac[] =
			{0x01, 0x32, 0x21, 0x93, 0xA1, 0x00};
			memcpy_special(g_pPlmConfigPara->main_node_addr, main_mac, sizeof(main_mac));
		}
	}
#endif

#if !defined(FUNC_HPLC_READER)
	OSTimeDly(OSCfg_TickRate_Hz, OS_OPT_TIME_DLY, &err);
	debug_str(DEBUG_LOG_INFO, "System startup: %02x-%02x-%02x %02x%02x, %02x-%02x-%02x %02x%02x\r\n",
			  c_Tmn_ver_info.softReleaseDate.year, c_Tmn_ver_info.softReleaseDate.month, c_Tmn_ver_info.softReleaseDate.day, c_Tmn_ver_info.software_v[1], c_Tmn_ver_info.software_v[0],
			  c_Tmn_ver_info_inner.softReleaseDate.year, c_Tmn_ver_info_inner.softReleaseDate.month, c_Tmn_ver_info_inner.softReleaseDate.day, c_Tmn_ver_info_inner.software_v[1], c_Tmn_ver_info_inner.software_v[0]);
	debug_str(DEBUG_LOG_INFO,"Git version %x\r\n", SLG_COMMIT_HASH);
	debug_str(DEBUG_LOG_INFO,"Git date %d-%d-%d\r\n", SLG_COMMIT_YEAR, SLG_COMMIT_MON, SLG_COMMIT_DATE);
#endif

	while (1)
	{
#if 0
		extern void test_change_net(void);
		test_change_net();
#endif
		PLC_MsgProc();

		tick_now = GetTickCount64();

//        PLC_RxStateSta();
#ifdef HPLC_CSG
		gotoDetect(tick_now);
#endif

		Reset_Proc();
		
		//秒事件
		if (TicksBetween64(tick_now, tick_last) >= (OS_TICK_64)OSCfg_TickRate_Hz)
		{
			tick_last += (OS_TICK_64)OSCfg_TickRate_Hz;
			PLC_second_task();
			tick_now = GetTickCount64();
		}

		
		//50毫秒事件
		if (TicksBetween64(tick_now, tick_last_RemoveRxQueue) >= (OS_TICK_64)OSCfg_TickRate_Hz/20)
		{
			tick_last_RemoveRxQueue += (OS_TICK_64)OSCfg_TickRate_Hz/20;
			//删除等待队列旧的抄表项
			PLC_RemoveRxQueueOldItem();
			tick_now = GetTickCount64();
		}
		

#if !defined(FUNC_HPLC_READER)
		if (0)
		{
		}
		//发现列表报文发送周期开始, 不发送发现报文，1级表会离线重新关联请求
		else if (g_pRelayStatus->innet_num)
		{
			if(g_pRelayStatus->cco_foundlist_interval_sec > 0
				 && TicksBetween64(tick_now, g_pRelayStatus->tick_last_foundlist_send) > (OSCfg_TickRate_Hz * g_pRelayStatus->cco_foundlist_interval_sec))
			{
				g_pRelayStatus->tick_last_foundlist_send = tick_now;
				if (g_pRelayStatus->innet_num > 0)
					PLC_SendFoundListAuto();
			}
			#ifdef HRF_SUPPORT
			if ((g_pRelayStatus->cco_hrf_foundlist_interval_sec > 0)
				&& (TicksBetween64(tick_now, g_pRelayStatus->tick_last_hrf_foundlist_send) > (OSCfg_TickRate_Hz * g_pRelayStatus->cco_hrf_foundlist_interval_sec)))

			{
				g_pRelayStatus->tick_last_hrf_foundlist_send = tick_now;
				if (g_pRelayStatus->innet_num > 0)
					PLC_SendHrfFoundListAuto();
			}
			#endif
		}

		//关联汇总指示报文
		PLC_SendAssocGatherIndAuto(tick_now);

		PLC_RouterProc(tick_now);

#ifdef GDW_2019_GRAPH_STOREDATA
		if (g_pRelayStatus->online_num == g_pRelayStatus->all_meter_num) //组网完成，请求集中器时钟
		{
			if(storeDataReqeustTimerId == INVAILD_TMR_ID)
			{
				storeDateRequestCount = 3;
				storeDataReqeustTimerId = HPLC_RegisterTmr(StoreDateRequestTime, NULL, 3*OSCfg_TickRate_Hz, TMR_OPT_CALL_ONCE);
			}
		}
#endif	
#ifdef GDW_JIANGSU
		if(routerNewStaSendRTCTimerId == INVAILD_TMR_ID)
		{
			routerNewStaSendRTCTimerId = HPLC_RegisterTmr(PLC_NewStaSendRTCTime,NULL, 30 * 60 * 1000, TMR_OPT_CALL_PERIOD);
		}
		
		if (g_pRelayStatus->online_num == g_pRelayStatus->all_meter_num) //组网完成，请求集中器时钟
		{
			if(routerSendRTCTimerId == INVAILD_TMR_ID)
			{
				routerSendRTCTime(0);
				routerSendRTCTimerId = HPLC_RegisterTmr(routerSendRTCTime,NULL, 12 * 60 * 60 * 1000, TMR_OPT_CALL_PERIOD);
			}
		}
#endif			
#endif

	}
}

BYTE PLC_Is_DL645_Protocol(BYTE plc_protocol)
{
	if (GD_DOWN_PROTOCOL_TRANSPARENT == plc_protocol)
		return FALSE;
	if (GD_DOWN_PROTOCOL_69845 == plc_protocol)
		return FALSE;
	return TRUE;
}

/* build PLC frame, post
*  return OK if response has correct metersn
*/
unsigned short PLC_Post_Aframe(MSG_SHORT_INFO *pDl645Msg,
							   unsigned short *relay_meter, unsigned char relay_level, USHORT wait_time_01s)
{
	return 0;
}

unsigned short PLC_Wait_frame(USHORT read_sn, USHORT read_status, unsigned char *pFrmHead1, P_MSG_INFO *ppMsg)
{
	return !(OK);
}

BYTE PLC_freeze_save_data(P_MSG_INFO pMsg)
{
	return 0;
}
				
BOOL is_Extend_ReadCollectGraphPkt(u8* data,u16 datalen)
{
	if(PLC_check_dl645_frame(data,datalen) == NULL)
		return FALSE;
	if(*(u32 *)(((GD_DL645_BODY*)data)->dataBuf)  != (0x3A353233))//0X0702FF00 加上0x33
		return FALSE;
	else
		return TRUE;
}

#ifdef PROTOCOL_GD_2016

P_READ_TASK_INFO PLC_find_read_task_gd(EM_TASK_TYPE task_type, u16 taskid)
{
	P_READ_TASK_INFO pRes = NULL;

	OS_ERR err;
	OS_MSG_SIZE msgSize;

	OS_MSG_QTY count = g_pOsRes->read_meter_task_q.MsgQ.NbrEntries;

	if (count <= 0)
		return pRes;

	OSSchedLock(&err);

	while (count--)
	{
		P_READ_TASK_INFO pTaskInfo = OSQPend(&g_pOsRes->read_meter_task_q, 0, OS_OPT_PEND_NON_BLOCKING, &msgSize, NULL, &err);
		if (OS_ERR_NONE != err)
			break;

		OSQPost(&g_pOsRes->read_meter_task_q, pTaskInfo, sizeof(READ_TASK_INFO), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);
		if (OS_ERR_NONE != err)
		{
			free_buffer(__func__,pTaskInfo);
			continue;
		}

		if (pTaskInfo->task_type == task_type && pTaskInfo->task_header.task_id == taskid)
			pRes = pTaskInfo;
	}

	OSSchedUnlock(&err);

	return pRes;
}

BOOL PLC_fetch_task_read_item_gd(P_READ_TASK_INFO *ppReadTask,bool isMultiTask,u16 *fliter_taskid,u8 taskmac[][MAC_ADDR_LEN],u8 max_num)
{
	OS_ERR err;
	OS_MSG_SIZE msgSize;

	*ppReadTask = NULL;

	OS_MSG_QTY count = g_pOsRes->read_meter_task_q.MsgQ.NbrEntries;
	if (count <= 0)
		return FALSE;

	OSSchedLock(&err);

	//相同优先级，还可以根据到达时间，先到先发送
	u8 min_priority = 0xff;
	u8 min_read_time=MAX_READ_TIME+1;
	P_READ_TASK_INFO pReadTask = NULL;
	u8 find_diff_mac=0;
	while (count--)
	{
		P_READ_TASK_INFO pTaskTemp = OSQPend(&g_pOsRes->read_meter_task_q, 0, OS_OPT_PEND_NON_BLOCKING, &msgSize, NULL, &err);
		if (OS_ERR_NONE != err)
			break;

		OSQPost(&g_pOsRes->read_meter_task_q, pTaskTemp, sizeof(READ_TASK_INFO), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);
		if (OS_ERR_NONE != err)
		{
			free_buffer(__func__,pTaskTemp);
			continue;
		}
		if (isMultiTask)
		{
			if (pTaskTemp->task_type!=EM_TASK_TYPE_MULTI_READ_TASK)
			{
				continue;
			}
		}
		else
		{
			if (pTaskTemp->task_type==EM_TASK_TYPE_MULTI_READ_TASK)
			{
				continue;
			}
		}
		if (fliter_taskid)
		{
			int i;
			for (i=0;i<max_num;i++)
			{
				if (pTaskTemp->task_header.task_id==fliter_taskid[i])
				{
					break;
				}
			}
			if (i<max_num)
			{
				continue;
			}
		}
		if (pTaskTemp->task_header.task_mode.priority <= min_priority)
		{
			if (pTaskTemp->task_header.task_mode.priority < min_priority)//出现新的优先级
			{
				find_diff_mac=0;
			}
			min_priority = pTaskTemp->task_header.task_mode.priority;
			if (taskmac == NULL)
			{
                          pReadTask = pTaskTemp;
						  min_read_time=MAX_READ_TIME+1;
			}
			else if (taskmac && (!find_diff_mac))
			{
				int i;
				u8 task_des_mac[LONG_ADDR_LEN];
				u8 reader_s=PLC_GetAppDstMac(pTaskTemp->mac_addr, task_des_mac);
				for (i=0;i < max_num; i++)
				{
					if (macCmp(taskmac[i], task_des_mac))//找到了相同mac地址
					{
						break;
					}
				}
				if (i>=max_num)
				{
					if(reader_s<=min_read_time)
					{
						min_read_time=min_read_time;
						pReadTask = pTaskTemp;
						find_diff_mac=1;
					}
				}
			}
		}
	}

	if (NULL == pReadTask)
	{
		OSSchedUnlock(&err);
		return FALSE;
	}

	u16 buffer_len = sizeof(READ_TASK_INFO) - 1 + pReadTask->task_header.task_len;
	*ppReadTask = (P_READ_TASK_INFO)alloc_buffer(__func__,buffer_len);

	if (NULL != (*ppReadTask))
	{
		memcpy_special((*ppReadTask), pReadTask, buffer_len);
		pReadTask->state = EM_READ_TASK_SENDED;
	}

	OSSchedUnlock(&err);

	return (BOOL)(NULL != (*ppReadTask));
}

//p_task_data: 抄读任务的数据，或者采集任务下发的参数 P_COLL_TASK_INFO
u8 PLC_add_read_meter_task(u8 sour_frome,u8 meter_moudle, u8 no_response_flag, u8 mac_addr[MAC_ADDR_LEN], EM_TASK_TYPE task_type, P_READ_TASK_HEADER pReadTask, u8 *p_rt_err)
{
	OS_ERR err;
	OS_MSG_SIZE msgSize;

	*p_rt_err = RT_ERR_NONE;

#if defined(NW_TEST)

    
    if(g_read_mete_time!=0 )
    {     
         OS_TICK_64 tick_diff =TicksBetween64(GetTickCount64(), g_read_mete_time);//机台测试 5-20s为过衰减测试 
         debug_str(DEBUG_LOG_INFO,"PLC_add_read_meter_task seq: %d,sec:%d,current:%d\r\n",g_pRelayStatus->packet_seq_read_meter,tick_diff/OSCfg_TickRate_Hz,BPLC_GetTxGain());
        if (tick_diff >= 5*OSCfg_TickRate_Hz && tick_diff<= 30*OSCfg_TickRate_Hz  && g_nw_test_atten)
        {   
          if(MAX_HPLC_POWER[BPLC_TxFrequenceGet()] != BPLC_GetTxGain())
          {
            BPLC_SetTxGain(MAX_HPLC_POWER[BPLC_TxFrequenceGet()]);
            debug_str(DEBUG_LOG_INFO,"read_meter_task ++++ SET_MAX_HPLC_POWER %d\r\n",MAX_HPLC_POWER[BPLC_TxFrequenceGet()]);
          }
        }
        else
        {
            //恢复
          if(HPLC_ChlPower[BPLC_TxFrequenceGet()] != BPLC_GetTxGain() && g_nw_test_high_power==0)
          {
            BPLC_SetTxGain(HPLC_ChlPower[BPLC_TxFrequenceGet()]);
            debug_str(DEBUG_LOG_INFO,"read_meter_task ++ resume_HPLC_POWER %d\r\n",HPLC_ChlPower[BPLC_TxFrequenceGet()]);
          }
        }
    }
    g_read_mete_time = GetTickCount64();
    if(PlcChceckTimerId == INVAILD_TMR_ID)
        PlcChceckTimerId =HPLC_RegisterTmr(PLC_Chceck_time_from5to20, NULL, 5 * 1000, TMR_OPT_CALL_PERIOD); //检测是否恢复
#endif
	OS_MSG_QTY count = g_pOsRes->read_meter_task_q.MsgQ.NbrEntries;

	if (count >= READ_TASK_SIZE)
	{
		*p_rt_err = RT_ERR_TASK_MEMORY_NOT_ENOUGH;
		return (!OK);
	}
	OSSchedLock(&err);
	while (count--)
	{
		P_READ_TASK_INFO pTaskTemp = OSQPend(&g_pOsRes->read_meter_task_q, 0, OS_OPT_PEND_NON_BLOCKING, &msgSize, NULL, &err);
		if (OS_ERR_NONE != err)
			break;

		if (pReadTask->task_id == pTaskTemp->task_header.task_id)
			*p_rt_err = RT_ERR_TASK_ID_DUPLICATE;

		OSQPost(&g_pOsRes->read_meter_task_q, pTaskTemp, sizeof(READ_TASK_INFO), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);
		if (OS_ERR_NONE != err)
			free_buffer(__func__,pTaskTemp);

//        if(RT_ERR_NONE != (*p_rt_err))
//            break;
	}
	OSSchedUnlock(&err);
	if (RT_ERR_NONE != (*p_rt_err))
		return (!OK);

	P_READ_TASK_INFO pTaskInfo = (P_READ_TASK_INFO)alloc_buffer(__func__,sizeof(READ_TASK_INFO) + pReadTask->task_len);
	if (NULL == pTaskInfo)
	{
		*p_rt_err = RT_ERR_TASK_MEMORY_NOT_ENOUGH;
		return (!OK);
	}
	if (task_type==EM_TASK_TYPE_MULTI_READ_TASK)
	{
		pTaskInfo->p_sta = (multi_sta_head*)alloc_buffer(__func__,sizeof(multi_sta_head));
		if (NULL==pTaskInfo->p_sta)
		{
			free_buffer(__func__,pTaskInfo);
			*p_rt_err = RT_ERR_TASK_MEMORY_NOT_ENOUGH;
			return (!OK);
		}
		pTaskInfo->tick_add = GetTickCount64();
		pTaskInfo->task_type = task_type;
		pTaskInfo->state = EM_READ_TASK_INIT;
		pTaskInfo->meter_moudle = 0;
		memcpy_special(&pTaskInfo->task_header, pReadTask, sizeof(READ_TASK_HEADER));

		P_MULTI_READ_TASK_HEADER p_head=(P_MULTI_READ_TASK_HEADER)pReadTask;
		u8 *p_data;
		if (p_head->sta_num!=0xffff)
		{
			p_data=&p_head->mac[p_head->sta_num*MAC_ADDR_LEN];
		}
		else
		{
			p_data=p_head->mac;
		}
		pTaskInfo->task_header.task_timeout = p_data[0] | (p_data[1] << 8);
		pTaskInfo->task_header.task_len = p_data[2];
		memcpy_special(pTaskInfo->task_buffer, &p_data[3], pTaskInfo->task_header.task_len);
		//配置节点位图
		memset(pTaskInfo->p_sta,0,sizeof(multi_sta_head));
		pTaskInfo->p_sta->add_num=p_head->sta_num;
		if (p_head->sta_num!=0xffff)
		{
			for (int i = 0; i < p_head->sta_num; i++)
			{
				u16 sn=PLC_GetWhiteNodeSnByMac(&p_head->mac[i*MAC_ADDR_LEN]);
				if (sn<CCTT_METER_NUMBER)
				{
					pTaskInfo->p_sta->bitmap[sn>>5]|=1<<(sn&0x1f);
				}
			}
		}
		else//全部节点
		{
			u16 metersn;
			for (metersn = 0; metersn < CCTT_METER_NUMBER; metersn++)
			{
				if (CCO_USERD == g_MeterRelayInfo[metersn].status.used)
				{
					pTaskInfo->p_sta->bitmap[metersn>>5]|=1<<(metersn&0x1f);
				}
			}
		}
	}
	else
	{
		pTaskInfo->tick_add = GetTickCount64();
		pTaskInfo->task_type = task_type;
		pTaskInfo->state = EM_READ_TASK_INIT;
		pTaskInfo->meter_moudle = meter_moudle;
		pTaskInfo->no_response_flag = no_response_flag;
		pTaskInfo->sour_frome = sour_frome;
		memcpy_special(pTaskInfo->mac_addr, mac_addr, MAC_ADDR_LEN);
		memcpy_special(&pTaskInfo->task_header, pReadTask, sizeof(READ_TASK_HEADER));
#ifndef GRAPH_COLLECT_EXTEND			
		if(meter_moudle) //转发标识为1时，数据第一个字节表示业务代码，不添加进载波报文中		
#else		
		if(meter_moudle && (is_Extend_ReadCollectGraphPkt(pReadTask->data,pReadTask->task_len) == FALSE)) //转发标识为1时，数据第一个字节表示业务代码，不添加进载波报文中 //曲线扩展支持645曲线读数据
#endif		
		{
			memcpy_special(pTaskInfo->task_buffer, ((PBYTE)pReadTask) + sizeof(READ_TASK_HEADER) + 1, pReadTask->task_len);
		}
		else
		{
			memcpy_special(pTaskInfo->task_buffer, ((PBYTE)pReadTask) + sizeof(READ_TASK_HEADER), pReadTask->task_len);
		}
	}

	OSQPost(&g_pOsRes->read_meter_task_q, pTaskInfo, sizeof(READ_TASK_INFO), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);

	if (OS_ERR_NONE != err)
	{
		free_buffer(__func__,pTaskInfo);
		*p_rt_err = RT_ERR_TASK_MEMORY_NOT_ENOUGH;
		return (!OK);
	}

	return OK;
}

void PLC_read_task_timeout_check_gd()
{
	OS_ERR err;
	OS_MSG_SIZE msgSize;

	OS_MSG_QTY count = g_pOsRes->read_meter_task_q.MsgQ.NbrEntries;
	u8 counter = 0;
	u8 max_post_per_second = 3;

	if (count <= 0)
		return;

	OSSchedLock(&err);
	OS_TICK_64 tick_now = GetTickCount64();

	while (count--)
	{
		P_READ_TASK_INFO pTaskInfo = OSQPend(&g_pOsRes->read_meter_task_q, 0, OS_OPT_PEND_NON_BLOCKING, &msgSize, NULL, &err);
		if (OS_ERR_NONE != err)
			break;

		if (counter < max_post_per_second)
		{
			if (TicksBetween64(tick_now, pTaskInfo->tick_add) >= (OS_TICK_64)OSCfg_TickRate_Hz * pTaskInfo->task_header.task_timeout)
			{
#if 1
				if (EM_TASK_TYPE_MULTI_READ_TASK==pTaskInfo->task_type)
				{
					PLM_ReportTaskStatus_gd(pTaskInfo->sour_frome,pTaskInfo->task_header.task_id, NULL, 0xff);
				}
				else
				{
					if(pTaskInfo->no_response_flag == 0) //需要上报给集中器
					{
					    debug_str(DEBUG_LOG_NET, "ReportTaskStatus(0xFF), id:%d, meter mac:%02x%02x%02x%02x%02x%02x\r\n", pTaskInfo->task_header.task_id,
			                                      pTaskInfo->mac_addr[5], pTaskInfo->mac_addr[4], pTaskInfo->mac_addr[3], 
			                                      pTaskInfo->mac_addr[2], pTaskInfo->mac_addr[1], pTaskInfo->mac_addr[0]);
                        
						PLM_ReportTaskStatus_gd(pTaskInfo->sour_frome,pTaskInfo->task_header.task_id, pTaskInfo->mac_addr, 0xff);
					}
				}
#else
				P_GD_DL645_BODY pDL645_task = PLC_check_dl645_frame(&pTaskInfo->task_buffer[0], pTaskInfo->task_header.task_len);
				if (pDL645_task)
					PLM_ReportTaskStatus_gd(pTaskInfo->task_header.task_id, &pDL645_task->meter_number[0], 0xff);
#endif
				free_buffer(__func__,pTaskInfo);
				if (++counter >= max_post_per_second)
					break;
				else
					continue;
			}
		}

		OSQPost(&g_pOsRes->read_meter_task_q, pTaskInfo, sizeof(READ_TASK_INFO), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);
		if (OS_ERR_NONE != err)
			free_buffer(__func__,pTaskInfo);
	}
	OSSchedUnlock(&err);
}

u8 PLC_delete_read_task_gd(EM_TASK_TYPE task_type, u16 taskid)
{
	u8 res = (!OK);

	OS_ERR err;
	OS_MSG_SIZE msgSize;

	OS_MSG_QTY count = g_pOsRes->read_meter_task_q.MsgQ.NbrEntries;

	if (count <= 0)
		return res;

	OSSchedLock(&err);

	while (count--)
	{
		P_READ_TASK_INFO pTaskInfo = OSQPend(&g_pOsRes->read_meter_task_q, 0, OS_OPT_PEND_NON_BLOCKING, &msgSize, NULL, &err);
		if (OS_ERR_NONE != err)
			break;

		if (pTaskInfo->task_type == task_type && pTaskInfo->task_header.task_id == taskid)
		{
			if (task_type==EM_TASK_TYPE_MULTI_READ_TASK)
			{
				free_buffer(__func__,pTaskInfo->p_sta);
			}
			free_buffer(__func__,pTaskInfo);
			res = OK;
			continue;
		}

		OSQPost(&g_pOsRes->read_meter_task_q, pTaskInfo, sizeof(READ_TASK_INFO), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);
		if (OS_ERR_NONE != err)
			free_buffer(__func__,pTaskInfo);
	}

	OSSchedUnlock(&err);

	return res;
}

u8 PLC_delete_all_read_task_gd(EM_TASK_TYPE task_type)
{
	u8 res = (!OK);

	OS_ERR err;
	OS_MSG_SIZE msgSize;

	OS_MSG_QTY count = g_pOsRes->read_meter_task_q.MsgQ.NbrEntries;

	if (count <= 0)
		return res;

	OSSchedLock(&err);

	while (count--)
	{
		P_READ_TASK_INFO pTaskInfo = OSQPend(&g_pOsRes->read_meter_task_q, 0, OS_OPT_PEND_NON_BLOCKING, &msgSize, NULL, &err);
		if (OS_ERR_NONE != err)
			break;

		if (pTaskInfo->task_type == task_type)
		{
			if (task_type==EM_TASK_TYPE_MULTI_READ_TASK)
			{
				free_buffer(__func__,pTaskInfo->p_sta);
			}
			free_buffer(__func__,pTaskInfo);
			res = OK;
		}
		else
		{
			OSQPost(&g_pOsRes->read_meter_task_q, pTaskInfo, sizeof(READ_TASK_INFO), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);
			if (OS_ERR_NONE != err)
				free_buffer(__func__,pTaskInfo);
		}
	}

	OSSchedUnlock(&err);

	return res;
}

//上报从节点信息
void PLC_report_node_ifo_gd(P_MF_RELAY_REG_METER_INFO pRptMeterIfo)
{
	P_MSG_INFO pMsgIfo;
	P_MSG_INFO pMsgData;
	PBYTE pDataUnit;
	int i, p = 0;
	BYTE nodeCounter = 0;

	if (pRptMeterIfo->meterCount <= 0)
		return;

	if (!(pMsgData = (P_MSG_INFO)alloc_send_buffer(__func__,MSG_SHORT)))
	{
		//Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);
		return;
	}

	pDataUnit = &pMsgData->msg_buffer[0];

	p++;

	for (i = 0; i < pRptMeterIfo->meterCount; i++)
	{
		P_MF_RELAY_METER_ITEM pItem = &pRptMeterIfo->Items[i];

		if (IsMemSet(pItem->meterAddr, 0xAA, FRM_DL645_ADDRESS_LEN))
			continue;

		if (IsMemSet(pItem->meterAddr, 0x99, FRM_DL645_ADDRESS_LEN))
			continue;

		memcpy_special(&pDataUnit[p], pItem->meterAddr, FRM_DL645_ADDRESS_LEN);
		p += 6;
		nodeCounter++;
	}

	if (0 == nodeCounter)
	{
		free_send_buffer(__func__,pMsgData);
		return;
	}

	pDataUnit[0] = nodeCounter;

	//pMsgIfo = LMP_make_frame_3762_cmd(AFN_REPORT_INFO, F1, PLM_PHASE_A, pDataUnit, p, FALSE, NULL);
	pMsgIfo = LMP_make_frame_gd_cmd(0x05, 0xE8050503, pDataUnit, p, NULL);

	free_send_buffer(__func__,pMsgData);

	if (!pMsgIfo)
	{
		return;
	}

	pMsgIfo->msg_header.end_id = COM_PORT_MAINTAIN;
	pMsgIfo->msg_header.need_buffer_free = TRUE;
	pMsgIfo->msg_header.send_delay = false;
	End_post(pMsgIfo);
}


void PLC_report_router_register_finish_gd()
{
	P_MSG_INFO p_CccttSendMsg;

	p_CccttSendMsg = LMP_make_frame_gd_cmd(0x05, 0xE8050504, NULL, 0, NULL);
	if (NULL != p_CccttSendMsg)
		End_post(p_CccttSendMsg);
}
#ifdef PLC_X4_FUNC
u8 PLC_X4GetReadTaskInfoLen(P_X4_READ_TASK_INFO p_read_task)
{
	return (u8)(OffsetOf(X4_READ_TASK_INFO, di_data[0]) + p_read_task->di_count * 6 + 2);
}


P_X4_READ_TASK_INFO PLC_X4FindReadTaskInfo(u8 task_id)
{
	for (u8 j = 0; j < CCTT_X4_READ_TASK_INFO_NUMBER; j++)
	{
		P_X4_READ_TASK_RECORD pRecord = &g_X4ReadTaskInfo[j];
		if (CCO_USERD != pRecord->status)
			continue;

		if (task_id == pRecord->task.task_id)
			return &pRecord->task;
	}

	return NULL;
}

P_X4_READ_TASK_INFO PLC_X4FindNodeReadTaskInfo(u8 node_addr[MAC_ADDR_LEN], u8 task_id)
{
	u16 node_sn = PLC_get_sn_from_addr(node_addr);
	if (node_sn >= CCTT_METER_NUMBER)
		return NULL;

	P_RELAY_INFO pNode = &g_MeterRelayInfo[node_sn];

	if (pNode->task_id_num > CCTT_X4_READ_TASK_NUM_PER_METER)
		return NULL;

	for (u8 i = 0; i < pNode->task_id_num; i++)
	{
		if (pNode->task_id_array[i] != task_id)
			continue;

		for (u8 j = 0; j < CCTT_X4_READ_TASK_INFO_NUMBER; j++)
		{
			P_X4_READ_TASK_RECORD pRecord = &g_X4ReadTaskInfo[j];
			if (CCO_USERD != pRecord->status)
				continue;

			if (task_id == pRecord->task.task_id)
				return &pRecord->task;
		}
	}

	return NULL;
}

u8 PLC_X4ReadTaskIsEqual(P_X4_READ_TASK_INFO p_read_task1, P_X4_READ_TASK_INFO p_read_task2)
{
	if (NULL == p_read_task1 || NULL == p_read_task2)
		return (!OK);

	u8 len1 = PLC_X4GetReadTaskInfoLen(p_read_task1);
	u8 len2 = PLC_X4GetReadTaskInfoLen(p_read_task2);

	if ((len1 == len2) && (len1 <= sizeof(X4_READ_TASK_INFO)) && (OK == IsEqual((u8 *)p_read_task1, (u8 *)p_read_task2, len1)))
		return OK;

	return (!OK);
}

u8 PLC_X4IsReadTaskCfg(u8 node_addr[MAC_ADDR_LEN], P_X4_READ_TASK_INFO p_read_task)
{
	P_X4_READ_TASK_INFO pReadTask = PLC_X4FindNodeReadTaskInfo(node_addr, p_read_task->task_id);

	if (NULL == pReadTask)
		return (!OK);

	return PLC_X4ReadTaskIsEqual(p_read_task, pReadTask);
}
#endif
#endif

BYTE PLC_freeze_save_data_645(USHORT read_sn, PBYTE pMeterNo, BYTE ctrl, PBYTE pDataUnit645, BYTE nDataUnitLen, BYTE phase, BYTE protocol)
{
	P_MSG_INFO pRespMsg;
	P_GD_DL645_BODY pDL645Resp;

	if (pRespMsg = alloc_send_buffer(__func__,MSG_SHORT))
	{
		pRespMsg->msg_header.meter_sn = read_sn;
		pRespMsg->msg_header.protocol = protocol;   //GD_DOWN_PROTOCOL_DL97;
		pRespMsg->msg_header.pos_dl645_head = 1;
		//p_PlcMsg->msg_header.meter_moudle=0;
		pDL645Resp = (P_GD_DL645_BODY)pRespMsg->msg_buffer;
		PLC_make_frame_645(pDL645Resp, pMeterNo, ctrl, pDataUnit645, nDataUnitLen);

		BSP_OS_TimeDly(200);
		pRespMsg->msg_header.phase = phase;
		PLC_freeze_save_data(pRespMsg);

		free_send_buffer(__func__,pRespMsg);
		return OK;
	}

	return (!OK);
}

BYTE PLC_is_freezedate_today(PBYTE freezedate)
{
	if (g_PlmStatusPara.t_now_sec < CCTT_INVALID_UNIXTIME)
	{
		struct tm now;
		__time64_t tt=g_PlmStatusPara.t_now_sec;
		now=*gmtime(&tt);

		if ((now.tm_year % 100) != Bcd2HexChar(freezedate[4] - 0x33))
			return (!OK);

		if ((now.tm_mon) != (Bcd2HexChar(freezedate[3] - 0x33)-1))
			return (!OK);

		if ((now.tm_mday) != Bcd2HexChar(freezedate[2] - 0x33))
			return (!OK);

		return OK;
	}

	return OK;
}

BYTE PLC_is_freezedate_correct(ULONG DI)
{
	//33 33 38 3A 4B
	//18-07-05 00:00

	if (DI_FREEZE_DATE_LST1_07 == DI)
	{
		if (!g_PlmStatusPara.meter_freeze_time_lst1_flag)
			return (!OK);

		if (g_PlmStatusPara.t_now_sec < CCTT_INVALID_UNIXTIME)
		{
			if (OK == PLC_is_freezedate_today(g_PlmStatusPara.meter_freeze_time_lst1))
			{
				return OK;
			}
			else
			{
				PLC_ClearCacheData();
				return (!OK);
			}
		}
	}

	return (!OK);
}

//免抄读，直接post点抄数据给集中器
BYTE PLC_dirread_postabled(USHORT read_sn, PBYTE pCmd, USHORT cmd_len)
{
	P_GD_DL645_BODY pDL645Cmd = PLC_check_dl645_frame(pCmd, cmd_len);
	P_GD_DL645_BODY pDL645Resp = NULL;
	P_MSG_INFO pResp645Msg;
	ULONG DI;
	BYTE dataUnit[80];

	if (NULL == pDL645Cmd)
		return (!OK);

	if (1 == g_MeterRelayInfo[read_sn].meter_type.cjq_with_add)
		return ERROR;

	DI = PLC_GetDIByDL645(pDL645Cmd);

	if (DI_FREEZE_DATE_LST1_07 == DI)
	{
		if (OK != PLC_is_freezedate_correct(DI))
			return ERROR;

		pResp645Msg = alloc_send_buffer(__func__,MSG_SHORT);
		if (NULL == pResp645Msg)
			return ERROR;

		pDL645Resp = (P_GD_DL645_BODY)pResp645Msg->msg_buffer;
		memcpy_special(&dataUnit[0], pDL645Cmd->dataBuf, pDL645Cmd->dataLen);
		memcpy_special(&dataUnit[pDL645Cmd->dataLen], &g_PlmStatusPara.meter_freeze_time_lst1[0], 5);
		AddValue(dataUnit, pDL645Cmd->dataLen + 5, PLC_645_MINUS_0X33);

		PLC_make_frame_645(pDL645Resp, pDL645Cmd->meter_number, 0x91, dataUnit, pDL645Cmd->dataLen + 5);
		PLC_dirread_response(g_MeterRelayInfo[read_sn].meter_type.protocol, pDL645Resp);
		free_send_buffer(__func__,pResp645Msg);

		return OK;
	}
	else if ((DI_ACT_POSI_ENER_LAST_DAY_BLK_07 == DI)
			 || (DI_ACT_NEGA_ENER_LAST_DAY_BLK_07 == DI)
			 || (DI_REA_POSI_ENER_LAST_DAY_BLK_07 == DI)
			 || (DI_REA_NEGA_ENER_LAST_DAY_BLK_07 == DI))
	{
		PBYTE pDataUnit645;
		BYTE dataUnitLen645;

		if (OK != PLC_is_freezedate_correct(DI_FREEZE_DATE_LST1_07))
			return ERROR;

		if (OK != PLC_GetMeterDataByDL645(pDL645Cmd, &pDataUnit645, &dataUnitLen645))
			return ERROR;

		if (dataUnitLen645 > sizeof(dataUnit))
			return ERROR;

		pResp645Msg = alloc_send_buffer(__func__,MSG_SHORT);
		if (NULL == pResp645Msg)
			return ERROR;

		pDL645Resp = (P_GD_DL645_BODY)pResp645Msg->msg_buffer;
		memcpy_special(&dataUnit[0], pDataUnit645, dataUnitLen645);
		AddValue(&dataUnit[0], dataUnitLen645, PLC_645_MINUS_0X33);

		PLC_make_frame_645(pDL645Resp, pDL645Cmd->meter_number, 0x91, dataUnit, dataUnitLen645);
		PLC_dirread_response(g_MeterRelayInfo[read_sn].meter_type.protocol, pDL645Resp);
		free_send_buffer(__func__,pResp645Msg);

#if defined(APP_CONCENTRATOR_MODE)
		if (DI_ACT_POSI_ENER_LAST_DAY_BLK_07 == DI)
			MF_RELAY_SET_FREEZE_05060101(read_sn);
#endif

		return OK;
	}

	return (!OK);
}

unsigned char PLC_Is_MainNodeAddr(unsigned char *pAddr6)
{
	if (memcmp(pAddr6, g_PlmConfigPara.main_node_addr, 6) == 0)
		return 1;

	return 0;
}

void Relay_Task_PauseAWhile()
{
	if (0xff != g_pRelayStatus->pause_sec)
		g_pRelayStatus->pause_sec = 180;
}

void Reboot_system(unsigned char sec, unsigned char reboot_type, const char* func_name)
{
	debug_str(DEBUG_LOG_ERR, "Reboot call by %d\r\n", func_name);
	
	if (0 == g_pRelayStatus->reboot_sec_down)
		g_pRelayStatus->reboot_sec_down = sec;
}
void Reboot_system_now(const char* func_name)
{
	OS_ERR err;

	debug_str(DEBUG_LOG_ERR, "Reboot Now call by %s\r\n", func_name);
	OSTimeDly(10, OS_OPT_TIME_DLY, &err);
		
	__disable_irq();
	#ifndef __DEBUG_MODE
	WDG_ShortReset();
	#endif
	while (1)
	{
		#ifdef __DEBUG_MODE
		FeedWdg();
		#endif
	}
}

#if 0
void test_change_net(void)
{
	/*构建树      */
	int i;
	__disable_irq();
	g_StaInfo[1].child=2;
	g_StaInfo[2].pco=1;
	g_StaInfo[2].brother = 0;
	g_StaInfo[2].child = 3;
	g_StaInfo[2].used=CCO_USERD;
	for (i=3;i<100;i++)
	{
		g_StaInfo[i].used=CCO_USERD;
		g_StaInfo[i].brother=i+1;
		g_StaInfo[i].pco=2;
		g_StaInfo[i].child=0;
	}
	g_StaInfo[100].used=CCO_USERD;
	g_StaInfo[100].pco=1;
	g_StaInfo[100].child=101;
	g_StaInfo[100].brother=0;
	for (int i=101;i<108;i++)
	{
		g_StaInfo[i].used=CCO_USERD;
		g_StaInfo[i].child=0;
		g_StaInfo[i].pco=100;
		g_StaInfo[i].brother=i+1;
	}
	g_StaInfo[108].used=CCO_USERD;
	g_StaInfo[108].pco=100;
	g_StaInfo[108].child=109;
	g_StaInfo[108].brother=0;
	for (i=109;i<120;i++)
	{
		g_StaInfo[i].used=CCO_USERD;
		g_StaInfo[i].child=0;
		g_StaInfo[i].pco=108;
		g_StaInfo[i].brother=i+1;
	}
	g_StaInfo[120].used=CCO_USERD;
	g_StaInfo[120].pco=108;
	g_StaInfo[120].child=121;
	g_StaInfo[120].brother=0;
	for (i=121;i<125;i++)
	{
		g_StaInfo[i].used=CCO_USERD;
		g_StaInfo[i].child=0;
		g_StaInfo[i].pco=120;
		g_StaInfo[i].brother=i+1;
	}
	g_StaInfo[i].used=CCO_USERD;
	g_StaInfo[125].pco=120;
	g_StaInfo[125].child=0;
	g_StaInfo[125].brother=0;

	g_StaInfo[80].child=126;
	for (i=126;i<=129;i++)
	{
		g_StaInfo[i].pco=80;
		g_StaInfo[i].used=CCO_USERD;
		g_StaInfo[i].child=0;
		if (i==129)
		{
			g_StaInfo[i].brother = 0;
		}
		else
		{
			g_StaInfo[i].brother = i + 1;
		}
	}

	P_RELAY_PATH p_relay_path_sta = NULL;
	P_GET_BEACON pStaInfo = NULL;

	p_relay_path_sta = (P_RELAY_PATH)alloc_buffer(__func__,sizeof(RELAY_PATH));

	pStaInfo = (P_GET_BEACON)alloc_buffer(__func__,sizeof(GET_BEACON));
	memset(pStaInfo,0,sizeof(GET_BEACON));
	TraversalBeaconTei(3, 100, p_relay_path_sta, pStaInfo);
	TraversalBeaconTei(2, 10, p_relay_path_sta, pStaInfo);
	TraversalBeaconTei(2, 10, p_relay_path_sta, pStaInfo);
	//测试函数
	PLC_GetAllSubStas(2);
	ASSOC_REQ_PACKET AssocReq;
	PLC_AssocCnfOK(&AssocReq,2, 15);
}
#endif

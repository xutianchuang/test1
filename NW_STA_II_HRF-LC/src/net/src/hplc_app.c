#include "os.h"
#include <string.h>
#include "protocol_includes.h"
#include "applocal.h"
#include "bsp.h"
#include "Revision.h"


#include "lc_all.h"

#ifdef II_STA
#include "app_search_meter.h"
#include "app_event_IIcai.h"
#include "app_RF_IIcai.h"
#include "app_led_IIcai.h"
#include "bps_ir_uart.h"
#endif

#ifdef I_STA
#include "plm_2005.h"
#endif

#include "app_event_IIcai.h"
#include "app_led_IIcai.h"
// 集中器并发抄表/数据汇集无响应超时时间
#define MAX_ERUP_READMETER_TIMEOUT 2000

// 集中器并发抄表/数据汇集抄表数据最大长度
#define MAX_ERUP_READMETER_SIZE 2000

// 读取本地事件超时时间
#define MAX_READ_EVENT_TIME 3000

// 等待CCO回复
#define MAX_WAIT_EVENT_ACK_TIME 20 * 1000

// 最大的等待开始发送停复电的时间
#define MAX_POWER_EVENT_DECETC_TIME 30 * 1000UL

// EVENT帧重发次数
#define EVENT_RESEND_TIMES 2

// 进入测试模式时间
#define ENTER_TEST_TIME 40 * 1000ul

// 最大的等待开始发送停复电的时间
#define MAX_EVENT_DECETC_TIME 30 * 1000UL

#define MAX_EVENT_ROUNT 2

typedef enum
{
	APP_IDLE_STATE,		// 空闲状态
	APP_READING_STATE,	// 正在抄表
	APP_WAIT_ACK_STATE, // 等待确认
	APP_REGISTER_STATE, // 从节点主动注册
} APP_CURRENT_STATE;

// 停电事件状态
typedef enum
{
	POWER_EVENT_IDLE,	   // 空闲状态
	POWER_EVENT_WAIT_TIME, // 等待发送状态
	POWER_EVENT_WAIT_ACK,
	POWER_EVENT_OVERFLOW, // CCO回复满，等待重复
	POWER_EVENT_NO_ACK,	  // CCO没有回复，等待半小时再发
	POWER_EVENT_WAIT_END, // 等待发完10帧，结束发送
} APP_POWER_EVENT_STATE;

// 停电事件上报功能码
typedef enum
{
	POWER_EVENT_FUNC_CCO_CONFIRM = 1,		// CCO应答确认给STA
	POWER_EVENT_FUNC_TRIGGER_BY_MODULE = 1, // STA主动上报事件给CCO(模块触发)
	POWER_EVENT_FUNC_TRIGGER_BY_COLLECT = 2 // STA主动上报事件给CCO(采集器触发)
} APP_POWER_EVENT_FUNC;

// STA上报停电事件类型
typedef enum
{
	POWER_EVENT_TYPE_OFF_BIT = 1, // 停电事件(位图)
	POWER_EVENT_TYPE_ON_BIT = 2,  // 上电事件(位图)
	POWER_EVENT_TYPE_OFF_MAC = 3, // 停电事件(MAC)
	POWER_EVENT_TYPE_ON_MAC = 4	  // 上电事件(MAC)
} APP_POWER_EVENT_TYPE;

// app序号
static u16 AppSequence = 0;

// 当前处于什么测试模式
static u8 AppCurrentModel = APP_TEST_NORMAL;

// 当前读取事件状态
APP_EVENT_STATE AppEventState = APP_EVENT_IDLE;

static u16 EventPacketID = 1;	   // 事件报文序号
static u16 meterPacketID = 0xffff; // 事件主动上报序号
// static u16 powerPacketID=0xffff; //停电事件上报序号
static u16 powerUpPacketID = 0xffff; // 复电事件上报序号，为了匹配CCO确认帧

// 事件通知信号量
OS_SEM EventNotificationSem;

// 读事件超时定时器
static TimeValue EventTimer = {0, 0};

// 是否允许上报事件
u8 EventReportEnable = true;

// 事件上报用于重发的PDU
SEND_PDU_BLOCK *EventPdu = NULL;

// 停电上电事件相关变量
OS_Q PowerEventQ;												   // 停电事件通知消息队列
static SEND_PDU_BLOCK *PowerEventPdu = NULL;					   // 停电事件上报用于重发的PDU
static APP_POWER_EVENT_STATE CurrentPowerState = POWER_EVENT_IDLE; // 当前处理的停电状态
static TimeValue PowerEventTimer = {0, 0};						   // 停电事件超时定时器
static u8 signalPowerEvent = 0;									   // 0 为复电事件  1为停电事件
static u8 PowerOffEventBitmap[MAX_BITMAP_SIZE];					   // 停电事件TEI位图
static u8 PowerOffMac[MAX_TEI_VALUE][7];						   // 停电事件MAC表
static u16 PowerOffNum = 0;										   // 发生停电事件mac的个数
static u16 PowerOffTEI = 0;										   // 发生停电事件最小TEI
static u16 PowerUpTEI = 0;										   // 停电事件上报上行目的TEI
static u16 PowerUpSendType = 0;									   // 停电事件上报上行发送类型
extern ST_STA_SYS_TYPE StaSysPara;

#ifdef CHECK_FREQ_NO_REBOOT
const u8 check_freq_no_reboot = 1; // 不复位
#else
const u8 check_freq_no_reboot = 0;						// 复位
#endif

#ifdef CHECK_MAC_HEAD_VER
const u8 check_mac_head_ver = 1; // 判断MAC帧头中版本号
#else
const u8 check_mac_head_ver = 0;						// 不判断MAC帧头中版本号
#endif

// 当前是否允许进入测试模式
bool EnterTestModel = true;

// 测试模式使用参数
_AppCommTestParm Test_Parm;

static u32 TestModelTmrID = 0xFFFFFFFF;

#ifdef GOTO_TEST_LIKE_GW
u32 FreTimeout = DEFULT_CHANGE_FRE_TIME; // 贵州初始使用频段1发送测试帧
#else
u32 FreTimeout = 2 * 1000ul;							// 南网初始时用于探测 测试模式
#endif

// 处理抄表返回
void AppHandleReadMeter(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8 *data, u16 len, u8 sendType, u16 srcTei);

// 抄控器测试帧返回
void AppHandleReaderBack(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8 *data, u16 len, u8 sendType, u16 srcTei);

// 处理事件返回
void AppHandleEvent(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8 *data, u16 len, u8 sendType, u16 srcTei);

void APP_BroadTimeHandle(u8 *data, u16 len);
bool APP_BroadcastTimeExt(u8 *data, u16 len);

bool AppGraphHandle(u8 srcAddr[6], u8 dstAddr[6], u16 srcTei, u16 seq, u8 sendType, u8 *data, u16 len);

void PowerEvent(void *);
u32 isGetedZero = 0;
u32 isGeteZeroPhase[3] = {0, 0, 0};
u32 zeroNtbDiff[3] = {0};
u32 first_phase_ntb[3] = {0, 0, 0};
bool getedZero(u32 ntb, int phase)
{
	++isGeteZeroPhase[phase];
	switch (phase)
	{
	case 0:
		if (first_phase_ntb[0] == 0)
		{
			first_phase_ntb[0] = ntb;
		}
		break;
	case 1:
		if (first_phase_ntb[1] == 0)
		{
			if (first_phase_ntb[0] != 0)
			{
				first_phase_ntb[1] = ntb;
			}
		}
		break;
	case 2:
		if (first_phase_ntb[2] == 0)
		{
			if (first_phase_ntb[0] != 0)
			{
				first_phase_ntb[2] = ntb;
			}
		}
		break;
	}
	// 过零NTB差值，产测命令使用
	if (isGeteZeroPhase[phase] == 20)
	{
		zeroNtbDiff[phase] = ntb;
	}
	else if (isGeteZeroPhase[phase] == 21)
	{
		zeroNtbDiff[phase] = ntb - zeroNtbDiff[phase];
	}

	isGetedZero++;
	if (isGetedZero > 150) // 30s后删除自身
	{
		return true;
	}
	return false;
}
u8 ZeroSelfStatus(void)
{
	if (myDevicType == 7) // 三相
	{
		int i;
		for (i = 0; i < 3; i++)
		{
			u8 nextidx = (i + 1) % 3;
			u32 diff = first_phase_ntb[i] > first_phase_ntb[nextidx] ? first_phase_ntb[i] - first_phase_ntb[nextidx]
																	 : first_phase_ntb[nextidx] - first_phase_ntb[i];
			if (first_phase_ntb[i] == 0)
			{
				return 3; // 存在断相
			}
			if (diff < (2 * NTB_FRE_PER_MS))
			{
				return 4; // 相同相位
			}
		}
		u32 diff_AB, diff_AC;
		diff_AB = first_phase_ntb[1] - first_phase_ntb[0];
		diff_AC = first_phase_ntb[2] - first_phase_ntb[0];
		if (diff_AB > (20 * NTB_FRE_PER_MS / 3 - 2 * NTB_FRE_PER_MS) && diff_AB < (20 * NTB_FRE_PER_MS / 3 + 2 * NTB_FRE_PER_MS))
		{
			if (diff_AC > (20 * NTB_FRE_PER_MS * 2 / 3 - 2 * NTB_FRE_PER_MS) && diff_AC < (20 * NTB_FRE_PER_MS * 2 / 3 + 2 * NTB_FRE_PER_MS))
			{
				return 1;
			}
		}
		if (diff_AB > (20 * NTB_FRE_PER_MS * 2 / 3 - 2 * NTB_FRE_PER_MS) && diff_AB < (20 * NTB_FRE_PER_MS * 2 / 3 + 2 * NTB_FRE_PER_MS))
		{
			if (diff_AC > (20 * NTB_FRE_PER_MS / 3 - 2 * NTB_FRE_PER_MS) && diff_AC < (20 * NTB_FRE_PER_MS / 3 + 2 * NTB_FRE_PER_MS))
			{
				return 2;
			}
		}
		return 5;
	}
	else
	{
		if (first_phase_ntb[0] == 0)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
}
// 获取APP序号(自动+1)
u16 GetAppSequence(void)
{
	return AppSequence++;
}

// 应用层初始化
void AppInit(void)
{
	OS_ERR err;

	// 事件引脚
#ifndef PROTOCOL_FPGA_VERSION
	EventOpen();
	OSSemCreate(&EventNotificationSem, "EventNotificationSem", 0, &err);

	OSQCreate(&PowerEventQ, "power event Q", 1, &err);
	// 周期查询停电事件
	HPLC_RegisterTmr(PowerEvent, 0, 100, TMR_OPT_CALL_PERIOD);
#endif

#ifndef II_STA
	// 默认允许事件上报
	EventReportEnable = true;
#endif

	// 升级状态机初始化
	UpdateInit(0);

	AppTestModeDeal();
}

// 10秒内处理APP测试帧
void AppTestModeDeal(void)
{
	AppCurrentModel = APP_TEST_NORMAL;
	if (EnterTestModel)
	{
		HPLC_DelTmr(TestModelTmrID);
	}
	EnterTestModel = true;
	TestModelTmrID = HPLC_RegisterTmr(CannotEnterTestModel, 0, ENTER_TEST_TIME, TMR_OPT_CALL_ONCE);
}

extern u32 HPLCCurrentFreq;
// 返回正常模式
void BackToAppNormalModel(void *arg)
{
	AppCurrentModel = APP_TEST_NORMAL;
	// ChangeLocalBaudParity(GetLoaclUartBaud(),Parity_Even);
	LocalEnterTestModel(false);
	BPLC_GoTest(0);
	//	HPLC_PHY_SetFrequence(HPLCCurrentFreq);
}

extern u32 CurrentFreBeaconFcCnt;
extern void StaLastFreqProcess(void);
// 30秒已过，不能进入测试模式
void CannotEnterTestModel(void *arg)
{

#ifndef GOTO_TEST_LIKE_GW
	if (GetStaBaseAttr()->NetState != NET_IN_NET&&
		EnterTestModel &&
		MacGetAllowNet() && APP_TEST_NORMAL == AppCurrentModel
#ifndef CHECK_LAST_FREQ
		&& CurrentFreBeaconFcCnt < 3
#endif
	)
	{
		StaLastFreqProcess();
	}
#endif
	EnterTestModel = false;
	FreTimeout = DEFULT_CHANGE_FRE_TIME; // 不再进入测试模式 timeOUT恢复正常
	TestModelTmrID = 0xffffffff;
}

// 当前通信测试属于什么模式
u8 GetAppCurrentModel(void)
{
	return AppCurrentModel;
}

// mac 转成key
u32 mac2key(u8 *mac)
{
	return CommonGetCrc32(mac, 6);
}

bool PowerMACHashInsert(u8 *mac)
{
	int i = 0;

	if (PowerOffNum > MAX_TEI_VALUE)
	{
		return false;
	}

	// 查找MAC是否已存在
	for (i = 0; i < PowerOffNum; i++)
	{
		if (memcmp(&PowerOffMac[i][0], mac, 6) == 0)
		{
			return false;
		}
	}

	// MAC不存在,直接加在现有个数的下一个
	if (PowerOffNum != MAX_TEI_VALUE)
	{
		memcpy(&PowerOffMac[PowerOffNum][0], mac, 7);
		PowerOffNum++;
		return true;
	}

	return false;
}

int power = 1;
u32 zeroCnt[3] = {0};

bool getZeroCnt(u32 ntb, int phase)
{
	if (phase > 2)
	{
		return false;
	}

	zeroCnt[phase]++;
	if (zeroCnt[phase] > 50 * 30) // 30s后删除自身
	{
		return true;
	}
	return false;
}

// 3个相线过零最大的值
u32 getZeroMaxCnt(void)
{
	if (zeroCnt[0] > zeroCnt[1])
	{
		return ((zeroCnt[0] > zeroCnt[2]) ? zeroCnt[0] : zeroCnt[2]);
	}
	else
	{
		return ((zeroCnt[1] > zeroCnt[2]) ? zeroCnt[1] : zeroCnt[2]);
	}
}

extern TimerParameterTypdef TopoPWMTmr;
bool moduleInsert(void)
{
#if defined(I_STA)
	return false;
#elif !defined(II_STA)
	volatile int i = 1000;
#if (PULLOUT_PORT == TOPO_PORT) && (PULLOUT_PIN == TOPO_PIN)
	PwmGpioFunc(TopoPWMTmr.Timer, 0);
#endif
	InsertOpen();
	while (i--)
	{
	}
	if (InsertRead())
	{
		return false;
	}
	else
	{
		return true;
	}
#else
	return true;
#endif
}

#define SEND_POWER_ON NULL
#define SEND_POWER_OFF ((void *)1)
u16 upPowerOnTmrID = INVAILD_TMR_ID;
u16 upPowerOffTmrID = INVAILD_TMR_ID;
u16 upPowerMacClearTmrID = INVAILD_TMR_ID;
u16 upPowerBitClearTmrID = INVAILD_TMR_ID;
static u32 stopPowerOffTime = 0;
void clear_mac_map(void *por)
{
	upPowerMacClearTmrID = INVAILD_TMR_ID;
	memset(PowerOffMac, 0, sizeof(PowerOffMac));
}
void clear_bit_map(void *arg)
{
	upPowerBitClearTmrID = INVAILD_TMR_ID;
	memset(PowerOffEventBitmap, 0, sizeof(PowerOffEventBitmap));
	debug_str(DEBUG_LOG_APP, "clear bitmap!\r\n");
}
void upPowerEvent(void *por)
{
	#if 0
	static u8 sendTime = 0;
	u8 addr[6] = {0};
	int len = 0;

	if (por == SEND_POWER_ON) // 复电事件
	{
		upPowerOnTmrID = INVAILD_TMR_ID;
		PowerUpTEI = CCO_TEI;
		PowerUpSendType = SENDTYPE_SINGLE;
	}
	else // 停电事件
	{
		upPowerOffTmrID = INVAILD_TMR_ID;
#if 1
		if (power) // 未停电STA都以单播形式上报
#else
		if ((CCO_TEI == GetRelayTEI(CCO_TEI)) && power) // 是一级站点并且本站点有电
#endif
		{
			PowerUpTEI = CCO_TEI;
			PowerUpSendType = SENDTYPE_SINGLE;
		}
		else
		{
			PowerUpTEI = MAC_BROADCAST_TEI;
			PowerUpSendType = SENDTYPE_LOCAL;
		}
	}

	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	if (pNetBasePara->NetState != NET_IN_NET)
	{
		CurrentPowerState = POWER_EVENT_IDLE;
		return;
	}

	memcpy(addr, pNetBasePara->Mac, 6);
	TransposeAddr(addr); // 地址转成大端

	SEND_PDU_BLOCK *pdu = MallocSendPDU();
	if (pdu == NULL)
	{
		if (por == SEND_POWER_ON)
		{
			upPowerOnTmrID = HPLC_RegisterTmr(upPowerEvent, SEND_POWER_ON, (rand() & 0xff) + 10, TMR_OPT_CALL_ONCE);
		}
		else
		{
			upPowerOffTmrID = HPLC_RegisterTmr(upPowerEvent, SEND_POWER_OFF, (rand() & 0xff) + 10, TMR_OPT_CALL_ONCE);
		}
		return;
	}

	// 应用报文头
	APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
	// 应用层业务报文头
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];
	// 停电事件上报报文头
	APP_POWER_EVENT_REPORT *powerEventUp = (APP_POWER_EVENT_REPORT *)&sendAppGeneralPacket->Data[0];

	// 生成应用层报文头
	CreateAppPacket(sendAppPacket);
	sendAppPacket->Port = 0x13; // 停电事件上报端口号为0x13

	// 生成停电事件上报报文数据
	if (por == SEND_POWER_ON) // 复电事件
	{
		POWER_EVENT_UP_MAC *pMacUp = (POWER_EVENT_UP_MAC *)powerEventUp->Data;
		pMacUp->upType = POWER_EVENT_TYPE_ON_MAC;
#if defined(II_STA) && !defined(PROTOCOL_NW_2021_GUANGDONG)
		u8 i, j = 0;
		for (i = 0; i < 6; i++)
		{
			if (!memIsHex(&list_flash_power_off[i * 7], 0, 6))
			{
				memcpy(&pMacUp->data[j][0], (u8 *)&list_flash_power_off[i * 7], 7); // memcpy(&pMacUp->data[j][0]+j*7,(u8 *)&list_flash_power_off[i*7],7);//
				j++;
			}
		}
		pMacUp->num = j;

		len = 1 + 2 + j * 7;
#else
		pMacUp->num = 1;
		memcpy(&pMacUp->data[0][0], addr, 6);
		pMacUp->data[0][6] = 1; // 有电
		len = 1 + 2 + 1 + 6;
#endif
		signalPowerEvent = 0;
		debug_str(DEBUG_LOG_APP, "up poweron event\r\n");
	}
	else // 停电事件
	{
		sendTime++;
		if (((sendTime > 30) && (PowerUpSendType == SENDTYPE_LOCAL)) ||
			((sendTime > 5) && (PowerUpSendType == SENDTYPE_SINGLE)))
		{
			memset(PowerOffEventBitmap, 0, sizeof(PowerOffEventBitmap));
			memset(PowerOffMac, 0, sizeof(PowerOffMac));
			debug_str(DEBUG_LOG_APP, "time clear bitmap!\r\n");
			PowerOffNum = 0;
			PowerOffTEI = 0;
			sendTime = 0;
			stopPowerOffTime = 2 * 60 * 10;
			FreeSendPDU(pdu);
			return;
		}
		if (upPowerMacClearTmrID == INVAILD_TMR_ID)
		{
			upPowerMacClearTmrID = HPLC_RegisterTmr(clear_mac_map, 0, 2 * 60 * 1000, TMR_OPT_CALL_ONCE);
		}
		else
		{
			HPLC_DelTmr(upPowerMacClearTmrID);
			upPowerMacClearTmrID = HPLC_RegisterTmr(clear_mac_map, 0, 2 * 60 * 1000, TMR_OPT_CALL_ONCE);
		}
		if (upPowerBitClearTmrID == INVAILD_TMR_ID)
		{
			upPowerBitClearTmrID = HPLC_RegisterTmr(clear_bit_map, 0, 2 * 60 * 1000, TMR_OPT_CALL_ONCE);
		}
		else
		{
			HPLC_DelTmr(upPowerBitClearTmrID);
			upPowerBitClearTmrID = HPLC_RegisterTmr(clear_bit_map, 0, 2 * 60 * 1000, TMR_OPT_CALL_ONCE);
		}
		debug_str(DEBUG_LOG_APP, "up poweroff event\r\n");
		if (sendTime & 0x1) // 奇数次发bitmap
		{
			if (PowerOffTEI != 0)
			{
				u8 bitmap_byte;
				POWER_EVENT_UP_BITMAP *pBitUp = (POWER_EVENT_UP_BITMAP *)powerEventUp->Data;
				pBitUp->upType = POWER_EVENT_TYPE_OFF_BIT;
				pBitUp->startTei = 0;
				u16 max_len;
				max_len = (PowerOffTEI) / 8 + 1;
				u16 idx = 0;
				u16 left_idx;
				debug_str(DEBUG_LOG_APP, "power bitmap is");
				debug_hex(DEBUG_LOG_APP, PowerOffEventBitmap, 50);
				if (max_len > sizeof(PowerOffEventBitmap))
				{
					max_len = sizeof(PowerOffEventBitmap);
				}
				u16 bitmap_start = 0, bitmap_end = 0;
				for (int i = 0; i < max_len; i++)
				{
					if (pBitUp->startTei == 0)
					{
						if (PowerOffEventBitmap[i] == 0)
						{
							continue;
						}
						for (int j = 0; j < 8; j++)
						{
							if (PowerOffEventBitmap[i] & (1 << j))
							{
								bitmap_start = i;
								pBitUp->startTei = (i << 3) | j;
								left_idx = pBitUp->startTei % 8;
								break;
							}
						}
					}
					if (i < sizeof(PowerOffEventBitmap) - 1)
					{
						bitmap_byte = (PowerOffEventBitmap[i] >> left_idx) | (PowerOffEventBitmap[i + 1] << (8 - left_idx));
					}
					else
					{
						bitmap_byte = PowerOffEventBitmap[i] >> left_idx;
					}
					if (bitmap_byte)
					{
						bitmap_end = i;
					}
					pBitUp->bitMap[idx++] = bitmap_byte;
				}
				len += bitmap_end - bitmap_start + 1;
				len += 3;
			}
		}
		else // 偶数次发mac
		{
			static u16 startsend = 0;
			if (PowerOffNum != 0)
			{
				debug_str(DEBUG_LOG_APP, "power mac\r\n");
				POWER_EVENT_UP_MAC *pMacUp = (POWER_EVENT_UP_MAC *)powerEventUp->Data;
				pMacUp->upType = POWER_EVENT_TYPE_OFF_MAC;
				u16 max_num = startsend + 200; // 最大发送200个mac
				if (PowerOffNum < max_num)
				{
					max_num = PowerOffNum;
				}
				pMacUp->num = max_num - startsend;
				len = 1 + 2;
				for (int i = startsend; i < max_num; i++)
				{
					memcpy(&pMacUp->data[i][0], &PowerOffMac[i][0], 7);
					len += 7;
					//					TransposeAddr(&pMacUp->data[i][0]); //地址转成大端
					//  				pMacUp->data[i][6] = 0; //停电
				}
				if (max_num >= PowerOffNum)
				{
					startsend = 0;
				}
				else
				{
					startsend = max_num;
				}
			}
		}

		if ((PowerUpSendType == SENDTYPE_SINGLE) && len)
		{
			signalPowerEvent = 1;
		}

		// 再次发送
		upPowerOffTmrID = HPLC_RegisterTmr(upPowerEvent, SEND_POWER_OFF, 1000 + (rand() & 0x3ff), TMR_OPT_CALL_ONCE);
	}

	if (len != 0)
	{
		// 生成应用层业务报文头
		CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_REPORT, 0, 1, 1, 1, APP_BUS_POWER_EVENT_REPORT, ++EventPacketID, 0, 0);

		// 生成停电事件上报报文头
#ifdef II_STA
		CreatePowerEventReportFrame(powerEventUp, POWER_EVENT_FUNC_TRIGGER_BY_COLLECT, addr, powerEventUp->Data, len);
#else
		CreatePowerEventReportFrame(powerEventUp, POWER_EVENT_FUNC_TRIGGER_BY_MODULE, addr, powerEventUp->Data, len);
#endif
		sendAppGeneralPacket->AppFrameLen = sizeof(APP_POWER_EVENT_REPORT) - 1 + len;
		pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;

		if (PowerUpSendType == SENDTYPE_SINGLE && signalPowerEvent == 0) // 单播需要等ACK
		{
			if (PowerEventPdu != NULL)
			{
				FreeSendPDU(PowerEventPdu);
				PowerEventPdu = NULL;
			}

			MacHandleApp(pdu->pdu, pdu->size, 3, PowerUpTEI, PowerUpSendType, BROADCAST_DRT_UNKNOW, 0);
			CurrentPowerState = POWER_EVENT_WAIT_ACK;
			StartTimer(&PowerEventTimer, MAX_WAIT_EVENT_ACK_TIME);
			PowerEventPdu = pdu;
			powerUpPacketID = EventPacketID;
		}
		else // 广播不等ACK
		{
			MacHandleApp(pdu->pdu, pdu->size, 3, PowerUpTEI, PowerUpSendType, BROADCAST_DRT_UP, 0);
			FreeSendPDU(pdu);
		}
	}
	else
	{
		FreeSendPDU(pdu);
	}
	#endif
}
static u8 update_flag = 0;
static u8 first_goto_event = 1;
u8 zero_check_pass[3] = {0};

void ClearPowerData(void)
{
	debug_str(DEBUG_LOG_APP, "clear_power_off_info!\r\n");

	if (INVAILD_TMR_ID != upPowerOffTmrID)
	{
		HPLC_DelTmr(upPowerOffTmrID);
		upPowerOffTmrID = INVAILD_TMR_ID;
	}

	if (INVAILD_TMR_ID != upPowerMacClearTmrID)
	{
		HPLC_DelTmr(upPowerMacClearTmrID);
		upPowerMacClearTmrID = INVAILD_TMR_ID;
	}

	if (INVAILD_TMR_ID != upPowerBitClearTmrID)
	{
		HPLC_DelTmr(upPowerBitClearTmrID);
		upPowerBitClearTmrID = INVAILD_TMR_ID;
	}

	memset(PowerOffEventBitmap, 0, sizeof(PowerOffEventBitmap));
	memset(PowerOffMac, 0, sizeof(PowerOffMac));
	PowerOffNum = 0;
	PowerOffTEI = 0;

	stopPowerOffTime = 0;
	update_flag = 0;
}

void PowerEvent(void *arg)
{
	#if 0
	static u8 realPowerDown = 0;
	static u8 reInsert = 0;
	static u8 startDecet = 0;
	static TimeValue detectTimer = {0, 0};
	static u16 getpower = 0;

	OS_ERR err;
	static u16 zero_time = 0;
	static u32 fliter = 0;
	if (first_goto_event)
	{
		first_goto_event = 0;
		AddZeroCrossCallback(getedZero);
		getpower = power;
	}
#if 0
	if (isGetedZero>50
		&&GetAppCurrentModel()==APP_TEST_NORMAL
		&&power)
	{
		BPLC_SetTxGain(HPLC_LinePower[HPLCCurrentFreq]);
	}
#endif
	if (zero_time == 50)
	{
		if (myDevicType == METER)
		{
			if (isGeteZeroPhase[0] > 20)
			{
				zero_check_pass[0] = 1;
				debug_str(DEBUG_LOG_NET, "zero check OK!\r\n");
			}
			else
			{
				debug_str(DEBUG_LOG_NET, "zero check Failed!!!\r\n");
			}
		}
		else if (myDevicType == METER_3PHASE)
		{
			if (isGeteZeroPhase[0] > 20 && isGeteZeroPhase[1] > 20 && isGeteZeroPhase[2] > 20)
			{
				debug_str(DEBUG_LOG_NET, "zero check OK!\r\n");
			}
			else
			{
				debug_str(DEBUG_LOG_NET, "zero check Failed!!!\r\n");
			}

			zero_check_pass[0] = (isGeteZeroPhase[0] > 20) ? 1 : 0;
			zero_check_pass[1] = (isGeteZeroPhase[1] > 20) ? 1 : 0;
			zero_check_pass[2] = (isGeteZeroPhase[2] > 20) ? 1 : 0;
		}
		++zero_time;
	}
	else if (zero_time < 800)
	{
		++zero_time;
	}
	static u32 lastPeriod;
	static u32 remainTime = 0xffffffff;
	u16 size;
	OSQPend(&PowerEventQ, 1, OS_OPT_PEND_NON_BLOCKING, &size, 0, &err);
	size = size;
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	if (!power) // 掉电
	{
#if 1
		if (realPowerDown) // 真正的掉电事件来临
		{
			BPLC_SetTxGain(HPLC_PowerOffPower[BPLC_TxFrequenceGet()]);
		}
#endif
		if (pNetBasePara->NetState == NET_IN_NET)
		{
			if (lastPeriod == GetStaPeriodPara()->BeaconPeriodCnt)
			{
				if (remainTime)
				{
					--remainTime;
				}
			}
			else
			{
				lastPeriod = GetStaPeriodPara()->BeaconPeriodCnt;
				remainTime = (SlotInfo.BeaconPeriod * 12 / 10) / 100; // 100代表PowerEvent轮询一次事件为100ms
			}
			if (!remainTime)
			{
				//				remainTime = 0xffffffff;
				remainTime = (SlotInfo.BeaconPeriod * 11 / 10) / 100;
				DummyBeacon();
			}
		}
	}
#if 0
	else
	{
#ifdef ALL_USE_TESTMODE_PARAM
		BPLC_SetTxGain(HPLC_TestChlPower[BPLC_TxFrequenceGet()]);
#else
		BPLC_SetTxGain(HPLC_ChlPower[BPLC_TxFrequenceGet()]);
#endif
	}
#endif
	if (err == OS_ERR_NONE)
	{
		fliter = 2;
	}
	else if (fliter)
	{
		--fliter;
		if (fliter == 0)
		{
			if (power != getpower)
			{
				memset(zeroCnt, 0, sizeof(zeroCnt));
				startDecet = 1;
				List_Del_Data(&ZeroCallbackList, (void *)getZeroCnt);
				AddZeroCrossCallback(getZeroCnt);
				StartTimer(&detectTimer, 1000);
				debug_str(DEBUG_LOG_APP, "12V event \r\n");
				getpower = power;
			}
		}
	}
	else
	{
		if (startDecet) // 当前处于探测是否掉电
		{
			if (TimeOut(&detectTimer))
			{
				startDecet = 0;
				debug_str(DEBUG_LOG_APP, "detect time out\r\n");
				if (moduleInsert()) // 当前模块处于插入状态
				{
					if (reInsert)
					{
						debug_str(DEBUG_LOG_APP, "reInsert\r\n");
						RebootSystem();
					}
					u32 zeroMaxCnt = getZeroMaxCnt();
					if (power) // 监测的是复电状态
					{
						if (zeroMaxCnt > 20 && isGetedZero >= 50) // 有过零 上报复电事件  以单拨方式
						{
							RebootSystem();
							if (CurrentPowerState == POWER_EVENT_IDLE)
							{
								if (upPowerOnTmrID == INVAILD_TMR_ID)
								{
									upPowerOnTmrID = HPLC_RegisterTmr(upPowerEvent, SEND_POWER_ON, (rand() & 0x3ff) + 10, TMR_OPT_CALL_ONCE);
								}
								else
								{
									HPLC_ReSetTmr(upPowerOnTmrID, (rand() & 0x3ff) + 10, TMR_RESET_IMMEDIATELY);
								}
								CurrentPowerState = POWER_EVENT_WAIT_TIME;
								StartTimer(&PowerEventTimer, MAX_EVENT_DECETC_TIME + 1000);
							}
						}
						else // 无过零 不上报事件  本次是重新上电
						{
							RebootSystem();
						}
					}
					else // 监测的是掉电事件
					{
						debug_str(DEBUG_LOG_APP, "Power OFF ZeroCnt:%d-%d-%d\r\n", zeroCnt[0], zeroCnt[1], zeroCnt[2]);

						if (zeroMaxCnt > 20) // 有过零不上报 掉电事件
						{
							debug_str(DEBUG_LOG_APP, "get zero no poweroff, zeroMaxCnt:%d\r\n", zeroMaxCnt);

#ifdef NW_TEST // 南网深化应用台体先关弱电，再关强电
							startDecet = 1;
							StartTimer(&detectTimer, 1000);
							memset(zeroCnt, 0, sizeof(zeroCnt));
							List_Del_Data(&ZeroCallbackList, (void *)getZeroCnt);
							AddZeroCrossCallback(getZeroCnt);

							debug_str(DEBUG_LOG_APP, "re startDecet\r\n");
#endif
						}
						else if (isGetedZero >= 50) // 上报掉电事件 以广播方式
						{
							debug_str(DEBUG_LOG_APP, "poweroff detect\r\n");
							realPowerDown = 1;
							if (!IsPowerOff)
							{
								EventClose();
								IsPowerOff = 1;
#ifndef II_STA
								StoragePowerOffFlag();
#else
								if (!GetProduceModeFlag()) // 非生产模式下
									StoragePowerOffFlag();
#endif
							}
#if !defined(II_STA) || defined(PROTOCOL_NW_2021_GUANGDONG) || defined(PROTOCOL_NW_2023_GUANGDONG_V2DOT1)
							PowerOffEventBitmap[pNetBasePara->TEI / 8] |= (1 << (pNetBasePara->TEI & 0x7));
							PowerOffTEI = PowerOffTEI > pNetBasePara->TEI ? PowerOffTEI : pNetBasePara->TEI;

							if (upPowerOffTmrID == INVAILD_TMR_ID)
							{
								upPowerOffTmrID = HPLC_RegisterTmr(upPowerEvent, SEND_POWER_OFF, (rand() & 0x3ff) + 1000, TMR_OPT_CALL_ONCE);
							}
							else
							{
								HPLC_ReSetTmr(upPowerOffTmrID, (rand() & 0x3ff) + 1000, TMR_RESET_IMMEDIATELY);
							}
							CurrentPowerState = POWER_EVENT_WAIT_TIME;
							StartTimer(&PowerEventTimer, MAX_EVENT_DECETC_TIME + 1000);
#endif
						}
						else
						{
							debug_str(DEBUG_LOG_APP, "No Zero with power on!\r\n");
						}
					}
				}
				else
				{
					debug_str(DEBUG_LOG_APP, "no insert\r\n");
					if (power) // 12v电源恢复
					{
						RebootSystem();
					}
					else // 12v电源失压
					{
						reInsert = 1;
					}
				}
			}
		}
		if (!power && !moduleInsert())
		{
			reInsert = 1;
		}
	}

	static u8 powerEventResendTimes = 0;
	static u8 powerEventRount = 0;
	switch (CurrentPowerState)
	{
	case POWER_EVENT_IDLE: // 空闲状态
	{
		powerEventResendTimes = 0;
		powerEventRount = 0;
		if (IsPowerOff && // 发生了掉电
			isGetedZero > 20)
		{
			if (power && (pNetBasePara->NetState == NET_IN_NET)
#if defined(II_STA) && !defined(PROTOCOL_NW_2021_GUANGDONG)
				&& (false == GetCMLPowerOnFlag())
#endif
			)
			{
				// 无论二采或是sta都在入网后上报复电事件
				if (upPowerOnTmrID == INVAILD_TMR_ID)
				{
					upPowerOnTmrID = HPLC_RegisterTmr(upPowerEvent, SEND_POWER_ON, (rand() & 0xfff) + 10, TMR_OPT_CALL_ONCE);
				}
				else
				{
					HPLC_ReSetTmr(upPowerOnTmrID, (rand() & 0xfff) + 10, TMR_RESET_IMMEDIATELY);
				}
				CurrentPowerState = POWER_EVENT_WAIT_TIME;
				StartTimer(&PowerEventTimer, MAX_POWER_EVENT_DECETC_TIME + 1000);
			}
		}
		else if (stopPowerOffTime)
		{
			stopPowerOffTime--;
			if (update_flag)
			{
				update_flag = 0;
			}
			if (stopPowerOffTime == 0)
			{
				memset(PowerOffEventBitmap, 0, sizeof(PowerOffEventBitmap));
				memset(PowerOffMac, 0, sizeof(PowerOffMac));
				PowerOffNum = 0;
				PowerOffTEI = 0;
			}
		}
		else if (update_flag) // 检测到其他节点事件
		{
			update_flag = 0;

			if (
#if 0
					(CCO_TEI == GetRelayTEI(CCO_TEI)) &&
#endif
				power) // 本站点有电,收集其他站点停电上报报文30秒后统一上报
			{
				if (upPowerOffTmrID == INVAILD_TMR_ID)
				{
					upPowerOffTmrID = HPLC_RegisterTmr(upPowerEvent, SEND_POWER_OFF, (rand() & 0x3ff) + MAX_EVENT_DECETC_TIME, TMR_OPT_CALL_ONCE);
				}
				else
				{
					HPLC_ReSetTmr(upPowerOffTmrID, (rand() & 0x3ff) + MAX_EVENT_DECETC_TIME, TMR_RESET_IMMEDIATELY);
				}
				CurrentPowerState = POWER_EVENT_WAIT_TIME;
				StartTimer(&PowerEventTimer, MAX_EVENT_DECETC_TIME + 1000);
			}
			else // 本站点(非一级站点)有电且停电位图或MAC有更新，或者本站点没电
			{
				if (upPowerOffTmrID == INVAILD_TMR_ID)
				{
					upPowerOffTmrID = HPLC_RegisterTmr(upPowerEvent, SEND_POWER_OFF, (rand() & 0x3ff) + 1000, TMR_OPT_CALL_ONCE);
				}
				else
				{
					HPLC_ReSetTmr(upPowerOffTmrID, (rand() & 0x3ff) + 1000, TMR_RESET_IMMEDIATELY);
				}
				CurrentPowerState = POWER_EVENT_WAIT_TIME;
				StartTimer(&PowerEventTimer, MAX_EVENT_DECETC_TIME + 1000);
			}
		}

		break;
	}
	case POWER_EVENT_WAIT_TIME:
		if (TimeOut(&PowerEventTimer))
		{
			CurrentPowerState = POWER_EVENT_IDLE;
		}
		break;
	case POWER_EVENT_WAIT_ACK: // 已经把事件帧发送给CCO，等待CCO回复
	{
		if (TimeOut(&PowerEventTimer))
		{
			// 等待CCO回复已经超时了,再发2次(一共发了3次),如果3次都没有回,等半小时再上报。
			if (powerEventResendTimes < EVENT_RESEND_TIMES)
			{
				StartTimer(&PowerEventTimer, MAX_WAIT_EVENT_ACK_TIME);
				MacHandleApp(PowerEventPdu->pdu, PowerEventPdu->size, 3, PowerUpTEI, PowerUpSendType, BROADCAST_DRT_UNKNOW, 0);
				powerEventResendTimes++;
			}
			else
			{
				powerEventRount++;
				if (signalPowerEvent == 0) // 复电事件
				{
					// 3次都没有回，等1分钟再发送
					StartTimer(&PowerEventTimer, 1 * 60 * 1000UL);
					CurrentPowerState = POWER_EVENT_NO_ACK;
				}
				else // 停电事件
				{
					// 3次都没有回，回到空闲
					if (PowerEventPdu != NULL)
					{
						FreeSendPDU(PowerEventPdu);
						PowerEventPdu = NULL;
					}
					CurrentPowerState = POWER_EVENT_IDLE;
				}
			}
		}
		break;
	}
	case POWER_EVENT_OVERFLOW: // CCO回复缓冲区满，等待重发
	{
		if (TimeOut(&PowerEventTimer))
		{
			// 等待超时时间到了，再次发送，进入等待回复状态
			StartTimer(&PowerEventTimer, MAX_WAIT_EVENT_ACK_TIME);
			MacHandleApp(PowerEventPdu->pdu, PowerEventPdu->size, 3, PowerUpTEI, PowerUpSendType, BROADCAST_DRT_UNKNOW, 0);
			CurrentPowerState = POWER_EVENT_WAIT_ACK;
			powerEventResendTimes = 0;
			powerEventRount = 0;
		}
		break;
	}
	case POWER_EVENT_NO_ACK: // 等待1分钟再次发送
	{
		if (TimeOut(&PowerEventTimer))
		{
			if (powerEventRount > MAX_EVENT_ROUNT)
			{
				powerEventRount = 0;
				powerEventResendTimes = 0;
				if (PowerEventPdu)
				{
					FreeSendPDU(PowerEventPdu);
					PowerEventPdu = NULL;
				}
				CurrentPowerState = POWER_EVENT_IDLE;
				if (signalPowerEvent == 0)
				{
					if (IsPowerOff)
					{
						IsPowerOff = 0;
						StoragePowerOffFlag();
					}
				}
			}
			else
			{
				StartTimer(&PowerEventTimer, MAX_WAIT_EVENT_ACK_TIME);
				MacHandleApp(PowerEventPdu->pdu, PowerEventPdu->size, 3, PowerUpTEI, PowerUpSendType, BROADCAST_DRT_UNKNOW, 0);
				powerEventResendTimes = 0;
				CurrentPowerState = POWER_EVENT_WAIT_ACK;
			}
		}
		break;
	}
	default:
		break;
	}
	#endif
}

#if defined(II_STA)
void II_STA_PowerOffEventReport(u8 *data, u8 poweroff_meter_num)
{
	u8 i;
	for (i = 0; i < poweroff_meter_num; i++)
		PowerMACHashInsert(&data[i * 7]);
	if (upPowerOffTmrID == INVAILD_TMR_ID)
	{
		upPowerOffTmrID = HPLC_RegisterTmr(upPowerEvent, SEND_POWER_OFF, (rand() & 0x3ff) + 1000, TMR_OPT_CALL_ONCE);
	}
	else
	{
		HPLC_ReSetTmr(upPowerOffTmrID, (rand() & 0x3ff) + 1000, TMR_RESET_IMMEDIATELY);
	}
	CurrentPowerState = POWER_EVENT_WAIT_TIME;
	StartTimer(&PowerEventTimer, MAX_EVENT_DECETC_TIME + 1000);
}
void II_STA_PowerOnEventReport(void)
{
	if (upPowerOnTmrID == INVAILD_TMR_ID)
	{
		upPowerOnTmrID = HPLC_RegisterTmr(upPowerEvent, SEND_POWER_ON, (rand() & 0xfff) + 10, TMR_OPT_CALL_ONCE);
	}
	else
	{
		HPLC_ReSetTmr(upPowerOnTmrID, (rand() & 0xfff) + 10, TMR_RESET_IMMEDIATELY);
	}
	CurrentPowerState = POWER_EVENT_WAIT_TIME;
	StartTimer(&PowerEventTimer, MAX_POWER_EVENT_DECETC_TIME + 1000);
}
#endif
#ifndef PROTOCOL_FPGA_VERSION
static TimeValue EventHighTimer = {0, 0};
static u8 EventResendTimes = 0;
static u8 EventRoundTimes = 0;
extern u8 MeterProtocol;
// 事件管脚高处理
void eventHighHandle(void)
{
#if !defined(II_STA) || defined(I_STA)
	if ((AppEventState != APP_EVENT_WAIT_LOCAL) && (AppEventState != APP_EVENT_CLEAR_HIGH))
	{
		const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
		if (pNetBasePara->NetState != NET_IN_NET)
		{
			AppEventState = APP_WAIT_IN_NET;
			return;
		}
		// 6分钟超时定时器，判断event管脚持续高的情况
		StartTimer(&EventHighTimer, 6 * 60 * 1000UL);

		// 清除之前状态
		if (EventPdu)
		{
			FreeSendPDU(EventPdu);
			EventPdu = NULL;
		}

		//        EventReportEnable = true;
		AppEventState = APP_EVENT_IDLE;
		EventResendTimes = 0;
		EventRoundTimes = 0;
		// 事件处理

		#ifndef NW_TEST
		OS_ERR err;
		for(u8 i=0;i<5;i++)
		{
			OSTimeDly(2, OS_OPT_TIME_DLY, &err);
			if(!EventRead())
			{
				debug_str(DEBUG_LOG_APS,"event@module no event\r\n");
				OSSemSet(&EventNotificationSem, 0, &err);
				return;
			}
		}
		#endif
		if (IsBindLocalMac() && pNetBasePara->NetState == NET_IN_NET && EventReportEnable)
		{
			// 发送一查询事件帧到本地
			u8 addr[6];
			u8 eventFrame[30] = {0};
			u16 dataLen = 0;
			memcpy(addr, pNetBasePara->Mac, 6);
			TransposeAddr(&addr[0]);

			//            if (MeterProtocol == LOCAL_PRO_698_45)
			//            {
			//                    AppEventState = APP_EVENT_WAIT_LOCAL;
			//                    dataLen = GetEventUPFrame(eventFrame, addr);
			//                    debug_str(DEBUG_LOG_APS,"event@query meter 645\r\n");
			//                    AppHandleLocal(0, APP_PACKET_EVENT_REPORT, 0, 0, true, eventFrame, dataLen, 0, 0);
			//            }
			//            else if(MeterProtocol == LOCAL_PRO_DLT645_2007)
			if (MeterProtocol != LOCAL_PRO_698_45)
			{
				dataLen = Get645_07EventFrame(eventFrame, addr);
				debug_str(DEBUG_LOG_APS, "event@query meter 07\r\n");
				AppLocalReceiveData(addr, addr, 20, 0, eventFrame, dataLen, AppHandleEvent, SENDTYPE_SINGLE, 0, BAUD_SEARCH);
				AppEventState = APP_EVENT_WAIT_LOCAL;
				StartTimer(&EventTimer, MAX_READ_EVENT_TIME);
			}
		}
	}
#endif
}
// 事件处理任务（中断上报）
void EventTask(void *arg)
{
	OS_ERR err;
	u8 wait_pin = 0;
	while (1)
	{
		const u32 WaitLocalMs = 1000 / (1000 / OSCfg_TickRate_Hz);
		OSSemPend(&EventNotificationSem, WaitLocalMs, OS_OPT_PEND_BLOCKING, 0, &err);
		if (err == OS_ERR_NONE) // 有事件中断
		{
#if 1
			eventHighHandle();
			wait_pin = 1;
#else
			if ((AppEventState != APP_EVENT_WAIT_LOCAL) && (AppEventState != APP_EVENT_CLEAR_HIGH))
			{
				// 清除之前状态
				if (EventPdu)
				{
					FreeSendPDU(EventPdu);
					EventPdu = NULL;
				}
				EventReportEnable = true;
				AppEventState = APP_EVENT_IDLE;
				EventResendTimes = 0;

				// 事件处理
				const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
				if (IsBindLocalMac() && pNetBasePara->NetState == NET_IN_NET && EventReportEnable)
				{
					// 发送一查询事件帧到本地
					u8 addr[6];
					u8 eventFrame[sizeof(ReadMeterEvent07)] = {0};
					u16 dataLen = 0;

					memcpy(addr, pNetBasePara->Mac, 6);
					TransposeAddr(&addr[0]);
					dataLen = Get645_07EventFrame(eventFrame, addr);
					AppLocalReceiveData(addr, addr, 20, 0, eventFrame, dataLen, AppHandleEvent, SENDTYPE_SINGLE, 0);
					AppEventState = APP_EVENT_WAIT_LOCAL;
					StartTimer(&EventTimer, MAX_READ_EVENT_TIME);
				}
			}
#endif
		}
		else // 没有事件中断,处理之前事件
		{
			switch (AppEventState)
			{
			case APP_EVENT_IDLE:
			{
				wait_pin = 1;
				if (EventRead() && power)
				{
					eventHighHandle();
				}
#ifdef PROTOCOL_NW_2021
				else
				{
					const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
					if (pNetBasePara->NetState == NET_IN_NET)
					{
						P_Dl645_Freme pEvent = (P_Dl645_Freme)GetMeterUpEvent();
						if (pEvent)
						{
							AppEventState = APP_EVENT_WAIT_LOCAL;
							debug_str(DEBUG_LOG_APS, "event@meter up 645\r\n");
							StartTimer(&EventTimer, MAX_READ_EVENT_TIME);
							if (EventPdu)
							{
								FreeSendPDU(EventPdu);
								EventPdu = NULL;
							}
							AppHandleEvent(NULL, NULL, 0, true, (u8 *)pEvent, pEvent->dataLen + 12, SENDTYPE_SINGLE, 0);
							pEvent->protocolHead = 0; // buffer使用完毕 设置第一个字节为0 表示该buffer内没有内容
							wait_pin = 0;
						}
					}
				}
#endif
			}
			case APP_EVENT_WAIT_LOCAL: // 已经发送一帧到本地查询事件状态字
			{
				if (TimeOut(&EventTimer))
				{
					AppEventState = APP_EVENT_IDLE;
				}
				break;
			}
			case APP_EVENT_WAIT_ACK: // 已经把事件帧发送给CCO，等待CCO回复
			{
				if (TimeOut(&EventTimer))
				{
					// 等待CCO回复已经超时了，再发2次(一共发了3次)，如果3次都没有回，等半小时再上报。
					if (EventResendTimes < EVENT_RESEND_TIMES)
					{
						StartTimer(&EventTimer, MAX_WAIT_EVENT_ACK_TIME);
						MacHandleApp(EventPdu->pdu, EventPdu->size, 3, CCO_TEI, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);
						EventResendTimes++;
					}
					else
					{
						// 3次都没有回，只能等半小时再回到空闲
						EventRoundTimes++;
						StartTimer(&EventTimer, 6 * 60 * 1000UL);
						//							if(EventPdu)
						//							{
						//		                    	FreeSendPDU(EventPdu);
						//								EventPdu = NULL;
						//							}
						AppEventState = APP_EVENT_NO_ACK;
					}
				}
				break;
			}
			case APP_EVENT_OVERFLOW: // CCO回复缓冲区满，等待重发
			{
				if (TimeOut(&EventTimer))
				{
					// 等待超时时间到了，再次发送，进入等待回复状态
					StartTimer(&EventTimer, MAX_WAIT_EVENT_ACK_TIME);
					MacHandleApp(EventPdu->pdu, EventPdu->size, 3, CCO_TEI, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);
					AppEventState = APP_EVENT_WAIT_ACK;
					EventResendTimes = 0;
					EventRoundTimes = 0;
				}
				break;
			}
			case APP_EVENT_NO_ACK: // 等待半小时再回到空闲态
			{
				if (TimeOut(&EventTimer))
				{
					if (EventRoundTimes > MAX_EVENT_ROUNT)
					{
						if (EventPdu)
						{
							FreeSendPDU(EventPdu);
							EventPdu = NULL;
						}
						StartTimer(&EventHighTimer, 10 * 60 * 1000UL);
						AppEventState = APP_EVENT_CLEAR_HIGH;
						EventResendTimes = 0;
						EventRoundTimes = 0;
					}
					else
					{
						StartTimer(&EventTimer, MAX_WAIT_EVENT_ACK_TIME);
						MacHandleApp(EventPdu->pdu, EventPdu->size, 3, CCO_TEI, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);
						EventResendTimes = 0;
						AppEventState = APP_EVENT_WAIT_ACK;
					}
				}
				break;
			}
			case APP_EVENT_CLEAR_HIGH:
			{
				if (!wait_pin)
				{
					AppEventState = APP_EVENT_IDLE;
					break;
				}
				if (!EventRead()) // 等待事件脚回到低电平才回到空闲状态
				{
					AppEventState = APP_EVENT_IDLE;
				}
				// 6分钟超时后，event管脚还是高，重新读取电表状态字并发送给CCO
				if (TimeOut(&EventHighTimer))
				{
					AppEventState = APP_EVENT_IDLE;
					wait_pin = 1;
					eventHighHandle();
				}
				break;
			}
			case APP_WAIT_IN_NET:
			{
				eventHighHandle();
			}
			default:
				break;
			}
		}

		static u8 count = 0;

		count++;
		if (count == 15) // 15秒打印一次发射功率
		{
			count = 0;

			const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
			debug_str(DEBUG_LOG_NET, "TxPower:%d, Sta State:%d, Tei:%d, isBindAddr:%d\r\n", BPLC_GetTxGain(),
					  pNetBasePara->NetState, pNetBasePara->TEI, IsBindLocalMac());
		}
	}
}
#endif

u16 ReadMeterFilter(u8 Protocol, u8 *data, u16 len)
{
	int index = 0xffff;
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	u8 deviceAddr[6];
	memcpy(deviceAddr, pNetBasePara->Mac, 6);
	TransposeAddr(deviceAddr);
	if (Protocol == APP_PRO_DLT645_1997 || Protocol == APP_PRO_DLT645_2007)
	{
		for (index = 0; index < len; index++)
		{
			if (data[index] == 0x68)
			{
				if (Is645Frame(&data[index], len - index))
				{
					break;
				}
			}
		}
		if (index >= len)
		{
			return 0xffff;
		}
		if (memcmp(&data[index + 1], deviceAddr, 6) == 0 || memIsHex(&data[index + 1], 0xff, LONG_ADDR_LEN) || memIsHex(&data[index + 1], 0xaa, LONG_ADDR_LEN) || memIsHex(&data[index + 1], 0x99, LONG_ADDR_LEN))
		{
			return index;
		}
		else
		{
			return 0xffff;
		}
	}
	else if (Protocol == APP_PRO_698_45)
	{
		u8 addr_index = 0;
		for (index = 0; index < len; index++)
		{
			if (data[index] == 0x68)
			{
				if (Is698Frame(&data[index], len - index))
				{
					break;
				}
			}
		}
		if (index >= len)
		{
			return 0xffff;
		}

		u8 sa_flag = data[index + 4];
		if ((sa_flag & (1 << 5)) && ((sa_flag & 0xF) == 6)) // 有1字节的扩展逻辑地址，并且地址长度为6
		{
			addr_index = 6;
		}
		else
		{
			addr_index = 5;
		}

		if (memcmp(&data[index + addr_index], deviceAddr, 6) == 0 || memIsHex(&data[index + addr_index], 0xff, LONG_ADDR_LEN) || memIsHex(&data[index + addr_index], 0xaa, LONG_ADDR_LEN) || memIsHex(&data[index + addr_index], 0x99, LONG_ADDR_LEN))
		{
			return index;
		}
		else
		{
			return 0xffff;
		}
	}
	return index;
}

#ifndef II_STA
#ifdef NEW_TOPO
extern u32 alarm_topo_time;
extern u32 remain_tick;
#endif
#endif

#ifdef NW_TEST
volatile u32 last_seq;
#endif
// 数据透传处理
void AppReadMeterHandle(u8 *data, u16 len, u16 port)
{
	#if 0
	APP_CREATE_MAC_DEF();
	APP_READ_METER_DOWN *readMeter = (APP_READ_METER_DOWN *)&appGeneral->Data[0];
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	u8 mac[LONG_ADDR_LEN];

	memcpy(mac, readMeter->DstMac, LONG_ADDR_LEN);
	TransposeAddr(mac);
	GetReadMeter();

	if (IsMeterInTable(readMeter->DstMac) ||
		(memIsHex(mac, 0x99, LONG_ADDR_LEN)) || (memIsHex(mac, 0xAA, LONG_ADDR_LEN)) || (memIsHex(mac, 0xFF, LONG_ADDR_LEN)))
	{
		if (port == 0x11)
		{
			u16 head_idx = ReadMeterFilter(APP_PRO_DLT645_2007, readMeter->Data, readMeter->DataLen);
			if (head_idx != 0xffff) // find 645
			{
#ifndef II_STA
#ifndef NEW_TOPO
				if ((*(u32 *)&readMeter->Data[head_idx + 8] == 0x33370804) || (*(u32 *)&readMeter->Data[head_idx + 8] == 0x33370604))
				{
					for (int i = head_idx + 10; i < readMeter->DataLen; i++)
					{
						readMeter->Data[i] -= 0x33;
					}
					extern void StartTopo(u8 func, u8 cnt, u8 interval, u8 * data, u8 datalen);
					StartTopo(readMeter->Data[head_idx + 12], readMeter->Data[head_idx + 13], readMeter->Data[head_idx + 14], &readMeter->Data[head_idx + 16], readMeter->Data[head_idx + 15]);
					readMeter->Data[head_idx + 8] = 0x84;
					readMeter->Data[head_idx + 9] = 0x00;
					readMeter->Data[head_idx + 10] = GetCheckSum(&readMeter->Data[head_idx], 10);
					readMeter->Data[head_idx + 11] = 0x16;
					AppHandleReadMeter(readMeter->DstMac, readMeter->DstMac, appGeneral->AppSeq, true, &readMeter->Data[head_idx], 12, macHead->SendType, macHead->SrcTei);
					return;
				}
#else
				P_Dl645_Freme pFrame = (P_Dl645_Freme)&readMeter->Data[head_idx];
				if (*(u32 *)pFrame->dataBuf == 0x3c3b343c)
				{

					for (int i = head_idx + 10; i < readMeter->DataLen; i++)
					{
						readMeter->Data[i] -= 0x33;
					}
					u8 device = pFrame->dataBuf[4];
					struct tpc
					{
						u8 sec;
						u8 min;
						u8 hour;
						u8 day;
						u8 mon;
						u8 year;
						u16 bitMs;
						u32 fre : 24;
						u32 len : 8;
						u8 data[0];
					} *topo_config;
					topo_config = (struct tpc *)&pFrame->dataBuf[5];
					u16 pwm_h = *((u16 *)(&(topo_config->data[topo_config->len])));		// u16 pwm_h = topo_config->data[topo_config->len];//
					u16 pwm_l = *((u16 *)(&(topo_config->data[topo_config->len + 2]))); // u16 pwm_l = topo_config->data[topo_config->len+2];//
					extern void StartTopo(u32 sec1900, u16 pwm_l, u16 pwm_h, u16 ms, u8 * data, u8 datalen);
					u8 err = 0x01;
					u32 sec1900 = 0;
					device = device;	  // 下面如果不用device解决警告
					if (remain_tick == 0) // 需要等待
					{
						if (topo_config->len < 17)
						{
							if (topo_config->bitMs > 1200)
							{
								topo_config->bitMs = 600;
							}

							if (!memIsHex((u8 *)topo_config, 0xff, 6)) // 定时发送
							{
								if (Time_Module[0].isVaild == 4) // 指定时间发送需要同步到集中器时间
								{
									Time_Special temp_time;
									memset(&temp_time, 0, sizeof(temp_time));
									temp_time.tm_year = (topo_config->year) + 2000 - 1900;
									temp_time.tm_mon = (topo_config->mon) - 1;
									temp_time.tm_mday = (topo_config->day);
									temp_time.tm_hour = (topo_config->hour);
									temp_time.tm_min = (topo_config->min);
									temp_time.tm_sec = (topo_config->sec);
									sec1900 = mktime(&temp_time);

									if (sec1900 < Time_Module[0].sec_form_1900)
									{
										sec1900 = 0;
									}
								}
							}
							else // 立即发送
							{
								sec1900 = 0xffffffff;
							}

							if (sec1900 != 0)
							{
								StartTopo(sec1900, pwm_l, pwm_h, topo_config->bitMs, (u8 *)topo_config->data, topo_config->len);
								pFrame->controlCode = 0x94;
								pFrame->dataLen = 0;
								pFrame->dataBuf[0] = GetCheckSum(&readMeter->Data[head_idx], 10);
								pFrame->dataBuf[1] = 0x16;

								AppHandleReadMeter(readMeter->DstMac, readMeter->DstMac, appGeneral->AppSeq, true, &readMeter->Data[head_idx], pFrame->dataLen + 12, macHead->SendType, macHead->SrcTei);
								return;
							}
						}
					}
					else
					{
						err = remain_tick / 1000;
					}

					pFrame->controlCode = 0xD4;
					pFrame->dataLen = 1;
					pFrame->dataBuf[0] = err + 0x33;
					pFrame->dataBuf[1] = GetCheckSum(&readMeter->Data[head_idx], 11);
					pFrame->dataBuf[2] = 0x16;
					AppHandleReadMeter(readMeter->DstMac, readMeter->DstMac, appGeneral->AppSeq, true, &readMeter->Data[head_idx], pFrame->dataLen + 12, macHead->SendType, macHead->SrcTei);

					return;
				}
				if (pFrame->controlCode == 0x08 && memIsHex(pFrame->meter_number, 0x99, 6)) // 广播校时
				{
					if (Time_Module[0].isVaild < 4)
					{
						Time_Special temp;
						memset(&temp, 0, sizeof(temp));
						temp.tm_year = 2000 + bcd_to_dec(pFrame->dataBuf[5] - 0x33) - 1900;
						temp.tm_mon = bcd_to_dec(pFrame->dataBuf[4] - 0x33) - 1;
						temp.tm_mday = bcd_to_dec(pFrame->dataBuf[3] - 0x33);
						temp.tm_hour = bcd_to_dec(pFrame->dataBuf[2] - 0x33);
						temp.tm_min = bcd_to_dec(pFrame->dataBuf[1] - 0x33);
						temp.tm_sec = bcd_to_dec(pFrame->dataBuf[0] - 0x33);
						Time_Module[0].isVaild = 4;
						Time_Module[0].time = temp;
						Time_Module[0].sec_form_1900 = mktime(&temp);
						OS_ERR err;
						Time_Module[0].sysTick = OSTimeGet(&err);
					}
				}
#endif
#endif
			}

			if (readMeter->Timeout < 18)
			{
				readMeter->Timeout = 18;
			}
#ifdef NW_TEST
			read_meter_time++;
			last_seq = appGeneral->AppSeq;
#endif
#ifdef I_STA
			u16 i_data_len = readMeter->DataLen + 2;
			char *pData = LMP_make_frame_gd_cmd(0x22, 0xEA042201, I_STA_SEQ++, ((u8 *)&readMeter->Data) - 2, &i_data_len, readMeter->DstMac, false);
			if (pData)
			{
				AppLocalReceiveData(readMeter->SrcMac, readMeter->DstMac, readMeter->Timeout, appGeneral->AppSeq, (u8 *)pData, i_data_len, AppHandleReadMeter, macHead->SendType, macHead->SrcTei, BAUD_SEARCH);
				free(pData);
			}
#else
			debug_str(DEBUG_LOG_NET, "AppReadMeterHandle, seq:%d, len:%d\r\n", appGeneral->AppSeq, readMeter->DataLen);
			AppLocalReceiveData(readMeter->SrcMac, readMeter->DstMac, readMeter->Timeout, appGeneral->AppSeq, readMeter->Data, readMeter->DataLen, AppHandleReadMeter, macHead->SendType, macHead->SrcTei, BAUD_SEARCH);
#endif
		}
		else if (port == 0x13)
		{
#if defined(PROTOCOL_NW_2020) && !defined(II_STA) // I II采不响应 深化应用的精准校时
			static u32 broad_seq = 0xffffffff;

			APP_READ_MOUDLE_DOWN *readMoudle = (APP_READ_MOUDLE_DOWN *)&appGeneral->Data[0];
			bool is_meter_timing = false; // 是否是电能表时钟精准校时

#ifdef PROTOCOL_NW_2020_OLD
			if (readMoudle->Timeout == APP_SHENHUA_APP_CODE_TIMING) // 电能表时钟精准校时
#else
			if (readMoudle->App_Code == APP_SHENHUA_APP_CODE_TIMING) // 电能表时钟精准校时
#endif
			{
				if (broad_seq == appGeneral->AppSeq)
				{
					return;
				}
				broad_seq = appGeneral->AppSeq;
				APP_READ_MOUDLE_TIMING_DOWN *readMoudleTiming = (APP_READ_MOUDLE_TIMING_DOWN *)&appGeneral->Data[0];

				// 6字节源地址+6字节目的地址+1字节保留+1字节业务代码+2字节转发数据长度
				is_meter_timing = APP_BroadcastTimeExt(readMoudleTiming->Data, readMoudleTiming->DataLen);

				debug_str(DEBUG_LOG_NET, "AppReadMeterHandle APP_SHENHUA_APP_CODE_TIMING, is meter timing:%d\r\n", is_meter_timing);
			}

			if (is_meter_timing == false)
			{
				if (readMoudle->App_Code == APP_SHENHUA_APP_CODE_TIMING) // 精准校时
				{
					if (broad_seq == appGeneral->AppSeq)
					{
						return;
					}
					broad_seq = appGeneral->AppSeq;
					APP_BroadTimeHandle(readMoudle->Data, readMoudle->DataLen);
				}
				else if (readMoudle->App_Code == APP_SHENHUA_APP_CODE_GRAPH) // 负荷曲线采集与存储
				{
					AppGraphHandle(readMoudle->SrcMac, readMoudle->DstMac, macHead->SrcTei, appGeneral->AppSeq, macHead->SendType, readMoudle->Data, readMoudle->DataLen);
				}
			}
#endif
		}
	}
	#endif
}
extern struct System_Params_Type SystemParams;
//搜表结束后主动上报搜表结果给CCO
void reportSerachMeterResult()
{
	static u16 reportSeq = 1;
	reportSeq++;
	#ifdef II_STA
	u8 MeterNum = SystemParams.SystemMeterList.meterlen; // 取电表总数
#endif
	SEND_PDU_BLOCK *pdu = MallocSendPDU();
	if (pdu == NULL)
		return;
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	// APP包头
	APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
	// 业务包头
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];
	// 搜表上行帧
	APP_SCAN_METER_UP *sendScanMeterUp = (APP_SCAN_METER_UP *)&sendAppGeneralPacket->Data[0];

	// 取下挂的电表信息块(6字节MAC地址+2字节保留数据)
#ifdef II_STA
	APP_METER_INFO meterInfo[32] = {0}; // 电表信息块
    u8 dataindex = 0;
    for(int i=0;i<32;i++)
    {
    	if(SystemParams.SystemMeterList.memter[i].Type != 0)
        {
            memcpy(&meterInfo[dataindex],SystemParams.SystemMeterList.memter[i].Id,6);
			dataindex++;
        }
    }
#else
	APP_METER_INFO meterInfo; // 电表信息块
	memcpy(meterInfo.MeterMac, pNetBasePara->Mac, 6);
	meterInfo.Reserve = 0;
	TransposeAddr(meterInfo.MeterMac);
#endif

	// 生成APP包头帧
	CreateAppPacket(sendAppPacket);
	// 生成业务包头帧
#ifdef II_STA
	CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_CMD, 0, 0, 0, 1, APP_BUS_QUIRY_LIST, reportSeq, sizeof(APP_SCAN_METER_UP) - 1 + (MeterNum * sizeof(APP_METER_INFO)), 0);
#else
	CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_CMD, 0, 0, 0, 1, APP_BUS_QUIRY_LIST, reportSeq, sizeof(APP_SCAN_METER_UP) - 1 + sizeof(APP_METER_INFO), 0);
#endif

	// 生成搜表上行帧
#ifdef II_STA
	CreateScanUpFrame(sendScanMeterUp, MeterNum, &meterInfo);
#else
	CreateScanUpFrame(sendScanMeterUp, 1, &meterInfo);
#endif
	// 发送
	pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
	MacHandleApp(pdu->pdu, pdu->size, 2, CCO_TEI, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);
	FreeSendPDU(pdu);
}



// 查询搜表结果处理
void AppCheckScanResultHandle(u8 *data, u16 len)
{
#ifdef II_STA
	u8 MeterNum = SystemParams.SystemMeterList.meterlen; // 取电表总数
#endif
	APP_CREATE_MAC_DEF();
	SEND_PDU_BLOCK *pdu = MallocSendPDU();
	if (pdu == NULL)
		return;
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	// APP包头
	APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
	// 业务包头
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];
	// 搜表上行帧
	APP_SCAN_METER_UP *sendScanMeterUp = (APP_SCAN_METER_UP *)&sendAppGeneralPacket->Data[0];

	// 取下挂的电表信息块(6字节MAC地址+2字节保留数据)
#ifdef II_STA
	APP_METER_INFO meterInfo[32] = {0}; // 电表信息块
    u8 dataindex = 0;
    for(int i=0;i<32;i++)
    {
    	if(SystemParams.SystemMeterList.memter[i].Type != 0)
        {
            memcpy(&meterInfo[dataindex],SystemParams.SystemMeterList.memter[i].Id,6);
			dataindex++;
        }
    }
#else
	APP_METER_INFO meterInfo; // 电表信息块
	memcpy(meterInfo.MeterMac, pNetBasePara->Mac, 6);
	meterInfo.Reserve = 0;
	TransposeAddr(meterInfo.MeterMac);
#endif

	// 生成APP包头帧
	CreateAppPacket(sendAppPacket);
	// 生成业务包头帧
#ifdef II_STA
	CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_CMD, 0, 0, 0, 1, APP_BUS_QUIRY_LIST, appGeneral->AppSeq, sizeof(APP_SCAN_METER_UP) - 1 + (MeterNum * sizeof(APP_METER_INFO)), 0);
#else
	CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_CMD, 0, 0, 0, 1, APP_BUS_QUIRY_LIST, appGeneral->AppSeq, sizeof(APP_SCAN_METER_UP) - 1 + sizeof(APP_METER_INFO), 0);
#endif

	// 生成搜表上行帧
#ifdef II_STA
	CreateScanUpFrame(sendScanMeterUp, MeterNum, &meterInfo);
#else
	CreateScanUpFrame(sendScanMeterUp, 1, &meterInfo);
#endif
	// 发送
	pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
	MacHandleApp(pdu->pdu, pdu->size, 2, CCO_TEI, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);
	FreeSendPDU(pdu);
}

// 下发搜表列表处理
void AppScanListHandle(u8 *data, u16 len)
{
	APP_CREATE_MAC_DEF();
	SEND_PDU_BLOCK *pdu = MallocSendPDU();
	if (pdu == NULL)
		return;
	// APP包头
	APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
	// 业务包头
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];

	// 生成APP包头帧
	CreateAppPacket(sendAppPacket);
	// 生成业务包头帧
	CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_CONFIRM, 0, 0, 0, 1, APP_BUS_CONFIRM, appGeneral->AppSeq, 0, 0);

	// 发送
	pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1;
	MacHandleApp(pdu->pdu, pdu->size, 2, CCO_TEI, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);
	FreeSendPDU(pdu);
}

// 允许/禁止从节点上报处理
void AppEnableReportHandle(u8 *data, u16 len)
{
	APP_CREATE_MAC_DEF();
#ifndef II_STA
	APP_EVENT_ENABLE *eventPacket = (APP_EVENT_ENABLE *)&appGeneral->Data[0];
#endif

	SEND_PDU_BLOCK *pdu = MallocSendPDU();
	if (pdu == NULL)
		return;
	// APP包头
	APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
	// 业务包头
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];

#ifndef II_STA
	EventReportEnable = eventPacket->EventFlg;
#endif

	// 生成APP包头帧
	CreateAppPacket(sendAppPacket);
	// 生成业务包头帧
	CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_CONFIRM, 0, 0, 0, 1, APP_BUS_CONFIRM, appGeneral->AppSeq, 0, 0);

	// 发送
	pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1;
	MacHandleApp(pdu->pdu, pdu->size, 2, CCO_TEI, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);
	FreeSendPDU(pdu);
}

static void AppRebootTimerCallback(void *arg)
{
	REBOOT_SYSTEM();
}

// 节点重启
void AppRebootHandle(u8 *data, u16 len)
{
	APP_CREATE_MAC_DEF();
	APP_NODE_REBOOT *rebootFrame = (APP_NODE_REBOOT *)&appGeneral->Data[0];

	SEND_PDU_BLOCK *pdu = MallocSendPDU();
	if (pdu == NULL)
		return;
	// APP包头
	APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
	// 业务包头
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];

	// 定时重启
	HPLC_RegisterTmr(AppRebootTimerCallback, 0, rebootFrame->DelayTime * 1000ul, TMR_OPT_CALL_ONCE);

	// 生成APP包头帧
	CreateAppPacket(sendAppPacket);
	// 生成业务包头帧
	CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_CONFIRM, 0, 0, 0, 1, APP_BUS_CONFIRM, appGeneral->AppSeq, 0, 0);

	// 发送
	pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1;
	MacHandleApp(pdu->pdu, pdu->size, 2, CCO_TEI, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);
	FreeSendPDU(pdu);
}

BroadcasetTimeExtResult TimeExtResult[MAX_BROADCAST_TIME_RESULT];
int TimeExtIdx = 0;
void APP_BroadcastExtHandle(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8 *data, u16 len, u8 sendType, u16 srcTei)
{
	TimeExtResult[TimeExtIdx].seq = seq;
	TimeExtResult[TimeExtIdx++].result = 1;
	if (TimeExtIdx >= MAX_BROADCAST_TIME_RESULT)
	{
		TimeExtIdx = 0;
	}
}
Time_Special cco_broad_time;
Time_Special sta_broad_time;
extern u8 shenhua_timing;
bool APP_BroadcastTimeExt(u8 *data, u16 len)
{
	APP_BROADCST_TIME_EXT *pTime = (APP_BROADCST_TIME_EXT *)data;
	u32 currentNtb = GetCurrentNTB() - pTime->NTB;
	currentNtb /= NTB_FRE_PER_MS * 1000;
	u16 head_idx = ReadMeterFilter(APP_PRO_DLT645_2007, pTime->data, len - sizeof(APP_BROADCST_TIME_EXT));
#if 0
	if (head_idx != 0xffff) //find 645
	{
		Time_Special time;
		memset(&time, 0, sizeof(time));
		time.tm_year = 2000 + bcd_to_dec(pTime->data[15+head_idx]-0x33) - 1900;
		time.tm_mon = bcd_to_dec(pTime->data[14+head_idx]-0x33) - 1;
		time.tm_mday = bcd_to_dec(pTime->data[13+head_idx]-0x33);
		time.tm_hour = bcd_to_dec(pTime->data[12+head_idx]-0x33);
		time.tm_min = bcd_to_dec(pTime->data[11+head_idx]-0x33);
		time.tm_sec = bcd_to_dec(pTime->data[10+head_idx]-0x33);
		time.tm_isdst = 0;
		DateSpecialChangeSec(&time,currentNtb);

		pTime->data[10+head_idx]=dec_to_bcd(time.tm_sec)+0x33;
		pTime->data[11+head_idx]=dec_to_bcd(time.tm_min)+0x33;
		pTime->data[12+head_idx]=dec_to_bcd(time.tm_hour)+0x33;
		pTime->data[13+head_idx]=dec_to_bcd(time.tm_mday)+0x33;
		pTime->data[14+head_idx]=dec_to_bcd(time.tm_mon+1)+0x33;
		pTime->data[15+head_idx]=dec_to_bcd(time.tm_year+1900-2000)+0x33;

#if 0
		const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();
		memcpy(&pTime->data[1+head_idx], pNetBasePara->Mac, LONG_ADDR_LEN);
		TransposeAddr(&pTime->data[1+head_idx]);
#else
		memset(&pTime->data[1+head_idx], 0x99, LONG_ADDR_LEN);
#endif
		pTime->data[head_idx+16]=GetCheckSum(&pTime->data[head_idx],16);

		AppLocalReceiveData(&pTime->data[1+head_idx], &pTime->data[1+head_idx], 10, pTime->Seq, &pTime->data[head_idx], 18, APP_BroadcastExtHandle, 0, 0, BAUD_SEARCH);

		return true;
	}
#else
	if (head_idx != 0xffff) // find 645
	{
		// 正在校时
		if (shenhua_timing)
			return false;
		// 修改模块当前时钟
		OS_ERR err;
		Time_Module_s temp;
		memset(&temp, 0, sizeof(temp));
		temp.time.tm_year = 2000 + bcd_to_dec(pTime->data[15 + head_idx] - 0x33) - 1900;
		temp.time.tm_mon = bcd_to_dec(pTime->data[14 + head_idx] - 0x33) - 1;
		temp.time.tm_mday = bcd_to_dec(pTime->data[13 + head_idx] - 0x33);
		temp.time.tm_hour = bcd_to_dec(pTime->data[12 + head_idx] - 0x33);
		temp.time.tm_min = bcd_to_dec(pTime->data[11 + head_idx] - 0x33);
		temp.time.tm_sec = bcd_to_dec(pTime->data[10 + head_idx] - 0x33);

		debug_str(DEBUG_LOG_NET, "APP_BroadcastTimeExt, time value %d-%d-%d %d:%d\r\n", temp.time.tm_year, temp.time.tm_mon,
				  temp.time.tm_mday, temp.time.tm_hour, temp.time.tm_min);
		temp.time.tm_isdst = 0;
		cco_broad_time = temp.time;
		DateSpecialChangeSec(&temp.time, currentNtb);

		temp.isVaild = 4;
		temp.sysTick = OSTimeGet(&err);
		temp.sec_form_1900 = Data2Sec(&temp.time);

		Time_Module[0] = temp;
		Time_Module[0].sysTick = OSTimeGet(&err);
		Time_Module[0].sec_form_1900 = Data2Sec(&Time_Module[0].time);
		shenhua_timing = 1;

		PLC_StartCalibMeter();

		return true;
	}
#endif
	return false;
}

// 深化应用精准校时

void APP_BroadTimeHandle(u8 *data, u16 len)
{
	APP_BROAD_TIME *p_time = (APP_BROAD_TIME *)data;

	// 正在校时
	if (shenhua_timing)
		return;
#if 1
	// 修改模块当前时钟
	OS_ERR err;
	Time_Module_s temp;
	memset(&temp, 0, sizeof(temp));
	temp.time.tm_year = 2000 + bcd_to_dec(p_time->time.year) - 1900;
	temp.time.tm_mon = bcd_to_dec(p_time->time.mon) - 1;
	temp.time.tm_mday = bcd_to_dec(p_time->time.day);
	temp.time.tm_hour = bcd_to_dec(p_time->time.hour);
	temp.time.tm_min = bcd_to_dec(p_time->time.min);
	temp.time.tm_sec = bcd_to_dec(p_time->time.sec);
	temp.time.tm_isdst = 0;
	temp.isVaild = 4;
	temp.sysTick = OSTimeGet(&err);
	temp.sec_form_1900 = Data2Sec(&temp.time);

	debug_str(DEBUG_LOG_NET, "APP_BroadTimeHandle, time value %d-%d-%d %d:%d\r\n", temp.time.tm_year, temp.time.tm_mon,
			  temp.time.tm_mday, temp.time.tm_hour, temp.time.tm_min);

#if 1
	Time_Module[0] = temp;
	Time_Module[0].sysTick = OSTimeGet(&err);
	Time_Module[0].sec_form_1900 = Data2Sec(&Time_Module[0].time);
	shenhua_timing = 1;
#else
	// if (Time_Module[0].isVaild == 2) //如果当前是表时间 进行计算偏差
	{
		Time_Module[1].isVaild = 1;
		Time_Module[1].diff_tick = Diff_Specialtime(&temp.time, &Time_Module[0].time);
	}
	Time_Module[0] = temp;
	Time_Module[0].sysTick = OSTimeGet(&err);
	Time_Module[0].sec_form_1900 = Data2Sec(&Time_Module[0].time);

	StorageTime();
#endif
	PLC_StartCalibMeter();
#if 0
	//电表校时
	u8 frame[32] = {0}, frame_len = 0;
	u8 addr[LONG_ADDR_LEN] = {0};
	
	if (MeterProtocol==LOCAL_PRO_DLT645_2007||MeterProtocol==LOCAL_PRO_DLT645_1997)
	{
		frame_len = Get645BroadTime(frame, &time);
	}
	else if (MeterProtocol==LOCAL_PRO_698_45)
	{
		frame_len = Get698BroadTime(frame, &time);
	}
	AppLocalReceiveData(addr, addr, 8, 0, frame, frame_len, NULL, 0, 0, BAUD_SEARCH);
#endif
#else
	// 修改模块当前时钟
	Time_Special time;
	memset(&time, 0, sizeof(time));
	time.tm_year = 2000 + bcd_to_dec(p_time->time.year) - 1900;
	time.tm_mon = bcd_to_dec(p_time->time.mon - 1);
	time.tm_mday = bcd_to_dec(p_time->time.day);
	time.tm_hour = bcd_to_dec(p_time->time.hour);
	time.tm_min = bcd_to_dec(p_time->time.min);
	time.tm_sec = bcd_to_dec(p_time->time.sec);

	int diff = Diff_Specialtime(&time, &Time_Module[0].time);
	if (diff > 20 || diff < -20)
	{
		Time_Module[0].time = time;
		Time_Module[0].sec_form_1900 = Data2Sec(&Time_Module[0].time);
	}
	Time_Module[0].isVaild = 4;

	StorageTime();
	PLC_StartCalibMeter();
#endif
}
#ifdef LOW_POWER
extern u8 join;
#endif
//测试模式
void AppTestModelHandle(u8 *data, u16 len)
{
	APP_CREATE_MAC_DEF();
	APP_TEST_DOWN *testFrame = (APP_TEST_DOWN *)&appGeneral->Data[0];
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	debug_str(DEBUG_LOG_APP, "get test frame\r\n");
	if (appPacket->Port == 0x11)
	{
		#ifdef LOW_POWER
		//进入测试模式前需要打开hrf
		HRF_SwitchAfe(1);
		join = 1;
		#endif
		if (pNetBasePara->NetState == NET_IN_NET)
		{
			if (appGeneral->Ctrl.AckFlg)
			{
				SEND_PDU_BLOCK *pdu = MallocSendPDU();
				if (pdu == NULL)
					return;
				// APP包头
				APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
				// 业务包头
				APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];

				// 生成APP包头帧
				CreateAppPacket(sendAppPacket);
				// 生成业务包头帧
				CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_CONFIRM, 0, 0, 0, 1, APP_BUS_CONFIRM, appGeneral->AppSeq, 0, 0);

				// 发送
				pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1;
				MacHandleApp(pdu->pdu, pdu->size, 2, CCO_TEI, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);
				FreeSendPDU(pdu);
			}
		}
		else
		{
			if (EnterTestModel)
			{
				if (testFrame->TestID == 2) // 切换频段命令
				{
					if ((testFrame->Data[0] & 0xff) < 4)
					{
						HPLC_PHY_SetFrequence(testFrame->Data[0] & 0xff);
					}
					CurrentFreBeaconFcCnt = 5;
					debug_str(DEBUG_LOG_APP, "PLC_Frequence_Change=%d\r\n", testFrame->Data[0]);
				}
				else if (testFrame->TestID < 2)
				{
					if (AppCurrentModel == APP_TEST_NORMAL)
					{
						AppCurrentModel = testFrame->TestID;
						if ((testFrame->Data[0] & 0xff) < 4)
							HPLC_PHY_SetFrequence(testFrame->Data[0] & 0xff);
						LocalEnterTestModel(true);
						BPLC_GoTest(1);
						HRF_TestModeFix(); // 进入测试模式关闭滤波器
						debug_str(DEBUG_LOG_APP, "get change test frame change freq offline\n\r");
						OfflineSetState();
						HPLC_RegisterTmr(BackToAppNormalModel, 0, 10 * 60 * 1000UL, TMR_OPT_CALL_ONCE);
						HRF_TestModeFix(); // 进入测试模式关闭滤波器
						BPLC_GoTest(1);
					}
				}
				else // 新的测试模式 该测试模式的识别也可能是使用帧长来区分具体到电科院测试后确认
				{
					if (AppCurrentModel == APP_TEST_NORMAL)
					{
						COMM_TEST_PACKET *commTestPacket = (COMM_TEST_PACKET *)testFrame->Data;
						if (commTestPacket->Model == SECUER_TO_UART - APP_TEST_NEW_MODE)
						{
							if (commTestPacket->Model != SECUER_TO_UART - APP_TEST_NEW_MODE)
							{
								// Secure_TestMode(Test_Parm.SecureTestAlgorithm,&commTestPacket->Data[0]);
							}
							else
							{
								OpenClockGate(); // 打开加密模块的clock
								AppCurrentModel = commTestPacket->Model + APP_TEST_NEW_MODE;
								Test_Parm.SecureTestAlgorithm = commTestPacket->Data[0];
								LocalEnterTestModel(true);
								BPLC_SetTxGain(HPLC_TestChlPower[HPLCCurrentFreq]);
								HRF_SetTxGain(HRF_TestChlPower[(HRFCurrentIndex >> 8) & 0x1]);
								HPLC_RegisterTmr(BackToAppNormalModel, 0, commTestPacket->DataLen * 60 * 1000UL, TMR_OPT_CALL_ONCE);
							}
							return;
						}
						else if (commTestPacket->Model == APP_TESE_CHANGE_PLC - APP_TEST_NEW_MODE)
						{
							if (commTestPacket->DataLen < 3)
							{
								u8 fre = HPLC_PHY_GetFrequence();
								ResetChangeFreTimer();
								if (fre != commTestPacket->DataLen)
								{
									HPLC_PHY_SetFrequence(commTestPacket->DataLen);
									ResetCalibrateNTB();
									SetLastFreq(commTestPacket->DataLen);
									StorageStaParaInfo();
								}
							}
							debug_str(DEBUG_LOG_APP, "PLC_FREQUENCE_CHANGE=%d\r\n", commTestPacket->DataLen);
							return;
						}
						else if (commTestPacket->Model == APP_TESE_TONEMASK - APP_TEST_NEW_MODE)
						{
							if (commTestPacket->DataLen < 3)
							{
								BPLC_Stop();
								SetToneMask(commTestPacket->DataLen);
								BPLC_Start();
							}
							return;
						}
						else if (commTestPacket->Model == APP_TESE_CHANGE_HRF - APP_TEST_NEW_MODE)
						{
							u8 channel = commTestPacket->DataLen >> 4;
							u8 option = commTestPacket->DataLen & 0xf;
							if ((option < 4) && channel < 0xff)
							{
								HRFCurrentIndex = option << 8 | channel;
								HRF_PHY_SetChannel(channel, option); //暂定等待正确的帧再进行修改
								//收到频段切换的帧令hrf的频段停留在本频段再长时间的timer方便测试机台测试
								HrfSetExtIndex(HRFCurrentIndex,1);
								HrfGetFc2LongRemain(HRFCurrentIndex);
								HrfGetFc2LongRemain(HRFCurrentIndex);
								HrfGetFc2LongRemain(HRFCurrentIndex);
								// 机台设置了频段 优先选择无线入网
								SetFixLinkParent(1);
								// 机台测试会发cco信标（无用 仅用来给sta校准频偏） 收到hrf变更的条目时清除邻居来 保证关联请求的是机台发出的节点
								CleanAllData();
								if (StaSysPara.LastRFChannel != HRFCurrentIndex)
								{
									SetLastRFchannel(HRFCurrentIndex);
									StorageStaParaInfo();
								}
							}
							debug_str(DEBUG_LOG_APP, "HRF_CHANNEL_CHANGE op=%d,ch=%d\r\n", option, channel);
							return;
						}
						else if (commTestPacket->Model > SECUER_TO_UART - APP_TEST_NEW_MODE)
						{
							return;
						}
						if (commTestPacket->Model == APP_TESE_PLC2HRF - APP_TEST_NEW_MODE)
						{
							Test_Parm.PHR_MCS = (commTestPacket->Data[0] >> 4) & 0x0f;
							Test_Parm.PSDU_MSC = commTestPacket->Data[1] & 0x0f;
							Test_Parm.PbSIZE = (commTestPacket->Data[1] >> 4) & 0x0f;
						}
						HRF_TestModeFix(); // 进入测试模式关闭滤波器
						BPLC_GoTest(1);
						AppCurrentModel = commTestPacket->Model + APP_TEST_NEW_MODE;
						LocalEnterTestModel(true);
						BPLC_SetTxGain(HPLC_TestChlPower[HPLCCurrentFreq]);
						HRF_SetTxGain(HRF_TestChlPower[(HRFCurrentIndex >> 8) & 0x1]);
						HPLC_RegisterTmr(BackToAppNormalModel, 0, commTestPacket->DataLen * 60 * 1000UL, TMR_OPT_CALL_ONCE);
						debug_str(DEBUG_LOG_APP, "goto TESTMODE=%d\r\n", AppCurrentModel);
					}
				}
			}
		}
	}
	else if (GetAppCurrentModel() == APP_TEST_NORMAL && appPacket->Port == 0x12 && macHead->NID == 0 && macHead->DstTei == MAC_BROADCAST_TEI && pNetBasePara->NetState != NET_IN_NET) // 支持威胜抄控器
	{
		static u16 seq = 0;
		Reader_Support *pread = (Reader_Support *)&appGeneral->Data;
		if (pread->fix_byte[0] == 0x11 && pread->fix_byte[1] == 0x01 && pread->fix_byte[2] == 0xff && pread->send_flag && memcmp(pread->dst_addr, pNetBasePara->Mac, LONG_ADDR_LEN) == 0 && seq != pread->seq)
		{
			seq = pread->seq;
			AppLocalReceiveData(pread->src_addr, pread->dst_addr, 10, pread->seq, pread->data, pread->dl645_len, AppHandleReaderBack, SENDTYPE_LOCAL, MAC_BROADCAST_TEI, BAUD_SEARCH);
		}
		else if (pread->unkonwByte1 == 0x6a && pread->unkonwByte2 == 0x00 && pread->fix_byte[0] == 0x02 && pread->fix_byte[1] == 0x1e && pread->fix_byte[2] == 0x02 && pread->dl645_len == 0)
		{
			ResetChangeFreTimer();
			MacReStopJoin();
			SetNID(0);
			ReaderConnectOffline();
			debug_str(DEBUG_LOG_APP, "get weisheng 4 reader\n\r");
			DummyBeacon();
		}
	}
}
// 抄控器支持
void AppReaderHandle(u8 *data, u16 len)
{
	APP_CREATE_MAC_DEF();
	APP_READER_UART_FRAME *p = (APP_READER_UART_FRAME *)appGeneral->Data;
	u16 head_idx = ReadMeterFilter(APP_PRO_DLT645_2007, p->data, p->len);
	if (head_idx != 0xffff)
	{
		const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
		u8 mac[LONG_ADDR_LEN];
		memcpy(mac, &p->data[head_idx + 1], LONG_ADDR_LEN);
		TransposeAddr(mac);
		if (memcmp(pNetBasePara->Mac, mac, LONG_ADDR_LEN) != 0)
		{
			return;
		}
	}
#ifndef II_STA
	else
	{
		if (Is645Frame(p->data, p->len)) // 目的地址不是本STA地址的645帧
		{
			return;
		}
	}
#endif

	if (p->type == 0 && p->start == 1 && p->seq == GetStaBaseAttr()->ReaderSeq)
	{
		sendUartDiffRate(appGeneral->AppSeq, p->data, p->len, p->rate);
	}
}
// 停电事件处理
void AppPowerEventHandle(u8 *data, u16 len) // len 在此函数中无用
{
	// APP_CREATE_MAC_DEF();
	APP_GENERAL_PACKET *appGeneral = (APP_GENERAL_PACKET *)data;
	APP_POWER_EVENT_REPORT *powerEventPacket = (APP_POWER_EVENT_REPORT *)(appGeneral->Data);
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	u8 addr[6] = {0};
	if (pNetBasePara->NetState != NET_IN_NET)
	{
		return;
	}
	if (appGeneral->Ctrl.TranDir == 0) // 下行
	{
		// 停电上报事件CCO回应
		if (powerEventPacket->Function == POWER_EVENT_FUNC_CCO_CONFIRM)
		{
			if (CurrentPowerState == POWER_EVENT_WAIT_ACK)
			{
				memcpy(addr, pNetBasePara->Mac, 6);
				TransposeAddr(addr);

				// if(memcmp(powerEventPacket->MeterAddr, addr, 6) == 0)
				{
					if (powerUpPacketID == appGeneral->AppSeq)
					{
						if (signalPowerEvent == 0)
						{
							if (IsPowerOff)
							{
								IsPowerOff = 0;
								StoragePowerOffFlag();
							}
						}

						if (PowerEventPdu)
						{
							FreeSendPDU(PowerEventPdu);
							PowerEventPdu = NULL;
						}
						CurrentPowerState = POWER_EVENT_IDLE;
					}
				}
			}
		}
	}
	else // 上行
	{
		// 其他站点停电上报事件
		if ((powerEventPacket->Function == POWER_EVENT_FUNC_TRIGGER_BY_MODULE) ||
			(powerEventPacket->Function == POWER_EVENT_FUNC_TRIGGER_BY_COLLECT))
		{
			//		    u8 update_flag = 0;
			POWER_EVENT_UP_MAC *pMAC = (POWER_EVENT_UP_MAC *)powerEventPacket->Data;
			POWER_EVENT_UP_BITMAP *pBIT = (POWER_EVENT_UP_BITMAP *)powerEventPacket->Data;

			if ((pMAC->upType == POWER_EVENT_TYPE_ON_BIT) || (pMAC->upType == POWER_EVENT_TYPE_ON_MAC))
			{
				return;
			}

			if (pMAC->upType == POWER_EVENT_TYPE_OFF_MAC)
			{
				for (int i = 0; i < (powerEventPacket->DataLen - 3) / 7; i++)
				{
					// 修改不再检查带电状态 无论表是否带电都加入MAC表中
					// if(pMAC->data[i][6] == 0)
					{
						if (PowerMACHashInsert(&pMAC->data[i][0]))
						{
							update_flag = 1; // MAC有更新
							PowerOffNum++;
						}
					}
				}
			}

			if (pBIT->upType == POWER_EVENT_TYPE_OFF_BIT)
			{
				// u16 startByte;
				// u8 real_bitmap = 0;

				// if((pBIT->startTei&0x7) == 0)
				{
					// startByte = pBIT->startTei/8;
					if ((pBIT->startTei + (powerEventPacket->DataLen - 3) * 8 - 1) > PowerOffTEI)
					{
						PowerOffTEI = pBIT->startTei + (powerEventPacket->DataLen - 3) * 8 - 1;
					}

					if (IsPowerOff && (!power))
					{
						PowerOffEventBitmap[pNetBasePara->TEI / 8] |= (1 << (pNetBasePara->TEI & 0x7));
						PowerOffTEI = PowerOffTEI > pNetBasePara->TEI ? PowerOffTEI : pNetBasePara->TEI;
					}

					if (PowerOffTEI > MAX_TEI_VALUE)
					{
						PowerOffTEI = MAX_TEI_VALUE;
					}

					for (int i = 0; i < powerEventPacket->DataLen - 3; i++)
					{
#if 1
						if (pBIT->bitMap[i])
						{
							for (int j = 0; j < 8; j++)
							{
								if (pBIT->bitMap[i] & (1 << j))
								{
									u16 bitmap_tei = pBIT->startTei + 8 * i + j;
									if (!(PowerOffEventBitmap[bitmap_tei / 8] & (1 << (bitmap_tei & 0x7))))
									{
										update_flag = 1;
										PowerOffEventBitmap[bitmap_tei / 8] |= (1 << (bitmap_tei & 0x7));
									}
								}
							}
						}
/*
						if(pBIT->startTei&0x7) //除8有余数
						{
							real_bitmap = pBIT->bitMap[i]<<(pBIT->startTei&0x7);
							if(i != 0)
							{
								real_bitmap |= pBIT->bitMap[i-1]>>(8-(pBIT->startTei&0x7));
							}
						}
						else
						{
							real_bitmap = pBIT->bitMap[i];
						}
						if(PowerOffEventBitmap[startByte] != real_bitmap)
						{
							update_flag = 1; //位图有更新
							PowerOffEventBitmap[startByte] |= real_bitmap;
						}
						if((pBIT->startTei&0x7) && (i == eventPacket->DataLen - 4) && (i != 0))
						{
							real_bitmap = pBIT->bitMap[i]>>(8-(pBIT->startTei&0x7));

							if(PowerOffEventBitmap[startByte+1] != real_bitmap)
							{
								update_flag = 1; //位图有更新
								PowerOffEventBitmap[startByte+1] |= real_bitmap;
							}
						}
*/
#else
						if (PowerOffEventBitmap[startByte] != pBIT->bitMap[i])
						{
							update_flag = 1; // 位图有更新
							PowerOffEventBitmap[startByte] |= pBIT->bitMap[i];
						}
#endif
						//				startByte++;
					}
				}
			}
#if 0
			switch (CurrentPowerState)
			{
				case POWER_EVENT_IDLE: //空闲状态
				if((CCO_TEI == GetRelayTEI(CCO_TEI)) && power) //本站点是一级站点且有电,收集其他站点停电上报报文30秒后统一上报
				{
					HPLC_RegisterTmr(upPowerEvent, SEND_POWER_OFF, MAX_POWER_EVENT_DECETC_TIME, TMR_OPT_CALL_ONCE);
					CurrentPowerState = POWER_EVENT_WAIT_TIME;
					StartTimer(&PowerEventTimer, MAX_POWER_EVENT_DECETC_TIME+1000);
				}
				else if((power && update_flag) || (!power)) //本站点(非一级站点)有电且停电位图或MAC有更新，或者本站点没电
				{
					HPLC_RegisterTmr(upPowerEvent, SEND_POWER_OFF, rand()%200+10, TMR_OPT_CALL_ONCE);
					CurrentPowerState = POWER_EVENT_WAIT_TIME;
					StartTimer(&PowerEventTimer, MAX_POWER_EVENT_DECETC_TIME+1000);
				}
				break;
				default:
				break;
			}
#endif
		}
	}
}

// 确认否认
void AppDataConfirmHandle(u8 *data, u16 len)
{
	if (AppEventState == APP_EVENT_WAIT_ACK)
	{
		APP_CREATE_MAC_DEF();

		if (meterPacketID == appGeneral->AppSeq)
		{
			switch (appGeneral->BusFlg)
			{
			case APP_BUS_CONFIRM:
			{
				if (EventPdu)
				{
					FreeSendPDU(EventPdu);
					EventPdu = NULL;
				}
				AppEventState = APP_EVENT_CLEAR_HIGH;
				break;
			}
			case APP_BUS_DENY:
			{
				if (appGeneral->Data[0] == APP_DENY_CCO_BUSY) // CCO忙
				{
					AppEventState = APP_EVENT_OVERFLOW;
				}
				else
				{
					if (EventPdu)
					{
						FreeSendPDU(EventPdu);
						EventPdu = NULL;
					}
					AppEventState = APP_EVENT_IDLE;
				}
				break;
			}
			default:
				break;
			}
		}
	}
	if (CurrentPowerState == POWER_EVENT_WAIT_ACK)
	{
		APP_CREATE_MAC_DEF();

		// if(memcmp(powerEventPacket->MeterAddr, addr, 6) == 0)
		{
			if (powerUpPacketID == appGeneral->AppSeq)
			{
				if (signalPowerEvent == 0)
				{
					if (IsPowerOff)
					{
						IsPowerOff = 0;
						StoragePowerOffFlag();
					}
				}

				if (PowerEventPdu)
				{
					FreeSendPDU(PowerEventPdu);
					PowerEventPdu = NULL;
				}
				CurrentPowerState = POWER_EVENT_IDLE;
			}
		}
	}
}
extern u8 last_up_status;
// 查询节点信息
void AppQueryStationHandle(u8 *data, u16 len)
{
	APP_CREATE_MAC_DEF();

	APP_QUIRY_INFO *queryDown = (APP_QUIRY_INFO *)&appGeneral->Data[0];
	u8 i = 0;
	const ST_STA_VER_TYPE *psVerInfo = GetStaVerInfo();
	const ST_STA_EXT_VER_TYPE *psExtVerInfo = GetStaExtVerInfo();
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	SEND_PDU_BLOCK *pdu = MallocSendPDU();
	if (pdu == NULL)
		return;

	// APP包头
	APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
	// 业务包头
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];
	// 信息上行帧
	APP_QUIRY_INFO *queryUp = (APP_QUIRY_INFO *)&sendAppGeneralPacket->Data[0];

	// 生成APP包头帧
	CreateAppPacket(sendAppPacket);
	// 生成业务包头帧
	CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_CMD, 0, 0, 0, 1, APP_BUS_QUIRY, appGeneral->AppSeq, 0, 0);
	queryUp->Num = queryDown->Num;

	u16 index = 0;
	if (queryDown->Num > (0x10 + 1)) // 信息元素数量，南网21标准信息元素ID目前最大定义到0x10（从0开始）
	{
		FreeSendPDU(pdu);
		return;
	}

	// 南网21标准规定资产信息小端传输
	for (i = 0; i < queryDown->Num; i++)
	{
		queryUp->Data[index] = queryDown->Data[i]; // 元素ID
		switch (queryDown->Data[i])
		{
		case 0: // 厂商编号，2字节ASCII
		{
			queryUp->Data[index + 1] = 2; // 数据长度
			memcpy_swap(&queryUp->Data[index + 2], (u8 *)psVerInfo->FactoryID, 2);
			break;
		}
		case 1: // 软件版本信息（模块），2字节BCD
		{
			queryUp->Data[index + 1] = 2; // 数据长度
			memcpy_swap(&queryUp->Data[index + 2], (u8 *)psVerInfo->SofeVer, 2);
			break;
		}
		case 2: // bootloader版本号，1字节BIN
		{
			queryUp->Data[index + 1] = 1; // 数据长度
			queryUp->Data[index + 2] = psVerInfo->BootVer;
			break;
		}
		case 3: // CRC-32,4B
		{
			queryUp->Data[index + 1] = 4; // 数据长度
			u32 *dataZone = (u32 *)&queryUp->Data[index + 2];
			*dataZone = GetUpdateFileCRC();
			break;
		}
		case 4: // 文件长度,4B
		{
			queryUp->Data[index + 1] = 4; // 数据长度
			u32 *dataZone = (u32 *)&queryUp->Data[index + 2];
			*dataZone = GetUpdateFileSize();
			break;
		}
		case 5: // 芯片厂商代码，2字节ASCII
		{
			queryUp->Data[index + 1] = 2; // 数据长度
			memcpy_swap(&queryUp->Data[index + 2], (u8 *)psVerInfo->ChipCode, 2);
			break;
		}
		case 6: // 固件发布日期（模块），3字节BIN
		{
			u8 data[3];
#if 1 // 南网21标准规定资产信息中日期BIN格式
			data[0] = psVerInfo->VerDate.Year;
			data[1] = psVerInfo->VerDate.Month;
			data[2] = psVerInfo->VerDate.Day;
#else
			data[0] = dec_to_bcd(psVerInfo->VerDate.Year);
			data[1] = dec_to_bcd(psVerInfo->VerDate.Month);
			data[2] = dec_to_bcd(psVerInfo->VerDate.Day);
#endif
			queryUp->Data[index + 1] = 3; // 数据长度
			memcpy_swap(&queryUp->Data[index + 2], data, 3);
			break;
		}
		case 7:
		{
			queryUp->Data[index + 1] = 8;
			queryUp->Data[index + 2] = last_up_status;
			memset(&queryUp->Data[index + 2], 0, 7);
			break;
		}
		case 8: // 模块出厂MAC地址，6字节BIN
		{
			queryUp->Data[index + 1] = 6;
			memcpy(&queryUp->Data[index + 2], StaChipID.mac, 6); // 小端储存
			break;
		}
			// 南网21标准新增
		case 9: // 硬件版本信息（模块），2字节BCD
		{
			queryUp->Data[index + 1] = 2; // 数据长度
			memcpy_swap(&queryUp->Data[index + 2], (u8 *)psExtVerInfo->ModuleHardVer, 2);

			break;
		}
		case 0xA: // 硬件发布日期（模块），3字节BIN
		{
			queryUp->Data[index + 1] = 3; // 数据长度

			u8 data[3] = {0};
#if 1 // 南网21标准规定资产信息中日期BIN格式
			data[0] = psExtVerInfo->ModuleHardVerDate.Year;
			data[1] = psExtVerInfo->ModuleHardVerDate.Month;
			data[2] = psExtVerInfo->ModuleHardVerDate.Day;
#else
			data[0] = dec_to_bcd(psExtVerInfo->ModuleHardVerDate.Year);
			data[1] = dec_to_bcd(psExtVerInfo->ModuleHardVerDate.Month);
			data[2] = dec_to_bcd(psExtVerInfo->ModuleHardVerDate.Day);
#endif
			memcpy_swap(&queryUp->Data[index + 2], data, 3);

			break;
		}
		case 0xB: // 软件版本信息（芯片），2字节BCD
		{
			queryUp->Data[index + 1] = 2; // 数据长度
			memcpy_swap(&queryUp->Data[index + 2], (u8 *)psExtVerInfo->ChipSoftVer, 2);

			break;
		}
		case 0xC: // 软件发布日期（芯片），3字节BIN
		{
			queryUp->Data[index + 1] = 3; // 数据长度

			u8 data[3];
#if 1 // 南网21标准规定资产信息中日期BIN格式
			data[0] = psExtVerInfo->ChipSofVerDate.Year;
			data[1] = psExtVerInfo->ChipSofVerDate.Month;
			data[2] = psExtVerInfo->ChipSofVerDate.Day;
#else
			data[0] = dec_to_bcd(psExtVerInfo->ChipSofVerDate.Year);
			data[1] = dec_to_bcd(psExtVerInfo->ChipSofVerDate.Month);
			data[2] = dec_to_bcd(psExtVerInfo->ChipSofVerDate.Day);
#endif
			memcpy_swap(&queryUp->Data[index + 2], data, 3);

			break;
		}
		case 0xD: // 硬件版本信息（芯片），2字节BCD
		{
			queryUp->Data[index + 1] = 2; // 数据长度
			memcpy_swap(&queryUp->Data[index + 2], (u8 *)psExtVerInfo->ChipHardVer, 2);

			break;
		}
		case 0xE: // 硬件发布日期（芯片），3字节BIN
		{
			queryUp->Data[index + 1] = 3; // 数据长度

			u8 data[3];
#if 1 // 南网21标准规定资产信息中日期BIN格式
			data[0] = psExtVerInfo->ChipHardVerDate.Year;
			data[1] = psExtVerInfo->ChipHardVerDate.Month;
			data[2] = psExtVerInfo->ChipHardVerDate.Day;
#else
			data[0] = dec_to_bcd(psExtVerInfo->ChipHardVerDate.Year);
			data[1] = dec_to_bcd(psExtVerInfo->ChipHardVerDate.Month);
			data[2] = dec_to_bcd(psExtVerInfo->ChipHardVerDate.Day);
#endif
			memcpy_swap(&queryUp->Data[index + 2], data, 3);

			break;
		}
		case 0xF: // 应用程序版本号，2字节BCD
		{
			queryUp->Data[index + 1] = 2; // 数据长度
			memcpy_swap(&queryUp->Data[index + 2], (u8 *)psExtVerInfo->AppVer, 2);

			break;
		}
		case 0x10: // 通信模块资产编码，24字节ASCII
		{
			queryUp->Data[index + 1] = 24;									  // 数据长度
			memcpy_swap(&queryUp->Data[index + 2], StaChipID.assetsCode, 24); // 大端储存，转换字节序

			break;
		}
		default:
			queryUp->Data[index + 1] = 0; // 未支持元素ID，数据长度赋值0
			break;
		}
		index += (2 + queryUp->Data[index + 1]);
	}

	sendAppGeneralPacket->AppFrameLen = sizeof(APP_QUIRY_INFO) - 1 + index;
	pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
	MacHandleApp(pdu->pdu, pdu->size, 2, macHead->SrcTei, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);
	FreeSendPDU(pdu);
}
extern u32 getRunTimeS(void);
extern u8 GetUartStatus(void);
extern u8 GetResetReason(void);
// 查询节点运行状态信息
void AppRunStatusHandle(u8 *data, u16 len)
{
	APP_CREATE_MAC_DEF();

	APP_QUIRY_STATUS *queryDown = (APP_QUIRY_STATUS *)&appGeneral->Data[0];
	u8 i = 0;
	const ST_STA_VER_TYPE *psVerInfo = GetStaVerInfo();
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	SEND_PDU_BLOCK *pdu = MallocSendPDU();
	if (pdu == NULL)
		return;

	// APP包头
	APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
	// 业务包头
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];
	// 信息上行帧
	APP_QUIRY_STATUS *queryUp = (APP_QUIRY_STATUS *)&sendAppGeneralPacket->Data[0];

	// 生成APP包头帧
	CreateAppPacket(sendAppPacket);
	sendAppPacket->Port = 0x13;
	// 生成业务包头帧
	CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_CMD, 0, 0, 0, 1, APP_BUS_STA_STATUS, appGeneral->AppSeq, 0, 0);
	queryUp->Num = queryDown->Num;

	u16 index = 0;
	if (queryDown->Num > 5)
	{
		FreeSendPDU(pdu);
		return;
	}

	for (i = 0; i < queryDown->Num; i++)
	{
		queryUp->Data[index] = queryDown->Data[i]; // 元素ID
		switch (queryDown->Data[i])
		{
		case 0: // 运行时长
		{
			queryUp->Data[index + 1] = 4; // 数据长度
			*(u32 *)&queryUp->Data[index + 2] = getRunTimeS();
			break;
		}
		case 1: // 过零自检结果
		{
			queryUp->Data[index + 1] = 1; // 数据长度
			queryUp->Data[index + 2] = ZeroSelfStatus();
			break;
		}
		case 2: // 串口不通状态
		{
			queryUp->Data[index + 1] = 1; // 数据长度
			queryUp->Data[index + 2] = GetUartStatus();
			break;
		}
		case 3: // 上次离网原因
		{
			queryUp->Data[index + 1] = 1; // 数据长度
			queryUp->Data[index + 2] = GetLastOfflineReasonNum();
			break;
		}
		case 4: // 复位原因
		{
			queryUp->Data[index + 1] = 1; // 数据长度
			queryUp->Data[index + 2] = GetResetReason();
			break;
		}

		default:
			break;
		}
		index += (2 + queryUp->Data[index + 1]);
	}
	sendAppGeneralPacket->AppFrameLen = sizeof(APP_QUIRY_STATUS) + index;

	pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;

	MacHandleApp(pdu->pdu, pdu->size, 2, macHead->SrcTei, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);
	FreeSendPDU(pdu);
}

void AppStaChannelHandle(u8 *data, u16 len)
{
	APP_CREATE_MAC_DEF();

	APP_QUIRY_CHANNEL *queryDown = (APP_QUIRY_CHANNEL *)&appGeneral->Data[0];
	u16 i = 0;
	if (queryDown->num < 1)
	{
		return;
	}

	u8 up_num = 0, limit_num = 0;
	u16 ngh_tei = 0, ngh_idx = 0;
	const ST_STA_VER_TYPE *psVerInfo = GetStaVerInfo();
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	SEND_PDU_BLOCK *pdu = MallocSendPDU();
	if (pdu == NULL)
		return;

	// APP包头
	APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
	// 业务包头
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];
	// 信息上行帧
	APP_QUIRY_CHANNEL_UP *queryUp = (APP_QUIRY_CHANNEL_UP *)&sendAppGeneralPacket->Data[0];

	// 生成APP包头帧
	CreateAppPacket(sendAppPacket);
	sendAppPacket->Port = 0x13;
	// 生成业务包头帧
	CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_CMD, 0, 0, 0, 1, APP_BUS_STA_CHANNEL, appGeneral->AppSeq, 0, 0);
	const ST_NEIGHBOR_TAB_TYPE *pneighbor = GetNeighborTable();
	queryUp->num = 0;

	queryUp->total = pneighbor->NeighborNum;
	if (queryDown->num > 6)
	{
		queryDown->num = 6;
	}
	limit_num = (queryDown->num > queryUp->total) ? queryUp->total : queryDown->num; // 实际查询最大个数，取最小值

	APP_CHANNEL_INFO *pinfo = (APP_CHANNEL_INFO *)queryUp->data;
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();

	for (i = 0; i < MAX_BITMAP_SIZE / 4; i++)
	{
		if (StaNeighborTab.TEI_MAP[i])
		{
			for (int bit = 0; bit < 32; bit++)
			{
				if (!(StaNeighborTab.TEI_MAP[i] & (1 << bit)))
				{
					continue;
				}

				if (up_num == limit_num)
				{
					break;
				}

				if (queryDown->start)
				{
					--queryDown->start;
					continue;
				}
				ngh_tei = i * 32 + bit;
				ngh_idx = pneighbor->TEI[ngh_tei];
				memcpy((pinfo + up_num)->mac, pneighbor->Neighbor[ngh_idx].Mac, LONG_ADDR_LEN);
				(pinfo + up_num)->tei = ngh_tei;
				(pinfo + up_num)->pco_tei = pneighbor->Neighbor[ngh_idx].PcoTEI;
				(pinfo + up_num)->level = pneighbor->Neighbor[ngh_idx].Layer;
				(pinfo + up_num)->snr = pneighbor->Neighbor[ngh_idx].SNR;
				(pinfo + up_num)->attenuator = 80 - pneighbor->Neighbor[ngh_idx].RSSI;
				GetNeighborSucRate(ngh_tei, &((pinfo + up_num)->up_rate), &((pinfo + up_num)->down_rate), &((pinfo + up_num)->min_rate), true, 0);
				queryUp->num++;
				up_num++;
			}
		}
	}
	OS_CRITICAL_EXIT();
	sendAppGeneralPacket->AppFrameLen = sizeof(APP_QUIRY_CHANNEL_UP) + queryUp->num * sizeof(APP_CHANNEL_INFO);

	pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;

	MacHandleApp(pdu->pdu, pdu->size, 2, macHead->SrcTei, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);
	FreeSendPDU(pdu);
}
// 下发映射表处理
void AppMeterListHandle(u8 *data, u16 len)
{
	APP_CREATE_MAC_DEF();
	SEND_PDU_BLOCK *pdu = MallocSendPDU();
	if (pdu == NULL)
		return;
	// APP包头
	APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
	// 业务包头
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];

	// 生成APP包头帧
	CreateAppPacket(sendAppPacket);
	// 生成业务包头帧
	CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_CONFIRM, 0, 0, 0, 1, APP_BUS_CONFIRM, appGeneral->AppSeq, 0, 0);

	// 发送
	pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1;
	MacHandleApp(pdu->pdu, pdu->size, 2, CCO_TEI, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);
	FreeSendPDU(pdu);
}

s32 factoryTestRecvRssi = 0xF0000000;

void AppExtFactoryTestHandle(BPLC_PARAM *bplcParam, u8 *data, u16 len)
{
	APP_CREATE_MAC_DEF();
	u16 up_data_len = 0, index = 0;
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();

	// 计算上行数据长度
	switch (appGeneral->BusFlg)
	{
	case APP_BUS_EXT_FACTORY_SET_MAC:
	{
		if (appGeneral->AppFrameLen != 12) // 长度判断
		{
			return;
		}
		up_data_len = 12; // 上行返回数据长度，6(节点地址)+6(MAC地址)
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_MAC:
	{
		if (appGeneral->AppFrameLen != 6) // 长度判断
		{
			return;
		}
		up_data_len = 12; // 6(节点地址)+6(MAC地址)
		break;
	}
#ifndef __DEBUG_MODE
	case APP_BUS_EXT_FACTORY_SET_FACTORY_CODE:
	{
		if (appGeneral->AppFrameLen != 15) // 长度判断
		{
			return;
		}
		up_data_len = 15; // 6(节点地址)+2(厂商代码)+2(芯片代码)+3(版本日期)+2(版本)
		break;
	}
#endif
	case APP_BUS_EXT_FACTORY_GET_ZERO:
	{
		if (appGeneral->AppFrameLen != 6) // 长度判断
		{
			return;
		}
		up_data_len = 23; // 6(节点地址)+1(采集方式)+1(设备类型)+3*(1(是否存在)+4(过零周期))
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_VER_ALL:
	{
		if (appGeneral->AppFrameLen != 6) // 长度判断
		{
			return;
		}
		up_data_len = 34; // 上行返回数据长度，从节点地址(6)+厂商代码(2)+芯片代码(2)+模板版本(5)+软件版本(7)+硬件版本(7)+内部版本(5)
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_TXPOWER:
	{
		if (appGeneral->AppFrameLen != 6) // 长度判断
		{
			return;
		}
		up_data_len = 10; // 上行返回数据长度，从节点地址(6)+发射功率(4)
		break;
	}
	case APP_BUS_EXT_FACTORY_SET_TXPOWER:
	{
		if (appGeneral->AppFrameLen != 10) // 长度判断
		{
			return;
		}
		up_data_len = 0; // 不回复
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_HRFTXPOWER:
	{
		if (appGeneral->AppFrameLen != 6) //长度判断
		{
			return;
		}
		up_data_len = 8; //6(节点地址+2(发射功率 op2op3
		break;
	}
	case APP_BUS_EXT_FACTORY_SET_HRFTXPOWER:
	{
		if (appGeneral->AppFrameLen != 8) //长度判断 addr(6)+op2(1)+op3(1)
		{
			return;
		}
		up_data_len = 0; //不回复
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_PLL:
	{
		if (appGeneral->AppFrameLen != 6) // 长度判断
		{
			return;
		}
		up_data_len = 8; // 6(节点地址+2(PLL
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_IO:
	{
		if (appGeneral->AppFrameLen != 6) // 长度判断
		{
			return;
		}
		up_data_len = 12; // 6(节点地址+2(event insert +4 res
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_WAFER:
	{
		if (appGeneral->AppFrameLen != 6) // 长度判断
		{
			return;
		}
		up_data_len = 14; // 6(节点地址)+8(wafer ID)
		break;
	}
	case APP_BUS_EXT_FACTORY_IR_TEST:
	{
		if (appGeneral->AppFrameLen != 6) // 长度判断
		{
			return;
		}
		up_data_len = 7; // 6(节点地址)+1(status)
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_RSSI:
	{
		if (appGeneral->AppFrameLen != 6) // 长度判断
		{
			return;
		}
		up_data_len = 10; // 6(节点地址)+4(RSSI)
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_ASSETS_CODE:
	{
		if (appGeneral->AppFrameLen != 6) // 长度判断
		{
			return;
		}
		up_data_len = 30; // 6(节点地址)+24(资产编码)
		break;
	}
	case APP_BUS_EXT_FACTORY_SET_ASSETS_CODE:
	{
		if (appGeneral->AppFrameLen != 30) // 长度判断
		{
			return;
		}
		up_data_len = 30; // 6(节点地址)+24(资产编码)
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_EXT_VER:
	{
		if (appGeneral->AppFrameLen != 6) // 长度判断
		{
			return;
		}
		up_data_len = 23; // 6(节点地址)+17(扩展版本信息)
		break;
	}
	case APP_BUS_EXT_FACTORY_SET_EXT_VER:
	{
		if (appGeneral->AppFrameLen != 23) // 长度判断
		{
			return;
		}
		up_data_len = 23; // 6(节点地址)+17(扩展版本信息)
		break;
	}

	default:
		return;
	}
	// 数据域前6个字节是从节点地址
	u8 addr[6] = {0};
	memcpy(addr, &appGeneral->Data[0], 6);
	TransposeAddr(addr); // 地址转成大端
	if (memcmp(addr, pNetBasePara->Mac, 6) != 0)
	{
		if (memIsHex(addr, 0xFF, 6) &&
			((appGeneral->BusFlg == APP_BUS_EXT_FACTORY_GET_TXPOWER) || (appGeneral->BusFlg == APP_BUS_EXT_FACTORY_SET_TXPOWER)))
		{
			memcpy(addr, pNetBasePara->Mac, 6);
		}
		else
		{
			return;
		}
	}
	else if (macHead->SrcTei == READER_TEI)
	{
		factoryTestRecvRssi = bplcParam->Rssi;
	}

	SEND_PDU_BLOCK *pdu = NULL;
	APP_PACKET *sendAppPacket = NULL;
	APP_GENERAL_PACKET *sendAppGeneralPacket = NULL;
	if (up_data_len != 0)
	{
		pdu = MallocSendPDU();
		if (pdu == NULL)
		{
			return;
		}

		// APP包头
		sendAppPacket = (APP_PACKET *)pdu->pdu;
		// 业务包头
		sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];

		// 生成APP包头帧
		CreateAppPacket(sendAppPacket);
		sendAppPacket->Port = 0xFF;
		// 生成业务包头帧
		CreateAppBusinessPacket(sendAppGeneralPacket, appGeneral->Ctrl.FrameType, 0, 0, 0, 1, appGeneral->BusFlg, appGeneral->AppSeq, 0, 0);

		// 组数据内容
		TransposeAddr(addr);
		memcpy(&sendAppGeneralPacket->Data[index], addr, 6);
	}

	index += 6;

	switch (appGeneral->BusFlg)
	{
	case APP_BUS_EXT_FACTORY_SET_MAC:
	{
		memcpy(StaChipID.mac, &appGeneral->Data[6], LONG_ADDR_LEN);
		StorageStaIdInfo();

		memcpy(&sendAppGeneralPacket->Data[6], &appGeneral->Data[6], up_data_len - 6);
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_MAC:
	{
		ReadStaIdInfo();

		memcpy(&sendAppGeneralPacket->Data[index], StaChipID.mac, LONG_ADDR_LEN);

		break;
	}
#ifndef __DEBUG_MODE
	case APP_BUS_EXT_FACTORY_SET_FACTORY_CODE:
	{
		StaVersion.FactoryID[1] = appGeneral->Data[index++];
		StaVersion.FactoryID[0] = appGeneral->Data[index++];
		StaVersion.ChipCode[1] = appGeneral->Data[index++];
		StaVersion.ChipCode[0] = appGeneral->Data[index++];
		StaVersion.VerDate.Day = bcd_to_dec(appGeneral->Data[index++]);
		StaVersion.VerDate.Month = bcd_to_dec(appGeneral->Data[index++]);
		StaVersion.VerDate.Year = bcd_to_dec(appGeneral->Data[index++]);
		StaVersion.SofeVer[1] = appGeneral->Data[index++];
		StaVersion.SofeVer[0] = appGeneral->Data[index++];

		StorageVerInfo();

		memcpy(&sendAppGeneralPacket->Data[6], &appGeneral->Data[6], up_data_len - 6);
		break;
	}
#endif
	case APP_BUS_EXT_FACTORY_GET_ZERO:
	{
		sendAppGeneralPacket->Data[index++] = ZERO_EDGE_AREA;
		sendAppGeneralPacket->Data[index++] = (myDevicType == 7 ? 2 : 1); // 设备类型

		for (u8 i = 0; i < 3; i++)
		{
			sendAppGeneralPacket->Data[index++] = zero_check_pass[i];

			memset(&sendAppGeneralPacket->Data[index], 0x00, 4);
			if (zero_check_pass[i])
			{
				memcpy(&sendAppGeneralPacket->Data[index], &zeroNtbDiff[i], 4);
			}
			index += 4;
		}
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_VER_ALL:
	{
		ReadVerInfo();

		sendAppGeneralPacket->Data[index++] = StaVersion.FactoryID[1];
		sendAppGeneralPacket->Data[index++] = StaVersion.FactoryID[0];
		sendAppGeneralPacket->Data[index++] = StaVersion.ChipCode[1];
		sendAppGeneralPacket->Data[index++] = StaVersion.ChipCode[0];

		// 模块版本信息
		sendAppGeneralPacket->Data[index++] = StaVersion.SofeVer[1];
		sendAppGeneralPacket->Data[index++] = StaVersion.SofeVer[0];
		sendAppGeneralPacket->Data[index++] = dec_to_bcd(StaVersion.VerDate.Day);
		sendAppGeneralPacket->Data[index++] = dec_to_bcd(StaVersion.VerDate.Month);
		sendAppGeneralPacket->Data[index++] = dec_to_bcd(StaVersion.VerDate.Year);

		// 软件版本信息
		memset(&sendAppGeneralPacket->Data[index], 0, 7);
		index += 7;

		// 硬件版本信息
		memset(&sendAppGeneralPacket->Data[index], 0, 7);
		index += 7;

		// 内部版本信息
		sendAppGeneralPacket->Data[index++] = Hex2BcdChar(SLG_COMMIT_DATE);
		sendAppGeneralPacket->Data[index++] = Hex2BcdChar(SLG_COMMIT_MON);
		sendAppGeneralPacket->Data[index++] = Hex2BcdChar(SLG_COMMIT_YEAR % 100);
		sendAppGeneralPacket->Data[index++] = (SLG_COMMIT_HASH >> 12) & 0xff;
		sendAppGeneralPacket->Data[index++] = (SLG_COMMIT_HASH >> 20) & 0xff;

		break;
	}
	case APP_BUS_EXT_FACTORY_GET_TXPOWER:
	{
		ReadStaFixPara();

		memset(&sendAppGeneralPacket->Data[index], 0x00, 4);
		if (StaFixPara.txPowerMode[0] != 0)
		{
			memcpy(&sendAppGeneralPacket->Data[index], StaFixPara.txPower, 4);
		}

		break;
	}
	case APP_BUS_EXT_FACTORY_SET_TXPOWER:
	{
		u8 valid_count = 0;

		for (u8 i = 0; i < 4; i++)
		{
			if (appGeneral->Data[index + i] == 0)
			{
				valid_count += 2;
			}
			else if(appGeneral->Data[index+i] <= 13)
			{
				valid_count += 3;
			}
		}

		if ((valid_count == 8) || (valid_count == 12)) // 数值全部有效，数值全0或者全部在范围之内
		{
			if (memIsHex(&appGeneral->Data[index], 0x00, 4)) // 数值全0
			{
				StaFixPara.txPowerMode[0] = 0; // 发射功率自动调整模式
				memset(StaFixPara.txPower, 0x00, 4);
			}
			else
			{
				StaFixPara.txPowerMode[0] = 1; // 发射功率固定模式
				memcpy(StaFixPara.txPower, &appGeneral->Data[index], 4);
			}

			StorageStaFixPara();
			StaTxPowerProcess();
			u8 appModel = GetAppCurrentModel();
			if (appModel == APP_TEST_NORMAL) // 正常模式
			{
				BPLC_SetTxGain(HPLC_ChlPower[HPLCCurrentFreq]);
			}
		}

		break;
	}
	case APP_BUS_EXT_FACTORY_GET_HRFTXPOWER:
	{
		ReadStaFixPara();
		sendAppGeneralPacket->Data[index] = 0;
		if (StaFixPara.txPowerMode[1] != 0)
		{
			memcpy(&sendAppGeneralPacket->Data[index], StaFixPara.hrfTxPower, 2);
		}
		break;
	}
	case APP_BUS_EXT_FACTORY_SET_HRFTXPOWER:
	{
#if defined(ZB204_CHIP)
		if (appGeneral->Data[index] <= 31 && appGeneral->Data[index + 1] <= 31) // 数值全部有效
#elif defined(ZB205_CHIP)
		if (appGeneral->Data[index] <= 15 && appGeneral->Data[index + 1] <= 15) // 数值全部有效
#endif
		{
			if (memIsHex(&appGeneral->Data[index], 0x00, 2)) // 数值全0
			{
				StaFixPara.txPowerMode[1] = 0; // 发射功率自动调整模式
				memset(StaFixPara.hrfTxPower, 0x00, 2);
			}
			else
			{
				StaFixPara.txPowerMode[1] = 1; // 发射功率固定模式
				memcpy(StaFixPara.hrfTxPower, &appGeneral->Data[index], 2);
			}
			StorageStaFixPara();
			StaTxPowerProcess();
			if (GetAppCurrentModel() == APP_TEST_NORMAL) // 正常模式
			{
				HRF_SetTxGain(HRF_ChlPower[(HRFCurrentIndex >> 8) & 0x1]);
			}
		}
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_PLL:
	{
		ReadPllTrim();
		s32 temp_PLL_Trimming = PLL_Trimming * 10 / 16; // PLL_Trimming单位1/16ppm，转换为单位0.1ppm
		if (temp_PLL_Trimming == 0 && PLL_Vaild)
		{
			*(s16 *)&sendAppGeneralPacket->Data[index] = 1;
		}
		else
		{
			*(s16 *)&sendAppGeneralPacket->Data[index] = temp_PLL_Trimming;
		}
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_IO:
	{
		memset(&sendAppGeneralPacket->Data[index], 0, 6);
		sendAppGeneralPacket->Data[index++] = EventRead();
		sendAppGeneralPacket->Data[index++] = InsertRead();
#ifdef RESET_PORT
		sendAppGeneralPacket->Data[index++] = GPIO_PinRead(RESET_PORT, RESET_PIN);
#endif
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_WAFER:
	{
		memset(&sendAppGeneralPacket->Data[index], 0, 8);
		BPLC_GetWaferID(&sendAppGeneralPacket->Data[index]);
		break;
	}
	case APP_BUS_EXT_FACTORY_IR_TEST:
	{
		sendAppGeneralPacket->Data[index] = UartIrTest(pNetBasePara->Mac);

		break;
	}
	case APP_BUS_EXT_FACTORY_GET_RSSI:
	{
		memset(&sendAppGeneralPacket->Data[index], 0, 4);
		*(s32 *)&sendAppGeneralPacket->Data[index] = bplcParam->Rssi;
		break;
	}
	case APP_BUS_EXT_FACTORY_GET_ASSETS_CODE:
	{
		ReadStaIdInfo();

		memset(&sendAppGeneralPacket->Data[index], 0, 24);
		memcpy(&sendAppGeneralPacket->Data[index], StaChipID.assetsCode, 24);

		break;
	}
	case APP_BUS_EXT_FACTORY_SET_ASSETS_CODE:
	{
		memcpy(StaChipID.assetsCode, &appGeneral->Data[6], 24);
		StorageStaIdInfo();

		memcpy(&sendAppGeneralPacket->Data[6], &appGeneral->Data[6], up_data_len - 6);

		break;
	}
	case APP_BUS_EXT_FACTORY_GET_EXT_VER:
	{
		ReadExtVerInfo();

		// 模块硬件版本信息
		memcpy_swap(&sendAppGeneralPacket->Data[index], (u8 *)StaExtVersion.ModuleHardVer, 2);
		index += 2;
		sendAppGeneralPacket->Data[index++] = dec_to_bcd(StaExtVersion.ModuleHardVerDate.Day);
		sendAppGeneralPacket->Data[index++] = dec_to_bcd(StaExtVersion.ModuleHardVerDate.Month);
		sendAppGeneralPacket->Data[index++] = dec_to_bcd(StaExtVersion.ModuleHardVerDate.Year);

		// 芯片硬件版本信息
		memcpy_swap(&sendAppGeneralPacket->Data[index], (u8 *)StaExtVersion.ChipHardVer, 2);
		index += 2;
		sendAppGeneralPacket->Data[index++] = dec_to_bcd(StaExtVersion.ChipHardVerDate.Day);
		sendAppGeneralPacket->Data[index++] = dec_to_bcd(StaExtVersion.ChipHardVerDate.Month);
		sendAppGeneralPacket->Data[index++] = dec_to_bcd(StaExtVersion.ChipHardVerDate.Year);

		// 芯片软件版本信息
		memcpy_swap(&sendAppGeneralPacket->Data[index], (u8 *)StaExtVersion.ChipSoftVer, 2);
		index += 2;
		sendAppGeneralPacket->Data[index++] = dec_to_bcd(StaExtVersion.ChipSofVerDate.Day);
		sendAppGeneralPacket->Data[index++] = dec_to_bcd(StaExtVersion.ChipSofVerDate.Month);
		sendAppGeneralPacket->Data[index++] = dec_to_bcd(StaExtVersion.ChipSofVerDate.Year);

		// 应用程序版本号
		memcpy_swap(&sendAppGeneralPacket->Data[index], (u8 *)StaExtVersion.AppVer, 2);
		index += 2;

		break;
	}
	case APP_BUS_EXT_FACTORY_SET_EXT_VER:
	{
		// 模块硬件版本信息
		memcpy_swap((u8 *)StaExtVersion.ModuleHardVer, &appGeneral->Data[index], 2);
		index += 2;
		StaExtVersion.ModuleHardVerDate.Day = bcd_to_dec(appGeneral->Data[index++]);
		StaExtVersion.ModuleHardVerDate.Month = bcd_to_dec(appGeneral->Data[index++]);
		StaExtVersion.ModuleHardVerDate.Year = bcd_to_dec(appGeneral->Data[index++]);

		// 芯片硬件版本信息
		memcpy_swap((u8 *)StaExtVersion.ChipHardVer, &appGeneral->Data[index], 2);
		index += 2;
		StaExtVersion.ChipHardVerDate.Day = bcd_to_dec(appGeneral->Data[index++]);
		StaExtVersion.ChipHardVerDate.Month = bcd_to_dec(appGeneral->Data[index++]);
		StaExtVersion.ChipHardVerDate.Year = bcd_to_dec(appGeneral->Data[index++]);

		// 芯片软件版本信息
		memcpy_swap((u8 *)StaExtVersion.ChipSoftVer, &appGeneral->Data[index], 2);
		index += 2;
		StaExtVersion.ChipSofVerDate.Day = bcd_to_dec(appGeneral->Data[index++]);
		StaExtVersion.ChipSofVerDate.Month = bcd_to_dec(appGeneral->Data[index++]);
		StaExtVersion.ChipSofVerDate.Year = bcd_to_dec(appGeneral->Data[index++]);

		// 应用程序版本号
		memcpy_swap((u8 *)StaExtVersion.AppVer, &appGeneral->Data[index], 2);
		index += 2;

		StorageExtVerInfo();

		memcpy(&sendAppGeneralPacket->Data[6], &appGeneral->Data[6], up_data_len - 6);

		break;
	}
	default:
		FreeSendPDU(pdu);
		return;
	}

	if (up_data_len != 0)
	{
		sendAppGeneralPacket->AppFrameLen = up_data_len;
		pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + up_data_len;
		MacHandleApp(pdu->pdu, pdu->size, 2, macHead->SrcTei, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);
		FreeSendPDU(pdu);
	}
}
#ifdef PROTOCOL_NW_2020
typedef struct
{
	u32 DI;
	u8 Len;
	u8 BlockNum; // 数据块数量
} DI_Len_s;

// 数据标识对应数据回复长度
const DI_Len_s DI_Len_Tbl[] =
	{
		{0x0201FF00, 2, 3},	 // 电压
		{0x0202FF00, 3, 3},	 // 电流
		{0x0203FF00, 3, 4},	 // 瞬时有功功率
		{0x0204FF00, 3, 4},	 // 瞬时无功功率
		{0x0205FF00, 3, 4},	 // 瞬时视在功率
		{0x0206FF00, 2, 4},	 // 功率因素
		{0x0207FF00, 2, 3},	 // 相角
		{0x0208FF00, 2, 3},	 // 电压波形失真度
		{0x0209FF00, 2, 3},	 // 电流波形失真度
		{0x020A01FF, 2, 21}, // A相电压谐波含量
		{0x020A02FF, 2, 21}, // B相电压谐波含量
		{0x020A03FF, 2, 21}, // C相电压谐波含量
		{0x020B01FF, 2, 21}, // A相电流谐波含量
		{0x020B02FF, 2, 21}, // B相电流谐波含量
		{0x020B03FF, 2, 21}, // C相电流谐波含量
		{0x02800101, 3, 1},	 // 零线电流
		{0x02800102, 2, 1},	 // 电网频率
		{0x02800103, 3, 1},	 // 一分钟有功总平均功率
		{0x02800104, 3, 1},	 // 当前有功需量
		{0x02800105, 3, 1},	 // 当前无功需量
		{0x02800106, 3, 1},	 // 当前视在需量
		{0x02800107, 2, 1},	 // 表内温度
		{0x02800108, 2, 1},	 // 时钟电池电压
		{0x02800109, 2, 1},	 // 停电超标电池电压
		{0x0280010A, 4, 1},	 // 内部电池工作时间
};

typedef struct
{
	u32 DI_CSG21;
	u32 DI;
	u8 Len;
	u8 DI_Num;
} CSG21_DI_Len_s;

// 南网21标准曲线数据标识对那个电表抄读数据标识
const CSG21_DI_Len_s CSG21_DI_Len_Tbl[] =
	{
		{0x06120101, 0x02010100, 2, 1}, // A相电压
		{0x06120102, 0x02010200, 2, 1}, // B相电压
		{0x06120103, 0x02010300, 2, 1}, // C相电压
		{0x061201FF, 0x0201FF00, 6, 1}, // 电压数据块

		{0x06120201, 0x02020100, 3, 1}, // A相电流
		{0x06120202, 0x02020200, 3, 1}, // B相电流
		{0x06120203, 0x02020300, 3, 1}, // C相电流
		{0x061202FF, 0x0202FF00, 9, 1}, // 电流数据块

		{0x06120300, 0x02030000, 3, 1},	 // 总有功功率
		{0x06120301, 0x02030100, 3, 1},	 // A相有功功率
		{0x06120302, 0x02030200, 3, 1},	 // B相有功功率
		{0x06120303, 0x02030300, 3, 1},	 // C相有功功率
		{0x061203FF, 0x0203FF00, 12, 1}, // 有功功率数据块

		{0x06120400, 0x02040000, 3, 1},	 // 总无功功率
		{0x06120401, 0x02040100, 3, 1},	 // A相无功功率
		{0x06120402, 0x02040200, 3, 1},	 // B相无功功率
		{0x06120403, 0x02040300, 3, 1},	 // C相无功功率
		{0x061204FF, 0x0204FF00, 12, 1}, // 无功功率数据块

		{0x06120500, 0x02060000, 2, 1}, // 总功率因数
		{0x06120501, 0x02060100, 2, 1}, // A相功率因数
		{0x06120502, 0x02060200, 2, 1}, // B相功率因数
		{0x06120503, 0x02060300, 2, 1}, // C相功率因数
		{0x061205FF, 0x0206FF00, 8, 1}, // 功率因数数据块

		{0x06120601, 0x00010000, 4, 1}, // 正向有功总电能
		{0x06120602, 0x00020000, 4, 1}, // 反向有功总电能
		{0x06120603, 0x00030000, 4, 1}, // 组合无功1总电能
		{0x06120604, 0x00040000, 4, 1}, // 组合无功2总电能
		{0x061206FF, 0x00010000, 4, 4}, // 总电能曲线数据块(由STA模块实现数据块)
		{0x061206FF, 0x00020000, 4, 4}, // 总电能曲线数据块(由STA模块实现数据块)
		{0x061206FF, 0x00030000, 4, 4}, // 总电能曲线数据块(由STA模块实现数据块)
		{0x061206FF, 0x00040000, 4, 4}, // 总电能曲线数据块(由STA模块实现数据块)

		{0x06120701, 0x00050000, 4, 1}, // 第一像限无功总电能
		{0x06120702, 0x00060000, 4, 1}, // 第二像限无功总电能
		{0x06120703, 0x00070000, 4, 1}, // 第三像限无功总电能
		{0x06120704, 0x00080000, 4, 1}, // 第四像限无功总电能
		{0x061207FF, 0x00050000, 4, 4}, // 象限无功曲线数据块(由STA模块实现数据块)
		{0x061207FF, 0x00060000, 4, 4}, // 象限无功曲线线数据块(由STA模块实现数据块)
		{0x061207FF, 0x00070000, 4, 4}, // 象限无功曲线线数据块(由STA模块实现数据块)
		{0x061207FF, 0x00080000, 4, 4}, // 象限无功曲线线数据块(由STA模块实现数据块)

		{0x06120801, 0x02800004, 3, 1}, // 当前有功需量
		{0x06120802, 0x02800005, 3, 1}, // 当前无功需量
		{0x061208FF, 0x02800004, 3, 2}, // 当前需量曲线数据块(由STA模块实现数据块)
		{0x061208FF, 0x02800005, 3, 2}, // 当前需量曲线数据块(由STA模块实现数据块)

#ifdef PROTOCOL_NW_EXT_GRAPH
		{0x06120900, 0x02800001, 3, 1}, // 零线电流
#endif
};

static bool Find_DI_Len(u8 is_CSG21, u32 DI, u16 *Len, u8 *Meter_DI_Num, u32 *Meter_DI)
{
	if (is_CSG21) // 南网21标准，匹配DI映射的电表抄读DI
	{
		for (int i = 0; i < sizeof(CSG21_DI_Len_Tbl) / sizeof(CSG21_DI_Len_Tbl[0]); i++)
		{
			if (DI == CSG21_DI_Len_Tbl[i].DI_CSG21)
			{
				if (((Len != NULL)) && (Meter_DI_Num != NULL) && (Meter_DI != NULL))
				{
					*Len += CSG21_DI_Len_Tbl[i].Len;
					Meter_DI[*Meter_DI_Num] = CSG21_DI_Len_Tbl[i].DI;
					*Meter_DI_Num += 1;

					// 匹配所有电表抄读DI，部分曲线数据块由STA实现
					if (*Meter_DI_Num == CSG21_DI_Len_Tbl[i].DI_Num)
					{
						return true;
					}
				}
				else // Len, Meter_DI_Num, Meter_DI为NULL，单纯匹配判断是否南网21标准
				{
					return true;
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < sizeof(DI_Len_Tbl) / sizeof(DI_Len_Tbl[0]); i++)
		{
			if ((DI & 0xFFFF00FF) == (DI_Len_Tbl[i].DI & 0xFFFF00FF))
			{
				if (Len != NULL)
				{
					if ((DI & 0xFFFFFFFF) == DI_Len_Tbl[i].DI)
					{
						*Len = DI_Len_Tbl[i].Len * DI_Len_Tbl[i].BlockNum;
					}
					else
					{
						*Len = DI_Len_Tbl[i].Len;
					}
				}

				// Len为NULL，单纯匹配判断是否南网老规范
				return true;
			}
			else if ((DI & 0xFFFFFF00) == (DI_Len_Tbl[i].DI & 0xFFFFFF00))
			{
				if (Len != NULL)
				{
					if ((DI & 0xFFFFFFFF) == DI_Len_Tbl[i].DI)
					{
						*Len = DI_Len_Tbl[i].Len * DI_Len_Tbl[i].BlockNum;
					}
					else
					{
						*Len = DI_Len_Tbl[i].Len;
					}
				}

				// Len为NULL，单纯匹配判断是否南网老规范
				return true;
			}
		}
	}

	return false;
}

static bool Find_CSG21_DI_TotalLen(GRAPH_ITEM_READ_CSG21_DOWN *pDownCSG21Data, u16 *ToatlLen)
{
	u8 DICount = 0;
	u16 DILen = 0;
	u8 meter_DI_num = 0;
	u32 meter_DI[4] = {0};

	debug_str(DEBUG_LOG_APP, "Find_CSG21_DI_TotalLen, DINum:%d\r\n", pDownCSG21Data->DINum);
	for (int i = 0; i < pDownCSG21Data->DINum; i++)
	{
		meter_DI_num = 0;
		DILen = 0;
		if (!Find_DI_Len(1, pDownCSG21Data->item[i].ident, &DILen, &meter_DI_num, meter_DI))
		{
			return false;
		}

		DICount++;
		*ToatlLen += DILen;

		debug_str(DEBUG_LOG_APP, "Find_CSG21_DI_TotalLen, DI:0x%08x, Len:%d\r\n", pDownCSG21Data->item[i].ident, DILen);
	}

	debug_str(DEBUG_LOG_APP, "Find_CSG21_DI_TotalLen, Real DINum:%d, ToatlLen:%d\r\n", DICount, *ToatlLen);
	if (pDownCSG21Data->DINum != DICount)
	{
		return false;
	}

	return true;
}

bool Find_ID_Meter(u32 DI, u8 *idxa, u8 *idxb)
{
	u8 meter_sn;

	if (myDevicType == METER)
	{
		meter_sn = 0;
	}
	else if (myDevicType == METER_3PHASE)
	{
		meter_sn = 1;
	}
	else
	{
		return false;
	}

	*idxa = meter_sn;
	for (int i = 0; i < Graph_Config.num[meter_sn]; i++)
	{
		if (Graph_Config.ident[meter_sn][i] == DI)
		{
			*idxb = i;
			return true;
		}
	}

	return false;
}

static u16 GetGraphFrame(u8 is_CSG21, u8 *data, u32 DI, u16 num, u8 date[5], u16 total_len, u8 period)
{
	u32 sec;
	u16 index = 0;
	u8 idxa, idxb;
	u16 store_data_len = 0; // 存储的数据长度
	u8 item_min_len = 0;	// 数据项最小长度
	u16 CSG21_DI_len = 0;
	u8 *pcount = NULL;
	u8 real_count = 0;
	u32 meter_DI[4] = {0}; // 电表抄读DI; 如果由STA实现的曲线数据块，则DI会对应多个电表抄读DI(目前南网21标准里定义最多4个)
	u8 meter_DI_num = 0;
	u8 collect_period = 0;

	if (is_CSG21) // 南网21标准
	{
		item_min_len = 4;
		collect_period = period;
	}
	else
	{
		item_min_len = 10; // 老规范多了采集点数量和起始时间点
	}

	if (total_len <= item_min_len)
	{
		return 0;
	}

	data[index++] = (DI & 0xFF);
	data[index++] = ((DI >> 8) & 0xFF);
	data[index++] = ((DI >> 16) & 0xFF);
	data[index++] = ((DI >> 24) & 0xFF);

	debug_str(DEBUG_LOG_APP, "GetGraphFrame time now value %d-%d-%d %d:%d, valid:%d, is_CSG21:%d\r\n",
			  Time_Module[0].time.tm_year + 1900, Time_Module[0].time.tm_mon + 1, Time_Module[0].time.tm_mday, Time_Module[0].time.tm_hour,
			  Time_Module[0].time.tm_min, Time_Module[0].isVaild, is_CSG21);
	debug_str(DEBUG_LOG_APP, "GetGraphFrame ident 0x%08x, time value %x-%x-%x %x:%x, num:%d, period:%d\r\n",
			  DI, date[4], date[3], date[2], date[1], date[0], num, period);

	Time_Special time;
	memset(&time, 0, sizeof(time));
	time.tm_sec = 0;
	time.tm_min = bcd_to_dec(date[0]);
	time.tm_hour = bcd_to_dec(date[1]);
	time.tm_mday = bcd_to_dec(date[2]);
	time.tm_mon = bcd_to_dec(date[3]) - 1;
	time.tm_year = 2000 + bcd_to_dec(date[4]) - 1900;
	time.tm_isdst = 0;

	if (is_CSG21) // 南网21标准，DI需要转换为电表抄读DI
	{
		if (!Find_DI_Len(1, DI, &CSG21_DI_len, &meter_DI_num, meter_DI))
		{
			return 0;
		}
	}
	else // 南网老规范每个数据标识都需要填充起始点时间
	{
		meter_DI[0] = DI;
		meter_DI_num = 1;

		for (int i = 0; i < 5; i++) // 起始点时间，分-->年
		{
			data[index++] = date[i];
		}

		pcount = &data[index++]; // 采集点数量
		*pcount = 0;
	}

	debug_str(DEBUG_LOG_APP, "GetGraphFrame meterDINum:%d\r\n", meter_DI_num);

	total_len -= item_min_len;

	for (int i = 0; i < num; i++)
	{
		sec = Data2Sec(&time);

		for (int k = 0; k < meter_DI_num; k++)
		{
			if (Find_ID_Meter(meter_DI[k], &idxa, &idxb))
			{
				if (!is_CSG21) // 南网21标准
				{
					collect_period = Graph_Config.period[idxa][idxb];
				}

				Graph_Data_s *pdata = NULL;
				/*
				u32 currentSec= 0;

				if (Time_Module[0].isVaild>2)//事件有效
				{
					Time_Special temp=Time_Module[0].time;
					currentSec = Data2Sec(&temp);
					if (currentSec>=sec)//要找的时间小于当前时间  数据才有效
					{
						pdata=FindStaGraphData(sec, meter_DI[k]);
					}
				}
				else
				*/
				{
					pdata = FindStaGraphData(sec, meter_DI[k]);
				}
				if (pdata != NULL)
				{
					if (store_data_len == 0)
					{
						store_data_len = pdata->len;
						debug_str(DEBUG_LOG_APP, "GetGraphFrame data len:%d\r\n", store_data_len);

						if (i != 0) // 获取到存储的数据长度时，之前的没有获取到的数据填充0xFF
						{
							if (total_len >= store_data_len * meter_DI_num * i)
							{
								memset(&data[index], 0xFF, store_data_len * meter_DI_num * i);
								index += store_data_len * meter_DI_num * i;
								total_len -= store_data_len * meter_DI_num * i;
								real_count = i;
							}
							else
							{
								u16 real_cnt = total_len / (store_data_len * meter_DI_num);
								memset(&data[index], 0xFF, store_data_len * meter_DI_num * real_cnt);
								index += store_data_len * meter_DI_num * real_cnt;
								total_len = 0;
								real_count = real_cnt;
								break;
							}
						}
					}
					if (total_len >= pdata->len)
					{
						for (int j = 0; j < pdata->len; j++)
						{
							data[index++] = pdata->data[j];
						}
						total_len -= pdata->len;
						real_count++;
					}
					else
					{
						total_len = 0;
						break;
					}
				}
				else
				{
					if (store_data_len) // 还没获取到存储的数据长度时，先不填充0xFF
					{
						if (total_len >= store_data_len)
						{
							memset(&data[index], 0xFF, store_data_len);
							index += store_data_len;
							total_len -= store_data_len;
							real_count++;
						}
						else
						{
							store_data_len = 0;
							break;
						}
					}
				}

				if (k == meter_DI_num - 1) // meter_DI最后一个DI才更新time；兼容南网21标准，由STA实现的曲线数据块，1个时间点需要存储多个对应的电表抄读DI数据
				{
					if (time.tm_min % collect_period)
					{
						time.tm_min = time.tm_min / collect_period * collect_period;
					}

					DateSpecialChangeSec(&time, 60 * collect_period);

					// 针对不能被60分钟整除的采集间隔，每小时开始的0分钟采集时刻需要调整
					if (time.tm_min % collect_period)
					{
						time.tm_min = time.tm_min / collect_period * collect_period;
					}
				}
			}
		}
	}

	// 没有获取到数据或者配置项不存在，回复配置项对应的长度FF
	if (store_data_len == 0)
	{
		u16 real_cnt = 0;

		if (is_CSG21) // 南网21标准，之前已经获取过电表抄读DI对应的长度
		{
			store_data_len = CSG21_DI_len;
		}
		else
		{
			Find_DI_Len(0, meter_DI[0], &store_data_len, NULL, NULL);
		}

		if (store_data_len != 0)
		{
			real_cnt = total_len / store_data_len;
			if (real_cnt > num)
			{
				real_cnt = num;
			}
			memset(&data[index], 0xFF, store_data_len * real_cnt);
			index += store_data_len * real_cnt;
			real_count = real_cnt;
		}
	}

	if (!is_CSG21)
	{
		*pcount = real_count;
	}

	return index;
}

void AppGraphReadHandle(u8 srcAddr[6], u8 dstAddr[6], u16 srcTei, u16 seq, u8 sendType, u8 *data, u16 len, u8 is_CSG21)
{
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	GRAPH_ITEM_READ_CSG21_DOWN *pDownCSG21Data = (GRAPH_ITEM_READ_CSG21_DOWN *)&data[1];
	GRAPH_ITEM_READ_DOWN *pDownData = NULL;
	u16 data_len = 0, data_index = 0;
	u16 total_DI_len = 0, total_len;
	u8 real_count = 0;

	if (pDownCSG21Data->meter_type > 1) // 表类型
	{
		debug_str(DEBUG_LOG_APP, "AppGraphReadHandle meter type:%d error!!!!!\r\n", pDownCSG21Data->meter_type);
		return;
	}

#ifndef PROTOCOL_NW_2021 // 南网21标准不判断电表类型，单三相采集的数据标识没有差异，且规避集中器测量点表类型错误的问题
	if (myDevicType == METER)
	{
		if (pDownCSG21Data->meter_type != 0)
		{
			debug_str(DEBUG_LOG_APP, "AppGraphReadHandle meter type:%d match error!!!!!\r\n", pDownCSG21Data->meter_type);
			return;
		}
	}
	else if (myDevicType == METER_3PHASE)
	{
		if (pDownCSG21Data->meter_type != 1)
		{
			debug_str(DEBUG_LOG_APP, "AppGraphReadHandle meter type:%d match error!!!!!\r\n", pDownCSG21Data->meter_type);
			return;
		}
	}
#endif

	if (is_CSG21) // 南网21标准曲线抄读
	{
		if (!Find_DI_Len(1, pDownCSG21Data->item[0].ident, NULL, NULL, NULL)) // 南网21标准匹配第一个数据标识
		{
			debug_str(DEBUG_LOG_APP, "AppGraphReadHandle CSG21 format error, first ident:0x%08x\r\n", pDownCSG21Data->item[0].ident);
			return;
		}

		if (!Find_CSG21_DI_TotalLen(pDownCSG21Data, &total_DI_len))
		{
			debug_str(DEBUG_LOG_APP, "AppGraphReadHandle CSG21 format error, all DI match\r\n");
			return;
		}

		debug_str(DEBUG_LOG_APP, "AppGraphReadHandle CSG21, DI num:%d, count:%d, period:%d\r\n", pDownCSG21Data->DINum, pDownCSG21Data->count, pDownCSG21Data->period);
	}
	else
	{
		pDownData = (GRAPH_ITEM_READ_DOWN *)&data[1];

		if (!Find_DI_Len(0, pDownData->item[0].ident, NULL, NULL, NULL)) // 南网深化应用老规范匹配第一个数据标识
		{
			debug_str(DEBUG_LOG_APP, "AppGraphReadHandle CSGOld format error, first ident:0x%08x\r\n", pDownData->item[0].ident);
			return;
		}
	}

	SEND_PDU_BLOCK *pdu = MallocSendPDU();
	if (pdu == NULL)
		return;

	// 应用层报文头
	APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
	// 应用层业务报文头
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];
	// 数据转发业务报文
	APP_READ_MODULE_UP *sendMeterUp = (APP_READ_MODULE_UP *)&sendAppGeneralPacket->Data[0];

	// 生成应用层报文头
	CreateAppPacket(sendAppPacket);
	sendAppPacket->Port = 0x13;

	// 生成应用层业务报文头
	CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_TRAN, 0, 0, 0, 1, APP_BUS_TRAN_MODULE, seq, 0, 0);
	// 生成应用层业务数据
	if (sendType == SENDTYPE_SINGLE)
	{
		memcpy(sendMeterUp->DstMac, srcAddr, 6);
		memcpy(sendMeterUp->SrcMac, dstAddr, 6);
	}
	else
	{
		memcpy(sendMeterUp->SrcMac, pNetBasePara->Mac, 6);
		TransposeAddr(sendMeterUp->SrcMac);
		memcpy(sendMeterUp->SrcMac, dstAddr, 6);
	}

	sendMeterUp->Reserve = 0;
	sendMeterUp->AppCode = APP_SHENHUA_APP_CODE_GRAPH;
	sendMeterUp->Data[data_len++] = is_CSG21 ? APP_GRAPH_FUNC_CODE_READ_21PROTOCOL : APP_GRAPH_FUNC_CODE_READ; // 功能码，抄读数据项
	sendMeterUp->Data[data_len++] = pDownCSG21Data->meter_type;												   // 表类型

	u16 remain_len = 1500;
	if (is_CSG21) // 南网21标准
	{
		total_len = total_DI_len * pDownCSG21Data->count + 4 * pDownCSG21Data->DINum; // 4字节数据标识

		if (total_len > remain_len) // 预计回复总长度超过限制，重新计算采集点数量
		{
			real_count = (remain_len - 4 * pDownCSG21Data->DINum) / total_DI_len;
		}
		else
		{
			real_count = pDownCSG21Data->count;
		}

		memcpy(&sendMeterUp->Data[data_len], pDownCSG21Data->time_data, sizeof(pDownCSG21Data->time_data)); // 起始时间点
		data_len += 5;
		sendMeterUp->Data[data_len++] = real_count;				// 采集点数量
		sendMeterUp->Data[data_len++] = pDownCSG21Data->period; // 采集间隔
		sendMeterUp->Data[data_len++] = pDownCSG21Data->DINum;	// 数据项数量

		for (u8 i = 0; i < pDownCSG21Data->DINum; i++)
		{
			data_index = GetGraphFrame(1, &sendMeterUp->Data[data_len], pDownCSG21Data->item[i].ident, real_count, pDownCSG21Data->time_data, remain_len, pDownCSG21Data->period);
			remain_len -= data_index;
			data_len += data_index;
			if (data_index == 0)
			{
				break;
			}
		}
	}
	else
	{
		sendMeterUp->Data[data_len++] = pDownData->num; // 数据项数量

		for (u8 i = 0; i < pDownData->num; i++)
		{
			data_index = GetGraphFrame(0, &sendMeterUp->Data[data_len], pDownData->item[i].ident, pDownData->item[i].count, pDownData->item[i].time_data, remain_len, 0);
			remain_len -= data_index;
			data_len += data_index;
			if (data_index == 0)
			{
				break;
			}
		}
	}

	sendMeterUp->DataLen = data_len;
	sendAppGeneralPacket->AppFrameLen = sizeof(APP_READ_MODULE_UP) - 1 + data_len;
	pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
	if (sendType == SENDTYPE_SINGLE)
		MacHandleApp(pdu->pdu, pdu->size, 3, srcTei, SENDTYPE_SINGLE, BROADCAST_DRT_UNKNOW, 0);
	else
		MacHandleApp(pdu->pdu, pdu->size, 3, MAC_BROADCAST_TEI, SENDTYPE_BROADCAST, BROADCAST_DRT_UP, 0);
	FreeSendPDU(pdu);
}

void AppGraphConfigHandle(u8 srcAddr[6], u8 dstAddr[6], u16 srcTei, u16 seq, u8 sendType, u8 *data, u16 len)
{
#if 1
	u8 meter_sn = 0, i = 0, j = 0;
#else
	GRAPH_ITEM *pDownData = (GRAPH_ITEM *)&data[2];
	u8 meter_sn = 0, i = 0, j = 0, k = 0;
	u16 real_len = 2;
#endif

	if ((len == 2) && data[1]) // 南网21标准，只配置采集间隔，填充默认采集数据项
	{
		u8 period = data[1];
		u8 default_DI_num = sizeof(CSG21_DI_Len_Tbl) / sizeof(CSG21_DI_Len_Tbl[0]);
		meter_sn = (myDevicType == METER) ? 0 : 1;

		debug_str(DEBUG_LOG_APP, "AppGraphConfigHandle CSG21, period %d, last config:%d, last DI num:%d\r\n",
				  period, Graph_Config.config_flag, Graph_Config.num[meter_sn]);

		if (!period)
		{
			return;
		}

		for (i = 0; i < default_DI_num; i++)
		{
			if (CSG21_DI_Len_Tbl[i].DI_Num == 1) // DI_Num不为1的数据标识不需要配置（由STA模块实现数据块）
			{
				j++;
			}
		}

		if (Graph_Config.config_flag && (Graph_Config.num[meter_sn] != j))
		{
			EraseGraph(); // 删除所有曲线数据
			memset(&Graph_Config, 0, sizeof(Graph_Config));
		}

		// 周期不同
		// if (Graph_Config.period[meter_sn][0] != period)
		{
			Graph_Config.config_flag = period ? 1 : 0;
			Graph_Config.num[meter_sn] = default_DI_num;

			j = 0;
			for (i = 0; i < default_DI_num; i++)
			{
				if (CSG21_DI_Len_Tbl[i].DI_Num == 1) // DI_Num不为1的数据标识不需要配置（由STA模块实现数据块）
				{
					Graph_Config.mode[meter_sn][j] = period ? 1 : 0;
					Graph_Config.period[meter_sn][j] = period;
					Graph_Config.ident[meter_sn][j] = CSG21_DI_Len_Tbl[i].DI;

					j++;
				}
			}

			Graph_Config.num[meter_sn] = j;
		}
	}
	else
	{
#if 1
		return;
#else
		Graph_Config.config_flag = data[1];
		for (i = 0; i < 2; i++)
		{
			if (Graph_Config.config_flag == 0) // 全部关闭
			{
				for (j = 0; j < MAC_GRAPH_CONFIG_NUM; j++)
				{
					Graph_Config.mode[i][j] = 0;
				}
			}
			else
			{
				real_len += 1;					// 表类型
				if (pDownData->meter_type == 0) // 单项表
				{
					debug_str(DEBUG_LOG_APP, "AppGraphConfigHandle config meter\r\n");
					meter_sn = 0;
				}
				else if (pDownData->meter_type == 1) // 三相表
				{
					debug_str(DEBUG_LOG_APP, "AppGraphConfigHandle config 3phase meter\r\n");
					meter_sn = 1;
				}
				else
				{
					debug_str(DEBUG_LOG_APP, "AppGraphConfigHandle config error!!!!!\r\n");
					Graph_Config.config_flag = 0;
					break;
				}

				if (pDownData->num > MAC_GRAPH_CONFIG_NUM)
				{
					return;
				}

				real_len += 1; // 数据项数量

				for (j = 0; j < pDownData->num; j++)
				{
					// 查找已存在项
					for (k = 0; k < MAC_GRAPH_CONFIG_NUM; k++)
					{
						if (Graph_Config.ident[meter_sn][k] == pDownData->item[j].ident)
						{
							if (pDownData->item[j].period)
							{
								Graph_Config.period[meter_sn][k] = pDownData->item[j].period;
								Graph_Config.mode[meter_sn][k] = 1; // 开启
							}
							else
							{
								Graph_Config.mode[meter_sn][k] = 0; // 关闭
							}
							debug_str(DEBUG_LOG_APP, "AppGraphConfigHandle period %d-%d\r\n", j, Graph_Config.period[meter_sn][k]);

							break;
						}
					}

					// 未找到已存在项
					if (k == MAC_GRAPH_CONFIG_NUM)
					{
						for (k = 0; k < MAC_GRAPH_CONFIG_NUM; k++)
						{
							if (Graph_Config.period[meter_sn][k] == 0)
							{
								Graph_Config.ident[meter_sn][k] = pDownData->item[j].ident;
								debug_str(DEBUG_LOG_APP, "AppGraphConfigHandle ident %d-0x%08x\r\n", meter_sn, Graph_Config.ident[meter_sn][k]);
								Graph_Config.period[meter_sn][k] = pDownData->item[j].period;
								if (pDownData->item[j].period)
								{
									Graph_Config.period[meter_sn][k] = pDownData->item[j].period;
									Graph_Config.mode[meter_sn][k] = 1; // 开启
								}
								else
								{
									Graph_Config.mode[meter_sn][k] = 0; // 关闭
								}
								debug_str(DEBUG_LOG_APP, "AppGraphConfigHandle period %d-%d\r\n", meter_sn, Graph_Config.period[meter_sn][k]);

								Graph_Config.num[meter_sn] += 1; // 增加数据项个数

								break;
							}
						}
					}

					real_len += 5;
				}

				if (real_len >= len)
				{
					break;
				}

				pDownData = (GRAPH_ITEM *)((u32)pDownData + 2 + 5 * pDownData->num);
			}
		}
#endif
	}

	StorageGraphPara();

	// 单播回复确认帧
	if (sendType == SENDTYPE_SINGLE)
	{
		SEND_PDU_BLOCK *pdu = MallocSendPDU();
		if (pdu == NULL)
			return;

		// 应用层报文头
		APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
		// 应用层业务报文头
		APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];

		// 生成应用层报文头
		CreateAppPacket(sendAppPacket);
		sendAppPacket->Port = 0x13;
		// 生成应用层业务报文头
		CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_CONFIRM, 0, 0, 0, 1, APP_BUS_CONFIRM, seq, 0, 0);

		pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1;
		MacHandleApp(pdu->pdu, pdu->size, 2, CCO_TEI, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);

		FreeSendPDU(pdu);
	}
}

void AppGraphQueryHandle(u8 srcAddr[6], u8 dstAddr[6], u16 srcTei, u16 seq, u8 sendType, u8 *data, u16 len)
{
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	GRAPH_ITEM_QUERY_DOWN *pDownData = (GRAPH_ITEM_QUERY_DOWN *)&data[1];
	u16 data_len = 0;

	if (pDownData->meter_type > 1) // 表类型
	{
		debug_str(DEBUG_LOG_APP, "AppGraphQueryHandle meter type:%d error!!!!!\r\n", pDownData->meter_type);
		return;
	}

	if (myDevicType == METER)
	{
		if (pDownData->meter_type != 0)
		{
			debug_str(DEBUG_LOG_APP, "AppGraphQueryHandle meter type:%d match error!!!!!\r\n", pDownData->meter_type);
			return;
		}
	}
	else if (myDevicType == METER_3PHASE)
	{
		if (pDownData->meter_type != 1)
		{
			debug_str(DEBUG_LOG_APP, "AppGraphQueryHandle meter type:%d match error!!!!!\r\n", pDownData->meter_type);
			return;
		}
	}

	SEND_PDU_BLOCK *pdu = MallocSendPDU();
	if (pdu == NULL)
		return;

	// 应用层报文头
	APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
	// 应用层业务报文头
	APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];
	// 数据转发业务报文
	APP_READ_MODULE_UP *sendMeterUp = (APP_READ_MODULE_UP *)&sendAppGeneralPacket->Data[0];

	// 生成应用层报文头
	CreateAppPacket(sendAppPacket);
	sendAppPacket->Port = 0x13;
	// 生成应用层业务报文头
	CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_TRAN, 0, 0, 0, 1, APP_BUS_TRAN_MODULE, seq, 0, 0);
	// 生成应用层业务数据
	if (sendType == SENDTYPE_SINGLE)
	{
		memcpy(sendMeterUp->DstMac, srcAddr, 6);
		memcpy(sendMeterUp->SrcMac, dstAddr, 6);
	}
	else
	{
		memcpy(sendMeterUp->SrcMac, pNetBasePara->Mac, 6);
		TransposeAddr(sendMeterUp->SrcMac);
		memcpy(sendMeterUp->SrcMac, dstAddr, 6);
	}

	sendMeterUp->Reserve = 0;
	sendMeterUp->AppCode = APP_SHENHUA_APP_CODE_GRAPH;
	sendMeterUp->Data[0] = APP_GRAPH_FUNC_CODE_QUERY; // 功能码，查询采集间隔

	GRAPH_ITEM *pUpData = (GRAPH_ITEM *)&sendMeterUp->Data[1];
	u8 index = pDownData->meter_type;

	pUpData->meter_type = pDownData->meter_type;
	pUpData->num = pDownData->num;
	for (int i = 0; i < pDownData->num; i++)
	{
		pUpData->item[i].ident = pDownData->item[i].ident;
		pUpData->item[i].period = 0;
		for (int j = 0; j < Graph_Config.num[index]; j++)
		{
			if (pDownData->item[i].ident == Graph_Config.ident[index][j])
			{
				if (Graph_Config.mode[index][j])
				{
					pUpData->item[i].period = Graph_Config.period[index][j];
				}
				else
				{
					pUpData->item[i].period = 0;
				}
				break;
			}
		}
	}

	data_len = 3 + pUpData->num * 5;
	sendMeterUp->DataLen = data_len;
	sendAppGeneralPacket->AppFrameLen = sizeof(APP_READ_MODULE_UP) - 1 + data_len;
	pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
	if (sendType == SENDTYPE_SINGLE)
		MacHandleApp(pdu->pdu, pdu->size, 3, srcTei, SENDTYPE_SINGLE, BROADCAST_DRT_UNKNOW, 0);
	else
		MacHandleApp(pdu->pdu, pdu->size, 3, MAC_BROADCAST_TEI, SENDTYPE_BROADCAST, BROADCAST_DRT_UP, 0);
	FreeSendPDU(pdu);
}

// 模块曲线配置查询抄读
bool AppGraphHandle(u8 srcAddr[6], u8 dstAddr[6], u16 srcTei, u16 seq, u8 sendType, u8 *data, u16 len)
{
#if 1
	if (data[0] == APP_GRAPH_FUNC_CODE_CONFIG) // 功能码：配置采集间隔
	{
		AppGraphConfigHandle(srcAddr, dstAddr, srcTei, seq, sendType, data, len);

		return true;
	}
	else if (data[0] == APP_GRAPH_FUNC_CODE_QUERY) // 功能码：查询采集间隔或者南网21标准抄读曲线
	{
		if (len >= sizeof(GRAPH_ITEM_READ_CSG21_DOWN) + 5) // 至少1个数据标识(4字节)+功能码(1字节)
		{
			GRAPH_ITEM_READ_CSG21_DOWN *pDownCSG21Data = (GRAPH_ITEM_READ_CSG21_DOWN *)&data[1];

			if (!pDownCSG21Data->period)
			{
				return false;
			}

			if (Find_DI_Len(1, pDownCSG21Data->item[0].ident, NULL, NULL, NULL)) // 南网21标准匹配第一个数据标识
			{
				AppGraphReadHandle(srcAddr, dstAddr, srcTei, seq, sendType, data, len, 1);

				return true;
			}
		}

		AppGraphQueryHandle(srcAddr, dstAddr, srcTei, seq, sendType, data, len);

		return true;
	}
	else if (data[0] == APP_GRAPH_FUNC_CODE_READ) // 功能码：南网深化应用老规范抄读曲线
	{
		AppGraphReadHandle(srcAddr, dstAddr, srcTei, seq, sendType, data, len, 0);

		return true;
	}

	return false;
#else
	if (!Is645Frame(data, len))
	{
		return false;
	}
	P_Dl645_Freme p_645;
	int i;
	for (i = 0; i < len; i++)
	{
		if (data[len] == 0x68)
		{
			break;
		}
	}
	p_645 = (P_Dl645_Freme)&data[len];

	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	//	TransposeAddr(pconfig->DstMac);

	debug_str(DEBUG_LOG_APP, "645 to moudle, sta mac:%02x-%02x-%02x-%02x-%02x-%02x\r\n",
			  p_645->meter_number[0], p_645->meter_number[1], p_645->meter_number[2], p_645->meter_number[3], p_645->meter_number[4], p_645->meter_number[5]);

	for (int idx = 0; idx < p_645->dataLen; idx++)
	{
		p_645->dataBuf[idx] -= 0x33;
	}
	u32 di = *(u32 *)p_645->dataBuf;
	if (memcmp(p_645->meter_number, pNetBasePara->Mac, LONG_ADDR_LEN) == 0 || memIsHex(p_645->meter_number, 0xaa, LONG_ADDR_LEN) || memIsHex(p_645->meter_number, 0xff, LONG_ADDR_LEN))
	{

		if (p_645->controlCode == 0x14 && p_645->dataLen == 16 && (di & 0xffffff00) == DL645_SET_RECORD_INTERVAL(00))
		{
			if (Meter_SupportRecord == 0) // 电表不支持负荷曲线
			{
				for (int i = 0; i < sizeof(Graph_Config.period); i++)
				{
					if (Graph_Config.period[i] == 0 || Graph_Config.ident[0][i] == di)
					{
						Graph_Config.ident[0][i] = di;
						Graph_Config.period[i] = *(u32 *)&p_645->dataBuf[12];
						Graph_Config.ident[1][i] = DL645_SET_RECORD_READ + (p_645->dataBuf[0] << 24);
						break;
					}
				}
				return true;
			}
		}
		else if (p_645->controlCode == 0x14 && p_645->dataLen == 0xa && (di & 0xff00ffff) == DL645_SET_RECORD_READ_33) // 读取数据块
		{
			if (Meter_SupportRecord == 0) // 电表不支持负荷曲线
			{
				const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
				SEND_PDU_BLOCK *pdu = MallocSendPDU();
				if (pdu == NULL)
					return true;
				// 应用层报文头
				APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
				// 应用层业务报文头
				APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];
				// 数据转发业务报文
				APP_READ_METER_UP *sendMeterUp = (APP_READ_METER_UP *)&sendAppGeneralPacket->Data[0];

				// 生成应用层报文头
				CreateAppPacket(sendAppPacket);
				// 生成应用层业务报文头
				CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_TRAN, 0, 0, 0, 1, APP_BUS_TRAN, seq, 0, 0);
				// 生成应用层业务数据
				if (sendType == SENDTYPE_SINGLE)
				{
					memcpy(sendMeterUp->SrcMac, srcAddr, 6);
					memcpy(sendMeterUp->DstMac, dstAddr, 6);
				}
				else
				{
				  	if ((memIsHex(dstAddr, 0x99, LONG_ADDR_LEN)) || (memIsHex(dstAddr, 0xAA, LONG_ADDR_LEN)) || (memIsHex(dstAddr, 0xFF, LONG_ADDR_LEN)))
					{
					  	//广播时源MAC地址填写站点MAC地址，替代0xAAAAAAAAAAAA
						CreateReadMeterAck(sendMeterUp, pNetBasePara->Mac, srcAddr, len, data);
						memcpy(sendMeterUp->SrcMac, pNetBasePara->Mac, 6);

						TransposeAddr(sendMeterUp->SrcMac);
					}
					else	//先同步AppHandleReadMeter的修改，预防广播带具体目的地址时给I采和II采后用其自身地址回复。不一定用得着
					{
					  	CreateReadMeterAck(sendMeterUp, dstAddr, srcAddr, len, data);
					}
					memcpy(sendMeterUp->DstMac, dstAddr, 6);
				}
				sendMeterUp->Reserve = 0;
				u8 deviceAddr[6];
				memcpy(deviceAddr, pNetBasePara->Mac, 6);
				TransposeAddr(deviceAddr);

				sendMeterUp->DataLen = Get645_07GraphFrame(sendMeterUp->Data, *(u32 *)p_645->dataBuf, p_645->dataBuf[4], &p_645->dataBuf[5], deviceAddr);
				sendAppGeneralPacket->AppFrameLen = sizeof(APP_READ_METER_UP) - 1 + len;
				pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
				if (sendType == SENDTYPE_SINGLE)
					MacHandleApp(pdu->pdu, pdu->size, 3, srcTei, SENDTYPE_SINGLE, BROADCAST_DRT_UNKNOW);
				else
					MacHandleApp(pdu->pdu, pdu->size, 3, MAC_BROADCAST_TEI, SENDTYPE_BROADCAST, BROADCAST_DRT_UP);
				FreeSendPDU(pdu);
				return true;
			}
		}
	}
	return false;
#endif
}

#endif
// 解析数据
void AppHandleData(BPLC_PARAM *bplcParam, u8 *data, u16 len)
{
	APP_CREATE_MAC_DEF();

	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	if (pNetBasePara->connectReader)
	{
		if (pNetBasePara->NetState != NET_IN_NET)
		{

			if (macHead->SrcTei != READER_TEI)
			{
				return;
			}
		}
	}
	else
	{
		// 如果还没入网，且不是通信测试帧，返回
		if (pNetBasePara->NetState != NET_IN_NET && (appGeneral->Ctrl.FrameType != APP_FRAME_CMD || appGeneral->BusFlg != APP_BUS_TEST_FRAME))
		{
			return;
		}
	}
	// 如果已经入网,但地址不一致且不是广播地址，返回
	if (pNetBasePara->NetState == NET_IN_NET)
	{
		if (macHead->DstTei != MAC_BROADCAST_TEI && pNetBasePara->TEI != macHead->DstTei)
		{
			if ((appGeneral->Ctrl.FrameType == APP_FRAME_CMD) && (appGeneral->BusFlg == APP_BUS_ZONE_RELATION))
			{
				ZONE_RELATION_PACKET *zonePacket = (ZONE_RELATION_PACKET *)&appGeneral->Data[0];
				u16 nextTei = 0;

				// 相位特征采集指示，单播如果找不到下一跳，不广播转发
				if (zonePacket->CollectType == COLLECT_PHASE_FEATURE_IND)
				{
					if (macHead->SendType == SENDTYPE_SINGLE)
					{
						nextTei = GetRelayTEI(macHead->DstTei);
						if (nextTei == 0)
						{
							macHead->DstTei = pNetBasePara->TEI; // 放止下面转发
						}
					}
				}
			}

			return;
		}
	}
#ifdef LED_REDUCE
	FlashReceiveLed();
#endif
	switch (appGeneral->Ctrl.FrameType)
	{
	case APP_FRAME_CONFIRM: // 确定/否定帧
	{
		if (appPacket->Port == 0x11)
		{
			switch (appGeneral->BusFlg)
			{
			case APP_BUS_CONFIRM:
			case APP_BUS_DENY:
			{
				AppDataConfirmHandle(data, len);
				break;
			}
			default:
				break;
			}
		}
		break;
	}
	case APP_FRAME_TRAN: // 数据转发帧
	{
		switch (appGeneral->BusFlg)
		{
		case APP_BUS_TRAN:
		case APP_BUS_TRAN_MODULE:
		{
			AppReadMeterHandle(data, len, appPacket->Port);
			break;
		}
		default:
			break;
		}
		break;
	}
	case APP_FRAME_CMD: // 命令帧
	{
		switch (appGeneral->BusFlg)
		{
		case APP_BUS_QUIRY_LIST: // 查询搜表结果
		{
			if (appPacket->Port == 0x11)
			{
				AppCheckScanResultHandle(data, len);
			}
			break;
		}
		case APP_BUS_LIST: // 下发搜表列表
		{
			if (appPacket->Port == 0x11)
			{
				AppScanListHandle(data, len);
			}
			break;
		}
		case APP_BUS_FILE_TRAN: // 文件传输
		{
			if (appPacket->Port == 0x11)
			{
				UpdateReceiveEventData(data, len);
			}
			break;
		}
		case APP_BUS_EVENT_FLG: // 允许/禁止事件上报
		{
			if (appPacket->Port == 0x11)
			{
				AppEnableReportHandle(data, len);
			}
			break;
		}
		case APP_BUS_REBOOT: // 重启
		{
			if (appPacket->Port == 0x11)
			{
				AppRebootHandle(data, len);
			}
			break;
		}
		case APP_BUS_QUIRY: // 查询节点信息
		{
			if (appPacket->Port == 0x11)
			{
				AppQueryStationHandle(data, len);
			}
			break;
		}
		case APP_BUS_TRAN_LIST: // 下发映射表
		{
			if (appPacket->Port == 0x11)
			{
				AppMeterListHandle(data, len);
			}
			break;
		}
		case APP_BUS_STA_STATUS: // 查询从节点运行状态
		{
			if (appPacket->Port == 0x13)
			{
				AppRunStatusHandle(data, len);
			}
			break;
		}
		case APP_BUS_STA_CHANNEL: // 查询从节点信道信息
		{
			if (appPacket->Port == 0x13)
			{
				AppStaChannelHandle(data, len);
			}
			break;
		}
		case APP_BUS_TEST_FRAME: // 测试帧
		{
			if (appPacket->Port == 0x11)
			{
				AppTestModelHandle(data, len);
			}
			break;
		}
		case APP_BUS_ZONE_RELATION: // 台区户变关系/相位识别
		{
			if (appPacket->Port == 0x11)
			{
				ZoneReceiveEventData(data, len, 1, pNetBasePara->NID);
			}
			break;
		}

		default:
			break;
		}
		break;
	}
	case APP_FRAME_READER:
		if ((appPacket->Port == 0x11) && (appGeneral->BusFlg == 1)) // 串口数据透传模式
		{
			AppReaderHandle(data, len);
		}
		break;
	case APP_FRAME_REPORT: // 主动上报帧
	{
		if ((appPacket->Port == 0x13) && (appGeneral->BusFlg == APP_BUS_POWER_EVENT_REPORT))
		{
			AppPowerEventHandle(appPacket->Data, len); // len 在此函数中无用
		}
		break;
	}
	case APP_FRAME_DEBUG://  LC 内部命令
	{
		LcAppHandleData(data, len);
		break;
	}
	case APP_FRAME_FACTORY: // 内部（威胜生产）
	{
		if (appPacket->Port == 0xFF)
		{
			AppExtFactoryTestHandle(bplcParam, data, len);
		}
		break;
	}

	default:
		break;
	}
}

bool is_GuangDongQY = false;

// 解析单跳帧
void AppOneHopHandleData(BPLC_PARAM *bplcParam, u8 *data, u16 len)
{
	MAC_HEAD_ONE_HOP *macHead = (MAC_HEAD_ONE_HOP *)data;

	APP_PACKET *appPacket = 0;
	APP_GENERAL_PACKET *appGeneral = 0;

	appPacket = (APP_PACKET *)(macHead + 1);
	appGeneral = (APP_GENERAL_PACKET *)&appPacket->Data[0];

	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();

	// 如果还没入网，且不是通信测试帧，返回
	if (pNetBasePara->NetState != NET_IN_NET)
	{
		return;
	}
	// 如果已经入网,但地址不一致且不是广播地址，返回

#ifdef LED_REDUCE
	FlashReceiveLed();
#endif
	switch (appGeneral->Ctrl.FrameType)
	{
	case APP_FRAME_REPORT: // 事件上报
	{
		if ((appPacket->Port == 0x13) && (appGeneral->BusFlg == APP_BUS_POWER_EVENT_REPORT))
		{
			AppPowerEventHandle(appPacket->Data, len); // len 在此函数中无用
		}
		break;
	}

	default:
		break;
	}
}

// 处理抄表返回
void AppHandleReadMeter(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8 *data, u16 len, u8 sendType, u16 srcTei)
{
	if (vaild)
	{
#ifdef I_STA
		if (len > *(u16 *)data)
		{
			len = *(u16 *)data;
			data += 2;
		}
#endif
		const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
		SEND_PDU_BLOCK *pdu = MallocSendPDU();
		if (pdu == NULL)
			return;

		// 应用层报文头
		APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
		// 应用层业务报文头
		APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];
		// 数据转发业务报文
		APP_READ_METER_UP *sendMeterUp = (APP_READ_METER_UP *)&sendAppGeneralPacket->Data[0];

		// 生成应用层报文头
		CreateAppPacket(sendAppPacket);
		// 生成应用层业务报文头
		CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_TRAN, 0, 0, 0, 1, APP_BUS_TRAN, seq, 0, 0);
		// 生成应用层业务数据
		if (sendType == SENDTYPE_SINGLE)
		{
			CreateReadMeterAck(sendMeterUp, dstAddr, srcAddr, len, data);
		}
		else
		{
			// 广播时源MAC地址填写站点MAC地址，替代0xAAAAAAAAAAAA
		  	if ((memIsHex(dstAddr, 0x99, LONG_ADDR_LEN)) || (memIsHex(dstAddr, 0xAA, LONG_ADDR_LEN)) || (memIsHex(dstAddr, 0xFF, LONG_ADDR_LEN)))
			{
				CreateReadMeterAck(sendMeterUp, pNetBasePara->Mac, srcAddr, len, data);
				TransposeAddr(sendMeterUp->SrcMac);
			}
			else
			{
			  	CreateReadMeterAck(sendMeterUp, dstAddr, srcAddr, len, data);
			}
		}

		debug_str(DEBUG_LOG_NET, "AppHandleReadMeter, seq:%d, len:%d\r\n", seq, len);

		sendAppGeneralPacket->AppFrameLen = sizeof(APP_READ_METER_UP) - 1 + len;
		pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
		if (sendType == SENDTYPE_SINGLE || srcTei == READER_TEI)
			MacHandleApp(pdu->pdu, pdu->size, 3, srcTei, SENDTYPE_SINGLE, BROADCAST_DRT_UNKNOW, 0);
		else
			MacHandleApp(pdu->pdu, pdu->size, 3, MAC_BROADCAST_TEI, SENDTYPE_BROADCAST, BROADCAST_DRT_UP, 0);

		FreeSendPDU(pdu);
	}
#ifdef II_STA_GDQY_COMPATIBLE
	else if (is_GuangDongQY) // 清远低压集抄综合鉴定台体测试II采实时召测，虚拟表切换波特率
	{
		if (dstAddr && !memIsHex(dstAddr, 0xAA, LONG_ADDR_LEN) && !memIsHex(dstAddr, 0xFF, LONG_ADDR_LEN))
		{
			u32 currentBaud = FindBaudInSEARCH_METER_FLAG_table(dstAddr);
			debug_str(DEBUG_LOG_NET, "AppReadMeter Timeout Rsp, baud: %d, %02x%02x%02x%02x%02x%02x\r\n", currentBaud,
					  dstAddr[5], dstAddr[4], dstAddr[3], dstAddr[2], dstAddr[1], dstAddr[0]);

			u8 nextBaudType = 0;
			if (GetNextBaudType(currentBaud, &nextBaudType))
			{
				debug_str(DEBUG_LOG_NET, "AppReadMeter Next baud: %d-%d\r\n", nextBaudType - 1, GetBaudFromType(nextBaudType));
				ChangeBaudInSEARCH_METER_FLAG_table(dstAddr, nextBaudType);
			}
		}
	}
#endif
}
void AppHandleReadModule(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8 *data, u16 len, u8 sendType, u16 srcTei)
{
	if (vaild)
	{
		const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
		SEND_PDU_BLOCK *pdu = MallocSendPDU();
		if (pdu == NULL)
			return;

		// 应用层报文头
		APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
		// 应用层业务报文头
		APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];
		// 数据转发业务报文
		APP_READER_UART_FRAME *send2ReaderUp = (APP_READER_UART_FRAME *)&sendAppGeneralPacket->Data[0];

		// 生成应用层报文头
		CreateAppPacket(sendAppPacket);
		sendAppPacket->Port = 0x11;
		// 生成应用层业务报文头
		CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_READER, 0, 0, 0, 1, 1, seq, 0, 0);
		// 生成应用层业务数据
		send2ReaderUp->type = 0;
		send2ReaderUp->start = 0;
		send2ReaderUp->rate = 0;
		send2ReaderUp->seq = GetStaBaseAttr()->ReaderSeq;
		send2ReaderUp->len = len;
		memcpy(send2ReaderUp->data, data, len);
		sendAppGeneralPacket->AppFrameLen = sizeof(APP_READER_UART_FRAME) + len;
		pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
		//		if(sendType == SENDTYPE_SINGLE)
		MacHandleApp(pdu->pdu, pdu->size, 3, READER_TEI, SENDTYPE_SINGLE, BROADCAST_DRT_UNKNOW, 0);
		//        else
		//            MacHandleApp(pdu->pdu, pdu->size, 3, MAC_BROADCAST_TEI, SENDTYPE_BROADCAST, BROADCAST_DRT_UP);

		FreeSendPDU(pdu);
	}
}
// 抄控器测试帧返回
void AppHandleReaderBack(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8 *data, u16 len, u8 sendType, u16 srcTei)
{
	if (vaild)
	{
		const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
		SEND_PDU_BLOCK *pdu = MallocSendPDU();
		if (pdu == NULL)
			return;

		// 应用层报文头
		APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
		// 应用层业务报文头
		APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];
		// 数据转发业务报文

		// 生成应用层报文头
		sendAppPacket->Port = 0x12;
		sendAppPacket->PacketID = 0x101;
		sendAppPacket->Reserve = 0;
		// 生成应用层业务报文头
		CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_CMD, 0, 0, 0, 1, APP_BUS_TEST_FRAME, seq, 0, 0);
		// 生成应用层业务数据
		Reader_Support *pdata = (Reader_Support *)sendAppGeneralPacket->Data;
		memcpy(pdata->data, data, len);
		pdata->dl645_len = len;
		memcpy(pdata->src_addr, dstAddr, LONG_ADDR_LEN);
		memcpy(pdata->dst_addr, srcAddr, LONG_ADDR_LEN);
		pdata->unkonwByte1 = pdata->dl645_len >= 0x14 ? 0x3b : 0xfb;
		pdata->unkonwByte2 = 0x1;
		pdata->fix_byte[0] = 0x11;
		pdata->fix_byte[1] = 0x01;
		pdata->fix_byte[2] = 0xff;
		pdata->send_flag = 0;
		pdata->rec_flag = 1;
		pdata->seq = seq;
		pdata->res1 = 0;

		sendAppGeneralPacket->AppFrameLen = sizeof(Reader_Support) + len;
		pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;

		MacHandleApp(pdu->pdu, pdu->size, 3, MAC_BROADCAST_TEI, SENDTYPE_BROADCAST, BROADCAST_DRT_UP, 0);

		FreeSendPDU(pdu);
	}
}

// 处理事件返回
void AppHandleEvent(u8 addr[6], u8 dstAddr[6], u16 seq, bool vaild, u8 *data, u16 len, u8 sendType, u16 srcTei)
{
	if (!Is645Frame(data, len))
	{
		vaild = false;
	}
	if (AppEventState == APP_EVENT_WAIT_LOCAL && vaild)
	{
		P_Dl645_Freme p = NULL;
		for (int index = 0; index < len; index++)
		{
			p = (P_Dl645_Freme)&data[index];
			if (p->protocolHead == 0x68)
			{
				break;
			}
		}

		if (p)
		{
			if ((p->dataLen <= 4) ||   // 长度不足（小于等于数据标识）
				p->controlCode & 0x40) // 异常应答
			{
				if (EventPdu)
				{
					FreeSendPDU(EventPdu);
					EventPdu = NULL;
				}
				StartTimer(&EventHighTimer, 10 * 60 * 1000UL);
				AppEventState = APP_EVENT_CLEAR_HIGH;
				debug_str(DEBUG_LOG_APS, "event@get meter err ack\r\n");
				return;
			}
		}
		else
		{
			return;
		}

		u8 addr[6] = {0};
		const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
		SEND_PDU_BLOCK *pdu = MallocSendPDU();
		if (pdu == NULL)
		{
			AppEventState = APP_EVENT_IDLE;
			return;
		}

		memcpy(addr, pNetBasePara->Mac, 6);
		TransposeAddr(addr); // 地址转成大端

		// 应用层报文头
		APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
		// 应用层业务报文头
		APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];
		// 事件上报报文
		APP_EVENT_REPORT *eventUp = (APP_EVENT_REPORT *)&sendAppGeneralPacket->Data[0];

		// 生成应用层报文头
		CreateAppPacket(sendAppPacket);
		// 生成应用层业务报文头
		meterPacketID = EventPacketID++;
		CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_REPORT, 0, 1, 1, 1, APP_BUS_EVENT_REPORT, meterPacketID, 0, 0);
		// 生成应用层业务数据
		CreateEventReportFrame(eventUp, addr, data, len);

		sendAppGeneralPacket->AppFrameLen = sizeof(APP_EVENT_REPORT) - 1 + len;
		pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
		MacHandleApp(pdu->pdu, pdu->size, 3, CCO_TEI, SENDTYPE_SINGLE, BROADCAST_DRT_UP, 0);

		AppEventState = APP_EVENT_WAIT_ACK;
		StartTimer(&EventTimer, MAX_WAIT_EVENT_ACK_TIME);
		EventPdu = pdu;
	}
}
EN_DEVICE_TYPE GetStaType(void)
{
#if defined(I_STA)
	return I_COLLECTOR;
#elif defined(II_STA)
	return II_COLLECTOR;
#else
	if (myDevicType <= 7) // 外部管脚识别到模块类型
	{
#if (defined CSG_201905) || (defined GUIZHOU_SJ)
		if (myDevicType == 7) // 2019年5月版规约，设备类型不区分单三相
		{
			return METER;
		}
#endif
		return (EN_DEVICE_TYPE)myDevicType;
	}
	return METER;
#endif
}

#ifdef REPORT_OTHER_CCO
void ReportOtherCCO(void)//* arg
{
    //static TimeValue SendBindTimer = { 0 , 0};
    const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();

	//if(TimeOut(&SendBindTimer))
	if(pNetBasePara->NetState==NET_IN_NET)
	{
		SEND_PDU_BLOCK *pdu = MallocSendPDU();
		if (pdu == NULL)
			return;		

		//搴旂敤灞傛姤鏂囧ご
		APP_PACKET *sendAppPacket = (APP_PACKET *)pdu->pdu;
		//搴旂敤灞備笟鍔℃姤鏂囧ご
		APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET *)&sendAppPacket->Data[0];
		//浜嬩欢涓婃姤鎶ユ枃
		u8 *eventUp = &sendAppGeneralPacket->Data[0];

		//鐢熸垚搴旂敤灞傛姤鏂囧ご
		CreateAppPacket(sendAppPacket);
		sendAppPacket->Port = 0x13; //浜嬩欢涓婃姤绔彛鍙蜂负0x13
		//鐢熸垚搴旂敤灞備笟鍔℃姤鏂囧ご
		meterPacketID = EventPacketID++;
		CreateAppBusinessPacket(sendAppGeneralPacket, APP_FRAME_REPORT, 0, 1, 1, 1, 0x4, meterPacketID, 0, 0);
		//鐢熸垚搴旂敤灞備笟鍔℃暟鎹?
		u8 cnt=0;
		for(u8 i=0;i<16;i++){
			if(otherNetBeacon[i].tick != 0){		  
				memcpy(&eventUp[1+cnt*6],otherNetBeacon[i].CCOMacAddr,6);
				cnt++;
			}
		}
		if(cnt==0)
		{
			FreeSendPDU(pdu);
			return;
		}
		eventUp[0] = cnt;

		debug_str(DEBUG_LOG_APP, "Report other net To CCO: ");
		debug_hex(DEBUG_LOG_APP, eventUp, 1+cnt*6);
		
		sendAppGeneralPacket->AppFrameLen = 1+cnt*6;
		pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
		MacHandleApp(pdu->pdu, pdu->size, 3, CCO_TEI, SENDTYPE_SINGLE, BROADCAST_DRT_UP,0);
		FreeSendPDU(pdu);		
	}
}
#endif

bool IsMacEqual(uint8_t *mac, uint8_t *income_mac)
{
	uint8_t up_part_mac = 0;
	uint8_t low_part_mac = 0;
	uint8_t up_part_income_mac = 0;
	uint8_t low_part_income_mac = 0;
	int i =0;
	for(i = 0; i < 6; i++)
	{
		up_part_mac = (mac[i] >> 4) & 0x0F;
		low_part_mac = mac[i] & 0x0F;
		up_part_income_mac = (income_mac[i] >> 4) & 0x0F;
		low_part_income_mac = income_mac[i] & 0x0F;
		if((up_part_income_mac != 0x0F) && (up_part_mac != up_part_income_mac))
			return false;
		
		if((low_part_income_mac != 0x0F) && (low_part_mac != low_part_income_mac))
			return false;
	}

	return true;
}

//濡傛灉杩斿洖true锛岀户缁叆缃戯紝濡傛槸杩斿洖false锛屽氨涓嶅叆杩欎釜缃戠粶
bool CheckNetCCOMac(uint8_t *mac, uint32_t nid)
{
	debug_str(DEBUG_LOG_UPDATA, "CheckNetCCOMac mac = %02x %02x %02x %02x %02x %02x\r\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	for(int i=0;i<WHITE_BLACK_MAC_NUM;i++)
	{
		uint8_t error_mac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
		//寮傚父鍦板潃搴旇烦杩?
		if(memcmp(error_mac,g_mac_list.black_mac_list[i],6) == 0)
		{
			continue;
		}
		if(true == IsMacEqual(mac,g_mac_list.black_mac_list[i]))
		{
			debug_str(DEBUG_LOG_UPDATA,"CheckNetCCOMac %d err \r\n",i);
			return false;
		}
	}
	
	//鍥哄畾鍐欐锛屽鏋滄槸00AX寮€澶寸殑鍦板潃锛屼竴寰嬮€氳繃锛岄渶瑕佽繃婊よ璁剧疆榛戝悕鍗?閬垮厤鐧藉悕鍗曞垵濮嬪€艰閿欒鏇存敼)
	if((mac[0] == 0x00) && ((mac[1] & 0xA0) == 0xA0))
	{
		return true;
	}
	for(int i=0;i<WHITE_BLACK_MAC_NUM;i++)
	{
		if(true == IsMacEqual(mac,g_mac_list.white_mac_list[i]))
		{
			debug_str(DEBUG_LOG_UPDATA,"CheckNetCCOMac %d ok \r\n",i);
			return true;
		}
	}
	return false;
}

//鑾峰彇鎷撴墤PLC淇℃伅
uint32_t lc_get_plc_neighbor_info(uint8_t *p_nodes,const uint32_t offset, const uint32_t count,uint16_t *p_sum)
{
	
    uint16_t macaddrsum = 0;
    uint16_t needreadmeterindex = offset;
    uint16_t needreadmeterstopindex = offset + count;
    uint16_t needreadmetersum = 0;
	
	const ST_NEIGHBOR_TAB_TYPE *info = GetNeighborTable();
	for(uint16_t index=0; index < MAX_BITMAP_SIZE/4; index++)
	{
		for(int j=0;j<32;j++)
		{
			if(info->TEI_MAP[index] & (0x01<<j)) 
			{
			
				uint16_t tei = index*32+j;
				macaddrsum++;
				if((macaddrsum >= needreadmeterindex) && (macaddrsum < (needreadmeterstopindex)))
				{
					Obj_NeighborTable *data = (Obj_NeighborTable *)(p_nodes+sizeof(Obj_NeighborTable)*needreadmetersum);
					ST_NEIGHBOR_TYPE *ngh_info = NULL;
					bool ret = GetNeighborInfo(tei, &ngh_info);
					if(ret == true && ngh_info != NULL)
					{
						uint8_t tmpmac[6] = {0};
						//CCO闇€瑕佸弽杞湴鍧€
						if(tei == 1)
						{
							for(int i=0;i<6;i++)
							{
								tmpmac[i] = ngh_info->Mac[5-i];
							}
							memcpy(data->mac,tmpmac,LONG_ADDR_SIZE);
						}
						else
						{
							memcpy(data->mac,ngh_info->Mac,LONG_ADDR_SIZE);
						}
						data->layer = ngh_info->Layer;
						uint8_t num = 0;
						uint8_t up_succ = 0;
						uint8_t down_succ = 0;
						for(int i=0;i<2;i++)
						{
							if(ngh_info->SucRate[i].Flag == true)
							{	
								up_succ +=  ngh_info->SucRate[i].UpRate;
								down_succ +=  ngh_info->SucRate[i].DownRate;
								num++;
							}
						}
						if(num > 0)
						{
							data->up_succ = up_succ/num;
							data->down_succ = down_succ/num;
						}
						else
						{
							data->up_succ = 0;
							data->down_succ = 0;
						}
						data->snr = ngh_info->SNR;
						data->rssi = ngh_info->RSSI;
						needreadmetersum ++;
					}
					else
					{
						macaddrsum--;
					}
				}
			}
		}
	}
	
	*p_sum = macaddrsum;
	return needreadmetersum;
}

//鑾峰彇鎷撴墤PLC淇℃伅
uint32_t lc_get_rf_neighbor_info(uint8_t *p_nodes,const uint32_t offset, const uint32_t count,uint16_t *p_sum)
{
	
    uint16_t macaddrsum = 0;
    uint16_t needreadmeterindex = offset;
    uint16_t needreadmeterstopindex = offset + count;
    uint16_t needreadmetersum = 0;
	
	const ST_NEIGHBOR_TAB_TYPE *info = GetNeighborTable();
	for(uint16_t index=0; index < MAX_BITMAP_SIZE/4; index++)
	{
		for(int j=0;j<32;j++)
		{
			if(info->TEI_MAP[index] & (0x01<<j)) 
			{
				uint16_t tei = index*32+j;
				macaddrsum++;
				if((macaddrsum >= needreadmeterindex) && (macaddrsum < (needreadmeterstopindex)))
				{
					Obj_NeighborTable *data = (Obj_NeighborTable *)(p_nodes+sizeof(Obj_NeighborTable)*needreadmetersum);
					ST_NEIGHBOR_TYPE *ngh_info = NULL;
					bool ret = GetNeighborInfo(tei, &ngh_info);
					if(ret == true && ngh_info != NULL)
					{
						uint8_t tmpmac[6] = {0};
						//CCO闇€瑕佸弽杞湴鍧€
						if(tei == 1)
						{
							for(int i=0;i<6;i++)
							{
								tmpmac[i] = ngh_info->Mac[5-i];
							}
							memcpy(data->mac,tmpmac,LONG_ADDR_SIZE);
						}
						else
						{
							memcpy(data->mac,ngh_info->Mac,LONG_ADDR_SIZE);
						}
						data->layer = ngh_info->Layer;
						data->up_succ = ngh_info->HrfUpRate;
						data->down_succ = GetHrfDownRate(tei);
						data->snr = ngh_info->RfSnr;
						data->rssi = ngh_info->RfRssi;
						needreadmetersum ++;
					}
					else
					{
						macaddrsum--;
					}
				}
			}
		}
	}
	
	*p_sum = macaddrsum;
	return needreadmetersum;
}

//鑾峰彇鎷撴墤鑺傜偣RF淇℃伅
bool lc_get_pco_info(uint8_t *data)
{
	Obj_Pco_Info *info = (Obj_Pco_Info *)data;
	uint16_t pcotei = GetPCOTei();
	ST_NEIGHBOR_TYPE *ngh_info = NULL;
	bool ret = GetNeighborInfo(pcotei, &ngh_info);
	if(ret == false || ngh_info == NULL)
	{
		return false;
	}
	memcpy(info->mac,ngh_info->Mac,LONG_ADDR_SIZE);
	info->layer = ngh_info->Layer;
	uint8_t num = 0;
	uint8_t up_succ = 0;
	uint8_t down_succ = 0;
	if(1 == GetRelaySendLink(1))
	{
		info->conntype = 0;
		for(int i=0;i<2;i++)
		{
			if(ngh_info->SucRate[i].Flag == true)
			{	
				up_succ +=  ngh_info->SucRate[i].UpRate;
				down_succ +=  ngh_info->SucRate[i].DownRate;
				num++;
			}
		}
		if(num > 0)
		{
			info->up_succ = up_succ/num;
			info->down_succ = down_succ/num;
		}
		else
		{
			info->up_succ = 0;
			info->down_succ = 0;
		}
	}
	else
	{
		info->up_succ = ngh_info->HrfUpRate;
		info->down_succ = GetHrfDownRate(pcotei);
		info->conntype = 1;
	}
	
	return true;
}


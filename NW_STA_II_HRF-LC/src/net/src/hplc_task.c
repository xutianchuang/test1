#include <string.h>
#include "os.h"
#include "protocol_includes.h"
#include "common_includes.h"
#include "phy_port.h"
#include "os_cfg_app.h"
#include <time.h>
#include "ble_net.h"
#include "ble_api.h"
#include "lamp_date.h"
#include "lc_whi_adapt.h"

//发射功率自动切换模式默认功率
// #ifdef TX_POWER_AUTO_HIGH_28
// const u8 HPLC_LinePower[] = {2,2,2,2};
// #else
// const u8 HPLC_LinePower[] = {2,4,7,6};
// #endif

u8 HPLC_ChlPower[4] = {0, 0, 0, 0};
u8 HRF_ChlPower[2] = {0, 0};


#ifdef NW_TEST
#if defined(ZB204_CHIP)
	const u8 MAX_HPLC_POWER[] = {2,8,7,6};
	#elif defined(ZB205_CHIP)
	const u8 MAX_HPLC_POWER[] = {4,12,12,4};
#endif
#endif

#ifdef NW_TEST
#if defined(ZB204_CHIP)
const u8 HPLC_HighPower[] = {2,4,5,6};
const u8 HPLC_LowPower[] = {2,4,7,6};
const u8 HPLC_TestChlPower[] = {0,0,4,4};
const u8 HPLC_PowerOffPower[] = {0,0,0,0};
const u8 HRF_HighPower[] = {27,27};
const u8 HRF_LowPower[] = {27,27};
const u8 HRF_TestChlPower[] = {16,10};//发射模板和EVM验证OK
const u8 HRF_PowerOffPower[] = {14,14};
#elif defined(ZB205_CHIP)
const u8 HPLC_HighPower[] = {2,2,3,4};
const u8 HPLC_LowPower[] = {3,2,3,2};
const u8 HPLC_TestChlPower[] = {4,4,12,4};
const u8 HPLC_PowerOffPower[] = {0,0,0,0};
const u8 HRF_HighPower[] = {5,5};
const u8 HRF_LowPower[] = {0,0};
const u8 HRF_TestChlPower[] = {0,0};//发射模板和EVM验证OK
const u8 HRF_PowerOffPower[] = {4,4};
#endif
#else
#if defined(ZB204_CHIP)
const u8 HPLC_HighPower[] = {11,11,11,11};
const u8 HPLC_LowPower[] = {7, 6, 8, 6};
const u8 HPLC_TestChlPower[] = {0,0,4,4};
const u8 HPLC_PowerOffPower[] = {0,0,0,0};
const u8 HRF_HighPower[] = {27,27};
const u8 HRF_LowPower[] = {27,27};
const u8 HRF_TestChlPower[] = {16,10};
const u8 HRF_PowerOffPower[] = {14,14};
#elif defined(ZB205_CHIP)
const u8 HPLC_HighPower[] = {11,11,11,11};
const u8 HPLC_LowPower[] = {7,6,8,6};
const u8 HPLC_TestChlPower[] = {4,4,12,4};
const u8 HPLC_PowerOffPower[] = {0,0,0,0};
const u8 HRF_HighPower[] = {10,10};
const u8 HRF_LowPower[] = {10,10};
const u8 HRF_TestChlPower[] = {0,0};//发射模板和EVM验证OK
const u8 HRF_PowerOffPower[] = {4,4};
#endif
#endif

//PLC接收数据队列
OS_Q PlcDataQueue;
static OS_SEM BroadTimeSem;
static u32 ReceivePhyNum = 0;

static u32 ReceiveErrFC = 0;

static u32 ReceiveCorrectFC = 0;

u32  Meter_SupportRecord=0xff;
extern OS_TCB  App_GraphTCB;

#ifdef AREA_EXTEND_RESULT
const u8 area_extend_result = 1;
#else
const u8 area_extend_result = 0;
#endif

void PLC_StartCalibMeter(void)
{
	OS_ERR err;
	OSSemPost(&BroadTimeSem, OS_OPT_POST_1, &err);
}
void ReceivePhyCallback(uint8_t *data, uint16_t len, uint8_t crc_res, BPLC_recv_para *para)
{
    OS_ERR err;
	
    if(len && len <= MEM_BLOCK_SIZE)
    {
        PDU_BLOCK* newPdu = MallocPDU();
        if(newPdu == NULL)
        {
            UnlockUserBuf();
			return;
        }
        memcpy(newPdu->pdu, data, len);
        newPdu->size = len;
		newPdu->crcState = crc_res;
		newPdu->type = RECEIVE_PL_PDU;
        memcpy(&newPdu->rxPara, para, sizeof(BPLC_recv_para));
        OSQPost(&PlcDataQueue,newPdu,crc_res,OS_OPT_POST_FIFO | OS_OPT_POST_ALL,&err);
		if (err != OS_ERR_NONE)
		{
			FreePDU(newPdu);
		}
    }

	ReceivePhyNum++;
    UnlockUserBuf();
}
extern void CSMA_VCSCallBack(void);
void ReceiveCtrlCallback(uint8_t *data, BPLC_recv_para *para, bool_t status)
{
    OS_ERR err;
	if(status)
	{
		ReceiveCorrectFC++;
		PDU_BLOCK* newPdu = MallocPDU();
		if(newPdu == NULL)
		{
			UnlockUserFcBuf();
			return;
		}
		memcpy(newPdu->pdu, data, 16);
		newPdu->size = 16;
		newPdu->type = RECEIVE_FC_PDU;
		memcpy(&newPdu->rxPara, para, sizeof(BPLC_recv_para));
		OSQPost(&PlcDataQueue,newPdu,status,OS_OPT_POST_FIFO | OS_OPT_POST_ALL,&err);
		if (err != OS_ERR_NONE)
		{
			FreePDU(newPdu);
		}
	}
	else
	{
		ReceiveErrFC++;
        VCS_HPLC_Conflict();
	}
	
    UnlockUserFcBuf();
}


#ifdef FIX_HRF_LEN
extern u32  ZB204CCO_nid[5];
#endif

void HRF_ReceivePhyCallback(uint8_t *data, uint16_t len, uint8_t crc_res, BPLC_recv_para *para)
{
    OS_ERR err;
//	debug_hex(DEBUG_LOG_PHY,data,len);
    if(len && len <= MEM_BLOCK_SIZE)
    {
        PDU_BLOCK* newPdu = MallocPDU();
        if(newPdu == NULL)
        {
            HRF_UnlockUserBuf();
			return;
		}
		#ifndef FIX_HRF_LEN
		memcpy(newPdu->pdu, data, len);
		#else

		if (len > (16))
		{
			u8 j;
			for (j = 0; j < 4; j++)
			{
				if (*(u32 *)&data[16 + (4 * j)] != *(u32 *)&data[16 + (4 * (j + 1))])
				{
					break;
				}
			}
			u16 pbsize = 0;
			u8 pbindex = 0;
			memcpy(newPdu->pdu, data, 16);
			MPDU_CTL *mpduCtl = (MPDU_CTL *)newPdu->pdu;
			SOF_HRF_ZONE *sofZone = (SOF_HRF_ZONE *)mpduCtl->Zone0;
			BEACON_HRF_ZONE *beaconZone = (BEACON_HRF_ZONE *)mpduCtl->Zone0;
			if (mpduCtl->Type == 0)
				pbindex = beaconZone->PBSize;
			else
				pbindex = sofZone->PbSize;
			if(pbindex <= 5&&pbindex>0)
			{

				if (mpduCtl->Type == 0)
				{
					beaconZone->PBSize--;
				}
				else
				{
					sofZone->PbSize--;
				}
				_CRC_24_ FCcrc24 = GetCrc24(&newPdu->pdu[0], 16 - 3);
				memcpy(&newPdu->pdu[16 - 3], &FCcrc24, 3);

				pbsize = GetRcvPbSize(pbindex - 1);
				memcpy(&newPdu->pdu[16], &data[16 + (4 * j)], pbsize);
//                        _CRC_24_ PRcrc24 = GetCrc24(&newPdu->pdu[16],pbsize-3);
//                        memcpy(&newPdu->pdu[pbsize+16-3], &PRcrc24, 3);
				len = pbsize + 16;
			}
			else
			{
				__BKPT(1);
			}
		}

		else
		{
			memcpy(newPdu->pdu, data, len);
		}
		#endif
        newPdu->size = len;
		newPdu->crcState = crc_res;
		newPdu->type = RECEIVE_HRF_PL_PDU;
        memcpy(&newPdu->rxPara, para, sizeof(BPLC_recv_para));
        OSQPost(&PlcDataQueue,newPdu,crc_res,OS_OPT_POST_FIFO | OS_OPT_POST_ALL,&err);
		if (err != OS_ERR_NONE)
		{
			FreePDU(newPdu);
		}
	}

    HRF_UnlockUserBuf();
}

void HRF_ReceiveCtrlCallback(uint8_t *data, BPLC_recv_para *para, bool_t status)
{
    OS_ERR err;
	
	if(status)
	{
		ReceiveCorrectFC++;
//		debug_hex(DEBUG_LOG_PHY,data,16);
		PDU_BLOCK* newPdu = MallocPDU();
		if(newPdu == NULL)
		{
			HRF_UnlockUserPhrBuf();
			return;
		}
		memcpy(newPdu->pdu, data, 16);
		newPdu->size = 16;
		newPdu->type = RECEIVE_HRF_FC_PDU;
		memcpy(&newPdu->rxPara, para, sizeof(BPLC_recv_para));
		OSQPost(&PlcDataQueue,newPdu,status,OS_OPT_POST_FIFO | OS_OPT_POST_ALL,&err);
		if (err != OS_ERR_NONE)
		{
			FreePDU(newPdu);
		}
	}
	else
	{
		ReceiveErrFC++;
        VCS_HRF_Conflict();
	}
	
    HRF_UnlockUserPhrBuf();
}

void HPLC_Task_Init(void)
{
    OS_ERR err;
    OSQCreate(&PlcDataQueue,"PlcDataQueue",MAX_RECEIVE_PDU_NUM,&err);
    if(err != OS_ERR_NONE)
    {
        SAFE_SYSTEM_RESET(SYS_RESET_POS_0);
    }
	OSSemCreate(&BroadTimeSem, "broad time sem", 0, &err);
	if(err != OS_ERR_NONE)
    {
        SAFE_SYSTEM_RESET(SYS_RESET_POS_0);
    }
}
static u32 timeH=0;
static u32 timeL=0;
void remRunTime(void)
{
	OS_ERR err;
	if(OSTimeGet(&err)<timeL)
	{
		timeH++;
	}
	timeL=OSTimeGet(&err);
}
u32 getRunTimeS(void)
{
	u32 temp=0;
	if (timeH)
	{
		temp=0xffffffff/OS_CFG_TICK_RATE_HZ;
		temp*=timeH;
	}
	temp+=timeL/OS_CFG_TICK_RATE_HZ;
	return temp;
}


s16 resetTime=0;
void Reset_CallBack(u32 arg)
{
	if (resetTime<0)
	{
		return;
	}
	resetTime = 10;
}
extern int power;
void Reset_Proc()
{
	if (!power)
	{
		resetTime = 0;
		return;
	}
	if (GetStaBaseAttr()->connectReader) // 链接抄控器时不复位
	{
		resetTime = 0;
		return;
	}
#ifndef RESET_DETECT_ONLY_LOW_LEVEL
	static u8 resetHighCnt = 0;
#endif
	if (myDevicType == METER)
	{
#if defined(RESET_PORT)
		if (resetTime > 0)
		{
			if (--resetTime == 0)
			{
				if (GPIO_PinRead(RESET_PORT, RESET_PIN) == 0)
				{
					resetTime = -1;
#ifndef RESET_DETECT_ONLY_LOW_LEVEL
					resetHighCnt = 0;
#endif
				}
			}
		}
		else if (resetTime < 0)
		{
#ifdef RESET_DETECT_ONLY_LOW_LEVEL
			if (!power)
			{
				resetTime = 0;
				return;
			}

			REBOOT_SYSTEM();
#else
			if (--resetTime > -1000)
			{
				if (GPIO_PinRead(RESET_PORT, RESET_PIN) == 1)
				{
					resetHighCnt++;
				}
				else
				{
					resetHighCnt = 0;
				}
				if (resetHighCnt == 10)
				{
					REBOOT_SYSTEM();
				}
			}
			else
			{
				resetTime = 0;
			}
#endif
		}
#endif
	}
	else if (myDevicType == METER_3PHASE)
	{
#if defined(RESET_PORT_3PHASE)
		if (resetTime > 0)
		{
			if (--resetTime == 0)
			{
				if (GPIO_PinRead(RESET_PORT_3PHASE, RESET_PIN_3PHASE) == 0)
				{
					resetTime = -1;
#ifndef RESET_DETECT_ONLY_LOW_LEVEL
					resetHighCnt = 0;
#endif
				}
			}
		}
		else if (resetTime < 0)
		{
#ifdef RESET_DETECT_ONLY_LOW_LEVEL
			if (!power)
			{
				resetTime = 0;
				return;
			}

			REBOOT_SYSTEM();
#else
			if (--resetTime > -1000)
			{
				if (GPIO_PinRead(RESET_PORT, RESET_PIN) == 1)
				{
					resetHighCnt++;
				}
				else
				{
					resetHighCnt = 0;
				}
				if (resetHighCnt == 10)
				{
					REBOOT_SYSTEM();
				}
			}
			else
			{
				resetTime = 0;
			}
#endif
		}
#endif
	}
}

#ifndef II_STA
void TopoCallBack(void)
{

}
TimerParameterTypdef TopoPWMTmr = {
	TOPO_PWM_TMR,
	5000,
	NULL,
};
TimerParameterTypdef TopoTmr = {
	TOPO_TOPO_TMR,
	5000,
	NULL,
};


#ifndef NEW_TOPO
struct{
	u8 start_topo;
	u8 topo_status;
	u8 last_outbit;
	u8 send_interval;//发送间隔 S
	u8 sec_inc;   //等待计数
	u8 send_cnt;//发送次数
	u8 Topo_Type;
	u8 bit_len;
	u16 send_num;
}TopoInfo;
#else
struct{
	u8 start_topo;
	u8 topo_status;
	u8 last_outbit;
	u8 sec_inc;   //等待计数
	u8 bit_len;
	u16 send_num;
	u16 bit_ms;
	u16 pwm_l;
	u16 pwm_h;
}TopoInfo;
#endif
#endif

extern TimeValue NIDTimer;

#ifndef II_STA
enum Topo_enum{
	TOPO_IDEL,
	TOPO_START,
	TOPO_WAIT,
	TOPO_END,
};

void Open1Hz(void)
{
	PwmGpioFunc(TopoPWMTmr.Timer, 1);
	TopoPWMTmr.TimerSlot20ns = 100 * 1000000-1;
	PwmGpioFunc(TopoPWMTmr.Timer, 1);
	TimerPwmOpen(&TopoPWMTmr, 100 * 500000);
	TimerPwmWrite(TopoPWMTmr.Timer, TIMER_SET_START);
}
void Close1Hz(void)
{
	TimerPwmWrite(TopoPWMTmr.Timer, TIMER_SET_STOP);
	PwmGpioFunc(TopoPWMTmr.Timer, 0);
}

extern void TopoProc(void);
u8 TopoData[16]={0xa5,0xe9};
#ifndef NEW_TOPO
void StartTopo(u8 func,u8 cnt,u8 interval,u8 *data,u8 datalen)
{
	TopoInfo.start_topo=1;
	if (datalen>sizeof(TopoData))
	{
		return;
	}
	TopoInfo.bit_len=datalen*8;
	if (func == 1)
	{
		TopoInfo.Topo_Type=1;
		memcpy(TopoData,data,datalen);
	}
	else
	{
		TopoInfo.Topo_Type=0;
	}
	TopoInfo.send_cnt=cnt;
	if (TopoInfo.send_cnt==0)
	{
		TopoInfo.send_cnt=1;
	}
	TopoInfo.send_interval = interval;
	TopoTmr.TimerCallBackFunction=TopoProc;
	if (TopoInfo.topo_status==TOPO_IDEL)
	{
		TopoProc();
	}
}
void StopTopo(void)
{
	TopoInfo.start_topo=0;
}




void TopoProc(void)
{
	switch (TopoInfo.topo_status)
	{
	case TOPO_IDEL:
		if (TopoInfo.start_topo)
		{
			TopoInfo.topo_status=TOPO_START;
			TopoInfo.last_outbit=0;
			if (TopoInfo.Topo_Type)//1是需要PWM来输出
			{
				PwmGpioFunc(TopoPWMTmr.Timer, 1);
				TopoPWMTmr.TimerSlot20ns = 100 * 1200;
				TopoPWMTmr.TimerCallBackFunction = TopoCallBack;
			}
			else
			{
				PwmGpioFunc(TopoPWMTmr.Timer, 0);
				InsertTopoFuncOpen();
			}
			TimerWrite(TopoTmr.Timer, TIMER_SET_STOP);
			TimerClose(TopoTmr.Timer);
			TopoTmr.TimerSlot20ns = 50 * 100;
			TopoTmr.TimerCallBackFunction = TopoProc;
			TimerOpen(&TopoTmr);
			TimerWrite(TopoTmr.Timer, TIMER_SET_START);
	
			TopoInfo.send_num=0;
		}
		break;
	case TOPO_START:
		if (TopoInfo.start_topo==0)
		{
			TopoInfo.topo_status=TOPO_END;
			return;
		}
		if (TopoInfo.Topo_Type)//方案1
		{

			if (TopoData[TopoInfo.send_num >> 3] & (1 << (7 - (TopoInfo.send_num & 0x7))))
			{
				if (TopoInfo.last_outbit==0)
				{
					PwmGpioFunc(TopoPWMTmr.Timer, 1);
					TimerPwmOpen(&TopoPWMTmr, 100 * 400);
					TimerPwmWrite(TopoPWMTmr.Timer, TIMER_SET_START);
				}
				TopoInfo.last_outbit = 1;
			}
			else
			{
				TimerPwmReset(&TopoPWMTmr);
				PwmGpioFunc(TopoPWMTmr.Timer, 0);
				InsertTopoFuncOpen();
				InsertTopoFuncWrite(0);
				TopoInfo.last_outbit = 0;
			}
			TopoInfo.send_num++;
			if (TopoInfo.send_num > TopoInfo.bit_len)//stop send
			{
				TimerPwmReset(&TopoPWMTmr);
				PwmGpioFunc(TopoPWMTmr.Timer, 0);
				InsertTopoFuncOpen();
				InsertTopoFuncWrite(0);
				TopoInfo.topo_status = TOPO_END;
				TopoInfo.send_num = 0;
				TopoInfo.last_outbit = 0;
			}
			else 
			{
				TimerWrite(TopoTmr.Timer, TIMER_SET_STOP);
				TimerClose(TopoTmr.Timer);
				TopoTmr.TimerSlot20ns = 50 * 1000*120;//120ms
				TopoTmr.TimerCallBackFunction = TopoProc;
				TimerOpen(&TopoTmr);
				TimerWrite(TopoTmr.Timer, TIMER_SET_START);
			}

		}
		else//方案2
		{

			if (TopoInfo.send_num == 0)
			{
				InsertTopoFuncWrite(1);
				TimerWrite(TopoTmr.Timer, TIMER_SET_STOP);
				TimerClose(TopoTmr.Timer);
				TopoTmr.TimerSlot20ns = 50 * 1000*50;//50ms
				TopoTmr.TimerCallBackFunction = TopoProc;
				TimerOpen(&TopoTmr);
				TimerWrite(TopoTmr.Timer, TIMER_SET_START);
			}
			else
			{
				InsertTopoFuncWrite(0);
				TopoInfo.topo_status = TOPO_END;
			}
			TopoInfo.send_num++;

		}
		break;
	
	}
	if(TopoInfo.topo_status== TOPO_END)
	{
		if (--TopoInfo.send_cnt)
		{
			TopoInfo.sec_inc=0;
			TopoInfo.topo_status=TOPO_WAIT;
		}
		else
		{
			PwmGpioFunc(TopoPWMTmr.Timer, 0);
			InsertOpen();
			TimerPwmReset(&TopoPWMTmr);
			TimerWrite(TopoTmr.Timer, TIMER_SET_STOP);
			TimerClose(TopoTmr.Timer);
			TopoInfo.start_topo=0;
			TopoInfo.topo_status=TOPO_IDEL;
		}
	}
	if (TopoInfo.topo_status== TOPO_WAIT)
	{
		if (TopoInfo.sec_inc++ < TopoInfo.send_interval)
		{
			TimerWrite(TopoTmr.Timer, TIMER_SET_STOP);
			TimerClose(TopoTmr.Timer);
			TopoTmr.TimerSlot20ns = 50 * 1000*1000;//1S
			TopoTmr.TimerCallBackFunction = TopoProc;
			TimerOpen(&TopoTmr);
			TimerWrite(TopoTmr.Timer, TIMER_SET_START);
		}
		else
		{
			TimerWrite(TopoTmr.Timer, TIMER_SET_STOP);
			TimerClose(TopoTmr.Timer);
			TopoTmr.TimerSlot20ns = 50 * 1000;//1mS
			TopoTmr.TimerCallBackFunction = TopoProc;
			TimerOpen(&TopoTmr);
			TimerWrite(TopoTmr.Timer, TIMER_SET_START);
			TopoInfo.topo_status=TOPO_START;
			TopoInfo.send_num = 0;
		}
	}

}
#else


u32 alarm_topo_time;
u32 remain_tick;
void BeginTopo(void)
{
	alarm_topo_time = 0;
	TopoProc();
}
void StartTopo(u32 sec1900,u16 pwm_l, u16 pwm_h, u16 ms, u8 *data, u8 datalen)
{
	TopoInfo.start_topo=1;
	if (datalen>sizeof(TopoData))
	{
		return;
	}
	memcpy(TopoData,data,datalen);
	TopoInfo.bit_len=datalen*8;

	TopoInfo.bit_ms=ms;
	TopoInfo.pwm_l=pwm_l;
	TopoInfo.pwm_h=pwm_h;
	TopoTmr.TimerCallBackFunction=TopoProc;
	if (sec1900==0xffffffff)
	{
		TopoProc();
	}
	else
	{
		alarm_topo_time=sec1900;
	}
}

void StopTopo(void)
{
	TopoInfo.start_topo=0;
}




void TopoProc(void)
{
	remain_tick=180*1000;
	switch (TopoInfo.topo_status)
	{
	case TOPO_IDEL:
		if (TopoInfo.start_topo)
		{
			TopoInfo.topo_status=TOPO_START;
			TopoInfo.last_outbit=0;
			//需要PWM来输出
			
			PwmGpioFunc(TopoPWMTmr.Timer, 0);
			TopoPWMTmr.TimerSlot20ns = 100 * (TopoInfo.pwm_h+TopoInfo.pwm_l);//TopoPWMTmr.TimerSlot20ns = 50 * TopoInfo.pwm_h;// //总时间:PWM一个周期的时间
			TopoPWMTmr.TimerCallBackFunction = TopoCallBack;

			TimerWrite(TopoTmr.Timer, TIMER_SET_STOP);
			TimerClose(TopoTmr.Timer);
			TopoTmr.TimerSlot20ns = 50 * TopoInfo.bit_ms*1000;
			TopoTmr.TimerCallBackFunction = TopoProc;
	
			TopoInfo.send_num=0;
		}
		else
		{
			break;
		}
	case TOPO_START:
		if (TopoInfo.start_topo==0)
		{
			TopoInfo.topo_status=TOPO_END;
			return;
		}


		if (TopoData[TopoInfo.send_num >> 3] & (1 << (7 - (TopoInfo.send_num & 0x7))))
		{
			if (TopoInfo.last_outbit==0)
			{
				PwmGpioFunc(TopoPWMTmr.Timer, 1);
				TimerPwmOpen(&TopoPWMTmr, 100 * TopoInfo.pwm_h);//TimerPwmOpen(&TopoPWMTmr, 50 * TopoInfo.pwm_l);// //高电平时间:PWM一个周期内高电平的时间
				TimerPwmWrite(TopoPWMTmr.Timer, TIMER_SET_START);
			}
			TopoInfo.last_outbit = 1;
		}
		else
		{
			TimerPwmReset(&TopoPWMTmr);
			PwmGpioFunc(TopoPWMTmr.Timer, 0);
			InsertTopoFuncOpen();
			InsertTopoFuncWrite(0);
			TopoInfo.last_outbit = 0;
		}
		TopoInfo.send_num++;
		if (TopoInfo.send_num > TopoInfo.bit_len)//stop send
		{
			TimerPwmReset(&TopoPWMTmr);
			PwmGpioFunc(TopoPWMTmr.Timer, 0);
			InsertTopoFuncOpen();
			InsertTopoFuncWrite(0);
			TopoInfo.topo_status = TOPO_END;
			TopoInfo.send_num = 0;
			TopoInfo.last_outbit = 0;
		}
		else 
		{
			TimerWrite(TopoTmr.Timer, TIMER_SET_STOP);
			TimerClose(TopoTmr.Timer);
			TopoTmr.TimerCallBackFunction = TopoProc;
			TimerOpen(&TopoTmr);
			TimerWrite(TopoTmr.Timer, TIMER_SET_START);
		}

		break;
	
	}
	if(TopoInfo.topo_status== TOPO_END)
	{
		PwmGpioFunc(TopoPWMTmr.Timer, 0);
		InsertOpen();
		TimerPwmReset(&TopoPWMTmr);
		TimerWrite(TopoTmr.Timer, TIMER_SET_STOP);
		TimerClose(TopoTmr.Timer);
		TopoInfo.start_topo=0;
		TopoInfo.topo_status=TOPO_IDEL;
	}
}
#endif
#endif

typedef struct __PACKED
{
	u8 dir;                   //方向,0下行，1上行
    u32 isp_kep;              //厂商key
    u32 proj_kep;             //项目key
}CFG_ISP_PROJ_KEY_DATA;

void tool_test_callback(int status)
{
	debug_str(DEBUG_LOG_NET, "tool_test_callback %d\r\n",status);
}

void tool_test_OptionCallback(uint32_t IsOpen,uint32_t phase)
{
	debug_str(DEBUG_LOG_NET, "tool_test_OptionCallback %d %d\r\n",IsOpen,phase);
}
extern bool TestMode;
extern void send_callback(bool_t status);


void MpduHandleMPDU_EXT(PDU_BLOCK* pdu,u8 CommunicationType)
{
	MPDU_CTL *mpduCtl =  (MPDU_CTL *)pdu->pdu;
	BEACON_MPDU *beaconMpdu; 
	const ST_STA_ATTR_TYPE* pNetBasePara = GetStaBaseAttr();

	if (mpduCtl->Type == MPDU_TYPE_BEACON)
	{
		if(pdu->crcState == 0)
		{
			return;
		}

		//TODO 匹配nid和判断入网状态
		if (pNetBasePara->NID == mpduCtl->NID)
		{
			beaconMpdu = (BEACON_MPDU *)((u8*)mpduCtl + sizeof(MPDU_CTL));
			lc_net_set_NetBindFlg(((beaconMpdu->Reserve3 >> 8) & 0xFF));
		}
	}
	else if (mpduCtl->Type == MPDU_TYPE_SOF)
	{
		if(pdu->crcState == 0)
		{
			return;
		}
        MAC_HEAD_LONG  *macLongHead = (MAC_HEAD_LONG*)(pdu->pdu + 16 + 4);
		MSDU_HEAD_LONG *msduLongHead = (MSDU_HEAD_LONG*)((u8*)macLongHead + sizeof(MAC_HEAD_LONG));
		MME_FRAME *mmeFrame = (MME_FRAME*)((u8*)msduLongHead + sizeof(MSDU_HEAD_LONG));
		// 判断长MPDU 判断是否是关联请求
		if((macLongHead->MacHead.HeadType == 0) && (pNetBasePara->NID == mpduCtl->NID))
		{
			if(mmeFrame->MMType == MME_CONFIRM_ISP_PROJ_KEY_REQUEST)
			{
				_CRC_32_ FCcrc32 = GetCrc32((uint8_t *)msduLongHead ,macLongHead->MacHead.MsduLen);
				_CRC_32_ FCcrc32_tmp = {0};
				memcpy(&FCcrc32_tmp,(u8*)mmeFrame + sizeof(MME_FRAME)+sizeof(CFG_ISP_PROJ_KEY_DATA),4);
				if((memcmp(&FCcrc32,&FCcrc32_tmp,4)==0) && (memcmp(msduLongHead->DstMac,pNetBasePara->Mac,6) ==0))
				{
					CFG_ISP_PROJ_KEY_DATA* pcfgispprojkey = (CFG_ISP_PROJ_KEY_DATA*)((u8*)mmeFrame + sizeof(MME_FRAME));
					lc_net_write_factory_id_code(pcfgispprojkey->isp_kep);
					lc_net_write_project_id_code(pcfgispprojkey->proj_kep);
					//鍥炲
					u16 dsttei =  macLongHead->MacHead.SrcTei;
					u16 srctei = macLongHead->MacHead.DstTei;
					macLongHead->MacHead.DstTei = dsttei;
					macLongHead->MacHead.SrcTei = srctei;
					memcpy(macLongHead->DstMac,msduLongHead->SrcMac,LC_NET_ADDR_SIZE);
					memcpy(msduLongHead->DstMac,msduLongHead->SrcMac,LC_NET_ADDR_SIZE);
					memcpy(msduLongHead->SrcMac,pNetBasePara->Mac,LC_NET_ADDR_SIZE);
					pcfgispprojkey->dir = 1;
					mmeFrame->MMType = MME_CONFIRM_ISP_PROJ_KEY_RESPONSE;
					macLongHead->MacHead.MsduLen = sizeof(MSDU_HEAD_LONG)+sizeof(MME_FRAME)+sizeof(CFG_ISP_PROJ_KEY_DATA);
					FCcrc32 = GetCrc32((uint8_t *)msduLongHead ,macLongHead->MacHead.MsduLen);
					memcpy((u8*)mmeFrame + sizeof(MME_FRAME)+sizeof(CFG_ISP_PROJ_KEY_DATA), &FCcrc32, 4);
					_CRC_24_ FCcrc24 = GetCrc24(pdu->pdu + 16,133);
					memcpy(pdu->pdu+16+4+128+1, &FCcrc24, 3);
					BPLC_SendImmediate(pdu->pdu,152,NULL,NULL,1);
					return ;
				}
			}
		}
		//娴嬭瘯妯″紡涓嬶紝骞朵笖甯ч暱涓?52
		if((pdu->size != 136+16)&&(TestMode != true))
		{
			return ;	
		}
		//濡傛灉涓烘祴璇曞抚0x10FF灏嗗鐞?
		//debug_str(DEBUG_LOG_NET, "type = %04x\r\n",mmeFrame->MMType);
		if(mmeFrame->MMType == 0x10FF)
		{
			uint8_t  *toolTestData = ((uint8_t*)(mmeFrame)) + sizeof(MME_FRAME);
			_CRC_32_ FCcrc32 = GetCrc32((uint8_t *)msduLongHead ,macLongHead->MacHead.MsduLen);
			_CRC_32_ FCcrc32_tmp = {0};
			memcpy(&FCcrc32_tmp,(u8*)mmeFrame + sizeof(MME_FRAME)+LC_NET_ADDR_SIZE,4);
			if(0 == memcmp(toolTestData,pNetBasePara->Mac,LC_NET_ADDR_SIZE) && 0 == memcmp(&FCcrc32,&FCcrc32_tmp,4))
			{
				debug_str(DEBUG_LOG_NET, "Recv 0x10FF tool test\r\n");
				//mac6 +r4
				u32 rssi = 0;
				u32 snr = 0;
				if(CommunicationType == RECEIVE_HRF_PL_PDU)
				{
					rssi = pdu->rxPara.hrf_rssi;
					snr = pdu->rxPara.hrf_snr;
				}
				else
				{
					rssi = pdu->rxPara.rssi;
					snr = pdu->rxPara.snr;
				}
				memcpy(toolTestData+10,&rssi,4);
				memcpy(toolTestData+14,&snr,4);//18
				//
				macLongHead->MacHead.MsduLen = sizeof(MSDU_HEAD_LONG)+sizeof(MME_FRAME)+18;
				//
				_CRC_32_ FCcrc32 = GetCrc32((uint8_t *)msduLongHead ,macLongHead->MacHead.MsduLen);
				memcpy(toolTestData+18, &FCcrc32, 4);

				_CRC_24_ FCcrc24 = GetCrc24(pdu->pdu + 16,133);
				memcpy(pdu->pdu+16+4+128+1, &FCcrc24, 3);
				
				//RF閫氶亾
				if(CommunicationType == RECEIVE_HRF_PL_PDU)
				{
					SOF_HRF_ZONE *sof = (SOF_HRF_ZONE *)mpduCtl->Zone0;
					HRF_SendImmediate(2, sof->MCS,pdu->pdu,sof->PbSize,send_callback);
					debug_str(DEBUG_LOG_NET, "send RECEIVE_HRF_PL_PDU ok\r\n");
				}
				//杞芥尝閫氶亾
				else
				{
					BPLC_SendImmediate(pdu->pdu,152,tool_test_callback,tool_test_OptionCallback,1);
					debug_str(DEBUG_LOG_NET, "send RECEIVE_PLC_PL_PDU ok\r\n");
				}
			}
			
		}
	}
}
extern bool MpduHandleMPDU_Diagnosis(PDU_BLOCK* pdu,u8 recvtype);

//plc协议任务
void HPLC_Task(void* arg)
{
    OS_ERR err;
    PDU_BLOCK* pdu = 0;
    OS_MSG_SIZE msgSize;
	const u32 PlcQueueWait = 5/(1000/OSCfg_TickRate_Hz);
	DeviceResetOpen(Reset_CallBack);
    while(1)
    {
		pdu = OSQPend (&PlcDataQueue,PlcQueueWait,OS_OPT_PEND_BLOCKING,&msgSize,0,&err);
		if(err == OS_ERR_NONE)
        {
            switch(pdu->type)
			{
				case RECEIVE_HRF_FC_PDU:            //hrf 和HPLC
				case RECEIVE_FC_PDU:
				{
					MpduHandleFC(pdu,msgSize);		//处理FC
					break;
				}
				case RECEIVE_HRF_PL_PDU:
				case RECEIVE_PL_PDU:
				{
					MpduHandleMPDU_Diagnosis(pdu,pdu->type);
					MpduHandleMPDU_EXT(pdu,pdu->type);
					MpduHandleMPDU(pdu);	//处理物理块
					break;
				}
				default:break;
			}
            FreePDU(pdu);
        }
		else if(GetAppCurrentModel() == APP_TEST_NORMAL)//没有得到FC时判断是否已经超过一个网络的请求时间(需要在正常模式下
		{
			if(GetStaBaseAttr()->NetState != NET_IN_NET)
			{			
				if(AttemptNet.VaildNID)
				{
                    if(TimeOut(&NIDTimer)){
                        SetNIDInvaild(AttemptNet.AttemptNID, 15 * 1000ul, 0);
                        ClearAttemptNet();
                        RstLastChannelIndex();
                        continue;
                    }
				}
			}
			HrfSearchProc();
		}
        HPLC_RunTmr();
		remRunTime();
		Reset_Proc();
    }
}

u8 GetGraphCs(Graph_Data_s *pData)
{
	u8 *p=(u8 *)pData;
	u8 len = pData->len+sizeof(Graph_Data_s);
	u8 sum=0;
	if (len>180)//len有问题
	{
		return ~pData->cs+1;
	}
	for (int i = 0; i < len; i++)
	{
		if (p != &pData->cs)
		{
			sum+=*p++;
		}
        else
        {
            p++;
        }
	}
	return sum;
}
extern u8 MeterProtocol;
u8 getGraphOk;
void readGraphCallback(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8* data, u16 len, u8 sendType, u16 srcTei)
{
	Graph_Data_s *pData;
	OS_ERR err;
	getGraphOk=0;
	if (MeterProtocol == LOCAL_PRO_DLT645_1997 || MeterProtocol == LOCAL_PRO_DLT645_2007)
	{
		if (Is645Frame(data, len))
		{
			u8 *p = data;
			for (int index=0; index<len; index++)
			{
				if (*p == 0x68)
				{
					break;
				}
				++p;
			}
			
			len = p[9];
			if (len <= 4)
			{
				//从站异常应答帧数据域长度为1字节
				debug_str(DEBUG_LOG_APP, "geted 645 data err, len:%d\r\n", len);
				getGraphOk=1;
				OSTaskSemPost(&App_GraphTCB, OS_OPT_POST_1, &err);
				return;
			}
			
			pData = (Graph_Data_s*)&p[4];//Graph_Data_s data前字节数与645 差4个字节
			for (int index=0; index<len; index++)
			{
				p[index+10] -= 0x33;
			}
			
			u32 di = *(u32*)&p[10];
			u8 idxa = 0, idxb = 0;
			if (false == Find_ID_Meter(di, &idxa, &idxb)) //DI匹配过滤
			{
				debug_str(DEBUG_LOG_APP, "geted 645 data err, ident:%08x, len:%d\r\n", di, len);
				OSTaskSemPost(&App_GraphTCB, OS_OPT_POST_1, &err);
				return;
			}
			
			pData->ident = di;
			pData->sec_from_1900 = Data2Sec(&Time_Module[0].time);
			pData->len = len-4;
			pData->cs = GetGraphCs(pData);
			debug_str(DEBUG_LOG_APP, "geted 645 data ident:%08x, data len:%d\r\n", di, pData->len);
			StorageGraphData(pData);
			getGraphOk=1;
			OSTaskSemPost(&App_GraphTCB, OS_OPT_POST_1, &err);
		}
	}
	else if (MeterProtocol == LOCAL_PRO_698_45)
	{
		if (Is698Frame(data, len))
		{
			OSTaskSemPost(&App_GraphTCB, OS_OPT_POST_1, &err);
		}
	}
}
Time_Special meter_time;
void readMeterTimeCallback(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8* data, u16 len, u8 sendType, u16 srcTei)
{
	OS_ERR err;
	if (MeterProtocol == LOCAL_PRO_DLT645_1997 || MeterProtocol == LOCAL_PRO_DLT645_2007)
	{
		if (Is645Frame(data, len))
		{
			u8 *p = data;
			for (int index = 0; index < len; index++)
			{
				if (*p == 0x68)
				{
					break;
				}
				++p;
			}
			len = p[9];
			for (int index = 0; index < len; index++)
			{
				p[index + 10] -= 0x33;
			}
			u32 di = *(u32 *)&p[10];
			if (di == 0x04000101) //年月日星期
			{
				meter_time.tm_isdst = 0;
				meter_time.tm_year = 2000 + bcd_to_dec(p[17]) - 1900;
				meter_time.tm_mon = bcd_to_dec(p[16]) - 1;
				meter_time.tm_mday = bcd_to_dec(p[15]);
				OSTaskSemPost(&App_GraphTCB, OS_OPT_POST_1, &err);
			}
			else if (di == 0x04000102) //时分秒
			{
				meter_time.tm_isdst = 0;
				meter_time.tm_hour = bcd_to_dec(p[16]);
				meter_time.tm_min = bcd_to_dec(p[15]);
				meter_time.tm_sec = bcd_to_dec(p[14]);
				OSTaskSemPost(&App_GraphTCB, OS_OPT_POST_1, &err);
			}
		}
	}
	else if (Is698Frame(data, len))
	{
		debug_str(DEBUG_LOG_APP, "not get 645 data\r\n");

		u8 *p = data;
		for (int index = 0; index < len; index++)
		{
			if (*p == 0x68)
			{
				break;
			}
			++p;
		}

		len = (p[4] & 0xf) + 1; //地址域长度
		p += len + 8; //得到数据起始长度
		if (p[0] != 133 || p[1] != 1) //只读取一个对象
		{
			return;
		}
		if (*(u32 *)&p[3] != 0x00020040) //IO
		{
			return;
		}
		if (p[8] != 28) //date_time_s
		{
			return;
		}
		u16 year = p[9] << 8 | p[10];
		meter_time.tm_isdst = 0;
		meter_time.tm_year = year - 1900;
		meter_time.tm_mon = p[11]-1;
		meter_time.tm_mday = p[12];
		meter_time.tm_hour = p[13];
		meter_time.tm_min = p[14];
		meter_time.tm_sec = p[15];
		OSTaskSemPost(&App_GraphTCB, OS_OPT_POST_1, &err);
	}

}
void readSupportCallback(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8* data, u16 len, u8 sendType, u16 srcTei)
{
	if (MeterProtocol == LOCAL_PRO_DLT645_1997 || MeterProtocol == LOCAL_PRO_DLT645_2007)
	{
		if (Is645Frame(data, len))
		{
			u8 *p = data;
			for (int index = 0; index < len; index++)
			{
				if (*p == 0x68)
				{
					break;
				}
				++p;
			}
			len = p[9];
			for (int index = 0; index < len; index++)
			{
				p[index + 10] -= 0x33;
			}
			if (p[9]==0xD1)//电表返回异常应答帧
			{
				Meter_SupportRecord=0;
			}
			u32 di = *(u32 *)&p[10];
			if (di == 0x04000A02) //得到了记录时间
			{
				Meter_SupportRecord=1;
			}
		}
	}
}

u8 shenhua_timing = 0; //深化应用精准校时
u32 NoGraphTime=0;//无需读取时间
extern Time_Special sta_broad_time;
void HPLC_GraphTask(void *arg)
{
	static u8 flag = 1;
	OS_ERR err;
	for (;;)
	{
		
		OSTimeDly(500, OS_OPT_TIME_DLY, &err);
		u32 current = OSTimeGet(&err);
#ifndef II_STA
		#ifdef NEW_TOPO
		if (remain_tick)
		{
			if (remain_tick > 500)
				remain_tick -= 500;
			else
				remain_tick = 0;
		}
		else  if (alarm_topo_time != 0
				  && alarm_topo_time != 0xffffffff
				  && alarm_topo_time < Time_Module[0].sec_form_1900
				 )
		{
			BeginTopo();
		}
		#endif
#endif
		if (Time_Module[0].isVaild)		
		{
			u32 sub=current - Time_Module[0].sysTick;
			if (sub > OS_CFG_TICK_RATE_HZ&&sub<0x3fffffff) //需要增加秒数
			{
				sub/=OS_CFG_TICK_RATE_HZ;//转为s  每次增量由原来的1s转为增加最大到58s 防止南网  抄读曲线后精准校时出现偏差较大的问题
				if (Time_Module[0].time.tm_sec+sub>58)//超过了58s
				{
					sub=1;
				}
				else
				{
//					sub=58-Time_Module[0].time.tm_sec;
				}
				Time_Module[0].sysTick += OS_CFG_TICK_RATE_HZ*sub;
				DateSpecialChangeSec(&Time_Module[0].time, sub);
				Time_Module[0].sec_form_1900 = Data2Sec(&Time_Module[0].time);
			}
			if(Time_Module[0].time.tm_sec == 0 &&flag)
			{
				flag = 0;
				debug_str(DEBUG_LOG_APP, "time module value %d-%d-%d %d:%d:%d\r\n", 
		      			  Time_Module[0].time.tm_year+1900, Time_Module[0].time.tm_mon+1, Time_Module[0].time.tm_mday, Time_Module[0].time.tm_hour, Time_Module[0].time.tm_min,Time_Module[0].time.tm_sec);
			}
			else
			{
				flag=1;
			}
			if (Time_Module[0].sec_form_1900<NoGraphTime)//之前的数据已经得到无需读取
			{
				continue ;
			}
			if (Graph_Config.config_flag  
				/*&&Meter_SupportRecord==0*/)//电表不支持记录数据
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
					continue ;
				}

				u8 read_data_flag = 0;
				
				for (int i = 0; i < Graph_Config.num[meter_sn]; i++)
				{
					if(Graph_Config.mode[meter_sn][i])
					{
						if (((Time_Module[0].time.tm_min % Graph_Config.period[meter_sn][i]) == 0) && (Time_Module[0].time.tm_sec == 0))
						{
							for (int rd=0;rd<2;rd++)
							{
								
							
								if (UartBusy())
								{
									OSTimeDly(500, OS_OPT_TIME_DLY, &err);
								}
								if (MeterProtocol == LOCAL_PRO_DLT645_1997 || MeterProtocol == LOCAL_PRO_DLT645_2007)
								{
									u8 frame[25];
									u8 len;
									const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
									u8 addr[LONG_ADDR_LEN];
									
									memcpy(addr, pNetBasePara->Mac, 6);
									TransposeAddr(addr);
									debug_str(DEBUG_LOG_APP,"read 645 data ident is %08x\r\n", Graph_Config.ident[meter_sn][i]);
									len = Get645_07DataFrame(frame, addr, Graph_Config.ident[meter_sn][i]);

									read_data_flag = 1;
									AppLocalReceiveData(addr, addr, 10, 0, frame, len, readGraphCallback, 0, 0, BAUD_SEARCH);
									OSTaskSemPend(2000,
												  OS_OPT_PEND_BLOCKING,
												  NULL,
												  &err);
									if (err == OS_ERR_NONE&&getGraphOk)
									{
										break;
									}
									OSTimeDlyHMSM(0, 0, 0, 500, OS_OPT_TIME_DLY, &err);
								}
								else//698.45
								{

								}
							}
							//延时500毫秒
							OSTimeDlyHMSM(0, 0, 0, 500, OS_OPT_TIME_DLY, &err);

						}
					}
				}

				if (read_data_flag) //读取过曲线
				{
					continue ;//读完表数据需要重新计算时间  才能继续
			}
			}
			
		}
		
		static u32 goto_time = 0;
		static u32 next_time=0;
		if (IsBindLocalMac()&&GetAppCurrentModel() == APP_TEST_NORMAL) //以获得表地址
#if 1		
		{
			goto_time++;
			OSSemPend(&BroadTimeSem, 0, OS_OPT_PEND_NON_BLOCKING, NULL, &err);
			if (err != OS_ERR_NONE)
			{
				if (Time_Module[0].isVaild != 0
					&& goto_time < (60 * 60 * 2)) //1小时
				{
					continue;
				}
				else if (goto_time<next_time)
				{
					continue;
				}
				else
				{
					if (Graph_Config.config_flag == 0)
					{
						goto_time = 0;
						continue; //曲线采集参数没有配置
					}
				}
			}
			goto_time = 0;
			next_time = goto_time+5*60;

			u8 frame[32];
			u8 len;
			const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
			u8 addr[LONG_ADDR_LEN];
			memcpy(addr, pNetBasePara->Mac, 6);
			TransposeAddr(addr);
#ifndef PROTOCOL_NW_2021
			//读年月日
			if (MeterProtocol == LOCAL_PRO_DLT645_1997 || MeterProtocol == LOCAL_PRO_DLT645_2007)
			{
				len = Get645_07DataFrame(frame, addr, 0x04000101);
			}
			else
			{
				//698 
				len = Get698_TimeFrame(frame, addr);
			}
			AppLocalReceiveData(addr,addr, 8, 0, frame, len, readMeterTimeCallback, 0, 0, BAUD_SEARCH);
			OSTaskSemPend(1000,
						  OS_OPT_PEND_BLOCKING,
						  NULL,
						  &err);
			if (err != OS_ERR_NONE)
			{
				shenhua_timing = 0;
				continue;
			}
			//读时分秒
			if (MeterProtocol == LOCAL_PRO_DLT645_1997 || MeterProtocol == LOCAL_PRO_DLT645_2007)
			{
				len = Get645_07DataFrame(frame, addr, 0x04000102);
				AppLocalReceiveData(addr,addr, 8, 0, frame, len, readMeterTimeCallback, 0, 0, BAUD_SEARCH);
				OSTaskSemPend(1000,
							  OS_OPT_PEND_BLOCKING,
							  NULL,
							  &err);
				if (err != OS_ERR_NONE)
				{
					shenhua_timing = 0;
					continue;
				}
			}
			debug_str(DEBUG_LOG_APP, "adjust time value %d-%d-%d %d:%d, meter time value %d-%d-%d %d:%d, shenhua_timing:%d\r\n",
					  Time_Module[0].time.tm_year+1900, Time_Module[0].time.tm_mon+1, Time_Module[0].time.tm_mday, Time_Module[0].time.tm_hour, Time_Module[0].time.tm_min,
					  meter_time.tm_year+1900, meter_time.tm_mon+1, meter_time.tm_mday, meter_time.tm_hour, meter_time.tm_min, shenhua_timing);
#endif
			if (shenhua_timing )//进行广播校时
			{
#ifndef PROTOCOL_NW_2021
				sta_broad_time=meter_time;
				int diff = Diff_Specialtime(&Time_Module[0].time, &meter_time);
				time_t sec_forme_1900;
				if (diff<(-5*60))
				{
					sec_forme_1900 = mktime(&meter_time);
					sec_forme_1900 -=4*60+60/2;//电表时间向正向调整4.5min
					meter_time = *gmtime(&sec_forme_1900);
				}
				else if (diff>(5*60))
				{
					sec_forme_1900 = mktime(&meter_time);
					sec_forme_1900 +=4*60+60/2;//电表时间向负向调整4.5min
					meter_time = *gmtime(&sec_forme_1900);
				}
				else//在±5min以内
				{
					meter_time = Time_Module[0].time;
				}
				//发送广播校时帧
				if (MeterProtocol==LOCAL_PRO_DLT645_2007||MeterProtocol==LOCAL_PRO_DLT645_1997)
				{
					len=Get645BroadTime(frame, &meter_time);
				}
				else if (MeterProtocol==LOCAL_PRO_698_45)
				{
					len=Get698BroadTime(frame, &meter_time);
				}
#else
				len=Get645BroadTime(frame, &Time_Module[0].time);
#endif
				memset(addr, 0x99, LONG_ADDR_LEN); //规避没收到回复NoAckCount++
				AppLocalReceiveData(addr,addr, 8, 0, frame, len, NULL, 0, 0, BAUD_SEARCH);
				
				meter_time.tm_isdst = 0;
				//Time_Module[0].time = meter_time;

				
				//精准校时时间和当前电表时间的偏差
				Time_Module[1].isVaild = 1;
				Time_Module[1].diff_tick = Diff_Specialtime(&Time_Module[0].time, &meter_time);

				StorageTime();
				
				shenhua_timing = 0;
				continue;
			}
			#ifndef PROTOCOL_NW_2021
			//当前电表时间加上偏差
			if (Time_Module[0].isVaild == 0) //时间无效
			{
				if (Time_Module[1].isVaild) //偏差有效
				{
					Time_Module[0].isVaild = 3; //校准表时间
					Time_Module[0].time = meter_time;
					DateSpecialChangeSec(&Time_Module[0].time, Time_Module[1].diff_tick);
					Time_Module[0].sysTick = OSTimeGet(&err);
				}
				else
				{
					Time_Module[0].isVaild = 2; //表时间
					Time_Module[0].time = meter_time;
					Time_Module[0].sec_form_1900 = Data2Sec(&Time_Module[0].time);
					Time_Module[0].sysTick = OSTimeGet(&err);
				}
			}
		
			StorageTime();
#endif			
		}
#else		
		{
			goto_time++;
			OSSemPend(&BroadTimeSem, 0, OS_OPT_PEND_NON_BLOCKING, NULL, &err);
			if (err != OS_ERR_NONE)
			{
				if (Time_Module[0].isVaild != 0
					&& goto_time < (60 * 60 * 2)) //1小时
				{
					continue;
				}
				else if (goto_time<next_time)
				{
					continue;
				}
			}
			goto_time = 0;
			next_time=goto_time+5*60;

			u8 frame[32];
			u8 len;
			const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
			u8 addr[LONG_ADDR_LEN];
			memcpy(addr, pNetBasePara->Mac, 6);
			TransposeAddr(addr);

#if 0
			if (Meter_SupportRecord==0xff)
			{
				len = Get645_07DataFrame(frame, addr, 0x04000A02);//读取第一类负荷记录间隔时间
				AppLocalReceiveData(addr, addr, 8, 0, frame, len, readSupportCallback, 0, 0, BAUD_SEARCH);
				OSTaskSemPend(1000,
							  OS_OPT_PEND_BLOCKING,
							  NULL,
							  &err);
			}
#endif

			//读年月日
			if (MeterProtocol == LOCAL_PRO_DLT645_1997 || MeterProtocol == LOCAL_PRO_DLT645_2007)
			{
				len = Get645_07DataFrame(frame, addr, 0x04000101);
			}
			else
			{
				//698 
				len = Get698_TimeFrame(frame, addr);
			}
			AppLocalReceiveData(addr,addr, 8, 0, frame, len, readMeterTimeCallback, 0, 0, BAUD_SEARCH);
			OSTaskSemPend(1000,
						  OS_OPT_PEND_BLOCKING,
						  NULL,
						  &err);
			if (err != OS_ERR_NONE)
			{
				continue;
			}
			//读时分秒
			if (MeterProtocol == LOCAL_PRO_DLT645_1997 || MeterProtocol == LOCAL_PRO_DLT645_2007)
			{
				len = Get645_07DataFrame(frame, addr, 0x04000102);
				AppLocalReceiveData(addr,addr, 8, 0, frame, len, readMeterTimeCallback, 0, 0, BAUD_SEARCH);
				OSTaskSemPend(1000,
							  OS_OPT_PEND_BLOCKING,
							  NULL,
							  &err);
				if (err != OS_ERR_NONE)
				{
					continue;
				}
			}
			debug_str(DEBUG_LOG_APP, "adjust time value %d-%d-%d %d:%d, meter time value %d-%d-%d %d:%d\r\n",
					  Time_Module[0].time.tm_year+1900, Time_Module[0].time.tm_mon+1, Time_Module[0].time.tm_mday, Time_Module[0].time.tm_hour, Time_Module[0].time.tm_min,
					  meter_time.tm_year+1900, meter_time.tm_mon+1, meter_time.tm_mday, meter_time.tm_hour, meter_time.tm_min);

			//if (err != OS_ERR_NONE)//进行广播校时
			{
				int diff = Diff_Specialtime(&Time_Module[0].time, &meter_time);
				time_t sec_forme_1900;
				if (diff<(-5*60))
				{
					sec_forme_1900 = mktime(&meter_time);
					sec_forme_1900 +=4*60+60/2;//电表时间向正向调整4.5min
					meter_time=*gmtime(&sec_forme_1900);

				}
				else if (diff>(5*60))
				{
					sec_forme_1900 = mktime(&meter_time);
					sec_forme_1900 -=4*60+60/2;//电表时间向负向调整4.5min
					meter_time=*gmtime(&sec_forme_1900);
				}
				else//在±5min以内
				{
					meter_time=Time_Module[0].time;
				}
				//发送广播校时帧
				if (MeterProtocol==LOCAL_PRO_DLT645_2007||MeterProtocol==LOCAL_PRO_DLT645_1997)
				{
					len=Get645BroadTime(frame, &meter_time);
				}
				else if (MeterProtocol==LOCAL_PRO_698_45)
				{
					len=Get698BroadTime(frame, &meter_time);
				}
				AppLocalReceiveData(addr,addr, 8, 0, frame, len, NULL, 0, 0, BAUD_SEARCH);
				meter_time.tm_isdst=0;
				Time_Module[0].time=meter_time;
				continue;
			}
#if 0
			if (DffSecIn24H(&meter_time, &Time_Module[2].time))
			{
				if (Time_Module[0].isVaild == 0) //时间无效
				{
					if (Time_Module[1].isVaild) //偏差有效
					{
						Time_Module[0].isVaild = 3; //校准表时间
						Time_Module[0].time = meter_time;
						DateSpecialChangeSec(&Time_Module[0].time,Time_Module[1].diff_tick);
						Time_Module[0].sysTick = OSTimeGet(&err);
					}
					else
					{
						Time_Module[0].isVaild = 2; //表时间
					}
				}
				if (Time_Module[0].isVaild < 3) //不是准确时间
				{
					Time_Module[0].time = meter_time;
					Time_Module[0].sysTick = OSTimeGet(&err);
					Time_Module[0].sec_form_1900 = Data2Sec(&Time_Module[0].time);
				}
				Time_Module[2].time = meter_time; //更新表时间
				if (Time_Module[0].isVaild > 2) //需要更新偏差
				{
					Time_Module[1].diff_tick = Diff_Specialtime(&Time_Module[0].time,&meter_time);
					Time_Module[1].isVaild = 1;
				}
			}
			else
			{
				memset(&Time_Module[1], 0, sizeof(Time_Module_s));
				Time_Module[2].time = meter_time; //更新表时间
				Time_Module[0].time = meter_time;
				Time_Module[0].sysTick = OSTimeGet(&err);
				Time_Module[0].sec_form_1900 = Data2Sec(&Time_Module[0].time);
				Time_Module[1].isVaild = 0; //偏差无效
				Time_Module[0].isVaild = 1; //读到但无效的表差
			}
			StorageTime();
#else
			meter_time.tm_isdst=0;
			int diff = Diff_Specialtime(&Time_Module[0].time, &meter_time);
			if (diff>(5*60)||diff<(-5*60))
			{
				Time_Module[0].time = meter_time;
			}
#endif

		}
#endif
	}
}

void HPLC_II_TimingTask(void *arg)
{
	static u8 flag = 1;
	OS_ERR err;
	
	for (;;)
	{
		u32 current = OSTimeGet(&err);
		OSTimeDly(500, OS_OPT_TIME_DLY, &err);
		if (Time_Module[0].isVaild)		
		{
			u32 sub = current - Time_Module[0].sysTick;
			if ((sub > OS_CFG_TICK_RATE_HZ) && (sub < 0x3fffffff)) //需要增加秒数
			{
				Time_Module[0].sysTick += OS_CFG_TICK_RATE_HZ;
				DateSpecialChangeSec(&Time_Module[0].time, 1);
				Time_Module[0].sec_form_1900 = Data2Sec(&Time_Module[0].time);
			}
			
			if(Time_Module[0].time.tm_sec == 0 &&flag)
			{
				flag = 0;
				debug_str(DEBUG_LOG_APP, "time module value %d-%d-%d %d:%d:%d\r\n", 
		      			  Time_Module[0].time.tm_year+1900, Time_Module[0].time.tm_mon+1, Time_Module[0].time.tm_mday, Time_Module[0].time.tm_hour, Time_Module[0].time.tm_min,Time_Module[0].time.tm_sec);
			}
			else
			{
				flag = 1;
			}
			if(Time_Module[0].isVaild == 4)
			{
				lc_iot_calibrate_date_cb_fun(Time_Module[0].sec_form_1900 - 946656000 - (8 * 60 * 60));//seconds from 2000 in beijing timezone
			}
		}

		if (GetAppCurrentModel() == APP_TEST_NORMAL) 
		{
			OSSemPend(&BroadTimeSem, 0, OS_OPT_PEND_NON_BLOCKING, NULL, &err);
			if (err != OS_ERR_NONE)
			{
				continue;
			}

			debug_str(DEBUG_LOG_APP, "adjust time value %d-%d-%d %d:%d, shenhua_timing:%d\r\n",
					  Time_Module[0].time.tm_year+1900, Time_Module[0].time.tm_mon+1, Time_Module[0].time.tm_mday, Time_Module[0].time.tm_hour, Time_Module[0].time.tm_min, shenhua_timing);

			if (shenhua_timing )//进行广播校时
			{
				u8 frame[32] = {0};
				u8 frame_len = 0;
				u32 sec_forme_1900;
				Time_Special adjust_time;
				u8 addr[LONG_ADDR_LEN] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99}; //避免NoAckCount++
				
				for (u8 i=0; i<11; i++)
				{
					sec_forme_1900 = mktime(&Time_Module[0].time);

					//T, T+-5min, T+-10min, T+-15min, T+-20min, T+-25min
					if (i != 0)
					{
						if (i%2)
						{
							sec_forme_1900 -= (i+1)/2*5*60;
						}
						else
						{
							sec_forme_1900 += (i+1)/2*5*60;
						}
					}

					adjust_time = *gmtime(&sec_forme_1900);
					
					//发送广播校时帧
					frame_len = Get645BroadTime(frame, &adjust_time);
					AppLocalReceiveData(addr, addr, 8, 0, frame, frame_len, NULL, 0, 0, BAUD_SEARCH);

					//延时2秒
					OSTimeDlyHMSM(0, 0, 2, 0, OS_OPT_TIME_DLY, &err);
				}
								
				StorageTime();

				shenhua_timing = 0;
			}
		}
	}
}


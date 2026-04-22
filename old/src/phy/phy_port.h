#ifndef _PHY_PORT_H
#define _PHY_PORT_H
#include <gsmcuxx_hal_def.h>

#if defined(ZB204_CHIP)
#include "ZB204.h"
#elif defined(ZB205_CHIP)
#include "ZB205.h"
#endif



#ifdef __cplusplus
extern  "C" {
#endif

extern const uint32_t PHYAUsedSCTable[4][16];

extern const uint16_t Tone_Phase_Tab_SG[128];
extern const uint16_t Tone_Phase_Tab_NG[128];
extern const uint16_t Preamb_Tone_Phase_Tab_SG[128];
extern const uint16_t Preamb_Tone_Phase_Tab_NG[128];
extern const uint8_t OffsetTable_QPSK_I_SG[8];
extern const uint8_t OffsetTable_QPSK_I_NG[12];

extern const uint16_t* Preamb_Tone_Phase_Tab[2];
extern const uint16_t* Tone_Phase_Tab[2];
extern uint8_t InterNumPerGroupTab[6];
extern const int16_t tx_band0_bpf_gain[6];
extern const int16_t tx_band0_bpf_a1[6];
extern const int16_t tx_band0_bpf_a2[6];
extern const int16_t tx_band1_bpf_gain[6];
extern const int16_t tx_band1_bpf_a1[6];
extern const int16_t tx_band1_bpf_a2[6];
extern const int16_t tx_band2_bpf_gain[6];
extern const int16_t tx_band2_bpf_a1[6];
extern const int16_t tx_band2_bpf_a2[6];
extern const int16_t tx_band3_bpf_gain[6];
extern const int16_t tx_band3_bpf_a1[6];
extern const int16_t tx_band3_bpf_a2[6];
extern const int16_t pulse_rm_cfg_a1_band2[8];
extern const int16_t pulse_rm_cfg_b1_band2[8];
extern const int16_t pulse_rm_cfg_b2_band2[8];
extern const int16_t pulse_rm_cfg_a1_band1[8];
extern const int16_t pulse_rm_cfg_b1_band1[8];
extern const int16_t pulse_rm_cfg_b2_band1[8];
extern const int16_t tx_band0_comp_phase[512];
extern const int16_t tx_band1_comp_phase[512];
extern const int16_t tx_band2_comp_phase[512];
extern const int16_t tx_band3_comp_phase[512];
extern const int16_t rx_band0_bpf_gain[6];
extern const int16_t rx_band0_bpf_a1[6];
extern const int16_t rx_band0_bpf_a2[6];
extern const int16_t rx_band1_bpf_gain[6];
extern const int16_t rx_band1_bpf_a1[6];
extern const int16_t rx_band1_bpf_a2[6];
extern const int16_t rx_band2_bpf_gain[6];
extern const int16_t rx_band2_bpf_a1[6];
extern const int16_t rx_band2_bpf_a2[6];
extern const int16_t rx_band3_bpf_gain[6];
extern const int16_t rx_band3_bpf_a1[6];
extern const int16_t rx_band3_bpf_a2[6];
extern const int16_t rx_band0_comp_phase[512];
extern const int16_t rx_band1_comp_phase[512];
extern const int16_t rx_band2_comp_phase[512];
extern const int16_t rx_band3_comp_phase[512];
extern const uint16_t robo_param_tab_ng[30][8];
extern const uint16_t robo_param_tab_sg[28][8];
extern const int16_t nbi_cal_cos_tab[512];
 
extern u8 HPLC_ChlPower[4];
extern const u8 HPLC_TestChlPower[4];
extern const u8 HPLC_PowerOffPower[4];

#define MPDU_FRAME_TYPE_BEACON              0
#define MPDU_FRAME_TYPE_SOF                 1
#define MPDU_FRAME_TYPE_ACK                 2
#define MPDU_FRAME_TYPE_COORDINATION        3 


typedef struct{
	uint8_t FrameType:3;
	uint8_t NetType:5;	
	uint8_t NetId[3];
	uint8_t Variable[8];
	uint8_t VariableLast:4;
	uint8_t Version:4;
	uint8_t FCRC[3]; 
}FC_t,*FC_p;

typedef struct{
	uint32_t timesTamp;
	uint16_t S_TEI:12;
	uint16_t tmi:4;
	uint16_t symbolNum:9;
	uint16_t line:3;
}BeaconVar,*BeaconVar_p;

typedef struct{
	uint32_t S_TEI:12;
	uint32_t D_TEI:12;
	uint32_t lid:8;
	uint16_t len:12;
	uint16_t pbNum:4;
	uint16_t symbolNum:9;
	uint16_t broadcastFlag:1;
	uint16_t retransmission:1;
	uint16_t encryptFlag:1;
	uint16_t tmi:4;
	uint16_t tmiExt:4; 

}SofVar,*SofVar_p;

//enum {FAIL = 0, SUCCESS = !FAIL};
typedef		int		bool_t;

typedef struct
{
	uint16_t BitsInLastSym;
	uint16_t TotalGroup;
	uint16_t N_PadIn;
	uint16_t PBSize;
	uint8_t InterShiftStepIndex;
	uint8_t ToneNumPerInter;
	uint8_t CopyNum;
	uint8_t Mod;
	uint8_t Rate;
	uint8_t CopyNumIdx;
	uint8_t PBSizeIdx;
	uint8_t pf0_pl_prr;
	uint8_t pf1_pl_prr;
}TMI_Para;

extern void calc_tmi_para(TMI_Para *params, uint8_t tmi, uint8_t tmiExt);
extern int calc_symbol_num(TMI_Para *params, uint8_t pb_num);


typedef enum{
	PLC_STATE_IDLE = 0,		//空闲状态，无数据包正在发送或者接收
							
	PLC_STATE_RECEIVING,	//数据接收中，从Sync开始 意味着可能出现帧
	PLC_STATE_RXBUSY,         //已经确认接到了帧 并且正在处理

	PLC_STATE_SENDING		//数据发送中，从打开LineDriver开始
}PLC_STATE;



//使能NVIC中BPLC中断,使能BPLC模块,返回固定为0
int BPLC_Start(void);

//失能NVIC中BPLC中断,失能BPLC模块,返回固定为0
int BPLC_Stop(void);

void BPLC_DisEnableAfe(void);

//使能中断,以下一个或多个
//BPLC_BBP_INT_EN_RXPLRCVINTEN_Msk
//BPLC_BBP_INT_EN_RXFCRCVINTEN_Msk
//BPLC_BBP_INT_EN_FRAMESYNCEDINTEN_Msk
//BPLC_BBP_INT_EN_TXENDINTEN_Msk
//BPLC_BBP_INT_EN_INITIALENDINTEN_Msk
//BPLC_BBP_INT_EN_TXDMADONEINTEN_Msk
void BPLC_EnableIRQ(uint32_t irq);

//失能中断,以下一个或多个
//BPLC_BBP_INT_EN_RXPLRCVINTEN_Msk
//BPLC_BBP_INT_EN_RXFCRCVINTEN_Msk
//BPLC_BBP_INT_EN_FRAMESYNCEDINTEN_Msk
//BPLC_BBP_INT_EN_TXENDINTEN_Msk
//BPLC_BBP_INT_EN_INITIALENDINTEN_Msk
//BPLC_BBP_INT_EN_TXDMADONEINTEN_Msk
void BPLC_DisableIRQ(uint32_t irq);

//清除中断状态,以下一个或多个
//BPLC_BBP_STATUS_RXPLRCVFLAG_Msk
//BPLC_BBP_STATUS_RXFCRCVFLAG_Msk
//BPLC_BBP_STATUS_RXFCDECODEFINISH_Msk
//BPLC_BBP_STATUS_FRAMESYNCEDFLAG_Msk
//BPLC_BBP_STATUS_TXENDFLAG_Msk
//BPLC_BBP_STATUS_INITIALENDFLAG_Msk
//BPLC_BBP_STATUS_RXPLCRCRES_Msk
//BPLC_BBP_STATUS_RXFCCRCRES_Msk
//BPLC_BBP_STATUS_TXDMADONEFLAG_Msk
void BPLC_ClearIRQ(uint32_t irq);

//获取当前频段,返回值(0,1,2,3)
uint8_t BPLC_TxFrequenceGet(void);

//设置频段(设置基带及ToneMask),输入(0,1,2,3)
//void BPLC_TxFrequenceSet(uint8_t num);更改为直接调用HPLC_PhyInit

//获取当前Tx增益,返回值(0~0x1F)
uint8_t BPLC_GetTxGain(void);

//设置Tx增益,输入(0~0x1F)
void BPLC_SetTxGain(uint8_t value);

//NBI 软件开启
void BPLC_NbiDet(void);

//获取当前ToneMask,返回16个字的数组首地址
uint32_t *BPLC_GetToneMask(void);



typedef void (*SendCallback)(bool_t status);


typedef enum {
  CLOSE_LINEDRIVER,
  OPEND_LINEDRIVER,
  
}linedriver_control;

typedef void (*OptionCallback)(uint32_t IsOpen,uint32_t phase);

#define RdReg(addr)    	        (*(uint32_t*)(addr))  
#define WrReg(addr,data)	((*(uint32_t*)(addr))=(data))


//NTB发送,NTB为数据发送时刻的NTB,*mpdu为MPDU(帧控制+帧载荷)数据,len为MPDU帧控制+帧载荷长度
void BPLC_SendAtNtb(uint32_t NTB, uint8_t *mpdu, uint16_t len, SendCallback callback, OptionCallback optionFun,uint32_t phase);

//立即发送,*mpdu为MPDU(帧控制+帧载荷)数据,len为MPDU帧控制+帧载荷长度
void BPLC_SendImmediate(uint8_t *mpdu, uint16_t len, SendCallback callback, OptionCallback optionFun,uint32_t phase);

void BPLC_GoTest(int mode);
typedef struct
{

	uint32_t sync_ntb;   //接收mpdu的sync NTB
//  uint32_t len;         //MPDU(帧控制+帧载荷)长度
	union
	{
		struct
		{
			int16_t rssi;        //信号强度,有符号值，1dbm为步进,NPW无法计算，给出的值同信噪比
			int16_t snr;         //信噪比,有符号值，0.25db为步进
			u8 band;
		};
		struct
		{
			int8_t hrf_rssi;
			int8_t hrf_snr;
			int8_t phr_mcs;
			uint16_t channel_index;
			int32_t freq;               //无线带来的频偏值
		};
	};

}BPLC_recv_para;


//判断用户缓冲区是否可填充接收数据
uint8_t IsUserBufLock(void);
//用户缓冲区上锁,执行接收回调函数前调用
void UnlockUserBuf(void);
//用户缓冲区解锁,用户处理完缓冲区数据后调用
void UnlockUserFcBuf(void);


typedef void (*RxOverFlowCallback)(void);  
typedef void (*FrameControlCallback)(uint8_t *fch, BPLC_recv_para *para, bool_t status);
typedef void (*ReceiveCallback)(uint8_t *mpdu, uint16_t len, uint8_t crc_res, BPLC_recv_para * para);
void SetReceiveCallback(ReceiveCallback cb_recv, FrameControlCallback cb_fch, RxOverFlowCallback cb_overflow);
/*
//获取状态
*/
PLC_STATE BPLC_GetState(void);

int GetSNR_DB(uint32_t signal, uint32_t noise);


//获取当前NTB值,返回NTB
uint32_t BPLC_GetNTB(void);

//获取SYNC NTB值,返回NTB
uint32_t BPLC_GetSYNC_NTB(void);





//过零依赖于GPIO的中断 因此在使用过零之前 需要自己配置GPIO的中断

//打开过零捕获
extern void ZeroCrossx_Start(uint32_t no);
//关闭过零捕获
extern void ZeroCrossx_Stop(uint32_t no);
//过零捕获在中断调用此函数  获得过零时刻NTB
extern uint32_t ZeroCrossx_Get(uint32_t no);


typedef void (*NTB_Match_Callback_Fn)(void);


//id:0-3;
//当NTB计数值达到match_ntb产生中断,调用callback_fn
//success return 0;
//error	return -1;
int NTB_TimerStart(uint8_t id, uint32_t match_ntb, NTB_Match_Callback_Fn callback_fn);

//id:0-3;
//success return 0;
//error	return -1;
int NTB_TimerStop(uint8_t id);

//NTB中断服务函数,需放置到对应的中断服务函数中
void ntb_interrupt_handler(void);

//BPLC中断服务函数,需放置到对应的中断服务函数中
void bplc_interrupt_handler(void);





//初始化,frq：0,1,2,3;power:0~0x1f
void		HPLC_PhyInit(uint8_t frq, uint8_t power);
void		HPLC_SouthPhyInit(uint8_t frq, uint8_t power);
//获取频段,返回值:0,1,2,3
uint8_t		FrequenceGet(void);

//计数帧载荷符号数
uint32_t	GetSymbolNum(uint8_t tmi, uint8_t tmiExt, uint8_t pb_num);
//计数帧长
uint32_t	GetFrameLen(uint8_t FC, uint16_t SymbolNum);
//获取NTB值
uint32_t	GetNTB(void);
//PHY复位
void		HPLC_PhyReset(void);
//获取PLC状态
PLC_STATE BPLC_GetTxState(void);
PLC_STATE BPLC_GetRxState(void);
//设置ToneMask   此函数用于测试模式下使用  设置tonemask前 需要把频段先切换到对应的频段再进行切换  否则参数会不对 导致性能下降
void		SetToneMask(uint8_t para);
void BPLC_SetToneMask(const uint32_t *table, uint16_t valid_tone_num);
//NTB调节
int			NTB_Sync_Adjust(uint32_t recvNTB0, uint32_t localNTB0, uint32_t recvNTB1, uint32_t localNTB1);
int			NTB_Sync_AdjustFarrow(uint32_t recvNTB0, uint32_t localNTB0, uint32_t recvNTB1, uint32_t localNTB1,int32_t *offset);
//NTB同步
void 		BPLC_SyncNTB(int32_t offset);
//NTB调节复位
void 		NTB_Reset(void);
int BPLC_TC_Get(void);
//用于检测窄带噪声  需要定时调用
void BPLC_StartNbi(void);

int BPLC_Reset();

void PLC_lower_Init(void);
void PHY_SendCallbackFun(bool_t flag);
void PHY_SetLineDriver(uint32_t state,uint32_t phase);
//获得不同tmi 不同pb块的符号数
uint32_t GetSymbolNum(uint8_t tmi, uint8_t tmiExt, uint8_t pb_num);
//计算帧的传输时间 FC 为1则包含帧长和前导的时间 为0则不包含帧长和前导  SymbolNum是payload的长度若为0表示不包含payload
uint32_t GetFrameLen(uint8_t FC, uint16_t SymbolNum);
//获取当前的NTB值
uint32_t GetNTB(void);
#define FRAMESYNC_OFFSET		12860
//extern void BPLC_RecfgPulse(int IsNeedSync);
#ifndef FPGA_ENV
#define PLL_TRIM_ENABLE
#endif
extern uint32_t trim_PLL(int32_t freq_diff, uint8_t reset);//*2^24
#define BPLC_SetReceiveCallback  SetReceiveCallback
void BPLC_GetWaferID(void * p_id);

int BPLC_CHIP_CHECK(void);
extern uint16_t HRFCurrentIndex;
extern u8 fixed_max_txgain;
#ifdef __cplusplus
}
#endif

#endif

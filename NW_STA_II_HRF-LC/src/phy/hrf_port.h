/**
 *  @file hrf_port.h
 *  @brief None.
 *  @author wufeng (feng.wu@gate-sea.com)
 *  @version v0.0.1 wufeng 2025.02.14 16:00 初始版本.
 *  @date 2025-02-14
 *
 *
 *  @par 修改日志:
 *  <table>
 *  <tr><th>Date       <th>Version <th>Author  <th>Description
 *  <tr><td>2025-02-14 <td>v0.0.1  <td>wufeng <td>初始版本
 *  </table>
 *  @copyright Copyright (c) 2025  Suzhou Gate-Sea Co.,Ltd.
 */
#ifndef _HRF_PORT_H
#define _HRF_PORT_H
/** Private includes ---------------------------------------------------------*/
#include <gsmcuxx_hal_def.h>

#if defined(ZB204_CHIP)
#include "ZB204.h"
#elif defined(ZB205_CHIP)
#include "ZB205.h"
#elif defined(ZB206_CHIP)
#include "ZB206.h" // Include the header file for ZB206 chip
#else
#error "Unsupported chip type"
#endif

#include "phy_port.h"
/** Use C compiler -----------------------------------------------------------*/
#ifdef __cplusplus /**< use C compiler */
extern "C"
{
#endif

// STF I&Q and LTF I&Q
extern const int16_t __Stf1[16];
extern const int16_t __Stf2[16];
extern const int16_t __Stf3[16];
extern const int16_t __Stf4[16];
extern const int16_t __Stf5[16];
extern const int16_t __Stf6[16];

// LTF FREQ
extern const uint32_t __LtfFreq1[4];
extern const uint32_t __LtfFreq2[4];
extern const uint32_t __LtfFreq3[4];
extern const uint32_t __LtfFreq4[4];

extern const uint32_t __PD[16];

extern const uint32_t __TM1[4];
extern const uint32_t __TM2[4];
extern const uint32_t __TM3[4];

extern const uint32_t __PTO1[4];
extern const uint32_t __PTO2[4];
extern const uint32_t __PTO3[4];
extern const uint32_t __PTO4[4];

#if defined(ZB204_CHIP) || defined(ZB205_CHIP)
  extern const int32_t __THC[24];
  extern const int32_t __RHC[24];
  extern const uint32_t __RAC[48];
  extern const uint32_t __TAC[48];

#elif defined(ZB206_CHIP)
  extern const int32_t __THC[32];
  extern const int32_t __RHC[32];
  extern const uint32_t __RAC[96];
  extern const uint32_t __TAC[96];
#endif
extern const int32_t __RFAC[240];

extern const uint32_t __CFO1[22];
extern const uint32_t __CFO2[22];
extern const uint32_t __CFO3[22];
extern const uint32_t __CFO4[22];

extern const int32_t __CF0O1[18];
extern const int32_t __CF0O2[18];
extern const int32_t __CF0O3[18];
extern const int32_t __CF0O4[18];

extern const int32_t __CF1O1[18];
extern const int32_t __CF1O2[18];
extern const int32_t __CF1O3[18];
extern const int32_t __CF1O4[18];

extern const int32_t __CF2O1[18];
extern const int32_t __CF2O2[18];
extern const int32_t __CF2O3[18];
extern const int32_t __CF2O4[18];

extern const uint32_t __SM[16];

extern const uint32_t __SFC[96];

extern const uint32_t __PM1[56];
extern const uint32_t __PM2[56];
extern const uint32_t __PM3[56];

extern const uint32_t __PP1[329];
extern const uint32_t __PP2[329];
extern const uint32_t __PP3[329];

extern const uint16_t HRF_PB_SIZE_ARRAY[6];

#if defined(ZB206_CHIP)
  extern const int16_t __Stf1_tx[16];
  extern const int16_t __Stf2_tx[16];
  extern const int16_t __Stf3_tx[16];
  extern const int16_t __Stf4_tx[16];
  extern const int16_t __Stf5_tx[16];
  extern const int16_t __Stf6_tx[16];
  extern const int16_t __Stf7_tx[16];
  extern const int16_t __Stf8_tx[16];
#endif

typedef struct
{
  uint8_t net_type;        // 0:南网 1:国网
  uint8_t channel;         // hrf的信道
  uint8_t option;          // 带宽(1：1M、2：500K、3：200K)
  uint8_t gain;            // 发送功率 (0 ~ 0x1F)
  uint16_t agc;            // 目标AGC增益
	void (*HrfDirFunc)(int);  //控制发送和接收方向的
} HRF_init_para;

#if defined(ZB204_CHIP)
	#define HRF_MIN_POWER_VAL   0
	#define HRF_MAX_POWER_VAL   31
#elif defined(ZB205_CHIP)
	#define HRF_MIN_POWER_VAL   0
	#define HRF_MAX_POWER_VAL   15
#elif defined(ZB206_CHIP)
	#ifdef EFEM_MODE
	#define HRF_MIN_POWER_VAL   16
	#define HRF_MAX_POWER_VAL   39
	#else
	#define HRF_MIN_POWER_VAL   0
	#define HRF_MAX_POWER_VAL   27
	#endif
#endif

typedef void (*HrfDirCallback)(int);

// 使能NVIC中HRF中断,使能HRF模块,返回固定为0
int HRF_Start(void);

// 失能NVIC中HRF中断,失能HRF模块,返回固定为0
int HRF_Stop(void);

// 使能中断,以下一个或多个
// HRF_BBP_INT_EN_RXPLRCVINTEN_Msk
// HRF_BBP_INT_EN_RXFCRCVINTEN_Msk
// HRF_BBP_INT_EN_FRAMESYNCEDINTEN_Msk
// HRF_BBP_INT_EN_TXENDINTEN_Msk
// HRF_BBP_INT_EN_INITIALENDINTEN_Msk
// HRF_BBP_INT_EN_TXDMADONEINTEN_Msk
void HRF_EnableIRQ(uint32_t irq);

// 失能中断,以下一个或多个
// HRF_BBP_INT_EN_RXPLRCVINTEN_Msk
// HRF_BBP_INT_EN_RXFCRCVINTEN_Msk
// HRF_BBP_INT_EN_FRAMESYNCEDINTEN_Msk
// HRF_BBP_INT_EN_TXENDINTEN_Msk
// HRF_BBP_INT_EN_INITIALENDINTEN_Msk
// HRF_BBP_INT_EN_TXDMADONEINTEN_Msk
void HRF_DisableIRQ(uint32_t irq);

// 清除中断状态,以下一个或多个
// HRF_BBP_STATUS_RXPLRCVFLAG_Msk
// HRF_BBP_STATUS_RXFCRCVFLAG_Msk
// HRF_BBP_STATUS_RXFCDECODEFINISH_Msk
// HRF_BBP_STATUS_FRAMESYNCEDFLAG_Msk
// HRF_BBP_STATUS_TXENDFLAG_Msk
// HRF_BBP_STATUS_INITIALENDFLAG_Msk
// HRF_BBP_STATUS_RXPLCRCRES_Msk
// HRF_BBP_STATUS_RXFCCRCRES_Msk
// HRF_BBP_STATUS_TXDMADONEFLAG_Msk
void HRF_ClearIRQ(uint32_t irq);

// 获取当前Tx增益,返回值(0~0x1F)
uint8_t HRF_GetTxGain(void);

// 设置Tx增益,输入(0~0x1F)
void HRF_SetTxGain(uint8_t value);
void HRF_SetTxGain_abs(uint8_t value);
//设置测试模式Tx增益,输入(0~0x1F)
void HRF_SetTestModeParam();

// 获取当前ToneMask,返回16个字的数组首地址
uint32_t *HRF_GetToneMask(void);

// NTB发送,NTB为数据发送时刻的NTB,*mpdu为MPDU(帧控制+帧载荷)数据,len为MPDU帧控制+帧载荷长度
void HRF_SendAtNtb(uint32_t NTB, uint8_t phr_mcs, uint8_t psdu_mcs, void *data, uint8_t pb_size_idx, SendCallback callBack);

// 立即发送,*mpdu为MPDU(帧控制+帧载荷)数据,len为MPDU帧控制+帧载荷长度
void HRF_SendImmediate(uint8_t phr_mcs, uint8_t psdu_mcs, void *data, uint8_t pb_size_idx, SendCallback callBack);

// 判断用户缓冲区是否可填充接收数据
uint8_t HRF_IsUserBufLock(void);
// 用户缓冲区上锁,执行接收回调函数前调用
void HRF_UnlockUserBuf(void);
// 用户缓冲区解锁,用户处理完缓冲区数据后调用
void HRF_UnlockUserPhrBuf(void);

// hrf计算与目标的频偏   发回值1/16ppm
int HRF_NTB_Freq(uint32_t targetNtb0, uint32_t syncNtb0, uint32_t targetNtb1, uint32_t syncNtb1);

typedef void (*PhrCallback)(uint8_t *fch, BPLC_recv_para *para, bool_t status);
typedef void (*Hrf_ReceiveCallback)(uint8_t *mpdu, uint16_t len, uint8_t crc_res, BPLC_recv_para *para);
void HRF_SetReceiveCallback(Hrf_ReceiveCallback cb_recv, PhrCallback cb_phr, RxOverFlowCallback cb_overflow);

// 获取状态
PLC_STATE HRF_GetState(void);

// 设置载波频偏
void HRF_SetSofPm(int32_t ppm);

// 获取当前NTB值,返回NTB
uint32_t HRF_GetNTB(void);

// 获取SYNC NTB值,返回NTB
uint32_t HRF_GetSYNC_NTB(void);

// 过零依赖于GPIO的中断 因此在使用过零之前 需要自己配置GPIO的中断

// 打开过零捕获
extern void ZeroCrossx_Start(uint32_t no);
// 关闭过零捕获
extern void ZeroCrossx_Stop(uint32_t no);
// 过零捕获在中断调用此函数  获得过零时刻NTB
extern uint32_t ZeroCrossx_Get(uint32_t no);

// id:0-3;
// 当NTB计数值达到match_ntb产生中断,调用callback_fn
// success return 0;
// error	return -1;
int NTB_TimerStart(uint8_t id, uint32_t match_ntb, NTB_Match_Callback_Fn callback_fn);

// id:0-3;
// success return 0;
// error	return -1;
int NTB_TimerStop(uint8_t id);

// HRF中断服务函数,需放置到对应的中断服务函数中
void HRF_interrupt_handler(void);
void HRF_dfe_interrupt_handler(void);
void HrfCheckLockParam(void);
// 初始化,frq：0,1,2,3;power:0~0x1f
int HRF_Init(HRF_init_para *para);
void HRF_PhyInit(uint8_t channel, uint8_t option, uint8_t power);
void HRF_SouthPhyInit(uint8_t frq, uint8_t power);

// PHY复位
void HRF_PhyReset(void);
// 滤波器开启
void HRF_TestModeFix(u8 mode);
// 获取PLC状态
PLC_STATE HRF_GetTxState(void);
PLC_STATE HRF_GetRxState(void);

void AFE_ChannelPara(void);
int HRF_Reset();
void HRF_phy_single_tone(uint8_t en);

void PLC_lower_Init(void);
void PHY_SendCallbackFun(bool_t flag);
void PHY_SetLineDriver(uint32_t state, uint32_t phase);
uint16_t HRF_PhrSymbol(uint8_t PHR_MCS, uint8_t option);
uint16_t HRF_PsduSymbol(uint8_t PSDU_MCS, uint8_t option, uint16_t len);
uint16_t HRFrameLen(uint8_t phr, int psdu, uint8_t option);
// 获取当前的NTB值

#define HRF_FRAMESYNC_OFFSET 652 * 25

void HRF_SwitchAfe(int isOpen);
int HRF_AfeStatus(void);

extern uint8_t  HRF_Option;
extern uint8_t  HRF_Channel;
int GetPbSize(int len);
int GetPbLengh(int pb);
uint16_t GetRcvPbSize(int index);

#define TFFT 12288
#define T_TR 768
#define T_G 3072



void rf_tx_iq_cali(void);

//产测固定AGC
void hrf_agc_factory_set(u8 gain);
void hrf_agc_factory_reset(void);
void SetHrfPowerComp(s8 op2, s8 op3);




#ifdef __cplusplus /**< end extern c */
}
#endif
#endif
/******************************** End of file *********************************/

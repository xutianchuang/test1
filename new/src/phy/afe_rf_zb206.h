/**
 *  @file afe_rf_zb206.h
 *  @brief None.
 *  @author wufeng (feng.wu@gate-sea.com)
 *  @version v0.0.1 wufeng 2025.05.08 16:00 初始版本.
 *  @date 2025-05-08
 *
 *
 *  @par 修改日志:
 *  <table>
 *  <tr><th>Date       <th>Version <th>Author  <th>Description
 *  <tr><td>2025-02-14 <td>v0.0.1  <td>wufeng <td>初始版本
 *  </table>
 *  @copyright Copyright (c) 2025  Suzhou Gate-Sea Co.,Ltd.
 */
#ifndef AFE_RF_ZB206_H
#define AFE_RF_ZB206_H
/** Includes -----------------------------------------------------------------*/
#include <stdint.h>
#if defined(ZB206_CHIP)
#include "ZB206.h" // Include the header file for ZB206 chip
#endif
#include "gsmcuxx_hal_def.h"
/** End of Includes ----------------------------------------------------------*/
//#define CST_WEISHENG   // 威胜API（库版本修改无效）

/** RF AFE defination --------------------------------------------------------*/
typedef struct
{
  uint8_t eLNA_en;
  uint8_t LNA_gain;
  uint8_t Gm_gain;
  uint8_t TIA_gain;
  uint8_t BPF0_gain;
  uint8_t BPF1_gain;
} afe_rx_gt; // RF_AFE gain table

typedef struct
{
  uint8_t QEC_Gain;
  uint8_t DAC_Gain;
  uint8_t MIX_Gain;
  uint8_t PA_Gain;
} afe_tx_gt; // RF_AFE tx gain table

typedef struct
{
  uint32_t rsv_3 : 1;
  uint32_t scrb : 2;
  uint32_t rsv_2 : 2;
  uint32_t frame_len : 11;
  uint32_t rsv_1 : 1;
  uint32_t rate : 5;
  uint32_t useless : 10;
} WISUN_OFDM_PHR;

typedef struct
{
  uint32_t rsv_2 : 6;
  uint32_t parity : 1;
  uint32_t check_sum : 4;
  uint32_t phy_mode : 4;
  uint32_t phy_type : 4;
  uint32_t rsv_1 : 2;
  uint32_t mode_switch : 1;
  uint32_t useless : 10;
} WISUN_MS_PHR;

typedef struct
{
  uint32_t rsv_2 : 6;
  uint32_t frame_len : 11;
  uint32_t data_whitening : 1;
  uint32_t fcs_type : 1;
  uint32_t rsv_1 : 2;
  uint32_t mode_switch : 1;
  uint32_t useless : 10;
} WISUN_FSK_PHR;

typedef struct
{
  uint8_t is_ms_frame;         // mode switch frame receive flag
  uint8_t is_fsk;
  union
  {
    uint32_t rx_phr_bits;        // received raw PHR bits
    WISUN_FSK_PHR rx_fsk_info;   // received FSK frame PHR frame info
    WISUN_MS_PHR rx_ms_info;     // received FSK mode switch frame PHR frame info
    WISUN_OFDM_PHR rx_ofdm_info; // received OFDM frame PHR frame info
  };
  uint16_t rx_frame_len;       // received frame PSDU length (including FCS)
  int sun_rssi;
  int sun_snr;
  int sun_ed_pwr;
} WISUN_RX_INFO;

typedef struct
{
  uint8_t fsk_en;    // wisun operation mode, 0:FSK 1:OFDM
  uint32_t ch0_freq; // wisun channel 0 center freq
  uint16_t channel;  // wisun channel number
  uint32_t ch_space; // wisun channel channel spacing
  uint32_t ct_freq;  // wisun current channel central freq
  uint32_t if_freq;  // wisun rx if freq

  uint32_t tx_time; // wisun tx pkt duration in us
  uint8_t phytype;  // PhyType, 0 : FSK wo FEC; 1 : FSK w FEC
  uint8_t phymode;  // PhyMode, 0 : reserved; 1: 50/0.5; 2: 50/1; 3: 100/0.5; 4: 100/1; 5: 150/0.5; 6: 200/0.5; 7: 200/1; 8: 300/0.5;

  // ofdm parameter
  uint8_t ofdm_opt;     // 带宽(1：1.2M、2：800K、3：400K、4：200K)
  uint8_t ofdm_mcs;     // data rate, 0~6
  uint8_t ofdm_phr_mcs; // phr_mcs, 0~2
  uint8_t ofdm_scrb;    // scrambler seed, 0~4
  uint8_t ofdm_intlv;   // interleaving
  uint8_t ofdm_fcslen;  // FCS length, 0: 4-bytes, 1: 2-bytes
  uint16_t ofdm_flen;   // frame length

  // fsk parameter
  uint8_t fsk_ms_en;   // mode switch, 0: dis, 1:en
  uint8_t fsk_gfsken;  // 0: FSK, 1: GFSK
  uint16_t fsk_rate;   // data rate, unit: kbps
  uint8_t fsk_midx;    // modulation index, 0: 0.5, 1:1
  uint8_t fsk_sfd;     // sfd mode, 0~1
  uint8_t fsk_plen;    // peramble length
  uint16_t fsk_flen;   // frame length
  uint8_t fsk_fecen;   // fec en, 0: dis, 1: en
  uint8_t fsk_fectype; // encoder type, 0:NRNSC, 1:RSC
  uint8_t fsk_intlv;   // interleaving,0: dis, 1: en
  uint8_t fsk_dw;      // data whitening, 0: dis, 1: en
  uint8_t fsk_fcslen;  // FCS length, 0: 4-bytes, 1: 2-bytes

  uint8_t gain; // 发送功率 (0 ~ 0x1F)
  uint16_t agc; // 目标AGC增益
  void (*HrfDirFunc)(int);   // 控制发送和接收方向的
} WISUN_init_para;

typedef struct 
{
  uint8_t fsk_gfsken;
  uint8_t fsk_sfd;
  uint8_t fsk_plen;
  uint8_t fsk_intlv;
  uint8_t fsk_dw;
  uint8_t fsk_fecen;
  uint8_t fsk_fectype;
  uint8_t fsk_fcslen;
} WISUN_FSK_ADVANCE_PAR;

typedef struct 
{
  uint8_t ofdm_phr_mcs;
  uint8_t ofdm_scrb;
  uint8_t ofdm_intlv;
  uint8_t ofdm_fcslen;
  uint8_t ofdm_flen;
} WISUN_OFDM_ADVANCE_PAR;

typedef struct
{
  uint8_t ofdm_sf; // frequency spreading
  uint16_t ofdm_Ncbps;
  uint8_t ofdm_Nrow;
  uint8_t ofdm_Ncbps_Nrow_ratio_log;
  uint16_t ofdm_pad_num;
  uint16_t ofdm_sym_num;
  uint8_t ofdm_mod;
  uint8_t reserved_bit;
  uint16_t phr_sym_num;
  uint16_t psdu_sym_num;
} WISUN_OFDM_Para;

typedef struct
{
  uint8_t phy_type;
  uint8_t phy_mode;
  uint8_t is_ms;    // 是否是mode switch帧
  uint8_t plen;     // FSK 的前导长度
  uint8_t fcs_len;  // payload CRC 长度, 1: 2bytes, 0: 4bytes
  uint16_t flen;    // payload 数据长度,不包括FCS
} WISUN_DURA_Para;  // 结构体用于计算frame duration

void rf_afe_ini(void);                             // RF_AFE initialization
void rf_afe_rx_enable(void);                       // RF_AFE set to RX mode
void rf_afe_tx_enable(void);                       // RF_AFE set to TX mode
void hrf_afe_rx_bbf_config(uint8_t option);        // RF_AFE RX BPF config
void sun_afe_rx_bbf_config(WISUN_init_para *para); // RF_AFE RX BPF config
void rf_afe_tx_bbf_config(uint8_t option);         // RF_AFE TX BPF config
void rf_afe_pll_config(uint32_t ch_freq);          // RF_AFE LO freq config
void rf_afe_cali_config(void);                     // RF_AFE TX IQ cal config
void rf_afe_vco_cali(void);                        // RF_AFE VCO calibration
void rf_afe_rck(void);                             // RF_AFE BPF RC time calibration
void rf_afe_rx_bbf_Zero_Shift(uint8_t option);
void rf_afe_rx_bbf_Left_Shift(uint8_t option);
void rf_afe_rx_bbf_Right_Shift(uint8_t option);
/** End of RF AFE defination -----------------------------------------------------------------*/
#if defined(ZB206_CHIP)
typedef int bool_t;
#endif
typedef void (*Wisun_PhrCallback)(ErrorStatus status, const WISUN_RX_INFO *info);
typedef void (*Wisun_ReceiveCallback)(ErrorStatus status, const WISUN_RX_INFO *info, const uint8_t *mpdu);
typedef void (*Wisun_SyncCallback)(void);
typedef void (*Wisun_TransmitCallback)(ErrorStatus status);
typedef void (*Wisun_RxOverFlowCallback)(void);
typedef void (*Wisun_HrfDirCallback)(int);
void DelayUs(int us);


/**
 * @brief wisun init (interrupt en and default par set)
 * 
 * @param dirCb switch pin control cb
 * @param gain gain set range 0-15
 * @return int 0 success
 */
int WISUN_PHYInit(Wisun_HrfDirCallback dirCb, uint8_t gain); // RF-PHY operation mode setting

/**
 * @brief Get RSSI
 * 
 * @return int -127 - +127
 */
int WISUN_GetRSSI(void);

/**
 * @brief Get Energy detect results
 * 
 * @return int 
 */
int WISUN_GetED(void);

/**
 * @brief Receive callback initialization
 * 
 * @param cb_recv rx payload cb
 * @param cb_phr rx phr cb
 * @param cb_overflow rx err cb
 * @param cb_sy rx sync start cb
 */
void WISUN_SetReceiveCallback(Wisun_ReceiveCallback cb_recv, Wisun_PhrCallback cb_phr, Wisun_RxOverFlowCallback cb_overflow, Wisun_SyncCallback cb_sy);

/**
 * @brief send payload
 * 
 * @param data data ptr
 * @param payload_len data len
 * @param callBack send ok cb set
 */
void WISUN_SendImmediate(const void *data, uint16_t payload_len, Wisun_TransmitCallback callBack);

/**
 * @brief send mode switch
 * 
 * @param phymode new mode phymode
 * @param phytype new mode phytype
 * @param callBack send ok cb set
 */
void WISUN_MSF_SendImmediate(int phymode, int phytype, Wisun_TransmitCallback callBack);

#ifndef CST_WEISHENG
  /**
   * @brief start trx use channel
   * 
   * @param rf_rx_channel channel number
   */
  void RfReceive(int rf_rx_channel);
#else
  /**
   * @brief start trx use center freq
   * 
   * @param rf_freq center freq
   */
  void RfReceive(int rf_freq);
#endif

/**
 * @brief set fsk advanced par before RfFskModulation init
 * 
 * @param advance_par fsk advanced par
 */
void RfSetFskAdvancePar(const WISUN_FSK_ADVANCE_PAR *advance_par);

/**
 * @brief set ofdm advanced par before RfOfdmModulation init
 * 
 * @param advance_par ofdm advanced par
 */
void RfSetOfdmAdvancePar(const WISUN_OFDM_ADVANCE_PAR *advance_par);

/**
 * @brief fsk init
 * 
 * @param fec 0: dis, 1: en
 * @param modulation_index 0: 0.5, 1:1
 * @param datarate data rate unit: kbps 
 * @param channel_0_frequency channel 0 center freq unit: kbps 
 * @param channel_spacing channel channel spacing unit: kbps 
 */
void RfFskModulation(int fec, int modulation_index, int datarate, int channel_0_frequency, int channel_spacing);

/**
 * @brief ofdm init
 * 
 * @param option bandwidth(1：OFDM 1 1.2M、2：OFDM 2 800K、3：OFDM 3 400K、4：OFDM 4 200K) 
 * @param mcs data rate mcs 0 ~ 6 
 * @param channel_0_frequency channel 0 center freq unit: kbps 
 * @param channel_spacing channel channel spacing unit: kbps 
 */
void RfOfdmModulation(int option, int mcs, int channel_0_frequency, int channel_spacing);

/**
 * @brief DfeReset
 * 
 */
void RfDfeReset(void);

/**
 * @brief set transmission power index
 * 
 * @param power gain index,range 0 ~ 30
 */
void RfSetTxPower(uint8_t pwr_index);

/**
 * @brief set transmission power in dBm
 * 
 * @param power gain index,range 18~30
 */
uint8_t RfSetTxPowerdBm(uint8_t pwr_dbm);

/**
 * @brief set single tone amplitude offset
 * 
 * @param amplitude offset -300 ~ +300
 */
void RfSetToneOfs(int amp_ofs);

/**
 * @brief set RF frequency offset in Hz 
 * 
 * @param rf_freq_ofs offset Hz
 */
void RfFreqOfs(int rf_freq_ofs);

/**
 * @brief get calculate tx time for current par 
 * 
 * @param len frame len
 * @return uint32_t tx time in us
 */
uint32_t RfGetTime(const WISUN_DURA_Para *dura_para);

/**
 * @brief get TX End Flag
 * 
 * @return uint8_t TX end Flag
 */
uint8_t WISUN_GetTxEnd_Flag(void);

/**
 * @brief set continous transmission status
 * 
 * @param transmission on or off
 */
void RfTestContTransmission(uint8_t status);

#endif
/******************************** End of file *********************************/
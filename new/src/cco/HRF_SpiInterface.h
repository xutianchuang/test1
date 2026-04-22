#ifndef __HRF_SPIINTERFACE_H__
#define __HRF_SPIINTERFACE_H__
#ifdef __cplusplus
extern "C"
{
#endif
#include<stdint.h>
#include"bps_spi.h"
#include "hplc_mpdu.h"
typedef struct{
	uint8_t  option;//0 表示使用默认的option和chanel
	uint8_t  channel;
	uint16_t Len;
	uint32_t SendNtbStart;//发送的NTB的起始时间
	uint32_t SendNtbEnd;  //发送NTB的结束时间	  
	uint8_t  MCS;
	uint8_t  BE;//be为csma退避参数取值若be为0 则意味着信道一旦空闲就进行发送 不进行退避
	uint8_t  data[0];
}HRF_SendFrame;
typedef struct{
	uint8_t  option;//发送的channel的index
	uint8_t  channel;
	uint16_t Len;
	uint8_t  MCS;
	uint8_t  crc24;//mpdu 的crc24是否正确 信标帧即使payload不对fc依然是需要的
	int8_t   Rssi;
	int8_t   Snr;
	uint32_t SyncNtb;//同步的NTB
	uint8_t data[0];
}HRF_RecFrame;
typedef struct{
	int32_t NtbOffset;
	int32_t PllTrim;
}HRF_NTbControl;//频偏调整  0代表无需调整
typedef struct{
	uint32_t NID;
	uint16_t TEI;
}HRF_Control;



typedef struct{
	SPI_CMD cmd;
	uint16_t len;
	uint32_t arg;
	CSMA_CALL_BACK call;
	uint8_t data[0];
}HRF_Pack;
void SPI_InterfaceTask(void *arg);
void HRF_CmdPack(SPI_CMD cmd,uint8_t *data,uint16_t len,CSMA_CALL_BACK call);
bool HRF_SendBeacon(u8* beacon,u16 len,u8 rfType,u32 ntbStart,u32 ntbStop);
bool HRF_SendMac(SOF_PARAMS *sofParam,u8* mac,CSMA_CALL_BACK callback);
bool HRF_SencConcert( u8 *pdu_data, u16 pdu_len);
void HRF_GetNtb(void (*callBack)(u32));
void HRF_SetPll(int ppm,u32 NTB);
void HRF_SetIndex(u16 channel,u8 option,u8 power);
void HRF_GetIndex(u8 *channel,u8 *option);
void HRF_SetTei(u16 tei, u32 NID);

u16 HRFrameLen(u8 phr,int psdu,u8 option);//100us
u16 HRFPhrBestMcs(u8 option,u8 isBo);
u16 HRFBestMcs(u8 option,u8 isBo);
u16 HRF_PhrSymbol(u8 PHR_MCS,u8 option);
u16 HRF_PsduSymbol(u8 PSDU_MCS,u8 option,u16 len);
void HRF_Reset(void);
void HRF_SetTestMode(u32 mode);
void CheckCurrentHrfCodeSection(void);
uint32_t sum4crc(void *p, uint16_t len);
#ifdef __cplusplus
}
#endif
#endif /* __HRF_SPIINTERFACE_H__ */


#ifndef _BPLC_PHY_H
#define _BPLC_PHY_H
#define HPLC_FREQ_CNT 4


typedef enum
{
	HPLC_PHY_IDLE,
	HPLC_PHY_BUSY,
}HPLC_PHY_STATE;

//HPLC物理底层初始化
void HPLC_PHY_Init(void);

//HPLC设置频段
void HPLC_PHY_SetFrequence(u8 fre);

//读取频段
u8 HPLC_PHY_GetFrequence(void);

//计数帧载荷符号数
uint32_t	HPLC_PHY_GetSymbolNum(uint8_t tmi, uint8_t tmiExt, uint8_t pb_num);
//计数帧长
uint32_t	HPLC_PHY_GetFrameLen(uint8_t FC, uint16_t SymbolNum);

//HPLC复位
void HPLC_PHY_Reset(void);

//HPLC调节
int HPLC_PHY_NTB_Adjust(uint32_t recvNTB0, uint32_t localNTB0, uint32_t recvNTB1, uint32_t localNTB1);

//HPLC状态
HPLC_PHY_STATE HPLC_PHY_GetState(void);

//PB回调
void HPLC_PHY_PB_Callback(uint8_t *data, uint16_t len, uint8_t crc_res, BPLC_recv_para *para);

//FC回调
void HPLC_PHY_FC_Callback(uint8_t *data, BPLC_recv_para *para, bool_t status);

//HPLC发送
void HPLC_PHY_SendImmediate(uint8_t *data, uint16_t len, SendCallback callback);

//HPLC NTB发送
void HPLC_PHY_SendAtNtb(uint32_t NTB, uint8_t *data, uint16_t len, SendCallback callback);


extern uint16_t HRFCurrentIndex;

#endif

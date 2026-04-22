#ifndef _PROTOCOL_STACK_H_
#define _PROTOCOL_STACK_H_
#include "os.h"
#include "phy_port.h"
#define MEM_BLOCK_SIZE      (520*4+16)

#define MAX_RECEIVE_PDU_NUM		25

//#define MAX_SEND_PDU_NUM		10

typedef enum
{
  	RECEIVE_FC_PDU,
	RECEIVE_PL_PDU,
	//扩展HRF消息类型
    RECEIVE_HRF_FC_PDU,
    RECEIVE_HRF_PL_PDU,
}RECEIVE_PDU_TYPE;

//接收使用的PDU
#pragma pack(4)
typedef struct
{
    BPLC_recv_para rxPara;
	RECEIVE_PDU_TYPE type;
	u8 crcState;
    u16 size;
    u8 pdu[MEM_BLOCK_SIZE];
}PDU_BLOCK;

//发送使用的PDU
#pragma pack(4)
typedef struct
{
    u16 size;
	u16 phr_mcs;
	u8 pdu[MEM_BLOCK_SIZE];
}SEND_PDU_BLOCK;

//初始化
void HPLC_ProtocolInit(void);

//获取一个PDU
PDU_BLOCK* MallocPDU(void);

//释放一个PDU
void FreePDU(PDU_BLOCK* pdu);

//分配一个发送PDU
SEND_PDU_BLOCK* MallocSendPDU(void);

//释放一个发送PDU
void FreeSendPDU(SEND_PDU_BLOCK* pdu);

//闪烁接收灯
void FlashReceiveLed(void);

//闪烁发送灯
void FlashSendLed(void);

//本地LED闪烁
void FlashLocalLed(void);

extern OS_MEM Mempool;
#endif

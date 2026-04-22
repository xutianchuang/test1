#ifndef _PROTOCOL_STACK_H_
#define _PROTOCOL_STACK_H_

#define MEM_BLOCK_SIZE      2096
#define MAX_PDU_BLOCK_SIZE  2096        //524


//PDU
#pragma pack(4)
typedef struct
{
    BPLC_recv_para rxPara;
    CPU_NTB_64     receive_ntb;     //接收时的NTB值
    u16            receive_phase;      //PHASE_A, PHASE_B, PHASE_C
    u16            size;
    u8 pdu[MEM_BLOCK_SIZE];
}PDU_BLOCK;


//初始化
void HPLC_ProtocolInit(u8 option,u8 channel);

//获取一个PDU
PDU_BLOCK* MallocPDU(void);

//释放一个PDU
void FreePDU(PDU_BLOCK* pdu);

//闪烁接收灯
void FlashReceiveLed(void);

//闪烁发送灯
void FlashSendLed(void);
void ClearZC_NTB(void);
#endif

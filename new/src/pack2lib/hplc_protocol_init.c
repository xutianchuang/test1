#include "System_inc.h"
#include "common_includes.h"
#include "os.h"
#include "hrf_phy.h"
#include "../../src/pack2lib/pack2lib.h"
#define MAX_PDU_SIZE    15
#define FLASH_TIME 20
//PDU内存池
//static OS_MEM Mempool;
//static PDU_BLOCK Mempdu[MAX_PDU_SIZE];

void FlashLedTimer(void* arg);

//--------para--------------
bool FlashReceive = false;
static TimeValue ReceiveLedTimer;
bool FlashSend = false;
static TimeValue SendLedTimer;
u8 FlashSendMode = 0; //本地发送绿灯当前模式（snid和freq指示时，串口发送闪灯模式取反）

//-------start-------
void FlashLedTimer(void* arg)
void HPLC_ProtocolInit(u8 option,u8 channel)
PDU_BLOCK* MallocPDU(void)
void FreePDU(PDU_BLOCK* pdu)
void FlashReceiveLed(void)
void FlashSendLed(void)


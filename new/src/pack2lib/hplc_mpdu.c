#include <string.h>
#include "os.h"
#include "common_includes.h"
#include "System_inc.h"
#include "hplc_receive.h"
#include "plc_autorelay.h"
#include "os_cfg_app.h"
#include "hrf_phy.h"
#include "../../src/pack2lib/pack2lib.h"

#define MAX_SOF_PHYBLOCK_NUM    4

typedef struct
{
    DIV_COPY_MODEL      Mode;
    u16                 BlkSize;
}COPY_MODEL_INFO, *P_COPY_MODEL_INFO;

typedef struct
{
    DIV_COPY_EX_MODEL   ModeEx;
    u16                 BlkSize;
}COPY_MODEL_EX_INFO, *P_COPY_MODEL_EX_INFO;

typedef struct
{
	u8 option;
	u8 channel;
}HRF_Reader_Channel;

typedef struct
{
    u16         tei;
    u8          seq_end;        //最后1个物理块的块号
    u8          reserve;
    OS_TICK_64  tick;           //最近接收到物理块的时间
    void*       blk_body_data[MAX_SOF_PHYBLOCK_NUM];     //物理块体
    u32         blk_body_len;                            //物理块体长度，每个物理块大小一致
}MPDU_SOF_INFO, *P_MPDU_SOF_INFO;

//过滤网络号
typedef struct
{
   u32  NID;        //指定网络 
   bool effect;     //是否生效(如果生效，会过滤除指定NID的所有报文)
}SPECIFY_NID;


P_MPDU_SOF_INFO MpduGetSofInfo(u16 tei, u16 new_blk_len);
BOOL MpduIsSofReceiveComplete(P_MPDU_SOF_INFO pSofInfo);
P_MPDU_SOF_INFO MpduPutSofInfo(u16 tei, u8 phyBlkCount, P_SOF_PHY_BLOCK_HEAD pPhyBlock, u16 nPhyBlockLen, BOOL* pCompleted,u8 crc);
BOOL MpduCombinaMpdu(P_MPDU_SOF_INFO pSofInfo, u8** ppMacFrame, u16* pMacLen);
void MpduClearSofInfo(P_MPDU_SOF_INFO pSofInfo);
void DivideLongMsdu(u8* macFrame,u16 len,MPDU_CTL *mpduCtl,u8* mpdu,u16 *mpduLen);
void MpduCheckSofInfoTimeOut(void* arg);
void Set_connectReader_Hrf(u8 hrfindex,u8 link);
int  PLC_SyncModelMacCalibrateNTB(PDU_BLOCK* pdu);
bool SendReceiveConfirm(u8 frome ,PDU_BLOCK* pdu);
void MpduHandlehHrfBeacon(PDU_BLOCK* pdu,u8 crc);
void MpduHandleHrfSOF(PDU_BLOCK* pdu,u8 crc);
void HrfMpduHandleConfirm(PDU_BLOCK* pdu);
void MpduHandleBeacon(PDU_BLOCK* pdu,u8 crc);
void MpduHandleSOF(PDU_BLOCK* pdu,u8 crc);
void MpduHandleConfirm(PDU_BLOCK* pdu);
//--------para--------------

static pfBeaconCallback s_pfBeaconCallback = NULL;
static pfConcertCallback s_pfConcertCallback = NULL;
P_MPDU_SOF_INFO s_mpdu_sof_info[32];
static SPECIFY_NID SpecifyNID;
u32 PLC_TO_RF_TESTMODE_PARAM;

//-------start-------
void MpduHandleBeacon(PDU_BLOCK* pdu,u8 crc)
void MpduHandlehHrfBeacon(PDU_BLOCK* pdu,u8 crc)
BOOL MpduIsSofInfoCorrect(P_MPDU_SOF_INFO pSofInfo, u16 new_blk_len)
static void MpduCheckSofInfoTimeOut(void* arg)
BOOL MpduIsSofReceiveComplete(P_MPDU_SOF_INFO pSofInfo)
P_MPDU_SOF_INFO MpduPutSofInfo(u16 tei, u8 phyBlkCount, P_SOF_PHY_BLOCK_HEAD pPhyBlock, u16 nPhyBlockLen, BOOL* pCompleted,u8 crc)
BOOL MpduCombinaMpdu(P_MPDU_SOF_INFO pSofInfo, u8** ppMacFrame, u16* pMacLen)
void MpduHandleSOF(PDU_BLOCK* pdu,u8 crc)
void MpduHandleHrfSOF(PDU_BLOCK* pdu,u8 crc)
BOOL    MpduIsMpduCorrect(u8* pData, u16 nLen,u8 crc)
bool MpduGetBeaconSlot(const P_BEACON_MPDU pBeaconPayload, P_BEACON_SLOT* ppBeaconSlot)
bool SendReceiveConfirm(u8 frome ,PDU_BLOCK* pdu)
void MpduHandleConfirm(PDU_BLOCK* pdu)
void HrfMpduHandleConfirm(PDU_BLOCK* pdu)
void MpduSetNID(bool effect, u32 NID)
void HPLC_HandleMpduToHRF(PDU_BLOCK* pdu,u8 crc)
void MpduHandleMPDU(PDU_BLOCK* pdu,u8 crc)


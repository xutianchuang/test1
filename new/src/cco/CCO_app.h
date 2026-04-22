#ifndef __CCO_APP_H__
#define __CCO_APP_H__
#ifdef __cplusplus
extern "C"
{
#endif
#include "type_define.h"
#include "plc_autorelay.h"
#include "system_inc.h"
#include "hplc_mac_def.h"
typedef struct
{
	u32 flash_addr;
	u32 file_size;
	u32 file_checksum;
}STA_UPDATE_PARAM, *P_STA_UPDATE_PARAM;
typedef struct 
{
	u8 cco_mac[LONG_ADDR_LEN];
	u8 option;
	u8 channel;
	u32 nid;
	uint64_t firstTick;
	uint64_t lastTick;
	uint64_t firstStaReportTick;
}HrfCcoNeighbor_s;
extern HrfCcoNeighbor_s HrfCcoNeighbor[HRF_CCO_MAX];
extern RELAY_STATUS        g_RelayStatus;
extern BEACON_PARAM     beacon_param_preget;
#ifdef HPLC_CSG
extern u32 selfreg_bitmap[MAX_BITMAP_SIZE/4];
#endif

//#define CON_PLC_RESEND_COUNT 3
//#define CON_REASSOC_TIME_MS  (60000)
//#define CON_REASSOC_MUTI_TIME_MS  (60000*7)







unsigned char PLC_X4SendDeleteReadTask(u16 tei, u8 task_id);
unsigned char PLC_X4SendReadTaskDetail(u16 tei, u8 task_id);
unsigned char PLC_X4SendReadTaskDetail(u16 tei, u8 task_id);
unsigned char PLC_SendDirRead(P_READ_MSG_INFO pReadMsg);
bool IsOrphanNode(u16 tei);
OS_TICK_64 PLC_GetReadMeterTimeoutTick(u16 datalen);
PLM_PHASE PLC_GetPlmPhaseByMacAddr(u8 mac[6]);
unsigned char PLC_X4SendReadTaskID(u16 tei);
unsigned char PLC_SendDataTrans(BOOL broadcast,u8 meter_moudle, HPLC_SEND_PHASE phase, u16 seq,u8 mac_addr[MAC_ADDR_LEN], u8 *pdata, u16 len);
unsigned char PLC_SendNewPreciseTime(BOOL broadcast,u8 meter_moudle, HPLC_SEND_PHASE phase, u16 seq,u8 mac_addr[MAC_ADDR_LEN], u8 *pdata, u16 len);
unsigned char PLC_SendPreciseTime(BOOL broadcast,u8 meter_moudle, HPLC_SEND_PHASE phase, u16 seq,u8 mac_addr[MAC_ADDR_LEN], u8 *pdata, u16 len);
u8 PLC_SendGetStaStatus(u8 broadcast, u8 mac_addr[MAC_ADDR_LEN], u32 element, HPLC_SEND_PHASE phase);
unsigned char PLC_X4SendReadCollTaskData(BOOL broadcast, HPLC_SEND_PHASE phase, u8 mac_addr[MAC_ADDR_LEN], u8 *pdata, u16 len);
u8 PLC_SendGetStaInfo(u8 broadcast, u8 mac_addr[MAC_ADDR_LEN], u8 element_num, u8* element_array, HPLC_SEND_PHASE phase);
u8 PLC_SendGetChannelInfo(u8 broadcast, u8 mac_addr[MAC_ADDR_LEN], u16 start_sn, u8 num, HPLC_SEND_PHASE phase);
u8 PLC_BroadcastProc(OS_TICK_64 tick_now);
unsigned char PLC_SendBroadcast(P_MSG_INFO pSendMsg, u16 seq, u8 timing_seq);
unsigned char PLC_SendAccurateTiming(P_MSG_INFO pSendMsg, u16 seq, u8 timing_seq);
unsigned char PLC_SendPhaseErr(u16 seq);
u8 PLC_ZCProc(OS_TICK_64 tick_now);
u8 PLC_SelfRegProc(OS_TICK_64 tick_now);
void PLC_ReportNodeInfoProc(void);
u8 PLC_SendStaAuthenProc(OS_TICK_64 tick_now);
u8 PLC_RouteUpdateProc(OS_TICK_64 tick_now);
u8 PLC_AreaGetEdgeProc(OS_TICK_64 tick_now);
u8 PLC_AreaDiscernmentProc(OS_TICK_64 tick_now);
u8 PLC_ConfigRecordProc(OS_TICK_64 tick_now);
u8 PLC_X4CollTaskInitProc(OS_TICK_64 tick_now);
u8 PLC_X4ReadTaskSyncProc(OS_TICK_64 tick_now);
u8 PLC_GetCollSnProc(OS_TICK_64 tick_now);
void PLC_AppZeroCrossReport(u8 *pdata, u16 len, u8 collectionType);
void PLC_HandlesAppEvent(u16 SrcTei,u16 DstTei,P_EVENT_PACKET pEvent
#ifdef HPLC_CSG
						 , P_APP_BUSINESS_PACKET pBusinessPacket
#endif
						);
u8 PLC_GetStaGetModuleIdRealTimeProc(OS_TICK_64 tick_now);
int insertNeighbor(u8 *cco_mac,u8 needConv,u32 nid);
u8 PLC_GetStaGetModuleIdRealTimeProc(OS_TICK_64 tick_now);
void PLC_HandleMacFrame(P_RECEIVE_MAC pReceiveMac);
unsigned char PLC_SendCommTest(P_COMM_TEST_INFO pTestMsg);
void  InitProcPara(P_PLC_STA_UPDATE_CB pcb, EM_STA_UPDATE_STATE status, u32 timeout, u8  broad, u8 send_cnt, BOOL bitmap, BOOL startFlag);
unsigned char PLC_SendInnerVerRead(u8 node_mac[6]);
#ifdef HPLC_CSG    
unsigned char PLC_SendStaReset(P_RESET_STA_INFO pResetSta);
#endif

void PLC_Task_Init(void);
void PLC_HandleAppFrame(P_HPLC_PARAM pHplcParam, P_MAC_HEAD pMacHead, P_APP_PACKET pAppFrame);
void PLC_MsgProc();
void PLC_RouterProc(OS_TICK_64 tick_now);
void gotoDetect(OS_TICK_64 tick_now);
void PLC_BeaconFrameCallback(const P_HPLC_PARAM pHplcParam, P_BEACON_ZONE pBeaconZone, P_BEACON_MPDU pBeaconPayload);
void PLC_BeaconFrameCallback(const P_HPLC_PARAM pHplcParam, P_BEACON_ZONE pBeaconZone, P_BEACON_MPDU pBeaconPayload);
void PLC_MacFrameCallback(const P_HPLC_PARAM pHplcParam, P_MAC_FRAME pMacFrame,SOF_PARAM *sofParam);
unsigned short PLC_Trans_MeterAddr2Sn(unsigned char *pAddr6, unsigned short *pEmptySn);
void PLC_GetSendParam(u16 tei, u16 *p_des_tei, HPLC_SEND_PHASE *p_send_phase,u8* link);
u8 PLC_SendAreaDiscernmentFeature(void);
void PLC_ClearSelfRegList(void);
void PLC_AutoCfgFreezeDI(void);
u8 PLC_SendAppPhaseDiscernment(u16 tei);
bool PLC_GetMacAddrAppFrame(u8*data,u16 len, U8 mac_addr[MAC_ADDR_LEN]);
void PLC_AddToSelfRegList(u8 pNodeAddr[CCTT_METER_ADDR_LEN]);
u8 PLC_SendRouteRead(P_PLC_ROUTE_READ_CB pReadItemInfo,u16 first_read);
u8 PLC_GetStaGetModuleIdProc(OS_TICK_64 tick_now);
u8 PLC_SetEventSwitchProc(OS_TICK_64 tick_now);
u8 PLC_StoreDataBcastTimeProc(OS_TICK_64 tick_now);
u8 PLC_StoreDataSyncCfgProc(OS_TICK_64 tick_now);
u8 PLC_GetStaPhaeEdgeProc(OS_TICK_64 tick_now);
void AddOutListMeter(u8* mac);
void ClearOutListMeter(int clear_with);
int FindOutListMeter(u8 *mac);
extern OS_MUTEX mutex_sta_info;
u8 PLC_PostMessage(TMESSAGE message,  WPARAM wParam, LPARAM lParam);
BOOL IsAllStaDone(u32 bitmap[MAX_BITMAP_SIZE / 4]);
void PLC_NewJoinGetColl(u16 tei);
unsigned char PLC_GetMacAddrByMacFrame(P_MAC_HEAD pMacHead, U8 mac_addr[MAC_ADDR_LEN]);
void PLC_HandleOneHopMacFrame(P_RECEIVE_MAC pReceiveMac,u16 stei,u16 dtei);
void PLC_RequestCentreTimePeriod(void);
BOOL PLC_fetch_task_read_item_gd(P_READ_TASK_INFO *ppReadTask,bool isMultiTask ,u16 *taskid,u8 taskmac[][MAC_ADDR_LEN],u8 max_num);
P_READ_TASK_INFO PLC_find_read_task_gd(EM_TASK_TYPE task_type, u16 taskid);
BYTE PLC_GetMeterDataByDL645(const P_GD_DL645_BODY pDL645Cmd, BYTE **ppDataUnit, BYTE *pLen);
BYTE PLC_GetMeterDataByDI(USHORT meter_sn, ULONG DI, BYTE **ppDataUnit, BYTE *pLen);
USHORT PLC_dirread_response(BYTE meter_protocol, P_GD_DL645_BODY pDL645Resp);
BOOL PLC_OnGetStaParam(u16 tei, P_HPLC_STA_PARAM pStaParam);
BOOL PLC_OnGetBeaconParam(P_BEACON_PARAM pBeaconParam);
void PLC_ChangeNetIDCallback(u32 new_net_id);
void PLC_CheckOfflineList();
void PLC_RestartNetworking();
void PLC_SendFoundListAuto();
void PLC_StaStatAndOffline();
void PLC_SendHrfFoundListAuto(void);
void PLC_SendAssocGatherIndAuto(OS_TICK_64 tick_now);
void PLC_SendOfflineIndAuto();
void lockStaInfo(void);
void unLockStaInfo(void);
int macVaileCmp(u8 *macA, u8 *macB);
void PLC_FreeStaInfoRelationship(u16 tei);
HPLC_PHASE PLC_GetHplcPhaseByReport(u32 phase_report);
void PLC_AssocCnfOK(ASSOC_REQ_PACKET *pAssocReq, USHORT tei, u8 assoc_res);
void PLC_AssocCnfError(MAC_HEAD *pMacHead, ASSOC_REQ_PACKET *pAssocReq, ASSOC_RESULT assoc_res);
void PLC_AssocIndOK(ASSOC_REQ_PACKET *pAssocReq, USHORT tei, u8 assoc_res);
void PLC_AssocIndError(MAC_HEAD *pMacHead, ASSOC_REQ_PACKET *pAssocReq, ASSOC_RESULT assoc_res);
u8 PLC_ResetSilenceTime(u16 tei, BOOL check_whitelist);
void PLC_HandleNodeList(MAC_HEAD *pMacHead, u8 *pdata, u16 len);
void PLC_HandleZeroCrossReport(MAC_HEAD *pMacHead, u8 *pdata, u16 len);
void PLC_SetReceivePhase(const P_HPLC_PARAM pReceiveParam);
void changeHplcPhase(P_STA_INFO psta, HPLC_PHASE phase);
void SplitFormePco(int sta);
u16 PLC_GetTopPco(u16 tei);
P_STA_BITMAP PLC_GetDirectSubStas(USHORT tei, bool with_pco);
void writeSendRoutTable(P_MM_FRAME pMmFrame, u8 IsCnfFrame, ROUTE_MSG_FILED *pRouteHead, P_STA_BITMAP pStaBimap, u16 tei, u16 *pEof);
static u8 NodePCOCheck(P_NOT_CENTER_BEACON_STA NodeInfo);
void Hrf2IdelChannel(uint64_t tick_now,u8 *bitmap);
BOOL PLC_GetNotCenterBeaconSta(const P_WAIT_FOR_SEND_INFO p_wait_for_send, P_NOT_CENTER_BEACON_INFO p_not_center_info,u8 * times);
void PLC_GenerateBeaconSlot(const u16 not_center_beacon_num, u16 *p_beacon_period, u16 cmsa_slot_phase[3], u16 tie_cmsa_slot_phase[3]);
u16 PLC_GetRoutePeriodSecond(u16 cfged_meter_num);
extern u8 PlcRxWaitQueueMem[sizeof(queue)-sizeof(HANDLE)+ PLC_RX_WAIT_QUEUE_SIZE*sizeof(HANDLE)];
extern queue *g_PlcWaitRxQueue;
extern OS_MUTEX mutex_sta_info;
extern beaconDescoverInfo_s beaconDescoverInfo;
extern const TMN_VER_INFO c_Tmn_ver_info_inner;
extern const u8 BatterHrfChannel[3][20];
#ifdef __cplusplus
}
#endif
#endif /* __CCO_APP_H__ */


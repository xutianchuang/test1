#include "system_inc.h"
//#include "../hal/macrodriver.h"
#include "manager_link.h"
#include "RTC.h"
//#include "../drv/uart_link.h"
#include "alert.h"
#include "queue.h"
#include "module_def.h"
//#include "data_types.h"
#include "plm_2005.h"
//#include "../app/PLM_AFN.h"
//#include "../app/storage.h"
//#include "../dev/dataflash/data_flash.h"
#include "plc_autorelay.h"
//#include "../dev/plcboard.h"
#include "os_cfg_app.h"
#include "hplc_send.h"
#include <string.h>
//#include <stdlib.h>
//#include "plc_mfrelay.h"
#include <stdio.h>
#include "Revision.h"
#include "Uart_Link.h"

#include "CCO_app.h"
#include "hplc_app_def.h"
#include "hrf_port.h"
#include "../../src/pack2lib/pack2lib.h"
extern phaseRecognizeType ZC_NTB[6]; //ZC_NTB 此处声明成一维数组方便运算
extern STA_INFO  g_StaInfo[TEI_CNT];
//#include "test.h"
//delete & copy
extern u8 topo_delay_times;

ASSOC_RESULT PLC_SmartSelectPco(u16 tei, u16 old_pco, TEI_ZONE candidata_pco[5], TEI_ZONE *p_pco_selected);
void Insert2pco(TEI_ZONE pco, int sta);
P_STA_BITMAP PLC_GetAllSubStas(USHORT tei);
//--------para--------------

//-------start-------
void PLC_HandleAssocReq(MAC_HEAD *pMacHead, u8 *pdata, u16 len,u8 from_link)
void PLC_HandlePcoChangeReq(MAC_HEAD *pMacHead, u8 *pdata, u16 len)
void PLC_HandleHeartbeat(MAC_HEAD *pMacHead, u8 *pdata, u16 len)
void PLC_HandleBeaconFrame(const P_HPLC_PARAM pHplcParam, P_BEACON_ZONE pBeaconZone, P_BEACON_MPDU pBeaconPayload)


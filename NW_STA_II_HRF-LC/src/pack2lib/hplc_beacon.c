#include "System_inc.h"
#include "common_includes.h"
#include <string.h>
#include "../../src/pack2lib/pack2lib.h"



//实际分片信息
typedef struct
{
    u8 Phase;
    u32 SlotLen;
}SLOT_INFO_TMP;

//static SLOT_INFO_TMP SlotInfoTmp[128];

//CSMA时隙信息
typedef union
{
    u32 Data;       //方便交换数据使用
    CSMA_SLOT_FILED Slot;
}BEACON_CSMA_INFO;



void GetBeaconItem(u8* beacon,BEACON_STATION **beaconCap,BEACON_ROUTE** beaconRoute,BEACON_FREQUENCE** beaconFrq,BEACON_SLOT **beaconSlot);

//--------para--------------

static u32 RecentBeaconPeriod = 0; 
static u8 RecentBeacon[600];
static u16 RecentBeaconLen = 0;
SLOT_INFO SlotInfo;
static BEACON_MPDU *s_BeaconMpdu = NULL;
static BEACON_STATION *s_BeaconCap = NULL;
static BEACON_ROUTE *s_BeaconRoute = NULL;
static BEACON_FREQUENCE *s_BeaconFrq = NULL;
static BEACON_SLOT *s_BeaconSlot = NULL;
//-------start-------

static void GetBeaconItem(u8* beacon,BEACON_STATION **beaconCap,BEACON_ROUTE** beaconRoute,BEACON_FREQUENCE** beaconFrq,BEACON_SLOT **beaconSlot)
bool IsNeedSendDiscover(void)
bool IsNeedSendProxy(void)
u32 GetBeaconPeriodLen(void)
bool GetDiscoverSlot(u32 *beginNTB,u32 *endNTB)
bool GetProxySlot(u32 *beginNTB,u32 *endNTB)
bool GetBeaconFrame(int beaconType,BEACON_STATION* beaconCap,u8 *beacon,u16 *len)
u8 GetNTB_Phase(u32 NTB)
void GetBeaconMpduPtr(const BEACON_MPDU **beaconMpdu)
void GetBeaconAbilityPtr(const BEACON_STATION ** beaconCap)
void GetBeaconRoutePtr(const BEACON_ROUTE ** beaconRoute)
void GetBeaconFrequencePtr(const BEACON_FREQUENCE ** beaconFrq)
void GetBeaconSlotPtr(const BEACON_SLOT ** beaconSlot)
void GetBeaconZone(const BEACON_ZONE ** beaconZone)


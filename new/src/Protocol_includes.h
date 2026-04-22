#ifndef _PROTOCOL_INCLUDES_H_
#define _PROTOCOL_INCLUDES_H_

#include <gsmcu_hal.h>
#include  "app_cfg.h"
#include "phy_port.h"

#if (DEV_PURE_DRV==0)
#include "init.h"
#include "hplc_mac_def.h"
#include "hplc_app_def.h"
#include "hplc_frame.h"
#include "hplc_protocol_init.h"
#include "hplc_data.h"
#include "applocal.h"
#include "local_protocol.h"
#include "hplc_app.h"
#include "hplc_phy.h"
#include "hplc_mpdu.h"
#include "hplc_mac.h"
#include "hplc_task.h"
#include "hplc_beacon.h"
#include "hplc_route_period.h"
#include "hplc_timer.h"
#include "hplc_send.h"
#include "hplc_offline.h"
#include "hplc_channel_status.h"
#endif
#if DEV_STA
#include "hplc_update.h"
#include "hplc_test_model.h"
#include "hplc_TMI.h"
#include "hplc_NID.h"
#include "hplc_zone.h"
#include "hplc_zc_manager.h"
#endif

#define DEBUG_SWITCH_LEVEL   2

#define SNR_MAX_CNT 8


//#define HPLC_DEBUG_TRACE


#endif

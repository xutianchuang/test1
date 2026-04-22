#ifndef _COMMON_INCLUDES_H_
#define _COMMON_INCLUDES_H_
#include <gsmcu_hal.h>
//#include "ntwdm700mp.h"
#include "timer.h"
#include "list.h"
#include "tmr_call.h"
#include "fsm.h"
#include "algorithm.h"
#include "map.h"

#define SYS_RESET_POS_0             0
#define SYS_RESET_PHY_INIT_ERR      1

#define SAFE_SYSTEM_RESET(rs)     BSP_safe_reset(rs);

//公共组件模块初始化
void CommonInit(void);



#endif

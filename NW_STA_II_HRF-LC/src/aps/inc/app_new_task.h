#ifndef __APP_NEW_TASK_H__
#define __APP_NEW_TASK_H__
#ifdef __cplusplus
extern "C"
{
#endif

#if defined(ZB204_CHIP)
#include "ZB204.h"
#elif defined(ZB205_CHIP)
#include "ZB205.h"
#endif

#include "gsmcu_hal.h"
bool AppMange(u8 srcAddr[6], u8 dstAddr[6], u8 timeout, u16 seq, u8* data, u16 len, u8 sendType, u16 srcTei);

void NewTaskInit(void);
#ifdef __cplusplus
}
#endif
#endif /* __APP_NEW_TASK_H__ */


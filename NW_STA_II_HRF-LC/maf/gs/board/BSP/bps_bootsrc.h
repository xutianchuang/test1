/*
*
*/
#ifndef __BSP_BOOTSRC_H__
#define __BSP_BOOTSRC_H__
#include  <gsmcu_hal.h>


#define REBOOT_BY_WATCHDOG      0x02
#define REBOOT_BY_HARDWARE      0x01
#define REBOOT_BY_SOFTWARE      0x00


#define GetRebootSource()       (((SCU->BTUP_STS)>>8)&0x03)



#endif

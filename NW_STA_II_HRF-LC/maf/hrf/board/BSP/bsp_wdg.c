/*
*
*/
#include  <gsmcu_hal.h>
#include "bsp_wdg.h"
#include "os.h"
#include "common_includes.h"
#include "gsmcu_iwdg.h"
/*
*
*/
void InitWdg(void)
{
#if defined(INTER_WATCH_DOG)
	WDG_Enable();
	WDG_EnInterrupt();
	NVIC_EnableIRQ(SWT_IRQn);
#else
    WDG_Disable();
#endif
}

void FeedWdg(void)
{
#if defined(INTER_WATCH_DOG)

	//IWDT_KickDog();
    Feed_WDG();
#endif
}

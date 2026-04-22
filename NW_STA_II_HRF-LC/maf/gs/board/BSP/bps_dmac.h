/*
*
*/
#ifndef __BSP_DMAC_H__
#define __BSP_DMAC_H__
#include  <gsmcu_hal.h>

void  BSP_InitDmacVector (void);
void  BSP_IntDmacChannelVectSet (uint32_t DMA_Channelx,CPU_FNCT_VOID  isr);


#endif






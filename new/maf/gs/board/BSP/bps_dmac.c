/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_dmac.h"
#include "os.h"
/*
*
*/
static CPU_FNCT_VOID BSP_DmacVectTbl[DMA_Channel7+1];

/*
*********************************************************************************************************
*********************************************************************************************************
*/
void DMAC_Handler_ISR(void)
{
    uint32_t dmaitstatus = DMA_GetITStatus(DMA);
    uint32_t channelmsk = 0x01;
    for(int i=0;i<= (DMA_Channel7);i++)
    {
        if((dmaitstatus&(channelmsk<< i)) == (channelmsk << i))
        {
            if(BSP_DmacVectTbl[i] != NULL)
            {
                BSP_DmacVectTbl[i]();
            }
			DMA_ClearChannelTCITStatus(DMA,i);
        }
    }
}


void  BSP_InitDmacVector (void)
{
    static bool InitDmacFlg = false;
    if(InitDmacFlg == false)
    {
        InitDmacFlg = true;
        for(int i=0;i<= (DMA_Channel7);i++)
        {
            BSP_DmacVectTbl[i] = NULL;
        }
        BSP_IntVectSet(DMAC_IRQn,DMAC_Handler_ISR);
        //DMA interrupt
        NVIC_ClearPendingIRQ(DMAC_IRQn);
        NVIC_EnableIRQ(DMAC_IRQn);
        //
    }
}
/*
*********************************************************************************************************
*                                            BSP_IntVectSet()
*
* Description : Assign ISR handler.
*
* Argument(s) : int_id      Interrupt for which vector will be set.
*
*               isr         Handler to assign
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_IntDmacChannelVectSet (uint32_t DMA_Channelx,
                      CPU_FNCT_VOID  isr)
{
    CPU_SR_ALLOC();

    if (DMA_Channelx <= DMA_Channel7) {
        CPU_CRITICAL_ENTER();
        BSP_DmacVectTbl[DMA_Channelx] = isr;
        CPU_CRITICAL_EXIT();
    }
}























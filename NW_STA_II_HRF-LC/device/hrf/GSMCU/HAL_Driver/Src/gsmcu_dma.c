/*
 * Copyright: 
 * ----------------------------------------------------------------
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 *   (C) COPYRIGHT 2000,2001 ARM Limited
 *       ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 * ----------------------------------------------------------------
 * File:     dma.c,v
 * Revision: 1.23
 * ----------------------------------------------------------------
 * 
 *  ----------------------------------------
 *  Version and Release Control Information:
 * 
 *  File Name              : dma.c.rca
 *  File Revision          : 1.3
 * 
 *  Release Information    : PrimeCell(TM)-PL080-r1p1-00ltd0
 *  ----------------------------------------
 */

/* 
 * DMA controller code
 */



#include <stdio.h>
#include "apdma.h"
#include "gsmcu_dma.h"
#include "GSMCU_M3.h"

/* 
 * =================== Private Data =============================
 */
uint32_t bit_get_vaile(uint32_t val, uint32_t bs, uint32_t bw)
{
    uint32_t mask = 0;
    for( int i = 0; i < bw; i++ )
    {
        mask <<= 1;
        mask |= 1;
    }
    return (val >> bs) & mask;
}

/*
 * Description:
 *  Array of DMA Controller records.
 */


static DMA_sStateStruct DMA_sState[apOS_DMA_MAXIMUM];


/* 
 * =================== Public Functions =============================
 */

/*
 * --------API (Public) Procedure definitions--------
 */


/* ----- See apdma.h for documentation-------------- */

uint32_t apDMA_StateSizeGet(void)
{

    return sizeof(DMA_sStateStruct);
}


/* ----- See apdma.h for documentation-------------- */

void apDMA_RawISR(void)
{

    /*call the normal ISR entry point*/
    apDMA_IntHandler(0);

}


/* ----- See apdma.h for documentation-------------- */

void apDMA_IntHandler(const uint32_t oInterruptId)
{
    uint32_t                     DmaChan;
    uint32_t                     MaxDmaChan;
    DMA_sPort                   *pRegs;
    uint32_t                     DmaChanBit;
    apError                     RetCode;
    BOOL                        Terminated;
    DMA_sChannelStatus          *pChanRecord;
    BOOL                        ClearINTC = TRUE;
    DMA_sStateStruct            *pCtrlrRecord;

    /* disable the interrupts at the interrupt controller */

    /* Disable Interrupt */
    NVIC_DisableIRQ((IRQn_Type)oInterruptId);



    /* Check the validty of the supplied Ctrlr oId */



    pCtrlrRecord = DMA_sState;

    /* now check that its been initialised & is enabled */

    if( pCtrlrRecord->Initialised && pCtrlrRecord->Enabled )
    {
        pRegs       = (DMA_sPort *)pCtrlrRecord->Base;
        MaxDmaChan  = pCtrlrRecord->sFixedConfig.NumDmaChannels - 1;

        /* find out which channel has interrupted - starting with
         * the highest priority
         */

        for( DmaChan = 0; DmaChan <= MaxDmaChan; DmaChan++ )
        {
            DmaChanBit = DMA_CHAN_BIT(DmaChan);

            if( pRegs->IntStatus & DmaChanBit )
            {
                /* this channel has interrupted - now find out why */

                if( pRegs->IntErrorStatus & DmaChanBit )
                {
                    /* An error has occured */

                    RetCode     = (apError)apERR_DMA_TRANSFER_ERROR;
                    Terminated  = TRUE;
                }
                else
                {
                    /* Assume Transfer Complete (the only alternative) */

                    if( pRegs->ActiveChannels & DmaChanBit )
                    {
                        /* the channel is still running */

                        pChanRecord = &pCtrlrRecord->sChans[DmaChan];

                        if( pChanRecord->eState ==
                                DMA_STATE_ACTIVE_LOW_MULTI_LLI_TRANSFER ||
                            pChanRecord->eState ==
                                DMA_STATE_ACTIVE_LLI_TRANSFER )
                        {
                            /* channel record shows the channel is doing a LLI transfer */

                            RetCode     = (apError)apERR_DMA_TRANSFER_LLI_COMPLETE;
                            Terminated  = FALSE;
                        }
                        else
                        {
                            /* Channel idle or not doing a LLI Transfer - terminate it. */

                            RetCode     = (apError)apERR_DMA_TRANSFER_COMPLETE;
                            Terminated  = TRUE;
                        }
                    }
                    else
                    {
                        /* channel has stopped */
                        RetCode     = (apError)apERR_DMA_TRANSFER_COMPLETE;
                        Terminated  = TRUE;
                    }
                }

                /* Call the generic transfer complete fn which will inform the user
                 * & update the channel/peripheral records & clear the interrupt.
                 */

                DMA_TransferComplete(pCtrlrRecord,
                                     DmaChan,
                                     Terminated,
                                     RetCode,
                                     DMA_USER_NONE);

                /* now we have to decide whether to service all the
                 * pending interrupts, or just the highest priority one.
                 * Currently - we just do the highest priority.
                 */

                break;
            }
        }

        if( pRegs->IntStatus )
        {
            /* We still have a pending interrupt, so we won't
             * clear the interrupt at the interrupt controller.
             * Which means it will interrupt again as soon as we
             * tell it to enable interrupts. Doing this means any
             * other peripherals with higher priority interrupts
             * get serviced first.
             */

            ClearINTC = FALSE;
        }
    }

    if( ClearINTC )
    {
        /*clear the interrupt at the interrupt controller */
        NVIC_ClearPendingIRQ((IRQn_Type)oInterruptId);
    }

    /* enable the interrupts at the interrupt controller */
    NVIC_EnableIRQ((IRQn_Type)oInterruptId);

}


/*              --- General Functions  --- */

/* ----- See apdma.h for documentation--------------*/
void apDMA_Channel_defultWidth(int width)
{
    DMA_sState->sDefaultMemAccess.eWidth=(apDMA_eWidth)width;
}
void apDMA_Initialize(uint32_t        BaseAddress,
                      uint32_t                         Interrupts,
                      apDMA_sCtrlrConfig              *pInitial)
{
    //apError     RetCode = apERR_NONE;   /* Default return code */
    uint32_t      J;

    DMA_sStateStruct                *pCtrlrRecord;
    DMA_sChannelStatus              *pChanRecord;
    DMA_sPeriphInfo                 *pPeriphRecord;





    /* Obtain controller record */
    pCtrlrRecord = DMA_sState;

    /* Initialise the DMA Ctrlr array to default values. */

    pCtrlrRecord->Initialised = FALSE;
    pCtrlrRecord->Enabled = FALSE;

    pChanRecord = &pCtrlrRecord->sChans[0];

    for( J = 0; J < apDMA_MAX_CHANNELS; J++ )
    {
        pChanRecord->eUser         = DMA_USER_NONE;
        pChanRecord->eState        = DMA_STATE_IDLE;
        pChanRecord->rTerminatedCb = (apDMA_rTransferTerminated)NULL;
        pChanRecord++;
    }

    pPeriphRecord = &pCtrlrRecord->sPeriphs[0];

    for( J = 0; J < apDMA_MAX_PERIPHERALS; J++ )
    {
        pPeriphRecord->eConfig          = DMA_PERIPH_NOT_CONFIGURED;
        pPeriphRecord->UseCallBack      = FALSE;
        pPeriphRecord->CurrentDmaChannel = apDMA_NO_DMA_CHANNEL;
        pPeriphRecord->rTerminatedCb    = (apDMA_rTransferTerminated)NULL;
        pPeriphRecord++;
    }

    /*  Copy the essential config fields from the supplied config
     *  data into DMA_sCtrlrs[] 
     */
    pCtrlrRecord->Base              = BaseAddress;
    pCtrlrRecord->AHB1BigEndian     = pInitial->AHB1BigEndian;
    pCtrlrRecord->AHB2BigEndian     = pInitial->AHB2BigEndian;
    pCtrlrRecord->Enabled           = TRUE;

    /* Then configure the controller & enable it - which only
     * requires the above fields
     */

    DMA_ConfigureController(pCtrlrRecord);

    /* Now read the fixed info from the DMA chip */
    DMA_ReadControllerFixedData(pCtrlrRecord);

    /* Now check validity of the remainder of the supplied config record */
    if( DMA_ValidMemAccessDetails(pCtrlrRecord, &pInitial->sDefaultMemAccess) )
    {
        /*  Copy the supplied config data into DMA_sCtrlrs[] */

        pCtrlrRecord->AHB1BigEndian = pInitial->AHB1BigEndian;
        pCtrlrRecord->AHB2BigEndian = pInitial->AHB2BigEndian;

        pCtrlrRecord->sDefaultMemAccess.eAhbBus =
            pInitial->sDefaultMemAccess.eAhbBus;
        pCtrlrRecord->sDefaultMemAccess.eWidth =
            pInitial->sDefaultMemAccess.eWidth;
        pCtrlrRecord->sDefaultMemAccess.eBurstSize =
            pInitial->sDefaultMemAccess.eBurstSize;
        pCtrlrRecord->NumAddressBlocks  = 0;

        /* Now initialise the DMA controller & its channels */
        DMA_InitializeController(pCtrlrRecord);

        /* Mark this ctrlr as initialised */
        pCtrlrRecord->Initialised = TRUE;

        /* Register the interrupt handler and enable the interrupt
         * on the interrupt controller for each source
         */
        //apBIND_ALL_INTERRUPTS(apDMA_IntHandler, apDMA_RawISR);

    }
    else
    {
        /* Invalid data in supplied config */
        //RetCode = apERR_BAD_PARAMETER;
    }
    NVIC_EnableIRQ((IRQn_Type)Interrupts);
/* apDMA_Initialize cannot return error code in current version of API */
/*    return RetCode; */

}

/* ----- See apdma.h for documentation--------------*/

apError apDMA_VirtualMemoryTable(
    uint32_t                             NumAddressBlocks,
    apDMA_sAddressBlockCharacteristics  *pAddressBlocks)
{
    apError          RetCode;
    DMA_sStateStruct *pCtrlrRecord;

#ifdef apDEBUG_ENABLED    /* Only do validation when compiled with debug on */

    uint32_t I;
    apDMA_sAddressBlockCharacteristics  *pAddressBlock;
    apDMA_sXferCharacteristics          *pXferInfo;

#endif  /* apDEBUG_ENABLED */

    /* Check the ctrlr Id is in range and the ctrlr has been initialised,
     * and also get a pointer to the appropriate ctrlr record.
     */
    RetCode = DMA_GeneralEntry(&pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {

#ifdef apDEBUG_ENABLED    /* Only do validation when compiled with debug on */

        /* Need to check the Address Block table */

        if( NumAddressBlocks )
        {
            pAddressBlock = pAddressBlocks;

            for( I = 0; I < NumAddressBlocks; I++ )
            {
                pXferInfo = &pAddressBlock->sXferAccess;

                if( !DMA_ValidMemAccessDetails(pCtrlrRecord, pXferInfo) )
                {
                    /* Something invalid - reject the lot */
                    RetCode = apERR_BAD_PARAMETER;
                    break;
                }
                else
                {
                    pAddressBlock++;   /* Next one */
                }
            }
        }

#endif  /* apDEBUG_ENABLED */

        /* If all OK we'll make a copy of the params, but we can not make
         * a copy of the table because it could be any size and we
         * can not use the heap. This means the table mustn't be altered.
         */

        if( RetCode == apERR_NONE )
        {
            pCtrlrRecord->NumAddressBlocks = NumAddressBlocks;
            pCtrlrRecord->pAddressBlocks = pAddressBlocks;
        }
    }

    return RetCode;
}

/* ----- See apdma.h for documentation--------------*/

apError apDMA_ControllerEnable()
{
    apError           RetCode;
    DMA_sStateStruct  *pCtrlrRecord;

    /* Check the ctrlr Id is in range and the ctrlr has been initialised,
     * and also get a pointer to the appropriate ctrlr record.
     */
    RetCode = DMA_GeneralEntry(&pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {
        if( !pCtrlrRecord->Enabled )
        {
            pCtrlrRecord->Enabled = TRUE;

            DMA_ConfigureController(pCtrlrRecord);

            DMA_InitializeController(pCtrlrRecord);
        }
    }

    return RetCode;
}

/* ----- See apdma.h for documentation--------------*/

apError apDMA_ControllerDisable()
{
    apError          RetCode;
    DMA_sStateStruct *pCtrlrRecord;
    int32_t           I;
    int32_t           NumChans;
    DMA_sPort        *pCtrlr;


    /* Check the ctrlr Id is in range and the ctrlr has been initialised,
     * and also get a pointer to the appropriate ctrlr record.
     */
    RetCode = DMA_GeneralEntry(&pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {
        if( pCtrlrRecord->Enabled )
        {
            pCtrlr      = (DMA_sPort *)pCtrlrRecord->Base;
            NumChans    = (int32_t)pCtrlrRecord->sFixedConfig.NumDmaChannels;

            /* Check that all channels are idle */
            for( I = 0; I < NumChans; I++ )
            {
                if( pCtrlrRecord->sChans[I].eState != DMA_STATE_IDLE )
                {


                    RetCode = (apError)apERR_DMA_CTRLR_ACTIVE;
                    break;
                }
                else
                {
                    /* Make sure the DMA channel is disabled */
                    pCtrlr->sDmaChannels[I].Configuration = 0;
                }
            }

            if( RetCode == apERR_NONE )
            {
                /* Might be a good idea to disable the interrupt controller
                 * however we don't have a copy of our
                 * const uint32_t array.
                 */

                /* clear out any nasty interrupts that might be lurking */
                pCtrlr->IntErrorClear   = apBITS_ALL;
                pCtrlr->IntTCClear      = apBITS_ALL;

                /* Disable the controller */
                pCtrlrRecord->Enabled = FALSE;

                DMA_ConfigureController(pCtrlrRecord);
            }
        }
    }

    return RetCode;
}

/*              ---  API Functions --- */

/* ----- See apdma.h for documentation--------------*/

apError apDMA_PeripheralConfigure(
    uint32_t          PeripheralId,
    apDMA_eBurstSize eBurstSize,
    apDMA_eWidth     eWidth,
    apDMA_eAhbBus    eAhbBus,
    apDMA_eSync      eDmaSync,
    uint32_t          Protection)
{
    apError                     RetCode;
    DMA_sPeriphInfo             *pPeriphRecord;
    DMA_sStateStruct            *pCtrlrRecord;
    apDMA_sXferCharacteristics  sXferAccess;
    DMA_sPort                   *pRegs;
    uint32_t                     RegisterValue;

    /* Check the ctrlr Id is in range and the ctrlr has been initialised,
     * and also get a pointer to the appropriate ctrlr record.
     */

    RetCode = DMA_GeneralEntry(&pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {
        /* Check PeripheralId is valid */

        if( PeripheralId < pCtrlrRecord->sFixedConfig.NumPeripherals )
        {
            pPeriphRecord   = &pCtrlrRecord->sPeriphs[PeripheralId];

            sXferAccess.eAhbBus      = eAhbBus;
            sXferAccess.eWidth       = eWidth;
            sXferAccess.eBurstSize   = eBurstSize;

            /* Check validity of the params */

            if( DMA_ValidMemAccessDetails(pCtrlrRecord, &sXferAccess) )
            {
                /* update the remainder of the peripheral record */

                pPeriphRecord->sXferAccess.eAhbBus      = eAhbBus;
                pPeriphRecord->sXferAccess.eWidth       = eWidth;
                pPeriphRecord->sXferAccess.eBurstSize   = eBurstSize;

                /* Update Synchronisation Register */

                pRegs = (DMA_sPort *)pCtrlrRecord->Base;

                RegisterValue = pRegs->Sync;

                if( eDmaSync == apDMA_SYNC_DISABLE )
                {
                    RegisterValue = RegisterValue | ((uint32_t)1U << PeripheralId);
                }
                else
                {
                    RegisterValue = RegisterValue & ~((uint32_t)1U << PeripheralId);
                }

                pRegs->Sync = RegisterValue;

                if( Protection <= (uint32_t)apDMA_PROTECTION_VALID )
                {
                    /* Update protection bits */

                    pPeriphRecord->Protection   = Protection;

                    /* Update the peripheral's configure state */

                    if( pPeriphRecord->eConfig == DMA_PERIPH_NOT_CONFIGURED )
                    {
                        pPeriphRecord->eConfig = DMA_PERIPH_OS_CONFIG;
                    }
                    else if( pPeriphRecord->eConfig == DMA_PERIPH_PERIPH_CONFIG )
                    {
                        pPeriphRecord->eConfig = DMA_PERIPH_FULLY_CONFIGURED;
                    }
                }
                else
                {
                    /* invalid Protection bits */
                    RetCode = apERR_BAD_PARAMETER;
                }
            }
            else
            {
                RetCode = apERR_BAD_PARAMETER;
            }
        }
        else
        {
            /* invalid DMA Peripheral Id. */
            RetCode = apERR_BAD_PARAMETER;
        }
    }

    return RetCode;
}

/* ----- See apdma.h for documentation--------------*/

apError apDMA_PeripheralAddressSet(
    uint32_t                      PeripheralId,
    void                         *pRegisterAddress,
    apDMA_rTransferTerminated    rTerminatedCb)
{
    apError             RetCode;
    DMA_sPeriphInfo     *pPeriphRecord;
    DMA_sStateStruct    *pCtrlrRecord;

    /* Check the ctrlr Id is in range and the ctrlr has been initialised,
     * and also get a pointer to the appropriate ctrlr record.
     */

    RetCode = DMA_GeneralEntry(&pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {
        /* Check PeripheralId is valid */

        if( PeripheralId < pCtrlrRecord->sFixedConfig.NumPeripherals )
        {
            pPeriphRecord = &pCtrlrRecord->sPeriphs[PeripheralId];

            /* Copy the params to the peripheral record */

            pPeriphRecord->pRegisterAddress = pRegisterAddress;
            pPeriphRecord->rTerminatedCb    = rTerminatedCb;

            /* Update the peripheral's configure state */

            if( pPeriphRecord->eConfig == DMA_PERIPH_NOT_CONFIGURED )
            {
                pPeriphRecord->eConfig = DMA_PERIPH_PERIPH_CONFIG;
            }
            else if( pPeriphRecord->eConfig == DMA_PERIPH_OS_CONFIG )
            {
                pPeriphRecord->eConfig = DMA_PERIPH_FULLY_CONFIGURED;
            }
        }
        else
        {
            /* invalid DMA Peripheral Id. */
            RetCode = apERR_BAD_PARAMETER;
        }
    }

    return RetCode;
}

/* ----- See apdma.h for documentation--------------*/

apError apDMA_PeripheralProtectionSet(
    uint32_t      PeripheralId,
    uint32_t       Protection)
{
    apError     RetCode;
    DMA_sStateStruct  *pCtrlrRecord;

    /* Check the ctrlr Id is in range and the ctrlr has been initialised,
     * and also get a pointer to the appropriate ctrlr record.
     */

    RetCode = DMA_GeneralEntry(&pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {
        if( PeripheralId < pCtrlrRecord->sFixedConfig.NumPeripherals )
        {
            /* check the peripheral has been fully configured */

            if( pCtrlrRecord->sPeriphs[PeripheralId].eConfig ==
                    DMA_PERIPH_FULLY_CONFIGURED )
            {
                if( Protection <= (uint32_t)apDMA_PROTECTION_VALID )
                {
                    pCtrlrRecord->sPeriphs[PeripheralId].Protection = Protection;
                }
                else
                {
                    /* Invalid protection bits requested */
                    RetCode = apERR_BAD_PARAMETER;
                }
            }
            else
            {
                RetCode = (apError)apERR_DMA_PERIPHERAL_NOT_INITIALISED;
            }
        }
        else
        {
            RetCode = apERR_BAD_PARAMETER;
        }
    }

    return RetCode;
}

/* ----- See apdma.h for documentation--------------*/

apError apDMA_PeripheralToPeripheralTransferRequest(
    uint32_t                 ChanId,
    uint32_t                 SrcPeriphId,
    uint32_t                 DstPeriphId,
    uint32_t                 UsersId,
    uint32_t                 NumTransfers,
    apDMA_eFlowControl      eFlowCtrl,
    uint32_t                  Protection)
{
    apError             RetCode;

    DMA_sPeriphInfo     *pSrcPeriphRecord;
    DMA_sPeriphInfo     *pDstPeriphRecord;
    DMA_sChannelStatus  *pChanRecord;
    DMA_sStateStruct    *pCtrlrRecord;

    /* Check the ctrlr Id is in range and the ctrlr has been initialised,
     * and also get a pointer to the appropriate ctrlr record.
     */

    RetCode = DMA_ValidateChannelEntry(ChanId, &pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {
        /* Check peripheral ids are valid */

        if( SrcPeriphId < pCtrlrRecord->sFixedConfig.NumPeripherals &&
            DstPeriphId < pCtrlrRecord->sFixedConfig.NumPeripherals )
        {
            pSrcPeriphRecord = &pCtrlrRecord->sPeriphs[SrcPeriphId];
            pDstPeriphRecord = &pCtrlrRecord->sPeriphs[DstPeriphId];

            /* Check the peripheral records have been configured */

            if( pSrcPeriphRecord->eConfig == DMA_PERIPH_FULLY_CONFIGURED &&
                pDstPeriphRecord->eConfig == DMA_PERIPH_FULLY_CONFIGURED )
            {
                /*  Check that the peripherals are not currently 
                 *  assinged to use any DMA channel
                 */
                if( pSrcPeriphRecord->CurrentDmaChannel == apDMA_NO_DMA_CHANNEL &&
                    pDstPeriphRecord->CurrentDmaChannel == apDMA_NO_DMA_CHANNEL )
                {
                    pChanRecord = &pCtrlrRecord->sChans[ChanId];

                    /*  Check that the channel is idle */

                    if( pChanRecord->eState == DMA_STATE_IDLE )
                    {
                        /*  update the channel record */

                        pChanRecord->SrcPeripheralId = SrcPeriphId;
                        pChanRecord->DstPeripheralId = DstPeriphId;
                        pChanRecord->eFlowCtrl       = eFlowCtrl;
                        pChanRecord->Protection      = Protection;
                        pChanRecord->NumTransfers    = NumTransfers;
                        pChanRecord->LockEnable      = FALSE;
                        pChanRecord->UsersId         = UsersId;
                        pChanRecord->rTerminatedCb   = NULL;
                        pChanRecord->eUser           = DMA_USER_APPLICATION;

                        /* Periph to Periph transfer */

                        DMA_PeriphToPeriphTransfer(pCtrlrRecord, ChanId);
                    }
                    else
                    {
                        /* The channel is busy  */
                        RetCode = apERR_BUSY;
                    }
                }
                else
                {
                    /* One or more peripherals is busy  */
                    RetCode = apERR_BUSY;
                }
            }
            else
            {
                /* One or more peripherals are not initialised */
                RetCode = (apError)apERR_DMA_PERIPHERAL_NOT_INITIALISED;
            }
        }
        else
        {
            /* invalid DMA Peripheral Id */
            RetCode = apERR_BAD_PARAMETER;
        }
    }

    return RetCode;
}

/* ----- See apdma.h for documentation--------------*/

apError apDMA_MemPeripheralTransferRequest(
    uint32_t             ChanId,
    uint32_t             PeripheralId,
    uint32_t             UsersId,
    uint32_t             NumTransfers,
    apDMA_eFlowControl  eFlowCtrl,
    void                *pMemAddr)
{
    apError            RetCode;
    DMA_sStateStruct   *pCtrlrRecord;
    DMA_sChannelStatus *pChanRecord;
    DMA_sPeriphInfo    *pPeriphRecord;

    apDMA_sPerMemLLI sTempLLI;

    /* Check the ctrlr Id is in range and the ctrlr has been initialised,
     * and also get a pointer to the appropriate ctrlr record.
     */

    RetCode = DMA_ValidateChannelEntry(ChanId, &pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {
        /* Check peripheral id is valid */

        if( PeripheralId < pCtrlrRecord->sFixedConfig.NumPeripherals )
        {
            pPeriphRecord = &pCtrlrRecord->sPeriphs[PeripheralId];

            /* Check the peripheral record has been configured */

            if( pPeriphRecord->eConfig == DMA_PERIPH_FULLY_CONFIGURED )
            {
                /*  Check that the peripheral is not currently 
                 *  assinged to use any DMA channel
                 */
                if( pPeriphRecord->CurrentDmaChannel == apDMA_NO_DMA_CHANNEL )
                {
                    pChanRecord = &pCtrlrRecord->sChans[ChanId];

                    /*  Check that the channel is idle */

                    if( pChanRecord->eState == DMA_STATE_IDLE )
                    {
                        /*  update the channel record */

                        pChanRecord = &pCtrlrRecord->sChans[ChanId];

                        pChanRecord->eFlowCtrl      = eFlowCtrl;
                        pChanRecord->UsersId        = UsersId;
                        pChanRecord->CbPerItem      = TRUE;

                        /* Fill in a local single LLI */

                        sTempLLI.NumTransfers   = NumTransfers;
                        sTempLLI.pMemAddress    = pMemAddr;
                        sTempLLI.pNextLLI       = (apDMA_sPerMemLLI *)NULL;

                        /* get a lower level, mem/peripheral linked list function
                         * to do all the work - but we only pass it one LLI.
                         */
                        RetCode = DMA_LinkedMemPeripheralTransfer(pCtrlrRecord,
                                                                  PeripheralId,
                                                                  ChanId,
                                                                  1,
                                                                  &sTempLLI);
                    }
                    else
                    {
                        /* The channel is busy  */
                        RetCode = apERR_BUSY;
                    }
                }
                else
                {
                    /* Peripheral is busy  */
                    RetCode = apERR_BUSY;
                }
            }
            else
            {
                /* Peripheral is not initialised */
                RetCode = (apError)apERR_DMA_PERIPHERAL_NOT_INITIALISED;
            }
        }
        else
        {
            /* invalid DMA Peripheral Id */
            RetCode = apERR_BAD_PARAMETER;
        }
    }

    return RetCode;
}

/* ----- See apdma.h for documentation--------------*/

apError apDMA_LinkedMemPeripheralTransferRrequest(

    uint32_t                 ChanId,
    uint32_t                 PeripheralId,
    uint32_t                 UsersId,
    apDMA_eFlowControl      eFlowCtrl,
    uint32_t                 NumLLItems,
    apDMA_sPerMemLLI        *pFirstLLI,
    BOOL                    CbPerItem)
{
    apError            RetCode;
    DMA_sStateStruct   *pCtrlrRecord;
    DMA_sChannelStatus *pChanRecord;
    DMA_sPeriphInfo    *pPeriphRecord;

    /* Check the ctrlr Id is in range and the ctrlr has been initialised,
     * and also get a pointer to the appropriate ctrlr record.
     */

    RetCode = DMA_ValidateChannelEntry(ChanId, &pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {
        /* Check peripheral id is valid */

        if( PeripheralId < pCtrlrRecord->sFixedConfig.NumPeripherals )
        {
            pPeriphRecord = &pCtrlrRecord->sPeriphs[PeripheralId];

            /* Check the peripheral record has been configured */

            if( pPeriphRecord->eConfig == DMA_PERIPH_FULLY_CONFIGURED )
            {
                /*  Check that the peripheral is not currently 
                 *  assinged to use any DMA channel
                 */
                if( pPeriphRecord->CurrentDmaChannel == apDMA_NO_DMA_CHANNEL )
                {
                    pChanRecord = &pCtrlrRecord->sChans[ChanId];

                    /*  Check that the channel is idle */

                    if( pChanRecord->eState == DMA_STATE_IDLE )
                    {
                        /*  update the channel record */

                        pChanRecord = &pCtrlrRecord->sChans[ChanId];

                        pChanRecord->eFlowCtrl  = eFlowCtrl;
                        pChanRecord->UsersId    = UsersId;
                        pChanRecord->CbPerItem  = CbPerItem;

                        /* Now get a lower level function to do all the work */

                        RetCode = DMA_LinkedMemPeripheralTransfer(pCtrlrRecord,
                                                                  PeripheralId,
                                                                  ChanId,
                                                                  NumLLItems,
                                                                  pFirstLLI);
                    }
                    else
                    {
                        /* The channel is busy  */
                        RetCode = apERR_BUSY;
                    }
                }
                else
                {
                    /* Peripheral is busy  */
                    RetCode = apERR_BUSY;
                }
            }
            else
            {
                /* Peripheral is not initialised */
                RetCode = (apError)apERR_DMA_PERIPHERAL_NOT_INITIALISED;
            }
        }
        else
        {
            /* invalid DMA Peripheral Id */
            RetCode = apERR_BAD_PARAMETER;
        }
    }

    return RetCode;
}

/* ----- See apdma.h for documentation--------------*/

apError apDMA_MemToMemTransferRequest(
    uint32_t                      ChanId,
    uint32_t                      UsersId,
    apDMA_rTransferTerminated    rTerminatedCb,
    uint32_t                       Protection,
    uint32_t                      NumTransfers,
    void                         *pSource,
    void                         *pDest)
{
    apError            RetCode;
    DMA_sStateStruct   *pCtrlrRecord;
    DMA_sChannelStatus *pChanRecord;

    apDMA_sMemMemLLI   sTempLLI;

    /* Get the DMA controller record and simultaneously check that 
     * the channel is valid
     */

    RetCode = DMA_ValidateChannelEntry(ChanId, &pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {
        pChanRecord = &pCtrlrRecord->sChans[ChanId];

        /*  Check that the channel is idle */

        if( pChanRecord->eState == DMA_STATE_IDLE )
        {
            /*  update the channel record */

            pChanRecord->UsersId       = UsersId;
            pChanRecord->rTerminatedCb = rTerminatedCb;
            pChanRecord->Protection    = Protection;
            pChanRecord->CbPerItem     = TRUE;
            pChanRecord->eUser         = DMA_USER_APPLICATION;

            /*  Create a single local LLI, fill it with the details, and pass it
             *  to the private code that handles Linked List Mem to Mem transfers.
             */

            sTempLLI.NumTransfers   = NumTransfers;
            sTempLLI.pSrcMemAddr    = pSource;
            sTempLLI.pDstMemAddr    = pDest;
            sTempLLI.pNextLLI       = (apDMA_sMemMemLLI *)NULL;

            RetCode = DMA_LinkedMemToMemTransfer(pCtrlrRecord,
                                                 ChanId,
                                                 1,
                                                 &sTempLLI);
        }
        else
        {
            RetCode = apERR_BUSY;
        }
    }

    return RetCode;
}

/* ----- See apdma.h for documentation--------------*/

apError apDMA_LinkedMemToMemTransferRequest(
    uint32_t                    ChanId,
    uint32_t                    UsersId,
    apDMA_rTransferTerminated  rTerminatedCb,
    uint32_t                     Protection,
    uint32_t                    NumLLItems,
    apDMA_sMemMemLLI           *pFirstLLI,
    BOOL                       CbPerItem)
{
    apError            RetCode;
    DMA_sStateStruct   *pCtrlrRecord;
    DMA_sChannelStatus *pChanRecord;

    /* Get the DMA controller record and simultaneously check that 
     * the Channel is valid 
     */

    RetCode = DMA_ValidateChannelEntry(ChanId, &pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {
        pChanRecord = &pCtrlrRecord->sChans[ChanId];

        /*  Check that the channel is idle */

        if( pChanRecord->eState == DMA_STATE_IDLE )
        {
            /*  update the channel record */

            pChanRecord->UsersId       = UsersId;
            pChanRecord->rTerminatedCb = rTerminatedCb;
            pChanRecord->Protection    = Protection;
            pChanRecord->CbPerItem      = CbPerItem;
            pChanRecord->eUser         = DMA_USER_APPLICATION;

            /* Call low level function which does the actual work. */

            RetCode = DMA_LinkedMemToMemTransfer(pCtrlrRecord,
                                                 ChanId,
                                                 NumLLItems,
                                                 pFirstLLI);
        }
        else
        {
            RetCode = apERR_BUSY;
        }
    }

    return RetCode;
}

/* ----- See apdma.h for documentation--------------*/

apError apDMA_AbortTransfer(
    uint32_t      ChanId,
    uint32_t      *const pCount)
{
    apError           RetCode;
    uint32_t           Count = 0;
    DMA_sStateStruct *pCtrlrRecord;

    RetCode = DMA_ValidateChannelEntry(ChanId, &pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {
        /* Call a lower level function to do the actual termination. It
         * will notify the user as appropriate and mark the channel and
         * peripheral records as idle
         */

        Count = DMA_TransferComplete(pCtrlrRecord,
                                     ChanId,
                                     TRUE,               /* Terminated */
                                     (apError)apERR_DMA_TRANSFER_ABORTED,
                                     DMA_USER_APPLICATION);
    }
    if( pCount != NULL )
    {
        *pCount = Count;
    }

    return RetCode;
}

static uint32_t DMA_TransfersCount(DMA_sStateStruct *pCtrlrRecord,
								   uint32_t          ChanId)
{
	uint32_t                 Count = 0;  /* Default no. transfers completed */

	DMA_sChannelStatus      *pChanRecord;
	DMA_sPort               *pRegs;
	DMA_sChannel            *pChanRegs;
	apDMA_eFlowControl      eFlowCtrl;

	DMA_eState              eState;

	/* Channel must be in range */
	if (ChanId < pCtrlrRecord->sFixedConfig.NumDmaChannels)
	{
		pChanRecord = &pCtrlrRecord->sChans[ChanId];
		pRegs       = (DMA_sPort *)pCtrlrRecord->Base;
		pChanRegs   = &pRegs->sDmaChannels[ChanId];
		eState      = pChanRecord->eState;

		/* Channel should be marked as in use */
		if (eState != DMA_STATE_IDLE)
		{

			//pChanRegs->Configuration = 0;   /* disable the channel */

			/* Can we work out how many transfers have been completed */
			if (eState == DMA_STATE_ACTIVE_TRANSFER ||
				eState == DMA_STATE_ACTIVE_P_TO_P_TRANSFER ||
				eState == DMA_STATE_ACTIVE_LOW_TRANSFER)
			{
				eFlowCtrl = pChanRecord->eFlowCtrl;

				if (eFlowCtrl == apDMA_MEM_TO_PERIPHERAL_DMA_CTRL ||
					eFlowCtrl == apDMA_PERIPHERAL_TO_MEM_DMA_CTRL ||
					eFlowCtrl == apDMA_PERIPHERAL_TO_PERIPHERAL_DMA_CTRL ||
					eFlowCtrl == apDMA_MEM_TO_MEM_DMA_CTRL)
				{
					Count = pChanRecord->NumTransfers -
						apBIT_GET(pChanRegs->Control, DMA_TRANSFER_SIZE);
				}
			}

		}
		else
		{
		}
	}

	return Count;
}
apError apDMA_GetCount(
    uint32_t      ChanId,
    uint32_t      *const pCount)
{
	apError           RetCode;
	uint32_t           Count = 0;
	DMA_sStateStruct *pCtrlrRecord;

	RetCode = DMA_ValidateChannelEntry(ChanId, &pCtrlrRecord);

	if (RetCode == apERR_NONE)
	{
		/* Call a lower level function to do the actual termination. It
		 * will notify the user as appropriate and mark the channel and
		 * peripheral records as idle
		 */

		Count = DMA_TransfersCount(pCtrlrRecord,
								   ChanId);
	}
	if (pCount != NULL)
	{
		*pCount = Count;
	}

	return RetCode;
}

/* ----- See apdma.h for documentation--------------*/

apError apDMA_HaltTransfer(
    uint32_t      ChanId,
    uint32_t      *const pCount)
{
    apError           RetCode;
    uint32_t           Count = 0;
    DMA_sStateStruct *pCtrlrRecord;

    DMA_sPort               *pRegs;
    DMA_sChannel            *pChanRegs;

    RetCode = DMA_ValidateChannelEntry(ChanId, &pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {
        pRegs       = (DMA_sPort *)pCtrlrRecord->Base;
        pChanRegs   = &pRegs->sDmaChannels[ChanId];

        /* Set HALT bit in configuration register */

        pChanRegs->Configuration|=1<<bsDMA_HALT;

        /* Wait for FIFO to become inactive */

        while( pChanRegs->Configuration& (1<<bsDMA_ACTIVE) )
        {
        }

        /* Call a lower level function to do the actual termination. It
         * will notify the user as appropriate and mark the channel and
         * peripheral records as idle
         */

        Count = DMA_TransferComplete(pCtrlrRecord,
                                     ChanId,
                                     TRUE,               /* Terminated */
                                     (apError)apERR_DMA_TRANSFER_ABORTED,
                                     DMA_USER_APPLICATION);
    }
    if( pCount != NULL )
    {
        *pCount = Count;
    }

    return RetCode;
}

/*          --- Low Level API --- */

/* ----- See apdma.h for documentation--------------*/

apError apDMA_SetupDMATransfer(
    uint32_t         DmaChannel,
    apDMA_sTransfer *pParam)
{
    apError             RetCode;

    DMA_sStateStruct    *pCtrlrRecord;
    DMA_sChannelStatus  *pChanRecord;

    /* Check the ctrlr Id and channel specified are valid */

    RetCode = DMA_ValidateChannelEntry(DmaChannel, &pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {
        pChanRecord = &pCtrlrRecord->sChans[DmaChannel];

        /* Check the channel is Idle */

        if( pChanRecord->eState == DMA_STATE_IDLE )
        {
            RetCode = DMA_LowLevelTransfer(pCtrlrRecord,
                                           DmaChannel,
                                           pParam);
        }
        else
        {
            RetCode = apERR_BUSY;
        }
    }

    return RetCode;
}

/* 
 * =================== Private Functions =============================
 */

/* ----- See dma.h for documentation--------------*/

static uint32_t DMA_TransferComplete(DMA_sStateStruct *pCtrlrRecord,
                                     uint32_t          ChanId,
                                     BOOL             Terminated,
                                     apError          RetCode,
                                     DMA_eUser        eCaller)
{
    uint32_t                 Count = 0;  /* Default no. transfers completed */
    uint32_t                 SrcPeriphId;
    uint32_t                 DstPeriphId;
    DMA_sChannelStatus      *pChanRecord;
    DMA_sPort               *pRegs;
    DMA_sChannel            *pChanRegs;
    apDMA_eFlowControl      eFlowCtrl;
    DMA_eUser               eUser;
    DMA_eState              eState;

    uint32_t                 ChanBit;
    uint32_t                 ChanUsersId = 0;
    uint32_t                 SrcUsersId;
    uint32_t                 DstUsersId;

    apDMA_rTransferTerminated rChanTerminatedCb =
        (apDMA_rTransferTerminated)NULL;
    apDMA_rTransferTerminated rSrcTerminatedCb =
        (apDMA_rTransferTerminated)NULL;
    apDMA_rTransferTerminated rDstTerminatedCb =
        (apDMA_rTransferTerminated)NULL;

    /* Channel must be in range */
    if( ChanId < pCtrlrRecord->sFixedConfig.NumDmaChannels )
    {
        ChanBit     = DMA_CHAN_BIT(ChanId);
        pChanRecord = &pCtrlrRecord->sChans[ChanId];
        pRegs       = (DMA_sPort *)pCtrlrRecord->Base;
        pChanRegs   = &pRegs->sDmaChannels[ChanId];
        eState      = pChanRecord->eState;

        /* Channel should be marked as in use */
        if( eState != DMA_STATE_IDLE )
        {
            if( Terminated )
            {
                pChanRegs->Configuration = 0;   /* disable the channel */

                /* Can we work out how many transfers have been completed */
                if( eState == DMA_STATE_ACTIVE_TRANSFER ||
                    eState == DMA_STATE_ACTIVE_P_TO_P_TRANSFER ||
                    eState == DMA_STATE_ACTIVE_LOW_TRANSFER )
                {
                    eFlowCtrl = pChanRecord->eFlowCtrl;

                    if( eFlowCtrl == apDMA_MEM_TO_PERIPHERAL_DMA_CTRL ||
                        eFlowCtrl == apDMA_PERIPHERAL_TO_MEM_DMA_CTRL ||
                        eFlowCtrl == apDMA_PERIPHERAL_TO_PERIPHERAL_DMA_CTRL ||
                        eFlowCtrl == apDMA_MEM_TO_MEM_DMA_CTRL )
                    {
                        /* DMA is flow controller, so the transfer size in
                         * the Channel Control register should be correct.
                         * In all other states, it is not possible 
                         * to work out how many transfers have occured.
                         */
                        Count = pChanRecord->NumTransfers -
                            apBIT_GET(pChanRegs->Control, DMA_TRANSFER_SIZE);
                    }
                }
            }

            /* Make a note of the src & dst periph Ids */
            SrcPeriphId = pChanRecord->SrcPeripheralId;
            DstPeriphId = pChanRecord->DstPeripheralId;

            /* Get the call back fn address and User Id for src & dst
             * periphs and mark their records as idle if terminated.
             */
            DMA_PeripheralTransferTerminating(pCtrlrRecord,
                                              SrcPeriphId,
                                              ChanId,
                                              Terminated,
                                              &SrcUsersId,
                                              &rSrcTerminatedCb);

            DMA_PeripheralTransferTerminating(pCtrlrRecord,
                                              DstPeriphId,
                                              ChanId,
                                              Terminated,
                                              &DstUsersId,
                                              &rDstTerminatedCb);

            /* When the user is the application or low level API, we
             * need to get the address of the call back fn and the
             * users Id from the channel record. However we only do
             * this when whoever issued this request is not the user.
             */
            eUser = pChanRecord->eUser;

            if( eUser != eCaller &&
                (eUser == DMA_USER_APPLICATION || eUser == DMA_USER_LOW_LEVEL_API) )
            {
                rChanTerminatedCb   = pChanRecord->rTerminatedCb;
                ChanUsersId         = pChanRecord->UsersId;
            }

            if( Terminated )
            {
                /* Clean up the channel record. Start by setting the
                 * Src & Dst peripheral Ids to idle state
                 */
                pChanRecord->SrcPeripheralId = apDMA_NO_PERIPHERAL_ID;
                pChanRecord->DstPeripheralId = apDMA_NO_PERIPHERAL_ID;

                /* Remove the Terminated Call back fn ptr */
                pChanRecord->rTerminatedCb = (apDMA_rTransferTerminated)NULL;

                /* Mark the channel as idle */
                pChanRecord->eState = DMA_STATE_IDLE;
                pChanRecord->eUser  = DMA_USER_NONE;
            }

            /* Clear any interrupt that might be pending on the channel */
            pRegs->IntErrorClear    = ChanBit;
            pRegs->IntTCClear       = ChanBit;

            /* Call the call back fns (if any) */
            if( rSrcTerminatedCb != (apDMA_rTransferTerminated)NULL )
            {
                rSrcTerminatedCb(SrcUsersId, RetCode, Count);
            }

            if( rDstTerminatedCb != (apDMA_rTransferTerminated)NULL )
            {
                rDstTerminatedCb(DstUsersId, RetCode, Count);
            }

            if( rChanTerminatedCb != (apDMA_rTransferTerminated)NULL )
            {
                rChanTerminatedCb(ChanUsersId, RetCode, Count);
            }
        }
        else
        {
            /* Channel is marked as idle. Best disable it and clear any
             * interrupt that might be pending on the channel.
             */
            pChanRegs->Configuration = 0;

            pRegs->IntErrorClear    = ChanBit;
            pRegs->IntTCClear       = ChanBit;
        }
    }

    return Count;
}




/* ----- See dma.h for documentation--------------*/

static void DMA_PeripheralTransferTerminating(
    DMA_sStateStruct          *pCtrlrRecord,
    uint32_t                   PeriphId,
    uint32_t                   ChanId,
    BOOL                      Terminated,
    uint32_t                   *pUsersId,
    apDMA_rTransferTerminated *rTerminatedCb)

{
    DMA_sPeriphInfo     *pPeriphRecord;

    *rTerminatedCb = NULL; /* default return value */
    *pUsersId      = 0;

    if( PeriphId < pCtrlrRecord->sFixedConfig.NumPeripherals )
    {
        pPeriphRecord = &pCtrlrRecord->sPeriphs[PeriphId];

        /* Check this peripheral is currently assigned to use the specified
         * channel
         */

        if( pPeriphRecord->CurrentDmaChannel == ChanId )
        {
            /* and are we supposed to use the call back function */

            if( pPeriphRecord->UseCallBack )
            {
                *rTerminatedCb  = pPeriphRecord->rTerminatedCb;
                *pUsersId       = pPeriphRecord->UsersId;
            }

            if( Terminated )
            {
                /* Mark peripheral record as idle */
                pPeriphRecord->UseCallBack          = FALSE;
                pPeriphRecord->CurrentDmaChannel    = apDMA_NO_DMA_CHANNEL;
            }
        }
    }

}

/* ----- See dma.h for documentation--------------*/

static void DMA_PeriphToPeriphTransfer(DMA_sStateStruct *pCtrlrRecord,
                                       uint32_t          ChanId)
{
    uint32_t             SrcPeriphId;
    uint32_t             DstPeriphId;

    DMA_sXferInfo       sSrcXferInfo;
    DMA_sXferInfo       sDstXferInfo;

    DMA_sPeriphInfo     *pSrcPeriphRecord;
    DMA_sPeriphInfo     *pDstPeriphRecord;

    DMA_sChannelStatus  *pChanRecord;

    apDMA_sRawLLI       sRawLLI;

    pChanRecord      = &pCtrlrRecord->sChans[ChanId];

    SrcPeriphId      = pChanRecord->SrcPeripheralId;
    DstPeriphId      = pChanRecord->DstPeripheralId;

    pSrcPeriphRecord = &pCtrlrRecord->sPeriphs[SrcPeriphId];
    pDstPeriphRecord = &pCtrlrRecord->sPeriphs[DstPeriphId];

    /* Update the peripheral record */
    pSrcPeriphRecord->CurrentDmaChannel = ChanId;
    pSrcPeriphRecord->UsersId           = pChanRecord->UsersId;
    pSrcPeriphRecord->UseCallBack       = TRUE;

    pDstPeriphRecord->CurrentDmaChannel = ChanId;
    pDstPeriphRecord->UsersId           = pChanRecord->UsersId;
    pDstPeriphRecord->UseCallBack       = TRUE;

    /* Update the channel record */
    pChanRecord->eUser  = DMA_USER_PERIPHERAL;
    pChanRecord->eState = DMA_STATE_ACTIVE_P_TO_P_TRANSFER;

    /* Get peripheral address details */

    sSrcXferInfo.pXferAccess = &pSrcPeriphRecord->sXferAccess;
    sSrcXferInfo.Increment   = FALSE;
    sSrcXferInfo.PhysAddr    = (uint32_t)pSrcPeriphRecord->pRegisterAddress;

    sDstXferInfo.pXferAccess = &pDstPeriphRecord->sXferAccess;
    sDstXferInfo.Increment   = FALSE;
    sDstXferInfo.PhysAddr    = (uint32_t)pDstPeriphRecord->pRegisterAddress;

    /* Create a single local dummy LLI */

    sRawLLI.SrcAddr = sSrcXferInfo.PhysAddr;
    sRawLLI.DstAddr = sDstXferInfo.PhysAddr;
    sRawLLI.NextLLI = apDMA_NO_LLI;

    /* Create a suitable word for writing to the channel control reg */
    sRawLLI.TransferCtrl = DMA_CreateChanControlReg(pChanRecord->NumTransfers,
                                                    &sSrcXferInfo,
                                                    &sDstXferInfo,
                                                    pChanRecord->Protection,
                                                    DMA_TC_INT_ENABLE);

    /* Copy the data from the channel record to the DMA controller &
     * Start the DMA transfer.
     */

    DMA_StartDMATransfer(pCtrlrRecord, ChanId, &sRawLLI);

}

/* ----- See dma.h for documentation--------------*/

static apError DMA_LinkedMemPeripheralTransfer(DMA_sStateStruct *pCtrlrRecord,
                                               uint32_t          PeriphId,
                                               uint32_t          ChanId,
                                               uint32_t          NumLLItems,
                                               apDMA_sPerMemLLI *pFirstLLI)
{
    apError RetCode = apERR_NONE;   /* Default return code */

    DMA_sPeriphInfo         *pPeriphRecord;
    DMA_sChannelStatus      *pChanRecord;
    apDMA_eFlowControl      eFlowCtrl;

    pPeriphRecord = &pCtrlrRecord->sPeriphs[PeriphId];
    pChanRecord = &pCtrlrRecord->sChans[ChanId];
    eFlowCtrl = pChanRecord->eFlowCtrl;

    /* Validate params, must be at least 1 LLI, and the flow ctrl must be
     * mem to/from periph.
     */
    if( NumLLItems > 0 && (eFlowCtrl == apDMA_MEM_TO_PERIPHERAL_DMA_CTRL ||
                           eFlowCtrl == apDMA_PERIPHERAL_TO_MEM_DMA_CTRL ||
                           eFlowCtrl == apDMA_MEM_TO_PERIPHERAL_PERIPHERAL_CTRL ||
                           eFlowCtrl == apDMA_PERIPHERAL_TO_MEM_PERIPHERAL_CTRL) )
    {

        /* Update a few of the channel record fields */

        pChanRecord->Protection     =   pPeriphRecord->Protection;
        pChanRecord->LockEnable     =   FALSE;

        /* Traverse the LLI list and fill in the raw fields */
        RetCode = DMA_FillInMemPeriphLLIFields(pCtrlrRecord,
                                               PeriphId,
                                               ChanId,
                                               NumLLItems,
                                               pFirstLLI);
        if( RetCode == apERR_NONE )
        {
            /* Update the peripheral and channel records */

            pPeriphRecord->CurrentDmaChannel = ChanId;
            pPeriphRecord->UsersId          = pChanRecord->UsersId;
            pPeriphRecord->UseCallBack      = TRUE;

            if( NumLLItems == 1 &&  pFirstLLI->pNextLLI == (apDMA_sPerMemLLI *)NULL )
            {
                /* Only single LLI which isn't looped - so treat this
                 * as a non LLI transfer
                 */
                pChanRecord->NumTransfers = pFirstLLI->NumTransfers;
                pChanRecord->eState = DMA_STATE_ACTIVE_TRANSFER;
            }
            else
            {
                /* Multiple or looped LLIs */
                pChanRecord->NumTransfers = 0;
                pChanRecord->eState = DMA_STATE_ACTIVE_LLI_TRANSFER;
            }

            pChanRecord->eUser  =   DMA_USER_PERIPHERAL;

            /* Copy the data from the channel & LLI records to the
             * DMA controller & start the DMA transfer.
             */

            DMA_StartDMATransfer(pCtrlrRecord, ChanId, &pFirstLLI->sRaw);
        }
    }
    else
    {
        RetCode = apERR_BAD_PARAMETER;
    }

    return RetCode;
}

/* ----- See dma.h for documentation--------------*/

static apError DMA_LinkedMemToMemTransfer(DMA_sStateStruct *pCtrlrRecord,
                                          uint32_t          ChanId,
                                          uint32_t          NumLLItems,
                                          apDMA_sMemMemLLI *pFirstLLI)
{
    apError   RetCode = apERR_NONE;

    DMA_sChannelStatus      *pChanRecord;

    if( NumLLItems > 0 )
    {
        pChanRecord     = &pCtrlrRecord->sChans[ChanId];

        /* Update a few of the channel record fields */

        pChanRecord->LockEnable     =   FALSE;
        pChanRecord->eFlowCtrl      =   apDMA_MEM_TO_MEM_DMA_CTRL;

        /* Traverse the LLI list and fill in the raw fields */

        RetCode = DMA_FillInMemMemLLIFields(pCtrlrRecord,
                                            ChanId,
                                            NumLLItems,
                                            pFirstLLI);

        if( RetCode == apERR_NONE )
        {
            /* Update the channel records */

            pChanRecord->SrcPeripheralId = apDMA_NO_PERIPHERAL_ID;
            pChanRecord->DstPeripheralId = apDMA_NO_PERIPHERAL_ID;

            if( NumLLItems == 1 && pFirstLLI->pNextLLI == (apDMA_sMemMemLLI *)NULL )
            {
                /* Only single LLI which isn't looped - so treat this
                 * as a non LLI transfer
                 */

                pChanRecord->NumTransfers = pFirstLLI->NumTransfers;
                pChanRecord->eState = DMA_STATE_ACTIVE_TRANSFER;
            }
            else
            {
                /* Multiple or looped LLIs */

                pChanRecord->NumTransfers = 0;
                pChanRecord->eState = DMA_STATE_ACTIVE_LLI_TRANSFER;
            }

            /* Copy the data from the channel & LLI records to the
             * DMA controller & start the DMA transfer.
             */

            DMA_StartDMATransfer(pCtrlrRecord, ChanId, &pFirstLLI->sRaw);
        }
    }
    else
    {
        /* Invalid number of LLIs */
        RetCode = apERR_BAD_PARAMETER;
    }

    return RetCode;
}

/* ----- See dma.h for documentation--------------*/

static apError DMA_LowLevelTransfer(DMA_sStateStruct *pCtrlrRecord,
                                    uint32_t          DmaChannel,
                                    apDMA_sTransfer  *pParam)
{
    apError             RetCode = apERR_NONE;
    uint32_t             SrcPeriphId;
    uint32_t             DstPeriphId;

    DMA_sChannelStatus  *pChanRecord;
    DMA_sPeriphInfo     *pSrcPeriphRecord;
    DMA_sPeriphInfo     *pDstPeriphRecord;

    pChanRecord = &pCtrlrRecord->sChans[DmaChannel];

    /* Update a few of the channel record fields */
    pChanRecord->eFlowCtrl      =   pParam->eFlowCtrl;
    pChanRecord->LockEnable     =   pParam->LockEnable;

    /* Traverse the LLI list and fill in the raw fields */

    if( pParam->NumLLItems > 0 )
    {
        RetCode = DMA_FillInLowLevelLLIFields(pCtrlrRecord, pParam);

        if( RetCode == apERR_NONE )
        {
            /* Update the associated peripheral records if specified*/

            SrcPeriphId = pParam->SrcPeripheralId;
            DstPeriphId = pParam->DstPeripheralId;

            if( SrcPeriphId < pCtrlrRecord->sFixedConfig.NumPeripherals )
            {
                pSrcPeriphRecord = &pCtrlrRecord->sPeriphs[SrcPeriphId];

                pSrcPeriphRecord->UseCallBack          = FALSE;
                pSrcPeriphRecord->CurrentDmaChannel    = DmaChannel;
            }

            if( DstPeriphId < pCtrlrRecord->sFixedConfig.NumPeripherals )
            {
                pDstPeriphRecord = &pCtrlrRecord->sPeriphs[DstPeriphId];

                pDstPeriphRecord->UseCallBack          = FALSE;
                pDstPeriphRecord->CurrentDmaChannel    = DmaChannel;
            }

            /* Update the channel record */

            pChanRecord->UsersId            = pParam->UserId;
            pChanRecord->rTerminatedCb      = pParam->rTTCallBack;
            pChanRecord->SrcPeripheralId    = SrcPeriphId;
            pChanRecord->DstPeripheralId    = DstPeriphId;

            if( pParam->NumLLItems == 1 && pParam->pLLI->pNextLLI == (apDMA_sLLI *)NULL )
            {
                /* Only single LLI which isn't looped - so treat this as a non LLI transfer
                 */
                pChanRecord->NumTransfers = pParam->pLLI->NumTransfers;
                pChanRecord->eState = DMA_STATE_ACTIVE_LOW_TRANSFER;
            }
            else
            {
                /* Multiple or looped LLIs */

                pChanRecord->NumTransfers = 0;
                pChanRecord->eState = DMA_STATE_ACTIVE_LOW_MULTI_LLI_TRANSFER;
            }

            pChanRecord->eUser = DMA_USER_LOW_LEVEL_API;

            /* Copy the data from the channel & LLI records to the
             * DMA controller & start the DMA transfer.
             */

            DMA_StartDMATransfer(pCtrlrRecord, DmaChannel, &pParam->pLLI->sRaw);
        }
    }
    else
    {
        RetCode = apERR_BAD_PARAMETER;
    }

    return RetCode;
}

/* ----- See dma.h for documentation--------------*/

static apError DMA_FillInMemPeriphLLIFields(DMA_sStateStruct *pCtrlrRecord,
                                            uint32_t PeriphId,
                                            uint32_t ChanId,
                                            uint32_t NumLLItems,
                                            apDMA_sPerMemLLI *pFirstLLI)
{
    apError              RetCode = apERR_NONE;   /* Default return code */
    uint32_t              I;
    DMA_sChannelStatus   *pChanRecord;
    DMA_sPeriphInfo      *pPeriphRecord;

    DMA_sXferInfo        sSrcXferInfo;
    DMA_sXferInfo        sDstXferInfo;
    DMA_sXferInfo        *pMemXferInfo;
    apDMA_eFlowControl   eFlowCtrl;

    void                 *pNextRawLLI;
    apDMA_sPerMemLLI     *pNextLLI;
    apDMA_sPerMemLLI     *pLLI = pFirstLLI;

    pPeriphRecord   = &pCtrlrRecord->sPeriphs[PeriphId];
    pChanRecord     = &pCtrlrRecord->sChans[ChanId];
    eFlowCtrl       = pChanRecord->eFlowCtrl;

    if( eFlowCtrl == apDMA_MEM_TO_PERIPHERAL_DMA_CTRL ||
        eFlowCtrl == apDMA_MEM_TO_PERIPHERAL_PERIPHERAL_CTRL )
    {
        /* Memory is the source */
        pChanRecord->SrcPeripheralId = apDMA_NO_PERIPHERAL_ID;
        pChanRecord->DstPeripheralId = PeriphId;

        sDstXferInfo.pXferAccess = &pPeriphRecord->sXferAccess;
        sDstXferInfo.Increment   = FALSE;
        sDstXferInfo.PhysAddr    = (uint32_t)pPeriphRecord->pRegisterAddress;

        pMemXferInfo = &sSrcXferInfo;
    }
    else
    {
        /* The peripheral is the source */
        pChanRecord->SrcPeripheralId = PeriphId;
        pChanRecord->DstPeripheralId = apDMA_NO_PERIPHERAL_ID;

        sSrcXferInfo.pXferAccess = &pPeriphRecord->sXferAccess;
        sSrcXferInfo.Increment   = FALSE;
        sSrcXferInfo.PhysAddr    = (uint32_t)pPeriphRecord->pRegisterAddress;

        pMemXferInfo = &sDstXferInfo;
    }

    for( I = 0; I < NumLLItems; I++ )
    {
        if( pLLI != (apDMA_sPerMemLLI *)NULL )
        {
            /* Get the access details for memory */
            DMA_GetAddressAccessDetails(pCtrlrRecord, pLLI->pMemAddress, pMemXferInfo);

            pNextLLI = pLLI->pNextLLI;

            if( pNextLLI != (apDMA_sPerMemLLI *)NULL )
            {
                pNextRawLLI = (void *)&pNextLLI->sRaw.SrcAddr;
            }
            else
            {
                pNextRawLLI = (void *)NULL;
            }

            /* Fill in the Raw fields of this LLI */
            DMA_FillInLLI(pCtrlrRecord,
                          &pLLI->sRaw,
                          &sSrcXferInfo,
                          &sDstXferInfo,
                          pNextRawLLI,
                          pLLI->NumTransfers,
                          pChanRecord->Protection,
                          pChanRecord->CbPerItem);

            /* and then move onto the next LLI */

            pLLI = pNextLLI;
        }
        else
        {
            /* List ptr is NULL but our count hasn't reached NumLLItems yet */
            RetCode = apERR_BAD_PARAMETER;
            break;
        }
    }

    return RetCode;
}

/* ----- See dma.h for documentation--------------*/

static apError DMA_FillInMemMemLLIFields(DMA_sStateStruct  *pCtrlrRecord,
                                         uint32_t           ChanId,
                                         uint32_t           NumLLItems,
                                         apDMA_sMemMemLLI  *pFirstLLI)
{
    apError RetCode = apERR_NONE;   /* Default return code */
    uint32_t                 I;
    DMA_sChannelStatus      *pChanRecord;
    DMA_sXferInfo           sSrcXferInfo;
    DMA_sXferInfo           sDstXferInfo;

    void                    *pNextRawLLI;
    apDMA_sMemMemLLI        *pNextLLI;
    apDMA_sMemMemLLI        *pLLI = pFirstLLI;

    pChanRecord     = &pCtrlrRecord->sChans[ChanId];

    for( I = 0; I < NumLLItems; I++ )
    {
        if( pLLI != (apDMA_sMemMemLLI *)NULL )
        {
            DMA_GetAddressAccessDetails(pCtrlrRecord, pLLI->pSrcMemAddr, &sSrcXferInfo);

            DMA_GetAddressAccessDetails(pCtrlrRecord, pLLI->pDstMemAddr, &sDstXferInfo);

            pNextLLI = pLLI->pNextLLI;

            if( pNextLLI != (apDMA_sMemMemLLI *)NULL )
            {
                pNextRawLLI = (void *)&pNextLLI->sRaw.SrcAddr;
            }
            else
            {
                pNextRawLLI = (void *)NULL;
            }

            /* Fill in the Raw fields of this LLI */

            DMA_FillInLLI(pCtrlrRecord,
                          &pLLI->sRaw,
                          &sSrcXferInfo,
                          &sDstXferInfo,
                          pNextRawLLI,
                          pLLI->NumTransfers,
                          pChanRecord->Protection,
                          pChanRecord->CbPerItem);

            /* and then move onto the next LLI */
            pLLI = pNextLLI;
        }
        else
        {
            /* List ptr is NULL but our count hasn't reached NumLLItems yet */
            RetCode = apERR_BAD_PARAMETER;
            break;
        }
    }

    return RetCode;
}

/* ----- See dma.h for documentation--------------*/

static apError DMA_FillInLowLevelLLIFields(DMA_sStateStruct *pCtrlrRecord,
                                           apDMA_sTransfer  *pParam)
{
    apError        RetCode = apERR_NONE;   /* Default return code */
    uint32_t        I;
    uint32_t        NumLLItems;

    apDMA_sLLI     *pLLI;        /* Ptr to LLI */
    apDMA_sLLI     *pNextLLI;    /* Ptr to Next LLI */
    DMA_sXferInfo  sSrcXferInfo;
    DMA_sXferInfo  sDstXferInfo;
    void           *pNextRawLLI;
    BOOL           PhysicalAddressing = pParam->PhysicalAddressing;

    pLLI        = pParam->pLLI;
    NumLLItems  = pParam->NumLLItems;

    for( I = 0; I < NumLLItems; I++ )
    {
        if( pLLI != (apDMA_sLLI *)NULL )
        {
            /* Convert src to DMA_sXferInfo format */

            sSrcXferInfo.Increment = pLLI->sSrc.AutoInc;

            if( PhysicalAddressing )
            {
                sSrcXferInfo.PhysAddr      = (uint32_t)pLLI->sSrc.pAddr;
                sSrcXferInfo.pXferAccess   = &pLLI->sSrc.sAccessInfo;
            }
            else
            {
                /* Virtual address supllied, need to get the access details for this address */

                DMA_GetAddressAccessDetails(pCtrlrRecord, pLLI->sSrc.pAddr, &sSrcXferInfo);
            }

            /* Convert dst to DMA_sXferInfo format */

            sDstXferInfo.Increment = pLLI->sDst.AutoInc;

            if( PhysicalAddressing )
            {
                sDstXferInfo.PhysAddr      = (uint32_t)pLLI->sDst.pAddr;
                sDstXferInfo.pXferAccess   = &pLLI->sDst.sAccessInfo;
            }
            else
            {
                /* Virtual address supllied, need to get the access details for this address */

                DMA_GetAddressAccessDetails(pCtrlrRecord, pLLI->sDst.pAddr, &sDstXferInfo);
            }

            /* Work on the ptr to the next LLI */
            pNextLLI = pLLI->pNextLLI;

            if( pNextLLI != (apDMA_sLLI *)NULL )
            {
                pNextRawLLI = (void *)&pNextLLI->sRaw.SrcAddr;
            }
            else
            {
                pNextRawLLI = (void *)NULL;
            }

            /* Fill in the LLI fields */
            DMA_FillInLLI(pCtrlrRecord,
                          &pLLI->sRaw,
                          &sSrcXferInfo,
                          &sDstXferInfo,
                          pNextRawLLI,
                          pLLI->NumTransfers,
                          pLLI->Protection,
                          pLLI->TCInterruptEnable);

            /* and then move onto the next LLI */
            pLLI = pNextLLI;
        }
        else
        {
            /* List stopped too early */
            RetCode = apERR_BAD_PARAMETER;
            break;
        }
    }

    return RetCode;
}

/* ----- See dma.h for documentation--------------*/

static void DMA_FillInLLI(DMA_sStateStruct    *pCtrlrRecord,
                          apDMA_sRawLLI       *pRawLLI,
                          DMA_sXferInfo       *pSrcXferInfo,
                          DMA_sXferInfo       *pDstXferInfo,
                          void                *pRawNext,
                          uint32_t             NumTransfers,
                          uint32_t              Protection,
                          BOOL                CbPerItem)
{
    DMA_eTCInterruptFlag  eTCIntFlag = DMA_TC_INT_DISABLE;
    DMA_sXferInfo         sLLIInfo;

    pRawLLI->SrcAddr = pSrcXferInfo->PhysAddr;
    pRawLLI->DstAddr = pDstXferInfo->PhysAddr;

    if( pRawNext != (void *)NULL )
    {
        /* Get the access details for the raw next LLI ptr */
        DMA_GetAddressAccessDetails(pCtrlrRecord, pRawNext, &sLLIInfo);

        /* Copy the access details to the next LLI field */
        pRawLLI->NextLLI = apBIT_BUILD(DMA_NEXT_LLI_BUS,
                                       sLLIInfo.pXferAccess->eAhbBus) |
            apBIT_BUILD(DMA_LLI_RESERVED, 0) |
            (sLLIInfo.PhysAddr &  DMA_NEXT_LLI);

        /* NOTE :   The bottom bits of the the LLI ptr are masked off but
         *          must already have been 0. The DMA Ctrlr requires that
         *          each LL Item starts on a word boundary, hence the 
         *          bottom two bits of a ptr to an LLI will be clear.
         */

        if( CbPerItem )
        {
            /* Want to interrupt when every LLI completes */
            eTCIntFlag = DMA_TC_INT_ENABLE;
        }
    }
    else
    {
        /* This must be the last item */
        pRawLLI->NextLLI = apDMA_NO_LLI;

        /* Always interrupt when the final item has completed */
        eTCIntFlag = DMA_TC_INT_ENABLE;
    }


    /* Create a suitable word for writing to the channel control reg */
    pRawLLI->TransferCtrl = DMA_CreateChanControlReg(NumTransfers,
                                                     pSrcXferInfo,
                                                     pDstXferInfo,
                                                     Protection,
                                                     eTCIntFlag);

}

/* ----- See dma.h for documentation--------------*/

static void DMA_StartDMATransfer(DMA_sStateStruct *pCtrlrRecord,
                                 uint32_t          ChanId,
                                 apDMA_sRawLLI    *pFirstLLI)
{
    DMA_sPort               *pRegs;
    DMA_sChannel            *pChanRegs;
    DMA_sChannelStatus      *pChanRecord;

    pRegs       = (DMA_sPort *)pCtrlrRecord->Base;
    pChanRegs   = &pRegs->sDmaChannels[ChanId];
    pChanRecord = &pCtrlrRecord->sChans[ChanId];

    /* disable the channel */
    pChanRegs->Configuration = 0;

    /* Write to the channel Src, Dst & LLI  Regs */
    pChanRegs->SrcAddr  = pFirstLLI->SrcAddr;
    pChanRegs->DestAddr = pFirstLLI->DstAddr;
    pChanRegs->LLI      = pFirstLLI->NextLLI;

    /* Write to the channel control reg */
    pChanRegs->Control  = pFirstLLI->TransferCtrl;

    /* Clear any interrupt that might be pending on the channel */
    pRegs->IntErrorClear    = DMA_CHAN_BIT(ChanId);
    pRegs->IntTCClear       = DMA_CHAN_BIT(ChanId);

    /* Create a word suitable for writing to the channel config register,
     * and write it to the register - enabling the channel & hence
     * starting the transfer.
     */

    pChanRegs->Configuration =
        apBIT_BUILD(bsDMA_CHANNEL_ENABLED,      1) |
        apBIT_BUILD(bsDMA_SRC_PERIPHERAL,       pChanRecord->SrcPeripheralId) |
        apBIT_BUILD(bsDMA_DEST_PERIPHERAL,      pChanRecord->DstPeripheralId) |
        apBIT_BUILD(bsDMA_FLOW_CONTROL,         pChanRecord->eFlowCtrl) |
        apBIT_BUILD(bsDMA_BUS_LOCK,             DMA_BOOL_TO_BINARY(pChanRecord->LockEnable)) |
        apBIT_BUILD(bsDMA_ERROR_INTERRUPT_MASK, 1) |
        apBIT_BUILD(bsDMA_TC_INTERRUPT_MASK,    1) |
        apBIT_BUILD(bsDMA_ACTIVE,               0) |
        apBIT_BUILD(bsDMA_HALT,                 0) |
        apBIT_BUILD(bsDMA_CONFIG_RESERVED,      0);

}

/* ----- See dma.h for documentation--------------*/

static void DMA_GetAddressAccessDetails(DMA_sStateStruct *pCtrlrRecord,
                                        void             *pVirtualAddress,
                                        DMA_sXferInfo    *pXferInfo)
{
    uint32_t             I;
    uint32_t             NumBlocks;
    uint32_t             StartAddr;
    int32_t              Offset;
    apDMA_sAddressBlockCharacteristics  *pAddrBlock;

    /* assume its not in the virtual address block table */

    pXferInfo->pXferAccess  = &pCtrlrRecord->sDefaultMemAccess;
    pXferInfo->PhysAddr     = (uint32_t)pVirtualAddress;
    pXferInfo->Increment    = TRUE;

    /* Now check if it is specified in the virtual address block table */

    NumBlocks   = pCtrlrRecord->NumAddressBlocks;
    pAddrBlock  = pCtrlrRecord->pAddressBlocks;

    for( I = 0; I < NumBlocks; I++ )
    {
        StartAddr = (uint32_t)pAddrBlock->pVirtualAddress;

        if( (uint32_t)pVirtualAddress >= StartAddr &&
            (uint32_t)pVirtualAddress <= (StartAddr +
                                          (pAddrBlock->BlockSize) - 1) )
        {
            /* Found a match - note the address of the access details */

            pXferInfo->pXferAccess = &pAddrBlock->sXferAccess;

            /* Calculate the physical address */

            Offset = pAddrBlock->PhysicalOffset;

            if( Offset < 0 )
            {
                /* The offset is negative so convert it to a positive offset
                 * and subtract it from the virtual address.  
                 * Note, the virtual address is unsigned  and could have the
                 * top most bit set - hence we couldn't just add the offset.
                 */

                Offset = -Offset;

                pXferInfo->PhysAddr = ((uint32_t)pVirtualAddress) - ((uint32_t)Offset);

            }
            else
            {
                /* The offset is positive, so simply add it to the virtual
                 * address
                 */

                pXferInfo->PhysAddr = ((uint32_t)pVirtualAddress) + ((uint32_t)Offset);

            }

            break;

        }
        else
        {
            pAddrBlock++;   /* Next one */
        }
    }

}

/* ----- See dma.h for documentation--------------*/

static uint32_t DMA_CreateChanControlReg(uint32_t              NumTransfers,
                                         DMA_sXferInfo        *pSrcXferInfo,
                                         DMA_sXferInfo        *pDstXferInfo,
                                         uint32_t               Protection,
                                         DMA_eTCInterruptFlag eTCIntFlag)
{
    apDMA_sXferCharacteristics  *pSrcInfo = pSrcXferInfo->pXferAccess;
    apDMA_sXferCharacteristics  *pDstInfo = pDstXferInfo->pXferAccess;

    /* Create a word suitable for writing to a channel control reg */

    return
           apBIT_BUILD(bsDMA_TRANSFER_SIZE, NumTransfers) |

           apBIT_BUILD(bsDMA_SRC_BURST_SIZE, pSrcInfo->eBurstSize) |
           apBIT_BUILD(bsDMA_SRC_WIDTH, pSrcInfo->eWidth) |
           apBIT_BUILD(bsDMA_SRC_BUS, pSrcInfo->eAhbBus) |
           apBIT_BUILD(bsDMA_SRC_INCREMENT,
                       DMA_BOOL_TO_BINARY(pSrcXferInfo->Increment)) |

           apBIT_BUILD(bsDMA_DEST_BURST_SIZE, pDstInfo->eBurstSize) |
           apBIT_BUILD(bsDMA_DEST_WIDTH, pDstInfo->eWidth) |
           apBIT_BUILD(bsDMA_DEST_BUS, pDstInfo->eAhbBus) |
           apBIT_BUILD(bsDMA_DEST_INCREMENT,
                       DMA_BOOL_TO_BINARY(pDstXferInfo->Increment)) |

           apBIT_BUILD(bsDMA_PROTECTION, Protection) |
           apBIT_BUILD(bsDMA_TC_INTERRUPT_ENABLE, eTCIntFlag);

}

/* ----- See dma.h for documentation--------------*/

static apError DMA_ValidateChannelEntry(
    uint32_t          DmaChannel,
    DMA_sStateStruct **pCtrlr)
{
    apError          RetCode;
    DMA_sStateStruct *pCtrlrRecord;

    /* Check the ctrlr Id is in range and the ctrlr has been initialised,
     * and also get a pointer to the appropriate ctrlr record.
     */

    RetCode = DMA_GeneralEntry(&pCtrlrRecord);

    if( RetCode == apERR_NONE )
    {
        if( pCtrlrRecord->Enabled )
        {

            /* Check the specified channel is in range */
            if( DmaChannel < pCtrlrRecord->sFixedConfig.NumDmaChannels )
            {
                *pCtrlr = pCtrlrRecord;
            }
            else
            {

                RetCode = apERR_BAD_PARAMETER;
            }
        }
        else
        {

            RetCode = (apError)apERR_DMA_CTRLR_DISABLED;
        }
    }

    return RetCode;
}

/* ----- See dma.h for documentation--------------*/

static apError DMA_GeneralEntry(DMA_sStateStruct **pCtrlr)
{
    apError           RetCode = apERR_NONE;   /* Default return code */
    DMA_sStateStruct  *pCtrlrRecord;

    /* Check the validty of the supplied Ctrlr oId */



    pCtrlrRecord = DMA_sState;

    /* Check the ctrlr has been initialised */

    if( pCtrlrRecord->Initialised )
    {
        *pCtrlr = pCtrlrRecord;
    }
    else
    {
        /* Has not been initialised ! */
        RetCode = (apError)apERR_DMA_CTRLR_NOT_INITIALISED;
    }

    return RetCode;
}

/* ----- See dma.h for documentation--------------*/

static BOOL DMA_ValidMemAccessDetails(DMA_sStateStruct *pCtrlrRecord,
                                      apDMA_sXferCharacteristics *pXferInfo)
{
    BOOL  RetCode = FALSE;

    if( //pXferInfo->eBurstSize >= apDMA_BURST_1 &&
        pXferInfo->eBurstSize <= apDMA_BURST_128 )
    {
        if( //pXferInfo->eWidth >= apDMA_WIDTH_8_BIT &&
            pXferInfo->eWidth <= apDMA_WIDTH_1024_BIT )
        {
            /* now need to check that its <= the AHB bus width of this ctlr */
            if( pXferInfo->eWidth <= apDMA_WIDTH_32_BIT ||
                ((pXferInfo->eWidth - apDMA_WIDTH_32_BIT) <=
                     pCtrlrRecord->sFixedConfig.eAhbWidth) )
            {
                /*  Need to check how many AHB busses the Ctrlr has */
                if( pXferInfo->eAhbBus == apDMA_AHB_BUS_1 ||
                    (pXferInfo->eAhbBus == apDMA_AHB_BUS_2 &&
                     pCtrlrRecord->sFixedConfig.NumAhbIf > 1) )
                {
                    RetCode = TRUE;
                }
            }
        }
    }

    return RetCode;
}

/* ----- See dma.h for documentation--------------*/

static void DMA_ReadControllerFixedData(DMA_sStateStruct *pCtrlrRecord)
{
    uint32_t                 PeriphId3;
    DMA_sCtrlrFixedConfig   *pFixedConfig = &pCtrlrRecord->sFixedConfig;
    DMA_sPort               *pCtrlr = (DMA_sPort *)pCtrlrRecord->Base;
    uint32_t                 NumChans;
    uint32_t                 NumAHB;
    uint32_t                 NumPeriphs;
    apDMA_eAhbWidth         eAhbWidth;

    /* Read the contents of Peripheral ID register 3 */

    PeriphId3 = pCtrlr->PeripheralId3;

    /* Now read the fixed config info from it */

    NumChans    = 2UL << (apBIT_GET(PeriphId3, DMA_NUM_CHANNELS));
    NumAHB      = 1UL << (apBIT_GET(PeriphId3, DMA_NUM_AHB_BUS));
    eAhbWidth   = (apDMA_eAhbWidth)apBIT_GET(PeriphId3, DMA_AHB_WIDTH);

    NumPeriphs  = 16UL << (apBIT_GET(PeriphId3, DMA_SOURCE_REQUESTORS));



    /* Copy the params to our controller record */

    pFixedConfig->NumDmaChannels    = NumChans;
    pFixedConfig->NumAhbIf          = NumAHB;
    pFixedConfig->NumPeripherals    = NumPeriphs;
    pFixedConfig->eAhbWidth         = eAhbWidth;

}

/* ----- See dma.h for documentation--------------*/

static void DMA_ConfigureController(DMA_sStateStruct *pCtrlrRecord)
{
    DMA_sPort   *pCtrlrReg = (DMA_sPort *)pCtrlrRecord->Base;

    /* Write appropriate data to Ctrlr's config registers. */

    pCtrlrReg->Configuration =
        apBIT_BUILD(bsDMA_CTRLR_ENABLED,
                    DMA_BOOL_TO_BINARY(pCtrlrRecord->Enabled)) |
        apBIT_BUILD(bsDMA_AHB1_ENDIAN,
                    DMA_BOOL_TO_BINARY(pCtrlrRecord->AHB1BigEndian)) |
        apBIT_BUILD(bsDMA_AHB2_ENDIAN,
                    DMA_BOOL_TO_BINARY(pCtrlrRecord->AHB2BigEndian));

}

/* ----- See dma.h for documentation--------------*/

static void DMA_InitializeController(DMA_sStateStruct *pCtrlrRecord)
{
    int32_t      I;
    DMA_sPort   *pCtrlr = (DMA_sPort *)pCtrlrRecord->Base;
    int32_t      NumChans = (int32_t)pCtrlrRecord->sFixedConfig.NumDmaChannels;

    /* Make sure all DMA channels are disabled */

    for( I = 0; I < NumChans; I++ )
    {
        pCtrlr->sDmaChannels[I].Configuration = 0;
    }

    /* Finally clear out any nasty interrupts that might be lurking */

    pCtrlr->IntErrorClear   = apBITS_ALL;
    pCtrlr->IntTCClear      = apBITS_ALL;

}


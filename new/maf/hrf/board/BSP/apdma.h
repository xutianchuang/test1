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
 * File:     apdma.h,v
 * Revision: 1.20
 * ----------------------------------------------------------------
 * 
 *  ----------------------------------------
 *  Version and Release Control Information:
 * 
 *  File Name              : apdma.h.rca
 *  File Revision          : 1.2
 * 
 *  Release Information    : PrimeCell(TM)-PL080-r1p1-00ltd0
 *  ----------------------------------------
 */

/* 
 * Public header file for the DMA Interface Driver code.
 * This file contains the headers for the API implementation of the code
 *
 * NOTE :
 *  The function headers in this file show all possible return values
 *  that can occur when the debug macro apDEBUG_ENABLED is defined. If this
 *  macro is not defined, then most functions will not do validity checking
 *  and therefore only return a subset of the specified return values.
 *  Code using these functions must assume all stated return values can occur.
 */

#ifndef APDMA_H
#define APDMA_H

#ifdef __cplusplus
extern "C"
{ /* allow C++ to use these headers */
#endif /* __cplusplus */
#include <stdint.h>
#include <stdbool.h>
#define BOOL bool
#define TRUE true
#define FALSE false
//typedef char BOOL;
/* The DMA driver is split into two APIs, the standard API and the Low Level API. 
 * Many types and structures are common to both. This header file is split into three
 * sections, showing the common, the standard & the low level types, structures and
 * procedure definitions
 */



/*
 *              ===================== COMMON =========================
 */

/*
 *              ---------------Constants & Macro definitions-----------
 */

/*
 * Description:
 * Some handy constants used to control array sizes. These should be 
 * reduced to the minimum acceptable values to reduce memory wastage.
 * 
 */
#define apOS_DMA_MAXIMUM 1
#if (defined apDMA_VERSION) && (apDMA_VERSION == 81)
#define apDMA_MAX_CHANNELS      2
#define apDMA_MAX_AHB_MASTERS   1
#else
#define apDMA_MAX_CHANNELS      8
#define apDMA_MAX_AHB_MASTERS   2
#endif

#define apDMA_MAX_PERIPHERALS   16

/*
 * Description:
 * More handy constants
 * 
 */

#define apDMA_NO_PERIPHERAL_ID  apDMA_MAX_PERIPHERALS
#define apDMA_NO_DMA_CHANNEL    apDMA_MAX_CHANNELS
#define apDMA_NO_LLI            (uint32_t) 0


#define apBITS_ALL 0xffffffff

typedef enum apxError
{
    apERR_NONE,
    apERR_BAD_PARAMETER,
    apERR_BUSY,
    apERR_DMA_START,
} apError;
/*
 *              ---------------Common Type definitions-----------
 */

/*
 * Description:
 * Call back function type
 *
 */
typedef void (*apDMA_rTransferTerminated)(uint32_t UsersId,
                                          apError ResultCode,
                                          uint32_t TransferCount);
/*
 * Description:
 * Id used to refer to AHB bus
 * 
 */
typedef enum apDMA_xAhbBus
{
    apDMA_AHB_BUS_1    = 0,             /* AHB Bus 1 */
    apDMA_AHB_BUS_2    = 1              /* AHB Bus 2 */

} apDMA_eAhbBus;



/*
 * Description:
 * Width of the AHB bus(es)
 * 
 */
typedef enum apDMA_xAhbWidth
{
    apDMA_AHB_WIDTH_32_BIT      = 0,    /* 32 Bits */
    apDMA_AHB_WIDTH_64_BIT      = 1,    /* 64 Bits */
    apDMA_AHB_WIDTH_128_BIT     = 2,    /* 128 Bits */
    apDMA_AHB_WIDTH_256_BIT     = 3,    /* 256 Bits */
    apDMA_AHB_WIDTH_512_BIT     = 4,    /* 512 Bits */
    apDMA_AHB_WIDTH_1024_BIT    = 5     /* 1024 Bits */

} apDMA_eAhbWidth;



/*
 * Description:
 * Burst Transfer Size
 * 
 */
typedef enum apDMA_xBurstSize
{
    apDMA_BURST_1       = 0,    /* 1 transfer per burst */
    apDMA_BURST_4       = 1,    /* 4 transfers per burst */
    apDMA_BURST_8       = 2,    /* 8 transfers per burst */
    apDMA_BURST_16      = 3,    /* 16 transfers per burst */
    apDMA_BURST_32      = 4,    /* 32 transfers per burst */
    apDMA_BURST_64      = 5,    /* 64 transfers per burst */
    apDMA_BURST_128     = 6     /* 128 transfers per burst */

} apDMA_eBurstSize;



/*
 * Description:
 * Transfer Width
 *
 * Implementation:
 * Do not specify a greater width than actual width of the AHB bus(es)
 * The DMA driver will reject such requests, or attempts to configure
 * an internal peripheral or virtual memory block with an invalid width.
 * 
 */
typedef enum apDMA_xWidth
{
    apDMA_WIDTH_8_BIT       = 0,    /* 8 Bits per transfer */
    apDMA_WIDTH_16_BIT      = 1,    /* 16 Bits per transfer */
    apDMA_WIDTH_32_BIT      = 2,    /* 32 Bits per transfer */
    apDMA_WIDTH_64_BIT      = 3,    /* 64 Bits per transfer */
    apDMA_WIDTH_128_BIT     = 4,    /* 128 Bits per transfer */
    apDMA_WIDTH_256_BIT     = 5,    /* 256 Bits per transfer */
    apDMA_WIDTH_512_BIT     = 6,    /* 512 Bits per transfer */
    apDMA_WIDTH_1024_BIT    = 7     /* 1024 Bits per transfer */

} apDMA_eWidth;


typedef enum apDMA_xSync
{
    apDMA_SYNC_ENABLE   = 0,
    apDMA_SYNC_DISABLE  = 1

} apDMA_eSync;




/*
 * Description:
 * Settings for AHB Bus Protection Lines
 * 
 * Implementation:
 * The desired protection should be created by binary ORing one entry
 * from each pair. eg.
 * apDMA_PROT_SUPER | apDMA_PROT_BUFFERABLE | apDMA_PROT_CACHEABLE
 *
 */
typedef enum apDMA_xProtectionBits
{
    apDMA_PROT_USER             = 0x00,     /* User access */
    apDMA_PROT_SUPER            = 0x01,     /* Supervisor access */

    apDMA_PROT_NON_BUFFERABLE   = 0x00,     /* Non bufferable data */
    apDMA_PROT_BUFFERABLE       = 0x02,     /* Bufferable data */

    apDMA_PROT_NON_CACHEABLE    = 0x00,     /* Non cacheable data */
    apDMA_PROT_CACHEABLE        = 0x04,     /* Cacheable data */

    apDMA_PROTECTION_VALID      =   (apDMA_PROT_CACHEABLE |
                                     apDMA_PROT_BUFFERABLE |
                                     apDMA_PROT_SUPER)
} apDMA_eProtectionBits;

/*
 * Description:
 * DMA Specific error return codes
 * 
 */
typedef enum apDMA_xError
{
    apERR_DMA_CTRLR_NOT_INITIALISED         = apERR_DMA_START,  /* Initialisation not completed */
    apERR_DMA_PERIPHERAL_NOT_INITIALISED,                       /* peripheral record not fully intialised */
    apERR_DMA_TRANSFER_ERROR,                                   /* Transfer terminated due to error */
    apERR_DMA_TRANSFER_ABORTED,                                 /* DMA Transfer aborted */
    apERR_DMA_TRANSFER_LLI_COMPLETE,                            /* DMA LLI Transfer complete */
    apERR_DMA_TRANSFER_COMPLETE,                                /* Entire DMA transfer complete */
    apERR_DMA_CTRLR_ACTIVE,                                     /* Ctrlr has active transfers in progress */
    apERR_DMA_CTRLR_DISABLED                                    /* Ctrlr has been disabled */
} apDMA_eError;


/*
 * Description:
 * Enum of possible DMA flow directions & flow controllers
 *
 * Implementation:
 * Each DMA transfer operation must indicate the types of source and destination
 * (memory or peripheral), and when a peripheral is involved, whether it or the
 * DMA controller is controlling the flow.
 */
typedef enum apDMA_xFlowControl
{
    apDMA_MEM_TO_MEM_DMA_CTRL                            = 0,
    apDMA_MEM_TO_PERIPHERAL_DMA_CTRL                     = 1,
    apDMA_PERIPHERAL_TO_MEM_DMA_CTRL                     = 2,
    apDMA_PERIPHERAL_TO_PERIPHERAL_DMA_CTRL              = 3,
    apDMA_PERIPHERAL_TO_PERIPHERAL_DEST_PERIPHERAL_CTRL  = 4,
    apDMA_MEM_TO_PERIPHERAL_PERIPHERAL_CTRL              = 5,
    apDMA_PERIPHERAL_TO_MEM_PERIPHERAL_CTRL              = 6,
    apDMA_PERIPHERAL_TO_PERIPHERAL_SRC_PERIPHERAL_CTRL   = 7

} apDMA_eFlowControl;

/*
 *              --------Common Structure definitions--------
 */


/* ---------------------------------------------------------------------------
 * Description:
 * Transfer Access characteristics - which specify how the DMA ctlrlr should
 * do a transfer.
 */
typedef struct apDMA_xXferCharacteristics
{
    apDMA_eAhbBus       eAhbBus;        /* Which AHB bus the address is physically on */
    apDMA_eWidth        eWidth;         /* Width of transfer to use when doing transfers */
    apDMA_eBurstSize    eBurstSize;     /* Size of burst transfers to use */

} apDMA_sXferCharacteristics;




/* ---------------------------------------------------------------------------
 * Description:
 * Virtual memory address block characteristics
 *
 * Implementation:
 * Used to define a virtual block of memory, the physical memory it maps to
 * and how it should be accessed by the DMA controller. Can also be used
 * to define access characteristics for physical memory by setting the
 * PhysicalOffset field to zero.
 */
typedef struct apDMA_xAddressBlockCharacteristics
{
    void                        *pVirtualAddress;   /* Virtual Address of this block */
    uint32_t                     BlockSize;          /* Size of this block (in bytes) */
    int32_t                      PhysicalOffset;     /* Offset to use to get to physical memory */
    apDMA_sXferCharacteristics  sXferAccess;        /* Transfer characteristics */

} apDMA_sAddressBlockCharacteristics;




/* ---------------------------------------------------------------------------
 * Description:
 * DMA Controller Configuration Structure
 *
 * Implementation:
 * Each DMA controller needs to be individually configured before it can
 * be used.
 *
 */
typedef struct apDMA_xCtrlrConfig
{
    bool                        AHB1BigEndian;      /* AHB Bus 1 big endian or little endian mode */
    BOOL                        AHB2BigEndian;      /* AHB Bus 2 big endian or little endian mode */
    apDMA_sXferCharacteristics  sDefaultMemAccess;  /* Default Memory Access Info               */

} apDMA_sCtrlrConfig;





/*
 *              --------Common Procedure declarations--------
 */



/* ---------------------------------------------------------------------------
 * Description:
 * This is the raw interrupt handler for the module, to be called directly
 * from the interrupt vector
 *
 * Note:
 * NOT FOR GENERAL USE.  This routine should only be executed as a branch from the IRQ vector
 *
 * Inputs:
 * none
 *
 * Outputs:
 * none
 *
 * Return Value:
 * none
 */
void apDMA_RawISR(void);

/* ---------------------------------------------------------------------------
 * Description:
 *  DMA Driver Interrupt Handler
 *
 * Remarks:
 *  Called by the Interrupt Controller Driver when any interrupt from
 *  any of the DMA controllers occurs.
 * Note:
 * NOT FOR GENERAL USE.  This routine should only be called by an interrupt dispatcher
 *
 * Inputs:
 *  oInterruptId   - The Interrupt Id on which the interrupt has occured
 *  DeviceId       - The unique ID that the DMA driver assigned to this
 *                   interrupt when it registered with the ineterrupt driver
 *
 * Outputs:
 *  None
 *
 * Return Value:
 *  none
 *
 */

void apDMA_IntHandler(const uint32_t oInterruptId);


/* ---------------------------------------------------------------------------
 * Description:
 * Finds the amount of space required for driver state data
 *
 * Implementation:
 * This function is required if apOS_NO_STATIC_STATE is defined as TRUE
 * as it will retrieve the size required for storage of the driver
 * state data
 *
 * Inputs:
 * none
 * 
 * Outputs:
 * none
 *
 * Return Value:
 * size (in bytes) required.
 */
uint32_t apDMA_StateSizeGet(void);



/* ---------------------------------------------------------------------------
 * Description:
 * Initialise the specified DMA controller.
 *
 * Implementation:
 * The DMA driver supports multiple DMA controllers, each has to be
 * individually initialised. This must be done before the DMA controller
 * can be used, hence this is usually the very first DMA driver function
 * to be called.
 *
 * Inputs:
 * oId                 - The DMA controller being intialised
 * eBase               - The base address of the controller
 * NumberInterrupts    - Number of interrupt Ids in eSources[]
 * pSources            - Ptr to array of interrupt Ids allocated to this
 *                        DMA controller
 * pInitial            - Pointer to a config info structure for this ctrlr
 *
 * Outputs:
 * none
 *
 * Return Value:
 * none
 */
void apDMA_Initialize(

    uint32_t        eBase,
    uint32_t                         Interrupts,
    apDMA_sCtrlrConfig              *pInitial);

/* ---------------------------------------------------------------------------
 * Description:
 * Specify a virtual memory table for the specified DMA controller to use
 *
 * Implementation:
 * NumAddressBlocks    - Number of virtual address blocks in table
 * pAddressBlocks      - Ptr to array of virtual address blocks
 *
 * Inputs:
 * oId                 - The DMA controller being intialised
 *
 * Outputs:
 * none
 *
 * Return Value:
 * apERR_NONE          - initialised OK
 * apERR_BAD_PARAMETER - if any parameter is considered to be invalid
 * apERR_DMA_CTRLR_NOT_INTIALISED - speaks for itself
 *
 */
apError apDMA_VirtualMemoryTable(

    uint32_t                             NumAddressBlocks,
    apDMA_sAddressBlockCharacteristics  *pAddressBlocks);




/* ---------------------------------------------------------------------------
 * Description:
 * Re-enables the specified DMA controller.
 *
 * Implementation:
 * Used to re-enable a controller that has been manually disabled.
 * The Controllers are enabled when apDMA_Initialize() is called.
 * Enabling a controller that is already enabled has no effect and
 * the fn returns apERR_NONE.
 * The DMA controller uses less power when disabled.
 *
 * Inputs:
 * oId     - The DMA controller to be re-enabled
 *
 * Outputs:
 * none
 *
 * Return Value:
 * apERR_NONE                      - enabled OK
 * apERR_BAD_PARAMETER             - if oId is invalid
 * apERR_DMA_CTRLR_NOT_INTIALISED  - if the ctrlr hasn't been initialised
 * apERR_DMA_CTRLR_DISABLED        - if the ctrlr failed to become enabled
 */
apError apDMA_ControllerEnable();


/* ---------------------------------------------------------------------------
 * Description:
 * Disables the specified DMA controller.
 *
 * Implementation:
 * Used to disable an idle DMA controller.
 * Disabling a controller that is already disabled has no effect and
 * the function returns apERR_NONE.
 * The function does not permit a controller that has active transfers
 * in progress to be disabled
 * The DMA controller uses less power when disabled.
 *
 * Inputs:
 * oId     - The DMA controller to be disabled
 *
 * Outputs:
 * none
 *
 * Return Value:
 * apERR_NONE
 * apERR_BAD_PARAMETER             - if oId is invalid
 * apERR_DMA_CTRLR_ACTIVE          - ctrlr has active transfers in progress
 * apERR_DMA_CTRLR_NOT_INTIALISED  - speaks for itself
 *
 */
apError apDMA_ControllerDisable();




/*
 *              ================== API =========================
 */





/*
 *              ---------- API Structure definitions-----------
 */


/* ---------------------------------------------------------------------------
 * Description: 
 * Raw Linked List Item used with API for peripheral/memory transfers.
 *
 * Implementation:
 * This structure is only used internally by the DMA driver. Users should
 * never read or write to these fields.
 *
 */
typedef struct apDMA_xRawLLI
{
    /* The DMA driver fills these fields */

    uint32_t SrcAddr;
    uint32_t DstAddr;
    uint32_t NextLLI;
    uint32_t TransferCtrl;

} apDMA_sRawLLI;



/* ---------------------------------------------------------------------------
 * Description: 
 * Linked List Item used with API for peripheral/memory transfers.
 *
 * Implementation:
 * This structure should only be used for peripheral/memory transfers
 * not for memory to memory or peripheral to peripheral.
 * IMPORTANT NOTE :
 *  Due to the design of the DMA PrimeCell controller, each 
 *  apDMA_sPerMemLLI MUST be aligned on a WORD boundary.
 */
typedef struct apDMA_xPerMemLLI
{
    /* The DMA driver fills in the first field.*/

    apDMA_sRawLLI   sRaw;

    /* The user has to fill in the remaining fields.*/

    uint32_t                  NumTransfers;   /* number of transfers to perform */
    void                     *pMemAddress;   /* memory address */
    uint32_t                  Dummy;
    struct apDMA_xPerMemLLI  *pNextLLI;      /* Ptr to next LLI or NULL if this is the last */

} apDMA_sPerMemLLI;



/* ---------------------------------------------------------------------------
 * Description: 
 * Linked List Item used with API for memory to memory transfers.
 *
 * Implementation:
 * This structure should only be used for memory to memory transfers
 * not for memory/peripheral or peripheral to peripheral.
 * IMPORTANT NOTE :
 *  Due to the design of the DMA PrimeCell controller, each 
 *  apDMA_sMemMemLLI MUST be aligned on a WORD boundary.
 */
typedef struct apDMA_xMemMemLLI
{
    /* The DMA driver fills in the first field.*/

    apDMA_sRawLLI   sRaw;

    /* The user has to fill in the remaining fields */

    uint32_t                   NumTransfers;   /* number of transfers to perform */
    void                      *pSrcMemAddr;   /* Source address */
    void                      *pDstMemAddr;   /* Destination address */
    struct apDMA_xMemMemLLI   *pNextLLI;      /* Ptr to next LLI or NULL if this is the last */

} apDMA_sMemMemLLI;


/*
 *              -------- API Procedure declarations--------
 */


/* ---------------------------------------------------------------------------
 * Description:
 * Supply DMA driver with info concerning characteristics of a peripheral.
 *
 * Implementation:
 * Must be called for each peripheral that wants to do a DMA transfer
 * before any transfers with that peripheral can be intiated.
 * The details supplied are unlikely to be changed, but users can use
 * this function to change peripheral details as and when they want. However
 * users are advised to only do so when the peripheral is not involved in any
 * ongoing transfer.
 *
 * Inputs:
 * oId                 - Controller Id
 * PeripheralId        - The DMA Peripheral Id
 * eBurstSize          - The burst transfer size
 * eWidth              - The width of the transfers
 * eAhbBus             - Which AHB bus this peripheral is physically on
 * eDmaSync            - Enable or Disable Peripheral Synchronization Logic
 * Protection          - How to set the bus protection lines when this
 *                       peripheral is involved in a transfer.
 *                       A combination of apDMA_eProtectionBits values
 *                       ORed together and cast to a uint32_t
 *
 * Outputs:
 * none
 *
 * Return Value:
 * apERR_NONE                      - If the request has been accepted
 * apERR_DMA_CTRLR_NOT_INITIALISED - If the associated DMA controller has
 *                                    not been initialised
 * apERR_BAD_PARAMETER             - If any of the params are invalid
 */
apError apDMA_PeripheralConfigure(
    uint32_t          PeripheralId,
    apDMA_eBurstSize eBurstSize,
    apDMA_eWidth     eWidth,
    apDMA_eAhbBus    eAhbBus,
    apDMA_eSync      eDmaSync,
    uint32_t          Protection);


/* ---------------------------------------------------------------------------
 * Description:
 * Supply the DMA driver with peripheral specific info (known only to the
 * driver of the peripheral concerned)
 *
 * Implementation:
 * Must be called before any transfers with that peripheral can be intiated.
 * The details supplied are unlikely to be changed, but users can use
 * this function to change peripheral details, as and when they want. However
 * users are advised to only do so when the peripheral is not involved in any
 * ongoing transfer.
 *
 * Inputs:
 * oId                 - Controller Id
 * PeripheralId        - The DMA Peripheral Id
 * pRegisterAddress    - The physical address of the peripheral's data
 *                       register to or from which all data will be transferred.
 *                       This must be the correct address for the AHB bus
 *                       that the peripheral is physically connected to.
 * rTerminatedCb       - Pointer to the peripheral driver's 
 *                       'transfer terminated' call back function which the
 *                       DMA driver will call when the transfer is compelete.
 *                       Users can specify NULL if they don't wish to be
 *                       told when the transfer has terminated.
 * Outputs:
 * none
 *
 * Return Value:
 * apERR_NONE                      - If the request has been accepted
 * apERR_DMA_CTRLR_NOT_INITIALISED - If the associated DMA controller has
 *                                   not been initialised
 * apERR_BAD_PARAMETER             - If any of the params are invalid
 */
apError apDMA_PeripheralAddressSet(
    uint32_t                   PeripheralId,
    void                      *pRegisterAddress,
    apDMA_rTransferTerminated rTerminatedCb);


/* ---------------------------------------------------------------------------
 * Description:
 * Change how the bus protection lines should be set when the specified
 * peripheral is involved in a transfer.
 *
 * Implementation:
 * Users are advised to only call this function when the peripheral is not
 * involved in any ongoing transfer.
 *
 * Inputs:
 * oId                 - Controller Id
 * PeripheralId        - The DMA Peripheral Id
 * Protection          - How to set the bus protection lines when this
 *                       A combination of apDMA_eProtectionBits values
 *                       ORed together and cast to a uint32_t
 *                       peripheral is involved in a transfer
 * Outputs:
 * none
 *
 * Return Value:
 * apERR_NONE                      - If the request has been accepted
 * apERR_DMA_CTRLR_NOT_INITIALISED - If the associated DMA controller has
 *                                   not been initialised
 * apERR_BAD_PARAMETER             - If any of the params are invalid
 * apERR_DMA_PERIPHERAL_NOT_INITIALISED -  If the specified internal
 *                                         peripheral record has not been
 *                                         configured
 * apERR_DMA_CTRLR_DISABLED        - If the DMA controller is disabled
 */
apError apDMA_PeripheralProtectionSet(
    uint32_t      PeripheralId,
    uint32_t      Protection);



/* ---------------------------------------------------------------------------
 * Description:
 * Used by the application to initiate a peripheral to peripheral transfer. 
 *
 * Implementation:
 * The DMA driver must have already been given all the config info
 * concerning the specified peripherals prior to this fn being called.
 * This function can not be used to intialise a Linked List transfer.
 *
 * Inputs:
 * oId                     - Controller Id
 * ChanId                  - The DMA Channel Id that should be used
 *                           for this transfer
 * SourcePeripheralId      - The DMA Peripheral Id of the source peripheral
 * DestPeripheralId        - The DMA Peripheral Id of the dest peripheral
 * UsersId                 - Users own Id - returned when transfer complete
 *                           Has no meaning to the DMA driver.
 * NumTransfers            - Number of transfer to perform
 * eFlowCtrl               - What direction data is to flow & who is flow ctrlr
 * Protection              - How to set the bus protection lines when doing
 *                           this transfer...
 *                           A combination of apDMA_eProtectionBits values
 *                           ORed together and cast to a uint32_t
 *
 * Outputs:
 * none
 *
 * Return Value:
 * apERR_NONE                      - If the request has been accepted
 * apERR_DMA_CTRLR_NOT_INITIALISED - If the associated DMA controller has
 *                                   not been initialised
 * apERR_BUSY                      - If the associated DMA channels of
 *                                    any of the specified peripherals is
 *                                    already in use.
 * apERR_BAD_PARAMETER             - If any of the params are invalid
 * apERR_DMA_PERIPHERAL_NOT_INITIALISED -  If any of the specified
 *                                         internal peripheral records have
 *                                         not been configured
 * apERR_DMA_CTRLR_DISABLED        - If the DMA controller is disabled
 */
apError apDMA_PeripheralToPeripheralTransferRequest(

    uint32_t                 ChanId,
    uint32_t                 SourcePeripheralId,
    uint32_t                 DestPeripheralId,
    uint32_t                 UsersId,
    uint32_t                 NumTransfers,
    apDMA_eFlowControl      eFlowCtrl,
    uint32_t                 Protection);



/* ---------------------------------------------------------------------------
 * Description:
 * Used to initiate a transfer between a peripheral and memory.
 *
 * Implementation:
 * The DMA driver must have already been given all the config info
 * concerning the requesting peripheral prior to this fn being called.
 * This function can not be used to intialise a Linked List transfer.
 *  
 *
 * Inputs:
 * oId                 - Controller Id
 * ChanId              - The DMA Channel Id that should be used
 *                       for this transfer
 * PeripheralId        - The DMA Peripheral Id
 * UsersId             - Users own Id - returned when transfer complete
 *                       Has no meaning to the DMA driver.
 * NumTransfers        - Number of transfer to perform
 * eFlowCtrl           - What direction data is to flow & who is flow ctrlr
 * pMemAddr            - Ptr to mem where data is to be transferred to or from
 *
 * Outputs:
 * none
 *
 * Return Value:
 * apERR_NONE                      - If the request has been accepted
 * apERR_DMA_CTRLR_NOT_INITIALISED - If the associated DMA controller has
 *                                   not been initialised
 * apERR_BUSY                      - If the associated DMA channel is
 *                                   already in use
 * apERR_BAD_PARAMETER             - If any of the params are invalid
 * apERR_DMA_PERIPHERAL_NOT_INITIALISED -  If the specified internal
 *                                         peripheral record has not been
 *                                         configured
 * apERR_DMA_CTRLR_DISABLED        - If the DMA controller is disabled
 */
apError apDMA_MemPeripheralTransferRequest(

    uint32_t             ChanId,
    uint32_t             PeripheralId,
    uint32_t             UsersId,
    uint32_t             NumTransfers,
    apDMA_eFlowControl  eFlowCtrl,
    void                *pMemAddr);


/* ---------------------------------------------------------------------------
 * Description:
 * Initiate a Linked List Memory/Peripheral Transfer  
 *
 * Implementation:
 * Called to initiate a linked list memory/peripheral transfer.
 * The direction of all transfers specified
 * must be the same (ie all to the peripheral or all from the peripheral),
 * All transfers must be to/from the same peripheral.
 * At least one linked list item must be specified.
 *
 * Inputs:
 * oId                 - Controller Id
 * ChanId              - The DMA Channel Id that should be used
 *                       for this transfer
 * PeripheralId        - The DMA Peripheral Id
 * UsersId             - Users own Id - returned when transfer complete
 *                       Has no meaning to the DMA driver.
 * eFlowCtrl           - What direction data is to flow & who is flow ctrlr
 * NumLLItems          - Number of Linked List Items (min 1)
 * pFirstLLI           - Ptr to first Linked List Item
 * CbPerItem           - If TRUE then user's 'transfer terminated' call back
 *                       function will be called as each linked list
 *                       item is completed. If set to FALSE then the
 *                       user's 'transfer terminated' call back will only
 *                       be called when all linked list items have been
 *                       completed (or an error has occured).
 * Outputs:
 * none  
 *
 * Return Value:
 * apERR_NONE                      - If the request has been accepted
 * apERR_DMA_CTRLR_NOT_INITIALISED - If the associated DMA controller has
 *                                   not been initialised
 * apERR_BUSY                      - If the associated DMA channel is
 *                                   already in use
 * apERR_BAD_PARAMETER             - If any of the params are invalid
 * apERR_DMA_PERIPHERAL_NOT_INITIALISED -  If the specified internal
 *                                         peripheral record has not been
 *                                         configured
 * apERR_DMA_CTRLR_DISABLED        - If the DMA controller is disabled
 */
apError apDMA_LinkedMemPeripheralTransferRrequest(

    uint32_t             ChanId,
    uint32_t             PeripheralId,
    uint32_t             UsersId,
    apDMA_eFlowControl  eFlowCtrl,
    uint32_t             NumLLItems,
    apDMA_sPerMemLLI    *pFirstLLI,
    BOOL                CbPerItem);



/* ---------------------------------------------------------------------------
 * Description:
 * Used by the application to initiate a memory to memory transfer.
 *
 * Implementation:
 * This function can not be used to intialise a Linked List transfer.
 *
 * Inputs:
 * oId                 - Controller Id
 * ChanId              - The DMA Channel Id that should be used
 *                       for this transfer
 * UsersId             - Users own Id - returned when transfer complete
 *                       Has no meaning to the DMA driver.
 * rTerminatedCb       - Pointer to the user's 'transfer terminated' call
 *                       back function which the DMA driver will call when
 *                       the transfer is compelete. Users can specify NULL
 *                       if they don't wish to be told when the transfer
 *                       has terminated.
 * Protection          - How to set the bus protection lines when doing
 *                       this transfer...
 *                            A combination of apDMA_eProtectionBits values
 *                            ORed together and cast to a uint32_t
 * NumTransfers        - Number of transfer to perform
 * pSources            - Ptr to where the data is to be transferred from
 * pDest               - Ptr to where its going
 *
 * Outputs:
 * none
 *
 * Return Values:
 * apERR_NONE                      - If the request has been accepted
 * apERR_DMA_CTRLR_NOT_INITIALISED - If the associated DMA controller has
 *                                   not been initialised
 * apERR_BUSY                      - If the associated DMA channel is
 *                                   already in use
 * apERR_BAD_PARAMETER             - If any of the params are invalid
 * apERR_DMA_CTRLR_DISABLED        - If the DMA controller is disabled
 */
apError apDMA_MemToMemTransferRequest(

    uint32_t                     ChanId,
    uint32_t                     UsersId,
    apDMA_rTransferTerminated   rTerminatedCb,
    uint32_t                     Protection,
    uint32_t                     NumTransfers,
    void                        *pSources,
    void                        *pDest);


/* ---------------------------------------------------------------------------
 * Description:
 * Initiate a Linked List Memory to Memory Transfer  
 *
 * Implementation:
 * Called by the application to initiate a linked list memory to memory
 * transfer.
 * At least one linked list item must be specified.
 *
 * Inputs:
 * oId                 - Controller Id
 * ChanId              - The DMA Channel Id that should be used
 *                       for this transfer
 * UsersId             - Users own Id - returned when transfer complete
 *                       Has no meaning to the DMA driver.
 * rTerminatedCb       - Pointer to the user's 'transfer terminated' call
 *                       back function which the DMA driver will call when
 *                       the transfer is compelete. Users can specify NULL
 *                       if they don't wish to be told when the transfer
 *                       has terminated.
 * Protection          - How to set the bus protection lines when doing
 *                       this transfer...
 *                           A combination of apDMA_eProtectionBits values
 *                           ORed together and cast to a uint32_t
 * NumLLItems          - Number of Linked List Items (min 1)
 * pFirstLLI           - Ptr to first Linked List Item
 * CbPerItem           - If TRUE then user's 'transfer terminated' call back
 *                       function will be called as each linked list
 *                       item is completed. If set to FALSE then the
 *                       user's 'transfer terminated' call back will only
 *                       be called when all linked list items have been
 *                       completed (or an error has occured).
 * Outputs:
 * none  
 *
 * Return Value:
 * apERR_NONE                      - If the request has been accepted
 * apERR_DMA_CTRLR_NOT_INITIALISED - If the associated DMA controller has
 *                                   not been initialised
 * apERR_BUSY                      - If the associated DMA channel is
 *                                   already in use
 * apERR_BAD_PARAMETER             - If any of the params are invalid
 * apERR_DMA_CTRLR_DISABLED        - If the DMA controller is disabled
 */
apError apDMA_LinkedMemToMemTransferRequest(

    uint32_t                     ChanId,
    uint32_t                     UsersId,
    apDMA_rTransferTerminated   rTerminatedCb,
    uint32_t                     Protection,
    uint32_t                     NumLLItems,
    apDMA_sMemMemLLI            *pFirstLLI,
    BOOL                        CbPerItem);


/* ---------------------------------------------------------------------------
 * Description:
 * Called by the application to terminate an ongoing DMA transfer.
 *
 * Implementation:
 * Can be called to terminate transfers that were initiated by the API
 * The DMA driver will immediately terminate the transfer and mark the
 * DMA channel as idle.
 * If the channel is marked as being used by the Low Level API, then the
 * DMA driver does NOT call the 'transfer terminated' call back function.
 * If the channel is NOT marked as being used by the Low Level API, then
 * the DMA driver calls the user's 'transfer terminated' call back function.
 *
 * Inputs:
 * oId         - Which DMA controller to use
 * DmaChannel  - Which DMA channel to use
 *
 * Outputs:
 * pCount      - pointer to store the following:
 *                 0              - if     invlaid DMA Ctrlr Id or DMA Channel or
 *                                  the DMA controller wasn't the flow controller or
 *                                  more than one linked list item was specified or
 *                                  the DMA channel was idle.
 *                 num transfers  - Number of transfers performed.  
 *
 * Return Value:
 * apERR_NONE                      - If the request has been accepted
 * apERR_BAD_PARAMETER             - If any of the params are invalid
 *  
 */
apError apDMA_AbortTransfer(
    uint32_t      DmaChannel,
    uint32_t      *const pCount);

apError apDMA_GetCount(
    uint32_t      ChanId,
    uint32_t      *const pCount);

/* ---------------------------------------------------------------------------
 * Description:
 * Called by the application to terminate an ongoing DMA transfer.
 *
 * Implementation:
 * Can be called to terminate transfers that were initiated by the API
 * The DMA driver will set the HALT bit in the configuration register and
 * then wait for the ACTIVE bit to go low. The transfer will then be terminated
 * and mark the DMA channel as idle.
 * If the channel is marked as being used by the Low Level API, then the
 * DMA driver does NOT call the 'transfer terminated' call back function.
 * If the channel is NOT marked as being used by the Low Level API, then
 * the DMA driver calls the user's 'transfer terminated' call back function.
 *
 * Inputs:
 * oId         - Which DMA controller to use
 * DmaChannel  - Which DMA channel to use
 *
 * Outputs:
 * pCount      - pointer to store the following:
 *                 0              - if     invlaid DMA Ctrlr Id or DMA Channel or
 *                                  the DMA controller wasn't the flow controller or
 *                                  more than one linked list item was specified or
 *                                  the DMA channel was idle.
 *                 num transfers  - Number of transfers performed.  
 *
 * Return Value:
 * apERR_NONE                      - If the request has been accepted
 * apERR_BAD_PARAMETER             - If any of the params are invalid
 *  
 */
apError apDMA_HaltTransfer(
    uint32_t      DmaChannel,
    uint32_t      *const pCount);


/* 
 *          ==================== LOW LEVEL API =========================
 */

/*
 *          -----------Low Level API Structure definitions------------
 */



/* ---------------------------------------------------------------------------
 * Description: 
 * Transfer access info. For Src & Dest. Low Level API
 *
 * Implementation:
 * The sAccessInfo field is only used if the PhysicalAddressing field of
 * the apDMA_sTransfer structure is set to TRUE.
 */
typedef struct apDMA_xTransferInfo
{
    void                        *pAddr;         /* Address of the src/dst data */
    apDMA_sXferCharacteristics  sAccessInfo;    /* Characteristics of the mem. Only used
                                                 * when apDMA_sTransfer indicates
                                                 * physical addressing
                                                 */
    BOOL                        AutoInc;        /* increment address on each transfer */

} apDMA_sTransferInfo;



/* ---------------------------------------------------------------------------
 * Description: 
 * A Low Level Linked List Item.
 *
 * Implementation:
 * This structure should only be used by the low level API.
 *
 * IMPORTANT NOTE :
 *
 * Due to the design of the DMA PrimeCell controller, each apDMA_sLLI
 * MUST be aligned on a WORD boundary.
 *
 */
typedef struct apDMA_xLLI
{
    /* The DMA driver fills in the first field */

    apDMA_sRawLLI   sRaw;

    /* The user has to fill in the remaining fields */

    uint32_t                 NumTransfers;       /* 0 - 4095 */
    uint32_t                 Protection;         /* Combination of apDMA_eProtectionBits */
    BOOL                    TCInterruptEnable;  /* Transfer terminated interrupt enable */

    apDMA_sTransferInfo     sSrc;               /* Where data to be transferred from */
    apDMA_sTransferInfo     sDst;               /* Where data to be transferred to */
    struct apDMA_xLLI       *pNextLLI;          /* Ptr to next linked list item or
                                                 * NULL if this is the last */

} apDMA_sLLI;



/* ---------------------------------------------------------------------------
 * Description: 
 * Full details on the DMA transfer requested
 * Used in Low Level API only.
 */
typedef struct apDMA_xTransfer
{
    apDMA_eFlowControl      eFlowCtrl;          /* flow control */

    uint32_t                 SrcPeripheralId;    /* 0 - 31 or apDMA_NO_PERIPHERAL_ID */
    uint32_t                 DstPeripheralId;    /* 0 - 31 or apDMA_NO_PERIPHERAL_ID */
    BOOL                    LockEnable;         /* lock bus on transfers */
    BOOL                    PhysicalAddressing; /* Src & Dst addresses in LLIs are physical */
    uint32_t                 NumLLItems;         /* Number of linked list items (min 1) */
    apDMA_sLLI              *pLLI;              /* Ptr to first LLI */

    uint32_t                         UserId;     /* Id passed back in call back fn */
    apDMA_rTransferTerminated    rTTCallBack;   /* Ptr to user's transfer terminated call back
                                                 * function, or NULL if none avail */

} apDMA_sTransfer;


/*
 *          --------Low Level API Procedure declarations--------
 */


/* ---------------------------------------------------------------------------
 * Description:
 * Set up a DMA transfer. - Low Level API
 *
 * Implementation:
 * Used to set up any of the DMA transfers supported by the DMA controller.
 *
 * Inputs:
 * oId         - Which DMA controller to use
 * DmaChannel  - When DMA channel to use
 * pParam      - Ptr to rest of the details for this transfer
 *
 * Outputs:
 * none
 *
 * Return Value:
 * apERR_NONE                      - If the request has been accepted
 * apERR_DMA_CTRLR_NOT_INITIALISED - If the associated DMA controller has
 *                                   not been initialised
 * apERR_BAD_PARAMETER             - If any of the params are invalid
 * apERR_DMA_CTRLR_DISABLED        - If the DMA controller is disabled
 */
apError apDMA_SetupDMATransfer(
    uint32_t             DmaChannel,
    apDMA_sTransfer     *pParam);



#ifdef __cplusplus
} /* allow C++ to use these headers */
#endif /* __cplusplus */

#endif    /* End of APDMA_H */

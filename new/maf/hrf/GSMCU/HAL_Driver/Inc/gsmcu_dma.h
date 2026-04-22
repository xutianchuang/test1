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
 * File:     dma.h,v
 * Revision: 1.20
 * ----------------------------------------------------------------
 * 
 *  ----------------------------------------
 *  Version and Release Control Information:
 * 
 *  File Name              : dma.h.rca
 *  File Revision          : 1.2
 * 
 *  Release Information    : PrimeCell(TM)-PL080-r1p1-00ltd0
 *  ----------------------------------------
 */
 
/*
 * Private header file for the DMA Interface Driver code.
 * This file contains the headers for the API implementation of the code
 *
 * NOTE :
 *  The function headers in this file show all possible return values
 *  that can occur when the debug macro apDEBUG_ENABLED is defined. If this
 *  macro is not defined, then most functions will not do validity checking
 *  and therefore only return a subset of the specified return values.
 *  Code using these functions must assume all stated return values can occur.
 */

#ifndef DMA_H
#define DMA_H

#ifdef __cplusplus
extern "C" { /* allow C++ to use these headers */
#endif /* __cplusplus */

#include "apdma.h"
/*
 *------------------------Type definitions------------------------
 */

/*
 * Description:
 *  DMA Internal Peripheral Record - configuration states
 *
 * Remarks:
 *  Each DMA internal peripheral record needs to be configured by the
 *  OS and by the driver of the coreesponding peripheral. This enum indicates
 *  the possible configuration states.
 */

typedef enum DMA_xPeriphConfigState
{
    DMA_PERIPH_NOT_CONFIGURED,      /* Not configured at all */
    DMA_PERIPH_OS_CONFIG,           /* OS Config only */
    DMA_PERIPH_PERIPH_CONFIG,       /* Peripheral Config only */
    DMA_PERIPH_FULLY_CONFIGURED     /* Fully configured */

} DMA_ePeriphConfigState;


/*
 * Description:
 *  DMA Channel Record - current state
 *
 * Remarks:
 *  Used to indicate the current state of a DMA channel
 */

typedef enum DMA_xState
{
    DMA_STATE_IDLE,                         /* not in use */
    DMA_STATE_ACTIVE_LOW_TRANSFER,          /* Low level API originated - Single LLI transfer ongoing */
    DMA_STATE_ACTIVE_LOW_MULTI_LLI_TRANSFER,/* Low level API originated - Multi LLI transfer ongoing */
    DMA_STATE_ACTIVE_TRANSFER,              /* API originated - transfer ongoing */
    DMA_STATE_ACTIVE_LLI_TRANSFER,          /* API originated - LLI transfer ongoing */
    DMA_STATE_ACTIVE_P_TO_P_TRANSFER        /* API originated - peripheral to peripheral transfer ongoing */

} DMA_eState;


/*
 * Description:
 *  DMA Channel Record - current user
 *
 * Remarks:
 *  Used to indicate who is using a DMA channel
 */

typedef enum DMA_xUser
{
    DMA_USER_NONE,              /* not in use */
    DMA_USER_LOW_LEVEL_API,     /* being used via low level API */
    DMA_USER_APPLICATION,       /* being used by the application */
    DMA_USER_PERIPHERAL         /* being used by a peripheral */

} DMA_eUser;


/*
 * Description:
 *  DMA Data Source enum
 *
 * Remarks:
 *  Used to indicate whether memory or a peripheral is the source of data
 */

typedef enum DMA_xDataSrc
{
    DMA_MEM_SRC,        /* source of data is memory */
    DMA_PERIPH_SRC      /* source of data is a peripheral */

} DMA_eDataSrc;


/*
 * Description:
 *  DMA TC Interrupt enable
 *
 * Remarks:
 *  
 */

typedef enum DMA_xTCInterruptFlag
{
    DMA_TC_INT_DISABLE  = 0,
    DMA_TC_INT_ENABLE   = 1

} DMA_eTCInterruptFlag;


/* ---------------------------------------------------------------------------
 * Description:
 *  DMA Transfer Info Record
 *
 * Remarks:
 *  Used to hold details on how to transfer data to / from the specified addr.
 */

typedef struct DMA_xXferInfo
{
    uint32_t                     PhysAddr;       /* Physical address */
    BOOL                        Increment;      /* Auto increment flag */
    apDMA_sXferCharacteristics  *pXferAccess;   /* Ptr to Access characteristics */

} DMA_sXferInfo;


/* ---------------------------------------------------------------------------
 * Description:
 *  DMA Internal Peripheral Record
 *
 * Remarks:
 *  The DMA internal peripheral record is split into a config and status
 *  section.
 */

typedef struct DMA_xPeriphInfo
{
    /* config fields */

    DMA_ePeriphConfigState       eConfig;        /* Configuration State */
    void                         *pRegisterAddress;  /* Peripharal's data register addr */
    apDMA_sXferCharacteristics   sXferAccess;    /* Transfer Access characteristics */
    uint32_t                      Protection;     /* How to set bus protection lines when accessing periph */
    apDMA_rTransferTerminated    rTerminatedCb;  /* Ptr to user's Transfer Terminated Call Back Fn */

    /* Status fields */

    BOOL                UseCallBack;        /* If TRUE then calls user's Transfer Terminated
                                               Call back fn on termination */
    uint32_t             CurrentDmaChannel;  /* 0 - 31, or apDMA_NO_DMA_CHANNEL */
    uint32_t             UsersId;            /* User's Id, returned on termination, No meaning to DMA driver */

} DMA_sPeriphInfo;


/* ---------------------------------------------------------------------------
 * Description:
 *  DMA Channel Status Record
 *
 * Remarks:
 *  One per DMA channel
 */

typedef struct DMA_xChannelStatus
{
    DMA_eState              eState;             /* State of the channel */
    DMA_eUser               eUser;              /* Indicates who is using this channel */
    apDMA_eFlowControl      eFlowCtrl;          /* flow control */
    uint32_t                 Protection;         /* bus protection bits */
    BOOL                    LockEnable;         /* lock bus on transfers */
    BOOL                    CbPerItem;          /* If TRUE call back function will be 
                                                 * called as each linked list item is completed.
                                                 * If set to FALSE then call back will only
                                                 * be called when all linked list items have 
                                                 * been completed (or an error has occured).*/
    uint32_t                 NumTransfers;       /* Number of transfers requested */

    /*  The following fields indicate which peripheral records are using
     *  this channel. These fields are copied into the DMA channel
     *  config register.
     */

    uint32_t     SrcPeripheralId;    /* 0 - 31 or apDMA_NO_PERIPHERAL_ID */
    uint32_t     DstPeripheralId;    /* 0 - 31 or apDMA_NO_PERIPHERAL_ID */

    /*  The following fields are only used if eUser is set to 
     *   DMA_USER_LOW_LEVEL_API or DMA_USER_APPLICATION
     */

    uint32_t                         UsersId;            /* The User's Id */
    apDMA_rTransferTerminated       rTerminatedCb;      /* The User's Call back function */

} DMA_sChannelStatus;


/* ---------------------------------------------------------------------------
 * Description:
 *  DMA Controller - Fixed Config
 *
 * Remarks:
 *  The info in this structure is read from the prime cell itself. 
 */

typedef struct DMA_xCtrlrFixedConfig
{
    uint32_t         NumDmaChannels; /* Number of DMA channels */
    uint32_t         NumAhbIf;       /* Number of AHB master interfaces */
    uint32_t         NumPeripherals; /* Number of peripheral source requestors */

    apDMA_eAhbWidth eAhbWidth;      /* Width of AHB bus(es) */

} DMA_sCtrlrFixedConfig;


/* ---------------------------------------------------------------------------
 * Description:
 *  DMA Controller Record
 *
 * Remarks:
 *  All the info concerning an individual DMA controller is held in this record
 */

typedef struct DMA_xStateStruct
{
    uint32_t    Base;           /* base address of this ctrlr */

    BOOL                        Initialised;    /* Initialised flag */
    BOOL                        Enabled;        /* Enabled if TRUE */
    BOOL                        AHB1BigEndian;  /* AHB 1 Big endian or little endian mode */
    BOOL                        AHB2BigEndian;  /* AHB 2 Big endian or little endian mode */

    apDMA_sXferCharacteristics          sDefaultMemAccess;  /* Default Memory Access Info */
    uint32_t                             NumAddressBlocks;
    apDMA_sAddressBlockCharacteristics  *pAddressBlocks;

    DMA_sCtrlrFixedConfig       sFixedConfig;  /* Info read from chip */
    DMA_sChannelStatus          sChans[ apDMA_MAX_CHANNELS ];
    DMA_sPeriphInfo             sPeriphs[ apDMA_MAX_PERIPHERALS ];

} DMA_sStateStruct;






/*
 *        -------Controller Hardware Access Definitions-------
 */



/*
 * Description:
 *  DMA Controller - Configuration Register
 *
 */

#define bwDMA_CTRLR_ENABLED     1   /* DMA Ctrlr enabled flag */
#define bwDMA_AHB1_ENDIAN       1   /* AHB Bus 1 Endianness */
#define bwDMA_AHB2_ENDIAN       1   /* AHB Bus 2 Endianness */

#define bsDMA_CTRLR_ENABLED     0
#define bsDMA_AHB1_ENDIAN       1
#define bsDMA_AHB2_ENDIAN       2



/*
 * Description:
 *  DMA Controller - Protection Register
 *
 */

#define bwDMA_PROTECTED_USE     1   /* Protected access bit */

#define bsDMA_PROTECTED_USE     0




/*
 * Description:
 *  DMA Controller - DMA Channel Control Register
 *
 */

#define bwDMA_TRANSFER_SIZE       12   /* Transfer size */
#define bwDMA_SRC_BURST_SIZE       3   /* Source Burst Size */
#define bwDMA_DEST_BURST_SIZE      3   /* Destination Burst Size */
#define bwDMA_SRC_WIDTH            3   /* Source Width */
#define bwDMA_DEST_WIDTH           3   /* Destination Width */
#define bwDMA_SRC_BUS              1   /* Source AHB Bus */
#define bwDMA_DEST_BUS             1   /* Destination AHB Bus */
#define bwDMA_SRC_INCREMENT        1   /* Source auto Increment */
#define bwDMA_DEST_INCREMENT       1   /* Destination auto Increment */
#define bwDMA_PROTECTION           3   /* Bus Protection Lines */
#define bwDMA_TC_INTERRUPT_ENABLE  1   /* TC Interrupt enable */

#define bsDMA_TRANSFER_SIZE        0
#define bsDMA_SRC_BURST_SIZE      12
#define bsDMA_DEST_BURST_SIZE     15
#define bsDMA_SRC_WIDTH           18
#define bsDMA_DEST_WIDTH          21
#define bsDMA_SRC_BUS             24
#define bsDMA_DEST_BUS            25
#define bsDMA_SRC_INCREMENT       26
#define bsDMA_DEST_INCREMENT      27
#define bsDMA_PROTECTION          28
#define bsDMA_TC_INTERRUPT_ENABLE 31


/*
 * Description:
 *  DMA Controller - DMA Channel LLI Register
 *
 */

#define bwDMA_NEXT_LLI_BUS      1   /* AHB Bus of next LLI */
#define bwDMA_LLI_RESERVED      1   /* Reserved (set to 0 on write) */
#define bwDMA_NEXT_LLI          30  /* Address of next LLI */

#define bsDMA_NEXT_LLI_BUS      0
#define bsDMA_LLI_RESERVED      1
#define bsDMA_NEXT_LLI          2


/*
 * Description:
 *  DMA Controller - DMA Channel Configuration Register
 *
 */

#define bwDMA_CHANNEL_ENABLED       1   /* Channel Enable */
#define bwDMA_SRC_PERIPHERAL        5   /* Source peripheral Id (0-31) */
#define bwDMA_DEST_PERIPHERAL       5   /* Destination peripheral Id (0-31) */
#define bwDMA_FLOW_CONTROL          3   /* Flow Control */
#define bwDMA_ERROR_INTERRUPT_MASK  1   /* Error Interrupt Mask */
#define bwDMA_TC_INTERRUPT_MASK     1   /* Terminal Count Interrupt Mask */
#define bwDMA_BUS_LOCK              1   /* Bus Lock */
#define bwDMA_ACTIVE                1   /* FIFO Active */
#define bwDMA_HALT                  1   /* Halt */
#define bwDMA_CONFIG_RESERVED       13  /* Reserved */

#define bsDMA_CHANNEL_ENABLED       0
#define bsDMA_SRC_PERIPHERAL        1
#define bsDMA_DEST_PERIPHERAL       6
#define bsDMA_FLOW_CONTROL          11
#define bsDMA_ERROR_INTERRUPT_MASK  14
#define bsDMA_TC_INTERRUPT_MASK     15
#define bsDMA_BUS_LOCK              16
#define bsDMA_ACTIVE                17
#define bsDMA_HALT                  18
#define bsDMA_CONFIG_RESERVED       19



/*
 * Description:
 *  Fields within Peripheral ID Register 3
 */

#define bwDMA_NUM_CHANNELS      3   /* Number of DMA channels */
#define bwDMA_NUM_AHB_BUS       1   /* Number of AHB Master interfaces */
#define bwDMA_AHB_WIDTH         3   /* Width of AHB Master interface(s) */
#define bwDMA_SOURCE_REQUESTORS 1   /* Number of DMA source requestors
                                     * 0 = 16 DMA requestors
                                     * 1 = 32 DMA requestors
                                     */

#define bsDMA_NUM_CHANNELS      0
#define bsDMA_NUM_AHB_BUS       3
#define bsDMA_AHB_WIDTH         4
#define bsDMA_SOURCE_REQUESTORS 7


#define DMA_NEXT_LLI_BUS 0
#define DMA_LLI_RESERVED 2
#define DMA_NEXT_LLI     0xfffffffC


#define CHANNEL_0 0
#define CHANNEL_1 1
#define CHANNEL_2 2
#define CHANNEL_3 3
#define CHANNEL_4 4
#define CHANNEL_5 5
#define CHANNEL_6 6



/*
 * Description:
 *  DMA Controller - DMA Channel Registers - template structure
 *
 */

typedef volatile struct DMA_xChannel
{
    uint32_t     SrcAddr;
    uint32_t     DestAddr;
    uint32_t     LLI;
    uint32_t     Control;
    uint32_t     Configuration;
    uint32_t     Reserved1;
    uint32_t     Reserved2;
    uint32_t     Reserved3;

} DMA_sChannel;


/*
 * Description:
 *  Constants defining the size (in 32 bit words) of reserved spaces in the
 *  DMA controllers address space
 *
 */

#define DMA_NUM_RSRVD_WRDS_BEFORE_CHANNELS      ( (0x100 - 0x038) >> 2 )
#define DMA_NUM_RSRVD_WRDS_BEFORE_PERIPHERAL_ID ( (0xfe0 - 0x500) >> 2 )


/*
 * Description:
 *  DMA Controller - Template structure of entire address space of
 *  a DMA controller
 *
 */

typedef volatile struct DMA_xPort
{
    uint32_t         IntStatus;              /* 0x000 */
    uint32_t         IntTCStatus;
    uint32_t         IntTCClear;
    uint32_t         IntErrorStatus;
    uint32_t         IntErrorClear;          /* 0x010 */
    uint32_t         RawIntTCStatus;
    uint32_t         RawIntErrorStatus;
    uint32_t         ActiveChannels;

    uint32_t         SoftBReq;               /* 0x020 */
    uint32_t         SoftSReq;               /* 0x024 */
    uint32_t         SoftLBReq;              /* 0x028 */
    uint32_t         SoftSBReq;              /* 0x02C */
    
    uint32_t         Configuration;          /* 0x030 */
    uint32_t         Sync;                   /* 0x034 */

    uint32_t         Reserved1[ DMA_NUM_RSRVD_WRDS_BEFORE_CHANNELS ];

    DMA_sChannel    sDmaChannels[ 32 ];     /* 0x100 -  */

    uint32_t         Reserved2[ DMA_NUM_RSRVD_WRDS_BEFORE_PERIPHERAL_ID ];

    uint32_t         PeripheralId0;          /* 0xFE0 */
    uint32_t         PeripheralId1;
    uint32_t         PeripheralId2;
    uint32_t         PeripheralId3;
    uint32_t         CellId0;                /* 0xFF0 */
    uint32_t         CellId1;
    uint32_t         CellId2;
    uint32_t         CellId3;

} DMA_sPort;


/*
 *        -------General Purpose Macro definitions-------
 */


/*
 * Description:
 *  Macro used to convert a BOOL to a binary value
 *
 *  eg. 
 *      DMA_BOOL_TO_BINARY( TRUE ) returns 1
 *      DMA_BOOL_TO_BINARY( FALSE ) returns 0
 */


#define DMA_BOOL_TO_BINARY( __val ) ( __val ? (1) : (0) )


/*
 * Description:
 *  Macro used to create a mask suitable for use with the interrupt/active
 *  regs of the DMA controller. Converts the DMA channel (0-31) into 
 *  the correspoding bit access mask: 0x00000001 to 0x10000000
 *
 */

#define DMA_CHAN_BIT( __val )   ( 1<<__val )

/* There's two versions of DMA_CHAN_BIT : */
/* apBIT_MASK_FIELD( 1, __val ) */
/* ( (uint32_t) 1U << __val ) */


#define apBIT_BUILD( __bit,__val )    (__val<<__bit)

#define LINK_MACRO(__pr,__val)        __pr##__val
#define apBIT_GET(val,MASK_BIT)         bit_get_vaile(val, LINK_MACRO(bs, MASK_BIT),LINK_MACRO(bw, MASK_BIT))
void apDMA_Channel_defultWidth(int width);
#ifndef DMA_NO_PRIVATE_FUNCTION_PROTOTYPES

/* ===================== Private Functions ================ */


/* ---------------------------------------------------------------------------
 * Description:
 *  Read the fixed config info from the DMA controller and
 *  save it in the specified Ctrlr Record
 *
 * Remarks:
 *
 * Inputs:
 *  pCtrlrRecord    -   Ptr to the record of the DMA controller being
 *                      intialised
 *
 * Outputs:
 *  Updates the sCtrlrs[] record as appropriate.
 *
 * Return Value:
 *  none
 *
 */

static void DMA_ReadControllerFixedData( DMA_sStateStruct *pCtrlrRecord );


/* ---------------------------------------------------------------------------
 * Description:
 *  Configures the specified DMA controller.
 *
 * Remarks:
 *  The DMA driver supports multiple DMA controllers, each has to be
 *  individually configured. The configuration data is taken from the
 *  supplied Ctrlr Record.
 *
 * Inputs:
 *  pCtrlrRecord    -   Ptr to the record of the DMA controller being
 *                      intialised
 *
 * Outputs:
 *  Updates the sCtrlrs[] record as appropriate.
 *
 * Return Value:
 *  None
 */

static void DMA_ConfigureController( DMA_sStateStruct *pCtrlrRecord );



/* ---------------------------------------------------------------------------
 * Description:
 *  Initialise the specified DMA controller & its channels
 *
 * Remarks:
 *  The DMA driver supports multiple DMA controllers, each has to be
 *  individually initialised.
 *
 * Inputs:
 *  pCtrlrRecord    -   Ptr to the record of the DMA controller being
 *                      intialised
 *
 * Outputs:
 *  none
 *
 * Return Value:
 *  none
 *
 */

static void DMA_InitializeController( DMA_sStateStruct *pCtrlrRecord );



/* ---------------------------------------------------------------------------
 * Description:
 *  Checks that all the fields in the specified apDMA_sXferCharacteristics
 *  are valid for the specified Ctrlr
 *
 * Remarks:
 *  Does NOT check that the specified ctrlr has been initialised
 *
 * Inputs:
 *  pCtrlrRecord                - Ptr to the Ctrlr record
 *  apDMA_sXferCharacteristics  - Ptr to mem xfer characterisitics to check
 *
 * Outputs:
 *  none
 *  
 *
 * Return Value:
 *  TRUE if all fields valid else FALSE.
 *
 */

static BOOL DMA_ValidMemAccessDetails( DMA_sStateStruct            *pCtrlrRecord,
                                        apDMA_sXferCharacteristics  *pXferInfo );

/* ---------------------------------------------------------------------------
 * Description:
 *  Get the access details & physical address for the specified virtual
 *  address
 *
 * Remarks:
 *  Called by API functions.
 *
 * Inputs:
 *  pCtrlr          - Ptr to the Controller record
 *  pVirtualAddress - The virtual address
 *
 * Outputs:
 *  pXferInfo       - Writes the Transfer access details to this structure.
 *
 * Return Value:
 *  none
 */

static void DMA_GetAddressAccessDetails( DMA_sStateStruct *pCtrlr,
                                          void             *pVirtualAddress,
                                          DMA_sXferInfo    *pXferInfo );

/* ---------------------------------------------------------------------------
 * Description:
 *  Converts the supplied data into a word ready
 *  for writing to the channel control register
 *
 * Remarks:
 *  Called by API functions
 *
 * Inputs:
 *  NumTransfers    - Number of transfers to perform
 *  pSrcXferInfo    - Ptr to details on the soruce
 *  pDstXferInfo    - Ptr to details on the dest
 *  Protection      - How to set the protection bits
 *  TCIntFlag       - Flag indicating whether the TC interrupt should be set
 *
 * Outputs:
 *  none
 *
 * Return Value:
 *  none
 */

static uint32_t DMA_CreateChanControlReg( uint32_t               NumTransfers,
                                          DMA_sXferInfo         *pSrcXferInfo,
                                          DMA_sXferInfo         *pDstXferInfo,
                                          uint32_t               Protection,
                                          DMA_eTCInterruptFlag  eTCIntFlag );

/* ---------------------------------------------------------------------------
 * Description:
 *  Used internally by API to initiate a periph to periph transfer
 *
 * Remarks:
 *
 * Inputs:
 *  pCtrlrRecord        - Ptr to the ctrlr record
 *  ChanId              - The channel Id
 *
 * Outputs:
 *  none
 *
 * Return Value:
 *  none
 */

static void DMA_PeriphToPeriphTransfer( DMA_sStateStruct *pCtrlrRecord,
                                            uint32_t          ChanId );

/* ---------------------------------------------------------------------------
 * Description:
 *  Used internally by API to get call back details 
 *  from an internal periph record when DMA transfer is terminated or when
 *  a LLI completes.
 *
 * Remarks:
 *  The oId should be valid, but the PeriphId need not as the function
 *  checks it before use.
 *  If the terminated flag is set, then the peripheral record is marked as
 *  idle.
 *
 * Inputs:
 *  pCtrlrRecord        - Ptr to the ctrlr record
 *  PeriphId            - The internal periph Id
 *  ChanId              - The channel Id that the peripheral should be using
 *  Terminated          - Flag that indicates if the transfer has terminated
 *
 * Outputs:
 *  pUsersId            - Users own Id
 *  pTerminatedCb       - Users terminated call back fn
 *
 * Return Value:
 *  none
 */

static void DMA_PeripheralTransferTerminating( 
                            DMA_sStateStruct           *pCtrlrRecord,
                            uint32_t                    PeriphId,
                            uint32_t                    ChanId,
                            BOOL                       Terminated,
                            uint32_t                    *pUsersId,
                            apDMA_rTransferTerminated  *pTerminatedCb );

/* ---------------------------------------------------------------------------
 * Description:
 *  Used internally by API - traverses the specified Linked List Items
 *  filling in the fields appropriately, and returns TRUE if no errors
 *  were encountered
 *
 * Remarks:
 *
 * Inputs:
 *  pCtrlrRecord        - Ptr to the ctrlr record
 *  pRawLLI             - Ptr to apDMA_sRawLLI struct to fill
 *  pSrcXferInfo        - Ptr to info about src data
 *  pDstXferInfo        - Ptr to info about dest data
 *  pRawNext            - Ptr to next LLI
 *  NumTransfers        - Num transfers for this LLI
 *  Protection          - How to set protection
 *  CbPerItem           - If TRUE then user's 'transfer terminated' call back
 *                        function will be called as each linked list
 *                        item is completed. If set to FALSE then the
 *                        user's 'transfer terminated' call back will only
 *                        be called when all linked list items have been
 *                        completed (or an error has occured).
 *
 * Outputs:
 *  none
 *
 * Return Value:
 *  none
 */

static void DMA_FillInLLI( DMA_sStateStruct  *pCtrlrRecord,
                            apDMA_sRawLLI     *pRawLLI,
                            DMA_sXferInfo     *pSrcXferInfo,
                            DMA_sXferInfo     *pDstXferInfo,
                            void              *pRawNext,
                            uint32_t           NumTransfers,
                            uint32_t           Protection,
                            BOOL              CbPerItem );

/* ---------------------------------------------------------------------------
 * Description:
 *  Used internally by API - traverses the specified Linked List Items
 *  filling in the raw fields appropriately, and returns TRUE if no errors
 *  were encountered
 *
 * Remarks:
 *
 * Inputs:
 *  pCtrlrRecord        - Ptr to the ctrlr record
 *  ChanId              - The channel Id
 *  NumLLItems          - The number of Linked List Items.
 *  pFirstLLI           - Pointer to the first API LLI
 *
 * Outputs:
 *  none
 *
 * Return Value:
 *  apERR_NONE        -   list traversed OK
 *  apERR_BAD_PARAMETER       -   some problem was encountered.
 */

static apError DMA_FillInMemMemLLIFields( DMA_sStateStruct *pCtrlrRecord,
                                        uint32_t          ChanId,
                                        uint32_t          NumLLItems, 
                                        apDMA_sMemMemLLI *pFirstLLI );

/* ---------------------------------------------------------------------------
 * Description:
 *  Used internally by API - traverses the specified Linked List Items
 *  filling in the fields appropriately, and returns TRUE if no errors
 *  were encountered
 *
 * Remarks:
 *
 * Inputs:
 *  pCtrlrRecord        - Pointer to the ctrlr record
 *  PeriphId            - The internal peripheral Id
 *  ChanId              - The channel Id
 *  NumLLItems          - The number of Linked List Items.
 *  pFirstLLI           - Pointer to the first API LLI
 *
 * Outputs:
 *  none
 *
 * Return Value:
 *  apERR_NONE        -   list traversed OK
 *  apERR_BAD_PARAMETER       -   some problem was encountered.
 */

static apError DMA_FillInMemPeriphLLIFields( DMA_sStateStruct *pCtrlrRecord,
                                           uint32_t          PeriphId,
                                           uint32_t          ChanId,
                                           uint32_t          NumLLItems, 
                                           apDMA_sPerMemLLI *pFirstLLI );


/* ---------------------------------------------------------------------------
 * Description:
 *  Used internally by API - copy the details from the channel & first
 *  LLI record to the DMA controller and start the transfer.
 *
 * Remarks:
 *
 * Inputs:
 *  pCtrlrRecord        - Ptr to the ctrlr record
 *  ChanId              - The channel Id
 *  pFirstLLI           - Ptr to the first API Raw LLI
 *
 * Outputs:
 *  none
 *
 * Return Value:
 *  none
 */

static void DMA_StartDMATransfer(   DMA_sStateStruct    *pCtrlrRecord,
                                     uint32_t             ChanId,
                                     apDMA_sRawLLI       *pFirstRawLLI );

/* ---------------------------------------------------------------------------
 * Description:
 *  Used within the DMA driver to initiate a Linked List memory to memory
 *  transfer requests.
 *
 * Remarks:
 *  At least one linked list item must be specified.
 *
 * Inputs:
 *  pCtrlrRecord        - Ptr to the ctrlr record
 *  ChanId              - The channel Id
 *  NumLLItems          - Number of Linked List Items (min 1)
 *  pFirstLLI           - Ptr to first Linked List Item
 *
 * Outputs:
 *  none
 *
 * Return Values:
 *  apERR_NONE                           - If the request has been accepted
 *  apERR_DMA_CTRLR_NOT_INITIALISED      - If the associated DMA controller has
 *                                         not been initialised
 *  apERR_BUSY                           - If the associated DMA channel is
 *                                         already in use
 *  apERR_BAD_PARAMETER                  - If any of the params are invalid
 *  apERR_DMA_PERIPHERAL_NOT_INITIALISED - If the specified internal
 *                                         peripheral record has not been
 *                                         configured
 *  apERR_DMA_CTRLR_DISABLED             - If the DMA controller is disabled
 */

static apError DMA_LinkedMemToMemTransfer( DMA_sStateStruct *pCtrlrRecord, 
                                            uint32_t          ChanId,
                                            uint32_t          NumLLItems,
                                            apDMA_sMemMemLLI *pFirstLLI );



/* ---------------------------------------------------------------------------
 * Description:
 *  Used within the DMA driver to initiate a Linked List memory / peripheral
 *  transfer.
 *
 * Remarks:
 *  At least one linked list item must be specified.
 *
 * Inputs:
 *  pCtrlrRecord        - Ptr to the ctrlr record
 *  PeriphId            - The internal peripheral Id
 *  ChanId              - The channel Id
 *  NumLLItems          - Number of Linked List Items (min 1)
 *  pFirstLLI           - Ptr to first Linked List Item
 *
 * Outputs:
 *  none
 *
 * Return Values:
 *  apERR_NONE                      - If the request has been accepted
 *  apERR_BAD_PARAMETER             - If any of the params are invalid
 */

static apError DMA_LinkedMemPeripheralTransfer( DMA_sStateStruct *pCtrlrRecord,
                                                 uint32_t          PeriphId,
                                                 uint32_t          ChanId,
                                                 uint32_t          NumLLItems,
                                                 apDMA_sPerMemLLI *pFirstLLI );

/* ---------------------------------------------------------------------------
 * Description:
 *  Checks the Controller Id is in range and that the Ctrlr has been
 *  intialised.
 *
 * Remarks:
 *  Called by all Low Level API functions & a few General fns
 *
 * Inputs:
 *  oId    - The Controller Id to validate.
 *
 * Outputs:
 *  pCtrlr      - Ptr to loc to write address of DMA controller record.
 *
 * Return Value:
 *  apERR_NONE                      - If all OK
 *  apERR_DMA_CTRLR_NOT_INITIALISED - If the DMA controller has
 *                                    not been initialised
 *  apERR_BAD_PARAMETER             - If any of the params are invalid
 *
 */

static apError DMA_GeneralEntry( 
                                  DMA_sStateStruct **pCtrlr );

/* ---------------------------------------------------------------------------
 * Description:
 *  Checks both the Controller Id and DMA channel are in range and that
 *  the Ctrlr has been intialised.
 *
 * Remarks:
 *  Called by all Low Level API functions.
 *
 * Inputs:
 *  oId    - The Controller Id to validate.
 *  DmaChannel  - The channel to validate
 *
 * Outputs:
 *  pCtrlr      - Ptr to loc to write address of DMA controller record.
 *
 * Return Value:
 *  apERR_NONE                      - If all OK
 *  apERR_DMA_CTRLR_NOT_INITIALISED - If the DMA controller has
 *                                    not been initialised
 *  apERR_BAD_PARAMETER             - If any of the params are invalid
 *  apERR_DMA_CTRLR_DISABLED        - If the DMA controller is disabled
 *
 */

static apError DMA_ValidateChannelEntry( 
                                          uint32_t          DmaChannel,
                                          DMA_sStateStruct **pCtrlr );


/* ---------------------------------------------------------------------------
 * Description:
 *  Used internally by Low Level API to initiate any type of memory / peripheral
 *  transfer.
 *
 * Remarks:
 *
 * Inputs:
 *  pCtrlrRecord    - Ptr to the ctrlr record
 *  DmaChannel      - Channel to use for transfer
 *  pParam          - Ptr to struct giving full details of the raw transfer
 *
 * Outputs:
 *  none
 *
 * Return Values:
 *  apERR_NONE                      - If the request has been accepted
 *  apERR_BAD_PARAMETER             - If any of the params are invalid
 */

static apError DMA_LowLevelTransfer( DMA_sStateStruct *pCtrlrRecord,
                                      uint32_t          DmaChannel,
                                      apDMA_sTransfer  *pParam );
                                      
                                      
/* ---------------------------------------------------------------------------
 * Description:
 *  Used internally by Low Level API - traverses the Linked List Items
 *  in the apDMA_sRawTransfer structure specified, filling in the raw 
 *  fields appropriately, and returns TRUE if no errors were encountered
 *
 * Remarks:
 *
 * Inputs:
 *  pCtrlrRecord    - Ptr to the ctrlr record
 *  pParam          - Ptr to struct giving full details of the raw transfer
 *
 * Outputs:
 *  none
 *
 * Return Value:
 *  apERR_NONE              -   list traversed OK
 *  apERR_BAD_PARAMETER     -   some problem was encountered.
 */

static apError DMA_FillInLowLevelLLIFields( DMA_sStateStruct  *pCtrlrRecord,
                                          apDMA_sTransfer   *pParam );



/* ---------------------------------------------------------------------------
 * Description:
 *  Called by the DMA Driver to terminate an ongoing DMA transfer or to
 *  notify the 'user' that a single LLI has completed.
 *
 * Remarks:
 *  Called by interrupt handler and other DMA driver functions. Can
 *  terminated any ongoing transfer irrespective of who initiated it.
 *
 *  If Terminated flag is set, then marks channel & associated peripheral
 *  records as idle
 *
 *  Passes suppled RetCode to the 'transfer terminated' call back functions
 *  of the 'user'
 *
 * Inputs:
 *  pCtrlrRecord        - Ptr to the ctrlr record
 *  ChanId              - The channel Id that the peripheral should be using
 *  Terminated          - Flag that indicates if the transfer has terminated
 *  RetCode             - The apError code to pass to the users 'transfer 
 *                        terminated' call back function.
 *  eCaller             - Type of the requestor:
 *                              (peripheral, application, low level API)
 *
 * Outputs:
 *  none
 *
 * Return Value:
 *  0              - if     transfer was a linked list or
 *                          invlaid DMA Peripheral Id or
 *                          DMA channel idle or
 *                          DMA controller wasn't the flow controller.
 *  num transfers  - Number of transfers performed.  
 */

static uint32_t DMA_TransferComplete( DMA_sStateStruct    *pCtrlrRecord,
                                      uint32_t             ChanId,
                                      BOOL                Terminated,
                                      apError             RetCode,
                                      DMA_eUser           eCaller );

#endif  /* ndef DMA_NO_PRIVATE_FUNCTION_PROTOTYPES */


#ifdef __cplusplus
} /* allow C++ to use these headers */
#endif /* __cplusplus */


#endif  /* End of this header file */

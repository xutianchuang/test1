#ifndef __IOGSMCU_H__
#define __IOGSMCU_H__
#ifdef __cplusplus
extern "C"
{
#endif
#include <gsmcuxx_hal_def.h>

#if ZB202
#define __FPU_PRESENT             0
#elif DM750
#define __FPU_PRESENT             1
#endif
#define __MPU_PRESENT             1
#define __NVIC_PRIO_BITS          4    
#define __Vendor_SysTickConfig    0   /*!< Set to 1 if different SysTick Config is used  */
  
typedef enum IRQn
{
/******  Cortex-M3 Processor Exceptions Numbers ***************************************************/
  NonMaskableInt_IRQn         = -14,    /*!< 2 Non Maskable Interrupt                             */
  MemoryManagement_IRQn       = -12,    /*!< 4 Cortex-M3 Memory Management Interrupt              */
  BusFault_IRQn               = -11,    /*!< 5 Cortex-M3 Bus Fault Interrupt                      */
  UsageFault_IRQn             = -10,    /*!< 6 Cortex-M3 Usage Fault Interrupt                    */
  SVCall_IRQn                 = -5,     /*!< 11 Cortex-M3 SV Call Interrupt                       */
  DebugMonitor_IRQn           = -4,     /*!< 12 Cortex-M3 Debug Monitor Interrupt                 */
  PendSV_IRQn                 = -2,     /*!< 14 Cortex-M3 Pend SV Interrupt                       */
  SysTick_IRQn                = -1,     /*!< 15 Cortex-M3 System Tick Interrupt                   */

/******  specific Interrupt Numbers *********************************************************/
  SYSC_IRQn                   = 0,
  WWDG_IRQn                   = 1,
  IWDG_IRQn                   = 2,
#if ZB202
  TMR1_1_IRQn                 = 3,
  TMR1_2_IRQn                 = 4,
  TMR1_3_IRQn                 = 5,
  TMR1_4_IRQn                 = 6,
  TMR1_5_IRQn                 = 7,
  TMR1_6_IRQn                 = 8,
  TMR0_1_IRQn                 = 9,
  TMR0_2_IRQn                 = 10,
  TMR0_3_IRQn                 = 11,
  TMR0_4_IRQn                 = 12,
  TMR0_5_IRQn                 = 13,
  TMR0_6_IRQn                 = 14,
#elif DM750
  TMR0_1_IRQn                 = 3,
  TMR0_2_IRQn                 = 4,
  TMR0_3_IRQn                 = 5,
  TMR1_1_IRQn                 = 6,
  TMR1_2_IRQn                 = 7,
  TMR1_3_IRQn                 = 8,
  TMR2_1_IRQn                 = 9,
  TMR2_2_IRQn                 = 10,
  TMR2_3_IRQn                 = 11,
  TMR2_4_IRQn                 = 12,
  TMR2_5_IRQn                 = 13,
  TMR2_6_IRQn                 = 14,  
#endif
  DMAC_IRQn                   = 15,
  DMAC_TC_IRQn                = 16,
  UART0_IRQn                  = 17,
  UART1_IRQn                  = 18,
  UART2_IRQn                  = 19,
  GPIO0_IRQn                  = 20,
  GPIO1_IRQn                  = 21,
  IIC0_IRQn                   = 22,
  UART3_IRQn                  = 23,
  IIC1_IRQn                   = 24,
  DMAC_ERR_IRQn               = 25,
  SSP0_IRQn                   = 26,
  SSP1_IRQn                   = 27,
  ADCC_IRQn                   = 28,
  SMS4_IRQn                   = 29,
  SPI0_IRQn                   = 30,
  BPLC_IRQn                   = 31,
  NTB_IRQn                    = 32,
  CRC_IRQn                    = 33,
  MAC_IRQn                    = 34,
  AES_IRQn                    = 35,
  CRY_IRQn                    = 36,
  SHA_IRQn                    = 37,
  EXTI0_IRQn			= 71,
  EXTI1_IRQn			= 72,
  EXTI2_IRQn			= 73,
  EXTI3_IRQn			= 74,
  EXTI4_IRQn			= 75,
  EXTI5_IRQn			= 76,
  EXTI6_IRQn			= 77,
  EXTI7_IRQn			= 78,
  EXTI8_IRQn			= 79,
  EXTI9_IRQn			= 80,
  EXTI10_IRQn			= 81,
  EXTI11_IRQn			= 82,
  EXTI12_IRQn			= 83,
  EXTI13_IRQn			= 84,
  EXTI14_IRQn			= 85,
  EXTI15_IRQn			= 86,
  EXTI16_IRQn			= 87,
  EXTI17_IRQn			= 88,
  EXTI18_IRQn			= 89,
  EXTI19_IRQn			= 90,
  EXTI20_IRQn			= 91,
  EXTI21_IRQn			= 92,
  EXTI22_IRQn			= 93,
  EXTI23_IRQn			= 94,
  EXTI24_IRQn			= 95
} IRQn_Type;

/**
  * @}
  */
#if ZB202
#include "core_cm3.h"
#elif DM750
#include "core_cm4.h"
#endif
#include <stdint.h>

/** @addtogroup Exported_types
  * @{
  */  

typedef int32_t  s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef const int32_t sc32;  /*!< Read Only */
typedef const int16_t sc16;  /*!< Read Only */
typedef const int8_t sc8;   /*!< Read Only */

typedef __IO int32_t  vs32;
typedef __IO int16_t  vs16;
typedef __IO int8_t   vs8;

typedef __I int32_t vsc32;  /*!< Read Only */
typedef __I int16_t vsc16;  /*!< Read Only */
typedef __I int8_t vsc8;   /*!< Read Only */

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef const uint32_t uc32;  /*!< Read Only */
typedef const uint16_t uc16;  /*!< Read Only */
typedef const uint8_t uc8;   /*!< Read Only */

typedef __IO uint32_t  vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t  vu8;

typedef __I uint32_t vuc32;  /*!< Read Only */
typedef __I uint16_t vuc16;  /*!< Read Only */
typedef __I uint8_t vuc8;   /*!< Read Only */

//typedef enum {RESET = 0, SET = !RESET} FlagStatus, ITStatus;

//typedef enum {DISABLE = 0, ENABLE = !DISABLE} FunctionalState;

#define IS_FUNCTIONAL_STATE(STATE) (((STATE) == DISABLE) || ((STATE) == ENABLE))

//typedef enum {ERROR = 0, SUCCESS = !ERROR} ErrorStatus;


//;               NVIC

//;               SCU
typedef struct
{
  __IO uint32_t BTUP_STS;
  __IO uint32_t BTUP_CTRL;
  __IO uint32_t PWR_CTRL;
  __IO uint32_t PWRUP_SEQ;
  __I  uint32_t CHIPID;
  __I  uint32_t VERID;
  __I  uint32_t STRAP;
  __IO uint32_t OSC_CTRL;
  __IO uint32_t PWR_MOD;
  __IO uint32_t INT_STS;
  __IO uint32_t INT_EN;
  __IO uint32_t SWRST_CTRL;
  __IO uint32_t PLL_CTRL;
  __I  uint32_t  RESERVED2[7];
  __IO uint32_t AHBCLKG;
  __I  uint32_t  RESERVED3[1];
  __IO uint32_t SLP_AHBCLKG;
  __I  uint32_t  RESERVED4[1];
  __IO uint32_t APBCLKG0;
  __IO uint32_t APBCLKG1;
  __IO uint32_t SLP_APBCLKG0;
  __IO uint32_t SLP_APBCLKG1;
  __I  uint32_t  RESERVED5[20];
  __IO uint32_t SLP_WAKUP_ST;
  __IO uint32_t SLP_WAKUP_EN;
  __I  uint32_t  RESERVED6[462];
  __IO uint32_t EX_RESET_MASK0;//
  __IO uint32_t EX_RESET_MASK1;
  __IO uint32_t EX_SOFTRESET_MASK0;//Software RESET Mask 
  __IO uint32_t EX_SOFTRESET_MASK1;
  
  __IO uint32_t EX_PERIPH_DRIVER_USART0_MODE;
  __IO uint32_t EX_PERIPH_PIN_REMAP0;
  __IO uint32_t EX_PERIPH_PIN_REMAP1;
  __IO uint32_t RESERVED7[1];
  
  __IO uint32_t EX_GPIO_DRIVER_OTHER;
  __IO uint32_t EX_PERIPH_OTHER;
  __IO uint32_t EX_MPU_CORTEX_OTHER;
  __IO uint32_t EX_PERIPH_INPUT_ENABLE;
  
  __IO uint32_t EX_ISRAM_MODE;
  __IO uint32_t RESERVED8[1];
  __IO uint32_t RESERVED9[1];
  __IO uint32_t EX_USART_MODE;
  
  __IO uint32_t EX_PLL_VALUE0;
  __IO uint32_t EX_PLL_VALUE1;
  __IO uint32_t EX_PLL_VALUE2;
  __IO uint32_t EX_PLL_VALUE3;
  
  __IO uint32_t EX_GPIO0_INPUT_ENABLE;
  __IO uint32_t EX_GPIO1_INPUT_ENABLE;
}SCU_TypeDef;
//;               FTSPI
typedef struct
{
  __IO uint32_t CMD0;
  __IO uint32_t CMD1;
  __IO uint32_t CMD2;
  __IO uint32_t CMD3;
  __IO uint32_t CR;
  __IO uint32_t ACTR;
  __I  uint32_t SR;
  __I  uint32_t  RESERVED0[1];
  __IO uint32_t ICR;
  __IO uint32_t ISR;
  __IO uint32_t SPISR;
  __IO uint32_t SPIFSR;
  __IO uint32_t XIPCMD;
  __IO uint32_t XIPPFO;
  __I  uint32_t  RESERVED1[6];
  __I  uint32_t REVISION;
  __I  uint32_t FEATURE;
  __I  uint32_t  RESERVED2[42];
  __IO uint32_t DR;
}FTSPI_TypeDef;
//;               DMAC
typedef struct
{
  __I  uint32_t INTSR;
  __I  uint32_t INTTC;
  __IO uint32_t INTTC_CLR;
  __I  uint32_t INT_ERR_ABT;
  __IO uint32_t INT_ERR_ABT_CLR;
  __I  uint32_t TCSR;
  __I  uint32_t INT_ERR_ABT_SR;
  __I  uint32_t CHEN;
  __I  uint32_t CH_BUSY;
  __IO uint32_t CSR;
  __IO uint32_t SYNC;
  __I  uint32_t  RESERVED0[1];
  __I  uint32_t REVISION;
  __I  uint32_t FEATURE;
  __I  uint32_t  RESERVED1[50];
  __IO uint32_t C0_CSR;
  __IO uint32_t C0_CFG;
  __IO uint32_t C0_SrcAdd;
  __IO uint32_t C0_DstAddr;
  __IO uint32_t C0_LLP;
  __IO uint32_t C0_SIZE;
  __I  uint32_t  RESERVED2[2];
  __IO uint32_t C1_CSR;
  __IO uint32_t C1_CFG;
  __IO uint32_t C1_SrcAdd;
  __IO uint32_t C1_DstAddr;
  __IO uint32_t C1_LLP;
  __IO uint32_t C1_SIZE;
  __I  uint32_t  RESERVED3[2];
  __IO uint32_t C2_CSR;
  __IO uint32_t C2_CFG;
  __IO uint32_t C2_SrcAdd;
  __IO uint32_t C2_DstAddr;
  __IO uint32_t C2_LLP;
  __IO uint32_t C2_SIZE;
  __I  uint32_t  RESERVED4[2];
  __IO uint32_t C3_CSR;
  __IO uint32_t C3_CFG;
  __IO uint32_t C3_SrcAdd;
  __IO uint32_t C3_DstAddr;
  __IO uint32_t C3_LLP;
  __IO uint32_t C3_SIZE;
  __I  uint32_t  RESERVED5[2];
  __IO uint32_t C4_CSR;
  __IO uint32_t C4_CFG;
  __IO uint32_t C4_SrcAdd;
  __IO uint32_t C4_DstAddr;
  __IO uint32_t C4_LLP;
  __IO uint32_t C4_SIZE;
  __I  uint32_t  RESERVED6[2];
  __IO uint32_t C5_CSR;
  __IO uint32_t C5_CFG;
  __IO uint32_t C5_SrcAdd;
  __IO uint32_t C5_DstAddr;
  __IO uint32_t C5_LLP;
  __IO uint32_t C5_SIZE;
  __I  uint32_t  RESERVED7[2];
  __IO uint32_t C6_CSR;
  __IO uint32_t C6_CFG;
  __IO uint32_t C6_SrcAdd;
  __IO uint32_t C6_DstAddr;
  __IO uint32_t C6_LLP;
  __IO uint32_t C6_SIZE;
  __I  uint32_t  RESERVED8[2];
  __IO uint32_t C7_CSR;
  __IO uint32_t C7_CFG;
  __IO uint32_t C7_SrcAdd;
  __IO uint32_t C7_DstAddr;
  __IO uint32_t C7_LLP;
  __IO uint32_t C7_SIZE;
}DMA_TypeDef;

typedef struct
{
  __IO uint32_t CSR;
  __IO uint32_t CFG;
  __IO uint32_t SrcAdd;
  __IO uint32_t DstAddr;
  __IO uint32_t LLP;
  __IO uint32_t SIZE;
}DMA_Channel_TypeDef;


//;               UART
typedef struct
{
  __IO uint32_t RTBR_DLL;
  __IO uint32_t IER_DLM;
  __IO uint32_t IIR_FCR_PSR;
  __IO uint32_t LCR;
  __IO uint32_t MCR;
  __IO uint32_t LSR_TST;
  __I  uint32_t MSR;
  __IO uint32_t SPR;
  __IO uint32_t MDR;
  __IO uint32_t ACR;
  __IO uint32_t TXLENL;
  __IO uint32_t TXLENH;
  __IO uint32_t MRXLENL;
  __IO uint32_t MRXLENH;
  __IO uint32_t PLR;
  __I  uint32_t FMIIR_PIO_DMA;
  __IO uint32_t FMIIER_PIO;
  __IO uint32_t FMIIER_DMA;
  __I  uint32_t STFF_STS;
  __I  uint32_t STFF_RXLENL;
  __I  uint32_t STFF_RXLENH;
  __I  uint32_t FMLSR;
  __IO uint32_t FMLSIER;
  __I  uint32_t  RESERVED0[1];
  __I  uint32_t RXFF_CNTR;
  __IO uint32_t LSTFMLENL;
  __IO uint32_t LSTFMLENH;
  __I  uint32_t FEATURE;
  __I  uint32_t REVD1;
  __I  uint32_t REVD2;
  __I  uint32_t REVD3;
}USART_TypeDef;
//;               SSP
typedef struct
{
  __IO uint32_t SSPCR0;
  __IO uint32_t SSPCR1;
  __IO uint32_t SSPCR2;
  __I  uint32_t SSPStatus;
  __IO uint32_t IntrCR;
  __I  uint32_t IntrStatus;
  __IO uint32_t TxRxDR;
  __IO uint32_t SSPCR3;
  __IO uint32_t ACLSVR;
  __IO uint32_t SPDIFStatus0;
  __IO uint32_t SPDIFStatus1;
  __IO uint32_t SPDIFUser0;
  __IO uint32_t SPDIFUser1;
  __IO uint32_t SPDIFUser2;
  __IO uint32_t SPDIFUser3;
  __IO uint32_t SPDIFUser4;
  __IO uint32_t SPDIFUser5;
  __IO uint32_t SPDIFUser6;
  __IO uint32_t SPDIFUser7;
  __IO uint32_t SPDIFUser8;
  __IO uint32_t SPDIFUser9;
  __IO uint32_t SPDIFUser10;
  __IO uint32_t SPDIFUser11;
  __I  uint32_t  RESERVED0[1];
  __I  uint32_t SSPRevision;
  __I  uint32_t SSPFeature;
}SSP_TypeDef;
//;               IIC
typedef struct
{
  __IO uint32_t CR;
  __I  uint32_t SR;
  __IO uint32_t CDR;
  __IO uint32_t DR;
  __IO uint32_t SAR;
  __IO uint32_t TGSR;
  __I  uint32_t BMR;
  __IO uint32_t SMCR;
  __IO uint32_t MAXTR;
  __IO uint32_t MINTR;
  __IO uint32_t METR;
  __IO uint32_t SETR;
  __IO uint32_t VERSION;
  __IO uint32_t FEATURE;
}IIC_TypeDef;
//;               TMR
typedef struct
{
  __IO uint32_t INT_CSTAT;
  __I  uint32_t  RESERVED0[3];
  __IO uint32_t TM1_CTRL;
  __IO uint32_t TM1_CNTB;
  __IO uint32_t TM1_CMPB;
  __I  uint32_t TM1_CNTO;
  __IO uint32_t TM2_CTRL;
  __IO uint32_t TM2_CNTB;
  __IO uint32_t TM2_CMPB;
  __I  uint32_t TM2_CNTO;
  __IO uint32_t TM3_CTRL;
  __IO uint32_t TM3_CNTB;
  __IO uint32_t TM3_CMPB;
  __I  uint32_t TM3_CNTO;
  __IO uint32_t TM4_CTRL;
  __IO uint32_t TM4_CNTB;
  __IO uint32_t TM4_CMPB;
  __I  uint32_t TM4_CNTO;
  __IO uint32_t TM5_CTRL;
  __IO uint32_t TM5_CNTB;
  __IO uint32_t TM5_CMPB;
  __I  uint32_t TM5_CNTO;
  __IO uint32_t TM6_CTRL;
  __IO uint32_t TM6_CNTB;
  __IO uint32_t TM6_CMPB;
  __I  uint32_t TM6_CNTO;
  __IO uint32_t TM7_CTRL;
  __IO uint32_t TM7_CNTB;
  __IO uint32_t TM7_CMPB;
  __I  uint32_t TM7_CNTO;
  __IO uint32_t TM8_CTRL;
  __IO uint32_t TM8_CNTB;
  __IO uint32_t TM8_CMPB;
  __I  uint32_t TM8_CNTO;
  __I  uint32_t TMR_REV;
}TMR_TypeDef;

typedef struct
{
  __IO uint32_t CHL_CTRL;
  __IO uint32_t CHL_CNTB;
  __IO uint32_t CHL_CMPB;
  __I  uint32_t CHL_CNTO;
}TMR_Chl_TypeDef;

//;               watchdog
typedef struct
{
  __IO uint32_t WdCounter;
  __IO uint32_t WdLoad;
  __IO uint32_t WdRestart;
  __IO uint32_t WdCR;
  __I  uint32_t WdStatus;
  __IO uint32_t WdClear;
  __IO uint32_t WdIntrlen;
  __I  uint32_t WdRevision;
}WDT_TypeDef;
//;              INDEPENDENT WATCHDOG TIMER
typedef struct
{
  __IO uint32_t KR;
  __IO uint32_t PR;
  __IO uint32_t RLR;
  __I  uint32_t SR;
  __IO uint32_t INTSR;
  __IO uint32_t CR;
  __IO uint32_t INTRLEN;
  __I  uint32_t REVISION;
}IWDT_TypeDef;
/* =========================================================================================================================== */
/* ================                                            SWT                                            ================ */
/* =========================================================================================================================== */


/**
  * @brief Software Watchdog Timer (SWT)
  */

typedef struct {                                /*!< (@ 0x40080000) SWT Structure                                              */
  
  union {
    __IOM uint32_t SWTCR;                       /*!< (@ 0x00000000) SWT Control Register                                       */
    
    struct {
      __IOM uint32_t WEN        : 1;            /*!< [0..0] Watchdog Enable Bit                                                */
      __IOM uint32_t FRZ        : 1;            /*!< [1..1] Watchdog Change Update Bit                                         */
      __IOM uint32_t STP        : 1;            /*!< [2..2] Watchdog Change Update Bit                                         */
      __IOM uint32_t CSL        : 1;            /*!< [3..3] Watchdog Interrupt Flag Bit                                        */
      __IOM uint32_t SLK        : 1;            /*!< [4..4] Watchdog Timer Prescaler                                           */
      __IOM uint32_t HLK        : 1;            /*!< [5..5] Watchdog Clock Domain Interrupt Status Bit                         */
      __IOM uint32_t ITR        : 1;            /*!< [6..6] Debug Mode Bit                                                     */
      __IOM uint32_t WND        : 1;            /*!< [7..7] STOP Mode Bit                                                      */
      __IOM uint32_t RIA        : 1;            /*!< [8..8] Doze Mode Bit                                                      */
      __IOM uint32_t KEY        : 1;            /*!< [9..9] Wait Mode Bit                                                      */
      __IM  uint32_t            : 14;
      __IOM uint32_t MAP        : 8;            /*!< [31..24] Wait Mode Bit                                                    */
    } SWTCR_b;
  } ;
  
  union {
    __IOM uint32_t SWTIR;                       /*!< (@ 0x00000004) SWT Interrupt Register                                     */
    
    struct {
      __IOM uint32_t TIF        : 1;            /*!< [0..0] Timeout Interrupt Flag                                             */
    } SWTIR_b;
  } ;
  __IOM uint32_t  SWTTO;                        /*!< (@ 0x00000008) SWT Timeout Register                                       */
  __IOM uint32_t  SWTWN;                        /*!< (@ 0x0000000C) SWT Window Register                                        */
  
  union {
    __IOM uint32_t SWTSR;                       /*!< (@ 0x00000010) SWT Service Register                                       */
    
    struct {
      __IOM uint32_t WSC        : 16;           /*!< [15..0] Watchdog Service Code                                             */
    } SWTSR_b;
  } ;
  __IOM uint32_t  SWTCO;                        /*!< (@ 0x00000014) SWT Counter Output Register                                */
  
  union {
    __IOM uint32_t SWTSK;                       /*!< (@ 0x00000018) SWT Service Key Register                                   */
    
    struct {
      __IOM uint32_t SK         : 16;           /*!< [15..0] Service Key                                                       */
    } SWTSK_b;
  } ;
} SWT_Type;                                     /*!< Size = 28 (0x1c)                                                            */
//;              ADC
typedef struct
{
  __IO uint32_t DATACH0;
  __IO uint32_t DATACH1;
  __IO uint32_t DATACH2;
  __IO uint32_t DATACH3;
  __IO uint32_t DATACH4;
  __IO uint32_t DATACH5;
  __IO uint32_t DATACH6;
  __IO uint32_t DATACH7;
  __I  uint32_t  RESERVED0[24];
  __IO uint32_t THRHOLD0_1;
  __IO uint32_t THRHOLD2_3;
  __IO uint32_t THRHOLD4_5;
  __IO uint32_t THRHOLD6_7;
  __IO uint32_t THRHOLD8_9;
  __IO uint32_t THRHOLD10_11;
  __IO uint32_t THRHOLD12_13;
  __IO uint32_t THRHOLD14_15;
  __I  uint32_t  RESERVED1[8];
  __IO uint32_t TS_DATA;
  __IO uint32_t TS_THRHOLD;
  __I  uint32_t  RESERVED2[14];
  __IO uint32_t CTRL;
  __IO uint32_t MISC;
  __IO uint32_t INTEN;
  __IO uint32_t INTR;
  __IO uint32_t TPARM;
  __IO uint32_t SMPR;
  __IO uint32_t DISCHTIME;
  __IO uint32_t PRESCAL;
  __IO uint32_t SQR;
  __I  uint32_t RESERVED3[55];
  __I  uint32_t DMA0DAT;
  __IO uint32_t DMA0CTRL;
  __IO uint32_t DMA0INTEN;
  __IO uint32_t DMA0STS;
  __I  uint32_t DMA1DAT;
  __IO uint32_t DMA1CTRL;
  __IO uint32_t DMA1INTEN;
  __IO uint32_t DMA1STS;
  __I  uint32_t DMA2DAT;
  __IO uint32_t DMA2CTRL;
  __IO uint32_t DMA2INTEN;
  __IO uint32_t DMA2STS;
  __I  uint32_t DMA3DAT;
  __IO uint32_t DMA3CTRL;
  __IO uint32_t DMA3INTEN;
  __IO uint32_t DMA3STS;
}ADC_TypeDef;
//;               MAC
typedef struct
{
  __I  uint32_t ISR;
  __IO uint32_t IME;
  __IO uint32_t MADR;
  __IO uint32_t LADR;
  __IO uint32_t MAHT0;
  __IO uint32_t MAHT1;
  __IO uint32_t TXPD;
  __IO uint32_t RXPD;
  __IO uint32_t TXR_BADR;
  __IO uint32_t RXR_BADR;
  __IO uint32_t ITC;
  __IO uint32_t APTC;
  __IO uint32_t DBLAC;
  __I  uint32_t REVR;
  __I  uint32_t  RESERVED0[18];
  __IO uint32_t LPICR;
  __IO uint32_t MACCR;
  __IO uint32_t MACSR;
  __IO uint32_t PHYCR;
  __IO uint32_t PHYWDATA;
  __IO uint32_t FCR;
  __IO uint32_t BPR;
  __IO uint32_t WOLCR;
  __IO uint32_t WOLSR;
  __IO uint32_t WFCRC;
  __I  uint32_t  RESERVED1[1];
  __IO uint32_t WFBM1;
  __IO uint32_t WFBM2;
  __IO uint32_t WFBM3;
  __IO uint32_t WFBM4;
  __I  uint32_t  RESERVED2[1];
  __IO uint32_t TS;
  __I  uint32_t DMAFIFOS;
  __IO uint32_t TM;
  __I  uint32_t  RESERVED3[1];
  __I  uint32_t TX_CNT0;
  __I  uint32_t RX_CNT0;
  __I  uint32_t TX_CNT1;
  __I  uint32_t RX_CNT1;
  __I  uint32_t RX_CNT2;
  __I  uint32_t RX_CNT3;
  __I  uint32_t RX_CNT4;
  __I  uint32_t MULCA;
  __I  uint32_t RP;
  __I  uint32_t XP;
}MAC_TypeDef;



//;               GPIO
typedef struct
{
  __IO uint32_t DATAOUT;
  __I  uint32_t DATAIN;
  __IO uint32_t PINDIR;
  __IO uint32_t PINBYPASS;
  __IO uint32_t DATASET;
  __IO uint32_t DATACLEAR;
  __IO uint32_t PINPULL;
  __IO uint32_t PINPULL_TYPE;
  __IO uint32_t INTRENABLE;
  __I  uint32_t IRSR;
  __I  uint32_t IMSR;
  __IO uint32_t IMR;
  __IO uint32_t ICR;
  __IO uint32_t ITR;
  __IO uint32_t IBETR;
  __IO uint32_t IRONETR;
  __IO uint32_t BER;
  __IO uint32_t BCPR;
  __I  uint32_t  RESERVED0[13];
  __I  uint32_t REVISION;
}GPIO_TypeDef;

/**
  * @}
  */
  
/** @addtogroup Peripheral_memory_map
  * @{
  */

#define SRAM_BASE             ((uint32_t)0x10000000) /*!< SRAM base address in the alias region */
#define FLASH_BASE            ((uint32_t)0x00000000) /*!< FLASH base address in the alias region */
#define IROM_BASE             ((uint32_t)0x13000000) /*!< FLASH base address in the alias region */
#define PERIPH_BASE           ((uint32_t)0x40000000) /*!< Peripheral base address in the alias region */

#define SRAM_BB_BASE          ((uint32_t)0x22000000) /*!< SRAM base address in the bit-band region */
#define PERIPH_BB_BASE        ((uint32_t)0x42000000) /*!< Peripheral base address in the bit-band region */


/*!< Peripheral memory map */
#define APB0PERIPH_BASE       	(PERIPH_BASE + 0x000000)
#define APB1PERIPH_BASE       	(PERIPH_BASE + 0x100000)
#define AHBPERIPH_BASE       	(PERIPH_BASE + 0x000000)

#define FTSPI_BASE	   	(AHBPERIPH_BASE + 0x200000)
#define DMA_BASE 	   	(AHBPERIPH_BASE + 0x300000)
#define MAC_BASE 	   	(AHBPERIPH_BASE + 0x700000)
//#define BPLC_BASE 	   	(AHBPERIPH_BASE + 0x500000)

#define SSP0_BASE	   	(APB0PERIPH_BASE + 0x0000)
#define UART0_BASE	   	(APB0PERIPH_BASE + 0x1000)
#define UART1_BASE	   	(APB0PERIPH_BASE + 0x2000)
#define IIC0_BASE 	   	(APB0PERIPH_BASE + 0x3000)
#define GPIO0_BASE	   	(APB0PERIPH_BASE + 0x4000)
#if ZB202
#define EFUSE64_BASE	(APB0PERIPH_BASE + 0x6000)
#define WDT_BASE 	   	(APB0PERIPH_BASE + 0x7000)
#define TMR0_BASE	   	(APB0PERIPH_BASE + 0x8000)
#define UART3_BASE	   	(APB0PERIPH_BASE + 0xa000)
#define EFUSE1024_BASE	(APB0PERIPH_BASE + 0x9000)
#elif DM750
#define TMR0_BASE	   	(APB0PERIPH_BASE + 0x5000)
#define EFUSE64_BASE	(APB0PERIPH_BASE + 0x6000)
#define WDT_BASE 	   	(APB0PERIPH_BASE + 0x7000)
#define TMR1_BASE	   	(APB0PERIPH_BASE + 0x8000)
#define EFUSE1024_BASE	(APB0PERIPH_BASE + 0x9000)
#define UART3_BASE	   	(APB0PERIPH_BASE + 0xA000)
#endif

#define SCU_BASE		(APB1PERIPH_BASE + 0x0000)
#define SSP1_BASE		(APB1PERIPH_BASE + 0x1000)
#define UART2_BASE		(APB1PERIPH_BASE + 0x2000)
#define IIC1_BASE		(APB1PERIPH_BASE + 0x3000)
#define GPIO1_BASE		(APB1PERIPH_BASE + 0x4000)
#define IWDT_BASE		(APB1PERIPH_BASE + 0x5000)
#if ZB202
#define TMR1_BASE		(APB1PERIPH_BASE + 0x6000)
#elif DM750
#define TMR2_BASE		(APB1PERIPH_BASE + 0x6000)
#endif
#define ADC_BASE		(APB1PERIPH_BASE + 0x7000)

//#define NVIC_BASE		(0xE000E000)
//
//#define NVIC                	((NVIC_TypeDef *) NVIC_BASE)


#define FTSPI			((FTSPI_TypeDef *) FTSPI_BASE)
#define DMA			    ((DMA_TypeDef *) DMA_BASE)
#define MAC			    ((MAC_TypeDef *) MAC_BASE)
//#define BPLC            ((BPLC_TypeDef *) BPLC_BASE)
//#define BPLC_S          ((BPLC_Type*)     BPLC_BASE)


#define SSP0			((SSP_TypeDef *) SSP0_BASE)
#define UART0			((USART_TypeDef *) UART0_BASE)
#define UART1			((USART_TypeDef *) UART1_BASE)
#define IIC0			((IIC_TypeDef *) IIC0_BASE)
#define GPIO0			((GPIO_TypeDef *) GPIO0_BASE)
#define TMR0			((TMR_TypeDef *) TMR0_BASE)
#define WDT			    ((WDT_TypeDef *) WDT_BASE)
#define TMR1			((TMR_TypeDef *) TMR1_BASE)
#define UART3			((USART_TypeDef *) UART3_BASE)

#define SCU			    ((SCU_TypeDef *) SCU_BASE)
#define SSP1			((SSP_TypeDef *) SSP1_BASE)
#define UART2			((USART_TypeDef *) UART2_BASE)
#define IIC1			((IIC_TypeDef *) IIC1_BASE)
#define GPIO1			((GPIO_TypeDef *) GPIO1_BASE)
#define IWDT			((IWDT_TypeDef *) IWDT_BASE)
#define SWT				((SWT_Type *) IWDT_BASE)//=========
#define TMR2			((TMR_TypeDef *) TMR2_BASE)
#define ADC			    ((ADC_TypeDef *) ADC_BASE)


/** @addtogroup Exported_macro
  * @{
  */

#define SET_BIT(REG, BIT)     ((REG) |= (BIT))

#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))

#define READ_BIT(REG, BIT)    ((REG) & (BIT))

#define CLEAR_REG(REG)        ((REG) = (0x0))

#define WRITE_REG(REG, VAL)   ((REG) = (VAL))

#define READ_REG(REG)         ((REG))

#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

/**
  * @}
  */



#ifdef __cplusplus
}
#endif
#endif /* __IOGSMCU_H__ */


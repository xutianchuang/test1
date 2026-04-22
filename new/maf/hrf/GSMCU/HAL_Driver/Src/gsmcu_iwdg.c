#include  <gsmcu_hal.h>
/* =========================================================================================================================== */
/* ================                                            SWT                                            ================ */
/* =========================================================================================================================== */

/* =========================================================  SWTCR  ========================================================= */
#define SWT_SWTCR_WEN_Pos                 (0UL)                     /*!< SWT SWTCR: WEN (Bit 0)                                */
#define SWT_SWTCR_WEN_Msk                 (0x1UL)                   /*!< SWT SWTCR: WEN (Bitfield-Mask: 0x01)                  */
#define SWT_SWTCR_FRZ_Pos                 (1UL)                     /*!< SWT SWTCR: FRZ (Bit 1)                                */
#define SWT_SWTCR_FRZ_Msk                 (0x2UL)                   /*!< SWT SWTCR: FRZ (Bitfield-Mask: 0x01)                  */
#define SWT_SWTCR_STP_Pos                 (2UL)                     /*!< SWT SWTCR: STP (Bit 2)                                */
#define SWT_SWTCR_STP_Msk                 (0x4UL)                   /*!< SWT SWTCR: STP (Bitfield-Mask: 0x01)                  */
#define SWT_SWTCR_CSL_Pos                 (3UL)                     /*!< SWT SWTCR: CSL (Bit 3)                                */
#define SWT_SWTCR_CSL_Msk                 (0x8UL)                   /*!< SWT SWTCR: CSL (Bitfield-Mask: 0x01)                  */
#define SWT_SWTCR_SLK_Pos                 (4UL)                     /*!< SWT SWTCR: SLK (Bit 4)                                */
#define SWT_SWTCR_SLK_Msk                 (0x10UL)                  /*!< SWT SWTCR: SLK (Bitfield-Mask: 0x01)                  */
#define SWT_SWTCR_HLK_Pos                 (5UL)                     /*!< SWT SWTCR: HLK (Bit 5)                                */
#define SWT_SWTCR_HLK_Msk                 (0x20UL)                  /*!< SWT SWTCR: HLK (Bitfield-Mask: 0x01)                  */
#define SWT_SWTCR_ITR_Pos                 (6UL)                     /*!< SWT SWTCR: ITR (Bit 6)                                */
#define SWT_SWTCR_ITR_Msk                 (0x40UL)                  /*!< SWT SWTCR: ITR (Bitfield-Mask: 0x01)                  */
#define SWT_SWTCR_WND_Pos                 (7UL)                     /*!< SWT SWTCR: WND (Bit 7)                                */
#define SWT_SWTCR_WND_Msk                 (0x80UL)                  /*!< SWT SWTCR: WND (Bitfield-Mask: 0x01)                  */
#define SWT_SWTCR_RIA_Pos                 (8UL)                     /*!< SWT SWTCR: RIA (Bit 8)                                */
#define SWT_SWTCR_RIA_Msk                 (0x100UL)                 /*!< SWT SWTCR: RIA (Bitfield-Mask: 0x01)                  */
#define SWT_SWTCR_KEY_Pos                 (9UL)                     /*!< SWT SWTCR: KEY (Bit 9)                                */
#define SWT_SWTCR_KEY_Msk                 (0x200UL)                 /*!< SWT SWTCR: KEY (Bitfield-Mask: 0x01)                  */
#define SWT_SWTCR_MAP_Pos                 (24UL)                    /*!< SWT SWTCR: MAP (Bit 24)                               */
#define SWT_SWTCR_MAP_Msk                 (0xff000000UL)            /*!< SWT SWTCR: MAP (Bitfield-Mask: 0xff)                  */
/* =========================================================  SWTIR  ========================================================= */
#define SWT_SWTIR_TIF_Pos                 (0UL)                     /*!< SWT SWTIR: TIF (Bit 0)                                */
#define SWT_SWTIR_TIF_Msk                 (0x1UL)                   /*!< SWT SWTIR: TIF (Bitfield-Mask: 0x01)                  */
/* =========================================================  SWTTO  ========================================================= */
/* =========================================================  SWTWN  ========================================================= */
/* =========================================================  SWTSR  ========================================================= */
#define SWT_SWTSR_WSC_Pos                 (0UL)                     /*!< SWT SWTSR: WSC (Bit 0)                                */
#define SWT_SWTSR_WSC_Msk                 (0xffffUL)                /*!< SWT SWTSR: WSC (Bitfield-Mask: 0xffff)                */
/* =========================================================  SWTCO  ========================================================= */
/* =========================================================  SWTSK  ========================================================= */
#define SWT_SWTSK_SK_Pos                  (0UL)                     /*!< SWT SWTSK: SK (Bit 0)                                 */
#define SWT_SWTSK_SK_Msk                  (0xffffUL)                /*!< SWT SWTSK: SK (Bitfield-Mask: 0xffff)                 */


void WDG_Initialize(void)
{

}

void WDG_SetWindowMode(void)
{
    SWT->SWTCR_b.WND = 1;   //窗口模式
}

void WDG_SetTimeOutMode(void)
{
    SWT->SWTCR_b.WND = 0;   //窗口模式
}

void Feed_WDG(void)
{
    if(SWT->SWTCR_b.KEY) 
    {
        SWT->SWTSR = 17 * SWT->SWTSK+3; //伪随机数
        SWT->SWTSR = 17 * SWT->SWTSK+4; //伪随机数
    }
    else
    {
        SWT->SWTSR = 0xA602;            //固定序列
		SWT->SWTSR = 0xB480;
    }
}

void SWT_IRQHandler(void)
{
    if(SWT->SWTIR & SWT_SWTIR_TIF_Msk)
    {
        SWT->SWTIR = SWT_SWTIR_TIF_Msk;
        Feed_WDG();
    }
}

//Lock,SWT_CR, SWT_TO and SWT_WN are read only registers
void WDG_HardLock(void)
{
    SWT->SWTCR_b.HLK = 1;
}

void WDG_HardUnLock(void)
{
    //仅Reset可以解锁
}

//Lock,SWT_CR, SWT_TO and SWT_WN are read only registers
void WDG_SoftLock(void)
{
    SWT->SWTCR_b.SLK=1;
}

void WDG_SoftUnLock(void)
{
    SWT->SWTSR = 0xC520;
	SWT->SWTSR = 0xD928;
}

//void WDG_SetWindowUs(uint32_t us)
//{
//    uint32_t value;
//    value = Get_SWT_Clock_us();
//    value = us/value;
//	
//	SWT->SWTWN = value;
//}
//
//void WDG_SetTimeOutUs(uint32_t timeout)
//{
//    uint32_t value;
//	value = Get_SWT_Clock_us();
//	value = timeout/value;
//	
//	SWT->SWTTO = value;
//	SWT->SWTWN = value-0x1;
//}




void WDG_Enable(void)
{
    SWT->SWTCR_b.WEN = 1;
    SWT->SWTCR_b.ITR = 0;
}
void WDG_EnInterrupt(void)
{
    SWT->SWTCR_b.ITR = 1;
}
void WDG_Disable(void)
{
    SWT->SWTCR_b.WEN = 0;
}
void WDG_ShortReset(void)
{
    WDG_SoftUnLock();
    SWT->SWTTO=3000;
    WDG_SoftLock();
    Feed_WDG();
    
}
//void WDG_Demo(void)
//{
//    SWT->SWTCR_b.ITR = 1;
//    NVIC_EnableIRQ((IRQn_Type)SWT_IRQn);
//    WDG_Enable();
//    while(1);
//}


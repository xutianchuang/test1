/*
*
*/
#include  <gsmcu_hal.h>
#include "bsp_wdg.h"
#include "os.h"
#include "common_includes.h"
/*
*
*/
void InitWdg(void)
{
#if defined(INTER_WATCH_DOG)
	WDG_Enable();
	WDG_EnInterrupt();//今后中断长开
	NVIC_EnableIRQ(IWDG_IRQn);
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

void PLLRangeCheck(void)
{
  uint32_t PLL_CFG0,PLL_CFG1,PLL_CFG2;
  uint32_t pll_frac,pll_integer;
  if(SCU->PLL_CTRL != 0x00000010)
  {
    SAFE_SYSTEM_RESET(SYS_RESET_POS_0);
  }
  PLL_CFG0 = REG_32BIT(SCU_BASE, 0x840);
  if(PLL_CFG0 != 0x0)
  {
    SAFE_SYSTEM_RESET(SYS_RESET_POS_0);
  }
  PLL_CFG1 = REG_32BIT(SCU_BASE, 0x844);
  pll_integer = PLL_CFG1&0xFFF;
  if((PLL_CFG1 & 0xFFFF0000) != 0x11180000)
  {
   SAFE_SYSTEM_RESET(SYS_RESET_POS_0);
  }
  PLL_CFG2 = REG_32BIT(SCU_BASE, 0x848);
  pll_frac = PLL_CFG2&0xFFFFFF;
  if((PLL_CFG2 & 0xFF000000) != 0x01000000)
  {
    SAFE_SYSTEM_RESET(SYS_RESET_POS_0);
  }
  if(pll_integer < 63 || pll_integer > 64 || (pll_integer == 63 && pll_frac < 0xF00000) || (pll_integer == 64 && pll_frac > 0x100000))
  {
    SAFE_SYSTEM_RESET(SYS_RESET_POS_0);
  }
}
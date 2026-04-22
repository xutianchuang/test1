/**************************************************************************//**
 * @file     system_ARMCM0.c
 * @brief    CMSIS Device System Source File for
 *           ARMCM0 Device Series
 * @version  V2.00
 * @date     18. August 2015
 ******************************************************************************/
/* Copyright (c) 2011 - 2015 ARM LIMITED

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ARM nor the names of its contributors may be used
     to endorse or promote products derived from this software without
     specific prior written permission.
   *
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   ---------------------------------------------------------------------------*/


#include "GSMCU_M3.h"
#include "RTE_Components.h"
#include "gsmcu_m3_port.h"
#include "system_GSMCU_M3.h"

#include "RTE_Device.h"
#include "gsmcuxx_hal_def.h"
#include "protocol_includes.h"
/*----------------------------------------------------------------------------
  Define clocks
 *----------------------------------------------------------------------------*/
#define SYSTEM_USE_100M
#if defined(FPGA_ENV)
#define  XTAL            ( 50000000 )
#else
#define  XTAL            ( 100000000 )
#endif
#define  SYSTEM_CLOCK    ( XTAL )

#define AHB_CLOCK_GATE_AES     (1<<1)
#define AHB_CLOCK_GATE_CRC     (1<<2)
#define AHB_CLOCK_GATE_CRY     (1<<3)
#define AHB_CLOCK_GATE_PLC     (1<<4)
#define AHB_CLOCK_GATE_RAM0    (1<<5)
#define AHB_CLOCK_GATE_RAM1    (1<<6)
#if defined(ZB204_CHIP)
#define AHB_CLOCK_GATE_HRF     (1<<7)
#define AHB_CLOCK_GATE_PWM0    (1<<8)
#define AHB_CLOCK_GATE_PWM1    (1<<9)
#define AHB_CLOCK_GATE_QSPI    (1<<10)
#define AHB_CLOCK_GATE_DMA     (1<<11)
#define AHB_CLOCK_GATE_ROM     (1<<12)
#define AHB_CLOCK_GATE_SPIBUS  (1<<13)
#define AHB_CLOCK_GATE_PSRAM   (1<<14)
#elif defined(ZB205_CHIP)
#define AHB_CLOCK_GATE_RAM2    (1<<7)
#define AHB_CLOCK_GATE_RAM3    (1<<8)
#define AHB_CLOCK_GATE_HRF     (1<<9)
#define AHB_CLOCK_GATE_PWM0    (1<<10)
#define AHB_CLOCK_GATE_PWM1    (1<<11)
#define AHB_CLOCK_GATE_QSPI    (1<<12)
#define AHB_CLOCK_GATE_DMA     (1<<13)
#define AHB_CLOCK_GATE_ROM     (1<<14)
#define AHB_CLOCK_GATE_ADC     (1<<15)
#define AHB_CLOCK_GATE_PSRAM   (1<<16)
#endif
#define IPS_CLOCK_GATE_PORT   (1<<1)
#define IPS_CLOCK_GATE_QSPI   (1<<2)
#define IPS_CLOCK_GATE_PIT0   (1<<3)
#define IPS_CLOCK_GATE_PIT1   (1<<4)
#define IPS_CLOCK_GATE_EFUSE0 (1<<5)
#define IPS_CLOCK_GATE_EFUSE1 (1<<6)
#define IPS_CLOCK_GATE_IIC    (1<<7)
#define IPS_CLOCK_GATE_UART0  (1<<8)
#define IPS_CLOCK_GATE_UART1  (1<<9)
#define IPS_CLOCK_GATE_UART2  (1<<10)
#define IPS_CLOCK_GATE_UART3  (1<<11)
#define IPS_CLOCK_GATE_SPI0   (1<<12)
#define IPS_CLOCK_GATE_SPI1   (1<<13)
#define IPS_CLOCK_GATE_EPORT0 (1<<14)
#define IPS_CLOCK_GATE_EPORT1 (1<<15)
#define IPS_CLOCK_GATE_EPORT2 (1<<16)
#define IPS_CLOCK_GATE_EPORT3 (1<<17)
#define IPS_CLOCK_GATE_EPORT4 (1<<18)
#define IPS_CLOCK_GATE_EPORT5 (1<<19)
#define IPS_CLOCK_GATE_EPORT6 (1<<20)
#define IPS_CLOCK_GATE_EPORT7 (1<<21)
#define IPS_CLOCK_GATE_EPORT8 (1<<22)
#define IPS_CLOCK_GATE_EPORT9 (1<<23)
#define IPS_CLOCK_GATE_SPIBUS (1<<24)
#define IPS_CLOCK_GATE_PSRAM  (1<<25)
#define IPS_CLOCK_GATE_HPLC   (1<<26)
#define IPS_CLOCK_GATE_WDT    (1<<27)
#define IPS_CLOCK_GATE_DFE    (1<<28)
#define IPS_CLOCK_GATE_HRF    (1<<29)

/*----------------------------------------------------------------------------
  System Core Clock Variable
 *----------------------------------------------------------------------------*/
uint32_t SystemCoreClock = SYSTEM_CLOCK;  /* System Core Clock Frequency      */
uint32_t IpgClock=SYSTEM_CLOCK/2; 		  /* Ipg Clock Frequency */
uint32_t SysTickms=0;
//100 system 50M IPS
void PLL_default(void)
{
#if defined(ZB204_CHIP)
  CPM->PLL_CONFIG0_b.PLL_FBDIV = 0x40;
#elif defined(ZB205_CHIP)
  CPM->PLL_CONFIG0_b.PLL_FBDIV = 0x20;
#endif
  CPM->PLL_CONFIG1_b.PLL_FRAC = 0;
}

uint32_t trim_PLL(int32_t freq_diff, uint8_t reset)//*2^24
{ 
	uint64_t a;
	int b;
	int a_int;
	//uint32_t cur_reg_val;
	uint32_t pll_integer,pll_frac;
	
	if(reset != 1)
	{
		#if defined(ZB205_CHIP)
		CPM->PLL_CONFIG0_b.TEST = 1;
		CPM->PLL_CONFIG0_b.TEST = 2;
		CPM->PLL_CONFIG0_b.TEST = 3;
		#endif
		pll_integer = CPM->PLL_CONFIG0_b.PLL_FBDIV;
		#if defined(ZB205_CHIP)
		CPM->PLL_CONFIG0_b.TEST = 0;
		#endif
		pll_frac = CPM->PLL_CONFIG1_b.PLL_FRAC;
		a = ((uint64_t)pll_integer<<24);
		a_int = (int)pll_integer;
		a += (uint64_t)pll_frac;
		b = ((1<<24)+(a_int*freq_diff));
		a += ((uint64_t)b);
		a -= (1<<24);
		pll_frac = (uint32_t)(a & 0xffffff);
		pll_integer = (uint32_t)(a >> 24);
		#if defined(ZB204_CHIP)
		if(pll_integer < 63 || pll_integer > 64 || (pll_integer == 63 && pll_frac < 16401406) || (pll_integer == 64 && pll_frac > 375810))
		{
			pll_integer = 64;
			pll_frac = 0;
			return 1;//é”™è¯¯ç›´æŽ¥è¿”å›ž
		}
		#elif defined(ZB205_CHIP)
		if(pll_integer < 31 || pll_integer > 32 || (pll_integer == 31 && pll_frac < 16589311) || (pll_integer == 32 && pll_frac > 187905))
		{
			pll_integer = 32;
			pll_frac = 0;
			return 1;//é”™è¯¯ç›´æŽ¥è¿”å›ž
		}
		#endif
		
	}
	else
	{
		#if defined(ZB204_CHIP)
		pll_integer = 64;
		#elif defined(ZB205_CHIP)
		pll_integer = 32;
		#endif
		pll_frac = 0;
	}

	#if defined(ZB205_CHIP)
	CPM->PLL_CONFIG0_b.TEST = 1;
	CPM->PLL_CONFIG0_b.TEST = 2;
	CPM->PLL_CONFIG0_b.TEST = 3;
	CPM->PLL_CONFIG1_b.TEST = 1;
	CPM->PLL_CONFIG1_b.TEST = 2;
	CPM->PLL_CONFIG1_b.TEST = 3;
	#endif
	CPM->PLL_CONFIG0_b.PLL_FBDIV = pll_integer;
	CPM->PLL_CONFIG1_b.PLL_FRAC = pll_frac;
	#if defined(ZB205_CHIP)
	CPM->PLL_CONFIG0_b.TEST = 0;
	CPM->PLL_CONFIG1_b.TEST = 0;
	#endif
	__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
	// Wait for FRAC PLL lock
	//	while ((READ(SCU_BASE + 0x84c) & 0x80000000) != 0x80000000)
	//	{
	//	}
	//	
	//	SCU->PLL_CTRL = cur_reg_val;
	//	REG_32BIT(SCU_BASE, 0x20) = (REG_32BIT(SCU_BASE, 0x20)|0x40);
	//	__asm("wfi"); //FCS for cortex M3
	return 0;
}

#if defined(ZB204_CHIP)
void CloseClockGate(void)
{
	CPM->AHBMSCR = 
		AHB_CLOCK_GATE_AES
		|AHB_CLOCK_GATE_CRC
		|AHB_CLOCK_GATE_CRY
#ifndef __DEBUG_MODE
		|AHB_CLOCK_GATE_ROM
#endif
		|AHB_CLOCK_GATE_PSRAM;//ÅäÖÃClock gate
    CPM->MSCR = 
		IPS_CLOCK_GATE_EFUSE0|
		IPS_CLOCK_GATE_EFUSE1|
		IPS_CLOCK_GATE_IIC|
		IPS_CLOCK_GATE_PSRAM|
		IPS_CLOCK_GATE_SPI0|
		IPS_CLOCK_GATE_SPI1;
	CPM->MISCCLKCR_b.TEST =1;
	CPM->MISCCLKCR_b.TEST =2;
	CPM->MISCCLKCR_b.TEST =3;
	CPM->MISCCLKCR_b.ARITH_CLK_EN = 0;
	CPM->MISCCLKCR_b.TEST =0;

}
void OpenClockGate(void)
{
	CPM->AHBMSCR = 
#ifndef __DEBUG_MODE
		AHB_CLOCK_GATE_ROM|//ÅäÖÃClock gate
#endif
		 AHB_CLOCK_GATE_PSRAM;
    CPM->MSCR = 
		//IPS_CLOCK_GATE_EFUSE0|
		IPS_CLOCK_GATE_EFUSE1|
		IPS_CLOCK_GATE_IIC|
		IPS_CLOCK_GATE_PSRAM|
		IPS_CLOCK_GATE_SPI0|
		IPS_CLOCK_GATE_SPI1;
	CPM->MISCCLKCR_b.TEST =1;
	CPM->MISCCLKCR_b.TEST =2;
	CPM->MISCCLKCR_b.TEST =3;
	CPM->MISCCLKCR_b.ARITH_CLK_EN = 1;
	CPM->MISCCLKCR_b.TEST =0;
}
void pll_switch(void)
{
	//clear write protect
	CPM->SYSCCFG_b.TEST = 1;
	CPM->SYSCCFG_b.TEST = 2;
	CPM->SYSCCFG_b.TEST = 3;
	//config ipsdiv=1
	CPM->SYSCCFG_b.IPS_CLK = 1;
	CPM->SYSCCFG_b.CLK_OUT_SEL = 7; //²»Êä³öclk
#ifdef SYSTEM_USE_100M
	//ÏµÍ³ÅÜ100M BPLCÐèÒª²»·ÖÆµ
	CPM->SYSCCFG_b.SYS_CLK_DIV=1;
	CPM->SYSCCFG_b.BPLC_CLK_DIV = 0;
	CPM->SYSCCFG_b.HRF_CLK_DIV = 0;
	
	CPM->MISCCLKCR_b.TEST =1;
	CPM->MISCCLKCR_b.TEST =2;
	CPM->MISCCLKCR_b.TEST =3;
	CPM->MISCCLKCR_b.ATRITH_CLK_DIV =0;
	CPM->MISCCLKCR_b.TEST =0;
#endif
	CPM->PMUCR_b.TEST = 1;
	CPM->PMUCR_b.TEST = 2;
	CPM->PMUCR_b.TEST = 3;
	CPM->PMUCR_b.RG_DCDC_VOUT_TR = 0;
	CPM->PMUCR_b.RG_VDD12_SEL = 1;
	CPM->PMUCR_b.TEST = 0;
	
	//config pll
	CPM->PLL_CONFIG0_b.PLL_FBDIV = 64;
	CPM->PLL_CONFIG0_b.PLL_REFDIV = 1;
	CPM->PLL_CONFIG0_b.PLL_POSTDIV1 = 8;
	CPM->PLL_CONFIG0_b.PLL_POSTDIV2 = 1;
	CPM->PLL_CONFIG0_b.PLL_PD = 0;
	CPM->PLL_CONFIG0_b.PLL_DSMPD = 0;
	CPM->PLL_CONFIG0_b.PLL_BYPASS = 0;
	CPM->PLL_CONFIG0_b.PLL_FOUT1PD = 0;
	CPM->PLL_CONFIG0_b.PLL_FOUTVCOPD = 0;
	CPM->PLL_CONFIG0_b.PLL_FOUT2PD = 1;
	CPM->PLL_CONFIG1_b.PLL_FRAC = 0;
	trim_PLL(0, 1); //(-402,0);
	//wait pll lock
	while (CPM->PLL_CONFIG1_b.PLL_LOCK == 0);
	//switch to pll
	CPM->SYSCCFG_b.CLK_MUX = 2;
	//enable write protect
	CPM->SYSCCFG_b.TEST = 0;
}
#elif defined(ZB205_CHIP)
void CloseClockGate(void)
{

	CPM->AHBMSCR_b.TEST = 1;
	CPM->AHBMSCR_b.TEST = 2;
	CPM->AHBMSCR_b.TEST = 3;
	CPM->AHBMSCR = 
		 ~AHB_CLOCK_GATE_AES&
		 ~AHB_CLOCK_GATE_CRC&
		 ~AHB_CLOCK_GATE_CRY&
#ifndef __DEBUG_MODE
		 ~AHB_CLOCK_GATE_ROM&
#endif
		 ~AHB_CLOCK_GATE_PSRAM;//ÅäÖÃClock gate
        CPM->AHBMSCR_b.TEST = 0;
        CPM->MSCR_b.TEST = 1;
        CPM->MSCR_b.TEST = 2;
        CPM->MSCR_b.TEST = 3;
    CPM->MSCR = 
		~IPS_CLOCK_GATE_EFUSE0&
		~IPS_CLOCK_GATE_EFUSE1&
		~IPS_CLOCK_GATE_IIC&
		~IPS_CLOCK_GATE_PSRAM&
		~IPS_CLOCK_GATE_SPI0&
		~IPS_CLOCK_GATE_SPI1;
        CPM->MSCR_b.TEST = 0;
	CPM->MISCCLKCR_b.TEST =1;
	CPM->MISCCLKCR_b.TEST =2;
	CPM->MISCCLKCR_b.TEST =3;
	CPM->MISCCLKCR_b.ARITH_CLK_EN = 0;
	CPM->MISCCLKCR_b.TEST =0;
}
void OpenClockGate(void)
{

	CPM->AHBMSCR_b.TEST = 1;
        CPM->AHBMSCR_b.TEST = 2;
        CPM->AHBMSCR_b.TEST = 3;
	CPM->AHBMSCR = 
#ifndef __DEBUG_MODE
		~AHB_CLOCK_GATE_ROM&//ÅäÖÃClock gate
#endif
		~AHB_CLOCK_GATE_PSRAM;
        CPM->AHBMSCR_b.TEST = 0;
        CPM->MSCR_b.TEST = 1;
        CPM->MSCR_b.TEST = 2;
        CPM->MSCR_b.TEST = 3;
    CPM->MSCR = 
		~IPS_CLOCK_GATE_EFUSE1&
		~IPS_CLOCK_GATE_IIC&
		~IPS_CLOCK_GATE_PSRAM&
		~IPS_CLOCK_GATE_SPI0&
		~IPS_CLOCK_GATE_SPI1;
        CPM->MSCR_b.TEST = 0;
	CPM->MISCCLKCR_b.TEST =1;
	CPM->MISCCLKCR_b.TEST =2;
	CPM->MISCCLKCR_b.TEST =3;
	CPM->MISCCLKCR_b.ARITH_CLK_EN = 1;
	CPM->MISCCLKCR_b.TEST =0;
}
void pll_switch(void)
{
	//clear write protect
	CPM->SYSCCFG_b.TEST = 1;
	CPM->SYSCCFG_b.TEST = 2;
	CPM->SYSCCFG_b.TEST = 3;
	CPM->SYSCCFG_b.CLK_OUT_SEL = 7;
    CPM->OSCCR_b.TEST = 1;
    CPM->OSCCR_b.TEST = 2;
    CPM->OSCCR_b.TEST = 3;
    CPM->OSCCR_b.RG_OSC_DRIVE = 0x3f;
    CPM->OSCCR_b.RG_OSC_CAPTRIM = RG_OSC_CAPTRIM_VAL;
    CPM->OSCCR_b.RG_OSC_BIAS_TRIM = 0x1;
	CPM->OSCCR_b.RG_OSC_EN_ALC = 0;
    CPM->OSCCR_b.TEST = 0;
	CPM->PMUCR_b.TEST = 1;
	CPM->PMUCR_b.TEST = 2;
	CPM->PMUCR_b.TEST = 3;
	CPM->PMUCR_b.RG_AVDD18_TR = 2;
#ifdef HRF_SEND_HIGH_1P5
	CPM->PMUCR_b.RG_DCDC_VOUT_TR = 2;
	CPM->PMUCR_b.RG_VDD12_SEL = 1;
#else
	#ifdef NW_TEST
	CPM->PMUCR_b.RG_DCDC_VOUT_TR = 2;
	CPM->PMUCR_b.RG_VDD12_SEL = 1;
	#else
	CPM->PMUCR_b.RG_DCDC_VOUT_TR = DCDC_VOUT_VAL;
	CPM->PMUCR_b.RG_VDD12_SEL = 1;
	#endif
#endif
	CPM->PMUCR_b.TEST = 0;

	//config pll
	CPM->PLL_CONFIG0_b.TEST = 1;
	CPM->PLL_CONFIG0_b.TEST = 2;
	CPM->PLL_CONFIG0_b.TEST = 3;
	CPM->PLL_CONFIG1_b.TEST = 1;
	CPM->PLL_CONFIG1_b.TEST = 2;
	CPM->PLL_CONFIG1_b.TEST = 3;
	CPM->PLL_CONFIG0_b.PLL_FBDIV = 32;
	CPM->PLL_CONFIG0_b.PLL_REFDIV = 1;
	CPM->PLL_CONFIG0_b.PLL_POSTDIV1 = 8;
	CPM->PLL_CONFIG0_b.PLL_POSTDIV2 = 1;
	CPM->PLL_CONFIG0_b.PLL_PD = 0;
	CPM->PLL_CONFIG0_b.PLL_DSMPD = 0;
	CPM->PLL_CONFIG1_b.PLL_BYPASS = 0;
	CPM->PLL_CONFIG0_b.PLL_FOUT1PD = 0;
	CPM->PLL_CONFIG0_b.PLL_FOUTVCOPD = 0;
	CPM->PLL_CONFIG0_b.PLL_FOUT2PD = 1;
	CPM->PLL_CONFIG1_b.PLL_FRAC = 0;
	CPM->PLL_CONFIG0_b.TEST = 0;
	CPM->PLL_CONFIG1_b.TEST = 0;
	trim_PLL(0, 1); //(-402,0);
	//wait pll lock
	while (CPM->PLL_CONFIG1_b.PLL_LOCK == 0);
	//switch to pll
	CPM->SYSCCFG_b.CLK_MUX = 2;
	//enable write protect
	CPM->SYSCCFG_b.TEST = 0;
}

#endif


void SystemSetCoreDcdc(int isHigh)
{
	static u8 last_mode=0xff;
	if (last_mode!=isHigh)
	{
		last_mode=isHigh;
		CPM->PMUCR_b.TEST = 1;
		CPM->PMUCR_b.TEST = 2;
		CPM->PMUCR_b.TEST = 3;
		if (!isHigh)
		{
			CPM->PMUCR_b.RG_DCDC_VOUT_TR = 2;//1.3V
			CPM->PMUCR_b.RG_VDD12_SEL = 1;
		}
		else
		{
			CPM->PMUCR_b.RG_DCDC_VOUT_TR = 0;//1.5V
			CPM->PMUCR_b.RG_VDD12_SEL = 1;
		}
		CPM->PMUCR_b.TEST = 0;
		if (!isHigh)
		{
			//DelayUs(10);
		}
		else
		{
			DelayUs(20);
		}
	}
}

void SystemCoreClockUpdate (void)
{
	SystemCoreClock = SYSTEM_CLOCK;
#ifdef FPGA_ENV
	IpgClock=SystemCoreClock;
#else
	IpgClock=SystemCoreClock/(CPM->SYSCCFG_b.IPS_CLK+1);
#endif
	
}

void SystemInit (void)
{
	SystemCoreClock = SYSTEM_CLOCK;
#ifdef FPGA_ENV
	IpgClock=SystemCoreClock;
#else
	IpgClock=SystemCoreClock/(CPM->SYSCCFG_b.IPS_CLK+1);
	pll_switch();
	#if defined(ZB204_CHIP)
	CPM->AHBMSCR = 0;
	CPM->MSCR = 0;
	#elif defined(ZB205_CHIP)
	CPM->AHBMSCR_b.TEST = 1;
	CPM->AHBMSCR_b.TEST = 2;
	CPM->AHBMSCR_b.TEST = 3;
	CPM->MSCR_b.TEST = 1;
	CPM->MSCR_b.TEST = 2;
	CPM->MSCR_b.TEST = 3;
	CPM->AHBMSCR = 0xffffffff;
	CPM->MSCR = 0xffffffff;
	CPM->AHBMSCR_b.TEST = 0;
	CPM->MSCR_b.TEST = 0;
	#endif
#endif
}


void Unlock_CPM_Reg(volatile uint32_t * reg)
{
	uint32_t temp=*reg;
	if((temp&0xC0000000)!=0xC0000000)
	{	
		*reg=0x40000000|temp;
		*reg=0x80000000|temp;
		*reg=0xc0000000|temp;
	}
}
void Lock_CPM_Reg(volatile uint32_t * reg)
{
	*reg&=0x3fffffff;
}
/**
 * set system and peripheral clock divider
 * 
 * @param sysDiv system divider
 * @param ipgDiv peripheral clock divider
 * 
 * @return status
 */
int SystemClockSet(uint32_t sysDiv,uint32_t ipgDiv)
{


	return ARM_DRIVER_OK;
}
#ifdef DB001
int AdcClockSet(uint32_t adcDiv)
{
	if (adcDiv>15) 
	{
		return -1;
	}
	CPM->SYNCR_b.ADCDIV=adcDiv;
	return 0;
}
#endif




///**
// * set swt clock source 0:2K 1:32K
// *
// * @param clk_sel clock select
// *
// * @return status
// */
//int Set_SWT_Clock_2K_32K(uint32_t clk_sel)
//{
//	if (CPM->MCR_b.TEST == 0x3 || clk_sel>1)
//	{
//		CPM->MCR_b.SWT_CLK_SEL = clk_sel;
//		return ARM_DRIVER_OK;
//	}
//	else                        /*  */
//	{
//#if 0			 /* all unlock option at external function,Alert programmer unlock  (It's important)  */
//		/*unlock TEST field */
//		CPM->MCR_b.TEST = 1;
//		CPM->MCR_b.TEST = 2;
//		CPM->MCR_b.TEST = 3;
//
//		CPM->MCR_b.SWT_CLK_SEL = clk_sel;
//		CPM->MCR_b.TEST = 0;
//		return ARM_DRIVER_OK;
//#else
//		return ARM_DRIVER_ERROR;
//#endif
//	}
//}

/**
 * get chip SWT clock unit is us
 *
 * @author sitieqiang 2017/1/16
 *
 * @return  period  unit is us
 */
 
uint32_t Get_SWT_Clock_us(void)
{
	#ifdef DB001
	if (CPM->MCR_b.WDTCLKWSEL)
	{
		return 500;	//2K
	}
	else
	{
		return 30;//32768K
	}
	#else
	return 30;
	#endif
	
}







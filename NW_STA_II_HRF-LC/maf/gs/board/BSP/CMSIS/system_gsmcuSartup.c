/**
  ******************************************************************************
  * @file    system_stm32f4xx.c
  * @author  MCD Application Team
  * @version V2.3.0
  * @date    02-March-2015
  * @brief   CMSIS Cortex-M4 Device Peripheral Access Layer System Source File.
  *
  *   This file provides two functions and one global variable to be called from 
  *   user application:
  *      - SystemInit(): This function is called at startup just after reset and 
  *                      before branch to main program. This call is made inside
  *                      the "startup_stm32f4xx.s" file.
  *
  *      - SystemCoreClock variable: Contains the core clock (HCLK), it can be used
  *                                  by the user application to setup the SysTick 
  *                                  timer or configure other parameters.
  *                                     
  *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
  *                                 be called whenever the core clock is changed
  *                                 during program execution.
  *
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32f4xx_system
  * @{
  */  
  
/** @addtogroup STM32F4xx_System_Private_Includes
  * @{
  */


#include <gsmcu_hal.h>
#include <stdlib.h>

uint32_t HAL_RCC_GetHCLKFreq(void)
{
  //SystemCoreClock = HAL_RCC_GetSysClockFreq() >> APBAHBPrescTable[(RCC->CFGR & RCC_CFGR_HPRE)>> POSITION_VAL(RCC_CFGR_HPRE)];
  return SystemCoreClock;
}


/************************* Miscellaneous Configuration ************************/

#define VECT_TAB_OFFSET  0x00 /*!< Vector Table base offset field. 
                                   This value must be a multiple of 0x200. */
/******************************************************************************/



/* This variable is updated in three ways:
  1) by calling CMSIS function SystemCoreClockUpdate()
  2) by calling HAL API function HAL_RCC_GetHCLKFreq()
  3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency 
     Note: If you use this function to configure the system clock; then there
           is no need to call the 2 first functions listed above, since SystemCoreClock
           variable is updated automatically.
*/
uint32_t SystemCoreClock = SYSCLK_FREQ_MHZ;


void sw(WORD addr, WORD data, BYTE type)
{
    switch( type ){
        case HSIZE_BYTE:
            *((BYTE  *) addr) = (BYTE) data;
            break;
        case HSIZE_HWORD:
            *((HWORD *) addr) = (HWORD) data;
            break;
        case HSIZE_WORD:
            *((WORD  *) addr) = (WORD) data;
            break;
        default:
            break;
    }
}

void sr(WORD addr, BYTE type)
{
    volatile int   i;

    switch( type ){
        case HSIZE_BYTE:
            i = *((BYTE *) addr);
            break; 
        case HSIZE_HWORD:
            i = *((HWORD *) addr);
            break;
        case HSIZE_WORD:
            i = *((WORD  *) addr);
            break;
        default:
            break;
    } 
}


int src( WORD addr, WORD data, BYTE type)
{
    switch(type){
        case HSIZE_BYTE:
            if( data != *((BYTE  *) addr))
                return 0; 
            break;
        case HSIZE_HWORD:
            if(data != *((HWORD *) addr))
                return 0; 
            break;
        case HSIZE_WORD:
            if( data != *((WORD  *) addr) )
                return 0; 
            break;    
    }  
    return 1;
}

void PLL_default(void)
{
  REG_32BIT(SCU_BASE, 0x844) = (REG_32BIT(SCU_BASE, 0x844)&(~0xFFF))|0x40;
  REG_32BIT(SCU_BASE, 0x848) = (REG_32BIT(SCU_BASE, 0x848)&(~0xFFFFFF));
}

uint32_t trim_PLL(int32_t freq_diff, uint8_t reset)//*2^24
{ 
	uint64_t a;
	int b;
	int a_int;
	//uint32_t cur_reg_val;
    uint32_t pll_integer,pll_frac;
	uint32_t ntb_pre,ntb_post, ntb_diff;
	if(reset != 1)
	{
		pll_integer = REG_32BIT(SCU_BASE, 0x844)&0xFFF;
		pll_frac = REG_32BIT(SCU_BASE, 0x848)&0xFFFFFF;
		a = ((uint64_t)pll_integer<<24);
		a_int = (int)pll_integer;
		a += (uint64_t)pll_frac;
		b = ((1<<24)+(a_int*freq_diff));
		a += ((uint64_t)b);
		a -= (1<<24);
		pll_frac = (uint32_t)(a & 0xffffff);
		pll_integer = (uint32_t)(a >> 24);
		if(pll_integer < 63 || pll_integer > 64 || (pll_integer == 63 && pll_frac < 0xF00000) || (pll_integer == 64 && pll_frac > 0x100000))
		{
			pll_integer = 64;
			pll_frac = 0;
		}
	}
	else
	{
		pll_integer = 64;
		pll_frac = 0;
	}
	//SYS_CONFIG_b.DD = 0;
	//    REG_32BIT(SCU_BASE, 0x840) = (REG_32BIT(SCU_BASE, 0x840)&(~0x2));
	// change sys clock to OSC25M
	//	cur_reg_val = SCU->PLL_CTRL;
	//	SCU->PLL_CTRL = 0;
	ntb_pre = BPLC->NTB_COUNTER_VALUE;
	//	REG_32BIT(SCU_BASE, 0x20) = (REG_32BIT(SCU_BASE, 0x20)|0x40);
	//	__asm("wfi"); //FCS for cortex M3
	
	REG_32BIT(SCU_BASE, 0x848) = (0x01<<24)|pll_frac;
	REG_32BIT(SCU_BASE, 0x844) = 0x11180000|pll_integer;
	__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
	// Wait for FRAC PLL lock
	//	while ((READ(SCU_BASE + 0x84c) & 0x80000000) != 0x80000000)
	//	{
	//	}
	//	
	//	SCU->PLL_CTRL = cur_reg_val;
	//	REG_32BIT(SCU_BASE, 0x20) = (REG_32BIT(SCU_BASE, 0x20)|0x40);
	//	__asm("wfi"); //FCS for cortex M3
	ntb_post = BPLC->NTB_COUNTER_VALUE;
	ntb_diff = ntb_post - ntb_pre;
	return ntb_diff;	
}


static void PLL_Init(void)
{

	unsigned int rdata;
	unsigned int wdata;
	unsigned int CLK_MUX;
	unsigned int BUS_MUX;
	unsigned int FCLK_MUX;
	unsigned int SELF_REFLSH;
	unsigned int REMAP;
	int temp;

// ==================== PLL enable =======================
	
	temp=READ(SCU_BASE + 0x840);
	if (temp!=0xffffffff)
	{
#if yy_tbd
        //need to reset again;
#endif
	//	return;
	}
    
	// Enable PLL by default setting, POSTDIV1_1=4'd8, POSTDIV1_2=4'd1
	WRITE(SCU_BASE + 0x840,   0x00000000);
	REG_32BIT(SCU_BASE, 0x848) = (0x01<<24)|0;
	REG_32BIT(SCU_BASE, 0x844) = 0x11180000|0x40;
	// Wait for FRAC PLL lock
	while ((READ(SCU_BASE + 0x84c) & 0x80000000) != 0x80000000)
	{
//		display("Wait for PLL Lock!"); 
	}
	//display ("##### PLL Locked! #####");

// ==================== Pattern ========================
// Initial the INTC
	sw(0xE000E280,  0x00000001, HSIZE_WORD);
	sw(0xE000E100,  0x00000001, HSIZE_WORD);
	sr(0xE000E100,  HSIZE_WORD);
//	NVIC_EnableIRQ(0);

//======== power on check ========
// write/read some registers
	src(SCU_BASE + SCU_BTUPSTS,          0x00010000, HSIZE_WORD);
	sw(SCU_BASE + SCU_BTUPSTS,          0x00010000, HSIZE_WORD);

	sw(SCU_BASE + SCU_BTUPCTL,          0x00020000, HSIZE_WORD);

#ifdef FTSCU100_DEBOUNCE_ON
	rdata = (FTSCU100_PCDCSR_DEF << 28) | (FTSCU100_DEBOUNCE_DEF << 16) | (FTSCU100_PCPWRUP_DEF << 8) | FTSCU100_PCPWRDN_DEF;
#else
	rdata = (FTSCU100_PCDCSR_DEF << 28) | (FTSCU100_PCPWRUP_DEF << 8) | FTSCU100_PCPWRDN_DEF;
#endif
//	debugport(rdata);
	src(SCU_BASE + SCU_PWRCTL,           rdata,         HSIZE_WORD);
	sw(SCU_BASE + SCU_BTUPSTS,          0x00000001, HSIZE_WORD);

	sw(SCU_BASE + SCU_PWRCTL,           0x00003f00, HSIZE_WORD);

// clear scu state
	sw(SCU_BASE + SCU_STATUS,           0xffffffff, HSIZE_WORD);

//==============1. FCS setting ========================
	CLK_MUX  = 1|0<<1|0<<2; // system is pll clk, pclk is hclk_div4,clkb is sys_clk_div4 ,clka is sys_clk_div4
                                 //[0]: pll_clk / clk_25MHz clk_mux
                                      //0 : clk_25MHz
                                      //1 : pll_clk
                                 //[1]: AES/CRY/SHA hclk_div2 / hclk_div4 clk_mux (PCLK)
                                      //0 : hclk_div4
                                      //1 : hclk_div2
                                 //[3:2]: clkb clk_mux
                                      //00 : sys_clk_div4
                                      //01 : sys_clk_div2
                                      //10 : sys_clk
                                      //11 : sys_clk
	BUS_MUX  = 0x0;
	SELF_REFLSH = 0x1;
	FCLK_MUX = 0x2;
	REMAP = 0x6;

	wdata = CLK_MUX << 4;
	sw(SCU_BASE + SCU_PLLCTL,     wdata,         HSIZE_WORD);

	wdata = (SELF_REFLSH << 31) | (REMAP << 24) | (BUS_MUX << 20) | (FCLK_MUX << 16) | 0x40;
	sw(SCU_BASE + SCU_PWRMOD,      wdata,    HSIZE_WORD);
	__asm("wfi"); //FCS for cortex M3

	sw(SCU_BASE + 0x24,  0x00000048, HSIZE_WORD);

	rdata = (SELF_REFLSH << 31) | (REMAP << 24) | (BUS_MUX << 20) | (FCLK_MUX << 16) | 0x00;
	src(SCU_BASE + SCU_PWRMOD,       rdata,         HSIZE_WORD);
    //	display(" #### PCLK = HCLK = CLKB = 50MHz Return !!");


}


/**
  * @brief  Setup the microcontroller system
  *         Initialize the FPU setting, vector table location and External memory 
  *         configuration.
  * @param  None
  * @retval None
  */
void SystemInit(void)
{
    
#ifdef __ARMVFP__    
    //Init FPU parameters
    SCB->CPACR|=(0x3<<20)|(0x3<<22);
#endif

    PLL_Init();
    PLL_default();
//	trim_PLL(0);
    SystemCoreClock=SYSCLK_FREQ_MHZ;
}

/**
   * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock (HCLK), it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  *           
  * @note   Each time the core clock (HCLK) changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.         
  *     
  * @note   - The system frequency computed by this function is not the real 
  *           frequency in the chip. It is calculated based on the predefined 
  *           constant and the selected clock source:
  *             
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(*)
  *                                              
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(**)
  *                          
  *           - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(**) 
  *             or HSI_VALUE(*) multiplied/divided by the PLL factors.
  *         
  *         (*) HSI_VALUE is a constant defined in stm32f4xx_hal_conf.h file (default value
  *             16 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.   
  *    
  *         (**) HSE_VALUE is a constant defined in stm32f4xx_hal_conf.h file (its value
  *              depends on the application requirements), user has to ensure that HSE_VALUE
  *              is same as the real frequency of the crystal used. Otherwise, this function
  *              may have wrong result.
  *                
  *         - The result of this function could be not correct when using fractional
  *           value for HSE crystal.
  *     
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate(void)
{  
    SystemCoreClock = SYSCLK_FREQ_MHZ;
}

/**
  * @}
  */

/**
  * @}
  */
  
/**
  * @}
  */    
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

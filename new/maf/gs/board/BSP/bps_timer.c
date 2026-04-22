/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_timer.h"
#include "os.h"
#include <gsmcuxx_hal_def.h>
/*
*
*/
static volatile TimerCallBackFunctionType TimerChanlIspList[BSP_TIMER_CHL_LENGTH] = {
    NULL,NULL,NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,NULL,NULL};



static void  TimerNoTrunRealyTimer(uint32_t Timer,TMR_TypeDef** TMR, uint8_t *TimChl)
{
#if ZB202
    if(Timer < BSP_TIMER_6)
    {
        *TMR = TMR1;
        *TimChl = Timer+1;
    }
    else
    {
        *TMR = TMR0;
        *TimChl = Timer - BSP_TIMER_6+1;
    }
#elif DM750
    if(Timer < BSP_TIMER_3)
    {
        *TMR = TMR0;
        *TimChl = Timer+1;
    }
    else if(Timer < BSP_TIMER_6)
    {
        *TMR = TMR1;
        *TimChl = Timer - BSP_TIMER_3+1;
    }
    else
    {
        *TMR = TMR2;
        *TimChl = Timer - BSP_TIMER_6+1;
    }	
#endif
}

static void TimerIsrFunction()
{
    uint32_t IntNo = ((SCB->ICSR)&SCB_ICSR_VECTACTIVE_Msk);
#if ZB202
    if(IntNo > 16)
    {
        IntNo -= 16;
        if((IntNo >= TMR1_1_IRQn)&&(IntNo <= TMR0_6_IRQn))
        {
            TMR_TypeDef* TMR;
            uint8_t TimChl;
            IntNo -= TMR1_1_IRQn;
            TimerNoTrunRealyTimer(IntNo,&TMR,&TimChl);
            TIM_ClearFlag(TMR,TimChl);
            if(TimerChanlIspList[IntNo] != NULL)
            {
                TimerChanlIspList[IntNo]();
            }
        }
    }
#elif DM750
    if(IntNo > 16)
    {
        IntNo -= 16;
        if((IntNo >= TMR0_1_IRQn)&&(IntNo <= TMR2_6_IRQn))
        {
            TMR_TypeDef* TMR;
            uint8_t TimChl;
            IntNo -= TMR0_1_IRQn;
            TimerNoTrunRealyTimer(IntNo,&TMR,&TimChl);
            TIM_ClearFlag(TMR,TimChl);
            if(TimerChanlIspList[IntNo] != NULL)
            {
                TimerChanlIspList[IntNo]();
            }
        }
    }	
#endif
}

void TimerOpen(TimerParameterTypdef *TimerParameter)
{

    TMR_TypeDef* TMR;
    uint8_t TimChl;
    TIM_InitTypeDef CHL_InitStruct;

    if(TimerParameter->Timer > BSP_TIMER_11)return;
    TimerNoTrunRealyTimer(TimerParameter->Timer,&TMR,&TimChl);
        
    CHL_InitStruct.DeadZone = 0;
    CHL_InitStruct.CfgValue = TIM_INT_ENABLE|TIM_AUTO_RELOAD_ON|TIM_UPDATA;
    CHL_InitStruct.TimReload = TimerParameter->TimerSlot20ns;
    CHL_InitStruct.CmpValue = 0;
    
    TIM_Init(TMR,TimChl,&CHL_InitStruct);
    TIM_ClearFlag(TMR,TimChl);
    //
    TimerChanlIspList[TimerParameter->Timer] = TimerParameter->TimerCallBackFunction;
#if ZB202
    NVIC_ClearPendingIRQ((IRQn_Type)(TMR1_1_IRQn + TimerParameter->Timer));
    BSP_IntVectSet(TMR1_1_IRQn+TimerParameter->Timer,TimerIsrFunction);
    NVIC_EnableIRQ((IRQn_Type)(TMR1_1_IRQn + TimerParameter->Timer));
#elif DM750
	NVIC_ClearPendingIRQ((IRQn_Type)(TMR0_1_IRQn + TimerParameter->Timer));
    BSP_IntVectSet(TMR0_1_IRQn+TimerParameter->Timer,TimerIsrFunction);
    NVIC_EnableIRQ((IRQn_Type)(TMR0_1_IRQn + TimerParameter->Timer));
#endif
    
    
    
    
    
}
//mode = 1 start;mode = 0stop;other nodefine
void TimerWrite(uint32_t Timer,uint32_t mode)
{
    TMR_TypeDef* TMR;
    uint8_t TimChl;
    if(Timer > BSP_TIMER_11)return;
    if(mode > 1)return;
    TimerNoTrunRealyTimer(Timer,&TMR,&TimChl);
    if(mode == 1)
    {
        TIM_Start(TMR,TimChl);
    }
    else
    {
        TIM_Stop(TMR,TimChl);
    }
}
uint32_t TimerRead(uint32_t Timer)
{
    TMR_TypeDef* TMR;
    uint8_t TimChl;
    if(Timer > BSP_TIMER_11)return 0;
    TimerNoTrunRealyTimer(Timer,&TMR,&TimChl);
    return TIM_GetChlCounter(TMR,TimChl);
}
void TimerClose(uint32_t Timer)
{
    TMR_TypeDef* TMR;
    uint8_t TimChl;
    if(Timer > BSP_TIMER_11)return;
    TimerNoTrunRealyTimer(Timer,&TMR,&TimChl);
    
    TIM_Stop(TMR,TimChl);
#if ZB202
    NVIC_DisableIRQ((IRQn_Type)(TMR1_1_IRQn + Timer));
    NVIC_ClearPendingIRQ((IRQn_Type)(TMR1_1_IRQn + Timer));
#elif DM750
    NVIC_DisableIRQ((IRQn_Type)(TMR0_1_IRQn + Timer));
    NVIC_ClearPendingIRQ((IRQn_Type)(TMR0_1_IRQn + Timer));
#endif
    TimerChanlIspList[Timer] = NULL;
    
}
























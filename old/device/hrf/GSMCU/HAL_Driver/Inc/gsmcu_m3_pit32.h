/****************************************************************
 *
 * 32 bit PIT model
 * @file gsmcu_m3_pit32.h
 * @author sitieqiang                     
 * @version 0.1
 * @date 2017/12/7 16:33:29
 * @note none
 *
*****************************************************************/

#ifndef __gsmcu_m3_PIT32_H__
#define __gsmcu_m3_PIT32_H__
#ifdef __cplusplus
extern "C"
{
#endif


#include "GSMCU_M3.h"
/**
 * Get Current Timer Value
 * 
 * @param pit    pointer for PIT
 * 
 * @return Current Timer Value
 */
__STATIC_INLINE uint32_t PIT32_GetCnt(PIT_CHANNEL_Type* pit)
{
	return pit->CVAL;
}
/**
 * Enable PIT
 * 
 * @param pit    pit pointer
 * 
 * @return 
 */
__STATIC_INLINE void PIT32_Enable(PIT_CHANNEL_Type* pit)
{
	pit->TCTRL_b.EN=1;
}
/**
 * Disable PIT
 * 
 * @param pit    PIT pointer
 * 
 * @return 
 */
__STATIC_INLINE void PIT32_Disable(PIT_CHANNEL_Type* pit)
{
	pit->TCTRL_b.EN=0;
}


/**
 * Enable PIT interrupt
 * 
 * @param pit    PIT pointer
 * 
 * @return 
 */
__STATIC_INLINE void PIT32_EnInt(PIT_CHANNEL_Type* pit)
{
	pit->TCTRL_b.TIE=1;
}
/**
 * Disable PIT interrupt
 * 
 * @param pit    PIT pointer
 * 
 * @return 
 */
__STATIC_INLINE void PIT32_DisInt(PIT_CHANNEL_Type* pit)
{
	pit->TCTRL_b.TIE=0;
}

/**
 * Read PIT status
 * 
 * @param pit    PIT pointer
 * 
 * @return PIT status
 */
__STATIC_INLINE int PIT32_Status(PIT_CHANNEL_Type* pit)
{
	return pit->TFLG;
}

/**
 * Clear PIT status
 * 
 * @param pit    PIT pointer
 * 
 * @return 
 */
__STATIC_INLINE void PIT32_Clear(PIT_CHANNEL_Type* pit)
{
	pit->TFLG=1;
}

/**
 * configure PIT
 * 
 * @param pit    PIT pointer
 * @param tick
 * 
 * @return 
 */
extern void PIT32_Config(PIT_CHANNEL_Type* pit, uint32_t tick);
#ifdef __cplusplus
}
#endif
#endif /* __gsmcu_m3_PIT32_H__ */


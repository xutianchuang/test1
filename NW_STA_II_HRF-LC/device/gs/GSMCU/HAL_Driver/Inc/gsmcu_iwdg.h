#ifndef __GSMCU_IWDG_H__
#define __GSMCU_IWDG_H__
#ifdef __cplusplus
extern "C"
{
#endif



void WDG_SetWindowMode(void);
void WDG_SetTimeOutMode(void);
void WDG_HardLock(void);
void WDG_SoftLock(void);
void WDG_SoftUnLock(void);
//void WDG_SetWindowUs(uint32_t us);
//void WDG_SetTimeOutUs(uint32_t timeout);
void Feed_WDG(void);
void WDG_Enable(void);
void WDG_Disable(void);
void WDG_EnInterrupt(void);
void WDG_ShortReset(void);



#ifdef __cplusplus
}
#endif
#endif /* __GSMCU_IWDG_H__ */


#include "System_inc.h"
#include "../../src/pack2lib/pack2lib.h"

//--------para--------------
static TMR_CALL BPLC_Task_Tmr;
//-------start-------

void HPLC_InitTmr(void)
int HPLC_RegisterTmr(pTmrFunCall fun,void *arg,u32 ms,TMR_OPT opt)
bool HPLC_StopTmr(int id)
bool HPLC_StartTmr(int id)
bool HPLC_ReSetTmr(int id,u32 ms)
bool HPLC_DelTmr(int id)
void HPLC_RunTmr(void)
u32 HPLC_LeftTmr(int id)

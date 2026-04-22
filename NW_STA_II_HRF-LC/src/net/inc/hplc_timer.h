#ifndef _HPLC_TIMER_H_
#define _HPLC_TIMER_H_
#include "common_includes.h"

//初始化BPLC协议栈定时器
void HPLC_InitTmr(void);

//注册一个定时器（注册后立即执行）
int HPLC_RegisterTmr(pTmrFunCall fun,void *arg,u32 ms,TMR_OPT opt);

//停止一个定时器
bool HPLC_StopTmr(int id);

//启动一个定时器
bool HPLC_StartTmr(int id);

//重置一个定时器
bool HPLC_ReSetTmr(int id,u32 ms,TMR_RESET_OPT);

//删除一个定时器
bool HPLC_DelTmr(int id);

//定时器运行
void HPLC_RunTmr(void);

//返回一个定时器剩余时间
u32 HPLC_LeftTmr(int id);


#define BEACON_NTB_TIME_NUM     (0)
#define DETECT_BEACON_TIME_NUM  (1)
#define NBI_TIME_NUM            (3)
#endif

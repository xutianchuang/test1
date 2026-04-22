#include "common_includes.h"
#include "os.h"
#include <limits.h>

void StartTimer(TimeValue *t, u32 mse)
{
    OS_ERR err;
    u32 basetime = OSTimeGet(&err)*(1000/OSCfg_TickRate_Hz);
    t->BeginTimeStamp = basetime;
    t->OverFlowTimeStamp = basetime + mse;
}

bool TimeOut(TimeValue *t)
{
    OS_ERR err;
    u32 basetime = OSTimeGet(&err)*(1000/OSCfg_TickRate_Hz);
	if (t->OverFlowTimeStamp ==0
        &&t->BeginTimeStamp==0)
    {
        return TRUE;
    }
    //think about it...
    if(t->OverFlowTimeStamp - basetime > basetime - t->OverFlowTimeStamp)
    {
        return TRUE;
    }
    return false;
}

//定时器单元是否已经超过过此时间段
bool TimeOver(TimeValue *t,u32 mse)
{
    OS_ERR err;
    u32 timeStamp = t->BeginTimeStamp + mse;
    u32 basetime = OSTimeGet(&err)*(1000/OSCfg_TickRate_Hz);
    if(timeStamp - basetime >= basetime - timeStamp)
    {
        return TRUE;
    }
    return false;
}

//定时器单元还剩多少溢出
u32  TimeLeft(TimeValue *t)
{
    OS_ERR err;
    u32 basetime = OSTimeGet(&err)*(1000/OSCfg_TickRate_Hz); 
    
    if(TimeOut(t))
        return 0;
    else
        return t->OverFlowTimeStamp - basetime;
}



void TimeStop(TimeValue *t)
{
    t->OverFlowTimeStamp = 0;
    t->BeginTimeStamp = 0;
}


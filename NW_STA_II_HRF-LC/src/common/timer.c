#include "common_includes.h"
#include "os.h"
#include <limits.h>

void StartTimer(TimeValue *t, u32 mse)
{
    OS_ERR err;
    u32 basetime = OSTimeGet(&err)*(1000/OSCfg_TickRate_Hz);
    t->mse = basetime + mse;
    if(t->mse < basetime)
    {
        t->carry = TRUE;
    }
    else
    {
        t->carry = FALSE;  
    }
}

bool TimeOut(TimeValue *t)
{
    OS_ERR err;
    u32 basetime = OSTimeGet(&err)*(1000/OSCfg_TickRate_Hz);  
    if(t->carry == FALSE)
    {
        if(t->mse <= basetime)
            return TRUE;
    }
    else
    {
        if(t->mse >= basetime)
        {
          t->carry = FALSE;
        }
    }
    return FALSE;
}

//定时器单元还剩多少溢出
u32  TimeLeft(TimeValue *t)
{
    OS_ERR err;
    u32 basetime = OSTimeGet(&err)*(1000/OSCfg_TickRate_Hz); 
    if(t->carry == FALSE)
    {
        if(t->mse <= basetime)
            return 0;
        else
            return t->mse - basetime;
    }
    else
    {
        if(basetime < ULONG_MAX)
        {
            return ULONG_MAX - basetime + t->mse;
        }
        else
        {
             if(t->mse <= basetime)
                return 0;
            else
                return t->mse - basetime;
        }
    }
}
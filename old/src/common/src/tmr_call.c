#include "common_includes.h"
#include "os.h"
#include <string.h>


//定时器初始化
void TmrCallInit(TMR_CALL* tmr)
{
    OS_ERR err;
    List_Init(&tmr->TmrList);
    OSMemCreate(&tmr->TmrMempool,"TMR List",tmr->Tmrmem, MAX_TMR_NUM, sizeof(TMR_FUN_CALL), &err);
    if(err != OS_ERR_NONE)
    {
        while(1);
    }
    
}

//分配一个PDU
static TMR_FUN_CALL* MallocTmr(TMR_CALL* tmr)
{
    OS_ERR err;
    TMR_FUN_CALL *tmr_ptr = 0;
    tmr_ptr = (TMR_FUN_CALL*)OSMemGet(&tmr->TmrMempool,&err);
    if(err != OS_ERR_NONE)
        return (void*)0;
    return tmr_ptr;
}

//释放一个PDU
static void FreeTmr(TMR_CALL* tmr,TMR_FUN_CALL* t)
{
    OS_ERR err;
    t->run = false;
    OSMemPut(&tmr->TmrMempool,t,&err);
}

//设置一个定时回调函数(回调函数,超时时间,模式),返回定时器标号
int TmrSet(TMR_CALL * tmr,pTmrFunCall fun,void*arg,u32 ms,TMR_OPT opt)
{
    TMR_FUN_CALL *t = MallocTmr(tmr);
    if(t == NULL)
        return INVAILD_TMR_ID;
    t->pFun = fun;
    t->arg = arg;
    t->ms = ms;
	if(opt == TMR_OPT_CALL_PERIOD_IMM)
    	StartTimer(&t->timer,1);
	else
		StartTimer(&t->timer,ms);
    t->opt = opt;
    t->run = true;
    if(!List_Push_Back(&tmr->TmrList,t))
    {
        FreeTmr(tmr,t);
        return INVAILD_TMR_ID;
    }
    return ((unsigned long)t - (unsigned long)tmr->Tmrmem)/sizeof(TMR_FUN_CALL);
}

//删除一个定时回调函数
bool TmrDel(TMR_CALL* tmrList,int id)
{
    if(id > sizeof(tmrList->Tmrmem)/sizeof(TMR_FUN_CALL))
        return false;
    TMR_FUN_CALL *tmr = (TMR_FUN_CALL *)((unsigned long)tmrList->Tmrmem + id * sizeof(TMR_FUN_CALL));
    List_Del_Data(&tmrList->TmrList,tmr);
    FreeTmr(tmrList,tmr);
    return true;
}

//重置一个定时器时间
bool TmrReset(TMR_CALL* tmrList,int id,u32 ms,TMR_RESET_OPT opt)
{
    if(id > sizeof(tmrList->Tmrmem)/sizeof(TMR_FUN_CALL))
        return false;
    TMR_FUN_CALL *tmr = (TMR_FUN_CALL *)((unsigned long)tmrList->Tmrmem + id * sizeof(TMR_FUN_CALL));
    if(ms != 0) 
        tmr->ms = ms;
    if(opt == TMR_RESET_IMMEDIATELY)
        StartTimer(&tmr->timer,tmr->ms);
    return true;
}

//停止一个定时器
bool TmrStop(TMR_CALL* tmrList,int id)
{
    if(id > sizeof(tmrList->Tmrmem)/sizeof(TMR_FUN_CALL))
        return false;
    TMR_FUN_CALL *tmr = (TMR_FUN_CALL *)((unsigned long)tmrList->Tmrmem + id * sizeof(TMR_FUN_CALL));
    tmr->run = false;
    return true;
}

//启动一个定时器
bool TmrStart(TMR_CALL* tmrList,int id)
{
    if(id > sizeof(tmrList->Tmrmem)/sizeof(TMR_FUN_CALL))
        return false;
    TMR_FUN_CALL *tmr = (TMR_FUN_CALL *)((unsigned long)tmrList->Tmrmem + id * sizeof(TMR_FUN_CALL));
    tmr->run = true;
    StartTimer(&tmr->timer,tmr->ms);
    return true;
}

//当前执行的定时器
static TMR_FUN_CALL *CurrentTMR = NULL;

//定时器执行函数
void TmrRun(TMR_CALL* tmrList)
{
    ListNode *node = tmrList->TmrList.head;
    for(;node != NULL;/*node= node->next*/)
    {
        TMR_FUN_CALL *tmr = (TMR_FUN_CALL *)node->data;
		CurrentTMR = tmr;
        if(tmr->run && TimeOut(&tmr->timer))
        {
            tmr->pFun(tmr->arg);
            if(tmr->opt == TMR_OPT_CALL_ONCE)
            {
                ListNode *nextNode = node->next;
                List_Del(&tmrList->TmrList,node);
                FreeTmr(tmrList,tmr);
                node = nextNode;
                continue;
            }
            else
            {
                StartTimer(&tmr->timer,tmr->ms);
            }
        }
        node= node->next;
    }
    CurrentTMR = NULL;
}

//在定时器函数内部停止删除定时器
void DeleteTmrSelf(void)
{
    if(CurrentTMR)
	{
		CurrentTMR->opt = TMR_OPT_CALL_ONCE;
	}
}

//返回定时函数下一个定时器溢出间隔时间
u32 GetTmrLeftTime(TMR_CALL* tmrList,int id)
{
    if(id > sizeof(tmrList->Tmrmem)/sizeof(TMR_FUN_CALL))
        return 0;
    TMR_FUN_CALL *tmr = (TMR_FUN_CALL *)((unsigned long)tmrList->Tmrmem + id * sizeof(TMR_FUN_CALL));
    return TimeLeft(&tmr->timer);
}

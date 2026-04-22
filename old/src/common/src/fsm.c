#include "common_includes.h"
#include "hplc_app_def.h"
#include "hplc_update.h"
#include <string.h>

//状态机初始化
void FSMSystemInit(FSMSystem* fsm)
{
    memset(fsm,0,sizeof(FSMSystem));
}

//注册状态动作
void FSMSystemRegisterState(FSMSystem* fsm,int state,pFsmAction run)
{
    if(state < MAX_STATE_NUM && state != INVAILD_STATE)
    {
        fsm->StateList[state].State = state;
        fsm->StateList[state].DoRunning = run;
    }
}

//注册触发动作
void FSMSystemRegistetEvent(FSMSystem* fsm,int state,int event,int newState,pFsmAction fun)
{
    if(state == INVAILD_STATE || event == INVAILD_EVENT)
        return;
    if(state < MAX_STATE_NUM && event < MAX_EVENT_NUM)
    {
        fsm->StateList[state].StateMap[event] = newState;
        fsm->StateList[state].StateEventFun[event] = fun;
    }
}

//执行状态切换
void FSMSystemStateChange(FSMSystem* fsm,int event,void* arg)
{
    if (UPDATE_EVENT_START==event)//下发文件信息单独处理  
    {
        if (StartFirstProc(fsm, arg))
        {
            return;
        }
    }
    if (event < MAX_EVENT_NUM && event != INVAILD_EVENT)
    {
        int newState = fsm->StateList[fsm->CurrentState].StateMap[event];
        debug_str(DEBUG_LOG_UPDATA, "up event is %d,status from %d to %d\r\n",event,fsm->CurrentState,newState);
        if(fsm->StateList[fsm->CurrentState].StateEventFun[event])
            fsm->StateList[fsm->CurrentState].StateEventFun[event](fsm,arg);
        if(newState != INVAILD_STATE)
        {
            fsm->CurrentState = newState;
        }
    }
}

//强制改变状态
void FSMChangeState(FSMSystem* fsm,int state)
{
    if(state < MAX_STATE_NUM && state != INVAILD_STATE)
    {
        fsm->CurrentState = state;
        debug_str(DEBUG_LOG_UPDATA, "force up stats %d\r\n",state);
    }
}

//获取状态机当前状态
int FSMGetState(FSMSystem* fsm)
{
    return fsm->CurrentState;
}

//执行状态机
void FSMSystemRun(FSMSystem* fsm)
{
    if(fsm->StateList[fsm->CurrentState].DoRunning)
        fsm->StateList[fsm->CurrentState].DoRunning(fsm,NULL);
}

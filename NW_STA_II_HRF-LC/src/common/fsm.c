#include "common_includes.h"
#include <string.h>

//×´Ì¬»ú³õÊ¼»¯
void FSMSystemInit(FSMSystem* fsm)
{
    memset(fsm,0,sizeof(FSMSystem));
}

//×¢²á×´Ì¬¶¯×÷
void FSMSystemRegisterState(FSMSystem* fsm,int state,pFsmAction before,pFsmAction run,pFsmAction after)
{
    if(state < MAX_STATE_NUM && state != INVAILD_STATE)
    {
        fsm->StateList[state].State = state;
        fsm->StateList[state].DoEntering = before;
        fsm->StateList[state].DoRunning = run;
        fsm->StateList[state].DoLeaving = after;
    }
}

//×¢²á´¥·¢¶¯×÷
void FSMSystemRegistetEvent(FSMSystem* fsm,int state,int event,int newState)
{
    if(state == INVAILD_STATE || event == INVAILD_EVENT)
        return;
    if(state < MAX_STATE_NUM && event < MAX_EVENT_NUM)
    {
        fsm->StateList[state].StateMap[event] = newState;
    }
}

//Ö´ÐÐ×´Ì¬ÇÐ»»
void FSMSystemStateChange(FSMSystem* fsm,int event,void* arg)
{
    if(event < MAX_EVENT_NUM && event != INVAILD_EVENT)
    {
        int newState = fsm->StateList[fsm->CurrentState].StateMap[event];
        if(newState != INVAILD_STATE)
        {
            if(fsm->StateList[fsm->CurrentState].DoLeaving)
                fsm->StateList[fsm->CurrentState].DoLeaving(arg);
            fsm->CurrentState = newState;
            if(fsm->StateList[fsm->CurrentState].DoEntering)
                fsm->StateList[fsm->CurrentState].DoEntering(arg);
        }
    }
}

//Ö´ÐÐ×´Ì¬»ú
void FSMSystemRun(FSMSystem* fsm)
{
    if(fsm->StateList[fsm->CurrentState].DoRunning)
        fsm->StateList[fsm->CurrentState].DoRunning(NULL);
}
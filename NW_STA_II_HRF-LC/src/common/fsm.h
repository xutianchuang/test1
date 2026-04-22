#ifndef _FSM_H_
#define _FSM_H_

#define MAX_EVENT_NUM       10      //最大事件枚举
#define MAX_STATE_NUM       10      //最大状态枚举

#define INVAILD_EVENT       0       //无效事件
#define INVAILD_STATE       0       //无效状态

typedef void(*pFsmAction)(void*);

//状态描述
typedef struct
{
    int State;      //当前状态
    pFsmAction DoEntering;  //进入状态前动作
    pFsmAction DoRunning;   //状态动作
    pFsmAction DoLeaving;   //离开状态后动作
    int StateMap[MAX_EVENT_NUM];
}FSM_STATE;

//状态机
typedef struct
{
    FSM_STATE StateList[MAX_STATE_NUM];     //状态列表
    int CurrentState;       //当前状态
}FSMSystem;

//状态机初始化
void FSMSystemInit(FSMSystem* fsm);

//注册状态动作
void FSMSystemRegisterState(FSMSystem* fsm,int state,pFsmAction before,pFsmAction run,pFsmAction after);

//注册触发动作
void FSMSystemRegistetEvent(FSMSystem* fsm,int state,int event,int newState);

//执行状态切换
void FSMSystemStateChange(FSMSystem* fsm,int event,void* arg);

//执行状态机
void FSMSystemRun(FSMSystem* fsm);
#endif

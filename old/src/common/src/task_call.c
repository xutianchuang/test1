#include "common_includes.h"
#include "os.h"
#include <string.h>

//定时器初始化
void TaskCallInit(TASK_CALL* taskList)
{
    OS_ERR err;
    List_Init(&taskList->TaskList);
    OSMemCreate(&taskList->TaskMempool,"Task List",taskList->Taskmem, MAX_TASK_CALL_NUM, sizeof(TASK_FUN_CALL), &err);
    if(err != OS_ERR_NONE)
    {
        while(1);
    }
}

//分配一个PDU
static TASK_FUN_CALL* MallocTaskCall(TASK_CALL* taskList)
{
    OS_ERR err;
    TASK_FUN_CALL *task_ptr = 0;
    task_ptr = (TASK_FUN_CALL*)OSMemGet(&taskList->TaskMempool,&err);
    if(err != OS_ERR_NONE)
        return (void*)0;
    return task_ptr;
}

//释放一个PDU
static void FreeTaskCall(TASK_CALL* taskList,TASK_FUN_CALL* t)
{
    OS_ERR err;
    OSMemPut(&taskList->TaskMempool,t,&err);
}

//设置一个实时任务
int TaskCallSet(TASK_CALL* taskList,pTaskFunCall fun,void*arg)
{
	TASK_FUN_CALL *t = MallocTaskCall(taskList);
    if(t == NULL)
        return INVAILD_TASK_CALL_ID;
    t->pFun = fun;
    t->arg = arg;
    t->run = true;
	t->exitFlag = false;
    if(!List_Push_Back(&taskList->TaskList,t))
    {
        FreeTaskCall(taskList,t);
        return INVAILD_TMR_ID;
    }
    return ((unsigned long)t - (unsigned long)taskList->Taskmem)/sizeof(TASK_FUN_CALL);
}

//停止一个实时任务
bool TaskCallStop(TASK_CALL* taskList,int id)
{
	if(id > sizeof(taskList->Taskmem)/sizeof(TASK_FUN_CALL))
       return false;
    TASK_FUN_CALL *tmr = (TASK_FUN_CALL *)((unsigned long)taskList->Taskmem + id * sizeof(TASK_FUN_CALL));
    tmr->run = false;
    return true;
}

//启动一个实时任务
bool TaskCallStart(TASK_CALL* taskList,int id)
{
	if(id > sizeof(taskList->Taskmem)/sizeof(TASK_FUN_CALL))
       return false;
    TASK_FUN_CALL *tmr = (TASK_FUN_CALL *)((unsigned long)taskList->Taskmem + id * sizeof(TASK_FUN_CALL));
    tmr->run = true;
    return true;
}

//当前执行的定时器
static TASK_FUN_CALL *CurrentTaskCall = NULL;

//定时器执行函数
void TaskCallRun(TASK_CALL* taskList)
{
    ListNode *node = taskList->TaskList.head;
    for(;node != NULL;/*node= node->next*/)
    {
        TASK_FUN_CALL *taskCall = (TASK_FUN_CALL *)node->data;
		CurrentTaskCall = taskCall;
		if(taskCall->run)
		{
			taskCall->pFun(taskCall->arg);
		}
		if(taskCall->exitFlag)
		{
			ListNode *nextNode = node->next;
			List_Del(&taskList->TaskList,node);
			FreeTaskCall(taskList,taskCall);
			node = nextNode;
			continue;
		}
		CurrentTaskCall = NULL;
		node = node->next;
    }
}

//在任务函数内部删除
bool TaskCallDel(void)
{
	if(CurrentTaskCall)
	{
		CurrentTaskCall->exitFlag = true;
	}
	return true;
}

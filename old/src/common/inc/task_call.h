#ifndef _TASK_CALL_H_
#define _TASK_CALL_H_

//实时执行的一些列函数


#define MAX_TASK_CALL_NUM     20

typedef void(*pTaskFunCall)(void*);

#define INVAILD_TASK_CALL_ID  0xFF
 
//定时回调函数结构体
#pragma pack(4)
typedef struct
{
    void *arg;           //参数
    pTaskFunCall pFun;   //定时器函数
    bool run;            //运行标志
	bool exitFlag;		  //退出标志
}TASK_FUN_CALL;


//定时器结构体
typedef struct
{
    List TaskList;			//定时器列表
    OS_MEM TaskMempool;		//
	TASK_FUN_CALL Taskmem[MAX_TMR_NUM];	//定时器资源池
}TASK_CALL;

//初始化
void TaskCallInit(TASK_CALL* taskList);

//设置一个实时任务
int TaskCallSet(TASK_CALL* taskList,pTaskFunCall fun,void*arg);

//停止一个实时任务
bool TaskCallStop(TASK_CALL* taskList,int);

//启动一个实时任务
bool TaskCallStart(TASK_CALL* taskList,int);

//在实时任务内部删除任务
bool TaskCallDel(void);

#endif

#ifndef _TMR_CALL_H_
#define _TMR_CALL_H_
#include "os.h"

//定时器回调函数

#define MAX_TMR_NUM     20

typedef void(*pTmrFunCall)(void*);

typedef enum
{
    TMR_OPT_CALL_ONCE,      //执行一次，然后把自己删除
    TMR_OPT_CALL_PERIOD,    //周期性执行
}TMR_OPT;

//定时回调函数结构体
#pragma pack(4)
typedef struct
{
    void *arg;          //参数,确保在第一个
    pTmrFunCall pFun;   //定时器函数
    TimeValue timer;    //定时器单元
    TMR_OPT opt;        //定时器操作
    u32 ms;             //定时器时间
    bool run;           //运行标志
}TMR_FUN_CALL;

//定时器结构体
typedef struct
{
    List TmrList;
    OS_MEM TmrMempool;
    TMR_FUN_CALL Tmrmem[MAX_TMR_NUM];
}TMR_CALL;

#define INVAILD_TMR_ID  0xFF
 

//定时器初始化
void TmrCallInit(TMR_CALL* tmr);

//设置一个定时回调函数(回调函数,参数,超时时间,模式),返回定时器标号
int TmrSet(TMR_CALL* tmrList,pTmrFunCall fun,void*arg,u32 ms,TMR_OPT opt);

//删除一个定时回调函数
bool TmrDel(TMR_CALL* tmrList,int);

//重置一个定时器时间
bool TmrReset(TMR_CALL* tmrList,int,u32 ms);

//停止一个定时器
bool TmrStop(TMR_CALL* tmrList,int);

//启动一个定时器
bool TmrStart(TMR_CALL* tmrList,int);

//定时器执行函数
void TmrRun(TMR_CALL* tmrList);

//在定时器函数内部停止删除定时器
void DeleteTmrSelf(void *arg);

//返回定时函数下一个定时器溢出间隔时间
u32 GetTmrLeftTime(TMR_CALL* tmrList,int);

#endif

#ifndef _TIMER_H_
#define _TIMER_H_

//定时器单元
typedef struct
{
    u32 BeginTimeStamp;         //原点时间戳
    u32 OverFlowTimeStamp;      //时间溢出值
}TimeValue;

//初始化一个定时器单元
void StartTimer(TimeValue *t, u32 mse);

//定时器单元是否已经溢出
bool TimeOut(TimeValue *t);

//定时器单元是否已经超过过此此时间段
bool TimeOver(TimeValue *t,u32 mse);

//定时器单元还剩多少溢出(0表示已经溢出)
u32  TimeLeft(TimeValue *t);

//重置time ,之后判断该定时器固定为溢出
void TimeStop(TimeValue *t);
#endif
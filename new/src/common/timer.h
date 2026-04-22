#ifndef _TIMER_H_
#define _TIMER_H_

//定时器单元
typedef struct
{
  u32 mse;      //时间溢出值
  bool carry;   //u32溢出指示位
}TimeValue;

//初始化一个定时器单元
void StartTimer(TimeValue *t, u32 mse);

//定时器单元是否已经溢出
bool TimeOut(TimeValue *t);

//定时器单元还剩多少溢出(0表示已经溢出)
u32  TimeLeft(TimeValue *t);
#endif
/*
*
*/
#ifndef __BSP_TIMER_H__
#define __BSP_TIMER_H__


#define BSP_TIMER_0            (0)
#define BSP_TIMER_1            (1)
#define BSP_TIMER_2            (2)
#define BSP_TIMER_3            (3)
#define BSP_TIMER_4            (4)
#define BSP_TIMER_5            (5)
#define BSP_TIMER_6            (6)
#define BSP_TIMER_7            (7)
#define BSP_TIMER_8            (8)
#define BSP_TIMER_9            (9)
#define BSP_TIMER_10           (10)//用于delay 不可被使用
#define BSP_TIMER_11           (11) //用于uart 不可被使用



#define BSP_TIMER_CHL_LENGTH    (BSP_TIMER_11+1)

//======
#define TIMER_SET_STOP        0
#define TIMER_SET_START       1

//======

typedef void (*TimerCallBackFunctionType)();
typedef struct {
    uint32_t Timer;
    uint32_t TimerSlot20ns;
    TimerCallBackFunctionType TimerCallBackFunction;
}TimerParameterTypdef;


void TimerOpen(TimerParameterTypdef *TimerParameter);
//mode = 1 start;mode = 0stop;other nodefine
void TimerWrite(uint32_t Timer,uint32_t mode);
uint32_t TimerRead(uint32_t Timer);
void TimerClose(uint32_t Timer);

void PwmInit(uint32_t Timer);
void PwmGpioFunc(uint32_t Timer,u8 isPwmFunc);
void TimerPwmOpen(TimerParameterTypdef *TimerParameter,uint32_t CmpTime20ns);
void TimerPwmReset(TimerParameterTypdef *TimerParameter);
void TimerPwmWrite(uint32_t Timer,uint32_t mode);


#endif





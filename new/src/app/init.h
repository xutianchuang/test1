#ifndef _INIT_H_
#define _INIT_H_


#define SYS_CLOCK_FRQ   25000000

#define NVIC_STCSR ((volatile unsigned long *)(0xE000E010))
#define NVIC_RELOAD ((volatile unsigned long *)(0xE000E014))
#define NVIC_CURRVAL ((volatile unsigned long *)(0xE000E018))
#define NVIC_CALVAL ((volatile unsigned long *)(0xE000E01C))
#define NVIC_PRIORITY ((volatile unsigned long *)(0xE000ED00 + 0x018 + 11))


//PHY任务堆栈大小
#define APP_CFG_TASK_PHY_STK_SIZE                  256

//BPLC发送任务堆栈大小
#define APP_CFG_TASK_BPLC_SEND_STK_SIZE              256        //128        //256

#define APP_CFG_TASK_BPLC_CSMA_SEND_STK_SIZE        384        //128        //256
//接收任务栈大小
#define APP_CFG_TASK_BPLC_REC_STK_SIZE              256        //128        //256
#define APP_CFG_TASK_BPLC_TIMER_STK_SIZE            256

//BPLC任务堆栈大小
#define APP_CFG_TASK_BPLC_STK_SIZE                  256


//本地通信口任务堆栈大小
#define APP_CFG_TASK_APPLOCAL_STK_SIZE              256
//本地通信任务优先级


#define  APP_PLC_AUTORELAY_STK_SIZE                 384     //256


#define  APP_END_STK_SIZE                           128
#define  APP_SYNC_NTB_STK_SIZE                      64
#define  APP_HRF_SEND_SIZE                          256

#define  APP_PLM_2005_STK_SIZE                      256

//PHY任务优先级
#define  APP_CFG_TASK_PHT_PRIO                       3u
//接收任务优先级
#define  APP_CFG_TASK_BPLC_REC_PRIO                  4u
//BPLC发送任务优先级
#define  APP_CFG_TASK_BPLC_SEND_PRIO                 5u
#define  APP_CFG_TASK_BPLC_CSMA_SEND_PRIO            6u
#define  APP_CFG_TASK_HRF_CSMA_SEND_PRIO             7u
#define  APP_CFG_TASK_HRF_SEND_PRIO                  8u
//BPLC任务优先级
#define  APP_CFG_TASK_MAC_PRIO                       9u
#define  APP_CFG_TASK_APPLOCAL_PRIO                  10u
#define  APP_PLC_AUTORELAY_PRIO                      11u
#define  APP_END_PRIO                                12u
#define  APP_PLM_2005_PRIO                           13u
#define  APP_TIMER_TASK_PRIO                         14u

//初始化代码
void  App_CreateTask (void *p_arg);

#endif

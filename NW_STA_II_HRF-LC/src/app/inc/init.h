#ifndef _INIT_H_
#define _INIT_H_


#define SYS_CLOCK_FRQ   25000000

#define NVIC_STCSR ((volatile unsigned long *)(0xE000E010))
#define NVIC_RELOAD ((volatile unsigned long *)(0xE000E014))
#define NVIC_CURRVAL ((volatile unsigned long *)(0xE000E018))
#define NVIC_CALVAL ((volatile unsigned long *)(0xE000E01C))
#define NVIC_PRIORITY ((volatile unsigned long *)(0xE000ED00 + 0x018 + 11))


//发送MAC帧任务堆栈大小
#define APP_CFG_TASK_BPLC_SENDMAC_STK_SIZE			256

//发送 bcsma MAC帧任务优先级



//BPLC发送MPDU任务堆栈大小
#define APP_CFG_TASK_BPLC_SEND_STK_SIZE              256
//BPLC发送MPDU任务优先级
//BPLC任务优先级
typedef enum
{
	APP_CFG_TASK_MAC_PRIO,
	APP_CFG_TASK_BPLC_SEND_PRIO,
	APP_CFG_TASK_BPLC_CSMA_SEND_PRIO,
	APP_CFG_TASK_BPLC_BCSMA_SEND_PRIO,
	APP_CFG_TASK_HRF_CSMA_SEND_PRIO,
	APP_CFG_TASK_HRF_BCSMA_SEND_PRIO,
	APP_CFG_TASK_BPLC_SENDMAC_PRIO,
	APP_CFG_TASK_BPLC_BCSMA_SENDMAC_PRIO,
	APP_CFG_TASK_HRF_SENDMAC_PRIO,
	APP_CFG_TASK_HRF_BCSMA_SENDMAC_PRIO,
	APP_CFG_TASK_EVENT_PRIO,
	APP_CFG_TASK_GRAPH_PRIO,
	APP_CFG_TASK_APPLOCAL_PRIO,

	APP_CFG_TASK_APPLCIP_PRIO = 13,
}TASK_PRIO;
//BPLC任务堆栈大小
#define APP_CFG_TASK_BPLC_STK_SIZE                  384


//本地通信口任务堆栈大小
#define APP_CFG_TASK_APPLOCAL_STK_SIZE              256
//本地通信任务优先级
#define NODE_REPORT_SIZE              256
#define OPERATEMETER_SIZE              1024

#define APP_CFG_TASK_APPLCIP_STK_SIZE              256


//事件上报任务堆栈大小
#define APP_CFG_TASK_EVENT_STK_SIZE                 256



//冻结曲线任务堆栈大小
#define APP_CFG_TASK_GRAPH_STK_SIZE                 256


//事件这主动上报任务优先级
#define APP_CFG_TASK_EVENT_UP_PRIO                  12u

#define APP_CFG_TASK_NEW_STK_SIZE                   256
#define APP_CFG_TASK_NEW_PRIO                       11u


//二采的任务栈
#define APP_CFG_TASK_II_STA_STK_SIZE                256
#define APP_CFG_TASK_II_STA_PRIO                    12u

//二采校时任务堆栈大小
#define APP_CFG_TASK_II_TIMING_STK_SIZE             256
//二采校时任务优先级
#define APP_CFG_TASK_II_TIMING_PRIO                 13u

//蓝牙任务
#define APP_CFG_TASK_BLE_STA_STK_SIZE				256
#define APP_CFG_TASK_BLE_STA_PRIO                   13u

//初始化代码
void  App_CreateTask (void *p_arg);
void GetReadMeter(void);

//void SyncNtbTask(void *arg);
#endif

#ifndef _APP_LED_IICAI_H
#define _APP_LED_IICAI_H

#include "board_sta_def.h"

#ifdef RUN_LED_PORT
#define RUN_LED_ON GPIO_PinWrite(RUN_LED_PORT, RUN_LED_PIN, 1)	// 运行灯开，红色
#define RUN_LED_OFF GPIO_PinWrite(RUN_LED_PORT, RUN_LED_PIN, 0) // 运行灯关，红色
#else
#define RUN_LED_ON
#define RUN_LED_OFF
#endif

#ifdef PLC_LED_PORT
#define PLC_LED_ON GPIO_PinWrite(PLC_LED_PORT, PLC_LED_PIN, 1)	// 载波通信指示灯开，绿色
#define PLC_LED_OFF GPIO_PinWrite(PLC_LED_PORT, PLC_LED_PIN, 0) // 载波通信指示灯关，绿色
#else
#define PLC_LED_ON
#define PLC_LED_OFF
#endif

#ifdef RS485_LED_PORT
#define RS485_LED_ON GPIO_PinWrite(RS485_LED_PORT, RS485_LED_PIN, 1)  // RS485通信指示灯开，红色
#define RS485_LED_OFF GPIO_PinWrite(RS485_LED_PORT, RS485_LED_PIN, 0) // RS485通信指示灯关，红色
#else
#define RS485_LED_ON
#define RS485_LED_OFF
#endif

#define LED_SHOW_TIME_10ms  10

#define LED_PLC              1 //
#define LED_RS485            2 //
#define LED_IFR              3 //

#define LED_ON               1
#define LED_OFF              0

#define LED_LIGHT_TIME       500 //ms
#define PLC_LED_LIGHT_TIME   500//750 //ms

#pragma pack(1)//字节对齐
typedef struct ledshow{	
	BYTE PLC_DataReceivingAndSending:1; //OK正在发送或接收载波数据,NO没有发送载波数据
	BYTE PLC_time:7;                   //计时器
}LEDSHOW;
#pragma pack()


void LED_Init(void);
void run_led_light(void);
void run_led_ctrl_fun(void);
void rs485_recving_flag_set_fun(uint8_t flag);
void rs485_sending_flag_set_fun(uint8_t flag);
void rs485_led_ctrl_fun(void);
void hplc_rec_send_flag_set_fun(void);
void plc_led_ctrl_fun(void);
void LedShowTimeStart(void);
void II_STA_LedControl(void);

#endif

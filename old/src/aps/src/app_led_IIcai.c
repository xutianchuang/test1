#include "protocol_includes.h"
#include "os.h"
#include "common_includes.h"
#include <string.h>
#include "bsp.h"
#include "Revision.h"
#include "hplc_data.h"
#include "app_search_meter.h"
#include "app_event_IIcai.h"
#include "app_led_IIcai.h"
#include "timer.h"
#include "bps_timer.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//add by LiHuaQiang 2020.10.13 -START-
//----------------------------------------------------------------
//声明
extern int power;
extern bool IsBindAddr;

//变量定义
uint16_t run_led_20s_count=OFF;
uint8_t rs485_recving_flag=OFF;//非搜表过程中,ON:在接收485数据 OFF:接收485数据结束
uint8_t rs485_sending_flag=OFF;//非搜表过程中,ON:在发送485数据 OFF:发送485数据结束
LEDSHOW LED_State;

TimeValue LedShowTime;

TimerParameterTypdef TimerLED;

void LED_Init(void)
{
#if defined(RUN_LED_PORT) && defined(PLC_LED_PORT) && defined(RS485_LED_PORT)
    GPIO_SetPinDir(RUN_LED_PORT, RUN_LED_PIN, GPIO_OUTPUT);
    PORT_PinConfigure(RUN_LED_PORT, RUN_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
    GPIO_PinWrite(RUN_LED_PORT, RUN_LED_PIN, 0);

    GPIO_SetPinDir(PLC_LED_PORT, PLC_LED_PIN, GPIO_OUTPUT);
    PORT_PinConfigure(PLC_LED_PORT, PLC_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
    GPIO_PinWrite(PLC_LED_PORT, PLC_LED_PIN, 0);

    GPIO_SetPinDir(RS485_LED_PORT, RS485_LED_PIN, GPIO_OUTPUT);
    PORT_PinConfigure(RS485_LED_PORT, RS485_LED_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
    GPIO_PinWrite(RS485_LED_PORT, RS485_LED_PIN, 0);

    RUN_LED_OFF;
    PLC_LED_OFF;
	RS485_LED_OFF;	
#endif	
}

void run_led_100ms_shine(void)//运行灯间隔0.1s快闪
{
    static uint8_t run_led_state=0;
    static uint8_t run_led_count=0;
	
	//if(!GetFirstCheckTableFlag())//不是第一次搜表，搜表时运行灯不快闪
	//	return;
	
    run_led_count++;
    if(run_led_count>=10)
    {        
        run_led_count=0;
        if(run_led_state)
            RUN_LED_ON;  
        else
            RUN_LED_OFF;
        run_led_state = ~ run_led_state;
    }
}
void run_led_1000ms_shine(void)//运行灯间隔1s慢闪
{
    static uint8_t run_led_state=0;
    static uint8_t run_led_count=0;
    run_led_count++;
    if(run_led_count>=100)
    {        
        run_led_count=0;
        if(run_led_state)
            RUN_LED_ON;
        else
            RUN_LED_OFF;
        run_led_state = ~ run_led_state;
    }
}
static uint8_t exc_one_time=0;
void run_led_light(void)
{
    if(SEARCH_METER_FLAG.meter_total!=0)
    {         
        if(run_led_20s_count ==OFF && exc_one_time==0)
        {
            RUN_LED_ON;
            run_led_20s_count=ON;
            exc_one_time=1;
            //print_debug("run led ON,20s start",0,0);
        }
    }
}
//运行灯控制函数,10ms执行1次
void run_led_ctrl_fun(void)
{
#if 0
    if(IsPowerOff == 1 && power == 0)//停电时运行灯长亮
    {
        RUN_LED_ON;
        return;
    }
    if(run_led_20s_count >= ON)
    {
        if(run_led_20s_count++ >= 2001)
        {
            run_led_20s_count=OFF;
            //print_debug("run led ON,20s end",0,0);
        }
    }
    else if(GetCheckTableFlag() && !GetSerach645OverFlag())//全A搜表阶段
    {
        if(SEARCH_METER_FLAG.meter_total == 0)//未搜到电表，快闪
            run_led_100ms_shine();
        else//搜到电表长亮20秒
        {
            if(run_led_20s_count ==OFF && exc_one_time==0)
            {
                RUN_LED_ON;
                run_led_20s_count=ON;
                exc_one_time=1;
                //print_debug("1.run led ON,20s start",0,0);
            }
        }
    }
    else if(GetCheckTableFlag() && GetSerach645OverFlag())//缩位寻址阶段
    {
        //if(meter_total == 0)//未搜到电表，快闪
            run_led_100ms_shine();
         /*else//搜到电表长亮20秒
	        {
	            if(run_led_20s_count ==OFF && exc_one_time==0)
	            {
	                RUN_LED_ON;
	                run_led_20s_count=ON;
	                exc_one_time=1;
	                //print_debug("2.run led ON,20s start",0,0);
	            }
	            else
	                run_led_100ms_shine();
	        }*/
    }
    else if(!GetCheckTableFlag())//搜表结束
    {
#ifdef PROTOCOL_NW_2021
		run_led_1000ms_shine();
#else
        if(SEARCH_METER_FLAG.meter_total == 0)//未搜到电表，慢闪
            run_led_1000ms_shine();
        else//搜到电表长亮
            RUN_LED_ON;
#endif
    }
#endif
    if(GetCheckTableFlag() == true)
	{
		run_led_100ms_shine();
	}
	else
	{
		run_led_1000ms_shine();
	}
}
//---------------------------------------------------------RS485通信指示灯控制相关函数------------------------------------------------------------------------
void rs485_led_100ms_shine(void)//间隔0.1s快闪
{
    static uint8_t run_led_state=0;
    static uint8_t run_led_count=0;
    run_led_count++;
    if(run_led_count>=10)
    {        
        run_led_count=0;
        if(run_led_state)
            RS485_LED_ON;  
        else
            RS485_LED_OFF;
        run_led_state = ~ run_led_state;
    }
}
void rs485_recving_flag_set_fun(uint8_t flag)//接收完成或者接收超时，rs485_recving_flag=OFF
{
    if(!GetCheckTableFlag())//非搜表过程中    
        rs485_recving_flag=flag;
}
void rs485_sending_flag_set_fun(uint8_t flag)
{
    if(!GetCheckTableFlag())//非搜表过程中    
        rs485_sending_flag=flag;
    else if(GetCheckTableFlag() && IsPowerOff == 1 && power == 0)//停电时确认电表是否正常
        rs485_sending_flag=flag;
}
//rs485灯控制函数,10ms执行1次
void rs485_led_ctrl_fun(void)
{
    if(rs485_recving_flag == ON || rs485_sending_flag == ON)//非搜表过程中，485正在发送或接收数据时快速闪烁
        rs485_led_100ms_shine();
    else
        RS485_LED_OFF;
}
//---------------------------------------------------------PLC通信指示灯控制相关函数------------------------------------------------------------------------
void plc_led_100ms_shine(void)//间隔0.1s快闪
{
    static uint8_t run_led_state=0;
    static uint8_t run_led_count=0;
    run_led_count++;
    if(run_led_count>=10)
    {        
        run_led_count=0;
        if(run_led_state)
            PLC_LED_ON;  
        else
            PLC_LED_OFF;
        run_led_state = ~ run_led_state;
    }
}
void plc_led_1000ms_shine(void)//间隔1s慢闪
{
    static uint8_t run_led_state=0;
    static uint8_t run_led_count=0;
    run_led_count++;
    if(run_led_count>=100)
    {        
        run_led_count=0;
        if(run_led_state)
            PLC_LED_ON;
        else
            PLC_LED_OFF;
        run_led_state = ~ run_led_state;
    }
}
void hplc_rec_send_flag_set_fun(void)
{
    LED_State.PLC_DataReceivingAndSending = OK;
    LED_State.PLC_time=0;
    PLC_LED_OFF;
}
//plc灯控制函数,10ms执行1次
void plc_led_ctrl_fun(void)
{
    if(IsBindAddr && GetStaBaseAttr()->NetState != NET_IN_NET)
        plc_led_1000ms_shine();    
    else if(GetStaBaseAttr()->NetState == NET_IN_NET)
    {
        if(LED_State.PLC_DataReceivingAndSending == OK)
        {
            LED_State.PLC_time++;
            if(LED_State.PLC_time <= 10)
                PLC_LED_ON;
            else if(LED_State.PLC_time <= 20)
                PLC_LED_OFF;
            else if(LED_State.PLC_time <= 30)
                PLC_LED_ON;
            else

            {
                PLC_LED_OFF;
                LED_State.PLC_time=0;
                LED_State.PLC_DataReceivingAndSending=NO;
            }           
        }        
        else
            PLC_LED_OFF;
    }
}
void LED_TimerCallBack(void)
{
	run_led_ctrl_fun();
//南网21标准，II采状态红绿双色灯由FlashLedTimer实现
#ifndef PROTOCOL_NW_2021 	
	rs485_led_ctrl_fun();
	plc_led_ctrl_fun();
#endif	
}

void LedShowTimeStart(void)
{
	TimerLED.Timer = LED_TIMER;
    TimerLED.TimerSlot20ns = 50*1000*10;  //10ms
    TimerLED.TimerCallBackFunction = LED_TimerCallBack;
    TimerOpen(&TimerLED);
    TimerWrite(TimerLED.Timer,TIMER_SET_START);
}
//----------------------------------------------------------------
//add by LiHuaQiang 2020.10.13 -END-
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------

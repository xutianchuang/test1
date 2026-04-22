#include  <os.h>
#include <stdio.h>
#include "common_includes.h"
#include "hplc_receive.h"
#include "bps_led.h"
#include "System_inc.h"
#include "manager_link.h"
#include "PLC_AutoRelay.h"
#include "uart_link.h"
#include "PLM_2005.h"
#include "hrf_port.h"
#include "bps_debug.h"
#include "bps_led.h"


u8 init_finish = false;
void BaseInit(void)
{
    TX_LED_ON();
    RX_LED_ON();
	if (CCO_Hard_Type)
	{
		PA_LED_ON();
		PB_LED_ON();
		PC_LED_ON();
	}

    //CCO_CHG_DIS();//关闭充电
    //PLC_Init();    
    //PLL_S_Init();
#if DEV_CCO
    SYS_STOR_Init(NULL);
    End_init();

	init_finish = true;
	HRF_Reset();//复位HRF重新同步参数
#endif
}

#define LED_SNID_FLAG		  (1<<0)
#define LED_FREQ_FLAG		  (1<<1)
#define LED_SILENCE_FLAG      (1<<2)

extern u8 FlashSendMode;

//闪灯逻辑逻辑，500毫秒调用一次
void LedFlashLogic(u32 flag, u8 times)
{
	static u8 LedRed = 0;
	static u8 LedGreen = 0;
	u8 mode = 0;
	
	if ((flag & LED_SNID_FLAG) == LED_SNID_FLAG)
	{
		mode = times%2; //500毫秒切换
		LedWrite(GREEN_LED, mode);
		FlashSendMode = mode;
	}
	else if ((flag & LED_FREQ_FLAG) == LED_FREQ_FLAG) 
	{
		mode = ((times-1)/4 + 1)%2; //2秒切换
		LedWrite(GREEN_LED, mode);
		FlashSendMode = mode;
	}
	else if ((flag & LED_SILENCE_FLAG) == LED_SILENCE_FLAG) 
	{
		LedWrite(GREEN_LED, 0);
		FlashSendMode = 0;
	}
	else 
	{
		LedWrite(GREEN_LED, (LedGreen + 1) % 2);
		LedRed = LedGreen;
		LedWrite(RED_LED, LedRed);
		LedGreen = (LedGreen + 1) % 2;
	}
}

u8 LED_Time=0;

void  App_CreateTask (void *p_arg)
{
	OS_ERR  os_err;
	u16 run_time = 0;
	u8 run_len = 0;
	#ifdef PROTOCOL_NW_2021
	u8 nid_times = 0, freq_times = 0, silence_times = 0;
	u32 led_flag = 0, led_cnt = 0;
	#endif
#if DEV_CCO
	ManagerLinkInit();
#endif
	//调用底层初始化
	BSP_Init();

#if 0
	u32 i = 0;
	while(1)
	{
		OSTimeDly(1000, OS_OPT_TIME_DLY, &os_err);

		PA_LED_OFF();
		PB_LED_OFF();
		PC_LED_OFF();

		if(0==(i%3))
		PA_LED_ON();
		else if(1==(i%3))
		PB_LED_ON();
		else if(2==(i%3))
		PC_LED_ON();

		i++;
	}
#endif

	BaseInit();

	DebugOpen(115200,DebugSendComplate,DebugGetMsg);

	CommonInit();
	HPLC_ProtocolInit(g_pPlmConfigPara->hrf_option, g_pPlmConfigPara->hrf_channel);
	//DebugOpen(115200);

	/*
	 OSTaskCreate(&App_PHY_TCB,
			 "PHY Task 0",
			  PHY_Task,
			  0,
			  APP_CFG_TASK_PHT_PRIO,
			  &App_PhyStk[0],
			  APP_CFG_TASK_PHY_STK_SIZE / 10u,
			  APP_CFG_TASK_PHY_STK_SIZE,
			  0u,
			  0u,
			  0,
			 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
			 &os_err);
*/


	OSTaskCreate(&g_pOsRes->App_Bplc_Rec_TCB,
				 "BPLC Recdive Task",
				 HPLC_ReveiveTask,
				 0,
				 APP_CFG_TASK_BPLC_REC_PRIO,
				 g_pOsRes->App_BplcRecStk,
				 APP_CFG_TASK_BPLC_REC_STK_SIZE / 10u,
				 APP_CFG_TASK_BPLC_REC_STK_SIZE,
				 20u,
				 0u,
				 0,
				 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 &os_err);

	OSTaskCreate(&g_pOsRes->App_Bplc_Timer_TCB,
				 "BPLC Timer Task",
				 HPLC_TimerTask,
				 0,
				 APP_TIMER_TASK_PRIO,
				 g_pOsRes->App_BplcTimerStk,
				 APP_CFG_TASK_BPLC_TIMER_STK_SIZE / 10u,
				 APP_CFG_TASK_BPLC_TIMER_STK_SIZE,
				 20u,
				 0u,
				 0,
				 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 &os_err);

	OSTaskCreate(&g_pOsRes->App_Bplc_Send_TCB,
				 "BPLC Send Task",
				 HPLC_SendTask,
				 0,
				 APP_CFG_TASK_BPLC_SEND_PRIO,
				 &g_pOsRes->App_BplcSendStk[0],
				 APP_CFG_TASK_BPLC_SEND_STK_SIZE / 10u,
				 APP_CFG_TASK_BPLC_SEND_STK_SIZE,
				 20u,
				 0u,
				 0,
				 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 &os_err);
	char *name = "HRF CSMA Task";
	char *bname = "HRF BCSMA Task";
	name += 4;
	bname += 4;


	OSTaskCreate(&g_pOsRes->App_Bplc_csmaSend_TCB[0],
				 name,
				 HPLC_SendCsmaTask,
				 0,
				 APP_CFG_TASK_BPLC_CSMA_SEND_PRIO,
				 g_pOsRes->App_BplcCsmaSendStk[0],
				 APP_CFG_TASK_BPLC_CSMA_SEND_STK_SIZE / 10u,
				 APP_CFG_TASK_BPLC_CSMA_SEND_STK_SIZE,
				 0u,
				 0u,
				 0,
				 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 &os_err);
	OSTaskCreate(&g_pOsRes->App_Bplc_BcsmaSend_TCB[0],
				 bname,
				 HPLC_SendBCsmaTask,
				 0,
				 APP_CFG_TASK_BPLC_CSMA_SEND_PRIO,
				 g_pOsRes->App_BplcBcsmaSendStk[0],
				 APP_CFG_TASK_BPLC_CSMA_SEND_STK_SIZE / 10u,
				 APP_CFG_TASK_BPLC_CSMA_SEND_STK_SIZE,
				 0u,
				 0u,
				 0,
				 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 &os_err);

	name -= 4;
	bname -= 4;
	OSTaskCreate(&g_pOsRes->App_Bplc_csmaSend_TCB[1],
				 name,
				 HRF_SendCsmaTask,
				 0,
				 APP_CFG_TASK_HRF_CSMA_SEND_PRIO,
				 g_pOsRes->App_BplcCsmaSendStk[1],
				 APP_CFG_TASK_BPLC_CSMA_SEND_STK_SIZE / 10u,
				 APP_CFG_TASK_BPLC_CSMA_SEND_STK_SIZE,
				 0u,
				 0u,
				 0,
				 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 &os_err);
	OSTaskCreate(&g_pOsRes->App_Bplc_BcsmaSend_TCB[1],
				 bname,
				 HRF_SendBCsmaTask,
				 0,
				 APP_CFG_TASK_HRF_CSMA_SEND_PRIO,
				 g_pOsRes->App_BplcBcsmaSendStk[1],
				 APP_CFG_TASK_BPLC_CSMA_SEND_STK_SIZE / 10u,
				 APP_CFG_TASK_BPLC_CSMA_SEND_STK_SIZE,
				 0u,
				 0u,
				 0,
				 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 &os_err);


#if DEV_CCO
	OSTaskCreate(&g_pOsRes->APP_PLM_2005_TCB,
				 "PLM Task Proc",
				 PLM_Task_Proc,
				 0,
				 APP_PLM_2005_PRIO,
				 &g_pOsRes->APP_PLM_2005_stk[0],
				 APP_PLM_2005_STK_SIZE / 10u,
				 APP_PLM_2005_STK_SIZE,
				 15,       //q_size
				 0u,
				 0,
				 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 &os_err);

	OSTaskCreate(&g_pOsRes->APP_End_TCB,
				 "End Task Proc",
				 End_Task_Proc,
				 0,
				 APP_END_PRIO,
				 &g_pOsRes->APP_End_stk[0],
				 APP_END_STK_SIZE / 10u,
				 APP_END_STK_SIZE,
				 END_TASK_MSG_NUM,       //q_size
				 0u,
				 0,
				 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 &os_err);

	OSTaskCreate(&g_pOsRes->APP_PLC_Autorelay_TCB,
				 "Task Plc proc",
				 Task_Plc_proc,
				 0,
				 APP_PLC_AUTORELAY_PRIO,
				 &g_pOsRes->APP_PLC_Autorelay_stk[0],
				 APP_PLC_AUTORELAY_STK_SIZE / 5u,
				 APP_PLC_AUTORELAY_STK_SIZE,
              PLC_MAX_PARALLEL_NUM,       //q_size
				 0u,
				 0,
				 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 &os_err);

#endif
#if OS_CFG_STAT_TASK_EN > 0u
	OSStatTaskCPUUsageInit(&os_err);                               /* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
	CPU_IntDisMeasMaxCurReset();
#endif




	int bplc_nbi_cnt=0;
	while (1)
	{
		FeedWdg();


		OSTimeDlyHMSM(0, 0, 0, 20, OS_OPT_TIME_DLY, &os_err);
		bplc_nbi_cnt++;
		if (bplc_nbi_cnt>50)//每s探测窄带噪声
		{
			BPLC_NbiDet();
			bplc_nbi_cnt=0;
#if 0
//CCO其实也不需要控核心电压			
			#ifndef ZB204_CHIP
			int tc=BPLC_TC_Get();
			debug_str(DEBUG_LOG_APS,"Temperature:%d\r\n",tc);
			if(tc<10)
			{
				SystemSetCoreDcdc(1);
			}
			else
			{
				SystemSetCoreDcdc(0);
			}
			#endif
#endif
		}
		if (BootPowerState == 0) //停电不发万年历信标
		{
			g_PlmStatusPara.t_now_sec = 0;
			g_PlmStatusPara.t_gettime_sec = 0;
		}
		if (run_time++ > 50)
		{
			LedRunWrite(run_len);
			run_time = 0;
			run_len = !run_len;
		}
		Debug_AutoLog();
#ifdef PROTOCOL_NW_2021
		if (init_finish)
		{
			led_cnt++;

			if (silence_times == 0) //赋初值
			{
				nid_times = g_pPlmConfigPara->net_id * 2;
				freq_times = (g_pPlmConfigPara->hplc_comm_param + 1) * 8;
				silence_times = 10;
			}

			if (led_cnt % 25 == 0) //500毫秒一次
			{
				u32 times = led_cnt / 25;

				if (times <= nid_times)
				{
					led_flag = LED_SNID_FLAG;
				}
				else if (times <= (nid_times + silence_times))
				{
					led_flag = LED_SILENCE_FLAG;
				}
				else if (times <= (nid_times + silence_times + freq_times))
				{
					led_flag = LED_FREQ_FLAG;
				}
				else if (times <= (nid_times + silence_times + freq_times + silence_times))
				{
					led_flag = LED_SILENCE_FLAG;
				}
				else
				{
					led_cnt = 0;
				}

				LedFlashLogic(led_flag, times);
			}
		}
#endif

		LED_Time++;
		if (CCO_Hard_Type)
		{
			if (LED_Time < 25)
			{
				PB_LED_ON();
				PA_LED_ON();
				PC_LED_ON();
			}
			else
			{
				if (LED_Time < 50)
				{
					PB_LED_OFF();
					PA_LED_OFF();
					PC_LED_OFF();
				}
				else
				{
					LED_Time = 51;
				}

			}
		}
		#ifdef __DEBUG_MODE
		static int debug_time=0;
		if(debug_time==50)
		{
			debug_str(0xffffffff,"debug mode\r\n");
			debug_time=0;
		}
		++debug_time;
		#endif
        
#ifndef NW_TEST
        extern u8 g_config_power;  //1：用户配置功率
        extern u8 g_config_hrf_power;  //1：用户配置功率
        
        if(g_pRelayStatus->innet_num> 5 && ZC_Time[0] !=0 )
        {         
            if(!g_config_power)
            {
              	u8 txPower = HPLC_HighPower[BPLC_TxFrequenceGet()];
				if(txPower > BPLC_GetTxGain() )
				{
				  BPLC_SetTxGain(txPower);
				  
//				  debug_str(DEBUG_LOG_NET, "PLC_change_power %d\r\n",txPower);
				}
            }
            
            if(!g_config_hrf_power)
            {
              	u8 txHrfPower = HRF_HighLinePower;
				if(  txHrfPower > HRF_GetTxGain())
				{
					HRF_SetTxGain(txHrfPower);
				  
//					debug_str(DEBUG_LOG_NET, "HRF_change_power %d\r\n",txHrfPower);
				}
            }
		}
#endif       
        HrfCheckLockParam();
	}
}

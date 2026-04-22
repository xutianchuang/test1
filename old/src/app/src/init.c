#include  <os.h>
#include <stdio.h>
#include "protocol_includes.h"
#include "common_includes.h"
#include "shell.h"
#include "app_new_task.h"
//HPLC接收处理任务
OS_TCB  App_BplcTCB;
CPU_STK  App_BplcStk[APP_CFG_TASK_BPLC_STK_SIZE];

//本地串口处理任务
OS_TCB  App_AppLocalTCB;
CPU_STK  App_AppLocalStk[APP_CFG_TASK_APPLOCAL_STK_SIZE];

//二采任务
#ifdef II_STA
OS_TCB  App_II_STA_TCB;
CPU_STK  App_II_STA_Stk[APP_CFG_TASK_II_STA_STK_SIZE];

//二采校时任务
OS_TCB  App_II_TimingTCB;
CPU_STK  App_II_TimingStk[APP_CFG_TASK_GRAPH_STK_SIZE];
#endif

//HPLC的CSMA冲突检测，空闲发送任务
OS_TCB  App_Bplc_Send_TCB;
CPU_STK  App_BplcSendStk[APP_CFG_TASK_BPLC_SEND_STK_SIZE];

OS_TCB  App_Bplc_CSMA_Send_TCB;
CPU_STK  App_BplcCsmaSendStk[APP_CFG_TASK_BPLC_SEND_STK_SIZE];

OS_TCB  App_Bplc_BCSMA_Send_TCB;
CPU_STK  App_BplcBcsmaSendStk[APP_CFG_TASK_BPLC_SEND_STK_SIZE];

OS_TCB  App_Hrf_CSMA_Send_TCB;
CPU_STK  App_HrfCsmaSendStk[APP_CFG_TASK_BPLC_SEND_STK_SIZE];

OS_TCB  App_Hrf_BCSMA_Send_TCB;
CPU_STK  App_HrfBcsmaSendStk[APP_CFG_TASK_BPLC_SEND_STK_SIZE];

//HPLC的MAC层处理MAC发送队列，拆包解包任务
OS_TCB  App_Bplc_SendMac_TCB;
CPU_STK  App_BplcSendMacStk[APP_CFG_TASK_BPLC_SENDMAC_STK_SIZE];
//HPLC的MAC层处理MAC 绑定CSMA发送队列，拆包解包任务
OS_TCB  App_Bplc_BcsmaSendMac_TCB;
CPU_STK  App_BplcBcsmaSendMacStk[APP_CFG_TASK_BPLC_SENDMAC_STK_SIZE];

OS_TCB  App_Hrf_SendMac_TCB;
CPU_STK  App_HrfSendMacStk[APP_CFG_TASK_BPLC_SENDMAC_STK_SIZE];
//HPLC的MAC层处理MAC 绑定CSMA发送队列，拆包解包任务
OS_TCB  App_Hrf_BcsmaSendMac_TCB;
CPU_STK  App_HrfBcsmaSendMacStk[APP_CFG_TASK_BPLC_SENDMAC_STK_SIZE];

//事件上报任务
OS_TCB  App_EventTCB;
CPU_STK  App_EventStk[APP_CFG_TASK_EVENT_STK_SIZE];




//曲线数据存储任务
OS_TCB  App_GraphTCB;
CPU_STK  App_GraphStk[APP_CFG_TASK_GRAPH_STK_SIZE];


#define LED_BIND_ADDR_FLAG		(1<<0)
#define LED_IN_NET_FLAG			(1<<1)
#define LED_CONNECT_READER      (1<<3)
//闪灯逻辑逻辑
void LedFlashLogic(u32 flag)
{
#if !defined(II_STA)||defined(I_STA)
	static u8 LedRed = 0;
	static u8 LedGreen = 0;
	static u8 ReaderCnt = 0;
	//const ST_STA_SYS_TYPE *psSysType = GetStaSysPara();
	
	if ((flag & LED_BIND_ADDR_FLAG) != LED_BIND_ADDR_FLAG)
	{
		if (StaFixPara.ledMode)
		{
#if defined(PROTOCOL_NW_2021)&&!defined(I_STA)
			//远程绿灯
			LedWrite(GREEN_LED, 1);
			LedGreen = 1;

			//本地红灯
			LedWrite(RED_LED, 0);
			LedRed = 0;
#else
			LedWrite(GREEN_LED, 1);
			LedGreen = 1;
			
			LedWrite(RED_LED, 1);
			LedRed = 1;
#endif			
		}
		else
		{
			LedWrite(GREEN_LED, (LedGreen + 1) % 2);
			LedRed = LedGreen;
			LedWrite(RED_LED, LedRed);
			LedGreen = (LedGreen + 1) % 2;
		}
	}
	else if ((flag & LED_CONNECT_READER) != LED_CONNECT_READER) //连接抄控器
	{
		if (LedGreen)
		{
			LedWrite(GREEN_LED, 0);
			LedGreen = 0;
		}
		if ((ReaderCnt++) % 3 == 0)
		{
			LedWrite(RED_LED, (LedRed + 1) % 2);
			LedRed = (LedRed + 1) % 2;
		}
	}
	else if ((flag & LED_IN_NET_FLAG) != LED_IN_NET_FLAG)
	{
		if (StaFixPara.ledMode)
		{
#if defined(PROTOCOL_NW_2021)&&!defined(I_STA)	
			//远程绿灯
			LedWrite(GREEN_LED, 1);
			LedGreen = 1;

			//本地红灯
			LedWrite(RED_LED, 1);
			LedRed = 1;
#else

			LedWrite(GREEN_LED, 0);
			LedWrite(RED_LED, 1);
			LedRed = 1;
#endif
		}
		else
		{
			if (LedGreen)
			{
				LedWrite(GREEN_LED, 0);
				LedGreen = 0;
			}
			LedWrite(RED_LED, (LedRed + 1) % 2);
			LedRed = (LedRed + 1) % 2;
		}
	}
	else
	{
		if (LedRed)
		{
#if defined(PROTOCOL_NW_2021)&&!defined(I_STA)
			//本地红灯
			LedWrite(RED_LED, 1);
			LedRed = 1;
#else
			LedWrite(RED_LED, 0);
			LedRed = 0;
#endif
		}
		if (LedGreen)
		{
			LedWrite(GREEN_LED, 0);
			LedGreen = 0;
		}
	}
#endif
}

#define CAP_CHARGE_TIME     (12*60*1000)
void CapEnCtrFun(void)
{
	static u8 charge_flag = 0;//1：为已充电12分钟，进入2分钟开，2分钟关流程
	static u8 charge = 1;
	static u32 change_tick = 0;
	OS_ERR err;
	u32 currentTick = OSTimeGet(&err);
	
	if(charge_flag)
	{
		if((currentTick - change_tick) >= (2*60*1000))
		{
			change_tick = currentTick;
			charge = !charge;
			CapEnCtrWrite(charge);
		}
	}
	else
	{
		if(currentTick > CAP_CHARGE_TIME)
		{
			change_tick = currentTick;
			charge_flag = 1;
			charge = 0;
			CapEnCtrWrite(0);
		}
	}
}

#ifdef NW_TEST
u8 joined=0;
int read_meter_time=0;
#endif
u8 fixed_max_txgain = 0; //南网深化应用机台 调大功率并需要保持
#ifdef LOW_POWER
u8 join = 0;
#endif
void  App_CreateTask (void *p_arg)
{
    u32 ledCnt = 0;
    OS_ERR  os_err;
	int bplc_nbi_cnt=0;
	
#if OS_CFG_STAT_TASK_EN > 0u
	OSStatTaskCPUUsageInit(&os_err);                               /* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
	CPU_IntDisMeasMaxCurReset();
#endif   
 
    //调用底层初始化
    BSP_Init();
	DebugOpen(115200);
    CommonInit();
    HPLC_ProtocolInit();
    //finsh_system_init();

	
    OSTaskCreate(&App_BplcTCB,
             "HPLC Task 0",
              HPLC_Task,
              0,
              APP_CFG_TASK_MAC_PRIO,
              &App_BplcStk[0],
              APP_CFG_TASK_BPLC_STK_SIZE / 10u,
              APP_CFG_TASK_BPLC_STK_SIZE,
              0u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err); 


    OSTaskCreate(&App_AppLocalTCB,
             "APPLOCAL Task 0",
              AppLocalTask,
              0,
              APP_CFG_TASK_APPLOCAL_PRIO,
              &App_AppLocalStk[0],
              APP_CFG_TASK_APPLOCAL_STK_SIZE / 10u,
              APP_CFG_TASK_APPLOCAL_STK_SIZE,
              0u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err); 

	
     OSTaskCreate(&App_Bplc_Send_TCB,
             "BPLC Send Task",
              HPLC_SendTask,
              0,
              APP_CFG_TASK_BPLC_SEND_PRIO,
              &App_BplcSendStk[0],
              APP_CFG_TASK_BPLC_SEND_STK_SIZE / 10u,
              APP_CFG_TASK_BPLC_SEND_STK_SIZE,
              0u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err);  
	 OSTaskCreate(&App_Bplc_CSMA_Send_TCB,
             "BPLC Send CSMA Task",
              HPLC_CSMA_SendTask,
              0,
              APP_CFG_TASK_BPLC_CSMA_SEND_PRIO,
              &App_BplcCsmaSendStk[0],
              APP_CFG_TASK_BPLC_SEND_STK_SIZE / 10u,
              APP_CFG_TASK_BPLC_SEND_STK_SIZE,
              0u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err);
	 OSTaskCreate(&App_Bplc_BCSMA_Send_TCB,
             "BPLC Send BCSMA Task",
              HPLC_BCSMA_SendTask,
              0,
              APP_CFG_TASK_BPLC_BCSMA_SEND_PRIO,
              &App_BplcBcsmaSendStk[0],
              APP_CFG_TASK_BPLC_SEND_STK_SIZE / 10u,
              APP_CFG_TASK_BPLC_SEND_STK_SIZE,
              0u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err);
	 
	 OSTaskCreate(&App_Hrf_CSMA_Send_TCB,
             "Hrf Send CSMA Task",
              HRF_CSMA_SendTask,
              0,
              APP_CFG_TASK_HRF_CSMA_SEND_PRIO,
              &App_HrfCsmaSendStk[0],
              APP_CFG_TASK_BPLC_SEND_STK_SIZE / 10u,
              APP_CFG_TASK_BPLC_SEND_STK_SIZE,
              0u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err);
	 OSTaskCreate(&App_Hrf_BCSMA_Send_TCB,
             "Hrf Send BCSMA Task",
              HRF_BCSMA_SendTask,
              0,
              APP_CFG_TASK_HRF_BCSMA_SEND_PRIO,
              &App_HrfBcsmaSendStk[0],
              APP_CFG_TASK_BPLC_SEND_STK_SIZE / 10u,
              APP_CFG_TASK_BPLC_SEND_STK_SIZE,
              0u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err);

	 OSTaskCreate(&App_Bplc_SendMac_TCB,
             "BPLC Send MAC Task",
              MpduTask,
              0,
              APP_CFG_TASK_BPLC_SENDMAC_PRIO,
              &App_BplcSendMacStk[0],
              APP_CFG_TASK_BPLC_SENDMAC_STK_SIZE / 10u,
              APP_CFG_TASK_BPLC_SENDMAC_STK_SIZE,
              0u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err); 
	OSTaskCreate(&App_Bplc_BcsmaSendMac_TCB,
             "BPLC Send  bcsma MAC Task",
              BcsmaMpduTask,
              0,
              APP_CFG_TASK_BPLC_BCSMA_SENDMAC_PRIO,
              &App_BplcBcsmaSendMacStk[0],
              APP_CFG_TASK_BPLC_SENDMAC_STK_SIZE / 10u,
              APP_CFG_TASK_BPLC_SENDMAC_STK_SIZE,
              0u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err);

	OSTaskCreate(&App_EventTCB,
             "Event handle Task",
              EventTask,
              0,
              APP_CFG_TASK_EVENT_PRIO,
              &App_EventStk[0],
              APP_CFG_TASK_EVENT_STK_SIZE / 10u,
              APP_CFG_TASK_EVENT_STK_SIZE,
              0u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err); 
#ifdef II_STA
	OSTaskCreate(&App_II_STA_TCB,
             "II STA Task",
              II_STA_Task,
              0,
              APP_CFG_TASK_II_STA_PRIO,
              App_II_STA_Stk,
              APP_CFG_TASK_II_STA_STK_SIZE / 10u,
              APP_CFG_TASK_II_STA_STK_SIZE,
              1u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err); 

	OSTaskCreate(&App_II_TimingTCB,
             "II Timing Task",
              HPLC_II_TimingTask,
              0,
              APP_CFG_TASK_II_TIMING_PRIO,
              &App_II_TimingStk[0],
              APP_CFG_TASK_II_TIMING_STK_SIZE / 10u,
              APP_CFG_TASK_II_TIMING_STK_SIZE,
              0u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err); 
#endif	
#if defined(PROTOCOL_NW_2020)&&!defined(II_STA)
	OSTaskCreate(&App_GraphTCB,
             "Graph handle Task",
              HPLC_GraphTask,
              0,
              APP_CFG_TASK_GRAPH_PRIO,
              &App_GraphStk[0],
              APP_CFG_TASK_GRAPH_STK_SIZE / 10u,
              APP_CFG_TASK_GRAPH_STK_SIZE,
              0u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err); 

#endif
	#ifdef OPERATION
	NewTaskInit();
	#endif
	OSTaskCreate(&App_Hrf_SendMac_TCB,
             "Hrf Send MAC Task",
              HrfMpduTask,
              0,
              APP_CFG_TASK_HRF_SENDMAC_PRIO,
              &App_HrfSendMacStk[0],
              APP_CFG_TASK_BPLC_SENDMAC_STK_SIZE / 10u,
              APP_CFG_TASK_BPLC_SENDMAC_STK_SIZE,
              0u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err); 
	OSTaskCreate(&App_Hrf_BcsmaSendMac_TCB,
             "Hrf Send  bcsma MAC Task",
              HrfBcsmaMpduTask,
              0,
              APP_CFG_TASK_HRF_BCSMA_SENDMAC_PRIO,
              &App_HrfBcsmaSendMacStk[0],
              APP_CFG_TASK_BPLC_SENDMAC_STK_SIZE / 10u,
              APP_CFG_TASK_BPLC_SENDMAC_STK_SIZE,
              0u,
              0u,
              0,
             (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
             &os_err);

#ifdef INTER_WATCH_DOG
    InitWdg();
#endif
    while(1)
    {
#ifdef INTER_WATCH_DOG
        FeedWdg();
#endif

		bplc_nbi_cnt++;
		if (bplc_nbi_cnt > 50)//每s探测窄带噪声
		{
			BPLC_NbiDet();
			bplc_nbi_cnt = 0;
		}
		
        //PLLRangeCheck();

		//BPLC_RecfgPulse(1);
		Zero_Lost();
        ledCnt++;
        if(ledCnt >= 50)
        {
             u32 flag = 0;
			const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();
			if(IsBindLocalMac())
			{
				flag |= LED_BIND_ADDR_FLAG;
			}
			if(pNetBasePara->NetState == NET_IN_NET)
			{
				flag |= LED_IN_NET_FLAG;
			}
			if (!pNetBasePara->connectReader)
			{
				flag |= LED_CONNECT_READER;
			}
			LedFlashLogic(flag);
			ledCnt = 0;
			//BPLC_StartNbi();
			#ifdef __DEBUG_MODE
            static u8 dug_cnt=0;
            dug_cnt++;
            if(dug_cnt>=60){
                dug_cnt = 0;
                debug_str(0xffffffff,"debug mode\r\n");
            }
			#endif
			
			CapEnCtrFun();//超容充电控制
			
			#ifdef NW_TEST
			u32 currentTick = OSTimeGet(&os_err);
			if (currentTick<8*60*1000)
			{
				if (pNetBasePara->NetState == NET_IN_NET)
				{
					joined=1;
				}
			}
			else
			{
				if (joined==0)//8分钟没有入网 恢复功率
				{
					BPLC_SetTxGain(HPLC_LowPower[BPLC_TxFrequenceGet()]);
				}
			}
			#endif
			
			#ifdef LOW_POWER
			if (join == 0)//没有入过网
			{
				if (pNetBasePara->NetState == NET_IN_NET || GetAppCurrentModel() != APP_TEST_NORMAL)
				{
					join = 1;
					//开启hrf的无线
					HRF_SwitchAfe(1);
				}
				else
				{
					u32 current = OSTimeGet(&os_err);
					static u32 tick_delay = 0;
					if (current - tick_delay >= (1000*60*2))
					{
						tick_delay = current;
						HRF_SwitchAfe(!HRF_AfeStatus());
					}
				}
			}
			#endif

            #if 0
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
        //ADC_TriggerConverson(ADC);
        OSTimeDlyHMSM(0,0,0,20,OS_OPT_TIME_DLY, &os_err);
    }
}

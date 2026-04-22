#include  <gsmcu_hal.h>
#include <string.h>
#include "os.h"
#include <bsp.h>
#include  <os_cfg_app.h>
#include "phy_port.h"
#include "GSMCU_M3.h"
#include "gsmcu_m3_port.h"
#include "gsmcu_m3_gpio.h"
#include "hplc_receive.h"
#include "hrf_port.h"
#include "System_inc.h"
#define ZERO_DIFF_TO_NORMAL     (-15479)



uint32_t recv_cnt = 0;
uint32_t send_cnt = 0;

uint32_t send_start_err = 0;
uint32_t switch_line = 0;

volatile uint32_t ntb_stamp[16][2];
uint8_t ntb_index = 0;



uint32_t user_over_flow_count = 0;
uint32_t user_fc_fail_count = 0;
uint32_t user_fc_pass_count = 0;





uint32_t ZC_Time[3] = {0, 0, 0};

phaseRecognizeType ZC_NTB[3][2];
static BspZeroCrossParamType ZeroCrossParam;
void ClearZC_NTB(void)
{
  memset(ZC_NTB,0,sizeof(ZC_NTB));
}

void SaveZCNtb(uint32_t NTB, phaseRecognizeType *zcNtb)
{
    if(zcNtb->Widx == zcNtb->Ridx)
	{
    	zcNtb->zc_ntb[zcNtb->Widx++] = NTB;
	}
	else
	{
		if(zcNtb->Widx == 0)
		{
			if(NTB - zcNtb->zc_ntb[MAX_ZC_RECOGNIZE_PHASE-1] < 5*NTB_FRE_PER_MS)
			{
				return;
			}
			else
			{
				zcNtb->zc_ntb[zcNtb->Widx++] = NTB;
			}
		}
		else
		{
			if(NTB - zcNtb->zc_ntb[zcNtb->Widx-1] < 5*NTB_FRE_PER_MS)
			{
				return;
			}
			else
			{
				zcNtb->zc_ntb[zcNtb->Widx++] = NTB;
			}
		}
	}
	
    if (zcNtb->Widx==MAX_ZC_RECOGNIZE_PHASE)
    {
        zcNtb->Widx=0;
    }
    if (zcNtb->Widx==zcNtb->Ridx)
    {
        zcNtb->Ridx=zcNtb->Widx+1;
        if (zcNtb->Ridx==MAX_ZC_RECOGNIZE_PHASE)
        {
            zcNtb->Ridx=0;
        }
    }
}

//20 ms 中断一次
void ZC_Handler(int phase)
{
	if (phase == BPS_PHASE_A)
	{
		ZC_Time[0] = ZeroCrossx_Get(BPS_PHASE_A);
		ZC_Time[0] += (u32)ZERO_DIFF_TO_NORMAL;  //机台偏差
		SaveZCNtb(ZC_Time[0], &ZC_NTB[0][0]);

#ifndef ZC_PIN_PHASEA_NEG
		SaveZCNtb(ZC_Time[0] - 10 * NTB_FRE_PER_MS, &ZC_NTB[0][1]);
#endif
	}
#if defined(ZC_PIN_PHASEA_NEG)
	else if (phase == BPS_PHASE_A_NEG)
	{
		ZC_Time[0] = ZeroCrossx_Get(BPS_PHASE_A_NEG);
		ZC_Time[0] += (u32)ZERO_DIFF_TO_NORMAL;  //机台偏差
		SaveZCNtb(ZC_Time[0], &ZC_NTB[0][1]);
	}
#endif
	else if (phase == BPS_PHASE_B)
	{
		ZC_Time[1] = ZeroCrossx_Get(BPS_PHASE_B);
		ZC_Time[1] += (u32)ZERO_DIFF_TO_NORMAL;  //机台偏差
		SaveZCNtb(ZC_Time[1], &ZC_NTB[1][0]);

#ifndef ZC_PIN_PHASEB_NEG
		SaveZCNtb(ZC_Time[1] - 10 * NTB_FRE_PER_MS, &ZC_NTB[1][1]);
#endif
	}
#if defined(ZC_PIN_PHASEB_NEG)
	else if (phase == BPS_PHASE_B_NEG)
	{
		ZC_Time[1] = ZeroCrossx_Get(BPS_PHASE_B_NEG);
		ZC_Time[1] += (u32)ZERO_DIFF_TO_NORMAL;  //机台偏差
		SaveZCNtb(ZC_Time[1], &ZC_NTB[1][1]);
	}
#endif
	else if (phase == BPS_PHASE_C)
	{
		ZC_Time[2] = ZeroCrossx_Get(BPS_PHASE_C);
		ZC_Time[2] += (u32)ZERO_DIFF_TO_NORMAL;  //机台偏差
		SaveZCNtb(ZC_Time[2], &ZC_NTB[2][0]);

#ifndef ZC_PIN_PHASEC_NEG
		SaveZCNtb(ZC_Time[2] - 10 * NTB_FRE_PER_MS, &ZC_NTB[2][1]);
#endif
	}
#if defined(ZC_PIN_PHASEC_NEG)
	else if (phase == BPS_PHASE_C_NEG)
	{
		ZC_Time[2] = ZeroCrossx_Get(BPS_PHASE_C_NEG);
		ZC_Time[2] += (u32)ZERO_DIFF_TO_NORMAL;  //机台偏差
		SaveZCNtb(ZC_Time[2], &ZC_NTB[2][1]);
	}
#endif
}

void NTB_IRQHandler(void)
{
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	OSIntEnter();
	OS_CRITICAL_EXIT();

	ntb_interrupt_handler();

	OSIntExit();
}


void BPLC_IRQHandler(void)
{
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	OSIntEnter();
	OS_CRITICAL_EXIT();

	bplc_interrupt_handler();

	OSIntExit();
}

//static 
void PHY_SetLineDriver(uint32_t state,uint32_t phase)
{
    if(!state)
    {
        //接收到信标帧也会回调到这里  ???
		//HPLC_SwitchReceivePin();
#if DEV_CCO &&  (!DEV_PURE_DRV)
        HPLC_SetSendPinOff((HPLC_SEND_PHASE)phase);
		sending_switch=0;
#endif        
		LD_OUT_OFF();
#if DEV_CCO &&  (!DEV_PURE_DRV)		
		HPLC_SwitchReceivePin();
#endif
    }
    else
    {
#if DEV_CCO && (!DEV_PURE_DRV)
		PA_IN_OFF();
		PB_IN_OFF();
		PC_IN_OFF();
        HPLC_SetSendPinOn((HPLC_SEND_PHASE)phase);
		sending_switch=1;
#endif
        LD_OUT_ON();
    }
}

void PHY_SendCallbackFun(bool_t flag)
{
	if (flag == SUCCESS)
	{
		send_cnt++;
	}
	else
	{

		send_start_err++;
	}

#if (DEV_PURE_DRV)
    SendCallbackFun(flag);
#endif 

}

void PHY_RecvCallbackFun(uint8_t *data, uint16_t len, uint8_t crc_res, BPLC_recv_para *para)
{
	recv_cnt++;

#if (DEV_PURE_DRV)
    RecvCallbackFun(data, len, crc_res, para);
#else
    if((len <= MSDU_MAX_BLOCK_SIZE) && len)
    {

        {

            HPLC_ReceiveCallback(data, len, para,crc_res);
        }
    }
#endif     
	UnlockUserBuf();
}

void PHY_OverFlowCallbackFun(void)
{
	user_over_flow_count++;
}

void PHY_FrameCtrlCallbackFun(uint8_t *data, BPLC_recv_para *para, bool_t status)
{
	if (status == SUCCESS)
	{
		user_fc_pass_count++;
#if (DEV_PURE_DRV)
#else
    #if DEV_CCO
        VCS_ReceiveFrame(0,data, sizeof(MPDU_CTL),true);
        MPDU_CTL *mpduCtl =  (MPDU_CTL *)data;
		VCS_FCCallBack(0,mpduCtl->Type<=MPDU_TYPE_SOF?1:0);
        switch(mpduCtl->Type)
        {
            case MPDU_TYPE_BEACON:
            {
                HPLC_ReceiveBeaconFCCallback(data, sizeof(MPDU_CTL), para);
                break;
            }
            case MPDU_TYPE_CONFIRM: //确认帧
            case MPDU_TYPE_CONCERT: //网间协调帧
            {
                HPLC_ReceiveCallback(data, sizeof(MPDU_CTL), para,status);
                break;
            }
            default:
            {
                break;
            }
        }
    #endif
#endif
	}
	else
	{
		VCS_ReceiveFrame(0,data, sizeof(MPDU_CTL),false);
		VCS_FCCallBack(0,1);
		user_fc_fail_count++;
	}

#if (DEV_PURE_DRV)
    FrameCtrlCallbackFun(data, para, status);
#endif

	UnlockUserFcBuf();
}

void HRF_PHY_RecvCallbackFun(uint8_t *data, uint16_t len, uint8_t crc_res, BPLC_recv_para *para)
{
	recv_cnt++;


	if ((len <= MSDU_MAX_BLOCK_SIZE) && len)
	{
		HRF_ReceiveCallback(data, len, para, crc_res);
	}
	HRF_UnlockUserBuf();
}



void HRF_PHY_FrameCtrlCallbackFun(uint8_t *data, BPLC_recv_para *para, bool_t status)
{
	if (status == SUCCESS)
	{
		user_fc_pass_count++;

        VCS_ReceiveFrame(1,data, sizeof(MPDU_CTL),true);
        MPDU_CTL *mpduCtl =  (MPDU_CTL *)data;
		VCS_FCCallBack(1,mpduCtl->Type<=MPDU_TYPE_SOF?1:0);
        switch(mpduCtl->Type)
        {
            case MPDU_TYPE_BEACON:
            {
                HRF_ReceiveBeaconFCCallback(data, sizeof(MPDU_CTL), para);
                break;
            }
            case MPDU_TYPE_CONFIRM: //确认帧
            case MPDU_TYPE_CONCERT: //网间协调帧
            {
                HRF_ReceiveCallback(data, sizeof(MPDU_CTL), para,status);
                break;
            }
            default:
            {
                break;
            }
        }
	}
	else
	{
		VCS_ReceiveFrame(1,data, sizeof(MPDU_CTL),false);
		VCS_FCCallBack(1,1);
		user_fc_fail_count++;
	}

	HRF_UnlockUserPhrBuf();
}
void PLC_lower_Init(void)
{


#if DEV_CCO && (!DEV_PURE_DRV)
    #if defined(HPLC_CSG)
#ifndef ALL_USE_TESTMODE_PARAM
	HPLC_SouthPhyInit(2, HPLC_ChlPower[2]);
#else
	HPLC_SouthPhyInit(2, HPLC_TestChlPower[2]);
#endif
    #else
#ifndef ALL_USE_TESTMODE_PARAM
	HPLC_PhyInit(1, HPLC_ChlPower[1]);
#else
	HPLC_PhyInit(1, HPLC_TestChlPower[1]);
#endif
    #endif
#endif
	ZeroCrossParam.callfunction = ZC_Handler;
	ZeroCrossParam.TriggerMode = ZeroCross_Trigger_Falling;
	
	ZeroCrossOpen(&ZeroCrossParam);
		
	ZeroCrossx_Start(0);
	ZeroCrossx_Start(1);
	ZeroCrossx_Start(2);

	ZeroCrossx_Start(3);
	ZeroCrossx_Start(4);
	ZeroCrossx_Start(5);	


	SetReceiveCallback(PHY_RecvCallbackFun, PHY_FrameCtrlCallbackFun, PHY_OverFlowCallbackFun);

	BPLC_EnableIRQ(BPLC_BBP_INT_EN_TX_END_INT_EN_Msk        |         //发送完成
				   BPLC_BBP_INT_EN_TX_START_FAIL_INT_EN_Msk |      //发送起始错误
				   BPLC_BBP_INT_EN_SYNC_BEGIN_INT_EN_Msk    |
				   BPLC_BBP_INT_EN_NBI_DET_END_INT_EN_Msk   |     //NBI
				   BPLC_BBP_INT_EN_FRAMESYNCED_INT_EN_Msk   |         //帧同步完成
				   BPLC_BBP_INT_EN_RX_FC_RCV_INT_EN_Msk     |         //FC中断
				   BPLC_BBP_INT_EN_RX_OVER_FLOW_INT_EN_Msk      |      //接受溢出
				   BPLC_BBP_INT_EN_RX_END_INT_EN_Msk);                //接收完成中断
	
#if (DEV_PURE_DRV)
    PLC_Demo_Init();
#endif
}
static void OverFlowHandle(void)
{

}
static void HrfDirPinInit(void)
{
#if EFEM_MODE 
    GPIO_SetPinDir(FEM_CPS_PORT, FEM_CPS_PIN, GPIO_OUTPUT);                       // CPS set to ANT_SEL mode, 1: TX, 0: RX
    PORT_PinConfigure(FEM_CPS_PORT, FEM_CPS_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
    GPIO_PinWrite(FEM_CPS_PORT, FEM_CPS_PIN, 0);
    GPIO_SetPinDir(FEM_CTX_PORT, FEM_CTX_PIN, GPIO_OUTPUT);                       // CTX set to ANT_SEL mode and controlled from RXGT, eLNA control
    PORT_PinConfigure(FEM_CTX_PORT, FEM_CTX_PIN, (PORT_FUNC)(PORT_CFG_MODE(1)));
    //GPIO_PinWrite(FEM_CTX_PORT, FEM_CTX_PIN, 1);
    GPIO_SetPinDir(FEM_CSD_PORT, FEM_CSD_PIN, GPIO_OUTPUT);                       // CSD set to GPIO mode and always ouput 1 except shutdown 
    PORT_PinConfigure(FEM_CSD_PORT, FEM_CSD_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));   
    GPIO_PinWrite(FEM_CSD_PORT, FEM_CSD_PIN, 1);
#elif (HRF_DIR_MODE==2)
	GPIO_SetPinDir(HRF_VC1_PORT, HRF_VC1_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(HRF_VC1_PORT, HRF_VC1_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(HRF_VC1_PORT, HRF_VC1_PIN, 1);
	GPIO_SetPinDir(HRF_VC2_PORT, HRF_VC2_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(HRF_VC2_PORT, HRF_VC2_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(HRF_VC2_PORT, HRF_VC2_PIN, 0);
#else
	GPIO_SetPinDir(HRF_DIR_PORT, HRF_DIR_PIN, GPIO_OUTPUT); //
	PORT_PinConfigure(HRF_DIR_PORT, HRF_DIR_PIN, (PORT_FUNC)(PORT_CFG_MODE(2)));
	GPIO_PinWrite(HRF_DIR_PORT, HRF_DIR_PIN, 0);
#endif

}

void HRF_PHY_Init(u8 option,u8 channel)
{
	HrfDirPinInit();
	HRF_SetReceiveCallback(HRF_PHY_RecvCallbackFun, HRF_PHY_FrameCtrlCallbackFun, OverFlowHandle);
#ifndef ALL_USE_TESTMODE_PARAM
	HRF_PhyInit(channel, option,  HRF_TestChlPower[option&0x1]);
#else
	HRF_PhyInit(channel, option,  HRF_LinePower);
#endif

	HRF_EnableIRQ(HRF_BBP_INT_EN_TX_END_INT_EN_Msk            |      //发送完成
				   HRF_BBP_INT_EN_TX_FAIL_INT_EN_Msk          |      //发送起始错误
				   HRF_BBP_INT_EN_RX_FRAMESYNCED_RISE_INT_EN_Msk |      //帧同步完成
				   HRF_BBP_INT_EN_RX_PHR_END_INT_EN_Msk       |      //PHR中断
				   HRF_BBP_INT_EN_RX_PSDU_END_INT_EN_Msk      |      //PSDU中断				   
				  HRF_BBP_INT_EN_RX_OVERFLOW_INT_EN_Msk       |
				   HRF_BBP_INT_EN_RX_END_INT_EN_Msk);                //接收完成
	NVIC_ClearPendingIRQ(HRF_IRQn);
	NVIC_EnableIRQ(HRF_IRQn);
	#if defined(ZB205_CHIP)||defined(ZB206_CHIP)
	NVIC_ClearPendingIRQ(HRF_DFE_IRQn);
	NVIC_EnableIRQ(HRF_DFE_IRQn);
	#endif
}

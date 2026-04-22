#include "protocol_includes.h"
#include "common_includes.h"
#include "os.h"
#include "CCO_app.h"
#include "HRF_SpiInterface.h"
#include "Uart_Link.h"

extern u32 total_bytes;
extern u8 *bin_data;
extern u32 bin_crc;
extern u32 PLC_TO_RF_TESTMODE_PARAM;
OS_SEM MutiEventSem;//多事件发生信号  包括事件产生  发送csma  发送信标 该信号量仅用于hrf任务等待过程中等待
OS_SEM SPI_BusySem;
OS_Q SPI_ResultQ;
extern P_OS_RES      g_pOsRes;
OS_Q HrfCsmaSendQ;
OS_Q HrfBeacSendQ;
static CSMA_CALL_BACK HRF_Callback;
u8 hrf_flag;
void (*HrfGetPara)(u32);

extern OS_Q PlcDataQueue;
const u16 HRF_PB_SIZE_ARRAY[]={
	16,40,72,136,264,520
};
struct {
	u32 cmd_count;
	u32 get_data_count;
	u32 send_data_count;
	u32 send_beacon_count;
	u32 send_coor_count;
}HRF_dbg_info;
static int GetPbSize(int len)
{
	for(int i=0;i<sizeof(HRF_PB_SIZE_ARRAY)/2;i++)
	{
		if(len<=HRF_PB_SIZE_ARRAY[i])
		{
			return HRF_PB_SIZE_ARRAY[i];
		}
	}
	return 0;//HRF无法发送此帧
}
static int GetPbIndex(int len)
{
	for(int i=0;i<sizeof(HRF_PB_SIZE_ARRAY)/2;i++)
	{
		if(len<=HRF_PB_SIZE_ARRAY[i])
		{
			return i;
		}
	}
	return 0xff;
}
//option3  phr 0 1   psdu 0

u16 HRF_PsduSymbol(u8 PSDU_MCS,u8 option,u16 len)
{

	u8  PSDU_ModulationMode;

	u8  PSDU_CopyNum;

	u8  PSDU_PuncMode;
	
	switch (PSDU_MCS)
	{
	case 0:
		PSDU_ModulationMode = 0; PSDU_CopyNum = 4; PSDU_PuncMode = 0; break;
	case 1:
		PSDU_ModulationMode = 0; PSDU_CopyNum = 2; PSDU_PuncMode = 0; break;
	case 2:
		PSDU_ModulationMode = 1; PSDU_CopyNum = 2; PSDU_PuncMode = 0; break;
	case 3:
		PSDU_ModulationMode = 1; PSDU_CopyNum = 1; PSDU_PuncMode = 0; break;
	case 4:
		PSDU_ModulationMode = 1; PSDU_CopyNum = 1; PSDU_PuncMode = 1; break;
	case 5:
		PSDU_ModulationMode = 2; PSDU_CopyNum = 1; PSDU_PuncMode = 0; break;
	case 6:
		PSDU_ModulationMode = 2; PSDU_CopyNum = 1; PSDU_PuncMode = 1; break;
	default:
		PSDU_ModulationMode = 0; PSDU_CopyNum = 4; PSDU_PuncMode = 0; break;
	}

	u16 PSDU_PBSize = GetPbSize(len);
	u16 PSDU_DataLenIn = PSDU_PuncMode ? (PSDU_PBSize << 3) + (PSDU_PBSize << 1) : PSDU_PBSize << 4;
	u8 PSDU_InterNum = (option == 1 | option == 2) ? (PSDU_CopyNum == 6  ? 6 :
													  PSDU_CopyNum == 4  ? 4 :
													  PSDU_CopyNum == 3  ? 6 :
													  PSDU_CopyNum == 2  ? 4 : 1) :
		(option == 3) ? (PSDU_CopyNum == 6  ? 6 :
						 PSDU_CopyNum == 4  ? 4 :
						 PSDU_CopyNum == 3  ? 6 :
						 PSDU_CopyNum == 2  ? 2 : 1) : 1;

	u8 ValidCarrierNum = (option == 1) ? 96 :
		(option == 2) ? 48 :
		(option == 3) ? 18 : 0;

	u8 PSDU_RoboUsedCarrierNum = PSDU_InterNum * (ValidCarrierNum / PSDU_InterNum);
	u8 PSDU_BPC = PSDU_ModulationMode == 0 ? 1 :
		PSDU_ModulationMode == 1 ? 2 : 4;
	s16 PSDU_OFDMNum_tmp;
	PSDU_OFDMNum_tmp = PSDU_DataLenIn * PSDU_CopyNum - PSDU_RoboUsedCarrierNum * PSDU_BPC;
	while (PSDU_OFDMNum_tmp > 0)
	{
		PSDU_OFDMNum_tmp = PSDU_OFDMNum_tmp - PSDU_RoboUsedCarrierNum * PSDU_BPC;
	}
	u16 PSDU_OFDMNum = PSDU_OFDMNum_tmp < 0 ? PSDU_DataLenIn * PSDU_CopyNum / (PSDU_RoboUsedCarrierNum * PSDU_BPC) + 1 : PSDU_DataLenIn * PSDU_CopyNum / (PSDU_RoboUsedCarrierNum * PSDU_BPC);
	return PSDU_OFDMNum ;
}
u16 HRF_PhrSymbol(u8 PHR_MCS,u8 option)
{
	u8  PHR_ModulationMode;

	u8  PHR_CopyNum;

	u8  PHR_PuncMode;

	switch (PHR_MCS)
	{
	case 0:
		PHR_ModulationMode = 0; PHR_CopyNum = 6; PHR_PuncMode = 0; break;
	case 1:
		PHR_ModulationMode = 0; PHR_CopyNum = 4; PHR_PuncMode = 0; break;
	case 2:
		PHR_ModulationMode = 0; PHR_CopyNum = 3; PHR_PuncMode = 0; break;
	case 3:
		PHR_ModulationMode = 0; PHR_CopyNum = 2; PHR_PuncMode = 0; break;
	case 4:
		PHR_ModulationMode = 1; PHR_CopyNum = 2; PHR_PuncMode = 0; break;
	case 5:
		PHR_ModulationMode = 1; PHR_CopyNum = 1; PHR_PuncMode = 0; break;
	case 6:
		PHR_ModulationMode = 1; PHR_CopyNum = 1; PHR_PuncMode = 1; break;
	default:
		PHR_ModulationMode = 0; PHR_CopyNum = 6; PHR_PuncMode = 0; break;
	}


	u16 PHR_DataLenIn = PHR_PuncMode ? (16 << 3) + (16 << 1) : 16 << 4;
	u8  PHR_InterNum = (option == 1 | option == 2) ? (PHR_CopyNum == 6  ? 6 :
													  PHR_CopyNum == 4  ? 4 :
													  PHR_CopyNum == 3  ? 6 :
													  PHR_CopyNum == 2  ? 4 : 1) :
		(option == 3) ? (PHR_CopyNum == 6  ? 6 :
						 PHR_CopyNum == 4  ? 4 :
						 PHR_CopyNum == 3  ? 6 :
						 PHR_CopyNum == 2  ? 2 : 1) : 1;
	u8 ValidCarrierNum = (option == 1) ? 96 :
		(option == 2) ? 48 :
		(option == 3) ? 18 : 0;

	u8 PHR_RoboUsedCarrierNum = PHR_InterNum * (ValidCarrierNum / PHR_InterNum);
	u8 PHR_BPC = PHR_ModulationMode == 0 ? 1 :
		PHR_ModulationMode == 1 ? 2 : 4;
	s16 PHR_OFDMNum_tmp;
	PHR_OFDMNum_tmp = PHR_DataLenIn * PHR_CopyNum - PHR_RoboUsedCarrierNum * PHR_BPC;
	while (PHR_OFDMNum_tmp > 0)
	{
		PHR_OFDMNum_tmp = PHR_OFDMNum_tmp - PHR_RoboUsedCarrierNum * PHR_BPC;
	}
	u16 PHR_OFDMNum = PHR_OFDMNum_tmp < 0 ? PHR_DataLenIn * PHR_CopyNum / (PHR_RoboUsedCarrierNum * PHR_BPC) + 1 : PHR_DataLenIn * PHR_CopyNum / (PHR_RoboUsedCarrierNum * PHR_BPC);
	return  PHR_OFDMNum;
}
u8 HrfBestTable[2][3]={
	{3,3,3},
	{2,2,3}
};
u8 HrfPhrBestTable[2][3]={
	{2,2,4},
	{1,1,2}
};
u16 HRFBestMcs(u8 option,u8 isBo)
{
	return HrfBestTable[isBo][option-1];
}
u16 HRFPhrBestMcs(u8 option,u8 isBo)
{
	return HrfPhrBestTable[isBo][option-1];
}
#define TFFT  12288
#define T_TR  768
#define T_G   3072
u16 HRFrameLen(u8 phr,int psdu,u8 option)//100us
{
	u32 totalUs=0;
	if (phr)
	{
		//SIG
		if (option==1)
		{
			totalUs=TFFT+T_G;
		}
		else
		{
			totalUs=2*TFFT+T_G;
		}
		//STF+LTF
		totalUs+=TFFT*5+2.5*TFFT+T_TR;
		//phr
		totalUs+=(TFFT+T_G)*phr;
	}
	if (psdu)
	{
		totalUs+=(TFFT+T_G)*psdu;
	}
	return (totalUs+9999)/10000;
}


void SPI_StatusCallBack(u8 cmd,u8 status,u8 *data,u16 len)
{
	OS_ERR err;
	if(status
	   &&cmd==CMD_GET_FRAME
	   &&len)//接收到了数据
	{


		P_TMSG pMsg = NULL;
		PDU_BLOCK *newPdu = NULL;

		HRF_RecFrame *rec = (HRF_RecFrame *)data;

		COMM_TEST_MODE test_mode_array[] = {TEST_MODE_HRF_PACKET_TO_UART};//无线透传模式下直接发送到串口
		if (MPDU_TYPE_BEACON != (((P_MPDU_CTL)rec->data)->Type) && IsCommTestModeOr(test_mode_array, countof(test_mode_array), NULL))
		{
				if(rec->crc24)
				{
					P_MSG_INFO pSendMsg = (P_MSG_INFO)alloc_send_buffer_by_len(__func__, (u16)len);
					if (NULL != pSendMsg)
					{
						pSendMsg->msg_header.end_id = COM_PORT_MAINTAIN;
						pSendMsg->msg_header.msg_len = len;
						pSendMsg->msg_header.send_delay = false;
						memcpy(pSendMsg->msg_buffer, data, len);
						End_post(pSendMsg);
						OSQPost(&SPI_ResultQ, (void *)status, 0, OS_OPT_POST_1, &err);
						OSSemPost(&MutiEventSem, OS_OPT_POST_1, &err);
						//debug_str("trans mpdu len%d\r\n", len);
					}
				}
			return;
		}

		pMsg = alloc_msg(__func__);
		if (NULL == pMsg)
			return;

		pMsg->message = MSG_HPLC_RECEIVE_MPDU;




		newPdu = MallocPDU();
		if (newPdu == NULL)
			goto L_Error;

		newPdu->receive_ntb = GetNtbCount64();

		//debug_str("%lld - %lld = %lld", newPdu->receive_ntb, (CPU_NTB_64)para->sync_ntb, newPdu->receive_ntb - (CPU_NTB_64)para->sync_ntb);
		newPdu->size =  len - sizeof(HRF_RecFrame);
		memcpy(newPdu->pdu, rec->data, newPdu->size);
		newPdu->rxPara.channel = 1;
		newPdu->rxPara.option = rec->option;
		newPdu->rxPara.channel = rec->channel;
		newPdu->rxPara.sync_ntb = rec->SyncNtb;
		newPdu->rxPara.hrf_rssi = rec->Rssi;
		newPdu->rxPara.hrf_snr = rec->Snr;
		newPdu->rxPara.link = 1;
		pMsg->lParam = (LPARAM)newPdu;
		//debug_hex(false, newPdu->pdu, newPdu->size);

		OSTaskQPost(&g_pOsRes->App_Bplc_Rec_TCB, pMsg, rec->crc24, OS_OPT_POST_FIFO, &err);
		HRF_dbg_info.get_data_count++;
		if (OS_ERR_NONE != err)
			goto L_Error;
		OSQPost(&SPI_ResultQ, (void *)status, 0, OS_OPT_POST_1, &err);
		OSSemPost(&MutiEventSem, OS_OPT_POST_1, &err);
		return;
L_Error:
		free_msg(__func__, pMsg);
		FreePDU(newPdu);
		OSQPost(&SPI_ResultQ, (void *)status, 0, OS_OPT_POST_1, &err);
		OSSemPost(&MutiEventSem, OS_OPT_POST_1, &err);
		return;


	}

	OSQPost(&SPI_ResultQ, (void *)status, 0, OS_OPT_POST_1, &err);
	if (status && HrfGetPara&&cmd==HRF_GET_SYNC_NTB)
	{
		SPI_FRAME *frame = (SPI_FRAME *)data;
		HrfGetPara(frame->data[0]);
		OSSemPost(&MutiEventSem, OS_OPT_POST_1, &err);
		return;
	}
	if (status && HRF_Callback) //发送或接收成功
	{
		HRF_Callback(CSMA_SEND_OK);
		HRF_Callback=NULL;
	}
	OSSemPost(&MutiEventSem, OS_OPT_POST_1, &err);
}
void SPI_IrqBusy(uint32_t arg)//SPI的BUSY IO的中断
{
	OS_ERR err;
	OSSemPost(&SPI_BusySem, OS_OPT_POST_1, &err);
	OSSemPost(&MutiEventSem, OS_OPT_POST_1, &err);
}

void SPI_IrqEvent(u32 arg)//有事件产生 
{
	OS_ERR err;
	OSSemPost(&MutiEventSem, OS_OPT_POST_1, &err);
	GPIO_ClearIRQStatus(EVENT_REQ_PORT, EVENT_REQ_PIN);
}
static int WaitSpiBusy(void)
{
	OS_ERR err;
	OS_MSG_SIZE size;
	uint32_t result=(uint32_t)OSQPend(&SPI_ResultQ,0, OS_OPT_PEND_BLOCKING, &size,NULL, &err);//等待SPI发送完毕
	for(int i = 0; i < 3; i++)
	{
		if(GPIO_ReadInputDataBit(SPI_BUSY_PORT, SPI_BUSY_PIN))//等待从设备可以重新接收数据
		{
			//开使能开关
			GPIO_ClearIRQStatus(SPI_BUSY_PORT, SPI_BUSY_PIN);
			GPIO_SetIRQMode(SPI_BUSY_PORT, SPI_BUSY_PIN,1);
			OSSemPend(&SPI_BusySem, 100, OS_OPT_PEND_BLOCKING, NULL, &err);
			//关闭使能开关
			GPIO_SetIRQMode(SPI_BUSY_PORT, SPI_BUSY_PIN,0);
			
		}
		else
		{
			break;
		}
	}
	return result;
}
HRF_Pack* MallocHrfPack(void)
{

	HRF_Pack *mem_ptr = 0;
	mem_ptr = (HRF_Pack *)alloc_buffer(__func__,600);
	return mem_ptr;
}
void FreeHrfPack(void * mem_ptr)
{
	free_buffer(__func__, mem_ptr);
}

//生成hrf帧
HRF_Pack* GenHrfFrame(SOF_PARAMS *sofParam,u8* mac,CSMA_CALL_BACK callback)
{
	//判断是否修改成单跳mac帧头
	MAC_HEAD * macHead=(MAC_HEAD*)mac;
	
	u16 hrfLen;
	void * macstart=NULL;
	u8 MsduType;
	u16 PbSize;
	if (macHead->Version==0)
	{
		if (macHead->SendType == SENDTYPE_LOCAL&&macHead->MsduType==MSDU_APP_FRAME) //
		{
			//需要转成单跳的hrfLen表示mac帧内容
			//macstart为MAC数据包的起始 
			if(macHead->MACFlag)
			{
				hrfLen = macHead->MsduLen;
				macstart=macHead+1;
			}
			else
			{
				hrfLen = macHead->MsduLen;//没有携带mac需要把mac长度减掉
				macstart = macHead->SrcMac;
			}
			if (hrfLen+sizeof(MAC_HEAD_ONE_HOP)+4>520-1-3)
			{
				return NULL;
			}
			PbSize=GetPbSize(hrfLen+sizeof(MAC_HEAD_ONE_HOP)+4+1+3);
		}
		else
		{
			//正常的帧hrfLen为整个mac的长度 
			//macstart为MAC头的起始 
			hrfLen = macHead->MsduLen;
			hrfLen+=sizeof(MAC_HEAD);
			if (!macHead->MACFlag)
			{
				hrfLen-=12;
			}
			if (hrfLen+4>520-1-3)
			{
				return NULL;
			}
			macstart=mac;
			PbSize=GetPbSize(hrfLen+4+1+3);
		}
		MsduType=macHead->MsduType;
	}
	else//单跳报头
	{
		//直接发单跳的hrfLen表示mac帧内容
		//macstart为MAC数据包的起始 
		MAC_HEAD_ONE_HOP *macHop=(MAC_HEAD_ONE_HOP*)macHead;
		hrfLen = macHop->MsduLen;
		macstart=macHop+1;
		MsduType=macHop->MsduType;
		if (hrfLen+sizeof(MAC_HEAD_ONE_HOP)+4>520-1-3)
		{
			return NULL;
		}
		PbSize=GetPbSize(hrfLen+sizeof(MAC_HEAD_ONE_HOP)+4+1+3);
	}
	
	HRF_Pack *pack = MallocHrfPack();
	if (pack)
	{
		pack->cmd=CMD_SEND_FRAME;
		pack->call=callback;
		pack->arg = 3; //arg 对于发送帧来说传递的参数是重传次数
		pack->arg |= sofParam->doubleSend<<31;
		HRF_SendFrame *pFrame = (HRF_SendFrame *)pack->data;
		//开始组包
		pFrame->option = g_PlmConfigPara.hrf_option;
		pFrame->channel = g_PlmConfigPara.hrf_channel;
		pFrame->BE=1;

		MPDU_CTL *mpduCtl = (MPDU_CTL *)pFrame->data;
		SOF_HRF_ZONE *sofZone = (SOF_HRF_ZONE *)(mpduCtl->Zone0);
		CreateMPDUCtl(mpduCtl, MPDU_TYPE_SOF, sofParam->NID);
		//组SOF域
		
		memset((sofZone), 0, sizeof(SOF_ZONE));
		sofZone->SrcTei = CCO_TEI;
		sofZone->DstTei = sofParam->DstTei;
		sofZone->LinkFlag = sofParam->Lid;

		sofZone->MCS = HRFBestMcs(g_PlmConfigPara.hrf_option, 0); //需要在发送端重新计算
		sofZone->FrameLen = HRFrameLen(0,
									   HRF_PsduSymbol(sofZone->MCS, g_PlmConfigPara.hrf_option,PbSize),
									   g_PlmConfigPara.hrf_option);
		if (sofZone->DstTei != MAC_BROADCAST_TEI)
		{
			sofZone->FrameLen += HRFrameLen(HRF_PhrSymbol(HRFPhrBestMcs(g_PlmConfigPara.hrf_option, 0), g_PlmConfigPara.hrf_option),
										   0,
										   g_PlmConfigPara.hrf_option)+8+23;
		}
		else
		{
			sofZone->FrameLen += 8;
		}
		sofZone->BroadcastFlag = 1;
		sofZone->ReSendFlag = sofParam->BroadcastFlag;
		sofZone->PbSize = GetPbIndex(PbSize);

		//填写PB
		P_SOF_PHY_BLOCK_HEAD head = (P_SOF_PHY_BLOCK_HEAD)(mpduCtl + 1);
		head->BeginFlag=1;
		head->EndFlag=1;
		head->Seq=0;
		if (macHead->Version ==1|| (macHead->Version ==0&&macHead->SendType == SENDTYPE_LOCAL&&macHead->MsduType==MSDU_APP_FRAME)) //本地广播需转成单跳mac
		{

			MAC_HEAD_ONE_HOP *pHopHead=(MAC_HEAD_ONE_HOP*)(head+1);
			memset(pHopHead,0,sizeof(MAC_HEAD_ONE_HOP));
			pHopHead->Version=1;//HRF 版本
			pHopHead->MsduType=MsduType==0?0:128;
			pHopHead->MsduLen = hrfLen;
			memcpy(pHopHead+1,macstart,hrfLen);
			u8 *pcrc=(u8 *)(pHopHead+1);
			*(_CRC_32_*)&pcrc[hrfLen]=GetCrc32(macstart,hrfLen);
			pFrame->Len=hrfLen+sizeof(MAC_HEAD_ONE_HOP)+sizeof(SOF_PHY_BLOCK_HEAD)+16+3+4;
		}
		else
		{
			memcpy(head+1,macstart,hrfLen+4);
			pFrame->Len=hrfLen+sizeof(SOF_PHY_BLOCK_HEAD)+16+3+4;
		}
		
		pack->len = pFrame->Len+sizeof(HRF_SendFrame);
		//*(u32*)(&((u8*)pack)[pack->len])=sum4crc(pack,pack->len);
	}
	
	return pack;
}
bool HRF_SendMac(SOF_PARAMS *sofParam,u8* mac,CSMA_CALL_BACK callback)
{
	HRF_Pack *pack = GenHrfFrame(sofParam, mac, callback);
	OS_ERR err;
	if(pack)
	{
		HRF_dbg_info.send_data_count++;
		OSQPost(&HrfCsmaSendQ, pack, 0, OS_OPT_POST_1, &err);
		if(err!=OS_ERR_NONE)
		{
			FreeHrfPack(pack);
			return false;
		}
		OSSemPost(&MutiEventSem, OS_OPT_POST_1, &err);
		return true;
	}
	return false;
}
void HPLCSendHRFImmediate(u8* mpduCtl, u8 crc)
{
	HRF_Pack *pack = MallocHrfPack();
	if (pack)
	{
		pack->cmd=CMD_SEND_FRAME;
		pack->call=CSMA_SEND_OK;
		pack->arg = 3; //arg 对于发送帧来说传递的参数是重传次数
		pack->arg = 0<<31;
		HRF_SendFrame *pFrame = (HRF_SendFrame *)pack->data;
		//开始组包
		pFrame->option = g_PlmConfigPara.hrf_option;
		pFrame->channel = g_PlmConfigPara.hrf_channel;
		pFrame->BE=1;

		MPDU_CTL *PackmpduCtl = (MPDU_CTL *)pFrame->data;

		MPDU_CTL *orimpduCtl = (MPDU_CTL *)mpduCtl;
		
		SOF_HRF_ZONE *sofZone = (SOF_HRF_ZONE *)(PackmpduCtl->Zone0);	
		
		SOF_HRF_ZONE *p0sofZone = (SOF_HRF_ZONE *)(orimpduCtl->Zone0);
		//组SOF域
		u32 phrmcs, psdumcs, pbsizeidx, framelen;
		phrmcs = (PLC_TO_RF_TESTMODE_PARAM &0xF00) >> 8;
		psdumcs = (PLC_TO_RF_TESTMODE_PARAM &0xF0) >> 4;
		pbsizeidx =  PLC_TO_RF_TESTMODE_PARAM & 0xF;
		u32 pbsize = HRF_PB_SIZE_ARRAY[pbsizeidx];
		framelen = HRFrameLen(0, HRF_PsduSymbol(psdumcs, g_PlmConfigPara.hrf_option,pbsize),g_PlmConfigPara.hrf_option)+8;
		if (p0sofZone->DstTei != MAC_BROADCAST_TEI)
		{
			framelen += HRFrameLen(HRF_PhrSymbol(phrmcs, g_PlmConfigPara.hrf_option), 0, g_PlmConfigPara.hrf_option)+8+23;
		}
		else
		{
			framelen += 8;
		}
		pFrame->Len = framelen + sizeof(MPDU_CTL);
		pFrame->MCS = phrmcs;
		memcpy(PackmpduCtl,mpduCtl,pFrame->Len);
		sofZone->MCS = psdumcs; //需要在发送端重新计算
		sofZone->PbSize = pbsizeidx;
		sofZone->FrameLen = framelen;
		PackmpduCtl->CRC24 = GetCrc24((u8*)PackmpduCtl, sizeof(MPDU_CTL) - 3);
		pack->len = pFrame->Len+sizeof(HRF_SendFrame);
	
		OS_ERR err;
		HRF_dbg_info.send_data_count++;
		OSQPost(&HrfCsmaSendQ, pack, 0, OS_OPT_POST_1, &err);
		if(err!=OS_ERR_NONE)
		{
			FreeHrfPack(pack);
			return ;
		}
		OSSemPost(&MutiEventSem, OS_OPT_POST_1, &err);
		return ;
	}
	return ;
}
bool HRF_SencConcert( u8 *pdu_data, u16 pdu_len)
{
	OS_ERR err;
	HRF_Pack *pack = MallocHrfPack();
	if (pack==NULL)
	{
		return false;
	}
	pack->arg=0;
	pack->call=0;
	pack->cmd=CMD_SEND_CONCERT;
	HRF_SendFrame *pFrame = (HRF_SendFrame *)pack->data;

	//开始组包
	pFrame->option = g_PlmConfigPara.hrf_option;
	pFrame->channel = g_PlmConfigPara.hrf_channel;
	pFrame->BE=1;
	pFrame->Len=pdu_len;
	memcpy(pFrame->data,pdu_data,pdu_len);
	pack->len=pdu_len+sizeof(HRF_SendFrame);
	OSQPost(&HrfCsmaSendQ, pack, 0, OS_OPT_POST_1, &err);
	HRF_dbg_info.send_coor_count++;
	if(err!=OS_ERR_NONE)
	{
		FreeHrfPack(pack);
		return false;
	}
	return true;

}
void HRF_CmdPack(SPI_CMD cmd,u8 *data,u16 len,CSMA_CALL_BACK call)
{
	HRF_Pack* pack=MallocHrfPack();
	OS_ERR err;
	if(pack)
	{
		pack->cmd=cmd;

		pack->arg=0;
		pack->len=len;
		pack->call=call;
		memcpy(pack->data, data, len);
		HRF_dbg_info.cmd_count++;
		//*(u32*)(&((u8*)pack)[pack->len])=sum4crc(pack,pack->len);
		OSTaskQPost(&g_pOsRes->APP_HRF_Send_TCB, pack, 0, OS_OPT_POST_1, &err);
		if(err!=OS_ERR_NONE)
		{
			FreeHrfPack(pack);
		}
		else
		{
			OSSemPost(&MutiEventSem, OS_OPT_POST_1, &err);
		}
	}
}
//HRF发送信标
extern CSMA_SLOT_INFO s_slot_info;
bool HRF_SendBeacon(u8* beacon,u16 len,u8 rfType,u32 ntbStart,u32 ntbStop)
{
	if (len>(520+16-4-3))//发送长度超限无法发送
	{
		return false;
	}
	HRF_Pack *pack = MallocHrfPack();
	if (!pack)
	{
		return false;
	}
	HRF_dbg_info.send_beacon_count++;
	pack->cmd = CMD_SEND_BEACON;
	pack->call=NULL;
	pack->arg=0;
	HRF_SendFrame *pFrame = (HRF_SendFrame *)pack->data;
	//开始组包
	pFrame->option = g_PlmConfigPara.hrf_option;
	pFrame->channel = g_PlmConfigPara.hrf_channel;
	pFrame->SendNtbStart = ntbStart;
	pFrame->SendNtbEnd = ntbStop;
	pFrame->MCS=HRFPhrBestMcs(g_PlmConfigPara.hrf_option, 0);
	pFrame->BE=0;
	
	
	if (rfType==LINK_CSMA||rfType==LINK_RETRENCH||rfType==LINK_RETRENCH_SAME_TIME)//发送精简信标
	{
		
		pFrame->Len=sizeof(BEACON_RETRENCH_MPDU)+sizeof(BEACON_RETRENCH_STATION)+1+1+1+4+3;//+1信标条目数+1信标条目头+1信标条目长度+crc32+crc24
		pFrame->Len=GetPbSize(pFrame->Len)+sizeof(MPDU_CTL);
		pack->len=pFrame->Len+sizeof(HRF_SendFrame);
		BEACON_RETRENCH_MPDU *pBeacon = (BEACON_RETRENCH_MPDU *)&pFrame->data[16];
		memcpy(pFrame->data, beacon, sizeof(BEACON_RETRENCH_MPDU)+sizeof(MPDU_CTL));
		pBeacon->Retrench=1;
		BEACON_MANAGER_MSG *pBeaconMng = (BEACON_MANAGER_MSG *)pBeacon->Data;
		pBeaconMng->BeaconNum = 1;


		//站点能力
		pBeaconMng->Data[0] = BEACON_ITEM_RF_RETRENCH;
		pBeaconMng->Data[1] = sizeof(BEACON_RETRENCH_STATION) + 2;
		BEACON_RETRENCH_STATION *pAbility = (BEACON_RETRENCH_STATION *)&pBeaconMng->Data[2];
		pAbility->TEI = TEI_CCO;
		pAbility->PCOTei = 0;
		memcpy_swap(pAbility->MacAddr, beacon_param_current.cco_mac, sizeof(beacon_param_current.cco_mac));
		pAbility->Role = ROLES_CCO;
		pAbility->Level = 0;        //层级数
		pAbility->RfHop=0;
		pAbility->Reserve=0;
		pAbility->CsmaStart = s_slot_info.csma_start_ntb;
		pAbility->CsmaLen= 0;
		for (int i=0;i<3;i++)
		{
			pAbility->CsmaLen+=beacon_param_current.csma_slot_phase_ms[i];
		}
		for (int i=0;i<3;i++)
		{
			pAbility->CsmaLen+=beacon_param_current.tie_csma_slot_phase_ms[i];
		}
	}
	else//发送标准信标
	{
		pFrame->Len=GetPbSize(len-16+3+4)+16;
		pack->len=pFrame->Len+sizeof(HRF_SendFrame);
		memcpy(pFrame->data, beacon, len);
	}
	CreateMACCrc32(&pFrame->data[sizeof(MPDU_CTL)], pFrame->Len-4-3-16);
	MPDU_CTL *mpduCtl = (MPDU_CTL *)pFrame->data;
	BEACON_HRF_ZONE *beaconZone = (BEACON_HRF_ZONE *)mpduCtl->Zone0;
	memset(beaconZone, 0, sizeof(mpduCtl->Zone0));
	beaconZone->SrcTei=CCO_TEI;
	beaconZone->MCS=HRFBestMcs(g_PlmConfigPara.hrf_option, 0);
	beaconZone->PBSize = GetPbIndex(pFrame->Len - 16);
	OS_ERR err;
	if(pack)
	{
		OSQPost(&HrfBeacSendQ, pack, 0, OS_OPT_POST_1, &err);
		if(err!=OS_ERR_NONE)
		{
			FreeHrfPack(pack);
			return false;
		}
		else
		{
			OSSemPost(&MutiEventSem, OS_OPT_POST_1, &err);
		}
		return true;
	}
	return false;
}
GPIO_TypeDef*pgpio=(GPIO_TypeDef*)0x40104000;
typedef enum
{
	SPI_StatusIdel,
	SPI_StatusSend,
	SPI_StatusFindNTB,
	SPI_StatusTxWait,
	SPI_StatusGetStatus,
}SPI_Status_e;
extern SLOT_INFO SlotInfo;
void SPI_InterfaceTask(void *arg)
{
	OS_MSG_SIZE msg_size;
	OS_ERR err;
	SPI_Status_e SendStatus=SPI_StatusIdel;
	HRF_Pack *SendPack=NULL;//发送帧用的指针
	u32 FrameLenMS;
	GPIO_InitTypeDef GPIO_InitStruct;	


	#ifdef RESET_HRF_PORT
	GPIO_InitStruct.GPIO_Pin = RESET_HRF_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPP;
	GPIO_Init(RESET_HRF_PORT,&GPIO_InitStruct);
	GPIO_ResetBits(RESET_HRF_PORT, RESET_HRF_PIN);
	#endif

	GPIO_InitStruct.GPIO_Pin = SPI_SS_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OPP;
	GPIO_Init(SPI_SS_PORT,&GPIO_InitStruct);
	GPIO_ResetBits(SPI_SS_PORT, SPI_SS_PIN);

	GPIO_InitStruct.GPIO_Pin = EVENT_REQ_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(EVENT_REQ_PORT,&GPIO_InitStruct);
	SCU_GPIOInputConfig(EVENT_REQ_PORT,EVENT_REQ_PIN);
	//GPIO引脚中断配置
	GPIO_SetIRQTriggerMode(EVENT_REQ_PORT, EVENT_REQ_PIN, GPIO_Trigger_Rising);
	if(EVENT_REQ_PORT == GPIO0)
	{
		BPS_AddGpio0Irq(EVENT_REQ_PIN,SPI_IrqEvent);
	}
	else if(EVENT_REQ_PORT == GPIO1)
	{
		BPS_AddGpio1Irq(EVENT_REQ_PIN,SPI_IrqEvent);
	}		
	


	GPIO_InitStruct.GPIO_Pin = SEND_BUSY_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(SEND_BUSY_PORT,&GPIO_InitStruct);
	SCU_GPIOInputConfig(SEND_BUSY_PORT,SEND_BUSY_PIN);

	GPIO_InitStruct.GPIO_Pin = SPI_BUSY_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(SPI_BUSY_PORT,&GPIO_InitStruct);
	SCU_GPIOInputConfig(SPI_BUSY_PORT,SPI_BUSY_PIN);
	//GPIO引脚中断配置
	GPIO_SetIRQTriggerMode(SPI_BUSY_PORT, SPI_BUSY_PIN, GPIO_Trigger_Falling);
	if(SPI_BUSY_PORT == GPIO0)
	{
		BPS_AddGpio0Irq(SPI_BUSY_PIN,SPI_IrqBusy);
	}
	else if(SPI_BUSY_PORT == GPIO1)
	{
		BPS_AddGpio1Irq(SPI_BUSY_PIN,SPI_IrqBusy);
	}	

	SPI_Open(10000000);

	OSSemCreate(&MutiEventSem, "muti-event", 0, &err);
	OSSemCreate(&SPI_BusySem, "spi busy", 0, &err);
	OSQCreate(&SPI_ResultQ, "SPI Result", 5, &err);
	OSQCreate(&HrfCsmaSendQ, "Hrf Csma", 5, &err);
	OSQCreate(&HrfBeacSendQ, "Hrf beacon", 2, &err);
	u8 hrf_need_wait=0;
	HRF_Pack *pack=NULL;
	CheckCurrentHrfCodeSection();

	#ifdef RESET_HRF_PORT
	GPIO_SetBits(RESET_HRF_PORT, RESET_HRF_PIN);
	#endif

	if (total_bytes==0)//没有得到hrf的代码 不执行HRF交互
	{
		for (;;)
		{
			//进行回收所有的数据和命令的空间
			pack = OSQPend(&HrfBeacSendQ, 0, OS_OPT_PEND_NON_BLOCKING, &msg_size, NULL, &err);
			FreeHrfPack(pack);
			SendPack = (HRF_Pack *)OSQPend(&HrfCsmaSendQ, 0, OS_OPT_PEND_NON_BLOCKING, &msg_size, NULL, &err);
			FreeHrfPack(SendPack);
			SendPack = (HRF_Pack *)OSTaskQPend(0, OS_OPT_PEND_NON_BLOCKING, &msg_size, NULL, &err);
			FreeHrfPack(SendPack);
			OSSemPend(&MutiEventSem,10, OS_OPT_PEND_BLOCKING, NULL,&err);
		}
	}
	for(;;)
	{
NEW_LOOP:
		hrf_need_wait=0;
		pack=NULL;
		if(HRF_BootReq()&&GPIO_ReadInputDataBit(EVENT_REQ_PORT,EVENT_REQ_PIN))//发送从设备code  暂时无需重发
		{
			SPI_SetPolarity(0);//SPI有个bug导致bootloader和正式的极性不相同因此需要增加此项
			OSSemSet(&SPI_BusySem,0,&err);//清除busy信号量
			SPI_TransData(CMD_BOOT_START, 0, NULL, 0,SPI_StatusCallBack);
			WaitSpiBusy();
			for(int i = 0; i < total_bytes; i += 2048)
			{
				OSSemSet(&SPI_BusySem,0,&err);//清除busy信号量
				SPI_TransData(CMD_BOOT_FILE, i, NULL, 0,SPI_StatusCallBack);
				WaitSpiBusy();
			}
			OSSemSet(&SPI_BusySem,0,&err);//清除busy信号量
			SPI_TransData(CMD_BOOT_END, 0, NULL, 0,SPI_StatusCallBack);
			WaitSpiBusy();
			HRF_SetBootFlag(0);//关闭hrf code请求
			OSTimeDly(500, OS_OPT_TIME_DLY, &err);
			SPI_SetPolarity(1);//SPI有个bug导致bootloader和正式的极性不相同因此需要增加此项
			HRF_SetIndex( g_PlmConfigPara.hrf_channel,g_PlmConfigPara.hrf_option,10);
			HRF_SetTei(CCO_TEI, g_pPlmConfigPara->net_id);
		}
		else
		{

			//发送信标帧
			if (GPIO_ReadInputDataBit(SEND_BUSY_PORT, SEND_BUSY_PIN)&&!GPIO_ReadInputDataBit(SPI_BUSY_PORT, SPI_BUSY_PIN))
			{
				pack = OSQPend(&HrfBeacSendQ, 0, OS_OPT_PEND_NON_BLOCKING, &msg_size, NULL, &err);
				if(err==OS_ERR_NONE)//需要发送信标帧
				{
					//组信标帧
					HRF_SendFrame *pFrame = (HRF_SendFrame *)pack->data;
					debug_str(DEBUG_LOG_NET, "B cur is %x start %x end %x \n\r", BPLC_GetNTB(), pFrame->SendNtbStart, pFrame->SendNtbEnd);					
					SPI_TransData(CMD_SEND_BEACON, 0, pack->data, pack->len,SPI_StatusCallBack);
					WaitSpiBusy();
					FreeHrfPack(pack);
					pack=NULL;
					goto NEW_LOOP;//返回
				}
			}

			//接收帧
			if(GPIO_ReadInputDataBit(EVENT_REQ_PORT, EVENT_REQ_PIN)&&!GPIO_ReadInputDataBit(SPI_BUSY_PORT, SPI_BUSY_PIN))//有事件产生
			{
				SPI_TransData(CMD_GET_FRAME, 0, NULL, 0,SPI_StatusCallBack);
				WaitSpiBusy();
				goto NEW_LOOP;
			}

			if (!GPIO_ReadInputDataBit(SPI_BUSY_PORT, SPI_BUSY_PIN))
			{
				//发送命令
				HRF_Pack *pack = (HRF_Pack *)OSTaskQPend(0, OS_OPT_PEND_NON_BLOCKING, &msg_size, NULL, &err);
				if(err==OS_ERR_NONE)
				{
					int i=0;
					OSQFlush(&SPI_ResultQ, &err);//清除等待队列
					for(; i < 3; i++) //发送3次
					{
						HRF_Callback = pack->call;
						OSSemSet(&SPI_BusySem,0,&err); //清除busy信号量
						if(pack->cmd == CMD_CONTROL||pack->cmd == HRF_GET_SYNC_NTB) //需要发送帧 先
						{
							SPI_TransData(pack->cmd, pack->arg, pack->data, pack->len, SPI_StatusCallBack);
							if(WaitSpiBusy())
							{
								break;
							}
						}			
					}
					FreeHrfPack(pack);
					if(i == 3)
					{
						if (HRF_Callback)
						{
							HRF_Callback(CSMA_SEND_FAILED); //发送失败
						}
					}
					else
					{
						goto NEW_LOOP;
					}

				}
			}

			//发送CSMA
			uint64_t BeginNTB, EndNTB;
			uint16_t PSDU_SYMBOL, PHR_SYMBOL;
			if (!GPIO_ReadInputDataBit(SPI_BUSY_PORT, SPI_BUSY_PIN))
			{
				switch (SendStatus)
				{
					case SPI_StatusIdel: //空闲状态寻找发送队列
						if (GPIO_ReadInputDataBit(SEND_BUSY_PORT, SEND_BUSY_PIN))
						{
							SendPack = (HRF_Pack *)OSQPend(&HrfCsmaSendQ, 0, OS_OPT_PEND_NON_BLOCKING, &msg_size, NULL, &err);
							if (err == OS_ERR_NONE)
							{
								SendStatus = SPI_StatusFindNTB;
								HRF_SendFrame *sendF = (HRF_SendFrame *)SendPack->data;
								if(IsCommTestMode(TEST_MODE_PLC_PACKET_TO_HRF, NULL))
								{
									sendF->MCS = (PLC_TO_RF_TESTMODE_PARAM & 0xF00) >> 8;
									PHR_SYMBOL = HRF_PhrSymbol((PLC_TO_RF_TESTMODE_PARAM & 0xF00) >> 8, g_PlmConfigPara.hrf_option);
									PSDU_SYMBOL = HRF_PsduSymbol((PLC_TO_RF_TESTMODE_PARAM & 0xF0) >> 4, g_PlmConfigPara.hrf_option, sendF->Len - 16);
								}
								else
								{
									sendF->MCS = HRFPhrBestMcs(g_PlmConfigPara.hrf_option, 0);
									PHR_SYMBOL = HRF_PhrSymbol(HRFPhrBestMcs(g_PlmConfigPara.hrf_option, 0), g_PlmConfigPara.hrf_option);
									PSDU_SYMBOL = HRF_PsduSymbol(HRFBestMcs(g_PlmConfigPara.hrf_option, 0), g_PlmConfigPara.hrf_option, sendF->Len - 16);
								}
								
								if (((MPDU_CTL *)sendF->data) ->Type > MPDU_TYPE_SOF)
								{
									PSDU_SYMBOL = 0;//选择确认帧和网间协调帧没有PB块
								}

								FrameLenMS = HRFrameLen(PHR_SYMBOL,PSDU_SYMBOL,g_PlmConfigPara.hrf_option);

								FrameLenMS = FrameLenMS / 10;
								goto NEW_LOOP;
							}
							else
							{
								hrf_need_wait=1;
							}
						}
						else
						{
							hrf_need_wait=1;
						}
						break;
					case SPI_StatusFindNTB: //寻找适合的NTB
						if(HPLC_GetCSMASlot(&s_slot_info, GetNtbCount64(), &BeginNTB, &EndNTB,  FrameLenMS)) //当前找到了合适的NTB
						{
							for(int i = 0; i < 3; i++) //发送3次
							{
								HRF_Callback = SendPack->call;
								OSSemSet(&SPI_BusySem,0,&err); //清除busy信号量
								if(SendPack->cmd == CMD_SEND_FRAME||SendPack->cmd == CMD_SEND_CONCERT) //需要发送帧 
								{

									HRF_SendFrame *pFrame = (HRF_SendFrame *)SendPack->data;
									pFrame->SendNtbStart=BeginNTB;
									pFrame->SendNtbEnd=EndNTB;

									SPI_TransData(SendPack->cmd, 0, SendPack->data, SendPack->len, SPI_StatusCallBack);
									if(WaitSpiBusy())
									{
										break;
									}
								}
							}
							if(SendPack->arg&0xF)//arg 最后4位为重发次数
							{
								SendPack->arg--;
							}
							SendStatus = SPI_StatusTxWait;
							goto NEW_LOOP;
						}
						else //可以处理其他的任务
						{
							hrf_need_wait=1;
							break;
						}
					case SPI_StatusTxWait:
						if(GPIO_ReadInputDataBit(SEND_BUSY_PORT, SEND_BUSY_PIN)) //发送是否完成
						{
							GPIO_InitTypeDef GPIO_InitStruct;
							GPIO_InitStruct.GPIO_Pin = SEND_BUSY_PIN;
							GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD;
							GPIO_Init(SEND_BUSY_PORT,&GPIO_InitStruct);
							OSTimeDly(1, OS_OPT_TIME_DLY, &err);
							if(!GPIO_ReadInputDataBit(SEND_BUSY_PORT, SEND_BUSY_PIN)) //send busy 管脚为高阻态代表发送失败
							{
								SendStatus=SPI_StatusGetStatus;
							}
							else //发送成功
							{
								FreeHrfPack(SendPack);
								SendPack = NULL;
								SendStatus = SPI_StatusIdel;
							}
							//重新改为输入上拉状态
							GPIO_InitStruct.GPIO_Pin = SEND_BUSY_PIN;
							GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
							GPIO_Init(SEND_BUSY_PORT,&GPIO_InitStruct);
							goto NEW_LOOP;
						}
						else //发送尚未完成 可以处理其他的任务
						{
							hrf_need_wait=1;
							break;
						}

						
					case SPI_StatusGetStatus://等待读取状态
						if((SendPack->arg&0xf)==0)//arg 为重试次数
						{
							if (SendPack->arg&(u32)(1<<31))//arg 最高位为是否需要转向HPLC发送
							{
								PLC_SEND_PARAM SendParam;
								u16 mac_frame_len=0;
								MPDU_CTL *mpduCtl = (MPDU_CTL *)SendPack->data;
								SOF_HRF_ZONE *hrfZone = (SOF_HRF_ZONE*)mpduCtl->Zone0;

								MacFrameGetMsduPtr( (P_MAC_FRAME)&SendPack->data[sizeof(MPDU_CTL)+1],&mac_frame_len);
								SOF_PARAMS SOFParam;
								SOFParam.BroadcastFlag = hrfZone->DstTei == MAC_BROADCAST_TEI?0:1;
								SOFParam.DstTei=hrfZone->DstTei;
								SOFParam.Lid = hrfZone->LinkFlag;
								SOFParam.NID = mpduCtl->NID;
								PDU_BLOCK* pdu=MpduCreateSOFFrame(&SOFParam, &SendPack->data[sizeof(MPDU_CTL)+1],  mac_frame_len);
								if (pdu)
								{									
									SendParam.doubleSend = 0;
									SendParam.resend_count = 1;
									SendParam.cb = SendPack->call;
									SendParam.send_type = CSMA_SEND_TYPE_CSMA;
									u16 DstTei = hrfZone->DstTei;
									PLC_GetSendParam(DstTei, &DstTei, &SendParam.send_phase, &SendParam.link);
									SendParam.link = FreamPathBplc;
									HPLC_SendSOFFrame(&SendParam, DstTei, SendPack->data, SendPack->len);
									FreePDU(pdu);
								}
								
							}
							FreeHrfPack(SendPack);
							SendPack = NULL;
							SendStatus = SPI_StatusIdel;
						}
						else
						{
							HRF_SendFrame *head=(HRF_SendFrame *)SendPack->data;
							MPDU_CTL *mpduCtl = (MPDU_CTL *)head->data;
							if (mpduCtl->Type==MPDU_TYPE_SOF)
							{
								SOF_ZONE *sofZone = (SOF_ZONE *)mpduCtl->Zone0;
								sofZone->ReSendFlag=1;
							}
							SendStatus = SPI_StatusFindNTB;
						}
						break;//继续执行其他任务

				}
			}
			else
			{
				hrf_need_wait=1;
			}
			
			


			

			

			//没有发送任务  等待一会
			if(hrf_need_wait)
				if((HrfBeacSendQ.MsgQ.NbrEntries==0&&g_pOsRes->APP_HRF_Send_TCB.MsgQ.NbrEntries==0)
				   //以下两种为异常情况
				   ||GPIO_ReadInputDataBit(SPI_BUSY_PORT, SPI_BUSY_PIN)||!GPIO_ReadInputDataBit(SEND_BUSY_PORT, SEND_BUSY_PIN))
			{
				//等待接收任务
				//开使能开关
				GPIO_ClearIRQStatus(EVENT_REQ_PORT, EVENT_REQ_PIN);
				GPIO_SetIRQMode(EVENT_REQ_PORT, EVENT_REQ_PIN,1);
				OSSemSet(&MutiEventSem,0,&err);
				//开启GPIO中断保证有事件产生时立即响应
				GPIO_SetIRQMode(EVENT_REQ_PORT, EVENT_REQ_PIN, ENABLE);
				OSSemPend(&MutiEventSem,2, OS_OPT_PEND_BLOCKING, NULL,&err);//等待接收 从设备事件请求 
				GPIO_SetIRQMode(EVENT_REQ_PORT, EVENT_REQ_PIN, DISABLE);
				GPIO_SetIRQMode(EVENT_REQ_PORT, EVENT_REQ_PIN,0);
			}
		}
	}
}
void HRF_SetIndex(u16 channel,u8 option,u8 power)
{
	u8 data[5] = {
		HRF_SET_CHANNEL,
		3,
		channel,
		option,
		power
	};
	//发送更改频段命令
	HRF_CmdPack(CMD_CONTROL, data, sizeof(data), NULL);
}

void HRF_SetPll(int ppm,u32 NTB)
{
	u8 data[14] = { HRF_SET_PLL ,12};
	static u32 SetPllTime=0;
	SetPllTime++;
	memcpy(&data[2], &SetPllTime, 4);
	memcpy(&data[6], &ppm, 4);
	memcpy(&data[10], &NTB, 4);
	//发送更改频段命令
	HRF_CmdPack(CMD_CONTROL, data, sizeof(data), NULL);
}
void HRF_SetTei(u16 tei, u32 NID)
{
	u8 data[8] = { HRF_SET_TEI ,6};
	memcpy(&data[2], &tei, 2);
	memcpy(&data[4], &NID, 4);
	HRF_CmdPack(CMD_CONTROL, data, sizeof(data), NULL);
}
void HRF_GetNtb(void (*callBack)(u32))
{
	u8 data[4] = {0,0,0,0};
	//发送更改频段命令
	HrfGetPara = callBack;
	HRF_CmdPack(HRF_GET_SYNC_NTB, data, sizeof(data), NULL);
}

void HRF_Reset(void)
{
	u8 data[1] = { HRF_RESERT};
	HRF_CmdPack(CMD_CONTROL, data, sizeof(data), NULL);
}
void HRF_SetTestMode(u32 mode)
{
	u8 data[6] = { HRF_SET_TEST_MODE ,4};
	//发送更改频段命令
	memcpy(&data[2],&mode,4);
	HRF_CmdPack(CMD_CONTROL, data, sizeof(data), NULL);
}
#include"system_inc.h"
void CheckCurrentHrfCodeSection(void)
{

	P_FLASH_USER_CHARACTER_FIELD usersection1;
	P_FLASH_USER_CHARACTER_FIELD usersection2;

	usersection1 = (P_FLASH_USER_CHARACTER_FIELD)(FLASH_HRF_CODE1_ADDRESS+0x12000000);
	usersection2 = (P_FLASH_USER_CHARACTER_FIELD)(FLASH_HRF_CODE2_ADDRESS+0x12000000);

	if ((usersection1->character == 0x0123abcd)
		&& (Get_checksum_32((unsigned long *)usersection1, sizeof(FLASH_USER_CHARACTER_FIELD)  / 4 -1) ==  (usersection1->field_checksum)))
	{
		if ((usersection2->character == 0x0123abcd)
			&& (Get_checksum_32((unsigned long *)usersection2, sizeof(FLASH_USER_CHARACTER_FIELD) / 4 -1) ==  (usersection2->field_checksum)))
		{
			if (usersection1->version >= usersection2->version)
			{
				bin_data = (u8 *)(usersection1 + 1);
				total_bytes = usersection1->code_size;
				bin_crc = sum4crc(bin_data, total_bytes);
				if (bin_crc == usersection1->code_checksum)
				{
					return;
				}
				else
				{
					bin_data = (u8 *)(usersection2 + 1);
					total_bytes = usersection2->code_size;
					bin_crc = sum4crc(bin_data, total_bytes);
					if (bin_crc == usersection2->code_checksum)
					{
						return;
					}
					else
					{
						total_bytes = 0; //没有代码设置代码大小为0
					}
				}

			}
			else
			{
				bin_data = (u8 *)(usersection2 + 1);
				total_bytes = usersection2->code_size;
				bin_crc = sum4crc(bin_data, total_bytes);
				if (bin_crc == usersection2->code_checksum)
				{
					return;
				}
				else
				{
					bin_data = (u8 *)(usersection1 + 1);
					total_bytes = usersection1->code_size;
					bin_crc = sum4crc(bin_data, total_bytes);
					if (bin_crc == usersection1->code_checksum)
					{
						return;
					}
					else
					{
						total_bytes = 0; //没有代码设置代码大小为0
					}
				}

			}
		}
		else
		{
			bin_data = (u8 *)(usersection1 + 1);
			total_bytes = usersection1->code_size;
			bin_crc = sum4crc(bin_data, total_bytes);
			if (bin_crc == usersection1->code_checksum)
			{
				return;
			}
			else
			{
				total_bytes = 0; //没有代码设置代码大小为0
			}
		}
	}
	else if ((usersection2->character == 0x0123abcd)
			 && (Get_checksum_32((unsigned long *)usersection2, sizeof(FLASH_USER_CHARACTER_FIELD)  / 4-1) ==  (usersection2->field_checksum)))
	{
		bin_data = (u8 *)(usersection2 + 1);
		total_bytes = usersection2->code_size;
		bin_crc = sum4crc(bin_data, total_bytes);
		if (bin_crc == usersection2->code_checksum)
		{
			return;
		}
		else
		{
			total_bytes = 0; //没有代码设置代码大小为0
		}
	}
	else
	{
		bin_data = (u8 *)(usersection1 + 1);
		total_bytes = 0; //没有代码设置代码大小为0
	}

}
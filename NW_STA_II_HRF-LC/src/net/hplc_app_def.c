#include "system_inc.h"
#include "plc_autorelay.h"
#ifdef HPLC_CSG
P_APP_PACKET HplcApp_MakeReadFrame(u16 packet_seq, u8 *pdata, u16 len, u16 *p_frame_len)
{
	u16 frame_len = sizeof(APP_PACKET) - 1 + sizeof(APP_BUSINESS_PACKET) - 1 + sizeof(READER_CCO_FRAME) + len;

	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,frame_len);

	if (NULL == pFrame)
		return pFrame;

	memset_special(pFrame, 0, frame_len);

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_CSG;
	pFrame->Ctl = 0;

	P_APP_BUSINESS_PACKET pBusinessPacket = (P_APP_BUSINESS_PACKET)pFrame->Data;

	pBusinessPacket->Ctrl.FrameType = 4;
	pBusinessPacket->Ctrl.ExtFieldFlag = 0;
	pBusinessPacket->Ctrl.ResponseFlag = EM_BUSINESS_NORESPONSE;
	pBusinessPacket->Ctrl.StartFlag = 1;
	pBusinessPacket->Ctrl.Direction = 0;
	pBusinessPacket->Ctrl.Reserve = 0;

	pBusinessPacket->BusinessID = 0x00;
	pBusinessPacket->Version = 1;
	pBusinessPacket->FrameSeq = packet_seq;
	pBusinessPacket->FrameLen = sizeof(READER_CCO_FRAME) + len;

	READER_CCO_FRAME *pData = (READER_CCO_FRAME *)pBusinessPacket->Data;

	memcpy_special(pData->data, pdata, len);     //memcpy_special       memcpy_swap
	pData->len = len;
	pData->seq = g_pRelayStatus->seqReader;
	pData->type=0;
	if (NULL != p_frame_len)
		*p_frame_len = frame_len;

	return pFrame;
}

P_APP_PACKET HplcApp_MakeDataTransFrame(u8 src_addr[6], u8 dst_addr[6],u8 app_code, u8 meter_moudel, u16 packet_seq, EM_BUSINESS_RESPONSE_FLAG response_flag, u8 *pdata, u16 len, u16 *p_frame_len)
{
	u16 frame_len = sizeof(APP_PACKET) - 1 + sizeof(APP_BUSINESS_PACKET) - 1 + sizeof(APP_DATA_TRANS_DOWN) - 1 + len;
#ifdef	PROTOCOL_NW_2020
	if (meter_moudel && (meter_moudel != 2)) //2表示电能表精准校时
	{
		frame_len = sizeof(APP_PACKET) - 1 + sizeof(APP_BUSINESS_PACKET) - 1 + sizeof(APP_DATA_TRANS_MOUDLE_DOWN) - 1 + len;
	}
#endif
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,frame_len);

	if (NULL == pFrame)
		return pFrame;

	memset_special(pFrame, 0, frame_len);

#ifdef	PROTOCOL_NW_2020
	if (meter_moudel)
	{
		pFrame->Port = HPLC_APP_PORT_RCU;
	}
	else
#endif
	{
		pFrame->Port = HPLC_APP_PORT_COMMON;
	}
	pFrame->PacketID = APP_PACKET_CSG;
	pFrame->Ctl = 0;

	P_APP_BUSINESS_PACKET pBusinessPacket = (P_APP_BUSINESS_PACKET)pFrame->Data;

	pBusinessPacket->Ctrl.FrameType = 1;
	pBusinessPacket->Ctrl.ExtFieldFlag = 0;
	pBusinessPacket->Ctrl.ResponseFlag = (response_flag);
	pBusinessPacket->Ctrl.StartFlag = 1;
	pBusinessPacket->Ctrl.Direction = 0;
	pBusinessPacket->Ctrl.Reserve = 0;

	if (meter_moudel)
	{
		pBusinessPacket->BusinessID = 0x01;
	}
	else
	{
		pBusinessPacket->BusinessID = 0x00;
	}
	pBusinessPacket->Version = 1;
	pBusinessPacket->FrameSeq = packet_seq;
#ifdef	PROTOCOL_NW_2020
	if (meter_moudel && (meter_moudel != 2)) //2表示电能表精准校时
	{
		pBusinessPacket->FrameLen = sizeof(APP_DATA_TRANS_MOUDLE_DOWN) - 1 + len;
	}
	else
#endif
	{
		pBusinessPacket->FrameLen = sizeof(APP_DATA_TRANS_DOWN) - 1 + len;
	}

	P_APP_DATA_TRANS_DOWN pDataTransPacket = (P_APP_DATA_TRANS_DOWN)pBusinessPacket->Data;
#ifdef	PROTOCOL_NW_2020
	P_APP_DATA_TRANS_MOUDLE_DOWN pMoudleTransPacket = (P_APP_DATA_TRANS_MOUDLE_DOWN)pBusinessPacket->Data;
	P_APP_DATA_TRANS_MOUDLE_TIMING_DOWN pMoudleTimingTransPacket = (P_APP_DATA_TRANS_MOUDLE_TIMING_DOWN)pBusinessPacket->Data;
#endif

	memcpy_special(pDataTransPacket->SrcAddr, src_addr, 6);     //memcpy_special       memcpy_swap
//	memset_special(pDataTransPacket->SrcAddr, 0, 6);
	memcpy_special(pDataTransPacket->DstAddr, dst_addr, 6);

	pDataTransPacket->Timeout = 18;
	//pDataTransPacket->Timeout = 0;
	pDataTransPacket->Reserve = 0;
	
	
#ifdef	PROTOCOL_NW_2020
	if (meter_moudel)
	{
		if (meter_moudel == 2) //2表示电能表精准校时
		{
			pMoudleTimingTransPacket->Reserve = 0;
			pMoudleTimingTransPacket->App_Code = app_code;
			pMoudleTimingTransPacket->DataLen = len;
			memcpy_special(pMoudleTimingTransPacket->Data, pdata, len);
		}
		else
		{
			pMoudleTransPacket->Reserve = 0;
			pMoudleTransPacket->App_Code = app_code;
			pMoudleTransPacket->DataLen = len;
			memcpy_special(pMoudleTransPacket->Data, pdata, len);
		}
	}
	else
#endif
	{		
		pDataTransPacket->DataLen = len;
		memcpy_special(pDataTransPacket->Data, pdata, len);
	}

	if (NULL != p_frame_len)
		*p_frame_len = frame_len;

	return pFrame;
}
P_APP_PACKET HplcApp_MakeConfirmFrame( u8 business_id, u16 packet_seq,u8 reason, u16 *p_frame_len)
{
	u16 frame_len = sizeof(APP_PACKET) - 1 + sizeof(APP_BUSINESS_PACKET);

	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,frame_len);

	if (NULL == pFrame)
		return pFrame;

	memset_special(pFrame, 0, frame_len);


	pFrame->Port = HPLC_APP_PORT_COMMON;

	pFrame->PacketID = APP_PACKET_CSG;
	pFrame->Ctl = 0;

	P_APP_BUSINESS_PACKET pBusinessPacket = (P_APP_BUSINESS_PACKET)pFrame->Data;

	pBusinessPacket->Ctrl.FrameType = 0;//确认/否认帧
	pBusinessPacket->Ctrl.ExtFieldFlag = 0;
	pBusinessPacket->Ctrl.ResponseFlag = 0;
	pBusinessPacket->Ctrl.StartFlag = 0;
	pBusinessPacket->Ctrl.Direction = 0;
	pBusinessPacket->Ctrl.Reserve = 0;

	pBusinessPacket->BusinessID = business_id;
	pBusinessPacket->Version = 1;
	pBusinessPacket->FrameSeq = packet_seq;
	if (business_id==0)//确认无数据
	{
		pBusinessPacket->FrameLen = 0;
		--frame_len;
	}
	else
	{
		pBusinessPacket->FrameLen = 1;
		pBusinessPacket->Data[0] = reason;
	}

	if (NULL != p_frame_len)
		*p_frame_len = frame_len;

	return pFrame;
}
P_APP_PACKET HplcApp_MakeCmdFrame(u16 packet_seq, EM_BUSINESS_RESPONSE_FLAG response_flag, EM_CMD_BUSINESS_ID business_id, u8 *pdata, u16 len, u16 *p_frame_len)
{
	u16 frame_len = sizeof(APP_PACKET) - 1 + sizeof(APP_BUSINESS_PACKET) - 1  + len;

	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,frame_len);

	if (NULL == pFrame)
		return pFrame;

	memset_special(pFrame, 0, frame_len);

	if (business_id==EM_CMD_BUSINESS_STA_RUN_STATUS||business_id==EM_CMD_BUSINESS_STA_CHANNEL_INFO||EM_CMD_BUSINESS_BROAD_TIME==business_id)
	{
		pFrame->Port = HPLC_APP_PORT_RCU;
	}
	else
	{
		pFrame->Port = HPLC_APP_PORT_COMMON;
	}
	pFrame->PacketID = APP_PACKET_CSG;
	pFrame->Ctl = 0;

	P_APP_BUSINESS_PACKET pBusinessPacket = (P_APP_BUSINESS_PACKET)pFrame->Data;

	pBusinessPacket->Ctrl.FrameType = 2;
	pBusinessPacket->Ctrl.ExtFieldFlag = 0;
	pBusinessPacket->Ctrl.ResponseFlag = (response_flag);
	pBusinessPacket->Ctrl.StartFlag = 1;
	pBusinessPacket->Ctrl.Direction = 0;
	pBusinessPacket->Ctrl.Reserve = 0;

	pBusinessPacket->BusinessID = business_id;
	pBusinessPacket->Version = 1;
	pBusinessPacket->FrameSeq = packet_seq;
	pBusinessPacket->FrameLen = len;

	if ((NULL != pdata) && (len > 0))
		memcpy_special(pBusinessPacket->Data, pdata, len);

	if (NULL != p_frame_len)
		*p_frame_len = frame_len;

	return pFrame;
}

P_APP_PACKET HplcApp_MakeEventFrame(u16 packet_seq, EVENT_FUNC_CODE func, EVENT_DIRECTION dir, EVENT_START start, u8 meter_addr[6], u8 *pdata, u16 len)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1  + sizeof(APP_BUSINESS_PACKET) - 1 + sizeof(EVENT_PACKET) - 1 + len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_RCU;
	pFrame->PacketID = APP_PACKET_CSG;
	pFrame->Ctl = 0;

	P_APP_BUSINESS_PACKET pBusinessPacket = (P_APP_BUSINESS_PACKET)pFrame->Data;

	pBusinessPacket->Ctrl.FrameType = 3;
	pBusinessPacket->Ctrl.ExtFieldFlag = 0;
	pBusinessPacket->Ctrl.ResponseFlag = 0;
	pBusinessPacket->Ctrl.StartFlag = 1;
	pBusinessPacket->Ctrl.Direction = 0;
	pBusinessPacket->Ctrl.Reserve = 0;

	pBusinessPacket->BusinessID = 1;
	pBusinessPacket->Version = 1;
	pBusinessPacket->FrameSeq = packet_seq;
	pBusinessPacket->FrameLen = sizeof(EVENT_PACKET) - 1 + len;

	P_EVENT_PACKET pReadFrame = (P_EVENT_PACKET)pBusinessPacket->Data;
	memset_special(pReadFrame, 0, sizeof(EVENT_PACKET) - 1 + len);
	
	pReadFrame->HeadLen = sizeof(EVENT_PACKET) - 1;
	pReadFrame->Function = func; //确认
	pReadFrame->DataLen = len;
	pReadFrame->res = 0;
//	pReadFrame->Seq = packet_seq;
	memcpy_special(pReadFrame->MeterAddr, meter_addr, 6);


	return pFrame;
}
#else

P_APP_PACKET HplcApp_MakeReadFrame(u16 packet_seq, u8 *pdata, u16 len, u16 *p_frame_len)
{
	u16 frame_len = sizeof(APP_PACKET) - 1 + sizeof(READER_CCO_FRAME)  + len;

	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,frame_len);

	if (NULL == pFrame)
		return pFrame;

	memset_special(pFrame, 0, frame_len);

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_READER_CONNECT;
	pFrame->Ctl = 0;

	READER_CCO_FRAME *pData = (READER_CCO_FRAME *)pFrame->Data;

	memcpy_special(pData->data, pdata, len);     //memcpy_special       memcpy_swap
	pData->len = len;
	pData->type=0;
	if (NULL != p_frame_len)
		*p_frame_len = frame_len;

	return pFrame;
}

#ifdef GDW_ZHEJIANG
P_APP_PACKET HplcApp_MakeSetOnlineTimeLockFrame(u16 OnlineLockTime, u16 AbnormalOfflineLockTime, u16 FrameSN)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(APP_ONLINE_LOCK_TIME_PACKET) - 1 + sizeof(APP_ONLINE_LOCK_TIME));

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_BROADCAST_SET_SUBNODE_PARA;
	pFrame->Ctl = 0;

	P_APP_ONLINE_LOCK_TIME_PACKET pLockTimePacketFrame = (P_APP_ONLINE_LOCK_TIME_PACKET)pFrame->Data;
	P_APP_ONLINE_LOCK_TIME pLockTimeFrame = (P_APP_ONLINE_LOCK_TIME)pLockTimePacketFrame->data;
	
	memset(pLockTimePacketFrame,0,sizeof(APP_ONLINE_LOCK_TIME_PACKET)-1);
	pLockTimePacketFrame->protocol = 1;
	pLockTimePacketFrame->FrameHeadLen = sizeof(APP_ONLINE_LOCK_TIME_PACKET)-1;
	pLockTimePacketFrame->FrameSN = FrameSN;
	pLockTimePacketFrame->DataLen = sizeof(APP_ONLINE_LOCK_TIME);
	
	memset(pLockTimeFrame,0,sizeof(APP_ONLINE_LOCK_TIME));
	pLockTimeFrame->FunctionCode = 1;
	pLockTimeFrame->OnlineLockTime = OnlineLockTime;
	pLockTimeFrame->AbnormalOfflineLockTime = AbnormalOfflineLockTime;

	return pFrame;
}
#endif

P_APP_PACKET HplcApp_MakeReadMeterFrame(HPLC_APP_PACKET_ID packet_id, u16 packet_seq, u8 *pdata, u16 len, APP_METER_PRO protocol, u16 *p_frame_len)
{
	u16 frame_len = sizeof(APP_PACKET) - 1 + sizeof(READ_METER_DOWN) - 1 + len;

	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,frame_len);

	if (NULL == pFrame)
		return pFrame;

	memset_special(pFrame, 0, frame_len);

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = packet_id;
	pFrame->Ctl = 0;

	READ_METER_DOWN *pReadFrame = (READ_METER_DOWN *)pFrame->Data;

	pReadFrame->AppProtocol = 1;
	pReadFrame->HeadLen = sizeof(READ_METER_DOWN) - 1;

#ifdef GDW_2019_GRAPH_STOREDATA
	if (APP_PACKET_PARALLEL == packet_id || APP_PACKET_DATA_COLLECT == packet_id || APP_PACKET_STORE_DATA_PARALLEL == packet_id)
#else
	if (APP_PACKET_PARALLEL == packet_id || APP_PACKET_DATA_COLLECT == packet_id)
#endif
		pReadFrame->Config = (1 << 1) | (1 << 0) | 0;
	else
		pReadFrame->Config = 0;

	pReadFrame->Protocol = protocol;
	pReadFrame->DataLen = len;
	pReadFrame->PacketSeq = packet_seq;
	pReadFrame->Timeout = (len*2*10)/200+10;//(总长度*2)/200 + 1S  //200是在2400的波特率下1s传输的字节数2400/11=218 舍入为200
	pReadFrame->Timeout=pReadFrame->Timeout >25?pReadFrame->Timeout:25;
#ifdef GDW_2019_GRAPH_STOREDATA
	if (APP_PACKET_PARALLEL == packet_id || APP_PACKET_DATA_COLLECT == packet_id || APP_PACKET_STORE_DATA_PARALLEL == packet_id)
#else
	if (APP_PACKET_PARALLEL == packet_id || APP_PACKET_DATA_COLLECT == packet_id)
#endif
		pReadFrame->Item = 5;
	else
		pReadFrame->Item = 0;

	memcpy_special(pReadFrame->Data, pdata, pReadFrame->DataLen);

	if (NULL != p_frame_len)
		*p_frame_len = frame_len;

	return pFrame;
}

P_APP_PACKET HplcApp_MakeEventFrame(u16 packet_seq, EVENT_FUNC_CODE func, EVENT_DIRECTION dir, EVENT_START start, u8 meter_addr[6], u8 *pdata, u16 len)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(EVENT_PACKET) - 1 + len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_EVENT_REPORT;
	pFrame->Ctl = 0;

	P_EVENT_PACKET pReadFrame = (P_EVENT_PACKET)pFrame->Data;
	memset(pReadFrame,0,sizeof(EVENT_PACKET)-1);
	pReadFrame->AppProtocol = 1;
	pReadFrame->HeadLen = sizeof(EVENT_PACKET) - 1;

	pReadFrame->Direction = dir;
	pReadFrame->StartFlag = start;
	pReadFrame->Function = func;
	pReadFrame->DataLen = len;
	pReadFrame->Seq = packet_seq;
	memcpy_special(pReadFrame->MeterAddr, meter_addr, 6);

	if (len > 0)
		memcpy_special(pReadFrame->Data, pdata, pReadFrame->DataLen);

	return pFrame;
}
#endif



P_APP_PACKET HplcApp_MakeBroadcastTimingFrame(u8 *pdata, u16 len)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(TIMING_DOWN) - 1 + len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_TIMING;
	pFrame->Ctl = 0;

	P_TIMING_DOWN pTimingFrame = (P_TIMING_DOWN)pFrame->Data;

	pTimingFrame->AppProtocol = 1;
	pTimingFrame->HeadLen = sizeof(TIMING_DOWN) - 1;
	pTimingFrame->Reserve1 = 0;
	pTimingFrame->Reserve2 = 0;
	pTimingFrame->DataLen = len;

	memcpy_special(pTimingFrame->Data, pdata, len);

	return pFrame;
}

#ifdef ERROR_APHASE
P_APP_PACKET HplcApp_MakePhaseErrFrame(u8 cmd,u8 vaile,u8* mac,u16 seq)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(PHASE_ERR_SWITCH) + 1);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_PHASE_ERR_SWITCH;
	pFrame->Ctl = 0;

	P_PHASE_ERR_SWITCH pPhaseFrame = (P_PHASE_ERR_SWITCH)pFrame->Data;

	pPhaseFrame->AppProtocol = 1;
	pPhaseFrame->HeadLen = sizeof(PHASE_ERR_SWITCH);
	pPhaseFrame->Seq = seq;
	pPhaseFrame->Dir=0;
	pPhaseFrame->Start=1;
	memcpy_swap(pPhaseFrame->SrcMac,mac,6);
	pPhaseFrame->Reserve = 0;
	pPhaseFrame->Cmd=cmd;
	if (cmd==0)
	{
		pPhaseFrame->DataLen = 0;
	}
	else
	{
		pPhaseFrame->DataLen = 1;
		pPhaseFrame->Data[0] = vaile;
	}
	return pFrame;
}
#endif
P_APP_PACKET HplcApp_MakeAccurateTimingFrame(u8 *pdata, u16 len,u8 seq,u32 NTB)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(ACCURATE_TIMING_DOWN) - 1 + len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_ACCURATE_TIMING;
	pFrame->Ctl = 0;

	P_ACCURATE_TIMING_DOWN	pTimingFrame = (P_ACCURATE_TIMING_DOWN)pFrame->Data;

	pTimingFrame->AppProtocol=1;
	pTimingFrame->HeadLen=sizeof(ACCURATE_TIMING_DOWN) - 1;
	pTimingFrame->DataLen=len;
	pTimingFrame->Seq=seq;
	pTimingFrame->NTB=NTB;

	memcpy_special(pTimingFrame->Data, pdata, len);

	return pFrame;
}

P_APP_PACKET HplcApp_MakeStartSelfRegFrame(u32 packet_seq)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(REGIST_START_DOWN));

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_START_REGISTER;
	pFrame->Ctl = 0;

	P_REGIST_START_DOWN pStartRegFrame = (P_REGIST_START_DOWN)pFrame->Data;

	pStartRegFrame->AppProtocol = 1;
	pStartRegFrame->HeadLen = sizeof(REGIST_START_DOWN);
	pStartRegFrame->FocusReply = 0;
	pStartRegFrame->Parameter = 1;
	pStartRegFrame->Reserve1 = 0;
	pStartRegFrame->Seq = packet_seq;

	return pFrame;
}

P_APP_PACKET HplcApp_MakeReadSelfRegNodeFrame(u32 packet_seq, u8 src_mac[MAC_ADDR_LEN], u8 des_mac[MAC_ADDR_LEN],u8 param)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(REGIST_CHECK_DOWN));

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_CHECK_REGISTER;
	pFrame->Ctl = 0;

	P_REGIST_CHECK_DOWN pReadRegFrame = (P_REGIST_CHECK_DOWN)pFrame->Data;

	pReadRegFrame->AppProtocol = 1;
	pReadRegFrame->HeadLen = sizeof(REGIST_CHECK_DOWN);
	pReadRegFrame->FocusReply = 0;
	pReadRegFrame->Parameter = param;
	pReadRegFrame->Reserve1 = 0;
	pReadRegFrame->Seq = packet_seq;

	memcpy_swap(pReadRegFrame->SrcMac, src_mac, MAC_ADDR_LEN);
	memcpy_swap(pReadRegFrame->DstMac, des_mac, MAC_ADDR_LEN);

	return pFrame;
}

P_APP_PACKET HplcApp_MakeStopSelfRegFrame(u32 packet_seq)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(REGIST_STOP_DOWN));

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_STOP_REGISTER;
	pFrame->Ctl = 0;

	P_REGIST_STOP_DOWN pStopRegFrame = (P_REGIST_STOP_DOWN)pFrame->Data;

	pStopRegFrame->AppProtocol = 1;
	pStopRegFrame->HeadLen = sizeof(REGIST_STOP_DOWN);
	pStopRegFrame->Reserve1 = 0;
	pStopRegFrame->Reserve2 = 1;
	pStopRegFrame->Seq = packet_seq;

	return pFrame;
}
#ifdef HPLC_CSG
//南网下发文件信息
P_APP_PACKET HplcApp_MakeStartUpdateFrame(u32 update_id, u16 total_time,u16 fileType, u16 block_cnt, u32 file_size, u32 file_checksum,u16 *len, EM_BUSINESS_RESPONSE_FLAG ack)
{
	P_APP_PACKET pFrame = NULL;
	P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;
	
	u16 msdulen;
	msdulen = sizeof(GD_UPDATE_START_DOWN);
	P_GD_UPDATE_START_DOWN pStartUpdateFrame = (P_GD_UPDATE_START_DOWN)alloc_buffer(__func__,msdulen);

	if (pStartUpdateFrame == NULL)
	{
		return NULL;
	}

	pStartUpdateFrame->InfoId=GD_UPINFO_SEND_INFO;
	memset(pStartUpdateFrame->res,0,sizeof(pStartUpdateFrame->res));


	pStartUpdateFrame->FileType = fileType;
	pStartUpdateFrame->res2=0;

	if ((pcb->plan) && (pcb->broad))
	{
		memset(pStartUpdateFrame->DstAddr, 0x99, sizeof(pStartUpdateFrame->DstAddr));
	}
	else
	{
		P_STA_INFO psta = PLC_GetStaInfoByTei(pcb->tei);
		
		memcpy(pStartUpdateFrame->DstAddr, psta->mac, MAC_ADDR_LEN);
	}
	pStartUpdateFrame->Crc = file_checksum;
	pStartUpdateFrame->FileSize = file_size;
	pStartUpdateFrame->UpdateId = update_id;
	pStartUpdateFrame->BlockCount = block_cnt;
	pStartUpdateFrame->Timeout=total_time;


	pFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, ack, EM_CMD_BUSINESS_SEND_FILE,  (u8 *)pStartUpdateFrame, msdulen, &msdulen);
	free_buffer(__func__,pStartUpdateFrame);
	*len = msdulen;
	return pFrame;
}

P_APP_PACKET HplcApp_MakeStopUpdateFrame(u32 update_id,u16 *len)
{
	P_PLC_STA_UPDATE_CB pcb = &g_pRelayStatus->sta_update_cb;
	
	return HplcApp_MakeStartUpdateFrame(update_id, pcb->update_minutes, 0, pcb->old_block_count, pcb->old_file_size, pcb->old_file_checksum, len, EM_BUSINESS_NORESPONSE);
}
static P_APP_PACKET S_MakeUpdateDataFrame(GD_UP_INFO_ID idInfo,u32 update_id, u32 block_id, u8 *block_data, u32 block_size ,u32 block_tatal,u16 *len)
{
	P_APP_PACKET pFrame = NULL;
	u16 msdulen;
	msdulen = sizeof(GD_UPDATE_DATA_DOWN)+block_size;
	P_GD_UPDATE_DATA_DOWN pStartUpdateFrame = (P_GD_UPDATE_DATA_DOWN)alloc_buffer(__func__,msdulen);

	if (pStartUpdateFrame == NULL)
	{
		return NULL;
	}

	pStartUpdateFrame->InfoId=idInfo;
	memset(pStartUpdateFrame->res,0,sizeof(pStartUpdateFrame->res));
	
	pStartUpdateFrame->UpdateID = update_id;
	pStartUpdateFrame->BlockID = block_id;
	pStartUpdateFrame->BlockSize = block_size;
	pStartUpdateFrame->TotalBlockCnt = block_tatal;

	memcpy(pStartUpdateFrame->BlockData, block_data, block_size);


	pFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NORESPONSE, EM_CMD_BUSINESS_SEND_FILE,  (u8 *)pStartUpdateFrame, msdulen, &msdulen);
	free_buffer(__func__,pStartUpdateFrame);
	*len = msdulen;
	return pFrame;
}
P_APP_PACKET HplcApp_MakeUpdateDataFrame(u32 update_id, u32 block_id, u8 *block_data, u32 block_size ,u32 block_tatal,u16 *len)
{
	return S_MakeUpdateDataFrame(GD_UPINFO_SEND_DATE,update_id, block_id, block_data,  block_size , block_tatal, len);
}

P_APP_PACKET HplcApp_MakeUpdateDataBroadcastFrame(u32 update_id, u32 block_id, u8 *block_data, u32 block_size ,u32 block_tatal,u16 *len)
{
	return S_MakeUpdateDataFrame(GD_UPINFO_SEND_BROAD,update_id, block_id, block_data,  block_size , block_tatal, len);
}

P_APP_PACKET HplcApp_MakeUpdateStatusFrame(u32 update_id, u32 start_block_id, u16 block_count,u16 *len)
{
	P_APP_PACKET pFrame = NULL;
	u16 msdulen;
	msdulen = sizeof(GD_UPDATE_STATUS_DOWN);
	P_GD_UPDATE_STATUS_DOWN pStartUpdateFrame = (P_GD_UPDATE_STATUS_DOWN)alloc_buffer(__func__,msdulen);

	if (pStartUpdateFrame == NULL)
	{
		return NULL;
	}

	pStartUpdateFrame->InfoId=GD_UPINFO_QUERY_STATUS;
	memset(pStartUpdateFrame->res,0,sizeof(pStartUpdateFrame->res));
	
	pStartUpdateFrame->UpdateID = update_id;
	pStartUpdateFrame->StartBlockID = start_block_id;
	pStartUpdateFrame->BlockCount = block_count;



	pFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NEED_RESPONSE, EM_CMD_BUSINESS_SEND_FILE,  (u8 *)pStartUpdateFrame, msdulen, &msdulen);
	free_buffer(__func__,pStartUpdateFrame);
	*len = msdulen;
	return pFrame;

}

P_APP_PACKET HplcApp_MakeUpdateExeFrame(u32 update_id, u16 reset_after_seconds,u16 *len)
{
	P_APP_PACKET pFrame = NULL;
	u16 msdulen;
	msdulen = sizeof(GD_UPDATE_EXE_DOWN);
	P_GD_UPDATE_EXE_DOWN pStartUpdateFrame = (P_GD_UPDATE_EXE_DOWN)alloc_buffer(__func__,msdulen);

	if (pStartUpdateFrame == NULL)
	{
		return NULL;
	}

	pStartUpdateFrame->InfoId=GD_UPINFO_SEND_DONE;
	memset(pStartUpdateFrame->res,0,sizeof(pStartUpdateFrame->res));
	
	pStartUpdateFrame->UpdateID = update_id;
	pStartUpdateFrame->ResetAfterSeconds = reset_after_seconds;


	pFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NEED_RESPONSE, EM_CMD_BUSINESS_SEND_FILE,  (u8 *)pStartUpdateFrame, msdulen, &msdulen);
	free_buffer(__func__,pStartUpdateFrame);
	*len = msdulen;
	return pFrame;
}


#else
P_APP_PACKET HplcApp_MakeStartUpdateFrame(u32 update_id, u16 update_minutes, u16 block_size, u32 file_size, u32 file_checksum)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(UPDATE_START_DOWN));

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_UPDATE;
	pFrame->PacketID = APP_PACKET_START_UPDATE;
	pFrame->Ctl = 0;

	P_UPDATE_START_DOWN pStartUpdateFrame = (P_UPDATE_START_DOWN)pFrame->Data;

	pStartUpdateFrame->AppProtocol = 1;
	pStartUpdateFrame->HeadLen = sizeof(UPDATE_START_DOWN);
	pStartUpdateFrame->Reserve1 = 0;
	pStartUpdateFrame->UpdateID = update_id;
	pStartUpdateFrame->UpdateMinutes = update_minutes;
	pStartUpdateFrame->BlockSize = block_size;
	pStartUpdateFrame->FileSize = file_size;
	pStartUpdateFrame->FileCheckSum = file_checksum;

	return pFrame;
}

P_APP_PACKET HplcApp_MakeStopUpdateFrame(u32 update_id)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(UPDATE_STOP_DOWN));

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_UPDATE;
	pFrame->PacketID = APP_PACKET_STOP_UPDATE;
	pFrame->Ctl = 0;

	P_UPDATE_STOP_DOWN pStopUpdateFrame = (P_UPDATE_STOP_DOWN)pFrame->Data;

	pStopUpdateFrame->AppProtocol = 1;
	pStopUpdateFrame->HeadLen = sizeof(UPDATE_STOP_DOWN);
	pStopUpdateFrame->Reserve1 = 0;
	pStopUpdateFrame->UpdateID = update_id;

	return pFrame;
}

P_APP_PACKET HplcApp_MakeUpdateDataFrame(u32 update_id, u32 block_id, u8 *block_data, u32 block_size)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(UPDATE_DATA_DOWN) - 1 + block_size);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_UPDATE;
	pFrame->PacketID = APP_PACKET_FILE_TRAN;
	pFrame->Ctl = 0;

	P_UPDATE_DATA_DOWN pUpdateDataFrame = (P_UPDATE_DATA_DOWN)pFrame->Data;

	pUpdateDataFrame->AppProtocol = 1;
	pUpdateDataFrame->HeadLen = sizeof(UPDATE_DATA_DOWN) - 1;
	pUpdateDataFrame->Reserve1 = 0;
	pUpdateDataFrame->UpdateID = update_id;
	pUpdateDataFrame->BlockID = block_id;
	memcpy_special(pUpdateDataFrame->BlockData, block_data, block_size);
	pUpdateDataFrame->BlockSize = block_size;

	return pFrame;
}

P_APP_PACKET HplcApp_MakeUpdateDataBroadcastFrame(u32 update_id, u32 block_id, u8 *block_data, u32 block_size)
{
	P_APP_PACKET pFrame = HplcApp_MakeUpdateDataFrame(update_id, block_id, block_data, block_size);

	if (NULL == pFrame)
		return pFrame;

	pFrame->PacketID = APP_PACKET_FILE_LOCAL;
	return pFrame;
}

P_APP_PACKET HplcApp_MakeUpdateStatusFrame(u32 update_id, u32 start_block_id, u16 block_count)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(UPDATE_STATUS_DOWN));

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_UPDATE;
	pFrame->PacketID = APP_PACKET_CHECK_UPDATE;
	pFrame->Ctl = 0;

	P_UPDATE_STATUS_DOWN pUpdateStatusFrame = (P_UPDATE_STATUS_DOWN)pFrame->Data;

	pUpdateStatusFrame->AppProtocol = 1;
	pUpdateStatusFrame->HeadLen = sizeof(UPDATE_STATUS_DOWN);
	pUpdateStatusFrame->Reserve1 = 0;
	pUpdateStatusFrame->UpdateID = update_id;
	pUpdateStatusFrame->StartBlockID = start_block_id;
	pUpdateStatusFrame->BlockCount = block_count;

	return pFrame;
}

P_APP_PACKET HplcApp_MakeUpdateExeFrame(u32 update_id, u16 reset_after_seconds, u32 test_run_seconds)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(UPDATE_EXE_DOWN));

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_UPDATE;
	pFrame->PacketID = APP_PACKET_UPDATE;
	pFrame->Ctl = 0;

	P_UPDATE_EXE_DOWN pUpdateExtFrame = (P_UPDATE_EXE_DOWN)pFrame->Data;

	pUpdateExtFrame->AppProtocol = 1;
	pUpdateExtFrame->HeadLen = sizeof(UPDATE_EXE_DOWN);
	pUpdateExtFrame->Reserve1 = 0;
	pUpdateExtFrame->UpdateID = update_id;
	pUpdateExtFrame->ResetAfterSeconds = reset_after_seconds;
	pUpdateExtFrame->TestRunSeconds = test_run_seconds;

	return pFrame;
}
P_APP_PACKET HplcApp_GetModuleIDFrame(u16 seq)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(READ_ID)+sizeof(Moudle_ID_Info));

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_GET_ID;
	pFrame->Ctl = 0;

	READ_ID *pStaIDFrame = (READ_ID*)pFrame->Data;

	pStaIDFrame->AppProtocol = 1;
	pStaIDFrame->HeadLen = sizeof(READ_ID);
	pStaIDFrame->FrameId = seq;
	pStaIDFrame->Dir=0;
	pStaIDFrame->Id_type=2;
	return pFrame;
}
P_APP_PACKET HplcApp_GetStaEdgeFrame(u16 seq,u8 src_mac[MAC_ADDR_LEN], u8 dst_mac[MAC_ADDR_LEN])
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(GET_STA_EDGE_DOWN));

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_GET_STA_EDGE;
	pFrame->Ctl = 0;

	GET_STA_EDGE_DOWN *pStaEdgeFrame = (GET_STA_EDGE_DOWN*)pFrame->Data;

	pStaEdgeFrame->AppProtocol = 1;
	pStaEdgeFrame->HeadLen = sizeof(GET_STA_EDGE_DOWN);
	pStaEdgeFrame->Reserve1 = 0;
	pStaEdgeFrame->Seq = seq;
	memcpy_swap(pStaEdgeFrame->SrcMac,src_mac,MAC_ADDR_LEN);
	memcpy_swap(pStaEdgeFrame->DstMac,dst_mac,MAC_ADDR_LEN);
	return pFrame;
}
P_APP_PACKET HplcApp_MakeUpdateStaInfoFrame(EM_UPDATE_STA_INFO elements[], u8 element_count)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(UPDATE_STA_INFO_DOWN) - 1 + element_count);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_UPDATE;
	pFrame->PacketID = APP_PACKET_CHECK_STA;
	pFrame->Ctl = 0;

	P_UPDATE_STA_INFO_DOWN pUpdateInfoFrame = (P_UPDATE_STA_INFO_DOWN)pFrame->Data;

	pUpdateInfoFrame->AppProtocol = 1;
	pUpdateInfoFrame->HeadLen = sizeof(UPDATE_STA_INFO_DOWN) - 1;
	pUpdateInfoFrame->Reserve1 = 0;
	pUpdateInfoFrame->ElementCount = element_count;
	for (u8 i = 0; i < element_count; i++) pUpdateInfoFrame->Elements[i] = (u8)elements[i];

	return pFrame;
}
#endif
#ifndef HPLC_CSG
P_APP_PACKET HplcApp_MakeAreaDiscernmentFrame(u32 packet_seq, u8 src_mac[MAC_ADDR_LEN], u8 feature_type, u8 collection_type, u8 *pdata, u16 *len)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(AREA_DISCERNMENT_DOWN) - 1 + *len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_AREA_DISCERNMENT;
	pFrame->Ctl = 0;

	P_AREA_DISCERNMENT_DOWN pAreaDiscernment = (P_AREA_DISCERNMENT_DOWN)pFrame->Data;

	pAreaDiscernment->AppProtocol = 1;
	pAreaDiscernment->HeadLen = sizeof(AREA_DISCERNMENT_DOWN) - 1;

	pAreaDiscernment->Direction = 0;        //方向位
	pAreaDiscernment->StartFlag = 1;        //启动位
	pAreaDiscernment->Phase = 0;            //相位
	pAreaDiscernment->Seq = packet_seq;
	memcpy_swap(pAreaDiscernment->MacAddr, src_mac, MAC_ADDR_LEN);
	pAreaDiscernment->FeatureType = feature_type;            //特征类型
	pAreaDiscernment->CollectionType = collection_type;         //采集类型

	if ((NULL != pdata) && (*len > 0))
		memcpy_special(pAreaDiscernment->Data, pdata, *len);
	*len = sizeof(APP_PACKET) - 1 + sizeof(AREA_DISCERNMENT_DOWN) - 1 + *len;
	return pFrame;
}
#else
P_APP_PACKET HplcApp_MakeAreaDiscernmentFrame(u32 packet_seq, u8 src_mac[MAC_ADDR_LEN], u8 feature_type, u8 collection_type, u8 *pdata, u16 *len)
{
	P_APP_PACKET pFrame = NULL;
//	u8 *pMsdu = NULL;
	u16 msdulen;
	msdulen = sizeof(AREA_DISCERNMENT_CSG_DOWN) + *len;
	P_AREA_DISCERNMENT_CSG_DOWN pAreaDiscernment = (P_AREA_DISCERNMENT_CSG_DOWN)alloc_buffer(__func__,msdulen);
	if (pAreaDiscernment == NULL)
	{
		return NULL;
	}
	memset(pAreaDiscernment->res, 0, sizeof(pAreaDiscernment->res));
	pAreaDiscernment->HeadLen = sizeof(AREA_DISCERNMENT_CSG_DOWN);

	pAreaDiscernment->Phase = 0;            //相位
	memcpy_swap(pAreaDiscernment->MacAddr, src_mac, MAC_ADDR_LEN);
	pAreaDiscernment->FeatureType = feature_type;            //特征类型
	pAreaDiscernment->CollectionType = collection_type;         //采集类型

	if ((NULL != pdata) && (*len > 0))
		memcpy_special(pAreaDiscernment->Data, pdata, *len);

	pFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NEED_RESPONSE, EM_CMD_BUSINESS_AREA_DISCERNMENT,  (u8 *)pAreaDiscernment, msdulen, &msdulen);
	free_buffer(__func__,pAreaDiscernment);
	*len = msdulen;
	return pFrame;
}
#endif
P_APP_PACKET HplcApp_MakeCommTestFrame(u8 *pdata, u16 len, APP_METER_PRO protocol)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(P_COMM_TEST_FRAME_PACKET) - 1 + len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_COMM_TEST;
	pFrame->Ctl = 0;

	P_COMM_TEST_FRAME_PACKET pCommTestFrame = (P_COMM_TEST_FRAME_PACKET)pFrame->Data;

	pCommTestFrame->AppProtocol = 1;
	pCommTestFrame->HeadLen = sizeof(COMM_TEST_FRAME_PACKET);
	pCommTestFrame->Reserve1 = 0;
	pCommTestFrame->Protocol = (u8)protocol;
	pCommTestFrame->DataLen = len;

	if ((len > 0) && (NULL != pdata))
		memcpy_special(pCommTestFrame->data, pdata, len);

	return pFrame;
}

P_APP_PACKET HplcApp_MakeEnterTestModeFrame(COMM_TEST_MODE test_mode, u32 test_value)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(COMM_TEST_FRAME_PACKET));

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_COMM_TEST;
	pFrame->Ctl = 0;

	P_COMM_TEST_FRAME_PACKET pCommTestFrame = (P_COMM_TEST_FRAME_PACKET)pFrame->Data;

	pCommTestFrame->AppProtocol = 1;
	pCommTestFrame->HeadLen = sizeof(COMM_TEST_FRAME_PACKET);
	pCommTestFrame->Reserve1 = test_mode;
	pCommTestFrame->Protocol = 0;
	pCommTestFrame->DataLen = test_value;

	return pFrame;
}

P_APP_PACKET HplcApp_MakeSyncRTCFrame(u8 *pdata, u16 len,u32 ntb)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(SYNC_RTC_DOWN) + len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_SYNC_RTC;
	pFrame->Ctl = 0;

	P_SYNC_RTC_DOWN pTimingFrame = (P_SYNC_RTC_DOWN)pFrame->Data;

	pTimingFrame->AppProtocol = 1;
	pTimingFrame->HeadLen = sizeof(SYNC_RTC_DOWN);
	pTimingFrame->Reserve1 = 0;
	pTimingFrame->ntb = ntb;

	memcpy_special(pTimingFrame->RTC, pdata, len);

	return pFrame;
}

P_APP_PACKET HplcApp_MakeStoreDataBcastTimingFrame(u8 *pdata, u16 len)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(STORE_DATA_BCAST_TIMING_DOWN) + len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_STORE_DATA_TIMING;
	pFrame->Ctl = 0;

	P_STORE_DATA_BCAST_TIMING_DOWN pTimingFrame = (P_STORE_DATA_BCAST_TIMING_DOWN)pFrame->Data;

	pTimingFrame->AppProtocol = 1;
	pTimingFrame->HeadLen = sizeof(STORE_DATA_BCAST_TIMING_DOWN);
	pTimingFrame->Reserve1 = 0;
	pTimingFrame->Reserve2 = 0;
	pTimingFrame->DataLen = len;

	memcpy_special(pTimingFrame->Data, pdata, len);

	return pFrame;
}

P_APP_PACKET HplcApp_MakeStoreDataSyncCfgFrame(u32 packet_seq, u8 src_mac[MAC_ADDR_LEN], u8 dst_mac[MAC_ADDR_LEN], u8 *pdata, u16 len)
{
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,sizeof(APP_PACKET) - 1 + sizeof(STORE_DATA_BCAST_SYNC_CFG_DOWN) + len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_STORE_DATA_SYNC_CFG;
	pFrame->Ctl = 0;

	P_STORE_DATA_BCAST_SYNC_CFG_DOWN pSyncCfgFrame = (P_STORE_DATA_BCAST_SYNC_CFG_DOWN)pFrame->Data;

	pSyncCfgFrame->AppProtocol = 1;
	pSyncCfgFrame->HeadLen = sizeof(STORE_DATA_BCAST_SYNC_CFG_DOWN);
	pSyncCfgFrame->Reserve1 = 0;
	pSyncCfgFrame->Seq = packet_seq;
	memcpy_swap(pSyncCfgFrame->SrcMac, src_mac, MAC_ADDR_LEN);
	memcpy_swap(pSyncCfgFrame->DstMac, dst_mac, MAC_ADDR_LEN);
	pSyncCfgFrame->Reserve2 = 0;
	pSyncCfgFrame->DataLen = len;
	memcpy_special(pSyncCfgFrame->Data, pdata, len);

	return pFrame;
}

P_APP_PACKET HplcApp_MakeGetInnerVerFrame(u8 node_mac[6], u16* len)
{
	u16 frame_len = 0;
#ifndef HPLC_CSG
	frame_len = sizeof(APP_PACKET) - 1 + sizeof(READ_INNER_VER);
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,frame_len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_FACTORY;
	pFrame->PacketID = APP_PACKET_EXT_FACTORY_TEST;
	pFrame->Ctl = 0;

	P_READ_INNER_VER pReadInnerVerFrame = (P_READ_INNER_VER)pFrame->Data;

	pReadInnerVerFrame->AppProtocol = 1;
	pReadInnerVerFrame->HeadLen = sizeof(READ_INNER_VER);
	pReadInnerVerFrame->Reserved = 0;
	pReadInnerVerFrame->Function = 0x3B;
	pReadInnerVerFrame->DataLen = 6;
	pReadInnerVerFrame->Seq = 0;
	memcpy_special(pReadInnerVerFrame->Mac, node_mac, 6);
#else
	frame_len = sizeof(APP_PACKET) - 1 + sizeof(APP_BUSINESS_PACKET) - 1 + 6;
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,frame_len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_FACTORY;
	pFrame->PacketID = APP_PACKET_CSG;
	pFrame->Ctl = 0;

	P_APP_BUSINESS_PACKET pBusinessPacket = (P_APP_BUSINESS_PACKET)pFrame->Data;

	pBusinessPacket->Ctrl.FrameType = 0xF;
	pBusinessPacket->Ctrl.ExtFieldFlag = 0;
	pBusinessPacket->Ctrl.ResponseFlag = EM_BUSINESS_NEED_RESPONSE;
	pBusinessPacket->Ctrl.StartFlag = 1;
	pBusinessPacket->Ctrl.Direction = 0;
	pBusinessPacket->Ctrl.Reserve = 0;

	pBusinessPacket->BusinessID = EM_CMD_BUSINESS_INNER_VER;
	pBusinessPacket->Version = 1;
	pBusinessPacket->FrameSeq = 0;
	pBusinessPacket->FrameLen = 6;
	memcpy_special(pBusinessPacket->Data, node_mac, 6);
#endif

	*len = frame_len;
	return pFrame;
}


P_APP_PACKET HplcApp_MakeStaPowerFrame(u8 node_mac[6], u16 func,void *data,u16* len)
{
	u16 frame_len = 0;
#ifndef HPLC_CSG
	frame_len = sizeof(APP_PACKET) - 1 + sizeof(STA_POWER)+ *len;
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,frame_len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_FACTORY;
	pFrame->PacketID = APP_PACKET_EXT_FACTORY_TEST;
	pFrame->Ctl = 0;

	P_STA_POWER pStaPower = (P_STA_POWER)pFrame->Data;

	pStaPower->AppProtocol = 1;
	pStaPower->HeadLen = sizeof(STA_POWER)+*len;
	pStaPower->Reserved = 0;
	pStaPower->Function = func;
	pStaPower->DataLen = 6 + *len;
	pStaPower->Seq = 0;
	memcpy_special(pStaPower->Mac, node_mac, 6);
	memcpy_special(pStaPower->Data,data,*len);
#else
	frame_len = sizeof(APP_PACKET) - 1 + sizeof(APP_BUSINESS_PACKET) - 1 + 6 +*len;
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,frame_len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_FACTORY;
	pFrame->PacketID = APP_PACKET_CSG;
	pFrame->Ctl = 0;

	P_APP_BUSINESS_PACKET pBusinessPacket = (P_APP_BUSINESS_PACKET)pFrame->Data;

	pBusinessPacket->Ctrl.FrameType = 0xF;
	pBusinessPacket->Ctrl.ExtFieldFlag = 0;
	pBusinessPacket->Ctrl.ResponseFlag = EM_BUSINESS_NEED_RESPONSE;
	pBusinessPacket->Ctrl.StartFlag = 1;
	pBusinessPacket->Ctrl.Direction = 0;
	pBusinessPacket->Ctrl.Reserve = 0;

	pBusinessPacket->BusinessID = func;
	pBusinessPacket->Version = 1;
	pBusinessPacket->FrameSeq = 0;
	pBusinessPacket->FrameLen = 6+*len;
	memcpy_special(pBusinessPacket->Data, node_mac, 6);
	memcpy_special(&pBusinessPacket->Data[6], data, *len);
#endif

	*len = frame_len;
	return pFrame;
}

P_APP_PACKET HplcApp_MakeGetRegisterFrame(u8 node_mac[6], void *data, u16* len)
{
	u16 frame_len = 0;
#ifndef HPLC_CSG
	frame_len = sizeof(APP_PACKET) - 1 + sizeof(READ_REGISTER) + *len;
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,frame_len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_FACTORY;
	pFrame->PacketID = APP_PACKET_EXT_FACTORY_TEST;
	pFrame->Ctl = 0;

	P_READ_REGISTER pReadRegister = (P_READ_REGISTER)pFrame->Data;

	pReadRegister->AppProtocol = 1;
	pReadRegister->HeadLen = sizeof(READ_REGISTER);
	pReadRegister->Reserved = 0;
	pReadRegister->Function = 0x70;
	pReadRegister->DataLen = 12;
	pReadRegister->Seq = 0;
	memcpy_special(pReadRegister->Mac, node_mac, 6);
	memcpy_special(pReadRegister->Data,data,*len);
#else
	frame_len = sizeof(APP_PACKET) - 1 + sizeof(APP_BUSINESS_PACKET) - 1 + 6 +*len; //12:mac+reg+len
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,frame_len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_FACTORY;
	pFrame->PacketID = APP_PACKET_CSG;
	pFrame->Ctl = 0;

	P_APP_BUSINESS_PACKET pBusinessPacket = (P_APP_BUSINESS_PACKET)pFrame->Data;

	pBusinessPacket->Ctrl.FrameType = 0xF;
	pBusinessPacket->Ctrl.ExtFieldFlag = 0;
	pBusinessPacket->Ctrl.ResponseFlag = EM_BUSINESS_NEED_RESPONSE;
	pBusinessPacket->Ctrl.StartFlag = 1;
	pBusinessPacket->Ctrl.Direction = 0;
	pBusinessPacket->Ctrl.Reserve = 0;

	pBusinessPacket->BusinessID = EM_CMD_BUSINESS_REGISTER;
	pBusinessPacket->Version = 1;
	pBusinessPacket->FrameSeq = 0;
	pBusinessPacket->FrameLen = 12;
	memcpy_special(pBusinessPacket->Data, node_mac, 6);
	memcpy_special(&pBusinessPacket->Data[6], data, *len);
#endif

	*len = frame_len;
	return pFrame;
}


P_APP_PACKET HplcApp_MakeStaAuthenFrame(int en,u16* len)
{
#ifndef HPLC_CSG
	u16 frame_len = 0;

	frame_len = sizeof(APP_PACKET) - 1 + sizeof(SET_STA_AUTHEN);
	P_APP_PACKET pFrame = (P_APP_PACKET)alloc_buffer(__func__,frame_len);

	if (NULL == pFrame)
		return pFrame;

	pFrame->Port = HPLC_APP_PORT_COMMON;
	pFrame->PacketID = APP_PACKET_SET_STA_AUTHEN;
	pFrame->Ctl = 0;

	SET_STA_AUTHEN* pStaAuthen = (SET_STA_AUTHEN*)pFrame->Data;

	pStaAuthen->AppProtocol = 1;
	pStaAuthen->HeadLen = sizeof(SET_STA_AUTHEN)-1;
	pStaAuthen->Reserve1 = 0;
	pStaAuthen->Seq = rand();
	pStaAuthen->IsEnable = en;
	pStaAuthen->Seq = 0;
	*len = frame_len;
	return pFrame;
#else
	return NULL;
#endif	
}

#ifdef HPLC_CSG
P_APP_PACKET HplcApp_MakeReadStaStatusFrame(u32 element, u16 *len)
{
	u8 frame[10];
	frame[0] = 0;
	for (int i = 0; i < 5; i++)
	{
		if (element & (1 << i))
		{
			frame[0]++;
			frame[frame[0]] = i;
		}
	}

	P_APP_PACKET pFrame = NULL;
	*len = frame[0] + 1;
	pFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NEED_RESPONSE, EM_CMD_BUSINESS_STA_RUN_STATUS,  frame, *len, len);
	return pFrame;
}
P_APP_PACKET HplcApp_MakeReadStaInfoFrame(u32 element, u16 *len)
{
	u8 frame[18]; //目前元素ID最大0x10（深化应用V1.1），最多17个元素；frame[0]表示个数，数组大小18
	frame[0] = 0;
	for (int i=0; i<=16; i++)
	{
		if (element & (1 << i))
		{
			frame[0]++;
			frame[frame[0]] = i;
		}
	}

	P_APP_PACKET pFrame = NULL;
	*len = frame[0] + 1;
	pFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NEED_RESPONSE, EM_CMD_BUSINESS_READ_STA_INFO,  frame, *len, len);
	return pFrame;
}

P_APP_PACKET HplcApp_MakeReadStaInfoFrameFromArray(u8 element_num, u8* element_array, u16 *len)
{
	u8 frame[18] = {0}; //目前元素ID最大0x10（深化应用V1.1），最多17个元素；frame[0]表示个数，数组大小18

	frame[0] = element_num;
	memcpy_special(&frame[1], element_array, element_num);


	P_APP_PACKET pFrame = NULL;
	*len = element_num + 1;
	pFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NEED_RESPONSE, EM_CMD_BUSINESS_READ_STA_INFO, frame, *len, len);
	return pFrame;
}

P_APP_PACKET HplcApp_MakeReadChannelInfoFrame(u16 start_sn, u8 num, u16 *len)
{
	struct
	{
		u16 start;
		u8  num;
	}frame;

	frame.num = num;
	frame.start = start_sn;
	P_APP_PACKET pFrame = NULL;
	*len = 3;
	pFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NEED_RESPONSE, EM_CMD_BUSINESS_STA_CHANNEL_INFO,  (u8 *)&frame, *len, len);
	return pFrame;
}

P_APP_PACKET HplcApp_MakeResetStaFrame(u16 seconds, u16* len)
{
    P_APP_PACKET pFrame = NULL;
    u16 msdulen;
    msdulen = sizeof(APP_RESET_STA_DOWN);
    P_APP_RESET_STA_DOWN pResetSta = (P_APP_RESET_STA_DOWN)alloc_buffer(__func__, msdulen);
    
    if (pResetSta == NULL)
    {
        return NULL;
    }

    pResetSta->ResetAfterSeconds = seconds;

    pFrame = HplcApp_MakeCmdFrame(++g_pRelayStatus->packet_seq_read_meter, EM_BUSINESS_NORESPONSE, EM_CMD_BUSINESS_REBOOT_STA, (u8 *)pResetSta, msdulen, &msdulen);
    free_buffer(__func__, pResetSta);

    *len = msdulen;
    return pFrame;
}

#endif

//#include "../hal/macrodriver.h"

#include "system_inc.h"
//#include "../drv/uart_link.h"
//#include "../drv/timers.h"
#include "alert.h"
#include "rtc.h"
//#include "../dev/dataflash/data_flash.h"
//#include "../dev/fram/fram.h"
#include "module_def.h"
//#include "data_types.h"
//#include "Flow_Cascade.h"
//#include "PLM_Warning.h"
//#include "PLM_Event.h"
//#include "gd_cctt_net.h"
//#include "Event_Detect.h"
//#include "Data_Report.h"
//#include "Data_Freeze.h"
#include "plm_2005.h"
#include "PLM_AFN.h"
#include "PLM_AFN_GD.h"
//#include "storage.h"
//#include "task.h"

#include "plc_autorelay.h"
#include "Uart_Link.h"
//#include <string.h>

unsigned char PLM_hn_GD_read1(pvoid h);
unsigned short afn_01_F3_proc(pvoid h);

unsigned char PLM_hn_GD_read2(pvoid h);

//湖南AFN处理
const AFN_FUNC_INFO _Q_GDW_AFN_PROC[] =
{

    0x01, (Q_GDW_PROC_PTR)afn_func_01,
    0x02, (Q_GDW_PROC_PTR)afn_func_02,
    0x03, (Q_GDW_PROC_PTR)afn_request_info_03,
    0x04, (Q_GDW_PROC_PTR)afn_mac_check_04,
    0x05, (Q_GDW_PROC_PTR)afn_ctl_cmd_05,            //0x05

#ifdef GDW_2019_GRAPH_STOREDATA
    0x09, (Q_GDW_PROC_PTR)afn_relay_query_09,      //陕西私有
#endif	
    0x10, (Q_GDW_PROC_PTR)afn_relay_query_10,
    0x11, (Q_GDW_PROC_PTR)afn_relay_setting_11,
    0x12, (Q_GDW_PROC_PTR)afn_relay_action_12,         //0x0b
    0x13, (Q_GDW_PROC_PTR)afn_relay_data_forward_13,
    //0x14, afn_relay_data_read_14,
    0x15, (Q_GDW_PROC_PTR)afn_15_file_trans,

    0xF0, (Q_GDW_PROC_PTR)afn_F0_internal_debug,
    0xF1, (Q_GDW_PROC_PTR)afn_func_F1,
    0xF3, (Q_GDW_PROC_PTR)afn_factory_read,
    0xF5, (Q_GDW_PROC_PTR)afn_factory_write,
    0xF6, (Q_GDW_PROC_PTR)afn_factory_reserved,

    0x0, (Q_GDW_PROC_PTR)NULL,
};

#if defined(PROTOCOL_GD_2016)
const AFN_FUNC_INFO _Q_GD_AFN_PROC[] =
{
    0x01, (Q_GDW_PROC_PTR)afn_func_01_gd,
    0x02, (Q_GDW_PROC_PTR)afn_func_02_gd,
    0x03, (Q_GDW_PROC_PTR)afn_func_03_gd,
    0x04, (Q_GDW_PROC_PTR)afn_func_04_gd,
    0x06, (Q_GDW_PROC_PTR)afn_func_06_gd,
    0x07, (Q_GDW_PROC_PTR)afn_func_07_gd,
    0x08, (Q_GDW_PROC_PTR)afn_func_08_gd,
	0xf0, (Q_GDW_PROC_PTR)afn_func_F0_gd,
    0x0, (Q_GDW_PROC_PTR)NULL,
};
#endif

/*
const unsigned short fcstab[256] = {
0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};*/
//#define PPPINITFCS16 0xffff                 /*初始  FCS  值*/

//unsigned char g_file_trans_map[UPDATE_FILE_MAP_SIZE];
//extern unsigned char g_file_trans_map[];


unsigned char g_download_data_chk[2];
extern UCHAR CCTT_recv_msg_buf[];
//extern OS_EVENT  * Emb_PLC_Router;

P_LMP_APP_INFO PLM_alloc_msg_buf(unsigned short rx_len)
{
    P_LMP_APP_INFO pLmpAppInfo = (P_LMP_APP_INFO)alloc_buffer(__func__,sizeof(LMP_APP_INFO));

    if(NULL == pLmpAppInfo)
        return NULL;

    if(rx_len > 0)
    {
        pLmpAppInfo->rxMsgBuf = alloc_buffer(__func__,rx_len);

        if(NULL == pLmpAppInfo->rxMsgBuf)
        {
            free_buffer(__func__,pLmpAppInfo);
            return NULL;
        }
    }
    else
    {
        pLmpAppInfo->rxMsgBuf = NULL;
    }

    return pLmpAppInfo;
}

void PLM_free_msg_buf(P_LMP_APP_INFO  p_plm_msg)
{
    if(NULL != p_plm_msg)
    {
        free_buffer(__func__,p_plm_msg->rxMsgBuf);
        free_buffer(__func__,p_plm_msg);
    }
}



//alan test
//unsigned short g_updata_firmware_cnt[4];





/*****************************************************************************

函数说明: 找到UCHAR inData (信息点标识DA 和信息类标识DT, etc) 中某有效bit

*****************************************************************************/
unsigned char GetOnePosition(unsigned char inData, unsigned char startPosition)
{
    unsigned char sn = startPosition;
    while( sn < 8 )
    {
        if( ( inData >> sn ) & 0x01 )
        {
            break;
        }
        sn++;
    }

    //if sn == 8, it means there is no 'one' bit in data;
    return sn;

}

unsigned char LMP_SendNakResponse(P_LMP_APP_INFO pLmpAppInfo, UCHAR nak_type)
{
    P_MSG_INFO p_CccttSendMsg = NULL;

    //send response
    P_LMP_LINK_HEAD sendLinkHead;

    // 68 10 00 81 00 00 40 00 00 00 00 02 00 05 C8 16
    const UCHAR LMP_Nak_Frame[] = {0x68, 0x10, 0x00, 0x83, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00};

    if( !(p_CccttSendMsg = alloc_send_buffer(__func__,MSG_SHORT)) )
    {
        //Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);

        //pDevicedesc->NumberOfLostCcttRespFrame ++;
        return ERROR;
    }

    //ExtIoSetOn(RUN_MSA_COMM_LED);

    sendLinkHead = (P_LMP_LINK_HEAD)p_CccttSendMsg->msg_buffer;
    //TBD
    memcpy_special(p_CccttSendMsg->msg_buffer, LMP_Nak_Frame, sizeof(LMP_Nak_Frame) );

    sendLinkHead->Inf.up_info.seq_no = pLmpAppInfo->lmpLinkHead->Inf.down_info.seq_no;

    p_CccttSendMsg->msg_buffer[sizeof(LMP_Nak_Frame)] = nak_type;
    p_CccttSendMsg->msg_buffer[sizeof(LMP_Nak_Frame)+1] = Get_checksum(&sendLinkHead->ctl_field, LMP_Nak_Frame[1]-5);

    p_CccttSendMsg->msg_buffer[sizeof(LMP_Nak_Frame)+2] = 0x16;

    p_CccttSendMsg->msg_header.msg_len = sizeof(LMP_Nak_Frame)+3;

    p_CccttSendMsg->msg_header.need_buffer_free = TRUE; /* 标识end 负责buffer 释放*/
	p_CccttSendMsg->msg_header.send_delay = false;

    p_CccttSendMsg->msg_header.end_id = pLmpAppInfo->end_id;

#ifdef APP_WIRELESS_EN
    if(COM_PORT_WIRELESS == pLmpAppInfo->end_id)
    {
        P_MSG_INFO pMsg = NULL;

        pMsg = LMP_make_frame_wireless(0x84, &p_CccttSendMsg->msg_buffer[0], p_CccttSendMsg->msg_header.msg_len);

        free_send_buffer(__func__,p_CccttSendMsg);

        if(!pMsg)
            return ERROR;

        End_post(pMsg); //End_send(pMsg);
    }
    else
#endif
    {
        PLM_SEND_DEBUG_FRAME_WL(p_CccttSendMsg);

#ifdef DEBUG_FRMAE_3762
        debug_hex(false, (u8*)p_CccttSendMsg->msg_buffer, p_CccttSendMsg->msg_header.msg_len);
#endif

        End_post(p_CccttSendMsg); //End_send(p_CccttSendMsg);
    }

    return OK;
}

unsigned char LMP_SendConfirmResponse(P_LMP_APP_INFO pLmpAppInfo, USHORT res)
{
    P_MSG_INFO p_CccttSendMsg = NULL;
    UCHAR * user_data; //, len

    //send response
    P_LMP_LINK_HEAD sendLinkHead = (P_LMP_LINK_HEAD)pLmpAppInfo->msg_buffer; //g_LmpProtocolInfo.responseBuffer;

    //ExtIoSetOn(RUN_MSA_COMM_LED);

    sendLinkHead->firstStartByte = FRAME_PLM2005_START_FLAG;

    //sendLinkHead->ctl_field &= 0x0f;
    //sendLinkHead->ctl_field |= 0x80;     //PRM = 0; DIR = 1;
    
	#if defined(HRF_SUPPORT) && !defined(COMM_MODE_OLD)
	sendLinkHead->ctl_field = 0x84;
	#else
	sendLinkHead->ctl_field = 0x83;
	#endif

    memset_special((UCHAR *)&sendLinkHead->Inf, 0, sizeof(INFO_FD));

    if(EM_FACTORY_DR == g_PlmStatusPara.factoryFlag)
    {
        sendLinkHead->Inf.up_info.channel_char = 0x4;    //东软路由板反回的是这个
    }
    //sendLinkHead->Inf.up_info.channel_char = 0;

    // TODO: support comm_module == 1
    //pLmpAppInfo->lmpLinkHead->Inf.down_info.comm_module = 0;

    if(pLmpAppInfo->lmpLinkHead->Inf.down_info.comm_module)
    {
#if 1
        //暂时不处理 comm_module == 1 的确认
        memmove_special((void*)&sendLinkHead->user_data[0], (void*)&sendLinkHead->user_data[12], 18);
        user_data = &sendLinkHead->user_data[0];
        pLmpAppInfo->responsePoint -= 12;
#else
        len = (pLmpAppInfo->lmpLinkHead->Inf.down_info.relay_level * 6);
        sendLinkHead->Inf.up_info.comm_module = 1;
        memcpy_special( &sendLinkHead->user_data[6], pLmpAppInfo->lmpLinkHead->user_data, 6);
        memcpy_special(&sendLinkHead->user_data[0], &pLmpAppInfo->lmpLinkHead->user_data[len+6],  6);
        user_data = &sendLinkHead->user_data[12];
#endif
    }
    else
    {
        user_data = &sendLinkHead->user_data[0];
    }

    sendLinkHead->Inf.up_info.relay_mode = pLmpAppInfo->lmpLinkHead->Inf.down_info.relay_mode;
#ifdef GDW_2012
    sendLinkHead->Inf.up_info.seq_no = pLmpAppInfo->lmpLinkHead->Inf.down_info.seq_no;
#else
    sendLinkHead->Inf.up_info.seq_no = 0;
#endif
    if(pLmpAppInfo->confirmFlag == 1)
    {
        user_data[0] = 0; //afn 0
        if(res == OK)
        {
            // ACK
            user_data[1] = 0x01; //F1
        }
        else
        {
            // NAK
            user_data[1] = 0x02; //F2

        }
        user_data[2] = 0;
    }

    // add 4 bytes channel idle
    if(pLmpAppInfo->responsePoint == 0x0d)
    {
#if 0
        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0xff;
        pLmpAppInfo->responsePoint ++;
        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0xff;
        pLmpAppInfo->responsePoint ++;
        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x02;
        pLmpAppInfo->responsePoint ++;
        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x00;
        pLmpAppInfo->responsePoint ++;
#else
        P_CONFIRM_DATA_UNIT pDataUnit = (P_CONFIRM_DATA_UNIT)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
        pDataUnit->router_status = ~0;
        pDataUnit->wait_time = 2;
        pLmpAppInfo->responsePoint += sizeof(CONFIRM_DATA_UNIT);
#endif
    }

    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = Get_checksum(&sendLinkHead->ctl_field, pLmpAppInfo->responsePoint-3);
    pLmpAppInfo->responsePoint++;

    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x16;
    pLmpAppInfo->responsePoint++;

    sendLinkHead->frm_len = pLmpAppInfo->responsePoint;

    //TBD
#ifdef APP_WIRELESS_EN
    if(COM_PORT_WIRELESS == pLmpAppInfo->end_id)
    {
        p_CccttSendMsg = LMP_make_frame_wireless(0x84, &pLmpAppInfo->msg_buffer[0], pLmpAppInfo->responsePoint);

        if(!p_CccttSendMsg)
            return ERROR;
    }
    else
#endif
    {
        if( !(p_CccttSendMsg = alloc_send_buffer(__func__,MSG_SHORT)) )
        {
            //Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);

            //pDevicedesc->NumberOfLostCcttRespFrame ++;
            return ERROR;
        }
        memcpy_special(p_CccttSendMsg->msg_buffer, pLmpAppInfo->msg_buffer, pLmpAppInfo->responsePoint );

        p_CccttSendMsg->msg_header.msg_len = pLmpAppInfo->responsePoint;

        p_CccttSendMsg->msg_header.need_buffer_free = TRUE; /* 标识end 负责buffer 释放*/
		p_CccttSendMsg->msg_header.send_delay = false;

        p_CccttSendMsg->msg_header.end_id = pLmpAppInfo->end_id;

        PLM_SEND_DEBUG_FRAME_WL(p_CccttSendMsg);
    }

#ifdef DEBUG_FRMAE_3762
    debug_hex(false, (u8*)p_CccttSendMsg->msg_buffer, p_CccttSendMsg->msg_header.msg_len);
#endif

    End_post(p_CccttSendMsg); //End_send(p_CccttSendMsg);

    return OK;
}

unsigned char LMP_SendQueryResponse(P_LMP_APP_INFO pLmpAppInfo)
{
    P_MSG_INFO p_CccttSendMsg = NULL;
    UCHAR len;

    //send response
    P_LMP_LINK_HEAD sendLinkHead = (P_LMP_LINK_HEAD)pLmpAppInfo->msg_buffer; //g_LmpProtocolInfo.responseBuffer;

    //ExtIoSetOn(RUN_MSA_COMM_LED);

    sendLinkHead->firstStartByte = FRAME_PLM2005_START_FLAG;

	
	#if defined(HRF_SUPPORT) && !defined(COMM_MODE_OLD)
	sendLinkHead->ctl_field = 0x84;     //PRM = 0; DIR = 1;
	#else
	sendLinkHead->ctl_field = 0x83;     //PRM = 0; DIR = 1;
	#endif
    

    memset_special((UCHAR *)&sendLinkHead->Inf, 0, sizeof(INFO_FD));

    // TODO: support comm_module == 1
    if(pLmpAppInfo->lmpLinkHead->Inf.down_info.comm_module)
    {
        //上行没中继地址
        len = (pLmpAppInfo->lmpLinkHead->Inf.down_info.relay_level * 6);
        sendLinkHead->Inf.up_info.comm_module = 1;
        memcpy_special(&sendLinkHead->user_data[6], pLmpAppInfo->lmpLinkHead->user_data, 6);
        memcpy_special(&sendLinkHead->user_data[0], &pLmpAppInfo->lmpLinkHead->user_data[len+6],  6);
    }
    else
    {
        pLmpAppInfo->lmpLinkHead->Inf.down_info.comm_module = 0;
    }

    sendLinkHead->Inf.up_info.relay_mode = pLmpAppInfo->lmpLinkHead->Inf.down_info.relay_mode;
#ifdef GDW_2012
    sendLinkHead->Inf.up_info.seq_no = pLmpAppInfo->lmpLinkHead->Inf.down_info.seq_no;
#else
    sendLinkHead->Inf.up_info.seq_no = 0;
#endif
    //sendLinkHead->Inf.up_info.channel_char = 0x4;
    if(EM_FACTORY_DR == g_PlmStatusPara.factoryFlag)
    {
        sendLinkHead->Inf.up_info.channel_char = 0x4;    //东软路由板反回的是这个
    }
    else
    {
        sendLinkHead->Inf.up_info.channel_char = 0x0;
    }

    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = Get_checksum(&sendLinkHead->ctl_field, pLmpAppInfo->responsePoint-3);
    pLmpAppInfo->responsePoint++;

    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x16;
    pLmpAppInfo->responsePoint++;

    sendLinkHead->frm_len = pLmpAppInfo->responsePoint;

    //TBD
#ifdef APP_WIRELESS_EN
    if(COM_PORT_WIRELESS == pLmpAppInfo->end_id)
    {
        p_CccttSendMsg = LMP_make_frame_wireless(0x84, &pLmpAppInfo->msg_buffer[0], pLmpAppInfo->responsePoint);

        if(!p_CccttSendMsg)
            return ERROR;
    }
    else
#endif
    {
        if( !(p_CccttSendMsg = alloc_send_buffer(__func__,MSG_LONG)) )
        {
            //Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);

            return ERROR;
        }

        memcpy_special(p_CccttSendMsg->msg_buffer, pLmpAppInfo->msg_buffer, pLmpAppInfo->responsePoint );

        p_CccttSendMsg->msg_header.msg_len = pLmpAppInfo->responsePoint;

        p_CccttSendMsg->msg_header.need_buffer_free = TRUE; /* 标识end 负责buffer 释放*/
		p_CccttSendMsg->msg_header.send_delay = false;

        p_CccttSendMsg->msg_header.end_id = pLmpAppInfo->end_id;

        PLM_SEND_DEBUG_FRAME_WL(p_CccttSendMsg);
    }
#if 0
    {
        //BYTE tmp[] = {0x68, 0x38, 0x00, 0xC1, 0x01, 0x00, 0x40, 0x00, 0x00, 0xD2, 0x03, 0x02, 0x01, 0x41, 0x36, 0x60, 0x01, 0x00, 0x00, 0x23, 0x5A, 0x00, 0xFE, 0x00, 0x82, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x07, 0x00, 0x00, 0x18, 0x09, 0x13, 0x21, 0x03, 0x13, 0x30, 0x31, 0x31, 0x31, 0x11, 0x10, 0x13, 0x00, 0x01, 0x00, 0x00, 0x17, 0x16};
        //BYTE tmp[] = {0x68, 0x38, 0x00, 0x81, 0x01, 0x00, 0x40, 0x00, 0x00, 0xD2, 0x03, 0x02, 0x01, 0x41, 0x36, 0x60, 0x01, 0x00, 0x00, 0x23, 0x5A, 0x00, 0xFE, 0x00, 0x82, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x07, 0x00, 0x00, 0x18, 0x09, 0x13, 0x21, 0x03, 0x13, 0x30, 0x31, 0x31, 0x31, 0x11, 0x10, 0x13, 0x00, 0x01, 0x00, 0x00, 0xD7, 0x16};
        BYTE tmp[] = {0x68, 0x38, 0x00, 0x81, 0x01, 0x00, 0x40, 0x00, 0x00, 0xD2, 0x03, 0x02, 0x01, 0x41, 0x36, 0x60, 0x01, 0x00, 0x00, 0x23, 0x5A, 0x00, 0xFE, 0x00, 0x82, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x07, 0x00, 0x00, 0x18, 0x09, 0x13, 0x21, 0x03, 0x13, 0x30, 0x32, 0x31, 0x31, 0x11, 0x10, 0x13, 0x00, 0x01, 0x00, 0x00, 0xD8, 0x16};
        memcpy_special(p_CccttSendMsg->msg_buffer, tmp, sizeof(tmp));
        p_CccttSendMsg->msg_header.msg_len = sizeof(tmp);
    }
#endif

#ifdef DEBUG_FRMAE_3762
    debug_hex(false, (u8*)p_CccttSendMsg->msg_buffer, p_CccttSendMsg->msg_header.msg_len);
#endif

    End_post(p_CccttSendMsg); //End_send(p_CccttSendMsg);

    return OK;
}

unsigned char LMP_Response_Proc(P_LMP_APP_INFO pLmpAppInfo)
{
#ifdef GDW_2012
    P_MSG_INFO p_CccttSendMsg;
#endif
    //unsigned char *sendBuffer = pLmpAppInfo->msg_buffer;

    unsigned char afn_code, *user_data, sn;

    INFO_FIELD_DOWN *d_inf = &pLmpAppInfo->lmpLinkHead->Inf.down_info;

    PBYTE pDesAddr = NULL;
//    u8 err_code;
    OS_ERR err;

    user_data = pLmpAppInfo->lmpLinkHead->user_data;
    pLmpAppInfo->appLayerLen -= 6;      //减信息域

    if (d_inf->comm_module)
    {
        u8 len = 6;        //源地址
        len += (6 * d_inf->relay_level);
        pDesAddr = &user_data[len];     //目的地址
        len += 6;

        user_data += len;
        pLmpAppInfo->appLayerLen -= len;
    }

//    debug_hex(false, (u8*)user_data, 3);

    afn_code = user_data[0];

    sn = GetOnePosition(user_data[1], 0);

    if (sn < 8)
    {
        /* 计算得到当前' Fn '  号 */
        pLmpAppInfo->curDT = user_data[2] * 8 + sn + 1;
    }
    else
    {
        return ERROR;
    }

//    debug_str("rsp %x, %x\r\n", afn_code, pLmpAppInfo->curDT);

    if (afn_code == AFN_REAME_READ)
    {
        if (F1 == pLmpAppInfo->curDT)
        {
            P_PLM_READ_ITEM_RSP pPlmReadItem = NULL;
            //兼容09和13规约
            P_PLC_READ_ITEM_RSP_GDW2009 pReadIfo09 = (P_PLC_READ_ITEM_RSP_GDW2009)&user_data[3];
            P_PLC_READ_ITEM_RSP_GDW2012 pReadIfo12 = (P_PLC_READ_ITEM_RSP_GDW2012)&user_data[3];

            u8 *readData = NULL;
            u8 readDataLen = 0, readFlag = 0, timeRelated = 0xff; //timeRelated默认0xff代表09协议，若非09协议0代表无延时校正1代表需要校正

            if (pLmpAppInfo->appLayerLen == (3 + sizeof(PLC_READ_ITEM_RSP_GDW2009) - 1 + pReadIfo09->len + (1 + 6 * pReadIfo09->data[pReadIfo09->len]))) //09规约
            {
                readFlag = pReadIfo09->read_flag;
                readDataLen = pReadIfo09->len;
                readData = pReadIfo09->data;
            }
            else if (pLmpAppInfo->appLayerLen == (3 + sizeof(PLC_READ_ITEM_RSP_GDW2012) - 1 + pReadIfo12->len + (1 + 6 * pReadIfo12->data[pReadIfo12->len]))) //13规约
            {
                readFlag = pReadIfo12->read_flag;
                timeRelated = pReadIfo12->time_related;
                readDataLen = pReadIfo12->len;
                readData = pReadIfo12->data;
            }
            else
            {
                debug_str(DEBUG_LOG_3762, "err AFN14, F1\r\n");
                return ERROR;
            }

            pPlmReadItem = (P_PLM_READ_ITEM_RSP)alloc_buffer(__func__, sizeof(PLM_READ_ITEM_RSP) - 1 + readDataLen);
            if (NULL != pPlmReadItem)
            {
                if (NULL != pDesAddr)
                    memcpy_special(pPlmReadItem->node_addr, pDesAddr, FRM_DL645_ADDRESS_LEN);
                else
                {
                    P_GD_DL645_BODY pDL645 = PLC_check_dl645_frame(readData, readDataLen);
                    P_DL69845_BODY pDL698 = PLC_check_69845_frame(readData, readDataLen);

                    if (pDL645)
                    {
                        memcpy_special(pPlmReadItem->node_addr, pDL645->meter_number, FRM_DL645_ADDRESS_LEN);
                    }
                    else if (pDL698)
                    {
                        PLC_get_69845_SAMac(pDL698, pPlmReadItem->node_addr);
                    }
                    else
                    {
                        memset_special(pPlmReadItem->node_addr, 0, FRM_DL645_ADDRESS_LEN);
                    }
                }
                pPlmReadItem->read_flag = readFlag;
                pPlmReadItem->time_related = timeRelated;
                pPlmReadItem->len = readDataLen;
                if (readDataLen > 0)
                    memcpy_special(pPlmReadItem->data, readData, readDataLen);
                PLC_OnPlmRequestReadItem(pPlmReadItem);
            }
        }
#ifdef GDW_2012
        else if (F2 == pLmpAppInfo->curDT)
        {
            struct tm t;
            memset(&t, 0, sizeof(t));
            PBYTE p = &user_data[3];

            t.tm_sec  = Bcd2HexChar(*p++);
            t.tm_min  = Bcd2HexChar(*p++);
            t.tm_hour = Bcd2HexChar(*p++);
            t.tm_mday = Bcd2HexChar(*p++);
            t.tm_mon  = Bcd2HexChar(*p++) - 1;
            t.tm_year = Bcd2HexChar(*p++) + 2000 - 1900;

            g_PlmStatusPara.t_gettime_sec = mktime(&t);
            g_PlmStatusPara.tick_gettime = GetTickCount64();
            g_PlmStatusPara.t_now_sec = g_PlmStatusPara.t_gettime_sec;

            debug_str(DEBUG_LOG_APP, "get local time, %02x-%02x-%02x, %02x:%02x\r\n",
                user_data[8], user_data[7], user_data[6], user_data[5], user_data[4]);
            //OSSemPost(&g_pOsRes->savedata_mbox, OS_OPT_POST_NO_SCHED, &err);

#ifdef GDW_2019_GRAPH_STOREDATA
            //通知存储曲线数据广播校时控制块发起广播校时
            PLC_StoreDataBcastTime();

            storeDateRequestCount = 0;
#if 0
            //收到路由请求集中器回复，重置周期性请求定时器
            u32 time_period = g_PlmStatusPara.store_data_param.bcast_time_period;

            storeDateRequestCount = 3;
            if (storeDataReqeustTimerId != INVAILD_TMR_ID)
            {
                HPLC_DelTmr(storeDataReqeustTimerId);
            }
            storeDataReqeustTimerId = HPLC_RegisterTmr(StoreDateRequestTime, NULL, time_period*OSCfg_TickRate_Hz, TMR_OPT_CALL_ONCE);
#endif
#endif
        }
        else if (F3 == pLmpAppInfo->curDT)
        {
            P_PLC_READ_ITEM_RSP_GDW2009 pReadIfo;
            P_GD_DL645_BODY pDL645;

            if (!(p_CccttSendMsg = alloc_send_buffer(__func__, MSG_SHORT)))
            {
                //Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);
                return ERROR;
            }
            pReadIfo = (P_PLC_READ_ITEM_RSP_GDW2009)p_CccttSendMsg->msg_buffer;
            pDL645 = (P_GD_DL645_BODY)pReadIfo->data;
            pReadIfo->read_flag = READ_ITEM_READABLE;
            pReadIfo->len = user_data[3];
            if (pReadIfo->len > 0)
                memcpy_special(pDL645, &user_data[4], pReadIfo->len);
            OSQPost(&g_pOsRes->q_up_rx_plc, p_CccttSendMsg, sizeof(p_CccttSendMsg), OS_OPT_POST_FIFO, &err);
            if (err != OS_ERR_NONE)
            {
                void *p_qmsg;
                OS_MSG_SIZE msgSize;
                while (p_qmsg = OSQPend(&g_pOsRes->q_up_rx_plc, 0, OS_OPT_PEND_NON_BLOCKING, &msgSize, NULL, &err))
                {
                    free_send_buffer(__func__, p_qmsg);
                }
                OSQPost(&g_pOsRes->q_up_rx_plc, p_CccttSendMsg, sizeof(p_CccttSendMsg), OS_OPT_POST_FIFO, &err);
                if (err != OS_ERR_NONE)
                {
                    free_send_buffer(__func__, p_CccttSendMsg);
                }
            }
        }
#endif
    }
    else if (afn_code == AFN_RESPONSE)
    {
        if (F1 == pLmpAppInfo->curDT)
        {
            PLC_OnPlmConfirm();
            OSSemPost(&g_pOsRes->savedata_mbox, OS_OPT_POST_NO_SCHED, &err);
            ClearWaitAckMsg(pLmpAppInfo->lmpLinkHead->Inf.down_info.seq_no);
        }
    }

    return OK;
}

unsigned char LMP_Response_Proc_gd(P_LMP_APP_INFO pLmpAppInfo)
{
    unsigned char afn_code, *user_data;
    OS_ERR err;

    user_data = pLmpAppInfo->lmpLinkHeadGD->user_data;

    if(pLmpAppInfo->lmpLinkHeadGD->ctl_field&0x20)
    {
        user_data += 12;
        pLmpAppInfo->appLayerLen -= 12;
    }

    afn_code = user_data[0];

    pLmpAppInfo->curDT = *(ULONG*)&user_data[2];
    pLmpAppInfo->curProcUnit = (P_LMP_DATA_UNIT)&user_data[4];

    if(afn_code == AFN_RESPONSE)
    {
        if (0xE8010001 == pLmpAppInfo->curDT || 0xE8010002 == pLmpAppInfo->curDT)
        {
            OSSemPost(&g_pOsRes->savedata_mbox, OS_OPT_POST_NO_SCHED, &err);
            ClearWaitAckMsg(user_data[1]);
        }
    }
    else if(afn_code == 0x06)
    {
        if(0xE8060601 == pLmpAppInfo->curDT)
        {
            struct tm t;
			memset(&t,0,sizeof(t));
            ClearWaitAckMsg(user_data[1]);  
            PBYTE p = &pLmpAppInfo->curProcUnit->data_unit[0];
            
            t.tm_sec  = Bcd2HexChar(*p++);
            t.tm_min  = Bcd2HexChar(*p++);
            t.tm_hour = Bcd2HexChar(*p++);
            t.tm_mday = Bcd2HexChar(*p++);
            t.tm_mon  = Bcd2HexChar(*p++)-1;
            t.tm_year = Bcd2HexChar(*p++) + 2000 -1900;

            g_PlmStatusPara.t_gettime_sec = mktime(&t);
            g_PlmStatusPara.tick_gettime = GetTickCount64();
            g_PlmStatusPara.t_now_sec = g_PlmStatusPara.t_gettime_sec;
			g_PlmStatusPara.no_response_times = 0;
			debug_str(DEBUG_LOG_APP, "get local time, %02x-%02x-%02x, %02x:%02x\r\n",
                pLmpAppInfo->curProcUnit->data_unit[5], pLmpAppInfo->curProcUnit->data_unit[4],
                pLmpAppInfo->curProcUnit->data_unit[3], pLmpAppInfo->curProcUnit->data_unit[2],
                pLmpAppInfo->curProcUnit->data_unit[1]);
            //OSSemPost(&g_pOsRes->savedata_mbox, OS_OPT_POST_NO_SCHED, &err);
        }
    }

    return OK;
}

unsigned char PLM_Get_Fn(u8 DT1, u8 DT2)
{
    u8 sn = GetOnePosition(DT1, 0);

    if(sn < 8)
    {
        return (DT2 * 8 + sn + 1);
    }
    else
    {
        return 0;
    }
}

//load manager protocol process for application layer
unsigned short LMP_App_Proc(P_LMP_APP_INFO pLmpAppInfo)
{

    //unsigned char *sendBuffer = pLmpAppInfo->msg_buffer; //g_LmpProtocolInfo.responseBuffer;

    unsigned char afn_code, *user_data, sn;

    INFO_FIELD_DOWN *d_inf = &pLmpAppInfo->lmpLinkHead->Inf.down_info;
    pLmpAppInfo->appLayerLen -= 6;      //减信息域

    user_data = pLmpAppInfo->lmpLinkHead->user_data;

    if(d_inf->comm_module)
    {
        BYTE addr_len = (pLmpAppInfo->lmpLinkHead->Inf.down_info.relay_level * 6 + 12);
        user_data += addr_len;
        pLmpAppInfo->appLayerLen -= addr_len;
    }

    afn_code = user_data[0];

    pLmpAppInfo->cur_afn_pos = find_AFN((AFN_FUNC_INFO *)_Q_GDW_AFN_PROC, afn_code);
	SetShortListenTick();//已经设置了MAC地址可以缩短时间了
    if( _Q_GDW_AFN_PROC[pLmpAppInfo->cur_afn_pos].diFunc == NULL )
    {
        if (afn_code == AFN_RESPONSE)
        {
            pLmpAppInfo->curProcUnit = (P_LMP_DATA_UNIT)(user_data+1);
            sn = GetOnePosition(pLmpAppInfo->curProcUnit->DT1, 0);

            if (sn < 8)
            {
                /* 计算得到当前' Fn '  号 */
                pLmpAppInfo->curDT = pLmpAppInfo->curProcUnit->DT2 * 8 + sn + 1;
            }
            else
            {
                return ERROR;
            }
            if (F1 == pLmpAppInfo->curDT)
            {
                OS_ERR err;
                PLC_OnPlmConfirm();
                OSSemPost(&g_pOsRes->savedata_mbox, OS_OPT_POST_NO_SCHED, &err);
                ClearWaitAckMsg(pLmpAppInfo->lmpLinkHead->Inf.down_info.seq_no);
            }
        }
        return ERROR;
    }

    /* 取得第一个' 数据单元标识 ' , 用'curProcUnit'  指向它*/
    pLmpAppInfo->curProcUnit = (P_LMP_DATA_UNIT)(user_data+1);

    sn = GetOnePosition( pLmpAppInfo->curProcUnit->DT1, 0 );

    if( sn < 8 )
    {
        /* 计算得到当前' Fn '  号 */
        pLmpAppInfo->curDT = pLmpAppInfo->curProcUnit->DT2 * 8 + sn + 1;
    }
    else
    {
        return ERROR;
    }


    switch(afn_code)
    {
        case AFN_CTL_CMD:
        case AFN_DATA_TRANS:
        case AFN_RESET:
        case AFN_MAC_CHECK:
        case AFN_RELAY_SETTING:
        case AFN_RELAY_CTROL:
        case AFN_FACTORY_WRITE:
            pLmpAppInfo->confirmFlag = 1;
            break;
		case AFN_FACTORY_RESERVED:
			{
				if((pLmpAppInfo->curDT == F15)||(pLmpAppInfo->curDT == F19))
				{
					pLmpAppInfo->confirmFlag = 1;
				}
				else
				{
					pLmpAppInfo->confirmFlag = 0;
				}
			}
			break;
        default:
            pLmpAppInfo->confirmFlag = 0;
            break;
    }

    return on_lmp_proc(PLM_FRAME_PROC_ENTRANCE, pLmpAppInfo);


}

#if defined(PROTOCOL_GD_2016)
unsigned short LMP_App_Proc_gd(P_LMP_APP_INFO pLmpAppInfo)
{
    unsigned char afn_code, *user_data;
    BYTE res;
    BYTE con_err_proc = FALSE;      //处理集中器下来的错误帧
    P_MSG_INFO p_CccttSendMsg = NULL;
    P_LMP_LINK_HEAD_GD pFrameResp = (P_LMP_LINK_HEAD_GD)&pLmpAppInfo->msg_buffer[0];
    PBYTE pAfnResp;
    ULONG* pDTResp;

    user_data = pLmpAppInfo->lmpLinkHeadGD->user_data;

    if(pLmpAppInfo->lmpLinkHeadGD->ctl_field&0x20)
    {
        user_data += 12;
        pLmpAppInfo->appLayerLen -= 12;
    }

    afn_code = user_data[0];

    pLmpAppInfo->cur_afn_pos = find_AFN((AFN_FUNC_INFO *)_Q_GD_AFN_PROC, afn_code);

    if( _Q_GD_AFN_PROC[pLmpAppInfo->cur_afn_pos].diFunc == NULL )
    {
        return ERROR;
    }

    pLmpAppInfo->curDT = *(ULONG*)&user_data[2];
    pLmpAppInfo->curProcUnit = (P_LMP_DATA_UNIT)&user_data[4];
    //pLmpAppInfo->appLayerLen = ;

    debug_str(DEBUG_LOG_NET, "AFN 0x%02x, Fn:0x%08x\r\n", afn_code, pLmpAppInfo->curDT);
    
    pLmpAppInfo->confirmFlag = 0;

    switch(afn_code)
    {
        case 0x01:
        case 0x04:      //写参数
            pLmpAppInfo->confirmFlag = 1;
            break;
        case 0x02:      //管理任务
            switch(pLmpAppInfo->curDT)
            {
                case 0xE8020201:
                case 0xE8020202:
                case 0xE8020207:
                case 0xE8020208:
                case 0xE8020209:
                    pLmpAppInfo->confirmFlag = 1;
                    break;
                default:
                    break;
            }
            break;
        case 0x06:
            con_err_proc = TRUE;
            break;
        case 0x07:
            switch(pLmpAppInfo->curDT)
            {
                case 0xE8020701:
                case 0xE8020702:
                    pLmpAppInfo->confirmFlag = 1;
                    break;
                default:
                    break;
            }
            break;
        case 0x08:
                switch(pLmpAppInfo->curDT)
                {
                    case 0xE8020801:
                    case 0xE8020802:
                    case 0xE8020805:
                    case 0xE8020806:
                        pLmpAppInfo->confirmFlag = 1;
                        break;
                    default:
                        break;
                }

                break;
		case 0xF0:
                switch(pLmpAppInfo->curDT)
                {
                    case 0xE802F010:
                    case 0xE802F011:
					case 0xE802F019:
					case 0xE802F133:
                        pLmpAppInfo->confirmFlag = 1;
                        break;
                    default:
                        break;
                }

                break;
        default:
            break;
    }

    pLmpAppInfo->responsePoint = OffsetOf(LMP_LINK_HEAD_GD, user_data);

    if(pLmpAppInfo->lmpLinkHeadGD->ctl_field&0x20)
    {
        pLmpAppInfo->responsePoint += 12;
    }

    pLmpAppInfo->responsePoint += 6;

    memcpy_special(pLmpAppInfo->msg_buffer, pLmpAppInfo->rxMsgBuf, pLmpAppInfo->lmpLinkHeadGD->frm_len);

    res = _Q_GD_AFN_PROC[pLmpAppInfo->cur_afn_pos].diFunc( pLmpAppInfo );

    pAfnResp = &pFrameResp->user_data[0];
    pDTResp = (ULONG*)&pFrameResp->user_data[2];
    if(pFrameResp->ctl_field&0x20)
    {
        if(pLmpAppInfo->confirmFlag || OK != res)
        {
            //确认报文，去掉地址域
            pFrameResp->ctl_field &= (~0x20);
            memmove_special(&pFrameResp->user_data[0], &pFrameResp->user_data[12], pLmpAppInfo->responsePoint-16);
            pLmpAppInfo->responsePoint -= 12;
        }
        else
        {
            //源地址目标地址交互
            BYTE tmp_addr[6];

            memcpy_special(&tmp_addr[0], &pFrameResp->user_data[0], 6);
            memcpy_special(&pFrameResp->user_data[0], &pFrameResp->user_data[6], 6);
            memcpy_special(&pFrameResp->user_data[6], &tmp_addr[0], 6);

            pAfnResp = &pFrameResp->user_data[12+0];
            pDTResp = (ULONG*)&pFrameResp->user_data[12+2];
        }
    }

    if(pLmpAppInfo->confirmFlag || OK != res)
    {
        *pAfnResp = 0x00;
        if(OK == res)
            *pDTResp = 0xE8010001;
        else
            *pDTResp = 0xE8010002;
    }
    else if(0x02==afn_code)
    {
        if(0xE8030204==pLmpAppInfo->curDT)
            *pDTResp = 0xE8040204;
        if(0xE8030205==pLmpAppInfo->curDT)
		{
			if(pLmpAppInfo->responsePoint == 10) //没有得到任务信息，返回否认帧
			{
				*pAfnResp = 0x00;
				*pDTResp = 0xE8010002;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 17; //task id no exist
			}
			else
			{
				*pDTResp = 0xE8040205;
			}
		}
    }
    else if(0x03==afn_code)
    {
		switch (pLmpAppInfo->curDT)
		{
			case 0xE8030304:
				*pDTResp = 0xE8040304;
				break;
			case 0xE8030306:
				*pDTResp = 0xE8040306;
				break;
			case 0xE8030308:
				*pDTResp = 0xE8040308;
				break;
			case 0xE803030A:
				*pDTResp = 0xE804030A;
				break;
			case 0xE803030C:
				*pDTResp = 0xE804030C;
				break;
			case 0xE803030D:
				*pDTResp = 0xE804030D;
				break;
			case 0xE803030E:
				*pDTResp = 0xE804030E;
				break;
			case 0xE803030F:
				*pDTResp = 0xE804030F;
				break;
			case 0xE8030310:
				*pDTResp = 0xE8040310;
				break;
			case 0xE8030311:
				*pDTResp = 0xE8040311;
				break;
			case 0xE8030312:
				*pDTResp = 0xE8040312;
				break;
			case 0xE803F012:
				*pDTResp = 0xE804F012;
				break;
            case 0xE8030381:
                *pDTResp = 0xE8040381;
				break;
			case 0xE8030313:
				if ((pLmpAppInfo->lmpLinkHeadGD->ctl_field&0x20)) //地址域
				{
					if (pLmpAppInfo->responsePoint == 22) //等待回复帧上报
					{
						return OK;
					}
				}
				else
				{
					if (pLmpAppInfo->responsePoint == 10) //等待回复帧上报
					{
						return OK;
					}
				}
				*pDTResp = 0xE8040313;
				break;
			case 0xE8030314:
				*pDTResp = 0xE8040314;
				break;
            case 0xE8030321:
				*pDTResp = 0xE8040321;
				break;
            case 0xE8030322:
				*pDTResp = 0xE8040322;
				break;
			case 0xE8030365:
				*pDTResp = 0xE8040365;
				break;
			case 0xE8000391:
				*pDTResp = 0xE8000391;
				break;
			case 0xE8000390:
				*pDTResp = 0xE8000390;
				break;
			case 0xE8000393:
				*pDTResp = 0xE8000393;
				break;
			case 0xE8030366:
				*pDTResp = 0xE8040366;
				break;
			case 0xE8030367:
				*pDTResp = 0xE8040367;
				break;
			case 0xE8030368:
				if (pLmpAppInfo->responsePoint==10)//查询节点信道信息 为0时等待回复帧上报
				{
					return OK;
				}
				*pDTResp = 0xE8040368;
				break;
            case 0xE8030398:
				if (pLmpAppInfo->responsePoint==10)//查询节点信道信息 为0时等待回复帧上报
				{
					return OK;
				}
				*pDTResp = 0xE8040398;
				break;    
			case 0xE8030369:
				*pDTResp = 0xE8040369;
				break;
			case 0xE8030370:
				*pDTResp = 0xE8040370;
				break;
			case 0xE8030364:
				*pDTResp = 0xE8040364;
				break;
			case 0xE8030396:
				*pDTResp = 0xE8040396;
				break;
		}
    }
    else if(0x08==afn_code)
    {
        if(0xE8030804==pLmpAppInfo->curDT)
            *pDTResp = 0xE8040804;
        if(0xE8000803==pLmpAppInfo->curDT)
            *pDTResp = 0xE8000803;
        if(0xE8030807==pLmpAppInfo->curDT)
            *pDTResp = 0xE8040807;
    }
    else if(0x07==afn_code)
    {
        if(0xE8030705==pLmpAppInfo->curDT)
            *pDTResp = 0xE8040705;
    }
    else if (0xF0 == afn_code)
    {
        //查询从节点内部版本信息
        if (0xE803F0E3 == pLmpAppInfo->curDT) //不回复，由从节点载波消息触发回复
            return OK;
        //查询从节点功率
        if (0xE802F222 == pLmpAppInfo->curDT) //不回复，由从节点载波消息触发回复
            return OK;
		//查询从节点rf功率
		if (0xE802F230 == pLmpAppInfo->curDT) //不回复，由从节点载波消息触发回复
			return OK;
		//查询从节点功率 新接口
        if (0xE802F172 == pLmpAppInfo->curDT) //不回复，由从节点载波消息触发回复
            return OK;
		//查询从节点rf功率 新接口
        if (0xE802F174 == pLmpAppInfo->curDT) //不回复，由从节点载波消息触发回复
            return OK;
		//查询从节点寄存器值
		if (0xE800F0E8 == pLmpAppInfo->curDT) //不回复，由从节点载波消息触发回复
            return OK;
    }

    pFrameResp->ctl_field |= 0x80;
    pFrameResp->ctl_field &= (~0x40);

    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = Get_checksum(&pFrameResp->ctl_field, pLmpAppInfo->responsePoint-3);
    pLmpAppInfo->responsePoint++;
    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x16;
    pFrameResp->frm_len = pLmpAppInfo->responsePoint;

    if(!con_err_proc)
    {
        p_CccttSendMsg = alloc_send_buffer(__func__,MSG_LONG);

        if(p_CccttSendMsg)
        {
            p_CccttSendMsg->msg_header.need_buffer_free = TRUE;
            p_CccttSendMsg->msg_header.end_id = pLmpAppInfo->end_id;
            p_CccttSendMsg->msg_header.msg_len = pFrameResp->frm_len;
			p_CccttSendMsg->msg_header.send_delay = false;
            memcpy_special(&p_CccttSendMsg->msg_buffer[0], pFrameResp, pFrameResp->frm_len);

            res = End_post(p_CccttSendMsg);
            debug_str(DEBUG_LOG_INFO, "End_post res:%d\r\n", res);
        }
    }

    return OK;
}
#endif

#ifdef PROTOCOL_GD_2016

P_MSG_INFO LMP_make_frame_gd_cmd(u8 Afn, ULONG DI, PBYTE pDataUnit, USHORT nDataLen, PBYTE pSrcAddr)
{
    P_MSG_INFO pMsg = NULL;
    USHORT p;
    P_LMP_LINK_HEAD_GD pFrame_gd;
	USHORT local_message_len = ((pSrcAddr) ? 24 : 12);  // 0X68-1字节，报文长度-2字节，控制域-1字节，地址域（如有）-12字节，AFN-1字节，seq-1字节，DI-4字节，CS-1字节，0x16-1字节

	if((nDataLen + local_message_len) > PLC_BUFFER_UART_SIZE)
	{
	  if( !(pMsg = alloc_send_buffer(__func__,MSG_LONG)) )
	  {
		  //Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);
		  return NULL;
	  }
	}  
    else 
    {
	  if( !(pMsg = alloc_send_buffer(__func__,MSG_SHORT)) )
	  {
        //Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);
        return NULL;
	  }
    }

    p = 0;

    pFrame_gd = (P_LMP_LINK_HEAD_GD)&pMsg->msg_buffer[0];
    pFrame_gd->firstStartByte = FRAME_PLM2005_START_FLAG;

    if(pSrcAddr)
    {
        pFrame_gd->ctl_field = 0xe0;
        memcpy_special(&pFrame_gd->user_data[p], pSrcAddr, 6);
        p += 6;
        memcpy_special(&pFrame_gd->user_data[p], &g_PlmConfigPara.main_node_addr[0], 6);
        p += 6;
    }
    else
    {
		//机台bug 不是0x80过不了
		if ((0xE8040313==DI) || (0xE8040368==DI) || (0xE8040398==DI))
		{
			pFrame_gd->ctl_field = 0x80;
		}
		else
		{
			pFrame_gd->ctl_field = 0xc0;
		}
    }

    pFrame_gd->user_data[p++] = Afn;       //AFN
	if (Afn==0x3&&DI==0xE8040368)
	{
		pFrame_gd->user_data[p++] = g_pRelayStatus->sta_channel_cb.report_sn;       //SEQ
	}
    else if (Afn==0x3&&DI==0xE8040398)
	{
		pFrame_gd->user_data[p++] = g_pRelayStatus->sta_new_channel_cb.report_sn;       //SEQ
	}
	else if (Afn==0x3&&DI==0xE8040313)
	{
		pFrame_gd->user_data[p++] = g_pRelayStatus->sta_info_realtime_cb.report_sn;
	}
	else
	{
		pFrame_gd->user_data[p++] = g_PlmStatusPara.plm_seq_no_up++;       //SEQ
	}
    *(ULONG*)&pFrame_gd->user_data[p] = DI;
    p += 4;

    if((NULL!=pDataUnit) && (nDataLen>0))
    {
        memcpy_special(&pFrame_gd->user_data[p], pDataUnit, nDataLen);
        p += nDataLen;
    }

    pFrame_gd->user_data[p] = Get_checksum((PBYTE)&pFrame_gd->ctl_field, p+1);
    p++;
    pFrame_gd->user_data[p++] = FRAME_PLM2005_END_FLAG;

    pFrame_gd->frm_len = 4+p;

    pMsg->msg_header.msg_len = pFrame_gd->frm_len;
    pMsg->msg_header.end_id = COM_PORT_MAINTAIN;
    pMsg->msg_header.need_buffer_free = TRUE;
	pMsg->msg_header.send_delay = TRUE;
    return pMsg;
}


void PLM_ReportTaskRead_gd(u8 sour_frome,u16 task_id, u8 mac_addr[MAC_ADDR_LEN], u8* pRespData, u16 nRespLen)
{
    USHORT p;
//    P_GD_DL645_BODY pDL645_task;
    P_MSG_INFO p_CccttSendMsg = NULL;   
	u8* pDataUnit = NULL;

#ifdef GRAPH_COLLECT_EXTEND
	if (*(u32 *)(pRespData+1) == 0x0702FF00 || *(u32 *)(pRespData+1) == 0x0702FF01)
	{
		pDataUnit = alloc_buffer(__func__, 4 + nRespLen);

		if (NULL == pDataUnit)
			return;

		p = 0;
		memcpy_special(&pDataUnit[p], &task_id, 2);
		p += 2;
		memcpy_special(&pDataUnit[p], &nRespLen, 2);
		p += 2;
		memcpy_special(&pDataUnit[p], pRespData, nRespLen);
		p += nRespLen;
		p_CccttSendMsg = LMP_make_frame_gd_cmd(0x05, 0xE8050508, pDataUnit, p, mac_addr);//上报分钟级冻结数据
	}
	else
#endif
	{
		pDataUnit = alloc_buffer(__func__, 3 + nRespLen);

		if (NULL == pDataUnit)
			return;

		p = 0;
		memcpy_special(&pDataUnit[p], &task_id, 2);
		p += 2;
		pDataUnit[p++] = nRespLen;
		memcpy_special(&pDataUnit[p], pRespData, nRespLen);
		p += nRespLen;
		p_CccttSendMsg = LMP_make_frame_gd_cmd(0x05, 0xE8050501, pDataUnit, p, mac_addr);
	}
	
    free_buffer(__func__,pDataUnit);
	
#if 1
    if(NULL != p_CccttSendMsg)
	{
		p_CccttSendMsg->msg_header.end_id=sour_frome;
        End_post(p_CccttSendMsg);
	}
#else
    p_CccttSendMsg->msg_header.need_buffer_free = FALSE;

    for(BYTE i=0; i<3; i++)
    {
        OS_ERR err;
        while(0 != OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz, OS_OPT_PEND_NON_BLOCKING, NULL, &err));
        End_post(p_CccttSendMsg);
        OSSemPend(&g_pOsRes->savedata_mbox, 3*OSCfg_TickRate_Hz, OS_OPT_PEND_BLOCKING, NULL, &err);
        if(OS_ERR_NONE==err_code)
            break;
    }

    free_send_buffer(__func__,p_CccttSendMsg);
#endif
}
//上报
void PLM_ReportTaskStatus_gd(u8 sour_frome,USHORT task_id, PBYTE pNodeAddr, u8 task_status)
{
    P_MSG_INFO p_CccttSendMsg;
    u8 dataUnit[10];
    u8 p;

    p = 0;

    *(USHORT*)&dataUnit[p] = task_id;
    p += 2;
	if (pNodeAddr)
	{
		memcpy_special(&dataUnit[p], pNodeAddr, 6);
	}
	else
	{
		memset(&dataUnit[p], 0xff, 6);
	}
	p += 6;
    dataUnit[p++] = task_status;

    p_CccttSendMsg = LMP_make_frame_gd_cmd(0x05, 0xE8050505, dataUnit, p, NULL);
	
    if(NULL != p_CccttSendMsg)
	{
		p_CccttSendMsg->msg_header.end_id=sour_frome;
        End_post(p_CccttSendMsg);
	}
}

//上报sta运行状态
void PLM_ReportStaStatus_gd(PBYTE data, u16 len)
{
    P_MSG_INFO p_CccttSendMsg;
	if (data!=NULL)
	{
		p_CccttSendMsg = LMP_make_frame_gd_cmd(0x03, 0xE8040366, data, len, NULL);
	}
	else
	{
		u32 temp=0;
		p_CccttSendMsg = LMP_make_frame_gd_cmd(0x03, 0xE8040366, (PBYTE)&temp, 4, NULL);
	}
    if(NULL != p_CccttSendMsg)
        End_post(p_CccttSendMsg);
}

//上报节点信道状态
void PLM_ReportChannelInfo_gd(PBYTE data,u16 len)
{
    P_MSG_INFO p_CccttSendMsg;

	if (data!=NULL) 
	{
#if 0
#define UP_LEN (6+2+2+1+1+1+1)  //mac +tei + pcotei +level + drate+ urate +ud rate
		//移出信噪比和衰减值信息
		u8 num=data[2];
		u8* pstart=&data[3];
		u8* pend= pstart;
		for (int i=0;i<num;i++)
		{
			memcpy(pstart,pend,UP_LEN);
			pstart+=UP_LEN;
			pend+=UP_LEN+2;
			len-=2;
		}
#else
#define UP_LEN (6+2+2+1+1+1+1+1+1)  //mac +tei + pcotei +level + drate+ urate +ud rate + snr + rssi
		u8 num=data[2];
		u8* pstart=&data[3];
		for (int i=0;i<num;i++)
		{
			for (int j=0;j<3;j++)
			{
				u8 temp=pstart[j];
				pstart[j]=pstart[5-j];
				pstart[5-j]=temp;
			}
			pstart += UP_LEN;
		}
#endif
		p_CccttSendMsg = LMP_make_frame_gd_cmd(0x03, 0xE8040368, data, len, NULL);
	}
	else
	{
		u32 temp=0;
		p_CccttSendMsg = LMP_make_frame_gd_cmd(0x03, 0xE8040368, (PBYTE)&temp, 3, NULL);
	}
    if(NULL != p_CccttSendMsg)
	{
		p_CccttSendMsg->msg_header.send_delay=false;
        End_post(p_CccttSendMsg);
	}
}

void PLM_ReportChannelNewInfo_gd(PBYTE data,u16 len)
{
    P_MSG_INFO p_CccttSendMsg;

	if (data!=NULL) 
	{
#define UP_LEN (6+2+2+1+1+1+1+1+1)  //mac +tei + pcotei +level + drate+ urate +ud rate + snr + rssi
		u8 num = data[12];
		u8* pstart = &data[13];
		for (int i=0; i<num; i++)
		{
			for (int j=0; j<3; j++)
			{
				u8 temp = pstart[j];
				pstart[j] = pstart[5-j];
				pstart[5-j] = temp;
			}
			pstart += UP_LEN;
		}

		p_CccttSendMsg = LMP_make_frame_gd_cmd(0x03, 0xE8040398, data, len, NULL);
	}
	else
	{
		u32 temp=0;
		p_CccttSendMsg = LMP_make_frame_gd_cmd(0x03, 0xE8040398, (PBYTE)&temp, 3, NULL);
	}
    if(NULL != p_CccttSendMsg)
	{
		p_CccttSendMsg->msg_header.send_delay=false;
        End_post(p_CccttSendMsg);
	}
}


//回复sta模块资产信息
void PLM_ReadStaModuleInfoResponse_gd(PBYTE data, u16 len)
{
    P_MSG_INFO p_CccttSendMsg;

	p_CccttSendMsg = LMP_make_frame_gd_cmd(0x03, 0xE8040313, data, len, NULL);
	
    if(NULL != p_CccttSendMsg)
    {
    	p_CccttSendMsg->msg_header.send_delay = false;
        End_post(p_CccttSendMsg);
    }
}

void PLM_RequestCentreTime_gd(void)
{
    P_MSG_INFO p_CccttSendMsg;

	p_CccttSendMsg = LMP_make_frame_gd_cmd(0x06, 0xE8060601, NULL, 0, NULL);
	
    if(NULL != p_CccttSendMsg)
        End_post(p_CccttSendMsg);
}
//上报采集任务状态
void PLM_ReportCollTaskStatus_gd(u8 task_id, PBYTE pNodeAddr, u8 task_status)
{
}

//上报采集数据读取任务状态
void PLM_ReportCollDataTaskStatus_gd(u8 task_id, PBYTE pNodeAddr, u8 task_status)
{
    //
}

//上报采集数据读取任务数据
void PLM_ReportCollDataTaskData_gd(u8* pRespData, u16 nRespLen)
{
    //
}

//删除采集任务
//status: 0 成功，其它失败
void PLM_DeleteCollTaskResponse_gd(u8 packet_seq_3762, u8 status)
{
}

//查询采集任务号
void PLM_ReadCollTaskIDResponse_gd(u8 packet_seq_3762, u8* pRespData, u16 nRespLen)
{
    //
}

//返回查询采集任务详细信息
void PLM_ReadCollTaskDetailResponse_gd(u8 packet_seq_3762, u8* pRespData, u16 nRespLen)
{
    //
}

#endif

//3762上行命令报文组桢
//phase AC_PHASE
P_MSG_INFO LMP_make_frame_3762_cmd(u8 Afn, u8 Fn, PLM_PHASE phase, PBYTE pDataUnit, USHORT nDataLen, BOOL isPlcMeterCommModule, PBYTE pSrcAddr)
{
    P_LMP_LINK_HEAD pFrame;
    PBYTE p;
    P_MSG_INFO pMsg = NULL;
	MSG_TTYPE type_msg=MSG_SHORT;

	if(isPlcMeterCommModule && pSrcAddr)
	{
		if((nDataLen+sizeof(LMP_LINK_HEAD)-1+12+6) > 256) //6=1(AFN)+2(DT)+2(CS)+1(0x16)，0x68等帧头部分已包含在LMP_LINK_HEAD里
		{
			type_msg = MSG_LONG;
		}
	}
	else
	{
		if((nDataLen+sizeof(LMP_LINK_HEAD)-1+6) > 256)
		{
			type_msg = MSG_LONG;
		}
	}
	if (!(pMsg = (P_MSG_INFO)alloc_send_buffer(__func__,type_msg)))
    {
        //Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);
        return NULL;
    }

    pFrame = (P_LMP_LINK_HEAD)pMsg->msg_buffer;
    p = pFrame->user_data;

    memset_special((PBYTE)pFrame, 0, sizeof(LMP_LINK_HEAD));

    pFrame->firstStartByte = 0x68;
    
	#if defined(HRF_SUPPORT) && !defined(COMM_MODE_OLD)
	pFrame->ctl_field = 0xc4;
	#else
	pFrame->ctl_field = 0xc3;
	#endif

    if(((AFN_DATA_TRANS == Afn) && (F5 == Fn))
       ||((AFN_RESET == Afn) && (F7 == Fn)))
    {
        pFrame->ctl_field = 0xc7;
    }

    if((AFN_REPORT_INFO == Afn) && (F5 == Fn))
    {
        pFrame->Inf.up_info.event_flag = 1;
    }

    if(isPlcMeterCommModule && pSrcAddr)  //对载波表的通信模块操作
    {
        //USHORT metersn;
        pFrame->Inf.up_info.comm_module = 1;
        //源地址A1
        memcpy_special(p, pSrcAddr, 6);
        p += 6;
        //中继地址A2
        pFrame->Inf.up_info.relay_level = 0;
        //目的地址A3
        memcpy_special(p, g_PlmConfigPara.main_node_addr, 6);
        p += 6;
    }
#ifdef GDW_2012
    pFrame->Inf.up_info.seq_no = g_PlmStatusPara.plm_seq_no_up++;
#endif

    pFrame->Inf.up_info.channel_phase = (u8)phase;

    pFrame->Inf.up_info.channel_flag = pFrame->Inf.up_info.channel_phase;

    *p++ = Afn;
    *p++ = (BYTE)(1<<((Fn-1)&0x07));    //DT1
    *p++ = (BYTE)((Fn-1)>>3);        //DT2
    pFrame->frm_len = 6
                      + sizeof(pFrame->Inf)
                      + (isPlcMeterCommModule?12:0)
                      + 3
                      + nDataLen;

    if(nDataLen)
    {
        memcpy_special(p, pDataUnit, nDataLen);
        p += nDataLen; //数据单元
    }

    *((PBYTE)pFrame + pFrame->frm_len - 2) = Get_checksum((PBYTE)&pFrame->ctl_field, pFrame->frm_len-5);
    *((PBYTE)pFrame + pFrame->frm_len - 1) = 0x16;

    pMsg->msg_header.msg_len = pFrame->frm_len;
    pMsg->msg_header.end_id = COM_PORT_MAINTAIN;
    pMsg->msg_header.need_buffer_free = TRUE;
	pMsg->msg_header.send_delay = false;
    return pMsg;
}

//3762上行应答报文组桢
P_MSG_INFO LMP_make_frame_3762_resp(u8 Afn, u8 Fn, PLM_PHASE phase, u8* pDataUnit, USHORT nDataLen, BOOL isPlcMeterCommModule, u8* pSrcAddr, u8 packet_seq)
{
    P_LMP_LINK_HEAD pFrame;
    PBYTE p;
    P_MSG_INFO pMsg = NULL;
	MSG_TTYPE tp;
	if (nDataLen>256)
	{
		tp=MSG_LONG;
	}
	else
	{
		tp=MSG_SHORT;
		
		if(isPlcMeterCommModule && pSrcAddr)
		{
			if((nDataLen+sizeof(LMP_LINK_HEAD)-1+12+6) > 256) //6=1(AFN)+2(DT)+2(CS)+1(0x16), 0x68等帧头部分已包含在LMP_LINK_HEAD里
			{
				tp = MSG_LONG;
			}
		}
		else
		{
			if((nDataLen+sizeof(LMP_LINK_HEAD)-1+6) > 256)
			{
				tp = MSG_LONG;
			}
		}
		
	}
	if (!(pMsg = (P_MSG_INFO)alloc_send_buffer(__func__,tp)))
    {
        //Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);
        return NULL;
    }

    pFrame = (P_LMP_LINK_HEAD)pMsg->msg_buffer;
    p = pFrame->user_data;

    memset_special((PBYTE)pFrame, 0, sizeof(LMP_LINK_HEAD));

    pFrame->firstStartByte = 0x68;
    pFrame->ctl_field = 0x83;

    if(((AFN_DATA_TRANS == Afn) && (F5 == Fn))
       ||((AFN_RESET == Afn) && (F7 == Fn)))
    {
        pFrame->ctl_field = 0xc7;
    }

    if(isPlcMeterCommModule && pSrcAddr)  //对载波表的通信模块操作
    {
        //USHORT metersn;
        pFrame->Inf.up_info.comm_module = 1;
        //源地址A1
        memcpy_special(p, pSrcAddr, 6);
        p += 6;
        //中继地址A2
        pFrame->Inf.up_info.relay_level = 0;
        //目的地址A3
        memcpy_special(p, g_PlmConfigPara.main_node_addr, 6);
        p += 6;
    }
#ifdef GDW_2012
    pFrame->Inf.up_info.seq_no = packet_seq;
#endif
    if (!mem32IsHex(g_pRelayStatus->area_dis_cb.zoneErrorMap,0,sizeof(g_pRelayStatus->area_dis_cb.zoneErrorMap)))
    {
        pFrame->Inf.up_info.area_flag=1;//线路异常标志
    }
    pFrame->Inf.up_info.channel_phase = (u8)phase & 0x7f;
    if ((phase&PLM_PHASE_ERR)==PLM_PHASE_ERR)
    {
        pFrame->Inf.up_info.line_flag=1;
    }

    pFrame->Inf.up_info.channel_flag = pFrame->Inf.up_info.channel_phase;

    *p++ = Afn;
    *p++ = (BYTE)(1<<((Fn-1)&0x07));    //DT1
    *p++ = (BYTE)((Fn-1)>>3);        //DT2
    pFrame->frm_len = 6
                      + sizeof(pFrame->Inf)
                      + (isPlcMeterCommModule?12:0)
                      + 3
                      + nDataLen;

    if(nDataLen)
    {
        memcpy_special(p, pDataUnit, nDataLen);
        p += nDataLen; //数据单元
    }

    *((PBYTE)pFrame + pFrame->frm_len - 2) = Get_checksum((PBYTE)&pFrame->ctl_field, pFrame->frm_len-5);
    *((PBYTE)pFrame + pFrame->frm_len - 1) = 0x16;

    pMsg->msg_header.msg_len = pFrame->frm_len;
    pMsg->msg_header.end_id = COM_PORT_MAINTAIN;
    pMsg->msg_header.need_buffer_free = TRUE;
	pMsg->msg_header.send_delay = false;
    return pMsg;
}

#if defined(FUNC_HPLC_READER)

void PLM_ReportHPLCReader(u32 ntb, s32 rssi, s32 snr, u8* pData, u32 len)
{
    // 13 37 79 91 + 98 AE 5C 8B 监控NTB + 48 00 00 00 rssi + 7E 00 00 00 00 SNR + 51 02 00 D7 72 A6 16 01 40 B6 02 00 75 DE 56 CA EB 11 22 33 44 55 66 01 00 00 00 00 00 00 00 00 00 00 00 03 00 0F 01 00 00 64 11 22 33 44 55 66 04 1E 01 01 0A 14 00 12 00 02 00 02 00 C0 23 00 00 33 00 00 1E 32 00 00 00 00 4A 10 A6 16 D0 07 00 00 00 00 7C 02 00 01 7C 02 00 02 7E 02 00 03 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 B7 A3 6D FB 39 9F E8 
    //alloc_send_buffer(__func__,MSG_TTYPE type)
    P_MSG_INFO pSendMsg = alloc_send_buffer_by_len(16+len);

    if(NULL == pSendMsg)
        return;

#if defined(DEBUG_PORT_CONCENTRATOR)
    pSendMsg->msg_header.end_id = COM_PORT_MAINTAIN;
#else
    pSendMsg->msg_header.end_id = COM_PORT_DEBUG;
#endif

    pSendMsg->msg_header.msg_len = 16+len;
	pSendMsg->msg_header.send_delay=false;
    u16 p = 0;

    u32 header = 0x91793713;

    memcpy_special(&pSendMsg->msg_buffer[p], &header, 4);
    p += 4;

    memcpy_special(&pSendMsg->msg_buffer[p], &ntb, 4);
    p += 4;

    memcpy_special(&pSendMsg->msg_buffer[p], &rssi, 4);
    p += 4;

    memcpy_special(&pSendMsg->msg_buffer[p], &snr, 4);
    p += 4;

    memcpy_special(&pSendMsg->msg_buffer[p], pData, len);
    p += len;

    End_post(pSendMsg);
}

#endif

void PLM_ReportDirRead(P_READ_MSG_INFO pReadMsg, PLM_PHASE phase, u8* pRespData, u16 nRespLen)
{
    u8 AFN = pReadMsg->header.res_afn;
    if (!(AFN & 0x80))
    {
        if (nRespLen > 255)
        {
            nRespLen = 255;
        }
    }
    u8 *pDataUnit = (u8 *)alloc_buffer(__func__, nRespLen + 4);
    int p = 0;
    if (NULL == pDataUnit)
        return;

    if (NULL == pRespData)
        nRespLen = 0;


    u8 Fn = F1;
    if (AFN == AFN_FRAME_FORWARD
        && pReadMsg->header.sender == EM_SENDER_AFN_GDW2013)
    {
        pDataUnit[p++] = 1;
        pDataUnit[p++] = 0;
    }
    pDataUnit[p++] = PLC_plcprotocol_to_3762protocol(pReadMsg->header.meter_protocol_plc);
    pDataUnit[p++] = nRespLen & 0xff;
    if (AFN & 0x80)
    {
        pDataUnit[p++] = nRespLen >> 8;
        AFN &= 0x7F;
        Fn = F2;
    }

    if (NULL != pRespData && nRespLen > 0)
    {
        memcpy_special(&pDataUnit[p], pRespData, nRespLen);
        p += nRespLen;
    }

    P_MSG_INFO pSendMsg = LMP_make_frame_3762_resp(AFN, Fn, phase, pDataUnit, p, TRUE, pReadMsg->header.mac, (u8)pReadMsg->header.packet_seq_3762);

    free_buffer(__func__, pDataUnit);

    if (NULL != pSendMsg)
    {
#ifdef DEBUG_FRMAE_3762
        debug_hex(false, (u8 *)pSendMsg->msg_buffer, pSendMsg->msg_header.msg_len);
#endif
        pSendMsg->msg_header.end_id = pReadMsg->header.sour_forme;
        End_post(pSendMsg);
    }
}

void PLM_ReportRouteRead(u8 protoxol_09or13,u16 user_sn, u8 node_addr[FRM_DL645_ADDRESS_LEN], u8 plm_protocol, PLM_PHASE phase, u8 took_time_sec, u8* pRespData, u16 nRespLen)
{
    u16 len;
    P_MSG_INFO pSendMsg = NULL;
    if (protoxol_09or13)
    {
        len = sizeof(RT_09REPORT_READ_RESULT) - 1 + nRespLen;
        P_RT_09REPORT_READ_RESULT pDataUnit = (P_RT_09REPORT_READ_RESULT)alloc_buffer(__func__, len);

        if (NULL == pDataUnit)
            return;

        pDataUnit->meter_sn = user_sn;
        pDataUnit->protocol = plm_protocol;
        pDataUnit->data_len = nRespLen;
        if (nRespLen > 0)
            memcpy_special(pDataUnit->data, pRespData, nRespLen);

        pSendMsg = LMP_make_frame_3762_cmd(AFN_REPORT_INFO, F2, phase, (u8 *)pDataUnit, len, TRUE, node_addr);

        free_buffer(__func__, pDataUnit);
    }
    else
    {
        len = sizeof(RT_13REPORT_READ_RESULT) - 1 + nRespLen;
        P_RT_13REPORT_READ_RESULT pDataUnit = (P_RT_13REPORT_READ_RESULT)alloc_buffer(__func__, len);

        if (NULL == pDataUnit)
            return;

        pDataUnit->meter_sn = user_sn;
        pDataUnit->protocol = plm_protocol;
        pDataUnit->read_took_time = took_time_sec;
        pDataUnit->data_len = nRespLen;
        if (nRespLen > 0)
            memcpy_special(pDataUnit->data, pRespData, nRespLen);

        pSendMsg = LMP_make_frame_3762_cmd(AFN_REPORT_INFO, F2, phase, (u8 *)pDataUnit, len, TRUE, node_addr);

        free_buffer(__func__, pDataUnit);
    }
    if (NULL != pSendMsg)
    {
#ifdef DEBUG_FRMAE_3762
        debug_hex(false, (u8 *)pSendMsg->msg_buffer, pSendMsg->msg_header.msg_len);
#endif
        End_post(pSendMsg);
    }
}

void PLM_ReportParallelRead(P_READ_MSG_INFO pReadMsg, PLM_PHASE phase, u16 upType, u8* pRespData, u16 nRespLen)
{
    u8* pDataUnit = (u8*)alloc_buffer(__func__,nRespLen + 3);
    int p = 0;
    if(NULL == pDataUnit)
        return;

    if(NULL==pRespData)
        nRespLen = 0;

    u8 AFN = AFN_PARALLEL_READ_METER;
    u8 Fn = F1;
	if(APP_PACKET_PARALLEL == upType)
	{
		Fn = F1;
	}
	else if (APP_PACKET_STORE_DATA_PARALLEL == upType)
	{
		Fn = F100;
	}


    pDataUnit[p++] = PLC_plcprotocol_to_3762protocol(pReadMsg->header.meter_protocol_plc);
    pDataUnit[p++] = (u8)nRespLen;
    pDataUnit[p++] = (u8)(nRespLen>>8);

    if(NULL!=pRespData && nRespLen>0)
    {
        memcpy_special(&pDataUnit[p], pRespData, nRespLen);
        p += nRespLen;
    }

    P_MSG_INFO pSendMsg = LMP_make_frame_3762_resp(AFN, Fn, phase, pDataUnit, p, TRUE, pReadMsg->header.mac, (u8)pReadMsg->header.packet_seq_3762);

    free_buffer(__func__,pDataUnit);

    if(NULL != pSendMsg)
    {
#ifdef DEBUG_FRMAE_3762
        debug_hex(false, (u8*)pSendMsg->msg_buffer, pSendMsg->msg_header.msg_len);
#endif
		pSendMsg->msg_header.end_id = pReadMsg->header.sour_forme;
        End_post(pSendMsg);
    }
}

#ifdef HPLC_CSG
bool PLM_ReportSelfReg(P_APP_SEARCH_UP pRegResult)
{
#if 1
	u8 index = 0, result_index = 0;
	u16 alloc_len = 1 + 6 * pRegResult->num; 

	u8 *pDataUnit = alloc_buffer(__func__, alloc_len);

	if (NULL == pDataUnit)
        return false;

	pDataUnit[index++] = pRegResult->num;
	for (u8 i=0; i<pRegResult->num; i++)
    {
        memcpy_special(&pDataUnit[index], &pRegResult->data[result_index], MAC_ADDR_LEN);
        index += MAC_ADDR_LEN;
		result_index += MAC_ADDR_LEN+2;
    }
	
	P_MSG_INFO pSendMsg = LMP_make_frame_gd_cmd(0x05, 0xE8050503, pDataUnit, alloc_len, NULL);

    free_buffer(__func__, pDataUnit);
#else
	u16 len = 1 + 6 * pRegResult->num; 
	pRegResult->res[2]=pRegResult->num;
	P_MSG_INFO pSendMsg = LMP_make_frame_gd_cmd(0x05, 0xE8050503, &pRegResult->res[2], len, NULL);
#endif	
	if (!pSendMsg)
		return false;
	
	return (bool)End_post(pSendMsg);
}
#else
u32 rpt_counter = 0;
extern u32 selfreg_bitmap[MAX_BITMAP_SIZE/4];
bool PLM_ReportSelfReg(P_REGIST_CHECK_UP pRegResult)
{
    P_REGIST_METER pRegMeter = (P_REGIST_METER)pRegResult->Data;

    u16 alloc_len = 16 + 7 * pRegResult->MeterNum;

    u8 *pDataUnit = alloc_buffer(__func__, alloc_len);

    if (NULL == pDataUnit)
        return false;

    u8 mac_addr[MAC_ADDR_LEN];
    memcpy_swap(mac_addr, pRegResult->SrcMac, MAC_ADDR_LEN);
    //memcpy_special(mac_addr, pRegResult->DeviceAddr, MAC_ADDR_LEN);

    P_AFN06_F4_DEV_INFO pDevInfo = (P_AFN06_F4_DEV_INFO)(pDataUnit);

    pDevInfo->node_count = 1;
    P_AFN06_F4_NODE_INFO pNodeInfo = (P_AFN06_F4_NODE_INFO)pDevInfo->data;
#ifdef GDW_JIANGSU
	P_STA_INFO pSta = PLC_GetStaInfoByMac(mac_addr);
	if (pSta)
	{
		memcpy_special(pNodeInfo->node_addr, &(pSta->module_id[5]), 6);
	}
#else
    memcpy_special(pNodeInfo->node_addr, mac_addr, 6);
#endif
    pNodeInfo->protocol = METER_PROTOCOL_3762_TRANSPARENT;
    pNodeInfo->node_sn = 0;

    u16 meter_sn = PLC_get_sn_from_addr(mac_addr);
    if (meter_sn < CCTT_METER_NUMBER)
    {
        pNodeInfo->protocol = PLC_meterprotocol_to_3762protocol(g_pMeterRelayInfo[meter_sn].meter_type.protocol);
        pNodeInfo->node_sn = g_pMeterRelayInfo[meter_sn].user_sn;
    }

    if (0 == pRegResult->ProductType)
        pNodeInfo->dev_type = 0x01;
    else
        pNodeInfo->dev_type = 0;

    pNodeInfo->sub_node_count = 0;
    pNodeInfo->trans_sub_node_count = 0;
    P_AFN06_F4_SUB_NODE_INFO pSubNode = (P_AFN06_F4_SUB_NODE_INFO)pNodeInfo->data;
    if (pNodeInfo->dev_type == 0) //采集器，统一存入g_MeterCollReportMap定时上报
    {
    	u16 coll_tei = PLC_GetStaSnByMac(mac_addr);
    	debug_str(DEBUG_LOG_NET, "PLM_ReportSelfReg, collect:%02X%02X%02X%02X%02X%02X, coll tei:%d, meterNum:%d\r\n", 
			      mac_addr[5], mac_addr[4], mac_addr[3], mac_addr[2], mac_addr[1], mac_addr[0], coll_tei, pRegResult->MeterNum);

        for (u8 i = 0; i < pRegResult->MeterNum; i++)
        {
            AddSelfRegCollReportMap(pRegMeter->MeterAddr, pRegMeter->Protocol, pRegResult->MeterNum, coll_tei);
			
            pRegMeter++;
        }

        free_buffer(__func__, pDataUnit);
        return true;
    }

    P_MSG_INFO pSendMsg = LMP_make_frame_3762_cmd(AFN_REPORT_INFO, F4, PLM_PHASE_U, pDataUnit, ((u8 *)pSubNode) - pDataUnit, FALSE, NULL);

    free_buffer(__func__, pDataUnit);

    if (NULL != pSendMsg)
    {
        debug_str(DEBUG_LOG_NET, "上报表号: %d, %02X%02X%02X%02X%02X%02X\r\n", ++rpt_counter, mac_addr[5], mac_addr[4], mac_addr[3], mac_addr[2], mac_addr[1], mac_addr[0]);
		pSendMsg->msg_header.send_delay=true;
		return (bool)End_post(pSendMsg);
    }
    return false;

}

bool PLM_ReportCollectSelfReg(void)
{
	//最多上报采集器下挂8个电表
    u16 alloc_len = 16 + 7 * 8;
    u8 *pDataUnit = alloc_buffer(__func__, alloc_len);

    if (NULL == pDataUnit)
        return false;

	u16 coll_tei = 0;
	FindSelfRegCollReportMapFirstDev(&coll_tei);

	P_STA_INFO pSta = PLC_GetValidStaInfoByTei(coll_tei);
	if (pSta == NULL)
	{
		free_buffer(__func__, pDataUnit);
		return false;
	}
	
	u8 mac_addr[MAC_ADDR_LEN];
    memcpy_special(mac_addr, pSta->mac, MAC_ADDR_LEN);

    P_AFN06_F4_DEV_INFO pDevInfo = (P_AFN06_F4_DEV_INFO)(pDataUnit);

    pDevInfo->node_count = 1;
    P_AFN06_F4_NODE_INFO pNodeInfo = (P_AFN06_F4_NODE_INFO)pDevInfo->data;
#ifdef GDW_JIANGSU
	memcpy_special(pNodeInfo->node_addr, mac_addr, 6);
	pSta = PLC_GetStaInfoByMac(mac_addr);
	if (pSta)
	{
		memcpy_special(pNodeInfo->node_addr, (unsigned char  *)(pSta->chip_id)+12, 12);
	}
#else
    memcpy_special(pNodeInfo->node_addr, mac_addr, 6);
#endif
    pNodeInfo->protocol = METER_PROTOCOL_3762_TRANSPARENT;
    pNodeInfo->node_sn = 0;

    u16 meter_sn = PLC_get_sn_from_addr(mac_addr);
    if (meter_sn < CCTT_METER_NUMBER)
    {
        pNodeInfo->protocol = PLC_meterprotocol_to_3762protocol(g_pMeterRelayInfo[meter_sn].meter_type.protocol);
        pNodeInfo->node_sn = g_pMeterRelayInfo[meter_sn].user_sn;
    }

    pNodeInfo->dev_type = 0;
	pNodeInfo->sub_node_count = 0;
		
    P_AFN06_F4_SUB_NODE_INFO pSubNode = (P_AFN06_F4_SUB_NODE_INFO)pNodeInfo->data;
	u16 total_num = 0, real_num = 0;

	debug_str(DEBUG_LOG_NET, "PLM_ReportCollectSelfReg, collect:%02X%02X%02X%02X%02X%02X, tei:%d\r\n", 
		      mac_addr[5], mac_addr[4], mac_addr[3], mac_addr[2], mac_addr[1], mac_addr[0], coll_tei);
	
    for (int i=0; i<CCTT_METER_NUMBER; i++)
    {			
    	if (!memIsHex(g_SelfRegCollReportMap[i].meter_addr, 0x00, 6))
    	{
    		total_num++;
    	}
		
		if ((g_SelfRegCollReportMap[i].coll_sn == coll_tei)&&(!g_SelfRegCollReportMap[i].report_flag))
		{
			memcpy_special(pSubNode->node_addr, g_SelfRegCollReportMap[i].meter_addr, 6);
        	pSubNode->protocol = g_SelfRegCollReportMap[i].protocol;

			real_num++;
			pSubNode++;

			if (pNodeInfo->sub_node_count == 0)
			{
				pNodeInfo->sub_node_count = g_SelfRegCollReportMap[i].meter_num;
			}
		}
					
		if ((total_num>=selfRegCollReportNum) || (real_num>=8)) //采集器，载波上报最多8个表地址
		{
			break;
		}		
    }

    pNodeInfo->trans_sub_node_count = real_num;

	if (real_num == 0)
	{
		free_buffer(__func__, pDataUnit);
		return false;
	}

	debug_str(DEBUG_LOG_NET, "PLM_ReportCollectSelfReg, meter num:%d, report num:%d, len:%d\r\n", 
	          pNodeInfo->sub_node_count, pNodeInfo->trans_sub_node_count, ((u8 *)pSubNode) - pDataUnit);
	
    P_MSG_INFO pSendMsg = LMP_make_frame_3762_cmd(AFN_REPORT_INFO, F4, PLM_PHASE_U, pDataUnit, ((u8 *)pSubNode) - pDataUnit, FALSE, NULL);

    free_buffer(__func__, pDataUnit);

    if (NULL != pSendMsg)
    {
    	DelSelfRegCollReportMap(real_num, coll_tei);
		
        debug_str(DEBUG_LOG_NET, "上报表号: %d, %02X%02X%02X%02X%02X%02X\r\n", ++rpt_counter, mac_addr[5], mac_addr[4], mac_addr[3], mac_addr[2], mac_addr[1], mac_addr[0]);
		pSendMsg->msg_header.send_delay = true;
		return (bool)End_post(pSendMsg);
    }

    return false;
}

#endif
#ifdef HPLC_CSG
void PLM_ReportSelfRegStop(void)
{
    P_MSG_INFO pSendMsg = LMP_make_frame_gd_cmd(0x05, 0xE8050504, NULL, 0, NULL);

    if(!pSendMsg)
        return;

    End_post(pSendMsg);

    return;
}
#endif
#ifdef GDW_ZHEJIANG
void PLM_ReportCollectSubNodeInfoAndDev(u8* pEventData, u16 len)
{
	u8* pDataUnit = (u8*)alloc_buffer(__func__,len);

	if(NULL == pDataUnit)
		return;
	if(len > 0)
		memcpy_special(pDataUnit, pEventData, len);

	
	P_MSG_INFO pSendMsg = LMP_make_frame_3762_cmd(AFN_REPORT_INFO, F4, PLM_PHASE_U, pDataUnit, len, FALSE, NULL);
	debug_str(DEBUG_LOG_INFO, "即装即采新站点上报至集中器, 事件报文长度=%d\r\n", len);
    free_buffer(__func__, pDataUnit);

	pSendMsg->msg_header.send_delay=true;
	End_post(pSendMsg);
	return;

}
#endif
bool PLM_ReportMeterEvent(EM_EVENT_DEVICE_TYPE device_type, u8 protocol_3762,BOOL isPlcMeterCommModule, u8 meter_addr[FRM_DL645_ADDRESS_LEN], u8* pEventData, u16 nEventLen)
{
#ifdef PROTOCOL_GD_2016

    if(1 != g_PlmStatusPara.report_meter_event)
        return true;

    u16 len = sizeof(RT_REPORT_METER_EVENT_GD)-1 + nEventLen;
    
    P_RT_REPORT_METER_EVENT_GD pDataUnit = (P_RT_REPORT_METER_EVENT_GD)alloc_buffer(__func__,len);

    if(NULL == pDataUnit)
        return false;

    pDataUnit->data_len = nEventLen;
    memcpy_special(&pDataUnit->data[0], pEventData, nEventLen);

    P_MSG_INFO pSendMsg = LMP_make_frame_gd_cmd(0x05, 0xE8050502, (PBYTE)pDataUnit, len, meter_addr);

    free_buffer(__func__,pDataUnit);

    if(!pSendMsg)
		return false;
	pSendMsg->msg_header.send_delay=true;
    return (bool)End_post(pSendMsg);


#else

    u16 len = sizeof(RT_REPORT_METER_EVENT)-1 + nEventLen;

    P_RT_REPORT_METER_EVENT pDataUnit = (P_RT_REPORT_METER_EVENT)alloc_buffer(__func__,len);

    if(NULL == pDataUnit)
        return false;

    pDataUnit->device_type = device_type;
    pDataUnit->protocol = protocol_3762;

    pDataUnit->data_len = nEventLen;
    if(nEventLen > 0)
        memcpy_special(&pDataUnit->data[0], pEventData, nEventLen);

    P_MSG_INFO pSendMsg = LMP_make_frame_3762_cmd(AFN_REPORT_INFO, F5, PLM_PHASE_U, (PBYTE)pDataUnit, len, isPlcMeterCommModule, meter_addr);

    free_buffer(__func__,pDataUnit);

    if(NULL == pSendMsg)
        return false;
	pSendMsg->msg_header.send_delay=true;
    return (bool)End_post(pSendMsg);

#endif
}

bool PLM_ReportAreaDiscernment(u8 meter_addr[FRM_DL645_ADDRESS_LEN], u8 main_node_mac[MAC_ADDR_LEN])
{
#ifdef PROTOCOL_GD_2016
	if (memIsHex(main_node_mac, 0, MAC_ADDR_LEN))
	{
		memset(main_node_mac, 0xff, MAC_ADDR_LEN);
	}

#ifdef PROTOCOL_NW_2017_GUANGZHOU
    u8 Area645Frame[19] = {0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x9E, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x16};
    u16 len = sizeof(RT_REPORT_METER_EVENT_GD) - 1 + sizeof(Area645Frame);
    
    P_RT_REPORT_METER_EVENT_GD pDataUnit = (P_RT_REPORT_METER_EVENT_GD)alloc_buffer(__func__, len);
    if(NULL == pDataUnit)
        return false;

    P_GD_DL645_BODY p645Frame = (P_GD_DL645_BODY)Area645Frame;    
	memcpy(p645Frame->meter_number, meter_addr, 6);
	memcpy(&p645Frame->dataBuf[1], main_node_mac, 6);

    for (int i=0; i<p645Frame->dataLen; i++)
	{
		p645Frame->dataBuf[i] += 0x33;
	}
	p645Frame->dataBuf[p645Frame->dataLen] = GetCheckSum((u8*)p645Frame, p645Frame->dataLen+10);
    
    pDataUnit->data_len = sizeof(Area645Frame);
    memcpy_special(&pDataUnit->data[0], Area645Frame, sizeof(Area645Frame));

    P_MSG_INFO pSendMsg = LMP_make_frame_gd_cmd(0x05, 0xE8050502, (PBYTE)pDataUnit, len, meter_addr);
#else
    P_RT_REPORT_METER_AREA_GD pDataUnit = (P_RT_REPORT_METER_AREA_GD)alloc_buffer(__func__,sizeof(RT_REPORT_METER_AREA_GD));

    if(NULL == pDataUnit)
        return false;

	memset(pDataUnit->res,0,sizeof(pDataUnit->res));
	memcpy_special(&pDataUnit->sta_addr[0], meter_addr, MAC_ADDR_LEN);
	memcpy_special(&pDataUnit->main_addr[0], main_node_mac, MAC_ADDR_LEN);

    pDataUnit->device_type = 1; //默认1，电表
    P_STA_INFO pSta = PLC_GetStaInfoByMac(meter_addr);
    if (NULL != pSta)
    {
        if ((pSta->dev_type == II_COLLECTOR) || (pSta->dev_type == I_COLLECTOR))
        {
            pDataUnit->device_type = 0; //采集器
        }
    }
    
	pDataUnit->meter_num=0;

    P_MSG_INFO pSendMsg = LMP_make_frame_gd_cmd(0x05, 0xE8050580, (PBYTE)pDataUnit, sizeof(RT_REPORT_METER_AREA_GD), meter_addr);
#endif

    free_buffer(__func__,pDataUnit);

    if(!pSendMsg)
        return false;

    return (bool)End_post(pSendMsg);

#else
    P_GD_DL645_BODY pDL645 = (P_GD_DL645_BODY)alloc_buffer(__func__,64);
	u8 protocol_3762 = METER_PROTOCOL_3762_TRANSPARENT;
	P_RELAY_INFO pNode = PLC_GetWhiteNodeInfoByMac(meter_addr);
	
    if(NULL == pDL645)
    {
        goto L_Exit;
    }
	
	if(NULL == pNode)
	{
		goto L_Exit;
	}
	protocol_3762 = PLC_meterprotocol_to_3762protocol(pNode->meter_type.protocol);

    u8 dl645_dataUnit[7];
    dl645_dataUnit[0] = 0x03;
    memcpy_special(&dl645_dataUnit[1], main_node_mac, 6);
    PLC_make_frame_645(pDL645, meter_addr, 0x9E, dl645_dataUnit, sizeof(dl645_dataUnit));

    return PLM_ReportMeterEvent(EVENT_DEVICE_TYPE_METER, protocol_3762, true, meter_addr, (u8*)pDL645, pDL645->dataLen+12);

L_Exit:
    free_buffer(__func__,pDL645);
	return false;
#endif
}

//06 F3 上报路由工况变动信息
void PLM_ReportRouterWorkStatus(u8 router_status)
{
    P_MSG_INFO pMsgIfo;

    if(pMsgIfo = LMP_make_frame_3762_cmd(AFN_REPORT_INFO, F3, PLM_PHASE_U, &router_status, sizeof(router_status), FALSE, NULL))
    {
        End_post(pMsgIfo);
    }
}

#ifdef HPLC_CSG
//回复从节点plc功率信息
void PLM_ResponseStaPower(u8 new_up, u8 node_mac[FRM_DL645_ADDRESS_LEN], u8* pRespData, u16 nRespLen)
{
    P_MSG_INFO pMsgIfo;
	u32 DI;
	
	if(new_up)
		DI = 0xE802F172;
	else
		DI = 0xE802F222;
	
	if(pMsgIfo = LMP_make_frame_gd_cmd(0xF0, DI, pRespData, nRespLen, node_mac))
	{
		pMsgIfo->msg_header.send_delay = false;
		End_post(pMsgIfo);
	}
}

//回复从节点rf功率信息
void PLM_ResponseStaRFPower(u8 new_up, u8 node_mac[FRM_DL645_ADDRESS_LEN], u8* pRespData, u16 nRespLen)
{
    P_MSG_INFO pMsgIfo;
	u32 DI;
	
	if(new_up)
		DI = 0xE802F174;
	else
		DI = 0xE802F230;
	
	if(pMsgIfo = LMP_make_frame_gd_cmd(0xF0, DI, pRespData, nRespLen, node_mac))
	{
		pMsgIfo->msg_header.send_delay = false;
		End_post(pMsgIfo);
	}
}
#else
//回复从节点plc功率信息
void PLM_ResponseStaPower(u8 new_up, u8 node_mac[FRM_DL645_ADDRESS_LEN], u8* pRespData, u16 nRespLen)
{
    P_MSG_INFO pMsgIfo;
	u8 Fn;
	
	if(new_up)
		Fn = F172;
	else
		Fn = F222;
	
	if(pMsgIfo = LMP_make_frame_3762_resp(AFN_FACTORY_RESERVED, Fn, PLM_PHASE_U, pRespData, nRespLen, FALSE, node_mac, 0))
	{
		pMsgIfo->msg_header.send_delay = false;
		End_post(pMsgIfo);
	}
}

//回复从节点rf功率信息
void PLM_ResponseStaRFPower(u8 new_up, u8 node_mac[FRM_DL645_ADDRESS_LEN], u8* pRespData, u16 nRespLen)
{
    P_MSG_INFO pMsgIfo;
	u8 Fn;
	
	if(new_up)
		Fn = F174;
	else
		Fn = F230;
	
	if(pMsgIfo = LMP_make_frame_3762_resp(AFN_FACTORY_RESERVED, Fn, PLM_PHASE_U, pRespData, nRespLen, FALSE, node_mac, 0))
	{
		pMsgIfo->msg_header.send_delay = false;
		End_post(pMsgIfo);
	}
}
#endif


//回复从节点内部版本信息
void PLM_ResponseStaInnerVerInfo(u8 node_mac[FRM_DL645_ADDRESS_LEN], u8* pRespData, u16 nRespLen)
{
    P_MSG_INFO pMsgIfo;

#ifndef HPLC_CSG
    if(pMsgIfo = LMP_make_frame_3762_resp(AFN_FACTORY_RESERVED, F214, PLM_PHASE_U, pRespData, nRespLen, FALSE, node_mac, 0))
#else	
	if(pMsgIfo = LMP_make_frame_gd_cmd(0xF0, 0xE803F0E3, pRespData, nRespLen, node_mac))
#endif
    {
		pMsgIfo->msg_header.send_delay=false;
        End_post(pMsgIfo);
    }
}

void PLM_ResponseStaRegisterInfo(u8 node_mac[FRM_DL645_ADDRESS_LEN], u8* pRespData, u16 nRespLen)
{
    P_MSG_INFO pMsgIfo;

#ifndef HPLC_CSG
    if(pMsgIfo = LMP_make_frame_3762_resp(AFN_FACTORY_RESERVED, F232, PLM_PHASE_U, pRespData, nRespLen, FALSE, node_mac, 0))
#else	
	if(pMsgIfo = LMP_make_frame_gd_cmd(0xF0, 0xE800F0E8, pRespData, nRespLen, node_mac))
#endif
    {
		pMsgIfo->msg_header.send_delay=false;
        End_post(pMsgIfo);
    }
}



void PLM_ResponseStaModuleID(u8* mac, u8 id_type, u8 dev_type, u16 seq, u8* data)
{
	u16 len = sizeof(AFN10_F40_ID_RESPONSE)+11;
        
    P_AFN10_F40_ID_RESPONSE pDataUnit = (P_AFN10_F40_ID_RESPONSE)alloc_buffer(__func__, len);
    if(NULL == pDataUnit)
    {
        return;
    }

    memcpy(pDataUnit->mac, mac, 6);
	pDataUnit->id_type = id_type; //模块ID值为0或者2
	pDataUnit->device_type = dev_type;
	pDataUnit->id_len = 11;
    
	if (data)
	{
	    Moudle_ID_Info *pMoudle_ID_Info = (Moudle_ID_Info*)data;
		memcpy_special(pDataUnit->data, pMoudle_ID_Info->id_data, 11);
	}
	else
	{
		memset_special(pDataUnit->data, 0xFF, 11);
	}
      
    P_MSG_INFO pMsgIfo = LMP_make_frame_3762_resp(AFN_RELAY_QUERY, F40, PLM_PHASE_U, (PBYTE)pDataUnit, len, FALSE, NULL, seq);

    free_buffer(__func__, pDataUnit);

    if(pMsgIfo)
    {
        End_post(pMsgIfo);
    }
}

//14 F1 请求抄表
void PLM_RequestReadItem(P_PLM_READ_ITEM_CMD pReadItemCmd)
{
    P_MSG_INFO pMsgIfo;

    if (pMsgIfo = LMP_make_frame_3762_cmd(AFN_REAME_READ, F1, (PLM_PHASE)pReadItemCmd->plm_phase, (u8 *)pReadItemCmd, sizeof(PLM_READ_ITEM_CMD), false, pReadItemCmd->node_addr))
    {
        End_post(pMsgIfo);
    }
}

//14 F1 请求依通信延时修正通信数据
void PLM_RequestReadItem2FixTime(PLM_PHASE plm_phase,u8* data ,u16 datalen)
{
    P_MSG_INFO pMsgIfo;

	if (pMsgIfo = LMP_make_frame_3762_cmd(AFN_REAME_READ, F3, plm_phase,  data, datalen, false, NULL))
    {
        End_post(pMsgIfo);
    }
}

//AFN14 F2 路由请求集中器时钟
void PLM_RequestCentreTime(void)
{
    P_MSG_INFO pMsgIfo;

	if (pMsgIfo = LMP_make_frame_3762_cmd(AFN_REAME_READ, F2, PLM_PHASE_U, NULL, 0, false, NULL))
    {
        End_post(pMsgIfo);
    }
}

NotWhiteSta_s NotWhiteSta={
	0,0,INVAILD_TMR_ID,
};
bool IsRepeatNotWhiteSta(u8 *mac)
{
    for (int i=0;i<MAX_NOT_WHITE_SIZE;i++)
    {
        if (macCmp(mac, &NotWhiteSta.mac[i][0])==OK)
        {
            return true;
        }
    }
    return false;
}
#ifndef HPLC_CSG
bool addReportNotWhiteSta(u8* mac)
{
	for (int i=0; i<MAX_NOT_WHITE_SIZE; i++)
    {
    	if (memIsHex(&NotWhiteSta.mac[i][0], 0x00, 6))
        {
        	memcpy(&NotWhiteSta.mac[i][0], mac, MAC_ADDR_LEN);
			NotWhiteSta.widx++;
            return true;
        }
    }
	
    return false;
}

void clearRepeatNotWhiteSta(void *arg)
{
    NotWhiteSta.widx=0;
    NotWhiteSta.ridx=0;
    memset(&NotWhiteSta.mac,0,sizeof(NotWhiteSta.mac));
}

//非白名单节点上报
void PLM_ReportNotWhite(void *arg)
{
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
//	OS_ERR err;
    u8* pBuf = (u8*)alloc_buffer(__func__, (6+1)*32);
    int len = 1;
    NotWhiteSta.timerId = INVAILD_TMR_ID;

	if (!g_PlmStatusPara.report_not_white_sta)
    {
        goto L_Exit;
    }
    if (NotWhiteSta.ridx == NotWhiteSta.widx)//buf is empty
    {
        goto L_Exit;
    }
    if (NULL == pBuf)
    {
        goto L_Exit;
    }

	while (NotWhiteSta.ridx != NotWhiteSta.widx)
    {
        u16 ridx = NotWhiteSta.ridx >= MAX_NOT_WHITE_SIZE ? NotWhiteSta.ridx - MAX_NOT_WHITE_SIZE : NotWhiteSta.ridx;
        memcpy(&pBuf[len], &NotWhiteSta.mac[ridx][0], MAC_ADDR_LEN);
        len += 6;
        pBuf[len] = NotWhiteSta.deviceType[ridx];
        len += 1;
        if(++NotWhiteSta.ridx >= 2*MAX_NOT_WHITE_SIZE)
        {
            NotWhiteSta.ridx = 0;
        }
    }
	
    *pBuf = (len-1)/(MAC_ADDR_LEN+1);
    PLM_ReportMeterEvent(EVENT_DEVICE_TYPE_METER, METER_PROTOCOL_3762_NOT_WHITE_STA, false, NULL, pBuf, len);
    
L_Exit:
    free_buffer(__func__, pBuf);
    OS_CRITICAL_EXIT();
//	OSSemPend(&g_pOsRes->savedata_mbox, OSCfg_TickRate_Hz / 2, OS_OPT_PEND_BLOCKING, NULL, &err);
}
#endif
BYTE LMP_DL645_Proc(BYTE end_id, PBYTE pDL645Msg)
{
    u8 res = (!OK);

    P_GD_DL645_BODY pDL645 = (P_GD_DL645_BODY)pDL645Msg;
    BYTE Afn;
    P_LMP_APP_INFO pLmpAppInfo = NULL;
    BYTE p = 4, sn;
    P_GD_DL645_BODY pDL645Resp = NULL;
    P_LMP_LINK_HEAD p3762Resp  = NULL;
    P_MSG_INFO pMsgIfo = NULL;
    PBYTE pLen = NULL;
    PBYTE user_data;

    pLmpAppInfo = PLM_alloc_msg_buf(0);

    if(NULL == pLmpAppInfo)
        return res;

    AddValue((PBYTE)pDL645->dataBuf, pDL645->dataLen, (UCHAR)-0x33);

    pLmpAppInfo->lmpLinkHead = (P_LMP_LINK_HEAD)&pDL645->dataBuf[p];

    pLmpAppInfo->linkFunc = pLmpAppInfo->lmpLinkHead->ctl_field;

    user_data = pLmpAppInfo->lmpLinkHead->user_data;
    if(pLmpAppInfo->lmpLinkHead->Inf.down_info.comm_module)
    {
        user_data += (pLmpAppInfo->lmpLinkHead->Inf.down_info.relay_level * 6 + 12);
    }

    Afn = user_data[0];

    sn = GetOnePosition( user_data[1], 0 );
    if( sn < 8 )
    {
        pLmpAppInfo->curDT = user_data[2] * 8 + sn + 1;
    }
    else
    {
        goto L_Exit;
    }

    pLmpAppInfo->cur_afn_pos = find_AFN((AFN_FUNC_INFO *)_Q_GDW_AFN_PROC, Afn);

    if( _Q_GDW_AFN_PROC[pLmpAppInfo->cur_afn_pos].diFunc == NULL )
    {
        goto L_Exit;
    }

    pLmpAppInfo->responsePoint = 0;
    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x02;
    pLen = &pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];

    pDL645Resp = (P_GD_DL645_BODY)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
    memcpy_special(pDL645Resp, pDL645, pDL645->dataLen+12);
    pLmpAppInfo->responsePoint += (OffsetOf(GD_DL645_BODY, dataBuf) + 4);
    p3762Resp = (P_LMP_LINK_HEAD)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
    
	#if defined(HRF_SUPPORT) && !defined(COMM_MODE_OLD)
	p3762Resp->ctl_field = 0x84;
	#else
	p3762Resp->ctl_field = 0x83;
	#endif

    if(pLmpAppInfo->lmpLinkHead->Inf.down_info.comm_module)
    {
        //源地址，目标地址互换
        memcpy_special(&p3762Resp->user_data[0], &pLmpAppInfo->lmpLinkHead->user_data[6+pLmpAppInfo->lmpLinkHead->Inf.down_info.relay_level*6], 6);
        memcpy_special(&p3762Resp->user_data[6], &pLmpAppInfo->lmpLinkHead->user_data[0], 6);
        pLmpAppInfo->responsePoint += (OffsetOf(LMP_LINK_HEAD, user_data) + 3 + 12);
    }
    else
    {
        pLmpAppInfo->responsePoint += (OffsetOf(LMP_LINK_HEAD, user_data) + 3);
    }

    _Q_GDW_AFN_PROC[pLmpAppInfo->cur_afn_pos].diFunc( pLmpAppInfo );

    //应答帧不加校验和结束标识，以免集中器把645里面的内容当作3762解释
    //pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Get_checksum((PBYTE)&p3762Resp->ctl_field, p3762Resp->frm_len-5);
    //pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x16;

    //3762的长度
    p3762Resp->frm_len = pLmpAppInfo->responsePoint;

    pDL645Resp->controlCode |= 0x80;
    pDL645Resp->dataLen = (BYTE)(pLmpAppInfo->responsePoint - 12);
    AddValue((PBYTE)pDL645Resp->dataBuf, pDL645Resp->dataLen, 0x33);
    pDL645Resp->dataBuf[pDL645Resp->dataLen] = Get_checksum((PBYTE)pDL645Resp, pDL645Resp->dataLen+10);
    pDL645Resp->dataBuf[pDL645Resp->dataLen+1] = 0x16;

    *pLen = pDL645Resp->dataLen + 12;
    pLmpAppInfo->responsePoint += 2; //DL645校验和0x16

    pMsgIfo = LMP_make_frame_3762_resp(AFN_FRAME_FORWARD, F1, PLM_PHASE_U, pLmpAppInfo->msg_buffer, pLmpAppInfo->responsePoint, TRUE, pDL645Resp->meter_number, g_PlmStatusPara.plm_seq_no_down);

    if(pMsgIfo)
    {
        pMsgIfo->msg_header.end_id = end_id;
        pMsgIfo->msg_header.need_buffer_free = TRUE;
		pMsgIfo->msg_header.send_delay = false;
        End_post(pMsgIfo); //End_send(pMsgIfo);
    }

L_Exit:
    PLM_free_msg_buf(pLmpAppInfo);
    return res;
}

P_LMP_LINK_HEAD PLM_Check_Frame_3762(PBYTE pdata, int datalen)
{
    int i = 0;
    int len = datalen-sizeof(LMP_LINK_HEAD)+1;
    P_LMP_LINK_HEAD pFrame = NULL;

    for(i=0; i<len; i++)
    {
        int leftLen = datalen-i;

        pFrame = (P_LMP_LINK_HEAD)&pdata[i];

        if(0x68 != pFrame->firstStartByte)
            continue;

        if((pFrame->frm_len > leftLen) || (pFrame->frm_len < 5))
            continue; //break;

        if(0x16 != ((PBYTE)pFrame)[pFrame->frm_len-1])
            continue;

        if(((PBYTE)pFrame)[pFrame->frm_len-2] == Get_checksum(&pFrame->ctl_field, pFrame->frm_len-5))
            return pFrame;
    }

    return NULL;
}

P_LMP_LINK_HEAD_WIRELESS PLM_Check_Frame_Wireless(PBYTE pdata, int datalen)
{
    int i = 0;
    int len = datalen-sizeof(LMP_LINK_HEAD_WIRELESS)+1;
    P_LMP_LINK_HEAD_WIRELESS pFrame = NULL;

    if(datalen >= 1061)
        i = 2;

    for(i=0; i<len; i++)
    {
        int leftLen = datalen-i;

        pFrame = (P_LMP_LINK_HEAD_WIRELESS)&pdata[i];

        if(0xfd != pFrame->firstStartByte)
            continue;

        if((pFrame->user_len+12) > leftLen)
            continue; //break;

        if(*(USHORT*)(&((PBYTE)pFrame)[pFrame->user_len+10]) == PLC_Checksum(&pFrame->firstStartByte, pFrame->user_len+10))
            return pFrame;
    }

    return NULL;
}


u8 PLM_process_frame(u8 end_id, P_LMP_LINK_HEAD pFrame)
{
    u8 res = (!OK);
    
    P_LMP_APP_INFO pLmpAppInfo = NULL;

    pLmpAppInfo = PLM_alloc_msg_buf(pFrame->frm_len);

    if(NULL == pLmpAppInfo)
        return res;

    pLmpAppInfo->end_id = end_id;

    //获取控制域功能码
    pLmpAppInfo->linkFunc = pFrame->ctl_field;

    // 应用层长度
    pLmpAppInfo->appLayerLen = pFrame->frm_len - 6;  //减帧头帧尾

    memcpy_special(pLmpAppInfo->rxMsgBuf, (PBYTE)pFrame, pFrame->frm_len);

    pLmpAppInfo->lmpLinkHead = (P_LMP_LINK_HEAD)(pLmpAppInfo->rxMsgBuf);
    pLmpAppInfo->lmpLinkHeadGD = (P_LMP_LINK_HEAD_GD)(pLmpAppInfo->rxMsgBuf);

    //国网宽带台体, 错误控制字0x03包括命令 : AFN03 F7, AFN13 F1
    //测试台体错误帧，AFN03 F7      68 0F 00 03 00 00 00 00 00 01 03 40 00 47 16

    BOOL is_cmd = (BOOL)(pLmpAppInfo->linkFunc&LMP_LINK_CTRL_PRM);

#ifdef GDW_2012
//    if(!is_cmd)
    {
        u16 p_afn = 0;
        if(pLmpAppInfo->lmpLinkHead->Inf.down_info.comm_module)
            p_afn += (12 + 6*pLmpAppInfo->lmpLinkHead->Inf.down_info.relay_level);

        u8 AFN = pLmpAppInfo->lmpLinkHead->user_data[p_afn];
        u8 Fn = PLM_Get_Fn(pLmpAppInfo->lmpLinkHead->user_data[p_afn+1], pLmpAppInfo->lmpLinkHead->user_data[p_afn+2]);

        if(AFN_REAME_READ!=AFN && AFN_RESPONSE!=AFN)
            is_cmd = TRUE;
		if (AFN_REAME_READ==AFN)
		{
			is_cmd = false;
		}
    }
#endif

    if(is_cmd)
    {
        //报文来自'启动站'，一般是主站任务报文

#if defined(PROTOCOL_GD_2016)
#ifdef PROTOCOL_NW_2021	
		FlashReceiveLed();
#endif
#ifndef COMPATIBLE_GDW_3762_INTERFACE
        res = LMP_App_Proc_gd(pLmpAppInfo);
#else
        if(pLmpAppInfo->lmpLinkHead->ctl_field&0x03)
            res = LMP_App_Proc(pLmpAppInfo);
        else
            res = LMP_App_Proc_gd(pLmpAppInfo);
#endif
#else
        res = LMP_App_Proc(pLmpAppInfo);
#endif
    }
    else
    {
        //报文来自'从动站'，这里只有主站对登陆报文的响应

#if defined(PROTOCOL_GD_2016)
        res = LMP_Response_Proc_gd(pLmpAppInfo);
#else
        res = LMP_Response_Proc(pLmpAppInfo);
#endif
    }

//L_Exit:

    PLM_free_msg_buf(pLmpAppInfo);
    return res;
}

BYTE PLM_start_scan(BYTE scan_time)
{
    int rand_time;
    g_PlmStatusPara.scan_ack_flag = FALSE;

    do
    {
        rand_time = rand_special()%scan_time;
    } while(rand_time==0);

    g_PlmStatusPara.scan_time_couter = (BYTE)rand_time;

    return OK;
}

BYTE PLM_get_dev_addr_wireless(PBYTE pAddr)
{
    memcpy_special(pAddr, g_PlmConfigPara.main_node_addr, 6);

    return OK;
}

#ifdef APP_WIRELESS_EN
P_MSG_INFO LMP_make_frame_wireless(BYTE ctrl, PBYTE pDataUnit, USHORT nDataLen)
{
    P_MSG_INFO pMsg = NULL;
    P_LMP_LINK_HEAD_WIRELESS pFrameWireless;

    if( !(pMsg = (P_MSG_INFO)alloc_send_buffer(__func__,MSG_LONG)) )
    {
        //Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);
        return NULL;
    }

    pFrameWireless = (P_LMP_LINK_HEAD_WIRELESS)&pMsg->msg_buffer[0];
    pFrameWireless->firstStartByte = 0xfd;
    PLM_get_dev_addr_wireless(pFrameWireless->addr);
    pFrameWireless->ctl_field = 0x80|ctrl;
    pFrameWireless->user_len = nDataLen;

    if((nDataLen > 0) && (NULL != pDataUnit))
    {
        memcpy_special(pFrameWireless->user_data, pDataUnit, nDataLen);
    }
    *(USHORT*)&((PBYTE)pFrameWireless)[pFrameWireless->user_len+10] = PLC_Checksum(&pFrameWireless->firstStartByte, pFrameWireless->user_len+10);

    pMsg->msg_header.msg_len = pFrameWireless->user_len + 12;

    pMsg->msg_header.end_id = COM_PORT_WIRELESS;
    pMsg->msg_header.need_buffer_free = TRUE;
	pMsgIfo->msg_header.send_delay = false;
    return pMsg;
}
#endif

u8 PLM_process_frame_wireless(u8 end_id, P_LMP_LINK_HEAD_WIRELESS pFrameWireless)
{
    u8 res = ERROR;
    u8 wireless_addr[6];

    PLM_get_dev_addr_wireless(wireless_addr);

    if(0x01==pFrameWireless->ctl_field)
    {
        //新扫描;
        PLM_start_scan(pFrameWireless->user_data[0]);
        return OK;
    }
    else if(0x02==pFrameWireless->ctl_field)
    {
        //补扫描
        if(FALSE == g_PlmStatusPara.scan_ack_flag)
        {
            PLM_start_scan(pFrameWireless->user_data[0]);
        }
        return OK;
    }
    else if(0x03==pFrameWireless->ctl_field)
    {
        BYTE i, count;

        count = (BYTE)(pFrameWireless->user_len / 6);

        for(i=0; i<count; i++)
        {
            if(OK == macCmp(wireless_addr, &pFrameWireless->user_data[i*6]))
            {
                g_PlmStatusPara.scan_ack_flag = TRUE;
                break;
            }
        }
        return OK;
    }
    else
    {
        P_LMP_LINK_HEAD pFrame = NULL;

        if(pFrame=PLM_Check_Frame_3762(pFrameWireless->user_data, pFrameWireless->user_len))
        {
            if(OK == macCmp(wireless_addr, &pFrameWireless->addr[0]))
            {
                PLM_process_frame(end_id, pFrame);
            }
            res = OK;
        }
    }

    return res;
}
static u32 uart_timer;
static u8 uart_frame_done=0;
extern u32 s_baudRate;
unsigned short PLM_2005_postprocess_GDW(unsigned char end_id,  pvoid h)
{
    P_LONG_MSG_INFO     pMsg = (P_LONG_MSG_INFO)h;
    //P_LMP_LINK_HEAD_WIRELESS pFrameWireless = NULL;
    P_LMP_LINK_HEAD pFrame = NULL;
    int leftLen = 0;
    PBYTE p, p0;
    USHORT res = ERROR;
    OS_ERR err;
    PBYTE   pComPortBuf = GetComPortBufPtr(end_id, 0);
    USHORT *pComPortLen = GetComPortLenPtr(end_id, 0);
    USHORT nMsgLen = pMsg->msg_header.msg_len;
    USHORT nTotalLen;

    if (pMsg->msg_header.msg_len > (COM_PORT_BUFFER_SIZE))
        return ERROR;

    u32 basetime = OSTimeGet(&err) * (1000 / OSCfg_TickRate_Hz);
    if (pComPortBuf && pComPortLen)
    {
		debug_str(DEBUG_LOG_INFO, "3762 last %d\r\n",*pComPortLen);
        //把刚接收到剩余的数据放入缓冲区
        if (!uart_frame_done)
        {
            uart_timer = basetime - uart_timer;
            u32 tanf_time = pMsg->msg_header.msg_len * 11 * 1000 / s_baudRate;
            tanf_time = 5 * tanf_time + 600;
            if (uart_timer > tanf_time)
            {
                *pComPortLen = 0;
            }
        }

        uart_frame_done = 0;
        if (nMsgLen > 0)
        {
            if ((*pComPortLen + nMsgLen) > COM_PORT_BUFFER_SIZE)
            {
                *pComPortLen = 0;
            }
            memcpy_special(&pComPortBuf[*pComPortLen], &pMsg->msg_buffer[0], nMsgLen);
            (*pComPortLen) += nMsgLen;
        }
    }
    else
    {
        pComPortBuf = &pMsg->msg_buffer[0];
        pComPortLen = &nMsgLen;
    }
    uart_timer = basetime;
    nTotalLen = *pComPortLen;

    //3762协议的处理
    leftLen = nTotalLen;
    p0 = pComPortBuf;
    p = p0;
    while ((leftLen > 0) && (pFrame = PLM_Check_Frame_3762(p, leftLen)))
    {
#ifdef GDW_2012
        if (0x40 & pFrame->ctl_field)
            g_PlmStatusPara.plm_seq_no_down = pFrame->Inf.down_info.seq_no;
#endif
        debug_str(DEBUG_LOG_3762, "get3762:");
        debug_hex(DEBUG_LOG_3762, (u8 *)pFrame, pFrame->frm_len);
        //帧处理
        PLM_process_frame(end_id, pFrame);
        //处理剩下的报文
        p = ((PBYTE)pFrame) + (pFrame->frm_len);
        leftLen = (int)nTotalLen - (int)(p - p0);
        uart_frame_done ++;
        res = OK;
		Feed_WDG();
    }

    if (uart_frame_done)
    {
        debug_str(DEBUG_LOG_INFO, "new 3762 cnt:%d\r\n",uart_frame_done);
    }
    else
    {
        debug_str(DEBUG_LOG_INFO, "incomplete 3762\r\n");
    }
    if (leftLen > 0)
    {
        if (*pComPortLen > leftLen)
            memmove_special(&pComPortBuf[0], &pComPortBuf[*pComPortLen - leftLen], leftLen);
    }
    *pComPortLen = leftLen;

    return res;
}

#ifdef APP_WIRELESS_EN
void PLM_send_debug_frame_wl(P_MSG_INFO pMsgSrc)
{
    P_MSG_INFO pMsgIfo;
    pMsgIfo = alloc_send_buffer(__func__,MSG_SHORT);
    if(pMsgIfo)
    {
        pMsgIfo->msg_header.end_id = COM_PORT_WIRELESS;
        pMsgIfo->msg_header.need_buffer_free = TRUE;
		pMsgIfo->msg_header.send_delay = false;
        pMsgIfo->msg_header.msg_len = pMsgSrc->msg_header.msg_len;
        if(pMsgIfo->msg_header.msg_len > PLC_BUFFER_UART_SIZE)
            pMsgIfo->msg_header.msg_len = PLC_BUFFER_UART_SIZE;

        memcpy_special(pMsgIfo->msg_buffer, pMsgSrc->msg_buffer, pMsgIfo->msg_header.msg_len);

        End_post(pMsgIfo);     //End_send(pMsgIfo);
    }
}
#endif

void PLM_Task_Proc(void *p_arg)
{
    OS_ERR err;
    OS_MSG_SIZE msgSize;
    P_TMSG pMsg;

    OSTimeDly(OSCfg_TickRate_Hz*2/3, OS_OPT_TIME_DLY, &err);

    while(1)
    {
        pMsg = OSTaskQPend(OSCfg_TickRate_Hz/40, OS_OPT_PEND_BLOCKING, &msgSize, NULL, &err);
        
        if(OS_ERR_NONE != err)
        {
            free_msg(__func__,pMsg);
            continue;
        }
        if(NULL == pMsg)
            continue;

        if(MSG_PLM_FRAME == pMsg->message)
        {
            P_MSG_INFO pSendMsg = (P_MSG_INFO)pMsg->lParam;

        #ifdef DEBUG_FRMAE_3762
            debug_hex(true, (u8*)pSendMsg->msg_buffer, pSendMsg->msg_header.msg_len);
        #endif

            PLM_2005_postprocess(pSendMsg->msg_header.end_id, pSendMsg);
            free_send_buffer(__func__,pSendMsg);
        }
        else if(MSG_PLM_SET_BAUDRATE == pMsg->message)
        {
            BSP_OS_TimeDly(30);      //等待应答再切换波特率
			if(pMsg->wParam>>8)//最高16位存储的是 需要变更的波特率
			{
				End_SetBaudrate(pMsg->wParam>>8, Parity_Even);
			}
			else//保留原来的设置方法
			{
				if(2 == pMsg->wParam)
					End_SetBaudrate(520000, Parity_Even);
				else if(1 == pMsg->wParam)
					End_SetBaudrate(115200, Parity_Even);
				else
					End_SetBaudrate(9600, Parity_Even);
			}
        }
        else if(MSG_PARAM_CLEAR == pMsg->message)
        {
            SYS_STOR_ParamClear();
        }
        else if(MSG_DATA_CLEAR == pMsg->message)
        {
            SYS_STOR_DataClear();
        }
        
        free_msg(__func__,pMsg);
    }
}

unsigned short PLM_2005_postprocess(unsigned char end_id,  pvoid h)
{
    unsigned short res = ERROR;

    //return res;

#ifdef APP_WIRELESS_EN
#ifdef APP_PRINT_CONCENTRATOR_FRAME_WL
    if(COM_PORT_WIRELESS != end_id)
    {
        PLM_SEND_DEBUG_FRAME_WL((P_MSG_INFO)h);
    }
#endif
#endif

    res = PLM_2005_postprocess_GDW(end_id, h);

//#ifdef APP_WIRELESS_EN
#if 0
    if(OK != res)
    {
        P_MSG_INFO pNewMsg;
		u8 result=RT_ERR_INVALID_CRC;
		pNewMsg=LMP_make_frame_3762_resp(AFN_RESPONSE, F2, PLM_PHASE_U, (u8*)&result, 4, FALSE, NULL, g_PlmStatusPara.plm_seq_no_down);
		if (NULL!=pNewMsg)
		{
			End_post(pNewMsg);
		}
    }
#endif

    return res;
}

/**************************************************************************************

函数描述:   集中器国电规约初始化入口

输入参数:   回程数据结果.

**************************************************************************************/
unsigned short PLM_init(HANDLE h)
{
    return TRUE;
}

/**************************************************************************************

函数描述:   集中器国电规约初始化入口

输入参数:   回程数据结果.

**************************************************************************************/
unsigned short PLM_proc(HANDLE h)
{
    return 0;
}

/******************************************************************************************************

函数说明:  这个event_proc 逐个进行数据解析与处理，将所有Fn Pn 逐个处理完毕!!

******************************************************************************************************/
USHORT on_lmp_proc(unsigned char enter_mode, HANDLE h)
{
    USHORT res;

    P_LMP_APP_INFO pLmpAppInfo = (P_LMP_APP_INFO)h;


    pLmpAppInfo->responsePoint = OffsetOf(LMP_LINK_HEAD, user_data) + 3;

    if(pLmpAppInfo->lmpLinkHead->Inf.down_info.comm_module)
    {
        //pLmpAppInfo->responsePoint += (pLmpAppInfo->lmpLinkHead->Inf.down_info.relay_level * 6) + 12;
        pLmpAppInfo->responsePoint += 12;   //上行没有中继地址
    }

    memcpy_special(pLmpAppInfo->msg_buffer, pLmpAppInfo->rxMsgBuf, pLmpAppInfo->lmpLinkHead->frm_len);

    if( _Q_GDW_AFN_PROC[pLmpAppInfo->cur_afn_pos].diFunc != NULL )
    {
        res = _Q_GDW_AFN_PROC[pLmpAppInfo->cur_afn_pos].diFunc( pLmpAppInfo );
    }
    else
    {
        return ERROR;
    }

    //OSTimeDly(OS_TICKS_PER_SEC);
    //OSTimeDly(30);

    if( OK != res )
    {
        UCHAR err_type;

        err_type = pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint-1];

        LMP_SendNakResponse(pLmpAppInfo, err_type);
    }
    else
    {
		pLmpAppInfo->curProcUnit =  NULL;

        if( (pLmpAppInfo->confirmFlag == 1) || (pLmpAppInfo->confirmFlag == 9))
        {
            LMP_SendConfirmResponse(pLmpAppInfo, res);
        }
        else if( pLmpAppInfo->confirmFlag == 0)
        {
            LMP_SendQueryResponse(pLmpAppInfo);
        }
    }
    //}

    return OK;

}


/* fine item */
unsigned char find_AFN(AFN_FUNC_INFO* pItemArray, unsigned char AFN_code)
{
    //P_PLM_ITEM_ARRAY pParaInfo;
    unsigned short i = 0;

    while(pItemArray->diFunc != NULL)
    {
        if(pItemArray->AFN == AFN_code)
        {
            return i;
        }
        pItemArray++;
        i++;
    }

    return i;
}

/* PLM: Power load management system*/

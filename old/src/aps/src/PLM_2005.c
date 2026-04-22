#include "gsmcu_hal.h"
#include "hplc_data.h"
#include "plm_2005.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "local_protocol.h"
#define memset_special memset
#define memcpy_special memcpy
#define OffsetOf(type, f) ((u16)((char *)&((type *)0)->f - (char *)(type *)0))
char * LMP_make_frame_3762_cmd(u8 Afn, u8 Fn, u8 seq,u8* pDataUnit, u16 *nDataLen, u8* pSrcAddr,u8 isAck)
{
    P_LMP_LINK_HEAD pFrame;
    u8* p;


    pFrame =(P_LMP_LINK_HEAD)malloc(256);

    p = pFrame->user_data;

    memset_special((u8*)pFrame, 0, sizeof(LMP_LINK_HEAD));

    pFrame->firstStartByte = 0x68;
	if (isAck)
	{
		pFrame->ctl_field = 0x43;
	}
	else
	{
		pFrame->ctl_field = 0x43;
	}

//    if(((AFN_DATA_TRANS == Afn) && (F5 == Fn))
//       ||((AFN_RESET == Afn) && (F7 == Fn)))
//    {
//        pFrame->ctl_field = 0xc7;
//    }

    if(pSrcAddr)  //对载波表的通信模块操作
    {
        //u16 metersn;
        pFrame->Inf.up_info.comm_module = 1;
        //源地址A1
        memset(p, 0, 6);
        p += 6;
        //中继地址A2
        pFrame->Inf.up_info.relay_level = 0;
        //目的地址A3
        memcpy(p, pSrcAddr, 6);
        p += 6;
    }

    pFrame->Inf.up_info.seq_no = seq;


    pFrame->Inf.up_info.channel_phase = 0;

    pFrame->Inf.up_info.channel_flag = pFrame->Inf.up_info.channel_phase;

    *p++ = Afn;
    *p++ = (BYTE)(1<<((Fn-1)&0x07));    //DT1
    *p++ = (BYTE)((Fn-1)>>3);        //DT2
    pFrame->frm_len = 6
                      + sizeof(pFrame->Inf)
                      + (pSrcAddr?12:0)
                      + 3
                      + *nDataLen;

    if(*nDataLen)
    {
        memcpy_special(p, pDataUnit, *nDataLen);
        p += *nDataLen; //数据单元
    }

    *((u8*)pFrame + pFrame->frm_len - 2) = GetCheckSum((u8*)&pFrame->ctl_field, pFrame->frm_len-5);
    *((u8*)pFrame + pFrame->frm_len - 1) = 0x16;

    *nDataLen=pFrame->frm_len;
    return (char*)pFrame;
}
char* LMP_make_frame_gd_cmd(u8 Afn, u32 DI,u8 seq, u8* pDataUnit, u16* nDataLen, u8* pDstAddr,u8 isAck)
{
    u16 p;
    P_LMP_LINK_HEAD_GD pFrame_gd;

    if( !(pFrame_gd = malloc(256)) )
    {
        return NULL;
    }

    p = 0;

    pFrame_gd->firstStartByte = FRAME_PLM2005_START_FLAG;

    if(pDstAddr)
    {
		if (isAck)
		{
			pFrame_gd->ctl_field = 0x20;
		}
		else
		{
			pFrame_gd->ctl_field = 0xE0;
		}
        memset(&pFrame_gd->user_data[p], 0, 6);
		const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();


		memcpy(&pFrame_gd->user_data[p], pNetBasePara->Mac, LONG_ADDR_LEN);
		TransposeAddr(&pFrame_gd->user_data[p]);

		p += 6;
		memcpy(&pFrame_gd->user_data[p], pDstAddr, 6);
        p += 6;
    }
    else
    {
		if (isAck)
		{
			pFrame_gd->ctl_field = 0x80;
		}
		else
		{
			pFrame_gd->ctl_field = 0xc0;
		}
    }

    pFrame_gd->user_data[p++] = Afn;       //AFN

    pFrame_gd->user_data[p++] = seq;       //SEQ

    *(u32*)&pFrame_gd->user_data[p] = DI;
    p += 4;

    if((NULL!=pDataUnit) && (*nDataLen>0))
    {
        memcpy(&pFrame_gd->user_data[p], pDataUnit, *nDataLen);
        p += *nDataLen;
    }

    pFrame_gd->user_data[p] = GetCheckSum((u8*)&pFrame_gd->ctl_field, p+1);
    p++;
    pFrame_gd->user_data[p++] = FRAME_PLM2005_END_FLAG;

    pFrame_gd->frm_len = 4+p;
    *nDataLen=pFrame_gd->frm_len;
    return (char*)pFrame_gd;

}
P_LMP_LINK_HEAD PLM_Check_Frame_3762(u8* pdata, int datalen)
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

        if(pFrame->frm_len > leftLen)
            continue; //break;

        if(0x16 != ((u8*)pFrame)[pFrame->frm_len-1])
            continue;

        if(((u8*)pFrame)[pFrame->frm_len-2] == GetCheckSum(&pFrame->ctl_field, pFrame->frm_len-5))
            return pFrame;
    }

    return NULL;
}
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
//提取国网帧
unsigned short LMP_App_Proc(P_LMP_APP_INFO pLmpAppInfo)
{

    //unsigned char *sendBuffer = pLmpAppInfo->msg_buffer; //g_LmpProtocolInfo.responseBuffer;

    unsigned char afn_code, *user_data, sn;
	pLmpAppInfo->appLayerLen = pLmpAppInfo->lmpLinkHead->frm_len;
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

    pLmpAppInfo->cur_afn = afn_code;


	/* 取得第一个' 数据单元标识 ' , 用'curProcUnit'  指向它 */
    pLmpAppInfo->curProcUnit = (P_LMP_DATA_UNIT)(user_data+1);
    pLmpAppInfo->seq = pLmpAppInfo->lmpLinkHead->Inf.up_info.seq_no;
    sn = GetOnePosition( pLmpAppInfo->curProcUnit->DT1, 0 );
    pLmpAppInfo->appLayerLen -= 3;//减3个字节的an fn
    if( sn < 8 )
    {
        /* 计算得到当前' Fn '  号 */
        pLmpAppInfo->curDT = pLmpAppInfo->curProcUnit->DT2 * 8 + sn + 1;
    }
    else
    {
        return ERROR;
    }
    return SUCCESS;


}

//提取南网帧
unsigned short LMP_App_Proc_gd(P_LMP_APP_INFO pLmpAppInfo)
{
    unsigned char afn_code, *user_data;

    user_data = pLmpAppInfo->lmpLinkHeadGD->user_data;
	pLmpAppInfo->appLayerLen = pLmpAppInfo->lmpLinkHeadGD->frm_len;
    if(pLmpAppInfo->lmpLinkHeadGD->ctl_field&0x20)
    {
        user_data += 12;
        pLmpAppInfo->appLayerLen -= 12;
    }

    afn_code = user_data[0];

    pLmpAppInfo->cur_afn = afn_code;

    pLmpAppInfo->seq = user_data[1];
    pLmpAppInfo->curDT = *(u32*)&user_data[2];
    pLmpAppInfo->curProcUnit = (P_LMP_DATA_UNIT)&user_data[4];
    pLmpAppInfo->appLayerLen -= 6;//功能码 数据单元标识 序列号

    pLmpAppInfo->confirmFlag = 0;

    return SUCCESS;
}

LMP_APP_INFO PLM_process_frame(P_LMP_LINK_HEAD pFrame)
{

	LMP_APP_INFO temp;
    P_LMP_APP_INFO pLmpAppInfo = &temp;


    //获取控制域功能码
    pLmpAppInfo->linkFunc = pFrame->ctl_field;

    // 应用层长度
    pLmpAppInfo->appLayerLen = pFrame->frm_len - 6;  //减帧头帧尾


    pLmpAppInfo->lmpLinkHead = pFrame;
    pLmpAppInfo->lmpLinkHeadGD = (P_LMP_LINK_HEAD_GD)(pFrame);


	LMP_App_Proc_gd(pLmpAppInfo);

    return temp;
}

#ifndef _PROTOCOL_69845_H_
#define _PROTOCOL_69845_H_

#include "Type_define.h"

#define PPPINITFCS16 0xffff /* Initial FCS value */
#define PPPGOODFCS16 0xf0b8 /* Good final FCS value */

#pragma pack(push,1)


typedef struct
{
    unsigned char SA_len:4;       //地址域服务器地址的字节数
    unsigned char reserved:2;
    unsigned char SA_type:2;      //地址域服务器地址的类别标志
}DL69845_AF, P_DL69845_AF;

typedef union 
{
    unsigned char   other_c[1];
    struct 
    {
        unsigned char SA[6];
        unsigned char CA;
        unsigned short HCS;
        unsigned char userdata[1];
	} other_s;

}DL69845_OTHER;

typedef struct
{
    unsigned char   protocolHead;
    unsigned short  len;
    unsigned char   ctrl;
    DL69845_AF      AF;     //地址标识
    DL69845_OTHER   other;
    //unsigned char   other[1];
    //服务器地址 SA
    //客户机地址 CA
    //帧头校验 HCS
    //链路用户数据
    //帧校验 FCS
    //结束字符（16H）
}DL69845_BODY, *P_DL69845_BODY;


#pragma pack(pop)

P_DL69845_BODY PLC_check_69845_frame(UCHAR* buf, USHORT nLen);
UCHAR* PLC_get_69845_SAMac(P_DL69845_BODY pFrame, UCHAR* saMac);
USHORT PLC_make_frame_698_event_frame(UCHAR* data, UCHAR addr[6]);

#endif

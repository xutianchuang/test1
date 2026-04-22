#ifndef _PROTOCOL_645_H_
#define _PROTOCOL_645_H_

#define SEND_DL645_DATA_BUF_SIZE    200     //40

#pragma pack(push,1)

// DL645 规约协议体定义
typedef struct _gd_dl645_body_
{
	unsigned char protocolHead;   
	unsigned char meter_number[6];			
	unsigned char protocolHead2;  
	unsigned char controlCode;
	
	unsigned char dataLen;
	unsigned char dataBuf[SEND_DL645_DATA_BUF_SIZE];
}GD_DL645_BODY, *P_GD_DL645_BODY;

#pragma pack(pop)

void PLC_make_frame_645(P_GD_DL645_BODY pFrame, PBYTE pMeterNo, BYTE ctrl, PVOID pDataUnit, BYTE nDataLen);
void PLC_make_frame_645_exception(P_GD_DL645_BODY pFrame, const P_GD_DL645_BODY pCmdDL645);
P_GD_DL645_BODY PLC_check_dl645_frame(UCHAR* buf, USHORT nLen); //检查645帧是否合法

#endif

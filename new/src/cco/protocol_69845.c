
#include "System_inc.h"
#include "protocol_69845.h"
//#include "Type_define.h"

P_DL69845_BODY PLC_check_69845_frame(UCHAR* buf, USHORT nLen)
{
    P_DL69845_BODY pFrame = NULL;
    
    USHORT i, j, fcs, data_len;
    BYTE sa_len;
    PBYTE p;

    for(i=0; i<nLen-10; i++)
    {
        p = &buf[i];
        pFrame = (P_DL69845_BODY)p;

        if(0x68 != pFrame->protocolHead)
            continue;

		if(pFrame->len & (1<<14)) //单位千字节
		{
			data_len = (pFrame->len & 0x3FFF)*1000;
			if(data_len > 2000)
            	continue;
		}
		else
		{
			data_len = pFrame->len & 0x3FFF;
		}

        if(data_len > (nLen-i-2))
            continue;

        if(0x16 != (*(p+data_len+1)))
            continue;

        sa_len = pFrame->AF.SA_len + 1;
        j = sa_len;
		j += 1;
#if 0 //1字节扩展逻辑地址已经包含在地址长度中
		if(pFrame->AF.reserved & (1<<1)) //有1字节的扩展逻辑地址
		{
			j += 1;
		}
#endif		
		
        fcs = pppfcs16(PPPINITFCS16, p+1, &pFrame->other.other_c[j]-(p+1)+2);
        if(fcs != PPPGOODFCS16)
            continue;

        fcs = pppfcs16(PPPINITFCS16, p+1, data_len);
        if(fcs != PPPGOODFCS16)
            continue;

        return pFrame;
    }

    return NULL;
}

//pFrame确定为698.45报文时，才会调用该地址解析函数
UCHAR* PLC_get_69845_SAMac(P_DL69845_BODY pFrame, UCHAR* saMac)
{
	if (pFrame->AF.reserved & (1 << 1) && pFrame->AF.SA_len==6) //有1字节的扩展逻辑地址
	{
		if(saMac) //saMac不为空，直接赋值给saMac
		{
			memcpy_special(saMac, &pFrame->other.other_c[1], MAC_ADDR_LEN);
			return NULL;
		}
		else //saMac为空，直接返回地址对应的指针
		{
			return (UCHAR*)(&pFrame->other.other_c[1]);
		}
	}
	else
	{
		if(saMac)
		{
			memcpy_special(saMac, &pFrame->other.other_c[0], MAC_ADDR_LEN);
			return NULL;
		}
		else
		{
			return (UCHAR*)(&pFrame->other.other_c[0]);
		}
	}
}

UCHAR dummyEvent[]={0x68,0x18,0x00,0x00,0x19,0x67,0x37,
                 0x68,0x91,0x12,0x34,0x48,0x33,0x37,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0xDD,0xDD,0x46,0x16};

//组698事件上报帧
USHORT PLC_make_frame_698_event_frame(UCHAR* data, UCHAR addr[6])
{
	memcpy(&dummyEvent[1], addr, 6);
	dummyEvent[sizeof(dummyEvent)-2] = GetCheckSum(dummyEvent, sizeof(dummyEvent)-2);
	memcpy(data, dummyEvent, sizeof(dummyEvent));
	
	return sizeof(dummyEvent);
}


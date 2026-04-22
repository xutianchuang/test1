#include "protocol_includes.h"

//97协议读取地址
const u8 ReadMeterAddr97[18] = 
{
    0xFE,0xFE,0xFE,0xFE,0x68,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0x68,0x01,0x02,0x43,0xC3,0xD5,0x16
};

//07协议读取地址
const u8 ReadMeterAddr07[16] = 
{
    0xFE,0xFE,0xFE,0xFE,0x68,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0x68,0x13,0x00,0xDF,0x16
};

//698读取地址
const u8 ReadMeterAddr698[29] = 
{
    0xFE,0xFE,0xFE,0xFE,0x68,0x17,0x00,0x43,0x45,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
    0x14,0xFE,0x19,0x05,0x01,0x01,0x40,0x01,0x02,0x00,0x00,0xC6,0x07,0x16
};

//07读事件帧
const u8 ReadMeterEvent07[18] = 
{
    0xFE,0xFE,0x68,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0x68,0x11,0x04,0x34,0x48,0x33,0x37,0x00,0x16
};

const u8 ChanegBaud[]={
	0xFE,0xFE,0x68,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0x68,0x1F,
    0x0B, //长度  
    0x05,0x07,0x00,0x04, //数据标识04 00 07 05
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, //通信模块ID  
    0x0, //通信速率特征字   
    0x00,0x16
};

//告知资产编码，32字节格式
const u8 WriteAssetsCode[]={
	0xFE,0xFE,0x68,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0x68,0x14,
        
#ifdef ASSETSCODE_32BYTES_INFORM        
    0x2C, //长度
#else
    0x24, //长度
#endif

    0x0D,0x14,0x00,0x04, //数据标识04 00 14 0D
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //密码和操作者代码
    
#ifdef ASSETSCODE_32BYTES_INFORM       
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //增加8字节资产编码
#endif    

    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //资产编码
    0xed,0x16
};


//CRC校验表格
const u16 CrcTable[256] =
{
	0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
	0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
	0x1081, 0x0108, 0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E,
	0x9CC9, 0x8D40, 0xBFDB, 0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876,
	0x2102, 0x308B, 0x0210, 0x1399, 0x6726, 0x76AF, 0x4434, 0x55BD,
	0xAD4A, 0xBCC3, 0x8E58, 0x9FD1, 0xEB6E, 0xFAE7, 0xC87C, 0xD9F5,
	0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E, 0x54B5, 0x453C,
	0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD, 0xC974,
	0x4204, 0x538D, 0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB,
	0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3,
	0x5285, 0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A,
	0xDECD, 0xCF44, 0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72,
	0x6306, 0x728F, 0x4014, 0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9,
	0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5, 0xA96A, 0xB8E3, 0x8A78, 0x9BF1,
	0x7387, 0x620E, 0x5095, 0x411C, 0x35A3, 0x242A, 0x16B1, 0x0738,
	0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862, 0x9AF9, 0x8B70,
	0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E, 0xF0B7,
	0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF,
	0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036,
	0x18C1, 0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E,
	0xA50A, 0xB483, 0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5,
	0x2942, 0x38CB, 0x0A50, 0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD,
	0xB58B, 0xA402, 0x9699, 0x8710, 0xF3AF, 0xE226, 0xD0BD, 0xC134,
	0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7, 0x6E6E, 0x5CF5, 0x4D7C,
	0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1, 0xA33A, 0xB2B3,
	0x4A44, 0x5BCD, 0x6956, 0x78DF, 0x0C60, 0x1DE9, 0x2F72, 0x3EFB,
	0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232,
	0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A,
	0xE70E, 0xF687, 0xC41C, 0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1,
	0x6B46, 0x7ACF, 0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9,
	0xF78F, 0xE606, 0xD49D, 0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330,
	0x7BC7, 0x6A4E, 0x58D5, 0x495C, 0x3DE3, 0x2C6A, 0x1EF1, 0x0F78,
};

//校验和
u8 GetCheckSum(const u8 * src,const u16 len)
{
    u8 sum=0;
    u16 i;
    for(i=0;i<len;i++)
    {
        sum += *src++;
    }
    return sum;
}

//698 CRC
u16 GetCrc16(const u8 * src,const u16 len)
{
	u16 crc = 0xffff;
	u16 i;
	for(i = 0; i < len; i ++){
		crc = (crc >> 8 ) ^ CrcTable[ (crc ^* src ++) & 0xFF];
	}
	crc ^= 0xffff;
	return crc;
}

//计算校验
bool CheckSum(const u8 * src,const u16 len)
{
    u16 index = 0;
    u16 i = 0;
    for(;i<len;i++)
    {
        if(src[i] != 0xFE)
        {
            index = i;
            break;
        }
    }
    
    if(len - index < 2)
        return false;
    if(GetCheckSum(&src[index],len - index - 2) == src[len - 2])
        return true;
    return false;
}

//校验698CRC
bool CheckCrc16(const u8 * src,const u16 len)
{
    u16 index = 0;
    u16 i = 0;
    for(;i<len;i++)
    {
        if(src[i] != 0xFE)
        {
            index = i;
            break;
        }
    }
    if(len - index < 4)
        return false;
    u16 crc = src[len - 2];
    crc = (crc<<8) + src[len - 3];
    if(GetCrc16(&src[1+index],len - index - 4) == crc)
        return true;
    return false;
}

//地址颠倒
void TransposeAddr(u8 addr[LONG_ADDR_LEN])
{
    u8 i = 0;
    for(;i < LONG_ADDR_LEN/2;i++)
    {
        Swap(addr[i],addr[LONG_ADDR_LEN - i - 1]);
    }
}

//判断是否645帧
bool Is645Frame(u8*data,u16 len)
{
    u16 index = 0;
    bool isFind = false;
	u16 total=len;
    for(;index < len;index++)
    {
        if(data[index] == 0x68)
        {
            isFind = true;
            break;
        }
    }
    if(!isFind)
        return false;
    if(len - index < 12)
        return false;
    if(data[index] != 0x68 || data[index + 7] != 0x68)
        return false;
	len=data[index+9]+12;
	if (total<len)
	{
		return false;
	}
	if (!CheckSum(&data[index], len))
        return false;
	if (data[index+len-1]!=0x16)
	{
		return false;
	}
	return true;
}



bool Is1024Date645Frame(u8*data,u16 len)
{
    u16 index = 0;
    bool isFind = false;
    for(;index < len;index++)
    {
        if(data[index] == 0x68)
        {
            isFind = true;
            break;
        }
    }
    if(!isFind)
        return false;
    if((len - index -1024) < 12)
        return false;
    if(data[index] != 0x68 || data[index + 7] != 0x68)
        return false;
    if(0!=data[index+9])
        return false;
	len=1024+12+5;
    if(!CheckSum(&data[index],len))
        return false;
    return true;
}

//判断是否698帧
bool Is698Frame(u8*data,u16 len)
{
    u16 index = 0, head_len = 9;
    bool isFind = false;
	u16 total=len;
    for(; index<len;index++)
    {
        if(data[index] == 0x68)
        {
            isFind = true;
            break;
        }
    }
	
    if(!isFind)
        return false;

	if(data[index+2] & (1<<6)) //单位千字节
	{
		len = (data[index+1]|(data[index+2]<<8))*1000;
		if(len > 1000)
		{
			return false;
		}
	}
	else
	{
		len = data[index+1]|(data[index+2]<<8);
	}

	len += 2;
	if (total<len)
	{
		return false;
	}
	head_len += (data[index+4]&0xF) + 1;
#if 0 //1字节扩展逻辑地址已经包含在地址长度中
	if(data[index+4] & (1<<5)) //有1字节的扩展逻辑地址
	{
		head_len += 1;
	}
#endif	

	//帧头校验
	if(!CheckCrc16(&data[index], head_len))
       	return false;
		
	//帧校验
    if(!CheckCrc16(&data[index], len))
        return false;
	if (data[index+len-1]!=0x16)
	{
		return false;
	}
    return true;
}

//传入地址和数据标识，生成帧
u16 Get645_07DataFrame(u8* data,u8 addr[6],u32 DI)
{
	u16 index = 0;
	u8 sum = 0;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0x68;
	memcpy(&data[index],addr,6);
	index += 6;
	data[index++] = 0x68;
	data[index++] = 0x11;
	data[index++] = 0x04;
	
	data[index++] = (DI&0xFF) + 0x33;
	data[index++] = ((DI>>8)&0xFF) + 0x33;
	data[index++] = ((DI>>16)&0xFF) + 0x33;
	data[index++] = ((DI>>24)&0xFF) + 0x33;
	
	sum = GetCheckSum(&data[4],index - 4);
	data[index++] = sum;
	data[index++] = 0x16;
	return index;
}
u16 Get645_07DataFrameWithData(u8* data,u8 addr[6],u32 DI,u8 *data_645,u8 len)
{
	u16 index = 0;
	u8 sum = 0;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0x68;
	memcpy(&data[index],addr,6);
	index += 6;
	data[index++] = 0x68;
	data[index++] = 0x11;
	data[index++] = 0x04+len;
	
	data[index++] = (DI&0xFF) + 0x33;
	data[index++] = ((DI>>8)&0xFF) + 0x33;
	data[index++] = ((DI>>16)&0xFF) + 0x33;
	data[index++] = ((DI>>24)&0xFF) + 0x33;

    for(int i=0;i<len;i++)
    {
        data[index++] = *data_645+0x33;
        data_645++;
    }

	sum = GetCheckSum(&data[4],index - 4);
	data[index++] = sum;
	data[index++] = 0x16;
	return index;
}

#ifdef INTELLIGENCE_LOADING

//判断是否是入网状态心跳帧
bool Is645_NetStatusFrame(u8*data,u16 len)
{
    u16 index = 0;
    bool isFind = false;

    for(;index < len;index++)
    {
        if(data[index] == 0x68)
        {
            isFind = true;
            break;
        }
    }
    if(!isFind)
        return false;
    if(len - index < 13)    //长度固定为13
        return false;
    if(data[index] != 0x68 || data[index + 7] != 0x68)
        return false;
    if (data[index + 8] != 0x00) {   //控制码为0x00
        return false;
    }
    if (data[index + 9] != 0x01) {    //数据长度为1
        return false;
    }
    if ((data[index + 10] != 0x33) && (data[index + 10] != 0x34)) {    //数据领域取值：0x34(已入网) 0x33(未入网)
        return false;
    }
    if (!CheckSum(&data[index], len))
        return false;
    if (data[index+len-1]!=0x16)
    {
        return false;
    }
    return true;
}

u16 Get645_NetStatusFrame(u8* data,u8 addr[6],u8 isInNet)
{
	u16 index = 0;
	u8 sum = 0;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0x68;
	memcpy(&data[index],addr,6);
	index += 6;
	data[index++] = 0x68;
	data[index++] = 0x00;  //控制码为0x00
	data[index++] = 0x01;  //数据长度为1

	if (isInNet) {
		data[index++] = 0x34;    //已入网
	} else {
		data[index++] = 0x33;    //未入网
	}

	sum = GetCheckSum(&data[4],index - 4);
	data[index++] = sum;
	data[index++] = 0x16;
	return index;
}

#endif

int GetBaud(u8 *data, u8 *len ,const u8* addr,u8 band)
{
	if (*len<sizeof(ChanegBaud))
	{
		return 0;
	}
	memcpy(data, ChanegBaud, sizeof(ChanegBaud));
	P_Dl645_Freme pFrame=(P_Dl645_Freme)&data[2];
	memcpy(pFrame->meter_number,addr,6);
    TransposeAddr(pFrame->meter_number);
	memcpy(&pFrame->dataBuf[4], StaChipID.mac,6);
    //1字节通信速率特征字，bit6-19200，bit5-9600，bit4-4800，bit3-2400，bit2-1200，bit1-600，bit7和bit0保留
	pFrame->dataBuf[10]=1<<band;
	*len=pFrame->dataLen+12+2;
	for (int i=0;i<pFrame->dataLen;i++)
	{
		pFrame->dataBuf[i]+=0x33;
	}
	pFrame->dataBuf[pFrame->dataLen]=GetCheckSum((u8*)pFrame,pFrame->dataLen+10);
	return pFrame->dataLen + 12+2;

}

//设置资产编码
u8 SetAssetsCode(u8 *data, const u8* addr)
{
    memcpy(data, WriteAssetsCode, sizeof(WriteAssetsCode));
    
	P_Dl645_Freme pFrame = (P_Dl645_Freme)&data[2];

	memcpy(pFrame->meter_number, addr, 6);
    TransposeAddr(pFrame->meter_number);

#ifdef ASSETSCODE_32BYTES_INFORM 
    memcpy_swap(&pFrame->dataBuf[20], StaChipID.assetsCode, 24);
#else
	memcpy_swap(&pFrame->dataBuf[12], StaChipID.assetsCode, 24);
#endif

    for (int i=0; i<pFrame->dataLen; i++)
	{
		pFrame->dataBuf[i] += 0x33;
	}
    
	pFrame->dataBuf[pFrame->dataLen] = GetCheckSum((u8*)pFrame, pFrame->dataLen+10);
 
	return sizeof(WriteAssetsCode);   
}

//获取07事件帧
u16 Get645_07EventFrame(u8* data,u8 addr[6])
{
    return Get645_07DataFrame(data,addr,0x04001501);
}

//获取07读电压数据块帧
u16 Get645_07VoltageFrame(u8* data,u8 addr[6])
{
	return Get645_07DataFrame(data,addr,0x0201FF00);
}

//获取07读频率帧
u16 Get645_07FrequenceFrame(u8* data,u8 addr[6])
{
	return Get645_07DataFrame(data,addr,0x02800002);
}
static u16 Get698_RequestFrame(u8* data, u8 addr[6], u16 OI)
{
	u16 index = 0, sum = 0, data_len = 0;
    
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0x68;

    /*len*/
    index += 2;
   
    /*control area*/
    data[index++] = 0<<7|1<<6|0<<5|3;

    /*addr area*/
    /*serv addr*/
    data[index++] = 0<<6|0<<4|(6-1);
    memcpy(&data[index], addr, 6);
	index += 6;
    /*client addr*/
    data[index++] = 0;

    /*HCS*/
    index += 2;
    
    /*Date*/
	data[index++] = 5; /*GET-Request*/
	data[index++] = 1; /*GetRequestNormal*/
    data[index++] = 0<<7|1;
	data[index++] = ((OI>>8)&0xFF);
	data[index++] = (OI&0xFF);
    data[index++] = 2;
//    data[index++] = 0;
//    data[index++] = 0;

    /*FCS*/
    index += 2;

    /*caculate len*/
    data_len = index - 4+3-2;//- fe +cs + end  
    data[5] = (data_len&0xFF);
    data[6] = ((data_len>>8)&0xFF);

    /*caculate FCS*/
    sum = GetCrc16(&data[5], 11);
	data[16] = (sum&0xFF);
	data[17] = ((sum>>8)&0xFF);

    /*caculate HCS*/
    sum = GetCrc16(&data[5], data_len-2);
	data[index++] = (sum&0xFF);
	data[index++] = ((sum>>8)&0xFF);

	data[index++] = 0x16;
    
	return index;
}


u16 Get698_EventFrame(u8* data, u8 addr[6])
{
    return Get698_RequestFrame(data, addr, 0x202F);
}
u16 Get698_TimeFrame(u8* data, u8 addr[6])
{
    return Get698_RequestFrame(data, addr, 0x4000);
}
u16 Get645BroadTime(u8* data, Time_Special *time)
{
	u16 index = 0;
	u8 sum = 0;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0x68;
	memset(&data[index],0x99,6);
	index += 6;
	data[index++] = 0x68;
	data[index++] = 0x08;
	data[index++] = 0x06;
	
	data[index++] = Hex2BcdChar(time->tm_sec) + 0x33;
	data[index++] = Hex2BcdChar(time->tm_min) + 0x33;
	data[index++] = Hex2BcdChar(time->tm_hour) + 0x33;
	data[index++] = Hex2BcdChar(time->tm_mday) + 0x33;
	data[index++] = Hex2BcdChar(time->tm_mon+1) + 0x33;
	data[index++] = Hex2BcdChar(time->tm_year+1900-2000) + 0x33;
	sum = GetCheckSum(&data[4],index - 4);
	data[index++] = sum;
	data[index++] = 0x16;
	return index;
}

u16 Get698BroadTime(u8* data,Time_Special *time)
{
	u16 index = 0, sum = 0, data_len = 0x1a;
    
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0xFE;
	data[index++] = 0x68;

    /*len*/
    data[index++] = 0x1a;
	data[index++] = 0x00;
    /*control area*/
    data[index++] = 0<<7|1<<6|0<<5|3;

    /*addr area*/
    /*serv addr*/
    data[index++] = 0xc0;//广播地址
	data[index++] = 0xaa;
    /*client addr*/
    data[index++] = 0;

    /*HCS*/
    data[index++] = 0x5e;
	data[index++] = 0xf7;
    
    /*Date*/
	data[index++] = 7; /*SET-Request*/
	data[index++] = 1; /*GetRequestNormal*/
    data[index++] = 0;
	data[index++] = 0x40;
	data[index++] = 0x00;
    data[index++] = 0x7f;
	data[index++] = 0x0;

	data[index++] = 0x1c;//date_time_s
	data[index++] = ((time->tm_year+1900)>>8)&0xff;
	data[index++] = (time->tm_year+1900)&0xff;
	data[index++] = time->tm_mon+1;
	data[index++] = time->tm_mday;
	data[index++] = time->tm_hour;
	data[index++] = time->tm_min;
	data[index++] = time->tm_sec;

    data[index++] = 0;//timeTag
//    data[index++] = 0;

    /*caculate HCS*/
    sum = GetCrc16(&data[5], data_len-2);
	data[index++] = (sum&0xFF);
	data[index++] = ((sum>>8)&0xFF);

	data[index++] = 0x16;
    
	return index;
}


//#include "../hal/macrodriver.h"
#include "type_define.h"
#include "system_inc.h"
//#include "../hal/rtc.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <os.h>
#include "RTC.h"

unsigned char IsPowerOn(void)
{
	return OK;
}

#if 0
void DelayShort(unsigned short t)
{
    for(;t>0;t--);
}


void delay(unsigned short t)
{    
    DelayShort(t);
}
#endif

unsigned char IsBcdCode(unsigned char *bcd,unsigned char len)
{
#if 1
    USHORT i;
    unsigned char tmp;

    for(i=0; i<len; i++)
    {
        tmp = bcd[i];

        if((tmp&0x0f) > 9)
            return ERROR;

        if((tmp>>4&0x0f) > 9)
            return ERROR;
    }

    return OK;
#else
    unsigned char tmp,rc;
    
    do{

        rc=OK;
        len--;
        tmp=(*(bcd+len))&0x0F;
        if(tmp>10)
        {
            rc=ERROR;
        }
		
        tmp=((*(bcd+len))&0xF0)>>4;
        if(tmp>10)
        {
            rc=ERROR;
        }
		
        if(rc)
        {
            *(bcd+len)=0;
        }
		
    }while(len);
    
    return rc;
#endif
}


unsigned char IsEqualSpecialData(unsigned char *src,unsigned char data,unsigned char len)
{
    do{
        if(*src!=data)
        {
            return(ERROR);
        }
        src++;
    }while(--len);
    return(OK);
}

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
};

u16 pppfcs16(u16 fcs, unsigned char* cp, u16 len)
{
    while (len--)
        fcs = (fcs >> 8) ^ fcstab[(fcs ^ *cp++) & 0xff];

    return (fcs);
}

unsigned char Get_checksum(unsigned char *ptr, unsigned short len)
{
    unsigned char sum = 0;
	unsigned short ii;
    
    //length
    for(ii=0;ii<len;ii++)
    {
        sum += *(ptr ++);
    }
    
    return sum;
}

unsigned long Get_checksum_32(unsigned long *ptr, unsigned long len)
{
    unsigned long sum = 0;
	unsigned long ii;

    for(ii=0; ii<len; ii++)
    {
        sum += ptr[ii];
    }
    
    return sum;
}

unsigned short crc_16_table[16] = {
  0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
  0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
};

unsigned short get_crc_16(unsigned short start, BYTE *p, int n)       //南网调用
{
	unsigned short crc = start;
	unsigned short r;
	/* while there is more data to process */
	while (n-- > 0)
	{
		/* compute checksum of lower four bits of *p */
		r = crc_16_table[crc & 0xF];
		crc = (crc >> 4) & 0x0FFF;
		crc = crc ^ r ^ crc_16_table[*p & 0xF];
		
		/* now compute checksum of upper four bits of *p */
		r = crc_16_table[crc & 0xF];
		crc = (crc >> 4) & 0x0FFF;
		crc = crc ^ r ^ crc_16_table[(*p >> 4) & 0xF];
		
		/* next... */
		p++;
	}
   return(crc);
}
unsigned short get_crc16(unsigned char *pbBuff, unsigned long dwLen)
{
    int i, j, nFlag = 0;
    unsigned short wStart = 0x0000; //初值为0x0000
    unsigned short wTemp;
    wTemp = wStart;
    for (i=0; i<dwLen; i++)
    {
        wTemp ^= (pbBuff[i] << 8);
        for (j=0; j<8; j++)
        {
        nFlag = wTemp & 0x8000;
        wTemp <<= 1;
        if (nFlag)
        wTemp ^= 0x1021;
        }
    }
    return wTemp;
}

void AddValue(void *ptr, unsigned short len, UCHAR value)
{
    unsigned short i = 0;
    
    for(i=0; i<len; i++)
    {
        ((unsigned char *)ptr)[i] += value;
    }
}

unsigned short ntohs(unsigned short netshort)
{
    return (unsigned short)(((netshort>>0&0xff)<<8) | ((netshort>>8&0xff)<<0));
}

unsigned long ntohl(unsigned long netlong)
{
    return (unsigned long)(((netlong>>0&0xff)<<24) | ((netlong>>8&0xff)<<16) | ((netlong>>16&0xff)<<8) | ((netlong>>24&0xff)<<0));
}

unsigned short htons(unsigned short hostshort)
{
    return ntohs(hostshort);
}

unsigned long htonl(unsigned long hostlong)
{
    return ntohl(hostlong);
}

unsigned long MakeDifferenceAbs(unsigned long v1, unsigned long v2)
{
    if(v1 > v2)
        return (v1-v2);
    else
        return (v2-v1);
}
    
/* fine item : 	
idx=find_item( _cctt_control_code_, pCcttBody->controlCode&CCTT_CONTROL_CODE_MASK) */
unsigned char find_item(const unsigned char *item_table,unsigned char item)
{
    unsigned char i=0;

    while(*(item_table+i)!=ERROR)
    {
        if(*(item_table+i)==item)
        {
            return i;
        }
        i++;
    }
	
    return ERROR;
}

//字节序转换
void TransposeByteOrder(u8 *data, u32 len)
{
	u8 i = 0;

	if (data == NULL)
	{
		return;
	}

	for (i = 0; i < len / 2; i++)
	{
		Swap(data[i], data[len - i - 1]);
	}
}

/* buffer 颠倒操作 */
unsigned char memcpy_swap(unsigned char *dst, unsigned char *src, unsigned short len)
{
    unsigned short ln = len;    // 2
    
    if( (dst == NULL) || (src == NULL) )
        return ERROR;

    //
    while(ln --)
    {
        *(unsigned char *)(dst ++ ) = *(unsigned char *)(src + ln);        
    }    

    return OK;
}


/**********************************************************

函数描述:  	将'from -> to'  的所有数据memory 初始化清空.

输入函数:	内存起始地址-> 内存结束地址

**********************************************************/
void mem_zeroinit(pvoid from, pvoid to)
{
	unsigned char* p_start =(unsigned char*)from;
	unsigned char* p_end =(unsigned char*)to;
	unsigned char* tmp;

	unsigned long 	size = 0;

	if( p_start > p_end )
	{
		tmp = p_end;

		p_end = p_start;
		p_start = tmp;
	}
		
	size = (unsigned long)p_end - (unsigned long)p_start;
	memset_far_intsafe((unsigned char*)p_start, 0, size);

	return;
}


unsigned char SlvsFindCmdItem(const unsigned char *cmd,unsigned char ctlw)
{
    unsigned char i=0;
    	
    while(*(cmd+i)!=ERROR)
    {
        if(*(cmd+i)==ctlw)
        {
            return i;
        }
        i++;
    }
    return ERROR;
};



void cctt_read_rtc_hour(DATA_FORMAT_80 * readTime)
{
    struct RTCCounterValue CounterReadVal;
    
    RTC_CounterGet( &CounterReadVal );
    
    readTime->year = CounterReadVal.Year;
    readTime->month = CounterReadVal.Month;
    readTime->day = CounterReadVal.Day;
    readTime->hour = CounterReadVal.Hour;
}

void cctt_read_rtc_date(DATA_FORMAT_20 * readDate)
{
    struct RTCCounterValue CounterReadVal;
    
    RTC_CounterGet( &CounterReadVal );
    
    readDate->year = CounterReadVal.Year;
    readDate->month = CounterReadVal.Month;
    readDate->day = CounterReadVal.Day;
}

void cctt_read_rtc_minute(DATA_FORMAT_15 * readMinute)
{
    struct RTCCounterValue CounterReadVal;
    
    RTC_CounterGet( &CounterReadVal );
    
    readMinute->year = CounterReadVal.Year;
    readMinute->month = CounterReadVal.Month;
    readMinute->day = CounterReadVal.Day;
    readMinute->hour = CounterReadVal.Hour;
    readMinute->minute = CounterReadVal.Min;
}

void cctt_read_rtc_df15(DATA_FORMAT_15 * df15)
{
    struct RTCCounterValue CounterReadVal;
    
    RTC_CounterGet( &CounterReadVal );
    
    df15->year = CounterReadVal.Year;
    df15->month = CounterReadVal.Month;
    df15->day = CounterReadVal.Day;
    df15->hour = CounterReadVal.Hour;
    df15->minute = CounterReadVal.Min;
}

void cctt_read_rtc_df01(DATA_FORMAT_01 * df01)
{
    struct RTCCounterValue CounterReadVal;
    
    RTC_CounterGet( &CounterReadVal );
    
    df01->year = CounterReadVal.Year;
    df01->month = CounterReadVal.Month;
    df01->day = CounterReadVal.Day;
    df01->hour = CounterReadVal.Hour;
    df01->minute = CounterReadVal.Min;
    df01->second = CounterReadVal.Sec;
    df01->week = CounterReadVal.Week;
    
}

void cctt_read_rtc_hn_df17(DATA_FORMAT_17 * df17)
{
    struct RTCCounterValue CounterReadVal;
    
    RTC_CounterGet( &CounterReadVal );
    
    df17->month = CounterReadVal.Month;
    df17->day = CounterReadVal.Day;
    df17->hour = CounterReadVal.Hour;
    df17->minute = CounterReadVal.Min;
}

void cctt_read_rtc_sys_time(CCTT_SYS_TIME * sys_tm)
{
    struct RTCCounterValue CounterReadVal;
    
    RTC_CounterGet( &CounterReadVal );
    
    sys_tm->year = CounterReadVal.Year;
    sys_tm->month = CounterReadVal.Month;
    sys_tm->day = CounterReadVal.Day;
    sys_tm->hour = CounterReadVal.Hour;
    sys_tm->minute = CounterReadVal.Min;
    sys_tm->second = CounterReadVal.Sec;
}

P_CCTT_MNG_METER cctt_mng_meter_offset(USHORT num)
{
    //P_CCTT_MNG_METER pMeterData;

    //pMeterData = (__far P_CCTT_MNG_METER) ((ULONG)g_MeterDataPool + (ULONG)num*sizeof(CCTT_MNG_METER));  

    return NULL;
}


P_TP_CFG cctt_mng_tp_offset(USHORT tpNo)
{
    //__far P_TP_CFG pMeterCfg;

    return NULL;      

    //return pMeterCfg;
}

unsigned char IS_Today(DATA_FORMAT_20 * p_Date)
{
#if 0  
    if( ( p_Date->month == MONTH) 
        && (p_Date->day == DAY)
        && (p_Date->year == YEAR))
    {
        return TRUE;
    } 
#endif
    return FALSE;
}

unsigned short  Slvs_Plc_Checksum(UCHAR *PLC_data,UCHAR  Len)
{
	unsigned short  temp=0;

	while(Len--)
	{
		temp+=*PLC_data ++;
	}
	
	return temp;
}

UCHAR hex2binStr(UCHAR val)
{
    if(val < 10)
    {
        return ('0'+val);
    }


    return ('A'+(val - 10));
      
}

/* 输入:  BCD CHAR	/输出: 返回Hex UCHAR */
unsigned char Bcd2HexChar(unsigned char bcd)
{
    return((bcd>>4)*10+(bcd&0x0F));
}

unsigned char* Bcd2HexBuffer(unsigned char* bcd, int len)
{
    while(len--)
    {
        bcd[len] = Bcd2HexChar(bcd[len]);
    }

    return bcd;
}

/* 输入:  Hex UCHAR	/输出: 返回BCD CHAR	 */
unsigned char Hex2BcdChar(unsigned char hex)
{
    return((hex%10)|(((unsigned char)(hex/10))<<4));
}

unsigned char IsEqual(unsigned char *src1,unsigned char * src2,unsigned char len)
{
    do{
        if(*src1!=*src2)
        {
            return(ERROR);
        }
        src1++;
        src2++;
    }while(--len);

    return(OK);
}
unsigned char macCmp(unsigned char *mac1,unsigned char * mac2)//当比较mac地址时替换IsEqual函数
{
	u32* pmac1=(u32*)mac1;
	u32* pmac2=(u32*)mac2;
	if (*pmac1++ != *pmac2++)
	{
		return(ERROR);
	}
	if ((*pmac1&0xffff)!=(*pmac2&0xffff))
	{
		return(ERROR);
	}
	return(OK);
}
unsigned char IsMemSet(void* pData, unsigned char value, unsigned short size)
{
    unsigned char* src1 = (unsigned char*)pData;
    do
    {
        if(*src1!=value)
        {
            return false;
        }
        src1++;
    }
    while(--size);

    return true;
}
/*
void * memcpy_far(ULONG pdata, ULONG psource, USHORT size)
{
    if(size == 0)
        return (void *)pdata;

    while(size --)
    {
        *((unsigned char *)pdata) = *((unsigned char *)psource);
        pdata ++;
        psource ++;
    }

    return (void *)pdata;
}*/

void * memcpy_intsafe(void * pdata, const void * psource, USHORT size)
{
	CPU_SR_ALLOC();
    if(size == 0)
        return pdata;
    OS_CRITICAL_ENTER();
    memcpy(pdata,psource,size);
    OS_CRITICAL_EXIT();

    return pdata;
}

void * memmove_intsafe( void *s1, const void *s2, USHORT n ) 
{
	CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    memmove(s1, s2, n);
    OS_CRITICAL_EXIT();
    return s1;
}

void * memset_intsafe(void * pdata, unsigned char val, unsigned short size )
{
	CPU_SR_ALLOC();
    if(size == 0)
        return pdata;
    OS_CRITICAL_ENTER();
    memset(pdata,val,size);
    OS_CRITICAL_EXIT();

    return pdata;
}

USHORT strlen_intsafe( const char *s ) 
{
	CPU_SR_ALLOC();
    USHORT len;
    OS_CRITICAL_ENTER();
    len = strlen(s);
    OS_CRITICAL_EXIT();
    return len;
}

char *strcpy_intsafe ( char *s1 , const char *s2 ) 
{
	CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    strcpy (s1 , s2 );
    OS_CRITICAL_EXIT();
    return s1;
}


void memset_far_intsafe(unsigned char *pdata, unsigned char val, unsigned long size )
{
	//unsigned long segment_size = 0x10000;

	unsigned char *ptr_start = (unsigned char*)pdata;
	unsigned char *ptr_end = (unsigned char *)((unsigned long)pdata + size);
	unsigned char *ptr = ptr_start;

	unsigned char segment_start = (unsigned char )(((ULONG)(ptr_start))>>16);
	unsigned char segment_end = (unsigned char )(((ULONG)(ptr_end))>>16);

    unsigned long len = 0, len1,  len2 = 0;
    unsigned char segment_idx = 0;
    unsigned char *ptr2 = ptr;


	if( segment_start == segment_end )
	{
		CPU_SR_ALLOC();
	    OS_CRITICAL_ENTER();
		memset((void *)ptr, val, (size_t)size);
        OS_CRITICAL_EXIT();
	}
	else
	{
		while( (ULONG)ptr < (ULONG)ptr_end )
		{	
			segment_idx = (UCHAR)(((ULONG)(ptr))>>16);
		
			ptr2 = (UCHAR *)(( (ULONG)(segment_idx + 1) )<<16);

			if( (ULONG)ptr2 > (ULONG)ptr_end )
			{
				len = (ULONG)(ptr_end) - (ULONG)(ptr);
			}
			else
			{
				len = (ULONG)(ptr2) - (ULONG)(ptr);
			}

            if(len & 0XFFFF0000)
            {
				CPU_SR_ALLOC();
                len1 = len >> 1;
                OS_CRITICAL_ENTER();
                memset((unsigned char*)ptr, val, (size_t)len1);
                OS_CRITICAL_EXIT();
                len2 = len - len1;
                OS_CRITICAL_ENTER();
                memset((unsigned char*)((unsigned long)ptr+len1), val, (size_t)len2);
                OS_CRITICAL_EXIT();
            }
            else
            {
				CPU_SR_ALLOC();
                OS_CRITICAL_ENTER();
			    memset((unsigned char*)ptr, val, (size_t)len);
                OS_CRITICAL_EXIT();
            }

			//
			ptr = (UCHAR *)((unsigned long)ptr + len);			
		}
		
	}
	
	// memset_special((unsigned char*)p_start, val, size);
	return;
}

int rand_special(void)
{
    int rs;
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    rs = rand();
    OS_CRITICAL_EXIT();
    return rs;
}

//[from, to]
u32 rand_range(u32 from, u32 to)
{
    u32 rs;
    srand_special();
    rs = rand_special();
    rs = from + rs%(to-from+1);
    return rs;
}

//[from, to]
//使用硬件种子做随机数
u32 rand_hard_range(u32 from, u32 to)
{
    u32 rs;
	rs = rand_special();
    rs = from + rs%(to-from+1);
    return rs;
}

void srand_special(void)
{
    OS_ERR err;
    int rs = OSTimeGet(&err);
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    srand(rs);
    OS_CRITICAL_EXIT();
    return;   
}


/* 输入:  Hex USHORT	/输出: 返回BCD SHORT */
unsigned short Hex2BcdShort(unsigned short hex)
{
    unsigned short temp;
    unsigned short ret;
    unsigned char ii;

    ret=0;
	
    for(ii=0;ii<4;ii++)
    {
        ret>>=4;

        temp=hex%10;

	 //向高位移位12 bit (16 bits - 4 bits).		
        temp<<=12;

        ret|=temp;

        hex=hex/10;
    }

    return(ret);
}



/////////////=================================================

static u8 III5_Addr_Compress(u8 *buff,u8 *ID)
{
    u8 i,j,n,dat=0xFF;

    memset(buff,0,6);
    j=0;
    n=0;
  
    //dat=ID[0];
    for(i=0;i<6;i++)
    {
        if(dat^ID[i])
        {
            if(n>1)
            {
                if(dat)
                {
                  buff[j++]=0xC0+(n+6); 
                }
                else
                {
                  buff[j++]=0xC0+(n-2);  
                }
            }
            else
            if(n)buff[j++]=Bcd2HexChar(dat);
            
            n=0;
            if(ID[i])
            buff[j++]=Bcd2HexChar(ID[i]);  
            else n=1;
            
            dat=ID[i];
        }
        else
        {
          n++;
        }
    }
    if(n)
    {
        if(n>1)
            {
                if(dat)
                {
                  buff[j++]=0xC0+(n+6); 
                }
                else
                {
                  buff[j++]=0xC0+(n-2);  
                }
            }
            else
            buff[j++]=Bcd2HexChar(dat);
    }
    return(j);
}
const u8 BROADCAST_ID[6]={0x99,0x99,0x99,0x99,0x99,0x99};
const u8 BROADCAST_ID_AA[6]={0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};
static void MeterID_PLC421_Cov_DL645(u8 dir,u8 *PLC421,u8 *DL645)
{
    u8 i;
    if(!dir)
    {
     if(!memcmp(DL645,BROADCAST_ID_AA,6))
        {
            memset(PLC421,0,6);
            PLC421[5]=0x66;
            return;
        }
        if(!memcmp(DL645,BROADCAST_ID,6))
        {
            memset(PLC421,0,6);
            PLC421[5]=0x64;
            return;
        }
    }
    for(i=0; i<6; i++)
    {
        if(dir)
        {  
            if(PLC421[5-i]==0x99)DL645[i]=0x99;
            else
            if(PLC421[5-i]==0xAA)DL645[i]=0xAA;
            else
            DL645[i]=Hex2BcdChar(PLC421[5-i]);
        }
        else 
        {
               
            PLC421[i]=Bcd2HexChar(DL645[5-i]);
                
        }
    }
}
static u8 PLC421_Frame_Compress(u8 *Buff,u8 Leve,u8 len)
{
    u8 j,i,cnt,new_len,leng;
    u8 *Addr_ptr;

    Addr_ptr=(u8 *)Buff;
    new_len=len;
    cnt=Leve+2;
    while(cnt>1)
    {
        cnt--;
        for(i=0; i<6; i++)
        {
            Buff[(cnt*6)+i]^=Buff[((cnt-1)*6)+i];
            Buff[(cnt*6)+i]<<=1;
        }
        Buff[(cnt*6)+5]+=1;
    }
    for(i=0; i<6; i++)
    {
        Buff[i]<<=1;
    }
    Buff[5]+=1;
    cnt=Leve+2;
    leng=0;
    for(j=0; j<cnt; j++)
    {
        for(i=0; i<6; i++)
        {
            if(Addr_ptr[i])break;
        }
        //new_len-=i;
        leng+=i;
        memmove(Addr_ptr,Addr_ptr+i,new_len-i);
        Addr_ptr+=(6-i);
        new_len-=6;
    }
    return(len-leng);
}
static u8 GDW3762_COV_DR_PLC_Addr(u8 *GDW_Addr,u8 *DR_Addr,u8 Leve)
{
  u8 length,i,cnt=0,cnt_bak=0,j,bat;
  u8 Temp_ID[6],Temp_ID1[6];


  	
  length=0;
  memset(Temp_ID1,0,6);
  for(i=0;i<(Leve+2);i++)
  {
 
        memcpy(Temp_ID,GDW_Addr+((Leve+1-i)*6),6);
 	if(Temp_ID[0]==0xAA)
 	{
     	memset(Temp_ID,0,6);
		Temp_ID[0]=0x7F;	
	}
    else
    if(!memcmp(Temp_ID,BROADCAST_ID,6))
    {
        memset(Temp_ID,0,6);
		Temp_ID[0]=0x7F;
    }
	else
	{
		for(j=0;j<6;j++)
	    	{
	        	bat=Bcd2HexChar(Temp_ID[j]);
	        	Temp_ID[j]=bat;
	    	}
	}

    if(i)
    {
        for(j=0;j<6;j++)
        	{
			Temp_ID1[j]-=Temp_ID[j];
			Temp_ID1[j]&=0x7F;
		}
	 for(j=0;j<6;j++)
	 	{
			if(Temp_ID1[5-j])break;
	 	}
	 Temp_ID1[5-j]|=0x80;
	 cnt_bak=6-j;
	 memmove(DR_Addr+cnt_bak,DR_Addr,length);
	 memcpy(DR_Addr,Temp_ID1,cnt_bak);
	 length+=cnt_bak;
    }
	memcpy(Temp_ID1,Temp_ID,6);
    	cnt_bak=cnt;
  }
 
  for(j=0;j<6;j++)
	{
		if(Temp_ID1[5-j])break;
	}
  	Temp_ID1[5-j]|=0x80;
	cnt_bak=6-j;
	memmove(DR_Addr+cnt_bak,DR_Addr,length);
	memcpy(DR_Addr,Temp_ID1,cnt_bak);
	length+=cnt_bak;
  return(length);
}

//-----------------------------计算下发报文延时--------------------------------------------
//返回值: 秒
u8 GDW3762_CopyMeter_Time(u8 *Addr, //3762 中的地址域
        u8 leve,    //级数
        u8 Cmd,     //0x02,0x13
        u8 Agrr,    //01:97, 02:07, 00:透传
        u8 L,       //DL645长度
        u8 *buff,   //DL645
        u16 bps     //330, 100
        )
{
    u16 Timeout;
    u8 Length,Point,i;
    //u8 *PLC_buff;
    u8 Send_buff[50];       //报文比较长时，会溢出

    if(bps==330)
    {
       Point=7;
       Length=GDW3762_COV_DR_PLC_Addr(Addr,Send_buff+7,leve);
       Point+=Length;
       Send_buff[Point++]=buff[9]+2;//RS485_DL645_Frame_Ptr->L+2;
       Send_buff[Point++]=0x00;
       Send_buff[Point++]=0x00;
       Send_buff[Point++]=buff[8];
       memcpy(Send_buff+Point,buff+10,buff[9]);
       Point+=buff[9];
       Point+=6;
       Timeout=Point*8;
       Timeout+=33;
       i=(u8)(Timeout/330);
       if(Timeout%330)i++;
       return(i);
    }
    if(bps==331)
    {
        i=III5_Addr_Compress(Send_buff+4,Addr);
        Length=i;
        Length+=III5_Addr_Compress(Send_buff+4+i,buff+1);
        Length+=(12+buff[9]);
        Timeout=Length*8;
        Timeout+=33;
        i=(u8)(Timeout/330);
        if(Timeout%330)i++;
        return(i);    
    }

    
    Send_buff[3]=0x9A;
    Send_buff[4]=0x01;
   
    Point=0;
    for(i=0;i<leve+2;i++)
    {
        MeterID_PLC421_Cov_DL645(0,Send_buff+5+Point,Addr+Point);
        Point+=6;
    }
    if(Agrr)
    {
      Point++;
      Point++;
      Point+=buff[9];
    }
    else
    {
       Point++;
       Point+=L;    
    }

    Send_buff[2]=Point+5;
    Length=PLC421_Frame_Compress(Send_buff+5,leve,Send_buff[2]-3);
    Send_buff[2]=Length+3;
   
    Timeout=(6+Length)*(leve+1);
    Timeout*=8;
    if(bps==1200)
    {
       Timeout+=100;
       i=(u8)(Timeout/1000);
       if(Timeout%1000)i++;
       return(i);
    }
    else
     if(bps==600)
     {
        Timeout+=50;
        i=(u8)(Timeout/500);
       if(Timeout%500)i++;
       return(i);
     }
     else
     if(bps==100)
     {
        Timeout+=10;
        i=(u8)(Timeout/100);
       if(Timeout%100)i++;
     }
     else
     {
       Timeout+=5;
        i=(u8)(Timeout/50);
       if(Timeout%50)i++;
     }
     return(i);
}

//需周期性调用    
OS_TICK_64 GetTickCount64()
{
    CPU_SR_ALLOC();
    CPU_CRITICAL_ENTER();

//    OS_ERR err;
    OS_TICK tick_now = OSTickCtr;   //OSTimeGet(&err);

    static OS_TICK tick_last_low = 0;
    static OS_TICK tick_last_high = 0;

    if(tick_now < tick_last_low)
        tick_last_high++;
    tick_last_low = tick_now;

    OS_TICK_64 tick_high_64 = tick_last_high;
    OS_TICK_64 tick_low_64 = tick_last_low;
    OS_TICK_64 tick_now_64 = ((tick_high_64<<32)|tick_low_64);

    CPU_CRITICAL_EXIT();

    return tick_now_64;
}

OS_TICK TicksBetween(OS_TICK tick_now, OS_TICK tick_then)
{
#if 0
    if(tick_now>=tick_then)
        return (tick_now-tick_then);
    else
        return (OS_TICK)0xffffffff-tick_then+tick_now+1;
#else
	tick_now-=tick_then;
	return tick_now;
#endif
}

OS_TICK_64 TicksBetween64(OS_TICK_64 tick_now, OS_TICK_64 tick_then)
{
#if 0
    if(tick_now>=tick_then)
        return (tick_now-tick_then);
    else
        return (OS_TICK_64)0xffffffffffffffff-tick_then+tick_now+1;
#else
	tick_now-=tick_then;
	return tick_now;
#endif

}

//[tick_begin, tick_end)
BOOL IsTickBetween64(OS_TICK_64 tick_now, OS_TICK_64 tick_begin, OS_TICK_64 tick_end)
{
//    if(tick_end >= tick_begin)
//    {
#if 1
        return (BOOL)((tick_now>=tick_begin) && (tick_now<tick_end));
#else
		if ((tick_now-tick_begin)<(tick_end-tick_begin))
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
#endif
//    }
//    else
//    {
//        return (tick_now>=tick_begin || tick_now<tick_end);
//    }
}

//需周期性调用

static CPU_NTB ntb_last_low = 0;
static CPU_NTB ntb_last_high = 0;
CPU_NTB_64 GetNtbCount64()
{
    CPU_SR_ALLOC();
    CPU_CRITICAL_ENTER();

    CPU_NTB ntb_now = BPLC_GetNTB();

    

    if(ntb_now < ntb_last_low)
        ntb_last_high++;
    ntb_last_low = ntb_now;

    CPU_NTB_64 ntb_high_64 = ntb_last_high;
    CPU_NTB_64 ntb_low_64 = ntb_last_low;
    OS_TICK_64 ntb_now_64 = ((ntb_high_64<<32)|ntb_low_64);

    CPU_CRITICAL_EXIT();

    return ntb_now_64;
}

uint32_t NtbsBetween(uint32_t ntb_now, uint32_t ntb_then)
{
    return TicksBetween(ntb_now, ntb_then);
}

CPU_NTB_64 NtbsBetween64(CPU_NTB_64 ntb_now, CPU_NTB_64 ntb_then)
{
    return TicksBetween64(ntb_now, ntb_then);
}

//[ntb_begin, ntb_end)
BOOL IsNtbBetween(uint32_t ntb_now, uint32_t ntb_begin, uint32_t ntb_end)
{
#if 0
    if(ntb_end >= ntb_begin)
    {
        return (ntb_now>=ntb_begin && (ntb_now<ntb_end));
    }
    else
    {
        return (ntb_now>=ntb_begin || ntb_now<ntb_end);
    }
#else
	if ((ntb_now-ntb_begin)<(ntb_end-ntb_begin))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
#endif
}

//[ntb_begin, ntb_end)
BOOL IsNtbBetween64(CPU_NTB_64 ntb_now, CPU_NTB_64 ntb_begin, CPU_NTB_64 ntb_end)
{
    //if(ntb_end >= ntb_begin)
    //{
#if 1
	return (BOOL)((ntb_now>=ntb_begin) && (ntb_now<ntb_end));
#else
	if ((ntb_now-ntb_begin)<(ntb_end-ntb_begin))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
#endif
    //}
    //else
    //{
    //    return (ntb_now>=ntb_begin || ntb_now<ntb_end);
    //}
}

#if 0
//result = System_NTB - System_TICK
int64_t GetOffsetNtbTick()
{
    OS_TICK_64 tick_now = GetTickCount64();
    CPU_NTB_64 ntb_now = GetNtbCount64();

    OS_TICK_64 tick_now_ms = tick_now*1000/OSCfg_TickRate_Hz;
    CPU_NTB_64 tick_now_ntb = NTB_FRE_PER_MS*tick_now_ms;

    if(ntb_now > tick_now_ntb)
        return (ntb_now-tick_now_ntb);
    else
        return (0-(int64_t)(tick_now_ntb-ntb_now));
}

OS_TICK_64 NtbToTick64(CPU_NTB_64 ntb)
{
    int64_t offset_ntb = GetOffsetNtbTick();
    if(ntb > offset_ntb)
        ntb -= offset_ntb;
    return ((OS_TICK_64)OSCfg_TickRate_Hz * ntb/NTB_FRE_PER_MS/1000);
}

CPU_NTB_64 TickToNtb64(OS_TICK_64 tick)
{
    return ((CPU_NTB_64)NTB_FRE_PER_SEC * tick/OSCfg_TickRate_Hz);
}
#endif

BOOL IsCommTestMode(COMM_TEST_MODE test_mode, HPLC_PHASE* pPhase)
{
    OS_TICK_64 tick_now = GetTickCount64();

    P_COMM_TEST_PARAM pTestMode = &g_PlmStatusPara.comm_test_param;

    if(pTestMode->comm_test_mode != test_mode)
        return FALSE;

    if(NULL != pPhase)
        *pPhase = pTestMode->receive_phase;

    switch(pTestMode->comm_test_mode)
    {
    case TEST_MODE_HRF_CHANNEL_CHANGE:
    case TEST_MODE_HRF_PACKET_TO_HRF:
    case TEST_MODE_HRF_PACKET_TO_UART:
    case TEST_MODE_HRF_PLC_PACKET_TO_HRF_PLC:
	case TEST_MODE_PLC_PACKET_TO_HRF:
    case TEST_MODE_SECUER_TO_UART: 
    case TEST_MODE_TRAN_MPDU_TO_COMM:
    case TEST_MODE_PASS_MPDU_BACK:
    case TEST_MODE_TRAN_MAC_TO_COMM:
    {
        if(TicksBetween64(tick_now, pTestMode->comm_test_start) < (OS_TICK_64)OSCfg_TickRate_Hz*60*pTestMode->comm_test_value)
            return TRUE;
        break;
    }
    case TEST_MODE_FREQUENCE:
    {
        return TRUE;
    }
    case TEST_MODE_TONEMASK:
    {
        return TRUE;
    }
    default:
        break;
    }
    
    return FALSE;
}

BOOL IsCommTestModeOr(COMM_TEST_MODE test_mode_array[], u8 array_len, HPLC_PHASE* pPhase)
{        
    OS_TICK_64 tick_now = GetTickCount64();
    P_COMM_TEST_PARAM pTestMode = &g_PlmStatusPara.comm_test_param;

    for(u8 i=0; i<array_len; i++)
    {
        COMM_TEST_MODE test_mode = test_mode_array[i];

        if(pTestMode->comm_test_mode != test_mode)
            continue;

        if(NULL != pPhase)
            *pPhase = pTestMode->receive_phase;

        switch(pTestMode->comm_test_mode)
        {
        case TEST_MODE_TRAN_MPDU_TO_COMM:
        case TEST_MODE_PASS_MPDU_BACK:
        case TEST_MODE_TRAN_MAC_TO_COMM:
        case TEST_MODE_HRF_CHANNEL_CHANGE:
        case TEST_MODE_HRF_PACKET_TO_HRF:
        case TEST_MODE_HRF_PACKET_TO_UART:
        case TEST_MODE_HRF_PLC_PACKET_TO_HRF_PLC:
		case TEST_MODE_PLC_PACKET_TO_HRF:
        case TEST_MODE_SECUER_TO_UART:
        {
            if(TicksBetween64(tick_now, pTestMode->comm_test_start) < (OS_TICK_64)OSCfg_TickRate_Hz*60*pTestMode->comm_test_value)
                return TRUE;
            break;
        }
        case TEST_MODE_FREQUENCE:
        {
            return TRUE;
        }
        case TEST_MODE_TONEMASK:
        {
            return TRUE;
        }
        default:
            continue;
        }
    }

    return FALSE;
}

void ClearCommTestMode()
{
    P_COMM_TEST_PARAM pTestMode = &g_PlmStatusPara.comm_test_param;

    CPU_SR_ALLOC();
    CPU_CRITICAL_ENTER();
    
    pTestMode->comm_test_mode = TEST_MODE_NONE;
	BPLC_SetNormalModeParam();

    CPU_CRITICAL_EXIT();
}


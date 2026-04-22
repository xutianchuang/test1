#ifndef _LOCAL_PROTOCOL_H_
#define _LOCAL_PROTOCOL_H_

//本地协议，包括DLT645-1997,DLT645-2007,P698.45协议
#define SEND_DL645_DATA_BUF_SIZE 128
enum
{
    LOCAL_PRO_TRANSPARENT = 0,
    LOCAL_PRO_DLT645_1997 = 1,
    LOCAL_PRO_DLT645_2007 = 2,
    LOCAL_PRO_698_45 = 4
};

typedef struct {
	unsigned char protocolHead;   
	unsigned char meter_number[6];			
	unsigned char protocolHead2;  
	unsigned char controlCode;
	unsigned char dataLen;
	unsigned char dataBuf[SEND_DL645_DATA_BUF_SIZE];
}Dl645_Freme,*P_Dl645_Freme;
//97协议读取地址
extern const u8 ReadMeterAddr97[18];
//07协议读取地址
extern const u8 ReadMeterAddr07[16];
//698读取地址
extern const u8 ReadMeterAddr698[29];
//645-2007读事件
extern const u8 ReadMeterEvent07[18] ;

//获取校验和
u8 GetCheckSum(const u8 * src,const u16 len);

//计算校验
bool CheckSum(const u8 * src,const u16 len);

//获取698CRC校验
u16 GetCrc16(const u8 * src,const u16 len);

//校验698CRC
bool CheckCrc16(const u8 * src,const u16 len);

//地址颠倒
void TransposeAddr(u8 addr[LONG_ADDR_LEN]);

//获得切换波特率的帧
int GetBaud(u8 *data, u8 *len ,const u8* addr,u8 band);
//设置资产编码
u8 SetAssetsCode(u8 *data, const u8* addr);
//判断是否645帧
bool Is645Frame(u8*data,u16 len);
//判断是否是1024长度的645帧
bool Is1024Date645Frame(u8*data,u16 len);
//判断是否698帧
bool Is698Frame(u8*data,u16 len);


//组07帧
u16 Get645_07DataFrame(u8* data,u8 addr[6],u32 DI);
u16 Get645_07DataFrameWithData(u8* data,u8 addr[6],u32 DI,u8 *data_645,u8 len);
//获取07时间帧
u16 Get645_07EventFrame(u8* data,u8 addr[6]);

//获取07读电压数据块帧
u16 Get645_07VoltageFrame(u8* data,u8 addr[6]);

//获取07读频率帧
u16 Get645_07FrequenceFrame(u8* data,u8 addr[6]);

//读取698时间帧
u16 Get698_TimeFrame(u8* data, u8 addr[6]);


u16 Get645BroadTime(u8* data, Time_Special *time);
u16 Get698BroadTime(u8* data,Time_Special *time);
bool EVENT_AddTolist(P_Dl645_Freme p645);

#ifdef INTELLIGENCE_LOADING
bool Is645_NetStatusFrame(u8*data,u16 len);
u16 Get645_NetStatusFrame(u8* data,u8 addr[6],u8 isInNet);
#endif


#endif

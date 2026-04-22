#ifndef _LOCAL_PROTOCOL_H_
#define _LOCAL_PROTOCOL_H_

//本地协议，包括DLT645-1997,DLT645-2007,P698.45协议

enum
{
    LOCAL_PRO_TRANSPARENT = 0,
    LOCAL_PRO_DLT645_1997 = 1,
    LOCAL_PRO_DLT645_2007 = 2,
    LOCAL_PRO_698_45 = 3
};


//97协议读取地址
extern const u8 ReadMeterAddr97[16];
//07协议读取地址
extern const u8 ReadMeterAddr07[14];
//698读取地址
extern const u8 ReadMeterAddr698[29];

//校验和
u8 GetCheckSum(const u8 * src,const u16 len);

//?645
bool CheckSum(const u8 * src,const u16 len);

//698 CRC
u16 GetCrc16(const u8 * src,const u16 len);
u16 GetInitCrc16(u16 crcinit,const u8 * src,const u16 len);
//?698
bool CheckCrc16(const u8 * src,const u16 len);

#endif

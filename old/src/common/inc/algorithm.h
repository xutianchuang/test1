#ifndef _ALGORITHM_H_
#define _ALGORITHM_H_

//存放一些公共算法

//交换两个值
#define Swap(a,b) \
do\
{\
    (a) = (a) ^ (b);\
    (b) = (a) ^ (b);\
    (a) = (a) ^ (b);\
}while(0)


typedef enum
{
    INSERT_FAIL,        //插入异常
    INSERT_EXIST,       //插入失败，因为原来的表中已存在数据，但参数返回值中有数据的下标
    INSERT_SUCCESS,     //插入成功，参数返回数据下标
}EN_INSERT_RESULT;

//map中插入一个数(map数组，maxsize数组最大值,数组当前拥有元素个数num,dat 要插入的数,返回下标)
EN_INSERT_RESULT InsertMap16(u16 map[],u16 maxsize,u16 *num,u16 dat,u16 *index);

//查找map中对应的数，index为返回下标
bool FindMap16(u16 *map,u16 num,u16 dat,u16 *index);

//移除
bool RemoveMap16(u16 *map,u16 *num,u16 dat);

//对应插入32位数
EN_INSERT_RESULT InsertMap32(u32 *map,u16 maxsize,u16 *num,u32 dat,u16 *index);

//对应查找32位数
bool FindMap32(u32 *map,u16 num,u32 dat,u16 *index);

//移除
bool RemoveMap32(u32 *map,u16 *num,u32 dat);

//冒泡排序
void BubbleSort16(u16 *array,int num);

//冒泡排序
void BubbleSort32(u32 *array,int num);

u32 CommonGetCrc32(u8 *data,u32 len);
u32 CommonGetCrc32Init(u32 init ,u8 *data,u32 len);

//bcd转十进制
unsigned char bcd_to_dec(unsigned char bcd);

//十进制转bcd
unsigned char dec_to_bcd(unsigned char dec);

/* 输入:  Hex UCHAR	/输出: 返回BCD CHAR	 */
unsigned char Hex2BcdChar(unsigned char hex);

unsigned short ntohs(unsigned short netshort);
unsigned short htons(unsigned short hostshort);

/* buffer 颠倒操作 */
unsigned char memcpy_swap(unsigned char *dst, unsigned char *src, unsigned short len);

//地址转字符串
void GetAddrString(const u8 addr[6],char strAddr[13]);

//获取两个NTB的差值
u32 NTBDiff(u32 begin,u32 end);

//获取一个字节有多少位置一
u8 ByteCount1Bit(u32 bitmap);
bool memIsHex(u8 *data,u8 hex,u16 len);
u32 GetMask(u8 num);
#endif

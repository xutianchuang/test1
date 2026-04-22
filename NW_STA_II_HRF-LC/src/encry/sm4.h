#ifndef SMS4_H
#define SMS4_H

#define SM4_BASE              0x40640000


#define SMSDIN3R               (*(volatile unsigned int *)(SM4_BASE))
#define SMSDIN2R               (*(volatile unsigned int *)(SM4_BASE|0x04))
#define SMSDIN1R               (*(volatile unsigned int *)(SM4_BASE|0x08))
#define SMSDIN0R               (*(volatile unsigned int *)(SM4_BASE|0x0c))
#define SMSKIN3R               (*(volatile unsigned int *)(SM4_BASE|0x10))
#define SMSKIN2R               (*(volatile unsigned int *)(SM4_BASE|0x14))
#define SMSKIN1R               (*(volatile unsigned int *)(SM4_BASE|0x18))
#define SMSKIN0R               (*(volatile unsigned int *)(SM4_BASE|0x1c))
#define SMSIV3R                (*(volatile unsigned int *)(SM4_BASE|0x20))
#define SMSIV2R                (*(volatile unsigned int *)(SM4_BASE|0x24))
#define SMSIV1R                (*(volatile unsigned int *)(SM4_BASE|0x28))
#define SMSIV0R                (*(volatile unsigned int *)(SM4_BASE|0x2c))
#define SMSCSR                 (*(volatile unsigned int *)(SM4_BASE|0x30))


#define SMSSEED                 (*(volatile unsigned int *)(SM4_BASE|0x34))
#define SMSKEY4R                (*(volatile unsigned int *)(SM4_BASE|0x38))



#define SM4_MODE_MASK			0x00C00000  
#define SM4_MEM				0x00000010  
#define SM4_DONE			0x00000020  
#define SM4_KEY_RDY			0x00000040 
#define SM4_RESET			0x00000008  
#define SM4_IE				0x00000004  
#define SM4_CBC				0x00800000//CBC模式
#define SM4_ECB				0x00000000//ECB模式  
#define SM4_ENCODE			0x00000000//加密
#define SM4_DECODE			0x00000001//解密




//sm4 加解密函数
//key: 输入参数，sm4密钥
//iv :输入参数，ECB模式传入0，CBC模式提供即可
//data_in:输入参数，待加解密的数据，必须是16字节的整数倍
//data_out:输出参数，加解密得到的结果
//length:输入参数，data_in的字节长度，必须是16字节的整数倍
//mode：模式，参数是SM4_CBC和SM4_ECB二选一
//en_de:加密或者解密模式，参数是SM4_ENCODE 和SM4_DECODE 二选一
void sm4_en_de(unsigned char*key,unsigned char *iv,unsigned char *data_in,unsigned char *data_out, unsigned int length,unsigned int mode,unsigned int en_de);

#endif
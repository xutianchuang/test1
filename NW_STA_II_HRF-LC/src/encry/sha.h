#ifndef SHA_H
#define SHA_H




#define         SHA_BASE                 0x40800000//0x80000000 

#define		SHACSR		        (*(volatile unsigned int *)(SHA_BASE))
#define 	SHAWDR		        (*(volatile unsigned int *)(SHA_BASE+0x4))
#define 	SHARDR		        (*(volatile unsigned int *)(SHA_BASE+0x8))
#define 	SHAWSR		        (*(volatile unsigned int *)(SHA_BASE+0xc))
#define 	SHAWLENTHR	        (*(volatile unsigned int *)(SHA_BASE+0x1c))


#define		MODE_SM3		0x00
#define 	MODE_SHA0		0x10
#define 	MODE_SHA1		0x20
#define 	MODE_SHA224		0x30
#define 	MODE_SHA256		0x40
#define 	FIRSTB 		        0x08
#define  	SHA_START 	        0x01
#define 	SHA_IE		        0x04
#define         SHA_FINAL               0x100
#define         SHA_DONE                0x20000
#define         SHA_BUSY                0x10000

//hash 模块init,每次做hash前需要调用
//mode 支持MODE_SM3,MODE_SHA256
void sha_init(unsigned int mode);
//hash update，每次update只支持63字节整数倍
//msg:输入参数,待处理的消息
//length:msg的字节长度，64字节整数倍
void sha_update(unsigned int* msg,unsigned int length);
//hash final处理
//msg:待处理的消息
//length:msg的字节长度，可以是任意字节长度
//result:hash的结果，需要32字节空间
void sha_final(unsigned char* msg,unsigned int length,unsigned int* result);


//sm2签名验签相关的hash处理函数,计算ZA = SM3(ENTLA ∥ IDA ∥ a ∥ b ∥ xG ∥ yG ∥ xA ∥ yA)
//ida:输入参数，用户A的ID
//entla:输入参数，ida的字节长度
//eccA:输入参数，SM2参数A
//eccB:输入参数，SM2参数B
//Gx:输入参数，SM2参数基点x坐标
//Gy:输入参数，SM2参数基点y坐标
//Px:输入参数，用户A的公钥x坐标
//Py:输入参数,用户A的公钥y坐标
//eccLen:输入参数，SM2参数以及Px,Py的字长度
//Za:输出参数，计算得到的Za结果,32字节空间
void sm2_calZA(unsigned char* ida,unsigned short entla,
               unsigned int* eccA,unsigned int* eccB,
               unsigned int* Gx,unsigned int* Gy,
               unsigned int* Px,unsigned int* Py,
               unsigned int eccLen,
               unsigned int* Za);

//sm2签名验签相关的hash处理函数,计算hash = SM3(Za||msg)
//Za:输入参数，函数sm2_calZA得到的结果，8个word
//msg:输入参数，签名的消息，任意长度
//msgLen:输入参数，msg的字节长度
//hash:输出参数，计算得到的hash结果
void sm2_calHash(unsigned int* Za,unsigned char* msg,unsigned int msgLen,
                 unsigned int* hash);


#endif

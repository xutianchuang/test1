#ifndef CRYPTO_H
#define CRYPTO_H

#define CRYTO_BASE                 0x40700000//0x60000000

#define CRYPTO_CACR                (*(volatile unsigned int *)(CRYTO_BASE))
#define CRYPTO_CASR                (*(volatile unsigned int *)(CRYTO_BASE+0x4))
#define CRYPTO_CAAR                CRYTO_BASE+0x8
#define CRYPTO_CAER                CRYTO_BASE+0x108
#define CRYPTO_CANR                CRYTO_BASE+0x208

#define CRYPTO_START               0x1
#define CRYPTO_RESET               0x2
#define CRYPTO_F2N                 0x8
#define CRYPTO_NOT_R2              0x20


#define CRYPTO_DONE                0x1

#define CRYPTO_PRE_CAL            (0x0<<8)
#define CRYPTO_A_DOUBLE           (0x1<<8)
#define CRYPTO_EXP                (0x2<<8)
#define CRYPTO_ADD                (0x3<<8)
#define CRYPTO_SUB                (0x4<<8)
#define CRYPTO_A_PRE_MULT         (0x5<<8)
#define CRYPTO_MONT_MULT          (0x6<<8)
#define CRYPTO_MONT_MULT_A2       (0x7<<8)
#define CRYPTO_ECC2P              (0x8<<8)
#define CRYPTO_ECCPQ              (0x9<<8)



#define ECC_BIGINT32_MAXLEN         16 //max support ecc512,如果只需要支持ecc256可以改成8

//数据的倒序
unsigned int Reverse(unsigned int value);


//ecc点乘函数，(By,Bx) = stuK*(Ax,Ay)
//stuK:输入参数，点乘的系数
//keyLen:输入参数，stuK的字长度
//Ax:输入参数，点坐标x
//Ay:输入参数，点坐标y
//Bx:输出参数，结果坐标x
//By:输出参数，结果坐标y
//eccP:输入参数，ecc参数p
//eccA:输入参数，ecc参数A
//eccLen:输入参数，ecc参数的字长度
//data_cache:输入参数，缓存空间大小为ECC_BIGINT32_MAXLEN*12个字
void PointMult(unsigned int *stuK, unsigned int keyLen,
			   unsigned int *Ax, unsigned int *Ay,
			   unsigned int *Bx, unsigned int *By,
			   unsigned int *eccP, unsigned int *eccA,
			   unsigned int eccLen, unsigned int *data_cache);



//sm2签名算法(内部做SM3运算)
//ida:输入参数，用户A的ID
//entla:输入参数，ida的字节长度
//msg:输入参数,待签名大的消息
//msgLen:输入参数,msg的字节长度
//random_data:输入参数，签名中的随机数，要小于基点的阶eccN，长度等于eccLen
//prikey:输入参数，用户的私钥，长度等于eccLen
//Gx:输入参数，ecc参数基点坐标x
//Gy:输入参数，ecc参数基点坐标y
//eccP:输入参数，ecc参数p
//eccA:输入参数，ecc参数A
//eccN:输入参数，ecc参数基点的阶
//eccLen:输入参数，ecc参数的字长度
//signR:输出参数，签名结果R，长度等于eccLen
//signS:输出参数，签名结果S，长度等于eccLen
//data_cache:输入参数，缓存空间大小为ECC_BIGINT32_MAXLEN*16个字
void SM2_Sign_WithIDA(unsigned char *ida, unsigned int entla,
					  unsigned char *msg, unsigned int msgLen,
					  unsigned int *random_data, unsigned int *prikey,
					  unsigned int *Px, unsigned int *Py,
					  unsigned int *Gx, unsigned int *Gy,
					  unsigned int *eccP, unsigned int *eccA,
					  unsigned int *eccB, unsigned int *eccN, unsigned int eccLen,
					  unsigned int *signR, unsigned int *signS, unsigned int *data_cache);

//sm2签名算法(外部做SM3运算):
//hash:输入参数，hash结果，函数sm2_calHash的输出
//hashLen:输入参数，hash的字长度
//random_data:输入参数，签名中的随机数，要小于基点的阶eccN，长度等于eccLen
//prikey:输入参数，用户的私钥，长度等于eccLen
//Gx:输入参数，ecc参数基点坐标x
//Gy:输入参数，ecc参数基点坐标y
//eccP:输入参数，ecc参数p
//eccA:输入参数，ecc参数A
//eccN:输入参数，ecc参数基点的阶
//eccLen:输入参数，ecc参数的字长度
//signR:输出参数，签名结果R，长度等于eccLen
//signS:输出参数，签名结果S，长度等于eccLen
//data_cache:输入参数，缓存空间大小为ECC_BIGINT32_MAXLEN*16个字
void SM2_Sign(unsigned int *hash, unsigned int hashLen,
			  unsigned int *random_data, unsigned int *prikey,
			  unsigned int *Gx, unsigned int *Gy,
			  unsigned int *eccP, unsigned int *eccA,
			  unsigned int *eccN, unsigned int eccLen,
			  unsigned int *signR, unsigned int *signS, unsigned int *data_cache);



//sm2验签函数(内部做SM3运算):
//ida:输入参数，用户A的ID
//entla:输入参数，ida的字节长度
//msg:输入参数,待签名大的消息
//msgLen:输入参数,msg的字节长度
//Px:输入参数，用户的公钥坐标x
//Py:输入参数，用户的公钥坐标y
//Gx:输入参数，ecc参数基点坐标x
//Gy:输入参数，ecc参数基点坐标y
//eccP:输入参数，ecc参数p
//eccA:输入参数，ecc参数A
//eccN:输入参数，ecc参数基点的阶
//eccLen:输入参数，ecc参数的字长度
//signR:输入参数，待验签的签名值R，长度等于eccLen
//signS:输入参数，待验签的签名值S，长度等于eccLen
//返回值:0:验签成功，1：验签失败
//data_cache:输入参数，缓存空间大小为ECC_BIGINT32_MAXLEN*17个字
unsigned char SM2_Verify_WithIDA(unsigned char *ida, unsigned int entla,
								 unsigned char *msg, unsigned int msgLen,
								 unsigned int *Px, unsigned int *Py,
								 unsigned int *Gx, unsigned int *Gy,
								 unsigned int *eccP, unsigned int *eccA,
								 unsigned int *eccB, unsigned int *eccN, unsigned int eccLen,
								 unsigned int *signR, unsigned int *signS, unsigned int *data_cache);

//sm2验签函数(外部做SM3运算):
//hash:输入参数，hash结果，函数sm2_calHash的输出
//hashLen:输入参数，hash的字长度
//Px:输入参数，用户的公钥坐标x
//Py:输入参数，用户的公钥坐标y
//Gx:输入参数，ecc参数基点坐标x
//Gy:输入参数，ecc参数基点坐标y
//eccP:输入参数，ecc参数p
//eccA:输入参数，ecc参数A
//eccN:输入参数，ecc参数基点的阶
//eccLen:输入参数，ecc参数的字长度
//signR:输入参数，待验签的签名值R，长度等于eccLen
//signS:输入参数，待验签的签名值S，长度等于eccLen
//返回值:0:验签成功，1：验签失败
//data_cache:输入参数，缓存空间大小为ECC_BIGINT32_MAXLEN*17个字
unsigned char SM2_Verify(unsigned int *hash, unsigned int hashLen,
						 unsigned int *Px, unsigned int *Py,
						 unsigned int *Gx, unsigned int *Gy,
						 unsigned int *eccP, unsigned int *eccA,
						 unsigned int *eccN, unsigned int eccLen,
						 unsigned int *signR, unsigned int *signS, unsigned int *data_cache);


//ecc签名(ecdsa)函数:内部会调用SHA256处理msg得到hash结果
//msg:输入参数，待签名的消息
//msgLen:输入参数，msg的字节长度
//random_data:输入参数，签名中的随机数，要小于基点的阶eccN，长度等于eccLen
//prikey:输入参数，用户的私钥，长度等于eccLen
//Gx:输入参数，ecc参数基点坐标x
//Gy:输入参数，ecc参数基点坐标y
//eccP:输入参数，ecc参数p
//eccA:输入参数，ecc参数A
//eccN:输入参数，ecc参数基点的阶
//eccLen:输入参数，ecc参数的字长度
//signR:输出参数，签名结果R，长度等于eccLen
//signS:输出参数，签名结果S，长度等于eccLen
//返回值:0:验签成功，1：验签失败
//data_cache:输入参数，缓存空间大小为ECC_BIGINT32_MAXLEN*16个字
void ECDSA_Sign(unsigned char *msg, unsigned int msgLen,
				unsigned int *random_data, unsigned int *prikey,
				unsigned int *Gx, unsigned int *Gy,
				unsigned int *eccP, unsigned int *eccA,
				unsigned int *eccN, unsigned int eccLen,
				unsigned int *signR, unsigned int *signS, unsigned int *data_cache);

//sm2验签函数:内部会调用SHA256处理msg得到hash结果
//msg:输入参数，待签名的消息
//msgLen:输入参数，msg的字节长度
//Px:输入参数，用户的公钥坐标x
//Py:输入参数，用户的公钥坐标y
//Gx:输入参数，ecc参数基点坐标x
//Gy:输入参数，ecc参数基点坐标y
//eccP:输入参数，ecc参数p
//eccA:输入参数，ecc参数A
//eccN:输入参数，ecc参数基点的阶
//eccLen:输入参数，ecc参数的字长度
//signR:输入参数，待验签的签名值R，长度等于eccLen
//signS:输入参数，待验签的签名值S，长度等于eccLen
//返回值:0:验签成功，1：验签失败
//data_cache:输入参数，缓存空间大小为ECC_BIGINT32_MAXLEN*17个字
unsigned char ECDSA_Verify(unsigned char *msg, unsigned int msgLen,
						   unsigned int *Px, unsigned int *Py,
						   unsigned int *Gx, unsigned int *Gy,
						   unsigned int *eccP, unsigned int *eccA,
						   unsigned int *eccN, unsigned int eccLen,
						   unsigned int *signR, unsigned int *signS, unsigned int *data_cache);


#endif

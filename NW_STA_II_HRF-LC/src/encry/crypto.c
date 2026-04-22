#include "crypto.h"
#include "sha.h"


//字节大小端转换函数
unsigned int Reverse(unsigned int value)
{
	return ((((value)&0xff) << 24) | (((value)&0xff00) << 8) | (((value)&0xff0000) >> 8) | (((value)&0xff000000) >> 24));
}


void cpu_write_data(unsigned int dst, unsigned int *src, unsigned int length)
{
	unsigned int i;
	for (i = 0; i < length; i = i + 1)
	{
		(*(volatile unsigned int *)dst) = (src[i]);
		dst = dst + 4;
	}
	if (length & 0x1)
		(*(volatile unsigned int *)dst) = 0;
}


void cpu_write_exp_data(unsigned int dst, unsigned int *src, unsigned int length)
{
	unsigned int i;
	for (i = 0; i < length; i = i + 1)
	{
		(*(volatile unsigned int *)dst) = (src[i]);
		dst = dst + 4;
	}
}


void cpu_read_data(unsigned int *result, unsigned int dst, unsigned int length)
{
	unsigned int i;
	for (i = 0; i < length; i = i + 1)
	{
		result[i] = (*(volatile unsigned int *)dst);
		dst = dst + 4;
	}
}

void EccBig123(unsigned int *stuA, unsigned int *stuE, unsigned int *stuC, unsigned int length, unsigned int cmd)
{

	unsigned int opsize = length - 1;

	if (stuA)
		cpu_write_data(CRYPTO_CAAR, stuA, length);

	if (stuE)
		cpu_write_data(CRYPTO_CAER, stuE, length);

	CRYPTO_CACR = cmd | (opsize << 16) | (opsize << 24) | 0x80008000 | CRYPTO_START;

	while ((CRYPTO_CASR & CRYPTO_DONE) != CRYPTO_DONE);

	if (stuC)
		cpu_read_data(stuC, CRYPTO_CAAR, length);

}





void PowMod(unsigned int *stuA, unsigned int *stuE, unsigned int *R2, unsigned int plen, unsigned int elen, unsigned int *result)
{

	unsigned int opsize = plen - 1;
	elen = elen - 1;
	//CRYPTO_CACR = 0x80000000;

	cpu_write_data(CRYPTO_CAAR, R2, plen);
	cpu_write_data(CRYPTO_CAER, stuA, plen);

	CRYPTO_CACR = CRYPTO_A_PRE_MULT | (opsize << 16) | (elen << 24) | 0x80008080 | CRYPTO_START;
	while ((CRYPTO_CASR & CRYPTO_DONE) != CRYPTO_DONE);

	CRYPTO_CACR = 0x80008080 | CRYPTO_EXP | (opsize << 16) | (elen << 24);
	cpu_write_exp_data(CRYPTO_CAER, stuE, plen);
	CRYPTO_CACR = CRYPTO_EXP | (opsize << 16) | (elen << 24) | 0x80008080 | CRYPTO_START;
	while ((CRYPTO_CASR & CRYPTO_DONE) != CRYPTO_DONE);

	cpu_read_data(result, CRYPTO_CAAR, plen);

}



void Eccint32MovBig(unsigned int  *pstuA, unsigned int  *pstuB, unsigned int maxsize)
{
	short i;

	for (i = 0; i < maxsize; i++)
	{
		pstuA[i] = pstuB[i]; //pstuB的值赋给pstuA的值
	}
}


unsigned int EccBigFf1(unsigned int  *pstuA, unsigned int  uBits)
{
	short i, j;
	int value;
	for (i = uBits; i >= 0; i--)
	{
		value = 0x80000000;
		for (j = 31; j >= 0; j--)
		{
			if (pstuA[i] & value)
			{
				return (32 * i + j);
			}
			value >>= 1;
		}
	}
	return 0;
}


unsigned short EccBig32GetNBit(unsigned int  *pstuA, unsigned int aLen, short sNum)
{
	short i;
	unsigned int  auValue;

	i = sNum >> 5;
	if (i >= aLen)
	{
		return 0;
	}
	auValue = pstuA[i];
	auValue >>= (sNum & 0x1F);
	auValue &= 0x1;
	return auValue;
}

void EccAssign(unsigned int *stuA, unsigned int value, unsigned int maxsize)
{

	short i;
	stuA[0] = value;
	for (i = 1; i < maxsize; i++)
	{
		stuA[i] = 0;
	}

}

void PointAddJac(unsigned int *Ax, unsigned int *Ay, unsigned int *Az,
				 unsigned int *Bx, unsigned int *By,
				 unsigned int *Cx, unsigned int *Cy, unsigned int *Cz, unsigned int length,
				 unsigned int *data_cache)
{
	unsigned int *l1 = data_cache;
	unsigned int *l2 = data_cache + ECC_BIGINT32_MAXLEN;
	unsigned int *l3 = data_cache + ECC_BIGINT32_MAXLEN * 2;
	unsigned int *l4 = data_cache + ECC_BIGINT32_MAXLEN * 3;
	Eccint32MovBig(l2, Ax, length);
	Eccint32MovBig(l4, Ay, length);
	//z2=1
	EccBig123(Az, Az, l1, length, CRYPTO_MONT_MULT); //l1=z1^2
	EccBig123(By, 0, 0, length, CRYPTO_MONT_MULT); //l3 = y2*z1
	EccBig123(0, l1, l3, length, CRYPTO_MONT_MULT); //l3=y2*z1^3
	EccBig123(Bx, 0, l1, length, CRYPTO_MONT_MULT); //l1=x2*z1^2

	EccBig123(l1, Ax, l1, length, CRYPTO_SUB); //l1=x2*z1^2-x1*z2^2=x2*z1^2-x1
	EccBig123(l3, Ay, l3, length, CRYPTO_SUB); //l3=y2*z1^3-y1*z2^3=y2*z1^3-y1

	EccBig123(l3, l3, Cx, length, CRYPTO_MONT_MULT); //x3=l3*l3
	EccBig123(Az, l1, Cz, length, CRYPTO_MONT_MULT); //z3=z1*l1*z2=z1*l1
	EccBig123(l1, l1, Cy, length, CRYPTO_MONT_MULT); //y3=l1*l2
	EccBig123(l4, 0, l4, length, CRYPTO_MONT_MULT); //l4=l4*l1

	EccBig123(Cy, 0, l1, length, CRYPTO_MONT_MULT); //l1=l1*y3
	EccBig123(l4, Cy, l4, length, CRYPTO_MONT_MULT); //l4=l4*y3
	EccBig123(l2, 0, Cy, length, CRYPTO_MONT_MULT); //y3=l2*y3

	EccBig123(Cx, l1, Cx, length, CRYPTO_SUB); //x3=x3-l1
	EccBig123(Cy, Cy, l2, length, CRYPTO_ADD); //l2=y3+y3

	EccBig123(Cx, l2, Cx, length, CRYPTO_SUB); //x3=x3-l2
	EccBig123(Cy, Cx, 0, length, CRYPTO_SUB); //y3=y3-x3
	EccBig123(0, l3, 0, length, CRYPTO_MONT_MULT); //y3=y3*l3
	EccBig123(0, l4, Cy, length, CRYPTO_SUB); //y3=y3-l4

}

void PointDBLJac(unsigned int *Ax, unsigned int *Ay, unsigned int *Az, unsigned int *Az4,
				 unsigned int length, unsigned int *data_cache)
{
	unsigned int *l1 = data_cache;
	unsigned int *l2 = data_cache + ECC_BIGINT32_MAXLEN;
	unsigned int *l3 = data_cache + ECC_BIGINT32_MAXLEN * 2;

	EccBig123(Ax, Ax, l1, length, CRYPTO_MONT_MULT); //l1 = x^2
	EccBig123(Ay, Ay, l2, length, CRYPTO_MONT_MULT); //l2 = y^2

	EccBig123(0, l2, l3, length, CRYPTO_MONT_MULT); //l3 = y^4
	EccBig123(Ax, 0, l2, length, CRYPTO_MONT_MULT); //l2 = x*y^2


	EccBig123(l1, 0, 0, length, CRYPTO_A_DOUBLE); //Ax=2*x^2
	EccBig123(0, l1, Ax, length, CRYPTO_ADD); //Ax=3*x^2
	EccBig123(l2, 0, l2, length, CRYPTO_A_DOUBLE); //l2 = 2*x*y^2
	EccBig123(0, 0, l2, length, CRYPTO_A_DOUBLE); //l2 = 4*x*y^2

	EccBig123(Ax, Az4, l1, length, CRYPTO_ADD); //l1=3*x^2+az4
	EccBig123(l3, 0, l3, length, CRYPTO_A_DOUBLE); //l3 = 2*y4

	EccBig123(l1, l1, 0, length, CRYPTO_MONT_MULT);
	EccBig123(0, l2, 0, length, CRYPTO_SUB);
	EccBig123(0, l2, Ax, length, CRYPTO_SUB); //Ax = l1^2-2*l2

	EccBig123(Ay, Az, 0, length, CRYPTO_MONT_MULT);
	EccBig123(0, 0, Az, length, CRYPTO_A_DOUBLE); //z = 2*y*z

	EccBig123(l3, 0, 0, length, CRYPTO_A_DOUBLE);
	EccBig123(0, 0, l3, length, CRYPTO_A_DOUBLE); //l3 = 8*y^4
	EccBig123(0, Az4, 0, length, CRYPTO_MONT_MULT);
	EccBig123(0, 0, Az4, length, CRYPTO_A_DOUBLE); //az4 = 16*y^4*z^4

	EccBig123(l2, Ax, 0, length, CRYPTO_SUB);
	EccBig123(0, l1, 0, length, CRYPTO_MONT_MULT);
	EccBig123(0, l3, Ay, length, CRYPTO_SUB);

}


void ECC_Cal_AZ4(unsigned int *AR, unsigned int *Cz, unsigned int *aZ4, unsigned int length)
{
	EccBig123(Cz, Cz, aZ4, length, CRYPTO_MONT_MULT); //cz^2
	EccBig123(0, aZ4, 0, length, CRYPTO_MONT_MULT); //cz^4
	EccBig123(0, AR, aZ4, length, CRYPTO_MONT_MULT); //a*z^4

}

void PointMult(unsigned int *stuK, unsigned int keyLen,
			   unsigned int *Ax, unsigned int *Ay,
			   unsigned int *Bx, unsigned int *By,
			   unsigned int *eccP, unsigned int *eccA, unsigned int eccLen,
			   unsigned int *data_cache)
{


	signed short sNum;
	signed short i;

	unsigned int *ceil_cache = data_cache;
	unsigned int *pPointCx = data_cache + ECC_BIGINT32_MAXLEN * 4;
	unsigned int *pPointCy = data_cache + ECC_BIGINT32_MAXLEN * 5;
	unsigned int *pPointCz = data_cache + ECC_BIGINT32_MAXLEN * 6;
	unsigned int *pPointCaz4 = data_cache + ECC_BIGINT32_MAXLEN * 7;
	unsigned int *AR = data_cache + ECC_BIGINT32_MAXLEN * 8;
	unsigned int *pPintAx_mont = data_cache + ECC_BIGINT32_MAXLEN * 9;
	unsigned int *pPintAy_mont = data_cache + ECC_BIGINT32_MAXLEN * 10;
	unsigned int *R2 = data_cache + ECC_BIGINT32_MAXLEN * 11;


	unsigned int opsize = eccLen - 1;



	sNum = EccBigFf1(stuK, keyLen - 1);

	if (sNum <= 0)
	{
		if (stuK[0] == 0)
		{
			EccAssign(Bx, 0, eccLen);
			EccAssign(By, 0, eccLen);
		}
		else
		{
			Eccint32MovBig(Bx, Ax, eccLen);
			Eccint32MovBig(By, Ay, eccLen);
		}
		return;
	}



	EccAssign(pPointCz, 0x1, ECC_BIGINT32_MAXLEN);
	//pPointCz.uLen = 1;

	Eccint32MovBig(pPointCaz4, eccA, eccLen);
	CRYPTO_CACR = 0x80000000;
	cpu_write_data(CRYPTO_CANR, eccP, eccLen);
	/*change to montogmery*/
	CRYPTO_CACR = 0x80008000 | (opsize << 16) | (opsize << 24) | CRYPTO_PRE_CAL | CRYPTO_START; //pre_calc

	while ((CRYPTO_CASR & CRYPTO_DONE) != CRYPTO_DONE);
	cpu_read_data(R2, CRYPTO_CAAR, eccLen);

	EccBig123(Ax, 0, pPointCx, eccLen, CRYPTO_A_PRE_MULT);
	EccBig123(Ay, 0, pPointCy, eccLen, CRYPTO_A_PRE_MULT);
	EccBig123(pPointCz, 0, pPointCz, eccLen, CRYPTO_A_PRE_MULT);
	EccBig123(pPointCaz4, 0, pPointCaz4, eccLen, CRYPTO_A_PRE_MULT);

	Eccint32MovBig(AR, pPointCaz4, ECC_BIGINT32_MAXLEN);
	Eccint32MovBig(pPintAx_mont, pPointCx, ECC_BIGINT32_MAXLEN);
	Eccint32MovBig(pPintAy_mont, pPointCy, ECC_BIGINT32_MAXLEN);
	/*end change to montogmery*/

	keyLen = (sNum >> 5) + 1;

	//first

	for (i = sNum - 1; i >= 0; i--)
	{

		PointDBLJac(pPointCx, pPointCy, pPointCz, pPointCaz4, eccLen, ceil_cache);

		if (EccBig32GetNBit(stuK, keyLen, i))
		{
			PointAddJac(pPointCx, pPointCy, pPointCz, pPintAx_mont, pPintAy_mont,
						pPointCx, pPointCy, pPointCz, eccLen, ceil_cache);

			ECC_Cal_AZ4(AR, pPointCz, pPointCaz4, eccLen);
		}

	}




	//evalute AR = 1
	EccAssign(AR, 0x1, ECC_BIGINT32_MAXLEN);



	//change Cz to general
	//pPintAx_mont = Cz*1
	EccBig123(pPointCz, AR, pPintAx_mont, eccLen, CRYPTO_MONT_MULT);

	//evalute pPintAy_mont = 2
	EccAssign(pPintAy_mont, 0x2, ECC_BIGINT32_MAXLEN);

	EccBig123(eccP, pPintAy_mont, pPintAy_mont, eccLen, CRYPTO_SUB);
	//pPintAx_mont = 1/Cz
	PowMod(pPintAx_mont, pPintAy_mont, R2, eccLen, eccLen, pPintAx_mont);

	CRYPTO_CACR = 0x80008000 | (opsize << 16) | (opsize << 24) | CRYPTO_PRE_CAL;
	//convert to montogmery
	EccBig123(pPintAx_mont, R2, AR, eccLen, CRYPTO_MONT_MULT);

	//pPintAx_mont = 1/(Cz^2)
	//pPintAy_mont = 1/(Cz^3)
	EccBig123(0, pPintAx_mont, pPintAx_mont, eccLen, CRYPTO_MONT_MULT);
	EccBig123(0, AR, pPintAy_mont, eccLen, CRYPTO_MONT_MULT);

	EccBig123(pPintAx_mont, pPointCx, Bx, eccLen, CRYPTO_MONT_MULT);
	EccBig123(pPintAy_mont, pPointCy, By, eccLen, CRYPTO_MONT_MULT);


}



unsigned char BigNumEqual(unsigned int *stuA, unsigned int *stuB, unsigned int length)
{
	unsigned int i;
	for (i = 0; i < length; i = i + 1)
	{
		if (stuB[i] != stuA[i])
			return 0;
	}

	return 1;
}

void ECC_PointAdd(unsigned int *Ax, unsigned int *Ay,
				  unsigned int *Bx, unsigned int *By,
				  unsigned int *Cx, unsigned int *Cy,
				  unsigned int *eccP, unsigned int *eccA,
				  unsigned int eccLen, unsigned int *data_cache)
{
	unsigned int *lamda = data_cache;
	unsigned int *t1 = data_cache + ECC_BIGINT32_MAXLEN;
	unsigned int *t2 = data_cache + ECC_BIGINT32_MAXLEN * 2;
	unsigned int *R2 = data_cache + ECC_BIGINT32_MAXLEN * 3;
	unsigned int opsize = eccLen - 1;
	CRYPTO_CACR = 0x80000000;
	cpu_write_data(CRYPTO_CANR, eccP, eccLen);
	CRYPTO_CACR = 0x80008080 | (opsize << 16) | (opsize << 24) | CRYPTO_PRE_CAL | CRYPTO_START; //pre_calc

	while ((CRYPTO_CASR & CRYPTO_DONE) != CRYPTO_DONE);
	cpu_read_data(R2, CRYPTO_CAAR, eccLen);

	EccAssign(lamda, 0x2, ECC_BIGINT32_MAXLEN);
	EccBig123(eccP, lamda, lamda, eccLen, CRYPTO_SUB);

	if (BigNumEqual(Ax, Bx, eccLen) &&
		BigNumEqual(Ay, By, eccLen))
	{
		//point_dbl

		//t1 = 2*y
		EccBig123(Ay, 0, t1, eccLen, CRYPTO_A_DOUBLE);
		//t1 = 1/(2y)
		PowMod(t1, lamda, R2, eccLen, eccLen, t1);
		CRYPTO_CACR = 0x80008000 | (opsize << 16) | (opsize << 24) | CRYPTO_PRE_CAL;
		//3*x2+a
		EccBig123(Ax, R2, 0, eccLen, CRYPTO_MONT_MULT);
		EccBig123(0, Ax, t2, eccLen, CRYPTO_MONT_MULT);
		EccBig123(0, 0, 0, eccLen, CRYPTO_A_DOUBLE);
		EccBig123(0, t2, 0, eccLen, CRYPTO_ADD);
		EccBig123(0, eccA, 0, eccLen, CRYPTO_ADD);

		EccBig123(0, R2, 0, eccLen, CRYPTO_MONT_MULT);

		EccBig123(0, t1, lamda, eccLen, CRYPTO_MONT_MULT);

	}
	else
	{
		//point_add
		//t1 = y2-y1
		EccBig123(By, Ay, t1, eccLen, CRYPTO_SUB);
		//t2 = x2-x1
		EccBig123(Bx, Ax, t2, eccLen, CRYPTO_SUB);
		//t2 = 1/(x2-x1)
		PowMod(t2, lamda, R2, eccLen, eccLen, t2);
		CRYPTO_CACR = 0x80008000 | (opsize << 16) | (opsize << 24) | CRYPTO_PRE_CAL;
		EccBig123(0, R2, 0, eccLen, CRYPTO_MONT_MULT);

		EccBig123(0, t1, lamda, eccLen, CRYPTO_MONT_MULT);

	}

	//t1 = lamda*lamda
	EccBig123(0, R2, 0, eccLen, CRYPTO_MONT_MULT);
	EccBig123(0, lamda, 0, eccLen, CRYPTO_MONT_MULT);
	//Cx = t1 - x1-x2
	EccBig123(0, Ax, 0, eccLen, CRYPTO_SUB);
	EccBig123(0, Bx, Cx, eccLen, CRYPTO_SUB);

	//x1-x3
	EccBig123(Ax, Cx, 0, eccLen, CRYPTO_SUB);
	//lamda *(x1-x3)
	EccBig123(0, R2, 0, eccLen, CRYPTO_MONT_MULT);
	EccBig123(0, lamda, 0, eccLen, CRYPTO_MONT_MULT);
	//Cy = lamda *(x1-x3) - y1
	EccBig123(0, Ay, Cy, eccLen, CRYPTO_SUB);
}

unsigned char BigNumBigger(unsigned int *stuA, unsigned int aLen,
						   unsigned int *stuN, unsigned int nLen)
{

	if (aLen > nLen)
		return 1;
	else if (aLen < nLen)
		return 0;
	else
	{
		signed int i;
		for (i = aLen - 1; i >= 0; i--)
		{
			if (stuA[i] > stuN[i])
				return 1;
			else if (stuA[i] < stuN[i])
				return 0;
		}
		return 0;
	}
}

unsigned int BigNumLeftShift(unsigned int *stuN, unsigned int nLen, unsigned int *result, unsigned int shiftLen)
{

	EccAssign(result, 0x0, ECC_BIGINT32_MAXLEN);
	unsigned int index = shiftLen >> 5;
	unsigned int leftLen = shiftLen & 0x1f;
	unsigned int rightLen = 32 - leftLen;
	signed int i;


	for (i = nLen - 1; i > 0; i--)
	{
		result[i + index] = (stuN[i] << leftLen) | (stuN[i] >> rightLen);
	}

	result[index] = stuN[0] << leftLen;
	return index + nLen;
}

void BigNumSub(unsigned int *stuA, unsigned int aLen, unsigned int *stuN, unsigned int nLen, unsigned int *result)
{

	unsigned int i;
	unsigned int carry = 0;
	for (i = 0; i < nLen; i++)
	{
		if (stuA[i] < stuN[i] + carry)
		{
			result[i] = stuA[i] - stuN[i] - carry;
			carry = 1;
		}
		else
		{
			result[i] = stuA[i] - stuN[i] - carry;
			carry = 0;
		}
	}

	for (i = nLen; i < aLen; i++)
	{
		result[i] = stuA[i] - carry;
		if (result[i] > stuA[i])
			carry = 1;
	}

}

//result = stuA mod stuN
void BigNumMod(unsigned int *stuA, unsigned int aLen, unsigned int *stuN, unsigned int nLen,
			   unsigned int *result, unsigned int *data_cache)
{
	unsigned int nbitLen = EccBigFf1(stuN, nLen - 1);
	unsigned int abitLen = EccBigFf1(stuA, aLen - 1);
	unsigned int *ntemp = data_cache;
	unsigned int ntlen;
	EccAssign(result, 0x0, ECC_BIGINT32_MAXLEN);
	unsigned int i;
	for (i = 0; i < aLen; i++)
	{
		result[i] = stuA[i];
	}
	while (BigNumBigger(result, aLen, stuN, nLen))
	{
		if (abitLen > nbitLen)
		{
			ntlen = BigNumLeftShift(stuN, nLen, ntemp, abitLen - nbitLen - 1);
			BigNumSub(result, aLen, ntemp, ntlen, result);
			abitLen = EccBigFf1(result, aLen - 1);
			aLen = (abitLen >> 5) + 1;
		}
		else
		{
			BigNumSub(result, aLen, stuN, nLen, result);
		}

	}
}




void SM2_Sign(unsigned int *hash, unsigned int hashLen,
			  unsigned int *random_data, unsigned int *prikey,
			  unsigned int *Gx, unsigned int *Gy,
			  unsigned int *eccP, unsigned int *eccA,
			  unsigned int *eccN, unsigned int eccLen,
			  unsigned int *signR, unsigned int *signS,
			  unsigned int *data_cache)
{

	unsigned int *point_mul_cache = data_cache;
	unsigned int *x1 = data_cache + ECC_BIGINT32_MAXLEN * 12;
	unsigned int *y1 = data_cache + ECC_BIGINT32_MAXLEN * 13;
	unsigned int *R2 = data_cache + ECC_BIGINT32_MAXLEN * 14;
	unsigned int *interHash = data_cache + ECC_BIGINT32_MAXLEN * 15;
	unsigned int opsize = eccLen - 1;
	EccAssign(x1, 0x0, ECC_BIGINT32_MAXLEN);
	EccAssign(y1, 0x0, ECC_BIGINT32_MAXLEN);
	EccAssign(R2, 0x0, ECC_BIGINT32_MAXLEN);

    CRYPTO_CACR = 0x80000000;

	BigNumMod(hash, hashLen, eccN, eccLen, interHash, point_mul_cache);

	PointMult(random_data, eccLen, Gx, Gy, x1, y1, eccP, eccA, eccLen, point_mul_cache);
	//y1 = x1 mod N
	BigNumMod(x1, eccLen, eccN, eccLen, y1, point_mul_cache);

	cpu_write_data(CRYPTO_CANR, eccN, eccLen);
	CRYPTO_CACR = 0x80008000 | (opsize << 16) | (opsize << 24) | CRYPTO_PRE_CAL | CRYPTO_START; //pre_calc

	while ((CRYPTO_CASR & CRYPTO_DONE) != CRYPTO_DONE);
	cpu_read_data(R2, CRYPTO_CAAR, eccLen);


	//r = (y1+hash)%N
	EccBig123(y1, interHash, signR, eccLen, CRYPTO_ADD);

	EccAssign(y1, 0x1, ECC_BIGINT32_MAXLEN);
	//s = prikey+1
	EccBig123(y1, prikey, signS, eccLen, CRYPTO_ADD);

	//s = s ^ -1
	EccAssign(y1, 0x2, ECC_BIGINT32_MAXLEN);
	EccBig123(eccN, y1, y1, eccLen, CRYPTO_SUB);
	PowMod(signS, y1, R2, eccLen, eccLen, signS);
	CRYPTO_CACR = 0x80008000 | (opsize << 16) | (opsize << 24) | CRYPTO_PRE_CAL;
	//y1 = R*prikey
	EccBig123(R2, signR, 0, eccLen, CRYPTO_MONT_MULT);
	EccBig123(0, prikey, y1, eccLen, CRYPTO_MONT_MULT);
	//y1 = random-y1
	EccBig123(random_data, y1, 0, eccLen, CRYPTO_SUB);
	//s = s * y1
	EccBig123(0, R2, 0, eccLen, CRYPTO_MONT_MULT);
	EccBig123(0, signS, signS, eccLen, CRYPTO_MONT_MULT);

}

void SM2_Sign_WithIDA(unsigned char *ida, unsigned int entla,
					  unsigned char *msg, unsigned int msgLen,
					  unsigned int *random_data, unsigned int *prikey,
					  unsigned int *Px, unsigned int *Py,
					  unsigned int *Gx, unsigned int *Gy,
					  unsigned int *eccP, unsigned int *eccA,
					  unsigned int *eccB, unsigned int *eccN, unsigned int eccLen,
					  unsigned int *signR, unsigned int *signS,
					  unsigned int *data_cache)
{

	unsigned int hash[8];
	sm2_calZA(ida, entla, eccA, eccB, Gx, Gy, Px, Py, eccLen, hash);

	sm2_calHash(hash, msg, msgLen, hash);

	unsigned int i, temp;
	//convret hash result to bignum
	for (i = 0; i < 4; i++)
	{
		temp = Reverse(hash[7 - i]);
		hash[7 - i] = Reverse(hash[i]);
		hash[i] = temp;
	}

	SM2_Sign(hash, 8, random_data, prikey, Gx, Gy, eccP, eccA, eccN, eccLen, signR, signS, data_cache);
}


unsigned char SM2_Verify(unsigned int *hash, unsigned int hashLen,
						 unsigned int *Px, unsigned int *Py,
						 unsigned int *Gx, unsigned int *Gy,
						 unsigned int *eccP, unsigned int *eccA,
						 unsigned int *eccN, unsigned int eccLen,
						 unsigned int *signR, unsigned int *signS,
						 unsigned int *data_cache)
{


	unsigned int *point_mul_cache = data_cache;
	unsigned int *x1 = data_cache + ECC_BIGINT32_MAXLEN * 12;
	unsigned int *y1 = data_cache + ECC_BIGINT32_MAXLEN * 13;
	unsigned int *x2 = data_cache + ECC_BIGINT32_MAXLEN * 14;
	unsigned int *y2 = data_cache + ECC_BIGINT32_MAXLEN * 15;
	unsigned int *t = data_cache + ECC_BIGINT32_MAXLEN * 16;

	unsigned int i;
	unsigned int opsize = eccLen - 1;
	EccAssign(t, 0x0, ECC_BIGINT32_MAXLEN);

    CRYPTO_CACR = 0x80000000;
	cpu_write_data(CRYPTO_CANR, eccN, eccLen);
	CRYPTO_CACR = 0x80008000 | (opsize << 16) | (opsize << 24) | CRYPTO_NOT_R2 | CRYPTO_PRE_CAL | CRYPTO_START; //pre_calc

	while ((CRYPTO_CASR & CRYPTO_DONE) != CRYPTO_DONE);

	//t = R + S
	EccBig123(signS, signR, t, eccLen, CRYPTO_ADD);

	PointMult(signS, eccLen, Gx, Gy, x1, y1, eccP, eccA, eccLen, point_mul_cache);
	PointMult(t, eccLen, Px, Py, x2, y2, eccP, eccA, eccLen, point_mul_cache);


	ECC_PointAdd(x2,y2,x1,y1,x1,y1,eccP,eccA,eccLen,point_mul_cache);
	//y1 = x1 mod N
	BigNumMod(x1, eccLen, eccN, eccLen, y1, point_mul_cache);

	BigNumMod(hash, hashLen, eccN, eccLen, t, point_mul_cache);

	cpu_write_data(CRYPTO_CANR, eccN, eccLen);

	EccBig123(y1, t, t, eccLen, CRYPTO_ADD);

	for (i = 0; i < eccLen; i++)
	{
		if (t[i] != signR[i])
			return 1;
	}

	return 0;
}


unsigned char SM2_Verify_WithIDA(unsigned char *ida, unsigned int entla,
								 unsigned char *msg, unsigned int msgLen,
								 unsigned int *Px, unsigned int *Py,
								 unsigned int *Gx, unsigned int *Gy,
								 unsigned int *eccP, unsigned int *eccA,
								 unsigned int *eccB, unsigned int *eccN, unsigned int eccLen,
								 unsigned int *signR, unsigned int *signS,
								 unsigned int *data_cache)
{
	unsigned int hash[8];
	sm2_calZA(ida, entla, eccA, eccB, Gx, Gy, Px, Py, eccLen, hash);

	sm2_calHash(hash, msg, msgLen, hash);

	unsigned int i, temp;
	//convret hash result to bignum
	for (i = 0; i < 4; i++)
	{
		temp = Reverse(hash[7 - i]);
		hash[7 - i] = Reverse(hash[i]);
		hash[i] = temp;
	}
	return SM2_Verify(hash, 8, Px, Py, Gx, Gy, eccP, eccA, eccN, eccLen, signR, signS, data_cache);
}

void ECDSA_Sign(unsigned char *msg, unsigned int msgLen,
				unsigned int *random_data, unsigned int *prikey,
				unsigned int *Gx, unsigned int *Gy,
				unsigned int *eccP, unsigned int *eccA,
				unsigned int *eccN, unsigned int eccLen,
				unsigned int *signR, unsigned int *signS, unsigned int *data_cache)
{


	unsigned int *point_mul_cache = data_cache;
	unsigned int *x1 = data_cache + ECC_BIGINT32_MAXLEN * 12;
	unsigned int *y1 = data_cache + ECC_BIGINT32_MAXLEN * 13;
	unsigned int *R2 = data_cache + ECC_BIGINT32_MAXLEN * 14;
	unsigned int *interHash = data_cache + ECC_BIGINT32_MAXLEN * 15;

	unsigned int opsize = eccLen - 1;
	EccAssign(x1, 0x0, ECC_BIGINT32_MAXLEN);
	EccAssign(y1, 0x0, ECC_BIGINT32_MAXLEN);
	EccAssign(R2, 0x0, ECC_BIGINT32_MAXLEN);

	EccAssign(y1, 0x0, ECC_BIGINT32_MAXLEN);
	sha_init(MODE_SHA256);
	sha_final(msg, msgLen, x1);

	unsigned int i;
	//convret hash result to bignum
	for (i = 0; i < 8; i++)
	{
		y1[i] = Reverse(x1[7 - i]);
	}
	BigNumMod(y1, 8, eccN, eccLen, interHash, point_mul_cache);


	PointMult(random_data, eccLen, Gx, Gy, x1, y1, eccP, eccA, eccLen, point_mul_cache);

	BigNumMod(x1, eccLen, eccN, eccLen, y1, point_mul_cache);
	Eccint32MovBig(signR, y1, eccLen);
	cpu_write_data(CRYPTO_CANR, eccN, eccLen);
	CRYPTO_CACR = 0x80008000 | (opsize << 16) | (opsize << 24) | CRYPTO_PRE_CAL | CRYPTO_START; //pre_calc

	while ((CRYPTO_CASR & CRYPTO_DONE) != CRYPTO_DONE);
	cpu_read_data(R2, CRYPTO_CAAR, eccLen);


	//s = k ^ -1
	EccAssign(y1, 0x2, ECC_BIGINT32_MAXLEN);
	EccBig123(eccN, y1, y1, eccLen, CRYPTO_SUB);
	PowMod(random_data, y1, R2, eccLen, eccLen, signS);
	CRYPTO_CACR = 0x80008000 | (opsize << 16) | (opsize << 24) | CRYPTO_PRE_CAL;
	//y1 = R*prikey
	EccBig123(R2, signR, 0, eccLen, CRYPTO_MONT_MULT);
	EccBig123(0, prikey, 0, eccLen, CRYPTO_MONT_MULT);
	//y1 = hash+y1
	EccBig123(0, interHash, y1, eccLen, CRYPTO_ADD);
	//s = s * y1
	EccBig123(0, R2, 0, eccLen, CRYPTO_MONT_MULT);
	EccBig123(0, signS, signS, eccLen, CRYPTO_MONT_MULT);

}

unsigned char ECDSA_Verify(unsigned char *msg, unsigned int msgLen,
						   unsigned int *Px, unsigned int *Py,
						   unsigned int *Gx, unsigned int *Gy,
						   unsigned int *eccP, unsigned int *eccA,
						   unsigned int *eccN, unsigned int eccLen,
						   unsigned int *signR, unsigned int *signS,
						   unsigned int *data_cache)
{

	unsigned int *point_mul_cache = data_cache;
	unsigned int *x1 = data_cache + ECC_BIGINT32_MAXLEN * 12;
	unsigned int *y1 = data_cache + ECC_BIGINT32_MAXLEN * 13;
	unsigned int *x2 = data_cache + ECC_BIGINT32_MAXLEN * 14;
	unsigned int *y2 = data_cache + ECC_BIGINT32_MAXLEN * 15;
	unsigned int *u1 = data_cache + ECC_BIGINT32_MAXLEN * 16;
	unsigned int *u2 = x2;
	unsigned int *R2 = y2;
	unsigned int i;
	unsigned int opsize = eccLen - 1;


	EccAssign(y1, 0x0, ECC_BIGINT32_MAXLEN);
	sha_init(MODE_SHA256);
	sha_final(msg, msgLen, x1);
	//convret hash result to bignum
	for (i = 0; i < 8; i++)
	{
		y1[i] = Reverse(x1[7 - i]);
	}

	BigNumMod(y1, 8, eccN, eccLen, x1, point_mul_cache);

	EccAssign(u1, 0x0, ECC_BIGINT32_MAXLEN);
	EccAssign(u2, 0x0, ECC_BIGINT32_MAXLEN);
	CRYPTO_CACR = 0x80000000;
	cpu_write_data(CRYPTO_CANR, eccN, eccLen);
	CRYPTO_CACR = 0x80008000 | (opsize << 16) | (opsize << 24) | CRYPTO_PRE_CAL | CRYPTO_START; //pre_calc

	while ((CRYPTO_CASR & CRYPTO_DONE) != CRYPTO_DONE);
	cpu_read_data(R2, CRYPTO_CAAR, eccLen);



	//u1 = s^-1
	EccAssign(y1, 0x2, ECC_BIGINT32_MAXLEN);
	EccBig123(eccN, y1, y1, eccLen, CRYPTO_SUB);
	PowMod(signS, y1, R2, eccLen, eccLen, u1);
	CRYPTO_CACR = 0x80008000 | (opsize << 16) | (opsize << 24) | CRYPTO_PRE_CAL;
	//u2 = u1 * signR
	EccBig123(R2, u1, 0, eccLen, CRYPTO_MONT_MULT);
	EccBig123(0, signR, u2, eccLen, CRYPTO_MONT_MULT);

	//u1 = u1 * msg
	EccBig123(R2, u1, 0, eccLen, CRYPTO_MONT_MULT);
	EccBig123(0, x1, u1, eccLen, CRYPTO_MONT_MULT);


	PointMult(u1, eccLen, Gx, Gy, x1, y1, eccP, eccA, eccLen, point_mul_cache);
	PointMult(u2, eccLen, Px, Py, x2, y2, eccP, eccA, eccLen, point_mul_cache);


	ECC_PointAdd(x1, y1, x2, y2, x1, y1, eccP, eccA, eccLen, point_mul_cache);

	BigNumMod(x1, eccLen, eccN, eccLen, y1, point_mul_cache);

	for (i = 0; i < eccLen; i++)
	{
		if (y1[i] != signR[i])
			return 1;
	}

	return 0;
}


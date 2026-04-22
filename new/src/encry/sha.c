#include "sha.h"
#include "crypto.h"


unsigned int hash_msg[16];
unsigned int hashLength = 0;
unsigned int hashMode = 0;
void sha_init(unsigned int mode)
{

	SHACSR = 0x02; //RESET
	SHACSR = mode | FIRSTB | SHA_START;
	hashMode = mode;
	hashLength = 0;
}

void sha_update(unsigned int *msg, unsigned int length)
{

	unsigned int i, j;
	hashLength = hashLength + length;
	for (i = 0; i < length; i = i + 64)
	{

		for (j = 0; j < 16; j++) SHAWDR = msg[j];

		while ((SHACSR & SHA_DONE) == 0);

		SHACSR = 0x02; //clear done
		SHACSR = hashMode | SHA_START;

		msg = msg + 16;
	}


}


void sha_final(unsigned char *msg, unsigned int length, unsigned int *result)
{

	unsigned int i;
	unsigned int *temp;
	hashLength = hashLength + length;
	while (length >= 64)
	{
		temp = (unsigned int *)msg;
		for (i = 0; i < 16; i++)
		{
			SHAWDR = temp[i];
		}
		while ((SHACSR & SHA_DONE) == 0);


		SHACSR = 0x02; //clear done
		SHACSR = hashMode | SHA_START;

		length = length - 64;
		msg = msg + 64;
	}

	unsigned char lastMsg[64];
	temp = (unsigned int *)lastMsg;
	if (length >= 56)
	{
		for (i = 0; i < length; i++) lastMsg[i] = msg[i];

		lastMsg[i] = 0x80;
		for (i = length + 1; i < 64; i++) lastMsg[i] = 0;
		for (i = 0; i < 16; i++)
		{
			SHAWDR = temp[i];
		}
		while ((SHACSR & SHA_DONE) == 0);

		SHACSR = 0x02; //clear done
		SHACSR = hashMode | SHA_START;

		for (i = 0; i < 60; i++) lastMsg[i] = 0;
	}
	else
	{
		for (i = 0; i < length; i++) lastMsg[i] = msg[i];
		lastMsg[i] = 0x80;

		for (i = length + 1; i < 60; i++) lastMsg[i] = 0;

	}
	hashLength = hashLength << 3;
	lastMsg[60] = hashLength >> 24;
	lastMsg[61] = hashLength >> 16;
	lastMsg[62] = hashLength >> 8;
	lastMsg[63] = hashLength;

	for (i = 0; i < 16; i++)
	{
		SHAWDR = temp[i];
	}
	while ((SHACSR & SHA_DONE) == 0);

	for (i = 0; i < 8; i++)
	{
		result[i] = SHARDR;
	}

}


void sha_raw(unsigned char *msg, unsigned int length, unsigned int mode, unsigned int *result)
{
	sha_init(mode);
	sha_final(msg, length, result);
}


void hash_deal_block(unsigned int *block)
{
	unsigned int i;
	for (i = 0; i < 16; i++)
	{
		SHAWDR = block[i];
	}
	while ((SHACSR & SHA_DONE) == 0);


	SHACSR = 0x02; //clear done
	SHACSR = hashMode | SHA_START;

	hashLength = hashLength + 64;
}

unsigned int sm2_deal_value(unsigned char *hash_msg_char, unsigned int used_length,
							unsigned char *ecc_value, unsigned int eccLen)
{
	unsigned int i;
	for (i = 0; i < eccLen; i++)
	{
		hash_msg_char[used_length] = ecc_value[eccLen - 1 - i];
		used_length = used_length + 1;
		if (used_length == 64)
		{
			hash_deal_block((unsigned int *)hash_msg_char);
			used_length = 0;
		}
	}
	return used_length;
}

void sm2_calZA(unsigned char *ida, unsigned short entla,
			   unsigned int *eccA, unsigned int *eccB,
			   unsigned int *Gx, unsigned int *Gy,
			   unsigned int *Px, unsigned int *Py,
			   unsigned int eccLen,
			   unsigned int *Za)
{

	sha_init(MODE_SM3);
	unsigned int i;
	unsigned int used_length = 0;
	unsigned char *hash_msg_char = (unsigned char *)hash_msg;

	hash_msg_char[0] = entla >> 5;
	hash_msg_char[1] = entla << 3;
	if (entla >= 56)
	{
		for (i = 0; i < 56; i++)
		{
			hash_msg_char[i + 2] = ida[i];
		}
		hash_deal_block(hash_msg);
		entla = entla - 56;
		while (entla >= 64)
		{
			hash_deal_block((unsigned int *)ida);
			ida = ida + 64;
			entla = entla - 64;
		}
	}
	else
	{
		for (i = 0; i < entla; i++)
		{
			hash_msg_char[i + 2] = ida[i];
		}
		used_length = 2 + entla;
	}

	eccLen = eccLen << 2;
	used_length = sm2_deal_value(hash_msg_char, used_length, (unsigned char *)eccA, eccLen);
	used_length = sm2_deal_value(hash_msg_char, used_length, (unsigned char *)eccB, eccLen);
	used_length = sm2_deal_value(hash_msg_char, used_length, (unsigned char *)Gx, eccLen);
	used_length = sm2_deal_value(hash_msg_char, used_length, (unsigned char *)Gy, eccLen);
	used_length = sm2_deal_value(hash_msg_char, used_length, (unsigned char *)Px, eccLen);
	used_length = sm2_deal_value(hash_msg_char, used_length, (unsigned char *)Py, eccLen);

	hashLength = hashLength + used_length;
	hash_msg_char[used_length] = 0x80;
	if (used_length >= 56)
	{

		for (i = used_length + 1; i < 64; i++) hash_msg_char[i] = 0;

		for (i = 0; i < 16; i++)
		{
			SHAWDR = hash_msg[i];
		}
		while ((SHACSR & SHA_DONE) == 0);

		SHACSR = 0x02; //clear done
		SHACSR = hashMode | SHA_START;

		for (i = 0; i < 60; i++) hash_msg_char[i] = 0;
	}
	else
	{

		for (i = used_length + 1; i < 60; i++) hash_msg_char[i] = 0;

	}
	hashLength = hashLength << 3;
	hash_msg_char[60] = hashLength >> 24;
	hash_msg_char[61] = hashLength >> 16;
	hash_msg_char[62] = hashLength >> 8;
	hash_msg_char[63] = hashLength;

	for (i = 0; i < 16; i++)
	{
		SHAWDR = hash_msg[i];
	}
	while ((SHACSR & SHA_DONE) == 0);

	for (i = 0; i < 8; i++)
	{
		Za[i] = SHARDR;
	}
}

void sm2_calHash(unsigned int *Za, unsigned char *msg, unsigned int msgLen,
				 unsigned int *hash)
{
	unsigned int i;
	sha_init(MODE_SM3);
	for (i = 0; i < 8; i++) hash_msg[i] = Za[i];
	unsigned int *temp = (unsigned int *)msg;

	if (msgLen >= 32)
	{
		for (i = 0; i < 8; i++)
		{
			hash_msg[i + 8] = temp[i];
		}
		msg = msg + 32;
		msgLen = msgLen - 32;

		hash_deal_block(hash_msg);

		sha_final(msg, msgLen, hash);
	}
	else
	{
		unsigned char *hash_msg_char = (unsigned char *)(hash_msg + 8);
		for (i = 0; i < msgLen; i++)
		{
			hash_msg_char[i] = msg[i];
		}

		sha_final((unsigned char *)hash_msg, msgLen + 32, hash);
	}

}

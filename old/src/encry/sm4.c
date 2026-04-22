#include "sm4.h"



void sms_set_key(unsigned int *key, unsigned int *iv, unsigned int en_de, unsigned int mode)
{
	SMSCSR = SM4_RESET;
	SMSCSR = 0;
	SMSCSR = SM4_MEM | en_de | mode;

	SMSKIN3R = key[0];
	SMSKIN2R = key[1];
	SMSKIN1R = key[2];
	SMSKIN0R = key[3];

	while ((SMSCSR & SM4_KEY_RDY) != SM4_KEY_RDY);
	SMSCSR |= SM4_KEY_RDY;
	if (mode == SM4_CBC)
	{
		SMSIV3R	= iv[0];
		SMSIV2R	= iv[1];
		SMSIV1R	= iv[2];
		SMSIV0R	= iv[3];
	}

}

void sm4_cpu_cal(unsigned int *data_in, unsigned int *data_out)
{

	SMSDIN3R = data_in[0];
	SMSDIN2R = data_in[1];
	SMSDIN1R = data_in[2];
	SMSDIN0R = data_in[3];

	while ((SMSCSR & SM4_DONE) == 0);
	SMSCSR |= SM4_DONE;
	data_out[0] = SMSDIN3R;
	data_out[1] = SMSDIN2R;
	data_out[2] = SMSDIN1R;
	data_out[3] = SMSDIN0R;


}

void sm4_en_de(unsigned char *key, unsigned char *iv, unsigned char *data_in, unsigned char *data_out, unsigned int length, unsigned int mode, unsigned int en_de)
{

	unsigned int i;

	sms_set_key((unsigned int *)key, (unsigned int *)iv, en_de, mode);

	for (i = 0; i < length; i = i + 16)
	{
		sm4_cpu_cal((unsigned int *)data_in, (unsigned int *)data_out);
		data_in = data_in + 16;
		data_out = data_out + 16;
	}


}


#include "aes.h"

void aes_cbc_set_key(unsigned int *cbc_key, unsigned int keysize, unsigned int *cbc_iv, unsigned int cmode)
{
	AES_CSR = AESRST;
	if (keysize == 4)
	{
		AES_CSR		= AES_CBC | AESKEYMODE00 | AESNEW_KEY | cmode;
		AES_KEY0        = cbc_key[0];
		AES_KEY1        = cbc_key[1];
		AES_KEY2        = cbc_key[2];
		AES_KEY3        = cbc_key[3];
	}
	else if (keysize == 6)
	{
		AES_CSR		= AES_CBC | AESKEYMODE01 | AESNEW_KEY | cmode;
		AES_KEY0        = cbc_key[0];
		AES_KEY1        = cbc_key[1];
		AES_KEY2        = cbc_key[2];
		AES_KEY3        = cbc_key[3];
		AES_KEY4        = cbc_key[4];
		AES_KEY5        = cbc_key[5];
	}
	else
	{
		AES_CSR		= AES_CBC | AESKEYMODE10 | AESNEW_KEY | cmode;
		AES_KEY0        = cbc_key[0];
		AES_KEY1        = cbc_key[1];
		AES_KEY2        = cbc_key[2];
		AES_KEY3        = cbc_key[3];
		AES_KEY4        = cbc_key[4];
		AES_KEY5        = cbc_key[5];
		AES_KEY6        = cbc_key[6];
		AES_KEY7        = cbc_key[7];
	}


	AES_IV_0	= cbc_iv[0];
	AES_IV_1	= cbc_iv[1];
	AES_IV_2	= cbc_iv[2];
	AES_IV_3	= cbc_iv[3];



}

void aes_cbc_en_de(unsigned int *data_in, unsigned int datalen, unsigned int *data_out)
{
	unsigned int i;
	for (i = 0; i < datalen; i = i + 16)
	{

		AESDIOR	= data_in[0];
		AESDIOR	= data_in[1];
		AESDIOR	= data_in[2];
		AESDIOR	= data_in[3];

		while ((AES_CSR & AESDONE) != AESDONE);

		data_out[0] = AESDIOR;
		data_out[1] = AESDIOR;
		data_out[2] = AESDIOR;
		data_out[3] = AESDIOR;

		AES_CSR	|= AESDONE;
		data_in  = data_in + 4;
		data_out = data_out + 4;

	}

}

void aes_cbc_encrypt(unsigned int *key, unsigned int keysize, unsigned int *cbc_iv, unsigned int *data_in, unsigned int datalen, unsigned int *data_out)
{

	aes_cbc_set_key(key, keysize, cbc_iv, 0);
	aes_cbc_en_de(data_in, datalen, data_out);
}

void aes_cbc_decrypt(unsigned int *key, unsigned int keysize, unsigned int *cbc_iv, unsigned int *data_in, unsigned int datalen, unsigned int *data_out)
{

	aes_cbc_set_key(key, keysize, cbc_iv, AESCMODE);
	aes_cbc_en_de(data_in, datalen, data_out);
}


void aes_gcm_init(unsigned int *gcm_key, unsigned int keysize, unsigned int *gcm_iv,	unsigned int ivlen,
				  unsigned int *gcm_add,	unsigned int addlen)
{
	unsigned int i;
	AES_CSR = AESRST;
	AES_IVLEN0      = ivlen;
	AES_ADDLEN0     = addlen;
	if (keysize == 4)
	{
		AES_CSR		= AES_GCM_INIT | AESKEYMODE00 | AESNEW_KEY;
		AES_KEY0        = gcm_key[0];
		AES_KEY1        = gcm_key[1];
		AES_KEY2        = gcm_key[2];
		AES_KEY3        = gcm_key[3];
	}
	else if (keysize == 6)
	{
		AES_CSR		= AES_GCM_INIT | AESKEYMODE01 | AESNEW_KEY;
		AES_KEY0        = gcm_key[0];
		AES_KEY1        = gcm_key[1];
		AES_KEY2        = gcm_key[2];
		AES_KEY3        = gcm_key[3];
		AES_KEY4        = gcm_key[4];
		AES_KEY5        = gcm_key[5];
	}
	else
	{
		AES_CSR		= AES_GCM_INIT | AESKEYMODE10 | AESNEW_KEY;
		AES_KEY0        = gcm_key[0];
		AES_KEY1        = gcm_key[1];
		AES_KEY2        = gcm_key[2];
		AES_KEY3        = gcm_key[3];
		AES_KEY4        = gcm_key[4];
		AES_KEY5        = gcm_key[5];
		AES_KEY6        = gcm_key[6];
		AES_KEY7        = gcm_key[7];
	}


	if (ivlen == 12)
	{
		AES_IV_0	= gcm_iv[0];
		AES_IV_1	= gcm_iv[1];
		AES_IV_2	= gcm_iv[2];

	}
	else
	{
		for (i = 0; i < ivlen; i = i + 16)
		{
			AES_IV_0	= gcm_iv[0];
			AES_IV_1	= gcm_iv[1];
			AES_IV_2	= gcm_iv[2];
			AES_IV_3	= gcm_iv[3];
			gcm_iv = gcm_iv + 4;
			while ((AES_CSR & AESDONE) != AESDONE);
			AES_CSR	|= AESDONE;
		}
	}



	for (i = 0; i < addlen; i = i + 16)
	{
		AESDIOR		= gcm_add[0];
		AESDIOR		= gcm_add[1];
		AESDIOR		= gcm_add[2];
		AESDIOR		= gcm_add[3];
		gcm_add = gcm_add + 4;
		while ((AES_CSR & AESDONE) != AESDONE);
		AES_CSR	|= AESDONE;
	}

}

void aes_gcm_en_de(unsigned int keysize, unsigned int *data_in, unsigned int datalen, unsigned int *data_out, unsigned int *mac, unsigned int cmode)
{
	unsigned int i;
	if (keysize == 4)
		AES_CSR		= AES_GCM_EN_DE | AESKEYMODE00 | cmode;
	else if (keysize == 6)
		AES_CSR		= AES_GCM_EN_DE | AESKEYMODE01 | cmode;
	else
		AES_CSR		= AES_GCM_EN_DE | AESKEYMODE10 | cmode;


	AES_MSGLEN0	= datalen;
	for (i = 0; i < datalen; i = i + 16)
	{

		AESDIOR	= data_in[0];
		AESDIOR	= data_in[1];
		AESDIOR	= data_in[2];
		AESDIOR	= data_in[3];

		while ((AES_CSR & AESDONE) != AESDONE);

		data_out[0] = AESDIOR;
		data_out[1] = AESDIOR;
		data_out[2] = AESDIOR;
		data_out[3] = AESDIOR;

		AES_CSR	|= AESDONE;
		data_in  = data_in + 4;
		data_out = data_out + 4;

	}

	mac[0] = AES_GCM_MAC0;
	mac[1] = AES_GCM_MAC1;
	mac[2] = AES_GCM_MAC2;
	mac[3] = AES_GCM_MAC3;

}

void aes_gcm_encrypt(unsigned int *key, unsigned int keysize, unsigned int *gcm_iv, unsigned int iv_len, unsigned int *gcm_add, unsigned int add_len,
					 unsigned int *data_in, unsigned int datalen, unsigned int *data_out, unsigned int *mac)
{

	aes_gcm_init(key, keysize, gcm_iv, iv_len, gcm_add, add_len);
	aes_gcm_en_de(keysize, data_in, datalen, data_out, mac, 0);
}

void aes_gcm_decrypt(unsigned int *key, unsigned int keysize, unsigned int *gcm_iv, unsigned int iv_len, unsigned int *gcm_add, unsigned int add_len,
					 unsigned int *data_in, unsigned int datalen, unsigned int *data_out, unsigned int *mac)
{

	aes_gcm_init(key, keysize, gcm_iv, iv_len, gcm_add, add_len);
	aes_gcm_en_de(keysize, data_in, datalen, data_out, mac, AESCMODE);
}

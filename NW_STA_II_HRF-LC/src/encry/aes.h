#ifndef AES_Y_H
#define AES_Y_H



#define AES_BASE              0x40600000UL

#define AESDIOR               (*(volatile unsigned int *)(AES_BASE))

#define AES_KEY0              (*(volatile unsigned int *)(AES_BASE+0x10))
#define AES_KEY1              (*(volatile unsigned int *)(AES_BASE+0x14))
#define AES_KEY2              (*(volatile unsigned int *)(AES_BASE+0x18))
#define AES_KEY3              (*(volatile unsigned int *)(AES_BASE+0x1c))
#define AES_KEY4              (*(volatile unsigned int *)(AES_BASE+0x20))
#define AES_KEY5              (*(volatile unsigned int *)(AES_BASE+0x24))
#define AES_KEY6              (*(volatile unsigned int *)(AES_BASE+0x28))
#define AES_KEY7              (*(volatile unsigned int *)(AES_BASE+0x2c))
#define AES_CSR               (*(volatile unsigned int *)(AES_BASE+0x30))
#define AES_COUNTER_3	      (*(volatile unsigned int *)(AES_BASE+0x34))
#define AES_COUNTER_2	      (*(volatile unsigned int *)(AES_BASE+0x38))
#define AES_COUNTER_1	      (*(volatile unsigned int *)(AES_BASE+0x3c))
#define AES_COUNTER_0	      (*(volatile unsigned int *)(AES_BASE+0x40))

#define AES_IV_0	      (*(volatile unsigned int *)(AES_BASE+0x34))
#define AES_IV_1	      (*(volatile unsigned int *)(AES_BASE+0x38))
#define AES_IV_2	      (*(volatile unsigned int *)(AES_BASE+0x3c))
#define AES_IV_3	      (*(volatile unsigned int *)(AES_BASE+0x40))

#define AES_MSGLEN0	      (*(volatile unsigned int *)(AES_BASE+0x44))
#define AES_MSGLEN1	      (*(volatile unsigned int *)(AES_BASE+0x48))
#define AES_ADDLEN0	      (*(volatile unsigned int *)(AES_BASE+0x4C))
#define AES_ADDLEN1	      (*(volatile unsigned int *)(AES_BASE+0x50))
#define AES_IVLEN0	      (*(volatile unsigned int *)(AES_BASE+0x54))
#define AES_IVLEN1	      (*(volatile unsigned int *)(AES_BASE+0x58))

#define AES_GCM_MAC0	      (*(volatile unsigned int *)(AES_BASE+0x5C))
#define AES_GCM_MAC1	      (*(volatile unsigned int *)(AES_BASE+0x60))
#define AES_GCM_MAC2	      (*(volatile unsigned int *)(AES_BASE+0x64))
#define AES_GCM_MAC3	      (*(volatile unsigned int *)(AES_BASE+0x68))

#define AESIE				0x00000080
#define AESRST				0x00000040
#define AESMSEL				0x00000020
#define AES_CBC				0x00000800
#define AES_CTR				0x00000020
#define AES_GCM_INIT			0x00000200
#define AES_GCM_EN_DE			0x00000400
#define AESNEW_KEY			0x00000100
#define AESDONE				0x00000010

#define AESBUSY				0x00000008
#define AESKEYMODE00			0x00000000
#define AESKEYMODE01			0x00000002
#define AESKEYMODE10			0x00000004
#define AESCMODE			0x00000001


#define KEY_MODE_128	      4
#define KEY_MODE_192	      6
#define KEY_MODE_256	      8

extern void aes_gcm_encrypt(unsigned int *key, unsigned int keysize, unsigned int *gcm_iv, unsigned int iv_len, unsigned int *gcm_add, unsigned int add_len,
							unsigned int *data_in, unsigned int datalen, unsigned int *data_out, unsigned int *mac);

extern void aes_gcm_decrypt(unsigned int *key, unsigned int keysize, unsigned int *gcm_iv, unsigned int iv_len, unsigned int *gcm_add, unsigned int add_len,
							unsigned int *data_in, unsigned int datalen, unsigned int *data_out, unsigned int *mac);


extern void aes_cbc_encrypt(unsigned int *key, unsigned int keysize, unsigned int *cbc_iv, unsigned int *data_in, unsigned int datalen, unsigned int *data_out);
extern void aes_cbc_decrypt(unsigned int *key, unsigned int keysize, unsigned int *cbc_iv, unsigned int *data_in, unsigned int datalen, unsigned int *data_out);
#endif

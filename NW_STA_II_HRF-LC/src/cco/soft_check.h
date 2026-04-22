#ifndef __SOFT_CHECK_H__
#define __SOFT_CHECK_H__
#ifdef __cplusplus
extern "C"
{
#endif


#include "SHA1.h"


typedef struct{
	uint32_t cpu_num:8;
	uint32_t factor:16;
	uint32_t :3;
	uint32_t day:5;

	uint32_t mon:4;
	uint32_t year:12;
	uint32_t hard_ver:16;
	uint16_t soft_ver;
	uint8_t  mcu_len;
}soft_info_s;

int GetSoftInfo(char *data);
void GetSoftUpdateFlag(void);
void GetCodeSha1(uint32_t start_addr, uint16_t len, uint8_t *sha1_data);
#ifdef __cplusplus
}
#endif
#endif /* __SOFT_CHECK_H__ */


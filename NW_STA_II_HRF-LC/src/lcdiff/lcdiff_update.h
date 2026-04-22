#ifndef _LCDIFF_UPDATE_H_
#define _LCDIFF_UPDATE_H_
#include "common_includes.h"
#include "bps_flash.h"




#define LCDIFF_CRC_POLY 0x82F63B78 // 标准CRC-32C(Castagnoli)多项式0x1EDC6F41,位反转0x82F63B78
#define DEFAULT_CRC_POLY 0xEDB88320 

#define TUZ_DICT_SIZE 256
#define TUZ_CACHE_SIZE 64


typedef	struct 
{
	uint32_t app_addr;
	uint32_t app_size;
    
	uint32_t download_addr;
	uint32_t download_size;

	uint32_t factory_addr;
	uint32_t factory_size;
    
}flash_addr_t;


typedef struct 
{
	char magic[8];
	uint32_t old_size;
	uint32_t new_size;
	uint32_t patch_size;
	uint32_t old_crc;
	uint32_t new_crc;
	uint32_t patch_crc;
} patch_header_t;
	


int lcdiff_update(uint32_t download_addr, uint32_t download_area_size);

#endif

/*
*
*/
#ifndef __BSP_FLASH_H__
#define __BSP_FLASH_H__
#include "gsmcu_flash.h"
//flash boot
#define FLASH_AHB_ADDR          0x13000000
#define DATA_FLASH_PAGE_SIZE    0x100UL         //256
#define DATA_FLASH_SECTION_SIZE 0x1000UL        //4K

#define DATA_FLASH_START_ADDR   0x3000UL

//#define DATA_FLASH_END_ADDR     (0x200000UL-14*DATA_FLASH_SECTION_SIZE)      //[0..2M)

#define MAX_DATA_FLASH_SIZE	    DATA_FLASH_END_ADDR

#define DataFlashClose() FLASH_Close()
#define DataFlashInit()	FLASH_Init()

void EraseDataFlash(u32 start_addr, u32 dlen);
void WriteDataFlash(u32 start_addr, u8 * pbuf, u32 dlen);
void ReadDataFlash(u32 start_addr, u8 * pbuf, u32 dlen);

void GetDevFlashID();
unsigned char data_flash_write_straight( unsigned long addr, u8* data, unsigned long len );
unsigned char data_flash_write_straight_with_crc( unsigned long addr, u8* data, unsigned long len );
unsigned char data_flash_read_straight( unsigned long addr, u8* data, unsigned long len );
unsigned char data_flash_read_straight_with_crc( unsigned long addr, u8* data, unsigned long len ,u8 defult_data);
unsigned char data_flash_read_straight_with_crc_noadc( unsigned long addr, u8* data, unsigned long len ,u8 defult_data);
unsigned char data_flash_erase_straight( unsigned long addr, unsigned long len );
unsigned char data_flash_erase_straight_with_poweron( unsigned long addr, unsigned long len);
void Protection1MFlash(void);
void Unprotection1MFlash(void);
void Protection1MFlashNoVolatile(void);
u32 data_flash_read_flash_id();

#endif

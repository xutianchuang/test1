/*
*
*/
#ifndef __BSP_FLASH_H__
#define __BSP_FLASH_H__

#if DEV_CCO

#define DATA_FLASH_PAGE_SIZE    0x100UL         //256
#define DATA_FLASH_SECTION_SIZE 0x1000UL        //4K


#define DATA_FLASH_START_ADDR   0x3000UL

#ifdef FLASH_SIZE_1M
#define DATA_FLASH_END_ADDR     (0x100000UL-9*DATA_FLASH_SECTION_SIZE)      //[0..1M)
#define MAX_DATA_FLASH_SIZE	    (0x100000UL-0x3000)
#else
#define DATA_FLASH_END_ADDR     (0x200000UL-13*DATA_FLASH_SECTION_SIZE)      //[0..2M)
#define MAX_DATA_FLASH_SIZE	    (0x100000UL-0x3000)
#endif


#define DataFlashClose() FLASH_Reset();FLASH_Close()
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
#elif DEV_STA
//flash boot
#define FLASH_START_ADDR        (0x12000000UL)

#define FLASH_INFO_ADDRESS	FLASH_START_ADDR	//FLASH信息字段
#define FLASH_INTERRUPT_SIZE			0x200

#define FLASH_USER_FEATURE1_ADDRESS		(FLASH_START_ADDR + 0x01000)	//FLASH用户特征字1
#define FLASH_USER_FEATURE1_SIZE		0x1000							//FLASH用户特征字1占用空间

#define FLASH_USER_FEATURE2_ADDRESS		(FLASH_USER_FEATURE1_ADDRESS + FLASH_USER_FEATURE1_SIZE)	//FLASH用户特征字2
#define FLASH_USER_FEATURE2_SIZE		0x1000

#define FLASH_CODE1_ADDRESS				(FLASH_USER_FEATURE2_ADDRESS + FLASH_USER_FEATURE2_SIZE)	//代码1
#define FLASH_CODE1_SIZE 0x60000

#define FLASH_CODE2_ADDRESS				(FLASH_CODE1_ADDRESS + FLASH_CODE1_SIZE)	//代码2
#define FLASH_CODE2_SIZE 0x60000

#define FLASH_PARAM_ADDRESS				(FLASH_CODE2_ADDRESS + FLASH_CODE2_SIZE)	//参数
//#define FLASH_PARAM_SIZE 0x6000

#define ENABLE_PROTOCOL_FLASH
#define DATA_STA_ATTR_STORE_ADDR FLASH_PARAM_ADDRESS
#define DATA_STA_PARA_STORE_ADDR (DATA_STA_ATTR_STORE_ADDR + DATA_FLASH_SECTION_SIZE)
#define DATA_STA_ROUTE_STORE_ADDR (DATA_STA_PARA_STORE_ADDR + DATA_FLASH_SECTION_SIZE)
#define DATA_STA_ID_INFO_STORE_ADDR (DATA_STA_ROUTE_STORE_ADDR + DATA_FLASH_SECTION_SIZE)
#define DATA_POWER_OFF_STORE_ADDR (DATA_STA_ID_INFO_STORE_ADDR + DATA_FLASH_SECTION_SIZE)
#define DATA_PLL_TRIM_STORE_ADDR (DATA_POWER_OFF_STORE_ADDR + DATA_FLASH_SECTION_SIZE)


#define TRY_RUN_TIME_ADDRESS (DATA_PLL_TRIM_STORE_ADDR + DATA_FLASH_SECTION_SIZE) //试运行参数存放

#define DATA_FLASH_PAGE_SIZE 0x100
#define DATA_FLASH_SECTION_SIZE 0x1000
#define MAX_DATA_FLASH_SIZE	0xFFFFFF

#define DataFlashClose() FLASH_ResetAtFault();FLASH_Close()
#define DataFlashInit()	FLASH_Init()

void EraseDataFlash(u32 start_addr, u32 dlen);
void WriteDataFlash(u32 start_addr, u8 * pbuf, u32 dlen);
void ReadDataFlash(u32 start_addr, u8 * pbuf, u32 dlen);

void EraseFlash(u32 start_addr, u32 dlen);
void WriteFlash(u32 start_addr, u8 * pbuf, u32 dlen);
void ReadFlash(u32 start_addr, u8 * pbuf, u32 dlen);

#endif
#endif

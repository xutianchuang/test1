/*
*
*/
#ifndef __BSP_FLASH_H__
#define __BSP_FLASH_H__

#if DEV_CCO

#define DATA_FLASH_PAGE_SIZE    0x100UL         //256
#define DATA_FLASH_SECTION_SIZE 0x1000UL        //4K

#define DATA_FLASH_START_ADDR   0x3000UL      //[1024K+12K
#define DATA_FLASH_END_ADDR     (0x200000UL-DATA_FLASH_SECTION_SIZE)      //[0..2M)
#define MAX_DATA_FLASH_SIZE	    0xFD000UL       //DATA_FLASH_END_ADDR - DATA_FLASH_START_ADDR

#define DataFlashClose() FLASH_Close()
#define DataFlashInit()	FLASH_Init()

void EraseDataFlash(u32 start_addr, u32 dlen);
void WriteDataFlash(u32 start_addr, u8 * pbuf, u32 dlen);
void ReadDataFlash(u32 start_addr, u8 * pbuf, u32 dlen);

unsigned char data_flash_write_straight( unsigned long addr, u8* data, unsigned long len );
unsigned char data_flash_read_straight( unsigned long addr, u8* data, unsigned long len );
unsigned char data_flash_erase_straight( unsigned long addr, unsigned long len );

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
#define FLASH_CODE1_SIZE 0x3F000         //212K

#define FLASH_CODE2_ADDRESS				(FLASH_CODE1_ADDRESS + FLASH_CODE1_SIZE)	//代码2
#define FLASH_CODE2_SIZE 0x3F000         //212K
//到此代码段为+flash信息+code信息未为 516K 此段修改时需要解锁 需要改变code段大小时必须同步更改 flash保护字段大小!!!!!
#define FLASH_PARAM_ADDRESS				(FLASH_CODE2_ADDRESS + FLASH_CODE2_SIZE)	//参数
//#define FLASH_PARAM_SIZE 0x6000

#define ENABLE_PROTOCOL_FLASH
#define DATA_STA_ATTR_STORE_ADDR FLASH_PARAM_ADDRESS
#define DATA_STA_FIX_PARA_STORE_ADDR (DATA_STA_ATTR_STORE_ADDR + DATA_FLASH_SECTION_SIZE)
#define DATA_STA_ID_INFO_STORE_ADDR (DATA_STA_FIX_PARA_STORE_ADDR + DATA_FLASH_SECTION_SIZE*2)
#define DATA_PLL_TRIM_STORE_ADDR (DATA_STA_ID_INFO_STORE_ADDR + DATA_FLASH_SECTION_SIZE*2)

#define DATA_VER_INFO_STORE_ADDR (DATA_PLL_TRIM_STORE_ADDR + DATA_FLASH_SECTION_SIZE*2)
#define DATA_STA_ROUTE_STORE_ADDR (DATA_VER_INFO_STORE_ADDR + DATA_FLASH_SECTION_SIZE*2)
#define STA_REAL_TIME_ADDRESS (DATA_STA_ROUTE_STORE_ADDR+DATA_FLASH_SECTION_SIZE) 
#define DATA_STA_PARA_STORE_ADDR (STA_REAL_TIME_ADDRESS + DATA_FLASH_SECTION_SIZE)
#define DATA_POWER_OFF_STORE_ADDR (DATA_STA_PARA_STORE_ADDR + DATA_FLASH_SECTION_SIZE)
#define TRY_RUN_TIME_ADDRESS (DATA_POWER_OFF_STORE_ADDR + DATA_FLASH_SECTION_SIZE) //试运行参数存放
#define DATA_EXT_VER_INFO_STORE_ADDR (TRY_RUN_TIME_ADDRESS+DATA_FLASH_SECTION_SIZE)
#define DATA_RESET_CNT_STORE_ADDR (DATA_EXT_VER_INFO_STORE_ADDR+DATA_FLASH_SECTION_SIZE*2)

#define STA_READ_GRAPH_PARA   (DATA_RESET_CNT_STORE_ADDR+DATA_FLASH_SECTION_SIZE) 
#define STA_READ_GRAPH_DATA   (STA_READ_GRAPH_PARA+DATA_FLASH_SECTION_SIZE) //0x12008A000
#define STA_READ_GRAPH_DATA_SIZE    0x32000            //200K


#define DATA_FLASH_PAGE_SIZE 0x100
#define DATA_FLASH_SECTION_SIZE 0x1000
#define MAX_DATA_FLASH_SIZE	0xFFFFF

//sta使用最后的一个BOLCK进行刷新 上面定义时要确认不可越过这个区域
#define FLASH_FLUSH_ADDR (FLASH_START_ADDR+MAX_DATA_FLASH_SIZE+1-DATA_FLASH_SECTION_SIZE*16)


#define DataFlashClose() FLASH_ResetAtFault(); \
						FLASH_Close();\
						OS_CRITICAL_EXIT()
#define DataFlashInit()	CPU_SR_ALLOC();\
						OS_CRITICAL_ENTER();\
						FLASH_Init()

void EraseDataFlash(u32 start_addr, u32 dlen);
void WriteDataFlash(u32 start_addr, u8 * pbuf, u32 dlen);
void ReadDataFlash(u32 start_addr, u8 * pbuf, u32 dlen);

void EraseFlash(u32 start_addr, u32 dlen);
void WriteFlash(u32 start_addr, u8 * pbuf, u32 dlen);
void ReadFlash(u32 start_addr, u8 * pbuf, u32 dlen);


void Protection512KFlash(void);
void Unprotection512KFlash(void);
void Protection512KFlashNoVolatile(void);
#endif
#endif

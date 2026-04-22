#ifndef _STORAGE_H_
#define _STORAGE_H_

//#pragma pack(push,1)

//#define METER_TABLE_STATUS_OK	0x79328712
#define UP_NEED_PRO_FLASH    0x11223344
#define TMN_VER_INFO_FLAG    0x99990001

typedef enum 
{
	FRAM_FLUSH = 0x0,		
	FRAM_FETCH,	
	

	FRAM_BUTT
}e_fram_action;

typedef enum 
{
	FLASH_FLUSH = 0x0,		
	FLASH_FETCH,	
	FLASH_ERASE,
	FLASH_BUTT
}e_flash_action;

/* 到storage 消息类G?*/
enum MSG_TO_STOR
{
    MSG_STOR_NONE_CLEAR,
    MSG_STOR_PARA_CLEAR,  //参?初始化
    MSG_STOR_DATA_CLEAR,  //??区初始化  	
    MSG_STOR_ALL_CLEAR,   //电柄??初始化    
    MSG_STOR_CMD_BUTT,
};

typedef struct _db_addr_array_
{
	unsigned long 		addr;           //在??库?保存的地址
	unsigned long       usedSize;       //在??库?大G?
	unsigned long       allocSize;      //实际分配大G?
}DB_ADDR_ITEM, *P_DA_ADDR_ITEM;

typedef struct
{
	u32 write_flag;
#ifdef HPLC_CSG
    u8  chip_id[6];
    u8  module_id[24]; //资产编码
    u8  mac[6];
	u8  reserved[21];
#else
    u8  chip_id[346];
	u8  module_id[11];
	u8  mac[6];
	u8  reserved[16];
#endif
}ST_CCO_ID_TYPE;
typedef struct{
	u32 len;
	u32 flag;
}UPDATA_PARA;
typedef struct {
	u32 flash_addr;
	u32 item_flag;
	u32 erase_size;
	u32 data_size;
	u8 data[0];
}UPDATA_ITEM;
extern ST_CCO_ID_TYPE g_cco_id;
//#pragma pack(pop)

unsigned char	 SYS_STOR_Init(HANDLE h);
unsigned char	 SYS_STOR_proc(HANDLE h);
unsigned char	 SYS_STOR_ParamClear();
unsigned char	 SYS_STOR_DataClear();
unsigned char    SYS_STOR_GetUpdateUserInfo(P_FLASH_INFO_FIELD* ppFlash, P_FLASH_USER_CHARACTER_FIELD* ppUser, unsigned long* p_user_addr, u32* p_new_version);
unsigned long    SYS_STOR_GetUpdateAddr();
void             SYS_STOR_CCO_ID(void);
void             SYS_STOR_VER_INFO(void);
void             SYS_STOR_DEBUGMASK(u32 mask);
void             SYS_STOR_FREQ(u8 freq);
void			 SYS_STOR_BAUDRATE(u32 baudrate);
void             SYS_STOR_MAIN_NODE(u8 *mac);
void             SYS_STOR_WHITE_LIST_SWITCH(u8 en);
void             SYS_READ_WHITE_LIST_SWITCH(void);
void             SYS_STOR_METER(void);
void             SYS_STOR_CCO_ID(void);
void             SYS_STOR_VER_INFO(void);
void             SYS_READ_CCO_ID(void);
void             SYS_READ_VER_INFO(void);
void             SYS_STOR_OUTLIST_METER(void);
bool             SYS_READ_OUTLIST_METER(void);
void             SYS_STOR_EXT_VER_INFO(void);
void             SYS_READ_EXT_VER_INFO(void);
void             SYS_UP_ReWrite(UPDATA_PARA* para);
void             SYS_READ_AUTHEN(void);
void             SYS_STOR_AUTHEN(void);
void             SYS_READ_RELAY_RANGE(void);
void             SYS_STOR_RELAY_RANGE(void);
void             SYS_STRO_PLL(signed int trim);
void             SYS_READ_PLL(void);
void             SYS_STOR_COLLECT_INNET_MODE(u8 mode);
void             SYS_READ_COLLECT_INNET_MODE(void);
void             SYS_STOR_COMPARE_STA_INVER(void);
void             SYS_READ_COMPARE_STA_INVER(void);
void             SYS_STOR_ELEMENT_REPORT0_MODE(u8 mode);
void             SYS_READ_ELEMENT_REPORT0_MODE(void);
void             SYS_STOR_HRF_CHANNEL(void);
void             SYS_READ_HRF_CHANNEL(void);
void             SYS_STOR_A_PHASE_ERR(void);
void             SYS_READ_A_PHASE_ERR(void);
void             SYS_STOR_HRF_POWER_COMP(void);
void             SYS_READ_HRF_POWER_COMP(void);

#ifdef GDW_JIANGXI
void             SYS_STOR_NETWORK_MODE(void);
void             SYS_READ_NETWORK_MODE(void);
#endif
#ifdef GDW_ZHEJIANG
void SYS_STOR_BLACK_METER(void);
bool SYS_READ_BLACK_METER(void);
#endif //GDW_ZHEJIANG
void FlushAllFlash(void);
unsigned char dc_flush_fram(void *ptr, U32 len, U8 action);
extern unsigned char g_MeterAddrSuccess;
#endif

#ifndef _COMMON_INCLUDES_H_
#define _COMMON_INCLUDES_H_
#include <gsmcu_hal.h>
#include "timer.h"
#include "list.h"
#include "tmr_call.h"
#include "task_call.h"
#include "fsm.h"
#include "algorithm.h"
#include "map.h"

#define SYS_RESET_POS_0             0
#define SYS_RESET_PHY_INIT_ERR      1

#define SOFT_VERSION        (0x1301)
#define MAX_BITMAP_SIZE             256
//#define VERSIOM_VALUE       (0x20191101UL)
#define USER_FEATURE_CODE       (0x0123abcdUL)

#define FLASH_FEATURE_CODE       (0x0123abcdUL)
#if ZB202
#define FLASH_ID_CODE           (0x00154020UL)
#elif DM750
#define FLASH_ID_CODE           (0x0015400BUL)
#endif

//==========================================
#define DEFAULT_CODE_START_FLAG             (0x00)
#define CODE_SECTION1_FLAG                  (0x01234567)
#define CODE_SECTION2_FLAG                  (0x87654320)

//==========================================

#if defined(ZB204_CHIP)
#define CODE_NUM 2
#elif defined(ZB205_CHIP)
#define CODE_NUM 3
#endif

typedef struct
{
    unsigned long RearFileCrc32;
    unsigned long FontFileCrc32;
    unsigned long AplVersion;
	unsigned long CodeLength;
	//=============
	unsigned long FactoryCode;
}AplCommonParam_type;

typedef struct
{
    unsigned long FeatureCode;
    unsigned long Version;
    #if defined(ZB204_CHIP)
    unsigned long CopyToRamFlg;
	#endif
    unsigned long CodeSpiFlashStartAddr;
    unsigned long CodeSize;
    unsigned long CodeCheckSum;
    unsigned long UserSectionCheckSum;
}UserSection_Type;

typedef struct
{
    unsigned long FeatureCode;
    unsigned long FalshIdCode;
    unsigned long FlashBlockSize;
    unsigned long FlashBlockSum;
    unsigned long UserSectionBlockIndex[CODE_NUM];
        
    unsigned long XIPCommandWord;
    unsigned long FalshClockDivider;
    //unsigned long MISCControl;
    unsigned long FirstFlashRDSRCommand;
    unsigned long SecondFlashRDSRCommand;
    //unsigned long ThirdFlashRDSRCommand;
    unsigned long FlashWRSRCommand;

    unsigned long FlashOEBitN;
    unsigned long SPIFIODrvingStrength;
    unsigned long SPIReadCommand;
    unsigned long DummyCycleForSpiRealCommand;
    unsigned long FlashInfoSectionCheckSum;

}FlashInfoSectin_Type;



//试运行的参数
typedef struct
{
	u32 TryRunTime;
	u32 TryRunVersion;
	u32 FileSize;
	u32 FileCRC;
}TryRunParam;

//
typedef struct
{
	TryRunParam Param1;
	TryRunParam Param2;
	u32 CRC32;
}FlashTryRunParam;

//
extern UserSection_Type* CurrentCodeSection ;
extern u8 OldCodeSection;

extern TryRunParam  CurrentTryRunParam;

//检测启动段
void CheckCurrentCodeSection();

u32 CheckU32Sum(u32 *pd,u32 len);

UserSection_Type * Code2ndIsVaild(void);
#define SAFE_SYSTEM_RESET(rs)     BSP_safe_reset(rs);

//#define REBOOT_SYSTEM()   DebugRebootSystem()
#define REBOOT_SYSTEM()   RebootSystem()

//公共组件模块初始化
void CommonInit(void);

//重启
void RebootSystem(void);

//假重启(离网、解绑地址)
void DebugRebootSystem(void);
//过零中断处理列表
extern List ZeroCallbackList;

#define WDT_NORMAL_RESET_FLAG  0x45968562
#define FALUT_RESET_FLAG       0x12345678

/* Crash info saved to SRAM across reset */
#define CRASH_INFO_MAGIC       0xDEADBEEF
#define CRASH_BT_DEPTH_MAX     16

/* Code region for backtrace address validation */
#define CODE_REGION_BASE       0x10000000
#define CODE_REGION_MASK       0xFFF80000
/* SRAM upper bound for stack backtrace scan (crash reserved area start) */
#define CRASH_SRAM_SCAN_LIMIT  0x100BFF78
/* Backtrace: number of stack words to scan (1 KB) */
#define CRASH_BT_SCAN_WORDS    256
/* ResetAddr at top of reserved SRAM area */
#define RESET_ADDR_PTR         (*(volatile int *)(0x100BFFFC))

#pragma pack(push, 1)
typedef struct {
    u32 magic;      /* CRASH_INFO_MAGIC if valid */
    u32 r0;
    u32 r1;
    u32 r2;
    u32 r3;
    u32 r12;
    u32 lr;         /* Link Register */
    u32 pc;         /* Program Counter */
    u32 xpsr;       /* Program Status Register */
    u32 cfsr;       /* Configurable Fault Status Register */
    u32 hfsr;       /* Hard Fault Status Register */
    u32 mmfar;      /* MemManage Fault Address Register */
    u32 bfar;       /* Bus Fault Address Register */
    u32 exc_lr;     /* EXC_RETURN value */
    u32 bt_depth;   /* Backtrace depth (actual count) */
    u32 backtrace[CRASH_BT_DEPTH_MAX]; /* Stack backtrace return addresses */
} crash_info_t;
#pragma pack(pop)

/* Crash info stored at fixed SRAM address (below ResetAddr at 0x100BFFFC)
 * struct size = (15+16)*4 = 124 bytes, at 0x100BFF78 (ends at 0x100BFFF3)
 * Area 0x100BFF78 ~ 0x100BFFFF is reserved in linker script (icf) */
#define CRASH_INFO_ADDR    ((crash_info_t *)0x100BFF78)

typedef enum
{
	SYS_NORMAL_RESET,
	SYS_FALUT_RESET,
}ResetReason_e;
void resetReasonInit(void);
ResetReason_e resetReason(void);
void realSoftReset(void);
void resetResonFlag(void);
void crash_info_print(void);
extern unsigned char diffMacFlag;
#endif

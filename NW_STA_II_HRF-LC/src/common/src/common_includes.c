#include "common_includes.h"
#include "os.h"
#include "protocol_includes.h"
#include "Revision.h"
#include "gsmcu_iwdg.h"
#include <bsp.h>
#define MAX_MAP_NODE_SIZE    100
static OS_MEM MapMempool;
static map_node MapMem[MAX_MAP_NODE_SIZE];
//====================

#pragma section = ".aplParam_RAM"
#pragma location = ".aplParam_RAM"
__root const AplCommonParam_type AplCommonParam =
{
    .AplVersion = SLG_COMMIT_HASH, //version
#if !defined(II_STA)
    .FactoryCode = FACTORY_CODE_FLAG,
#elif defined(I_STA)
	.FactoryCode = FACTORY_CODE_I_FLAG,
#else
	.FactoryCode = FACTORY_CODE_II_FLAG,
#endif
};


UserSection_Type* CurrentCodeSection = (UserSection_Type*)FLASH_USER_FEATURE1_ADDRESS ;//当前code用户特征地址
u8 OldCodeSection = 0 ;

TryRunParam  CurrentTryRunParam;


typedef struct {
    s32 version;
    u8  sector;
    u8  isOk;
}code_cmp;

static void sort_code(code_cmp * code_info)
{
    code_cmp code_temp;
    for (int i=0;i<CODE_NUM-1;i++)
	{
		for (int j=0;j<CODE_NUM-1-i;j++)
		{
			if (code_info[j].version<code_info[j+1].version)
			{
				code_temp=code_info[j];
				code_info[j]=code_info[j+1];
				code_info[j+1]=code_temp;
			}
		}
	}

}
static u8 check_code_info(UserSection_Type *usersection)
{
    if ((usersection->CodeSpiFlashStartAddr|FLASH_START_ADDR) < FLASH_USER_FEATURE1_ADDRESS || (usersection->CodeSpiFlashStartAddr|FLASH_START_ADDR) > FLASH_PARAM_ADDRESS)
    {
        return 0;
    }
    if ((usersection->FeatureCode == USER_FEATURE_CODE)
        && (CheckU32Sum((u32 *)usersection, (sizeof(UserSection_Type) - 4) / 4) ==  (usersection->UserSectionCheckSum))
        && CheckU32Sum((u32*)(usersection->CodeSpiFlashStartAddr|FLASH_START_ADDR),usersection->CodeSize/4) ==  (usersection->CodeCheckSum)
        )
    {
        return 1;
    }
    return 0;
}

void CheckCurrentCodeSection()
{
    
    code_cmp cmpInfo[CODE_NUM];
    for (int i=0;i<CODE_NUM;i++)
    {
        UserSection_Type * usersection=(UserSection_Type *)(FLASH_USER_FEATURE1_ADDRESS +FLASH_USER_FEATURE1_SIZE*i);
        cmpInfo[i].sector=i+1;
        cmpInfo[i].isOk = check_code_info(usersection);
        if(cmpInfo[i].isOk) 
        {
            cmpInfo[i].version = usersection->Version;
        }
        else
        {
            cmpInfo[i].version = -1;
        }
    }
    sort_code(cmpInfo);
    for (int i=0;i<CODE_NUM;i++)
    {
        if (cmpInfo[i].isOk)
        {
            CurrentCodeSection = (UserSection_Type *)(FLASH_START_ADDR | DATA_FLASH_SECTION_SIZE * cmpInfo[i].sector);
			debug_str(DEBUG_LOG_UPDATA, "CurrentCodeSection:%d, version:%d, IsOK:%d\r\n", cmpInfo[i].sector,cmpInfo[i].version,cmpInfo[i].isOk);
        #if defined(ZB205_CHIP)
			debug_str(DEBUG_LOG_UPDATA, "BackupCodeSection:%d, version:%d, IsOK:%d\r\n", cmpInfo[CODE_NUM-2].sector,cmpInfo[CODE_NUM-2].version,cmpInfo[CODE_NUM-2].isOk);
        #endif
            debug_str(DEBUG_LOG_UPDATA, "UpdataCodeSection:%d, version:%d, IsOK:%d\r\n", cmpInfo[CODE_NUM-1].sector,cmpInfo[CODE_NUM-1].version,cmpInfo[CODE_NUM-1].isOk);
            break;
        }
    }
    OldCodeSection=cmpInfo[CODE_NUM-1].sector-1;
}
//查找是否有备份代码
UserSection_Type * Code2ndIsVaild(void)
{
    
    code_cmp cmpInfo[CODE_NUM];
    for (int i=0;i<CODE_NUM;i++)
    {
        UserSection_Type * usersection=(UserSection_Type *)(FLASH_USER_FEATURE1_ADDRESS +FLASH_USER_FEATURE1_SIZE*i);
        cmpInfo[i].sector=i+1;
        cmpInfo[i].isOk = check_code_info(usersection);
        if(cmpInfo[i].isOk) 
        {
            cmpInfo[i].version = usersection->Version;
        }
        else
        {
            cmpInfo[i].version = -1;
        }
    }
    CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
    sort_code(cmpInfo);
    OS_CRITICAL_EXIT();
    u8 finded=0;
    UserSection_Type * firstCode=NULL;
    for (int i=0;i<CODE_NUM;i++)
    {
        if (cmpInfo[i].isOk)
        {
            if (finded)
            {
               return firstCode;
            }
            else
            {
                firstCode=(UserSection_Type *)(FLASH_START_ADDR | DATA_FLASH_SECTION_SIZE * cmpInfo[i].sector);
                finded=1;
            }
        }
    }
    return NULL;

}


u32 CheckU32Sum(u32 *pd,u32 len)
{
    u32 checksum = 0;
    for(int i=0;i<len;i++)
    {
        checksum += pd[i];
    }
    return checksum;
}





//================
static void* MallocMap(void)
{
	OS_ERR err;
    map_node *mem_ptr = 0;
    mem_ptr = (map_node*)OSMemGet(&MapMempool,&err);
    if(err != OS_ERR_NONE)
	{
#ifdef __DEBUG_MODE
			while(1)
			{
				__disable_irq();
				FeedWdg();
			}
#else
			return (void *)0;
#endif	
	}
    return mem_ptr;
}

static void FreeMap(void* node)
{
	OS_ERR err;
    OSMemPut(&MapMempool,node,&err);
}

bool CheckTryRunParamInvaild(FlashTryRunParam * param)
{
	if(param->CRC32 == CommonGetCrc32((u8*)param,sizeof(FlashTryRunParam) - 4))
	{
		return true;
	}
	return false;
}

//1?12×é?t?￡?é3?ê??ˉ
void CommonInit(void)
{
    //?ú′?3?3?ê??ˉ
    OS_ERR err;
	FlashTryRunParam tryRunParam;
	AplCommonParam_type Code1Param;
    OSMemCreate(&MapMempool,"MapNode Memory",MapMem, MAX_MAP_NODE_SIZE, sizeof(map_node), &err);
    if(err != OS_ERR_NONE)
    {
        while(1);
    }
    SetpMallocMapNode(MallocMap);
    SetpFreeMapNode(FreeMap);
    CheckCurrentCodeSection();
    //读取试运行时间
	ReadFlash(TRY_RUN_TIME_ADDRESS,(u8*)&tryRunParam,sizeof(FlashTryRunParam));
	//读代码段参数
	memcpy((u8*)&Code1Param,(void*)(0x10000000 + FLASH_INTERRUPT_SIZE),sizeof(AplCommonParam_type));
	
	//如果读回来是无效的，初始化
	if(!CheckTryRunParamInvaild(&tryRunParam))
	{
		tryRunParam.Param1.TryRunVersion = Code1Param.AplVersion;
		tryRunParam.Param1.FileCRC = Code1Param.RearFileCrc32;
		tryRunParam.Param1.FileSize = Code1Param.CodeLength;
		tryRunParam.Param1.TryRunTime = 0;
		tryRunParam.CRC32 = CommonGetCrc32((u8*)&tryRunParam,sizeof(tryRunParam) - 4);
		DataFlashInit();
		EraseFlash(TRY_RUN_TIME_ADDRESS,DATA_FLASH_SECTION_SIZE);
		WriteFlash(TRY_RUN_TIME_ADDRESS,(u8*)&tryRunParam,sizeof(FlashTryRunParam));
		DataFlashClose();
	}
	else
    {
        if (Code1Param.AplVersion != tryRunParam.Param1.TryRunVersion)
        {
            tryRunParam.Param1.TryRunVersion = Code1Param.AplVersion;
            tryRunParam.Param1.FileCRC = Code1Param.RearFileCrc32;
            tryRunParam.Param1.FileSize = Code1Param.CodeLength;
        }
    }


    memcpy(&CurrentTryRunParam, &tryRunParam.Param1, sizeof(TryRunParam));

	
    List_Common_Init();
}




//重启
void RebootSystem(void)
{
#ifdef INTER_WATCH_DOG
//    CPU_SR_ALLOC();
//    CPU_CRITICAL_ENTER();
	realSoftReset();
	__disable_irq();
	#ifndef __DEBUG_MODE
    WDG_ShortReset();
	#endif
    while(1)
	{
#ifdef __DEBUG_MODE
	  FeedWdg();
#endif
	}
//    CPU_CRITICAL_EXIT();
#else
	__disable_irq();
	#ifndef __DEBUG_MODE
	WDG_SoftUnLock();
	#endif
	WDG_Enable();
	while(1)
	{
#ifdef __DEBUG_MODE
	  FeedWdg();
#endif
	}
#endif
	
}


//假重启(离网、解绑地址)
void DebugRebootSystem(void)
{
    OfflineSetState();
    DispatchLocalMac();
    AppTestModeDeal();
}

static ResetReason_e reason;
#define ResetAddr RESET_ADDR_PTR
void resetReasonInit(void)
{
    switch(ResetAddr)
    {
        case FALUT_RESET_FLAG://上一次程序异常复位
            reason=SYS_FALUT_RESET;
            break;
        case WDT_NORMAL_RESET_FLAG://
        default:
            reason=SYS_NORMAL_RESET;
    }
    ResetAddr=FALUT_RESET_FLAG;
}
ResetReason_e resetReason(void)
{
    return reason;
}
void realSoftReset(void)
{
    ResetAddr=WDT_NORMAL_RESET_FLAG;
}
void resetResonFlag(void)
{
    ResetAddr=FALUT_RESET_FLAG;
}

/**
 * @brief  打印上次异常复位的寄存器和回溯信息
 *         在 DebugOpen 之后调用，仅当检测到 CRASH_INFO_MAGIC 时输出
 */
void crash_info_print(void)
{
    volatile crash_info_t *info_p = CRASH_INFO_ADDR;
    u32 i;

    if (info_p->magic != CRASH_INFO_MAGIC)
        return;

    debug_str(0xFFFFFFFF, "========== CRASH INFO ==========\r\n");
    debug_str(0xFFFFFFFF, "PC   = 0x%08X\r\n", info_p->pc);
    debug_str(0xFFFFFFFF, "LR   = 0x%08X\r\n", info_p->lr);
    debug_str(0xFFFFFFFF, "R0   = 0x%08X\r\n", info_p->r0);
    debug_str(0xFFFFFFFF, "R1   = 0x%08X\r\n", info_p->r1);
    debug_str(0xFFFFFFFF, "R2   = 0x%08X\r\n", info_p->r2);
    debug_str(0xFFFFFFFF, "R3   = 0x%08X\r\n", info_p->r3);
    debug_str(0xFFFFFFFF, "R12  = 0x%08X\r\n", info_p->r12);
    debug_str(0xFFFFFFFF, "xPSR = 0x%08X\r\n", info_p->xpsr);
    debug_str(0xFFFFFFFF, "CFSR = 0x%08X\r\n", info_p->cfsr);
    debug_str(0xFFFFFFFF, "HFSR = 0x%08X\r\n", info_p->hfsr);
    debug_str(0xFFFFFFFF, "MMFAR= 0x%08X\r\n", info_p->mmfar);
    debug_str(0xFFFFFFFF, "BFAR = 0x%08X\r\n", info_p->bfar);
    debug_str(0xFFFFFFFF, "EXC_LR=0x%08X\r\n", info_p->exc_lr);

    if (info_p->bt_depth > 0)
    {
        u32 depth = info_p->bt_depth;
        if (depth > CRASH_BT_DEPTH_MAX) depth = CRASH_BT_DEPTH_MAX;
        debug_str(0xFFFFFFFF, "-------- CALL BACKTRACE --------\r\n");
        debug_str(0xFFFFFFFF, "  [crash] PC = 0x%08X\r\n", info_p->pc);
        debug_str(0xFFFFFFFF, "  [  0  ] LR = 0x%08X\r\n", info_p->lr);
        for (i = 0; i < depth; i++)
        {
            debug_str(0xFFFFFFFF, "  [%3d  ] 0x%08X\r\n", i + 1, info_p->backtrace[i]);
        }
    }

    debug_str(0xFFFFFFFF, "================================\r\n");

    /* Clear crash info to prevent reprinting on next normal reset */
    info_p->magic = 0;
}

#ifndef __GSMCU_FLASH_H__
#define __GSMCU_FLASH_H__
#ifdef __cplusplus
extern "C"
{
#endif

#if DEV_CCO
#ifndef RAMFUNC
    #if __ICCARM__
        #define RAMFUNC 
    #else
        #define RAMFUNC
    #endif
#endif
#elif DEV_STA
#define RAMFUNC
#ifndef RAMFUNC
    #if __ICCARM__
        #define RAMFUNC __ramfunc
    #else
        #define RAMFUNC
    #endif
#endif
#endif
/** 
  * @brief 
  */ 
RAMFUNC void FLASH_Init();
RAMFUNC uint8_t FLASH_GetFalshStatus(uint8_t regx);
RAMFUNC void FLASH_EarseBlock(uint32_t block_start);
RAMFUNC void FLASH_ProgramPackageData(uint32_t Package_start,uint32_t *pdata,uint32_t pagewords);
RAMFUNC void FLASH_ProgramPackageBytes(uint32_t Package_start,uint8_t *pdata,uint32_t bytes);
RAMFUNC void FLASH_GetPackageData(uint32_t Package_start,uint32_t *pdata,uint32_t wordsize);
RAMFUNC void FLASH_WriteFlashStatus(uint8_t regx,uint32_t data);
RAMFUNC void FLASH_WriteFlashStatusNoVolatile(uint8_t regx,uint32_t data);
RAMFUNC void FLASH_GetSecurity(uint32_t reg_num,uint32_t addr,uint32_t *pdata,uint32_t wordsize);
#if DEV_CCO
RAMFUNC void FLASH_GetPackageBytes(uint32_t Package_start,uint8_t *pdata,uint32_t bytes);           //by sen
#endif
RAMFUNC void FLASH_Close();
RAMFUNC void FLASH_Reset(void);
void FLASH_ResetAtFault(void);

void GetDevFlashID(void);
extern const u8 FlashStatusRegNum;
extern uint32_t dev_flash_id;

#ifdef __cplusplus
}
#endif
#endif /* __GSMCU_FLASH_H__ */


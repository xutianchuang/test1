/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_flash.h"
#include "string.h"
#include "os.h"
#if DEV_CCO
#include "System_inc.h"
#endif

bool DiagnosticDataFlashAddr(u32 start_addr, u32 dlen)
{
#if DEV_CCO
	if((start_addr < DATA_FLASH_START_ADDR) ||
		((start_addr + dlen) > DATA_FLASH_START_ADDR + MAX_DATA_FLASH_SIZE))
#elif DEV_STA
	if((start_addr < FLASH_START_ADDR) ||
		((start_addr + dlen) > FLASH_START_ADDR + MAX_DATA_FLASH_SIZE))
#endif
	{
		return false;
	}
	return true;
}

void EraseDataFlash(u32 start_addr, u32 dlen)
{
	u32 head_section, tail_section;
	u32 section_num;
	u32 i;
	u32 addr_idx;
	head_section = start_addr % DATA_FLASH_SECTION_SIZE;
    head_section = DATA_FLASH_SECTION_SIZE - head_section;
	if(dlen > head_section)
	{
		if(0 != head_section)
		{
			FLASH_EarseBlock(start_addr);
		}
		addr_idx = start_addr + head_section;
		dlen -= head_section;
		section_num = dlen / DATA_FLASH_SECTION_SIZE;
		tail_section = dlen % DATA_FLASH_SECTION_SIZE;
		for(i = 0; i < section_num; i++)
		{
			FLASH_EarseBlock(addr_idx);
			addr_idx += DATA_FLASH_SECTION_SIZE;
		}
		if(0 != tail_section)
		{
			FLASH_EarseBlock(addr_idx);
		}
	}
	else
	{
		FLASH_EarseBlock(start_addr);
	}
}

void WriteDataFlash(u32 start_addr, u8 * pbuf, u32 dlen)
{
	u32 head_data_len, tail_data_len;
	u32 page_num;
	u32 data_idx = 0;
	u32 i;
	head_data_len = start_addr % DATA_FLASH_PAGE_SIZE;
    head_data_len = DATA_FLASH_PAGE_SIZE - head_data_len;
	if(dlen > head_data_len)
	{
		FLASH_ProgramPackageBytes(start_addr, pbuf, head_data_len);
		data_idx += head_data_len;
		dlen -= head_data_len;
		page_num = dlen / DATA_FLASH_PAGE_SIZE;
		tail_data_len = dlen % DATA_FLASH_PAGE_SIZE;
		for(i = 0; i < page_num; i++)
		{
			FLASH_ProgramPackageBytes(start_addr + data_idx, &pbuf[data_idx], DATA_FLASH_PAGE_SIZE);
			data_idx += DATA_FLASH_PAGE_SIZE;
		}
		FLASH_ProgramPackageBytes(start_addr + data_idx, &pbuf[data_idx], tail_data_len);
		
	}
	else
	{
		FLASH_ProgramPackageBytes(start_addr, pbuf, dlen);
	}
}

void ReadDataFlash(u32 start_addr, u8 * pbuf, u32 dlen)
{
	if(!DiagnosticDataFlashAddr(start_addr, dlen))
	{
		return;
	}
	FLASH_Close();
	memcpy(pbuf, (u32*)start_addr, dlen);
}


#if DEV_CCO
//以page为单位写入
/*
0-----256------512-------768------1024
             510             770   len = 770-510+1 = 261
             510 写入长度 2
			 512 写入长度 256
			 768 写入长度 3
*/

unsigned char data_flash_wirte_page_by_page(unsigned long addr, u8* data, unsigned long len)
{
    uint32_t addr_mod = addr % DATA_FLASH_PAGE_SIZE;

    if(addr_mod > 0)
    {
        uint32_t head_len = (DATA_FLASH_PAGE_SIZE-addr_mod);
        head_len = MIN(head_len, len);
        FLASH_ProgramPackageBytes(addr, data, head_len);
        addr += head_len;
        data += head_len;
        len -= head_len;
    }

    if(len <= 0)
        return true;

    uint16_t pages = len/DATA_FLASH_PAGE_SIZE;
    uint16_t leaved = len%DATA_FLASH_PAGE_SIZE;

    for(uint16_t i=0; i<pages; i++)
        FLASH_ProgramPackageBytes(addr+i*DATA_FLASH_PAGE_SIZE, data+i*DATA_FLASH_PAGE_SIZE, DATA_FLASH_PAGE_SIZE);

    if(leaved > 0)
        FLASH_ProgramPackageBytes(addr+pages*DATA_FLASH_PAGE_SIZE, data+pages*DATA_FLASH_PAGE_SIZE, leaved);

    return true;
}

uint8_t s_flash_section[DATA_FLASH_SECTION_SIZE];


unsigned char data_flash_write_straight( unsigned long addr, u8* data, unsigned long len )
{
//    debug_str("addr=%x, len=%d\r\n", addr, len);
    
//    if(!DiagnosticDataFlashAddr(addr, len))
//        return false;

    //先读取一个section，如果和写的一样，则直接退出，否则，擦除，然后写入新数据
    int32_t leaved_len = (int32_t)len;
    int32_t current_len = 0;
    uint32_t current_addr = addr;
    uint8_t* current_data = data;
    uint32_t addr_mod = addr % DATA_FLASH_SECTION_SIZE;

    OS_ERR err;
    OSSchedLock(&err);

    uint16_t counter = 0;
    
    FLASH_Init();

    while(leaved_len > 0)
    {
        current_len = (leaved_len<=DATA_FLASH_SECTION_SIZE) ? leaved_len:DATA_FLASH_SECTION_SIZE;

        uint32_t section_addr = current_addr;

        if(addr_mod > 0)
        {
            section_addr -= addr_mod;
            current_len = (DATA_FLASH_SECTION_SIZE-addr_mod)<current_len ? (DATA_FLASH_SECTION_SIZE-addr_mod):current_len;
        }

        FLASH_GetPackageData(section_addr, (uint32_t*)s_flash_section, DATA_FLASH_SECTION_SIZE/4);
        //memcpy_special((uint32_t*)s_flash_section, (uint32_t*)section_addr, DATA_FLASH_SECTION_SIZE);

        //没有新数据写入，跳过
        if(0 == memcmp(&s_flash_section[addr_mod], current_data, current_len))
            goto L_Next;

        //擦除，重新写
        FLASH_EarseBlock(section_addr);
        memcpy(&s_flash_section[addr_mod], current_data, current_len);
        data_flash_wirte_page_by_page(section_addr, s_flash_section, DATA_FLASH_SECTION_SIZE);
L_Next:
        addr_mod = 0;
        current_addr += current_len;
        current_data += current_len;
        leaved_len -= current_len;

        counter++;
        if((counter&7)==0)
            FeedWdg();
    }
	FLASH_ResetAtFault();
    FLASH_Close();

    OSSchedUnlock(&err);

    return true;
}

unsigned char data_flash_read_straight( unsigned long addr, u8* data, unsigned long len )
{
//    if(!DiagnosticDataFlashAddr(addr, len))
//        return false;

#if 1
    OS_ERR err;
    OSSchedLock(&err);
    
    FLASH_Init();
    FLASH_GetPackageBytes(addr, data, len);
	FLASH_ResetAtFault();
    FLASH_Close();

    OSSchedUnlock(&err);
    
    return true;
#elif 1
    OS_ERR err;
    OSSchedLock(&err);

	FLASH_Close();
	memcpy(data, (u32*)addr, len);

    OSSchedUnlock(&err);

    return true;
#endif
}

unsigned char data_flash_erase_straight( unsigned long addr, unsigned long len )
{
    //地址从block的整数倍开始
    u32 addr_mod = addr % DATA_FLASH_SECTION_SIZE;
    if(addr_mod > 0)
    {
        addr -= addr_mod;
        len += addr_mod;
    }

    //长度到block的整数倍
    u32 len_mod = len % DATA_FLASH_SECTION_SIZE;
    if(len_mod > 0)
        len += (DATA_FLASH_SECTION_SIZE-len_mod);

    if(!DiagnosticDataFlashAddr(addr, len))
        return false;

    //擦除
    OS_ERR err;
    OSSchedLock(&err);

    uint16_t counter = 0;
    
    u32 addr_end = addr + len;
    while(addr < addr_end)
    {
        FLASH_Init();
        FLASH_EarseBlock(addr);
		FLASH_ResetAtFault();
        FLASH_Close();
        addr += DATA_FLASH_SECTION_SIZE;

        counter++;
        if((counter&7)==0)
            FeedWdg();
    }
    
    OSSchedUnlock(&err);
    
    return true;
}

#elif DEV_STA
void EraseFlash(u32 start_addr, u32 dlen)
{
	if(start_addr==0||start_addr==0x12000000)
	{
		return;		
	}
	//擦除只能以DATA_FLASH_SECTION_SIZE为单位进行擦除，传入的参数横跨多少个BLOCK就擦多少个
	//计算擦除的起始地址
	u32 realStartAddr = 0;
	u32 blockNum = 0;
	u32 i = 0;
	u32 headLen = start_addr % DATA_FLASH_SECTION_SIZE;
	CPU_SR_ALLOC();
	if(headLen)
		realStartAddr = start_addr - headLen;
	else
		realStartAddr = start_addr;
	
	//计算需要擦除多少个block
	if(dlen > DATA_FLASH_SECTION_SIZE - headLen)
	{
		u32 leftByte = dlen - (DATA_FLASH_SECTION_SIZE - headLen);
		if(leftByte%DATA_FLASH_SECTION_SIZE)
			blockNum = leftByte/DATA_FLASH_SECTION_SIZE + 1 + 1;
		else
			blockNum = leftByte/DATA_FLASH_SECTION_SIZE + 1;
	}
	else
		blockNum = 1;
	
	//擦除
	for(;i < blockNum;i++)
	{
		OS_CRITICAL_ENTER();
		FLASH_EarseBlock(realStartAddr + i*DATA_FLASH_SECTION_SIZE);
		OS_CRITICAL_EXIT();
	}
	
}
void WriteFlash(u32 start_addr, u8 * pbuf, u32 dlen)
{
	if(start_addr==0||start_addr==0x12000000)
	{
		return;		
	}
	u32 headLen = start_addr % DATA_FLASH_PAGE_SIZE;
	u32 currentStartAddr = start_addr;
	u32 nextStartAddr = start_addr - headLen + DATA_FLASH_PAGE_SIZE;
	CPU_SR_ALLOC();
	for(;dlen;)
	{
		u32 currentWriteNum = 0;
		if(nextStartAddr - currentStartAddr < dlen)
		{
			currentWriteNum = nextStartAddr - currentStartAddr;
		}
		else
		{
			currentWriteNum = dlen;
		}
		OS_CRITICAL_ENTER();
		FLASH_ProgramPackageBytes(currentStartAddr, pbuf, currentWriteNum);
		OS_CRITICAL_EXIT();
		currentStartAddr += currentWriteNum;
		nextStartAddr += DATA_FLASH_PAGE_SIZE;
		dlen -= currentWriteNum;
		pbuf += currentWriteNum;
	}
}

void ReadFlash(u32 start_addr, u8 * pbuf, u32 dlen)
{
	FLASH_Close();
	memcpy(pbuf, (u32*)start_addr, dlen);
}

//保护头512K数据
void Protection512KFlash(void)
{
	if (FlashStatusRegNum == 3)
	{
		FLASH_WriteFlashStatus(1, ((1<<4|1<<5)<<16)|(0x2<<8)|(1<<6));
	}
	else if (FlashStatusRegNum == 2)
	{
		FLASH_WriteFlashStatus(1, ((1<<4|1<<5)<<8)|(0x2<<0));
	}
	else
	{
		__BKPT(1);
	}
}
void Unprotection512KFlash(void)
{
	if (FlashStatusRegNum == 3)
	{
		FLASH_WriteFlashStatus(1, (0x2 << 8) | (1 << 6));
	}
	else if (FlashStatusRegNum == 2)
	{
		FLASH_WriteFlashStatus(1, 0x2);
	}
	else
	{
		__BKPT(1);
	}
}

void Protection512KFlashNoVolatile(void)
{
	DataFlashInit();
	u8 reg =FLASH_GetFalshStatus(1);
	if ((reg&(1<<4|1<<5))==(1<<4|1<<5))
	{
		DataFlashClose();
		return;
	}
	if (FlashStatusRegNum == 3)
	{
		FLASH_WriteFlashStatusNoVolatile(1, ((1<<4|1<<5)<<16)|(0x3e<<8)|(1<<6));
	}
	else if (FlashStatusRegNum == 2)
	{
		FLASH_WriteFlashStatusNoVolatile(1, ((1<<4|1<<5)<<8)|(0x6<<0));
	}
	else
	{
		__BKPT(1);
	}
	DataFlashClose();
}

#endif

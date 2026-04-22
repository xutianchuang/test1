/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_flash.h"
#include "string.h"
#include "os.h"
#include "gsmcu_flash.h"

bool DiagnosticDataFlashAddr(u32 start_addr, u32 dlen)
{
	if((start_addr < FLASH_START_ADDR) ||
		((start_addr + dlen) > FLASH_START_ADDR + MAX_DATA_FLASH_SIZE))
	{
		return false;
	}
	return true;
}

void EraseDataFlash(u32 start_addr, u32 dlen)
{
	if ((start_addr&0xffffff)==0)
	{
		return;
	}
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
	if ((start_addr&0xffffff)==0)
	{
		return;
	}
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



void EraseFlash(u32 start_addr, u32 dlen)
{
	if ((start_addr&0xffffff)==0)
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
	if ((start_addr&0xffffff)==0)
	{
		return;
	}
	u32 headLen = start_addr % DATA_FLASH_PAGE_SIZE;
	u32 currentStartAddr = start_addr;
	u32 nextStartAddr = start_addr - headLen + DATA_FLASH_PAGE_SIZE;
	CPU_SR_ALLOC();
	for (; dlen;)
	{
		u32 currentWriteNum = 0;
		if (nextStartAddr - currentStartAddr < dlen)
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
void Protection1MFlash(void)
{
	if (FlashStatusRegNum == 3)
	{
		FLASH_WriteFlashStatus(1, (1 << 4 | 1 << 5 | 1 << 2) | (0x2 << 8) | ((1 << 6) << 16));
	}
	else if (FlashStatusRegNum == 2)
	{
		FLASH_WriteFlashStatus(1, (1 << 4 | 1 << 5 | 1 << 2) | (0x2 << 8));
	}
	else
	{
		__BKPT(1);
	}

}
void Unprotection1MFlash(void)
{
	if (FlashStatusRegNum == 3)
	{
		FLASH_WriteFlashStatus(1, (0x2 << 8) | (1 << 6));
	}
	else if (FlashStatusRegNum == 2)
	{
		FLASH_WriteFlashStatus(1, (0x2 << 8));
	}
	else
	{
		__BKPT(1);
	}
}

void Protection1MFlashNoVolatile(void)
{
	DataFlashInit();
    u8 reg=FLASH_GetFalshStatus(1);
    u8 reg2= FLASH_GetFalshStatus(2);
	if((reg2&(1<<1)) != (1 << 1))
	{
		FLASH_WriteQeFlag();
	}
	if ((reg&(1 << 4 | 1 << 5 | 1 << 2))==(1 << 4 | 1 << 5 | 1 << 2))
	{
		DataFlashClose();
		return;
	}

	if (FlashStatusRegNum == 3)
	{
		FLASH_WriteFlashStatusNoVolatile(1, (1 << 4 | 1 << 5 | 1 << 2) | (0x3e << 8) | ((1 << 6) << 16));
	}
	else if (FlashStatusRegNum == 2)
	{
		FLASH_WriteFlashStatusNoVolatile(1, (1 << 4 | 1 << 5 | 1 << 2) | (0x6<< 8));
	}
	else
	{
		__BKPT(1);
	}
	DataFlashClose();
}



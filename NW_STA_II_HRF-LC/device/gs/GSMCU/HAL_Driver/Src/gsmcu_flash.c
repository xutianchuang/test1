
#include  <gsmcu_hal.h>

/**
  * @brief
**/


/**
  * @brief
**/
#define WRITE_ENABLE    (0x06UL)
#define WRITE_DISABLE   (0x04UL)
#define SECTION_ERASE   (0x20UL)
//
#define READ_STATUS_REG1    (0x05UL)
#define READ_STATUS_REG2    (0x35UL)
#define READ_STATUS_REG3    (0x15UL)

#define WRITE_STATUS_REG1    (0x01UL)
#define WRITE_STATUS_REG2    (0x31UL)
#define WRITE_STATUS_REG3    (0x11UL)

#define READ_DATA            (0x03UL)
#define PAGE_PROGRAM         (0x02UL)

/**
  * @brief 
  * @param 
  * @param 
  * @retval 
  */
RAMFUNC void FLASH_Init()
{
	FTSPI->XIPCMD = 0x0006B208;
    if((FTSPI->CR&(0x01UL<<20)) != 0)
    {
        FTSPI->CR = FTSPI->CR & (~(0x01UL<<20));  //切换模式
        while((FTSPI->CR&(0x01UL<<20))); //由1变成0
        //while(((FTSPI->CR&(0x01UL<<8)) == 1)); //由1变成0

    }
    //
    FTSPI->ICR = (0x02<<12)|(0x02<<8);//fifo 24words
}
/**
  * @brief 
  * @param 
  * @param 
  * @retval 
  */
RAMFUNC void FLASH_Close()
{

    if((FTSPI->CR&(0x01UL<<20)) == 0)
    {
        FTSPI->ICR = 0;//fifo 24words
        //
        FTSPI->CR = FTSPI->CR | (0x01UL<<20);  //切换模式
        while(((FTSPI->CR&(0x01UL<<20)) == 0)); //由0变成1
    }
	FTSPI->XIPCMD = 0x0006B208;
}
void FLASH_WaitCmdDone(void)
{
	int wait_cnt=350000;//大概等待1.1s的时间 足够当前代码中的任何操作完成了
	for (;wait_cnt>0;wait_cnt--)
	{
		if ((FLASH_GetFalshStatus(1) & 0x01) != 0x01)
		{
			break;
		}
	}
	if (wait_cnt==0)//超出等待时间 需要复位
	{
		FLASH_Reset();
	}
}
/**
  * @brief
  * @param
  * @param 
  * @retval 
  */
RAMFUNC uint8_t FLASH_GetFalshStatus(uint8_t regx)
{
    FTSPI->CMD0 = 0x00;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(0x00UL<<0);
    FTSPI->CMD2 = 0;
    switch(regx)
    {
        case 2:
        FTSPI->CMD3 = (READ_STATUS_REG2<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                |(0x00UL<<4)|(0x01UL<<3)|(0x01UL<<2)|(0x00UL<<1);
        break;
        case 3:
        FTSPI->CMD3 = (READ_STATUS_REG3<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                |(0x00UL<<4)|(0x01UL<<3)|(0x01UL<<2)|(0x00UL<<1);
        break;
        default:
        FTSPI->CMD3 = (READ_STATUS_REG1<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                |(0x00UL<<4)|(0x01UL<<3)|(0x01UL<<2)|(0x00UL<<1);
        break;
    }
    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;
    return (FTSPI->SPISR)&0xFF;
}

RAMFUNC void FLASH_WriteFlashStatus(uint8_t regx,uint32_t data)
{

    //write enable
    FTSPI->CMD0 = (uint32_t)0x00;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(0x00UL<<0);
    FTSPI->CMD2 = 0;
    FTSPI->CMD3 = (0x50<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                    |(0x00UL<<4)|(0x00UL<<3)|(0x00UL<<2)|(0x01UL<<1);
    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;

    FTSPI->CMD0 = (uint32_t)data;
	FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(FlashStatusRegNum<<0);
    FTSPI->CMD2 = 0;

	
	switch(regx)
    {
        case 2:
        FTSPI->CMD3 = (WRITE_STATUS_REG2<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                |(0x00UL<<4)|(0x01UL<<3)|(0x00UL<<2)|(0x1UL<<1);
        break;
        case 3:
        FTSPI->CMD3 = (WRITE_STATUS_REG3<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                |(0x00UL<<4)|(0x01UL<<3)|(0x00UL<<2)|(0x1UL<<1);
        break;
        default:
        FTSPI->CMD3 = (WRITE_STATUS_REG1<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                |(0x00UL<<4)|(0x01UL<<3)|(0x00UL<<2)|(0x1UL<<1);
        break;
    }

    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;
	FLASH_WaitCmdDone();
}

RAMFUNC void FLASH_WriteFlashStatusNoVolatile(uint8_t regx,uint32_t data)
{

    //write enable
    FTSPI->CMD0 = (uint32_t)0x00;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(0x00UL<<0);
    FTSPI->CMD2 = 0;
    FTSPI->CMD3 = (0x06<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                    |(0x00UL<<4)|(0x00UL<<3)|(0x00UL<<2)|(0x01UL<<1);
    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;

    FTSPI->CMD0 = (uint32_t)data;
	FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(FlashStatusRegNum<<0);
    FTSPI->CMD2 = 0;

	
	switch(regx)
    {
        case 2:
        FTSPI->CMD3 = (WRITE_STATUS_REG2<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                |(0x00UL<<4)|(0x01UL<<3)|(0x00UL<<2)|(0x1UL<<1);
        break;
        case 3:
        FTSPI->CMD3 = (WRITE_STATUS_REG3<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                |(0x00UL<<4)|(0x01UL<<3)|(0x00UL<<2)|(0x1UL<<1);
        break;
        default:
        FTSPI->CMD3 = (WRITE_STATUS_REG1<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                |(0x00UL<<4)|(0x01UL<<3)|(0x00UL<<2)|(0x1UL<<1);
        break;
    }

    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;
   
    FLASH_WaitCmdDone();
}

/**
  * @brief
  * @param
  * @param 
  * @retval 
  */
RAMFUNC void FLASH_EarseBlock(uint32_t block_start)
{
    //write enable
    FTSPI->CMD0 = (uint32_t)0x00;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(0x00UL<<0);
    FTSPI->CMD2 = 0;
    FTSPI->CMD3 = (WRITE_ENABLE<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                    |(0x00UL<<4)|(0x00UL<<3)|(0x00UL<<2)|(0x01UL<<1);
    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;

    //section erase 
    FTSPI->CMD0 = (uint32_t)block_start;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(0x03UL<<0);
    FTSPI->CMD2 = 0;
    FTSPI->CMD3 = (SECTION_ERASE<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                    |(0x00UL<<4)|(0x00UL<<3)|(0x00UL<<2)|(0x01UL<<1);
    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;
    
    FLASH_WaitCmdDone();

}

/**
  * @brief
  * @param
  * @param 
  * @retval 
  */
RAMFUNC void FLASH_ProgramPackageData(uint32_t Package_start,uint32_t *pdata,uint32_t pagewords)
{
    uint32_t dataindex = 0;
    uint32_t currentdatalen = 0;
    uint32_t leavedatalen = pagewords;
    //write enable
    FTSPI->CMD0 = (uint32_t)0x00;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(0x00UL<<0);
    FTSPI->CMD2 = 0;
    FTSPI->CMD3 = (WRITE_ENABLE<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                    |(0x00UL<<4)|(0x00UL<<3)|(0x00UL<<2)|(0x01UL<<1);
    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;
    //section erase 
    FTSPI->CMD0 = (uint32_t)Package_start;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(0x03UL<<0);
    FTSPI->CMD2 = pagewords<<2;
    FTSPI->CMD3 = (PAGE_PROGRAM<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                    |(0x00UL<<4)|(0x00UL<<3)|(0x00UL<<2)|(0x01UL<<1);
    while(1)
    {
        currentdatalen = leavedatalen > 0x20 ? 0x20:leavedatalen;
        leavedatalen -= currentdatalen;
        while((FTSPI->SR&0x01) == 0x00);
        for(int i = 0;i<currentdatalen;i++)
        {
            (FTSPI->DR) = ((uint32_t*)pdata)[dataindex++];
        }
        if(leavedatalen == 0)break;
    }
    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;
    
    FLASH_WaitCmdDone();
}

/**
  * @brief
  * @param
  * @param 
  * @retval 
  */
RAMFUNC void FLASH_ProgramPackageBytes(uint32_t Package_start,uint8_t *pdata,uint32_t bytes)
{
    uint32_t dataindex = 0;
    uint32_t currentdatalen = 0;
    uint32_t leavedatalen = bytes>>2;
	if(bytes==0)return;
    //write enable
    FTSPI->CMD0 = (uint32_t)0x00;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(0x00UL<<0);
    FTSPI->CMD2 = 0;
    FTSPI->CMD3 = (WRITE_ENABLE<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                    |(0x00UL<<4)|(0x00UL<<3)|(0x00UL<<2)|(0x01UL<<1);
    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;
    //action erase 
    FTSPI->CMD0 = (uint32_t)Package_start;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(0x03UL<<0);
    FTSPI->CMD2 = bytes;
    FTSPI->CMD3 = (PAGE_PROGRAM<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                    |(0x00UL<<4)|(0x00UL<<3)|(0x00UL<<2)|(0x01UL<<1);
    while(1)
    {
        currentdatalen = leavedatalen > 0x20 ? 0x20:leavedatalen;
        leavedatalen -= currentdatalen;
        while((FTSPI->SR&0x01) == 0x00);
        for(uint32_t i = 0;i<currentdatalen;i++)
        {
            (FTSPI->DR) = ((uint32_t*)pdata)[dataindex++];
        }
        if(leavedatalen == 0)
		{
			if(currentdatalen ==0x20)
			{
				while((FTSPI->SR&0x01) == 0x00);
			}
			
			if((bytes&0x03) > 0)
			{
				uint32_t datatmp = 0xFFFFFFFF;
				for(uint32_t i= (bytes&0x03);i > 0;i--)
				{
					datatmp = (datatmp <<8)|pdata[(dataindex<<2) + (i-1)];
				}
				(FTSPI->DR) = datatmp;
			}
			break;
		}
    }
	
    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;
    

    FLASH_WaitCmdDone();
}
/**
  * @brief
  * @param
  * @param 
  * @retval 
  */
RAMFUNC void FLASH_GetPackageData(uint32_t Package_start,uint32_t *pdata,uint32_t wordsize)
{
    uint32_t dataindex = 0;
    uint32_t currentdatalen = 0;
    uint32_t leavedatalen = wordsize;
    
    //section erase 
    FTSPI->CMD0 = (uint32_t)Package_start;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(0x03UL<<0);
    FTSPI->CMD2 = wordsize<<2;
    FTSPI->CMD3 = (READ_DATA<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                    |(0x00UL<<4)|(0x00UL<<3)|(0x00UL<<2)|(0x00UL<<1);
    //
    while(1)
    {
        currentdatalen = leavedatalen > 0x20 ? 0x20:leavedatalen;
        leavedatalen -= currentdatalen;
        while((FTSPI->SR&0x02) == 0x00);
        for(uint32_t i = 0;i<currentdatalen;i++)
        {
            ((uint32_t*)pdata)[dataindex++] = (FTSPI->DR);
        }
        if(leavedatalen == 0)break;
    }
    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;
}


#if DEV_CCO
RAMFUNC void FLASH_GetPackageBytes(uint32_t Package_start,uint8_t *pdata,uint32_t bytes)
{
    uint32_t dataindex = 0;
    uint32_t currentdatalen = 0;
    uint32_t leavedatalen = bytes>>2;

    if(bytes==0)return;

    //section erase
    FTSPI->CMD0 = (uint32_t)Package_start;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(0x03UL<<0);
    FTSPI->CMD2 = bytes;
    FTSPI->CMD3 = (READ_DATA<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                    |(0x00UL<<4)|(0x00UL<<3)|(0x00UL<<2)|(0x00UL<<1);
    //
    while(1)
    {
        currentdatalen = leavedatalen > 0x20 ? 0x20:leavedatalen;
        leavedatalen -= currentdatalen;
        while((FTSPI->SR&0x02) == 0x00);
        for(uint32_t i = 0;i<currentdatalen;i++)
        {
            ((uint32_t*)pdata)[dataindex++] = (FTSPI->DR);
        }
        if(leavedatalen == 0)
		{
            uint32_t len_mod = bytes&0x03;
			if(len_mod > 0)
			{
    			if(currentdatalen ==0x20)
    				while((FTSPI->SR&0x02) == 0x00);

				uint32_t datatmp = (FTSPI->DR);
				for(uint32_t i=0; i<len_mod; i++)
                    pdata[(dataindex<<2) + i] = (uint8_t)(datatmp>>i*8 & 0xff);
			}
			break;
		}
    }
    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;
}
#endif
//0x0015400B
static u32 FLASH_GetId(void)
{
	u32 flash_id = 0;
	FTSPI->CMD0 = (uint32_t)0x00;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(0x00UL<<0);
    FTSPI->CMD2 = 3;
    FTSPI->CMD3 = (0X9FUL<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                    |(0x00UL<<4)|(0x00UL<<3)|(0x00UL<<2)|(0x00UL<<1);

    while((FTSPI->SR&0x02) == 0x00);
    flash_id = (FTSPI->DR);

    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;
	return flash_id;
}
u32 data_flash_read_flash_id()
{
    u32 flash_id = 0;
    
    FLASH_Init();

    flash_id=FLASH_GetId();
    FLASH_Close();

    return flash_id;
}

uint32_t dev_flash_id;
const u8 FlashStatusRegNum=0xff;
void GetDevFlashID(void)
{
    dev_flash_id = data_flash_read_flash_id();
    int i = 0;
    u32  FlashArray[] =
    {
        0x00144020, //WHXX 1M
        0x00154020, //WHXX 2M
		0x0014605e, //new flash
        0x0014400b, //XTX 1M
        0x0015400b, //XTX 2M
		0x001540A1, //FDW 2M
    };
    for (i = 0; i < sizeof(FlashArray)/4; i++)
    {
        if (dev_flash_id == FlashArray[i])
        {
            if (i > 2) //芯天下
            {
                *(u8*)&FlashStatusRegNum = 2;
            }
            else //武汉新芯
            {
                *(u8*)&FlashStatusRegNum = 3;
            }
            return;
        }

    }
    while (1)
    {
        __BKPT(1);
    }
}



RAMFUNC void FLASH_GetSecurity(uint32_t reg_num,uint32_t addr,uint32_t *pdata,uint32_t wordsize)
{
    uint32_t dataindex = 0;
    uint32_t currentdatalen = 0;
    uint32_t leavedatalen = wordsize;
    if (FlashStatusRegNum != 3)
	{
		return;
	}
	if (reg_num > 0 && reg_num < 4)
	{
		addr|=reg_num<<12;
	}
	else
	{
		return;
	}
    //section erase 
    FTSPI->CMD0 = (uint32_t)addr;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x08UL<<16)|(0x03UL<<0);
    FTSPI->CMD2 = wordsize<<2;
    FTSPI->CMD3 = (0x48<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                    |(0x00UL<<4)|(0x00UL<<3)|(0x00UL<<2)|(0x00UL<<1);
    //
    while(1)
    {
        currentdatalen = leavedatalen > 0x20 ? 0x20:leavedatalen;
        leavedatalen -= currentdatalen;
        while((FTSPI->SR&0x02) == 0x00);
        for(uint32_t i = 0;i<currentdatalen;i++)
        {
            ((uint32_t*)pdata)[dataindex++] = (FTSPI->DR);
        }
        if(leavedatalen == 0)break;
    }
    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;
}
//只有发生异常时才进行flash复位
void FLASH_Reset(void)
{
    //write enable
	FTSPI->CMD0 = (uint32_t)0x00;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(0x00UL<<0);
    FTSPI->CMD2 = 0;
    FTSPI->CMD3 = (0x66<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                    |(0x00UL<<4)|(0x00UL<<3)|(0x00UL<<2)|(0x01UL<<1);
    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;

    FTSPI->CMD0 = (uint32_t)0x00;
    FTSPI->CMD1 = (0x00UL<<28)|(0x01UL<<24)|(0x00UL<<16)|(0x00UL<<0);
    FTSPI->CMD2 = 0;
    FTSPI->CMD3 = (uint32_t)(0x99<<24)|(0x00UL<<16)|(0x00UL<<8)|(0x00UL<<5)
                    |(0x00UL<<4)|(0x00UL<<3)|(0x00UL<<2)|(0x01UL<<1);
    while((FTSPI->ISR&0x01)==0x00);
    FTSPI->ISR = 0x01;

    //Reset操作后到Flash下一个指令增加延时
    volatile int i = 1450; //默认20us  
    if (dev_flash_id == 0)
    {
        i = 1450; //60us
    }
    while (i--)
    {
        ;
    }
    
    while((FLASH_GetFalshStatus(1)&0x01) == 0x01);

}


void FLASH_ResetAtFault(void)
{
    //write enable
	if (FLASH_GetId()==dev_flash_id)
	{
		return;	
	}
	FLASH_Reset();

}

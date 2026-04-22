#include  <gsmcu_hal.h>
#include <string.h>
#include "RTE_Device.h"
#define FLASH_BASE					0x13000000UL
/**
  * @brief
**/
typedef enum
{
	Read_Data_SPI_ID				= 0x00,
	Write_Enable_SPI_novolatile_ID	= 0X01,
	Read_SR1_SPI_ID					= 0x02,
	Write_SR_SPI_ID				 	= 0x03,		
 	Enable_Reset_ID					= 0x04,
 	Sector_Erase_4k_SPI				= 0x05,
 	Fast_Read_Quad_IO_1st_ID		= 0x06,
 	Write_Enable_SPI_volatile_ID	= 0x07,
 	Erase_Securityreg_ID			= 0x08,
 	Reset_ID						= 0x09,
 	Read_Security_Reg_ID			= 0x0A,
 	Read_JEDECID_SPI_ID				= 0x0c,
 	Read_SR2_SPI_ID					= 0x0d,
 	Page_program_ID					= 0x0e,
 	Program_Securityreg_ID			= 0x0f
}QSPI_LUTSQEID_TypeDef;

typedef enum
{
	STOP		= 0x00,
	CMD			= 0x01,
	ADDR		= 0x02,
	DUMMY		= 0x03,
	MODE		= 0x04,
	MODE2		= 0x05,
	MODE4		= 0x06,
	READ		= 0x07,
	WRITE		= 0x08,
	JMP_ON_CS	= 0x09,
	ADDR_DDR	= 0x0A,
	MODE_DDR	= 0x0B,
	MODE2_DDR	= 0x0C,
	MODE4_DDR	= 0x0D,
	READ_DDR	= 0x0E,
	WRITE_DDR	= 0x0F,
	DATA_LEARN	= 0x10,
	CMD_DDR		= 0x11,
	CADDR		= 0x12,
	CADDR_DDR	= 0x13
}QSPI_Instruction_TypeDef;

typedef enum
{
	Write_Enable				= 0x06,
	Volatile_SR_WRITE_Enable	= 0x50,
	Read_Security_Reg			= 0x48,
	Read_SR1					= 0x05,
	Read_SR2					= 0x35,
	Write_SR					= 0x01,
	Page_Program				= 0x02,
	Quad_Page_program			= 0x32,
	Sector_Erase_4k				= 0x20,
	Block_Erase_32k				= 0x52,
	Block_Erase_64k				= 0xD8,
	Chip_Erase					= 0xC7,
	Power_Down					= 0xB9,
	Read_Data					= 0x03,
	Fast_Read					= 0x0B,
	JEDC_ID						= 0x9F,
	Write_SR2					= 0x31,
	Fast_Read_Quad_Output		= 0x6B,
	Fast_Read_Quad_IO			= 0xEB,
	Enable_Reset				= 0x66,
	Reset						= 0x99,
	Erase_SecurityReg			= 0x44,
	Program_SecurityReg			= 0x42
}QSPI_LUT_OPERAND_TypeDef;

void BFGENCR_SEQ(uint32_t SQEID);
void IPCR_SEQ(uint32_t SQEID, uint32_t IDATSZ);
void Flash_Wait_BUSY(void);

/**
  * @brief
**/
#define WAIT_REM32(regval, expdata)\
	do\
	{\
		;\
	}while(regval != expdata)

void QSPI_Enable(void)
{
	/*Enable QSPI*/
	QSPI->MCR_b.SCLKCFG = 7;
	QSPI->MCR_b.END_CFG = 3;
	QSPI->MCR_b.MDIS = 0;

	QSPI->BUF0CR_b.MSTRID = 1;
	QSPI->BUF1CR_b.MSTRID = 2;
	QSPI->BUF2CR_b.MSTRID = 3;

	QSPI->BUF3CR_b.ALLMST = 1;
	QSPI->BUF3CR_b.ADATSZ = 1;

	QSPI->BUF0IND = 0x200000;
	QSPI->BUF1IND = 0x400000;
	QSPI->BUF2IND = 0x600000;

	QSPI->SFA1AD = FLASH_BASE + 0x400000;
	QSPI->SFA2AD = FLASH_BASE + 0x800000;
	QSPI->SFB1AD = FLASH_BASE + 0xC00000;
	QSPI->SFB2AD = FLASH_BASE + 0xF00000;
	QSPI->SFAR = FLASH_BASE;

	PORT_PinConfigure(HRF_QSPI_SCK_PORT,HRF_QSPI_SCK_BIT, (PORT_FUNC)(HRF_QSPI_SCK_FUNC|PORT_FAST_RATE));
	PORT_PinConfigure(HRF_QSPI_SS_PORT,HRF_QSPI_SS_BIT, (PORT_FUNC)(HRF_QSPI_SS_FUNC|PORT_FAST_RATE));
	PORT_PinConfigure(HRF_QSPI_WP_IO2_PORT,HRF_QSPI_WP_IO2_BIT, (PORT_FUNC)(HRF_QSPI_WP_IO2_FUNC|PORT_FAST_RATE));
	PORT_PinConfigure(HRF_QSPI_HOLD_IO3_PORT,HRF_QSPI_HOLD_IO3_BIT, (PORT_FUNC)(HRF_QSPI_HOLD_IO3_FUNC|PORT_FAST_RATE));
	PORT_PinConfigure(HRF_QSPI_MISO_IO1_PORT,HRF_QSPI_MISO_IO1_BIT, (PORT_FUNC)(HRF_QSPI_MISO_IO1_FUNC|PORT_FAST_RATE));
	PORT_PinConfigure(HRF_QSPI_MOSI_IO0_PORT,HRF_QSPI_MOSI_IO0_BIT, (PORT_FUNC)(HRF_QSPI_MOSI_IO0_FUNC|PORT_FAST_RATE));
}



void FLASH_CMD_CONFIG(void)
{
		//Read Data(????×???)
	/*LUT0-3*/

	//write enable
	/*LUT4-7*/
	QSPI->LUT_b[4].INSTR0 = CMD;
	QSPI->LUT_b[4].PAD0 = 0x00;
	QSPI->LUT_b[4].OPRND0 = Write_Enable;
	QSPI->LUT_b[4].INSTR1 = STOP;

	//Read SR 1
	/*LUT8-11*/
	QSPI->LUT_b[8].INSTR0 = CMD;
	QSPI->LUT_b[8].PAD0 = 0x00;
	QSPI->LUT_b[8].OPRND0 = Read_SR1;
	QSPI->LUT_b[8].INSTR1 = READ;
	QSPI->LUT_b[8].PAD1 = 0x00;
	QSPI->LUT_b[8].OPRND1 = 0x01;
	
	QSPI->LUT_b[9].INSTR0 = STOP;

	//Write SR
	/*LUT12-15*/
	QSPI->LUT_b[12].INSTR0 = CMD;
	QSPI->LUT_b[12].PAD0 = 0x00;
	QSPI->LUT_b[12].OPRND0 = Write_SR;
	QSPI->LUT_b[12].INSTR1 = WRITE;
	QSPI->LUT_b[12].PAD1 = 0x00;
	QSPI->LUT_b[12].OPRND1 = 0x02;
	
	QSPI->LUT_b[13].INSTR0 = STOP;

	//setup Enable Reset
	/*LUT16-19*/
	QSPI->LUT_b[16].INSTR0 = CMD;
	QSPI->LUT_b[16].PAD0 = 0x00;
	QSPI->LUT_b[16].OPRND0 = Enable_Reset;
	QSPI->LUT_b[16].INSTR1 = STOP;
	QSPI->LUT_b[16].PAD1 = 0x00;
	QSPI->LUT_b[16].OPRND1 = 0x00;

	//setup erase program
	/*LUT20-23*/
	QSPI->LUT_b[20].INSTR0 = CMD;
	QSPI->LUT_b[20].PAD0 = 0x00;
	QSPI->LUT_b[20].OPRND0 = Sector_Erase_4k;
	QSPI->LUT_b[20].INSTR1 = ADDR;
	QSPI->LUT_b[20].PAD1 = 0x00;
	QSPI->LUT_b[20].OPRND1 = 0x18;	

	QSPI->LUT_b[21].INSTR0 = STOP;

	//setup fast read quad io(1st)
	/*LUT24-27*/
	QSPI->LUT_b[24].INSTR0 = CMD;
	QSPI->LUT_b[24].PAD0 = 0x00;
	QSPI->LUT_b[24].OPRND0 = Fast_Read_Quad_Output;
	QSPI->LUT_b[24].INSTR1 = ADDR;
	QSPI->LUT_b[24].PAD1 = 0x00;
	QSPI->LUT_b[24].OPRND1 = 0x18; 

	QSPI->LUT_b[25].INSTR0 = DUMMY;
	QSPI->LUT_b[25].PAD0 = 0x00;
	QSPI->LUT_b[25].OPRND0 = 0x08;
	QSPI->LUT_b[25].INSTR1 = READ;
	QSPI->LUT_b[25].PAD1 = 0x02;
	QSPI->LUT_b[25].OPRND1 = 0x04;
	
	QSPI->LUT_b[26].INSTR0 = JMP_ON_CS;
	QSPI->LUT_b[26].PAD0 = 0x00;
	QSPI->LUT_b[26].OPRND0 = 0x00;
	QSPI->LUT_b[26].INSTR1 = 0;

	//setup block32k erase program
	/*LUT28-31*/
	QSPI->LUT_b[28].INSTR0 = CMD;
	QSPI->LUT_b[28].PAD0 = 0x00;
	QSPI->LUT_b[28].OPRND0 = Volatile_SR_WRITE_Enable;
	QSPI->LUT_b[28].INSTR1 = STOP;

	//setup erase security reg
	/*LUT32-35*/
	QSPI->LUT_b[32].INSTR0 = CMD;
	QSPI->LUT_b[32].PAD0 = 0x00;
	QSPI->LUT_b[32].OPRND0 = Erase_SecurityReg;
	QSPI->LUT_b[32].INSTR1 = ADDR;
	QSPI->LUT_b[32].PAD1 = 0x00;
	QSPI->LUT_b[32].OPRND1 = 0x18;	

	QSPI->LUT_b[33].INSTR0 = STOP;

	//setup Reset
	/*LUT36-39*/
	QSPI->LUT_b[36].INSTR0 = CMD;
	QSPI->LUT_b[36].PAD0 = 0x00;
	QSPI->LUT_b[36].OPRND0 = Reset;
	QSPI->LUT_b[36].INSTR1 = STOP;
	QSPI->LUT_b[36].PAD1 = 0x00;
	QSPI->LUT_b[36].OPRND1 = 0x00;	

	//setup read secyrity regd
	/*LUT40-43*/
	QSPI->LUT_b[40].INSTR0 = CMD;
	QSPI->LUT_b[40].PAD0 = 0x00;
	QSPI->LUT_b[40].OPRND0 = Read_Security_Reg;
	QSPI->LUT_b[40].INSTR1 = ADDR;
	QSPI->LUT_b[40].PAD1 = 0x00;
	QSPI->LUT_b[40].OPRND1 = 0x18;

	QSPI->LUT_b[41].INSTR0 = DUMMY;
	QSPI->LUT_b[41].PAD0 = 0x00;
	QSPI->LUT_b[41].OPRND0 = 0x8;
	QSPI->LUT_b[41].INSTR1 = READ;
	QSPI->LUT_b[41].PAD1 = 0x00;
	QSPI->LUT_b[41].OPRND1 = 0x18;

	QSPI->LUT_b[42].INSTR0 = STOP;

	//Read JEDEC ID
	/*LUT48-51*/
	QSPI->LUT_b[48].INSTR0 = CMD;
	QSPI->LUT_b[48].PAD0 = 0x00;
	QSPI->LUT_b[48].OPRND0 = JEDC_ID;
	QSPI->LUT_b[48].INSTR1 = READ;
	QSPI->LUT_b[48].PAD1 = 0x00;
	QSPI->LUT_b[48].OPRND1 = 0x04;

	QSPI->LUT_b[49].INSTR0 = STOP;
	QSPI->LUT_b[49].PAD0 = 0x00;
	QSPI->LUT_b[49].OPRND0 = 0x00;	


	//Read sr2 spi
	/*LUT52-55*/
	QSPI->LUT_b[52].INSTR0 = CMD;
	QSPI->LUT_b[52].PAD0 = 0x00;
	QSPI->LUT_b[52].OPRND0 = Read_SR2;
	QSPI->LUT_b[52].INSTR1 = READ;
	QSPI->LUT_b[52].PAD1 = 0x00;
	QSPI->LUT_b[52].OPRND1 = 0x01;
	
	QSPI->LUT_b[53].INSTR0 = STOP;

	//setup page program
	/*LUT56-59*/
	QSPI->LUT_b[56].INSTR0 = CMD;
	QSPI->LUT_b[56].PAD0 = 0x00;
	QSPI->LUT_b[56].OPRND0 = Page_Program;
	QSPI->LUT_b[56].INSTR1 = ADDR;
	QSPI->LUT_b[56].PAD1 = 0x00;
	QSPI->LUT_b[56].OPRND1 = 0x18;	

	QSPI->LUT_b[57].INSTR0 = WRITE;
	QSPI->LUT_b[57].PAD0 = 0x00;
	QSPI->LUT_b[57].OPRND0 = 0x20;
	QSPI->LUT_b[57].INSTR1 = STOP;

	//setup program security reg
	/*LUT60-63*/
	QSPI->LUT_b[60].INSTR0 = CMD;
	QSPI->LUT_b[60].PAD0 = 0x00;
	QSPI->LUT_b[60].OPRND0 = Program_SecurityReg;
	QSPI->LUT_b[60].INSTR1 = ADDR;
	QSPI->LUT_b[60].PAD1 = 0x00;
	QSPI->LUT_b[60].OPRND1 = 0x18;	

	QSPI->LUT_b[61].INSTR0 = STOP;

//	FLASH_WriteQeFlag();

	BFGENCR_SEQ(Fast_Read_Quad_IO_1st_ID);
}

void IPCR_SEQ(uint32_t SQEID, uint32_t IDATSZ)
{
	WAIT_REM32(QSPI->SR_b.BUSY,0);
	QSPI->IPCR = (SQEID << 24) | IDATSZ;
	return;
}

void BFGENCR_SEQ(uint32_t SQEID)
{
	WAIT_REM32(QSPI->SR_b.BUSY,0);
	QSPI->BFGENCR_b.SEQID = SQEID;
	return;
}

void Flash_Wait_BUSY(void)
{
	int wait_cnt=350000;//大概等待1.1s的时间 足够当前代码中的任何操作完成了
	for (;wait_cnt>0;wait_cnt--)//大概等待1s
	{
		IPCR_SEQ(Read_SR1_SPI_ID, 0x01);
		WAIT_REM32(QSPI->SR_b.BUSY, 0);
		if ((QSPI->RBDR[0] & 0x00000001) == 0x00000000)
		{
			QSPI->FR_b.RBDF = 1; //pop one rx buffer entry
			break;
		}
		QSPI->FR_b.RBDF = 1; //pop one rx buffer entry
	}
	QSPI->FR_b.RBDF = 1; //pop one rx buffer entry
	QSPI->MCR_b.MDIS=0;
	QSPI->MCR|=QSPI_MCR_SWRSTSD_Msk  |  QSPI_MCR_SWRSTHD_Msk  ; //
	QSPI->MCR_b.MDIS=1;
	QSPI->MCR&=~(QSPI_MCR_SWRSTSD_Msk  |  QSPI_MCR_SWRSTHD_Msk)  ;
	QSPI->MCR_b.MDIS=0;
	if (wait_cnt==0)//超出等待时间 需要复位
	{
		FLASH_Reset();
	}
}

void FLASH_WriteQeFlag(void)
{
	IPCR_SEQ(Read_SR1_SPI_ID, 0x01);
	WAIT_REM32(QSPI->SR_b.BUSY, 0);
	u8 SR1OriVal = QSPI->RBDR[0] & 0XFF;
	QSPI->FR_b.RBDF = 1;

	IPCR_SEQ(Read_SR2_SPI_ID, 0x01);
	WAIT_REM32(QSPI->SR_b.BUSY, 0);
	u8 SR2OriVal = QSPI->RBDR[0] & 0XFF;
	QSPI->FR_b.RBDF = 1;
	/*run Write_Enable_SPI*/
	IPCR_SEQ(Write_Enable_SPI_novolatile_ID, 1);
	//Flash_Wait_BUSY();
	/*QE Enable*/
	//QSPI->TBDR =  0x00000200;;
	QSPI->TBDR = (SR1OriVal | (SR2OriVal << 8) | 1 << 9/*QE bit*/ | 0x0200);
	QSPI->TBDR = 0x00000000;
	QSPI->TBDR = 0x00000000;
	QSPI->TBDR = 0x00000000;

	/*run Write_SR_SPI*/
	IPCR_SEQ(Write_SR_SPI_ID, 0x02);
	WAIT_REM32(QSPI->SR_b.BUSY, 0);
	Flash_Wait_BUSY();
}


/**
  * @brief 
  * @param 
  * @param 
  * @retval 
  */
void FLASH_Init()
{
	WAIT_REM32(QSPI->SR_b.BUSY,0);
}
/**
  * @brief 
  * @param 
  * @param 
  * @retval 
  */
void FLASH_Close()
{

}

/**
  * @brief
  * @param
  * @param 
  * @retval 
  */
uint8_t FLASH_GetFalshStatus(uint8_t regx)
{
	uint8_t retval;
	QSPI->MCR_b.CLR_RXF = 1;
	WAIT_REM32(QSPI->SR_b.BUSY,0);
	switch (regx)
	{
	case 1:
		IPCR_SEQ(Read_SR1_SPI_ID, 0x01);
		break;
	case 2:
		IPCR_SEQ(Read_SR2_SPI_ID, 0x01);
		break;
	default:
		IPCR_SEQ(Read_SR1_SPI_ID, 0x01);
		break;
	}
	WAIT_REM32(QSPI->SR_b.BUSY, 0);
	retval = QSPI->RBDR[0] & 0xFF;
	QSPI->FR_b.RBDF = 1; //pop one rx buffer entry
	return retval;
}
//最后要根据不同flash的写状态寄存器方式再作调整
void FLASH_WriteFlashStatus(uint8_t regx, uint32_t data)
{
	if(regx == 2)
	{
	    /*run Write_Enable_SPI*/
	    IPCR_SEQ(Write_Enable_SPI_volatile_ID, 1);
	    //Flash_Wait_BUSY();
	    /*QE Enable*/
	    //QSPI->TBDR =  0x00000200;;
	    QSPI->TBDR = (data);
	    QSPI->TBDR = 0x00000000;
	    QSPI->TBDR = 0x00000000;
	    QSPI->TBDR = 0x00000000;

	    /*run Write_SR_SPI*/
	    IPCR_SEQ(Write_SR_SPI_ID, 0x02);
	    WAIT_REM32(QSPI->SR_b.BUSY, 0);
	    Flash_Wait_BUSY();
	}
	else
	{
	     /*run Write_Enable_SPI*/
	    IPCR_SEQ(Write_Enable_SPI_volatile_ID, 1);
	    //Flash_Wait_BUSY();
	    /*QE Enable*/
	    //QSPI->TBDR =  0x00000200;;
	    QSPI->TBDR = (data);
	    QSPI->TBDR = 0x00000000;
	    QSPI->TBDR = 0x00000000;
	    QSPI->TBDR = 0x00000000;

	    /*run Write_SR_SPI*/
	    IPCR_SEQ(Write_SR_SPI_ID, 0x02);
	    WAIT_REM32(QSPI->SR_b.BUSY, 0);
	    Flash_Wait_BUSY();
	}
}

void FLASH_WriteFlashStatusNoVolatile(uint8_t regx, uint32_t data)
{
	if(regx == 2)
	{
		/*run Write_Enable_SPI*/
		IPCR_SEQ(Write_Enable_SPI_novolatile_ID, 1);
		//Flash_Wait_BUSY();
		/*QE Enable*/
		//QSPI->TBDR =	0x00000200;;
		QSPI->TBDR = (data);
		QSPI->TBDR = 0x00000000;
		QSPI->TBDR = 0x00000000;
		QSPI->TBDR = 0x00000000;

		/*run Write_SR_SPI*/
		IPCR_SEQ(Write_SR_SPI_ID, 0x02);
		WAIT_REM32(QSPI->SR_b.BUSY, 0);
		Flash_Wait_BUSY();
	}
	else
	{	
		 /*run Write_Enable_SPI*/
		IPCR_SEQ(Write_Enable_SPI_novolatile_ID, 1);
		//Flash_Wait_BUSY();
		/*QE Enable*/
		//QSPI->TBDR =	0x00000200;;
		QSPI->TBDR = (data);
		QSPI->TBDR = 0x00000000;
		QSPI->TBDR = 0x00000000;
		QSPI->TBDR = 0x00000000;

		/*run Write_SR_SPI*/
		IPCR_SEQ(Write_SR_SPI_ID, 0x02);
		WAIT_REM32(QSPI->SR_b.BUSY, 0);
		Flash_Wait_BUSY();
	}
}

/**
  * @brief
  * @param
  * @param 
  * @retval 
  */
void FLASH_EarseBlock(uint32_t block_start)
{ 
	/*run Write_Enable_SPI*/
	IPCR_SEQ(Write_Enable_SPI_novolatile_ID, 1);
	WAIT_REM32(QSPI->SR_b.BUSY, 0);
    QSPI->SFAR = (block_start & 0xFFFFFF) | FLASH_BASE;
	IPCR_SEQ(Sector_Erase_4k_SPI, 0x00);
	WAIT_REM32(QSPI->SR_b.BUSY, 0);
	Flash_Wait_BUSY();

}

/**
  * @brief
  * @param
  * @param 
  * @retval 
  */
void FLASH_ProgramPackageData(uint32_t Package_start, uint32_t *pdata, uint32_t pagewords)
{
	uint32_t dataindex = 0;
	uint32_t pageidx = 0;
	uint32_t offset = 0;
	uint32_t currentdatalen = 0;
	uint32_t leavedatalen = pagewords;
	while (1)
	{
		currentdatalen = leavedatalen > 8 ? 8 : leavedatalen;
		leavedatalen -= currentdatalen;
		offset = Package_start + dataindex * 4;
		QSPI->SFAR = (offset & 0xFFFFFF) | FLASH_BASE;
		QSPI->MCR_b.CLR_TXF = 1;
		for (pageidx = 0; pageidx < currentdatalen; pageidx++)
		{
			QSPI->TBDR = ((uint32_t *)pdata)[dataindex++];
		}
		/*run page program*/
		IPCR_SEQ(Write_Enable_SPI_novolatile_ID, 0x01);
		WAIT_REM32(QSPI->SR_b.BUSY, 0);
		//Flash_Wait_BUSY();

		IPCR_SEQ(Page_program_ID, 0x20);
		WAIT_REM32(QSPI->SR_b.BUSY, 0);
		Flash_Wait_BUSY();
		if (leavedatalen == 0)
			break;
	}
}

/**
  * @brief
  * @param
  * @param 
  * @retval 
  */
void FLASH_ProgramPackageBytes(uint32_t Package_start, uint8_t *pdata, uint32_t bytes)
{
	uint32_t i = 0;
	uint32_t dataindex = 0;
	uint32_t pageidx = 0;
	uint32_t offset = 0;
	uint32_t currentdatalen = 0;
	uint32_t leavedatalen = bytes >> 2;
	uint32_t pagebytes;
	while (1)
	{
		currentdatalen = leavedatalen > 16 ? 16 : leavedatalen;
		leavedatalen -= currentdatalen;
		if(currentdatalen != 0)//page in word
		{
			offset = Package_start + dataindex * 4;
			QSPI->SFAR = (offset & 0xFFFFFF) | FLASH_BASE;
			QSPI->MCR_b.CLR_TXF = 1;
			for (pageidx = 0; pageidx < currentdatalen; pageidx++)
			{
				QSPI->TBDR = ((uint32_t *)pdata)[dataindex++];
			}
			if(currentdatalen%4!=0)//Tx buffer depth is 64 bytes, push data to highest pos
			{
				uint8_t supplyTxBufferWordNUM = 4 - (currentdatalen%4);
				for(int i = 0; i < supplyTxBufferWordNUM; i++)
				{
					QSPI->TBDR = 0x00000000;
				}
			}
			/*run page program*/
			IPCR_SEQ(Write_Enable_SPI_novolatile_ID, 0x01);
			WAIT_REM32(QSPI->SR_b.BUSY, 0);
			//Flash_Wait_BUSY();
			pagebytes = currentdatalen * 4;
			IPCR_SEQ(Page_program_ID, pagebytes);
			WAIT_REM32(QSPI->SR_b.BUSY, 0);
			Flash_Wait_BUSY();
		}
		if (leavedatalen == 0)//less than one word
		{
			if ((bytes & 0x03) > 0)
			{
				uint32_t bytecnt;
				uint32_t datatmp = 0xFFFFFFFF;
				for (i = (bytes & 0x03); i > 0; i--)
				{
					bytecnt = (dataindex << 2) + i-1;
					datatmp = (datatmp << 8) | (*(pdata + bytecnt));
				}
				QSPI->TBDR = datatmp;
				
				for(int i = 0; i < 3; i++)//Tx buffer depth is 64 bytes, push data to highest pos
				{
					QSPI->TBDR = 0x00000000;
				}

				offset = Package_start + dataindex * 4;
				QSPI->SFAR = (offset & 0xFFFFFF) | FLASH_BASE;
				IPCR_SEQ(Write_Enable_SPI_novolatile_ID, 0x01);
				WAIT_REM32(QSPI->SR_b.BUSY, 0);
				//Flash_Wait_BUSY();
				pagebytes = bytes & 0x03;
				IPCR_SEQ(Page_program_ID, pagebytes);
				WAIT_REM32(QSPI->SR_b.BUSY, 0);
				Flash_Wait_BUSY();
			}
			break;
		}
	}
}
/**
  * @brief
  * @param
  * @param 
  * @retval 
  */
void FLASH_GetPackageData(uint32_t Package_start, uint32_t *pdata, uint32_t wordsize)
{
	memcpy((u8 *)(pdata), (u8 *)((Package_start & 0xFFFFFF) | FLASH_BASE), wordsize);
}



//0x0015400B
u32 data_flash_read_flash_id()
{
	u32 flash_id = 0;

	IPCR_SEQ(Read_JEDECID_SPI_ID, 0x03);
	WAIT_REM32(QSPI->SR_b.BUSY, 0);
	flash_id = QSPI->RBDR[0];
	QSPI->FR_b.RBDF = 1; //pop one rx buffer entry

	return flash_id;
}

uint32_t dev_flash_id;
const u8 FlashStatusRegNum = 0xff;
void GetDevFlashID(void)
{
	dev_flash_id = data_flash_read_flash_id();
	int i = 0;
	u32  FlashArray[] =
	{
		0x00144020, //WHXX 1M
		0x00154020, //WHXX 2M
		0x0014605e, //new flash
		0x001528A1, //hrf FuDanWei
		0x0014400b, //XTX 1M
		0x0015400b, //XTX 2M
		0x001440EF,
		0x001540A1,
		0x0018400B,
	};
	for (i = 0; i < sizeof(FlashArray)/4; i++)
	{
		if (dev_flash_id == FlashArray[i])
		{
			if (i > 2) 
			{
				*(u8 *)&FlashStatusRegNum = 2;
			}
			else 
			{
				*(u8 *)&FlashStatusRegNum = 3;
			}
			return;
		}

	}
	while (1)
	{
		__BKPT(1);
	}
}



void FLASH_GetSecurity(uint32_t reg_num, uint32_t addr, uint32_t *pdata, uint32_t wordsize)
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
    
    while(1)
    {
        currentdatalen = leavedatalen > 0x10 ? 0x10:leavedatalen;
        leavedatalen -= currentdatalen;
		if(QSPI->SR_b.RXFULL == 1)//clear rx buffer
		{
			QSPI->MCR_b.CLR_RXF = 1;
		}
		//Flash_Wait_BUSY();
		QSPI->SFAR = ((addr + dataindex * 4) & 0xFFFFFF) | FLASH_BASE;	
		uint32_t readdatalen = currentdatalen * 4;
		IPCR_SEQ(Read_Security_Reg_ID,readdatalen);
		WAIT_REM32(QSPI->SR_b.BUSY,0);
        for(uint32_t i = 0;i<currentdatalen;i++)
        {
            ((uint32_t*)pdata)[dataindex++] = QSPI->RBDR[0];
			QSPI->FR_b.RBDF = 1;
        }
        if(leavedatalen == 0)break;
    }
}

void FLASH_Reset(void)
{

	IPCR_SEQ(Enable_Reset_ID,0x00);
	WAIT_REM32(QSPI->SR_b.BUSY,0);


	IPCR_SEQ(Reset_ID,0x00);
	WAIT_REM32(QSPI->SR_b.BUSY,0);

    volatile int i = 750; //榛樿20us
	i = 750; //60us
	
	while (i--)
    {
        ;
    }
	
	Flash_Wait_BUSY();

}
void FLASH_ResetAtFault(void)
{
    //write enable
	if (data_flash_read_flash_id()==dev_flash_id)
	{
		return;	
	}
	FLASH_Reset();

}
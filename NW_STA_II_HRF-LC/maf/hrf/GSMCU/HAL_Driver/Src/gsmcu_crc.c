#include "gsmcu_crc.h"
#include "protocol_includes.h"

uint32_t Hal_Crc(uint32_t crcmode, uint8_t *pdata, uint32_t datalen)
{
	uint32_t i;
	volatile uint32_t res=0;
	uint32_t *c_pdata;
	uint32_t temp;
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	CRC_CTR = crcmode|CRC_SRCSEL|CRC_BYTSWAP|CRC_RDSWAP;
	CRC_INI = (crcmode == CRC_MODE32)?0xffffffff : 0;
	
	
	c_pdata = (uint32_t *)pdata;
	res=datalen/4;
	for(i =0;i<res;i++)
	{
		temp=*c_pdata;
		CRC_DATI = __REV(temp);   // big endian
		c_pdata++;
	}
	datalen%=4;	
	pdata=(uint8_t*)c_pdata;
	while(datalen)
	{
		CRC_DATI8_0 = *pdata++;
		datalen--;
	}
	
	//CRC_CTR	|= CRC_UPDATE;
	
	res = CRC_RES;
	OS_CRITICAL_EXIT();
	switch(crcmode)
	{
		case CRC_MODE32:
			res=~res;
		break;
		
		case CRC_MODE24:
			res>>=8;
		break;
		
		case CRC_MODE16:
			res>>=16;
		break;		
		
	}	
	
	return res;
}



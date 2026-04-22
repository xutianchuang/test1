#include "gsmcu_trng.h"
//#include <GSMCU_M4.h>
#include <stdint.h>
typedef union {
	volatile uint32_t REG;                                                               
	struct
	{
		volatile  uint16_t HRNG;
		volatile  uint16_t SOURCE_EN: 1;
		volatile  uint16_t POST_PROCESS_EN: 1;
		volatile  uint16_t DONE_IE: 1;
		volatile  uint16_t VMMBPS: 1;
		volatile  uint16_t TEST: 1;
		volatile  uint16_t : 2;
		volatile  uint16_t DONE: 1;
		volatile  uint16_t SAMPLE_DIV: 8;
	} TRNG_b;
}TRNG_Type;
#ifndef CRC_BASE
#define CRC_BASE              0x40400000
#endif
#define TRNG    ((TRNG_Type*)              (CRC_BASE+0x10))


	


	
static void config_trng(uint16_t div)
{
	TRNG->TRNG_b.SAMPLE_DIV=div;
}


static uint16_t  hardware_random(void)
{
	uint16_t trng;
	TRNG->REG |= 1 << 16 | 1 << 17; //SOURCE_EN&POST_PROCESS_EN
	while (!TRNG->TRNG_b.DONE );
	trng = TRNG->TRNG_b.HRNG;
	TRNG->REG = TRNG->REG;//clear flag
	TRNG->TRNG_b.SOURCE_EN=0;
	return trng;

}

uint16_t getTrngRand(void)
{
	config_trng(200);
	return hardware_random();
}

#include"gsmcu_m3_pit32.h"
void PIT32_Config(PIT_CHANNEL_Type* pit, uint32_t tick)
{
	PIT_CONTROL0->MCR_b.MDIS=0;
	PIT_CONTROL1->MCR_b.MDIS=0;
	pit->LDVAL=tick;

}

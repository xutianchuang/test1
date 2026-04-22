#include <gsmcu_hal.h>

#include "System_inc.h"
#include "os.h"



//PLC接收数据队列
static OS_Q s_phyDataQueue;



//设置接收缓冲区
void BplcPhySetBuffer(u8 *buff)
{

}


void PHY_Init(void)
{
	OS_ERR err;

	//*((unsigned int*)0x4000700C) = 0;
	PLC_lower_Init();

	OSQCreate(&s_phyDataQueue, "PHY DataQueue", MSDU_BLOCK_NUM, &err);
	if (err != OS_ERR_NONE)
	{
		SAFE_SYSTEM_RESET(SYS_RESET_PHY_INIT_ERR);
	}


}




#include <string.h>
#include "os.h"
#include "System_inc.h"
#include "common_includes.h"

#ifdef HPLC_CSG
#ifdef GUIZHOU_SJ
//广东清远台体，频段2功率谱密度测试
const u8 HPLC_ChlPower[] = {0,0,0,0};
#endif


const u8 HPLC_PowerOffPower[] = {0,0,0,0};
#if defined(ZB204_CHIP)
const u8 HPLC_ChlPower[] = {3,7,7,0};
const u8 HPLC_HighPower[] = {3,7,7,0};
const u8 HPLC_TestChlPower[] = {0,0,4,4};
const u8 HRF_LinePower = 22;
const u8 HRF_HighLinePower = 17;
const u8 HRF_TestChlPower[] = {16,10};
const u8 HRF_PowerOffPower = 6;
#elif defined(ZB205_CHIP)
const u8 HPLC_ChlPower[] = {10,6,7,6};
const u8 HPLC_HighPower[] = {13,13,13,13};
const u8 HPLC_TestChlPower[] = {10,6,7,6};
const u8 HRF_LinePower = 1;
const u8 HRF_HighLinePower = 3;
const u8 HRF_TestChlPower[] = {0,0};
const u8 HRF_PowerOffPower = 0;
#elif defined(ZB206_CHIP)
const u8 HPLC_ChlPower[] = {7,7,7,7};
const u8 HPLC_HighPower[] = {11,11,11,11};
const u8 HPLC_TestChlPower[] = {4,4,4,0};
#ifdef EFEM_MODE
const u8 HRF_LinePower = 20;
const u8 HRF_HighLinePower = 20;
const u8 HRF_TestChlPower[] = {20,20};
const u8 HRF_PowerOffPower = 20;
#else
const u8 HRF_LinePower = 17;
const u8 HRF_HighLinePower = 16;
const u8 HRF_TestChlPower[] = {16,16};
const u8 HRF_PowerOffPower = 17;
#endif
#endif

#elif defined(PERFORMAINCE_OUT)


#if defined(ZB204_CHIP)
const u8 HPLC_ChlPower[] = {2,4,7,6};
const u8 HPLC_HighPower[] = {2,4,7,6};
const u8 HPLC_TestChlPower[] = {0,0,2,4};
const u8 HPLC_PowerOffPower[] = {0,0,0,0};
const u8 HRF_LinePower = 22;
const u8 HRF_HighLinePower = 17;
const u8 HRF_TestChlPower[] = {16,10};
const u8 HRF_PowerOffPower = 6;
#elif defined(ZB205_CHIP)
const u8 HPLC_ChlPower[] = {12,8,9,8};
const u8 HPLC_HighPower[] = {12,12,12,12};
const u8 HPLC_TestChlPower[] = {10,6,7,6};
const u8 HPLC_PowerOffPower[] = {0,0,0,0};
const u8 HRF_LinePower = 1;
const u8 HRF_HighLinePower = 3;
const u8 HRF_TestChlPower[] = {0,0};
const u8 HRF_PowerOffPower = 0;
#elif defined(ZB206_CHIP)
const u8 HPLC_ChlPower[] = {7,7,7,7};
const u8 HPLC_HighPower[] = {11,11,11,11};
const u8 HPLC_TestChlPower[] = {4,4,4,0};
const u8 HPLC_PowerOffPower[] = {0,0,0,0};
#ifdef EFEM_MODE
const u8 HRF_LinePower = 20;
const u8 HRF_HighLinePower = 20;
const u8 HRF_TestChlPower[] = {20,20};
const u8 HRF_PowerOffPower = 20;
#else
const u8 HRF_LinePower = 17;
const u8 HRF_HighLinePower = 16;
const u8 HRF_TestChlPower[] = {16,16};
const u8 HRF_PowerOffPower = 17;
#endif
#endif

#else


#if defined(ZB204_CHIP)
const u8 HPLC_ChlPower[] = {2,4,7,6};
const u8 HPLC_HighPower[] = {2,4,7,6};
const u8 HPLC_TestChlPower[] = {0,0,2,4};
const u8 HPLC_PowerOffPower[] = {0,0,0,0};
const u8 HRF_LinePower = 22;
const u8 HRF_HighLinePower = 17;
const u8 HRF_TestChlPower[] = {16,10};
const u8 HRF_PowerOffPower = 6;
#elif defined(ZB205_CHIP)
const uint8_t HPLC_ChlPower[] = {5,5,5,5};
const uint8_t HPLC_HighPower[] = {8,8,8,8};
const u8 HPLC_TestChlPower[] = {0,0,0,0};
const u8 HPLC_PowerOffPower[] = {0,0,0,0};
const u8 HRF_LinePower = 5;
const u8 HRF_HighLinePower = 17;
const u8 HRF_TestChlPower[] = {0,0};
const u8 HRF_PowerOffPower = 4;
#elif defined(ZB206_CHIP)
const uint8_t HPLC_ChlPower[] = {11,11,11,11};
const uint8_t HPLC_HighPower[] = {12,12,12,12};
const u8 HPLC_TestChlPower[] = {4,4,4,4};
const u8 HPLC_PowerOffPower[] = {0,0,0,0};
#ifdef EFEM_MODE
const u8 HRF_LinePower = 20;
const u8 HRF_HighLinePower = 20;
const u8 HRF_TestChlPower[] = {20,20};
const u8 HRF_PowerOffPower = 20;
#else
const u8 HRF_LinePower = 17;
const u8 HRF_HighLinePower = 16;
const u8 HRF_TestChlPower[] = {0,0};
const u8 HRF_PowerOffPower = 17;
#endif
#endif
#endif

#ifdef TEST_MODE_USE_BAND1
const u8 USE_BAND1_TESE_MODE=1;
#else
const u8 USE_BAND1_TESE_MODE=0;
#endif

#ifdef PHASE_USE_OLD_PROTOCOL
const u8 USE_OLD_PHASE=1;
#else
const u8 USE_OLD_PHASE=0;
#endif




//static PDU_BLOCK* CurrentReceivePdu = 0;







s16 resetTime=0;
void Reset_CallBack(u32 arg)
{
	if (resetTime<0)
	{
		return;
	}
	resetTime = 10;
}
void Reset_Proc(void)
{
#if defined(RESET_PORT_CCO) || defined(NEW_RESET_PORT_CCO)
	u8 value = 0;
	static u8 resetHighCnt=0;
#ifdef HPLC_CSG
	if (!CCO_Hard_Type) //南网双模终端没有reset引脚
		return;
#endif
#if 0
	if (resetTime)
	{
#ifdef INTER_WATCH_DOG
					#ifndef __DEBUG_MODE
					Reboot_system_now(__func__);
					#endif
					while (1)
					{
						#ifdef __DEBUG_MODE
						FeedWdg();
						#endif
					}
#else
		__disable_irq();
		WDG_SoftUnLock();
		WDG_Enable();
		while (1);
#endif
	}
#endif
	if (resetTime > 0)
	{
		if (--resetTime == 0)
		{
			value = 1;

			if (CCO_Hard_Type)
			{
#if defined(RESET_PORT_CCO)
				value = GPIO_PinRead(RESET_PORT_CCO, RESET_PIN_CCO);
#endif
			}
			else
			{
#if defined(NEW_RESET_PORT_CCO) && defined(HPLC_GDW)  //国网双模
				value = GPIO_PinRead(NEW_RESET_PORT_CCO, NEW_RESET_PIN_CCO);
#endif
			}

			if (value == 0)
			{
				resetTime = -1;
				resetHighCnt=0;
			}
		}
	}
	else if (resetTime < 0)
	{
		if (--resetTime > -6000)
		{
			value = 0;

			if (CCO_Hard_Type)
			{
#if defined(RESET_PORT_CCO)
				value = GPIO_PinRead(RESET_PORT_CCO, RESET_PIN_CCO);
#endif
			}
			else
			{
#if defined(NEW_RESET_PORT_CCO) && defined(HPLC_GDW)  //国网双模
				value = GPIO_PinRead(NEW_RESET_PORT_CCO, NEW_RESET_PIN_CCO);
#endif
			}
			if (value == 1)
			{
				resetHighCnt++;
			}
			else
			{
				resetHighCnt = 0;
			}
			if (resetHighCnt == 10)
			{
#ifdef INTER_WATCH_DOG
				#ifndef __DEBUG_MODE
				Reboot_system_now(__func__);
				#endif
				while (1)
				{
					#ifdef __DEBUG_MODE
					FeedWdg();
					#endif
				}
#else
				__disable_irq();
				WDG_SoftUnLock();
				WDG_Enable();
				while (1);
#endif
			}
		}
		else
		{
			resetTime = 0;
		}
	}
#endif

}

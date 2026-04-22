/*
*
*/
#ifndef __BSP_ZEROCROSS_H__
#define __BSP_ZEROCROSS_H__
#include <gsmcu_hal.h>
#include "gsmcu_m3_gpio.h"

typedef void (*ZeroCrossCallBackFunctionType)(int phase);

#define ZeroCross_Trigger_Rising            (GPIO_RISING_TRIGER)
#define ZeroCross_Trigger_Falling           (GPIO_FALLING_TRIGER)
#define ZeroCross_Trigger_Rising_Falling    (GPIO_BOTHEDGE_TRIGER)
#define MAX_ZC_NUM_PER_PHASE           37
#define MAX_ZC_RECOGNIZE_PHASE         2000
#define CON_FEATURE_COUNT_PER_PHASE    (MAX_ZC_NUM_PER_PHASE)
typedef struct
{
    uint32_t TriggerMode;
    ZeroCrossCallBackFunctionType callfunction;
}BspZeroCrossParamType;
typedef struct
{
	uint32_t zc_ntb[MAX_ZC_RECOGNIZE_PHASE];
	uint16_t Widx;
	uint16_t Ridx;
}phaseRecognizeType;

void ZeroCrossInit(void);
void ZeroCrossOpen(BspZeroCrossParamType *param);
void ZeroCrossClose(void);

#endif


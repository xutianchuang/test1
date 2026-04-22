/*
*******************************************************************************
**
**  This device driver was created by Applilet2 for 78K0R/Kx3
**  16-Bit Single-Chip Microcontrollers
**
**  Copyright(C) NEC Electronics Corporation 2002 - 2007
**  All rights reserved by NEC Electronics Corporation.
**
**  This program should be used on your own responsibility.
**  NEC Electronics Corporation assumes no responsibility for any losses
**  incurred by customers or third parties arising from the use of this file.
**
**  Filename :	RTC.h
**  Abstract :	This file implements device driver for RTC module.
**  APIlib :	Applilet2 for 78K0R/Kx3 V2.10 [31 Jan. 2007]
**
**  Device :	uPD78F1166_A0
**
**  Compiler :	CC78K0R
**
**  Creation date:	2007-12-27
**  
*******************************************************************************
*/
#ifndef _MDRTC_
#define _MDRTC_
#ifndef _DLIB_TIME_USES_64 
#define _DLIB_TIME_USES_64 1
#endif
#include <time.h>
#include "type_define.h"

typedef struct tm local_tm;
//#pragma pack(pop)

/*
*******************************************************************************
**  Function define
*******************************************************************************
*/
MD_STATUS RTC_Run_Check( void );

void RTC_Init( void );
void RTC_CounterEnable( void );
MD_STATUS RTC_CounterGet( struct RTCCounterValue* CounterReadVal );
MD_STATUS RTC_CounterSet( struct RTCCounterValue* CounterWriteVal );

#endif

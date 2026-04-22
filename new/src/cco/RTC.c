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
**  Filename :	RTC.c
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




/*
*******************************************************************************
** Include files
*******************************************************************************
*/
//#include "macrodriver.h"
#include "type_define.h"
#include "system_inc.h"
//#include "user_define.h"
#include "RTC.h"
/* Start user code for include definition. Do not edit comment generated here */
/* End user code for include definition. Do not edit comment generated here */

/*
*******************************************************************************
**  Global define
*******************************************************************************
*/
unsigned long  sys_err_fsub;	/* 副时钟错误状态计数*/

const UCHAR convert24hto12h[36] = {0x12, 0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0,0,0,0,0,0,0x10,0x11,0x32,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0,0,0,0,0,0,0x28,0x29,0x30,0x31};
const UCHAR convert12hto24h[51] = {0, 0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0,0,0,0,0,0,0x10,0x11,0x0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x20,0x21,0,0,0,0,0,0,0x22,0x23,0x12};
/* Start user code for global definition. Do not edit comment generated here */
/* End user code for global definition. Do not edit comment generated here */


/*days for 12 months*/
const unsigned short days_per_month[] = {
    0, 31, 60, 91, 121, 152,
    182, 213, 244, 274, 305, 335,
};

const char monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};

unsigned char Bcd2HexChar(unsigned char bcd);
unsigned char Hex2BcdChar(unsigned char hex);

/*
**-----------------------------------------------------------------------------
**
**  Abstract:
**	This function initializes the RTC module.
**
**  Parameters:
**	None
**
**  Returns:
**	None
**
**-----------------------------------------------------------------------------
*/
void RTC_Init( void )
{
#if 0
	RTCEN = 0;		/* RTC clock supply */
	/* Real-time counter setting */
	RTCE = 0;		/* RTC counter disable */
#endif
}


/*
**-----------------------------------------------------------------------------
**
**  Abstract:
**	This function is used to read the results of real-time counter and store them in the variables.
**
**  Parameters:
**	struct RTCCounterValue* CounterReadVal :	the current real-time counter value(BCD code)
**
**  Returns:
**	MD_OK
**	MD_BUSY
**
**-----------------------------------------------------------------------------
*/
MD_STATUS RTC_CounterGet( struct RTCCounterValue* CounterReadVal )
{
#if 0
	unsigned char i = 0;
	RWAIT = 1;
	NOP();
	NOP();

	while( RWST == 0) 
	{
		i++;

		if(i > RTC_COUNT_PAUSE_DELAY)
			break;
	}

	
	if ( RWST == 0 ) {
		return MD_BUSY;
	}
	
	CounterReadVal->Sec = SEC;
	CounterReadVal->Min = MIN;
	CounterReadVal->Hour = HOUR;
	CounterReadVal->Week = WEEK;
	CounterReadVal->Day = DAY;
	CounterReadVal->Month = MONTH;
	CounterReadVal->Year = YEAR;
	
	RWAIT = 0;
	NOP();
	NOP();

	i = 0;
	while( RWST == 1)
	{
		i++;

		if(i > RTC_COUNT_PAUSE_DELAY)
			break;
	}
	
	if ( RWST == 1 ) {
		return MD_BUSY;
	}
#endif
	return OK;
}

/*
**-----------------------------------------------------------------------------
**
**  Abstract:
**	This function is used to change the real-time counter value.
**
**  Parameters:
**	struct RTCCounterValue CounterWriteVal :	the expected real-time counter value(BCD code)
**
**  Returns:
**	MD_OK
**	MD_BUSY
**
**-----------------------------------------------------------------------------
*/
MD_STATUS RTC_CounterSet( struct RTCCounterValue* CounterWriteVal )
{
#if 0
	unsigned char i = 0;
	RWAIT = 1;
	NOP();
	NOP();
	
	while( RWST == 0) 
	{
		i++;

		if(i > RTC_COUNT_PAUSE_DELAY)
			break;
	}
	
	if ( RWST == 0 ) {
		return MD_BUSY;
	}
	
	SEC = CounterWriteVal->Sec;
	MIN = CounterWriteVal->Min;
	HOUR = CounterWriteVal->Hour;
	WEEK = CounterWriteVal->Week;
	DAY = CounterWriteVal->Day;
	MONTH = CounterWriteVal->Month;
	YEAR = CounterWriteVal->Year;

	RWAIT = 0;
	NOP();
	NOP();
	i = 0;
	while( RWST == 1)
	{
		i++;

		if(i > RTC_COUNT_PAUSE_DELAY)
			break;
	}
	if ( RWST == 1 ) {
		return MD_BUSY;
	}
#endif
	return OK;
}



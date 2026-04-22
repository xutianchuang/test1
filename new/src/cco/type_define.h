#ifndef _TYPE_DEFINE_
#define _TYPE_DEFINE_

//Iontwdm700mp.h

#if 0
#define __FPU_PRESENT             1
#define __MPU_PRESENT             0
#define __NVIC_PRIO_BITS          4    
#define __Vendor_SysTickConfig    0   /*!< Set to 1 if different SysTick Config is used  */

typedef enum IRQn
{
/******  Cortex-M4 Processor Exceptions Numbers ***************************************************/
  NonMaskableInt_IRQn         = -14,    /*!< 2 Non Maskable Interrupt                             */
  MemoryManagement_IRQn       = -12,    /*!< 4 Cortex-M4 Memory Management Interrupt              */
  BusFault_IRQn               = -11,    /*!< 5 Cortex-M4 Bus Fault Interrupt                      */
  UsageFault_IRQn             = -10,    /*!< 6 Cortex-M4 Usage Fault Interrupt                    */
  SVCall_IRQn                 = -5,     /*!< 11 Cortex-M4 SV Call Interrupt                       */
  DebugMonitor_IRQn           = -4,     /*!< 12 Cortex-M4 Debug Monitor Interrupt                 */
  PendSV_IRQn                 = -2,     /*!< 14 Cortex-M4 Pend SV Interrupt                       */
  SysTick_IRQn                = -1,     /*!< 15 Cortex-M4 System Tick Interrupt                   */

/******  specific Interrupt Numbers *********************************************************/
  SYSC_IRQn			= 0,
  WWDG_IRQn			= 1,
  IWDG_IRQn			= 2,
  TMR0_1_IRQn			= 3,
  TMR0_2_IRQn			= 4,
  TMR0_3_IRQn			= 5,
  TMR1_1_IRQn			= 6,
  TMR1_2_IRQn			= 7,
  TMR1_3_IRQn			= 8,
  TMR2_1_IRQn			= 9,
  TMR2_2_IRQn			= 10 ,
  TMR2_3_IRQn			= 11,
  TMR2_4_IRQn			= 12,
  TMR2_5_IRQn			= 13,
  TMR2_6_IRQn			= 14,
  DMAC_IRQn			= 15,
  DMAC_TC_IRQn			= 16,
  UART0_IRQn			= 17,
  UART1_IRQn			= 18,
  UART2_IRQn			= 19,
  GPIO0_IRQn			= 20,
  GPIO1_IRQn			= 21,
  IIC0_IRQn			= 22,
  UART3_IRQn			= 23,
  IIC1_IRQn			= 24,
//;; 25,
  SSP0_IRQn			= 26,
  SSP1_IRQn			= 27,
  ADCC_IRQn			= 28,
//;; 29,
  SPI0_IRQn			= 30,
  BPLC_IRQn			= 31,
//;;31,
  CRC_IRQn			= 32,
  MAC_IRQn			= 33,
  AES_IRQn			= 34,
  CRY_IRQn			= 35,
  SHA_IRQn			= 36,
//;; 37-70
  EXTI0_IRQn			= 71,
  EXTI1_IRQn			= 72,
  EXTI2_IRQn			= 73,
  EXTI3_IRQn			= 74,
  EXTI4_IRQn			= 75,
  EXTI5_IRQn			= 76,
  EXTI6_IRQn			= 77,
  EXTI7_IRQn			= 78,
  EXTI8_IRQn			= 79,
  EXTI9_IRQn			= 80,
  EXTI10_IRQn			= 81,
  EXTI11_IRQn			= 82,
  EXTI12_IRQn			= 83,
  EXTI13_IRQn			= 84,
  EXTI14_IRQn			= 85,
  EXTI15_IRQn			= 86,
  EXTI16_IRQn			= 87,
  EXTI17_IRQn			= 88,
  EXTI18_IRQn			= 89,
  EXTI19_IRQn			= 90,
  EXTI20_IRQn			= 91,
  EXTI21_IRQn			= 92,
  EXTI22_IRQn			= 93,
  EXTI23_IRQn			= 94,
  EXTI24_IRQn			= 95,

} IRQn_Type;

#include "core_cm4.h"
#include <stdint.h>

typedef int32_t  s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef const int32_t sc32;  /*!< Read Only */
typedef const int16_t sc16;  /*!< Read Only */
typedef const int8_t sc8;   /*!< Read Only */

typedef __IO int32_t  vs32;
typedef __IO int16_t  vs16;
typedef __IO int8_t   vs8;

typedef __I int32_t vsc32;  /*!< Read Only */
typedef __I int16_t vsc16;  /*!< Read Only */
typedef __I int8_t vsc8;   /*!< Read Only */

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef const uint32_t uc32;  /*!< Read Only */
typedef const uint16_t uc16;  /*!< Read Only */
typedef const uint8_t uc8;   /*!< Read Only */

typedef __IO uint32_t  vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t  vu8;

typedef __I uint32_t vuc32;  /*!< Read Only */
typedef __I uint16_t vuc16;  /*!< Read Only */
typedef __I uint8_t vuc8;   /*!< Read Only */


//ntwDM7xx_hal_def.h

typedef enum 
{
  ERROR = 0, 
  SUCCESS = !ERROR
} ErrorStatus;

//ntwDM7xx_hal.h
typedef enum {false = 0, true = !false} bool;
#define BOOL        bool
#define TRUE        true
#define FALSE       false

#endif

/* data type defintion */

#define OK   SUCCESS

typedef	unsigned long	ULONG;
typedef	unsigned int	UINT;
typedef	unsigned short	USHORT;
typedef	unsigned char	UCHAR;
//typedef	unsigned char	BYTE;
//typedef	unsigned char	BOOL;

typedef unsigned char 		U8;
//typedef unsigned char 		u8;
//typedef unsigned char 		bool;
typedef unsigned short 	U16;
typedef unsigned long 		U32;
//typedef unsigned long u32;
typedef unsigned long tick;

typedef signed char  	S8;
typedef signed short 	S16;
//typedef unsigned short u16;
typedef signed long  	S32;
typedef unsigned int 	BIT_FIELD;


typedef void *pvoid;
typedef void VOID;
typedef void *PVOID;
typedef unsigned char* PBYTE;


#define MD_STATUS		unsigned short

//#define	TRUE	1
//#define	FALSE	0

#define	FAILURE		0
//#define	SUCCESS	1
#define	INITIAL		2

#define PIN_LOW		0
#define PIN_HIGH	1

#define NULL_PTR               0

#define SENDED	2
#define ALLOC	1
#define FREE		0

//#define NULL	    (PVOID)0		
//#define NULL	    0
#define HANDLE	    PVOID


//#define DEV_NORMAL			1			//设备初始化正常
//#define DEV_NONE_EXISTENT 	0			//设备不存在

#define LOWORD(v)	((U16)(v))
#define HIWORD(v)	((U16)((v) >> 16))

#define LOBYTE(v)	((v) & 0xff)
#define HIBYTE(v)	(((v) >> 8) & 0xff)

#define SIZEINWORDS(v)	(((v) & 0x3) ? (((v)>>2) + 1) : ((v)>>2))

#define MAX(a,b) (((a) < (b)) ? (b) : (a))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

//#define K 			1024
//#define M			(unsigned long)K*K

#define	ARRAY_COUNT(_arr)	(sizeof(_arr)/sizeof((_arr)[0]))

//typedef BOOL (*CALLBACK)(HANDLE);
//typedef BOOL (*TIMER_CALLBACK)(HANDLE, HANDLE);
//typedef void (* PROC_FUNC)( void);


//与主站上行协议处理函数入口
typedef  unsigned short (*UPPER_PROTOCOL_PROC)(unsigned char end_id,  pvoid h);

// CALLBACK
typedef unsigned char (* TIME_CALLBACK_PTR)(void* HANDLE);


struct RTCCounterValue 
{
	UCHAR Sec;
	UCHAR Min;
	UCHAR Hour;
	UCHAR Week;
	UCHAR Day;	
	UCHAR Month;
	UCHAR Year;
};
#endif

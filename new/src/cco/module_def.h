
#ifndef _Module_Def_H_
#define _Module_Def_H_

//#pragma pack(push,1)

/*
#ifndef PRINT_HIST_LOG
#ifdef PRINT_HIST_LOG

#undef PRINT_HIST_LOG
#define PRINT_HIST_LOG
*/

//#define MODULE_GDW2005_DEF 
//#undef MODULE_GDW2005_DEF 

/*
 *  System Constant: Process ID
 */
enum  Process_ID  /* enumeration of process ID */
{
	
	PID_STOR,		   /* storage module */

	//PID_TASK,	       /* message queue module      */

	//PID_AMR,     	   /* DL645 Module                  */

	PID_GDW2005,       /* 1  POWER LOADING MANAGERMENT                  */

	PID_PLC_BOARD,	    //载波模块

    //自动中继模块，用于遍历寻找载波自动中继路由。
	PID_AUTO_PLC,		/* 自动中继模块*/

	PID_BUTT,           /* End of Module ID Definition */
} ;

enum  Device_ID  /* enumeration of process ID */
{
	DID_FRAM,         

	DID_DATAFLASH,         

	DID_BUTT,            /* End of Module ID Definition */
} ;



typedef unsigned short (*SYS_INIT_PROC)(HANDLE);
typedef unsigned short (*SYS_MSG_PROC)(HANDLE);

typedef struct  module
 { 	
	unsigned char            module_ID;
//	unsigned char            device_ID;

	// 各模块初始化函数	
	SYS_INIT_PROC   module_init_function;

	// 事件处理函数	
	SYS_MSG_PROC   module_process;
 }module_s;

//extern BOOL	SYS_AMR_Init(HANDLE h);
//extern BOOL	SYS_AMR_proc(HANDLE h);

//#pragma pack(pop)

#endif



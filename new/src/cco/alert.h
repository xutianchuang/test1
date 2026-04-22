
#ifndef _ALERT_H_
#define _ALERT_H_

//#pragma pack(push,1)

#define ALERT_RESET_ERROR_TIMES    5

#define ALERT_FILE_NAME_SIZE		24

#define ALERT_NO_ACTION			0
#define ALERT_RESET_DEVICE			1
#define ALERT_ERROR_TIMES			2

#define ALERT_NO_MEMORY			    0xE001
#define ALERT_NO_MIAL_MEMORY		0xE002	//无法申请新的消息
#define ALERT_UNEX_VALUE			0xE003
#define ALERT_FRAM_INIT_FAIL		0xE004	//FRAM初始化失败
#define ALERT_FULL_QUEUE			0xE005
#define ALERT_NO_MSG_MEMORY			0xE006	//无法申请新的UART block
#define ALERT_MODULE_INIT_FAIL		0xE007	//模块初始化失败
#define ALERT_DB_INIT_FAIL		    0xE008	//data flash初始化失败
#define ALERT_FRAME_RX_FAIL		    0xE009	//数据接受溢出
#define ALERT_MSG_PROC_TIMEOUT	    0xE00A	//数据接受溢出
#define ALERT_PLC_BOARD_FAIL	    0xE00B	//数据接受溢出
#define ALERT_TASK_ERROR     	    0xE00C	
#define ALERT_GROUP_RANGE_ERROR	    0xE00D	
#define ALERT_NO_ALERT		        0xEEEE	//

#define ALERT_YES_ALERT		        0xBCBD	//


typedef struct _alert_info__
{
    U32 alert_flag;
	U32 alert_type;
	U32 line_num;
    U8  occur_time[6];
	U8  file_name[ALERT_FILE_NAME_SIZE];
}ALERT_INFO;

//#pragma pack(pop)

extern U16 alert_sn;
extern U32 alert_num;
extern ALERT_INFO  g_alert_buf[ALERT_BUFFER_SIZE];

void Alert(U16 alert_type, U8 alert_act, S8 * file_name, U32 line_num);

#endif

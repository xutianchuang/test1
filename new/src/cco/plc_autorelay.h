#ifndef _PLC_RELAYPATH_H_
#define _PLC_RELAYPATH_H_

#include "hplc_mac_def.h"
#include"RTC.h"
//#include "System_inc.h"

#define NTB_FREQ         (25000000UL)           //40纳秒

#define DI_ACT_POSI_ENER_97                 0x9010
#define DI_ACT_POSI_ENER_BLK_97             0x901f

#define DI_ACT_POSI_ENER_LAST_DAY_97        0x9010

#if defined(APP_CUS_JICHENG_97)
#define DI_ACT_POSI_ENER_LAST_DAY_BLK_97    0x9a1f
#elif defined(APP_CUS_SANXING_NEIMENG_97)
#define DI_ACT_POSI_ENER_LAST_DAY_BLK_97    0x901f
#else
#define DI_ACT_POSI_ENER_LAST_DAY_BLK_97    0xe611
#endif

#define DI_READ_SHORT_FRAME_97              0xffff      //短帧学习标识符

#define DI_ACT_POSI_ENER_07                 0x00010000

#if defined(APP_CUS_WEISHENG_HUNAN)
#define DI_ACT_POSI_ENER_BLK_07             0x0002ff00  //测试用,威胜集中器 
#else
//#define DI_ACT_POSI_ENER_BLK_07             0x05060201 //测试用
#define DI_ACT_POSI_ENER_BLK_07             0x0001ff00
#endif
#define DI_ACT_NEGA_ENER_BLK_07             0x0002ff00

#define DI_ACT_POSI_ENER_LAST_DAY_BLK_07    0x05060101
#define DI_ACT_POSI_ENER_LST2_BLK_07        0x05060102
#define DI_ACT_NEGA_ENER_LAST_DAY_BLK_07    0x05060201
#define DI_REA_POSI_ENER_LAST_DAY_BLK_07    0x05060301
#define DI_REA_NEGA_ENER_LAST_DAY_BLK_07    0x05060401

#define DI_FREEZE_DATE_LST1_07              0x05060001
#define DI_FREEZE_DATE_LST2_07              0x05060002
#define DI_MONEY_LEFT_07                    0x00900200      //剩余金额
#define DI_MONEY_CREDIT_07                  0x00900201      //透支金额
#define DI_BUY_COUNT_07                     0x00900202      //购电次数
#define DI_MONEY_USED_07                    0x0090020e      //累计用电金额
#define DI_READ_METER_NO                    0x04000402      //读电表表号  35 37 33 37

#define DI_READ_SHORT_FRAME_07              0xffffffff      //短帧学习标识符
#define DI_READ_NODES_AROUND_07             0xfffffffe      //读周围节点
#define DI_READ_NODE_VERSION_07             0xfffffffd      //读节点版本

#define DI_READ_METER_COUNT_GD              0xffffff02       //EA 06 21 01
#define DI_READ_METER_LIST_GD               0xffffff03       //EA 04 21 02
#define DI_TRANS_FILE_BEGIN_GD              0xffffff0b       //EA 05 24 01
#define DI_TRANS_FILE_CONTENT_GD            0xffffff0c       //EA 05 24 02
#define DI_REQUEST_FILE_INFO_GD             0xffffff0d       //EA 06 24 03
#define DI_REQUEST_FILE_PROGRESS_GD         0xffffff0e       //EA 06 24 04

//0xfffffff01
//0xfffffff02
//0xfffffff03
//0xfffffff04

#define DL645_RESP_MAX_LEN   240

#define EVENT_COLLECT_REPORT_INTERVAL	10

#define PLC_RX_WAIT_QUEUE_SIZE    50
#define PLC_MAX_NID_NEIGHBOR_COUNT    12

#define PLC_MAX_PARALLEL_NUM      50

#define PLC_BROADCAST_QUEUE_SIZE   5

#define PLC_RX_WAIT_MAX_TIMER     40  //单位秒，如果超时没有回响应则从等待队列中删除该消息


#define PLC_SAVE_TIMER_COUNT    (ULONG)(OS_TMR_CFG_TICKS_PER_SEC*60*60*6L-3L)  //6小时
//#define PLC_SAVE_TIMER_COUNT    (ULONG)(OS_TMR_CFG_TICKS_PER_SEC*60*2)  //2分钟，测试flash

#define PLC_RE_TIMER_COUNT    (ULONG)(10L*60*60*72L)  //72小时

#define PLC_RELAY_RESET_TIMER    (USHORT)(24L*60*4L)  //4天

#define PLC_MAX_TASK_DELAY      (180*TICK_CNT_PER_SECOND)

#define PLC_MED_TASK_DELAY      (90*TICK_CNT_PER_SECOND)

#define CCTT_CONTROL_97_DIR_MASK   0x80
#define CCTT_CONTROL_CODE_MASK   0x1F
#define CCTT_CONTROL_97_REJECT_MASK   0x40

#define FRM_CTRW_97_READ_SLVS_DATA                     	0x01  //主机发出命令读数据
#define FRM_CTRW_97_WRITE_SLVS_DATA                    	0x04   //主机发出命令写数据
#define FRM_CTRW_97_READ_MNUM                           0x16
#define FRM_CTRW_97_WRITE_PASSWORD                     	0x0F   //修改密码
#define FRM_CTRW_97_CLR_PWRDATA                        	0x09
#define FRM_CTRW_97_CLR_METER_DATA                     	0x6B
#define FRM_CTRW_97_CLR_PASSWORD                       	0x6F
#define FRM_CTRW_97_PWR_FREEZE                        		0x07
#define FRM_CTRW_97_REVISE_TIME                        	0x08 //广播校时
#define FRM_CTRW_97_WRITE_MNUM                         	0x0A //写设备地址
#define FRM_CTRW_97_BAUD_RATE                          		0x0C //更改通信率率
#define FRM_CTRW_97_CLR_REQ_PWRDATA                    	0x10
#define FRM_CTRW_97_READ_LOAD_CURVE                    	0x11
#define FRM_CTRW_97_CLR_ZT_DATA                        	0x6C
#define FRM_CTRW_97_PRI_FRMAE                          	0x1E //和顶板通讯的私有协议祯


#define FRM_CTRW_07_READ_SLVS_DATA                     	0x11  //主机发出命令读数据
#define FRM_CTRW_07_WRITE_SLVS_DATA                    	0x14   //主机发出命令写数据


#define FRM_DL645_97_START_BYTE1                             		0	
#define FRM_DL645_97_ADDR                                    		1  
#define FRM_DL645_97_START_BYTE2                             		7
#define FRM_DL645_97_COMMAND                                 		8
#define FRM_DL645_97_DATALEN                                 		9
#define FRM_DL645_97_DATA                                    		10		/* 数据区的开始位置 */



#define READ_ITEM_FAILURE                0x0
#define READ_ITEM_FINISH                 0x1
#define READ_ITEM_READABLE               0x2


#define READ_ITEM_DIRECT_READ               0x3
#define READ_ITEM_SHORTFRAME            0x4
// read done for below reason
#define READ_ITEM_HAVE_READ              0x5
#define READ_ITEM_DONT_READ              0x6
#define READ_ITEM_TIMEOUT               0xFF


#define FRM_PLC_ADDR_LEN_HARBIN					6

//
#define FRM_PLC_DATATYPE_HARBIN					2
#define FRM_PLC_DATALEN_HARBIN					3
#define FRM_PLC_SRCADDR_HARBIN					4
#define FRM_PLC_RELAYADDR_HARBIN				( FRM_PLC_SRCADDR_HARBIN + (FRM_PLC_ADDR_LEN_HARBIN) )
#define FRM_PLC_DATA_HARBIN(relay_level)		( FRM_PLC_RELAYADDR_HARBIN + (FRM_PLC_ADDR_LEN_HARBIN*relay_level) )

#define PLC_TX_QUEUE_SIZE		30


#define PLC_DELAY_TIME_PER_LEVEL   ((USHORT)50)    //单位0.1s, 默认5s
#define PLC_DELAY_TIME_UPPER_LEVEL   ((USHORT)40)    //单位0.1s, 默认4s

#define PLC_FREEZE_FIND_SON             0
#define PLC_FREEZE_FIND_BROTHER         1

// MACRO DEFINITION
#define HARBIN_PLC_FRAMETYPE_FXXC  	0x3
#define HARBIN_PLC_FRAMETYPE_HDLC  	0x2

#define HARBIN_PLC_IFC_REDUCED_DL645 	0x0
#define HARBIN_PLC_IFC_TRANSPARANT 	0x1

#define HARBIN_PLC_SA_NOHAVE 	0x0
#define HARBIN_PLC_SA_HAVE 		0x1
//标示当前载波协议是否包含'源地址': '0x1'-含源地址；'0x0'-不含源地址
#define HARBIN_PLC_SA_HAVE_TYPE 	HARBIN_PLC_SA_HAVE

#define HARBIN_PLC_RESPONSETYPE 	0x0
#define HARBIN_PLC_COMMANDTYPE 	0x1

#define PLC_RECV_FRAME_PROC_OK    MSG_AMR_READING_PLC_OK_RESP
#define PLC_RECV_FRAME_PROC_NON   MSG_AMR_READING_PLC_NON_RESP
#define PLC_POLL_TASK_PROC        (MSG_AMR_READING_PLC_NON_RESP+1)

#define PLC_TASK_GROUP          PID_BUTT
#define PLC_TASK_FREEZE         (PID_BUTT+1)
#define PLC_TASK_SEARCH         (PID_BUTT+2)
#define PLC_TASK_READ           (PID_BUTT+3)   //表号不存在的点抄
#define PLC_TASK_DIR_READ       (PID_BUTT+4)   //队列中点抄
#define PLC_TASK_TRANS          (PID_BUTT+5)   //02 F1 透明转发

/**************************************************************************
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
该结构体对应的数据会保存到dataflash中，修改该结构体会影响到存储的数据
在1207表的数量下，总的数据已经接近64k，更改结构体小心数据越界
***************************************************************************/
#define MAX_CCO_NUM 50


// autoPLC载波自动路由功能启动标示 !!
#define  AUTOPLC_MODULE_ENABLE      TRUE    // 启动
//#define  AUTOPLC_MODULE_ENABLE      FALSE   // 不启动


#define AUTOPLC_SHORT_TIMEOUT		10		/* 10s   短超时*/
//#define AUTOPLC_SHORT_TIMEOUT		10		/* 10s   短超时*/
#define AUTOPLC_LONG_TIMEOUT		30		/* 30s 长超时*/

#define  AUTOPLC_CONCENTRATOR_ID  0xBBBB

#define PLC_MAX_GROUP_NUM         150  //240

#define MAX_METER_NUM_7_GROUP       500
#define MAX_METER_NUM_0_GROUP       150


/*HRF*/ 
#define HRF_CCO_MAX   20
#define HRF_OLD_PERIOD              9
#define HRF_HRF_PERIOD_SEND_CNT     5
/*HRF*/ 

#define GET_METER_ROUTER_POS(meterno) (((g_PlmConfigPara.meter_router_pos[meterno>>2] >> ((meterno & 0x03)) * 2)) & 0x03)

//=OK 代表冻结成功
#define PLC_IS_FREEZE(meterno)        (((g_pRelayStatus->s_freeze_flag[meterno>>3] >> ((meterno & 0x07)))) & 0x01)
#define PLC_SET_FREEZE_OK(meterno)    ((g_pRelayStatus->s_freeze_flag[meterno>>3]) |= (UCHAR)((0x01 << (meterno & 0x07))))
#define PLC_SET_FREEZE_NO(meterno)    ((g_pRelayStatus->s_freeze_flag[meterno>>3]) &= (UCHAR)(~(0x01 << (meterno & 0x07))))
#define PLC_CLR_ALL_FREEZE()           memset_special(g_pRelayStatus->s_freeze_flag, 0, sizeof(g_pRelayStatus->s_freeze_flag))

#define PLC_IS_DIRREAD_TASK(pTask) ((pTask)->task_header.task_mode.priority<=0)

typedef enum 
{
    AutoRelay_Depth_0 = 0,	/*集中器直连*/
    AutoRelay_Depth_1,	       /*需要一级中继路由*/
    AutoRelay_Depth_2,	       
    AutoRelay_Depth_3,	       
    AutoRelay_Depth_4,
    AutoRelay_Depth_5,
    AutoRelay_Depth_6,
    AutoRelay_Depth_7,

    AutoRelay_Depth_butt,	       
    AutoRelay_Depth_NOT0 = 0xFE,	 //不是零级 
    AutoRelay_Depth_Invalid = 0xFF,	       

}e_Relay_DepthLevel;	



/* 到DL645 消息类型 */
typedef enum MSG_TO_AMR
{
    MSG_AMR_REALTIME_READING,		       /*实时召测*/    
    MSG_AMR_READING_PLC_OK_RESP,
    MSG_AMR_READING_PLC_NON_RESP,
    MSG_AMR_SEND_METER_RESP,
    //
    MSG_CCTT_CMD_BUTT,
}MSG_TO_AMR_S;


typedef enum 
{
    AutoRelay_Status_NoStart = 0,	/*未启动*/
    AutoRelay_Status_OnSearching,	    /*正在进行中*/
    AutoRelay_Status_Completed,	/*已结束*/
    
    AutoRelay_Status_butt,	       

}e_Relay_SearchStatus;	


/* 到storage 消息类型 */
typedef enum MSG_TO_AUTOPLC
{
   MSG_PLC_READING_RESP,
}MSG_TO_AUTOPLC_S;

typedef enum 
{
    AutoRelay_Stage_Group_NoStart = 0,	/*未启动*/
    AutoRelay_Stage_Group_Searching,	    /*正在进行中*/
    AutoRelay_Stage_Group_Init,
    AutoRelay_Stage_Group_Completed,	/*已结束*/
    
    AutoRelay_Stage_Group_butt,	       

}e_Relay_Group_Stage;	


typedef enum 
{
    Relay_Stage_init = 0,	/*未启动*/
    Relay_Stage_Idle,
    Relay_Stage_Start,
    Relay_Stage_Grouping,	    /*正在进行中*/
    Relay_Stage_Freezing,
    Relay_Stage_Searching,	/*已结束*/    
    Relay_Stage_butt,	       

}e_Relay_Stage;	

typedef enum 
{
    Relay_Running = 0,	/*未启动*/
    Relay_Run_Idle,    
    Relay_Run_butt,	       

}e_Run_State;

typedef enum 
{
    AutoRelay_Stage_Freeze_Idle = 0,	    /*未启动*/
    AutoRelay_Stage_Freeze_Start,	    /*正在进行中*/
    AutoRelay_Stage_Freeze_Going,
    AutoRelay_Stage_Freeze_Zero_search,
    AutoRelay_Stage_Freeze_Upper_Init,
    AutoRelay_Stage_Freeze_Upper_Proc,
    AutoRelay_Stage_Freeze_Local_Init,
    AutoRelay_Stage_Freeze_Local_Proc,
    //AutoRelay_Stage_Freeze_Found,	/*已结束*/
    
    AutoRelay_Stage_Freeze_butt,	       

}e_Relay_Freeze_Stage;	

typedef enum 
{
    AutoRelay_Stage_Search_Idle = 0,	    /*未启动*/
    AutoRelay_Stage_Search_Start,	    /*正在进行中*/
    AutoRelay_Stage_Search_Find_Group,
    AutoRelay_Stage_Search_Going,
    AutoRelay_Stage_Search_Change_Proc,
    AutoRelay_Stage_Search_Completed,	/*已结束*/
    
    AutoRelay_Stage_Search_butt,	       

}e_Relay_Search_Stage;	

/*
typedef enum 
{
    EM_FACTORY_DR,
    EM_FACTORY_DR_ESY,
    EM_FACTORY_DX,
    EM_FACTORY_SL,
}EM_FACTORY_FLAG;	
*/

typedef enum 
{
    EM_PROTOCOL_N12,
    EM_PROTOCOL_3762,
}EM_PROTOCOL_FLAG;

typedef enum 
{
    EM_REPORT_ENABLED,
    EM_REPORT_DISABLED, //默认不开启上报
}EM_REPORT_SWITCH;

typedef enum 
{
    EM_SELFREG_INIT,
    EM_SELFREG_START,

    EM_SELFREG_SEND_START,
    EM_SELFREG_WAIT_SEND_START,
    //超时前，反复以下两个步骤，直到上报完成，或者持续时间 >= 表数量*120秒
    EM_SELFREG_SEND_READ,
    EM_SELFREG_WAIT_SEND_READ,

    EM_SELFREG_REPORT_STOP,     //上报路由工况，搜表结束

    //反复以下两个步骤若干次
    EM_SELFREG_SEND_OFFLINE,    //发送离线指示
    EM_SELFREG_WAIT_SEND_OFFLINE,
    EM_SELFREG_SEND_STOP,       //发送主动注册结束
    EM_SELFREG_WAIT_SEND_STOP,

    EM_SELFREG_STOP             //搜表结束

}EM_SELFREG_STATE;


typedef enum __EM_GET_SN_STATE
{
	EM_GETS_SN_INIT,
	EM_GETS_SN_FIND,
	EM_GETS_SN_SEND,
	EM_GETS_SN_END,
}EM_GET_SN_STATE;

enum _MF_RUN_MODE
{
    MF_RUN_MODE_INTERTASK = 0x00,  // default, must be 0
    MF_RUN_MODE_INTERTASK3D5 = 0x01, 
    MF_RUN_MODE_3PHASE = 0x02
};

typedef struct _stat_direct_read_
{		
    unsigned short cmd_recved;
    unsigned short cmd_unknown;
    unsigned short forge_resp;
    unsigned short forge_sent;
    unsigned short timeout_resp;
    unsigned short timeout_sent;
    unsigned short succeed_resp;    
    unsigned short succeed_sent;
    
}STAT_DIRECT_READ, *P_STAT_DIRECT_READ;

#define LIST_TYPE_WHITE 0
#define LIST_TYPE_BLACK 1

#pragma pack(push,1)


typedef struct 
{
    unsigned char addr[6];      
    unsigned char relay_level;                      
    unsigned char relay_meter[MAX_RELAY_DEEP][6];    
}RT_RELAY, *P_RT_RELAY;

struct mf_meter_sta
{
unsigned char used:2;
unsigned char rsvd:1;           //0错相，1不错相
unsigned char selfreg:1;        //1主动注册，0非主动注册
#ifdef GDW_ZHEJIANG
unsigned char IsMeter:1;		//该白名单是电能表标志位,0：该节点为电表 1：该节点不是电表
unsigned char IsCollector:1;	//该白名单是采集器标志位,0：该节点为采集器 1：该节点不是采集器
#endif
unsigned char plc:1;
unsigned char cfged:1;          //1是集中器下发的档案   0是主动注册的档案
unsigned char phase;
};

struct mf_meter_type
{
    /*
#define METER_TYPE_MASK_PROTOCOL            0x01

#define METER_TYPE_MASK_COLLECTION          0x02

#define METER_TYPE_MASK_RELAY               0x04  //该表为中继表或使用中继

#define METER_TYPE_MASK_WATER               0x08
    */
    unsigned char protocol:2;           //1,97规约；0, 07规约   METER_TYPE_PROTOCOL_97, METER_TYPE_PROTOCOL_07, METER_TYPE_PROTOCOL_69845, METER_TYPE_PROTOCOL_TRANSPARENT
    unsigned char m0c1:1;               //METER_TYPE_METER,测量点；METER_TYPE_COLL, 采集器
    unsigned char bRelay:1;             //1使用中继
    unsigned char phaseType:1;          //METER_TYPE_SINGLE_PHASE 单相表，METER_TYPE_THREE_PHASE 三相表
#if 0
    // Dl645  resesp bit5, indicate event on
    // 00 no_event; 11 event reported 
    // 01 event not; reported 10 no use/clear to 00
    unsigned char eventOccur_Reported:2;
#else
    unsigned char xiaocheng:1;          //1华越小程表计，0其它标记
    unsigned char reserve:1;
#endif
    unsigned char cjq_with_add:1;
//    unsigned char reserved:1;
};

typedef struct
{
    //数组下标：原始目的 TEI
    //一共32个字节
    struct mf_meter_sta status;
    //bit0: 1,97规约；0, 07规约 
    //bit1: 1,带采集器; 0,不带采集器
    //unsigned char meter_type;
    struct mf_meter_type meter_type;
    unsigned short user_sn:11;        //集中器下发档案配置的序号
    unsigned short del_flag:1;        //集中器删除标志。0表示集中器命令删除的表，需要在一段时间后把该表真正删除
    unsigned short bridge_flag:1;           //1载波桥，0非载波桥
	unsigned short freeze_done:1;           //0未冻结  1冻结完成
	unsigned short power_status:2;        //电表停复电带电状态; 0-停电，1-带电

    unsigned char meter_addr[CCTT_METER_ADDR_LEN];

    unsigned char cco_num;             //MAX_CCO_NUM 代表是本台区 MAX_CCO_NUM + 1代表是其他台区但是没有位置存, mac地址 <MAX_CCO_NUM 其他台区代表已知mac地址
#if defined(PROTOCOL_GD_2016)
    unsigned char task_id_num;                                      //任务数 0~CCTT_X4_READ_TASK_NUM_PER_METER
    unsigned char task_id_array[CCTT_X4_READ_TASK_NUM_PER_METER];     //X4采集任务
//    unsigned char reserve2[9];
#else
//    unsigned char reserve2[15];
#endif
    
#ifdef GDW_ZHEJIANG
    unsigned char white_list:1;          //白名单 0:使能
    unsigned char black_list:1;          //黑名单 0:使能
	unsigned char isReport:1;			  // 是否上报过
    unsigned char reserve:5;
#endif
    
    //下面字段，后续要删除
    //unsigned char  relay_level;          // [1..MAX_RELAY_DEEP]
    unsigned short coll_sn;              //本表指向的采集器序号
										//0xffff代表未知相线信息

}RELAY_INFO, *P_RELAY_INFO;

#ifdef GDW_ZHEJIANG
typedef struct
{
	unsigned char used;
	unsigned char meter_addr[CCTT_METER_ADDR_LEN];
}BLACK_RELAY_INFO,*P_BLACK_RELAY_INFO;
#endif //GDW_ZHEJIANG

typedef struct sta_run_status_
{
	s32 runtime;//此处存储的与协议不同 存储的是获得运行时间后与当前cco运行起始时刻的差值 单位为s
	u8 zc;
	u8 u485;
	u8 leave;
	u8 reset;
}sta_run_status_s;
#define HRF_BIT_MAP_SIZE 16
//数组下标为分配的TEI，第0个不用
typedef struct
{
    unsigned char mac[MAC_ADDR_LEN];            //对应表地址

    unsigned short used:2;
//    unsigned long roles:3;           //节点角色 NODE_ROLES: ROLES_UNKNOWN, ROLES_STA, ROLES_PCO, ROLES_CCO
    unsigned short net_state:2;       //已入网、离线、未入网  NET_OUT_NET NET_IN_NET NET_OFFLINE_NET
    unsigned short child:11;
    u16           pco;              //此处修改一下  由于PCO是经常需要访问的因此PCO直接提取出来作为一个单独的u16来访问
    u16           brother:12;
    u16           Link:1;           //入网路径
    u16           moduleType:3;
    VERSION_DATE  year_mon_data;
    unsigned char dev_type:3;        //1 抄控器	 2 集中器本地通信单元  3 电表通信单元	 4 中继器  5 II型采集器  6 I型采集器单元  7 三相电表通信单元
    unsigned char re_join:1;         //是否是重新入网
	unsigned char geted_id:1;
    unsigned char phase_edge:1;      //获取到的sta边沿信息
    unsigned char phase_edge_ask:1;  //是否得到边沿信息(用于陕西协议)
    unsigned char phase_edge_vaile:1;//过零采集时对应cco的边沿信息
    unsigned char snr;
    unsigned char phy_phase;           //物理相线     bit [0:1]第一出线相位 bit[2:3]第二出线相位 bit [4:5]第三出线相位 bit [6]线路异常标志 [7]代表三相电表
    unsigned char receive_phase:3;       //接收相线     HPLC_PHASE, PHASE_UNKNOW, PHASE_A, PHASE_B, PHASE_C
    unsigned char hplc_phase:3;          //通信相线     HPLC_PHASE, PHASE_UNKNOW, PHASE_A, PHASE_B, PHASE_C
    unsigned char V2_7_get_edge:1;
    unsigned char mac_join:1;           //存储是否为自己的mac的入网信息

    unsigned char bootloader:7;
	unsigned char geted_phase:1;	 //已获取到相线信息
#ifdef GDW_PEROID_ID_MESSAGE_SET_CAPTURE
    unsigned char received_id;
#endif
    /*hrf*/ 

    s8 RfSnr;               //hrf SNR值
	s8 RfRssi;              // hrf RSSI
	u8 HrfSeq;              //无线节点发现列表 序号
	u8 HrfUpRate;           //hrf上行成功率 （需老化
	u8 HrfNoCnt:7;            //hrf 未更新上行成功率的cnt
    u8 IsHaveHrf:1;         //是否有HRF节点
    u16 ChildNum;         //模块直连子节点个数  为保证空间数目 此处最大保存128个节点 超过128的一律使用128
    u16 HrfBitmap;          //hrf统计成功率的bitmap

    /*hrf*/
    
    unsigned char software_v[2];        //模块软件版本信息
	unsigned char inner_v[2];           //模块内部版本号
	
    //芯片 ID 信息
    unsigned char dev_class;
    unsigned char factory_code[2];
    unsigned char chip_code[2];
    unsigned char dev_seq[5];
    unsigned char check_sum[8];
#ifndef HPLC_CSG
	unsigned char module_id[11];
	unsigned char chip_id[24];
#else
	unsigned char module_id[6];
#endif
    unsigned char ver_factory_code[2];    //版本信息中厂商代码
    unsigned char ver_chip_code[2];       //版本信息中芯片代码

#ifdef HPLC_CSG
    sta_run_status_s run_status;
#endif
    //unsigned char neighbor[TEI_CNT/8+1];   //邻居节点, 字节0的bit0不用，字节0的bit1是TEI1，即集中器。如果CCTT_METER_NUMBER%8>0 则，需更改为 CCTT_METER_NUMBER/8+1
#if defined(GDW_2019_PERCEPTION) || defined(NW_GUANGDONG_NEW_LOCAL_INTERFACE)
    u32           offline_tick;//(s)
#endif
	unsigned char  ReaderSec;   //抄表的时间每次更新
#ifdef GDW_ZHEJIANG
	u32				MeterNumConnect;//采集器下挂从节点数量
#endif
#ifdef HPLC_CSG //资产信息
    unsigned char  FactoryCode[2];
    unsigned char  ChipCode[2];
    unsigned char  ModuleSoftVer[2];
	VERSION_DATE   ModuleSoftVerDate;
	unsigned char  BootVer;
	unsigned char  ModuleHardVer[2];
	VERSION_DATE   ModuleHardVerDate;
	unsigned char  ChipSoftVer[2];
	VERSION_DATE   ChipSoftVerDate;
	unsigned char  ChipHardVer[2];
	VERSION_DATE   ChipHardVerDate;
	unsigned char  AppVer[2];
    
#ifndef ASSETSCODE_32BYTES_INFORM
	unsigned char  assetsCode[24];
#else
    unsigned char  assetsCode[32];
#endif
#endif
#ifdef GDW_JIANGSU 
	unsigned char  minute_coll;
#endif	
}STA_INFO, *P_STA_INFO;
#define MAX_READ_TIME         0xfe
#define CONTINUE_SEND_MAX     5
#define SEND_BEACON_PERIOD    15
typedef struct
{
    OS_TICK_64    tick_innet;                       //入网时间

	unsigned char found_list_counter_this_route;    //CCO本路由周期接收到该站点的发现报文计数
    unsigned char found_list_count_last_route;      //CCO上个路由周期接收到该站点的发现报文个数

	unsigned char sendPeriod;                       //每个信标周期倒计时当该值等于CONTINUE_SEND_MAX代表本周期已经准备好发送，若小于CONTINUE_SEND_MAX代表上个周期已经发送本周期还需要继续发送
//    unsigned short 会复位
    unsigned long firstJoinIn:2;                    //刚入网的周期数
    unsigned long :3;
    unsigned long silence_sec:12;           //静默计时，秒

    //主动注册
    unsigned long selfreg_read_node:1;      //0未读节点，1已读节点
    unsigned long selfreg_search_node:1;    //0搜表未完成，1搜表已完成
    unsigned long selfreg_send_offline:1;   //0未发离线指示，1已发离线指示

    //过零
    unsigned long zc_read_node_area:1;        //0未读节点，1已读节点 用于台区识别相线
    unsigned long zc_read_node:1;           //0未读节点，1已读节点 用于过零识别相线
    
    unsigned long res:1;                    //STA内部版本比对结果，用于升级判断
    unsigned long assoc_gather_ind:1;       //1需要发送关联汇总指示
    unsigned long need_up:1;
	unsigned long first_unkonw_phase:1;      //sta节点第一次是否获得相线信息
	unsigned long get_area_phase:1;          //sta节点是否支持2.7
//    unsigned long try_update_counter:3;
	unsigned long need_sync_cfg:2;               //需要曲线数据采集方案同步，1-645-2007,2-698
	unsigned long sync_cfg_result:1;             

	unsigned long check_in_net:1;          //确定真正入网标志
    unsigned long updateGetStart:1;        //已经确定收到开始升级信息   

#if defined(HPLC_CSG)
    unsigned char x4_sync_complete:1;
    unsigned char leaveTime:7;
	unsigned short changePcoTime;
#else

#endif

#ifdef TICK_ASSOC
    OS_TICK_64    tick_assoc;                       //关联请求通过的时间，某些场景下用做“入网”时间，关联时刷新
#endif

}STA_STATUS_INFO, *P_STA_STATUS_INFO;

//主动注册采集器下电表所属采集器映射关系，AFN06 F4上报采集器从节点信息及设备类型时使用
typedef struct
{
    unsigned char  meter_addr[6];  //电表地址
    unsigned char  protocol:2;     //电表规约
    unsigned char  meter_num:6;    //采集器下挂电表数量
    unsigned short coll_sn:15;     //电表指向的采集器序号
    unsigned short report_flag:1;  //该电表是否已经上报过
}SELF_REG_COLL_REPORT_MAP, *P_SELF_REG_COLL_REPORT_MAP;

#ifdef PROTOCOL_GD_2016

//从节点映射表
typedef struct
{
    unsigned char coll_addr[6];
    unsigned char meter_addr[12];
}METER_COLL_MAP, *P_METER_COLL_MAP;

//采集任务
typedef struct
{
    u8  task_id;

    u16 reserve1:8;
    u16 protocol:3;
    u16 meter_type:2;
    u16 reserve2:3;

    u16 timeout_sec;
    u16 coll_base_time;      //采样基准时间
    u8  coll_period;         //采样周期
    u8  di_count;            //数据标识个数 n

    u8  di_data[CCTT_X4_READ_TASK_DI_NUM*6+2];      //4个数据标识 + 校验

}X4_READ_TASK_INFO, *P_X4_READ_TASK_INFO;

typedef struct
{
    u8 Addr[6];
    u8 ID;
    u16 Min:6;
    u16 Hour:5;
    u16 Day:5;
    u8  num;
}X4_CLL_Task_INFO;

typedef struct
{
    u8  status:2;                   //值为  CCO_USERD 有效
    u8  reserve:6;                  //
    X4_READ_TASK_INFO  task;        //任务数据
}X4_READ_TASK_RECORD, *P_X4_READ_TASK_RECORD;

typedef struct
{
    u8  status:2;                   //值为  CCO_USERD 有效
    u8  reserve:6;                  //
    X4_CLL_Task_INFO  task;        //任务数据
}X4_CLL_Task_C;

#endif

#pragma pack(pop)


/*
typedef struct _cctt_meter_relay_
{		  
    unsigned char relay_level;
    unsigned short relay_meter[MAX_RELAY_DEEP];
}RELAY_ADDR, *P_RELAY_ADDR;
*/
typedef struct 
{
    unsigned long PosActEnergyDisp;  //正向有功总电量示值
    unsigned long BreakPowerWarningOn;  //断电告警
    unsigned long BreakPowerWarningOff;  //断电告警解除
    unsigned long BreakPowerActOn;   //断电动作
    unsigned long BreakPowerActOff;  //通电动作
    unsigned long MeterRunStatusWord;  //电表运行状态字
}DL645_DI;

//需离线指示的节点
typedef struct 
{
    unsigned char mac[MAC_ADDR_LEN];    //主机字节顺序
    unsigned char reason:3;             //离线原因      OFFLINE_REASON:  OFFLINE_NOTIFY, OFFLINE_OVER_LEVEL, OFFLINE_NOT_IN_WHITE_LIST
    unsigned char indflag:1;            //1需要发送指示，0不需要
    unsigned char reserve:4;
}LEAVE_IND_STA_INFO, *P_LEAVE_IND_STA_INFO;

//两个bit表示一只表
//#define SET_NEED_OFLINE(node_sn, reason)     g_pRelayStatus->need_offline_nodes[node_sn>>2] |= (reason<<((node_sn&0x03)<<1))
//#define GET_NEED_OFLINE(node_sn)            (g_pRelayStatus->need_offline_nodes[node_sn>>2] >> ((node_sn&0x03)<<1) & 0x03)
//#define CLEAR_NEED_OFFLINE(node_sn)          g_pRelayStatus->need_offline_nodes[node_sn>>2] &= ~(0x03<<((node_sn&0x03)<<1))

typedef struct _relay_group_info_
{
    unsigned short meter_num;
    unsigned short meter_sn[MAX_METER_NUM_7_GROUP];
}GROUP_INFO, *P_GROUP_INFO;

typedef struct _relay_addr_info_
{
    unsigned char relay_ok_flag;
    unsigned char addr[6];
}RD_INFO, *P_RD_INFO;

typedef struct _relay_max_group_info_
{
    unsigned short meter_num;
    unsigned short meter_sn[CCTT_METER_NUMBER];
}MAX_GROUP_INFO, *P_MAX_GROUP_INFO;

typedef struct _plc_ccb_info_
{
    unsigned char plc_status;

    /* group task
    */
    unsigned char group_stage;    
    unsigned char group_now_sn;
    unsigned char group_sn;
    unsigned short group_work_sn;
    

    /* freeze task VAR
    */
    unsigned char freeze_stage;
    unsigned char  freeze_cmp_flag;
    unsigned char freeze_old_phase;
    unsigned char freeze_meter_relation;    
    unsigned char freeze_search_level;    
    unsigned char freeze_group_sn;
    unsigned short freeze_sel_sn;
    unsigned short freeze_search_num;
    USHORT freeze_loop;    
    unsigned short freeze_work_sn;
    unsigned short freeze_root_level;
    unsigned short freeze_tree[AutoRelay_Depth_butt];  
    unsigned short freeze_tree_sn;    
    unsigned short freeze_ok_sn;

    unsigned char plc_run;

    unsigned char  search_stage; 
    unsigned char  search_group_sn;
    unsigned char  search_level;
    unsigned short search_loops;    
    unsigned short search_sel_sn;
    unsigned short search_ok_num;
    unsigned short search_relay_sn;

	OS_TMR    *      plc_save_timer;
	OS_TMR    *      plc_check_timer;
#if 0
    OS_EVENT  *   group_mbox;
    OS_EVENT  *   search_mbox;
    OS_EVENT  *   freeze_mbox;
	//OS_EVENT  *   post_frame_sem;
#endif
        
    GROUP_INFO    group_Info;
    GROUP_INFO    freeze_group_Info;
    GROUP_INFO    freeze_group_tree;
    GROUP_INFO    freeze_ok_group;
    MAX_GROUP_INFO search_unknown_group;
    MAX_GROUP_INFO search_relay_group;    
    MSG_SHORT_INFO  plc_send_msg_bak;
	
    
}PLC_CCB, *P_PLC_CCB;

typedef struct _plc_ccb_info2_	//发送给上位机的调试信息
{
unsigned char plc_status;

    unsigned char group_stage;    
    unsigned char group_now_sn;
    unsigned char group_sn;
    unsigned short group_work_sn;
    
    unsigned char freeze_stage;
    unsigned char  freeze_cmp_flag;
    unsigned char freeze_old_phase;
    unsigned char freeze_meter_relation;    
    unsigned char freeze_search_level;    
    unsigned char freeze_group_sn;
    unsigned short freeze_sel_sn;
    unsigned short freeze_search_num;
    USHORT freeze_loop;    
    unsigned short freeze_work_sn;
    unsigned short freeze_root_level;
    unsigned short freeze_tree[AutoRelay_Depth_butt];  
    unsigned short freeze_tree_sn;    
    unsigned short freeze_ok_sn;

    unsigned char plc_run;

    unsigned char  search_stage; 
    unsigned char  search_group_sn;
    unsigned char  search_level;
    unsigned short search_loops;    
    unsigned short search_sel_sn;
    unsigned short search_ok_num;
    unsigned short search_relay_sn;

	OS_TMR    *      plc_save_timer;
	OS_TMR    *      plc_check_timer;
#if 0
    OS_EVENT  *   group_mbox;    
    OS_EVENT  *    search_mbox;
    OS_EVENT  *   freeze_mbox;        
#endif
        
    USHORT      group_Info_meter_num;
    USHORT      freeze_group_Info_num;
    USHORT      freeze_group_tree_num;
    USHORT      freeze_ok_group_num;
    USHORT      search_unknown_group_num;
    USHORT      search_relay_group_num;    
    UCHAR      plc_send_msg[30];
	    
}PLC_CCB2, *P_PLC_CCB2;

#pragma pack(push,1)

typedef struct _gd_plc_rsp_
{
	unsigned char protocolPLCHead;		//0xAF 	

	unsigned char ADD1[3];			
	unsigned char controlCode;	
	unsigned char dataLen;
	
	//unsigned short DI0DI1;
	//unsigned char ADD2[3];			
	//unsigned char State;

	unsigned char dataBuf[1];
}GD_PLC_RSP, *P_GD_PLC_RSP;

// DL645 规约简化协议体
typedef struct _gd_reduced_dl645_body_
{
	//unsigned char protocolHead;   
	unsigned char meter_number[6];			
	unsigned char protocolHead2;  
	unsigned char controlCode;
	
	unsigned char dataLen;
	unsigned char dataBuf[1];
}GD_REDUCED_DL645_BODY, *P_GD_REDUCED_DL645_BODY;

// DL645 规约简化协议体
typedef struct _dl645_resp_info_
{
	unsigned char meter_number[6];	
    unsigned char protocolHead2;
	unsigned char controlCode;	
	unsigned char dataLen;
	unsigned char dataBuf[DL645_RESP_MAX_LEN];
}DL645_RESP, *P_DL645_RESP;

typedef union 
{
	UCHAR	ucType;
	
	struct 
	{
		UCHAR RELAY:		3; 
		UCHAR CR:			1;
		
		UCHAR SA:			1;
		UCHAR IFC:			1;
		UCHAR FRAMKIND:	2;

	} sepType;
	
}PLC_SFRAME_t;


typedef struct _harbin_plc_rsp_
{
	// unsigned char protocolPLCHead0;		//0x09 	~  不返回!! 
	unsigned char protocolPLCHead1;		//0xAF 	

	PLC_SFRAME_t frameFormat;	/* 黑龙江版PLC 帧格式 */
	unsigned char frameLen;		/* 帧长度域 */	

	// 
	union 
	{
		struct 
		{
			unsigned char concentor_addr[6];	/* 携带源地址 */			
			GD_REDUCED_DL645_BODY puredata;
		} plc_data_no_src;

		struct 
		{			
			GD_REDUCED_DL645_BODY puredata;
		} plc_data_src;
		
	}plc_block;

}HARBIN_PLC_RSP, *P_HARBIN_PLC_RSP;

#pragma pack(pop)

//上报类型
#define REPORT_TYPE_DIR         0
#define REPORT_TYPE_ROUTER      1

//搜表命令
#define CMD_SELF_REG    1

//启动搜表标志
#define SELF_REG_START      0
#define SELF_REG_CONFIRM    1

#define RELAY_REG_METER_MAX 16

#pragma pack(push,1)

typedef union _RELAY_SELFREG_CTRL_
{
    struct _down
    {
        unsigned char seqNum:4;
        unsigned char reserved1:1;
        unsigned char selfRegFlag:1;
        unsigned char reserved2:1;
        unsigned char cmdType:1;
    }down;
    struct _up
    {
        unsigned char seqNum:4;
        unsigned char reportType:1;
        unsigned char reserved:2;
        unsigned char cmdType:1;
    } up;
}RELAY_SELFREG_CTRL, *P_RELAY_SELFREG_CTRL;

typedef struct _RELAY_SELFREG_FRAME_
{
    unsigned char meter_number[6];
    RELAY_SELFREG_CTRL ctrl;
    unsigned char data[64];
}RELAY_SELFREG_FRAME, *P_RELAY_SELFREG_FRAME;

#pragma pack(pop)

//#define INIT_MAC    0x0FEF
#define INIT_MAC    0x0555  //通道板默认

//#define LEVEL(metersn)       g_MeterRelayInfo[metersn].relay_level
//#define LEVEL(metersn)       0

//最高3bit是初始,次高3bit是当前,最低2bit是差值
#define MAKE_LEVER(downlevel)   ((downlevel<<5&0xe0)|(downlevel<<2&0x1c))
#define PARSE_LEVER(uplevel)    ((uplevel>>5&0x7)-(uplevel>>2&0x7))

//命令
#define CMD_SELF_REG_START_YL    0
#define CMD_SELF_REG_YL          1
#define CMD_READ_METER_YL        2
#define CMD_SET_PARAM_YL         7

#define DIR_DOWN_YL              0
#define DIR_UP_YL                1

#pragma pack(push,1)

typedef struct _RELAY_PAYLOAD_CTRL_
{
    unsigned char cmd:3;
    unsigned char rev:2;
    unsigned char flo:1;
    unsigned char err:1;
    unsigned char dir:1;
}RELAY_PAYLOAD_CTRL, *P_RELAY_PAYLOAD_CTRL;

typedef struct _RELAY_PAYLOAD_FRAME_
{
    RELAY_PAYLOAD_CTRL ctrl;
    unsigned char len;
    unsigned char data[64];
}RELAY_PAYLOAD_FRAME, *P_RELAY_PAYLOAD_FRAME;

typedef struct _MF_RELAY_METER_ITEM_
{
    unsigned char meterPhase;   //相位, AC_PHASE_U, AC_PHASE_A, AC_PHASE_B, AC_PHASE_C
    unsigned char meterProto:3;    //1:97 2:07 3:采集器 4:上报采集器下面挂的表
    unsigned char is_bridge:1;
    unsigned char reserse:2;
    unsigned char meterAddr[6];
    unsigned short meterSn;

}MF_RELAY_METER_ITEM, *P_MF_RELAY_METER_ITEM;

typedef struct _MF_RELAY_REG_METER_INFO_
{
    unsigned char meterCount;
    MF_RELAY_METER_ITEM Items[RELAY_REG_METER_MAX];

}MF_RELAY_REG_METER_INFO, *P_MF_RELAY_REG_METER_INFO;

//请求抄读时保存的数据结构
typedef struct
{
    u8 read_flag;
    u8 len;       //后面645报文的长度
    u8 data[1];
}PLC_READ_ITEM_RSP_GDW2009, *P_PLC_READ_ITEM_RSP_GDW2009;

typedef struct
{
    u8 read_flag;
    u8 time_related;
    u8 len;       //后面645报文的长度
    u8 data[1];
}PLC_READ_ITEM_RSP_GDW2012, *P_PLC_READ_ITEM_RSP_GDW2012;

typedef struct
{
    u8 len;       //后面645报文的长度
    u8 data[0];
}PLC_READ_ITEM_RSP_FIXTIME_GDW2012, *P_PLC_READ_ITEM_RSP_FIXTIME_GDW2012;

typedef struct
{
    u8  plm_phase;       //
    u8  node_addr[FRM_DL645_ADDRESS_LEN];
    u16 user_sn;
}PLM_READ_ITEM_CMD, *P_PLM_READ_ITEM_CMD;

typedef struct
{
    u8 node_addr[FRM_DL645_ADDRESS_LEN];
    //从节点附属节点数量 n
    //从节点附属节点 1..n 地址
    u8 read_flag;
    u8 time_related;//0xff代表09协议，若非09协议0代表无延时校正1代表需要校正
    u8 len;       //后面645报文的长度
    u8 data[1];
}PLM_READ_ITEM_RSP, *P_PLM_READ_ITEM_RSP;

typedef struct
{
    u8 node_addr[FRM_DL645_ADDRESS_LEN];
	u16 delay_seconds;//预计延时时间
    u8 len;       //后面645报文的长度
    u8 data[0];
}PLM_READ_ITEM_FIX_TIME_RSP, *P_PLM_READ_ITEM_FIX_TIME_RSP;

#pragma pack(pop)

typedef struct
{   
    unsigned short msg_len;
}PLC_RSP_MSG_HEAD, *P_PLC_RSP_MSG_HEAD;

typedef struct
{
    PLC_RSP_MSG_HEAD header;
    unsigned char body[1];            //GD_DL645_BODY DL645;      DL69845_BODY DL69845;     透传
}PLC_RSP_MSG_INFO, *P_PLC_RSP_MSG_INFO;

//抄表状态
typedef enum
{
    EM_ROUTE_INIT,
    EM_ROUTE_NETTING,       //组网
    EM_ROUTE_DIR_READ,      //点抄
    EM_ROUTE_ROUTE_READ,    //路由抄表
    EM_ROUTE_BROAD_CAST,    //广播校时
    EM_ROUTE_ZC,            //过零采集
    EM_ROUTE_SELFREG,       //主动注册
	EM_ROUTE_GET_COLL_SN,   //获取二采的序列号
	EM_ROUTE_UPDATE,        //升级
    EM_ROUTE_AREA_DISCERNMENT, //台区识别
	EM_ROUTE_AREA_PHASE,//使用台区识别来得到相线相关信息
	EM_ROUTE_STA_AUTHEN, //设置STA认证信息
#ifdef GDW_ZHEJIANG
	EM_ROUTE_ONLINE_TIME_LOCK,//在网锁定时间
	EM_ROUTE_COLLECT_EVENT_REPORT,//即装即采
#endif

    EM_ROUTE_STORE_DATA_BCAST_TIMING, //曲线存储数据广播校时
    EM_ROUTE_STORE_DATA_SYNC_CFG,     //曲线存储数据采集方案同步
	EM_ROUTE_GET_MODULE_ID,          //获取模块ID
	EM_ROUTE_GET_MODULE_ID_REALTIME,  //实时获取模块ID
	EM_ROUTE_GET_STA_EDGE,            //陕西读表信息和表采集边沿
	EM_MULTI_READ_TASK,              //南网多播抄表
    EM_ROUTE_EVENT_SWITCH,              //事件开关
	EM_ROUTE_PHASE_ERR,              //零线电流异常
#if defined(HPLC_CSG)
	EM_ROUTE_READ_STA_STATUS,        //采集节点状态信息
	EM_ROUTE_READ_STA_CHANNEL,
	EM_ROUTE_READ_STA_NEW_CHANNEL,
	EM_ROUTE_READ_STA_INFO,
	EM_ROUTE_READ_STA_INFO_REALTIME,
    //南网X4
    EM_ROUTE_READ_TASK_INIT,         //初始化采集任务
    EM_ROUTE_X4_READ_TASK_SYNC,      //添加采集任务
#if 0
    EM_ROUTE_READ_TASK_INIT,         //初始化采集任务
    EM_ROUTE_ADD_READ_TASK,          //添加采集任务
    EM_ROUTE_DELETE_READ_TASK,       //删除采集任务
    EM_ROUTE_READ_TASK_ID,           //查询采集任务号
    EM_ROUTE_READ_TASK_DETAIL,       //查询采集任务详细信息
    EM_ROUTE_READ_TASK_DATA,         //采集任务数据
    EM_ROUTE_TIMING,                 //校时命令
#endif

#endif

}EM_READ_STATE;

//点抄状态
typedef enum
{
    //点抄
    EM_DIR_READ_INIT,
	EM_DIR_READ_TIME_FIX_REQ,
	EM_DIR_READ_TIME_FIX_WAIT,
    EM_DIR_READ_SEND,
    EM_DIR_READ_WAIT_PLC,
    EM_DIR_READ_POST,
    EM_DIR_READ_WAIT_POST,    
    EM_DIR_READ_POSTED,

}EM_DIR_READ_STATE;

//任务抄读状态
typedef enum
{
    EM_TASK_READ_INIT,
    EM_TASK_READ_SEND,
    EM_TASK_READ_WAIT_PLC,     //等待PLC应答
    EM_TASK_READ_POST,
    EM_TASK_READ_WAIT_POST,     //等待上报数据应答
    EM_TASK_READ_POSTED,
    EM_TASK_READ_EXIT,
    
    EM_TASK_READ_FIND_STA,  //多播时查找sta
}EM_TASK_READ_STATE;

//路由抄表状态
typedef enum
{
    EM_ROUTE_READ_INIT,
    EM_ROUTE_READ_REQUEST,
    EM_ROUTE_READ_WAIT_PLM,     //等待请求应答
	EM_ROUTE_READ_FIXTIME_REQUEST, //请求修正路由时间
	EM_ROUTE_READ_FIXTIME_WAIT_PLM,
	EM_ROUTE_READ_SEND,
    EM_ROUTE_READ_WAIT_PLC,     //等待PLC应答
    EM_ROUTE_READ_POST,
    EM_ROUTE_READ_WAIT_POST,     //等待上报数据应答
    EM_ROUTE_READ_POSTED,
    EM_ROUTE_WAIT               //直接等待
}EM_ROUTE_READ_STATE;


//升级状态
typedef enum
{
    EM_STA_UPDATE_INIT,
    EM_STA_UPDATE_REQ_INFO,
    EM_STA_UPDATE_WAIT_REQ_INFO,
    EM_STA_UPDATE_START,
    EM_STA_UPDATE_WAIT_START,
    EM_STA_UPDATE_STOP,
    EM_STA_UPDATE_WAIT_STOP,
    EM_STA_UPDATE_DATA,
    EM_STA_UPDATE_WAIT_DATA,
    EM_STA_UPDATE_DATA_LOCAL_BROADCAST,
    EM_STA_UPDATE_WAIT_DATA_LOCAL_BROADCAST,
    EM_STA_UPDATE_REQ_STATUS,
    EM_STA_UPDATE_WAIT_REQ_STATUS,
    EM_STA_UPDATE_EXE,
    EM_STA_UPDATE_WAIT_EXE,
}EM_STA_UPDATE_STATE;




//台区识别状态
typedef enum
{
    EM_AREA_DISCERNMENT_INIT,
    EM_AREA_DISCERNMENT_START,
    EM_AREA_DISCERNMENT_SEND_START,         //发送台区识别开始
    EM_AREA_DISCERNMENT_SEND_FEATURE,       //发送特征告知
    EM_AREA_DISCERNMENT_WAIT_REPORT,        //等待判别告知
	EM_AREA_DISCERNMENT_ASK,                //问询结果
	EM_AREA_DISCERNMENT_ASK_WAIT,           //等待问询结果
}EM_AREA_DISCERNMENT_STATE;

typedef enum{
	PLAN_INIT,
	PLAN_SEND,
	PLAN_WAIT,
	PLAN_DONE,
	PLAN_RUNNING,
	PLAN_ERROR,
}PLAN_STATUS;

#ifdef GDW_ZHEJIANG
//在网锁定时间广播状态
typedef enum
{
	EM_ONLINE_LOCK_TIME_TIME_INIT,
	EM_ONLINE_LOCK_TIME_TIME_START,
	EM_ONLINE_LOCK_TIME_TIME_WAIT
}EM_ONLINE_LOCK_TIME_TIME_STATE;
	
//即装即采上报集中器状态
typedef enum
{
	EM_EVENT_COLLECT_REPORT_TIME_INIT,
	EM_EVENT_COLLECT_REPORT_TIME_START,
}EM_EVENT_COLLECT_REPORT_TIME_STATE;
#endif

//存储曲线数据广播校时状态
typedef enum
{
	EM_STORE_DATA_BCAST_TIME_INIT,
	EM_STORE_DATA_BCAST_TIME_START,
	EM_STORE_DATA_BCAST_TIME_WAIT
}EM_STORE_DATA_BCAST_TIME_STATE;

//存储曲线数据采集方案同步状态
typedef enum
{
	EM_STORE_DATA_SYNC_CFG_INIT,
	EM_STORE_DATA_SYNC_CFG_START,
	EM_STORE_DATA_SYNC_CFG_SEND_UNICAST,
	EM_STORE_DATA_SYNC_CFG_WAIT_UNICAST,
	EM_STORE_DATA_SYNC_CFG_RESTART,
	EM_STORE_DATA_SYNC_CFG_WAIT,
}EM_STORE_DATA_SYNC_CFG_STATE;

#if defined(HPLC_CSG)
//任务初始化
typedef enum
{
    EM_STA_READ_TASK_INIT,
    EM_STA_READ_TASK_SEND,
}EM_STA_READ_TASK_INIT_STATE;

typedef enum
{
    EM_X4_READ_TASK_SYNC_INIT,

    EM_X4_READ_TASK_SYNC_READ_TASK_ID_SEND,
    EM_X4_READ_TASK_SYNC_READ_TASK_ID_WAIT_PLC,
    EM_X4_READ_TASK_SYNC_READ_TASK_ID_RECEIVE_PLC,
    
    EM_X4_READ_TASK_SYNC_READ_TASK_DETAIL_SEND,
    EM_X4_READ_TASK_SYNC_READ_TASK_DETAIL_WAIT_PLC,
    EM_X4_READ_TASK_SYNC_READ_TASK_DETAIL_RECEIVE_PLC,

    EM_X4_READ_TASK_SYNC_COMMPARE,
    
    EM_X4_READ_TASK_SYNC_DELETE_TASK_SEND,
    EM_X4_READ_TASK_SYNC_DELETE_TASK_WAIT_PLC,
    EM_X4_READ_TASK_SYNC_DELETE_TASK_RECEIVE_PLC,

    EM_X4_READ_TASK_SYNC_ADD_TASK_SEND,
    EM_X4_READ_TASK_SYNC_ADD_TASK_WAIT_PLC,
    EM_X4_READ_TASK_SYNC_ADD_TASK_RECEIVE_PLC,

    EM_X4_READ_TASK_SYNC_COMPLETE,
    EM_X4_READ_TASK_SYNC_NOT_SUPPORT,
    EM_X4_READ_TASK_SYNC_EXIT,

}EM_X4_READ_TASK_SYNC_STATE;

#endif

//点抄控制块
typedef struct
{
    EM_DIR_READ_STATE   state;

    //PLM dir read param
    P_READ_MSG_INFO     pPlmCmd;
	P_READ_MSG_INFO     pPlmCmdBak;
    P_PLC_RSP_MSG_INFO  pPlcRsp;

    //PLC param
    OS_TICK_64          tick_send;
    u8                  plc_send_counter;       //plc 发送递减计数
    u8                  protocol_plc;           //APP_METER_PRO

    //PLM post param
    OS_TICK_64          tick_post;
}PLC_DIR_READ_CB, *P_PLC_DIR_READ_CB;

#if defined(PROTOCOL_GD_2016)


//任务抄表控制块
typedef struct
{
    EM_TASK_READ_STATE  state;
    u8                  idx;
	
    struct
    {
        P_READ_TASK_INFO    pReadTask;
        OS_TICK_64          tick_next;
//    OS_TICK_64          tick_timeout;
        u8                  plc_send_counter;       //plc 发送轮数
        u8                  plc_send_num;           //plc 发送次数
        unsigned short      packet_seq_plc;         //plc 抄表数据包的序列号
//	unsigned short      packet_first_seq_plc;   //plc 本次抄表的第一个序列号
        P_PLC_RSP_MSG_INFO  pPlcRsp;
		u8                  bcast_sta; //使用广播抄读
		u8                  pkt_devtype; //负荷曲线抄读本地接口中表类型
        OS_TICK_64          first_read_tick;//第一次读取的tick
        //PLM post param
        // OS_TICK_64          tick_post;
    }array[MAX_NW_PARALLEL_NUM];
}PLC_TASK_READ_CB, *P_PLC_TASK_READ_CB;

typedef struct
{
    EM_TASK_READ_STATE  state;

    //PLC param
    P_READ_TASK_INFO    pReadTask;
    unsigned short      packet_seq_plc;         //plc 抄表数据包的序列号
    unsigned short      send_idx;               //多播时需要发送的tei
    unsigned short      total_cnt;              //多播时本次任务总共需要的sta数量
    unsigned short      get_cnt;                //已经获得的sta数量
	u8                  plc_send_counter; 
	//PLM post param
    OS_TICK_64          tick_post;
}PLC_TASK_MultiREAD_CB, *P_PLC_TASK_MultiREAD_CB;   

typedef struct
{
    EM_TASK_READ_STATE  state;
    u8                  report_sn;
    //PLC param
    unsigned char       mac_addr[MAC_ADDR_LEN];
	EM_TASK_TYPE        task_type;
	u32                 element;
	u8                  num;
	u16                 start_sn;
    OS_TICK_64          tick_send;
    OS_TICK_64          tick_timeout;
    u8                  plc_send_counter;       //plc 发送轮数
	u8                  plc_send_num;           //plc 发送次数
	unsigned short      packet_seq_plc;         //plc 抄表数据包的序列号
	u8                  element_array[0x10+1];  //信息元素队列，按本地接口查询顺序
	u8                  element_num;            //信息元素个数
    P_PLC_RSP_MSG_INFO  pPlcRsp;

    //PLM post param
    OS_TICK_64          tick_post;    
}PLC_MAINTENANCE_NET_CB, *P_PLC_MAINTENANCE_NET_CB;

#endif

//路由抄表控制块
typedef struct
{
    EM_ROUTE_READ_STATE state;
    u16                 sn;

    //PLM request param
    u8                  req_counter;            //请求递减计数
    OS_TICK_64          tick_last_req;
    PLM_READ_ITEM_CMD   plmCmd;
    P_PLM_READ_ITEM_RSP pPlmRsp;
//    P_PLM_READ_ITEM_RSP pPlmRspLast;

    //PLC param
    OS_TICK_64          tick_send;
    OS_TICK_64          tick_timeout;
    u8                  plc_send_counter;       //plc 发送递减计数
    u8                  protocol_plc;           //APP_METER_PRO
    unsigned short      packet_seq_plc;         //plc 抄表数据包的序列号
	//unsigned short      packet_first_seq_plc;         //plc 抄表数据包的序列号
	P_PLC_RSP_MSG_INFO  pPlcRsp;
    u8                  plc_read_success_counter;       //一只表连续抄读成功次数，用于切表，防止在一只表上挂死

    //PLM post param
    OS_TICK_64          tick_post;    
}PLC_ROUTE_READ_CB, *P_PLC_ROUTE_READ_CB;

//广播校时控制块
typedef struct
{
    P_MSG_INFO      pSendMsg;
    P_MSG_INFO      pSendMsgLower;//低优先任务
    P_MSG_INFO      pSendMsgHight;//高优先任务
    P_MSG_INFO      pSendMsgBak;//备用指针放止由于任务调度导致判断指针不准确的情况
    OS_TICK_64      tick_start;
    OS_TICK_64      tick_last_snd;
}PLC_BROADCAST_CB, *P_PLC_BROADCAST_CB;


typedef struct
{
    u8              status;
	u8              cnt;
	u16             seq;
	u8              sta_percent;
    OS_TICK_64      tick_start;
    OS_TICK_64      tick_last_snd;
}PLC_PHASE_ERR_CB, *P_PLC_PHASE_ERR_CB;

//主动上报控制块
typedef struct
{
    EM_SELFREG_STATE    state;
    u8                  start_stop_req;
    u16                 selfreg_time_last_min;
    
    OS_TICK_64          selfreg_start_tick;
    OS_TICK_64          selfreg_new_sta_tick;
    OS_TICK_64          selfreg_send_start_tick;
    OS_TICK_64          selfreg_read_meter_tick;
    OS_TICK_64          selfreg_send_offline_tick;
    OS_TICK_64          selfreg_send_stop_tick;
    
    u8                  selfreg_send_start_counter;
    u8                  selfreg_send_stop_counter;
    u8                  selfreg_send_read_all;

    u8                  selfreg_try_stop_counter;
    u8                  upBitmap[MAX_BITMAP_SIZE];
    u32                 packet_seq_self_reg;                //APP 搜表报文序列号
}PLC_SELFREG_CB, *P_PLC_SELFREG_CB;


//获取电表对应的二采序列号
typedef struct
{
    EM_GET_SN_STATE     state;    
	u16                 tei;
    OS_TICK_64          next_hour_tick;  //固定的小时tick
	OS_TICK_64          new_tei_tick;    //有新入网的节点tick
	OS_TICK_64          next_tick;
	u32                 bitmap[MAX_BITMAP_SIZE/4];
}PLC_GET_SN_CB, *P_PLC_GET_SN_CB;
//过零采集控制块
typedef struct
{
    OS_TICK_64 tick_last_read;
}PLC_ZC_CB, *P_PLC_ZC_CB;

//频率切换控制块
typedef struct
{
    u16         second_down;        //值大于0时，需要频段切换
    u16         new_frequence;
}PLC_CHANGE_FREQUENCE_CB, *P_PLC_CHANGE_FREQUENCE_CB;

#ifdef GDW_ZHEJIANG
//在网锁定时间控制块
typedef struct
{
	u8								FrameSN;
	u8								lockTimeFlag;//0:不锁网     1:锁网
	u8  							second_down;
	u16 							OnlineLockTime;
	u16 							abnormalOfflineLockTime;
	EM_ONLINE_LOCK_TIME_TIME_STATE 	state;
	u8 						        send_counter;
	OS_TICK_64                                              tick_send;
}PLC_ONLINE_LOCK_TIME_CB, *P_PLC_ONLINE_LOCK_TIME_CB;

//即装即采控制块
typedef struct
{
	EM_EVENT_COLLECT_REPORT_TIME_STATE 	state;
	u32			                updatecnt;
	OS_TICK_64                      	tick_send;
}PLC_EVENT_COLLECT_REPORT_CB, *P_PLC_EVENT_COLLECT_REPORT_CB;

#endif

//子节点升级控制块
typedef struct
{
    u8      update_flag;
    u8      need_update;
    u8      broad;
    u8      plan;
    u8      plc_send_counter;       //plc 发送递减计数
//    u8      read_sta_info;
    u8      get_current_status;     //获取到当前的sta 数据bitmap信息
    u8      fileType;               //仅南网使用来存储升级文件类型
    u16     update_round;           //升级轮数
    u16     retry;                  //单次发送时的重试次数
    u16     get_sta_status_frame;  //收到有效的sta升级状态个数
    u16     total_sta;             //目前剩余需要升级的sta个数
    u32     period;
    u32     flash_addr;
    u32     file_size;
    u32     file_checksum;

    EM_STA_UPDATE_STATE state;
    PLAN_STATUS         planStatus;
    u32     update_id;
    u32     block_id;       //当前发送的块id
    u16     update_minutes;
    u16     block_size;
    u16     block_count;
    u16     tei;            //当前升级的STA
    u8 *bitmap;         //传输文件成功的位图，大小为block_count/8+1字节

    OS_TICK_64  tick_send;
    OS_TICK_64  tick_timeout;
    OS_TICK_64  tick_start_update; //升级开始时间

    u32     file_size_sta;
    u32     file_checksum_sta;
    u32     staBitmap[MAX_BITMAP_SIZE/4];

#ifdef HPLC_CSG
	u32     old_file_size;
    u32     old_file_checksum;
    u16     old_block_count;
#endif	
}PLC_STA_UPDATE_CB, *P_PLC_STA_UPDATE_CB;

//台区识别控制块
typedef struct
{
    EM_AREA_DISCERNMENT_STATE state;

    u32                 start_ntb;

    u16                 packet_seq;                //报文序号

    OS_TICK_64          tick_last_send;
    u8                  send_counter;
    u8                  collection_seq;            //采集序列号, 整个网络第几次启动采集，CCO每启动一次采集累加一次
//    u8                  phase_or_area;            //当前正在进行的是相线识别或是台区识别0为未进行任何识别 1为相线识别
    u8                  allow;
    u16                 round;                    //发送了多少次告知事件
	u32                 getedZoneStatusBitMap[MAX_BITMAP_SIZE / 4];//是否获取到了sta台区信息的结果
    u32                 zoneErrorMap[MAX_BITMAP_SIZE / 4];//台区信息是否有误
}PLC_AREA_DISCERNMENT, *P_PLC_AREA_DISCERNMENT;
//台区识别控制块
typedef struct
{
    EM_AREA_DISCERNMENT_STATE state;

    u32                 start_ntb;

    u16                 ntb_collection_seq;        //台区特征信息采集序列号

    u16                 packet_seq;                //报文序号

    OS_TICK_64          tick_last_send;
    u8                  send_counter;
    
}PLC_AREA_PHASE, *P_PLC_AREA_PHASE;


//读取模块ID
typedef struct
{
	OS_TICK_64 tick_next;
	u32 state;
    u16 tei;
    u16 seq;
    u16 retry; //实时查询存ID类型
    u8  dev_type;
    u8  mac_addr[6];
    u8                  plc_send_counter;       //plc 发送轮数
	u8                  plc_send_num;           //plc 发送次数
    P_PLC_RSP_MSG_INFO  pPlcRsp;
    OS_TICK_64          tick_send;
    OS_TICK_64          tick_timeout;
    OS_TICK_64          tick_post;  //PLM post param
}PLC_GET_MOUDLE_ID_CB;


typedef struct
{
	OS_TICK_64 tick_next;
    u32 state;
    u16 tei;
    u16 seq;
    u16 retry;
}PLC_GET_STAEDGE_CB;

typedef struct
{
    OS_TICK_64 tick_next;
    u8 status;
    u8 time;
    u8 isEnable;
}PLC_SET_STA_AUTHEN_CB;

typedef struct
{
    OS_TICK_64 tick_next;
    u32 state;
    u32 bitmap[MAX_BITMAP_SIZE/4]; //bitmap[0] bit 0 代表广播 其他为单拨
    u16 remainTime;
    u16 seq;
    u16 tei;
}PLC_EVENT_SWITCH_CB;
//曲线数据存储广播校时控制块
typedef struct
{
    EM_STORE_DATA_BCAST_TIME_STATE   state;
    u8                               send_counter;
    OS_TICK_64                       tick_send;
	OS_TICK_64                       next_tick_send;
}PLC_STORE_DATA_BCAST_CB, *P_PLC_STORE_DATA_BCAST_CB;

//曲线存储数据项采集方案同步控制块
typedef struct
{
	EM_STORE_DATA_SYNC_CFG_STATE     state;
	OS_TICK_64                       tick_send;
	u8                               round;
	u8                               portocol;
#ifndef HPLC_CSG
	u8                               send_counter;
	u16                              tei;
	u8                               restart_immediate;
	u32                              packet_seq_total;
	u32                              packet_seq[2];
	u32                              sta_bitmap[2][32];
	u8                               meter_645_data_len;
	u8                               meter_698_data_len;
	u8                               meter_645_data[256];
	u8                               meter_698_data[256];
#else
#if 1
	u8                               send_counter;
	u16                              tei;
	u8                               restart_immediate;
	u32                              packet_seq_total;
	u32                              sta_bitmap[1][32];
	u8                               meter_645_data_len;
	u8                               meter_645_data[256];
#else
	u8                               meter_645_data[6][28];
	u8                               collect_cnt[6];
	u8                               idx;
#endif
#endif
}PLC_STORE_DATA_SYNC_CFG_CB, *P_PLC_STORE_DATA_SYNC_CFG_CB;

#if defined(HPLC_CSG)

//任务初始化控制块
typedef struct
{
    EM_STA_READ_TASK_INIT_STATE state;
    OS_TICK_64      tick_start;
    OS_TICK_64      tick_last_snd;
}PLC_STA_READ_TASK_INIT_CB, *P_STA_PLC_READ_TASK_INIT_CB;

//采集任务同步
typedef struct
{
    EM_X4_READ_TASK_SYNC_STATE state;

    u16                 tei_sync;

    u16                 app_frame_seq_task_id_read;
    u16                 app_frame_seq_task_info_read;
    u16                 app_frame_seq_task_delete;
    u16                 app_frame_seq_task_add;

    u8                  task_idx;       //task_id_read_array下标

    //读取回来的采集任务信息
    u8                  task_read_num;
    u8                  task_id_read_array[CCTT_X4_READ_TASK_NUM_PER_METER*3];
#ifdef PROTOCOL_GD_2016
    P_X4_READ_TASK_INFO task_info_read_array[CCTT_X4_READ_TASK_NUM_PER_METER*3];
#endif
    //需要删除的采集任务
    u8                  task_delete_num;
    u8                  task_id_delete_array[CCTT_X4_READ_TASK_NUM_PER_METER*3];

    //需要添加的采集任务
    u8                  task_add_num;
    u8                  task_id_add_array[CCTT_X4_READ_TASK_NUM_PER_METER];
#ifdef PROTOCOL_GD_2016
    P_X4_READ_TASK_INFO p_read_task_info;        //抄读回来的任务详细
#endif
    OS_TICK_64      tick_last_find;
    OS_TICK_64      tick_send;
    OS_TICK_64      tick_timeout;
    
}PLC_X4_READ_TASK_SYNC_CB, *P_PLC_X4_READ_TASK_SYNC_CB;

#endif

#pragma pack(push,1)

typedef struct
{
    BYTE comm_mode:4;           //通信方式
    BYTE is_router:1;           //路由管理方式
    BYTE tp_mode:1;             //测量点信息模式
    BYTE read_period_mode:2;    //周期抄表模式

    BYTE read_delay_enabled:3;      //传输延时参数支持
    BYTE meter_tab_mode:2;          //失败节点切换发起方式
    BYTE broadcast_confirm_mode:1;  //广播命令确认方式
    BYTE broadcast_exe_mode:2;      //广播命令信道执行方式

    BYTE chanel_count:5;            //信道数量
    BYTE powercut_ifo:3;            //低压电网掉电信息
    
    BYTE baudrate_count:4;          //速率数量 n
	BYTE minute_coll:1;          //是否支持分钟级采集
    BYTE reverse1:3;

    BYTE reverse2;
    BYTE reverse3;

}COMM_MODE, *P_COMM_MODE;

typedef struct
{
    BYTE comm_mode:4;           //通信方式
    BYTE reverse:4;
}COMM_MODE_GD, *P_COMM_MODE_GD;

typedef struct
{
    char factoryCode[2];
    char chipCode[2];
    BYTE day;
    BYTE month;
    BYTE year;
    BYTE version[2];
} LOCAL_MODEL_VERSION, *P_LOCAL_MODEL_VERSION;

typedef struct
{
	//char factoryCode[4];
	char factoryCode[2];        //16年6月的协议，去掉了两个字节
    char chipCode[2];
    DATA_FORMAT_20  softReleaseDate;
    BYTE version[2];
    
} LOCAL_MODEL_VERSION_GD, *P_LOCAL_MODEL_VERSION_GD;

typedef struct
{
    COMM_MODE   comm_mode;                 //本地通信模式字
    UCHAR       direct_read_timeout;       //从节点监控最大超时时间（单位 S）
    USHORT      broadcast_timeout;         //广播命令最大超时时间（单位 S） 
    USHORT      max_frame_len;             //最大支持的 376.2 报文长度 
    USHORT      max_file_package_len;      //文件传输支持的最大单包长度
    BYTE        update_waittime;           //升级操作等待时间
    BYTE        main_node_addr[6];

    USHORT      max_node_count;       //支持的最大从节点数量
    USHORT      now_node_count;       //当前从节点数量

    DATA_FORMAT_20 protocol_date_release;     //通信模块使用的 376.2 协议发布日期（BCD）
    DATA_FORMAT_20 protocol_date_bak;         //通信模块使用的 376.2 协议最后备案日期（BCD）

    LOCAL_MODEL_VERSION  router_version;      //通信模块厂商代码及版本信息
    USHORT         baudrate_ifo[1];           //大小由前面的通讯速率个数决定

}ROUTER_IFO, *P_ROUTER_IFO;

typedef struct
{
    COMM_MODE_GD            comm_mode;                 //本地通信模式字
    USHORT                  max_frame_len;             //最大支持的协议报文长度
    USHORT                  max_file_package_len;      //文件传输支持的最大单包长度
    BYTE                    update_waittime;           //升级操作等待时间
    BYTE                    main_node_addr[6];
    USHORT                  max_node_count;             //支持的最大从节点数量
    USHORT                  now_node_count;             //当前从节点数量
    USHORT                  max_node_per_frame;         //支持单次读写从节点信息的最大数量 
    DATA_FORMAT_20          protocol_date_release;     //通信模块使用的协议发布日期 
    LOCAL_MODEL_VERSION_GD  router_version;             //厂商代码和版本信息

}ROUTER_IFO_GD, *P_ROUTER_IFO_GD;
#pragma pack(pop)
#pragma pack(4)
//抄回保存到exram是数据格式
typedef struct {
    unsigned short meter_sn:15;
    unsigned short data_flag:1;     //1 打包抄数据, 0 非打包抄数据; 目前只实现打包的存储
    unsigned char data_len;
    unsigned char data[CCTT_READ_PACK_DATA_LEN];
}METER_DATA_STORAGE_BODY, *P_METER_DATA_STORAGE_BODY;

typedef struct {
    unsigned short              nBufferCount;
    P_METER_DATA_STORAGE_BODY   pBufferAddr;
}METER_DATA_STORAGE, *P_METER_DATA_STORAGE;

//经过解释存储到 exram 得到的数据
typedef struct {
    unsigned char data_len;
    unsigned char data[CCTT_READ_PACK_DATA_LEN];        //DL645_RESP_MAX_LEN
}METER_DATA_BODY, *P_METER_DATA_BODY;

typedef struct {
	//unsigned char meter_addr[6];
    unsigned short meter_sn;
    //METER_DATA_BODY meter_data[CCTT_READ_PACK_NUMBER];
    METER_DATA_BODY meter_data[CCTT_METER_DATA_NUMBER];      //解释后是DL645的数据区部分，不是打包后的数据
}METER_DATA, *P_METER_DATA;


typedef struct
{
	u16 startBeaconTei;
	u16 stopBeaconTei;
	u8 raminCnt;
	u8 teiCnt;
	u8 rountHrf;//hrf 的发现信标依附于hplc的发现信标中其中第一轮采集哪些sta发了发现信标第2-4轮分别在发送csma中发送发现信标的前1/3中1/3和后1/3的sta
	u8 res;
	u32 bitmap[MAX_BITMAP_SIZE/4];
	u32 bitmapHrf[MAX_BITMAP_SIZE/4];//hrf已经发送的bitmap
}beaconDescoverInfo_s;
#pragma pack(4)
typedef struct {
    u8  lid;
    u8  resend_count;

    //u16 PcoTei;

    u16 DstTei;
    u8  DstMac[MAC_ADDR_LEN];       //主机字节顺序
    u16 MsduLen;

    MSDU_TYPE                   MsduType;
    MAC_SEND_TYPE               SendType;
    MAC_BROADCAST_DIRECTION     Broadcast;
    MAC_FLAG_e                  MACFlag;
}SEND_CMD_MAC_PARAM, *P_SEND_CMD_MAC_PARAM;

typedef struct {
    u8  lid;
    u8  resend_count;

    u16 MsduSeq;

    u16 DstTei;
    u8  DstMac[MAC_ADDR_LEN];       //主机字节顺序
    u16 MsduLen;

    MSDU_TYPE                   MsduType;
    MAC_SEND_TYPE               SendType;
    MAC_BROADCAST_DIRECTION     Broadcast;
    MAC_FLAG_e                  MACFlag;
}SEND_RSP_MAC_PARAM, *P_SEND_RSP_MAC_PARAM;

#define MAX_OUT_LIST_METER 100
typedef struct
{
	u16 cnt;
	u8 mac[MAX_OUT_LIST_METER][6];
	u8 cco_num[MAX_OUT_LIST_METER]; //台区识别结果
}RemMeter_s;

typedef struct
{
	u32 is_vaild;
	u32 staBitmap[MAX_BITMAP_SIZE/4];
	u32 totalSta;//本次获取的sta的总数
	OS_TICK_64 last_tick;
}ReportTopoInfo;

typedef struct
{
    unsigned char   reboot_sec_down;
    unsigned char   save_nodes_sec_down;
#ifdef GDW_ZHEJIANG
	unsigned char   save_black_nodes_sec_down;
#endif //GDW_ZHEJIANG	
    unsigned char   save_OutListMeter_sec_down;
    unsigned char   save_sta_sec_down;
    unsigned char   sta_offline_sec_down;
    unsigned char   sta_change_netid_sec_down;
    unsigned char   plc_listen_sec_down;    //载波侦听
    unsigned char   powercut_ifo;           //低压电网掉电信息
    
	unsigned char   II_geted;        //是否有二采入网
    //路由状态
    unsigned char   pause_sec;       //路由暂停时间

    unsigned char   init_meter_delay;               //初始化表档案后请求集中器的延时时间
    OS_TICK_64      plc_start_time;

    //组网参数
    unsigned short  search_loop;                    //循环组网次数
    unsigned char   search_level;                   //组网层级 [1..15]
    unsigned char   search_found_beacon_complete;   //当前层次是否都发完了发送信标
    OS_TICK_64      search_start;                   //当前层次开始组网时间
    OS_TICK_64      search_level_last_assoc_req;    //当前组网级别最后一次关联请求时间，如果当前级别发完了两轮发现信标，并且超过30秒没有关联请求，则学习下一层次

    //路由信息
    unsigned short route_period_sec;         //路由周期
    unsigned short route_period_sec_down;    //路由周期剩余时间
    unsigned short route_period_count;       //路由周期计数
    
    unsigned short heartbeat_period_sec;     //心跳周期

    unsigned char  cco_foundlist_interval_sec;          //CCO发送发现列表报文的间隔
    unsigned char  cco_hrf_foundlist_interval_sec;          //CCO发送发现列表报文的间隔
//    unsigned short pco_interval_sec;                    //代理站点发送发现列表报文的间隔
//    unsigned short sta_interval_sec;                    //发现站点发送发现列表报文的间隔
    
    unsigned char  found_list_count_last_route;        //上个路由周期发送发现列表报文个数
    unsigned char  found_list_counter_this_route;      //本路由周期发送发现列表报文计数

    OS_TICK_64     tick_last_foundlist_send;        //上次发送发现列表报文时刻
    OS_TICK_64     tick_last_hrf_foundlist_send;        //上次发送发现列表报文时刻

    u16            packet_seq_read_meter;           //APP 抄表报文序列号

    LEAVE_IND_STA_INFO  leave_ind_infos[TEI_CNT];        //需要通知离线的节点
    OS_TICK_64          tick_assoc_req_level1;

    unsigned char s_freeze_flag[CCTT_METER_NUMBER/8+1];                  //是否冻结完成

    //抄表状态
    EM_READ_STATE   read_state;

    //点抄控制块
    PLC_DIR_READ_CB     dir_read_cb;

#if defined(PROTOCOL_GD_2016)
    //任务抄表控制块
    PLC_TASK_READ_CB    task_read_cb;
	PLC_TASK_MultiREAD_CB  task_multi_read_cb;
	PLC_MAINTENANCE_NET_CB sta_status_cb;
	PLC_MAINTENANCE_NET_CB sta_info_cb;
	PLC_MAINTENANCE_NET_CB sta_channel_cb;
    PLC_MAINTENANCE_NET_CB sta_new_channel_cb;
	PLC_MAINTENANCE_NET_CB sta_info_realtime_cb; //实时查询模块资产信息
#endif

    //路由主动抄表控制块
    PLC_ROUTE_READ_CB   route_read_cb;

    //广播校时控制块
    PLC_BROADCAST_CB broadcast_cb;

    //从节点主动注册控制块
    PLC_SELFREG_CB      selfreg_cb;


	//获取电表对应的二采序列号
	PLC_GET_SN_CB       get_sn_cb;
	//过零采集控制块
    PLC_ZC_CB           zc_cb;

    //频率切换控制块
    PLC_CHANGE_FREQUENCE_CB change_frequence_cb;

#ifdef GDW_ZHEJIANG
	//在网锁定时间控制块
	PLC_ONLINE_LOCK_TIME_CB online_lock_time_cb;

	//即装即采控制块
	PLC_EVENT_COLLECT_REPORT_CB event_collect_report_cb;
#endif

    //Hrf 频率切换
    PLC_CHANGE_FREQUENCE_CB change_hrf_channel_cb;

    //子节点升级控制块
    PLC_STA_UPDATE_CB   sta_update_cb;

    //台区识别控制块
    PLC_AREA_DISCERNMENT area_dis_cb;

	//台区识别相线控制块
	PLC_AREA_PHASE area_phase_cb;
	//读取模块ID
	PLC_GET_MOUDLE_ID_CB get_moudle_id_cb;
    PLC_GET_MOUDLE_ID_CB get_moudle_id_realtime_cb;

    //获取陕西模块信息控制块
    PLC_GET_STAEDGE_CB   get_phase_edge_cb;

	//存储曲线数据广播校时控制块
    PLC_STORE_DATA_BCAST_CB store_data_bcast_cb;

	//曲线存储数据项采集方案同步控制块
	PLC_STORE_DATA_SYNC_CFG_CB store_data_sync_cfg_cb;

	PLC_SET_STA_AUTHEN_CB set_sta_authen_cb;
	PLC_EVENT_SWITCH_CB event_switch_cb;
#ifdef ERROR_APHASE
	PLC_PHASE_ERR_CB   phase_err_cb;
#endif
#if defined(HPLC_CSG)
	//任务初始化控制块
    PLC_STA_READ_TASK_INIT_CB sta_read_task_init_cb;
    PLC_X4_READ_TASK_SYNC_CB x4_read_task_sync_cb;
#endif
	ReportTopoInfo get_topo_info;               //获取拓扑的暂存信息
    //节点统计信息
    unsigned short all_meter_num;               //白名单 + 上报节点
    unsigned short selfreg_meter_num;           //上报节点
    unsigned short cfged_meter_num;             //白名单节点数
	unsigned short freeze_num;                  //冻结完成

    //STA信息统计
	unsigned short innet_num;                   //入网数
	unsigned short online_num;                  //在线数，入网数-离线数
	unsigned short check_online;                //确认已入网状态
	unsigned short max_tei; 
	unsigned short pco_num;
    unsigned short no_cent_sta;
    unsigned short no_cent_cnt;
    unsigned short sta_num;
    unsigned short sta_num_not_level_1;                 //level >=2 的STA数量，直连表的level_1
    unsigned short innet_num_phase[4];                  //A,B,C,ALL的表计数目
    unsigned short innet_num_level[MAX_RELAY_DEEP+1];   //每个层次 [1..MAX_RELAY_DEEP] 的入网数目
    unsigned short sta_num_level[MAX_RELAY_DEEP+1];     //每个层次 [1..MAX_RELAY_DEEP] 的STA数目
    unsigned short assoc_gather_ind_num;        //需要发送汇总指示数目

	u8             connectReader;               //是否链接抄控器
	u8             ReaderLink;                  //抄控器连接的链路  0:hplc   1:hrf
	u8             seqReader;                   //抄控器连接序号
	OS_TICK_64     startTick;                   //链接开始时刻
	OS_TICK_64     lastTick;                    //接到抄控器app层最后时间
	u32            offline_time;                //离线次数
    u32            pco_change;                  //代理变更次数
    s32            recv_rssi;                   //接收rssi，0xE802F241或者F241
    s32            factory_recv_rssi;           //抄控器产测接收rssi
}RELAY_STATUS, *P_RELAY_STATUS;

typedef struct
{
    HPLC_PARAM  hplcParam;      //接收参数
    u8          macFrame[1];    //MAC帧
}RECEIVE_MAC, *P_RECEIVE_MAC;
    

typedef struct {
	u8 happend[MAX_BITMAP_SIZE];
	u8 uped[MAX_BITMAP_SIZE];
}PowerEventBitMap_s; //0 停电事件(g_pStaInfo) 1 上电事件(g_pStaInfo) 2 停电事件(g_pMeterRelayInfo 白名单) 3 上电事件(g_pMeterRelayInfo 白名单)
					 //0/1 用于存储模块map  2/3 用于存储采集器bitmap 所以保存的bitmap是白名单
#define  PLC_645_MINUS_0X33    ((UCHAR)-0x33)

////////////////////////////////////////////////////////////////////////////////////////
extern RELAY_INFO g_MeterRelayInfo[CCTT_METER_NUMBER];
#ifdef GDW_ZHEJIANG
extern BLACK_RELAY_INFO g_BlackMeterInfo[CCTT_BLACK_METER_NUMBER];
#endif //GDW_ZHEJIANG
#ifdef HPLC_GDW
extern unsigned short selfRegCollReportNum;
extern SELF_REG_COLL_REPORT_MAP g_SelfRegCollReportMap[CCTT_METER_NUMBER];
#endif

#ifdef PROTOCOL_GD_2016

extern  P_METER_COLL_MAP    g_pMeterCollMap;
extern  X4_READ_TASK_RECORD g_X4ReadTaskInfo[CCTT_X4_READ_TASK_INFO_NUMBER];
extern  X4_CLL_Task_C       g_X4_CLL_Task[CCTT_X4_CLL_TASK_NUM_MAX];
#endif
#ifdef GDW_2019_GRAPH_STOREDATA
//陕西全网感知要求
extern u32 goto_online_bitmap[MAX_BITMAP_SIZE/4];
extern u32 goto_offline_bitmap[MAX_BITMAP_SIZE/4];
extern u32 offline_reason[MAX_BITMAP_SIZE/4];
#endif

#ifdef GDW_JIANGXI
extern u8 max_beacon_period_ms;
#endif

extern u8 allow_save;
extern uint32_t ZC_Time[3];       //如果不接线，则对应的值是0
extern METER_DATA g_Meter_Data;


extern P_GLOBAL_CFG_PARA   g_pPlmConfigPara;
extern P_RELAY_STATUS      g_pRelayStatus;
extern P_RELAY_INFO        g_pMeterRelayInfo;
extern P_STA_INFO          g_pStaInfo;
extern P_STA_STATUS_INFO   g_pStaStatusInfo;
extern RemMeter_s          OutListMeter;



#define MF_RELAY_FREEZED(meterno)   0
#define MF_RELAY_CLR_FREEZED(meterno)
#define MF_RELAY_SET_FREEZED(meterno)
#define MF_RELAY_CLRALL_FREEZED()

#define MF_RELAY_FREEZE_DONE(meterno)  0
#define MF_RELAY_CLR_FREEZE_DONE(meterno)  0
#define MF_RELAY_SET_FREEZE_DONE(meterno)
#define MF_RELAY_CLRALL_FREEZE_DONE()

#define INLINE_STA_NUM_LEVEL0     8
#define INLINE_STA_NUM_LEVEL1     20
#define INLINE_STA_NUM_LEVEL2     80
#define INLINE_STA_NUM_LEVEL3     130
#define INLINE_STA_NUM_LEVEL4     230
#define INLINE_STA_NUM_LEVEL5     380
#define INLINE_STA_NUM_LEVEL6     600


//typedef unsigned char (*PLC_Get_Phase_Func)(unsigned short metersn);
unsigned char PLC_Get_Meter_Phase(unsigned short metersn);

unsigned char	 SYS_AUTOPLC_init(HANDLE h);

unsigned char	 SYS_AUTOPLC_proc(HANDLE h);

unsigned char    AUTOPLC_ontick(void* pTimerHandle, void* h);
unsigned char    AUTOPLC_update();
unsigned char    AUTOPLC_set_research_flag(unsigned short sn);

void             AUTOPLC_proc(pvoid msg);

UCHAR PLC_get_run_state();
void PLC_tenth_second_proc();
void PLC_relay_sys_init(unsigned short meter_type);
void PLC_relay_sorage_init();
bool PLC_find_new_node(UCHAR *addr);
void PLC_del_new_node(UCHAR *addr,u8 list_type);
void PLC_add_new_node(USHORT user_sn, UCHAR meter_type, UCHAR * addr);
BYTE PLC_Is_Meter_270(USHORT metersn);
BYTE PLC_IsTestMode();
BYTE PLC_Get_Meter_Baud(USHORT metersn);
USHORT PLC_Get_Baud_Number(BYTE plc_baudrate_storage);
USHORT PLC_Get_Baud_Default();
USHORT PLC_Is_Baud_SL();
 u8 X4_Task_AddtoSTA(u8 *addr,u8 Task_ID);
 u8 X4_Task_DeltoSTA(u8 *addr,u8 Task_ID);
unsigned short PLC_freeze_start();
void PLC_set_delay(UCHAR delay_seconds);
void PLC_get_relay_info();
void PLC_save_relay_info(void *ptmr, void *parg);
void PLC_Reset_freezeflag();

unsigned short PLC_stage_start();
unsigned short PLC_stage_pause();
unsigned short PLC_stage_resume();

void PLC_save_relay_timer_start();
void PLC_set_rf_channel_flag(BYTE flag);
unsigned short Relay_Stack_L0L1_ItemNum();
void PLC_trigger_rmsg_report();

/*for MF relay*/
unsigned char PLC_Is_MainNodeAddr(unsigned char * pAddr6);
unsigned short Relay_MF_Statistic();

#ifdef PROTOCOL_GD_2016
u8 PLC_add_read_meter_task(u8 sour_frome,u8 meter_moudle, u8 no_response_flag, u8 mac_addr[MAC_ADDR_LEN], EM_TASK_TYPE task_type, P_READ_TASK_HEADER pReadTask, u8* p_rt_err);
u8 PLC_delete_read_task_gd(EM_TASK_TYPE task_type, u16 taskid);
u8 PLC_delete_all_read_task_gd(EM_TASK_TYPE task_type);
void PLC_report_node_ifo_gd(P_MF_RELAY_REG_METER_INFO pRptMeterIfo);
void PLC_report_router_register_finish_gd();
BYTE PLC_update();
u16 get_multi_sta_sn(multi_sta_head *p_sta, u16 tei);

//X4
u8 PLC_PostCollTaskInit();      //初始化采集任务
u8 PLC_ReadCollTaskID(u8 mac_addr[MAC_ADDR_LEN], u8 packet_seq_3762);                       //查询采集任务号
u8 PLC_ReadCollTaskDetail(u8 mac_addr[MAC_ADDR_LEN], u8 task_id, u8 packet_seq_3762);       //查询采集任务详细信息
u8 PLC_DeleteCollTask(u8 mac_addr[MAC_ADDR_LEN], u8 task_id, u8 packet_seq_3762);           //删除采集任务

#endif
void PLC_NewStaJoinTimeDelay();
#ifdef GDW_ZHEJIANG
void PLC_NewStaJoinOnlineLockTimeSetTimeDelay();
void PLC_EventCollectReport();
#endif
BYTE PLC_freeze_save_data( P_MSG_INFO pMsg);
BYTE PLC_dirread_postabled(USHORT read_sn, PBYTE pCmd, USHORT cmd_len);
BOOL PLC_get_system_time(local_tm *p_sys_time);
BYTE PLC_Is_DL645_Protocol(BYTE plc_protocol);
//UCHAR PLC_Directread_Time_Related_Request(USHORT metersn, unsigned short relay_meter[MAX_RELAY_DEEP+1]);
//unsigned short PLC_Correct_07_97(unsigned short read_sn, P_GD_DL645_BODY pDL645Cmd);
unsigned short PLC_Correct_07_97(unsigned short read_sn, PBYTE pCmd, USHORT cmd_len);

unsigned short PLC_Post_Aframe(MSG_SHORT_INFO * pDl645Msg, 
            unsigned short *relay_meter, unsigned char relay_level, USHORT wait_time_01s);

unsigned short PLC_Wait_frame(USHORT read_sn, USHORT read_status, unsigned char *pFrmType, P_MSG_INFO* ppMsg);

BOOL is_Extend_ReadCollectGraphPkt(u8* data,u16 datalen);
USHORT PLC_Get_Router_Ifo(P_ROUTER_IFO pRouterIfo);
#if defined(PROTOCOL_GD_2016)
USHORT PLC_Get_Router_Ifo_gd(P_ROUTER_IFO_GD pRouterIfo);
#endif
USHORT PLC_report_msg_to_main();
  
unsigned char Relay_3Phase_FindMetersn(unsigned short metersn);

unsigned short Relay_GetRouteReadMin();
unsigned char Relay_MF_GetLikelyRelay2Meter(unsigned short metersn, unsigned short relay_meter[MAX_RELAY_DEEP]);
void Relay_MF_Set_LastItem(unsigned short metersn, unsigned char itemnum);
unsigned char Relay_MF_Rec_Dl645CmdItem(unsigned char * itemp, unsigned char item_len);
unsigned char Relay_MF_Get_Rec_Dl645CmdItem(const unsigned char * itemp, unsigned char item_len, unsigned char* pCmdItemCount);
unsigned char Relay_MF_Recd_Dl645RespLen(unsigned char * item68_6b, unsigned char len);
unsigned char Relay_MF_Recd_69845RespLen(P_DL69845_BODY p69845Cmd);
void Relay_MF_Save_DL645Resp(P_MSG_INFO   pMsg);
unsigned char Relay_MF_Get_DL645_01ff00(unsigned short metersn, unsigned char * pOutBuf);
unsigned char Relay_MF_Get_DL645_05060101(unsigned short metersn, unsigned char * pOutBuf);
//unsigned short Relay_MF_Direct_ReadMeter(unsigned short metersn, unsigned char meter_protocol_plc, unsigned char * p_645_msg, unsigned char len, UCHAR* pRelayAddr, UCHAR nRelayLevel, BYTE time_related, BYTE try_read);
unsigned short Relay_MF_DR3D5_Dir_ReadMeter(unsigned short metersn, unsigned char meter_protocol_plc, unsigned char * p_645_msg, unsigned char len, BYTE time_related);
unsigned char Relay_MF_Is_ReadStatus_04001501(unsigned char * pBuf);
//unsigned char Relay_MF_Rec_ResetStatus_04001503(unsigned short metersn, unsigned char * pBuf);
unsigned char Relay_MF_FilterEventReport(P_GD_REDUCED_DL645_BODY pDl645);

void Relay_MF_SSearch_ClrXFlg(unsigned char phase);
void Relay_MF_new_node(USHORT meter_sn);
void Relay_MF_del_node(USHORT meter_sn);
void Relay_MF_re_reg_node(USHORT metersn);
void Relay_MF_Task_proc(void *p_arg);
unsigned short Relay_Queue_Init();
unsigned char Relay_Stack_Init();
unsigned char Relay_Stack_Per_Node_Init();

void Relay_MF_broadcast_RxRPTFrame_YL(P_RELAY_PAYLOAD_FRAME pSelfRegFrame);
void Relay_MF_Ack_Resp_RxRPTFrame_YL(P_RELAY_PAYLOAD_FRAME pSelfRegFrame);
void Relay_MF_Rpt_Process(P_MF_RELAY_REG_METER_INFO pRptMeterIfo);

unsigned char Relay_MF_is_RPTFrame_YL(P_RELAY_PAYLOAD_FRAME pPayLoad);

unsigned char Relay_MF_XC_WRD();

unsigned char Relay_Stack_Per_Node_Gen();
unsigned short * Relay_Stack_Per_Node_Stk(unsigned short metersn);
unsigned char * Relay_Stack_Per_Node_StkMask(unsigned short metersn);

unsigned short Relay_Queue_Get_HeadItems(unsigned char * buf, unsigned char num);

unsigned char Relay_Stack_Unstable_List_Init();
#ifdef PROTOCOL_GD_2016
extern u16 areaTimerId;
unsigned short Relay_Trans_MeterColl2Sn(const P_METER_COLL_MAP pMap, unsigned short * pEmptySn);
#endif
unsigned short Relay_MF_searchknown_unused();
unsigned short Relay_MF_searchx_unused();
unsigned short Relay_MF_searchever_unused();
void Relay_MF_clear_prio();
unsigned short Relay_MF_Get_Dl645_CmdItems(unsigned char * buf);
unsigned short Relay_MF_Get_Dl645_RespItems(unsigned char * buf);
unsigned short Relay_MF_Get_Unfreezed_Meter_Arry(unsigned char * buf);
unsigned short Relay_MF_Get_Wrongupper_Meter_Arry(unsigned char * buf);
unsigned short Relay_MF_Get_UnfreezedDone_Meter_Arry(unsigned char * buf);
unsigned short Relay_MF_Get_Unfreezed_GrpNum_Arry(unsigned char * buf);
unsigned short Relay_MF_Get_Unknown_Meter_Arry(unsigned char * buf);
unsigned short Relay_MF_GetTotalCfged();
unsigned short Relay_MF_Get_NeighbourStat(unsigned char * buf);
unsigned short Relay_MF_forget_wrongupper();
void Relay_MF_remove_selfreged();
void Relay_MF_forget_group(unsigned short group_num);
void Relay_MF_forget_unstable();
void Relay_MF_forget_unfreezed();
void Relay_MF_forget_unfreezed_and_done();
void Relay_MF_forget_one_meter(unsigned short metersn);
void Relay_MF_Task_Reset(unsigned char pwr_on);
void Relay_MF_Task_Pause();
void Relay_Task_PauseAWhile();
void Relay_MF_Task_Pause_Sec(unsigned char sec);
u8 PLC_GetStaGetModuleIdRealTimeProc(OS_TICK_64 tick_now);

//获取458表地址
UCHAR PLC_GetRs485MeterAddr(P_GD_DL645_BODY pDl645, UCHAR* pRs485MeterAddr);
unsigned long PLC_GetDIByDL645(P_GD_DL645_BODY pDl645);
unsigned long PLC_GetDIByReducedDL645(P_GD_REDUCED_DL645_BODY pDl645);
unsigned char PLC_get_wait_rx_count();

void PLC_report_node_ifo(P_MF_RELAY_REG_METER_INFO pRptMeterIfo);
void PLC_report_dev_ifo(P_MF_RELAY_REG_METER_INFO pRptMeterIfo);

P_PACK_INFO PLC_GetPackInfoByDIIdx(BYTE di_idx);

BYTE PLC_IsFreezeDICfg();
BYTE PLC_IsTheFreezeDICfg(ULONG DI);

BYTE PLC_IsPackReadValidable();
BYTE PLC_IsPackInfoValid(P_PACK_INFO p_pack_info);

BYTE PLC_GetDIIdxByDL645(P_GD_DL645_BODY pDL645, BYTE* di_idx);
BYTE PLC_GetPackInfoIdx(const P_PACK_INFO p_pack_info, BYTE di_idx);
    
P_METER_DATA PLC_GetMeterDataByMetersn(unsigned short meter_sn);
BYTE PLC_GetMeterDataByDL645(const P_GD_DL645_BODY pDL645, BYTE** ppDataUnit, BYTE* pLen);
BYTE PLC_GetMeterDataByDIIdx(unsigned short meter_sn, BYTE di_idx, BYTE** ppDataUnit, BYTE* pLen);
BYTE PLC_DelMeterData();
BYTE PLC_ClearCacheData();
BYTE PLC_DelMeterDataByMetersn(unsigned short meter_sn);
BYTE PLC_freeze_update_meterdata(USHORT meter_sn, BYTE pack_flag, P_GD_REDUCED_DL645_BODY pDl645Resp);
USHORT PLC_get_sn_from_addr(UCHAR * addr);
P_DL69845_BODY PLC_check_69845_frame(UCHAR* buf, USHORT nLen);
BYTE PLC_is_freezedate_today(PBYTE freezedate);

AC_PHASE  PLC_PlmPhase_to_AcPhase(PLM_PHASE phase);
PLM_PHASE PLC_AcPhase_to_PlmPhase(AC_PHASE phase);
PLM_PHASE PLC_HplcPhase_to_PlmPhase(HPLC_PHASE phase);
PLM_PHASE PLC_PhyPhase_to_PlmPhase(u8 PhyPhase);
HPLC_PHASE PLC_PhyPhase_to_HplcPhase(u8 PhyPhase);

unsigned char PLC_3762protocol_to_meterprotocol(unsigned char protocol_3762);
unsigned char PLC_3762protocol_to_plcprotocol(unsigned char protocol_3762);
unsigned char PLC_plcprotocol_to_3762protocol(unsigned char protocol_plc);
unsigned char PLC_meterprotocol_to_plcprotocol(unsigned char protocol_meter);
unsigned char PLC_meterprotocol_to_3762protocol(unsigned char protocol_meter);

P_STA_INFO PLC_AllocStaInfo();
void PLC_FreeStaInfo(USHORT tei);

P_RELAY_INFO PLC_GetValidNodeInfoBySn(USHORT node_sn);

u8 PLC_IsTeiUsed(USHORT tei);
u8 PLC_GetStaLevel(USHORT tei);
OS_TICK_64 PLC_GetStaInNetTick(USHORT tei);
P_STA_INFO PLC_GetStaInfoByTei(USHORT tei);
P_STA_INFO PLC_GetValidStaInfoByTei(USHORT tei);
P_STA_INFO PLC_GetStaInfoByMac(PBYTE pMac);
HPLC_PHASE PLC_GetSendPhase(P_STA_INFO pSta);
USHORT PLC_GetStaSnByMac(PBYTE pMac);
int PLC_GetAppDstMac(u8 *meter,u8 *mac);

P_STA_STATUS_INFO PLC_GetStaStatusInfoByTei(USHORT tei);

P_RELAY_INFO PLC_GetWhiteNodeInfoByMac(PBYTE pMac);
USHORT PLC_GetWhiteNodeSnByMac(PBYTE pMac);
USHORT PLC_GetWhiteNodeSnByStaInfo(P_STA_INFO pSta);
void PLC_CopyCCOMacAddr(u8 mac_addr[MAC_ADDR_LEN], BOOL swap);

BOOL PLC_GetRealNodeSnCfg(BOOL usersn_from_zero, u16 usersn, u16* p_real_sn);
BOOL PLC_GetRealNodeSnSelfreg(BOOL usersn_from_zero, u16 usersn, u16* p_real_sn);
BOOL PLC_GetRealStaSn(BOOL usersn_from_zero, u16 usersn, u16* p_real_sn);

u8 PLC_AddToOfflineList(u8 mac_addr[MAC_ADDR_LEN], OFFLINE_REASON resson);
u16 PLC_GetOfflineWaitCount();

//切换载波频率
void PLC_ChangeFrequence(u8 new_frequence);
void HRF_ChangeFrequence(u8 channel,u8 option);
//搜表
u8 PLC_StartSelfReg(u16 time_last_min);
void PLC_StopSelfReg();
void PLC_SelfRegClearSendOffline();
void AddSelfRegCollReportMap(u8* meter_mac, u8 protocol, u8 meter_num, u16 coll_sn);
void DelSelfRegCollReportMap(u8 meter_num, u16 coll_sn);
void FindSelfRegCollReportMapFirstDev(u16* coll_sn);
unsigned char PLC_SendGetReadMeterColl(u16 tei);
unsigned char PLC_SendGetReadMeterCollStore(u16 tei);
//升级
u8 PLC_StartUpdateSta(u32 flash_addr, u32 file_size, u32 file_checksum);
u8 PLC_StopUpdateSta();

//台区识别
u8 PLC_StartAreaDiscernment();
void PLC_StopAreaDiscernment(void *agc);


void Task_Plc_proc(void *p_arg);

unsigned char PLC_IsNetComplited();

void PLC_TriggerSaveNodeInfo();
#ifdef GDW_ZHEJIANG
void PLC_TriggerSaveBlackNodeInfo();
#endif //GDW_ZHEJIANG
void PLC_TriggerSaveOutListMeterInfo();
void PLC_TriggerSaveStaInfo();
void PLC_TriggerOfflineList();
void PLC_TriggerChangeNetid(u32 after_sec);

u8 PLC_PostDirRead(P_READ_MSG_INFO pReadMsg);
u8 PLC_PostParallelRead(P_READ_MSG_INFO pReadMsg, u8 is_src_3762);
u8 PLC_Broadcast(P_MSG_INFO pMsgIfo);
u8 PLC_CommTest(P_COMM_TEST_INFO pTestMsg);
u8 PLC_InnerVer(u8* node_mac);
u8 PLC_PhaseErrStart(u8 isForce);
#ifdef HPLC_CSG
u8 PLC_ResetSta(P_RESET_STA_INFO pResetSta);
#endif

unsigned char PLC_SendStaPower(u8 node_mac[6],u16 func,u8 *data);
unsigned char PLC_SendRegisterRead(u8 node_mac[6], u8 *data, u16 len);

#ifdef GDW_JIANGSU
extern u16 routerReqeustCenterRTCTimerId;
extern u16 routerSendRTCTimerId;
extern u16 routerResendRTCTimerId;
extern u16 routerNewStaSendRTCTimerId;
#endif
#ifdef GDW_2019_GRAPH_STOREDATA
extern u16 storeDataReqeustTimerId;
extern u8 storeDateRequestCount;
void StoreDateRequestTime(void *arg);
u8 PLC_StoreDataBcastTime(void);

#endif
#ifdef GDW_PEROID_ID_MESSAGE_SET_CAPTURE
extern u16 IDMsgReqeustTimerId;
void ReadModuleIDMsg(void *arg);
#endif

#ifdef GDW_ZHEJIANG
void PLC_OnlineLockTimeTime(void);
bool PLC_Is_Black_Meter(UCHAR *addr);
#endif
#if defined (GDW_2019_GRAPH_STOREDATA)||defined(PROTOCOL_NW_2020)
u8 PLC_StoreDateSyncCfg(P_STORE_DATA_SYNC_CFG pSyncCfg);
#endif

#ifdef GDW_ZHEJIANG
void PLC_SetOnlineLockTime(u32 OnlineLockTime, u32 abnormalOfflineLockTime);
#endif
void PLC_ParamReset();
void PLC_DataReset();
void PLC_TaskReset(void);
void flash_test();
P_STA_BITMAP PLC_GetCCONeighbor();

void PLC_OnPlmRequestReadItem(P_PLM_READ_ITEM_RSP pPlmReadItem);
void PLC_OnPlmConfirm();

int insertAreaCCO(u8 mac[6]);
void getAreaCCO(u8 *mac,u8 sn);

void PLC_DetectNid();
unsigned char PLC_SendReadAck(u8 *pdata, u16 len);
bool SaveHandleMac(u16 SrcTei,u16 MsduSeq,u8 RebootTimes);


void PLC_IdentInNet(u16 tei);
BOOL PLC_IsInNet(u16 tei);
bool PLC_AskNewStaID(u16 tei);

void PowerOnUp3762(void *arg);
void PowerOffUp3762(void *arg);
void ClearOutListMeter(int clear_with);

void PLC_HandleMMFrame(P_MAC_HEAD pMacHead, P_MM_FRAME pMmFrame,P_HPLC_PARAM para);
void PLC_ResetPhaseFault(u16 tei);

bool Check_Relay_Addr(u8* mac);

#ifdef GDW_2019_GRAPH_STOREDATA
void PLC_AskNewSta(u16 tei);
#endif

typedef bool(*pPLC_AssocReqCheckAuthFunc)(u8* data);
typedef void(*pPLC_AssocCnfAddAuthFunc)(u8* data, u16 data_len);
extern const pPLC_AssocReqCheckAuthFunc PLC_AssocReqCheckAuthFunc;
extern const pPLC_AssocCnfAddAuthFunc PLC_AssocCnfAddAuthFunc;

bool PLC_AssocReqCheckAuth(u8* data);
void PLC_AssocCnfAddAuth(u8* data, u16 data_len);


extern u8  ReaderPhase;
#endif

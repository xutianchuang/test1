#ifndef CCTT_DEF_H
#define CCTT_DEF_H
#include "plc_autorelay.h"
//Message Type
#define GET_PLC_STD                         0x40
#define SHOW_ACTIVE_NODES                   0x41
#define EVENT                               0x55
#define ENABLE_EVENT                        0x5c  
#define DISABLE_EVENT                       0x5b
#define IEC_61334_4_32_LLC_BIND_SAP         0x20
#define IEC_61334_4_32_LLC_BIND_SAP_CNFR    0x21
#define IEC_61334_4_32_LLC_DATA_REQ         0x10
#define IEC_61334_4_32_LLC_DATA_CNFRM       0x11
#define IEC_61334_4_32_LLC_DATA_IND         0x12
#define IEC_61334_4_32_LLC_KEEP_ALIVE       0x50

//TLV-Type
#define UI_TLV_TYPE_ACTIVE_NODE_INFO_LIST   0x0400
#define UI_TLV_TYPE_ACTIVE_NODE_INFO        0x0401
#define UI_TLV_TYPE_UC_CONN_INFO_LIST       0x0410
#define UI_TLV_TYPE_UC_CONN_INFO            0x0411
#define UI_TLV_TYPE_EVENT                   0x0900
#define UI_TLV_TYPE_EVENT_ID                0x0901
#define UI_TLV_TYPE_EVENT_NR                0x0902
#define UI_TLV_TYPE_EVENT_SPECIFIC_INFO     0x0903
#define PLC_API_TLV_TYPE_DSAP               0x0200
#define PLC_API_TLV_TYPE_SSAP               0x0201
#define PLC_API_TLV_TYPE_SAP                0x0202
#define PLC_API_TLV_TYPE_DADDR              0x0210
#define PLC_API_TLV_TYPE_SADDR              0x0211
#define PLC_API_TLV_TYPE_PARAMS             0x0280

#define PLC_API_TLV_TYPE_TRX_HNDL           0x0270
#define PLC_API_TLV_TYPE_STATUS             0x0240
#define PLC_API_TLV_TYPE_SDU                0x0220
#define PLC_API_TLV_TYPE_KEEP_ALIVE         0x0612

#define UI_TLV_TYPE_NODE_EUI_48             0x0605
#define UI_TLV_TYPE_NODE_LNID               0x0611
#define UI_TLV_TYPE_NODE_SID                0x0600
#define UI_TLV_TYPE_NODE_LSID               0x0601
#define UI_TLV_TYPE_DEV_SERIAL_NR           0x0603
#define UI_TLV_TYPE_NODE_LEVEL              0x060c



#define MAX_NODES_SENT_PER_TIME             30


#define PLM_FRAME_PROC_ENTRANCE             0    //帧处理入口
#define PLM_MSG_PROC_ENTRANCE               1    //消息处理入口

#define METER_READ_ENABLED           0
#define METER_READ_DISABLED          1

//继电器动作控制字，黑龙江规约
#define DL645_97_RELAY_OPEN			    0xAA
#define DL645_97_RELAY_CLOSE			0x55 

#define METER_INFO_NUMER_PER_PAGE   (1024/sizeof(CCTT_MNG_METER))

/*
国网错误状态字：
 0为通信超时，1为无效数据单元，2为长度错，3为校验错误，4为信息类不存在，5为格式错误，        
 6为表号重复，7为表号不存在，8为电表应用层无应答，9为主节点忙， 10主节点不支持此命令， 
 11为从节点不应答， 12为从节点不在网内， 13～255备用
*/

/*
广东规约错误状态字：
0为通信超时，1为无效数据标识内容，2为长度错误，3为校验错误，
4为数据标识编码不存在，5为格式错误， 6为表号重复，7为表号不存在，8为电表应用层无应答，
9为主节点忙，10主节点不支持此命令，11为从节点不应答，12为从节点不在网内，

13为添加任务时剩余可分配任务数不足，14为上报任务数据时任务不存在,15为任务ID重复，
16为查询任务时模块没有此任务，FFH其他。
*/

//广东规约错误
#define RT_ERR_TASK_NOT_ID                 17
#define RT_ERR_TASK_NOT_EXIST              16
#define RT_ERR_TASK_ID_DUPLICATE           15
#define RT_ERR_TASK_MEMORY_NOT_ENOUGH      13

//国网错误
#define RT_ERR_PARALLEL_STA_BUSY                 111      //集中器抄表时，不允许并发给同一块电表多条抄表报文
#define RT_ERR_PARALLEL_OVER_MAX_STA_PACKET      110      //集中器下发报文中的电表协议报文条数超过允许的最大值
#define RT_ERR_PARALLEL_OVER_MAX_COO_PACKET      109      //抄表报文的并发个数超过 CCO 允许的最大值
#define RT_ERR_NODE_NOT_IN_NET             12
#define RT_ERR_NODE_NOT_RESPONSE           11
#define RT_ERR_CMD_NOT_SUPPORT             10
#define RT_ERR_BUSY_NOW                    9
#define RT_ERR_METER_NOT_RESPONSE          8
#define RT_ERR_METERNO_INEXISTS       0x07
#define RT_ERR_DUPLICATE_METERNO      0x06
#define RT_ERR_INVALID_FORMAT         0x05
#define RT_ERR_INVALID_DT             0x04
#define RT_ERR_INVALID_CRC            0x03
#define RT_ERR_INVALID_LEN            0x02
#define RT_ERR_INVALID_DATA_PAYLOAD   0x01
#define RT_ERR_TIMEOUT                0x00

#define RT_ERR_NONE                   0xff


#define COMMON_METER_RATES_NUM      1

#define    METER_INTEGER_NUM_6      2
#define    METER_DECIMAL_NUM_2      1

#define             F0              0
#define             F1              1
#define             F2              2
#define             F3              3
#define             F4              4
#define             F5              5
#define             F6              6
#define             F7              7
#define             F8              8
#define             F9              9
#define             F10             10
#define             F11             11
#define             F12             12
#define             F13             13
#define             F14             14
#define             F15             15
#define             F16             16
#define             F17             17
#define             F18             18
#define             F19             19
#define             F20             20
#define             F21             21
#define             F22             22
#define             F23             23
#define             F24             24
#define             F25             25
#define             F26             26
#define             F27             27
#define             F28             28
#define             F29             29
#define             F30             30
#define             F31             31
#define             F33             33
#define             F34             34
#define             F35             35
#define             F36             36
#define             F37             37
#define             F38             38
#define             F39             39
#define             F40             40
#define             F41             41
#define             F42             42
#define             F43             43
#define             F44             44
#define             F45             45
#define             F46             46
#define             F47             47
#define             F48             48
#define             F49             49
#define             F50             50
#define             F51             51
#define             F52             52
#define             F53             53
#define             F54             54
#define             F56             56
#define             F57             57
#define             F58             58
#define             F59             59
#define             F60             60
#define             F61             61
#define             F62             62
#define             F63             63
#define             F64             64
#define             F65             65
#define             F66             66
#define             F67             67
#define             F73             73
#define             F74             74
#define             F80             80
#define             F81             81
#define             F82             82
#define             F83             83
#define             F84             84
#define             F85             85
#define             F86             86
#define             F87             87
#define             F88             88
#define             F89             89
#define             F90             90
#define             F91             91
#define             F92             92
#define             F93             93
#define             F94             94
#define             F95             95
#define             F96             96
#define             F97             97
#define             F98             98
#define             F99             99
#define             F100           100
#define             F101           101
#define             F102           102
#define             F103           103
#define             F104           104
#define             F105           105
#define             F106           106
#define             F107           107
#define             F108           108
#define             F111           111
#define             F112           112
#define             F113           113
#define             F114           114
#define             F115           115
#define             F116           116
#define             F120           120
#define             F129           129
#define             F130           130
#define             F131           131
#define             F133           133
#define             F134           134
#define             F150           150
#define             F151           151
#define             F152           152
#define             F160           160
#define             F161           161
#define             F165           165
#define             F166           166
#define             F167           167
#define             F168           168
#define             F171           171
#define             F172           172
#define             F173           173
#define             F174           174
#define             F200           200
#define             F201           201
#define             F213           213
#define             F214           214
#define             F215           215
#define             F216           216
#define             F218           218
#define             F219           219
#define             F220           220
#define             F221           221
#define             F222           222
#define             F223           223
#define             F224           224
#define             F225           225
#define             F226           226
#define             F227           227
#define             F228           228
#define             F229           229
#define             F230           230
#define 			F231		   231
#define 			F232		   232
#define             F233           233
#define             F234           234
#define             F235           235
#define             F236           236
#define             F241           241
#define             F242           242
#define             F243           243
#define             F245           245
#define             F246           246
#define             F247           247
#define             F248           248
#define             F251           251
#define             F253           253
#define             F254           254

#define             ERR_CODE_OK                 0
#define             ERR_CODE_DATA_ERROR         1
#define             ERR_CODE_PWD_ERROR          2
#define             ERR_CODE_INVALID_DATA       3
#define             ERR_CODE_INVALID_VERSION    4



/* 应用层功能码AFN */
// 确认/否认
#define AFN_RESPONSE 0x00  
// 复位
#define AFN_RESET 0x01  
#define AFN_DR_EXTEND_01 0x01  
// 数据转发
#define AFN_DATA_TRANS 0x02  
#define AFN_DATA_TRANS_2BYTE 0x82
// 查询参数
#define AFN_READ_PARAM 0x03 
// 链路接口检测
#define AFN_MAC_CHECK 0x04  
// 控制命令
#define AFN_CTL_CMD 0x05 
// 主动上报
#define AFN_REPORT_INFO 0x06  

// 路由查询
#define AFN_RELAY_QUERY 0x10
// 路由设置
#define AFN_RELAY_SETTING 0x11
// 路由控制
#define AFN_RELAY_CTROL 0x12
// 路由数据转发
#define AFN_FRAME_FORWARD 0x13
#define AFN_FRAME_FORWARD_2BYTE 0x93
// 路由数据转发
#define AFN_REAME_READ 0x14
//文件传输
#define AFN_FEIL_TRANSFER 0x15
//表号上报
#define AFN_METER_REPORT 0x16

#define AFN_PARALLEL_READ_METER   0xF1

// 厂商保留
#define AFN_FACTORY_RESERVED 0xF0
#define AFN_FACTORY_READ      0xF3
#define AFN_FACTORY_WRITE     0xF5

//东软定义
#define AFN_EXT_DATA_QUERY 0x02  


/* 桢起始字节及结束字节定义 */
#define FRAME_PLM2005_START_FLAG                            	0x68
#define FRAME_PLM2005_END_FLAG                					0x16

#define LOAD_MANAGER_PROTOCAL_SIGN               0x01

#define LMP_LINK_CTRL_DIR               0x80
#define LINK_CTRL_DIR_DOWN               0x0
#define LINK_CTRL_DIR_UP                 0x80

#define LMP_LINK_CTRL_FCV                 0x10
#define LINK_CTRL_FCV_VALID               0x10
#define LINK_CTRL_FCV_INVALID             0x0

#define LMP_LINK_CTRL_FCB                 0x20

#define LMP_LINK_CTRL_PRM                 0x40

#define LMP_LINK_CTRL_FUNC                0x0f

#define THIS_TERMINAL_SELF                 0
#define THIS_TERMINAL_SELF_1               1

#define LMP_RECEIVE_BUFFER_NUMBER               5
#define LMP_RESPONSE_BUFFER_NUMBER              2


#define PLM_MSG_BUF_IDLE         0
#define PLM_MSG_BUF_BUSY         1


#define MAX_NOREADMETER_NUM	12		//不抄表时段数n=20(乱设置的,可以改)

#define DOWNLOAD_FRAM_NAME          0X09
#define DOWNLOAD_FRAM_ATTR           0X01

#define DOWNLOAD_DATAFLASH_NAME 0X0a
#define DOWNLOAD_DATAFLASH_ATTR  0X01




/* 返回值 */
#define RET_ERR 0
#define RET_OK BIT0
#define RET_RESP (RET_OK | BIT1)

/* Q/GDW桢格式 */
#define Q_GDW_FRM_START1 0
#define Q_GDW_FRM_LEN 1
#define Q_GDW_FRM_START2 5
#define Q_GDW_FRM_CTLW 6
#define Q_GDW_FRM_ADDR 7
#define Q_GDW_FRM_DATA 12
#define Q_GDW_FRM_CS(len) (len + 6)
#define Q_GDW_FRM_STOP(len) (Q_GDW_FRM_CS(len) + 1)

/* 应用层格式 */
#define Q_GDW_APP_AFN (Q_GDW_FRM_DATA)
#define Q_GDW_APP_SEQ (Q_GDW_FRM_DATA + 1)
#define Q_GDW_APP_DA1(id) (Q_GDW_FRM_DATA + 2 + id * 4)
#define Q_GDW_APP_DA2(id) (Q_GDW_FRM_DATA + 2 + id * 4 + 1)
#define Q_GDW_APP_DT1(id) (Q_GDW_FRM_DATA + 2 + id * 4 + 2)
#define Q_GDW_APP_DT2(id) (Q_GDW_FRM_DATA + 2 + id * 4 + 3)
#define Q_GDW_APP_DAT(id) (Q_GDW_FRM_DATA + 2 + id * 4 + 4)

/* 控制域说明 */
#define FLG_C_DIR BIT7 // 传输方向
#define FLG_C_PRM BIT6 // 启动标志位
#define FLG_C_FCB BIT5 // 桢计数位
#define FLG_C_ACD BIT5 // 要求访问位
#define FLG_C_FCV BIT4 // 桢计数有效位

/* 到DL645 消息类型 */
enum MSG_TO_PLM2005
{
    
    //MSG_PLM_REALTIME_WRITTEN_RESULT,
    //MSG_PLM_REALTIME_READING_RESULT, 
    MSG_PLM_MSG_BASE = 50,   
   
    MSG_PLM_CMD_BUTT,
};

enum EN_MONTH_FREEZE_ITEM
{
    en_md_ActEnergy, //F17：月冻结正向有/无功电能示值
    en_ms_ActEnergy, //F21：月冻结正向有功电能量（总、费率1～M）
    en_rd_ActEnergy, //F9: 抄表日冻结正向有/无功电能示值
    en_rr_Demand,    //F11：抄表日冻结电能表正向有/无功最大需量及发生时间

};


typedef unsigned short (*PLM_PROC_PTR)(pvoid h);

typedef unsigned char (*Q_GDW_PROC_PTR)(pvoid h);

/**************************04-F3***************************************/
// 主站IP 地址和端口
////////////////////////////////////////////////////////////////////////

#pragma pack(push,1)

typedef struct 
{
    unsigned char addr[6];
#ifndef GDW_2012
    unsigned short meter_sn;
#endif
    unsigned char protocol;    
}RT_NODE, *P_RT_NODE;

//上报抄读数据的结构体
typedef struct 
{
    USHORT meter_sn;
    u8   protocol;
    USHORT read_took_time;
    u8   data_len;
    u8   data[1];
}RT_13REPORT_READ_RESULT, *P_RT_13REPORT_READ_RESULT;
typedef struct 
{
    USHORT meter_sn;
    u8   protocol;
    u8   data_len;
    u8   data[1];
}RT_09REPORT_READ_RESULT, *P_RT_09REPORT_READ_RESULT;


typedef enum
{
    EVENT_DEVICE_TYPE_COLLECTION,
    EVENT_DEVICE_TYPE_METER,
    EVENT_DEVICE_TYPE_HPLC,
    EVENT_DEVICE_TYPE_PLC,
    EVENT_DEVICE_TYPE_RF,
    EVENT_DEVICE_TYPE_PLC_RF
}EM_EVENT_DEVICE_TYPE;

//上报从节点事件
typedef struct
{
    u8   device_type;
    u8   protocol;
    u8   data_len;
    u8   data[1];
}RT_REPORT_METER_EVENT, *P_RT_REPORT_METER_EVENT;
typedef struct
{
	u8 mac[6];
	u8 status;
	u32 time;
	u8 result;
    u8 chip_id[24];
}RT_REPORT_METER_LINE_STATUS;
typedef struct 
{
    u8   data_len;
    u8   data[1];
}RT_REPORT_METER_EVENT_GD, *P_RT_REPORT_METER_EVENT_GD;

//上报台区信息
typedef struct 
{
    u8 sta_addr[6];
	u8 device_type;
	u8 meter_num;
	u8 main_addr[6];
	u8 res[2];
	u8 meter_addr[0][6];
}RT_REPORT_METER_AREA_GD, *P_RT_REPORT_METER_AREA_GD;
//06F4 上报设备信息
typedef struct
{
    u8 node_addr[6];
    u8 protocol;
}AFN06_F4_SUB_NODE_INFO, *P_AFN06_F4_SUB_NODE_INFO;

typedef struct
{
    u8  node_addr[6];
    u8  protocol;
    u16 node_sn;
    u8  dev_type;
    u8  sub_node_count;
    u8  trans_sub_node_count;
    u8  data[1];     //P_AFN06_F4_SUB_NODE_INFO
}AFN06_F4_NODE_INFO, *P_AFN06_F4_NODE_INFO;

typedef struct
{
    u8  node_count;
    u8  data[1];        //P_AFN06_F4_NODE_INFO
}AFN06_F4_DEV_INFO, *P_AFN06_F4_DEV_INFO;

typedef struct
{
    u16  total;
	u16 start_sn;
    u8  num;      
}AFN06_F5_PHASE_HEAD, *P_AFN06_F5_PHASE_HEAD;

//afn10 f40回复ID
typedef struct
{
    u8   device_type;
    u8   mac[6];
    u8   id_type;
    u8   id_len;
    u8   data[0];
}AFN10_F40_ID_RESPONSE, *P_AFN10_F40_ID_RESPONSE;

//点抄应答的结构体

typedef struct _LMP_data_unit_
{
    unsigned char DT1;      
    unsigned char DT2;                      
    unsigned char data_unit[1];    
}LMP_DATA_UNIT, *P_LMP_DATA_UNIT;

typedef struct 
{
	unsigned char relay_mode:1;
	unsigned char slave_node_flag:1;
	unsigned char comm_module:1;
	unsigned char collision_flag:1;
    unsigned char relay_level:4;
			
	unsigned char channel_flag:4;
	unsigned char encode:4;
	
	unsigned char response_count;

	unsigned short rate:15;
	unsigned short unit:1;
	
	unsigned char seq_no;
	
}INFO_FIELD_DOWN, *P_INFO_FIELD_DOWN;


typedef struct 
{
	unsigned char relay_mode:1;
	unsigned char reserved2:1;	
	unsigned char comm_module:1;
	unsigned char reserved1:1;
    unsigned char relay_level:4;
				
	unsigned char channel_flag:4;
	unsigned char reserved3:4;
	
	unsigned char channel_phase:4;
	unsigned char channel_char:4;	

	unsigned char signal_cmd:4;
	unsigned char signal_resp:4;
		
	//unsigned char reserved4[2];
	unsigned char event_flag:1;
    unsigned char line_flag:1;      //线路标志
    unsigned char area_flag:1;      //台区标志，0从节点台区归属无异常，1从节点台区归属有异常
    unsigned char reserved4:5;
    unsigned char seq_no;

}INFO_FIELD_UP, *P_INFO_FIELD_UP;

typedef union
{
	INFO_FIELD_DOWN down_info;
	INFO_FIELD_UP up_info;
} INFO_FD;

typedef struct _LMP_link_head_
{
    unsigned char  firstStartByte;           //0x68
    unsigned short frm_len;                   //frame length D2~D15
    unsigned char  ctl_field;                //control word
    INFO_FD        Inf;            //信息域   
    unsigned char  user_data[1];                 
}LMP_LINK_HEAD, *P_LMP_LINK_HEAD;

typedef struct
{
    unsigned char  firstStartByte;           //0x68
    unsigned short frm_len;                   //frame length D2~D15
    unsigned char  ctl_field;                //control word
    unsigned char  user_data[1];                 
}LMP_LINK_HEAD_GD, *P_LMP_LINK_HEAD_GD;

typedef struct
{
    unsigned char  firstStartByte;           //0xfd
    unsigned char  addr[6];
    unsigned char  ctl_field;                //control word
    unsigned short user_len;
    unsigned char  user_data[1];
}LMP_LINK_HEAD_WIRELESS, *P_LMP_LINK_HEAD_WIRELESS;

typedef struct _LMP_request_info_
{
    unsigned char  plcPhase;           //0x68
    unsigned char  addr[6];                   //frame length D2~D15
    unsigned short  sn;                //control word
}NODE_RQ, *P_NODE_RQ;




typedef struct _plm_item_array_
{
	unsigned short 	dataItem;
	pvoid 		   	dataAddr;
	unsigned short 	dataLen;
	PLM_PROC_PTR  	diFunc;
	
	//PLM_PROC_PTR  diValidate;		/* 进行数据合法性校验 */
}PLM_ITEM_ARRAY, *P_PLM_ITEM_ARRAY;


typedef struct _plm_afn_array_
{
	unsigned char 		AFN;
	Q_GDW_PROC_PTR  	diFunc;  //AFN的入口处理函数	
}AFN_FUNC_INFO, *P_AFN_FUNC_INFO;



#define LMP_MAX_AFN_CODE                 (sizeof(_Q_GDW_AFN_PROC)/sizeof(Q_GDW_PROC_PTR))


/**************************09-F1***************************************/
// 下载文件设置
////////////////////////////////////////////////////////////////////////

typedef struct _cfg_file_begin_para_
{
    unsigned char fileName;         //文件标识
    unsigned char fileAttr;         //文件属性
    unsigned char fileCmd;          //文件指令
    unsigned short segmentNum;      //文件总段数
    unsigned short segmentLen;      //段字节长度
}CFG_FILE_BEGIN_PARA,*P_CFG_FILE_BEGIN_PARA;

typedef struct _cfg_file_trans_para_
{
    unsigned char fileName;         //文件标识
    unsigned char fileAttr;         //文件属性
    unsigned char fileCmd;          //文件指令
    unsigned short segmentNum;      //文件总段数
    unsigned short segmentOffset;   //段偏移
    unsigned short segmentLen;      //段字节长度
    unsigned short transLen;        //传输数据长度
    unsigned char  dataBuf[1];      //传输数据
}CFG_FILE_TRANS_PARA,*P_CFG_FILE_TRANS_PARA;

typedef struct
{
    unsigned char fileName;         //文件标识
    unsigned char fileAttr;         //文件属性
    unsigned char fileCmd;          //文件指令
    unsigned short segmentNum;      //文件总段数
    unsigned long segmentOffset;   //段偏移
    unsigned short segmentLen;      //段字节长度
    unsigned char  dataBuf[1];      //传输数据
}CFG_FILE_TRANS_PARA_GDW13,*P_CFG_FILE_TRANS_PARA_GDW13;

//0X0D  CLASS 2 F1

typedef struct
{
#ifdef GDW_2012
#ifdef GDW_NEIMENG //内蒙兼容09和13规约，确认帧按09标准
	USHORT router_status;
#else
    ULONG router_status;
#endif
#else
    USHORT router_status;
#endif
    USHORT wait_time;
}CONFIRM_DATA_UNIT,*P_CONFIRM_DATA_UNIT;

#pragma pack(pop)


#pragma pack(4)
//len = 24 + MSA_MESSAGAE_MAX_SIZE
typedef struct _LMP_app_info_
{
    unsigned char end_id;

    union
    {
        P_LMP_LINK_HEAD lmpLinkHead;	/* 想用它来保存进来的数据buffer 指针，后续可以一直使用其中参数*/
        P_LMP_LINK_HEAD_GD lmpLinkHeadGD;       //广东规约协议头
    };

    unsigned short appLayerLen;   //应用层长度, [AFN, 校验)
    unsigned char  linkFunc;  	//控制域功能码                   
    unsigned char  confirmFlag;

	unsigned char 	cur_afn_pos; //当前AFN在函数指针数组的位置
    unsigned long   curDT;		/* 当前待处理Fn 号i.e. F1, F8 ...  */	
    P_LMP_DATA_UNIT curProcUnit; 	/* 保存当前处理的数据单元标识DA  +  DT */   

    unsigned char* rxMsgBuf;     //接受消息处理缓冲区

    //发送响应报文组祯时用到的指针
	unsigned short responsePoint; //发送指针
	unsigned char msg_buffer[MSA_MESSAGAE_MAX_SIZE];     //发送buffer
}LMP_APP_INFO, *P_LMP_APP_INFO;
#define MAX_NOT_WHITE_STA  32
#define MAX_NOT_WHITE_SIZE 1999
typedef struct{
	u16 widx;
	u16 ridx;
	u8 timerId;
 //   u8 clearTimerId;
	u8 mac[MAX_NOT_WHITE_SIZE][6];
    u8 deviceType[MAX_NOT_WHITE_SIZE];
}NotWhiteSta_s;
extern NotWhiteSta_s NotWhiteSta;

typedef struct 
{
	u8 Chip[6];
	u8 Factory[2];
	u32 Factory_code;
	u32 AreaProtocol;
}ST_CODE_INFO_TYPE;
extern ST_CODE_INFO_TYPE DefaultCodeInfo;

unsigned short PLM_EvtDet_15minMeterAMR_register();
unsigned short PLM_EvtDet_autoFrozen_register();
unsigned short PLM_EvtDet_unAMR_register(unsigned char hostcmd);
extern unsigned short PLM_2005_postprocess(unsigned char end_id,  pvoid h);
extern unsigned short PLM_Cascade_postProcess(unsigned char end_id,  pvoid h);
extern unsigned short PLM_init(HANDLE h);
unsigned short PLM_proc(HANDLE h);
extern unsigned short PLM_gprs_login(pvoid h);
extern unsigned char find_AFN(AFN_FUNC_INFO* pItemArray, unsigned char AFN_code);
unsigned char find_DT(P_PLM_ITEM_ARRAY pItemArray, unsigned char DT);
extern USHORT on_lmp_proc(unsigned char enter_mode, HANDLE h);
BYTE LMP_DL645_Proc(BYTE end_id, PBYTE pDL645Msg);
unsigned char PLM_afn_file_trans(pvoid h);
unsigned char afn_verified_password(unsigned short pwd);
unsigned short PLM_add_vip_user(unsigned short sn);
unsigned char afn_hn_84_F1_sub_metertype(unsigned char metreage_G, unsigned char metreage_H);
extern unsigned char PLM_refresh_meter_counts(pvoid h);
unsigned short PLM_gprs_logout(pvoid h);
unsigned short PLM_heart_beat(pvoid h);
unsigned short PLM_gprs_login(pvoid h);
unsigned char PLM_get_vip_sn(unsigned short sn);
USHORT PLM_del_vip_sn(unsigned short sn);
unsigned char PLM_del_vip_data(unsigned char vip_sn);
unsigned char afn_F0_internal_debug(P_LMP_APP_INFO pLmpAppInfo);
unsigned char afn_factory_reserved(P_LMP_APP_INFO pLmpAppInfo);
unsigned char afn_factory_read(P_LMP_APP_INFO pLmpAppInfo);
unsigned char afn_factory_write(P_LMP_APP_INFO pLmpAppInfo);
unsigned char afn_15_file_trans(P_LMP_APP_INFO pLmpAppInfo);

USHORT PLM_set_vip_meter(P_LMP_APP_INFO pLmpAppInfo);
void PLM_send_amr_msg(UCHAR afn, USHORT da, UCHAR dt, UCHAR id, UCHAR sub_id);
unsigned short PLM_judge_vip_meter(unsigned short meter_sn);
unsigned short PLM_get_vip_meter_no(unsigned short meter_sn);
void PLM_clear_all_meter(unsigned short meter_type);
void PLM_delete_one_meter(unsigned short meter_sn);
unsigned char PLM_clear_vip_user();
void PLM_save_comm_flux(unsigned char end_id, unsigned short datalen);
unsigned char GetOnePosition(unsigned char inData, unsigned char startPosition);
P_LMP_LINK_HEAD PLM_Check_Frame_3762(PBYTE pdata, int datalen);
unsigned char LMP_send_read_frame_YL(USHORT metersn, UCHAR* pReadMsg, USHORT nReadMsgLen);
unsigned short LMP_Change_Addr_62056(unsigned char* p62056, unsigned short len, unsigned char isDown); //更改62056抄表报文的地址域
P_MSG_INFO LMP_make_frame_3762_cmd(u8 Afn, u8 Fn, PLM_PHASE phase, PBYTE pDataUnit, USHORT nDataLen, BOOL isPlcMeterCommModule, PBYTE pSrcAddr);
P_MSG_INFO LMP_make_frame_3762_resp(u8 Afn, u8 Fn, PLM_PHASE phase, u8* pDataUnit, USHORT nDataLen, BOOL isPlcMeterCommModule, u8* pSrcAddr, u8 packet_seq);
P_MSG_INFO LMP_make_frame_wireless(BYTE ctrl, PBYTE pDataUnit, USHORT nDataLen);
void PLM_send_debug_frame_wl(P_MSG_INFO pMsgSrc);

#ifdef PROTOCOL_GD_2016
P_MSG_INFO LMP_make_frame_gd_cmd(u8 Afn, ULONG DI, PBYTE pDataUnit, USHORT nDataLen, PBYTE pSrcAddr);
#endif
void PLM_ReportCollectSubNodeInfoAndDev(u8* pEventData, u16 len);
void PLM_ReportSelfRegStop(void);
bool PLM_ReportMeterEvent(EM_EVENT_DEVICE_TYPE device_type, u8 protocol_3762, BOOL isPlcMeterCommModule,u8 meter_addr[FRM_DL645_ADDRESS_LEN], u8* pEventData, u16 nEventLen);
#ifdef HPLC_CSG
bool PLM_ReportSelfReg(P_APP_SEARCH_UP pRegResult);
#else
bool PLM_ReportSelfReg(P_REGIST_CHECK_UP pRegResult);
bool PLM_ReportCollectSelfReg(void);
#endif
bool PLM_ReportAreaDiscernment(u8 meter_addr[FRM_DL645_ADDRESS_LEN], u8 main_node_mac[MAC_ADDR_LEN]);
void PLM_ReportParallelRead(P_READ_MSG_INFO pReadMsg, PLM_PHASE phase, u16 upType, u8* pRespData, u16 nRespLen);
void PLM_ReportRouterWorkStatus(u8 router_status);
void PLM_ReportDirRead(P_READ_MSG_INFO pReadMsg, PLM_PHASE phase, u8* pRespData, u16 nRespLen);
void PLM_ReportRouteRead(u8 protoxol_09or13,u16 user_sn, u8 node_addr[FRM_DL645_ADDRESS_LEN], u8 plm_protocol, PLM_PHASE phase, u8 took_time_sec, u8* pRespData, u16 nRespLen);
void PLM_ResponseStaInnerVerInfo(u8 node_mac[FRM_DL645_ADDRESS_LEN], u8* pRespData, u16 nRespLen);
void PLM_ResponseStaRegisterInfo(u8 node_mac[FRM_DL645_ADDRESS_LEN], u8* pRespData, u16 nRespLen);
void PLM_ResponseStaRFPower(u8 new_up, u8 node_mac[FRM_DL645_ADDRESS_LEN], u8* pRespData, u16 nRespLen);
void PLM_ResponseStaPower(u8 new_up, u8 node_mac[FRM_DL645_ADDRESS_LEN], u8* pRespData, u16 nRespLen);
void PLM_ResponseStaModuleID(u8* mac, u8 id_type, u8 dev_type, u16 seq, u8* data);
void PLM_RequestReadItem(P_PLM_READ_ITEM_CMD pReadItemCmd);
void PLM_RequestReadItem2FixTime(PLM_PHASE plm_phase,u8* data ,u16 datalen);
void PLM_RequestCentreTime(void);
void PLM_ReportNotWhite(void*);
void PLM_Task_Proc(void *p_arg);
bool IsRepeatNotWhiteSta(u8 *mac);
bool addReportNotWhiteSta(u8* mac);
void clearRepeatNotWhiteSta(void*);


void PLM_DeleteCollTaskResponse_gd(u8 packet_seq_3762, u8 status);
void PLM_ReadCollTaskIDResponse_gd(u8 packet_seq_3762, u8* pRespData, u16 nRespLen);
void PLM_ReadCollTaskDetailResponse_gd(u8 packet_seq_3762, u8* pRespData, u16 nRespLen);
void PLM_ReportCollDataTaskStatus_gd(u8 task_id, PBYTE pNodeAddr, u8 task_status);
void PLM_ReportCollDataTaskData_gd(u8* pRespData, u16 nRespLen);
void PLM_DeleteCollTaskResponse_gd(u8 packet_seq_3762, u8 status);
void PLM_ReadCollTaskIDResponse_gd(u8 packet_seq_3762, u8* pRespData, u16 nRespLen);
void PLM_ReadCollTaskDetailResponse_gd(u8 packet_seq_3762, u8* pRespData, u16 nRespLen);
void PLM_ReportTaskStatus_gd(u8 sour_frome,USHORT task_id, PBYTE pNodeAddr, u8 task_status);
void PLM_ReportTaskRead_gd(u8 sour_frome,u16 task_id, u8 mac_addr[MAC_ADDR_LEN], u8* pRespData, u16 nRespLen);
void PLM_ReportStaStatus_gd(PBYTE data, u16 len);
void PLM_ReportChannelInfo_gd(PBYTE data, u16 len);
void PLM_ReportChannelNewInfo_gd(PBYTE data,u16 len);
void PLM_ReadStaModuleInfoResponse_gd(PBYTE data, u16 len);
void PLM_RequestCentreTime_gd(void);
#endif

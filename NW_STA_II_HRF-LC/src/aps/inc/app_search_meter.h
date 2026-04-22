#ifndef _APP_SEARCH_METER_H
#define _APP_SEARCH_METER_H
#if 0

//add by LiHuaQiang 2020.10.8 -START-
#define PRO_645_97        0x01
#define PRO_645_07        0x02
#define PRO_698           0X03    
#define READ_ACK_07       0x91
#define READ_ACK_07_B1    0xB1
#define READ_METER_ACK    0x93
#define READ_ACK_97       0x81


#define SEARCH_LEVEL0 99  //搜表级别:0级 搜波特率
#define SEARCH_LEVEL1 0  //搜表级别:1级
#define SEARCH_LEVEL2 1  //搜表级别:2级
#define SEARCH_LEVEL3 2  //搜表级别:3级
#define SEARCH_LEVEL4 3  //搜表级别:4级
#define SEARCH_LEVEL5 4  //搜表级别:5级
#define SEARCH_LEVEL6 5  //搜表级别:6级
#define SEARCH_LEVEL7 6  //搜表级别:7级
#define SEARCH_LEVEL8 7  //搜表级别:8级
#define SEARCH_LEVEL9 8  //搜表级别:9级
#define SEARCH_LEVEL10 9 //搜表级别:10级
#define SEARCH_LEVEL11 10//搜表级别:11级
#define SEARCH_LEVEL12 11//搜表级别:12级


#define CONFLICT_CNT_MAX_PRO645           16//645规约在某一级搜表时允许冲突的最大数
#define CONFLICT_CNT_MAX_PRO698           10//698规约在某一级搜表时允许冲突的最大数

#define GOTO_LOW_LEVEL_VALUE_698          11
#define GOTO_LOW_LEVEL_VALUE_645          101

#define UART_SCAN_485                     0x01//搜表时串口占用标志
#define UART_EVENT_DATA                   0x02//读事件时串口占用标志
#define UART_NORMAL_DATA_NO_UPLOAD        0x03
#define UART_IFR_DATA                     0x04//红外抄表时串口占用标志
#define UART_CHECK_POWERCUT_FRAME         0x05//停电时发探测帧占用标志
#define UART_CHECK_NO_EXIST_METER_FRAME   0X06//检测不存在的电表

#define SEARCH_METER_1500MS               10   //15*100ms=1.5s
#define IFR_READ_METER_5000MS             50  //15*100ms=1.5s

#define TABLE_MAX_SIZE                    32*7//32块表地址和表协议类型

#define METER_NUM_MAX                     32

typedef uint8_t StackEntry;
#define MAX_STACK 30
typedef struct stack
{
    StackEntry item[MAX_STACK];
    int top;
}STACK;



struct CONFLICT_TABLE
{
	
	u8 conflict_level;// 当前冲突级别	
};

struct CONFLICT_LEVEL1//一级搜表冲突列表存储
{
	
	u8 conflict_count;// 每级冲突单独计数
	u8 conflict_list[CONFLICT_CNT_MAX_PRO645];//冲突列表    (   序号)    最多有16个表地址冲突(因为总表数是32,32/2=16)
};

struct CONFLICT_LEVEL2//二级搜表冲突列表存储
{
	
	u8 conflict_count;// 每级冲突单独计数
	u8 conflict_list[CONFLICT_CNT_MAX_PRO645][2];//冲突列表    (   序号)  最多有32个表地址冲突(因为总表数是32,32*2/2=32)
};

struct CONFLICT_LEVEL3//三级搜表冲突列表存储
{
	
	u8 conflict_count;// 每级冲突单独计数
	u8 conflict_list[CONFLICT_CNT_MAX_PRO645][3];//冲突列表    (   序号)
};

struct CONFLICT_LEVEL4//四级搜表冲突列表存储
{
	
	u8 conflict_count;// 每级冲突单独计数
	u8 conflict_list[CONFLICT_CNT_MAX_PRO645][4];//冲突列表    (   序号)
};

struct CONFLICT_LEVEL5//五级搜表冲突列表存储
{
	
	u8 conflict_count;// 每级冲突单独计数
	u8 conflict_list[CONFLICT_CNT_MAX_PRO645][5];//冲突列表    (   序号)
};

struct CONFLICT_LEVEL6//六级搜表冲突列表存储
{
	
	u8 conflict_count;// 每级冲突单独计数
	u8 conflict_list[CONFLICT_CNT_MAX_PRO645][6];//冲突列表    (   序号)
};
//--------------------------------------------------------------------------
//698.45因为是半字节方式搜表，所以有12级搜表，以下结构体用于698.45协议搜表
struct CONFLICT_LEVEL7//7级搜表冲突列表存储
{
	u8 conflict_count;// 每级冲突单独计数
	u8 conflict_list[CONFLICT_CNT_MAX_PRO698][7];//冲突列表    (   序号) 10代表在该级搜表时最多有10个表冲突
};
struct CONFLICT_LEVEL8//8级搜表冲突列表存储
{
	u8 conflict_count;// 每级冲突单独计数
	u8 conflict_list[CONFLICT_CNT_MAX_PRO698][8];//冲突列表    (   序号) 10代表在该级搜表时最多有10个表冲突
};
struct CONFLICT_LEVEL9//9级搜表冲突列表存储
{
	u8 conflict_count;// 每级冲突单独计数
	u8 conflict_list[CONFLICT_CNT_MAX_PRO698][9];//冲突列表    (   序号) 10代表在该级搜表时最多有10个表冲突
};
struct CONFLICT_LEVEL10//10级搜表冲突列表存储
{
	u8 conflict_count;// 每级冲突单独计数
	u8 conflict_list[CONFLICT_CNT_MAX_PRO698][10];//冲突列表    (   序号) 10代表在该级搜表时最多有10个表冲突
};
struct CONFLICT_LEVEL11//11级搜表冲突列表存储
{
	u8 conflict_count;// 每级冲突单独计数
	u8 conflict_list[CONFLICT_CNT_MAX_PRO698][11];//冲突列表    (   序号) 10代表在该级搜表时最多有10个表冲突
};
//第12级为最后一级搜表，还有冲突说明表地址完全一样，不再记录

typedef struct 
{
    u8  table[224];
    u32 TableList;
    u8  table_index;
    u8  meter_total;
}TABLE_COPY;

typedef struct
{
	u8 is_exist_flag[32];
	u8 meter_count;
	u8 check_count;
}CHECK_NO_EXIST_METER;

#pragma pack(1)
struct strList_Meter
{ 
    u8 address[6];  
    struct LIST{    
    u8 Statute:2;   // 规约类型 01为645-97规约，02为645-07规约，03为698.45规约
    u8 BaudType:4;  // 波特率类型 0x01:   1200bps   0x02:  2400bps   0x03:4800bps   0x04:9600bps    0x05:19200
    u8 meter_type1:1;// 0:09表 1:13表
    u8 meter_type2:1;// 0:09单相表 1:09三相表
    }Type;
};
struct frame645
{
    unsigned char frame_start_flag1;
    unsigned char addr[6];
    unsigned char frame_start_flag2;
    union
    {
        unsigned char control_byte;
        struct
        {
            unsigned char function_flag:5;
            unsigned char following_flag:1;
            unsigned char exception_flag:1;
            unsigned char direction_flag:1;
        } control_bits;
    } control_code;
    unsigned char datalen;
    unsigned char data[255];//[50];
};
#pragma pack()

typedef struct
{
	bool check_meter_list_power_on_flag;//true:上电检索停电过的电表是否复电 false:关闭检索
	u8   check_power_off_flag;//true: 是否是第一次检测是否掉电
	bool check_table_flag;//true:启动搜表 false:关闭搜表
	bool send_AA_flag;
	bool search645_over_flag;
	bool search698_over_flag;
	bool search645_97_over_flag;
	u8   wait_485data_flag;
	u8   sub_count;
    u8   list_count;
	u8   table[TABLE_MAX_SIZE];//存储485列表和对应规约、波特率,定时搜表前不会被清0	
	u8   table_index;//记录索引表最后一个记录的索引地址
	u8   meter_total;// 检索到的电表总数	
	u32  TableList;//记录32只表存储状态，置1表示存在表记录
	u32  back_List;//检测白名单返回的LIST 标志位,用于比对原来记录的标志位，从而找出那些表被变更
	u32  end_search_tick;
	u8   search_times;//每个地址搜表的次数
	bool firt_check_table_flag;//true: 是上电第一次搜表
	u8   sub_count_99_twice;
	bool check_meter_type_end_flag;//true:检查表的类型结束
	bool search_one_meter_flag;//true:全A搜到表 false:全A未搜到表
	bool rs485_conflict_flag;//true:485口接收的数据有冲突 
	u8   online_addr[6];
}SEARCH_METER_FLAG_STRUCT;

extern SEARCH_METER_FLAG_STRUCT SEARCH_METER_FLAG;
extern struct strList_Meter meter_list;
extern u8 ReadPower645_07[20];//extern u8 ReadPower645_07[16];//
extern u32 AppUseUartFlag;

void SEARCH_METER_FLAG_init(void);
bool GetCMLPowerOnFlag(void);
void SearchMeterStart(void);
void SetWait485DataFlag(u8 data);
u8 GetWait485DataFlag(void);
void SearchMeter(void);
void t_table_copy_fun(void);
void t_CHECK_NO_EXIST_METER_init_fun(void);
void TableCopyInit(void);
void t_no_exist_meter_init_fun(void);
void conflict_val_record(void);
bool GetFirstCheckTableFlag(void);
void check_no_exist_meter_fun(void);
void t_CHECK_NO_EXIST_METER_set_fun(void);
void write_meter_list(u8 *addr,u8 protocol_type);
void write_meter_list2(u8 *addr,u8 protocol_type);
void RS485DataDeal(u8 srcAddr[6], u8 dstAddr[6], u16 seq, bool vaild, u8* data, u16 len, u8 sendType, u16 srcTei);
bool GetCheckMeterEndFlag(void);
void SetCheckMeterEndFlag(bool data);
bool GetSearchOneMeterFlag(void);
bool GetCheckTableFlag(void);
bool IsMeterInTable(u8 *addr);
void BindCollectorAddr(void);
void SetCheckTableFlag(bool data);
void SetCMLPowerOnFlag(bool data);
bool GetSerach645OverFlag(void);

//add by LiHuaQiang 2020.10.8 -END-

#ifdef II_STA_NO_9600_BAUD
#define BAUDTABLE_T_SIZE 3
#else
#define BAUDTABLE_T_SIZE 4
#endif

extern const u32 baudTable_t[BAUDTABLE_T_SIZE];

u8 BaudValidityCheck( u32 CurrentBaud_t );
u32 FindBaudInSEARCH_METER_FLAG_table( u8 dstAddr[6] );
bool ChangeBaudInSEARCH_METER_FLAG_table(u8 dstAddr[6], u8 baudType);
u32 GetBaudFromType( u8 BaudType_t );
u8 GetBaudType( u32 CurrentBaud_t );
bool GetNextBaudType(u32 CurrentBaud_t, u8* NextBaudType);
#endif
#endif

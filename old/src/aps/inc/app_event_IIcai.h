#ifndef _APP_EVENT_IICAI_H
#define _APP_EVENT_IICAI_H
//add by LiHuaQiang 2020.10.9 -START-
typedef unsigned long long UINT64;
typedef unsigned int       UINT32;
typedef unsigned short     UINT16;
typedef unsigned char      UINT8;

#define OK  1
#define NO  0
#define ON  1
#define OFF 0
#define StaEventTypeIsPowerCut_addr     3 //II采 上报事件类型是: 停电事件(带地址)
#define StaEventTypeIsRecovePower_addr  4 //II采 上报事件类型是: 复电事件(带地址)
#define RS485_BUFFER_SIZE       55 //29//每个缓冲区29个字节 读事件最长报文的长度为29个字节
#define RS485_MULTI_SEND        480//100//100个缓冲区  09类型表读取的事件很多，所以缓冲区个数要多一些

#define STATUE_BUFFER_SIZE      12 //每个缓冲区12个字节 主动上报状态字为12个字节
#define STATUE_MULTI            32 //32个缓冲区 最多32个09类型表

#define EVENT_HPLC_BUFFER_SIZE  100//50 //每个缓冲区50个字节
#define EVENT_HPLC_MULTI_SEND   32//64 //64个缓冲区 最多32个698协议表，每个698协议表两个状态字，故32*2=64

//645-07事件相关数据标识宏定义
#define DI_event_statue_H         0x37334834//事件主动上报状态字
#define DI_reset_event_statue_H   0x37334836//复位事件主动上报状态字

#define DI_open_meter_cover_H     0x36634033//开表盖总次数
#define DI_power_cut_H            0x36443333//掉电总次数
#define DI_calibration_time_H     0x36633733//校时总次数
#define DI_meter_run_statue1_H    0x37333834//电表运行状态字1

#define DI_open_button_box_H      0x36634133//开端钮盒总次数
#define DI_phaseA_volte_lose_H    0x43343334//A相失压总次数
#define DI_phaseB_volte_lose_H    0x43353334//B相失压总次数
#define DI_phaseC_volte_lose_H    0x43363334//C相失压总次数
#define DI_phaseA_current_lose_H  0x4B343334//A相失流总次数
#define DI_phaseB_current_lose_H  0x4B353334//B相失流总次数
#define DI_phaseC_current_lose_H  0x4B363334//C相失流总次数
#define DI_phaseA_break_H         0x46343334//A相断相总次数
#define DI_phaseB_break_H         0x46353334//B相断相总次数
#define DI_phaseC_break_H         0x46363334//C相断相总次数
#define DI_volte_reverse_H        0x47333334//电压逆相序总次数

#define METERLIST_REPORT_SER_NUM  1//搜表信息上报序列号
#define EVENT_REPORT_SER_NUM      2//事件采集上报序列号
#define RECOVE_REPORT_SER_NUM     3//复电上报序列号

#define COLLECT_METER_NUM         32//收集其他停电II采节点的停电报文电表地址个数10
#define COLLECT_POW_MAX_SIZE      COLLECT_METER_NUM*7// 7 = 6(表地址) + 1(带电状态)

#pragma pack(1)
typedef struct{
UINT64 bit0:1; UINT64 bit1:1; UINT64 bit2:1; UINT64 bit3:1; UINT64 bit4:1; UINT64 bit5:1; UINT64 bit6:1; UINT64 bit7:1;
UINT64 bit8:1; UINT64 bit9:1; UINT64 bit10:1;UINT64 bit11:1;UINT64 bit12:1;UINT64 bit13:1;UINT64 bit14:1;UINT64 bit15:1;
UINT64 bit16:1;UINT64 bit17:1;UINT64 bit18:1;UINT64 bit19:1;UINT64 bit20:1;UINT64 bit21:1;UINT64 bit22:1;UINT64 bit23:1;
UINT64 bit24:1;UINT64 bit25:1;UINT64 bit26:1;UINT64 bit27:1;UINT64 bit28:1;UINT64 bit29:1;UINT64 bit30:1;UINT64 bit31:1;
UINT64 bit32:1;UINT64 bit33:1;UINT64 bit34:1;UINT64 bit35:1;UINT64 bit36:1;UINT64 bit37:1;UINT64 bit38:1;UINT64 bit39:1;
UINT64 bit40:1;UINT64 bit41:1;UINT64 bit42:1;UINT64 bit43:1;UINT64 bit44:1;UINT64 bit45:1;UINT64 bit46:1;UINT64 bit47:1;
UINT64 bit48:1;UINT64 bit49:1;UINT64 bit50:1;UINT64 bit51:1;UINT64 bit52:1;UINT64 bit53:1;UINT64 bit54:1;UINT64 bit55:1;
UINT64 bit56:1;UINT64 bit57:1;UINT64 bit58:1;UINT64 bit59:1;UINT64 bit60:1;UINT64 bit61:1;UINT64 bit62:1;UINT64 bit63:1;
UINT32 bit64:1;UINT32 bit65:1;UINT32 bit66:1;UINT32 bit67:1;UINT32 bit68:1;UINT32 bit69:1;UINT32 bit70:1;UINT32 bit71:1;
UINT32 bit72:1;UINT32 bit73:1;UINT32 bit74:1;UINT32 bit75:1;UINT32 bit76:1;UINT32 bit77:1;UINT32 bit78:1;UINT32 bit79:1;
UINT32 bit80:1;UINT32 bit81:1;UINT32 bit82:1;UINT32 bit83:1;UINT32 bit84:1;UINT32 bit85:1;UINT32 bit86:1;UINT32 bit87:1;
UINT32 bit88:1;UINT32 bit89:1;UINT32 bit90:1;UINT32 bit91:1;UINT32 bit92:1;UINT32 bit93:1;UINT32 bit94:1;UINT32 bit95:1;
}event_statue_bit_645;
typedef struct{
uint32_t open_meter_cover_cnt;//开表盖总次数
uint32_t power_cut_cnt;//掉电总次数
uint32_t calibration_time_cnt;//校时总次数
uint32_t meter_run_statue1;//电表运行状态字1
uint32_t open_button_box_cnt_cnt;//开端钮盒总次数
uint32_t phaseA_volte_lose_cnt;//A相失压总次数
uint32_t phaseB_volte_lose_cnt;//B相失压总次数
uint32_t phaseC_volte_lose_cnt;//C相失压总次数
uint32_t phaseA_current_lose_cnt;//A相失流总次数
uint32_t phaseB_current_lose_cnt;//B相失流总次数
uint32_t phaseC_current_lose_cnt;//C相失流总次数
uint32_t phaseA_break_cnt;//A相断相总次数
uint32_t phaseB_break_cnt;//B相断相总次数
uint32_t phaseC_break_cnt;//C相断相总次数
uint32_t volte_reverse_cnt;//电压逆相序总次数
}event_count_645;
#pragma pack()

#pragma pack(1)
typedef struct{
uint8_t  rec_first_powercut_flag:1;//接收到第一帧停电报文标志
uint8_t  start_collect_Flag:1;//未停电节点开始搜集停电报文标志
uint8_t  reserve:6;
uint32_t start_collect_time;//未停电收到第一帧停电报文时间点
uint8_t sta_event_type;
uint8_t meter_num;//收集的停电电表个数低字节
uint8_t meter_num_h;//收集的停电电表个数高字节
uint8_t buff[COLLECT_POW_MAX_SIZE];//收集停电报文缓冲区
}collect_powercut_struct;

#pragma pack()

extern uint8_t DI_event_statue[4];//事件主动上报状态字
extern uint8_t DI_reset_event_statue[4];//复位事件主动上报状态字
extern uint8_t DI_phaseB_volte[4];//B相电压

extern u8 collector_addr[6];
extern u8 answer_reset_statue_09_meter[12];

extern u8 poweroff_report_buff[42];
extern u8 list_flash_power_off[42];

u8 get_read_event_start_flag(void);
void set_read_event_start_flag(u8 data);
void report_meterlist_flag_set_fun(u8 flag);
void delete_wrong_meter(void);
void check_meter_type(void);
void event_port_monitor(void);
void get_collector_addr_fun(void);
void hplc_start_report_meterlist_fun(void);
void AppEventReportIIcai(void);
void write_09meter_event_to_flash(void);
void read_09meter_event_from_flash(void);
u8 is_right_reset_event_statue_645_09ver_fun(uint8_t *data,uint16_t len);
void write_list_flash_power_off(void);
void read_list_flash_power_off(void);
void check_meter_list_power_off(void);
bool GetCheckPowerOffMeterEndFlag(void);
void event_RS485_send_data_queue_in(void);
void check_meter_list_power_on(void);
void check_meter_list_power_on_var_init(void);
void rs485_recv_event_data_deal_fun(struct frame645 *pframe,UINT16 receive_len);
void SetCheckPowerOffMeterEndFlag(bool data);

extern void II_STA_PowerOffEventReport(u8 *data,u8 poweroff_meter_num);
//void II_STA_PowerOnEventReport(void);
void reset_cco_comfirm_flag_fun(uint16_t *serial,uint8_t *addr);
//add by LiHuaQiang 2020.10.9 -END-
#endif

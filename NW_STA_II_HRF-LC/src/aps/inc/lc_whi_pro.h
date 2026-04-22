#ifndef __LC_WHI_PRO_H__
#define __LC_WHI_PRO_H__
#include <stdint.h>
#include <gsmcu_hal.h>

#ifdef PLAT_READ_METER
// 0x0103：支持拉闸、合闸操作
// 0x0104: 命令行写/读/清除project key
// 0x0105: 解决表入网了但是在主站还是显示离线的问题
// 0x0106：改进载波广播补发操作
// 0x0107：调试版本
// 0x0108：修复多级载波网络广播失败的问题
// 0x0109：从信标里获取启动绑定的操作条件
// 0x0110: 事件上报入网事件
// 0x0111: 上线事件始终上报，不受上报配置影响
#define WHI_PRO_VER		(0x0111)

#elif PLAT_II_485_READ_METER
// 0x0103: 二采抄表，通过命令行编辑表地址(set_meter_mac, clear_meter_mac, show_meter_mac)，最多支持20个表
//				set_meter_mac meter_index xx xx xx xx xx xx baudrate
//				meter_index: 表序号0-19，必须依次写入，表序号之间不能有跳号，0号表mac地址作为二采的mac地址
//				xx xx xx xx xx xx：表mac地址
//				baudrate：表485通信波特率
// 0x0104：物模型读写表地址、复位
// 0x0105：改进载波广播补发操作
// 0x0106：调试版本
// 0x0107：修复多级载波网络广播失败的问题
// 0x0108：从信标里获取启动绑定的操作条件
// 0x0109：增加LED灯指示
// 0x0110: 事件上报入网事件
// 0x0111: II采支持波特率为115200的表；上线事件始终上报，不受上报配置影响
// 0x0112：入网后等待一个随机时间后才上报
// 0x0113: 解决重启后从表配置丢失bug
// 0x0114: 增加从表列表结束的判断
#define WHI_PRO_VER		(0x0114)

#elif PLAT_HPLC_SWITCH
// 0x0102：从信标里获取启动绑定的操作条件
#define WHI_PRO_VER		(0x0102)

#else
// 0x0220: 宽带工作频段改为1，及加大载波功率
// 0x0222: 广播帧储存重发
// 0x0223：重构灯孔程序代码；修复存储
// 0x0224：
// 0x0225：修復常亮模式任務
// 0x0226: 
// 0x0228: 策略结构更改
// 0x0230: 事件上报
// 0x0231: 储存重发的广播帧增加超时控制，超时就不再重发；快速调灯模式和亮度的广播帧不存储重发
// 0x0232：更新雷达统计操作
// 0x0233：广播不停的发/
// 0x0234：判断载波节点符合离线条件时，给该节点广播离线指示
// 0x0235：1.修改事件上报添加动态value长度特性；2.添加冻结数据主动上报；3.添加事件使能控制
// 0x0237: 命令行写/读/清除project key
// 0x0238：增加恒照度模式
// 0x0239：支持面板开关、低功耗设备上报电量
// 0x0240：修改按键识别流程，添加恒照模式记录亮灯时长
// 0x0241：串口接收处理时增加多帧检查
// 0x0242：修复电池电压事件上报错误
// 0x0243：改进载波广播补发操作
// 0x0244：调试版本
// 0x0247：修复多级载波网络广播失败的问题
// 0x0248：从信标里获取启动绑定的操作条件
// 0x0249：修改BLE_MAX_LAYER_VALUE=15
// 0x0250：因为KEY不正确入网被拒时上报
// 0x0251：修改开关控制步进值掉电存储、面板开关电池电压上报时间限制、物模型增加传感器列表、修复切换时段策略时雷达不触发不切换无人亮度、添加灯参数存储版本更新兼容历史数据、支持绑定3个面板开关
// 0x0252: 事件上报入网事件；载波入网后指示蓝牙重新组网
// 0x0253：载波离网后指示蓝牙重新组网
// 0x0254：上线事件始终上报，不受上报配置影响
// 0x0255：入网后等待一个随机时间后才上报
// 0x0256: 光照传感器六小时上报一次电池电压
// 0x0257: 更新phy.a库 - 20240925
// 0x0258: 添加照度感应模式。修改存储结构升级功能，支持v0238及其之前版本存储信息升级，事件上报总开关默认开启
// 0x0259: D8灯在未入网状态下不处理载波控制报文
#define WHI_PRO_VER		(0x0259)

#endif

#define WHI_PRO_STAT_HEAD_SIZE	(8)
#define WHI_PRO_END_HEAD_SIZE	(2)
#define WHI_PRO_HEAD_SIZE	(10)
#define LAMP_BLE_DEVICE_TYPE (2)


//==
#define WHI_READ_VERSION     (0x0001)
#define WHI_READ_MAC         (0x0002)
#define WHI_READ_ADDR        (0x0003)
#define WHI_SET_ADDR         (0x0004)
#define WHI_RESET_MODEL      (0x0005)
#define WHI_FILE_CMD         (0x0006)
#define WHI_READ_RUN_TIME    (0x0007)
#define WHI_READ_RESET_INFO    (0x0008)
#define WHI_READ_DEVICE_TYPE (0x0009)

#define WHI_READ_NODE_SUM    (0x0010)
#define WHI_READ_NODE_INFO   (0x0011)
#define WHI_ADD_NODE_INFO    (0x0012)
#define WHI_DEL_NODE_INFO    (0x0013)
#define WHI_CLEAR_NODE_INFO      (0x0014)
#define WHI_SET_ZZW_STATE     (0x0015)
#define WHI_SET_RECORD_STATE     (0x0016)
#define WHI_READ_RECORD_STATE    (0x0017)

#define WHI_READ_TOPOLOGY_NODE_SUM     (0x0020)
#define WHI_READ_TOPOLOGY_NODE_INFO    (0x0021)

#define WHI_CFG_ISP_KEY_CMD     	(0x3001)
#define WHI_READ_ISP_KEY_CMD     	(0x3002)
#define WHI_CFG_PROJ_KEY_CMD     	(0x3003)
#define WHI_READ_PROJ_KEY_CMD     	(0x3004)
#define WHI_CFG_NET_ENABLE_FLG_CMD     	(0x3011)
#define WHI_READ_NET_ENABLE_FLG_CMD    	(0x3012)

#define WHI_NET_START_BIND_NODE_CMD     (0x3081)
#define WHI_NET_STOP_BIND_NODE_CMD     	(0x3082)
#define WHI_NET_QUERY_BIND_NODE_CMD    	(0x3083)
//
#define WHI_REPORT_IN_NET_NODE_LIST    (0x2018)	//上报注册节点
#define WHI_REPORT_LEAVE_NET_NODE_LIST    (0x2019)	//上报注册节点
#define WHI_READ_DATE_TIME_INFO         (0x8008)	//向主模块获取日期时间

// 参数配置
#define WHI_LAMP_BLE_WRITE_POWER_VALUE     			(0xC000)
#define WHI_LAMP_BLE_READ_POWER_VALUE     			(0xC001)
#define WHI_LAMP_RADAR_WRITE_PARAM_VALUE     		(0xC004)
#define WHI_LAMP_RADAR_READ_PARAM_VALUE     		(0xC005)
#define CMD_SET_LINKAGE_PARAM     			(0xC006)
#define CMD_READ_LINKAGE_PARAM     			(0xC007)


#define WHI_LAMP_RADAR_EVENT_REPORT_VALUE     		(0xC010)
#define WHI_LAMP_SENSOR_EVENT_REPORT_VALUE     			(0xC011)
#define WHI_LAMP_SENSOR_EVENT_SEND_VALUE     			(0xC012)

#define WHI_LAMP_BLE_NET_CTRL_FRAME     			(0xC020)

//
#define WHI_READ_FREQ_BAND        (0xF001)
#define WHI_SET_FREQ_BAND         (0xF002)


//
#define WHI_SEND_HPLC_FRAME            (0x0100)
#define WHI_RECEIVE_HPLC_FRAME         (0x0101)

#define WHI_SEND_STA_FRAME             (0x0110)
#define WHI_RECEIVE_STA_FRAME          (0x0111)

#define WHI_SEND_STA_NO_ACK_FRAME        (0x0102)
#define WHI_RECEIVE_STA_NO_ACK_FRAME     (0x0103)
#define WHI_SEND_BUS_TRAN_FRAME          (0x0150)
#define WHI_PLC_DIAGNOSIS     (0xFF01)
#define WHI_HPLC_ADAPT_DIAGNOSIS_FRAME             (0xFF01)

#define WHI_SEND_BUS_FRAME             (0x0120)

#define WHI_READ_BLE_BREAKER_FRAME     (0x5500)
//
#define WHI_ACK_EOK					(0x00)
#define WHI_ACK_TRAN_TIME_OUT		(0x01)
#define WHI_ACK_FRAME_ERR			(0x02)
#define WHI_ACK_BUSY				(0x03)
#define WHI_ACK_OTHER				(0xFF)
//=====
//文件传输
#define WHI_START_TRAN_FILE         (0x01)
#define WHI_TRAN_FILE_DATA          (0x02)
#define WHI_SCAN_FILE_INFO          (0x03)
#define WHI_SET_UPDATA_NODE_LIST    (0x04)

#ifndef   __PACKED
  #define __PACKED                               __attribute__((packed))
#endif

typedef struct __PACKED
{
	uint8_t fun_cmd;
	uint8_t file_attr;
	uint16_t segment_total;
	uint32_t file_len;
	uint32_t file_crc;
	uint32_t trans_timeout;
}whi_file_updata_start_frame_type;

typedef struct __PACKED
{
	uint8_t fun_cmd;
	uint8_t rsv;
	uint16_t segment_num;
	uint16_t segment_size;
	uint16_t segment_crc;
	uint8_t segment_data[];
}whi_file_updata_file_segment_frame_type;

typedef struct
{
	//
	uint32_t uart_tx_task_cnt;
	uint32_t uart_tx_task_add_full_err_cnt;
	uint32_t uart_tx_task_add_malloc_err_cnt;
	uint32_t uart_tx_task_add_len_err_cnt;
	//
	uint32_t uart_rx_frame_cnt; //接收次数
	uint32_t uart_rx_frame_crc_error_cnt;//crc错误 
	uint32_t uart_rxe_mult_frame_cnt;//多帧
	uint32_t uart_rx_lost_frame_cnt;
	//
	uint32_t uart_tx_cnt; //接收次数
	uint32_t uart_tx_rs_cnt; //接收次数
	uint32_t uart_tx_brocast_cnt;
	uint32_t uart_tx_brocast_time_out_cnt; //接收次数
	//
	uint32_t receive_ble_net_frame;
	uint32_t radar_rx_frame_cnt;
	//
	uint32_t store_cnt_data_error_cnt;
	//
	uint32_t ble_updata_file_resend_cnt;
	//
	uint32_t bus_fram_cnt;
	uint32_t bus_fram_brocast_cnt;
	uint32_t bus_fram_local_brocast_cnt;
	uint32_t bus_fram_err_cnt;
	uint32_t bus_fram_send_err_cnt;
	uint32_t bus_fram_send_success_cnt;
	//
}local_debug_cnt_type;
extern local_debug_cnt_type local_debug_cnt;

//============
//发送任务优先级，越大，优先级越高
#define SEND_LOCAL_ACK_PRIO_VALUE               	(0x50) //立马应答
#define SEND_METER_ACK_DATA_PRIO_VALUE         		(0x30) //抄表应用数据
#define SEND_METER_ACK_STATE_PRIO_VALUE        		(0x30) //抄表应答抄表状态

#define SEND_LOCAL_READ_VERSION_PRIO_VALUE              (0x4A) //
#define SEND_LOCAL_READ_RESET_INFO_PRIO_VALUE              (0x4B) //
#define SEND_LOCAL_TOOL_TEST_SET_RSSI_PRIO_VALUE                 (0x49) //
#define SEND_LOCAL_TOOL_TEST_READ_RSSI_PRIO_VALUE                 (0x48) //
#define SEND_LOCAL_CFG_FRAME_PRIO_VALUE                 (0x45) //

#define SEND_METER_EVENT_PRIO_VALUE                 (0x40) //事件上报

#define SEND_REPORT_METER_LIST_PRIO_VALUE           (0x21) //搜表列表上报
#define SEND_REPORT_REGIST_METER_STATU_PRIO_VALUE   (0x20) //搜表状态上报

#define SEND_REPORT_CHECK_ZONE_METER_LIST_PRIO_VALUE	(0x20) //台区识别上报数据
#define SEND_QUERY_CAPINFO_ACK_DATA_PRIO_VALUE			(0x20) //读资产回复应答

#define SEND_CKQ_TRANS_PRIO_VALUE               		(0x20) //抄控器本地优先级

#define SEND_DEFAULT_PRIO_VALUE                 		(0x10) //默认优先级


//============
#define WHI_PRO_HEAD_FRONT_SIZE	(8)

#define SEND_LOCAL_ACK						(0x05)  //本帧本地应答

//============
//新增接口
typedef bool (*local_task_cb_fun_t)(void * exparam,u8* data,u16 len,u8 state);
void lc_net_hplc_tx_sta_frame(uint8_t *dst_mac,uint8_t *src_data,uint16_t src_len);
extern bool local_send_fram_add_task(uint8_t *data,uint16_t len,uint16_t pri,uint16_t t_time,uint16_t resend,local_task_cb_fun_t cb_fun,void *cb_fun_arg);

//
void whi_pro_init();
void whi_pro_task();
void Local_Task(void *arg);

uint16_t WHI_CreateLocalSeq();
/**
* @brief 全屋智能帧分派处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
#define LOCAL_PROCESS_MODE		(0) //本地处理模式
#define LOCAL_BLE_NET_PROCESS_MODE		(1) //ble蓝牙网络处理模式
uint16_t WHI_UartLocalWHIProcess(uint8_t *psrc,uint16_t len,uint8_t process_mode);
/**
* @brief 全屋智能检测帧格式处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传校难成功或失败
* @author hh
*/
bool WHI_IsLocalWHIFrame(uint8_t *pbuff,uint16_t len);



uint16_t WHI_BuildFrameProcess(uint8_t *pdst,bool ackflg,uint16_t cmd,uint16_t seq,uint8_t *pdsrc,uint16_t dlen);
//
bool whi_pro_local_add_uart_task(uint8_t *data,uint16_t len,uint16_t pri,uint16_t resend);
//#ifdef PLAT_LAMP
uint16_t WHI_BuildReportInNetNodeListProcess(uint8_t *send_buff,uint8_t *node_list,uint16_t node_sum,uint16_t frame_seq);
uint16_t WHI_BuildReportLeaveNetNodeListProcess(uint8_t *send_buff,uint8_t *node_list,uint16_t node_sum,uint16_t frame_seq);
//#endif

bool whi_pro_local_add_uart_task_cb_fun(void * exparam,u8* psrc,u16 len,u8 state);
void sensor_adv_illum_ctrl_send(void *data, uint8_t len);


//================
//重发广播帧管理
void ResetBroadcastSavedFrame();
bool SaveBroadcastFrame(uint32_t seq, uint16_t len, uint8_t *buff);
int GetBroadcastFrame(uint32_t seq);
int GetBetterNeighbor(uint16_t *tei,uint32_t seq);
void ResendBroadcastFrame(uint32_t seq);

#endif



#ifndef _LC_APP_H_
#define _LC_APP_H_
#include <stdint.h>
#include <gsmcu_hal.h>

//0x0104: 收到CCO写KEY操作时要匹配nid和判断入网状态
//0x0105: 收到BLE CCO主动发送的0x0120帧，ble_net_ack_send_data_port设为BLE_NET_ACK_SEND_BLE_UART_PORT
//0x0106: TimeOut改为LcTimeOut
//0x0107: 增加蓝牙升级载波代码
//0x0108: lcip_protocol_parsing里crc校验出错时，释放已分配内存
//0x0109: 修正站点与PCO通信成功率获取方法
//0x010A: 修复上行没入网能组D7问题
//0x0111: 修正站点与CCO通信成功率获取方法
//0x0112: MAC地址的初始化要在蓝牙初始化之前进行，否则每个放大器的NetId都是一样的
//0x0113: 增加载波/无线入网0x07事件上报代码
//0x0114: 增加物模型广播转发
//0x0115: 修改升级位图
//0x0116: 增加对时接口，否则BLE子节点时间不同步
//0x0117: 修正对时时间为北京时区的时间
//0x0118: 增加物模型读取接口;修改升级最后一包回复失败问题;打开事件任务;
//0x0119: 升级传输文件完成后等待5秒重启;
//0x0120: v0119的等待5秒会阻塞对模拟集中器的回复,改成采用定时器等待5秒钟后重启
//0x0121: 增加模拟集中器的调试接口
//0x0122: 先初始化key后初始化蓝牙参数
//0x0123: 通过超时手段恢复数据传输口的选定（根据连接情况选定）;增加应用洪泛广播功能,调试灯和模式配置物模型使用应用广播,注意:蓝牙程序版本需要到0x0290,不然无法使用实时控制
//0x0124: 写ex_param前先读flash，防止改写结构体中的其他变量
//0x0125：D7灯上报处理接口存在内存泄漏的可能
//0x0126：增加蓝牙版本比对，比对失败上报0x08版本异常事件，若串口不通上报0x0b连接失败事件
//0x0127：上报事件增加入网条件判断，载波未入网情况下产生的事件先入队，等待入网后上报。
//0x0128: 解决使用蓝牙连接修改MAC需要重启两次才生效
//0x0129: 添加搜表和抄表功能
//0x0130: 添加物模型写Key功能
//0x0131: 切换CCO入网时，上报入网事件
//0x0132: 添加邻居表，邻居数量，邻居tei物模型
//0x0133: 解决0129产生的bug，网络nid一样问题; 超过一个小时邻居数都为零，复位
//0x0134: 支持节点上报上线事件物模型0x8F03
//0x0135: 放大器上报底下电表入网事件
//0x0136: 放大器增加查询底下搜表结果物模型(SIID:0x03EA,CIID:0x3EA2)
//0x0137: 放大器搜表结束入网后，会主动上报搜表结果
//0x0138: 更新门海最新SDK
//0X0139: 能透传空调控制器报文
//0x0140: 增加差分升级功能
//0x0141: 增加蓝牙断路器的配置和抄读功能（配合D8蓝牙v0300）
//0x0142: 修复v0141版载波串口不能连续发包给蓝牙的bug
//0x0143: 修复蓝牙升级出错问题
//0x0144: 修改抄表任务处理，避免低波特率抄多项时失败，修改串口任务处理，避免内存重复释放
//0x0145: 解决启动绑定后未能给D7设置key问题，解决物模型8F03,6F03下发时，非需要上报的节点回复的bug
//0x0146: 解决读取蓝牙设备类型错误问题,解决自身BLE升级问题，解决大端不上报问题,启动绑定key被改后同步给载波
//0x0147: 解决读取蓝牙版本失败的后续错误处理方式问题
//0x0148: 提供蓝牙作为STA的功能，提供SIID:03EC物模型
//0x0149: 前30s可以进行485线帧交互（115200），搜表收到回复多试2次，支持非BCD
//0x0150: 提供手动设置电表物模型功能，抄表和透传时串口奇偶检验位不再固定为偶校验，根据电表实际更改
//0x0151: 修改产测帧回复地址为全A
//0x0152: 搜表时，同时使用抄表帧和搜表帧进行搜表
//0x0153: 添加成品工装测试功能
//0x0154: 解决读取蓝牙RSSI延迟问题
//0x0155: 临时版本，只支持00A1
//0x0156: 提供入网限制名单物模型，默认白名单存在00A1FFFFFFFF，设置后默认失效
//0x0157: 白名单入网规则修改，与蓝牙串口处理流程修改；增加物模型读取简化版的读取网络基础信息;LC的代码里要用LcTimeOut替代TimeOut函数
//0x0159: 解决key同步问题,解决黑名单初始化问题
//0x0160: 不再代报底下入网蓝牙节点的入网上线事件
//0x0161: 删除二采应用部分代码，提供查询邻居表功能，查询父节点信息
//0x0162: 解决161版本出现问题，提供窗帘控制物模型
//0x0163: 修改窗帘控制物模型
//0x0164: 修改读取邻居表错误
//0x0165: 修改读取PCO信息错误
//0x0166: 对底层收到同步key报文和测试报文进行CRC检验，同步key报文给与回应,黑名单白名单存储进行crc校验（兼容前面版本）
//0x0167: 添加载波RF广播报文测试，添加key开关功能,默认放大器蓝牙STA功能关闭
//0x0168: 修复蓝牙网络节点上报数据有概率失败问题
//0x0169: 添加读取异常复位日志功能，添加异常复位日志功能
// 软件版本号 BCD码 各位最大值9
#define LC_APP_VER  0x0169

#ifndef NULL
#define NULL	0
#endif

//数据收发
#define LC_NET_NOT_BLOCK		(0x00)
#define LC_NET_BLOCK			(0x01)

/**
* @brief 网络错误类型
*/
typedef uint32_t lc_net_err_t;

#define LC_NET_EOK                          0               /**< 无错误 */
#define LC_NET_ERROR                        1               /**< 一般错误 */
#define LC_NET_ETIMEOUT                     2               /**< 超时错误 */
#define LC_NET_EFULL                        3               /**< 资源已满 */
#define LC_NET_EEMPTY                       4               /**< 资源已空  */
#define LC_NET_ENOMEM                       5               /**< 内存不足 */
#define LC_NET_ENOSYS                       6               /**< 无系统  */
#define LC_NET_EBUSY                        7               /**< 忙线中  */
#define LC_NET_EIO                          8               /**< IO 错误  */
#define LC_NET_EINTR                        9               /**<  中断系统调用  */
#define LC_NET_EINVAL                       10              /**< 无效参数  */


#define LEN_APP_PACKET 12
#define LONG_ADDR_SIZE  6


#define STAII_MAX_RF_POWER 15
#define STAII_MAX_PLC_POWER 13


#define LC_645_CMD  0x00
#define LC_645_FT   0x16
#define LC_645_CS_POSITION 10
#define LC_645_BASE_LEN 12
#define LC_645_ID 10
//抄表中
#define TASK_RUN 1
//空闲
#define TASK_IDLE 0

#pragma pack(1)
typedef struct
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
}lcFrame645;

typedef struct
{
    uint8_t task_status;
	uint32_t meter_baudrate;
	uint8_t meter_parity;
	uint8_t mac[6];
	uint16_t plc_pro;
	uint16_t seq_num;
	uint8_t fun_code;
	uint8_t* src_data;
	uint16_t data_len;
}operate_meter_task_param_t;

typedef struct
{
	uint8_t sendtype;
	uint8_t fre;
	uint8_t dst_mac[6];
	uint8_t src_mac[6];
}msg_test_plc_t;

typedef struct
{
	uint8_t sendtype;
	uint8_t channel;
	uint8_t option;
	uint8_t dst_mac[6];
	uint8_t src_mac[6];
}msg_test_rf_t;

#pragma pack()
void InitOperateMeterTask();
bool AddOperateMeterTask(operate_meter_task_param_t *param);
bool DeleteOperateMeterTask(u8 taskindex);
void OperateMeterTask(void *arg);

void LcAppHandleData(u8 *data, u16 len);
bool lc_IsHave645Frame(u8*data,u16 len,u16 *hindex);
u16 lc_protocol645_dispose_IFR(u8 *pframe);

void LcAppRebootTimerCallback(void *arg);
bool LcRfPowerCheckAndSet(u8 *data);
bool LcPlcPowerCheckAndSet(u8 *data);
lc_net_err_t lc_net_tx_data(const uint8_t* to_mac,const void *msg, const uint32_t len,const uint32_t flg);
lc_net_err_t lc_net_tx_data_to_tei_broadcast(const uint16_t to_tei,const void *msg, const uint32_t len,const uint32_t flg);
#endif
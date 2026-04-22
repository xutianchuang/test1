#ifndef __BLE_DATA_H__
#define __BLE_DATA_H__
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "cmsis_compiler.h"
#include "ble_api.h"

#ifndef __VOLATILE
#define __VOLATILE
#endif

//#define REGIST_NET_REPORT_USING	
#define BLE_NODE_IN_LEAVE_NET_REPORT_USING

//#define BLE_NODE_IN_NET_REPORT_ENABLE


#define BLE_LONG_ADDR_SIZE		(6)
#define BLE_SHORT_ADDR_SIZE		(2)
#define BLE_MAX_NODE_MAC_ADDR_NUM	(302)

//整个档案状态
#define PARAM_MAC_CLEARED_LIST_STATUS    (0x11)
#define PARAM_MAC_CHANGE_PARAM_STATUS    (0x22)
#define PARAM_MAC_POWERUP_LIST_STATUS    (0x33)
#define PARAM_MAC_SYNC_COMPLETE_LIST_STATUS    (0x44)


//档案地址属性
#define BLE_MAC_ADDR_NORMAL_ATTR                   (0x01)//添加的档案地址
#define BLE_MAC_ADDR_FINDED_ATTR                  (0x02)//搜表到的档案地址
#define BLE_MAC_ADDR_NEED_DELETE_ATTR            (0x03)//需要删除的档案地址
#define BLE_MAC_ADDR_RELAY_ATTR                   (0x04)//添加的档案地址


#define BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE	(0xFFFF)

#define BLE_STORE_HEAD_FLG                         (0x11223344)


typedef struct
{
    uint32_t Head;
    uint32_t DataIdx;
    uint32_t DataLen;
    uint32_t Crc32;
}flash_store_head_st;

typedef struct
{
     uint16_t NeightIndex;                                //地址分配的TEI
     __VOLATILE uint16_t MacAttr:8;                   //档案地址状态
     __VOLATILE uint16_t MacProt:8;                   //档案协议
     uint8_t MacAddr[BLE_LONG_ADDR_SIZE];                 //MAC
     uint8_t LocalMacAddr[BLE_LONG_ADDR_SIZE];
}ble_mac_addr_type;

#define MAX_METERS_NUM	20
typedef struct __PACKED
{
	uint8_t mac[LC_NET_ADDR_SIZE];
	uint8_t baudrate;
	uint8_t parity;
}meter_addr_t;

//当前结构体大小：5042，总存储大小为0x2000-0x100
typedef struct 
{
    uint16_t SystemParamsStatus;
    
    uint16_t DirtySystemParamFlg;
    uint16_t HardRstCnt;
    uint16_t SofeRstCnt;
    uint16_t NetSeq;
	uint16_t NetId;
	uint8_t NetEnableFlag;
	uint8_t PlcBleMode;
    //
    uint16_t MacAddrSum;                     //目前档案总数
    uint16_t MacAddrNormalSum;               //下发的档案总数
    uint16_t MacAddrRelaySum;               //下发的中继档案总数
    uint16_t MacAddrFindedSum;               //搜表档案总数
    uint16_t MacAddrNeedDelSum;              //临时在网档案，需要删除
	//
	uint16_t NetNodeSyncFlg;
    ble_mac_addr_type MacAddrList[BLE_MAX_NODE_MAC_ADDR_NUM];    //档案列表
    
    uint8_t SystemLongAdd[BLE_LONG_ADDR_SIZE];
    //
    uint32_t Crc;

    // ST_MODE_ID_INFO_TYPE keyInfo;

    //蓝牙功率
    uint16_t ble_power;
    // 事件上报等待时间
    uint32_t lamp_report_waite_time;
    // 事件使能开关
    uint32_t event_en_mask;
	//BLE_STA开关使能，如果打开将支持BLE_STA功能
	uint32_t ble_sta_mode;
	//物模型设置的电表
	meter_addr_t meters_addr[MAX_METERS_NUM];
	uint8_t key_switch ;
}ble_system_params_type;
//===
extern ble_system_params_type ble_system_params;
//插入表地址到档案列表中
int ble_insert_mac_addr_to_list(const uint8_t *macaddr,const uint8_t prot,uint8_t attr);
//从地址列表中删除表地址,flg=true强制删除，false正常删除
int ble_delete_mac_addr_from_list(const uint8_t *macaddr,uint8_t flg);
//-1,不在档案内，其它在档案内
int ble_check_mac_addr_from_list(const uint8_t *macaddr);
bool ble_system_info_stored();

void ble_system_info_reset_init();
bool ble_system_info_syn_process();

bool ble_set_system_net_id(uint32_t net_id);
//===========
#define BLE_CCO_TEI				(1)
#define BLE_MAX_NEIGHBOR_NUM 	(BLE_MAX_NODE_MAC_ADDR_NUM+2)
#define BLE_MIN_TEI_VALUE 		(1)
#define BLE_MAX_TEI_VALUE		(BLE_MAX_NEIGHBOR_NUM-1)

#define BLE_NEIGHT_RES_PCO_LIST_LEN	(4)
//
//单个邻居节点信息如下
#define NEIGHT_NOTHAVE_FLG_VALUE           (0x00)  //邻居未分配
#define NEIGHT_HAVE_FLG_VALUE           (0x11)  //邻居已分配

typedef struct
{
    __VOLATILE uint16_t PcoTei:12;
    __VOLATILE uint16_t PcoTeiProid:4;
}ble_res_pco_neight_type;
//网络结构体
typedef struct
{
    //此结构体的内容轻易不要调整，调整需要注意调用相应结构体成员的代码
    //单个邻居节点信息如下, 内部成员必须2字节对齐

    //如果此结构体定义的变量定义在内存中，则联合体的成员可以直接访问
    //如果此结构体定义的变量定义在LC830的DSP RAM中，则联合体的成员不能直接访问，
    //此时可以通过访问data16成员。

    uint32_t brush_neighbor_tick;            //发送信标时的tick值，保证170秒内发送最少两个信标
    //===============
    uint16_t NeighborPcoTEI;             //自已父节点TEI
    uint16_t DirectChilTei;      //本节点作为父节点，第一个子点TEI
    uint16_t BrotherNextTei;     //兄弟下节点TEI,或采集器下挂485表；
    uint16_t Meter485ListNextTei;  //II采或I采下边挂的表，相同pco
    //=======
	
    uint16_t ResPcoTeiList[BLE_NEIGHT_RES_PCO_LIST_LEN];//备选pcotei列表
    uint16_t ResDirtChilSum;//备直连子站点总数，释放tei时将要清空计数
    
    __VOLATILE uint16_t NetStatePeriodCount:8;
    __VOLATILE uint16_t ResPcoTeiListIdx:8;
    //============================================
    __VOLATILE uint16_t NeightState:8;                //0未使用,0x1111已使用；
    __VOLATILE uint16_t Layer:5;                      //层级
	__VOLATILE uint16_t NeedNetReport:1;			//上报使能
	__VOLATILE uint16_t NetReportFlg:1;				//1已上报，0未上报完成
    __VOLATILE uint16_t CollectMeterListFlg:1;        //标记I采或II采是否已采集子节点
    
	
    __VOLATILE uint16_t pro_check_flg:1;      //角色   EN_ROLE_TYPE
    __VOLATILE uint16_t Role:3;      //角色   EN_ROLE_TYPE
    __VOLATILE uint16_t Phase:4;     //相线
    //uint16_t Layer:5;     //层级
    __VOLATILE uint16_t DeviceType:4;          //设备类型
    __VOLATILE uint16_t NetState:4;           //网络状态,离线，离网；

    //__VOLATILE uint16_t UpdataState:8;
    __VOLATILE uint32_t StayInNetPrioCnt; //在网周期统计

    __VOLATILE uint32_t FromCcoAplCnt;
    __VOLATILE uint32_t ToCcoAplCnt;
    //===================
    //上1路由周期统计计数    
    __VOLATILE uint16_t HeartFrameHeartCnt:8;          //每个周期心跳帧上报
    __VOLATILE uint16_t NeighborHeartCnt:8;         //每个周期邻居心跳
    __VOLATILE uint16_t OtherHeartCnt:8;       //每个周期心跳
    __VOLATILE uint16_t AssocReqCnt:8;       //关联请求数
    //当前周期内接收
    __VOLATILE uint16_t CurrentRxBeaconCnt:8;          //当前接收到的信标数
    __VOLATILE uint16_t BindProjKeyCnt:8;         //当前接收到的发现列表数
    __VOLATILE uint16_t SNR:8;                 //信噪比    
    __VOLATILE uint16_t RSSI:8;                //信号强度
    //==================================
    __VOLATILE uint8_t MacAddr[BLE_LONG_ADDR_SIZE];  //MAC
}ble_neighbor_info_st_type;

extern ble_neighbor_info_st_type ble_neighbor_list[BLE_MAX_NEIGHBOR_NUM];
//=============================================
#define BLE_MAX_LAYER_VALUE			(15)
#define BLE_NET_MAX_PCO_SUM	(120)

//网络节点状态
enum
{
    BLE_NET_OUT_NET,            //未入网
    BLE_NET_SELECT_NET,            //选定网络
    BLE_NET_JOINING_NET,        //正在加入网络
    BLE_NET_QUIT_SELECT_NET,    //退出当前选择网络
    BLE_NET_IN_NET,                //已入网
    BLE_NET_OFFLINE_NET,        //离线
    
    BLE_NET_OFFLINE_NET_INC,        //离线指示
    
    BLE_NET_STATE_LIMIT_VALUE,
};
//网络基本参数
typedef struct
{
    uint32_t LocalMaxNeightSum;
    uint32_t NeighMaxNeightSum;
    uint32_t NeighMaxNeightTei;

    uint32_t HplcNetLoadRate;

    uint32_t HplcPcoSum;//限制入网，入网pc数已达上限
    uint32_t HpclInNetSum;//入网总数
    uint32_t HpclStayNetSum;//在网计数
    uint32_t HpclNetState;
    uint32_t HplcNetPcoMaxLayer;
    //
    uint32_t HplcInNetSumLayer[BLE_MAX_LAYER_VALUE];//各层级入网数
    uint32_t HplcPcoInNetSumLayer[BLE_MAX_LAYER_VALUE];//各层级入网数
    uint32_t HplcStayNetSumLayer[BLE_MAX_LAYER_VALUE];//各层级在网数
    uint32_t HplcPcoStayNetSumLayer[BLE_MAX_LAYER_VALUE];//各层级在网数
    //
    uint32_t HplcBuildNetTimeMs;
    //
    uint32_t HplcBuildNetStartTicks;
    uint32_t HPlcBuildNetLastStayNetTicks;
    uint32_t HPlcBuildNetLastInNetTicks;

    //网络运行模式
    uint32_t HplcNetRunMode;
    uint32_t HplcNetRunModeParam;
    uint32_t HplcSetNetRunModeFlg;
    uint32_t HplcStayNetSumModeTmpCnt;
    //
    uint32_t HplcMaxTei;
    //
    uint16_t HplcBeaconPerioProcessCnt;
    uint16_t HplcBeaconPerioAssoccCnt;
    //
    uint16_t  TieCSMAPhaseNum;        //绑定CSMA时隙支持的相线个数
    uint16_t  TieCSMALid;             //绑定CSMA时隙支持的业务报文
    uint16_t  TieCSMAPrioCnt;          //绑定CSMA 周期
	
    uint16_t  TDMASlotLen;            //TDMA时隙长度,100us
    uint16_t  TDMALid;                //TDMA报文
    uint16_t  TDMAPrioCnt;            //TDMA报文
	//
    uint32_t NetId;       //网络id
    uint8_t NetEnableFlag;     //组网标志,0组网中，1组网完成
    uint8_t NetModeFlg;  //0表示CCO，1表示STA
    uint8_t NetPrior;    //多网络优选开关
    uint8_t RelateFlag;  //关联标志位(0:不允许站点发送关联请求,1:允许)
    uint8_t NetSeq;      //网络系列号
    uint8_t NetEventSwitchFlg;//事件上报开关
    uint8_t NetDateBeaconFlg;
    uint8_t NetFreq;          //网络使用频段
    uint8_t NetPwr;          //网络使用功率
	
    uint8_t NetNodeSyncFlg;          //同步标记
    //
	uint8_t NetBindFlg;
    uint32_t NetRoutePriod;   //网络路由周期
//    uint32_t BeaconPriodCnt;  //信标周期记数
    uint8_t CCO_MAC[BLE_LONG_ADDR_SIZE];
    //
    uint8_t SofeRstCnt;
    uint8_t HardRstCnt;
	//
	uint32_t IspKey;
	uint32_t ProjKey;
    //
}ble_net_info_st_type;

extern ble_net_info_st_type ble_net_info;

typedef struct
{
	uint8_t net_statu;//网络状态
	uint8_t net_seq;
	uint8_t net_enable_flg;
	uint8_t net_mode;
	
	uint32_t net_id;
	uint8_t cco_addr[BLE_LONG_ADDR_SIZE]; 
	uint8_t pco_addr[BLE_LONG_ADDR_SIZE]; 

	uint32_t isp_key;
	uint32_t proj_key;
}ble_sta_net_info_st_type;
extern ble_sta_net_info_st_type ble_sta_net_info;
//tei入网一个pco tei
//0成功，非0失败；
//mofe 0:正常
//mofe 1:刷新入网;
#define INTO_NET_REQ_SUCCESS         (0x00)      //成功
#define INTO_NET_NOT_IN_LIST_FAIL       (0x01)      //不在白名单中
#define INTO_NET_IN_NLIST_FAIL          (0x02)      //在黑名单中
#define INTO_NET_B_MAXLISTSUM_FAIL      (0x03)      //节点数超过门限
#define INTO_NET_NOTHING_LIST_FAIL      (0x04)      //无设置白名单
#define INTO_NET_MAX_PCOS_FAIL           (0x05)      //代理超门限
#define INTO_NET_MAX_CHILDS_FAIL         (0x06)      //子站点数超门限
#define INTO_NET_RES_FAIL               (0x07)      //保留
#define INTO_NET_MAC_SAME_FAIL         (0x08)      //mac地址重复
#define INTO_NET_MAX_LEVEL_FAIL         (0x09)      //超过拓扑层级
#define INTO_NET_REREQ_SUCCESS         (0x0a)      //再次关联成功
#define INTO_NET_REQ_CHILD_FAIL         (0x0b)      //试图关联子站点入网
#define INTO_NET_RING_FAIL              (0x0c)      //拓扑存在环路
#define INTO_NET_CCO_UNKNOW_FAIL         (0x0d)      //CCO未知错误

#define INTO_NET_TEI_RANG_ERR         (0x40)      //tei范围错误
#define INTO_NET_PCO_RANG_ERR         (0x41)      //tei范围错误
#define INTO_NET_PCO_NOT_IN_NET_ERR         (0x42)      //所选pco不在网错误

//节点设备类型
enum
{
//     UNKNOWN_DEVICE_TYPE,
//     CONTROLLER = 1,
//     CONCENTRATOR,
    SINGLE_METER = 3,
//     RELAY,
//     II_COLLECTOR,
//     I_COLLECTOR,
    THREE_METER = 7,
    METER_485 = 8,
 };

//节点网络类型
// enum
// {
//     UNKNOWN_ROLE = 0,
//     STA_ROLE,
//     PCO_ROLE,
//     RESERVED_ROLE,
//     CCO_ROLE,
//     ROLE_LIMIT_VALUE,
// };

//=================
uint16_t ble_malloc_tei_from_mac_addr(uint8_t *macaddr);
uint16_t ble_into_pco_tei(uint16_t mode,uint16_t tei,uint16_t pcotei,uint16_t dtype,uint16_t phaseline);//加入pco,tei为sta或是pco;新加入或变更代理
//配置tei节点离线状态
bool ble_set_tei_out_net_stay_state(uint16_t tei);
//配置节点从离线到入网
bool ble_set_tei_in_net_stay_state(uint16_t tei);
//================================
//白名单管理
//白名单添加地址
bool ble_data_add_addrs(const uint8_t *p_addrs,const uint8_t count);
//从白名单中删除通信地址
void ble_data_del_addrs(const uint8_t *p_addrs,const uint8_t count);
//清空白名单通信地址
void ble_data_clear_addrs();
//获取白名单中的通信地址总数
uint32_t ble_data_get_count();
//获取白名单中的通信地址
uint32_t ble_data_get_addrs(uint8_t *p_addrs,const uint32_t offset,const uint32_t count);
//获取mac地址到知地址
uint16_t ble_get_mac_to_tei(uint8_t *macaddr);
//通过tei获取macaddr地址；
bool ble_get_net_tei_mac_addr(uint16_t tei,uint8_t *macaddr);
//================================
//拓扑
//拓扑节点网络信息结构类型
typedef struct __PACKED
{
	uint8_t mac[BLE_LONG_ADDR_SIZE]; //节点mac地址
	uint16_t tei;//本节点TEI  1-1000；0：未入网无TEI
	uint16_t pco_tei;//父节点TEI：CCO父节点为0;
	uint8_t level:4;//0-15 层级
	uint8_t  role:4;//sta pco cco 
	uint8_t  add_check_flg:1;//sta pco cco 
	uint8_t  red:7;//sta pco cco 
}ble_data_topo_node_info_t;
//获取网络拓扑节点总数
uint32_t ble_data_get_topo_node_count();
//获取网络拓扑节点
uint32_t ble_data_get_topo_nodes(ble_data_topo_node_info_t *p_nodes,const uint32_t offset, const uint32_t count);

//================================
//邻居
//拓扑节点网络信息结构类型
typedef struct __PACKED
{
	uint8_t mac[BLE_LONG_ADDR_SIZE]; //节点mac地址
	uint8_t rssi;//0-15 层级
	uint8_t red;//sta pco cco 
}ble_data_neighbor_node_info_t;

//PLC邻居
//拓扑节点网络信息结构类型
typedef struct __PACKED
{
	uint8_t mac[BLE_LONG_ADDR_SIZE]; //节点mac地址
	uint8_t rssi;//0-15 层级
	uint8_t red;//sta pco cco 
	uint8_t up_succ_rate;//上行通信成功率
	uint8_t down_succ_rate;//下行通信成功率
}plc_data_neighbor_node_info_t;


uint32_t ble_data_get_neighbor_node_count();
uint32_t ble_data_get_neighbor_node_info(ble_data_neighbor_node_info_t *p_nodes,const uint32_t offset, const uint32_t count);


void ble_local_neighbor_list_init();
bool ble_local_set_neighbor_info(uint8_t *mac_addr,uint8_t rssi);
void ble_local_neighbor_list_task();
uint32_t ble_local_neighbor_get_count();
uint32_t ble_local_get_neighbor_node_info(ble_data_neighbor_node_info_t *p_nodes,const uint32_t offset, const uint32_t count);
//=============
//更新邻居场强
void ble_net_set_neighbor(uint8_t *macaddr,uint8_t rssi);
//=========
//通过macaddr地址tei获取；0为未入网，其它为正常
uint16_t ble_get_mac_addr_to_net_tei(uint8_t *macaddr);
//获取到达指定tei的路径，目<-源 返回TEI数量，0为非入网；
uint16_t ble_get_to_tei_route_path(uint16_t to_tei,uint16_t *path_list);
//获取到达指定tei的下一跳路由tei,0失败；其它为nexttei
uint16_t ble_get_to_tei_route(uint16_t tei);
//==================
//网络状态刷新,0heart,1neighbor,0xFF,other
void ble_brush_neighbor_heart(uint16_t tei,uint16_t mode);
void ble_neighbor_heart_period_process(uint16_t addr_idx,uint16_t ngh_idx);

//============================================
//离线指示
//检测发送离线指示
uint16_t ble_get_leave_net_addr_process(uint8_t *maclist,uint16_t maxaddrlen,uint16_t *plidx);
uint16_t ble_get_leave_net_tei_process(uint8_t *teilist,uint16_t teiaddrlen,uint16_t *plidx);

bool ble_free_tei_and_mac_addr(uint8_t *macaddr,uint16_t tei);
//=====
//周期处理函数
void ble_neighbor_list_period_process();
//
void ble_data_reset_net_data();

//对外获取参数接口
uint32_t ble_get_report_wait_time();
uint32_t ble_get_report_event_en_mask();
uint32_t ble_get_report_wait_time();
uint32_t ble_get_report_event_en_mask();
bool ble_put_report_wait_time(uint32_t waite_time);
bool ble_put_report_event_en_mask(uint32_t en_mask);
bool ble_change_ble_send_power(uint32_t ble_power);
uint16_t ble_get_ble_send_power();
#endif

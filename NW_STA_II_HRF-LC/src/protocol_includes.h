#ifndef _PROTOCOL_INCLUDES_H_
#define _PROTOCOL_INCLUDES_H_
//#define NW_TEST     //南网电科院送检必须打开
//#define NW_NEW_TEST //南网新机台测试
#ifdef NW_NEW_TEST
#define NW_TEST       //南网新机台测试,同步开NW_TEST宏
//#define FILTER_MSDU_HEAD
#define ASSOCIND_NOT_FFF
#endif

#include <gsmcu_hal.h>
#include  "app_cfg.h"
#include "phy_port.h"


#include "init.h"
#include "hplc_mac_def.h"
#include "hplc_app_def.h"
#include "hplc_frame.h"
#include "hplc_protocol_init.h"
#include "hplc_data.h"
#include "applocal.h"
#include "local_protocol.h"
#include "hplc_app.h"
#include "hplc_phy.h"
#include "hrf_phy.h"
#include "hplc_mpdu.h"
#include "hplc_mac.h"
#include "hplc_task.h"
#include "hplc_beacon.h"
#include "hplc_route_period.h"
#include "hplc_timer.h"
#include "hplc_send.h"
#include "hplc_offline.h"
#include "hplc_channel_status.h"


#include "hplc_update.h"
#include "hplc_test_model.h"
#include "hplc_TMI.h"
#include "hplc_NID.h"
#include "hplc_zone.h"
#include "hplc_zc_manager.h"
#include "os_cfg_app.h"
#define ZERO_DIFF_TO_NORMAL     (-15479)
#define DEBUG_SWITCH_LEVEL   2
#ifndef __DEBUG_MODE
#define LED_REDUCE    //减少led闪烁
#endif
//#define DIS_FUNC
#define ASSOCIND_NOT_FFF 
#define SNR_MAX_CNT 8
//#define HRF_SEND_HIGH_1P5   //发送时拉高1.5V

extern u8 HPLC_ChlPower[];
extern const u8 HPLC_TestChlPower[];
extern const u8 HPLC_PowerOffPower[];
extern const u8 HPLC_HighPower[];
extern const u8 HPLC_LowPower[];
extern const u8 MAX_HPLC_POWER[];
extern u8 joined;
extern int read_meter_time;
extern const u32 myDevicType;

extern u8 HRF_ChlPower[];
extern const u8 HRF_HighPower[];
extern const u8 HRF_LowPower[];
extern const u8 HRF_TestChlPower[];
extern const u8 HRF_PowerOffPower[];
//#define HPLC_DEBUG_TRACE
//#define GUIZHOU_SJ
//#define GUANGXI_CSG //广西计量适配中电华瑞机台
//#define SHENZHEN_CSG
//#define YUNNAN_CSG
//#define CSG_201905 //2019年5月版规约
//#define GUANGDONG_CSG //广东深化应用V2.1
//#define GUANGDONG_CSG_23_1 //广东23年1批
//#define DONGGUAN_CSG //东莞，版本日期修改为231030，兼容现场230901和211101版本混合，需要升级规避Flash问题
#define TEST_COM_MODE_USE_MAC   1
#define PROTOCOL_NW_2020    //2020年深化应用和南网21标准(曲线配置和抄读兼容老规范)
#define PROTOCOL_NW_2021    //南网21标准
#define GOTO_TEST_LIKE_GW   //像国网一样在一个频段等待测试帧
//#define FIXED_DEFAULT_VER_INFO //固定版本信息
//#define FORCE_TX_POWER_HIGH //强制发射功率高模式
//#define CHANGE_FREQ_RETRENC  //根据精简信标修改自身HPLC

#define PHASE3_USE_ALL_PHASE    //三相使用全相线

//#define USE_NW_FACTORY_CODE     //使用"NW"厂商代码，给I采表陪测时使用

#define AUTO_CHANGE_POWER
#ifdef NW_TEST
#undef AUTO_CHANGE_POWER
//南网测试等待时间超过15min
#define CHECK_FREQ_NO_REBOOT
#define LOW_POWER
#endif
#ifdef HRF_SEND_HIGH_1P5
#define HRF_WAIT_CPU_NTB        2250
#else
#define HRF_WAIT_CPU_NTB        1000
#endif

#ifdef DONGGUAN_CSG //东莞宏打开广东宏
#define GUANGDONG_CSG
#endif

#ifdef GUIZHOU_SJ
#define GOTO_TEST_LIKE_GW
#endif

#if defined(GUANGDONG_CSG) || defined(YUNNAN_CSG)
#define FIXED_DEFAULT_VER_INFO //固定版本信息
#endif

#ifdef SHENZHEN_CSG
#define PROTOCOL_NW_EXT_GRAPH //南网深化应用曲线扩展（目前是负荷曲线零线电流采集），默认关闭
//#define TX_POWER_AUTO_HIGH_28 //发射功率自动调整模式，高发射功率调整为28
//#define FIXED_DEFAULT_VER_INFO //固定版本信息
#endif

//#ifdef GUANGXI_CSG
//#define ASSETSCODE_32BYTES_INFORM //资产编码32字节告知（暂不启用，广西23年1批及以后统一规定24位告知）
//#define FIXED_DEFAULT_VER_INFO //固定版本信息
//#endif

#define SUPPORT_21_METER
#define SUPPORT_21_METER_NO_CHANGE_RATE
//#define CHECK_LAST_FREQ
//#define CHECK_LAST_CCO //上一次入网CCO优先
//#define CHECK_LAST_CCO_TIMEOUT    15*1000UL

#define TX_POWER_CHANGE_SLOT_NUM 3 //基于信标中非中央信标个数来调整发射功率
                                   //检测到非中央信标时隙个数<TX_POWER_CHANGE_SLOT_NUM，切换到低功率HPLC_TestChlPower
                                   //检测到非中央信标时隙个数>=TX_POWER_CHANGE_SLOT_NUM时，切换到高功率HPLC_ChlPower

#define TX_POWER_CHANGE_SLOT_NUM_MAX 10  //检测到非中央信标时隙个数大于该宏，直接切换到高功率HPLC_ChlPower，不受20分钟限制（中慧）

//#define BOOST_LINE_PWR
#define II_STA
//#define I_STA

//#define NO_FILTER //针对给林洋的I采版本，不过滤

#ifdef I_STA  //I采宏在二采的基础上建立
#define II_STA
#endif

#define II_STA_ONLY645
#define BAUD_SEARCH 0
#define DEBUG_SEARCH_LOG_OUT //搜表log输出，不用输出log时可以注释掉

#ifdef HUIZHOU_CSG
#define II_STA_NO_9600_BAUD //不在9600波特率搜表, 针对科陆2016年电表，485波特率2400，在9600波特率搜表会响应
#define II_STA_SUPPORT_DLT645_97 //II采支持DLT645-1997协议搜表
#endif

#define RESET_DETECT_ONLY_LOW_LEVEL //规避广东清远台体（STA波特率自适应）,拉复位引脚复位STA不成功的问题
//#define CHECK_FREQ_NO_REBOOT //切频后没有找到相应网络成功入网，不复位
//#define CHECK_MAC_HEAD_VER //判断MAC帧头中版本号

//#define AREA_EXTEND_RESULT //采用扩展定义的台区识别结果，电科院定义暂用于广东现场台区调试试点

//#define OPERATION    //南网运维模块新建的TASK
#define FORCE_ZONE_RESULT //强制台区识别结果
//#define REPORT_OTHER_CCO  //涓婃姤鍏朵粬cco缁欐垜鐨刢oo
#define CHECKPOWEROFF_DELAY5S
#endif

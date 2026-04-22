#ifndef _HPLC_ROUTE_PERIOD_H_
#define _HPLC_ROUTE_PERIOD_H_
#include "common_includes.h"

//统一管理路由周期

//路由周期发生的事件
typedef enum
{
    ROUTE_COUNT,                  //路由周期计数
    ROUTE_SEND_HEART_BEAT,      //1/8个路由周期，发送心跳检测报文(最高层级的PCO)
    ROUTE_SEND_DISCOVER,        //1个路由周期，至少发送10次发现列表
    ROUTE_SEND_SUCCSEE_RATE,    //4个路由周期，发送通信成功率上报报文(PCO)
    ROUTE_SEND_PROXY_CHANGE,    //1个路由周期，评估代理变更
    ROUTE_SEND_HRF_DISCOVER,    //发送无线发送列表的周期
    ROUTE_HRF_OLD,    			//hrf的老化周期

    ROUTE_SEND_MAX_NUM,         //事件个数
}ROUTE_PERIOD_EVENT;

typedef struct
{
	int TmrID;      //定时器ID
	u32 ms;         //周期时间
}ROUTE_TMR_ID;

extern ROUTE_TMR_ID RouteTmrID[ROUTE_SEND_MAX_NUM];
extern bool unFinishedProxyChange;

//路由周期管理初始化
void RouteEventInit(void);

//更新路由周期参数
void UpdateRoutePeriod(u16 period);

//精简信标更新默认的路由周期
void RetrenchUpDefultRoutePeriod(void);

//路由周期校准
void AdjustRoutePeriod(u16 left);

//路由周期事件是否存在
bool IsRouteEventExist(ROUTE_PERIOD_EVENT);

//创建一个路由周期事件
bool CreateRouteEvent(ROUTE_PERIOD_EVENT);

//开始一个路由时间
bool StartRouteEvent(ROUTE_PERIOD_EVENT);

//停止一个路由周期事件
bool StopRouteEvent(ROUTE_PERIOD_EVENT);

//重置一个路由周期事件
bool ResetRouteEvent(ROUTE_PERIOD_EVENT,u32 period,TMR_RESET_OPT opt);

//删除一个路由周期事件
bool DeleteRouteEvent(ROUTE_PERIOD_EVENT);

//返回路由周期剩余时间
u32  RouteTimeLeft(void);

//返回路由周期序号
u32  RouteSequence(void);

//返回路由周期
u32 GetRoutePeriod(void);

//发送发现列表数递增
void IncreaseSendDisNum(u8 frameType);

//上个路由周期发送的发现列表个数
u16 LastSendDiscoverNum(void);

//当前路由周期发送的发现列表个数
u16 ThisSendDiscoverNum(void);

//停止所有路由周期事件
void StopAllRouteEvent(void);

//删除所有路由周期事件
void DeleteAllRouteEvent(void);
u8 SendHrfConflictPacket(void);
#endif

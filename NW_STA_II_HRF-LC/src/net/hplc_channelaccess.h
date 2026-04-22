#ifndef _HPLC_CHANNELACCESS_H_
#define _HPLC_CHANNELACCESS_H_


//触发VCS定时器启动的事件
typedef enum
{
    CHECK_CONFLICT,     //检测到冲突
    CHECK_CHARACTER,    //检测到前导符
    RECEIVE_SOF_CTL,    //解析到帧控制
    RECEIVE_CONFIRM,    //解析到选择确认
}VCS_EVENT;

//调用发送函数后返回的状态
typedef enum
{
    PLC_SEND_SUCCESS,   //发送成功，已经立即发送出去
    PLC_SEND_RANGE,     //能在指定的NTB发送，自动等到相应的NTB发送出去。
    PLC_SEND_QUEUE,     //进入发送队列，等待发送
    PLC_SEND_FAIL,      //不能在指定的NTB发送
}PLC_SEND_STATE;

//信道状态
typedef enum
{
    PLC_CHANNEL_IDLE,       //空闲
    PLC_CHANNEL_BUSY,       //繁忙
}PLC_CHANNEL_STATE;

//接收一个VCS定时器事件
void ReceiveVCSEvent(VCS_EVENT,u32 VCSTime);

//立即发送数据
PLC_SEND_STATE  ChannelSendImmediate(u8* data,u16 len);
//在某个NTB发送数据
PLC_SEND_STATE  ChannelSendAtNTB(u8* data,u16 len,u32 NTB);
//在某个NTB范围发送数据(vcs表示是否检查VCS定时器)
PLC_SEND_STATE  ChannelSendAtRange(u8* data,u16 len,u32 beginNTB,u32 endNTB,bool vcs);

//信道访问任务
void ChannelAccessTask(void* arg);
#endif

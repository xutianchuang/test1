#ifndef _HPLC_SEND_H_
#define _HPLC_SEND_H_
#define MAX_BITMAP_SIZE  256
#define CSG_DISCOVER_BITMAP_SIZE 128
#define CSG_BITMAP_SIZE    130     //心跳 代理变更确认的bitmapsize
#if defined(HPLC_CSG)

#define BEACON_SLOT_LEN_UNIT_US      100        //每个信标时隙占用的时隙长度单位
#define CSMA_SLOT_SPLIT_SIZE_UNIT_US 100        //CSMA时隙分片大小单位
#define CSMA_SLOT_LEN_UNIT_US        100        //CSMA时隙长度单位
#define TDMA_SLOT_LEN_UNIT_US        100        //TDMA时隙长度单位
#define BEACON_PERIOD_UNIT_US        100        //信标周期时间长度单位

#define BEACON_SLOT_LEN_MS  25      //15       //信标时隙长度，网间协调单位是4ms，最好能被4ms整除
#define BEACON_SLOT_LEN_US  25000UL //15000    //信标时隙长度，网间协调单位是4ms，最好能被4ms整除
#define BEACON_SPLIT_UNIT_MS          10
#else

#define BEACON_SLOT_LEN_UNIT_US      1000       //每个信标时隙占用的时隙长度单位
#define CSMA_SLOT_SPLIT_SIZE_UNIT_US 10000      //CSMA时隙分片大小单位
#define CSMA_SLOT_LEN_UNIT_US        1000       //CSMA时隙长度单位
#define TDMA_SLOT_LEN_UNIT_US        1000       //TDMA时隙长度单位
#define BEACON_PERIOD_UNIT_US        1000       //信标周期时间长度单位

#define BEACON_SLOT_LEN_MS  25       //信标时隙长度
#define BEACON_SLOT_LEN_US  25000UL  //信标时隙长度
#define BEACON_SPLIT_UNIT_MS          10
#endif

#define MAX_SEND_PDU    30
#define MAX_NID_NEIGHBOR_COUNT 20    //邻居网络数目

typedef enum
{
    CSMA_SEND_OK, 
    CSMA_SEND_FAILED,        
    CSMA_SEND_OVER_SLOT,  
	CSMA_SEND_NO_MEM,   
}CSMA_SEND_STATUS;

typedef void (*CSMA_CALL_BACK)(CSMA_SEND_STATUS resion,u8 link); 

typedef enum
{
    HPLC_LINK_LINSTEN,      //侦听
    HPLC_LINK_READY,        //侦听时间已过
    HPLC_LINK_WORKING,      //已发送信标
}HPLC_LINK_STATE;

typedef enum
{
    SLOT_TYPE_BEACON,
    SLOT_TYPE_CSMA,         //CSMA 指定相位才能发送
    SLOT_TYPE_TDMA,
    SLOT_TYPE_TIE_CSMA,
    SLOT_TYPE_CSMA_ANY,     //CSMA 任意时隙都可以发送
}SLOT_TYPE;

typedef enum
{
    CSMA_SEND_TYPE_CSMA,           //发送至CSMA
    CSMA_SEND_TYPE_BCSMA,          //发送至绑定CSMA
    CSMA_SEND_TYPE_DETECT,         //发送探测信标
    CSMA_SEND_TYPE_SYNC,           //发送同步信标
	CSMA_SEND_TYPE_COOR,           //发送网件协调
	CSMA_SEND_TYPE_RANGING,        //发送测距    
}CSMA_SEND_TYPE;
#pragma pack(4)
typedef struct
{
    u32  nid;
    u16  time_last_ms;
    u16  offset_ms;

    CPU_NTB_64      ntb_receive;        //最近接收到邻居网络网间协调帧的时间

    bool            can_listen_me;       //邻居网络是否侦听到本网络
    CPU_NTB_64      ntb_listen_me;      //最近接收到邻居网络网间协调帧中有本网络ID的时间
	//OS_TICK_64      first_hrf_conflict; //第一次无线冲突检测时间
	u8              channel;            //邻居网络信道
    u8              option;             //邻居网络option
}NET_NEIGHBOR, *P_NET_NEIGHBOR;

typedef struct
{
    u16 num;
    u16 sendCnt;
    u32 sta[MAX_BITMAP_SIZE/4];

}WAIT_FOR_SEND_INFO, *P_WAIT_FOR_SEND_INFO;


typedef struct
{
    u16 tei:12;
    u16 beaconType:1; //发送的信标类型
    u16 rfBeacon:3; //无线信标标志
    u8 next;
    u8 needSlot; //仍然需要的信标时隙个数
}NOT_CENTER_BEACON_STA, *P_NOT_CENTER_BEACON_STA;

typedef struct
{
    u16                   num;
    NOT_CENTER_BEACON_STA sta[MAX_NOT_CENTER_BEACON_LIMIT];
}NOT_CENTER_BEACON_INFO, *P_NOT_CENTER_BEACON_INFO;
typedef struct
{
    u8 sta[MAX_BITMAP_SIZE];
}GET_BEACON, *P_GET_BEACON;
typedef struct
{
    //Hplc_send 输入
//    u16                         sending_tei;
    WAIT_FOR_SEND_INFO          wait_for_send_info;
    u8                          need_not_center_beacon_sta;         //FALSE, PLC_AutoRelay 不填写 not_center_beacon_count 和 not_center_beacon_sta

    //PLC_AutoRelay 输出
    u8                          cco_mac[MAC_ADDR_LEN];
    u8                          net_seq;              //组网序列号
//    u16                         cfged_meter_num;      //白名单数
    u8                          is_net_complited;       //TRUE 表示组网完成
	u16                         route_period_sec;
    u16                         route_estimate_left_sec;           //距离下次路由评估的时间
    u8                          PcoFoundListIntervalSecond;
    u8                          StaFoundListIntervalSecond;

    NOT_CENTER_BEACON_INFO      not_center_beacon_info;         //如果是要抄读某只表，则包括表本身
    u16                         beacon_period_ms;               //信标周期
    u16                         csma_slot_phase_ms[3];          //CSMA 时隙信息
    u16                         tie_csma_slot_phase_ms[3];      //绑定CSMA 时隙信息

    //频率切换
    u16         new_frequence_second_down;        //值大于0时，需要频段切换
    u16         new_frequence;

	u16         new_hrf_second_down;        //值大于0时，需要频段切换
    u8          new_hrf_channel;
	u8          new_hrf_option;
	u8          Hrf_Hplc_Times;            //hrf信标需要多少个HPLC信标
}BEACON_PARAM, *P_BEACON_PARAM;
extern BEACON_PARAM     beacon_param_current;
typedef struct
{
    HPLC_PHASE              hplc_phase;
    HPLC_PHASE              receive_phase;    
    //NOT_CENTER_BEACON_STA   relay_info[MAX_RELAY_DEEP];     //中继信息，不包括CCO，不包括本身
}HPLC_STA_PARAM, *P_HPLC_STA_PARAM;

typedef struct
{
    HPLC_SEND_PHASE  send_phase;
    CSMA_SEND_TYPE   send_type;
    CSMA_CALL_BACK   cb;
    u8               resend_count;
    u8               link;
	u8               phr_mcs;
    u8               doubleSend;
}PLC_SEND_PARAM, *P_PLC_SEND_PARAM;

typedef BOOL (*GetBeaconParam)(P_BEACON_PARAM pBeaconParam);
typedef BOOL (*GetStaParam)(u16 tei, P_HPLC_STA_PARAM pStaParam);
typedef void (*ChangeNetID)(u32 new_net_id);

void HPLC_SetBeaconParamFunction(GetBeaconParam getbeaconparam);
void HPLC_SetStaParamFunction(GetStaParam getstaparam);
void HPLC_SetChangeNetIDFunction(ChangeNetID changeNetID);
void HPLC_ChangeNID();

//初始化
void HPLC_SendTask_Init(void);

void HPLC_SendCsmaTask(void *arg);
void HPLC_SendBCsmaTask(void *arg);
void HRF_SendCsmaTask(void *arg);
void HRF_SendBCsmaTask(void *arg);
void HPLC_ChangeFrequence(u8 new_frequence, BOOL test_mode);
void HPLC_ChangeFrequenceNotSave(u8 new_frequence, BOOL test_mode);
void HRF_ChangeChannel(u8 channel, u8 option);
void HPLC_ChangeHrfChannelNotSave(u8 channel, u8 option);
void HPLC_ChangeMCSNotSave(u8 phr_mcs, u8 psdu_mcs, u8 pbsize);
void HPLC_ChangeToneMask(u8 new_value);
void HPLC_ChangePower();
void HPLC_SwitchReceivePin();

//CSMA发送
bool HPLC_MakeAndSendMacFrame(const P_PLC_SEND_PARAM pSendParam, const P_SOF_PARAMS pSOFParam, const P_MAC_PARAM pMacParam, u8* pMsdu, u16 msduLen);
bool HPLC_SendTest(const P_PLC_SEND_PARAM pSendParam, u8* pdu_data, u16 pdu_len);

//信标发送
bool Beacon_SendData(u32 beginNTB,u32 endNTB,int64_t offsetNTB,PDU_BLOCK* pdu,SendCallback callback);

//确认帧发送
bool Confirm_SendData(HPLC_PHASE phase,PDU_BLOCK* pdu,SendCallback callback);
bool HrfConfirm_SendData(u8 phr_mcs , PDU_BLOCK *pdu, SendCallback callback);

bool HPLC_TestModeUpdateSlot(CPU_NTB sync_ntb, u32 senderNtb, P_BEACON_MPDU pBeaconPayload);


void HPLC_SendImmediate(uint8_t *pdata, uint16_t len, SendCallback callback, OptionCallback optionFun, HPLC_SEND_PHASE phase);

void HPLC_SendCoor(uint8_t *pdata, uint16_t len, SendCallback callback, OptionCallback optionFun, HPLC_SEND_PHASE phase);
//BOOL HPLC_CanSend(OS_TICK wait_time);
void HPLC_SetLineDriver(uint32_t state,uint32_t phase);
void HPLC_SendSofCallback( int flag);
P_NET_NEIGHBOR HPLC_GetNeighborPtr();

#define MAX_BEACON_NTB          (15*25000*1000ul)


//任务
void HPLC_SendTask(void* arg);

bool HPLC_SendSOFFrame(u8 link,const P_PLC_SEND_PARAM pSendParam, u16 mac_dst_tei, u8* pdu_data, u16 pdu_len);

void HPLC_SetSendPinOff(HPLC_SEND_PHASE phase);

//void HPLC_SetInOff(void);
void HPLC_SetSendPinOn(HPLC_SEND_PHASE phase);
HPLC_PHASE HPLC_GetCurrentReceivePhase(void);
BOOL HPLC_IsMpduApp(u8* pData, u16 nLen);
void HPLC_SetReceiveLedOn(HPLC_PHASE phase);

bool HPLC_NTBSyncAdjust(CPU_NTB sync_ntb, u32 senderNtb);
u8 HPLC_GetPower(u8 frequence, BOOL is_test_mode);
void HrfConfilctProc(u8 *ccoMac,u32  nid,u8 option,u8 channel);

void HPLC_TrySetReceiveLedOff(OS_TICK_64 tick_now);
#ifdef HPLC_CSG
void HPLC_DetectStart(void);
void HPLC_DetectStop(void);
extern HPLC_PHASE detectPhase;
#endif
extern u8 Bcsma_Lid;
extern u8 DetectLidReq;
extern u8 UpdataLidReq;
extern NET_NEIGHBOR     s_net_neigbor[MAX_NID_NEIGHBOR_COUNT]; 
u32 HPLC_FindOneNeighborNidNum(void);
void ChoiceRfChannel(u8 *option, u8 *channel, u8 bitmap[8]);
extern u32 getBeaconPeriod(void);
extern u8 sending_switch;
#define HPLC_WAIT_CPU_NTB     550
#define HRF_WAIT_CPU_NTB      950
#define MAX_CSMA_SLOT_COUNT 30     //信标周期最大10秒，最小分片长度500毫秒
//CSMA 均衡分割结果，时间片序列
typedef struct
{
	u16         slot_ms;
	u8          phase;      //HPLC_PHASE
}SLOT_POS, *P_SLOT_POS;

typedef struct
{
	CPU_NTB_64  csma_start_ntb;
	CPU_NTB_64  csma_end_ntb;
	u16         slot_num;
	SLOT_POS    slot_pos[MAX_CSMA_SLOT_COUNT];
}CSMA_SLOT_INFO, *P_CSMA_SLOT_INFO;
extern CSMA_SLOT_INFO s_slot_bcsma_info;
extern const u8 BatterHrfChannel[3][20];
extern const u8 MaxHrfChannel[3];
extern u8 CCONeedCnt;
void Secure_TestMode(u8 mode ,u8 *data);
bool HPLC_GetCSMASlot(P_CSMA_SLOT_INFO pCsmaSlot, CPU_NTB_64 current_ntb,  CPU_NTB_64 *beginNTB, CPU_NTB_64 *endNTB, u32 leftNTB);
#define CSG_COOR_RATE    4//南网为4ms

u16 HRFBestMcs(u8 isBo,u8 option,u16 pack_len);
u16 HRFPhrBestMcs(u8 isBo,u8 option,u16 pack_len);
int GetPbIndex(int len);
void SetShortListenTick(void);
#endif

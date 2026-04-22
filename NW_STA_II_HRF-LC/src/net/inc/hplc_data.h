#ifndef _HPLC_DATA_H_
#define _HPLC_DATA_H_

#include "os.h"
#include "common_includes.h"
#include <time.h>
//#define WUQI_CCO

#define CCO_TEI 1
#define TRIM_TEI     0xFCF
#define READER_TEI 0xffd
#define	LONG_ADDR_LEN 6
#define NID_BYTE_WIDTH 3
#define CHIP_ID_BYTE_WIDTH 24

#define MAX_PARENT_NUM 5
#define MAX_ROUTE_NUM 1015
#define MAX_NEIGHBOR_NUM MAX_ROUTE_NUM

#define MIN_TEI_VALUE 0
#define MAX_TEI_VALUE 1015

#define MIN_LAYER_VALUE 0
#define MAX_LAYER_VALUE 15

#define MAX_CLC_ROUT_PERIOD  1

//#define FIND_MIN_TEI_NODE

#define LEVEL_DEC_RATE			85 //90%

#define LEVEL_DEC_RATE_BITS     8
#define LEVEL_DEC_RATE_0        256

#define SAME_LEVEL_RATE         70*LEVEL_DEC_RATE_0/100

#define LEVEL_DEC_RATE_1        LEVEL_DEC_RATE*LEVEL_DEC_RATE_0/100
#define LEVEL_DEC_RATE_2        ((LEVEL_DEC_RATE_1*LEVEL_DEC_RATE_1)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_3        ((LEVEL_DEC_RATE_1*LEVEL_DEC_RATE_2)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_4        ((LEVEL_DEC_RATE_2*LEVEL_DEC_RATE_2)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_5        ((LEVEL_DEC_RATE_2*LEVEL_DEC_RATE_3)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_6        ((LEVEL_DEC_RATE_3*LEVEL_DEC_RATE_3)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_7        ((LEVEL_DEC_RATE_3*LEVEL_DEC_RATE_4)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_8        ((LEVEL_DEC_RATE_4*LEVEL_DEC_RATE_4)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_9        ((LEVEL_DEC_RATE_4*LEVEL_DEC_RATE_5)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_10       ((LEVEL_DEC_RATE_5*LEVEL_DEC_RATE_5)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_11       ((LEVEL_DEC_RATE_5*LEVEL_DEC_RATE_6)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_12       ((LEVEL_DEC_RATE_6*LEVEL_DEC_RATE_6)>>LEVEL_DEC_RATE_BITS)
#define LEVEL_DEC_RATE_13       ((LEVEL_DEC_RATE_6*LEVEL_DEC_RATE_7)>>LEVEL_DEC_RATE_BITS)

#define UP_NEED_PRO_FLASH    0x11223344
#define STA_VERSION_FLAG     0x99990002
extern const u16 level_dec_rate[14];

typedef enum
{
    NET_OUT_NET,        //δ����
    NET_WAIT_NET,       //�ȴ�����
    NET_IN_NET,         //������
    NET_OFFLINE_NET,    //����
	NET_STATE_LIMIT_VALUE,
}EN_NET_STATE_TYPE;

typedef enum
{
	UNKNOWN_ROLE = 0,
	STA_ROLE,
	PCO_ROLE,
	RESERVED_ROLE,
	CCO_ROLE,
	ROLE_LIMIT_VALUE,
}EN_ROLE_TYPE;

typedef enum
{
	UNKNOWN_LINE = 0,
	A_LINE,
	B_LINE,
	C_LINE,
	LINE_LIMIT_VALUE,
}EN_PHASE_TYPE;

typedef struct{
	u32 len;
	u32 flag;
}UPDATA_PARA;
typedef struct {
	u32 flash_addr;
	u32 item_flag;
	u32 erase_size;
	u32 data_size;
	u8 data[0];
}UPDATA_ITEM;

typedef enum
{
	NORMAL_RESET = 0,
	POWER_ON_RESET,
	WATCHDOG_RESET,
	EXCEPTION_RESET,
	RESET_LIMIT_VALUE,
}EN_RESET_TYPE;

//�ϱ��ɹ�����Ϣ�ֶ�
#pragma pack(1)
typedef struct
{
	u16 TEI;
	u8 DownSucRate;
	u8 UpSucRate;
}ST_SUC_RATE_TYPE;

//�ɹ�����Ϣ
#pragma pack(4)
typedef struct
{
    u8 UpRate;      //����ͨ�ųɹ���
    u8 DownRate;    //����ͨ�ųɹ���
    u8 SuccessRate; //ͨ�ųɹ���
    bool Flag;      //�ɹ�����Ч��־
}ST_SUC_RATE_DATA;
#define HRF_BIT_MAP_SIZE 16
//�����ھӽڵ���Ϣ����
#pragma pack(4)
typedef struct
{
//	u16 TEI;                //TEI
	u8 Mac[LONG_ADDR_LEN];  //MAC
	EN_ROLE_TYPE Role;      //��ɫ
	EN_PHASE_TYPE Phase;    //����
	u8 Layer;               //�㼶
	s8 SNR;                 //�����
	s8 RSSI;                //�ź�ǿ��
	u8 RouteMinSucRate;     //·����Сͨ�ųɹ���
	//u32 RouteEstSucRate;     //·�������ɹ���
	u8 SnrCount;           //����ȸ��´���
	u8 PcoSNR;              //����վ�����վ��ͨ�������
	u8 PcoSucRate;          //����վ�����վ���ͨ�ųɹ���
	u8 PcoDownSucRate;      //����վ�����վ��ͨ�ŵ�����ͨ�ųɹ���

	s16 snrSum;             //����Ⱥ�
	u16 PcoTEI;             //����վ��TEI
	
//������ز���
	u8 Sta2CcoRfHop:4;        //��cco����hrfת��������
	u8 GetHplc:1;
	u8 GetHrf:1;
	s8 RfSnr;               //hrf SNRֵ
	s8 RfRssi;              // hrf RSSI
	s8 ReportSnr;           //�Է���֪�ҷ��������
	s8 ReportRssi;          //�Է���֪�ҷ���Rssi 
	u8 HrfSeq;              //���߽ڵ㷢���б� ���
	//u8 NoListCnt;           //����δ�յ������б��ĸ���
	u8 HrfUpRate;           //hrf���гɹ��� �����ϻ�
	u8 HrfNoCnt;            //hrf δ�������гɹ��ʵ�cnt
	u16 HrfBitmap;          //hrfͳ�Ƴɹ��ʵ�bitmap
	//u32 HrfUpTick;          //hrf ����bitmap��tick 
	//u32 HrfRouteEstSucRate; 	//Hrf·�������ɹ���
//��1·������ͳ�Ƽ���	
	u8	LastNghTxCnt[MAX_CLC_ROUT_PERIOD];       //�ϸ�·�������ھӽڵ㷢�ͷ����б����ĵĸ���
	u8	LastNghRxCnt[MAX_CLC_ROUT_PERIOD];       //�ϸ�·�������ھ�վ����յ���վ��ķ��ͱ��ĸ���
	u8  headNghIdx;//idx ָ��ǰ���ڵõ�����һ���ڵĲ���
	u8  headNghCnt;//cnt �洢�������ϸ�·������֮ǰ�洢�˼�������
	u8	LastTheStaRxCnt[MAX_CLC_ROUT_PERIOD];    //�ϸ�·�����ڱ�վ����յ��ı��ĸ���
    u8  headStaCnt;
	u8  headStaIdx;
	u8  TheStaRxCnt;	    //��վ����յ��ı��ĸ���
	u8 PeriodCount;        //��֮ͨ�ŵ�·�����ڸ���

	
	//----------------------------//
	//�������
	bool CannotConnect:1;		//CCO�����ֹ����PCO
	bool NoAckFlag:1;			//��������Ӧ��־
	u8   ConnectTryTimes;	//�������Դ���

	ST_SUC_RATE_DATA  SucRate[2];   //[0]��[1]�ֱ�Ϊ������εĳɹ���
    TimeValue DiscoverTimer;       //�˶�ʱ���������Լ�·�����ڵ�������ת����ڴ˶�ʱ��ʱû��ʱ���յ��ķ����б�֡��ͳ�Ƴɹ���
    TimeValue DiscoverTimer2;        //�˶�ʱ���������Լ�·�����ڵ�������ת����ڳ����˶�ʱ�����յ��ķ����б�֡��ͳ�Ƴɹ���


	TimeValue ConnectTimer;	//����������Ӧ�Ķ�ʱ��
}ST_NEIGHBOR_TYPE;

////�ھ�������������TEI��С����˳������
//typedef struct 
//{
//	u16 TEI;
//	u16 Idx;
//}ST_NEIGHBOR_IDX_TYPE;
#pragma pack(4)
typedef struct 
{
	u16 NeighborNum;
	u16 TEI[MAX_ROUTE_NUM];//tei�洢��
	u32 TEI_MAP[MAX_BITMAP_SIZE/4];//tei������
	ST_NEIGHBOR_TYPE Neighbor[MAX_NEIGHBOR_NUM];
}ST_NEIGHBOR_TAB_TYPE;

#pragma pack(4)
typedef struct
{
	u16 TEI;
	u8 Layer;
}ST_PARENT_TYPE;

#pragma pack(4)
typedef struct
{
	u8 ParentNum;
	ST_PARENT_TYPE Parent[MAX_PARENT_NUM];
}ST_PARENT_TAB_TYPE;

#pragma pack(4)
typedef struct
{
	//	u16 TEI;
	u16 RelayTEI:12;
	u16 RelayType:4;
	u16 CandTEI:12;//��ѡTEI���ڴ洢������������TEI
	u16 CandType:4;
	u32 seqBitMap;
	u16 MsduSeq;            //�����һ��MSDU���
    u8  RebootTimes;        //��������
}ST_ROUTE_TYPE;

#pragma pack(4)
typedef struct
{
//	u16 RouteNum;
//	u16 RouteArray[MAX_ROUTE_NUM];		//����ͳ���ж�����·��
	ST_ROUTE_TYPE Route[MAX_ROUTE_NUM];	
}ST_ROUTE_TAB_TYPE;

#pragma pack(4)
typedef struct
{
	u8 NetState;                //��������״̬(δ�������ȴ�������������������)
	u16 TEI;                    //TEI
	u8 Mac[LONG_ADDR_LEN];      //MAC
	u8 CcoMac[LONG_ADDR_LEN];   //CCO��MAC
	u8 NID;                     //����NID
	u8 connectReader;           //�Ƿ����ӳ�����
	u8 ReaderLink;              //���������ӵ��ŵ� 0:HPLC  1:HRF
	u16 HrfOrgChannel;          //����������֮ǰ��channel
	EN_DEVICE_TYPE DeviceType;  //�豸����
	u8 NetBuildSeq;             //�������к�
	u8 Layer;                   //�㼶
    u8 NetFlag;                 //������־(0:δ���,1:�����)
	u8 RfHop;                   //����rf������
	EN_ROLE_TYPE Role;          //��ɫ
	EN_PHASE_TYPE Phase;        //��������
	u32 startTick;
	u32 lastTick;
	u8 ReaderSeq;
	u8 PcoTmi;             //PCO �����ű�ʹ�õ�hplcƵ��
	u8 PcoPhrMCS;               //PCO �����ű�� phr MCS
	u8 PcoMCS;                  //PCO �����ű�ʹ�õ�HRF MCS
}ST_STA_ATTR_TYPE;

#pragma pack(1)
typedef struct
{
	u16 Year  :7;
	u16 Month :4;
	u16 Day	  :5; 
}ST_SYS_DATE_TYPE;

#pragma pack(1)
typedef struct 
{
	EN_RESET_TYPE LastRstReason;
	u8 BootVer;
	u8 SofeVer[2];
	ST_SYS_DATE_TYPE VerDate;
	u8 FactoryID[2];
	u8 ChipCode[2];
}ST_STA_VER_TYPE;

#pragma pack(1)
typedef struct 
{
	//ģ��Ӳ���汾��Ϣ
	u8 ModuleHardVer[2];
	ST_SYS_DATE_TYPE ModuleHardVerDate;

	//оƬӲ���汾��Ϣ
	u8 ChipHardVer[2];
	ST_SYS_DATE_TYPE ChipHardVerDate;

	//оƬ�����汾��Ϣ
	u8 ChipSoftVer[2];
	ST_SYS_DATE_TYPE ChipSofVerDate;

	//Ӧ�ó���汾��
	u8 AppVer[2];
}ST_STA_EXT_VER_TYPE;

#pragma pack(1)
typedef struct
{
	u8 mac[6];
	u8 factoryID[2];
	u8 ver[2];
	u8 chipCode[2];
	u8 year;
	u8 month;
	u8 day;
    u8 assetsCode[24]; //�ʲ�����
}ST_STA_ID_TYPE;

extern ST_STA_ID_TYPE StaChipID;
extern ST_STA_VER_TYPE StaVersion;
extern ST_STA_EXT_VER_TYPE StaExtVersion;

#pragma pack(4)
typedef struct 
{
	u16 HardRstCnt;
	u16 SofeRstCnt;
	u32 AssocRandom;
	//u8 ChipFactoryId[CHIP_ID_BYTE_WIDTH];
	//ST_STA_VER_TYPE StaVerInfo;
//	u16 IsPowerOff;
	u8 LastMeter[LONG_ADDR_LEN];   //��¼��һ��������meter
	u8 LastCcoMac[LONG_ADDR_LEN];   //��¼��һ��������CCO��MAC
	u8 LastFreq;                    //��¼��һ��������Ƶ��
	u16 LastRFChannel;				//��¼��һ�������������ŵ�
}ST_STA_SYS_TYPE;
#pragma pack(1)
typedef struct
{
	u8 res[45];
	u8 baud; //�����ʣ�0-1200;1-2400;2-4800;3-9600;4-38400
	u8 ledMode; //LED����ģʽ��0->ԭ������1->������һ���������
	u8 txPowerMode[2]; //0��ʾ���书���Զ��л�ģʽ��Ĭ��ʹ��HPLC_LinePower��ΪHPLC_ChlPower
	                //1��ʾ���书�ʹ̶�ģʽ��ʹ��flash��洢��txPower��ΪHPLC_ChlPower
	u8 txPower[4];
	u8 hrfTxPower[2];
}ST_STA_FIX_PARA;
extern ST_STA_SYS_TYPE StaSysPara;

#pragma pack(4)
typedef struct
{
	u16 RoutingPeriod;//s           //·������
	u16 HeartBeatPeriod;//s         //��������
	u16 SucRatePeriod;//s           //�ɹ����ϱ�����
	u16 StaDiscListPeriod;//s          //�����б�����
	u16 StaProxyChangePeriod;//s		//�����������
    u16 PcoDiscListPeriod;//s          //����վ�㷢���б�����
	u16 HrfDiscListPeriod;//s          //hrf �����б�����
    u32 BeaconPeriodCnt;               //�ű����ڼ���
    u32 ReceiveBeaconCnt;               //�յ��ű����
	u8 LastPeriodDiscListTxCnt[MAX_CLC_ROUT_PERIOD];     //�ϸ�·�����ڷ��ͷ����б�����
	u8 headCnt;
	u8 headIdx;
	u8 DiscListTxCnt;               //���·�����ڷ��ͷ����б�����
	u8 OldCnt;                      //�ϻ����ڸ���
	//u8 FirstBeacIsRetrench;         //�����ڵ�һ�νӵ����ű��Ǿ����ű�
}ST_STA_PERIOD_TYPE;
typedef struct tm  Time_Special;
typedef struct {
	u8 isVaild;//0 ��Ч 1 ����������Ч  2 ��ʱ�� 3У׼��ʱ�� 4������ʱ��
	u32 sec_form_1900;//��2000�굽���ڹ�ȥ������
	Time_Special time;
	u32 sysTick;//����ʱ�̵�ϵͳtick
	s32 diff_tick;
}Time_Module_s;

#define MAC_GRAPH_CONFIG_NUM 50

typedef struct{
	u8 config_flag;
	u8 num[2];//������
	u8 mode[2][MAC_GRAPH_CONFIG_NUM]; //ʹ�ܣ�1��ʼ��0�رգ����Ϊ0ʱ��
	u8 period[2][MAC_GRAPH_CONFIG_NUM];//min
	u32 ident[2][MAC_GRAPH_CONFIG_NUM];
}Graph_Config_s;
typedef struct {
	u32 all_used;//�ÿ������Ƿ�ȫ��ʹ����ϵ�falg ����0xffffffff�������û��ʹ����� 
				 //����ʱ��Ҫ�����һ�βſ���д���Է�ֹ�ظ���̵�����
	u32 end_sec; //����ʱ��ʱ���			 
	u32 have_data;//�ÿ������Ƿ������ݵ�flag
	u32 used_num;//�ÿ�����ʹ�õ���ţ�ÿһ��ʹ���¿����ż�1
	u32 sum_befor;//ǰ�������ĺ�
	u8 data[0];
}Flash_4K_Area;

typedef struct{
	u32 sec_from_1900;//��1900�굽���ڹ�ȥ������
	u32 ident;
	u8 len;
	u8 cs;
	u8 data[0];
}Graph_Data_s;

typedef enum {
	DATE_YEAR,
	DATE_MON,
	DATE_DATE,
	DATE_HOUR,
	DATE_MIN,
	DATE_SEC,
}DATE_ENUM;
typedef struct
{
	u8 option;
	u8 channel;
}HRF_Reader_Channel;

//=============================================================================
void RestoreFactorySet(void);

void InitStaData(void);

void StorageStaAttrInfo(void);

void StorageStaParaInfo(void);
void ReadStaParaInfo(void);

void StorageStaRouteInfo(void);
void StorageStaResetTime(u16 soft_cnt,u16 hard_cnt);
void StorageStaIdInfo(void);
void ReadStaIdInfo(void);
void FlushAllFlash(void);
//=============================================================================
//���ݼ��غͳ�ʼ��
void StaDataInit(void);

//��������������ݿɶ�дָ��
const ST_STA_ATTR_TYPE * GetStaBaseAttr(void);

//�������ڲ����ɶ�дָ��
const ST_STA_PERIOD_TYPE * GetStaPeriodPara(void);

//����ϵͳ�����ɶ�дָ��
const ST_STA_SYS_TYPE * GetStaSysPara(void);

const ST_STA_VER_TYPE * GetStaVerInfo(void);

const ST_STA_EXT_VER_TYPE* GetStaExtVerInfo(void);

//��������״̬
bool SetNetState(EN_NET_STATE_TYPE netState);
//����CCO��ַ
bool SetCCO_Mac(u8 addr[LONG_ADDR_LEN]);
//��������CCO��ַ��������flash��
bool SetLastCCOMac(u8 addr[LONG_ADDR_LEN]);
//��������Ƶ�Σ�������flash��
bool SetLastFreq(u8 freq);
//�������������ŵ���������flash��
bool SetLastRFchannel(u16 ch);
//���ò�����
bool SetBaud(u8 baud);
//���ñ�վ��MAC��ַ
bool SetStaMAC(u8 addr[LONG_ADDR_LEN]);
//��¼��һ�������ĵ����ַ
bool SetStaLastMeter(void);
//���ñ�վ��TEI
bool SetTEI(u16 TEI);
//���ñ�վ�����TEI
bool SetPCO_TEI(u16 TEI);
//���ñ�վ��㼶
bool SetLayer(u8 layer);
//���ñ�վHrf����
bool SetRfHop(u8 hop);
//���ñ�վ���ɫ
bool SetRole(EN_ROLE_TYPE role);
//��������NID
bool SetNID(u8 NID);
//�����������к�
bool SetNetSequence(u32 net);
//���ñ�վ������
bool SetPhase(EN_PHASE_TYPE phase);
//����Ƶ��
bool SetSequence(u8 seq);
//����������־
bool SetNetFlag(u8 flag);


//���������ϻ����ڸ���
bool SetHrfOldPeriodCnt(u16 OldCnt);
//����·������
bool SetRoutePeriod(u16	RoutePeriod);
//����STA�����б�����
bool SetStaDiscoverPeriod(u16 DiscoverPeriod);
//����PCO�����б�����
bool SetPcoDiscoverPeriod(u16 DiscoverPeriod);
//������������
bool SetHeartBeatPeriod(u16 HeartbeatPeriod);
//���óɹ����ϱ�����
bool SetSuccessPeriod(u16 SuccessPeriod);
//���ô����������
bool SetProxyChangePeriod(u16 ProxyPeriod);
//����HRF�����б�����
bool SetHrfDiscoverPeriod(u16 ProxyPeriod);
//�����ű����ڼ���
bool SetBeaconPeriodCnt(u32 BeaconPeriodCnt);
//�����յ��ű����
bool IncreaseBeaconCnt(void);
//���ñ����ڵ�һ���յ����ű��Ƿ��Ǿ����ű�
//void SetFirstRetrenchFlag(int flag);
//����PCO���ű�ʹ�õ�TMI
void SetPcoBeaconTmi(u8 tmi);
//����PCO���ű�ʹ�õ�phr��psdu��mcs
void SetPcoBeaconMcs(u8 phr, u8 psdu);
//��ȡ�ӿ�

//��ȡ�����ڵ��(���5��,����ֵΪʵ���������)
u8 GetParentPCO(u16 *pco_arr);
u8 GetHrfParentPCO(u16 *pco_arr);
int GetBatterParentPCO(u16 *pco_arr);
//���ù̶���ѡȡ���ڵ����ȼ� �˺������ڻ�̨����ʱ��Ч
void SetFixLinkParent(u8 link);
//��ȡ��������ڵ�
void GetProxyParent(u16 *pco_arr,u8 link);

int GetBatterProxyPCO(u16 pco_arr[2][5]);
//��ȡ�ھ�λͼ(bitmap��ʾλͼ,����ֵΪʵ���ֽ���)
u8 GetNeighborBitmap(u8 *bitmap, u16 *ngh_num,u16 *max_tei);

//��ȡ�����б���Ϣ(receivePacketΪ�ھӽڵ���յķ����б������������飬maxָʾreceivePacket��������ֵ������ָʾʵ�ʵ����鳤��)
u16 GetReceiveNeighbor(u8 *receivePacket, u16 max,u8 *bitmap,u16 bitmapSize);
//��ȡ�����б�����
u16 GetNeighborNum(void);

//��ȡ������վ����Ϣ
const ST_NEIGHBOR_TYPE* GetPCO_Info(void);

//��ȡ�ھӵ�TEI
u16 GetPCOTei(void);
//��ȡָ���ھӽڵ���Ϣ
bool GetNeighborInfo(u16 tei, ST_NEIGHBOR_TYPE** ngh_info);

//��ȡ�ھӱ�
const ST_NEIGHBOR_TAB_TYPE* GetNeighborTable(void);

//��ȡ��վ���(tableΪָ����վ�����ָ��,������վ��ĸ���)
u16 GetChildSucRateTab(ST_SUC_RATE_TYPE* table);

//����һ·�����ڽ���ʱת�����ݴ���
void DataProcessAtEndRoutePeriod(u8 SendDisNum);

//��ȡ�ھ�վ��������ͨ�ųɹ��ʣ��ϸ�·�����ڣ�,getLast��־˵�����û�л�ȡ���ϸ�·�����ڵ�ͨ�ųɹ��ʣ����Ի�ȡ���ϸ�·������
bool GetNeighborSucRate(u16 TEI,u8 *upRate,u8 *downRate,u8 *sucRate,bool getLast,u8 link);
//���˷����б��ĺ��� ���õĵط�����Timer��task�����nei_bitmap�������ٽ���Դ����
void* NeighborFilter(int hplc_hrf,int send_num,int*bitmap_size);

//����ͨ�ųɹ���
bool SetNeighborSucRate(u16 TEI,u16 sendNum,u16 receiveNum);
//��ȡ����ͨ�ųɹ���
u32 GetEstSucRate(u8 link, u16 tei);
//���ýӿ�
#define SET_NEIGHBOR_TEI				(1UL<<0)
#define SET_NEIGHBOR_MAC				(1UL<<1)
#define SET_NEIGHBOR_ROLE				(1UL<<2)
#define SET_NEIGHBOR_PHASE				(1UL<<3)
#define SET_NEIGHBOR_LAYER				(1UL<<4)
#define SET_NEIGHBOR_SNR				(1UL<<5)
#define SET_NEIGHBOR_RSSI				(1UL<<6)
#define SET_NEIGHBOR_ROUTE_MIN_RATE		(1UL<<7)
#define SET_NEIGHBOR_PCO_TEI			(1UL<<8)
#define SET_NEIGHBOR_PCO_SNR			(1UL<<9)
#define SET_NEIGHBOR_PCO_RATE			(1UL<<10)
#define SET_NEIGHBOR_PCO_DOWNRATE 		(1UL<<11)
#define SET_NEIGHBOR_LAST_TX_CNT		(1UL<<12)
#define SET_NEIGHBOR_LAST_RX_CNT		(1UL<<13)
//#define SET_NEIGHBOR_LAST_STA_RX_CNT	(1UL<<14)
//#define SET_NEIGHBOR_TX_CNT				(1UL<<15)
//#define SET_NEIGHBOR_RX_CNT				(1UL<<16)
//#define SET_NEIGHBOR_MSDU_SEQ			(1UL<<15)
#define SET_NEIGHBOR_BEACON_CNT			(1UL<<16)
#define SET_NEIGHBOR_STA_RX_CNT			(1UL<<17)
//#define SET_NEIGHBOR_REBOOTTIMES        (1UL<<18)
#define SET_NEIGHBOR_HRF_SNR            (1<<19)
#define SET_NEIGHBOR_HRF_RSSI           (1<<20)
#define SET_NEIGHBOR_HRF_HOP            (1<<21)
#define SET_NEIGHBOR_HRF_REPORT_RSSI    (1<<22)
#define SET_NEIGHBOR_HRF_REPORT_SNR     (1<<23)
#define SET_NEIGHBOR_HRF_REPORT_RATE    (1<<24)


#define HRF_INVAILD_CHANNEL_INFO        0x7f
//����һ���ھӽڵ�(TEIΪ�ھӽڵ�TEI,neighborΪֵ,flagsΪ�����궨��Ļ�ֵ,�����һλ����λ����neighbor��Ӧ����Ӧ�ֶ���Ч)
//���������TEI�����ڱ�������һ�������false�����÷���ΪTEI���Ϸ������߲������ڴ���
bool SetNeighbor(u16 TEI,const ST_NEIGHBOR_TYPE* neighbor,u32 flags);

//ɾ���ھ�
bool DeleteNeighbor(u16 TEI);

//��������ھӵ�������ز���
void CleanAllNeighborEntryNetPara(void);

//����������վ��(����ͬ��)
bool SetMainParent(u16 TEI,const ST_NEIGHBOR_TYPE* neighbor,int flags);

//���ò�������PCO
bool SetCannotConnectPCO(u16 TEI);

//������������ӦPCO
bool SetNoAckPCO(u16 TEI,u32 timeout);
void clearPCOTryTime(u16 TEI);
//�ж�һ��PCO�Ƿ��������
bool IsPCOVaild(u16 TEI);


/*------------------·�ɱ�����----------------------*/
//��ȡ·����һ��(������Ҫ���������Ŀ��TEI��������һ��TEI����û��·�ɣ�����0)
u16 GetRelayTEI(u16 TEI);
u16 GetRelayTeiType(u16 tei);
u8 GetRelaySendLink(u16 tei);
//����·����Ϣ(����Ŀ��TEI,�м�TEI)
bool SetRoute(u16 DstTEI, u16 RelayTEI,u8 RoutType);

//���ñ�ѡ·����Ϣ(����Ŀ��TEI,�м�TEI)
bool SetCandRoute(u16 DstTEI, u16 RelayTEI,u8 RoutType);
//������ѡ·����Ϣ ��û����·��������·�� ������·�����ñ�ѡ·��(����Ŀ��TEI,�м�TEI)
bool SetPreferenceRoute(u16 DstTEI,u16 RelayTEI,u8 RoutType);
//ɾ��·��
bool DeleteRoute(u16 TEI);

//�������·�ɱ����ھ�����
void CleanAllData(void);
//�������������
void CleanPeriodData(void);
//ʹ��Ĭ��ֵ��ʼ���������
void InitAttrData(void);

void SetHrfBit(u16 tei, u8 seq);
//��ȡվ���hrf����ͨ�ųɹ���
u8 GetHrfDownRate(u16 tei);
/*------------------��������----------------------*/
void GetSelfMSG(u8 msg[18]);	//��ȡ�����Զ�����Ϣ


extern u32 IsPowerOff;
extern u32 PLL_Vaild;
extern int PLL_Trimming;

void StorageStaFixPara(void);
int ReadStaFixPara(void);
void StorageVerInfo(void);
void ReadVerInfo(void);
void StorageExtVerInfo(void);
void ReadExtVerInfo(void);
void ConfigCSG21DeafultGraphPara(void);
void ReConfigCSG21GraphPara(void);
void StorageGraphPara(void);
void EraseGraph(void);

void StoragePowerOffFlag(void);
void ReadPowerOffFlag(void);
void StoragePllTrim(int trim);
void ReadPllTrim(void);
void             SYS_UP_ReWrite(UPDATA_PARA* para);
void OpenOfflineSendSlot(void);
void DummyBeacon(void);
void MakeBeacon(void);

void StopSendSync(void);

void ReadStaParaInfoOnly(void);
void StaTxPowerProcess(void);
void StaRebootCntProcess(void);
void crash_log_flash_save(void);
void crash_log_flash_clear(void);
u32  crash_log_flash_read_block(u8 block_idx, u8 *buf, u32 buf_size);
#define CRASH_LOG_BLOCK_SIZE   240  /* 5 records * 48 bytes per record */
void StaLastFreqProcess(void);
extern Time_Module_s Time_Module[3];
extern Graph_Config_s  Graph_Config;
extern ST_STA_FIX_PARA StaFixPara;
void StorageTime(void);
Graph_Data_s * FindStaGraphData(u32 sec_from_2000,u32 ident);
void StorageGraphData(Graph_Data_s *pdata);
u32 Data2Sec(Time_Special* ptime);
void DateSpecialChangeSec(Time_Special* ptime,s32 sec);
bool DffSecIn24H(Time_Special* ptimea,Time_Special* ptimeb);
int Diff_Specialtime(Time_Special* ptimea,Time_Special* ptimeb);
void EraseGraphAtTime(u32 time);
bool SetGraphConfig(u8 config);

void StaLastRFChannelProcess(void);//��ȡflash��վ����һ�������������ŵ�


void GetNewReader(u8 link);
void StopSendSync(void);
void StartSendSync(u8 broad,u8 seq,u8 hrfindex,u8 pdu_type);

//HRF�����ŵ���Ϣ
u8* UpNeighChannelInfo(u16 scrTei,u8 type, u8 *data, u16 *bitStart,u8 isBit,u16 tei);
//HRFд�����б��ŵ���Ϣ
u16  WiteNeighborInfo(u8 type,u8 bit_type,u8 *src_data,s16 len,u32 *bitmap);
//�����ϻ����й���
void HrfOldUpRate(void* arg);
extern ST_STA_FIX_PARA StaFixPara;
extern int defultAddrCnt;
extern u32 HPLCCurrentFreq;
extern ST_NEIGHBOR_TAB_TYPE StaNeighborTab;
extern const HRF_Reader_Channel HrfBroadChannel[14];
extern TimeValue PowerChangeTimer;

#endif

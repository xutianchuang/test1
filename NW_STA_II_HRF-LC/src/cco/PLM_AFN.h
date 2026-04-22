/* Q_GDW_AFN.h */
#ifndef Q_GDW_AFN_H
#define Q_GDW_AFN_H

#ifdef __cplusplus
extern "C"
{
#endif

#pragma pack(push,1)

typedef struct
{
    u8 protocol;        //规约类型
    u8 slave_num;       //从节点附属节点数量 n
    //从节点附属节点 1 地址
    //......
    //从节点附属节点 n 地址
//    u8 len;             //报文长度 L
    u8 data[1];         //报文内容
}AFN13_F1_09, *P_AFN13_F1_09;

typedef struct
{
    u8 protocol;        //规约类型
    u8 time_related;
    u8 slave_num;       //从节点附属节点数量 n
    //从节点附属节点 1 地址
    //......
	//从节点附属节点 n 地址
//   u8 len;             //报文长度 L
    u8 data[1];         //报文内容
}AFN13_F1_13, *P_AFN13_F1_13;

typedef struct
{
    u16 tei;
    u16 pco;
    u8 level:4;
    u8 roles:4;
}STA_TOPOLOGY_INFO, *P_STA_TOPOLOGY_INFO;
typedef struct
{
    u16 tei;
    u16 pco;
	u32 InNetTime;
	u16 changePcoTime;
	u16 leaveTime;
	struct{
		u8 level:4;
		u8 role:4;
	};

}STA_TOPOLOGY_INFO_GD, *P_STA_TOPOLOGY_INFO_GD;
typedef struct
{
    u16 tei:12;
	u16 module:4;
    u16 pco;
	u32 InNetTime;
	u16 changePcoTime;
	u16 leaveTime;
	struct
	{
		u8 level:4;
		u8 role:3;
		u8 Link:1;
	};

}STA_TOPOLOGY_INFO_HRF_GD, *P_STA_TOPOLOGY_INFO_HRF_GD;
typedef struct
{
    u16 tei:12;
	u16 module:4;
    u16 pco;
    u8 level:4;
    u8 roles:3;
	u8 Link:1;
}STA_TOPOLOGY_INFO_HRF, *P_STA_TOPOLOGY_INFO_HRF;
typedef struct
{
    u8                  mac[MAC_ADDR_LEN];
    u8                  dev_type;
    STA_CHIP_ID_INFO    chip_id_info;
    u8                  software_v[2];        //芯片软件版本信息
}STA_CHIP_INFO, *P_STA_CHIP_INFO;
typedef struct
{
	u8                  mac[6];
    u8                  software_v[2];
    u8                  Data[3];
	u8                  moudleCode[2];
	u8                  chipCode[2];
}STA_NODE_INFO, *P_STA_NODE_INFO;
typedef struct
{
	u8 mac[MAC_ADDR_LEN];
    u16 tei;
    u16 pco;
    u8 level:4;
    u8 roles:4;
	u8 device;
	u8 factory_code[2];
	u8 up_rate;
	u8 down_rate;
	u8 boot_version;
	u8 soft_version[2];
	u8 res[5];
}STA_10F102_INFO, *P_STA_10F102_INFO;
typedef struct
{
    u8  protocol;
    u8  reserve;
    u16 len;
    u8  data[0];
}PARALLEL_READ_METER, *P_PARALLEL_READ_METER;

typedef struct
{
    u8  sec;
    u8  min;
    u8  hour;
    u8  day;
    u8  month;
    u8  year;

}PLM_DATETIME_BCD, *P_PLM_DATETIME_BCD;

typedef struct
{
    u8  comm_speed;
    u8  mac_addr[MAC_ADDR_LEN];
    u8  protocol;
    u8  len;
    u8  data[1];

}AFN04_F3, *P_AFN04_F3;

typedef struct
{
    PLM_DATETIME_BCD start_time;
    u16 time_last_min;
    u8  node_resend_count;      //从节点重发次数
    u8  rand_time_count;        //随机等待时间片个数，时间片长度：150ms

}AFN11_F5, *P_AFN11_F5;

typedef struct
{
    u8  mode;  //0-禁用，1-启用
    u8  period; //单位分钟
    u8  protocol; //2-645-2007，3-698
    u8  meter_1phase;
	u8  meter_1phase_data_num;
	u8  meter_1phase_rsp_len;
    u8  data[0];
}AFN05_F101, *P_AFN05_F101;


typedef struct
{
	u8 meter_addr[6];
	u8 relay_level:4;
	u8 snr:4;
	u8 phase:3;
	u8 protocol:3;
	u8 :2;
}AFN10_F03,*P_AFN10_F03;


typedef struct
{
	u8 meter_addr[6];
	u8 relay_level:4;
	u8 snr:4;
	u8 rec_time:5;
	u8 :3;
}AFN03_F03,*P_AFN03_F03;

typedef struct{
	u8 work_status:1;
	u8 reg_status:1;
	u8 event_status:1;
	u8 area_status:1;
	u8 :2;
	u8 func:2;
}ROUT_SWITCH,*P_ROUT_SWITCH;

typedef struct{
	u8 work_status:1;
	u8 reg_status:1;
	u8 func:2;
	u8 :4;
}SHANXI_ROUT_SWITCH,*P_SHANXI_ROUT_SWITCH;

#pragma pack (pop)

extern unsigned char afn_func_01(P_LMP_APP_INFO pLmpAppInfo);
extern unsigned char afn_func_02(P_LMP_APP_INFO pLmpAppInfo);
extern unsigned char afn_request_info_03(P_LMP_APP_INFO pLmpAppInfo);
extern unsigned char afn_mac_check_04(P_LMP_APP_INFO pLmpAppInfo);
extern unsigned char afn_ctl_cmd_05(P_LMP_APP_INFO pLmpAppInfo);
extern unsigned char afn_auto_report_06(P_LMP_APP_INFO pLmpAppInfo);

unsigned char afn_relay_query_09(P_LMP_APP_INFO pLmpAppInfo);
unsigned char afn_relay_query_10(P_LMP_APP_INFO pLmpAppInfo);
unsigned char afn_relay_setting_11(P_LMP_APP_INFO pLmpAppInfo);
unsigned char afn_relay_data_forward_13(P_LMP_APP_INFO pLmpAppInfo);
unsigned char afn_relay_action_12(P_LMP_APP_INFO pLmpAppInfo);
unsigned char afn_relay_data_read_14(P_LMP_APP_INFO pLmpAppInfo);
unsigned char afn_func_F1(P_LMP_APP_INFO pLmpAppInfo);

//extern USHORT PLC_get_sn_from_addr(UCHAR * addr);
USHORT afn_get_meter_phase_info(USHORT node_sn);
#ifdef GDW_2019_GRAPH_STOREDATA
//陕西读取相位
USHORT afn_get_meter_shanxi_phase_info(USHORT node_sn);
#endif
#ifdef __cplusplus
}
#endif

#endif /* Q_GDW_AFN_H */


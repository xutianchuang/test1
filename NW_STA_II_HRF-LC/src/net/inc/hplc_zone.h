#ifndef _HPLC_ZONE_H_
#define _HPLC_ZONE_H_

//台区识别
#pragma pack(push, 8)
typedef struct __cco_type_zone__
{
	int64_t cco_local_diff_sum;
	s32 corr_sum1;
	s32 corr_sum2;
	s32 corr_sum3;
	s32 rssi_sum[3];
	u32 rssi_cnt[3];
	s32 ave_rssi;
	u32 no_get_ntb;//未找到正确的NTB的次数
        u32 large_period_diff_cnt;
        u32 local_period_err_cnt;
        u32 cco_period_err_cnt;
	u32 total_time;//总共进行多少次采集
	u32 seq;//采集序号
	u32 beacon_diff;//该网络与本网NTB的差值
	u8 NID;
	u32 last_zone_tick;//上一次开始采集的时刻
	u32 total_period_num;
	u8 cco_mac[6];
	u8 sta_phase;
}cco_type_zone;
#pragma pack(pop)

extern const u8 area_extend_result;

//初始化
void ZoneInit(void);
void ZoneReceiveEventData(u8* data,u16 len,u8 ismyNet,u32 nid);

cco_type_zone *find_zone_cco_by_nid(u8 nid);
cco_type_zone *alloc_zone_cco_by_nid(u8 nid);
#ifdef NW_TEST
#define ZONE_MIN_INFO           18
#else
#define ZONE_MIN_INFO           30
#endif
#define ZONE_MAX_NO_NTB         10
#define ZONE_MAX_PERIOD_DIFF     5000/40  //5us
#define ZONE_MAX_PERIOD_DIFF_THR    3//unit 320ns
#ifdef NW_TEST
#define ZONE_PERIOD_VALID_THR   500//unit 320ns
#else
#define ZONE_PERIOD_VALID_THR   200//unit 320ns
#endif
#define ZONE_LARGE_PERIOD_DIFF_CNT_THR  5
#define ZONE_CORR_THR           200

#define MAX_CCO_ZONE  20

#endif

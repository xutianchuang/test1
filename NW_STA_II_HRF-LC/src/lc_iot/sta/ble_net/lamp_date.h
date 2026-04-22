#ifndef __LAMP_DATE_H__
#define __LAMP_DATE_H__

#define LAMP_DATE_NOT_SYNC_FLG (0x00)
#define LAMP_DATE_HAVE_SYNC_FLG (0x31)

#define LAMP_DATE_MINUTE_SECOND_COUNT (60) 
#define LAMP_DATE_HOUR_SECOND_COUNT (60 * 60) 
#define LAMP_DATE_DAY_SECOND_COUNT (24 * 60 * 60) 

#define RE_SYNC_DATE_TIME_MS (1000 * 60 * 1)

typedef struct
{
    unsigned int time_second_stamp_cnt;
    unsigned int date_second_stamp_cnt;
    unsigned int data_sync_second_cnt;
    unsigned int date_check;
    unsigned int runtime;
} lamp_date_param_t;

void lamp_date_init();
void lamp_date_task();
void lc_iot_calibrate_date_cb_fun(unsigned int date_time_stamp);
const lamp_date_param_t* get_lamp_date_base_param();

extern lamp_date_param_t lamp_date;

#endif

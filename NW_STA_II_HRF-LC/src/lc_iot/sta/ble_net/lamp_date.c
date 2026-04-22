#include "common_includes.h"
#include "lamp_date.h"


lamp_date_param_t lamp_date;
static bool re_sync_date;
const lamp_date_param_t* get_lamp_date_base_param()
{
	return &lamp_date;
}

/**
 * @brief    定时器秒级回调函数
 *
 * 该函数每秒执行一次，更新日期和时间计数器。
 * 如果日期已同步，更新计数器；
 * 若达到一天，则重置计数器；
 * 若达到一小时，则重置计数器和标志。
 *
 * @param    none
 * @return   none
 */
static void lamp_date_timer_second_cb_fun(void)
{
    if (lamp_date.date_check == LAMP_DATE_HAVE_SYNC_FLG)
    {
        lamp_date.time_second_stamp_cnt++;
        lamp_date.date_second_stamp_cnt++;
        lamp_date.data_sync_second_cnt++;
        if (LAMP_DATE_DAY_SECOND_COUNT <= lamp_date.time_second_stamp_cnt)
        {
            lamp_date.time_second_stamp_cnt = 0;
        }
    }
    lamp_date.runtime++;
}

/**
 * @brief  时间校准回调函数
 * @param  date_time_stamp - 时间戳
 * @return none
 */
void lc_iot_calibrate_date_cb_fun(unsigned int date_time_stamp)
{
	if(re_sync_date)
	{
		if(lamp_date.date_check != LAMP_DATE_HAVE_SYNC_FLG)
			lamp_date.data_sync_second_cnt = 0;
		
	    lamp_date.date_second_stamp_cnt = date_time_stamp;
	    lamp_date.time_second_stamp_cnt = date_time_stamp - ((date_time_stamp / LAMP_DATE_DAY_SECOND_COUNT) * LAMP_DATE_DAY_SECOND_COUNT);
	    lamp_date.date_check = LAMP_DATE_HAVE_SYNC_FLG;
		re_sync_date = false;
	}
}
/**
 * @brief  时间校准参数初始化
 * @param  none
 * @return none
 */
void lamp_date_init()
{
    memset(&lamp_date, 0, sizeof(lamp_date_param_t));
    lamp_date.date_check = LAMP_DATE_NOT_SYNC_FLG;
	re_sync_date = true;
}

/**
 * @brief  灯模块万年历时间更新
 * @param  none
 * @return none
 */
void lamp_date_task()
{
    static unsigned int ref = 0;
    if (LcTimeExceed(ref, 1000))
    {
        ref += 1000;
        lamp_date_timer_second_cb_fun();
    }
	
    static unsigned int ref_sync = 0;
    if (LcTimeExceed(ref_sync, RE_SYNC_DATE_TIME_MS))
    {
        ref_sync += RE_SYNC_DATE_TIME_MS;
		
		re_sync_date = true;
    }
}



#include "system_inc.h"
//#include "../dev/fram/fram.h"
//#include "../app/storage.h"
#include "ALERT.h"


//ALERT_INFO  g_alert_buf[ALERT_BUFFER_SIZE];

extern GLOBAL_CFG_PARA g_PlmConfigPara;


U8 alert_flag = 0;
U16 alert_times = 0;

U16 alert_sn;
U32 alert_num;

void Alert(U16 alert_type, U8 alert_act, S8 * file_name, U32 line_num)
{	
#if 0
    //unsigned short alert_bak;
    ALERT_INFO  * p_alert_buf = &g_alert_buf[alert_sn];
    
	p_alert_buf->alert_type = alert_type;
    p_alert_buf->alert_flag = ALERT_YES_ALERT;
    p_alert_buf->alert_type |= (((U32)alert_act) << 24);
	p_alert_buf->line_num = line_num;
    
	if(strlen_intsafe((char *)file_name) >= ALERT_FILE_NAME_SIZE)
	{
            memcpy_intsafe((char *)p_alert_buf->file_name, file_name, ALERT_FILE_NAME_SIZE);
	}
	else
	{
	    strcpy_intsafe((char *)p_alert_buf->file_name, (char *)file_name);
		
	}

    cctt_read_rtc_sys_time((CCTT_SYS_TIME *)p_alert_buf->occur_time);

    //alert_bak = alert_sn;

    alert_flag = 1;

    alert_sn++;

    alert_num++;

    if(alert_sn >= ALERT_BUFFER_SIZE)
        alert_sn = 0;

    //dc_flush_fram(&alert_sn, 6, FRAM_FLUSH );
    
    //dc_flush_fram(&g_alert_buf[alert_bak], sizeof(ALERT_INFO), FRAM_FLUSH );

    if(ALERT_RESET_DEVICE == alert_act)
    {
        g_PlmConfigPara.sysAlertRebootCount++;
        dc_flush_fram(&g_PlmConfigPara.sysAlertRebootCount, 4, FLASH_FLUSH );
        Reboot_system(6, en_reboot_alert);
    }
    else if(ALERT_ERROR_TIMES == alert_act)
    {
        alert_times++;
        if(alert_times > ALERT_RESET_ERROR_TIMES)
        {
            Reboot_system(6, en_reboot_timeout);
        }
    }
#endif
}

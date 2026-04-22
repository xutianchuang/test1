#include "ble_update.h"
#include "ble_api.h"
#include "lc_whi_pro.h"
#include "lc_all.h"
#include "ble_event.h"

#define LAMP_BLE_POWER_UP_STATUS (0)
#define LAMP_BLE_INIT_STATUS (1)
#define LAMP_BLE_RUN_STATUS (2)

#define LAMP_BLE_POWER_DISACTION_VALUE (0xFF)

#define LAMP_BLE_WAITE_CONNECT_TIME_MS (20000)      // 20秒
#define LAMP_BLE_HEART_SOLT_CONNECT_TIME_MS (60000) // 90秒
#define LAMP_BLE_WAITE_VERSION_TIME_MS (2000)       // 2秒


lamp_ble_param_t lamp_ble_param;

#define LAMP_BLE_UPDATA_FILE_POWER_UP_ACTION_CHECK (0)
#define LAMP_BLE_UPDATA_FILE_POWER_UP_DISACTION_CHECK (0x11223344)

// 蓝牙模块升级
#define LAMP_BLE_UPDATA_SEGMENT_SIZE (256)
#define LAMP_BLE_UPDATA_FLG_VALE (0x1122)
#define LAMP_BLE_UPDATA_WAITE_MAX_MS (4000)                // ms
#define LAMP_BLE_UPDATA_RESEND_LOCAL_MS (10 * 60 * 1000UL) // ms
#define LAMP_BLE_UPDATA_WAITE_FRAME_SOLT_MS (2)            // ms
enum
{
    LAMP_BLE_UPDATA_PAUSE,
    LAMP_BLE_UPDATA_SEND_CLEAR,
    LAMP_BLE_UPDATA_SEND_CLEAR_WAITE_ACK,
    LAMP_BLE_UPDATA_SEND_START,
    LAMP_BLE_UPDATA_SEND_START_WAITE_ACK,
    LAMP_BLE_UPDATA_SEND_FILE_SEGMENT,
    LAMP_BLE_UPDATA_SEND_FILE_SEGMENT_WAITE_ACK,
    LAMP_BLE_UPDATA_LOCAL_RESEND_PAUSE,
};

typedef struct
{
    uint16_t updata_flg;

    uint16_t updata_status;
    uint16_t updata_local_seq;

    uint16_t updata_local_seq_ack_flg;

    uint16_t updata_segment_idx;
    uint16_t updata_segment_sum;
    uint16_t updata_segment_size;

    //
    uint16_t updata_cb_status;
    //
    uint32_t updata_file_size;
    uint32_t updata_file_crc;
    //
    TimeValue updata_timer;
    //
} lamp_ble_updata_param_type;

lamp_ble_updata_param_type lamp_ble_updata_param;



//=======================
// 检测存储文件是否合法
extern uint32_t GetUpdateFileAddr(void);
#define LAMP_BLE_UPDATA_CHECK_BUFF_ZISE (256)
uint32_t lamp_ble_updata_check_store_file()
{
    uint32_t r_flg = 0;
    uint32_t fw_magic;  
    uint32_t fw_size;
    uint32_t fileAddr = GetUpdateFileAddr();

    if(fileAddr == 0) return r_flg;

    // lc_net_read_file_data((0x8), (u8 *)&fw_magic, 4); /// 0x8 store bin magic value  
    // lc_net_read_file_data((0x18), (u8 *)&fw_size, 4); /// 0x18 store bin size value    
    ReadDataFlash(fileAddr + (0x8), (u8 *)&fw_magic, 4);   
    ReadDataFlash(fileAddr + (0x18), (u8 *)&fw_size, 4); 

    if (fw_magic == 0x544c4e4b && fw_size <= 0x30000 && fw_size >= 0x100)
    {

        uint8_t *check_buff = malloc(LAMP_BLE_UPDATA_CHECK_BUFF_ZISE + 32);
        //
        if (check_buff != NULL)
        {
            uint32_t l_len = fw_size - 4;
            uint32_t start_addr = fileAddr;
            uint32_t crc32_value = CRC32_INIT_VAL;
            uint32_t file_crc32_value = 0;
            do
            {
                uint32_t read_len = l_len > LAMP_BLE_UPDATA_CHECK_BUFF_ZISE ? LAMP_BLE_UPDATA_CHECK_BUFF_ZISE : l_len;
                ReadDataFlash(start_addr, check_buff, read_len);
                crc32_value = crc32_cyc_cal(crc32_value, check_buff, read_len);

                start_addr += read_len;
                l_len -= read_len;

            } while (l_len != 0);

            // lc_net_read_file_data(fw_size - 4, (uint8_t *)&file_crc32_value, 4);
            ReadDataFlash(fileAddr + (fw_size - 4), (uint8_t *)&file_crc32_value, 4);

            if (file_crc32_value == crc32_value)
            {
                lamp_ble_updata_param.updata_file_size = fw_size;
                lamp_ble_updata_param.updata_file_crc = crc32_value;
                lamp_ble_updata_param.updata_segment_sum = (fw_size + LAMP_BLE_UPDATA_SEGMENT_SIZE - 1) / LAMP_BLE_UPDATA_SEGMENT_SIZE;
                lamp_ble_updata_param.updata_segment_size = LAMP_BLE_UPDATA_SEGMENT_SIZE;
                lamp_ble_updata_param.updata_segment_idx = 0;
                r_flg = 1;
            }
            free(check_buff);
        }
    }
    return r_flg;
}

// 远程传输升级文件状态变更
void lamp_ble_updata(lc_file_state_type c_status)
{
    if (lamp_ble_updata_param.updata_cb_status != c_status)
    {
        lamp_ble_updata_param.updata_cb_status = c_status;
        lamp_ble_updata_param.updata_status = LAMP_BLE_UPDATA_PAUSE;
        lamp_ble_param.updata_status = 0;
    }
}

// 升级下发帧应答处理
bool lamp_ble_updata_send_ack_chekc_frame(uint32_t seq, uint32_t status)
{
    bool r_flg = false;
    if (status == WHI_ACK_EOK)
    {
        if (seq == lamp_ble_updata_param.updata_local_seq)
        {
            r_flg = true;
            lamp_ble_updata_param.updata_local_seq_ack_flg = 0xFF;
        }
    }
    return r_flg;
}

// 发送清除帧
bool lamp_ble_updata_send_clear_frame()
{
    bool r_flg = false;
    uint8_t *send_buff = malloc(WHI_PRO_HEAD_SIZE + 32);
    if (send_buff != NULL)
    {
        whi_file_updata_start_frame_type *p_updata_fram = (whi_file_updata_start_frame_type *)(&send_buff[WHI_PRO_STAT_HEAD_SIZE]);
        memset(p_updata_fram, 0, sizeof(whi_file_updata_start_frame_type));
        p_updata_fram->fun_cmd = WHI_START_TRAN_FILE;
        p_updata_fram->file_attr = 0; // clear file
        //
        lamp_ble_updata_param.updata_local_seq = WHI_CreateLocalSeq();
        //
        uint16_t send_len = WHI_BuildFrameProcess(send_buff, false, WHI_FILE_CMD, lamp_ble_updata_param.updata_local_seq, (uint8_t *)p_updata_fram, sizeof(whi_file_updata_start_frame_type));
        r_flg = whi_pro_local_add_uart_task(send_buff, send_len, SEND_LOCAL_CFG_FRAME_PRIO_VALUE, 2);
        free(send_buff);
    }
    return r_flg;
}

// 发送启动帧
bool lamp_ble_updata_send_start_frame()
{
    bool r_flg = false;
    uint8_t *send_buff = malloc(WHI_PRO_HEAD_SIZE + 32);
    if (send_buff != NULL)
    {
        whi_file_updata_start_frame_type *p_updata_fram = (whi_file_updata_start_frame_type *)(&send_buff[WHI_PRO_STAT_HEAD_SIZE]);
        memset(p_updata_fram, 0, sizeof(whi_file_updata_start_frame_type));
        p_updata_fram->fun_cmd = WHI_START_TRAN_FILE;
        p_updata_fram->file_attr = 1; // local file
        p_updata_fram->file_len = lamp_ble_updata_param.updata_file_size;
        p_updata_fram->file_crc = lamp_ble_updata_param.updata_file_crc;
        p_updata_fram->segment_total = lamp_ble_updata_param.updata_segment_sum;
        p_updata_fram->trans_timeout = 60;
        //
        lamp_ble_updata_param.updata_local_seq = WHI_CreateLocalSeq();
        //
        uint16_t send_len = WHI_BuildFrameProcess(send_buff, false, WHI_FILE_CMD, lamp_ble_updata_param.updata_local_seq, (uint8_t *)p_updata_fram, sizeof(whi_file_updata_start_frame_type));
        r_flg = whi_pro_local_add_uart_task(send_buff, send_len, SEND_LOCAL_CFG_FRAME_PRIO_VALUE, 2);
        free(send_buff);
    }
    return r_flg;
}

// 发送文件数据帧
bool lamp_ble_updata_send_file_segment_frame(lamp_ble_updata_param_type *p_lamp_ble_updata_param, uint16_t *p_updata_local_seq)
{
    bool r_flg = false;
    uint32_t file_addr = GetUpdateFileAddr();
    //
    if (p_lamp_ble_updata_param->updata_segment_idx >= p_lamp_ble_updata_param->updata_segment_sum)
        return false;
    //
    uint8_t *send_buff = malloc(WHI_PRO_HEAD_SIZE + 32 + p_lamp_ble_updata_param->updata_segment_size);
    if (send_buff != NULL)
    {
        whi_file_updata_file_segment_frame_type *p_updata_fram = (whi_file_updata_file_segment_frame_type *)(&send_buff[WHI_PRO_STAT_HEAD_SIZE]);
        memset(p_updata_fram, 0, sizeof(whi_file_updata_file_segment_frame_type));
        p_updata_fram->fun_cmd = WHI_TRAN_FILE_DATA;
        p_updata_fram->rsv = 0; // local file
        p_updata_fram->segment_num = p_lamp_ble_updata_param->updata_segment_idx;
        //
        uint32_t start_addr = p_lamp_ble_updata_param->updata_segment_idx * (p_lamp_ble_updata_param->updata_segment_size);
        uint32_t l_data_len = p_lamp_ble_updata_param->updata_file_size - start_addr;
        l_data_len = l_data_len < p_lamp_ble_updata_param->updata_segment_size ? l_data_len : p_lamp_ble_updata_param->updata_segment_size;

        ReadDataFlash(file_addr + start_addr, p_updata_fram->segment_data, l_data_len);
        // lc_net_read_file_data

        p_updata_fram->segment_size = l_data_len;
        p_updata_fram->segment_crc = 0xFFFF; // crc32_cal(p_updata_fram->segment_data,p_updata_fram->segment_size);

        uint16_t l_seq = WHI_CreateLocalSeq();
        *p_updata_local_seq = l_seq;
        //
        uint16_t send_len = WHI_BuildFrameProcess(send_buff, false, WHI_FILE_CMD, l_seq, (uint8_t *)p_updata_fram, sizeof(whi_file_updata_file_segment_frame_type) + p_updata_fram->segment_size);
        r_flg = whi_pro_local_add_uart_task(send_buff, send_len, SEND_LOCAL_CFG_FRAME_PRIO_VALUE, 2);
        free(send_buff);
    }
    return r_flg;
}

// 升级初始化
void lamp_ble_updata_init()
{
    memset(&lamp_ble_updata_param, 0, sizeof(lamp_ble_updata_param));

    lamp_ble_updata_param.updata_cb_status = LC_FILE_UNKNOW;

    lamp_ble_updata_param.updata_status = LAMP_BLE_UPDATA_PAUSE;
    lamp_ble_updata_param.updata_flg = LAMP_BLE_UPDATA_FLG_VALE;
    //
    lc_file_set_states_cbfun(lamp_ble_updata, lamp_ble_updata_check_store_file);
}

// 升级任务
void lamp_ble_updata_task()
{
    if (lamp_ble_updata_param.updata_flg == LAMP_BLE_UPDATA_FLG_VALE)
    {
        switch (lamp_ble_updata_param.updata_status)
        {
        case LAMP_BLE_UPDATA_PAUSE:
        {
            if ((lamp_ble_updata_param.updata_cb_status == LC_FILE_UNKNOW) || (lamp_ble_updata_param.updata_cb_status == LC_FILE_FINISH))
            {
                if (1 == lamp_ble_updata_check_store_file())
                {
                	debug_str(DEBUG_LOG_UPDATA, "lamp_ble_updata_check_store_file ok\r\n");
                    lamp_ble_updata_param.updata_status = LAMP_BLE_UPDATA_SEND_CLEAR;
                    lamp_ble_updata_param.updata_cb_status = LC_FILE_FINISH;
                    StartTimer(&lamp_ble_updata_param.updata_timer, LAMP_BLE_UPDATA_WAITE_FRAME_SOLT_MS);
                }
                else
                {
                	debug_str(DEBUG_LOG_UPDATA, "lamp_ble_updata_check_store_file fail\r\n");
                    lamp_ble_updata_param.updata_status = LAMP_BLE_UPDATA_PAUSE;
                    lamp_ble_updata_param.updata_cb_status = LC_FILE_WRITING;
                    lamp_ble_param.updata_status = 0;
                }
            }
        }
        break;
        case LAMP_BLE_UPDATA_SEND_CLEAR:
        {
            if (TRUE == LcTimeOut(&lamp_ble_updata_param.updata_timer))
            {
            	debug_str(DEBUG_LOG_UPDATA, "LAMP_BLE_UPDATA_SEND_CLEAR\r\n");
                lamp_ble_updata_param.updata_local_seq_ack_flg = 0;
                if (lamp_ble_updata_send_clear_frame())
                {
                    lamp_ble_updata_param.updata_status = LAMP_BLE_UPDATA_SEND_CLEAR_WAITE_ACK;
                }
                StartTimer(&lamp_ble_updata_param.updata_timer, LAMP_BLE_UPDATA_WAITE_MAX_MS);
            }
        }
        break;
        case LAMP_BLE_UPDATA_SEND_START:
        {
            if (TRUE == LcTimeOut(&lamp_ble_updata_param.updata_timer))
            {
            	debug_str(DEBUG_LOG_UPDATA, "LAMP_BLE_UPDATA_SEND_START\r\n");
                lamp_ble_updata_param.updata_local_seq_ack_flg = 0;
                if (lamp_ble_updata_send_start_frame())
                {
                    lamp_ble_updata_param.updata_status = LAMP_BLE_UPDATA_SEND_START_WAITE_ACK;
                }
                StartTimer(&lamp_ble_updata_param.updata_timer, LAMP_BLE_UPDATA_WAITE_MAX_MS);
            }
        }
        break;
        case LAMP_BLE_UPDATA_SEND_FILE_SEGMENT:
        {
            if (TRUE == LcTimeOut(&(lamp_ble_updata_param.updata_timer)))
            {
            	debug_str(DEBUG_LOG_UPDATA, "LAMP_BLE_UPDATA_SEND_FILE_SEGMENT %d\r\n",lamp_ble_updata_param.updata_segment_idx);
                if (lamp_ble_updata_param.updata_segment_idx >= lamp_ble_updata_param.updata_segment_sum)
                { // 发送完成
                	debug_str(DEBUG_LOG_UPDATA, "LAMP_BLE_UPDATA_SEND_FILE_SEGMENT  ok\r\n");
                    lamp_ble_updata_param.updata_status = LAMP_BLE_UPDATA_PAUSE;
                    lamp_ble_updata_param.updata_cb_status = LC_FILE_WRITING;
                    lamp_ble_param.updata_status = 0;
                }
                else
                {
                    lamp_ble_updata_param.updata_local_seq_ack_flg = 0;
                    if (lamp_ble_updata_send_file_segment_frame(&lamp_ble_updata_param, &(lamp_ble_updata_param.updata_local_seq)))
                    {
                        lamp_ble_updata_param.updata_segment_idx++;
                        lamp_ble_updata_param.updata_status = LAMP_BLE_UPDATA_SEND_FILE_SEGMENT_WAITE_ACK;
                    }
                    StartTimer(&lamp_ble_updata_param.updata_timer, LAMP_BLE_UPDATA_WAITE_MAX_MS);
                }
            }
        }
        break;
        case LAMP_BLE_UPDATA_SEND_CLEAR_WAITE_ACK:
        case LAMP_BLE_UPDATA_SEND_START_WAITE_ACK:
        case LAMP_BLE_UPDATA_SEND_FILE_SEGMENT_WAITE_ACK:
        {
            if (lamp_ble_updata_param.updata_local_seq_ack_flg == 0)
            {
                if (TRUE == LcTimeOut(&lamp_ble_updata_param.updata_timer))
                {
                    lamp_ble_updata_param.updata_status = LAMP_BLE_UPDATA_LOCAL_RESEND_PAUSE;
                    lamp_ble_updata_param.updata_cb_status = LC_FILE_FINISH;
                    lamp_ble_param.updata_status = 1; // 升级失败
                    StartTimer(&lamp_ble_updata_param.updata_timer, LAMP_BLE_UPDATA_RESEND_LOCAL_MS);
                }
            }
            else
            {
                if (lamp_ble_updata_param.updata_status == LAMP_BLE_UPDATA_SEND_CLEAR_WAITE_ACK)
                {
                    lamp_ble_updata_param.updata_status = LAMP_BLE_UPDATA_SEND_START;
                }
                else
                {
                    lamp_ble_updata_param.updata_status = LAMP_BLE_UPDATA_SEND_FILE_SEGMENT;
                }
                StartTimer(&lamp_ble_updata_param.updata_timer, LAMP_BLE_UPDATA_WAITE_FRAME_SOLT_MS);
            }
        }
        break;
        case LAMP_BLE_UPDATA_LOCAL_RESEND_PAUSE:
            if (TRUE == LcTimeOut(&lamp_ble_updata_param.updata_timer))
            {
                lamp_ble_updata_param.updata_status = LAMP_BLE_UPDATA_PAUSE;
                lamp_ble_updata_param.updata_cb_status = LC_FILE_FINISH;
                //local_debug_cnt.ble_updata_file_resend_cnt++;
            }
            break;
        default:
            break;
        }
    }
}

// 配置蓝牙功率
void lamp_ble_power_set(uint8_t ble_power)
{
    uint8_t send_buff[WHI_PRO_HEAD_SIZE + sizeof(local_ble_param_t)];
    local_ble_param_t param = {0};
    param.power = ble_power;

    uint16_t send_len = WHI_BuildFrameProcess(send_buff, false, WHI_LAMP_BLE_WRITE_POWER_VALUE, WHI_CreateLocalSeq(), (uint8_t *)&param, sizeof(local_ble_param_t));
	whi_pro_local_add_uart_task(send_buff, send_len, SEND_LOCAL_CFG_FRAME_PRIO_VALUE, 3);
}

// 配置雷达参数
/*void lamp_ble_radar_param_set(uint8_t freq, uint16_t thr, uint16_t scale)
{
    uint8_t send_buff[WHI_PRO_HEAD_SIZE + sizeof(local_radar_param_t)];
    local_radar_param_t param = {0};
    param.freq = freq;
    param.thr = thr;
    param.energy_scale = scale;

    uint16_t send_len = WHI_BuildFrameProcess(send_buff, false, WHI_LAMP_RADAR_WRITE_PARAM_VALUE, WHI_CreateLocalSeq(), (uint8_t *)&param, sizeof(local_radar_param_t));
	whi_pro_local_add_uart_task(send_buff, send_len, SEND_LOCAL_CFG_FRAME_PRIO_VALUE, 3);
}

// 读雷达参数
void lamp_ble_radar_param_read()
{
    uint8_t *send_buff =malloc(WHI_PRO_HEAD_SIZE + 32);
    if (send_buff != NULL)
    {
        uint16_t send_len = WHI_BuildFrameProcess(send_buff, false, WHI_LAMP_RADAR_READ_PARAM_VALUE, WHI_CreateLocalSeq(), NULL, 0);
        bool send_flg = whi_pro_local_add_uart_task(send_buff, send_len, SEND_LOCAL_READ_VERSION_PRIO_VALUE, 1);
        free(send_buff);
    }
}

// 读雷达参数回调
bool lamp_ble_radar_param_version_cb_fun(uint16_t ack_seq, uint8_t result, uint8_t *data, uint16_t len)
{
    bool r_flg = false;

    if (len >= sizeof(local_radar_param_t))
    {
        r_flg = lamp_radar_self_check_read_callback(data);    
    }
    return r_flg;
      return 0;
}*/


// 读取版本号
void lamp_ble_start_read_version()
{
    uint8_t *send_buff = malloc(WHI_PRO_HEAD_SIZE + 32);
    if (send_buff != NULL)
    {
        uint16_t send_len = WHI_BuildFrameProcess(send_buff, false, WHI_READ_VERSION, WHI_CreateLocalSeq(), NULL, 0);
        bool send_flg = whi_pro_local_add_uart_task(send_buff, send_len, SEND_LOCAL_READ_VERSION_PRIO_VALUE, 1);
        free(send_buff);
        if (send_flg)
        {
            StartTimer(&(lamp_ble_param.ble_read_version_connect_timer), LAMP_BLE_WAITE_VERSION_TIME_MS);
        }
    }
    memset(&lamp_ble_param.ble_ver_value, 0, sizeof(lamp_ble_param.ble_ver_value));
}

// 读取设备类型
void lamp_ble_start_read_device_type()
{
    uint8_t *send_buff = malloc(WHI_PRO_HEAD_SIZE + 32);
    if (send_buff != NULL)
    {
        uint16_t send_len = WHI_BuildFrameProcess(send_buff, false, WHI_READ_DEVICE_TYPE, WHI_CreateLocalSeq(), NULL, 0);
        bool send_flg = whi_pro_local_add_uart_task(send_buff, send_len, SEND_LOCAL_READ_VERSION_PRIO_VALUE, 1);
        free(send_buff);
    }
    memset(&lamp_ble_param.device_type, 0, LAMP_BLE_DEVICE_TYPE);
}

// 读取设备类型回调
bool lamp_ble_device_type_info_cb_fun(uint16_t ack_seq, uint8_t result, uint8_t *info, uint16_t info_len)
{
    bool r_flg = false;

    if (info_len > 0)
    {
        memcpy((uint8_t *)&lamp_ble_param.device_type, info, info_len > LAMP_BLE_DEVICE_TYPE ? LAMP_BLE_DEVICE_TYPE : info_len);
        r_flg = true;
    }
    return r_flg;
}

// 读取版本号回调
bool lamp_ble_start_read_version_cb_fun(uint16_t ack_seq, uint8_t result, uint8_t *version, uint16_t ver_len)
{
    bool r_flg = false;

    if (ver_len > 0)
    {
        TimeStop(&(lamp_ble_param.ble_read_version_connect_timer));
        lamp_ble_param.ble_connect_status = LAMP_BLE_CONNECT_SUCCESS_STATUS;
        memcpy(&lamp_ble_param.ble_ver_value, version, sizeof(lamp_ble_param.ble_ver_value));

        if (lamp_ble_param.ble_ver_value.chip_type != 0x8258)
        {
            lamp_event_add(EVENT_BLE_VER_ERR, NULL, -1);   
            //led_pwm_ctrl_enable(0);
        }

        r_flg = true;
    }
    return r_flg;
}

// 读取复位信息
void lamp_ble_start_read_reset_info()
{
    uint8_t *send_buff = malloc(WHI_PRO_HEAD_SIZE + 32);
    if (send_buff != NULL)
    {
        uint16_t send_len = WHI_BuildFrameProcess(send_buff, false, WHI_READ_RESET_INFO, WHI_CreateLocalSeq(), NULL, 0);
        bool send_flg = whi_pro_local_add_uart_task(send_buff, send_len, SEND_LOCAL_READ_RESET_INFO_PRIO_VALUE, 1);
        free(send_buff);
    }
    memset(lamp_ble_param.ble_sys_reset_info, 0, LAMP_BLE_RESET_INFO_LEN);
}

// 读取复位信息回调
bool lamp_ble_start_read_reset_info_cb_fun(uint16_t ack_seq, uint8_t result, uint8_t *info, uint16_t info_len)
{
    bool r_flg = false;

    if (info_len > 0)
    {
        memcpy(lamp_ble_param.ble_sys_reset_info, info, info_len > LAMP_BLE_RESET_INFO_LEN ? LAMP_BLE_RESET_INFO_LEN : info_len);
        r_flg = true;
    }
    return r_flg;
}

// 设置mac地址
bool lamp_ble_set_mac_addr(uint8_t *mac_addr)
{
    bool r_flg = true;

    uint8_t *send_buff = malloc(WHI_PRO_HEAD_SIZE + 32);
    if (send_buff != NULL)
    {
        lamp_ble_param.set_mac_seq = WHI_CreateLocalSeq();
        uint16_t send_len = WHI_BuildFrameProcess(send_buff, false, WHI_SET_ADDR, lamp_ble_param.set_mac_seq, mac_addr, LC_NET_ADDR_SIZE);
        r_flg = whi_pro_local_add_uart_task(send_buff, send_len, SEND_LOCAL_CFG_FRAME_PRIO_VALUE, 1);
        free(send_buff);

        if (r_flg)
        {
            StartTimer(&(lamp_ble_param.ble_check_connect_timer), LAMP_BLE_WAITE_CONNECT_TIME_MS);
        }
    }
    else
    {
        r_flg = false;
    }
    
    return r_flg;
}

// 设置mac地址回调
bool lamp_ble_set_mac_addr_cb_fun(uint16_t ack_seq, uint8_t result)
{
    bool r_flg = false;

    if ((result == WHI_ACK_EOK) && (lamp_ble_param.set_mac_seq == ack_seq))
    {
        TimeStop(&(lamp_ble_param.ble_check_connect_timer));
        r_flg = true;
    }
    return r_flg;
}

void lamp_ble_init()
{
    memset(&lamp_ble_param, 0, sizeof(lamp_ble_param));
    lamp_ble_param.ble_connect_status = LAMP_BLE_CONNECT_ERROR_STATUS;
    lamp_ble_param.ble_status = LAMP_BLE_POWER_UP_STATUS;

    lamp_ble_updata_init();
}

void lamp_ble_task()
{
    switch (lamp_ble_param.ble_status)
    {
    case LAMP_BLE_POWER_UP_STATUS:
    {
        lc_net_get_factory_mac(lamp_ble_param.ble_mac_addr);
        lamp_ble_set_mac_addr(lamp_ble_param.ble_mac_addr);

        lamp_ble_start_read_version();
		lamp_ble_start_read_device_type();
        lamp_ble_start_read_reset_info();

        lamp_ble_power_set(6);  //默认配置为6
        //lamp_ble_radar_param_set(light_param.radar_freq, light_param.radar_threshold, light_param.radar_energy_scale); $$$先不设置

        lamp_ble_param.ble_status = LAMP_BLE_INIT_STATUS;
    }
    break;
    case LAMP_BLE_INIT_STATUS:
    { 
        if (LAMP_BLE_CONNECT_ERROR_STATUS != lamp_ble_param.ble_connect_status)
        {
        	debug_str(DEBUG_LOG_UPDATA, "LAMP_BLE_INIT_STATUS ok\r\n");
            lamp_ble_param.ble_status = LAMP_BLE_RUN_STATUS;
            StartTimer(&(lamp_ble_param.ble_heart_connect_timer), LAMP_BLE_HEART_SOLT_CONNECT_TIME_MS);
        }
        else    /*未收到版本回调，状态一直为 LAMP_BLE_CONNECT_ERROR_STATUS 超时上报连接失败事件*/
        {
        	//存在升级第一次重启后，可能出现第一次读不到版本的情况，理应多次读取，不能读不到就不往下走了
            if (TRUE == LcTimeOut(&(lamp_ble_param.ble_read_version_connect_timer)))
            {
            	debug_str(DEBUG_LOG_UPDATA, "LAMP_BLE_INIT_STATUS fail\r\n");
                lamp_event_add(EVENT_BLE_CONN_FAIL, NULL, -1);   
				lamp_ble_start_read_version();
	            StartTimer(&(lamp_ble_param.ble_read_version_connect_timer), LAMP_BLE_WAITE_VERSION_TIME_MS);
            }
        }
    }
    break;
    case LAMP_BLE_RUN_STATUS:
    {
        lamp_ble_updata_task();
		if (TRUE == LcTimeOut(&(lamp_ble_param.ble_heart_connect_timer)))
        {
        	lamp_ble_start_read_version(); 
            lamp_ble_start_read_reset_info();
            lamp_ble_power_set(6);
			StartTimer(&(lamp_ble_param.ble_heart_connect_timer), LAMP_BLE_HEART_SOLT_CONNECT_TIME_MS);
		}
    }
    break;
    default:
        break;
    }
}
#define CMD_READ_BLE_FACTORY_TEST_PARAM     (0xC00A)
#define CMD_SET_BLE_FACTORY_TEST_PARAM      (0xC009)

factory_ble_test_t factory_ble;

// 读产测蓝牙信号测试参数
void lamp_ble_factory_test_param_read()
{
	debug_str(DEBUG_LOG_UPDATA, "lamp_ble_factory_test_param_read\r\n");
    uint8_t *send_buff = malloc(WHI_PRO_HEAD_SIZE + 32);
    if (send_buff != NULL)
    {
        uint16_t send_len = WHI_BuildFrameProcess(send_buff, false, CMD_READ_BLE_FACTORY_TEST_PARAM, WHI_CreateLocalSeq(), NULL, 0);
        bool send_flg = whi_pro_local_add_uart_task(send_buff, send_len, SEND_LOCAL_TOOL_TEST_READ_RSSI_PRIO_VALUE, 0);
        free(send_buff);
    }
}
uint8_t test_param_version_cb_flag = 0;
// 配置产测蓝牙信号测试参数
void lamp_ble_factory_test_param_set(uint8_t *p)
{
	debug_str(DEBUG_LOG_UPDATA, "lamp_ble_factory_test_param_set\r\n");
    uint8_t send_buff[WHI_PRO_HEAD_SIZE + sizeof(local_ble_factory_test_param_t)];
    local_ble_factory_test_param_t param = {0};
    memcpy(&param.ble, p, sizeof(factory_ble_test_t));

    uint16_t send_len = WHI_BuildFrameProcess(send_buff, false, CMD_SET_BLE_FACTORY_TEST_PARAM, WHI_CreateLocalSeq(), (uint8_t *)&param, sizeof(local_ble_factory_test_param_t));
	whi_pro_local_add_uart_task(send_buff, send_len, SEND_LOCAL_TOOL_TEST_SET_RSSI_PRIO_VALUE, 0);
	test_param_version_cb_flag = 0;
	//写入后，需要读取，先进行，避免抄控器读取时没数据
	lamp_ble_factory_test_param_read();
}

// 读产测蓝牙信号测试参数回调
bool lamp_ble_factory_test_param_version_cb_fun(uint16_t ack_seq, uint8_t result, uint8_t *data, uint16_t len)
{
    bool r_flg = false;
	debug_str(DEBUG_LOG_UPDATA, "lamp_ble_factory_test_param_version_cb_fun\r\n");

    if (len >= sizeof(local_ble_factory_test_param_t))
    {
    	debug_str(DEBUG_LOG_UPDATA, "lamp_ble_factory_test_param_version_cb_fun ok\r\n");
    	test_param_version_cb_flag = 1;
        local_ble_factory_test_param_t *param = (local_ble_factory_test_param_t *)data;
        memcpy((uint8_t *)&factory_ble, (uint8_t *)&param->ble, sizeof(factory_ble_test_t));
        r_flg = true;
    }
    return r_flg;
}

// 接收成品产测蓝牙信号测试参数
bool WHI_ReadBleFactoryTestParamAckProcess(uint8_t *psrc,uint16_t dlen)
{
	debug_str(DEBUG_LOG_UPDATA, "WHI_ReadBleFactoryTestParamAckProcess\r\n");
    uint16_t r_flg = false;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	
	if(framelen > 0)
	{
		r_flg = lamp_ble_factory_test_param_version_cb_fun(frameseq,WHI_ACK_EOK,pdst,framelen);
	}
	return r_flg;
}

bool WHI_SetRadarFactoryTestParamAckProcess(uint8_t *psrc,uint16_t dlen)
{
	debug_str(DEBUG_LOG_UPDATA, "WHI_SetRadarFactoryTestParamAckProcess\r\n");
	return true;
}



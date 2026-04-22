#include <string.h>
#include "common_includes.h"
#include "lc_whi_pro.h"
#include "lc_whi_adapt.h"
#include "../../lc_iot/sta/ble_net/ble_api.h"
#include "ble_data.h"
#include "crclib.h"
#include "ble_net.h"
#include "lc_app.h"
#include "ble_local.h"
#include "ble_mac.h"
#include "ble_apl.h"
#include "ble_update.h"
#include "lc_whi_sysctrl.h"
#include "hplc_mac.h"
#include "ble_net.h"
#include "lamp_date.h"
#include "ble_event.h"

//重新发送广播帧超时定时器
static TimeValue ResendBroadcastFrameTimer;
#define RESEND_BROADCAST_FRAME_CHECK_TIME_MS		(2 * 1000UL)
#define RESEND_BROADCAST_FRAME_SAVE_TIME_MS			(2 * 60 * 1000UL) //5 minutes
#define MAX_BROADCAST_SAVED_FRAME_NUM 				(5)
typedef struct
{
	uint32_t broadcastSeq;
	uint16_t sendLen;
	uint8_t *sendBuff;
	TimeValue savedTimer;
}whi_broadcast_saved_frame;

whi_broadcast_saved_frame broadcastSavedFrame[MAX_BROADCAST_SAVED_FRAME_NUM] = {0};
int broadcastSavedFrameCount = 0;
int broadcastFrameSavedIndex = -1;


static TimeValue UpdateFinishTimer;
#define UPDATE_FINISH_TIME_MS		(5 * 1000UL)

void StartUpdateFinishTimerToResetSystem(u32 willResetInMs)
{
	StartTimer(&UpdateFinishTimer, (willResetInMs == 0) ? UPDATE_FINISH_TIME_MS : willResetInMs);
}

static TimeValue NeighborWatchingTimer;
#define NO_NEIGHBOR_TIME_MS		(60 * 60 * 1000UL)


//接收超时定时器
//#define LOCAL_UART_RECEIVE_WATCH_DOG_CHECK_TIME_MS		(10*60*1000UL)
//static TimeValue LocalUartReceiveWatchDogTimer;
/**
* @brief 全屋智能帧系号产生
* @param 
* @param 
* @return 帧系号
* @author hh
*/
uint16_t WHI_CreateLocalSeq()
{
	static uint16_t whi_local_seq = 0;
	++whi_local_seq;
	return whi_local_seq;
}

//====================
//本地串口任务管理模块
//====================
//local_debug_cnt_type local_debug_cnt;
//===================================
//local缺失函数移植
/* lc_net_err_t lc_net_set_rx_data_evnet_cb_fun(void (*lc_net_rx_data_cb)(const uint8_t* from_mac,const uint8_t *buff, const uint32_t len))
{
	lc_net_callback_fun_list.lc_net_rx_data_event_cb_fun = lc_net_rx_data_cb;
	return LC_NET_EOK;
}

void local_debug_uart_tx_data(uint8_t *ptr,uint16_t frame_len)
{
	if(frame_len < LOCAL_DEBUG_UART_RX_BUFF_SIZE)
	{
		memcpy(local_debug_uart_receive_process_buff,ptr,frame_len);
		hal_uart_write(LOCAL_DEBUG_UART,local_debug_uart_receive_process_buff,frame_len);
	}
 }*/

//串口状态
// static LOCAL_UART_STATE LocalUartState = LOCAL_IDLE;
//#define LOCAL_UART_TASK_LENGTH (16)
//local_task_param_t *LocalUartTaskParamPointList[LOCAL_UART_TASK_LENGTH];
//uint8_t LocalUartTaskParamPointListLen = 0;
//uint8_t CurrentLoaclUartTaskIndex = 0;



//本地串口发送任务接口
//static uint16_t lsn_cnt = 0;
#define LOCAL_UART_SEND_MAX_TIME_OUT_VALUE	(50)  //5秒
//tout 0.1s 
//whi_pro_local_add_uart_task(ptr,frame_len,SEND_LOCAL_CFG_FRAME_PRIO_VALUE,1);
// __WEAK bool local_send_fram_add_task(uint8_t *data,uint16_t len,uint16_t pri,uint16_t t_time,uint16_t resend,local_task_cb_fun_t cb_fun,void *cb_fun_arg)
// {
// 	return true;

// }


void lc_net_hplc_tx_sta_frame(uint8_t *dst_mac,uint8_t *src_data,uint16_t src_len)
{
	uint8_t* send_buff = malloc(WHI_HPLC_ADAPT_HEAD_LEN+src_len+32);
	if(send_buff == NULL)return;
	//
	uint16_t d_idx = WHI_HPLC_ADAPT_HEAD_START_SIZE;
	memcpy(&send_buff[d_idx],dst_mac,LC_NET_ADDR_SIZE);
	d_idx += LC_NET_ADDR_SIZE;

	send_buff[d_idx++] = src_len&0xFF;
	send_buff[d_idx++] = (src_len>>8)&0xFF;

	//
	memcpy(&send_buff[d_idx],src_data,src_len);
	//
	uint16_t send_len = whi_hplc_adapt_create(WHI_SEND_STA_FRAME,send_buff,&send_buff[WHI_HPLC_ADAPT_HEAD_START_SIZE],8+src_len);
	lc_net_tx_data(dst_mac,send_buff,send_len,LC_NET_NOT_BLOCK);
	free(send_buff);
}


/**
* @brief 全屋智能构建否认帧处理函数
* @param pdst 数据缓冲,ackflg应答标记，cmd命令，seq系号，psrc帧数据，dlen帧数据长度
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
uint16_t WHI_BuildFrameProcess(uint8_t *pdst,bool ackflg,uint16_t cmd,uint16_t seq,uint8_t *pdsrc,uint16_t dlen)
{
    uint8_t *pdstdata = pdst;
    uint16_t rlen = 0;
    if(dlen != 0)
    {
        if(&pdstdata[rlen + WHI_PRO_HEAD_FRONT_SIZE] != pdsrc)
        {
            memcpy(&pdstdata[rlen + WHI_PRO_HEAD_FRONT_SIZE],pdsrc,dlen);
        }
    }
    
    pdstdata[rlen++] = 0x48;
    pdstdata[rlen++] = (ackflg == true)?0x80:0xC0;

    pdstdata[rlen++] = cmd&0xFF;
    pdstdata[rlen++] = (cmd>>8)&0xFF;

    pdstdata[rlen++] = seq&0xFF;
    pdstdata[rlen++] = (seq>>8)&0xFF;

    pdstdata[rlen++] = dlen&0xFF;
    pdstdata[rlen++] = (dlen>>8)&0xFF;

    rlen += dlen;

    uint16_t crcvalue = crc16_cal(pdstdata, rlen);

    pdstdata[rlen++] = crcvalue&0xFF;
    pdstdata[rlen++] = (crcvalue>>8)&0xFF;

    return rlen;
}


/**
* @brief 全屋智能读取版本处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_READ_VERSION     (0x0001)
uint16_t WHI_ReadVersionProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];

//厂商代码（ 2bytes）
//芯片类型（ 2bytes） sta:8201H cco:8202H
//软件版本号（ 2bytes） BCD 格式
//保留（ 2bytes） 0000H
	
//获取版本配置
	lc_net_ver_info_t p_ver;
	memset(&p_ver,0,sizeof(lc_net_ver_info_t));
	lc_net_get_ver_info(&p_ver);
	
    pdst[rdlen++] = (p_ver.factory_code)&0xFF;
    pdst[rdlen++] = ((p_ver.factory_code)>>8)&0xFF;
    pdst[rdlen++] = 0x82;
//  #ifdef PLAT_LAMP_NET
    pdst[rdlen++] = 0x03;
// #else
	// pdst[rdlen++] = 0x01;
// #endif

    pdst[rdlen++] = LC_APP_VER&0xFF;
    pdst[rdlen++] = (LC_APP_VER>>8)&0xFF;
	//
    pdst[rdlen++] = 0x00;
    pdst[rdlen++] = 0x00;
    //
    return WHI_BuildFrameProcess(psrc,true,WHI_READ_VERSION,frameseq,pdst,rdlen);
    
}
/**
* @brief 全屋智能读取MAC处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_READ_MAC         (0x0002)
uint16_t WHI_ReadMacProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint8_t *pdst = &psrc[8];

	lc_net_get_factory_mac(pdst);
    rdlen += LC_NET_ADDR_SIZE;
    pdst[rdlen++] = 0x01;
    pdst[rdlen++] = 0x00;

    return WHI_BuildFrameProcess(psrc,true,WHI_READ_MAC,frameseq,pdst,rdlen);
    
}

/**
* @brief 全屋智能读取通讯地址处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_READ_ADDR        (0x0003)
uint16_t WHI_ReadAddrProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint8_t *pdst = &psrc[8];
//    const ST_STA_FACTORY_PARA * pStaSysPara = GetStaSysPara();
    //MAC 地址（ 6bytes）
    //保留（ 2bytes） 0000H
	

    
	lc_net_my_info_t p_info;
	memset(&p_info,0,sizeof(lc_net_my_info_t));
	lc_net_get_net_self_info(&p_info);
	
    memcpy(pdst,p_info.addr,LC_NET_ADDR_SIZE);
	
	rdlen += LC_NET_ADDR_SIZE;
    pdst[rdlen++] = 0x00;
    pdst[rdlen++] = 0x00;

    return WHI_BuildFrameProcess(psrc,true,WHI_READ_ADDR,frameseq,pdst,rdlen);

}

/**
* @brief 全屋智能设置通讯地址处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_SET_ADDR         (0x0004)
uint16_t WHI_SetAddrProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
    //
    //    设置结果 (1byte)
    //    0：成功， 1：失败
    //    失败原因 (1byte)，见 5.3.6
    //    Rsv (2bytes)
    
    if(framelen >= LC_NET_ADDR_SIZE)
    {		
		lc_net_set_factory_mac(pdst,LC_NET_ADDR_SIZE);
		if(LC_NET_EOK == lc_net_set_addr(pdst,LC_NET_ADDR_SIZE))
		{
			memcpy(lamp_ble_param.ble_mac_addr, pdst,LC_NET_ADDR_SIZE);
	        lc_net_get_factory_mac(lamp_ble_param.ble_mac_addr);
	        lamp_ble_set_mac_addr(lamp_ble_param.ble_mac_addr);
			
			pdst[rdlen++] = 0;
			pdst[rdlen++] = 0;
		}
		else
		{
			 pdst[rdlen++] = 1;
			 pdst[rdlen++] = WHI_ACK_FRAME_ERR;
		}
    }
    else
    {
         pdst[rdlen++] = 1;
         pdst[rdlen++] = WHI_ACK_FRAME_ERR;
    }

    return WHI_BuildFrameProcess(psrc,true,WHI_SET_ADDR,frameseq,pdst,rdlen);
}

/**
* @brief 全屋智能复位模块处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_RESET_MODEL      (0x0005)
uint16_t WHI_ReSetModelProcess(uint8_t *psrc,uint16_t dlen)
{
    //REBOOT_NORMAL_OUT(REBOOT_STA_APP_RST);
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
    uint8_t runflg = 1;//1失败，0成功
    if(framelen >= 1)
    {
        uint32_t rebootdelaysecond = pdst[0];
        rebootdelaysecond++;
		lc_net_reboot_system(rebootdelaysecond*1000ul);
		runflg = 0;
    }

    pdst[rdlen++] = runflg;
    pdst[rdlen++] = runflg==0?WHI_ACK_EOK:WHI_ACK_OTHER;
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;

    return WHI_BuildFrameProcess(psrc,true,WHI_RESET_MODEL,frameseq,pdst,rdlen);
}


/**
* @brief 全屋智能文件传输处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_FILE_CMD         (0x0006)



//文件性质 BIN 1
//文件 ID BIN 1
//目的地址 BIN 6
//文件总段数 n BIN 2
//文件大小 BIN 4
//文件总校验 BIN 2
//文件传输超时时间 BIN 1

//typedef struct __PACKED
//{
//    uint8_t fileattr;
//    uint8_t fileid;
//    uint8_t dstaddr[LC_NET_ADDR_SIZE];
//    uint16_t segmentsum;
//    uint32_t filesize;
//    uint16_t filecrc;
//    uint16_t filetrantime;

//}UPD_NwStartParamType;


uint16_t WHI_FileCmdProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];

    uint8_t filecmd = pdst[0];
    switch (filecmd)
    {
        case WHI_START_TRAN_FILE:
        {
            bool prflg = false;
            if((framelen >= 16)&&(pdst[1] < 2))
            {
				if(pdst[1] == 0)
				{
					lc_net_clear_file();
					prflg = true;
				}
				else if(pdst[1] == 1)
				{
					lc_net_file_info_t file_info;
					memset(&file_info,0,sizeof(lc_net_file_info_t));
					
					
					file_info.id = rand();//pdst[8] + pdst[9] + pdst[10] + pdst[11];
					file_info.size = pdst[4]|(pdst[5]<<8)|(pdst[6]<<16)|(pdst[7]<<24);
					file_info.crc = pdst[8]|(pdst[9]<<8)|(pdst[10]<<16)|(pdst[11]<<24);
					file_info.type = (lc_net_file_type_t)pdst[1];
					file_info.seg_cnt = (pdst[3]<<8)|pdst[2];
					lc_net_config_file(&file_info);
					prflg = true;
				}
            }

            pdst[rdlen++] = WHI_START_TRAN_FILE;
            pdst[rdlen++] = prflg == true?0:1;
            pdst[rdlen++] = prflg == true?0:WHI_ACK_OTHER;
            pdst[rdlen++] = 0;
            
            return WHI_BuildFrameProcess(psrc,true,WHI_FILE_CMD,frameseq,pdst,rdlen);

        }
        break;
        case WHI_TRAN_FILE_DATA:
        {
            bool prflg = false;
            if(framelen > 8)
            {
				uint32_t seg_offset = (pdst[3]<<8)|pdst[2];
				uint32_t seg_crc = (pdst[7]<<8)|pdst[6];
				uint8_t *seg_data = &pdst[8];
				uint32_t seg_len = (pdst[5]<<8)|pdst[4];
				if(seg_offset == 0)
				{
					if((seg_len&0x3F) != 0)
					{
						prflg = false;
					}
					else
					{
						prflg = (LC_NET_EOK == lc_net_write_file_seg(seg_offset,seg_crc,seg_data,seg_len))?true:false;
					}
				}
				else
				{
					prflg = (LC_NET_EOK == lc_net_write_file_seg(seg_offset,seg_crc,seg_data,seg_len))?true:false;
				}
            }
            
            pdst[rdlen++] = WHI_TRAN_FILE_DATA;
            pdst[rdlen++] = prflg == true?0:1;
            pdst[rdlen++] = prflg == true?0:WHI_ACK_OTHER;
            pdst[rdlen++] = 0;
            
            return WHI_BuildFrameProcess(psrc,true,WHI_FILE_CMD,frameseq,pdst,rdlen);


            
            
        }
        break;
        case WHI_SCAN_FILE_INFO:
        {
//            bool prflg = false;
//            if(framelen >= 4)
//            {
//                TaskFunProcessStType updatafunstr;
//                updatafunstr.cmd = UPDATA_FUN_SCAN_FILE_INFO_SOUTH;
//                updatafunstr.resultvalue = 0;
//                updatafunstr.Psrc = psrc;
//                updatafunstr.Pdst = psrc;
//                CallHplcReadDataFun(&updatafunstr);
//                if(updatafunstr.resultvalue != 0)
//                {
//                    UPD_NwStartParamType *pfileparam = (UPD_NwStartParamType*)psrc;
//                    //如果为 0 表示尚未开始传输，为 n 表示传输已完成，为 m 表示段号 0~m-1 的帧已传输
//                    //完成，可以从段号为 m 的帧开始续传。
//                    if((pfileparam->filetrantime != 0)
//                    &&(pfileparam->segmentsum != pfileparam->filetrantime))
//                    {
//                        uint16_t failsum = (pfileparam->segmentsum > pfileparam->filetrantime)?
//                            pfileparam->segmentsum - pfileparam->filetrantime:pfileparam->segmentsum;
//                        pdst[rdlen++] = WHI_SCAN_FILE_INFO;
//                        pdst[rdlen++] = 2;
//                        pdst[rdlen++] = failsum&0xFF;
//                        pdst[rdlen++] = (failsum>>8)&0xFF;
//                        return WHI_BuildFrameProcess(psrc,true,WHI_FILE_CMD,frameseq,pdst,rdlen);
//                    }
//                }
//            }
            pdst[rdlen++] = WHI_SCAN_FILE_INFO;
            pdst[rdlen++] = 0;
            pdst[rdlen++] = 0;
            pdst[rdlen++] = 0;

            return WHI_BuildFrameProcess(psrc,true,WHI_FILE_CMD,frameseq,pdst,rdlen);
        }
        break;
        case WHI_SET_UPDATA_NODE_LIST:
        {
            pdst[rdlen++] = WHI_SET_UPDATA_NODE_LIST;
            pdst[rdlen++] = 1;
            pdst[rdlen++] = WHI_ACK_FRAME_ERR;
            pdst[rdlen++] = 0;
            
            return WHI_BuildFrameProcess(psrc,true,WHI_FILE_CMD,frameseq,pdst,rdlen);

        }
        break;
        default:
        break;
    }

    return rdlen;
}

/**
* @brief 全屋智能读取运行时间处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_READ_RUN_TIME    (0x0007)
uint16_t WHI_ReadRunTimeProcess(uint8_t *psrc,uint16_t dlen)
{
    OS_ERR err;
    u32 runtime = OSTimeGet(&err);
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
    

    uint16_t readseq = (framelen >= 2)?((((uint16_t)pdst[1])<<8)|pdst[0]):0;

	lc_net_my_info_t p_info;
	memset(&p_info,0,sizeof(lc_net_my_info_t));
	lc_net_get_net_self_info(&p_info);
	
    memcpy(pdst,p_info.addr,LC_NET_ADDR_SIZE);

    rdlen += LC_NET_ADDR_SIZE;

    pdst[rdlen++] = readseq&0xFF;
    pdst[rdlen++] = (readseq>>8)&0xFF;

    pdst[rdlen++] = (runtime>>0)&0xFF;
    pdst[rdlen++] = (runtime>>8)&0xFF;
    pdst[rdlen++] = (runtime>>16)&0xFF;
    pdst[rdlen++] = (runtime>>24)&0xFF;

    return WHI_BuildFrameProcess(psrc,true,WHI_READ_ADDR,frameseq,pdst,rdlen);

}

/**
* @brief 全屋智能读取节点总数处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_READ_NODE_SUM   
uint16_t WHI_ReadNodeSumProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];

// #ifdef PLAT_LAMP_NET
	uint16_t node_sum = ble_data_get_count();
// #else
	// uint16_t node_sum = lc_net_wl_get_count();
// #endif

    pdst[rdlen ++] = node_sum&0xFF;//当前从节点数量 BIN 2
    pdst[rdlen++] = (node_sum>>8)&0xFF;
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;

    //dsp_memset(pdst,0,4);
    //rdlen += 4;

    return WHI_BuildFrameProcess(psrc,true,WHI_READ_NODE_SUM,frameseq,pdst,rdlen);

}
/**
* @brief 全屋智能读取节点处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_READ_NODE_INFO   
uint16_t WHI_ReadNodeInfoProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];

    u16 needreadmeterindex = pdst[1];
    needreadmeterindex = (needreadmeterindex<<8)|pdst[0];
    u16 needreadmetersum = pdst[3];
    needreadmetersum = (needreadmetersum<<8)|pdst[2];
    
// #ifdef PLAT_LAMP_NET
	uint16_t macaddrsum = ble_data_get_count();
// #else
	// uint16_t macaddrsum = lc_net_wl_get_count();
// #endif
    uint16_t datalen = 0;
    pdst[datalen ++] = macaddrsum&0xFF;//当前从节点数量 BIN 2
    pdst[datalen ++] = (macaddrsum>>8)&0xFF;
    
    pdst[datalen ++] = needreadmeterindex&0xFF;//当前从节点数量 BIN 2
    pdst[datalen ++] = (needreadmeterindex>>8)&0xFF;

    pdst[datalen ++] = needreadmetersum&0xFF;//当前从节点数量 BIN 2
    pdst[datalen ++] = (needreadmetersum>>8)&0xFF;

    pdst[datalen ++] = 0;
    pdst[datalen ++] = 0;


    u8 *addmeterp = &pdst[datalen];
    
// #ifdef PLAT_LAMP_NET
	needreadmetersum = ble_data_get_addrs(addmeterp,needreadmeterindex,needreadmetersum);
// #else
    // needreadmetersum = lc_net_wl_get_addrs(addmeterp,needreadmeterindex,needreadmetersum);
// #endif
	
	
    pdst[4] = needreadmetersum&0xFF;//当前从节点数量 BIN 2
    pdst[5] = (needreadmetersum>>8)&0xFF;
    datalen += needreadmetersum*LC_NET_ADDR_SIZE;
   
    return WHI_BuildFrameProcess(psrc,true,WHI_READ_NODE_INFO,frameseq,pdst,datalen);

}
/**
* @brief 全屋智能添加节点处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_ADD_NODE_INFO    
uint16_t WHI_AddNodeInfoProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];

    u16 addrsum = pdst[1];
    addrsum = (addrsum<<8)|pdst[0];

	bool rflg = false;
// #ifdef PLAT_LAMP_NET
// 	// rflg = ble_data_add_addrs(&pdst[2],addrsum);
// #else
// 	if(LC_NET_EOK == lc_net_wl_add_addrs(&pdst[2],addrsum))
// 	{
// 		rflg = true;
// 	}
// #endif
    //===
    if(rflg)
    {
        pdst[rdlen++] = 0;
        pdst[rdlen++] = 0;
        pdst[rdlen++] = 0;
        pdst[rdlen++] = 0;
    }
    else
    {
        pdst[rdlen++] = 1;
        pdst[rdlen++] = WHI_ACK_FRAME_ERR;
        pdst[rdlen++] = 0;
        pdst[rdlen++] = 0;
    }

    return WHI_BuildFrameProcess(psrc,true,WHI_ADD_NODE_INFO,frameseq,pdst,rdlen);

}
/**
* @brief 全屋智能删除节点处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_DEL_NODE_INFO    
uint16_t WHI_DelNodeInfoProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];

    u16 addrsum = pdst[1];
    addrsum = (addrsum<<8)|pdst[0];
// #ifdef PLAT_LAMP_NET
	ble_data_del_addrs(&pdst[2],addrsum);
// #else
// 	lc_net_wl_del_addrs(&pdst[2],addrsum);
// #endif
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;

    return WHI_BuildFrameProcess(psrc,true,WHI_DEL_NODE_INFO,frameseq,pdst,rdlen);

}

/**
* @brief 全屋智能清空节点处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_CLEAR_NODE_INFO      
uint16_t WHI_ClearNodeInfoProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
// #ifdef PLAT_LAMP_NET
    ble_data_clear_addrs();
	ble_local_restart_net();//重新组网
// #else
// 	lc_net_wl_clear_addrs();
// #endif
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;

    return WHI_BuildFrameProcess(psrc,true,WHI_CLEAR_NODE_INFO,frameseq,pdst,rdlen);

    
}
/**
* @brief 全屋智能开始自组网功能处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_SET_ZZW_STATE      
uint16_t WHI_SetZzwStateProcess(uint8_t *psrc,uint16_t dlen)
{

    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];

    pdst[rdlen++] = 1;
    pdst[rdlen++] = WHI_ACK_FRAME_ERR;
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;

    return WHI_BuildFrameProcess(psrc,true,WHI_SET_ZZW_STATE,frameseq,pdst,rdlen);

}


/**
* @brief 全屋智能设置白名单状态处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_SET_RECORD_STATE
uint16_t WHI_SetRecordStateProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];

    if((framelen >= 1)&&(pdst[0] == 0x00))
    {
        pdst[rdlen++] = 1;
        pdst[rdlen++] = WHI_ACK_FRAME_ERR;
        pdst[rdlen++] = 0;
        pdst[rdlen++] = 0;
    }
    else
    {
        pdst[rdlen++] = 0;
        pdst[rdlen++] = 0;
        pdst[rdlen++] = 0;
        pdst[rdlen++] = 0;

    }

    return WHI_BuildFrameProcess(psrc,true,WHI_SET_RECORD_STATE,frameseq,pdst,rdlen);

    
}

/**
* @brief 全屋智能读取白名单状态处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_READ_RECORD_STATE    (0x0017)
uint16_t WHI_ReadRecordStateProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];

    pdst[rdlen++] = 1; //开启状态
    pdst[rdlen++] = WHI_ACK_FRAME_ERR;
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;

    return WHI_BuildFrameProcess(psrc,true,WHI_READ_RECORD_STATE,frameseq,pdst,rdlen);

    
}

/**
* @brief 全屋智能读取拓扑节点总数处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_READ_TOPOLOGY_NODE_SUM     (0x0020)
uint16_t WHI_ReadTopologyNodeSumProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];

   // uint16_t needreadsum = 0;
	// #ifdef PLAT_LAMP_NET
    uint16_t nodesum = ble_data_get_topo_node_count();
	// #else
	// uint16_t nodesum = lc_net_get_topo_node_count();
	// #endif
    //
    rdlen = 0;
    
    pdst[rdlen++] = nodesum&0xFF;
    pdst[rdlen++] = (nodesum>>8)&0xFF;
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;

    return WHI_BuildFrameProcess(psrc,true,WHI_READ_TOPOLOGY_NODE_SUM,frameseq,pdst,rdlen);

}

/**
* @brief 全屋智能读取拓扑节点处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
uint16_t WHI_ReadTopologyNodeInfoProcess(uint8_t *psrc,uint16_t dlen)
{
    //
    uint16_t seqofframe = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t lenofframe = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
    //
    uint16_t readindex = pdst[1];
    readindex = (readindex<<8)|pdst[0];
    uint16_t needreadsum = pdst[3];
    needreadsum = (needreadsum<<8)|pdst[2];

    uint16_t nodesum = 0;
    //
    needreadsum = needreadsum >= 100?100:needreadsum;
    //
	if(readindex > 0)readindex--;
    
	// #ifdef PLAT_LAMP_NET
	nodesum = ble_data_get_topo_node_count();
    needreadsum = ble_data_get_topo_nodes((ble_data_topo_node_info_t*)&pdst[8],readindex,needreadsum);
	// #else
	// nodesum = lc_net_get_topo_node_count();
	// needreadsum = lc_net_get_topo_nodes((lc_net_topo_node_info_t*)&pdst[8],readindex,needreadsum);
	// #endif
	
    uint16_t rdlen = 0;
	readindex++;
    pdst[rdlen++] = nodesum&0xFF;
    pdst[rdlen++] = (nodesum>>8)&0xFF;
    pdst[rdlen++] = readindex&0xFF;
    pdst[rdlen++] = (readindex>>8)&0xFF;
    pdst[rdlen++] = needreadsum&0xFF;
    pdst[rdlen++] = (needreadsum>>8)&0xFF;

    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;
	//

    rdlen += needreadsum*sizeof(lc_net_topo_node_info_t);

    return WHI_BuildFrameProcess(psrc,true,WHI_READ_TOPOLOGY_NODE_INFO,seqofframe,pdst,rdlen);
}

//===
/*
*/
uint16_t WHI_CfgIspKeyCmdProcess(uint8_t *psrc,uint16_t dlen)
{
	uint16_t rdlen = 0;
    uint16_t seqofframe = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t lenofframe = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	//
	if(lenofframe >= 4)
	{
		uint32_t isp_kep = pdst[3];
		isp_kep = (isp_kep<<8)|pdst[2];
		isp_kep = (isp_kep<<8)|pdst[1];
		isp_kep = (isp_kep<<8)|pdst[0];
		
        ble_net_info.IspKey = isp_kep;

		if (isp_kep != lc_net_get_factory_id_code())
		{
			lc_net_write_factory_id_code(isp_kep);
		}
	}
	
	pdst[rdlen++] = 0;
	pdst[rdlen++] = 0;
	pdst[rdlen++] = 0;
	pdst[rdlen++] = 0;

    return WHI_BuildFrameProcess(psrc,true,WHI_CFG_ISP_KEY_CMD,seqofframe,pdst,rdlen);
}
/*
*/
uint16_t WHI_ReadIspKeyCmdProcess(uint8_t *psrc,uint16_t dlen)
{
	uint16_t rdlen = 0;
    uint16_t seqofframe = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t lenofframe = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	//
	uint32_t isp_kep = lc_net_get_factory_id_code();
	pdst[rdlen++] = (isp_kep>>0)&0xFF;
	pdst[rdlen++] = (isp_kep>>8)&0xFF;
	pdst[rdlen++] = (isp_kep>>16)&0xFF;
	pdst[rdlen++] = (isp_kep>>24)&0xFF;
	//
	return WHI_BuildFrameProcess(psrc,true,WHI_READ_ISP_KEY_CMD,seqofframe,pdst,rdlen);
}
/*
*/
uint16_t WHI_CfgProjKeyCmdProcess(uint8_t *psrc,uint16_t dlen)
{
	uint16_t rdlen = 0;
    uint16_t seqofframe = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t lenofframe = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	//
	if(lenofframe >= 4)
	{
		uint32_t proj_kep = pdst[3];
		proj_kep = (proj_kep<<8)|pdst[2];
		proj_kep = (proj_kep<<8)|pdst[1];
		proj_kep = (proj_kep<<8)|pdst[0];
        
        ble_net_info.ProjKey = proj_kep;
		//
		if(proj_kep != lc_net_get_project_id_code())
		{
			lc_net_write_project_id_code(proj_kep);

			ble_local_restart_net();

			LcSetFactoryMsg();

		}
		//
	}
	//
	pdst[rdlen++] = 0;
	pdst[rdlen++] = 0;
	pdst[rdlen++] = 0;
	pdst[rdlen++] = 0;

    return WHI_BuildFrameProcess(psrc,true,WHI_CFG_PROJ_KEY_CMD,seqofframe,pdst,rdlen);
}
/*
*/
uint16_t WHI_ReadProjKeyCmdProcess(uint8_t *psrc,uint16_t dlen)
{
	uint16_t rdlen = 0;
    uint16_t seqofframe = (((uint16_t)psrc[5])<<8)|psrc[4];
   // uint16_t lenofframe = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	//
	uint32_t proj_kep = lc_net_get_project_id_code();
	pdst[rdlen++] = (proj_kep>>0)&0xFF;
	pdst[rdlen++] = (proj_kep>>8)&0xFF;
	pdst[rdlen++] = (proj_kep>>16)&0xFF;
	pdst[rdlen++] = (proj_kep>>24)&0xFF;
	//
	return WHI_BuildFrameProcess(psrc,true,WHI_READ_PROJ_KEY_CMD,seqofframe,pdst,rdlen);
}
/*
*/
// #ifdef PLAT_LAMP_NET
uint16_t WHI_CfgNetEnableFlgCmdProcess(uint8_t *psrc,uint16_t dlen)
{
	uint16_t rdlen = 0;
    uint16_t seqofframe = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t lenofframe = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	//
	uint8_t return_code = WHI_ACK_EOK;
	if(lenofframe >= 1)
	{
		uint8_t net_enalbe_flg = pdst[0];
		if(ble_system_params.NetEnableFlag != net_enalbe_flg)
		{
			ble_system_params.NetEnableFlag = net_enalbe_flg;
			//
			uint32_t param = lc_net_get_ex_param_code();
			if(ble_system_params.NetEnableFlag == 0)
			{
				param &= ~(0x01UL);
			}
			else
			{
				param |= 0x01;
			}
			lc_net_write_ex_param_code(param);
			//
			ble_local_restart_net();
		}
	}
	else
	{
		return_code = WHI_ACK_FRAME_ERR;
	}
	
	pdst[rdlen++] = return_code==WHI_ACK_EOK?0:1;
	pdst[rdlen++] = return_code;
	pdst[rdlen++] = 0;
	pdst[rdlen++] = 0;
	//
	return WHI_BuildFrameProcess(psrc,true,WHI_CFG_NET_ENABLE_FLG_CMD,seqofframe,pdst,rdlen);
}
uint16_t WHI_ReadNetEnableFlgCmdProcess(uint8_t *psrc,uint16_t dlen)
{
	uint16_t rdlen = 0;
    uint16_t seqofframe = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t lenofframe = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	//
	pdst[rdlen++] = ble_system_params.NetEnableFlag;
	pdst[rdlen++] = 0;
	pdst[rdlen++] = 0;
	pdst[rdlen++] = 0;
	//
	return WHI_BuildFrameProcess(psrc,true,WHI_READ_NET_ENABLE_FLG_CMD,seqofframe,pdst,rdlen);
}

uint16_t WHI_NetStartBindNodeCmdProcess(uint8_t *psrc,uint16_t dlen)
{
	uint16_t rdlen = 0;
    uint16_t seqofframe = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t lenofframe = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	//
	uint8_t return_code = WHI_ACK_OTHER;
	//
	if(lenofframe >= 4)
	{
		uint32_t second = pdst[3];
		second = (second<<8)|pdst[2];
		second = (second<<8)|pdst[1];
		second = (second<<8)|pdst[0];
		//
		if(second > (36*3600UL))
		{
			second = (36*3600UL);
		}
		if(0 != StartNetBindTimer(second*1000UL))
		{
			return_code = WHI_ACK_EOK;
		}
	}
	//
	pdst[rdlen++] = return_code==WHI_ACK_EOK?0:1;
	pdst[rdlen++] = return_code;
	pdst[rdlen++] = 0;
	pdst[rdlen++] = 0;

    return WHI_BuildFrameProcess(psrc,true,WHI_NET_START_BIND_NODE_CMD,seqofframe,pdst,rdlen);
}
/*
*/
uint16_t WHI_NetStopBindNodeCmdProcess(uint8_t *psrc,uint16_t dlen)
{
	uint16_t rdlen = 0;
    uint16_t seqofframe = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t lenofframe = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	uint8_t return_code;
	if(1 == StopNetBindTimer())
	{
		return_code = WHI_ACK_EOK;
	}
	else
	{
		return_code = WHI_ACK_OTHER;
	}
	pdst[rdlen++] = return_code==WHI_ACK_EOK?0:1;
	pdst[rdlen++] = return_code;
	pdst[rdlen++] = 0;
	pdst[rdlen++] = 0;

    return WHI_BuildFrameProcess(psrc,true,WHI_NET_STOP_BIND_NODE_CMD,seqofframe,pdst,rdlen);
}
/*
*/
uint16_t WHI_NetQueryBindNodeCmdProcess(uint8_t *psrc,uint16_t dlen)
{
	uint16_t rdlen = 0;
    uint16_t seqofframe = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t lenofframe = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	//lc_net_set_stop_bind_code();
	bind_code_query_info_type info;
	QueryNetBindInfo(&info);
	//
	pdst[rdlen++] = info.bind_statue;
	//
	info.leave_time_ms = (info.leave_time_ms + 999)/1000UL;
	//
	pdst[rdlen++] = ((info.leave_time_ms)>>0)&0xFF;
	pdst[rdlen++] = ((info.leave_time_ms)>>8)&0xFF;
	pdst[rdlen++] = ((info.leave_time_ms)>>16)&0xFF;
	pdst[rdlen++] = ((info.leave_time_ms)>>24)&0xFF;


    return WHI_BuildFrameProcess(psrc,true,WHI_NET_QUERY_BIND_NODE_CMD,seqofframe,pdst,rdlen);
}

// #endif
/**
* @brief 全屋智能信道转发发送处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_SEND_HPLC_FRAME            (0x0100)  发送数据 主控到模组
uint16_t WHI_SendHplcFrameProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t send_len = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
	uint8_t* send_buff = NULL;
	uint8_t *pdst = &psrc[8];
	//
	uint8_t return_code = WHI_ACK_EOK;
	//
	lc_net_my_info_t p_info;
	lc_net_get_net_self_info(&p_info);
	//
//	uint8_t brocast_addr[LC_NET_ADDR_SIZE] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	if(framelen <= 8)
	{
		return_code = WHI_ACK_FRAME_ERR;
		goto fun_end;
	}
	
	if(p_info.net_state != LC_NET_IN_STATE)
	{
		return_code = WHI_ACK_BUSY;
		goto fun_end;
	}
	
//	if((0 != dsp_memcmp(&psrc[8],p_info.cco_addr,LC_NET_ADDR_SIZE))
//		&&(0 != dsp_memcmp(&psrc[8],brocast_addr,LC_NET_ADDR_SIZE)))
//	{
//		return_code = WHI_ACK_FRAME_ERR;
//		goto fun_end;
//	}
	
	
	send_buff = malloc(WHI_HPLC_ADAPT_HEAD_LEN+framelen+32);
	if(send_buff == NULL)
	{
		return_code = WHI_ACK_BUSY;
		goto fun_end;
	}
	
	send_len = whi_hplc_adapt_create(WHI_SEND_HPLC_FRAME,send_buff,&psrc[8],framelen);
	if(LC_NET_EOK != lc_net_tx_data(p_info.cco_addr,send_buff,send_len,LC_NET_BLOCK))
	{
		return_code = WHI_ACK_TRAN_TIME_OUT;
	}
	//
	free(send_buff);
	//
fun_end:
	
	pdst[rdlen++] = return_code==WHI_ACK_EOK?0:1;
	pdst[rdlen++] = return_code;
	pdst[rdlen++] = 0;
	pdst[rdlen++] = 0;
    return WHI_BuildFrameProcess(psrc,true,WHI_SEND_HPLC_FRAME,frameseq,pdst,rdlen);
}
/**
* @brief 全屋智能信道转发接收处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_RECEIVE_HPLC_FRAME         (0x0101)  接收数据 模组到主控
uint16_t WHI_ReceiveHplcFrameProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
//    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];

    pdst[rdlen++] = 1;
    pdst[rdlen++] = WHI_ACK_FRAME_ERR;
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;

    return WHI_BuildFrameProcess(psrc,true,WHI_RECEIVE_HPLC_FRAME,frameseq,pdst,rdlen);
    
}

/**
* @brief 全屋智能远程调试发送处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_SEND_STA_FRAME             (0x0110) 发送数据 主控到模组
uint16_t WHI_SendStaFrameProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    //uint16_t send_len = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];

	uint8_t *pdst = &psrc[8];
	//
	uint8_t return_code = WHI_ACK_EOK;
	//
// #ifdef PLAT_LAMP_NET
	if(framelen >= 8)
	{
		lc_net_my_info_t p_info;
		lc_net_get_net_self_info(&p_info);
		if(0 != memcmp(&psrc[8],p_info.addr,LC_NET_ADDR_SIZE))
		{
			ble_apl_send_local_cmd_frame(&psrc[8],&psrc[8],framelen,LC_NET_NOT_BLOCK);
		}
		else
		{
			return_code = WHI_ACK_OTHER;
			pdst[rdlen++] = return_code==WHI_ACK_EOK?0:1;
			pdst[rdlen++] = return_code;
			pdst[rdlen++] = 0;
			pdst[rdlen++] = 0;
			return WHI_BuildFrameProcess(psrc,true,WHI_SEND_STA_FRAME,frameseq,pdst,rdlen);
		}
	}
    return 0;
/* #else
	//
		uint8_t* send_buff = NULL;
	lc_net_my_info_t p_info;
	lc_net_get_net_self_info(&p_info);
	//
	uint8_t brocast_addr[LC_NET_ADDR_SIZE] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	if(framelen <= 8)
	{
		return_code = WHI_ACK_FRAME_ERR;
		goto fun_end;
	}
	
	if(p_info.net_state != LC_NET_IN_STATE)
	{
		return_code = WHI_ACK_BUSY;
		goto fun_end;
	}
	
	if((0 != memcmp(&psrc[8],p_info.cco_addr,LC_NET_ADDR_SIZE))
		&&(0 != memcmp(&psrc[8],brocast_addr,LC_NET_ADDR_SIZE)))
	{
		return_code = WHI_ACK_FRAME_ERR;
		goto fun_end;
	}
	
	send_buff = malloc(WHI_HPLC_ADAPT_HEAD_LEN+framelen+32);
	if(send_buff == NULL)
	{
		return_code = WHI_ACK_BUSY;
		goto fun_end;
	}
	
	send_len = whi_hplc_adapt_create(WHI_SEND_STA_FRAME,send_buff,&psrc[8],framelen);
	if(LC_NET_EOK != lc_net_tx_data(p_info.cco_addr,send_buff,send_len,LC_NET_BLOCK))
	{
		return_code = WHI_ACK_TRAN_TIME_OUT;
	}
	//
	free(send_buff);
	//
	fun_end:
	
	pdst[rdlen++] = return_code==WHI_ACK_EOK?0:1;
	pdst[rdlen++] = return_code;
	pdst[rdlen++] = 0;
	pdst[rdlen++] = 0;
    return WHI_BuildFrameProcess(psrc,true,WHI_SEND_STA_FRAME,frameseq,pdst,rdlen);
 #endif*/
}
/**
* @brief 全屋智能远程调试接收处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_RECEIVE_STA_FRAME          (0x0111) 接收数据 模组到主控
uint16_t WHI_ReceiveStaFrameProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];

    pdst[rdlen++] = 1;
    pdst[rdlen++] = WHI_ACK_FRAME_ERR;
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;

    return WHI_BuildFrameProcess(psrc,true,WHI_RECEIVE_HPLC_FRAME,frameseq,pdst,rdlen);
}

/**
* @brief 全屋智能总线数据通讯处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_SEND_BUS_FRAME             (0x0120)
uint16_t WHI_SendBusFrameProcess(uint8_t *psrc,uint16_t dlen)
{
    //uint16_t rdlen = 0;
    // uint16_t send_len = 0;
    //uint16_t send_len = 0;
    //uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
	//uint8_t* send_buff = NULL;
	//uint8_t *pdst = &psrc[8];
	//
	// uint8_t return_code = WHI_ACK_EOK;
	//
#if 1
        	uint8_t *pdst = &psrc[8];
            uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
	if(framelen >= 8)
	{
		uint8_t brocast_addr[LC_NET_ADDR_SIZE] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
		uint8_t local_brocast_addr[LC_NET_ADDR_SIZE] = {0xFE,0xFF,0xFF,0xFF,0xFF,0xFF};
		
		lc_net_my_info_t p_info;
		lc_net_get_net_self_info(&p_info);
		#if 1
		if(0 != memcmp(&psrc[8],p_info.addr,LC_NET_ADDR_SIZE))
		{
			ble_apl_send_system_ctrl_cmd_frame(&psrc[8],&psrc[8],framelen,LC_NET_NOT_BLOCK);
		}
		#endif
		if((0 == memcmp(&psrc[8],p_info.addr,LC_NET_ADDR_SIZE)))
		{
			rdlen = whi_sysctrl_process_frame(&psrc[8],framelen,BROCAST_ADDR_INVALID);
			if(rdlen > 0)
			{
				return WHI_BuildFrameProcess(psrc,true,WHI_SEND_BUS_FRAME,frameseq,pdst,rdlen);
			}
			else
			{
				return 0;
			}
		}
		else if((0 == memcmp(&psrc[8],brocast_addr,LC_NET_ADDR_SIZE))
		||(0 == memcmp(&psrc[8],local_brocast_addr,LC_NET_ADDR_SIZE)))
		{
			rdlen = whi_sysctrl_process_frame(&psrc[8],framelen,BROCAST_ADDR_VALID);
			if(rdlen > 0)
			{
				return WHI_BuildFrameProcess(psrc,true,WHI_SEND_BUS_FRAME,frameseq,pdst,rdlen);
			}
			else
			{
				return 0;
			}
		}
	}
    return 0;
#else
	//uint8_t brocast_addr[LC_NET_ADDR_SIZE] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	if(framelen <= 8)
	{
		return_code = WHI_ACK_FRAME_ERR;
		goto fun_end;
	}
	
	lc_net_my_info_t p_info;
	lc_net_get_net_self_info(&p_info);
	if(p_info.net_state != LC_NET_IN_STATE)
	{
		return_code = WHI_ACK_BUSY;
		goto fun_end;
	}
//	if((0 != dsp_memcmp(&psrc[8],p_info.cco_addr,LC_NET_ADDR_SIZE))
//		&&(0 != dsp_memcmp(&psrc[8],brocast_addr,LC_NET_ADDR_SIZE)))
//	{
//		return_code = WHI_ACK_FRAME_ERR;
//		goto fun_end;
//	}
	
	
	send_buff = malloc(WHI_HPLC_ADAPT_HEAD_LEN+framelen+32);
	if(send_buff == NULL)
	{
		return_code = WHI_ACK_BUSY;
		goto fun_end;
	}
	
        send_len = whi_hplc_adapt_create(WHI_SEND_BUS_FRAME,send_buff,&psrc[8],framelen);
	if(LC_NET_EOK != lc_net_tx_data(p_info.cco_addr,send_buff,send_len,LC_NET_NOT_BLOCK))
	{
		return_code = WHI_ACK_TRAN_TIME_OUT;
	}
	//
	free(send_buff);
	//
	fun_end:
	return return_code;
#endif
}
/**
* @brief 全屋智能远程调试发送处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_SEND_STA_NO_ACK_FRAME        (0x0102)
uint16_t WHI_SendStaNoAckProcess(uint8_t *psrc,uint16_t dlen)
{
    //uint16_t rdlen = 0;
    uint16_t send_len = 0;
    //uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
	uint8_t* send_buff = NULL;
	//uint8_t *pdst = &psrc[8];
	//
	uint8_t return_code = WHI_ACK_EOK;
	//
	lc_net_my_info_t p_info;
	lc_net_get_net_self_info(&p_info);
	//
	//uint8_t brocast_addr[LC_NET_ADDR_SIZE] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	if(framelen <= 8)
	{
		return_code = WHI_ACK_FRAME_ERR;
		goto fun_end;
	}
	
	if(p_info.net_state != LC_NET_IN_STATE)
	{
		return_code = WHI_ACK_BUSY;
		goto fun_end;
	}
	
//	if((0 != dsp_memcmp(&psrc[8],p_info.cco_addr,LC_NET_ADDR_SIZE))
//		&&(0 != dsp_memcmp(&psrc[8],brocast_addr,LC_NET_ADDR_SIZE)))
//	{
//		return_code = WHI_ACK_FRAME_ERR;
//		goto fun_end;
//	}
	
	
	send_buff = malloc(WHI_HPLC_ADAPT_HEAD_LEN+framelen+32);
	if(send_buff == NULL)
	{
		return_code = WHI_ACK_BUSY;
		goto fun_end;
	}
	
	send_len = whi_hplc_adapt_create(WHI_SEND_STA_NO_ACK_FRAME,send_buff,&psrc[8],framelen);
	if(LC_NET_EOK != lc_net_tx_data(p_info.cco_addr,send_buff,send_len,LC_NET_NOT_BLOCK))
	{
		return_code = WHI_ACK_TRAN_TIME_OUT;
	}
	//
	free(send_buff);
	//
	fun_end:
	return return_code;

}
/**
* @brief 全屋智能远程调试接收处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_RECEIVE_STA_NO_ACK_FRAME     (0x0103)
uint16_t WHI_ReceiveStaNoAckFrameProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];

    pdst[rdlen++] = 1;
    pdst[rdlen++] = WHI_ACK_FRAME_ERR;
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;

    return WHI_BuildFrameProcess(psrc,true,WHI_RECEIVE_HPLC_FRAME,frameseq,pdst,rdlen);
}

/**
* @brief 全屋智能总线数据通讯处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
//#define WHI_SEND_BUS_TRAN_FRAME          (0x0150)
uint16_t WHI_SendBusTranFrameProcess(uint8_t *psrc,uint16_t dlen)
{
   // uint16_t rdlen = 0;
    uint16_t send_len = 0;
    //uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
	uint8_t* send_buff = NULL;
	//uint8_t *pdst = &psrc[8];
	//
	uint8_t return_code = WHI_ACK_EOK;
	//
	lc_net_my_info_t p_info;
	lc_net_get_net_self_info(&p_info);
	//
	//uint8_t brocast_addr[LC_NET_ADDR_SIZE] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	if(framelen <= 8)
	{
		return_code = WHI_ACK_FRAME_ERR;
		goto fun_end;
	}
	
	if(p_info.net_state != LC_NET_IN_STATE)
	{
		return_code = WHI_ACK_BUSY;
		goto fun_end;
	}
	
	
//	if((0 != dsp_memcmp(&psrc[8],p_info.cco_addr,LC_NET_ADDR_SIZE))
//		&&(0 != dsp_memcmp(&psrc[8],brocast_addr,LC_NET_ADDR_SIZE)))
//	{
//		return_code = WHI_ACK_FRAME_ERR;
//		goto fun_end;
//	}
	
	
	send_buff = malloc(WHI_HPLC_ADAPT_HEAD_LEN+framelen+32);
	if(send_buff == NULL)
	{
		return_code = WHI_ACK_BUSY;
		goto fun_end;
	}
	
	send_len = whi_hplc_adapt_create(WHI_SEND_BUS_TRAN_FRAME,send_buff,&psrc[8],framelen);
	if(LC_NET_EOK != lc_net_tx_data(p_info.cco_addr,send_buff,send_len,LC_NET_NOT_BLOCK))
	{
		return_code = WHI_ACK_TRAN_TIME_OUT;
	}
	//
	free(send_buff);
	//
	fun_end:
	return return_code;
}
/**
* @brief 全屋智能读取频段处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
uint16_t WHI_ReadFreqBandProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint8_t *pdst = &psrc[8];
//
	lc_net_my_info_t p_info;
	lc_net_get_net_self_info(&p_info);
	
    pdst[rdlen++] = p_info.freq_band;
    pdst[rdlen++] = 0x00;
    pdst[rdlen++] = 0;
    pdst[rdlen++] = 0;

    return WHI_BuildFrameProcess(psrc,true,WHI_READ_FREQ_BAND,frameseq,pdst,rdlen);

}

/**
* @brief 全屋智能设置频段处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
uint16_t WHI_SetFreqBandProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
    //
    //    设置结果 (1byte)
    //    0：成功， 1：失败
    //    失败原因 (1byte)，见 5.3.6
    //    Rsv (2bytes)
    
//    if((framelen >= 1)
//    &&(SetNetChlPwrAndThr(pdst[0]%4,0xFF,0xFF)))
//    {
//        pdst[rdlen++] = 0;
//        pdst[rdlen++] = 0;
//        pdst[rdlen++] = 0;
//        pdst[rdlen++] = 0;

//    }
//    else
    {
         pdst[rdlen++] = 1;
         pdst[rdlen++] = WHI_ACK_FRAME_ERR;
        pdst[rdlen++] = 0;
        pdst[rdlen++] = 0;

    }
    return WHI_BuildFrameProcess(psrc,true,WHI_SET_FREQ_BAND,frameseq,pdst,rdlen);
}
//=======================
// #ifdef PLAT_LAMP
//启动站
uint16_t WHI_LampBleNetCtrlFrameProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	bool need_ack = true;
    //
	//local_debug_cnt.receive_ble_net_frame++;  $$$
	//cb_fun
	// #ifdef PLAT_LAMP_NET
	rdlen = ble_local_process_frame_ex(pdst,framelen, &need_ack);
	// #endif
	//
	if(need_ack)
	{
		if(rdlen == 0)
		{
			pdst[rdlen++] = WHI_ACK_EOK;
			pdst[rdlen++] = 0;
			pdst[rdlen++] = 0;
			pdst[rdlen++] = 0;
		}
		//
	    return WHI_BuildFrameProcess(psrc,true,WHI_LAMP_BLE_NET_CTRL_FRAME,frameseq,pdst,rdlen);
	}
	else
	{
		return 0;
	}
}

// #endif
uint16_t WHI_BuildReportInNetNodeListProcess(uint8_t *send_buff,uint8_t *node_list,uint16_t node_sum,uint16_t frame_seq)
{
	uint16_t rdlen = 0;
	uint16_t w_idx = WHI_PRO_STAT_HEAD_SIZE;
	send_buff[w_idx++] = node_sum&0xFF;
	send_buff[w_idx++] = (node_sum>>8)&0xFF;
	if(node_list != &send_buff[w_idx])
	{
		memcpy(&send_buff[w_idx],node_list,node_sum*LC_NET_ADDR_SIZE);
	}
	rdlen = WHI_BuildFrameProcess(send_buff,false,WHI_REPORT_IN_NET_NODE_LIST,frame_seq,&send_buff[WHI_PRO_STAT_HEAD_SIZE],node_sum*LC_NET_ADDR_SIZE + 2);

	return rdlen;
}

uint16_t WHI_BuildReportLeaveNetNodeListProcess(uint8_t *send_buff,uint8_t *node_list,uint16_t node_sum,uint16_t frame_seq)
{
	uint16_t rdlen = 0;
//WHI_CreateLocalSeq()
	uint16_t w_idx = WHI_PRO_STAT_HEAD_SIZE;
	send_buff[w_idx++] = node_sum&0xFF;
	send_buff[w_idx++] = (node_sum>>8)&0xFF;
	if(node_list != &send_buff[w_idx])
	{
		memcpy(&send_buff[w_idx],node_list,node_sum*LC_NET_ADDR_SIZE);
	}
	rdlen = WHI_BuildFrameProcess(send_buff,false,WHI_REPORT_LEAVE_NET_NODE_LIST,frame_seq,&send_buff[WHI_PRO_STAT_HEAD_SIZE],node_sum*LC_NET_ADDR_SIZE + 2);

	return rdlen;
}




// #ifdef PLAT_LAMP_NET
bool WHI_LampReportInNetNodeListAckFrameProcess(uint8_t *psrc,uint16_t dlen)
{
	bool rflg = false;
    //uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    //uint8_t *pdst = &psrc[8];
	rflg = ble_mac_addr_in_leave_net_ack_cb_fun(frameseq,1);
	return rflg;
}
bool WHI_LampReportLeaveNetNodeListAckFrameProcess(uint8_t *psrc,uint16_t dlen)
{
	bool rflg = false;
   // uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    //uint8_t *pdst = &psrc[8];
	rflg = ble_mac_addr_in_leave_net_ack_cb_fun(frameseq,0);
	return rflg;
}
// #endif
/**
* @brief 全屋智能检测帧格式处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传校难成功或失败
* @author hh
*/
bool WHI_IsLocalWHIFrame(uint8_t *pbuff,uint16_t len)
{
    bool flg = false;
    uint16_t dlen;
	if(pbuff == NULL)
		return false;
	
    if((pbuff[0] == 0x48)&&(len>=10)&&((pbuff[1]&0x3F) == 0x00))
    {
        dlen = (((u16)pbuff[7])<<8)|pbuff[6];
        dlen += 10;
        if(dlen  <= len)
        {
			uint16_t crcvalue = crc16_cal(&(pbuff[0]), dlen-2);//crc16_cal(&(pbuff[0]), dlen-2);
			
            uint16_t crcfram = pbuff[dlen-1];
            crcfram = (crcfram<<8)|pbuff[dlen-2];
            if((crcfram == crcvalue)||(crcfram == 0xCCCC))
            {
                return true;
            }
        }
    }
    return flg;
}

//================
//重发广播帧管理    $$$路由表无广播序号
void ResetBroadcastSavedFrame()
{
	for(int i = 0; i < MAX_BROADCAST_SAVED_FRAME_NUM; i++)
	{
		broadcastSavedFrame[i].broadcastSeq = 0;
		broadcastSavedFrame[i].sendLen = 0;
		if(broadcastSavedFrame[i].sendBuff != NULL)
			free(broadcastSavedFrame[i].sendBuff);
		
		broadcastSavedFrame[i].sendBuff = NULL;
		
		//if(IsTimeRun(&(broadcastSavedFrame[i].savedTimer)))
			TimeStop(&(broadcastSavedFrame[i].savedTimer));
	}
	broadcastFrameSavedIndex = -1;
	broadcastSavedFrameCount = 0;
}

bool SaveBroadcastFrame(uint32_t seq, uint16_t len, uint8_t *buff)
{
	if((seq == 0) || (len < WHI_HPLC_ADAPT_HEAD_LEN) || (buff == NULL))
		return false;
	
	whi_hplc_adapt_head_t *plc_head = (whi_hplc_adapt_head_t*)buff;
	plc_head->broadcast_seq = seq;
	
	broadcastFrameSavedIndex++;
	if(broadcastFrameSavedIndex == MAX_BROADCAST_SAVED_FRAME_NUM)
		broadcastFrameSavedIndex = 0;
	
	broadcastSavedFrame[broadcastFrameSavedIndex].broadcastSeq = seq;
	broadcastSavedFrame[broadcastFrameSavedIndex].sendLen = len;
	if(broadcastSavedFrame[broadcastFrameSavedIndex].sendBuff != NULL)
		free(broadcastSavedFrame[broadcastFrameSavedIndex].sendBuff);
	broadcastSavedFrame[broadcastFrameSavedIndex].sendBuff = malloc(len);
	if(broadcastSavedFrame[broadcastFrameSavedIndex].sendBuff == NULL)
	{
		broadcastFrameSavedIndex--;
		if(broadcastFrameSavedIndex == 0)
			broadcastFrameSavedIndex = MAX_BROADCAST_SAVED_FRAME_NUM - 1;
		return false;
	}
	memcpy(broadcastSavedFrame[broadcastFrameSavedIndex].sendBuff, buff, len);
	
	StartTimer(&(broadcastSavedFrame[broadcastFrameSavedIndex].savedTimer), RESEND_BROADCAST_FRAME_SAVE_TIME_MS);
	
	broadcastSavedFrameCount++;
	if(broadcastSavedFrameCount > MAX_BROADCAST_SAVED_FRAME_NUM)
		broadcastSavedFrameCount = MAX_BROADCAST_SAVED_FRAME_NUM;

	if(broadcastSavedFrameCount == 1)
		StartTimer(&ResendBroadcastFrameTimer,RESEND_BROADCAST_FRAME_CHECK_TIME_MS);
	return true;
}

int GetBroadcastFrame(uint32_t seq)
{
	uint32_t minDiff = 0xFFFFFFFF;
	uint32_t diff;
	int index = broadcastFrameSavedIndex;
	int foundIndex = -1;
	for(int i = 0; i < broadcastSavedFrameCount; i++)
	{
		if(TimeOut(&(broadcastSavedFrame[index].savedTimer)))
		{
			broadcastSavedFrame[index].broadcastSeq = 0;
		}
		if(0 != broadcastSavedFrame[index].broadcastSeq)
		{
			if(broadcastSavedFrame[index].broadcastSeq > seq)
			{
				diff = broadcastSavedFrame[index].broadcastSeq - seq;
				if(diff < minDiff)
				{
					minDiff = diff;
					foundIndex = index;
				}
			}
			else
			{
				break;
			}
		}
		index--;
		if(index < 0)
			index = MAX_BROADCAST_SAVED_FRAME_NUM - 1;
	}
	return foundIndex;
}

int GetBetterNeighbor(uint16_t *tei,uint32_t seq)
{
	uint16_t succRate = 0;
	uint16_t upRate = 0;
	int index = -1;
	int foundIndex = -1;
	uint16_t foundTei = 0xFFFF;

	for(int i = 0; i < 1015 + 2; i++ )
	{
		if(lc_net_get_neighbor_some_info(i, &upRate))
		{
			index = GetBroadcastFrame(seq);
			if(index != -1)
			{
				if(succRate <= upRate)
				{
					succRate = upRate;
					foundIndex = index;
					foundTei = i;
				}
			}
		}
	}
	*tei = foundTei;
	return foundIndex;
}

void ResendBroadcastFrame(uint32_t seq)
{
	uint16_t tei = 0;
	int index = -1;

	index = GetBetterNeighbor(&tei,seq);
	if((index != -1) && (tei != 0xFFFF))
	{
		//lc_net_update_neighbor_broadcast_seq(tei, broadcastSavedFrame[index].broadcastSeq);
		lc_net_tx_data_to_tei_broadcast(tei, broadcastSavedFrame[index].sendBuff, broadcastSavedFrame[index].sendLen, LC_NET_NOT_BLOCK);
	}
}


//================
//
/**
* @brief 全屋智能帧分派处理函数
* @param psrc数据缓冲
* @param len 数据长度
* @return 回传数据长度
* @author hh
*/
#define LOCAL_PROCESS_MODE		(0) //本地处理模式
#define LOCAL_BLE_NET_PROCESS_MODE		(1) //ble蓝牙网络处理模式
uint16_t WHI_UartLocalWHIProcess(uint8_t *psrc,uint16_t len,uint8_t process_mode)
{
    uint16_t dlen = 0;
    //if(WHI_IsLocalWHIFrame(psrc,len))
    {
        uint16_t framecmd = (((uint16_t)psrc[3])<<8)|psrc[2];

        //Dir： Dir=0 表示此帧报文是由主控设备发出的下行报文；
        //Dir=1 表示此帧报文是由通信模组发出的上行报文。
        //Prm： Prm=1 表示此帧报文来自启动站；
        //Prm=0 表示此帧报文来自从动站。
        
        //if((psrc[1]&0x80) == 0x00)
        {//主控设备发出报文
			if((psrc[1]&0x40) == 0x40)
			{//启动站报文
				switch(framecmd)
				{
					case WHI_READ_VERSION://     (0x0001)
					dlen = WHI_ReadVersionProcess(psrc,len);
					break;
					case  WHI_READ_MAC://         (0x0002)
					dlen = WHI_ReadMacProcess(psrc,len);
					break;
					case  WHI_READ_ADDR://        (0x0003)
					dlen = WHI_ReadAddrProcess(psrc,len);
					break;
					case  WHI_SET_ADDR://         (0x0004)
					dlen = WHI_SetAddrProcess(psrc,len);
					break;
					case WHI_RESET_MODEL://      (0x0005)
					dlen = WHI_ReSetModelProcess(psrc,len);
					break;
					case WHI_FILE_CMD://         (0x0006)
					dlen = WHI_FileCmdProcess(psrc,len);
					break;
					case WHI_READ_RUN_TIME://    (0x0007)
					dlen = WHI_ReadRunTimeProcess(psrc,len);
					break;  
					
					//==================================================
					case WHI_READ_NODE_SUM:
					dlen = WHI_ReadNodeSumProcess(psrc,len);
					break;
					case WHI_READ_NODE_INFO:
					dlen = WHI_ReadNodeInfoProcess(psrc,len);
					break;
					case WHI_ADD_NODE_INFO:
					dlen = WHI_AddNodeInfoProcess(psrc,len);
					break;
					case WHI_DEL_NODE_INFO:
					dlen = WHI_DelNodeInfoProcess(psrc,len);
					break;
					case WHI_CLEAR_NODE_INFO:
					dlen = WHI_ClearNodeInfoProcess(psrc,len);
					break;
					case WHI_SET_ZZW_STATE:
					dlen = WHI_SetZzwStateProcess(psrc,len);
					break;
					case WHI_SET_RECORD_STATE:
					dlen = WHI_SetRecordStateProcess(psrc,len);
					break;
					case WHI_READ_RECORD_STATE:
					dlen = WHI_ReadRecordStateProcess(psrc,len);
					break;
					case WHI_READ_TOPOLOGY_NODE_SUM://     (0x0020)
					dlen = WHI_ReadTopologyNodeSumProcess(psrc,len);
					break;
					case WHI_READ_TOPOLOGY_NODE_INFO://    (0x0021)
					dlen = WHI_ReadTopologyNodeInfoProcess(psrc,len);
					break;
					//
					case WHI_CFG_ISP_KEY_CMD:
					dlen = WHI_CfgIspKeyCmdProcess(psrc,len);
					break;
					case WHI_READ_ISP_KEY_CMD:
					dlen = WHI_ReadIspKeyCmdProcess(psrc,len);
					break;
					case WHI_CFG_PROJ_KEY_CMD:
					dlen = WHI_CfgProjKeyCmdProcess(psrc,len);
					break;
					case WHI_READ_PROJ_KEY_CMD:
					dlen = WHI_ReadProjKeyCmdProcess(psrc,len);
					break;
					// #ifdef PLAT_LAMP_NET
					case WHI_CFG_NET_ENABLE_FLG_CMD:
					dlen = WHI_CfgNetEnableFlgCmdProcess(psrc,len);
					break;
					case WHI_READ_NET_ENABLE_FLG_CMD:
					dlen = WHI_ReadNetEnableFlgCmdProcess(psrc,len);
					break;
					case WHI_NET_START_BIND_NODE_CMD:
					dlen = WHI_NetStartBindNodeCmdProcess(psrc,len);
					break;
					case WHI_NET_STOP_BIND_NODE_CMD:
					dlen = WHI_NetStopBindNodeCmdProcess(psrc,len);
					break;
					case WHI_NET_QUERY_BIND_NODE_CMD:
					dlen = WHI_NetQueryBindNodeCmdProcess(psrc,len);
					break;
					
					// #endif
					
					case WHI_SEND_HPLC_FRAME://            (0x0100) 发送数据
					dlen = WHI_SendHplcFrameProcess(psrc,len);
					break;
					case WHI_RECEIVE_HPLC_FRAME://         (0x0101) 接收数据
					dlen = WHI_ReceiveHplcFrameProcess(psrc,len);
					break;
					case WHI_SEND_STA_FRAME://             (0x0110) 发送数据
					dlen = WHI_SendStaFrameProcess(psrc,len);
					break;
					case WHI_RECEIVE_STA_FRAME://          (0x0111) 接收数据
					dlen = WHI_ReceiveStaFrameProcess(psrc,len);
					break;
					case WHI_SEND_BUS_FRAME://             (0x0120) 控制转发
					dlen = WHI_SendBusFrameProcess(psrc,len);
					break;
					//=======
					case WHI_LAMP_BLE_NET_CTRL_FRAME://蓝牙网络控制数据
					dlen = WHI_LampBleNetCtrlFrameProcess(psrc,len);
					break;
					// case WHI_LAMP_RADAR_EVENT_REPORT_VALUE:
					// // dlen = WHI_LampRadarEventReportFrameProcess(psrc,len); 
					// break;

                    // case WHI_LAMP_SENSOR_EVENT_REPORT_VALUE:
					// // dlen = WHI_LampSensorEventReportFrameProcess(psrc,len);
					// break;
		
					case WHI_SEND_STA_NO_ACK_FRAME:
					dlen = WHI_SendStaNoAckProcess(psrc,len);	
					break;
					case WHI_RECEIVE_STA_NO_ACK_FRAME:
					dlen = WHI_ReceiveStaNoAckFrameProcess(psrc,len);
					break;
					case WHI_SEND_BUS_TRAN_FRAME:
					dlen = WHI_SendBusTranFrameProcess(psrc,len);
					break;
					case WHI_READ_FREQ_BAND://        (0xF001)
					dlen = WHI_ReadFreqBandProcess(psrc,len);
					break;
					case WHI_SET_FREQ_BAND://         (0xF002)
					dlen = WHI_SetFreqBandProcess(psrc,len);
					break;
					default:
					break;
				}
			}
			else
			{//从动站报文
				switch(framecmd)
				{
					#if 0
					case WHI_SEND_HPLC_FRAME://            (0x0100)  发送数据
					dlen = WHI_SendHplcFrameProcess(psrc,len);
					break;
					case WHI_RECEIVE_HPLC_FRAME://         (0x0101)  接收数据
					dlen = WHI_ReceiveHplcFrameProcess(psrc,len);
					break;
					case WHI_SEND_STA_FRAME://             (0x0110)  发送数据
					dlen = WHI_SendStaFrameProcess(psrc,len);
					break;
					case WHI_RECEIVE_STA_FRAME://          (0x0111)   接收数据
					dlen = WHI_ReceiveStaFrameProcess(psrc,len);
					break;
					case WHI_SEND_BUS_FRAME://             (0x0120)    控制转发
					dlen = WHI_SendBusFrameProcess(psrc,len);
					break;
					case WHI_SEND_STA_NO_ACK_FRAME:
					dlen = WHI_SendStaNoAckProcess(psrc,len);	
					break;
					case WHI_RECEIVE_STA_NO_ACK_FRAME:
					dlen = WHI_ReceiveStaNoAckFrameProcess(psrc,len);
					break;
					case WHI_SEND_BUS_TRAN_FRAME:
					dlen = WHI_SendBusTranFrameProcess(psrc,len);
					break;
					#else
					#if 1
					case WHI_REPORT_IN_NET_NODE_LIST:
						WHI_LampReportInNetNodeListAckFrameProcess(psrc,len);
						break;
					case WHI_REPORT_LEAVE_NET_NODE_LIST:
						WHI_LampReportLeaveNetNodeListAckFrameProcess(psrc,len);
						break;
					case WHI_SEND_BUS_FRAME://             (0x0120)    控制转发
						dlen = WHI_SendBusFrameProcess(psrc,len);
					break;
					#endif
					#endif
					default:
					break;
				}
			}
        }
    }
    return dlen;
}
//==============
bool WHI_ReadVersionAckProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t r_flg = false;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	//
	if(framelen > 0)
	{
		r_flg = lamp_ble_start_read_version_cb_fun(frameseq,WHI_ACK_EOK,pdst,framelen);
	}
	//
	return r_flg;
}
bool WHI_ReadResetInfoAckProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t r_flg = false;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	//
	if(framelen > 0)
	{
		r_flg = lamp_ble_start_read_reset_info_cb_fun(frameseq,WHI_ACK_EOK,pdst,framelen);
	}
	//
	return r_flg;
}

bool WHI_ReadDeviceTypeAckProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t r_flg = false;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	//
	if(framelen > 0)
	{
		r_flg = lamp_ble_device_type_info_cb_fun(frameseq,WHI_ACK_EOK,pdst,framelen);
	}
	//
	return r_flg;
}


bool WHI_ReadRadarParamAckProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t r_flg = true;//false;
   // uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
   // uint8_t *pdst = &psrc[8];
	
	if(framelen > 0)
	{
		//r_flg = lamp_ble_radar_param_version_cb_fun(frameseq,WHI_ACK_EOK,pdst,framelen);
	}
	return r_flg;
}
/*
bool WHI_LampBleWritePowerAckFrameProcess(uint8_t *psrc,uint16_t dlen)
{
	return true;
}

bool WHI_LampRadarWriteLevelNumThrAckFrameProcess(uint8_t *psrc,uint16_t dlen)
{
	return true;
}

bool WHI_LampRadarWriteThresholdValueAckFrameProcess(uint8_t *psrc,uint16_t dlen)
{
	return true;
}*/


volatile uint32_t receive_ble_net_ack_frame = 0;
bool WHI_LampBleNetCtrlAckFrameProcess(uint8_t *psrc,uint16_t dlen)
{
	bool rflg = false;
    //uint16_t rdlen = 0;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
	++receive_ble_net_ack_frame;
	rflg = ble_local_process_ack_frame(pdst,framelen,frameseq);//(frameseq,pdst[0]);
	return rflg;
}


//#define WHI_SET_ADDR         (0x0004)
bool WHI_SetAddrAckProcess(uint8_t *psrc,uint16_t dlen)
{
    uint16_t r_flg = false;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
    //uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];
    //
    //    设置结果 (1byte)
    //    0：成功， 1：失败
    //    失败原因 (1byte)，见 5.3.6
    //    Rsv (2bytes)
    
	r_flg = lamp_ble_set_mac_addr_cb_fun(frameseq,pdst[0]);
   
    
    return r_flg;
}


bool WHI_FileCmdAckProcess(uint8_t *psrc,uint16_t dlen)
{
    bool r_flg = false;
    uint16_t frameseq = (((uint16_t)psrc[5])<<8)|psrc[4];
   // uint16_t framelen = (((uint16_t)psrc[7])<<8)|psrc[6];
    uint8_t *pdst = &psrc[8];

    uint8_t filecmd = pdst[0];
    switch (filecmd)
    {
        case WHI_START_TRAN_FILE:
        {
           r_flg = lamp_ble_updata_send_ack_chekc_frame(frameseq,pdst[2]);
        }
        break;
        case WHI_TRAN_FILE_DATA:
        {
            r_flg = lamp_ble_updata_send_ack_chekc_frame(frameseq,pdst[2]);
        }
        break;
        case WHI_SCAN_FILE_INFO:
        {

        }
        break;
        case WHI_SET_UPDATA_NODE_LIST:
        {

        }
        break;
        default:
        break;
    }

    return r_flg;
}
extern uint8_t set_ble_power_reply;
//收到蓝牙设置功率成功后，给抄控器回复
bool WHI_LampBleWritePowerAckFrameProcess(uint8_t *psrc,uint16_t dlen)
{
	if(set_ble_power_reply == 0)
	{
		set_ble_power_reply = 1;
	}
	return true;
}
#define CMD_SET_RADAR_FACTORY_TEST_PARAM    (0xC009)
#define CMD_READ_BLE_FACTORY_TEST_PARAM     (0xC00A)
extern bool WHI_ReadBleFactoryTestParamAckProcess(uint8_t *psrc,uint16_t dlen);
extern bool WHI_SetRadarFactoryTestParamAckProcess(uint8_t *psrc,uint16_t dlen);

//==============
bool whi_pro_local_add_uart_task_cb_fun(void * exparam,u8* psrc,u16 len,u8 state)
{
	if(psrc != NULL && len != 0)//state == UART_SUCCESS) $$$
	{//接收数据
		bool r_flg = false;
		if(WHI_IsLocalWHIFrame(psrc,len))
		{
			uint32_t framecmd = (((uint32_t)psrc[3])<<8)|psrc[2];

			//Dir： Dir=0 表示此帧报文是由主控设备发出的下行报文；
			//Dir=1 表示此帧报文是由通信模组发出的上行报文。
			//Prm： Prm=1 表示此帧报文来自启动站；
			//Prm=0 表示此帧报文来自从动站。
			
			if((psrc[1]&0x40) == 0x00)
			{//从动站报文
				switch(framecmd)
				{
					//#ifdef PLAT_LAMP
					case WHI_READ_VERSION:
						r_flg = WHI_ReadVersionAckProcess(psrc,len);
						break;
					case WHI_READ_RESET_INFO:
						r_flg = WHI_ReadResetInfoAckProcess(psrc,len);
						break;
					case WHI_READ_DEVICE_TYPE:
						r_flg = WHI_ReadDeviceTypeAckProcess(psrc,len);
						break;
					case WHI_SET_ADDR:
						r_flg = WHI_SetAddrAckProcess(psrc,len);
						break;
					case WHI_FILE_CMD:
						r_flg = WHI_FileCmdAckProcess(psrc,len);
						break;
					case WHI_LAMP_BLE_WRITE_POWER_VALUE:
						r_flg = WHI_LampBleWritePowerAckFrameProcess(psrc,len);
						break;
					// case WHI_LAMP_RADAR_WRITE_PARAM_VALUE:
					// 	//r_flg = WHI_LampRadarWriteThresholdValueAckFrameProcess(psrc,len);
					// 	break;
                    case WHI_LAMP_RADAR_READ_PARAM_VALUE:
						r_flg = WHI_ReadRadarParamAckProcess(psrc,len);
						break;
					// case CMD_SET_LINKAGE_PARAM:
					// 	//r_flg = WHI_LampRadarWriteLevelNumThrAckFrameProcess(psrc,len);
					// 	break;
				//	#endif
					case WHI_LAMP_BLE_NET_CTRL_FRAME:
						r_flg = WHI_LampBleNetCtrlAckFrameProcess(psrc,len);
						break;
					
					case CMD_READ_BLE_FACTORY_TEST_PARAM:
						r_flg = WHI_ReadBleFactoryTestParamAckProcess(psrc,len);
						break;
					case CMD_SET_RADAR_FACTORY_TEST_PARAM:
						r_flg = WHI_SetRadarFactoryTestParamAckProcess(psrc,len);
						break;

					default:
					break;
				}
			}
		}
		return r_flg;
	}
	else
	{
		return true;
	}
}


bool whi_pro_local_add_uart_task(uint8_t *data,uint16_t len,uint16_t pri,uint16_t resend)
{
	return local_send_fram_add_task(data,len,pri,4,resend,whi_pro_local_add_uart_task_cb_fun,NULL);
}

//读取芯片ID
void ReadStaIdInfo(void);
extern void mac_list_flash_init();


extern bool IsBindAddr;
extern void ChangeLocalUartBaud_t(u32 baud);
int local_task_init(void)
{
	u8 addr_temp[] = {0x36, 0x00, 0x00, 0x10, 0xe7, 0x00};
	// uint8_t factory_mac[LC_NET_ADDR_SIZE] = {0};
	ble_molloc_init();
	// ble_init_key_info_from_store();

	ChangeLocalUartBaud_t(256000);
	// StartTimer(&LocalUartReceiveWatchDogTimer,LOCAL_UART_RECEIVE_WATCH_DOG_CHECK_TIME_MS);

	//MAC地址的设置必须要在蓝牙初始化之前操作，蓝牙初始化里会用的MAC地址来做一些操作，比如初始化NetId
	ReadStaIdInfo();
	memcpy(addr_temp,StaChipID.mac,6);
    // TransposeAddr(addr_temp);
    SetMACType(1);
    // SetStaMAC(addr_temp);

	lc_net_set_addr(addr_temp,LC_NET_ADDR_SIZE);
    IsBindAddr = true;
	
	//设置网络事件回调函数
	// lc_net_set_ctrl_self_frame_cb_fun(ctrl_debug_process_cb_fun_process); //$$$回调
	// lc_net_set_rx_data_evnet_cb_fun(lc_net_hplc_rx_event_cb_fun);
	
    lamp_date_init();
	
    lamp_ble_init();
	ble_net_init();
	mac_list_flash_init();

	ResetBroadcastSavedFrame();
    return 0;
}

void neighbor_watching_task()
{
	static bool timer_running = false;
	const ST_NEIGHBOR_TAB_TYPE* tabType = GetNeighborTable();
	if(0 == tabType->NeighborNum)
	{
		if(false == timer_running)
		{
			timer_running = true;
			StartTimer(&NeighborWatchingTimer, NO_NEIGHBOR_TIME_MS);
		}
	}
	else
	{
		timer_running = false;
		TimeStop(&NeighborWatchingTimer);
	}
	
	if(LcTimeOut(&NeighborWatchingTimer))
	{
		REBOOT_SYSTEM();
	}
}

TimeValue LocalUartTestModeTimer;
bool TestMode = true;
extern void TransposeAddr(u8 addr[LONG_ADDR_LEN]);
extern u8 CheckMeterListIndex;
extern uint8_t  HRF_Option;
extern uint8_t  HRF_Channel;

TimeValue ToolTestTimer;

void HPLC_PHY_SetFrequence(u8 fre);

#if 0
u8 tool_test_buf[200] = 
	{0x09 ,0x00 ,0xf0 ,0xff ,0x00 ,0x12 ,0x31 ,0x12 ,0x00 ,0x00 ,0x00 ,0x10 ,0x10 ,0x03 ,0x2e ,0xd0 ,0x00 ,0x00 ,0x00 ,0x00 ,
0x02 ,0x00 ,0x22 ,0x00 ,0xff ,0x0f ,0x00 ,0x10 ,0x11 ,0x0a ,0x02 ,0x00 ,0x00 ,0xd7 ,0x00 ,0x00 ,0x00 ,0x88 ,0x00 ,0x00 ,
0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0xd7 ,0x00 ,0x00 ,0x00 ,0x88 ,0x11 ,0x11 ,
0x11 ,0x11 ,0x11 ,0x11 ,0x00 ,0x81 ,0x00 ,0x00 ,0xe1 ,0x88 ,0x01 ,0xff ,0x10 ,0x00 ,0x00 ,0x00 ,0x88 ,0x00 ,0x00 ,0x00 ,
0xd7 ,0x00 ,0xd6 ,0x48 ,0x21 ,0xa6 ,0x6b ,0x8f ,0x8f ,0xbb ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,
0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,
0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,
0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x0e ,0xfb ,0x97};
#endif
extern void tool_test_callback(int status);

void send_callback(bool_t status)
{
	debug_str(DEBUG_LOG_NET, "send_callback %d\r\n",status);
}
extern void tool_test_OptionCallback(uint32_t IsOpen,uint32_t phase);
extern void HRF_PHY_SetChannel(uint8_t channel, uint8_t option);

//成功工装测试时，为了避免切换频段，每1s设置一次频段
void ToolTest_ParamSet()
{
	if(TRUE == LcTimeOut(&ToolTestTimer) && (TestMode == true))
	{
		uint8_t band = FrequenceGet();
		HPLC_PHY_SetFrequence(HPLCCurrentFreq);
		//HRF_PHY_SetChannel(HRF_Option,HRF_Channel);
		//BPLC_SendImmediate(tool_test_buf,152,tool_test_callback,tool_test_OptionCallback,1);
		//HRF_SendImmediate(2, 2,tool_test_buf,3,send_callback);
		debug_str(DEBUG_LOG_NET, "band = %d %d %d\r\n",band,HRF_Option,HRF_Channel);
		StartTimer(&ToolTestTimer, 1*1000);
	}	
}

void Local_Task(void *arg)
{
	OS_ERR err;

	local_task_init();
	StartTimer(&LocalUartTestModeTimer, 30*1000);
	uint8_t mac1[LC_NET_ADDR_SIZE] = {0};
	uint8_t mac0[LC_NET_ADDR_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	for(int i = 0; i < MAX_METERS_NUM; i++)
	{
		if(0 == memcmp(mac0, ble_system_params.meters_addr[i].mac, LC_NET_ADDR_SIZE))
			break;
		memcpy(mac1, ble_system_params.meters_addr[i].mac, LC_NET_ADDR_SIZE);
		TransposeAddr(mac1);
		AddSystemMeterToList(mac1, (uint8_t)ble_system_params.meters_addr[i].baudrate,DLT_07,UART_8_DATA_BITS,ble_system_params.meters_addr[i].parity,UART_1_STOP_BITS,UART_TX_TTL);
		//不对物模型设置的档案进行校验
		CheckMeterListIndex++;
	}
	StartTimer(&ToolTestTimer, 1*1000);
	//上报数据，初始化完成
	// uint16_t dlen = lc_get_start_local_frame(LocalReceiveBuf);
	// if(dlen > 0)
	// 	local_send_fram_add_task(LocalReceiveBuf,dlen,SEND_LOCAL_ACK_PRIO_VALUE,10,0,NULL,NULL);
    for(;;)
	{
		ToolTest_ParamSet();
		lamp_event_task();
        lamp_date_task();
		
		lamp_ble_task();
		ble_net_process();
		
		if(TRUE == LcTimeOut(&UpdateFinishTimer))
		{
			REBOOT_SYSTEM();
		}
		if(RT_TRUE == LcTimeOut(&LocalUartTestModeTimer) && TestMode == true)
		{
			TestMode = false;
			StartMeterListTaskhandle();
		}
		
		neighbor_watching_task();
	
		OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);	
	}
}

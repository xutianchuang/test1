//#include "../hal/macrodriver.h"
#include "type_define.h"
#include "system_inc.h"
#include "manager_link.h"
//#include "../drv/uart_link.h"
//#include "../drv/timers.h"
#include "rtc.h"
#include "alert.h"
//#include "../sys/system_event.h"
//#include "../dev/dataflash/data_flash.h"
#include "queue.h"
//#include "Storage.h"
#include "Module_def.h"
//#include "data_types.h"
//#include "PLM_AFN_HN.H"
//#include "data_freeze.h"
//#include "Event_Detect.h"
//#include "Data_Report.h"
//#include "Data_Freeze.h"
#include "PLC_AutoRelay.h"
//#include "../par/plc_mfrelay.h"
#include "hplc_receive.h"
#include "plm_2005.h"
#include "hplc_app_def.h"
//#include <string.h>
//#include "../sys/Print.h"
#include "plm_afn.h"
#include <string.h>
#include "Revision.h"
#include "soft_check.h"
#include "CCO_app.h"
#include "hrf_port.h"
#include "hrf_phy.h"
BOOL s_meter_sn_from_zero = FALSE;
extern GLOBAL_CFG_PARA g_PlmConfigPara;
//extern TMN_VER_INFO c_Tmn_ver_info_inner;
#define REAL_CHIP_ID_OFFSET		322
unsigned char g_file_trans_map[UPDATE_FILE_MAP_SIZE];
unsigned char g_updata_firmware_chk[2];
u32 g_update_crc32[2];
extern uint32_t ZC_Time[3];       //如果不接线，则对应的值是0
u8 g_use_hplc_hrf = 0xff;
void PLC_set_fix_relay(P_RT_RELAY relay_info);
USHORT PLC_get_sn_from_addr(UCHAR *addr);
USHORT PLC_read_proc_YL(USHORT sn, UCHAR *pReadMsg, USHORT nMsgLen);
extern void Relay_MF_selfreg(unsigned short secdown, P_RELAY_ADDR pRelay);

unsigned char afn_reset_01(P_LMP_APP_INFO pLmpAppInfo)
{
//    OS_ERR err;

	P_CONFIRM_DATA_UNIT pDataUnit = (P_CONFIRM_DATA_UNIT)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];

	pDataUnit->router_status = ~0;
	pDataUnit->wait_time = 0;

	pLmpAppInfo->responsePoint += sizeof(CONFIRM_DATA_UNIT);

	switch (pLmpAppInfo->curDT)
	{
	case F1:
		{
			Reboot_system(1, en_reboot_cmd, __func__);
			pDataUnit->wait_time = 20;
			break;
		}
	case F2:
		{
#if 1
			debug_str(DEBUG_LOG_NET, "init parameter all meter will be 0\r\n");
#ifdef OVERSEA
			if (g_PlmConfigPara.close_white_list == 2) //关闭白名单，参数初始化后不能查询到STA信息（集中器下发的档案）
			{
				for (int i=0; i<CCTT_METER_NUMBER; i++)
				{
					g_MeterRelayInfo[i].status.cfged = 0;
				}
				PLC_TriggerSaveNodeInfo();
			}
			else
#endif
			{
				PLC_ParamReset();
				g_pRelayStatus->offline_time = 0;
				g_pRelayStatus->pco_change = 0;
			}
#else
			P_TMSG pMsg = alloc_msg(__func__);
			if(NULL != pMsg)
			{
				pMsg->message = MSG_PARAM_CLEAR;
				OSTaskQPost(&g_pOsRes->APP_PLM_2005_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO|OS_OPT_POST_NO_SCHED, &err);
				if(OS_ERR_NONE != err)
				free_msg(__func__,pMsg);
			}
#endif
			pDataUnit->wait_time = 20;
			break;
		}
	case F3:
		{
#if 1
			PLC_DataReset();
			g_pRelayStatus->offline_time=0;
			g_pRelayStatus->pco_change=0;
#else
			P_TMSG pMsg = alloc_msg(__func__);
			if(NULL != pMsg)
			{
				pMsg->message = MSG_DATA_CLEAR;
				OSTaskQPost(&g_pOsRes->APP_PLM_2005_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO|OS_OPT_POST_NO_SCHED, &err);
				if(OS_ERR_NONE != err)
				free_msg(__func__,pMsg);
			}
#endif
			pDataUnit->wait_time = 2;
			return OK;
		}
#ifdef GDW_ZHEJIANG                 
	case F160:
		{
                    USHORT ii;

                    RELAY_INFO *p_RelayInfo = g_MeterRelayInfo;

                    for (ii = 0; ii <  CCTT_METER_NUMBER; ii++, p_RelayInfo++)
                    {
                        if(p_RelayInfo->black_list == 0)
                        {
                            p_RelayInfo->black_list = 1;//0:使能
                        }
                            
                    }
                    PLC_TriggerSaveNodeInfo();
					memset_special(g_BlackMeterInfo,0xff,CCTT_BLACK_METER_NUMBER*sizeof(BLACK_RELAY_INFO));
					PLC_TriggerSaveBlackNodeInfo();
                   return OK;
		}
#endif
	default:
		pLmpAppInfo->responsePoint -= sizeof(CONFIRM_DATA_UNIT);
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}

	return OK;
}

unsigned char afn_func_01(P_LMP_APP_INFO pLmpAppInfo)
{
	unsigned char err_code;

	if (!pLmpAppInfo)
		return ERROR;

	err_code = afn_reset_01(pLmpAppInfo);
	return err_code;
}


unsigned char afn_relay_cmd_02(P_LMP_APP_INFO pLmpAppInfo)
{
	UCHAR *recv_buf = &pLmpAppInfo->curProcUnit->data_unit[0];
	U16 len;
	P_GD_DL645_BODY pDL645 = NULL;
	UCHAR *pDesAddr = NULL;

	if (pLmpAppInfo->lmpLinkHead->Inf.down_info.comm_module)
	{
		u8 level = pLmpAppInfo->lmpLinkHead->Inf.down_info.relay_level;
		pDesAddr = &pLmpAppInfo->lmpLinkHead->user_data[6 + 6 * level];
	}
	switch (pLmpAppInfo->curDT)
	{
	case F1:
	case F2:
		{
			UCHAR protocol;
			protocol=*recv_buf++;
			len = *recv_buf++;;
			if (pLmpAppInfo->curDT==F2)
			{
				len |= *recv_buf<<8;
				++recv_buf;
			}
			if (NULL == pDesAddr)
			{
				pDL645 = PLC_check_dl645_frame(recv_buf, len);
				P_DL69845_BODY pDL698 =  PLC_check_69845_frame(recv_buf, len);
				if (pDL645)
				{
					pDesAddr = pDL645->meter_number;
				}
				else if (pDL698)
				{
					pDesAddr = PLC_get_69845_SAMac(pDL698, NULL);
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
					return ERROR;
				}

			}

			if (PLC_get_sn_from_addr(pDesAddr) >= CCTT_METER_NUMBER)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_METERNO_INEXISTS;
				return ERROR;
			}
#if 1
			u8 dst_addr[LONG_ADDR_LEN] = {0};
			PLC_GetAppDstMac(pDesAddr, dst_addr);
			
			if (PLC_GetStaInfoByMac(dst_addr) == NULL)
#else			
			if (PLC_GetStaInfoByMac(pDesAddr)==NULL)
#endif			
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
				return ERROR;
			}

			P_READ_MSG_INFO pReadMsg = (P_READ_MSG_INFO)alloc_buffer(__func__,sizeof(READ_MSG_HEAD) + len);

			if (NULL == pReadMsg)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}

			pReadMsg->header.broadcast = 0;
			pReadMsg->header.read_count = 3;
			pReadMsg->header.read_time = 0;
			pReadMsg->header.sour_forme = pLmpAppInfo->end_id;
			pReadMsg->header.packet_seq_3762 = pLmpAppInfo->lmpLinkHead->Inf.down_info.seq_no;
			memcpy_special(pReadMsg->header.mac, pDesAddr, MAC_ADDR_LEN);
			pReadMsg->header.meter_protocol_plc = PLC_3762protocol_to_plcprotocol(protocol);
			pReadMsg->header.packet_id = APP_PACKET_COLLECT;
			pReadMsg->header.time_related = 0;
			pReadMsg->header.sender = EM_SENDER_AFN_GDW2013;
			pReadMsg->header.msg_len = len;
			pReadMsg->header.res_afn=AFN_DATA_TRANS;
			if (pLmpAppInfo->curDT==F2)
			{
				pReadMsg->header.res_afn |= 0x80;
			}
			memcpy_special(pReadMsg->body, recv_buf, len);

			PLC_PostDirRead(pReadMsg);

			pLmpAppInfo->confirmFlag = 2;
			break;
		}
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}

	return OK;
}

unsigned char afn_dr_extend_02(P_LMP_APP_INFO pLmpAppInfo)
{
	//unsigned char protocol_type = pLmpAppInfo->curProcUnit->data_unit[0];
	//unsigned char frame_len = pLmpAppInfo->curProcUnit->data_unit[1];
	//unsigned char protocol_type = pLmpAppInfo->curProcUnit->data_unit[0];

	switch (pLmpAppInfo->curDT)
	{
	case F1:       // concentrator ID
		{
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_PlmConfigPara.main_node_addr, 6);
			pLmpAppInfo->responsePoint += 6;
			return OK;
		}
	case F2:       // unfreezed meters
		{
			return OK;
		}
	case F3:       // DL645 items need to read
		{
			return OK;
		}
	case F4:       // get time
		{
			return OK;
		}
	case F5:
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 1;
#if defined(APP_CUS_BAIFU)
			//百富集中器直接返回3.5代
			//pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x01;
#elif defined(APP_CUS_CONGXING_JX)
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x03;
#else
			//pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x03;
#endif
			pLmpAppInfo->responsePoint++;
			return OK;
		}
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}

	//return OK;
}

unsigned char afn_func_02(P_LMP_APP_INFO pLmpAppInfo)
{
	unsigned char res;
	
	if (!pLmpAppInfo)
		return ERROR;

	if (pLmpAppInfo->linkFunc == 0x47)
	{
		afn_dr_extend_02(pLmpAppInfo);
		pLmpAppInfo->confirmFlag = 9;
		return OK;
	}
	else
	{
		res = afn_relay_cmd_02(pLmpAppInfo);
		return res;
	}

	//return ERROR;
}

BYTE AFN03_F11_fill_fns(PBYTE pIfo, PBYTE Fns, BYTE nFnCount)
{
	BYTE i;
	BYTE Fn;

	for (i = 0; i < nFnCount; i++)
	{
		Fn = Fns[i]-1;
		pIfo[Fn / 8] |= 1 << (Fn&0x7);
	}

	return OK;
}
extern u8 nbi_status;
//请求终端配置及信息（AFN=03H）
unsigned char afn_request_info_03(P_LMP_APP_INFO pLmpAppInfo)
{

	switch (pLmpAppInfo->curDT)
	{
	case F1:  //F1：终端版本信息
		{
#ifdef PROTOCOL_GD_2016
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DT;
			return ERROR;
#else

			TMN_VER_INFO ver_inf;
			memcpy(&ver_inf, &c_Tmn_ver_info, sizeof(TMN_VER_INFO));

			ver_inf.softReleaseDate.day = Hex2BcdChar(ver_inf.softReleaseDate.day);
			ver_inf.softReleaseDate.month = Hex2BcdChar(ver_inf.softReleaseDate.month);
			ver_inf.softReleaseDate.year = Hex2BcdChar(ver_inf.softReleaseDate.year);
			
			TransposeByteOrder((UCHAR *)ver_inf.software_v, sizeof(ver_inf.software_v));
			ver_inf.software_v[0] = Hex2BcdChar(ver_inf.software_v[0]);
			ver_inf.software_v[1] = Hex2BcdChar(ver_inf.software_v[1]);

#if 1
			//中惠要求厂商代码和芯片代码写死，不从芯片ID中获取
			TransposeByteOrder((UCHAR *)ver_inf.factoryCode, sizeof(ver_inf.factoryCode));
			TransposeByteOrder((UCHAR *)ver_inf.chip_code, sizeof(ver_inf.chip_code));
#else
			//V2.7要求“厂商代码”和“芯片代码”倒序传输。CCO录入芯片ID（包含厂商代码和芯片代码）时是倒序录入，所以不需要转换字节序
			memcpy(ver_inf.factoryCode, ((P_STA_CHIP_ID_INFO)(g_cco_id.chip_id))->factory_code, sizeof(ver_inf.factoryCode));
			memcpy(ver_inf.chip_code, ((P_STA_CHIP_ID_INFO)(g_cco_id.chip_id))->chip_code, sizeof(ver_inf.chip_code));
#endif
			
			//memset_special(&pLmpAppInfo->lmpLinkHead->Inf, 0, sizeof(pLmpAppInfo->lmpLinkHead->Inf));
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &ver_inf, sizeof(TMN_VER_INFO)-4);
			pLmpAppInfo->responsePoint += sizeof(TMN_VER_INFO)-4;
			break;
#endif

		}
	case F2:  //噪声值
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x03;
			break;
		}
	case F3: //监听从节点信息
		{
			u16 start = pLmpAppInfo->curProcUnit->data_unit[0];
			u16 num = pLmpAppInfo->curProcUnit->data_unit[1];
			if (num > 16)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}
			
			P_STA_BITMAP pStaBitmap = PLC_GetCCONeighbor();
			if(NULL == pStaBitmap)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_CRC;
				return ERROR;
			}
			
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pStaBitmap->count > 0xff ? 0xff : pStaBitmap->count;
			num = (pStaBitmap->count - start) > num ? num : pStaBitmap->count - start;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = num;
			P_AFN03_F03 pinfo = (P_AFN03_F03)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			for (int i = 0; i < pStaBitmap->size; i++)
			{
				if (pStaBitmap->bitmap[i])
				{
					for (int j = 0; j < 8; j++)
					{
						if (pStaBitmap->bitmap[i] & (1 << j))
						{
							if (start)
							{
								--start;
								continue;
							}
                                                        
							if (num-- == 0)
							{
								break;
							}
                                                        
							P_STA_INFO	p_sta = PLC_GetValidStaInfoByTei((i << 3) | j);
							memset(pinfo, 0, sizeof(AFN03_F03));
							if (p_sta)
							{

								memcpy(pinfo->meter_addr, p_sta->mac, LONG_ADDR_LEN);

								s8 snr = p_sta->snr;
								if (snr < 0)
								{
									snr = 0;
								}
								else if (snr > 0xf)
								{
									snr = 0xf;
								}
								pinfo->snr = snr;
								pinfo->relay_level = PLC_GetStaLevel((i << 3) | j);

							}
							P_STA_STATUS_INFO pStaStatus = PLC_GetStaStatusInfoByTei((i << 3) | j);
							pinfo->rec_time = pStaStatus ? pStaStatus->found_list_counter_this_route : 0;
							pinfo++;
							pLmpAppInfo->responsePoint += sizeof(AFN03_F03);
						}

					}
				}
			}
			free_buffer(__func__,pStaBitmap);
			break;
		}
	case F4: //F4：载波主节点地址
		{
			u8 new_main_node[6];
#if 1
			memcpy_special(new_main_node, g_PlmConfigPara.main_node_addr, 6);
#else
			memcpy_swap(new_main_node, g_PlmConfigPara.main_node_addr, 6);
#endif
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, new_main_node, 6);
			pLmpAppInfo->responsePoint += 6;
#if 0
			{
				//载波CCO MAC 地址 00 A2 93 20 49 19
				//u8 test_main_node[] = {0x00, 0xA2, 0x93, 0x20, 0x49, 0x19};
				u8 test_main_node[] =
				{0x00, 0xA2, 0x93, 0x20, 0x29, 0x41};
				memcpy_swap(g_PlmConfigPara.main_node_addr, test_main_node, 6);
			}
#endif

			break;
		}
	case F5: //F5：载波主节点状态字和载波速率
		{
			BYTE mn_status[] = { 0x34, 0x03, 0xB0, 0x04, 0x58, 0x02, 0x64, 0x00, 0x32, 0x00 };        //鼎信模块返回值，七叶集中器认此命令
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &mn_status[0], sizeof(mn_status));
			pLmpAppInfo->responsePoint += sizeof(mn_status);
			break;
		}
	case F6://主节点干扰状态
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = nbi_status;
			break;
		}
	case F7: //5.5.4.1.3.7　F7：读取从节点监控最大超时时间
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_PlmConfigPara.plc_max_timeout;
		break;
	case F8: //设置无线通讯参数（适用于小无线 双模仅仅应对台体
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_PlmConfigPara.RF_CH;
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_PlmConfigPara.RF_PO;
		break;
	case F9: //5.5.4.1.3.9  F9：通信延时相关广播通信时长
		{
			BYTE protocol;
			BYTE len;
			PBYTE pData;
			USHORT time_delay = 10;
			BYTE addr_field[2][6];
			P_GD_DL645_BODY pDL645;

			protocol = pLmpAppInfo->curProcUnit->data_unit[0];
			len = pLmpAppInfo->curProcUnit->data_unit[1];
			pData = &pLmpAppInfo->curProcUnit->data_unit[2];

			pDL645 = PLC_check_dl645_frame(pData, len);

			if (pDL645)
			{
				memcpy_special(&addr_field[0][0], g_PlmConfigPara.main_node_addr, 6);
				memcpy_special(&addr_field[1][0], pDL645->meter_number, 6);
				time_delay = 3;
			}

			*((USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint])) = time_delay;
			pLmpAppInfo->responsePoint += 2; //延时时间
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = protocol;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = len;
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pData, len);
			pLmpAppInfo->responsePoint += len;

			break;
		}
	case F10: //5.5.4.2.2.10  F10：本地通信模块运行模式信息
		{
			pLmpAppInfo->responsePoint += PLC_Get_Router_Ifo((P_ROUTER_IFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
#if 0
			{
				BYTE tmp[] =
				{0x68, 0x38, 0x00, 0x81, 0x01, 0x00, 0x40, 0x00, 0x00, 0xD2, 0x03, 0x02, 0x01, 0x41, 0x36, 0x60, 0x01, 0x00, 0x00, 0x23, 0x5A, 0x00, 0xFE, 0x00, 0x82, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x07, 0x00, 0x00, 0x18, 0x09, 0x13, 0x21, 0x03, 0x13, 0x30, 0x31, 0x31, 0x31, 0x11, 0x10, 0x13, 0x00, 0x01, 0x00, 0x00};
				memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint-sizeof(tmp)], tmp, sizeof(tmp));
			}
#endif
			break;
		}
	case F11: //5.5.4.2.2.11  F11：本地通信模块376.2报文支持信息
		{
			BYTE afn = pLmpAppInfo->curProcUnit->data_unit[0];
			PBYTE pIfo;

			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = afn;
			pIfo = &pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			memset_special(pIfo, 0, 32);
			pLmpAppInfo->responsePoint += 32;

			if (AFN_RESPONSE == afn)
			{
				BYTE fns[] = { F1, F2 };
				AFN03_F11_fill_fns(pIfo, fns, sizeof(fns));
			}
			else if (AFN_RESET == afn)
			{
				BYTE fns[] = { F1, F2, F3 };
				AFN03_F11_fill_fns(pIfo, fns, sizeof(fns));
			}
			else if (AFN_DATA_TRANS == afn)
			{
				BYTE fns[] = { F1 };
				AFN03_F11_fill_fns(pIfo, fns, sizeof(fns));
			}
			else if (AFN_READ_PARAM == afn)
			{
				BYTE fns[] = { F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F16};
				AFN03_F11_fill_fns(pIfo, fns, sizeof(fns));
			}
			else if (AFN_MAC_CHECK == afn)
			{
				BYTE fns[] = { F1, F2, F3 };
				AFN03_F11_fill_fns(pIfo, fns, sizeof(fns));
			}
			else if (AFN_CTL_CMD == afn)
			{
				BYTE fns[] = { F1, F2, F3, F4, F5, F6, F16 
#ifdef GDW_ZHEJIANG
					,F30
#endif
					};
				AFN03_F11_fill_fns(pIfo, fns, sizeof(fns));
			}
			else if (AFN_REPORT_INFO == afn)
			{
				BYTE fns[] = { F1, F2, F3, F4, F5 };
				AFN03_F11_fill_fns(pIfo, fns, sizeof(fns));
			}
			else if (AFN_RELAY_QUERY == afn)
			{
				BYTE fns[] = { F1, F2, F3, F4, F5, F6, F7, F9, F21, F31, F40, F104, F111, F112 };
				AFN03_F11_fill_fns(pIfo, fns, sizeof(fns));
			}
			else if (AFN_RELAY_SETTING == afn)
			{
				BYTE fns[] = { F1, F2, F3, F4, F5, F6 };
				AFN03_F11_fill_fns(pIfo, fns, sizeof(fns));
			}
			else if (AFN_RELAY_CTROL == afn)
			{
				BYTE fns[] = { F1, F2, F3 };
				AFN03_F11_fill_fns(pIfo, fns, sizeof(fns));
			}
			else if (AFN_FRAME_FORWARD == afn)
			{
				BYTE fns[] = { F1 };
				AFN03_F11_fill_fns(pIfo, fns, sizeof(fns));
			}
			else if (AFN_REAME_READ == afn)
			{
				BYTE fns[] = { F1, F2, F3 };
				AFN03_F11_fill_fns(pIfo, fns, sizeof(fns));
			}
			else if (AFN_FEIL_TRANSFER == afn)
			{
				BYTE fns[] = { F1 };
				AFN03_F11_fill_fns(pIfo, fns, sizeof(fns));
			}
			else if (AFN_PARALLEL_READ_METER == afn)
			{
				BYTE fns[] = { F1 };
				AFN03_F11_fill_fns(pIfo, fns, sizeof(fns));
			}
			else
			{
			}
			break;
		}
	case F12:       //应用手册, 03F12查询本地主节点通信模块ID信息
		{
#if 1
			//中惠要求厂商代码和芯片代码写死，不从芯片ID中获取
			memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], c_Tmn_ver_info.factoryCode, 2);
			TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2);
#else
			memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], ((P_STA_CHIP_ID_INFO)(g_cco_id.chip_id))->factory_code, 2);
#endif
			pLmpAppInfo->responsePoint += 2;

			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = sizeof(g_cco_id.module_id);      //模块ID长度，根据面向流水线协议
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x01;   //模块ID格式, bin
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_cco_id.module_id, sizeof(g_cco_id.module_id));
			pLmpAppInfo->responsePoint += sizeof(g_cco_id.module_id);

			break;
		}
	case F16:
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_pPlmConfigPara->hplc_comm_param;
			break;
		}
	case F17://读取无线频段
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_pPlmConfigPara->hrf_option;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_pPlmConfigPara->hrf_channel;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_pPlmConfigPara->hrf_allowChangeChannel;
			break;
		}
#ifdef GDW_JIANGXI
	case F18: //读取组网方式
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_PlmConfigPara.network_mode;
		break;
#endif
	case F21://海外查询从节点厂商代码和版本信息
		{
			P_STA_INFO pSta = PLC_GetStaInfoByMac(pLmpAppInfo->curProcUnit->data_unit);
			if (NULL == pSta)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}

			P_TMN_VER_INFO pVer = (P_TMN_VER_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			pLmpAppInfo->responsePoint += sizeof(TMN_VER_INFO) - sizeof(pVer->write_flag);


			//中惠要求厂商代码和芯片代码写死，不从芯片ID中获取
			memcpy_special(pVer->factoryCode, pSta->ver_factory_code, sizeof(pVer->factoryCode));
			memcpy_special(pVer->chip_code, pSta->ver_chip_code, sizeof(pVer->chip_code));
			TransposeByteOrder((u8*)pVer->factoryCode, sizeof(pVer->factoryCode));
			TransposeByteOrder((u8*)pVer->chip_code, sizeof(pVer->chip_code));

			memcpy_special(pVer->software_v, pSta->software_v, sizeof(pSta->software_v));
			TransposeByteOrder((u8*)pVer->software_v, sizeof(pVer->software_v));

			if ((pSta-g_pStaInfo) == TEI_CCO)
			{
				pVer->software_v[0] = Hex2BcdChar(pVer->software_v[0]);
				pVer->software_v[1] = Hex2BcdChar(pVer->software_v[1]);

				pVer->softReleaseDate.day = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.day);
				pVer->softReleaseDate.month = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.month);
				pVer->softReleaseDate.year = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.year);
			}
			else
			{
				pVer->softReleaseDate.day = Hex2BcdChar(pSta->year_mon_data.Day);
				pVer->softReleaseDate.month = Hex2BcdChar(pSta->year_mon_data.Month);
				pVer->softReleaseDate.year = Hex2BcdChar(pSta->year_mon_data.Year);
			}
		}
		break;
#if defined(OVERSEA)&&defined(GDW_2019_GRAPH_STOREDATA)
#error "OVERSEA&GDW_2019_GRAPH_STOREDATA macro!"
#endif
#ifdef OVERSEA	
	case F102:
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = (g_PlmConfigPara.close_white_list==0)?0x5A:0x5F;
			break;
		}
#endif	
#ifdef GDW_2019_GRAPH_STOREDATA		
	case F102: //查询模块曲线存储数据项
		{
			P_PLC_STORE_DATA_SYNC_CFG_CB pcb = &g_pRelayStatus->store_data_sync_cfg_cb;
			u8 protocol = pLmpAppInfo->curProcUnit->data_unit[0];
#ifdef GDW_HUNAN //湖南规范增加获取启停标志和采集周期，20210604正式稿
			u8 buff_none[9] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}; //没有设置数据回复
#else
			u8 buff_none[7] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}; //没有设置数据回复
#endif

			if(protocol == METER_PROTOCOL_3762_07)
			{
				if(pcb->meter_645_data_len == 0)
				{
#ifdef GDW_HUNAN //湖南规范增加获取启停标志和采集周期，20210604正式稿
					buff_none[2] = protocol;
#else
					buff_none[0] = protocol;
#endif
					memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], buff_none, sizeof(buff_none));
					pLmpAppInfo->responsePoint += sizeof(buff_none);
				}
				else
				{
#ifdef GDW_HUNAN //湖南规范增加获取启停标志和采集周期，20210604正式稿
					memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pcb->meter_645_data, pcb->meter_645_data_len);
					pLmpAppInfo->responsePoint += pcb->meter_645_data_len;
#else
					memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pcb->meter_645_data+2, pcb->meter_645_data_len-2);
					pLmpAppInfo->responsePoint += pcb->meter_645_data_len-2;
#endif
				}
			}
			else if(protocol == METER_PROTOCOL_3762_69845)
			{
				if(pcb->meter_698_data_len == 0)
				{
#ifdef GDW_HUNAN //湖南规范增加获取启停标志和采集周期，20210604正式稿
					buff_none[2] = protocol;
#else
					buff_none[0] = protocol;
#endif
					memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], buff_none, sizeof(buff_none));
					pLmpAppInfo->responsePoint += sizeof(buff_none);
				}
				else
				{
#ifdef GDW_HUNAN //湖南规范增加获取启停标志和采集周期，20210604正式稿
					memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pcb->meter_698_data, pcb->meter_698_data_len);
					pLmpAppInfo->responsePoint += pcb->meter_698_data_len;
#else				
					memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pcb->meter_698_data+2, pcb->meter_698_data_len-2);
					pLmpAppInfo->responsePoint += pcb->meter_698_data_len-2;
#endif
				}
			}
					
			break;
		}
#endif	
#ifdef GDW_HUNAN
	case F130:
		{
			pLmpAppInfo->responsePoint+=GetSoftInfo((char*)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			break;
		}
	case F131:
		{
			u32 start_addr=*(u32*)&pLmpAppInfo->curProcUnit->data_unit[1];
			u16 len = *(u16*)&pLmpAppInfo->curProcUnit->data_unit[5];
			GetCodeSha1(start_addr,len,&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			pLmpAppInfo->responsePoint+=20;
			break;
		}
	case F201://查询sta认证
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]=g_pRelayStatus->set_sta_authen_cb.isEnable;
			break;
		}
#endif
	case F230:  //F230：内部版本
		{
			unsigned char *ver_inf;
			ver_inf = (UCHAR *)&c_Tmn_ver_info_inner;
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, ver_inf, sizeof(TMN_VER_INFO)-4);
			pLmpAppInfo->responsePoint += sizeof(TMN_VER_INFO)-4;

#if defined(GDW_2012)               //国网13
	#if defined(APP_CUS_GDW13_CJQ)
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x41;
	#else
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x40;
	#endif
#elif defined(PROTOCOL_GD_2016)     //南网16
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x80;
#else                               //国网09
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x00;
#endif

			break;
		}
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}

	return OK;
}


unsigned char afn_mac_check_04(P_LMP_APP_INFO pLmpAppInfo)
{
	switch (pLmpAppInfo->curDT)
	{
#ifndef OVERSEA
	case F1:  //发送测试（主/从节点检测命令）
		{
			//unsigned char time_len = pLmpAppInfo->curProcUnit->data_unit[0];
			break;
		}
#endif
	case F2: //F2：载波从节点点名
		{
			break;
		}
	case F3:    //本地通讯模块报文通信测试
		{
			P_AFN04_F3 pDown = (P_AFN04_F3)pLmpAppInfo->curProcUnit->data_unit;

			u16 node_sn = PLC_get_sn_from_addr(pDown->mac_addr);

			if (node_sn >= CCTT_METER_NUMBER)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_METERNO_INEXISTS;
				return ERROR;
			}

			P_COMM_TEST_INFO pTestMsg = (P_COMM_TEST_INFO)alloc_buffer(__func__,sizeof(COMM_TEST_INFO) + pDown->len);

			if (NULL == pTestMsg)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_BUSY_NOW;
				return ERROR;
			}

			memcpy_special(pTestMsg->mac, pDown->mac_addr, MAC_ADDR_LEN);
			pTestMsg->meter_protocol_plc = (APP_METER_PRO)PLC_3762protocol_to_plcprotocol(pDown->protocol);
			pTestMsg->len = pDown->len;
			memcpy_special(pTestMsg->data, pDown->data, pDown->len);

			PLC_CommTest(pTestMsg);

			break;
		}
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}

	return OK;
}

extern u32 nodeinfo_bitmap[MAX_BITMAP_SIZE/4];
extern u32 nodeinfo_report_bitmap[MAX_BITMAP_SIZE/4];

unsigned char afn_ctl_cmd_05(P_LMP_APP_INFO pLmpAppInfo)
{
	switch (pLmpAppInfo->curDT)
	{
	case F1:  //F1: 设置载波主节点地址
		{
			u8 new_main_node[6]={0x88,0x88,0x88,0x99,0x99,0x02};
			if (memcmp(pLmpAppInfo->curProcUnit->data_unit, new_main_node, 6)==0)//主节点地址是测试功耗地址
			{
				//使用最长的信标和路由周期来降低功耗
				g_PlmConfigPara.forceLongTime=1;
			}
#if 1
			memcpy_special(new_main_node, pLmpAppInfo->curProcUnit->data_unit, 6);
#else
			memcpy_swap(new_main_node, pLmpAppInfo->curProcUnit->data_unit, 6);
#endif
			if (memcmp(g_PlmConfigPara.main_node_addr, new_main_node, 6) != 0)
			{
				memcpy_special(g_PlmConfigPara.main_node_addr, new_main_node, 6);
				SYS_STOR_MAIN_NODE(g_PlmConfigPara.main_node_addr);
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(TEI_CCO);
				if (NULL != pSta)
				{
					PLC_CopyCCOMacAddr(pSta->mac, FALSE);
				}
			}
			break;
		}
	case F2:  //F2：允许/禁止从节点上报
		{
			//兼容09和13规约
			if(pLmpAppInfo->appLayerLen == 3) //09规约（用户数据长度，AFN+Fn）
			{
				g_PlmStatusPara.report_meter_event = 1;
			}
			else if(pLmpAppInfo->appLayerLen == 4) //13规约（用户数据长度，AFN+Fn+数据1字节）
			{
				if (pLmpAppInfo->curProcUnit->data_unit[0] > 1)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
					return ERROR;
				}
				
				g_PlmStatusPara.report_meter_event = pLmpAppInfo->curProcUnit->data_unit[0];
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}
		
			break;
		}
	case F3: // F3: 启动广播，不需要回复
		{
			P_MSG_INFO  p_PlcMsg = NULL;
			UCHAR *data_buf;

			P_CONFIRM_DATA_UNIT pDataUnit;

			data_buf = pLmpAppInfo->curProcUnit->data_unit;

			USHORT data_len = data_buf[1];
#ifdef OVERSEA
			//报文长度2字节，文档V0.6开始	
			data_len = *(u16 *)&data_buf[1];
#endif
			if (!(p_PlcMsg = (P_MSG_INFO)alloc_send_buffer_by_len(__func__, data_len)))
			{
				//Alert(ALERT_NO_MEMORY, ALERT_NO_ACTION, __FILE__, __LINE__);
				return ERROR;
			}

			p_PlcMsg->msg_header.protocol = data_buf[0];
			p_PlcMsg->msg_header.msg_len = data_len;
#ifdef OVERSEA
			memcpy_special(p_PlcMsg->msg_buffer, &data_buf[3], data_len);
#else
//			p_PlcMsg->msg_header.meter_moudle=0;
			memcpy_special(p_PlcMsg->msg_buffer, &data_buf[2], data_len);
#endif
#ifndef DEBUG_FRMAE_ACCURATETIMING
			p_PlcMsg->msg_header.time_stamp=GetTickCount64();
#endif
#ifdef HPLC_GDW
#ifdef DEBUG_FRMAE_ACCURATETIMING
			p_PlcMsg->msg_header.time_stamp=GetNtbCount64();//精准校时测试，后续此处应为NTB而不是tick
			p_PlcMsg->msg_header.accutate_timing=1;//精准校时测试,后续在精准校时对应1376.2处理逻辑中置1
#else
			p_PlcMsg->msg_header.accutate_timing=0;
#endif
#endif
			PLC_Broadcast(p_PlcMsg);
			
			pDataUnit = (P_CONFIRM_DATA_UNIT)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];

			pDataUnit->router_status = ~0;
			pDataUnit->wait_time = 60;      //等待时间       900

			pLmpAppInfo->responsePoint += sizeof(CONFIRM_DATA_UNIT);
			break;
		}
#ifdef GDW_2012
	#ifndef OVERSEA
	case F4:  //5.5.6.1.3.4  F4：设置从节点监控最大超时时间
		{
//			if (pLmpAppInfo->curProcUnit->data_unit[0]<DIR_READ_MIN_TIMEOUT_SEC)
//			{
//				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
//				return ERROR;
//			}
			if (g_PlmConfigPara.plc_max_timeout != pLmpAppInfo->curProcUnit->data_unit[0])
			{
				g_PlmConfigPara.plc_max_timeout = pLmpAppInfo->curProcUnit->data_unit[0];
				dc_flush_fram(&g_PlmConfigPara.plc_max_timeout, 1, FRAM_FLUSH);
			}
			break;
		}
	#else
	case F4://海外
		{
		//遵循原内部版本配置
			if (pLmpAppInfo->curProcUnit->data_unit[0]==0x5a)
			{
				pLmpAppInfo->curProcUnit->data_unit[0] = 0; //开启白名单
			}
			else
			{
				pLmpAppInfo->curProcUnit->data_unit[0] = 2; //关闭白名单且档案不写入flash
			}

			if (pLmpAppInfo->curProcUnit->data_unit[0] != g_PlmConfigPara.close_white_list)
			{
				g_PlmConfigPara.close_white_list = pLmpAppInfo->curProcUnit->data_unit[0];
				SYS_STOR_WHITE_LIST_SWITCH(g_PlmConfigPara.close_white_list);

				if (g_PlmConfigPara.close_white_list == 0) //开启白名单
				{
					PLC_SelfRegClearSendOffline(); //集中器未下发档案的STA离线
				}
				else
				{
					memset(nodeinfo_bitmap, 0, sizeof(nodeinfo_bitmap));
					memset(nodeinfo_report_bitmap, 0, sizeof(nodeinfo_report_bitmap));
					
					for (u16 tei=TEI_STA_MIN; tei<TEI_CNT; tei++)
					{
						P_STA_INFO p_sta = PLC_GetValidStaInfoByTei(tei);
						if (p_sta && (p_sta->net_state==NET_IN_NET))
						{
							nodeinfo_bitmap[tei>>5] |= 1<<(tei&0x1f);
						}
					}
				}
			}
			break;
		}
	#endif
	case F5:    //原来的小无线通信参数
		g_PlmConfigPara.RF_CH=pLmpAppInfo->curProcUnit->data_unit[0];
		g_PlmConfigPara.RF_PO=pLmpAppInfo->curProcUnit->data_unit[1];
		break;
	case F6:    //台区识别
		{
			if (pLmpAppInfo->curProcUnit->data_unit[0])
			{
				debug_str(DEBUG_LOG_NET, "PLC_StartAreaDiscernment, GW\r\n");
				PLC_StartAreaDiscernment();
			}
			else
			{
				debug_str(DEBUG_LOG_NET, "PLC_StopAreaDiscernment, GW\r\n");
				PLC_StopAreaDiscernment((void*)1);
			}
			break;
		}
#endif
	case F10://双模新增修改串口波特率
		{
			//修改波特率
			P_TMSG pMsg = alloc_msg(__func__);
			if (NULL != pMsg)
			{
				OS_ERR err;
				pMsg->message = MSG_PLM_SET_BAUDRATE;
				pMsg->wParam = pLmpAppInfo->curProcUnit->data_unit[0];
				if (pMsg->wParam==0)
				{
					pMsg->wParam=9600<<8;
					g_PlmConfigPara.baud_rate = 9600;
				}
				else if (pMsg->wParam==1)
				{
					pMsg->wParam=19200<<8;
					g_PlmConfigPara.baud_rate = 19200;
				}
				else if (pMsg->wParam==2)
				{
					pMsg->wParam=38400<<8;
					g_PlmConfigPara.baud_rate = 38400;
				}
				else if (pMsg->wParam==3)
				{
					pMsg->wParam=57600<<8;
					g_PlmConfigPara.baud_rate = 57600;
				}
				else if (pMsg->wParam==4)
				{
					pMsg->wParam=115200<<8;
					g_PlmConfigPara.baud_rate = 115200;
				}
				else
				{
					free_msg(__func__,pMsg);
					break;
				}
				OSTaskQPost(&g_pOsRes->APP_PLM_2005_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);
				if (OS_ERR_NONE != err)
					free_msg(__func__,pMsg);
			}
			break;
		}
	case F16:       //wb 设置工作频段
		{
			//河北电科院台体设置频段后，等待360秒
			u8 new_frequence = pLmpAppInfo->curProcUnit->data_unit[0];
			//if(g_pPlmConfigPara->hplc_comm_param != new_frequence)
			{
				if (new_frequence <= 3)
				{
					g_pPlmConfigPara->hplc_comm_param = new_frequence;
					SYS_STOR_FREQ(g_pPlmConfigPara->hplc_comm_param);
#if !defined(FUNC_HPLC_READER)
					PLC_ChangeFrequence(g_pPlmConfigPara->hplc_comm_param);
#else
					HPLC_ChangeFrequence(g_pPlmConfigPara->hplc_comm_param, FALSE);
#endif
				}
			}

			break;
		}
	case F17://设置无线频段
		{
			u8 newchannel, newoption;
			newoption = pLmpAppInfo->curProcUnit->data_unit[0];
			newchannel = pLmpAppInfo->curProcUnit->data_unit[1];
			g_pPlmConfigPara->hrf_allowChangeChannel = pLmpAppInfo->curProcUnit->data_unit[2];
			if (g_pPlmConfigPara->hrf_option == newoption && g_pPlmConfigPara->hrf_channel==newchannel)
			{
				break;
			}
			if(((newoption == 2) && (newchannel < 1 || newchannel > 80)) || ((newoption == 3) && (newchannel < 1 || newchannel > 200)))
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			uint64_t tick_now = GetTickCount64();
			if (tick_now > 11 * 1000) //30 s后cco开始发送信标若要修改信道需要信道切换流程
			{
				HRF_ChangeFrequence(newchannel, newoption);
			}
			else //11s内直接修改
			{
				HRF_ChangeChannel(newchannel, newoption);
			}

			break;
		}
#ifdef GDW_JIANGXI
	case F18:  //设置组网方式
		{
			if (pLmpAppInfo->appLayerLen == 4) //（用户数据长度，AFN+Fn+数据1字节）
			{
				if (pLmpAppInfo->curProcUnit->data_unit[0] >= NETWORK_MODE_INVALID)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
					return ERROR;
				}
				g_PlmConfigPara.network_mode = pLmpAppInfo->curProcUnit->data_unit[0];
				SYS_STOR_NETWORK_MODE();
				if (g_PlmConfigPara.network_mode == NETWORK_MODE_PLC) //仅允许载波组网
				{
					max_beacon_period_ms = 10;
				}
				else if (g_PlmConfigPara.network_mode == NETWORK_MODE_HRF)
				{
					max_beacon_period_ms = 15;
				}
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}
			break;
		}
#endif
#if defined(GDW_SHENYANG)			
	case F80://设置抄控通道
		{
			u8 newLink;
			newLink = pLmpAppInfo->curProcUnit->data_unit[0];
			//g_pPlmConfigPara->hrf_allowChangeChannel = pLmpAppInfo->curProcUnit->data_unit[2];
			g_use_hplc_hrf =newLink;
			break;
		}	
	case F81://设置无线参数
		{
			u8 newchannel, newoption;
			newoption = pLmpAppInfo->curProcUnit->data_unit[0];
			newchannel = pLmpAppInfo->curProcUnit->data_unit[1];
			g_pPlmConfigPara->hrf_allowChangeChannel = 0;
			if (g_pPlmConfigPara->hrf_option == newoption && g_pPlmConfigPara->hrf_channel==newchannel)
			{
				break;
			}
			else
				HPLC_ChangeHrfChannelNotSave(newchannel, newoption);
			g_use_hplc_hrf =FreamPathHrf;
			break;
		}	
	case F82://无线射频控制
		{
			u8 newStatus;
			newStatus = pLmpAppInfo->curProcUnit->data_unit[0];
			if(newStatus == 2)//关闭
				g_use_hplc_hrf =FreamPathBplc;
			
			break;
		}	
#endif
#ifdef GDW_ZHEJIANG
	case F30://设置从节点模块在网锁定时间和异常离网锁定时间
		{
			u32 OnlineLockTime, abnormalOfflineLockTime;
			memcpy_special(&OnlineLockTime, pLmpAppInfo->curProcUnit->data_unit, 2);
			memcpy_special(&abnormalOfflineLockTime, pLmpAppInfo->curProcUnit->data_unit+2, 2);

			PLC_SetOnlineLockTime(OnlineLockTime,abnormalOfflineLockTime);
			break;
		}
#endif
#ifdef GDW_2019_GRAPH_STOREDATA		
	case F103: //设置模块曲线存储数据项
		{
			P_PLC_STORE_DATA_SYNC_CFG_CB pcb = &g_pRelayStatus->store_data_sync_cfg_cb;
			u16 meter_1phase_data_len = 0;
			u16 meter_3phase_data_len = 0;
			u16 total_data_len = 0;

			P_AFN05_F101 pDataUnit = (P_AFN05_F101)pLmpAppInfo->curProcUnit->data_unit;

			meter_1phase_data_len = pDataUnit->meter_1phase_data_num*5; //4(数据标识)+1(回复长度)
			meter_3phase_data_len = (*(pDataUnit->data+meter_1phase_data_len+1))*5;
			total_data_len = sizeof(AFN05_F101) + meter_1phase_data_len + 3 + meter_3phase_data_len;
			
			//限制单相表或三相表曲线数据不超过20项（100/5）
			if((meter_1phase_data_len > 100)||(meter_3phase_data_len > 100))
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_LEN;
				return ERROR;
			}

			if(pDataUnit->protocol == METER_PROTOCOL_3762_07)
			{
				if(memcmp(pLmpAppInfo->curProcUnit->data_unit, pcb->meter_645_data, total_data_len) == 0)
				{
					return SUCCESS;
				}
			}
			else if(pDataUnit->protocol == METER_PROTOCOL_3762_69845)
			{
				if(memcmp(pLmpAppInfo->curProcUnit->data_unit, pcb->meter_698_data, total_data_len) == 0)
				{
					return SUCCESS;
				}
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}

			P_STORE_DATA_SYNC_CFG pStoreDataSyncCfg = (P_STORE_DATA_SYNC_CFG)alloc_buffer(__func__,sizeof(STORE_DATA_SYNC_CFG)+total_data_len);
			if (NULL == pStoreDataSyncCfg)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_BUSY_NOW;
				return ERROR;
			}
			
			pStoreDataSyncCfg->protocol = pDataUnit->protocol;
			pStoreDataSyncCfg->data_len = total_data_len;
			memcpy(pStoreDataSyncCfg->data, pLmpAppInfo->curProcUnit->data_unit, total_data_len);

			//通知存储曲线数据采集方案控制块发起方案同步
            PLC_StoreDateSyncCfg(pStoreDataSyncCfg);
					
			break;
		}
	case F102: //存储曲线数据，设置路由请求终端时钟和全网广播周期
		{
			u32 period = pLmpAppInfo->curProcUnit->data_unit[0];
			u8 unit = pLmpAppInfo->curProcUnit->data_unit[1];

			if(unit == 2) //单位小时
			{
				period = period*60*60; //转成秒
			}
			else //1-单位分钟
			{
				period = period*60; //转成秒
			}

			if(period != g_PlmStatusPara.store_data_param.bcast_time_period)
			{
				g_PlmStatusPara.store_data_param.bcast_time_period = period;

				//重置周期性请求定时器
				storeDateRequestCount = 3;

				if (storeDataReqeustTimerId != INVAILD_TMR_ID)
				{
					HPLC_DelTmr(storeDataReqeustTimerId);
				}
				storeDataReqeustTimerId = HPLC_RegisterTmr(StoreDateRequestTime, NULL, period*OSCfg_TickRate_Hz, TMR_OPT_CALL_ONCE);
			}
			break;
		}
#endif	
	#ifdef ERROR_APHASE
	case F104:
		g_PlmConfigPara.a_phase_error = pLmpAppInfo->curProcUnit->data_unit[0];
		PLC_PhaseErrStart(1);
		SYS_STOR_A_PHASE_ERR();
		break;
	#endif
	case F200:
		g_PlmStatusPara.report_not_white_sta = pLmpAppInfo->curProcUnit->data_unit[0];
		break;
	case F201://设置sta开启认证
		{
			g_pRelayStatus->set_sta_authen_cb.isEnable=pLmpAppInfo->curProcUnit->data_unit[0];
			g_pRelayStatus->set_sta_authen_cb.time = 3;
			break;
		}
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}

	return OK;
}

unsigned char afn_relay_query_09(P_LMP_APP_INFO pLmpAppInfo)
{

	switch (pLmpAppInfo->curDT)
	{
	case F101:    //F1：载波从节点数量
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x2;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0xee;
			break;
		}
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}

	return OK;
}

#ifdef JIANGXI_APPLICATION
u8 topo_delay_times = 0; //江西深化应用台体，12V供电不足超级电容充电慢，网络拓扑待一定次数后全部上报
#endif
unsigned char afn_relay_query_10(P_LMP_APP_INFO pLmpAppInfo)
{
	UCHAR *send_buf = &pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];

	switch (pLmpAppInfo->curDT)
	{
	case F1:    //F1：载波从节点数量
		{
			PLC_get_relay_info();
			if (g_pRelayStatus->selfreg_cb.state<EM_SELFREG_START
				||g_pRelayStatus->selfreg_cb.state> EM_SELFREG_STOP)
			{

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
							   &g_pRelayStatus->cfged_meter_num, 2);
			}
			else
			{
				u16 meter_num = 0;
				//P_RELAY_INFO p_relay = (P_RELAY_INFO)(METER_RELAY_INFO_ADDR + 0x12000000);
				for (int ii = 0; ii <  CCTT_METER_NUMBER; ii++)
				{
#ifdef OVERSEA
					//屏蔽中继地址
					if (true == Check_Relay_Addr(g_MeterRelayInfo[ii].meter_addr))
					{
						continue;
					}
#endif
					if (g_MeterRelayInfo[ii].status.used != CCO_USERD)
					{
						continue;
					}
					if (g_PlmConfigPara.close_white_list != 1) //开启白名单;或者关闭白名单但档案不写入flash
					{
						if (!g_MeterRelayInfo[ii].status.cfged)
						{
							continue;
						}
					}
					meter_num++;
				}
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
							   &meter_num, 2);
			}
			pLmpAppInfo->responsePoint += 2;

			*((USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]) = TEI_STA_MAX-TEI_STA_MIN+1;
			pLmpAppInfo->responsePoint += 2;

//            Relay_Task_PauseAWhile();
			break;
		}
	case F2: //F2：载波从节点信息
		{
			USHORT ii;
//            P_RELAY_INFO p_relayInfo;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT *total_num;

			PLC_get_relay_info();

//			P_RELAY_INFO p_relay=(P_RELAY_INFO)(METER_RELAY_INFO_ADDR+0x12000000);
//			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
//						   //&g_pRelayStatus->all_meter_num, 2);
//						   &g_pRelayStatus->cfged_meter_num, 2);
			total_num=(USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			*total_num=0;
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)((ULONG)pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);
			*real_num = 0;

			pLmpAppInfo->responsePoint++;

			for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
			{

				if (g_MeterRelayInfo[ii].status.used != CCO_USERD)
				{
					continue;
				}

				if (g_PlmConfigPara.close_white_list != 1) //开启白名单;或者关闭白名单但档案不写入flash
				{
					if (!g_MeterRelayInfo[ii].status.cfged)
					{
						continue ;
					}
				}

#ifdef OVERSEA
				//屏蔽中继地址
				if (true == Check_Relay_Addr(g_MeterRelayInfo[ii].meter_addr))
				{
					continue;
				}
#endif				

				(*total_num)++;
				if (start_sn)
				{
					start_sn--;
					if (start_sn!=0)
					{
						continue;
					}
				}
				if (*real_num == meter_num)
					continue;
				(*real_num)++;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
							   g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;

				u16 tei = g_MeterRelayInfo[ii].coll_sn;
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				u8 level = PLC_GetStaLevel(tei);

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = level;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= (0 << 4);
				pLmpAppInfo->responsePoint++;

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0;
				if (NULL != pSta)
				{
#if defined(GET_PHY_PHASE)
				#ifndef UP_STA_BINDING_POST
					for(u8 i=0;i<3;i++)
					{
						u8 phase=(pSta->phy_phase>>(i*2))&0x3;
						if (phase==PHASE_A)
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1<<0;
						}
						else if (phase==PHASE_B)
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1<<1;
						}
						else if (phase==PHASE_C)
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1<<2;
						}
					}
				#else
					if (pSta->phy_phase & (1 << 7))
					{
						for (u8 i = 0; i < 3; i++)
						{
							u8 phase = (pSta->phy_phase >> (i * 2)) & 0x3;
							if (phase != PHASE_UNKNOW)
							{
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << i;
							}

						}
					}
					else
					{
						for (u8 i = 0; i < 3; i++)
						{
							u8 phase = (pSta->phy_phase >> (i * 2)) & 0x3;
							if (phase == PHASE_A)
							{
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << 0;
							}
							else if (phase == PHASE_B)
							{
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << 1;
							}
							else if (phase == PHASE_C)
							{
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << 2;
							}
						}
					}
				#endif
#else
					HPLC_PHASE phase;

				#if defined(COMM_USE_RECEIVE_PHASE)
					if (pSta->receive_phase >= PHASE_A && pSta->receive_phase <= PHASE_C)
						phase = pSta->receive_phase;
					else
						phase = pSta->hplc_phase;
				#else
					phase = pSta->hplc_phase;
				#endif
					if (PHASE_A == phase)
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 1;
					else if (PHASE_B == phase)
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 2;
					else if (PHASE_C == phase)
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 4;

#endif
				}

#ifdef GDW_2012
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= (PLC_meterprotocol_to_3762protocol(g_MeterRelayInfo[ii].meter_type.protocol) << 3);
				if (g_MeterRelayInfo[ii].meter_type.xiaocheng)
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 0x80;
#else
#endif
				pLmpAppInfo->responsePoint++;

			}

//            Relay_Task_PauseAWhile();

			break;
		}
	case F3: //F3：指定载波从节点的上一级中继路由信息
		{

			P_STA_INFO p_sta=PLC_GetValidStaInfoByTei(PLC_GetStaSnByMac(pLmpAppInfo->curProcUnit->data_unit));
			UCHAR *node_num = pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint;
			pLmpAppInfo->responsePoint++;
			P_AFN10_F03 pinfo=(P_AFN10_F03)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			*node_num = 0;
			if (p_sta!=NULL)
			{
				p_sta = PLC_GetValidStaInfoByTei(p_sta->pco);
				while (p_sta && p_sta->pco != 0 && p_sta != &g_pStaInfo[CCO_TEI])
				{
					memset(pinfo,0,sizeof(AFN10_F03));
					memcpy(pinfo->meter_addr, p_sta->mac, LONG_ADDR_LEN);
#ifndef UP_STA_BINDING_POST           
					for (u8 i = 0; i < 3; i++)
					{
						u8 phase = (p_sta->phy_phase >> (i * 2)) & 0x3;
						if (phase == PHASE_A)
						{
							pinfo->phase |= 1 << 0;
						}
						else if (phase == PHASE_B)
						{
							pinfo->phase |= 1 << 1;
						}
						else if (phase == PHASE_C)
						{
							pinfo->phase |= 1 << 2;
						}
					}
#else//上报sta接线柱
					if (p_sta->phy_phase & (1 << 7))
					{

						for (u8 i = 0; i < 3; i++)
						{
							u8 phase = (p_sta->phy_phase >> (i * 2)) & 0x3;
							if (phase != PHASE_UNKNOW)
							{
								pinfo->phase |= 1 << i;
							}

						}

					}
					else
					{
						for (u8 i = 0; i < 3; i++)
						{
							u8 phase = (p_sta->phy_phase >> (i * 2)) & 0x3;
							if (phase == PHASE_A)
							{
								pinfo->phase |= 1 << 0;
							}
							else if (phase == PHASE_B)
							{
								pinfo->phase |= 1 << 1;
							}
							else if (phase == PHASE_C)
							{
								pinfo->phase |= 1 << 2;
							}
						}
					}
#endif
					s8 snr=p_sta->snr;
					if (snr<0)
					{
						snr=0;
					}
					else if (snr>0xf)
					{
						snr=0xf;
					}
					pinfo->snr = snr;
					P_RELAY_INFO p_meter = PLC_GetWhiteNodeInfoByMac(p_sta->mac);
					if (p_meter)
					{
						pinfo->protocol = PLC_meterprotocol_to_3762protocol(p_meter->meter_type.protocol);
					}
					++(*node_num);
					++pinfo;
					p_sta = PLC_GetValidStaInfoByTei(p_sta->pco);
				}
				pinfo=(P_AFN10_F03)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
				for (int i=0;i<*node_num;i++)
				{
					pinfo->relay_level=*node_num-i;
					pLmpAppInfo->responsePoint+=sizeof(AFN10_F03);
					++pinfo;
				}
			}

			break;
		}
	case F4:    //F4：路由运行状态
		{
			u8 state;
			u8 work_step=7;
			PLC_get_relay_info();
//			P_PLC_STA_UPDATE_CB pUpPcb = &g_pRelayStatus->sta_update_cb;
			//搜表没完成，工作标志是正在工作，否则是停止工作
			state = PLC_get_run_state();
			u8 *p_state=send_buf++;
			*p_state = state;

			*((USHORT *)send_buf) = g_pRelayStatus->all_meter_num;
			send_buf += 2;

			*((USHORT *)send_buf) = g_pRelayStatus->freeze_num;
			send_buf += 2;

			*((USHORT *)send_buf) = g_pRelayStatus->sta_num_not_level_1;
			send_buf += 2;
#ifdef GDW_2019_PERCEPTION
			P_SHANXI_ROUT_SWITCH p_switch=(P_SHANXI_ROUT_SWITCH)send_buf;
#else
			P_ROUT_SWITCH p_switch=(P_ROUT_SWITCH)send_buf;
#endif
			p_switch->work_status=1;
			u8 set_work=0;//g_PlmConfigPara.rm_run_switch&1;
			u8 set_reg=(g_PlmConfigPara.rm_run_switch>>1)&1;
			p_switch->reg_status=set_reg;
#ifndef GDW_2019_PERCEPTION
			p_switch->area_status=g_pRelayStatus->area_dis_cb.allow;
			p_switch->event_status=g_PlmStatusPara.report_meter_event;
#endif
			if (g_pRelayStatus->all_meter_num!=0
				&&(g_pRelayStatus->all_meter_num == g_pRelayStatus->check_online
				   ||(g_pRelayStatus->check_online && GetTickCount64()>30*60*1000)
				   )
				)
			{
				p_switch->work_status = set_work; //学习完成  rm_run_switch bit1是上位机设置的工作状态
			}
			if (g_pRelayStatus->selfreg_cb.state >= EM_SELFREG_START && g_pRelayStatus->selfreg_cb.state <= EM_SELFREG_REPORT_STOP)
			{
				p_switch->reg_status=1;//允许注册
				p_switch->func=1;//搜表态
				p_switch->work_status=1;//学习
				*p_state|=1<<1;
#ifndef GDW_2019_PERCEPTION
				p_switch->area_status =	g_pRelayStatus->area_dis_cb.allow;
#endif
			}
			else if (g_pRelayStatus->sta_update_cb.update_flag)
			{
				p_switch->func= 2; //升级
				*p_state|=1<<1;
#ifndef GDW_2019_PERCEPTION
				p_switch->area_status=g_pRelayStatus->area_dis_cb.allow;
#endif
			}
			else if ((/*(g_pRelayStatus->route_read_cb.state != EM_ROUTE_READ_INIT)&&*/(g_pRelayStatus->pause_sec<=1))
					 || g_pRelayStatus->dir_read_cb.state != EM_DIR_READ_INIT
					 || 0 != get_queue_cnt(g_PlcWaitRxQueue))
			{
				p_switch->func=0;//轮抄
				work_step=2;//直抄
				*p_state|=1<<1;
			}
			else
			{
				p_switch->func= 3; 
			}

			send_buf++;


//			*send_buf++ = g_PlmConfigPara.rm_run_switch | ((state & 0x2) << 6); //hplc 远程升级方案 新加

			*((USHORT *)send_buf) = g_PlmConfigPara.rm_plc_rate;
			send_buf += 2;

			u8 level=0;
			for (int i=0;i<MAX_RELAY_DEEP;i++)
			{
				if(g_pRelayStatus->innet_num_level!=0)
				{
					level=i;
				}
			}
			

			*send_buf++ = level;
			*send_buf++ = level;
			*send_buf++ = level;

			if(g_pRelayStatus->broadcast_cb.pSendMsg!=NULL)
			{
				work_step=5;
			}

			*send_buf++ = work_step;
			*send_buf++ = work_step;
			*send_buf++ = work_step;

			pLmpAppInfo->responsePoint += 16;

			break;
		}
	case F5: // F5：未抄读成功的载波从节点信息
		{
			USHORT ii;
//            P_RELAY_INFO p_relayInfo;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT *total_num;
			PLC_get_relay_info();

			total_num=(USHORT*)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			*total_num = 0;
			pLmpAppInfo->responsePoint += 2;

			real_num = pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint;
			*real_num = 0;

			pLmpAppInfo->responsePoint++;
			if(start_sn>0)
			{
				--start_sn;
			}

			for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
			{
				if (g_MeterRelayInfo[ii].status.used != CCO_USERD)
					continue;

				if (g_MeterRelayInfo[ii].status.cfged == 0)
					continue;
				
				u16 tei = PLC_GetStaSnByMac(g_MeterRelayInfo[ii].meter_addr);

				if (tei<TEI_CNT)
				{
					if(g_MeterRelayInfo[tei].freeze_done==1)
					{
						continue ;
					}
				}

				(*total_num)++;
				if (start_sn)
				{
					--start_sn;
					continue;
				}
				if (*real_num == meter_num)
					continue;
				
				if (*real_num==meter_num)
				{
					continue;
				}
				(*real_num)++;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
							   g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = PLC_GetStaLevel(tei);

				pLmpAppInfo->responsePoint++;


				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0;
				P_STA_INFO pSta = PLC_GetStaInfoByMac(g_MeterRelayInfo[ii].meter_addr);
				if (NULL != pSta)
				{
#ifndef UP_STA_BINDING_POST           
					for(u8 i=0; i<3; i++)
					{
						u8 phase = (pSta->phy_phase>>(i*2))&0x3;
						if (phase == PHASE_A)
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1<<0;
						}
						else if (phase == PHASE_B)
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1<<1;
						}
						else if (phase == PHASE_C)
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1<<2;
						}
					}
#else//上报sta接线柱
					if (pSta->phy_phase & (1 << 7))
					{
						for (u8 i = 0; i < 3; i++)
						{
							u8 phase = (pSta->phy_phase >> (i * 2)) & 0x3;
							if (phase != PHASE_UNKNOW)
							{
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << i;
							}

						}
					}
					else
					{
						for (u8 i = 0; i < 3; i++)
						{
							u8 phase = (pSta->phy_phase >> (i * 2)) & 0x3;
							if (phase == PHASE_A)
							{
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << 0;
							}
							else if (phase == PHASE_B)
							{
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << 1;
							}
							else if (phase == PHASE_C)
							{
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << 2;
							}
						}
					}
#endif

				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]=0;
				}


				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 
					PLC_meterprotocol_to_3762protocol(g_pMeterRelayInfo[ii].meter_type.protocol) << 3;

				pLmpAppInfo->responsePoint++;

				

			}

			break;
		}
	case F6: //F6：主动注册的载波从节点信息
		{
			USHORT ii;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT *total_num;

			if (g_pRelayStatus->selfreg_cb.state<EM_SELFREG_START
				||g_pRelayStatus->selfreg_cb.state> EM_SELFREG_STOP)
			{
				memset(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, 0, 3);
				pLmpAppInfo->responsePoint+=3;
				break;
			}

			if (0 == start_sn)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			total_num = (USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			*total_num = 0;
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)((ULONG)pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);
			*real_num = 0;
			pLmpAppInfo->responsePoint++;

			for (ii = 0; ii < CCTT_METER_NUMBER; ii++)
			{
				if (g_MeterRelayInfo[ii].status.used != CCO_USERD)
					continue;

//				if (g_MeterRelayInfo[ii].status.selfreg == 0)
//					continue;

				P_STA_INFO pSta = PLC_GetStaInfoByMac(g_MeterRelayInfo[ii].meter_addr);
				if (NULL == pSta)
					continue;

				if (NET_IN_NET != pSta->net_state)
					continue;

				(*total_num)++;

				if (start_sn != 1)
                {
                    --start_sn;
                    continue;
                }
				
				if (*real_num == meter_num)
					continue;

				(*real_num)++;
				
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
							   g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = PLC_GetStaLevel(PLC_GetStaSnByMac(g_MeterRelayInfo[ii].meter_addr));
				pLmpAppInfo->responsePoint++;

#if 1
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0;
				{

#ifndef UP_STA_BINDING_POST
					for (u8 i = 0; i < 3; i++)
					{
						u8 phase = (pSta->phy_phase >> (i * 2)) & 0x3;
						if (phase == PHASE_A)
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << 0;
						}
						else if (phase == PHASE_B)
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << 1;
						}
						else if (phase == PHASE_C)
						{
							pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << 2;
						}
					}
#else//上报sta接线柱
					if (pSta->phy_phase & (1 << 7))
					{
						for (u8 i = 0; i < 3; i++)
						{
							u8 phase = (pSta->phy_phase >> (i * 2)) & 0x3;
							if (phase != PHASE_UNKNOW)
							{
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << i;
							}

						}
					}
					else
					{
						for (u8 i = 0; i < 3; i++)
						{
							u8 phase = (pSta->phy_phase >> (i * 2)) & 0x3;
							if (phase == PHASE_A)
							{
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << 0;
							}
							else if (phase == PHASE_B)
							{
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << 1;
							}
							else if (phase == PHASE_C)
							{
								pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 1 << 2;
							}
						}
					}
#endif
				}
#else
				if (AC_PHASE_U == g_MeterRelayInfo[ii].status.phase)
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0;
				else
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 1 + g_MeterRelayInfo[ii].status.phase;
#endif

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= 
					PLC_meterprotocol_to_3762protocol(g_pMeterRelayInfo[ii].meter_type.protocol) << 3;

				pLmpAppInfo->responsePoint++;
			}
			break;
		}
	case F7:        //应用手册，查询从节点ID号信息
#if 1
        {
            USHORT ii;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT *total_num;

            if (start_sn == 0) //节点起始序号从1开始
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
            
			PLC_get_relay_info();

			total_num = (USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			*total_num = 0;
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)((ULONG)pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);
			*real_num = 0;

			pLmpAppInfo->responsePoint++;

			for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
			{
				if (g_MeterRelayInfo[ii].status.used != CCO_USERD)
				{
					continue;
				}

				if (g_PlmConfigPara.close_white_list != 1) //开启白名单;或者关闭白名单但档案不写入flash
				{
					if (!g_MeterRelayInfo[ii].status.cfged)
					{
						continue ;
					}
				}
			
				(*total_num)++;
				if (start_sn > 1)
				{
					start_sn--;
					continue;
				}
                
				if (*real_num == meter_num)
					continue;
				(*real_num)++;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
							   g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;

                P_STA_INFO pSta = PLC_GetValidStaInfoByTei(g_MeterRelayInfo[ii].coll_sn);
				if (NULL != pSta)
				{
				    if ((pSta->dev_type == II_COLLECTOR) || (pSta->dev_type == I_COLLECTOR))
                    {
                        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 1; //节点类型字段, 模块类型采集器
                    }
                    else
                    {
                        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0; //节点类型字段, 模块类型电表
                    }
				    #ifdef GDW_JIANGSU 
					if(pSta->minute_coll)
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] |= 0x10; //是否支持分钟级采集
					#endif	              
				    if ((pSta->net_state == NET_IN_NET) && pSta->geted_id)
    				{
    				    pLmpAppInfo->responsePoint++; //节点类型字段，已更新    					
    				}
    				else
    				{
    					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] |= 0x80; //节点类型字段，未更新
    				}

                    //中惠要求厂商代码和芯片代码写死，不从芯片ID中获取
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_factory_code, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2);					
					pLmpAppInfo->responsePoint += 2;
                    
    				if (!pSta->geted_id)
    				{
    					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x01; //长度
    					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x02; //格式
    					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0xFF; //ID号
    				}
    				else
    				{
    					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = sizeof(pSta->module_id); //长度
    					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x01; //格式
    					memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->module_id, sizeof(pSta->module_id)); //模块ID
    					pLmpAppInfo->responsePoint += sizeof(pSta->module_id);
    				}
				}
                else
                {
                    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x8F; //节点类型字段，未知未更新
                    memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, 2); //厂商代码				
					pLmpAppInfo->responsePoint += 2;
                    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x01; //长度
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x02; //格式
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0xFF; //ID号
                }
			}
            break;
        }
#else		
		{
#if 1
			USHORT ii;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT *total_num;

            if (start_sn == 0) //节点起始序号从1开始
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
            
			PLC_get_relay_info();

			total_num = (USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			*total_num = 0;
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)((ULONG)pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);
			*real_num = 0;

			pLmpAppInfo->responsePoint++;

			for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
			{
				if (g_MeterRelayInfo[ii].status.used != CCO_USERD)
				{
					continue;
				}

				if (g_PlmConfigPara.close_white_list != 1) //开启白名单;或者关闭白名单但档案不写入flash
				{
					if (!g_MeterRelayInfo[ii].status.cfged)
					{
						continue ;
					}
				}
			
				(*total_num)++;
				if (start_sn > 1)
				{
					start_sn--;
					continue;
				}
                
				if (*real_num == meter_num)
					continue;
				(*real_num)++;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
							   g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;

                P_STA_INFO pSta = PLC_GetValidStaInfoByTei(g_MeterRelayInfo[ii].coll_sn);
				if (NULL != pSta)
				{
				    if ((pSta->dev_type == II_COLLECTOR) || (pSta->dev_type == I_COLLECTOR))
                    {
                        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 1; //模块类型采集器
                    }
                    else
                    {
                        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0; //模块类型电表
                    }
				                    
				    if ((pSta->net_state == NET_IN_NET) && pSta->geted_id)
    				{
    				    pLmpAppInfo->responsePoint++; //节点类型字段，已更新    					
    				}
    				else
    				{
    					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] |= 0x80; //节点类型字段，未更新
    				}

                    //中惠要求厂商代码和芯片代码写死，不从芯片ID中获取
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_factory_code, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2);					
					pLmpAppInfo->responsePoint += 2;
                    
    				if (!pSta->geted_id)
    				{
    					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x01; //长度
    					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x02; //格式
    					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0xFF; //ID号
    				}
    				else
    				{
    					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = sizeof(pSta->module_id); //长度
    					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x01; //格式
    					memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->module_id, sizeof(pSta->module_id)); //模块ID
    					pLmpAppInfo->responsePoint += sizeof(pSta->module_id);
    				}
				}
                else
                {
                    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x8F; //节点类型字段，未知未更新
                    memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, 2); //厂商代码				
					pLmpAppInfo->responsePoint += 2;
                    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x01; //长度
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x02; //格式
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0xFF; //ID号
                }
			}
#else        
			USHORT ii;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT *total_num;
			//PLC_get_relay_info();

//			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
//						   &g_pRelayStatus->all_meter_num, 2);
			total_num=(u16*)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			*total_num=0;
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)((ULONG)pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);
			*real_num = 0;

			pLmpAppInfo->responsePoint++;

			if (start_sn==0)
			{
				break;
			}
			start_sn--;

			for (ii = TEI_STA_MIN; ii <  TEI_CNT; ii++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(ii);
				if (NULL==pSta)
					continue;
				(*total_num)++;
				

				if (start_sn == 0)
				{
					--start_sn;
					continue;
				}
				(*real_num)++;
				memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->mac, 6);
				pLmpAppInfo->responsePoint += 6;

				
				if (pSta->net_state==NET_IN_NET&& pSta->geted_id)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x00;     //已更新

					//中惠要求厂商代码和芯片代码写死，不从芯片ID中获取
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_factory_code, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2);					
					pLmpAppInfo->responsePoint += 2;
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x80;     //未更新
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x00;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x00;
				}
				if (!pSta->geted_id)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x01;       //长度
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x02;       //格式
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0xFF;       //ID号
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = sizeof(pSta->module_id);       //长度
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0x01;       //格式
					memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint],pSta->module_id,sizeof(pSta->module_id));     //ID号
					pLmpAppInfo->responsePoint+=sizeof(pSta->module_id);
				}
				

				if (*real_num == meter_num)
					break;
			}
#endif
			break;
		}
#endif
	case F9:        //查询网络规模，应用手册
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = (u8)(g_pRelayStatus->innet_num+1);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = (u8)(((g_pRelayStatus->innet_num+1) >> 8) & 0xff);
			break;
		}
#if 0
		//私有协议
	case F20:
		{
			USHORT ii;
			P_RELAY_INFO p_relayInfo;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1]<<8)|(pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;

			USHORT node_counter = 0;

			PLC_get_relay_info();

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
						   //&g_pRelayStatus->all_meter_num, 2);
						   &g_pRelayStatus->cfged_meter_num, 2);
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)((ULONG)pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);
			*real_num = 0;

			pLmpAppInfo->responsePoint++;

			//寻找真正的起始位置
			PLC_GetRealNodeSnCfg(s_meter_sn_from_zero, start_sn, &start_sn);

			for(ii = start_sn; ii <  CCTT_METER_NUMBER; ii++)
			{
				if(g_MeterRelayInfo[ii].status.used != CCO_USERD)
				continue;
				if(g_MeterRelayInfo[ii].status.cfged == 0)
				continue;

				(*real_num)++;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
							   g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_MeterRelayInfo[ii].meter_type.protocol;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_MeterRelayInfo[ii].meter_type.xiaocheng;

				if(*real_num == meter_num)
				break;
			}

			break;
		}
#endif
	case F20:       //双模查询网络拓扑信息（新增AFN=10H， F20）
		{
			USHORT tei;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
            USHORT *total_num;
			ReportTopoInfo *info=&g_pRelayStatus->get_topo_info;

			if (0 == start_sn)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			
            total_num = (USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			*total_num=0;
            pLmpAppInfo->responsePoint += 2;

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &start_sn, 2);
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;
			OS_TICK_64 now_tick = GetTickCount64();
			if (now_tick-info->last_tick>30*1000)//超过20s之前询问的内容无效
			{
				info->is_vaild=0;
			}
			if (start_sn == 1 || !info->is_vaild)
			{
				memset(info->staBitmap,0,sizeof(info->staBitmap));
				info->last_tick=now_tick;
				info->is_vaild=1;
				info->totalSta=1;//cco不在遍历列表里
				for (USHORT ii = 0; ii < CCTT_METER_NUMBER; ii++)
				{
					if ((g_MeterRelayInfo[ii].status.used != CCO_USERD) ||
						(g_MeterRelayInfo[ii].status.cfged == 0) ||
						(g_MeterRelayInfo[ii].coll_sn >= TEI_CNT || g_MeterRelayInfo[ii].coll_sn < TEI_STA_MIN))
						continue;

					tei = g_MeterRelayInfo[ii].coll_sn;
					P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
					if (NULL == pSta)
						continue;

					if (NET_OUT_NET == pSta->net_state)
						continue;

					if (!PLC_IsInNet(tei))
					{
						continue;
					}

					//华为离线也会返回，离网不返回

					if (NET_IN_NET != pSta->net_state)
					{
						continue;
					}
					info->staBitmap[ii>>5]|=1<<(ii&0x1f);
					info->totalSta++;
				}
			}
			*total_num=info->totalSta;//加上cco的

			if (start_sn == 1) //CCO
			{
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_pStaInfo[TEI_CCO].mac, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

				P_STA_TOPOLOGY_INFO_HRF pTop = (P_STA_TOPOLOGY_INFO_HRF)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
				pTop->tei = TEI_CCO;
				pTop->module = 1;
				pTop->pco = g_pStaInfo[TEI_CCO].pco;
				pTop->level = 0;
				pTop->roles = ROLES_CCO;
				pTop->Link=0;
				pLmpAppInfo->responsePoint += sizeof(STA_TOPOLOGY_INFO_HRF);
				start_sn=0;
				(*real_num)++;	
			}
			else
			{
				start_sn-=2;
			}

			
			if (*real_num == meter_num)//只询问了CCO的
			{
				if ((*real_num + start_sn) == (info->totalSta)) //全部读完则放弃
				{
					info->is_vaild = 0;
				}
				break;
			}
			int meterIdx=0;
			for (USHORT ii = 0; ii < (MAX_BITMAP_SIZE / 4); ii++)
			{

				if (info->staBitmap[ii])
				{
					for (int j = 0; j < 32; j++)
					{
						if (info->staBitmap[ii] & (1 << j))
						{
							if (meterIdx < start_sn)
							{
								++meterIdx;
							}
							else
							{
								u16 sn = (ii << 5) | j;
								tei = g_MeterRelayInfo[sn].coll_sn;
								P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
								if (tei <= MAX_TEI_VALUE)
								{
									memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[sn].meter_addr, MAC_ADDR_LEN);
								}
								else
								{
									memset(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, 0xff, MAC_ADDR_LEN);
								}
								pLmpAppInfo->responsePoint += MAC_ADDR_LEN;
								if (pSta)
								{
									P_STA_TOPOLOGY_INFO_HRF pTop = (P_STA_TOPOLOGY_INFO_HRF)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
									pTop->tei = tei;
									pTop->module = pSta->moduleType;
									pTop->pco = pSta->pco;
									pTop->level = PLC_GetStaLevel(tei);
									pTop->roles = pSta->child == 0 ? ROLES_STA : ROLES_PCO;
									pTop->Link = pSta->Link;
									pLmpAppInfo->responsePoint += sizeof(STA_TOPOLOGY_INFO_HRF);
								}
								else
								{
									P_STA_TOPOLOGY_INFO_HRF pTop = (P_STA_TOPOLOGY_INFO_HRF)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
									pTop->tei = 0;
									pTop->module = 0;
									pTop->pco = 0;
									pTop->level = 0;
									pTop->roles =  ROLES_STA;
									pTop->Link = 0;
									pLmpAppInfo->responsePoint += sizeof(STA_TOPOLOGY_INFO_HRF);
								}

								(*real_num)++;
								++meterIdx;
								if (*real_num == meter_num)
								{
									break;
								}
							}
						}
					}
					if (*real_num == meter_num)
					{
						break;
					}
				}

			}
			if((*real_num+start_sn) == (info->totalSta))//全部读完则放弃
			{
				info->is_vaild=0;
			}

			break;
		}
	case F21:       //查询网络拓扑信息（新增AFN=10H， F21）
		{
			USHORT tei;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
            USHORT *total_num;
			ReportTopoInfo *info=&g_pRelayStatus->get_topo_info;

			if (0 == start_sn)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			
            total_num = (USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			*total_num=0;
            pLmpAppInfo->responsePoint += 2;

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &start_sn, 2);
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;
			OS_TICK_64 now_tick = GetTickCount64();
			if (now_tick-info->last_tick>30*1000)//超过20s之前询问的内容无效
			{
				info->is_vaild=0;
			}
			if (start_sn == 1 || !info->is_vaild)
			{
				memset(info->staBitmap,0,sizeof(info->staBitmap));
				info->last_tick=now_tick;
				info->is_vaild=1;
				info->totalSta=1;//cco不在遍历列表里
				for (USHORT ii = 0; ii < CCTT_METER_NUMBER; ii++)
				{
					if ((g_MeterRelayInfo[ii].status.used != CCO_USERD) ||
						(g_MeterRelayInfo[ii].status.cfged == 0) ||
						(g_MeterRelayInfo[ii].coll_sn >= TEI_CNT || g_MeterRelayInfo[ii].coll_sn < TEI_STA_MIN))
						continue;

					tei = g_MeterRelayInfo[ii].coll_sn;
					P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
					if (NULL == pSta)
						continue;

					if (NET_OUT_NET == pSta->net_state)
						continue;

					if (!PLC_IsInNet(tei))
					{
						continue;
					}

					//华为离线也会返回，离网不返回

					if (NET_IN_NET != pSta->net_state)
					{
						continue;
					}
					info->staBitmap[ii>>5]|=1<<(ii&0x1f);
					info->totalSta++;
#ifdef JIANGXI_APPLICATION
					//江西深化应用台体停复电测试，组网完成10秒查一次，延后30次报全
					if((topo_delay_times > 0)&&(topo_delay_times <= 30))
					{
						topo_delay_times++;
						break;
					}
#endif	
				}
			}
			*total_num=info->totalSta;//加上cco的

			if (start_sn == 1) //CCO
			{
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_pStaInfo[TEI_CCO].mac, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

				P_STA_TOPOLOGY_INFO pTop = (P_STA_TOPOLOGY_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
				pTop->tei = TEI_CCO;
				pTop->pco = g_pStaInfo[TEI_CCO].pco;
				pTop->level = 0;
				pTop->roles = ROLES_CCO;
				pLmpAppInfo->responsePoint += sizeof(STA_TOPOLOGY_INFO_HRF);
				start_sn=0;
				(*real_num)++;	
			}
			else
			{
				start_sn-=2;
			}

			
			if (*real_num == meter_num)//只询问了CCO的
			{
				if ((*real_num + start_sn) == (info->totalSta)) //全部读完则放弃
				{
					info->is_vaild = 0;
				}
				break;
			}
			int meterIdx=0;
			for (USHORT ii = 0; ii < (MAX_BITMAP_SIZE / 4); ii++)
			{
				if (info->staBitmap[ii])
				{
					for (int j = 0; j < 32; j++)
					{
						if (info->staBitmap[ii] & (1 << j))
						{
							if (meterIdx < start_sn)
							{
								++meterIdx;
							}
							else
							{
								u16 sn = (ii << 5) | j;
								tei = g_MeterRelayInfo[sn].coll_sn;
								P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
								if (tei <= MAX_TEI_VALUE)
								{
									memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[sn].meter_addr, MAC_ADDR_LEN);
								}
								else
								{
									memset(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, 0xff, MAC_ADDR_LEN);
								}
								pLmpAppInfo->responsePoint += MAC_ADDR_LEN;
								if (pSta)
								{
									P_STA_TOPOLOGY_INFO pTop = (P_STA_TOPOLOGY_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
									pTop->tei = tei;
									pTop->pco = pSta->pco;
									pTop->level = PLC_GetStaLevel(tei);
									pTop->roles = pSta->child == 0 ? ROLES_STA : ROLES_PCO;
									pLmpAppInfo->responsePoint += sizeof(STA_TOPOLOGY_INFO_HRF);
								}
								else
								{
									P_STA_TOPOLOGY_INFO pTop = (P_STA_TOPOLOGY_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
									pTop->tei = 0;
									pTop->pco = 0;
									pTop->level = 0;
									pTop->roles =  ROLES_STA;
									pLmpAppInfo->responsePoint += sizeof(STA_TOPOLOGY_INFO_HRF);
								}

								(*real_num)++;
								++meterIdx;
								if (*real_num == meter_num)
								{
									break;
								}
							}
						}
					}
					if (*real_num == meter_num)
					{
						break;
					}
				}

			}
			if((*real_num+start_sn) == (info->totalSta))//全部读完则放弃
			{
				info->is_vaild=0;
			}

			break;
		}
	case F31://查询相线信息
		{
#if 1
			USHORT ii;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
            USHORT *total_num;

			if (0 == start_sn)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			
            total_num = (USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			*total_num=0;
            pLmpAppInfo->responsePoint += 2;

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &start_sn, 2);
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;

			if (1 == start_sn) //CCO相线信息
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(TEI_CCO);
				if (NULL != pSta)
				{
					(*total_num)++;
					(*real_num)++;

					memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, pSta->mac, MAC_ADDR_LEN);
					pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

					*(u16*)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = afn_get_meter_phase_info(TEI_CCO);
					pLmpAppInfo->responsePoint += 2;
				}
			}

			for (ii=0; ii<CCTT_METER_NUMBER; ii++)
			{
				if (g_MeterRelayInfo[ii].status.used != CCO_USERD)
				{
					continue;
				}

				if (g_PlmConfigPara.close_white_list != 1) //开启白名单;或者关闭白名单但档案不写入flash
				{
					if (!g_MeterRelayInfo[ii].status.cfged)
					{
						continue;
					}
				}

#ifdef OVERSEA
				//屏蔽中继地址
				if (true == Check_Relay_Addr(g_MeterRelayInfo[ii].meter_addr))
				{
					continue;
				}
#endif				

				u16 tei = g_MeterRelayInfo[ii].coll_sn;
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (NULL == pSta)
					continue;

				if (NET_OUT_NET == pSta->net_state)
					continue;

				if (!PLC_IsInNet(tei))
				{
					continue;
				}

				(*total_num)++;

				if (start_sn > 2)
                {
                    start_sn--;
                    continue;
                }

				if (*real_num == meter_num)
					continue;
				
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

				*(u16*)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = afn_get_meter_phase_info(tei);
				pLmpAppInfo->responsePoint += 2;
				(*real_num)++;
			}
#else
			USHORT tei;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
            USHORT *total_num;

			if (0 == start_sn)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
            total_num = (USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			*total_num=0;
//  		memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &node_count, 2);
            pLmpAppInfo->responsePoint += 2;

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &start_sn, 2);
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;

//			if (!PLC_GetRealStaSn(FALSE, start_sn, &start_sn))
//				break;

			for (tei = TEI_CCO; tei < TEI_CNT; tei++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (NULL == pSta)
					continue;

				if (NET_OUT_NET == pSta->net_state)
					continue;
                if (!PLC_IsInNet(tei))
				{
					continue;
				}
                ++(*total_num);
                if (start_sn!=1)
                {
                    --start_sn;
                    continue ;
                }
                if (*real_num == meter_num)
					continue;
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, pSta->mac, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

				*(u16*)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]=afn_get_meter_phase_info(tei);
				pLmpAppInfo->responsePoint+=2;
				++(*real_num);

			}
#endif

			break;
		}
#ifdef HPLC_GDW				
	case F40:       //3.3.8 流水线ID信息读取一致性测试
		{
			//《面向流水线改造的HPLC相关设备ID信息读取协议扩展》
			//芯片 ID 长度为 24 字节；模块 ID 长度为 11 字节；
			//ID 信息：24 位芯片 ID 的格式定义同 AFN10H-F112 中的芯片 ID 信息格式定义；模块 ID 的具体定义在其它文档中明确。

			u8 dev_type = pLmpAppInfo->curProcUnit->data_unit[0];
			u8 *pMacAddr = (u8 *)&pLmpAppInfo->curProcUnit->data_unit[1];
			u8 id_type = pLmpAppInfo->curProcUnit->data_unit[7];

			if (2 == dev_type)     //1 抄控器	 2 集中器本地通信单元  3 电表通信单元	 4 中继器  5 II型采集器  6 I型采集器单元  7 三相电表通信单元
			{
				if (0x01 == id_type)       //芯片ID
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    //设备类型
					//设备地址
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pMacAddr, 6);
					pLmpAppInfo->responsePoint += 6;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = id_type;    //ID 类型
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = sizeof(STA_CHIP_ID_INFO);      //ID 长度
					//ID 信息
					//V2.7要求“芯片ID”倒序传输。CCO录入“芯片ID”时是倒序录入，所以不再需要转换字节序
					P_STA_CHIP_ID_INFO pChipID = (P_STA_CHIP_ID_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
					memcpy_special(pChipID, &g_cco_id.chip_id[REAL_CHIP_ID_OFFSET], sizeof(STA_CHIP_ID_INFO));

				#if 0
					pChipID->fix_code1 = 0x01;
					pChipID->fix_code2 = 0x02;
					pChipID->fix_code3 = 0x9c;
					pChipID->fix_code4[0] = 0xfb;
					pChipID->fix_code4[1] = 0xc1;
					pChipID->fix_code4[2] = 0x01;
					pChipID->dev_class = 0x02;
					memcpy_special(pChipID->factory_code, c_Tmn_ver_info.factoryCode, 2);
					memcpy_special(pChipID->chip_code, c_Tmn_ver_info.chip_code, 2);
					//memcpy_special(pChipID->dev_seq, pSta->dev_seq, sizeof(pSta->dev_seq));
					//memcpy_special(pChipID->check_sum, pSta->check_sum, sizeof(pSta->check_sum));
				#else
					//华为STA没分配芯片ID的，为全0
				#endif

					pLmpAppInfo->responsePoint += sizeof(STA_CHIP_ID_INFO);
				}
				else if (0x02 == id_type)  //模块ID
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    //设备类型
					//设备地址
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pMacAddr, 6);
					pLmpAppInfo->responsePoint += 6;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = id_type;    //ID 类型
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = sizeof(g_cco_id.module_id);      //ID 长度
					//V2.7要求“模块ID”倒序传输。CCO录入“模块ID”时是倒序录入，所以不再需要转换字节序
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_cco_id.module_id, sizeof(g_cco_id.module_id));
					pLmpAppInfo->responsePoint += 11;
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
					return ERROR;
				}
			}
			else if (3 == dev_type)  
			{
				if (0x01 == id_type) 
				{
				    u8 dst_addr[LONG_ADDR_LEN] = {0};
                    PLC_GetAppDstMac(pMacAddr, dst_addr);
        
					P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
					if(pSta != NULL)
					{
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    //设备类型
						memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pMacAddr, 6);
						pLmpAppInfo->responsePoint += 6;
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = id_type;    //ID 类型
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = sizeof(STA_CHIP_ID_INFO);      //ID 长度
						
						//ID 信息
                        if ((memIsHex(pSta->check_sum, 0xff, sizeof(pSta->check_sum)))||
                            (memIsHex(pSta->check_sum, 0x00, sizeof(pSta->check_sum))))
                        {
                            //未录入芯片ID，全F返回处理
                            memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0xFF, sizeof(STA_CHIP_ID_INFO));
                        }
                        else
                        {
    						//V2.7要求“芯片ID”倒序传输。
							P_STA_CHIP_ID_INFO pChipID = (P_STA_CHIP_ID_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
    						
							pChipID->fix_code1 = 0x01;
							pChipID->fix_code2 = 0x02;
							pChipID->fix_code3 = 0x9c;
							pChipID->fix_code4[0] = 0xfb;
							pChipID->fix_code4[1] = 0xc1;
							pChipID->fix_code4[2] = 0x01;
							//pChipID->dev_class = pSta->moduleType?2:3;
	//						if (pInfo)
							{
								memcpy_special(&pChipID->dev_class, &pSta->dev_class, sizeof(pSta->dev_class));
								memcpy_special(pChipID->factory_code, pSta->factory_code, sizeof(pSta->factory_code));
								memcpy_special(pChipID->chip_code, pSta->chip_code, sizeof(pSta->chip_code));
								memcpy_special(pChipID->dev_seq, pSta->dev_seq, sizeof(pSta->dev_seq));
								memcpy_special(pChipID->check_sum, pSta->check_sum, sizeof(pSta->check_sum));
							}
                        }
					
						pLmpAppInfo->responsePoint += sizeof(STA_CHIP_ID_INFO);
					}
					else
					{
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
						return ERROR;
					}
				}
				else
				{
					u8 *p = &pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
                    u8 dst_addr[LONG_ADDR_LEN] = {0};
                    PLC_GetAppDstMac(pMacAddr, dst_addr);

					P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
					if (pSta && pSta->geted_id)
					{
						*p++ = pSta->dev_type;
						memcpy_special(p, pSta->mac, LONG_ADDR_LEN);
						p += LONG_ADDR_LEN;
						*p++ = 2; //ID类型，模块ID
						*p++ = sizeof(pSta->module_id);
						if (pSta)
						{
							memcpy_special(p, pSta->module_id, sizeof(pSta->module_id));
						}

						pLmpAppInfo->responsePoint += 9 + sizeof(pSta->module_id);
						return OK;
					}
					else if (pSta && (pSta->geted_id==0))
					{
					    PLC_GET_MOUDLE_ID_CB* pcb = &g_pRelayStatus->get_moudle_id_realtime_cb;

                        debug_str(DEBUG_LOG_NET, "AFN10 F40 realtime, state:%d, mac:%02x:%02x:%02x:%02x:%02x:%02x\r\n", pcb->state, 
                            dst_addr[5], dst_addr[4], dst_addr[3], dst_addr[2], dst_addr[1], dst_addr[0]);
                        
					    if (pcb->state == EM_TASK_READ_INIT)
                        {               
    					    memcpy(pcb->mac_addr, dst_addr, MAC_ADDR_LEN); 
            				pcb->state = EM_TASK_READ_SEND;
            				pcb->seq = pLmpAppInfo->lmpLinkHead->Inf.down_info.seq_no;
                            pcb->retry = id_type;
                            pcb->dev_type = dev_type;
                            pcb->tei = PLC_GetStaSnByMac(dst_addr);

                            pLmpAppInfo->confirmFlag = 2;
                        }
                        else
                        {
                            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_BUSY_NOW;
						    return ERROR;
                        }						
					}
					else
					{
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
						return ERROR;
					}
				}
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			break;
		}
	case F41:       //扩展F41 用于写入ID信息
		{
			//《面向流水线改造的HPLC相关设备ID信息读取协议扩展》
			//芯片 ID 长度为 24 字节；模块 ID 长度为 11 字节；
			//ID 信息：24 位芯片 ID 的格式定义同 AFN10H-F112 中的芯片 ID 信息格式定义；模块 ID 的具体定义在其它文档中明确。

			u8 dev_type = pLmpAppInfo->curProcUnit->data_unit[0];
			u8 *pMacAddr = (u8 *)&pLmpAppInfo->curProcUnit->data_unit[1];
			u8 id_type = pLmpAppInfo->curProcUnit->data_unit[7];
			u8 id_len = pLmpAppInfo->curProcUnit->data_unit[8];
			if (2 == dev_type)     //1 抄控器	 2 集中器本地通信单元  3 电表通信单元	 4 中继器  5 II型采集器  6 I型采集器单元  7 三相电表通信单元
			{
				if (0x01 == id_type && id_len == sizeof(STA_CHIP_ID_INFO))       //芯片ID
				{
					memcpy(&g_cco_id.chip_id[REAL_CHIP_ID_OFFSET],&pLmpAppInfo->curProcUnit->data_unit[9],id_len);
					SYS_STOR_CCO_ID();
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    //设备类型
					//设备地址
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pMacAddr, 6);
					pLmpAppInfo->responsePoint += 6;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = id_type;    //ID 类型
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = sizeof(STA_CHIP_ID_INFO);      //ID 长度
					//ID 信息
					P_STA_CHIP_ID_INFO pChipID = (P_STA_CHIP_ID_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
					memcpy_special(pChipID, &g_cco_id.chip_id[REAL_CHIP_ID_OFFSET], sizeof(STA_CHIP_ID_INFO));


					pLmpAppInfo->responsePoint += sizeof(STA_CHIP_ID_INFO);
//					P_STA_INFO pCco = &g_pStaInfo[TEI_CCO];
//					memcpy_special(pCco->factory_code, pChipID->factory_code, sizeof(pChipID->factory_code));
//					memcpy_special(pCco->chip_code, pChipID->chip_code, sizeof(pChipID->chip_code));
//					memcpy_special(pCco->dev_seq, pChipID->dev_seq, sizeof(pChipID->dev_seq));
//					memcpy_special(pCco->check_sum, pChipID->check_sum, sizeof(pChipID->check_sum));
				}
				else if (0x02 == id_type && id_len == sizeof(g_cco_id.module_id))  //模块ID
				{
					memcpy(g_cco_id.module_id,&pLmpAppInfo->curProcUnit->data_unit[9],id_len);
					SYS_STOR_CCO_ID();
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    //设备类型
					//设备地址
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pMacAddr, 6);
					pLmpAppInfo->responsePoint += 6;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = id_type;    //ID 类型
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 11;      //ID 长度
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_cco_id.module_id, 11);
					pLmpAppInfo->responsePoint += 11;

				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
					return ERROR;
				}
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			break;
		}
#endif		
#ifdef GDW_2019_PERCEPTION
	case F102://陕西查询节点相位信息
		{
			USHORT tei;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT *total_num;

			if (0 == start_sn)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			total_num = (USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			*total_num = 0;
			pLmpAppInfo->responsePoint += 2;



			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;

//			if (!PLC_GetRealStaSn(FALSE, start_sn, &start_sn))
//				break;

			for (tei = TEI_CCO; tei < TEI_CNT; tei++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (NULL == pSta)
					continue;

				if (NET_OUT_NET == pSta->net_state)
					continue;

				++(*total_num);
				if (start_sn != 1)
				{
					--start_sn;
					continue;
				}
				if (*real_num == meter_num)
					continue;
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, pSta->mac, MAC_ADDR_LEN);
				pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

				*(u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = afn_get_meter_shanxi_phase_info(tei);
				pLmpAppInfo->responsePoint += 2;
				++(*real_num);

			}

			break;
		}
#else
	case F102://查询网络节点信息青岛归约添加
		{
			USHORT tei;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT *total_num;

			if (0 == start_sn)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			if (meter_num > 64)
			{
				meter_num = 64;
			}
			total_num = (USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			*total_num = 0;
//  		memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &node_count, 2);
			pLmpAppInfo->responsePoint += 2;

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &start_sn, 2);
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;



			for (tei = TEI_CCO; tei < TEI_CNT; tei++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (NULL == pSta)
					continue;

				if (NET_OUT_NET == pSta->net_state)
					continue;
				if (!PLC_IsInNet(tei))
				{
					continue;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					continue;
				}
				++(*total_num);
				if (start_sn != 1)
				{
					--start_sn;
					continue;
				}

				if (*real_num == meter_num)
					continue;

				P_STA_10F102_INFO pinfo = (P_STA_10F102_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
				memcpy(pinfo->mac, pSta->mac, MAC_ADDR_LEN);
				pinfo->tei = tei;
				pinfo->pco = pSta->pco;
				pinfo->level = PLC_GetStaLevel(tei);
				pinfo->roles = tei == CCO_TEI ? ROLES_CCO : pSta->child == 0 ? ROLES_STA : ROLES_PCO;

				if (tei == TEI_CCO)
				{

					pSta->phy_phase = (ZC_Time[0] ? PHASE_A : 0) |
						(ZC_Time[1] ? PHASE_B : 0) << 2 |
						(ZC_Time[2] ? PHASE_C : 0) << 4 |
						(1 << 7);

					pinfo->soft_version[0] = Hex2BcdChar(pSta->software_v[0]);
					pinfo->soft_version[1] = Hex2BcdChar(pSta->software_v[1]);
				}
				else
				{
					memcpy(pinfo->soft_version, pSta->software_v, sizeof(pinfo->soft_version));
				}
				TransposeByteOrder(pinfo->soft_version, sizeof(pinfo->soft_version));
				memcpy_swap(pinfo->factory_code, pSta->ver_factory_code, sizeof(pinfo->factory_code));
				P_STA_STATUS_INFO pStaStaus = PLC_GetStaStatusInfoByTei(tei);
				pinfo->boot_version = pSta->bootloader;
				if (pSta->pco == TEI_CCO)
				{
					pinfo->up_rate = StaNeighborTab.Neighbor[tei].LastNghTxCnt*100 / pStaStaus->found_list_count_last_route;
					pinfo->down_rate = StaNeighborTab.Neighbor[tei].LastNghRxCnt*100 / g_pRelayStatus->found_list_count_last_route;
					if(pinfo->up_rate>99)
					{
						pinfo->up_rate=99;
					}
					if(pinfo->down_rate>99)
					{
						pinfo->down_rate=99;
					}
				}
				else
				{
					pinfo->up_rate = StaNeighborTab.Neighbor[tei].PcoSucRate;
					pinfo->down_rate = StaNeighborTab.Neighbor[tei].PcoSucRate;
				}

				memset(pinfo->res, 0, sizeof(pinfo->res));
				pinfo->device=0;
				u8 phase_num=0;
				for (int i=0;i<3;i++)
				{
					u8 phase=(pSta->phy_phase >> (i * 2)) & 0x3;
					if (phase)
					{
						pinfo->device=phase<<4;
						++phase_num;
					}
				}
				if (phase_num==3)
				{
					pinfo->device=4<<4;
				}
				switch (pSta->dev_type)
				{
				case 2:
					pinfo->device |= 0; //集中器模块
					break;
				case 3:
					pinfo->device |= 1; //表
					break;
				case 5:
					pinfo->device |= 4; //II采
					break;
				case 6:
					pinfo->device |= 3; //I采
					break;
				case 7:
					pinfo->device |= 2; //三相
					break;
				default:
					pinfo->device |= 0; //集中器模块
					break;
				}
				
				pLmpAppInfo->responsePoint += sizeof(STA_10F102_INFO);

				++(*real_num);

			}

			break;
		}
#endif
	#ifdef ERROR_APHASE
	case F106:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_PlmConfigPara.a_phase_error ;
		break;
	#endif
	case F111:      //查询多网络信息（新增AFN=10H， F111）
		{
			u8 *pCounter = &pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*pCounter = 0;
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &g_pPlmConfigPara->net_id, 3);
			pLmpAppInfo->responsePoint += 3;
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_pPlmConfigPara->main_node_addr, 6);
			pLmpAppInfo->responsePoint += 6;

			P_NET_NEIGHBOR pNeighbor = HPLC_GetNeighborPtr();

			for (u8 i = 0; i < PLC_MAX_NID_NEIGHBOR_COUNT; i++)
			{
				if (pNeighbor->nid >= NID_MIN && pNeighbor->nid <= NID_MAX)
				{
					if(NtbsBetween64(GetNtbCount64(), pNeighbor->ntb_receive) >= 120 * NTB_FRE_PER_SEC)
					{
						continue;
					}
					
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &pNeighbor->nid, 3);
					pLmpAppInfo->responsePoint += 3;
					(*pCounter)++;
				}

				pNeighbor++;
			}
			break;
		}
#ifdef HPLC_GDW				
	case F112:      //查询宽带载波芯片信息（新增AFN=10H， F112）
		{
#if 1
			UCHAR node_count = 0;
			UCHAR *real_num;
			USHORT start_sn = 0;
			USHORT total_num = g_pRelayStatus->cfged_meter_num + 1;;
			USHORT ii = 0;
			P_STA_INFO pSta = NULL;

			memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2); //下行节点起始序号
			if (0 == start_sn)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			
			node_count = pLmpAppInfo->curProcUnit->data_unit[2]; //下行节点数量

			//上行节点总数量
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &total_num, 2);
			pLmpAppInfo->responsePoint += 2;

			//上行节点起始序号
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &start_sn, 2);
			pLmpAppInfo->responsePoint += 2;

			//上行本次应答的节点数量
			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]; 
			*real_num = 0;
			
			if (start_sn == 1) //CCO
			{   
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_PlmConfigPara.main_node_addr, 6);
				pLmpAppInfo->responsePoint += 6;

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = CONCENTRATOR;
				
				
		  		memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &g_cco_id.chip_id[REAL_CHIP_ID_OFFSET], sizeof(STA_CHIP_ID_INFO));
				pLmpAppInfo->responsePoint += sizeof(STA_CHIP_ID_INFO);

				memcpy_swap(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, (u8 *)&c_Tmn_ver_info.software_v, 2);
				pLmpAppInfo->responsePoint += 2;
				
				(*real_num)++;
			}

			for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
			{
				if ((*real_num) == node_count)
					break;
		 
				if (!METER_IS_CFG(ii))
					continue;

				if (start_sn > 2) //从节点序号从1开始
				{
					start_sn--;
					continue;
				}
		 
				(*real_num)++;

				memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, sizeof(STA_CHIP_INFO));

				P_STA_CHIP_INFO pChip = (P_STA_CHIP_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
				pLmpAppInfo->responsePoint += sizeof(STA_CHIP_INFO);
								
				memcpy_special(pChip->mac, g_MeterRelayInfo[ii].meter_addr, 6);
				
				u8 dst_addr[LONG_ADDR_LEN] = {0};
                PLC_GetAppDstMac(g_MeterRelayInfo[ii].meter_addr, dst_addr);              
                pSta = PLC_GetStaInfoByMac(dst_addr);

				if (pSta != NULL)
				{     					
					pChip->dev_type = pSta->dev_type;
					
					pChip->chip_id_info.fix_code1 = 0x01;
					pChip->chip_id_info.fix_code2 = 0x02;
					pChip->chip_id_info.fix_code3 = 0x9c;
					pChip->chip_id_info.fix_code4[0] = 0xfb;
					pChip->chip_id_info.fix_code4[1] = 0xc1;
					pChip->chip_id_info.fix_code4[2] = 0x01;
					memcpy_special(&pChip->chip_id_info.dev_class, &pSta->dev_class, sizeof(pSta->dev_class));
					memcpy_special(pChip->chip_id_info.factory_code, pSta->factory_code, sizeof(pSta->factory_code));
					memcpy_special(pChip->chip_id_info.chip_code, pSta->chip_code, sizeof(pSta->chip_code));
					memcpy_special(pChip->chip_id_info.dev_seq, pSta->dev_seq, sizeof(pSta->dev_seq));
					memcpy_special(pChip->chip_id_info.check_sum, pSta->check_sum, sizeof(pSta->check_sum));				
					
					memcpy_swap(pChip->software_v, pSta->software_v, sizeof(pSta->software_v));
				}
			}
#else
			USHORT tei;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT node_count = g_pRelayStatus->innet_num + 1;

			if (0 == start_sn)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &node_count, 2);
			pLmpAppInfo->responsePoint += 2;

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &start_sn, 2);
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;

			if (!PLC_GetRealStaSn(FALSE, start_sn, &start_sn))
				break;

			for (tei = start_sn; tei < TEI_CNT; tei++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (NULL == pSta)
					continue;

				P_STA_CHIP_INFO pChip = (P_STA_CHIP_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
				pLmpAppInfo->responsePoint += sizeof(STA_CHIP_INFO);

				memcpy_special(pChip->mac, pSta->mac, MAC_ADDR_LEN);
				pChip->dev_type = pSta->dev_type;

				//V2.7要求“芯片ID”和“软件版本”倒序传输，STA需要转换字节序
				if(tei == TEI_CCO)
				{
					memcpy_special(&pChip->chip_id_info, g_cco_id.chip_id, sizeof(STA_CHIP_ID_INFO));
				}
				else
				{
					//P_RELAY_INFO pInfo = PLC_GetWhiteNodeInfoByMac(pSta->mac);
					pChip->chip_id_info.fix_code1 = 0x01;
					pChip->chip_id_info.fix_code2 = 0x02;
					pChip->chip_id_info.fix_code3 = 0x9c;
					pChip->chip_id_info.fix_code4[0] = 0xfb;
					pChip->chip_id_info.fix_code4[1] = 0xc1;
					pChip->chip_id_info.fix_code4[2] = 0x01;
					pChip->chip_id_info.dev_class = pSta->moduleType?2:3;
					if (pSta)
					{
						memcpy_special(pChip->chip_id_info.factory_code, pSta->factory_code, sizeof(pSta->factory_code));
						memcpy_special(pChip->chip_id_info.chip_code, pSta->chip_code, sizeof(pSta->chip_code));
						memcpy_special(pChip->chip_id_info.dev_seq, pSta->dev_seq, sizeof(pSta->dev_seq));
						memcpy_special(pChip->chip_id_info.check_sum, pSta->check_sum, sizeof(pSta->check_sum));				
					}
				}

				memcpy_special(pChip->software_v, pSta->software_v, sizeof(pSta->software_v));
				TransposeByteOrder(pChip->software_v, sizeof(pChip->software_v));

				if(tei == TEI_CCO)
				{
					pChip->software_v[0] = Hex2BcdChar(pChip->software_v[0]);
					pChip->software_v[1] = Hex2BcdChar(pChip->software_v[1]);
				}
				
				(*real_num)++;

				if (*real_num == meter_num)
					break;
			}
#endif
			break;
		}
#endif		
	case F104:
		{
#if 1
			UCHAR node_count = 0;
			UCHAR *real_num;
			USHORT start_sn = 0;
			USHORT total_num = g_pRelayStatus->cfged_meter_num + 1;;
			USHORT ii = 0;
			P_STA_INFO pSta = NULL;
			
			memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2); //下行节点起始序号
			node_count = pLmpAppInfo->curProcUnit->data_unit[2]; //下行节点数量

			//上行节点总数量
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &total_num, 2);
			pLmpAppInfo->responsePoint += 2;

			//上行本次应答的节点数量
			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]; 
			*real_num = 0;
			
			if (start_sn == 0) //CCO
			{   
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_PlmConfigPara.main_node_addr, 6);
				pLmpAppInfo->responsePoint += 6;

                pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.software_v[1]);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.software_v[0]);
                
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.day);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.month);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.year);

				memcpy_swap(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, (u8 *)&c_Tmn_ver_info.factoryCode, 2);
				pLmpAppInfo->responsePoint += 2;

				memcpy_swap(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, (u8 *)&c_Tmn_ver_info.chip_code, 2);
				pLmpAppInfo->responsePoint += 2;
		  
				(*real_num)++;
			}

			for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
			{
				if ((*real_num) == node_count)
					break;
		 
				if (!METER_IS_CFG(ii))
					continue;

				if (start_sn > 1) //从节点序号从1开始
				{
					start_sn--;
					continue;
				}
		 
				(*real_num)++;

				memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, sizeof(STA_NODE_INFO));
				
				P_STA_NODE_INFO pChip = (P_STA_NODE_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
				pLmpAppInfo->responsePoint += sizeof(STA_NODE_INFO);
				
				memcpy_special(pChip->mac, g_MeterRelayInfo[ii].meter_addr, 6);
				
				u8 dst_addr[LONG_ADDR_LEN] = {0};
                PLC_GetAppDstMac(g_MeterRelayInfo[ii].meter_addr, dst_addr);              
                pSta = PLC_GetStaInfoByMac(dst_addr);

				if (pSta != NULL)
				{     					
					memcpy_swap(pChip->software_v, pSta->software_v, sizeof(pSta->software_v));
					pChip->Data[0] = Hex2BcdChar(pSta->year_mon_data.Day);
					pChip->Data[1] = Hex2BcdChar(pSta->year_mon_data.Month);
					pChip->Data[2] = Hex2BcdChar(pSta->year_mon_data.Year);
					memcpy_swap(pChip->moudleCode, pSta->ver_factory_code, sizeof(pChip->moudleCode));				
					memcpy_swap(pChip->chipCode, pSta->ver_chip_code, sizeof(pChip->chipCode));
				}
			}
#else			
			USHORT tei;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT node_count = g_pRelayStatus->innet_num + 1;
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &node_count, 2);
			pLmpAppInfo->responsePoint += 2;
			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;
			if (!PLC_GetRealStaSn(true, start_sn, &start_sn))
				break;

			for (tei = start_sn; tei < TEI_CNT; tei++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (NULL == pSta)
					continue;

				P_STA_NODE_INFO pChip = (P_STA_NODE_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
				pLmpAppInfo->responsePoint += sizeof(STA_NODE_INFO);

				memcpy_special(pChip->mac, pSta->mac, MAC_ADDR_LEN);
#if 1
				//中惠要求厂商代码和芯片代码写死，不从芯片ID中获取
				memcpy_special(pChip->moudleCode, pSta->ver_factory_code, sizeof(pChip->moudleCode));				
				memcpy_special(pChip->chipCode, pSta->ver_chip_code, sizeof(pChip->chipCode));
				TransposeByteOrder(pChip->moudleCode, sizeof(pChip->moudleCode));
				TransposeByteOrder(pChip->chipCode, sizeof(pChip->chipCode));
#else
//				P_RELAY_INFO pInfo = PLC_GetWhiteNodeInfoByMac(pSta->mac);
				if (pSta)
				{
					memcpy_special(pChip->moudleCode, pSta->factory_code, sizeof(pChip->moudleCode));
					memcpy_special(pChip->chipCode, pSta->chip_code, sizeof(pChip->chipCode));
				}
#endif				
				memcpy_special(pChip->software_v, pSta->software_v, sizeof(pSta->software_v));
				TransposeByteOrder(pChip->software_v, sizeof(pChip->software_v));
				
				if(tei == TEI_CCO)
				{
					pChip->software_v[0] = Hex2BcdChar(pChip->software_v[0]);
					pChip->software_v[1] = Hex2BcdChar(pChip->software_v[1]);
				
					pChip->Data[0] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.day);
					pChip->Data[1] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.month);
					pChip->Data[2] = Hex2BcdChar(c_Tmn_ver_info.softReleaseDate.year);
				}
				else
				{
					pChip->Data[0] = Hex2BcdChar(pSta->year_mon_data.Day);
					pChip->Data[1] = Hex2BcdChar(pSta->year_mon_data.Month);
					pChip->Data[2] = Hex2BcdChar(pSta->year_mon_data.Year);
				}
				
				(*real_num)++;

				if (*real_num == meter_num)
					break;
			}
#endif
		}
		break;
#ifdef GDW_ZHEJIANG
	case F150:
		{
			USHORT i;
			u32 MeterStartSN = 0;
			u8  MeterNum;
			u16 totalsize = 0;
			P_RELAY_INFO pNode = NULL;
			u8 MeterMac[6];
			u8 CollectMac[6];
			u16 collect_sn;
			UCHAR *real_num;
//测试代码
/*			g_pStaInfo[2].net_state = 1;
			g_pStaInfo[2].MeterNumConnect = 3;

			g_pMeterRelayInfo[0].coll_sn = 2;
			g_pMeterRelayInfo[0].meter_addr[0] = 0x01;
			g_pMeterRelayInfo[0].meter_addr[1] = 0x01;
			g_pMeterRelayInfo[0].meter_addr[2] = 0x01;
			g_pMeterRelayInfo[0].meter_addr[3] = 0x01;
			g_pMeterRelayInfo[0].meter_addr[4] = 0x01;
			g_pMeterRelayInfo[0].meter_addr[5] = 0x01;
			g_pMeterRelayInfo[0].status.IsMeter = 0;
			g_pMeterRelayInfo[0].status.IsCollector = 1;
			g_pMeterRelayInfo[0].status.used = CCO_USERD;
			g_pMeterRelayInfo[0].status.cfged = 1;

			g_pMeterRelayInfo[1].coll_sn = 2;
			g_pMeterRelayInfo[1].meter_addr[0] = 0x02;
			g_pMeterRelayInfo[1].meter_addr[1] = 0x02;
			g_pMeterRelayInfo[1].meter_addr[2] = 0x02;
			g_pMeterRelayInfo[1].meter_addr[3] = 0x02;
			g_pMeterRelayInfo[1].meter_addr[4] = 0x02;
			g_pMeterRelayInfo[1].meter_addr[5] = 0x02;
			g_pMeterRelayInfo[1].status.IsMeter = 0;
			g_pMeterRelayInfo[1].status.IsCollector = 0;
			g_pMeterRelayInfo[1].status.used = CCO_USERD;
			g_pMeterRelayInfo[1].status.cfged = 1;

			g_pMeterRelayInfo[3].coll_sn = 2;
			g_pMeterRelayInfo[3].meter_addr[0] = 0x03;
			g_pMeterRelayInfo[3].meter_addr[1] = 0x03;
			g_pMeterRelayInfo[3].meter_addr[2] = 0x03;
			g_pMeterRelayInfo[3].meter_addr[3] = 0x03;
			g_pMeterRelayInfo[3].meter_addr[4] = 0x03;
			g_pMeterRelayInfo[3].meter_addr[5] = 0x03;
			g_pMeterRelayInfo[3].status.IsMeter = 0;
			g_pMeterRelayInfo[3].status.IsCollector = 1;
			g_pMeterRelayInfo[3].status.used = CCO_USERD;
			g_pMeterRelayInfo[3].status.cfged = 1;

			g_pStaInfo[3].net_state = 1;
			g_pStaInfo[3].MeterNumConnect = 2;

			g_pMeterRelayInfo[4].coll_sn = 3;
			g_pMeterRelayInfo[4].meter_addr[0] = 0x06;
			g_pMeterRelayInfo[4].meter_addr[1] = 0x06;
			g_pMeterRelayInfo[4].meter_addr[2] = 0x06;
			g_pMeterRelayInfo[4].meter_addr[3] = 0x06;
			g_pMeterRelayInfo[4].meter_addr[4] = 0x06;
			g_pMeterRelayInfo[4].meter_addr[5] = 0x06;
			g_pMeterRelayInfo[4].status.IsMeter = 0;
			g_pMeterRelayInfo[4].status.IsCollector = 1;
			g_pMeterRelayInfo[4].status.used = CCO_USERD;
			g_pMeterRelayInfo[4].status.cfged = 1;

			g_pMeterRelayInfo[5].coll_sn = 3;
			g_pMeterRelayInfo[5].meter_addr[0] = 0x05;
			g_pMeterRelayInfo[5].meter_addr[1] = 0x05;
			g_pMeterRelayInfo[5].meter_addr[2] = 0x05;
			g_pMeterRelayInfo[5].meter_addr[3] = 0x05;
			g_pMeterRelayInfo[5].meter_addr[4] = 0x05;
			g_pMeterRelayInfo[5].meter_addr[5] = 0x05;
			g_pMeterRelayInfo[5].status.IsMeter = 1;
			g_pMeterRelayInfo[5].status.IsCollector = 0;
			g_pMeterRelayInfo[5].status.used = CCO_USERD;
			g_pMeterRelayInfo[5].status.cfged = 1;

			g_pMeterRelayInfo[8].coll_sn = 3;
			g_pMeterRelayInfo[8].meter_addr[0] = 0x04;
			g_pMeterRelayInfo[8].meter_addr[1] = 0x04;
			g_pMeterRelayInfo[8].meter_addr[2] = 0x04;
			g_pMeterRelayInfo[8].meter_addr[3] = 0x04;
			g_pMeterRelayInfo[8].meter_addr[4] = 0x04;
			g_pMeterRelayInfo[8].meter_addr[5] = 0x04;
			g_pMeterRelayInfo[8].status.IsMeter = 0;
			g_pMeterRelayInfo[8].status.IsCollector = 1;
			g_pMeterRelayInfo[8].status.used = CCO_USERD;
			g_pMeterRelayInfo[8].status.cfged = 1;
*/		
			memcpy_special(&MeterStartSN, pLmpAppInfo->curProcUnit->data_unit, 2);
			MeterNum = pLmpAppInfo->curProcUnit->data_unit[2];

			//参数校验
			if((MeterNum == 0) || (MeterStartSN != 1))
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			//发上行报文
            memcpy_special(pLmpAppInfo->msg_buffer+pLmpAppInfo->responsePoint,&g_pRelayStatus->all_meter_num, 2);
			pLmpAppInfo->responsePoint += 2;

			memcpy_special(pLmpAppInfo->msg_buffer+pLmpAppInfo->responsePoint,&MeterStartSN, 2);//电表起始序号
			pLmpAppInfo->responsePoint += 2;

            real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]; 
			*real_num = 0;

			totalsize += 5;

			u16 rsvbuffersize = 0;

			for(i = 0; i < CCTT_METER_NUMBER; i++)//电表基本信息
			{
				if (!METER_IS_CFG(i))
					continue;
			
				pNode = &g_pMeterRelayInfo[i];
				collect_sn = pNode->coll_sn;
				if(collect_sn >= CCTT_METER_NUMBER)
				{
					continue;
				}
				P_STA_INFO psta=PLC_GetStaInfoByTei(collect_sn);
				if(psta == NULL || psta->net_state != 1)//采集器离线后不返回对应下挂电表信息
				{
					continue;
				}
				if((pNode->status.IsMeter == 0)&&(pNode->status.IsCollector == 1))
				{
					
					P_RELAY_INFO p_collectRelayInfo = &g_MeterRelayInfo[collect_sn];
					if(p_collectRelayInfo->status.IsCollector == 0)
					{
						memcpy_special(&CollectMac,p_collectRelayInfo->meter_addr, 6);
					}
				
					memcpy_special(&MeterMac,pNode->meter_addr, 6);
				}
                else if((pNode->status.IsMeter == 1)&&(pNode->status.IsCollector == 0))
            	{
            		memcpy_special(&CollectMac,pNode->meter_addr, 6);
            		MeterMac[0] = 0xEE;
                    MeterMac[1] = 0xEE;
                    MeterMac[2] = 0xEE;
                    MeterMac[3] = 0xEE;
                    MeterMac[4] = 0xEE;
                    MeterMac[5] = 0xEE;
            	}
				else if((pNode->status.IsMeter == 0)&&(pNode->status.IsCollector == 0))
				{
					memcpy_special(&CollectMac,pNode->meter_addr, 6);
					memcpy_special(&MeterMac,pNode->meter_addr, 6);
				}
				else
				{
					continue;
				}

				rsvbuffersize = MSA_MESSAGAE_MAX_SIZE - totalsize;
				if(rsvbuffersize < 13)//剩余空间放不下一个电表信息,跳出当前循环
				{
					break;
				}
				
				memcpy_special(pLmpAppInfo->msg_buffer+pLmpAppInfo->responsePoint,&CollectMac, 6);//电表隶属采集器逻辑地址
				pLmpAppInfo->responsePoint += 6;

				memcpy_special(pLmpAppInfo->msg_buffer+pLmpAppInfo->responsePoint,&MeterMac, 6);//电表地址
				pLmpAppInfo->responsePoint += 6;

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = PLC_meterprotocol_to_3762protocol(pNode->meter_type.protocol);
				pLmpAppInfo->responsePoint += 1;

				(*real_num)++;
				if(MeterNum == (*real_num))
				{
					break;
				}
				totalsize += 13;
			}
		
		}
		break;               
	case F160: 
		{
			USHORT ii;

			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT *total_num;

			PLC_get_relay_info();

			total_num=(USHORT *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
			*total_num=0;
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)((ULONG)pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);
			*real_num = 0;

			pLmpAppInfo->responsePoint++;

			for (ii = 0; ii <  CCTT_BLACK_METER_NUMBER; ii++)
			{

				if (g_BlackMeterInfo[ii].used != CCO_USERD)
				{
					continue;
				}
			
				(*total_num)++;
				if (start_sn)
				{
					start_sn--;
					if (start_sn!=0)
					{
						continue;
					}
				}
				if (*real_num == meter_num)
					continue;
				(*real_num)++;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
							   g_BlackMeterInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;

			}

			break;
		}                
#endif
	case F214:
		{
			USHORT ii;
//            P_RELAY_INFO p_relayInfo;
			UCHAR  meter_num;
			USHORT meter_sn;

			PBYTE pMeterAddr;

			meter_num = pLmpAppInfo->curProcUnit->data_unit[0];

			for (ii = 0; ii < meter_num; ii++)
			{
				pMeterAddr = (PBYTE)&pLmpAppInfo->curProcUnit->data_unit[1 + ii * CCTT_METER_ADDR_LEN];
				meter_sn = PLC_get_sn_from_addr(pMeterAddr);

				if (meter_sn >= CCTT_METER_NUMBER)
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x01;
				else if (!MF_RELAY_FREEZED(meter_sn))
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x02;
				else
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x00;
				pLmpAppInfo->responsePoint++;
			}

			break;
		}
	case F230: //私有协议读档案
		{
			USHORT ii;
//            P_RELAY_INFO p_relayInfo;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;

//            USHORT node_counter = 0;

			PLC_get_relay_info();

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
						   //&g_pRelayStatus->all_meter_num, 2);
						   &g_pRelayStatus->cfged_meter_num, 2);
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)((ULONG)pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);
			*real_num = 0;

			pLmpAppInfo->responsePoint++;

			//寻找真正的起始位置
			if (!PLC_GetRealNodeSnCfg(FALSE, start_sn, &start_sn))
				break;

			for (ii = start_sn; ii <  CCTT_METER_NUMBER; ii++)
			{
				if (g_MeterRelayInfo[ii].status.used != CCO_USERD)
					continue;
				if (g_MeterRelayInfo[ii].status.cfged == 0)
					continue;

				(*real_num)++;

				*(USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]) = ii;
				pLmpAppInfo->responsePoint += 2;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
							   g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = PLC_GetStaLevel(PLC_GetStaSnByMac(g_MeterRelayInfo[ii].meter_addr));
#ifndef GDW_ZHEJIANG
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= (g_MeterRelayInfo[ii].status.plc << 4);
#endif
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= (g_MeterRelayInfo[ii].meter_type.m0c1 << 5);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= (g_MeterRelayInfo[ii].meter_type.phaseType << 6);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] |= (g_MeterRelayInfo[ii].bridge_flag << 7);

				pLmpAppInfo->responsePoint++;

				if (AC_PHASE_A == g_MeterRelayInfo[ii].status.phase)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 1;
				}
				else if (AC_PHASE_B == g_MeterRelayInfo[ii].status.phase)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 2;
				}
				else if (AC_PHASE_C == g_MeterRelayInfo[ii].status.phase)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 4;
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0;
				}

				pLmpAppInfo->responsePoint++;
#ifndef GDW_ZHEJIANG
				if (0 == g_MeterRelayInfo[ii].status.plc
					&& METER_TYPE_METER == g_MeterRelayInfo[ii].meter_type.m0c1)
				{
					*(USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]) = g_MeterRelayInfo[ii].coll_sn;
				}
				else
				{
					*(USHORT *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]) = 0xffff;
				}
#endif
				pLmpAppInfo->responsePoint += 2;

				if (*real_num == meter_num)
					break;
			}

			break;
		}
#ifdef GDW_2019_GRAPH_STOREDATA		
	case F103:
		{
			USHORT tei;
			USHORT start_sn = (pLmpAppInfo->curProcUnit->data_unit[1] << 8) | (pLmpAppInfo->curProcUnit->data_unit[0]);
			UCHAR  meter_num = (pLmpAppInfo->curProcUnit->data_unit[2]>30)?30:pLmpAppInfo->curProcUnit->data_unit[2];
			UCHAR *real_num;
			USHORT node_count = g_pRelayStatus->innet_num;

			if (0 == start_sn)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &node_count, 2);
			pLmpAppInfo->responsePoint += 2;

			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++];
			*real_num = 0;

			if (!PLC_GetRealStaSn(FALSE, start_sn, &start_sn))
				break;

			for (tei = start_sn; tei < TEI_CNT; tei++)
			{
				P_STA_INFO pSta = PLC_GetValidStaInfoByTei(tei);
				if (NULL == pSta)
					continue;

				if(tei != TEI_CCO)
				{
					memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, pSta->mac, MAC_ADDR_LEN);
					pLmpAppInfo->responsePoint += MAC_ADDR_LEN;

					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = (g_pStaStatusInfo[tei].sync_cfg_result == 1)?1:0;
					
					(*real_num)++;
				}

				if (*real_num == meter_num)
					break;
			}

			break;
		}
#endif	
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}

	return OK;
}

#if 0
u8 tmp_buffer[] =
{0x01, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF, 0xFE, 0xEE, 0x12, 0x79, 0x42, 0x00, 0x44, 0xFB, 0x21, 0xC0, 0x00, 0x00, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x30, 0x08, 0x00, 0xFF, 0x00, 0x00, 0x01, 0x00, 0x00, 0x11, 0x06, 0x00, 0x00, 0x01, 0x61, 0x20, 0x00, 0x36, 0x39, 0xCE, 0xD2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4B, 0x92, 0x3C};
#elif 0
u8 tmp_buffer1[] =
{0x01, 0x11, 0x22, 0x33, 0x00, 0xF0, 0xFF, 0x03, 0x02, 0x11, 0x30, 0xD0, 0x00, 0xC8, 0x33, 0xC3, 0x40, 0x00, 0x00, 0xFF, 0x2F, 0x00, 0x11, 0x11, 0x01, 0xFC, 0x00, 0x11, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0xF3, 0x10, 0x69};
u8 tmp_buffer2[] =
{0x01, 0x11, 0x22, 0x33, 0x00, 0xF0, 0xFF, 0x03, 0x02, 0x11, 0x30, 0xD0, 0x00, 0xC8, 0x33, 0xC3, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x81, 0xE7, 0x70};
u8 tmp_buffer3[] =
{0x01, 0x11, 0x22, 0x33, 0x00, 0xF0, 0xFF, 0x03, 0x02, 0x11, 0x30, 0xD0, 0x00, 0xC8, 0x33, 0xC3, 0x02, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xF2, 0x55, 0x3A};
u8 tmp_buffer4[] =
{0x01, 0x11, 0x22, 0x33, 0x00, 0xF0, 0xFF, 0x03, 0x02, 0x11, 0x30, 0xD0, 0x00, 0xC8, 0x33, 0xC3, 0x83, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0xB2, 0xA2, 0xB2, 0x0B, 0xE4, 0xC7, 0x8E};

#else

u8 tmp_buffer1[] = {
	0x01, 0x03, 0x00, 0x00, 0x01, 0xF0, 0xFF, 0xFE, 0x68, 0x12, 0x5F, 0x12, 0x00, 0x9E, 0xE3, 0x96, 0x40, 0x10, 0x00, 0xFF, 0x1F, 0x00, 0x05, 0x00, 0x30, 0xF0, 0x07, 0xFF, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xA8, 0x01, 0x63, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5E, 0x58, 0x9A
};
u8 tmp_buffer2[] = {
	0x01, 0x03, 0x00, 0x00, 0x01, 0xF0, 0xFF, 0xFE, 0x68, 0x12, 0x5F, 0x12, 0x00, 0x9E, 0xE3, 0x96, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD7, 0x9B, 0xC3
};
u8 tmp_buffer3[] = {
	0x01, 0x03, 0x00, 0x00, 0x01, 0xF0, 0xFF, 0xFE, 0x68, 0x12, 0x5F, 0x12, 0x00, 0x9E, 0xE3, 0x96, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x34, 0x72
};
u8 tmp_buffer4[] = {
	0x01, 0x03, 0x00, 0x00, 0x01, 0xF0, 0xFF, 0xFE, 0x68, 0x12, 0x5F, 0x12, 0x00, 0x9E, 0xE3, 0x96, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x1E, 0x43, 0x2D, 0x20, 0x1A, 0x6E, 0x2D, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCF, 0x73, 0x72, 0xB8, 0xC9, 0xC9, 0x55
};

#endif
const u8 zero_mac[6]={
	  0x01,0x11,0x00,0x68,0x21,0x19
};
unsigned char afn_relay_setting_11(P_LMP_APP_INFO pLmpAppInfo)
{
	UCHAR ii, meter_num = pLmpAppInfo->curProcUnit->data_unit[0];
	UCHAR *del_addr;

	switch (pLmpAppInfo->curDT)
	{
	case F1:    //F1：添加载波从节点
		{
#if 1 //兼容09和13规约
			UCHAR protocol_type = 0;
			USHORT meter_data_len = 0, index = 1;

			meter_data_len = pLmpAppInfo->appLayerLen - 4; //afn(1)+FN(2)+(从节点数量)1

			if(meter_data_len == (meter_num*7)) //13规约，地址(6)+规约类型(1)
			{
				protocol_type = 0; 
			}
			else if(meter_data_len == (meter_num*9)) //09规约，地址(6)+序号(2)+规约类型(1)
			{
				protocol_type = 1;
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}
			PLC_get_relay_info();
			if (g_pRelayStatus->all_meter_num+meter_num > (TEI_STA_MAX-TEI_STA_MIN+1)) 
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NONE;
				return ERROR;
			}
			debug_str(DEBUG_LOG_NET, "add node num %d", meter_num);
			if ((g_PlmConfigPara.test_flag&1)&&meter_num==1)
			{
				if (memcmp(zero_mac,&pLmpAppInfo->curProcUnit->data_unit[index], 6)==0)
				{
					g_PlmConfigPara.test_flag|=2;
				}
			}
			#ifdef ERROR_APHASE
			//重置广播配置电流事件
			g_pRelayStatus->phase_err_cb.sta_percent=0;
			#endif
#ifdef GDW_NEIMENG //只要有重复的就回复否认
			for(ii = 0; ii < meter_num; ii++)
			{
				UCHAR meter_mac[6] = {0};

				memcpy_special(meter_mac, &pLmpAppInfo->curProcUnit->data_unit[index], 6); //从节点地址

				if (PLC_find_new_node(meter_mac) == FALSE) 
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_DUPLICATE_METERNO;
					return ERROR;
				}

				if(protocol_type == 1) //09规约
				{
					index += 9; //6字节(MAC)+2字节(序号)+1字节(规约)
				}
				else //13规约
				{
					index += 7; //6字节(MAC)+1字节(规约)
				}
			}

			index = 1;
#endif			
			
			for(ii = 0; ii < meter_num; ii++)
			{
				UCHAR meter_mac[6] = {0};
				BYTE protocol_meter = 0, protocol_3762 = 0;
				USHORT meter_sn = 0;

				memcpy_special(meter_mac, &pLmpAppInfo->curProcUnit->data_unit[index], 6); //从节点地址
				index += 6;

				if(protocol_type == 1) //09规约
				{
					memcpy_special((UCHAR *)&meter_sn, &pLmpAppInfo->curProcUnit->data_unit[index], 2); //从节点序号
					index += 2;

					if(meter_sn >= CCTT_METER_NUMBER)
					{
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_METERNO_INEXISTS;
						return ERROR;
					}
				}
				
				protocol_3762 = pLmpAppInfo->curProcUnit->data_unit[index] & 0x7f; //协议类型最高位用于区分是否晓程N6表计

				if (METER_PROTOCOL_3762_97 == protocol_3762)
					protocol_meter = METER_TYPE_PROTOCOL_97;
				else if (METER_PROTOCOL_3762_07 == protocol_3762)
					protocol_meter = METER_TYPE_PROTOCOL_07;
				else if (METER_PROTOCOL_3762_69845 == protocol_3762)
					protocol_meter = METER_TYPE_PROTOCOL_69845;
				else
				{
#if defined(APP_KENENG_MXDX)
//                    protocol_meter = METER_TYPE_PROTOCOL_07;
					protocol_meter = METER_TYPE_PROTOCOL_TRANSPARENT;
#else
//                    protocol_meter = METER_TYPE_PROTOCOL_07;
//                    protocol_meter = METER_TYPE_PROTOCOL_69845;
					protocol_meter = METER_TYPE_PROTOCOL_TRANSPARENT;
#endif
				}

				protocol_meter |= (pLmpAppInfo->curProcUnit->data_unit[index++] & 0x80);

#ifdef GDW_ZHEJIANG
				if (g_PlmConfigPara.close_white_list == 2)
				{
					meter_sn = CCTT_METER_NUMBER;
				}
#endif

				PLC_add_new_node(meter_sn, protocol_meter, meter_mac);
			}
#else		
			P_RT_NODE rt_mode;
			rt_mode = (P_RT_NODE)(pLmpAppInfo->curProcUnit->data_unit + 1);
			debug_str(DEBUG_LOG_NET, "add node num %d", meter_num);
			for (ii = 0; ii < meter_num; ii++, rt_mode++)
			{
				BYTE protocol_meter;
				BYTE protocol_3762 = rt_mode->protocol & 0x7f;      //协议类型最高位用于区分是否晓程N6表计

				if (METER_PROTOCOL_3762_97 == protocol_3762)
					protocol_meter = METER_TYPE_PROTOCOL_97;
				else if (METER_PROTOCOL_3762_07 == protocol_3762)
					protocol_meter = METER_TYPE_PROTOCOL_07;
				else if (METER_PROTOCOL_3762_69845 == protocol_3762)
					protocol_meter = METER_TYPE_PROTOCOL_69845;
				else
				{
#if defined(APP_KENENG_MXDX)
//                    protocol_meter = METER_TYPE_PROTOCOL_07;
					protocol_meter = METER_TYPE_PROTOCOL_TRANSPARENT;
#else
//                    protocol_meter = METER_TYPE_PROTOCOL_07;
//                    protocol_meter = METER_TYPE_PROTOCOL_69845;
					protocol_meter = METER_TYPE_PROTOCOL_TRANSPARENT;
#endif
				}

				protocol_meter |= (rt_mode->protocol & 0x80);

#ifndef GDW_2012
				if (rt_mode->meter_sn >= CCTT_METER_NUMBER)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_METERNO_INEXISTS;
					pLmpAppInfo->responsePoint++;
					return ERROR;
				}
				/*
				if(CCO_USERD == g_MeterRelayInfo[rt_mode->meter_sn].status.used)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_DUPLICATE_METERNO;
					pLmpAppInfo->responsePoint++;
					return ERROR;
				}
				*/
				PLC_add_new_node(rt_mode->meter_sn, protocol_meter, rt_mode->addr);
#else
				PLC_add_new_node(0, protocol_meter, rt_mode->addr);
#endif
			}
#endif
			break;
		}
	case F2: // F2：删除载波从节点
		{
			del_addr = (pLmpAppInfo->curProcUnit->data_unit + 1);
			debug_str(DEBUG_LOG_NET, "delete node num %d", meter_num);
			for (ii = 0; ii < meter_num; ii++)
			{
				PLC_del_new_node(del_addr,LIST_TYPE_WHITE);
				del_addr += 6;
			}

			break;
		}
#ifndef OVERSEA
	case F3: //F3：设置载波从节点固定中继路径
		{
			PLC_set_fix_relay((P_RT_RELAY)pLmpAppInfo->curProcUnit->data_unit);
			break;
		}
#endif
	case F4:    //  F4：设置工作模式
		{
			memcpy_special(&g_PlmConfigPara.rm_run_switch, pLmpAppInfo->curProcUnit->data_unit, 1);
			dc_flush_fram(&g_PlmConfigPara.rm_run_switch, 1, FLASH_FLUSH);
			memcpy_special(&g_PlmConfigPara.rm_plc_rate, pLmpAppInfo->curProcUnit->data_unit + 1, 2);
			dc_flush_fram(&g_PlmConfigPara.rm_plc_rate, 2, FLASH_FLUSH);

			//百富集中器 第二天 通过设置模式启动重新抄表
#ifdef APP_CUS_BAIFU
			if (0 == (0x01 & g_PlmConfigPara.rm_run_switch))
			{
				PLC_stage_start();
			}
#endif
			break;
		}
	case F5:   // F5: 激活载波从节点注册
		{
			//河北台体的上报时间设了30分钟
			P_AFN11_F5 pDataUnit = (P_AFN11_F5)pLmpAppInfo->curProcUnit->data_unit;
			if (OK != PLC_StartSelfReg(pDataUnit->time_last_min))
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_BUSY_NOW;
				return (!OK);
			}
			break;
		}
	case F6:   //F6：终止从节点主动注册
		{
			PLC_StopSelfReg();
			break;
		}
	case F7: //友讯达afn_11_07命令回一个确认
		{
			return OK;
		}
	case F22:
		{
			//设定指定表相位
			PBYTE p = &pLmpAppInfo->curProcUnit->data_unit[0];
			PBYTE pMeterAddr;
			BYTE phase = 1;
			USHORT metersn;

			meter_num = *p++;
			for (ii = 0; ii < meter_num; ii++)
			{
				phase = *p++;
				pMeterAddr = p;
				p += 6;

				phase--;
				if (phase > AC_PHASE_U)
					phase = AC_PHASE_U;

				metersn = PLC_get_sn_from_addr(pMeterAddr);
				if (metersn < CCTT_METER_NUMBER)
				{
					g_MeterRelayInfo[metersn].status.phase = phase;
					PLC_save_relay_timer_start();
				}
			}

			break;
		}
	case F23:
		{
			//设置波特率
			break;
		}
	case F100:
		{
			PLC_save_relay_info(NULL_PTR, NULL_PTR);
			PLC_relay_sorage_init();
			break;
		}
	case F101:
		{
			BYTE count = pLmpAppInfo->curProcUnit->data_unit[0];
			BYTE flag = pLmpAppInfo->curProcUnit->data_unit[1];

			for (ii = 0; ii < count; ii++)
			{
				PBYTE pMeterAddr = &pLmpAppInfo->curProcUnit->data_unit[2 + ii * CCTT_METER_ADDR_LEN];
				USHORT meter_sn = PLC_get_sn_from_addr(pMeterAddr);
				if ((meter_sn < CCTT_METER_NUMBER) && (CCO_USERD == g_MeterRelayInfo[meter_sn].status.used))
					g_MeterRelayInfo[meter_sn].bridge_flag = flag;
			}

			break;
		}
	case F102:
		{
			for (USHORT meter_sn = 0; meter_sn < CCTT_METER_NUMBER; meter_sn++)
			{
				if (CCO_USERD != g_MeterRelayInfo[meter_sn].status.used)
					continue;
				g_MeterRelayInfo[meter_sn].bridge_flag = 0;
			}
			break;
		}
	case F103:
		{
			//u8 mac_addr[MAC_ADDR_LEN] = {0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
			//PLC_AddToOfflineList(mac_addr, OFFLINE_NOTIFY);
			break;
		}
	case F104:
		{
			USHORT meter_from, meter_to;
			USHORT meter_sn;
			BYTE meter_addr[CCTT_METER_ADDR_LEN];

			memcpy_special(&meter_from, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
			meter_to = meter_from + pLmpAppInfo->curProcUnit->data_unit[2] - 1;

			for (meter_sn = meter_from; meter_sn <= meter_to; meter_sn++)
			{
				memset_special(meter_addr, 0x00, sizeof(meter_addr));
				meter_addr[0] = Hex2BcdChar((BYTE)(meter_sn % 100));
				meter_addr[1] = Hex2BcdChar((BYTE)(meter_sn / 100 % 100));
				meter_addr[2] = Hex2BcdChar((BYTE)(meter_sn / 10000 % 100));

				PLC_add_new_node(meter_sn, METER_TYPE_PROTOCOL_07, meter_addr);
			}
			break;
		}
	case F105:
		{
//            u8 tmp_buffer[] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x5C, 0x78, 0xFF, 0x08, 0x00, 0x46, 0x00, 0x00, 0x02, 0x19, 0x04, 0x07, 0x49, 0x12, 0x00, 0xA1, 0x93, 0x21, 0x32, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x19, 0x04, 0x07, 0x49, 0x12, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0xCF, 0xAE, 0xB5, 0xD0, 0x00, 0x07, 0x11, 0xC8, 0x03, 0x00, 0x12, 0x06, 0x05, 0x11, 0x2C, 0x02, 0x05, 0x00, 0x16, 0x00, 0x48, 0x0F, 0x01, 0x07, 0x16, 0x23, 0x12, 0x2B, 0x48, 0x52, 0x30, 0x30, 0x5F};
//            PLC_MacFrameCallback((P_MAC_FRAME)tmp_buffer);

#if 0
			BPLC_recv_para para;
			HPLC_ReceiveCallback(tmp_buffer, sizeof(tmp_buffer), &para);
#elif 0
			BPLC_recv_para para;
			HPLC_ReceiveCallback(tmp_buffer1, sizeof(tmp_buffer1), &para);
			HPLC_ReceiveCallback(tmp_buffer2, sizeof(tmp_buffer2), &para);
			HPLC_ReceiveCallback(tmp_buffer3, sizeof(tmp_buffer3), &para);
			HPLC_ReceiveCallback(tmp_buffer4, sizeof(tmp_buffer4), &para);
#else
			P_APP_PACKET pAppFrame = HplcApp_MakeEnterTestModeFrame((COMM_TEST_MODE)pLmpAppInfo->curProcUnit->data_unit[0], 3600);     //TEST_MODE_TRAN_MAC_TO_COMM
			if(NULL == pAppFrame)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_CRC;
				return (!OK);
			}
			
			MAC_PARAM macParam;
			#ifdef FORWARD_FRAME
			macParam.SrcTei = CCO_TEI;
			#endif
			macParam.Broadcast = BROADCAST_DRT_DUL;
			macParam.DstTei = TEI_BROADCAST;
			macParam.HopCount = 1;
			macParam.mac_flag = MACFLAG_NONE;
			macParam.MsduType = MSDU_APP_FRAME;
			macParam.Broadcast = BROADCAST_DRT_DUL;
			macParam.NetSeq = 0;

			u16 msduLen = sizeof(APP_PACKET) - 1 + sizeof(COMM_TEST_FRAME_PACKET)-1;

			P_MAC_FRAME pMacFrame = MakeMacFrame(&macParam, (u8 *)pAppFrame, msduLen);
			if(NULL == pMacFrame)
			{
				free_buffer(__func__,pAppFrame);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_CRC;
				return (!OK);
			}
			
			SOF_PARAMS SOFParam;
			SOFParam.BroadcastFlag = 1;
			SOFParam.DstTei = TEI_BROADCAST;
			SOFParam.Lid = 1;
			SOFParam.NID = 0;

			PDU_BLOCK *pud = MpduCreateCommTestFrame(&SOFParam, (u8 *)pMacFrame, GetMacFrameLen(pMacFrame));
			if(NULL == pud)
			{
				free_buffer(__func__,pMacFrame);
				free_buffer(__func__,pAppFrame);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_CRC;
				return (!OK);
			}
			
			BPLC_recv_para para;
			HPLC_ReceiveCallback(pud->pdu, pud->size, &para, 0xf);

			FreePDU(pud);
			free_buffer(__func__,pMacFrame);
			free_buffer(__func__,pAppFrame);

	#if 0
			OS_ERR err;
			OSTimeDly(200, OS_OPT_TIME_DLY, &err);

			HPLC_ReceiveCallback(tmp_buffer1, sizeof(tmp_buffer1), &para);
			OSTimeDly(96, OS_OPT_TIME_DLY, &err);
			HPLC_ReceiveCallback(tmp_buffer2, sizeof(tmp_buffer2), &para);
			OSTimeDly(96, OS_OPT_TIME_DLY, &err);
			HPLC_ReceiveCallback(tmp_buffer3, sizeof(tmp_buffer3), &para);
			OSTimeDly(96, OS_OPT_TIME_DLY, &err);
			HPLC_ReceiveCallback(tmp_buffer4, sizeof(tmp_buffer4), &para);

			OSTimeDly(500, OS_OPT_TIME_DLY, &err);
	#endif
#endif

			break;
		}
	case F106:
		{
			PLC_SEND_PARAM sendParam;
			sendParam.resend_count = 2;
			sendParam.send_type = CSMA_SEND_TYPE_CSMA;
			sendParam.cb = NULL;
			sendParam.send_phase = (HPLC_SEND_PHASE)(1 << pLmpAppInfo->curProcUnit->data_unit[0]);
			if (sendParam.send_phase > (SEND_PHASE_A + SEND_PHASE_B + SEND_PHASE_C + SEND_PHASE_ALL))
				sendParam.send_phase = SEND_PHASE_ALL;
			sendParam.link = FreamPathBplc;
			HPLC_SendTest(&sendParam, &pLmpAppInfo->curProcUnit->data_unit[1], pLmpAppInfo->appLayerLen - 4);
			//u8 pdu[] = {0x01, 0x60, 0x16, 0xA3, 0x01, 0x20, 0x00, 0x03, 0x13, 0x13, 0x60, 0xE0, 0x00, 0xFE, 0x34, 0xB6, 0xC0, 0x10, 0x00, 0x04, 0x00, 0x00, 0x09, 0x00, 0x30, 0x1C, 0x00, 0xFF, 0x00, 0x00, 0x46, 0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x02, 0x00, 0x12, 0x00, 0x68, 0x13, 0x49, 0x07, 0x04, 0x19, 0x02, 0x68, 0x11, 0x04, 0x33, 0x32, 0x35, 0x33, 0x34, 0x16, 0xBA, 0xB8, 0xC3, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCC, 0xAA, 0xCB};
			//HPLC_SendTest(&sendParam, pdu, sizeof(pdu));
			pLmpAppInfo->confirmFlag = 2;
			break;
		}
	case F107:
		{
			BPLC_recv_para para;
			para.sync_ntb = BPLC_GetNTB();
			para.snr = 20 * 4;
			para.rssi = 20;
			HPLC_ReceiveCallback(&pLmpAppInfo->curProcUnit->data_unit[0], pLmpAppInfo->appLayerLen - 3, &para, 0xf);
			pLmpAppInfo->confirmFlag = 2;
			break;
		}
#ifdef GDW_ZHEJIANG
        case F160:    
		{
			USHORT index = 1;
            UCHAR black_meter_num = pLmpAppInfo->curProcUnit->data_unit[0];
			
			for(int i = 0; i < black_meter_num; i++)
			{
				UCHAR meter_mac[6] = {0};
				USHORT meter_sn = 0;

				memcpy_special(meter_mac, &pLmpAppInfo->curProcUnit->data_unit[index], 6); //从节点地址
				index += 6;
	
				if (g_PlmConfigPara.close_white_list == 2)
				{
					meter_sn = CCTT_METER_NUMBER;
				}

				PLC_add_new_node(meter_sn, (METER_TYPE_PROTOCOL_07| 0x10 ), meter_mac);
			}

			break;
		}
#endif                
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}

	return OK;
}

unsigned char afn_relay_action_12(P_LMP_APP_INFO pLmpAppInfo)
{
	switch (pLmpAppInfo->curDT)
	{
	case F1:  //F1：重启
		{
			PLC_stage_start();
			break;
		}
	case F2: // F2：暂停
		{
#if defined(APP_CONCENTRATOR_MODE)
			PLC_ClearCacheData();
#endif
			PLC_stage_pause();
			break;
		}
	case F3: //F3：恢复
		{
			PLC_stage_resume();
			break;
		}
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}
	/*
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0xFF;
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = 0xFF;
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = 0x02;
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+3] = 0x00;
		pLmpAppInfo->responsePoint += 4;
	*/
	return OK;
}

//路由数据转发类（AFN=13H）
unsigned char afn_relay_data_forward_13(P_LMP_APP_INFO pLmpAppInfo)
{
	UCHAR *recv_buf = &pLmpAppInfo->curProcUnit->data_unit[0];
	U16 len;
//    USHORT sn;
//    USHORT rc;
	P_GD_DL645_BODY pDL645 = NULL;
//    P_DL69845_BODY pDL69845 = NULL;
//    UCHAR* pRelayAddr = NULL;
	UCHAR *pDesAddr = NULL;

	if (pLmpAppInfo->lmpLinkHead->Inf.down_info.comm_module)
	{
		u8 level = pLmpAppInfo->lmpLinkHead->Inf.down_info.relay_level;
//      if(level > 0)
//          pRelayAddr = &pLmpAppInfo->lmpLinkHead->user_data[6];
		pDesAddr = &pLmpAppInfo->lmpLinkHead->user_data[6 + 6 * level];
	}

	switch (pLmpAppInfo->curDT)
	{
		// 02 00 10 68 01 00 00 00 00 00 68 11 04 33 33 34 33 B3 16 42 16 0C 16
	case F1:  //F1：监控载波从节点
	case F2:
		{
			UCHAR protocol;
			u8 time_related = 0;
			EM_SENDER sender;
#if !defined(GDW_SHENYANG)			
			u8 addr_no_exist = 0; //sta不在档案
            (void)addr_no_exist;
#endif			
#if 0
	#ifdef GDW_2012
			protocol = recv_buf[0];
			time_related = recv_buf[1];
			recv_buf += (recv_buf[2]*6 + 3);
			len = recv_buf[0];
			recv_buf++;
	#else
			protocol = recv_buf[0];
			recv_buf += (recv_buf[1]*6 + 2);
			len = recv_buf[0];
			recv_buf++;
	#endif
#else
			P_AFN13_F1_09 pAFN09 = (P_AFN13_F1_09)recv_buf;
			P_AFN13_F1_13 pAFN13 = (P_AFN13_F1_13)recv_buf;

			if (pLmpAppInfo->curDT==F1)
			{
				if (pLmpAppInfo->appLayerLen == (3 + sizeof(AFN13_F1_09) + pAFN09->data[pAFN09->slave_num * 6]))
				{
					protocol = pAFN09->protocol;

					len = pAFN09->data[pAFN09->slave_num * 6];
					recv_buf = &pAFN09->data[pAFN09->slave_num*6+1];
		
					sender = EM_SENDER_AFN_GDW2009;
				}
				else if (pLmpAppInfo->appLayerLen == (3 + sizeof(AFN13_F1_13) + pAFN13->data[pAFN13->slave_num*6]))
				{
					protocol = pAFN13->protocol;
					len = pAFN13->data[pAFN13->slave_num*6];
					recv_buf = &pAFN13->data[pAFN13->slave_num*6+1];
					time_related = pAFN13->time_related;
					sender = EM_SENDER_AFN_GDW2013;
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
					return ERROR;
				}
			}
			else
			{
				protocol = pAFN09->protocol;
				len = pAFN09->data[pAFN09->slave_num * 6] | (pAFN09->data[pAFN09->slave_num * 6 + 1] << 8);
				if (pLmpAppInfo->appLayerLen != (3 + sizeof(AFN13_F1_09) + 1 + len))
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
					return ERROR;
				}

				recv_buf = &pAFN09->data[pAFN09->slave_num * 6 + 2];
				sender = EM_SENDER_AFN_GDW2009;
			}

#endif
			if (NULL == pDesAddr)
			{
				pDL645 = PLC_check_dl645_frame(recv_buf, len);
				P_DL69845_BODY pDL698 =  PLC_check_69845_frame(recv_buf, len);
				if (pDL645)
				{
					pDesAddr = pDL645->meter_number;
				}
				else if (pDL698)
				{
					pDesAddr = PLC_get_69845_SAMac(pDL698, NULL);
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
					return ERROR;
				}

			}
#if !defined(GDW_SHENYANG)
			if (PLC_get_sn_from_addr(pDesAddr) >= CCTT_METER_NUMBER)
			{
#if defined(SUPPORT_READ_METER_NOT_IN_WHITELIST) && !defined(OVERSEA)	
				addr_no_exist = 1;
#else	
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_METERNO_INEXISTS;
				return ERROR;
#endif
			}

			u8 dst_addr[LONG_ADDR_LEN] = {0};
			PLC_GetAppDstMac(pDesAddr, dst_addr);
			
#ifdef OVERSEA
			P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
			if (NULL == pSta)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
				return ERROR;
			}
			
			if (pSta->net_state != NET_IN_NET) //海外版本不进行广播尝试，离线回复否认
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
				return ERROR;
			}
#else
			if (!addr_no_exist) //在档案的sta沿用老逻辑判断是否离网
			{
                if (PLC_GetStaInfoByMac(dst_addr) == NULL)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
			}
#endif
#endif
			P_READ_MSG_INFO pReadMsg = (P_READ_MSG_INFO)alloc_buffer(__func__,sizeof(READ_MSG_HEAD) + len);

			if (NULL == pReadMsg)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}

			pReadMsg->header.broadcast = 0;
			#if defined(GDW_SHENYANG)
			pReadMsg->header.broadcast = 1;
			#endif
			pReadMsg->header.read_count = 3; //SUPPORT_READ_METER_NOT_IN_WHITELIST由PLC_DirReadMeterProc控制抄读次数
			pReadMsg->header.read_time = 0;
			pReadMsg->header.sour_forme = pLmpAppInfo->end_id;
			pReadMsg->header.packet_seq_3762 = pLmpAppInfo->lmpLinkHead->Inf.down_info.seq_no;
			memcpy_special(pReadMsg->header.mac, pDesAddr, MAC_ADDR_LEN);
			pReadMsg->header.meter_protocol_plc = PLC_3762protocol_to_plcprotocol(protocol);
			pReadMsg->header.packet_id = APP_PACKET_COLLECT;
			pReadMsg->header.time_related = time_related;
			pReadMsg->header.sender = sender;
			pReadMsg->header.msg_len = len;
			pReadMsg->header.res_afn=AFN_FRAME_FORWARD;
			if(pLmpAppInfo->curDT==F2)
			{
				pReadMsg->header.res_afn|=0x80;
			}
			memcpy_special(pReadMsg->body, recv_buf, len);

			PLC_PostDirRead(pReadMsg);

			pLmpAppInfo->confirmFlag = 2;
			break;
		}
	case F4:
		{
			pLmpAppInfo->confirmFlag = 2;
			break;
		}
		// 02 00 10 68 01 00 00 00 00 00 68 11 04 33 33 34 33 B3 16 42 16 0C 16
	case F213:  //F213：监控载波从节点  use sn
		{
#if 0
			UCHAR protocol;
			protocol = recv_buf[0];
			recv_buf += (recv_buf[1]*6 + 2);
			len = recv_buf[0];
			recv_buf++;

			sn = ((USHORT)recv_buf[2] << 8) | recv_buf[1];

			if((sn >= CCTT_METER_NUMBER) || (CCO_USERD != g_MeterRelayInfo[sn].status.used))
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_FORMAT;
				pLmpAppInfo->responsePoint++;
				return ERROR;
			}

			if((protocol != 0x01)&&(protocol != 0x02))
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_FORMAT;
				pLmpAppInfo->responsePoint++;
				return ERROR;
			}

			if((len < 14) || (recv_buf[0] != 0x68))
			{

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_FORMAT;
				pLmpAppInfo->responsePoint++;
				return ERROR;
			}

			memcpy_special(&recv_buf[1], &g_MeterRelayInfo[sn].meter_addr[0], 6);

			rc = PLC_PostDirRead(sn,
								 protocol,
								 recv_buf,
								 len,
								 pRelayAddr,
								 pLmpAppInfo->lmpLinkHead->Inf.down_info.relay_level,
								 0,
								 0xff
								 );

			if(rc != OK)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_FORMAT;
				pLmpAppInfo->responsePoint++;
				return ERROR;
			}
#endif
			pLmpAppInfo->confirmFlag = 2;

			break;
		}
	case F214:
		{
			break;
		}

	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}

	return OK;
}
/*  F120: 设置路由波特率*/
unsigned char afn_F0_F120_proc(P_LMP_APP_INFO pLmpAppInfo)
{
	unsigned long baudrate_set = 0; //波特率
	//unsigned char ii;

	if ((pLmpAppInfo->appLayerLen - 9) != 4)
		return ERROR;

	memcpy_special(&baudrate_set, pLmpAppInfo->curProcUnit->data_unit, 4);
	/*  for(ii = 0;ii < 4; ii++)
			baudrate_set = (baudrate_set << 8) | pLmpAppInfo->curProcUnit->data_unit[ii];
		*/
	if (baudrate_set > 0x25800 || baudrate_set < 0x12C)
		return ERROR;

	switch (baudrate_set)
	{
	case 0x12C:     //300bps
	case 0x258: //600bps
	case 0x4B0: //1200
	case 0x960: //2400
	case 0x12C0: //4800
	case 0x1C20: //7200
	case 0x2580:
		//SPS1 = 0x0036;//SAU_CK00_FCLK_6 | SAU_CK01_FCLK_3;  //baudrate:9600bps
		break;
	case 0x4B00:
		//SPS1 = 0x0026;//SAU_CK00_FCLK_6 | SAU_CK01_FCLK_2;  //baudrate:19200bps
		break;
	case 0x9600:
		//SPS1 = 0x0016;//SAU_CK00_FCLK_6 | SAU_CK01_FCLK_1;  //baudrate:38400bps
		break;
	case 0xE100:
		//SPS1 = 0x0006;//SAU_CK00_FCLK_6 | SAU_CK01_FCLK_0;  //baudrate:57600bps
		break;
	case 0x13308: //76800bps
	case 0x1C200: //115200bps
	case 0x25800: //153600bps
	default:
		; //SPS1 = 0x0036;
	}
	return OK;
}

extern u8 zero_check_pass[];
extern u32 zero_check_ntb[];

/*AFN_F0:内部调试*/
unsigned char afn_F0_internal_debug(P_LMP_APP_INFO pLmpAppInfo)
{
	switch (pLmpAppInfo->curDT)
	{
	case F1: //中慧产测，CCO Wafer号
		{
			if (pLmpAppInfo->appLayerLen != 3) //afn(1)+FN(2)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			BPLC_GetWaferID(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			pLmpAppInfo->responsePoint += 8;

			break;
		}
	case F3: //断开抄控器链接
		{
			g_pRelayStatus->connectReader=0;
			return OK;
		}
	case F7://强制设置cco相线
		{
			if (pLmpAppInfo->curProcUnit->data_unit[0]>=PHASE_A&&pLmpAppInfo->curProcUnit->data_unit[0]<=PHASE_C)
			{
				g_PlmStatusPara.force_phase=pLmpAppInfo->curProcUnit->data_unit[0];
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}
		}
		break;
	case F8:    //查询节点代理主路径，应用手册，没实现
		{
			break;
		}
	case F9:    //查询节点聚类信息，应用手册，没实现
		{
			break;
		}
	case F14: //设置HPLC/HRF临时发送功率（产测）
		{
			if(pLmpAppInfo->curProcUnit->data_unit[0] == 0) //设置HPLC临时发送功率，AFN+Fn+数据
			{
				BPLC_SetTxGain(pLmpAppInfo->curProcUnit->data_unit[BPLC_TxFrequenceGet()+1]);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;				
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &pLmpAppInfo->curProcUnit->data_unit[1], 4);
				pLmpAppInfo->responsePoint += 4;					
			}
			else if(pLmpAppInfo->curProcUnit->data_unit[0] == 1)//设置HRF临时发送功率-无补偿
			{
				HRF_SetTxGain_abs(pLmpAppInfo->curProcUnit->data_unit[(HRF_Option&0x1)+1]);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1;				
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &pLmpAppInfo->curProcUnit->data_unit[1], 2);
				pLmpAppInfo->responsePoint += 2;	
			}
			else
			{
				HRF_SetTxGain(pLmpAppInfo->curProcUnit->data_unit[(HRF_Option&0x1)+1]);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 2;				
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &pLmpAppInfo->curProcUnit->data_unit[1], 2);
				pLmpAppInfo->responsePoint += 2;					
			}
			
			//memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			
			break;
		}		
	case F15: //生产MAC地址设置
		{
			if(pLmpAppInfo->appLayerLen != 9) //用户数据长度，AFN+Fn+数据
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}

			memcpy(g_cco_id.mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			SYS_STOR_CCO_ID();
			
            //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
            SYS_READ_CCO_ID();
			
			break;
		}
	case F16: //查询MAC地址查询
		{
			if(pLmpAppInfo->appLayerLen != 3) //用户数据长度，AFN+Fn+数据
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}

			//回复
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_cco_id.mac, 6);
			pLmpAppInfo->responsePoint += 6;
			
			break;
		}
	case F17: //打开/关闭singletones输出（产测）
		{
			HRF_phy_single_tone(pLmpAppInfo->curProcUnit->data_unit[0]);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pLmpAppInfo->curProcUnit->data_unit[0];
			break;
		}
	case F18: //设置/查询HRF发送功率补偿（产测）
		{
			if(pLmpAppInfo->curProcUnit->data_unit[0] != 1) 
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}
			
			if(pLmpAppInfo->appLayerLen != 4) //设置
			{
				//pLmpAppInfo->curProcUnit->data_unit[0]==8
				memcpy(g_PlmConfigPara.HRF_TXChlPowerComp, &pLmpAppInfo->curProcUnit->data_unit[1], 2);
				SYS_STOR_HRF_POWER_COMP();
                SetHrfPowerComp(g_PlmConfigPara.HRF_TXChlPowerComp[0],g_PlmConfigPara.HRF_TXChlPowerComp[1]);
				//u8 gain = HRF_GetTxGain();
				HRF_SetTxGain(g_PlmConfigPara.HRF_TXChlPowerComp[HRF_Option&0x1]);		
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1;				
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_PlmConfigPara.HRF_TXChlPowerComp, sizeof(g_PlmConfigPara.HRF_TXChlPowerComp));
				pLmpAppInfo->responsePoint += 2;					
			}
			else//查询
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1;
                SYS_READ_HRF_POWER_COMP();				
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], g_PlmConfigPara.HRF_TXChlPowerComp, sizeof(g_PlmConfigPara.HRF_TXChlPowerComp));
				pLmpAppInfo->responsePoint += 2;			
			}
			
			break;
		}		
	case F20://修改波特率
		{
			if (pLmpAppInfo->curProcUnit->data_unit[0]!=4
				&&pLmpAppInfo->curProcUnit->data_unit[0]!=8)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_CMD_NOT_SUPPORT;
				pLmpAppInfo->responsePoint++;
				return ERROR;
			}
			P_TMSG pMsg = alloc_msg(__func__);
			if (NULL != pMsg)
			{
				OS_ERR err;
				pMsg->message = MSG_PLM_SET_BAUDRATE;
				if (pLmpAppInfo->curProcUnit->data_unit[0]==8)
				{
					pMsg->wParam = 1;
					g_PlmConfigPara.baud_rate = 115200;
				}
				else 
				{
					pMsg->wParam = 0;
					g_PlmConfigPara.baud_rate = 9600;
				}
				SYS_STOR_BAUDRATE(g_PlmConfigPara.baud_rate);
				OSTaskQPost(&g_pOsRes->APP_PLM_2005_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);
				if (OS_ERR_NONE != err)
					free_msg(__func__, pMsg);
			}
			break;
		}
	case F25:
		{
#ifdef GDW_ZHEJIANG
			pLmpAppInfo->curProcUnit->data_unit[0] = pLmpAppInfo->curProcUnit->data_unit[0]?0:2; //上位机1为开启白名单，0为关闭白名单
#else		
			pLmpAppInfo->curProcUnit->data_unit[0] = pLmpAppInfo->curProcUnit->data_unit[0]?0:1; //上位机1为开启白名单，0为关闭白名单
#endif
			if (pLmpAppInfo->curProcUnit->data_unit[0] != g_PlmConfigPara.close_white_list)
			{
				g_PlmConfigPara.close_white_list = pLmpAppInfo->curProcUnit->data_unit[0];
				SYS_STOR_WHITE_LIST_SWITCH(g_PlmConfigPara.close_white_list);

                //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
                SYS_READ_WHITE_LIST_SWITCH();
			}
			break;
		}
	case F26:
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = !g_PlmConfigPara.close_white_list;
			break;
		}
	case F120:  //F120：设置路由波特率
		{
			afn_F0_F120_proc(pLmpAppInfo);
			break;
		}
	case F19:  //设置主节点厂商代码和版本信息（生产）
		{
			if(pLmpAppInfo->appLayerLen != 12) //用户数据长度，AFN+Fn+数据
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}

			c_Tmn_ver_info.factoryCode[0] = pLmpAppInfo->curProcUnit->data_unit[1];
			c_Tmn_ver_info.factoryCode[1] = pLmpAppInfo->curProcUnit->data_unit[0];
			c_Tmn_ver_info.chip_code[0] = pLmpAppInfo->curProcUnit->data_unit[3];
			c_Tmn_ver_info.chip_code[1] = pLmpAppInfo->curProcUnit->data_unit[2];
			c_Tmn_ver_info.softReleaseDate.day = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[4]);
			c_Tmn_ver_info.softReleaseDate.month = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[5]);
			c_Tmn_ver_info.softReleaseDate.year = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[6]);
			c_Tmn_ver_info.software_v[0] = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[8]);
			c_Tmn_ver_info.software_v[1] = Bcd2HexChar(pLmpAppInfo->curProcUnit->data_unit[7]);
			
			SYS_STOR_VER_INFO();
			
            //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
            SYS_READ_VER_INFO();
			
			break;
		}
	case F129://设置主节点rf发送功率
		{
			if (pLmpAppInfo->curProcUnit->data_unit[0] != 0xff)
			{
#if (HRF_MIN_POWER_VAL == 0)
				if (pLmpAppInfo->curProcUnit->data_unit[0] > HRF_MAX_POWER_VAL)
#else
				if ((pLmpAppInfo->curProcUnit->data_unit[0] < HRF_MIN_POWER_VAL) || (pLmpAppInfo->curProcUnit->data_unit[0] > HRF_MAX_POWER_VAL))
#endif
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DT;
					return ERROR;
				}
			}
			data_flash_write_straight_with_crc(STA_RF_POWER_MODE_ADDR, pLmpAppInfo->curProcUnit->data_unit, 1);
			break;
		}
	case F130://查询主节点rf发送功率
		{
			u8 rfpower;
			data_flash_read_straight_with_crc_noadc(STA_RF_POWER_MODE_ADDR,&rfpower,1,0xff);
			if(rfpower == 0xff)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0xff;
				pLmpAppInfo->responsePoint += 1;
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = HRF_LinePower;
				pLmpAppInfo->responsePoint += 1;
			}
			break;
		}
	case F171://设置从节点plc功率 新接口
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 14) //afn(1)+FN(2)+Data(11) 
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x61,&pLmpAppInfo->curProcUnit->data_unit[6]);
			break;
		}
	case F172://查询从节点plc功率 新接口
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 9) //afn(1)+FN(2)+Data(6)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x60,NULL);
			pLmpAppInfo->confirmFlag = 2;
			break;
		}
	case F173://设置从节点rf功率 新接口
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 12) //afn(1)+FN(2)+Data(9)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x63,&pLmpAppInfo->curProcUnit->data_unit[6]);
			break;
		}
	case F174://查询从节点rf功率 新接口
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 9) //afn(1)+FN(2)+Data(6)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x62,NULL);
			pLmpAppInfo->confirmFlag = 2;
			break;
		}
	case F213:  //查询主节点过零信号信息（生产）
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 3; //采集方式，双沿
			for(u8 i=0; i<6; i++)
			{
				memset(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0, 5);
				if(zero_check_pass[i])
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1; //采集到过零信号
					memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &zero_check_ntb[i], 4); //过零信号周期
					pLmpAppInfo->responsePoint += 4;
				}
				else
				{
					pLmpAppInfo->responsePoint += 5;
				}
			}
			break;
		}
	case F214:  //查询从节点内部版本信息（生产）
		{
			u8 sta_mac[6] = {NULL};

			if (pLmpAppInfo->appLayerLen != 9) //afn(1)+FN(2)+Data(6)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

			if (NULL == pSta)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
				return ERROR;
			}

			if (NET_IN_NET != pSta->net_state)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
				return ERROR;
			}
			
			u8* node_mac = (u8 *)alloc_buffer(__func__, MAC_ADDR_LEN);
			if(NULL == node_mac)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_TASK_MEMORY_NOT_ENOUGH;
				return ERROR;
			}
			memcpy_special(node_mac, sta_mac, MAC_ADDR_LEN);
			
			PLC_InnerVer(node_mac);

			pLmpAppInfo->confirmFlag = 2;

			break;
		}
	#ifndef __DEBUG_MODE
	case F215:  //查询主节点内部版本信息（生产）
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(SLG_COMMIT_DATE);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(SLG_COMMIT_MON);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(SLG_COMMIT_YEAR % 100);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = (SLG_COMMIT_HASH >> 12) & 0xff;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = (SLG_COMMIT_HASH >> 20) & 0xff;
			break;
		}
	#endif
	case F216:  //查询主节点内部CODE信息（升级flag、省份宏等）
		{
			#if defined(JIANGXI_APPLICATION)
				DefaultCodeInfo.AreaProtocol |= 1<<0;
			#endif
			#if defined(GDW_BEIJING)
				DefaultCodeInfo.AreaProtocol |= 1<<1;
			#endif
			#if defined(GDW_HUNAN)
				DefaultCodeInfo.AreaProtocol |= 1<<2;
			#endif
			#if defined(GDW_JILIN)
				DefaultCodeInfo.AreaProtocol |= 1<<3;
			#endif
			#if defined(GDW_SHAN3XI)
				DefaultCodeInfo.AreaProtocol |= 1<<4;
			#endif
			#if defined(GDW_CHONGQING)
				DefaultCodeInfo.AreaProtocol |= 1<<5;
			#endif
			#if defined(GDW_HENAN)
				DefaultCodeInfo.AreaProtocol |= 1<<6;
			#endif
			#if defined(GDW_SHANDONG)
				DefaultCodeInfo.AreaProtocol |= 1<<7;
			#endif
			#if defined(OVERSEA)
				DefaultCodeInfo.AreaProtocol |= 1<<8;
			#endif
			#if defined(DLMS_BROADCAST)
				DefaultCodeInfo.AreaProtocol |= 1<<9;
			#endif
			#if defined(GDW_NEIMENG)
				DefaultCodeInfo.AreaProtocol |= 1<<10;
			#endif
			#if defined(GDW_ZHEJIANG)
				DefaultCodeInfo.AreaProtocol |= 1<<11;
			#endif
			#if defined(GDW_HEILONGJIANG)
				DefaultCodeInfo.AreaProtocol |= 1<<12;
			#endif
			#if defined(GDW_SICHUAN)
				DefaultCodeInfo.AreaProtocol |= 1<<13;
			#endif
			#if defined(GDW_JIANGSU)
				DefaultCodeInfo.AreaProtocol |= 1<<14;
			#endif
			#if defined(GDW_SHENYANG)
				DefaultCodeInfo.AreaProtocol |= 1<<15;
			#endif
			#if defined(GDW_SHENYANG_READER)
				DefaultCodeInfo.AreaProtocol |= 1<<16;
			#endif
			#if defined(GDW_JIANGXI)
				DefaultCodeInfo.AreaProtocol |= 1<<17;
			#endif
			#if defined(PERFORMAINCE_OUT)
				DefaultCodeInfo.AreaProtocol |= 1<<30;
			#endif
			#if defined(GDW_TEST)
				DefaultCodeInfo.AreaProtocol |= (u32)1<<31;
			#endif

			memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &DefaultCodeInfo.Chip[0], sizeof(DefaultCodeInfo.Chip));
			pLmpAppInfo->responsePoint += sizeof(DefaultCodeInfo.Chip);
			memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &DefaultCodeInfo.Factory[0], sizeof(DefaultCodeInfo.Factory));
			pLmpAppInfo->responsePoint += sizeof(DefaultCodeInfo.Factory);
			memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8*)&DefaultCodeInfo.Factory_code, sizeof(DefaultCodeInfo.Factory_code));
			pLmpAppInfo->responsePoint += sizeof(DefaultCodeInfo.Factory_code);
			memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8*)&DefaultCodeInfo.AreaProtocol, sizeof(DefaultCodeInfo.AreaProtocol));
			pLmpAppInfo->responsePoint += sizeof(DefaultCodeInfo.AreaProtocol);
			
			u8 branch_len = strlen(SLG_BRANCH_NAME);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = branch_len;
			strcpy((char *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], SLG_BRANCH_NAME);
			pLmpAppInfo->responsePoint += branch_len;
			
			break;
		}
	case F218://查询节点离线和代理变更次数
		{
			*(u32*)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]=g_pRelayStatus->offline_time;
			pLmpAppInfo->responsePoint+=4;
			*(u32 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = g_pRelayStatus->pco_change;
			pLmpAppInfo->responsePoint+=4;
			break;
		}
	case F219://设置主节点plc发送功率
		{
			for (int i = 0; i < 4; i++)
			{
				if (pLmpAppInfo->curProcUnit->data_unit[i] == 0xff)
				{
					memset(pLmpAppInfo->curProcUnit->data_unit,0xff,4);
					break;
				}
				
#if (HPLC_MIN_POWER_VAL == 0)
				if (pLmpAppInfo->curProcUnit->data_unit[i] > HPLC_MAX_POWER_VAL)
#else
				if ((pLmpAppInfo->curProcUnit->data_unit[i] < HPLC_MIN_POWER_VAL) || (pLmpAppInfo->curProcUnit->data_unit[i] > HPLC_MAX_POWER_VAL))
#endif
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DT;
					return ERROR;
				}
			}
			
			data_flash_write_straight_with_crc(STA_POWER_MODE_ADDR, pLmpAppInfo->curProcUnit->data_unit, 4);
			break;
		}
	case F220://查询主节点plc发送功率
		{
			u8 power[4];
			data_flash_read_straight_with_crc_noadc(STA_POWER_MODE_ADDR,power,4,0xff);
			if(memIsHex(power, 0xff, 4))
			{
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], power, 4);
				pLmpAppInfo->responsePoint += 4;
			}
			else
			{
				memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], HPLC_ChlPower, sizeof(HPLC_ChlPower));
				pLmpAppInfo->responsePoint += 4;
			}
			
			break;
		}
	case F221://设置从节点plc功率
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 13) //afn(1)+FN(2)+Data(10)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x3F,&pLmpAppInfo->curProcUnit->data_unit[6]);
			break;
		}
	case F222://查询从节点plc功率
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 9) //afn(1)+FN(2)+Data(6)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x3E,NULL);
			pLmpAppInfo->confirmFlag = 2;
			break;
		}
#ifdef ASSOC_CNF_YINGXIAO		
	case F224://设置CCO开启认证方式
		{
			g_PlmConfigPara.YX_mode = pLmpAppInfo->curProcUnit->data_unit[0]==1?'Y':0;
			SYS_STOR_AUTHEN();

            //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
            SYS_READ_AUTHEN();
			break;
		}
	case F225://查询CCO认证方式
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]=g_PlmConfigPara.YX_mode=='Y'?1:0;
			break;
		}
#endif		
	case F228://查询CCO的IO状态
		{
			if (pLmpAppInfo->appLayerLen != 3) //afn(1)+FN(2)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			//IO字段，和sta复用。第1字节STA Event，第2字节STA Insert，第3字节CCO Reset，其余保留			
			memset(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0, 6);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+2] = ResetRead();
			pLmpAppInfo->responsePoint += 6;
						
			break;
		}
	case F229://设置从节点rf功率
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 11) //afn(1)+FN(2)+Data(8)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x31,&pLmpAppInfo->curProcUnit->data_unit[6]);
			break;
		}
	case F230://查询从节点rf功率
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 9) //afn(1)+FN(2)+Data(6)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendStaPower(sta_mac,0x30,NULL);
			pLmpAppInfo->confirmFlag = 2;
			break;
		}
	case F231://查询主节点寄存器值
		{
			u32 addr = *(u32 *)&pLmpAppInfo->curProcUnit->data_unit[0];
			u16 len = *(u32 *)&pLmpAppInfo->curProcUnit->data_unit[4];
			
			if(len > MSA_MESSAGAE_MAX_SIZE || pLmpAppInfo->appLayerLen != 9) //afn(1)+FN(2)+Data(6)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)&len, 2);
			pLmpAppInfo->responsePoint += 2;
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)addr, len);
			pLmpAppInfo->responsePoint += len;
			break;
		}
	case F232://查询从节点寄存器值
		{
			u8 sta_mac[6];

			if (pLmpAppInfo->appLayerLen != 15) //afn(1)+FN(2)+Data(12)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			memcpy(sta_mac, &pLmpAppInfo->curProcUnit->data_unit[0], 6);
			if (!memIsHex(sta_mac,0xff,6))
			{
				P_STA_INFO pSta = PLC_GetStaInfoByMac(sta_mac);

				if (NULL == pSta)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}

				if (NET_IN_NET != pSta->net_state)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_NODE_NOT_IN_NET;
					return ERROR;
				}
				
			}
			
			PLC_SendRegisterRead(sta_mac,&pLmpAppInfo->curProcUnit->data_unit[6], 6);
			pLmpAppInfo->confirmFlag = 2;
			break;
		}
#ifndef HPLC_CSG  //只有国网版本支持
	case F233://重新生成网络ID
		{
			if (pLmpAppInfo->appLayerLen != 3) //afn(1)+FN(2)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			HPLC_ChangeNID();

			break;
		}
	case F234://查询网络ID
		{
			if (pLmpAppInfo->appLayerLen != 3) //afn(1)+FN(2)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			
			memcpy(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &g_pPlmConfigPara->net_id, sizeof(g_pPlmConfigPara->net_id));
			pLmpAppInfo->responsePoint += sizeof(g_pPlmConfigPara->net_id);

			break;
		}
#endif

	case F235:
		{
			if (pLmpAppInfo->appLayerLen != 7) //afn(1)+FN(2)+Data(4)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			u16 addr_start = ntohs(*(u16 *)&pLmpAppInfo->curProcUnit->data_unit[0]); //2字节大端传输
			u16 addr_end = ntohs(*(u16 *)&pLmpAppInfo->curProcUnit->data_unit[2]); //2字节大端传输

			if (addr_start > addr_end)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			
			g_PlmConfigPara.relay_addr_start = addr_start;
			g_PlmConfigPara.relay_addr_end = addr_end;
			SYS_STOR_RELAY_RANGE();
			
            //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
            SYS_READ_RELAY_RANGE();
			
			break;
		}
	case F236:
		{
			if (pLmpAppInfo->appLayerLen != 3) //afn(1)+FN(2)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

            //2字节大端传输
			memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)&g_pPlmConfigPara->relay_addr_start, sizeof(g_pPlmConfigPara->relay_addr_start));
			pLmpAppInfo->responsePoint += sizeof(g_pPlmConfigPara->relay_addr_start);
                        
            //2字节大端传输
			memcpy_swap(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], (u8 *)&g_pPlmConfigPara->relay_addr_end, sizeof(g_pPlmConfigPara->relay_addr_end));
			pLmpAppInfo->responsePoint += sizeof(g_pPlmConfigPara->relay_addr_end);
			
			break;
		}
	case F241://查询接收抄控器RSSI
		{
		    if (!g_pRelayStatus->connectReader)
    		{
    			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_CMD_NOT_SUPPORT;
				return ERROR;
    		}
            
			if (pLmpAppInfo->appLayerLen != 3&&pLmpAppInfo->appLayerLen != 4) //afn(1)+FN(2)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			if (pLmpAppInfo->appLayerLen==4)
			{
				ReaderPhase=pLmpAppInfo->curProcUnit->data_unit[0];
			}

            *(s32 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = g_pRelayStatus->recv_rssi;
            pLmpAppInfo->responsePoint += 4;

			break;
		}	
	case F242://查询是否允许采集器模块自身地址入网（不在档案中）的开关
        {
            if (pLmpAppInfo->appLayerLen != 3) //afn(1)+FN(2)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = g_pPlmConfigPara->allow_collect_innet;
            return OK;
        }
    case F243://设置是否允许采集器模块自身地址入网（不在档案中）的开关
        {
            if (pLmpAppInfo->appLayerLen != 4) //afn(1)+FN(2)+Data(1)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

            u8 mode = (pLmpAppInfo->curProcUnit->data_unit[0]==1)?1:0;

            if (mode != g_pPlmConfigPara->allow_collect_innet)
            {
                g_PlmConfigPara.allow_collect_innet = mode;
				SYS_STOR_COLLECT_INNET_MODE(mode);

                //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
                SYS_READ_COLLECT_INNET_MODE();
            }

            return OK;
        }
	case F245://查询cco频偏
		{
			if (pLmpAppInfo->appLayerLen != 3) //afn(1)+FN(2)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}
			if (g_PlmConfigPara.PLL_Vaild==1&&g_PlmConfigPara.PLL_Trimming==0)
			{
				*(s16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]=1;
			}
			else
			{
				*(s16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]=g_PlmConfigPara.PLL_Trimming*10/16;
				if (*(s16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]==0)
				{
					*(s16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]=1;
				}
			}
			pLmpAppInfo->responsePoint+=2;
			return OK;
		}
	case F246:       
		{
			//芯片 ID 长度为 24 字节；模块 ID 长度为 11 字节；
			//ID 信息：24 位芯片 ID 的格式定义同 AFN10H-F112 中的芯片 ID 信息格式定义；模块 ID 的具体定义在其它文档中明确。

			u8 dev_type = pLmpAppInfo->curProcUnit->data_unit[0];
			u8 *pMacAddr = (u8 *)&pLmpAppInfo->curProcUnit->data_unit[1];
			u8 id_type = pLmpAppInfo->curProcUnit->data_unit[7];

			if (2 == dev_type)     //1 抄控器	 2 集中器本地通信单元  3 电表通信单元	 4 中继器  5 II型采集器  6 I型采集器单元  7 三相电表通信单元
			{
				if (0x01 == id_type)       //芯片ID
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    //设备类型
					//设备地址
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pMacAddr, 6);
					pLmpAppInfo->responsePoint += 6;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = id_type;    //ID 类型
					pLmpAppInfo->responsePoint+=3;
					*(u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = sizeof(g_cco_id.chip_id);      //ID 长度
					//ID 信息
					//V2.7要求“芯片ID”倒序传输。CCO录入“芯片ID”时是倒序录入，所以不再需要转换字节序
					P_STA_CHIP_ID_INFO pChipID = (P_STA_CHIP_ID_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
					memcpy_special(pChipID, g_cco_id.chip_id, sizeof(g_cco_id.chip_id));

					pLmpAppInfo->responsePoint += sizeof(g_cco_id.chip_id);
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
					return ERROR;
				}
			}
			else if (3 == dev_type)  
			{
				if (0x01 == id_type) 
				{
				    u8 dst_addr[LONG_ADDR_LEN] = {0};
                    PLC_GetAppDstMac(pMacAddr, dst_addr);
        
					P_STA_INFO pSta = PLC_GetStaInfoByMac(dst_addr);
					if(pSta != NULL)
					{
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    //设备类型
						memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pMacAddr, 6);
						pLmpAppInfo->responsePoint += 6;
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = id_type;    //ID 类型
						pLmpAppInfo->responsePoint += 3;
						*(u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = sizeof(g_cco_id.chip_id);      //ID 长度
						
						//ID 信息
                        if ((memIsHex(pSta->check_sum, 0xff, sizeof(pSta->check_sum)))||
                            (memIsHex(pSta->check_sum, 0x00, sizeof(pSta->check_sum))))
                        {
                            //未录入芯片ID，全F返回处理
                            memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0xFF, sizeof(g_cco_id.chip_id));
                        }
                        else
                        {
    						//V2.7要求“芯片ID”倒序传输。
							P_STA_CHIP_ID_INFO pChipID = (P_STA_CHIP_ID_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
    						
							pChipID->fix_code1 = 0x01;
							pChipID->fix_code2 = 0x02;
							pChipID->fix_code3 = 0x9c;
							pChipID->fix_code4[0] = 0xfb;
							pChipID->fix_code4[1] = 0xc1;
							pChipID->fix_code4[2] = 0x01;
							//pChipID->dev_class = pSta->moduleType;

	//						if (pInfo)
							{
								memcpy_special(&pChipID->dev_class, &pSta->dev_class, sizeof(pSta->dev_class));
								memcpy_special(pChipID->factory_code, pSta->factory_code, sizeof(pSta->factory_code));
								memcpy_special(pChipID->chip_code, pSta->chip_code, sizeof(pSta->chip_code));
								memcpy_special(pChipID->dev_seq, pSta->dev_seq, sizeof(pSta->dev_seq));
								memcpy_special(pChipID->check_sum, pSta->check_sum, sizeof(pSta->check_sum));
							}
                        }
					
						pLmpAppInfo->responsePoint += sizeof(g_cco_id.chip_id);
					}
					else
					{
						pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
						return ERROR;
					}
				}
				else
				{

					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
					return ERROR;
				}
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			break;
		}
	case F247:     
		{
			//芯片 ID 长度为 24 字节；模块 ID 长度为 11 字节；
			//ID 信息：24 位芯片 ID 的格式定义同 AFN10H-F112 中的芯片 ID 信息格式定义；模块 ID 的具体定义在其它文档中明确。

			u8 dev_type = pLmpAppInfo->curProcUnit->data_unit[0];
			u8 *pMacAddr = (u8 *)&pLmpAppInfo->curProcUnit->data_unit[1];
			u8 id_type = pLmpAppInfo->curProcUnit->data_unit[7];
			u16 id_len = ntohs(*(u16 *)&pLmpAppInfo->curProcUnit->data_unit[8]);
			if (2 == dev_type)     //1 抄控器	 2 集中器本地通信单元  3 电表通信单元	 4 中继器  5 II型采集器  6 I型采集器单元  7 三相电表通信单元
			{
				if (0x01 == id_type && id_len == sizeof(g_cco_id.chip_id))       //芯片ID
				{
					memcpy(g_cco_id.chip_id,&pLmpAppInfo->curProcUnit->data_unit[10],id_len);
					SYS_STOR_CCO_ID();
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = dev_type;    //设备类型
					//设备地址
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pMacAddr, 6);
					pLmpAppInfo->responsePoint += 6;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = id_type;    //ID 类型
					pLmpAppInfo->responsePoint += 3;
					*(u16 *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = sizeof(g_cco_id.chip_id);      //ID 长度
					//ID 信息
					P_STA_CHIP_ID_INFO pChipID = (P_STA_CHIP_ID_INFO)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint];
					memcpy_special(pChipID, g_cco_id.chip_id, sizeof(g_cco_id.chip_id));


					pLmpAppInfo->responsePoint += sizeof(g_cco_id.chip_id);
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
					return ERROR;
				}
			}
			else
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			break;
		}

	case F251://固定增益（产测）
		{
            hrf_agc_factory_set(pLmpAppInfo->curProcUnit->data_unit[0]);
			return OK;
		} 

	case F133://设置debug_mask
		{
#ifndef DEBUG_UART
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_CMD_NOT_SUPPORT;
			return ERROR;
#endif
			if (pLmpAppInfo->appLayerLen != (3+DebugGetMaskLen())) //afn(1)+FN(2)+Data(4)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			DebugSetMask(*(u32 *)(&pLmpAppInfo->curProcUnit->data_unit[0]));
			//存FLASH
			SYS_STOR_DEBUGMASK(*(u32 *)(&pLmpAppInfo->curProcUnit->data_unit[0]));

			return OK;
		}
	case F134://查询debug_mask
		{
#ifndef DEBUG_UART
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_CMD_NOT_SUPPORT;
			return ERROR;
#endif
			if (pLmpAppInfo->appLayerLen != 3) //afn(1)+FN(2)+Data(0)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
				return ERROR;
			}

			*(u32 *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]) = DebugGetMask();
			pLmpAppInfo->responsePoint += DebugGetMaskLen();
			*(u32 *)(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]) = DebugGetMaskLater();
			pLmpAppInfo->responsePoint += DebugGetMaskLen();
			
			return OK;
		}
    case F100://设置和查询需要比对的从节点内部版本信息
        {
            if (pLmpAppInfo->appLayerLen == 3) //afn(1)+FN(2)
            {
                memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &g_PlmConfigPara.sta_inner_ver, 6);
                pLmpAppInfo->responsePoint += 2;

                return OK;
            }
            else if (pLmpAppInfo->appLayerLen == 5) //afn(1)+FN(2)+Data(2)
            {
                memcpy_special((u8 *)&g_PlmConfigPara.sta_inner_ver, &pLmpAppInfo->curProcUnit->data_unit[0], 2);
                SYS_STOR_COMPARE_STA_INVER();

                //存储后重新从Flash读取，方便查询命令验证是否真正写入成功
                SYS_READ_COMPARE_STA_INVER();
                
                return OK;
            }

            pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
            return ERROR;
		}
    case F101://批量查询从节点比对内部版本信息结果
        {
            if (pLmpAppInfo->appLayerLen != 6) //afn(1)+FN(2)+Data(从节点起始序号(2)+从节点数量(1))
            {
                pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
                return ERROR;
            }
            
    		BYTE node_count = 0, real_num = 0;
    		USHORT start_sn = 0;
    		USHORT presp = pLmpAppInfo->responsePoint;
    		USHORT ii = 0;
    		P_STA_INFO pSta = NULL;
            P_STA_STATUS_INFO pStaStatus = NULL;
    	   
    		memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2); //从节点起始序号
    		if (start_sn == 0) //从节点序号从1开始
    		{   
    			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_DATA_PAYLOAD;
                return ERROR;
    		}
            
    		node_count = pLmpAppInfo->curProcUnit->data_unit[2]; //本次查询的从节点数量

    		PLC_get_relay_info();
    		u16 total_num = g_pRelayStatus->cfged_meter_num; //从节点总数量
    		
    		memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &total_num, 2);
    		pLmpAppInfo->responsePoint += 2;

    		pLmpAppInfo->responsePoint++;

			for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
			{
				if (real_num == node_count)
					break;
		 
				if (!METER_IS_CFG(ii))
					continue;

				if (start_sn > 1) //从节点序号从1开始
				{
					start_sn--;
					continue;
				}
		 
				real_num++;

				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;
		 
				u8 dst_addr[LONG_ADDR_LEN] = {0};
                PLC_GetAppDstMac(g_MeterRelayInfo[ii].meter_addr, dst_addr);

                u16 tei = PLC_GetStaSnByMac(g_MeterRelayInfo[ii].meter_addr);
                pSta = PLC_GetValidStaInfoByTei(tei);
                if (pSta != NULL)
				{     
				    
					//厂商代码
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_factory_code, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2); 
					pLmpAppInfo->responsePoint += 2;

					//芯片代码
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->ver_chip_code, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2); 
					pLmpAppInfo->responsePoint += 2;

                    //版本时间
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Day);
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Month);
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = Hex2BcdChar(pSta->year_mon_data.Year);

					//版本号
					memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pSta->software_v, 2);
					TransposeByteOrder((UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 2); 
					pLmpAppInfo->responsePoint += 2;

                    pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0;
                    pStaStatus = PLC_GetStaStatusInfoByTei(tei);
                    if (pStaStatus != NULL)
    				{   
                        pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = pStaStatus->res; //内部版本比对结果
                    }
                    pLmpAppInfo->responsePoint += 1;
				}
                else
                {
                    memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0x00, 10);
                    pLmpAppInfo->responsePoint += 10;
                }
			}

			memcpy_special(pLmpAppInfo->msg_buffer + presp + 2, &real_num, 1);

			return OK;
		}
#ifdef GDW_PEROID_ID_MESSAGE_SET_CAPTURE		
	case F102://设置ID信息采集周期，并启动采集ID信息
		{
			u32 period = pLmpAppInfo->curProcUnit->data_unit[0];
			u8 unit = pLmpAppInfo->curProcUnit->data_unit[1];

			if(unit == 2) //单位小时
			{
				period = period*60*60; //转成秒
			}
			else //1-单位分钟
			{
				period = period*60; //转成秒
			}

			if(period != g_PlmStatusPara.id_set_period)
			{
				g_PlmStatusPara.id_set_period = period;

				//周期性请求定时器
				OS_TICK_64 now= GetTickCount64()/1000;
				period = now + period;
			
				IDMsgReqeustTimerId = HPLC_RegisterTmr(ReadModuleIDMsg, NULL, period*OSCfg_TickRate_Hz, TMR_OPT_CALL_PERIOD);
			}
			break;
		}
#endif	

	case F104:
		{
			UCHAR node_count = 0;
			UCHAR *real_num;
			USHORT start_sn = 0;
			USHORT total_num = g_pRelayStatus->cfged_meter_num + 1;;
			USHORT ii = 0;
			P_STA_INFO pSta = NULL;
			
			memcpy_special(&start_sn, &pLmpAppInfo->curProcUnit->data_unit[0], 2); //下行节点起始序号
			node_count = pLmpAppInfo->curProcUnit->data_unit[2]; //下行节点数量

			//上行节点总数量
			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, &total_num, 2);
			pLmpAppInfo->responsePoint += 2;

			//上行本次应答的节点数量
			real_num = (UCHAR *)&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++]; 
			*real_num = 0;
			
			if (start_sn == 0) //CCO
			{   
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_PlmConfigPara.main_node_addr, 6);
				pLmpAppInfo->responsePoint += 6;

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = (SLG_COMMIT_HASH >> 12) & 0xff;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = (SLG_COMMIT_HASH >> 20) & 0xff;
		  
				(*real_num)++;
			}

			for (ii = 0; ii <  CCTT_METER_NUMBER; ii++)
			{
				if ((*real_num) == node_count)
					break;
		 
				if (!METER_IS_CFG(ii))
					continue;

				if (start_sn > 1) //从节点序号从1开始
				{
					start_sn--;
					continue;
				}
		 
				(*real_num)++;
				
				memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint, g_MeterRelayInfo[ii].meter_addr, 6);
				pLmpAppInfo->responsePoint += 6;
				
				u8 dst_addr[LONG_ADDR_LEN] = {0};
				PLC_GetAppDstMac(g_MeterRelayInfo[ii].meter_addr, dst_addr);
				pSta = PLC_GetStaInfoByMac(dst_addr);
				//版本号小端序
				if (pSta != NULL)
				{					
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->inner_v[0];
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = pSta->inner_v[1];
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;
				}
			}
		}
		break;	
	default:
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
			pLmpAppInfo->responsePoint++;
			return ERROR;
		}
	}
	return OK;
}
/*  F1: 加载文件 */
unsigned char afn_F6_F1_proc(P_LMP_APP_INFO pLmpAppInfo)
{
#if 1
	//私有协议升级，废弃
	return ERROR;
#else
	CFG_FILE_TRANS_PARA *  pFilePara = ((CFG_FILE_TRANS_PARA *  )(pLmpAppInfo->curProcUnit->data_unit));
	CFG_FILE_BEGIN_PARA *  pBeginPara = (CFG_FILE_BEGIN_PARA *  )(pFilePara);
	unsigned char * chk_res;

	pLmpAppInfo->confirmFlag = 0;

	if(pFilePara->fileName != UPDATE_FIRMWARE_NAME)
	return ERROR;

	if(pFilePara->fileAttr != UPDATE_FIRMWARE_ATTR)
	return ERROR;

	//memcpy(tmp, pLmpAppInfo->curFnData, 32);

	switch(pFilePara->fileCmd)
	{
		case UPDATE_FILE_TRANS_BEGIN:
		{

			//CFG_FILE_BEGIN_PARA *  pBeginPara = (CFG_FILE_BEGIN_PARA *  )(pFilePara);

			//pLmpAppInfo->curFnLen = sizeof(CFG_FILE_BEGIN_PARA);

			memcpy_special((pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint), pBeginPara, sizeof(CFG_FILE_BEGIN_PARA) );
			pBeginPara = (CFG_FILE_BEGIN_PARA * )(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);

			pLmpAppInfo->responsePoint += sizeof(CFG_FILE_BEGIN_PARA);


			if(((unsigned long)pBeginPara->segmentNum) * pBeginPara->segmentLen > UPDATA_MAX_FILE_LENGTH)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = OK+1;
				pLmpAppInfo->responsePoint++;
				return ERROR;
			}

			if(pBeginPara->segmentNum > UPDATE_FILE_MAP_SIZE * 8)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = OK+2;
				pLmpAppInfo->responsePoint++;
				return ERROR;
			}

			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = OK;
			pLmpAppInfo->responsePoint++;

			memset_special(g_file_trans_map, 0 , (UPDATE_FILE_MAP_SIZE));
			memset_special(g_updata_firmware_chk, 0 , 2);

			//memset(g_updata_firmware_cnt, 0 , sizeof(g_updata_firmware_cnt) );

			break;
		}
		case UPDATE_FILE_TRANS_WRITE:

		PLC_stage_pause();
		//g_updata_firmware_cnt[1] ++;

		if(pFilePara->segmentOffset >= pFilePara->segmentNum)
		return ERROR;

		//pLmpAppInfo->curFnLen = pFilePara->transLen + sizeof(CFG_FILE_TRANS_PARA) - 1;

		if(pFilePara->segmentNum && pFilePara->segmentLen && pFilePara->transLen )
		{
			//判断此前是否收到过该段数据
			if(!(g_file_trans_map[pFilePara->segmentOffset >> 3] & (0x01 << (pFilePara->segmentOffset & 0x07))))
			{
				unsigned short ii;
				unsigned long wr_addr;

				wr_addr = UPDATE_FIRMWARE_DB_ADDR + ((unsigned long)(pFilePara->segmentOffset))*(pFilePara->segmentLen);

				data_flash_write_straight(wr_addr, pFilePara->dataBuf, pFilePara->transLen);
				data_flash_read_straight(wr_addr, pFilePara->dataBuf, pFilePara->transLen);
				for(ii = 0; ii < pFilePara->transLen; ii++)
				{
					g_updata_firmware_chk[0] += pFilePara->dataBuf[ii];
					g_updata_firmware_chk[1] ^= pFilePara->dataBuf[ii];
				}
				g_file_trans_map[pFilePara->segmentOffset >> 3] |= (UCHAR)(0x01 << (pFilePara->segmentOffset & 0x07));

				//g_updata_firmware_cnt[3] = pFilePara->segmentOffset;

				//g_updata_firmware_cnt[2] ++;

			}
		}

		pLmpAppInfo->confirmFlag = 0xff;

		break;
		case UPDATE_FILE_TRANS_READ:
		{


			//pLmpAppInfo->curFnLen = sizeof(CFG_FILE_BEGIN_PARA);

			memcpy_special((pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint), pBeginPara, sizeof(CFG_FILE_BEGIN_PARA) );
			pBeginPara = (CFG_FILE_BEGIN_PARA * )(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);

			if(((unsigned long)pBeginPara->segmentNum) * pBeginPara->segmentLen > UPDATA_MAX_FILE_LENGTH)
			{
				pLmpAppInfo->responsePoint += sizeof(CFG_FILE_BEGIN_PARA);

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = OK+1;
				pLmpAppInfo->responsePoint++;
				return ERROR;
			}

			if(pBeginPara->segmentNum > UPDATE_FILE_MAP_SIZE * 8)
			{
				pLmpAppInfo->responsePoint += sizeof(CFG_FILE_BEGIN_PARA);

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = OK+2;
				pLmpAppInfo->responsePoint++;
				return ERROR;
			}

			pLmpAppInfo->responsePoint += sizeof(CFG_FILE_BEGIN_PARA);

			//memcpy((pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint), pBeginPara, sizeof(CFG_FILE_BEGIN_PARA) );
			//pBeginPara = (CFG_FILE_BEGIN_PARA * )(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);
			pBeginPara->segmentLen = (pBeginPara->segmentNum >> 3);  //保存后面写的map表的数目
			if(pBeginPara->segmentNum & 0x07)
			pBeginPara->segmentLen += 1;

			//pLmpAppInfo->responsePoint += sizeof(CFG_FILE_BEGIN_PARA);
			memcpy_special((pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint), g_file_trans_map, pBeginPara->segmentLen );
			pLmpAppInfo->responsePoint += pBeginPara->segmentLen;

			break;
		}
		case UPDATE_FILE_TRANS_CHK:
		{
			chk_res = (unsigned char * )(pBeginPara + 1);

			memcpy_special((pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint), pBeginPara, sizeof(CFG_FILE_BEGIN_PARA) );
			pBeginPara = (CFG_FILE_BEGIN_PARA * )(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);

			pLmpAppInfo->responsePoint += sizeof(CFG_FILE_BEGIN_PARA);

			if(  ((chk_res[0]) != g_updata_firmware_chk[0])
			   || ((chk_res[1]) != g_updata_firmware_chk[1]) )
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = OK+1;
				pLmpAppInfo->responsePoint++;
				return ERROR;
			}

			chk_res[0] = LOAD_APP_REQ;
			data_flash_write_straight(ADDR_LOAD_BOOT_FLAG, chk_res, 1);

			if(chk_res[2] == 1)
			{
				Reboot_system(2, 0);
			}

			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = OK;
			pLmpAppInfo->responsePoint++;

			break;
		}
		default:
		return ERROR;
	}

	return OK;
#endif
}


//厂家保留，兼容，主要用于升级（AFN=FFH）
unsigned char afn_factory_reserved(P_LMP_APP_INFO pLmpAppInfo)
{
	switch (pLmpAppInfo->curDT)
	{
	case F1:  //F1：重启
		{
			afn_F6_F1_proc(pLmpAppInfo);
			break;
		}
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}

	return OK;

}

unsigned char afn_func_F1(P_LMP_APP_INFO pLmpAppInfo)
{
	switch (pLmpAppInfo->curDT)
	{
	case F1:        //并发抄表
#ifdef GDW_2019_GRAPH_STOREDATA	
	case F100:      //并发抄表曲线存储数据
#endif	
		{
			u8 *pDesAddr = NULL;
			P_GD_DL645_BODY pDL645;
			P_DL69845_BODY pDL698;
			P_PARALLEL_READ_METER pReadMeter = (P_PARALLEL_READ_METER)pLmpAppInfo->curProcUnit->data_unit;
			if ((pLmpAppInfo->appLayerLen-7)>2000)//7:afn(1) FN(2) Data head(4)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_PARALLEL_OVER_MAX_STA_PACKET;
				return ERROR;
			}
			if (pLmpAppInfo->lmpLinkHead->Inf.down_info.comm_module)
			{
				u8 level = pLmpAppInfo->lmpLinkHead->Inf.down_info.relay_level;
				pDesAddr = &pLmpAppInfo->lmpLinkHead->user_data[6 + 6 * level];
			}
			if (NULL == pDesAddr)
			{
				pDL645 = PLC_check_dl645_frame(pReadMeter->data, pReadMeter->len);
				pDL698 =  PLC_check_69845_frame(pReadMeter->data, pReadMeter->len);
				if (pDL645)
				{
					pDesAddr = pDL645->meter_number;
				}
				else if (pDL698)
				{
					pDesAddr = PLC_get_69845_SAMac(pDL698, NULL);
				}
				else
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
					return ERROR;
				}

			}

//			if (g_pRelayStatus->cfged_meter_num!=g_pRelayStatus->online_num)
//			{
//				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_BUSY_NOW;
//				return ERROR;
//			}
				
			if (pReadMeter->protocol==1||pReadMeter->protocol==2)//645
			{
				int max_palralle=0;
				pDL645 = PLC_check_dl645_frame(pReadMeter->data, pReadMeter->len);
				s16 remain=pReadMeter->len;
				u8 head_tail_645 = sizeof(GD_DL645_BODY) - SEND_DL645_DATA_BUF_SIZE + 2; //645帧帧头+帧尾长度 
				while (pDL645)
				{
					++max_palralle;
					remain =  pReadMeter->len-((u32)pDL645-(u32)pReadMeter->data + pDL645->dataLen + head_tail_645);
					if (remain<0)
					{
						remain=0;
					}
					pDL645 = (P_GD_DL645_BODY)((u32)pDL645 + pDL645->dataLen + head_tail_645);
					pDL645 = PLC_check_dl645_frame((u8*)pDL645, remain);
				}
				if (max_palralle>13)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_PARALLEL_OVER_MAX_STA_PACKET;
					return ERROR;
				}
				else if (max_palralle==0)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
					return ERROR;
				}
			}
			else if (pReadMeter->protocol==3)//698
			{
				pDL698 =  PLC_check_69845_frame(pReadMeter->data, pReadMeter->len);

				if (pDL698==NULL)
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
					return ERROR;
				}
				/* 防止698 前面有前导*/
				if((pDL698->len+((u32)pDL698-(u32)pReadMeter->data))>(pReadMeter->len-2))
				{
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_PARALLEL_OVER_MAX_STA_PACKET;
					return ERROR;
				}
				
			}

			P_READ_MSG_INFO pReadMsg = (P_READ_MSG_INFO)alloc_buffer(__func__,sizeof(READ_MSG_HEAD) + pReadMeter->len);

			if (NULL == pReadMsg)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = RT_ERR_INVALID_FORMAT;
				return ERROR;
			}

			pReadMsg->header.broadcast = 0;
			pReadMsg->header.read_count = 3;
			pReadMsg->header.read_time = 0;
			pReadMsg->header.sour_forme = pLmpAppInfo->end_id;
			pReadMsg->header.packet_seq_3762 = pLmpAppInfo->lmpLinkHead->Inf.down_info.seq_no;
			memcpy_special(pReadMsg->header.mac, pDesAddr, MAC_ADDR_LEN);
			pReadMsg->header.meter_protocol_plc = PLC_3762protocol_to_plcprotocol(pReadMeter->protocol);
#ifdef GDW_2019_GRAPH_STOREDATA
			if(pLmpAppInfo->curDT == F100)
			{
				pReadMsg->header.packet_id = APP_PACKET_STORE_DATA_PARALLEL;
			}
			else
#endif				
			{
				pReadMsg->header.packet_id = APP_PACKET_PARALLEL;
			}
			
			pReadMsg->header.time_related = 0;
			pReadMsg->header.sender = EM_SENDER_AFN_GDW2013;
			pReadMsg->header.msg_len = pReadMeter->len;
			pReadMsg->header.ts_send = 0;
			pReadMsg->header.ts_timeout = 0;
			memcpy_special(pReadMsg->body, pReadMeter->data, pReadMeter->len);

			u8 res = PLC_PostParallelRead(pReadMsg, 1);
			if (res != RT_ERR_NONE)
			{
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = res;
				return ERROR;
			}

			pLmpAppInfo->confirmFlag = 2;

			break;
		}
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}

	return OK;
}

u32 s_file_addr_start = 0;


u32 s_code_checksum = 0;
u8  s_last_left_bytes = 0;
u32 s_dword = 0;
u32 s_user_addr = 0;
u32 s_new_version = 0;
P_FLASH_USER_CHARACTER_FIELD s_pUser = NULL;

#pragma section = ".vectors"
extern char _evectors;
extern char _svectors;
//文件传输, 仅支持测试
unsigned char afn_15_file_trans(P_LMP_APP_INFO pLmpAppInfo)
{
#define AFN_15_F1_ERR 0xffffffff

	switch (pLmpAppInfo->curDT)
	{
	case F1:
		{
			P_CFG_FILE_TRANS_PARA_GDW13 pFileParam = (P_CFG_FILE_TRANS_PARA_GDW13)pLmpAppInfo->curProcUnit->data_unit;
			ULONG err_code = pFileParam->segmentOffset;

			if (0x00 == pFileParam->fileName || (pFileParam->fileAttr == 00 && pFileParam->segmentOffset==0 && s_pUser==NULL))      //00H：清除下装文件；开始升级
			{
				g_pRelayStatus->sta_update_cb.update_flag=0;
				memset_special(g_file_trans_map, 0, (UPDATE_FILE_MAP_SIZE));
				memset_special(g_updata_firmware_chk, 0, 2);
				s_code_checksum = 0;
				if (s_pUser != NULL)
				{
					free_buffer(__func__,s_pUser);
				}
				SYS_STOR_GetUpdateUserInfo(NULL, &s_pUser, (unsigned long *)&s_user_addr, &s_new_version);
				g_PlmStatusPara.file_addr = s_pUser->code_addr;
				s_pUser->code_size = 0xffffffff;
				s_pUser->character = 0xffffffff;
				Unprotection1MFlash();
				data_flash_write_straight(s_user_addr, (u8 *)s_pUser, sizeof(FLASH_USER_CHARACTER_FIELD));
				Protection1MFlash();
				s_pUser->character = 0x0123abcd;
				s_file_addr_start = g_PlmStatusPara.file_addr;
				g_update_crc32[0] = 0xFFFFFFFF;
				g_update_crc32[1] = 0xFFFFFFFF;
				PLC_StopUpdateSta();
			}
			if (s_pUser==NULL)
			{
				err_code = AFN_15_F1_ERR;
			}
			else if (0x03 == pFileParam->fileName) //03H：本地通信模块升级文件；
			{
				g_pRelayStatus->sta_update_cb.update_flag=1;
				if (0x01 == pFileParam->fileAttr)      //结束帧为 01H；
				{
					if (pFileParam->segmentLen >= 2)     //bug: 如果最后一段只有1字节，则升级不成功
					{
                        if (pFileParam->segmentLen == 2) //最后一帧只有2字节校验
                        {
	                        pFileParam->segmentOffset -= 1;
	                        err_code = pFileParam->segmentOffset;
                        }
                                            
					    pFileParam->segmentLen -= 2;   
					}
				}

				if (pFileParam->segmentNum && pFileParam->segmentLen)
				{
					//判断此前是否收到过该段数据
					if (!(g_file_trans_map[pFileParam->segmentOffset >> 3] & (0x01 << (pFileParam->segmentOffset & 0x07))))
					{
						unsigned short ii;
						Unprotection1MFlash();
						data_flash_write_straight(g_PlmStatusPara.file_addr, pFileParam->dataBuf, pFileParam->segmentLen);
						Protection1MFlash();
						memset_special(pFileParam->dataBuf, 0xff, pFileParam->segmentLen);
						data_flash_read_straight(g_PlmStatusPara.file_addr, pFileParam->dataBuf, pFileParam->segmentLen);
						g_PlmStatusPara.file_addr += pFileParam->segmentLen;

						for (ii = 0; ii < pFileParam->segmentLen; ii++)
						{
							g_updata_firmware_chk[0] += pFileParam->dataBuf[ii];
							g_updata_firmware_chk[1] ^= pFileParam->dataBuf[ii];
						}
						g_file_trans_map[pFileParam->segmentOffset >> 3] |= (UCHAR)(0x01 << (pFileParam->segmentOffset & 0x07));

						ii = 0;
						for (ii = 0; ii < pFileParam->segmentLen;)
						{
							u8 cpy_size = (4 - s_last_left_bytes);
							memcpy_special(((u8 *)&s_dword) + s_last_left_bytes, &pFileParam->dataBuf[ii], cpy_size);
							s_code_checksum += s_dword;
							s_dword = 0;
							s_last_left_bytes = 0;
							ii += cpy_size;

							if ((pFileParam->segmentLen - ii) < 4)
							{
								s_last_left_bytes = (pFileParam->segmentLen - ii);
								memcpy_special(((u8 *)&s_dword), &pFileParam->dataBuf[ii], s_last_left_bytes);
								break;
							}
						}

					}
				}

				if (0x00 == pFileParam->fileAttr)  //起始帧、中间帧为 00H；
				{
					//可以校验文件大小是否溢出
				}
				else if (0x01 == pFileParam->fileAttr)     //结束帧为 01H；
				{
					g_pRelayStatus->sta_update_cb.update_flag=0;
					//最后一段，检查自带校验，正确则升级
					for (u16 i = 0; i <= pFileParam->segmentOffset; i++)
					{
						if (!(g_file_trans_map[i >> 3] & (0x01 << (i & 0x07))))
						{
							err_code = AFN_15_F1_ERR;
							break;
						}
					}

					if (err_code == pFileParam->segmentOffset)						
					{
						u32 ownCode;
						#if defined(__ICCARM__)
						data_flash_read_straight((s_pUser->code_addr+__section_size(".vectors")-4), (u8*)&ownCode, sizeof(ownCode));
						#elif defined(__GNUC__)
						data_flash_read_straight(s_pUser->code_addr+(u32)(&_evectors-&_svectors)-4, (u8*)&ownCode, sizeof(ownCode));
						#endif
						if (ownCode == FACTORY_CODE_FLAG
							&& (g_updata_firmware_chk[0] == pFileParam->dataBuf[pFileParam->segmentLen])
							&& (g_updata_firmware_chk[1] == pFileParam->dataBuf[pFileParam->segmentLen + 1]))
						{
							//校验
							s_pUser->character = 0x0123abcd;
							s_pUser->version = s_new_version;
							s_pUser->code_size = g_PlmStatusPara.file_addr - s_file_addr_start;
							if (0 == s_pUser->code_size % 4)
							{
								#if defined(ZB204_CHIP)
								s_pUser->need_copy = 1;
								#endif
								s_pUser->code_checksum = s_code_checksum;
								s_pUser->field_checksum = Get_checksum_32((unsigned long *)s_pUser, (sizeof(FLASH_USER_CHARACTER_FIELD) - 4) / 4);
								Unprotection1MFlash();
								data_flash_write_straight(s_user_addr, (u8 *)s_pUser, sizeof(FLASH_USER_CHARACTER_FIELD));
								Protection1MFlash();
								SYS_UP_ReWrite((UPDATA_PARA*)(FLASH_AHB_ADDR+s_pUser->code_addr+s_pUser->code_size-sizeof(UPDATA_PARA)));
								Reboot_system(3, 0, __func__);
							}
						}
						

					}
					else
					{
						err_code = AFN_15_F1_ERR;
					}
					free_buffer(__func__,s_pUser);
					s_pUser=NULL;
				}
			}
			else if (0x08 == pFileParam->fileName||0x04 == pFileParam->fileName)     //08H：子节点模块升级  04H:海外升级子模块
			{
				if (pFileParam->segmentNum && pFileParam->segmentLen)
				{
					//判断此前是否收到过该段数据
					if (!(g_file_trans_map[pFileParam->segmentOffset >> 3] & (0x01 << (pFileParam->segmentOffset & 0x07))))
					{
						g_pRelayStatus->sta_update_cb.update_flag = 1;
						Unprotection1MFlash();
						data_flash_write_straight(g_PlmStatusPara.file_addr, pFileParam->dataBuf, pFileParam->segmentLen);
						Protection1MFlash();
						g_update_crc32[0] = GetCrcInit32(g_update_crc32[0], pFileParam->dataBuf, pFileParam->segmentLen);
						memset_special(pFileParam->dataBuf, 0xff, pFileParam->segmentLen);
						data_flash_read_straight(g_PlmStatusPara.file_addr, pFileParam->dataBuf, pFileParam->segmentLen);
						g_update_crc32[1] = GetCrcInit32(g_update_crc32[1], pFileParam->dataBuf, pFileParam->segmentLen);
						g_PlmStatusPara.file_addr += pFileParam->segmentLen;
						g_file_trans_map[pFileParam->segmentOffset >> 3] |= (UCHAR)(0x01 << (pFileParam->segmentOffset & 0x07));
					}
				}

				if (0x00 == pFileParam->fileAttr)  //起始帧、中间帧为 00H；
				{
					//可以校验文件大小是否溢出
					if (0 == pFileParam->segmentOffset)
					{
						//检查是否自家文件
					}
				}
				else if (0x01 == pFileParam->fileAttr)     //结束帧为 01H；
				{
					//最后一段，检查自带校验，正确则升级
					//检查是否所有段都已传输
					for (u16 i = 0; i <= pFileParam->segmentOffset; i++)
					{
						if (!(g_file_trans_map[i >> 3] & (0x01 << (i & 0x07))))
						{
							err_code = AFN_15_F1_ERR;
							break;
						}
					}

					if (err_code == pFileParam->segmentOffset)
					{
						if (g_update_crc32[0] == g_update_crc32[1])
						{
							u32 crcmask = ((((u32)1 << (32 - 1)) - 1) << 1) | 1;
							g_update_crc32[0] ^= 0xFFFFFFFF;
							g_update_crc32[0] &= crcmask;
							//启动升级子节点
							u32 file_size = g_PlmStatusPara.file_addr - s_file_addr_start;
							PLC_StartUpdateSta(s_file_addr_start, file_size, g_update_crc32[0]);
						}
						else
						{
							err_code = AFN_15_F1_ERR;
						}
					}
					else
					{
						err_code = AFN_15_F1_ERR;
					}
					free_buffer(__func__,s_pUser);
					s_pUser=NULL;
				}
			}

			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &err_code, 4);
			pLmpAppInfo->responsePoint += 4;

			break;
		}
	case F2:
		{
			pLmpAppInfo->confirmFlag = 2;
			break;
		}
	case F100:
		{
			if ((3 + 4) == pLmpAppInfo->appLayerLen)
			{
				u32 addr;
				memcpy_special(&addr, &pLmpAppInfo->curProcUnit->data_unit[0], 4);

				memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &addr, 4);
				pLmpAppInfo->responsePoint += 4;

				memset_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 0, 1024);
				data_flash_read_straight(addr, &pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], 1024);
				pLmpAppInfo->responsePoint += 1024;
				return OK;
			}

			return ERROR;
		}
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}

	return OK;
}

#if 1
// BCD code 转化为long type.
unsigned long Bcd2HexLong(unsigned long bcd)
{
	unsigned long temp;
	unsigned long ret;
	unsigned char ii;

	ret = 0;

	for (ii = 0; ii < 8; ii++)
	{
		// 每次取4-bits, 然后向后偏移到最低位
		temp = bcd & 0xF0000000;
		temp >>= 28;

		/* BCD 码转化为十进制数:  按位取出4 bit value, 然后* 10 , 累加到终值. 得到结果就是实际数值*/
		ret = ret * 10 + temp;
		bcd <<= 4;
	}

	return (ret);
}

unsigned short Isthere8771()
{
	USHORT metersn;
	unsigned long *bcd;
	unsigned long num;

	for (metersn = 0; metersn < CCTT_METER_NUMBER; metersn++)
	{
		if (CCO_USERD != g_MeterRelayInfo[metersn].status.used)
			continue;

		if ((g_MeterRelayInfo[metersn].meter_addr[5] == 0x01)
			&& (g_MeterRelayInfo[metersn].meter_addr[4] == 0x00))
		{
			bcd = (unsigned long *)&g_MeterRelayInfo[metersn].meter_addr[0];

			num = Bcd2HexLong(*bcd);

			if ((num >= 4384431) && (num <= 4396220))
				return metersn;
			if ((num >= 4418671) && (num <= 4419820))
				return metersn;
			if ((num >= 4420664) && (num <= 4420675))
				return metersn;
			if ((num >= 4421422) && (num <= 4421697))
				return metersn;
			if ((num >= 4423621) && (num <= 4423820))
				return metersn;

		}
	}

	return 0xFFFF;
}
#endif


unsigned char afn_factory_read(P_LMP_APP_INFO pLmpAppInfo)
{
#if 0
	switch(pLmpAppInfo->curDT)
	{
		case F1:
		{
			break;
		}
		case F2:
		{
#ifdef APP_MF_RELAY

#else
			PLC_get_relay_info();
			memcpy_special((pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint),
						   g_pRelayStatus,
						   (OffsetOf(RELAY_STATUS,group_meter_num)+g_pRelayStatus->group_num*2));
			pLmpAppInfo->responsePoint += (OffsetOf(RELAY_STATUS,group_meter_num)+g_pRelayStatus->group_num*2);
#endif
			break;
		}
		case F3:
		{
#ifdef APP_MF_RELAY
 #else
#endif
			break;
		}
		case F4:
		{
			break;
		}
		case F5:

#ifdef APP_MF_RELAY
#else
		PLC_get_relay_info();

		memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
					   &g_pRelayStatus->all_meter_num, 2);
		pLmpAppInfo->responsePoint += 2;

		UCHAR *real_num;
		real_num = (UCHAR *)((ULONG)pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint);
		*real_num = 0;

		pLmpAppInfo->responsePoint++;

		UCHAR  meter_num = pLmpAppInfo->curProcUnit->data_unit[2];
		if(meter_num > 20)
		meter_num = 20;

		USHORT start_sn = *((USHORT *)pLmpAppInfo->curProcUnit->data_unit);
		USHORT ii;
		for(ii = start_sn; ii <  CCTT_METER_NUMBER; ii++)
		{

			if(g_MeterRelayInfo[ii].status.used != CCO_USERD)
			continue;

			(*real_num)++;

			memcpy_special(pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint,
						   &g_MeterRelayInfo[ii], sizeof(RELAY_INFO));
			pLmpAppInfo->responsePoint += sizeof(RELAY_INFO);

			if(*real_num == meter_num)
			break;

		}
#endif
		break;
		case F6:
		{
			USHORT item_num;

			item_num = Relay_Queue_Num();
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = (UCHAR)item_num;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = (UCHAR)(item_num>>8);
			pLmpAppInfo->responsePoint += 2;

			item_num = Relay_Stack_ItemNum();
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = (UCHAR)item_num;
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = (UCHAR)(item_num>>8);
			pLmpAppInfo->responsePoint += 2;

			memcpy_special((pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint),
						   (void *)(ULONG)&g_mf_relay_credit, sizeof(g_mf_relay_credit));
			pLmpAppInfo->responsePoint += sizeof(g_mf_relay_credit);

			break;
		}
		case F7:
		{
			unsigned short d1, d2;
			ULONG mem_add;
			unsigned char mem_len;

			d1 = pLmpAppInfo->curProcUnit->data_unit[0] | ((USHORT)pLmpAppInfo->curProcUnit->data_unit[1] << 8);
			d2 = pLmpAppInfo->curProcUnit->data_unit[2] | ((USHORT)pLmpAppInfo->curProcUnit->data_unit[3] << 8);

			mem_add = (ULONG)(((ULONG)d2 << 16) | d1);
			mem_len = pLmpAppInfo->curProcUnit->data_unit[4];

			if(mem_len > 64)
			mem_len = 64;

			d1 = 0;
			if(((ULONG)mem_add+mem_len) < 0xE0000)
			d1 = 1;
			if(((ULONG)mem_add >= 0xFCF00) && (((ULONG)mem_add+mem_len) < 0xFFEE0))
			d1 = 1;

			if(d1)
			{
				// read 32 bytes from given
				memcpy_special((pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint),
							   (UCHAR *)mem_add, mem_len);
				pLmpAppInfo->responsePoint += mem_len;
			}
			else
			{
				memcpy_special((pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint),
							   &mem_add, 4);
				pLmpAppInfo->responsePoint += 4;

				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = mem_len;
			}

			break;
		}
		case F8:
		{
			unsigned short metersn;

			metersn = pLmpAppInfo->curProcUnit->data_unit[0] | ((USHORT)pLmpAppInfo->curProcUnit->data_unit[1] << 8);

			if(metersn < CCTT_METER_NUMBER)
			{
				memcpy_special((pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint),
							   Relay_Stack_Per_Node_StkMask(metersn), 8);
				pLmpAppInfo->responsePoint += 8;

				memcpy_special((pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint),
							   Relay_Stack_Per_Node_Stk(metersn), 16);
				pLmpAppInfo->responsePoint += 16;

				memcpy_special((pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint),
							   &g_MeterRelayInfo[metersn], 32);
				pLmpAppInfo->responsePoint += 32;
			}
			break;
		}
		case F9:
		{
			// forget unstable meters, it will restart search/group
			if((pLmpAppInfo->curProcUnit->data_unit[0] == 0x77)
			   && (pLmpAppInfo->curProcUnit->data_unit[1] == 0x88))
			{
				Relay_MF_forget_unstable();
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x78;
				pLmpAppInfo->responsePoint += 1;
			}
			else if((pLmpAppInfo->curProcUnit->data_unit[0] == 0x67)
					&& (pLmpAppInfo->curProcUnit->data_unit[1] == 0x70))
			{
				Relay_MF_forget_group(0xff);
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0;
				pLmpAppInfo->responsePoint += 1;
			}
			else if((pLmpAppInfo->curProcUnit->data_unit[0] == 0x99)
					&& (pLmpAppInfo->curProcUnit->data_unit[1] == 0x00))
			{
			}
			else if((pLmpAppInfo->curProcUnit->data_unit[0] == 0xDD)
					&& (pLmpAppInfo->curProcUnit->data_unit[1] == 0xDD))
			{
				Relay_MF_forget_unfreezed_and_done();
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x6d;
				pLmpAppInfo->responsePoint += 1;
			}
			else if((pLmpAppInfo->curProcUnit->data_unit[0] == 0x66)
					&& (pLmpAppInfo->curProcUnit->data_unit[1] == 0xDD))
			{
				Relay_MF_forget_unfreezed();
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x6d;
				pLmpAppInfo->responsePoint += 1;
			}
			else if((pLmpAppInfo->curProcUnit->data_unit[0] == 0x83)
					&& (pLmpAppInfo->curProcUnit->data_unit[1] == 0x38))
			{
				Relay_MF_remove_selfreged();
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x38;
				pLmpAppInfo->responsePoint += 1;
			}
			else if((pLmpAppInfo->curProcUnit->data_unit[0] == 0xF1)
					&& (pLmpAppInfo->curProcUnit->data_unit[1] == 0x1F))
			{
				unsigned short metersn;

				metersn = pLmpAppInfo->curProcUnit->data_unit[2] | ((USHORT)pLmpAppInfo->curProcUnit->data_unit[3] << 8);

				if(metersn < CCTT_METER_NUMBER)
				{
					Relay_MF_forget_one_meter(metersn);
					pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0xF1;
					pLmpAppInfo->responsePoint += 1;
				}
			}
			else if((pLmpAppInfo->curProcUnit->data_unit[0] == 0xF2)
					&& (pLmpAppInfo->curProcUnit->data_unit[1] == 0x2F))
			{
				unsigned short metersn;

				metersn = pLmpAppInfo->curProcUnit->data_unit[2] | ((USHORT)pLmpAppInfo->curProcUnit->data_unit[3] << 8);

				while(metersn < CCTT_METER_NUMBER)
				{
					memset_special((unsigned char *)&g_MeterRelayInfo[metersn], 0xff, sizeof(g_MeterRelayInfo[0]));
					metersn ++;
				}
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0xF2;
				pLmpAppInfo->responsePoint += 1;
			}
			else if((pLmpAppInfo->curProcUnit->data_unit[0] == 0xF5)
					&& (pLmpAppInfo->curProcUnit->data_unit[1] == 0x5F))
			{
				unsigned short count;

				count = Relay_MF_forget_wrongupper();
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = (unsigned char)count;
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = (unsigned char)(count>>8);
				pLmpAppInfo->responsePoint += 2;
			}
			break;
		}
		case F10:
		{
			extern MSG_SHORT_INFO g_mf_Dl645Msg;

			memcpy_special((pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint),
						   g_mf_Dl645Msg.msg_buffer, 64);

			pLmpAppInfo->responsePoint += 64;

			break;
		}
		case F11:
		{
			break;
		}
		case F12:
		{
			if(pLmpAppInfo->curProcUnit->data_unit[0] == 0)
			PLC_stage_pause();
			else
			PLC_stage_resume();

			break;
		}
		case F13:
		{
			unsigned short msn;
			msn = Isthere8771();
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = (unsigned char)(msn & 0xff);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = (unsigned char)(msn >> 8);
			pLmpAppInfo->responsePoint += 2;
			break;
		}
		case F14:
		{
			if(pLmpAppInfo->curProcUnit->data_unit[0] == 0x55)
			g_mf_relay_credit.search_resettle = 1000;
			else if(pLmpAppInfo->curProcUnit->data_unit[0] == 0x66)
			g_mf_relay_credit.search_use_upper_peer = 1000;
			break;
		}
		case F15:
		{
			Relay_MF_clear_prio();
			Relay_Stack_Unstable_List_Init();
			break;
		}
		case F16:
		{
			unsigned short len;
			len = Relay_MF_Get_Unfreezed_Meter_Arry(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			pLmpAppInfo->responsePoint += len;
			break;
		}
		case F17:
		{
			if((pLmpAppInfo->curProcUnit->data_unit[0] == 0x97)
			   && (pLmpAppInfo->curProcUnit->data_unit[1] == 0x79))
			{
				Relay_Stack_Per_Node_Gen();
				pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0x97;
				pLmpAppInfo->responsePoint += 1;
			}
			else if((pLmpAppInfo->curProcUnit->data_unit[0] == 0x84)
					&& (pLmpAppInfo->curProcUnit->data_unit[1] == 0x5d))
			{
				unsigned short len;

				len = Relay_MF_Get_Wrongupper_Meter_Arry(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
				pLmpAppInfo->responsePoint += len;
			}
			break;
		}
		case F18:
		{
			unsigned short len;
			len = Relay_MF_Get_UnfreezedDone_Meter_Arry(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			pLmpAppInfo->responsePoint += len;
			break;
		}
		case F19:
		{
			unsigned short len;
			len = Relay_MF_Get_Unfreezed_GrpNum_Arry(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			pLmpAppInfo->responsePoint += len;
			break;
		}
		case F20:
		{
			unsigned short len;
			len = Relay_MF_Get_Unknown_Meter_Arry(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			pLmpAppInfo->responsePoint += len;
			break;
		}
		case F21:
		{
			unsigned short len;
			len = Relay_Queue_Get_HeadItems(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], pLmpAppInfo->curProcUnit->data_unit[0]);
			pLmpAppInfo->responsePoint += len;
			break;
		}
		case F22:
		{
			PLC_stage_resume();
			break;
		}
		case F23:
		{
			break;
		}
		case F24:
		{
			break;
		}
		case F25:
		{
			break;
		}
		case F26:
		{
			// display selfreg infomation
			memset_special((unsigned char *)g_relay_neighbour_num, 0xff, sizeof(g_relay_neighbour_num));
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = 0;
			pLmpAppInfo->responsePoint++;

			break;
		}
		case F27:
		{
#ifdef MF_USE_NEIGHBOURHOOD
			unsigned short len;
			len = Relay_MF_Get_NeighbourStat(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			pLmpAppInfo->responsePoint += len;
#endif
			break;
		}
		case F28:
		{
			extern unsigned short g_mf_last_relay[8];
			//extern UCHAR g_plc_ctl_debug[4];

			//memcpy_special(g_plc_ctl_debug, &pLmpAppInfo->curProcUnit->data_unit[0], 4);

			memcpy_special((pLmpAppInfo->msg_buffer + pLmpAppInfo->responsePoint),
						   &g_mf_last_relay, 16);
			pLmpAppInfo->responsePoint += 16;

			break;
		}
		case F29:
		{
			unsigned short unused;

			unused = Relay_MF_searchever_unused();
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = (unsigned char)(unused & 0xff);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = (unsigned char)(unused >> 8);
			pLmpAppInfo->responsePoint += 2;

			unused = Relay_MF_searchx_unused();
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = (unsigned char)(unused & 0xff);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = (unsigned char)(unused >> 8);
			pLmpAppInfo->responsePoint += 2;

			unused = Relay_MF_searchknown_unused();
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = (unsigned char)(unused & 0xff);
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint+1] = (unsigned char)(unused >> 8);
			pLmpAppInfo->responsePoint += 2;

			//Relay_MF_SSearch_ClrXFlg();
			break;
		}
		case F30:
		{
			break;
		}
		case F31:
		{
			unsigned short len;
			len = Relay_MF_Get_Dl645_CmdItems(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			pLmpAppInfo->responsePoint += len;
			break;
		}
		case F33:
		{
			unsigned short len;
			len = Relay_MF_Get_Dl645_RespItems(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint]);
			pLmpAppInfo->responsePoint += len;
			break;
		}
		case F34:
		{
			Reboot_system(3, en_reboot_cmd);
			break;
		}
		case F35:
		{
			break;
		}
		case F36:
		{
			//pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = g_PlmConfigPara.plc_auto_reported;
			//pLmpAppInfo->responsePoint++;
			break;
		}
		case F37:
		{
			break;
		}
		case F38:
		{
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = g_PlmConfigPara.plc_baud_dirread;
			pLmpAppInfo->responsePoint++;
			break;
		}
		case F39:
		{
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &g_PlmConfigPara.pack_info[0], sizeof(g_PlmConfigPara.pack_info));
			pLmpAppInfo->responsePoint += sizeof(g_PlmConfigPara.pack_info);
			break;
		}
		case F40:
		{
			MF_RELAY_QUENUM_ABCU que_abcu;
			Relay_Queue_ABC_num(&que_abcu);
			memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &que_abcu, sizeof(que_abcu));
			pLmpAppInfo->responsePoint += sizeof(que_abcu);
			break;
		}
		case F44:
		{
			//读取配置的冻结项
			BYTE counter = 0;
			USHORT i_count = pLmpAppInfo->responsePoint++;
			for(BYTE i=0; i<CCTT_FREEZE_DI_NUMBER; i++)
			{
				if(CCTT_INVALID_DL645_DI == g_PlmConfigPara.freeze_di[i])
				break;

				memcpy_special(&pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint], &g_PlmConfigPara.freeze_di[i], 4);
				pLmpAppInfo->responsePoint += 4;

				counter++;
			}
			pLmpAppInfo->msg_buffer[i_count] = counter;

			break;
		}
		case F45:
		{
			if(OK == Relay_MF_IsAllBridgeRelayed())
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 1;
			else
			pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint++] = 0;
			break;
		}
		case F100:  //F100：终端版本信息
		{
			unsigned char *ver_inf;
			if(pLmpAppInfo->lmpLinkHead->Inf.down_info.comm_module)
			ver_inf = (UCHAR *)&plc_ver_info;
			else
			ver_inf = (UCHAR *)&c_Tmn_ver_info;
			memcpy_special(pLmpAppInfo->msg_buffer+pLmpAppInfo->responsePoint, ver_inf, sizeof(TMN_VER_INFO)-4);
			pLmpAppInfo->responsePoint += sizeof(TMN_VER_INFO)-4;
			break;
		}
		default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}
#endif
	return OK;

}

unsigned char afn_factory_write(P_LMP_APP_INFO pLmpAppInfo)
{
	switch (pLmpAppInfo->curDT)
	{
	case F1:
		{
			break;
		}
	case F2:
		{
			#if 0
			USHORT metersn;
			BYTE bEdit = FALSE;
			for(metersn=0; metersn<CCTT_METER_NUMBER; metersn++)
			{
				if(CCO_USERD == g_MeterRelayInfo[metersn].status.used)
				{
					if(1 != g_MeterRelayInfo[metersn].status.rsvd)
					{
						bEdit = TRUE;
						g_MeterRelayInfo[metersn].status.rsvd = 1;
					}
				}
			}
			if(bEdit)
			{
				PLC_save_relay_info(NULL, NULL);
			}
			#endif
			break;
		}
	case F3:
		{
			#if 0
			USHORT metersn;
			BYTE bEdit = FALSE;
			for(metersn=0; metersn<CCTT_METER_NUMBER; metersn++)
			{
				if(CCO_USERD == g_MeterRelayInfo[metersn].status.used)
				{
					if(AC_PHASE_U != g_MeterRelayInfo[metersn].status.phase)
					{
						bEdit = TRUE;
						g_MeterRelayInfo[metersn].status.phase = AC_PHASE_U;
					}
				}
			}
			if(bEdit)
			{
				PLC_save_relay_info(NULL, NULL);
			}
			#endif
			break;
		}
	case F4:
		{
			break;
		}
	case F35:
		{
			break;
		}
	case F36:
		{
			break;
		}
	case F37:
		{
			break;
		}
	case F38:
		{
			break;
		}
	case F39:
		{
			PBYTE p = &pLmpAppInfo->curProcUnit->data_unit[0];
			P_PACK_INFO pPackInfo = (P_PACK_INFO)p;
			for (BYTE i = 0; i < CCTT_READ_PACK_NUMBER; i++)
			{
				if (IsMemSet(pPackInfo, 0xff, sizeof(PACK_INFO)))
				{
					if (FALSE == PLC_IsPackInfoValid(pPackInfo))
						return ERROR;
				}
				pPackInfo++;
			}
			memcpy_special(&g_PlmConfigPara.pack_info[0], p, sizeof(g_PlmConfigPara.pack_info));
			dc_flush_fram(&g_PlmConfigPara.pack_info[0], sizeof(g_PlmConfigPara.pack_info), FLASH_FLUSH);
			break;
		}
	case F41:
		{
			//485表的采集器号无效，则修改为载波表
			#if 0
			USHORT ii, nCollSn;
			BYTE bEdit = FALSE;

			for(ii=0; ii<CCTT_METER_NUMBER; ii++)
			{
				if(CCO_USERD != g_MeterRelayInfo[ii].status.used)
				continue;

				if(0 != g_MeterRelayInfo[ii].status.plc)
				continue;

				if(METER_TYPE_METER != g_MeterRelayInfo[ii].meter_type.m0c1)
				continue;

				nCollSn = g_MeterRelayInfo[ii].coll_sn;

				if(nCollSn >= CCTT_METER_NUMBER
				   || CCO_USERD != g_MeterRelayInfo[nCollSn].status.used
				   || 1 != g_MeterRelayInfo[nCollSn].status.plc
				   || METER_TYPE_COLL != g_MeterRelayInfo[nCollSn].meter_type.m0c1
				   )
				{
					g_MeterRelayInfo[ii].status.plc = 1;
					g_MeterRelayInfo[ii].meter_type.m0c1 = METER_TYPE_METER;
					g_MeterRelayInfo[ii].coll_sn = 0xffff;
					bEdit = TRUE;
				}
			}
			if(bEdit)
			{
				PLC_save_relay_info(NULL, NULL);
			}
			#endif

			break;
		}
	case F42:
		{
#ifdef PROTOCOL_GD_2016
			g_PlmStatusPara.file_update_flag = pLmpAppInfo->curProcUnit->data_unit[0];
#endif
			break;
		}
	case F43:
		{
			//修改波特率
			P_TMSG pMsg = alloc_msg(__func__);
			if (NULL != pMsg)
			{
				OS_ERR err;
				pMsg->message = MSG_PLM_SET_BAUDRATE;
				pMsg->wParam = pLmpAppInfo->curProcUnit->data_unit[0];
				OSTaskQPost(&g_pOsRes->APP_PLM_2005_TCB, pMsg, sizeof(pMsg), OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED, &err);
				if (OS_ERR_NONE != err)
					free_msg(__func__,pMsg);
			}
			break;
		}
	case F44:
		{
			//配置的冻结项
			BYTE count = pLmpAppInfo->curProcUnit->data_unit[0];
			PBYTE p = &pLmpAppInfo->curProcUnit->data_unit[1];
			ULONG sum_cmd;
			ULONG sum_cal = 0;

			if (count > CCTT_FREEZE_DI_NUMBER)
				return ERROR;

			memcpy_special(&sum_cmd, p + 4 * count, 4);

			for (BYTE i = 0; i < count; i++)
			{
				ULONG DI_cmd;
				memcpy_special(&DI_cmd, p + 4 * i, 4);
				sum_cal += DI_cmd;
			}

			if (sum_cal != sum_cmd)
				return ERROR;

			memset_special(g_PlmConfigPara.freeze_di, 0xff, sizeof(g_PlmConfigPara.freeze_di));

			for (BYTE i = 0; i < count; i++)
			{
				memcpy_special(&g_PlmConfigPara.freeze_di[i], p + 4 * i, 4);
			}

			memcpy_special(&g_PlmConfigPara.freeze_di[CCTT_FREEZE_DI_NUMBER], p + 4 * count, 4);

			dc_flush_fram(&g_PlmConfigPara.freeze_di[0], sizeof(g_PlmConfigPara.freeze_di), FLASH_FLUSH);

			break;
		}
	default:
		pLmpAppInfo->msg_buffer[pLmpAppInfo->responsePoint] = RT_ERR_INVALID_DT;
		pLmpAppInfo->responsePoint++;
		return ERROR;
	}

	return OK;
}


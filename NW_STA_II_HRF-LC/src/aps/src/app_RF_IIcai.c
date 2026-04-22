#include "protocol_includes.h"
#include "os.h"
#include "common_includes.h"
#include <string.h>
#include "bsp.h"
#include "Revision.h"
#include "hplc_data.h"
#include "app_search_meter.h"
#include "app_event_IIcai.h"
#include "app_RF_IIcai.h"
#include "bps_ir_uart.h"
#include "PLM_2005.h"
#include "lc_all.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//add by LiHuaQiang 2020.10.10 -START-
//----------------------------------------------------------------
//????
#if 0
typedef struct{
	u8 Model[8];
	u8 ReleaseDate[3];
	u8 ReleaseDateChar[6];
	u8 ExtVer[2];
}DEVICE_VERSION;



u8 IRF_send_data_start_flag = true;//true:紅外數據發送開始
u8 IRF_send_data_end_flag = true;//true:紅外數據發送完成
//接收缓存
static u8 LocalReceiveIFRBuf[LOCAL_IFR_BUFF_SIZE] = { 0 };

bool ProduceModeFlag = false;//true:生產模式

extern OS_SEM ReceiveIrComplateSEM;

void UartIrWrite(uint8_t *pbuff,uint32_t len, bool allow_receive);

//----------------------------------------------------------------
void SetProduceModeFlag(bool data)
{
	ProduceModeFlag = data;
}
bool GetProduceModeFlag(void)
{
	return ProduceModeFlag;
}
//??????
UINT16 LoadInt16(const UINT8 *pData)
{
    UINT16 tmp;

    tmp = *(pData + 1);
    tmp <<= 8;
    tmp |= *pData;
    return tmp;
}
UINT32 LoadInt32(const UINT8 *pData)
{
    UINT32 tmp;

    tmp = (UINT32)LoadInt16(pData + 2);
    tmp <<= 16;
    tmp |= (UINT32)LoadInt16(pData); 
    return tmp;
}
/* 内存数据加法操作 */
void memadd(UINT8 buf[], UINT8 addend, UINT8 cnt)
{
    UINT8 i = 0;

    for (i = 0; i < cnt; i++)
    {
        buf[i] += addend;
    }
    return;
}

/* 内存数据减法操作 */
void memsub(unsigned char buf[], unsigned char addend, unsigned char cnt)
{
    UINT8 i = 0;

    for (i = 0; i < cnt; i++)
    {
        buf[i] -= addend;
    }
    return;
}
//----------------------------------------------------------------
void Dlt645_SendRtnFrame(struct frame645 *pframe,UINT8 len)
{    
    pframe->datalen = len;
    pframe->control_code.control_bits.direction_flag = 1;
    memadd((UINT8 *)pframe->data, 0x33, pframe->datalen);
    *(pframe->data + pframe->datalen) = GetCheckSum((UINT8 *)pframe, pframe->datalen + CS_POSITION);
    *(pframe->data + pframe->datalen + 1) = END_FLAG;

    memmove((UINT8 *)(pframe) + 2, (UINT8 *)pframe, pframe->datalen + PROTOCOL_COAT_SIZE);
    memset((UINT8 *)pframe, 0xFE, 2);

}
/* 红外645帧处理 */
bool protocol645_dispose_IFR(struct frame645 *pframe)
{
    UINT8 i,size;   
    UINT16 id;
	UINT32 id_07;	
    
    // 地址类型判断    
    memsub((UINT8 *)(pframe->data), 0x33, pframe->datalen);
    id = LoadInt16(&pframe->data[0]);
    id_07= LoadInt32(&pframe->data[0]);
    
    switch(pframe->control_code.control_byte)
    {    
        case SELF_DEFINE://68 AA AA AA AA AA AA 68 00 08 33 35 35 33 33 33 33 33 70 16  设置采集器地址
                         
			if(id == 0x0200)//设置地址
			{
				memcpy(&pframe->addr[0],&pframe->data[2],6);
				memcpy((UINT8 *)&collector_addr[0],&pframe->data[2],6);				
		        goto WOkResponse;
			}		
			else if(id == 0x0002)//68 AA AA AA AA AA AA 68 00 02 35 33 36 16 读采集器地址
			{						        
		        memcpy((UINT8 *)&pframe->data[2],(UINT8 *)&collector_addr[0],6);
				memcpy(&pframe->addr[0],(UINT8 *)&collector_addr[0],6);
		        size=8;
		        goto ROkResponse;
			}
            else if(id == 0x000A)//读采集器版本   外部版本号
			{     
				memcpy(&pframe->addr[0],(UINT8 *)&collector_addr[0],6);

		        size=27;
		        goto ROkResponse;
			}
			
        case READ_DATA_97:   
                      
            size=0;
            goto NoResponse;
	    case READ_DATA_07:   
            if(id_07 == 0x04a00101)  //厂家软件版本号
            {
				memset((UINT8 *)&pframe->data[4],0x00,16);
	    
                size=23;
                goto ROkResponse;
            }
			if(id_07 == 0x04a00103)  //厂家软件版本号(扩展指令)
            {
                memset((UINT8 *)&pframe->data[4],0x00,16);
							    

                size=20;
                goto ROkResponse;
            }
			if(id_07 == 0x04a00201)  //终端复位次数
            {
				pframe->data[4]=(BYTE)(0&0x00ff);
                pframe->data[5]=(BYTE)(0>>8);
                size=6;
                goto ROkResponse;
            }
//			if(id_07 == 0x04a00300)  //注册集中器逻辑地址
//            {
//                memcpy((UINT8 *)&pframe->data[4],DeviceVersion.ExtVer,2);//
//				pframe->data[6]=0x00;
//                pframe->data[7]=0XFF;
//				pframe->data[8]=0xFF;
//                pframe->data[9]=0x00;
//                size=10;
//                goto ROkResponse;
//            }
			if(id_07 == 0x04a00301)  //电能表信息
            {                
                UINT8 buf[6]={0x00,0x00,0x00,0x00,0x00,0x00},meter_cnt=0;				
				if(SEARCH_METER_FLAG.meter_total > 0)
				{
					for(i=0;i<SEARCH_METER_FLAG.meter_total;i++)
					{                        
                        if(memcmp(&SEARCH_METER_FLAG.table[i*7],buf,6)==0)
                        {
                            continue;
                        }
                        memcpy(&pframe->data[meter_cnt*7+5],&SEARCH_METER_FLAG.table[i*7],6);
        				pframe->data[meter_cnt*7+11]=(SEARCH_METER_FLAG.table[i*7+6] & 0x03);       				
						pframe->data[meter_cnt*7+11] = pframe->data[meter_cnt*7+11] | 0x10;	
                        meter_cnt++;						
					}
				}
                pframe->data[4]=meter_cnt;//电表数量
                size=meter_cnt*7+5;//meter_total*7+5;
                goto ROkResponse;
            }
			if(id_07 == 0x04a00303)
			{
				memcpy((UINT8 *)&pframe->data[4],(UINT8 *)&collector_addr[0],6);
				memcpy(&pframe->addr[0],(UINT8 *)&collector_addr[0],6);
				size=10;				
                goto ROkResponse;
			}
			if(id_07 == 0x04a004ff)
            {
				pframe->data[4]=GetProduceModeFlag();           
                size=5;
                goto ROkResponse;
            }
            size=0;
            goto NoResponse;
		case WRITE_DATA_97:   
                      
            size=0;
            goto NoResponse;
		case WRITE_DATA_07: 
			if(id_07 == 0x04a00300)//设置集中器逻辑地址
			{				
                goto WOkResponse;
			}   
            if(id_07 == 0x04a00303)//设置采集器资产号
			{				
				memcpy(collector_addr,&pframe->data[4],6);				
                goto WOkResponse;
			}       
			if(id_07 == 0x04a00401)//硬件初始化
			{
			    RebootSystem();
				goto WOkResponse;
			}
			if(id_07 == 0x04a00402)//数据初始化
			{			    
				goto WOkResponse;
			}
			if(id_07 == 0x04a00403)//参数初始化
			{			    
				goto WOkResponse;
			}	
			if(id_07 == 0x04a004ff)
			{
				SetProduceModeFlag(true);
				goto WOkResponse;
			}
            size=0;
            goto NoResponse;
	    case REQUEST_DEVICE_ADDR_07:// Read Address			 
	        memcpy(&pframe->addr[0],&pframe->data[0],6);          
	        size=6;
	        goto ROkResponse;
	    case WRITE_DEVICE_ADDR_07:// Write Address			
	        memcpy(&pframe->addr[0],&pframe->data[0],6);
			memcpy(collector_addr,&pframe->data[0],6);			
	        goto WOkResponse;
        default:
            goto NoResponse;
    }
ROkResponse:
	Dlt645_SendRtnFrame(pframe,size);	
	return TRUE;
WOkResponse:
	Dlt645_SendRtnFrame(pframe,0);
	return TRUE;
NoResponse:
	return FALSE;
}
void TransIFR2Rs485(u8 *data,u16 len)
{
	u8 buf[54];
	u8 addr[6] = {0};
	if(len > 50)
		AppLocalReceiveData( addr, addr, IFR_READ_METER_5000MS, 0, data, len, RS485DataDeal, 0, 0, BAUD_SEARCH );
//		AppLocalReceiveData(IFR_READ_METER_5000MS,
//				0,
//				0,
//				0,
//				0,
//				0,
//				0,
//				0, 
//				data, 
//				len, 
//				RS485DataDeal,
//				0,
//				0);
	else
	{
		memset(buf,0xFE,4);//加前導符
		memcpy(buf+4,data,len);
		AppLocalReceiveData( addr, addr, IFR_READ_METER_5000MS, 0, buf, len+4, RS485DataDeal, 0, 0, BAUD_SEARCH );
//		AppLocalReceiveData(IFR_READ_METER_5000MS,
//				0,
//				0,
//				0,
//				0,
//				0,
//				0,
//				0, 
//				buf, 
//				len+4, 
//				RS485DataDeal,
//				0,
//				0);
	}
	SetWait485DataFlag(UART_IFR_DATA); 
}
void JugeSendData2IFR_End()
{
	OS_ERR err;
	
	if (!IsIrUartTxData())//DMA已經傳輸完成
	{
		OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
		while (!IsIrUartTxSendAllDone())//硬件buff已經為空
		{
			OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
		}
		//紅外發送完數據后，紅外有可能收到錯誤的數據，所以以下是清空接收數據
		OSSemPend(&ReceiveIrComplateSEM, 100, OS_OPT_PEND_BLOCKING, 0, &err);
		if (err==OS_ERR_NONE)
		{
		  UartIrFlushBuffer();
		}
		IRF_send_data_end_flag = true;
		IRF_send_data_start_flag = false;
		IR_UART.Control(ARM_USART_CONTROL_RX, 1);
	}
}
void AppLocalHandleIFR(void)
{
	u16 index=0;	
	OS_ERR err;
	u8 m_len;
	
	if(AppUseUartFlag != 0)
		return;
	
	if(IRF_send_data_end_flag)
	{
		OSSemPend(&ReceiveIrComplateSEM, 1, OS_OPT_PEND_BLOCKING, 0, &err);
		P_LMP_LINK_HEAD pIrHead=NULL;
		if (err==OS_ERR_NONE)
		{
			u32 len = UartIrRead(&LocalReceiveIFRBuf[0], LOCAL_IFR_BUFF_SIZE);	
			if(lc_IsHave645Frame(LocalReceiveIFRBuf,len,&index))//是645報文
			{
				m_len = lc_protocol645_dispose_IFR(&LocalReceiveIFRBuf[index]);
				UartIrWrite(&LocalReceiveIFRBuf[index],m_len,false);
				IRF_send_data_start_flag = true;	
				
			}
			else if(pIrHead=PLM_Check_Frame_3762(LocalReceiveIFRBuf,len))//是南网本地红外接口协议
			{
				LMP_APP_INFO pHead = PLM_process_frame(pIrHead);
				char *pAck = NULL;
				u8 err = RT_ERR_CMD_NOT_SUPPORT;
				u16 dg_len = 0;

				if (pHead.cur_afn == 0x21)//请求II采自身信息
				{
					switch (pHead.curDT)
					{
					case 0xEA062101: //请求表地址个数
						{
							dg_len = 1;
							pAck = LMP_make_frame_gd_cmd(pHead.cur_afn, pHead.curDT, pHead.seq, &SEARCH_METER_FLAG.meter_total, &dg_len, NULL, true);
							break;
						}
					case 0xEA042102: //请求表地址
						{
							if (pHead.appLayerLen != 2)
							{
								err = RT_ERR_INVALID_FORMAT;
								break;
							}

							u8 start_sn = pHead.curProcUnit->data_unit[0];   //起始表序号
							u8 meter_num = pHead.curProcUnit->data_unit[1];  //表个数

							if (start_sn > 31)
							{
								err = RT_ERR_INVALID_FORMAT;
								break;
							}

							if ((meter_num < 1) || (meter_num > 32))
							{
								err = RT_ERR_INVALID_FORMAT;
								break;
							}
							
							u8 *p = malloc(256);
							if (p == NULL)
							{
								err = RT_ERR_BUSY_NOW;
								break;
							}

							u8 *head = p;
							u8 *real_num = head++;
							*real_num = 0;

							for (int i=0; i<32; i++)
							{
								if (SEARCH_METER_FLAG.TableList & (1<<i))
								{
									if (start_sn == 0)
									{
										(*real_num)++;
										memcpy(head, &SEARCH_METER_FLAG.table[i*7], 6);
										head += 6;
										
										if (!meter_num)
										{
											--meter_num;
											break;
										}
									}
									else
									{
										start_sn--;
									}
								}
							}
							
							dg_len = *real_num*6 + 1;
							pAck = LMP_make_frame_gd_cmd(pHead.cur_afn, 0xEA032102, pHead.seq, p, &dg_len, NULL, true);
							free(p);

							break;
						}
					case 0xEA062103: //请求采集器地址
						{
							dg_len = 6;
							pAck = LMP_make_frame_gd_cmd(pHead.cur_afn, pHead.curDT, pHead.seq, StaChipID.mac, &dg_len, NULL, true);
							break;
						}
					case 0xEA052104: //电表探测列表
						{
							u16 wait_time = 0;
							u8 meter_num = pHead.curProcUnit->data_unit[0];   //表个数

							if ((meter_num < 1) || (meter_num > 16))
							{
								err = RT_ERR_INVALID_FORMAT;
								break;
							}
							
							if (pHead.appLayerLen != (meter_num*6+1))
							{
								err = RT_ERR_INVALID_FORMAT;
								break;
							}
							
							dg_len = 2;
							pAck = LMP_make_frame_gd_cmd(0x00, 0xEA010001, pHead.seq, (u8 *)&wait_time, &dg_len, NULL, true);
							break;
						}
					case 0xEA062105: //电表探测状态
						{
							u8 status = GetCheckTableFlag();
							dg_len = 1;
							pAck = LMP_make_frame_gd_cmd(pHead.cur_afn, pHead.curDT, pHead.seq, &status, &dg_len, NULL, true);
							break;
						}
					case 0xEA062106: //读取采集器绑定地址
						{
							u8 addr[6];
							memcpy_swap(addr, (u8 *)GetStaBaseAttr()->Mac, 6);

							dg_len = 6;
							pAck = LMP_make_frame_gd_cmd(pHead.cur_afn, pHead.curDT, pHead.seq, addr, &dg_len, NULL, true);
							break;
						}
					}
				}
				else if (pHead.cur_afn == 0x23)//读参数
				{
					switch (pHead.curDT)
					{
					case 0xEA002301: //厂商代码和版本信息
						{
							index = 0;
							u8 Data[9] = {0};

							Data[index++] = StaVersion.FactoryID[1];
							Data[index++] = StaVersion.FactoryID[0];
							Data[index++] = StaVersion.ChipCode[1];
							Data[index++] = StaVersion.ChipCode[0];

							//模块版本信息
							Data[index++] = dec_to_bcd(StaVersion.VerDate.Day);
							Data[index++] = dec_to_bcd(StaVersion.VerDate.Month);
							Data[index++] = dec_to_bcd(StaVersion.VerDate.Year);
							Data[index++] = StaVersion.SofeVer[1];
							Data[index++] = StaVersion.SofeVer[0];
							dg_len = index;
							pAck = LMP_make_frame_gd_cmd(pHead.cur_afn, pHead.curDT, pHead.seq, Data, &dg_len, NULL, true);
							break;
						}	
					}
				}
				else if (pHead.cur_afn == 0x25)
				{
					switch (pHead.curDT)
					{
					case 0xEA062501: //查询设备类型
						{
							u8 tt = 0;
							dg_len = 1;
							pAck = LMP_make_frame_gd_cmd(pHead.cur_afn, pHead.curDT, pHead.seq, &tt, &dg_len, NULL, true);
							break;
						}
					}
				}
				else if (pHead.cur_afn == 0x31)
				{
					switch (pHead.curDT)
					{
					case 0xEA063101: //请求映射表地址个数
						{
							u8 num = 0;
							
							dg_len = 1;
							pAck = LMP_make_frame_gd_cmd(pHead.cur_afn, pHead.curDT, pHead.seq, &num, &dg_len, NULL, true);
							break;
						}
					case 0xEA043102: //请求映射表地址
						{
							u8 num = 0;

							if (pHead.appLayerLen != 2)
							{
								err = RT_ERR_INVALID_FORMAT;
								break;
							}

							u8 start_sn = pHead.curProcUnit->data_unit[0];   //起始表记序号
							u8 meter_num = pHead.curProcUnit->data_unit[1];  //表记个数

							if (start_sn > 15)
							{
								err = RT_ERR_INVALID_FORMAT;
								break;
							}

							if ((meter_num < 1) || (meter_num > 16))
							{
								err = RT_ERR_INVALID_FORMAT;
								break;
							}

							dg_len = 1;
							pAck = LMP_make_frame_gd_cmd(pHead.cur_afn, 0xEA033102, pHead.seq, &num, &dg_len, NULL, true);
							break;
						}
					case 0xEA053104: //映射探测列表
						{
							u16 wait_time = 0;							

							dg_len = 2;
							pAck = LMP_make_frame_gd_cmd(0x00, 0xEA010001, pHead.seq, (u8 *)&wait_time, &dg_len, NULL, true);
							break;	
						}
					case 0xEA063105: //映射探测状态
						{
							u8 num = 0;

							dg_len = 1;
							pAck = LMP_make_frame_gd_cmd(pHead.cur_afn, pHead.curDT, pHead.seq, &num, &dg_len, NULL, true);
							break;
						}
					}
				}
				
				if ((pAck == NULL) && (dg_len == 0))
				{
					dg_len = 1;
					pAck = LMP_make_frame_gd_cmd(0x00, 0xEA010002, pHead.seq, &err, &dg_len, NULL, true);
				}
				if (pAck)
				{
					IRF_send_data_start_flag = true;	
					IRF_send_data_end_flag = false;
					UartIrWrite((u8 *)pAck, dg_len,false);
					free(pAck);
				}
			}
		}
	}
	
	if(IRF_send_data_start_flag)
	{
		JugeSendData2IFR_End();		
	}
}
void AnswerReadMeter2IFR(u8 *data,u16 len)
{
	UartIrWrite(data,len,false);
	IRF_send_data_start_flag = true;	
	IRF_send_data_end_flag = false;
}
#endif
//----------------------------------------------------------------
//add by LiHuaQiang 2020.10.10 -END-
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------

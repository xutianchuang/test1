#include <stdlib.h>
#include <string.h>
#include "lc_all.h"

//
uint32_t whi_hplc_adapt_create(uint32_t whi_cmd,uint8_t *p_dst,uint8_t *p_src,uint32_t len)
{
	uint32_t dlen = 0;
	if((p_src == NULL)||(len ==0)||(p_dst == NULL))
		return 0;
	
	//uint8_t brocast_addr[LC_NET_ADDR_SIZE] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	//uint8_t local_brocast_addr[LC_NET_ADDR_SIZE] = {0xFE,0xFF,0xFF,0xFF,0xFF,0xFF};
	
	whi_hplc_adapt_head_t *plc_head = (whi_hplc_adapt_head_t*)p_dst;
	plc_head->port = 0x11;
	
	switch(whi_cmd)
	{
		case WHI_SEND_HPLC_FRAME:
			plc_head->plc_id = WHI_HPLC_ADAPT_SEND_HPLC_FRAME;
			break;
		case WHI_SEND_STA_FRAME:
			plc_head->plc_id = WHI_HPLC_ADAPT_SEND_STA_FRAME;
			break;
		case WHI_SEND_STA_NO_ACK_FRAME:
			plc_head->plc_id = WHI_HPLC_ADAPT_SEND_HPLC_NO_ACK_FRAME;
			break;
		case WHI_SEND_BUS_TRAN_FRAME:
			plc_head->plc_id = WHI_HPLC_ADAPT_SEND_TRAN_FRAME;
			break;
		case WHI_PLC_DIAGNOSIS:
			plc_head->plc_id = WHI_HPLC_ADAPT_DIAGNOSIS_FRAME;
			break;
		default:
			plc_head->plc_id = WHI_HPLC_ADAPT_SEND_BUS_FRAME;
			break;
	}

	plc_head->ctl = 0x08;
	/*
	if((whi_cmd == WHI_SEND_BUS_FRAME) &&
		((0 == dsp_memcmp(p_src,brocast_addr,LC_NET_ADDR_SIZE)) || 
		(0 == dsp_memcmp(p_src,local_brocast_addr,LC_NET_ADDR_SIZE))))
	{
		plc_head->broadcast_seq = lc_net_GetCurrentBroadcastSeq();
	}
	else
	*/
	{
		plc_head->broadcast_seq = 0x00;
	}
	plc_head->user_len = len;
	memcpy(plc_head->user_data,p_src,len);
	dlen = sizeof(whi_hplc_adapt_head_t) + len;
	
	return 	dlen;
}


/**
 * [whi_hplc_adapt_process    whi处理]
 * @author    ginvc
 * @dateTime  2022-10-16
 * @attention none
 * @param     whi_cmd      []
 * @return    index        [报文起始位相较于buffer起始位的指针偏移量]    
 */
uint8_t* whi_hplc_adapt_process(uint32_t *whi_cmd,uint8_t *p_src,uint32_t *len)
{
	uint32_t dlen = *len;
	if((p_src == NULL)||(dlen ==0))return NULL;
	
	whi_hplc_adapt_head_t *plc_head = (whi_hplc_adapt_head_t*)p_src;
	
	if(plc_head->port != 0x11)return NULL;
	if(plc_head->ctl != 0x08)return NULL;
	//if(plc_head->plc_id != 0x55)return NULL;
	if(dlen < (WHI_HPLC_ADAPT_HEAD_LEN + plc_head->user_len))return NULL;
	
	switch(plc_head->plc_id)
	{
		case WHI_HPLC_ADAPT_SEND_HPLC_FRAME:
			*whi_cmd = WHI_RECEIVE_HPLC_FRAME;
			break;
		case WHI_HPLC_ADAPT_SEND_STA_FRAME:
			*whi_cmd = WHI_RECEIVE_STA_FRAME;
			break;
		case WHI_HPLC_ADAPT_SEND_HPLC_NO_ACK_FRAME:
			*whi_cmd = WHI_RECEIVE_STA_NO_ACK_FRAME;
			break;
		case WHI_HPLC_ADAPT_SEND_TRAN_FRAME:
			*whi_cmd = WHI_SEND_BUS_TRAN_FRAME;
			break;
		case WHI_HPLC_ADAPT_DIAGNOSIS_FRAME:
			*whi_cmd = WHI_PLC_DIAGNOSIS;
			break;
		default:
			*whi_cmd = WHI_SEND_BUS_FRAME;
		break;
	}
	
	*len = plc_head->user_len;
	//dsp_memcpy(p_dst,plc_head->user_data,dlen);
	return 	&p_src[WHI_HPLC_ADAPT_HEAD_LEN];
}




















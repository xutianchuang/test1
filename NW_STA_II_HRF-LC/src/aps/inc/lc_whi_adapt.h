#ifndef __LC_WHI_ADAPT_H__
#define __LC_WHI_ADAPT_H__

#include <stdint.h>
#include  <gsmcu_hal.h>
//
#ifndef   __PACKED
  #define __PACKED                               __attribute__((packed))
#endif
typedef struct __PACKED
{
	uint8_t port; //固定 0x11
	uint16_t plc_id; //120H->0x55
	uint8_t ctl;//固定 0x08
	uint16_t crc;//固定 0x00
	uint32_t broadcast_seq;
	uint16_t user_len;
	uint8_t user_data[];
}whi_hplc_adapt_head_t;
//


#define WHI_HPLC_ADAPT_HEAD_LEN		(sizeof(whi_hplc_adapt_head_t))
#define WHI_HPLC_ADAPT_HEAD_START_SIZE	(sizeof(whi_hplc_adapt_head_t))

#define WHI_HPLC_ADAPT_SEND_HPLC_FRAME            (0x0050)
#define WHI_HPLC_ADAPT_SEND_STA_FRAME             (0x0051)
#define WHI_HPLC_ADAPT_SEND_BUS_FRAME             (0x0055)

#define WHI_HPLC_ADAPT_SEND_HPLC_NO_ACK_FRAME            (0x0150)
#define WHI_HPLC_ADAPT_SEND_TRAN_FRAME             (0x0155)

#define LC_NET_ADDR_SIZE 6

#define WHI_SEND_BUS_FRAME             (0x0120)

uint32_t whi_hplc_adapt_create(uint32_t whi_cmd,uint8_t *p_dst,uint8_t *p_src,uint32_t len);
uint8_t* whi_hplc_adapt_process(uint32_t *whi_cmd,uint8_t *p_src,uint32_t *len);

#endif
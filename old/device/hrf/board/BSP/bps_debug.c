/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_debug.h"
#include "os.h"
#include <stdarg.h>
#include <stdio.h>

#include <string.h>
#include "phy_port.h"
#include "gsmcu_m3_usart.h"
#define MAX_DEBUG_BUFFER (8192)
#define MAX_DEBUG_TEMP   (512)


static u8 debug_buffer[MAX_DEBUG_BUFFER];
static char debug_temp[MAX_DEBUG_TEMP];
static u32 debug_read = 0;
static u32 debug_write = 0;
static u32 debug_mask = DEBUG_LOG_INFO|DEBUG_LOG_ERR|DEBUG_LOG_NET|DEBUG_LOG_APS|DEBUG_LOG_MAC|DEBUG_LOG_APP|DEBUG_LOG_UPDATA|DEBUG_LOG_ZONE|DEBUG_LOG_ROUT|DEBUG_LOG_METER;


void Debug_UartIRQ(uint32_t);
void DebugOpen(uint32_t BaudRate)
{

	DEBUG_UART.Initialize(Debug_UartIRQ);
	DEBUG_UART.PowerControl(ARM_POWER_FULL);
	DEBUG_UART.Control(ARM_USART_MODE_ASYNCHRONOUS |
						  ARM_USART_DATA_BITS_8 |
						  ARM_USART_PARITY_NONE |
						  ARM_USART_STOP_BITS_1 |
						  ARM_USART_FLOW_CONTROL_NONE, BaudRate);
	DEBUG_UART.Control(ARM_USART_CONTROL_TX, 1);
	DEBUG_UART.Control(ARM_USART_CONTROL_RX, 1);
	debug_str(DEBUG_LOG_TEMP,"Debug Is Opened.\r\n");

}

void DebugSetMask(uint32_t mask)
{
	debug_mask = mask;
}




void debug_uart_send(void)
{

	if (DEBUG_UART.GetStatus().tx_busy)
	{
		return;
	}
	if (debug_write != debug_read)
	{
		u32 this_send_cnt = 0;
		if (debug_write > debug_read)
		{
			this_send_cnt = debug_write - debug_read;
		}
		else
		{
			this_send_cnt = MAX_DEBUG_BUFFER - debug_read;
		}
		DEBUG_UART.Send(&debug_buffer[debug_read], this_send_cnt);
	}

}
void put_temp2buffer_send(int size)
{
	u32 remain_cnt = (debug_write - debug_read + MAX_DEBUG_BUFFER) % MAX_DEBUG_BUFFER;
	remain_cnt = MAX_DEBUG_BUFFER - 1 - remain_cnt;

	if (size < remain_cnt) //当前buffer可以放入
	{
		if (size > (MAX_DEBUG_BUFFER - debug_write))
		{
			int right_len = MAX_DEBUG_BUFFER - debug_write;
			memcpy(&debug_buffer[debug_write], debug_temp, right_len);
			memcpy(debug_buffer, &debug_temp[right_len], size - right_len);
		}
		else
		{
			memcpy(&debug_buffer[debug_write], debug_temp, size);
		}
		debug_write += size;
		debug_write %= MAX_DEBUG_BUFFER;
		debug_uart_send();
	}

}

void debug_hex(u32 mask, uint8_t *buf, uint16_t len)
{

	if (!(debug_mask & mask))
	{
		return;
	}
	int p = 0;
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
	if (len > MAX_DEBUG_TEMP / 3)
	{
		CPU_CRITICAL_EXIT();
		return;
	}
	for (uint16_t i = 0; i < len; i++)
	{
		sprintf(&debug_temp[p], "%02x  ", buf[i]);
		p += 3;
	}
	debug_temp[p - 1] = '\r';
	debug_temp[p++] = '\n';
	debug_temp[p++] = 0;
	put_temp2buffer_send(p);
	CPU_CRITICAL_EXIT();

}
void debug_str(u32 mask, const char *format, ...)
{

	uint16_t head_len = 9;
	uint16_t body_len = MAX_DEBUG_TEMP - head_len;
	if (!(debug_mask & mask))
	{
		return;
	}
	u32 cur_ntb = BPLC_GetNTB();

	va_list pArgs;

	va_start(pArgs, format);

	//vsprintf(buffer, format, pArgs);

	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
	sprintf(debug_temp, "%08x: ", cur_ntb);

	int size = vsnprintf(&debug_temp[head_len], body_len, format, pArgs);
	if (size < 0)
	{
		CPU_CRITICAL_EXIT();
		return;
	}
	size += head_len;

	va_end(pArgs);

	put_temp2buffer_send(size);
	CPU_CRITICAL_EXIT();

}

void Debug_UartIRQ(uint32_t event)
{

	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
	if (ARM_USART_EVENT_SEND_COMPLETE & event)
	{
		debug_read += DEBUG_UART.GetTxCount();
		debug_read %= MAX_DEBUG_BUFFER;

		if (debug_read != debug_write)
		{
			debug_uart_send();
		}
	}
	CPU_CRITICAL_EXIT();

}

#ifdef __DEBUG_MODE
typedef struct {
	uint32_t COMP;                  /*!< Offset: 0x020 (R/W)  Comparator Register 0 */
	uint32_t MASK;                  /*!< Offset: 0x024 (R/W)  Mask Register 0 */
	uint32_t FUNCTION;              /*!< Offset: 0x028 (R/W)  Function Register 0 */
	uint32_t RESERVED[1U];
}DWT_Array_s;
struct{
	u32 addr;
	u8 vaild;
}dwt_info[4];
void debug_wdt_init(void)
{
	CoreDebug->DEMCR|=CoreDebug_DEMCR_TRCENA_Msk|CoreDebug_DEMCR_MON_EN_Msk;
}
int debug_at_addr(void *addr, u32 size, u8 mode)
{
	DWT_Array_s *p = (DWT_Array_s *)&DWT->COMP0;

	for (int i = 3; i >= 0; i--)
	{
		if (!dwt_info[i].vaild)
		{
			u32 mask = 0;
			int j;
			p[i].FUNCTION = 0; //清楚上一状态
			for (j = 0; j < 4; j++)
			{
				mask <<= 1;
				mask |= 1;
				if (size <= (1 << j))
				{
					break;
				}

			}
			if (j == 4)
			{
				break;
			}
			p[i].COMP = (uint32_t)addr & (~mask);
			p[i].MASK = j;
			p[i].FUNCTION = (0 << DWT_FUNCTION_DATAVMATCH_Pos) | (mode << DWT_FUNCTION_FUNCTION_Pos);
			dwt_info[i].vaild = 1;
			dwt_info[i].addr = (u32)addr;
			return 1;
		}
	}
	return 0;
}
void debug_no_addr(void *addr)
{
        DWT_Array_s *p = (DWT_Array_s *)&DWT->COMP0;
	for (int i = 3; i >= 0; i--)
	{
		if (dwt_info[i].vaild && dwt_info[i].addr == (uint32_t)addr)
		{
			dwt_info[i].vaild = 0;
			p[i].FUNCTION = 0;
		}
	}
}
int debug_data_match(void *addr, u32 size, u8 mode,u8 match_size,u32 vaile)
{
	if (dwt_info[0].vaild || dwt_info[1].vaild)
	{
		return 0;
	}
	u32 mask = 0;
	int j;

	for (j = 0; j < 4; j++)
	{
		mask <<= 1;
		mask |= 1;
		if (size <= (1 << j))
		{
			break;
		}

	}
	if (j == 4)
	{
		return 0;
	}
	DWT->COMP0 = (uint32_t)addr & (~mask);
	DWT->MASK0 = j;
	DWT->FUNCTION0 =  0;
	DWT->COMP1 = vaile;
	DWT->MASK1 = 0;
	DWT->FUNCTION1 =  (1 << DWT_FUNCTION_DATAVMATCH_Pos) |  (match_size << DWT_FUNCTION_DATAVSIZE_Pos) | (mode << DWT_FUNCTION_FUNCTION_Pos);
	dwt_info[0].vaild = 2;
	dwt_info[1].vaild = 2;
        return 1;
}
void debug_no_data(void)
{
	if (dwt_info[0].vaild == 2 || dwt_info[1].vaild == 2)
	{
		dwt_info[0].vaild = 0;
		dwt_info[1].vaild = 0;
		DWT->FUNCTION0 = 0;
		DWT->FUNCTION1 = 0;
	}
}
#endif



#include "bps_ir_uart.h"
/*
 *
 */
#include <gsmcu_hal.h>
#include "os.h"
#include "protocol_includes.h"
#include "bps_ir_uart.h"
#include "bps_uart.h"

//===========

#define BPS_UART_IR_BUFF_LENGTH (1000)

static uint8_t BPS_UartIrRxIrqBuff[BPS_UART_IR_BUFF_LENGTH];
static uint8_t BPS_UartIrRxBuff[BPS_UART_IR_BUFF_LENGTH];

static uint32_t BPS_UartIrRxBuffReadIndex;
static uint32_t BPS_UartIrRxBuffWrtieIndex;

static uint32_t UartIrStatus = 0x00;

static UartCallBackFunctionType UartIrRxDataCallBackFunction = NULL;
static UartCallBackFunctionType UartIrTxDataCallBackFunction = NULL;

//
void IR_UART_Handler_ISR(uint32_t status);
//

void UartIrOpen(UartParameterType *UartParameter)
{
	IR_UART.Initialize(IR_UART_Handler_ISR);
	IR_UART.PowerControl(ARM_POWER_FULL);

	IR_UART.Control(ARM_USART_MODE_IRDA | ARM_USART_DATA_BITS_9 | ARM_USART_PARITY_EVEN | ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE,
					SET_USART_IRDA_MODE(1, 0, UartParameter->BaudRate));
	IR_UART.Control(ARM_USART_SET_IRDA_PULSE, 75988);

	IR_UART.Control(ARM_USART_CONTROL_TX, 1);
	IR_UART.Control(ARM_USART_CONTROL_RX, 1);

	UartIrRxDataCallBackFunction = UartParameter->RxHaveDataFunction;

	if (UartIrStatus == 0)
	{
		IR_UART.Receive(BPS_UartIrRxIrqBuff, BPS_UART_IR_BUFF_LENGTH);
	}
	UartIrStatus |= BPS_UART_OPEN_MASK;
}

void UartIrWrite(uint8_t *pbuff, uint32_t len, bool allow_receive)
{
	if (!len)
		return;
	if ((UartIrStatus & BPS_UART_OPEN_MASK) != BPS_UART_OPEN)
		return;
	if (IR_UART.GetStatus().tx_busy)
		return;
    
	IR_UART.Control(ARM_USART_CONTROL_RX, allow_receive?1:0);
	UartIrStatus |= BPS_UART_TRANSMITING;
	IR_UART.Send(pbuff, len);
}

bool IsIrUartTxData(void)
{
	if ((UartIrStatus & BPS_UART_OPEN_MASK) != BPS_UART_OPEN)
		return false;

	if ((UartIrStatus & BPS_UART_TRANSMITMASK) == BPS_UART_TRANSMITING)
		return true;

	return IR_UART.GetStatus().tx_busy ? true : false;
}

bool IsIrUartTxSendAllDone(void)
{
	return !IsIrUartTxData();
}

uint32_t UartIrRead(uint8_t *pbuff, uint32_t len)
{
	if ((UartIrStatus & BPS_UART_OPEN_MASK) != BPS_UART_OPEN)
		return 0;

	if (len == 0)
	{
		len = BPS_UART_IR_BUFF_LENGTH;
	}

	return popFifo(&BPS_UartIrRxBuffReadIndex, &BPS_UartIrRxBuffWrtieIndex,
				   BPS_UartIrRxBuff, BPS_UART_IR_BUFF_LENGTH,
				   pbuff, len);
}

void UartIrFlushBuffer(void)
{
	u8 temp[5];

	while (UartIrRead(temp, sizeof(temp)))
	{
	}
}



static void IRuartTimeIrq(void);

static const TimerParameterTypdef UartTimerParam = {
	IR_TIMER,
	50 * 1000 * 100,
	IRuartTimeIrq,
};


static void IRuartTimeIrq(void)
{
#if defined(IR_UART)
	int cnt = getFifoCnt(&BPS_UartIrRxBuffReadIndex, &BPS_UartIrRxBuffWrtieIndex, BPS_UART_IR_BUFF_LENGTH);
	if (IR_UART.GetRxCount() == 0) // 经过了超时之后没有再收到新的字节
	{
		if (UartIrRxDataCallBackFunction != NULL)
		{
			UartIrRxDataCallBackFunction(cnt);
		}
	}
	TimerClose(UartTimerParam.Timer);
#endif
}


void IR_UART_Handler_ISR(uint32_t status)
{
	if ((ARM_USART_EVENT_RECEIVE_COMPLETE | ARM_USART_EVENT_RX_TIMEOUT) & status) // 接收完成或空闲状态
	{
		u32 rx_cnt = IR_UART.GetRxCount();

		if (false == pushFifo(&BPS_UartIrRxBuffReadIndex, &BPS_UartIrRxBuffWrtieIndex,
							  BPS_UartIrRxBuff, BPS_UART_IR_BUFF_LENGTH, BPS_UartIrRxIrqBuff, rx_cnt))
		{
			int cnt = getFifoCnt(&BPS_UartIrRxBuffReadIndex, &BPS_UartIrRxBuffWrtieIndex, BPS_UART_IR_BUFF_LENGTH);
			if (cnt && (UartIrRxDataCallBackFunction != NULL))
			{
				UartIrRxDataCallBackFunction(cnt);
			}
		}
		else
		{
			// 开启timer等待超时
			TimerOpen((TimerParameterTypdef *)&UartTimerParam);
			TimerWrite(UartTimerParam.Timer, TIMER_SET_START);
		}
		IR_UART.Receive(BPS_UartIrRxIrqBuff, BPS_UART_IR_BUFF_LENGTH);
	}

	if (ARM_USART_EVENT_SEND_COMPLETE & status) // 发送完成
	{
		UartIrStatus &= ~BPS_UART_TRANSMITING;

		if (UartIrTxDataCallBackFunction != NULL)
		{
			UartIrTxDataCallBackFunction(0);
		}
	}

	return;
}

// 串口数据使用DMA发送，用全局变量定义（地址不变）
static u8 ir_test_data[14] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0};

volatile u32 ir_uart_recv_cnt = 0;

static void IRUartUartIrTestRecv(int cnt)
{
	ir_uart_recv_cnt = cnt;

	return;
}

bool UartIrTest(const u8 *mac)
{
	OS_ERR err;
	u8 count = 0;
	u8 ir_test_read_data[14] = {0};
	
	// 暂存原有的接收处理函数
	UartCallBackFunctionType temp = UartIrRxDataCallBackFunction;
	UartIrRxDataCallBackFunction = IRUartUartIrTestRecv;
	
	UartIrFlushBuffer();

	// 发送自测数据
	// 自测数据增加自身入网mac地址，防止产测时多机台干扰
	memcpy(&ir_test_data[8], mac, 6);
	ir_uart_recv_cnt = 0;
	
	UartIrWrite(ir_test_data, sizeof(ir_test_data),true);

	while (IsIrUartTxData())
	{
		OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_DLY, &err);
	}

	while (++count < 100)
	{
		if (ir_uart_recv_cnt >= sizeof(ir_test_read_data))
		{
			UartIrRead(ir_test_read_data, sizeof(ir_test_read_data));
			break;
		}

		OSTimeDlyHMSM(0, 0, 0, 50, OS_OPT_TIME_DLY, &err);
	}

	// 恢复原有的接收处理函数
	UartIrRxDataCallBackFunction = temp;

	if (memcmp(ir_test_data, ir_test_read_data, sizeof(ir_test_data)) == 0)
	{
		return true;
	}

	return false;
}


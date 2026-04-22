/*
*
*/
#include <string.h>
#include  <gsmcu_hal.h>
#include "bps_uart.h"
#include "os.h"
#include "bps_timer.h"
#include "protocol_includes.h"


#include "bps_uart_lcip.h"

//===========
#define LCIP_BPS_UART_BUFF_LENGTH        (2100)
#define UART_RX_CCO_STREAM_BUFFER  1024
stream_buffer_t uart_rxstream;
__attribute__((aligned(4))) unsigned char uart_recv_stream_buffer[UART_RX_CCO_STREAM_BUFFER];

static uint32_t UartStatus = 0x00;

static uint32_t BPS_UartRxBuffReadIndex;
static uint32_t BPS_UartRxBuffWrtieIndex;

static UartCallBackFunctionType UartTxCallBackFunction = NULL;
static UartCallBackFunctionType UartRxDataCallBackFunction= NULL;

static uint8_t BPS_UartIrqBuff[LCIP_BPS_UART_BUFF_LENGTH];
static void LCIP_UART_Handler_ISR(uint32_t event);

static void uartTimeIrq(void);

static const TimerParameterTypdef UartTimerParam={
	USART_TIMER,
	50*1000*100,
	uartTimeIrq,
};

void LcipUartOpen(UartParameterType *UartParameter)
{
#if defined(LCIP_UART)

    stream_buffer_init(&uart_rxstream, uart_recv_stream_buffer, UART_RX_CCO_STREAM_BUFFER);

	//zb204的开漏输出改到RTE_Device.h里进行定义
	LCIP_UART.Initialize(LCIP_UART_Handler_ISR);
	LCIP_UART.PowerControl(ARM_POWER_FULL);
	u32 control=ARM_USART_DATA_BITS_9 | UartParameter->Parity;
	if (UartParameter->Parity==ARM_USART_PARITY_NONE)
	{
		control = ARM_USART_DATA_BITS_8 | UartParameter->Parity;
	}
	LCIP_UART.Control(ARM_USART_MODE_ASYNCHRONOUS |
						  control |//对于本模块奇偶校验也算一个数据位
						  ARM_USART_STOP_BITS_1 |
						  ARM_USART_FLOW_CONTROL_NONE, UartParameter->BaudRate);
	LCIP_UART.Control(ARM_USART_CONTROL_TX, 1);
	LCIP_UART.Control(ARM_USART_CONTROL_RX, 1);
	
    UartTxCallBackFunction = UartParameter->TxDataCompleteFunction;
    UartRxDataCallBackFunction= UartParameter->RxHaveDataFunction;
    //UartRxErrorCallBackFunction= UartParameter->RxParityErrorFunction;
    //
	if (UartStatus==0)
	{
		LCIP_UART.Receive(BPS_UartIrqBuff, LCIP_BPS_UART_BUFF_LENGTH);
	}
	UartStatus |= BPS_UART_OPEN_MASK;

#endif
}

void LcipUartWrite(uint8_t *pbuff,uint32_t len)
{
#if defined(LCIP_UART) 

    if(len < 1) return;
	LCIP_UART.Control(ARM_USART_CONTROL_RX, 0);
	GPIO_PinWrite(1,6,0);
    if((UartStatus&BPS_UART_OPEN_MASK) != BPS_UART_OPEN) return;
	UartStatus |= BPS_UART_TRANSMITING;
	LCIP_UART.Send(pbuff, len);
#endif
}
void LcipUartClose()
{
	;
}

bool LcipIsUartTxData()
{
    if((UartStatus&BPS_UART_OPEN_MASK) != BPS_UART_OPEN) return false;
    if((UartStatus& BPS_UART_TRANSMITMASK)== BPS_UART_TRANSMITING) return true;
    return false;
}

bool LcipIsUartTxSendAllDone()
{
	return !LcipIsUartTxData();
}


bool LcipIsUartOpen()
{
    if((UartStatus&BPS_UART_OPEN_MASK) != BPS_UART_OPEN) return false;
    return true;
}

//extern void UART_ExernExe(void);
bool LcipIsUartRxing(void)
{
    return LCIP_UART.GetRxCount();
}

static void uartTimeIrq(void)
{
	int cnt =getFifoCnt(&BPS_UartRxBuffReadIndex, &BPS_UartRxBuffWrtieIndex, LCIP_BPS_UART_BUFF_LENGTH);
	if (LCIP_UART.GetRxCount()==0)//经过了超时之后没有再收到新的字节
	{
		if (UartRxDataCallBackFunction != NULL)
		{
			UartRxDataCallBackFunction(cnt);
		}
	}
	TimerClose(UartTimerParam.Timer);
}

// 串口发送事件回调函数
static void LCIP_UART_Handler_ISR(uint32_t event)
{
	if ((ARM_USART_EVENT_RECEIVE_COMPLETE | ARM_USART_EVENT_RX_TIMEOUT) & event) //接收完成或者进入空闲状态
	{
		u32 rx_cnt = LCIP_UART.GetRxCount();

		// if (false == pushFifo(&BPS_UartRxBuffReadIndex, &BPS_UartRxBuffWrtieIndex,
		// 					  BPS_UartRxBuff, LCIP_BPS_UART_BUFF_LENGTH, BPS_UartIrqBuff,rx_cnt))
		if(0 == stream_buffer_write(&uart_rxstream, BPS_UartIrqBuff,rx_cnt))
		{
			if (UartRxDataCallBackFunction != NULL)
			{
				UartRxDataCallBackFunction(rx_cnt);
			}
		}
		else
		{
			//开启timer等待超时
			TimerOpen((TimerParameterTypdef *)&UartTimerParam);
			TimerWrite(UartTimerParam.Timer,TIMER_SET_START);
		}
		// 再次配置接收BUFF
		LCIP_UART.Receive(BPS_UartIrqBuff,LCIP_BPS_UART_BUFF_LENGTH);
	}

	if (event & ARM_USART_EVENT_SEND_COMPLETE) //发送完成
	{
		UartStatus &= ~BPS_UART_TRANSMITMASK;

		if (UartTxCallBackFunction != NULL)
		{
			UartTxCallBackFunction(0);
		}
	}
}














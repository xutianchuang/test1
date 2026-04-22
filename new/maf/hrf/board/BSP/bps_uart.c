/*
*
*/
#include <string.h>
#include  <gsmcu_hal.h>
#include "bps_uart.h"
#include "os.h"
#include "bps_timer.h"
/*
*
*/
#define BPS_UART_OPEN                  (1)
#define BPS_UART_OPEN_MASK             (1)

#define BPS_UART_HAVEDATA               (0x01 <<4)
#define BPS_UART_HAVEDATA_MASK          (0x01 <<4)

#define BPS_UART_TRANSMITING            (0x01 <<8)
#define BPS_UART_TRANSMITMASK           (0x01 <<8)
//===========


#define BPS_UART_BUFF_LENGTH        (1024)
#define BPS_UART_IRQ_BUFF_LENGTH        (512)

static uint8_t BPS_UartIrqBuff[2][BPS_UART_IRQ_BUFF_LENGTH];
static uint8_t BPS_UartRxBuff[BPS_UART_BUFF_LENGTH];

static uint32_t BPS_UartRxBuffReadIndex;
static uint32_t BPS_UartRxBuffWrtieIndex;

static apDMA_sPerMemLLI sUartRxLLI[2];
//串口使用最后一组timer来进行串口超时判断
#define UART_USE_TIME (BSP_TIMER_CHL_LENGTH-1)



int getFifoCnt(uint32_t * ri,uint32_t * wi,uint32_t buffsize)
{
	return (*wi - *ri + buffsize) % buffsize;
}

bool pushFifo(uint32_t * ri,uint32_t * wi,uint8_t *pbuff,uint32_t buffsize,uint8_t *data,uint32_t len)
{
	u16 remain_cnt = (*wi - *ri + buffsize) % buffsize;
	remain_cnt = buffsize - 1 - remain_cnt;
	bool pushOk = true;
	if (len > remain_cnt) //当前buffer可以放入
	{
		pushOk = false;
		len = remain_cnt;
	}


	if (len > (buffsize - *wi))
	{
		int right_len=buffsize - *wi;
		memcpy(&pbuff[*wi], data, right_len);
		memcpy(pbuff, &data[right_len], len - right_len);
	}
	else
	{
		memcpy(&pbuff[*wi], data, len);
	}
	*wi += len;
	*wi %= buffsize;
	return pushOk;
}





uint32_t popFifo(uint32_t * ri,uint32_t * wi,uint8_t *pbuff,uint32_t buffsize,uint8_t *pdst,uint32_t poplen)
{
	uint32_t total_cnt=0;
    while (* wi != * ri)
	{
		u16 this_send_cnt = 0;
		if (*wi > *ri)
		{
			this_send_cnt = *wi - *ri;
		}
		else
		{
			this_send_cnt = buffsize - *ri;
		}
		if (this_send_cnt==0)
		{
			return total_cnt;
		}
		else if (this_send_cnt > poplen)
		{
			this_send_cnt = poplen;
			memcpy(&pdst[total_cnt], &pbuff[*ri], this_send_cnt);
			total_cnt += this_send_cnt;
			*ri+=this_send_cnt;
			*ri %= buffsize;
			return total_cnt;
		}
		else
		{
			memcpy(&pdst[total_cnt], &pbuff[*ri], this_send_cnt);
			*ri+=this_send_cnt;
			*ri %= buffsize;
			total_cnt += this_send_cnt;
		}
	}
	return total_cnt;
}

//==============
//static bool UartInInterruptIsp = false;


static uint32_t UartStatus = 0x00;


static UartCallBackFunctionType UartTxCallBackFunction = NULL;
static UartCallBackFunctionType UartRxDataCallBackFunction= NULL;
//static UartCallBackFunctionType UartRxErrorCallBackFunction= NULL;
//
static void METER_UART_Handler_ISR(uint32_t event);

//
void UartOpen(UartParameterType *UartParameter)
{
#if defined(METER_UART)

	//zb204的开漏输出改到RTE_Device.h里进行定义
	METER_UART.Initialize(METER_UART_Handler_ISR);
	METER_UART.PowerControl(ARM_POWER_FULL);
	u32 control=ARM_USART_DATA_BITS_9 | UartParameter->Parity;
	if (UartParameter->Parity==ARM_USART_PARITY_NONE)
	{
		control = ARM_USART_DATA_BITS_8 | UartParameter->Parity;
	}
	METER_UART.Control(ARM_USART_MODE_ASYNCHRONOUS |
						  control |//对于本模块奇偶校验也算一个数据位
						  ARM_USART_STOP_BITS_1 |
						  ARM_USART_FLOW_CONTROL_NONE, UartParameter->BaudRate);
	METER_UART.Control(ARM_USART_CONTROL_TX, 1);
	METER_UART.Control(ARM_USART_CONTROL_RX, 1);
	METER_UART.Control(ARM_USART_CONTROL_RX_LLI, (uint32_t)sUartRxLLI);//启动链模式
	
    //
    UartTxCallBackFunction = UartParameter->TxDataCompleteFunction;
    UartRxDataCallBackFunction= UartParameter->RxHaveDataFunction;
    //UartRxErrorCallBackFunction= UartParameter->RxParityErrorFunction;
    //
	if (UartStatus==0)
	{
		METER_UART.Receive(BPS_UartIrqBuff, BPS_UART_IRQ_BUFF_LENGTH);
	}
	UartStatus |= BPS_UART_OPEN_MASK;

#endif
}


void UartWrite(uint8_t *pbuff,uint32_t len)
{
#if defined(METER_UART) 

    if(len < 1) return;
    if((UartStatus&BPS_UART_OPEN_MASK) != BPS_UART_OPEN) return;
	UartStatus |= BPS_UART_TRANSMITING;
	METER_UART.Send(pbuff, len);
#endif
}
uint32_t UartRead(uint8_t *pbuff,uint32_t len)
{
#if defined(METER_UART)
    if((UartStatus&BPS_UART_OPEN_MASK) != BPS_UART_OPEN) return 0;
    if(len == 0)
    {
        len = BPS_UART_BUFF_LENGTH;
    }
    return popFifo(&BPS_UartRxBuffReadIndex,&BPS_UartRxBuffWrtieIndex,
                  BPS_UartRxBuff,BPS_UART_BUFF_LENGTH,
                  pbuff,len);
#else
	return -1;
#endif
}




void UartClose()
{
}

bool IsUartTxData()
{
#if defined(METER_UART)
    if((UartStatus&BPS_UART_OPEN_MASK) != BPS_UART_OPEN) return false;
    if((UartStatus& BPS_UART_TRANSMITMASK)== BPS_UART_TRANSMITING) return true;
#endif
    return false;
}



bool IsUartOpen()
{
#if defined(METER_UART)
    if((UartStatus&BPS_UART_OPEN_MASK) != BPS_UART_OPEN) return false;
    return true;
#else
	return false;
#endif
}

//extern void UART_ExernExe(void);
bool IsUartRxing(void)
{
#if defined(METER_UART)
    return METER_UART.GetRxCount();
#else
    return false;
#endif
}




static void METER_UART_Handler_ISR(uint32_t event)
{

	static uint32_t uart_idx;
#if defined(METER_UART)
	//UartInInterruptIsp = true;

	if ((ARM_USART_EVENT_RECEIVE_NO_DATA_COMPLETE & event) != ARM_USART_EVENT_RECEIVE_NO_DATA_COMPLETE)//有数据进行处理
	{
		if ((ARM_USART_EVENT_RECEIVE_COMPLETE | ARM_USART_EVENT_RX_TIMEOUT |ARM_USART_EVENT_RECEIVE_HALF_COMPLETE) & event) //接收完成或者进入空闲状态
		{
			u32 rx_cnt = METER_UART.GetRxCount();
			
			uint8_t *desBuffer = BPS_UartIrqBuff[0];
			if (ARM_USART_EVENT_RECEIVE_COMPLETE&event)
			{
				desBuffer=BPS_UartIrqBuff[1];
			}
			
			u32 rx_len = rx_cnt - uart_idx;
			if (false == pushFifo(&BPS_UartRxBuffReadIndex, &BPS_UartRxBuffWrtieIndex,
								  BPS_UartRxBuff, BPS_UART_BUFF_LENGTH, &desBuffer[uart_idx], rx_len))
			{
				if (UartRxDataCallBackFunction != NULL)
				{
					UartRxDataCallBackFunction(rx_len);
				}
			}
			else
			{
				if (UartRxDataCallBackFunction != NULL)
				{
					UartRxDataCallBackFunction(rx_len);
				}
			}
			
			//更新buffer起始buffer
			if (rx_cnt!=BPS_UART_IRQ_BUFF_LENGTH)
			{
				uart_idx=rx_cnt;
			}
			else
			{
				uart_idx=0;
			}
			//METER_UART.Receive(BPS_UartIrqBuff,BPS_UART_IRQ_BUFF_LENGTH);
		}
	}

	if (event & ARM_USART_EVENT_SEND_COMPLETE) //发送完成
	{
		UartStatus &= ~BPS_UART_TRANSMITMASK;

		if (UartTxCallBackFunction != NULL)
		{
			UartTxCallBackFunction(0);
		}
	}
#endif
}














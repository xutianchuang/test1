#include "finsh_device.h"

static u8 CurrentChar = 0;
#if defined(FINSH_UART)
static pFinshCharInfoFun pCharInfoFun = NULL;
#endif

void Finsh_Handle_ISR(void);


//打印口初始化
void FinshDeviceInit(u32 baud)
{
#if defined(FINSH_UART)
	PinRemapConfig(FINSH_UART_TX_REMAP,ENABLE);
	PinRemapConfig(FINSH_UART_RX_REMAP,ENABLE);
	SCU_PeriphInputConfig(FINSH_UART_INPUT_ENABLE);
	SCU_GPIOInputConfig(FINSH_UART_TX_PORT, FINSH_UART_TX_PIN);
	SCU_GPIOInputConfig(FINSH_UART_RX_PORT, FINSH_UART_RX_PIN);
	
	USART_InitTypeDef USART_InitStruct;
	USART_InitStruct.USART_BaudRate = baud;
	USART_InitStruct.USART_WordLength_StopBits = USART_8Bit_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	
	USART_InitStruct.USART_Fifo_Config = USART_FIFO_CONFIG_NONE;//not use fifo
	
	USART_Init(FINSH_UART,&USART_InitStruct);
	//enable interrupt  
	USART_ConfigInterruptEnable(FINSH_UART,USART_RECEIVE_LINE_INTER_ENABLE|USART_RECEIVE_DATA_INTER_ENABLE);
	
	BSP_IntVectSet(FINSH_IRQn,Finsh_Handle_ISR);
	NVIC_ClearPendingIRQ(FINSH_IRQn);
	NVIC_EnableIRQ(FINSH_IRQn);
#endif
}

//提供一个获取接口
u8  FinshGetChar(void)
{
    return CurrentChar;
}

//设置一个数据通知回调函数
void FinshCharCallback(pFinshCharInfoFun fun)
{
#if defined(FINSH_UART)
    pCharInfoFun = fun;
#endif
}

//重定向c库函数printf到USART0
int fputc(int ch, FILE *f)
{
#if defined(FINSH_UART)
    /* 发送一个字节数据到USART1 */
    USART_SendData(FINSH_UART, (uint8_t) ch);
    /* 等待发送完毕 */
    while(USART_GetStatus(FINSH_UART,USART_TRANSMITTER_EMPTY_STATUS) == RESET);
    return (ch);
#else
	return 0;
#endif
}

void Finsh_Handle_ISR(void)
{
#if defined(FINSH_UART)
    //UartInInterruptIsp = true;
    switch(USART_GetITAllStatus(FINSH_UART))
    {
        case USART_RECEIVE_DATA_READY_INTER_STATUS:
        {
            CurrentChar = USART_ReceiveData(FINSH_UART);
            if(pCharInfoFun)
                pCharInfoFun();
        }
        break;
        case USART_RECEIVE_LINE_INTER_STATUS:
            USART_GetAllStatus(FINSH_UART);//read clear
        break;

        default:
        break;
    }
#endif
}

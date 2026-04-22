/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_debug.h"
#include "os.h"
#include <stdarg.h>
#include <stdio.h>
#if DEV_STA
#include <string.h>
#include "phy_port.h"
#define MAX_DEBUG_BUFFER (1024*20)
#define MAX_DEBUG_TEMP   (2*4*(520+16))
#elif DEV_CCO
#include "system_inc.h"
#ifdef HPLC_CSG
#define MAX_DEBUG_BUFFER (1024*2)
#define MAX_DEBUG_TEMP   (2*4*(520+16))
#else
#define MAX_DEBUG_BUFFER (1024*20)
#define MAX_DEBUG_TEMP   (2*4*(520+16))
#endif
#endif
#ifdef DEBUG_UART
static u8 debug_buffer[MAX_DEBUG_BUFFER];
static char debug_temp[MAX_DEBUG_TEMP];
static u8 dma_sending = 0;
static u16 debug_read = 0;
static u16 debug_cur_send_cnt = 0;
static u16 debug_write = 0;
static u16 remain_cnt = MAX_DEBUG_BUFFER;
static u32 debug_mask = DEBUG_MASK_BIT;
static u32 debug_mask_later = DEBUG_MASK_BIT; //用于维护调试，用调试口下发命令时需要延后等切回日志模式再设置
#endif

#if DEV_CCO
#include "system_inc.h"
#endif
static OS_TICK_64 last_uart_tick;
static UartCallBackFunctionType UartTxCallBackFunction = NULL;
static DebugUartRxDataCallBackFunction UartRxDataCallBackFunction= NULL;
void Debug_UartIRQ(void);
void Debug_UartRecIRQ(void);
void DebugOpen(uint32_t BaudRate,UartCallBackFunctionType txCallback,DebugUartRxDataCallBackFunction rxCallback)
{
#if defined(DEBUG_UART)
    PinRemapConfig(DEBUG_UART_TX_PIN_REMAP, ENABLE);
    PinRemapConfig(DEBUG_UART_RX_PIN_REMAP, ENABLE);
    SCU_PeriphInputConfig(DEBUG_UART_INPUT_ENABLE);
    SCU_GPIOInputConfig(DEBUG_UART_TX_PORT, DEBUG_UART_TX_PIN);
    SCU_GPIOInputConfig(DEBUG_UART_RX_PORT, DEBUG_UART_RX_PIN);
    SCU_SetUsartMode(DEBUG_UART, USART_DONT_INVERT38K_OUTPUT |
                     USART_OPEN_DRAIN_DISABLE |
                     USART_SELECT_NORMAL__USART_MODE | USART_IO_FAST |
                     USART_IO_DRIVER_12MA);
    GPIO_InitTypeDef GPIO_InitStruct;
    //GPIO引脚配置
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin = DEBUG_UART_RX_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DEBUG_UART_RX_PORT, &GPIO_InitStruct);

    USART_InitTypeDef USART_InitStruct;
    USART_InitStruct.USART_BaudRate = BaudRate;
    USART_InitStruct.USART_WordLength_StopBits = USART_8Bit_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Fifo_Config = USART_RXFIFO_THR_LEVEL_3 | USART_TXFIFO_THR_LEVEL_0 |
        USART_RXFIFO_CLEAR | USART_TXFIFO_CLEAR | USART_FIFO_ENABLE; //use fifo
//    USART_InitStruct.USART_Fifo_Config = USART_FIFO_CONFIG_NONE; //not use fifo
    USART_InitStruct.USART_DMA_Flag = ENABLE;
    USART_Init(DEBUG_UART, &USART_InitStruct);

    USART_ConfigInterruptEnable(DEBUG_UART,USART_RECEIVE_LINE_INTER_ENABLE|USART_RECEIVE_DATA_INTER_ENABLE);
    BSP_IntVectSet(DEBUG_UART_IRQn,Debug_UartRecIRQ);


    BSP_InitDmacVector();
    ReSet_DMA_Channel(DMA, DEBUG_UART_DMA_CHL);
    BSP_IntDmacChannelVectSet(DEBUG_UART_DMA_CHL, Debug_UartIRQ);
    UartTxCallBackFunction = txCallback;
    UartRxDataCallBackFunction = rxCallback;

    NVIC_ClearPendingIRQ(DEBUG_UART_IRQn);
    NVIC_EnableIRQ(DEBUG_UART_IRQn);
    debug_str(DEBUG_LOG_NET, "Debug Is Opened.\r\n");
#endif
}

void DebugSetMask(uint32_t mask)
{
#ifdef DEBUG_UART
	debug_mask_later = mask;
    if (!last_uart_tick) //不在响应上位机模式才能直接设置，否则延后设置
	{
		debug_mask = mask;
	}
#endif
}

u32 DebugGetMaskLen(void)
{
#ifdef DEBUG_UART
    return sizeof(debug_mask);
#else
    return 0;
#endif
}

u32 DebugGetMask(void)
{
#ifdef DEBUG_UART
    return debug_mask;
#else
    return 0;
#endif
}

u32 DebugGetMaskLater(void)
{
#ifdef DEBUG_UART
    return debug_mask_later;
#else
    return 0;
#endif
}

void debug_uart_send(void)
{
#ifdef DEBUG_UART
    u16 read_remain = MAX_DEBUG_BUFFER - remain_cnt;
    if (remain_cnt == MAX_DEBUG_BUFFER)
    {
        dma_sending = 0;
        return;
    }
    if (read_remain >= (MAX_DEBUG_BUFFER - debug_read))
    {
        debug_cur_send_cnt = MAX_DEBUG_BUFFER - debug_read;
        UartWriteWithDma(DEBUG_UART, &debug_buffer[debug_read], debug_cur_send_cnt, DEBUG_UART_DMA_CHL);
        debug_read = 0;
    }
    else
    {
        debug_cur_send_cnt = read_remain;
        UartWriteWithDma(DEBUG_UART, &debug_buffer[debug_read], debug_cur_send_cnt, DEBUG_UART_DMA_CHL);
        debug_read = debug_read + read_remain;
    }
    dma_sending = 1;
#endif
}
void put_temp2buffer_send(int size)
{
#ifdef DEBUG_UART

    if (size < remain_cnt) //当前buffer可以放入
    {
        if (size > (MAX_DEBUG_BUFFER - debug_write))
        {
            memcpy(&debug_buffer[debug_write], debug_temp, MAX_DEBUG_BUFFER - debug_write);
            memcpy(debug_buffer, &debug_temp[MAX_DEBUG_BUFFER - debug_write], size - (MAX_DEBUG_BUFFER - debug_write));
        }
        else
        {
            memcpy(&debug_buffer[debug_write], debug_temp, size);
        }
        debug_write += size;
        debug_write %= MAX_DEBUG_BUFFER;
        remain_cnt -= size;
        if (dma_sending == 0)
        {
            debug_uart_send();
        }
    }
	else
	{
		remain_cnt = (debug_write - debug_read + MAX_DEBUG_BUFFER) % MAX_DEBUG_BUFFER;
		remain_cnt = MAX_DEBUG_BUFFER - 1 - remain_cnt;
		
	}

#endif    
}

void debug_hex(u32 mask, uint8_t *buf, uint16_t len)
{
#ifdef DEBUG_UART
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
#endif
}
void debug_str(u32 mask, const char *format, ...)
{
#ifdef DEBUG_UART
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
#endif
}

void Debug_UartIRQ(void)
{
#ifdef DEBUG_UART
    CPU_SR_ALLOC();
    CPU_CRITICAL_ENTER();
    dma_sending = 0;
    remain_cnt += debug_cur_send_cnt;
    debug_cur_send_cnt=0;
    if (remain_cnt != MAX_DEBUG_BUFFER)
    {
        debug_uart_send();
    }
    
    CPU_CRITICAL_EXIT();
    if (UartTxCallBackFunction!=NULL)
    {
        UartTxCallBackFunction();
    }
#endif
}


#define debug_BUFFER_SIZE 2048
static u8 debug_uart_buffer[debug_BUFFER_SIZE];
static u16 uart_piont;
void Debug_AutoLog(void)
{
    if (last_uart_tick)
    {
        if (GetTickCount64() - last_uart_tick>(60*1000))
        {
            last_uart_tick =0;
            DebugSetMask(debug_mask_later);//恢复 在响应上位机期间修改值 或 原值
        }
    }
}
void Debug_UartRecIRQ(void)
{
#ifdef DEBUG_UART
    switch(USART_GetITAllStatus(DEBUG_UART))
    {
    case USART_RECEIVE_DATA_READY_INTER_STATUS:
        {
            debug_mask=0;
            for (int i = 0; i < 2; i++)
            {
                debug_uart_buffer[uart_piont++] = USART_ReceiveData(DEBUG_UART);
                if (uart_piont == debug_BUFFER_SIZE)
                {
                    if (UartRxDataCallBackFunction != NULL && last_uart_tick != 0)//第一帧debug串口发出来的数据无论是什么串口都不会响应 需要等待debuglog 不再打印后再与cco通讯
                    {
                        UartRxDataCallBackFunction(debug_uart_buffer, uart_piont);
                    }
                    uart_piont = 0;
                }
            }
            last_uart_tick = GetTickCount64();
        }
        break;
    case USART_RECEIVE_DATA_TIMEOUT_INTER_STATUS:
        {
            debug_mask=0;
            while (USART_GetStatus(DEBUG_UART, USART_DATA_READY_STATUS) == SET)
            {
                debug_uart_buffer[uart_piont++] = USART_ReceiveData(DEBUG_UART);
                if (uart_piont == debug_BUFFER_SIZE)
                {
                    if (UartRxDataCallBackFunction != NULL && last_uart_tick != 0)//第一帧debug串口发出来的数据无论是什么串口都不会响应 需要等待debuglog 不再打印后再与cco通讯
                    {
                        UartRxDataCallBackFunction(debug_uart_buffer, uart_piont);
                        uart_piont = 0;
                    }

                }
            }
            if (UartRxDataCallBackFunction != NULL && last_uart_tick != 0)//第一帧debug串口发出来的数据无论是什么串口都不会响应 需要等待debuglog 不再打印后再与cco通讯
            {
                UartRxDataCallBackFunction(debug_uart_buffer, uart_piont);
            }
            last_uart_tick = GetTickCount64();
            uart_piont = 0;
        }
        break;
    case USART_RECEIVE_LINE_INTER_STATUS:
        USART_GetAllStatus(DEBUG_UART); //read clear
        break;
    case USART_TRANSMITTER_DATA_EMPTY_INTER_STATUS:
        break;
    default:
        //error
        break;
    }

#endif
}


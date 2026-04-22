/*
*
*/
#include  <gsmcu_hal.h>
#include "bps_uart.h"
#include "os.h"
#include "protocol_includes.h"
/*
*
*/

//===========


#define BPS_UART_BUFF_LENGTH        (1500)


static uint8_t BPS_UartRxBuff[BPS_UART_BUFF_LENGTH];

static uint32_t BPS_UartRxBuffReadIndex;
static uint32_t BPS_UartRxBuffWrtieIndex;

//==============
uint32_t IncreaseIndex(uint32_t idx,uint32_t buffsize)
{
    idx++;
    if(idx >= buffsize)
    {
        idx = 0;
    }
    return idx;
}
//
bool IsFifoEmpty(uint32_t * ri,uint32_t * wi)
{
    return (* ri == * wi)?true:false;
}

bool IsFifoFull(uint32_t * ri,uint32_t * wi,uint32_t buffsize)
{
    return (IncreaseIndex(*wi,buffsize) == *ri)?true:false;
}

void InitFifo(uint32_t * ri,uint32_t * wi)
{
    * ri = 0;
    * wi = 0;
}

bool PushFifo(uint32_t * ri,uint32_t * wi,uint8_t *pbuff,uint32_t buffsize,uint8_t dv)
{
    uint32_t witmp;
    if(IsFifoFull(ri,wi,buffsize))return false;
    witmp = IncreaseIndex(*wi,buffsize);
    pbuff[witmp] = dv;
    *wi = witmp;
    return true;
}

bool PopFifo(uint32_t * ri,uint32_t * wi,uint8_t *pbuff,uint32_t buffsize,uint8_t *pdv)
{
    uint32_t ritmp;
    if(IsFifoEmpty(ri,wi))return false;
    ritmp = *ri;
    *pdv = pbuff[ritmp];
    *ri = IncreaseIndex(ritmp,buffsize);
    return true;
}

uint32_t GetFifoDataLen(uint32_t ri,uint32_t wi,uint32_t buffsize)
{
    uint32_t len;
    if(wi >= ri)
    {
        len = (wi) - (ri);
    }
    else   
    {
        len = buffsize - (ri) + (wi);
    }
    return len;
}



uint32_t PopSomeOfFifo(uint32_t * ri,uint32_t * wi,uint8_t *pbuff,uint32_t buffsize,uint8_t *pdst,uint32_t poplen)
{
    static bool InReadFlg = false;
    uint32_t witmp,ritmp,rilen = 0;

    if(poplen == 0) return 0;
    if(false == InReadFlg)
    {//不可重入
        InReadFlg = true;
        if(IsFifoEmpty(ri,wi))
        {
            InReadFlg = false;
            return 0;        
        }
        rilen = 0;
        ritmp = *ri;
        witmp = *wi;
        for(uint32_t i=0;i<poplen;i++)
        {
            ritmp = IncreaseIndex(ritmp,buffsize);
            pdst[i] = pbuff[ritmp];
            ++rilen;
            if(ritmp == witmp)
            {
                break;
            }
        }
        *ri = ritmp;
        InReadFlg = false;
    }
    return rilen;
}

//==============
//static bool UartInInterruptIsp = false;


static uint32_t UartStatus = 0x00;
static uint32_t UartFifoDepth = 0x00;

static UartCallBackFunctionType UartTxCallBackFunction = NULL;
static UartCallBackFunctionType UartRxDataCallBackFunction= NULL;
static UartCallBackFunctionType UartRxErrorCallBackFunction= NULL;
//
void METER_UART_Handler_ISR(void);
void DMAC_Handler_METER_UART_ISR(void);
//
void UartOpen(UartParameterType *UartParameter)
{
#if defined(METER_UART)
    PinRemapConfig(METER_UART_TX_PIN_REMAP,ENABLE);
    PinRemapConfig(METER_UART_RX_PIN_REMAP,ENABLE);
    SCU_PeriphInputConfig(METER_UART_INPUT_ENABLE);
	SCU_GPIOInputConfig(METER_UART_TX_PORT, METER_UART_TX_PIN);
	SCU_GPIOInputConfig(METER_UART_RX_PORT, METER_UART_RX_PIN);
	SCU_SetUsartMode(METER_UART, USART_DONT_INVERT38K_OUTPUT|
#if defined(METER_UART_OPEN_DRAIN)&& ((!defined(II_STA))||defined(I_STA))
								USART_OPEN_DRAIN_ENABLE|
#else
								USART_OPEN_DRAIN_DISABLE|
#endif
								USART_SELECT_NORMAL__USART_MODE|USART_IO_FAST|
								USART_IO_DRIVER_12MA);
	GPIO_InitTypeDef GPIO_InitStruct;
	//GPIO引脚配置
	GPIO_StructInit(&GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = METER_UART_RX_PIN;
#if DEV_CCO
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD;
#else
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
#endif
	GPIO_Init(METER_UART_RX_PORT,&GPIO_InitStruct);
	
    USART_InitTypeDef USART_InitStruct;
    
    USART_InitStruct.USART_BaudRate = UartParameter->BaudRate;
    USART_InitStruct.USART_WordLength_StopBits = UartParameter->StopBits;
    USART_InitStruct.USART_Parity = UartParameter->Parity;
    //
    UartTxCallBackFunction = UartParameter->TxDataCompleteFunction;
    UartRxDataCallBackFunction= UartParameter->RxHaveDataFunction;
    UartRxErrorCallBackFunction= UartParameter->RxParityErrorFunction;
    //
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Fifo_Config = USART_RXFIFO_THR_LEVEL_3|USART_TXFIFO_THR_LEVEL_0|
                                        USART_RXFIFO_CLEAR|USART_TXFIFO_CLEAR|USART_FIFO_ENABLE;//use fifo
    USART_InitStruct.USART_DMA_Flag = ENABLE;
    //USART_InitStruct.USART_Fifo_Config = USART_FIFO_CONFIG_NONE;//not use fifo
    
    //
    USART_Init(METER_UART,&USART_InitStruct);
    UartFifoDepth = USART_GetFifoDepth(METER_UART);
    //
    UartStatus = BPS_UART_OPEN;
    InitFifo(&BPS_UartRxBuffReadIndex,&BPS_UartRxBuffWrtieIndex);
    //enable interrupt
    USART_ConfigInterruptEnable(METER_UART,USART_RECEIVE_LINE_INTER_ENABLE|USART_RECEIVE_DATA_INTER_ENABLE);
    //
    BSP_IntVectSet(METER_UART_IRQn,METER_UART_Handler_ISR);
    //DMA
    BSP_InitDmacVector();
    ReSet_DMA_Channel(DMA, METER_UART_DMA_CHL);
    BSP_IntDmacChannelVectSet(METER_UART_DMA_CHL,DMAC_Handler_METER_UART_ISR);
    //
    NVIC_ClearPendingIRQ(METER_UART_IRQn);
    NVIC_EnableIRQ(METER_UART_IRQn);
#endif
}

void UartWriteWithDma(USART_TypeDef *puart,uint8_t *pbuff,uint32_t len,uint32_t dma_channel)
{
    //DMA
    DMA_InitTypeDef DMA_InitStruct;
    if(len < 1) return;
    //if((UartStatus&BPS_UART_OPEN_MASK) != BPS_UART_OPEN) return;
    DMA_InitStruct.CSR =    CSR_SRC_SIZE_1|
                            CSR_SRC_WIDTH_8BIT|
                            CSR_DST_WIDTH_8BIT|
                            CSR_MODE_HANDSHAKE|
                            CSR_SRCAD_CTL_INC|
                            CSR_DSTAD_CTL_FIX;
    
    DMA_InitStruct.CFG = CFG_DST_HE_SET|CFG_SRC_HE_SET|
                         CFG_INT_ABT_MSK|CFG_INT_ERR_MSK;//|CFG_INT_TC_MSK;
    
    DMA_InitStruct.SrcAdd = (uint32_t)pbuff;
    DMA_InitStruct.DstAddr = GetUartSRAddr(puart);
    DMA_InitStruct.LLP = 0;
    if(GetUartDmaSrc(puart,&DMA_InitStruct.ChlPeriph,NULL)!=0)
    {
      __BKPT(1);
    }
    DMA_InitStruct.SIZE = len;
    DMA_InitStruct.SYNC_Flg = SET;
    DMA_Init(DMA, dma_channel,&DMA_InitStruct);
    DMA_ClearChannelTCITStatus(DMA, dma_channel);
    Set_DMA_Channel(DMA, dma_channel);
}

void UartWrite(uint8_t *pbuff,uint32_t len)
{
#if defined(METER_UART) && defined(METER_UART_DMA_CHL)
    //DMA
    DMA_InitTypeDef DMA_InitStruct;
    if(len < 1) return;
    if((UartStatus&BPS_UART_OPEN_MASK) != BPS_UART_OPEN) return;
    UartStatus |= BPS_UART_TRANSMITING;
    DMA_InitStruct.CSR =    CSR_SRC_SIZE_1|
                            CSR_SRC_WIDTH_8BIT|
                            CSR_DST_WIDTH_8BIT|
                            CSR_MODE_HANDSHAKE|
                            CSR_SRCAD_CTL_INC|
                            CSR_DSTAD_CTL_FIX;
    
    DMA_InitStruct.CFG = CFG_DST_HE_SET|CFG_SRC_HE_SET|
                         CFG_INT_ABT_MSK|CFG_INT_ERR_MSK;//|CFG_INT_TC_MSK;
    
    DMA_InitStruct.SrcAdd = (uint32_t)pbuff;
    DMA_InitStruct.DstAddr = GetUartSRAddr(METER_UART);
    DMA_InitStruct.LLP = 0;
    DMA_InitStruct.ChlPeriph = METER_UART_DMA_REQ;
    DMA_InitStruct.SIZE = len;
    DMA_InitStruct.SYNC_Flg = SET;
    DMA_Init(DMA, METER_UART_DMA_CHL,&DMA_InitStruct);
    //
    DMA_ClearChannelTCITStatus(DMA, METER_UART_DMA_CHL);
    //
    Set_DMA_Channel(DMA, METER_UART_DMA_CHL);
    //
    
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
    return PopSomeOfFifo(&BPS_UartRxBuffReadIndex,&BPS_UartRxBuffWrtieIndex,
                  BPS_UartRxBuff,BPS_UART_BUFF_LENGTH,
                  pbuff,len);
#else
	return -1;
#endif
}

uint32_t GetUartRxDataLen()
{
#if defined(METER_UART)
    return GetFifoDataLen(BPS_UartRxBuffReadIndex,BPS_UartRxBuffWrtieIndex,BPS_UART_BUFF_LENGTH);
#else
	return -1;
#endif
}


void UartClose()
{
#if defined(METER_UART) && defined(METER_UART_DMA_CHL)
    if((UartStatus&BPS_UART_OPEN_MASK) != BPS_UART_OPEN) return;
    NVIC_DisableIRQ(METER_UART_IRQn);
    ReSet_DMA_Channel(DMA, METER_UART_DMA_CHL);
    UartStatus = 0x00;
#endif
}

bool IsUartTxData()
{
#if defined(METER_UART)
    if((UartStatus&BPS_UART_OPEN_MASK) != BPS_UART_OPEN) return false;
    if((UartStatus& BPS_UART_TRANSMITMASK)== BPS_UART_TRANSMITING) return true;
#endif
    return false;
}

bool IsUartTxSendAllDone()
{
#if defined(METER_UART)
	if (USART_GetStatus(METER_UART, USART_TRANSMITTER_EMPTY_STATUS) == SET)
	{
		return true;
	}
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

extern void UART_ExernExe(void);
void METER_UART_Handler_ISR(void)
{
    UART_ExernExe();
#if defined(METER_UART)
    //UartInInterruptIsp = true;
    switch(USART_GetITAllStatus(METER_UART))
    {
        case USART_RECEIVE_DATA_READY_INTER_STATUS:
        {
            for(int i=0;i<(UartFifoDepth-1);i++)
            {
                uint8_t dtmp = USART_ReceiveData(METER_UART);
                
                if(false == PushFifo(&BPS_UartRxBuffReadIndex,&BPS_UartRxBuffWrtieIndex,
                  BPS_UartRxBuff,BPS_UART_BUFF_LENGTH,dtmp))
                {
                    if(UartRxDataCallBackFunction != NULL)
                    {
                        UartRxDataCallBackFunction();
                        PushFifo(&BPS_UartRxBuffReadIndex,&BPS_UartRxBuffWrtieIndex,
                            BPS_UartRxBuff,BPS_UART_BUFF_LENGTH,dtmp);
                    }
                }
            }
        }
        break;
        case USART_RECEIVE_DATA_TIMEOUT_INTER_STATUS:
        {
            uint8_t dtmp;
            while(USART_GetStatus(METER_UART,USART_DATA_READY_STATUS) == SET)
            {
                dtmp = USART_ReceiveData(METER_UART);
                if(false == PushFifo(&BPS_UartRxBuffReadIndex,&BPS_UartRxBuffWrtieIndex,
                  BPS_UartRxBuff,BPS_UART_BUFF_LENGTH,dtmp))
                {
                    if(UartRxDataCallBackFunction != NULL)
                    {
                        UartRxDataCallBackFunction();
                        PushFifo(&BPS_UartRxBuffReadIndex,&BPS_UartRxBuffWrtieIndex,
                            BPS_UartRxBuff,BPS_UART_BUFF_LENGTH,dtmp);
                    }
                }
            }
            if(UartRxDataCallBackFunction != NULL)
            {
                UartRxDataCallBackFunction();
            }
        }
        break;
        case USART_RECEIVE_LINE_INTER_STATUS:
         USART_GetAllStatus(METER_UART);//read clear
        if(UartRxErrorCallBackFunction != NULL)
        {
            UartRxErrorCallBackFunction();
        }
        break;
        case USART_TRANSMITTER_DATA_EMPTY_INTER_STATUS:
        break;
        default:
        //error
        break;
    }
#endif
}

void DMAC_Handler_METER_UART_ISR(void)
{
#if defined(METER_UART)
    UartStatus &= ~BPS_UART_TRANSMITMASK;
    if(UartTxCallBackFunction != NULL)
    {
        UartTxCallBackFunction();
    }
    //发送完成
#endif
}












/****************************************************************
 *
 * UART driver
 * @file gsmcu_m3_USART.c
 * @author sitieqiang 
 * @version 0.1 
 * @date 2016/10/21 15:38:07
 * @note
//   None
//		need to check uart register tobe changed at the end of function
*****************************************************************/

#include "gsmcu_m3_usart.h"

#include "RTE_Device.h" 
#include "gsmcu_m3_gpio.h"
#include "gsmcu_m3_port.h"
#include "Driver_USART.h"
#include "gsmcu_dma.h"


#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif



#if !(RTE_USART0|RTE_USART1|RTE_USART2|RTE_USART3)
	#error "Without any RTE_USART macro in RTE_Components.h!" 
#endif


#define UART_DIRECTION 0x2



// USART Pin Configuration
typedef const struct _USART_PINS {
  PIN_ID                 *tx;            // TX  Pin identifier
  PIN_ID                 *rx;            // RX  Pin identifier
  PIN_ID                 *clk;           // CLK  Pin identifier
  PIN_ID                 *cts;           // CTS Pin identifier
  PIN_ID                 *rts;           // RTS Pin identifier
  PIN_ID                 *dcd;           // DCD Pin identifier
  PIN_ID                 *dsr;           // DSR Pin identifier
  PIN_ID                 *dtr;           // DTR Pin identifier
  PIN_ID                 *ri;            // RI  Pin identifier
} USART_PINS;

// USART Clocks Configuration
typedef const struct _USART_CLOCK {
  __IO uint32_t          *base_clk;      // USART base clock
} USART_CLOCKS;

#if USE_DMA
// USART DMA
typedef const struct _USART_DMA {
  uint8_t                       channel;       // DMA Channel
  uint8_t                       peripheral;    // DMA mux
  uint16_t                       user_id;       // DMA Users own Id
  //apDMA_rTransferTerminated     cb_event;      // DMA Event callback
} USART_DMA;
#endif
// USART Resources definitions
typedef struct 
{
	const ARM_USART_CAPABILITIES  *capabilities;  // Capabilities
	SCI_Type		         *reg;           // Pointer to USART peripheral

	USART_PINS              pins;          // USART pins configuration
	USART_CLOCKS            clk;           // USART clocks configuration

	IRQn_Type               irq_num;       // USART IRQ Number

#if USE_DMA
	const USART_DMA         *dma_tx;
	const USART_DMA         *dma_rx;
#endif
	USART_INFO             *info;          // Run-Time Information
	uint8_t								 **ppvalue;
} const USART_RESOURCES;


/* 
*    //fix 
*	//change 
*/ 
#define ARM_USART_DRV_VERSION    ARM_DRIVER_VERSION_MAJOR_MINOR(1, 0)  /* driver version */

/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion = { 
    ARM_USART_API_VERSION,
    ARM_USART_DRV_VERSION
};
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
/* Driver Capabilities */
//static const ARM_USART_CAPABILITIES DriverCapabilities = 
static const ARM_USART_CAPABILITIES UART_CAPABILITES=                                                                  
{   																					 
    1, /* supports UART (Asynchronous) mode */  										 
    0, /* supports Synchronous Master mode */   										 
    0, /* supports Synchronous Slave mode */											 
    1, /* supports UART Single-wire mode */ 											 
    1, /* supports UART IrDA mode */													 
    0, /* supports UART Smart Card mode */  											 
    0, /* Smart Card Clock generator available */   									 
    0, /* RTS Flow Control available */ 												 
    0, /* CTS Flow Control available */ 												 
    0, /* Transmit completed event: \ref ARM_USART_EVENT_TX_COMPLETE */ 				 
    0, /* Signal receive character timeout event: \ref ARM_USART_EVENT_RX_TIMEOUT */	 
    0, /* RTS Line: 0=not available, 1=available */ 									 
    0, /* CTS Line: 0=not available, 1=available */ 									 
    0, /* DTR Line: 0=not available, 1=available */ 									 
    0, /* DSR Line: 0=not available, 1=available */ 									 
    0, /* DCD Line: 0=not available, 1=available */ 									 
    0, /* RI Line: 0=not available, 1=available */  									 
    0, /* Signal CTS change event: \ref ARM_USART_EVENT_CTS */  						 
    0, /* Signal DSR change event: \ref ARM_USART_EVENT_DSR */  						 
    0, /* Signal DCD change event: \ref ARM_USART_EVENT_DCD */  						 
    0,  /* Signal RI change event: \ref ARM_USART_EVENT_RI */						 
};


uint8_t *p_value[4];
#if USE_DMA
static void DMA_PeripheralCallBack(uint32_t UsersId, apError ResultCode,uint32_t  TransferCount);

const static  USART_DMA DmaArray[4][2]=
{
	{
	#if RTE_USART0
		{ UART0_TX_DMA, DMA_REQ_UART0_TX, 0 | 0 << 8 }, //tx
		{ UART0_RX_DMA, DMA_REQ_UART0_RX, 0 | 1 << 8 }, //rx
	#else
		{ 0, 0, 0 }, { 0, 0, 0 },
	#endif
	},
	{
	#if RTE_USART1
		{ UART1_TX_DMA, DMA_REQ_UART1_TX, 1 | 0 << 8 }, //tx
		{ UART1_RX_DMA, DMA_REQ_UART1_RX, 1 | 1 << 8 }, //rx
	#else
		{ 0, 0, 0 }, { 0, 0, 0 },
	#endif
	},
	{
	#if RTE_USART2
		{ UART2_TX_DMA, DMA_REQ_UART2_TX, 2 | 0 << 8 }, //tx
		{ UART2_RX_DMA, DMA_REQ_UART2_RX, 2 | 1 << 8 }, //rx
	#else
		{ 0, 0, 0 }, { 0, 0, 0 },
	#endif
	},
	{
	#if RTE_USART3
		{ UART3_TX_DMA, DMA_REQ_UART3_TX, 3 | 0 << 8 }, //tx
		{ UART3_RX_DMA, DMA_REQ_UART3_RX, 3 | 1 << 8 }, //rx
	#else
		{ 0, 0, 0 }, { 0, 0, 0 },
	#endif
	},

};
#endif
#if RTE_USART0
static PIN_ID USART0_pin_tx	= {RTE_USART0_TX_PORT,RTE_USART0_TX_BIT,(PORT_FUNC)RTE_USART0_TX_FUNC};
static PIN_ID USART0_pin_rx  = { RTE_USART0_RX_PORT,   RTE_USART0_RX_BIT,   (PORT_FUNC)RTE_USART0_RX_FUNC };
static USART_INFO USART0_Info = {0};

static const USART_RESOURCES USART0_Resources=
{
	&UART_CAPABILITES,
	SCI0,//SCI0_Type
	{//USART_PINS
		&USART0_pin_tx,
		&USART0_pin_rx,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	},
	{//// USART clocks configuration       
		&IpgClock,
	},
	SCI0_IRQn,// USART IRQ Number
#if USE_DMA
	&DmaArray[0][0],//dma_tx
	&DmaArray[0][1],//dma_rx
#endif	 
	&USART0_Info,
	&p_value[0],
};
#endif

#if RTE_USART1
static PIN_ID USART1_pin_tx={RTE_USART1_TX_PORT,RTE_USART1_TX_BIT,(PORT_FUNC)RTE_USART1_TX_FUNC};
static PIN_ID USART1_pin_rx  = {RTE_USART1_RX_PORT,RTE_USART1_RX_BIT,(PORT_FUNC)RTE_USART1_RX_FUNC };
static USART_INFO USART1_Info = {0};
static const USART_RESOURCES USART1_Resources=
{
	&UART_CAPABILITES,
	SCI1,//SCI0_Type
	{//USART_PINS
		&USART1_pin_tx,
		&USART1_pin_rx,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	},
	{//// USART clocks configuration       
		&IpgClock,
	},
	SCI1_IRQn,// USART IRQ Number
#if USE_DMA
	&DmaArray[1][0],//dma_tx
	&DmaArray[1][1],//dma_rx
#endif	
	&USART1_Info,
	&p_value[1],
};
#endif

#if RTE_USART2
static PIN_ID USART2_pin_tx={RTE_USART2_TX_PORT,RTE_USART2_TX_BIT,(PORT_FUNC)RTE_USART2_TX_FUNC};
static PIN_ID USART2_pin_rx  = { RTE_USART2_RX_PORT,   RTE_USART2_RX_BIT,(PORT_FUNC)RTE_USART2_RX_FUNC };
static USART_INFO USART2_Info = {0};
static const USART_RESOURCES USART2_Resources=
{
	&UART_CAPABILITES,
	SCI2,//SCI0_Type
	{//USART_PINS
		&USART2_pin_tx,
		&USART2_pin_rx,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	},
	{//// USART clocks configuration       
		&IpgClock,
	},
	SCI2_IRQn,// USART IRQ Number
#if USE_DMA
	&DmaArray[2][0],//dma_tx
	&DmaArray[2][1],//dma_rx
#endif	
	&USART2_Info,
	&p_value[2],	
};
#endif


#if RTE_USART3
static PIN_ID USART3_pin_tx={RTE_USART3_TX_PORT,RTE_USART3_TX_BIT,(PORT_FUNC)RTE_USART3_TX_FUNC};
static PIN_ID USART3_pin_rx  = { RTE_USART3_RX_PORT,   RTE_USART3_RX_BIT,(PORT_FUNC)RTE_USART3_RX_FUNC };
static USART_INFO USART3_Info = {0};
static const USART_RESOURCES USART3_Resources=
{
	&UART_CAPABILITES,
	SCI3,//SCI0_Type
	{//USART_PINS
		&USART3_pin_tx,
		&USART3_pin_rx,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	},
	{//// USART clocks configuration       
		&IpgClock,
	},
	SCI3_IRQn,// USART IRQ Number
#if USE_DMA
	&DmaArray[3][0],//dma_tx
	&DmaArray[3][1],//dma_rx
#endif	
	&USART3_Info,
	&p_value[3],
};
#endif
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic pop
#endif


static const USART_RESOURCES *uartArray[4]=
{
#if RTE_USART0
	&USART0_Resources,
#else
	NULL,
#endif
#if RTE_USART1
	&USART1_Resources,
#else
	NULL,
#endif
#if RTE_USART2
	&USART2_Resources,
#else
	NULL,
#endif
#if RTE_USART3
	&USART3_Resources,
#else
	NULL,
#endif
};


/**
 * Set baudrate dividers
 * 
 * @author sitieqiang
 * @param uart     Pointer to USART resources
 * @param baudrate Usart baudrate
 * 
 * @return - \b  0: function succeeded
 *         - \b -1: function failed
 * @since 2016/10/17
 */
static int32_t USART_SetBaudrate (USART_RESOURCES *uart,uint32_t baudrate)
{
	uint32_t band_rate = 0;
	if (uart->info->mode == ARM_USART_MODE_SMART_CARD)
	{
		return -1;
	}
	else
	{ 
		uart->reg->SCIBRDF = (((*uart->clk.base_clk * 8 / baudrate) + 1) / 2) & 0x003f; 
		band_rate =(*uart->clk.base_clk*4/baudrate)>>6;
		uart->reg->SCIBRDIH=(band_rate>>8)&0x00ff;
		uart->reg->SCIBRDIL=band_rate&0x00ff;
	}
	return 0;
}



/**
 * get driver version
 * 
 * @author sitieqiang
 * @return driver version
 * @since 2016/10/12
 */
static ARM_DRIVER_VERSION USARTx_GetVersion(void)
{
	return DriverVersion;
}

/**
 * ger driver capabilities
 * 
 * @author sitieqiang
 * @return driver capabilities
 * @since 2016/10/12
 */
static ARM_USART_CAPABILITIES USARTx_GetCapabilities(USART_RESOURCES *uart)
{
	return *uart->capabilities;
}

/**
 * init uart
 * 
 * @param uart     uart pointer
 * @param cb_event callback func
 * 
 * @return OK?
 * @since 2016/10/12
 */
static int32_t USARTx_Initialize(USART_RESOURCES *uart, ARM_USART_SignalEvent_t cb_event)
{
	if (uart->info->flags & USART_FLAG_INITIALIZED)
	{
		// Driver is already initialized
		return ARM_DRIVER_OK;
	}

	// Initialize USART Run-time Resources
	uart->info->cb_event = cb_event;

	uart->info->rx_status.rx_busy          = 0U;
	uart->info->rx_status.rx_overflow      = 0U;
	uart->info->rx_status.rx_break         = 0U;
	uart->info->rx_status.rx_framing_error = 0U;
	uart->info->rx_status.rx_parity_error  = 0U;

	uart->info->xfer.send_active           = 0U;
	uart->info->xfer.tx_def_val            = 0U;
							
	uart->reg->SCIDDR=0x2;
	//select pin function
	PORT_PinConfigure(uart->pins.rx->port,uart->pins.rx->num,uart->pins.rx->config_val);
	PORT_PinConfigure(uart->pins.tx->port,uart->pins.tx->num,uart->pins.tx->config_val);
	if (uart->pins.clk) 
	{
		PORT_PinConfigure(uart->pins.clk->port, uart->pins.clk->num, uart->pins.clk->config_val); 
	}
	if (uart->pins.cts) 
	{
		PORT_PinConfigure(uart->pins.cts->port,uart->pins.cts->num,uart->pins.cts->config_val);
	}
	if (uart->pins.dcd) 
	{
		PORT_PinConfigure(uart->pins.dcd->port,uart->pins.dcd->num,uart->pins.dcd->config_val);
	}
	if (uart->pins.dsr) 
	{
		PORT_PinConfigure(uart->pins.dsr->port,uart->pins.dsr->num,uart->pins.dsr->config_val);
	}
	if (uart->pins.dtr) 
	{
		PORT_PinConfigure(uart->pins.dtr->port,uart->pins.dtr->num,uart->pins.dtr->config_val);
	}
	if (uart->pins.ri) 
	{
		PORT_PinConfigure(uart->pins.ri->port,uart->pins.ri->num,uart->pins.ri->config_val);
	}
	if (uart->pins.rts) 
	{
		PORT_PinConfigure(uart->pins.rts->port,uart->pins.rts->num,uart->pins.rts->config_val);
	}

	uart->info->flags = USART_FLAG_INITIALIZED;
	return ARM_DRIVER_OK;

}

/**
 * uninit uart
 * 
 * @author sitieqiang
 * @param uart   uart pointer
 * 
 * @return OK?
 * @since 2016/10/12
 */
static int32_t USARTx_Uninitialize(USART_RESOURCES *uart)
{
	//reset pin
	uart->reg->SCIDDR=0x00;

	PORT_PinConfigure(uart->pins.rx->port,uart->pins.rx->num,PORT_FUNC_GPIO);
	PORT_PinConfigure(uart->pins.tx->port,uart->pins.tx->num,PORT_FUNC_GPIO);
	uart->reg->SCIDDR=UART_DIRECTION;
	if (uart->pins.clk) 
	{
		PORT_PinConfigure(uart->pins.clk->port, uart->pins.clk->num, PORT_FUNC_GPIO); 
	}
	if (uart->pins.cts) 
	{
		PORT_PinConfigure(uart->pins.cts->port,uart->pins.cts->num,PORT_FUNC_GPIO);
	}
	if (uart->pins.dcd) 
	{
		PORT_PinConfigure(uart->pins.dcd->port,uart->pins.dcd->num,PORT_FUNC_GPIO);
	}
	if (uart->pins.dsr) 
	{
		PORT_PinConfigure(uart->pins.dsr->port,uart->pins.dsr->num,PORT_FUNC_GPIO);
	}
	if (uart->pins.dtr) 
	{
		PORT_PinConfigure(uart->pins.dtr->port,uart->pins.dtr->num,PORT_FUNC_GPIO);
	}
	if (uart->pins.ri) 
	{
		PORT_PinConfigure(uart->pins.ri->port,uart->pins.ri->num,PORT_FUNC_GPIO);
	}
	if (uart->pins.rts) 
	{
		PORT_PinConfigure(uart->pins.rts->port,uart->pins.rts->num,PORT_FUNC_GPIO);
	}
	uart->info->flags = 0U; 
	return ARM_DRIVER_OK;
}
/**
 * 
 * @author sitieqiang
 * @param uart
 * @param state
 * 
 * @return 
 * @since 2016/10/12
 */
static int32_t USARTx_PowerControl(USART_RESOURCES *uart,ARM_POWER_STATE state)
{
    switch (state)
    {
	case ARM_POWER_OFF:
		// Disable USART IRQ
		NVIC_DisableIRQ(uart->irq_num);
#if USE_DMA
		// If DMA mode - disable TX DMA channel
		if ((uart->dma_tx) && (uart->info->xfer.send_active != 0U))
		{ 
//change			GPDMA_ChannelDisable (uart->dma_tx->channel);     
		}
		// If DMA mode - disable RX DMA channel
		if ((uart->dma_rx) && (uart->info->rx_status.rx_busy))
		{
//change			GPDMA_ChannelDisable (uart->dma_rx->channel);
		}
#endif
		//Disable peripheral
		uart->reg->SCICR2=0x0;
		uart->reg->SCICR1=0x0;

		// Clear pending USART interrupts in NVIC
		NVIC_ClearPendingIRQ(uart->irq_num);

		// Clear driver variables
		uart->info->rx_status.rx_busy          = 0U;
		uart->info->rx_status.rx_overflow      = 0U;
		uart->info->rx_status.rx_break         = 0U;
		uart->info->rx_status.rx_framing_error = 0U;
		uart->info->rx_status.rx_parity_error  = 0U;
		uart->info->xfer.send_active           = 0U;
		uart->info->flags &= ~USART_FLAG_POWERED;
        break;

	case ARM_POWER_LOW:
		return ARM_DRIVER_ERROR_UNSUPPORTED;//temp not supported
//        break;

	case ARM_POWER_FULL:
		if ((uart->info->flags & USART_FLAG_INITIALIZED) == 0U)
			return ARM_DRIVER_ERROR;
		if ((uart->info->flags & USART_FLAG_POWERED)     != 0U)
			return ARM_DRIVER_OK;


		// Disable transmitter
		uart->reg->SCICR2_b.TE=0;

		// Disable receiver
		uart->reg->SCICR2_b.RE=0;

		// Disable interrupts
		uart->reg->SCICR2 = 0;//(uint8_t)(~(SCI_SCICR2_ILIE_Msk | SCI_SCICR2_RIE_Msk | SCI_SCICR2_TCIE_Msk | SCI_SCICR2_TIE_Msk));


		uart->reg->DMATHR_b.RXDMATH=0;
		uart->reg->DMATHR_b.TXDMATH=4;

		// Clear driver variables
		uart->info->rx_status.rx_busy          = 0U;
		uart->info->rx_status.rx_overflow      = 0U;
		uart->info->rx_status.rx_break         = 0U;
		uart->info->rx_status.rx_framing_error = 0U;
		uart->info->rx_status.rx_parity_error  = 0U;

		uart->info->mode                       = 0U;

		uart->info->xfer.send_active           = 0U;

		uart->info->flags |= USART_FLAG_POWERED ;

		// Clear and Enable USART IRQ
		NVIC_ClearPendingIRQ(uart->irq_num);
		NVIC_EnableIRQ(uart->irq_num);
		break;

    default:
        return ARM_DRIVER_ERROR_UNSUPPORTED;
    }
	return ARM_DRIVER_OK;
}



/**
 * uart send data
 * 
 * @author sitieqiang
 * @param uart		Pointer to USART resources
 * @param data		Pointer to buffer with data to send to USART transmitter
 * @param num		Number of data items to send
 * 
 * @return OK?
 * @since 2016/10/12
 */
static int32_t USARTx_Send(USART_RESOURCES *uart,const void *data, uint32_t num)
{

	if ((data == NULL) || (num == 0U))
	{
		return ARM_DRIVER_ERROR;
		//Invalid parameters
	}
	if ((uart->info->flags & USART_FLAG_CONFIGURED) == 0U)
	{
		return ARM_DRIVER_ERROR;
		//UART is not config
	}
	if (uart->info->xfer.send_active != 0U )
	{
		// Send is not completed yet
		return ARM_DRIVER_ERROR_BUSY;
	}
	//set flag active
	uart->info->xfer.send_active = 1U;

	uart->info->xfer.tx_buf = (const uint8_t *)data;
	uart->info->xfer.tx_num = num;
	uart->info->xfer.tx_cnt = 0U;

	//single line mode
	if ((uart->reg->SCICR1 & (SCI_SCICR1_LOOPS_Msk | SCI_SCICR1_RSRC_Msk)) == (SCI_SCICR1_LOOPS_Msk | SCI_SCICR1_RSRC_Msk))
	{
		//set txd DDR output
		uart->reg->SCIDDR_b.DDRSC1 = 1;
	}


#if USE_DMA
	//DMA mode
	apDMA_PeripheralAddressSet(
		uart->dma_tx->peripheral,
		(void *)&uart->reg->SCIDRL,
		(apDMA_rTransferTerminated)DMA_PeripheralCallBack);

	apDMA_PeripheralConfigure(
		uart->dma_tx->peripheral,
		apDMA_BURST_1,
		apDMA_WIDTH_8_BIT,
		apDMA_AHB_BUS_1,
		apDMA_SYNC_ENABLE,
		0);
	apDMA_MemPeripheralTransferRequest(
		uart->dma_tx->channel,
		uart->dma_tx->peripheral,
		uart->dma_tx->user_id,
		num,
		apDMA_MEM_TO_PERIPHERAL_DMA_CTRL,
		(void*)data);
	
	uart->reg->SCISR1=SCI_SCISR1_TC_Msk;
#ifdef USART_TCIE_ENABLE
	uart->reg->SCICR2_b.TCIE = 1;
#endif
	uart->reg->DMACR_b.TXDMA_EN = 1;
#else
	if ((uart->info->mode & ARM_USART_DATA_BITS_Msk) == ARM_USART_DATA_BITS_9)
	{
		if (uart->info->xfer.tx_num & 1U)
		{
			//9-bit need unsigned short data
			return ARM_DRIVER_ERROR_PARAMETER;
		}
		uart->reg->SCIDRH = uart->info->xfer.tx_buf[uart->info->xfer.tx_cnt++];

	}

	uart->reg->SCIDRL = uart->info->xfer.tx_buf[uart->info->xfer.tx_cnt++];
	uart->reg->SCICR2_b.TIE = 1;
#endif


	NVIC_ClearPendingIRQ(uart->irq_num);

	return ARM_DRIVER_OK;
}




/**
 * receive uart data
 * 
 * @author sitieqiang
 * @param uart   uart pointer
 * @param data   receive data buffer
 * @param num    receive data size
 * 
 * @return OK?
 * @since 2016/10/12
 */
static int32_t USARTx_Receive(USART_RESOURCES *uart,void *data, uint32_t num)
{

	if ((data == NULL) || num == 0)
	{
		//Invalid parameters
		return ARM_DRIVER_ERROR;
	}

	if ((uart->info->flags & USART_FLAG_CONFIGURED) == 0U)
	{
		//uart is not configured
		return ARM_DRIVER_ERROR;
	}

	//Check uart busy
	if (uart->info->rx_status.rx_busy == 1U)
	{
		return ARM_DRIVER_ERROR_BUSY;
	}
/*	
	if (uart->reg->SCISR1_b.RDRF)
	{
		uart->reg->SCIDRH;
		uart->reg->SCIDRL;
	}
*/	
	//Set RX busy flag
	uart->info->rx_status.rx_busy = 1U;


	uart->info->xfer.rx_num = num;
	uart->info->xfer.rx_buf = (uint8_t *)data;
	uart->info->xfer.rx_cnt = 0U;

	//clear rx statuses
	uart->info->rx_status.rx_break = 0U;
	uart->info->rx_status.rx_framing_error = 0U;
	uart->info->rx_status.rx_overflow = 0U;
	uart->info->rx_status.rx_parity_error = 0U;
	//single line mode
	if ((uart->reg->SCICR1 & (SCI_SCICR1_LOOPS_Msk | SCI_SCICR1_RSRC_Msk)) == (SCI_SCICR1_LOOPS_Msk | SCI_SCICR1_RSRC_Msk))
	{
		//set txd DDR output
		uart->reg->SCIDDR_b.DDRSC1 = 0;
	}


#if USE_DMA

	apDMA_PeripheralConfigure(
		uart->dma_rx->peripheral,
		apDMA_BURST_1,
		apDMA_WIDTH_8_BIT,
		apDMA_AHB_BUS_1,
		apDMA_SYNC_ENABLE,
		0);

	apDMA_PeripheralAddressSet(
		uart->dma_rx->peripheral,
		(void *)&uart->reg->SCIDRL,
		(apDMA_rTransferTerminated)DMA_PeripheralCallBack);


	apDMA_MemPeripheralTransferRequest(
		uart->dma_rx->channel,
		uart->dma_rx->peripheral,
		uart->dma_rx->user_id,
		num,
		apDMA_PERIPHERAL_TO_MEM_DMA_CTRL,
		data);

	uart->reg->SCICR1_b.ILT = 1;
	uart->reg->SCICR2_b.ILIE = 1;
	uart->reg->DMACR_b.RXDMA_EN = 1;
	//Interrupt mode
#else
	//enable interrupt
	uart->reg->SCICR2_b.RIE = 1;
#endif

	NVIC_ClearPendingIRQ(uart->irq_num);


	return ARM_DRIVER_OK;
}

/**
 * Start sending/receiving data to/from USART transmitter/receiver
 * 
 * @author sitieqiang
 * @param uart    uart pointer
 * @param data_out Pointer to buffer with data to send to USART transmitter
 * @param data_in  Pointer to buffer for data to receive from USART receiver
 * @param num      Number of data items to transfer
 * 
 * @return execution_status
 * @since 2016/10/12
 */
static int32_t USARTx_Transfer(USART_RESOURCES  *uart,const void *data_out, void *data_in, uint32_t num)
{

	if ((data_in == NULL) || (data_out==NULL)||num==0) 
	{
		//Invalid parameters
		return ARM_DRIVER_ERROR_PARAMETER;
	}
	if ((uart->info->flags&USART_FLAG_CONFIGURED)==0U) 
	{
		//uart is not config
		return ARM_DRIVER_ERROR;
	}
	//now  not supports
	return ARM_DRIVER_ERROR_UNSUPPORTED;
}

/**
 * Get transmitted data count
 * 
 * @author sitieqiang
 * @param uart  Pointer to USART resources
 * 
 * @return number of data items transmitted
 * @since 2016/10/12
 */
static uint32_t USARTx_GetTxCount(USART_RESOURCES  *uart)
{
	uint32_t cnt;
#if 0
	if (uart->dma_tx) 
	{
		apDMA_GetCount(uart->dma_tx->channel,&cnt);
	}
	else
#endif
	{
		cnt = uart->info->xfer.tx_cnt;
	}
	return cnt;
}

/**
 * Get received data count.
 * 
 * @author sitieqiang
 * @param uart   Pointer to USART resources
 * 
 * @return number of data items received
 * @since 2016/10/12
 */
static uint32_t USARTx_GetRxCount(USART_RESOURCES  *uart)
{
	uint32_t cnt;
#if USE_DMA
	if (uart->info->xfer.rx_cnt==0) 
	{
		apDMA_GetCount(uart->dma_rx->channel,&cnt);
	}
	else
#endif
	{
		cnt = uart->info->xfer.rx_cnt;
	}
	return cnt;
}

/**
 * Control USART Interface.
 * 
 * @author sitieqiang
 * @param uart    Pointer to USART resources
 * @param control control  Operation
 * @param arg     arg      Argument of operation (optional)
 * 
 * @return common \ref execution_status and driver specific \ref usart_execution_status
 * @since 2016/10/12
 */
static int32_t USARTx_Control(USART_RESOURCES  *uart,uint32_t control, uint32_t arg)
{
	uint32_t mode;
	uint8_t cr1, icr;
	if ((uart->info->flags & USART_FLAG_POWERED) == 0U)
	{
		return ARM_DRIVER_ERROR;/* uart is not powered */
	}

	icr      = 0U;
	cr1  	 = 0U;

	switch (control & ARM_USART_CONTROL_Msk)
	{
	case ARM_USART_CONTROL_TX:
		if (arg)
		{
			uart->reg->SCICR2_b.TE=(uint8_t)arg;
			uart->info->flags|=USART_FLAG_TX_ENABLED;
		}
		else
		{
			uart->reg->SCICR2_b.TE=0U;
			uart->info->flags&=~USART_FLAG_TX_ENABLED;
		}
		return ARM_DRIVER_OK;
	case ARM_USART_CONTROL_RX:
		if (arg)
		{
			uart->info->flags|=USART_FLAG_RX_ENABLED;
			uart->reg->SCICR2_b.RE=1U;
		}
		else
		{
			uart->info->flags&=~USART_FLAG_RX_ENABLED;
			uart->reg->SCICR2_b.RE=0U;
		}
		return ARM_DRIVER_OK;
	case ARM_USART_CONTROL_TXIE:
		if (arg)
		{
			uart->reg->SCICR2_b.TIE=1;
		}
		else
		{ 
			uart->reg->SCICR2_b.TIE=0;
		}
		return ARM_DRIVER_OK;
	case ARM_USART_CONTROL_RXIE:
		if (arg)
		{
			uart->reg->SCICR2_b.RIE=1;
		}
		else
		{
			uart->reg->SCICR2_b.RIE=0;
		}
		return ARM_DRIVER_OK;		
	case ARM_USART_CONTROL_BREAK:
		if (arg)
		{
			if (uart->info->xfer.send_active!=0U) 
			{
				return ARM_DRIVER_ERROR;
			}
			uart->reg->SCICR2_b.SBK=1U;
			uart->info->xfer.send_active=1U;
		}
		else
		{
			uart->reg->SCICR2_b.SBK=0U;
			uart->info->xfer.send_active=0;
		}
		return ARM_DRIVER_OK;
	case ARM_USART_ABORT_SEND:
		uart->reg->SCICR2_b.TIE=0;
#if USE_DMA
		//if dma mode
		if ((uart->dma_tx))//&&(uart->info->xfer.send_active!=0U)) 
		{
			uint32_t count;
			uart->reg->DMACR_b.TXDMA_EN = 0;
			apDMA_AbortTransfer(uart->dma_tx->channel, &count);
		}
#endif
		//clear send active
		uart->info->xfer.send_active=0U;
		return ARM_DRIVER_OK;
	case ARM_USART_ABORT_RECEIVE:
		uart->reg->SCICR2_b.RIE=0U;
#if USE_DMA
		if (uart->dma_rx) 
		{
			//change uart->reg->SCICR1_b /* DMA off */
			uint32_t count;
			uart->reg->DMACR_b.RXDMA_EN = 0;
			apDMA_AbortTransfer(uart->dma_rx->channel, &count);
		}
#endif
		//clear flag
		uart->info->rx_status.rx_busy=0U;
		return ARM_DRIVER_OK;
	case ARM_USART_ABORT_TRANSFER:
		return ARM_DRIVER_ERROR_UNSUPPORTED;
	default:break;
	}

	switch (control&ARM_USART_CONTROL_Msk)
	{
	case ARM_USART_MODE_ASYNCHRONOUS:		
		mode = ARM_USART_MODE_ASYNCHRONOUS;
		break;
	case ARM_USART_MODE_SYNCHRONOUS_MASTER:
		if (uart->capabilities->synchronous_master)
		{
		}
		else
			return ARM_USART_ERROR_MODE;
	case ARM_USART_MODE_SYNCHRONOUS_SLAVE:
		if (uart->capabilities->synchronous_master)
		{
		}
		else
			return ARM_USART_ERROR_MODE;
	case ARM_USART_MODE_SINGLE_WIRE:
		// Enable Half duplex
		cr1 = (1<<SCI_SCICR1_RSRC_Pos) | (1<<SCI_SCICR1_LOOPS_Pos);
		mode =ARM_USART_MODE_SINGLE_WIRE;
		break;
	case ARM_USART_MODE_IRDA:
		if (uart->capabilities->irda) 
		{
			//uart->reg->SCIIRCR_b.IREN=1;
			icr |= SCI_SCIIRCR_IREN_Msk;
			if(arg&0x80000000)
			{
				icr|=SCI_SCIIRCR_IRMD_Msk;
			}
			if (arg&(1<<30))
			{
				icr |= SCI_SCIIRCR_RINV_Msk | SCI_SCIIRCR_TINV_Msk; 
			}
			mode = ARM_USART_MODE_IRDA;
		}
		else
		{
			return ARM_DRIVER_ERROR;
		}
		break;
	case ARM_USART_MODE_SMART_CARD:
		if (uart->capabilities->smart_card)
		{
			//change
			return ARM_DRIVER_ERROR_UNSUPPORTED;
		}
		else
		{
			return ARM_DRIVER_ERROR;
		}
//		break;
	case ARM_USART_SET_DEFAULT_TX_VALUE:
//		uart->info->xfer.tx_def_val = arg;
		return ARM_DRIVER_ERROR_UNSUPPORTED;
	case ARM_USART_SET_IRDA_PULSE:
		if (uart->capabilities->irda) 
		{
			if (uart->reg->SCIIRCR_b.IRMD) 
			{	
				uart->reg->SCIIRDR = (uint16_t)((*uart->clk.base_clk / arg) | SCI_SCIIRDR_IRSC_Msk); 
			}
			else
			{
				if (arg>2)
				{
					return ARM_DRIVER_ERROR_PARAMETER;
				}
				uart->reg->SCIIRCR_b.TNUM = 2-(arg&0xff);
				uart->reg->SCIIRCR_b.RNUM = 2-((arg>>8)&0xff);
			}
		}
		else
		{
			return ARM_DRIVER_ERROR_UNSUPPORTED;
		}
		return ARM_DRIVER_OK;
	case ARM_USART_SET_SMART_CARD_GUARD_TIME:
	case ARM_USART_SET_SMART_CARD_CLOCK:
	case ARM_USART_CONTROL_SMART_CARD_NACK:
		if (uart->capabilities->smart_card)
		{
			//change
			return ARM_DRIVER_ERROR_UNSUPPORTED;
		}
		else
		{
			return ARM_DRIVER_ERROR;
		}
	default: 
		return ARM_DRIVER_ERROR_UNSUPPORTED;
	}

	//check uart status 
	if (//uart->info->rx_status.rx_busy || 
		(uart->info->xfer.send_active!=0U) ) 
	{
		return ARM_DRIVER_ERROR_BUSY;
	}
	//DATA BITS
	switch (control&ARM_USART_DATA_BITS_Msk)
	{
	case ARM_USART_DATA_BITS_8:
		break;
	case ARM_USART_DATA_BITS_9:
		cr1|=(1<<SCI_SCICR1_M_Pos);
		break;
	default:
		return ARM_USART_ERROR_DATA_BITS;
	}

	//Parity
	switch (control&ARM_USART_PARITY_Msk) 
	{
	case ARM_USART_PARITY_NONE:
		break;
	case ARM_USART_PARITY_EVEN:
		cr1 |= (1 << SCI_SCICR1_PE_Pos);
		break;
	case ARM_USART_PARITY_ODD:
		cr1|=(1 << SCI_SCICR1_PE_Pos)|(1<<SCI_SCICR1_PT_Pos);
		break;
	default:
		return ARM_USART_ERROR_PARITY;
	}

	//Stop bit
	switch (control & ARM_USART_STOP_BITS_Msk)
	{
	case ARM_USART_STOP_BITS_1:
		break;
	default:
		return ARM_USART_ERROR_STOP_BITS;
	}

	// USART Flow control
	switch (control & ARM_USART_FLOW_CONTROL_Msk)
	{
	case ARM_USART_FLOW_CONTROL_NONE:
		break;
	default:
		return ARM_USART_ERROR_FLOW_CONTROL;
	}
	uart->info->mode = mode;
	// USART Baudrate
	if (USART_SetBaudrate (uart,arg&0xffffff) == -1)
	{
		uart->info->mode = 0;
		return ARM_USART_ERROR_BAUDRATE;
	}

	/* 
	change config pin here  tx rx for different mode
	*/ 
	switch (uart->info->mode)
	{
	case ARM_USART_MODE_SINGLE_WIRE:
	case ARM_USART_MODE_SMART_CARD:
		break;
	default:
		break;
	}

	uart->reg->SCICR1 = cr1;

	uart->reg->SCIIRCR=icr;

	// Set configured flag
	uart->info->flags |= USART_FLAG_CONFIGURED;
	return ARM_DRIVER_OK;
}

/**
 * Get USART status.
 * 
 * @author sitieqiang
 * @param uart   Pointer to USART resources
 * 
 * @return USART status \ref ARM_USART_STATUS
 * @since 2016/10/12
 */
static ARM_USART_STATUS USARTx_GetStatus(USART_RESOURCES  *uart)
{
	ARM_USART_STATUS stat;
	stat.tx_busy          = uart->info->xfer.send_active;
	stat.rx_busy          = uart->info->rx_status.rx_busy;
	stat.tx_underflow     = 0U;
	stat.rx_overflow      = uart->info->rx_status.rx_overflow;
	stat.rx_break         = uart->info->rx_status.rx_break;
	stat.rx_framing_error = uart->info->rx_status.rx_framing_error;
	stat.rx_parity_error  = uart->info->rx_status.rx_parity_error;
	return stat;
}

/**
 * Set USART Modem Control line state
 * 
 * @author sitieqiang
 * @param uart    Pointer to USART resources
 * @param control control   \ref ARM_USART_MODEM_CONTROL
 * 
 * @return execution_status
 * @since 2016/10/12
 */
static int32_t USARTx_SetModemControl(USART_RESOURCES  *uart,ARM_USART_MODEM_CONTROL control)
{
	if ((uart->info->flags & USART_FLAG_CONFIGURED) == 0U || control>ARM_USART_DTR_SET)
	{
		//uart is not configured
		return ARM_DRIVER_ERROR;
	}
	return ARM_DRIVER_ERROR_UNSUPPORTED;
}

/**
 * Get USART Modem Status lines state.
 * 
 * @author sitieqiang
 * @param uart   Pointer to USART resources
 * 
 * @return modem status \ref ARM_USART_MODEM_STATUS
 * @since 2016/10/12
 */
static ARM_USART_MODEM_STATUS USARTx_GetModemStatus(USART_RESOURCES  *uart)
{
	ARM_USART_MODEM_STATUS modem_status;
	modem_status.cts = 0U;
	modem_status.dsr = 0U;
	modem_status.ri  = 0U;
	modem_status.dcd = 0U;
	if ((uart->info->flags &
    USART_FLAG_CONFIGURED) == 0U) {
		//uart is not configured
		modem_status.cts = 1U;
		modem_status.dsr = 1U;
		modem_status.ri  = 1U;
		modem_status.dcd = 1U;
	}

	return  modem_status;
}


/*
static void ARM_USART_SignalEvent(USART_RESOURCES  *uart,uint32_t event)
{
    // function body
}
*/

#if USE_DMA
static void DMA_PeripheralCallBack(uint32_t    UsersId,
								   apError     ResultCode,
								   uint32_t    TransferCount)
{
	u8 u_num=UsersId&0xff;
	u8 isRx=(UsersId>>8)&0xff;
	USART_RESOURCES *uart=uartArray[u_num];
	if (uart)
	{
		if (isRx)//接收满
		{
			
		}
		else
		{
			//uart->reg->DMACR_b.TXDMA_EN = 0;
			uart->info->xfer.tx_cnt += TransferCount;
#ifndef USART_TCIE_ENABLE
			if (uart->info->xfer.send_active)
			{

				uart->info->xfer.send_active = 0;
				if (uart->info->cb_event != NULL)
				{
					uart->info->cb_event(ARM_USART_EVENT_SEND_COMPLETE);
				}
			}
#endif
			// Disable DMA req
			
		}
	}
}
#endif
/**
 * USART Interrupt handler.
 * 
 * @author sitieqiang
 * @param uart   Pointer to USART resources
 * @since 2016/10/17
 */
static void USARTx_IRQHandler(USART_RESOURCES *uart)
{
	uint32_t status,event;
	event=0;
	status = uart->reg->SCISR1;

	#ifndef USE_DMA
	if (status & SCI_SCISR1_TDRF_Msk)
	{
		if (uart->info->xfer.tx_num != uart->info->xfer.tx_cnt)
		{
			if (((uart->info->mode == ARM_USART_MODE_SYNCHRONOUS_MASTER)  ||
				(uart->info->mode == ARM_USART_MODE_SYNCHRONOUS_SLAVE )) &&
				(uart->info->xfer.sync_mode == USART_SYNC_MODE_RX))
			{
				//change
				return;
			}
			else
			{
				
				if ((uart->info->mode&ARM_USART_DATA_BITS_Msk)==ARM_USART_DATA_BITS_9) 
				{
					uart->reg->SCIDRH = uart->info->xfer.tx_buf[uart->info->xfer.tx_cnt];
					uart->info->xfer.tx_cnt++;
				}
				uart->reg->SCIDRL = uart->info->xfer.tx_buf[uart->info->xfer.tx_cnt]; 
				uart->info->xfer.tx_cnt++;

				

			}
		}
		else if (uart->info->xfer.send_active)
		{
			// Disable THRE interrupt
			uart->reg->SCICR2_b.TCIE=0U;
			uart->reg->SCICR2_b.TIE=0U;
			//clear tx busy flag
			uart->info->xfer.send_active=0U;
			//sent complent event
			event |= ARM_USART_EVENT_SEND_COMPLETE;
		}		
		else 
		{
			// Disable THRE interrupt
			uart->reg->SCICR2_b.TIE=0U;
			uart->reg->SCICR2_b.TCIE=0U;
		}
		event|=ARM_USART_EVENT_CTS;
	}
	#else
	#ifdef USART_TCIE_ENABLE
	if (status & SCI_SCISR1_TC_Msk)//TX 发送完毕
	{
		if (uart->info->xfer.send_active)
		{
			event |= ARM_USART_EVENT_SEND_COMPLETE;
			uart->info->xfer.send_active=0;
		}
		// Disable DMA req
		uart->reg->DMACR_b.TXDMA_EN=0;
		uart->reg->SCICR2_b.TIE=0U;
		uart->reg->SCICR2_b.TCIE=0U;
		uart->reg->SCISR1=SCI_SCISR1_TC_Msk;
	}
	#endif
	#endif

	//check error event 
	//OVERFLOW
	if (status&SCI_SCISR1_OR_Msk)
	{
		uart->info->rx_status.rx_overflow=1;
		event|=ARM_USART_EVENT_RX_OVERFLOW;
		uart->reg->SCISR1=SCI_SCISR1_OR_Msk;
	}
	// Parity error
	if (status & SCI_SCISR1_PF_Msk) 
	{
		uart->info->rx_status.rx_parity_error=1U;
		event|=ARM_USART_EVENT_RX_PARITY_ERROR;
		uart->reg->SCISR1=SCI_SCISR1_PF_Msk;
	}
	// Framing error
	if (status & SCI_SCISR1_FE_Msk) 
	{
		uart->info->rx_status.rx_framing_error=1;
		event|=ARM_USART_EVENT_RX_FRAMING_ERROR;
		uart->reg->SCISR1=SCI_SCISR1_FE_Msk;
	}



	#ifndef USE_DMA
	if (status & SCI_SCISR1_RDRF_Msk)
	{
		/* 
		change add sync code
		*/
		event|=ARM_USART_EVENT_RI;//receive data
		if (uart->info->rx_status.rx_busy)
		{
			if ((uart->info->mode & ARM_USART_DATA_BITS_Msk) == ARM_USART_DATA_BITS_9)
			{
				uart->info->xfer.rx_buf[uart->info->xfer.rx_cnt] = uart->reg->SCIDRH;
				uart->info->xfer.rx_cnt++;
			}
			uart->info->xfer.rx_buf[uart->info->xfer.rx_cnt] = uart->reg->SCIDRL;
			*(uart->ppvalue) = &uart->info->xfer.rx_buf[uart->info->xfer.rx_cnt];
			uart->info->xfer.rx_cnt++;
			
			uart->reg->SCICR2_b.ILIE = 1; //get first byte open idle Interrupt
			
			//check rx if all data is transmitted
			if (uart->info->xfer.rx_cnt == uart->info->xfer.rx_num) 
			{
				//disable rx interrupt
				uart->reg->SCICR2_b.RIE=0U;
				uart->reg->SCICR2_b.ILIE = 0; //get all data,close overtime detect 

				//clear rx busy flag

				uart->info->rx_status.rx_busy=0U;

				/*
				//change add sync code support
				*/

				//reveive event
				event|=ARM_USART_EVENT_RECEIVE_COMPLETE;
			}

		}
		else // Dummy read
		{
			event|=ARM_USART_EVENT_RECEIVE_COMPLETE|ARM_USART_EVENT_RI; //receive done ,but new data was received
			
			uart->info->ri_value = uart->reg->SCIDRH;
			uart->info->ri_value = uart->info->ri_value<<8|uart->reg->SCIDRL;
			*(uart->ppvalue) = (uint8_t *)&(uart->info->ri_value);
		}

	}
	#else
	if (status & SCI_SCISR1_IDLE_Msk)
	{
		if (uart->info->rx_status.rx_busy)
		{
			uint32_t count;
			apDMA_AbortTransfer(uart->dma_rx->channel,&count);
			uart->info->xfer.rx_cnt+=count;
			event |= ARM_USART_EVENT_RECEIVE_COMPLETE;
			//close rx dma
			uart->info->rx_status.rx_busy=0;
			uart->reg->DMACR_b.RXDMA_EN=0;
			uart->reg->SCICR2_b.ILIE=0U;
		}
		uart->reg->SCISR1=SCI_SCISR1_IDLE_Msk;
	}
	#endif
	if (uart->info->rx_status.rx_busy)
	{
		if (status & SCI_SCISR1_IDLE_Msk)  //over time
		{
			uart->reg->SCIDRL; //clear flag
			event |= ARM_USART_EVENT_RX_TIMEOUT;
		}
	}
	if ((uart->info->cb_event != NULL) && (event != 0U)) 
	{
		uart->info->cb_event (event);
	}
}

#if RTE_USART0
// USART0 Driver Wrapper functions
static ARM_USART_CAPABILITIES USART0_GetCapabilities (void) {
  return USARTx_GetCapabilities (&USART0_Resources);
}
static int32_t USART0_Initialize (ARM_USART_SignalEvent_t cb_event) {
  return USARTx_Initialize (&USART0_Resources,cb_event );
}
static int32_t USART0_Uninitialize (void) {
  return USARTx_Uninitialize(&USART0_Resources);
}
static int32_t USART0_PowerControl (ARM_POWER_STATE state) {
  return USARTx_PowerControl (&USART0_Resources,state);
}
static int32_t USART0_Send (const void *data, uint32_t num) {
  return USARTx_Send (&USART0_Resources,data, num);
}
static int32_t USART0_Receive (void *data, uint32_t num) {
  return USARTx_Receive (&USART0_Resources,data, num);
}
static int32_t USART0_Transfer (const void      *data_out,
                                      void      *data_in,
                                      uint32_t   num) {
  return USARTx_Transfer (&USART0_Resources,data_out, data_in, num);
}
static uint32_t USART0_GetTxCount (void) {
  return USARTx_GetTxCount (&USART0_Resources);
}
static uint32_t USART0_GetRxCount (void) {
  return USARTx_GetRxCount (&USART0_Resources); 
}
static int32_t USART0_Control (uint32_t control, uint32_t arg) {
  return USARTx_Control ( &USART0_Resources,control, arg);
}
static ARM_USART_STATUS USART0_GetStatus (void) {
  return USARTx_GetStatus (&USART0_Resources);
}
static int32_t USART0_SetModemControl (ARM_USART_MODEM_CONTROL control) {
  return USARTx_SetModemControl (&USART0_Resources,control );
}
static ARM_USART_MODEM_STATUS USART0_GetModemStatus (void) {
  return USARTx_GetModemStatus (&USART0_Resources);
}
void USART0_IRQHandler (void) {
  USARTx_IRQHandler (&USART0_Resources);
}
#endif


#if RTE_USART1
// USART1 Driver Wrapper functions
static ARM_USART_CAPABILITIES USART1_GetCapabilities (void) {
  return USARTx_GetCapabilities (&USART1_Resources);
}
static int32_t USART1_Initialize (ARM_USART_SignalEvent_t cb_event) {
  return USARTx_Initialize (&USART1_Resources,cb_event );
}
static int32_t USART1_Uninitialize (void) {
  return USARTx_Uninitialize(&USART1_Resources);
}
static int32_t USART1_PowerControl (ARM_POWER_STATE state) {
  return USARTx_PowerControl (&USART1_Resources,state);
}
static int32_t USART1_Send (const void *data, uint32_t num) {
  return USARTx_Send (&USART1_Resources,data, num);
}
static int32_t USART1_Receive (void *data, uint32_t num) {
  return USARTx_Receive (&USART1_Resources,data, num);
}
static int32_t USART1_Transfer (const void      *data_out,
                                      void      *data_in,
                                      uint32_t   num) {
  return USARTx_Transfer (&USART1_Resources,data_out, data_in, num);
}
static uint32_t USART1_GetTxCount (void) {
  return USARTx_GetTxCount (&USART1_Resources);
}
static uint32_t USART1_GetRxCount (void) {
  return USARTx_GetRxCount (&USART1_Resources); 
}
static int32_t USART1_Control (uint32_t control, uint32_t arg) {
  return USARTx_Control ( &USART1_Resources,control, arg);
}
static ARM_USART_STATUS USART1_GetStatus (void) {
  return USARTx_GetStatus (&USART1_Resources);
}
static int32_t USART1_SetModemControl (ARM_USART_MODEM_CONTROL control) {
  return USARTx_SetModemControl (&USART1_Resources,control );
}
static ARM_USART_MODEM_STATUS USART1_GetModemStatus (void) {
  return USARTx_GetModemStatus (&USART1_Resources);
}
void USART1_IRQHandler (void) {
  USARTx_IRQHandler (&USART1_Resources);
}
#endif


#if RTE_USART2
// USART2 Driver Wrapper functions
static ARM_USART_CAPABILITIES USART2_GetCapabilities (void) {
  return USARTx_GetCapabilities (&USART2_Resources);
}
static int32_t USART2_Initialize (ARM_USART_SignalEvent_t cb_event) {
  return USARTx_Initialize (&USART2_Resources,cb_event );
}
static int32_t USART2_Uninitialize (void) {
  return USARTx_Uninitialize(&USART2_Resources);
}
static int32_t USART2_PowerControl (ARM_POWER_STATE state) {
  return USARTx_PowerControl (&USART2_Resources,state);
}
static int32_t USART2_Send (const void *data, uint32_t num) {
  return USARTx_Send (&USART2_Resources,data, num);
}
static int32_t USART2_Receive (void *data, uint32_t num) {
  return USARTx_Receive (&USART2_Resources,data, num);
}
static int32_t USART2_Transfer (const void      *data_out,
                                      void      *data_in,
                                      uint32_t   num) {
  return USARTx_Transfer (&USART2_Resources,data_out, data_in, num);
}
static uint32_t USART2_GetTxCount (void) {
  return USARTx_GetTxCount (&USART2_Resources);
}
static uint32_t USART2_GetRxCount (void) {
  return USARTx_GetRxCount (&USART2_Resources); 
}
static int32_t USART2_Control (uint32_t control, uint32_t arg) {
  return USARTx_Control ( &USART2_Resources,control, arg);
}
static ARM_USART_STATUS USART2_GetStatus (void) {
  return USARTx_GetStatus (&USART2_Resources);
}
static int32_t USART2_SetModemControl (ARM_USART_MODEM_CONTROL control) {
  return USARTx_SetModemControl (&USART2_Resources,control );
}
static ARM_USART_MODEM_STATUS USART2_GetModemStatus (void) {
  return USARTx_GetModemStatus (&USART2_Resources);
}
void USART2_IRQHandler (void) {
  USARTx_IRQHandler (&USART2_Resources);
}
#endif



#if RTE_USART3
// USART3 Driver Wrapper functions
static ARM_USART_CAPABILITIES USART3_GetCapabilities (void) {
  return USARTx_GetCapabilities (&USART3_Resources);
}
static int32_t USART3_Initialize (ARM_USART_SignalEvent_t cb_event) {
  return USARTx_Initialize (&USART3_Resources,cb_event );
}
static int32_t USART3_Uninitialize (void) {
  return USARTx_Uninitialize(&USART3_Resources);
}
static int32_t USART3_PowerControl (ARM_POWER_STATE state) {
  return USARTx_PowerControl (&USART3_Resources,state);
}
static int32_t USART3_Send (const void *data, uint32_t num) {
  return USARTx_Send (&USART3_Resources,data, num);
}
static int32_t USART3_Receive (void *data, uint32_t num) {
  return USARTx_Receive (&USART3_Resources,data, num);
}
static int32_t USART3_Transfer (const void      *data_out,
                                      void      *data_in,
                                      uint32_t   num) {
  return USARTx_Transfer (&USART3_Resources,data_out, data_in, num);
}
static uint32_t USART3_GetTxCount (void) {
  return USARTx_GetTxCount (&USART3_Resources);
}
static uint32_t USART3_GetRxCount (void) {
  return USARTx_GetRxCount (&USART3_Resources); 
}
static int32_t USART3_Control (uint32_t control, uint32_t arg) {
  return USARTx_Control ( &USART3_Resources,control, arg);
}
static ARM_USART_STATUS USART3_GetStatus (void) {
  return USARTx_GetStatus (&USART3_Resources);
}
static int32_t USART3_SetModemControl (ARM_USART_MODEM_CONTROL control) {
  return USARTx_SetModemControl (&USART3_Resources,control );
}
static ARM_USART_MODEM_STATUS USART3_GetModemStatus (void) {
  return USARTx_GetModemStatus (&USART3_Resources);
}
void USART3_IRQHandler (void) {
  USARTx_IRQHandler (&USART3_Resources);
}
#endif

#if RTE_USART0
// End USART Interface
ARM_DRIVER_USART Driver_USART0 = {
    USARTx_GetVersion,
    USART0_GetCapabilities,
    USART0_Initialize,
    USART0_Uninitialize,
    USART0_PowerControl,
    USART0_Send,
    USART0_Receive,
    USART0_Transfer,
    USART0_GetTxCount,
    USART0_GetRxCount,
    USART0_Control,
    USART0_GetStatus,
    USART0_SetModemControl,
    USART0_GetModemStatus
};
#endif 

#if RTE_USART1
// End USART Interface
ARM_DRIVER_USART Driver_USART1 = {
    USARTx_GetVersion,
    USART1_GetCapabilities,
    USART1_Initialize,
    USART1_Uninitialize,
    USART1_PowerControl,
    USART1_Send,
    USART1_Receive,
    USART1_Transfer,
    USART1_GetTxCount,
    USART1_GetRxCount,
    USART1_Control,
    USART1_GetStatus,
    USART1_SetModemControl,
    USART1_GetModemStatus
};
#endif 


#if RTE_USART2
// End USART Interface
ARM_DRIVER_USART Driver_USART2 = {
    USARTx_GetVersion,
    USART2_GetCapabilities,
    USART2_Initialize,
    USART2_Uninitialize,
    USART2_PowerControl,
    USART2_Send,
    USART2_Receive,
    USART2_Transfer,
    USART2_GetTxCount,
    USART2_GetRxCount,
    USART2_Control,
    USART2_GetStatus,
    USART2_SetModemControl,
    USART2_GetModemStatus
};
#endif 


#if RTE_USART3
// End USART Interface
ARM_DRIVER_USART Driver_USART3 = {
    USARTx_GetVersion,
    USART3_GetCapabilities,
    USART3_Initialize,
    USART3_Uninitialize,
    USART3_PowerControl,
    USART3_Send,
    USART3_Receive,
    USART3_Transfer,
    USART3_GetTxCount,
    USART3_GetRxCount,
    USART3_Control,
    USART3_GetStatus,
    USART3_SetModemControl,
    USART3_GetModemStatus
};
#endif 



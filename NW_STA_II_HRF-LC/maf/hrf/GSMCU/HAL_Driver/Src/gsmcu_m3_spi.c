/****************************************************************
 *
 * spi driver
 * @file gsmcu_m3_SPI.c
 * @author sitieqiang                     
 * @version 0.1
 * @date 2016/11/24 14:18:45
 * @note NONE
 *
*****************************************************************/
#include "gsmcu_m3_spi.h"
#include "gsmcu_m3_port.h"
#include "gsmcu_m3_gpio.h"
#include "gsmcu_m3_spi.h"
#include "RTE_Device.h"
#include	<string.h>
#include "gsmcu_dma.h"

#define ARM_SPI_DRV_VERSION    ARM_DRIVER_VERSION_MAJOR_MINOR(2, 0) /* driver version */
#if !(RTE_SPI0||RTE_SPI1)
#error "SPI not configured in RTE_Components.h!"
#endif
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#if USE_DMA
// USART DMA
typedef const struct _SPI_DMA {
  uint8_t                       channel;       // DMA Channel
  uint8_t                       peripheral;    // DMA mux
  uint16_t                      user_id;       // DMA Users own Id
  //apDMA_rTransferTerminated     cb_event;      // DMA Event callback
} SPI_DMA;
#endif

/* SPI Resource Configuration */
typedef struct spi_resources
{
	SPI_Type                   *reg;          // SPI register interface
	IRQn_Type                  irqn;         // SPI IRQ Number in NVIC
	uint32_t                   *clk;         //clock
	const SPI_PIN              *sck;          // SPI SCK pin
	const SPI_PIN              *ss;         // SPI SSEL pin
	const SPI_PIN              *mosi;         // SPI MOSI pin
	const SPI_PIN              *miso;         // SPI MISO pin
	SPI_INFO                   *info;         // Run-Time control information
#if USE_DMA
	const SPI_DMA              *dma_tx;
	const SPI_DMA              *dma_rx;
#endif
} const SPI_RESOURCES;


#if USE_DMA
static void DMA_PeripheralCallBack(uint32_t UsersId, apError ResultCode,uint32_t  TransferCount);

const static SPI_DMA DmaArray[2][2]=
{
	{
	#if (RTE_SPI0)
		{ SPI0_TX_DMA, DMA_REQ_SPI0_TX, 0 | 0 << 8 }, //tx
		{ SPI0_RX_DMA, DMA_REQ_SPI0_RX, 0 | 1 << 8 }, //rx
	#else
		{ 0, 0, 0 }, { 0, 0, 0 },
	#endif
	},
	{
	#if (RTE_SPI1)
		{ SPI1_TX_DMA, DMA_REQ_SPI1_TX, 1 | 0 << 8 }, //tx
		{ SPI1_RX_DMA, DMA_REQ_SPI1_RX, 1 | 1 << 8 }, //rx
	#else
		{ 0, 0, 0 }, { 0, 0, 0 },
	#endif
	},
};
#endif
#if (RTE_SPI0)
static const SPI_PIN spi_ss[] = {
#ifndef SPI0_SS_PIN
	{
		RTE_SPI0_SS0_PORT,
		RTE_SPI0_SS0_BIT,
		(PORT_FUNC)RTE_SPI0_SS0_FUNC,
	},
#else
	SPI0_SS_PIN,
#endif
};
static const SPI_PIN spi_sck[] = {
#ifndef SPI0_SCK_PIN
	{
		RTE_SPI0_SCK0_PORT,
		RTE_SPI0_SCK0_BIT,
		(PORT_FUNC)RTE_SPI0_SCK0_FUNC,
	},

#else
	SPI0_SCK_PIN,
#endif
};
static const SPI_PIN spi_mosi[] = {
#ifndef SPI0_MOSI_PIN
	{
		RTE_SPI0_MOSI0_PORT,
		RTE_SPI0_MOSI0_BIT,
		(PORT_FUNC)RTE_SPI0_MOSI0_FUNC,
	},

#else
	SPI0_MOSI_PIN,
#endif
};
static const SPI_PIN spi_miso[] = {
#ifndef SPI0_MISO_PIN
	{
		RTE_SPI0_MISO0_PORT,
		RTE_SPI0_MISO0_BIT,
		(PORT_FUNC)RTE_SPI0_MISO0_FUNC,
	},
#else
	SPI0_MISO_PIN,
#endif
};

/* SPI0 Control Information */
static SPI_INFO SPI_Ctrl = { 0 };
/* SPI0 Resources */
static SPI_RESOURCES SPI0_Resources = {
	SPI0,
	SPI0_IRQn,
	&IpgClock,
	spi_sck,
	spi_ss,
	spi_mosi,
	spi_miso,
	&SPI_Ctrl,
#if USE_DMA
	&DmaArray[0][0],//dma_tx
	&DmaArray[0][1],//dma_rx
#endif
};

#endif



#if (RTE_SPI1)
static const SPI_PIN spi1_ss[] = {
#ifndef SPI1_SS_PIN
	{
		RTE_SPI1_SS0_PORT,
		RTE_SPI1_SS0_BIT,
		(PORT_FUNC)RTE_SPI1_SS0_FUNC,
	},

#else
	SPI1_SS_PIN,
#endif
};
static const SPI_PIN spi1_sck[] = {
#ifndef SPI1_SCK_PIN
	{
		RTE_SPI1_SCK0_PORT,
		RTE_SPI1_SCK0_BIT,
		(PORT_FUNC)RTE_SPI1_SCK0_FUNC,
	},

#else
	SPI1_SCK_PIN,
#endif
};
static const SPI_PIN spi1_mosi[] = {
#ifndef SPI1_MOSI_PIN
	{
		RTE_SPI1_MOSI0_PORT,
		RTE_SPI1_MOSI0_BIT,
		(PORT_FUNC)RTE_SPI1_MOSI0_FUNC,
	},

#else
	SPI1_MOSI_PIN,
#endif
};
static const SPI_PIN spi1_miso[] = {
#ifndef SPI1_MISO_PIN
	{
		RTE_SPI1_MISO0_PORT,
		RTE_SPI1_MISO0_BIT,
		(PORT_FUNC)RTE_SPI1_MISO0_FUNC,
	},

#else
	SPI1_MISO_PIN,
#endif
};

/* SPI1 Control Information */
static SPI_INFO SPI1_Ctrl = { 0 };
/* SPI1 Resources */
static SPI_RESOURCES SPI1_Resources = {
	SPI1,
	SPI1_IRQn,
	&IpgClock,
	spi1_sck,
	spi1_ss,
	spi1_mosi,
	spi1_miso,
	&SPI1_Ctrl,
#if USE_DMA
	&DmaArray[1][0],//dma_tx
	&DmaArray[1][1],//dma_rx
#endif
};

#endif



static const SPI_RESOURCES *spiArray[2]=
{
#if RTE_SPI0
	&SPI0_Resources,
#else
	NULL,
#endif
#if RTE_SPI1
	&SPI1_Resources,
#else
	NULL,
#endif

};

/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion = {
	ARM_SPI_API_VERSION,
	ARM_SPI_DRV_VERSION
};



/* Driver Capabilities */
static const ARM_SPI_CAPABILITIES DriverCapabilities = {
	1, /* Simplex Mode (Master and Slave) */
	1, /* TI Synchronous Serial Interface */
	0, /* Microwire Interface */
	1  /* Signal Mode Fault event: \ref ARM_SPI_EVENT_MODE_FAULT */
};

//
//static Functions
//

static uint32_t pow(uint32_t m, uint32_t n)
{
	uint32_t sum = m;
	uint32_t i;
	if (n == 0)
		return 1;
	for (i = 1; i < n; i++) sum *= m;
	return sum;
}

static int Calculate2Exponential(uint32_t num, uint32_t *p_x, uint32_t *p_y)
{
	uint32_t x, y;

	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			if (num == (x + 1) * pow(2, y + 1))
			{
				*p_x = x;
				*p_y = y;
				return ARM_DRIVER_OK;
			}
		}
	}
	return ARM_DRIVER_ERROR;
}



static int SetSpiSpeed(SPI_RESOURCES *spi, uint32_t speed)
{
	uint32_t div;
	uint32_t divl, divh;
	div = *spi->clk / speed;
	if (div < SPI_DIV_MIN || div > SPI_DIV_MAX)
	{
		return ARM_DRIVER_ERROR;
	}
	if (ARM_DRIVER_ERROR == Calculate2Exponential(div, &divh, &divl))
	{
		return ARM_DRIVER_ERROR;
	}
	div = (divh << 4) | divl;
	return (int)div;
}




//
//  Functions
//

/**
 * Get SPI driver version.
 * 
 * @return ARM_DRV_VERSION
 */
static ARM_DRIVER_VERSION ARM_SPI_GetVersion(void)
{
	return DriverVersion;
}

/**
 * Get driver capabilities.
 * 
 * @return ARM_SPI_CAPABILITIES
 */
static ARM_SPI_CAPABILITIES ARM_SPI_GetCapabilities(void)
{
	return DriverCapabilities;
}

/**
 * Initialize SPI Interface.
 * 
 * @param spi      Pointer to SPI resources
 * @param cb_event Pointer to \ref ARM_SPI_SignalEvent
 * 
 * @return execution_status
 */
static int32_t SPI_Initialize(SPI_RESOURCES *spi, ARM_SPI_SignalEvent_t cb_event)
{
	if (spi->info->flags & SPI_INIT)
	{
		return ARM_DRIVER_OK;
	}
	PORT_PinConfigure(spi->ss[spi->info->pin_group].port,
					  spi->ss[spi->info->pin_group].pin,
					  spi->ss[spi->info->pin_group].func);
	PORT_PinConfigure(spi->sck[spi->info->pin_group].port,
					  spi->sck[spi->info->pin_group].pin,
					  spi->sck[spi->info->pin_group].func);
	PORT_PinConfigure(spi->mosi[spi->info->pin_group].port,
					  spi->mosi[spi->info->pin_group].pin,
					  spi->mosi[spi->info->pin_group].func);
	PORT_PinConfigure(spi->miso[spi->info->pin_group].port,
					  spi->miso[spi->info->pin_group].pin,
					  spi->miso[spi->info->pin_group].func);

	spi->reg->SPITXFCR = 0;
	spi->reg->SPIRXFCR = 0;
	spi->info->cb_event = cb_event;
	spi->info->flags    = SPI_INIT;

	return ARM_DRIVER_OK;
}

/**
 * De-initialize SPI Interface.
 * 
 * @param spi    spi  Pointer to SPI resources
 * 
 * @return execution_status
 */
static int32_t SPI_Uninitialize(SPI_RESOURCES *spi)
{
	spi->info->flags = 0U;
	PORT_PinConfigure(spi->ss[spi->info->pin_group].port,
					  spi->ss[spi->info->pin_group].pin,
					  PORT_FUNC_GPIO);
	PORT_PinConfigure(spi->sck[spi->info->pin_group].port,
					  spi->sck[spi->info->pin_group].pin,
					  PORT_FUNC_GPIO);
	PORT_PinConfigure(spi->mosi[spi->info->pin_group].port,
					  spi->mosi[spi->info->pin_group].pin,
					  PORT_FUNC_GPIO);
	PORT_PinConfigure(spi->miso[spi->info->pin_group].port,
					  spi->miso[spi->info->pin_group].pin,
					  PORT_FUNC_GPIO);
	return ARM_DRIVER_OK;
}

/**
 * Control SPI Interface Power.
 * 
 * @param spi    Pointer to SPI resources
 * @param state  Power state
 * 
 * @return execution_status
 */
static int32_t SPI_PowerControl(SPI_RESOURCES *spi, ARM_POWER_STATE state)
{
	switch (state)
	{
	case ARM_POWER_OFF:
		/* Power off SPI peripheral */
		NVIC_DisableIRQ(spi->irqn);
		/* Reset status */
		spi->info->status.busy       = 0U;
		spi->info->status.data_lost  = 0U;

		SPI_CLEAR_TX_FIFO(spi->reg);
		SPI_CLEAR_RX_FIFO(spi->reg);

		spi->info->flags  &= ~SPI_POWER;
		break;

//    case ARM_POWER_LOW:
//        break;

	case ARM_POWER_FULL:
		if ((spi->info->flags & SPI_INIT) == 0U)
		{
			return ARM_DRIVER_ERROR;
		}
		if ((spi->info->flags & SPI_POWER) != 0U)
		{
			return ARM_DRIVER_OK;
		}
		SPI_CLEAR_TX_FIFO(spi->reg);
		SPI_CLEAR_RX_FIFO(spi->reg);
		/* Init peripheral (enable clock + reset) */
//		CCM_SPI_Init(spi->reg);

		/* Reset status */
		spi->info->status.busy       = 0U;
		spi->info->status.data_lost  = 0U;


		//DMA Threshold
		spi->reg->SPIDMATHR_b.RXDMATH=0;
		spi->reg->SPIDMATHR_b.TXDMATH=3;
		/* Enable peripheral interrupts in NVIC */
		/*disable SPI interrupts */
		spi->reg->SPICR1_b.SPIE = 0;
		NVIC_ClearPendingIRQ(spi->irqn);
		NVIC_EnableIRQ(spi->irqn);


		spi->info->flags |= SPI_POWER;
		break;

	case ARM_POWER_LOW:
		return ARM_DRIVER_ERROR_UNSUPPORTED;
	}
	return ARM_DRIVER_OK;
}

#if USE_DMA
static void DMA_TransferSpi(SPI_RESOURCES *spi,void*t_data,uint32_t t_len,void*r_data,uint32_t r_len)
{
	/* Memory to peripheral */

	if (t_data&&t_len)
	{
		apDMA_PeripheralAddressSet(
			spi->dma_tx->channel,
			(void *)&spi->reg->SPIDR_L,
			(apDMA_rTransferTerminated)DMA_PeripheralCallBack);

		apDMA_PeripheralConfigure(
			spi->dma_tx->peripheral,
			apDMA_BURST_1,
			apDMA_WIDTH_8_BIT,
			apDMA_AHB_BUS_1,
			apDMA_SYNC_ENABLE,
			0);

		apDMA_MemPeripheralTransferRequest(
			spi->dma_tx->channel,
			spi->dma_tx->peripheral,
			spi->dma_tx->user_id,
			t_len,
			apDMA_MEM_TO_PERIPHERAL_DMA_CTRL,
			t_data);
	}

	if (r_data&&r_len)
	{
		apDMA_PeripheralConfigure(
			spi->dma_rx->peripheral,
			apDMA_BURST_1,
			apDMA_WIDTH_8_BIT,
			apDMA_AHB_BUS_1,
			apDMA_SYNC_ENABLE,
			0);

		apDMA_PeripheralAddressSet(
			spi->dma_rx->channel,
			(void *)&SPI0->SPIDR_L,
			(apDMA_rTransferTerminated)DMA_PeripheralCallBack);


		apDMA_MemPeripheralTransferRequest(
			spi->dma_rx->channel,
			spi->dma_rx->peripheral,
			spi->dma_rx->user_id,
			r_len,
			apDMA_PERIPHERAL_TO_MEM_DMA_CTRL,
			r_data
			);
	}
}

#endif
/**
 * Start sending data to SPI transmitter.
 * 
 * @param spi    Pointer to SPI resources
 * @param data   Pointer to buffer with data to send to SPI transmitter
 * @param num    Number of data items to send
 * 
 * @return execution_status
 */
static int32_t SPI_Send(SPI_RESOURCES *spi, const void *data, uint32_t num)
{
	if ((data == NULL) || (num == 0U))
	{
		return ARM_DRIVER_ERROR_PARAMETER;
	}
	if ((spi->info->flags & SPI_SETUP) == 0U)
	{
		return ARM_DRIVER_ERROR;
	}
	if (spi->info->status.busy)
	{
		return ARM_DRIVER_ERROR_BUSY;
	}

	spi->reg->SPICR1_b.SPIE = 0;
	spi->reg->SPITXFCR_b.TXFSTHIE = 0;
	spi->reg->SPITXFCR_b.TXFSTHIE = 0;



	spi->info->status.busy       = 1U;
	spi->info->status.data_lost  = 0U;

	spi->info->xfer.rx_buf = NULL;
	spi->info->xfer.tx_buf = (const uint8_t *)data;
	spi->info->xfer.num    = num;
	spi->info->xfer.rx_cnt = 0U;
	spi->info->xfer.tx_cnt = 0U;

	SPI_CLEAR_TX_FIFO(spi->reg);
	SPI_CLEAR_RX_FIFO(spi->reg);

	if (spi->info->xfer.data_bits > 8)
	{
		spi->reg->SPITXFCR_b.TXFSTH = num > (SPI_FIFO_DEEP / 2) ? (SPI_FIFO_DEEP / 4) : 0;
		spi->reg->SPIRXFCR_b.RXFSTH = num > (SPI_FIFO_DEEP / 2) ? (SPI_FIFO_DEEP / 4) : num;
	}
	else
	{
		spi->reg->SPITXFCR_b.TXFSTH = num > SPI_FIFO_DEEP ? (SPI_FIFO_DEEP / 2) : 0;
		spi->reg->SPIRXFCR_b.RXFSTH = num > SPI_FIFO_DEEP ? (SPI_FIFO_DEEP / 2) : num;
	}




	// single line mode
	if (1 == spi->reg->SPICR2_b.SPC0)
	{
		if (1 == spi->reg->SPICR1_b.MSTR)
		{
			//master receivee MOSI set output bit 1
			spi->reg->SPIDDR |= (uint8_t)(1 << 1);
		}
		else
		{
			// slave receive  MISO set output bit 0
			spi->reg->SPIDDR |= (uint8_t)(1 << 0);
		}
	}
	NVIC_ClearPendingIRQ(spi->irqn);
#if USE_DMA
	DMA_TransferSpi(spi,(void*)data, num, NULL, 0);
	spi->reg->SPIDMACR_b.RXDMAE = 1;
	spi->reg->SPIDMACR_b.TXDMAE = 1;
	spi->reg->SPICR1_b.SPIE=1;//EOTF interrupt
#else
	spi->reg->SPIRXFCR_b.RXFSTHIE = 1;
	spi->reg->SPITXFCR_b.TXFSTHIE = 1;
#endif
	

	return ARM_DRIVER_OK;
}

/**
 * Start receiving data from SPI receiver.
 * 
 * @param spi    Pointer to SPI resources
 * @param data   Pointer to buffer for data to receive from SPI receiver
 * @param num    Number of data items to receive
 * 
 * @return execution_status
 */
static int32_t SPI_Receive(SPI_RESOURCES *spi, void *data, uint32_t num)
{
	if ((data == NULL) || (num == 0U))
	{
		return ARM_DRIVER_ERROR_PARAMETER;
	}
	if ((spi->info->flags & SPI_SETUP) == 0U)
	{
		return ARM_DRIVER_ERROR;
	}
	if (spi->info->status.busy)
	{
		return ARM_DRIVER_ERROR_BUSY;
	}

	spi->reg->SPICR1_b.SPIE = 0;
	spi->reg->SPITXFCR_b.TXFSTHIE = 0;
	spi->reg->SPITXFCR_b.TXFSTHIE = 0;

	spi->info->status.busy       = 1U;
	spi->info->status.data_lost  = 0U;
	spi->info->xfer.rx_buf = (uint8_t *)data;
	spi->info->xfer.tx_buf = NULL;
	spi->info->xfer.num    = num;
	spi->info->xfer.rx_cnt = 0U;
	spi->info->xfer.tx_cnt = 0U;

	SPI_CLEAR_TX_FIFO(spi->reg);
	SPI_CLEAR_RX_FIFO(spi->reg);

	if (spi->info->xfer.data_bits > 8)
	{
		spi->reg->SPITXFCR_b.TXFSTH = num > (SPI_FIFO_DEEP / 2) ? (SPI_FIFO_DEEP / 4) : 0;
		spi->reg->SPIRXFCR_b.RXFSTH = num > (SPI_FIFO_DEEP / 2) ? (SPI_FIFO_DEEP / 4) : num;
	}
	else
	{
		spi->reg->SPITXFCR_b.TXFSTH = num > SPI_FIFO_DEEP ? (SPI_FIFO_DEEP / 2) : 0;
		spi->reg->SPIRXFCR_b.RXFSTH = num > SPI_FIFO_DEEP ? (SPI_FIFO_DEEP / 2) : num;
	}



// single line mode
	if (1 == spi->reg->SPICR2_b.SPC0)
	{
		if (1 == spi->reg->SPICR1_b.MSTR)
		{
			//master receivee MOSI set input bit 1
			spi->reg->SPIDDR &= (uint8_t)~(1 << 1);
		}
		else
		{
			// slave receive  MISO set input bit 0
			spi->reg->SPIDDR &= (uint8_t)~(1 << 0);
		}
	}
	NVIC_ClearPendingIRQ(spi->irqn);
#if USE_DMA
	DMA_TransferSpi(spi,NULL, 0,data, num);
	spi->reg->SPIDMACR_b.RXDMAE = 1;
	spi->reg->SPIDMACR_b.TXDMAE = 1;
	spi->reg->SPICR1_b.SPIE=1;//EOTF interrupt
#else
	spi->reg->SPIRXFCR_b.RXFSTHIE = 1;
	spi->reg->SPITXFCR_b.TXFSTHIE = 1;
#endif
	


	return ARM_DRIVER_OK;
}

/**
 * Start sending/receiving data to/from SPI transmitter/receiver.
 * 
 * @param spi      Pointer to SPI resources
 * @param data_out Pointer to buffer with data to send to SPI transmitter
 * @param data_in  Pointer to buffer for data to receive from SPI receiver
 * @param num      Number of data items to transfer
 * 
 * @return execution_status
 */
static int32_t SPI_Transfer(SPI_RESOURCES *spi, const void *data_out, void *data_in, uint32_t num)
{
//		uint32_t data;
//uint32_t tx_fifo_left;
	if ((data_out == NULL) || (data_in == NULL) || (num == 0U))
	{
		return ARM_DRIVER_ERROR_PARAMETER;
	}
	if ((spi->info->flags & SPI_SETUP) == 0U)
	{
		return ARM_DRIVER_ERROR;
	}
	if (spi->info->status.busy)
	{
		return ARM_DRIVER_ERROR_BUSY;
	}
	spi->reg->SPICR1_b.SPIE = 0;
	spi->reg->SPITXFCR_b.TXFSTHIE = 0;
	spi->reg->SPITXFCR_b.TXFSTHIE = 0;



	spi->info->status.busy       = 1U;
	spi->info->status.data_lost  = 0U;
	spi->info->xfer.rx_buf = (uint8_t *)data_in;
	spi->info->xfer.tx_buf = (const uint8_t *)data_out;
	spi->info->xfer.num    = num;
	spi->info->xfer.rx_cnt = 0U;
	spi->info->xfer.tx_cnt = 0U;

	//SPI_CLEAR_TX_FIFO(spi->reg);
	//SPI_CLEAR_RX_FIFO(spi->reg);



	spi->reg->SPITXFCR_b.TXFSTH = 0;
	spi->reg->SPIRXFCR_b.RXFSTH = 3;


	NVIC_ClearPendingIRQ(spi->irqn);
#if USE_DMA
	DMA_TransferSpi(spi,(void*)data_out, num, data_in, num);
	spi->reg->SPIDMACR_b.RXDMAE = 1;
	spi->reg->SPIDMACR_b.TXDMAE = 1;
	spi->reg->SPICR1_b.SPIE=1;//EOTF interrupt
#else
	spi->reg->SPIRXFCR_b.RXFSTHIE = 1;
	spi->reg->SPITXFCR_b.TXFSTHIE = 1;
#endif
	
	return ARM_DRIVER_OK;
}

/**
 * Get transferred data count.
 * 
 * @param spi    spi  Pointer to SPI resources
 * 
 * @return number of data items transferred
 *       
 */
static uint32_t SPI_GetDataCount(SPI_RESOURCES *spi)
{
	return (spi->info->xfer.rx_cnt);
}

/**
 * Control SPI Interface.
 * 
 * @param spi     Pointer to SPI resources
 * @param control operation
 * @param arg     argument of operation (optional)
 * 
 * @return execution_status
 */
static int32_t SPI_Control(SPI_RESOURCES *spi, uint32_t control, uint32_t arg)
{
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif
	SPI_Type reg = { 0 };
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic pop
#endif
	uint32_t n;
	int value = 0;

	memcpy((&reg), spi->reg, sizeof(SPI_Type));
	if ((spi->info->flags & SPI_POWER) == 0U)
	{
		return ARM_DRIVER_ERROR;
	}
	if ((control & ARM_SPI_CONTROL_Msk) == ARM_SPI_ABORT_TRANSFER)
	{
		/* Abort data transfer */
		spi->reg->SPICR1_b.SPE = 0;

		spi->info->xfer.rx_cnt = 0U;
		spi->info->xfer.tx_cnt = 0U;
		spi->info->status.busy = 0U;

		return ARM_DRIVER_OK;
	}
	if (spi->info->status.busy)
	{
		return ARM_DRIVER_ERROR_BUSY;
	}
	switch (control & ARM_SPI_CONT_MODE_Msk)
	{
	case ARM_SPI_CONT_MODE_EN:
		if (arg < 1)
		{
			return ARM_DRIVER_ERROR_PARAMETER;
		}
		spi->reg->SPIFR_b.CONT = 1;
		spi->reg->SPITCNT_L = arg & 0xff;
		spi->reg->SPITCNT_H = arg >> 8;
		return ARM_DRIVER_OK;
	case ARM_SPI_CONT_MODE_DIS:
		spi->reg->SPIFR_b.CONT = 0;
		return ARM_DRIVER_OK;
	}

	switch (control & ARM_SPI_SWP_PIN_Msk)
	{
	case ARM_SPI_SWP_PIN_GROUP:
		if (spi->info->flags & SPI_SS_SWCTRL)
		{
			PORT_PinConfigure(spi->ss[arg].port,
							  spi->ss[arg].pin,
							  PORT_FUNC_GPIO);
			GPIO_SetPinDir(spi->ss[arg].port,
						   spi->ss[arg].pin,
						   GPIO_OUTPUT);
		}
		else
		{
			PORT_PinConfigure(spi->ss[arg].port,
							  spi->ss[arg].pin,
							  spi->ss[arg].func);
		}


		PORT_PinConfigure(spi->sck[arg].port,
						  spi->sck[arg].pin,
						  spi->sck[arg].func);
		if (!(reg.SPICR2_b.SPC0 && reg.SPICR1_b.MSTR))
		{

			PORT_PinConfigure(spi->mosi[arg].port,
							  spi->mosi[arg].pin,
							  spi->mosi[arg].func);
		}
		if (!(reg.SPICR2_b.SPC0 && (!reg.SPICR1_b.MSTR)))
		{
			PORT_PinConfigure(spi->miso[arg].port,
							  spi->miso[arg].pin,
							  spi->miso[arg].func);
		}
		spi->info->pin_group = (uint8_t)arg;
		return ARM_DRIVER_OK;

	case ARM_SPI_SWP_MOSI_MISO:
		spi->reg->SPIIR_b.PSW = (arg > 0) ? 1 : 0;
		return ARM_DRIVER_OK;

	}


	switch (control & ARM_SPI_DELAY_Msk)
	{
	case ARM_SPI_GT_TIME:
		if (arg < 0x40)
		{
			spi->reg->SPIFR_b.GTE = 1;
			spi->reg->SPICR2_b.GT = (uint8_t)arg;
			return ARM_DRIVER_OK;
		}
		return ARM_DRIVER_ERROR_PARAMETER;
	case ARM_BEFORE_SCK_TIME:
		if (arg < 0x80)
		{
			spi->reg->SPIBSCDR_b.BSCDE = 1;
			spi->reg->SPIBSCDR |= arg;
			return ARM_DRIVER_OK;
		}
		return ARM_DRIVER_ERROR_PARAMETER;
	case ARM_AFTER_SCK_TIME:
		if (arg < 0x80)
		{
			spi->reg->SPIASCDR_b.ASCDE = 1;
			spi->reg->SPIASCDR |= arg;
			return ARM_DRIVER_OK;
		}
		return ARM_DRIVER_ERROR_PARAMETER;

	}



	switch (control & ARM_SPI_THRESHOLD_Msk)
	{
	case ARM_SPI_TX_THRESHOLD:
		if (arg < 8)
		{
			spi->reg->SPITXFCR_b.TXFSTH = (uint8_t)(SPI_SPITXFCR_TXFSTHIE_Msk | arg);
			return ARM_DRIVER_OK;
		}
		return ARM_DRIVER_ERROR_PARAMETER;
	case ARM_SPI_RX_THRESHOLD:
		if (arg < 8)
		{
			spi->reg->SPIRXFCR_b.RXFSTH = (uint8_t)(SPI_SPIRXFCR_RXFSTHIE_Msk | arg);
			return ARM_DRIVER_OK;
		}
		return ARM_DRIVER_ERROR_PARAMETER;
	}
	switch (control & ARM_SPI_CONTROL_Msk)
	{
	

	case ARM_SPI_MODE_INACTIVE:             // SPI Inactive
		spi->reg->SPICR1_b.SPE = 0;
		spi->info->flags &= ~SPI_SETUP;
		return ARM_DRIVER_OK;

	case ARM_SPI_MODE_MASTER:               // SPI Master (Output on MOSI, Input on MISO); arg = Bus Speed in bps
		reg.SPICR1_b.MSTR = 1;
		value = SetSpiSpeed(spi, arg);
		if (value == ARM_DRIVER_ERROR)
		{
			return ARM_DRIVER_ERROR;
		}

		break;

	case ARM_SPI_MODE_SLAVE:                // SPI Slave  (Output on MISO, Input on MOSI)
		reg.SPICR1_b.MSTR = 0;
		break;

	case ARM_SPI_MODE_MASTER_SIMPLEX:       // SPI Master (Output/Input on MOSI); arg = Bus Speed in bps
		reg.SPICR1_b.MSTR = 1;
		reg.SPICR2_b.SPC0 = 1;
		value = SetSpiSpeed(spi, arg);
		if (value == ARM_DRIVER_ERROR)
		{
			return ARM_DRIVER_ERROR;
		}
		PORT_PinConfigure(spi->miso[spi->info->pin_group].port,
						  spi->miso[spi->info->pin_group].pin,
						  PORT_FUNC_GPIO);

		break;
	case ARM_SPI_MODE_SLAVE_SIMPLEX:        // SPI Slave  (Output/Input on MISO)
		reg.SPICR1_b.MSTR = 0;
		reg.SPICR2_b.SPC0 = 1;
		PORT_PinConfigure(spi->mosi[spi->info->pin_group].port,
						  spi->mosi[spi->info->pin_group].pin,
						  PORT_FUNC_GPIO);
		break;

	case ARM_SPI_SET_BUS_SPEED:             // Set Bus Speed in bps; arg = value
		value = SetSpiSpeed(spi, arg);
		if (value == ARM_DRIVER_ERROR)
		{
			return ARM_DRIVER_ERROR;
		}
		spi->reg->SPIBR = (uint8_t)value;
		return ARM_DRIVER_OK;

	case ARM_SPI_GET_BUS_SPEED:             // Get Bus Speed in bps
		n = reg.SPIBR;
		n = *spi->clk / (((n & 0x38) >> 1) | (n & 0x7));
		return (int32_t)n;

	case ARM_SPI_SET_DEFAULT_TX_VALUE:      // Set default Transmit value; arg = value
		spi->info->xfer.def_val = arg;
		return ARM_DRIVER_OK;

	case ARM_SPI_CONTROL_SS:                // Control Slave Select; arg = 0:inactive, 1:active
		if (spi->info->flags & SPI_SS_SWCTRL)
		{
			if (arg == ARM_SPI_SS_INACTIVE)
			{
				GPIO_PinWrite(spi->ss[spi->info->pin_group].port,
							  spi->ss[spi->info->pin_group].pin,
							  1);
			}
			else
			{
				GPIO_PinWrite(spi->ss[spi->info->pin_group].port,
							  spi->ss[spi->info->pin_group].pin,
							  0);
			}
			return ARM_DRIVER_OK;
		}
		return ARM_DRIVER_ERROR;


	case ARM_SPI_SET_BITSWIDTH:
		spi->info->xfer.data_bits = arg;
		if (spi->info->xfer.data_bits > 16U)
		{
			return ARM_SPI_ERROR_DATA_BITS;
		}
		spi->reg->SPIFR_b.FMSZ = spi->info->xfer.data_bits - 1;
		return ARM_DRIVER_OK;
	default:
		return ARM_DRIVER_ERROR_UNSUPPORTED;

	}
	if (((control & ARM_SPI_CONTROL_Msk) == ARM_SPI_MODE_MASTER) ||
		((control & ARM_SPI_CONTROL_Msk) == ARM_SPI_MODE_MASTER_SIMPLEX))
	{
		switch (control & ARM_SPI_SS_MASTER_MODE_Msk)
		{
			//	case ARM_SPI_SS_MASTER_UNUSED:
			//		if (spi->ss != NULL)
			//		{
			//			PORT_PinConfigure(spi->ss[spi->info->pin_group].port,
			//							  spi->ss[spi->info->pin_group].pin,
			//							  PORT_FUNC_GPIO);

			//		}
			//		spi->info->flags &= ~SPI_SS_SWCTRL;
			//		break;
		case ARM_SPI_SS_MASTER_HW_INPUT:      // SPI Slave Select when Master: Hardware monitored Input
			return ARM_SPI_ERROR_SS_MODE;
		case ARM_SPI_SS_MASTER_SW:
			if (spi->ss == NULL)
			{
				return ARM_SPI_ERROR_SS_MODE;
			}
			spi->info->flags |= SPI_SS_SWCTRL;
			reg.SPICR1_b.SSOE = 0;
			spi->reg->SPIDDR = 0xe;
			PORT_PinConfigure(spi->ss[spi->info->pin_group].port,
							  spi->ss[spi->info->pin_group].pin,
							  PORT_FUNC_GPIO);
			GPIO_SetPinDir(spi->ss[spi->info->pin_group].port,
						   spi->ss[spi->info->pin_group].pin,
						   GPIO_OUTPUT);
			break;
		case ARM_SPI_SS_MASTER_HW_OUTPUT:     // SPI Slave Select when Master: Hardware controlled Output
			if (spi->ss == NULL)
			{
				return ARM_SPI_ERROR_SS_MODE;
			}
			spi->info->flags &= ~SPI_SS_SWCTRL;
			reg.SPICR1_b.SSOE = 1;
			spi->reg->SPIDDR = 0xe;
			PORT_PinConfigure(spi->ss[spi->info->pin_group].port,
							  spi->ss[spi->info->pin_group].pin,
							  spi->ss[spi->info->pin_group].func);
			break;
		default:
			return ARM_SPI_ERROR_SS_MODE;

		}
	}




	if (control & ARM_SPI_HIGH_SPEED_MODE_Msk)
	{
		spi->reg->SPIIR_b.HS = 1;
	}
	else
	{
		spi->reg->SPIIR_b.HS = 0;
	}
	spi->reg->SPIIR|=1<<5;//bug
	if (((control & ARM_SPI_CONTROL_Msk) == ARM_SPI_MODE_SLAVE) ||
		((control & ARM_SPI_CONTROL_Msk) == ARM_SPI_MODE_SLAVE_SIMPLEX))
	{
		switch (control & ARM_SPI_SS_SLAVE_MODE_Msk)
		{
		case ARM_SPI_SS_SLAVE_HW:
			if (spi->ss == NULL)
			{
				return ARM_SPI_ERROR_SS_MODE;
			}
			spi->reg->SPIDDR = 0x1;
			PORT_PinConfigure(spi->ss[spi->info->pin_group].port,
							  spi->ss[spi->info->pin_group].pin,
							  spi->ss[spi->info->pin_group].func);
			break;
		case ARM_SPI_SS_SLAVE_SW:             // SPI Slave Select when Slave: Software controlled
		default:
			return ARM_SPI_ERROR_SS_MODE;
		}
	}



	switch (control & ARM_SPI_FRAME_FORMAT_Msk)
	{
	case ARM_SPI_CPOL0_CPHA0:
		reg.SPICR1 &= ~(SPI_SPICR1_CPHA_Msk | SPI_SPICR1_CPOL_Msk);
		break;
	case ARM_SPI_CPOL0_CPHA1:
		reg.SPICR1_b.CPHA = 1;
		reg.SPICR1_b.CPOL = 0;
		break;
	case ARM_SPI_CPOL1_CPHA0:
		reg.SPICR1_b.CPHA = 0;
		reg.SPICR1_b.CPOL = 1;
		break;

	case ARM_SPI_CPOL1_CPHA1:
		reg.SPICR1 |= (SPI_SPICR1_CPHA_Msk | SPI_SPICR1_CPOL_Msk);
		break;

	case ARM_SPI_TI_SSI:
	case ARM_SPI_MICROWIRE:
	default:
		return ARM_SPI_ERROR_FRAME_FORMAT;
	}



	/* Configure Number of Data Bits */
	spi->info->xfer.data_bits = ((control & ARM_SPI_DATA_BITS_Msk) >> ARM_SPI_DATA_BITS_Pos);
	if (spi->info->xfer.data_bits > 16U)
	{
		return ARM_SPI_ERROR_DATA_BITS;
	}
	reg.SPIFR_b.FMSZ = spi->info->xfer.data_bits - 1;

	/* Configure Bit Order */
	if ((control & ARM_SPI_BIT_ORDER_Msk) == ARM_SPI_LSB_MSB)
	{
		reg.SPICR1_b.LSBFE = 1;
	}
	else
	{
		reg.SPICR1_b.LSBFE = 0;
	}

	spi->reg->SPICR1 = reg.SPICR1;
	spi->reg->SPICR2 = reg.SPICR2;
	spi->reg->SPIFR = reg.SPIFR;
	spi->reg->SPIBR = (uint8_t)value;
	spi->reg->SPICR1_b.SPE = 1; //enable spi
	spi->info->flags |= SPI_SETUP;
	return ARM_DRIVER_OK;
}

/**
 * Get SPI status.
 * 
 * @param spi    Pointer to SPI resources
 * 
 * @return status \ref ARM_SPI_STATUS
 */
static ARM_SPI_STATUS SPI_GetStatus(SPI_RESOURCES *spi)
{
	return spi->info->status;
}

//void SPI_SignalEvent(SPI_RESOURCES spi,uint32_t event)
//{
//    // function body
//}


#if USE_DMA
static void DMA_PeripheralCallBack(uint32_t    UsersId,
								   apError     ResultCode,
								   uint32_t    TransferCount)
{
	u8 s_num=UsersId&0xff;
	u8 isRx=(UsersId>>8)&0xff;
	SPI_RESOURCES *spi=spiArray[s_num];
	if (spi)
	{
		if (isRx)//½ÓÊÕÂú
		{
			
		}
		else
		{
			spi->info->xfer.tx_cnt+=TransferCount;
		}
	}
}
#endif

/**
 * SPI Interrupt handler.
 * 
 * @param spi    Pointer to SPI resources
 */
static void SPI_IRQHandler(const SPI_RESOURCES *spi)
{
	SPI_TRANSFER_INFO *tr = &spi->info->xfer;
	uint32_t event;
//	uint32_t state;
	uint32_t data;
#if !USE_DMA	
	uint32_t tx_fifo_left;
#endif	
//	bool end = false;

	event = 0U;

	if (0 != (spi->reg->SPISR & (SPI_SPISR_TXFTO_Msk | SPI_SPISR_TXFOVF_Msk | SPI_SPISR_TXFUDF_Msk |\
								 SPI_SPISR_RXFTO_Msk | SPI_SPISR_RXFOVF_Msk | SPI_SPISR_RXFUDF_Msk)))
	{
		event = ARM_SPI_EVENT_DATA_LOST;
		tr->num = 0U;
		spi->info->status.busy = 0U;
		spi->reg->SPITXFCR_b.TXFSTHIE = 0;
		spi->reg->SPIRXFCR_b.RXFSTHIE = 0;
		spi->reg->SPICR1_b.SPIE = 0;
		goto irq_end;
	}

#if !USE_DMA
// master mode

//rx interrupt first *********************************
//if (	(spi->reg->SPISR & SPI_SPISR_RXFSER_Msk)   )

//rx_recv:
	{
		while (!spi->reg->SPISR_b.RXFEMP)
		{
			/* Receiver Ready */
			if (spi->info->xfer.data_bits > 8) data |= (spi->reg->SPIDR_H & 0xFFU) << 8;
			data = spi->reg->SPIDR_L & 0xFFU;


			if (tr->rx_cnt < tr->num)
			{
				if (tr->rx_buf)
				{
					*(tr->rx_buf++) = (uint8_t)data;

					if (spi->info->xfer.data_bits > 8)
					{
						*(tr->rx_buf++) = (uint8_t)(data >> 8);
					}
				}
				tr->rx_cnt++;
				if (tr->rx_cnt == tr->num)
				{
					//end

					if (tr->tx_cnt < tr->num)
					{
						event |= ARM_SPI_EVENT_DATA_LOST;
					}
					//transfer end
					event |= ARM_SPI_EVENT_TRANSFER_COMPLETE;
					tr->num = 0U;
					spi->info->status.busy = 0U;
					spi->reg->SPITXFCR_b.TXFSTHIE = 0;
					spi->reg->SPIRXFCR_b.RXFSTHIE = 0;
					spi->reg->SPICR1_b.SPIE = 0;
					goto irq_end;
				}
			}
			else
			{
//				spi->info->status.data_lost = 1U;
//				event |= ARM_SPI_EVENT_DATA_LOST;
				break;
			}

		}

		if (spi->info->xfer.data_bits > 8)
		{
			spi->reg->SPIRXFCR_b.RXFSTH = (tr->num - tr->rx_cnt) > (SPI_FIFO_DEEP / 2) ? (SPI_FIFO_DEEP / 4) : (tr->num - tr->rx_cnt);
		}
		else
		{
			spi->reg->SPIRXFCR_b.RXFSTH = (tr->num - tr->rx_cnt) > SPI_FIFO_DEEP ? (SPI_FIFO_DEEP / 2) : (tr->num - tr->rx_cnt);
		}




		if (spi->reg->SPISR & SPI_SPISR_RXFSER_Msk)
		{
			spi->reg->SPISR = SPI_SPISR_RXFSER_Msk;
		}
	}

//tx interrupt ********************************
//	if ((spi->reg->SPISR & SPI_SPISR_TXFSER_Msk))
	{
		// fill the TX FIFO, at most fill till the FIFO max cnt (max cnt - tx reserve cnt)

		if (spi->info->xfer.data_bits > 8) tx_fifo_left = (SPI_FIFO_DEEP - spi->reg->SPITXFSR_b.TXFCTR) / 2;
		else tx_fifo_left = (SPI_FIFO_DEEP - spi->reg->SPITXFSR_b.TXFCTR);
		while ((!spi->reg->SPISR_b.TXFFULL) && (tx_fifo_left > 0))
		{
			/* Transmitter Ready */
			if (tr->tx_cnt < tr->num)
			{
				if (tr->tx_buf)
				{
					data = *tr->tx_buf++;
					if (spi->info->xfer.data_bits > 8)
					{
						data |= (uint32_t)(*tr->tx_buf++ << 8);
					}
				}
				else
				{
					data = tr->def_val;
				}

				if (spi->info->xfer.data_bits > 8) spi->reg->SPIDR_H = (uint8_t)(data >> 8);
				spi->reg->SPIDR_L = (uint8_t)data;




				tr->tx_cnt++;
				tx_fifo_left--;
				if (tr->tx_cnt >= tr->num)
				{
					spi->reg->SPITXFCR_b.TXFSTHIE = 0; //don't need TXFSTHIE
					spi->reg->SPICR1_b.SPIE = 1;   // wait eof
				}
			}
			else
			{
				/* Unexpected transfer, data is lost */
				//event |= ARM_SPI_EVENT_DATA_LOST;
				break;
			}

		}

		if (spi->info->xfer.data_bits > 8)
		{
			spi->reg->SPITXFCR_b.TXFSTH = (tr->num - tr->tx_cnt) > (SPI_FIFO_DEEP / 2) ? (SPI_FIFO_DEEP / 4) : 0;
		}
		else
		{
			spi->reg->SPITXFCR_b.TXFSTH = (tr->num - tr->tx_cnt) > SPI_FIFO_DEEP ? (SPI_FIFO_DEEP / 2) : 0;
		}


		/*
		if (stat & SPI_SPISR_TXFSER_Msk)
		{
			spi->reg->SPISR = SPI_SPISR_TXFSER_Msk;
		} 
		*/
	}
#endif


	if ((spi->reg->SPISR & SPI_SPISR_EOTF_Msk) && (tr->tx_cnt >= tr->num))
	{
		spi->reg->SPISR = SPI_SPISR_EOTF_Msk;
		if (tr->tx_cnt == tr->num)
		{   //end
			if (tr->rx_cnt < tr->num)
			{
				while (!spi->reg->SPISR_b.RXFEMP)
				{
					/* Receiver Ready */
					if (spi->info->xfer.data_bits > 8) data |= (spi->reg->SPIDR_H & 0xFFU) << 8;
					data = spi->reg->SPIDR_L & 0xFFU;

					if (tr->rx_cnt < tr->num)
					{
						if (tr->rx_buf)
						{
							*(tr->rx_buf++) = (uint8_t)data;

							if (spi->info->xfer.data_bits > 8)
							{
								*(tr->rx_buf++) = (uint8_t)(data >> 8);
							}
						}
						tr->rx_cnt++;
					}
					else
					{
						break;
					}
				}
			}
			if (tr->rx_cnt < tr->num)
			{
				event |= ARM_SPI_EVENT_DATA_LOST;
			}
			//transfer end
			event |= ARM_SPI_EVENT_TRANSFER_COMPLETE;
			tr->num = 0U;
			spi->info->status.busy = 0U;
			spi->reg->SPITXFCR_b.TXFSTHIE = 0;
			spi->reg->SPIRXFCR_b.RXFSTHIE = 0;
			spi->reg->SPICR1_b.SPIE = 0;
		}
	}





	irq_end:
	NVIC_ClearPendingIRQ(spi->irqn);
	/* Send event */
	if ((event != 0U) && (spi->info->cb_event != NULL))
	{
		spi->info->cb_event(event);
	}
}



/**
 * SPI MODF Handler
 * 
 * @param spi    Pointer to SPI resources
 */
static void SPI_MODF_IRQHandler(const SPI_RESOURCES *spi)
{

	//clear modf
	if (spi->reg->SPISR_b.MODF)
	{
		spi->reg->SPICR1 = spi->reg->SPICR1;
		spi->info->cb_event(ARM_SPI_EVENT_MODE_FAULT);
	}
}



// End SPI Interface
#if RTE_SPI0
static int32_t 	SPI0_Initialize(ARM_SPI_SignalEvent_t cb_event)
{
	return SPI_Initialize(&SPI0_Resources, cb_event);
}
static int32_t 	SPI0_Uninitialize(void)
{
	return SPI_Uninitialize(&SPI0_Resources);
}
static int32_t 	SPI0_PowerControl(ARM_POWER_STATE state)
{
	return SPI_PowerControl(&SPI0_Resources, state);
}
static int32_t 	SPI0_Send(const void *data, uint32_t num)
{
	return SPI_Send(&SPI0_Resources, data,  num);
}
static int32_t 	SPI0_Receive(void *data, uint32_t num)
{
	return SPI_Receive(&SPI0_Resources, data,  num);
}
static int32_t 	SPI0_Transfer(const void *data_out, void *data_in, uint32_t num)
{
	return SPI_Transfer(&SPI0_Resources, data_out,  data_in,  num);
}
static uint32_t SPI0_GetDataCount(void)
{
	return SPI_GetDataCount(&SPI0_Resources);
}
static int32_t 	SPI0_Control(uint32_t control, uint32_t arg)
{
	return SPI_Control(&SPI0_Resources, control,  arg);
}
static ARM_SPI_STATUS 	SPI0_GetStatus(void)
{
	return SPI_GetStatus(&SPI0_Resources);
}
void SPI0_IRQHandler(void)
{
	SPI_IRQHandler(&SPI0_Resources);
}
void SPI0_MODF_IRQHandler(void)
{
	SPI_MODF_IRQHandler(&SPI0_Resources);
}

ARM_DRIVER_SPI Driver_SPI0 = {
	ARM_SPI_GetVersion,
	ARM_SPI_GetCapabilities,
	SPI0_Initialize,
	SPI0_Uninitialize,
	SPI0_PowerControl,
	SPI0_Send,
	SPI0_Receive,
	SPI0_Transfer,
	SPI0_GetDataCount,
	SPI0_Control,
	SPI0_GetStatus,
};


#endif



#if RTE_SPI1
static int32_t 	SPI1_Initialize(ARM_SPI_SignalEvent_t cb_event)
{
	return SPI_Initialize(&SPI1_Resources, cb_event);
}
static int32_t 	SPI1_Uninitialize(void)
{
	return SPI_Uninitialize(&SPI1_Resources);
}
static int32_t 	SPI1_PowerControl(ARM_POWER_STATE state)
{
	return SPI_PowerControl(&SPI1_Resources, state);
}
static int32_t 	SPI1_Send(const void *data, uint32_t num)
{
	return SPI_Send(&SPI1_Resources, data,  num);
}
static int32_t 	SPI1_Receive(void *data, uint32_t num)
{
	return SPI_Receive(&SPI1_Resources, data,  num);
}
static int32_t 	SPI1_Transfer(const void *data_out, void *data_in, uint32_t num)
{
	return SPI_Transfer(&SPI1_Resources, data_out,  data_in,  num);
}
static uint32_t SPI1_GetDataCount(void)
{
	return SPI_GetDataCount(&SPI1_Resources);
}
static int32_t 	SPI1_Control(uint32_t control, uint32_t arg)
{
	return SPI_Control(&SPI1_Resources, control,  arg);
}
static ARM_SPI_STATUS 	SPI1_GetStatus(void)
{
	return SPI_GetStatus(&SPI1_Resources);
}
void SPI1_IRQHandler(void)
{
	SPI_IRQHandler(&SPI1_Resources);
}
void SPI1_MODF_IRQHandler(void)
{
	SPI_MODF_IRQHandler(&SPI1_Resources);
}

ARM_DRIVER_SPI Driver_SPI1 = {
	ARM_SPI_GetVersion,
	ARM_SPI_GetCapabilities,
	SPI1_Initialize,
	SPI1_Uninitialize,
	SPI1_PowerControl,
	SPI1_Send,
	SPI1_Receive,
	SPI1_Transfer,
	SPI1_GetDataCount,
	SPI1_Control,
	SPI1_GetStatus,
};


#endif
// End SPI Interface


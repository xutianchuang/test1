/********************************************************************************
  * @file    gsmcu_spi.c
  * @author  jun.zhang
  * @version V1.0.0
  * @date    2020-09-17
  * @brief   SPI HAL module driver.
  *
  ==============================================================================
                        ##### How to use SPI HAL driver #####
  ==============================================================================
      The SPI HAL driver can be used as follows:

      (1) Declare a SPI_HandleTypeDef handle structure, for example:
          SPI_HandleTypeDef  hspi;

      (2)Initialize the SPI low level resources by implementing the HAL_SPI_MspInit() API:
          ① Enable the SPIx interface clock
          ② SPI pins configuration
              (+++) Enable the clock for the SPI GPIOs
              (+++) Configure these SPI pins as alternate function push-pull
          ③ NVIC configuration if you need to use interrupt process
              (+++) Configure the SPIx interrupt priority
              (+++) Enable the NVIC SPI IRQ handle
          ④ DMA Configuration if you need to use DMA process
              (+++) Declare a DMA_HandleTypeDef handle structure for the transmit or receive Channel
              (+++) Enable the DMAx clock
              (+++) Configure the DMA handle parameters 
              (+++) Configure the DMA Tx or Rx Channel
              (+++) Associate the initilalized hdma_tx(or _rx) handle to the hspi DMA Tx (or Rx) handle
              (+++) Configure the priority and enable the NVIC for the transfer complete interrupt on the DMA Tx or Rx Channel

      (3) Program the Mode , Data size, Baudrate Prescaler,Clock polarity and phase, 
		  FirstBit and CRC configuration in the hspi Init structure.

      (4) Initialize the SPI registers by calling the HAL_SPI_Init() API:
          (++) This API configures also the low level Hardware GPIO, CLOCK, CORTEX...etc)
              by calling the customized HAL_SPI_MspInit() API.
 */
 
#include  <gsmcu_hal.h>   
#include  "gsmcu_spi.h"
/* SPI_Private_Functions ------------------------------------------------------------*/
static void SPI_RxISR_16BIT(struct __SPI_HandleTypeDef *hspi);
static void SPI_RxISR_8BIT(struct __SPI_HandleTypeDef *hspi);
static void SPI_CloseRx_ISR(SPI_HandleTypeDef *hspi);
static HAL_StatusTypeDef SPI_WaitTxRxFIFOEmptyUntilTimeout(SPI_HandleTypeDef *hspi, uint8_t TxRxFIFO, uint32_t Timeout, uint32_t Tickstart);
static HAL_StatusTypeDef SPI_WaitFlagStateUntilTimeout(SPI_HandleTypeDef *hspi, uint32_t Flag, uint32_t State, uint32_t Timeout, uint32_t Tickstart);
static HAL_StatusTypeDef SPI_CheckFlag_BSY(SPI_HandleTypeDef *hspi, uint32_t Timeout, uint32_t Tickstart);


/* Exported functions --------------------------------------------------------*/
/** @defgroup SPI_Exported_Functions SPI Exported Functions
  * @{
  */
extern void DMA_AbtChannel(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);


/** @defgroup SPI_Exported_Functions_Group1 Initialization and de-initialization functions
 *  @brief    Initialization and Configuration functions
 *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
    [..]  This subsection provides a set of functions allowing to initialize and
          de-initialize the SPIx peripheral:

      (+) User must implement HAL_SPI_MspInit() function in which he configures
          all related peripherals resources (CLOCK, GPIO, DMA, IT and NVIC ).

      (+) Call the function HAL_SPI_Init() to configure the selected device with
          the selected configuration:
        (++) Mode
        (++) Data Size
        (++) Clock Polarity and Phase
        (++) BaudRate Prescaler
        (++) FirstBit

      (+) Call the function HAL_SPI_DeInit() to restore the default configuration
          of the selected SPIx peripheral.

@endverbatim
  * @{
  */

/**
  * @brief  Initialize the SPI according to the specified parameters
  *         in the SPI_InitTypeDef and initialize the associated handle.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef *hspi)
{
	/* Check the SPI handle allocation */
	if(hspi == NULL)
	{
	  	return HAL_ERROR;
	}
	
	/* Check the parameters */
	assert_param(IS_SPI_ALL_INSTANCE(hspi->Instance));
	assert_param(IS_SPI_FRAME_ACTIVE(hspi->Init.FramePolarity));
	assert_param(IS_SPI_FIRST_BIT(hspi->Init.FirstBit));
	assert_param(IS_SPI_MODE(hspi->Init.Mode));
	assert_param(IS_SPI_CPOL(hspi->Init.CLKPolarity));
	assert_param(IS_SPI_CPHA(hspi->Init.CLKPhase));
	assert_param(IS_SPI_DATASIZE(hspi->Init.DataSize));
	if(hspi->Instance == SSP0)
	{
		SCU->APBCLKG0 |= (APBCLK_SSP0_PCLK_ENABLE | APBCLK_SSP0_SSPCLK_ENABLE);
		SCU->SLP_APBCLKG0 |= (APBCLK_SSP0_PCLK_ENABLE | APBCLK_SSP0_SSPCLK_ENABLE);
	}
	else if(hspi->Instance == SSP1)
	{
		SCU->APBCLKG0 |= (APBCLK_SSP1_PCLK_ENABLE | APBCLK_SSP1_SSPCLK_ENABLE);
		SCU->SLP_APBCLKG0 |= (APBCLK_SSP1_PCLK_ENABLE | APBCLK_SSP1_SSPCLK_ENABLE);
	}
	if(hspi->State == HAL_SPI_STATE_RESET)
	{
	  	/* Allocate lock resource and initialize it */
	  	hspi->Lock = HAL_UNLOCKED;
	
	  	/* Init the low level hardware : GPIO, CLOCK, NVIC... */
	  	HAL_SPI_MspInit(hspi);
	}
	
	hspi->State = HAL_SPI_STATE_BUSY;
	hspi->Instance->IntrCR=1<<SPI_RFTHOD_Pos|4<<SPI_TFTHOD_Pos;// tx & rx fifo config
	/* Disable the selected SPI peripheral */
	__HAL_SPI_DISABLE(hspi);
	
	/*----------------------- SPIx SSPCR0 & SSPCR1,SSPCR2,SSPCR3 Configuration ---------------------*/
	/* SSPCR0 Configure : Frame/Sync. polarity, Frame format, Bit sequence indicator, 
	   Operation mode, SCLK polarity and phase */
	hspi->Instance->SSPCR0 = 0;
	hspi->Instance->SSPCR1 = 0;
	hspi->Instance->SSPCR2 = 0;
	if(hspi->Init.Mode == SPI_MODE_SLAVE)
	{
	  	hspi->Instance->SSPCR0 |= hspi->Init.FramePolarity;
	}
		 
	hspi->Instance->SSPCR0 |= (1 << 17) | (1<<16) | ((1<<12) | hspi->Init.FirstBit | hspi->Init.Mode | 
							    hspi->Init.CLKPolarity | hspi->Init.CLKPhase);
	hspi->Instance->SSPCR2 |= SPI_TXEN | SPI_RXEN | SPI_RXFCLR | SPI_TXFCLR;
	/* SSPCR1 Configure : Serial data length, SCLK divider */
	hspi->Instance->SSPCR1 |= (hspi->Init.DataSize | hspi->Init.SCLKDIV);
	
	/* SSPCR3 Configure : Padding Cycle Length */
	hspi->Instance->SSPCR3 |= hspi->Init.PaddingCycle;
	
	hspi->ErrorCode = HAL_SPI_ERROR_NONE;
	
	hspi->State     = HAL_SPI_STATE_READY;
	
	return HAL_OK;
}

/**
  * @brief  De Initialize the SPI peripheral.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_DeInit(SPI_HandleTypeDef *hspi)
{
  	/* Check the SPI handle allocation */
  	if(hspi == NULL)
  	{
  	  return HAL_ERROR;
  	}
	
  	/* Check SPI Instance parameter */
  	assert_param(IS_SPI_ALL_INSTANCE(hspi->Instance));
	
  	hspi->State = HAL_SPI_STATE_BUSY;
	
  	/* Disable the SPI Peripheral */
  	hspi->Instance->SSPCR2 &= (~0x01);
	
  	/* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
  	HAL_SPI_MspDeInit(hspi);
	
  	hspi->ErrorCode = HAL_SPI_ERROR_NONE;
  	hspi->State = HAL_SPI_STATE_RESET;
	
  	/* Release Lock */
  	__HAL_UNLOCK(hspi);
	
  	return HAL_OK;
}

/**
  * @brief  Initialize the SPI MSP.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
__weak void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
  	/* Prevent unused argument(s) compilation warning */
  	UNUSED(hspi);
  	/* NOTE : This function should not be modified, when the callback is needed,
  	          the HAL_SPI_MspInit should be implemented in the user file
  	*/
}

/**
  * @brief  De-Initialize the SPI MSP.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
__weak void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
  	/* Prevent unused argument(s) compilation warning */
  	UNUSED(hspi);
  	/* NOTE : This function should not be modified, when the callback is needed,
  	          the HAL_SPI_MspDeInit should be implemented in the user file
  	*/
}

/**
  * @}
  */

/** @defgroup SPI_Exported_Functions_Group2 IO operation functions
 *  @brief   Data transfers functions
 *
@verbatim
  ==============================================================================
                      ##### IO operation functions #####
 ===============================================================================
 [..]
    This subsection provides a set of functions allowing to manage the SPI
    data transfers.

    [..] The SPI supports master and slave mode :

    (#) There are two modes of transfer:
       (++) Blocking mode: The communication is performed in polling mode.
            The HAL status of all data processing is returned by the same function
            after finishing transfer.
       (++) No-Blocking mode: The communication is performed using Interrupts
            or DMA, These APIs return the HAL status.
            The end of the data processing will be indicated through the
            dedicated SPI IRQ when using Interrupt mode or the DMA IRQ when
            using DMA mode.
            The HAL_SPI_TxCpltCallback(), HAL_SPI_RxCpltCallback() and HAL_SPI_TxRxCpltCallback() user callbacks
            will be executed respectively at the end of the transmit or Receive process
            The HAL_SPI_ErrorCallback()user callback will be executed when a communication error is detected

    (#) APIs provided for these 2 transfer modes (Blocking mode or Non blocking mode using either Interrupt or DMA)
        exist for 1Line (simplex) and 2Lines (full duplex) modes.

@endverbatim
  * @{
  */

/**
  * @brief  Transmit an amount of data in blocking mode.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @param  pData: pointer to data buffer
  * @param  Size: amount of data to be sent
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  	uint32_t tickstart = 0U;
  	
  	HAL_StatusTypeDef errorcode = HAL_OK;
	
  	/* Process Locked */
  	__HAL_LOCK(hspi);
	
  	/* Init tickstart for timeout management*/
  	tickstart = HAL_GetTick();
	
  	if(hspi->State != HAL_SPI_STATE_READY)
  	{
  	  	errorcode = HAL_BUSY;
  	  	goto error;
  	}
	
  	if(Size == 0U)
  	{
  	  	errorcode = HAL_ERROR;
  	  	goto error;
  	}
	
  	/* Set the transaction information */
  	hspi->State       = HAL_SPI_STATE_BUSY_TX;
  	hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
  	hspi->pTxBuffPtr  = (uint8_t *)pData;
  	hspi->TxXferSize  = Size;
  	hspi->TxXferCount = Size;
	
  	/*Init field not used in handle to zero */
  	hspi->pRxBuffPtr  = (uint8_t *)NULL;
  	hspi->RxXferSize  = 0U;
  	hspi->RxXferCount = 0U;
  	hspi->TxISR       = NULL;
  	hspi->RxISR       = NULL;
	
  	/* Check if the SPI is already enabled */
  	if((hspi->Instance->SSPCR2 & 0x01) != 0x01)
  	{
  	  	/* Enable SPI peripheral */
  	  	__HAL_SPI_ENABLE(hspi);
  	}
	hspi->Instance->SSPCR2 &= ~SPI_RXEN;
  	/* Transmit data in 16 Bit mode */

  	if(hspi->Init.DataSize == SPI_DATASIZE_16BIT)
  	{
  	  	/* Transmit data in 16 Bit mode */
  	  	while (hspi->TxXferCount > 0U)
  	  	{
  	  	  	/* Wait until TXE flag is set to send data */
  	  	  	if(__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TFNF))
  	  	  	{
				if(pData != NULL)
				{
					hspi->Instance->TxRxDR = *((uint16_t *)pData);
					pData += sizeof(uint16_t);
				}
				else
				{
					hspi->Instance->TxRxDR = NULL;
				}
				hspi->TxXferCount--;
  	  	  	}
  	  	  	else
  	  	  	{
  	  	  	  	/* Timeout management */
  	  	  	  	if((Timeout == 0U) || ((Timeout != HAL_MAX_DELAY) && ((HAL_GetTick()-tickstart) >=  Timeout)))
  	  	  	  	{
  	  	  	  		errorcode = HAL_TIMEOUT;
  	  	  	  		goto error;
  	  	  	  	}
  	  	  	}
  	  	}
  	}
  	/* Transmit data in 8 Bit mode */
  	else
  	{
  	  	while (hspi->TxXferCount > 0U)
  	  	{
  	  	  	/* Wait until TXE flag is set to send data */
  	  	  	if(__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TFNF))
  	  	  	{
				if(pData != NULL)
				{
					*((__IO uint8_t*)&hspi->Instance->TxRxDR) = (*pData);
					pData += sizeof(uint8_t);
				}
				else
				{
					hspi->Instance->TxRxDR = NULL;
				}
  	  	  	  	hspi->TxXferCount--;
  	  	  	}
  	  	  	else
  	  	  	{
  	  	  	  	/* Timeout management */
  	  	  	  	if((Timeout == 0U) || ((Timeout != HAL_MAX_DELAY) && ((HAL_GetTick()-tickstart) >=  Timeout)))
  	  	  	  	{
  	  	  	  	  	errorcode = HAL_TIMEOUT;
  	  	  	  	  	goto error;
  	  	  	  	}
  	  	  	}
  	  	}
  	}
	
  	/* Wait until TXE flag */
  	if(SPI_WaitTxRxFIFOEmptyUntilTimeout(hspi, 1, Timeout, tickstart) != HAL_OK)
  	{
  	  	errorcode = HAL_TIMEOUT;
  	  	goto error;
  	}
  	
  	/* Check Busy flag */
  	if(SPI_CheckFlag_BSY(hspi, Timeout, tickstart) != HAL_OK)
  	{
  	  	errorcode = HAL_ERROR;
  	  	hspi->ErrorCode = HAL_SPI_ERROR_FLAG;
  	  	goto error;
  	}
	
  	if(hspi->ErrorCode != HAL_SPI_ERROR_NONE)
  	{
  	  	errorcode = HAL_ERROR;
  	}

error:
	hspi->Instance->SSPCR2 |= SPI_RXEN;
  	hspi->State = HAL_SPI_STATE_READY;
  	/* Process Unlocked */
  	__HAL_UNLOCK(hspi);
  	return errorcode;
}

/**
  * @brief  Receive an amount of data in blocking mode.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @param  pData: pointer to data buffer
  * @param  Size: amount of data to be received
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  	uint32_t tickstart = 0U;
  	HAL_StatusTypeDef errorcode = HAL_OK;
	
  	if(hspi->Init.Mode == SPI_MODE_MASTER)
  	{
  	   	hspi->State = HAL_SPI_STATE_BUSY_RX;
  	   	/* Call transmit-receive function to send Dummy data on Tx line and generate clock on CLK line */
  	  	return HAL_SPI_TransmitReceive(hspi,pData,pData,Size,Timeout);
  	}
	
  	/* Process Locked */
  	__HAL_LOCK(hspi);
	
  	/* Init tickstart for timeout management*/
  	tickstart = HAL_GetTick();
	
  	if(hspi->State != HAL_SPI_STATE_READY)
  	{
  	  errorcode = HAL_BUSY;
  	  goto error;
  	}
	
  	if(Size == 0U)
  	{
  	  errorcode = HAL_ERROR;
  	  goto error;
  	}
	
  	/* Set the transaction information */
  	hspi->State       = HAL_SPI_STATE_BUSY_RX;
  	hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
  	hspi->pRxBuffPtr  = (uint8_t *)pData;
  	hspi->RxXferSize  = Size;
  	hspi->RxXferCount = Size;
	
  	/*Init field not used in handle to zero */
  	hspi->pTxBuffPtr  = (uint8_t *)NULL;
  	hspi->TxXferSize  = 0U;
  	hspi->TxXferCount = 0U;
  	hspi->RxISR       = NULL;
  	hspi->TxISR       = NULL;
	
  	/* Check if the SPI is already enabled */

  	if((hspi->Instance->SSPCR2 & 0x01) != 0x01)
  	{
  	  	/* Enable SPI peripheral */
  	  	__HAL_SPI_ENABLE(hspi);
  	}
	
  	/* Receive data in 8 Bit mode */
  	if(hspi->Init.DataSize == SPI_DATASIZE_8BIT)
  	{
  	  	/* Transfer loop */
  	  	while(hspi->RxXferCount > 0U)
  	  	{
  	  	  	/* Check the RXNE flag */
  	  	  	if(hspi->Instance->SSPStatus & 0x3f0)
  	  	  	{
  	  	  	  	/* read the received data */
				if(pData != NULL)
				{
					(* (uint8_t *)pData)= *(__IO uint8_t *)&hspi->Instance->TxRxDR;
					pData += sizeof(uint8_t);
				}
				else
				{
					uint32_t r_temp = hspi->Instance->TxRxDR;
				}
  	  	  	  	hspi->RxXferCount--;
  	  	  	}
  	  	  	else
  	  	  	{
  	  	  	  	/* Timeout management */
  	  	  	  	if((Timeout == 0U) || ((Timeout != HAL_MAX_DELAY) && ((HAL_GetTick()-tickstart) >=  Timeout)))
  	  	  	  	{
  	  	  	  	  errorcode = HAL_TIMEOUT;
  	  	  	  	  goto error;
  	  	  	  	}
  	  	  	}
  	  	}
  	}
  	else
  	{
  	  	/* Transfer loop */
  	  	while(hspi->RxXferCount > 0U)
  	  	{
  	  	  	/* Check the RXNE flag */
  	  	  	if(hspi->Instance->SSPStatus & 0x3f0)
  	  	  	{
				if(pData != NULL)
				{
					*((uint16_t*)pData) = hspi->Instance->TxRxDR;
					pData += sizeof(uint16_t);
				}
				else
				{
					uint32_t r_temp = hspi->Instance->TxRxDR;
				}
  	  	  	  	hspi->RxXferCount--;
  	  	  	}
  	  	  	else
  	  	  	{
  	  	  	  	/* Timeout management */
  	  	  	  	if((Timeout == 0U) || ((Timeout != HAL_MAX_DELAY) && ((HAL_GetTick()-tickstart) >=  Timeout)))
  	  	  	  	{
  	  	  	  	  errorcode = HAL_TIMEOUT;
  	  	  	  	  goto error;
  	  	  	  	}
  	  	  	}
  	  	}
  	}
  	
  	/* Check the end of the transaction */
  	if(hspi->Init.Mode == SPI_MODE_MASTER)
  	{
  	  	/* Disable SPI peripheral */
  	  	__HAL_SPI_DISABLE(hspi);
  	}
	
  	if(hspi->ErrorCode != HAL_SPI_ERROR_NONE)
  	{
  	  errorcode = HAL_ERROR;
  	}

error :
  	hspi->State = HAL_SPI_STATE_READY;
  	__HAL_UNLOCK(hspi);
  	return errorcode;
}

/**
  * @brief  Transmit and Receive an amount of data in blocking mode.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @param  pTxData: pointer to transmission data buffer
  * @param  pRxData: pointer to reception data buffer
  * @param  Size: amount of data to be sent and received
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout)
{
  	uint32_t tmp = 0U, tmp1 = 0U;
  	uint32_t tickstart = 0U;
  	/* Variable used to alternate Rx and Tx during transfer */
  	HAL_StatusTypeDef errorcode = HAL_OK;
	
  	/* Process Locked */
  	__HAL_LOCK(hspi);
	
  	/* Init tickstart for timeout management*/
  	tickstart = HAL_GetTick();
  	
  	tmp  = hspi->State;
  	tmp1 = hspi->Init.Mode;
  	
  	if(!((tmp == HAL_SPI_STATE_READY) || \
  	  ((tmp1 == SPI_MODE_MASTER) && (tmp == HAL_SPI_STATE_BUSY_RX))))
  	{
  	  	errorcode = HAL_BUSY;
  	  	goto error;
  	}
  	if(Size == 0)
  	{
  	  	errorcode = HAL_ERROR;
  	  	goto error;
  	}
  	/* Don't overwrite in case of HAL_SPI_STATE_BUSY_RX */
  	if(hspi->State == HAL_SPI_STATE_READY)
  	{
  	  	hspi->State = HAL_SPI_STATE_BUSY_TX_RX;
  	}
	
  	/* Set the transaction information */
  	hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
  	hspi->pRxBuffPtr  = (uint8_t *)pRxData;
  	hspi->RxXferCount = Size;
  	hspi->RxXferSize  = Size;
  	hspi->pTxBuffPtr  = (uint8_t *)pTxData;
  	hspi->TxXferCount = Size;
  	hspi->TxXferSize  = Size;
	
  	/*Init field not used in handle to zero */
  	hspi->RxISR       = NULL;
  	hspi->TxISR       = NULL;
	
  	/* Check if the SPI is already enabled */
  	if((hspi->Instance->SSPCR2 & 0x01) != 0x01)
  	{
  	  	/* Enable SPI peripheral */
  	  	__HAL_SPI_ENABLE(hspi);
  	}
	//Clear RXFIFO
	hspi->Instance->SSPCR2 |= SPI_RXFCLR;
  	/* Transmit and Receive data in 16 Bit mode */
  	if(hspi->Init.DataSize == SPI_DATASIZE_16BIT)
  	{
  	  	while ((hspi->TxXferCount > 0U) || (hspi->RxXferCount > 0))
  	  	{
  	  	  	/* Check TFNF flag */
  	  	  	if((hspi->TxXferCount > 0U) && (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TFNF)))
  	  	  	{
				if(pTxData != NULL)
				{
  	  	  	  		hspi->Instance->TxRxDR = *((uint16_t *)pTxData);
					pTxData += sizeof(uint16_t);
				}
				else
				{
					hspi->Instance->TxRxDR = NULL;
				}
  	  	  	  	hspi->TxXferCount--;
  	  	  	}
			
  	  	  	/* Check RFF flag */
  	  	  	if(hspi->Instance->SSPStatus & 0x3f0)
  	  	  	{
				if(pRxData != NULL)
				{
					*((uint16_t *)pRxData) = hspi->Instance->TxRxDR;
					pRxData += sizeof(uint16_t);
				}
				else
				{
					uint32_t r_temp = hspi->Instance->TxRxDR;
				}
  	  	  	  	hspi->RxXferCount--;
  	  	  	}
  	  	  	if((Timeout != HAL_MAX_DELAY) && ((HAL_GetTick()-tickstart) >=  Timeout))
  	  	  	{
  	  	  	  	errorcode = HAL_TIMEOUT;
  	  	  	  	goto error;
  	  	  	}
  	  	}
  	}
  	/* Transmit and Receive data in 8 Bit mode */
  	else
  	{
  	  	while((hspi->TxXferCount > 0U) || (hspi->RxXferCount > 0))
  	  	{
  	  	  	/* check TFNF flag */
  	  	  	if((hspi->TxXferCount > 0U) && (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TFNF)))
  	  	  	{
				if(pRxData != NULL)
				{
  	  	  	  		*(__IO uint8_t *)&hspi->Instance->TxRxDR = (*pTxData++);
				}
				else
				{
					hspi->Instance->TxRxDR = NULL;
				}
  	  	  	  	hspi->TxXferCount--;
  	  	  	}
			
  	  	  	/* Wait until RFF flag is reset */
  	  	  	if(hspi->Instance->SSPStatus & 0x3f0)
  	  	  	{
				if(pRxData != NULL)
				{
  	  	  	  		(*(uint8_t *)pRxData++) = hspi->Instance->TxRxDR;
				}
				else
				{
					uint32_t r_temp = hspi->Instance->TxRxDR;
				}
  	  	  	  	hspi->RxXferCount--;
  	  	  	}
  	  	  	if((Timeout != HAL_MAX_DELAY) && ((HAL_GetTick()-tickstart) >=  Timeout))
  	  	  	{
  	  	  	  	errorcode = HAL_TIMEOUT;
  	  	  	  	goto error;
  	  	  	}
  	  	}
  	}
	
  	/* Wait until TFNF flag */
  	if(SPI_WaitTxRxFIFOEmptyUntilTimeout(hspi, 1, Timeout, tickstart) != HAL_OK)
  	{
  	  	errorcode = HAL_TIMEOUT;
  	  	goto error;
  	}
  	
  	/* Check Busy flag */
  	if(SPI_CheckFlag_BSY(hspi, Timeout, tickstart) != HAL_OK)
  	{
  	  	errorcode = HAL_ERROR;
  	  	hspi->ErrorCode = HAL_SPI_ERROR_FLAG;
  	  	goto error;
  	}
  
error :
  	hspi->State = HAL_SPI_STATE_READY;
  	__HAL_UNLOCK(hspi);
 	return errorcode;
}

//使用dma传输SPI数据
HAL_StatusTypeDef HAL_SPI_TransmitReceiveWithDma(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, void (*callback)(void))
{
  	uint32_t tmp = 0U, tmp1 = 0U;

  	/* Variable used to alternate Rx and Tx during transfer */
  	HAL_StatusTypeDef errorcode = HAL_OK;
	
  	/* Process Locked */
  	__HAL_LOCK(hspi);

  	tmp  = hspi->State;
  	tmp1 = hspi->Init.Mode;
  	
  	if(!((tmp == HAL_SPI_STATE_READY) || \
  	  ((tmp1 == SPI_MODE_MASTER) && (tmp == HAL_SPI_STATE_BUSY_RX))))
  	{
  	  	errorcode = HAL_BUSY;
  	  	goto error;
  	}
  	if(Size == 0)
  	{
  	  	errorcode = HAL_ERROR;
  	  	goto error;
  	}
  	/* Don't overwrite in case of HAL_SPI_STATE_BUSY_RX */
  	if(hspi->State == HAL_SPI_STATE_READY)
  	{
  	  	hspi->State = HAL_SPI_STATE_BUSY_TX_RX;
  	}
	
  	/* Set the transaction information */
  	hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
  	hspi->pRxBuffPtr  = (uint8_t *)pRxData;
  	hspi->RxXferCount = Size;
  	hspi->RxXferSize  = Size;
  	hspi->pTxBuffPtr  = (uint8_t *)pTxData;
  	hspi->TxXferCount = Size;
  	hspi->TxXferSize  = Size;
	
  	/*Init field not used in handle to zero */
  	hspi->RxISR       = NULL;
  	hspi->TxISR       = NULL;
	
  	/* Check if the SPI is already enabled */
  	if((hspi->Instance->SSPCR2 & 0x01) != 0x01)
  	{
  	  	/* Enable SPI peripheral */
  	  	__HAL_SPI_ENABLE(hspi);
  	}
	//Clear RXFIFO
	hspi->Instance->SSPCR2 |= SPI_RXFCLR;
  	/* Transmit and Receive data in 16 Bit mode */
  	if(hspi->Init.DataSize == SPI_DATASIZE_16BIT)
  	{
  	  	while ((hspi->TxXferCount > 0U) || (hspi->RxXferCount > 0))
  	  	{
  	  	  	/* Check TFNF flag */
  	  	  	if((hspi->TxXferCount > 0U) && (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TFNF)))
  	  	  	{
				if(pTxData != NULL)
				{
  	  	  	  		hspi->Instance->TxRxDR = *((uint16_t *)pTxData);
					pTxData += sizeof(uint16_t);
				}
				else
				{
					hspi->Instance->TxRxDR = NULL;
				}
  	  	  	  	hspi->TxXferCount--;
  	  	  	}
			
  	  	  	/* Check RFF flag */
  	  	  	if(hspi->Instance->SSPStatus & 0x3f0)
  	  	  	{
				if(pRxData != NULL)
				{
					*((uint16_t *)pRxData) = hspi->Instance->TxRxDR;
					pRxData += sizeof(uint16_t);
				}
				else
				{
					uint32_t r_temp = hspi->Instance->TxRxDR;
				}
  	  	  	  	hspi->RxXferCount--;
  	  	  	}

  	  	}
  	}
  	/* Transmit and Receive data in 8 Bit mode */
  	else
  	{
  	  	while((hspi->TxXferCount > 0U) || (hspi->RxXferCount > 0))
  	  	{
  	  	  	/* check TFNF flag */
  	  	  	if((hspi->TxXferCount > 0U) && (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TFNF)))
  	  	  	{
				if(pRxData != NULL)
				{
  	  	  	  		*(__IO uint8_t *)&hspi->Instance->TxRxDR = (*pTxData++);
				}
				else
				{
					hspi->Instance->TxRxDR = NULL;
				}
  	  	  	  	hspi->TxXferCount--;
  	  	  	}
			
  	  	  	/* Wait until RFF flag is reset */
  	  	  	if(hspi->Instance->SSPStatus & 0x3f0)
  	  	  	{
				if(pRxData != NULL)
				{
  	  	  	  		(*(uint8_t *)pRxData++) = hspi->Instance->TxRxDR;
				}
				else
				{
					uint32_t r_temp = hspi->Instance->TxRxDR;
				}
  	  	  	  	hspi->RxXferCount--;
  	  	  	}

  	  	}
  	}
	

  
error :
  	hspi->State = HAL_SPI_STATE_READY;
  	__HAL_UNLOCK(hspi);
 	return errorcode;
}
/**
  * @brief  Transmit an amount of data in non-blocking mode with Interrupt.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @param  pData: pointer to data buffer
  * @param  Size: amount of data to be sent
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_Transmit_IT(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size)
{
  	HAL_StatusTypeDef errorcode = HAL_OK;
	
  	/* Process Locked */
  	__HAL_LOCK(hspi);
	
  	if(Size == 0U)
  	{
  	  	errorcode = HAL_ERROR;
  	  	goto error;
  	}
	
  	if(hspi->State != HAL_SPI_STATE_READY)
  	{
  	  	errorcode = HAL_BUSY;
  	  	goto error;
  	}
	
  	/* Set the transaction information */
  	hspi->State       = HAL_SPI_STATE_BUSY_TX;
  	hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
  	hspi->pTxBuffPtr  = (uint8_t *)pData;
  	hspi->TxXferSize  = Size;
  	hspi->TxXferCount = Size;
	
  	/* Init field not used in handle to zero */
  	hspi->pRxBuffPtr  = (uint8_t *)NULL;
  	hspi->RxXferSize  = 0U;
  	hspi->RxXferCount = 0U;
  	hspi->RxISR       = NULL;

  	/* Enable TFTHIEN and TFURIEN interrupt */
  	__HAL_SPI_ENABLE_IT(hspi, (SPI_IT_TFTHIEN | SPI_IT_TFURIEN));
	
  	/* Check if the SPI is already enabled */
  	if((hspi->Instance->SSPCR2 & 0x01) != 0x01)
  	{
  	  	/* Enable SPI peripheral */
  	  	__HAL_SPI_ENABLE(hspi);
  	}

error :
  	__HAL_UNLOCK(hspi);
  	return errorcode;
}

/**
  * @brief  Receive an amount of data in non-blocking mode with Interrupt.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @param  pData: pointer to data buffer
  * @param  Size: amount of data to be sent
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_Receive_IT(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size)
{
  	HAL_StatusTypeDef errorcode = HAL_OK;
	
  	/* Process Locked */
  	__HAL_LOCK(hspi);
	
  	if(hspi->State != HAL_SPI_STATE_READY)
  	{
  	  	errorcode = HAL_BUSY;
  	  	goto error;
  	}
	
  	if((pData == NULL) || (Size == 0U))
  	{
  	  	errorcode = HAL_ERROR;
  	  	goto error;
  	}
	
  	/* Set the transaction information */
  	hspi->State       = HAL_SPI_STATE_BUSY_RX;
  	hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
  	hspi->pRxBuffPtr  = (uint8_t *)pData;
  	hspi->RxXferSize  = Size;
  	hspi->RxXferCount = Size;
	
  	/* Init field not used in handle to zero */
  	hspi->pTxBuffPtr  = (uint8_t *)NULL;
  	hspi->TxXferSize  = 0U;
  	hspi->TxXferCount = 0U;
  	hspi->TxISR       = NULL;
	
  	/* Set the function for IT treatment */
  	if(hspi->Init.DataSize > SPI_DATASIZE_8BIT )
  	{
  	  	hspi->RxISR = SPI_RxISR_16BIT;
  	}
  	else
  	{
  	  	hspi->RxISR = SPI_RxISR_8BIT;
  	}
	
  	/* Enable RFTHIEN and RFORIEN interrupt */
  	__HAL_SPI_ENABLE_IT(hspi, (SPI_IT_RFTHIEN | SPI_IT_RFORIEN));
	
  	/* Note : The SPI must be enabled after unlocking current process
  	          to avoid the risk of SPI interrupt handle execution before current
  	          process unlock */
	
  	/* Check if the SPI is already enabled */
  	if((hspi->Instance->SSPCR2 & 0x01) != 0x01)
  	{
  	  	/* Enable SPI peripheral */
  	  	__HAL_SPI_ENABLE(hspi);
  	}

error :
  	/* Process Unlocked */
  	__HAL_UNLOCK(hspi);
  	return errorcode;
}

/**
  * @brief  Transmit and Receive an amount of data in non-blocking mode with Interrupt.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @param  pTxData: pointer to transmission data buffer
  * @param  pRxData: pointer to reception data buffer
  * @param  Size: amount of data to be sent and received
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_TransmitReceive_IT(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size)
{
  	uint32_t tmp = 0U, tmp1 = 0U;
  	HAL_StatusTypeDef errorcode = HAL_OK;
	
  	/* Process locked */
  	__HAL_LOCK(hspi);
	
  	tmp  = hspi->State;
  	tmp1 = hspi->Init.Mode;
  	
  	if(!((tmp == HAL_SPI_STATE_READY) || \
  	  ((tmp1 == SPI_MODE_MASTER) && (tmp == HAL_SPI_STATE_BUSY_RX))))
  	{
  	  	errorcode = HAL_BUSY;
  	  	goto error;
  	}
	
  	if((pTxData == NULL ) || (pRxData == NULL ) || (Size == 0U))
  	{
  	  	errorcode = HAL_ERROR;
  	  	goto error;
  	}
	
  	/* Don't overwrite in case of HAL_SPI_STATE_BUSY_RX */
  	if(hspi->State == HAL_SPI_STATE_READY)
  	{
  	  	hspi->State = HAL_SPI_STATE_BUSY_TX_RX;
  	}
	
  	/* Set the transaction information */
  	hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
  	hspi->pTxBuffPtr  = (uint8_t *)pTxData;
  	hspi->TxXferSize  = Size;
  	hspi->TxXferCount = Size;
  	hspi->pRxBuffPtr  = (uint8_t *)pRxData;
  	hspi->RxXferSize  = Size;
  	hspi->RxXferCount = Size;
	
  	/* Enable TFTHIEN, RFTHIEN and TFURIEN, RFORIEN interrupt */
  	__HAL_SPI_ENABLE_IT(hspi, (SPI_IT_RFTHIEN | SPI_IT_RFORIEN | SPI_IT_TFTHIEN | SPI_IT_TFURIEN));
	
  	/* Check if the SPI is already enabled */
  	if((hspi->Instance->SSPCR2 & 0x01) != 0x01)
  	{
  	  	/* Enable SPI peripheral */
  	  	__HAL_SPI_ENABLE(hspi);
  	}

error :
  	/* Process Unlocked */
  	__HAL_UNLOCK(hspi);
  	return errorcode;
}

int GetSPIDmaSrc(SSP_TypeDef* SPIx,uint32_t *Tx_chanel,uint32_t*Rx_chanel)
{
    switch ((int)SPIx)
    {
    case SSP0_BASE:
        if (Tx_chanel)
        {
            *Tx_chanel = DMA_SSP0_TX;
        }
        if (Rx_chanel)
        {
            *Rx_chanel = DMA_SSP0_RX;
        }
        break;
    case SSP1_BASE:
        if (Tx_chanel)
        {
            *Tx_chanel = DMA_SSP1_TX;
        }
        if (Rx_chanel)
        {
            *Rx_chanel = DMA_SSP1_RX;
        }
        break;
    default:
        return -1;
    }
    return 0;
}

/**
  * @brief  Transmit an amount of data in non-blocking mode with DMA.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @param  pData: pointer to data buffer
  * @param  Size: amount of data to be sent
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_Transmit_DMA(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size)
{
  	HAL_StatusTypeDef errorcode = HAL_OK;
	
	DMA_InitTypeDef DMA_InitStruct;
	
  	/* Process Locked */
  	__HAL_LOCK(hspi);
	
  	if(hspi->State != HAL_SPI_STATE_READY)
  	{
  	  	errorcode = HAL_BUSY;
  	  	goto error;
  	}
	
  	if((pData == NULL) || (Size == 0U))
  	{
  	  	errorcode = HAL_ERROR;
  	  	goto error;
  	}
	
  	/* Set the transaction information */
  	hspi->State       = HAL_SPI_STATE_BUSY_TX;
  	hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
  	hspi->pTxBuffPtr  = (uint8_t *)pData;
  	hspi->TxXferSize  = Size;
  	hspi->TxXferCount = Size;
	
  	/* Init field not used in handle to zero */
  	hspi->pRxBuffPtr  = (uint8_t *)NULL;
  	hspi->TxISR       = NULL;
  	hspi->RxISR       = NULL;
  	hspi->RxXferSize  = 0U;
  	hspi->RxXferCount = 0U;
	
	DMA_InitStruct.CSR =    CSR_SRC_SIZE_1|
                            CSR_SRC_WIDTH_8BIT|
                            CSR_DST_WIDTH_8BIT|
                            CSR_MODE_HANDSHAKE|
                            CSR_SRCAD_CTL_INC|
                            CSR_DSTAD_CTL_FIX;
    
    DMA_InitStruct.CFG = CFG_DST_HE_SET|CFG_SRC_HE_SET|
                         CFG_INT_ABT_MSK|CFG_INT_ERR_MSK;//|CFG_INT_TC_MSK;
    
    DMA_InitStruct.SrcAdd = (uint32_t)hspi->pTxBuffPtr;
    DMA_InitStruct.DstAddr = (uint32_t)&hspi->Instance->TxRxDR;
    DMA_InitStruct.LLP = 0;
    if(GetSPIDmaSrc(hspi->Instance,&DMA_InitStruct.ChlPeriph,NULL)!=0)
    {
      	__BKPT(1);
    }
    DMA_InitStruct.SIZE = Size;
    DMA_InitStruct.SYNC_Flg = SET;
    DMA_Init(DMA, SPI_DMA_TX_CHL,&DMA_InitStruct);
    DMA_ClearChannelTCITStatus(DMA, SPI_DMA_TX_CHL);
    Set_DMA_Channel(DMA, SPI_DMA_TX_CHL);
	
  	/* Check if the SPI is already enabled */
  	if((hspi->Instance->SSPCR2 & 0x01) != 0x01)
  	{
  	  	/* Enable SPI peripheral */
  	  	__HAL_SPI_ENABLE(hspi);
  	}
	
  	/* Enable the SPI Error Interrupt Bit */
  	SET_BIT(hspi->Instance->IntrCR, SPI_IT_TFURIEN);
	
  	/* Enable Tx DMA Request */
  	SET_BIT(hspi->Instance->IntrCR, SPI_DMA_TFDMAEN);

error :
  	/* Process Unlocked */
  	__HAL_UNLOCK(hspi);
  	return errorcode;
}

/**
  * @brief  Receive an amount of data in non-blocking mode with DMA.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @param  pData: pointer to data buffer
  * @note   When the CRC feature is enabled the pData Length must be Size + 1.
  * @param  Size: amount of data to be sent
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_Receive_DMA(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size)
{
  	HAL_StatusTypeDef errorcode = HAL_OK;
	
	DMA_InitTypeDef DMA_InitStruct;
	
  	/* Process Locked */
  	__HAL_LOCK(hspi);
	
  	if(hspi->State != HAL_SPI_STATE_READY)
  	{
  	  errorcode = HAL_BUSY;
  	  goto error;
  	}
	
  	if((pData == NULL) || (Size == 0U))
  	{
  	  errorcode = HAL_ERROR;
  	  goto error;
  	}
	
  	/* Set the transaction information */
  	hspi->State       = HAL_SPI_STATE_BUSY_RX;
  	hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
  	hspi->pRxBuffPtr  = (uint8_t *)pData;
  	hspi->RxXferSize  = Size;
  	hspi->RxXferCount = Size;
	
  	/*Init field not used in handle to zero */
  	hspi->RxISR       = NULL;
  	hspi->TxISR       = NULL;
  	hspi->TxXferSize  = 0U;
  	hspi->TxXferCount = 0U;
	
	DMA_InitStruct.CSR =    CSR_SRC_SIZE_1|
                            CSR_SRC_WIDTH_8BIT|
                            CSR_DST_WIDTH_8BIT|
                            CSR_MODE_HANDSHAKE|
                            CSR_SRCAD_CTL_INC|
                            CSR_DSTAD_CTL_FIX;
    
    DMA_InitStruct.CFG = CFG_DST_HE_SET|CFG_SRC_HE_SET|
                         CFG_INT_ABT_MSK|CFG_INT_ERR_MSK;//|CFG_INT_TC_MSK;
    
    DMA_InitStruct.SrcAdd = (uint32_t)&hspi->Instance->TxRxDR;
    DMA_InitStruct.DstAddr = (uint32_t)hspi->pTxBuffPtr;
    DMA_InitStruct.LLP = 0;
    if(GetSPIDmaSrc(hspi->Instance,NULL,&DMA_InitStruct.ChlPeriph)!=0)
    {
      	__BKPT(1);
    }
    DMA_InitStruct.SIZE = Size;
    DMA_InitStruct.SYNC_Flg = SET;
    DMA_Init(DMA, SPI_DMA_RX_CHL,&DMA_InitStruct);
    DMA_ClearChannelTCITStatus(DMA, SPI_DMA_RX_CHL);
    Set_DMA_Channel(DMA, SPI_DMA_RX_CHL);
	
  	/* Check if the SPI is already enabled */
  	if((hspi->Instance->SSPCR2 & 0x01) != 0x01)
  	{
  	  /* Enable SPI peripheral */
  	  __HAL_SPI_ENABLE(hspi);
  	}
	
  	/* Enable the SPI Error Interrupt Bit */
  	SET_BIT(hspi->Instance->IntrCR, SPI_IT_RFORIEN);
	
  	/* Enable Rx DMA Request */
  	SET_BIT(hspi->Instance->IntrCR, SPI_DMA_RFDMAEN);

error:
  	/* Process Unlocked */
  	__HAL_UNLOCK(hspi);
  	return errorcode;
}

/**
  * @brief  Set FS HIGH OR LOW.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval HAL status
*/
HAL_StatusTypeDef HAL_SPI_Set_FS(SPI_HandleTypeDef *hspi, uint8_t FS_Status)
{
    assert_param(IS_SPI_FS(FS_Status));
	if(FS_Status)
    	hspi->Instance->SSPCR2 |=  (1 << 9);
	else
	    hspi->Instance->SSPCR2 &=  (~(1 << 9));
    return HAL_OK;
}

/**
  * @brief  Transmit and Receive an amount of data in non-blocking mode with DMA.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @param  pTxData: pointer to transmission data buffer
  * @param  pRxData: pointer to reception data buffer
  * @note   When the CRC feature is enabled the pRxData Length must be Size + 1
  * @param  Size: amount of data to be sent
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size)
{
  	uint32_t tmp = 0U, tmp1 = 0U;
  	HAL_StatusTypeDef errorcode = HAL_OK;
	
	DMA_InitTypeDef DMA_InitStruct;
	
  	/* Check Direction parameter */
  	assert_param(IS_SPI_DIRECTION_2LINES(hspi->Init.Direction));
	
  	/* Process locked */
  	__HAL_LOCK(hspi);
	
  	tmp  = hspi->State;
  	tmp1 = hspi->Init.Mode;
  	if(!((tmp == HAL_SPI_STATE_READY) ||
  	    ((tmp1 == SPI_MODE_MASTER) && (tmp == HAL_SPI_STATE_BUSY_RX))))
  	{
  	  errorcode = HAL_BUSY;
  	  goto error;
  	}
	
  	if((pTxData == NULL ) || (pRxData == NULL ) || (Size == 0U))
  	{
  	  errorcode = HAL_ERROR;
  	  goto error;
  	}
	//clear tx rx fifo
	hspi->Instance->SSPCR2|=SPI_CLEAR_TX_FIFO|SPI_CLEAR_RX_FIFO;
  	/* Don't overwrite in case of HAL_SPI_STATE_BUSY_RX */
  	if(hspi->State == HAL_SPI_STATE_READY)
  	{
  	  hspi->State = HAL_SPI_STATE_BUSY_TX_RX;
  	}
	
  	/* Set the transaction information */
  	hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
  	hspi->pTxBuffPtr  = (uint8_t*)pTxData;
  	hspi->TxXferSize  = Size;
  	hspi->TxXferCount = Size;
  	hspi->pRxBuffPtr  = (uint8_t*)pRxData;
  	hspi->RxXferSize  = Size;
  	hspi->RxXferCount = Size;
	
  	DMA_InitStruct.CSR =    CSR_SRC_SIZE_1|
                            CSR_SRC_WIDTH_8BIT|
                            CSR_DST_WIDTH_8BIT|
                            CSR_MODE_HANDSHAKE|
                            CSR_SRCAD_CTL_FIX|
                            CSR_DSTAD_CTL_INC;
    
    DMA_InitStruct.CFG = CFG_SRC_HE_SET|
                         CFG_INT_ABT_MSK|CFG_INT_ERR_MSK;//|CFG_INT_TC_MSK;
    
    DMA_InitStruct.SrcAdd = (uint32_t)&hspi->Instance->TxRxDR;
    DMA_InitStruct.DstAddr = (uint32_t)hspi->pRxBuffPtr;
    DMA_InitStruct.LLP = 0;
    if(GetSPIDmaSrc(hspi->Instance,NULL,&DMA_InitStruct.ChlPeriph)!=0)
    {
      	__BKPT(1);
    }
    DMA_InitStruct.SIZE = Size;
    DMA_InitStruct.SYNC_Flg = SET;
    DMA_Init(DMA, SPI_DMA_RX_CHL,&DMA_InitStruct);
    DMA_ClearChannelTCITStatus(DMA, SPI_DMA_RX_CHL);
    Set_DMA_Channel(DMA, SPI_DMA_RX_CHL);
	
  	/* Enable Rx DMA Request */
  	SET_BIT(hspi->Instance->IntrCR, SPI_DMA_RFDMAEN);
	
  	DMA_InitStruct.CSR =    CSR_SRC_SIZE_1|
                            CSR_SRC_WIDTH_8BIT|
                            CSR_DST_WIDTH_8BIT|
                            CSR_MODE_HANDSHAKE|
                            CSR_SRCAD_CTL_INC|
                            CSR_DSTAD_CTL_FIX;
    
    DMA_InitStruct.CFG = CFG_DST_HE_SET|
                         CFG_INT_ABT_MSK|CFG_INT_ERR_MSK;//|CFG_INT_TC_MSK;
    
    DMA_InitStruct.SrcAdd = (uint32_t)hspi->pTxBuffPtr;
    DMA_InitStruct.DstAddr = (uint32_t)&hspi->Instance->TxRxDR;
    DMA_InitStruct.LLP = 0;
    if(GetSPIDmaSrc(hspi->Instance,&DMA_InitStruct.ChlPeriph,NULL)!=0)
    {
      	__BKPT(1);
    }
    DMA_InitStruct.SIZE = Size;
    DMA_InitStruct.SYNC_Flg = SET;
    DMA_Init(DMA, SPI_DMA_TX_CHL,&DMA_InitStruct);
    DMA_ClearChannelTCITStatus(DMA, SPI_DMA_TX_CHL);
    Set_DMA_Channel(DMA, SPI_DMA_TX_CHL);
	
  	/* Check if the SPI is already enabled */
  	if((hspi->Instance->SSPCR2 & 0x01) != 0x01)
  	{
  	  	/* Enable SPI peripheral */
  	  	__HAL_SPI_ENABLE(hspi);
  	}
  	/* Enable the SPI Error Interrupt Bit */
  	//SET_BIT(hspi->Instance->IntrCR, SPI_IT_TFURIEN);
	
	//SET_BIT(hspi->Instance->IntrCR, SPI_IT_RFORIEN);
	
  	/* Enable Tx DMA Request */
  	SET_BIT(hspi->Instance->IntrCR, SPI_DMA_TFDMAEN);

error :
  	/* Process Unlocked */
  	__HAL_UNLOCK(hspi);
  	return errorcode;
}

/**
  * @brief  Handle SPI interrupt request.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for the specified SPI module.
  * @retval None
  */
void HAL_SPI_IRQHandler(SPI_HandleTypeDef *hspi)
{
  	uint32_t itsource = hspi->Instance->IntrStatus;
	
  	uint32_t itflag   = hspi->Instance->SSPStatus;
	
  	/* SPI in mode Receiver ----------------------------------------------------*/
  	if(((itflag & SPI_FLAG_RFF) != RESET) && ((itsource & SPI_ISR_RFTHI) != RESET))
  	{
  	  	hspi->RxISR(hspi);
  	  	return;
  	}

  	/* SPI in mode Transmitter -------------------------------------------------*/
  	if(((itflag & SPI_FLAG_TFNF) != RESET) && ((itsource & SPI_ISR_TFTHI) != RESET))
  	{
  	  	hspi->TxISR(hspi);
  	  	return;
  	}
	
  	/* SPI in Error Treatment --------------------------------------------------*/
  	if((itflag & (SPI_ISR_TFURI | SPI_ISR_RFORI)) != RESET)
  	{
  	  	if(hspi->State != HAL_SPI_STATE_BUSY_TX)
  	  	{
  	  	  	SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_OVR);
  	  	  	__HAL_SPI_CLEAR_OVRFLAG(hspi);
  	  	}
  	  	else
  	  	{
  	  	  	__HAL_SPI_CLEAR_OVRFLAG(hspi);
  	  	  	return;
  	  	}
  	  	
		
  	  	if(hspi->ErrorCode != HAL_SPI_ERROR_NONE)
  	  	{
  	  	  	/* Disable all interrupts */
  	  	  	__HAL_SPI_DISABLE_IT(hspi, SPI_IT_TFTHIEN | SPI_IT_RFTHIEN | SPI_IT_TFURIEN);
			
  	  	  	hspi->State = HAL_SPI_STATE_READY;
  	  	  	/* Disable the SPI DMA requests if enabled */
  	  	  	if ((HAL_IS_BIT_SET(itsource, SPI_DMA_TFDMAEN))||(HAL_IS_BIT_SET(itsource, SPI_DMA_RFDMAEN)))
  	  	  	{
  	  	  	  	CLEAR_BIT(hspi->Instance->IntrCR, (SPI_DMA_TFDMAEN | SPI_DMA_RFDMAEN));
			
  	  	  	  	DMA_AbtChannel(DMA,SPI_DMA_TX_CHL);
				DMA_AbtChannel(DMA,SPI_DMA_RX_CHL);
  	  	  	}
  	  	  	else
  	  	  	{
  	  	  	  	/* Call user error callback */
  	  	  	  	HAL_SPI_ErrorCallback(hspi);
  	  	  	}
  	  	}
  	  	return;
  	}
}

/**
  * @brief Tx Transfer completed callback.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
__weak void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hspi);
  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_SPI_TxCpltCallback should be implemented in the user file
  */
}

/**
  * @brief Rx Transfer completed callback.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
__weak void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hspi);
  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_SPI_RxCpltCallback should be implemented in the user file
  */
}

/**
  * @brief Tx and Rx Transfer completed callback.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
__weak void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hspi);
  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_SPI_TxRxCpltCallback should be implemented in the user file
  */
}

/**
  * @brief Tx Half Transfer completed callback.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
__weak void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hspi);
  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_SPI_TxHalfCpltCallback should be implemented in the user file
  */
}

/**
  * @brief Rx Half Transfer completed callback.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
__weak void HAL_SPI_RxHalfCpltCallback(SPI_HandleTypeDef *hspi)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hspi);
  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_SPI_RxHalfCpltCallback() should be implemented in the user file
  */
}

/**
  * @brief Tx and Rx Half Transfer callback.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
__weak void HAL_SPI_TxRxHalfCpltCallback(SPI_HandleTypeDef *hspi)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hspi);
  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_SPI_TxRxHalfCpltCallback() should be implemented in the user file
  */
}

/**
  * @brief SPI error callback.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
 __weak void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hspi);
  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_SPI_ErrorCallback should be implemented in the user file
   */
  /* NOTE : The ErrorCode parameter in the hspi handle is updated by the SPI processes
            and user can use HAL_SPI_GetError() API to check the latest error occurred
  */
}

/**
  * @brief  SPI Abort Complete callback.
  * @param  hspi SPI handle.
  * @retval None
  */
__weak void HAL_SPI_AbortCpltCallback(SPI_HandleTypeDef *hspi)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hspi);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_SPI_AbortCpltCallback can be implemented in the user file.
   */
}

/**
  * @}
  */

/** @defgroup SPI_Exported_Functions_Group3 Peripheral State and Errors functions
 ===============================================================================
                      ##### Peripheral State and Errors functions #####
 ===============================================================================
    [..]
    This subsection provides a set of functions allowing to control the SPI.
     (+) HAL_SPI_GetState() API can be helpful to check in run-time the state of the SPI peripheral
     (+) HAL_SPI_GetError() check in run-time Errors occurring during communication

  * @brief  Return the SPI handle state.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval SPI state
  */
HAL_SPI_StateTypeDef HAL_SPI_GetState(SPI_HandleTypeDef *hspi)
{
  	/* Return SPI handle state */
  	return hspi->State;
}

/**
  * @brief  Return the SPI error code.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval SPI error code in bitmap format
  */
uint32_t HAL_SPI_GetError(SPI_HandleTypeDef *hspi)
{
  	/* Return SPI ErrorCode */
  	return hspi->ErrorCode;
}

/**
  * @brief Handle SPI Communication Timeout.
  * @param hspi: pointer to a SPI_HandleTypeDef structure that contains
  *              the configuration information for SPI module.
  * @param TxRxFIFO: 1: TxFIFO to check  0: RxFIFO to check
  * @param Timeout: Timeout duration
  * @param Tickstart: tick start value
  * @retval HAL status
  */
static HAL_StatusTypeDef SPI_WaitTxRxFIFOEmptyUntilTimeout(SPI_HandleTypeDef *hspi, uint8_t TxRxFIFO, uint32_t Timeout, uint32_t Tickstart)
{
	if(TxRxFIFO == 1)
	{
		while(hspi->Instance->SSPStatus & 0x3f000)
		{
			if(Timeout != HAL_MAX_DELAY)
			{
				if((Timeout == 0U) || ((HAL_GetTick()-Tickstart) >= Timeout))
				{
					/* Disable the SPI and reset the CRC: the CRC value should be cleared
					on both master and slave sides in order to resynchronize the master
					and slave for their respective CRC calculation */
					
					/* Disable TXE, RXNE and ERR interrupts for the interrupt process */
					__HAL_SPI_DISABLE_IT(hspi, (SPI_IT_TFTHIEN | SPI_IT_RFTHIEN | SPI_IT_TFURIEN | SPI_IT_RFORIEN));
					
					hspi->State= HAL_SPI_STATE_READY;
					
					/* Process Unlocked */
					__HAL_UNLOCK(hspi);
					
					return HAL_TIMEOUT;
				}
			}
		}
	}
	else
	{
		while(hspi->Instance->SSPStatus & 0x3f0)
		{
			if(Timeout != HAL_MAX_DELAY)
			{
				if((Timeout == 0U) || ((HAL_GetTick()-Tickstart) >= Timeout))
				{
					/* Disable the SPI and reset the CRC: the CRC value should be cleared
					on both master and slave sides in order to resynchronize the master
					and slave for their respective CRC calculation */
					
					/* Disable TXE, RXNE and ERR interrupts for the interrupt process */
					__HAL_SPI_DISABLE_IT(hspi, (SPI_IT_TFTHIEN | SPI_IT_RFTHIEN | SPI_IT_TFURIEN | SPI_IT_RFORIEN));
					
					hspi->State= HAL_SPI_STATE_READY;
					
					/* Process Unlocked */
					__HAL_UNLOCK(hspi);
					
					return HAL_TIMEOUT;
				}
			}
		}
	}

  	return HAL_OK;
}
/**
  * @brief Handle SPI Communication Timeout.
  * @param hspi: pointer to a SPI_HandleTypeDef structure that contains
  *              the configuration information for SPI module.
  * @param Flag: SPI flag to check
  * @param State: flag state to check
  * @param Timeout: Timeout duration
  * @param Tickstart: tick start value
  * @retval HAL status
  */
static HAL_StatusTypeDef SPI_WaitFlagStateUntilTimeout(SPI_HandleTypeDef *hspi, uint32_t Flag, uint32_t State, uint32_t Timeout, uint32_t Tickstart)
{
  	while((((hspi->Instance->SSPStatus & Flag) == (Flag)) ? SET : RESET) != State)
  	{
  	  	if(Timeout != HAL_MAX_DELAY)
  	  	{
  	  	  	if((Timeout == 0U) || ((HAL_GetTick()-Tickstart) >= Timeout))
  	  	  	{
  	  	  	  	/* Disable the SPI and reset the CRC: the CRC value should be cleared
  	  	  	  	on both master and slave sides in order to resynchronize the master
  	  	  	  	and slave for their respective CRC calculation */
				
  	  	  	  	/* Disable TXE, RXNE and ERR interrupts for the interrupt process */
  	  	  	  	__HAL_SPI_DISABLE_IT(hspi, (SPI_IT_TFTHIEN | SPI_IT_RFTHIEN | SPI_IT_TFURIEN | SPI_IT_RFORIEN));
				
  	  	  	  	hspi->State= HAL_SPI_STATE_READY;
				
  	  	  	  	/* Process Unlocked */
  	  	  	  	__HAL_UNLOCK(hspi);
				
  	  	  	  	return HAL_TIMEOUT;
  	  	  	}
  	  	}
  	}

  	return HAL_OK;
}
/**
  * @brief Handle to check BSY flag before start a new transaction.
  * @param hspi: pointer to a SPI_HandleTypeDef structure that contains
  *              the configuration information for SPI module.
  * @param Timeout: Timeout duration
  * @param Tickstart: tick start value
  * @retval HAL status
  */
static HAL_StatusTypeDef SPI_CheckFlag_BSY(SPI_HandleTypeDef *hspi, uint32_t Timeout, uint32_t Tickstart)
{
  	/* Control the BSY flag */
  	if(SPI_WaitFlagStateUntilTimeout(hspi, SPI_FLAG_BSY, RESET, Timeout, Tickstart) != HAL_OK)
  	{
  	  	SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FLAG);
  	  	return HAL_TIMEOUT;
  	}
  	return HAL_OK;
}

/**
  * @brief  Manage the 16-bit receive in Interrupt context.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
static void SPI_RxISR_16BIT(struct __SPI_HandleTypeDef *hspi)
{
  	*((uint16_t *)hspi->pRxBuffPtr) = hspi->Instance->TxRxDR;
  	hspi->pRxBuffPtr += sizeof(uint16_t);
  	hspi->RxXferCount--;
	
  	if(hspi->RxXferCount == 0U)
  	{
  	  	SPI_CloseRx_ISR(hspi);
  	}
}

/**
  * @brief  Handle the end of the RX transaction.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
static void SPI_CloseRx_ISR(SPI_HandleTypeDef *hspi)
{
    /* Disable RFTHIEN and RFORIEN interrupt */
    __HAL_SPI_DISABLE_IT(hspi, (SPI_IT_RFTHIEN | SPI_IT_RFORIEN));
	
    hspi->State = HAL_SPI_STATE_READY;

    if(hspi->ErrorCode == HAL_SPI_ERROR_NONE)
    {
      	HAL_SPI_RxCpltCallback(hspi);
    }
    else
    {
      	HAL_SPI_ErrorCallback(hspi);
    }
}

/**
  * @brief  Manage the receive 8-bit in Interrupt context.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
static void SPI_RxISR_8BIT(struct __SPI_HandleTypeDef *hspi)
{
  	*hspi->pRxBuffPtr++ = (*(__IO uint8_t *)&hspi->Instance->TxRxDR);
  	
  	hspi->RxXferCount--;
	
  	if(hspi->RxXferCount == 0U)
  	{
  	  	SPI_CloseRx_ISR(hspi);
  	}
}

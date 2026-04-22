/********************************************************************************
  * @file    gsmcu_spi.h
  * @author  jun.zhang
  * @version V1.0.0
  * @date    2020-09-17
  * @brief   Header file of SPI HAL module.
  ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GSMCU_SPI_H__
#define __GSMCU_SPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
  * @brief  SPI Configuration Structure definition
  */
typedef struct
{
  	uint32_t FramePolarity;       /*!< Frame/Sync. polarity for the SPI mode
										If this bit is set to ¡®1¡¯, frame/sync. will be regarded as active high.
										If this bit is set to ¡®0¡¯, frame/sync. will be regarded as active low. 
  										This parameter can be a value of @ref SPI_Frame_Polarity */
  	
  	uint32_t FirstBit;           /*!< If this bit is set to ¡®0¡¯, the most significant bit (MSB) of a data word
										will be transmitted or received first.
									    If this bit is set to ¡®1¡¯, the least significant bit (LSB) of a data word
										will be transmitted or received first. 
										This parameter can be a value of @ref SPI_MSB_LSB_transmission */
  	
  	uint32_t Mode;               /*!< Operation mode
										00, 01: Slave mode
										10, 11: Master mode.
  	                                  This parameter can be a value of @ref SPI_Mode */
  	
  	uint32_t CLKPolarity;        /*!< SCLK polarity
										If this bit is set to ¡®0¡¯, SCLK will remain low when SSP is idle.
										If this bit is set to ¡®1¡¯, SCLK will remain high when SSP is idle.
  										This parameter can be a value of @ref SPI_Clock_Polarity */
										  
  	uint32_t CLKPhase;           /*!< This bit defines the relationship between SCLK and frame/sync.
										If this bit is set to ¡®0¡¯, SCLK will start toggling after one SCLK cycle
										time when frame/sync. is activated.
										If this bit is set to ¡®1¡¯, SCLK will start running after half an SCLK
										cycle time when frame/sync. is activated.
  	                                  This parameter can be a value of @ref SPI_Clock_Phase */
	
  	uint32_t DataSize;           /*!< Serial data length.
										These bits define the bit length of a transmitted/received data word. 
										@note The actual data length equals to these bits plus one.
										@note The minimum value of SDL should not be less than four.
  	                                  This parameter can be a value of @ref SPI_Data_Size */
	
	
  	uint32_t SCLKDIV;  			/*!< These bits define the serial clock divider.
                                     Fsck = Fsspclk / (1 + SCLKDIV) / 2
                                     @note The communication clock is derived from the master clock.
                                          The slave clock does not need to be set. 
									      They cannot be set as zeros */
  
  uint32_t PaddingCycle;  		/*!< These bits are relevant only when the SPI frame format is specified.
									In the master mode, SPI will wait (PCL + 1) SCLK cycles between
									each successive transfer. */
}SPI_InitTypeDef;

/**
  * @brief  HAL SPI State structure definition
  */
typedef enum
{
  HAL_SPI_STATE_RESET      = 0x00U,    /*!< Peripheral not Initialized                         */
  HAL_SPI_STATE_READY      = 0x01U,    /*!< Peripheral Initialized and ready for use           */
  HAL_SPI_STATE_BUSY       = 0x02U,    /*!< an internal process is ongoing                     */
  HAL_SPI_STATE_BUSY_TX    = 0x03U,    /*!< Data Transmission process is ongoing               */
  HAL_SPI_STATE_BUSY_RX    = 0x04U,    /*!< Data Reception process is ongoing                  */
  HAL_SPI_STATE_BUSY_TX_RX = 0x05U,    /*!< Data Transmission and Reception process is ongoing */
  HAL_SPI_STATE_ERROR      = 0x06U     /*!< SPI error state                                    */
}HAL_SPI_StateTypeDef;

/**
  * @brief  SPI handle Structure definition
  */
typedef struct __SPI_HandleTypeDef
{
  	SSP_TypeDef                *Instance;    /*!< SPI registers base address */
	
  	SPI_InitTypeDef            Init;         /*!< SPI communication parameters */
	
  	uint8_t                    *pTxBuffPtr;  /*!< Pointer to SPI Tx transfer Buffer */
	
  	uint16_t                   TxXferSize;   /*!< SPI Tx Transfer size */
	
  	__IO uint16_t              TxXferCount;  /*!< SPI Tx Transfer Counter */
	
  	uint8_t                    *pRxBuffPtr;  /*!< Pointer to SPI Rx transfer Buffer */
	
  	uint16_t                   RxXferSize;   /*!< SPI Rx Transfer size */
	
  	__IO uint16_t              RxXferCount;  /*!< SPI Rx Transfer Counter */
	
  	void                       (*RxISR)(struct __SPI_HandleTypeDef * hspi); /*!< function pointer on Rx ISR */
	
  	void                       (*TxISR)(struct __SPI_HandleTypeDef * hspi); /*!< function pointer on Tx ISR */
	
  	HAL_LockTypeDef            Lock;         /*!< Locking object                 */
	
  	__IO HAL_SPI_StateTypeDef  State;        /*!< SPI communication state */
	
  	__IO uint32_t              ErrorCode;    /*!< SPI Error code */

}SPI_HandleTypeDef;


/** @defgroup SPI_Error_Code SPI Error Code
  * @{
  */
#define HAL_SPI_ERROR_NONE              0x00000000U   /*!< No error             */
#define HAL_SPI_ERROR_MODF              0x00000001U   /*!< MODF error           */
#define HAL_SPI_ERROR_OVR               0x00000004U   /*!< OVR error            */
#define HAL_SPI_ERROR_FRE               0x00000008U   /*!< FRE error            */
#define HAL_SPI_ERROR_DMA               0x00000010U   /*!< DMA transfer error   */
#define HAL_SPI_ERROR_FLAG              0x00000020U   /*!< Flag: RXNE,TXE, BSY  */


/** @defgroup SPI_Frame_Polarity Frame/Sync. polarity
  * @{
  */
#define SPI_FRAME_ACTIVE_LOW            ((uint32_t)0<<15)
#define SPI_FRAME_ACTIVE_HIGH           ((uint32_t)1<<15)


/** @defgroup SPI_MSB_LSB_transmission SPI MSB LSB Transmission
  * @{
  */
#define SPI_FIRSTBIT_MSB                ((uint32_t)0<<6)
#define SPI_FIRSTBIT_LSB                ((uint32_t)1<<6)

	
/** @defgroup SPI_Mode SPI Mode
  * @{
  */
#define SPI_MODE_SLAVE                  ((uint32_t)1<<2)
#define SPI_MODE_MASTER                 ((uint32_t)3<<2)


/** @defgroup SPI_Clock_Polarity SPI Clock Polarity
  * @{
  */
#define SPI_POLARITY_LOW                ((uint32_t)0<<1)
#define SPI_POLARITY_HIGH               ((uint32_t)1<<1)


/** @defgroup SPI_Clock_Phase SPI Clock Phase
  * @{
  */
#define SPI_PHASE_1EDGE                 ((uint32_t)0<<0)
#define SPI_PHASE_2EDGE                 ((uint32_t)1<<0)


/** @defgroup SPI_Data_Size SPI Data Size
  * @{
  */
#define SPI_DATASIZE_8BIT               ((uint32_t)7<<16)
#define SPI_DATASIZE_16BIT              ((uint32_t)15<<16)



/** @defgroup SPI_Interrupt_definition SPI Interrupt Definition
  * @{
  */
#define SPI_IT_TFTHIEN                  (1<<3)
#define SPI_IT_RFTHIEN                  (1<<2)
#define SPI_IT_TFURIEN                  (1<<1)
#define SPI_IT_RFORIEN                  (1<<0)

#define SPI_DMA_TFDMAEN                 (1<<5)
#define SPI_DMA_RFDMAEN                 (1<<4)
	
#define SPI_ISR_TFTHI                   (1<<3)
#define SPI_ISR_RFTHI                   (1<<2)
#define SPI_ISR_TFURI                   (1<<1)
#define SPI_ISR_RFORI                   (1<<0)

/** @defgroup SPI_Flags_definition SPI Flags Definition
  * @{
  */
#define SPI_FLAG_RFF                    (1<<0)   	  /* SPI status flag: Receive FIFO is full */
#define SPI_FLAG_TFNF                   (1<<1)    	  /* SPI status flag: transmit FIFO is available */
#define SPI_FLAG_BSY                    (1<<2)        /* SPI status flag: Busy flag */

#define SPI_TXEN                        (1<<8)
#define SPI_RXEN                        (1<<7)
#define SPI_TXFCLR                      (1<<3)
#define SPI_RXFCLR                      (1<<2)
#define SPI_TXDOE                       (1<<1)


#define SPI_TFTHOD_Pos                  (12)
#define SPI_TFTHOD_MASK                 (0x1f<<SPI_TFTHOD_Pos)           
#define SPI_RFTHOD_Pos                  (7)     
#define SPI_RFTHOD_MASK                 (0x1f<<SPI_RFTHOD_Pos) 

#define SPI_CLEAR_TX_FIFO               (1<<3)
#define SPI_CLEAR_RX_FIFO               (1<<2)
/* Exported macro ------------------------------------------------------------*/

/** @brief  Reset SPI handle state.
  * @param  __HANDLE__: specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2, or 3 to select the SPI peripheral.
  * @retval None
  */
#define __HAL_SPI_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = HAL_SPI_STATE_RESET)

/** @brief  Enable the specified SPI interrupts.
  * @param  __HANDLE__: specifies the SPI handle.
  *         This parameter can be SPI where x: 1, 2 to select the SPI peripheral.
  * @param  __INTERRUPT__: specifies the interrupt source to enable.
  *         This parameter can be one of the following values:
  *            @arg SPI_IT_TFTHIEN: Transmit FIFO Threshold Interrupt Enable
  *            @arg SPI_IT_RFTHIEN: Receive FIFO Threshold Interrupt Enable
  *            @arg SPI_IT_TFURIEN: Transmit FIFO Underrun Interrupt Enable
			   @arg SPI_IT_RFORIEN: Receive FIFO Overrun Interrupt Enable
  * @retval None
  */
#define __HAL_SPI_ENABLE_IT(__HANDLE__, __INTERRUPT__)   ((__HANDLE__)->Instance->IntrCR |= (__INTERRUPT__))

/** @brief  Disable the specified SPI interrupts.
  * @param  __HANDLE__: specifies the SPI handle.
  *         This parameter can be SPI where x: 1, 2 to select the SPI peripheral.
  * @param  __INTERRUPT__: specifies the interrupt source to disable.
  *         This parameter can be one of the following values:
  *            @arg SPI_IT_TFTHIEN: Transmit FIFO Threshold Interrupt Enable
  *            @arg SPI_IT_RFTHIEN: Receive FIFO Threshold Interrupt Enable
  *            @arg SPI_IT_TFURIEN: Transmit FIFO Underrun Interrupt Enable
			   @arg SPI_IT_RFORIEN: Receive FIFO Overrun Interrupt Enable
  * @retval None
  */
#define __HAL_SPI_DISABLE_IT(__HANDLE__, __INTERRUPT__)  ((__HANDLE__)->Instance->IntrCR &= (~(__INTERRUPT__)))


/** @brief  Check whether the specified SPI flag is set or not.
  * @param  __HANDLE__: specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2 to select the SPI peripheral.
  * @param  __FLAG__: specifies the flag to check.
  *         This parameter can be one of the following values:
  *            @arg SPI_FLAG_RFF: Receive buffer not empty flag
  *            @arg SPI_FLAG_TFNF: Transmit buffer empty flag
  *            @arg SPI_FLAG_MODF: Mode fault flag
  *            @arg SPI_FLAG_OVR: Overrun flag
  *            @arg SPI_FLAG_BSY: Busy flag
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_SPI_GET_FLAG(__HANDLE__, __FLAG__) ((((__HANDLE__)->Instance->SSPStatus) & (__FLAG__)) == (__FLAG__))

/** @brief  Clear the SPI CRCERR pending flag.
  * @param  __HANDLE__: specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2, or 3 to select the SPI peripheral.
  * @retval None
  */
#define __HAL_SPI_CLEAR_CRCERRFLAG(__HANDLE__) ((__HANDLE__)->Instance->SSPStatus = (uint16_t)(~SPI_FLAG_CRCERR))

/** @brief  Clear the SPI MODF pending flag.
  * @param  __HANDLE__: specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2 to select the SPI peripheral.
  * @retval None
  */
#define __HAL_SPI_CLEAR_MODFFLAG(__HANDLE__)       \
do{                                                \
    __IO uint32_t tmpreg_modf = 0x00U;             \
    tmpreg_modf = (__HANDLE__)->Instance->SSPStatus;      \
    (__HANDLE__)->Instance->SSPCR2 &= (~0x01); \
    UNUSED(tmpreg_modf);                           \
  } while(0U)

/** @brief  Clear the SPI OVR pending flag.
  * @param  __HANDLE__: specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2to select the SPI peripheral.
  * @retval None
  */
#define __HAL_SPI_CLEAR_OVRFLAG(__HANDLE__)        \
do{                                                \
    __IO uint32_t tmpreg_ovr = 0x00U;              \
    tmpreg_ovr = (__HANDLE__)->Instance->TxRxDR;       \
    tmpreg_ovr = (__HANDLE__)->Instance->SSPStatus;       \
    UNUSED(tmpreg_ovr);                            \
  } while(0U)


/** @brief  Enable the SPI peripheral.
  * @param  __HANDLE__: specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2 to select the SPI peripheral.
  * @retval None
  */
#define __HAL_SPI_ENABLE(__HANDLE__) ((__HANDLE__)->Instance->SSPCR2 |=  0x01)

/** @brief  Disable the SPI peripheral.
  * @param  __HANDLE__: specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2 to select the SPI peripheral.
  * @retval None
  */
#define __HAL_SPI_DISABLE(__HANDLE__) ((__HANDLE__)->Instance->SSPCR2 &= (~0x01))
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup SPI_Exported_Functions
  * @{
  */

/** @addtogroup SPI_Exported_Functions_Group1
  * @{
  */
/* Initialization/de-initialization functions  **********************************/
HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef HAL_SPI_DeInit (SPI_HandleTypeDef *hspi);
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi);
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi);
/**
  * @}
  */

/** @addtogroup SPI_Exported_Functions_Group2
  * @{
  */
/* I/O operation functions  *****************************************************/
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_Transmit_IT(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_Receive_IT(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_TransmitReceive_IT(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_Transmit_DMA(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_Receive_DMA(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_Set_FS(SPI_HandleTypeDef *hspi, uint8_t FS_Status);
/* Transfer Abort functions */
HAL_StatusTypeDef HAL_SPI_Abort(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef HAL_SPI_Abort_IT(SPI_HandleTypeDef *hspi);

void HAL_SPI_IRQHandler(SPI_HandleTypeDef *hspi);
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi);
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi);
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi);
void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi);
void HAL_SPI_RxHalfCpltCallback(SPI_HandleTypeDef *hspi);
void HAL_SPI_TxRxHalfCpltCallback(SPI_HandleTypeDef *hspi);
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi);
void HAL_SPI_AbortCpltCallback(SPI_HandleTypeDef *hspi);
/**
  * @}
  */

/** @addtogroup SPI_Exported_Functions_Group3
  * @{
  */
/* Peripheral State and Error functions ***************************************/
HAL_SPI_StateTypeDef HAL_SPI_GetState(SPI_HandleTypeDef *hspi);
uint32_t             HAL_SPI_GetError(SPI_HandleTypeDef *hspi);
/**
  * @}
  */

#define IS_SPI_ALL_INSTANCE(INSTANCE) (((INSTANCE) == SSP0) || \
                                       ((INSTANCE) == SSP1))									

#define IS_SPI_FRAME_ACTIVE(ACTIVE)   (((ACTIVE) == SPI_FRAME_ACTIVE_LOW) || \
                           			   ((ACTIVE) == SPI_FRAME_ACTIVE_HIGH))		

#define IS_SPI_FIRST_BIT(BIT) 		  (((BIT) == SPI_FIRSTBIT_MSB) || \
                              		   ((BIT) == SPI_FIRSTBIT_LSB))
									   
#define IS_SPI_MODE(MODE) 			  (((MODE) == SPI_MODE_SLAVE) || \
                          			   ((MODE) == SPI_MODE_MASTER))

#define IS_SPI_CPOL(CPOL) 			  (((CPOL) == SPI_POLARITY_LOW) || \
                          			   ((CPOL) == SPI_POLARITY_HIGH))

#define IS_SPI_CPHA(CPHA) 			  (((CPHA) == SPI_PHASE_1EDGE) || \
                          			   ((CPHA) == SPI_PHASE_2EDGE))

#define IS_SPI_DATASIZE(DATASIZE)     (((DATASIZE) == SPI_DATASIZE_8BIT) || \
                                       ((DATASIZE) == SPI_DATASIZE_16BIT) 
										 
#define IS_SPI_FS(FS) 				  (((FS) == SPI_FS_MODE_HIGH)       || \
                         		      ((FS) == SPI_FS_MODE_LOW))

#ifdef __cplusplus
}
#endif

#endif /* __GSMCU_SPI_H__ */


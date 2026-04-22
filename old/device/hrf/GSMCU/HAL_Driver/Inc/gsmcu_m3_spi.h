/****************************************************************
 *
 * SPI head file
 * @file gsmcu_m3_spi.h
 * @author sitieqiang                     
 * @version 0.1
 * @date 2016/11/11 14:59:30
 * @note NONE
 *
*****************************************************************/

#ifndef __M3_SPI_H
#define __M3_SPI_H
#include "GSMCU_M3.h"
#include "Driver_Common.h"
#include "gsmcu_m3_port.h"

#include "Driver_SPI.h"
#include "RTE_Components.h"
#ifdef __cplusplus
extern "C" {
#endif


/* SPI Parameter*/
#define SPI_DIV_MAX 2048
#define SPI_DIV_MIN 2
#define SPI_FIFO_DEEP 8
#define SPI_CLEAR_TX_FIFO(reg) do{\
									reg->SPITXFCR_b.TXFCLR = 1;\
									reg->SPITXFCR_b.TXFCLR = 0;\
								}while(0)
#define SPI_CLEAR_RX_FIFO(reg) do{\
									reg->SPIRXFCR_b.RXFCLR = 1;\
									reg->SPIRXFCR_b.RXFCLR = 0;\
								}while(0)
/* SPI Driver state flags */
#define SPI_INIT       (uint8_t)(1U << 0)   // Driver initialized
#define SPI_POWER      (uint8_t)(1U << 1)   // Driver power on
#define SPI_SETUP      (uint8_t)(1U << 2)   // Driver configured
#define SPI_SS_SWCTRL  (uint8_t)(1U << 3)   // Software controlled SSEL
#define SPI_SS_START   (uint8_t)(1U << 4)   // Slave start
#define SPI_PACK_ERROR (uint8_t)(1U << 7)  //SPI ´«ÊäÖ¡´íÎó
/* Add SPI Control Codes: Mode Parameters:  Select SPI PIN */

#define ARM_SPI_HIGH_SPEED_MODE_Pos	21
#define ARM_SPI_HIGH_SPEED_MODE_Msk	(0x1UL<<ARM_SPI_HIGH_SPEED_MODE_Pos)
#define ARM_SPI_HIGH_SPEED_MODE_EN	(0x1UL<<ARM_SPI_HIGH_SPEED_MODE_Pos)


#define ARM_SPI_DELAY_Pos			22
#define ARM_SPI_DELAY_Msk			(0x3UL<<ARM_SPI_DELAY_Pos)
#define ARM_SPI_GT_TIME				(0x1UL<<ARM_SPI_DELAY_Pos)
#define ARM_BEFORE_SCK_TIME			(0x2UL<<ARM_SPI_DELAY_Pos)
#define ARM_AFTER_SCK_TIME			(0x3UL<<ARM_SPI_DELAY_Pos)


#define ARM_SPI_SWP_PIN_Pos 24								
#define ARM_SPI_SWP_PIN_Msk (0xFUL<<ARM_SPI_SWP_PIN_Pos)	//arg is num of pin_group
#define ARM_SPI_SWP_PIN_GROUP			(0x1UL<<ARM_SPI_SWP_PIN_Pos)
#define ARM_SPI_SWP_MOSI_MISO			(0x2UL<<ARM_SPI_SWP_PIN_Pos)


#define ARM_SPI_CONT_MODE_Pos		28
#define ARM_SPI_CONT_MODE_Msk		(0x3UL<<ARM_SPI_CONT_MODE_Pos)
#define ARM_SPI_CONT_MODE_EN		(0x1UL<<ARM_SPI_CONT_MODE_Pos)			//arg is count of data
#define ARM_SPI_CONT_MODE_DIS		(0x2UL<<ARM_SPI_CONT_MODE_Pos)

#define ARM_SPI_THRESHOLD_Pos		30
#define ARM_SPI_THRESHOLD_Msk		(0x3UL<<ARM_SPI_THRESHOLD_Pos)
#define ARM_SPI_TX_THRESHOLD		(0x1UL<<ARM_SPI_THRESHOLD_Pos)			//arg is Threshold Value 
#define ARM_SPI_RX_THRESHOLD		(0x2UL<<ARM_SPI_THRESHOLD_Pos)			//arg is Threshold Value
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wpadded"
#endif
/* SPI Transfer Information (Run-Time) */
typedef struct spi_transfer_info {
  uint32_t              num;                // Total number of transfers
  uint8_t              *rx_buf;             // Rx data pointer
  const uint8_t        *tx_buf;             // Tx data pointer
  uint32_t              rx_cnt;             // Rx data counter
  uint32_t              tx_cnt;             // Tx data counter
  uint32_t              def_val;            // Default transfer value
  uint32_t              dummy;              // Data dump variable
  uint8_t               data_bits;          // Data bits
  void *                rec_point;
} SPI_TRANSFER_INFO;

/* SPI status */
typedef struct spi_status {
  uint8_t busy;                             // Tx/Rx busy flag
  uint8_t data_lost;                        // Data lost: Rx overflow / Tx underflow
} SPI_STATUS;

/* SPI Control Information */
typedef struct spi_info {
  ARM_SPI_SignalEvent_t cb_event;           // Event callback
  SPI_TRANSFER_INFO     xfer;               // Transfer configuration
  ARM_SPI_STATUS        status;             // Status flags
  uint8_t               flags;              // Control and state flags
  uint8_t								pin_group;			//Group for PIN
} SPI_INFO;

typedef struct{
  uint8_t                 port;             // Port number
  uint8_t                 pin;              // Pin number
  PORT_FUNC               func;             // funtion for pin
} SPI_PIN;




#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic pop
#endif
extern ARM_DRIVER_SPI Driver_SPI0;
extern void SPI0_IRQHandler (void);
extern void SPI0_MODF_IRQHandler(void);


extern ARM_DRIVER_SPI Driver_SPI1;
extern void SPI1_IRQHandler (void);
extern void SPI1_MODF_IRQHandler(void);

#define ARM_SPI_SET_BITSWIDTH          (0x15UL << ARM_SPI_CONTROL_Pos)     ///< set bits width

#ifdef __cplusplus
}
#endif
#endif /* __M0_SPI_H */


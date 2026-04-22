/****************************************************************
 *
 * UART driver head
 * @file gsmcu_m3_usart.h
 * @author sitieqiang 
 * @version 0.1
 * @date 2016/10/21 15:38:34
 * @note None
 *
*****************************************************************/


#ifndef __M3_USART_H
#define __M3_USART_H


#include "Driver_USART.h"
#include "Driver_Common.h"
#include "RTE_Components.h"
#include "GSMCU_M3.h"
#include "gsmcu_m3_port.h"


#ifdef __cplusplus
extern "C"
{
#endif

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wpadded"
#endif
typedef struct _FRACT_DIV
{
	uint16_t val;
	uint8_t  add_mul;
} FRACT_DIVIDER;

// USART Transfer Information (Run-Time)
typedef struct _USART_TRANSFER_INFO
{
	uint32_t                rx_num;        // Total number of data to be received
	uint32_t                tx_num;        // Total number of data to be send
	uint8_t                *rx_buf;        // Pointer to in data buffer
	const uint8_t          *tx_buf;        // Pointer to out data buffer
	uint32_t                rx_cnt;        // Number of data received
	uint32_t                tx_cnt;        // Number of data sent
	uint8_t                 tx_def_val;    // Transmit default value (used in USART_SYNC_MASTER_MODE_RX)
	uint8_t                 rx_dump_val;   // Receive dump value (used in USART_SYNC_MASTER_MODE_TX)
	uint8_t                 send_active;   // Send active flag
	uint8_t                 sync_mode;     // Synchronous mode
} USART_TRANSFER_INFO;

typedef struct _USART_RX_STATUS
{
	uint8_t rx_busy;                       // Receiver busy flag
	uint8_t rx_overflow;                   // Receive data overflow detected (cleared on start of next receive operation)
	uint8_t rx_break;                      // Break detected on receive (cleared on start of next receive operation)
	uint8_t rx_framing_error;              // Framing error detected on receive (cleared on start of next receive operation)
	uint8_t rx_parity_error;               // Parity error detected on receive (cleared on start of next receive operation)
} USART_RX_STATUS;

// USART Information (Run-Time)
typedef struct _USART_INFO
{
	ARM_USART_SignalEvent_t cb_event;      // Event callback
	USART_RX_STATUS         rx_status;     // Receive status flags
	USART_TRANSFER_INFO     xfer;          // Transfer information
	uint32_t                mode;          // USART mode
	uint8_t                 flags;         // USART driver flags
	uint32_t                baudrate;      // Baudrate
	uint16_t				ri_value;            // ri value
} USART_INFO;








typedef struct _USART_RI_S
{
	uint16_t								RIvalue;                // if rx_busy == 0, RIvalue is use for store receive data
	uint8_t 								*ppvalue;
} USART_RI_S;




#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic pop
#endif



//define *******************************************************************


// USART flags
#define USART_FLAG_INITIALIZED       (1U << 0)
#define USART_FLAG_POWERED           (1U << 1)
#define USART_FLAG_CONFIGURED        (1U << 2)
#define USART_FLAG_TX_ENABLED        (1U << 3)
#define USART_FLAG_RX_ENABLED        (1U << 4)
#define USART_FLAG_SEND_ACTIVE       (1U << 5)

// USART synchronous xfer modes
#define USART_SYNC_MODE_TX           ( 1U )
#define USART_SYNC_MODE_RX           ( 2U )
#define USART_SYNC_MODE_TX_RX        (USART_SYNC_MODE_TX | \
                                      USART_SYNC_MODE_RX)

#define USART_TCIE_ENABLE            ( 1U ) 

/************************USART Control Codes***********************************/
#ifdef ARM_USART_MODE_IRDA
#undef ARM_USART_MODE_IRDA
#endif
#define ARM_USART_MODE_IRDA                 (0x05UL << ARM_USART_CONTROL_Pos)   ///< UART IrDA; arg = Baudrate;arg[31]:1:38K mode 0:irda1.0 mode ;arg[30]: Inverted
#define SET_USART_IRDA_MODE(mode,inv,rate)			(((mode)<<31)|((inv)<<30)|(rate))

#ifdef ARM_USART_SET_IRDA_PULSE
#undef ARM_USART_SET_IRDA_PULSE
#endif
#define ARM_USART_SET_IRDA_PULSE            (0x11UL << ARM_USART_CONTROL_Pos)   ///< Set IrDA Pulse in ns;   ; irda1.0 mode:  arg[7:0](TNUM): 0=3/16  1=2/16 2=1/16   arg[15:8](RNUM): 0=3/16  1=2/16 2=1/16 ;38K mode:arg is frequency for pulse
#define SET_IRDA1P0_PULSE(tnum,rnum)		(tnum)|((rnum)<<8)


#define ARM_USART_CONTROL_TXIE           		(0x1CUL << ARM_USART_CONTROL_Pos)   ///< set baudrate
#define ARM_USART_CONTROL_RXIE            	(0x1DUL << ARM_USART_CONTROL_Pos)   ///< set baudrate

#if RTE_USART0
extern ARM_DRIVER_USART Driver_USART0;
extern void USART0_IRQHandler(void);
#endif
#if RTE_USART1
extern ARM_DRIVER_USART Driver_USART1;
extern void USART1_IRQHandler(void);
#endif
#if RTE_USART2
extern ARM_DRIVER_USART Driver_USART2;
extern void USART2_IRQHandler(void);
#endif
#if RTE_USART3
extern ARM_DRIVER_USART Driver_USART3;
extern void USART3_IRQHandler(void);
#endif

extern uint8_t *p_value[4];







#ifdef __cplusplus
}
#endif
#endif /* __M0_USART_H */


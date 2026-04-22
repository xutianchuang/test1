/****************************************************************
 *
 * define device pin head
 * @file RTE_Device.h
 * @author sitieqiang                     
 * @version 0.1
 * @date 2016/11/25 16:08:59
 * @note NONE
 *
*****************************************************************/

#ifndef __RTE_DEVICE_H
#define __RTE_DEVICE_H

#define __MUX_FUNC			5

/******************************************************************* 
 ===		UART PORT		===
 *******************************************************************/
#define RTE_USART0_TX_PORT					1
#define RTE_USART0_TX_BIT					1
#define RTE_USART0_TX_FUNC					0
#define RTE_USART0_RX_PORT					1
#define RTE_USART0_RX_BIT					0
#define RTE_USART0_RX_FUNC					0x08

#define RTE_USART1_TX_PORT					1
#define RTE_USART1_TX_BIT					3
#define RTE_USART1_TX_FUNC					0
#define RTE_USART1_RX_PORT					1
#define RTE_USART1_RX_BIT					2
#define RTE_USART1_RX_FUNC					0

#define RTE_USART2_TX_PORT					1
#define RTE_USART2_TX_BIT					5
#define RTE_USART2_TX_FUNC					0
#define RTE_USART2_RX_PORT					1
#define RTE_USART2_RX_BIT					4
#define RTE_USART2_RX_FUNC					0

#define RTE_USART3_TX_PORT					5
#define RTE_USART3_TX_BIT					7
#define RTE_USART3_TX_FUNC					0
#define RTE_USART3_RX_PORT					5
#define RTE_USART3_RX_BIT					6
#define RTE_USART3_RX_FUNC					0



/************************************************************** 
====		SPI PORT		====
***************************************************************/
#define RTE_SPI0_SS0_PORT				0
#define RTE_SPI0_SS0_BIT				1
#define RTE_SPI0_SS0_FUNC				0

#define RTE_SPI0_MOSI0_PORT				0
#define RTE_SPI0_MOSI0_BIT				2
#define RTE_SPI0_MOSI0_FUNC				0

#define RTE_SPI0_MISO0_PORT				0
#define RTE_SPI0_MISO0_BIT				3
#define RTE_SPI0_MISO0_FUNC				0

#define RTE_SPI0_SCK0_PORT				0
#define RTE_SPI0_SCK0_BIT				1
#define RTE_SPI0_SCK0_FUNC				0



#define RTE_SPI0_SS1_PORT				0 
#define RTE_SPI0_SS1_BIT				1 
#define RTE_SPI0_SS1_FUNC				0 
										  
#define RTE_SPI0_MOSI1_PORT				0 
#define RTE_SPI0_MOSI1_BIT				2 
#define RTE_SPI0_MOSI1_FUNC				0 
										  
#define RTE_SPI0_MISO1_PORT				0 
#define RTE_SPI0_MISO1_BIT				3 
#define RTE_SPI0_MISO1_FUNC				0 
										  
#define RTE_SPI0_SCK1_PORT				0 
#define RTE_SPI0_SCK1_BIT				1 
#define RTE_SPI0_SCK1_FUNC				0 


#define RTE_SPI1_SS0_PORT				2
#define RTE_SPI1_SS0_BIT				3
#define RTE_SPI1_SS0_FUNC				0
			   
#define RTE_SPI1_MOSI0_PORT				2
#define RTE_SPI1_MOSI0_BIT				4
#define RTE_SPI1_MOSI0_FUNC				0
			   
#define RTE_SPI1_MISO0_PORT				2
#define RTE_SPI1_MISO0_BIT				5
#define RTE_SPI1_MISO0_FUNC				0
			   
#define RTE_SPI1_SCK0_PORT				2
#define RTE_SPI1_SCK0_BIT				2
#define RTE_SPI1_SCK0_FUNC				0
			   

#define RTE_SPI1_SS1_PORT				2 
#define RTE_SPI1_SS1_BIT				3 
#define RTE_SPI1_SS1_FUNC				0 
										 
#define RTE_SPI1_MOSI1_PORT				2 
#define RTE_SPI1_MOSI1_BIT				4 
#define RTE_SPI1_MOSI1_FUNC				0 
										 
#define RTE_SPI1_MISO1_PORT				2 
#define RTE_SPI1_MISO1_BIT				5 
#define RTE_SPI1_MISO1_FUNC				0 
										 
#define RTE_SPI1_SCK1_PORT				2 
#define RTE_SPI1_SCK1_BIT				2 
#define RTE_SPI1_SCK1_FUNC				0 




/***************************************************************************************** 
		========== PWM	=============
 *****************************************************************************************/
#define	PWM0_CH0_PORT					1
#define PWM0_CH0_BIT					6
#define PWM0_CH0_FUNC   				0
    									
#define	PWM0_CH1_PORT					1
#define PWM0_CH1_BIT					7
#define PWM0_CH1_FUNC   				0
										
#define	PWM0_CH2_PORT					2
#define PWM0_CH2_BIT					0
#define PWM0_CH2_FUNC   				0
										
#define	PWM0_CH3_PORT					2
#define PWM0_CH3_BIT					1
#define PWM0_CH3_FUNC   				0
										
#define	PWM1_CH0_PORT					3
#define PWM1_CH0_BIT					6
#define PWM1_CH0_FUNC   				0
    	    							
#define	PWM1_CH1_PORT					3
#define PWM1_CH1_BIT					7
#define PWM1_CH1_FUNC   				0
		    							
#define	PWM1_CH2_PORT					4
#define PWM1_CH2_BIT					6
#define PWM1_CH2_FUNC   				0
		    							
#define	PWM1_CH3_PORT					4
#define PWM1_CH3_BIT					7
#define PWM1_CH3_FUNC   				0


/************************************************************** 
====		QSPI PORT		====
***************************************************************/
#define HRF_QSPI_SCK_PORT					3
#define HRF_QSPI_SCK_BIT					0
#define HRF_QSPI_SCK_FUNC					0x00
	
#define HRF_QSPI_SS_PORT					3
#define HRF_QSPI_SS_BIT						1
#define HRF_QSPI_SS_FUNC					0x00
	
#define HRF_QSPI_WP_IO2_PORT				3
#define HRF_QSPI_WP_IO2_BIT					2
#define HRF_QSPI_WP_IO2_FUNC				0x08
	
#define HRF_QSPI_HOLD_IO3_PORT				3
#define HRF_QSPI_HOLD_IO3_BIT				3
#define HRF_QSPI_HOLD_IO3_FUNC				0x08
	
#define HRF_QSPI_MISO_IO1_PORT				3
#define HRF_QSPI_MISO_IO1_BIT				4
#define HRF_QSPI_MISO_IO1_FUNC				0x00
	
#define HRF_QSPI_MOSI_IO0_PORT				3
#define HRF_QSPI_MOSI_IO0_BIT				5
#define HRF_QSPI_MOSI_IO0_FUNC				0x00

/* General return codes */
#ifndef ARM_DRIVER_OK
#define ARM_DRIVER_OK                 0 ///< Operation succeeded 
#endif
#ifndef ARM_DRIVER_ERROR
#define ARM_DRIVER_ERROR             -1 ///< Unspecified error
#endif
#ifndef ARM_DRIVER_ERROR_BUSY										///
#define ARM_DRIVER_ERROR_BUSY        -2 ///< Driver is busy
#endif
#ifndef ARM_DRIVER_ERROR_TIMEOUT
#define ARM_DRIVER_ERROR_TIMEOUT     -3 ///< Timeout occurred
#endif
#ifndef ARM_DRIVER_ERROR_UNSUPPORTED
#define ARM_DRIVER_ERROR_UNSUPPORTED -4 ///< Operation not supported
#endif
#ifndef ARM_DRIVER_ERROR_PARAMETER
#define ARM_DRIVER_ERROR_PARAMETER   -5 ///< Parameter error
#endif
#ifndef ARM_DRIVER_ERROR_SPECIFIC
#define ARM_DRIVER_ERROR_SPECIFIC    -6 ///< Start of driver specific errors      
#endif     
     

     
#endif  /* __RTE_DEVICE_H */


/************************************************************************************************
GateSea HPLC STA Board Defines
************************************************************************************************/
#ifndef  BOARD_DEF
#define  BOARD_DEF


//#define ALL_USE_TESTMODE_PARAM
	#define FACTORY_CODE					"LC"
	#define CHIP_CODE						"LC"
	#define FACTORY_CODE_FLAG				0x20600027
	#define CPU_NAME						"ZB206"

	#define DEV_CCO  						1u

	//#define FLASH_SIZE_1M
    //资产信息
	//模块软件版本信息
    #define MODULE_SOFT_VER                 (01 << 8 | 21)
	#define MODULE_SOFT_VER_DATE_YEAR       (24)
	#define MODULE_SOFT_VER_DATE_MON        (3)
	#define MODULE_SOFT_VER_DATE_DAY        (29)

    //模块硬件版本信息
    #define MODULE_HARD_VER                 (0x0001)
	#define MODULE_HARD_VER_DATE_YEAR       (21)
	#define MODULE_HARD_VER_DATE_MON        (4)
	#define MODULE_HARD_VER_DATE_DAY        (25)
	//芯片硬件版本信息
	#define CHIP_HARD_VER                   (0x0001)
    #define CHIP_HARD_VER_DATE_YEAR         (21)
	#define CHIP_HARD_VER_DATE_MON          (4)
	#define CHIP_HARD_VER_DATE_DAY          (25)
	//芯片软件版本信息
	#define CHIP_SOFT_VER                   (0x0001)
    #define CHIP_SOFT_VER_DATE_YEAR         (21)
	#define CHIP_SOFT_VER_DATE_MON          (4)
	#define CHIP_SOFT_VER_DATE_DAY          (25)
	//应用程序版本号
	#define APP_VER                         (0x0001)
    
	#if defined(ZB206_CHIP)
	#include "ZB206.h"
	#else
	#error "Current board file not match the Project!!!"
	#endif


	#define HRF_900M                        1

	#define GREEN_TX_LED_PORT               0
	#define GREEN_TX_LED_PIN				7

	#define RED_RX_LED_PORT                 0
	#define RED_RX_LED_PIN					6

	#define A_LED_PORT                      2
	#define A_LED_PIN						2

	#define B_LED_PORT                      2
	#define B_LED_PIN						3

	#define C_LED_PORT                      2
	#define C_LED_PIN						5

	#define LD_PORT                         3
	#define LD_PIN							7

	#define SWITCH_A_TX_PORT                5
	#define SWITCH_A_TX_PIN					5

	#define SWITCH_A_RX_PORT                5
	#define SWITCH_A_RX_PIN					2

	#define SWITCH_B_TX_PORT                5
	#define SWITCH_B_TX_PIN					0

	#define SWITCH_B_RX_PORT                5
	#define SWITCH_B_RX_PIN					1

	#define SWITCH_C_TX_PORT                5
	#define SWITCH_C_TX_PIN					4

	#define SWITCH_C_RX_PORT                5
	#define SWITCH_C_RX_PIN					3

	

	#define METER_UART                      Driver_USART0
	//ZCPORTs must be PORT4
	#define ZC_PIN_PHASEA_POS				0
	#define ZC_PIN_PHASEB_POS				2
	#define ZC_PIN_PHASEC_POS				4
	#define ZC_PIN_PHASEA_NEG				1
	#define ZC_PIN_PHASEB_NEG				3
	#define ZC_PIN_PHASEC_NEG				5

	#define ZC_CH_PHASEA_POS				1
	#define ZC_CH_PHASEB_POS				3
	#define ZC_CH_PHASEC_POS				5
	#define ZC_CH_PHASEA_NEG				2
	#define ZC_CH_PHASEB_NEG				4
	#define ZC_CH_PHASEC_NEG				6


	#define HRF_DIR_PORT                    1
	#define HRF_DIR_PIN                     7
	#define HRF_DIR_MODE                    0  //非IO脚，模式选择 =0：0发送1接收； =1：1发送0接收

	#define EFEM_MODE                       1

	#define FEM_CPS_PORT                    6  // PA_EN &  TRSW
	#define FEM_CPS_PIN                     1
	#define FEM_CPS_MODE                    0  // 0: same as phy tx mode; 1: inversed with phy tx mode

	#define FEM_CTX_PORT                    6  // LAN_EN
	#define FEM_CTX_PIN                     2

	#define FEM_CSD_PORT                    6
	#define FEM_CSD_PIN                     3

	#define HRF_TX_LED_PORT                 1
	#define HRF_TX_LED_PIN                  4
	#define HRF_RX_LED_PORT                 1
	#define HRF_RX_LED_PIN                  5

	#define DEBUG_UART                      Driver_USART3

	#define RESET_PORT_CCO                  1
	#define RESET_PIN_CCO			        2

	#define DCDC_VOUT_VAL                   0  // 0:1.5V; 1:1.6V; 2:1.3V ; 3:1.4V

#endif

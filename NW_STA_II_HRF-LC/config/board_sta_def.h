/************************************************************************************************
GateSea HPLC STA Board Defines
************************************************************************************************/
#ifndef  BOARD_DEF
#define  BOARD_DEF

//客户：澜潮   芯片：ZB205 
	#define  GS_STA     1u

	#define FACTORY_CODE					"LC"
	#define CHIP_CODE						"LC"
	#define FACTORY_CODE_II_FLAG			0x4c434402
	#define CPU_NAME						"ZB205"

	#define DEV_CCO  						0u
	#define DEV_STA  						1u

	//模块软件版本信息
    #define MODULE_SOFT_VER                 (0x0021)
	#define MODULE_SOFT_VER_DATE_YEAR       (24)
	#define MODULE_SOFT_VER_DATE_MON        (3)
	#define MODULE_SOFT_VER_DATE_DAY        (28)

    //模块硬件版本信息
    #define MODULE_HARD_VER                 (0x1000)
	#define MODULE_HARD_VER_DATE_YEAR       (24)
	#define MODULE_HARD_VER_DATE_MON        (4)
	#define MODULE_HARD_VER_DATE_DAY        (9)

	//芯片软件版本信息
	#define CHIP_SOFT_VER                   (0x0021)
    #define CHIP_SOFT_VER_DATE_YEAR         (24)
	#define CHIP_SOFT_VER_DATE_MON          (1)
	#define CHIP_SOFT_VER_DATE_DAY          (1)
	
	//芯片硬件版本信息
	#define CHIP_HARD_VER                   (0x0021)
    #define CHIP_HARD_VER_DATE_YEAR         (24)
	#define CHIP_HARD_VER_DATE_MON          (1)
	#define CHIP_HARD_VER_DATE_DAY          (1)

	//应用程序版本号
	#define APP_VER                         (0x0011)


	#include <ZB205.h>

//	#define GREEN_RX_LED_PORT               2
//	#define GREEN_RX_LED_PIN				0
//
//	#define RED_TX_LED_PORT                 2
//	#define RED_TX_LED_PIN					1
//
//	#define GREEN_RX_LED_PORT_3PHASE        2
//	#define GREEN_RX_LED_PIN_3PHASE			0
//
//	#define RED_TX_LED_PORT_3PHASE          2
//	#define RED_TX_LED_PIN_3PHASE			1

//	#define STAOUT_PORT                     1
//	#define STAOUT_PIN						5
//
//	#define STAOUT_PORT_3PHASE              1
//	#define STAOUT_PIN_3PHASE				5
//
//	#define EVENT_PORT                      1
//	#define EVENT_PIN						4
//
//	#define EVENT_PORT_3PHASE               1
//	#define EVENT_PIN_3PHASE				4
//
//	#define SET_PORT                        5
//	#define SET_PIN							7
//
//	#define SET_PORT_3PHASE                 5
//	#define SET_PIN_3PHASE					7
//
//	#define RESET_PORT                      5
//	#define RESET_PIN                       6
//
//	#define RESET_PORT_3PHASE               5
//	#define RESET_PIN_3PHASE				6

//	#define MODE_PIN1_PORT                  4
//	#define MODE_PIN1                       0
//    #define MODE_PIN1_MODE                  1  //1：外部下拉； 0：外部上拉
      
	#define LD_PORT                         3
	#define LD_PIN							7

	#define LD_PORT_3PHASE                  3
	#define LD_PIN_3PHASE					7

	#define HRF_DIR_PORT                    6
	#define HRF_DIR_PIN                     1
	#define HRF_DIR_MODE                    1  //非IO脚，模式选择 =0：0发送1接收； =1：1发送0接收
	
//	#define PULLOUT_PORT                    1
//	#define PULLOUT_PIN						6
//
//	#define PULLOUT_PORT_3PHASE             1
//	#define PULLOUT_PIN_3PHASE				6
//	
//	#define TOPO_PORT                       1
//	#define TOPO_PIN						7
//
//	#define TOPO_PORT_3PHASE                1
//	#define TOPO_PIN_3PHASE 				7

	#define TOPO_PWM_TMR                    0  

//	#define MULTI_PORT                      2
//	#define MULTI_PIN                       5

//	#define MULTI_PORT_3PHASE               2
//	#define MULTI_PIN_3PHASE                5
	
	//#define METER_UART_OPEN_DRAIN
	//ZCPORTs must be PORT1
	
	#define ZC_PIN_PHASEA_POS					0
//	#define ZC_PIN_PHASEB_POS					4
//	#define ZC_PIN_PHASEC_POS					3

	#define ZC_CH_PHASEA_POS					1
//	#define ZC_CH_PHASEB_POS					5
//	#define ZC_CH_PHASEC_POS					4
//	#define ZC_CH_PHASEA_NEG                    3
//	#define ZC_CH_PHASEB_NEG                    2
//	#define ZC_CH_PHASEC_NEG                    1


//    #define HRF_TX_LED_PORT                         2
//    #define HRF_TX_LED_PIN                          2
//    #define HRF_RX_LED_PORT                         2
//    #define HRF_RX_LED_PIN                          3


	#define TOPO_PWM_TMR                    0  


	#define U485_EN_PORT                    1
	#define U485_EN_PIN                     7


	#define METER_UART						Driver_USART0
	#define DEBUG_UART                      Driver_USART3
	#define IR_UART						    Driver_USART2
	#define LCIP_UART						Driver_USART1

	#define RUN_LED_PORT					5
	#define RUN_LED_PIN						2

	#define PLC_LED_PORT					5
	#define PLC_LED_PIN						0

	#define RS485_LED_PORT					5
	#define RS485_LED_PIN					5

	//1下降沿 2上升沿
	#define ZERO_EDGE_AREA                  1
	
	//0下降沿 1上升沿
    #define ZERO_EDGE_SHANXI_ASK            0
	#define DCDC_VOUT_VAL                   0  // 0:1.5V; 1:1.6V; 2:1.3V ; 3:1.4V  
	#define RG_OSC_CAPTRIM_VAL				1
	#define DIS_FUNC
	
#endif
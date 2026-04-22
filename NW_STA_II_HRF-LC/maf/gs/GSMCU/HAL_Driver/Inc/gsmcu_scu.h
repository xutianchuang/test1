#ifndef __GSMCU_SCU_H__
#define __GSMCU_SCU_H__
#ifdef __cplusplus
extern "C"
{
#endif


#include <iogsmcu.h>
#include <gsmcuxx_hal_def.h>
/** @defgroup clk
  * @{
  */

#define AHBCLK_AESCRYSHA_HCLK_ENABLE            (0x01UL<<29)
#define AHBCLK_SRYSHA_HCLK_ENABLE               (0x01UL<<22)
#define AHBCLK_AES_HCLK_ENABLE                  (0x01UL<<21)
#define AHBCLK_BPLC_HCLKA_ENABLE                (0x01UL<<20)
#define AHBCLK_BPLC_HCLKB_ENABLE                (0x01UL<<19)
#define AHBCLK_MAC_HCLK_ENABLE                  (0x01UL<<16)
#define AHBCLK_SCU_HCLK_ENABLE                  (0x01UL<<15)


#define APBCLK_REG_MASKFLG                      (0x01UL<<29)
#define APBCLK0_REG_FLG                         (0x00UL<<29)
#define APBCLK1_REG_FLG                         (0x01UL<<29)

#define APBCLK_SSP1_SSPCLK_ENABLE               ((0x01UL<<26)|(APBCLK0_REG_FLG))
#define APBCLK_ADC_PCLK_ENABLE                  ((0x01UL<<24)|(APBCLK0_REG_FLG))
#define APBCLK_IWDT_PCLK_ENABLE                 ((0x01UL<<23)|(APBCLK0_REG_FLG))
#define APBCLK_USART2_PCLK_ENABLE               ((0x01UL<<22)|(APBCLK0_REG_FLG))
#define APBCLK_SSP1_PCLK_ENABLE                 ((0x01UL<<21)|(APBCLK0_REG_FLG))
#define APBCLK_TMR2_PCLK_ENABLE                 ((0x01UL<<20)|(APBCLK0_REG_FLG))
#define APBCLK_IIC1_PCLK_ENABLE                 ((0x01UL<<19)|(APBCLK0_REG_FLG))

#define APBCLK_GPIO1_PCLK_ENABLE                ((0x01UL<<18)|(APBCLK0_REG_FLG))
#define APBCLK_SCU_PCLK_ENABLE                  ((0x01UL<<16)|(APBCLK0_REG_FLG))

#define APBCLK_EFUSECTRL64B_PCLK_ENABLE         ((0x01UL<<14)|(APBCLK0_REG_FLG))
#define APBCLK_EFUSECTRL1024B_PCLK_ENABLE       ((0x01UL<<13)|(APBCLK0_REG_FLG))
#define APBCLK_USART3_PCLK_ENABLE               ((0x01UL<<12)|(APBCLK0_REG_FLG))

#define APBCLK_SSP0_SSPCLK_ENABLE               ((0x01UL<<10)|(APBCLK0_REG_FLG))
#define APBCLK_WWDT_PCLK_ENABLE                 ((0x01UL<<9)|(APBCLK0_REG_FLG))
#define APBCLK_TMR1_PCLK_ENABLE                 ((0x01UL<<8)|(APBCLK0_REG_FLG))
#define APBCLK_TMR0_PCLK_ENABLE                 ((0x01UL<<7)|(APBCLK0_REG_FLG))

#define APBCLK_GPIO0_PCLK_ENABLE                ((0x01UL<<5)|(APBCLK0_REG_FLG))
#define APBCLK_IIC0_PCLK_ENABLE                 ((0x01UL<<4)|(APBCLK0_REG_FLG))
#define APBCLK_USART1_PCLK_ENABLE               ((0x01UL<<3)|(APBCLK0_REG_FLG))
#define APBCLK_USART0_PCLK_ENABLE               ((0x01UL<<2)|(APBCLK0_REG_FLG))
#define APBCLK_SSP0_PCLK_ENABLE                 ((0x01UL<<1)|(APBCLK0_REG_FLG))


//#define APBCLK_USART3_PCLK_ENABLE               ((0x01UL<<31)|(APBCLK1_REG_FLG))
#define APBCLK_ADC_MCLK_ENABLE                  ((0x01UL<<30)|(APBCLK1_REG_FLG))
//#define APBCLK_USART3_PCLK_ENABLE               ((0x01UL<<31)|(APBCLK1_REG_FLG))



/** @defgroup USART_Mode
  * @{
  */ 
#define USART_OPEN_DRAIN_DISABLE                        ((uint32_t)0x0200)
#define USART_OPEN_DRAIN_ENABLE                         ((uint32_t)0x0000)
	  
#define USART_DONT_INVERT38K_OUTPUT                     ((uint32_t)0x0100)
#define USART_INVERT38K_OUTPUT                          ((uint32_t)0x0000)

#define USART_SELECT_38K_MODULATION_MODE                ((uint32_t)0x0010)
#define USART_SELECT_NORMAL__USART_MODE                 ((uint32_t)0x0000)

#define USART_IO_FAST                                   ((uint32_t)0x0000)
#define USART_IO_SLOW                                   ((uint32_t)0x0004)

#define USART_IO_DRIVER_4MA                             ((uint32_t)0x0000)
#define USART_IO_DRIVER_8MA                             ((uint32_t)0x0001)
#define USART_IO_DRIVER_12MA                            ((uint32_t)0x0002)
#define USART_IO_DRIVER_16MA                            ((uint32_t)0x0003)

#define IS_USART_MODE(MODE)                 (((MODE)&(~0x1117)) == 0x00)
#define IS_USART(USART)                     ((((uint32_t)(USART)) == (uint32_t)UART0)|| \
                                             (((uint32_t)(USART)) == (uint32_t)UART1)|| \
                                             (((uint32_t)(USART)) == (uint32_t)UART2)|| \
                                             (((uint32_t)(USART)) == (uint32_t)UART3))


//=======
#define GPIO_INPUT_ENABLE                (0x01UL<<14)
#define UART3_INPUT_ENABLE               (0x01UL<<11)
#define SWIO_INPUT_ENABLE                (0x01UL<<9)
#define SPIF_INPUT_ENABLE                (0x01UL<<8)
#define MAC_INPUT_ENABLE                 (0x01UL<<7)
#define SSP1_INPUT_ENABLE                (0x01UL<<6)
#define SSP0_INPUT_ENABLE                (0x01UL<<5)
#define UART2_INPUT_ENABLE               (0x01UL<<4)
#define UART1_INPUT_ENABLE               (0x01UL<<3)
#define UART0_INPUT_ENABLE               (0x01UL<<2)
#define I2C1_INPUT_ENABLE                (0x01UL<<1)
#define I2C0_INPUT_ENABLE                (0x01UL<<0)

#define IS_PERIPH_INPUT_ENABLE(PERIPH) (((PERIPH) == GPIO_INPUT_ENABLE) || \
                                     ((PERIPH) == UART3_INPUT_ENABLE) || \
                                     ((PERIPH) == SWIO_INPUT_ENABLE) || \
                                     ((PERIPH) == SPIF_INPUT_ENABLE) || \
                                     ((PERIPH) == MAC_INPUT_ENABLE) || \
                                     ((PERIPH) == SSP1_INPUT_ENABLE) || \
                                     ((PERIPH) == SSP0_INPUT_ENABLE) || \
                                     ((PERIPH) == UART2_INPUT_ENABLE) || \
                                     ((PERIPH) == UART1_INPUT_ENABLE) || \
                                     ((PERIPH) == UART0_INPUT_ENABLE) || \
                                     ((PERIPH) == I2C1_INPUT_ENABLE) || \
                                     ((PERIPH) == I2C0_INPUT_ENABLE))



/** @defgroup pin remap
  * @{
  */ 
#if ZB202

#define GPIO000_Remap_MACCRS                0
#define GPIO001_Remap_MACCOL                1
#define GPIO002_Remap_MACRXER               2
#define GPIO003_Remap_MACRXCK               3
#define GPIO004_Remap_MACRXDV               4
#define GPIO005_Remap_MACRXD0               5
#define GPIO006_Remap_MACRXD1               6
#define GPIO007_Remap_MACRXD2               7
#define GPIO008_Remap_MACRXD3               8
#define GPIO009_Remap_SWDCLK                9
#define GPIO010_Remap_SWDIO                 10
#define GPIO011_Remap_SPIFHOLDN             11
#define GPIO012_Remap_SPIFSCLK              12
#define GPIO013_Remap_SPIFTXD               13
#define GPIO014_Remap_UART0RX               14
#define GPIO015_Remap_UART0TX               15
#define GPIO016_Remap_UART1RX               16
#define GPIO017_Remap_UART1TX               17
#define GPIO018_Remap_SSP1FS                18
#define GPIO019_Remap_SSP1RXD               19
#define GPIO020_Remap_SSP1SCLK              20
#define GPIO021_Remap_SSP1TXD               21
#define GPIO022_Remap_I2C1SCL               22
#define GPIO023_Remap_I2C2SDA               23
#define GPIO024_Remap_UART2RX               24
#define GPIO025_Remap_UART2TX               25
#define GPIO026_Remap_PWM0                  26
#define GPIO027_Remap_PWM1                  27
#define GPIO028_Remap_PWM2                  28
#define GPIO029_Remap_PWM3                  29
#define GPIO030_Remap_PWM4                  30
#define GPIO031_Remap_PWM5                  31
#define GPIO102_Remap_SPIFCS                32
#define GPIO103_Remap_SPIFRXD               33
#define GPIO104_Remap_SPIFWPN               34
#define GPIO105_Remap_SSP2FS                35
#define GPIO106_Remap_SSP2RXD               36
#define GPIO107_Remap_SSP2SCLK              37
#define GPIO108_Remap_SSP2TXD               38
#define GPIO109_Remap_UART3RX               39
#define GPIO110_Remap_UART3TX               40
#define GPIO111_Remap_I2C2SCL               41
#define GPIO112_Remap_I2C2SDA               42
#define GPIO120_Remap_MACMDC                43
#define GPIO121_Remap_MACMDIO               44
#define GPIO122_Remap_MACREFCLK             45
#define GPIO123_Remap_MACPHYLINK            46
#define GPIO124_Remap_MACPHYPDN             47
#define GPIO125_Remap_MACWOL                48
#define GPIO126_Remap_MACTXEN               49
#define GPIO127_Remap_MACTXCK               50
#define GPIO128_Remap_MACTXD0               51
#define GPIO129_Remap_MACTXD1               52
#define GPIO130_Remap_MACTXD2               53
#define GPIO131_Remap_MACTXD3               54


#define IS_PIN_REMAP(REMAP)                 (((REMAP) == GPIO000_Remap_MACCRS)|| \
                                             ((REMAP) == GPIO001_Remap_MACCOL)|| \
                                             ((REMAP) == GPIO002_Remap_MACRXER)|| \
                                             ((REMAP) == GPIO003_Remap_MACRXCK)|| \
                                             ((REMAP) == GPIO004_Remap_MACRXDV)|| \
                                             ((REMAP) == GPIO005_Remap_MACRXD0)|| \
                                             ((REMAP) == GPIO006_Remap_MACRXD1)|| \
                                             ((REMAP) == GPIO007_Remap_MACRXD2)|| \
                                             ((REMAP) == GPIO008_Remap_MACRXD3)|| \
                                             ((REMAP) == GPIO009_Remap_SWDCLK)|| \
                                             ((REMAP) == GPIO010_Remap_SWDIO)|| \
                                             ((REMAP) == GPIO011_Remap_SPIFHOLDN)|| \
                                             ((REMAP) == GPIO012_Remap_SPIFSCLK)|| \
                                             ((REMAP) == GPIO013_Remap_SPIFTXD)|| \
                                             ((REMAP) == GPIO014_Remap_UART0RX)|| \
                                             ((REMAP) == GPIO015_Remap_UART0TX)|| \
                                             ((REMAP) == GPIO016_Remap_UART1RX)|| \
                                             ((REMAP) == GPIO017_Remap_UART1TX)|| \
                                             ((REMAP) == GPIO018_Remap_SSP1FS)|| \
                                             ((REMAP) == GPIO019_Remap_SSP1RXD)|| \
                                             ((REMAP) == GPIO020_Remap_SSP1SCLK)|| \
                                             ((REMAP) == GPIO021_Remap_SSP1TXD)|| \
                                             ((REMAP) == GPIO022_Remap_I2C1SCL)|| \
                                             ((REMAP) == GPIO023_Remap_I2C2SDA)|| \
                                             ((REMAP) == GPIO024_Remap_UART2RX)|| \
                                             ((REMAP) == GPIO025_Remap_UART2TX)|| \
                                             ((REMAP) == GPIO026_Remap_PWM0)|| \
                                             ((REMAP) == GPIO027_Remap_PWM1)|| \
                                             ((REMAP) == GPIO028_Remap_PWM2)|| \
                                             ((REMAP) == GPIO029_Remap_PWM3)|| \
                                             ((REMAP) == GPIO030_Remap_PWM4)|| \
                                             ((REMAP) == GPIO031_Remap_PWM5)|| \
                                             ((REMAP) == GPIO102_Remap_SPIFCS)|| \
                                             ((REMAP) == GPIO103_Remap_SPIFRXD)|| \
                                             ((REMAP) == GPIO104_Remap_SPIFWPN)|| \
                                             ((REMAP) == GPIO105_Remap_SSP2FS)|| \
                                             ((REMAP) == GPIO106_Remap_SSP2RXD)|| \
                                             ((REMAP) == GPIO107_Remap_SSP2SCLK)|| \
                                             ((REMAP) == GPIO108_Remap_SSP2TXD)|| \
                                             ((REMAP) == GPIO109_Remap_UART3RX)|| \
                                             ((REMAP) == GPIO110_Remap_UART3TX)|| \
                                             ((REMAP) == GPIO111_Remap_I2C2SCL)|| \
                                             ((REMAP) == GPIO112_Remap_I2C2SDA)|| \
                                             ((REMAP) == GPIO120_Remap_MACMDC)|| \
                                             ((REMAP) == GPIO121_Remap_MACMDIO)|| \
                                             ((REMAP) == GPIO122_Remap_MACREFCLK)|| \
                                             ((REMAP) == GPIO123_Remap_MACPHYLINK)|| \
                                             ((REMAP) == GPIO124_Remap_MACPHYPDN)|| \
                                             ((REMAP) == GPIO125_Remap_MACWOL)|| \
                                             ((REMAP) == GPIO126_Remap_MACTXEN)|| \
                                             ((REMAP) == GPIO127_Remap_MACTXCK)|| \
                                             ((REMAP) == GPIO128_Remap_MACTXD0)|| \
                                             ((REMAP) == GPIO129_Remap_MACTXD1)|| \
                                             ((REMAP) == GPIO130_Remap_MACTXD2)|| \
                                             ((REMAP) == GPIO131_Remap_MACTXD3))

#define UART0_TX_PORT						GPIO0
#define UART0_TX_PIN						GPIO_Pin_15
#define UART0_TX_REMAP						GPIO015_Remap_UART0TX
#define UART0_RX_PORT						GPIO0
#define UART0_RX_PIN						GPIO_Pin_14
#define UART0_RX_REMAP						GPIO014_Remap_UART0RX

#define UART1_TX_PORT						GPIO0
#define UART1_TX_PIN						GPIO_Pin_17
#define UART1_TX_REMAP						GPIO017_Remap_UART1TX
#define UART1_RX_PORT						GPIO0
#define UART1_RX_PIN						GPIO_Pin_16
#define UART1_RX_REMAP						GPIO016_Remap_UART1RX

#define UART2_TX_PORT						GPIO0
#define UART2_TX_PIN						GPIO_Pin_25
#define UART2_TX_REMAP						GPIO025_Remap_UART2TX
#define UART2_RX_PORT						GPIO0
#define UART2_RX_PIN						GPIO_Pin_24
#define UART2_RX_REMAP						GPIO024_Remap_UART2RX

#define UART3_TX_PORT						GPIO1
#define UART3_TX_PIN						GPIO_Pin_10
#define UART3_TX_REMAP						GPIO110_Remap_UART3TX
#define UART3_RX_PORT						GPIO1
#define UART3_RX_PIN						GPIO_Pin_9
#define UART3_RX_REMAP						GPIO109_Remap_UART3RX

#elif DM750

#define GPIO015_Remap_SSP1RXD             ((15)|(0x00<<8))
#define GPIO014_Remap_SSP1TXD             ((14)|(0x00<<8))
#define GPIO013_Remap_SSP1FS              ((13)|(0x00<<8))
#define GPIO012_Remap_SSPSCLK             ((12)|(0x00<<8))
#define GPIO011_Remap_I2C2SDA             ((11)|(0x00<<8))
#define GPIO010_Remap_I2C2SCL             ((10)|(0x00<<8))
#define GPIO009_Remap_I2C1SDA             ((9)|(0x00<<8))
#define GPIO008_Remap_I2C1SCL             ((8)|(0x00<<8))
#define GPIO007_Remap_UART2TX             ((7)|(0x00<<8))
#define GPIO006_Remap_UART2RX             ((6)|(0x00<<8))
#define GPIO005_Remap_GPIO005             ((8)|(0x00<<8))
#define GPIO004_Remap_GPIO004             ((4)|(0x00<<8))
#define GPIO003_Remap_UART1TX             ((3)|(0x00<<8))
#define GPIO002_Remap_UART1RX             ((2)|(0x00<<8))
#define GPIO001_Remap_GPIO001             ((1)|(0x00<<8))
#define GPIO000_Remap_GPIO000             ((0)|(0x00<<8))

#define GPIO031_Remap_UART3TX             ((15|(0x01<<8))
#define GPIO030_Remap_UART3RX             ((14|(0x01<<8))
#define GPIO029_Remap_SWDTMS              ((13|(0x01<<8))
#define GPIO028_Remap_SWDCLK              ((12)|(0x01<<8))
#define GPIO027_Remap_UART0TX             ((11)|(0x01<<8))
#define GPIO026_Remap_UART0RX             ((10)|(0x01<<8))
#define GPIO025_Remap_SPIFHOLD            ((9)|(0x01<<8))
#define GPIO024_Remap_SPIFWP              ((8)|(0x01<<8))
#define GPIO023_Remap_SPIFTX              ((7)|(0x01<<8))
#define GPIO022_Remap_SPIFRX              ((6)|(0x01<<8))
#define GPIO021_Remap_SPIFCS              ((5)|(0x01<<8))
#define GPIO020_Remap_SPIFSCK             ((4)|(0x01<<8))
#define GPIO019_Remap_SSP1RXD             ((3)|(0x01<<8))
#define GPIO018_Remap_SSP1TXD             ((2)|(0x01<<8))
#define GPIO017_Remap_SSP2FS              ((1)|(0x01<<8))
#define GPIO016_Remap_SSP2SCLK            ((0)|(0x01<<8))


#define IS_PIN_REMAP(REMAP)                 (((REMAP) == GPIO015_Remap_SSP1RXD)|| \
                                             ((REMAP) == GPIO014_Remap_SSP1TXD)|| \
                                             ((REMAP) == GPIO013_Remap_SSP1FS)|| \
                                             ((REMAP) == GPIO012_Remap_SSPSCLK)|| \
                                             ((REMAP) == GPIO011_Remap_I2C2SDA)|| \
                                             ((REMAP) == GPIO010_Remap_I2C2SCL)|| \
                                             ((REMAP) == GPIO009_Remap_I2C1SDA)|| \
                                             ((REMAP) == GPIO008_Remap_I2C1SCL)|| \
                                             ((REMAP) == GPIO007_Remap_UART2TX)|| \
                                             ((REMAP) == GPIO006_Remap_UART2RX)|| \
                                             ((REMAP) == GPIO005_Remap_GPIO005)|| \
                                             ((REMAP) == GPIO004_Remap_GPIO004)|| \
                                             ((REMAP) == GPIO003_Remap_UART1TX)|| \
                                             ((REMAP) == GPIO002_Remap_UART1RX)|| \
                                             ((REMAP) == GPIO001_Remap_GPIO001)|| \
                                             ((REMAP) == GPIO000_Remap_GPIO000)|| \
                                             ((REMAP) == GPIO031_Remap_UART3TX)|| \
                                             ((REMAP) == GPIO030_Remap_UART3RX)|| \
                                             ((REMAP) == GPIO029_Remap_SWDTMS)|| \
                                             ((REMAP) == GPIO028_Remap_SWDCLK)|| \
                                             ((REMAP) == GPIO027_Remap_UART0TX)|| \
                                             ((REMAP) == GPIO026_Remap_UART0RX)|| \
                                             ((REMAP) == GPIO025_Remap_SPIFHOLD)|| \
                                             ((REMAP) == GPIO024_Remap_SPIFWP)|| \
                                             ((REMAP) == GPIO023_Remap_SPIFTX)|| \
                                             ((REMAP) == GPIO022_Remap_SPIFRX)|| \
                                             ((REMAP) == GPIO021_Remap_SPIFCS)|| \
                                             ((REMAP) == GPIO020_Remap_SPIFSCK)|| \
                                             ((REMAP) == GPIO019_Remap_SSP2RXD)|| \
                                             ((REMAP) == GPIO018_Remap_SSP2TXD)|| \
                                             ((REMAP) == GPIO017_Remap_SSP2FS)|| \
                                             ((REMAP) == GPIO016_Remap_SSP2SCLK))

#define UART0_TX_PORT						GPIO0
#define UART0_TX_PIN						GPIO_Pin_27
#define UART0_TX_REMAP						GPIO027_Remap_UART0TX
#define UART0_RX_PORT						GPIO0
#define UART0_RX_PIN						GPIO_Pin_26
#define UART0_RX_REMAP						GPIO026_Remap_UART0RX

#define UART1_TX_PORT						GPIO0
#define UART1_TX_PIN						GPIO_Pin_3
#define UART1_TX_REMAP						GPIO003_Remap_UART1TX
#define UART1_RX_PORT						GPIO0
#define UART1_RX_PIN						GPIO_Pin_2
#define UART1_RX_REMAP						GPIO002_Remap_UART1RX

#define UART2_TX_PORT						GPIO0
#define UART2_TX_PIN						GPIO_Pin_7
#define UART2_TX_REMAP						GPIO007_Remap_UART2TX
#define UART2_RX_PORT						GPIO0
#define UART2_RX_PIN						GPIO_Pin_6
#define UART2_RX_REMAP						GPIO006_Remap_UART2RX

#define UART3_TX_PORT						GPIO0
#define UART3_TX_PIN						GPIO_Pin_31
#define UART3_TX_REMAP						GPIO031_Remap_UART3TX
#define UART3_RX_PORT						GPIO0
#define UART3_RX_PIN						GPIO_Pin_30
#define UART3_RX_REMAP						GPIO030_Remap_UART3RX

#endif

/**
  * @}
  */
void SCU_SetUsartMode(USART_TypeDef *  Usartx ,uint32_t mode);
void SCU_PeriphInputConfig(uint32_t perinput);
void SCU_GPIOInputConfig(GPIO_TypeDef * port, uint32_t pin);
void PinRemapConfig(uint32_t Pin_Remap, FunctionalState NewState);
void SCU_DisableALLGPIOInput();

#ifdef __cplusplus
}
#endif
#endif /* __GSMCU_SCU_H__ */


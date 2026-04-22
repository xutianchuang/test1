#ifndef __GSMCU_DMA_H__
#define __GSMCU_DMA_H__
#ifdef __cplusplus
extern "C"
{
#endif

/** @defgroup 
  * @{
  */
#define IS_DMA_PERIPH(PERIPH)       ((PERIPH) == DMA)

/** @defgroup 
  * @{
  */

#define DMA_Channel0    (0)
#define DMA_Channel1    (1)
#define DMA_Channel2    (2)
#define DMA_Channel3    (3)
#define DMA_Channel4    (4)
#define DMA_Channel5    (5)
#define DMA_Channel6    (6)
#define DMA_Channel7    (7)

#define IS_DMA_CHANNEL(CHL)     (((CHL) == DMA_Channel0) || \
                                ((CHL) == DMA_Channel1) ||\
                                ((CHL) == DMA_Channel2) ||\
                                ((CHL) == DMA_Channel3) ||\
                                ((CHL) == DMA_Channel4) ||\
                                ((CHL) == DMA_Channel5) ||\
                                ((CHL) == DMA_Channel6) ||\
                                ((CHL) == DMA_Channel7))

/** @defgroup 
  * @{
  */
#define DMA_TIME1           (0UL)
#define DMA_UART3_TX        (1UL)
#define DMA_UART3_RX        (2UL)
#define DMA_CRC             (3UL)
#define DMA_UART0_TX        (4UL)
#define DMA_UART0_RX        (5UL)
#define DMA_UART1_TX        (6UL)
#define DMA_UART1_RX        (7UL)
#define DMA_UART2_TX        (8UL)
#define DMA_UART2_RX        (9UL)
#define DMA_SSP0_TX         (10UL)
#define DMA_SSP0_RX         (11UL)
#define DMA_SSP1_TX         (12UL)
#define DMA_SSP1_RX         (13UL)
#define DMA_SPI             (14UL)
#define DMA_ADC             (15UL)

#define IS_DMA_CHANNEL_PERIPH(PERIPH)  (((PERIPH) == DMA_TIME1) || \
                                        ((PERIPH) == DMA_UART3_TX) ||\
										((PERIPH) == DMA_UART3_RX) ||\
                                        ((PERIPH) == DMA_CRC) ||\
                                        ((PERIPH) == DMA_UART0_TX) ||\
                                        ((PERIPH) == DMA_UART0_RX) ||\
                                        ((PERIPH) == DMA_UART1_TX) ||\
                                        ((PERIPH) == DMA_UART1_RX) ||\
                                        ((PERIPH) == DMA_UART2_TX) ||\
                                        ((PERIPH) == DMA_UART2_RX) ||\
                                        ((PERIPH) == DMA_SSP0_TX) ||\
                                        ((PERIPH) == DMA_SSP0_RX) ||\
                                        ((PERIPH) == DMA_SSP1_TX) ||\
                                        ((PERIPH) == DMA_SSP1_RX) ||\
                                        ((PERIPH) == DMA_SPI) ||\
                                        ((PERIPH) == DMA_DMA))



/** 
  * @brief  DMA Init structure definition
  */
//CSR
#define CSR_SRC_SIZE_1      (0x00UL<<16)
#define CSR_SRC_SIZE_4      (0x01UL<<16)
#define CSR_SRC_SIZE_8      (0x02UL<<16)
#define CSR_SRC_SIZE_16     (0x03UL<<16)
#define CSR_SRC_SIZE_32     (0x04UL<<16)
#define CSR_SRC_SIZE_64     (0x05UL<<16)
#define CSR_SRC_SIZE_128    (0x06UL<<16)
#define CSR_SRC_SIZE_256    (0x07UL<<16)

#define IS_DMA_CHANNEL_CSR_SRC_SIZE(SIZE)   (((SIZE)&(~(0x07<<16))) == 0)

#define CSR_SRC_WIDTH_8BIT      (0x00UL<<11)
#define CSR_SRC_WIDTH_16BIT     (0x01UL<<11)
#define CSR_SRC_WIDTH_32BIT     (0x02UL<<11)
#define IS_DMA_CHANNEL_CSR_SRC_WIDTH(WIDTH)   ((((WIDTH)&(~(0x03<<11))) == 0)&&(((WIDTH>>11))<0x03))


#define CSR_DST_WIDTH_8BIT      (0x00UL<<11)
#define CSR_DST_WIDTH_16BIT     (0x01UL<<11)
#define CSR_DST_WIDTH_32BIT     (0x02UL<<11)
#define IS_DMA_CHANNEL_CSR_DST_WIDTH(WIDTH)   ((((WIDTH)&(~(0x03<<11))) == 0)&&(((WIDTH>>11))<0x03))


#define CSR_MODE_HANDSHAKE      (0x01<<7)
#define CSR_MODE_NORMAL         (0x00<<7)


#define CSR_SRCAD_CTL_INC   (0x00UL<<5)
#define CSR_SRCAD_CTL_DEC   (0x01UL<<5)
#define CSR_SRCAD_CTL_FIX   (0x02UL<<5)
#define IS_DMA_CHANNEL_CSR_SRCAD_CTL(CTRL)   ((((CTRL)&(~(0x03<<5))) == 0)&&(((CTRL>>5))<0x03))

#define CSR_DSTAD_CTL_INC   (0x00UL<<3)
#define CSR_DSTAD_CTL_DEC   (0x01UL<<3)
#define CSR_DSTAD_CTL_FIX   (0x02UL<<3)
#define IS_DMA_CHANNEL_CSR_DSTAD_CTL(CTRL)   ((((CTRL)&(~(0x03<<3))) == 0)&&(((CTRL>>3))<0x03))


//CFG
#define CFG_DST_HE_SET        (0x01UL<<13)
#define CFG_DST_HE_RESET      (0x00UL<<13)

#define CFG_SRC_HE_SET        (0x01UL<<7)
#define CFG_SRC_HE_RESET      (0x00UL<<7)

#define CFG_INT_ABT_MSK         (0x01<<2)
#define CFG_INT_ERR_MSK         (0x01<<1)
#define CFG_INT_TC_MSK          (0x01<<0)


/** 
  * @brief  DMA Init structure definition
  */

typedef struct
{
  uint32_t CSR;
  uint32_t CFG;
  uint32_t SrcAdd;
  uint32_t DstAddr;
  uint32_t ChlPeriph;
  uint32_t SIZE;
  uint32_t LLP;
  uint32_t SYNC_Flg;
}DMA_InitTypeDef;



/** 
  * @brief  DMA function
  */

void DMA_Init(DMA_TypeDef* DMAx, uint32_t DMA_Channelx,DMA_InitTypeDef* DMA_InitStruct);
void Set_DMA_Channel(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);
void ReSet_DMA_Channel(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);
ITStatus DMA_GetChannelITStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);
uint32_t DMA_GetITStatus(DMA_TypeDef* DMAx);
ITStatus DMA_GetChannelTCITStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);
void DMA_ClearChannelTCITStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);
ITStatus DMA_GetChannelAbtITStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);
void DMA_ClearChannelAbtITStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);
ITStatus DMA_GetChannelErrITStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);
void DMA_ClearChannelErrITStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);
uint32_t DMA_GetAllChannelAllITStatus(DMA_TypeDef* DMAx);
void DMA_ClearAllChannelAllITStatus(DMA_TypeDef* DMAx, uint32_t clvalue);
ITStatus DMA_GetChannelBusyStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);
ITStatus DMA_GetChannelEnableStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);
ITStatus DMA_GetChannelTCStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);
ITStatus DMA_GetChannelAbtStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);
ITStatus DMA_GetChannelErrStatus(DMA_TypeDef* DMAx, uint32_t DMA_Channelx);
uint32_t DMA_GetAllChannelAllStatus(DMA_TypeDef* DMAx);






#ifdef __cplusplus
}
#endif
#endif /* __GSMCU_DMA_H__ */



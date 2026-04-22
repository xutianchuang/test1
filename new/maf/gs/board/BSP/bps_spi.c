#include "common_includes.h"
#include  <gsmcu_hal.h>

#include "gsmcu_spi.h"
#include "bps_spi.h"
#include "os.h"


static SPI_HandleTypeDef SPI_HandleStruct;
static u8 spi_tx_buff[3*1024];
static u8 spi_rx_buff[3*1024];

static u8 recHead=0;
static u8 bootReq=0;
static SPI_CallBackFun pfun=NULL;

u8 *bin_data=0;
u32 total_bytes;
u32 bin_crc;
uint32_t sum4crc(void *p, uint16_t len)
{
	uint16_t len4 = len / 4;
	uint32_t sum = 0;
	uint32_t *temp = (uint32_t *)p;
	for (int i = 0; i < len4; i++)
	{
		sum += *temp;
		temp++;
	}
	p = (void *)((uint32_t)p + len4 * 4);
	len = len - 4 * len4;
	uint32_t last_sum = 0;
	for (int i = 0; i < len; i++)
	{

		last_sum |= ((uint8_t *)p)[i] << (i * 8);
	}
	sum += last_sum;
	return sum;
}

void SPI_RX_DMA_IRQHandler(void)
{
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
	SPI_FRAME *pfrem = (SPI_FRAME *)spi_tx_buff;

	if (pfrem->cmd == CMD_GET_FRAME) //对于接收的部分先接到了长度数据之后再进行
	{
		if (recHead) //等待接收
		{
			if (spi_rx_buff[0] & 0x40) //SPI有个bug导致bootloader和正式的极性不相同因此此处收到的值从0x80变成了0x40
			{
				HRF_SetBootFlag(1);
			}
			SPI_FRAME *prfrem = (SPI_FRAME *)spi_rx_buff;
			if (prfrem->len != 0 && (prfrem->len < sizeof(spi_rx_buff) - 4)) //接收数据长度有效
			{
				SPI_HandleStruct.State = HAL_SPI_STATE_READY; //设置发送完成准备接受以后的数据
				HAL_SPI_TransmitReceive_DMA(&SPI_HandleStruct, &spi_tx_buff[DATA_START_BYTE], &spi_rx_buff[DATA_START_BYTE], prfrem->len + 4);
			}
			else
			{
				if (pfun)
				{
					pfun(prfrem->cmd, 0, spi_rx_buff, prfrem->len);
				}
				//最后一个byte传输完毕发送完毕把CS切换成SPI的cs功能FS会在传输完毕自动拉高
				PinRemapConfig(GPIO018_Remap_SSP1FS, ENABLE);
				CLEAR_BIT(SSP0->IntrCR, SPI_DMA_RFDMAEN);
				CLEAR_BIT(SSP0->IntrCR, SPI_DMA_TFDMAEN);
				SPI_HandleStruct.State = HAL_SPI_STATE_READY;
			}
			recHead = 0; //清除接收头部标志位
		}
		else //已经收到了数据
		{
			SPI_FRAME *prfrem = (SPI_FRAME *)spi_rx_buff;
			u8 status = *(u32 *)&spi_rx_buff[prfrem->len] == sum4crc(spi_rx_buff, prfrem->len);
			if (pfun)
			{
				pfun(prfrem->cmd, status, (u8*)prfrem->data, prfrem->len-DATA_START_BYTE);
			}
			SPI_HandleStruct.State = HAL_SPI_STATE_READY;
			//最后一个byte传输完毕发送完毕把CS切换成SPI的cs功能FS会在传输完毕自动拉高
			PinRemapConfig(GPIO018_Remap_SSP1FS, ENABLE);
			CLEAR_BIT(SSP0->IntrCR, SPI_DMA_RFDMAEN);
			CLEAR_BIT(SSP0->IntrCR, SPI_DMA_TFDMAEN);
		}
	}
	else if (pfrem->cmd == HRF_GET_SYNC_NTB)
	{
		SPI_FRAME *prfrem = (SPI_FRAME *)spi_rx_buff;
		u8 status = pfrem->len == 4 ? prfrem->data[0] == ~prfrem->data[1] : 0;
		if (pfun)
		{
			pfun(pfrem->cmd, status, spi_rx_buff, pfrem->len);
		}
		SPI_HandleStruct.State = HAL_SPI_STATE_READY;
		//最后一个byte传输完毕发送完毕把CS切换成SPI的cs功能FS会在传输完毕自动拉高
		PinRemapConfig(GPIO018_Remap_SSP1FS, ENABLE);
		CLEAR_BIT(SSP0->IntrCR, SPI_DMA_RFDMAEN);
		CLEAR_BIT(SSP0->IntrCR, SPI_DMA_TFDMAEN);
	}
	else
	{
		u8 status = pfrem->len < 4096 ? !memcmp(&spi_rx_buff[DATA_START_BYTE], spi_tx_buff, pfrem->len) : 0; //判断接收是否成功
		if (pfun)
		{
			if (!status && HRF_BootReq() && pfrem->len < 4096) //SPI有个bug导致bootloader和正式的极性不相同并且会少传一个数据
			{
				status = !memcmp(&spi_rx_buff[DATA_START_BYTE - 1], spi_tx_buff, pfrem->len);
			}
			pfun(pfrem->cmd, status, spi_rx_buff, pfrem->len);
		}
		SPI_HandleStruct.State = HAL_SPI_STATE_READY;
		//最后一个byte传输完毕发送完毕把CS切换成SPI的cs功能FS会在传输完毕自动拉高
		PinRemapConfig(GPIO018_Remap_SSP1FS, ENABLE);
		CLEAR_BIT(SSP0->IntrCR, SPI_DMA_RFDMAEN);
		CLEAR_BIT(SSP0->IntrCR, SPI_DMA_TFDMAEN);
	}
	CPU_CRITICAL_EXIT();
}
void SPI_TX_DMA_IRQHandler(void)
{
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
	SPI_FRAME *pfrem = (SPI_FRAME *)spi_tx_buff;
	if (pfrem->cmd == CMD_GET_FRAME) //对于接收的部分先接到了长度数据之后再进行
	{
	}
	else
	{
		//PinRemapConfig(GPIO018_Remap_SSP1FS, ENABLE);
	}
	CPU_CRITICAL_EXIT();
}
void SPI_Open(uint32_t SCLK_Freq)
{
	SPI_HandleStruct.Instance 	      = SSP0;
	SPI_HandleStruct.Init.Mode 	      = SPI_MODE_MASTER;
	SPI_HandleStruct.Init.DataSize    = SPI_DATASIZE_8BIT;
	SPI_HandleStruct.Init.FirstBit    = SPI_FIRSTBIT_MSB;
	SPI_HandleStruct.Init.CLKPolarity = SPI_POLARITY_HIGH;
	SPI_HandleStruct.Init.CLKPhase    = SPI_PHASE_1EDGE;
	SPI_HandleStruct.Init.SCLKDIV     = 50000000/(2*SCLK_Freq)    -1;
	SPI_HandleStruct.Init.PaddingCycle= 1;
	HAL_SPI_Init(&SPI_HandleStruct);
	BSP_IntDmacChannelVectSet(SPI_DMA_RX_CHL,SPI_RX_DMA_IRQHandler);
	BSP_IntDmacChannelVectSet(SPI_DMA_TX_CHL,SPI_TX_DMA_IRQHandler);
	SCU_GPIOInputConfig(SPI_SS_PORT,GPIO_Pin_19);
	SCU->EX_PERIPH_PIN_REMAP0&=~(0xF<<18);//SPI 0 IO mux
}
void SPI_SetPolarity(u8 mode)
{
	if (mode)
	{
		SPI_HandleStruct.Instance->SSPCR0 = (SPI_HandleStruct.Instance->SSPCR0 & ~0x3) | SPI_POLARITY_HIGH | SPI_PHASE_1EDGE;
	}
	else
	{
		SPI_HandleStruct.Instance->SSPCR0 = (SPI_HandleStruct.Instance->SSPCR0 & ~0x3) | SPI_POLARITY_HIGH | SPI_PHASE_2EDGE;
	}

}

int HRF_BootReq(void)
{
	return bootReq;
}
void HRF_SetBootFlag(int v)
{
	bootReq=v;
}
void SPI_TransData(SPI_CMD cmd,u32 arg,u8 *data,u16 len,SPI_CallBackFun call)
{
	PinRemapConfig(GPIO018_Remap_SSP1FS, DISABLE);
	OS_ERR err;

	GPIO_ResetBits(SPI_SS_PORT, SPI_SS_PIN);
	SPI_FRAME *pfrem = (SPI_FRAME *)spi_tx_buff;
	switch (cmd)
	{
	case CMD_BOOT_START: //发送boot 起始报文
		{
			pfrem->cmd = CMD_BOOT_START;
			pfrem->len = sizeof(SPI_BOOT_START) + DATA_START_BYTE;
			SPI_BOOT_START *pinfo = (SPI_BOOT_START *)&spi_tx_buff[DATA_START_BYTE];
			pinfo->FileAddr = 0x20000000;
			pinfo->FileSize = total_bytes;
			pinfo->FileCrc = bin_crc;
			*(uint32_t *)&spi_tx_buff[pfrem->len] = sum4crc(spi_tx_buff, pfrem->len);
			pfun = call;
			OSTimeDly(1, OS_OPT_TIME_DLY, &err); //bootloader 是25M需要等待一会让从机处理完毕
			HAL_SPI_TransmitReceive_DMA(&SPI_HandleStruct, spi_tx_buff, spi_rx_buff, pfrem->len + DATA_START_BYTE);
			break;
		}
	case CMD_BOOT_FILE:
		{
			pfrem->cmd = CMD_BOOT_FILE;
			if (arg + 2048 > total_bytes)
			{
				OSTimeDly(1, OS_OPT_TIME_DLY, &err);
				pfrem->len = total_bytes - arg;
			}
			else
			{
				pfrem->len = 2048;
			}
			SPI_BOOT_FILE *p = (SPI_BOOT_FILE *)&spi_tx_buff[DATA_START_BYTE];
			p->FileSize = pfrem->len;
			memcpy(p->data, &bin_data[arg], p->FileSize);
			pfrem->len = DATA_START_BYTE + p->FileSize +sizeof(SPI_BOOT_FILE);
			p->FileAddr = arg  + 0x20000000;
			*(uint32_t *)&spi_tx_buff[pfrem->len] = sum4crc(spi_tx_buff, pfrem->len);
			pfun = call;
			HAL_SPI_TransmitReceive_DMA(&SPI_HandleStruct, spi_tx_buff, spi_rx_buff, pfrem->len + DATA_START_BYTE);
			break;
		}
	case CMD_BOOT_END:
		{
			pfrem->cmd = CMD_BOOT_END;
			pfrem->len = DATA_START_BYTE + sizeof(SPI_BOOT_END);
			memset(&pfrem->boot_end, 0, sizeof(SPI_BOOT_END));
			*(uint32_t *)&spi_tx_buff[pfrem->len] = sum4crc(spi_tx_buff, pfrem->len);
			pfun = call;
			OSTimeDly(1, OS_OPT_TIME_DLY, &err);
			HAL_SPI_TransmitReceive_DMA(&SPI_HandleStruct, spi_tx_buff, spi_rx_buff, pfrem->len + DATA_START_BYTE);
			break;
		}
	case CMD_SEND_CONCERT:
	case CMD_SEND_BEACON:
	case CMD_SEND_FRAME: //发送hrf帧
		{
			pfrem->cmd = cmd;
			pfrem->len = DATA_START_BYTE + len;
			memcpy(&spi_tx_buff[DATA_START_BYTE], data, len);
			*(uint32_t *)&spi_tx_buff[pfrem->len] = sum4crc(spi_tx_buff, pfrem->len);
			pfun = call;
			//OSTimeDly(1, OS_OPT_TIME_DLY, &err); //进入正常的通讯后子板的PLL配置好之后就无需等待了
			HAL_SPI_TransmitReceive_DMA(&SPI_HandleStruct, spi_tx_buff, spi_rx_buff, pfrem->len + DATA_START_BYTE);
			break;
		}
	case CMD_GET_FRAME:
		{
			recHead = 1;
			pfrem->cmd = CMD_GET_FRAME;
			pfrem->len = 0; //接收时长度由从机给出  无需准备crc
			pfun = call;
			//OSTimeDly(1, OS_OPT_TIME_DLY, &err); //进入正常的通讯后子板的PLL配置好之后就无需等待了
			HAL_SPI_TransmitReceive_DMA(&SPI_HandleStruct, spi_tx_buff, spi_rx_buff, DATA_START_BYTE);
			break;
		}
	case CMD_CONTROL:
		{
			recHead = 1;
			pfrem->cmd = CMD_CONTROL;
			pfrem->len = len + DATA_START_BYTE;
			memcpy(&spi_tx_buff[DATA_START_BYTE], data, len);
			*(uint32_t *)&spi_tx_buff[pfrem->len] = sum4crc(spi_tx_buff, pfrem->len);
			pfun = call;
			//OSTimeDly(1, OS_OPT_TIME_DLY, &err); //进入正常的通讯后子板的PLL配置好之后就无需等待了
			HAL_SPI_TransmitReceive_DMA(&SPI_HandleStruct, spi_tx_buff, spi_rx_buff, pfrem->len + DATA_START_BYTE);
			break;
		}
	case HRF_GET_SYNC_NTB:
		{
			recHead = 1;
			pfrem->cmd = HRF_GET_SYNC_NTB;
			pfrem->len = len;
			memcpy(&spi_tx_buff[DATA_START_BYTE], data, len);
			*(uint32_t *)&spi_tx_buff[pfrem->len] = sum4crc(spi_tx_buff, pfrem->len);
			pfun = call;
			//OSTimeDly(1, OS_OPT_TIME_DLY, &err); //进入正常的通讯后子板的PLL配置好之后就无需等待了
			HAL_SPI_TransmitReceive_DMA(&SPI_HandleStruct, spi_tx_buff, spi_rx_buff, pfrem->len + DATA_START_BYTE+4);
			break;
		}

	}

}





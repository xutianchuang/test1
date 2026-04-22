#ifndef __BPS_SPI_H__
#define __BPS_SPI_H__
#ifdef __cplusplus
extern "C"
{
#endif


#include<stdint.h>

typedef enum
{
	CMD_ACK=1,

	CMD_BOOT_START,
	CMD_BOOT_FILE,
	CMD_BOOT_END,
	CMD_SEND_FRAME,
	CMD_SEND_BEACON,
	CMD_GET_FRAME,
	CMD_CONTROL,
	HRF_GET_SYNC_NTB,
	HRF_GET_CHANNEL,
	CMD_SEND_CONCERT,

}SPI_CMD;

typedef enum
{
	HRF_SET_CHANNEL,
	HRF_SET_PLL,
	HRF_SET_TEI,
	HRF_RESERT,
	HRF_SET_TEST_MODE,
}HRF_CONTROL_CMD;
typedef struct{
	uint32_t FileSize;//文件大小
	uint32_t FileAddr;//文件首地址存放位置
	uint32_t FileCrc;//文件校验
}SPI_BOOT_START;
typedef struct{
	uint32_t FileAddr;//文件地址
	uint32_t FileSize;//文件大小
	uint8_t data[0];
}SPI_BOOT_FILE;
typedef struct{
	uint32_t FileCrc;//文件校验
}SPI_BOOT_END;

typedef struct{
	uint32_t cmd:8;//命令
	uint32_t len:16;//除crc以外的所有字节长度  当len为0时为读数据  （HRF应用时从设备每一帧的LEN位置固定填写当前可返回的数据帧的长度
	uint32_t trun_time:8; //低8位缓冲时间   //每次发送方向切换时 需要一个转换延时用来给从机响应时间  cmd处固定有一个转换时延
	uint32_t trun_array[15];//第一版测试用使用25M时钟 后面改成100M之后需要把这个值改小
	union {
		uint32_t data[1];
		SPI_BOOT_START boot_start;
		SPI_BOOT_FILE boot_file;
		SPI_BOOT_END boot_end;
	};
}SPI_FRAME;

typedef void (*SPI_CallBackFun)(u8 cmd,u8 status,u8 *data,u16 len);

#define DATA_START_BYTE   64

#define SPI_SS_PORT GPIO0
#define SPI_SS_PIN  GPIO_Pin_18



void SPI_TransData(SPI_CMD cmd,u32 arg,u8 *data,u16 len,SPI_CallBackFun call);
int HRF_BootReq(void);
void HRF_SetBootFlag(int v);
void SPI_Open(uint32_t SCLK_Freq);
void SPI_SetPolarity(u8 mode);
#ifdef __cplusplus
}
#endif
#endif /* __BPS_SPI_H__ */


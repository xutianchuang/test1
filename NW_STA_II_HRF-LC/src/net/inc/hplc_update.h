#ifndef _HPLC_UPDATE_H_
#define _HPLC_UPDATE_H_

typedef struct _dec_file_head{
	u32 factorFlag;
	u32 fileSize;
	u32 decFileSize;
	u32 crc;
	u8  props[5];
	u8  data[0];
}DEC_FILE_HEAD;

//初始化
void UpdateInit(u32 tryRunTime);
//强制进入试运行状态
void UpdateEnterTryRun(u32 runTime);

void UpdateReceiveEventData(u8* data,u16 len);

//升级状态
u8 GetUpdateState(void);

//有效块数
u32 GetUpdateBlockNum(u32 beginBlock,u32 blockNum,u32 updateID,u8 *bitmap,u16 *bitmapSize);

//升级ID
u32 GetUpdateID(void);

//升级文件CRC
u32 GetUpdateFileCRC(void);

//升级文件大小
u32 GetUpdateFileSize(void);

bool StartFirstProc(void *fsm,void* arg);

void DecCode(void);

#endif

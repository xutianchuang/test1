#ifndef _HPLC_TMI_H_
#define _HPLC_TMI_H_
//缓存待发送的MAC帧
#pragma pack(push,4)
typedef struct
{
	SOF_PARAM sofParam;
	u8 pdu[MEM_BLOCK_SIZE];
	u16 len;
	pSendMacCallback callback;
	u32 opt;
	u8 resendFlag;
}HPLC_MAC_CACH;
#pragma pack(pop)
//处理一切拷贝模式分包组包

void MpduTask(void *arg);
void BcsmaMpduTask(void *arg);
void HrfMpduTask(void *arg);
void HrfBcsmaMpduTask(void *arg);
u16 HRFBestMcs(u8 isBo,u8 option,u16 pack_len);
u16 HRFPhrBestMcs(u8 isBo,u8 option,u16 pack_len);
int GetPbIndex(int len);
//发送MAC帧接口
bool SendMac(SOF_PARAM *sofParam,u8* mac,u16 len,pSendMacCallback callback,u32 opt);

void MpduTMIInit(void);
#endif

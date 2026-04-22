#ifndef _HPLC_MAC_H_
#define _HPLC_MAC_H_

//网络业务

typedef void (*pfMacCallback)(const P_HPLC_PARAM pHplcParam, P_MAC_FRAME pMacFrame,SOF_PARAM *sofParam);
typedef void (*pfMMFrameCallback)(P_MM_FRAME pMMFrame);

//MAC层初始化
void MacInit(void);

//处理MAC帧
void MacHandleMAC(SOF_PARAM *sofParam,BPLC_PARAM* bplcParam,u8* data,u16 len);

//获取MAC序列号
u16 MacGetMacSequence(void);

//获取上次发送的端口序列号
u32 MacGetPortSequence(void);

//生成一个新的端口序列号
u32 MacCreatePortSequence(void);

void MacSetMacFrameCallback(pfMacCallback macCallback);
void MacSetMMFrameCallback(pfMMFrameCallback mmCallback);
    
#endif

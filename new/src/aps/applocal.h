#ifndef _APPLOCAL_H_
#define _APPLOCAL_H_

//应用层本地任务
void AppLocalTask(void *arg);

//返回是否已经绑定了地址
bool IsBindLocalMac(void);

//绑定地址是否已经超时(目前设定为1分钟)
bool IsBindLocalTimeout(void);

//返回电能表协议
u8 GetMeterProtocol(void);

//接收APP层的数据
bool AppLocalReceiveData(u8* data,u16 len);
#endif

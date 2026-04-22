/*
*
*/
#ifndef __BSP_PORT_H__
#define __BSP_PORT_H__
#include <gsmcuxx_hal_def.h>
typedef   void (*GPIOx_CALLBACK)(unsigned int);
void DeviceResetOpen(GPIOx_CALLBACK fun);
void ResetInterrupt(FunctionalState open_close);


void StaOpen(void);
void StaWrite(uint32_t mode);





//Event
void EventOpen(void);
uint32_t EventRead();
void EventClose(void);
void EventReopen(void);


//input 0_16
void SetOpen(void);
void SetWrite(uint32_t mode);



void InsertOpen(void);
uint32_t InsertRead(void);
void InsertTopoFuncOpen(void);
void InsertTopoFuncWrite(u8 vaile);

void MultiOpen(void);

void CapEnCtrOpen(void);
void CapEnCtrWrite(u32 charge);

//
void HplcLineDriverOpen(void);
void HplcSetLineDriver(bool flg);
void HplcSwitchWrite(int IsTx);

void HrfDirPinInit(void);
void HrfDirPinWrite(int v);

#define LD_OUT_OFF()    GPIO_SetBits(LD_PORT,LD_PIN)
#define LD_OUT_ON()     GPIO_ResetBits(LD_PORT,LD_PIN)

void BPS_AddGpioIrq(u32 port,u32 pin,GPIOx_CALLBACK func);

void StaModeOpen(void);
int GetStaModeValue(void);
extern const u32 myDevicType;

void U485EnOpen(void);
void U485EnWrite(uint32_t mode);
#endif


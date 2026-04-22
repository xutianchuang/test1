/*
*
*/
#ifndef __BSP_PORT_H__
#define __BSP_PORT_H__
#include <gsmcuxx_hal_def.h>
typedef   void (*GPIOx_CALLBACK)(unsigned int);
void DeviceResetOpen(GPIOx_CALLBACK fun);
void ResetInterrupt(FunctionalState open_close);

int32_t ResetRead();




void TxSwitchOpen(void);
void TxSwitchSet(bool flg, u8 phase);


void RxSwitchOpen(void);
void RxSwitchSet(bool flg, uint32_t phase);



#ifdef SWITCH_A_TX_PORT
#define PA_OUT_ON()    TxSwitchSet(true, BPS_PHASE_A)
#define PA_OUT_OFF()   TxSwitchSet(false, BPS_PHASE_A)
#else
#define PA_OUT_ON()   
#define PA_OUT_OFF()    
#endif

#ifdef SWITCH_A_RX_PORT
#define PA_IN_ON()    RxSwitchSet(true, BPS_PHASE_A)
#define PA_IN_OFF()   RxSwitchSet(false, BPS_PHASE_A)
#else
#define PA_IN_ON()   
#define PA_IN_OFF()    
#endif

#ifdef SWITCH_B_TX_PORT
#define PB_OUT_ON()    TxSwitchSet(true, BPS_PHASE_B)
#define PB_OUT_OFF()   TxSwitchSet(false, BPS_PHASE_B)
#else
#define PB_OUT_ON()   
#define PB_OUT_OFF()    
#endif

#ifdef SWITCH_B_RX_PORT
#define PB_IN_ON()    RxSwitchSet(true, BPS_PHASE_B)
#define PB_IN_OFF()   RxSwitchSet(false, BPS_PHASE_B)
#else
#define PB_IN_ON()   
#define PB_IN_OFF()    
#endif

#ifdef SWITCH_C_TX_PORT
#define PC_OUT_ON()    TxSwitchSet(true, BPS_PHASE_C)
#define PC_OUT_OFF()   TxSwitchSet(false, BPS_PHASE_C)
#else
#define PC_OUT_ON()   
#define PC_OUT_OFF()    
#endif

#ifdef SWITCH_C_RX_PORT
#define PC_IN_ON()    RxSwitchSet(true, BPS_PHASE_C)
#define PC_IN_OFF()   RxSwitchSet(false, BPS_PHASE_C)
#else
#define PC_IN_ON()   
#define PC_IN_OFF()    
#endif


//
void HplcLineDriverOpen(void);
void HplcSetLineDriver(bool flg);


#define LD_OUT_OFF()    GPIO_PinWrite(LD_PORT,LD_PIN,1)
#define LD_OUT_ON()     GPIO_PinWrite(LD_PORT,LD_PIN,0)

void BPS_AddGpioIrq(u32 port,u32 pin,GPIOx_CALLBACK func);
void CcoModeOpen(void);
int GetCcoModeValue(void);

extern const u32 CCO_Hard_Type;
#endif


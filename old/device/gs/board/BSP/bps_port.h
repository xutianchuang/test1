/*
*
*/
#ifndef __BSP_PORT_H__
#define __BSP_PORT_H__
#include <gsmcuxx_hal_def.h>
typedef   void (*GPIOx_CALLBACK)(unsigned int);
void DeviceResetOpen(GPIOx_CALLBACK fun);
void ResetInterrupt(FunctionalState open_close);
#if defined(RESET_PORT_USE_PORT0)
#define RESET_PORT				GPIO0
#elif defined(RESET_PORT_USE_PORT1)
#define RESET_PORT				GPIO1
#endif

#if defined(RESET_PORT_3PHASE_USE_PORT0)
#define RESET_PORT_3PHASE		GPIO0
#elif defined(RESET_PORT_3PHASE_USE_PORT1)
#define RESET_PORT_3PHASE		GPIO1
#endif

#if DEV_STA
//STA
#if defined(MODE_PIN1_PORT_USE_PORT0)
#define MODE_PIN1_PORT			GPIO0
#elif defined(MODE_PIN1_PORT_USE_PORT1)
#define MODE_PIN1_PORT			GPIO1
#endif

#if defined(MODE_PIN2_PORT_USE_PORT0)
#define MODE_PIN2_PORT			GPIO0
#elif defined(MODE_PIN2_PORT_USE_PORT0)
#define MODE_PIN2_PORT			GPIO1
#endif

void StaOpen(void);
void StaWrite(uint32_t mode);
uint32_t StaRead();
void StaClose(void);

#if defined(STAOUT_PORT_USE_PORT0)
#define STAOUT_PORT				GPIO0
#elif defined(STAOUT_PORT_USE_PORT1)
#define STAOUT_PORT				GPIO1
#endif

#if defined(STAOUT_PORT_3PHASE_USE_PORT0)
#define STAOUT_PORT_3PHASE		GPIO0
#elif defined(STAOUT_PORT_3PHASE_USE_PORT1)
#define STAOUT_PORT_3PHASE		GPIO1
#endif


//Event
void EventOpen(void);
void EventWrite(uint32_t mode);
uint32_t EventRead();
void EventClose(void);
void EventReopen(void);
#if defined(EVENT_PORT_USE_PORT0)
#define EVENT_PORT				GPIO0
#define EVENT_PORT_IRQn			GPIO0_IRQn
#elif defined(EVENT_PORT_USE_PORT1)
#define EVENT_PORT				GPIO1
#define EVENT_PORT_IRQn			GPIO1_IRQn
#endif

#if defined(EVENT_PORT_3PHASE_USE_PORT0)
#define EVENT_PORT_3PHASE		GPIO0
#define EVENT_PORT_3PHASE_IRQn	GPIO0_IRQn
#elif defined(EVENT_PORT_3PHASE_USE_PORT1)
#define EVENT_PORT_3PHASE		GPIO1
#define EVENT_PORT_3PHASE_IRQn	GPIO1_IRQn
#endif

//input 0_16
void SetOpen(void);
void SetWrite(uint32_t mode);
uint32_t SetRead();
void SetClose(void);

#if defined(SET_PORT_USE_PORT0)
#define SET_PORT				GPIO0
#elif defined(SET_PORT_USE_PORT1)
#define SET_PORT				GPIO1
#endif

#if defined(SET_PORT_3PHASE_USE_PORT0)
#define SET_PORT_3PHASE			GPIO0
#elif defined(SET_PORT_3PHASE_USE_PORT1)
#define SET_PORT_3PHASE			GPIO1
#endif

void InsertOpen(void);
uint32_t InsertRead(void);
void U485EnOpen(void);
void U485EnWrite(uint32_t mode);
void InsertTopoFuncOpen(void);
void InsertTopoFuncWrite(BitAction vaile);
#if defined(PULLOUT_PORT_USE_PORT0)
#define PULLOUT_PORT			GPIO0
#elif defined(PULLOUT_PORT_USE_PORT1)
#define PULLOUT_PORT			GPIO1
#endif

#if defined(PULLOUT_PORT_3PHASE_USE_PORT0)
#define PULLOUT_PORT_3PHASE		GPIO0
#elif defined(PULLOUT_PORT_3PHASE_USE_PORT1)
#define PULLOUT_PORT_3PHASE		GPIO1
#endif

#if defined(TOPO_PORT_USE_PORT0)
#define TOPO_PORT			    GPIO0
#endif

#if defined(TOPO_PORT_3PHASE_USE_PORT0)
#define TOPO_PORT_3PHASE		GPIO0
#endif

#elif DEV_CCO
void TxSwitchOpen(void);
void TxSwitchSet(bool flg, u8 phase);

#if defined(SWITCH_A_TX_PORT_USE_PORT0)
#define SWITCH_A_TX_PORT		GPIO0
#elif defined(SWITCH_A_TX_PORT_USE_PORT1)
#define SWITCH_A_TX_PORT		GPIO1
#endif

#if defined(SWITCH_B_TX_PORT_USE_PORT0)
#define SWITCH_B_TX_PORT		GPIO0
#elif defined(SWITCH_B_TX_PORT_USE_PORT1)
#define SWITCH_B_TX_PORT		GPIO1
#endif

#if defined(SWITCH_C_TX_PORT_USE_PORT0)
#define SWITCH_C_TX_PORT		GPIO0
#elif defined(SWITCH_C_TX_PORT_USE_PORT1)
#define SWITCH_C_TX_PORT		GPIO1
#endif

void RxSwitchOpen(void);
void RxSwitchSet(bool flg, uint32_t phase);

#if defined(SWITCH_A_RX_PORT_USE_PORT0)
#define SWITCH_A_RX_PORT		GPIO0
#elif defined(SWITCH_A_RX_PORT_USE_PORT1)
#define SWITCH_A_RX_PORT		GPIO1
#endif

#if defined(SWITCH_B_RX_PORT_USE_PORT0)
#define SWITCH_B_RX_PORT		GPIO0
#elif defined(SWITCH_B_RX_PORT_USE_PORT1)
#define SWITCH_B_RX_PORT		GPIO1
#endif

#if defined(SWITCH_C_RX_PORT_USE_PORT0)
#define SWITCH_C_RX_PORT		GPIO0
#elif defined(SWITCH_C_RX_PORT_USE_PORT1)
#define SWITCH_C_RX_PORT		GPIO1
#endif

#define PA_OUT_ON()    GPIO_SetBits(SWITCH_A_TX_PORT, SWITCH_A_TX_PIN)
#define PA_OUT_OFF()    GPIO_ResetBits(SWITCH_A_TX_PORT,SWITCH_A_TX_PIN)

#define PA_IN_ON()    GPIO_SetBits(SWITCH_A_RX_PORT,SWITCH_A_RX_PIN)
#define PA_IN_OFF()    GPIO_ResetBits(SWITCH_A_RX_PORT,SWITCH_A_RX_PIN)

#define PB_OUT_ON()    GPIO_SetBits(SWITCH_B_TX_PORT,SWITCH_B_TX_PIN)
#define PB_OUT_OFF()    GPIO_ResetBits(SWITCH_B_TX_PORT,SWITCH_B_TX_PIN)

#define PB_IN_ON()    GPIO_SetBits(SWITCH_B_RX_PORT,SWITCH_B_RX_PIN)
#define PB_IN_OFF()    GPIO_ResetBits(SWITCH_B_RX_PORT,SWITCH_B_RX_PIN)

#define PC_OUT_ON()    GPIO_SetBits(SWITCH_C_TX_PORT,SWITCH_C_TX_PIN)
#define PC_OUT_OFF()    GPIO_ResetBits(SWITCH_C_TX_PORT,SWITCH_C_TX_PIN)

#define PC_IN_ON()    GPIO_SetBits(SWITCH_C_RX_PORT,SWITCH_C_RX_PIN)
#define PC_IN_OFF()    GPIO_ResetBits(SWITCH_C_RX_PORT,SWITCH_C_RX_PIN)

#endif
//
void HplcLineDriverOpen(void);
void HplcSetLineDriver(bool flg);

#if defined(LD_PORT_USE_PORT0)
#define LD_PORT					GPIO0
#elif defined(LD_PORT_USE_PORT1)
#define LD_PORT					GPIO1
#endif

#if defined(LD_PORT_3PHASE_USE_PORT0)
#define LD_PORT_3PHASE			GPIO0
#elif defined(LD_PORT_3PHASE_USE_PORT1)
#define LD_PORT_3PHASE			GPIO1
#endif

#define LD_OUT_OFF()    GPIO_SetBits(LD_PORT,LD_PIN)
#define LD_OUT_ON()     GPIO_ResetBits(LD_PORT,LD_PIN)

#if defined(MULTI_PORT_USE_PORT0)
#define MULTI_PORT				GPIO0
#elif defined(MULTI_PORT_USE_PORT1)
#define MULTI_PORT				GPIO1
#endif

#if defined(MULTI_PORT_3PHASE_USE_PORT0)
#define MULTI_PORT_3PHASE		GPIO0
#elif defined(MULTI_PORT_3PHASE_USE_PORT1)
#define MULTI_PORT_3PHASE		GPIO1
#endif

void BPS_AddGpio0Irq(u32 pin,GPIOx_CALLBACK func);
void BPS_AddGpio1Irq(u32 pin,GPIOx_CALLBACK func);
void StaModeOpen(void);
int GetStaModeValue(void);
extern const u32 myDevicType;
void MultiOpen(void);
#endif


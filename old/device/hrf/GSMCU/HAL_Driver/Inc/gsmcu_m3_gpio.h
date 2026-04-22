/****************************************************************
 *
 * GPIO driver head file
 * @file gsmcu_m3_gpio.h
 * @author sitieqiang 
 * @version 0.1
 * @date 2016/10/21 15:36:30
 * @note None
 *
*****************************************************************/

#ifndef __M3_GPIO_H
#define __M3_GPIO_H
#include <stdint.h>
#include "GSMCU_M3.h"


#ifdef __cplusplus
extern "C" {
#endif




// GPIO Direction

typedef enum _GPIO_DIR{
	GPIO_INPUT,
	GPIO_OUTPUT,
}GPIO_DIR;

typedef enum _GPIO_TRIGER{
	GPIO_TRIGER_NONE,
	GPIO_RISING_TRIGER,
	GPIO_FALLING_TRIGER,
	GPIO_BOTHEDGE_TRIGER,
	GPIO_LEVEL_TRIGER_H,
	GPIO_LEVEL_TRIGER_L
}GPIO_TRIGER;


extern EPORT_Type *  const port_array[10];
/**
 * Configure GPIO pin direction input||output
 * 
 * @param port_num GPIO number (0..8)
 * @param pin_num  Port pin number
 * @param dir      GPIO_DIR_INPUT, GPIO_DIR_OUTPUT
 */
__STATIC_INLINE void GPIO_SetPinDir(uint32_t port, uint32_t pin_num, GPIO_DIR dir)
{
	EPORT_Type *Pointer=port_array[port];//(EPORT_Type *)(port*0x10000+EPORT_BASE);
	dir?(Pointer->EPDDR|=1<<pin_num):( Pointer->EPDDR&=~(1<<pin_num) );
}

/**
 * Configure GPIO port direction
 * 
 * @param port   GPIO number (0..8)
 * @param dir    GPIO Direction
 */
__STATIC_INLINE void GPIO_SetPortDir(uint32_t port,  GPIO_DIR dir)
{
	EPORT_Type *Pointer=port_array[port];//(EPORT_Type *)(port*0x10000+EPORT_BASE);
	Pointer->EPDDR=dir;
}

/**
 * Write port pin
 * 
 * @param port GPIO number
 * @param pin_num  pin_num
 * @param val      Port pin value (0 or 1)
 */
__STATIC_INLINE void GPIO_PinWrite (uint32_t port, uint32_t pin_num, uint32_t val) 
{

	EPORT_Type *Pointer=port_array[port];//(EPORT_Type *)(port*0x10000+EPORT_BASE);
	val?(Pointer->EPBSR=(1U<<pin_num)):\
		(Pointer->EPBCR=((1U<<pin_num)) );
}
/**
 * Read port pin
 * 
 * @param port GPIO number (0..8)
 * @param pin_num  Port pin number
 * 
 * @return pin value (0 or 1)
 */
__STATIC_INLINE uint32_t GPIO_PinRead (uint32_t port, uint32_t pin_num) 
{
	EPORT_Type *Pointer=port_array[port];//(EPORT_Type *)(port*0x10000+EPORT_BASE);
	return (Pointer->EPPDR&(1UL << pin_num))?1:0;
}
/**
 * Write port pins
 * 
 * @param port GPIO number (0..8)
 * @param mask     Selected pins
 * @param val      Pin values
 */
__STATIC_INLINE void GPIO_PortWrite (uint32_t port, uint32_t val) 
{
	EPORT_Type *Pointer=port_array[port];//(EPORT_Type *)(port*0x10000+EPORT_BASE);
	Pointer->EPDR=val;
}
/**
 * Read port pins
 * 
 * @param port port   GPIO number (0..8)
 * 
 * @return port pin inputs
 */
__STATIC_INLINE uint32_t GPIO_PortRead (uint32_t port)
{
	EPORT_Type *Pointer=port_array[port];//(EPORT_Type *)(port*0x10000+EPORT_BASE);EPORT_Type *Pointer=(EPORT_Type *)(port*0x10000+EPORT_BASE);
	return Pointer->EPPDR;
}

/**
 * Not pin data
 * 
 * @param port GPIO number (0..8)
 * @param pin_num  Pin num
 */
__STATIC_INLINE void GPIO_TogglePin(uint32_t port, uint32_t pin_num)
{
	EPORT_Type *Pointer=port_array[port];//(EPORT_Type *)(port*0x10000+EPORT_BASE);
	Pointer->EPBTR = (1U << pin_num);
}
/**
 * Not port data
 * 
 * @param port GPIO number (0..8)
 * @param val  Pin Mask values
 */
__STATIC_INLINE void GPIO_TogglePort(uint32_t port, uint32_t val)
{
	EPORT_Type *Pointer=port_array[port];//(EPORT_Type *)(port*0x10000+EPORT_BASE);
	Pointer->EPBTR = val;
}


///////////////new add zj 0828
__STATIC_INLINE void GPIO_Port_settrigger (uint32_t port, uint32_t pin, uint32_t val) 
{
	EPORT_Type *Pointer=port_array[port];//(EPORT_Type *)(port*0x10000+EPORT_BASE);

	
	if((val == GPIO_LEVEL_TRIGER_H)||(val == GPIO_LEVEL_TRIGER_L))
	{
		Pointer ->EPLPR = (val == GPIO_LEVEL_TRIGER_H)? (Pointer ->EPLPR|(1<<pin)): (Pointer ->EPLPR&~(0x1<<pin));
		Pointer->EPPAR &=~(0x3<<(pin*2)) ;
		val = 0;
	}
	Pointer->EPPAR = (Pointer->EPPAR&(~(0x3<<(pin*2))))|(val<<(pin*2)) ;	
	Pointer->EPFR = (0x1<<pin);
	Pointer->EPLFR = (0x1<<pin);
#if defined(ZB204_CHIP)
	Pointer->EPIER|= (0x1<<pin);  //open poer interrupt enable
#elif defined(ZB205_CHIP)
	if(val == GPIO_BOTHEDGE_TRIGER)
	{
		Pointer->EPIER|= (0x1<<pin);
		Pointer->EPIER1 |= (0x1<<pin);
	}
	else if(val == GPIO_FALLING_TRIGER)
	{
		Pointer->EPIER1 |= (0x1<<pin);
	}
	else
	{
		Pointer->EPIER|= (0x1<<pin);
	}
#endif
}


__STATIC_INLINE void GPIO_DisableIrq (uint32_t port, uint32_t pin) 
{
	EPORT_Type *Pointer=port_array[port];//(EPORT_Type *)(port*0x10000+EPORT_BASE);
	
	Pointer->EPIER&= ~(0x1<<pin);  //open poer interrupt enable
#if defined(ZB205_CHIP)
	Pointer->EPIER1 &= ~(0x1<<pin);
#endif
}

__STATIC_INLINE void GPIO_Clear_Irq (uint32_t port, uint32_t pin) 
{
	EPORT_Type *Pointer = port_array[port];
	Pointer->EPLFR = (0x1 << pin);
	Pointer->EPFR = (0x1 << pin);
}



__STATIC_INLINE int GPIO_Port_Irq_Status (uint32_t port) 
{
	EPORT_Type *Pointer = port_array[port];
	uint32_t irqMask=0;
	uint32_t assignment = Pointer->EPPAR;
	uint32_t EPLFR=Pointer->EPLFR;
	uint32_t EPFR=Pointer->EPFR;
#if defined(ZB204_CHIP)
	irqMask=EPLFR|EPFR;
	return irqMask & Pointer->EPIER;
#elif defined(ZB205_CHIP)
	uint32_t EPIER = Pointer->EPIER;
	uint32_t EPIER1 = Pointer->EPIER1;
	irqMask = (EPIER&EPFR)|(EPIER1&EPLFR);
	return irqMask;
#endif
}

#ifdef __cplusplus
}
#endif
#endif


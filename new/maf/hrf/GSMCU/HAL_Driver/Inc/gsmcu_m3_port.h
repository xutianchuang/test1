/****************************************************************
 *
 * port configure
 * @file gsmcu_m3_port.h
 * @author sitieqiang 
 * @version 0.1 
 * @date 2016/10/20 11:31:10
 * @note 
 *
*****************************************************************/
#ifndef __M3_PORT_H
#define __M3_PORT_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wpadded"
#endif
	
typedef enum{
	PORT_FUNC1,//ÎÞÐ§
	PORT_FAST_RATE=0x1,
	PORT_HIGH_STRENGTH=0x2,
	PORT_PULLUP=0x8,
	PORT_PULLDOWN=0x4,
	PORT_OPEN_DRAIN=0x10,
	PORT_FUNC2=0x20,
	PORT_FUNC_GPIO=0x40UL,
	PORT_FUNC_ANALOG=0x60UL,
}PORT_FUNC;	
// Pin identifier
typedef struct _PIN_ID {
  uint8_t       port;
  uint8_t       num;
  PORT_FUNC     config_val;
} PIN_ID;
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic pop
#endif
#define PORT_CFG_MODE(cfg)		((cfg&0x3)<<5) 


/**
 * Set pin function and electrical characteristics
 * 
 * @author sitieqiang
 * @param port    Port number (0..8)
 * @param pin     Pin number (0..7)
 * @param pin_cfg pin_cfg configuration bit mask
 * 
 * @return - \b  0: function succeeded
 *         - \b -1: function failed
 * @since 2016/10/20
 */
extern int32_t PORT_PinConfigure (uint8_t port, uint8_t pin, PORT_FUNC pin_cfg);





#ifdef __cplusplus
}
#endif
#endif 


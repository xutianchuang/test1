/****************************************************************
 *
 * port configure
 * @file gsmcu_m3_port.c
 * @author sitieqiang 
 * @version v0.1
 * @date 2016/10/20 11:29:59
 * @note 
 *
*****************************************************************/

#include "gsmcu_m3_port.h"
#include "RTE_Device.h" 
#include "GSMCU_M3.h"
#define PORT_OFFSET 0x8
#define PORTx(port, pin) (port*PORT_OFFSET+pin)

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
int32_t PORT_PinConfigure (uint8_t port, uint8_t pin, PORT_FUNC pin_cfg)
{
	if ((port>9)||(pin>15))
	{
		return -1;
	}
	PORTS->PORT_CR[port*8+pin]=pin_cfg;
	return 0;
}


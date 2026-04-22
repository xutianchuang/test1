#include <gsmcuxx_hal_def.h>
#include  <gsmcu_hal.h>
/**
  * @brief  
  * @param  
  * @retval 
  */
void SCU_SetUsartMode(USART_TypeDef * Usartx ,uint32_t mode)
{
    /* Check the parameters */
    assert_param(IS_USART(Usartx));
    assert_param(IS_USART_MODE(mode));
    /* config */
    uint32_t invert38k,uartmod,uartdcsr,opendrain;
    
	opendrain = (mode>>9)&0x01;
    invert38k = (mode>>8)&0x01;
    uartmod = (mode>>4)&0x01;
    uartdcsr = (mode>>0)&0x07;
   
    
    if(Usartx == UART0)
    {
        SCU->EX_PERIPH_DRIVER_USART0_MODE = ((SCU->EX_PERIPH_DRIVER_USART0_MODE)&(~((0x03UL<<30)|(0x07UL<<21))))
                                            |(invert38k<<31)
                                            |(uartmod << 30)
                                            |(uartdcsr<<21);
        SCU->EX_USART_MODE = (SCU->EX_USART_MODE & (~(1<<12))) | (opendrain << 12);
    
    }
    else if(Usartx == UART1)
    {
        SCU->EX_PERIPH_DRIVER_USART0_MODE = ((SCU->EX_PERIPH_DRIVER_USART0_MODE)&(~(0x07UL<<24)))
                                            |(uartdcsr << 24);
        
        SCU->EX_USART_MODE = ((SCU->EX_USART_MODE)&(~(0x03UL<<6)))
                            |(invert38k<<7)
                            |(uartmod << 6);
		SCU->EX_USART_MODE = (SCU->EX_USART_MODE & (~(1<<13))) | (opendrain << 13);
        
    }
    else if(Usartx == UART2)
    {
        SCU->EX_PERIPH_DRIVER_USART0_MODE = ((SCU->EX_PERIPH_DRIVER_USART0_MODE)&(~(0x07UL<<27)))
                                            |(uartdcsr << 27);
        
        SCU->EX_USART_MODE = ((SCU->EX_USART_MODE)&(~(0x03UL<<8)))
                            |(invert38k<<9)
                            |(uartmod << 8);
		SCU->EX_USART_MODE = (SCU->EX_USART_MODE & (~(1<<14))) | (opendrain << 14);
        
    }
    else //if(Usartx == ((uint32_t)(UART0)))
    {
        SCU->EX_USART_MODE = ((SCU->EX_USART_MODE)&(~(0x1FUL<<1)))
                            |(invert38k<<5)
                            |(uartmod << 4)
                            |(uartdcsr << 1);
		SCU->EX_USART_MODE = (SCU->EX_USART_MODE & (~(1<<15))) | (opendrain << 15);
    }
}

/**
  * @brief  
  * @param  
  * @retval 
  */
void SCU_PeriphInputConfig(uint32_t perinput)
{
    /* Check the parameters */
    assert_param(IS_PERIPH_INPUT_ENABLE(perinput));
    SCU->EX_PERIPH_INPUT_ENABLE |=  perinput;
}

void SCU_GPIOInputConfig(GPIO_TypeDef * port, uint32_t pin)
{
#if ZB202
	if(port == GPIO0)
	{
    	SCU->EX_GPIO0_INPUT_ENABLE |=  pin;
	}
	else if(port == GPIO1)
	{
		SCU->EX_GPIO1_INPUT_ENABLE |=  pin;
	}
#elif DM750
	return;
#endif
}

void SCU_DisableALLGPIOInput()
{
#if ZB202
	SCU->EX_GPIO0_INPUT_ENABLE = ~GPIO_Pin_All | 
								GPIO_Pin_9 | GPIO_Pin_10 | //SWD
								GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13;//SPIFLASH

	SCU->EX_GPIO1_INPUT_ENABLE = ~GPIO_Pin_All | 
								GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;//SPIFLASH
#elif DM750
	return;
#endif
}

/**
  * @brief  
  * @param  
  * @retval 
  */
void PinRemapConfig(uint32_t Pin_Remap, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_PIN_REMAP(Pin_Remap));
    assert_param(IS_FUNCTIONAL_STATE(NewState));  
    //remap pin
#if ZB202
    uint32_t pos = Pin_Remap&0x1f;
    uint32_t nstate = (NewState != DISABLE)?0:0x01;
    nstate = nstate << pos;
	uint32_t vmask = 0x1UL<<pos;
    
    if(Pin_Remap < 32)
    {
        SCU->EX_PERIPH_PIN_REMAP0 = (SCU->EX_PERIPH_PIN_REMAP0 & (~vmask))| nstate;
    }
    else
    {
        SCU->EX_PERIPH_PIN_REMAP1 = (SCU->EX_PERIPH_PIN_REMAP1 & (~vmask)) | nstate;
    }
#elif DM750
    uint32_t pos = ((Pin_Remap)&0xFF)<<1;  //22
    uint32_t vmask = 0x03UL<<pos;   //22
    uint32_t nstate = (NewState != DISABLE)?0:0x01;
    nstate = nstate << pos;
    
    if(((Pin_Remap>>8)&0xFF) == 0x00)
    {
        SCU->EX_PERIPH_PIN_REMAP0 = ((SCU->EX_PERIPH_PIN_REMAP0)&(~vmask))|nstate;
    }
    else
    {
        SCU->EX_PERIPH_PIN_REMAP1 = ((SCU->EX_PERIPH_PIN_REMAP1)&(~vmask))|nstate;
    }	
#endif
}

















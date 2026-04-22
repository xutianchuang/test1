
#include  <gsmcu_hal.h>


/** @defgroup GPIO_Exported_Functions
  * @{
  */
void GPIO_GlobalInit()
{
    //enable gpio input 
    //SCU->EX_GPIO_DRIVER_OTHER &= ~(0x07<<22);
    //SCU->EX_GPIO_DRIVER_OTHER |= (0x01<<25);
    //SCU->EX_PERIPH_INPUT_ENABLE |= (0x01<<14);
#if ZB202
    SCU->EX_PERIPH_INPUT_ENABLE |= 0xCFFF;
#elif DM750
    SCU->EX_PERIPH_INPUT_ENABLE |= 0xCBFF;
#endif
}


/** @defgroup GPIO_Exported_Functions
  * @{
  */
void GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct)
{
  uint32_t currentdir, currentpullen, currentpulltype,pos,curpass;
  uint32_t setdir, setpullen, setpulltype;
  
  uint8_t pinpos;
  /* Check the parameters */
  assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
  assert_param(IS_GPIO_MODE(GPIO_InitStruct->GPIO_Mode));
  assert_param(IS_GPIO_PIN(GPIO_InitStruct->GPIO_Pin));
  
  if ((uint32_t)GPIO_InitStruct->GPIO_Pin  != 0x00)
  {
/*---------------------------- GPIO Mode Configuration -----------------------*/
      currentdir = GPIOx->PINDIR;
      currentpullen = GPIOx->PINPULL;
      currentpulltype = GPIOx->PINPULL_TYPE;
      curpass = GPIOx->PINBYPASS;
/*---------------------------- GPIO CRL Configuration ------------------------*/
      /* Configure port pins */
      setdir = (((uint32_t)GPIO_InitStruct->GPIO_Mode)>>2)&0x01;
      setpullen = (((uint32_t)GPIO_InitStruct->GPIO_Mode)>>1)&0x01;
      setpulltype = (((uint32_t)GPIO_InitStruct->GPIO_Mode)>>0)&0x01;   
    for (pinpos = 0x00; pinpos < 32; pinpos++)
    {
      pos = ((uint32_t)0x01) << pinpos;
      /* Get the port pins position */
      if (((GPIO_InitStruct->GPIO_Pin) & pos) == pos)
      {
        currentdir &= ~pos;
        currentpullen &= ~pos;
        currentpulltype &= ~pos;
        curpass &= ~pos;
        //===
        currentdir |= setdir<<pinpos;
        currentpullen |= setpullen<<pinpos;
        currentpulltype |= setpulltype<<pinpos;
      }
    }
    GPIOx->PINDIR = currentdir;
    GPIOx->PINPULL = currentpullen;
    GPIOx->PINPULL_TYPE = currentpulltype;
    GPIOx->PINBYPASS = curpass;
  }
}


void GPIO_StructInit(GPIO_InitTypeDef* GPIO_InitStruct)
{
  /* Reset GPIO init structure parameters values */
  GPIO_InitStruct->GPIO_Pin  = GPIO_Pin_All;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IN_FLOATING;
}

uint8_t GPIO_ReadInputDataBit(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin)
{
  uint8_t bitstatus = 0x00;
  
  /* Check the parameters */
  assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
  assert_param(IS_GET_GPIO_PIN(GPIO_Pin)); 
  
  if ((GPIOx->DATAIN & GPIO_Pin) != (uint32_t)Bit_RESET)
  {
    bitstatus = (uint8_t)Bit_SET;
  }
  else
  {
    bitstatus = (uint8_t)Bit_RESET;
  }
  return bitstatus;
}

uint32_t GPIO_ReadInputData(GPIO_TypeDef* GPIOx)
{
    /* Check the parameters */
    assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
    return GPIOx->DATAIN;
}
uint8_t GPIO_ReadOutputDataBit(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin)
{
  uint8_t bitstatus = 0x00;
  
  /* Check the parameters */
  assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
  assert_param(IS_GET_GPIO_PIN(GPIO_Pin)); 
  
  if ((GPIOx->DATAOUT & GPIO_Pin) != (uint32_t)Bit_RESET)
  {
    bitstatus = (uint8_t)Bit_SET;
  }
  else
  {
    bitstatus = (uint8_t)Bit_RESET;
  }
  return bitstatus;
}
uint32_t GPIO_ReadOutputData(GPIO_TypeDef* GPIOx)
{
    /* Check the parameters */
    assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
    return GPIOx->DATAOUT;
}

void GPIO_SetBits(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin)
{
  /* Check the parameters */
  assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
  assert_param(IS_GET_GPIO_PIN(GPIO_Pin)); 
  GPIOx->DATASET = GPIO_Pin;
}
void GPIO_ResetBits(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin)
{
  /* Check the parameters */
  assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
  assert_param(IS_GET_GPIO_PIN(GPIO_Pin)); 
  GPIOx->DATACLEAR = GPIO_Pin;
}

void GPIO_Write(GPIO_TypeDef* GPIOx, uint32_t PortVal)
{
    /* Check the parameters */
    assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
    GPIOx->DATAOUT = PortVal;
}

//设置GPIO引脚对应中断开关
void GPIO_SetIRQMode(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin, uint8_t mode)
{

	
    assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
    assert_param(IS_GET_GPIO_PIN(GPIO_Pin)); 

	GPIOx->ICR |= GPIO_Pin;
	if (mode == ENABLE)
	{
		GPIOx->INTRENABLE |=  GPIO_Pin;
	}
	else
	{
		GPIOx->INTRENABLE &= ~GPIO_Pin;
	}
}

//设置GPIO引脚对应中断触发模式
void GPIO_SetIRQTriggerMode(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin, uint32_t mode)
{
    uint32_t pinpos = 0, pos = 0;
	
    assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
    assert_param(IS_GET_GPIO_PIN(GPIO_Pin)); 

    for(pinpos=0; pinpos<32; pinpos++)
    {
        pos = 1 << pinpos;
        if((GPIO_Pin & pos) == pos)
        {
            GPIOx->ITR &= ~(1<<pinpos);
            GPIOx->IBETR &= ~(1<<pinpos);
            GPIOx->IRONETR &= ~(1<<pinpos);
			GPIOx->ITR |= ((mode>>16)&0x01)<<pinpos; 
			GPIOx->IBETR |= ((mode>>8)&0x01)<<pinpos; 
			GPIOx->IRONETR |= ((mode>>0)&0x01)<<pinpos; 
        }
	}
}

//清除GPIO引脚对应中断状态
void GPIO_ClearIRQStatus(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin)
{
    uint32_t pinpos = 0, pos = 0;
	
    assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
    assert_param(IS_GET_GPIO_PIN(GPIO_Pin)); 

	for(pinpos=0; pinpos<32; pinpos++)
    {
        pos = 1 << pinpos;
        if((GPIO_Pin & pos) == pos)
        {
            GPIOx->ICR |= 1<<pinpos;
        }
	}
}

//使能Bounce
void GPIO_EnableBounce(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin)
{
    uint32_t pinpos = 0, pos = 0;
	
    assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
    assert_param(IS_GET_GPIO_PIN(GPIO_Pin)); 

	for(pinpos=0; pinpos<32; pinpos++)
    {
        pos = 1 << pinpos;
        if((GPIO_Pin & pos) == pos)
        {
            GPIOx->BER |= 1<<pinpos;
        }
	}
}

//关闭Bounce
void GPIO_DisableBounce(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin)
{
    uint32_t pinpos = 0, pos = 0;
	
    assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
    assert_param(IS_GET_GPIO_PIN(GPIO_Pin)); 

	for(pinpos=0; pinpos<32; pinpos++)
    {
        pos = 1 << pinpos;
        if((GPIO_Pin & pos) == pos)
        {
            GPIOx->BER &= ~(1<<pinpos);
        }
	}
}

//设置Bounce Prescale
void GPIO_SetBouncePreScale(GPIO_TypeDef* GPIOx, uint32_t pre_scale)
{
	
    assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
	GPIOx->BCPR = pre_scale;

}


unsigned int GPIO_GetIntStatus(GPIO_TypeDef *GPIOx)
{
    assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
    return GPIOx->IRSR;
}

unsigned int GPIO_ClearPortIntStatus(GPIO_TypeDef *GPIOx, unsigned int pins)
{
    assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
    return GPIOx->ICR=pins;
}


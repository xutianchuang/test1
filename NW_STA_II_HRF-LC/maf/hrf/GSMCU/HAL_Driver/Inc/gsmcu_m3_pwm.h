#ifndef __M0_PWM_H
#define __M0_PWM_H
#include "GSMCU_M3.h"
#include "RTE_Components.h"
#ifdef __cplusplus
extern "C" {
#endif


  
/* ========================================  Start of section using anonymous unions  ======================================== */
#if defined (__CC_ARM)
  #pragma push
  #pragma anon_unions
#elif defined (__ICCARM__)
  #pragma language=extended
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wc11-extensions"
  #pragma clang diagnostic ignored "-Wreserved-id-macro"
  #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
  #pragma clang diagnostic ignored "-Wnested-anon-types"
  #pragma clang diagnostic ignored "-Wpadded"
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning 586
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Not supported compiler type
#endif





typedef struct{
  union {
    __IO uint32_t  PCNR;                           /*!< PWM Counter Register0                                                 */
    
    struct {
      __IO uint32_t  CNR        : 16;               /*!< PWM Counter/Timer Loaded Value                                        */
      __IO uint32_t             : 16;
    } PCNR_b;                                      /*!< BitSize                                                               */
  };
  
  union {
    __IO uint32_t  PCMR;                           /*!< PWM Comparator Register0                                              */
    
    struct {
      __IO uint32_t  CMR        : 16;               /*!< PWM Comparator Register                                               */
      __IO uint32_t             : 16;
    } PCMR_b;                                      /*!< BitSize                                                               */
  };
  
  union {
    __IO uint32_t  PTR;                            /*!< PWM Timer Register0                                                   */
    
    struct {
      __IO uint32_t  CTR        : 16;               /*!< PWM Timer value                                                       */
      __IO uint32_t             : 16;
    } PTR_b;                                       /*!< BitSize                                                               */
  };
}PWM_TIMER_CHANNEL_Type ;


typedef struct{
  union {
    __IO uint32_t  PCRLR;                          /*!< PWM Capture Rising Latch Register                                     */
    
    struct {
      __IO uint32_t  CRLR       : 16;               /*!< Capture Rising Latch Registerx                                        */
      __IO uint32_t             : 16;
    } PCRLR_b;                                     /*!< BitSize                                                               */
  };
  
  union {
    __IO uint32_t  PCFLR;                          /*!< PWM Capture Falling Latch Register                                    */
    
    struct {
      __IO uint32_t  CFLR       : 16;               /*!< Capture Falling Latch Registerx                                       */
      __IO uint32_t             : 16;
    } PCFLR_b;                                     /*!< BitSize                                                               */
  };
}PWM_CAPTURE_CHANNEL_Type ;

typedef union {
	__IO uint8_t  PCR;                           

	struct {
	  __IO uint8_t  CHEN       :  1;             
		   uint8_t             :  1;
	  __IO uint8_t  CHINV      :  1;             
	  __IO uint8_t  CHMOD      :  1;             
	  __IO uint8_t  DZEN0      :  1;             
	  __IO uint8_t  DZEN1      :  1;             
		   uint8_t             :  1;
	  __IO uint8_t  CH_HDU    :  1;             
	} PCR_b;                                     
}PWM_PCR_Type ;


typedef struct {
  uint8_t                 port;             // Port number
  uint8_t                 pin;              // Pin number
  uint8_t                 func;             // funtion for pin
} const PWM_PIN;
typedef struct{
	uint8_t						flags;
	uint8_t						pwm_need_int;
	uint8_t						capture_need_int;
	uint32_t					speed[4];
}PWM_INFO;


typedef struct{
	PWM_Type					*reg;
	IRQn_Type					irqn;
	uint32_t					*clk;	
	const	PWM_PIN						*pin;
	PWM_INFO					*info;

}const PWM_RESOURCES;
/* =========================================  End of section using anonymous unions  ========================================= */
#if defined (__CC_ARM)
  #pragma pop
#elif defined (__ICCARM__)
  /* leave anonymous unions enabled */
#elif (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic pop
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning restore
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#endif






typedef struct _PWM_DRIVER {
	int32_t       (*Initialize)                     (uint32_t prescale0,uint32_t prescale1);
	int32_t       (*UnInitialize)                   (void);
	int32_t       (*Control)                        (uint32_t control, uint32_t arg);
	int32_t       (*TimerStart)                     (uint32_t channel, uint32_t valve);
	int32_t       (*TimerStop)                      (uint32_t channel);
	int32_t       (*CaptureStart)                   (uint32_t channel, uint32_t latch);
	int32_t       (*CaptureStop)                    (uint32_t channel);                                                               ///< Pointer to \ref ARM_I2C_GetStatus : Get I2C status.
} PWM_DRIVER;



#define PWM0_TIMER_CHANNEL                             ((PWM_TIMER_CHANNEL_Type                *) (PWM_BASE+0x0c))
#define PWM0_CAPTURE_CHANNEL                           ((PWM_CAPTURE_CHANNEL_Type              *) (PWM_BASE+0x4c))

#define PWM1_TIMER_CHANNEL                             ((PWM_TIMER_CHANNEL_Type                *) (PWM1_BASE+0x0c))
#define PWM1_CAPTURE_CHANNEL                           ((PWM_CAPTURE_CHANNEL_Type              *) (PWM1_BASE+0x4c))




#define PWM_PRESCALE(timer01,timer23)					( ((timer01)&0xff) | ((timer23&0xff)<<8) )



/*  	PWM Code	*/
#define PWM_CHANNEL_Pos				24U
#define PWM_CHANNEL_Msk				(0x3U<<PWM_CHANNEL_Pos)
#define	PWM_CHANNEL0				(0x0U<<PWM_CHANNEL_Pos)
#define	PWM_CHANNEL1				(0x1U<<PWM_CHANNEL_Pos)
#define	PWM_CHANNEL2				(0x2U<<PWM_CHANNEL_Pos)
#define	PWM_CHANNEL3				(0x3U<<PWM_CHANNEL_Pos)


#define PWM_CLOCK_SEL_Pos			0U
#define PWM_CLOCK_SEL_Msk			(0xfU<<PWM_CLOCK_SEL_Pos)					
#define PWM_CLOCK_DEV1				(0x4U|0x8U)//CSR is 3bit  4th bit is flags
#define PWM_CLOCK_DEV16				(0x3U|0x8U)//CSR is 3bit  4th bit is flags
#define PWM_CLOCK_DEV8				(0x2U|0x8U)//CSR is 3bit  4th bit is flags
#define PWM_CLOCK_DEV4				(0x1U|0x8U)//CSR is 3bit  4th bit is flags
#define PWM_CLOCK_DEV2				(0x0U|0x8U)//CSR is 3bit  4th bit is flags
											 //


#define PWM_DEADZONE_Pos				4U						//arg is DZEN timer
#define PWM_DEADZONE_Msk				(0x3U<<PWM_DEADZONE_Pos)
#define PWM_DZEN0						(0x1U<<PWM_DEADZONE_Pos)
#define PWM_DZEN1						(0x2U<<PWM_DEADZONE_Pos)
#define PWM_DZEN_ALL					(0x3U<<PWM_DEADZONE_Pos)

#define PWM_HALF_CYCLE_Pos				7U						//	Timer Half cycle add 
#define PWM_HALF_CYCLE_Msk				(1U<<PWM_HALF_CYCLE_Pos)

#define PWM_CH_MODE_Pos					9U
#define PWM_CH_MODE_Msk					(3U<<PWM_CH_MODE_Pos)
#define PWM_CH_MODE_AUTO					(1U<<PWM_CH_MODE_Pos)
#define PWM_CH_MODE_ONE					(2U<<PWM_CH_MODE_Pos)

#define PWM_INVERTER_Pos				11U
#define PWM_INVERTER_Msk				(0x3U<<PWM_INVERTER_Pos)
#define PWM_INVERTER_HIGH				(1U<<PWM_INVERTER_Pos)			//CMR vale is high timer
#define PWM_INVERTER_LOW				(2U<<PWM_INVERTER_Pos)			//CMR vale is low timer
																		
#define PWM_SET_BASE_VAL_Pos			13U
#define PWM_SET_BASE_VAL_Msk			(1U<<PWM_SET_BASE_VAL_Pos)		//arg is CNR value ,mean PWM Counter/Timer Loaded Value



#define PWM_PINDDR_Pos				15U
#define PWM_PINDDR_Msk				(0x3U<<PWM_PINDDR_Pos)
#define PWM_PINDDR_OUT			(1U<<PWM_PINDDR_Pos)			//PIN OUT 
#define PWM_PINDDR_IN				(2U<<PWM_PINDDR_Pos)			//PIN IN






#define PWM_CAPTURE_FALLING				0x1U
#define PWM_CAPTURE_RISING				(0x1U<<1)

#if RTE_PWM0
extern const PWM_DRIVER Driver_PWM0;
#endif
#if RTE_PWM1
extern const PWM_DRIVER Driver_PWM1;
#endif





#ifdef __cplusplus
}
#endif
#endif


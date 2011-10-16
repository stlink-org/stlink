/* this example is only for stm32l discover.
   adapted from ST firmwares projects.
 */

/* base headers */
#include "stdint.h"

/* libstm32l_discovery headers */
#include "stm32l1xx_gpio.h"
#include "stm32l1xx_adc.h"
#include "stm32l1xx_lcd.h"
#include "stm32l1xx_rcc.h"
#include "stm32l1xx_rtc.h"
#include "stm32l1xx_exti.h"
#include "stm32l1xx_pwr.h"
#include "stm32l1xx_syscfg.h"
#include "stm32l1xx_dbgmcu.h"

/* lcd wrapper routines header */
#include "stm32l_discovery_lcd.h"

/* boot mode */

#define CONFIG_BOOT_SRAM 1
#define CONFIG_BOOT_FLASH 0


/* gpios
   refer to CD00277537.pdf, APB memory space.
   refer to CD00240193.pdf, GPIO.
*/

#define GPIOA_MODER (GPIOA_BASE + 0x00)
#define GPIOA_ODR (GPIOA_BASE + 0x14)

#define GPIOB_MODER (GPIOB_BASE + 0x00)
#define GPIOB_ODR (GPIOB_BASE + 0x14)

#define GPIOC_MODER (GPIOC_BASE + 0x00)
#define GPIOC_ODR (GPIOC_BASE + 0x14)


/* leds */

#define LED_BLUE (1 << 6) /* port B, pin 6 */
#define LED_GREEN (1 << 7) /* port B, pin 7 */

static inline void setup_leds(void)
{
  /* configure port 6 and 7 as output */
  *(volatile uint32_t*)GPIOB_MODER |= (1 << (7 * 2)) | (1 << (6 * 2));
}

static inline void switch_leds_on(void)
{
  *(volatile uint32_t*)GPIOB_ODR = LED_BLUE | LED_GREEN;
}

static inline void switch_leds_off(void)
{
  *(volatile uint32_t*)GPIOB_ODR = 0;
}


#define delay()						\
do {							\
  register unsigned int i;				\
  for (i = 0; i < 1000000; ++i)				\
    __asm__ __volatile__ ("nop\n\t":::"memory");	\
} while (0)


#if CONFIG_BOOT_SRAM

extern uint32_t _fstack;

static inline void setup_stack(void)
{
  /* setup the stack to point to _fstack (refer to ld script) */

  static const uint32_t fstack = (uint32_t)&_fstack;

  __asm__ __volatile__
    (
     "ldr sp, %0\n\t"
     : 
     : "m"(fstack)
     : "sp"
    );
}

#endif /* CONFIG_BOOT_SRAM */


/* application related setup */

static void RCC_Configuration(void)
{  
  /* Enable HSI Clock */
  RCC_HSICmd(ENABLE);
  
  /*!< Wait till HSI is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);

  /* Set HSI as sys clock*/
  RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
  
  /* Set MSI clock range to ~4.194MHz*/
  RCC_MSIRangeConfig(RCC_MSIRange_6);
  
  /* Enable the GPIOs clocks */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC| RCC_AHBPeriph_GPIOD| RCC_AHBPeriph_GPIOE| RCC_AHBPeriph_GPIOH, ENABLE);     

  /* Enable comparator, LCD and PWR mngt clocks */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_COMP | RCC_APB1Periph_LCD | RCC_APB1Periph_PWR,ENABLE);
    
  /* Enable ADC & SYSCFG clocks */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_SYSCFG , ENABLE);

  /* Allow access to the RTC */
  PWR_RTCAccessCmd(ENABLE);

  /* Reset RTC Backup Domain */
  RCC_RTCResetCmd(ENABLE);
  RCC_RTCResetCmd(DISABLE);

  /* LSE Enable */
  RCC_LSEConfig(RCC_LSE_ON);

  /* Wait until LSE is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
  
   /* RTC Clock Source Selection */ 
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE); 
  
  /* Enable the RTC */
  RCC_RTCCLKCmd(ENABLE);   
  
  /*Disable HSE*/
  RCC_HSEConfig(RCC_HSE_OFF);
  if(RCC_GetFlagStatus(RCC_FLAG_HSERDY) != RESET )
  {
    /* Stay in infinite loop if HSE is not disabled*/
    while(1); 
  }
}

static void Init_GPIOs(void)
{
#if 0 /* fixme: GPIO_Init raises a bug in some gcc toolchains */

  /* GPIO, EXTI and NVIC Init structure declaration */
  GPIO_InitTypeDef GPIO_InitStructure;

#if 0
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
#endif

#if 0
  /* Configure User Button pin as input */
  GPIO_InitStructure.GPIO_Pin = USERBUTTON_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
  GPIO_Init(USERBUTTON_GPIO_PORT, &GPIO_InitStructure);
#endif

#if 0
  /* Select User Button pin as input source for EXTI Line */
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA,EXTI_PinSource0);

  /* Configure EXT1 Line 0 in interrupt mode trigged on Rising edge */
  EXTI_InitStructure.EXTI_Line = EXTI_Line0 ;  // PA0 for User button AND IDD_WakeUP
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* Enable and set EXTI0 Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn ;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure); 
#endif

#if 0
  /* Configure the LED_pin as output push-pull for LD3 & LD4 usage*/
  GPIO_InitStructure.GPIO_Pin = LD_GREEN_GPIO_PIN | LD_BLUE_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(LD_GPIO_PORT, &GPIO_InitStructure);
  
  /* Force a low level on LEDs*/ 
  GPIO_LOW(LD_GPIO_PORT,LD_GREEN_GPIO_PIN);	
  GPIO_LOW(LD_GPIO_PORT,LD_BLUE_GPIO_PIN);
    
  /* Counter enable: GPIO set in output for enable the counter */
  GPIO_InitStructure.GPIO_Pin = CTN_CNTEN_GPIO_PIN;
  GPIO_Init( CTN_GPIO_PORT, &GPIO_InitStructure);
  
  /* To prepare to start counter */
  GPIO_HIGH(CTN_GPIO_PORT,CTN_CNTEN_GPIO_PIN);
      
  /* Configure Port A LCD Output pins as alternate function */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_8 | GPIO_Pin_9 |GPIO_Pin_10 |GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init( GPIOA, &GPIO_InitStructure);
  
  /* Select LCD alternate function for Port A LCD Output pins */
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource1,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource2,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource3,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource8,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource9,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource10,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource15,GPIO_AF_LCD) ;  
  
  /* Configure Port B LCD Output pins as alternate function */ 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_9 \
                                 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init( GPIOB, &GPIO_InitStructure);
  
  /* Select LCD alternate function for Port B LCD Output pins */
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource3,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource4,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource5,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource8,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource9,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource10,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource11,GPIO_AF_LCD) ;  
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource12,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource13,GPIO_AF_LCD) ;   
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource14,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource15,GPIO_AF_LCD) ;   
#endif

#if 0  
  /* Configure Port C LCD Output pins as alternate function */ 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_6 \
                                 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 |GPIO_Pin_11 ;                               
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init( GPIOC, &GPIO_InitStructure);  

  /* Select LCD alternate function for Port B LCD Output pins */
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource0,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource1,GPIO_AF_LCD) ; 
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource2,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource3,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource6,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource7,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource8,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource9,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource10,GPIO_AF_LCD) ; 
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource11,GPIO_AF_LCD) ;  
#endif
  
#if 0
  /* Configure ADC (IDD_MEASURE) pin as Analogue */
  GPIO_InitStructure.GPIO_Pin = IDD_MEASURE  ;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_Init( IDD_MEASURE_PORT, &GPIO_InitStructure);
#endif

#else /* fixme */

  /* set every port in digital output mode  */

  /* PA[1:3,8:10,15] */
  *(volatile uint32_t*)GPIOA_MODER |=
    (GPIO_Mode_AF << (1 * 2)) |
    (GPIO_Mode_AF << (2 * 2)) |
    (GPIO_Mode_AF << (3 * 2)) |
    (GPIO_Mode_AF << (8 * 2)) |
    (GPIO_Mode_AF << (9 * 2)) |
    (GPIO_Mode_AF << (10 * 2)) |
    (GPIO_Mode_AF << (15 * 2));

  /* PB[3:5,8:15] */
  *(volatile uint32_t*)GPIOB_MODER |=
    (GPIO_Mode_AF << (3 * 2)) |
    (GPIO_Mode_AF << (4 * 2)) |
    (GPIO_Mode_AF << (5 * 2)) |
    (GPIO_Mode_AF << (8 * 2)) |
    (GPIO_Mode_AF << (9 * 2)) |
    (GPIO_Mode_AF << (10 * 2)) |
    (GPIO_Mode_AF << (11 * 2)) |
    (GPIO_Mode_AF << (12 * 2)) |
    (GPIO_Mode_AF << (13 * 2)) |
    (GPIO_Mode_AF << (14 * 2)) |
    (GPIO_Mode_AF << (15 * 2));

  /* PC[0:3,6:11] */
  *(volatile uint32_t*)GPIOC_MODER |=
    (GPIO_Mode_AF << (0 * 2)) |
    (GPIO_Mode_AF << (1 * 2)) |
    (GPIO_Mode_AF << (2 * 2)) |
    (GPIO_Mode_AF << (3 * 2)) |
    (GPIO_Mode_AF << (6 * 2)) |
    (GPIO_Mode_AF << (7 * 2)) |
    (GPIO_Mode_AF << (8 * 2)) |
    (GPIO_Mode_AF << (9 * 2)) |
    (GPIO_Mode_AF << (10 * 2)) |
    (GPIO_Mode_AF << (11 * 2));

#endif /* fixme */
}


/* main */

static void __attribute__((naked)) __attribute__((used)) main(void)
{
#if CONFIG_BOOT_SRAM
  /* do not use previsouly setup stack, if any */
  setup_stack();
#endif /* CONFIG_BOOT_SRAM */

  RCC_Configuration();

  Init_GPIOs();

  LCD_GLASS_Init();

  setup_leds();

  while (1)
  {
    /* switch_leds_on(); */
    GPIO_HIGH(LD_GPIO_PORT, LD_GREEN_GPIO_PIN);	
    GPIO_HIGH(LD_GPIO_PORT, LD_BLUE_GPIO_PIN);

    LCD_GLASS_Clear();
    LCD_GLASS_DisplayString("ON   ");

    delay();

    /* switch_leds_off(); */
    GPIO_LOW(LD_GPIO_PORT, LD_GREEN_GPIO_PIN);	
    GPIO_LOW(LD_GPIO_PORT, LD_BLUE_GPIO_PIN);	

    LCD_GLASS_Clear();
    LCD_GLASS_DisplayString("  OFF");

    delay();
  }
}

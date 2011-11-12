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
#include "stm32l1xx_flash.h"
#include "stm32l1xx_syscfg.h"
#include "stm32l1xx_dbgmcu.h"

/* board specific macros */
#include "discover_board.h"


/* hardware configuration */

#if CONFIG_STM32VL_DISCOVERY

# define GPIOC 0x40011000 /* port C */
# define GPIOC_CRH (GPIOC + 0x04) /* port configuration register high */
# define GPIOC_ODR (GPIOC + 0x0c) /* port output data register */

# define LED_BLUE (1 << 8) /* port C, pin 8 */
# define LED_GREEN (1 << 9) /* port C, pin 9 */

static inline void setup_leds(void)
{
  *(volatile uint32_t*)GPIOC_CRH = 0x44444411;
}

static inline void switch_leds_on(void)
{
  *(volatile uint32_t*)GPIOC_ODR = LED_BLUE | LED_GREEN;
}

static inline void switch_leds_off(void)
{
  *(volatile uint32_t*)GPIOC_ODR = 0;
}

#elif CONFIG_STM32L_DISCOVERY

# define GPIOB_MODER (GPIOB + 0x00) /* port mode register */
# define GPIOB_ODR (GPIOB + 0x14) /* port output data register */

# define LED_BLUE (1 << 6) /* port B, pin 6 */
# define LED_GREEN (1 << 7) /* port B, pin 7 */

static inline void setup_leds(void)
{
  /* configure port 6 and 7 as output */
  *(volatile uint32_t*)GPIOB_MODER |= (1 << (7 * 2)) | (1 << (6 * 2));
}

static inline void switch_leds_on(void)
{
  GPIO_HIGH(LD_GPIO_PORT, LD_GREEN_GPIO_PIN);	
  GPIO_HIGH(LD_GPIO_PORT, LD_BLUE_GPIO_PIN);
}

static inline void switch_leds_off(void)
{
  GPIO_LOW(LD_GPIO_PORT, LD_GREEN_GPIO_PIN);	
  GPIO_LOW(LD_GPIO_PORT, LD_BLUE_GPIO_PIN);
}

#elif CONFIG_STM32F4_DISCOVERY

//#define GPIOD 0x40020C00 /* port D */
# define GPIOD_MODER (GPIOD + 0x00) /* port mode register */
# define GPIOD_ODR (GPIOD + 0x14) /* port output data register */

# define LED_GREEN (1 << 12) /* port B, pin 12 */
# define LED_ORANGE (1 << 13) /* port B, pin 13 */
# define LED_RED (1 << 14) /* port B, pin 14 */
# define LED_BLUE (1 << 15) /* port B, pin 15 */

void _tmain(void) {
	main();
}
static inline void setup_leds(void)
{
  *(volatile uint32_t*)GPIOD_MODER |= (1 << (12 * 2)) | (1 << (13 * 2)) |
  	(1 << (13 * 2)) | (1 << (14 * 2)) | (1 << (15 * 2));
}


static inline void switch_leds_on(void)
{
  *(volatile uint32_t*)GPIOD_ODR = LED_GREEN | LED_RED ;
}

static inline void switch_leds_off(void)
{
  *(volatile uint32_t*)GPIOD_ODR = 0;
}

#endif /* otherwise, error */


#define delay()						\
do {							\
  volatile unsigned int i;				\
  for (i = 0; i < 1000000; ++i)				\
    __asm__ __volatile__ ("nop\n\t":::"memory");	\
} while (0)


static void RCC_Configuration(void)
{  
  /* Enable HSI Clock */
  RCC_HSICmd(ENABLE);
  
  /*!< Wait till HSI is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET)
  {}

  RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
  
  RCC_MSIRangeConfig(RCC_MSIRange_6);

  RCC_HSEConfig(RCC_HSE_OFF);  
  if(RCC_GetFlagStatus(RCC_FLAG_HSERDY) != RESET )
  {
    while(1);
  }
}


static void RTC_Configuration(void)
{
  /* Allow access to the RTC */
  PWR_RTCAccessCmd(ENABLE);

  /* Reset Backup Domain */
  RCC_RTCResetCmd(ENABLE);
  RCC_RTCResetCmd(DISABLE);

  /* LSE Enable */
  RCC_LSEConfig(RCC_LSE_ON);

  /* Wait till LSE is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
  {}
  
  RCC_RTCCLKCmd(ENABLE);
   
  /* LCD Clock Source Selection */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

}

void main(void)
{
  static RCC_ClocksTypeDef RCC_Clocks;
  static GPIO_InitTypeDef GPIO_InitStructure;

  /* Configure Clocks for Application need */
  RCC_Configuration();
  
  /* Configure RTC Clocks */
  RTC_Configuration();

  /* Set internal voltage regulator to 1.8V */
  PWR_VoltageScalingConfig(PWR_VoltageScaling_Range1);

  /* Wait Until the Voltage Regulator is ready */
  while (PWR_GetFlagStatus(PWR_FLAG_VOS) != RESET) ;

  /* configure gpios */

  /* Enable GPIOs clock */ 	
  RCC_AHBPeriphClockCmd(LD_GPIO_PORT_CLK, ENABLE);

  /* Configure the GPIO_LED pins  LD3 & LD4*/
  GPIO_InitStructure.GPIO_Pin = LD_GREEN_GPIO_PIN | LD_BLUE_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(LD_GPIO_PORT, &GPIO_InitStructure);
  GPIO_LOW(LD_GPIO_PORT, LD_GREEN_GPIO_PIN);	
  GPIO_LOW(LD_GPIO_PORT, LD_BLUE_GPIO_PIN);

  while (1)
  {
    switch_leds_on();
    delay();
    switch_leds_off();
    delay();
  }
}

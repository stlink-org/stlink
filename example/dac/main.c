/* base headers */
#include "stdint.h"

/* libstm32l_discovery headers */
#include "stm32l1xx_gpio.h"
#include "stm32l1xx_adc.h"
#include "stm32l1xx_dac.h"
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

#endif /* otherwise, error */


#define delay()						\
do {							\
  volatile unsigned int i;				\
  for (i = 0; i < 1000000; ++i)				\
    __asm__ __volatile__ ("nop\n\t":::"memory");	\
} while (0)


static void RCC_Configuration(void)
{
  /* HSI is 16mhz RC clock directly fed to SYSCLK (rm00038, figure 9) */

  /* enable the HSI clock (high speed internal) */
  RCC_HSICmd(ENABLE);
  
  /* wail til HSI ready */
  while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET)
  {}

  /* at startup, SYSCLK driven by MSI. set to HSI */
  RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
  
  /* set MSI to 4mhz */
  RCC_MSIRangeConfig(RCC_MSIRange_6);

  /* turn HSE off */
  RCC_HSEConfig(RCC_HSE_OFF);  
  if (RCC_GetFlagStatus(RCC_FLAG_HSERDY) != RESET)
  {
    while (1) ;
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

static void setup_dac1(void)
{
  /* see 10.2 notes */

  static GPIO_InitTypeDef GPIO_InitStructure;
  static DAC_InitTypeDef DAC_InitStructure;

  /* DAC clock path:
     HSI (16mhz) -> SYSCLK -> HCLK(/1) -> PCLK1(/1)
   */

  /* set the AHB clock (HCLK) prescaler to 1 */
  RCC_HCLKConfig(RCC_SYSCLK_Div1);

  /* set the low speed APB clock (APB1, ie. PCLK1) prescaler to 1 */
  RCC_PCLK1Config(RCC_HCLK_Div1);

  /* enable DAC APB1 clock */
  /* signal connections: HSI(16mhz) -> SYSCLK -> AHB */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4; /* GPIO_Pin_5 for channel 2 */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  DAC_StructInit(&DAC_InitStructure);
  DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
#if 0 /* triangle waveform generation */
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_Triangle;
  DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_TriangleAmplitude_1;
#else
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
  DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
#endif
  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
  DAC_Init(DAC_Channel_1, &DAC_InitStructure);

  /* enable dac channel */
  DAC_Cmd(DAC_Channel_1, ENABLE);
}

static inline void set_dac1_mv(unsigned int mv)
{
  /* mv the millivolts */

  /* vref in millivolts */
  /* #define CONFIG_VREF 5000 */
#define CONFIG_VREF 3000

  /* resolution in bits */
#define CONFIG_DAC_RES 12

  const uint16_t n = (mv * (1 << (CONFIG_DAC_RES - 1))) / CONFIG_VREF;
  DAC_SetChannel1Data(DAC_Align_12b_R, n);
}

void main(void)
{
  static RCC_ClocksTypeDef RCC_Clocks;
  static GPIO_InitTypeDef GPIO_InitStructure;
  static uint16_t dac_value;
  static unsigned int led_state = 0;

  /* Configure Clocks for Application need */
  RCC_Configuration();
  
  /* Configure RTC Clocks */
  RTC_Configuration();

#if 0
  /* Set internal voltage regulator to 1.8v */
  PWR_VoltageScalingConfig(PWR_VoltageScaling_Range1);
  /* Wait Until the Voltage Regulator is ready */
  while (PWR_GetFlagStatus(PWR_FLAG_VOS) != RESET) ;
#endif

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

  setup_dac1();

  dac_value = 0;

  while (1)
  {
    DAC_SetChannel1Data(DAC_Align_12b_R, dac_value & 0xfff);
    dac_value += 0x10;

    if (led_state & 1) switch_leds_on();
    else switch_leds_off();
    led_state ^= 1;

    delay();
  }
}

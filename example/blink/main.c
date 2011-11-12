/* missing type */

typedef unsigned int uint32_t;


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

# define GPIOB 0x40020400 /* port B */
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
  *(volatile uint32_t*)GPIOB_ODR = LED_BLUE | LED_GREEN;
}

static inline void switch_leds_off(void)
{
  *(volatile uint32_t*)GPIOB_ODR = 0;
}

#elif CONFIG_STM32F4_DISCOVERY

#define GPIOD 0x40020C00 /* port D */
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

#else
#error "Architecture must be defined!"
#endif /* otherwise, error */


#define delay()						\
do {							\
  register unsigned int i;				\
  for (i = 0; i < 1000000; ++i)				\
    __asm__ __volatile__ ("nop\n\t":::"memory");	\
} while (0)

/* static void __attribute__((naked)) __attribute__((used)) main(void) */
void main(void)
{
  setup_leds();

  while (1)
  {
    switch_leds_on();
    delay();
    switch_leds_off();
    delay();
  }
}

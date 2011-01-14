typedef unsigned int uint32_t;

#define GPIOC		0x40011000 // port C
#define GPIOC_CRH	(GPIOC + 0x04) // port configuration register high
#define GPIOC_ODR	(GPIOC + 0x0c) // port output data register
#define LED_BLUE	(1<<8) // pin 8
#define LED_GREEN	(1<<9) // pin 9

#define delay()						\
do {							\
  register unsigned int i;				\
  for (i = 0; i < 1000000; ++i)				\
    __asm__ __volatile__ ("nop\n\t":::"memory");	\
} while (0)

static void __attribute__((naked)) __attribute__((used)) main(void)
{
  *(volatile uint32_t*)GPIOC_CRH = 0x44444411;

  while (1)
  {
    *(volatile uint32_t*)GPIOC_ODR = LED_BLUE | LED_GREEN;
    delay();
    *(volatile uint32_t*)GPIOC_ODR = 0;
    delay();
  }
}

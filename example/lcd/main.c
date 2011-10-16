/* this example is only for stm32l discover */

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;


/* boot mode */

#define CONFIG_BOOT_SRAM 1
#define CONFIG_BOOT_FLASH 0


/* gpios
   refer to CD00277537.pdf, APB memory space.
   refer to CD00240193.pdf, GPIO.
*/

#define GPIOA 0x40020000
#define GPIOA_MODER (GPIOA + 0x00)
#define GPIOA_ODR (GPIOA + 0x14)

#define GPIOB 0x40020400
#define GPIOB_MODER (GPIOB + 0x00)
#define GPIOB_ODR (GPIOB + 0x14)

#define GPIOC 0x40020800
#define GPIOC_MODER (GPIOC + 0x00)
#define GPIOC_ODR (GPIOC + 0x14)


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


/* lcd. refer to DM00027954.pdf. */

#if 0

#define LCD_SEG0 PA1
#define LCD_SEG1 PA2
#define LCD_SEG2 PA3
#define LCD_SEG3 PB3
#define LCD_SEG4 PB4
#define LCD_SEG5 PB5
#define LCD_SEG6 PB10
#define LCD_SEG7 PB11
#define LCD_SEG8 PB12
#define LCD_SEG9 PB13
#define LCD_SEG10 PB14
#define LCD_SEG11 PB15
#define LCD_SEG12 PA15
#define LCD_SEG13 PB8
#define LCD_SEG14 PC0
#define LCD_SEG15 PC1
#define LCD_SEG16 PC2
#define LCD_SEG17 PC3
#define LCD_SEG18 PC6
#define LCD_SEG19 PC7
#define LCD_SEG20 PC8
#define LCD_SEG21 PC9
#define LCD_SEG22 PC10
#define LCD_SEG23 PC11
#define LCD_COM0 PA8
#define LCD_COM1 PA9
#define LCD_COM2 PA10
#define LCD_COM3 PB9

#endif

static void setup_lcd(void)
{
  /* set every port in digital output mode  */

  /* PA[1:3,8:10,15] */
  *(volatile uint32_t*)GPIOA_MODER |=
    (1 << (1 * 2)) |
    (1 << (2 * 2)) |
    (1 << (3 * 2)) |
    (1 << (8 * 2)) |
    (1 << (9 * 2)) |
    (1 << (10 * 2)) |
    (1 << (15 * 2));

  /* PB[3:5,8:15] */
  *(volatile uint32_t*)GPIOB_MODER |=
    (1 << (3 * 2)) |
    (1 << (4 * 2)) |
    (1 << (5 * 2)) |
    (1 << (8 * 2)) |
    (1 << (9 * 2)) |
    (1 << (10 * 2)) |
    (1 << (11 * 2)) |
    (1 << (12 * 2)) |
    (1 << (13 * 2)) |
    (1 << (14 * 2)) |
    (1 << (15 * 2));

  /* PC[0:3,6:11] */
  *(volatile uint32_t*)GPIOC_MODER |=
    (1 << (0 * 2)) |
    (1 << (1 * 2)) |
    (1 << (2 * 2)) |
    (1 << (3 * 2)) |
    (1 << (6 * 2)) |
    (1 << (7 * 2)) |
    (1 << (8 * 2)) |
    (1 << (9 * 2)) |
    (1 << (10 * 2)) |
    (1 << (11 * 2));
}

static inline void set_lcd_com(unsigned int i, unsigned int val)
{
  /* table for LCD_COM<N> */
  static const uint32_t regs[4] = { GPIOA_ODR, GPIOA_ODR, GPIOA_ODR, GPIOB_ODR };
  static const uint8_t bits[4] = { 8, 9, 10, 9 };

  uint32_t tmp = *(volatile uint32_t*)regs[i];
  tmp &= ~(1 << bits[i]);
  tmp |= val << bits[i];
  *(volatile uint32_t*)regs[i] = tmp;
}

static void clear_lcd(void)
{
  /* tables for LCD_SEG<N> */

  static const uint32_t regs[24] =
  {
    GPIOA_ODR,
    GPIOA_ODR,
    GPIOA_ODR,

    GPIOB_ODR,
    GPIOB_ODR,
    GPIOB_ODR,
    GPIOB_ODR,
    GPIOB_ODR,
    GPIOB_ODR,
    GPIOB_ODR,
    GPIOB_ODR,
    GPIOB_ODR,

    GPIOA_ODR,

    GPIOB_ODR,

    GPIOC_ODR,
    GPIOC_ODR,
    GPIOC_ODR,
    GPIOC_ODR,
    GPIOC_ODR,
    GPIOC_ODR,
    GPIOC_ODR,
    GPIOC_ODR,
    GPIOC_ODR,
    GPIOC_ODR
  };

  static const uint8_t bits[24] =
  {
    1,
    2,
    3,

    3,
    4,
    5,
    10,
    11,
    12,
    13,
    14,
    15,

    15,

    8,

    0,
    1,
    2,
    3,
    6,
    7,
    8,
    9,
    10,
    11
  };

  /* foreach lcd selector, select and zero */
  unsigned int i;
  for (i = 0; i < sizeof(regs) / sizeof(regs[0]); ++i)
  {
    /* select */
    *(volatile uint32_t*)regs[i] |= 1 << bits[i];

    /* set segments */
    set_lcd_com(0, 0);
    set_lcd_com(1, 0);
    set_lcd_com(2, 0);
    set_lcd_com(3, 0);

    /* deselect */
    *(volatile uint32_t*)regs[i] &= ~(1 << bits[i]);
  }
}

static void update_lcd(void)
{
  static unsigned int state = 0;

  clear_lcd();

/*   if (state == 0) */
  if (1)
  {
    /* left square (segments: 1A, 1B, 1C, 1D, 1E, 1F) */

    /* 1A, 1B: PC10, COM0, COM1 */
    *(volatile uint32_t*)GPIOC_ODR |= 1 << 10;
    set_lcd_com(0, 1);
    set_lcd_com(1, 1);
    set_lcd_com(2, 0);
    set_lcd_com(3, 0);
    *(volatile uint32_t*)GPIOC_ODR &= ~(1 << 10);

    /* 1C: PA2, COM1 */
    *(volatile uint32_t*)GPIOA_ODR |= 1 << 2;
    set_lcd_com(0, 0);
    set_lcd_com(1, 1);
    set_lcd_com(2, 0);
    set_lcd_com(3, 0);
    *(volatile uint32_t*)GPIOA_ODR &= ~(1 << 2);

    /* 1D, 1E: PA1, COM0, COM1 */
    *(volatile uint32_t*)GPIOA_ODR |= 1 << 1;
    set_lcd_com(0, 1);
    set_lcd_com(1, 1);
    set_lcd_com(2, 0);
    set_lcd_com(3, 0);
    *(volatile uint32_t*)GPIOA_ODR &= ~(1 << 1);

    /* 1F: PC11, COM1 */
    *(volatile uint32_t*)GPIOC_ODR |= 1 << 11;
    set_lcd_com(0, 0);
    set_lcd_com(1, 1);
    set_lcd_com(2, 0);
    set_lcd_com(3, 0);
    *(volatile uint32_t*)GPIOC_ODR &= ~(1 << 11);
  }
  else
  {
    /* right square (segments: 6A, 6B, 6C, 6D, 6E, 6F) */
  }

/*   state ^= 1; */
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


static void __attribute__((naked)) __attribute__((used)) main(void)
{
#if CONFIG_BOOT_SRAM
  /* do not use previsouly setup stack, if any */
  setup_stack();
#endif /* CONFIG_BOOT_SRAM */

  setup_leds();

  setup_lcd();
  clear_lcd();
/*   while (1) ; */

  update_lcd();
  while (1) ;

  while (1)
  {
    /* update_lcd(); */
    switch_leds_on();
    delay();

    /* update_lcd(); */
    switch_leds_off();
    delay();
  }
}

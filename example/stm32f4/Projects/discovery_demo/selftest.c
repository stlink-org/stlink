/**
  ******************************************************************************
  * @file    selftest.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    19-September-2011
  * @brief   This file provides the hardware tests
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  */ 

/* Includes ------------------------------------------------------------------*/
#include "selftest.h"

//Library config for this project!!!!!!!!!!!
#include "stm32f4xx_conf.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define MEMS_PASSCONDITION              15
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Init Structure definition */
RCC_ClocksTypeDef      RCC_Clocks;
ADC_InitTypeDef        ADC_InitStructure;
ADC_CommonInitTypeDef  ADC_CommonInitStructure;

__IO uint16_t ConvData1, ConvData2;
__IO uint16_t counter0 = 0, counter1 = 0, Idx = 0;
uint8_t ADC_Channel[2] = {ADC_Channel_2, ADC_Channel_3};
uint8_t DACTest = 0;
uint8_t GPIO_Pin [2] = {GPIO_Pin_2, GPIO_Pin_3};

uint16_t count = 0, count1 = 24, Left_Right = 0;
const int16_t sinebuf[48] = {0, 4276, 8480, 12539, 16383, 19947, 23169, 25995,
                             28377, 30272, 31650, 32486, 32767, 32486, 31650, 30272,
                             28377, 25995, 23169, 19947, 16383, 12539, 8480, 4276,
                             0, -4276, -8480, -12539, -16383, -19947, -23169, -25995,
                             -28377, -30272, -31650, -32486, -32767, -32486, -31650, -30272,
                             -28377, -25995, -23169, -19947, -16383, -12539, -8480, -4276
                             };
extern __IO uint32_t TimingDelay;

extern LIS302DL_InitTypeDef  LIS302DL_InitStruct;
extern LIS302DL_FilterConfigTypeDef LIS302DL_FilterStruct;  

extern __IO int8_t X_Offset, Y_Offset, Z_Offset;
extern uint8_t Buffer[6];
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/**
  * @brief Test MEMS Hardware.
  *   The main objectif of this test is to check the hardware connection of the 
  *   MEMS peripheral.
  * @param None
  * @retval None
  */
void Accelerometer_MEMS_Test(void)
{
  uint8_t temp, memsteststatus = 0x00;
  uint8_t xdata, ydata = 0;
  
  /* MEMS configuration ------------------------------------------------------*/
  /* Set configuration of LIS302DL*/
  LIS302DL_InitStruct.Power_Mode = LIS302DL_LOWPOWERMODE_ACTIVE;
  LIS302DL_InitStruct.Output_DataRate = LIS302DL_DATARATE_100;
  LIS302DL_InitStruct.Axes_Enable = LIS302DL_X_ENABLE | LIS302DL_Y_ENABLE;
  LIS302DL_InitStruct.Full_Scale = LIS302DL_FULLSCALE_2_3;
  LIS302DL_InitStruct.Self_Test = LIS302DL_SELFTEST_NORMAL;
  LIS302DL_Init(&LIS302DL_InitStruct);
  
  /* Set configuration of Internal High Pass Filter of LIS302DL*/
  LIS302DL_FilterStruct.HighPassFilter_Data_Selection = LIS302DL_FILTEREDDATASELECTION_OUTPUTREGISTER;
  LIS302DL_FilterStruct.HighPassFilter_CutOff_Frequency = LIS302DL_HIGHPASSFILTER_LEVEL_1;
  LIS302DL_FilterStruct.HighPassFilter_Interrupt = LIS302DL_HIGHPASSFILTERINTERRUPT_1_2;
  LIS302DL_FilterConfig(&LIS302DL_FilterStruct);
  
  /* Required delay for the MEMS Accelerometre: Turn-on time = 3/Output data Rate 
                                                             = 3/100 = 30ms */
  Delay(30);
  
  /* Read WHO_AM_I register */
  LIS302DL_Read(&temp, LIS302DL_WHO_AM_I_ADDR, 1);
  
  /* Check device identification register, this register should contains 
  the device identifier that for LIS302DL is set to 0x3B */
  if (temp != 0x3B)
  {
    Fail_Handler();
  }

  TimingDelay = 500;
  /* Wait until detecting all MEMS direction or timeout */
  while((memsteststatus == 0x00)&&(TimingDelay != 0x00))
  {
    LIS302DL_Read(Buffer, LIS302DL_OUT_X_ADDR, 4);
    xdata = ABS((int8_t)(Buffer[0]));
    ydata = ABS((int8_t)(Buffer[2]));
    /* Check test PASS condition */   
    if ((xdata > MEMS_PASSCONDITION) || (ydata > MEMS_PASSCONDITION)) 
    {
      /* MEMS Test PASS */
      memsteststatus = 0x01;
    }
  }
  
  /* MEMS test status: PASS */ 
  if(memsteststatus != 0x00)
  {
    /* Turn Green LED ON: signaling MEMS Test PASS */
    STM_EVAL_LEDOn(LED4);
    
    /* Waiting User Button is pressed */
    while (STM_EVAL_PBGetState(BUTTON_USER) == Bit_RESET)
    {}
    
    /* Waiting User Button is Released */
    while (STM_EVAL_PBGetState(BUTTON_USER) == Bit_SET)
    {}
    
    /* Turn Green LED OFF: signaling the end of MEMS Test and switching to 
       the next Sub Test */
    STM_EVAL_LEDOff(LED4);
  }
  /* MEMS test status: Timeout occurs */
  else
  {
    Fail_Handler();
  }
}

/**
  * @brief Test USB Hardware.
  *   The main objectif of this test is to check the hardware connection of the 
  *   Audio and USB peripheral.
  * @param None
  * @retval None
  */
void USB_Test(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /******************************** USB Test **********************************/
  
  /*----------------- Part1: without cables connected ------------------------*/ 
  
  /* GPIOA, GPIOC and GPIOD clock enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC | \
                         RCC_AHB1Periph_GPIOD, ENABLE);
  
  /* GPIOD Configuration: Pins 5 in output push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  /* Turn LED8 ON using PD5 */
  GPIO_ResetBits(GPIOD, GPIO_Pin_5);
  
  /* GPIOC Configuration: Pin 0 in output push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /* GPIOA Configuration: Pin 9 in input pull-up */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Turn LED7 ON using PC0 (5v) */
  GPIO_ResetBits(GPIOC, GPIO_Pin_0); 
  
  /* Waiting delay 10ms */
  Delay(1);
  
  if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_9) == Bit_RESET)
  {
    Fail_Handler();
  }
  
  /* GPIOA Configuration: Pins 10 in output push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Waiting delay 10ms */
  Delay(1);
  
  /* Check the ID level without cable connected */
  if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) == Bit_RESET)
  {
    Fail_Handler();
  }
  
  /* Turn LED7 OFF using PC0 */
  GPIO_SetBits(GPIOC, GPIO_Pin_0);  
  
  /* GPIOA Configuration: Pins 11, 12 in input pull-up */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* GPIOA Configuration: Pin 9 in output push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_ResetBits(GPIOA, GPIO_Pin_9);
  
  /* Waiting delay 10ms */
  Delay(1);
  
  /* Check PA11 and PA12 level without cable connected */
  if ((GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11) == Bit_RESET) || \
      (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == Bit_RESET))
  {
    Fail_Handler();
  }
  
  /* GPIOA Configuration: Pins 12 in input pull-up */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* GPIOA Configuration: Pin 11 in output push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_ResetBits(GPIOA, GPIO_Pin_11);
  
  /* Waiting delay 10ms */
  Delay(1);
  
  /* Check PA12 level without cable connected */
  if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == Bit_RESET)
  {
    Fail_Handler();
  }
  
  /* GPIOA Configuration: Pins 11 in input pull-up */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* GPIOA Configuration: Pin 12 in output push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_ResetBits(GPIOA, GPIO_Pin_12);
  
  /* Waiting delay 10ms */
  Delay(1);
  
  /* Check PA12 level without cable connected */
  if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11) == Bit_RESET)
  {
    Fail_Handler();
  }
  
  /* GPIOA Configuration: Pins 9 in output push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Turn LED7 ON using PA9 */
  GPIO_SetBits(GPIOA, GPIO_Pin_9);
  
  /* Turn Green LED ON: signaling Audio USB Test part1 PASS */
  STM_EVAL_LEDOn(LED4);
  
  /* Waiting User Button is pressed */
  while (STM_EVAL_PBGetState(BUTTON_USER) == Bit_RESET)
  {}
  
  /* Waiting User Button is Released */
  while (STM_EVAL_PBGetState(BUTTON_USER) != Bit_RESET)
  {}
  
  /* Turn Green LED OFF: signaling the end of Audio USB Test part1 and switching to 
  the part2 */
  STM_EVAL_LEDOff(LED4);
  
  /* Turn LED7 OFF using PA9 */
  GPIO_ResetBits(GPIOA, GPIO_Pin_9);
  
  /* Turn LED8 OFF using PD5 */
  GPIO_SetBits(GPIOD, GPIO_Pin_5);
  
  /*--------------- Part2: with Audio USB cables connected  ------------------*/ 
  
  /*********************************** USB Test *******************************/
  /* Check the ID level with cable connected */
  if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) != Bit_RESET)
  {
    Fail_Handler();
  }
  
  /* GPIOA Configuration: Pins 11, 12 in input pull-down */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* GPIOA Configuration: Pin 9 in output push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_SetBits(GPIOA, GPIO_Pin_9);
  
  /* Waiting delay 10ms */
  Delay(1);
  
  /* Check PA11 and PA12 level with cable connected */
  if ((GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11) == Bit_RESET) || \
      (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == Bit_RESET))
  {
    Fail_Handler();
  }
  
  /* GPIOA Configuration: Pins 9, 12 in input pull-down */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* GPIOA Configuration: Pin 11 in output push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_SetBits(GPIOA, GPIO_Pin_11);
  
  /* Waiting delay 10ms */
  Delay(1);
  
  /* Check PA9 and PA12 level with cable connected */
  if ((GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_9) == Bit_RESET)|| \
      (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == Bit_RESET))
  {
    Fail_Handler();
  }
  
  /* GPIOA Configuration: Pins 9, 11 in input pull-down */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* GPIOA Configuration: Pin 12 in output push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_SetBits(GPIOA, GPIO_Pin_12);
  
  /* Waiting delay 10ms */
  Delay(1);
  
  /* Check PA9 and PA12 level with cable connected */
  if ((GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_9) == Bit_RESET)|| \
      (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11) == Bit_RESET))
  {
    Fail_Handler();
  }

  /* GPIOA Configuration: Pins 11, 12 in input pull-down */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* GPIOA Configuration: Pin 9 in output push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Turn LED7 OFF using PA9 */
  GPIO_ResetBits(GPIOA, GPIO_Pin_9);
}

/**
  * @brief Test Audio Hardware.
  *   The main objectif of this test is to check the hardware connection of the 
  *   Audio peripheral.
  * @param  None
  * @retval None
  */
void Audio_Test(void)
{ 
  GPIO_InitTypeDef  GPIO_InitStructure;  
  uint8_t audioteststatus = 0x00;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
  
  /* Set the current audio interface: I2S or DAC */
  EVAL_AUDIO_SetAudioInterface(AUDIO_INTERFACE_I2S);
  
  /* Initialize the Audio codec and all related peripherals (I2S, I2C, IOs...) */  
  if (EVAL_AUDIO_Init(OUTPUT_DEVICE_HEADPHONE, 87, I2S_AudioFreq_48k) !=0)
  {
    Fail_Handler();
  }
  /* I2S code to be exectued under the I2S interrupt */
  DACTest = 0;
  
  /* ADC Common Init */
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div8;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_20Cycles; 
  ADC_CommonInit(&ADC_CommonInitStructure);
  
  /* ADC peripherals Init */
  ADC_StructInit(&ADC_InitStructure);
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_8b;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = 1;
  ADC_Init(ADC1, &ADC_InitStructure);

  ADC_Init(ADC2, &ADC_InitStructure);
  
  /* Configure ADC Channels pin as analog input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 ;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  counter1 = 0;
  counter0 = 0;
  audioteststatus = 0;
  /* ADCperipheral[PerIdx] Regular Channel Config */  
  ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_56Cycles);
  /* ADCperipheral[PerIdx] Regular Channel Config */  
  ADC_RegularChannelConfig(ADC2, ADC_Channel_3, 1, ADC_SampleTime_56Cycles);
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);
  ADC_Cmd(ADC2, ENABLE);
 
  TimingDelay = 500;
  /* Wait until detecting 500 data*/
  while((audioteststatus == 0)&&(TimingDelay != 0))
  {
    ADC_SoftwareStartConv(ADC1);
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    ConvData1 = ADC_GetConversionValue(ADC1); 

    ADC_SoftwareStartConv(ADC2);
    while(ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC) == RESET);
    ConvData2 = ADC_GetConversionValue(ADC2);
    
    /* 1.75V equals to 150 */
    if ((ConvData1 > 150) && (ConvData2 < 10) && (counter1 != 500))
    {
      counter1 ++;
    }
    if ((ConvData1 < 10) && (ConvData2 > 150) && (counter0 != 500))
    {
      counter0 ++;
    }
    if((counter1 == 500) && (counter0 == 500))
    {
      audioteststatus = 1; 
    }
  }
  
  /* Disable ADC Peripherals */ 
  ADC_Cmd(ADC1, DISABLE);
  ADC_Cmd(ADC2, DISABLE);
  
  /* Audio test status: FAIL */ 
  if(audioteststatus == 0)
  {
    Fail_Handler();
  }
  
  EVAL_AUDIO_DeInit();
  EVAL_AUDIO_SetAudioInterface(AUDIO_INTERFACE_DAC);
  /* Initialize the Audio codec and all related peripherals (I2S, I2C, IOs...) */  
  if (EVAL_AUDIO_Init(OUTPUT_DEVICE_HEADPHONE, 100, I2S_AudioFreq_48k) !=0)
  {
    Fail_Handler();
  }
  
  /* DAC code to be exectued under the I2S interrupt */
  DACTest = 1;
  counter1 = 0;
  counter0 = 0;
  audioteststatus = 0;
  
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);
  ADC_Cmd(ADC2, ENABLE);
  
  TimingDelay = 500;
  /* Wait until detecting 50 data*/
  while((audioteststatus == 0)&&(TimingDelay != 0))
  {
    
    ADC_SoftwareStartConv(ADC1);
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    
    ConvData1 = ADC_GetConversionValue(ADC1); 
    
    ADC_SoftwareStartConv(ADC2);
    while(ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC) == RESET);
    
    ConvData2 = ADC_GetConversionValue(ADC2);
    
    /* 2.0V equals to 170 */
    if ((ConvData1 > 170) && (ConvData2 > 170) &&(counter1 != 500))
    {
      counter1 ++;
    }
    if ((ConvData1 < 10) && (ConvData2 < 10) && (counter0 != 500))
    {
      counter0 ++;
    }
    if((counter1 == 500) && (counter0 == 500))
    {
      audioteststatus = 1; 
    }
  }
  
  /* Audio test status: FAIL */ 
  if(audioteststatus == 0x00)
  {
    Fail_Handler();
  }
  
  /* Turn Green LED ON: signaling Audio USB Test part2 PASS */
  STM_EVAL_LEDOn(LED4);
  
  /* Waiting User_Button pressed */
  while (STM_EVAL_PBGetState(BUTTON_USER) == Bit_RESET)
  {}
  
  /* Turn Green LED OFF: signaling the end of Audio USB Test part2 */
  STM_EVAL_LEDOff(LED4);
}

/**
  * @brief Test Micophone MEMS Hardware.
  *   The main objectif of this test is to check the hardware connection of the 
  *   Microphone MEMS peripheral.
  * @param None
  * @retval None
  */
void Microphone_MEMS_Test(void)
{
  uint16_t data = 0x00;
  uint8_t index = 0x00;
  I2S_InitTypeDef  I2S_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable the SPI clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

  /* Enable GPIO clocks */
  RCC_AHB1PeriphClockCmd(SPI_SCK_GPIO_CLK | SPI_MOSI_GPIO_CLK, ENABLE);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;

  /* SPI SCK pin configuration */
  GPIO_InitStructure.GPIO_Pin = SPI_SCK_PIN;
  GPIO_Init(SPI_SCK_GPIO_PORT, &GPIO_InitStructure);
  
  /* Connect SPI pins to AF5 */  
  GPIO_PinAFConfig(SPI_SCK_GPIO_PORT, SPI_SCK_SOURCE, SPI_SCK_AF);
  
  /* SPI MOSI pin configuration */
  GPIO_InitStructure.GPIO_Pin =  SPI_MOSI_PIN;
  GPIO_Init(SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_PinAFConfig(SPI_MOSI_GPIO_PORT, SPI_MOSI_SOURCE, SPI_MOSI_AF);
  
  /* I2S configuration -------------------------------------------------------*/
  SPI_I2S_DeInit(SPI2);
  I2S_InitStructure.I2S_AudioFreq = 64000;
  I2S_InitStructure.I2S_Standard = I2S_Standard_MSB;
  I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;
  I2S_InitStructure.I2S_Mode = I2S_Mode_MasterRx;
  I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
  /* Initialize the I2S peripheral with the structure above */
  I2S_Init(SPI2, &I2S_InitStructure);
  
  /* Enable the I2S peripheral */
  I2S_Cmd(SPI2, ENABLE);
  
  /* Waiting until MEMS microphone ready : Wake-up Time */
  Delay(10);
  
  TimingDelay = 500;
  /* Wait until detect the click on the MEMS microphone or TimeOut delay*/
  while((index < 30) && (TimingDelay != 0x00))
  { 
    /* Waiting RXNE Flag or TimeOut delay */
    while((SPI_I2S_GetFlagStatus(SPI2, SPI_FLAG_RXNE) == RESET)&& (TimingDelay != 0x00))
    {}
    data = SPI_I2S_ReceiveData(SPI2);
    if (data == 0xFFFF)
    {
      index++;  
    }
  }
  
  /* MEMS microphone test status: Timeout occurs */
  if(index != 30)
  {
    Fail_Handler();
  }
}

/*--------------------------------
       Callbacks implementation:
           the callbacks prototypes are defined in the stm324xg_eval_audio_codec.h file
           and their implementation should be done in the user code if they are needed.
           Below some examples of callback implementations.
                                     --------------------------------------------------------*/
/**
  * @brief  Calculates the remaining file size and new position of the pointer.
  * @param  None
  * @retval None
  */
void EVAL_AUDIO_TransferComplete_CallBack(uint32_t pBuffer, uint32_t Size)
{
  /* Calculate the remaining audio data in the file and the new size 
     for the DMA transfer. If the Audio files size is less than the DMA max 
     data transfer size, so there is no calculation to be done, just restart 
     from the beginning of the file ... */
  /* Check if the end of file has been reached */

}

/**
  * @brief  Manages the DMA Half Transfer complete interrupt.
  * @param  None
  * @retval None
  */
void EVAL_AUDIO_HalfTransfer_CallBack(uint32_t pBuffer, uint32_t Size)
{  
#ifdef AUDIO_MAL_MODE_CIRCULAR
  
  /* Display message on the LCD screen */
  LCD_DisplayStringLine(Line8, " 1/2 Buffer Reached "); 
  
#endif /* AUDIO_MAL_MODE_CIRCULAR */
  
  /* Generally this interrupt routine is used to load the buffer when 
  a streaming scheme is used: When first Half buffer is already transferred load 
  the new data to the first half of buffer while DMA is transferring data from 
  the second half. And when Transfer complete occurs, load the second half of 
  the buffer while the DMA is transferring from the first half ... */
  /* 
    ...........
                   */
}
/**
  * @brief  Get next data sample callback
  * @param  None
  * @retval Next data sample to be sent
  */
uint16_t EVAL_AUDIO_GetSampleCallBack(void)
{
  uint16_t data = 0;
  
  if (DACTest == 0)
  {
    if (Left_Right==0)
    {
      /* Get the next sample to be sent */
      data = sinebuf[count++];
      
      if (count == 48)
      {
        count = 0x00;
      }
      Left_Right = 1;
    }
    else
    {
      /* Get the next sample to be sent */
      data = sinebuf[count1++];
      
      if (count1 == 48)
      {
        count1 = 0x00;
      }
      Left_Right = 0;
    }
  }
  else
  {
    /* Get the next sample to be sent */
    data = 32768 + sinebuf[count++];

    if (count == 48)
    {
      count = 0x00;
    }
  }
  return data;
}


/**
  * @brief  Manages the DMA FIFO error interrupt.
  * @param  None
  * @retval None
  */
void EVAL_AUDIO_Error_CallBack(void* pData)
{
  /* Stop the program with an infinite loop */
  while (1)
  {}
  
  /* could also generate a system reset to recover from the error */
  /* .... */
}

#ifndef USE_DEFAULT_TIMEOUT_CALLBACK
/**
  * @brief  Basic management of the timeout situation.
  * @param  None.
  * @retval None.
  */
uint32_t Codec_TIMEOUT_UserCallback(void)
{   
  /* Block communication and all processes */
  while (1)
  {   
  }
}
#endif /* USE_DEFAULT_TIMEOUT_CALLBACK */
/*----------------------------------------------------------------------------*/


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

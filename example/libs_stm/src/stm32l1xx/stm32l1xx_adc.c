/**
  ******************************************************************************
  * @file    stm32l1xx_adc.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    31-December-2010
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of the Analog to Digital Convertor (ADC) peripheral:           
  *           - Initialization and Configuration
  *           - Power saving
  *           - Analog Watchdog configuration              
  *           - Temperature Sensor & Vrefint (Voltage Reference internal) management 
  *           - Regular Channels Configuration
  *           - Regular Channels DMA Configuration
  *           - Injected channels Configuration      
  *           - Interrupts and flags management       
  *         
  *  @verbatim
  *                               
  *          ===================================================================      
  *                                   How to use this driver
  *          ===================================================================          
  *            - Configure the ADC Prescaler, conversion resolution and data 
  *              alignment using the ADC_Init() function.
  *            - Activate the ADC peripheral using ADC_Cmd() function.  
  *
  *          Regular channels group configuration
  *          ====================================    
  *            - To configure the ADC regular channels group features, use 
  *              ADC_Init() and ADC_RegularChannelConfig() functions.
  *            - To activate the continuous mode, use the ADC_continuousModeCmd()
  *              function.
  *            - To configurate and activate the Discontinuous mode, use the 
  *              ADC_DiscModeChannelCountConfig() and ADC_DiscModeCmd() functions.        
  *            - To read the ADC converted values, use the ADC_GetConversionValue()
  *              function.
  *
  *          DMA for Regular channels group features configuration
  *          ====================================================== 
  *           - To enable the DMA mode for regular channels group, use the 
  *             ADC_DMACmd() function.
  *           - To enable the generation of DMA requests continuously at the end
  *             of the last DMA transfer, use the ADC_DMARequestAfterLastTransferCmd() 
  *             function.    
             
  *          Injected channels group configuration
  *          =====================================    
  *            - To configure the ADC Injected channels group features, use 
  *              ADC_InjectedChannelConfig() and  ADC_InjectedSequencerLengthConfig()
  *              functions.
  *            - To activate the continuous mode, use the ADC_continuousModeCmd()
  *              function.
  *            - To activate the Injected Discontinuous mode, use the 
  *              ADC_InjectedDiscModeCmd() function.  
  *            - To activate the AutoInjected mode, use the ADC_AutoInjectedConvCmd() 
  *              function.        
  *            - To read the ADC converted values, use the ADC_GetInjectedConversionValue() 
  *              function.
  *              
  *  @endverbatim
  *         
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
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  ******************************************************************************  
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_adc.h"
#include "stm32l1xx_rcc.h"

/** @addtogroup STM32L1xx_StdPeriph_Driver
  * @{
  */

/** @defgroup ADC 
  * @brief ADC driver modules
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* ADC DISCNUM mask */
#define CR1_DISCNUM_RESET         ((uint32_t)0xFFFF1FFF)
   
/* ADC AWDCH mask */
#define CR1_AWDCH_RESET           ((uint32_t)0xFFFFFFE0) 
  
/* ADC Analog watchdog enable mode mask */
#define CR1_AWDMODE_RESET         ((uint32_t)0xFF3FFDFF)
  
/* CR1 register Mask */
#define CR1_CLEAR_MASK            ((uint32_t)0xFCFFFEFF) 
   
/* ADC DELAY mask */            
#define CR2_DELS_RESET            ((uint32_t)0xFFFFFF0F)
   
/* ADC JEXTEN mask */
#define CR2_JEXTEN_RESET          ((uint32_t)0xFFCFFFFF)
  
/* ADC JEXTSEL mask */
#define CR2_JEXTSEL_RESET         ((uint32_t)0xFFF0FFFF)
  
/* CR2 register Mask */
#define CR2_CLEAR_MASK            ((uint32_t)0xC0FFF7FD)

/* ADC SQx mask */
#define SQR5_SQ_SET               ((uint32_t)0x0000001F)  
#define SQR4_SQ_SET               ((uint32_t)0x0000001F)  
#define SQR3_SQ_SET               ((uint32_t)0x0000001F)  
#define SQR2_SQ_SET               ((uint32_t)0x0000001F)  
#define SQR1_SQ_SET               ((uint32_t)0x0000001F)

/* ADC L Mask */
#define SQR1_L_RESET              ((uint32_t)0xFE0FFFFF) 

/* ADC JSQx mask */
#define JSQR_JSQ_SET              ((uint32_t)0x0000001F) 
 
/* ADC JL mask */
#define JSQR_JL_SET               ((uint32_t)0x00300000) 
#define JSQR_JL_RESET             ((uint32_t)0xFFCFFFFF) 

/* ADC SMPx mask */
#define SMPR1_SMP_SET             ((uint32_t)0x00000007)  
#define SMPR2_SMP_SET             ((uint32_t)0x00000007)
#define SMPR3_SMP_SET             ((uint32_t)0x00000007) 

/* ADC JDRx registers offset */
#define JDR_OFFSET                ((uint8_t)0x30)   
  
/* ADC CCR register Mask */
#define CR_CLEAR_MASK             ((uint32_t)0xFFFCFFFF) 

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup ADC_Private_Functions
  * @{
  */

/** @defgroup ADC_Group1 Initialization and Configuration functions
 *  @brief   Initialization and Configuration functions 
 *
@verbatim    
 ===============================================================================
                      Initialization and Configuration functions
 ===============================================================================  
  This section provides functions allowing to:
   - Initialize and configure the ADC Prescaler
   - ADC Conversion Resolution (12bit..6bit)
   - Scan Conversion Mode (multichannels or one channel) for regular group
   - ADC Continuous Conversion Mode (Continuous or Single conversion) for regular group
   - External trigger Edge and source of regular group, 
   - Converted data alignment (left or right)
   - The number of ADC conversions that will be done using the sequencer for regular channel group
   - Enable or disable the ADC peripheral
   
@endverbatim
  * @{
  */

/**
  * @brief  Deinitializes ADC1 peripheral registers to their default reset values.
  * @param  None
  * @retval None
  */
void ADC_DeInit(ADC_TypeDef* ADCx)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  
  /* Enable ADC1 reset state */
  RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, ENABLE);
  /* Release ADC1 from reset state */
  RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, DISABLE);
}

/**
  * @brief  Initializes the ADCx peripheral according to the specified parameters
  *         in the ADC_InitStruct.
  * @note   This function is used to configure the global features of the ADC ( 
  *         Resolution and Data Alignment), however, the rest of the configuration
  *         parameters are specific to the regular channels group (scan mode 
  *         activation, continuous mode activation, External trigger source and 
  *         edge, number of conversion in the regular channels group sequencer).   
  * @param  ADCx: where x can be 1 to select the ADC peripheral.
  * @param  ADC_InitStruct: pointer to an ADC_InitTypeDef structure that contains 
  *         the configuration information for the specified ADC peripheral.
  * @retval None
  */
void ADC_Init(ADC_TypeDef* ADCx, ADC_InitTypeDef* ADC_InitStruct)               
{
  uint32_t tmpreg1 = 0;
  uint8_t tmpreg2 = 0;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_RESOLUTION(ADC_InitStruct->ADC_Resolution)); 
  assert_param(IS_FUNCTIONAL_STATE(ADC_InitStruct->ADC_ScanConvMode));
  assert_param(IS_FUNCTIONAL_STATE(ADC_InitStruct->ADC_ContinuousConvMode)); 
  assert_param(IS_ADC_EXT_TRIG_EDGE(ADC_InitStruct->ADC_ExternalTrigConvEdge)); 
  assert_param(IS_ADC_EXT_TRIG(ADC_InitStruct->ADC_ExternalTrigConv));    
  assert_param(IS_ADC_DATA_ALIGN(ADC_InitStruct->ADC_DataAlign)); 
  assert_param(IS_ADC_REGULAR_LENGTH(ADC_InitStruct->ADC_NbrOfConversion));
  
  /*---------------------------- ADCx CR1 Configuration -----------------*/
  /* Get the ADCx CR1 value */
  tmpreg1 = ADCx->CR1;
  /* Clear RES and SCAN bits */ 
  tmpreg1 &= CR1_CLEAR_MASK;
  /* Configure ADCx: scan conversion mode and resolution */
  /* Set SCAN bit according to ADC_ScanConvMode value */
  /* Set RES bit according to ADC_Resolution value */ 
  tmpreg1 |= (uint32_t)(((uint32_t)ADC_InitStruct->ADC_ScanConvMode << 8) | ADC_InitStruct->ADC_Resolution);
  /* Write to ADCx CR1 */
  ADCx->CR1 = tmpreg1;
  
  /*---------------------------- ADCx CR2 Configuration -----------------*/
  /* Get the ADCx CR2 value */
  tmpreg1 = ADCx->CR2;
  /* Clear CONT, ALIGN, EXTEN and EXTSEL bits */
  tmpreg1 &= CR2_CLEAR_MASK;
  /* Configure ADCx: external trigger event and edge, data alignment and continuous conversion mode */
  /* Set ALIGN bit according to ADC_DataAlign value */
  /* Set EXTEN bits according to ADC_ExternalTrigConvEdge value */ 
  /* Set EXTSEL bits according to ADC_ExternalTrigConv value */
  /* Set CONT bit according to ADC_ContinuousConvMode value */
  tmpreg1 |= (uint32_t)(ADC_InitStruct->ADC_DataAlign | ADC_InitStruct->ADC_ExternalTrigConv | 
              ADC_InitStruct->ADC_ExternalTrigConvEdge | ((uint32_t)ADC_InitStruct->ADC_ContinuousConvMode << 1));
  /* Write to ADCx CR2 */
  ADCx->CR2 = tmpreg1;
  
  /*---------------------------- ADCx SQR1 Configuration -----------------*/
  /* Get the ADCx SQR1 value */
  tmpreg1 = ADCx->SQR1;
  /* Clear L bits */
  tmpreg1 &= SQR1_L_RESET;
  /* Configure ADCx: regular channel sequence length */
  /* Set L bits according to ADC_NbrOfConversion value */ 
  tmpreg2 |= (uint8_t)(ADC_InitStruct->ADC_NbrOfConversion - (uint8_t)1);
  tmpreg1 |= ((uint32_t)tmpreg2 << 20);
  /* Write to ADCx SQR1 */
  ADCx->SQR1 = tmpreg1;
}

/**
  * @brief  Fills each ADC_InitStruct member with its default value.
  * @note   This function is used to initialize the global features of the ADC ( 
  *         Resolution and Data Alignment), however, the rest of the configuration
  *         parameters are specific to the regular channels group (scan mode 
  *         activation, continuous mode activation, External trigger source and 
  *         edge, number of conversion in the regular channels group sequencer).   
  * @param  ADC_InitStruct: pointer to an ADC_InitTypeDef structure which will 
  *         be initialized.
  * @retval None
  */
void ADC_StructInit(ADC_InitTypeDef* ADC_InitStruct)                            
{
  /* Reset ADC init structure parameters values */
  /* Initialize the ADC_Resolution member */
  ADC_InitStruct->ADC_Resolution = ADC_Resolution_12b;

  /* Initialize the ADC_ScanConvMode member */
  ADC_InitStruct->ADC_ScanConvMode = DISABLE;

  /* Initialize the ADC_ContinuousConvMode member */
  ADC_InitStruct->ADC_ContinuousConvMode = DISABLE;

  /* Initialize the ADC_ExternalTrigConvEdge member */
  ADC_InitStruct->ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;

  /* Initialize the ADC_ExternalTrigConv member */
  ADC_InitStruct->ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_CC2;

  /* Initialize the ADC_DataAlign member */
  ADC_InitStruct->ADC_DataAlign = ADC_DataAlign_Right;

  /* Initialize the ADC_NbrOfConversion member */
  ADC_InitStruct->ADC_NbrOfConversion = 1;
}

/**
  * @brief  Initializes the ADCs peripherals according to the specified parameters
  *          in the ADC_CommonInitStruct.
  * @param  ADC_CommonInitStruct: pointer to an ADC_CommonInitTypeDef structure 
  *         that contains the configuration information (Prescaler) for ADC1 peripheral.
  * @retval None
  */
void ADC_CommonInit(ADC_CommonInitTypeDef* ADC_CommonInitStruct)                           
{
  uint32_t tmpreg = 0;
  
  /* Check the parameters */
  assert_param(IS_ADC_PRESCALER(ADC_CommonInitStruct->ADC_Prescaler));

  /*---------------------------- ADC CCR Configuration -----------------*/
  /* Get the ADC CCR value */
  tmpreg = ADC->CCR;

  /* Clear ADCPRE bit */ 
  tmpreg &= CR_CLEAR_MASK;
  
  /* Configure ADCx: ADC prescaler according to ADC_Prescaler */                
  tmpreg |= (uint32_t)(ADC_CommonInitStruct->ADC_Prescaler);        
                
  /* Write to ADC CCR */
  ADC->CCR = tmpreg;
}

/**
  * @brief  Fills each ADC_CommonInitStruct member with its default value.
  * @param  ADC_CommonInitStruct: pointer to an ADC_CommonInitTypeDef structure
  *         which will be initialized.
  * @retval None
  */
void ADC_CommonStructInit(ADC_CommonInitTypeDef* ADC_CommonInitStruct)                      
{
  /* Reset ADC init structure parameters values */
  /* Initialize the ADC_Prescaler member */
  ADC_CommonInitStruct->ADC_Prescaler = ADC_Prescaler_Div1;

}

/**
  * @brief  Enables or disables the specified ADC peripheral.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  NewState: new state of the ADCx peripheral. 
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void ADC_Cmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    /* Set the ADON bit to wake up the ADC from power down mode */
    ADCx->CR2 |= (uint32_t)ADC_CR2_ADON;
  }
  else
  {
    /* Disable the selected ADC peripheral */
    ADCx->CR2 &= (uint32_t)(~ADC_CR2_ADON);
  }
}

/**
  * @}
  */

/** @defgroup ADC_Group2 Power saving functions
 *  @brief   Power saving functions 
 *
@verbatim   
 ===============================================================================
                              Power saving functions
 ===============================================================================  

  This section provides functions allowing to reduce power consumption.
  The two function must be combined to get the maximal benefits:
  When the ADC frequency is higher than the CPU one, it is recommended to  
  1. Insert a freeze delay : 
     ==> using ADC_DelaySelectionConfig(ADC1, ADC_DelayLength_Freeze);
  2. Enable the power down in Idle and Delay phases :
     ==> using ADC_PowerDownCmd(ADC1, ADC_PowerDown_Idle_Delay, ENABLE);

@endverbatim
  * @{
  */

/**
  * @brief  Enables or disables the ADC Power Down during Delay and/or Idle phase.
  * @note   ADC power-on and power-off can be managed by hardware to cut the 
  *         consumption when the ADC is not converting. 
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  ADC_PowerDown: The ADC power down configuration. 
  *         This parameter can be one of the following values:
  *     @arg ADC_PowerDown_Delay:      ADC is powered down during delay phase  
  *     @arg ADC_PowerDown_Idle:       ADC is powered down during Idle phase 
  *     @arg ADC_PowerDown_Idle_Delay: ADC is powered down during Delay and Idle phases
  * @note   The ADC can be powered down: 
  *         - During the hardware delay insertion (using the ADC_PowerDown_Delay
  *           parameter) 
  *           => The ADC is powered up again at the end of the delay. 
  *         - During the ADC is waiting for a trigger event ( using the 
  *           ADC_PowerDown_Idle parameter) 
  *           => The ADC is powered up at the next trigger event.
  *         - During the hardware delay insertion or the ADC is waiting for a 
  *           trigger event (using the ADC_PowerDown_Idle_Delay parameter) 
  *            => The ADC is powered up only at the end of the delay and at the 
  *              next trigger event.     
  * @param  NewState: new state of the ADCx power down. 
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void ADC_PowerDownCmd(ADC_TypeDef* ADCx, uint32_t ADC_PowerDown, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  assert_param(IS_ADC_POWER_DOWN(ADC_PowerDown));
  
  if (NewState != DISABLE)
  {
    /* Enable the ADC power-down during Delay and/or Idle phase */
    ADCx->CR1 |= ADC_PowerDown;
  }
  else
  {
    /* Disable The ADC power-down during Delay and/or Idle phase */
    ADCx->CR1 &= (uint32_t)~ADC_PowerDown;
  }
}

/**
  * @brief  Defines the length of the delay which is applied after a conversion 
  *         or a sequence of conversion.
  * @note   When the CPU clock is not fast enough to manage the data rate, a 
  *         Hardware delay can be introduced between ADC conversions to reduce 
  *         this data rate. 
  * @note   The Hardware delay is inserted after :
  *         - each regular conversion
  *         - after each sequence of injected conversions
  * @note   No Hardware delay is inserted between conversions of different groups.
  * @note   When the hardware delay is not enough, the Freeze Delay Mode can be 
  *         selected and a new conversion can start only if all the previous data 
  *         of the same group have been treated:
  *         - for a regular conversion: once the ADC conversion data register has 
  *           been read (using ADC_GetConversionValue() function) or if the EOC 
  *           Flag has been cleared (using ADC_ClearFlag() function).
  *         - for an injected conversion: when the JEOC bit has been cleared 
  *           (using ADC_ClearFlag() function).
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  ADC_DelayLength: The length of delay which is applied after a 
  *         conversion or a sequence of conversion. 
  *   This parameter can be one of the following values:
  *     @arg ADC_DelayLength_None: No delay 
  *     @arg ADC_DelayLength_Freeze: Delay until the converted data has been read.
  *     @arg ADC_DelayLength_7Cycles: Delay length equal to 7 APB clock cycles
  *     @arg ADC_DelayLength_15Cycles: Delay length equal to 15 APB clock cycles	
  *     @arg ADC_DelayLength_31Cycles: Delay length equal to 31 APB clock cycles	
  *     @arg ADC_DelayLength_63Cycles: Delay length equal to 63 APB clock cycles	
  *     @arg ADC_DelayLength_127Cycles: Delay length equal to 127 APB clock cycles	
  *     @arg ADC_DelayLength_255Cycles: Delay length equal to 255 APB clock cycles	
  * @retval None
  */
void ADC_DelaySelectionConfig(ADC_TypeDef* ADCx, uint8_t ADC_DelayLength)
{
  uint32_t tmpreg = 0;
   
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_DELAY_LENGTH(ADC_DelayLength));

  /* Get the old register value */    
  tmpreg = ADCx->CR2;
  /* Clear the old delay length */
  tmpreg &= CR2_DELS_RESET;
  /* Set the delay length */
  tmpreg |= ADC_DelayLength;
  /* Store the new register value */
  ADCx->CR2 = tmpreg;

}

/**
  * @}
  */

/** @defgroup ADC_Group3 Analog Watchdog configuration functions
 *  @brief   Analog Watchdog configuration functions 
 *
@verbatim   
 ===============================================================================
                    Analog Watchdog configuration functions
 ===============================================================================  

  This section provides functions allowing to configure the Analog Watchdog
  (AWD) feature in the ADC.
  
  A typical configuration Analog Watchdog is done following these steps :
   1. the ADC guarded channel(s) is (are) selected using the 
      ADC_AnalogWatchdogSingleChannelConfig() function.
   2. The Analog watchdog lower and higher threshold are configured using the  
     ADC_AnalogWatchdogThresholdsConfig() function.
   3. The Analog watchdog is enabled and configured to enable the check, on one
      or more channels, using the  ADC_AnalogWatchdogCmd() function.

@endverbatim
  * @{
  */
  
/**
  * @brief  Enables or disables the analog watchdog on single/all regular
  *         or injected channels
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  ADC_AnalogWatchdog: the ADC analog watchdog configuration.
  *   This parameter can be one of the following values:
  *     @arg ADC_AnalogWatchdog_SingleRegEnable: Analog watchdog on a single 
  *          regular channel
  *     @arg ADC_AnalogWatchdog_SingleInjecEnable: Analog watchdog on a single 
  *          injected channel
  *     @arg ADC_AnalogWatchdog_SingleRegOrInjecEnable: Analog watchdog on a 
  *          single regular or injected channel       
  *     @arg ADC_AnalogWatchdog_AllRegEnable: Analog watchdog on all regular 
  *          channel
  *     @arg ADC_AnalogWatchdog_AllInjecEnable: Analog watchdog on all injected 
  *          channel
  *     @arg ADC_AnalogWatchdog_AllRegAllInjecEnable: Analog watchdog on all 
  *           regular and injected channels
  *     @arg ADC_AnalogWatchdog_None: No channel guarded by the analog watchdog
  * @retval None	  
  */
void ADC_AnalogWatchdogCmd(ADC_TypeDef* ADCx, uint32_t ADC_AnalogWatchdog)
{
  uint32_t tmpreg = 0;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_ANALOG_WATCHDOG(ADC_AnalogWatchdog));

  /* Get the old register value */
  tmpreg = ADCx->CR1;
  /* Clear AWDEN, JAWDEN and AWDSGL bits */   
  tmpreg &= CR1_AWDMODE_RESET;
  /* Set the analog watchdog enable mode */
  tmpreg |= ADC_AnalogWatchdog;
  /* Store the new register value */
  ADCx->CR1 = tmpreg;
}

/**
  * @brief  Configures the high and low thresholds of the analog watchdog. 
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  HighThreshold: the ADC analog watchdog High threshold value.
  *         This parameter must be a 12bit value.
  * @param  LowThreshold: the ADC analog watchdog Low threshold value.
  *         This parameter must be a 12bit value.
  * @retval None
  */
void ADC_AnalogWatchdogThresholdsConfig(ADC_TypeDef* ADCx, uint16_t HighThreshold,
                                        uint16_t LowThreshold)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_THRESHOLD(HighThreshold));
  assert_param(IS_ADC_THRESHOLD(LowThreshold));

  /* Set the ADCx high threshold */
  ADCx->HTR = HighThreshold;
  /* Set the ADCx low threshold */
  ADCx->LTR = LowThreshold;
}

/**
  * @brief  Configures the analog watchdog guarded single channel
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  ADC_Channel: the ADC channel to configure for the analog watchdog. 
  *   This parameter can be one of the following values:
  *     @arg ADC_Channel_0: ADC Channel0 selected
  *     @arg ADC_Channel_1: ADC Channel1 selected
  *     @arg ADC_Channel_2: ADC Channel2 selected
  *     @arg ADC_Channel_3: ADC Channel3 selected
  *     @arg ADC_Channel_4: ADC Channel4 selected
  *     @arg ADC_Channel_5: ADC Channel5 selected
  *     @arg ADC_Channel_6: ADC Channel6 selected
  *     @arg ADC_Channel_7: ADC Channel7 selected
  *     @arg ADC_Channel_8: ADC Channel8 selected
  *     @arg ADC_Channel_9: ADC Channel9 selected
  *     @arg ADC_Channel_10: ADC Channel10 selected
  *     @arg ADC_Channel_11: ADC Channel11 selected
  *     @arg ADC_Channel_12: ADC Channel12 selected
  *     @arg ADC_Channel_13: ADC Channel13 selected
  *     @arg ADC_Channel_14: ADC Channel14 selected
  *     @arg ADC_Channel_15: ADC Channel15 selected
  *     @arg ADC_Channel_16: ADC Channel16 selected
  *     @arg ADC_Channel_17: ADC Channel17 selected
  *     @arg ADC_Channel_18: ADC Channel18 selected
  *     @arg ADC_Channel_19: ADC Channel19 selected
  *     @arg ADC_Channel_20: ADC Channel20 selected
  *     @arg ADC_Channel_21: ADC Channel21 selected
  *     @arg ADC_Channel_22: ADC Channel22 selected
  *     @arg ADC_Channel_23: ADC Channel23 selected
  *     @arg ADC_Channel_24: ADC Channel24 selected
  *     @arg ADC_Channel_25: ADC Channel25 selected
  * @retval None
  */
void ADC_AnalogWatchdogSingleChannelConfig(ADC_TypeDef* ADCx, uint8_t ADC_Channel)
{
  uint32_t tmpreg = 0;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_CHANNEL(ADC_Channel));

  /* Get the old register value */
  tmpreg = ADCx->CR1;
  /* Clear the Analog watchdog channel select bits */
  tmpreg &= CR1_AWDCH_RESET;
  /* Set the Analog watchdog channel */
  tmpreg |= ADC_Channel;
  /* Store the new register value */
  ADCx->CR1 = tmpreg;
}

/**
  * @}
  */

/** @defgroup ADC_Group4 Temperature Sensor & Vrefint (Voltage Reference internal) management function
 *  @brief   Temperature Sensor & Vrefint (Voltage Reference internal) management function 
 *
@verbatim   
 ===============================================================================
  Temperature Sensor & Vrefint (Voltage Reference internal) management function
 ===============================================================================  

  This section provides a function allowing to enable/ disable the internal 
  connections between the ADC and the Temperature Sensor and the Vrefint source.
     
  A typical configuration to get the Temperature sensor and Vrefint channels 
  voltages or is done following these steps :
   1. Enable the internal connection of Temperature sensor and Vrefint sources 
      with the ADC channels using ADC_TempSensorVrefintCmd() function. 
   2. select the ADC_Channel_TempSensor and/or ADC_Channel_Vrefint using 
      ADC_RegularChannelConfig() or  ADC_InjectedChannelConfig() functions 
   3. Get the voltage values, using ADC_GetConversionValue() or  
      ADC_GetInjectedConversionValue().
 
@endverbatim
  * @{
  */
  
/**
  * @brief  Enables or disables the temperature sensor and Vrefint channel.
  * @param  NewState: new state of the temperature sensor and Vref int channels.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void ADC_TempSensorVrefintCmd(FunctionalState NewState)                
{
  /* Check the parameters */
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    /* Enable the temperature sensor and Vrefint channel*/
    ADC->CCR |= (uint32_t)ADC_CCR_TSVREFE;
  }
  else
  {
    /* Disable the temperature sensor and Vrefint channel*/
    ADC->CCR &= (uint32_t)(~ADC_CCR_TSVREFE);
  }
}

/**
  * @}
  */

/** @defgroup ADC_Group5 Regular Channels Configuration functions
 *  @brief   Regular Channels Configuration functions 
 *
@verbatim   
 ===============================================================================
                  Regular Channels Configuration functions
 ===============================================================================  

  This section provides functions allowing to manage the ADC regular channels,
  it is composed of 2 sub sections : 
  
  1. Configuration and management functions for regular channels: This subsection 
     provides functions allowing to configure the ADC regular channels :    
          - Configure the rank in the regular group sequencer for each channel
          - Configure the sampling time for each channel
          - select the conversion Trigger for regular channels
          - select the desired EOC event behavior configuration
          - Activate the continuous Mode  (*)
          - Activate the Discontinuous Mode 
     Please Note that the following features for regular channels are configurated
     using the ADC_Init() function : 
          - scan mode activation 
          - continuous mode activation (**) 
          - External trigger source  
          - External trigger edge 
          - number of conversion in the regular channels group sequencer.
     
     @note : (*) and (**) are performing the same configuration
     
  2. Get the conversion data: This subsection provides an important function in 
     the ADC peripheral since it returns the converted data of the current 
     regular channel. When the Conversion value is read, the EOC Flag is 
     automatically cleared.
  
@endverbatim
  * @{
  */

/**
  * @brief  Configures for the selected ADC regular channel its corresponding
  *         rank in the sequencer and its sampling time.
  * @param  ADCx: where x can be 1 to select the ADC peripheral.
  * @param  ADC_Channel: the ADC channel to configure. 
  *   This parameter can be one of the following values:
  *     @arg ADC_Channel_0: ADC Channel0 selected
  *     @arg ADC_Channel_1: ADC Channel1 selected
  *     @arg ADC_Channel_2: ADC Channel2 selected
  *     @arg ADC_Channel_3: ADC Channel3 selected
  *     @arg ADC_Channel_4: ADC Channel4 selected
  *     @arg ADC_Channel_5: ADC Channel5 selected
  *     @arg ADC_Channel_6: ADC Channel6 selected
  *     @arg ADC_Channel_7: ADC Channel7 selected
  *     @arg ADC_Channel_8: ADC Channel8 selected
  *     @arg ADC_Channel_9: ADC Channel9 selected
  *     @arg ADC_Channel_10: ADC Channel10 selected
  *     @arg ADC_Channel_11: ADC Channel11 selected
  *     @arg ADC_Channel_12: ADC Channel12 selected
  *     @arg ADC_Channel_13: ADC Channel13 selected
  *     @arg ADC_Channel_14: ADC Channel14 selected
  *     @arg ADC_Channel_15: ADC Channel15 selected
  *     @arg ADC_Channel_16: ADC Channel16 selected
  *     @arg ADC_Channel_17: ADC Channel17 selected
  *     @arg ADC_Channel_18: ADC Channel18 selected 
  *     @arg ADC_Channel_19: ADC Channel19 selected
  *     @arg ADC_Channel_20: ADC Channel20 selected
  *     @arg ADC_Channel_21: ADC Channel21 selected
  *     @arg ADC_Channel_22: ADC Channel22 selected
  *     @arg ADC_Channel_23: ADC Channel23 selected
  *     @arg ADC_Channel_24: ADC Channel24 selected
  *     @arg ADC_Channel_25: ADC Channel25 selected
  * @param  Rank: The rank in the regular group sequencer. This parameter
  *               must be between 1 to 26.
  * @param  ADC_SampleTime: The sample time value to be set for the selected 
  *         channel. 
  *   This parameter can be one of the following values:
  *     @arg ADC_SampleTime_4Cycles: Sample time equal to 4 cycles  
  *     @arg ADC_SampleTime_9Cycles: Sample time equal to 9 cycles
  *     @arg ADC_SampleTime_16Cycles: Sample time equal to 16 cycles
  *     @arg ADC_SampleTime_24Cycles: Sample time equal to 24 cycles	
  *     @arg ADC_SampleTime_48Cycles: Sample time equal to 48 cycles	
  *     @arg ADC_SampleTime_96Cycles: Sample time equal to 96 cycles	
  *     @arg ADC_SampleTime_192Cycles: Sample time equal to 192 cycles	
  *     @arg ADC_SampleTime_384Cycles: Sample time equal to 384 cycles	
  * @retval None
  */
void ADC_RegularChannelConfig(ADC_TypeDef* ADCx, uint8_t ADC_Channel, uint8_t Rank, uint8_t ADC_SampleTime)
{
  uint32_t tmpreg1 = 0, tmpreg2 = 0;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_CHANNEL(ADC_Channel));
  assert_param(IS_ADC_REGULAR_RANK(Rank));
  assert_param(IS_ADC_SAMPLE_TIME(ADC_SampleTime));

  /* if ADC_Channel_20 ... ADC_Channel_25 is selected */
  if (ADC_Channel > ADC_Channel_19)
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SMPR1;
    /* Calculate the mask to clear */
    tmpreg2 = SMPR1_SMP_SET << (3 * (ADC_Channel - 20));
    /* Clear the old sample time */
    tmpreg1 &= ~tmpreg2;
    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_SampleTime << (3 * (ADC_Channel - 20));
    /* Set the new sample time */
    tmpreg1 |= tmpreg2;
    /* Store the new register value */
    ADCx->SMPR1 = tmpreg1;
  }
  
   /* if ADC_Channel_10 ... ADC_Channel_19 is selected */
  else if (ADC_Channel > ADC_Channel_9)
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SMPR2;
    /* Calculate the mask to clear */
    tmpreg2 = SMPR2_SMP_SET << (3 * (ADC_Channel - 10));
    /* Clear the old sample time */
    tmpreg1 &= ~tmpreg2;
    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_SampleTime << (3 * (ADC_Channel - 10));
    /* Set the new sample time */
    tmpreg1 |= tmpreg2;
    /* Store the new register value */
    ADCx->SMPR2 = tmpreg1;
  }
  
  else /* ADC_Channel include in ADC_Channel_[0..9] */
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SMPR3;
    /* Calculate the mask to clear */
    tmpreg2 = SMPR3_SMP_SET << (3 * ADC_Channel);
    /* Clear the old sample time */
    tmpreg1 &= ~tmpreg2;
    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_SampleTime << (3 * ADC_Channel);
    /* Set the new sample time */
    tmpreg1 |= tmpreg2;
    /* Store the new register value */
    ADCx->SMPR3 = tmpreg1;
  }
  /* For Rank 1 to 6 */
  if (Rank < 7)
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SQR5;
    /* Calculate the mask to clear */
    tmpreg2 = SQR5_SQ_SET << (5 * (Rank - 1));
    /* Clear the old SQx bits for the selected rank */
    tmpreg1 &= ~tmpreg2;
    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_Channel << (5 * (Rank - 1));
    /* Set the SQx bits for the selected rank */
    tmpreg1 |= tmpreg2;
    /* Store the new register value */
    ADCx->SQR5 = tmpreg1;
  }
  /* For Rank 7 to 12 */
  else if (Rank < 13)
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SQR4;
    /* Calculate the mask to clear */
    tmpreg2 = SQR4_SQ_SET << (5 * (Rank - 7));
    /* Clear the old SQx bits for the selected rank */
    tmpreg1 &= ~tmpreg2;
    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_Channel << (5 * (Rank - 7));
    /* Set the SQx bits for the selected rank */
    tmpreg1 |= tmpreg2;
    /* Store the new register value */
    ADCx->SQR4 = tmpreg1;
  }  
  /* For Rank 13 to 18 */
  else if (Rank < 19)
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SQR3;
    /* Calculate the mask to clear */
    tmpreg2 = SQR3_SQ_SET << (5 * (Rank - 13));
    /* Clear the old SQx bits for the selected rank */
    tmpreg1 &= ~tmpreg2;
    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_Channel << (5 * (Rank - 13));
    /* Set the SQx bits for the selected rank */
    tmpreg1 |= tmpreg2;
    /* Store the new register value */
    ADCx->SQR3 = tmpreg1;
  }
    
  /* For Rank 19 to 24 */
  else if (Rank < 25)
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SQR2;
    /* Calculate the mask to clear */
    tmpreg2 = SQR2_SQ_SET << (5 * (Rank - 19));
    /* Clear the old SQx bits for the selected rank */
    tmpreg1 &= ~tmpreg2;
    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_Channel << (5 * (Rank - 19));
    /* Set the SQx bits for the selected rank */
    tmpreg1 |= tmpreg2;
    /* Store the new register value */
    ADCx->SQR2 = tmpreg1;
  }   
  
  /* For Rank 25 to 27 */
  else
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SQR1;
    /* Calculate the mask to clear */
    tmpreg2 = SQR1_SQ_SET << (5 * (Rank - 25));
    /* Clear the old SQx bits for the selected rank */
    tmpreg1 &= ~tmpreg2;
    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_Channel << (5 * (Rank - 25));
    /* Set the SQx bits for the selected rank */
    tmpreg1 |= tmpreg2;
    /* Store the new register value */
    ADCx->SQR1 = tmpreg1;
  }
}

/**
  * @brief  Enables the selected ADC software start conversion of the regular channels.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @retval None
  */
void ADC_SoftwareStartConv(ADC_TypeDef* ADCx)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));

  /* Enable the selected ADC conversion for regular group */
  ADCx->CR2 |= (uint32_t)ADC_CR2_SWSTART;
}

/**
  * @brief  Gets the selected ADC Software start regular conversion Status.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @retval The new state of ADC software start conversion (SET or RESET).
  */
FlagStatus ADC_GetSoftwareStartConvStatus(ADC_TypeDef* ADCx)
{
  FlagStatus bitstatus = RESET;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));

  /* Check the status of SWSTART bit */
  if ((ADCx->CR2 & ADC_CR2_SWSTART) != (uint32_t)RESET)
  {
    /* SWSTART bit is set */
    bitstatus = SET;
  }
  else
  {
    /* SWSTART bit is reset */
    bitstatus = RESET;
  }
  /* Return the SWSTART bit status */
  return  bitstatus;
}

/**
  * @brief  Enables or disables the EOC on each regular channel conversion
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  NewState: new state of the selected ADC EOC flag rising
  *    This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void ADC_EOCOnEachRegularChannelCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    /* Enable the selected ADC EOC rising on each regular channel conversion */
    ADCx->CR2 |= ADC_CR2_EOCS;
  }
  else
  {
    /* Disable the selected ADC EOC rising on each regular channel conversion */
    ADCx->CR2 &= (uint32_t)~ADC_CR2_EOCS;
  }
}

/**
  * @brief  Enables or disables the ADC continuous conversion mode 
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  NewState: new state of the selected ADC continuous conversion mode
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void ADC_ContinuousModeCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    /* Enable the selected ADC continuous conversion mode */
    ADCx->CR2 |= (uint32_t)ADC_CR2_CONT;
  }
  else
  {
    /* Disable the selected ADC continuous conversion mode */
    ADCx->CR2 &= (uint32_t)(~ADC_CR2_CONT);
  }
}

/**
  * @brief  Configures the discontinuous mode for the selected ADC regular
  *         group channel.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  Number: specifies the discontinuous mode regular channel count value. 
  *         This number must be between 1 and 8.
  * @retval None
  */
void ADC_DiscModeChannelCountConfig(ADC_TypeDef* ADCx, uint8_t Number)
{
  uint32_t tmpreg1 = 0;
  uint32_t tmpreg2 = 0;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_REGULAR_DISC_NUMBER(Number));

  /* Get the old register value */
  tmpreg1 = ADCx->CR1;
  /* Clear the old discontinuous mode channel count */
  tmpreg1 &= CR1_DISCNUM_RESET;
  /* Set the discontinuous mode channel count */
  tmpreg2 = Number - 1;
  tmpreg1 |= tmpreg2 << 13;
  /* Store the new register value */
  ADCx->CR1 = tmpreg1;
}

/**
  * @brief  Enables or disables the discontinuous mode on regular group
  *         channel for the specified ADC
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  NewState: new state of the selected ADC discontinuous mode on regular 
  *         group channel. 
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void ADC_DiscModeCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    /* Enable the selected ADC regular discontinuous mode */
    ADCx->CR1 |= (uint32_t)ADC_CR1_DISCEN;
  }
  else
  {
    /* Disable the selected ADC regular discontinuous mode */
    ADCx->CR1 &= (uint32_t)(~ADC_CR1_DISCEN);
  }
}

/**
  * @brief  Returns the last ADCx conversion result data for regular channel.  
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @retval The Data conversion value.
  */
uint16_t ADC_GetConversionValue(ADC_TypeDef* ADCx)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));

  /* Return the selected ADC conversion value */
  return (uint16_t) ADCx->DR;
}

/**
  * @}
  */

/** @defgroup ADC_Group6 Regular Channels DMA Configuration functions
 *  @brief   Regular Channels DMA Configuration functions 
 *
@verbatim   
 ===============================================================================
                   Regular Channels DMA Configuration functions
 ===============================================================================  

  This section provides functions allowing to configure the DMA for ADC regular 
  channels.
  Since converted regular channel values are stored into a unique data register, 
  it is useful to use DMA for conversion of more than one regular channel. This 
  avoids the loss of the data already stored in the ADC Data register. 
  
  When the DMA mode is enabled (using the ADC_DMACmd() function), after each
  conversion of a regular channel, a DMA request is generated.
  
  Depending on the "DMA disable selection" configuration (using the 
  ADC_DMARequestAfterLastTransferCmd() function), at the end of the last DMA 
  transfer, two possibilities are allowed:
  - No new DMA request is issued to the DMA controller (feature DISABLED) 
  - Requests can continue to be generated (feature ENABLED).

@endverbatim
  * @{
  */

/**
  * @brief  Enables or disables the specified ADC DMA request.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  NewState: new state of the selected ADC DMA transfer.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void ADC_DMACmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_ADC_DMA_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    /* Enable the selected ADC DMA request */
    ADCx->CR2 |= (uint32_t)ADC_CR2_DMA;
  }
  else
  {
    /* Disable the selected ADC DMA request */
    ADCx->CR2 &= (uint32_t)(~ADC_CR2_DMA);
  }
}


/**
  * @brief  Enables or disables the ADC DMA request after last transfer (Single-ADC mode)  
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  NewState: new state of the selected ADC EOC flag rising
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void ADC_DMARequestAfterLastTransferCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    /* Enable the selected ADC DMA request after last transfer */
    ADCx->CR2 |= ADC_CR2_DDS;
  }
  else
  {
    /* Disable the selected ADC DMA request after last transfer */
    ADCx->CR2 &= (uint32_t)~ADC_CR2_DDS;
  }
}

/**
  * @}
  */

/** @defgroup ADC_Group7 Injected channels Configuration functions
 *  @brief   Injected channels Configuration functions 
 *
@verbatim   
 ===============================================================================
                     Injected channels Configuration functions
 ===============================================================================  

  This section provide functions allowing to configure the ADC Injected channels,
  it is composed of 2 sub sections : 
    
  1. Configuration functions for Injected channels: This subsection provides 
     functions allowing to configure the ADC injected channels :    
    - Configure the rank in the injected group sequencer for each channel
    - Configure the sampling time for each channel    
    - Activate the Auto injected Mode  
    - Activate the Discontinuous Mode 
    - scan mode activation  
    - External/software trigger source   
    - External trigger edge 
    - injected channels sequencer.
    
   2. Get the Specified Injected channel conversion data: This subsection 
      provides an important function in the ADC peripheral since it returns the 
      converted data of the specific injected channel.

@endverbatim
  * @{
  */ 

/**
  * @brief  Configures for the selected ADC injected channel its corresponding
  *         rank in the sequencer and its sample time.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  ADC_Channel: the ADC channel to configure. 
  *   This parameter can be one of the following values:
  *     @arg ADC_Channel_0: ADC Channel0 selected
  *     @arg ADC_Channel_1: ADC Channel1 selected
  *     @arg ADC_Channel_2: ADC Channel2 selected
  *     @arg ADC_Channel_3: ADC Channel3 selected
  *     @arg ADC_Channel_4: ADC Channel4 selected
  *     @arg ADC_Channel_5: ADC Channel5 selected
  *     @arg ADC_Channel_6: ADC Channel6 selected
  *     @arg ADC_Channel_7: ADC Channel7 selected
  *     @arg ADC_Channel_8: ADC Channel8 selected
  *     @arg ADC_Channel_9: ADC Channel9 selected
  *     @arg ADC_Channel_10: ADC Channel10 selected
  *     @arg ADC_Channel_11: ADC Channel11 selected
  *     @arg ADC_Channel_12: ADC Channel12 selected
  *     @arg ADC_Channel_13: ADC Channel13 selected
  *     @arg ADC_Channel_14: ADC Channel14 selected
  *     @arg ADC_Channel_15: ADC Channel15 selected
  *     @arg ADC_Channel_16: ADC Channel16 selected
  *     @arg ADC_Channel_17: ADC Channel17 selected
  *     @arg ADC_Channel_18: ADC Channel18 selected 
  *     @arg ADC_Channel_19: ADC Channel19 selected
  *     @arg ADC_Channel_20: ADC Channel20 selected
  *     @arg ADC_Channel_21: ADC Channel21 selected
  *     @arg ADC_Channel_22: ADC Channel22 selected
  *     @arg ADC_Channel_23: ADC Channel23 selected
  *     @arg ADC_Channel_24: ADC Channel24 selected
  *     @arg ADC_Channel_25: ADC Channel25 selected
  * @param  Rank: The rank in the injected group sequencer. This parameter
  *         must be between 1 to 4.
  * @param  ADC_SampleTime: The sample time value to be set for the selected 
  *         channel. This parameter can be one of the following values:
  *     @arg ADC_SampleTime_4Cycles: Sample time equal to 4 cycles  
  *     @arg ADC_SampleTime_9Cycles: Sample time equal to 9 cycles
  *     @arg ADC_SampleTime_16Cycles: Sample time equal to 16 cycles
  *     @arg ADC_SampleTime_24Cycles: Sample time equal to 24 cycles	
  *     @arg ADC_SampleTime_48Cycles: Sample time equal to 48 cycles	
  *     @arg ADC_SampleTime_96Cycles: Sample time equal to 96 cycles	
  *     @arg ADC_SampleTime_192Cycles: Sample time equal to 192 cycles	
  *     @arg ADC_SampleTime_384Cycles: Sample time equal to 384 cycles	
  * @retval None
  */
void ADC_InjectedChannelConfig(ADC_TypeDef* ADCx, uint8_t ADC_Channel, uint8_t Rank, uint8_t ADC_SampleTime)
{
  uint32_t tmpreg1 = 0, tmpreg2 = 0, tmpreg3 = 0;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_CHANNEL(ADC_Channel));
  assert_param(IS_ADC_INJECTED_RANK(Rank));
  assert_param(IS_ADC_SAMPLE_TIME(ADC_SampleTime));
  
  /* if ADC_Channel_20 ... ADC_Channel_25 is selected */
  if (ADC_Channel > ADC_Channel_19)
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SMPR1;
    /* Calculate the mask to clear */
    tmpreg2 = SMPR1_SMP_SET << (3 * (ADC_Channel - 20));
    /* Clear the old sample time */
    tmpreg1 &= ~tmpreg2;
    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_SampleTime << (3 * (ADC_Channel - 20));
    /* Set the new sample time */
    tmpreg1 |= tmpreg2;
    /* Store the new register value */
    ADCx->SMPR1 = tmpreg1;
  }
  
   /* if ADC_Channel_10 ... ADC_Channel_19 is selected */
  else if (ADC_Channel > ADC_Channel_9)
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SMPR2;
    /* Calculate the mask to clear */
    tmpreg2 = SMPR2_SMP_SET << (3 * (ADC_Channel - 10));
    /* Clear the old sample time */
    tmpreg1 &= ~tmpreg2;
    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_SampleTime << (3 * (ADC_Channel - 10));
    /* Set the new sample time */
    tmpreg1 |= tmpreg2;
    /* Store the new register value */
    ADCx->SMPR2 = tmpreg1;
  }
  
  else /* ADC_Channel include in ADC_Channel_[0..9] */
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SMPR3;
    /* Calculate the mask to clear */
    tmpreg2 = SMPR3_SMP_SET << (3 * ADC_Channel);
    /* Clear the old sample time */
    tmpreg1 &= ~tmpreg2;
    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_SampleTime << (3 * ADC_Channel);
    /* Set the new sample time */
    tmpreg1 |= tmpreg2;
    /* Store the new register value */
    ADCx->SMPR3 = tmpreg1;
  }
  
  /* Rank configuration */
  /* Get the old register value */
  tmpreg1 = ADCx->JSQR;
  /* Get JL value: Number = JL+1 */
  tmpreg3 =  (tmpreg1 & JSQR_JL_SET)>> 20;
  /* Calculate the mask to clear: ((Rank-1)+(4- (JL+1))) */ 
  tmpreg2 = JSQR_JSQ_SET << (5 * (uint8_t)((Rank + 3) - (tmpreg3 + 1)));
  /* Clear the old JSQx bits for the selected rank */
  tmpreg1 &= ~tmpreg2;
  /* Calculate the mask to set: ((Rank-1)+(4- (JL+1))) */ 
  tmpreg2 = (uint32_t)ADC_Channel << (5 * (uint8_t)((Rank + 3) - (tmpreg3 + 1)));
  /* Set the JSQx bits for the selected rank */
  tmpreg1 |= tmpreg2;
  /* Store the new register value */
  ADCx->JSQR = tmpreg1;
}

/**
  * @brief  Configures the sequencer length for injected channels
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  Length: The sequencer length. 
  *         This parameter must be a number between 1 to 4.
  * @retval None
  */
void ADC_InjectedSequencerLengthConfig(ADC_TypeDef* ADCx, uint8_t Length)
{
  uint32_t tmpreg1 = 0;
  uint32_t tmpreg2 = 0;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_INJECTED_LENGTH(Length));
  
  /* Get the old register value */
  tmpreg1 = ADCx->JSQR;
  /* Clear the old injected sequence length JL bits */
  tmpreg1 &= JSQR_JL_RESET;
  /* Set the injected sequence length JL bits */
  tmpreg2 = Length - 1; 
  tmpreg1 |= tmpreg2 << 20;
  /* Store the new register value */
  ADCx->JSQR = tmpreg1;
}

/**
  * @brief  Set the injected channels conversion value offset
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  ADC_InjectedChannel: the ADC injected channel to set its offset. 
  *   This parameter can be one of the following values:
  *     @arg ADC_InjectedChannel_1: Injected Channel1 selected
  *     @arg ADC_InjectedChannel_2: Injected Channel2 selected
  *     @arg ADC_InjectedChannel_3: Injected Channel3 selected
  *     @arg ADC_InjectedChannel_4: Injected Channel4 selected
  * @param  Offset: the offset value for the selected ADC injected channel
  *         This parameter must be a 12bit value.
  * @retval None
  */
void ADC_SetInjectedOffset(ADC_TypeDef* ADCx, uint8_t ADC_InjectedChannel, uint16_t Offset)
{
  __IO uint32_t tmp = 0;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_INJECTED_CHANNEL(ADC_InjectedChannel));
  assert_param(IS_ADC_OFFSET(Offset));  
  
  tmp = (uint32_t)ADCx;
  tmp += ADC_InjectedChannel;
  
  /* Set the selected injected channel data offset */
  *(__IO uint32_t *) tmp = (uint32_t)Offset;
}

/**
  * @brief  Configures the ADCx external trigger for injected channels conversion.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  ADC_ExternalTrigInjecConv: specifies the ADC trigger to start injected 
  *    conversion. This parameter can be one of the following values:                    
  *     @arg ADC_ExternalTrigInjecConv_T9_CC1: Timer9 capture compare1 selected 
  *     @arg ADC_ExternalTrigInjecConv_T9_TRGO: Timer9 TRGO event selected 
  *     @arg ADC_ExternalTrigInjecConv_T2_TRGO: Timer2 TRGO event selected 
  *     @arg ADC_ExternalTrigInjecConv_T2_CC1: Timer2 capture compare1 selected
  *     @arg ADC_ExternalTrigInjecConv_T3_CC4: Timer3 capture compare4 selected 
  *     @arg ADC_ExternalTrigInjecConv_T4_TRGO: Timer4 TRGO event selected 
  *     @arg ADC_ExternalTrigInjecConv_T4_CC1: Timer4 capture compare1 selected                       
  *     @arg ADC_ExternalTrigInjecConv_T4_CC2: Timer4 capture compare2 selected 
  *     @arg ADC_ExternalTrigInjecConv_T4_CC3: Timer4 capture compare3 selected                        
  *     @arg ADC_ExternalTrigInjecConv_T10_CC1: Timer10 capture compare1 selected
  *     @arg ADC_ExternalTrigInjecConv_T7_TRGO: Timer7 TRGO event selected                         
  *     @arg ADC_ExternalTrigInjecConv_Ext_IT15: External interrupt line 15 event selected                          
  * @retval None
  */
void ADC_ExternalTrigInjectedConvConfig(ADC_TypeDef* ADCx, uint32_t ADC_ExternalTrigInjecConv)
{
  uint32_t tmpreg = 0;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_EXT_INJEC_TRIG(ADC_ExternalTrigInjecConv));

  /* Get the old register value */
  tmpreg = ADCx->CR2;
  /* Clear the old external event selection for injected group */
  tmpreg &= CR2_JEXTSEL_RESET;
  /* Set the external event selection for injected group */
  tmpreg |= ADC_ExternalTrigInjecConv;
  /* Store the new register value */
  ADCx->CR2 = tmpreg;
}

/**
  * @brief  Configures the ADCx external trigger edge for injected channels conversion.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  ADC_ExternalTrigInjecConvEdge: specifies the ADC external trigger
  *         edge to start injected conversion. 
  *   This parameter can be one of the following values:
  *     @arg ADC_ExternalTrigConvEdge_None: external trigger disabled for 
  *          injected conversion
  *     @arg ADC_ExternalTrigConvEdge_Rising: detection on rising edge
  *     @arg ADC_ExternalTrigConvEdge_Falling: detection on falling edge
  *     @arg ADC_External ADC_ExternalTrigConvEdge_RisingFalling: detection on 
  *          both rising and falling edge
  * @retval None
  */
void ADC_ExternalTrigInjectedConvEdgeConfig(ADC_TypeDef* ADCx, uint32_t ADC_ExternalTrigInjecConvEdge)
{
  uint32_t tmpreg = 0;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_EXT_INJEC_TRIG_EDGE(ADC_ExternalTrigInjecConvEdge));

  /* Get the old register value */
  tmpreg = ADCx->CR2;
  /* Clear the old external trigger edge for injected group */
  tmpreg &= CR2_JEXTEN_RESET;
  /* Set the new external trigger edge for injected group */
  tmpreg |= ADC_ExternalTrigInjecConvEdge;
  /* Store the new register value */
  ADCx->CR2 = tmpreg;
}

/**
  * @brief  Enables the selected ADC software start conversion of the injected 
  *         channels.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @retval None
  */
void ADC_SoftwareStartInjectedConv(ADC_TypeDef* ADCx)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  /* Enable the selected ADC conversion for injected group */
  ADCx->CR2 |= (uint32_t)ADC_CR2_JSWSTART;
}

/**
  * @brief  Gets the selected ADC Software start injected conversion Status.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @retval The new state of ADC software start injected conversion (SET or RESET).
  */
FlagStatus ADC_GetSoftwareStartInjectedConvCmdStatus(ADC_TypeDef* ADCx)
{
  FlagStatus bitstatus = RESET;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));

  /* Check the status of JSWSTART bit */
  if ((ADCx->CR2 & ADC_CR2_JSWSTART) != (uint32_t)RESET)
  {
    /* JSWSTART bit is set */
    bitstatus = SET;
  }
  else
  {
    /* JSWSTART bit is reset */
    bitstatus = RESET;
  }
  /* Return the JSWSTART bit status */
  return  bitstatus;
}

/**
  * @brief  Enables or disables the selected ADC automatic injected group
  *         conversion after regular one.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  NewState: new state of the selected ADC auto injected
  *         conversion. 
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void ADC_AutoInjectedConvCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    /* Enable the selected ADC automatic injected group conversion */
    ADCx->CR1 |= (uint32_t)ADC_CR1_JAUTO;
  }
  else
  {
    /* Disable the selected ADC automatic injected group conversion */
    ADCx->CR1 &= (uint32_t)(~ADC_CR1_JAUTO);
  }
}

/**
  * @brief  Enables or disables the discontinuous mode for injected group
  *         channel for the specified ADC
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  NewState: new state of the selected ADC discontinuous mode
  *         on injected group channel. This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void ADC_InjectedDiscModeCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    /* Enable the selected ADC injected discontinuous mode */
    ADCx->CR1 |= (uint32_t)ADC_CR1_JDISCEN;
  }
  else
  {
    /* Disable the selected ADC injected discontinuous mode */
    ADCx->CR1 &= (uint32_t)(~ADC_CR1_JDISCEN);
  }
}

/**
  * @brief  Returns the ADC injected channel conversion result
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  ADC_InjectedChannel: the converted ADC injected channel.
  *   This parameter can be one of the following values:
  *     @arg ADC_InjectedChannel_1: Injected Channel1 selected
  *     @arg ADC_InjectedChannel_2: Injected Channel2 selected
  *     @arg ADC_InjectedChannel_3: Injected Channel3 selected
  *     @arg ADC_InjectedChannel_4: Injected Channel4 selected
  * @retval The Data conversion value.
  */
uint16_t ADC_GetInjectedConversionValue(ADC_TypeDef* ADCx, uint8_t ADC_InjectedChannel)
{
  __IO uint32_t tmp = 0;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_INJECTED_CHANNEL(ADC_InjectedChannel));

  tmp = (uint32_t)ADCx;
  tmp += ADC_InjectedChannel + JDR_OFFSET;
  
  /* Returns the selected injected channel conversion data value */
  return (uint16_t) (*(__IO uint32_t*)  tmp); 
}

/**
  * @}
  */

/** @defgroup ADC_Group8 Interrupts and flags management functions
 *  @brief   Interrupts and flags management functions
 *
@verbatim   
 ===============================================================================
                   Interrupts and flags management functions
 ===============================================================================  

  This section provides functions allowing to configure the ADC Interrupts and get 
  the status and clear flags and Interrupts pending bits.
  
  The ADC provide 4 Interrupts sources and 9 Flags which can be divided into 3 groups:
  
  I. Flags and Interrupts for ADC regular channels
  =================================================
  Flags :
  ---------- 
     1. ADC_FLAG_OVR : Overrun detection when regular converted data are lost

     2. ADC_FLAG_EOC : Regular channel end of conversion�+ to indicate (depending 
              on EOCS bit, managed by ADC_EOCOnEachRegularChannelCmd() ) the end of :
               ==> a regular CHANNEL conversion 
               ==> sequence of regular GROUP conversions .

     3. ADC_FLAG_STRT: Regular channel start�+ to indicate when regular CHANNEL 
              conversion starts.
              
     4. ADC_FLAG_RCNR: Regular channel not ready+ to indicate if a new regular 
              conversion can be launched
     
  Interrupts :
  ------------
     1. ADC_IT_OVR 
     2. ADC_IT_EOC 
  
  
  II. Flags and Interrupts for ADC Injected channels
  =================================================
  Flags :
  ---------- 
     1. ADC_FLAG_JEOC : Injected channel end of conversion+ to indicate at 
               the end of injected GROUP conversion  
              
     2. ADC_FLAG_JSTRT: Injected channel start�+  to indicate hardware when 
               injected GROUP conversion starts.

     3. ADC_FLAG_JCNR: Injected channel not ready�+ to indicate if a new 
               injected conversion can be launched.

  Interrupts :
  ------------
     1. ADC_IT_JEOC     

  III. General Flags and Interrupts for the ADC
  ================================================= 
  Flags :
  ---------- 
     1. ADC_FLAG_AWD: Analog watchdog�+ to indicate if the converted voltage 
              crosses the programmed thresholds values.
              
     2. ADC_FLAG_ADONS: ADC ON status�+ to indicate if the ADC is ready to convert.
 
  Interrupts :
  ------------
     1. ADC_IT_AWD 

@endverbatim
  * @{
  */ 

/**
  * @brief  Enables or disables the specified ADC interrupts.
  * @param  ADCx: where x can be 1 to select the ADC peripheral.
  * @param  ADC_IT: specifies the ADC interrupt sources to be enabled or disabled. 
  *   This parameter can be one of the following values:
  *     @arg ADC_IT_EOC: End of conversion interrupt 
  *     @arg ADC_IT_AWD: Analog watchdog interrupt 
  *     @arg ADC_IT_JEOC: End of injected conversion interrupt 
  *     @arg ADC_IT_OVR: overrun interrupt                        
  * @param  NewState: new state of the specified ADC interrupts.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void ADC_ITConfig(ADC_TypeDef* ADCx, uint16_t ADC_IT, FunctionalState NewState)  
{
  uint32_t itmask = 0;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  assert_param(IS_ADC_IT(ADC_IT)); 

  /* Get the ADC IT index */
  itmask = (uint8_t)ADC_IT;
  itmask = (uint32_t)0x01 << itmask;    

  if (NewState != DISABLE)
  {
    /* Enable the selected ADC interrupts */
    ADCx->CR1 |= itmask;
  }
  else
  {
    /* Disable the selected ADC interrupts */
    ADCx->CR1 &= (~(uint32_t)itmask);
  }
}

/**
  * @brief  Checks whether the specified ADC flag is set or not.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  ADC_FLAG: specifies the flag to check. 
  *   This parameter can be one of the following values:
  *     @arg ADC_FLAG_AWD: Analog watchdog flag
  *     @arg ADC_FLAG_EOC: End of conversion flag
  *     @arg ADC_FLAG_JEOC: End of injected group conversion flag
  *     @arg ADC_FLAG_JSTRT: Start of injected group conversion flag
  *     @arg ADC_FLAG_STRT: Start of regular group conversion flag
  *     @arg ADC_FLAG_OVR: Overrun flag   
  *     @arg ADC_FLAG_ADONS: ADC ON status 
  *     @arg ADC_FLAG_RCNR: Regular channel not ready 
  *     @arg ADC_FLAG_JCNR: Injected channel not ready 
  * @retval The new state of ADC_FLAG (SET or RESET).
  */
FlagStatus ADC_GetFlagStatus(ADC_TypeDef* ADCx, uint16_t ADC_FLAG)
{
  FlagStatus bitstatus = RESET;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_GET_FLAG(ADC_FLAG));

  /* Check the status of the specified ADC flag */
  if ((ADCx->SR & ADC_FLAG) != (uint8_t)RESET)
  {
    /* ADC_FLAG is set */
    bitstatus = SET;
  }
  else
  {
    /* ADC_FLAG is reset */
    bitstatus = RESET;
  }
  /* Return the ADC_FLAG status */
  return  bitstatus;
}

/**
  * @brief  Clears the ADCx's pending flags.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  ADC_FLAG: specifies the flag to clear. 
  *   This parameter can be any combination of the following values:
  *     @arg ADC_FLAG_AWD: Analog watchdog flag
  *     @arg ADC_FLAG_EOC: End of conversion flag
  *     @arg ADC_FLAG_JEOC: End of injected group conversion flag
  *     @arg ADC_FLAG_JSTRT: Start of injected group conversion flag
  *     @arg ADC_FLAG_STRT: Start of regular group conversion flag
  *     @arg ADC_FLAG_OVR: overrun flag                                              
  * @retval None
  */
void ADC_ClearFlag(ADC_TypeDef* ADCx, uint16_t ADC_FLAG)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_CLEAR_FLAG(ADC_FLAG));

  /* Clear the selected ADC flags */
  ADCx->SR = ~(uint32_t)ADC_FLAG;
}

/**
  * @brief  Checks whether the specified ADC interrupt has occurred or not.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  ADC_IT: specifies the ADC interrupt source to check. 
  *   This parameter can be one of the following values:
  *     @arg ADC_IT_EOC: End of conversion interrupt 
  *     @arg ADC_IT_AWD: Analog watchdog interrupt 
  *     @arg ADC_IT_JEOC: End of injected conversion interrupt 
  *     @arg ADC_IT_OVR: Overrun interrupt                         
  * @retval The new state of ADC_IT (SET or RESET).
  */
ITStatus ADC_GetITStatus(ADC_TypeDef* ADCx, uint16_t ADC_IT)
{
  ITStatus bitstatus = RESET;
  uint32_t itmask = 0, enablestatus = 0;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_IT(ADC_IT));

  /* Get the ADC IT index */
  itmask = (uint32_t)((uint32_t)ADC_IT >> 8);

  /* Get the ADC_IT enable bit status */
  enablestatus = (ADCx->CR1 & ((uint32_t)0x01 << (uint8_t)ADC_IT)); 

  /* Check the status of the specified ADC interrupt */
  if (((uint32_t)(ADCx->SR & (uint32_t)itmask) != (uint32_t)RESET) && (enablestatus != (uint32_t)RESET))
  {                                                         
    /* ADC_IT is set */
    bitstatus = SET;
  }
  else
  {
    /* ADC_IT is reset */
    bitstatus = RESET;
  }
  /* Return the ADC_IT status */
  return  bitstatus;
}

/**
  * @brief  Clears the ADCx�s interrupt pending bits.
  * @param  ADCx: where x can be 1 to select the ADC1 peripheral.
  * @param  ADC_IT: specifies the ADC interrupt pending bit to clear.
  *   This parameter can be one of the following values:
  *     @arg ADC_IT_EOC: End of conversion interrupt 
  *     @arg ADC_IT_AWD: Analog watchdog interrupt 
  *     @arg ADC_IT_JEOC: End of injected conversion interrupt 
  *     @arg ADC_IT_OVR: Overrun interrupt                          
  * @retval None
  */
void ADC_ClearITPendingBit(ADC_TypeDef* ADCx, uint16_t ADC_IT)
{
  uint8_t itmask = 0;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_IT(ADC_IT)); 

  /* Get the ADC IT index */
  itmask = (uint8_t)(ADC_IT >> 8);

  /* Clear the selected ADC interrupt pending bits */
  ADCx->SR = ~(uint32_t)itmask;
}

/**
  * @}
  */

/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/

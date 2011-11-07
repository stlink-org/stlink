/**
  ******************************************************************************
  * @file    stm32l1xx_spi.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    31-December-2010
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of the Serial peripheral interface (SPI):           
  *           - Initialization and Configuration
  *           - Data transfers functions
  *           - Hardware CRC Calculation
  *           - DMA transfers management
  *           - Interrupts and flags management 
  *           
  *  @verbatim
  *          
  *          The I2S feature is not implemented in STM32L1xx Ultra Low Power
  *          Medium-density devices and will be supported in future products.
  *                    
  *          ===================================================================
  *                                 How to use this driver
  *          ===================================================================
  *          1. Enable peripheral clock using RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE)
  *             function for SPI1 or using RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE)
  *             function for SPI2.
  *
  *          2. Enable SCK, MOSI, MISO and NSS GPIO clocks using RCC_AHBPeriphClockCmd()
  *             function. 
  *
  *          3. Peripherals alternate function: 
  *                 - Connect the pin to the desired peripherals' Alternate 
  *                   Function (AF) using GPIO_PinAFConfig() function
  *                 - Configure the desired pin in alternate function by:
  *                   GPIO_InitStruct->GPIO_Mode = GPIO_Mode_AF
  *                 - Select the type, pull-up/pull-down and output speed via 
  *                   GPIO_PuPd, GPIO_OType and GPIO_Speed members
  *                 - Call GPIO_Init() function
  *        
  *          4. Program the Polarity, Phase, First Data, Baud Rate Prescaler, Slave 
  *             Management, Peripheral Mode and CRC Polynomial values using the SPI_Init()
  *             function.
  *
  *          5. Enable the NVIC and the corresponding interrupt using the function 
  *             SPI_ITConfig() if you need to use interrupt mode. 
  *
  *          6. When using the DMA mode 
  *                   - Configure the DMA using DMA_Init() function
  *                   - Active the needed channel Request using SPI_I2S_DMACmd() function
  * 
  *          7. Enable the SPI using the SPI_Cmd() function.
  * 
  *          8. Enable the DMA using the DMA_Cmd() function when using DMA mode. 
  *
  *          9. Optionally you can enable/configure the following parameters without
  *             re-initialization (i.e there is no need to call again SPI_Init() function):
  *              - When bidirectional mode (SPI_Direction_1Line_Rx or SPI_Direction_1Line_Tx)
  *                is programmed as Data direction parameter using the SPI_Init() function
  *                it can be possible to switch between SPI_Direction_Tx or SPI_Direction_Rx
  *                using the SPI_BiDirectionalLineConfig() function.
  *              - When SPI_NSS_Soft is selected as Slave Select Management parameter 
  *                using the SPI_Init() function it can be possible to manage the 
  *                NSS internal signal using the SPI_NSSInternalSoftwareConfig() function.
  *              -  Reconfigure the data size using the SPI_DataSizeConfig() function  
  *              -  Enable or disable the SS output using the SPI_SSOutputCmd() function  
  *          
  *          10. To use the CRC Hardware calculation feature refer to the Peripheral 
  *              CRC hardware Calculation subsection.
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
#include "stm32l1xx_spi.h"
#include "stm32l1xx_rcc.h"

/** @addtogroup STM32L1xx_StdPeriph_Driver
  * @{
  */

/** @defgroup SPI 
  * @brief SPI driver modules
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* SPI registers Masks */
#define CR1_CLEAR_MASK       ((uint16_t)0x3040)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup SPI_Private_Functions
  * @{
  */

/** @defgroup SPI_Group1 Initialization and Configuration functions
 *  @brief   Initialization and Configuration functions 
 *
@verbatim   
 ===============================================================================
                  Initialization and Configuration functions
 ===============================================================================  

  This section provides a set of functions allowing to initialize the SPI Direction,
  SPI Mode, SPI Data Size, SPI Polarity, SPI Phase, SPI NSS Management, SPI Baud
  Rate Prescaler, SPI First Bit and SPI CRC Polynomial.
  
  The SPI_Init() function follows the SPI configuration procedures for Master mode
  and Slave mode (details for these procedures are available in reference manual
  (RM0038)).
  
@endverbatim
  * @{
  */

/**
  * @brief  Deinitializes the SPIx peripheral registers to their default
  *         reset values.
  * @param  SPIx: where x can be 1 or 2 to select the SPI peripheral.
  * @retval None
  */
void SPI_I2S_DeInit(SPI_TypeDef* SPIx)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));

  if (SPIx == SPI1)
  {
    /* Enable SPI1 reset state */
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1, ENABLE);
    /* Release SPI1 from reset state */
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1, DISABLE);
  }
  else
  {
    if (SPIx == SPI2)
    {
      /* Enable SPI2 reset state */
      RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2, ENABLE);
      /* Release SPI2 from reset state */
      RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2, DISABLE);
    }
  }
}

/**
  * @brief  Initializes the SPIx peripheral according to the specified 
  *   parameters in the SPI_InitStruct.
  * @param  SPIx: where x can be 1 or 2 to select the SPI peripheral.
  * @param  SPI_InitStruct: pointer to a SPI_InitTypeDef structure that
  *   contains the configuration information for the specified SPI peripheral.
  * @retval None
  */
void SPI_Init(SPI_TypeDef* SPIx, SPI_InitTypeDef* SPI_InitStruct)
{
  uint16_t tmpreg = 0;
  
  /* check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  
  /* Check the SPI parameters */
  assert_param(IS_SPI_DIRECTION_MODE(SPI_InitStruct->SPI_Direction));
  assert_param(IS_SPI_MODE(SPI_InitStruct->SPI_Mode));
  assert_param(IS_SPI_DATASIZE(SPI_InitStruct->SPI_DataSize));
  assert_param(IS_SPI_CPOL(SPI_InitStruct->SPI_CPOL));
  assert_param(IS_SPI_CPHA(SPI_InitStruct->SPI_CPHA));
  assert_param(IS_SPI_NSS(SPI_InitStruct->SPI_NSS));
  assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_InitStruct->SPI_BaudRatePrescaler));
  assert_param(IS_SPI_FIRST_BIT(SPI_InitStruct->SPI_FirstBit));
  assert_param(IS_SPI_CRC_POLYNOMIAL(SPI_InitStruct->SPI_CRCPolynomial));

/*---------------------------- SPIx CR1 Configuration ------------------------*/
  /* Get the SPIx CR1 value */
  tmpreg = SPIx->CR1;
  /* Clear BIDIMode, BIDIOE, RxONLY, SSM, SSI, LSBFirst, BR, MSTR, CPOL and CPHA bits */
  tmpreg &= CR1_CLEAR_MASK;
  /* Configure SPIx: direction, NSS management, first transmitted bit, BaudRate prescaler
     master/salve mode, CPOL and CPHA */
  /* Set BIDImode, BIDIOE and RxONLY bits according to SPI_Direction value */
  /* Set SSM, SSI and MSTR bits according to SPI_Mode and SPI_NSS values */
  /* Set LSBFirst bit according to SPI_FirstBit value */
  /* Set BR bits according to SPI_BaudRatePrescaler value */
  /* Set CPOL bit according to SPI_CPOL value */
  /* Set CPHA bit according to SPI_CPHA value */
  tmpreg |= (uint16_t)((uint32_t)SPI_InitStruct->SPI_Direction | SPI_InitStruct->SPI_Mode |
                  SPI_InitStruct->SPI_DataSize | SPI_InitStruct->SPI_CPOL |  
                  SPI_InitStruct->SPI_CPHA | SPI_InitStruct->SPI_NSS |  
                  SPI_InitStruct->SPI_BaudRatePrescaler | SPI_InitStruct->SPI_FirstBit);
  /* Write to SPIx CR1 */
  SPIx->CR1 = tmpreg;
  
/*---------------------------- SPIx CRCPOLY Configuration --------------------*/
  /* Write to SPIx CRCPOLY */
  SPIx->CRCPR = SPI_InitStruct->SPI_CRCPolynomial;
}

/**
  * @brief  Fills each SPI_InitStruct member with its default value.
  * @param  SPI_InitStruct : pointer to a SPI_InitTypeDef structure which will be initialized.
  * @retval None
  */
void SPI_StructInit(SPI_InitTypeDef* SPI_InitStruct)
{
/*--------------- Reset SPI init structure parameters values -----------------*/
  /* Initialize the SPI_Direction member */
  SPI_InitStruct->SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  /* initialize the SPI_Mode member */
  SPI_InitStruct->SPI_Mode = SPI_Mode_Slave;
  /* initialize the SPI_DataSize member */
  SPI_InitStruct->SPI_DataSize = SPI_DataSize_8b;
  /* Initialize the SPI_CPOL member */
  SPI_InitStruct->SPI_CPOL = SPI_CPOL_Low;
  /* Initialize the SPI_CPHA member */
  SPI_InitStruct->SPI_CPHA = SPI_CPHA_1Edge;
  /* Initialize the SPI_NSS member */
  SPI_InitStruct->SPI_NSS = SPI_NSS_Hard;
  /* Initialize the SPI_BaudRatePrescaler member */
  SPI_InitStruct->SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
  /* Initialize the SPI_FirstBit member */
  SPI_InitStruct->SPI_FirstBit = SPI_FirstBit_MSB;
  /* Initialize the SPI_CRCPolynomial member */
  SPI_InitStruct->SPI_CRCPolynomial = 7;
}

/**
  * @brief  Enables or disables the specified SPI peripheral.
  * @param  SPIx: where x can be 1 or 2 to select the SPI peripheral.
  * @param  NewState: new state of the SPIx peripheral. 
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SPI_Cmd(SPI_TypeDef* SPIx, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {
    /* Enable the selected SPI peripheral */
    SPIx->CR1 |= SPI_CR1_SPE;
  }
  else
  {
    /* Disable the selected SPI peripheral */
    SPIx->CR1 &= (uint16_t)~((uint16_t)SPI_CR1_SPE);
  }
}

/**
  * @brief  Configures the data size for the selected SPI.
  * @param  SPIx: where x can be 1 or 2  to select the SPI peripheral.
  * @param  SPI_DataSize: specifies the SPI data size.
  *   This parameter can be one of the following values:
  *     @arg SPI_DataSize_16b: Set data frame format to 16bit
  *     @arg SPI_DataSize_8b: Set data frame format to 8bit
  * @retval None
  */
void SPI_DataSizeConfig(SPI_TypeDef* SPIx, uint16_t SPI_DataSize)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_SPI_DATASIZE(SPI_DataSize));
  /* Clear DFF bit */
  SPIx->CR1 &= (uint16_t)~SPI_DataSize_16b;
  /* Set new DFF bit value */
  SPIx->CR1 |= SPI_DataSize;
}

/**
  * @brief  Selects the data transfer direction in bidirectional mode for the specified SPI.
  * @param  SPIx: where x can be 1 or 2  to select the SPI peripheral.
  * @param  SPI_Direction: specifies the data transfer direction in bidirectional mode. 
  *   This parameter can be one of the following values:
  *     @arg SPI_Direction_Tx: Selects Tx transmission direction
  *     @arg SPI_Direction_Rx: Selects Rx receive direction
  * @retval None
  */
void SPI_BiDirectionalLineConfig(SPI_TypeDef* SPIx, uint16_t SPI_Direction)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_SPI_DIRECTION(SPI_Direction));
  if (SPI_Direction == SPI_Direction_Tx)
  {
    /* Set the Tx only mode */
    SPIx->CR1 |= SPI_Direction_Tx;
  }
  else
  {
    /* Set the Rx only mode */
    SPIx->CR1 &= SPI_Direction_Rx;
  }
}

/**
  * @brief  Configures internally by software the NSS pin for the selected SPI.
  * @param  SPIx: where x can be 1 or 2 to select the SPI peripheral.
  * @param  SPI_NSSInternalSoft: specifies the SPI NSS internal state.
  *   This parameter can be one of the following values:
  *     @arg SPI_NSSInternalSoft_Set: Set NSS pin internally
  *     @arg SPI_NSSInternalSoft_Reset: Reset NSS pin internally
  * @retval None
  */
void SPI_NSSInternalSoftwareConfig(SPI_TypeDef* SPIx, uint16_t SPI_NSSInternalSoft)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_SPI_NSS_INTERNAL(SPI_NSSInternalSoft));
  if (SPI_NSSInternalSoft != SPI_NSSInternalSoft_Reset)
  {
    /* Set NSS pin internally by software */
    SPIx->CR1 |= SPI_NSSInternalSoft_Set;
  }
  else
  {
    /* Reset NSS pin internally by software */
    SPIx->CR1 &= SPI_NSSInternalSoft_Reset;
  }
}

/**
  * @brief  Enables or disables the SS output for the selected SPI.
  * @param  SPIx: where x can be 1 or 2 to select the SPI peripheral.
  * @param  NewState: new state of the SPIx SS output. 
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SPI_SSOutputCmd(SPI_TypeDef* SPIx, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {
    /* Enable the selected SPI SS output */
    SPIx->CR2 |= (uint16_t)SPI_CR2_SSOE;
  }
  else
  {
    /* Disable the selected SPI SS output */
    SPIx->CR2 &= (uint16_t)~((uint16_t)SPI_CR2_SSOE);
  }
}

/**
  * @}
  */

/** @defgroup SPI_Group2 Data transfers functions
 *  @brief   Data transfers functions
 *
@verbatim   
 ===============================================================================
                         Data transfers functions
 ===============================================================================  

  This section provides a set of functions allowing to manage the SPI data transfers
  
  In reception, data are received and then stored into an internal Rx buffer while 
  In transmission, data are first stored into an internal Tx buffer before being 
  transmitted.

  The read access of the SPI_DR register can be done using the SPI_I2S_ReceiveData()
  function and returns the Rx buffered value. Whereas a write access to the SPI_DR 
  can be done using SPI_I2S_SendData() function and stores the written data into 
  Tx buffer.

@endverbatim
  * @{
  */

/**
  * @brief  Returns the most recent received data by the SPIx peripheral. 
  * @param  SPIx: where x can be 1 or 2 in SPI mode.
  * @retval The value of the received data.
  */
uint16_t SPI_I2S_ReceiveData(SPI_TypeDef* SPIx)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  
  /* Return the data in the DR register */
  return SPIx->DR;
}

/**
  * @brief  Transmits a Data through the SPIx peripheral.
  * @param  SPIx: where x can be 1 or 2 in SPI mode. 
  * @param  Data: Data to be transmitted.
  * @retval None
  */
void SPI_I2S_SendData(SPI_TypeDef* SPIx, uint16_t Data)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  
  /* Write in the DR register the data to be sent */
  SPIx->DR = Data;
}

/**
  * @}
  */

/** @defgroup SPI_Group3 Hardware CRC Calculation functions
 *  @brief   Hardware CRC Calculation functions
 *
@verbatim   
 ===============================================================================
                         Hardware CRC Calculation functions
 ===============================================================================  

  This section provides a set of functions allowing to manage the SPI CRC hardware 
  calculation

  SPI communication using CRC is possible through the following procedure:
     1. Program the Data direction, Polarity, Phase, First Data, Baud Rate Prescaler, 
        Slave Management, Peripheral Mode and CRC Polynomial values using the SPI_Init()
        function.
     2. Enable the CRC calculation using the SPI_CalculateCRC() function.
     3. Enable the SPI using the SPI_Cmd() function
     4. Before writing the last data to the TX buffer, set the CRCNext bit using the 
      SPI_TransmitCRC() function to indicate that after transmission of the last 
      data, the CRC should be transmitted.
     5. After transmitting the last data, the SPI transmits the CRC. The SPI_CR1_CRCNEXT
        bit is reset. The CRC is also received and compared against the SPI_RXCRCR 
        value. 
        If the value does not match, the SPI_FLAG_CRCERR flag is set and an interrupt
        can be generated when the SPI_I2S_IT_ERR interrupt is enabled.

Note: 
-----
    - It is advised to don't read the calculate CRC values during the communication.

    - When the SPI is in slave mode, be careful to enable CRC calculation only 
      when the clock is stable, that is, when the clock is in the steady state. 
      If not, a wrong CRC calculation may be done. In fact, the CRC is sensitive 
      to the SCK slave input clock as soon as CRCEN is set, and this, whatever 
      the value of the SPE bit.

    - With high bitrate frequencies, be careful when transmitting the CRC.
      As the number of used CPU cycles has to be as low as possible in the CRC 
      transfer phase, it is forbidden to call software functions in the CRC 
      transmission sequence to avoid errors in the last data and CRC reception. 
      In fact, CRCNEXT bit has to be written before the end of the transmission/reception 
      of the last data.

    - For high bit rate frequencies, it is advised to use the DMA mode to avoid the
      degradation of the SPI speed performance due to CPU accesses impacting the 
      SPI bandwidth.

    - When the STM32L15xxx are configured as slaves and the NSS hardware mode is 
      used, the NSS pin needs to be kept low between the data phase and the CRC 
      phase.

    - When the SPI is configured in slave mode with the CRC feature enabled, CRC
      calculation takes place even if a high level is applied on the NSS pin. 
      This may happen for example in case of a multislave environment where the 
      communication master addresses slaves alternately.

    - Between a slave deselection (high level on NSS) and a new slave selection 
      (low level on NSS), the CRC value should be cleared on both master and slave
      sides in order to resynchronize the master and slave for their respective 
      CRC calculation.

    To clear the CRC, follow the procedure below:
      1. Disable SPI using the SPI_Cmd() function
      2. Disable the CRC calculation using the SPI_CalculateCRC() function.
      3. Enable the CRC calculation using the SPI_CalculateCRC() function.
      4. Enable SPI using the SPI_Cmd() function.

@endverbatim
  * @{
  */

/**
  * @brief  Enables or disables the CRC value calculation of the transferred bytes.
  * @param  SPIx: where x can be 1 or 2  to select the SPI peripheral.
  * @param  NewState: new state of the SPIx CRC value calculation.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SPI_CalculateCRC(SPI_TypeDef* SPIx, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {
    /* Enable the selected SPI CRC calculation */
    SPIx->CR1 |= SPI_CR1_CRCEN;
  }
  else
  {
    /* Disable the selected SPI CRC calculation */
    SPIx->CR1 &= (uint16_t)~((uint16_t)SPI_CR1_CRCEN);
  }
}

/**
  * @brief  Transmit the SPIx CRC value.
  * @param  SPIx: where x can be 1 or 2  to select the SPI peripheral.
  * @retval None
  */
void SPI_TransmitCRC(SPI_TypeDef* SPIx)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  
  /* Enable the selected SPI CRC transmission */
  SPIx->CR1 |= SPI_CR1_CRCNEXT;
}

/**
  * @brief  Returns the transmit or the receive CRC register value for the specified SPI.
  * @param  SPIx: where x can be 1 or 2  to select the SPI peripheral.
  * @param  SPI_CRC: specifies the CRC register to be read.
  *   This parameter can be one of the following values:
  *     @arg SPI_CRC_Tx: Selects Tx CRC register
  *     @arg SPI_CRC_Rx: Selects Rx CRC register
  * @retval The selected CRC register value..
  */
uint16_t SPI_GetCRC(SPI_TypeDef* SPIx, uint8_t SPI_CRC)
{
  uint16_t crcreg = 0;
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_SPI_CRC(SPI_CRC));
  if (SPI_CRC != SPI_CRC_Rx)
  {
    /* Get the Tx CRC register */
    crcreg = SPIx->TXCRCR;
  }
  else
  {
    /* Get the Rx CRC register */
    crcreg = SPIx->RXCRCR;
  }
  /* Return the selected CRC register */
  return crcreg;
}

/**
  * @brief  Returns the CRC Polynomial register value for the specified SPI.
  * @param  SPIx: where x can be 1 or 2  to select the SPI peripheral.
  * @retval The CRC Polynomial register value.
  */
uint16_t SPI_GetCRCPolynomial(SPI_TypeDef* SPIx)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  
  /* Return the CRC polynomial register */
  return SPIx->CRCPR;
}

/**
  * @}
  */

/** @defgroup SPI_Group4 DMA transfers management functions
 *  @brief   DMA transfers management functions
  *
@verbatim   
 ===============================================================================
                         DMA transfers management functions
 ===============================================================================  

@endverbatim
  * @{
  */

/**
  * @brief  Enables or disables the SPIx DMA interface.
  * @param  SPIx: where x can be 1 or 2 in SPI mode 
  * @param  SPI_I2S_DMAReq: specifies the SPI DMA transfer request to be enabled or disabled. 
  *   This parameter can be any combination of the following values:
  *     @arg SPI_I2S_DMAReq_Tx: Tx buffer DMA transfer request
  *     @arg SPI_I2S_DMAReq_Rx: Rx buffer DMA transfer request
  * @param  NewState: new state of the selected SPI DMA transfer request.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SPI_I2S_DMACmd(SPI_TypeDef* SPIx, uint16_t SPI_I2S_DMAReq, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  assert_param(IS_SPI_I2S_DMAREQ(SPI_I2S_DMAReq));

  if (NewState != DISABLE)
  {
    /* Enable the selected SPI DMA requests */
    SPIx->CR2 |= SPI_I2S_DMAReq;
  }
  else
  {
    /* Disable the selected SPI DMA requests */
    SPIx->CR2 &= (uint16_t)~SPI_I2S_DMAReq;
  }
}

/**
  * @}
  */

/** @defgroup SPI_Group5 Interrupts and flags management functions
 *  @brief   Interrupts and flags management functions
  *
@verbatim   
 ===============================================================================
                         Interrupts and flags management functions
 ===============================================================================  

  This section provides a set of functions allowing to configure the SPI Interrupts 
  sources and check or clear the flags or pending bits status.
  The user should identify which mode will be used in his application to manage 
  the communication: Polling mode, Interrupt mode or DMA mode. 
    
  Polling Mode
  =============
  In Polling Mode, the SPI communication can be managed by 6 flags:
     1. SPI_I2S_FLAG_TXE : to indicate the status of the transmit buffer register
     2. SPI_I2S_FLAG_RXNE : to indicate the status of the receive buffer register
     3. SPI_I2S_FLAG_BSY : to indicate the state of the communication layer of the SPI.
     4. SPI_FLAG_CRCERR : to indicate if a CRC Calculation error occur              
     5. SPI_FLAG_MODF : to indicate if a Mode Fault error occur
     6. SPI_I2S_FLAG_OVR : to indicate if an Overrun error occur

Note: Do not use the BSY flag to handle each data transmission or reception.
----- It is better to use the TXE and RXNE flags instead.

  In this Mode it is advised to use the following functions:
     - FlagStatus SPI_I2S_GetFlagStatus(SPI_TypeDef* SPIx, uint16_t SPI_I2S_FLAG);
     - void SPI_I2S_ClearFlag(SPI_TypeDef* SPIx, uint16_t SPI_I2S_FLAG);

  Interrupt Mode
  ===============
  In Interrupt Mode, the SPI communication can be managed by 3 interrupt sources
  and 5 pending bits: 
  Pending Bits:
  ------------- 
     1. SPI_I2S_IT_TXE : to indicate the status of the transmit buffer register
     2. SPI_I2S_IT_RXNE : to indicate the status of the receive buffer register
     3. SPI_IT_CRCERR : to indicate if a CRC Calculation error occur              
     4. SPI_IT_MODF : to indicate if a Mode Fault error occur
     5. SPI_I2S_IT_OVR : to indicate if an Overrun error occur

  Interrupt Source:
  -----------------
     1. SPI_I2S_IT_TXE: specifies the interrupt source for the Tx buffer empty 
                        interrupt.  
     2. SPI_I2S_IT_RXNE : specifies the interrupt source for the Rx buffer not 
                          empty interrupt.
     3. SPI_I2S_IT_ERR : specifies the interrupt source for the errors interrupt.

  In this Mode it is advised to use the following functions:
     - void SPI_I2S_ITConfig(SPI_TypeDef* SPIx, uint8_t SPI_I2S_IT, FunctionalState NewState);
     - ITStatus SPI_I2S_GetITStatus(SPI_TypeDef* SPIx, uint8_t SPI_I2S_IT);
     - void SPI_I2S_ClearITPendingBit(SPI_TypeDef* SPIx, uint8_t SPI_I2S_IT);

  DMA Mode
  ========
  In DMA Mode, the SPI communication can be managed by 2 DMA Channel requests:
     1. SPI_I2S_DMAReq_Tx: specifies the Tx buffer DMA transfer request
     2. SPI_I2S_DMAReq_Rx: specifies the Rx buffer DMA transfer request

  In this Mode it is advised to use the following function:
    - void SPI_I2S_DMACmd(SPI_TypeDef* SPIx, uint16_t SPI_I2S_DMAReq, FunctionalState NewState);

@endverbatim
  * @{
  */

/**
  * @brief  Enables or disables the specified SPI interrupts.
  * @param  SPIx: where x can be 1 or 2 in SPI mode 
  * @param  SPI_I2S_IT: specifies the SPI interrupt source to be enabled or disabled. 
  *   This parameter can be one of the following values:
  *     @arg SPI_I2S_IT_TXE: Tx buffer empty interrupt mask
  *     @arg SPI_I2S_IT_RXNE: Rx buffer not empty interrupt mask
  *     @arg SPI_I2S_IT_ERR: Error interrupt mask
  * @param  NewState: new state of the specified SPI interrupt.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SPI_I2S_ITConfig(SPI_TypeDef* SPIx, uint8_t SPI_I2S_IT, FunctionalState NewState)
{
  uint16_t itpos = 0, itmask = 0 ;
  
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  assert_param(IS_SPI_I2S_CONFIG_IT(SPI_I2S_IT));

  /* Get the SPI IT index */
  itpos = SPI_I2S_IT >> 4;

  /* Set the IT mask */
  itmask = (uint16_t)1 << (uint16_t)itpos;

  if (NewState != DISABLE)
  {
    /* Enable the selected SPI interrupt */
    SPIx->CR2 |= itmask;
  }
  else
  {
    /* Disable the selected SPI interrupt */
    SPIx->CR2 &= (uint16_t)~itmask;
  }
}

/**
  * @brief  Checks whether the specified SPI flag is set or not.
  * @param  SPIx: where x can be 1 or 2 in SPI mode 
  * @param  SPI_I2S_FLAG: specifies the SPI flag to check. 
  *   This parameter can be one of the following values:
  *     @arg SPI_I2S_FLAG_TXE: Transmit buffer empty flag.
  *     @arg SPI_I2S_FLAG_RXNE: Receive buffer not empty flag.
  *     @arg SPI_I2S_FLAG_BSY: Busy flag.
  *     @arg SPI_I2S_FLAG_OVR: Overrun flag.
  *     @arg SPI_I2S_FLAG_MODF: Mode Fault flag.
  *     @arg SPI_I2S_FLAG_CRCERR: CRC Error flag.
  * @retval The new state of SPI_I2S_FLAG (SET or RESET).
  */
FlagStatus SPI_I2S_GetFlagStatus(SPI_TypeDef* SPIx, uint16_t SPI_I2S_FLAG)
{
  FlagStatus bitstatus = RESET;
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_SPI_I2S_GET_FLAG(SPI_I2S_FLAG));
  
  /* Check the status of the specified SPI flag */
  if ((SPIx->SR & SPI_I2S_FLAG) != (uint16_t)RESET)
  {
    /* SPI_I2S_FLAG is set */
    bitstatus = SET;
  }
  else
  {
    /* SPI_I2S_FLAG is reset */
    bitstatus = RESET;
  }
  /* Return the SPI_I2S_FLAG status */
  return  bitstatus;
}

/**
  * @brief  Clears the SPIx CRC Error (CRCERR) flag.
  * @param  SPIx: where x can be 1 or 2 in SPI mode 
  * @param  SPI_I2S_FLAG: specifies the SPI flag to clear. 
  *   This function clears only CRCERR flag.
  * @note
  *   - OVR (OverRun error) flag is cleared by software sequence: a read 
  *     operation to SPI_DR register (SPI_I2S_ReceiveData()) followed by a read 
  *     operation to SPI_SR register (SPI_I2S_GetFlagStatus()).
  *   - MODF (Mode Fault) flag is cleared by software sequence: a read/write 
  *     operation to SPI_SR register (SPI_I2S_GetFlagStatus()) followed by a 
  *     write operation to SPI_CR1 register (SPI_Cmd() to enable the SPI).
  * @retval None
  */
void SPI_I2S_ClearFlag(SPI_TypeDef* SPIx, uint16_t SPI_I2S_FLAG)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_SPI_I2S_CLEAR_FLAG(SPI_I2S_FLAG));
    
  /* Clear the selected SPI CRC Error (CRCERR) flag */
  SPIx->SR = (uint16_t)~SPI_I2S_FLAG;
}

/**
  * @brief  Checks whether the specified SPI interrupt has occurred or not.
  * @param  SPIx: where x can be
  *   - 1 or 2 in SPI mode 
  * @param  SPI_I2S_IT: specifies the SPI interrupt source to check. 
  *   This parameter can be one of the following values:
  *     @arg SPI_I2S_IT_TXE: Transmit buffer empty interrupt.
  *     @arg SPI_I2S_IT_RXNE: Receive buffer not empty interrupt.
  *     @arg SPI_I2S_IT_OVR: Overrun interrupt.
  *     @arg SPI_I2S_IT_MODF: Mode Fault interrupt.
  *     @arg SPI_I2S_IT_CRCERR: CRC Error interrupt.
  * @retval The new state of SPI_I2S_IT (SET or RESET).
  */
ITStatus SPI_I2S_GetITStatus(SPI_TypeDef* SPIx, uint8_t SPI_I2S_IT)
{
  ITStatus bitstatus = RESET;
  uint16_t itpos = 0, itmask = 0, enablestatus = 0;

  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_SPI_I2S_GET_IT(SPI_I2S_IT));

  /* Get the SPI_I2S_IT index */
  itpos = 0x01 << (SPI_I2S_IT & 0x0F);

  /* Get the SPI_I2S_IT IT mask */
  itmask = SPI_I2S_IT >> 4;

  /* Set the IT mask */
  itmask = 0x01 << itmask;

  /* Get the SPI_I2S_IT enable bit status */
  enablestatus = (SPIx->CR2 & itmask) ;

  /* Check the status of the specified SPI interrupt */
  if (((SPIx->SR & itpos) != (uint16_t)RESET) && enablestatus)
  {
    /* SPI_I2S_IT is set */
    bitstatus = SET;
  }
  else
  {
    /* SPI_I2S_IT is reset */
    bitstatus = RESET;
  }
  /* Return the SPI_I2S_IT status */
  return bitstatus;
}

/**
  * @brief  Clears the SPIx CRC Error (CRCERR) interrupt pending bit.
  * @param  SPIx: where x can be
  *   - 1 or 2 in SPI mode 
  * @param  SPI_I2S_IT: specifies the SPI interrupt pending bit to clear.
  *   This function clears only CRCERR interrupt pending bit.   
  * @note
  *   - OVR (OverRun Error) interrupt pending bit is cleared by software 
  *     sequence: a read operation to SPI_DR register (SPI_I2S_ReceiveData()) 
  *     followed by a read operation to SPI_SR register (SPI_I2S_GetITStatus()).
  *   - MODF (Mode Fault) interrupt pending bit is cleared by software sequence:
  *     a read/write operation to SPI_SR register (SPI_I2S_GetITStatus()) 
  *     followed by a write operation to SPI_CR1 register (SPI_Cmd() to enable 
  *     the SPI).
  * @retval None
  */
void SPI_I2S_ClearITPendingBit(SPI_TypeDef* SPIx, uint8_t SPI_I2S_IT)
{
  uint16_t itpos = 0;
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_SPI_I2S_CLEAR_IT(SPI_I2S_IT));

  /* Get the SPI_I2S IT index */
  itpos = 0x01 << (SPI_I2S_IT & 0x0F);

  /* Clear the selected SPI CRC Error (CRCERR) interrupt pending bit */
  SPIx->SR = (uint16_t)~itpos;
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

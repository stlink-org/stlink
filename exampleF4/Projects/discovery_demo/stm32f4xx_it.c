/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    19-September-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides all exceptions handler and peripherals interrupt
  *          service routine.
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
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "main.h"
#include "usb_core.h"
#include "usbd_core.h"
#include "stm32f4_discovery.h"
#include "usbd_hid_core.h"

//Library config for this project!!!!!!!!!!!
#include "stm32f4xx_conf.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define CURSOR_STEP     7

extern uint8_t Buffer[6];
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern __IO uint8_t DemoEnterCondition;
uint8_t Counter  = 0x00;
extern int8_t X_Offset;
extern int8_t Y_Offset;
extern __IO uint8_t UserButtonPressed;
__IO uint8_t TempAcceleration = 0;
/* Private function prototypes -----------------------------------------------*/
extern USB_OTG_CORE_HANDLE           USB_OTG_dev;
static uint8_t *USBD_HID_GetPos (void);
extern uint32_t USBD_OTG_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
  uint8_t *buf;
  uint8_t temp1, temp2 = 0x00;
  
  if (DemoEnterCondition == 0x00)
  {
    TimingDelay_Decrement();
  }
  else
  {
    buf = USBD_HID_GetPos();
    if((buf[1] != 0) ||(buf[2] != 0))
    {
      USBD_HID_SendReport (&USB_OTG_dev, 
                           buf,
                           4);
    } 
    Counter ++;
    if (Counter == 10)
    {
      Buffer[0] = 0;
      Buffer[2] = 0;
      /* Disable All TIM4 Capture Compare Channels */
      TIM_CCxCmd(TIM4, TIM_Channel_1, DISABLE);
      TIM_CCxCmd(TIM4, TIM_Channel_2, DISABLE);
      TIM_CCxCmd(TIM4, TIM_Channel_3, DISABLE);
      TIM_CCxCmd(TIM4, TIM_Channel_4, DISABLE);
      
      LIS302DL_Read(Buffer, LIS302DL_OUT_X_ADDR, 6);
      /* Remove the offsets values from data */
      Buffer[0] -= X_Offset;
      Buffer[2] -= Y_Offset;
      /* Update autoreload and capture compare registers value*/
      temp1 = ABS((int8_t)(Buffer[0]));
      temp2 = ABS((int8_t)(Buffer[2]));       
      TempAcceleration = MAX(temp1, temp2);

      if(TempAcceleration != 0)
      { 
        if ((int8_t)Buffer[0] < -2)
        {
          /* Enable TIM4 Capture Compare Channel 4 */
          TIM_CCxCmd(TIM4, TIM_Channel_4, ENABLE);
          /* Sets the TIM4 Capture Compare4 Register value */
          TIM_SetCompare4(TIM4, TIM_CCR/TempAcceleration);
        }
        if ((int8_t)Buffer[0] > 2)
        {
          /* Enable TIM4 Capture Compare Channel 2 */
          TIM_CCxCmd(TIM4, TIM_Channel_2, ENABLE);
          /* Sets the TIM4 Capture Compare2 Register value */
          TIM_SetCompare2(TIM4, TIM_CCR/TempAcceleration);
        }
        if ((int8_t)Buffer[2] > 2)
        { 
          /* Enable TIM4 Capture Compare Channel 1 */
          TIM_CCxCmd(TIM4, TIM_Channel_1, ENABLE);
          /* Sets the TIM4 Capture Compare1 Register value */
          TIM_SetCompare1(TIM4, TIM_CCR/TempAcceleration);
        }      
        if ((int8_t)Buffer[2] < -2)
        { 
          /* Enable TIM4 Capture Compare Channel 3 */
          TIM_CCxCmd(TIM4, TIM_Channel_3, ENABLE);
          /* Sets the TIM4 Capture Compare3 Register value */
          TIM_SetCompare3(TIM4, TIM_CCR/TempAcceleration);
        }
        /* Time base configuration */
        TIM_SetAutoreload(TIM4,  TIM_ARR/TempAcceleration);
      }
      Counter = 0x00;
    }  
  }
  
}

/******************************************************************************/
/*                 STM32Fxxx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32fxxx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @brief  This function handles EXTI0_IRQ Handler.
  * @param  None
  * @retval None
  */
void EXTI0_IRQHandler(void)
{
  UserButtonPressed = 0x01;
  
  /* Clear the EXTI line pending bit */
  EXTI_ClearITPendingBit(USER_BUTTON_EXTI_LINE);
}

/**
  * @brief  This function handles EXTI15_10_IRQ Handler.
  * @param  None
  * @retval None
  */
void OTG_FS_WKUP_IRQHandler(void)
{
  if(USB_OTG_dev.cfg.low_power)
  {
	/* Reset SLEEPDEEP and SLEEPONEXIT bits */
	SCB->SCR &= (uint32_t)~((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));

	/* After wake-up from sleep mode, reconfigure the system clock */
	SystemInit();
    USB_OTG_UngateClock(&USB_OTG_dev);
  }
  EXTI_ClearITPendingBit(EXTI_Line18);
}

/**
  * @brief  This function handles OTG_HS Handler.
  * @param  None
  * @retval None
  */
void OTG_FS_IRQHandler(void)
{
  USBD_OTG_ISR_Handler (&USB_OTG_dev);
}

/**
* @brief  USBD_HID_GetPos
* @param  None
* @retval Pointer to report
*/
static uint8_t *USBD_HID_GetPos (void)
{
  static uint8_t HID_Buffer[4] = {0};
  
  HID_Buffer[1] = 0;
  HID_Buffer[2] = 0;
  /* LEFT Direction */
  if(((int8_t)Buffer[2]) < -2)
  {
    HID_Buffer[1] += CURSOR_STEP;
  }
  /* RIGHT Direction */ 
  if(((int8_t)Buffer[2]) > 2)
  {
   HID_Buffer[1] -= CURSOR_STEP;
  } 
  /* UP Direction */
  if(((int8_t)Buffer[0]) < -2)
  {
    HID_Buffer[2] += CURSOR_STEP;
  }
  /* DOWN Direction */ 
  if(((int8_t)Buffer[0]) > 2)
  {
    HID_Buffer[2] -= CURSOR_STEP;
  } 
  
  return HID_Buffer;
}
/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

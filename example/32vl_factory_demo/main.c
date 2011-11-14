/**
  ******************************************************************************
  * @file    Demo/src/main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    09/13/2010
  * @brief   Main program body
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "STM32vldiscovery.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define  LSE_FAIL_FLAG  0x80
#define  LSE_PASS_FLAG  0x100
/* Private macro -------------------------------------------------------------*/
/* Private consts ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
u32 LSE_Delay = 0;
u32 count = 0;
u32 BlinkSpeed = 0;
u32 KeyState = 0;
static __IO uint32_t TimingDelay;
/* Private function prototypes -----------------------------------------------*/
void Delay(uint32_t nTime);
void TimingDelay_Decrement(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */

int main(void)
{
  /* Enable GPIOx Clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  
  /* Initialise LEDs LD3&LD4, both off */
  STM32vldiscovery_LEDInit(LED3);
  STM32vldiscovery_LEDInit(LED4);
  
  STM32vldiscovery_LEDOff(LED3);
  STM32vldiscovery_LEDOff(LED4);
  
  /* Initialise USER Button */
  STM32vldiscovery_PBInit(BUTTON_USER, BUTTON_MODE_GPIO); 
  
  /* Setup SysTick Timer for 1 msec interrupts  */
  if (SysTick_Config(SystemCoreClock / 1000))
  { 
    /* Capture error */ 
    while (1);
  }

  /* Enable access to the backup register => LSE can be enabled */
  PWR_BackupAccessCmd(ENABLE);
  
  /* Enable LSE (Low Speed External Oscillation) */
  RCC_LSEConfig(RCC_LSE_ON);
  
  /* Check the LSE Status */
  while(1)
  {
    if(LSE_Delay < LSE_FAIL_FLAG)
    {
      /* check whether LSE is ready, with 4 seconds timeout */
      Delay (500);
      LSE_Delay += 0x10;
      if(RCC_GetFlagStatus(RCC_FLAG_LSERDY) != RESET)
      {
        /* Set flag: LSE PASS */
        LSE_Delay |= LSE_PASS_FLAG;
        /* Turn Off Led4 */
        STM32vldiscovery_LEDOff(LED4);
        /* Disable LSE */
        RCC_LSEConfig(RCC_LSE_OFF);
        break;
      }        
    }
    
    /* LSE_FAIL_FLAG = 0x80 */  
    else if(LSE_Delay >= LSE_FAIL_FLAG)
    {          
      if(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
      {
        /* Set flag: LSE FAIL */
        LSE_Delay |= LSE_FAIL_FLAG;
        /* Turn On Led4 */
        STM32vldiscovery_LEDOn(LED4);
      }        
      /* Disable LSE */
      RCC_LSEConfig(RCC_LSE_OFF);
      break;
    }
  }
  
  /* main while */
  while(1)
  {
    if(0 == STM32vldiscovery_PBGetState(BUTTON_USER))
      {
        if(KeyState == 1)
        {
           if(0 == STM32vldiscovery_PBGetState(BUTTON_USER))
          {
            /* USER Button released */
              KeyState = 0;
            /* Turn Off LED4 */
              STM32vldiscovery_LEDOff(LED4);
          }       
        }
      }
    else if(STM32vldiscovery_PBGetState(BUTTON_USER))
      { 
        if(KeyState == 0)
        {
           if(STM32vldiscovery_PBGetState(BUTTON_USER))
          {
            /* USER Button released */
              KeyState = 1;
            /* Turn ON LED4 */
            STM32vldiscovery_LEDOn(LED4);
            Delay(1000);
            /* Turn OFF LED4 */
            STM32vldiscovery_LEDOff(LED4);
            /* BlinkSpeed: 0 -> 1 -> 2, then re-cycle */    
              BlinkSpeed ++ ; 
          }
        }
      }
        count++;
        Delay(100);
      /* BlinkSpeed: 0 */ 
      if(BlinkSpeed == 0)
          {
            if(4 == (count % 8))
            STM32vldiscovery_LEDOn(LED3);
            if(0 == (count % 8))
            STM32vldiscovery_LEDOff(LED3);
         }
           /* BlinkSpeed: 1 */ 
           if(BlinkSpeed == 1)
          {
            if(2 == (count % 4))
            STM32vldiscovery_LEDOn(LED3);
            if(0 == (count % 4))
            STM32vldiscovery_LEDOff(LED3);
          }  
          /* BlinkSpeed: 2 */        
          if(BlinkSpeed == 2)
          {
            if(0 == (count % 2))
            STM32vldiscovery_LEDOn(LED3);
            else
            STM32vldiscovery_LEDOff(LED3);     
          }     
          /* BlinkSpeed: 3 */ 
          else if(BlinkSpeed == 3)
        BlinkSpeed = 0;
  }
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in milliseconds.
  * @retval None
  */
void Delay(uint32_t nTime)
{ 
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/

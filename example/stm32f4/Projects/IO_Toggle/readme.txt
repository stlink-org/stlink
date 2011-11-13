/**
  @page GPIO_IOToggle GPIO IO Toggle example
  
  @verbatim
  ******************** (C) COPYRIGHT 2011 STMicroelectronics *******************
  * @file    IO_Toggle/readme.txt 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    19-September-2011
  * @brief   Description of the GPIO IO Toggle example.
  ******************************************************************************
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  ******************************************************************************
   @endverbatim

@par Example Description 

This example describes how to toggle the GPIO pins connected on AHB bus.

PD12, PD13, PD14 and PD15 (configured in output pushpull mode) toggles in a forever loop.
 - Set PD12, PD13, PD14 and PD15 by setting corresponding bits in BSRRL register
 - Reset PD12, PD13, PD14 and PD15  by setting corresponding bits in BSRRH register

In this example, HCLK is configured at 168 MHz.

@par Directory contents 

  - IO_Toggle/stm32f4xx_conf.h    Library Configuration file
  - IO_Toggle/stm32f4xx_it.c      Interrupt handlers
  - IO_Toggle/stm32f4xx_it.h      Interrupt handlers header file
  - IO_Toggle/main.c              Main program
  - IO_Toggle/system_stm32f4xx.c  STM32F4xx system source file

@par Hardware and Software environment

  - This example runs on STM32F4xx Devices Revision A.

  - This example has been tested with STM32F4-Discovery (MB997) RevA and can be
    easily tailored to any other development board

  - STM32F4-Discovery
    - LED4, LED3, LED5 and LED6 are connected respectively to PD.12, PD.13, PD.14 and PD.15.
      

@par How to use it ? 

In order to make the program work, you must do the following :

 + EWARM
    - Open the IO_Toggle.eww workspace 
    - Rebuild all files: Project->Rebuild all
    - Load project image: Project->Debug
    - Run program: Debug->Go(F5)

 + MDK-ARM
    - Open the IO_Toggle.uvproj project
    - Rebuild all files: Project->Rebuild all target files
    - Load project image: Debug->Start/Stop Debug Session
    - Run program: Debug->Run (F5)    

 + TASKING
    - Open TASKING toolchain.
    - Click on File->Import, select General->'Existing Projects into Workspace' 
      and then click "Next". 
    - Browse to  TASKING workspace directory and select the project "IO_Toggle"   
    - Rebuild all project files: Select the project in the "Project explorer" 
      window then click on Project->build project menu.
    - Run program: Select the project in the "Project explorer" window then click 
      Run->Debug (F11)

 + TrueSTUDIO
    - Open the TrueSTUDIO toolchain.
    - Click on File->Switch Workspace->Other and browse to TrueSTUDIO workspace 
      directory.
    - Click on File->Import, select General->'Existing Projects into Workspace' 
      and then click "Next". 
    - Browse to the TrueSTUDIO workspace directory and select the project "IO_Toggle" 
    - Rebuild all project files: Select the project in the "Project explorer" 
      window then click on Project->build project menu.
    - Run program: Select the project in the "Project explorer" window then click 
      Run->Debug (F11)


    
 * <h3><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h3>
 */

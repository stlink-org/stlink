/**
  ******************************************************************************
  * @file    stm32l1xx_flash.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    31-December-2010
  * @brief   This file contains all the functions prototypes for the FLASH 
  *          firmware library.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32L1xx_FLASH_H
#define __STM32L1xx_FLASH_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx.h"

/** @addtogroup STM32L1xx_StdPeriph_Driver
  * @{
  */

/** @addtogroup FLASH
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** 
  * @brief  FLASH Status  
  */ 
typedef enum
{ 
  FLASH_BUSY = 1,
  FLASH_ERROR_WRP,
  FLASH_ERROR_PROGRAM,
  FLASH_COMPLETE,
  FLASH_TIMEOUT
}FLASH_Status;

/* Exported constants --------------------------------------------------------*/
  
/** @defgroup FLASH_Exported_Constants
  * @{
  */ 
  
/** @defgroup FLASH_Latency 
  * @{
  */ 
#define FLASH_Latency_0                ((uint8_t)0x00)  /*!< FLASH Zero Latency cycle */
#define FLASH_Latency_1                ((uint8_t)0x01)  /*!< FLASH One Latency cycle */

#define IS_FLASH_LATENCY(LATENCY) (((LATENCY) == FLASH_Latency_0) || \
                                   ((LATENCY) == FLASH_Latency_1))
/**
  * @}
  */ 

/** @defgroup FLASH_Interrupts 
  * @{
  */
   
#define FLASH_IT_EOP               FLASH_PECR_EOPIE  /*!< End of programming interrupt source */
#define FLASH_IT_ERR               FLASH_PECR_ERRIE  /*!< Error interrupt source */
#define IS_FLASH_IT(IT) ((((IT) & (uint32_t)0xFFFCFFFF) == 0x00000000) && (((IT) != 0x00000000)))
/**
  * @}
  */ 

/** @defgroup FLASH_Address 
  * @{
  */
  
#define IS_FLASH_DATA_ADDRESS(ADDRESS) (((ADDRESS) >= 0x08080000) && ((ADDRESS) <= 0x08080FFF))
#define IS_FLASH_PROGRAM_ADDRESS(ADDRESS) (((ADDRESS) >= 0x08000000) && ((ADDRESS) <= 0x0801FFFF))  

/**
  * @}
  */ 

/** @defgroup Option_Bytes_Write_Protection 
  * @{
  */
  

#define OB_WRP_Pages0to15           ((uint32_t)0x00000001) /* Write protection of Sector0 */
#define OB_WRP_Pages16to31          ((uint32_t)0x00000002) /* Write protection of Sector1 */
#define OB_WRP_Pages32to47          ((uint32_t)0x00000004) /* Write protection of Sector2 */
#define OB_WRP_Pages48to63          ((uint32_t)0x00000008) /* Write protection of Sector3 */
#define OB_WRP_Pages64to79          ((uint32_t)0x00000010) /* Write protection of Sector4 */
#define OB_WRP_Pages80to95          ((uint32_t)0x00000020) /* Write protection of Sector5 */
#define OB_WRP_Pages96to111         ((uint32_t)0x00000040) /* Write protection of Sector6 */
#define OB_WRP_Pages112to127        ((uint32_t)0x00000080) /* Write protection of Sector7 */
#define OB_WRP_Pages128to143        ((uint32_t)0x00000100) /* Write protection of Sector8 */
#define OB_WRP_Pages144to159        ((uint32_t)0x00000200) /* Write protection of Sector9 */
#define OB_WRP_Pages160to175        ((uint32_t)0x00000400) /* Write protection of Sector10 */
#define OB_WRP_Pages176to191        ((uint32_t)0x00000800) /* Write protection of Sector11 */
#define OB_WRP_Pages192to207        ((uint32_t)0x00001000) /* Write protection of Sector12 */
#define OB_WRP_Pages208to223        ((uint32_t)0x00002000) /* Write protection of Sector13 */
#define OB_WRP_Pages224to239        ((uint32_t)0x00004000) /* Write protection of Sector14 */
#define OB_WRP_Pages240to255        ((uint32_t)0x00008000) /* Write protection of Sector15 */
#define OB_WRP_Pages256to271        ((uint32_t)0x00010000) /* Write protection of Sector16 */
#define OB_WRP_Pages272to287        ((uint32_t)0x00020000) /* Write protection of Sector17 */
#define OB_WRP_Pages288to303        ((uint32_t)0x00040000) /* Write protection of Sector18 */
#define OB_WRP_Pages304to319        ((uint32_t)0x00080000) /* Write protection of Sector19 */
#define OB_WRP_Pages320to335        ((uint32_t)0x00100000) /* Write protection of Sector20 */
#define OB_WRP_Pages336to351        ((uint32_t)0x00200000) /* Write protection of Sector21 */
#define OB_WRP_Pages352to367        ((uint32_t)0x00400000) /* Write protection of Sector22 */
#define OB_WRP_Pages368to383        ((uint32_t)0x00800000) /* Write protection of Sector23 */
#define OB_WRP_Pages384to399        ((uint32_t)0x01000000) /* Write protection of Sector24 */
#define OB_WRP_Pages400to415        ((uint32_t)0x02000000) /* Write protection of Sector25 */
#define OB_WRP_Pages416to431        ((uint32_t)0x04000000) /* Write protection of Sector26 */
#define OB_WRP_Pages432to447        ((uint32_t)0x08000000) /* Write protection of Sector27 */
#define OB_WRP_Pages448to463        ((uint32_t)0x10000000) /* Write protection of Sector28 */
#define OB_WRP_Pages464to479        ((uint32_t)0x20000000) /* Write protection of Sector29 */
#define OB_WRP_Pages480to495        ((uint32_t)0x40000000) /* Write protection of Sector30 */
#define OB_WRP_Pages496to511        ((uint32_t)0x80000000) /* Write protection of Sector31 */

#define OB_WRP_AllPages       ((uint32_t)0xFFFFFFFF) /*!< Write protection of all Sectors */

#define IS_OB_WRP(PAGE) (((PAGE) != 0x0000000))

/**
  * @}
  */

/** @defgroup Option_Bytes_Read_Protection 
  * @{
  */ 

/** 
  * @brief  Read Protection Level  
  */ 
#define OB_RDP_Level_0   ((uint8_t)0xAA)
#define OB_RDP_Level_1   ((uint8_t)0xBB)
/*#define OB_RDP_Level_2   ((uint8_t)0xCC)*/ /* Warning: When enabling read protection level 2 
                                                it's no more possible to go back to level 1 or 0 */

#define IS_OB_RDP(LEVEL) (((LEVEL) == OB_RDP_Level_0)||\
                          ((LEVEL) == OB_RDP_Level_1))/*||\
                          ((LEVEL) == OB_RDP_Level_2))*/
/**
  * @}
  */ 

/** @defgroup Option_Bytes_IWatchdog 
  * @{
  */

#define OB_IWDG_SW                     ((uint8_t)0x10)  /*!< Software WDG selected */
#define OB_IWDG_HW                     ((uint8_t)0x00)  /*!< Hardware WDG selected */
#define IS_OB_IWDG_SOURCE(SOURCE) (((SOURCE) == OB_IWDG_SW) || ((SOURCE) == OB_IWDG_HW))

/**
  * @}
  */

/** @defgroup Option_Bytes_nRST_STOP 
  * @{
  */

#define OB_STOP_NoRST                  ((uint8_t)0x20) /*!< No reset generated when entering in STOP */
#define OB_STOP_RST                    ((uint8_t)0x00) /*!< Reset generated when entering in STOP */
#define IS_OB_STOP_SOURCE(SOURCE) (((SOURCE) == OB_STOP_NoRST) || ((SOURCE) == OB_STOP_RST))

/**
  * @}
  */

/** @defgroup Option_Bytes_nRST_STDBY 
  * @{
  */

#define OB_STDBY_NoRST                 ((uint8_t)0x40) /*!< No reset generated when entering in STANDBY */
#define OB_STDBY_RST                   ((uint8_t)0x00) /*!< Reset generated when entering in STANDBY */
#define IS_OB_STDBY_SOURCE(SOURCE) (((SOURCE) == OB_STDBY_NoRST) || ((SOURCE) == OB_STDBY_RST))

/**
  * @}
  */

/** @defgroup Option_Bytes_BOR_Level 
  * @{
  */

#define OB_BOR_OFF       ((uint8_t)0x00) /*!< BOR is disabled at power down, the reset is asserted when the VDD 
                                              power supply reaches the PDR(Power Down Reset) threshold (1.5V) */
#define OB_BOR_LEVEL1    ((uint8_t)0x08) /*!< BOR Reset threshold levels for 1.7V - 1.8V VDD power supply    */
#define OB_BOR_LEVEL2    ((uint8_t)0x09) /*!< BOR Reset threshold levels for 1.9V - 2.0V VDD power supply    */
#define OB_BOR_LEVEL3    ((uint8_t)0x0A) /*!< BOR Reset threshold levels for 2.3V - 2.4V VDD power supply    */
#define OB_BOR_LEVEL4    ((uint8_t)0x0B) /*!< BOR Reset threshold levels for 2.55V - 2.65V VDD power supply  */
#define OB_BOR_LEVEL5    ((uint8_t)0x0C) /*!< BOR Reset threshold levels for 2.8V - 2.9V VDD power supply    */

#define IS_OB_BOR_LEVEL(LEVEL)  (((LEVEL) == OB_BOR_OFF) || \
                                 ((LEVEL) == OB_BOR_LEVEL1) || \
                                 ((LEVEL) == OB_BOR_LEVEL2) || \
                                 ((LEVEL) == OB_BOR_LEVEL3) || \
                                 ((LEVEL) == OB_BOR_LEVEL4) || \
                                 ((LEVEL) == OB_BOR_LEVEL5))

/**
  * @}
  */
  
/** @defgroup FLASH_Flags 
  * @{
  */ 

#define FLASH_FLAG_BSY                 FLASH_SR_BSY  /*!< FLASH Busy flag */
#define FLASH_FLAG_EOP                 FLASH_SR_EOP  /*!< FLASH End of Programming flag */
#define FLASH_FLAG_ENDHV               FLASH_SR_ENHV  /*!< FLASH End of High Voltage flag */
#define FLASH_FLAG_READY               FLASH_SR_READY  /*!< FLASH Ready flag after low power mode */
#define FLASH_FLAG_WRPERR              FLASH_SR_WRPERR  /*!< FLASH Write protected error flag */
#define FLASH_FLAG_PGAERR              FLASH_SR_PGAERR  /*!< FLASH Programming Alignment error flag */
#define FLASH_FLAG_SIZERR              FLASH_SR_SIZERR  /*!< FLASH Size error flag  */
#define FLASH_FLAG_OPTVERR             FLASH_SR_OPTVERR  /*!< FLASH Option Validity error flag  */
 
#define IS_FLASH_CLEAR_FLAG(FLAG) ((((FLAG) & (uint32_t)0xFFFFF0FD) == 0x00000000) && ((FLAG) != 0x00000000))

#define IS_FLASH_GET_FLAG(FLAG)  (((FLAG) == FLASH_FLAG_BSY) || ((FLAG) == FLASH_FLAG_EOP) || \
                                  ((FLAG) == FLASH_FLAG_ENDHV) || ((FLAG) == FLASH_FLAG_READY ) || \
                                  ((FLAG) ==  FLASH_FLAG_WRPERR) || ((FLAG) == FLASH_FLAG_PGAERR ) || \
                                  ((FLAG) ==  FLASH_FLAG_SIZERR) || ((FLAG) == FLASH_FLAG_OPTVERR))
/**
  * @}
  */ 

/** @defgroup FLASH_Keys 
  * @{
  */ 

#define FLASH_PDKEY1               ((uint32_t)0x04152637) /*!< Flash power down key1 */
#define FLASH_PDKEY2               ((uint32_t)0xFAFBFCFD) /*!< Flash power down key2: used with FLASH_PDKEY1 
                                                              to unlock the RUN_PD bit in FLASH_ACR */

#define FLASH_PEKEY1               ((uint32_t)0x89ABCDEF) /*!< Flash program erase key1 */
#define FLASH_PEKEY2               ((uint32_t)0x02030405) /*!< Flash program erase key: used with FLASH_PEKEY2
                                                               to unlock the write access to the FLASH_PECR register and
                                                               data EEPROM */

#define FLASH_PRGKEY1              ((uint32_t)0x8C9DAEBF) /*!< Flash program memory key1 */
#define FLASH_PRGKEY2              ((uint32_t)0x13141516) /*!< Flash program memory key2: used with FLASH_PRGKEY2
                                                               to unlock the program memory */

#define FLASH_OPTKEY1              ((uint32_t)0xFBEAD9C8) /*!< Flash option key1 */
#define FLASH_OPTKEY2              ((uint32_t)0x24252627) /*!< Flash option key2: used with FLASH_OPTKEY1 to
                                                              unlock the write access to the option byte block */
/**
  * @}
  */
  
/** @defgroup Timeout_definition 
  * @{
  */ 
#define FLASH_ER_PRG_TIMEOUT         ((uint32_t)0x8000)

/**
  * @}
  */ 

/**
  * @}
  */ 

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
  
/** 
  * @brief  FLASH memory functions that can be executed from FLASH.  
  */  
/* FLASH Interface configuration functions ************************************/  
void FLASH_SetLatency(uint32_t FLASH_Latency);
void FLASH_PrefetchBufferCmd(FunctionalState NewState);
void FLASH_ReadAccess64Cmd(FunctionalState NewState);
void FLASH_SLEEPPowerDownCmd(FunctionalState NewState);

/* FLASH Memory Programming functions *****************************************/   
void FLASH_Unlock(void);
void FLASH_Lock(void);
FLASH_Status FLASH_ErasePage(uint32_t Page_Address);
FLASH_Status FLASH_FastProgramWord(uint32_t Address, uint32_t Data);

/* DATA EEPROM Programming functions ******************************************/  
void DATA_EEPROM_Unlock(void);
void DATA_EEPROM_Lock(void);
void DATA_EEPROM_FixedTimeProgramCmd(FunctionalState NewState);
FLASH_Status DATA_EEPROM_EraseWord(uint32_t Address);
FLASH_Status DATA_EEPROM_FastProgramByte(uint32_t Address, uint8_t Data);
FLASH_Status DATA_EEPROM_FastProgramHalfWord(uint32_t Address, uint16_t Data);
FLASH_Status DATA_EEPROM_FastProgramWord(uint32_t Address, uint32_t Data);
FLASH_Status DATA_EEPROM_ProgramByte(uint32_t Address, uint8_t Data);
FLASH_Status DATA_EEPROM_ProgramHalfWord(uint32_t Address, uint16_t Data);
FLASH_Status DATA_EEPROM_ProgramWord(uint32_t Address, uint32_t Data);

/* Option Bytes Programming functions *****************************************/
void FLASH_OB_Unlock(void);
void FLASH_OB_Lock(void);
void FLASH_OB_Launch(void);
FLASH_Status FLASH_OB_WRPConfig(uint32_t OB_WRP, FunctionalState NewState);
FLASH_Status FLASH_OB_RDPConfig(uint8_t OB_RDP);
FLASH_Status FLASH_OB_UserConfig(uint8_t OB_IWDG, uint8_t OB_STOP, uint8_t OB_STDBY);
FLASH_Status FLASH_OB_BORConfig(uint8_t OB_BOR);
uint8_t FLASH_OB_GetUser(void);
uint32_t FLASH_OB_GetWRP(void);
FlagStatus FLASH_OB_GetRDP(void);
uint8_t FLASH_OB_GetBOR(void);

/* Interrupts and flags management functions **********************************/  
void FLASH_ITConfig(uint32_t FLASH_IT, FunctionalState NewState);
FlagStatus FLASH_GetFlagStatus(uint32_t FLASH_FLAG);
void FLASH_ClearFlag(uint32_t FLASH_FLAG);
FLASH_Status FLASH_GetStatus(void);
FLASH_Status FLASH_WaitForLastOperation(uint32_t Timeout);

/** 
  * @brief  FLASH memory functions that should be executed from internal SRAM.
  *         These functions are defined inside the "stm32l1xx_flash_ramfunc.c"
  *         file.
  */ 
FLASH_Status FLASH_RUNPowerDownCmd(FunctionalState NewState);
FLASH_Status FLASH_ProgramHalfPage(uint32_t Address, uint32_t* pBuffer);
FLASH_Status DATA_EEPROM_EraseDoubleWord(uint32_t Address);
FLASH_Status DATA_EEPROM_ProgramDoubleWord(uint32_t Address, uint64_t Data);
  
#ifdef __cplusplus
}
#endif

#endif /* __STM32L1xx_FLASH_H */

/**
  * @}
  */

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/

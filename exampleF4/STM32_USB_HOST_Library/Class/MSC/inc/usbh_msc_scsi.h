/**
  ******************************************************************************
  * @file    usbh_msc_scsi.h
  * @author  MCD Application Team
  * @version V2.0.0
  * @date    22-July-2011
  * @brief   Header file for usbh_msc_scsi.c
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

/* Define to prevent recursive  ----------------------------------------------*/
#ifndef __USBH_MSC_SCSI_H__
#define __USBH_MSC_SCSI_H__

/* Includes ------------------------------------------------------------------*/
#include "usbh_stdreq.h"


/** @addtogroup USBH_LIB
  * @{
  */

/** @addtogroup USBH_CLASS
  * @{
  */

/** @addtogroup USBH_MSC_CLASS
  * @{
  */
  
/** @defgroup USBH_MSC_SCSI
  * @brief This file is the Header file for usbh_msc_scsi.c
  * @{
  */ 


/** @defgroup USBH_MSC_SCSI_Exported_Types
  * @{
  */ 
typedef enum {
  USBH_MSC_OK = 0,
  USBH_MSC_FAIL = 1,
  USBH_MSC_PHASE_ERROR = 2,
  USBH_MSC_BUSY = 3
}USBH_MSC_Status_TypeDef;

typedef enum {
  CMD_UNINITIALIZED_STATE =0,
  CMD_SEND_STATE,
  CMD_WAIT_STATUS
} CMD_STATES_TypeDef;  



typedef struct __MassStorageParameter
{
  uint32_t MSCapacity;
  uint32_t MSSenseKey; 
  uint16_t MSPageLength;
  uint8_t MSBulkOutEp;
  uint8_t MSBulkInEp;
  uint8_t MSWriteProtect;
} MassStorageParameter_TypeDef;
/**
  * @}
  */ 



/** @defgroup USBH_MSC_SCSI_Exported_Defines
  * @{
  */ 



#define OPCODE_TEST_UNIT_READY            0X00
#define OPCODE_READ_CAPACITY10            0x25
#define OPCODE_MODE_SENSE6                0x1A
#define OPCODE_READ10                     0x28
#define OPCODE_WRITE10                    0x2A
#define OPCODE_REQUEST_SENSE              0x03

#define DESC_REQUEST_SENSE                0X00
#define ALLOCATION_LENGTH_REQUEST_SENSE   63 
#define XFER_LEN_READ_CAPACITY10           8
#define XFER_LEN_MODE_SENSE6              63

#define MASK_MODE_SENSE_WRITE_PROTECT     0x80
#define MODE_SENSE_PAGE_CONTROL_FIELD     0x00
#define MODE_SENSE_PAGE_CODE              0x3F
#define DISK_WRITE_PROTECTED              0x01
/**
  * @}
  */ 

/** @defgroup USBH_MSC_SCSI_Exported_Macros
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup _Exported_Variables
  * @{
  */ 
extern MassStorageParameter_TypeDef USBH_MSC_Param;
/**
  * @}
  */ 

/** @defgroup USBH_MSC_SCSI_Exported_FunctionsPrototype
  * @{
  */ 
uint8_t USBH_MSC_TestUnitReady(USB_OTG_CORE_HANDLE *pdev);
uint8_t USBH_MSC_ReadCapacity10(USB_OTG_CORE_HANDLE *pdev);
uint8_t USBH_MSC_ModeSense6(USB_OTG_CORE_HANDLE *pdev);
uint8_t USBH_MSC_RequestSense(USB_OTG_CORE_HANDLE *pdev);
uint8_t USBH_MSC_Write10(USB_OTG_CORE_HANDLE *pdev,
                         uint8_t *,
                         uint32_t ,
                         uint32_t );
uint8_t USBH_MSC_Read10(USB_OTG_CORE_HANDLE *pdev,
                        uint8_t *,
                        uint32_t ,
                        uint32_t );
void USBH_MSC_StateMachine(USB_OTG_CORE_HANDLE *pdev);

/**
  * @}
  */ 

#endif  //__USBH_MSC_SCSI_H__


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

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/


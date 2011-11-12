/**
  ******************************************************************************
  * @file    usbh_msc_scsi.c 
  * @author  MCD Application Team
  * @version V2.0.0
  * @date    22-July-2011
  * @brief   This file implements the SCSI commands
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
#include "usbh_msc_core.h"
#include "usbh_msc_scsi.h"
#include "usbh_msc_bot.h"
#include "usbh_ioreq.h"
#include "usbh_def.h"


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
  * @brief    This file includes the mass storage related functions
  * @{
  */ 


/** @defgroup USBH_MSC_SCSI_Private_TypesDefinitions
  * @{
  */ 

MassStorageParameter_TypeDef USBH_MSC_Param; 
/**
  * @}
  */ 

/** @defgroup USBH_MSC_SCSI_Private_Defines
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup USBH_MSC_SCSI_Private_Macros
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup USBH_MSC_SCSI_Private_Variables
  * @{
  */ 
  
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN uint8_t USBH_DataInBuffer[512] __ALIGN_END ;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN uint8_t USBH_DataOutBuffer[512] __ALIGN_END ;
/**
  * @}
  */ 


/** @defgroup USBH_MSC_SCSI_Private_FunctionPrototypes
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup USBH_MSC_SCSI_Exported_Variables
  * @{
  */ 

/**
  * @}
  */ 


/** @defgroup USBH_MSC_SCSI_Private_Functions
  * @{
  */ 




/**
  * @brief  USBH_MSC_TestUnitReady 
  *         Issues 'Test unit ready' command to the device. Once the response  
  *         received, it updates the status to upper layer.
  * @param  None
  * @retval Status
  */
uint8_t USBH_MSC_TestUnitReady (USB_OTG_CORE_HANDLE *pdev)
{
  uint8_t index;
  USBH_MSC_Status_TypeDef status = USBH_MSC_BUSY;
  
  if(HCD_IsDeviceConnected(pdev))
  {  
    switch(USBH_MSC_BOTXferParam.CmdStateMachine)
    {
    case CMD_SEND_STATE:  
      /*Prepare the CBW and relevent field*/
      USBH_MSC_CBWData.field.CBWTransferLength = 0;       /* No Data Transfer */
      USBH_MSC_CBWData.field.CBWFlags = USB_EP_DIR_OUT;
      USBH_MSC_CBWData.field.CBWLength = CBW_LENGTH_TEST_UNIT_READY;
      USBH_MSC_BOTXferParam.pRxTxBuff = USBH_MSC_CSWData.CSWArray;
      USBH_MSC_BOTXferParam.DataLength = USBH_MSC_CSW_MAX_LENGTH;
      USBH_MSC_BOTXferParam.MSCStateCurrent = USBH_MSC_TEST_UNIT_READY;
      
      for(index = CBW_CB_LENGTH; index != 0; index--)
      {
        USBH_MSC_CBWData.field.CBWCB[index] = 0x00;
      }
      
      USBH_MSC_CBWData.field.CBWCB[0]  = OPCODE_TEST_UNIT_READY; 
      USBH_MSC_BOTXferParam.BOTState = USBH_MSC_SEND_CBW;
      /* Start the transfer, then let the state 
      machine magage the other transactions */
      USBH_MSC_BOTXferParam.MSCState = USBH_MSC_BOT_USB_TRANSFERS;
      USBH_MSC_BOTXferParam.BOTXferStatus = USBH_MSC_BUSY;
      USBH_MSC_BOTXferParam.CmdStateMachine = CMD_WAIT_STATUS;
      
      status = USBH_MSC_BUSY; 
      break;
      
    case CMD_WAIT_STATUS: 
      if(USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_OK)
      { 
        /* Commands successfully sent and Response Received  */       
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
       
        status = USBH_MSC_OK;      
      }
      else if ( USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_FAIL )
      {
        /* Failure Mode */
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_FAIL;
      }
      
      else if ( USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_PHASE_ERROR )
      {
        /* Failure Mode */
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_PHASE_ERROR;    
      }  
      break;
      
    default:
      break;
    }
  }
  return status;
}


/**
  * @brief  USBH_MSC_ReadCapacity10  
  *         Issue the read capacity command to the device. Once the response 
  *         received, it updates the status to upper layer
  * @param  None
  * @retval Status
  */
uint8_t USBH_MSC_ReadCapacity10(USB_OTG_CORE_HANDLE *pdev)
{
  uint8_t index;
  USBH_MSC_Status_TypeDef status = USBH_MSC_BUSY;
  
  if(HCD_IsDeviceConnected(pdev))
  {  
    switch(USBH_MSC_BOTXferParam.CmdStateMachine)
    {
    case CMD_SEND_STATE:
      /*Prepare the CBW and relevent field*/
      USBH_MSC_CBWData.field.CBWTransferLength = XFER_LEN_READ_CAPACITY10;
      USBH_MSC_CBWData.field.CBWFlags = USB_EP_DIR_IN;
      USBH_MSC_CBWData.field.CBWLength = CBW_LENGTH;
      
      USBH_MSC_BOTXferParam.pRxTxBuff = USBH_DataInBuffer;
      USBH_MSC_BOTXferParam.MSCStateCurrent = USBH_MSC_READ_CAPACITY10;
      
      for(index = CBW_CB_LENGTH; index != 0; index--)
      {
        USBH_MSC_CBWData.field.CBWCB[index] = 0x00;
      }    
      
      USBH_MSC_CBWData.field.CBWCB[0]  = OPCODE_READ_CAPACITY10; 
      USBH_MSC_BOTXferParam.BOTState = USBH_MSC_SEND_CBW;
      
      /* Start the transfer, then let the state machine manage the other 
                                                                transactions */
      USBH_MSC_BOTXferParam.MSCState = USBH_MSC_BOT_USB_TRANSFERS;
      USBH_MSC_BOTXferParam.BOTXferStatus = USBH_MSC_BUSY;
      USBH_MSC_BOTXferParam.CmdStateMachine = CMD_WAIT_STATUS;
      
      status = USBH_MSC_BUSY;
      break;
      
    case CMD_WAIT_STATUS:
      if(USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_OK)
      {
        /*assign the capacity*/
        (((uint8_t*)&USBH_MSC_Param.MSCapacity )[3]) = USBH_DataInBuffer[0];
        (((uint8_t*)&USBH_MSC_Param.MSCapacity )[2]) = USBH_DataInBuffer[1];
        (((uint8_t*)&USBH_MSC_Param.MSCapacity )[1]) = USBH_DataInBuffer[2];
        (((uint8_t*)&USBH_MSC_Param.MSCapacity )[0]) = USBH_DataInBuffer[3];
        
        /*assign the page length*/
        (((uint8_t*)&USBH_MSC_Param.MSPageLength )[1]) = USBH_DataInBuffer[6];
        (((uint8_t*)&USBH_MSC_Param.MSPageLength )[0]) = USBH_DataInBuffer[7];
        
        /* Commands successfully sent and Response Received  */       
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_OK;      
      }
      else if ( USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_FAIL )
      {
        /* Failure Mode */
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_FAIL;
      }  
      else if ( USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_PHASE_ERROR )
      {
        /* Failure Mode */
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_PHASE_ERROR;    
      } 
      else
      {
        /* Wait for the Commands to get Completed */
        /* NO Change in state Machine */
      }
      break;
      
    default:
      break;
    }
  }
  return status;
}


/**
  * @brief  USBH_MSC_ModeSense6  
  *         Issue the Mode Sense6 Command to the device. This function is used 
  *          for reading the WriteProtect Status of the Mass-Storage device. 
  * @param  None
  * @retval Status
  */
uint8_t USBH_MSC_ModeSense6(USB_OTG_CORE_HANDLE *pdev)
{
  uint8_t index;
  USBH_MSC_Status_TypeDef status = USBH_MSC_BUSY;
  
  if(HCD_IsDeviceConnected(pdev))
  {  
    switch(USBH_MSC_BOTXferParam.CmdStateMachine)
    {
    case CMD_SEND_STATE:
      /*Prepare the CBW and relevent field*/
      USBH_MSC_CBWData.field.CBWTransferLength = XFER_LEN_MODE_SENSE6;
      USBH_MSC_CBWData.field.CBWFlags = USB_EP_DIR_IN;
      USBH_MSC_CBWData.field.CBWLength = CBW_LENGTH;
      
      USBH_MSC_BOTXferParam.pRxTxBuff = USBH_DataInBuffer;
      USBH_MSC_BOTXferParam.MSCStateCurrent = USBH_MSC_MODE_SENSE6;
      
      for(index = CBW_CB_LENGTH; index != 0; index--)
      {
        USBH_MSC_CBWData.field.CBWCB[index] = 0x00;
      }    
      
      USBH_MSC_CBWData.field.CBWCB[0]  = OPCODE_MODE_SENSE6; 
      USBH_MSC_CBWData.field.CBWCB[2]  = MODE_SENSE_PAGE_CONTROL_FIELD | \
                                         MODE_SENSE_PAGE_CODE;
                                           
      USBH_MSC_CBWData.field.CBWCB[4]  = XFER_LEN_MODE_SENSE6;
                                                                                      
      USBH_MSC_BOTXferParam.BOTState = USBH_MSC_SEND_CBW;
      
      /* Start the transfer, then let the state machine manage the other 
                                                                transactions */
      USBH_MSC_BOTXferParam.MSCState = USBH_MSC_BOT_USB_TRANSFERS;
      USBH_MSC_BOTXferParam.BOTXferStatus = USBH_MSC_BUSY;
      USBH_MSC_BOTXferParam.CmdStateMachine = CMD_WAIT_STATUS;
      
      status = USBH_MSC_BUSY;
      break;
      
    case CMD_WAIT_STATUS:
      if(USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_OK)
      {
        /* Assign the Write Protect status */
        /* If WriteProtect = 0, Writing is allowed 
           If WriteProtect != 0, Disk is Write Protected */
        if ( USBH_DataInBuffer[2] & MASK_MODE_SENSE_WRITE_PROTECT)
        {
          USBH_MSC_Param.MSWriteProtect   = DISK_WRITE_PROTECTED;
        }
        else
        {
          USBH_MSC_Param.MSWriteProtect   = 0;
        }
        
        /* Commands successfully sent and Response Received  */       
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_OK;      
      }
      else if ( USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_FAIL )
      {
        /* Failure Mode */
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_FAIL;
      }
      else if ( USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_PHASE_ERROR )
      {
        /* Failure Mode */
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_PHASE_ERROR;    
      }
      else
      {
        /* Wait for the Commands to get Completed */
        /* NO Change in state Machine */
      }
      break;
      
    default:
      break;
    }
  }
  return status;
}

/**
  * @brief  USBH_MSC_RequestSense  
  *         Issues the Request Sense command to the device. Once the response 
  *         received, it updates the status to upper layer
  * @param  None
  * @retval Status
  */
uint8_t USBH_MSC_RequestSense(USB_OTG_CORE_HANDLE *pdev)
{
  USBH_MSC_Status_TypeDef status = USBH_MSC_BUSY;
  
  uint8_t index;
  
  
  if(HCD_IsDeviceConnected(pdev))
  {  
    switch(USBH_MSC_BOTXferParam.CmdStateMachine)
    {
    case CMD_SEND_STATE:
      
      /*Prepare the CBW and relevent field*/
      USBH_MSC_CBWData.field.CBWTransferLength = \
                                                ALLOCATION_LENGTH_REQUEST_SENSE;
      USBH_MSC_CBWData.field.CBWFlags = USB_EP_DIR_IN;
      USBH_MSC_CBWData.field.CBWLength = CBW_LENGTH;
      
      USBH_MSC_BOTXferParam.pRxTxBuff = USBH_DataInBuffer;
      USBH_MSC_BOTXferParam.MSCStateBkp = USBH_MSC_BOTXferParam.MSCStateCurrent;
      USBH_MSC_BOTXferParam.MSCStateCurrent = USBH_MSC_REQUEST_SENSE;
      

      for(index = CBW_CB_LENGTH; index != 0; index--)
      {
        USBH_MSC_CBWData.field.CBWCB[index] = 0x00;
      }    
      
      USBH_MSC_CBWData.field.CBWCB[0]  = OPCODE_REQUEST_SENSE; 
      USBH_MSC_CBWData.field.CBWCB[1]  = DESC_REQUEST_SENSE;
      USBH_MSC_CBWData.field.CBWCB[4]  = ALLOCATION_LENGTH_REQUEST_SENSE;
      
      USBH_MSC_BOTXferParam.BOTState = USBH_MSC_SEND_CBW;
      /* Start the transfer, then let the state machine magage 
      the other transactions */
      USBH_MSC_BOTXferParam.MSCState = USBH_MSC_BOT_USB_TRANSFERS;
      USBH_MSC_BOTXferParam.BOTXferStatus = USBH_MSC_BUSY;
      USBH_MSC_BOTXferParam.CmdStateMachine = CMD_WAIT_STATUS;
      
      status = USBH_MSC_BUSY;
      
      break;
      
    case CMD_WAIT_STATUS:
      
      if(USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_OK)
      {
        /* Get Sense data*/
        (((uint8_t*)&USBH_MSC_Param.MSSenseKey )[3]) = USBH_DataInBuffer[0];
        (((uint8_t*)&USBH_MSC_Param.MSSenseKey )[2]) = USBH_DataInBuffer[1];
        (((uint8_t*)&USBH_MSC_Param.MSSenseKey )[1]) = USBH_DataInBuffer[2];
        (((uint8_t*)&USBH_MSC_Param.MSSenseKey )[0]) = USBH_DataInBuffer[3];
        
        /* Commands successfully sent and Response Received  */       
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_OK;      
      }
      else if ( USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_FAIL )
      {
        /* Failure Mode */
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_FAIL;
      }
      
      else if ( USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_PHASE_ERROR )
      {
        /* Failure Mode */
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_PHASE_ERROR;    
      }
      
      else
      {
        /* Wait for the Commands to get Completed */
        /* NO Change in state Machine */
      }
      break;
      
    default:
      break;
    }
  }
  return status;
}


/**
  * @brief  USBH_MSC_Write10 
  *         Issue the write command to the device. Once the response received, 
  *         it updates the status to upper layer
  * @param  dataBuffer : DataBuffer contains the data to write
  * @param  address : Address to which the data will be written
  * @param  nbOfbytes : NbOfbytes to be written
  * @retval Status
  */
uint8_t USBH_MSC_Write10(USB_OTG_CORE_HANDLE *pdev, 
                         uint8_t *dataBuffer,
                         uint32_t address,
                         uint32_t nbOfbytes)
{
  uint8_t index;
  USBH_MSC_Status_TypeDef status = USBH_MSC_BUSY;
  uint16_t nbOfPages;
  
  if(HCD_IsDeviceConnected(pdev))
  {  
    switch(USBH_MSC_BOTXferParam.CmdStateMachine)
    {
    case CMD_SEND_STATE:   
      USBH_MSC_CBWData.field.CBWTransferLength = nbOfbytes;
      USBH_MSC_CBWData.field.CBWFlags = USB_EP_DIR_OUT;
      USBH_MSC_CBWData.field.CBWLength = CBW_LENGTH;
      USBH_MSC_BOTXferParam.pRxTxBuff = dataBuffer;
      
      
      for(index = CBW_CB_LENGTH; index != 0; index--)  
      {
        USBH_MSC_CBWData.field.CBWCB[index] = 0x00;
      }
      
      USBH_MSC_CBWData.field.CBWCB[0]  = OPCODE_WRITE10; 
      
      /*logical block address*/
      USBH_MSC_CBWData.field.CBWCB[2]  = (((uint8_t*)&address)[3]) ;
      USBH_MSC_CBWData.field.CBWCB[3]  = (((uint8_t*)&address)[2]);
      USBH_MSC_CBWData.field.CBWCB[4]  = (((uint8_t*)&address)[1]);
      USBH_MSC_CBWData.field.CBWCB[5]  = (((uint8_t*)&address)[0]);
      
      /*USBH_MSC_PAGE_LENGTH = 512*/
      nbOfPages = nbOfbytes/ USBH_MSC_PAGE_LENGTH; 
      
      /*Tranfer length */
      USBH_MSC_CBWData.field.CBWCB[7]  = (((uint8_t *)&nbOfPages)[1]) ; 
      USBH_MSC_CBWData.field.CBWCB[8]  = (((uint8_t *)&nbOfPages)[0]) ; 
      
      USBH_MSC_BOTXferParam.BOTState = USBH_MSC_SEND_CBW;
      /* Start the transfer, then let the state machine 
      magage the other transactions */
      USBH_MSC_BOTXferParam.MSCState = USBH_MSC_BOT_USB_TRANSFERS;
      USBH_MSC_BOTXferParam.BOTXferStatus = USBH_MSC_BUSY;
      USBH_MSC_BOTXferParam.CmdStateMachine = CMD_WAIT_STATUS;
      
      status = USBH_MSC_BUSY;
      
      break;
      
    case CMD_WAIT_STATUS:
      if(USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_OK)
      { 
        /* Commands successfully sent and Response Received  */       
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_OK;      
      }
      else if ( USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_FAIL )
      {
        /* Failure Mode */
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
      }
      
      else if ( USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_PHASE_ERROR )
      {
        /* Failure Mode */
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_PHASE_ERROR;    
      }
      break;
      
    default:
      break;
    }
  }
  return status;
}

/**
  * @brief  USBH_MSC_Read10 
  *         Issue the read command to the device. Once the response received, 
  *         it updates the status to upper layer
  * @param  dataBuffer : DataBuffer will contain the data to be read
  * @param  address : Address from which the data will be read
  * @param  nbOfbytes : NbOfbytes to be read
  * @retval Status
  */
uint8_t USBH_MSC_Read10(USB_OTG_CORE_HANDLE *pdev,
                        uint8_t *dataBuffer,
                        uint32_t address,
                        uint32_t nbOfbytes)
{
  uint8_t index;
  static USBH_MSC_Status_TypeDef status = USBH_MSC_BUSY;
  uint16_t nbOfPages;
  status = USBH_MSC_BUSY;
  
  if(HCD_IsDeviceConnected(pdev))
  {
    switch(USBH_MSC_BOTXferParam.CmdStateMachine)
    {
    case CMD_SEND_STATE:
      /*Prepare the CBW and relevent field*/
      USBH_MSC_CBWData.field.CBWTransferLength = nbOfbytes;
      USBH_MSC_CBWData.field.CBWFlags = USB_EP_DIR_IN;
      USBH_MSC_CBWData.field.CBWLength = CBW_LENGTH;
      
      USBH_MSC_BOTXferParam.pRxTxBuff = dataBuffer;
      
      for(index = CBW_CB_LENGTH; index != 0; index--)
      {
        USBH_MSC_CBWData.field.CBWCB[index] = 0x00;
      }
      
      USBH_MSC_CBWData.field.CBWCB[0]  = OPCODE_READ10; 
      
      /*logical block address*/
      
      USBH_MSC_CBWData.field.CBWCB[2]  = (((uint8_t*)&address)[3]);
      USBH_MSC_CBWData.field.CBWCB[3]  = (((uint8_t*)&address)[2]);
      USBH_MSC_CBWData.field.CBWCB[4]  = (((uint8_t*)&address)[1]);
      USBH_MSC_CBWData.field.CBWCB[5]  = (((uint8_t*)&address)[0]);
      
      /*USBH_MSC_PAGE_LENGTH = 512*/
      nbOfPages = nbOfbytes/ USBH_MSC_PAGE_LENGTH;  
      
      /*Tranfer length */
      USBH_MSC_CBWData.field.CBWCB[7]  = (((uint8_t *)&nbOfPages)[1]) ; 
      USBH_MSC_CBWData.field.CBWCB[8]  = (((uint8_t *)&nbOfPages)[0]) ; 
      
      
      USBH_MSC_BOTXferParam.BOTState = USBH_MSC_SEND_CBW;
      /* Start the transfer, then let the state machine 
      magage the other transactions */
      USBH_MSC_BOTXferParam.MSCState = USBH_MSC_BOT_USB_TRANSFERS;
      USBH_MSC_BOTXferParam.BOTXferStatus = USBH_MSC_BUSY;
      USBH_MSC_BOTXferParam.CmdStateMachine = CMD_WAIT_STATUS;
      
      status = USBH_MSC_BUSY;
      
      break;
      
    case CMD_WAIT_STATUS:
      
      if((USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_OK) && \
        (HCD_IsDeviceConnected(pdev)))
      { 
        /* Commands successfully sent and Response Received  */       
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_OK;      
      }
      else if (( USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_FAIL ) && \
        (HCD_IsDeviceConnected(pdev)))
      {
        /* Failure Mode */
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
      }
      
      else if ( USBH_MSC_BOTXferParam.BOTXferStatus == USBH_MSC_PHASE_ERROR )
      {
        /* Failure Mode */
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
        status = USBH_MSC_PHASE_ERROR;    
      }
      else
      {
        /* Wait for the Commands to get Completed */
        /* NO Change in state Machine */
      }
      break;
      
    default:
      break;
    }
  }
  return status;
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

/**
  * @}
  */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/




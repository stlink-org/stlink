/**
  ******************************************************************************
  * @file    usbh_msc_bot.c 
  * @author  MCD Application Team
  * @version V2.0.0
  * @date    22-July-2011
  * @brief   This file includes the mass storage related functions
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
#include "usb_hcd_int.h"


/** @addtogroup USBH_LIB
* @{
*/

/** @addtogroup USBH_CLASS
* @{
*/

/** @addtogroup USBH_MSC_CLASS
* @{
*/

/** @defgroup USBH_MSC_BOT 
* @brief    This file includes the mass storage related functions
* @{
*/ 


/** @defgroup USBH_MSC_BOT_Private_TypesDefinitions
* @{
*/ 
/**
* @}
*/ 

/** @defgroup USBH_MSC_BOT_Private_Defines
* @{
*/ 
/**
* @}
*/ 

/** @defgroup USBH_MSC_BOT_Private_Macros
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_MSC_BOT_Private_Variables
* @{
*/ 

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */ 
__ALIGN_BEGIN HostCBWPkt_TypeDef USBH_MSC_CBWData __ALIGN_END ;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN HostCSWPkt_TypeDef USBH_MSC_CSWData __ALIGN_END ;


static uint32_t BOTStallErrorCount;   /* Keeps count of STALL Error Cases*/

/**
* @}
*/ 


/** @defgroup USBH_MSC_BOT_Private_FunctionPrototypes
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_MSC_BOT_Exported_Variables
* @{
*/ 
USBH_BOTXfer_TypeDef USBH_MSC_BOTXferParam; 
/**
* @}
*/ 


/** @defgroup USBH_MSC_BOT_Private_Functions
* @{
*/ 


/**
* @brief  USBH_MSC_Init 
*         Initializes the mass storage parameters
* @param  None
* @retval None
*/
void USBH_MSC_Init(USB_OTG_CORE_HANDLE *pdev )
{
  if(HCD_IsDeviceConnected(pdev))
  {      
    USBH_MSC_CBWData.field.CBWSignature = USBH_MSC_BOT_CBW_SIGNATURE;
    USBH_MSC_CBWData.field.CBWTag = USBH_MSC_BOT_CBW_TAG;
    USBH_MSC_CBWData.field.CBWLUN = 0;  /*Only one LUN is supported*/
    USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;  
  }
  
  BOTStallErrorCount = 0;
  MSCErrorCount = 0;
}

/**
* @brief  USBH_MSC_HandleBOTXfer 
*         This function manages the different states of BOT transfer and 
*         updates the status to upper layer.
* @param  None
* @retval None
* 
*/
void USBH_MSC_HandleBOTXfer (USB_OTG_CORE_HANDLE *pdev ,USBH_HOST *phost)
{
  uint8_t xferDirection, index;
  static uint32_t remainingDataLength;
  static uint8_t *datapointer;
  static uint8_t error_direction;
  USBH_Status status;
  
  URB_STATE URB_Status = URB_IDLE;
  
  if(HCD_IsDeviceConnected(pdev))
  {  
    
    switch (USBH_MSC_BOTXferParam.BOTState)
    {
    case USBH_MSC_SEND_CBW:
      /* send CBW */    
      USBH_BulkSendData (pdev,
                         &USBH_MSC_CBWData.CBWArray[0], 
                         USBH_MSC_BOT_CBW_PACKET_LENGTH , 
                         MSC_Machine.hc_num_out);
      
      USBH_MSC_BOTXferParam.BOTStateBkp = USBH_MSC_SEND_CBW;
      USBH_MSC_BOTXferParam.BOTState = USBH_MSC_SENT_CBW;
      
      break;
      
    case USBH_MSC_SENT_CBW:
      URB_Status = HCD_GetURB_State(pdev , MSC_Machine.hc_num_out);
      
      if(URB_Status == URB_DONE)
      { 
        BOTStallErrorCount = 0;
        USBH_MSC_BOTXferParam.BOTStateBkp = USBH_MSC_SENT_CBW; 
        
        /* If the CBW Pkt is sent successful, then change the state */
        xferDirection = (USBH_MSC_CBWData.field.CBWFlags & USB_REQ_DIR_MASK);
        
        if ( USBH_MSC_CBWData.field.CBWTransferLength != 0 )
        {
          remainingDataLength = USBH_MSC_CBWData.field.CBWTransferLength ;
          datapointer = USBH_MSC_BOTXferParam.pRxTxBuff;
          
          /* If there is Data Transfer Stage */
          if (xferDirection == USB_D2H)
          {
            /* Data Direction is IN */
            USBH_MSC_BOTXferParam.BOTState = USBH_MSC_BOT_DATAIN_STATE;
          }
          else
          {
            /* Data Direction is OUT */
            USBH_MSC_BOTXferParam.BOTState = USBH_MSC_BOT_DATAOUT_STATE;
          } 
        }
        
        else
        {/* If there is NO Data Transfer Stage */
          USBH_MSC_BOTXferParam.BOTState = USBH_MSC_RECEIVE_CSW_STATE;
        }
        
      }   
      else if(URB_Status == URB_NOTREADY)
      {
        USBH_MSC_BOTXferParam.BOTState  = USBH_MSC_BOTXferParam.BOTStateBkp;    
      }     
      else if(URB_Status == URB_STALL)
      {
        error_direction = USBH_MSC_DIR_OUT;
        USBH_MSC_BOTXferParam.BOTState  = USBH_MSC_BOT_ERROR_OUT;
      }
      break;
      
    case USBH_MSC_BOT_DATAIN_STATE:
      
      URB_Status =   HCD_GetURB_State(pdev , MSC_Machine.hc_num_in);
      /* BOT DATA IN stage */
      if((URB_Status == URB_DONE) ||(USBH_MSC_BOTXferParam.BOTStateBkp != USBH_MSC_BOT_DATAIN_STATE))
      {
        BOTStallErrorCount = 0;
        USBH_MSC_BOTXferParam.BOTStateBkp = USBH_MSC_BOT_DATAIN_STATE;    
        
        if(remainingDataLength > USBH_MSC_MPS_SIZE)
        {
          USBH_BulkReceiveData (pdev,
	                        datapointer, 
			        USBH_MSC_MPS_SIZE , 
			        MSC_Machine.hc_num_in);
          
          remainingDataLength -= USBH_MSC_MPS_SIZE;
          datapointer = datapointer + USBH_MSC_MPS_SIZE;
        }
        else if ( remainingDataLength == 0)
        {
          /* If value was 0, and successful transfer, then change the state */
          USBH_MSC_BOTXferParam.BOTState = USBH_MSC_RECEIVE_CSW_STATE;
        }
        else
        {       
          USBH_BulkReceiveData (pdev,
	                        datapointer, 
			        remainingDataLength , 
			        MSC_Machine.hc_num_in);
          
          remainingDataLength = 0; /* Reset this value and keep in same state */
        }
      }
      else if(URB_Status == URB_STALL)
      {
        /* This is Data Stage STALL Condition */
        
        error_direction = USBH_MSC_DIR_IN;
        USBH_MSC_BOTXferParam.BOTState  = USBH_MSC_BOT_ERROR_IN;
        
        /* Refer to USB Mass-Storage Class : BOT (www.usb.org) 
        6.7.2 Host expects to receive data from the device
        3. On a STALL condition receiving data, then:
        The host shall accept the data received.
        The host shall clear the Bulk-In pipe.
        4. The host shall attempt to receive a CSW.
        
        USBH_MSC_BOTXferParam.BOTStateBkp is used to switch to the Original 
        state after the ClearFeature Command is issued.
        */
        USBH_MSC_BOTXferParam.BOTStateBkp = USBH_MSC_RECEIVE_CSW_STATE;
        
      }     
      break;   
      
      
    case USBH_MSC_BOT_DATAOUT_STATE:
      /* BOT DATA OUT stage */
      URB_Status = HCD_GetURB_State(pdev , MSC_Machine.hc_num_out);       
      if(URB_Status == URB_DONE)
      {
        BOTStallErrorCount = 0;
        USBH_MSC_BOTXferParam.BOTStateBkp = USBH_MSC_BOT_DATAOUT_STATE;    
        if(remainingDataLength > USBH_MSC_MPS_SIZE)
        {
          USBH_BulkSendData (pdev,
                             datapointer, 
                             USBH_MSC_MPS_SIZE , 
                             MSC_Machine.hc_num_out);
          datapointer = datapointer + USBH_MSC_MPS_SIZE;
          remainingDataLength = remainingDataLength - USBH_MSC_MPS_SIZE;
        }
        else if ( remainingDataLength == 0)
        {
          /* If value was 0, and successful transfer, then change the state */
          USBH_MSC_BOTXferParam.BOTState = USBH_MSC_RECEIVE_CSW_STATE;
        }
        else
        {
          USBH_BulkSendData (pdev,
	                     datapointer, 
			     remainingDataLength , 
			     MSC_Machine.hc_num_out);
          
          remainingDataLength = 0; /* Reset this value and keep in same state */   
        }      
      }
      
      else if(URB_Status == URB_NOTREADY)
      {
        USBH_BulkSendData (pdev,
	                   (datapointer - USBH_MSC_MPS_SIZE), 
			   USBH_MSC_MPS_SIZE , 
			   MSC_Machine.hc_num_out);
      }
      
      else if(URB_Status == URB_STALL)
      {
        error_direction = USBH_MSC_DIR_OUT;
        USBH_MSC_BOTXferParam.BOTState  = USBH_MSC_BOT_ERROR_OUT;
        
        /* Refer to USB Mass-Storage Class : BOT (www.usb.org) 
        6.7.3 Ho - Host expects to send data to the device
        3. On a STALL condition sending data, then:
        " The host shall clear the Bulk-Out pipe.
        4. The host shall attempt to receive a CSW.
        
        The Above statement will do the clear the Bulk-Out pipe.
        The Below statement will help in Getting the CSW.  
        
        USBH_MSC_BOTXferParam.BOTStateBkp is used to switch to the Original 
        state after the ClearFeature Command is issued.
        */
        
        USBH_MSC_BOTXferParam.BOTStateBkp = USBH_MSC_RECEIVE_CSW_STATE;
        
      }
      break;
      
    case USBH_MSC_RECEIVE_CSW_STATE:
      /* BOT CSW stage */     
        /* NOTE: We cannot reset the BOTStallErrorCount here as it may come from 
        the clearFeature from previous command */
        
        USBH_MSC_BOTXferParam.BOTStateBkp = USBH_MSC_RECEIVE_CSW_STATE;
        
        USBH_MSC_BOTXferParam.pRxTxBuff = USBH_MSC_CSWData.CSWArray;
        USBH_MSC_BOTXferParam.DataLength = USBH_MSC_CSW_MAX_LENGTH;
        
        for(index = USBH_MSC_CSW_LENGTH; index != 0; index--)
        {
          USBH_MSC_CSWData.CSWArray[index] = 0;
        }
        
        USBH_MSC_CSWData.CSWArray[0] = 0;
        
        USBH_BulkReceiveData (pdev,
                              USBH_MSC_BOTXferParam.pRxTxBuff, 
                              USBH_MSC_CSW_MAX_LENGTH , 
                              MSC_Machine.hc_num_in);
        USBH_MSC_BOTXferParam.BOTState = USBH_MSC_DECODE_CSW;    

      break;
      
    case USBH_MSC_DECODE_CSW:
      URB_Status = HCD_GetURB_State(pdev , MSC_Machine.hc_num_in);
      /* Decode CSW */
      if(URB_Status == URB_DONE)
      {
        BOTStallErrorCount = 0;
        USBH_MSC_BOTXferParam.BOTStateBkp = USBH_MSC_RECEIVE_CSW_STATE;
        
        USBH_MSC_BOTXferParam.MSCState = USBH_MSC_BOTXferParam.MSCStateCurrent ;
        
        USBH_MSC_BOTXferParam.BOTXferStatus = USBH_MSC_DecodeCSW(pdev , phost);
      }
      else if(URB_Status == URB_STALL)     
      {
        error_direction = USBH_MSC_DIR_IN;
        USBH_MSC_BOTXferParam.BOTState  = USBH_MSC_BOT_ERROR_IN;
      }
      break;
      
    case USBH_MSC_BOT_ERROR_IN: 
      status = USBH_MSC_BOT_Abort(pdev, phost, USBH_MSC_DIR_IN);
      if (status == USBH_OK)
      {
        /* Check if the error was due in Both the directions */
        if (error_direction == USBH_MSC_BOTH_DIR)
        {/* If Both directions are Needed, Switch to OUT Direction */
          USBH_MSC_BOTXferParam.BOTState = USBH_MSC_BOT_ERROR_OUT;
        }
        else
        {
          /* Switch Back to the Original State, In many cases this will be 
          USBH_MSC_RECEIVE_CSW_STATE state */
          USBH_MSC_BOTXferParam.BOTState = USBH_MSC_BOTXferParam.BOTStateBkp;
        }
      }
      else if (status == USBH_UNRECOVERED_ERROR)
      {
        /* This means that there is a STALL Error limit, Do Reset Recovery */
        USBH_MSC_BOTXferParam.BOTXferStatus = USBH_MSC_PHASE_ERROR;
      }
      break;
      
    case USBH_MSC_BOT_ERROR_OUT: 
      status = USBH_MSC_BOT_Abort(pdev, phost, USBH_MSC_DIR_OUT);
      if ( status == USBH_OK)
      { /* Switch Back to the Original State */
        USBH_MSC_BOTXferParam.BOTState = USBH_MSC_BOTXferParam.BOTStateBkp;        
      }
      else if (status == USBH_UNRECOVERED_ERROR)
      {
        /* This means that there is a STALL Error limit, Do Reset Recovery */
        USBH_MSC_BOTXferParam.BOTXferStatus = USBH_MSC_PHASE_ERROR;
      }
      break;
      
    default:      
      break;
    }
  }
}

/**
* @brief  USBH_MSC_BOT_Abort 
*         This function manages the different Error handling for STALL
* @param  direction : IN / OUT 
* @retval None
*/
USBH_Status USBH_MSC_BOT_Abort(USB_OTG_CORE_HANDLE *pdev, 
                               USBH_HOST *phost,
                               uint8_t direction)
{
  USBH_Status status;
  
  status = USBH_BUSY;
  
  switch (direction)
  {
  case USBH_MSC_DIR_IN :
    /* send ClrFeture on Bulk IN endpoint */
    status = USBH_ClrFeature(pdev,
                             phost,
                             MSC_Machine.MSBulkInEp,
                             MSC_Machine.hc_num_in);
    
    break;
    
  case USBH_MSC_DIR_OUT :
    /*send ClrFeature on Bulk OUT endpoint */
    status = USBH_ClrFeature(pdev, 
                             phost,
                             MSC_Machine.MSBulkOutEp,
                             MSC_Machine.hc_num_out);
    break;
    
  default:
    break;
  }
  
  BOTStallErrorCount++; /* Check Continous Number of times, STALL has Occured */ 
  if (BOTStallErrorCount > MAX_BULK_STALL_COUNT_LIMIT )
  {
    status = USBH_UNRECOVERED_ERROR;
  }
  
  return status;
}

/**
* @brief  USBH_MSC_DecodeCSW
*         This function decodes the CSW received by the device and updates the
*         same to upper layer.
* @param  None
* @retval On success USBH_MSC_OK, on failure USBH_MSC_FAIL
* @notes
*     Refer to USB Mass-Storage Class : BOT (www.usb.org)
*    6.3.1 Valid CSW Conditions :
*     The host shall consider the CSW valid when:
*     1. dCSWSignature is equal to 53425355h
*     2. the CSW is 13 (Dh) bytes in length,
*     3. dCSWTag matches the dCBWTag from the corresponding CBW.
*/

uint8_t USBH_MSC_DecodeCSW(USB_OTG_CORE_HANDLE *pdev , USBH_HOST *phost)
{
  uint8_t status;
  uint32_t dataXferCount = 0;
  status = USBH_MSC_FAIL;
  
  if(HCD_IsDeviceConnected(pdev))
  {
    /*Checking if the transfer length is diffrent than 13*/
    dataXferCount = HCD_GetXferCnt(pdev, MSC_Machine.hc_num_in); 
    
    if(dataXferCount != USBH_MSC_CSW_LENGTH)
    {
      /*(4) Hi > Dn (Host expects to receive data from the device,
      Device intends to transfer no data)
      (5) Hi > Di (Host expects to receive data from the device,
      Device intends to send data to the host)
      (9) Ho > Dn (Host expects to send data to the device,
      Device intends to transfer no data)
      (11) Ho > Do  (Host expects to send data to the device,
      Device intends to receive data from the host)*/
      
      
      status = USBH_MSC_PHASE_ERROR;
    }
    else
    { /* CSW length is Correct */
      
      /* Check validity of the CSW Signature and CSWStatus */
      if(USBH_MSC_CSWData.field.CSWSignature == USBH_MSC_BOT_CSW_SIGNATURE)
      {/* Check Condition 1. dCSWSignature is equal to 53425355h */
        
        if(USBH_MSC_CSWData.field.CSWTag == USBH_MSC_CBWData.field.CBWTag)
        {
          /* Check Condition 3. dCSWTag matches the dCBWTag from the 
          corresponding CBW */
          
          if(USBH_MSC_CSWData.field.CSWStatus == USBH_MSC_OK) 
          {
            /* Refer to USB Mass-Storage Class : BOT (www.usb.org) 
            
            Hn Host expects no data transfers
            Hi Host expects to receive data from the device
            Ho Host expects to send data to the device
            
            Dn Device intends to transfer no data
            Di Device intends to send data to the host
            Do Device intends to receive data from the host
            
            Section 6.7 
            (1) Hn = Dn (Host expects no data transfers,
            Device intends to transfer no data)
            (6) Hi = Di (Host expects to receive data from the device,
            Device intends to send data to the host)
            (12) Ho = Do (Host expects to send data to the device, 
            Device intends to receive data from the host)
            
            */
            
            status = USBH_MSC_OK;
          }
          else if(USBH_MSC_CSWData.field.CSWStatus == USBH_MSC_FAIL)
          {
            status = USBH_MSC_FAIL;
          }
          
          else if(USBH_MSC_CSWData.field.CSWStatus == USBH_MSC_PHASE_ERROR)
          { 
            /* Refer to USB Mass-Storage Class : BOT (www.usb.org) 
            Section 6.7 
            (2) Hn < Di ( Host expects no data transfers, 
            Device intends to send data to the host)
            (3) Hn < Do ( Host expects no data transfers, 
            Device intends to receive data from the host)
            (7) Hi < Di ( Host expects to receive data from the device, 
            Device intends to send data to the host)
            (8) Hi <> Do ( Host expects to receive data from the device, 
            Device intends to receive data from the host)
            (10) Ho <> Di (Host expects to send data to the device,
            Di Device intends to send data to the host)
            (13) Ho < Do (Host expects to send data to the device, 
            Device intends to receive data from the host)
            */
            
            status = USBH_MSC_PHASE_ERROR;
          }
        } /* CSW Tag Matching is Checked  */
      } /* CSW Signature Correct Checking */
      else
      {
        /* If the CSW Signature is not valid, We sall return the Phase Error to
        Upper Layers for Reset Recovery */
        
        status = USBH_MSC_PHASE_ERROR;
      }
    } /* CSW Length Check*/
  }
  
  USBH_MSC_BOTXferParam.BOTXferStatus  = status;
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




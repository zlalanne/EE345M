// *********Can0.c ***************
// Runs on LM3S8962 <===> LM3S2110
// Low-level CAN driver
// Jonathan Valvano, derived from code written by Donald Owen
// December 9, 2011
// The system communicates 4-byte objects
// The differences between LM3S8962 LM3S2110 drivers
// 1) RCV_ID and XMT_ID are reversed
// 2) LM3S8962 runs at 50 MHz,  LM3S2110 runs at 25 MHz
// PD0/CAN0Rx
// PD1/CAN0Tx

/* This example accompanies the book
   Embedded Systems: Real-Time Operating Systems for the Arm Cortex-M3, Volume 3,  
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2012

   Program xx.x, section xx.x

 Copyright 2012 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */               

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_can.h"
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "CAN0.h"

#define CAN_BITRATE             1000000
// reverse these IDs on the other microcontroller
#define RCV_ID 4
#define XMT_ID 2

// Mailbox linkage from background to foreground
unsigned char static RCVData[4];
int static MailFlag;


//*****************************************************************************
//
// The CAN controller interrupt handler.
//
//*****************************************************************************
void CAN0_Handler(void){ unsigned char data[4];
  unsigned long ulIntStatus, ulIDStatus;
  int i;
  tCANMsgObject xTempMsgObject;
  xTempMsgObject.pucMsgData = data;
  ulIntStatus = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE); // cause?
  if(ulIntStatus & CAN_INT_INTID_STATUS){  // receive?
    ulIDStatus = CANStatusGet(CAN0_BASE, CAN_STS_NEWDAT);
    for(i = 0; i < 32; i++){    // test every bit of the mask
      if( (0x1 << i) & ulIDStatus){  // if active, get data
        CANMessageGet(CAN0_BASE, (i+1), &xTempMsgObject, TRUE);
        if(xTempMsgObject.ulMsgID == RCV_ID){
          RCVData[0] = data[0];
          RCVData[1] = data[1];
          RCVData[2] = data[2];
          RCVData[3] = data[3];
          MailFlag = TRUE;   // new mail
        }
      }
    }
  }
  CANIntClear(CAN0_BASE, ulIntStatus);  // acknowledge
}

//Set up a message object.  Can be a TX object or an RX object.
void static CAN0_Setup_Message_Object( unsigned long MessageID, \
                                unsigned long MessageFlags, \
                                unsigned long MessageLength, \
                                unsigned char * MessageData, \
                                unsigned long ObjectID, \
                                tMsgObjType eMsgType){
    tCANMsgObject xTempObject;
    xTempObject.ulMsgID = MessageID;          // 11 or 29 bit ID
    xTempObject.ulMsgLen = MessageLength;
    xTempObject.pucMsgData = MessageData;
    xTempObject.ulFlags = MessageFlags;
    CANMessageSet(CAN0_BASE, ObjectID, &xTempObject, eMsgType);
}
// Initialize CAN port
void CAN0_Open(void){
    if(REVISION_IS_A2){
      SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }
    MailFlag = FALSE;
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);              // PD0 is CAN0Rx
    GPIOPinTypeCAN(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1); // PD1 is CAN0Tx
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
    CANInit(CAN0_BASE);
    CANBitRateSet(CAN0_BASE, 8000000, CAN_BITRATE);
    CANEnable(CAN0_BASE);
    //make sure to enable STATUS interrupts
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
// Set up filter to receive these IDs
// in this case there is just one type, but you could accept multiple ID types
		
		
    CAN0_Setup_Message_Object(RCV_ID, MSG_OBJ_RX_INT_ENABLE, 4, NULL, RCV_ID, MSG_OBJ_TYPE_RX);
    IntEnable(INT_CAN0);
    return;
}

// send 4 bytes of data to other microcontroller 
void CAN0_SendData(unsigned char data[4]){
// in this case there is just one type, but you could accept multiple ID types
  CAN0_Setup_Message_Object(XMT_ID, NULL, 4, data, XMT_ID, MSG_OBJ_TYPE_TX);
}

// Returns true if receive data is available
//         false if no receive data ready
int CAN0_CheckMail(void){
  return MailFlag;
}
// if receive data is ready, gets the data and returns true
// if no receive data is ready, returns false
int CAN0_GetMailNonBlock(unsigned char data[4]){
  if(MailFlag){
    data[0] = RCVData[0];
    data[1] = RCVData[1];
    data[2] = RCVData[2];
    data[3] = RCVData[3];
    MailFlag = FALSE;
    return TRUE;
  }
  return FALSE;
}
// if receive data is ready, gets the data 
// if no receive data is ready, it waits until it is ready
void CAN0_GetMail(unsigned char data[4]){
  while(MailFlag==FALSE){};
  data[0] = RCVData[0];
  data[1] = RCVData[1];
  data[2] = RCVData[2];
  data[3] = RCVData[3];
  MailFlag = FALSE;
}






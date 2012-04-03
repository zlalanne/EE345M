// Modified By:
// Thomas Brezinski	TCB567
// Zachary Lalanne ZLL67
// Jeff Mahler
// Will Collins
// TA: Zahidul Haq
// Date of last change: 3/31/2012

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

// Mailbox linkage from background to foreground
unsigned char static RCVData[4];
unsigned char static TachData[4];

unsigned long static GenMailFlag;
unsigned long static TachMailFlag;

//******** convertCharToLong *************** 
// Converts a array of chars to a long
// Inputs: Array of chars
// Outputs: Long result
unsigned long convertCharToLong(unsigned char input[4]) {
	return ((input[3] << 24) | (input[2] << 16) | (input[1] << 8) | input[0]);
}

//******** convertLongToChar *************** 
// Converts a long to array of chars
// Inputs: A long to input, an array of chars
// Outputs: None
void convertLongToChar(unsigned long input, unsigned char output[4]) {
	output[3] = (input & 0xFF000000) >> 24;
	output[2] = (input & 0x00FF0000) >> 16;
	output[1] = (input & 0x0000FF00) >> 8;
	output[0] = (input & 0x000000FF);
}

//******** CAN0_Setup_Message_Object ************** 
// Creates a setup message object, need one for every ID 
//   you could recieve
// Inputs: None
// Outputs: None
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


//******** CAN0_Open ************** 
// Initializes the CAN0
// Note: need to set clock to correct speed before calling this
// Inputs: None
// Outputs: None
void CAN0_Open(void){

  if(REVISION_IS_A2){
    SysCtlLDOSet(SYSCTL_LDO_2_75V);
  }
  GenMailFlag = FALSE;
	TachMailFlag = FALSE;
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);              // PD0 is CAN0Rx
  GPIOPinTypeCAN(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1); // PD1 is CAN0Tx
  SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
  CANInit(CAN0_BASE);
  CANBitRateSet(CAN0_BASE, 8000000, CAN_BITRATE);
  CANEnable(CAN0_BASE);
// make sure to enable STATUS interrupts
  CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
// Set up filter to receive these IDs
// in this case there is just one type, but you could accept multiple ID types
  CAN0_Setup_Message_Object(RCV_ID, MSG_OBJ_RX_INT_ENABLE, 4, NULL, RCV_ID, MSG_OBJ_TYPE_RX);
	CAN0_Setup_Message_Object(TACH_ID, MSG_OBJ_RX_INT_ENABLE, 4, NULL, TACH_ID, MSG_OBJ_TYPE_RX);

	IntEnable(INT_CAN0);
  return;
}


//******** CAN0_SendData************** 
// Sends data over can on the XMT_ID
// Inputs: Data to send
// Outputs: None
void CAN0_SendData(unsigned long data, unsigned long msgType){
  // in this case there is just one type, but you could accept multiple ID types
  unsigned char dataMsg[4];
	convertLongToChar(data,dataMsg);	
	CAN0_Setup_Message_Object(msgType, NULL, 4, dataMsg, msgType, MSG_OBJ_TYPE_TX);
}


//******** CAN0_CheckMail************** 
// Returns if there is new data
// Inputs: None
// Outputs: TRUE if valid data, FALSE if invalid
int CAN0_CheckMail(unsigned long msgType){
  if (msgType == RCV_ID) {
		return GenMailFlag;
	} else if (msgType == TACH_ID) {
		return TachMailFlag;
	}
	return -1;
}

//******** CAN0_GetMailNonBlock *************** 
// Returns the data that was recieved
// Inputs: Pointer to variable to store data
// Outputs: TRUE if valid data, FALSE if invalid
int CAN0_GetMailNonBlock(unsigned long *data, unsigned long msgType){
	
  unsigned char tempData[4];
	
	if (msgType == RCV_ID) {
		if (GenMailFlag) {
			tempData[0] = RCVData[0];
      tempData[1] = RCVData[1];
      tempData[2] = RCVData[2];
      tempData[3] = RCVData[3];
		  (*data) = convertCharToLong(tempData);
      GenMailFlag = FALSE;
			return TRUE;
		} 
  } else if (msgType == TACH_ID) {
		if (TachMailFlag) {
			tempData[0] = TachData[0];
			tempData[1] = TachData[1];
			tempData[2] = TachData[2];
			tempData[3] = TachData[3];
			(*data) = convertCharToLong(tempData);
			TachMailFlag = FALSE;
			return TRUE;
		}
	}
  return FALSE;
}

//******** CAN0_GetMail *************** 
// Returns the data that was recieved, blocks
// Inputs: Pointer to variable to store data
// Outputs: None
void CAN0_GetMail(unsigned long *data, unsigned long MsgType){
  
	unsigned char tempData[4];
	
	if (MsgType == RCV_ID) {
		while(GenMailFlag==FALSE){};
    tempData[0] = RCVData[0];
    tempData[1] = RCVData[1];
    tempData[2] = RCVData[2];
    tempData[3] = RCVData[3];
	  (*data) = convertCharToLong(tempData);
		GenMailFlag = FALSE;
	} else if (MsgType == TACH_ID) {
		while(TachMailFlag==FALSE){};
		tempData[0] = TachData[0];
		tempData[1] = TachData[1];
		tempData[2] = TachData[2];
		tempData[3] = TachData[3];
		(*data) = convertCharToLong(tempData);
		TachMailFlag = FALSE;
	}
}

//******** CAN0_Handler *************** 
// Handler for CAN0 Input, stores data in RCVData, disregards all messages
// that are on wrong ID
// Inputs: None
// Outputs: None
void CAN0_Handler(void){ 
	unsigned char data[4];
  unsigned long ulIntStatus, ulIDStatus;
  int i;
  tCANMsgObject xTempMsgObject;
  xTempMsgObject.pucMsgData = data;
  ulIntStatus = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE); // cause?
  if(ulIntStatus & CAN_INT_INTID_STATUS){  // receive?
    ulIDStatus = CANStatusGet(CAN0_BASE, CAN_STS_NEWDAT);
    for(i = 0; i < 32; i++){    //test every bit of the mask
      if( (0x1 << i) & ulIDStatus){  // if active, get data
        CANMessageGet(CAN0_BASE, (i+1), &xTempMsgObject, TRUE);
        if(xTempMsgObject.ulMsgID == RCV_ID){
          RCVData[0] = data[0];
          RCVData[1] = data[1];
          RCVData[2] = data[2];
          RCVData[3] = data[3];
          GenMailFlag = TRUE;   // new mail
        } else if(xTempMsgObject.ulMsgID == TACH_ID){
					TachData[0] = data[0];
					TachData[1] = data[1];
					TachData[2] = data[2];
					TachData[3] = data[3];
					TachMailFlag = TRUE;
				}
      }
    }
  }
  CANIntClear(CAN0_BASE, ulIntStatus);  // acknowledge
}


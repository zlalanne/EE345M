// Modified By:
// Thomas Brezinski	TCB567
// Zachary Lalanne ZLL67
// Jeff Mahler
// Will Collins
// TA: Zahidul Haq
// Date of last change: 3/31/2012

// *********Can0.h ***************
// Runs on LM3S8962	<===> LM3S2110
// Low-level CAN driver
// Jonathan Valvano, derived from code written by Donald Owen
// December 9, 2011
// The system communicates 4-byte objects
// The differences between LM3S8962	LM3S2110 drivers
// 1) RCV_ID and XMT_ID are reversed
// 2) LM3S8962 runs at 50 MHz,	LM3S2110 runs at 25 MHz
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

#ifndef __CAN0_H__
#define __CAN0_H__

#define TRUE 1
#define FALSE 0
#define NULL 0

//******** CAN0_CheckMail************** 
// Returns if there is new data
// Inputs: None
// Outputs: TRUE if valid data, FALSE if invalid
int CAN0_CheckMail(unsigned long msgType);

//******** CAN0_GetMailNonBlock *************** 
// Returns the data that was recieved
// Inputs: Pointer to variable to store data
// Outputs: TRUE if valid data, FALSE if invalid
int CAN0_GetMailNonBlock(unsigned long * data, unsigned long msgType);

//******** CAN0_GetMail *************** 
// Returns the data that was recieved, blocks
// Inputs: Pointer to variable to store data
// Outputs: None
void CAN0_GetMail(unsigned long *data, unsigned long msgType);

//******** CAN0_Open ************** 
// Initializes the CAN0
// Inputs: None
// Outputs: None
void CAN0_Open(void);

//******** CAN0_SendData************** 
// Sends data over can on the XMT_ID
// Inputs: Data to send
// Outputs: None
void CAN0_SendData(unsigned long data, unsigned long msgType);

#endif //  __CAN0_H__


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

// Returns true if receive data is available
//         false if no receive data ready
int CAN0_CheckMail(void);

// if receive data is ready, gets the data and returns true
// if no receive data is ready, returns false
int CAN0_GetMailNonBlock(unsigned char data[4]);

// if receive data is ready, gets the data 
// if no receive data is ready, it waits until it is ready
void CAN0_GetMail(unsigned char data[4]);

// Initialize CAN port
void CAN0_Open(void);

// send 4 bytes of data to other microcontroller 
void CAN0_SendData(unsigned char data[4]);

#endif //  __CAN0_H__


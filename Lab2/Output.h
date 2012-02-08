// Modified By:
// Thomas Brezinski
// Zachary Lalanne ZLL67
// TA:
// Date of last change: 1/24/2012

// Modified By:
// Megan Ruthven MAR3939
// Zachary Lalanne ZLL67
// TA: NACHI
// Date of last change: 9/19/2011

// Output.h
// Runs on LM3S811, LM3S1968, LM3S8962
// Implement the fputc() function in stdio.h to enable useful
// functions such as printf() to write text to the onboard
// organic LED display.  Remember that the OLED is vulnerable
// to screen image "burn-in" like old CRT monitors, so be
// sure to turn the OLED off when it is not in use.
// Daniel Valvano
// July 28, 2011

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to the Arm Cortex M3",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2011
   Section 3.4.5

 Copyright 2011 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

#ifndef ASCII
  #define ASCII
  #define CR   0x0D
  #define LF   0x0A
  #define TAB  0x09
  #define BS   0x08
  #define BACKSPACE 0x08
  #define ESC  0x1B
  #define SP   0x20
  #define DEL  0x7F
  #define HOME 0x0A
  #define NEWLINE 0x0D
  #define RETURN 0x0D
#endif

//------------Output_Init------------
// Initializes the OLED interface.
// Input: none
// Output: none
void Output_Init(void);

//------------Output_Clear------------
// Clears the OLED display.
// Input: none
// Output: none
void Output_Clear(void);

//------------Output_Off------------
// Turns off the OLED display
// Input: none
// Output: none
void Output_Off(void);

//------------Output_On------------
// Turns on the OLED display
//  called after Output_Off to turn it back on
// Input: none
// Output: none
void Output_On(void);

//------------Output_Color------------
// Set the color of future characters.
// Input: 0 is off, non-zero is on
// Output: none
void Output_Color(unsigned char newColor);

extern void oLED_Message(int device, int line, char *string, long value);

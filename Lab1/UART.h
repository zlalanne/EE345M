// UART.h
// Implements an interpreter on UART0, to add new commands
// for the interrupter modify CMD_Run()

// Modified By:
// Thomas Brezinski
// Zachary Lalanne ZLL67
// TA:
// Date of last change: 2/1/2012

// Written By:
// Megan Ruthven MAR3939
// Zachary Lalanne ZLL67
// TA: NACHI
// Date of last change: 10/17/2011

// Standard ASCII symbols
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

#ifndef boolean
  #define boolean
  #define TRUE 1
  #define FALSE 0
  #define SUCCESS 1
  #define FAILURE 0
#endif


// UART Parameters
#define BAUD 9600

//------------UART0_Init------------
// Initilizes UART0 as interpreturer
// Input: none
// Output: none
void UART0_Init(void);

//--------UART0_SendString---------
// Outputs a string to UART0
// Input: Null terminated string
// Output: none
void UART0_SendString(char *stringBuffer);

//--------UART0_OutChar------------
// Outputs a character to UART0, spin
//   if TxFifo is full
// Input: Single character to print
// Output: none
void UART0_OutChar(char data);

//------------CMD_Run--------------
// Runs the latest command entered 
//   if no new command simply returns
// Input: none
// Output: none
void CMD_Run(void);



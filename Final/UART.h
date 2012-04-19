// UART.h
// Implements an interpreter on UART0, to add new commands
// for the interrupter modify CMD_Run()

// Modified By:
// Thomas Brezinski	TCB567
// Zachary Lalanne ZLL67
// TA: Zahidul Haq
// Date of last change: 2/24/2012

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

// Boolean phrases
#ifndef boolean
  #define boolean
  #define SUCCESS 1
  #define FAILURE 0
  #define VALID 1
  #define INVALID 0
#endif


// UART Parameters
#define BAUD 115200


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

//--------UART0_SendStringLength---------
// Outputs a string to UART0
// Input: String, Length of string
// Output: Number of chracters written
int UART0_SendStringLength(const char *stringBuffer, unsigned long length);

//--------UART0_OutChar------------
// Outputs a character to UART0, spin
//   if TxFifo is full
// Input: Single character to print
// Output: none
void UART0_OutChar(unsigned char data);

//------------Interpreter--------------
// Continuosly runs commands
// Input: none
// Output: none
void Interpreter(void);

//------------CMD_Run--------------
// Runs the latest command entered 
//   if no new command simply returns
// Input: none
// Output: none
void CMD_Run(void);

//------------UARTprintf--------------
// Implements printf for UART
// Input: String in printf format
// Output: none
void UARTprintf(const char *pcString, ...);

// External variables from Lab2.c
extern unsigned long NumCreated;   // number of foreground threads created
extern unsigned long PIDWork;      // current number of PID calculations finished
extern unsigned long FilterWork;   // number of digital filter calculations finished
extern unsigned long NumSamples;   // incremented every sample
extern unsigned long DataLost;     // data sent by Producer, but not received by Consumer
extern long MaxJitter1;             // largest time jitter between interrupts in usec
extern long MinJitter1;             // smallest time jitter between interrupts in usec
//extern char DigFiltEn;				// Enable/disable digital filter

// UART.h
// Implements an interpreter on UART0.
// To add new commands for the interrupter modify CMD_Run()

// Modified By:
// Thomas Brezinski TCB567
// Zachary Lalanne ZLL67
// TA: Zahidul Haq
// Date of last change: 3/21/2012

// Written By:
// Megan Ruthven MAR3939
// Zachary Lalanne ZLL67
// TA: NACHI
// Date of last change: 10/17/2011

#include <string.h>
#include <stdio.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

#include "Fifo.h"
#include "UART.h"
#include "ADC.h"
#include "Output.h"
#include "OS.h"
#include "efile.h"

#define STARTSTRING "\n\rUART Initilization Complete\n\r"
#define CMDPROMPT "$ "

#define MAXCMDSIZE 30       // Max size of a command entered
#define BUFFERSIZE 20       // Max size of snprintf buffer
#define MAXARGS 7
#define MAXARGLENGTH 10

#define FIFOSIZE   128      // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS 1       // return value on success
#define FIFOFAIL    0       // return value on failure
char CMDCursor = 0;
char CurCMD[MAXCMDSIZE] = "";

AddIndexFifo(Rx, FIFOSIZE, char, FIFOSUCCESS, FIFOFAIL)
AddIndexFifo(Tx, FIFOSIZE, char, FIFOSUCCESS, FIFOFAIL)

//------------UART0_Init------------
// Initilizes UART0 as interpreturer
// Input: none
// Output: none
void UART0_Init(void) {

  // Enabling the peripherals
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

  // Initilizaing FIFOs
  RxFifo_Init();
  TxFifo_Init();

  // Disable UART during initlization
  UARTDisable(UART0_BASE);

  // Configuring the pins needed for UART
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

  // Configure clock
  UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), BAUD,
      (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
          UART_CONFIG_PAR_NONE));

  // Configuring UART0 Hardware FIFOs
  UARTFIFOEnable(UART0_BASE);
  UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);

  // Enable UART0 and configure interupts
  UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT | UART_INT_TX);
  UARTEnable(UART0_BASE);
  IntEnable(INT_UART0);

  // Send string to show that UART is initialized
  UART0_SendString(STARTSTRING);

  // Print command prompt
  eFile_PrintWorkingDirectory();
  UART0_SendString(CMDPROMPT);
}

//------------Interpreter--------------
// Continuosly runs commands
// Input: none
// Output: none
void Interpreter(void) {
  while(1) {
    CMD_Run();
  }
}

//------------CMD_Run--------------
// Runs the latest command entered
//   if no new command simply returns
// Input: none
// Output: none
void CMD_Run(void) {

  unsigned long measurement;
  char buffer[BUFFERSIZE]; // Used for snprintf

  char arg[MAXARGS][MAXARGLENGTH] = { NULL, NULL };
  char letter;
  char *tokenPtr;
  char newCMD = FAILURE;
  int i = 0;

  // If no new characters then exit
  if(RxFifo_Get(&letter) == FIFOFAIL) {
    return;
  }

  // Decoding character pressed
  switch(letter) {
    case '\n':
      break;
    case '\r':
      // Print new line if user presses ENTER
      UART0_OutChar('\n'); // Echo to screen
      UART0_OutChar('\r');
      CurCMD[CMDCursor] = '\0'; // Terminate string
      CMDCursor = 0;
      newCMD = SUCCESS;
      break;
    case 0x7F:
      // User pressed backspace
      if(CMDCursor > 0) {
        UART0_OutChar(letter);
        CMDCursor--;
        CurCMD[CMDCursor] = '\0';
      }
      break;
    default:
      // Save char typed if user press key
      UART0_OutChar(letter);
      CurCMD[CMDCursor] = letter;
      CMDCursor = (CMDCursor + 1) % MAXCMDSIZE;
      break;
  }

  // Leave function if user has not pressed enter yet
  if(newCMD == FAILURE) {
    return;
  }

  // Seperating spaces into differnt arguments, the cmd is in arg[0]
  tokenPtr = strtok(CurCMD, " ");
  i = 0;
  while(tokenPtr != NULL) {
    strncpy(arg[i], tokenPtr, MAXARGLENGTH);
    tokenPtr = strtok(NULL, " ");
    i++;
  }

  // Decode command
  // Note: no commands check their arguments, make sure to use correctly
  switch(arg[0][0]) {
    case 'a':
      // ADC Measurement, arg[1] is channel number
      measurement = ADC_In(arg[1][0] - 0x30);
      snprintf(buffer, BUFFERSIZE, "ADC%c: %d\n\r", arg[1][0], measurement);
      UART0_SendString(buffer);
      break;
    case 'c':
      switch(arg[0][1]) {
        case 'l':
          // Close the current file
          eFile_WClose();
          break;
        case 'd':
          // Change directory
          eFile_ChangeDirectory(arg[1]);
          break;
        case 'a':
          // Read entire file
          eFile_ReadEntireFile(arg[1]);
          UART0_SendString("\n\r");
          break;
      }
      break;
    case 'o':
      switch(arg[0][1]) {
        case 'l':
          // Turn on oLED screen
          Output_On();
          UART0_SendString("oLED On\n\r");
          break;
        case 'p':
          // Open a file for writing
          if(eFile_WOpen(arg[1]) == 0) {
            UART0_SendString("Opened Successfully\n\r");
          } else {
            UART0_SendString("Error Opening File\n\r");
          }
          break;
      }
      break;
    case 'p':
      switch(arg[0][1]) {
        case 'r':
          // Print string to oLED, must include device/line
          // arg[1] is device number, arg[2] is line number
          // arg[3] - arg[5] are strings to print
          snprintf(buffer, BUFFERSIZE, "%s %s %s", arg[3], arg[4], arg[5]);
          oLED_Message(arg[1][0] - 0x30, arg[2][0] - 0x30, buffer, 0);
          UART0_SendString("Message Printed\n\r");
          break;
        case 'w':
          // Pring working directory
          eFile_PrintWorkingDirectory();
          UART0_SendString("\n\r");
          break;
      }
      break;
    case 't':
      // Get Timer2 interrupt counter
      measurement = OS_MsTime(1);
      snprintf(buffer, BUFFERSIZE, "Timer2 Counter: %d\n\r", measurement);
      UART0_SendString(buffer);
      break;
    case 'r':
      switch(arg[0][1]) {
        case 'e':
          // Reset Timer2 interurpt counter
          OS_ClearMsTime(1);
          UART0_SendString("Timer2 Counter Cleared\n\r");
          break;
        case 'm':
          // Delete file
          eFile_Delete(arg[1]);
          break;
      }
      break;
    case 'l':
      // List directory
      eFile_Directory(&UART0_OutChar);
      break;
    case 'f':
      // Format SD Card
      eFile_Format();
      break;
    case 'n':
      // Create new file
      eFile_Create(arg[1]);
      break;
    case 'm':
      // Create new directory
      eFile_CreateDirectory(arg[1]);
      break;
    case 'h':
      // Help listing
      // TODO: update this to be relevant
      UART0_SendString("Available commands: adc, on, clear, print\n\r");
      break;
    case 'w':
      // Writes to currently open file
      eFile_WriteString(arg[1]);
      break;
    default:
      UART0_SendString("Command not recgonized\n\r");
      break;
  }

  // Print command prompt
  eFile_PrintWorkingDirectory();
  UART0_SendString(CMDPROMPT);

  return;
}

// Copy from hardware RX FIFO to software RX FIFO
// Stop when hardware RX FIFO is empty or software RX FIFO is full
void copyHardwareToSoftware(void) {
  char letter;
  while((UARTCharsAvail(UART0_BASE) != false) && (RxFifo_Size() < (FIFOSIZE - 1))) {
    letter = (char) UARTCharGetNonBlocking(UART0_BASE);
    RxFifo_Put(letter);
  }
}

// Copy from software TX FIFO to hardware TX FIFO
// Stop when software TX FIFO is empty or hardware TX FIFO is full
void copySoftwareToHardware(void) {
  char letter;

  while((UARTSpaceAvail(UART0_BASE) != false) && (TxFifo_Size() > 0)) {
    TxFifo_Get(&letter);
    UARTCharPutNonBlocking(UART0_BASE, letter);
  }
}

//--------UART0_OutChar------------
// Outputs a character to UART0, spin
//   if TxFifo is full
// Input: Single character to print
// Output: none
void UART0_OutChar(unsigned char data) {
  while(TxFifo_Put(data) == FIFOFAIL) {};
  
  UARTIntDisable(UART0_BASE, UART_INT_TX);
  copySoftwareToHardware();
  UARTIntEnable(UART0_BASE, UART_INT_TX);
}

//--------UART0_SendString---------
// Outputs a string to UART0
// Input: Null terminated string
// Output: none
void UART0_SendString(char *stringBuffer) {
  // Loop while there are more characters to send.
  while(*stringBuffer) {
    UART0_OutChar(*stringBuffer);
    stringBuffer++;
  }
}

//--------UART0_SendStringLength---------
// Outputs a string to UART0
// Input: String, Length of string
// Output: Number of chracters written
int UART0_SendStringLength(const char *stringBuffer, unsigned long length) {
  // Loop while there are more characters to send.
  int i;

  for(i = 0; i < length; i++) {
    UART0_OutChar(stringBuffer[i]);
  }

  return i + 1;
}

// Interrupt on recieve or transmit FIFO getting too full
void UART0_Handler(void) {

  unsigned long status;
  status = UARTIntStatus(UART0_BASE, true); // Finding what casued interrupt (TXFIFO or RXFIFO)

  if(status == UART_INT_TX) {
    UARTIntClear(UART0_BASE, UART_INT_TX); // Clearing interrupt flag
    copySoftwareToHardware();

    if(TxFifo_Size() == 0) {
      UARTIntDisable(UART0_BASE, UART_INT_TX); // Going too fast, need to disable TX FIFO interrupt
    }
  }

  if(status == UART_INT_RX) {
    UARTIntClear(UART0_BASE, status); // Clearing interrupt flag
    copyHardwareToSoftware();
  }

  if(status == UART_INT_RT) {
    UARTIntClear(UART0_BASE, status);
    copyHardwareToSoftware();
  }
}

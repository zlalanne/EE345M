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
#include <stdarg.h>
#include <stdlib.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/debug.h"

#include "Fifo.h"
#include "UART.h"
#include "ADC.h"
#include "Output.h"
#include "OS.h"
#include "Servo.h"

#define STARTSTRING "\n\rUART Initilization Complete\n\r"
#define CMDPROMPT ">> "

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
    case 'o':
      switch(arg[0][1]) {
        case 'l':
          // Turn on oLED screen
          Output_On();
          UART0_SendString("oLED On\n\r");
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
      }
      break;
    case 'h':
      // Help listing
      // TODO: update this to be relevant
      UART0_SendString("Available commands: adc, on, clear, print\n\r");
      break;
	case 's':
	  // servo position
	  Servo_Set_Position((unsigned long)atoi((const char *)arg[1]));
	  break;

    default:
      UART0_SendString("Command not recgonized\n\r");
      break;
  }

  // Print command prompt
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


// The code below is taking from the StellarisWare UARTStdio.c file. It contains
// a useful implementation of the printf routine for UART

//------------UARTprintf--------------
// Implements printf for UART
// Input: String in printf format
// Output: none

//*****************************************************************************
//
// A mapping from an integer between 0 and 15 to its ASCII character
// equivalent.
//
//*****************************************************************************
static const char * const g_pcHex = "0123456789abcdef";

//*****************************************************************************
//
//! A simple UART based printf function supporting \%c, \%d, \%p, \%s, \%u,
//! \%x, and \%X.
//!
//! \param pcString is the format string.
//! \param ... are the optional arguments, which depend on the contents of the
//! format string.
//!
//! This function is very similar to the C library <tt>fprintf()</tt> function.
//! All of its output will be sent to the UART.  Only the following formatting
//! characters are supported:
//!
//! - \%c to print a character
//! - \%d to print a decimal value
//! - \%s to print a string
//! - \%u to print an unsigned decimal value
//! - \%x to print a hexadecimal value using lower case letters
//! - \%X to print a hexadecimal value using lower case letters (not upper case
//! letters as would typically be used)
//! - \%p to print a pointer as a hexadecimal value
//! - \%\% to print out a \% character
//!
//! For \%s, \%d, \%u, \%p, \%x, and \%X, an optional number may reside
//! between the \% and the format character, which specifies the minimum number
//! of characters to use for that value; if preceded by a 0 then the extra
//! characters will be filled with zeros instead of spaces.  For example,
//! ``\%8d'' will use eight characters to print the decimal value with spaces
//! added to reach eight; ``\%08d'' will use eight characters as well but will
//! add zeroes instead of spaces.
//!
//! The type of the arguments after \e pcString must match the requirements of
//! the format string.  For example, if an integer was passed where a string
//! was expected, an error of some kind will most likely occur.
//!
//! \return None.
//
//*****************************************************************************
void
UARTprintf(const char *pcString, ...)
{
    unsigned long ulIdx, ulValue, ulPos, ulCount, ulBase, ulNeg;
    char *pcStr, pcBuf[16], cFill;
    va_list vaArgP;

    //
    // Check the arguments.
    //
    ASSERT(pcString != 0);

    //
    // Start the varargs processing.
    //
    va_start(vaArgP, pcString);

    //
    // Loop while there are more characters in the string.
    //
    while(*pcString)
    {
        //
        // Find the first non-% character, or the end of the string.
        //
        for(ulIdx = 0; (pcString[ulIdx] != '%') && (pcString[ulIdx] != '\0');
            ulIdx++)
        {
        }

        //
        // Write this portion of the string.
        //
        UART0_SendStringLength(pcString, ulIdx);

        //
        // Skip the portion of the string that was written.
        //
        pcString += ulIdx;

        //
        // See if the next character is a %.
        //
        if(*pcString == '%')
        {
            //
            // Skip the %.
            //
            pcString++;

            //
            // Set the digit count to zero, and the fill character to space
            // (i.e. to the defaults).
            //
            ulCount = 0;
            cFill = ' ';

            //
            // It may be necessary to get back here to process more characters.
            // Goto's aren't pretty, but effective.  I feel extremely dirty for
            // using not one but two of the beasts.
            //
again:

            //
            // Determine how to handle the next character.
            //
            switch(*pcString++)
            {
                //
                // Handle the digit characters.
                //
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                {
                    //
                    // If this is a zero, and it is the first digit, then the
                    // fill character is a zero instead of a space.
                    //
                    if((pcString[-1] == '0') && (ulCount == 0))
                    {
                        cFill = '0';
                    }

                    //
                    // Update the digit count.
                    //
                    ulCount *= 10;
                    ulCount += pcString[-1] - '0';

                    //
                    // Get the next character.
                    //
                    goto again;
                }

                //
                // Handle the %c command.
                //
                case 'c':
                {
                    //
                    // Get the value from the varargs.
                    //
                    ulValue = va_arg(vaArgP, unsigned long);

                    //
                    // Print out the character.
                    //
                    UART0_SendStringLength((char *)&ulValue, 1);

                    //
                    // This command has been handled.
                    //
                    break;
                }

                //
                // Handle the %d command.
                //
                case 'd':
                {
                    //
                    // Get the value from the varargs.
                    //
                    ulValue = va_arg(vaArgP, unsigned long);

                    //
                    // Reset the buffer position.
                    //
                    ulPos = 0;

                    //
                    // If the value is negative, make it positive and indicate
                    // that a minus sign is needed.
                    //
                    if((long)ulValue < 0)
                    {
                        //
                        // Make the value positive.
                        //
                        ulValue = -(long)ulValue;

                        //
                        // Indicate that the value is negative.
                        //
                        ulNeg = 1;
                    }
                    else
                    {
                        //
                        // Indicate that the value is positive so that a minus
                        // sign isn't inserted.
                        //
                        ulNeg = 0;
                    }

                    //
                    // Set the base to 10.
                    //
                    ulBase = 10;

                    //
                    // Convert the value to ASCII.
                    //
                    goto convert;
                }

                //
                // Handle the %s command.
                //
                case 's':
                {
                    //
                    // Get the string pointer from the varargs.
                    //
                    pcStr = va_arg(vaArgP, char *);

                    //
                    // Determine the length of the string.
                    //
                    for(ulIdx = 0; pcStr[ulIdx] != '\0'; ulIdx++)
                    {
                    }

                    //
                    // Write the string.
                    //
                    UART0_SendStringLength(pcStr, ulIdx);

                    //
                    // Write any required padding spaces
                    //
                    if(ulCount > ulIdx)
                    {
                        ulCount -= ulIdx;
                        while(ulCount--)
                        {
                            UART0_SendStringLength(" ", 1);
                        }
                    }
                    //
                    // This command has been handled.
                    //
                    break;
                }

                //
                // Handle the %u command.
                //
                case 'u':
                {
                    //
                    // Get the value from the varargs.
                    //
                    ulValue = va_arg(vaArgP, unsigned long);

                    //
                    // Reset the buffer position.
                    //
                    ulPos = 0;

                    //
                    // Set the base to 10.
                    //
                    ulBase = 10;

                    //
                    // Indicate that the value is positive so that a minus sign
                    // isn't inserted.
                    //
                    ulNeg = 0;

                    //
                    // Convert the value to ASCII.
                    //
                    goto convert;
                }

                //
                // Handle the %x and %X commands.  Note that they are treated
                // identically; i.e. %X will use lower case letters for a-f
                // instead of the upper case letters is should use.  We also
                // alias %p to %x.
                //
                case 'x':
                case 'X':
                case 'p':
                {
                    //
                    // Get the value from the varargs.
                    //
                    ulValue = va_arg(vaArgP, unsigned long);

                    //
                    // Reset the buffer position.
                    //
                    ulPos = 0;

                    //
                    // Set the base to 16.
                    //
                    ulBase = 16;

                    //
                    // Indicate that the value is positive so that a minus sign
                    // isn't inserted.
                    //
                    ulNeg = 0;

                    //
                    // Determine the number of digits in the string version of
                    // the value.
                    //
convert:
                    for(ulIdx = 1;
                        (((ulIdx * ulBase) <= ulValue) &&
                         (((ulIdx * ulBase) / ulBase) == ulIdx));
                        ulIdx *= ulBase, ulCount--)
                    {
                    }

                    //
                    // If the value is negative, reduce the count of padding
                    // characters needed.
                    //
                    if(ulNeg)
                    {
                        ulCount--;
                    }

                    //
                    // If the value is negative and the value is padded with
                    // zeros, then place the minus sign before the padding.
                    //
                    if(ulNeg && (cFill == '0'))
                    {
                        //
                        // Place the minus sign in the output buffer.
                        //
                        pcBuf[ulPos++] = '-';

                        //
                        // The minus sign has been placed, so turn off the
                        // negative flag.
                        //
                        ulNeg = 0;
                    }

                    //
                    // Provide additional padding at the beginning of the
                    // string conversion if needed.
                    //
                    if((ulCount > 1) && (ulCount < 16))
                    {
                        for(ulCount--; ulCount; ulCount--)
                        {
                            pcBuf[ulPos++] = cFill;
                        }
                    }

                    //
                    // If the value is negative, then place the minus sign
                    // before the number.
                    //
                    if(ulNeg)
                    {
                        //
                        // Place the minus sign in the output buffer.
                        //
                        pcBuf[ulPos++] = '-';
                    }

                    //
                    // Convert the value into a string.
                    //
                    for(; ulIdx; ulIdx /= ulBase)
                    {
                        pcBuf[ulPos++] = g_pcHex[(ulValue / ulIdx) % ulBase];
                    }

                    //
                    // Write the string.
                    //
                    UART0_SendStringLength(pcBuf, ulPos);

                    //
                    // This command has been handled.
                    //
                    break;
                }

                //
                // Handle the %% command.
                //
                case '%':
                {
                    //
                    // Simply write a single %.
                    //
                    UART0_SendStringLength(pcString - 1, 1);

                    //
                    // This command has been handled.
                    //
                    break;
                }

                //
                // Handle all other commands.
                //
                default:
                {
                    //
                    // Indicate an error.
                    //
                    UART0_SendStringLength("ERROR", 5);

                    //
                    // This command has been handled.
                    //
                    break;
                }
            }
        }
    }

    //
    // End the varargs processing.
    //
    va_end(vaArgP);
}

// UART.h
// Implements an interpreter on UART0.
// To add new commands for the interrupter modify CMD_Run()

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

#define STARTSTRING "\n\rUART0 Initilization Done!\n\r"
#define CMDPROMPT ">> "

<<<<<<< HEAD
#define MAXCMDSIZE 60       // Max size of a command entered
#define BUFFERSIZE 40       // Max size of snprintf buffer
=======
#define MAXCMDSIZE 30       // Max size of a command entered
#define BUFFERSIZE 20       // Max size of snprintf buffer
>>>>>>> 4808a194f01c59feb1989d96b198028d287377f6
#define MAXARGS 7
#define MAXARGLENGTH 10

#define FIFOSIZE   256      // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS 1       // return value on success
#define FIFOFAIL    0       // return value on failure

char CMDCursor = 0;
char CurCMD[MAXCMDSIZE] = "";

AddIndexFifo(Rx, FIFOSIZE, char, FIFOSUCCESS, FIFOFAIL)
AddIndexFifo(Tx, FIFOSIZE, char, FIFOSUCCESS, FIFOFAIL)

// Function Protoypes
void UART0_Handler(void);

//------------UART0_Init------------
// Initilizes UART0 as interpreturer
// Input: none
// Output: none
void UART0_Init(void){
  
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
<<<<<<< HEAD
  UARTIntEnable(UART0_BASE, UART_INT_TX | UART_INT_RX | UART_INT_RT);
  UARTEnable(UART0_BASE);
  IntEnable(INT_UART0);
=======
  UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT | UART_INT_TX);
  UARTEnable(UART0_BASE);
  IntEnable(INT_UART0); 
>>>>>>> 4808a194f01c59feb1989d96b198028d287377f6

  // Send string to show that UART is initialized
  UART0_SendString(STARTSTRING);
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

char buffer[BUFFERSIZE]; // Used for snprintf
//------------CMD_Run--------------
// Runs the latest command entered 
//   if no new command simply returns
// Input: none
// Output: none
void CMD_Run(void) {
  
  unsigned long measurement;
<<<<<<< HEAD
  
=======
  char buffer[BUFFERSIZE]; // Used for snprintf
  char buffer1[BUFFERSIZE]; // Used for snprintf
  char buffer2[BUFFERSIZE]; // Used for snprintf
  char buffer3[BUFFERSIZE]; // Used for snprintf
  char buffer4[BUFFERSIZE]; // Used for snprintf
>>>>>>> 4808a194f01c59feb1989d96b198028d287377f6

  char arg[MAXARGS][MAXARGLENGTH] = {NULL, NULL};
  char letter;
  char *tokenPtr;
  char newCMD = FALSE;
  int i = 0;

  // If no new characters then exit
  if(RxFifo_Get(&letter) == FIFOFAIL){
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
		newCMD = TRUE;
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
  if(newCMD == FALSE){
    return;
  }

  // Seperating spaces into differnt arguments, the cmd is in arg[0]
  tokenPtr = strtok(CurCMD," ");
  i = 0;
  while (tokenPtr != NULL) {
	strncpy(arg[i], tokenPtr, MAXARGLENGTH); 	  
    tokenPtr = strtok(NULL," ");
	i++;
  }

  // Decode command
  // Note: no commands check their arguments, make sure to use correctly
  switch(arg[0][0]){
    case 'a':
	  // ADC Measurement, arg[1] is channel number
	  measurement = ADC_In(arg[1][0] - 0x30);
	  snprintf(buffer1, BUFFERSIZE, "ADC%c: %d\n\r", arg[1][0], measurement);
	  UART0_SendString(buffer);
	  break;
	case 'c':
	  // Clear oLED screen
	  Output_Clear();
	  UART0_SendString("oLED Cleared\n\r");
	  break;
	case 'o':
	  // Turn on oLED screen
	  Output_On();
	  UART0_SendString("oLED On\n\r");
	  break;
	case 'p':
<<<<<<< HEAD
	  switch(arg[0][1]){
=======
	  switch(arg[0][1]) {
>>>>>>> 4808a194f01c59feb1989d96b198028d287377f6
	    case 'r':
	      // Print string to oLED, must include device/line
	      // arg[1] is device number, arg[2] is line number
	      // arg[3] - arg[5] are strings to print
<<<<<<< HEAD
	      snprintf(buffer, BUFFERSIZE, "%s %s %s", arg[3], arg[4], arg[5]);
	      oLED_Message(arg[1][0] - 0x30, arg[2][0] - 0x30, buffer, 0);
          UART0_SendString("Message Printed\n\r");
	      break;
		case 'e':
		  // Print performance measurements to UART
		  UART0_SendString("Performance Measurements:\n\r");
		  snprintf(buffer, BUFFERSIZE, "NumCreated = %d\n\r", NumCreated);
		  UART0_SendString(buffer);
		  snprintf(buffer, BUFFERSIZE, "PIDWork = %d\n\r", PIDWork);
		  UART0_SendString(buffer);
		  snprintf(buffer, BUFFERSIZE, "NumSamples = %d\n\r", NumSamples);
		  UART0_SendString(buffer);
		  snprintf(buffer, BUFFERSIZE, "DataLost = %d\n\r", DataLost);
		  UART0_SendString(buffer);
		  snprintf(buffer, BUFFERSIZE, "Jitter = %d\n\r", MaxJitter-MinJitter);
		  UART0_SendString(buffer);
=======
	      snprintf(buffer1, BUFFERSIZE, "%s %s %s", arg[3], arg[4], arg[5]);
	      oLED_Message(arg[1][0] - 0x30, arg[2][0] - 0x30, buffer, 0);
          UART0_SendString("Message Printed\n\r");
	      break;
	    case 'e':
		  // Print performance measurements to UART
		  UART0_SendString("Performance Measurements:\n\r");
		  snprintf(buffer1, BUFFERSIZE, "NumCreated: %d\r\n", NumCreated);
		  UART0_SendString(buffer1);
		  snprintf(buffer2, BUFFERSIZE, "PIDWork: %d\r\n", PIDWork);
		  UART0_SendString(buffer2);
		  snprintf(buffer3, BUFFERSIZE, "DataLost: %d\r\n", DataLost);
		  UART0_SendString(buffer3);
		  snprintf(buffer4, BUFFERSIZE, "Jitter: %d\r\n", (MaxJitter-MinJitter));
		  UART0_SendString(buffer4);
>>>>>>> 4808a194f01c59feb1989d96b198028d287377f6
		  break;
	  }
	  break;
	case 't':
	  // Get Timer2 interrupt counter
	  measurement = OS_MsTime();
<<<<<<< HEAD
	  sprintf(buffer, "Timer2 Counter: %d\n\r", measurement);
=======
	  snprintf(buffer1, BUFFERSIZE, "Timer2 Counter: %d\n\r", measurement);
>>>>>>> 4808a194f01c59feb1989d96b198028d287377f6
	  UART0_SendString(buffer);
	  break;
	case 'r':
	  // Reset Timer2 interurpt counter
	  OS_ClearMsTime();
	  UART0_SendString("Timer2 Counter Cleared\n\r");
	  break;
	case 'h':
	  UART0_SendString("Available commands: adc, on, clear, print\n\r");
	  break;
	case 'n':
	  UART0_SendString("Test\n\r");
	  break;
	default:
	  UART0_SendString("Command not recgonized\n\r");
	  break;
  }

  // Store command executed as last command
  UART0_SendString(CMDPROMPT);

  return;
}

// Copy from hardware RX FIFO to software RX FIFO
// Stop when hardware RX FIFO is empty or software RX FIFO is full
void copyHardwareToSoftware(void){
  char letter;
  while((UARTCharsAvail(UART0_BASE) != false) && (RxFifo_Size() < (FIFOSIZE - 1))){
    letter = (char) UARTCharGetNonBlocking(UART0_BASE);
    RxFifo_Put(letter);
  }
}

// Copy from software TX FIFO to hardware TX FIFO
// Stop when software TX FIFO is empty or hardware TX FIFO is full
void copySoftwareToHardware(void){
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
void UART0_OutChar(char data){
  while(TxFifo_Put(data) == FIFOFAIL){};
  UARTIntDisable(UART0_BASE, UART_INT_TX);
  copySoftwareToHardware();
  UARTIntEnable(UART0_BASE, UART_INT_TX);
}

//--------UART0_SendString---------
// Outputs a string to UART0
// Input: Null terminated string
// Output: none
void UART0_SendString(char *stringBuffer){
  // Loop while there are more characters to send.
  while(*stringBuffer) {
    UART0_OutChar(*stringBuffer);
	stringBuffer++;
  }
}

// Interrupt on recieve or transmit FIFO getting too full
void UART0_Handler(void){

	unsigned long status;
    status = UARTIntStatus(UART0_BASE, true); // Finding what casued interrupt (TXFIFO or RXFIFO)

	if(status == UART_INT_TX){
	  UARTIntClear(UART0_BASE, UART_INT_TX); // Clearing interrupt flag
	  copySoftwareToHardware();

	  if(TxFifo_Size() == 0){
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

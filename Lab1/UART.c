#include <string.h>
#include <stdio.h>

#include "Fifo.h"
#include "UART.h"
#include "ADC.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

#define START_STRING "\n\rVCom Initilization Done!\n\r"
#define SIZE_START_STRING 26 // Numer of chars of START_STRING

#define TRUE 1
#define FALSE 0
#define MAXCMDSIZE 32
#define BUFFERSIZE 20

#define FIFOSIZE   128         // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS 1         // return value on success
#define FIFOFAIL    0         // return value on failure

char CommandRx = FALSE;
char CMDCursor = 0;
char LastCMD[MAXCMDSIZE] = "";
char CurCMD[MAXCMDSIZE] = "";

AddIndexFifo(Rx, FIFOSIZE, char, FIFOSUCCESS, FIFOFAIL)
AddIndexFifo(Tx, FIFOSIZE, char, FIFOSUCCESS, FIFOFAIL)

// Function Protoypes
void UART0_Handler(void);
void UART0_SendString(char *stringBuffer);
void UART_OutChar(char data);

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
  UARTIntRegister(UART0_BASE, UART0_Handler);
  UARTEnable(UART0_BASE);
  IntEnable(INT_UART0);
  UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT); 

  // Send string to show that UART is initialized
  UART0_SendString((unsigned char *) START_STRING);
}

char CMD_Status(void) {
  char letter;
  
  if(RxFifo_Get(&letter) == FIFOFAIL){
    return FIFOFAIL;
  }

  if(letter == '\n' || letter == '\r'){
    // Print new line if user presses ENTER
	UART_OutChar('\n'); // Echo to screen
	UART_OutChar('\r');
	CurCMD[CMDCursor] = '\0';	// Terminate string
	strncpy(LastCMD, CurCMD, MAXCMDSIZE); // Store into LastCMD
	CMDCursor = 0;
	return SUCCESS;
	// Use last command if user presses ESC
  } else if(letter == 0x1B){
	strncpy(CurCMD, LastCMD, MAXCMDSIZE);
	CMDCursor = strlen(LastCMD);
	CurCMD[CMDCursor] = '\0';
	UART0_SendString(CurCMD);
	// Erase character if user presses BS
  } else if(letter == 0x07F) {
    UART_OutChar(letter);
	CMDCursor--;
	CurCMD[CMDCursor] = '\0';
  } else {
    // Save char typed if user press key
    UART_OutChar(letter);
    CurCMD[CMDCursor] = letter;
    CMDCursor++;
  }

  return FAILURE;
}

void CMD_Run(void) {
  
  unsigned long measurement;
  char buffer[20];

  switch(LastCMD[0]){
    case 'a':
	  measurement = ADC_In(LastCMD[1] - 0x30);
	  snprintf(buffer, BUFFERSIZE, "ADC%c: %d\n\r", LastCMD[1], measurement);
	  UART0_SendString(buffer);
	  break;
  }


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

// Output ASCII character to UART
// Spin if TxFifo is full
void UART_OutChar(char data){
  while(TxFifo_Put(data) == FIFOFAIL){};
  UARTIntDisable(UART0_BASE, UART_INT_TX);
  copySoftwareToHardware();
  UARTIntEnable(UART0_BASE, UART_INT_TX | UART_INT_RX | UART_INT_RT);
}


void UART0_SendString(char *stringBuffer){
  // Loop while there are more characters to send.
  while(*stringBuffer) {
    UART_OutChar(*stringBuffer);
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

    if((status == UART_INT_RX) || (status == UART_INT_RT)){
	  UARTIntClear(UART0_BASE, status); // Clearing interrupt flag
	  copyHardwareToSoftware();
	}
}

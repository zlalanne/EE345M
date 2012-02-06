#include "Fifo.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

#define START_STRING "\r\nVCom Initilization Done!\n\r"
#define SIZE_START_STRING 26 // Numer of chars of START_STRING
#define BAUD 9600
#define MAXTRIES 200

#define TRUE 1
#define FALSE 0

#define FIFOSIZE   128         // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS 1         // return value on success
#define FIFOFAIL    0         // return value on failure

AddIndexFifo(Rx, FIFOSIZE, char, FIFOSUCCESS, FIFOFAIL)
AddIndexFifo(Tx, FIFOSIZE, char, FIFOSUCCESS, FIFOFAIL)

// Function Protoypes
void UART0_Handler(void);
void UART0_SendString(unsigned char *stringBuffer);

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
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
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
void UART_OutChar(unsigned char data){
  while(TxFifo_Put(data) == FIFOFAIL){};
  UARTIntDisable(UART0_BASE, UART_INT_TX);
  copySoftwareToHardware();
  UARTIntEnable(UART0_BASE, UART_INT_TX | UART_INT_RX | UART_INT_RT);
}

// Input ASCII character from UART
// Spin if RxFifo is empty
unsigned char UART_InChar(void){
  char letter;
  char tries = MAXTRIES;
  while((RxFifo_Get(&letter) == FIFOFAIL) && tries > 0){
    tries--;
    letter = '\0';
  };
  return(letter);
}

void UART0_SendString(unsigned char *stringBuffer){
  // Loop while there are more characters to send.
  while(*stringBuffer) {
    UARTCharPut(UART0_BASE, *stringBuffer);
	stringBuffer++;
  }
}

char Read_CharTyped(char *typedChar){
  if(RxFifo_Get(typedChar) == FIFOSUCCESS){
    return TRUE;
  } else {
    return FALSE;
  }
}

// Recieves a <enter>/CR terminated string
void UART0_RecieveString(unsigned char *stringBuffer, unsigned short max) {
  int length = 0;
  char character;
  character = UART_InChar();
  
  while(character != '\n' && character != '\0'){
    if(length < max){
      *stringBuffer = character;
      stringBuffer++;
      length++;
    }
    character = UART_InChar();
  }
  *stringBuffer = 0;
}

// Interrupt on recieve from XBee or transmit FIFO getting too full
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

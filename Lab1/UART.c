#include <stdio.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

#include "SysTick.h"
#include "FIFO.h"
#include "XBeeTX.h"
#include "VComUART.h"
					
#define OVRSEC 110	
#define TWENTMS 2

#define TRUE 1
#define FALSE 0

#define FIFOSIZE   128         // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS 1         // return value on success
#define FIFOFAIL    0         // return value on failure

// Create index implementation FIFO (see FIFO.h)
AddIndexFifo(Rx, FIFOSIZE, char, FIFOSUCCESS, FIFOFAIL)
AddIndexFifo(Tx, FIFOSIZE, char, FIFOSUCCESS, FIFOFAIL)

char FrameID = 1;
char numOK = 0;	// For debugging purposes, allows you to see number of OK's recieved during initilization

// Function Protoypes
void UART1_Handler(void);
void sendInitCommand(void);
void sendAtCommand(unsigned char *stringBuffer);


void XBeeTX_Init(void) {

  char commandBuffer[MAXCOMMANDSIZE];
  VCom_SendString((unsigned char*) "Starting XBee Initilization\r\n");
  // Enabling the peripherals
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

  // Initlizing FIFOs
  RxFifo_Init();     
  TxFifo_Init();
  
  // Disable UART during initlization
  UARTDisable(UART1_BASE);

  // Configuring the pins needed for UART
  GPIOPinConfigure(GPIO_PD2_U1RX);
  GPIOPinConfigure(GPIO_PD3_U1TX);
  GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);

  // Configure clock
  UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), BAUD,
    (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
	UART_CONFIG_PAR_NONE));
  
  // Configuring UART1 Hardware FIFOs
  UARTFIFOEnable(UART1_BASE);
  UARTFIFOLevelSet(UART1_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);

  // Enabling UART1 and configuring interrupts
  UARTEnable(UART1_BASE);
  UARTIntRegister(UART1_BASE, UART1_Handler);
  IntEnable(INT_UART1);
  UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT | UART_INT_TX); // Interrupt on recieve/transfer/receive-timeout 
  
  // Enter command mode and intialize XBee
  sendInitCommand();

  sprintf(commandBuffer, "ATDL%s\r\0", LOWDESTADDRSTR);
  sendAtCommand((unsigned char*) commandBuffer); // Setting destination low address
  sendAtCommand("ATDH0\r"); // Setting destination high address

  sprintf(commandBuffer, "ATMY%s\r\0", MYADDR); // Setting my address
  sendAtCommand((unsigned char*) commandBuffer);

  sendAtCommand("ATAP1\r");
  sendAtCommand("ATCN\r");

  VCom_SendString((unsigned char*) "\r\nXBee Initlization Done!\r\n");

  return;
}

// Copy from hardware RX FIFO to software RX FIFO
// Stop when hardware RX FIFO is empty or software RX FIFO is full
void copyHardwareToSoftware(void){
  char letter;
  while((UARTCharsAvail(UART1_BASE) != false) && (RxFifo_Size() < (FIFOSIZE - 1))){
    letter = (char) UARTCharGetNonBlocking(UART1_BASE);
    RxFifo_Put(letter);
  }
}


// Copy from software TX FIFO to hardware TX FIFO
// Stop when software TX FIFO is empty or hardware TX FIFO is full
void copySoftwareToHardware(void){
  char letter;

  while((UARTSpaceAvail(UART1_BASE) != false) && (TxFifo_Size() > 0)) {
    TxFifo_Get(&letter);
	UARTCharPutNonBlocking(UART1_BASE, letter);
  }
}

// Puts XBee in API mode
void sendInitCommand(void) {

  unsigned char message[2];
  int i;

  for(i = 0; i < MAXTRIES; i++) {

    XBee_SendString("X");
	VCom_SendString((unsigned char*) "TX: X\r\n");
	SysTick_Wait10ms(OVRSEC);
	XBee_SendString("+++");
	VCom_SendString((unsigned char*) "TX: +++\r\n");
	SysTick_Wait10ms(OVRSEC);

	XBee_RecieveString(message, 2);
	VCom_SendString((unsigned char*) "RX: ");
	VCom_SendString((unsigned char*) message);

	if(message[0] == 'O' && message[1] == 'K') {
		i = MAXTRIES;
		numOK++;
	}
  }
}

// Sends AT commands to XBEE
void sendAtCommand(unsigned char *stringBuffer) {

  unsigned char message[2];
  int i;

  for(i = 0; i < MAXTRIES; i++) {

	XBee_SendString(stringBuffer);
	VCom_SendString((unsigned char*) "\r\nTX: ");
	VCom_SendString((unsigned char*) stringBuffer);
	SysTick_Wait10ms(TWENTMS);

	XBee_RecieveString(message, 2);
	VCom_SendString((unsigned char*) "\r\nRX: ");
	VCom_SendString((unsigned char*) message);

	if(message[0] == 'O' && message[1] == 'K') {
		i = MAXTRIES;
		numOK++;
	}
  }
}

// Creates a transfer frame
int Create_TransferFrame(unsigned char *message, unsigned char *frame, unsigned short length){
   	
  int i;
  int accum = 0;
  unsigned short frameLength = length + LENGTHOFMESSAGE;

  frame[0] = 0x7E;

  // Determining length
  frame[1] = (unsigned char) (frameLength >> 8);
  frame[2] = (unsigned char) (frameLength & 0x00FF);
  
  // FrameID & API
  frame[3] = 0x01;
  frame[4] = FrameID; FrameID++;
  if(FrameID == 256) {
    FrameID = 1;
  }

  // Destination
  frame[5] = 0x00; // High byte of address
  frame[6] = LOWDESTADDRHEX; // Low byte of address

  // Options & Message
  frame[7] = 0x00;
  for(i = 0; i < length; i++){
    frame[8 + i] = message[i];
  }	

  // Checksum
  for(i = 3; i < (length + 8); i++){
    accum += frame[i];
  }
  accum = (0xFF - accum) & 0xFF;
  frame[8 + length] = (unsigned char) accum;

  return frameLength + 4;
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

// Output ASCII character to UART
// Spin if TxFifo is full
void UART_OutChar(unsigned char data){
  while(TxFifo_Put(data) == FIFOFAIL){};
  UARTIntDisable(UART1_BASE, UART_INT_TX);
  copySoftwareToHardware();
  UARTIntEnable(UART1_BASE, UART_INT_TX | UART_INT_RX | UART_INT_RT);
}

// Input: needs to be NULL terminated string
void XBee_SendString(unsigned char *stringBuffer){
  // Loop while there are more characters to send.
  while(*stringBuffer) {
    UART_OutChar(*stringBuffer);
	stringBuffer++;
  }
}

// Input: does not need to be NULL terminated, just need to know length
void XBee_SendTxFrame(unsigned char *stringBuffer, int length){

  while(length > 0) {
    UART_OutChar(*stringBuffer);
	stringBuffer++;
	length--;
  }

}

// Determines status of last sent frame
char XBee_TxStatus(void){
  
  char start, lengthLow, lengthHigh, api, id, message, checksum;
  start = UART_InChar();
  if(start != 0x7E) {
    return 1;
  }
  
  lengthHigh = UART_InChar();
  lengthLow = UART_InChar();
  if(lengthHigh != 0x00 || lengthLow != 0x03) {
    return 1;
  }
  
  api = UART_InChar();
  if(api != 0x89) {
    return 1;
  }
  
  id = UART_InChar();
  
  message = UART_InChar();
  if(message != 0x00) {
    return 1;
  }
  
  checksum = UART_InChar();

  /*
  checksum = UART_InChar();
  if((checksum + 0xFF) & 0xFF != 0x00) {
    return 1;
  }									*/
  
  return 0;  
}

// Recieves a <enter>/CR terminated string
void XBee_RecieveString(unsigned char *stringBuffer, unsigned short max) {
  int length = 0;
  char character;
  character = UART_InChar();
  
  while(character != CR && character != '\0'){
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
void UART1_Handler(void){

	unsigned long status;
    status = UARTIntStatus(UART1_BASE, true); // Finding what casued interrupt (TXFIFO or RXFIFO)

	if(status == UART_INT_TX){
	  UARTIntClear(UART1_BASE, UART_INT_TX); // Clearing interrupt flag
	  copySoftwareToHardware();

	  if(TxFifo_Size() == 0){
	    UARTIntDisable(UART1_BASE, UART_INT_TX); // Going too fast, need to disable TX FIFO interrupt
	  }
	}

    if((status == UART_INT_RX) || (status == UART_INT_RT)){
	  UARTIntClear(UART1_BASE, status); // Clearing interrupt flag
	  copyHardwareToSoftware();
	}
}

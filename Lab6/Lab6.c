// Lab6.c     MOTHER BOARD (8962)

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"

#include "OS.h"
#include "UART.h"
#include "ADC.h"
#include "Output.h"
#include "rit128x96x4.h"
#include "CAN0.h"
#include "Motor.h"

#define TIMESLICE 2*TIME_1MS

unsigned long NumCreated;

void CanConsumer(void) {
	// just prints out the can data on the oLED
	unsigned long data;
	while(1) {
	  if(CAN0_GetMailNonBlock(&data,TACH_ID)) {
		  oLED_Message(0,0,"Tach Data:", data);
	  }
	  if(CAN0_GetMailNonBlock(&data,RCV_ID)) {
		  oLED_Message(0,1,"Gen Data:", data);
	  } 
	}
}


int main(void) {
	
  SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
  
	UART0_Init();
	Output_Init();
	CAN0_Open();
	OS_Fifo_Init(512);
	OS_Init(); 
	
	Motor_Init();
	Motor_Start();
	
	NumCreated = 0;
	NumCreated += OS_AddThread(&Interpreter, 512, 1);
	NumCreated += OS_AddThread(&CanConsumer, 512, 1);
	OS_Launch(TIMESLICE);  // doesn't return, interrupts enabled in here
	return 0;  // This never executes	
}

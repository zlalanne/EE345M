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
#include "IR.h"
#include "Motor.h"
#include "Ping.h"

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
		  oLED_Message(0,1,"CAN Data:", data);
	  } 
	}
}

void IRConsumer(void) {
	 // Prints out all IR data to oLED
	unsigned short data;
	
	IR_Init();
	
	while(1){
		data = IR_GetDistance(0);
		oLED_Message(1,0,"IR 0:",data);
		data = IR_GetDistance(1);
		oLED_Message(1,1,"IR 1:",data);
		data = IR_GetDistance(2);
		oLED_Message(1,2,"IR 2:",data);
		data = IR_GetDistance(3);
		oLED_Message(1,3,"IR 3:",data);
	}
}

void PingConsumer(void) {
	
	unsigned long data;
	Ping_Init();
	
	while(1){
		data = Ping_GetDistance(0);
		oLED_Message(0,2,"Ping 0:",data);
	  data = Ping_GetDistance(1);
		oLED_Message(0,3,"Ping 1:",data);
  }
}


int main(void) {
	
  SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
  
	ADC_Open();
	UART0_Init();
	Output_Init();
	CAN0_Open();
	OS_Fifo_Init(512);
	OS_Init(); 
	
	Motor_Init();
	Motor_Start();
	
	NumCreated = 0;
	NumCreated += OS_AddThread(&Interpreter, 512, 3);
	NumCreated += OS_AddThread(&CanConsumer, 512, 1);
	NumCreated += OS_AddThread(&IRConsumer, 512, 1);
	NumCreated += OS_AddThread(&PingConsumer, 512, 1);
	OS_Launch(TIMESLICE);  // doesn't return, interrupts enabled in here
	return 0;  // This never executes	
}

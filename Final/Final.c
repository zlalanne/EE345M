// Final.c

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"

#include "Ping.h"
#include "IR.h"
#include "OS.h"
#include "UART.h"
#include "ADC.h"
#include "Output.h"
#include "rit128x96x4.h"
#include "CAN0.h"
#include "Servo.h"

#define TIMESLICE 2*TIME_1MS

unsigned long NumCreated;


void Display(void) {
	// just prints the current servo info to the oLED
	oLED_Message(0,0, "servo ticks:",Servo_Pulse_Get());
	oLED_Message(0,1, "servo percent:", 0);
}

int main(void) {
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
  
	//ADC_Open();
	UART0_Init();
	Output_Init();
	Servo_Init();
	Servo_Start();
	//CAN0_Open();
	OS_Init();
	
	NumCreated = 0;
	NumCreated += OS_AddThread(&Interpreter, 512, 3);
	NumCreated += OS_AddThread(&Display, 512, 3);	
	OS_Launch(TIMESLICE);
	return 0;	
}

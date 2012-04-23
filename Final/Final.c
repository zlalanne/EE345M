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
#include "Motor.h"

#define TIMESLICE 2*TIME_1MS

unsigned long NumCreated;


void Display(void) {

Servo_Set_Position(50000);
	while(1) {
	  // just prints the current servo info to the oLED
	  //oLED_Message(0,0, "servo ticks:",Servo_Pulse_Get());
	  // 45,000 to 105,000
	  //OS_Sleep(1000); // sleep a second
//	  Servo_Set_Position(37500);
	  oLED_Message(0,0, "servo ticks:",Servo_Pulse_Get());
//	  OS_Sleep(2000);
//	  Servo_Set_Position(15000);
//	  oLED_Message(0,0, "servo ticks:",Servo_Pulse_Get());
//	  OS_Sleep(2000);
//	  Servo_Set_Position(60000);
//	  oLED_Message(0,0, "servo ticks:",Servo_Pulse_Get());
//	  OS_Sleep(2000);
	  //Servo_Set_Position(45000);
	  //oLED_Message(0,0, "servo ticks:",Servo_Pulse_Get());
	  //OS_Sleep(1000);
	  //Servo_Set_Position(80000);
	  //oLED_Message(0,0, "servo ticks:",Servo_Pulse_Get());
	  //OS_Sleep(3000);
    }
}


void MotorControl(void) {

  // Signal to start the motors
  CAN0_SendData(MOTOR_START, MOTOR_XMT_ID);

  // Signal to go straight
  //CAN0_SendData(MOTOR_STRAIGHT, MOTOR_XMT_ID);

  // Sleep three minutes
  //OS_Sleep(100);
  
  // Signal to stop the motors
  CAN0_SendData(MOTOR_STOP, MOTOR_XMT_ID);

  // Killing the thread
  OS_Kill();

}

int main(void) {
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
  
	//ADC_Open();
	UART0_Init();
	Output_Init();
	Servo_Init();
	Servo_Start();
	CAN0_Open();
	OS_Init();
	
	NumCreated = 0;
	NumCreated += OS_AddThread(&MotorControl, 512, 1);
	NumCreated += OS_AddThread(&Interpreter, 512, 3);
	NumCreated += OS_AddThread(&Display, 512, 3);	
	OS_Launch(TIMESLICE);
	return 0;	
}

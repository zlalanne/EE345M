// Final.c

#include <stdlib.h>

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
#define PIDDepth 4

unsigned long NumCreated;

void PID(void) {
  // start with PD for now
 
  unsigned long PingL = 0;
  unsigned long PingR = 0;
  unsigned long IRL = 0;
  unsigned long IRR = 0;
  unsigned long Left = 0;
  unsigned long Right = 0;
  static long Errors[PIDDepth] = {};
  long Integral = 0;
  long Derivate = 0;
  long Output = 0;

  long Kp = 10;
  long Ki = 2;
  long Kd = 5;

  while(1) {

    // Get all 4 sensor values
    IRL = IR_GetDistance(0);
    IRR = IR_GetDistance(1);
    PingL = Ping_GetDistance(0);
    PingR = Ping_GetDistance(1);

    // change weighting to use barrel shifter
    Left =(.7)*PingL + (0.3)*IRL;
    Right = (.7)*PingR + (0.3)*IRR;

    Errors[0] = Left - Right; // negative errors mean turn right

	// Calculate Derivate
	//D(n) = ([E(n) + 3E(n-1) - 3E(n-2) - E(n-3)]/(6*t))
	Derivate = (Errors[0] + 3*Errors[1] - 3*Errors[2] - Errors[3]) / (6 * PIDperiod);
    
	Output = (Kp*Errors[0]) + (Ki*Integral) + (Kd*Derivative);
    // if errors are all 0 output should be 0 degrees

    

	// Update Errors
	Errors[3] = Errors[2];
	Errors[2] = Errors[1];
	Errors[1] = Errors[0];

  }
}


void MotorControl(void) {

  // Signal to start the motors
  CAN0_SendData(MOTOR_START, MOTOR_XMT_ID);

  // Signal to go straight
  CAN0_SendData(MOTOR_STRAIGHT, MOTOR_XMT_ID);

  // Sleep three minutes
  OS_Sleep(5000);
  
  // Signal to stop the motors
  CAN0_SendData(MOTOR_STOP, MOTOR_XMT_ID);

  // Killing the thread
  OS_Kill();

}

int main(void) {
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
  
	ADC_Open();
	UART0_Init();
	Output_Init();
	Servo_Init();
	Servo_Start();
	CAN0_Open();
	IR_Init();
	Ping_Init();
	OS_Init();
	
	NumCreated = 0;
	NumCreated += OS_AddThread(&MotorControl, 512, 1);
	NumCreated += OS_AddThread(&Interpreter, 512, 3);
	NumCreated += OS_AddThread(&PID, 512, 1);	
	OS_Launch(TIMESLICE);
	return 0;	
}

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
#define PIDPeriod 50  // in ms now
#define PIDSystemPeriod 19531

#define ZERO_POSITION 41100
#define MinOut 27000
#define MaxOut 53000

unsigned long NumCreated;
unsigned long g0;
unsigned long g1;
unsigned long g2;
unsigned long g3;
unsigned long gLeft;
unsigned long gRight;
long gError;
long gDerivative;
long gOutput;
long gOutputFinal;



void Display(void) {
// just prints some data to UART for PID tweaking
  while(1) {
  //UARTprintf("----------------------------------\n\r");
  //UARTprintf("0: %d cm\n\r", g0);
  //UARTprintf("1: %d cm\n\r", g1);
  //UARTprintf("2: %d cm\n\r", g2);
  //UARTprintf("3: %d cm\n\r", g3);
  //UARTprintf("Left: %d cm\n\r", gLeft);
  //UARTprintf("Right: %d cm\n\r", gRight);
  //UARTprintf("Error: %d cm\n\r", gError);
  //UARTprintf("Derivative: %d \n\r", gDerivative);
  //UARTprintf("Output Dif: %d \n\r", gOutput);
  //UARTprintf("Output Final: %d \n\r", gOutputFinal);
  }
}

void PID(void) {
  // start with PD for now
 
  unsigned long IRL1 = 0;
  unsigned long IRL2 = 0;
  unsigned long IRR1 = 0;
  unsigned long IRR2 = 0;
  unsigned long Left = 0;
  unsigned long Right = 0;
  static long Errors[PIDDepth] = {0,};
  long Integral = 0;
  long Derivative = 0;
  long Output = 0;

  long Kp = 400; // 384;  // 384/256 = 1.5
  long Ki = 0;
  long Kd = 80; //64;   // 384/256 = .25
  IR_Init();
  //while(1) {
  // Get all 4 sensor values
  IRL1 = IR_GetDistance(2);  // should be the side left ir
  g2 = IRL1;
  IRL2 = IR_GetDistance(0);  // should be the front left
  g0 = IRL2;
  IRR1 = IR_GetDistance(3); 
  g3 = IRR1;
  IRR2 = IR_GetDistance(1);
  g1 = IRR2;
  
    //UARTprintf("----------------------------------\n\r");
    //UARTprintf("Front Left: %d cm\n\r", IRL2);
	//UARTprintf("Front Right: %d cm\n\r", IRR2);
	//UARTprintf("Side Left: %d cm\n\r", IRL1);
	//UARTprintf("Side Right: %d cm\n\r", IRR1);

    // change weighting to use barrel shifter
    Left = (((153*IRL2) + (102*IRL1))/256); //(((179*IRL1) + (77*IRL2))/256);
    Right = (((153*IRR2) + (102*IRR1))/256); //(((179*IRR1) + (77*IRR2))/256);

    gLeft = Left;
	gRight = Right;

    //UARTprintf("Left: %d\n\r", Left);
    //UARTprintf("Right: %d\n\r", Right);

    Errors[0] = Left - Right; // negative errors mean turn right

    gError = Errors[0];
    //UARTprintf("Current P: %d\n\r", Errors[0]);

    // Calculate Derivate
    //D(n) = ([E(n) + 3E(n-1) - 3E(n-2) - E(n-3)]/(6*t))
	Derivative = (Errors[0] + 3*Errors[1] - 3*Errors[2] - Errors[3]) / (6 * PIDPeriod);
    //UARTprintf("Current D: %d\n\r", Derivative);

	gDerivative = Derivative;
	
	Output = (Kp*Errors[0]) + (Ki*Integral) + (Kd*Derivative);
	
    gOutput = Output;

	Output += ZERO_POSITION;
	
	if (Output > MaxOut) {
	  Output = MaxOut;
	} else if (Output < MinOut) {
	  Output = MinOut;
	}

	gOutputFinal = Output;
    // if errors are all 0 output should be 0 degrees
    //UARTprintf("Current Output: %d\n\r", Output);

	// Update Errors
	Errors[3] = Errors[2]; 
	Errors[2] = Errors[1];
    Errors[1] = Errors[0];
    
	// Send change to servo
	Servo_Set_Degrees(Output); 
    //OS_Sleep(500);
	//}
  
}

void PingConsumer(void) {
  
  unsigned short data;
  IR_Init();
  
  while(1) {
    
	//data = Ping_GetDistance(0);
	//UARTprintf("Ping0: %u\n\r", data);
	//data = Ping_GetDistance(1);
	//UARTprintf("Ping1: %u\n\r", data);
	
	data = IR_GetDistance(0);
	UARTprintf("IR0: %d\n\r", data);
//	data = IR_GetDistance(1);
//	UARTprintf("IR1: %d\n\r", data);
//	data = IR_GetDistance(2);
//	UARTprintf("IR2: %d\n\r", data);
//	data = IR_GetDistance(3);
//	UARTprintf("IR3: %d\n\r", data);	
	OS_Sleep(1000);
  }

}

void MotorControl(void) {

  // Signal to start the motors
  CAN0_SendData(MOTOR_START, MOTOR_XMT_ID);

  // Signal to go straight
  CAN0_SendData(MOTOR_STRAIGHT, MOTOR_XMT_ID);

  // Sleep three minutes
  OS_Sleep(60000);
  
  // Signal to stop the motors
  //CAN0_SendData(MOTOR_STOP, MOTOR_XMT_ID);

  //OS_Sleep(3000);

  //CAN0_SendData(MOTOR_START, MOTOR_XMT_ID);

  //OS_Sleep(15000);

  CAN0_SendData(MOTOR_STOP, MOTOR_XMT_ID);

  // Killing the thread
  OS_Kill();

}

int main(void) {
	
  SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
  
  Ping_Init();
  ADC_Open();
  UART0_Init();
  Output_Init();
  Servo_Init();
  Servo_Start();
  CAN0_Open();
  
  OS_Init();
	
  NumCreated = 0;
  NumCreated += OS_AddThread(&MotorControl, 512, 1);
  NumCreated += OS_AddThread(&Interpreter, 512, 3);
  NumCreated += OS_AddThread(&Display, 512, 1);
  NumCreated += OS_AddPeriodicThread(&PID, 1, PIDSystemPeriod, 1);	
  OS_Launch(TIMESLICE);
  return 0;	
}

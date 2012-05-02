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
#define PIDDepth 40
#define PIDSystemPeriod 9765

#define ZERO_POSITION 41100
#define MinOut 27000
#define MaxOut 53000

#define USESTATEMACHINE 1

#define StraightKp 10
#define StraightKi 8
#define StraightKd 100

#define NormalKp 14
#define NormalKi 30
#define NormalKd 100

#define ShitKp 50
#define ShitKi 0
#define ShitKd 0

unsigned long NumCreated;
unsigned long g0;
unsigned long g1;
unsigned long g2;
unsigned long g3;
unsigned long gLeft;
unsigned long gRight;
unsigned long gStraightHistory = 0;

long gError;
long gDerivative;
long gIntegral;
long gIntegral2;
long gOutput;
long gOutputFinal;

long gKp = NormalKp;
long gKi = NormalKi;
long gKd = NormalKd;
long gMotorRunTime = 30000;
long gDisplay;
	  
long gErrors[PIDDepth];
long *gCurrent = &gErrors[0];

// both sides are within 8 cm of each other, maybe with an average across samples
// both forward sensors are between 40 and 70 cm
unsigned long gStateStraight = 0; // STATE 0

unsigned long gStateNormal = 1; // STATE 1

// both front sensors are less that 25 cm, should basically saturate the servo
unsigned long gStateOhShit = 0; // STATE 2

unsigned long gStateNumber = 1;

void Display(void) {
  while(1) {
    if(gDisplay) {
      //UARTprintf("----------------------------------\n\r");
      //UARTprintf("0: %d cm\n\r", g0);
      //UARTprintf("1: %d cm\n\r", g1);
      //UARTprintf("2: %d cm\n\r", g2);
      //UARTprintf("3: %d cm\n\r", g3);
      UARTprintf("0: %d cm   1: %d cm   2: %d cm   3: %d cm\r\n", g0, g1, g2, g3);
      //UARTprintf("State Number: %d     %d     %d\n\r", gStateNumber, gStateNumber, gStateNumber);
      //UARTprintf("Left: %d cm\n\r", gLeft);
      //UARTprintf("Right: %d cm\n\r", gRight);
      //UARTprintf("Error: %d cm\n\r", gError);
      //UARTprintf("Derivative: %d \n\r", gDerivative);
      //UARTprintf("Output Dif: %d \n\r", gOutput);
      //UARTprintf("Output Final: %d \n\r", gOutputFinal);
      //UARTprintf("Errors: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\r\n", *gCurrent, *(gCurrent + 1), *(gCurrent + 2), *(gCurrent + 3), *(gCurrent + 4), *(gCurrent + 5), *(gCurrent + 6), *(gCurrent + 7), *(gCurrent + 8), *(gCurrent + 9), *(gCurrent + 10), *(gCurrent + 11), *(gCurrent + 12), *(gCurrent + 13), *(gCurrent + 14), *(gCurrent + 15), *(gCurrent + 16), *(gCurrent + 17), *(gCurrent + 18), *(gCurrent + 19), *(gCurrent + 20));
      //UARTprintf("Sum or Errors: %d   %d\r\n", gIntegral, gIntegral2);
    }
  }
}

void PID(void) {

  static int lastState = 1;
  unsigned long index;
  unsigned long IRLB = 0;
  unsigned long IRLF = 0;
  unsigned long IRRB = 0;
  unsigned long IRRF = 0;
  unsigned long Left = 0;
  unsigned long Right = 0;
  //static long Errors[PIDDepth] = {0,};
  //long *gCurrent = &gErrors[0];
  long Integral = 0;
  long Derivative = 0;
  long Output = 0;

  static long Kp = NormalKp;
  static long Ki = NormalKi;
  static long Kd = NormalKd;

  long Difference;

  // Get all 4 sensor values
  IRLB = IR_GetDistance(2); // should be the side left ir
  g2 = IRLB;
  IRLF = IR_GetDistance(0); // should be the front left
  g0 = IRLF;
  IRRB = IR_GetDistance(3);
  g3 = IRRB;
  IRRF = IR_GetDistance(1);
  g1 = IRRF;

  Left = (IRLB + IRLF) / 2;
  Right = (IRRB + IRRF) / 2;

  gLeft = Left;
  gRight = Right;

  Integral = Integral - *(gCurrent + 10); // remove the one from 10 samples ago
  if(gCurrent == &gErrors[0]) {
    gCurrent = &gErrors[9]; // Wrap
  } else {
    gCurrent--;
  }
  *gCurrent = *(gCurrent + 10) = Left - Right;
  Integral += *gCurrent;
  gIntegral = Integral;
  gIntegral2 = 0;

	// Calculating integral
  for(index = 0; index < 10; index++) {
    gIntegral2 += *(gCurrent + index);
  }

  gError = *gCurrent; //Errors[0];

  // Calculate Derivative
  //D(n) = ([E(n) + 3E(n-1) - 3E(n-2) - E(n-3)]/(6*t))
  Derivative = *(gCurrent) - *(gCurrent + 1); //Errors[1] - Errors[0];
  //Derivative = (Errors[0] + 3*Errors[1] - 3*Errors[2] - Errors[3]) / (6 * PIDPeriod);

  gDerivative = Derivative;

  // Calculate state change
  if(USESTATEMACHINE) {
    // check if in straight state
    // both sides are within 8 cm of each other, maybe with an average across samples
    // both forward sensors are between 40 and 70 cm
    Difference = IRLB - IRRB;
    if(Difference < 0) {
      Difference = Difference * -1;
    }
    if((Difference < 80) && (IRLF > 400) && (IRRF > 400) ) {
	  gStraightHistory++;
	  if (gStraightHistory > 10) {
	    // Straight State
		gStateStraight = 1;
	    gStateNormal = 0;
	    gStateOhShit = 0;
	    if(lastState != 0) {
	      // send message
	      CAN0_SendData(MOTOR_SPEED1, MOTOR_XMT_ID);
	      // set weights
	      Kp = StraightKp;
	      Ki = StraightKi;
	      Kd = StraightKd;
	      lastState = 0;
	      gStateNumber = 0; 
        } 
	  } else {
	    // Normal State, not enough straight
		// history to get into straight state
	    gStateStraight = 0;
        gStateNormal = 1;
        gStateOhShit = 0;
        if(lastState != 1) {
          // send message
          CAN0_SendData(MOTOR_SPEED2, MOTOR_XMT_ID);
          // set weights
          Kp = NormalKp;
          Ki = NormalKi;
          Kd = NormalKd;
          lastState = 1;
          gStateNumber = 1;
        }
	  }
    } else if((IRLF < 300) && (IRRF < 300)) {
      // OhShit State
	  gStraightHistory = 0;
	  gStateStraight = 0;
      gStateNormal = 0;
      gStateOhShit = 1;
      if(lastState != 2) {
        // send message
        CAN0_SendData(MOTOR_SPEED3, MOTOR_XMT_ID);
        // set weights
        Kp = ShitKp;
        Ki = ShitKi;
        Kd = ShitKd;
        lastState = 2;
        gStateNumber = 2;
      }
    } else {
	  // Normal State
	  gStraightHistory = 0;
      gStateStraight = 0;
      gStateNormal = 1;
      gStateOhShit = 0;
      if(lastState != 1) {
        // send message
        CAN0_SendData(MOTOR_SPEED2, MOTOR_XMT_ID);
        // set weights
        Kp = NormalKp;
        Ki = NormalKi;
        Kd = NormalKd;
        lastState = 1;
        gStateNumber = 1;
      }
    }
  }
  // both front sensors are less that 25 cm, should basically saturate the servo

  Output = (Kp * (*gCurrent)) + (Ki * gIntegral2) / 256 + (Kd * Derivative);

  gOutput = Output;

  Output += ZERO_POSITION;

  if(Output > MaxOut) {
    Output = MaxOut;
  } else if(Output < MinOut) {
    Output = MinOut;
  }

  gOutputFinal = Output;

  // Send change to servo
  Servo_Set_Degrees(Output);
}

void MotorControl(void) {

  // Signal to start the motors
  CAN0_SendData(MOTOR_START, MOTOR_XMT_ID);
  CAN0_SendData(MOTOR_SPEED2, MOTOR_XMT_ID);
  // Sleep three minutes
  OS_Sleep(gMotorRunTime);
  OS_Sleep(gMotorRunTime);
  OS_Sleep(50000);

  CAN0_SendData(MOTOR_STOP, MOTOR_XMT_ID);

  // Killing the thread
  OS_Kill();

}

void StartMotors(void) {
  OS_AddThread(&MotorControl, 512, 1);
}

int main(void) {

  SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);

  ADC_Open();
  UART0_Init();
  Output_Init();
  Servo_Init();
  Servo_Start();
  CAN0_Open();

  OS_Init();

  IR_Init();

  NumCreated = 0;
  NumCreated += OS_AddThread(&Interpreter, 512, 3);
  NumCreated += OS_AddThread(&Display, 512, 1);
  NumCreated += OS_AddPeriodicThread(&PID, 1, PIDSystemPeriod, 1);
  NumCreated += OS_AddButtonTask(&StartMotors, 2);
  OS_Launch(TIMESLICE);
  return 0;
}

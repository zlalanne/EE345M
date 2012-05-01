// FinalDaughter.c

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"

#include "../OS.h"
#include "../QRB1134.h"
#include "../CAN0.h"
#include "../Motor.h"

#define TIMESLICE 2*TIME_1MS  // thread switch time in system time units

double NumCreated;


//----------- MotorConsumer ----------
// Gets motor command from CAN0 and executes
void MotorConsumer(void) {

  unsigned long data;

  while(1){
    if(CAN0_GetMailNonBlock(&data,MOTOR_RCV_ID) == TRUE) {
			switch(data) {
				case MOTOR_START:
				Motor_Start(); break;
			case MOTOR_STOP:
				Motor_Stop(); break;
			case MOTOR_STRAIGHT:
				Motor_Straight(); break;
			case MOTOR_REVERSE:
				Motor_Reverse(); break;
			case MOTOR_LEFT:
				Motor_Turn_Left(); break;
			case MOTOR_RIGHT:
				Motor_Turn_Right(); break;
			case MOTOR_SPEED1:
				Motor_Speed1(); break;
			case MOTOR_SPEED2:
				Motor_Speed2(); break;
			case MOTOR_SPEED3:
				Motor_Speed3(); break;
			}
	  }
  }
}

//---------- DummyThread ----------
// Does nothing useful, should be lowest
// priority foreground thread
void DummyThread(void) {
	static unsigned long dummy = 0;
	while(1) {
		dummy++;
	}
}


int main(void){
	
  // bus clock at 25 MHz
  SysCtlClockSet(SYSCTL_SYSDIV_8 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                 SYSCTL_XTAL_8MHZ);  
	
  CAN0_Open();
  Tach_Init();
  Motor_Init();
  OS_Init();
  	
  OS_Fifo_Init(512);
	
  NumCreated = 0 ;
  NumCreated += OS_AddThread(&MotorConsumer, 512, 1);
  NumCreated += OS_AddThread(&DummyThread, 512, 6);
	
	
  OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
  return 0;             // this never executes
}


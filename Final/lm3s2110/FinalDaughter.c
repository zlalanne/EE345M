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

void TachThread(void){
	unsigned long data;
	while(1) {
		data = Tach_GetPeriod();
		CAN0_SendData(data, TACH_ID);
		OS_Sleep(2000);
  }
}

void GenDataThread(void) {
	static unsigned long countGen = 0;
	while(1) {
		CAN0_SendData(countGen++, XMT_ID);
		OS_Sleep(5000);
	}
}

void MotorThread(void) {

  Motor_Init();
  Motor_Start();
  while(1) {
    Motor_Straight();
  }
}

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
  OS_Init();
	
  OS_Fifo_Init(512);
	
	
  NumCreated = 0 ;
  NumCreated += OS_AddThread(&MotorThread, 512, 1);
  NumCreated += OS_AddThread(&DummyThread, 512, 2);
	
	
  OS_Launch(TIMESLICE); // doesn't return, interrupts enabledin here
  return 0;             // this never executes
}


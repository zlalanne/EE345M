
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"

#include "../OS.h"
#include "../QRB1134.h"
#include "../CAN0.h"


#define TIMESLICE 2*TIME_1MS  // thread switch time in system time units

double NumCreated;

void TachThread(void){
	unsigned long data;
	while(1) {
		data = Tach_GetPeriod();
		CAN0_SendData(data);
		data = Tach_GetRPS();
		CAN0_SendData(data);
		data = Tach_GetRPM();
		CAN0_SendData(data);
		
		// Add OS_Sleep()?
  }
}

int main(void){
	
	// bus clock at 25 MHz
  SysCtlClockSet(SYSCTL_SYSDIV_8 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                 SYSCTL_XTAL_8MHZ);  
	
	Tach_Init();
	CAN0_Open();
	
  OS_Init();
  OS_Fifo_Init(512);
	
	
  NumCreated = 0 ;
  NumCreated += OS_AddThread(&TachThread, 512, 1);
	
	
  OS_Launch(TIMESLICE); // doesn't return, interrupts enabledin here
  return 0;             // this never executes
}


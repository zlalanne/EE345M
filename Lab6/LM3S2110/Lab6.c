
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"

#include "../OS.h"
#include "../QRB1134.h"
#include "../CAN0.h"

// ID's for sending messages
#ifdef BOARD_LM3S2110
  #define RCV_ID 2
	#define TACH_ID 3
  #define XMT_ID 4
#endif

#ifdef BOARD_LM3S8962
  #define RCV_ID 4
	#define TACH_ID 3
	#define XMT_ID 2
#endif

#define TIMESLICE 2*TIME_1MS  // thread switch time in system time units

double NumCreated;

void TachThread(void){
	unsigned long data;
	static unsigned long countTach = 0;
	while(1) {
		CAN0_SendData(countTach++, TACH_ID);
		OS_Sleep(10000);
  }
}

void GenDataThread(void) {
	static unsigned long countGen = 0;
	while(1) {
		CAN0_SendData(countGen++, XMT_ID);
		OS_Sleep(5000);
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
	
	
	
	Tach_Init();
	CAN0_Open();
	
  OS_Init();
  OS_Fifo_Init(512);
	
	
  NumCreated = 0 ;
  NumCreated += OS_AddThread(&TachThread, 512, 1);
	NumCreated += OS_AddThread(&GenDataThread, 512, 1);
	NumCreated += OS_AddThread(&DummyThread, 512, 2);
	
	
  OS_Launch(TIMESLICE); // doesn't return, interrupts enabledin here
  return 0;             // this never executes
}


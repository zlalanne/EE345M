#include "OS.h"
#include "CAN.h"
#include "ADC.h"

#define TIMESLICE 2*TIME_1MS  // thread switch time in system time units



unsigned long NumCreated;   // number of foreground threads created
unsigned long NumSamples;   // incremented every sample
unsigned long DataLost;     // data sent by Producer, but not received by Consumer

//******** Producer *************** 
// The Producer in this lab will be called from your ADC ISR
// A timer runs at 1 kHz, started by your ADC_Collect
// The timer triggers the ADC, creating the 1 kHz sampling
// Your ADC ISR runs when ADC data is ready
// Your ADC ISR calls this function with a 10-bit sample 
// sends data to the Robot, runs periodically at 1 kHz
// inputs:  none
// outputs: none
void Producer(unsigned short data){  
	
	if(OS_Fifo_Put(data)){ 
		NumSamples++;
	} else{ 
		DataLost++;
	} 
}

//******** IdleTask  *************** 
// foreground thread, runs when no other work needed
// never blocks, never sleeps, never dies
// inputs:  none
// outputs: none
unsigned long Idlecount=0;
void IdleTask(void){ 
  while(1) { 
    Idlecount++;        // debugging 
  }
}

int maintx(void) {
	
	OS_Init();
	OS_Fifo_Init(512);
	CAN_Init();
	
	ADC_Collect(0, 1000, &Producer); // start ADC sampling, channel 0, 1000 Hz
	
	NumCreated = 0;
	NumCreated += OS_AddThread(&IdleTask,128,7);  // runs when nothing useful to do
	NumCreated += OS_AddThread(&CANTX_Thread,128,1); 
	
	OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
	
}

int main(void) {
	
	OS_Init();
	CAN_Init();
	
	NumCreated = 0;
	NumCreated += OS_AddThread(&IdleTask,128,7);  // runs when nothing useful to do
	NumCreated += OS_AddThread(&CANRX_Thread,128,1); 
	
	OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
	
	return 1;
	
}

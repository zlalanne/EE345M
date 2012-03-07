#include "OS.h"
#include "UART.h"
#include "ADC.h"
#include "Output.h"
#include "rit128x96x4.h"

#define TIMESLICE TIME_1MS*2

// TODO: Need to remove refrences to these eventually
unsigned long NumCreated;   // number of foreground threads created
unsigned long PIDWork;      // current number of PID calculations finished
unsigned long FilterWork;   // number of digital filter calculations finished
unsigned long NumSamples;   // incremented every sample
unsigned long DataLost;     // data sent by Producer, but not received by Consumer
short IntTerm;     // accumulated error, RPM-sec
short PrevError;   // previous error, RPM


//******** Producer *************** 
// The Producer in this lab will be called from your ADC ISR
// A timer runs at 1 kHz, started by your ADC_Collect
// The timer triggers the ADC, creating the 1 kHz sampling
// Your ADC ISR runs when ADC data is ready
// Your ADC ISR calls this function with a 10-bit sample 
// sends data to the consumer, runs periodically at 1 kHz
// inputs:  none
// outputs: none
void Producer(unsigned short data){  
  if(OS_Fifo_Put(data) == 0){ // send to consumer
    DataLost++;
  } 
}

void Display(void) {

  unsigned short data;

  Output_Init();
  Output_On();

  // Start periodic voltage sampling
  ADC_Collect(0, 1000, &Producer); 
 
  RIT128x96x4PlotClear(0, 30, 6, 12, 18, 24);  // range from 0 to 1023

  while(1) {
	data = OS_Fifo_Get();
    RIT128x96x4PlotPoint(data); 
    RIT128x96x4PlotNext(); // called 108 times
    RIT128x96x4ShowPlot();
  }
}

// Low priority, never blocks or sleeps
void Dummy(void){
  int dummy = 0;
  while(1){
    dummy++;
  }
}

int main(void){
  
  OS_Init();           // initialize, disable interrupts

  OS_MailBox_Init();
  OS_Fifo_Init(32);

  NumCreated = 0 ;
  
  // Create initial foreground threads
  NumCreated += OS_AddThread(&Interpreter,128,2); 
  NumCreated += OS_AddThread(&Display,128,1);

  // Low priority thread, never sleeps or blocks
  NumCreated += OS_AddThread(&Dummy,128,8);
 
  OS_Launch(TIMESLICE);
  return 0;
}


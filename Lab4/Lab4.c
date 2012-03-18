#include <stdio.h>
#include <math.h>

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


const long h[64]={489,-714,171,-1027,137,-769,446,-512,446,-769,137,-1027,171, 
     -714,489,-300,237,-414,-667,-564,-1158,-18,-767,512,-383,-256,-222, 
     -1711,1491,-1939,5655,-1748,8755,-5827,5275,-17177,-5027,-31022,-14434,126464,-14434, 
     -31022,-5027,-17177,5275,-5827,8755,-1748,5655,-1939,1491,-1711,-222,-256,-383, 
     512,-767,-18,-1158,-564,-667,-414,237,-300};


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
  if(OS_Fifo_Put(data) == 0){ // Send to consumer
    DataLost++;
  } 
}

//******** DisplayVoltage *************** 
// Foreground thread, accepts data from consumer
// Displays calculated results on the LCD
// inputs:  none                            
// outputs: none
void DisplayVoltage(void) {

  unsigned long data;
  unsigned long voltage;
  char buffer[30];
  

  Output_Init();
  Output_On();

  // Plot shows between 0.0V and 3.0V
  RIT128x96x4PlotClear(0, 30, 0, 10, 20, 30);

  while(1) {

    data = OS_MailBox_Recv();

	// Calculate voltage
    voltage = (3000 * data)/1024;
	snprintf(buffer, 30, "Voltage: %d mV         ", voltage);

    voltage = voltage / 100;

	// Plot voltage
	RIT128x96x4PlotPoint(voltage); 
    RIT128x96x4PlotNext();
    RIT128x96x4ShowPlot();
	RIT128x96x4StringDraw(buffer, 0, 0, 15);
  }
}

//******** DisplayFrequency *************** 
// Foreground thread, accepts data from consumer
// Displays calculated results on the LCD
// inputs:  none                            
// outputs: none
void DisplayFrequency(void) {

  unsigned long data;
  unsigned long frequency;
  signed short real, imag;
  int i;

  Output_Init();
  Output_On();

  while(1) {

    // Plot shows between 0.0V and 3.0V
    RIT128x96x4PlotClear(0, 30, 0, 10, 20, 30);

    for(i = 0; i < 64; i++) {
	  data = OS_MailBox_Recv();
	  real = data & 0xFFFF;
	  imag = data >> 16;
	   
	  RIT128x96x4PlotdBfs(sqrt(real*real +imag*imag));
	  RIT128x96x4PlotNext();

	}

	RIT128x96x4ShowPlot();
  }
}

//******** Filter_Init *************** 
// Initialize filter
// Input: None
// Output: None
long Data[128];
long *DataPt;
void Filter_Init(void) {
  DataPt = &Data[0];
}

//******** Filter_Calc *************** 
// Calculate one filter output
// called at sampling rate
// Input: New ADC data
// Output: Filter output
long Filter_Calc(short newdata) {
  int i;
  long sum;
  long *calcPt;
  const long *hPt;

  // Checking bounds
  if(DataPt == &Data[0]) {
    DataPt = &Data[63];
  } else {
    DataPt--;
  }

  *DataPt = *(DataPt + 64) = newdata; // Storing two copies

  calcPt = DataPt; // Copy of data pointer for calculations
  hPt = h; // Coefficent pointer

  // Performing filter
  sum = 0;
  for(i = 0; i < 64; i++) {
    sum += (*calcPt)*(*hPt);
	calcPt++;
	hPt++;
  }

  sum = sum / 256;

  return sum;
}

//******** Consumer *************** 
// Foreground thread, accepts data from producer
// Performs averaging filter
// inputs:  none
// outputs: none
char DigFiltEn = FALSE; // Enables the digital filter
void cr4_fft_64_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);
void ConsumerFrequency(void){

  unsigned long data[64];
  unsigned long result[64];
  int i = 0;

  Filter_Init();
  // Start ADC sampling, channel 0
  ADC_Collect(0,500, &Producer); 
  NumCreated += OS_AddThread(&DisplayFrequency,128,0); 
  
  while(1) {
    
	for(i = 0; i < 64; i++) {
	  data[i] = OS_Fifo_Get();
	}

    cr4_fft_64_stm32(result,data,64);

	for(i = 0; i < 64; i++) {
	  OS_MailBox_Send(result[i]);
	}

  }
}

//******** Consumer *************** 
// Foreground thread, accepts data from producer
// Performs averaging filter
// inputs:  none
// outputs: none
void ConsumerVoltage(void){

  unsigned long data;
  unsigned short result;

  Filter_Init();
  // Start ADC sampling, channel 0, 1000 Hz
  ADC_Collect(0,1000, &Producer); 
  NumCreated += OS_AddThread(&DisplayVoltage,128,0); 
  
  while(1) {  
	data = OS_Fifo_Get();
    result = Filter_Calc(data);
	
	if(DigFiltEn == FALSE) {
	  OS_MailBox_Send(data);
	} else {
	  OS_MailBox_Send(result);
	}
  }
}

// Low priority, never blocks or sleeps
void Dummy(void){
  int dummy = 0;
  while(1){
    dummy++;
  }
}

void DownButtonTask(void) {
  //NumCreated += OS_AddThread
}


int main(void){
  
  OS_Init();           // initialize, disable interrupts

  OS_MailBox_Init();
  OS_Fifo_Init(32);

  NumCreated = 0 ;
  
  // Create initial foreground threads
  NumCreated += OS_AddThread(&Interpreter,128,2); 
  NumCreated += OS_AddThread(&ConsumerVoltage,128,1);

  // Low priority thread, never sleeps or blocks
  NumCreated += OS_AddThread(&Dummy,128,8);

  // Creating down button task
  OS_AddDownTask(&DownButtonTask, 1);
 
  OS_Launch(TIMESLICE);
  return 0;
}


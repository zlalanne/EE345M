#include <stdio.h>
#include <math.h>

#include "OS.h"
#include "UART.h"
#include "ADC.h"
#include "Output.h"
#include "rit128x96x4.h"

#define TIMESLICE TIME_1MS*2
#define SAMPLESIZE 1024

#define FIRLENGTH 51
#define FILTERARRAYLENGTH FIRLENGTH*2

#define FREQ 0
#define TIME 1

							 
// TODO: Need to remove refrences to these eventually
unsigned long NumCreated;   // number of foreground threads created
unsigned long PIDWork;      // current number of PID calculations finished
unsigned long FilterWork;   // number of digital filter calculations finished
unsigned long NumSamples;   // incremented every sample
unsigned long DataLost;     // data sent by Producer, but not received by Consumer
short IntTerm;     // accumulated error, RPM-sec
short PrevError;   // previous error, RPM

// Arrays used to store pre/post digital filter and post FFT
unsigned long PreFilter[SAMPLESIZE] = {0,};
unsigned long PostFilter[SAMPLESIZE] = {0,};
unsigned long PostFFT[SAMPLESIZE] = {0,};
char DigFiltEn = FALSE;

// Variables used for oLED Display
char PlotMode = TIME;
char Freeze = FALSE;
Sema4Type SampleDone;

// Variables used to perform digital filter
long Data[FILTERARRAYLENGTH];
int FilterIndex = 0;

// FFT
void cr4_fft_64_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);
void cr4_fft_1024_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);

const long h[51]={0,-7,-45,-64,5,78,-46,-355,-482,-138,329,
     177,-722,-1388,-767,697,1115,-628,-2923,-2642,1025,4348,1820,-8027,-19790,
    56862,-19790,-8027,1820,4348,1025,-2642,-2923,-628,1115,697,-767,-1388,-722,
 177,329,-138,-482,-355,-46,78,5,-64,-45,-7,0};

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

//******** Display *************** 
// Foreground thread, accepts data from consumer
// Displays calculated results on the LCD
// inputs:  none                            
// outputs: none

unsigned long DebugMags[SAMPLESIZE / 2];
void Display(void) {

  long status;
  unsigned long voltage, mag;
  long real, imag;
  int i;
  char buffer[30];
  

  Output_Init();
  Output_On();

  while(1) {

    OS_bWait(&SampleDone);
  
    if(PlotMode == FREQ) {

	  // Perform FFT
      if(DigFiltEn == TRUE) {
        cr4_fft_1024_stm32(PostFFT,PostFilter,1024);
      } else {
        cr4_fft_1024_stm32(PostFFT,PreFilter,1024);
      }
	  
	  RIT128x96x4PlotClearFreq();

	  for(i = 0; i < SAMPLESIZE / 2; i++) {
	    real = PostFFT[i] & 0xFFFF;
		imag = PostFFT[i] >> 16;
		mag = sqrt(real*real + imag*imag);

		if(DigFiltEn == TRUE) {
		  // Max mag = 5*1024 = 5120, can only map to 512 
		  mag = mag / 10;
		} else {
		  // Max mag = 1024, can only map to 512
		  mag = mag / 2;
		}
		
		RIT128x96x4PlotdBfs(mag);
		DebugMags[i] = mag;

        if((i%4) == 3){
		  RIT128x96x4PlotNext();
		}

	  }
      
	  // Check if still in frequency mode and plot
	  if(PlotMode == FREQ) {
		status = StartCritical();
        RIT128x96x4StringDraw("FFT   fs = 10k       ", 0, 0, 15);	    
	    RIT128x96x4ShowPlotFreq();
		EndCritical(status);
	  }
	
	} else {

	  
	  if(DigFiltEn == TRUE) {
        // Range is from 0 to 15V because max input is 3V and max gain is 5
		RIT128x96x4PlotClear(0, 15, 0, 5, 10, 15);
	  } else {
	    // Range is from 0 to 3.0V because max input is 3.0V 
		RIT128x96x4PlotClear(0, 30, 0, 10, 20, 30);
	  }

	  // TODO: Might want to change this, we have 1024 voltage samples but only
	  // can display 128 samples across the screen
	  for(i = 0; i < SAMPLESIZE / 8; i++) {
	
	    if(DigFiltEn == TRUE) {
		  voltage = PostFilter[i];
		} else {
		  voltage = PreFilter[i];
		}

		// Calculate voltage
		voltage = (3000 * voltage)/1024;
	    snprintf(buffer, 30, "Voltage: %d mV         ", voltage);

		if(DigFiltEn == TRUE) {
		  voltage = voltage / 1000;
		} else {
		  voltage = voltage / 100;
		}

		RIT128x96x4PlotPoint(voltage);
		RIT128x96x4PlotNext();
	  }

	  // Check if still in time mode and plot
	  if(PlotMode == TIME) {
	    status = StartCritical();
        RIT128x96x4StringDraw(buffer, 0, 0, 15);	    
	    RIT128x96x4ShowPlot();
		EndCritical(status);
	  }

	}

	// Freeze current picture until select button pressed
	while(Freeze){}

  }
}

//******** Filter_Init *************** 
// Initialize filter
// Input: None
// Output: None
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
    DataPt = &Data[50];
  } else {
    DataPt--;
  }

  *DataPt = *(DataPt + 51) = newdata; // Storing two copies

  calcPt = DataPt; // Copy of data pointer for calculations
  hPt = h; // Coefficent pointer

  // Performing filter
  sum = 0;

  for(i = 51; i ; i--) {
    sum = sum + (*calcPt)*(*hPt);
	calcPt++;
	hPt++;
  }

  sum = sum / 16384;

  return sum;
}

//******** Consumer *************** 
// Foreground thread, accepts data from producer
// Performs averaging filter
// inputs:  none
// outputs: none
void Consumer(void){

  unsigned long data;

  Filter_Init();

  // Start ADC sampling, channel 0, 1000 Hz
  ADC_Collect(0,10000, &Producer); 
  NumCreated += OS_AddThread(&Display,128,0);
  OS_InitSemaphore(&SampleDone, 0);
  
  while(1) {
    
	data = OS_Fifo_Get();

    PreFilter[FilterIndex] = data;
    PostFilter[FilterIndex] = Filter_Calc(data);
    FilterIndex++;
	
	// Wrap if end of sampling
	if(FilterIndex == SAMPLESIZE) {
	  FilterIndex = 0;
	  OS_bSignal(&SampleDone);
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

void ChangeDisplay(void) {
  PlotMode ^= 0x01;
  RIT128x96x4Clear();
}

void ChangeMode(void) {
  Freeze ^= 0x01;
}
  

int main(void){
  
  OS_Init();           // initialize, disable interrupts

  OS_MailBox_Init();
  OS_Fifo_Init(32);

  NumCreated = 0 ;
  
  // Create initial foreground threads
  NumCreated += OS_AddThread(&Interpreter,128,2); 
  NumCreated += OS_AddThread(&Consumer,128,1);


  // Low priority thread, never sleeps or blocks
  NumCreated += OS_AddThread(&Dummy,128,8);

  // Creating button tasks
  OS_AddDownTask(&ChangeDisplay, 4);
  OS_AddButtonTask(&ChangeMode, 3);
 
  OS_Launch(TIMESLICE);
  return 0;
}


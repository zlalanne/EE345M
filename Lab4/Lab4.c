#include <stdio.h>

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
char DigFiltEn = TRUE;

// Variables used for oLED Display
char PlotMode = TIME;
char Freeze = FALSE;
Sema4Type SampleDone;
Sema4Type FFTDone;

// Variables used to perform digital filter
long Data[FILTERARRAYLENGTH];
int FilterIndex = 0;

// FFT
void cr4_fft_64_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);
void cr4_fft_1024_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);

const long h[51]={4,-2,-5,-3,2,1,-3,-7,-8,-6,-5,
     -5,-8,-15,-19,-13,-3,-7,-27,-39,-19,17,12,-66,-175,
     798,-175,-66,12,17,-19,-39,-27,-7,-3,-13,-19,-15,-8,
     -5,-5,-6,-8,-7,-3,1,2,-3,-5,-2,4};

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
  if(FFTDone.Value == 0) { 
    if(OS_Fifo_Put(data) == 0){ // Send to consumer
      DataLost++;
    }
  } 
}

// Newton's method
// s is an integer
// sqrt(s) is an integer
unsigned long sqrt(unsigned long s){
unsigned long t;         // t*t will become s
int n;                   // loop counter to make sure it stops running
  t = s/10+1;            // initial guess 
  for(n = 16; n; --n){   // guaranteed to finish
    t = ((t*t+s)/t)/2;  
  }
  return t; 
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

    OS_bWait(&SampleDone); // Wait until sampling done

    if(PlotMode == FREQ) {

	  // Perform FFT
      if(DigFiltEn == TRUE) {
        cr4_fft_1024_stm32(PostFFT,PreFilter,1024);
      } else {
        cr4_fft_1024_stm32(PostFFT,PreFilter,1024);
      }
 
	  RIT128x96x4PlotClearFreq();

	  for(i = 0; i < SAMPLESIZE / 2; i++) {
	    real = PostFFT[i] & 0xFFFF;
		imag = PostFFT[i] >> 16;
		mag = sqrt(real*real + imag*imag);

		if(DigFiltEn == TRUE) {
		  // Max mag = 2*1024 = 2048, can only map to 512 
		  mag = mag / 4;
		} else {
		  // Max mag = 1024, can only map to 512
		  mag = mag / 2;
		}
		
		RIT128x96x4PlotdBfs(mag);
		DebugMags[i] = mag;

        if((i%4) == 3){
		  RIT128x96x4PlotNextFreq();
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
		RIT128x96x4PlotClear(0, 60, 0, 20, 40, 60);
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

    OS_bSignal(&FFTDone); // Signal FFT Done

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
/*
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
}							   */


long DebugFilter[200];
int DebugFilterIndex = 0;

static unsigned int n = 50;

unsigned long Filter_Calc(unsigned short newdata) {
  static unsigned long x[102] = {0,};
  int k;
  long y;
  
  n++;
  if(n==102) n = 51;

  x[n]=x[n-51]=newdata;

  y=0;

  for(k=0;k<51;k++){
    y = y + h[k]*x[n-k];
  }

  y = y/512;

  //if(y < 0) y = 0;

  y = y+600;
  if(DebugFilterIndex < 200) {
    DebugFilter[DebugFilterIndex] = y;
	DebugFilterIndex++;
  }

  return y;
}

//******** Consumer *************** 
// Foreground thread, accepts data from producer
// Performs averaging filter
// inputs:  none
// outputs: none
void Consumer(void){
  int i;
  unsigned long data;
  long status;
  unsigned long voltage, mag;
  unsigned long real, imag;
  char buffer[30];
  

  Output_Init();
  Output_On();
  OS_Fifo_Init(64);
  Filter_Init();

  // Start ADC sampling, channel 0, 1000 Hz
  ADC_Collect(0,10000, &Producer); 
  //NumCreated += OS_AddThread(&Display,128,0);
  //OS_InitSemaphore(&SampleDone, 0);
  //OS_InitSemaphore(&FFTDone, 0);
  
  while(1) {
    
	for(i = 0; i < SAMPLESIZE; i++) {
	  data = OS_Fifo_Get();

      PreFilter[i] = data;
      PostFilter[i] = Filter_Calc(data);
    }

	if(PlotMode == FREQ) {

	  // Perform FFT
      if(DigFiltEn == TRUE) {
        cr4_fft_1024_stm32(PostFFT,PreFilter,1024);
      } else {
        cr4_fft_1024_stm32(PostFFT,PreFilter,1024);
      }
 
	  RIT128x96x4PlotClearFreq();

	  for(i = 0; i < 512; i++) {
	    real = PostFFT[i] & 0xFFFF;
		imag = PostFFT[i] >> 16;
		mag = sqrt(real*real + imag*imag);

		if(DigFiltEn == TRUE) {
		  // Max mag = 5*1024 = 5120, can only map to 512 
		  mag = mag / 4;
		} else {
		  // Max mag = 1024, can only map to 512
		  mag = mag / 2;
		}
		
		RIT128x96x4PlotdBfs(mag);
		DebugMags[i] = mag;

        if((i%4) == 3){
		  RIT128x96x4PlotNextFreq();
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
		RIT128x96x4PlotClear(0, 60, 0, 20, 40, 60);
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
		  voltage = voltage / 100;
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


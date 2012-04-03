// Modified By:
// Thomas Brezinski	TCB567
// Zachary Lalanne ZLL67
// TA: Zahidul Haq
// Date of last change: 2/24/2012

// Written By:
// Megan Ruthven MAR3939
// Zachary Lalanne ZLL67
// TA: NACHI
// Date of last change: 10/17/2011

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/adc.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

#include "ADC.h"

unsigned long *Buffer;

ADCTask SS0Task;
ADCTask SS1Tasks[4];
int NumActiveChannels;

void ADC0_Handler(void);

void ADC_Open(void){

  // The ADC0 peripheral must be enabled for use.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
}

unsigned long ADC_In(unsigned int channelNum){

  unsigned long config;
  unsigned long data;

  // Configuring ADC to start by processor call instead of interrupt
  ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);

  // Determine input channel
  switch(channelNum){
    case 0: config = ADC_CTL_CH0; break;
		case 1: config = ADC_CTL_CH1; break;
		case 2: config = ADC_CTL_CH2; break;
		case 3: config = ADC_CTL_CH3; break;
  } 

  // Enable ADC interrupt and last step of sequence
  config |= ADC_CTL_IE | ADC_CTL_END;
  ADCSequenceStepConfigure(ADC0_BASE, 3, 0, config);

  ADCSequenceEnable(ADC0_BASE, 3);
  ADCIntClear(ADC0_BASE, 3);

  // Start ADC conversion
  ADCProcessorTrigger(ADC0_BASE, 3);

  // Wait for ADC conversion to finish
  while(!ADCIntStatus(ADC0_BASE, 3, false)){}

  // Clear interrupt flag and read conversion data
  ADCIntClear(ADC0_BASE, 3);
  ADCSequenceDataGet(ADC0_BASE, 3, &data);

  return data;
}

int ADC_Collect(unsigned int channelNum, unsigned int fs, 
  void (*task)(unsigned short)){

  unsigned long config;

  SS0Task = task;

  // Determine input channel
  switch(channelNum){
    case 0: config = ADC_CTL_CH0; break;
	case 1: config = ADC_CTL_CH1; break;
	case 2: config = ADC_CTL_CH2; break;
	case 3: config = ADC_CTL_CH3; break;
  }

  ADCSequenceDisable(ADC0_BASE, 0);

  // Enable the ADC0 for interrupt Sequence 0 with lower priority then single shot
  ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_TIMER, 1);

  // Configuring steps of sequence, last step contains ADC_CTL_END and ADC_CTL_IE config paramter
  config |= ADC_CTL_END | ADC_CTL_IE;
  ADCSequenceStepConfigure(ADC0_BASE, 0, 0, config);

  // Disabling Timer0A for configuration
  TimerDisable(TIMER0_BASE, TIMER_A);

  // Configure as 16 bit timer and trigger ADC conversion
  TimerControlTrigger(TIMER0_BASE, TIMER_A, true);

  TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet()/ fs);
  TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

  ADCSequenceOverflowClear(ADC0_BASE, 0);
  ADCSequenceUnderflowClear(ADC0_BASE, 0);
  ADCIntClear(ADC0_BASE, 0);
  ADCSequenceEnable(ADC0_BASE, 0);
 
  TimerEnable(TIMER0_BASE, TIMER_A);
  TimerIntEnable(TIMER0_BASE, TIMER_A);
  ADCIntEnable(ADC0_BASE, 0);

  IntEnable(INT_ADC0SS0);
  
  return 1; 
}

// Sets up an adc collection sequence on channels 0-(numChannels-1)
// tasks is an array of function pointers (one for each channel num)
int ADC_CollectSequence(unsigned short numChannels, unsigned int fs,
  ADCTask* tasks) {
	int channelNum;
	unsigned long config;
	NumActiveChannels = numChannels;

  ADCSequenceDisable(ADC0_BASE, 1);

	// Enable the ADC0 for interrupt Sequence 0 with lower priority then single shot
  ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_TIMER, 1);

	// loop through the channels from 0 to numChannels-1, configuring each channel as a step
	// of the sequence and setting the respective task to handle the result
	for (channelNum = 0; channelNum < numChannels; channelNum++) {
		SS1Tasks[channelNum] = tasks[channelNum];
	  
		switch(channelNum){
      case 0: config = ADC_CTL_CH0; break;
	    case 1: config = ADC_CTL_CH1; break;
	    case 2: config = ADC_CTL_CH2; break;
	    case 3: config = ADC_CTL_CH3; break;
    }
		// Configuring steps of sequence, last step contains ADC_CTL_END and ADC_CTL_IE config paramter
    if (channelNum == numChannels-1) {
		  config |= ADC_CTL_END | ADC_CTL_IE;
		}
		ADCSequenceStepConfigure(ADC0_BASE, 1, channelNum, config);
	}

  // Disabling Timer0A for configuration
  TimerDisable(TIMER0_BASE, TIMER_A);

  // Configure as 16 bit timer and trigger ADC conversion
  TimerControlTrigger(TIMER0_BASE, TIMER_A, true);

  TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet()/ fs);
  TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

  ADCSequenceOverflowClear(ADC0_BASE, 1);
  ADCSequenceUnderflowClear(ADC0_BASE, 1);
  ADCIntClear(ADC0_BASE, 1);
  ADCSequenceEnable(ADC0_BASE, 1);
 
  TimerEnable(TIMER0_BASE, TIMER_A);
  TimerIntEnable(TIMER0_BASE, TIMER_A);
  ADCIntEnable(ADC0_BASE, 1);

  IntEnable(INT_ADC0SS1);
  
  return 1; 
}

void ADC0S0_Handler(void){
  
  unsigned long data[8];
  unsigned short average = 0;
  char samples;
  int i;

  // Clear flag
  ADCIntClear(ADC0_BASE, 0);
  samples = ADCSequenceDataGet(ADC0_BASE, 0, data);
  
  // Get average of samples from FIFO
  for(i = 0; i < samples; i++) {
    average = average + data[i];
  }

  average = average / samples; 
  SS0Task(average);
}

void ADC0S1_Handler(void){
  unsigned long data[4];
  char samples;
  int seqStep;

  // Clear flag
  ADCIntClear(ADC0_BASE, 1);
  samples = ADCSequenceDataGet(ADC0_BASE, 1, data);
  
  // Handle samples from FIFO
  for(seqStep = 0; seqStep < samples; seqStep++) {
    (SS1Tasks[seqStep])(data[seqStep]);
  }
}


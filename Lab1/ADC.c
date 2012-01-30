// Modified By:
// Thomas Brezinski
// Zachary Lalanne ZLL67
// TA:
// Date of last change: 1/25/2012

// Written By:
// Megan Ruthven MAR3939
// Zachary Lalanne ZLL67
// TA: NACHI
// Date of last change: 10/17/2011

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/adc.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"

#include "ADC.h"

void ADC0_Handler(void) {
	ADCIntClear(ADC0_BASE, 3);
	ADCSequenceDataGet(ADC0_BASE, 3, Buffer);
}

unsigned long * Buffer;
unsigned char Status = FALSE;

void ADC_Open(void){

  // The ADC0 & ADC1 peripheral must be enabled for use.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
  // Do we need to enable the port that ADC0 is being used with as well?
  // SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
  // GPIOinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_7);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

}

unsigned long ADC_In(unsigned int channelNum){

  unsigned long config;
  unsigned long data;

  // Configuring ADC to start by processor call instead of interrupt
  ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);

  // Determine input channel
  switch(channelNum){
    case 0: config = ADC_CTL_0CH0; break;
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
  unsigned long buffer[], unsigned int numberOfSamples){

  int i;
  unsigned long config;

  // Setting global pointer to point at array passed by funciton
  Buffer = buffer;

  // Determine input channel
  switch(channelNum){
    case 0: config = ADC_CTL_CH0; break;
	case 1: config = ADC_CTL_CH1; break;
	case 2: config = ADC_CTL_CH2; break;
	case 3: config = ADC_CTL_CH3; break;
  }

  // Enable the ADC0 for interrupt Sequence 0 with lower priority then single shot
  ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_TIMER, 1);

  // Configuring steps of sequence, last step contains ADC_CTL_END and ADC_CTL_IE config paramter
  for(i = 0; i < (numberOfSamples - 1); i++){
    ADCSequenceStepConfigure(ADC0_BASE, 0, i, config);
  }
  ADCSequenceStepConfigure(ADC0_BASE, 0, numberOfSamples - 1, config | ADC_CTL_END | ADC_CTL_IE);
  ADCIntRegister(ADC0_BASE, 0, ADC0_Handler);
  ADCSequenceEnable(ADC0_BASE, 0);

  // Disabling Timer0A for configuration
  TimerDisable(TIMER0_BASE, TIMER_A);

  // Configure as 16 bit timer and trigger ADC conversion
  TimerConfigure(TIMER0_BASE, TIMER_CFG_16_BIT_PAIR | TIMER_CFG_A_PERIODIC);
  TimerControlTrigger(TIMER0_BASE, TIMER_A, true);

  //
  //
  // TODO: configure this to calculate load value based on frequency inputed
  //
  //
  TimerLoadSet(TIMER0_BASE, TIMER_A, 1000);
  TimerEnable(TIMER0_BASE, TIMER_A);

  ADCIntClear(ADC0_BASE, 0);
  TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

  ADCIntEnable(ADC0_BASE, 0);
  TimerIntEnable(TIMER0_BASE, TIMER_A);

  // Claering Status flag
  Status = FALSE;
  
  return 1; 
}

int ADC_Status(void){
  if(Status == TRUE){
    Status = FALSE;
	return TRUE;
  } else {
    return FALSE;
  }
};
    
void ADC0_Handler(void){

  // Clear flag
  ADCIntClear(ADC0_BASE, 0);
  
  ADCSequenceDataGet(ADC0_BASE, 0, Buffer);
	
  Status = TRUE;
}


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

#define TRUE 1
#define FALSE 0

char MailFlag = FALSE;
unsigned long LastConversion = 0;
unsigned char LastIndex = 0;

void ADC3_Handler(void);

void ADC_Open(void){

  // The ADC0 & ADC1 peripheral must be enabled for use.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);

}

unsigned long ADC_In(int channel){
  
  unsigned long config;
  unsigned long data;

  // Configuring ADC to start by processor call instead of interrupt
  ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);

  // Determine input channel
  switch(channel){
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

void ADC3_Handler(void) {

  // Clear flag
  ADCIntClear(ADC0_BASE, 3);
  
  // Get ADC value, and store in LastConversion
  ADCSequenceDataGet(ADC0_BASE, 3, &LastConversion);

  // Indicate new ADC value
  MailFlag = TRUE;
}


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

void ADC_Init(void) {

  // The ADC0 peripheral must be enabled for use.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

  // Enable the ADC0 for interrupt Sequence 3
  ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_TIMER, 0);
  ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
  ADCSequenceEnable(ADC0_BASE, 3);
  ADCIntClear(ADC0_BASE, 3);
  ADCIntRegister(ADC0_BASE, 3, ADC3_Handler);
  ADCIntEnable(ADC0_BASE, 3);

  // Setting Timer0 as a peripheral
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

  // Disabling Timers during configuration
  TimerDisable(TIMER0_BASE, TIMER_A);

  // Configuring prescale
  TimerPrescaleSet(TIMER0_BASE, TIMER_A, 125);

  // Loading count time for timer
  TimerLoadSet(TIMER0_BASE, TIMER_A, 5600);

  // Enabling timers
  TimerEnable(TIMER0_BASE, TIMER_A);
  TimerIntEnable(TIMER0_BASE, TIMER_A); 

}

unsigned char Get_ADC_Data(unsigned short *temperature, unsigned long *resistance) {



  // Checking if new data in mailbox
  if(MailFlag == FALSE) {
    return FALSE;
  } else {
	// Transverse through the ADCdata array
	/*for(i = 1; i < 1024; i++) { 
	  // If you find that the last conversion, is less than the 
	  // index of the array, then store the index before it
	  if(LastConversion < ADCdata[i]) {
	    LastIndex = (i - 1);
	    i = 1024;
	  }
	} */


	// Set all passed refferences to the corresponding
	// array at index LastIndex
	//*temperature = Tdata[LastIndex];
	//*resistance = Rdata[LastIndex];
	
	// Set mail flag to false
	MailFlag = FALSE;

	// Return true
	return TRUE;
  }
}

void ADC3_Handler(void) {

  // Clear flag
  ADCIntClear(ADC0_BASE, 3);
  
  // Get ADC value, and store in LastConversion
  ADCSequenceDataGet(ADC0_BASE, 3, &LastConversion);

  // Indicate new ADC value
  MailFlag = TRUE;
}


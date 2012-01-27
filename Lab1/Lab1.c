// Written By:
// Thomas Brezinski
// Zachary Lalanne ZLL67
// TA:
// Date of last change: 1/24/2012

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/sysctl.h"

#include "Output.h"
#include "rit128x96x4.h"
#include "ADC.h"


// Functions from startup.s
void EnableInterrupts(void);
void DisableInterrupts(void);
long StartCritical(void);
void EndCritical(long st);


int main(void){
  
   unsigned long data;
   unsigned long buffer[4];
  // Setting the clock to 50 MHz
  SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                  SYSCTL_XTAL_8MHZ);       

  Output_Init();
  ADC_Open();
  EnableInterrupts();
  			 
  ADC_Collect(0, 12000, buffer, 4);
  for(;;){

  while(ADC_Status == FALSE){};
  oLED_Message(0, 0, "Chan0 Sam1", buffer[0]);
  oLED_Message(0, 1, "Chan0 Sam2", buffer[1]);
  oLED_Message(0, 2, "Chan0 Sam3", buffer[2]);
  oLED_Message(0, 3, "Chan0 Sam4", buffer[3]);

	
  data = ADC_In(0);
  oLED_Message(1, 0, "Chan0", data);
  data = ADC_In(1);
  oLED_Message(1, 1, "Chan1", data);
  data = ADC_In(2);
  oLED_Message(1, 2, "Chan2", data);
  data = ADC_In(3);
  oLED_Message(1, 3, "Chan3", data);
  }

}

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
  // Setting the clock to 50 MHz
  SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                  SYSCTL_XTAL_8MHZ);       

  Output_Init();
  EnableInterrupts();
  ADC_Open();


  oLED_Message(0, 0, "Device 0, Line 0", 1);
  oLED_Message(0, 1, "Device 0, Line 1", 20);
  oLED_Message(0, 2, "Device 0, Line 2", 300);
  oLED_Message(0, 3, "Device 0, Line 3", 4000);
  


  for(;;){

  data = ADC_In(0);
  oLED_Message(1, 0, "Channel 0", data);
  data = ADC_In(1);
  oLED_Message(1, 1, "Channel 1", data);
  data = ADC_In(2);
  oLED_Message(1, 2, "Channel 2", data);
  data = ADC_In(3);
  oLED_Message(1, 3, "Channel 3", data);

  }

}

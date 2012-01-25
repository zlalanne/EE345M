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


// Functions from startup.s
void EnableInterrupts(void);
void DisableInterrupts(void);
long StartCritical(void);
void EndCritical(long st);


int main(void){
  
  // Setting the clock to 50 MHz
  SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                  SYSCTL_XTAL_8MHZ);       

  Output_Init();
  // Enabling interrupts
  EnableInterrupts();

  oLED_Message(0, 0, "Device 0, Line 0", 15);
  oLED_Message(0, 1, "Device 0, Line 1", 15);
  oLED_Message(0, 2, "Device 0, Line 2", 15);
  oLED_Message(0, 3, "Device 0, Line 3", 15);
  
  oLED_Message(1, 0, "Device 1, Line 0", 15);
  oLED_Message(1, 1, "Device 1, Line 1", 15);
  oLED_Message(1, 2, "Device 1, Line 2", 15);
  oLED_Message(1, 3, "Device 1, Line 3", 15);

  for(;;){

  }

}

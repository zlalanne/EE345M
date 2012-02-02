// Written By:
// Thomas Brezinski
// Zachary Lalanne ZLL67
// TA:
// Date of last change: 2/1/2012

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/sysctl.h"

#include "Output.h"
#include "rit128x96x4.h"
#include "ADC.h"
#include "UART.h"


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
  ADC_Open();
  UART0_Init();

  EnableInterrupts();

  for(;;){
	CMD_Run();
  }

}

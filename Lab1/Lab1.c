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
#include "OS.h"
#include "inc/lm3s8962.h"


// Functions from startup.s
void EnableInterrupts(void);
void DisableInterrupts(void);
long StartCritical(void);
void EndCritical(long st);

void DummyInit(void) {
	volatile unsigned long ulLoop;
	
	//
    // Enable the GPIO port that is used for the on-board LED.
    //
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;

	//
	// Do a dummy read to insert a few cycles after enabling the peripheral.
	//
	ulLoop = SYSCTL_RCGC2_R;


    //
    // Enable the GPIO pin for the LED (PF0).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    //
    GPIO_PORTF_DIR_R = 0x01;
    GPIO_PORTF_DEN_R = 0x01;
}

void Dummy(void) {
	GPIO_PORTF_DATA_R ^= 0x01;
}

int main(void){
  
   unsigned long data;
   unsigned long buffer[4];
  // Setting the clock to 50 MHz
   SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                  SYSCTL_XTAL_8MHZ);       
   DummyInit();
   Output_Init();
   
   ADC_Open();
   EnableInterrupts();
  			 
//  ADC_Collect(0, 12000, buffer, 4);
   oLED_Message(0,1, "************************", 0);
   OS_AddPeriodicThread(&Dummy, 50, 0);
   //oLED_Message(0,2, "Periodic Thread added", 0);

  for(;;){

/*  while(ADC_Status == FALSE){};
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
  oLED_Message(1, 3, "Chan3", data);  */

  }

}

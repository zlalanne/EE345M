// QRB1134.c

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"

unsigned long Period; 
unsigned long Last; // last edge time


void Tach_Init(void) { 
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_0);

    /* Setup Timer0A Counter in edge-time-capture mode.  */
    TimerDisable(TIMER0_BASE, TIMER_A);
    //TimerConfigure(TIMER0_BASE, TIMER_CFG_16_BIT_PAIR | TIMER_CFG_A_CAP_TIME | TIMER_CFG_B_PERIODIC);
	  TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT | TIMER_TIMA_TIMEOUT);

    TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);
    TimerLoadSet(TIMER0_BASE, TIMER_A, 0xFFFF);

    IntEnable(INT_TIMER0A);       // Activate timer
    TimerEnable(TIMER0_BASE, TIMER_A);
}

unsigned long Tach_GetPeriod(void) {
        return Period;
}

unsigned long Tach_GetRPS(void) {
        return SysCtlClockGet()/(Period*4);
}

unsigned long Tach_GetRPM(void) {
        return (SysCtlClockGet()*60)/(Period*4);
        
}

void Timer0A_Handler(void){
        static unsigned long Counter = 0;  // may interrupt from timeouts multiple times before edge
        unsigned long ulIntFlags = TimerIntStatus(TIMER0_BASE, true);
        TimerIntClear(TIMER0_BASE, ulIntFlags);// acknowledge
       if(ulIntFlags & TIMER_TIMA_TIMEOUT) {
                Counter++;	// keep track of timeouts
        }
        
        if(ulIntFlags & TIMER_CAPA_EVENT) {
                Period = Last - TimerValueGet(TIMER0_BASE, TIMER_A) + Counter*TimerLoadGet(TIMER0_BASE, TIMER_A);
                Last = TimerValueGet(TIMER0_BASE, TIMER_A);
                Counter = 0;
        }
}


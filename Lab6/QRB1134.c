// QRB1134.c

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

unsigned long Period; 
unsigned long Last; // last edge time

void Tach_Init(void) { 
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_0);
    GPIOPinConfigure(GPIO_PB0_CCP0);

    /* Setup Timer3A Counter in edge-time-capture mode.  */

    TimerDisable(TIMER3_BASE, TIMER_A);
    TimerConfigure(TIMERA_BASE, TIMER_CFG_16_BIT_PAIR | TIMER_CFG_A_CAP_TIME);
    TimerIntEnable(TIMER3_BASE, TIMER_CAPA_EVENT | TIMER_TIMA_TIMEOUT);

    TimerControlEvent(TIMER3_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);
    TimerLoadSet(TIMER3_BASE, TIMER_A, 0xFFFF);

    IntEnable(INT_TIMER3A);       // Activate timer
    TimerEnable(TIMER3_BASE, TIMER_A);
        IntMasterEnable();
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

void Timer0AHandler(void){
        static unsigned long Counter = 0;  // may interrupt from timeouts multiple times before edge
        unsigned long ulIntFlags = TimerIntStatus(TIMER3_BASE, true);
        TimerIntClear(TIMER3_BASE, ulIntFlags);// acknowledge
        
        if(ulIntFlags & TIMER_TIMA_TIMEOUT) {
                Counter++;	// keep track of timeouts
        }
        
        if(ulIntFlags & TIMER_CAPA_EVENT) {
                Period = Last - TimerValueGet(TIMER3_BASE, TIMER_A) + Counter*TimerLoadGet(TIMER3_BASE, TIMER_A);
                Last = TimerValueGet(TIMER0_BASE, TIMER_A);
                Counter = 0;
        }
}
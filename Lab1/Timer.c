// Megan Ruthven MAR3939
// Zachary Lalanne ZLL67
// TA: NACHI
// Date of last change: 10/3/2011
// Lab Assignment 5
// Purpose: Initialize timers, as well use interrupts
// HW Config: None

#include "inc/lm3s1968.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"


// Prototypes for the Timer handlers

void Timer0A_Handler(void);

void Timer_Init(void){					   

  // Setting the clock to 50 MHz
  SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                  SYSCTL_XTAL_8MHZ);       

  // Setting Timer0 as a peripheral
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

  // Disabling Timers during configuration
  TimerDisable(TIMER0_BASE, TIMER_A);
  
  //TimerConfigure(TIMER0_BASE, TIMER_CFG_16_BIT_PAIR | TIMER_CFG_A_PERIODIC);

  // Configuring prescale
  TimerPrescaleSet(TIMER0_BASE, TIMER_A, 125);

  // Loading count time for timer
  TimerLoadSet(TIMER0_BASE, TIMER_A, 5600);

  // Tie Timer0A to interrupt handler
  //TimerIntRegister(TIMER0_BASE, TIMER_A, Timer0A_Handler);

  // Enabling timers
  TimerEnable(TIMER0_BASE, TIMER_A);

  TimerIntEnable(TIMER0_BASE, TIMER_A); 
  
  return;
}
/*
void Timer0A_Handler(void){
  // Acknowledge TimerA interrupt
  TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
  return;
} */


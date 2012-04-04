// Ping.c
// Created By:
// Thomas Brezinski	TCB567
// Zachary Lalanne ZLL67
// Jeff Mahler
// Will Collins
// TA: Zahidul Haq
// Date of last change: 03/31/2012

#include "Ping.h"
#include "OS.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

#include "inc/lm3s8962.h"

#define GPIO_PORTB0				 (*((volatile unsigned long *)0x40005004))
#define GPIO_PORTB1				 (*((volatile unsigned long *)0x40005008))

#define TIME_1S 10000000					// in units of 100ns
#define SPEED_OF_SOUND 42000			// in centimeters / sec (CALIBRATED FROM DATA - NOT ACTUAL SPEED OF SOUND)
#define PING_OFFSET -14           // distance measured at 0 cm to sensor

unsigned long Sensor0Reading;
unsigned long Sensor1Reading;
unsigned long Sensor0StartTime;
unsigned long Sensor1StartTime;
Sema4Type Sensor0Free;
Sema4Type Sensor1Free;
Sema4Type Sensor0DataAvailable;
Sema4Type Sensor1DataAvailable;

void TriggerSensor0(void);
void TriggerSensor1(void);

void Ping_Init(void) {	volatile unsigned long delay;
  OS_DisableInterrupts();
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB; // activate port B
	delay = SYSCTL_RCGC2_R;
  // **** Port B Edge Trigger Initialization ****
	GPIO_PORTB_DIR_R |= 0x03;   // make PB0-1 out
  GPIO_PORTB_DEN_R |= 0x03;   // enable digital I/O on PB0-1
	GPIO_PORTB_PUR_R |= 0x03;		// enable pull up on PB0-1
	GPIO_PORTB_IEV_R |= 0x03;	  // make PB0-1 rising edge triggered
	GPIO_PORTB_IS_R &= ~0x03;		// make PB0-1 edge-sensitive
	GPIO_PORTB_ICR_R = 0x03;		// clear flag
	NVIC_PRI0_R = (NVIC_PRI1_R&0xFFFF00FF)|0x00006000;  // priority 3
	NVIC_EN0_R |= NVIC_EN0_INT1;

	OS_InitSemaphore(&Sensor0Free, 1);
	OS_InitSemaphore(&Sensor1Free, 1);
	OS_InitSemaphore(&Sensor0DataAvailable, 0);
	OS_InitSemaphore(&Sensor1DataAvailable, 0);

	OS_EnableInterrupts();
}

unsigned long Ping_GetDistance(int sensor) {
  unsigned long	time;
	unsigned long distance = 0;

	if (sensor == 0) {

	  OS_bWait(&Sensor0Free);
		TriggerSensor0();
		OS_bWait(&Sensor0DataAvailable);
		time = Sensor0Reading;
		distance = SPEED_OF_SOUND * time / (2 * TIME_1S) + PING_OFFSET;
		OS_bSignal(&Sensor0Free);

	} else if (sensor == 1) {																

	  OS_bWait(&Sensor1Free);
		TriggerSensor1();
		OS_bWait(&Sensor1DataAvailable);
		time = Sensor1Reading;
		distance = SPEED_OF_SOUND * time / (2 * TIME_1S) + PING_OFFSET;
		OS_bSignal(&Sensor1Free);

	}
	return distance;
}

void TriggerSensor0(void) {
  int i;
	OS_DisableInterrupts();
	GPIO_PORTB0 = 0x01;
	
	// delay 5 us
	for (i = 0; i < 15; i++);
	
	GPIO_PORTB0 = 0x00;
	GPIO_PORTB_DIR_R &= ~0x01;   // make PB0 in	
	GPIO_PORTB_IM_R |= 0x01;		// enable PB0 interrupts 
  OS_EnableInterrupts();
}

void TriggerSensor1(void) {
  int i;
	OS_DisableInterrupts();
	GPIO_PORTB1 = 0x02;
	
	// delay 5 us
	for (i = 0; i < 15; i++);
	
	GPIO_PORTB1 = 0x00;
	GPIO_PORTB_DIR_R &= ~0x02;   // make PB1 in	
	GPIO_PORTB_IM_R |= 0x02;		// enable PB1 interrupts 
  OS_EnableInterrupts();
}

void GPIOPortB_Handler(void) {
  // handle port B0 interrupt
  if ((GPIO_PORTB_MIS_R&0x01) == 0x01) {
    GPIO_PORTB_ICR_R = 0x01;

	  // handle rising edge
	  if ((GPIO_PORTB_IEV_R&0x01) == 0x01) {
		  Sensor0StartTime = OS_Time();
			GPIO_PORTB_IEV_R &= ~0x01;  // switch to falling edge 
		}  else {	 // handle falling edge
			Sensor0Reading = OS_TimeDifference(Sensor0StartTime, OS_Time());	
	    GPIO_PORTB_IM_R &= ~0x01;		 // disable PB0 interrupts
			GPIO_PORTB_IEV_R |= 0x01;    // switch to rising edge (for next time)
			GPIO_PORTB_DIR_R |= 0x01;    // make PB0 out (for next time)	
			OS_bSignal(&Sensor0DataAvailable); 
		}
	}

	// handle port B1 interrupt
	if ((GPIO_PORTB_MIS_R&0x02) == 0x02) {
    GPIO_PORTB_ICR_R = 0x02;

	  // handle rising edge
	  if ((GPIO_PORTB_IEV_R&0x02) == 0x02) {
		  Sensor1StartTime = OS_Time();
			GPIO_PORTB_IEV_R &= ~0x02;  // switch to falling edge 
		}  else {	 // handle falling edge
			Sensor1Reading = OS_TimeDifference(Sensor1StartTime, OS_Time());	
	    GPIO_PORTB_IM_R &= ~0x02;		// disable PB1 interrupts
			GPIO_PORTB_IEV_R |= 0x02;   // switch to rising edge (for next time)
			GPIO_PORTB_DIR_R |= 0x02;   // make PB1 out (for next time)	
			OS_bSignal(&Sensor1DataAvailable); 
		}
	}
}


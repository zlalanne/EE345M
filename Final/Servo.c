// Servo.c

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"

#define ZERO_POSITION 37500

//-------- Servo_Init -----------
// Initializes the PWM used to control the steering servo
// Inputs: none
// Outputs: none
void Servo_Init(void){
  SysCtlPWMClockSet(SYSCTL_PWMDIV_2);  // sets the divider from the processor clock, so for 8962 20ns period
  
  // Setting up PWM0
  SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_0);
	
  // Configuring for period, cycle period for the HS-300 is 20ms
  PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);  // center centered pulses
  PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, 63200); // 1000000 20ns ticks for 20ms servo period
	
  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, ZERO_POSITION);
	
  PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);
}

//------ Servo_Start --------------
// Starts the servo
// Inputs: none
// Outputs: none
void Servo_Start(void) {
	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
}

void Servo_Set_Position(unsigned long position) {
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, position);
}

unsigned long Servo_Pulse_Get(void) {
	return PWMPulseWidthGet(PWM0_BASE, PWM_OUT_0);
}

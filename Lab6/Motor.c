
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"

// Frequency of PWM in Hz
#define FREQUENCY 1500

//------------Motor_Init------------
// Initilizes PWM for motor interfacing
// Input: none
// Output: none
void Motor_Init(void) {
	
	unsigned long period;
	
	SysCtlPWMClockSet(SYSCTL_PWMDIV_1);
	
	// Setting up PWM0
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_0);
	
	// Getting period
	period = SysCtlClockGet() / FREQUENCY;
	
	// Configuring for 440Hz Period
	PWMGenConfigure(PWM0_BASE, PWM_GEN_0,
    PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, period);
	
	// Setting PWM0 to 25% duty cycle
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, period / 2);
	
	// Enabling PWM0 output 
	PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);
	
}

//------------Motor_Start------------
// Starts the motor
// Input: none
// Output: none
void Motor_Start(void) {
	// Enable the PWM generator.
  PWMGenEnable(PWM0_BASE, PWM_GEN_0);
}


int motortestmain(void) {
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
	Motor_Init();
	Motor_Start();
	
	while(1){
		
	}
}

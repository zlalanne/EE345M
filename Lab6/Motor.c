
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"

// Frequency of PWM in Hz
#define FREQUENCY 2000

unsigned long MotorPeriod;

//------------Motor_Init------------
// Initilizes PWM for motor interfacing
// Input: none
// Output: none
void Motor_Init(void) {
	
	
	
	SysCtlPWMClockSet(SYSCTL_PWMDIV_1);
	
	// Setting up PWM0
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_0);
	
	// Setting up PWM2
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_0);
	
	// Getting period
	MotorPeriod = SysCtlClockGet() / FREQUENCY;
	
	// Configuring for period
	PWMGenConfigure(PWM0_BASE, PWM_GEN_0,
    PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, MotorPeriod);
	
	PWMGenConfigure(PWM0_BASE, PWM_GEN_1,
    PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, MotorPeriod);
	
	// Setting to forward full speed
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, (31* MotorPeriod) / 32);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (31* MotorPeriod) / 32);
		
	// Enabling PWM0 and PWM2 output 
	PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);
	PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);

	// Configure logic for forwards/backwards
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_5 | GPIO_PIN_7);

}

//------------Motor_Start------------
// Starts the motor
// Input: none
// Output: none
void Motor_Start(void) {
	
	// Both motors look at positive duty cycle
	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_5 | GPIO_PIN_7, 0);
	
	// Enable the PWM generator.
  PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMGenEnable(PWM0_BASE, PWM_GEN_1);
}


void Motor_Straight(void) {
	
	// Setting to forward full speed
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, (31* MotorPeriod) / 32);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (31* MotorPeriod) / 32);
	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_5 | GPIO_PIN_7, 0);
	
	
}

void Motor_Turn(void) {
	
  // Setting motors to turn
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, (31* MotorPeriod) / 32);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (1* MotorPeriod) / 32);
	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_5 | GPIO_PIN_7, GPIO_PIN_7);
	
}


int motortestmain(void) {
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
	Motor_Init();
	Motor_Start();
	
	while(1){
		
	}
}

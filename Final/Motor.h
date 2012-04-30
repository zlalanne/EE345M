// Motor.h
// Created By:
// Thomas Brezinski	TCB567
// Zachary Lalanne ZLL67
// Jeff Mahler
// Will Collins
// TA: Zahidul Haq
// Date of last change: 04/22/2012

// CANO Codes for motors
#define MOTOR_START 1
#define MOTOR_STOP 2
#define MOTOR_STRAIGHT 3
#define MOTOR_REVERSE 4
#define MOTOR_LEFT 5
#define MOTOR_RIGHT 6

#define MOTOR_SPEED1 7
#define MOTOR_SPEED2 8
#define MOTOR_SPEED3 9

//------------Motor_Init------------
// Initilizes PWM for motor interfacing
// Input: none
// Output: none
void Motor_Init(void);

//------------Motor_Start------------
// Starts the motor
// Input: none
// Output: none
void Motor_Start(void);

//------------Motor_Stop------------
// Stops the motors the motor
// Input: none
// Output: none
void Motor_Stop(void);

void Motor_Straight(void);
void Motor_Turn_Right(void);
void Motor_Turn_Left(void);
void Motor_Reverse(void);

void Motor_Speed1(void);
void Motor_Speed2(void);
void Motor_Speed3(void);
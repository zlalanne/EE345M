// Servo.h

//-------- Servo_Init -----------
// Initializes the PWM used to control the steering servo
// Inputs: none
// Outputs: none
void Servo_Init(void);
  

//------ Servo_Start --------------
// Starts the servo
// Inputs: none
// Outputs: none
void Servo_Start(void);

void Servo_Set_Position(unsigned long position);

unsigned long Servo_Pulse_Get(void);

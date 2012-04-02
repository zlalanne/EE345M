// IR.c
// Created By:
// Thomas Brezinski	TCB567
// Zachary Lalanne ZLL67
// Jeff Mahler
// Will Collins
// TA: Zahidul Haq
// Date of last change: 03/31/2012

#include "ADC.h"
#include "OS.h"
#include "IR.h"

#include "inc/lm3s8962.h"

#define NUM_SENSORS 4
#define IR_FREQ 10

unsigned short SensorValues[NUM_SENSORS];

void IRSensor0_Handler(unsigned short data);
void IRSensor1_Handler(unsigned short data);
void IRSensor2_Handler(unsigned short data);
void IRSensor3_Handler(unsigned short data);

void IR_Init(void) {
  ADCTask tasks[4];
	tasks[0] = &IRSensor0_Handler;
	tasks[1] = &IRSensor1_Handler;  
	tasks[2] = &IRSensor2_Handler;  
	tasks[3] = &IRSensor3_Handler; 
	
	ADC_CollectSequence(4, IR_FREQ, tasks);
}

unsigned short IR_GetDistance(int sensor) {
	return SensorValues[sensor];
}

void IRSensor0_Handler(unsigned short data) {
  SensorValues[0] = data;
}

void IRSensor1_Handler(unsigned short data) {
	SensorValues[1] = data;
}

void IRSensor2_Handler(unsigned short data) {
	SensorValues[2] = data;
}

void IRSensor3_Handler(unsigned short data) {
	SensorValues[3] = data;
}

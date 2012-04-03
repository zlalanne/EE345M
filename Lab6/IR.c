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
#define MEDIAN_SIZE 3
#define IR_FREQ 10

Sema4Type SensorDataAvailable[4];

unsigned short SensorDistances[4];
unsigned short S0Values[MEDIAN_SIZE] = {0, };
unsigned short S1Values[MEDIAN_SIZE] = {0, };
unsigned short S2Values[MEDIAN_SIZE] = {0, };
unsigned short S3Values[MEDIAN_SIZE] = {0, };
int S0PutIndex = 0;
int S1PutIndex = 0;
int S2PutIndex = 0;
int S3PutIndex = 0;

void IRSensor0_Handler(unsigned short data);
void IRSensor1_Handler(unsigned short data);
void IRSensor2_Handler(unsigned short data);
void IRSensor3_Handler(unsigned short data);
unsigned short Median(unsigned short v1, unsigned short v2, unsigned short v3);

void IR_Init(void) {
  ADCTask tasks[4];
	tasks[0] = &IRSensor0_Handler;
	tasks[1] = &IRSensor1_Handler;  
	tasks[2] = &IRSensor2_Handler;  
	tasks[3] = &IRSensor3_Handler; 
	
	ADC_CollectSequence(4, IR_FREQ, tasks);

	OS_InitSemaphore(&SensorDataAvailable[0], 0);
	OS_InitSemaphore(&SensorDataAvailable[1], 0);
	OS_InitSemaphore(&SensorDataAvailable[2], 0);
	OS_InitSemaphore(&SensorDataAvailable[3], 0);
}

unsigned short IR_GetDistance(int sensor) {
  OS_bWait(&SensorDataAvailable[sensor]);
	return SensorDistances[sensor];
}

void IRSensor0_Handler(unsigned short data) {
  S0Values[S0PutIndex] = data; // overwrite old data if it is there
	S0PutIndex = (S0PutIndex + 1) % MEDIAN_SIZE;
  SensorDistances[0] = Median(S0Values[0], S0Values[1], S0Values[2]);
	OS_Signal(&SensorDataAvailable[0]);
}

void IRSensor1_Handler(unsigned short data) {
	S1Values[S1PutIndex] = data; // overwrite old data if it is there
	S1PutIndex = (S1PutIndex + 1) % MEDIAN_SIZE;
  SensorDistances[1] = Median(S1Values[0], S1Values[1], S1Values[2]);
	OS_Signal(&SensorDataAvailable[1]);
}

void IRSensor2_Handler(unsigned short data) {
	S2Values[S2PutIndex] = data; // overwrite old data if it is there
	S2PutIndex = (S2PutIndex + 1) % MEDIAN_SIZE;
  SensorDistances[2] = Median(S2Values[0], S2Values[1], S2Values[2]);
	OS_Signal(&SensorDataAvailable[2]);
}

void IRSensor3_Handler(unsigned short data) {
	S3Values[S3PutIndex] = data; // overwrite old data if it is there
	S3PutIndex = (S3PutIndex + 1) % MEDIAN_SIZE;
  SensorDistances[3] = Median(S3Values[0], S3Values[1], S3Values[2]);
	OS_Signal(&SensorDataAvailable[3]);
}

unsigned short Median(unsigned short v1, unsigned short v2, unsigned short v3) {
  if (v1 < v2 && v1 < v3) {
	  if (v2 < v3) return v2;
		else return v3;
	}	else if (v2 < v1 && v2 < v3) {
	  if (v1 < v3) return v1;
		else return v3;
	} else if (v2 < v1) {
	  return v2;
	}
	return v1;
}

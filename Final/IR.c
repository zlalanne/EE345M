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
#define TABLE_SIZE 12

Sema4Type SensorDataAvailable[4];

unsigned short Sensor0Calibration[TABLE_SIZE] = {1023, 855, 585, 450, 355, 310, 230, 185, 158, 125, 105,   0};
unsigned short Sensor0Measurement[TABLE_SIZE] = {   5,  10,  15,  20,  25,  30,  40,  50,  60,  70,  80, 100};

unsigned short Sensor1Calibration[TABLE_SIZE] = {1023, 855, 585, 450, 355, 310, 230, 185, 158, 125, 105,   0};
unsigned short Sensor1Measurement[TABLE_SIZE] = {   5,  10,  15,  20,  25,  30,  40,  50,  60,  70,  80, 100};

unsigned short Sensor2Calibration[TABLE_SIZE] = {1023, 855, 585, 450, 355, 310, 230, 185, 158, 125, 105,   0};
unsigned short Sensor2Measurement[TABLE_SIZE] = {   5,  10,  15,  20,  25,  30,  40,  50,  60,  70,  80, 100};

unsigned short Sensor3Calibration[TABLE_SIZE] = {1023, 878, 595, 455, 365, 315, 240, 195, 160, 140, 125,   0};
unsigned short Sensor3Measurement[TABLE_SIZE] = {   5,  10,  15,  20,  25,  30,  40,  50,  60,  70,  80, 100};

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
unsigned short Interpolate(unsigned short val, unsigned short* calibration, unsigned short* measurement, int size);

void IR_Init(void) {
    
	ADCTask tasks[4];
	tasks[0] = &IRSensor0_Handler;
	tasks[1] = &IRSensor1_Handler;  
	tasks[2] = &IRSensor2_Handler;  
	tasks[3] = &IRSensor3_Handler; 
	
	ADC_CollectSequence(4, IR_FREQ, tasks);
	
	// initialize semaphores to -2 to ensure that data is not read before the 3rd sample	
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
	unsigned short dist;
    S0Values[S0PutIndex] = data; // overwrite old data if it is there
	S0PutIndex = (S0PutIndex + 1) % MEDIAN_SIZE;
    dist = Median(S0Values[0], S0Values[1], S0Values[2]);
	SensorDistances[0] = Interpolate(dist, Sensor0Calibration, Sensor0Measurement, TABLE_SIZE);
	OS_Signal(&SensorDataAvailable[0]);
}

void IRSensor1_Handler(unsigned short data) {
	unsigned short dist;
	S1Values[S1PutIndex] = data; // overwrite old data if it is there
	S1PutIndex = (S1PutIndex + 1) % MEDIAN_SIZE;
    dist = Median(S1Values[0], S1Values[1], S1Values[2]);
	SensorDistances[1] = dist;
	SensorDistances[1] = Interpolate(dist, Sensor1Calibration, Sensor1Measurement, TABLE_SIZE);
	OS_Signal(&SensorDataAvailable[1]);
}

void IRSensor2_Handler(unsigned short data) {
	unsigned short dist;
	S2Values[S2PutIndex] = data; // overwrite old data if it is there
	S2PutIndex = (S2PutIndex + 1) % MEDIAN_SIZE;
    dist = Median(S2Values[0], S2Values[1], S2Values[2]);
	SensorDistances[2] = Interpolate(dist, Sensor2Calibration, Sensor2Measurement, TABLE_SIZE);
	OS_Signal(&SensorDataAvailable[2]);
}

void IRSensor3_Handler(unsigned short data) {
	unsigned short dist;
	S3Values[S3PutIndex] = data; // overwrite old data if it is there
	S3PutIndex = (S3PutIndex + 1) % MEDIAN_SIZE;
	dist = Median(S3Values[0], S3Values[1], S3Values[2]);
	SensorDistances[3] = Interpolate(dist, Sensor3Calibration, Sensor3Measurement, TABLE_SIZE);
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

unsigned short Interpolate(unsigned short val, unsigned short* calibration, unsigned short* measurement, int size) {
  int x = 1;
	unsigned short distance;
	short slope;
	short dx = 0;
	short dy = 0;
	short diff;

	while (x < size && calibration[x] >= val) {
	  x++;
	}

	dy = measurement[x] - measurement[x-1];
	dx = calibration[x] - calibration[x-1];
	diff = val - calibration[x-1];

	distance = (((dy * diff) + (measurement[x-1] * dx)) / dx);
	return distance;
}

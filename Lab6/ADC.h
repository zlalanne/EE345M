// Modified By:
// Thomas Brezinski	TCB567
// Zachary Lalanne ZLL67
// TA: Zahidul Haq
// Date of last change: 2/24/2012

// Written By:
// Megan Ruthven MAR3939
// Zachary Lalanne ZLL67
// TA: NACHI
// Date of last change: 10/17/2011

// Boolean phrases
#ifndef boolean
  #define boolean
  #define TRUE 1
  #define FALSE 0
  #define SUCCESS 1
  #define FAILURE 0
  #define VALID 1
  #define INVALID 0
#endif


void ADC_Open(void);
int ADC_Status(void);
unsigned long ADC_In(unsigned int channelNum);
int ADC_Collect(unsigned int channelNum, unsigned int fs, 
  void (*task)(unsigned short));


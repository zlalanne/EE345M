// Modified By:
// Thomas Brezinski
// Zachary Lalanne ZLL67
// TA:
// Date of last change: 1/25/2012

// Written By:
// Megan Ruthven MAR3939
// Zachary Lalanne ZLL67
// TA: NACHI
// Date of last change: 10/17/2011

//------------ADC_Init------------
// Initializes the Timer0A, ADC0 to interrupt 
// with sequence 3
// Input: none
// Output: none
void ADC_Init(void);
// test

//------------Get_ADC_Data------------
// If the MAILflag is set to true, then the values of the current 
// ADCdata, Rdata, and Tdata are stored into the refferences passed 
// Input: unsigned int, and char array
// Output: none
unsigned char Get_ADC_Data(unsigned short *temperature, unsigned long *resistance);


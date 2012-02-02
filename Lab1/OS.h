// Modified By:
// Thomas Brezinski
// Zachary Lalanne ZLL67
// TA:
// Date of last change: 1/25/2012

// Written By:
// Thomas Brezinski TCB567
// Zachary Lalanne ZLL67
// TA: 
// Date of last change: 10/17/2011

						   
#ifndef __OS_H
#define __OS_H  1

// fill these depending on your clock (Currently for 50Mhz clock)
#define TIME_1MS  50000
#define TIME_2MS  2*TIME_1MS


// ******** OS_Init ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: serial, ADC, systick, select switch and timer2
// input:  none
// output: none
void OS_Init(void);

//******** OS_AddThread ***************
// add three foregound threads to the scheduler
// Inputs: three pointers to a void/void foreground tasks
// Outputs: 1 if successful, 0 if this thread can not be added
int OS_AddThreads(void(*task0)(void),
                 void(*task1)(void),
                 void(*task2)(void));

//******** OS_Launch ***************
// start the scheduler, enable interrupts
// Inputs: number of 20ns clock cycles for each time slice
//         (maximum of 24 bits)
// Outputs: none (does not return)
void OS_Launch(unsigned long theTimeSlice);

// ********** OS_AddPeriodicThread *********
// Inputs: task is a pointer to the functino to execute every period
// Inputs: period is number of milliseconds, priority is value specified for NVIC
// Output: ??????
void OS_AddPeriodicThread(void(*task)(void), unsigned long period, unsigned long priority);

// ******** OS_ClearMsTime *************
// Clears the Timer2 interrupt counter gTimer2Counter
// Input: none
// Output: none
void OS_ClearMsTime(void);

// ******** OS_MsTime *************
// Returns the Timer2 interrupt counter gTimer2Counter
// Input: none
// Output: current value of gTimer2Counter
unsigned long OS_MsTime(void);


#endif

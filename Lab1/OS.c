// Written By:
// Thomas Brezinski	TCB567
// Zachary Lalanne ZLL67
// TA:
// Date of last change: 1/25/2012


#include "inc/hw_types.h"
#include "inc/hw_timer.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h" 

#include "driverlib/timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"  // defines IntEnable

#include "os.h"

// Assembly function protoypes
//void OS_DisableInterrupts(void); // Disable interrupts
void DisableInterrupts(void); // Disable interrupts
//void OS_EnableInterrupts(void);  // Enable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical(void);
void EndCritical(long primask);
//void StartOS(void);

#define NUMTHREADS 3    // maximum number of threads
#define STACKSIZE  100  // number of 32-bit words in stack

struct tcb{
   long *sp;          // pointer to stack (valid for threads not running
   struct tcb *next;  // linked-list pointer
};

typedef struct tcb tcbType;
tcbType tcbs[NUMTHREADS];
tcbType *RunPt;
long Stacks[NUMTHREADS][STACKSIZE];

unsigned long gTimer2Count;      // global 32-bit counter incremented everytime Timer2 executes
void (*gThread1p)(void);			 //	global function pointer for Thread1 function


void TIMER2_Handler(void);

// ************ OS_Init ******************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: serial, ADC, systick, select switch and timer2
// input: none
// output: non
void OS_Init(void) {
   // FUTURE LAB
}

// *********** OS_AddThread **************
// add three foreground threads to the scheduler
// Inputs: three pointers to a void/void foreground tasks
// Outputs: 1 if successful, 0 if this thread can not be added
int OS_AddThreads(void(*task0)(void),
                  void(*task1)(void),
                  void(*task2)(void)) {
   // FUTURE LAB
   return 1;
}

// ******* OS_Launch *********************
// start the scheduler, enable interrupts
// Inputs: number of 20ns clock cycles for each time slice
//        ( Maximum of 24 bits)
// Outputs: none (does not return)
void OS_Launch(unsigned long theTimeSlice){
   // FUTURE LAB
}

// ********** OS_ClearMsTime ***************
// Resets the 32-bit global counter to 0
// Inputs: none
// Outputs: none
void OS_ClearMsTime(void) {
   gTimer2Count = 0;
}

// ********** OS_MsTime *******************
// Returns the current 32-bit global counter
// Inputs: none
// Outputs: Current 32-bit global counter value
unsigned long OS_MsTime(void) {
   return gTimer2Count;
}

// ********** OS_AddPeriodicThread *********
// Inputs: task is a pointer to the functino to execute every period
// Inputs: period is number of milliseconds, priority is value specified for NVIC
// Output: ??????
void OS_AddPeriodicThread(void(*task)(void),
   unsigned long period,
   unsigned long priority){
   // The Timer2 peripheral must be enabled for use.
   SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
   // Disabling Timer2 for configuration
   TimerDisable(TIMER2_BASE, TIMER_BOTH);
   // Configure Timer2 as a 32-bit periodic timer.
   TimerConfigure(TIMER2_BASE, TIMER_CFG_PERIODIC);
   // Set the Timer2 load value
   TimerLoadSet(TIMER2_BASE, TIMER_A, (period * TIME_1MS)); // should be TIMER_A when configured for 32-bit
   // Clear periodic counter
   OS_ClearMsTime();
   // should the task be registered as the int handler? or should there be a regular int handler that calls task?
   TimerIntRegister(TIMER2_BASE,TIMER_BOTH,TIMER2_Handler);
   // Configure the Timer2 interrupt for timer timeout.
   TimerIntEnable(TIMER2_BASE, TIMER_TIMA_TIMEOUT); // or should it be TIMER_TIMB_TIMEOUT?
   // Enable the Timer2 interrupt on the processor (NVIC).
   IntEnable(INT_TIMER2A); // or should it be INT_TIMER2B?      
   // Set the global function pointer to the address of the provided function
   gThread1p = task;
   TimerEnable(TIMER2_BASE, TIMER_BOTH);
}

void TIMER2_Handler(void){
   TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT); // Or should it be TIMER_TIMB_TIMEOUT
   gTimer2Count++;
   gThread1p();   // Call periodic function
}

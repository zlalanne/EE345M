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

#include "os.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"

// function definitions in osasm.s
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
typedef struct tcb tccbType;
tcbType tcbs[NUMTHREADS];
tcbType *RunPt;
long Stacks[NUMTHREADS][STACKSIZE];

unsigned long gTimer2Count;      // global 32-bit counter incremented everytime Timer2 executes



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
}

// ******* OS_Launch *********************
// start the scheduler, enable interrupts
// Inputs: number of 20ns clock cycles for each time slice
//        ( Maximum of 24 bits)
// Outputs: none (does not return)
void OS_Launch(unsigned long theTimeSlice){
   // FUTURE LAB
}

// ********** OS_AddPeriodicThread *********
// Inputs: task is a pointer to the functino to execute every period
// Inputs: period is number of milliseconds, priority is value specified for NVIC
void OS_AddPeriodicThread(void(*task)(void),
   unsigned long period,
   unsigned long priority){
   // needs to configure Timer2 to run at specified frequency with appropriate priority and execute fx pointer
   
   // The Timer2 peripheral must be enabled for use.
   SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
   // Configure Timer2 as a 32-bit periodic timer.
   TimerConfigure(TIMER2_BASE, TIMER_CFG_PERIODIC);
   // Set the Timer2 load value
   
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
long OS_MsTime(void) {
   return gTimer2Count;
}

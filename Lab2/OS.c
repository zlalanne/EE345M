// Written By:
// Thomas Brezinski	TCB567
// Zachary Lalanne ZLL67
// TA:
// Date of last change: 1/25/2012

#include "OS.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/lm3s8962.h"
 

#include "driverlib/timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"  // defines IntEnable

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
   struct tcb *next, *previous; // linked-list pointer
   char id, sleepState, priority, blockedState;
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
int OS_AddPeriodicThread(void(*task)(void),
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

   return 1;
}

//******** OS_AddThread *************** 
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority (0 is highest)
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
int OS_AddThread(void(*task)(void), 
   unsigned long stackSize, unsigned long priority);

//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
unsigned long OS_Id(void);

// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(unsigned long size) {
  return;
}

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(unsigned long data);  

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long OS_Fifo_Get(void);

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero  if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void);

// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, long value) {
  semaPt->Value = value;
  return;
};

// ******** OS_Wait ************
// decrement semaphore and spin/block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt) {
  IntMasterDisable();
  while(!(semaPt->Value)) {
    IntMasterEnable();
    IntMasterDisable();
  }
  semaPt->Value--; // Lock
  IntMasterEnable();
}

// ******** OS_Signal ************
// increment semaphore, wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt) {
  IntMasterDisable();
  // Free
  semaPt->Value++;
  IntMasterEnable();
}

// ******** OS_bWait ************
// if the semaphore is 0 then spin/block
// if the semaphore is 1, then clear semaphore to 0
// input:  pointer to a binary semaphore
// output: none
void OS_bWait(Sema4Type *semaPt) {
  IntMasterDisable();
  while(semaPt->Value == 0){
    IntMasterEnable();
    IntMasterDisable();
  }
  // Lock
  semaPt->Value = 0;
  IntMasterEnable();
} 

// ******** OS_bSignal ************
// set semaphore to 1, wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt) {
  IntMasterDisable();
  // Free
  semaPt->Value = 1;
  IntMasterEnable();
}



void TIMER2_Handler(void){
   GPIO_PORTG_DATA_R |= 0x01;
   TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT); // Or should it be TIMER_TIMB_TIMEOUT
   gTimer2Count++;
   gThread1p();   // Call periodic function
   GPIO_PORTG_DATA_R &= 0xFE;
}

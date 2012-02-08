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
   unsigned long stackSize, unsigned long priority){
   return 0;
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
int OS_Fifo_Put(unsigned long data){
  return 0;
}

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data
unsigned long OS_Fifo_Get(void){
  return 0;
}

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

//******** OS_AddButtonTask ***************
// add a background task to run whenever the Select button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In labs 2 and 3, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, there will be up to four background threads, and this priority field
//           determines the relative priority of these four threads
int OS_AddButtonTask(void(*task)(void), unsigned long priority) {
  return 0;
}

//******** OS_Id ***************
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero
unsigned long OS_Id(void){
  return 0;
}

// ******** OS_Kill ************
// kill the currently running thread, release its TCB memory
// input:  none
// output: none
void OS_Kill(void) {
  return;
}

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void) {
  return;
}

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received
void OS_MailBox_Send(unsigned long data) {
  return;
}

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty
unsigned long OS_MailBox_Recv(void){
  return 0;
}

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(unsigned long sleepTime) {
  return;
}

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void) {
  return;
}

// ******** OS_Time ************
// reads a timer value
// Inputs:  none
// Outputs: time in 20ns units, 0 to max
// The time resolution should be at least 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as
//   this function and OS_TimeDifference have the same resolution and precision
unsigned long OS_Time() {
  return 0;
}

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 20ns units
// The time resolution should be at least 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as
//   this function and OS_Time have the same resolution and precision
unsigned long OS_TimeDifference(unsigned long start, unsigned long stop){
  return 0;
}


void TIMER2_Handler(void){
   GPIO_PORTG_DATA_R |= 0x01;
   TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT); // Or should it be TIMER_TIMB_TIMEOUT
   gTimer2Count++;
   gThread1p();   // Call periodic function
   GPIO_PORTG_DATA_R &= 0xFE;
}

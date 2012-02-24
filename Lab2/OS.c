// Written By:
// Thomas Brezinski	TCB567
// Zachary Lalanne ZLL67
// TA: Zahidul Haq
// Date of last change: 2/24/2012

#include "OS.h"
#include "UART.h"	// defines UART0_Init for OS_Init
#include "ADC.h"    // defines ADC_Open for OS_Init
#include "Output.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/lm3s8962.h"
 
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"  // defines IntEnable
#include "driverlib/systick.h"

#define MAXTHREADS 8    // maximum number of threads
#define STACKSIZE  512  // number of 32-bit words in stack
#define FIFOSIZE 256

// TCB structure, assembly code depends on the order of variables
// in this structure. Specifically sp, next and sleepState.
struct tcb{
   long *sp;          // pointer to stack (valid for threads not running)
   struct tcb *next, *prev;  // linked-list pointer
   long sleepState, blockedState;
   long id, priority;
   char valid;
   long stack[STACKSIZE]; 
};

// Global variables for TCBs
typedef struct tcb tcbType;
tcbType tcbs[MAXTHREADS];
tcbType *RunPt = '\0';
tcbType *NextPt = '\0';
tcbType *Head = '\0';
tcbType *Tail = '\0';
int NumThreads = 0; // global index to point to place to put new tcb in array

unsigned long gTimeSlice;


// Global variables for background thread
unsigned long gTimer2Count;      // global 32-bit counter incremented everytime Timer2 executes
void (*gThread1p)(void);			 //	global function pointer for Thread1 function
char gThreadValid = INVALID;

// Global variables for button thread
void (*gButtonThreadPt)(void);       // global function pointer for the thread to be launched on button pushes
int gButtonThreadPriority;

// Global variables for mailbox
Sema4Type BoxFree;
Sema4Type DataValid;
unsigned long Mailbox;

// Global variables for FIFO
unsigned long OS_Fifo[FIFOSIZE];
Sema4Type FifoMutex;
Sema4Type CurrentSize;
unsigned long *OS_PutPt, *OS_GetPt;

// Interrupt Handlers
void Select_Switch_Handler(void);
void Timer2_Handler(void);
void SysTick_Handler(void);


// ********* Scheduler *************
// Calculates next thread to be run and sets NextPt to it
void Scheduler(void) {
   NextPt = (*RunPt).next;
   while ((*NextPt).sleepState != 0) {
      NextPt = (*NextPt).next;
   }
   IntPendSet(FAULT_PENDSV); 
}

// ************ OS_Init ******************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: serial, ADC, systick, select switch and timer2
// input: none
// output: non
void OS_Init(void) {

	int i; // Used for indexing

	// Disable interrupts
	OS_DisableInterrupts();

	// Setting the clock to 50 MHz
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
    	SYSCTL_XTAL_8MHZ);   
	
	// Initialze peripherals
	UART0_Init();
	ADC_Open();
	Output_Init();
	
	// Initialize systick - period is set in OS_Launch
	SysTickIntRegister(SysTick_Handler);
	SysTickIntEnable();	

	// Select switch
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_1);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
	GPIOPortIntRegister(GPIO_PORTF_BASE, Select_Switch_Handler);
	GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_RISING_EDGE);
	GPIOPinIntClear(GPIO_PORTF_BASE, GPIO_PIN_1);
	GPIOPinIntEnable(GPIO_PORTF_BASE, GPIO_PIN_1);
	
	// Initialize Timer2A and Timer2B
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
    TimerDisable(TIMER2_BASE, TIMER_A | TIMER_B);
    TimerConfigure(TIMER2_BASE, TIMER_CFG_16_BIT_PAIR | TIMER_CFG_A_PERIODIC | TIMER_CFG_B_PERIODIC);
    TimerIntEnable(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	TimerIntDisable(TIMER2_BASE, TIMER_TIMB_TIMEOUT);
    TimerLoadSet(TIMER2_BASE, TIMER_A, TIME_1MS);
	TimerLoadSet(TIMER2_BASE, TIMER_B, 65535);
	IntEnable(INT_TIMER2A);

	// Setting priorities for all interrupts
	// To add more look up correct names in inc\hw_ints.h
	IntPrioritySet(FAULT_PENDSV, 7);
	IntPrioritySet(FAULT_SYSTICK, 6);
	
	IntPrioritySet(INT_GPIOF, 3);
	IntPrioritySet(INT_TIMER2A, 0);
	IntPrioritySet(INT_UART0, 4);
    IntPrioritySet(INT_ADC0SS0, 2);
	IntPrioritySet(INT_ADC0SS3, 0);
	
	// Initializing TCBs
	for(i = 0; i < MAXTHREADS; i++) {
	  tcbs[i].valid = INVALID;
	}

	RunPt = &tcbs[0];       // Thread 0 will run first
}

// *********** OS_AddThreads **************
// add three foreground threads to the scheduler
// Inputs: three pointers to a void/void foreground tasks
// Outputs: 1 if successful, 0 if this thread can not be added
int OS_AddThreads(void(*task0)(void),
                  void(*task1)(void),
                  void(*task2)(void)) {
   
   int result = 0;
   result += OS_AddThread(task0, STACKSIZE, 0);
   result += OS_AddThread(task1, STACKSIZE, 0);
   result += OS_AddThread(task2, STACKSIZE, 0);

   if(result == 3) {
      return 1;
   } else {
      return 0;
   }
}

// *********** setInitialStack **************
// Initializes the stack for easy debugging
// Inputs: Index of tcbs array to initialize 
// Outputs: None
void setInitialStack(int i) {
  tcbs[i].sp = &tcbs[i].stack[STACKSIZE-16]; // thread stack pointer
  tcbs[i].stack[STACKSIZE-1] = 0x01000000;   // thumb bit
  tcbs[i].stack[STACKSIZE-3] = 0x14141414;   // R14
  tcbs[i].stack[STACKSIZE-4] = 0x12121212;   // R12
  tcbs[i].stack[STACKSIZE-5] = 0x03030303;   // R3
  tcbs[i].stack[STACKSIZE-6] = 0x02020202;   // R2
  tcbs[i].stack[STACKSIZE-7] = 0x01010101;   // R1
  tcbs[i].stack[STACKSIZE-8] = 0x00000000;   // R0
  tcbs[i].stack[STACKSIZE-9] = 0x11111111;   // R11
  tcbs[i].stack[STACKSIZE-10] = 0x10101010;  // R10
  tcbs[i].stack[STACKSIZE-11] = 0x09090909;  // R9
  tcbs[i].stack[STACKSIZE-12] = 0x08080808;  // R8
  tcbs[i].stack[STACKSIZE-13] = 0x07070707;  // R7
  tcbs[i].stack[STACKSIZE-14] = 0x06060606;  // R6
  tcbs[i].stack[STACKSIZE-15] = 0x05050505;  // R5
  tcbs[i].stack[STACKSIZE-16] = 0x04040404;  // R4
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
  unsigned long stackSize, unsigned long priority) {
   
  long status;
  int i;
  int index;

  status = StartCritical();

  if(NumThreads == 0) {
    // First thread no TCBs yet
    tcbs[0].next = &tcbs[0];
	tcbs[0].prev = &tcbs[0];
	Head = &tcbs[0];
	Tail = &tcbs[0];
	index = 0;
  } else {
    
	// Look for open spot in tcbs array
	for(i = 0; i < MAXTHREADS; i++) {
	  if(tcbs[i].valid == INVALID) {
	    index = i;
		i = MAXTHREADS; // Exit loop
	  } else {
	    index = -1;  // Sentinel to detect no invalid spots
	  }
	}

	if(index == -1) {
	  EndCritical(status);
	  return FAILURE; // No space in tcbs
	}

	tcbs[index].next = Head; // New tcb points to head
	tcbs[index].prev = Tail; // Point back to current tail
	(*Tail).next = &tcbs[index]; // Tail now points to new tcb
	Tail = &tcbs[index]; // New tcb becomes the tail
	(*Head).prev = &tcbs[index]; // Head now points backwards to new tcb
  }


  // Initilizing the stack for debugging
  setInitialStack(index);

  // Set PC for stack to point to function to run
  tcbs[index].stack[STACKSIZE-2] = (long)(task);

  // Set inital values for sleep status and id
  tcbs[index].sleepState = 0;
  tcbs[index].id = index;
  tcbs[index].valid = VALID;
  NumThreads++;
  
  EndCritical(status);

  return SUCCESS;
}

// ******* OS_Launch *********************
// start the scheduler, enable interrupts
// Inputs: number of 20ns clock cycles for each time slice
//        ( Maximum of 24 bits)
// Outputs: none (does not return)
void OS_Launch(unsigned long theTimeSlice){
  gTimeSlice = theTimeSlice;
  SysTickPeriodSet(theTimeSlice);
  SysTickEnable();
  SysTickIntEnable();
  TimerEnable(TIMER2_BASE, TIMER_A | TIMER_B);

  StartOS(); // Assembly language function that initilizes stack for running
  OS_EnableInterrupts();
 
  while(1) { }
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

//******** OS_AddPeriodicThread *************** 
// add a background periodic task
// typically this function receives the highest priority
// Inputs: pointer to a void/void background function
//         period given in system time units
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// You are free to select the time resolution for this function
// This task does not have a Thread ID
// In lab 2, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddPeriodicThread(void(*task)(void),
   unsigned long period,
   unsigned long priority){

   // Clear periodic counter
   OS_ClearMsTime();
       
   // Set the global function pointer to the address of the provided function
   gThread1p = task;
   gThreadValid = VALID;

   // Sets new TIMER2 period
   TimerLoadSet(TIMER2_BASE, TIMER_A, period);

   return 1;
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
  gButtonThreadPt = task;
  gButtonThreadPriority = priority;
  return 1;
}

// ******** OS_Kill ************
// kill the currently running thread, release its TCB memory
// input:  none
// output: none
void OS_Kill(void) {

  long status;
  int id;
  tcbType *temp;

  // Starting critical section to delete TCB
  status = StartCritical();
  
  NumThreads--;
  id = (*RunPt).id;

  // Make TCB invalid
  tcbs[id].valid = INVALID;
  
  // Remove TCB from linked list
  temp = tcbs[id].prev;
  (*temp).next = tcbs[id].next;
  temp = tcbs[id].next;
  (*temp).prev = tcbs[id].prev;

  
  // Check if thread is Head or Tail
  if(&tcbs[id] == Head) {
    Head = tcbs[id].next; 
  }	else if(&tcbs[id] == Tail) {
    Tail = tcbs[id].prev;
  }

  EndCritical(status);

  // Trigger threadswitch
  IntPendSet(FAULT_SYSTICK);
  while(1){} // Never leave
}

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(unsigned long sleepTime) {

  (*RunPt).sleepState = (2*sleepTime);		  
  IntPendSet(FAULT_SYSTICK); // Triger threadswitch
  
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
  IntPendSet(FAULT_SYSTICK); // Triger threadswitch
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
  return TimerValueGet(TIMER2_BASE , TIMER_B);
}

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 20ns units 
// The time resolution should be at least 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
unsigned long OS_TimeDifference(unsigned long start, unsigned long stop){
  unsigned long difference;
  difference = (start - stop) & 0x0000FFFF;
  return difference;
}

//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
unsigned long OS_Id(void){
  return (*RunPt).id;
}

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
  
  OS_InitSemaphore(&FifoMutex, 1);
  OS_InitSemaphore(&CurrentSize, 0);
  OS_PutPt = &OS_Fifo[0];
  OS_GetPt = &OS_Fifo[0];

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
int OS_Fifo_Put(unsigned long data) {
  unsigned long *nextPutPt;
  nextPutPt = OS_PutPt+1;

  if(nextPutPt == &OS_Fifo[FIFOSIZE]) {
    nextPutPt = &OS_Fifo[0]; //Wrap
  }

  if(nextPutPt == OS_GetPt) {
    return FAILURE;
  } else {
    *(OS_PutPt) = data;
	OS_PutPt = nextPutPt;
	OS_Signal(&CurrentSize);
	return SUCCESS;
  }
}  

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long OS_Fifo_Get(void){
  unsigned long data;

  OS_Wait(&CurrentSize);
  OS_Wait(&FifoMutex);
  data = *(OS_GetPt++);
  
  if(OS_GetPt == &OS_Fifo[FIFOSIZE]) {
    OS_GetPt = &OS_Fifo[0];
  }

  OS_Signal(&FifoMutex);
  return data;
}

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero  if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void) {
  return CurrentSize.Value;
}

// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, long value) {
  (*semaPt).Value = value;
  return;
};

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void) {
  OS_InitSemaphore(&BoxFree, 1);
  OS_InitSemaphore(&DataValid, 0);
  Mailbox = 0;
  return;
}

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(unsigned long data) {
  OS_bWait(&BoxFree);
  Mailbox = data;
  OS_bSignal(&DataValid);
  return;
}

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
unsigned long OS_MailBox_Recv(void) {
  unsigned long mail;
  
  OS_bWait(&DataValid);
  mail = Mailbox;
  OS_bSignal(&BoxFree);
  
  return mail;
}

// ******** Timer2A_Handler ************
// Updates sleep state and calls periodic function
// For lab 2 this is executing at 2kHz
void Timer2A_Handler(void) {
  int i;
 
  TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
  gTimer2Count++;

  if(gThreadValid == VALID) {
    gThread1p();   // Call periodic function
  }

  // Decrment sleepState of each TCB
  for(i = 0; i < MAXTHREADS; i++) {
    if(tcbs[i].sleepState != 0) {
	  tcbs[i].sleepState--;
    }
  }
}

// ******** Select_Switch_Handler ************
// Clears the interrupt and starts buttion function
void Select_Switch_Handler(void){
	GPIOPinIntClear(GPIO_PORTF_BASE, GPIO_PIN_1);
	gButtonThreadPt();	
}

// ******** SysTick_Handler ************
// Resets its value to gTimeSlice and calls thread scheduler
void SysTick_Handler(void) {
   SysTickPeriodSet(gTimeSlice);  
   Scheduler();
}


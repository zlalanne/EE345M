
#include "Output.h"
#include "rit128x96x4.h"


// Functions from startup.s
void EnableInterrupts(void);
void DisableInterrupts(void);
long StartCritical(void);
void EndCritical(long st);


int main(void){
  const char prompt[] = "Set the alarm time:";

  Output_Init();
  

  // Enabling interrupts
  EnableInterrupts();

  RIT128x96x4StringDraw(prompt,0,0,0xF);

  for(;;){

  }

}

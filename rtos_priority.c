/*  
 *    SIMPLE RTOS (pre-emptive scheduler) for HCS08
 *
 *  Copyright (c) 2004-2015  Dr. Fernando Rodriguez Salazar
 * 
 * This version of the scheduler uses the TPM timer, and allows assigning priorities to 
 * the different tasks, so a given task executes more often than another/
 * 
 * This is a very simple pre-emptive scheduler for the HCS08 family of microcontrollers.
 * Such microcontrollers lack resources to run a more complex OS, yet are prefectly fitted
 * for multitasking via simple schedulers, such as this one.
 * 
 * The system works by scheduling tasks in a mary-go-round fashion, letting them run
 * for a tick, and then pre-emptying them, saving the CPU state in a stack space reserved
 * independently and individually for each task.
 * 
 * Please see the accompanying License file for licensing information
 */
 

#include <hidef.h> /* for EnableInterrupts macro */
#include "derivative.h" /* include peripheral declarations */


void WaitNext(void);


unsigned char function1_stack[30];
void function1() {
  for (;;) {
    PTAD=PTAD&0xF0| (!PTAD)&0x0F;
    __RESET_WATCHDOG();
  }
}

unsigned char function2_stack[20];
void function2() {
  for (;;) {
    WaitNext();
    PTAD=PTAD&0x0F| (!PTAD)&0xF0;
     __RESET_WATCHDOG();
  }
}



void RegisterFunction(unsigned char i, int stack_size);

void main(void) {
  RegisterFunction(0, sizeof(function1_stack));
  RegisterFunction(1, sizeof(function2_stack));
  TPM1SC=0x0A;
  TPM1C0SC=0x50;  //Compare mode, Interrupt enabled
  EnableInterrupts; 
  
  for(;;) {
    __RESET_WATCHDOG(); //Do nothing; wait for interrupt.  We will NEVER return here.
  } 
}



void *TASKS[]={function1, function2};
unsigned char *STACKS[]={function1_stack, function2_stack};
unsigned char *STACK_POINTERS[]={function1_stack, function2_stack};
unsigned int TIMES[]={200, 1000};  //How long to spend in each task


void RegisterFunction(unsigned char i, int stack_size)
{
  static void *f;
  f=TASKS[i];
  STACKS[i][stack_size-2]=(unsigned char)((int)f>>8);
  STACKS[i][stack_size-1]=(unsigned char)(int)f&0xff;
  STACK_POINTERS[i]=STACKS[i]+stack_size-1-5;  //
}


#pragma INLINE
static void ScheduleNext(void) {
  static signed char current_task=-1;
  static signed char next_task;
  static void *_pCurTask, *_pNexTask;
 
  next_task=current_task+1;
  if(next_task>=sizeof(TASKS)/sizeof(void *))
    next_task=0; 
  
  TPM1C0V+=TIMES[next_task]; //Schedule next task for future time
  
   _pNexTask=STACK_POINTERS[next_task];
  __asm { tsx 
    sthx _pCurTask
    ldhx _pNexTask
    txs ; 
    }
  if (current_task!=-1)  
    STACK_POINTERS[current_task]=_pCurTask;
  current_task=next_task;  
}



void WaitNext() {
	__asm { 
		pshx
		psha
		tpa
		psha 
	  	pshh ;
	}
	ScheduleNext();
	__asm  {
	   	pulh
	   	rti ; 
  	}
}
	
interrupt 5 void TimerISR(void) {
	TPM1C0SC&=~0x80;
	ScheduleNext();
	return;
}

  
  




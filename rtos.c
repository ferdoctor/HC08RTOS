/*  
 *    SIMPLE RTOS (pre-emptive scheduler) for HCS08
 *
 *  Copyright (c) 2004-2015  Dr. Fernando Rodriguez Salazar
 * 
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


//For example define two tasks:

unsigned char task1_stack[30];     //Each task needs to reserve space for its stack
								   //Be careful to not use all memory; but save enough space to accomodate 
								   // stack formations (function calls, parameters, and CPU state)
void task1() {
  for (;;) {
    PTAD=PTAD&0xF0| (!PTAD)&0x0F;
    __RESET_WATCHDOG();
  }
}

unsigned char task2_stack[20];
void task2() {
  for (;;) {
    PTAD=PTAD&0x0F| (!PTAD)&0xF0;
     __RESET_WATCHDOG();
  }
}


//Tasks need to be (arbitrarily) ordered and the following structures defined:

const void *TASKS[]={task1, task2};							//Holds starting location of each task
unsigned char *STACKS[]={task1_stack, task2_stack};			//Holds location of each task stack 
unsigned char *STACK_POINTERS[]={task1_stack, task2_stack}; //Hold location of current stack pointer for each task



/* 	Function used to register a task with scheduler.
	Need to call this with the ordinal number associated to each task
	to register the task with the kernel */
void RegisterFunction(unsigned char i, int stack_size);

void main(void) {
	//Our first step is to register the tasks with the scheduler:
	RegisterFunction(0, sizeof(task1_stack));
	RegisterFunction(1, sizeof(task2_stack));
	
	//Timer 1 is used to generate periodic interrupts to drive the preemptive scheduler.
	TPM1SC=0x4A;
	EnableInterrupts; 
	
	//Do nothing; wait for interrupt.  We will NEVER return here after first interrupt occurs.
	for(;;) {
		__RESET_WATCHDOG(); 
	} 
}





/* This function creates a proper stack formation within the stack
 * associated to task [i].   The task will be entered via a RTI instruction
 * which will pull the data stored by this routine (mainly the PC) and hence
 * start execution from the correct place
 */
 
void RegisterFunction(unsigned char i, int stack_size)
{
  static void *f;
  f=TASKS[i];											//Get entry point (PC) of task
  STACKS[i][stack_size-2]=(unsigned char)((int)f>>8);	//And store in correct position in stack
  STACKS[i][stack_size-1]=(unsigned char)(int)f&0xff;
  STACK_POINTERS[i]=STACKS[i]+stack_size-1-5;  			// Adjust stack pointer of stack, as we have stored data in it
}


/* ISR for timer.  This is the only entry point into the kernel.
 * This functions schedule the tasks declared in a mary-go-round
 * fashion.   This routine can be modified to use a different 
 * timer or another mechanism to preempt tasks.
 */
 
interrupt 8 void TimerISR(void) {
  static signed char current_task=-1;  
  static signed char next_task;
  static void *_pCurTask, *_pNexTask;
  TPM1SC&=~0x80;
 
  next_task=current_task+1;				//Cycle through the tasks
  if(next_task>=sizeof(TASKS)/sizeof(void *))
    next_task=0; 
  
   _pNexTask=STACK_POINTERS[next_task];	//Get stack pointer of next task to be executed
  __asm { 
		tsx 				//This gets current stack pointer
		sthx _pCurTask		//    which gets stored here
		ldhx _pNexTask		//
		txs ; 				// And this pushes the sp of next stack into the CPUs SP
    }
  if (current_task!=-1)  // No need to save SP of init
    STACK_POINTERS[current_task]=_pCurTask;		//Save SP of pre-empted task for future schedulings
  current_task=next_task;
  return;
}
  
  



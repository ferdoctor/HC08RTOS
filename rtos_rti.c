/*  
 *    SIMPLE RTOS (pre-emptive scheduler) for HCS08
 *
 *  Copyright (c) 2004-2015  Dr. Fernando Rodriguez Salazar
 * 
 * This version of the scheduler uses the RTI (Realtime Interrupt) which makes it ideal
 * for LOW POWER operation.
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

//Example shows low power mode of operation.  RealTime Interrupt is used as the timer.
//so frequency of interrupts is Fe/1024.  A task can optionally call STOP or the WaitNext
//convenience function, which does the same.  This will result in the micro going to sleep
//for the remainder of the task allocated time (hence save power).  This maintains accurate
//timing, so tasks can still rely on being scheduled at the same frequency.


void WaitNext(void);
#define Stop __asm STOP
  

unsigned char function1_stack[10];  //6 is the minimum size; variables take space.
void function1() {
  int i;
  for (;;) {
    PTFDD=0xff;
    for (i=0; i<100; i++) {
      __RESET_WATCHDOG();
      PTFD=i;
      Stop;
    }
  }
}

unsigned char function2_stack[6];  //6 is the minimum size.
void function2() {
  for (;;) {
    Stop;

    PTBD=PTAD&0x0F| (!PTAD)&0xF0;
     __RESET_WATCHDOG();
  }
}



void RegisterFunction(unsigned char i, int stack_size);

void main(void) {
  RegisterFunction(0, sizeof(function1_stack));
  RegisterFunction(1, sizeof(function2_stack));

  //Configure Oscillator
  ICGC1=0x3C; //Need to Turn on oscillator in stop3 mode
  ICGC2=0;
  
  SRTISC=0x32; //Use external clk RTI with (div 1024) timeout
  
  SOPT_STOPE=1;  //Enable STOP instruction 
  SPMSC2_PDC=0;  //Enable STOP3 mode
  SPMSC1 = 0x00;    // disable  low voltage detect
  EnableInterrupts; 

  Stop;  
  for(;;) {
    __RESET_WATCHDOG(); //Do nothing; wait for interrupt.  We will NEVER return here.
  } 
}



void *TASKS[]={function1, function2};
unsigned char *STACKS[]={function1_stack, function2_stack};
unsigned char *STACK_POINTERS[]={function1_stack, function2_stack};


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
	
interrupt 25 void RTI_ISR(void) {
	SRTISC|=0x40; //Clear Real Time Interrupt
	ScheduleNext();
	return;
}

  
  




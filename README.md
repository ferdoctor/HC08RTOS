# HC08RTOS
Tiny real-time scheduler (OS) for HC08 microcontrollers

The goal of this project is to maintain a very simple pre-emptive OS/scheduler for the HCS08 family of microcontrollers written in plain C.  As such the kernel consists of a single file with less than 100 lines of C code.
Such microcontrollers lack resources to run a more complex OS, yet are prefectly fitted for multitasking via simple schedulers, such as this one.

The system works by scheduling tasks in a mary-go-round fashion, letting them run for a tick, and then pre-emptying them, saving the CPU state in a stack space reserved independently and individually for each task.
 
Please see the accompanying License file for licensing information

Copyright (c) 2004-2015 Dr. Fernando Rodriguez Salazar

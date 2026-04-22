/*
********************************************************************************************************
*                                                uC/OS-III
*                                          The Real-Time Kernel
*
*
*                         (c) Copyright 2009-2015; Micrium, Inc.; Weston, FL
*                    All rights reserved.  Protected by international copyright laws.
*
*                                           ARM Cortex-M4 Port
*
* File      : OS_CPU_A.S
* Version   : V3.04.05
* By        : JJL
*             BAN
*             JBL
*
* For       : ARMv7 Cortex-M4
* Mode      : Thumb-2 ISA
* Toolchain : GCC ARM
********************************************************************************************************
*/

    .syntax unified
    .cpu cortex-m4
    .fpu softvfp
    .thumb

    .global  OSRunning                                           @ External references
    .global  OSPrioCur
    .global  OSPrioHighRdy
    .global  OSTCBCurPtr
    .global  OSTCBHighRdyPtr
    .global  OSIntExit
    .global  OSTaskSwHook
    .global  OS_CPU_ExceptStkBase

    .global  OSStartHighRdy                                      @ Functions declared in this file
    .global  OSCtxSw
    .global  OSIntCtxSw
    .global  OS_CPU_PendSVHandler

#if 0
    .global  OS_CPU_FP_Reg_Push
    .global  OS_CPU_FP_Reg_Pop
#endif

/*
********************************************************************************************************
*                                               EQUATES
********************************************************************************************************
*/
#define NVIC_INT_CTRL   0xE000ED04                              @ Interrupt control state register.
#define NVIC_SYSPRI14   0xE000ED22                              @ System priority register (priority 14).
#define NVIC_PENDSV_PRI 0xFF                                    @ PendSV priority value (lowest).
#define NVIC_PENDSVSET  0x10000000                              @ Value to trigger PendSV exception.

    .text
    .thumb_func

/*
********************************************************************************************************
*                                   FLOATING POINT REGISTERS PUSH
*                             void  OS_CPU_FP_Reg_Push (CPU_STK  *stkPtr)
*
* Note(s) : 1) This function saves S0-S31, and FPSCR registers of the Floating Point Unit.
*
*           2) Pseudo-code is:
*              a) Get FPSCR register value;
*              b) Push value on process stack;
*              c) Push remaining regs S0-S31 on process stack;
*              d) Update OSTCBCurPtr->StkPtr;
********************************************************************************************************
*/

#if 0
    .thumb_func
OS_CPU_FP_Reg_Push:
    mrs     r1, PSP                                             @ PSP is process stack pointer
    cbz     r1, OS_CPU_FP_nosave                                @ Skip FP register save the first time

    vmrs    r1, FPSCR
    str r1, [r0, #-4]!
    vstmdb  r0!, {s0-s31}
    ldr     r1, =OSTCBCurPtr
    ldr     r2, [r1]
    str     r0, [r2]
OS_CPU_FP_nosave:
    bx      lr
#endif

/*
********************************************************************************************************
*                                   FLOATING POINT REGISTERS POP
*                             void  OS_CPU_FP_Reg_Pop (CPU_STK  *stkPtr)
*
* Note(s) : 1) This function restores S0-S31, and FPSCR registers of the Floating Point Unit.
*
*           2) Pseudo-code is:
*              a) Restore regs S0-S31 of new process stack;
*              b) Restore FPSCR reg value
*              c) Update OSTCBHighRdyPtr->StkPtr pointer of new proces stack;
********************************************************************************************************
*/

#if 0
    .thumb_func
OS_CPU_FP_Reg_Pop:
    vldmia  r0!, {s0-s31}
    ldmia   r0!, {r1}
    vmsr    FPSCR, r1
    ldr     r1, =OSTCBHighRdyPtr
    ldr     r2, [r1]
    str     r0, [r2]
    bx      lr
#endif

/*
********************************************************************************************************
*                                         START MULTITASKING
*                                      void OSStartHighRdy(void)
*
* Note(s) : 1) This function triggers a PendSV exception (essentially, causes a context switch) to cause
*              the first task to start.
*
*           2) OSStartHighRdy() MUST:
*              a) Setup PendSV exception priority to lowest;
*              b) Set initial PSP to 0, to tell context switcher this is first run;
*              c) Set the main stack to OS_CPU_ExceptStkBase
*              d) Trigger PendSV exception;
*              e) Enable interrupts (tasks will run with interrupts enabled).
********************************************************************************************************
*/

    .thumb_func
    .align  4
OSStartHighRdy:
    cpsid   i                                                   @ Prevent interruption during context switch
    ldr     r0, =NVIC_SYSPRI14                                   @ Set the PendSV exception priority
    mov     r1, #NVIC_PENDSV_PRI
    strb    r1, [r0]

    mov     r0, #0
    msr     PSP, r0
    bl      OSTaskSwHook                                        @ Call OSTaskSwHook for FPU Pop

    ldr     r0, =OS_CPU_ExceptStkBase                            @ Initialize the MSP to the OS_CPU_ExceptStkBase
    ldr     r1, [r0]
    msr     MSP, r1

    ldr     r0, =OSPrioCur                                       @ OSPrioCur   = OSPrioHighRdy;
    ldr     r1, =OSPrioHighRdy
    ldrb    r2, [r1]
    strb    r2, [r0]

    ldr     r5, =OSTCBCurPtr
    ldr     r1, =OSTCBHighRdyPtr                                 @ OSTCBCurPtr = OSTCBHighRdyPtr;
    ldr     r2, [r1]
    str     r2, [r5]

    ldr     r0, [r2]                                            @ R0 is new process SP; SP = OSTCBHighRdyPtr->StkPtr;
    msr     PSP, r0                                             @ Load PSP with new process SP

    mrs     r0, CONTROL
    orr     r0, r0, #2
    msr     CONTROL, r0
    isb                                                         @ Sync instruction stream

    ldmfd   sp!, {r4-r11}                                       @ Restore r4-11 from new process stack

#ifdef __ARMVFP__
    ldmfd   sp!, {lr}
    tst     lr, #0x10
    itte    eq
    vldmiaeq r0!, {s16-s31}
#endif
    ldmfd   sp!, {r0-r3}                                       @ Restore r0
    mov     r3, lr
    ldmfd   sp!, {r12, lr}                                     @ Load R12 and LR
    ldmfd   sp!, {r1, r2}                                      @ Load PC and discard xPSR
#ifdef __ARMVFP__
    tst     r3, #0x10
    it      eq
    vldmiaeq sp!, {s0-s15}
#endif
    cpsie   i
    bx      r1

/*
********************************************************************************************************
*                       PERFORM A CONTEXT SWITCH (From task level) - OSCtxSw()
*
* Note(s) : 1) OSCtxSw() is called when OS wants to perform a task context switch.  This function
*              triggers the PendSV exception which is where the real work is done.
********************************************************************************************************
*/

    .thumb_func
OSCtxSw:
    ldr     r0, =NVIC_INT_CTRL                                  @ Trigger the PendSV exception (causes context switch)
    ldr     r1, =NVIC_PENDSVSET
    str     r1, [r0]
    bx      lr

/*
********************************************************************************************************
*                   PERFORM A CONTEXT SWITCH (From interrupt level) - OSIntCtxSw()
*
* Note(s) : 1) OSIntCtxSw() is called by OSIntExit() when it determines a context switch is needed as
*              the result of an interrupt.  This function simply triggers a PendSV exception which will
*              be handled when there are no more interrupts active and interrupts are enabled.
********************************************************************************************************
*/

    .thumb_func
OSIntCtxSw:
    ldr     r0, =NVIC_INT_CTRL                                  @ Trigger the PendSV exception (causes context switch)
    ldr     r1, =NVIC_PENDSVSET
    str     r1, [r0]
    bx      lr

/*
********************************************************************************************************
*                                       HANDLE PendSV EXCEPTION
*                                   void OS_CPU_PendSVHandler(void)
*
* Note(s) : 1) PendSV is used to cause a context switch.  This is a recommended method for performing
*              context switches with Cortex-M3.  This is because the Cortex-M3 auto-saves half of the
*              processor context on any exception, and restores same on return from exception.  So only
*              saving of R4-R11 is required and fixing up the stack pointers.  Using the PendSV exception
*              this way means that context saving and restoring is identical whether it is initiated from
*              a thread or occurs due to an interrupt or exception.
*
*           2) Pseudo-code is:
*              a) Get the process SP, if 0 then skip (goto d) the saving part (first context switch);
*              b) Save remaining regs r4-r11 on process stack;
*              c) Save the process SP in its TCB, OSTCBCurPtr->OSTCBStkPtr = SP;
*              d) Call OSTaskSwHook();
*              e) Get current high priority, OSPrioCur = OSPrioHighRdy;
*              f) Get current ready thread TCB, OSTCBCurPtr = OSTCBHighRdyPtr;
*              g) Get new process SP from TCB, SP = OSTCBHighRdyPtr->OSTCBStkPtr;
*              h) Restore R4-R11 from new process stack;
*              i) Perform exception return which will restore remaining context.
*
*           3) On entry into PendSV handler:
*              a) The following have been saved on the process stack (by processor):
*                 xPSR, PC, LR, R12, R0-R3
*              b) Processor mode is switched to Handler mode (from Thread mode)
*              c) Stack is Main stack (switched from Process stack)
*              d) OSTCBCurPtr      points to the OS_TCB of the task to suspend
*                 OSTCBHighRdyPtr  points to the OS_TCB of the task to resume
*
*           4) Since PendSV is set to lowest priority in the system (by OSStartHighRdy() above), we
*              know that it will only be run when no other exception or interrupt is active, and
*              therefore safe to assume that context being switched out was using the process stack (PSP).
********************************************************************************************************
*/

    .thumb_func
    .align  4
OS_CPU_PendSVHandler:
    cpsid   i                                                   @ Prevent interruption during context switch
    mrs     r0, PSP                                             @ PSP is process stack pointer
#ifdef __ARMVFP__
    tst     r14, #0x10                                          @ if lr.4==0
    ittte   eq
    vmrseq  r1, FPSCR                                          @ use pfu push s0-s15
    vstmdeq r0!, {s16-s31}                                     @ push s16-s31
    subs    r0, r0, #0x4
    str     lr, [r0]
#endif
    stmfd   r0!, {r4-r11}                                       @ Save remaining regs r4-11 on process stack

    ldr     r5, =OSTCBCurPtr                                     @ OSTCBCurPtr->OSTCBStkPtr = SP;
    ldr     r6, [r5]
    str     r0, [r6]                                            @ R0 is SP of process being switched out

                                                                @ At this point, entire context of process has been saved
    mov     r4, lr                                              @ Save LR exc_return value
    bl      OSTaskSwHook                                        @ OSTaskSwHook();
    mov     lr, r4

    ldr     r0, =OSPrioCur                                       @ OSPrioCur   = OSPrioHighRdy;
    ldr     r1, =OSPrioHighRdy
    ldrb    r2, [r1]
    strb    r2, [r0]

    ldr     r1, =OSTCBHighRdyPtr                                 @ OSTCBCurPtr = OSTCBHighRdyPtr;
    ldr     r2, [r1]
    str     r2, [r5]

    ldr     r0, [r2]                                            @ R0 is new process SP; SP = OSTCBHighRdyPtr->StkPtr;
    ldmfd   r0!, {r4-r11}                                       @ Restore r4-11 from new process stack

#ifdef __ARMVFP__
    ldr     lr, [r0]
    adds    r0, r0, #0x4

    tst     lr, #0x10
    it      eq
    vldmiaeq r0!, {s16-s31}
#endif

    msr     PSP, r0                                             @ Load PSP with new process SP
    orr     lr, lr, #0x04                                       @ Ensure exception return uses process stack
    cpsie   i
    bx      lr                                                  @ Exception return will restore remaining context

    .end

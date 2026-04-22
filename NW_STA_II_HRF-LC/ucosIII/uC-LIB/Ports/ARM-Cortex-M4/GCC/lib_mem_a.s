/*
********************************************************************************************************
*                                                uC/LIB
*                                        CUSTOM LIBRARY MODULES
*
*                          (c) Copyright 2004-2011; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*
*               uC/LIB is provided in source form to registered licensees ONLY.  It is
*               illegal to distribute this source code to any third party unless you receive
*               written permission by an authorized Micrium representative.  Knowledge of
*               the source code may NOT be used to develop a similar product.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can contact us at www.micrium.com.
********************************************************************************************************
*/


/*
********************************************************************************************************
*
*                                     STANDARD MEMORY OPERATIONS
*
*                                           ARM-Cortex-M4
*                                           GCC Compiler
*
* Filename      : lib_mem_a.s
* Version       : V1.38.01.00
* Programmer(s) : JDH
*                 BAN
********************************************************************************************************
* Note(s)       : (1) NO compiler-supplied standard library functions are used in library or product software.
*
*                     (a) ALL standard library functions are implemented in the custom library modules :
*
*                         (1) \<Custom Library Directory>\lib*.*
*
*                         (2) \<Custom Library Directory>\Ports\<cpu>\<compiler>\lib*_a.*
*
*                               where
*                                       <Custom Library Directory>      directory path for custom library software
*                                       <cpu>                           directory name for specific processor (CPU)
*                                       <compiler>                      directory name for specific compiler
*
*                     (b) Product-specific library functions are implemented in individual products.
*
*                 (2) Assumes ARM CPU mode configured for Little Endian.
********************************************************************************************************
*/


    .syntax unified
    .cpu cortex-m4
    .fpu softvfp
    .thumb

    .global Mem_Copy

    .text

/*
********************************************************************************************************
*                                             Mem_Copy()
*
* Description : Copy data octets from one buffer to another buffer.
*
* Argument(s) : pdest       Pointer to destination memory buffer.
*
*               psrc        Pointer to source      memory buffer.
*
*               size        Number of data buffer octets to copy.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Null copies allowed (i.e. 0-octet size).
*
*               (2) Memory buffers NOT checked for overlapping.
*
*               (3) Modulo arithmetic is used to determine whether a memory buffer starts on a 'CPU_ALIGN'
*                   address boundary.
*
*               (4) ARM Cortex-M3 processors use a subset of the ARM Thumb-2 instruction set which does
*                   NOT support 16-bit conditional branch instructions but ONLY supports 8-bit conditional
*                   branch instructions.
*
*                   Therefore, branches exceeding 8-bit, signed, relative offsets :
*
*                   (a) CANNOT be implemented with     conditional branches; but ...
*                   (b) MUST   be implemented with non-conditional branches.
********************************************************************************************************
*/

@ void  Mem_Copy (void        *pdest,       @  ==>  R0
@                 void        *psrc,        @  ==>  R1
@                 CPU_SIZE_T   size)        @  ==>  R2

    .thumb_func
Mem_Copy:
    cmp     r0, #0
    bne     Mem_Copy_1
    bx      lr                          @ return if pdest == NULL

Mem_Copy_1:
    cmp     r1, #0
    bne     Mem_Copy_2
    bx      lr                          @ return if psrc  == NULL

Mem_Copy_2:
    cmp     r2, #0
    bne     Mem_Copy_3
    bx      lr                          @ return if size  == 0

Mem_Copy_3:
    stmfd   sp!, {r3-r12}               @ save registers on stack

Chk_Align_32:                               @ check if both dest & src 32-bit aligned
    and     r3, r0, #0x03
    and     r4, r1, #0x03
    cmp     r3, r4
    bne     Chk_Align_16                @ not 32-bit aligned, check for 16-bit alignment

    rsb     r3, r3, #0x04               @ compute 1-2-3 pre-copy bytes (to align to the next 32-bit boundary)
    and     r3, r3, #0x03

Pre_Copy_1:
    cmp     r3, #1                      @ copy 1-2-3 bytes (to align to the next 32-bit boundary)
    bcc     Copy_32_1                   @ start real 32-bit copy
    cmp     r2, #1                      @ check if any more data to copy
    bcs     Pre_Copy_1_Cont
    b       Mem_Copy_END                @           no more data to copy (see Note #4b)

Pre_Copy_1_Cont:
    ldrb    r4, [r1], #1
    strb    r4, [r0], #1
    sub     r3, r3, #1
    sub     r2, r2, #1
    b       Pre_Copy_1

Chk_Align_16:                               @ check if both dest & src 16-bit aligned
    and     r3, r0, #0x01
    and     r4, r1, #0x01
    cmp     r3, r4
    beq     Pre_Copy_2
    b       Copy_08_1                   @ not 16-bit aligned, start 8-bit copy (see Note #4b)

Pre_Copy_2:
    cmp     r3, #1                      @ copy 1 byte (to align to the next 16-bit boundary)
    bcc     Copy_16_1                   @ start real 16-bit copy

    ldrb    r4, [r1], #1
    strb    r4, [r0], #1
    sub     r3, r3, #1
    sub     r2, r2, #1
    b       Pre_Copy_2

Copy_32_1:
    cmp     r2, #(4*10*9)             @ Copy 9 chunks of 10 32-bit words (360 octets per loop)
    bcc     Copy_32_2
    ldmia   r1!, {r3-r12}
    stmia   r0!, {r3-r12}
    ldmia   r1!, {r3-r12}
    stmia   r0!, {r3-r12}
    ldmia   r1!, {r3-r12}
    stmia   r0!, {r3-r12}
    ldmia   r1!, {r3-r12}
    stmia   r0!, {r3-r12}
    ldmia   r1!, {r3-r12}
    stmia   r0!, {r3-r12}
    ldmia   r1!, {r3-r12}
    stmia   r0!, {r3-r12}
    ldmia   r1!, {r3-r12}
    stmia   r0!, {r3-r12}
    ldmia   r1!, {r3-r12}
    stmia   r0!, {r3-r12}
    sub     r2, r2, #(4*10*9)
    b       Copy_32_1

Copy_32_2:
    cmp     r2, #(4*10*1)             @ Copy chunks of 10 32-bit words (40 octets per loop)
    bcc     Copy_32_3
    ldmia   r1!, {r3-r12}
    stmia   r0!, {r3-r12}
    sub     r2, r2, #(4*10*1)
    b       Copy_32_2

Copy_32_3:
    cmp     r2, #(4*1*1)             @ Copy remaining 32-bit words
    bcc     Copy_16_1
    ldr     r3, [r1], #4
    str     r3, [r0], #4
    sub     r2, r2, #(4*1*1)
    b       Copy_32_3

Copy_16_1:
    cmp     r2, #(2*1*16)             @ Copy chunks of 16 16-bit words (32 bytes per loop)
    bcc     Copy_16_2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    sub     r2, r2, #(2*1*16)
    b       Copy_16_1

Copy_16_2:
    cmp     r2, #(2*1*1)             @ Copy remaining 16-bit words
    bcc     Copy_08_1
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    sub     r2, r2, #(2*1*1)
    b       Copy_16_2

Copy_08_1:
    cmp     r2, #(1*1*16)             @ Copy chunks of 16 8-bit words (16 bytes per loop)
    bcc     Copy_08_2
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    sub     r2, r2, #(1*1*16)
    b       Copy_08_1

Copy_08_2:
    cmp     r2, #(1*1*1)             @ Copy remaining 8-bit words
    bcc     Mem_Copy_END
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    sub     r2, r2, #(1*1*1)
    b       Copy_08_2

Mem_Copy_END:
    ldmfd   sp!, {r3-r12}               @ restore registers from stack
    bx      lr                          @ return

    .end

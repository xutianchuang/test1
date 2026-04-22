/*
 * GCC Linker Script for SRAM
 * Converted from IAR linker script (sram.icf)
 */

/* Entry Point */
ENTRY(Reset_Handler)

/* Memory Regions */
MEMORY
{
    /* Vector table region (separate from code) */
    RAM_VECTORS (rx) : ORIGIN = 0x10000000, LENGTH = 0x200  /* 0x10000000 to 0x100001FF */
    
    /* RAM region for code and read-only data */
    RAM_CODE (rwx) : ORIGIN = 0x10000200, LENGTH = 0x4FE00  /* 0x10000200 to 0x1004FFFF */
    
    /* RAM region for read-write data */
    RAM_RW (rwx)   : ORIGIN = 0x10050000, LENGTH = 0xAF500  /* 0x10050000 to 0x100FF4FF */
    
    /* HRF special region */
    RAM_HRF (rw)   : ORIGIN = 0x100FF500, LENGTH = 0xB00    /* 0x100FF500 to 0x100FFFFF */
}

/* Stack and Heap sizes */
__STACK_SIZE = 0x1000;
__HEAP_SIZE  = 0x1000;

/* Sections */
SECTIONS
{
    /* Vector table at fixed address */
    .vectors 0x10000000 :
    {
        _svectors = .;
        KEEP(*(.vectors))
        _evectors = .;
    } > RAM_VECTORS


    /* Code and read-only data in RAM_CODE region */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > RAM_CODE
    
    /* Store the end of code section for data initialization */
    _sidata = LOADADDR(.data);

    /* ARM exception handling sections */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > RAM_CODE

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > RAM_CODE

    /* Constructor/Destructor tables */
    .preinit_array :
    {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } > RAM_CODE

    .init_array :
    {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } > RAM_CODE

    .fini_array :
    {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } > RAM_CODE

    /* Initialized data section */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM_RW AT > RAM_CODE

    /* Uninitialized data section */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM_RW

    /* No-init section */
    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        *(.noinit)
        *(.noinit*)
        . = ALIGN(4);
    } > RAM_RW
    
    /* Provide end symbol for heap management */
    . = ALIGN(4);
    end = .;
    _end = .;
    __end__ = .;

    /* Heap section */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        __heap_start__ = .;
        . = . + __HEAP_SIZE;
        __heap_end__ = .;
    } > RAM_RW

    /* Stack section */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        __stack_start__ = .;
        . = . + __STACK_SIZE;
        . = ALIGN(8);
        __stack_end__ = .;
    } > RAM_RW

    /* Set stack pointer to end of stack */
    PROVIDE(__stack = __stack_end__);
    PROVIDE(_estack = __stack_end__);
    
    /* Provide additional symbols for compatibility */
    PROVIDE(__heap_limit = __heap_end__);
    /* PHY RX Buffer at fixed address in HRF region */
    .PHY_RX_Buffer_Sector 0x100FF500 (NOLOAD) :
    {
        KEEP(*(.PHY_RX_Buffer_Sector))
    } > RAM_HRF
}
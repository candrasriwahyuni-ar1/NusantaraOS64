/*
 * NusantaraOS64 - Linker Script
 * 
 * Defines memory layout for the kernel
 */

ENTRY(_start)

SECTIONS
{
    /* Kernel loaded at 1MB (after BIOS area) */
    . = 0x100000;
    
    /* Multiboot header must be first */
    .multiboot2 : ALIGN(8)
    {
        *(.multiboot2)
    }
    
    /* Text section (code) */
    .text : ALIGN(4K)
    {
        *(.text)
        *(.text.*)
    }
    
    /* Read-only data */
    .rodata : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata.*)
    }
    
    /* Initialized data */
    .data : ALIGN(4K)
    {
        *(.data)
        *(.data.*)
    }
    
    /* Uninitialized data (BSS) */
    _bss_start = .;
    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }
    _bss_end = .;
    
    /* Kernel heap follows BSS */
    .heap : ALIGN(4K)
    {
        . = . + 16M;  /* 16 MB heap */
    }
    
    /* Discard unwanted sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
        *(.eh_frame_hdr)
    }
}

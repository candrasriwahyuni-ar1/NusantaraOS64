; ============================================================================
; NusantaraOS64 - Kernel Entry Point (Assembly - NASM Syntax)
; 
; Entry point setelah UEFI handoff
; Setup awal: stack, GDT, IDT, paging, long mode
; ============================================================================

[BITS 64]
[EXTERN kernel_main]
[EXTERN interrupt_handler]
[EXTERN syscall_handler]

; ============================================================================
; Kernel Entry Point
; ============================================================================
[SECTION .text]
GLOBAL _start
GLOBAL gdt_ptr
GLOBAL idt_ptr
GLOBAL kernel_stack_top
GLOBAL context_switch
GLOBAL syscall_entry
GLOBAL interrupt_common

; Export ISR stubs
%assign i 0
%rep 32
    GLOBAL isr %+ i
%assign i i+1
%endrep

; Export IRQ stubs
%assign i 0
%rep 16
    GLOBAL irq %+ i
%assign i i+1
%endrep

_start:
    ; Disable interrupts
    CLI
    
    ; Setup stack
    MOV RSP, kernel_stack_top
    
    ; Clear BSS section
    LEA RDI, [_bss_start]
    LEA RCX, [_bss_end]
    SUB RCX, RDI
    XOR RAX, RAX
    REP STOSB
    
    ; Load GDT
    LGDT [gdt_ptr]
    
    ; Reload segment registers
    MOV AX, 0x10
    MOV DS, AX
    MOV ES, AX
    MOV FS, AX
    MOV GS, AX
    MOV SS, AX
    
    ; Enable SSE/AVX
    MOV RAX, CR0
    AND AX, 0xFFFB
    OR AX, 0x0002
    MOV CR0, RAX
    
    MOV RAX, CR4
    OR RAX, 3 << 9      ; OSFXSR | OSXMMEXCPT
    MOV CR4, RAX
    
    ; Enable FXSAVE/FXRSTOR
    XOR RAX, RAX
    FXRSTOR [RAX]
    
    ; Call kernel main
    CALL kernel_main
    
    ; Halt if kernel returns
.hang:
    HLT
    JMP .hang

; ============================================================================
; Interrupt Service Routines Stubs
; ============================================================================
%macro ISR 1
isr%1:
    PUSH QWORD 0              ; Error code placeholder
    PUSH QWORD %1             ; Interrupt number
    JMP interrupt_common
%endmacro

%macro ISR_ERR 1
isr_err%1:
    PUSH QWORD %1             ; Interrupt number (error code already pushed)
    JMP interrupt_common
%endmacro

%macro IRQ 2
irq%1:
    PUSH QWORD 0
    PUSH QWORD %2
    JMP interrupt_common
%endmacro

[SECTION .text]

; CPU Exceptions
ISR 0   ; Division by Zero
ISR 1   ; Debug
ISR 2   ; Non-Maskable Interrupt
ISR 3   ; Breakpoint
ISR 4   ; Overflow
ISR 5   ; Bound Range Exceeded
ISR 6   ; Invalid Opcode
ISR 7   ; Device Not Available
ISR_ERR 8   ; Double Fault
ISR 9   ; Coprocessor Segment Overrun
ISR_ERR 10  ; Invalid TSS
ISR_ERR 11  ; Segment Not Present
ISR_ERR 12  ; Stack-Segment Fault
ISR_ERR 13  ; General Protection Fault
ISR_ERR 14  ; Page Fault
ISR 15  ; Reserved
ISR 16  ; x87 FPU Error
ISR_ERR 17  ; Alignment Check
ISR 18  ; Machine Check
ISR 19  ; SIMD FPU Exception
ISR 20  ; Virtualization Exception
ISR_ERR 21  ; Control Protection Exception

; IRQs (remapped to 32-47)
IRQ 0, 32  ; Timer
IRQ 1, 33  ; Keyboard
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; ============================================================================
; System Call Handler (syscall instruction)
; ============================================================================
syscall_entry:
    SWAPGS
    PUSH RBP
    MOV RBP, RSP
    PUSH RAX
    PUSH RBX
    PUSH RCX
    PUSH RDX
    PUSH RSI
    PUSH RDI
    PUSH R8
    PUSH R9
    PUSH R10
    PUSH R11
    PUSH R12
    PUSH R13
    PUSH R14
    PUSH R15
    
    ; Set up argument for C handler: pointer to trap_frame on stack
    ; RSP currently points to the saved R15 (top of trap_frame)
    MOV RDI, RSP          ; ✅ FIXED: Pass stack pointer as argument
    CALL syscall_handler  ; Call C handler: void syscall_handler(trap_frame *tf)
    
    ; Restore registers
    POP R15
    POP R14
    POP R13
    POP R12
    POP R11
    POP R10
    POP R9
    POP R8
    POP RDI
    POP RSI
    POP RDX
    POP RCX
    POP RBX
    POP RAX
    POP RBP
    SWAPGS
    SYSRETQ

; ============================================================================
; Common interrupt handler
; ============================================================================
interrupt_common:
    PUSH RAX
    PUSH RBX
    PUSH RCX
    PUSH RDX
    PUSH RSI
    PUSH RDI
    PUSH R8
    PUSH R9
    PUSH R10
    PUSH R11
    PUSH R12
    PUSH R13
    PUSH R14
    PUSH R15
    
    ; Call C handler
    MOV RDI, RSP
    CALL interrupt_handler
    
    ; Restore registers
    POP R15
    POP R14
    POP R13
    POP R12
    POP R11
    POP R10
    POP R9
    POP R8
    POP RDI
    POP RSI
    POP RDX
    POP RCX
    POP RBX
    POP RAX
    
    ; Return from interrupt
    ADD RSP, 16        ; Remove error code and interrupt number
    IRETQ

; ============================================================================
; Context Switch Function
; ============================================================================
context_switch:
    ; Save old context (rdi = &old_ctx)
    MOV [RDI], RBX
    MOV [RDI + 8], RBP
    MOV [RDI + 16], R12
    MOV [RDI + 24], R13
    MOV [RDI + 32], R14
    MOV [RDI + 40], R15
    MOV [RDI + 48], RSP
    LEA RAX, [.return_addr]
    MOV [RDI + 56], RAX
    
    ; Load new context (rsi = &new_ctx)
    MOV RAX, [RSI + 56]
    MOV RSP, [RSI + 48]
    MOV R15, [RSI + 40]
    MOV R14, [RSI + 32]
    MOV R13, [RSI + 24]
    MOV R12, [RSI + 16]
    MOV RBP, [RSI + 8]
    MOV RBX, [RSI]
    
.return_addr:
    RET

; ============================================================================
; Data Section
; ============================================================================
[SECTION .data]
ALIGN 16

; GDT Entries
gdt_null:
    DQ 0
    DQ 0

gdt_code:
    DW 0xFFFF              ; Limit (low)
    DW 0                   ; Base (low)
    DB 0                   ; Base (middle)
    DB 0x9A                ; Access (exec, read, non-system, DPL=0, present)
    DB 0x20                ; Granularity (4KB, 64-bit)
    DB 0                   ; Base (high)

gdt_data:
    DW 0xFFFF
    DW 0
    DB 0
    DB 0x92                ; Access (read/write, non-system, DPL=0, present)
    DB 0x00
    DB 0

gdt_user_code:
    DW 0xFFFF
    DW 0
    DB 0
    DB 0xFA                ; Access (exec, read, non-system, DPL=3, present)
    DB 0x20
    DB 0

gdt_user_data:
    DW 0xFFFF
    DW 0
    DB 0
    DB 0xF2                ; Access (read/write, non-system, DPL=3, present)
    DB 0x00
    DB 0

gdt_ptr:
    DW gdt_end - gdt_null - 1
    DQ gdt_null

gdt_end:

; IDT (placeholder, initialized at runtime)
ALIGN 16
idt_entries:
    TIMES 256 * 16 DB 0

idt_ptr:
    DW 256 * 16 - 1
    DQ idt_entries

; Kernel Stack
[SECTION .bss]
ALIGN 16
kernel_stack:
    RESB 65536            ; 64 KB stack
kernel_stack_top:

_bss_start:
_bss_end:

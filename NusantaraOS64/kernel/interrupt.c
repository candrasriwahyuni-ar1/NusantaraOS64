/*
 * NusantaraOS64 - Interrupt Management
 * 
 * Fase 2: Trap Table dan Interrupt Handling
 * Setup IDT, PIC remapping, dan interrupt handlers
 */

#include "../include/nusantara.h"

/* External interrupt handler declarations */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/* Forward declaration for timer_tick from task.c */
extern void timer_tick(void);

/* IDT entries - defined in assembly */
extern idt_entry_t idt_entries[256];

/*
 * Set an IDT entry
 */
static void set_idt_gate(int n, u64 handler) {
    idt_entries[n].offset_low = handler & 0xFFFF;
    idt_entries[n].selector = GDT_KERNEL_CODE;
    idt_entries[n].ist = 0;
    idt_entries[n].type_attr = 0x8E;  /* Present, DPL=0, Interrupt Gate */
    idt_entries[n].offset_middle = (handler >> 16) & 0xFFFF;
    idt_entries[n].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt_entries[n].reserved = 0;
}

/*
 * Remap PIC to use interrupts 32-47 instead of 0-15
 */
static void remap_pic(void) {
    /* ICW1 - Start initialization */
    __asm__ volatile("outb %0, $0x20" :: "a"((u8)0x11));
    __asm__ volatile("outb %0, $0xA0" :: "a"((u8)0x11));
    
    /* ICW2 - Set vector offsets */
    __asm__ volatile("outb %0, $0x21" :: "a"((u8)0x20));  /* Master: 32-39 */
    __asm__ volatile("outb %0, $0xA1" :: "a"((u8)0x28));  /* Slave: 40-47 */
    
    /* ICW3 - Configure cascading */
    __asm__ volatile("outb %0, $0x21" :: "a"((u8)0x04));  /* Master has slave on IRQ2 */
    __asm__ volatile("outb %0, $0xA1" :: "a"((u8)0x02));  /* Slave connected to IRQ2 */
    
    /* ICW4 - Set mode */
    __asm__ volatile("outb %0, $0x21" :: "a"((u8)0x01));
    __asm__ volatile("outb %0, $0xA1" :: "a"((u8)0x01));
    
    /* Mask all interrupts initially */
    __asm__ volatile("outb %0, $0x21" :: "a"((u8)0xFF));
    __asm__ volatile("outb %0, $0xA1" :: "a"((u8)0xFF));
}

/*
 * Send End of Interrupt signal to PIC
 */
static void send_eoi(int irq) {
    if (irq >= 8) {
        __asm__ volatile("outb %0, $0xA0" :: "a"((u8)0x20));
    }
    __asm__ volatile("outb %0, $0x20" :: "a"((u8)0x20));
}

/*
 * C interrupt handler called from assembly
 */
void interrupt_handler(trap_frame_t *frame) {
    u64 interrupt_num = frame->rsp;  /* Interrupt number pushed by assembly */
    
    switch (interrupt_num) {
        /* CPU Exceptions */
        case 0:
            console_print("\n#DE Division Error\n");
            break;
        case 3:
            console_print("\n#BP Breakpoint\n");
            break;
        case 6:
            console_print("\n#UD Invalid Opcode\n");
            break;
        case 8:
            console_print("\n#DF Double Fault\n");
            break;
        case 13:
            console_print("\n#GP General Protection Fault\n");
            break;
        case 14:
            console_print("\n#PF Page Fault\n");
            break;
            
        /* Timer Interrupt */
        case 32:
            timer_tick();
            send_eoi(0);
            break;
            
        /* Keyboard Interrupt */
        case 33:
            /* keyboard_handler(); */
            send_eoi(1);
            break;
            
        default:
            if (interrupt_num >= 32 && interrupt_num < 48) {
                send_eoi(interrupt_num - 32);
            }
            break;
    }
}

/*
 * Initialize interrupt system
 */
void interrupt_init(void) {
    /* Remap PIC */
    remap_pic();
    
    /* Set up exception handlers */
    set_idt_gate(0, (u64)isr0);
    set_idt_gate(1, (u64)isr1);
    set_idt_gate(2, (u64)isr2);
    set_idt_gate(3, (u64)isr3);
    set_idt_gate(4, (u64)isr4);
    set_idt_gate(5, (u64)isr5);
    set_idt_gate(6, (u64)isr6);
    set_idt_gate(7, (u64)isr7);
    set_idt_gate(8, (u64)isr8);
    set_idt_gate(9, (u64)isr9);
    set_idt_gate(10, (u64)isr10);
    set_idt_gate(11, (u64)isr11);
    set_idt_gate(12, (u64)isr12);
    set_idt_gate(13, (u64)isr13);
    set_idt_gate(14, (u64)isr14);
    set_idt_gate(15, (u64)isr15);
    set_idt_gate(16, (u64)isr16);
    set_idt_gate(17, (u64)isr17);
    set_idt_gate(18, (u64)isr18);
    set_idt_gate(19, (u64)isr19);
    set_idt_gate(20, (u64)isr20);
    set_idt_gate(21, (u64)isr21);
    
    /* Set up IRQ handlers */
    set_idt_gate(32, (u64)irq0);
    set_idt_gate(33, (u64)irq1);
    set_idt_gate(34, (u64)irq2);
    set_idt_gate(35, (u64)irq3);
    set_idt_gate(36, (u64)irq4);
    set_idt_gate(37, (u64)irq5);
    set_idt_gate(38, (u64)irq6);
    set_idt_gate(39, (u64)irq7);
    set_idt_gate(40, (u64)irq8);
    set_idt_gate(41, (u64)irq9);
    set_idt_gate(42, (u64)irq10);
    set_idt_gate(43, (u64)irq11);
    set_idt_gate(44, (u64)irq12);
    set_idt_gate(45, (u64)irq13);
    set_idt_gate(46, (u64)irq14);
    set_idt_gate(47, (u64)irq15);
    
    /* Load IDT */
    idt_ptr_t idt_ptr;
    idt_ptr.limit = 256 * sizeof(idt_entry_t) - 1;
    idt_ptr.base = (u64)&idt_entries;
    
    __asm__ volatile("lidt %0" :: "m"(idt_ptr));
    
    console_print("[INT] Interrupt system initialized\n");
}

/*
 * Enable interrupts
 */
void enable_interrupts(void) {
    __asm__ volatile("sti");
}

/*
 * Disable interrupts
 */
void disable_interrupts(void) {
    __asm__ volatile("cli");
}

/*
 * Halt CPU until next interrupt
 */
void halt_cpu(void) {
    __asm__ volatile("hlt");
}

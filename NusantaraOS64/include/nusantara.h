/*
 * NusantaraOS64 - Main Kernel Header
 * 
 * Prinsip Desain:
 * 1. Limited Direct Execution (LDE)
 * 2. Pemisahan Kebijakan dan Mekanisme
 * 3. Virtualisasi Transparan
 */

#ifndef _NUSANTARA_H
#define _NUSANTARA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

/* Forward declaration for pid_t */
typedef long pid_t;

/* ============================================
 * Tipe Data Fundamental
 * ============================================ */
typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;
typedef int8_t      s8;
typedef int16_t     s16;
typedef int32_t     s32;
typedef int64_t     s64;
typedef uintptr_t   addr_t;
typedef size_t      size_t;

/* ============================================
 * Konstanta Arsitektur x86_64
 * ============================================ */
#define KERNEL_BASE         0xFFFFFFFF80000000ULL
#define PAGE_SIZE           4096
#define PAGE_SHIFT          12
#define STACK_SIZE          (PAGE_SIZE * 4)

/* Segment Selectors */
#define GDT_KERNEL_CODE     0x08
#define GDT_KERNEL_DATA     0x10
#define GDT_USER_CODE       0x18
#define GDT_USER_DATA       0x20

/* CR0 Bits */
#define CR0_PROTECTED_MODE  0x00000001
#define CR0_MONITOR_COPROC  0x00000002
#define CR0_EMULATION       0x00000004
#define CR0_PAGING          0x80000000

/* CR3 Bits */
#define CR3_PAGE_WRITE_BACK 0x00000008
#define CR3_PAGE_CACHE_DIS  0x00000010

/* CR4 Bits */
#define CR4_PSE             0x00000010  /* Page Size Extension */
#define CR4_PAE             0x00000020  /* Physical Address Extension */
#define CR4_SMEP            0x00100000  /* Supervisor Mode Execution Prevention */
#define CR4_SMAP            0x00200000  /* Supervisor Mode Access Prevention */

/* EFER MSR */
#define EFER_SCE            0x00000001  /* System Call Extensions */
#define EFER_LME            0x00000100  /* Long Mode Enable */
#define EFER_NXE            0x00000800  /* No-Execute Enable */
#define EFER_LMA            0x00001000  /* Long Mode Active */

/* ============================================
 * Status Proses
 * ============================================ */
typedef enum {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

/* ============================================
 * Struktur Process Control Block (PCB)
 * ============================================ */
typedef struct process {
    u64 pid;
    u64 ppid;
    process_state_t state;
    
    /* Context untuk context switch */
    u64 rip;
    u64 rsp;
    u64 rbp;
    u64 rbx;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;
    u64 rflags;
    u64 cr3;
    
    /* Informasi memori */
    u64 *page_table;
    addr_t kernel_stack;
    addr_t user_stack;
    
    /* Parent-Child relationship */
    struct process *parent;
    struct process **children;
    u32 child_count;
    
    /* Exit status */
    int exit_status;
    
    /* Nama proses */
    char name[64];
} process_t;

/* ============================================
 * Trap Frame (untuk menyimpan konteks saat interrupt)
 * ============================================ */
typedef struct {
    /* Pushed by hardware */
    u64 error_code;
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
    
    /* Pushed by software (syscall.c) */
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rsi;
    u64 rdi;
    u64 rbp;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;
} trap_frame_t;

/* ============================================
 * Descriptor Table Structures
 * ============================================ */
typedef struct {
    u16 limit_low;
    u16 base_low;
    u8 base_middle;
    u8 access;
    u8 granularity;
    u8 base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    u16 limit;
    u64 base;
} __attribute__((packed)) gdt_ptr_t;

typedef struct {
    u16 offset_low;
    u16 selector;
    u8 ist;
    u8 type_attr;
    u16 offset_middle;
    u32 offset_high;
    u32 reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    u16 limit;
    u64 base;
} __attribute__((packed)) idt_ptr_t;

/* ============================================
 * Page Table Structures (4-level paging)
 * ============================================ */
typedef struct {
    u64 present       : 1;
    u64 writable      : 1;
    u64 user          : 1;
    u64 write_through : 1;
    u64 cache_disable : 1;
    u64 accessed      : 1;
    u64 dirty         : 1;
    u64 large_page    : 1;  /* For PDPTE and PDE */
    u64 global        : 1;
    u64 available     : 3;
    u64 frame         : 52; /* Physical frame number */
    u64 reserved      : 11;
    u64 nx            : 1;  /* No-Execute */
} __attribute__((packed)) page_entry_t;

#define PML4_ENTRIES    512
#define PDPT_ENTRIES    512
#define PD_ENTRIES      512
#define PT_ENTRIES      512

/* ============================================
 * System Call Numbers
 * ============================================ */
#define SYS_EXIT        1
#define SYS_FORK        2
#define SYS_READ        3
#define SYS_WRITE       4
#define SYS_OPEN        5
#define SYS_CLOSE       6
#define SYS_WAITPID     7
#define SYS_EXEC        8
#define SYS_GETPID      9
#define SYS_YIELD       10
#define SYS_MMAP        11
#define SYS_MUNMAP      12

/* ============================================
 * VGA/Framebuffer Constants
 * ============================================ */
#define FRAMEBUFFER_WIDTH     1920
#define FRAMEBUFFER_HEIGHT    1080
#define FRAMEBUFFER_BPP       32
#define VGA_WIDTH             80
#define VGA_HEIGHT            25

/* ============================================
 * Function Prototypes (Kernel Core)
 * ============================================ */

/* boot/main.c */
void kernel_main(void);

/* kernel/memory.c */
void memory_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void memset(void *dest, u8 val, size_t count);
void memcpy(void *dest, const void *src, size_t count);
int memcmp(const void *s1, const void *s2, size_t n);

/* kernel/paging.c */
void paging_init(void);
void map_page(u64 virt, u64 phys, u64 flags);
void unmap_page(u64 virt);
u64 translate_address(u64 virt);
void flush_tlb(void);
process_t *create_process_address_space(void);
void switch_address_space(process_t *proc);

/* kernel/task.c */
void task_init(void);
process_t *task_create(const char *name, void (*entry)(void));
void task_switch(trap_frame_t *frame);
void schedule(void);
void yield(void);
process_t *get_current_process(void);
void set_current_process(process_t *proc);

/* kernel/syscall.c */
void syscall_init(void);
void syscall_handler(trap_frame_t *frame);
s64 sys_fork(void);
s64 sys_exec(const char *path, char **argv);
s64 sys_waitpid(pid_t pid, int *status);
void sys_exit(int status);
void sys_yield(void);
s64 sys_getpid(void);
s64 sys_write(int fd, const char *buf, size_t count);

/* kernel/interrupt.c */
void interrupt_init(void);
void enable_interrupts(void);
void disable_interrupts(void);
void halt_cpu(void);

/* kernel/console.c */
void console_init(void);
void console_putchar(char c);
void console_print(const char *str);
void console_printf(const char *fmt, ...);
void console_clear(void);

/* drivers/framebuffer.c */
void framebuffer_init(void);
void framebuffer_clear(u32 color);
void framebuffer_put_pixel(u32 x, u32 y, u32 color);
void framebuffer_draw_rect(u32 x, u32 y, u32 w, u32 h, u32 color);
void framebuffer_draw_string(const char *str, u32 x, u32 y, u32 color);

/* drivers/keyboard.c */
void keyboard_init(void);
char keyboard_getchar(void);
bool keyboard_available(void);

/* lib/string.c */
size_t strlen(const char *str);
char *strcpy(char *dest, const char *src);
int strcmp(const char *s1, const char *s2);
char *itoa(int value, char *str, int base);

#endif /* _NUSANTARA_H */

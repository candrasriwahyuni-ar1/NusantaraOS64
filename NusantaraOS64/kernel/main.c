/*
 * NusantaraOS64 - Kernel Main Entry Point
 * 
 * Initializes all subsystems and starts the first process
 */

#include "../include/nusantara.h"

/* External functions from boot */
extern void syscall_init_asm(void);

/*
 * Kernel main entry point
 * Called from boot/kernel_entry.S after basic setup
 */
void kernel_main(void) {
    /* Initialize console first for debug output */
    console_init();
    
    console_print("\n");
    console_print("  _   _                      ____  _             _        \n");
    console_print(" | \\ | | ___  _ __ ___   ___|  _ \\| |_ __ _  ___| | _____ \n");
    console_print(" |  \\| |/ _ \\| '_ ` _ \\ / _ \\ |_) | __/ _` |/ __| |/ / __|\n");
    console_print(" | |\\  | (_) | | | | | |  __/  __/| || (_| | (__|   <\\__ \\\n");
    console_print(" |_| \\_|\\___/|_| |_| |_|\\___|_|    \\__\\__,_|\\___|_|\\_\\___/\n");
    console_print("                        64-bit Edition                     \n\n");
    
    console_print("[KERNEL] NusantaraOS64 starting...\n");
    
    /* Initialize memory manager */
    console_print("[KERNEL] Initializing memory manager...\n");
    memory_init();
    
    /* Initialize paging */
    console_print("[KERNEL] Initializing paging system...\n");
    paging_init();
    
    /* Initialize interrupt system */
    console_print("[KERNEL] Initializing interrupts...\n");
    interrupt_init();
    
    /* Initialize task scheduler */
    console_print("[KERNEL] Initializing task scheduler...\n");
    task_init();
    
    /* Initialize framebuffer */
    console_print("[KERNEL] Initializing framebuffer...\n");
    framebuffer_init();
    
    /* Initialize keyboard */
    console_print("[KERNEL] Initializing keyboard...\n");
    keyboard_init();
    
    /* Enable interrupts */
    console_print("[KERNEL] Enabling interrupts...\n");
    enable_interrupts();
    
    console_print("\n[KERNEL] NusantaraOS64 ready!\n");
    console_print("========================================\n\n");
    
    /* Create first user process */
    console_print("[SCHED] Creating initial user process...\n");
    
    /* Example: Create a simple test process */
    process_t *test_proc = task_create("test", NULL);
    if (test_proc) {
        console_printf("[SCHED] Created process: %s (PID: %d)\n", test_proc->name, test_proc->pid);
    }
    
    /* Main kernel loop (idle) */
    console_print("\n[KERNEL] Entering kernel idle loop...\n");
    console_print("Type commands or press keys to interact.\n\n");
    
    while (1) {
        /* Check for keyboard input */
        if (keyboard_available()) {
            char c = keyboard_getchar();
            console_putchar(c);
            
            /* Simple command handling */
            if (c == 'h') {
                console_print("\n--- NusantaraOS64 Help ---\n");
                console_print("Commands:\n");
                console_print("  h - Show this help\n");
                console_print("  p - Show process info\n");
                console_print("  c - Clear screen\n");
                console_print("  t - Create test process\n");
                console_print("  q - Quit (halt)\n");
                console_print("--------------------------\n");
            } else if (c == 'p') {
                process_t *proc = get_current_process();
                if (proc) {
                    console_printf("\nCurrent Process: %s (PID: %d, State: %d)\n",
                        proc->name, proc->pid, proc->state);
                }
            } else if (c == 'c') {
                console_clear();
            } else if (c == 't') {
                process_t *new_proc = task_create("worker", NULL);
                if (new_proc) {
                    console_printf("\nCreated new process: %s (PID: %d)\n",
                        new_proc->name, new_proc->pid);
                }
            } else if (c == 'q') {
                console_print("\nShutting down...\n");
                halt_cpu();
            }
        }
        
        /* Yield to allow other processes to run */
        yield();
        
        /* Halt until next interrupt */
        halt_cpu();
    }
}

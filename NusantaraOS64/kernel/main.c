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
    console_print("[SCHED] Creating initial processes...\n");
    
    /* Example: Create a simple test process */
    process_t *test_proc = task_create("test", NULL);
    if (test_proc) {
        console_printf("[SCHED] Created process: %s (PID: %d)\n", test_proc->name, test_proc->pid);
    }
    
    /* Create IPC demo processes - Fase 7 */
    console_print("[IPC] Initializing IPC subsystem...\n");
    process_t *producer = task_create("producer", NULL);
    process_t *consumer = task_create("consumer", NULL);
    if (producer) {
        console_printf("[IPC] Created producer process (PID: %d)\n", producer->pid);
    }
    if (consumer) {
        console_printf("[IPC] Created consumer process (PID: %d)\n", consumer->pid);
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
                console_print("  i - Test IPC (mutex, sem, shm)\n");
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
            } else if (c == 'i') {
                /* IPC Test - Fase 7 */
                console_print("\n[IPC] Testing mutex...\n");
                s64 mutex_id = sys_mutex_create();
                if (mutex_id > 0) {
                    console_printf("[IPC] Created mutex ID: %d\n", mutex_id);
                    sys_mutex_lock(mutex_id);
                    console_print("[IPC] Mutex locked\n");
                    sys_mutex_unlock(mutex_id);
                    console_print("[IPC] Mutex unlocked\n");
                }
                
                console_print("\n[IPC] Testing semaphore...\n");
                s64 sem_id = sys_sem_create(3);
                if (sem_id > 0) {
                    console_printf("[IPC] Created semaphore ID: %d (value=3)\n", sem_id);
                }
                
                console_print("\n[IPC] Testing shared memory...\n");
                s64 shm_id = sys_shm_create("test_shm", 4096);
                if (shm_id > 0) {
                    console_printf("[IPC] Created shared memory ID: %d\n", shm_id);
                    s64 addr = sys_shm_attach(shm_id);
                    console_printf("[IPC] Attached at virtual address: 0x%x\n", addr);
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

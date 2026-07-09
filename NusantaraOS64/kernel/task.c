/*
 * NusantaraOS64 - Task/Process Management
 * 
 * Fase 3: Manajemen Proses & API UNIX-Like
 * Fase 7: IPC Integration untuk blocking/wakeup
 * Implementasi PCB, context switch, dan scheduler
 */

#include "../include/nusantara.h"

#define MAX_PROCESSES   256

static process_t *processes[MAX_PROCESSES];
static process_t *current_process = NULL;
static u64 next_pid = 1;
static u32 process_count = 0;
static u64 system_ticks = 0;

/* Forward declaration */
void timer_tick(void);
void unblock_process(pid_t pid);

/* External IPC init */
extern void ipc_init(void);

/*
 * Initialize task management system
 */
void task_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i] = NULL;
    }
    
    /* Initialize IPC subsystem - Fase 7 */
    ipc_init();
    
    /* Create initial kernel process (idle) */
    current_process = task_create("idle", NULL);
    if (current_process) {
        current_process->state = PROCESS_RUNNING;
        current_process->start_time = system_ticks;
    }
}

/*
 * Create a new process/task
 */
process_t *task_create(const char *name, void (*entry)(void)) {
    if (process_count >= MAX_PROCESSES) {
        return NULL;
    }
    
    /* Allocate PCB */
    process_t *proc = (process_t *)kmalloc(sizeof(process_t));
    if (!proc) {
        return NULL;
    }
    
    memset(proc, 0, sizeof(process_t));
    
    /* Assign PID */
    proc->pid = next_pid++;
    process_count++;
    
    /* Set name */
    if (name) {
        int len = strlen(name);
        if (len > 63) len = 63;
        memcpy(proc->name, name, len);
        proc->name[len] = '\0';
    } else {
        proc->name[0] = '\0';
    }
    
    /* Allocate kernel stack */
    proc->kernel_stack = (addr_t)kmalloc(STACK_SIZE);
    if (!proc->kernel_stack) {
        kfree(proc);
        process_count--;
        return NULL;
    }
    
    /* Set up initial context */
    proc->rsp = proc->kernel_stack + STACK_SIZE;
    proc->rbp = proc->rsp;
    proc->rip = (u64)entry;
    proc->rflags = 0x202;  /* Interrupts enabled */
    
    /* Create address space */
    process_t *addr_space = create_process_address_space();
    if (addr_space) {
        proc->page_table = addr_space->page_table;
        proc->cr3 = (u64)addr_space->page_table - KERNEL_BASE;
        kfree(addr_space);
    }
    
    /* Set state */
    proc->state = PROCESS_READY;
    
    /* Add to process table */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] == NULL) {
            processes[i] = proc;
            break;
        }
    }
    
    return proc;
}

/*
 * Context switch implementation (called from assembly)
 */
extern void context_switch(u64 *old_ctx, u64 *new_ctx);

/*
 * Schedule next process (round-robin)
 */
void schedule(void) {
    if (!current_process) {
        return;
    }
    
    /* Save current process state */
    current_process->state = PROCESS_READY;
    
    /* Find next ready process */
    process_t *next = NULL;
    u32 start_idx = 0;
    
    /* Find current process index */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] == current_process) {
            start_idx = (i + 1) % MAX_PROCESSES;
            break;
        }
    }
    
    /* Round-robin search */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        u32 idx = (start_idx + i) % MAX_PROCESSES;
        if (processes[idx] && processes[idx]->state == PROCESS_READY) {
            next = processes[idx];
            break;
        }
    }
    
    /* If no other process, continue with current */
    if (!next) {
        next = current_process;
    }
    
    /* Switch to next process */
    next->state = PROCESS_RUNNING;
    
    /* Perform context switch */
    if (next != current_process) {
        /* Switch address space */
        switch_address_space(next);
        
        /* Switch stack and registers */
        context_switch(
            (u64 *)&current_process->rbx,
            (u64 *)&next->rbx
        );
        
        current_process = next;
    }
}

/*
 * Yield CPU voluntarily
 */
void yield(void) {
    disable_interrupts();
    schedule();
    enable_interrupts();
}

/*
 * Get current running process
 */
process_t *get_current_process(void) {
    return current_process;
}

/*
 * Set current process (used during context switch)
 */
void set_current_process(process_t *proc) {
    current_process = proc;
}

/*
 * Handle timer interrupt for preemptive scheduling
 */
void timer_tick(void) {
    system_ticks++;
    
    /* Check for sleeping processes - Fase 7 */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] && processes[i]->state == PROCESS_BLOCKED) {
            if (processes[i]->sleep_until > 0) {
                processes[i]->sleep_until--;
                if (processes[i]->sleep_until == 0) {
                    processes[i]->state = PROCESS_READY;
                }
            }
        }
    }
    
    /* Preempt every 10 ticks (adjustable) */
    static u32 tick_count = 0;
    tick_count++;
    
    if (tick_count >= 10) {
        tick_count = 0;
        yield();
    }
}

/*
 * Unblock a process by PID (for IPC wakeups) - Fase 7
 */
void unblock_process(pid_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] && processes[i]->pid == (u64)pid) {
            if (processes[i]->state == PROCESS_BLOCKED) {
                processes[i]->state = PROCESS_READY;
                return;
            }
        }
    }
}

/*
 * Get process by PID - Fase 7 utility
 */
process_t *get_process_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] && processes[i]->pid == (u64)pid) {
            return processes[i];
        }
    }
    return NULL;
}

/*
 * Get system ticks - Fase 7 utility
 */
u64 get_system_ticks(void) {
    return system_ticks;
}

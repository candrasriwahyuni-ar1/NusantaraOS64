/*
 * NusantaraOS64 - System Call Implementation
 * 
 * Fase 2 & 3: Trap mechanism dan UNIX-like API
 * sys_fork, sys_exec, sys_wait, sys_exit, sys_write
 */

#include "../include/nusantara.h"

/* External context switch function */
extern void context_switch(u64 *old_ctx, u64 *new_ctx);

/* External IPC functions */
extern void ipc_init(void);
extern s64 sys_mutex_create(void);
extern s64 sys_mutex_lock(s64 mutex_id);
extern s64 sys_mutex_unlock(s64 mutex_id);
extern s64 sys_sem_create(s32 initial_value);
extern s64 sys_sem_wait(s64 sem_id);
extern s64 sys_sem_post(s64 sem_id);
extern s64 sys_msg_send(pid_t dest, u32 type, const void *data, u32 size);
extern s64 sys_msg_recv(pid_t src, u32 type, void *buf, u32 max_size);
extern s64 sys_shm_create(const char *name, u64 size);
extern s64 sys_shm_attach(s64 shm_id);
extern s64 sys_shm_detach(s64 shm_id);
extern s64 sys_sleep(u64 ms);

/*
 * System call handler
 * Called from assembly syscall_entry
 */
void syscall_handler(trap_frame_t *frame) {
    u64 syscall_num = frame->rax;
    s64 ret = 0;
    
    switch (syscall_num) {
        case SYS_EXIT:
            sys_exit((int)frame->rdi);
            break;
            
        case SYS_FORK:
            ret = sys_fork();
            break;
            
        case SYS_READ:
            ret = sys_read((int)frame->rdi, (void *)frame->rsi, (size_t)frame->rdx);
            break;
            
        case SYS_WRITE:
            ret = sys_write((int)frame->rdi, (const char *)frame->rsi, (size_t)frame->rdx);
            break;
            
        case SYS_OPEN:
            ret = sys_open((const char *)frame->rdi, (int)frame->rsi);
            break;
            
        case SYS_CLOSE:
            ret = sys_close((int)frame->rdi);
            break;
            
        case SYS_WAITPID:
            ret = sys_waitpid((pid_t)frame->rdi, (int *)frame->rsi);
            break;
            
        case SYS_EXEC:
            ret = sys_exec((const char *)frame->rdi, (char **)frame->rsi);
            break;
            
        case SYS_GETPID:
            ret = sys_getpid();
            break;
            
        case SYS_YIELD:
            sys_yield();
            break;
            
        /* IPC & Synchronization syscalls - Fase 7 */
        case SYS_MUTEX_CREATE:
            ret = sys_mutex_create();
            break;
            
        case SYS_MUTEX_LOCK:
            ret = sys_mutex_lock((s64)frame->rdi);
            break;
            
        case SYS_MUTEX_UNLOCK:
            ret = sys_mutex_unlock((s64)frame->rdi);
            break;
            
        case SYS_SEM_CREATE:
            ret = sys_sem_create((s32)frame->rdi);
            break;
            
        case SYS_SEM_WAIT:
            ret = sys_sem_wait((s64)frame->rdi);
            break;
            
        case SYS_SEM_POST:
            ret = sys_sem_post((s64)frame->rdi);
            break;
            
        case SYS_MSG_SEND:
            ret = sys_msg_send((pid_t)frame->rdi, (u32)frame->rsi, 
                               (const void *)frame->rdx, (u32)frame->r10);
            break;
            
        case SYS_MSG_RECV:
            ret = sys_msg_recv((pid_t)frame->rdi, (u32)frame->rsi,
                               (void *)frame->rdx, (u32)frame->r10);
            break;
            
        case SYS_SHM_CREATE:
            ret = sys_shm_create((const char *)frame->rdi, (u64)frame->rsi);
            break;
            
        case SYS_SHM_ATTACH:
            ret = sys_shm_attach((s64)frame->rdi);
            break;
            
        case SYS_SHM_DETACH:
            ret = sys_shm_detach((s64)frame->rdi);
            break;
            
        case SYS_SLEEP:
            ret = sys_sleep((u64)frame->rdi);
            break;
            
        default:
            console_printf("Unknown syscall: %d\n", syscall_num);
            ret = -1;
            break;
    }
    
    /* Return value in RAX */
    frame->rax = ret;
}

/*
 * sys_fork: Create a child process
 * Returns: PID of child (parent), 0 (child), or -1 on error
 */
s64 sys_fork(void) {
    process_t *parent = get_current_process();
    if (!parent) {
        return -1;
    }
    
    /* Create child process */
    process_t *child = task_create(parent->name, NULL);
    if (!child) {
        return -1;
    }
    
    /* Copy parent's address space */
    /* In a real implementation, this would use copy-on-write */
    
    /* Copy register context from parent */
    child->rip = parent->rip;
    child->rbp = parent->rbp;
    child->rbx = parent->rbx;
    child->r12 = parent->r12;
    child->r13 = parent->r13;
    child->r14 = parent->r14;
    child->r15 = parent->r15;
    child->rflags = parent->rflags;
    
    /* Set up parent-child relationship */
    child->ppid = parent->pid;
    child->parent = parent;
    
    /* Add child to parent's children list */
    if (parent->child_count < 64) {
        if (!parent->children) {
            parent->children = (process_t **)kmalloc(sizeof(process_t *) * 64);
        }
        if (parent->children) {
            parent->children[parent->child_count++] = child;
        }
    }
    
    /* Child returns 0, parent returns child PID */
    /* This is handled by setting different return values in their contexts */
    
    child->state = PROCESS_READY;
    
    return (s64)child->pid;
}

/*
 * sys_exec: Execute a new program
 * Returns: 0 on success, -1 on error
 */
s64 sys_exec(const char *path, char **argv) {
    process_t *proc = get_current_process();
    if (!proc || !path) {
        return -1;
    }
    
    console_printf("EXEC: %s\n", path);
    
    /* In a real implementation, this would:
     * 1. Load the executable from disk
     * 2. Parse ELF format
     * 3. Map segments into address space
     * 4. Set up stack with argv/envp
     * 5. Jump to entry point
     */
    
    /* For now, just update process name */
    int len = strlen(path);
    if (len > 63) len = 63;
    memcpy(proc->name, path, len);
    proc->name[len] = '\0';
    
    return 0;
}

/*
 * sys_waitpid: Wait for child process to terminate
 * Returns: PID of terminated child, or -1 on error
 */
s64 sys_waitpid(pid_t pid, int *status) {
    process_t *proc = get_current_process();
    if (!proc) {
        return -1;
    }
    
    /* If pid == -1, wait for any child */
    if (pid == -1) {
        for (u32 i = 0; i < proc->child_count; i++) {
            process_t *child = proc->children[i];
            if (child && child->state == PROCESS_TERMINATED) {
                if (status) {
                    *status = child->exit_status;
                }
                
                /* Clean up child */
                kfree(child);
                proc->children[i] = NULL;
                
                return (s64)child->pid;
            }
        }
        
        /* No terminated child, block current process */
        proc->state = PROCESS_BLOCKED;
        yield();
        return -1;
    }
    
    /* Wait for specific child */
    for (u32 i = 0; i < proc->child_count; i++) {
        process_t *child = proc->children[i];
        if (child && child->pid == (u64)pid) {
            if (child->state == PROCESS_TERMINATED) {
                if (status) {
                    *status = child->exit_status;
                }
                
                kfree(child);
                proc->children[i] = NULL;
                
                return (s64)child->pid;
            } else {
                proc->state = PROCESS_BLOCKED;
                yield();
                return -1;
            }
        }
    }
    
    return -1;
}

/*
 * sys_exit: Terminate current process
 */
void sys_exit(int status) {
    process_t *proc = get_current_process();
    if (!proc) {
        return;
    }
    
    proc->exit_status = status;
    proc->state = PROCESS_TERMINATED;
    
    /* Notify parent */
    if (proc->parent) {
        /* Parent might be waiting */
    }
    
    /* Schedule another process */
    yield();
    
    /* Should not reach here */
    while (1) {
        halt_cpu();
    }
}

/*
 * sys_yield: Voluntarily give up CPU
 */
void sys_yield(void) {
    yield();
}

/*
 * sys_getpid: Get current process ID
 */
s64 sys_getpid(void) {
    process_t *proc = get_current_process();
    if (!proc) {
        return -1;
    }
    return (s64)proc->pid;
}

/*
 * sys_write: Write to file descriptor
 * Currently supports fd 1 (stdout/console)
 */
s64 sys_write(int fd, const char *buf, size_t count) {
    if (fd != 1 && fd != 2) {  /* stdout or stderr */
        return -1;
    }
    
    /* Write to console */
    for (size_t i = 0; i < count; i++) {
        console_putchar(buf[i]);
    }
    
    return (s64)count;
}

/*
 * sys_read: Read from file descriptor
 * Currently supports fd 0 (stdin/keyboard)
 */
s64 sys_read(int fd, void *buf, size_t count) {
    if (fd != 0) {  /* stdin only for now */
        return -1;
    }
    
    /* Read from keyboard buffer */
    char *cbuf = (char *)buf;
    size_t i = 0;
    while (i < count) {
        char c = keyboard_getchar();
        if (c == 0) {
            /* No character available, yield and retry */
            if (i > 0) break;
            yield();
            continue;
        }
        cbuf[i++] = c;
        if (c == '\n') break;  /* Line buffered */
    }
    
    return (s64)i;
}

/*
 * sys_open: Open a file by path
 * Returns file descriptor or -1 on error
 */
s64 sys_open(const char *path, int flags) {
    if (!path) {
        return -1;
    }
    
    /* Use VFS to open the file */
    int fd = vfs_open(path, flags);
    return (s64)fd;
}

/*
 * sys_close: Close a file descriptor
 * Returns 0 on success, -1 on error
 */
s64 sys_close(int fd) {
    if (fd < 0) {
        return -1;
    }
    
    /* Use VFS to close the file */
    int ret = vfs_close(fd);
    return (s64)ret;
}

/*
 * NusantaraOS64 - IPC & Synchronization Implementation
 * 
 * Fase 7: Inter-Process Communication & Sync
 * Progres: 50% -> 55%
 * 
 * Implements:
 * - Spinlock (untuk kernel internal)
 * - Mutex (user process synchronization)
 * - Semaphore (counting semaphore)
 * - Message Passing (IPC)
 * - Shared Memory
 */

#include "../include/nusantara.h"

/* ============================================
 * Global IPC State
 * ============================================ */
#define MAX_MUTEX     256
#define MAX_SEM       256
#define MAX_SHM       64

static mutex_t mutex_table[MAX_MUTEX];
static semaphore_t sem_table[MAX_SEM];
static shm_region_t shm_table[MAX_SHM];
static u32 next_mutex_id = 1;
static u32 next_sem_id = 1;
static u32 next_shm_id = 1;

/* Kernel internal spinlock untuk melindungi tabel IPC */
static volatile u32 ipc_lock = 0;

/* ============================================
 * Spinlock Implementation (Kernel Internal)
 * ============================================ */

void spinlock_acquire(volatile u32 *lock) {
    while (__sync_lock_test_and_set(lock, 1)) {
        /* Busy wait dengan pause instruction untuk efisiensi */
        __asm__ volatile("pause" ::: "memory");
    }
}

void spinlock_release(volatile u32 *lock) {
    __sync_lock_release(lock);
}

/* ============================================
 * Mutex Implementation
 * ============================================ */

s64 sys_mutex_create(void) {
    spinlock_acquire(&ipc_lock);
    
    /* Cari slot kosong */
    for (u32 i = 0; i < MAX_MUTEX; i++) {
        if (mutex_table[i].id == 0) {
            mutex_table[i].id = next_mutex_id++;
            mutex_table[i].locked = 0;
            mutex_table[i].owner = -1;
            mutex_table[i].wait_count = 0;
            memset(mutex_table[i].wait_queue, 0xFF, sizeof(mutex_table[i].wait_queue));
            
            u32 mutex_id = mutex_table[i].id;
            spinlock_release(&ipc_lock);
            return (s64)mutex_id;
        }
    }
    
    spinlock_release(&ipc_lock);
    return -1; /* No available mutex */
}

s64 sys_mutex_lock(s64 mutex_id) {
    if (mutex_id <= 0 || mutex_id > MAX_MUTEX) {
        return -1;
    }
    
    process_t *current = get_current_process();
    mutex_t *mtx = &mutex_table[mutex_id - 1];
    
    spinlock_acquire(&ipc_lock);
    
    if (mtx->id == 0) {
        spinlock_release(&ipc_lock);
        return -1; /* Invalid mutex */
    }
    
    /* Cek jika mutex sudah di-lock */
    if (mtx->locked) {
        /* Tambahkan proses ke wait queue */
        if (mtx->wait_count < 32) {
            mtx->wait_queue[mtx->wait_count++] = current->pid;
        }
        
        /* Block proses ini */
        current->state = PROCESS_BLOCKED;
        spinlock_release(&ipc_lock);
        
        /* Yield CPU */
        yield();
        return 0; /* Akan return setelah mendapat lock */
    }
    
    /* Ambil lock */
    mtx->locked = 1;
    mtx->owner = current->pid;
    
    /* Track mutex yang dipegang proses */
    if (current->held_mutex_count < 16) {
        current->held_mutexes[current->held_mutex_count++] = mtx;
    }
    
    spinlock_release(&ipc_lock);
    return 0;
}

s64 sys_mutex_unlock(s64 mutex_id) {
    if (mutex_id <= 0 || mutex_id > MAX_MUTEX) {
        return -1;
    }
    
    process_t *current = get_current_process();
    mutex_t *mtx = &mutex_table[mutex_id - 1];
    
    spinlock_acquire(&ipc_lock);
    
    if (mtx->id == 0 || (u64)mtx->owner != current->pid) {
        spinlock_release(&ipc_lock);
        return -1; /* Invalid mutex atau bukan owner */
    }
    
    /* Release lock */
    mtx->locked = 0;
    mtx->owner = -1;
    
    /* Hapus dari tracked mutexes */
    for (u32 i = 0; i < current->held_mutex_count; i++) {
        if (current->held_mutexes[i] == mtx) {
            for (u32 j = i; j < current->held_mutex_count - 1; j++) {
                current->held_mutexes[j] = current->held_mutexes[j + 1];
            }
            current->held_mutex_count--;
            break;
        }
    }
    
    /* Wake up satu proses dari wait queue */
    if (mtx->wait_count > 0) {
        
        /* Shift wait queue */
        for (u32 i = 0; i < mtx->wait_count - 1; i++) {
            mtx->wait_queue[i] = mtx->wait_queue[i + 1];
        }
        mtx->wait_count--;
        
        /* TODO: Find and unblock the process */
        /* Ini memerlukan scheduler support */
    }
    
    spinlock_release(&ipc_lock);
    return 0;
}

/* ============================================
 * Semaphore Implementation
 * ============================================ */

s64 sys_sem_create(s32 initial_value) {
    spinlock_acquire(&ipc_lock);
    
    for (u32 i = 0; i < MAX_SEM; i++) {
        if (sem_table[i].id == 0) {
            sem_table[i].id = next_sem_id++;
            sem_table[i].value = initial_value;
            sem_table[i].wait_count = 0;
            memset(sem_table[i].wait_queue, 0xFF, sizeof(sem_table[i].wait_queue));
            
            u32 sem_id = sem_table[i].id;
            spinlock_release(&ipc_lock);
            return (s64)sem_id;
        }
    }
    
    spinlock_release(&ipc_lock);
    return -1;
}

s64 sys_sem_wait(s64 sem_id) {
    if (sem_id <= 0 || sem_id > MAX_SEM) {
        return -1;
    }
    
    process_t *current = get_current_process();
    semaphore_t *sem = &sem_table[sem_id - 1];
    
    spinlock_acquire(&ipc_lock);
    
    if (sem->id == 0) {
        spinlock_release(&ipc_lock);
        return -1;
    }
    
    if (sem->value > 0) {
        sem->value--;
        spinlock_release(&ipc_lock);
        return 0;
    }
    
    /* Value = 0, block proses */
    if (sem->wait_count < 32) {
        sem->wait_queue[sem->wait_count++] = current->pid;
    }
    
    current->state = PROCESS_BLOCKED;
    spinlock_release(&ipc_lock);
    
    yield();
    return 0;
}

s64 sys_sem_post(s64 sem_id) {
    if (sem_id <= 0 || sem_id > MAX_SEM) {
        return -1;
    }
    
    semaphore_t *sem = &sem_table[sem_id - 1];
    
    spinlock_acquire(&ipc_lock);
    
    if (sem->id == 0) {
        spinlock_release(&ipc_lock);
        return -1;
    }
    
    if (sem->wait_count > 0) {
        /* Ada proses waiting, wake up satu */
        
        for (u32 i = 0; i < sem->wait_count - 1; i++) {
            sem->wait_queue[i] = sem->wait_queue[i + 1];
        }
        sem->wait_count--;
        
        /* TODO: Unblock process */
    } else if (sem->value < SEM_VALUE_MAX) {
        sem->value++;
    }
    
    spinlock_release(&ipc_lock);
    return 0;
}

/* ============================================
 * Message Passing Implementation
 * ============================================ */

s64 sys_msg_send(pid_t dest, u32 type, const void *data, u32 size) {
    if (size > MAX_MSG_SIZE || dest < 0) {
        return -1;
    }
    
    process_t *current = get_current_process();
    
    /* TODO: Find destination process by PID */
    /* Untuk sementara, return error */
    
    /* Copy message ke buffer */
    message_t msg;
    msg.sender = current->pid;
    msg.receiver = dest;
    msg.type = type;
    msg.size = size;
    if (data && size > 0) {
        memcpy(msg.data, data, size);
    }
    
    /* TODO: Deliver message to destination queue */
    
    return (s64)size;
}

s64 sys_msg_recv(pid_t src, u32 type, void *buf, u32 max_size) {
    if (max_size > MAX_MSG_SIZE) {
        return -1;
    }
    
    process_t *current = get_current_process();
    
    spinlock_acquire(&ipc_lock);
    
    /* Cek ada message di queue */
    if (current->msg_count == 0) {
        spinlock_release(&ipc_lock);
        /* Block sampai ada message */
        current->state = PROCESS_BLOCKED;
        yield();
        return -1;
    }
    
    /* Ambil message dari head */
    message_t *msg = &current->msg_queue[current->msg_head];
    
    /* Filter by sender dan type */
    if ((src != -1 && msg->sender != src) || (type != 0 && msg->type != type)) {
        spinlock_release(&ipc_lock);
        return -1; /* Message tidak match */
    }
    
    /* Copy data ke buffer user */
    u32 copy_size = (msg->size < max_size) ? msg->size : max_size;
    if (buf && copy_size > 0) {
        memcpy(buf, msg->data, copy_size);
    }
    
    /* Advance head */
    current->msg_head = (current->msg_head + 1) % 8;
    current->msg_count--;
    
    spinlock_release(&ipc_lock);
    return (s64)copy_size;
}

/* ============================================
 * Shared Memory Implementation
 * ============================================ */

s64 sys_shm_create(const char *name, u64 size) {
    if (!name || size == 0) {
        return -1;
    }
    
    spinlock_acquire(&ipc_lock);
    
    /* Cek apakah nama sudah ada */
    for (u32 i = 0; i < MAX_SHM; i++) {
        if (shm_table[i].id != 0 && strcmp(shm_table[i].name, name) == 0) {
            spinlock_release(&ipc_lock);
            return -2; /* Already exists */
        }
    }
    
    /* Cari slot kosong */
    for (u32 i = 0; i < MAX_SHM; i++) {
        if (shm_table[i].id == 0) {
            shm_table[i].id = next_shm_id++;
            strncpy(shm_table[i].name, name, 31);
            shm_table[i].name[31] = '\0';
            shm_table[i].size = size;
            shm_table[i].owner = get_current_process()->pid;
            shm_table[i].attach_count = 0;
            shm_table[i].kernel_mapped = false;
            
            /* Alokasi physical memory untuk shared region */
            /* TODO: Call physical memory allocator */
            shm_table[i].phys_addr = 0; /* Placeholder */
            shm_table[i].virt_addr = 0; /* Will be mapped on attach */
            
            memset(shm_table[i].attached_procs, 0xFF, sizeof(shm_table[i].attached_procs));
            
            u32 shm_id = shm_table[i].id;
            spinlock_release(&ipc_lock);
            return (s64)shm_id;
        }
    }
    
    spinlock_release(&ipc_lock);
    return -1; /* No available slots */
}

s64 sys_shm_attach(s64 shm_id) {
    if (shm_id <= 0 || shm_id > MAX_SHM) {
        return -1;
    }
    
    process_t *current = get_current_process();
    shm_region_t *shm = &shm_table[shm_id - 1];
    
    spinlock_acquire(&ipc_lock);
    
    if (shm->id == 0) {
        spinlock_release(&ipc_lock);
        return -1;
    }
    
    /* Cek sudah attached */
    for (u32 i = 0; i < shm->attach_count; i++) {
        if (shm->attached_procs[i] == (u64)current->pid) {
            spinlock_release(&ipc_lock);
            return (s64)shm->virt_addr; /* Already attached */
        }
    }
    
    /* Add process ke attached list */
    if (shm->attach_count < 16) {
        shm->attached_procs[shm->attach_count++] = current->pid;
    }
    
    /* Map shared memory ke address space proses */
    /* TODO: Call page table mapper */
    u64 virt_addr = 0x100000000ULL + (shm_id * 0x1000000ULL); /* Simple allocation */
    
    /* Add ke process tracking */
    if (current->shm_count < MAX_SHM_REGIONS) {
        current->shm_regions[current->shm_count++] = shm;
    }
    
    spinlock_release(&ipc_lock);
    return (s64)virt_addr;
}

s64 sys_shm_detach(s64 shm_id) {
    if (shm_id <= 0 || shm_id > MAX_SHM) {
        return -1;
    }
    
    process_t *current = get_current_process();
    shm_region_t *shm = &shm_table[shm_id - 1];
    
    spinlock_acquire(&ipc_lock);
    
    if (shm->id == 0) {
        spinlock_release(&ipc_lock);
        return -1;
    }
    
    /* Remove dari attached list */
    for (u32 i = 0; i < shm->attach_count; i++) {
        if (shm->attached_procs[i] == (u64)current->pid) {
            for (u32 j = i; j < shm->attach_count - 1; j++) {
                shm->attached_procs[j] = shm->attached_procs[j + 1];
            }
            shm->attach_count--;
            break;
        }
    }
    
    /* Remove dari process tracking */
    for (u32 i = 0; i < current->shm_count; i++) {
        if (current->shm_regions[i] == shm) {
            for (u32 j = i; j < current->shm_count - 1; j++) {
                current->shm_regions[j] = current->shm_regions[j + 1];
            }
            current->shm_count--;
            break;
        }
    }
    
    /* TODO: Unmap dari address space */
    
    spinlock_release(&ipc_lock);
    return 0;
}

/* ============================================
 * Sleep Implementation
 * ============================================ */

s64 sys_sleep(u64 ms) {
    process_t *current = get_current_process();
    
    /* Set wake-up time (dalam tick, asumsi 1 tick = 10ms) */
    u64 ticks = ms / 10;
    current->sleep_until = ticks; /* Simplified: perlu timer integration */
    current->state = PROCESS_BLOCKED;
    
    yield();
    return 0;
}

/* ============================================
 * IPC Initialization
 * ============================================ */

void ipc_init(void) {
    memset(mutex_table, 0, sizeof(mutex_table));
    memset(sem_table, 0, sizeof(sem_table));
    memset(shm_table, 0, sizeof(shm_table));
    
    next_mutex_id = 1;
    next_sem_id = 1;
    next_shm_id = 1;
    ipc_lock = 0;
}

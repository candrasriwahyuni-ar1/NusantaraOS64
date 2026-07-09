/**
 * NusantaraOS64 - nusa-libc
 * 
 * C Library minimalis untuk user space applications
 * Wrapper syscall dan fungsi utility dasar
 */

#include "../include/nusantara.h"

// ============================================================================
// SYSCALL WRAPPERS
// ============================================================================

/**
 * System call interface
 * Menggunakan instruksi 'syscall' x86_64
 */
static inline uint64_t syscall(uint64_t num, uint64_t arg1, uint64_t arg2, 
                                uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    uint64_t ret;
    asm volatile (
        "mov %0, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "mov %2, %%rsi\n\t"
        "mov %3, %%rdx\n\t"
        "mov %4, %%r10\n\t"
        "mov %5, %%r8\n\t"
        "syscall\n\t"
        "mov %%rax, %0\n\t"
        : "=r"(ret)
        : "r"(num), "r"(arg1), "r"(arg2), "r"(arg3), "r"(arg4), "r"(arg5)
        : "rax", "rdi", "rsi", "rdx", "r10", "r8", "rcx", "r11", "memory"
    );
    return ret;
}

int nusa_open(const char *pathname, int flags) {
    return (int)syscall(SYS_OPEN, (uint64_t)pathname, (uint64_t)flags, 0, 0, 0);
}

ssize_t nusa_read(int fd, void *buf, size_t count) {
    return (ssize_t)syscall(SYS_READ, (uint64_t)fd, (uint64_t)buf, (uint64_t)count, 0, 0);
}

ssize_t nusa_write(int fd, const void *buf, size_t count) {
    return (ssize_t)syscall(SYS_WRITE, (uint64_t)fd, (uint64_t)buf, (uint64_t)count, 0, 0);
}

int nusa_close(int fd) {
    return (int)syscall(SYS_CLOSE, (uint64_t)fd, 0, 0, 0, 0);
}

pid_t nusa_fork(void) {
    return (pid_t)syscall(SYS_FORK, 0, 0, 0, 0, 0);
}

int nusa_exec(const char *filename, char *const argv[]) {
    return (int)syscall(SYS_EXEC, (uint64_t)filename, (uint64_t)argv, 0, 0, 0);
}

int nusa_exit(int status) {
    return (int)syscall(SYS_EXIT, (uint64_t)status, 0, 0, 0, 0);
}

pid_t nusa_waitpid(pid_t pid, int *wstatus, int options) {
    return (pid_t)syscall(SYS_WAITPID, (uint64_t)pid, (uint64_t)wstatus, (uint64_t)options, 0, 0);
}

int nusa_yield(void) {
    return (int)syscall(SYS_YIELD, 0, 0, 0, 0, 0);
}

pid_t nusa_getpid(void) {
    return (pid_t)syscall(SYS_GETPID, 0, 0, 0, 0, 0);
}

int nusa_sleep(uint64_t ms) {
    return (int)syscall(SYS_SLEEP, (uint64_t)ms, 0, 0, 0, 0);
}

// ============================================================================
// STRING FUNCTIONS
// ============================================================================

size_t nusa_strlen(const char *s) {
    if (!s) return 0;
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

char* nusa_strcpy(char *dest, const char *src) {
    if (!dest || !src) return dest;
    char *d = dest;
    while ((*d++ = *src++) != '\0');
    return dest;
}

int nusa_strcmp(const char *s1, const char *s2) {
    if (!s1 || !s2) return s1 == s2 ? 0 : (s1 ? 1 : -1);
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char* nusa_strcat(char *dest, const char *src) {
    if (!dest || !src) return dest;
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++) != '\0');
    return dest;
}

// ============================================================================
// MEMORY ALLOCATION (Simple Bump Allocator)
// ============================================================================

#define HEAP_SIZE (64 * 1024)  // 64 KB heap awal
static uint8_t heap[HEAP_SIZE];
static size_t heap_ptr = 0;

void* nusa_malloc(size_t size) {
    // Align ke 8 byte
    size = (size + 7) & ~7;
    
    if (heap_ptr + size > HEAP_SIZE) {
        return NULL;  // Out of memory
    }
    
    void *ptr = &heap[heap_ptr];
    heap_ptr += size;
    return ptr;
}

void nusa_free(void *ptr) {
    // Simple bump allocator: tidak ada free individual
    // Memory hanya dibebaskan saat proses exit
    (void)ptr;
}

void* nusa_calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *ptr = nusa_malloc(total);
    if (ptr) {
        // Zero fill
        uint8_t *p = (uint8_t*)ptr;
        for (size_t i = 0; i < total; i++) {
            p[i] = 0;
        }
    }
    return ptr;
}

// ============================================================================
// FORMATTED OUTPUT (Minimal printf)
// ============================================================================

static void print_char(char c, void **arg) {
    int *count = (int*)arg[0];
    int fd = *(int*)arg[1];
    nusa_write(fd, &c, 1);
    (*count)++;
}

static void print_string(const char *s, void **arg) {
    int *count = (int*)arg[0];
    int fd = *(int*)arg[1];
    while (*s) {
        nusa_write(fd, s++, 1);
        (*count)++;
    }
}

static void print_number(long num, int base, int is_signed, void **arg) {
    char buf[32];
    char *p = buf + sizeof(buf);
    *--p = '\0';
    
    unsigned long unum;
    if (is_signed && num < 0) {
        unum = -num;
    } else {
        unum = num;
    }
    
    do {
        *--p = "0123456789abcdef"[unum % base];
        unum /= base;
    } while (unum > 0);
    
    if (is_signed && num < 0) {
        *--p = '-';
    }
    
    print_string(p, arg);
}

int nusa_vprintf(int fd, const char *fmt, va_list args) {
    int count = 0;
    void *arg_array[2];
    arg_array[0] = &count;
    arg_array[1] = &fd;
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd':
                case 'i':
                    print_number(va_arg(args, int), 10, 1, arg_array);
                    break;
                case 'u':
                    print_number(va_arg(args, unsigned int), 10, 0, arg_array);
                    break;
                case 'x':
                    print_number(va_arg(args, unsigned int), 16, 0, arg_array);
                    break;
                case 'X':
                    print_number(va_arg(args, unsigned int), 16, 0, arg_array);
                    break;
                case 'p':
                    print_number((long)va_arg(args, void*), 16, 0, arg_array);
                    break;
                case 'c':
                    {
                        char c = va_arg(args, int);
                        print_char(c, arg_array);
                    }
                    break;
                case 's':
                    {
                        const char *s = va_arg(args, const char*);
                        if (s) {
                            print_string(s, arg_array);
                        } else {
                            print_string("(null)", arg_array);
                        }
                    }
                    break;
                case '%':
                    print_char('%', arg_array);
                    break;
                default:
                    print_char('%', arg_array);
                    print_char(*fmt, arg_array);
                    break;
            }
        } else {
            print_char(*fmt, arg_array);
        }
        fmt++;
    }
    
    return count;
}

int nusa_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = nusa_vprintf(STDOUT_FILENO, fmt, args);
    va_end(args);
    return ret;
}

int nusa_fprintf(int fd, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = nusa_vprintf(fd, fmt, args);
    va_end(args);
    return ret;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void nusa_memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t*)s;
    while (n--) {
        *p++ = c;
    }
}

void nusa_memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
}

int nusa_memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t*)s1;
    const uint8_t *p2 = (const uint8_t*)s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

// ============================================================================
// MAIN ENTRY POINT WRAPPER
// ============================================================================

/**
 * Wrapper untuk main() user program
 * Setup environment dan panggil main user
 */
extern int main(int argc, char *argv[]);

void _start(void) {
    // TODO: Parse argument dari stack
    // Untuk sementara, panggil main tanpa argumen
    int ret = main(0, NULL);
    nusa_exit(ret);
}

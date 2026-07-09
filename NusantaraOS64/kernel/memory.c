/*
 * NusantaraOS64 - Memory Management (Kernel Heap)
 * 
 * Implementasi kernel heap sederhana dengan first-fit allocation
 */

#include "../include/nusantara.h"

#define HEAP_SIZE         (16 * 1024 * 1024)  /* 16 MB */
#define HEAP_START        0xFFFF800000000000ULL

typedef struct memory_block {
    size_t size;
    struct memory_block *next;
    int free;
} memory_block_t;

static u8 heap[HEAP_SIZE] __attribute__((aligned(PAGE_SIZE)));
static memory_block_t *free_list = NULL;
static bool memory_initialized = false;

/*
 * Initialize kernel memory manager
 */
void memory_init(void) {
    if (memory_initialized) {
        return;
    }
    
    /* Initialize free list with entire heap */
    free_list = (memory_block_t *)heap;
    free_list->size = HEAP_SIZE - sizeof(memory_block_t);
    free_list->next = NULL;
    free_list->free = 1;
    
    memory_initialized = true;
}

/*
 * Allocate memory from kernel heap (first-fit algorithm)
 */
void *kmalloc(size_t size) {
    if (!memory_initialized || size == 0) {
        return NULL;
    }
    
    /* Align size to 16 bytes */
    size = (size + 15) & ~15;
    
    memory_block_t *block = free_list;
    
    while (block != NULL) {
        if (block->free && block->size >= size) {
            /* Found a suitable block */
            
            /* Split block if there's enough remaining space */
            if (block->size > size + sizeof(memory_block_t) + 16) {
                memory_block_t *new_block = (memory_block_t *)((u8 *)block + sizeof(memory_block_t) + size);
                new_block->size = block->size - size - sizeof(memory_block_t);
                new_block->next = block->next;
                new_block->free = 1;
                
                block->size = size;
                block->next = new_block;
            }
            
            block->free = 0;
            return (void *)((u8 *)block + sizeof(memory_block_t));
        }
        
        block = block->next;
    }
    
    /* No suitable block found */
    return NULL;
}

/*
 * Free previously allocated memory
 */
void kfree(void *ptr) {
    if (ptr == NULL || !memory_initialized) {
        return;
    }
    
    memory_block_t *block = (memory_block_t *)((u8 *)ptr - sizeof(memory_block_t));
    block->free = 1;
    
    /* Coalesce adjacent free blocks */
    memory_block_t *curr = free_list;
    
    while (curr != NULL) {
        if (curr->free && curr->next != NULL && curr->next->free) {
            /* Merge with next block */
            curr->size += sizeof(memory_block_t) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

/*
 * memset implementation
 */
void memset(void *dest, u8 val, size_t count) {
    u8 *d = (u8 *)dest;
    while (count--) {
        *d++ = val;
    }
}

/*
 * memcpy implementation
 */
void memcpy(void *dest, const void *src, size_t count) {
    const u8 *s = (const u8 *)src;
    u8 *d = (u8 *)dest;
    while (count--) {
        *d++ = *s++;
    }
}

/*
 * memcmp implementation
 */
int memcmp(const void *s1, const void *s2, size_t n) {
    const u8 *p1 = (const u8 *)s1;
    const u8 *p2 = (const u8 *)s2;
    
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    
    return 0;
}

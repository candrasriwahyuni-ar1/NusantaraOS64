/*
 * NusantaraOS64 - Paging System (4-Level Page Tables)
 * 
 * Fase 4: Virtualisasi Memori (Address Translation)
 * Implementasi multi-level page tables untuk x86_64
 */

#include "../include/nusantara.h"

/* Page directory hierarchy */
static page_entry_t *pml4 = NULL;
static page_entry_t *kernel_pdpt = NULL;
static u64 next_physical_addr = 0;

/*
 * Get next available physical page frame
 */
static u64 get_next_frame(void) {
    u64 frame = next_physical_addr;
    next_physical_addr += PAGE_SIZE;
    return frame;
}

/*
 * Allocate a zeroed page table page
 */
static page_entry_t *alloc_page_table(void) {
    u64 phys = get_next_frame();
    page_entry_t *table = (page_entry_t *)(phys + KERNEL_BASE);
    
    /* Zero out the page */
    for (int i = 0; i < 512; i++) {
        table[i].present = 0;
        table[i].writable = 0;
        table[i].user = 0;
        table[i].frame = 0;
    }
    
    return table;
}

/*
 * Initialize paging system
 */
void paging_init(void) {
    /* Allocate PML4 (top level) */
    pml4 = alloc_page_table();
    
    /* Allocate kernel PDPT */
    kernel_pdpt = alloc_page_table();
    
    /* Set up identity mapping for kernel (first 512 MB) */
    /* This allows the kernel to run while setting up full paging */
    
    /* Map kernel code and data */
    for (u64 addr = 0; addr < (512 * 1024 * 1024); addr += PAGE_SIZE) {
        map_page(addr, addr, 0x3);  /* Present + Writable */
    }
    
    /* Load CR3 with PML4 physical address */
    u64 pml4_phys = (u64)pml4 - KERNEL_BASE;
    __asm__ volatile("movq %0, %%cr3" :: "r"(pml4_phys));
    
    /* Enable paging and long mode */
    u64 cr0;
    __asm__ volatile("movq %%cr0, %0" : "=r"(cr0));
    cr0 |= CR0_PAGING;
    __asm__ volatile("movq %0, %%cr0" :: "r"(cr0));
}

/*
 * Map virtual address to physical address
 * flags: bit 0 = present, bit 1 = writable, bit 2 = user
 */
void map_page(u64 virt, u64 phys, u64 flags) {
    /* Extract indices from virtual address */
    u64 pml4_idx = (virt >> 39) & 0x1FF;
    u64 pdpt_idx = (virt >> 30) & 0x1FF;
    u64 pd_idx   = (virt >> 21) & 0x1FF;
    u64 pt_idx   = (virt >> 12) & 0x1FF;
    
    /* Get or create PML4 entry */
    if (!pml4[pml4_idx].present) {
        page_entry_t *pdpt = alloc_page_table();
        pml4[pml4_idx].present = 1;
        pml4[pml4_idx].writable = 1;
        pml4[pml4_idx].frame = ((u64)pdpt - KERNEL_BASE) >> 12;
    }
    
    page_entry_t *pdpt = (page_entry_t *)((pml4[pml4_idx].frame << 12) + KERNEL_BASE);
    
    /* Get or create PDPT entry */
    if (!pdpt[pdpt_idx].present) {
        page_entry_t *pd = alloc_page_table();
        pdpt[pdpt_idx].present = 1;
        pdpt[pdpt_idx].writable = 1;
        pdpt[pdpt_idx].frame = ((u64)pd - KERNEL_BASE) >> 12;
    }
    
    page_entry_t *pd = (page_entry_t *)((pdpt[pdpt_idx].frame << 12) + KERNEL_BASE);
    
    /* Get or create PD entry */
    if (!pd[pd_idx].present) {
        page_entry_t *pt = alloc_page_table();
        pd[pd_idx].present = 1;
        pd[pd_idx].writable = 1;
        pd[pd_idx].frame = ((u64)pt - KERNEL_BASE) >> 12;
    }
    
    page_entry_t *pt = (page_entry_t *)((pd[pd_idx].frame << 12) + KERNEL_BASE);
    
    /* Set page table entry */
    pt[pt_idx].present = flags & 0x1;
    pt[pt_idx].writable = (flags >> 1) & 0x1;
    pt[pt_idx].user = (flags >> 2) & 0x1;
    pt[pt_idx].frame = phys >> 12;
}

/*
 * Unmap a virtual page
 */
void unmap_page(u64 virt) {
    u64 pml4_idx = (virt >> 39) & 0x1FF;
    u64 pdpt_idx = (virt >> 30) & 0x1FF;
    u64 pd_idx   = (virt >> 21) & 0x1FF;
    u64 pt_idx   = (virt >> 12) & 0x1FF;
    
    if (!pml4[pml4_idx].present) return;
    page_entry_t *pdpt = (page_entry_t *)((pml4[pml4_idx].frame << 12) + KERNEL_BASE);
    
    if (!pdpt[pdpt_idx].present) return;
    page_entry_t *pd = (page_entry_t *)((pdpt[pdpt_idx].frame << 12) + KERNEL_BASE);
    
    if (!pd[pd_idx].present) return;
    page_entry_t *pt = (page_entry_t *)((pd[pd_idx].frame << 12) + KERNEL_BASE);
    
    pt[pt_idx].present = 0;
}

/*
 * Translate virtual address to physical address
 */
u64 translate_address(u64 virt) {
    u64 pml4_idx = (virt >> 39) & 0x1FF;
    u64 pdpt_idx = (virt >> 30) & 0x1FF;
    u64 pd_idx   = (virt >> 21) & 0x1FF;
    u64 pt_idx   = (virt >> 12) & 0x1FF;
    u64 offset   = virt & 0xFFF;
    
    if (!pml4[pml4_idx].present) return 0;
    page_entry_t *pdpt = (page_entry_t *)((pml4[pml4_idx].frame << 12) + KERNEL_BASE);
    
    if (!pdpt[pdpt_idx].present) return 0;
    page_entry_t *pd = (page_entry_t *)((pdpt[pdpt_idx].frame << 12) + KERNEL_BASE);
    
    if (!pd[pd_idx].present) return 0;
    page_entry_t *pt = (page_entry_t *)((pd[pd_idx].frame << 12) + KERNEL_BASE);
    
    if (!pt[pt_idx].present) return 0;
    
    return (pt[pt_idx].frame << 12) | offset;
}

/*
 * Flush TLB by reloading CR3
 */
void flush_tlb(void) {
    u64 cr3;
    __asm__ volatile("movq %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("movq %0, %%cr3" :: "r"(cr3));
}

/*
 * Create address space for a new process
 */
process_t *create_process_address_space(void) {
    /* Allocate new PML4 for the process */
    page_entry_t *proc_pml4 = alloc_page_table();
    
    /* Copy kernel mappings (upper half of address space) */
    for (int i = 256; i < 512; i++) {
        proc_pml4[i] = pml4[i];
    }
    
    /* Create a minimal process structure */
    process_t *proc = (process_t *)kmalloc(sizeof(process_t));
    if (!proc) {
        return NULL;
    }
    
    memset(proc, 0, sizeof(process_t));
    proc->page_table = (void *)(u64)proc_pml4;  /* Cast to void* to avoid alignment warning */
    
    return proc;
}

/*
 * Switch to a process's address space
 */
void switch_address_space(process_t *proc) {
    if (!proc || !proc->page_table) {
        return;
    }
    
    u64 pml4_phys = (u64)proc->page_table - KERNEL_BASE;
    __asm__ volatile("movq %0, %%cr3" :: "r"(pml4_phys));
}

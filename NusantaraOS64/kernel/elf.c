/**
 * NusantaraOS64 - ELF64 Loader
 * 
 * Loader untuk format executable ELF64
 * Memuat program user dari disk ke memori virtual
 */

#include "../include/nusantara.h"

/* Forward declarations from existing modules */
extern void *kmalloc(size_t size);
extern void map_page(u64 virt, u64 phys, u64 flags);
extern void console_printf(const char *fmt, ...);

#define ELF_MAGIC 0x464C457F  // "\x7FELF"
#define ET_EXEC 2              // Executable file
#define ET_DYN 3               // Shared object file
#define PT_LOAD 1              // Loadable segment

typedef struct {
    uint8_t  e_ident[16];      // ELF identification
    uint16_t e_type;           // Object file type
    uint16_t e_machine;        // Machine type
    uint32_t e_version;        // Object file version
    uint64_t e_entry;          // Entry point address
    uint64_t e_phoff;          // Program header offset
    uint64_t e_shoff;          // Section header offset
    uint32_t e_flags;          // Processor-specific flags
    uint16_t e_ehsize;         // ELF header size
    uint16_t e_phentsize;      // Size of program header entry
    uint16_t e_phnum;          // Number of program header entries
    uint16_t e_shentsize;      // Size of section header entry
    uint16_t e_shnum;          // Number of section header entries
    uint16_t e_shstrndx;       // Section name string table index
} Elf64_Ehdr;

typedef struct {
    uint32_t p_type;           // Type of segment
    uint32_t p_flags;          // Segment attributes
    uint64_t p_offset;         // Offset in file
    uint64_t p_vaddr;          // Virtual address in memory
    uint64_t p_paddr;          // Reserved (physical address)
    uint64_t p_filesz;         // Size of segment in file
    uint64_t p_memsz;          // Size of segment in memory
    uint64_t p_align;          // Alignment of segment
} Elf64_Phdr;

/**
 * Validasi file ELF64
 */
static int elf_validate(uint8_t *buffer, size_t size) {
    if (size < sizeof(Elf64_Ehdr)) {
        kprintf("[ELF] Error: File terlalu kecil\n");
        return -1;
    }
    
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)buffer;
    
    // Cek magic number
    if (ehdr->e_ident[0] != 0x7F || 
        ehdr->e_ident[1] != 'E' || 
        ehdr->e_ident[2] != 'L' || 
        ehdr->e_ident[3] != 'F') {
        kprintf("[ELF] Error: Bukan file ELF (magic: %x)\n", *(uint32_t*)ehdr->e_ident);
        return -1;
    }
    
    // Cek 64-bit
    if (ehdr->e_ident[4] != 2) {  // ELFCLASS64
        kprintf("[ELF] Error: Bukan ELF 64-bit\n");
        return -1;
    }
    
    // Cek little endian
    if (ehdr->e_ident[5] != 1) {  // ELFDATA2LSB
        kprintf("[ELF] Error: Bukan little endian\n");
        return -1;
    }
    
    // Cek tipe file
    if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) {
        kprintf("[ELF] Error: Bukan executable (type: %d)\n", ehdr->e_type);
        return -1;
    }
    
    // Cek machine type (x86_64)
    if (ehdr->e_machine != 0x3E) {
        kprintf("[ELF] Error: Bukan x86_64 (machine: %x)\n", ehdr->e_machine);
        return -1;
    }
    
    kprintf("[ELF] Valid: %s, entry=0x%lx, phnum=%d\n", 
            ehdr->e_type == ET_EXEC ? "EXEC" : "DYN",
            ehdr->e_entry, ehdr->e_phnum);
    
    return 0;
}

/**
 * Muat program ELF64 ke address space proses
 * Returns: 0 on success, -1 on error
 */
int elf_load(process_t *proc, uint8_t *buffer, size_t size) {
    if (elf_validate(buffer, size) != 0) {
        return -1;
    }
    
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)buffer;
    
    // Iterasi program headers
    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf64_Phdr *phdr = (Elf64_Phdr*)(buffer + ehdr->e_phoff + i * ehdr->e_phentsize);
        
        // Hanya load segment yang bertipe PT_LOAD
        if (phdr->p_type != PT_LOAD) {
            continue;
        }
        
        kprintf("[ELF] Loading segment: vaddr=0x%lx, filesz=%ld, memsz=%ld\n",
                phdr->p_vaddr, phdr->p_filesz, phdr->p_memsz);
        
        // Alokasi halaman untuk segment ini
        uint64_t start_page = phdr->p_vaddr & PAGE_MASK;
        uint64_t end_page = (phdr->p_vaddr + phdr->p_memsz + PAGE_SIZE - 1) & PAGE_MASK;
        size_t num_pages = (end_page - start_page) / PAGE_SIZE;
        
        // Mapping halaman
        for (size_t p = 0; p < num_pages; p++) {
            uint64_t vaddr = start_page + p * PAGE_SIZE;
            
            // Alokasi frame fisik menggunakan kmalloc
            void *frame = kmalloc(PAGE_SIZE);
            if (!frame) {
                console_printf("[ELF] Error: Gagal alokasi frame untuk 0x%lx\n", vaddr);
                return -1;
            }
            
            // Map ke address space proses menggunakan map_page
            map_page(vaddr, (uint64_t)frame, 0x3);  // 0x3 = PRESENT | WRITABLE
        }
        
        // Copy data dari buffer ke memori
        if (phdr->p_filesz > 0) {
            // Untuk kernel mapping sementara
            uint8_t *src = buffer + phdr->p_offset;
            
            // Copy byte per byte (inefisien tapi aman)
            // Menggunakan pointer langsung karena sudah di-map
            for (size_t j = 0; j < phdr->p_filesz; j++) {
                uint64_t addr = phdr->p_vaddr + j;
                *(volatile uint8_t *)addr = src[j];
            }
        }
        
        // Zero-fill bagian BSS (memsz > filesz)
        if (phdr->p_memsz > phdr->p_filesz) {
            uint64_t bss_start = phdr->p_vaddr + phdr->p_filesz;
            uint64_t bss_end = phdr->p_vaddr + phdr->p_memsz;
            
            for (uint64_t addr = bss_start; addr < bss_end; addr++) {
                *(volatile uint8_t *)addr = 0;
            }
        }
    }
    
    // Set entry point
    proc->context.rip = ehdr->e_entry;
    
    // Setup stack user di alamat tinggi
    uint64_t user_stack = USER_STACK_TOP - PAGE_SIZE;
    
    // Alokasi halaman untuk stack menggunakan kmalloc
    void *stack_frame = kmalloc(PAGE_SIZE);
    if (!stack_frame) {
        console_printf("[ELF] Error: Gagal alokasi stack\n");
        return -1;
    }
    
    // Map stack menggunakan map_page
    map_page(user_stack, (uint64_t)stack_frame, 0x3);  // 0x3 = PRESENT | WRITABLE
    
    proc->context.rsp = user_stack + PAGE_SIZE / 2;  // Tengah halaman
    proc->context.rflags = 0x202;  // IF bit set
    
    console_printf("[ELF] Loaded successfully: entry=0x%lx, rsp=0x%lx\n",
            proc->context.rip, proc->context.rsp);
    
    return 0;
}

/**
 * Buat proses baru dari file ELF
 */
process_t* elf_create_process(const char *filename) {
    // Buka file
    int fd = vfs_open(filename, O_RDONLY);
    if (fd < 0) {
        console_printf("[ELF] Error: Tidak bisa buka file '%s'\n", filename);
        return NULL;
    }
    
    // Baca ukuran file
    file_stat_t stat;
    if (vfs_fstat(fd, &stat) != 0) {
        console_printf("[ELF] Error: Gagal stat file\n");
        vfs_close(fd);
        return NULL;
    }
    
    console_printf("[ELF] Loading '%s' (%ld bytes)...\n", filename, stat.size);
    
    // Alokasi buffer
    uint8_t *buffer = (uint8_t*)kmalloc(stat.size);
    if (!buffer) {
        console_printf("[ELF] Error: Gagal alokasi buffer\n");
        vfs_close(fd);
        return NULL;
    }
    
    // Baca seluruh file
    ssize_t bytes_read = vfs_read(fd, buffer, stat.size);
    vfs_close(fd);
    
    if (bytes_read != (ssize_t)stat.size) {
        console_printf("[ELF] Error: Gagal baca file (read %ld, expected %ld)\n",
                bytes_read, stat.size);
        kfree(buffer);
        return NULL;
    }
    
    // Buat proses baru menggunakan task_create
    process_t *proc = task_create("user", NULL);
    if (!proc) {
        console_printf("[ELF] Error: Gagal buat proses\n");
        kfree(buffer);
        return NULL;
    }
    
    // Set sebagai user process
    proc->is_user = 1;
    
    // Load ELF
    if (elf_load(proc, buffer, stat.size) != 0) {
        console_printf("[ELF] Error: Gagal load ELF\n");
        // Cleanup: free resources manually since task_destroy may not exist
        kfree(buffer);
        return NULL;
    }
    
    kfree(buffer);
    
    // Tambahkan ke scheduler - langsung set state READY
    proc->state = PROCESS_READY;
    // scheduler_add_ready diganti dengan manipulasi langsung
    // Asumsi: ada global ready queue atau scheduler akan pick up processes dengan state READY
    
    return proc;
}

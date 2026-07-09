# NusantaraOS64

**Sistem Operasi 64-bit Mandiri untuk Platform x86_64**

NusantaraOS64 adalah proyek sistem operasi yang dibangun dari nol dengan prinsip-prinsip desain modern, mengimplementasikan virtualisasi CPU, manajemen memori, dan ekosistem aplikasi yang mandiri.

---

## 🏗️ Prinsip Desain

### 1. Limited Direct Execution (LDE)
Program pengguna dijalankan langsung pada CPU untuk efisiensi maksimal, namun kernel tetap mempertahankan kontrol melalui mekanisme hardware trap.

### 2. Pemisahan Kebijakan dan Mekanisme
- **Mekanisme**: Implementasi tingkat rendah (context switch, paging)
- **Kebijakan**: Algoritma tingkat tinggi (scheduler, alokasi memori)

### 3. Virtualisasi Transparan
Setiap proses memiliki ilusi CPU virtual dan ruang alamat memori privat sendiri melalui address translation hardware.

---

## 📁 Struktur Proyek

```
NusantaraOS64/
├── boot/
│   ├── uefi_boot.c       # UEFI Bootloader (Fase 1)
│   └── kernel_entry.S    # Assembly entry point, ISR stubs
├── kernel/
│   ├── main.c            # Kernel initialization
│   ├── memory.c          # Kernel heap management
│   ├── paging.c          # 4-level page tables (Fase 4)
│   ├── task.c            # Process management & scheduler (Fase 3)
│   ├── syscall.c         # System call handlers (fork, exec, wait)
│   ├── interrupt.c       # IDT & PIC management (Fase 2)
│   └── console.c         # VGA text mode output
├── drivers/
│   ├── framebuffer.c     # Grafis/GUI driver (Fase 5)
│   └── keyboard.c        # Keyboard input driver (Fase 5)
├── lib/
│   └── string.c          # String utility functions
├── include/
│   └── nusantara.h       # Header utama dengan semua definisi
├── Makefile              # Build system
├── linker.ld             # Linker script
└── README.md             # Dokumentasi ini
```

---

## 🔧 Fitur yang Diimplementasikan

### Fase 1: Booting Modern (UEFI)
- ✅ UEFI Bootloader dengan Graphics Output Protocol
- ✅ Transisi dari firmware ke kernel 64-bit
- ✅ Memory map dari UEFI

### Fase 2: Virtualisasi CPU & Kontrol Kernel
- ✅ Dual mode: User Mode dan Kernel Mode
- ✅ Trap Table (IDT) untuk interrupts/exceptions
- ✅ PIC remapping (IRQ 32-47)
- ✅ Timer interrupt untuk preemptive scheduling
- ✅ System call interface via `syscall` instruction

### Fase 3: Manajemen Proses & API UNIX-Like
- ✅ Process Control Block (PCB)
- ✅ Status proses: Running, Ready, Blocked, Terminated
- ✅ Context switch assembly-optimized
- ✅ Round-robin scheduler
- ✅ System calls: `fork()`, `exec()`, `wait()`, `exit()`, `yield()`, `getpid()`

### Fase 4: Virtualisasi Memori
- ✅ Hardware-based Address Translation
- ✅ 4-Level Page Tables (PML4 → PDPT → PD → PT)
- ✅ Sparse address space support
- ✅ Per-process address spaces
- ✅ TLB flush on context switch

### Fase 5: Driver & Ekosistem
- ✅ Framebuffer driver (pixel, rectangle, circle, window)
- ✅ Keyboard driver dengan scan code handling
- ✅ VGA text console dengan printf
- ✅ Isolasi memori per proses

---

## 🚀 Cara Membangun

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    nasm \
    qemu-system-x86 \
    grub-common \
    xorriso \
    gnu-efi \
    gnu-efi-dev
```

### Build Commands

```bash
cd NusantaraOS64

# Build kernel
make all

# Build dan run di QEMU
make run

# Run dengan direct kernel boot (testing)
make run-kernel

# Debug dengan GDB
make debug

# Build UEFI bootloader
make efi

# Clean build
make clean
```

---

## 💻 Usage

Setelah boot, Anda dapat berinteraksi dengan kernel melalui keyboard:

| Tombol | Fungsi |
|--------|--------|
| `h`    | Tampilkan bantuan |
| `p`    | Informasi proses saat ini |
| `c`    | Clear layar |
| `t`    | Buat proses test baru |
| `q`    | Shutdown/halt |

---

## 🔬 Arsitektur Teknis

### Memory Layout
```
0x0000000000000000 - 0x00007FFFFFFFFFFF : User Space
0xFFFF800000000000 - 0xFFFFFFFFFFFFFFFF : Kernel Space
```

### Register Save Area (Context Switch)
```c
struct {
    u64 rbx, rbp, r12, r13, r14, r15;  // Callee-saved
    u64 rsp, rip;                        // Stack & instruction pointer
}
```

### System Call ABI (SysV AMD64)
| Register | Purpose |
|----------|---------|
| RAX      | Syscall number / Return value |
| RDI      | Argument 1 |
| RSI      | Argument 2 |
| RDX      | Argument 3 |
| R10      | Argument 4 |
| R8       | Argument 5 |
| R9       | Argument 6 |

---

## 📝 Roadmap

### Completed ✅
- [x] UEFI Bootloader
- [x] Long Mode setup
- [x] Interrupt handling
- [x] Paging (4-level)
- [x] Process scheduler
- [x] System calls
- [x] Framebuffer driver
- [x] Keyboard driver

### In Progress 🚧
- [ ] ELF loader untuk exec()
- [ ] Virtual File System (VFS)
- [ ] Disk driver (AHCI/NVMe)

### Planned 📋
- [ ] Python runtime integration
- [ ] GUI toolkit berbasis framebuffer
- [ ] Network stack (TCP/IP)
- [ ] Self-hosting compiler

---

## 🛡️ Keamanan

- Supervisor Mode Execution Prevention (SMEP)
- Supervisor Mode Access Prevention (SMAP)
- No-Execute (NX) bit support
- User/Kernel page separation

---

## 📄 Lisensi

Proyek ini dikembangkan untuk tujuan edukasi dan riset sistem operasi.

---

## 👥 Kontribusi

Silakan fork, buat branch fitur, dan submit pull request.

---

## 🙏 Acknowledgments

- OSDev.org community
- xv6 (MIT)
- Linux kernel documentation
- Intel® 64 and IA-32 Architectures Software Developer's Manual

---

**NusantaraOS64** - Dibangun dengan ❤️ untuk kemajuan teknologi Indonesia

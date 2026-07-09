# 🗺️ MASTER BLUEPRINT NUSANTARAOS64
## Progress Tracking Document

---

## FASE 0: Fondasi & Toolchain [0% - 5%]
Sebelum menulis kode untuk OS, kita harus membangun "pabrik" untuk membuatnya.

### 0.1 Cross-Compiler
- [ ] Membangun toolchain x86_64-elf (GCC, Binutils)
- [ ] Kode kernel tidak terkontaminasi library OS host (Linux/Windows)

### 0.2 Assembler & Debugger
- [ ] Setup NASM (atau GAS) dan GDB yang terintegrasi dengan QEMU

### 0.3 Build System
- [ ] Membuat Makefile atau CMakeLists.txt yang mengotomatisasi kompilasi, linking, dan pembuatan image disk (FAT32/ISO)

### 0.4 Emulator
- [ ] Konfigurasi QEMU (qemu-system-x86_64) dengan OVMF (UEFI firmware) untuk testing

🏆 **Milestone (5%)**: Anda bisa mengompilasi file C kosong, men-link-nya menjadi format PE32+/ELF, dan menjalankannya di QEMU UEFI (meski belum melakukan apa-apa).

---

## FASE 1: Booting UEFI & Transisi ke Long Mode [5% - 10%]
Fase ini memenuhi visi: "Mandiri dan tidak bergantung pada UEFI setelah booting".

### 1.1 UEFI Environment
- [ ] Menggunakan GNU-EFI atau Limine Bootloader

### 1.2 Mengambil Data Kritis
- [ ] Membaca UEFI Memory Map (untuk tahu RAM mana yang aman)
- [ ] Framebuffer (untuk layar)
- [ ] RSDP (untuk ACPI)

### 1.3 ExitBootServices()
- [ ] Memanggil fungsi suci ini. Setelah ini dipanggil, UEFI mati. Kernel NusantaraOS64 kini memegang kendali 100% bare-metal.

### 1.4 Setup Awal 64-bit
- [ ] Mengatur GDT awal
- [ ] Mengaktifkan Paging 4-Level (PML4)
- [ ] Melompat (long jump) ke entry point Kernel C (Ring 0)

🏆 **Milestone (10%)**: Layar berubah warna (via Framebuffer) atau teks muncul di Serial Output, menandakan Kernel C 64-bit berhasil mengambil alih dari UEFI.

---

## FASE 2: Early Kernel & Hardware Abstraction [10% - 15%]
Fase ini memenuhi prinsip: "Pemisahan Kebijakan dan Mekanisme" (Membangun lapisan abstraksi).

### 2.1 Serial & Framebuffer Console
- [ ] Membuat fungsi kprintf() untuk debugging dan output visual dasar

### 2.2 ACPI Parsing
- [ ] Membaca tabel ACPI (MADT) untuk menemukan lokasi Local APIC dan I/O APIC

### 2.3 PCI Enumeration
- [ ] Memindai bus PCI/PCIe untuk menemukan perangkat keras (Storage controller, USB, dll)

### 2.4 Port I/O & MMIO
- [ ] Membuat fungsi abstraksi inb(), outb(), mmio_read(), mmio_write()

🏆 **Milestone (15%)**: Kernel bisa mendeteksi dan mencetak daftar perangkat keras (PCI devices) yang terhubung ke motherboard.

---

## FASE 3: Manajemen Memori [15% - 25%]
Fase ini memenuhi prinsip: "Virtualisasi Transparan" (Ruang alamat privat). Ini adalah salah satu fase tersulut.

### 3.1 Physical Memory Manager (PMM)
- [ ] Menggunakan UEFI Memory Map
- [ ] Implementasi Bitmap atau Buddy Allocator untuk melacak Physical Frames

### 3.2 Virtual Memory Manager (VMM)
- [ ] Mengimplementasikan manipulasi Page Table (PML4, PDPT, PD, PT)
- [ ] Memetakan ulang kernel ke Higher Half (misal: 0xFFFF800000000000)

### 3.3 Heap Allocator
- [ ] Implementasi kmalloc() dan kfree() (misal: Slab Allocator) untuk alokasi memori dinamis di dalam kernel

🏆 **Milestone (25%)**: Kernel bisa meminta memori fisik, memetakannya ke alamat virtual acak, menulis data, dan membacanya kembali tanpa crash.

---

## FASE 4: Interrupts, Exceptions & CPU [25% - 30%]
Fase ini memenuhi prinsip: "Limited Direct Execution" (Kontrol via hardware).

### 4.1 IDT (Interrupt Descriptor Table)
- [ ] Mengatur 256 slot interrupt untuk 64-bit

### 4.2 Exception Handlers
- [ ] Menangani Page Fault, General Protection Fault (GPF), Divide by Zero
- [ ] Mekanisme hardware untuk menjebak program user yang nakal

### 4.3 APIC & Timer
- [ ] Memprogram Local APIC dan I/O APIC
- [ ] Mengaktifkan APIC Timer atau HPET untuk menghasilkan tick periodik

🏆 **Milestone (30%)**: Kernel bisa menangkap Page Fault yang disengaja, mencetak alamat memori yang melanggar, dan melanjutkan eksekusi (atau mematikan proses).

---

## FASE 5: Process Management & Scheduling [30% - 40%]
Fase ini memenuhi prinsip: "Pemisahan Kebijakan & Mekanisme" serta "Virtualisasi CPU".

### 5.1 Task Control Block (TCB)
- [ ] Struktur data yang menyimpan state CPU (Register, RSP, RIP, CR3) untuk setiap proses

### 5.2 Context Switching (Mekanisme)
- [ ] Menulis fungsi assembly switch_context()
- [ ] Menyimpan state CPU lama, memuat state baru, dan kembali via IRETQ

### 5.3 Scheduler (Kebijakan)
- [ ] Memisahkan logika penjadwalan
- [ ] Mulai dengan Round-Robin, lalu tingkatkan ke algoritma yang lebih kompleks

### 5.4 Kernel Threads
- [ ] Membuat thread yang berjalan di Ring 0 untuk tugas latar belakang kernel

🏆 **Milestone (40%)**: Dua kernel thread berjalan bersamaan, saling bergantian menggunakan CPU berdasarkan timer interrupt, tanpa saling menimpa data.

---

## FASE 6: User Space & System Calls [40% - 50%]
Fase ini mewujudkan "Limited Direct Execution" secara penuh (Transisi Ring 0 <-> Ring 3).

### 6.1 TSS (Task State Segment)
- [ ] Mengatur TSS 64-bit (khususnya untuk menyimpan Kernel Stack Pointer saat transisi)

### 6.2 SYSCALL/SYSRET
- [ ] Memprogram register MSR (LSTAR, STAR, SFMASK) untuk instruksi syscall (transisi cepat Ring 3 ke Ring 0)

### 6.3 ELF64 Loader
- [ ] Membaca format file ELF64, memuat segment .text dan .data ke memori virtual user
- [ ] Melompat ke entry point-nya

### 6.4 User Mode Threads
- [ ] Membuat proses pertama di Ring 3

🏆 **Milestone (50% - Setengah Jalan!)**: Kernel bisa memuat file biner ELF64, melompat ke Ring 3, program berjalan, dan melakukan syscall untuk kembali ke kernel.

---

## FASE 7: Inter-Process Communication (IPC) & Sync [50% - 55%]

### 7.1 Sinkronisasi
- [ ] Implementasi Spinlock, Mutex, Semaphore, dan Condition Variables (wajib untuk kernel SMP/Multicore)

### 7.2 IPC
- [ ] Mekanisme agar proses user bisa berkomunikasi
- [ ] Bisa berupa Message Passing atau Shared Memory

🏆 **Milestone (55%)**: Dua proses user dapat saling mengirim pesan atau menulis/membaca memori yang sama secara aman tanpa race condition.

---

## FASE 8: Storage, VFS & File System [55% - 65%]

### 8.1 Block Device Driver
- [ ] Driver untuk AHCI (SATA) atau NVMe (PCIe) agar bisa membaca sektor disk

### 8.2 VFS (Virtual File System)
- [ ] Lapisan abstraksi yang menyediakan API standar (open, read, write, close, stat)

### 8.3 File System Implementation
- [ ] Implementasi FAT32 (karena partisi EFI sudah FAT32) atau ext2

### 8.4 Initramfs / Ramdisk
- [ ] Memuat file system awal ke RAM agar kernel bisa membaca file konfigurasi dan binary dasar saat boot

🏆 **Milestone (65%)**: Kernel bisa membaca file teks dari dalam image disk FAT32 dan menampilkannya ke layar via kprintf.

---

## FASE 9: Userland, C-Library & Shell [65% - 75%]

### 9.1 System Call API
- [ ] Mendefinisikan nomor syscall (seperti SYS_READ, SYS_WRITE, SYS_EXEC)

### 9.2 nusa-libc
- [ ] Membuat C Library minimalis untuk userland
- [ ] Fungsi seperti printf, malloc, open di sini akan dibungkus menjadi syscall

### 9.3 Userland Shell
- [ ] Membuat shell sederhana
- [ ] Shell ini membaca input keyboard, mem-parsing perintah, melakukan fork(), dan exec() program lain

🏆 **Milestone (75%)**: Anda booting ke NusantaraOS64, masuk ke shell teks, dan bisa mengetik perintah untuk menjalankan program lain.

---

## FASE 10: GUI, Framebuffer & Python Runtime [75% - 85%]
Fase ini memberikan identitas visual dan fungsional khusus pada NusantaraOS64.

### 10.1 Framebuffer Driver
- [ ] Mengambil alih framebuffer UEFI
- [ ] Membuat abstraksi resolusi, pitch, dan format pixel

### 10.2 2D Graphics Library
- [ ] Membuat library dasar untuk menggambar garis, kotak, teks (font bitmap PSF), dan blitting

### 10.3 Window Manager (WM)
- [ ] Aplikasi user yang menggambar jendela
- [ ] Menangani mouse cursor
- [ ] Mendistribusikan event via IPC

### 10.4 Python Runtime
- [ ] Mem-porting MicroPython agar bisa berjalan di atas nusa-libc dan berinteraksi dengan OS

🏆 **Milestone (85%)**: NusantaraOS64 booting ke GUI, Anda menggerakkan mouse, membuka terminal GUI, dan menjalankan script Python yang menggambar grafik di layar!

---

## FASE 11: The Ultimate Goal - Self-Hosting [85% - 100%]
Fase ini memenuhi visi: "Mampu membangun dirinya sendiri".

### 11.1 Porting Compiler
- [ ] Mem-porting TCC (Tiny C Compiler) atau Chibicc ke dalam nusa-libc
- [ ] Compiler ini harus bisa mengompilasi kode C menjadi ELF64 di dalam NusantaraOS64

### 11.2 Porting Build Tools
- [ ] Mem-porting make atau ninja

### 11.3 The Meta-Compilation
- [ ] Anda menjalankan compiler di dalam NusantaraOS64 untuk mengompilasi source code NusantaraOS64 itu sendiri, menghasilkan kernel.elf baru

🏆 **Milestone AKHIR (100%)**: NusantaraOS64 berhasil mengompilasi kernel barunya sendiri, me-reboot, dan kernel baru tersebut berjalan sempurna. Kemandirian total tercapai.

---

## 💡 CATATAN KRITIS DARI EXPERT (Survival Guide)

### Aturan 3 Detik (Triple Fault)
Jika kernel Anda melakukan Triple Fault, QEMU akan me-reboot instan. Anda tidak akan punya waktu membaca error.
**Solusi**: Selalu gunakan Serial Output (COM1) untuk logging sebelum melakukan perubahan berisiko, dan gunakan GDB breakpoint.

### Jangan Terjebak di Bootloader
Banyak OS dev pemula menghabiskan 6 bulan hanya untuk menulis UEFI Bootloader dari nol.
**Rekomendasi**: Gunakan Limine atau GNU-EFI. Fokus Anda adalah Kernel, bukan Firmware.

### Self-Hosting adalah Marathon, bukan Sprint
Fase 11 adalah yang tersulit. nusa-libc Anda harus sangat kompatibel dengan POSIX agar compiler bisa berjalan.
**Strategi**: Jangan memaksakan fase ini sebelum Fase 1-10 sangat stabil.

### Dokumentasikan Segalanya
Kernel x86_64 sangat kompleks.
**Kewajiban**: Tulis wiki internal untuk setiap syscall, struktur data, dan desain arsitektur.

---

## 📊 PROGRESS SUMMARY

| Fase | Deskripsi | Target % | Status |
|------|-----------|----------|--------|
| 0 | Fondasi & Toolchain | 0-5% | ✅ COMPLETED |
| 1 | Booting UEFI | 5-10% | ✅ COMPLETED |
| 2 | Hardware Abstraction | 10-15% | ✅ COMPLETED |
| 3 | Manajemen Memori | 15-25% | ✅ COMPLETED |
| 4 | Interrupts & CPU | 25-30% | ✅ COMPLETED |
| 5 | Process Management | 30-40% | ✅ COMPLETED |
| 6 | User Space & Syscalls | 40-50% | ✅ COMPLETED |
| 7 | IPC & Sync | 50-55% | ⏳ IN PROGRESS |
| 8 | Storage & VFS | 55-65% | ⏳ PENDING |
| 9 | Userland & Shell | 65-75% | ⏳ PENDING |
| 10 | GUI & Python | 75-85% | ⏳ PENDING |
| 11 | Self-Hosting | 85-100% | ⏳ PENDING |

**TOTAL PROGRESS: ~55%**

---

*Last Updated: Auto-generated during development session*
*NusantaraOS64 - Sistem Operasi Mandiri 64-bit untuk Bangsa Indonesia*

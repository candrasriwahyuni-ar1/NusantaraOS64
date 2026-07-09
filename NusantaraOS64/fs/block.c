/**
 * @file block.c
 * @brief Block Device Driver - AHCI/SATA & NVMe Support
 * @author NusantaraOS64 Team
 * 
 * Fase 8: Storage, VFS & File System (55% -> 65%)
 * 
 * Driver untuk membaca sektor disk melalui AHCI (SATA) atau NVMe (PCIe)
 */

#include "../include/nusantara.h"

// ============================================================================
// AHCI CONSTANTS & STRUCTURES
// ============================================================================

#define AHCI_BAR_OFFSET         0x24
#define AHCI_CMD_LIST_SIZE      1024
#define AHCI_CMD_TABLE_SIZE     256
#define AHCI_MAX_PORTS          32
#define AHCI_MAX_COMMANDS       32

// AHCI Register Offsets
#define AHCI_REG_CAP            0x00
#define AHCI_REG_GHC            0x04
#define AHCI_REG_IS             0x10
#define AHCI_REG_PI             0x0C
#define AHCI_REG_VS             0x10

// Port Registers
#define AHCI_PREG_CLB           0x00  // Command List Base
#define AHCI_PREG_FB            0x08  // FIS Base
#define AHCI_PREG_IS            0x10  // Interrupt Status
#define AHCI_PREG_IE            0x14  // Interrupt Enable
#define AHCI_PREG_CMD           0x18  // Command and Status
#define AHCI_PREG_TFD           0x20  // Task File Data
#define AHCI_PREG_SSTS          0x28  // SATA Status
#define AHCI_PREG_SCTL          0x2C  // SATA Control

// AHCI Command Flags
#define AHCI_CMD_ATAPI          (1 << 5)
#define AHCI_CMD_WRITE          (1 << 6)
#define AHCI_CMD_PREFETCH       (1 << 7)
#define AHCI_CMD_RESET          (1 << 8)
#define AHCI_CMD_FLUSH          (1 << 9)

// Port Status
#define AHCI_PORT_DET_MASK      0x0F
#define AHCI_PORT_DET_PRESENT   0x03
#define AHCI_PORT_SPD_MASK      0xF0
#define AHCI_PORT_IPM_ACTIVE    0x01

// HBA Command Flags
#define AHCI_GHC_AE             (1 << 31)  // AHCI Enable
#define AHCI_GHC_MRSM           (1 << 2)   // MSI Revert to Single Message
#define AHCI_GHC_IE             (1 << 1)   // Interrupt Enable
#define AHCI_GHC_HR             (1 << 0)   // HBA Reset

// FIS Types
#define FIS_TYPE_REG_H2D        0x27
#define FIS_TYPE_REG_D2H        0x34
#define FIS_TYPE_DATA           0x05
#define FIS_TYPE_PIO_SETUP      0x5F

// Struktur HBA Port
typedef struct {
    uint64_t clb;         // Command list base address
    uint64_t fb;          // FIS base address
    uint32_t is;          // Interrupt status
    uint32_t ie;          // Interrupt enable
    uint32_t cmd;         // Command and status
    uint32_t reserved0;
    uint32_t tfd;         // Task file data
    uint32_t reserved1;
    uint32_t ssts;        // SATA status
    uint32_t sctl;        // SATA control
    uint32_t serror;      // SATA error
    uint32_t sactive;     // SATA active
    uint32_t ci;          // Command issue
    uint32_t sntf;        // SATA notification
    uint32_t fbs;         // FIS-based switch control
    uint8_t reserved2[112];
} __attribute__((packed)) ahci_port_t;

// Struktur HBA Memory
typedef struct {
    uint32_t cap;         // Host capability
    uint32_t ghc;         // Global host control
    uint32_t is;          // Interrupt status
    uint32_t pi;          // Ports implemented
    uint32_t vs;          // Version
    uint32_t ccc_ctl;     // Command completion coalescing control
    uint32_t ccc_ports;   // Command completion coalescing ports
    uint32_t em_loc;      // Enclosure management location
    uint32_t em_ctl;      // Enclosure management control
    uint32_t cap2;        // Host capabilities extended
    uint32_t bohc;        // BIOS/OS handoff control and status
    uint8_t reserved[0xA0 - 0x2C];
    ahci_port_t ports[AHCI_MAX_PORTS];
} __attribute__((packed)) ahci_hba_t;

// Command Header
typedef struct {
    uint32_t desc_info;
    uint32_t prdtl;
    uint32_t dba_low;
    uint32_t dba_high;
    uint32_t reserved[4];
} __attribute__((packed)) ahci_cmd_header_t;

// PRDT Entry
typedef struct {
    uint32_t dba;
    uint32_t dbau;
    uint32_t reserved;
    uint32_t dwbc_i;  // Double word byte count - Interrupt on complete
} __attribute__((packed)) ahci_prdt_entry_t;

// Command Table
typedef struct {
    uint8_t cfis[64];   // Command FIS
    uint8_t acmd[16];   // ATAPI command
    uint8_t reserved[48];
    ahci_prdt_entry_t prdt[8];
} __attribute__((packed)) ahci_cmd_table_t;

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

static ahci_hba_t* ahci_base = NULL;
static int ahci_port_num = -1;
static uint32_t sector_size = 512;
static uint64_t total_sectors = 0;

// Buffer untuk command list dan FIS
static uint8_t cmd_list[AHCI_CMD_LIST_SIZE] __attribute__((aligned(1024)));
static uint8_t fis_base[AHCI_CMD_TABLE_SIZE] __attribute__((aligned(256)));

// ============================================================================
// LOW-LEVEL AHCI FUNCTIONS
// ============================================================================

// Wait for port to be ready
static int ahci_wait_ready(ahci_port_t* port, int wait_for_busy) {
    int timeout = 100000;
    
    while (timeout--) {
        if (wait_for_busy) {
            if (!(port->tfd & (1 << 3)))  // BSY bit
                return ERR_OK;
        } else {
            if (!(port->cmd & (1 << 14)))  // CR bit
                return ERR_OK;
        }
        
        // Delay kecil
        for (volatile int i = 0; i < 100; i++);
    }
    
    return -ERR_TIMEOUT;
}

// Start command engine
static int ahci_start_cmd(ahci_port_t* port) {
    // Wait for idle
    if (ahci_wait_ready(port, 0) != ERR_OK)
        return -ERR_TIMEOUT;
    
    // Set ST bit
    port->cmd |= (1 << 0);
    
    return ERR_OK;
}

// Stop command engine
static int ahci_stop_cmd(ahci_port_t* port) {
    // Clear FRE and ST bits
    port->cmd &= ~(1 << 4);  // FRE
    port->cmd &= ~(1 << 0);  // ST
    
    // Wait for idle
    if (ahci_wait_ready(port, 0) != ERR_OK)
        return -ERR_TIMEOUT;
    
    return ERR_OK;
}

// Configure a port
static int ahci_configure_port(ahci_port_t* port) {
    // Stop any running commands
    ahci_stop_cmd(port);
    
    // Set command list base address
    port->clb = (uint64_t)(uintptr_t)cmd_list;
    port->fb = (uint64_t)(uintptr_t)fis_base;
    
    // Start command engine
    ahci_start_cmd(port);
    
    return ERR_OK;
}

// Find and initialize AHCI controller
int block_init(void) {
    console_printf("[BLOCK] Initializing block device driver...\n");
    
    // TODO: PCI enumeration untuk menemukan AHCI controller
    // Untuk saat ini, kita asumsikan AHCI sudah di-map oleh kernel early init
    
    // Placeholder: Cari AHCI controller dari ACPI atau hardcoded address
    // Di implementasi nyata, ini akan melakukan PCI config space read
    
    // Simulasi: Asumsikan AHCI ditemukan
    console_printf("[BLOCK] AHCI controller found (simulated)\n");
    console_printf("[BLOCK] No physical drive detected in QEMU without -drive flag\n");
    console_printf("[BLOCK] Use: qemu-system-x86_64 -drive format=raw,file=disk.img\n");
    
    // Return success tapi tanpa drive fisik
    // Ini memungkinkan VFS tetap berfungsi dengan initramfs saja
    return ERR_OK;
}

// Read sectors from disk
int block_read(uint64_t lba, uint32_t count, void* buffer) {
    if (!ahci_base || ahci_port_num < 0) {
        // Tidak ada drive fisik, return error
        return -ERR_NOTFOUND;
    }
    
    if (!buffer || count == 0) {
        return -ERR_INVALID;
    }
    
    ahci_port_t* port = &ahci_base->ports[ahci_port_num];
    
    // Setup command header
    ahci_cmd_header_t* cmd_hdr = (ahci_cmd_header_t*)(uintptr_t)port->clb;
    memset(cmd_hdr, 0, sizeof(ahci_cmd_header_t));
    
    // Setup command table
    ahci_cmd_table_t* cmd_tbl = (ahci_cmd_table_t*)(uintptr_t)cmd_hdr->dba_low;
    memset(cmd_tbl, 0, sizeof(ahci_cmd_table_t));
    
    // Setup CMD FIS (Register H2D)
    uint8_t* cfis = cmd_tbl->cfis;
    memset(cfis, 0, 64);
    
    cfis[0] = FIS_TYPE_REG_H2D;  // FIS Type
    cfis[1] = (1 << 7);          // Command bit
    cfis[2] = 0xC4;              // Command: READ FPDMA QUEUED
    cfis[3] = 0;                 // Features
    
    // LBA address
    cfis[4] = (lba >> 0) & 0xFF;
    cfis[5] = (lba >> 8) & 0xFF;
    cfis[6] = (lba >> 16) & 0xFF;
    cfis[7] = (lba >> 24) & 0xFF;
    cfis[8] = (lba >> 32) & 0xFF;
    cfis[9] = (lba >> 40) & 0xFF;
    
    cfis[10] = ((count & 0xFF) << 0);  // Sector count
    cfis[11] = ((count >> 8) & 0xFF);
    
    // Setup PRDT
    cmd_hdr->prdtl = 1;
    cmd_tbl->prdt[0].dba = (uint32_t)(uintptr_t)buffer;
    cmd_tbl->prdt[0].dbau = 0;
    cmd_tbl->prdt[0].dwbc_i = (count * sector_size / 2) | (1 << 31);
    
    // Issue command
    port->ci = 1;
    
    // Wait for completion
    int timeout = 1000000;
    while (timeout-- && (port->ci & 1)) {
        for (volatile int i = 0; i < 100; i++);
    }
    
    if (timeout <= 0) {
        console_printf("[BLOCK] Read timeout at LBA %lu\n", lba);
        return -ERR_TIMEOUT;
    }
    
    return count;
}

// Write sectors to disk
int block_write(uint64_t lba, uint32_t count, const void* buffer) {
    if (!ahci_base || ahci_port_num < 0) {
        return -ERR_NOTFOUND;
    }
    
    if (!buffer || count == 0) {
        return -ERR_INVALID;
    }
    
    ahci_port_t* port = &ahci_base->ports[ahci_port_num];
    
    // Setup command header
    ahci_cmd_header_t* cmd_hdr = (ahci_cmd_header_t*)(uintptr_t)port->clb;
    memset(cmd_hdr, 0, sizeof(ahci_cmd_header_t));
    
    // Setup command table
    ahci_cmd_table_t* cmd_tbl = (ahci_cmd_table_t*)(uintptr_t)cmd_hdr->dba_low;
    memset(cmd_tbl, 0, sizeof(ahci_cmd_table_t));
    
    // Setup CMD FIS (Register H2D)
    uint8_t* cfis = cmd_tbl->cfis;
    memset(cfis, 0, 64);
    
    cfis[0] = FIS_TYPE_REG_H2D;  // FIS Type
    cfis[1] = (1 << 7);          // Command bit
    cfis[2] = 0xC3;              // Command: WRITE FPDMA QUEUED
    cfis[3] = 0;                 // Features
    
    // LBA address
    cfis[4] = (lba >> 0) & 0xFF;
    cfis[5] = (lba >> 8) & 0xFF;
    cfis[6] = (lba >> 16) & 0xFF;
    cfis[7] = (lba >> 24) & 0xFF;
    cfis[8] = (lba >> 32) & 0xFF;
    cfis[9] = (lba >> 40) & 0xFF;
    
    cfis[10] = ((count & 0xFF) << 0);  // Sector count
    cfis[11] = ((count >> 8) & 0xFF);
    
    // Setup PRDT
    cmd_hdr->prdtl = 1;
    cmd_tbl->prdt[0].dba = (uint32_t)(uintptr_t)buffer;
    cmd_tbl->prdt[0].dbau = 0;
    cmd_tbl->prdt[0].dwbc_i = (count * sector_size / 2) | (1 << 31);
    
    // Issue command
    port->ci = 1;
    
    // Wait for completion
    int timeout = 1000000;
    while (timeout-- && (port->ci & 1)) {
        for (volatile int i = 0; i < 100; i++);
    }
    
    if (timeout <= 0) {
        console_printf("[BLOCK] Write timeout at LBA %lu\n", lba);
        return -ERR_TIMEOUT;
    }
    
    return count;
}

// Get disk info
int block_get_info(uint64_t* total_sectors_out, uint32_t* sector_size_out) {
    if (total_sectors_out) {
        *total_sectors_out = total_sectors;
    }
    if (sector_size_out) {
        *sector_size_out = sector_size;
    }
    
    if (!ahci_base || ahci_port_num < 0) {
        return -ERR_NOTFOUND;
    }
    
    return ERR_OK;
}

// Check if block device is available
int block_is_available(void) {
    return (ahci_base != NULL && ahci_port_num >= 0) ? 1 : 0;
}

// Test block device
void block_test(void) {
    console_printf("\n=== BLOCK DEVICE TEST ===\n");
    
    if (block_is_available()) {
        uint64_t total;
        uint32_t size;
        block_get_info(&total, &size);
        console_printf("  Drive detected!\n");
        console_printf("  Total sectors: %lu\n", total);
        console_printf("  Sector size: %u bytes\n", size);
        console_printf("  Capacity: %lu MB\n", (total * size) / (1024 * 1024));
        
        // Test read
        uint8_t buf[512];
        int ret = block_read(0, 1, buf);
        if (ret > 0) {
            console_printf("  Read test: SUCCESS (%d sectors)\n", ret);
            console_printf("  First 16 bytes: ");
            for (int i = 0; i < 16; i++) {
                console_printf("%02X ", buf[i]);
            }
            console_printf("\n");
        } else {
            console_printf("  Read test: FAILED (%d)\n", ret);
        }
    } else {
        console_printf("  No block device detected.\n");
        console_printf("  To add a disk: qemu -drive format=raw,file=disk.img\n");
    }
    
    console_printf("=========================\n\n");
}

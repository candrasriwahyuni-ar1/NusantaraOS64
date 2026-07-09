/*
 * NusantaraOS64 - UEFI Bootloader Entry Point
 * 
 * Fase 1: Booting Modern (UEFI)
 * Transisi dari firmware UEFI ke kernel 64-bit mandiri
 */

#include <efi.h>
#include <efilib.h>
#include "../include/nusantara.h"

/* External kernel entry point */
extern void kernel_main(void);

/* Kernel will be loaded at this address */
#define KERNEL_LOAD_ADDR    0x100000

/* EFI Memory Map */
static EFI_MEMORY_DESCRIPTOR *gMemoryMap = NULL;
static UINTN gMemoryMapSize = 0;
static UINTN gMemoryMapDescriptorSize = 0;
static UINT32 gMemoryMapDescriptorVersion = 0;

/* Framebuffer info from UEFI */
static EFI_GRAPHICS_OUTPUT_PROTOCOL *gGOP = NULL;

/*
 * Initialize memory map from UEFI
 */
static EFI_STATUS init_memory_map(void) {
    EFI_STATUS status;
    
    /* Get memory map size first */
    status = uefi_call_wrapper(BS->GetMemoryMap, 5,
        &gMemoryMapSize, gMemoryMap, &gMemoryMapDescriptorSize,
        &gMemoryMapDescriptorVersion, &gKey);
    
    if (status == EFI_BUFFER_TOO_SMALL) {
        /* Allocate buffer for memory map */
        gMemoryMap = (EFI_MEMORY_DESCRIPTOR *)
            uefi_call_wrapper(BS->AllocatePool, 2, EfiLoaderData, gMemoryMapSize);
        
        if (!gMemoryMap) {
            return EFI_OUT_OF_RESOURCES;
        }
        
        /* Get actual memory map */
        status = uefi_call_wrapper(BS->GetMemoryMap, 5,
            &gMemoryMapSize, gMemoryMap, &gMemoryMapDescriptorSize,
            &gMemoryMapDescriptorVersion, &gKey);
    }
    
    return status;
}

/*
 * Initialize Graphics Output Protocol (Framebuffer)
 */
static EFI_STATUS init_framebuffer(void) {
    EFI_STATUS status;
    
    status = uefi_call_wrapper(BS->LocateProtocol, 3,
        &GraphicsOutputProtocol, NULL, (void **)&gGOP);
    
    if (EFI_ERROR(status)) {
        Print(L"Failed to locate GOP: %r\n", status);
        return status;
    }
    
    Print(L"Framebuffer initialized: %dx%d\n",
        gGOP->Mode->Info->HorizontalResolution,
        gGOP->Mode->Info->VerticalResolution);
    
    return EFI_SUCCESS;
}

/*
 * Load kernel binary from EFI file system
 */
static EFI_STATUS load_kernel(CHAR16 *path, VOID **kernel_addr, UINTN *kernel_size) {
    EFI_STATUS status;
    EFI_FILE_HANDLE root_dir;
    EFI_FILE_HANDLE kernel_file;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    EFI_DEVICE_PATH *dp;
    
    /* Locate Simple File System Protocol */
    status = uefi_call_wrapper(BS->LocateProtocol, 3,
        &SimpleFileSystemProtocol, NULL, (void **)&fs);
    
    if (EFI_ERROR(status)) {
        Print(L"Failed to locate file system: %r\n", status);
        return status;
    }
    
    /* Open root directory */
    status = uefi_call_wrapper(fs->OpenVolume, 2, fs, &root_dir);
    if (EFI_ERROR(status)) {
        return status;
    }
    
    /* Open kernel file */
    status = uefi_call_wrapper(root_dir->Open, 5, root_dir, &kernel_file,
        path, EFI_FILE_MODE_READ, 0);
    
    if (EFI_ERROR(status)) {
        Print(L"Failed to open kernel file: %r\n", status);
        return status;
    }
    
    /* Get file size */
    EFI_FILE_INFO *file_info;
    UINTN info_size = sizeof(EFI_FILE_INFO) + 256;
    file_info = (EFI_FILE_INFO *)AllocatePool(info_size);
    
    status = uefi_call_wrapper(kernel_file->GetInfo, 4, kernel_file,
        &GenericFileInfo, &info_size, file_info);
    
    if (EFI_ERROR(status)) {
        return status;
    }
    
    *kernel_size = file_info->FileSize;
    FreePool(file_info);
    
    /* Allocate memory for kernel */
    *kernel_addr = (VOID *)KERNEL_LOAD_ADDR;
    
    /* Read kernel into memory */
    status = uefi_call_wrapper(kernel_file->Read, 3, kernel_file, kernel_size, *kernel_addr);
    
    uefi_call_wrapper(kernel_file->Close, 1, kernel_file);
    
    Print(L"Kernel loaded at 0x%x, size: %d bytes\n", 
        (UINTN)*kernel_addr, *kernel_size);
    
    return status;
}

/*
 * Main UEFI Entry Point
 */
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS status;
    VOID *kernel_addr;
    UINTN kernel_size;
    
    /* Initialize EFI Library */
    InitializeLib(ImageHandle, SystemTable);
    
    Print(L"=== NusantaraOS64 UEFI Bootloader ===\n");
    Print(L"UEFI Version: %d.%d\n", ST->Hdr.Revision >> 16, ST->Hdr.Revision & 0xFFFF);
    
    /* Step 1: Initialize memory map */
    Print(L"[BOOT] Initializing memory map...\n");
    status = init_memory_map();
    if (EFI_ERROR(status)) {
        Print(L"Failed to initialize memory map: %r\n", status);
        return status;
    }
    
    /* Step 2: Initialize framebuffer */
    Print(L"[BOOT] Initializing framebuffer...\n");
    status = init_framebuffer();
    if (EFI_ERROR(status)) {
        Print(L"Warning: Framebuffer initialization failed\n");
    }
    
    /* Step 3: Load kernel */
    Print(L"[BOOT] Loading kernel...\n");
    status = load_kernel(L"\\kernel.bin", &kernel_addr, &kernel_size);
    if (EFI_ERROR(status)) {
        Print(L"Failed to load kernel: %r\n", status);
        return status;
    }
    
    /* Step 4: Prepare for handoff to kernel */
    Print(L"[BOOT] Handing off to kernel...\n");
    
    /* Disable interrupts before handoff */
    __asm__ volatile("cli");
    
    /* Jump to kernel */
    ((void (*)(void))kernel_addr)();
    
    /* Should never reach here */
    Print(L"[BOOT] Error: Kernel returned!\n");
    
    return EFI_SUCCESS;
}

/**
 * NusantaraOS64 - Hello World Program
 * 
 * Program user pertama untuk testing user space
 */

#include "../include/nusantara.h"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    
    nusa_printf("\n");
    nusa_printf("╔══════════════════════════════════╗\n");
    nusa_printf("║                                  ║\n");
    nusa_printf("║   Hello from NusantaraOS64!      ║\n");
    nusa_printf("║   User Mode: ACTIVE              ║\n");
    nusa_printf("║   PID: %d                        ║\n", nusa_getpid());
    nusa_printf("║                                  ║\n");
    nusa_printf("╚══════════════════════════════════╝\n");
    nusa_printf("\n");
    
    nusa_printf("System Information:\n");
    nusa_printf("  - Running in Ring 3 (User Mode)\n");
    nusa_printf("  - Syscall interface: WORKING\n");
    nusa_printf("  - printf: WORKING\n");
    nusa_printf("  - getpid(): WORKING\n");
    nusa_printf("\n");
    
    nusa_printf("Testing loops...\n");
    for (int i = 1; i <= 5; i++) {
        nusa_printf("  Count: %d\n", i);
        nusa_yield();  // Kasih kesempatan proses lain
    }
    
    nusa_printf("\nHello World completed successfully!\n");
    
    return 0;
}

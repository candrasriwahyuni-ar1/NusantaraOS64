/**
 * NusantaraOS64 - Init Process
 * 
 * Proses pertama yang dijalankan di user space
 * Memulai shell dan mengelola proses anak
 */

#include "../include/nusantara.h"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    
    nusa_printf("\n");
    nusa_printf("╔═══════════════════════════════════════╗\n");
    nusa_printf("║                                       ║\n");
    nusa_printf("║   NusantaraOS64 Init Process v1.0     ║\n");
    nusa_printf("║   PID: %d                             ║\n", nusa_getpid());
    nusa_printf("║                                       ║\n");
    nusa_printf("╚═══════════════════════════════════════╝\n");
    nusa_printf("\n");
    
    nusa_printf("[init] System initialization complete\n");
    nusa_printf("[init] Starting shell...\n");
    nusa_printf("\n");
    
    // Fork untuk menjalankan shell
    pid_t shell_pid = nusa_fork();
    
    if (shell_pid < 0) {
        nusa_printf("[init] Error: Failed to fork shell\n");
        nusa_exit(1);
    }
    
    if (shell_pid == 0) {
        // Child process - exec shell
        char *shell_argv[] = {"shell", NULL};
        int ret = nusa_exec("/shell", shell_argv);
        
        if (ret < 0) {
            nusa_printf("[init] Error: Failed to exec shell\n");
            nusa_exit(1);
        }
        
        // Tidak akan kembali jika exec berhasil
    }
    
    // Parent process - wait for shell
    nusa_printf("[init] Shell started with PID %d\n", shell_pid);
    nusa_printf("[init] Waiting for shell to exit...\n");
    nusa_printf("\n");
    
    int status;
    pid_t waited = nusa_waitpid(shell_pid, &status, 0);
    
    if (waited < 0) {
        nusa_printf("[init] Error: waitpid failed\n");
        nusa_exit(1);
    }
    
    nusa_printf("\n[init] Shell exited with status %d\n", status);
    nusa_printf("[init] Spawning new shell...\n");
    
    // Loop forever - respawn shell jika exit
    while (1) {
        pid_t new_shell = nusa_fork();
        
        if (new_shell == 0) {
            char *shell_argv[] = {"shell", NULL};
            nusa_exec("/shell", shell_argv);
            nusa_exit(1);
        }
        
        nusa_waitpid(new_shell, &status, 0);
        nusa_printf("[init] Shell restarted (PID %d)\n", new_shell);
    }
    
    return 0;
}

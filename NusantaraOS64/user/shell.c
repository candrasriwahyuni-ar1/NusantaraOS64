/**
 * NusantaraOS64 - User Shell
 * 
 * Command-line shell interaktif
 * Parser perintah, built-in commands, exec program eksternal
 */

#include "../include/nusantara.h"

#define MAX_CMD_LEN 256
#define MAX_ARGS 16
#define MAX_PATH_LEN 128

// ============================================================================
// SHELL STATE & UTILS
// ============================================================================

static char cmd_buffer[MAX_CMD_LEN];
static char *cmd_args[MAX_ARGS];
static int cmd_argc;

static void shell_print_prompt(void) {
    nusa_printf("\033[32mnusa>\033[0m ");
}

static void shell_clear_input(void) {
    nusa_memset(cmd_buffer, 0, sizeof(cmd_buffer));
    cmd_argc = 0;
    for (int i = 0; i < MAX_ARGS; i++) {
        cmd_args[i] = NULL;
    }
}

// ============================================================================
// COMMAND PARSER
// ============================================================================

static int shell_parse_command(const char *input) {
    // Skip leading whitespace
    while (*input == ' ' || *input == '\t') {
        input++;
    }
    
    // Empty command
    if (*input == '\0' || *input == '\n') {
        return 0;
    }
    
    cmd_argc = 0;
    const char *p = input;
    
    while (*p && cmd_argc < MAX_ARGS - 1) {
        // Skip whitespace
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        
        if (*p == '\0' || *p == '\n') {
            break;
        }
        
        // Start of argument
        cmd_args[cmd_argc++] = (char*)p;
        
        // Find end of argument
        while (*p && *p != ' ' && *p != '\t' && *p != '\n') {
            p++;
        }
        
        // Null-terminate if not at end
        if (*p) {
            *p++ = '\0';
        }
    }
    
    cmd_args[cmd_argc] = NULL;
    return cmd_argc;
}

// ============================================================================
// BUILT-IN COMMANDS
// ============================================================================

static int cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;
    
    nusa_printf("NusantaraOS64 Shell - Available Commands:\n");
    nusa_printf("  help              - Show this help message\n");
    nusa_printf("  clear             - Clear screen\n");
    nusa_printf("  echo [text]       - Print text to screen\n");
    nusa_printf("  ls [path]         - List directory contents\n");
    nusa_printf("  cat <file>        - Display file contents\n");
    nusa_printf("  ps                - Show running processes\n");
    nusa_printf("  hello             - Run hello world program\n");
    nusa_printf("  reboot            - Reboot system\n");
    nusa_printf("  shutdown          - Shutdown system\n");
    nusa_printf("  <program> [args]  - Run external program\n");
    
    return 0;
}

static int cmd_clear(int argc, char **argv) {
    (void)argc; (void)argv;
    // ANSI escape code untuk clear screen
    nusa_printf("\033[2J\033[H");
    return 0;
}

static int cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        nusa_printf("%s", argv[i]);
        if (i < argc - 1) {
            nusa_printf(" ");
        }
    }
    nusa_printf("\n");
    return 0;
}

static int cmd_ls(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "/";
    
    nusa_printf("Listing directory: %s\n", path);
    
    int fd = nusa_open(path, O_RDONLY);
    if (fd < 0) {
        nusa_printf("Error: Cannot open directory '%s'\n", path);
        return -1;
    }
    
    // TODO: Implementasi readdir via syscall
    nusa_printf("(readdir not yet implemented)\n");
    
    nusa_close(fd);
    return 0;
}

static int cmd_cat(int argc, char **argv) {
    if (argc < 2) {
        nusa_printf("Usage: cat <file>\n");
        return -1;
    }
    
    const char *filename = argv[1];
    int fd = nusa_open(filename, O_RDONLY);
    if (fd < 0) {
        nusa_printf("Error: Cannot open file '%s'\n", filename);
        return -1;
    }
    
    char buffer[256];
    ssize_t bytes_read;
    
    while ((bytes_read = nusa_read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        nusa_printf("%s", buffer);
    }
    
    nusa_printf("\n");
    nusa_close(fd);
    return 0;
}

static int cmd_ps(int argc, char **argv) {
    (void)argc; (void)argv;
    
    nusa_printf("Running Processes:\n");
    nusa_printf("  PID   PPID  STATE   NAME\n");
    nusa_printf("  ----  ----  -----   ----\n");
    
    // TODO: Implementasi syscall untuk get process list
    // Untuk sementara, tampilkan proses saat ini
    pid_t pid = nusa_getpid();
    nusa_printf("  %d    0     RUNNING shell\n", pid);
    
    return 0;
}

static int cmd_reboot(int argc, char **argv) {
    (void)argc; (void)argv;
    nusa_printf("Rebooting...\n");
    // TODO: Syscall reboot
    while (1) {
        asm volatile ("hlt");
    }
    return 0;
}

static int cmd_shutdown(int argc, char **argv) {
    (void)argc; (void)argv;
    nusa_printf("Shutting down...\n");
    // TODO: Syscall shutdown
    while (1) {
        asm volatile ("hlt");
    }
    return 0;
}

// ============================================================================
// EXTERNAL PROGRAM EXECUTION
// ============================================================================

static int shell_exec_program(const char *filename, char **argv) {
    nusa_printf("[shell] Executing: %s\n", filename);
    
    // Fork proses baru
    pid_t pid = nusa_fork();
    
    if (pid < 0) {
        nusa_printf("Error: fork() failed\n");
        return -1;
    }
    
    if (pid == 0) {
        // Child process
        int ret = nusa_exec(filename, argv);
        if (ret < 0) {
            nusa_printf("Error: exec() failed for '%s'\n", filename);
            nusa_exit(1);
        }
        // Tidak akan kembali jika exec berhasil
    } else {
        // Parent process - wait for child
        int status;
        pid_t waited = nusa_waitpid(pid, &status, 0);
        
        if (waited < 0) {
            nusa_printf("Error: waitpid() failed\n");
            return -1;
        }
        
        nusa_printf("[shell] Process %d exited with status %d\n", pid, status);
    }
    
    return 0;
}

// ============================================================================
// COMMAND DISPATCHER
// ============================================================================

typedef struct {
    const char *name;
    int (*func)(int, char**);
} builtin_cmd_t;

static builtin_cmd_t builtins[] = {
    {"help", cmd_help},
    {"clear", cmd_clear},
    {"echo", cmd_echo},
    {"ls", cmd_ls},
    {"cat", cmd_cat},
    {"ps", cmd_ps},
    {"reboot", cmd_reboot},
    {"shutdown", cmd_shutdown},
    {NULL, NULL}
};

static int shell_execute_command(void) {
    if (cmd_argc == 0) {
        return 0;
    }
    
    const char *cmd = cmd_args[0];
    
    // Cek built-in commands
    for (int i = 0; builtins[i].name != NULL; i++) {
        if (nusa_strcmp(cmd, builtins[i].name) == 0) {
            return builtins[i].func(cmd_argc, cmd_args);
        }
    }
    
    // Cek special command "hello"
    if (nusa_strcmp(cmd, "hello") == 0) {
        return shell_exec_program("/hello", cmd_args);
    }
    
    // Coba execute sebagai program eksternal
    return shell_exec_program(cmd, cmd_args);
}

// ============================================================================
// INPUT HANDLING
// ============================================================================

static int shell_read_command(void) {
    shell_print_prompt();
    
    nusa_memset(cmd_buffer, 0, sizeof(cmd_buffer));
    int pos = 0;
    
    while (1) {
        char c;
        ssize_t ret = nusa_read(STDIN_FILENO, &c, 1);
        
        if (ret <= 0) {
            continue;
        }
        
        // Handle Enter
        if (c == '\n' || c == '\r') {
            nusa_printf("\n");
            cmd_buffer[pos] = '\0';
            return pos;
        }
        
        // Handle Backspace
        if (c == '\b' || c == 127) {
            if (pos > 0) {
                pos--;
                nusa_printf("\b \b");
            }
            continue;
        }
        
        // Handle Ctrl+C
        if (c == 3) {
            nusa_printf("^C\n");
            shell_clear_input();
            return -1;
        }
        
        // Normal character
        if (pos < MAX_CMD_LEN - 1 && c >= 32) {
            cmd_buffer[pos++] = c;
            nusa_printf("%c", c);
        }
    }
}

// ============================================================================
// MAIN SHELL LOOP
// ============================================================================

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    
    nusa_printf("\n");
    nusa_printf("╔════════════════════════════════════════╗\n");
    nusa_printf("║   NusantaraOS64 Shell v1.0             ║\n");
    nusa_printf("║   Ketik 'help' untuk daftar perintah   ║\n");
    nusa_printf("╚════════════════════════════════════════╝\n");
    nusa_printf("\n");
    
    while (1) {
        shell_clear_input();
        
        int len = shell_read_command();
        if (len < 0) {
            continue;  // Ctrl+C atau error
        }
        
        if (len == 0) {
            continue;  // Empty command
        }
        
        if (shell_parse_command(cmd_buffer) > 0) {
            shell_execute_command();
        }
    }
    
    return 0;
}

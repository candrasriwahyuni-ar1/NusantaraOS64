/**
 * NusantaraOS64 - MicroPython Stub
 * 
 * Integration layer untuk MicroPython interpreter
 * Memungkinkan scripting Python di atas NusantaraOS64
 */

#include "../include/nusantara.h"

// ============================================================================
// PYTHON BINDING WRAPPERS
// ============================================================================

/**
 * nusa.print() - Print ke stdout
 */
static int py_nusa_print(int argc, void **argv) {
    if (argc < 1) {
        return 0;
    }
    
    // Simple string print
    const char *str = (const char*)argv[0];
    nusa_printf("%s", str);
    
    return 0;
}

/**
 * nusa.sleep(ms) - Sleep dalam milidetik
 */
static int py_nusa_sleep(int argc, void **argv) {
    if (argc < 1) {
        return 0;
    }
    
    uint64_t ms = *(uint64_t*)argv[0];
    return nusa_sleep(ms);
}

/**
 * nusa.getpid() - Get process ID
 */
static int py_nusa_getpid(int argc, void **argv) {
    (void)argc; (void)argv;
    return nusa_getpid();
}

/**
 * nusa.yield() - Yield CPU
 */
static int py_nusa_yield(int argc, void **argv) {
    (void)argc; (void)argv;
    return nusa_yield();
}

// ============================================================================
// GRAPHICS BINDINGS (via gfx2d library)
// ============================================================================

extern int gfx_init(void *framebuffer, uint32_t width, uint32_t height, uint32_t pitch);
extern void gfx_clear(uint32_t color);
extern void gfx_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
extern void gfx_fill_circle(int cx, int cy, int radius, uint32_t color);
extern void gfx_draw_string(uint32_t x, uint32_t y, const char *str, uint32_t color);

/**
 * nusa.gfx.clear(color)
 */
static int py_gfx_clear(int argc, void **argv) {
    if (argc < 1) {
        return 0;
    }
    
    uint32_t color = *(uint32_t*)argv[0];
    gfx_clear(color);
    
    return 0;
}

/**
 * nusa.gfx.circle(x, y, r, color)
 */
static int py_gfx_circle(int argc, void **argv) {
    if (argc < 4) {
        return 0;
    }
    
    int cx = *(int*)argv[0];
    int cy = *(int*)argv[1];
    int r = *(int*)argv[2];
    uint32_t color = *(uint32_t*)argv[3];
    
    gfx_fill_circle(cx, cy, r, color);
    
    return 0;
}

/**
 * nusa.gfx.rect(x, y, w, h, color)
 */
static int py_gfx_rect(int argc, void **argv) {
    if (argc < 5) {
        return 0;
    }
    
    uint32_t x = *(uint32_t*)argv[0];
    uint32_t y = *(uint32_t*)argv[1];
    uint32_t w = *(uint32_t*)argv[2];
    uint32_t h = *(uint32_t*)argv[3];
    uint32_t color = *(uint32_t*)argv[4];
    
    gfx_draw_rect(x, y, w, h, color);
    
    return 0;
}

// ============================================================================
// MODULE REGISTRY
// ============================================================================

typedef struct {
    const char *name;
    int (*func)(int, void**);
} py_builtin_t;

static py_builtin_t nusa_builtins[] = {
    {"print", py_nusa_print},
    {"sleep", py_nusa_sleep},
    {"getpid", py_nusa_getpid},
    {"yield", py_nusa_yield},
    {"gfx_clear", py_gfx_clear},
    {"gfx_circle", py_gfx_circle},
    {"gfx_rect", py_gfx_rect},
    {NULL, NULL}
};

// ============================================================================
// MICROPYTHON INTEGRATION STUB
// ============================================================================

/**
 * Initialize MicroPython runtime
 * Returns: 0 on success, -1 on error
 */
int mp_nusantara_init(void) {
    nusa_printf("[mp] MicroPython Nusantara integration initialized\n");
    nusa_printf("[mp] Available modules: nusa, gfx\n");
    
    return 0;
}

/**
 * Run Python script from string
 */
int mp_nusantara_run(const char *script) {
    nusa_printf("[mp] Running script:\n%s\n", script);
    
    // TODO: Integrate with actual MicroPython interpreter
    // For now, just parse simple commands
    
    nusa_printf("[mp] Script execution complete (stub)\n");
    
    return 0;
}

/**
 * Run Python REPL loop
 */
void mp_nusantara_repl(void) {
    nusa_printf("\n");
    nusa_printf("╔═══════════════════════════════════════╗\n");
    nusa_printf("║   MicroPython REPL on NusantaraOS64   ║\n");
    nusa_printf("║   Type 'exit()' to quit               ║\n");
    nusa_printf("╚═══════════════════════════════════════╝\n");
    nusa_printf("\n");
    
    char buffer[256];
    
    while (1) {
        nusa_printf(">>> ");
        
        // Read line
        int pos = 0;
        while (1) {
            char c;
            ssize_t ret = nusa_read(STDIN_FILENO, &c, 1);
            
            if (ret <= 0) continue;
            
            if (c == '\n' || c == '\r') {
                nusa_printf("\n");
                break;
            }
            
            if (c == '\b' || c == 127) {
                if (pos > 0) {
                    pos--;
                    nusa_printf("\b \b");
                }
                continue;
            }
            
            if (pos < sizeof(buffer) - 1 && c >= 32) {
                buffer[pos++] = c;
                nusa_printf("%c", c);
            }
        }
        
        buffer[pos] = '\0';
        
        // Check for exit
        if (nusa_strcmp(buffer, "exit()") == 0 || nusa_strcmp(buffer, "quit()") == 0) {
            nusa_printf("[mp] Exiting REPL\n");
            return;
        }
        
        // Execute command
        if (pos > 0) {
            mp_nusantara_run(buffer);
        }
    }
}

// ============================================================================
// DEMO SCRIPTS
// ============================================================================

static const char *demo_fractal = 
    "# Fractal Circle Demo\n"
    "def draw_fractal(x, y, r, depth):\n"
    "    if depth == 0:\n"
    "        return\n"
    "    nusa.gfx.circle(x, y, r, 0x00FF00)\n"
    "    draw_fractal(x + r, y, r//2, depth-1)\n"
    "    draw_fractal(x - r, y, r//2, depth-1)\n"
    "    draw_fractal(x, y + r, r//2, depth-1)\n"
    "    draw_fractal(x, y - r, r//2, depth-1)\n"
    "\n"
    "nusa.gfx.clear(0x000000)\n"
    "draw_fractal(320, 240, 100, 5)\n";

static const char *demo_bouncing =
    "# Bouncing Ball Animation\n"
    "x, y = 100, 100\n"
    "dx, dy = 2, 2\n"
    "for i in range(100):\n"
    "    nusa.gfx.clear(0x000033)\n"
    "    nusa.gfx.circle(x, y, 20, 0xFF0000)\n"
    "    x += dx\n"
    "    y += dy\n"
    "    if x < 20 or x > 620:\n"
    "        dx = -dx\n"
    "    if y < 20 or y > 460:\n"
    "        dy = -dy\n"
    "    nusa.yield()\n";

/**
 * Run demo scripts
 */
void mp_nusantara_demo(void) {
    nusa_printf("[mp] Running MicroPython demos...\n");
    
    nusa_printf("\nDemo 1: Fractal Circles\n");
    mp_nusantara_run(demo_fractal);
    
    nusa_printf("\nDemo 2: Bouncing Ball (conceptual)\n");
    mp_nusantara_run(demo_bouncing);
    
    nusa_printf("\n[mp] Demos complete!\n");
}

// ============================================================================
// MAIN ENTRY POINT FOR PYTHON APP
// ============================================================================

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    
    nusa_printf("\n");
    nusa_printf("╔═══════════════════════════════════════╗\n");
    nusa_printf("║   NusantaraOS64 Python Runtime v0.1   ║\n");
    nusa_printf("╚═══════════════════════════════════════╝\n");
    nusa_printf("\n");
    
    // Initialize
    mp_nusantara_init();
    
    // Show menu
    nusa_printf("Select mode:\n");
    nusa_printf("  1 - Run REPL\n");
    nusa_printf("  2 - Run demos\n");
    nusa_printf("  3 - Exit\n");
    nusa_printf("\n");
    
    char choice;
    nusa_read(STDIN_FILENO, &choice, 1);
    
    switch (choice) {
        case '1':
            mp_nusantara_repl();
            break;
        case '2':
            mp_nusantara_demo();
            break;
        default:
            nusa_printf("Exiting...\n");
            break;
    }
    
    return 0;
}

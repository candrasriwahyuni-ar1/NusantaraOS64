/*
 * NusantaraOS64 - Console/Serial Output
 * 
 * Basic console output via VGA text mode and serial port
 */

#include "../include/nusantara.h"

#define VGA_BUFFER      0xB8000
#define VGA_COLS        80
#define VGA_ROWS        25

static u16 *vga_buffer = (u16 *)VGA_BUFFER;
static int cursor_x = 0;
static int cursor_y = 0;
static u8 color = 0x0F;  /* White on black */

/*
 * Initialize console
 */
void console_init(void) {
    cursor_x = 0;
    cursor_y = 0;
    console_clear();
}

/*
 * Clear console screen
 */
void console_clear(void) {
    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++) {
        vga_buffer[i] = (color << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
}

/*
 * Scroll screen if needed
 */
static void scroll(void) {
    if (cursor_y >= VGA_ROWS) {
        /* Move all lines up */
        for (int i = 0; i < (VGA_ROWS - 1) * VGA_COLS; i++) {
            vga_buffer[i] = vga_buffer[i + VGA_COLS];
        }
        
        /* Clear last line */
        for (int i = (VGA_ROWS - 1) * VGA_COLS; i < VGA_ROWS * VGA_COLS; i++) {
            vga_buffer[i] = (color << 8) | ' ';
        }
        
        cursor_y = VGA_ROWS - 1;
    }
}

/*
 * Put a single character to console
 */
void console_putchar(char c) {
    switch (c) {
        case '\n':
            cursor_x = 0;
            cursor_y++;
            break;
        case '\r':
            cursor_x = 0;
            break;
        case '\t':
            cursor_x = (cursor_x + 8) & ~7;
            break;
        case '\b':
            if (cursor_x > 0) {
                cursor_x--;
                vga_buffer[cursor_y * VGA_COLS + cursor_x] = (color << 8) | ' ';
            }
            break;
        default:
            vga_buffer[cursor_y * VGA_COLS + cursor_x] = (color << 8) | c;
            cursor_x++;
            break;
    }
    
    /* Wrap around */
    if (cursor_x >= VGA_COLS) {
        cursor_x = 0;
        cursor_y++;
    }
    
    scroll();
    
    /* Update hardware cursor */
    u16 pos = cursor_y * VGA_COLS + cursor_x;
    __asm__ volatile("outb %0, $0x3D4" :: "a"((u8)0x0F));
    __asm__ volatile("outb %0, $0x3D5" :: "a"((u8)(pos & 0xFF)));
    __asm__ volatile("outb %0, $0x3D4" :: "a"((u8)0x0E));
    __asm__ volatile("outb %0, $0x3D5" :: "a"((u8)((pos >> 8) & 0xFF)));
}

/*
 * Print a string
 */
void console_print(const char *str) {
    while (*str) {
        console_putchar(*str++);
    }
}

/*
 * Convert integer to string
 */
static void print_number(long value, int base) {
    char buffer[32];
    int i = 0;
    int negative = 0;
    
    if (value < 0 && base == 10) {
        negative = 1;
        value = -value;
    }
    
    do {
        int digit = value % base;
        buffer[i++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        value /= base;
    } while (value > 0);
    
    if (negative) {
        console_putchar('-');
    }
    
    while (i > 0) {
        console_putchar(buffer[--i]);
    }
}

/*
 * printf implementation (minimal)
 */
void console_printf(const char *fmt, ...) {
    va_list args;
    __builtin_va_start(args, fmt);
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd':
                case 'i':
                    print_number(__builtin_va_arg(args, int), 10);
                    break;
                case 'u':
                    print_number(__builtin_va_arg(args, unsigned int), 10);
                    break;
                case 'x':
                    print_number(__builtin_va_arg(args, unsigned int), 16);
                    break;
                case 'X':
                    print_number(__builtin_va_arg(args, unsigned long), 16);
                    break;
                case 'p':
                    console_print("0x");
                    print_number(__builtin_va_arg(args, unsigned long), 16);
                    break;
                case 'c':
                    console_putchar(__builtin_va_arg(args, int));
                    break;
                case 's':
                    console_print(__builtin_va_arg(args, const char *));
                    break;
                case '%':
                    console_putchar('%');
                    break;
            }
        } else {
            console_putchar(*fmt);
        }
        fmt++;
    }
    
    __builtin_va_end(args);
}

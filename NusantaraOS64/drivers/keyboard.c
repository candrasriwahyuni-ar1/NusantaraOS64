/*
 * NusantaraOS64 - Keyboard Driver
 * 
 * Fase 5: Driver Input
 * Handling keyboard interrupts dan scan codes
 */

#include "../include/nusantara.h"

#define KEYBOARD_DATA_PORT  0x60
#define KEYBOARD_STATUS_PORT 0x64

/* US QWERTY Scan Code Set 1 */
static const char scancode_to_ascii[128] = {
    0,   /* 0x00 - Unused */
    0,   /* 0x01 - ESC */
    '1', /* 0x02 */
    '2', /* 0x03 */
    '3', /* 0x04 */
    '4', /* 0x05 */
    '5', /* 0x06 */
    '6', /* 0x07 */
    '7', /* 0x08 */
    '8', /* 0x09 */
    '9', /* 0x0A */
    '0', /* 0x0B */
    '-', /* 0x0C */
    '=', /* 0x0D */
    '\b',/* 0x0E - Backspace */
    '\t',/* 0x0F - Tab */
    'q', /* 0x10 */
    'w', /* 0x11 */
    'e', /* 0x12 */
    'r', /* 0x13 */
    't', /* 0x14 */
    'y', /* 0x15 */
    'u', /* 0x16 */
    'i', /* 0x17 */
    'o', /* 0x18 */
    'p', /* 0x19 */
    '[', /* 0x1A */
    ']', /* 0x1B */
    '\n',/* 0x1C - Enter */
    0,   /* 0x1D - L Ctrl (modifier) */
    'a', /* 0x1E */
    's', /* 0x1F */
    'd', /* 0x20 */
    'f', /* 0x21 */
    'g', /* 0x22 */
    'h', /* 0x23 */
    'j', /* 0x24 */
    'k', /* 0x25 */
    'l', /* 0x26 */
    ';', /* 0x27 */
    '\'',/* 0x28 */
    '`', /* 0x29 */
    0,   /* 0x2A - L Shift (modifier) */
    '\\',/* 0x2B */
    'z', /* 0x2C */
    'x', /* 0x2D */
    'c', /* 0x2E */
    'v', /* 0x2F */
    'b', /* 0x30 */
    'n', /* 0x31 */
    'm', /* 0x32 */
    ',', /* 0x33 */
    '.', /* 0x34 */
    '/', /* 0x35 */
    0,   /* 0x36 - R Shift (modifier) */
    '*', /* 0x37 - Keypad * */
    0,   /* 0x38 - L Alt (modifier) */
    ' ', /* 0x39 - Space */
    0,   /* 0x3A - Caps Lock */
    /* Function keys and extended keys not implemented */
};

/* Modifier key states */
static bool lshift_pressed = false;
static bool rshift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static bool caps_lock = false;

/* Keyboard buffer */
#define KEYBOARD_BUFFER_SIZE 256
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static u32 buffer_head = 0;
static u32 buffer_tail = 0;
static bool keyboard_initialized = false;

/*
 * Read a byte from keyboard controller
 */
static u8 keyboard_read_byte(void) {
    return __builtin_ia32_inbyte(KEYBOARD_DATA_PORT);
}

/*
 * Wait for keyboard controller to be ready
 */
static void keyboard_wait(void) {
    while (__builtin_ia32_inbyte(KEYBOARD_STATUS_PORT) & 0x02);
}

/*
 * Handle keyboard interrupt
 */
void keyboard_handler(void) {
    u8 scancode = keyboard_read_byte();
    
    /* Check for key release (bit 7 set) */
    bool released = (scancode & 0x80) != 0;
    scancode &= 0x7F;
    
    if (scancode >= 128) {
        return;
    }
    
    /* Handle modifier keys */
    switch (scancode) {
        case 0x1D:  /* Ctrl */
            ctrl_pressed = !released;
            return;
        case 0x2A:  /* L Shift */
            lshift_pressed = !released;
            return;
        case 0x36:  /* R Shift */
            rshift_pressed = !released;
            return;
        case 0x38:  /* Alt */
            alt_pressed = !released;
            return;
        case 0x3A:  /* Caps Lock */
            if (!released) {
                caps_lock = !caps_lock;
            }
            return;
    }
    
    /* Only process on key press */
    if (released) {
        return;
    }
    
    /* Get ASCII character */
    char c = scancode_to_ascii[scancode];
    
    if (c == 0) {
        return;  /* No mapping for this key */
    }
    
    /* Apply modifiers */
    if (lshift_pressed || rshift_pressed) {
        if (c >= 'a' && c <= 'z') {
            c -= 32;  /* Convert to uppercase */
        } else if (c == '[') c = '{';
        else if (c == ']') c = '}';
        else if (c == ';') c = ':';
        else if (c == '\'') c = '"';
        else if (c == ',') c = '<';
        else if (c == '.') c = '>';
        else if (c == '/') c = '?';
        else if (c == '`') c = '~';
        else if (c == '-') c = '_';
        else if (c == '=') c = '+';
        else if (c == '\\') c = '|';
    } else if (caps_lock) {
        if (c >= 'a' && c <= 'z') {
            c -= 32;
        }
    }
    
    /* Add to buffer */
    u32 next_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next_head != buffer_tail) {
        keyboard_buffer[buffer_head] = c;
        buffer_head = next_head;
    }
}

/*
 * Initialize keyboard driver
 */
void keyboard_init(void) {
    console_print("[KBD] Initializing keyboard...\n");
    
    /* Clear buffer */
    buffer_head = 0;
    buffer_tail = 0;
    
    /* Enable keyboard interrupts in PIC */
    __asm__ volatile("inb $0x21, %al");
    __asm__ volatile("andb $0xFD, %al");  /* Clear bit 1 (IRQ1) */
    __asm__ volatile("outb %al, $0x21");
    
    keyboard_initialized = true;
    console_print("[KBD] Keyboard initialized\n");
}

/*
 * Check if a key is available in buffer
 */
bool keyboard_available(void) {
    return buffer_head != buffer_tail;
}

/*
 * Get a character from keyboard (blocking)
 */
char keyboard_getchar(void) {
    while (!keyboard_available()) {
        /* Wait for input */
        halt_cpu();
    }
    
    char c = keyboard_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

/*
 * Get a character from keyboard (non-blocking)
 * Returns 0 if no character available
 */
char keyboard_trygetchar(void) {
    if (keyboard_available()) {
        char c = keyboard_buffer[buffer_tail];
        buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
        return c;
    }
    return 0;
}

/*
 * Read a line from keyboard
 */
void keyboard_readline(char *buffer, int max_len) {
    int pos = 0;
    
    while (pos < max_len - 1) {
        char c = keyboard_getchar();
        
        switch (c) {
            case '\n':
            case '\r':
                console_putchar('\n');
                buffer[pos] = '\0';
                return;
                
            case '\b':
                if (pos > 0) {
                    pos--;
                    console_putchar('\b');
                    console_putchar(' ');
                    console_putchar('\b');
                }
                break;
                
            default:
                console_putchar(c);
                buffer[pos++] = c;
                break;
        }
    }
    
    buffer[pos] = '\0';
}

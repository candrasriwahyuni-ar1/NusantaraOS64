/*
 * NusantaraOS64 - Framebuffer Driver
 * 
 * Fase 5: Driver Grafis untuk GUI
 * Manipulasi framebuffer untuk rendering grafis
 */

#include "../include/nusantara.h"

/* Framebuffer info (will be populated by bootloader) */
static u32 *framebuffer = NULL;
static u32 fb_width = FRAMEBUFFER_WIDTH;
static u32 fb_height = FRAMEBUFFER_HEIGHT;
static u32 fb_pitch = FRAMEBUFFER_WIDTH;
static bool fb_initialized = false;

/* Font data (8x16 bitmap font - simplified) */
__attribute__((unused)) static const u8 font_data[256][16] = {
    /* Basic ASCII font would go here - using placeholder */
};

/*
 * Initialize framebuffer
 */
void framebuffer_init(void) {
    /* In real implementation, get framebuffer info from UEFI GOP */
    /* For now, use a simulated framebuffer address */
    framebuffer = (u32 *)(0xE0000000);  /* Common framebuffer address */
    
    if (!framebuffer) {
        console_print("[FB] Warning: Framebuffer not available\n");
        return;
    }
    
    fb_initialized = true;
    console_printf("[FB] Framebuffer initialized: %dx%d\n", fb_width, fb_height);
}

/*
 * Clear framebuffer with a color
 * Color format: 0xAARRGGBB
 */
void framebuffer_clear(u32 color) {
    if (!fb_initialized) return;
    
    for (u32 i = 0; i < fb_width * fb_height; i++) {
        framebuffer[i] = color;
    }
}

/*
 * Put a single pixel
 */
void framebuffer_put_pixel(u32 x, u32 y, u32 color) {
    if (!fb_initialized) return;
    if (x >= fb_width || y >= fb_height) return;
    
    framebuffer[y * fb_pitch + x] = color;
}

/*
 * Draw a rectangle
 */
void framebuffer_draw_rect(u32 x, u32 y, u32 w, u32 h, u32 color) {
    if (!fb_initialized) return;
    
    for (u32 row = 0; row < h; row++) {
        for (u32 col = 0; col < w; col++) {
            if (x + col < fb_width && y + row < fb_height) {
                framebuffer[(y + row) * fb_pitch + (x + col)] = color;
            }
        }
    }
}

/*
 * Draw a filled rectangle
 */
void framebuffer_fill_rect(u32 x, u32 y, u32 w, u32 h, u32 color) {
    framebuffer_draw_rect(x, y, w, h, color);
}

/*
 * Draw a horizontal line
 */
void framebuffer_draw_hline(u32 x1, u32 x2, u32 y, u32 color) {
    if (x1 > x2) {
        u32 tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    framebuffer_draw_rect(x1, y, x2 - x1 + 1, 1, color);
}

/*
 * Draw a vertical line
 */
void framebuffer_draw_vline(u32 x, u32 y1, u32 y2, u32 color) {
    if (y1 > y2) {
        u32 tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    framebuffer_draw_rect(x, y1, 1, y2 - y1 + 1, color);
}

/*
 * Draw a circle (midpoint algorithm)
 */
void framebuffer_draw_circle(u32 cx, u32 cy, u32 radius, u32 color) {
    if (!fb_initialized) return;
    
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        framebuffer_put_pixel(cx + x, cy + y, color);
        framebuffer_put_pixel(cx + y, cy + x, color);
        framebuffer_put_pixel(cx - y, cy + x, color);
        framebuffer_put_pixel(cx - x, cy + y, color);
        framebuffer_put_pixel(cx - x, cy - y, color);
        framebuffer_put_pixel(cx - y, cy - x, color);
        framebuffer_put_pixel(cx + y, cy - x, color);
        framebuffer_put_pixel(cx + x, cy - y, color);
        
        y++;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) {
            x--;
            err += 1 - 2 * x;
        }
    }
}

/*
 * Draw a string on framebuffer (simplified)
 * In production, this would use a proper font bitmap
 */
void framebuffer_draw_string(const char *str, u32 x, u32 y, u32 color) {
    (void)x; (void)y; (void)color; /* Stub implementation - not yet using parameters */
    if (!fb_initialized || !str) return;
    
    /* For now, just print to console as fallback */
    console_print(str);
    
    /* TODO: Implement proper font rendering */
    /* This would iterate through each character and draw the bitmap */
}

/*
 * Draw a window/frame for GUI
 */
void framebuffer_draw_window(u32 x, u32 y, u32 w, u32 h, const char *title) {
    if (!fb_initialized) return;
    
    /* Window border */
    framebuffer_draw_rect(x, y, w, h, 0xFF404040);  /* Gray border */
    
    /* Window background */
    framebuffer_draw_rect(x + 1, y + 1, w - 2, h - 2, 0xFFC0C0C0);  /* Light gray */
    
    /* Title bar */
    framebuffer_draw_rect(x + 1, y + 1, w - 2, 20, 0xFF000080);  /* Navy blue */
    
    /* Title text */
    if (title) {
        framebuffer_draw_string(title, x + 5, y + 3, 0xFFFFFFFF);
    }
}

/*
 * Get framebuffer info
 */
u32 *framebuffer_get_ptr(void) {
    return framebuffer;
}

u32 framebuffer_get_width(void) {
    return fb_width;
}

u32 framebuffer_get_height(void) {
    return fb_height;
}

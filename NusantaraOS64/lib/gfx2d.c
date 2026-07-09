/**
 * NusantaraOS64 - Framebuffer Graphics Library (User Space)
 * 
 * 2D graphics primitives untuk aplikasi user
 * Drawing lines, circles, rectangles, text rendering
 */

#include "../include/nusantara.h"

// ============================================================================
// GRAPHICS CONTEXT
// ============================================================================

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t pixel_format;  // 0=RGB, 1=BGR
    void *framebuffer;
} gfx_context_t;

static gfx_context_t gfx_ctx;

// ============================================================================
// COLOR UTILS
// ============================================================================

#define RGB(r, g, b) (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define RGBA(r, g, b, a) (((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

#define COLOR_BLACK     RGB(0, 0, 0)
#define COLOR_WHITE     RGB(255, 255, 255)
#define COLOR_RED       RGB(255, 0, 0)
#define COLOR_GREEN     RGB(0, 255, 0)
#define COLOR_BLUE      RGB(0, 0, 255)
#define COLOR_YELLOW    RGB(255, 255, 0)
#define COLOR_CYAN      RGB(0, 255, 255)
#define COLOR_MAGENTA   RGB(255, 0, 255)
#define COLOR_GRAY      RGB(128, 128, 128)
#define COLOR_ORANGE    RGB(255, 165, 0)

// ============================================================================
// LOW-LEVEL PIXEL ACCESS
// ============================================================================

static inline void gfx_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= gfx_ctx.width || y >= gfx_ctx.height) {
        return;
    }
    
    uint32_t *fb = (uint32_t*)gfx_ctx.framebuffer;
    uint32_t offset = y * (gfx_ctx.pitch / 4) + x;
    fb[offset] = color;
}

static inline uint32_t gfx_get_pixel(uint32_t x, uint32_t y) {
    if (x >= gfx_ctx.width || y >= gfx_ctx.height) {
        return COLOR_BLACK;
    }
    
    uint32_t *fb = (uint32_t*)gfx_ctx.framebuffer;
    uint32_t offset = y * (gfx_ctx.pitch / 4) + x;
    return fb[offset];
}

// ============================================================================
// PRIMITIVE DRAWING
// ============================================================================

void gfx_clear(uint32_t color) {
    uint32_t *fb = (uint32_t*)gfx_ctx.framebuffer;
    size_t num_pixels = (gfx_ctx.pitch / 4) * gfx_ctx.height;
    
    for (size_t i = 0; i < num_pixels; i++) {
        fb[i] = color;
    }
}

void gfx_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    // Draw top and bottom edges
    for (uint32_t i = 0; i < w; i++) {
        gfx_put_pixel(x + i, y, color);
        gfx_put_pixel(x + i, y + h - 1, color);
    }
    
    // Draw left and right edges
    for (uint32_t j = 0; j < h; j++) {
        gfx_put_pixel(x, y + j, color);
        gfx_put_pixel(x + w - 1, y + j, color);
    }
}

void gfx_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t j = y; j < y + h; j++) {
        for (uint32_t i = x; i < x + w; i++) {
            gfx_put_pixel(i, j, color);
        }
    }
}

// Bresenham's line algorithm
void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        gfx_put_pixel((uint32_t)x0, (uint32_t)y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Midpoint circle algorithm
void gfx_draw_circle(int cx, int cy, int radius, uint32_t color) {
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        gfx_put_pixel((uint32_t)(cx + x), (uint32_t)(cy + y), color);
        gfx_put_pixel((uint32_t)(cx + y), (uint32_t)(cy + x), color);
        gfx_put_pixel((uint32_t)(cx - y), (uint32_t)(cy + x), color);
        gfx_put_pixel((uint32_t)(cx - x), (uint32_t)(cy + y), color);
        gfx_put_pixel((uint32_t)(cx - x), (uint32_t)(cy - y), color);
        gfx_put_pixel((uint32_t)(cx - y), (uint32_t)(cy - x), color);
        gfx_put_pixel((uint32_t)(cx + y), (uint32_t)(cy - x), color);
        gfx_put_pixel((uint32_t)(cx + x), (uint32_t)(cy - y), color);
        
        y++;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) {
            x--;
            err += 1 - 2 * x;
        }
    }
}

void gfx_fill_circle(int cx, int cy, int radius, uint32_t color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                gfx_put_pixel((uint32_t)(cx + x), (uint32_t)(cy + y), color);
            }
        }
    }
}

// ============================================================================
// TEXT RENDERING (Simple Bitmap Font)
// ============================================================================

// 8x8 bitmap font (subset ASCII 32-127)
static const uint8_t font_8x8[96][8] = {
    // Space (index 0)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ! (index 1)
    {0x00, 0x00, 0x5F, 0x5F, 0x00, 0x00, 0x00, 0x00},
    // " (index 2)
    {0x00, 0x07, 0x07, 0x00, 0x00, 0x07, 0x07, 0x00},
    // # (index 3)
    {0x14, 0x14, 0x3F, 0x3F, 0x14, 0x14, 0x3F, 0x3F},
    // $ (index 4)
    {0x00, 0x0C, 0x1E, 0x33, 0x1E, 0x0C, 0x00, 0x00},
    // % (index 5)
    {0x00, 0x33, 0x37, 0x04, 0x08, 0x1E, 0x33, 0x00},
    // & (index 6)
    {0x00, 0x1C, 0x36, 0x1C, 0x2D, 0x36, 0x2C, 0x00},
    // ' (index 7)
    {0x00, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ( (index 8)
    {0x00, 0x1C, 0x3E, 0x63, 0x41, 0x00, 0x00, 0x00},
    // ) (index 9)
    {0x00, 0x41, 0x63, 0x3E, 0x1C, 0x00, 0x00, 0x00},
    // * (index 10)
    {0x08, 0x1C, 0x3E, 0x1C, 0x08, 0x00, 0x00, 0x00},
    // + (index 11)
    {0x08, 0x08, 0x3E, 0x3E, 0x08, 0x08, 0x00, 0x00},
    // , (index 12)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00},
    // - (index 13)
    {0x00, 0x00, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x00},
    // . (index 14)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00},
    // / (index 15)
    {0x00, 0x01, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x00},
    // 0 (index 16)
    {0x00, 0x1C, 0x3E, 0x63, 0x63, 0x3E, 0x1C, 0x00},
    // 1 (index 17)
    {0x00, 0x18, 0x1C, 0x18, 0x18, 0x18, 0x3C, 0x00},
    // 2 (index 18)
    {0x00, 0x1E, 0x3F, 0x03, 0x0E, 0x1C, 0x3F, 0x00},
    // 3 (index 19)
    {0x00, 0x1E, 0x3F, 0x03, 0x0E, 0x3F, 0x1E, 0x00},
    // 4 (index 20)
    {0x00, 0x0E, 0x1E, 0x3E, 0x63, 0x03, 0x03, 0x00},
    // 5 (index 21)
    {0x00, 0x3F, 0x3F, 0x03, 0x0F, 0x3F, 0x1E, 0x00},
    // 6 (index 22)
    {0x00, 0x1C, 0x3E, 0x63, 0x63, 0x3F, 0x1E, 0x00},
    // 7 (index 23)
    {0x00, 0x3F, 0x3F, 0x01, 0x03, 0x06, 0x0C, 0x00},
    // 8 (index 24)
    {0x00, 0x1E, 0x3F, 0x63, 0x63, 0x3F, 0x1E, 0x00},
    // 9 (index 25)
    {0x00, 0x1E, 0x3F, 0x63, 0x63, 0x3F, 0x1E, 0x00},
    // Default for rest (simplified)
};

void gfx_draw_char(uint32_t x, uint32_t y, char c, uint32_t color) {
    if (c < 32 || c > 127) {
        c = '?';
    }
    
    int idx = c - 32;
    const uint8_t *char_bitmap = font_8x8[idx];
    
    for (int row = 0; row < 8; row++) {
        uint8_t bits = char_bitmap[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                gfx_put_pixel(x + col, y + row, color);
            }
        }
    }
}

void gfx_draw_string(uint32_t x, uint32_t y, const char *str, uint32_t color) {
    while (*str) {
        if (*str == '\n') {
            x = 0;
            y += 8;
        } else {
            gfx_draw_char(x, y, *str, color);
            x += 8;
        }
        str++;
    }
}

// ============================================================================
// WINDOW DRAWING
// ============================================================================

void gfx_draw_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h, 
                     const char *title, uint32_t bg_color, uint32_t fg_color) {
    // Background
    gfx_fill_rect(x, y, w, h, bg_color);
    
    // Border
    gfx_draw_rect(x, y, w, h, fg_color);
    
    // Title bar
    gfx_fill_rect(x, y, w, 16, fg_color);
    
    // Title text
    gfx_draw_string(x + 4, y + 4, title, bg_color);
}

// ============================================================================
// INITIALIZATION
// ============================================================================

int gfx_init(void *framebuffer, uint32_t width, uint32_t height, uint32_t pitch) {
    gfx_ctx.framebuffer = framebuffer;
    gfx_ctx.width = width;
    gfx_ctx.height = height;
    gfx_ctx.pitch = pitch;
    gfx_ctx.pixel_format = 0;  // RGB
    
    nusa_printf("[gfx] Initialized: %dx%d, pitch=%d\n", width, height, pitch);
    
    return 0;
}

// ============================================================================
// DEMO FUNCTION
// ============================================================================

void gfx_demo(void) {
    nusa_printf("[gfx] Running graphics demo...\n");
    
    // Clear to blue gradient
    for (uint32_t y = 0; y < gfx_ctx.height; y++) {
        uint32_t color = RGB(0, 0, (y * 255) / gfx_ctx.height);
        for (uint32_t x = 0; x < gfx_ctx.width; x++) {
            gfx_put_pixel(x, y, color);
        }
    }
    
    // Draw some shapes
    gfx_fill_circle(gfx_ctx.width / 4, gfx_ctx.height / 2, 50, COLOR_RED);
    gfx_fill_circle(gfx_ctx.width * 3 / 4, gfx_ctx.height / 2, 50, COLOR_GREEN);
    
    // Draw connecting lines
    for (int i = 0; i < 20; i++) {
        int y1 = (gfx_ctx.height / 2) - 50 + (i * 5);
        int y2 = (gfx_ctx.height / 2) - 50 + (i * 5);
        gfx_draw_line(gfx_ctx.width / 4 + 50, y1, gfx_ctx.width * 3 / 4 - 50, y2, COLOR_YELLOW);
    }
    
    // Draw windows
    gfx_draw_window(100, 100, 300, 200, "Window 1", COLOR_WHITE, COLOR_BLUE);
    gfx_draw_window(200, 150, 300, 200, "Window 2", COLOR_GRAY, COLOR_RED);
    
    // Draw text
    gfx_draw_string(150, 50, "NusantaraOS64 Graphics Demo!", COLOR_WHITE);
    gfx_draw_string(150, 60, "Press any key to exit", COLOR_YELLOW);
    
    nusa_printf("[gfx] Demo complete!\n");
}

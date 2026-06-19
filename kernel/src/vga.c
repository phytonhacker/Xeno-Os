#include "../include/vga.h"
#include "../include/io.h"

static uint16_t* const vga_mem = (uint16_t*)0xB8000;

int cx = 0, cy = 0;
uint8_t cur_color = C_DEFAULT;

/* ============================================================
 * Hardware kurzor (CRT controller, portok 0x3D4/0x3D5)
 * ============================================================ */

void cur_update(void) {
    uint16_t p = (uint16_t)(cy * VGA_WIDTH + cx);
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(p & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)(p >> 8));
}

void cur_enable(void) {
    outb(0x3D4, 0x0A); outb(0x3D5, (inb(0x3D5) & 0xC0) | 14);
    outb(0x3D4, 0x0B); outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

/* Direkt kurzor pozícionálás (editor használja, eltér a cx/cy-tól) */
void cur_set(int row, int col) {
    uint16_t p = (uint16_t)(row * VGA_WIDTH + col);
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(p & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)(p >> 8));
}

/* ============================================================
 * Alapvető VGA írás
 * ============================================================ */

void vga_putc_at(int x, int y, char c, uint8_t color) {
    vga_mem[y * VGA_WIDTH + x] = (uint16_t)((color << 8) | (uint8_t)c);
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga_mem[i] = (uint16_t)((C_DEFAULT << 8) | ' ');
    cx = 0; cy = 0;
    cur_update();
}

void vga_scroll(void) {
    for (int y = 0; y < VGA_HEIGHT - 1; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            vga_mem[y * VGA_WIDTH + x] = vga_mem[(y + 1) * VGA_WIDTH + x];
    for (int x = 0; x < VGA_WIDTH; x++)
        vga_putc_at(x, VGA_HEIGHT - 1, ' ', C_DEFAULT);
}

void vga_putchar(char c) {
    if (c == '\n' || c == '\r') {
        cx = 0; cy++;
    } else if (c == '\b') {
        if (cx > 0) { cx--; vga_putc_at(cx, cy, ' ', cur_color); }
    } else {
        vga_putc_at(cx, cy, c, cur_color);
        if (++cx >= VGA_WIDTH) { cx = 0; cy++; }
    }
    if (cy >= VGA_HEIGHT) {
        vga_scroll();
        cy = VGA_HEIGHT - 1;
    }
    cur_update();
}

/* ============================================================
 * Kiírási segédfüggvények
 * ============================================================ */

void kprint(const char* s) {
    while (*s) vga_putchar(*s++);
}

void kprint_color(const char* s, uint8_t color) {
    uint8_t old = cur_color;
    cur_color = color;
    while (*s) vga_putchar(*s++);
    cur_color = old;
}

void kprint_int(int n) {
    if (n < 0) { vga_putchar('-'); n = -n; }
    if (n == 0) { vga_putchar('0'); return; }
    char buf[12]; int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i > 0) vga_putchar(buf[--i]);
}

void kprint_hex(uint32_t n) {
    const char* hex = "0123456789ABCDEF";
    vga_putchar('0'); vga_putchar('x');
    for (int i = 28; i >= 0; i -= 4)
        vga_putchar(hex[(n >> i) & 0xF]);
}
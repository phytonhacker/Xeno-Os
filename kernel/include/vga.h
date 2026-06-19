#ifndef VGA_H
#define VGA_H

#include "types.h"

#define VGA_WIDTH   80
#define VGA_HEIGHT  25

/* Színek (BG<<4 | FG formátum, de itt csak FG + fekete BG-vel használjuk) */
#define C_DEFAULT   0x07
#define C_PROMPT    0x0A
#define C_ERROR     0x0C
#define C_INFO      0x0B
#define C_TITLE     0x70   /* fekete betű, fehér háttér – statusbar */
#define C_STATUS    0x70

/* Kurzor és írás állapota - editor.c-nek is kell a kurzor pozícionáláshoz */
extern int cx, cy;
extern uint8_t cur_color;

void vga_clear(void);
void vga_scroll(void);
void vga_putchar(char c);
void vga_putc_at(int x, int y, char c, uint8_t color);

void cur_update(void);
void cur_enable(void);
void cur_set(int row, int col);

/* Kiírási segédfüggvények */
void kprint(const char* s);
void kprint_color(const char* s, uint8_t color);
void kprint_int(int n);
void kprint_hex(uint32_t n);

#endif

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

#define SCANCODE_MAX 58

#define SC_LSHIFT 0x2A
#define SC_RSHIFT 0x36
#define SC_CTRL   0x1D
#define SC_UP     0x48
#define SC_DOWN   0x50
#define SC_LEFT   0x4B
#define SC_RIGHT  0x4D
#define SC_DEL    0x53
#define SC_HOME   0x47
#define SC_END    0x4F
#define SC_PGUP   0x49
#define SC_PGDN   0x51
#define SC_F1     0x3B
#define SC_ESC    0x01

typedef enum { LAYOUT_EN, LAYOUT_HU } kb_layout_t;

/* Aktuális billentyűzetkiosztás - a shell "set keyboard" parancsa állítja */
extern kb_layout_t kb_layout;

/* IRQ1 handler C oldali része - kernel_entry.asm hívja */
void keyboard_handler_c(void);

#endif
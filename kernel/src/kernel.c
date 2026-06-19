#include "../include/types.h"
#include "../include/io.h"
#include "../include/vga.h"
#include "../include/keyboard.h"
#include "../include/shell.h"
#include "../include/app.h"
#include "../include/fs.h"

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[256];
static idt_ptr_t   idt_ptr;

app_mode_t mode = MODE_SHELL;

static void idt_set_gate(uint8_t n, uint32_t h, uint16_t s, uint8_t f) {
    idt[n].offset_low = h & 0xFFFF;
    idt[n].selector = s;
    idt[n].zero = 0;
    idt[n].type_attr = f;
    idt[n].offset_high = (h >> 16) & 0xFFFF;
}

static void pic_init(void) {
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0xFD); outb(0xA1, 0xFF);
}

extern void keyboard_isr(void);

static void idt_init(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)&idt;
    idt_set_gate(0x21, (uint32_t)keyboard_isr, 0x08, 0x8E);
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
}

void kernel_main(void) {
    vga_clear();
    cur_enable();
    pic_init();
    idt_init();
    fs_init();

    kprint_color("X-DOS v0.1\n", C_INFO);
    kprint_color("==========\n", C_INFO);
    kprint("Tipd: "); kprint_color("help\n\n", C_PROMPT);
    shell_prompt();

    __asm__ volatile("sti");
    for (;;) __asm__ volatile("hlt");
}
#include "../include/shell.h"
#include "../include/vga.h"
#include "../include/string.h"
#include "../include/fs.h"
#include "../include/editor.h"
#include "../include/app.h"
#include "../include/keyboard.h"

#define INPUT_MAX 128
#define PROMPT    "X-DOS> "

static char input_buf[INPUT_MAX];
static int  input_len = 0;

void shell_redraw(void) {
    vga_clear();
    kprint_color("X-DOS v0.1\n", C_INFO);
    kprint_color("==========\n", C_INFO);
    kprint("Tipd: "); kprint_color("help\n\n", C_PROMPT);
    kprint_color(PROMPT, C_PROMPT);
}

void shell_prompt(void) {
    kprint_color(PROMPT, C_PROMPT);
}

static void shell_exec(const char* cmd) {
    if (kstrlen(cmd) == 0) return;

    if (kstrcmp(cmd, "help") == 0) {
        kprint_color("Parancsok:\n", C_INFO);
        kprint("  help                    - ez a lista\n");
        kprint("  clear                   - kepernyo torles\n");
        kprint("  about                   - rendszer info\n");
        kprint("  echo <szoveg>           - kiiras\n");
        kprint("  edit <fajlnev>          - editor megnyitasa\n");
        kprint("  ls                      - fajlok listaja\n");
        kprint("  rm <fajlnev>            - fajl torlese\n");
        kprint("  cat <fajlnev>           - fajl tartalma\n");
        kprint("  set keyboard hu/en      - billentyuzet\n");
        kprint("  reboot                  - ujrainditas\n");

    } else if (kstrcmp(cmd, "clear") == 0) {
        vga_clear();

    } else if (kstrcmp(cmd, "about") == 0) {
        kprint_color("X-DOS v0.1\n", C_INFO);
        kprint("Arch  : x86 32-bit protected mode\n");
        kprint("Video : VGA 80x25\n");
        kprint("Keyb  : ");
        kprint_color(kb_layout == LAYOUT_HU ? "HU QWERTZ\n" : "EN QWERTY\n", C_PROMPT);

    } else if (kstrcmp(cmd, "ls") == 0) {
        int found = 0;
        for (int i = 0; i < FS_MAX_FILES; i++) {
            if (fs[i].used) {
                kprint("  ");
                kprint(fs[i].name);
                kprint("  (");
                kprint_int(fs[i].size);
                kprint(" byte)\n");
                found = 1;
            }
        }
        if (!found) kprint_color("  (ures filesystem)\n", C_INFO);

    } else if (cmd[0] == 'e' && cmd[1] == 'd' && cmd[2] == 'i' && cmd[3] == 't' && cmd[4] == ' ') {
        const char* fname = cmd + 5;
        if (kstrlen(fname) == 0) {
            kprint_color("Hasznalat: edit <fajlnev>\n", C_ERROR);
        } else {
            editor_open(fname);
        }

    } else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == ' ') {
        const char* fname = cmd + 4;
        ramfile_t* f = fs_find(fname);
        if (!f) {
            kprint_color("Nem talalhato: ", C_ERROR);
            kprint(fname);
            kprint("\n");
        } else {
            for (int i = 0; i < f->size; i++) vga_putchar(f->data[i]);
            kprint("\n");
        }

    } else if (cmd[0] == 'r' && cmd[1] == 'm' && cmd[2] == ' ') {
        const char* fname = cmd + 3;
        ramfile_t* f = fs_find(fname);
        if (!f) {
            kprint_color("Nem talalhato: ", C_ERROR);
            kprint(fname);
            kprint("\n");
        } else {
            fs_delete(fname);
            kprint_color("Torolve.\n", C_INFO);
        }

    } else if (kstrcmp(cmd, "set keyboard hu") == 0 || kstrcmp(cmd, "set keyboard hu_HU") == 0) {
        kb_layout = LAYOUT_HU;
        kprint_color("Billentyuzet: Magyar HU QWERTZ\n", C_INFO);

    } else if (kstrcmp(cmd, "set keyboard en") == 0 || kstrcmp(cmd, "set keyboard en_EN") == 0 ||
               kstrcmp(cmd, "set keyboard eng_eng") == 0) {
        kb_layout = LAYOUT_EN;
        kprint_color("Keyboard: English US QWERTY\n", C_INFO);

    } else if (kstrcmp(cmd, "reboot") == 0) {
        kprint_color("Ujrainditas...\n", C_INFO);
        struct {
            uint16_t limit;
            uint32_t base;
        } __attribute__((packed)) idt_ptr_reboot = {0, 0};
        __asm__ volatile("lidt %0\n int $0x00\n" : : "m"(idt_ptr_reboot));

    } else if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' && cmd[4] == ' ') {
        kprint(cmd + 5);
        kprint("\n");

    } else {
        kprint_color("Ismeretlen: ", C_ERROR);
        kprint(cmd);
        kprint("\n");
        kprint("Tipd: help\n");
    }
}

static void shell_enter(void) {
    kprint("\n");
    input_buf[input_len] = '\0';
    shell_exec(input_buf);
    input_len = 0;
    if (mode == MODE_SHELL) shell_prompt();
}

void shell_putchar(char c) {
    if (c == '\b') {
        if (input_len > 0) {
            input_len--;
            vga_putchar('\b');
        }
    } else if (c == '\n' || c == '\r') {
        shell_enter();
    } else {
        if (input_len < INPUT_MAX - 1) {
            input_buf[input_len++] = c;
            vga_putchar(c);
        }
    }
}

#include "../include/shell.h"
#include "../include/vga.h"
#include "../include/string.h"
#include "../include/fs.h"
#include "../include/editor.h"
#include "../include/app.h"
#include "../include/keyboard.h"
#include "../include/net.h"

#define INPUT_MAX 128
#define PROMPT "X-DOS> "

static char input_buf[INPUT_MAX];
static int input_len = 0;

void shell_redraw(void) {
    vga_clear();
    kprint_color("X-DOS v0.1\n", C_INFO);
    kprint_color("==========\n", C_INFO);
    kprint_color("Tipd: ", C_DEFAULT);
    kprint_color("help\n\n", C_PROMPT);
    kprint_color(PROMPT, C_PROMPT);
}

void shell_prompt(void) {
    kprint_color(PROMPT, C_PROMPT);
}

static void shell_exec(const char* cmd) {
    if (kstrlen(cmd) == 0) return;

    if (kstrcmp(cmd, "help") == 0) {
        kprint_color("Parancsok:\n", C_INFO);
        kprint_color(" help - ez a lista\n", C_DEFAULT);
        kprint_color(" clear - kepernyo torles\n", C_DEFAULT);
        kprint_color(" about - rendszer info\n", C_DEFAULT);
        kprint_color(" echo <szoveg> - kiiras\n", C_DEFAULT);
        kprint_color(" edit <fajlnev> - editor megnyitasa\n", C_DEFAULT);
        kprint_color(" ls - fajlok listaja\n", C_DEFAULT);
        kprint_color(" rm <fajlnev> - fajl torlese\n", C_DEFAULT);
        kprint_color(" cat <fajlnev> - fajl tartalma\n", C_DEFAULT);
        kprint_color(" set keyboard hu/en - billentyuzet\n", C_DEFAULT);
        kprint_color(" ping <host> - ping parancs (pl. ping 192.168.1.1)\n", C_DEFAULT);
        kprint_color(" XD install <csomagnev> - csomag telepitese (driver szint)\n", C_DEFAULT);
        kprint_color(" sync - fajlok mentese lemezre (XD-x32)\n", C_DEFAULT);
        kprint_color(" net info - halozat informacio (MAC, allapot)\n", C_DEFAULT);
        kprint_color(" net dump - kovetkezo csomag megjelenito\n", C_DEFAULT);
        kprint_color(" ping <host> - ping parancs (pl. ping 192.168.1.1)\n", C_DEFAULT);
        kprint_color(" XD install <csomagnev> - csomag telepitese (driver szint)\n", C_DEFAULT);
        kprint_color(" reboot - ujrainditas\n", C_DEFAULT);
    } else if (kstrcmp(cmd, "clear") == 0) {
        vga_clear();
    } else if (kstrcmp(cmd, "about") == 0) {
        kprint_color("X-DOS v0.1\n", C_INFO);
        kprint_color("Arch : x86 32-bit protected mode\n", C_DEFAULT);
        kprint_color("Video : VGA 80x25\n", C_DEFAULT);
        kprint_color("Keyb : ", C_DEFAULT);
        kprint_color(kb_layout == LAYOUT_HU ? "HU QWERTZ\n" : "EN QWERTY\n", C_PROMPT);
    } else if (kstrcmp(cmd, "ls") == 0) {
        int found = 0;
        for (int i = 0; i < FS_MAX_FILES; i++) {
            if (fs[i].used) {
                kprint_color(" ", C_DEFAULT);
                kprint_color(fs[i].name, C_DEFAULT);
                kprint_color(" (", C_DEFAULT);
                kprint_int(fs[i].size);
                kprint_color(" byte)\n", C_DEFAULT);
                found = 1;
            }
        }
        if (!found) kprint_color(" (ures filesystem)\n", C_INFO);
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
            kprint_color(fname, C_DEFAULT);
            kprint_color("\n", C_DEFAULT);
        } else {
            for (int i = 0; i < f->size; i++) vga_putchar(f->data[i]);
            kprint_color("\n", C_DEFAULT);
        }
    } else if (cmd[0] == 'r' && cmd[1] == 'm' && cmd[2] == ' ') {
        const char* fname = cmd + 3;
        ramfile_t* f = fs_find(fname);
        if (!f) {
            kprint_color("Nem talalhato: ", C_ERROR);
            kprint_color(fname, C_DEFAULT);
            kprint_color("\n", C_DEFAULT);
        } else {
            fs_delete(fname);
            kprint_color("Torolve.\n", C_INFO);
        }
    } else if (kstrcmp(cmd, "set keyboard hu") == 0 || kstrcmp(cmd, "set keyboard hu_HU") == 0) {
        kb_layout = LAYOUT_HU;
        kprint_color("Billentyuzet: Magyar HU QWERTZ\n", C_INFO);
    } else if (kstrcmp(cmd, "set keyboard en") == 0 || kstrcmp(cmd, "set keyboard en_EN") == 0 || kstrcmp(cmd, "set keyboard eng_eng") == 0) {
        kb_layout = LAYOUT_EN;
        kprint_color("Keyboard: English US QWERTY\n", C_INFO);
    } else if (kstrcmp(cmd, "reboot") == 0) {
        kprint_color("Ujrainditas...\n", C_INFO);
        struct { uint16_t limit; uint32_t base; } __attribute__((packed)) idt_ptr_reboot = {0, 0};
        __asm__ volatile("lidt %0\n int $0x00\n" : : "m"(idt_ptr_reboot));
    } else if (kstrcmp(cmd, "sync") == 0) {
        kprint_color("Fajlok mentese az XDisk (XD-x32) lemezre... ", C_INFO);
        fs_sync();
        kprint_color("Kesz.\n", C_PROMPT);
    } else if (kstrcmp(cmd, "net info") == 0) {
        if (!net_available()) {
            kprint_color("Halozati kartya nem talalhato (RTL8139).\n", C_ERROR);
        } else {
            uint8_t mac[NET_MAC_LEN];
            net_get_mac(mac);
            kprint_color("Halozat: ", C_INFO);
            kprint_color("RTL8139 (aktiv)\n", C_PROMPT);
            kprint_color("MAC: ", C_DEFAULT);
            const char* hex = "0123456789ABCDEF";
            for (int i = 0; i < NET_MAC_LEN; i++) {
                if (i > 0) kprint_color(":", C_DEFAULT);
                vga_putchar(hex[mac[i] >> 4]);
                vga_putchar(hex[mac[i] & 0xF]);
            }
            kprint_color("\n", C_DEFAULT);
        }
    } else if (kstrcmp(cmd, "net dump") == 0) {
        if (!net_available()) {
            kprint_color("Halozati kartya nem talalhato (RTL8139).\n", C_ERROR);
        } else {
            kprint_color("Var egy Ethernet csomagra... (halozati forgalom)\n", C_INFO);
            uint8_t pkt[NET_MTU];
            uint16_t len = 0;
            for (int tries = 0; tries < 500000 && len == 0; tries++) len = net_poll(pkt, NET_MTU);
            if (len == 0) {
                kprint_color("Nincs beerkezett csomag.\n", C_ERROR);
            } else {
                kprint_color("Csomag fogadva! Hossz: ", C_PROMPT);
                kprint_int(len);
                kprint_color(" byte\n", C_DEFAULT);
                kprint_color("DST MAC: ", C_DEFAULT);
                const char* hex2 = "0123456789ABCDEF";
                for (int i = 0; i < 6; i++) {
                    if (i > 0) vga_putchar(':');
                    vga_putchar(hex2[pkt[i] >> 4]);
                    vga_putchar(hex2[pkt[i] & 0xF]);
                }
                kprint_color(" SRC MAC: ", C_DEFAULT);
                for (int i = 6; i < 12; i++) {
                    if (i > 6) vga_putchar(':');
                    vga_putchar(hex2[pkt[i] >> 4]);
                    vga_putchar(hex2[pkt[i] & 0xF]);
                }
                uint16_t ethertype = (uint16_t)(pkt[12] << 8 | pkt[13]);
                kprint_color(" Type: 0x", C_DEFAULT);
                vga_putchar(hex2[(ethertype >> 12) & 0xF]);
                vga_putchar(hex2[(ethertype >> 8) & 0xF]);
                vga_putchar(hex2[(ethertype >> 4) & 0xF]);
                vga_putchar(hex2[(ethertype >> 0) & 0xF]);
                kprint_color("\n", C_DEFAULT);
            }
        }
    } else if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' && cmd[4] == ' ') {
        kprint_color(cmd + 5, C_DEFAULT);
        kprint_color("\n", C_DEFAULT);
    } else {
        kprint_color("Ismeretlen: ", C_ERROR);
        kprint_color(cmd, C_DEFAULT);
        kprint_color("\n", C_DEFAULT);
        kprint_color("Tipd: help\n", C_DEFAULT);
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
            /* Input karakterek színe: világosszürke (látható a fekete háttér felett) */
            uint8_t saved_color = cur_color;
            cur_color = 0x0F; /* Fehér szöveg, fekete háttér */
            vga_putchar(c);
            cur_color = saved_color;
        }
    }
}

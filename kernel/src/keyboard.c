#include "../include/keyboard.h"
#include "../include/io.h"
#include "../include/app.h"
#include "../include/shell.h"
#include "../include/editor.h"

/* ============================================================
 * Scan code -> ASCII táblák
 * ============================================================ */

static const char kb_en_low[] = {
0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b',0,
'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s',
'd','f','g','h','j','k','l',';','\'',0,0,'\\','z','x','c','v',
'b','n','m',',','.','/',0,'*',0,' '
};

static const char kb_en_hi[] = {
0,0,'!','@','#','$','%','^','&','*','(',')','_','+','\b',0,
'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A','S',
'D','F','G','H','J','K','L',':','"',0,0,'|','Z','X','C','V',
'B','N','M','<','>','?',0,'*',0,' '
};

/* HU QWERTZ – ékezetes betűk egyenlőre alapbetűre mappelve (lásd korábbi megjegyzés) */
static const char kb_hu_low[] = {
0,0,'1','2','3','4','5','6','7','8','9','0','o','u','\b',0,
'q','w','e','r','t','z','u','i','o','p','o','u','\n',0,'a','s',
'd','f','g','h','j','k','l','e','a',0,0,'u','y','x','c','v',
'b','n','m',',','.','-',0,'*',0,' '
};

static const char kb_hu_hi[] = {
0,0,'!','"','#','$','%','&','/','(',')','=','O','U','\b',0,
'Q','W','E','R','T','Z','U','I','O','P','O','U','\n',0,'A','S',
'D','F','G','H','J','K','L','E','A',0,0,'U','Y','X','C','V',
'B','N','M',';',':','_',0,'*',0,' '
};

kb_layout_t kb_layout = LAYOUT_EN;

static const char* get_table(int shift) {
    if (kb_layout == LAYOUT_HU) return shift ? kb_hu_hi : kb_hu_low;
    return shift ? kb_en_hi : kb_en_low;
}

static int shift_on = 0, ctrl_on = 0, extended = 0;

/* ============================================================
 * IRQ1 handler
 * ============================================================ */

void keyboard_handler_c(void) {
    uint8_t sc = inb(0x60);

    if (sc == 0xE0) { extended = 1; return; }   /* extended prefix, EOI nélkül várjuk a folytatást... */

    if (sc == SC_CTRL) { ctrl_on = 1; outb(0x20, 0x20); return; }
    if (sc == (SC_CTRL | 0x80)) { ctrl_on = 0; outb(0x20, 0x20); return; }

    if (sc == SC_LSHIFT || sc == SC_RSHIFT) { shift_on = 1; outb(0x20, 0x20); return; }
    if (sc == (SC_LSHIFT | 0x80) || sc == (SC_RSHIFT | 0x80)) { shift_on = 0; outb(0x20, 0x20); return; }

    if (!(sc & 0x80)) {
        uint8_t raw_sc = sc;

        if (extended) {
            /* nyilak, Home/End/Del stb. - csak editor módban érdekes most */
            if (mode == MODE_EDITOR) editor_key(0, raw_sc, ctrl_on);
            extended = 0;
            outb(0x20, 0x20);
            return;
        }

        char c = 0;
        if (sc < SCANCODE_MAX) c = get_table(shift_on)[sc];

        if (mode == MODE_SHELL) {
            if (c) shell_putchar(c);
        } else {
            editor_key(c, raw_sc, ctrl_on);
            if (mode == MODE_SHELL) shell_redraw();   /* editorból kiléptünk */
        }
    } else {
        extended = 0;
    }

    outb(0x20, 0x20);
}
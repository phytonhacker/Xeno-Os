#include "../include/editor.h"
#include "../include/vga.h"
#include "../include/string.h"
#include "../include/app.h"
#include "../include/keyboard.h"

editor_t ed;

static void ed_pos_to_rc(int p, int* row, int* col) {
    int r = 0, c = 0;
    for (int i = 0; i < p; i++) {
        if (ed.buf[i] == '\n') { r++; c = 0; } else c++;
    }
    *row = r; *col = c;
}

static int ed_row_start(int row) {
    int r = 0;
    for (int i = 0; i < ed.len; i++) {
        if (r == row) return i;
        if (ed.buf[i] == '\n') r++;
    }
    return ed.len;
}

static int ed_row_len(int row) {
    int s = ed_row_start(row), l = 0;
    while (s + l < ed.len && ed.buf[s + l] != '\n') l++;
    return l;
}

static int ed_total_rows(void) {
    int r = 1;
    for (int i = 0; i < ed.len; i++) if (ed.buf[i] == '\n') r++;
    return r;
}

static void ed_draw_statusbar(void) {
    char line[VGA_WIDTH + 1];
    for (int i = 0; i < VGA_WIDTH; i++) line[i] = ' ';
    line[VGA_WIDTH] = 0;

    int ni = 0;
    const char* fn = ed.filename[0] ? ed.filename : "[Nincs nev]";
    while (fn[ni] && ni < 30) { line[ni] = fn[ni]; ni++; }
    if (ed.dirty) { line[ni++] = '*'; }

    int row, col;
    ed_pos_to_rc(ed.pos, &row, &col);
    char tmp[20]; int ti = 0;
    const char* lbl = "Sor:";
    while (lbl[ti]) tmp[ti] = lbl[ti++];
    int rr = row + 1; char nb[6]; int ni2 = 0;
    if (rr == 0) { nb[ni2++] = '0'; }
    else { int rrtmp = rr; while (rrtmp > 0) { nb[ni2++] = '0' + (rrtmp % 10); rrtmp /= 10; } }
    for (int k = ni2 - 1; k >= 0; k--) tmp[ti++] = nb[k];
    tmp[ti++] = ' '; tmp[ti++] = 'O'; tmp[ti++] = 's'; tmp[ti++] = 'l'; tmp[ti++] = ':';
    int cc = col + 1; int ni3 = 0; char cb[6];
    if (cc == 0) { cb[ni3++] = '0'; }
    else { int cctmp = cc; while (cctmp > 0) { cb[ni3++] = '0' + (cctmp % 10); cctmp /= 10; } }
    for (int k = ni3 - 1; k >= 0; k--) tmp[ti++] = cb[k];
    tmp[ti] = 0;
    int tlen = ti;
    int start = VGA_WIDTH - tlen - 1;
    for (int k = 0; k < tlen; k++) line[start + k] = tmp[k];

    for (int x = 0; x < VGA_WIDTH; x++)
        vga_putc_at(x, 0, line[x], C_TITLE);

    const char* help = "^S Ment  ^Q Kil  ^N Uj  Del Torol";
    for (int x = 0; x < VGA_WIDTH; x++)
        vga_putc_at(x, VGA_HEIGHT - 1, ' ', C_STATUS);
    for (int x = 0; help[x] && x < VGA_WIDTH; x++)
        vga_putc_at(x, VGA_HEIGHT - 1, help[x], C_STATUS);
}

static void ed_redraw(void) {
    int cur_row, cur_col;
    ed_pos_to_rc(ed.pos, &cur_row, &cur_col);

    if (cur_row < ed.view_row) ed.view_row = cur_row;
    if (cur_row >= ed.view_row + ED_ROWS) ed.view_row = cur_row - ED_ROWS + 1;

    for (int y = 1; y < VGA_HEIGHT - 1; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            vga_putc_at(x, y, ' ', C_DEFAULT);

    int total = ed_total_rows();
    for (int r = 0; r < ED_ROWS && (ed.view_row + r) < total; r++) {
        int row_idx = ed.view_row + r;
        int rs = ed_row_start(row_idx);
        int rl = ed_row_len(row_idx);
        if (rl > ED_COLS) rl = ED_COLS;
        for (int c = 0; c < rl; c++)
            vga_putc_at(c, r + 1, ed.buf[rs + c], C_DEFAULT);
    }

    ed_draw_statusbar();

    int screen_row = cur_row - ed.view_row + 1;
    int screen_col = cur_col;
    if (screen_col >= VGA_WIDTH) screen_col = VGA_WIDTH - 1;
    cur_set(screen_row, screen_col);
}

void editor_open(const char* filename) {
    kmemset(&ed, 0, sizeof(editor_t));
    kstrcpy(ed.filename, filename);

    ramfile_t* f = fs_find(filename);
    if (f) {
        for (int i = 0; i < f->size; i++) ed.buf[i] = f->data[i];
        ed.len = f->size;
    } else {
        ed.len = 0;
    }
    ed.buf[ed.len] = 0;
    ed.pos = 0; ed.view_row = 0; ed.dirty = 0;
    mode = MODE_EDITOR;
    ed_redraw();
}

static void ed_save(void) {
    ramfile_t* f = fs_create(ed.filename);
    if (!f) { return; }
    for (int i = 0; i < ed.len; i++) f->data[i] = ed.buf[i];
    f->data[ed.len] = 0;
    f->size = ed.len;
    ed.dirty = 0;
    ed_redraw();
}

static void ed_insert(char c) {
    if (ed.len >= ED_MAXLEN) return;
    for (int i = ed.len; i > ed.pos; i--) ed.buf[i] = ed.buf[i - 1];
    ed.buf[ed.pos] = c;
    ed.len++; ed.pos++;
    ed.dirty = 1;
}

static void ed_backspace(void) {
    if (ed.pos == 0) return;
    for (int i = ed.pos - 1; i < ed.len - 1; i++) ed.buf[i] = ed.buf[i + 1];
    ed.len--; ed.pos--;
    ed.buf[ed.len] = 0;
    ed.dirty = 1;
}

static void ed_delete(void) {
    if (ed.pos >= ed.len) return;
    for (int i = ed.pos; i < ed.len - 1; i++) ed.buf[i] = ed.buf[i + 1];
    ed.len--;
    ed.buf[ed.len] = 0;
    ed.dirty = 1;
}

static void ed_move_left(void) { if (ed.pos > 0) ed.pos--; }
static void ed_move_right(void) { if (ed.pos < ed.len) ed.pos++; }

static void ed_move_up(void) {
    int row, col; ed_pos_to_rc(ed.pos, &row, &col);
    if (row == 0) { ed.pos = 0; return; }
    int prev_len = ed_row_len(row - 1);
    int nc = col < prev_len ? col : prev_len;
    ed.pos = ed_row_start(row - 1) + nc;
}

static void ed_move_down(void) {
    int row, col; ed_pos_to_rc(ed.pos, &row, &col);
    int total = ed_total_rows();
    if (row >= total - 1) { ed.pos = ed.len; return; }
    int next_len = ed_row_len(row + 1);
    int nc = col < next_len ? col : next_len;
    ed.pos = ed_row_start(row + 1) + nc;
}

static void ed_home(void) {
    int row, col; ed_pos_to_rc(ed.pos, &row, &col);
    ed.pos = ed_row_start(row);
}
static void ed_end(void) {
    int row, col; ed_pos_to_rc(ed.pos, &row, &col);
    ed.pos = ed_row_start(row) + ed_row_len(row);
}

void editor_key(char c, uint8_t raw_sc, int ctrl_on) {
    if (ctrl_on) {
        if (c == 's' || c == 'S') { ed_save(); return; }
        if (c == 'q' || c == 'Q') {
            mode = MODE_SHELL;
            vga_clear();
            return;
        }
        if (c == 'n' || c == 'N') {
            editor_open("untitled.txt");
            return;
        }
        return;
    }

    /* speciális scan code-ok (extended) */
    if (raw_sc == SC_UP)   { ed_move_up();    ed_redraw(); return; }
    if (raw_sc == SC_DOWN) { ed_move_down();  ed_redraw(); return; }
    if (raw_sc == SC_LEFT) { ed_move_left();  ed_redraw(); return; }
    if (raw_sc == SC_RIGHT) { ed_move_right(); ed_redraw(); return; }
    if (raw_sc == SC_HOME) { ed_home();       ed_redraw(); return; }
    if (raw_sc == SC_END)  { ed_end();        ed_redraw(); return; }
    if (raw_sc == SC_DEL)  { ed_delete();     ed_redraw(); return; }

    if (c == '\b') { ed_backspace(); ed_redraw(); return; }
    if (c == '\n') { ed_insert('\n'); ed_redraw(); return; }

    if (c >= 32 && c < 127) { ed_insert(c); ed_redraw(); return; }
}
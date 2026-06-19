
#include <stdint.h>

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

static inline uint8_t inb(uint16_t port) {
    uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(port)); return v;
}
static inline void outb(uint16_t port, uint8_t v) {
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(port));
}

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define C_DEFAULT   0x07
#define C_PROMPT    0x0A
#define C_ERROR     0x0C
#define C_INFO      0x0B
#define C_TITLE     0x70   
#define C_STATUS    0x70

static uint16_t* const vga = (uint16_t*)0xB8000;
static int cx = 0, cy = 0;
static uint8_t cur_color = C_DEFAULT;

static void cur_update(void) {
    uint16_t p = (uint16_t)(cy * VGA_WIDTH + cx);
    outb(0x3D4,0x0F); outb(0x3D5,(uint8_t)(p&0xFF));
    outb(0x3D4,0x0E); outb(0x3D5,(uint8_t)(p>>8));
}
static void cur_enable(void) {
    outb(0x3D4,0x0A); outb(0x3D5,(inb(0x3D5)&0xC0)|14);
    outb(0x3D4,0x0B); outb(0x3D5,(inb(0x3D5)&0xE0)|15);
}

static void vga_putc_at(int x, int y, char c, uint8_t col) {
    vga[y*VGA_WIDTH+x] = (uint16_t)((col<<8)|(uint8_t)c);
}

static void vga_clear(void) {
    for(int i=0;i<VGA_WIDTH*VGA_HEIGHT;i++)
        vga[i]=(uint16_t)((C_DEFAULT<<8)|' ');
    cx=cy=0; cur_update();
}

static void vga_scroll(void) {
    for(int y=0;y<VGA_HEIGHT-1;y++)
        for(int x=0;x<VGA_WIDTH;x++)
            vga[y*VGA_WIDTH+x]=vga[(y+1)*VGA_WIDTH+x];
    for(int x=0;x<VGA_WIDTH;x++)
        vga_putc_at(x,VGA_HEIGHT-1,' ',C_DEFAULT);
}

static void vga_putchar(char c) {
    if(c=='\n'||c=='\r'){ cx=0; cy++; }
    else if(c=='\b'){ if(cx>0){ cx--; vga_putc_at(cx,cy,' ',cur_color); } }
    else { vga_putc_at(cx,cy,c,cur_color); if(++cx>=VGA_WIDTH){cx=0;cy++;} }
    if(cy>=VGA_HEIGHT){ vga_scroll(); cy=VGA_HEIGHT-1; }
    cur_update();
}

void kprint(const char*s){ while(*s) vga_putchar(*s++); }

void kprint_color(const char*s,uint8_t col){
    uint8_t o=cur_color; cur_color=col;
    while(*s) vga_putchar(*s++);
    cur_color=o;
}

static void kprint_int(int n){
    if(n<0){vga_putchar('-');n=-n;}
    if(n==0){vga_putchar('0');return;}
    char b[12];int i=0;
    while(n>0){b[i++]='0'+(n%10);n/=10;}
    while(i>0) vga_putchar(b[--i]);
}

static int kstrcmp(const char*a,const char*b){
    while(*a&&*b&&*a==*b){a++;b++;} return *a-*b;
}
static int kstrlen(const char*s){int i=0;while(s[i])i++;return i;}
static void kstrcpy(char*d,const char*s){while((*d++=*s++));}
static void kmemset(void*p,uint8_t v,int n){
    uint8_t*b=(uint8_t*)p; while(n--)*b++=v;
}

#define FS_MAX_FILES   16
#define FS_MAX_NAME    32
#define FS_MAX_SIZE  4096   

typedef struct {
    char     name[FS_MAX_NAME];
    char     data[FS_MAX_SIZE];
    int      size;
    int      used;
} ramfile_t;

static ramfile_t fs[FS_MAX_FILES];

static ramfile_t* fs_find(const char*name){
    for(int i=0;i<FS_MAX_FILES;i++)
        if(fs[i].used && kstrcmp(fs[i].name,name)==0) return &fs[i];
    return 0;
}

static ramfile_t* fs_create(const char*name){
    ramfile_t*f=fs_find(name);
    if(f) return f;
    for(int i=0;i<FS_MAX_FILES;i++){
        if(!fs[i].used){
            kmemset(&fs[i],0,sizeof(ramfile_t));
            kstrcpy(fs[i].name,name);
            fs[i].used=1;
            fs[i].size=0;
            return &fs[i];
        }
    }
    return 0;
}

static void fs_delete(const char*name){
    ramfile_t*f=fs_find(name);
    if(f){ kmemset(f,0,sizeof(ramfile_t)); }
}

static const char kb_en_low[]=
{0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b',0,
 'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s',
 'd','f','g','h','j','k','l',';','\'',0,0,'\\','z','x','c','v',
 'b','n','m',',','.','/',0,'*',0,' '};

static const char kb_en_hi[]=
{0,0,'!','@','#','$','%','^','&','*','(',')', '_','+','\b',0,
 'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A','S',
 'D','F','G','H','J','K','L',':','"', 0,0, '|','Z','X','C','V',
 'B','N','M','<','>','?',0,'*',0,' '};

static const char kb_hu_low[]=
{0,0,'1','2','3','4','5','6','7','8','9','0','o','u','\b',0,
 'q','w','e','r','t','z','u','i','o','p','o','u','\n',0,'a','s',
 'd','f','g','h','j','k','l','e','a',0,0,'u','y','x','c','v',
 'b','n','m',',','.','-',0,'*',0,' '};

static const char kb_hu_hi[]=
{0,0,'!','"','#','$','%','&','/','(',')', '=','O','U','\b',0,
 'Q','W','E','R','T','Z','U','I','O','P','O','U','\n',0,'A','S',
 'D','F','G','H','J','K','L','E','A',0,0,'U','Y','X','C','V',
 'B','N','M',';',':','_',0,'*',0,' '};

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
static kb_layout_t kb_layout = LAYOUT_EN;

static const char* get_table(int shift){
    if(kb_layout==LAYOUT_HU) return shift?kb_hu_hi:kb_hu_low;
    return shift?kb_en_hi:kb_en_low;
}

static int shift_on=0, ctrl_on=0, extended=0;

/* ============================================================
 * MODE
 * ============================================================ */
typedef enum { MODE_SHELL, MODE_EDITOR } app_mode_t;
static app_mode_t mode = MODE_SHELL;

/* ============================================================
 * EDITOR
 * ============================================================ */
#define ED_ROWS   (VGA_HEIGHT-2)   
#define ED_COLS   VGA_WIDTH        
#define ED_MAXLEN (FS_MAX_SIZE-1)

typedef struct {
    char     filename[FS_MAX_NAME];
    char     buf[FS_MAX_SIZE];     
    int      len;                 
    int      pos;                  
    int      view_row;           
    int      dirty;                
} editor_t;

static editor_t ed;

static void ed_pos_to_rc(int p, int*row, int*col){
    int r=0,c=0;
    for(int i=0;i<p;i++){
        if(ed.buf[i]=='\n'){r++;c=0;}else c++;
    }
    *row=r; *col=c;
}

static int ed_row_start(int row){
    int r=0;
    for(int i=0;i<ed.len;i++){
        if(r==row) return i;
        if(ed.buf[i]=='\n') r++;
    }
    return ed.len;
}

static int ed_row_len(int row){
    int s=ed_row_start(row),l=0;
    while(s+l<ed.len && ed.buf[s+l]!='\n') l++;
    return l;
}

static int ed_total_rows(void){
    int r=1;
    for(int i=0;i<ed.len;i++) if(ed.buf[i]=='\n') r++;
    return r;
}

static void ed_draw_statusbar(void){
    char line[VGA_WIDTH+1];
    for(int i=0;i<VGA_WIDTH;i++) line[i]=' ';
    line[VGA_WIDTH]=0;

    int ni=0;
    const char*fn=ed.filename[0]?ed.filename:"[Nincs név]";
    while(fn[ni]&&ni<30){ line[ni]=fn[ni]; ni++; }
    if(ed.dirty){ line[ni++]='*'; }

    int row,col;
    ed_pos_to_rc(ed.pos,&row,&col);
    char tmp[20]; int ti=0;
    const char*lbl="Sor:";
    while(lbl[ti]) tmp[ti]=lbl[ti++];
    int rr=row+1; char nb[6]; int ni2=0;
    if(rr==0){nb[ni2++]='0';}
    else{ int rrtmp=rr; while(rrtmp>0){nb[ni2++]='0'+(rrtmp%10);rrtmp/=10;} }
    for(int k=ni2-1;k>=0;k--) tmp[ti++]=nb[k];
    tmp[ti++]=' '; tmp[ti++]='O'; tmp[ti++]='s'; tmp[ti++]='l'; tmp[ti++]=':';
    int cc=col+1; int ni3=0; char cb[6];
    if(cc==0){cb[ni3++]='0';}
    else{ int cctmp=cc; while(cctmp>0){cb[ni3++]='0'+(cctmp%10);cctmp/=10;} }
    for(int k=ni3-1;k>=0;k--) tmp[ti++]=cb[k];
    tmp[ti]=0;
    int tlen=ti;
    int start=VGA_WIDTH-tlen-1;
    for(int k=0;k<tlen;k++) line[start+k]=tmp[k];

    for(int x=0;x<VGA_WIDTH;x++)
        vga_putc_at(x,0,line[x],C_TITLE);

    const char*help="^S Ment  ^Q Kil  ^N Uj  Del Torol";
    for(int x=0;x<VGA_WIDTH;x++)
        vga_putc_at(x,VGA_HEIGHT-1,' ',C_STATUS);
    for(int x=0;help[x]&&x<VGA_WIDTH;x++)
        vga_putc_at(x,VGA_HEIGHT-1,help[x],C_STATUS);
}

static void ed_redraw(void){
    int cur_row, cur_col;
    ed_pos_to_rc(ed.pos, &cur_row, &cur_col);

    if(cur_row < ed.view_row) ed.view_row = cur_row;
    if(cur_row >= ed.view_row + ED_ROWS) ed.view_row = cur_row - ED_ROWS + 1;

    for(int y=1;y<VGA_HEIGHT-1;y++)
        for(int x=0;x<VGA_WIDTH;x++)
            vga_putc_at(x,y,' ',C_DEFAULT);

    int total = ed_total_rows();
    for(int r=0; r<ED_ROWS && (ed.view_row+r)<total; r++){
        int row_idx = ed.view_row + r;
        int rs = ed_row_start(row_idx);
        int rl = ed_row_len(row_idx);
        if(rl>ED_COLS) rl=ED_COLS;
        for(int c=0;c<rl;c++)
            vga_putc_at(c, r+1, ed.buf[rs+c], C_DEFAULT);
    }

    ed_draw_statusbar();

    int screen_row = cur_row - ed.view_row + 1;
    int screen_col = cur_col;
    if(screen_col >= VGA_WIDTH) screen_col = VGA_WIDTH-1;
    uint16_t p = (uint16_t)(screen_row * VGA_WIDTH + screen_col);
    outb(0x3D4,0x0F); outb(0x3D5,(uint8_t)(p&0xFF));
    outb(0x3D4,0x0E); outb(0x3D5,(uint8_t)(p>>8));
}

static void ed_open(const char*filename){
    kmemset(&ed,0,sizeof(editor_t));
    kstrcpy(ed.filename, filename);

    ramfile_t*f = fs_find(filename);
    if(f){
        for(int i=0;i<f->size;i++) ed.buf[i]=f->data[i];
        ed.len = f->size;
    } else {
        ed.len = 0;
    }
    ed.buf[ed.len]=0;
    ed.pos=0; ed.view_row=0; ed.dirty=0;
    mode = MODE_EDITOR;
    ed_redraw();
}

static void ed_save(void){
    ramfile_t*f = fs_create(ed.filename);
    if(!f){ return; }
    for(int i=0;i<ed.len;i++) f->data[i]=ed.buf[i];
    f->data[ed.len]=0;
    f->size=ed.len;
    ed.dirty=0;
    ed_redraw();
}

static void ed_insert(char c){
    if(ed.len>=ED_MAXLEN) return;
    for(int i=ed.len;i>ed.pos;i--) ed.buf[i]=ed.buf[i-1];
    ed.buf[ed.pos]=c;
    ed.len++; ed.pos++;
    ed.dirty=1;
}

static void ed_backspace(void){
    if(ed.pos==0) return;
    for(int i=ed.pos-1;i<ed.len-1;i++) ed.buf[i]=ed.buf[i+1];
    ed.len--; ed.pos--;
    ed.buf[ed.len]=0;
    ed.dirty=1;
}

static void ed_delete(void){
    if(ed.pos>=ed.len) return;
    for(int i=ed.pos;i<ed.len-1;i++) ed.buf[i]=ed.buf[i+1];
    ed.len--;
    ed.buf[ed.len]=0;
    ed.dirty=1;
}

static void ed_move_left(void){  if(ed.pos>0) ed.pos--; }
static void ed_move_right(void){ if(ed.pos<ed.len) ed.pos++; }

static void ed_move_up(void){
    int row,col; ed_pos_to_rc(ed.pos,&row,&col);
    if(row==0){ ed.pos=0; return; }
    int prev_len=ed_row_len(row-1);
    int nc=col<prev_len?col:prev_len;
    ed.pos=ed_row_start(row-1)+nc;
}

static void ed_move_down(void){
    int row,col; ed_pos_to_rc(ed.pos,&row,&col);
    int total=ed_total_rows();
    if(row>=total-1){ ed.pos=ed.len; return; }
    int next_len=ed_row_len(row+1);
    int nc=col<next_len?col:next_len;
    ed.pos=ed_row_start(row+1)+nc;
}

static void ed_home(void){
    int row,col; ed_pos_to_rc(ed.pos,&row,&col);
    ed.pos=ed_row_start(row);
}
static void ed_end(void){
    int row,col; ed_pos_to_rc(ed.pos,&row,&col);
    ed.pos=ed_row_start(row)+ed_row_len(row);
}

static void ed_key(char c, uint8_t sc){
    (void)sc;

    if(ctrl_on){
        if(c=='s'||c=='S'){ ed_save(); return; }
        if(c=='q'||c=='Q'){
            mode=MODE_SHELL;
            vga_clear();
            return;
        }
        if(c=='n'||c=='N'){
            ed_open("untitled.txt");
            return;
        }
        return;
    }

    /* speciális scan code-ok (extended) */
    if(sc==SC_UP)   { ed_move_up();    ed_redraw(); return; }
    if(sc==SC_DOWN) { ed_move_down();  ed_redraw(); return; }
    if(sc==SC_LEFT) { ed_move_left();  ed_redraw(); return; }
    if(sc==SC_RIGHT){ ed_move_right(); ed_redraw(); return; }
    if(sc==SC_HOME) { ed_home();       ed_redraw(); return; }
    if(sc==SC_END)  { ed_end();        ed_redraw(); return; }
    if(sc==SC_DEL)  { ed_delete();     ed_redraw(); return; }

    if(c=='\b'){ ed_backspace(); ed_redraw(); return; }
    if(c=='\n'){ ed_insert('\n'); ed_redraw(); return; }

    if(c>=32 && c<127){ ed_insert(c); ed_redraw(); return; }
}

/* ============================================================
 * SHELL
 * ============================================================ */
#define INPUT_MAX 128
#define PROMPT    "X-DOS> "

static char input_buf[INPUT_MAX];
static int  input_len=0;

static void shell_redraw(void){
    vga_clear();
    kprint_color("X-DOS v0.1\n",C_INFO);
    kprint_color("==========\n",C_INFO);
    kprint("Tipd: "); kprint_color("help\n\n",C_PROMPT);
    kprint_color(PROMPT,C_PROMPT);
}

static void shell_prompt(void){ kprint_color(PROMPT,C_PROMPT); }

static void shell_exec(const char*cmd){
    if(kstrlen(cmd)==0) return;

    if(kstrcmp(cmd,"help")==0){
        kprint_color("Parancsok:\n",C_INFO);
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

    } else if(kstrcmp(cmd,"clear")==0){
        vga_clear();

    } else if(kstrcmp(cmd,"about")==0){
        kprint_color("X-DOS v0.1\n",C_INFO);
        kprint("Arch  : x86 32-bit protected mode\n");
        kprint("Video : VGA 80x25\n");
        kprint("Keyb  : ");
        kprint_color(kb_layout==LAYOUT_HU?"HU QWERTZ\n":"EN QWERTY\n",C_PROMPT);

    } else if(kstrcmp(cmd,"ls")==0){
        int found=0;
        for(int i=0;i<FS_MAX_FILES;i++){
            if(fs[i].used){
                kprint("  ");
                kprint(fs[i].name);
                kprint("  (");
                kprint_int(fs[i].size);
                kprint(" byte)\n");
                found=1;
            }
        }
        if(!found) kprint_color("  (ures filesystem)\n",C_INFO);

    } else if(cmd[0]=='e'&&cmd[1]=='d'&&cmd[2]=='i'&&cmd[3]=='t'&&cmd[4]==' '){
        const char*fname=cmd+5;
        if(kstrlen(fname)==0){
            kprint_color("Hasznalatl: edit <fajlnev>\n",C_ERROR);
        } else {
            ed_open(fname);
            return; 
        }

    } else if(cmd[0]=='c'&&cmd[1]=='a'&&cmd[2]=='t'&&cmd[3]==' '){
        const char*fname=cmd+4;
        ramfile_t*f=fs_find(fname);
        if(!f){ kprint_color("Nem talalhato: ",C_ERROR); kprint(fname); kprint("\n"); }
        else  { for(int i=0;i<f->size;i++) vga_putchar(f->data[i]); kprint("\n"); }

    } else if(cmd[0]=='r'&&cmd[1]=='m'&&cmd[2]==' '){
        const char*fname=cmd+3;
        ramfile_t*f=fs_find(fname);
        if(!f){ kprint_color("Nem talalhato: ",C_ERROR); kprint(fname); kprint("\n"); }
        else  { fs_delete(fname); kprint_color("Torolve.\n",C_INFO); }

    } else if(kstrcmp(cmd,"set keyboard hu")==0||kstrcmp(cmd,"set keyboard hu_HU")==0){
        kb_layout=LAYOUT_HU;
        kprint_color("Billentyuzet: Magyar HU QWERTZ\n",C_INFO);

    } else if(kstrcmp(cmd,"set keyboard en")==0||kstrcmp(cmd,"set keyboard en_EN")==0||
              kstrcmp(cmd,"set keyboard eng_eng")==0){
        kb_layout=LAYOUT_EN;
        kprint_color("Keyboard: English US QWERTY\n",C_INFO);

    } else if(kstrcmp(cmd,"reboot")==0){
        kprint_color("Ujrainditas...\n",C_INFO);
        idt_ptr.limit=0; idt_ptr.base=0;
        __asm__ volatile("lidt %0\n int $0x00\n"::"m"(idt_ptr));

    } else if(cmd[0]=='e'&&cmd[1]=='c'&&cmd[2]=='h'&&cmd[3]=='o'&&cmd[4]==' '){
        kprint(cmd+5); kprint("\n");

    } else {
        kprint_color("Ismeretlen: ",C_ERROR); kprint(cmd); kprint("\n");
        kprint("Tipd: help\n");
    }
}

static void shell_enter(void){
    kprint("\n");
    input_buf[input_len]='\0';
    shell_exec(input_buf);
    input_len=0;
    if(mode==MODE_SHELL) shell_prompt();
}

static void shell_putchar(char c){
    if(c=='\b'){
        if(input_len>0){ input_len--; vga_putchar('\b'); }
    } else if(c=='\n'||c=='\r'){
        shell_enter();
    } else {
        if(input_len<INPUT_MAX-1){ input_buf[input_len++]=c; vga_putchar(c); }
    }
}

void keyboard_handler_c(void){
    uint8_t sc=inb(0x60);

    if(sc==0xE0){ extended=1; return; }

    if(sc==SC_CTRL){ ctrl_on=1; outb(0x20,0x20); return; }
    if(sc==(SC_CTRL|0x80)){ ctrl_on=0; outb(0x20,0x20); return; }

    if(sc==SC_LSHIFT||sc==SC_RSHIFT){ shift_on=1; outb(0x20,0x20); return; }
    if(sc==(SC_LSHIFT|0x80)||sc==(SC_RSHIFT|0x80)){ shift_on=0; outb(0x20,0x20); return; }

    if(!(sc&0x80)){
        uint8_t raw_sc = sc;
        if(extended){
            if(mode==MODE_EDITOR){
                ed_key(0, raw_sc);
            }
            extended=0;
            outb(0x20,0x20);
            return;
        }
        extended=0;

        char c=0;
        if(sc<SCANCODE_MAX) c=get_table(shift_on)[sc];

        if(mode==MODE_SHELL){
            if(c) shell_putchar(c);
        } else {
            ed_key(c, raw_sc);
            /* ha visszatért shellbe */
            if(mode==MODE_SHELL) shell_redraw();
        }
    } else {
        extended=0;
    }

    outb(0x20,0x20);
}

static void idt_set_gate(uint8_t n,uint32_t h,uint16_t s,uint8_t f){
    idt[n].offset_low=h&0xFFFF; idt[n].selector=s;
    idt[n].zero=0; idt[n].type_attr=f;
    idt[n].offset_high=(h>>16)&0xFFFF;
}

static void pic_init(void){
    outb(0x20,0x11); outb(0xA0,0x11);
    outb(0x21,0x20); outb(0xA1,0x28);
    outb(0x21,0x04); outb(0xA1,0x02);
    outb(0x21,0x01); outb(0xA1,0x01);
    outb(0x21,0xFD); outb(0xA1,0xFF);
}

extern void keyboard_isr(void);

static void idt_init(void){
    idt_ptr.limit=sizeof(idt)-1;
    idt_ptr.base=(uint32_t)&idt;
    idt_set_gate(0x21,(uint32_t)keyboard_isr,0x08,0x8E);
    __asm__ volatile("lidt %0"::"m"(idt_ptr));
}

void kernel_main(void){
    vga_clear();
    cur_enable();
    pic_init();
    idt_init();

    kprint_color("X-DOS v0.1\n",C_INFO);
    kprint_color("==========\n",C_INFO);
    kprint("Tipd: "); kprint_color("help\n\n",C_PROMPT);
    shell_prompt();

    __asm__ volatile("sti");
    for(;;) __asm__ volatile("hlt");
}
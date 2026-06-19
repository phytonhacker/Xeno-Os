#ifndef EDITOR_H
#define EDITOR_H

#include "types.h"
#include "fs.h"

#define ED_ROWS   (25 - 2)      /* statusbar (felül) + helpbar (alul) miatt -2 */
#define ED_COLS   80
#define ED_MAXLEN (FS_MAX_SIZE - 1)

typedef struct {
    char filename[FS_MAX_NAME];
    char buf[FS_MAX_SIZE];
    int  len;
    int  pos;        /* kurzor pozíció a bufferben (karakter index) */
    int  view_row;   /* felül látható sor (scrollozáshoz) */
    int  dirty;
} editor_t;

extern editor_t ed;

/* Megnyitja a fájlt szerkesztésre (ha nem létezik, üres új fájl) */
void editor_open(const char* filename);

/* Egy lenyomott billentyű feldolgozása.
 * c       = ASCII karakter (0 ha speciális/extended billentyű)
 * raw_sc  = nyers scan code (nyilak, Home/End/Del azonosításához)
 * ctrl_on = Ctrl jelenleg lenyomva-e
 */
void editor_key(char c, uint8_t raw_sc, int ctrl_on);

#endif
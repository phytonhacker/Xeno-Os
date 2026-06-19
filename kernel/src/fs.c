#include "../include/fs.h"
#include "../include/string.h"

/*
 * RAM filesystem - egyenlőre minden fájl a memóriában él, reboot után
 * elvész. Később ide jön az X-DOS saját, lemezre író fájlrendszere;
 * a fs_find/fs_create/fs_delete signature-öket úgy terveztem, hogy
 * a hívók (shell.c, editor.c) ne kelljen módosítani amikor lecseréljük
 * a háttértárolásra.
 */

ramfile_t fs[FS_MAX_FILES];

ramfile_t* fs_find(const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (fs[i].used && kstrcmp(fs[i].name, name) == 0) return &fs[i];
    return 0;
}

ramfile_t* fs_create(const char* name) {
    ramfile_t* f = fs_find(name);
    if (f) return f;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!fs[i].used) {
            kmemset(&fs[i], 0, sizeof(ramfile_t));
            kstrcpy(fs[i].name, name);
            fs[i].used = 1;
            fs[i].size = 0;
            return &fs[i];
        }
    }
    return 0;   /* nincs hely - később hibakód lenne */
}

void fs_delete(const char* name) {
    ramfile_t* f = fs_find(name);
    if (f) kmemset(f, 0, sizeof(ramfile_t));
}
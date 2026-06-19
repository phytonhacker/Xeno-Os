#ifndef FS_H
#define FS_H

#include "types.h"

#define FS_MAX_FILES  16
#define FS_MAX_NAME   32
#define FS_MAX_SIZE   4096   /* max 4KB / fájl, egyenlőre RAM-ban */

typedef struct {
    char name[FS_MAX_NAME];
    char data[FS_MAX_SIZE];
    int  size;
    int  used;
} ramfile_t;

extern ramfile_t fs[FS_MAX_FILES];

ramfile_t* fs_find(const char* name);
ramfile_t* fs_create(const char* name);
void       fs_delete(const char* name);

#endif

#include "../include/fs.h"
#include "../include/string.h"
#include "../include/ata.h"

ramfile_t fs[FS_MAX_FILES];

/* XDisk (XD-x32) könyvtárszerkezet leíró tábla (LBA 100-101) */
typedef struct {
    char magic[8];     /* "XD-x32\0\0" */
    struct {
        char name[FS_MAX_NAME];
        int size;
        int used;
    } entries[FS_MAX_FILES];
    char padding[376]; /* Padding to make it exactly 1024 bytes (2 sectors) */
} xdisk_dir_t;

/* Segédfüggvények a szektor buffer (szavak) és bájt buffer konverzióhoz */
static void copy_words_to_bytes(void* dest, const uint16_t* src, int byte_count) {
    uint8_t* d = (uint8_t*)dest;
    int word_count = byte_count / 2;
    for (int i = 0; i < word_count; i++) {
        d[i*2] = src[i] & 0xFF;
        d[i*2+1] = (src[i] >> 8) & 0xFF;
    }
}

static void copy_bytes_to_words(uint16_t* dest, const void* src, int byte_count) {
    const uint8_t* s = (const uint8_t*)src;
    int word_count = byte_count / 2;
    for (int i = 0; i < word_count; i++) {
        dest[i] = s[i*2] | (s[i*2+1] << 8);
    }
}

void fs_init(void) {
    uint16_t buf[256];
    xdisk_dir_t dir;

    /* Könyvtár tábla beolvasása (két szektor, 1024 bájt) */
    ata_read_sector(XDISK_LBA_DIRECTORY, buf);
    copy_words_to_bytes(&dir, buf, 512);
    ata_read_sector(XDISK_LBA_DIRECTORY + 1, buf);
    copy_words_to_bytes(((uint8_t*)&dir) + 512, buf, 512);

    /* Mágikus azonosító ellenőrzése */
    int magic_ok = 1;
    const char* magic = XDISK_MAGIC;
    for (int i = 0; i < 6; i++) {
        if (dir.magic[i] != magic[i]) {
            magic_ok = 0;
            break;
        }
    }

    if (magic_ok) {
        /* Betöltjük a fájlokat a memóriába */
        for (int i = 0; i < FS_MAX_FILES; i++) {
            fs[i].used = dir.entries[i].used;
            fs[i].size = dir.entries[i].size;
            kstrcpy(fs[i].name, dir.entries[i].name);

            if (fs[i].used && fs[i].size > 0) {
                int sector_count = (fs[i].size + 511) / 512;
                if (sector_count > 8) sector_count = 8;

                uint32_t file_start_lba = XDISK_LBA_DATA_START + i * 8;
                for (int s = 0; s < sector_count; s++) {
                    ata_read_sector(file_start_lba + s, buf);
                    copy_words_to_bytes(fs[i].data + s * 512, buf, 512);
                }
            }
        }
    } else {
        /* Nincs inicializálva a lemez: Formázás */
        kmemset(&dir, 0, sizeof(xdisk_dir_t));
        kstrcpy(dir.magic, XDISK_MAGIC);

        kmemset(buf, 0, 512);
        copy_bytes_to_words(buf, &dir, 512);
        ata_write_sector(XDISK_LBA_DIRECTORY, buf);

        kmemset(buf, 0, 512);
        copy_bytes_to_words(buf, ((uint8_t*)&dir) + 512, 512);
        ata_write_sector(XDISK_LBA_DIRECTORY + 1, buf);

        kmemset(fs, 0, sizeof(fs));
    }
}

void fs_sync(void) {
    uint16_t buf[256];
    xdisk_dir_t dir;

    kmemset(&dir, 0, sizeof(xdisk_dir_t));
    kstrcpy(dir.magic, XDISK_MAGIC);

    for (int i = 0; i < FS_MAX_FILES; i++) {
        dir.entries[i].used = fs[i].used;
        dir.entries[i].size = fs[i].size;
        kstrcpy(dir.entries[i].name, fs[i].name);

        if (fs[i].used && fs[i].size > 0) {
            int sector_count = (fs[i].size + 511) / 512;
            if (sector_count > 8) sector_count = 8;

            uint32_t file_start_lba = XDISK_LBA_DATA_START + i * 8;
            for (int s = 0; s < sector_count; s++) {
                kmemset(buf, 0, 512);
                copy_bytes_to_words(buf, fs[i].data + s * 512, 512);
                ata_write_sector(file_start_lba + s, buf);
            }
        }
    }

    kmemset(buf, 0, 512);
    copy_bytes_to_words(buf, &dir, 512);
    ata_write_sector(XDISK_LBA_DIRECTORY, buf);

    kmemset(buf, 0, 512);
    copy_bytes_to_words(buf, ((uint8_t*)&dir) + 512, 512);
    ata_write_sector(XDISK_LBA_DIRECTORY + 1, buf);
}

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
            fs_sync();
            return &fs[i];
        }
    }
    return 0;
}

void fs_delete(const char* name) {
    ramfile_t* f = fs_find(name);
    if (f) {
        kmemset(f, 0, sizeof(ramfile_t));
        fs_sync();
    }
}
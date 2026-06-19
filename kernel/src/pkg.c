#include "../include/pkg.h"
#include "../include/vga.h"
#include "../include/string.h"
#include "../include/fs.h"
#include "../include/net.h"

typedef struct {
    const char* name;
    const char* desc;
} pkg_entry_t;

static const pkg_entry_t known_packages[] = {
    {"net",    "Halozati driver (RTL8139 Ethernet)"},
    {"ata",    "ATA/IDE lemez driver"},
    {"shell",  "Alaprendszer parancsertelmezo"},
    {"editor", "Szovegszerkeszto (nano-stilus)"},
};
#define KNOWN_PACKAGE_COUNT (int)(sizeof(known_packages) / sizeof(known_packages[0]))

static void build_marker_name(const char* pkgname, char* out) {
    int i = 0;
    while (pkgname[i] && i < FS_MAX_NAME - 5) { out[i] = pkgname[i]; i++; }
    out[i++] = '.'; out[i++] = 'p'; out[i++] = 'k'; out[i++] = 'g';
    out[i] = '\0';
}

void pkg_install(const char* name) {
    if (kstrlen(name) == 0) {
        kprint_color("Hasznalat: XD install <csomagnev>\n", C_ERROR);
        return;
    }

    const pkg_entry_t* found = 0;
    for (int i = 0; i < KNOWN_PACKAGE_COUNT; i++) {
        if (kstrcmp(known_packages[i].name, name) == 0) {
            found = &known_packages[i];
            break;
        }
    }

    if (!found) {
        kprint_color("Ismeretlen csomag: ", C_ERROR);
        kprint(name);
        kprint("\n");
        kprint("Tipd: XD list - elerheto csomagok megtekintesehez\n");
        return;
    }

    char marker[FS_MAX_NAME];
    build_marker_name(found->name, marker);

    if (fs_find(marker)) {
        kprint_color("Mar telepitve: ", C_INFO);
        kprint(name);
        kprint("\n");
        return;
    }

    kprint_color("Telepites: ", C_INFO);
    kprint(name);
    kprint(" - ");
    kprint(found->desc);
    kprint("\n");

    /* Driver-specifikus inicializálás, ha van ilyen a csomaghoz */
    if (kstrcmp(found->name, "net") == 0) {
        if (net_available()) {
            kprint_color("  Halozati kartya mar aktiv.\n", C_PROMPT);
        } else if (net_init()) {
            kprint_color("  RTL8139 inicializalva.\n", C_PROMPT);
        } else {
            kprint_color("  Figyelmeztetes: nem talalhato RTL8139 kartya.\n", C_ERROR);
        }
    }

    ramfile_t* f = fs_create(marker);
    if (f) {
        const char* content = found->desc;
        int len = kstrlen(content);
        for (int k = 0; k < len; k++) f->data[k] = content[k];
        f->size = len;
    }

    kprint_color("Kesz.\n", C_INFO);
}

void pkg_list(void) {
    kprint_color("Elerheto csomagok:\n", C_INFO);
    for (int i = 0; i < KNOWN_PACKAGE_COUNT; i++) {
        char marker[FS_MAX_NAME];
        build_marker_name(known_packages[i].name, marker);

        kprint("  ");
        kprint(known_packages[i].name);
        kprint(fs_find(marker) ? " [telepitve] - " : " [nincs telepitve] - ");
        kprint(known_packages[i].desc);
        kprint("\n");
    }
}
#ifndef PKG_H
#define PKG_H

/*
 * Egyszerű "csomagkezelő" a XD install parancshoz.
 * Egyenlőre beépített (driver-szintű) csomagokat tud "telepíteni":
 * létrehoz egy info fájlt a RAM filesystembe, és inicializálja
 * a hozzá tartozó drivert, ha van ilyen.
 *
 * Később ide jöhet: igazi letöltés hálózaton (net.c) keresztül,
 * majd kiírás az XDisk-re (fs_sync).
 */

/* XD install <name> parancs végrehajtása. Kiírja az eredményt. */
void pkg_install(const char* name);

/* XD list - elérhető/telepített csomagok listázása */
void pkg_list(void);

#endif
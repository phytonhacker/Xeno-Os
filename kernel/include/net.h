#ifndef NET_H
#define NET_H

#include "types.h"

#define NET_MTU         1518   /* Max Ethernet frame méret */
#define NET_MAC_LEN     6
#define NET_HEADER_LEN  14     /* Ethernet header: 6+6+2 bájt */

/* Hálózati meghajtó inicializálása (RTL8139 keresés + reset + konfig) */
int  net_init(void);

/* Visszaadja: 1 ha a hálózati kártya megtalálható és inicializált, 0 ha nem */
int  net_available(void);

/* MAC cím lekérdezése (6 bájtos tömb) */
void net_get_mac(uint8_t mac[NET_MAC_LEN]);

/* Ethernet frame küldése (nyers adat, src MAC-ot a driver tölti be) */
int  net_send(const uint8_t* data, uint16_t len);

/* Polling: visszaad egy érkező csomagot a bufferbe.
 * Visszatérési érték: fogadott bájtok száma, 0 ha nincs csomag. */
uint16_t net_poll(uint8_t* buf, uint16_t bufsize);

#endif

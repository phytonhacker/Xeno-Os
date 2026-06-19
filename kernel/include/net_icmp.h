#ifndef NET_ICMP_H
#define NET_ICMP_H

#include "types.h"

/*
 * Minimális hálózati protokoll-stack a ping parancshoz.
 * Réteg: Ethernet (net.c) -> ARP / IPv4 -> ICMP
 *
 * Csak a ping működéséhez szükséges minimumot tartalmazza:
 *  - ARP request/reply (cél MAC cím feloldása)
 *  - IPv4 header építés/beolvasás (fragmentáció nélkül)
 *  - ICMP echo request küldés, echo reply fogadás
 */

/* Beállítja a kernel saját IP-jét (pl. DHCP helyett statikus cím).
 * ip[4] = pl. {10,0,2,15} - QEMU user-mode hálózat alapértelmezett címe */
void net_icmp_set_local_ip(const uint8_t ip[4]);
void net_icmp_get_local_ip(uint8_t out[4]);

/*
 * Ping egy IPv4 címre.
 * ip[4]      - cél IP cím, pl. {8,8,8,8}
 * timeout_iters - hány polling ciklusig várjon válaszra
 * Visszatérési érték:
 *    1  = válasz érkezett (rtt_out-ba egy durva "kör" szám kerül)
 *    0  = timeout, nincs válasz
 *   -1  = nincs hálózati kártya
 *   -2  = ARP feloldás sikertelen (cél nem válaszolt MAC-re)
 */
int net_icmp_ping(const uint8_t ip[4], int timeout_iters, int* rtt_out);

#endif
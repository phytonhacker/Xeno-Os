#include "../include/net_icmp.h"
#include "../include/net.h"
#include "../include/string.h"

/* ============================================================
 * Saját IP cím (statikus, később DHCP válthatja ki)
 * QEMU "-net user" alapértelmezett hálózatán a vendég (mi) alapból
 * 10.0.2.15-öt szokott kapni DHCP-vel - egyenlőre kézzel ezt állítjuk.
 * ============================================================ */
static uint8_t local_ip[4] = {10, 0, 2, 15};

void net_icmp_set_local_ip(const uint8_t ip[4]) {
    for (int i = 0; i < 4; i++) local_ip[i] = ip[i];
}
void net_icmp_get_local_ip(uint8_t out[4]) {
    for (int i = 0; i < 4; i++) out[i] = local_ip[i];
}

/* ============================================================
 * Byte order segéd (hálózati protokollok big-endian-ek)
 * ============================================================ */
static uint16_t htons16(uint16_t v) {
    return (uint16_t)((v << 8) | (v >> 8));
}

/* ============================================================
 * Ethernet header építés
 * ============================================================ */
#define ETH_TYPE_ARP  0x0806
#define ETH_TYPE_IPV4 0x0800

static void eth_build(uint8_t* frame, const uint8_t dst_mac[6],
                       const uint8_t src_mac[6], uint16_t ethertype) {
    for (int i = 0; i < 6; i++) frame[i]     = dst_mac[i];
    for (int i = 0; i < 6; i++) frame[6 + i] = src_mac[i];
    uint16_t et = htons16(ethertype);
    frame[12] = (uint8_t)(et & 0xFF);
    frame[13] = (uint8_t)(et >> 8);
}

/* ============================================================
 * ARP - cél IP -> MAC cím feloldás
 * ============================================================ */
#define ARP_HW_ETHERNET 1
#define ARP_PROTO_IPV4  0x0800
#define ARP_OP_REQUEST  1
#define ARP_OP_REPLY    2

static uint8_t broadcast_mac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

/* ARP gyorsítótár - csak 1 bejegyzés, elég a ping-hez */
static uint8_t arp_cache_ip[4];
static uint8_t arp_cache_mac[6];
static int     arp_cache_valid = 0;

static void arp_send_request(const uint8_t target_ip[4]) {
    uint8_t frame[42];
    uint8_t my_mac[6];
    net_get_mac(my_mac);

    eth_build(frame, broadcast_mac, my_mac, ETH_TYPE_ARP);

    uint8_t* arp = frame + 14;
    arp[0] = 0x00; arp[1] = ARP_HW_ETHERNET;
    arp[2] = (uint8_t)(ARP_PROTO_IPV4 >> 8);
    arp[3] = (uint8_t)(ARP_PROTO_IPV4 & 0xFF);
    arp[4] = 6;
    arp[5] = 4;
    arp[6] = 0x00; arp[7] = ARP_OP_REQUEST;

    for (int i = 0; i < 6; i++) arp[8 + i]  = my_mac[i];
    for (int i = 0; i < 4; i++) arp[14 + i] = local_ip[i];
    for (int i = 0; i < 6; i++) arp[18 + i] = 0;
    for (int i = 0; i < 4; i++) arp[24 + i] = target_ip[i];

    net_send(frame, 42);
}

static void arp_handle_incoming(const uint8_t* pkt, uint16_t len) {
    if (len < 14 + 28) return;
    uint16_t ethertype = (uint16_t)((pkt[12] << 8) | pkt[13]);
    if (ethertype != ETH_TYPE_ARP) return;

    const uint8_t* arp = pkt + 14;
    uint16_t opcode = (uint16_t)((arp[6] << 8) | arp[7]);
    if (opcode != ARP_OP_REPLY) return;

    for (int i = 0; i < 4; i++) arp_cache_ip[i]  = arp[14 + i];
    for (int i = 0; i < 6; i++) arp_cache_mac[i] = arp[8 + i];
    arp_cache_valid = 1;
}

static int arp_resolve(const uint8_t target_ip[4], uint8_t out_mac[6], int timeout_iters) {
    if (arp_cache_valid &&
        arp_cache_ip[0]==target_ip[0] && arp_cache_ip[1]==target_ip[1] &&
        arp_cache_ip[2]==target_ip[2] && arp_cache_ip[3]==target_ip[3]) {
        for (int i = 0; i < 6; i++) out_mac[i] = arp_cache_mac[i];
        return 1;
    }

    arp_cache_valid = 0;
    arp_send_request(target_ip);

    uint8_t buf[NET_MTU];
    for (int i = 0; i < timeout_iters; i++) {
        uint16_t len = net_poll(buf, NET_MTU);
        if (len > 0) arp_handle_incoming(buf, len);
        if (arp_cache_valid &&
            arp_cache_ip[0]==target_ip[0] && arp_cache_ip[1]==target_ip[1] &&
            arp_cache_ip[2]==target_ip[2] && arp_cache_ip[3]==target_ip[3]) {
            for (int j = 0; j < 6; j++) out_mac[j] = arp_cache_mac[j];
            return 1;
        }
    }
    return 0;
}

/* ============================================================
 * IPv4 header
 * ============================================================ */
#define IP_PROTO_ICMP 1

static uint16_t ip_checksum(const uint8_t* data, int len) {
    uint32_t sum = 0;
    for (int i = 0; i + 1 < len; i += 2)
        sum += (uint16_t)((data[i] << 8) | data[i + 1]);
    if (len & 1) sum += (uint16_t)(data[len - 1] << 8);
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)(~sum & 0xFFFF);
}

static void ip_build_header(uint8_t* ip, uint16_t payload_len,
                             const uint8_t src[4], const uint8_t dst[4],
                             uint8_t proto, uint16_t ident) {
    ip[0] = 0x45;
    ip[1] = 0x00;
    uint16_t total_len = (uint16_t)(20 + payload_len);
    ip[2] = (uint8_t)(total_len >> 8);
    ip[3] = (uint8_t)(total_len & 0xFF);
    ip[4] = (uint8_t)(ident >> 8);
    ip[5] = (uint8_t)(ident & 0xFF);
    ip[6] = 0x00; ip[7] = 0x00;
    ip[8] = 64;
    ip[9] = proto;
    ip[10] = 0; ip[11] = 0;
    for (int i = 0; i < 4; i++) ip[12 + i] = src[i];
    for (int i = 0; i < 4; i++) ip[16 + i] = dst[i];

    uint16_t csum = ip_checksum(ip, 20);
    ip[10] = (uint8_t)(csum >> 8);
    ip[11] = (uint8_t)(csum & 0xFF);
}

/* ============================================================
 * ICMP echo request / reply
 * ============================================================ */
#define ICMP_TYPE_ECHO_REQUEST 8
#define ICMP_TYPE_ECHO_REPLY   0

int net_icmp_ping(const uint8_t ip[4], int timeout_iters, int* rtt_out) {
    if (!net_available()) return -1;

    uint8_t dst_mac[6];
    if (!arp_resolve(ip, dst_mac, timeout_iters)) return -2;

    uint8_t frame[14 + 20 + 8];
    uint8_t my_mac[6];
    net_get_mac(my_mac);

    eth_build(frame, dst_mac, my_mac, ETH_TYPE_IPV4);

    uint8_t* ip_hdr = frame + 14;
    static uint16_t ip_ident = 1;
    ip_build_header(ip_hdr, 8, local_ip, ip, IP_PROTO_ICMP, ip_ident++);

    uint8_t* icmp = frame + 14 + 20;
    icmp[0] = ICMP_TYPE_ECHO_REQUEST;
    icmp[1] = 0;
    icmp[2] = 0; icmp[3] = 0;
    icmp[4] = 0x00; icmp[5] = 0x01;
    static uint16_t seq = 0;
    seq++;
    icmp[6] = (uint8_t)(seq >> 8);
    icmp[7] = (uint8_t)(seq & 0xFF);

    uint16_t icmp_csum = ip_checksum(icmp, 8);
    icmp[2] = (uint8_t)(icmp_csum >> 8);
    icmp[3] = (uint8_t)(icmp_csum & 0xFF);

    net_send(frame, sizeof(frame));

    uint8_t buf[NET_MTU];
    int rounds = 0;
    for (int i = 0; i < timeout_iters; i++) {
        uint16_t len = net_poll(buf, NET_MTU);
        if (len == 0) continue;

        rounds++;

        uint16_t ethertype = (uint16_t)((buf[12] << 8) | buf[13]);
        if (ethertype == ETH_TYPE_ARP) { arp_handle_incoming(buf, len); continue; }
        if (ethertype != ETH_TYPE_IPV4) continue;
        if (len < 14 + 20 + 8) continue;

        uint8_t* rip = buf + 14;
        if (rip[9] != IP_PROTO_ICMP) continue;

        if (rip[12]!=ip[0]||rip[13]!=ip[1]||rip[14]!=ip[2]||rip[15]!=ip[3]) continue;

        uint8_t* ricmp = rip + 20;
        if (ricmp[0] == ICMP_TYPE_ECHO_REPLY) {
            if (rtt_out) *rtt_out = rounds;
            return 1;
        }
    }

    return 0;
}
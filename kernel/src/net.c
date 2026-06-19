#include "../include/net.h"
#include "../include/pci.h"
#include "../include/io.h"

/* RTL8139 Vendor/Device ID-k */
#define RTL8139_VENDOR_ID  0x10EC
#define RTL8139_DEVICE_ID  0x8139

/* RTL8139 I/O regiszter offszetek */
#define RTL_MAC0           0x00   /* MAC cím (6 bájt) */
#define RTL_CMD            0x37   /* Command Register */
#define RTL_IMR            0x3C   /* Interrupt Mask Register */
#define RTL_ISR            0x3E   /* Interrupt Status Register */
#define RTL_RCR            0x44   /* Receive Configuration Register */
#define RTL_TCR            0x40   /* Transmit Configuration Register */
#define RTL_RBSTART        0x30   /* RX Buffer Start Address */
#define RTL_CAPR           0x38   /* Current Address of Packet Read */
#define RTL_CBR            0x3A   /* Current Buffer Address */

/* TX regiszterek (4 TX slot) */
#define RTL_TSAD0          0x20   /* TX Status Address 0 */
#define RTL_TSD0           0x10   /* TX Status/Control 0 */

/* Command register bitek */
#define RTL_CMD_RST        0x10   /* Reset */
#define RTL_CMD_RE         0x08   /* Receiver Enable */
#define RTL_CMD_TE         0x04   /* Transmitter Enable */

/* Interrupt State Register bitek */
#define RTL_ISR_ROK        0x01   /* Receive OK */
#define RTL_ISR_TOK        0x04   /* Transmit OK */

/* Receive Configuration */
#define RTL_RCR_AAP        (1 << 0)  /* All packets */
#define RTL_RCR_APM        (1 << 1)  /* Physical match */
#define RTL_RCR_AM         (1 << 2)  /* Multicast */
#define RTL_RCR_AB         (1 << 3)  /* Broadcast */
#define RTL_RCR_WRAP       (1 << 7)  /* Wrap mode */
#define RTL_RCR_RBLEN_32K  (1 << 11) /* 32 KB RX buffer */

/* ============================================================
 * Belső állapot
 * ============================================================ */

/* RX ring buffer: 32 KB + 1.5 KB wrap guard */
#define RX_BUF_SIZE   (32*1024 + 1500 + 4)
/* TX bufferek: 4 slot, egyenként max 1536 bájt */
#define TX_BUF_SIZE   1536
#define TX_SLOTS      4

/* Statikus bufferek (BSS szegmensben — a linker 4KB-ra igazítja) */
static uint8_t  rx_buf[RX_BUF_SIZE] __attribute__((aligned(4)));
static uint8_t  tx_buf[TX_SLOTS][TX_BUF_SIZE] __attribute__((aligned(4)));

static uint16_t rtl_iobase = 0;   /* A kártya I/O báziscíme */
static uint8_t  mac[NET_MAC_LEN]; /* A kártya MAC-címe */
static int      tx_slot  = 0;     /* Következő TX buffer slot */
static uint16_t rx_read  = 0;     /* RX olvasási pozíció */
static int      initialized = 0;

/* ============================================================
 * Belső állapot
 * ============================================================ */

int net_available(void) {
    return initialized;
}

void net_get_mac(uint8_t out[NET_MAC_LEN]) {
    for (int i = 0; i < NET_MAC_LEN; i++) out[i] = mac[i];
}

int net_init(void) {
    /* 1. RTL8139 megkeresése a PCI buszon */
    rtl_iobase = pci_find_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
    if (rtl_iobase == 0) return 0; /* Nincs hálózati kártya */

    /* 2. Szoftver reset */
    outb(rtl_iobase + RTL_CMD, RTL_CMD_RST);
    while (inb(rtl_iobase + RTL_CMD) & RTL_CMD_RST); /* Várjuk, míg befejezi */

    /* 3. MAC cím kiolvasása */
    for (int i = 0; i < NET_MAC_LEN; i++)
        mac[i] = inb(rtl_iobase + RTL_MAC0 + i);

    /* 4. RX buffer cím beállítása */
    outl(rtl_iobase + RTL_RBSTART, (uint32_t)(uintptr_t)rx_buf);

    /* 5. TX buffer címek beállítása */
    for (int i = 0; i < TX_SLOTS; i++)
        outl(rtl_iobase + RTL_TSAD0 + i * 4, (uint32_t)(uintptr_t)tx_buf[i]);

    /* 6. IMR: ROK + TOK megszakítás engedélyezése (polling módban nem kell,
          de a hardware elvárja a beállítást) */
    outb(rtl_iobase + RTL_IMR, RTL_ISR_ROK | RTL_ISR_TOK);

    /* 7. Receive Configuration: minden csomag, 32 KB ring, wrap */
    outl(rtl_iobase + RTL_RCR,
         RTL_RCR_AAP | RTL_RCR_APM | RTL_RCR_AM | RTL_RCR_AB |
         RTL_RCR_WRAP | RTL_RCR_RBLEN_32K);

    /* 8. TX Configuration: standard */
    outl(rtl_iobase + RTL_TCR, 0x03000700);

    /* 9. RE + TE engedélyezése */
    outb(rtl_iobase + RTL_CMD, RTL_CMD_RE | RTL_CMD_TE);

    rx_read = 0;
    tx_slot = 0;
    initialized = 1;
    return 1;
}

int net_send(const uint8_t* data, uint16_t len) {
    if (!initialized || len == 0 || len > TX_BUF_SIZE) return 0;

    /* Másoljuk az adatot a TX bufferbe */
    uint8_t* tbuf = tx_buf[tx_slot];
    for (uint16_t i = 0; i < len; i++) tbuf[i] = data[i];

    /* TX Status Register: állítsuk be a méretet és indítsuk el a küldést */
    uint16_t tsd_reg = (uint16_t)(RTL_TSD0 + tx_slot * 4);
    outl(rtl_iobase + tsd_reg, (uint32_t)len);

    /* Várjuk, míg a kártya elküldi (TOK bit a TSD-ben) */
    while (!(inl(rtl_iobase + tsd_reg) & 0x8000)); /* TSD TOK bit */

    /* Következő TX slot (körkörösen) */
    tx_slot = (tx_slot + 1) % TX_SLOTS;
    return 1;
}

uint16_t net_poll(uint8_t* buf, uint16_t bufsize) {
    if (!initialized) return 0;

    /* Ha nincs érkező csomag (ROK bit az ISR-ben nincs beállítva) */
    uint16_t isr = inb(rtl_iobase + RTL_ISR) |
                   ((uint16_t)inb(rtl_iobase + RTL_ISR + 1) << 8);
    if (!(isr & RTL_ISR_ROK)) return 0;

    /* RX csomag header: 4 bájt (status word + hossz szó) */
    uint8_t* pkt = rx_buf + rx_read;
    uint16_t pkt_status = (uint16_t)(pkt[0] | (pkt[1] << 8));
    uint16_t pkt_len    = (uint16_t)(pkt[2] | (pkt[3] << 8));

    /* Ellenőrzés: ROK bit a csomag status szóban */
    if (!(pkt_status & 0x01) || pkt_len < NET_HEADER_LEN || pkt_len > NET_MTU) {
        /* Hibás csomag: ISR törlés és pointer reset */
        outb(rtl_iobase + RTL_ISR, RTL_ISR_ROK);
        return 0;
    }

    /* Csomag másolása a felhasználói bufferbe (4 bájtos header kihagyva) */
    uint16_t copy_len = pkt_len;
    if (copy_len > bufsize) copy_len = bufsize;
    uint8_t* payload = pkt + 4;
    for (uint16_t i = 0; i < copy_len; i++) buf[i] = payload[i];

    /* RX olvasási pozíció frissítése (DWORD-igazítás, ring-ként) */
    rx_read = (uint16_t)((rx_read + pkt_len + 4 + 3) & ~3);
    rx_read %= (32 * 1024);

    /* CAPR regiszter frissítése (rx_read - 16 a spec szerint) */
    uint16_t capr = (uint16_t)((rx_read - 16) % (32 * 1024));
    outb(rtl_iobase + RTL_CAPR,     (uint8_t)(capr & 0xFF));
    outb(rtl_iobase + RTL_CAPR + 1, (uint8_t)(capr >> 8));

    /* ISR ROK törlése */
    outb(rtl_iobase + RTL_ISR, RTL_ISR_ROK);

    return copy_len;
}

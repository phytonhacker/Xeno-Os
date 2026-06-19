#include "../include/ata.h"
#include "../include/io.h"

#define ATA_PORT_DATA          0x1F0
#define ATA_PORT_ERROR         0x1F1
#define ATA_PORT_SECTOR_COUNT  0x1F2
#define ATA_PORT_LBA_LOW       0x1F3
#define ATA_PORT_LBA_MID       0x1F4
#define ATA_PORT_LBA_HIGH      0x1F5
#define ATA_PORT_DRIVE_HEAD    0x1F6
#define ATA_PORT_STATUS        0x1F7
#define ATA_PORT_COMMAND       0x1F7

#define ATA_CMD_READ_PIO       0x20
#define ATA_CMD_WRITE_PIO      0x30

/*
 * Timeout-os várakozás: ha a lemez nem válaszol (pl. nincs csatlakoztatva
 * megfelelően QEMU-ban), ne fagyjon le örökre a kernel. A timeout érték
 * tisztán "iterációszám" alapú (nincs még valós időzítőnk a kernelben).
 */
#define ATA_TIMEOUT_ITERS 1000000

/* Visszatérés: 1 = sikeres, 0 = timeout (lemez nem válaszolt) */
static int ata_wait_ready(void) {
    int timeout = ATA_TIMEOUT_ITERS;
    while ((inb(ATA_PORT_STATUS) & 0x80) != 0) { if (--timeout <= 0) return 0; }
    timeout = ATA_TIMEOUT_ITERS;
    while ((inb(ATA_PORT_STATUS) & 0x08) == 0) { if (--timeout <= 0) return 0; }
    return 1;
}

static int ata_wait_bsy(void) {
    int timeout = ATA_TIMEOUT_ITERS;
    while ((inb(ATA_PORT_STATUS) & 0x80) != 0) { if (--timeout <= 0) return 0; }
    return 1;
}

void ata_read_sector(uint32_t lba, uint16_t* buffer) {
    if (!ata_wait_bsy()) { for (int i = 0; i < 256; i++) buffer[i] = 0; return; }
    outb(ATA_PORT_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PORT_SECTOR_COUNT, 1);
    outb(ATA_PORT_LBA_LOW, (uint8_t)lba);
    outb(ATA_PORT_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PORT_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_PORT_COMMAND, ATA_CMD_READ_PIO);

    if (!ata_wait_ready()) { for (int i = 0; i < 256; i++) buffer[i] = 0; return; }

    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(ATA_PORT_DATA);
    }
}

void ata_write_sector(uint32_t lba, const uint16_t* buffer) {
    if (!ata_wait_bsy()) return;
    outb(ATA_PORT_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PORT_SECTOR_COUNT, 1);
    outb(ATA_PORT_LBA_LOW, (uint8_t)lba);
    outb(ATA_PORT_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PORT_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_PORT_COMMAND, ATA_CMD_WRITE_PIO);

    if (!ata_wait_ready()) return;

    for (int i = 0; i < 256; i++) {
        outw(ATA_PORT_DATA, buffer[i]);
    }
}
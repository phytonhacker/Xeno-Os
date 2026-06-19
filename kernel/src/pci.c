#include "../include/pci.h"
#include "../include/io.h"

#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

/* PCI konfigurációs register beolvasása */
uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address =
        (1u << 31)              |
        ((uint32_t)bus  << 16)  |
        ((uint32_t)slot << 11)  |
        ((uint32_t)func <<  8)  |
        (offset & 0xFC);

    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

/* Végigmegyünk a PCI buszon (bus 0, slot 0-31) és megkeressük
 * a megadott Vendor+Device ID-jú eszközt.
 * Visszaadja az eszköz BAR0 I/O báziscímét (alsó bitek maszkolt). */
uint16_t pci_find_device(uint16_t vendor_id, uint16_t device_id) {
    for (uint8_t slot = 0; slot < 32; slot++) {
        uint32_t id  = pci_read(0, slot, 0, 0x00);
        uint16_t vid = (uint16_t)(id & 0xFFFF);
        uint16_t did = (uint16_t)(id >> 16);

        if (vid == vendor_id && did == device_id) {
            /* BAR0 = Base Address Register 0 (offset 0x10) */
            uint32_t bar0 = pci_read(0, slot, 0, 0x10);
            /* I/O bar: bit0=1, báziscím a felsőbb bitek */
            if (bar0 & 0x1)
                return (uint16_t)(bar0 & 0xFFFC);
        }
    }
    return 0; /* Nem található */
}

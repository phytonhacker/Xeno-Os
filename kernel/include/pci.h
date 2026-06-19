#ifndef PCI_H
#define PCI_H

#include "types.h"

/* PCI konfigurációs tér olvasás */
uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

/* Adott Vendor+Device ID-jú kártya megkeresése a PCI buszon.
 * Visszaadja az I/O báziscímet, vagy 0-t ha nem találja. */
uint16_t pci_find_device(uint16_t vendor_id, uint16_t device_id);

#endif

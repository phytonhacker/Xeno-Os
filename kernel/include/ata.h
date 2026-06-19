#ifndef ATA_H
#define ATA_H

#include "types.h"

/* Olvas egy 512 bájtos szektort az LBA címről a bufferbe */
void ata_read_sector(uint32_t lba, uint16_t* buffer);

/* Ír egy 512 bájtos szektort az LBA címről a bufferből */
void ata_write_sector(uint32_t lba, const uint16_t* buffer);

#endif

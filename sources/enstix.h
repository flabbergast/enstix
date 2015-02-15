/*
 * enstix.h
 * (c) 2015 flabbergast
 *  mainly callbacks that should be implemented in the main code, and some
 *  global variables
 */

#ifndef _ENSTIX_H_
#define _ENSTIX_H_

/* "CALLBACK_disk_readSector" is called internally by the USB/SCSI stack.
 * Its purpose is to:
 * (1) read a sector "sectorNumber" (starting at 0),
 * (2) put the secotors content into "out_sectordata"
 * (3) return the number of bytes read - usually 512 (DISK_BLOCK_SIZE).
 */
int16_t CALLBACK_disk_readSector(uint8_t out_sectordata[DISK_BLOCK_SIZE], const uint32_t sectorNumber);

/* "CALLBACK_disk_writeSector" is called internally by the USB/SCSI stack.
 * Its purpose is to:
 * (1) put the content "in_sectordata" on the media at sector "sectorNumber"
 * (2) return the number of bytes written - usually 512 (DISK_BLOCK_SIZE)
 */
int16_t CALLBACK_disk_writeSector(uint8_t in_sectordata[DISK_BLOCK_SIZE], const uint32_t sectorNumber);

#endif

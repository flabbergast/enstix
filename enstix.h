/*
 * enstix.h
 * (c) 2014 flabbergast
 *  mainly callbacks that should be implemented in the main code, and some
 *  global variables
 */

#ifndef _ENSTIX_H_
#define _ENSTIX_H_

/** global variables that can be used in the main code */

// Is the virtual disk read only? Should Detach/Attach the USB device after changing,
//   since the OS can still think otherwise
volatile bool disk_read_only;

// Size of the virtual disk to be reported to OS - in BLOCKS!
volatile uint32_t disk_size;

// Initial state or encrypting? (should detach/attach USB after changing)
#define DISK_STATE_INITIAL 1
#define DISK_STATE_ENCRYPTING 2
volatile uint8_t disk_state;

// Button state (milliseconds pressed)
volatile uint32_t button_pressed_ms;

/* "CALLBACK_disk_readSector" is called internally by the USB/SCSI stack.
 * Its purpose is to:
 * (1) read a sector "sectorNumber" (starting at 0),
 * (2) put the secotors content into "out_sectordata"
 * (3) return the number of bytes read - usually 512 (VIRTUAL_DISK_BLOCK_SIZE).
 */
int16_t CALLBACK_disk_readSector(uint8_t out_sectordata[VIRTUAL_DISK_BLOCK_SIZE], const uint32_t sectorNumber);

/* "CALLBACK_disk_writeSector" is called internally by the USB/SCSI stack.
 * Its purpose is to:
 * (1) put the content "in_sectordata" on the media at sector "sectorNumber"
 * (2) return the number of bytes written - usually 512 (VIRTUAL_DISK_BLOCK_SIZE)
 */
int16_t CALLBACK_disk_writeSector(uint8_t in_sectordata[VIRTUAL_DISK_BLOCK_SIZE], const uint32_t sectorNumber);

#endif

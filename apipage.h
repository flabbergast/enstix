/*
 * APIPAGE.H
 * This file contains interfaces to access and use the
 * BLS API page, usually located in the last page of
 * the flash memory.
 *
 * This is a public released file for specific use
 * with avrstick ATxmega128a3u model.
 * The magic internally used is "0xfdffff80".
 *
 * THIS FILE DOES NOT IMPLEMENT STUFF FOR THE PAGE
 * ITSELF. IT JUST INTERFACES EXISTING BOOTLOADERS.
 *
 * WARNING:
 * BE CAREFUL NOT TO ACCIDENTALLY OVERWRITE THE BOOT-
 * LOADER! BY DEFAULT THERE IS NO CHECKING PERFORMED!
 * USE "__reportBLSpagesize" TO INFORM YOURSELF ABOUT
 * THE NUMBER OF PAGES THE BOOTLOADER OCCUPIES.
 *
 *
 * This is version 20140710T2100ZSB
 *
 * Stephan Baerwolf (matrixstorm@gmx.de), Ilmenau 2014
 */

#ifndef __APIPAGE_H_6e4723fa71ec40528d1ab16d9ff806d7
#define __APIPAGE_H_6e4723fa71ec40528d1ab16d9ff806d7 1

#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>

/* platform identification - can be defined externally */
#ifndef USE_SPMINTEREFACE_MAGICVALUE
#	define USE_SPMINTEREFACE_MAGICVALUE	(0xFDFFFF80)
#endif

/* general addressing makros */
#define APIPAGE_PAGEBYTEADDRESS			(BOOT_SECTION_START+(BOOT_SECTION_SIZE-BOOT_SECTION_PAGE_SIZE))
#define APIPAGE_PAGEBYTESIZE			(BOOT_SECTION_PAGE_SIZE)

/* for bootloaders using his */
#ifdef APIPAGEADDR
#	if (APIPAGEADDR != APIPAGE_PAGEBYTEADDRESS)
#		error APIPAGE located on wrong position in flash!
#	else
#		include "Config/auxsection.h"
#		define APIPAGEAUXSECTION	AUXSECTION6
#	endif
#endif

#ifndef APIPAGEAUXSECTION
#	define APIPAGEAUXSECTION
#endif

/* mainly for internal use: */
#define APIPAGE_TOTALADDRESSSPACEBYTESIZE_EXEC		(PROGMEM_SIZE)

/* specific API functions */
/* WARNING: these are flash locations NOT function addresses! */
#define APIPAGE__bootloader__do_spm_magicblock		(APIPAGE_PAGEBYTEADDRESS+0x00)
#define APIPAGE__bootloader__do_spm_magic		(APIPAGE_PAGEBYTEADDRESS+0x04)
#define APIPAGE__bootloader__do_spm			(APIPAGE_PAGEBYTEADDRESS+0x08)

#define APIPAGE__bootloader__checkmagic			(APIPAGE_PAGEBYTEADDRESS+0x10)
#define APIPAGE__bootloader__reportBLSpagesize		(APIPAGE_PAGEBYTEADDRESS+0x14)

#define APIPAGE__bootloader__bootloader__ctrlfunc	(APIPAGE_PAGEBYTEADDRESS+0x1C)


#ifdef __APIPAGE_C_6e4723fa71ec40528d1ab16d9ff806d7
#	define APIPAGEPUBLIC
#else
#	define APIPAGEPUBLIC extern
#endif

#ifndef APIPAGESTANDALONE

/*
 * __do_spm:
 *
 * Call the "bootloader__do_spm"-function, located within the BLS via comfortable C-interface
 * During operation code will block - disable or reset watchdog before call.
 *
 * REMEMBER: interrupts have to be disabled! (otherwise code may crash non-deterministic)
 */
APIPAGEPUBLIC void __do_spm(const uint_farptr_t flashword_ptr, const uint8_t nvmcommand, const uint16_t dataword) APIPAGEAUXSECTION;

/*
 * __checkmagic:
 *
 * Asks the bootloader if MagicValue is correct on the running board
 * (uint8_t) retval is "0" (false) if magic is not correct - true otherwise.
 */
APIPAGEPUBLIC uint8_t __checkmagic(void) APIPAGEAUXSECTION;

/*
 * __reportBLSpagesize:
 *
 * Asks the bootloader how large it is in numbers of "BOOT_SECTION_PAGE_SIZE", decreasing downwards
 * from "BOOT_SECTION_END" in direction 0x00000.
 * (uint16_t) retval is "32" (=16384 bytes) in case "__bootloader__reportBLSpagesize" is not implemented.
 * This is since all bootloaders delivered before "__bootloader__reportBLSpagesize" was introduced,
 * were 16KiB at maximum.
 *
 * WARNING: DO NOT RELY ON THE REPORTED SIZE TO BE SMALLER OR EQUAL OF THE TOTAL NUMBER OF FLASH PAGES !
 */
APIPAGEPUBLIC uint16_t __reportBLSpagesize(void) APIPAGEAUXSECTION;

/*
 * __bootloader__ctrlfunc:
 *
 * Performs a bootloader specific syscall. Register "r18" contains the function code a.k.a syscall no.
 * If functioncode 0x00 and 0xff do not respond with 0x5a (in r18) - syscalls are not implemented by
 * the installed bootloader.
 */
APIPAGEPUBLIC uint8_t __ctrlfunc(uint8_t callcode) APIPAGEAUXSECTION;


/*
 * Read byte from address in flash
 */
APIPAGEPUBLIC uint8_t flash_read_Ex(const uint_farptr_t address, const uint8_t nvmCommand) APIPAGEAUXSECTION;

/*
 * Read page nr. "in_pageAddress" into "out_pageBuffer" and returns number of bytes read
 */
APIPAGEPUBLIC size_t flash_readpage(uint8_t out_pageBuffer[SPM_PAGESIZE], const uint_farptr_t in_pageNr) APIPAGEAUXSECTION;
APIPAGEPUBLIC size_t flash_readpage_Ex(uint8_t out_pageBuffer[SPM_PAGESIZE], const uint_farptr_t in_pageNr, const uint8_t nvmCommand) APIPAGEAUXSECTION;

/*
 * Compares "in_pageBuffer" with the page "in_pageNr" from flash memory.
 * The internal comparing uses bytewise backward (MSB to LSB) compare - so it needs only a few stack (compared to reading a whole page first).
 * The result will be -1 if "in_pageBuffer" is smaller, +1 if it is greater. If both pages are identical, it returns 0.
 */
APIPAGEPUBLIC int flash_comparepage(uint8_t in_pageBuffer[SPM_PAGESIZE], const uint_farptr_t in_pageNr) APIPAGEAUXSECTION;
APIPAGEPUBLIC int flash_comparepage_Ex(const uint8_t in_pageBuffer[SPM_PAGESIZE], const uint_farptr_t in_pageNr, const uint8_t nvmCommand) APIPAGEAUXSECTION;


/*
 * Write "in_pageBuffer" into page nr. "in_pageAddress" using "nvmCommand" via "spmfunc".
 * "flash_writepage" normally will use "NVM_CMD_ERASE_WRITE_FLASH_PAGE_gc" as "nvmCommand".
 * Returns the number of bytes written - usually SPM_PAGESIZE
 */
APIPAGEPUBLIC size_t flash_writepage(const uint8_t in_pageBuffer[SPM_PAGESIZE], const uint_farptr_t in_pageNr) APIPAGEAUXSECTION;
APIPAGEPUBLIC size_t flash_writepage_Ex(void *not_implemented_in_public__use_NULL, const uint8_t in_pageBuffer[SPM_PAGESIZE], const uint_farptr_t in_pageNr, const uint8_t nvmCommand) APIPAGEAUXSECTION;


/*
 * Compare "in_pageBuffer" with the page "in_pageNr" from flash memory via nvmReadCommand.
 * If "in_pageBuffer" is different, then write "in_pageBuffer" into page nr. "in_pageAddress"
 * using "nvmWriteCommand" via "spmfunc".
 * "flash_updatepage" normally will use "NVM_CMD_ERASE_WRITE_FLASH_PAGE_gc" as "nvmWriteCommand".
 * Returns the number of bytes written - usually SPM_PAGESIZE
 */
APIPAGEPUBLIC size_t flash_updatepage(const uint8_t in_pageBuffer[SPM_PAGESIZE], const uint_farptr_t in_pageNr) APIPAGEAUXSECTION;
APIPAGEPUBLIC size_t flash_updatepage_Ex(void *not_implemented_in_public__use_NULL, const uint8_t in_pageBuffer[SPM_PAGESIZE], const uint_farptr_t in_pageNr, const uint8_t nvmReadCommand, const uint8_t nvmWriteCommand) APIPAGEAUXSECTION;


/*
 * "bootloader_startup" hands the cpu over to the bootloader.
 * All pending operations should be finished and the system should be in default state (reboot?) before call.
 * Also ensure you restore initial clockcalibration and clockselection before call.
 * In case of success this function never returns. In case of error, negative numbers are returned.
 */
APIPAGEPUBLIC int8_t bootloader_startup(void) APIPAGEAUXSECTION;

#endif

#endif

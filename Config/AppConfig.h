/*
 * AppConfig.h
 * (c) 2014 flabbergast
 *  Custom definitions for use with LUFA, for AVR stick.
 *
 *  Based on a template from LUFA library (license below).
 */

/*
             LUFA Library
     Copyright (C) Dean Camera, 2014.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2014  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

   #ifndef FORMATTED_DATE
   #define FORMATTED_DATE __DATE__
   #endif

  #define FIRMWARE_VERSION          PSTR(" * enstix v1.2 (compiled "FORMATTED_DATE")\r\n   (c) 2014 flabbergast")

  #define TOTAL_LUNS                1

  /** Total number of bytes of the storage medium. */
  #define VIRTUAL_DISK_BYTES                65536

  /** Block size of the device. This is kept at 512 to remain compatible with the OS. */
  #define VIRTUAL_DISK_BLOCK_SIZE           512

  /** Where does the disk image in flash begin?
   * WARNING!!! No checking is done! It needs to be further than the firmware code ends!
   * WARNING!!! BEGIN+SIZE needs to fit into the available flash! */
  #define DISK_AREA_BEGIN_BYTE              0x6000

  /** Compute some extra numbers from Config/AppConfig ones. */
  /** Total number of blocks of the virtual memory for reporting to the host as the device's total capacity. */
  #define VIRTUAL_DISK_BLOCKS              (VIRTUAL_DISK_BYTES / VIRTUAL_DISK_BLOCK_SIZE)

  /** Blocks in each LUN, calculated from the total capacity divided by the total number of Logical Units in the device. */
  #define LUN_MEDIA_BLOCKS                 (VIRTUAL_DISK_BLOCKS / TOTAL_LUNS)

  /** Initial VirtualFAT disk configuration */
  #define VIRTUALFAT_DISK_BYTES             16384
  #define VIRTUALFAT_TOTAL_LUNS             1
  /** Block size of the device. This is kept at 512 to remain compatible with the OS. */
  #define VIRTUALFAT_DISK_BLOCK_SIZE        512
  /** Total number of blocks of the virtual memory for reporting to the host as the device's total capacity. */
  #define VIRTUALFAT_DISK_BLOCKS            (VIRTUALFAT_DISK_BYTES / VIRTUALFAT_DISK_BLOCK_SIZE)
  /** Blocks in each LUN, calculated from the total capacity divided by the total number of Logical Units in the device. */
  #define VIRTUALFAT_LUN_MEDIA_BLOCKS       (VIRTUALFAT_DISK_BLOCKS / VIRTUALFAT_TOTAL_LUNS)

  /** Size of the virtual README.TXT file in bytes. */
  /**  It's assumed to be a multiple of 512 (BLOCK_SIZE). */
  #define README_FILE_SIZE_BYTES    512

  /** Contents of the README file. Needs to have exactly README_FILE_SIZE_BYTES bytes!  */
  #define README_CONTENTS "\
To do anything interesting with this AVR stick, you should conne\
ct to it via the virtual serial port (using a serial terminal pr\
ogram, e.g. puTTY, minicom, picocom or screen).                 \
                                                                \
                                                                \
                                                                \
                                                                \
                                                                "

#endif

/*
 * SCSI.h
 * (c) 2015 flabbergast
 *  USB layer that acts as a SCSI device: header file.
 *
 *  Most of the code comes from a LUFA library Demo (license below).
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

/** \file
 *
 *  Header file for SCSI.c.
 */

#ifndef _SCSI_H_
#define _SCSI_H_

    #ifdef _INCLUDED_FROM_SCSI_C_
      #define GLOBALS_EXTERN_SCSI // nothing
    #else
      #define GLOBALS_EXTERN_SCSI extern
    #endif

  /* Includes: */
    #include <LUFA/Drivers/USB/USB.h>
    #include "../Config/AppConfig.h"

  /* Global variables */
    // Is the virtual disk read only? Should Detach/Attach the USB device after changing,
    //   since the OS can still think otherwise
    GLOBALS_EXTERN_SCSI volatile bool disk_read_only_GLOBAL;

    // Size of the virtual disk to be reported to OS - in BLOCKS!
    GLOBALS_EXTERN_SCSI volatile uint32_t disk_size_GLOBAL;

    // Initial state or encrypting? (should detach/attach USB after changing)
    #define DISK_STATE_INITIAL 1
    #define DISK_STATE_ENCRYPTING 2
    GLOBALS_EXTERN_SCSI volatile uint8_t disk_state_GLOBAL;

  /* Macros: */
    /** Macro to set the current SCSI sense data to the given key, additional sense code and additional sense qualifier. This
     *  is for convenience, as it allows for all three sense values (returned upon request to the host to give information about
     *  the last command failure) in a quick and easy manner.
     *
     *  \param[in] Key    New SCSI sense key to set the sense code to
     *  \param[in] Acode  New SCSI additional sense key to set the additional sense code to
     *  \param[in] Aqual  New SCSI additional sense key qualifier to set the additional sense qualifier code to
     */
    #define SCSI_SET_SENSE(Key, Acode, Aqual)  do { SenseData.SenseKey                 = (Key);   \
                                                    SenseData.AdditionalSenseCode      = (Acode); \
                                                    SenseData.AdditionalSenseQualifier = (Aqual); } while (0)

    /** Macro for the \ref SCSI_Command_ReadWrite_10() function, to indicate that data is to be read from the storage medium. */
    #define DATA_READ           true

    /** Macro for the \ref SCSI_Command_ReadWrite_10() function, to indicate that data is to be written to the storage medium. */
    #define DATA_WRITE          false

    /** Value for the DeviceType entry in the SCSI_Inquiry_Response_t enum, indicating a Block Media device. */
    #define DEVICE_TYPE_BLOCK   0x00

    /** Value for the DeviceType entry in the SCSI_Inquiry_Response_t enum, indicating a CD-ROM device. */
    #define DEVICE_TYPE_CDROM   0x05

  /* Function Prototypes: */
    bool SCSI_DecodeSCSICommand(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo);

    #if defined(_INCLUDED_FROM_SCSI_C_)
      static bool SCSI_Command_Inquiry(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo);
      static bool SCSI_Command_Request_Sense(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo);
      static bool SCSI_Command_Read_Capacity_10(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo);
      static bool SCSI_Command_ReadWrite_10(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo,
                                            const bool IsDataRead);
      static bool SCSI_Command_ModeSense_6(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo);
    #endif

#endif


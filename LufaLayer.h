/*
 * LufaLayer.h
 * (c) 2014 flabbergast
 *  USB and Board layer: header file.
 *  See below the #includes for a list of functions provided by this
 *    layer.
 *
 *  Portions of code come from a LUFA library Demo (license below).
 */

#ifndef _LUFALAYER_H_
#define _LUFALAYER_H_

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
 *  Header file for LufaLayer.c.
 */

  /* Includes: */
    #include <LUFA/Drivers/Board/LEDs.h>
    #include <LUFA/Drivers/Board/Buttons.h>
    #include <LUFA/Drivers/USB/USB.h>
    #include <LUFA/Platform/Platform.h>

    #include "SCSI/SCSI.h"

  /* by flabbergast: exported functions to be used in the main program code
   */
    // basic functions
    void init(void);
    void usb_tasks(void);
    // usb_serial
    uint16_t usb_serial_available(void); // number of getchars guaranteed to succeed immediately
    int16_t usb_serial_getchar(void); // negative values mean error in receiving (not connected or no input)
    void usb_serial_flush_input(void);
    void usb_serial_putchar(uint8_t ch);
    void usb_serial_wait_for_key(void); // BLOCKING (takes care of _tasks)
    void usb_serial_write(const char* const buffer);
    void usb_serial_write_P(const char* data);
    void usb_serial_writeln(const char* const buffer);
    void usb_serial_writeln_P(const char* data);
    void usb_serial_flush_output(void);
    uint16_t usb_serial_readline(char *buffer, const uint16_t buffer_size, const bool obscure_input); // BLOCKING (takes care of _tasks)
    // usb_keyboard
    bool usb_keyboard_press(uint8_t key, uint8_t mod);
    // buttons, LEDs and such
    uint32_t button_pressed_for(void); // for how long was the button pressed? (in 10/1024 sec; 0 if not pressed)
    void service_button(void); // should be called periodically to update the button state

  /* Macros: */
    /** LED mask for the library LED driver, to indicate that the USB interface is not ready. */
    #define LEDMASK_USB_NOTREADY      LEDS_LED1

    /** LED mask for the library LED driver, to indicate that the USB interface is enumerating. */
    #define LEDMASK_USB_ENUMERATING  (LEDS_LED2 | LEDS_LED3)

    /** LED mask for the library LED driver, to indicate that the USB interface is ready. */
    #define LEDMASK_USB_READY        (LEDS_LED2 | LEDS_LED4)

    /** LED mask for the library LED driver, to indicate that an error has occurred in the USB interface. */
    #define LEDMASK_USB_ERROR        (LEDS_LED1 | LEDS_LED3)

  /* Function Prototypes: */
    void SetupHardware(void);

    void EVENT_USB_Device_Connect(void);
    void EVENT_USB_Device_Disconnect(void);
    void EVENT_USB_Device_ConfigurationChanged(void);
    void EVENT_USB_Device_ControlRequest(void);
    void EVENT_USB_Device_StartOfFrame(void);

    bool CALLBACK_MS_Device_SCSICommandReceived(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo);

    bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                             uint8_t* const ReportID,
                                             const uint8_t ReportType,
                                             void* ReportData,
                                             uint16_t* const ReportSize);
    void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                              const uint8_t ReportID,
                                              const uint8_t ReportType,
                                              const void* ReportData,
                                              const uint16_t ReportSize);
#endif


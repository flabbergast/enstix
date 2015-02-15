/*
 * LufaLayer.h
 * (c) 2015 flabbergast
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
    #include "SCSI/SCSI.h"

  /* by flabbergast: exported functions to be used in the main program code
   */
    // --- basic functions ---
    void init(void);
    void usb_tasks(void);
    // --- usb_serial ---
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
    bool usb_serial_dtr(void);
    // --- usb_keyboard ---
    #ifdef _INCLUDED_FROM_LUFALAYER_C_
      #define GLOBALS_EXTERN_LUFALAYER
    #else
      #define GLOBALS_EXTERN_LUFALAYER extern
    #endif
    bool usb_keyboard_press(uint8_t key, uint8_t mod);
    // see  LUFA/Drivers/USB/Class/Common/HIDClassCommon.h for names for keys
    bool usb_keyboard_write(char* text);
    // status of keyboard LEDs
    GLOBALS_EXTERN_LUFALAYER uint8_t volatile usb_keyboard_leds;
    // delay between keypresses (*10ms)
    #define USB_KEYBOARD_KEYPRESS_DELAY 4
    // this can be used to send more complicated keypresses directly
    GLOBALS_EXTERN_LUFALAYER bool usb_keyboard_send_current_data_GLOBAL;
    GLOBALS_EXTERN_LUFALAYER uint8_t usb_keyboard_current_keys_GLOBAL[6];
    GLOBALS_EXTERN_LUFALAYER uint8_t usb_keyboard_current_modifier_GLOBAL;
    // for sending longer text
    GLOBALS_EXTERN_LUFALAYER bool usb_keyboard_sending_string_GLOBAL;

    // --- buttons, LEDs and such ---
    uint32_t button_pressed_for(void); // for how long was the button pressed? (in 10/1024 sec; 0 if not pressed)
    void service_button(void); // should be called periodically to update the button state

#endif


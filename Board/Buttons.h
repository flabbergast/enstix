/*
 * Buttons.h
 * (c) 2014 flabbergast
 *  Custom Buttons definitions for use with LUFA, for AVR stick.
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
#ifndef __BUTTONS_USER_H__
#define __BUTTONS_USER_H__
    /* Includes: */
        // TODO: Add any required includes here
        #include <avr/io.h>
    /* Enable C linkage for C++ Compilers: */
        #if defined(__cplusplus)
            extern "C" {
        #endif
    /* Preprocessor Checks: */
        #if !defined(__INCLUDE_FROM_BUTTONS_H)
            #error Do not include this file directly. Include LUFA/Drivers/Board/Buttons.h instead.
        #endif
    /* Public Interface - May be used in end-application: */
        /* Macros: */
#if defined(__AVR_ATxmega128A3U__) // avrstick
            #define BUTTONS_BUTTON1        _BV(7)
            #define BUTTONS_PORT PORTF
#elif defined(__AVR_ATxmega128A4U__) // X-A4U board
            #define BUTTONS_BUTTON1        _BV(0)
            #define BUTTONS_PORT PORTE
#endif
        /* Inline Functions: */
        #if !defined(__DOXYGEN__)
            static inline void Buttons_Init(void)
            {
                BUTTONS_PORT.DIRCLR =  BUTTONS_BUTTON1;
                BUTTONS_PORT.PIN7CTRL = PORT_OPC_PULLUP_gc;     // pull-up on pin F7
            }
            static inline void Buttons_Disable(void)
            {
                BUTTONS_PORT.PIN7CTRL = 0;     // don't know what's the default state of pull-up/down?
                BUTTONS_PORT.DIRCLR = BUTTONS_BUTTON1;
            }
            static inline uint8_t Buttons_GetStatus(void) ATTR_WARN_UNUSED_RESULT;
            static inline uint8_t Buttons_GetStatus(void)
            {
                // TODO: Return current button status here, debounced if required
                return ((BUTTONS_PORT.IN & BUTTONS_BUTTON1) ^ BUTTONS_BUTTON1);
            }
        #endif
    /* Disable C linkage for C++ Compilers: */
        #if defined(__cplusplus)
            }
        #endif
#endif


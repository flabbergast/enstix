/*
 * LEDs.h
 * (c) 2014 flabbergast
 *  Custom LEDs definitions for use with LUFA, for AVR stick.
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
#ifndef __LEDS_USER_H__
#define __LEDS_USER_H__
    /* Includes: */
        #include <avr/io.h>
    /* Enable C linkage for C++ Compilers: */
        #if defined(__cplusplus)
            extern "C" {
        #endif
    /* Preprocessor Checks: */
        #if !defined(__INCLUDE_FROM_LEDS_H)
            #error Do not include this file directly. Include LUFA/Drivers/Board/LEDS.h instead.
        #endif
    /* Public Interface - May be used in end-application: */
        /* Macros: */
            #define LEDS_LED1        (1 << 0)
            #define LEDS_LED2        (1 << 1)
            #define LEDS_LED3        (1 << 2)
            #define LEDS_LED4        (1 << 3)
            #define LEDS_ALL_LEDS    (LEDS_LED1 | LEDS_LED2 | LEDS_LED3 | LEDS_LED4)
            #define LEDS_NO_LEDS     0
        /* Inline Functions: */
        #if !defined(__DOXYGEN__)
            static inline void LEDs_Init(void)
            {
                PORTE.DIRSET = LEDS_ALL_LEDS;
                PORTE.OUTCLR = LEDS_ALL_LEDS;
            }
            static inline void LEDs_Disable(void)
            {
                PORTE.DIRCLR = LEDS_ALL_LEDS;
                PORTE.OUTCLR = LEDS_ALL_LEDS;
            }
            static inline void LEDs_TurnOnLEDs(const uint8_t LEDMask)
            {
                PORTE.OUTSET = LEDMask & LEDS_ALL_LEDS;
            }
            static inline void LEDs_TurnOffLEDs(const uint8_t LEDMask)
            {
                PORTE.OUTCLR = LEDMask & LEDS_ALL_LEDS;
            }
            static inline void LEDs_SetAllLEDs(const uint8_t LEDMask)
            {
                PORTE.OUTCLR = LEDS_ALL_LEDS;
                PORTE.OUTSET = LEDMask & LEDS_ALL_LEDS;
            }
            static inline void LEDs_ChangeLEDs(const uint8_t LEDMask, const uint8_t ActiveMask)
            {
                PORTE.OUTCLR = (LEDMask & LEDS_ALL_LEDS);
                PORTE.OUTSET = (ActiveMask & LEDS_ALL_LEDS);
            }
            static inline void LEDs_ToggleLEDs(const uint8_t LEDMask)
            {
                PORTE.OUTTGL = (LEDMask & LEDS_ALL_LEDS);
            }
            static inline uint8_t LEDs_GetLEDs(void) ATTR_WARN_UNUSED_RESULT;
            static inline uint8_t LEDs_GetLEDs(void)
            {
                return ((PORTE.OUT & LEDS_ALL_LEDS));
            }
        #endif
    /* Disable C linkage for C++ Compilers: */
        #if defined(__cplusplus)
            }
        #endif
#endif


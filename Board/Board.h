/*
 * Board.h
 * (c) 2014 flabbergast
 *  Custom Board definitions for use with LUFA, for AVR stick.
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
#ifndef __BOARD_USER_H__
#define __BOARD_USER_H__
    /* Includes: */
        // TODO: Add any required includes here
    /* Enable C linkage for C++ Compilers: */
        #if defined(__cplusplus)
            extern "C" {
        #endif
    /* Preprocessor Checks: */
        #if !defined(__INCLUDE_FROM_BOARD_H)
            #error Do not include this file directly. Include LUFA/Drivers/Board/Board.h instead.
        #endif
    /* Public Interface - May be used in end-application: */
        /* Macros: */
          #define BOARD_HAS_BUTTONS
//          #define BOARD_HAS_DATAFLASH
//          #define BOARD_HAS_JOYSTICK
//          #define BOARD_HAS_LEDS
    /* Disable C linkage for C++ Compilers: */
        #if defined(__cplusplus)
            }
        #endif
#endif


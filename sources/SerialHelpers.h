/*
 * SerialHelpers.h
 * (c) 2015 flabbergast
 *
 * Helper functions for printing over usb serial.
 *
 */

#ifndef _SERIALHELPERS_H_
#define _SERIALHELPERS_H_

#include <stdint.h>

#define HEXPRINT_SEPARATOR ' '

void hexprint(uint8_t *p, uint16_t length);
void hexprint_noln(uint8_t *p, uint16_t length);
void hexprint_byte(uint8_t b);
void hexprint_byte_sep(uint8_t b);
void hexprint_4bits(uint8_t b);
void hexprint_1bit(uint8_t b);

void usb_serial_write_dec8(uint8_t byte);
void usb_serial_writeln_dec8(uint8_t byte);
void usb_serial_write_dec16(uint16_t word);
void usb_serial_writeln_dec16(uint16_t word);
void usb_serial_write_dec32(uint32_t dword);
void usb_serial_writeln_dec32(uint32_t dword);

#endif

/*
 * SerialHelpers.c
 * (c) 2015 flabbergast
 *
 * Helper functions for printing over usb serial.
 *
 */

#include "SerialHelpers.h"
#include "LufaLayer.h"

void hexprint_byte(uint8_t b) {
  uint8_t high, low;
  low = b & 0xF;
  high = b >> 4;
  usb_serial_putchar(high+'0'+7*(high/10));
  usb_serial_putchar(low+'0'+7*(low/10));
}

void hexprint_byte_sep(uint8_t b) {
  hexprint_byte(b);
  usb_serial_putchar(HEXPRINT_SEPARATOR);
}

void hexprint_noln(uint8_t *p, uint16_t length) {
  for(uint16_t i=0; i<length; i++) {
    hexprint_byte(p[i]);
    if(HEXPRINT_SEPARATOR!=0)
      usb_serial_putchar(HEXPRINT_SEPARATOR);
  }
}

void hexprint(uint8_t *p, uint16_t length) {
  hexprint_noln(p, length);
  usb_serial_write_P(PSTR("\n\r"));
}

void hexprint_4bits(uint8_t b) {
  usb_serial_putchar((b&0xF)+'0'+7*((b&0xF)/10));
}

void hexprint_1bit(uint8_t b) {
  usb_serial_putchar((b&1)+'0');
}

void usb_serial_write_dec8(uint8_t byte) {
  uint8_t num = 100;
  uint8_t started = 0;

  while(num > 0)
  {
    uint8_t b = byte / num;
    if(b > 0 || started || num == 1)
    {
      usb_serial_putchar('0' + b);
      started = 1;
    }
    byte -= b * num;

    num /= 10;
  }
}

void usb_serial_writeln_dec8(uint8_t byte) {
  usb_serial_write_dec8(byte);
  usb_serial_write_P(PSTR("\n\r"));
}

void usb_serial_write_dec16(uint16_t word) {
  uint16_t num = 10000;
  uint8_t started = 0;

  while(num > 0)
  {
    uint8_t b = word / num;
    if(b > 0 || started || num == 1)
    {
      usb_serial_putchar('0' + b);
      started = 1;
    }
    word -= b * num;

    num /= 10;
  }
}

void usb_serial_writeln_dec16(uint16_t word) {
  usb_serial_write_dec16(word);
  usb_serial_write_P(PSTR("\n\r"));
}

void usb_serial_write_dec32(uint32_t dword) {
  uint32_t num = 1000000000;
  uint8_t started = 0;

  while(num > 0)
  {
    uint8_t b = dword / num;
    if(b > 0 || started || num == 1)
    {
      usb_serial_putchar('0' + b);
      started = 1;
    }
    dword -= b * num;

    num /= 10;
  }
}

void usb_serial_writeln_dec32(uint32_t dword) {
  usb_serial_write_dec32(dword);
  usb_serial_write_P(PSTR("\n\r"));
}


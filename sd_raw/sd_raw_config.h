/*
 * sd_raw_config.h
 * (c) 2014 flabbergast
 *  Configuration (mainly pins used) for sd_raw.
 *  Code originally by Roland Riegel (license below).
 *  Removed code not needed for this app by flabbergast.
 */

/*
 * Copyright (c) 2006-2012 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef SD_RAW_CONFIG_H
#define SD_RAW_CONFIG_H

#include <stdint.h>

// for LED indicator of activity (I use LUFA for this)
#include <LUFA/Drivers/Board/LEDS.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * \addtogroup sd_raw
 *
 * @{
 */
/**
 * \file
 * MMC/SD support configuration (license: GPLv2 or LGPLv2.1)
 */

/**
 * \ingroup sd_raw_config
 * Controls support for SDHC cards.
 *
 * Set to 1 to support so-called SDHC memory cards, i.e. SD
 * cards with more than 2 gigabytes of memory.
 */
#define SD_RAW_SDHC 1

/**
 * @}
 */

/* defines for customisation of sd/mmc port access */
#if defined(__AVR_ATmega8__) || \
    defined(__AVR_ATmega48__) || \
    defined(__AVR_ATmega48P__) || \
    defined(__AVR_ATmega88__) || \
    defined(__AVR_ATmega88P__) || \
    defined(__AVR_ATmega168__) || \
    defined(__AVR_ATmega168P__) || \
    defined(__AVR_ATmega328P__)
    #define configure_pin_mosi() DDRB |= (1 << DDB3)
    #define configure_pin_sck() DDRB |= (1 << DDB5)
    #define configure_pin_ss() DDRB |= (1 << DDB2)
    #define configure_pin_miso() DDRB &= ~(1 << DDB4)

    #define select_card() PORTB &= ~(1 << PORTB2)
    #define unselect_card() PORTB |= (1 << PORTB2)
#elif defined(__AVR_ATmega16__) || \
      defined(__AVR_ATmega32__)
    #define configure_pin_mosi() DDRB |= (1 << DDB5)
    #define configure_pin_sck() DDRB |= (1 << DDB7)
    #define configure_pin_ss() DDRB |= (1 << DDB4)
    #define configure_pin_miso() DDRB &= ~(1 << DDB6)

    #define select_card() PORTB &= ~(1 << PORTB4)
    #define unselect_card() PORTB |= (1 << PORTB4)
#elif defined(__AVR_ATmega64__) || \
      defined(__AVR_ATmega128__) || \
      defined(__AVR_ATmega169__)
    #define configure_pin_mosi() DDRB |= (1 << DDB2)
    #define configure_pin_sck() DDRB |= (1 << DDB1)
    #define configure_pin_ss() DDRB |= (1 << DDB0)
    #define configure_pin_miso() DDRB &= ~(1 << DDB3)

    #define select_card() PORTB &= ~(1 << PORTB0)
    #define unselect_card() PORTB |= (1 << PORTB0)
#elif defined(__AVR_ATmega32U4__)
    #define configure_pin_mosi() DDRB |= (1 << DDB2)
    #define configure_pin_sck() DDRB |= (1 << DDB1)
    #define configure_pin_ss() DDRB |= (1 << DDB6)
    #define configure_pin_miso() DDRB &= ~(1 << DDB3)

    #define select_card() PORTB &= ~(1 << PORTB6)
    #define unselect_card() PORTB |= (1 << PORTB6)
#elif defined(__AVR_ATxmega128A3U__)
    #define SPIPORT SPIE
    #define configure_pin_mosi() PORTE.DIRSET = (1 << 5)
    #define configure_pin_sck() PORTE.DIRSET = (1 << 7)
    #define configure_pin_ss() PORTE.DIRSET = (1 << 4)
    #define configure_pin_miso() PORTE.DIRCLR = (1 << 6)

    #define select_card() PORTE.OUTCLR = (1 << 4); LEDs_TurnOnLEDs(LEDS_LED1)
    #define unselect_card() PORTE.OUTSET = (1 << 4); LEDs_TurnOffLEDs(LEDS_LED1)
#else
    #error "no sd/mmc pin mapping available!"
#endif

  /* If available/lock pins are connected, define things here */
  /*
#define configure_pin_available() DDRC &= ~(1 << DDC4)
#define configure_pin_locked() DDRC &= ~(1 << DDC5)

#define get_pin_available() (PINC & (1 << PINC4))
#define get_pin_locked() (PINC & (1 << PINC5))
*/

  /* If available/lock pins are not used, use these: */
#define configure_pin_available() // nothing
#define configure_pin_locked() // nothing

#define get_pin_available() 0
#define get_pin_locked() 1

#if SD_RAW_SDHC
    typedef uint64_t offset_t;
#else
    typedef uint32_t offset_t;
#endif

#ifdef __cplusplus
}
#endif

#endif


/*
 * enstix.c
 * (c) 2014 flabbergast
 *  main firmware logic
 */

#include "LufaLayer.h"
#include "enstix.h"

#include "crypto/crypto.h"
#include "crypto/sha256.h"

#include "apipage.h"

#include <avr/eeprom.h>
#include "eeprom_contents.c"

#include <string.h>

/*************************************************************************
 * ----------------------- Global variables -----------------------------*
 *************************************************************************/
/*  Encryption related: used both in main() and disk read/write callbacks. */
#define PASSPHRASE_MAX_LEN 100
char passphrase[PASSPHRASE_MAX_LEN];
uint8_t pp_hash[32];
uint8_t pp_hash_hash[32];
uint8_t key[16];
uint8_t key_hash[32];
uint8_t iv[16];

/*************************************************************************
 * ----------------------- Helper functions -----------------------------*
 *************************************************************************/
void hexprint(uint8_t *p, uint16_t length) {
  uint16_t i;
  char buffer[4];
  for(i=0; i<length; i++) {
    snprintf(buffer, 3, "%02x", p[i]);
    usb_serial_write(buffer);
  }
  usb_serial_write_P(PSTR("\n\r"));
}

void compute_iv_for_sector(uint32_t sectorNumber);

/*************************************************************************
 * ------------------------- Main Program -------------------------------*
 *************************************************************************/

int main(void)
{
  /* Variables */
  bool button_press_registered = false;
  uint32_t button_press_length = 0;

  /* Initialisation */
  init();

  /* read the eeprom data into SRAM */
  eeprom_read_block((void*)key, (const void*)aes_key_encrypted, 16); // it's still encrypted at this point

  /* Must throw away unused bytes from the host, or it will lock up while waiting for the device */
  usb_serial_flush_input();

  /* Main loop.*/
  for (;;)
  {
    // run the task which checks the state of the button
    service_button();
    // for how long was the button pressed?
    button_press_length = button_pressed_for();
    // if at least 70ms and we haven't acted on this press yet
    if( button_press_length >= 7 && !button_press_registered ) {
      // remember that we've acted on the current button press
      button_press_registered = true;
      // announce the button press over the serial
      usb_serial_writeln_P(PSTR("Button pressed."));
      //usb_keyboard_press(HID_KEYBOARD_SC_N, HID_KEYBOARD_MODIFIER_LEFTSHIFT);
    }
    // was the button released after being pressed?
    if( button_press_registered && button_press_length < 7 ) {
      button_press_registered = false;
    }

    /* Handling of the serial dialogue */
    if( usb_serial_available() > 0 ) {
      switch(usb_serial_getchar()) {
        case 'i':
          usb_serial_writeln_P(FIRMWARE_VERSION);
          if(disk_state == DISK_STATE_INITIAL) {
            usb_serial_write_P(PSTR("Encrypted main AES key: "));
            hexprint(key, 16);
          } else if(disk_state == DISK_STATE_ENCRYPTING) {
            usb_serial_write_P(PSTR("Main AES key: "));
            hexprint(key, 16);
            usb_serial_write_P(PSTR("Passphrase hash: "));
            hexprint(pp_hash, 32);
            usb_serial_write_P(PSTR("Hash(passphrase hash): "));
            hexprint(pp_hash_hash, 32);
            usb_serial_write_P(PSTR("Disk state: "));
            if(disk_read_only) {
              usb_serial_writeln_P(PSTR("read-only"));
            } else {
              usb_serial_writeln_P(PSTR("writable"));
            }
          }
          break;
        case 'r':
          if(disk_state == DISK_STATE_ENCRYPTING) {
            usb_serial_write_P(PSTR("Disk state: "));
            if(disk_read_only) {
              usb_serial_writeln_P(PSTR("read-only. Switch to writable? [yN]"));
            } else {
              usb_serial_writeln_P(PSTR("writable. Switch to read-only? [Yn]"));
            }
            usb_serial_wait_for_key();
            char c = usb_serial_getchar();
            if(disk_read_only) {
              if(c == 'Y' || c == 'y') {
                usb_serial_write_P(PSTR("Switching to writable."));
                usb_serial_writeln_P(PSTR("Everything will disconnect."));
                usb_serial_write_P(PSTR("Press a key to continue..."));
                usb_serial_wait_for_key();
                USB_Disable();
                disk_read_only = false;
                _delay_ms(1000);
                USB_Init();
                _delay_ms(200);
                usb_serial_flush_input();
                break;
              }
            } else {
              if(c != 'N' && c != 'n') {
                usb_serial_write_P(PSTR("Switching to read-only."));
                usb_serial_writeln_P(PSTR("Everything will disconnect."));
                usb_serial_write_P(PSTR("Press a key to continue..."));
                usb_serial_wait_for_key();
                USB_Disable();
                disk_read_only = true;
                _delay_ms(1000);
                USB_Init();
                _delay_ms(200);
                usb_serial_flush_input();
                break;
              }
            }
            usb_serial_writeln_P(PSTR("Not doing anything."));
          } else {
            usb_serial_writeln_P(PSTR("This only works in encrypted mode."));
          }
          break;
        case 'p':
          if(disk_state == DISK_STATE_INITIAL) {
            // TODO: we should ask for a password here -> into 'passphrase'
            usb_serial_writeln_P(PSTR("Enter passphrase:"));
            usb_serial_readline(passphrase, PASSPHRASE_MAX_LEN, true);
            // compute hashes
            sha256((sha256_hash_t *)pp_hash, (const void*)passphrase, 8*strlen(passphrase));
            sha256((sha256_hash_t *)pp_hash_hash, (const void*)pp_hash, 8*32);
            // compare the hash of hash of the passphrase with the eeprom
            bool match = true;
            for(uint8_t i=0; i<32; i++) {
              if(eeprom_read_byte(passphrase_hash_hash+i) != pp_hash_hash[i]) {
                match = false;
                break;
              }
            }
            if(match) { // if the passphrase is correct
              aes256_dec_single(pp_hash, key); // decrypt the aes key
              sha256((sha256_hash_t *)key_hash, (const void*)key, 8*16); // remember the hash as well, for ESSIV
              usb_serial_writeln_P(PSTR("Password OK. Switching to encrypted disk mode (everything will disconnect)."));
              usb_serial_write_P(PSTR("Press a key to continue..."));
              usb_serial_wait_for_key();
              disk_state = DISK_STATE_ENCRYPTING;
              disk_read_only = true;
              USB_Disable();
              // which to use? _Detach and _Attach; or _Disable and _Init; or _ResetInterface
              // best experience with Disable/Init so far
              _delay_ms(1000);
              USB_Init();
              _delay_ms(200);
              usb_serial_flush_input();
            } else {
              usb_serial_writeln_P(PSTR("Problem: the entered passphrase is not correct. Not doing anything."));
            }
          } else {
            usb_serial_writeln_P(PSTR("Already in encrypted disk mode."));
          }
          break;
        default:
          usb_serial_writeln_P(PSTR("-> Help: [i]nfo, [r]o/rw, [p]assphrase"));
      }
    }

    /* need to run this relatively often to keep the USB connection alive */
    usb_tasks();
  }
}

/*************************************************************************
 * ------------------- CALLBACKS to be implemented ----------------------*
 *************************************************************************/

/* Compute some constants */
#define DISK_AREA_BEGIN_PAGE              (DISK_AREA_BEGIN_BYTE/BOOT_SECTION_PAGE_SIZE)
#ifndef PROGMEM_PAGECOUNT
  #define PROGMEM_PAGECOUNT               (PROGMEM_SIZE/BOOT_SECTION_PAGE_SIZE)
#endif
#define MEM_PAGES_PER_DISK_BLOCK          (VIRTUAL_DISK_BLOCK_SIZE/BOOT_SECTION_PAGE_SIZE)
// this is 1 on atxmega128a3u

int16_t CALLBACK_disk_readSector(uint8_t out_sectordata[VIRTUAL_DISK_BLOCK_SIZE], const uint32_t sectorNumber) {
  //memset(&out_sectordata[0], ~(sectorNumber & 0xff), VIRTUAL_DISK_BLOCK_SIZE);

  /* compute iv */
  compute_iv_for_sector(sectorNumber);

  /* read the data */
  memcpy_PF(out_sectordata, (uint_farptr_t)DISK_AREA_BEGIN_BYTE+(uint_farptr_t)(sectorNumber*VIRTUAL_DISK_BLOCK_SIZE), VIRTUAL_DISK_BLOCK_SIZE);
  //flash_readpage(out_sectordata, DISK_AREA_BEGIN_PAGE+sectorNumber);

  /* decrypt */
  aes128_cbc_dec(key, iv, out_sectordata, VIRTUAL_DISK_BLOCK_SIZE);

  return VIRTUAL_DISK_BLOCK_SIZE;
}

int16_t CALLBACK_disk_writeSector(uint8_t in_sectordata[VIRTUAL_DISK_BLOCK_SIZE], const uint32_t sectorNumber) {
  if(__checkmagic()==0) { // do not run the code if not on Stephan Baerwolf's hardware/bootloader
    return 0;
  }

  /* compute iv */
  compute_iv_for_sector(sectorNumber);

  /* encrypt the data */
  aes128_cbc_enc(key, iv, in_sectordata, VIRTUAL_DISK_BLOCK_SIZE);

  /* write the data to flash */
  // figure out how many memory pages we need to write
  uint_farptr_t write_to_page = DISK_AREA_BEGIN_PAGE + (sectorNumber * MEM_PAGES_PER_DISK_BLOCK);
  // check that we're not going into the bootloader area
  if(write_to_page+MEM_PAGES_PER_DISK_BLOCK <= PROGMEM_PAGECOUNT-__reportBLSpagesize())
    for(uint8_t i=0; i<MEM_PAGES_PER_DISK_BLOCK; i++)
      flash_writepage(in_sectordata+(MEM_PAGES_PER_DISK_BLOCK*i), write_to_page+i);

  return VIRTUAL_DISK_BLOCK_SIZE;
}

// uses global variables: iv, key_hash. Assumes key_hash has the hash of the key in it :)
void compute_iv_for_sector(uint32_t sectorNumber) {
  /* compute iv for the sector */
  // prepare for encrypting sector number (endianness matters!)
  memset((void *)iv, 0, 16);
  memcpy((void *)iv, (const void*)&sectorNumber, sizeof(uint32_t));
  // encrypt the sn with aes128, the key being the hash of the main key
  aes128_enc_single(key_hash, iv);
}

/*
 * enstix.c
 * (c) 2014 flabbergast
 *  main firmware logic
 */

#include "LufaLayer.h"
#include "SerialHelpers.h"
#include "enstix.h"

#include "crypto/crypto.h"
#include "crypto/sha256.h"

#include "sd_raw/sd_raw.h"

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
char temp_buf[PASSPHRASE_MAX_LEN];
uint8_t pp_hash[32];
uint8_t pp_hash_hash[32];
uint8_t key[16];
#if (defined(__AVR_ATxmega128A3U__))
uint8_t lastsubkey[16]; // hardware AES module needs a different key for decryption
#endif
uint8_t key_hash[32];
uint8_t iv[16];
#if defined(USE_SDCARD)
uint8_t sd_exists = 0;
struct sd_raw_info sd_card_info;
#endif

/*************************************************************************
 * ----------------------- Helper functions -----------------------------*
 *************************************************************************/
void print_help(void);
void print_header(void);
#if defined(USE_SDCARD)
void print_sd_card_info(void);
#endif
void compute_iv_for_sector(uint32_t sectorNumber);

#define DISABLE_JTAG CPU_CCP = CCP_IOREG_gc; MCU.MCUCR = MCU_JTAGD_bm

/*************************************************************************
 * ------------------------- Main Program -------------------------------*
 *************************************************************************/

int main(void)
{
  /* Variables */
  bool button_press_registered = false;
  uint32_t button_press_length = 0;

  // should set the disk size soon
  disk_size = VIRTUALFAT_DISK_BLOCKS;

  bool dtr,prev_dtr = false;

  /* disable JTAG on XMEGAs */
#if (defined(__AVR_ATxmega128A3U__))
    DISABLE_JTAG;
#endif

  /* Initialisation */
  init();

#if defined(USE_SDCARD)
  if( sd_raw_init() && sd_raw_get_info(&sd_card_info) ) {
    sd_exists = 1;
  }
#endif

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

    // check dtr
    dtr = usb_serial_dtr();
    if( dtr && !prev_dtr ) {
      _delay_ms(50);
      print_header();
      print_help();
    }
    prev_dtr = dtr;

    /* Handling of the serial dialogue */
    if( usb_serial_available() > 0 ) {
      switch(usb_serial_getchar()) {
        case 'i': // info
          print_header();
#if defined(USE_SDCARD)
          usb_serial_write_P(PSTR("(Micro)SD status: "));
          if(sd_exists) {
            usb_serial_writeln_P(PSTR("initialised"));
            print_sd_card_info();
          } else {
            usb_serial_writeln_P(PSTR("not connected/communicating"));
          }
#endif
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
        case 'c': // change the password
          if(disk_state == DISK_STATE_ENCRYPTING) {
            usb_serial_writeln_P(PSTR("Enter current passphrase:"));
            usb_serial_readline(passphrase, PASSPHRASE_MAX_LEN, true);
            // compute hashes
            sha256((sha256_hash_t *)temp_buf, (const void*)passphrase, 8*strlen(passphrase));
            sha256((sha256_hash_t *)passphrase, (const void*)temp_buf, 8*32); // re-use the space allocated for the passphrase buffer
            int compare = memcmp((const void *)passphrase, (const void *)pp_hash_hash, 32);
            memset(passphrase, 0xFF, PASSPHRASE_MAX_LEN); // wipe the passphrase from memory
            if(compare == 0) {
              usb_serial_writeln_P(PSTR("Enter a new passphrase:"));
              usb_serial_readline(passphrase, PASSPHRASE_MAX_LEN, true);
              usb_serial_writeln_P(PSTR("Enter the new passphrase again:"));
              usb_serial_readline(temp_buf, PASSPHRASE_MAX_LEN, true);
              compare = strcmp(passphrase, temp_buf);
              memset(temp_buf, 0xFF, PASSPHRASE_MAX_LEN);
              if(compare == 0) {
                usb_serial_writeln_P(PSTR("Match. Changing the passphrase."));
                sha256((sha256_hash_t *)pp_hash, (const void*)passphrase, 8*strlen(passphrase));
                memset(passphrase, 0xFF, PASSPHRASE_MAX_LEN); // wipe the passphrase from memory
                sha256((sha256_hash_t *)pp_hash_hash, (const void*)pp_hash, 8*32);
                memcpy(temp_buf, key, 16); // copy the key to a temp buffer for encrypting
                aes128_enc_single(pp_hash, temp_buf); // encrypt the aes key
                // save the new pp_hash_hash and encr.aes.key to EEPROM
                eeprom_write_block((const void*)temp_buf, (void*)aes_key_encrypted, 16);
                eeprom_write_block((const void*)pp_hash_hash, (void*)passphrase_hash_hash, 32);
              } else {
                usb_serial_writeln_P(PSTR("The passphrases don't match. Not changing anything."));
              }
            } else {
              usb_serial_writeln_P(PSTR("Entered passphrase is not correct."));
            }
          } else {
            usb_serial_writeln_P(PSTR("Passphrase changing works only in encrypted mode."));
          }
          break;
        case 'r': // switch ro/rw
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
        case 'p': // enter password
          if(disk_state == DISK_STATE_INITIAL) {
            usb_serial_writeln_P(PSTR("Enter passphrase:"));
            usb_serial_readline(passphrase, PASSPHRASE_MAX_LEN, true);
            // compute hashes
            sha256((sha256_hash_t *)pp_hash, (const void*)passphrase, 8*strlen(passphrase));
            sha256((sha256_hash_t *)pp_hash_hash, (const void*)pp_hash, 8*32);
            // wipe the passphrase from memory
            memset(passphrase, 0xFF, PASSPHRASE_MAX_LEN);
            // compare the hash of hash of the passphrase with the eeprom
            bool match = true;
            for(uint8_t i=0; i<32; i++) {
              if(eeprom_read_byte(passphrase_hash_hash+i) != pp_hash_hash[i]) {
                match = false;
                break;
              }
            }
            if(match) { // if the passphrase is correct
#if (defined(__AVR_ATxmega128A3U__))
                // hardware AES decryption needs another key
                AES_lastsubkey_generate(pp_hash, lastsubkey);
                aes128_dec_single(lastsubkey, key); // decrypt the aes key
                AES_lastsubkey_generate(key, lastsubkey);
#else
                aes128_dec_single(pp_hash, key); // decrypt the aes key
#endif
              sha256((sha256_hash_t *)key_hash, (const void*)key, 8*16); // remember the hash as well, for ESSIV
              usb_serial_writeln_P(PSTR("Password OK. Switching to encrypted disk mode (everything will disconnect)."));
              usb_serial_write_P(PSTR("Press a key to continue..."));
              usb_serial_wait_for_key();
              disk_state = DISK_STATE_ENCRYPTING;
              disk_read_only = true;
#if defined(USE_SDCARD)
              if(sd_exists) {
                disk_size = (uint32_t)(sd_card_info.capacity / DISK_BLOCK_SIZE);
              }
#endif
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
        case 'T': // TODO: testing
          if(disk_state == DISK_STATE_INITIAL) {
              disk_state = DISK_STATE_ENCRYPTING;
              disk_read_only = true;
#if defined(USE_SDCARD)
              if(sd_exists) {
                disk_size = (uint32_t)(sd_card_info.capacity / SD_BLOCK_SIZE);
              }
#endif
              USB_Disable();
              // which to use? _Detach and _Attach; or _Disable and _Init; or _ResetInterface
              // best experience with Disable/Init so far
              _delay_ms(1000);
              USB_Init();
              _delay_ms(200);
              usb_serial_flush_input();
          }
          break;
        default:
          print_help();
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
#define MEM_PAGES_PER_DISK_BLOCK          (DISK_BLOCK_SIZE/BOOT_SECTION_PAGE_SIZE)
// this should be 1 on atxmega128a3u

int16_t CALLBACK_disk_readSector(uint8_t out_sectordata[DISK_BLOCK_SIZE], const uint32_t sectorNumber) {
  /* compute iv */
  compute_iv_for_sector(sectorNumber);

#if defined(USE_SDCARD)
  if(sd_exists) {
    sd_raw_read_block(sectorNumber, out_sectordata);
  } else {
    return 0;
  }
#else
  #if (defined(__AVR_ATxmega128A3U__))
  /* read the data */
  memcpy_PF(out_sectordata, (uint_farptr_t)DISK_AREA_BEGIN_BYTE+(uint_farptr_t)(sectorNumber*DISK_BLOCK_SIZE), DISK_BLOCK_SIZE);
  //flash_readpage(out_sectordata, DISK_AREA_BEGIN_PAGE+sectorNumber);
  #else // need to do something else if not on x128a3u
  memset(&out_sectordata[0], ~(sectorNumber & 0xff), DISK_BLOCK_SIZE);
  #endif
#endif

  /* decrypt */
#if (defined(__AVR_ATxmega128A3U__))
    // hardware AES module needs a different key for decryption
    aes128_cbc_dec(lastsubkey, iv, out_sectordata, DISK_BLOCK_SIZE);
#else
    aes128_cbc_dec(key, iv, out_sectordata, DISK_BLOCK_SIZE);
#endif

  return DISK_BLOCK_SIZE;
}

int16_t CALLBACK_disk_writeSector(uint8_t in_sectordata[DISK_BLOCK_SIZE], const uint32_t sectorNumber) {
  /* compute iv */
  compute_iv_for_sector(sectorNumber);

  /* encrypt the data */
  aes128_cbc_enc(key, iv, in_sectordata, DISK_BLOCK_SIZE);

#if defined(USE_SDCARD)
  if(sd_exists) {
    sd_raw_write_block(sectorNumber, in_sectordata);
  }
#else
  #if (defined(__AVR_ATxmega128A3U__)) // this can only happen on x128a3u
  if(__checkmagic()==0) { // do not run the code if not on Stephan Baerwolf's hardware/bootloader
    return 0;
  }
  /* write the data to flash */
  // figure out how many memory pages we need to write
  uint_farptr_t write_to_page = DISK_AREA_BEGIN_PAGE + (sectorNumber * MEM_PAGES_PER_DISK_BLOCK);
  // check that we're not going into the bootloader area
  if(write_to_page+MEM_PAGES_PER_DISK_BLOCK <= PROGMEM_PAGECOUNT-__reportBLSpagesize())
    for(uint8_t i=0; i<MEM_PAGES_PER_DISK_BLOCK; i++)
      flash_writepage(in_sectordata+(BOOT_SECTION_PAGE_SIZE*i), write_to_page+i);
  #else
  return 0;
  #endif
#endif

  return DISK_BLOCK_SIZE;
}

/*************************************************************************
 * ----------------- Helper functions implementation --------------------*
 *************************************************************************/

// uses global variables: iv, key_hash. Assumes key_hash has the hash of the key in it :)
void compute_iv_for_sector(uint32_t sectorNumber) {
  /* compute iv for the sector */
  // prepare for encrypting sector number (endianness matters!)
  memset(iv, 0, 16);
  memcpy(iv, (const void*)&sectorNumber, sizeof(uint32_t));
  // encrypt the sn with aes128, the key being the hash of the main key
  aes128_enc_single(key_hash, iv);
}

void print_help(void) {
  usb_serial_writeln_P(PSTR("-> Help: [i]nfo | [r]o/rw | enter [p]assphrase | [c]hange passphrase"));
}

void print_header(void) {
  usb_serial_writeln_P(FIRMWARE_VERSION);
}

#if defined(USE_SDCARD)
void print_sd_card_info() {
  usb_serial_write_P(PSTR("manuf:    0x")); hexprint(&sd_card_info.manufacturer,1);
  usb_serial_write_P(PSTR("oem:      ")); usb_serial_writeln((char*) sd_card_info.oem);
  usb_serial_write_P(PSTR("product:  ")); usb_serial_writeln((char*) sd_card_info.product);
  usb_serial_write_P(PSTR("revis:    ")); hexprint(&sd_card_info.revision,1);
  usb_serial_write_P(PSTR("serial:   0x")); hexprint((uint8_t*)(&sd_card_info.serial),1);
  usb_serial_write_P(PSTR("date mfd: ")); usb_serial_write_dec8(sd_card_info.manufacturing_month);
                                          usb_serial_putchar('/');
                                          usb_serial_writeln_dec8(sd_card_info.manufacturing_year);
  usb_serial_write_P(PSTR("size:     ")); usb_serial_write_dec32(sd_card_info.capacity / 1024 / 1024);
                                          usb_serial_writeln_P(PSTR("MB"));
  usb_serial_write_P(PSTR("content:  ")); (sd_card_info.flag_copy ?
                                           usb_serial_writeln_P(PSTR("original")) :
                                           usb_serial_writeln_P(PSTR("copy")));
  usb_serial_write_P(PSTR("writable: ")); (sd_card_info.flag_write_protect ?
                                           usb_serial_write_P(PSTR("no")) :
                                           usb_serial_write_P(PSTR("yes")) );
  usb_serial_write_P(PSTR(" temp: ")); (sd_card_info.flag_write_protect_temp ?
                                        usb_serial_writeln_P(PSTR("no")) :
                                        usb_serial_writeln_P(PSTR("yes")) );
}
#endif

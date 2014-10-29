/*
 * crypto.c
 * (c) 2014 flabbergast
 *  Provide some easy-to-use crypto functions.
 *  Credits:
 *   - the actual heavy lifting is done by avr-crypto-lib
 *     https://git.cryptolib.org/avr-crypto-lib.git
 *   - some code comes from https://github.com/DavyLandman/AESLib (*_single functions)
 *   - hardware AES code based on ATMEL's example code
 */

#include "crypto.h"
#include "aes.h" // software AES
#include <avr/pgmspace.h>
#include <string.h> // memcpy

#if (defined(__AVR_ATxmega128A3U__)) // use hardware AES accelerator on the xmega for AES128

// Encrypt or decrypt a single 128bit block. data and key are assumed
//  to be 16 uint8_t's. The mode is directly used to set AES.CTRL register.
bool aes128_do_single(const uint8_t* key, void* data, uint8_t mode) {
  bool ok;
  uint8_t i;

  // Load key into AES key memory.
  for(i = 0; i < 16; i++)
    AES.KEY = key[i];

  // Load data into AES state memory.
  for(i = 0; i < 16; i++)
    AES.STATE = *((uint8_t *)data +i);

  // Set AES mode (assumed to start AES, ie set the START bit!)
  AES.CTRL = mode;

  do{ // Wait until AES is finished or an error occurs.
  } while((AES.STATUS & (AES_SRIF_bm|AES_ERROR_bm) ) == 0);

  // If not error.
  if((AES.STATUS & AES_ERROR_bm) == 0) {
    /* Store the result. */
    for(i = 0; i < 16; i++)
      *((uint8_t *)data +i) = AES.STATE;
    ok = true;
  } else {
    ok = false;
  }

  return ok;
}

// Decrypt a single 128bit block. Both data, key are 16 uint8_t.
bool aes128_enc_single(const uint8_t* key, void* data) {
  return aes128_do_single(key, data, (AES.CTRL & (~AES_DECRYPT_bm)) | AES_START_bm);
}

// Decrypt a single 128bit block. Both data, key are 16 uint8_t.
// Note that it expects the "lastsubkey" as "key".
bool aes128_dec_single(const uint8_t* key, void* data) {
  return aes128_do_single(key, data, AES.CTRL | (AES_DECRYPT_bm | AES_START_bm));
}

// Encrypt data with AES128-CBC. key, iv are 16 uint8_t's, data assumed to have
//   data_len bytes, data_len needs to be divisible by 16 (ie padded)
//   Doing it "manually" instead of calling enc_single, because we can
//   use the XOR feature of the module
uint16_t aes128_cbc_enc(const uint8_t* key, const uint8_t* iv, void* data, const uint16_t data_len){
  if(data_len % 16 != 0) {
    return 0;
  }
  uint16_t counter = 0;
  uint8_t i;

  // the first block uses IV (will be XORed with data later)
  for(i = 0; i < 16; i++)
    AES.STATE =  iv[i];

  // set AES into encryption mode, enable XORing and AUTO start mode
  AES.CTRL = (AES.CTRL & (~AES_DECRYPT_bm))| AES_XOR_bm |AES_AUTO_bm;

  while(counter < data_len) {
    // load key into AES memory (needs to be done every time as it changes during work)
    for(i = 0; i < 16; i++)
      AES.KEY = key[i];

    // load plaintext into AES. encryption autostarts.
    for(i = 0; i < 16; i++)
      AES.STATE = *((uint8_t *)data + counter + i);

    do { // Wait until AES is finished or an error occurs.
    } while((AES.STATUS & (AES_SRIF_bm|AES_ERROR_bm) ) == 0);

    // if no error
    if((AES.STATUS & AES_ERROR_bm) == 0){
      // Store result.
      for(i = 0; i < 16; i++)
        *((uint8_t *)data + counter + i) = AES.STATE;
      // move to the next block
      counter += 16;
    } else {
      // turn off XOR and AUTO
      AES.CTRL = (AES.CTRL & ~( AES_XOR_bm |AES_AUTO_bm));
      return 0;
    }
  }

  // Turn off auto mode and xor feature.
  AES.CTRL = (AES.CTRL & ~( AES_XOR_bm |AES_AUTO_bm));

  return(counter);
}

// Generate the last subkey of the Expanded Key needed during decryption.
bool AES_lastsubkey_generate(uint8_t * key, uint8_t * last_sub_key)
{
  bool keygen_ok;
  uint8_t i;

  // do a software reset to put the AES module in a known state
  AES.CTRL = AES_RESET_bm;

  // Load key into AES key memory.
  for(i = 0; i < 16; i++)
    AES.KEY =  key[i];

  // Load dummy data into AES state memory.
  for(i = 0; i < 16; i++)
    AES.STATE =  0x00;

  // Set AES in encryption mode and start AES.
  AES.CTRL = (AES.CTRL & (~AES_DECRYPT_bm)) | AES_START_bm;

  do { // Wait until AES is finished or an error occurs.
  } while((AES.STATUS & (AES_SRIF_bm|AES_ERROR_bm) ) == 0);

  // If not error.
  if((AES.STATUS & AES_ERROR_bm) == 0) {
    // Store the last subkey.
    for(i = 0; i < 16; i++)
      last_sub_key[i] = AES.KEY;
    AES.STATUS = AES_SRIF_bm;
    keygen_ok = true;
  } else {
    AES.STATUS = AES_ERROR_bm;
    keygen_ok = false;
  }

  return keygen_ok;
}

#else // software AES128 if not on xmega128a3u

uint16_t aes128_cbc_enc(const uint8_t* key, const uint8_t* iv, void* data, const uint16_t data_len){
  if(data_len % 16 != 0) {
    return 0;
  }
  uint8_t cur_iv[16];
  uint16_t counter = 0;
  memcpy(cur_iv, iv, 16);
  while(counter < data_len) {
    for(uint8_t i=0; i<16; i++)
      *((uint8_t *)data+counter+i) ^= cur_iv[i];
    aes128_enc_single(key, data+counter);
    memcpy(cur_iv, data+counter, 16);
    counter += 16;
  }
  return(counter);
}

// encrypt single 128bit block. data is assumed to be 16 uint8_t's
// key and iv are assumed to be both 128bit thus 16 uint8_t's
bool aes128_enc_single(const uint8_t* key, void* data){
  aes128_ctx_t ctx;
  aes128_init(key, &ctx);
  aes128_enc(data, &ctx);
  return true;
}

// decrypt single 128bit block. data is assumed to be 16 uint8_t's
// key is assumed to be 128bit thus 16 uint8_t's
bool aes128_dec_single(const uint8_t* key, void* data){
  aes128_ctx_t ctx;
  aes128_init(key, &ctx);
  aes128_dec(data, &ctx);
  return true;
}

#endif

// Decrypt data with AES128-CBC. key, iv are 16 uint8_t's, data assumed to have
//   data_len bytes, data_len needs to be divisible by 16 (ie padded).
// XORed input for hardware AES doesn't help much here; could "save" on manual
//  xoring... let's do it the same way as software AES (_single call uses hardware
//  when available).
// Note that "key" is supposed to be "lastsubkey" when using hardware AES.
uint16_t aes128_cbc_dec(const uint8_t* key, const uint8_t* iv, void *data, const uint16_t data_len){
  if(data_len % 16 != 0) {
    return 0;
  }
  uint8_t next_iv[16];
  uint8_t cur_iv[16];
  uint16_t counter = 0;
  memcpy(cur_iv, iv, 16);
  while(counter < data_len) {
    memcpy(next_iv, data+counter, 16);
    aes128_dec_single(key, data+counter);
    for(uint8_t i=0; i<16; i++)
      *((uint8_t *)data+counter+i) ^= cur_iv[i];
    memcpy(cur_iv, next_iv, 16);
    counter += 16;
  }
  return(counter);
}

/* Software AES256 */

// encrypt single 128bit block. data is assumed to be 16 uint8_t's
// key is assumed to be 256bit thus 32 uint8_t's
void aes256_enc_single(const uint8_t* key, void* data){
  aes256_ctx_t ctx;
  aes256_init(key, &ctx);
  aes256_enc(data, &ctx);
}

// decrypt single 128bit block. data is assumed to be 16 uint8_t's
// key is assumed to be 256bit thus 32 uint8_t's
void aes256_dec_single(const uint8_t* key, void* data){
  aes256_ctx_t ctx;
  aes256_init(key, &ctx);
  aes256_dec(data, &ctx);
}


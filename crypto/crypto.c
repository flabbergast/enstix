/*
 * crypto.c
 * (c) 2014 flabbergast
 *  Provide some easy-to-use crypto functions.
 *  Credits:
 *   - the actual heavy lifting is done by avr-crypto-lib
 *     https://git.cryptolib.org/avr-crypto-lib.git
 *   - some code comes from https://github.com/DavyLandman/AESLib (*_single functions)
 */

#include "crypto.h"
#include <stdint.h>
#include "aes.h"
#include <avr/pgmspace.h>
#include <string.h> // memcpy

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
void aes128_enc_single(const uint8_t* key, void* data){
  aes128_ctx_t ctx;
  aes128_init(key, &ctx);
  aes128_enc(data, &ctx);
}

// decrypt single 128bit block. data is assumed to be 16 uint8_t's
// key is assumed to be 128bit thus 16 uint8_t's
void aes128_dec_single(const uint8_t* key, void* data){
  aes128_ctx_t ctx;
  aes128_init(key, &ctx);
  aes128_dec(data, &ctx);
}

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


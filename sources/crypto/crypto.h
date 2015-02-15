/*
 * crypto.h
 * (c) 2015 flabbergast
 *  Provide some easy-to-use crypto functions: header file.
 */

#ifndef CRYPTO_H
#define CRYPTO_H
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C"{
#endif
// encrypt multiple blocks of 128bit data, data_len must be divisible by 16
// key and iv are assumed to be both 128bit thus 16 uint8_t's
uint16_t aes128_cbc_enc(const uint8_t* key, const uint8_t* iv, void* data, const uint16_t data_len);

// decrypt multiple blocks of 128bit data, data_len must be divisible by 16
// key and iv are assumed to be both 128bit thus 16 uint8_t's
uint16_t aes128_cbc_dec(const uint8_t* key, const uint8_t* iv, void *data, const uint16_t data_len);

// encrypt single 128bit block. data is assumed to be 16 uint8_t's
// key is assumed to be 128bit thus 16 uint8_t's
bool aes128_enc_single(const uint8_t* key, void* data);

// decrypt single 128bit block. data is assumed to be 16 uint8_t's
// key is assumed to be 128bit thus 16 uint8_t's
bool aes128_dec_single(const uint8_t* key, void* data);

#if defined(__AVR_ATxmega128A3U__) || defined(__AVR_ATxmega128A4U__)
// hardware AES module needs a different key for decryption
// this function computes it from the main key
bool AES_lastsubkey_generate(uint8_t * key, uint8_t * last_sub_key);
#endif

#ifdef __cplusplus
}
#endif
#endif

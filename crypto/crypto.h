/*
 * crypto.h
 * (c) 2014 flabbergast
 *  Provide some easy-to-use crypto functions: header file.
 */

#ifndef CRYPTO_H
#define CRYPTO_H
#include <stdint.h>
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
void aes128_enc_single(const uint8_t* key, void* data);

// decrypt single 128bit block. data is assumed to be 16 uint8_t's
// key is assumed to be 128bit thus 16 uint8_t's
void aes128_dec_single(const uint8_t* key, void* data);

// encrypt single 128bit block. data is assumed to be 16 uint8_t's
// key is assumed to be 256bit thus 32 uint8_t's
void aes256_enc_single(const uint8_t* key, void* data);

// decrypt single 128bit block. data is assumed to be 16 uint8_t's
// key is assumed to be 256bit thus 32 uint8_t's
void aes256_dec_single(const uint8_t* key, void* data);


/*
// prepare an decrypter to use for decrypting multiple blocks later on.
// key and iv are assumed to be both 128bit thus 16 uint8_t's
(bcal_cbc_ctx_t *) aes128_cbc_dec_start(const uint8_t* key, const void* iv);

// decrypt one or more blocks of 128bit data
// data_len should be mod 16
void aes128_cbc_dec_continue(const bcal_cbc_ctx_t *ctx, void* data, const uint16_t data_len);

// cleanup decryption context
void aes128_cbc_dec_finish(const aes128_ctx_t *ctx);
*/

#ifdef __cplusplus
}
#endif
#endif

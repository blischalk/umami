#ifndef AESENCRYPT_H
#define AESENCRYPT_H

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
  unsigned char *iv, unsigned char *ciphertext);

#endif /* AESENCRYPT_H */

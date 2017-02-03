#ifndef UMAMICRYPT_H
#define UMAMICRYPT_H
#include "base64encode.h"
#include "base64decode.h"
#include "aesencrypt.h"
#include "aesdecrypt.h"

int Encode(char **dest, char *plaintext, unsigned char *key, unsigned char *iv);
#endif // UMAMICRYPT_H
